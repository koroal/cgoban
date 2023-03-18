/*
 * wmslib/include/but/canvas.h, part of wmslib (Library functions)
 * Copyright (C) 1994-1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 *
 * Includes for canvas.c
 */

#ifndef  _BUT_CANVAS_H_
#define  _BUT_CANVAS_H_  1

/**********************************************************************
 * Functions.
 **********************************************************************/
extern ButWin  *butCan_create(void *packet, ButWin *parent, int layer,
			      ButWinFunc *resize, ButWinFunc *destroy,
			      void (*change)(void *packet, int xOff, int yOff,
					     int w, int h,
					     int viewW, int viewH));
/* Any values of BUT_NOCHANGE in this function will be left unchanged. */
extern void  butCan_resizeView(ButWin *win, int x, int y, int w, int h,
			       bool propagate);
extern void  butCan_slide(ButWin *win, int xOff, int yOff, bool propagate);
extern void  butCan_resizeWin(ButWin *win, int w, int h, bool propagate);
#define  butCan_xOff(win)  ((win)->xOff)
#define  butCan_yOff(win)  ((win)->yOff)
extern void  butCan_destroy(ButWin *can);

/* Private. */
extern Window  butCan_xWin(ButWin *win);
extern void  butCan_redrawn(ButWin *win, int x,int y, int w,int h);
extern void  butCan_winDead(ButWin *win);


#endif  /* _BUT_CANVAS_H_ */
