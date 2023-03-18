/*
 * wmslib/src/abut/swin.c, part of wmslib (Library functions)
 * Copyright (C) 1994-1996 William Shubert.
 * See "configure.h.in" for more copyright information.
 */

#include <wms.h>

#if  X11_DISP

#include <but/but.h>
#include <but/slide.h>
#include <but/canvas.h>
#include <but/box.h>
#include <but/ctext.h>
#include <but/timer.h>
#ifdef  _ABUT_SWIN_H_
  Levelization Error.
#endif
#include "swin.h"


/**********************************************************************
 * Forward Declarations
 **********************************************************************/
static void  draw_uparrow(void *packet, ButWin *win,
			  int x, int y, int w, int h);
static void  draw_downarrow(void *packet, ButWin *win,
			    int x, int y, int w, int h);
static void  draw_leftarrow(void *packet, ButWin *win,
			    int x, int y, int w, int h);
static void  draw_rightarrow(void *packet, ButWin *win,
			     int x, int y, int w, int h);
static ButOut  pressUp(But *but), pressDown(But *but);
static ButOut  pressLeft(But *but), pressRight(But *but);
static ButOut  releaseVArrow(But *but), releaseHArrow(But *but);
static ButOut  v_moved(But *but, int new_yoff, bool newPress);
static ButOut  hMoved(But *but, int new_xOff, bool newPress);
static void  redo_slider(void *packet, int xoff, int yoff,
			 int w, int h, int viewW, int viewH);


/**********************************************************************
 * Functions
 **********************************************************************/
AbutSwin  *abutSwin_create(void *packet, ButWin *parent, int layer, uint flags,
			   ButWinFunc resize)  {
  AbutSwin  *swin;
  ButEnv  *env = butWin_env(parent);
  static ButKey  upKeys[] = {{XK_Prior, 0, ShiftMask}, {0,0,0}};
  static ButKey  downKeys[] = {{XK_Next, 0, ShiftMask}, {0,0,0}};
  static ButKey  leftKeys[] = {{XK_Left, 0, ShiftMask}, {0,0,0}};
  static ButKey  rightKeys[] = {{XK_Right, 0, ShiftMask}, {0,0,0}};

  butEnv_setChar(env, 1.0, ABUT_UPPIC,    draw_uparrow,    NULL);
  butEnv_setChar(env, 1.0, ABUT_DOWNPIC,  draw_downarrow,  NULL);
  butEnv_setChar(env, 1.0, ABUT_LEFTPIC,  draw_leftarrow,  NULL);
  butEnv_setChar(env, 1.0, ABUT_RIGHTPIC, draw_rightarrow, NULL);
  swin = wms_malloc(sizeof(AbutSwin));
  MAGIC_SET(swin);
  if (flags & (ABUTSWIN_LSLIDE | ABUTSWIN_RSLIDE))  {
    swin->up = butAct_vCreate(pressUp, releaseVArrow,
			      swin, parent, layer,
			      BUT_DRAWABLE|BUT_PRESSABLE,
			      ABUT_UPPIC, BUT_SRIGHT|BUT_SLEFT);
    but_setKeys(swin->up, upKeys);
    swin->down = butAct_vCreate(pressDown, releaseVArrow,
				swin, parent, layer,
				BUT_DRAWABLE|BUT_PRESSABLE,
				ABUT_DOWNPIC, BUT_SRIGHT|BUT_SLEFT);
    but_setKeys(swin->down, downKeys);
    swin->vSlide = butSlide_vCreate(v_moved, swin, parent, layer,
				    BUT_DRAWABLE|BUT_PRESSABLE,
				    100,10,10);
  }
  if (flags & (ABUTSWIN_TSLIDE | ABUTSWIN_BSLIDE))  {
    swin->left = butAct_vCreate(pressLeft, releaseHArrow,
				swin, parent, layer,
				BUT_DRAWABLE|BUT_PRESSABLE,
				ABUT_LEFTPIC, BUT_SRIGHT|BUT_SLEFT);
    but_setKeys(swin->left, leftKeys);
    swin->right = butAct_vCreate(pressRight, releaseHArrow,
				 swin, parent, layer,
				 BUT_DRAWABLE|BUT_PRESSABLE,
				 ABUT_RIGHTPIC, BUT_SRIGHT|BUT_SLEFT);
    but_setKeys(swin->right, rightKeys);
    swin->hSlide = butSlide_hCreate(hMoved, swin, parent, layer,
				    BUT_DRAWABLE|BUT_PRESSABLE,
				    100,10,10);
  }
  swin->box = butBox_create(parent, layer, BUT_DRAWABLE);
  butBox_setColors(swin->box, BUT_SHAD, BUT_LIT);
  swin->win = butCan_create(swin, parent, layer, resize, NULL, redo_slider);
  swin->flags = flags;
  swin->timer = NULL;
  swin->xCenter = swin->yCenter = 0.0;
  swin->packet = packet;
  return(swin);
}


void  abutSwin_resize(AbutSwin *swin, int x, int y, int w, int h,
		       int slideW, int lineH)  {
  int  bx, by, bw, bh;
  int  canWinW, canWinH, canViewW, canViewH;
  int  butbw = butEnv_stdBw(butWin_env(swin->win));
  int  new_yoff = 0, new_xoff = 0;
  float  yratio = swin->yCenter, xratio = swin->xCenter;

  assert(MAGIC(swin));
  if (x == BUT_NOCHANGE)
    x = swin->x;
  if (y == BUT_NOCHANGE)
    y = swin->y;
  if (w == BUT_NOCHANGE)
    w = swin->w;
  if (h == BUT_NOCHANGE)
    h = swin->h;
  if (slideW == BUT_NOCHANGE)
    slideW = swin->slideW;
  if (lineH == BUT_NOCHANGE)
    lineH = swin->lineH;
  swin->x = x;
  swin->y = y;
  swin->w = w;
  swin->h = h;
  swin->slideW = slideW;
  swin->lineH = lineH;

  bx = x;
  by = y;
  bw = w;
  bh = h;

  if (swin->flags & ABUTSWIN_LSLIDE)  {
    bx += slideW;
    bw -= slideW;
  } else if (swin->flags & ABUTSWIN_RSLIDE)  {
    bw -= slideW;
  }
  if (swin->flags & ABUTSWIN_TSLIDE)  {
    by += slideW;
    bh -= slideW;
  } else if (swin->flags & ABUTSWIN_BSLIDE)  {
    bh -= slideW;
  }
  butCan_resizeView(swin->win, bx+butbw,by+butbw, bw-2*butbw,bh-2*butbw,
		    FALSE);
  canViewW = bw - butbw*2;
  canViewH = bh - butbw*2;
  canWinW = butWin_w(swin->win);
  canWinH = butWin_h(swin->win);
  if (swin->flags & (ABUTSWIN_TSLIDE|ABUTSWIN_BSLIDE))  {
    new_xoff = xratio * (canWinW - canViewW) + 0.5;
  }
  if (swin->flags & (ABUTSWIN_LSLIDE|ABUTSWIN_RSLIDE))  {
    new_yoff = yratio * (canWinH - canViewH) + 0.5;
  }
  if (new_xoff > canWinW - canViewW)
    new_xoff = canWinW - canViewW;
  if (new_yoff > canWinH - canViewH)
    new_yoff = canWinH - canViewH;
  if (new_xoff < 0)
    new_xoff = 0;
  if (new_yoff < 0)
    new_yoff = 0;
  if (swin->flags & ABUTSWIN_LSLIDE)  {
    if (bh - 2*slideW < butbw * 5)  {
      but_resize(swin->up, x, by, slideW, bh/2);
      but_resize(swin->down, x, by + bh/2, slideW, (bh + 1) / 2);
      but_resize(swin->vSlide, 0, 0, 0, 0);
      butSlide_set(swin->vSlide, canWinH - canViewH, new_yoff, canViewH);
      v_moved(swin->vSlide, new_yoff, FALSE);
    } else  {
      but_resize(swin->up, x,by,slideW,slideW);
      but_resize(swin->down, x,by+bh-slideW,slideW,slideW);
      but_resize(swin->vSlide, x,by+slideW,slideW,bh-2*slideW);
      butSlide_set(swin->vSlide, canWinH - canViewH, new_yoff, canViewH);
      v_moved(swin->vSlide, new_yoff, FALSE);
    }
  } else if (swin->flags & ABUTSWIN_RSLIDE)  {
    but_resize(swin->up, x+w-slideW,by,slideW,slideW);
    but_resize(swin->down, x+w-slideW,by+h-slideW,slideW,slideW);
    but_resize(swin->vSlide, x+w-slideW,by+slideW,slideW,bh-2*slideW);
    butSlide_set(swin->vSlide, canWinH - canViewH, new_yoff, canViewH);
    v_moved(swin->vSlide, new_yoff, FALSE);
  }
  if (swin->flags & ABUTSWIN_TSLIDE)  {
    but_resize(swin->left, bx, y, slideW, slideW);
    but_resize(swin->right, bx + bw - slideW, y, slideW, slideW);
    but_resize(swin->hSlide, bx + slideW, y, bw - 2 * slideW, slideW);
    butSlide_set(swin->hSlide, canWinW - canViewW, new_xoff, canViewW);
    v_moved(swin->hSlide, new_xoff, FALSE);
  } else if (swin->flags & ABUTSWIN_BSLIDE)  {
    but_resize(swin->left, bx, y + h - slideW, slideW, slideW);
    but_resize(swin->right, bx + bw - slideW, y + h - slideW, slideW, slideW);
    but_resize(swin->hSlide, bx + slideW, y + h - slideW,
	       bw - 2 * slideW, slideW);
    butSlide_set(swin->hSlide, canWinW - canViewW, new_xoff, canViewW);
    v_moved(swin->hSlide, new_xoff, FALSE);
  }
  but_resize(swin->box, bx,by,bw,bh);
  swin->canButW = bw - butbw*2;
  swin->canButH = bh - butbw*2;
}


static void  draw_uparrow(void *packet, ButWin *win,
			  int x, int y, int w, int h)  {
  XPoint  points[3];
  Display  *dpy = butEnv_dpy(butWin_env(win));
  Window  xwin = butWin_xwin(win);
  GC  gc = butEnv_gc(butWin_env(win));

  h &= ~1;
  w &= ~1;
  points[0].x = x;
  points[0].y = y+h/2;
  points[1].x = x+w/2;
  points[1].y = y;
  points[2].x = x+w;
  points[2].y = y+h/2;
  XFillPolygon(dpy, xwin, gc, points, 3, Nonconvex, CoordModeOrigin);
}


static void  draw_downarrow(void *packet, ButWin *win,
			    int x, int y, int w, int h)  {
  XPoint  points[3];
  Display  *dpy = butEnv_dpy(butWin_env(win));
  Window  xwin = butWin_xwin(win);
  GC  gc = butEnv_gc(butWin_env(win));

  h &= ~1;
  w &= ~1;
  points[0].x = x;
  points[0].y = y+h/2;
  points[1].x = x+w/2;
  points[1].y = y+h;
  points[2].x = x+w;
  points[2].y = y+h/2;
  XFillPolygon(dpy, xwin, gc, points, 3, Nonconvex, CoordModeOrigin);
}


static void  draw_leftarrow(void *packet, ButWin *win,
			    int x, int y, int w, int h)  {
  XPoint  points[3];
  Display  *dpy = butEnv_dpy(butWin_env(win));
  Window  xwin = butWin_xwin(win);
  GC  gc = butEnv_gc(butWin_env(win));

  h &= ~1;
  w &= ~1;
  points[0].x = x;
  points[0].y = y+h/2;
  points[1].x = x+w/2;
  points[1].y = y;
  points[2].x = x+w/2;
  points[2].y = y+h;
  XFillPolygon(dpy, xwin, gc, points, 3, Nonconvex, CoordModeOrigin);
}


static void  draw_rightarrow(void *packet, ButWin *win,
			     int x, int y, int w, int h)  {
  XPoint  points[3];
  Display  *dpy = butEnv_dpy(butWin_env(win));
  Window  xwin = butWin_xwin(win);
  GC  gc = butEnv_gc(butWin_env(win));

  h &= ~1;
  w &= ~1;
  points[0].x = x+w;
  points[0].y = y+h/2;
  points[1].x = x+w/2;
  points[1].y = y;
  points[2].x = x+w/2;
  points[2].y = y+h;
  XFillPolygon(dpy, xwin, gc, points, 3, Nonconvex, CoordModeOrigin);
}


static ButOut  pressUp(But *but)  {
  AbutSwin  *swin;

  swin = but_packet(but);
  assert(MAGIC(swin));
  butSlide_startSlide(swin->vSlide, TRUE, -2*swin->lineH, TRUE);
  return(0);
}


static ButOut  pressDown(But *but)  {
  AbutSwin  *swin;

  swin = but_packet(but);
  assert(MAGIC(swin));
  butSlide_startSlide(swin->vSlide, TRUE, 2*swin->lineH, TRUE);
  return(0);
}


static ButOut  pressLeft(But *but)  {
  AbutSwin  *swin;

  swin = but_packet(but);
  assert(MAGIC(swin));
  butSlide_startSlide(swin->hSlide, TRUE, -2*swin->lineH, TRUE);
  return(0);
}


static ButOut  pressRight(But *but)  {
  AbutSwin  *swin;

  swin = but_packet(but);
  assert(MAGIC(swin));
  butSlide_startSlide(swin->hSlide, TRUE, 2*swin->lineH, TRUE);
  return(0);
}


static ButOut  releaseVArrow(But *but)  {
  AbutSwin  *swin = but_packet(but);

  assert(MAGIC(swin));
  butSlide_stopSlide(swin->vSlide);
  return(0);
}


static ButOut  releaseHArrow(But *but)  {
  AbutSwin  *swin = but_packet(but);

  assert(MAGIC(swin));
  butSlide_stopSlide(swin->hSlide);
  return(0);
}


void  abutSwin_vMove(AbutSwin *swin, int newLoc)  {
  butSlide_set(swin->vSlide, BUT_NOCHANGE, newLoc, BUT_NOCHANGE);
  v_moved(swin->vSlide, newLoc, FALSE);
}


static ButOut  v_moved(But *but, int new_yoff, bool newPress)  {
  AbutSwin  *swin = but_packet(but);

  assert(MAGIC(swin));
  if (butWin_viewH(swin->win) == butWin_h(swin->win))
    swin->yCenter = 0.0;
  else
    swin->yCenter = (float)new_yoff / (float)(butWin_h(swin->win) -
					       butWin_viewH(swin->win));
  if (swin->yCenter > 1.0)
    swin->yCenter = 1.0;
  if (swin->yCenter < 0.0)
    swin->yCenter = 0.0;
  butCan_slide(swin->win, BUT_NOCHANGE, new_yoff, FALSE);
  return(0);
}


static ButOut  hMoved(But *but, int newXOff, bool newPress)  {
  AbutSwin  *swin = but_packet(but);

  assert(MAGIC(swin));
  if (butWin_viewW(swin->win) == butWin_w(swin->win))
    swin->xCenter = 0.0;
  else
    swin->xCenter = (float)newXOff / (float)(butWin_w(swin->win) -
					      butWin_viewW(swin->win));
  if (swin->xCenter > 1.0)
    swin->xCenter = 1.0;
  if (swin->xCenter < 0.0)
    swin->xCenter = 0.0;
  butCan_slide(swin->win, newXOff, BUT_NOCHANGE, FALSE);
  return(0);
}


void  abutSwin_destroy(AbutSwin *swin)  {
  assert(MAGIC(swin));
  MAGIC_UNSET(swin);
  wms_free(swin);
}


static void  redo_slider(void *packet, int xoff, int yoff,
			 int w, int h, int viewW, int viewH)  {
  AbutSwin  *swin = packet;

  assert(MAGIC(swin));
  if (swin->flags & (ABUTSWIN_TSLIDE|ABUTSWIN_BSLIDE))  {
    butSlide_set(swin->hSlide, w-viewW, xoff, viewW);
    if (viewW == w)
      swin->xCenter = 0.0;
    else
      swin->xCenter = (float)xoff / (float)(w - viewW);
  }
  if (swin->flags & (ABUTSWIN_LSLIDE|ABUTSWIN_RSLIDE))  {
    butSlide_set(swin->vSlide, h-viewH, yoff, viewH);
    if (viewH == h)
      swin->yCenter = 0.0;
    else
      swin->yCenter = (float)yoff / (float)(h - viewH);
  }
}

#endif  /* X11_DISP */
