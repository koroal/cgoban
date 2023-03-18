/*
 * wmslib/src/but/checkbox.h, part of wmslib (Library functions)
 * Copyright (C) 1994-1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 *
 * Includes for checkbox.c
 */

#ifndef  _BUT_CHECKBOX_H_
#define  _BUT_CHECKBOX_H_  1

/**********************************************************************
 * Functions
 **********************************************************************/
extern But  *butCb_create(ButOut (*func)(But *but, bool value), void *packet,
			  ButWin *win, int layer, int flags,
			  bool on);
extern bool  butCb_get(But *but);
extern void  butCb_set(But *but, bool on, bool makeCallback);

#endif  /* _BUT_CHECKBOX_H_ */
