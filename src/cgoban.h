/*
 * src/cgoban.h, part of Complete Goban (game program)
 * Copyright (C) 1995-1996 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _CGOBAN_H_
#ifndef  _WMS_STR_H_
#include <wms/str.h>
#endif
#ifndef  _CGBUTS_H_
#include "cgbuts.h"
#endif
#ifndef  _WMS_CLP_H_
#include <wms/clp.h>
#endif
#ifndef  _ABUT_H_
#include <abut/abut.h>
#endif
#ifndef  _BUT_BUT_H_
#include <but/but.h>
#endif
#ifndef  _ABUT_HELP_H_
#include <abut/help.h>
#endif
#ifndef  _HELP_H_
#include "help.h"
#endif
#ifndef  _ABUT_MSG_H_
#include <abut/msg.h>
#endif
#ifdef  _CGOBAN_H_
#error  Levelization Error.
#endif

#define  _CGOBAN_H_  1


/**********************************************************************
 * Data types
 **********************************************************************/
typedef struct Cgoban_struct  {
  char  **envp;  /* This is the original envp passed to main(). */
  Clp  *clp;
  ButEnv  *env;
  Abut  *abut;
  AbutHelp  *help;
  int  rcMajor, rcMinor, rcBugFix;
  Str  lastDirAccessed;

  int  fontH;
  bool  markLastMove, markHotStones;
  bool  showCoords;
  Pixmap  boardPixmap, bgPixmap, bgLitPixmap, bgShadPixmap;

  Rnd  *rnd;

  Cgbuts  cgbuts;

  MAGIC_STRUCT
} Cgoban;


/**********************************************************************
 * Globals
 **********************************************************************/
extern const ButKey  cg_return[];
extern const ButKey  cg_help[];


/**********************************************************************
 * Functions
 **********************************************************************/
extern Cgoban  *cgoban_create(int argc, char *argv[], char *envp[]);
extern void  cgoban_destroy(Cgoban *cg);
#define  cgoban_createMsgWindow(cg, title, message)  \
  abutMsg_winCreate((cg)->abut, (title), (message))
ButOut  cgoban_createHelpWindow(But *but);

#endif  /* _CGOBAN_H_ */
