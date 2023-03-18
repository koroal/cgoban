/*
 * src/setup.h, part of Complete Goban (game program)
 * Copyright (C) 1995-1996 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _SETUP_H_

#ifndef  _CGOBAN_H_
#include "cgoban.h"
#endif

#ifdef  _SETUP_H_
   Levelization Error
#endif
#define  _SETUP_H_  1


/**********************************************************************
 * Constants
 **********************************************************************/

/*
 * If you change CONTROL_MAXSERVERS, you must also change the number of 
 *   entries in a lot of the "client.<whatever>" CLP entries found in
 *   "src/cgoban.c".
 */
#define  SETUP_MAXSERVERS  10


/**********************************************************************
 * Data types
 **********************************************************************/
typedef struct Setup_struct  {
  Cgoban  *cg;

  void  (*destroyCallback)(struct Setup_struct *setup, void *packet);
  void  (*newServerCallback)(struct Setup_struct *setup, void *packet);
  void  *packet;

  ButWin  *win;

  But  *bg, *title;

  AbutSwin  *swin;

  int  srvNum;
  But  *srvMenu;
  But  *srvBox, *srvTitle, *srvName, *srvNameLabel;
  But  *srvProto, *srvProtoLabel, *srvDirect, *srvDirectLabel;
  But  *igsLabel, *nngsLabel;
  But  *srvComp, *srvCompIn, *srvPort, *srvPortIn;
  But  *srvCmd, *srvCmdLabel;

  But  *miscBox, *miscTitle;
  But  *coordLabel, *coord;
  But  *hiLabel, *hi;
  But  *numKibsLabel, *numKibs;
  But  *noTypoLabel, *noTypo;
  But *warnLabel, *warnLimit;

  But  *help, *ok;

  MAGIC_STRUCT
} Setup;


/**********************************************************************
 * Functions
 **********************************************************************/
extern Setup  *setup_create(Cgoban *cg,
			    void (*destroyCallback)(Setup *setup,
						    void *packet),
			    void  (*newServerCallback)(Setup *setup,
						       void *packet),
			    void *packet);
extern void  setup_destroy(Setup *setup, bool propagate);


#endif  /* _SETUP_H_ */
