/*
 * src/abut.h, part of wmslib library
 * Copyright (C) 1995-1996 William Shubert.
 * See "configure.h.in" for more copyright information.
 */

#ifndef  _ABUT_ABUT_H_

#ifndef  _WMS_CLP_H_
#include <wms/clp.h>
#endif
#ifdef  _ABUT_ABUT_H_
  Levelization Error.
#endif

#define  _ABUT_ABUT_H_  1


/**********************************************************************
 * Types
 **********************************************************************/

/* Private. */
typedef struct Abut_struct  {
  ButEnv *env;
  const char  *ok, *cancel;
  int  butH;
  int  ulColor, lrColor, bgColor;
  Pixmap  ulPixmap, lrPixmap, bgPixmap;
  Clp  *clp;
  MAGIC_STRUCT
} Abut;


/**********************************************************************
 * Functions available
 **********************************************************************/
extern Abut  *abut_create(ButEnv *env, const char *ok, const char *cancel);
extern void  abut_destroy(Abut *a);
extern void  abut_setColors(Abut *a, int ul, int lr, int bg);
extern void  abut_setPixmaps(Abut *a, Pixmap ul, Pixmap lr, Pixmap bg);
#define  abut_setClp(a, c)  ((a)->clp = (c))
#define  abut_setButH(a, h)  ((a)->butH = (h))

#endif  /* _ABUT_ABUT_H_ */
