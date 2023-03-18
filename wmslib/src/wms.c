/*
 * wmslib/src/wms.c, part of wmslib (Library functions)
 * Copyright (C) 1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 */

#include <wms.h>
#if  !HAVE_GETDTABLESIZE
#include <sys/resource.h>
#endif

#if  DEBUG

typedef struct alloc_struct  {
  int  magic;
  int  size;
  const char  *file;
  int   line;
  struct alloc_struct  *next, *prev;
} alloc_t;

static int  alloc_count=0, total_bytes=0, check=FALSE;
static alloc_t  *ring_head = NULL;


void  *wms_malloc_debug(uint bufsize, const char *file, int line)  {
  void  *ret;
  alloc_t  *alloc;

  if (bufsize == 0)
    return(NULL);
  ++alloc_count;
  total_bytes += bufsize;
  bufsize += sizeof(alloc_t);
  alloc = malloc(bufsize);
  assert(alloc != NULL);
  if (alloc == NULL)  {
    fprintf(stderr, "%s: FATAL ERROR: Failed to allocate %d bytes.\n",
	    wms_progname, bufsize);
    exit(1);
  }
  ret = (void *)((int)alloc + sizeof(alloc_t));
  alloc->magic = 0x91325832;
  alloc->size  = bufsize - sizeof(alloc_t);
  alloc->file  = file;
  alloc->line  = line;
  if (ring_head == NULL)  {
    ring_head = alloc;
    alloc->next = alloc->prev = alloc;
  } else  {
    alloc->next = ring_head;
    alloc->prev = ring_head->prev;
    alloc->next->prev = alloc;
    alloc->prev->next = alloc;
  }
  return(ret);
}


void  wms_free(void *deadbuf)  {
  alloc_t  *alloc;

  assert(deadbuf != NULL);
  alloc = (alloc_t *)((int)deadbuf - sizeof(alloc_t));
  --alloc_count;
  assert(alloc->magic == 0x91325832);
  total_bytes -= alloc->size;
  alloc->magic = 0;
  if (ring_head == alloc)  {
    if (alloc->next == alloc)
      ring_head = NULL;
    else  {
      ring_head = alloc->next;
      alloc->next->prev = alloc->prev;
      alloc->prev->next = alloc->next;
    }
  } else if (alloc->next != alloc)  {
    alloc->next->prev = alloc->prev;
    alloc->prev->next = alloc->next;
  }
  free(alloc);
}


void  wms_alloc_ring_reset(void)  {
  ring_head = NULL;
}


void  wms_alloc_ring_show(void)  {
  alloc_t  *alloc;

  if (ring_head == NULL)  {
    printf("*** wms_alloc: Empty ring\n");
  } else  {
    alloc = ring_head;
    do  {
      printf("*** wms_alloc: %4d bytes; file \"%s\", line %d.\n",
	     alloc->size, alloc->file, alloc->line);
      alloc = alloc->next;
    } while (alloc != ring_head);
    printf("\n");
  }
}


void  wms_alloc_stat(void)  {
  fprintf(stderr, "Count of allocated buffers: %d\n", alloc_count);
  fprintf(stderr, "     Total bytes allocated: %d\n", total_bytes);
  check = TRUE;
}



#else  /* !DEBUG */


void  *wms_malloc(uint bufsize)  {
  void  *ret;

  if (bufsize == 0)
    return(NULL);
  ret = malloc(bufsize);
  assert(ret != NULL);
  if (ret == NULL)  {
    fprintf(stderr, "%s: FATAL ERROR: Failed to allocate %d bytes.\n",
	    wms_progname, bufsize);
    exit(1);
  }
  return(ret);
}

#endif  /* DEBUG */


#if  !HAVE_STRERROR

/*
 * Do a fake, really bad strerror() function.
 */
const char  *strerror(int error)  {
  static char  out[20];

  sprintf(out, "System error %d", error);
  return(out);
}

#endif  /* !HAVE_STRERROR */


#if  !HAVE_GETDTABLESIZE

int  getdtablesize(void)  {
  struct rlimit  tmp_rlim;

  getrlimit(RLIMIT_NOFILE, &tmp_rlim);
  return(tmp_rlim.rlim_cur);
}

#endif  /* !HAVE_GETDTABLESIZE */


int  wms_atoi(const char *str, bool *err)  {
  bool  dummy;
  bool  negative = FALSE, inNum = FALSE, gotNum = FALSE, doneNum = FALSE;
  int  i, v = 0, newV;

  if (err == NULL)
    err = &dummy;
  *err = FALSE;
  for (i = 0;  str[i];  ++i)  {
    if (isspace(str[i]))  {
      if (inNum)
	doneNum = TRUE;
    } else if (str[i] == '-')  {
      if (inNum)  {
	*err = TRUE;
	return(0);
      } else  {
	inNum = TRUE;
	negative = TRUE;
      }
    } else if (isdigit(str[i]))  {
      if (doneNum)  {
	*err = TRUE;
	return(0);
      } else  {
	inNum = TRUE;
	gotNum = TRUE;
	newV = v*10 + (int)(str[i] - '0');
	if (newV / 10 != v)  {
	  *err = TRUE;
	  return(0);
	}
	v = newV;
      }
    } else  {
      *err = TRUE;
      return(0);
    }
  }
  if (!gotNum)  {
    *err = TRUE;
    return(0);
  }
  if (negative)
    return(-v);
  else
    return(v);
}


double  wms_atof(const char *str, bool *err)  {
  int  i;
  bool  decimal = FALSE;

  i = 0;
  if (str[0] == '-')
    ++i;
  while (str[i])  {
    if (str[i] == '.')  {
      if (decimal)  {
	if (err)
	  *err = TRUE;
	return(0.0);
      }
      decimal = TRUE;
    } else if (!isdigit(str[i]))  {
      if (err)
	*err = TRUE;
      return(0.0);
    }
    ++i;
  }
  if (err)
    *err = FALSE;
  return(atof(str));
}


#if  WORDS_BIGENDIAN

int  stdInt32_int(StdInt32 i)  {
  return(((i >> 24) & 0x000000ff) |
	 ((i >>  8) & 0x0000ff00) |
	 ((i <<  8) & 0x00ff0000) |
	 ((i << 24) & 0xff000000));
}


StdInt32  int_stdInt32(int i)  {
  return(((i >> 24) & 0x000000ff) |
	 ((i >>  8) & 0x0000ff00) |
	 ((i <<  8) & 0x00ff0000) |
	 ((i << 24) & 0xff000000));
}

#endif  /* WORDS_BIGENDIAN */


#if  !HAVE_MEMMOVE

void  *memmove(void *dest, const void *src, size_t n)  {
  char  *dc;
  const char  *sc;
  int  i;

  dc = dest;
  sc = src;
  if ((dc + n < sc) || (sc + n < dc))
    return(memcpy(dest, src, n));
  else  {
    if (dc < sc)  {
      for (i = 0;  i < n;  ++i)  {
	dc[i] = sc[i];
      }
    } else if (sc < dc)  {
      for (i = n - 1;  i >= 0;  --i)  {
	dc[i] = sc[i];
      }
    }
  }
  return(dest);
}

#endif  /* !HAVE_MEMMOVE */

#if  !HAVE_STRCASECMP

int  strcasecmp(const char *s1, const char *s2)  {
  int  i;
  char  c1, c2;

  for (i = 0;  s1[i];  ++i)  {
    c1 = s1[i];
    c2 = s2[i];
    if ((c1 >= 'a') && (c1 <= 'z'))
      c1 += 'A' - 'a';
    if ((c2 >= 'a') && (c2 <= 'z'))
      c2 += 'A' - 'a';
    if (c1 != c2)
      return((int)c1 - (int)c2);
  }
  if (s2[i])
    return(-1);
  return(0);
}

#endif  /* !HAVE_STRCASECMP */
