/*
 * src/xio/cm2pm.c, part of Complete Goban (game program)
 * Copyright (C) 1994-1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 */

#include <wms.h>
#include <but/but.h>
#include "cm2pm.h"


Pixmap  cm2Pm(ButEnv *env, uchar *cmap, uint cmapw,uint cmaph, uint w,uint h,
	      int pic0, int npic, bool flat)  {
  Pixmap  pic;
  Display *dpy = butEnv_dpy(env);

  pic =  XCreatePixmap(dpy, RootWindow(dpy, DefaultScreen(dpy)), w,h,
		       DefaultDepth(dpy, DefaultScreen(dpy)));
  cm2OldPm(env, cmap, cmapw, cmaph, pic0, npic, pic, flat);
  return(pic);
}


void  cm2OldPm(ButEnv *env, uchar *cmap, uint cmapw,uint cmaph,
	       int pic0, int npic, Pixmap pic, bool flat)  {
  Display *dpy;
  int  i, x, y, div, cmx, cmy;
  int  w, h;
  unsigned long  clist[256];
  XImage  *im;
  int  xaddmin, addstd, subextra, current;
  bool  done = FALSE;
  int  stripeSize = 4, stripeShift = 2;
  int  pixVal, cmap1, cmap2, color1, color2;
  Window  dummyWindow;
  unsigned int  wUns, hUns, dummyBw, dummyDepth;

  dpy = butEnv_dpy(env);
  XGetGeometry(dpy, pic, &dummyWindow, &x, &y, &wUns, &hUns,
	       &dummyBw, &dummyDepth);
  w = wUns;
  h = hUns;
  while (stripeSize * 400 < w)  {
    stripeSize += stripeSize;
    ++stripeShift;
  }
  div = (256*1024) / npic;
  for (i = 0;  i < 256;  ++i)  {
    clist[i] = butEnv_color(env, pic0 + (i*1024)/div);
  }
  if (flat)  {
    XSetForeground(dpy, env->gc, clist[128]);
    XFillRectangle(dpy, pic, env->gc, 0, 0, w, h);
  } else  {
    im = butEnv_imageCreate(env, w, h);
    xaddmin = cmapw / w;
    addstd = cmapw - (xaddmin * w);
    subextra = w;
    current = 0;
    if (!done)  {
      for (y = 0;  y < h;  ++y)  {
	cmy = ((y * cmaph) / h) * cmapw;
	cmx = 0;
	for (x = 0;  x < w;  ++x)  {
	  cmap1 = cmap[(cmx + cmy + 0) % (cmapw * cmaph)];
	  cmap2 = cmap[(cmx + cmy + 1) % (cmapw * cmaph)];
	  color1 = (x << 8) + (uint)(cmap1 << (stripeShift + 2));
	  color2 = (x << 8) + 128 +
	    (uint)((cmap1 << (stripeShift + 1)) +
		   (cmap2 << (stripeShift + 1)));
	  pixVal = (((color1 >> stripeShift) & 255) +
		    ((color2 >> stripeShift) & 255)) >> 1;
	  XPutPixel(im, x, y, clist[pixVal]);
	  cmx += xaddmin;
	  current += addstd;
	  if (current > 0)  {
	    ++cmx;
	    current -= subextra;
	  }
	}
      }
    }
    XPutImage(dpy, pic, env->gc, im, 0,0,0,0,w,h);
    butEnv_imageDestroy(im);
  }
}
