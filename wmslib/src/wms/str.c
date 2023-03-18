/*
 * wmslib/src/wms/str.c, part of wmslib (Library functions)
 * Copyright (C) 1995-1996 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#include <stdarg.h>
#include <wms.h>
#include <wms/str.h>


/**********************************************************************
 * Global variables
 **********************************************************************/
static char  empty = '\0';


/**********************************************************************
 * Functions
 **********************************************************************/
Str  *str_create(void)  {
  Str  *ret;

  ret = wms_malloc(sizeof(Str));
  MAGIC_SET(ret);
  ret->len = ret->maxLen = 0;
  ret->chars = &empty;
  return(ret);
}


Str  *str_createStr(const Str *s)  {
  Str  *ret;

  ret = wms_malloc(sizeof(Str));
  MAGIC_SET(ret);
  ret->len = ret->maxLen = s->len;
  ret->chars = wms_malloc(ret->len + 1);
  strcpy(ret->chars, s->chars);
  assert(strlen(ret->chars) == ret->len);
  return(ret);
}


Str  *str_createChars(const char *chars)  {
  Str  *ret;

  ret = wms_malloc(sizeof(Str));
  MAGIC_SET(ret);
  ret->len = ret->maxLen = strlen(chars);
  ret->chars = wms_malloc(ret->len + 1);
  strcpy(ret->chars, chars);
  assert(strlen(ret->chars) == ret->len);
  return(ret);
}


Str  *str_createChar(char c)  {
  Str  *ret;

  ret = wms_malloc(sizeof(Str));
  MAGIC_SET(ret);
  ret->len = ret->maxLen = 1;
  ret->chars = wms_malloc(ret->len + 1);
  ret->chars[0] = c;
  ret->chars[1] = '\0';
  assert(strlen(ret->chars) == ret->len);
  return(ret);
}


void  str_destroy(Str *s)  {
  assert(MAGIC(s));
  if (s->chars != &empty)
    wms_free(s->chars);
  MAGIC_UNSET(s);
  wms_free(s);
}


void  str_init(Str *s)  {
  MAGIC_SET(s);
  s->len = s->maxLen = 0;
  s->chars = &empty;
}


void  str_initStr(Str *s, const Str *src)  {
  MAGIC_SET(s);
  s->len = s->maxLen = src->len;
  s->chars = wms_malloc(s->len + 1);
  strcpy(s->chars, src->chars);
  assert(strlen(s->chars) == s->len);
}


void  str_initChars(Str *s, const char *src)  {
  MAGIC_SET(s);
  s->len = s->maxLen = strlen(src);
  s->chars = wms_malloc(s->len + 1);
  strcpy(s->chars, src);
  assert(strlen(s->chars) == s->len);
}


void  str_initCharsLen(Str *s, const char *src, int len)  {
  assert(strlen(src) >= len);
  MAGIC_SET(s);
  s->len = s->maxLen = len;
  s->chars = wms_malloc(s->len + 1);
  strncpy(s->chars, src, len);
  s->chars[len] = '\0';
  assert(strlen(s->chars) == s->len);
}


void  str_deinit(Str *s)  {
  assert(MAGIC(s));
  if (s->chars != &empty)
    wms_free(s->chars);
  MAGIC_UNSET(s);
}


void  str_copy(Str *dest, const Str *src)  {
  char  *newChars;

  assert(MAGIC(dest));
  assert(MAGIC(src));
  assert(strlen(src->chars) == src->len);
  if (src->len > dest->maxLen)  {
    dest->maxLen = src->len * 2;
    newChars = wms_malloc(dest->maxLen + 1);
    if (dest->chars != &empty)
      wms_free(dest->chars);
    dest->chars = newChars;
  }
  strcpy(dest->chars, src->chars);
  dest->len = src->len;
  assert(strlen(dest->chars) == dest->len);
}


void  str_copyChars(Str *dest, const char *src)  {
  int  srcLen = strlen(src);
  char  *newChars;

  assert(MAGIC(dest));
  if (srcLen > dest->maxLen)  {
    dest->maxLen = srcLen * 2;
    newChars = wms_malloc(dest->maxLen + 1);
    if (dest->chars != &empty)
      wms_free(dest->chars);
    dest->chars = newChars;
  }
  strcpy(dest->chars, src);
  dest->len = srcLen;
  assert(strlen(dest->chars) == dest->len);
}


void  str_copyCharsLen(Str *dest, const char *src, int srcLen)  {
  char  *newChars;

  assert(MAGIC(dest));
  assert(strlen(src) >= srcLen);
  if (srcLen > dest->maxLen)  {
    dest->maxLen = srcLen * 2;
    newChars = wms_malloc(dest->maxLen + 1);
    if (dest->chars != &empty)
      wms_free(dest->chars);
    dest->chars = newChars;
  }
  strncpy(dest->chars, src, srcLen);
  dest->chars[dest->len = srcLen] = '\0';
  assert(strlen(dest->chars) == dest->len);
}


void  str_copyChar(Str *dest, char src)  {
  char  *newChars;

  assert(MAGIC(dest));
  if (1 > dest->maxLen)  {
    dest->maxLen = 32;
    newChars = wms_malloc(dest->maxLen + 1);
    if (dest->chars != &empty)
      wms_free(dest->chars);
    dest->chars = newChars;
  }
  dest->chars[0] = src;
  dest->chars[1] = '\0';
  dest->len = 1;
  assert(src != '\0');
}


void  str_cat(Str *dest, const Str *src)  {
  char  *newChars;

  assert(MAGIC(dest));
  assert(MAGIC(src));
  assert(strlen(dest->chars) == dest->len);
  assert(strlen(src->chars) == src->len);
  if (dest->len + src->len > dest->maxLen)  {
    dest->maxLen = (dest->len + src->len) * 2;
    newChars = wms_malloc(dest->maxLen + 1);
    strcpy(newChars, dest->chars);
    if (dest->chars != &empty)
      wms_free(dest->chars);
    dest->chars = newChars;
  }
  strcpy(dest->chars + dest->len, src->chars);
  dest->len += src->len;
  assert(strlen(dest->chars) == dest->len);
}


void  str_catChars(Str *dest, const char *src)  {
  int  srcLen = strlen(src);
  char  *newChars;

  assert(MAGIC(dest));
  assert(strlen(dest->chars) == dest->len);
  if (dest->len + srcLen > dest->maxLen)  {
    dest->maxLen = (dest->len + srcLen) * 2;
    newChars = wms_malloc(dest->maxLen + 1);
    strcpy(newChars, dest->chars);
    if (dest->chars != &empty)
      wms_free(dest->chars);
    dest->chars = newChars;
  }
  strcpy(dest->chars + dest->len, src);
  dest->len += srcLen;
}


void  str_catCharsLen(Str *dest, const char *src, int srcLen)  {
  char  *newChars;

  assert(MAGIC(dest));
  assert(strlen(dest->chars) == dest->len);
  assert(strlen(src) >= srcLen);
  if (dest->len + srcLen > dest->maxLen)  {
    dest->maxLen = (dest->len + srcLen) * 2;
    newChars = wms_malloc(dest->maxLen + 1);
    strcpy(newChars, dest->chars);
    if (dest->chars != &empty)
      wms_free(dest->chars);
    dest->chars = newChars;
  }
  strncpy(dest->chars + dest->len, src, srcLen);
  dest->len += srcLen;
  dest->chars[dest->len] = '\0';
}


void  str_catChar(Str *dest, char src)  {
  char  *newChars;

  assert(MAGIC(dest));
  assert(strlen(dest->chars) == dest->len);
  if (dest->len + 1 > dest->maxLen)  {
    dest->maxLen = (dest->len + 1) * 2;
    newChars = wms_malloc(dest->maxLen + 1);
    strcpy(newChars, dest->chars);
    if (dest->chars != &empty)
      wms_free(dest->chars);
    dest->chars = newChars;
    assert(strlen(dest->chars) == dest->len);
  }
  dest->chars[dest->len++] = src;
  dest->chars[dest->len] = '\0';
  assert(strlen(dest->chars) == dest->len);
}


void  str_catInt(Str *dest, int src)  {
  char  srcChars[SIZEOF_INT * 3 + 1];

  assert(MAGIC(dest));
  sprintf(srcChars, "%d", src);
  str_catChars(dest, srcChars);
}


void  str_print(Str *dest, const char *fmt, ...)  {
  va_list  ap;
  char  tmp[40];
  static char  tmpFmt[] = "%0xd";
  double  dval;
  const char  *sVal;

  assert(MAGIC(dest));
  str_copyChars(dest, "");
  va_start(ap, fmt);
  while (*fmt)  {
    if (*fmt != '%')
      str_catChar(dest, *fmt);
    else  {
      ++fmt;
      switch(*fmt)  {
      case '0':
	assert(fmt[2] == 'd');
	tmpFmt[2] = fmt[1];
	sprintf(tmp, tmpFmt,  va_arg(ap, int));
	str_catChars(dest, tmp);
	fmt += 2;
	break;
      case 'c':
	str_catChar(dest, (char)va_arg(ap, int));
	break;
      case 'd':
	str_catInt(dest, va_arg(ap, int));
	break;
      case 'g':
	dval = va_arg(ap, double);
	if ((int)dval == dval)  {
	  sprintf(tmp, "%d", (int)dval);
	} else  {
	  sprintf(tmp, "%.1f", dval);
	}
	str_catChars(dest, tmp);
	break;
      case 's':
	sVal = va_arg(ap, char *);
	assert(sVal != NULL);
	str_catChars(dest, sVal);
	break;
      default:
	str_catChar(dest, *fmt);
	break;
      }
    }
    ++fmt;
  }
  va_end(ap);
}


int  str_alphaCmp(const void *a, const void *b)  {
  const Str  *sa = a, *sb = b;

  assert(MAGIC(sa));
  assert(MAGIC(sb));
  return(strcmp(sa->chars, sb->chars));
}
