/*
 * src/crwin.h, part of Complete Goban (game program)
 * Copyright (C) 1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _CRWIN_H_
#define  _CRWIN_H_  1


/**********************************************************************
 * Data types
 **********************************************************************/
typedef struct  {
  ButEnv  *env;
  But  *box, *title, *byBill, *noWarr, *seeHelp;
  ButTimer  *timer;

  MAGIC_STRUCT
} Crwin;


/**********************************************************************
 * Functions
 **********************************************************************/
Crwin  *crwin_create(Cgoban *cg, ButWin *win, int layer);


#endif  /* _CRWIN_H_ */
