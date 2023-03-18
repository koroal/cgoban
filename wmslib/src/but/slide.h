/*
 * wmslib/src/but/slide.h, part of wmslib (Library functions)
 * Copyright (C) 1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 *
 * Includes for slide.c
 */

#ifndef  _BUT_SLIDE_H_
#define  _BUT_SLIDE_H_  1

/**********************************************************************
 * Functions
 **********************************************************************/
extern But  *butSlide_hCreate(ButOut (*func)(But *but, int setting,
					     bool newPress),
			      void *packet, ButWin *win, int layer, int flags,
			      int maxval, int cval, int size);
extern But  *butSlide_vCreate(ButOut (*func)(But *but, int setting,
					     bool newPress),
			      void *packet, ButWin *win, int layer, int flags,
			      int maxval, int cval, int size);
extern int  butSlide_get(But *but);
extern void  butSlide_set(But *but, int maxval, int cval, int size);
extern void  butSlide_startSlide(But *but, bool pauseStart, int valsPerSecond,
				 bool propagate);
extern void  butSlide_stopSlide(But *but);

#endif  /* _BUT_SLIDE_H_ */
