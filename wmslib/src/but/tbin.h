/*
 * wmslib/src/but/textin.h, part of wmslib (Library functions)
 * Copyright (C) 1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 *
 * Includes for textin.c
 */

#ifndef  _BUT_TBIN_H_
#define  _BUT_TBIN_H_  1

/**********************************************************************
 * Functions
 **********************************************************************/
extern But  *butTbin_create(ButWin *win, int layer,
			    int flags, const char *text);
extern void  butTbin_set(But *but, const char *str);
extern void  butTbin_setMaxLines(But *but, int maxLines);
extern const char  *butTbin_get(But *but);
extern int  butTbin_numLines(But *but);
extern void  butTbin_insert(But *but, const char *appText);
extern void  butTbin_delete(But *but, int delStart, int delLen);
extern int  butTbin_len(But *but);
/*
 * The offWinCallback function will be called whenever your cursor leaves
 *   the window.  Useful if you're on a canvas and want the cursor to
 *   stay on the canvas.
 */
extern void  butTbin_setOffWinCallback(But *but,
				       void (*func)(But *but, int activeLine,
						    int passiveLine,
						    int mouseY));
extern void  butTbin_setReadOnly(But *but, bool ro);


#endif  /* _BUT_TBIN_H_ */
