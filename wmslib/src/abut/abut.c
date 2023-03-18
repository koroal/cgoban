/*
 * src/abut.c, part of wmslib library
 * Copyright (C) 1995-1996 William Shubert.
 * See "configure.h.in" for more copyright information.
 */

#include <wms.h>
#include <but/but.h>
#ifdef  _ABUT_ABUT_H_
  Levelization Error.
#endif
#include <abut/abut.h>


/**********************************************************************
 * Functions
 **********************************************************************/
Abut  *abut_create(ButEnv *env, const char *ok, const char *cancel)  {
  Abut  *a;

  a = wms_malloc(sizeof(Abut));
  MAGIC_SET(a);
  a->env = env;
  a->ok = ok;
  a->cancel = cancel;
  a->butH = 0;
  a->ulColor = BUT_LIT;
  a->lrColor = BUT_SHAD;
  a->bgColor = BUT_BG;
  a->ulPixmap = None;
  a->lrPixmap = None;
  a->bgPixmap = None;
  a->clp = NULL;
  return(a);
}


void  abut_destroy(Abut *a)  {
  assert(MAGIC(a));
  MAGIC_UNSET(a);
  wms_free(a);
}


void  abut_setColors(Abut *a, int ul, int lr, int bg)  {
  assert(MAGIC(a));
  if (ul >= 0)  {
    a->ulColor = ul;
    a->ulPixmap = None;
  }
  if (lr >= 0)  {
    a->lrColor = lr;
    a->lrPixmap = None;
  }
  if (bg >= 0)  {
    a->bgColor = bg;
    a->bgPixmap = None;
  }
}


void  abut_setPixmaps(Abut *a, Pixmap ul, Pixmap lr, Pixmap bg)  {
  if (ul != None)  {
    a->ulColor = -1;
    a->ulPixmap = ul;
  }
  if (lr != None)  {
    a->lrColor = -1;
    a->lrPixmap = lr;
  }
  if (bg != None)  {
    a->bgColor = -1;
    a->bgPixmap = bg;
  }
}

