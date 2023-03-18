/*
 * wmslib/src/but/list.h, part of wmslib (Library functions)
 * Copyright (C) 1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 *
 * Includes for list.c
 */

#ifndef  _BUT_LIST_H_
#define  _BUT_LIST_H_  1

#ifndef  _BUT_TEXT_H_
#include <but/text.h>
#endif


/**********************************************************************
 * Functions
 **********************************************************************/
extern But  *butList_create(ButOut (*func)(But *but, int line), void *packet,
			    ButWin *win, int layer, int flags);
extern void  butList_setTabs(But *but, const int *tabs,
			     const ButTextAlign *tabAligns, int numTabs);
extern void  butList_changeLine(But *but, int line, const char *text);
extern void  butList_resize(But *list, int x, int y, int w);
extern const char  *butList_get(But *but, int line);
/*
 * setPress is automatically set to TRUE when a changeLine to a nonempty
 *   string is done, and set to FALSE when changeLine to an empty string
 *   is done.
 */
extern void  butList_setPress(But *but, int line, bool pressable);
extern void  butList_setLen(But *but, int newLen);
extern int  butList_len(But *but);


#endif  /* _BUT_LIST_H_ */
