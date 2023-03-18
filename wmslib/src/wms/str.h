/*
 * wmslib/src/wms/str.h, part of wmslib (Library functions)
 * Copyright (C) 1994-1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 */

#ifndef  _WMS_STR_H_
#define  _WMS_STR_H_  1

#ifndef  _WMS_H_
#include <wms.h>
#endif


/**********************************************************************
 * Data Types
 **********************************************************************/
typedef struct Str_struct  {
  char  *chars;
  int  len;
  int  maxLen;

  MAGIC_STRUCT
} Str;

/**********************************************************************
 * Functions
 **********************************************************************/
extern Str  *str_create(void);
extern Str  *str_createStr(const Str *s);
extern Str  *str_createChars(const char *chars);
extern Str  *str_createChar(char c);
extern void  str_destroy(Str *s);

extern void  str_init(Str *s);
extern void  str_initStr(Str *s, const Str *src);
extern void  str_initChars(Str *s, const char *src);
extern void  str_initCharsLen(Str *s, const char *src, int len);

extern void  str_deinit(Str *s);

#define  str_len(s)    ((s)->len)
#define  str_chars(s)  ((const char *)((s)->chars))

extern void  str_copy(Str *dest, const Str *src);
extern void  str_copyChars(Str *dest, const char *src);
extern void  str_copyCharsLen(Str *dest, const char *src, int len);
extern void  str_copyChar(Str *dest, char src);
extern void  str_print(Str *dest, const char *fmt, ...);
extern void  str_cat(Str *dest, const Str *src);
extern void  str_catChars(Str *dest, const char *src);
extern void  str_catCharsLen(Str *dest, const char *src, int len);
extern void  str_catChar(Str *dest, char src);
extern void  str_catInt(Str *dest, int src);
/* str_alphaCmd is useful in qsort and bsearch. */
extern int  str_alphaCmp(const void *a, const void *b);
#define  str_clip(s, l)                   \
  do  {                                   \
    if ((l) < (s)->len)                   \
      (s)->chars[(s)->len = (l)] = '\0';  \
  } while(0)

#endif  /* _WMS_STR_H_ */
