/*
 * src/control.h, part of Complete Goban (game program)
 * Copyright (C) 1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _CONTROL_H_
#define  _CONTROL_H_  1

#ifndef  _SETUP_H_
#include "setup.h"
#endif


/**********************************************************************
 * Data types
 **********************************************************************/
typedef struct Control_struct  {
  Cgoban  *cg;

  ButWin  *win;
  But  *box;
  But  *sPics[2], *servers[2];
  But  *lGame, *lGamePic;
  But  *lLoad, *lLoadPic;
  But  *edit, *editPic;
  But  *gmp, *gmpPic;
  But  *sBox;
  Setup  *setupWin;
  But  *help, *setup, *quit;

  ButWin  *iWin;
  But  *iBg, *ig;

  MAGIC_STRUCT
} Control;


/**********************************************************************
 * Functions
 **********************************************************************/
extern Control  *control_create(Cgoban *cg, bool iconic);

#endif  /* _CONTROL_H_ */
