/*
 * wmslib/src/but/tblock.h, part of wmslib (Library functions)
 * Copyright (C) 1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 *
 * Includes for tblock.c
 */

#ifndef  _BUT_TBLOCK_H_
#define  _BUT_TBLOCK_H_  1

#include <but/text.h>

/**********************************************************************
 * Functions
 **********************************************************************/
extern But  *butTblock_create(ButWin *win, int layer, int flags,
			      const char *text, ButTextAlign align);

/* resize returns the height of the text block after being resized. */
extern int  butTblock_resize(But *but, int x, int y, int w);
extern void  butTblock_setText(But *but, const char *text);
extern const char  *butTblock_getText(But *but);
extern void  butTblock_setFont(But *but, int fontnum);
extern int  butTblock_getH(But *but);
extern int  butTblock_guessH(ButEnv *env, const char *text, int w,
			     int fontNum);

#endif  /* _BUT_TBLOCK_H_ */
