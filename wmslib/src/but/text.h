/*
 * wmslib/src/but/text.h, part of wmslib (Library functions)
 * Copyright (C) 1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 *
 * Includes for text.c
 */

#ifndef  _BUT_TEXT_H_
#define  _BUT_TEXT_H_  1

/**********************************************************************
 * Data types
 **********************************************************************/
typedef enum  {
  butText_left, butText_right, butText_center, butText_just
} ButTextAlign;

/**********************************************************************
 * Functions
 **********************************************************************/
extern But  *butText_create(ButWin *win, int layer, int flags,
			    const char *text, ButTextAlign align);
extern int  butText_resize(But *but, int x, int y, int h);
extern const char  *butText_get(But *but);
extern void  butText_set(But *but, const char *text);
extern void  butText_setFont(But *but, int fontnum);
extern void  butText_setColor(But *but, int color, bool stipple);

#endif  /* _BUT_TEXT_H_ */
