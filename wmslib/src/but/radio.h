/*
 * wmslib/src/but/radio.h, part of wmslib (Library functions)
 * Copyright (C) 1994-1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 *
 * Includes for radio.c
 */

#ifndef  _BUT_RADIO_H_
#define  _BUT_RADIO_H_  1

/**********************************************************************
 * Functions
 **********************************************************************/
extern But  *butRadio_create(ButOut (*func)(But *but, int value),
			     void *packet, ButWin *win, int layer, int flags,
			     int val, int maxVal);
extern int  butRadio_get(But *but);
extern void  butRadio_set(But *but, int newVal, bool propagate);

#endif  /* _BUT_CHECKBOX_H_ */
