/*
 * wmslib/src/but/menu.h, part of wmslib (Library functions)
 * Copyright (C) 1995-1996 William Shubert.
 * See "configure.h.in" for more copyright information.
 *
 * Includes for menu.c
 */

#ifndef  _BUT_MENU_H_
#define  _BUT_MENU_H_  1


/**********************************************************************
 * Constants
 **********************************************************************/
#define  BUTMENU_OLEND     NULL
#define  BUTMENU_OLBREAK   (&butMenu_dummy)

#define  BUTMENU_DISABLED  1  /* Flags. */
#define  BUTMENU_END       2  /* Private. */
#define  BUTMENU_BREAK     4  /* Private. */


/**********************************************************************
 * Global variables
 **********************************************************************/
/* Private. */
extern char  butMenu_dummy;


/**********************************************************************
 * Functions
 **********************************************************************/
extern But  *butMenu_upCreate(ButOut (*func)(But *but, int value),
			      void *packet, ButWin *win, int layer,
			      int toplayer, int flags, const char *title,
			      const char *optlist[], int cur_opt);
extern But  *butMenu_downCreate(ButOut (*func)(But *but, int value),
				void *packet, ButWin *win, int layer,
				int toplayer, int flags, const char *title,
				const char *optlist[], int cur_opt);
extern void  butMenu_set(But *but, int new_opt);
extern int   butMenu_get(But *but);
extern void  butMenu_setColor(But *but, int fg, int bg);
/* If newcval is -1, then it will be left unchanged. */
extern void  butMenu_setText(But *but, const char *title,
			     const char *optlist[], int newcval);
extern void  butMenu_setOptionName(But *but, const char *new, int entryNum);
/*
 * flags should be from the BUTMENU_DISABLED list above.
 */
extern void  butMenu_setFlags(But *but, int optnum, uint flags);

#endif  /* _BUT_MENU_H_ */
