/*
 * wmslib/src/but/write.h, part of wmslib (Library functions)
 * Copyright (C) 1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 *
 * Includes for write.c
 */

#ifndef  _BUT_WRITE_H_
#define  _BUT_WRITE_H_  1

#ifndef  _BUT_TEXT_H_
#include <but/text.h>
#endif


/**********************************************************************
 * Functions
 **********************************************************************/
extern void  butWin_writeTabs(ButWin *win, int startX, int y, const char *text,
			      int font, const int *tabList,
			      const ButTextAlign *aligns);


#endif  /* _BUT_WRITE_H_ */
