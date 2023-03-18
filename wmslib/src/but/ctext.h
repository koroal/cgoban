/*
 * wmslib/src/but/ctext.h, part of wmslib (Library functions)
 * Copyright (C) 1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 *
 * Includes for ctext.c
 */

#ifndef  _BUT_CTEXT_H_
#define  _BUT_CTEXT_H_  1

#include <but/box.h>

/**********************************************************************
 * Functions
 **********************************************************************/
extern But  *butCt_create(ButOut (*func)(But *but), void *packet,
			  ButWin *win, int layer, int flags,
			  const char *text);
extern void  butCt_setText(But *info, const char *text);
extern But  *butAct_create(ButOut (*func)(But *but), void *packet,
			   ButWin *win, int layer, int flags,
			   const char *text, int angleflags);
extern But  *butAct_vCreate(ButOut (*pfunc)(But *but),
			    ButOut (*rfunc)(But *but), void *packet,
			    ButWin *win, int layer, int flags,
			    const char *text, int angleflags);
#define  butAct_setText  butCt_setText
extern void  butCt_setNetAction(But *but, bool netAction);
#define  butAct_setNetAction  butCt_setNetAction
extern void  butCt_setTextLoc(But *but, int x, int y, int w, int h);

#endif  /* _BUT_CTEXT_H_ */
