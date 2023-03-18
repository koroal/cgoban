/*
 * wmslib/src/but/plain.h, part of wmslib (Library functions)
 * Copyright (C) 1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 *
 * Includes for plain.c
 */

#ifndef  _BUT_PLAIN_H_
#define  _BUT_PLAIN_H_  1

/**********************************************************************
 * Functions.
 **********************************************************************/
extern But  *butPlain_create(ButWin *win, int layer, int flags, int cnum);
extern void  butPlain_setColor(But *but, int cnum);

extern But  *butPixmap_create(ButWin *win, int layer, int flags, Pixmap pic);
extern void  butPixmap_setPic(But *but, Pixmap pic, int x,int y);

/*
 * dummy is a button that isn't drawn, but is opaque.  It is sometimes
 *   useful to create one of these to prevent the button library from
 *   doing tons of extra redraws, then erase the dummy to force one
 *   final redraw.
 */
extern But  *butDummy_create(ButWin *win, int layer, int flags);

/*
 * A keytrap does nothing but call your func when the key you specify
 *   is pressed or released.
 */
extern But  *butKeytrap_create(ButOut (*func)(But *but, bool press),
			       void *packet, ButWin *win, int flags);
extern void  butKeytrap_setHold(But *but, bool hold);


#endif  /* _BUT_PLAIN_H_ */
