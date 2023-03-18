/*
 * wmslib/src/but/box.h, part of wmslib (Library functions)
 * Copyright (C) 1994-1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 *
 * Includes for but.c
 */

#ifndef  _BUT_BOX_H_

#ifndef  _BUT_BUT_H_
#include <but/but.h>
#endif
#define  _BUT_BOX_H_  1

/**********************************************************************
 * Constants
 **********************************************************************/
#define  BUT_ALEFT   0x01  /* Arrows on left/right. */
#define  BUT_ARIGHT  0x02
#define  BUT_RLEFT   0x04  /* Rounded edges on left/right. */
#define  BUT_RRIGHT  0x08
#define  BUT_SLEFT   0x10  /* Square edges on left/right. */
#define  BUT_SRIGHT  0x20

/**********************************************************************
 * Functions
 **********************************************************************/

/* Public. */
extern But  *butBox_create(ButWin *win, int layer, int flags);
extern void  butBox_setColors(But *but, int ul, int lr);
extern void  butBox_setPixmaps(But *but, Pixmap ul, Pixmap lr);

extern But  *butBoxFilled_create(ButWin *win, int layer, int flags);
extern void  butBoxFilled_setColors(But *but, int ul, int lr, int c);
extern void  butBoxFilled_setPixmaps(But *but, Pixmap ul, Pixmap lr, Pixmap c);


/* Private. */
extern void  but_drawCtb(ButWin *win, int flags, int fgpic,
			 int bgpic, int pbgpic,
			 int x, int y, int w, int h, int bw, int angles);
extern void  but_drawCt(ButWin *win, int flags, int fgpic,
			int bgpic, int pbgpic,
			int x, int y, int w, int h,
			int bw, const char *text, int angles,
			int fontnum);
/* If bstate is nonzero, invert the ul and lr colors. */
extern void  but_drawBox(ButWin *win, int x, int y,
			 int w, int h, int bstate, int bw, int angles,
			 int ulcolor, int lrcolor, Pixmap ulmap,
			 Pixmap lrmap);


#endif  /* _BUT_BOX_H_ */
