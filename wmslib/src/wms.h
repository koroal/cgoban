/*
 * wmslib/include/wms.h, part of wmslib (Library functions)
 * Copyright (C) 1994 William Shubert.
 * See "configure.h.in" for more copyright information.
 *
 * This contains a bunch of definitions I always like to have.
 * This should ALWAYS be AFTER the last system include, BEFORE the first
 *   local include.
 */

#ifndef  _WMS_H_
#define  _WMS_H_  1

#include <configure.h>

#ifdef  STDC_HEADERS
# include <stdlib.h>
# include <unistd.h>
#endif  /* STDC_HEADERS */
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <ctype.h>
#if  TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else  /* !AC_TIME_WITH_SYS_TIME */
# if  HAVE_SYS_TIME_H
#  include <sys/time.h>
# else  /* !HAVE_SYS_TIME_H */
#  include <time.h>
# endif  /* HAVE_SYS_TIME_H */
#endif  /* AC_TIME_WITH_SYS_TIME */
#if  X11_DISP
# include <X11/Xlib.h>
# include <X11/Xutil.h>
# include <X11/keysym.h>
#endif  /* X11_DISP */
#if  SUN_SOUND
# include <errno.h>
# include <stropts.h>
# include <sun/audioio.h>
#endif  /* SUN_SOUND */
/* Get all these headers out of the way here. */
#include <stdio.h>
#include <errno.h>
#if  HAVE_STDLIB_H
# include <stdlib.h>
#endif
#if  HAVE_UNISTD_H
# include <unistd.h>
#endif  /* HAVE_UNISTD_H */

#if  HAVE_ASSERT_H && DEBUG
#include <assert.h>
#else
#define  assert(ignore)  
#endif  /* HAVE_ASSERT_H && DEBUG */

#if  STDC_HEADERS || HAVE_STRING_H
#include <string.h>
/* An ANSI string.h and pre-ANSI memory.h might conflict. */
#if !STDC_HEADERS && HAVE_MEMORY_H
#include <memory.h>
#endif  /* not STDC_HEADERS and HAVE_MEMORY_H */
#else  /* not STDC_HEADERS and not HAVE_STRING_H */
#include <strings.h>
#define  memcpy(d,s,n)  bcopy((s),(d),(n))
#define  strchr  index
#define  strrchr  rindex
#endif  /* Not STDC_HEADERS and not HAVE_STRING_H */

#ifdef   bool
#undef   bool
#endif  /* bool */
#define  bool int

#ifndef  TRUE
#define  FALSE  0
#define  TRUE   1
#endif

/* int32 must be AT LEAT 32 bits.  More is OK. */
#ifdef  int32
#undef  int32
#endif  /* int32 */
#if  (SIZEOF_INT >= 4)
#define  int32  int
#else  /* SIZEOF_INT < 4 */
#define  int32  long
#endif

#ifdef  uint32
#undef  uint32
#endif  /* uint32 */
#define  uint32  unsigned int32

#ifdef  uchar
#undef  uchar
#endif  /* uchar */
#define  uchar  unsigned char

#ifdef  ushort
#undef  ushort
#endif  /* ushort */
#define  ushort  unsigned short

#ifdef  uint
#undef  uint
#endif  /* uint */
#define  uint  unsigned int

#ifdef  ulong
#undef  ulong
#endif
#define  ulong  unsigned long

#define  int32_max  0x7fffffff
#define  int32_min  (-int32_max-1)

#define  int_max   ((int)((((1 << (8*(sizeof(int)-2)))-1)<<1)+1))
#define  int_min   (-int_max-1)
#define  uint_max  ((((1U << (8*(sizeof(uint)-1)))-1)<<1)+1)

#define  long_max   ((long)((((1L << (8*(sizeof(long)-2)))-1)<<1)+1))
#define  long_min   (-long_max-1)
#define  ulong_max  ((((1UL << (8*(sizeof(ulong)-1)))-1)<<1)+1)


/*
 * This atoi sets err to TRUE if there's an overflow or a non-digit
 *   character around.  If err is NULL it is just like regular atoi().
 */
extern int  wms_atoi(const char *str, bool *err);
extern double  wms_atof(const char *str, bool *err);


/*
 * Debugging aide 1 - The MAGIC macros.
 *
 * In every dynamically allocated structure, add this:
 * #if  DEBUG
 *   int  magic;
 * #endif  / * DEBUG * /
 * Then, every time you alloc a structure, _immediately_ do a MAGIC_SET(foo)
 *   on the newly freed buffer.  Last thing before you free a structure, do
 *   a MAGIC_UNSET(foo).  Then, before you access a structure, always first
 *   say "assert(MAGIC(foo))".  This way, if you touch a structure after you
 *   free it, an assertion will trip!  And if debug is set to zero, the
 *   checks all disappear and leave you with an efficient program.
 */
#if  DEBUG
#define  MAGIC_NUM  31415927  /* Any random number will do. */
#define  MAGIC_SET(x)    ((x)->magic = MAGIC_NUM)
#define  MAGIC_UNSET(x)  ((x)->magic = 0)
#define  MAGIC(x)        ((x)->magic == MAGIC_NUM)
#define  MAGICNULL(x)    (((x) == NULL) || ((x)->magic == MAGIC_NUM))
#define  MAGIC_STRUCT    int magic;
#else
#define  MAGIC_SET(x)    
#define  MAGIC_UNSET(x)  
#define  MAGIC(x)        1
#define  MAGIC_STRUCT
#endif  /* magic */

/*
 * Debugging aide 2 - The wms_malloc routines
 *
 * This makes it much easier to spot memory leaks and objects freed multiple
 *   times.  Basically, every time you call wms_alloc_stat(), you will see
 *   how much memory is allocated.  If this grows indefinitely, you know
 *   that you have a memory leak.  Doing wms_alloc_ring_show() will list
 *   EVERY wms_malloc that hasn't yet been freed.  Ugh.  Probably way
 *   more than you need to see.  So when you call wms_alloc_ring_reset(),
 *   the list gets cleared, so call wms_alloc_ring_reset(), do the stuff
 *   that shouldn't leave buffers allocated but does, then call
 *   wms_alloc_ring_show() and voila!  The files and lines where your
 *   memory leak is coming from!
 * As in the MAGIC macros, when DEBUG is set to zero this stuff all
 *   disappears.  Good thing, too, because it consumes bunches of memory and
 *   a significant amount of CPU cycles.
 */
#if  DEBUG
extern void  wms_free(void *deadbuf);
extern void  wms_alloc_stat(void);
#define  wms_malloc(x)  wms_malloc_debug(x, __FILE__, __LINE__)
extern void  *wms_malloc_debug(uint bufsize, const char *file, int line);
void  wms_alloc_ring_reset(void);
void  wms_alloc_ring_show(void);
#else
#define  wms_free  free
extern void  *wms_malloc(uint bufsize);
#endif

extern const char  *wms_progname;

/*
 * A StdInt32 is an int that is exactly 4 bytes long and in little endian
 *   format.
 */
#if  SIZEOF_INT == 4
typedef int  StdInt32;
#elif  SIZEOF_SHORT == 4
typedef short  StdInt32;
#elif  SIZEOF_LONG == 4
typedef long  StdInt32;
#else
#error  You must have a 4 byte data type to compile this program!
#endif

#if  !WORDS_BIGENDIAN

#define  stdInt32_int(i)  (i)
#define  int_stdInt32(i)  (i)

#else  /* Big endian machine. */

extern int  stdInt32_int(StdInt32 i);
extern StdInt32  int_stdInt32(int i);

#endif


#if  !HAVE_STRERROR
extern const char  *strerror(int error);
#endif

#if  !HAVE_STRCASECMP
extern int  strcasecmp(const char *s1, const char *s2);
#endif

#if  !HAVE_MEMMOVE
extern void  *memmove(void *dest, const void *src, size_t n);
#endif

#if  !HAVE_GETDTABLESIZE
extern int  getdtablesize(void);
#endif

#endif  /* _WMS_H_ */
