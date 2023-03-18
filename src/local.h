/*
 * src/local.h, part of Complete Goban (game program)
 * Copyright (C) 1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _LOCAL_H_
#define  _LOCAL_H_  1

#ifndef  _SGF_H_
#include "sgf.h"
#endif
#ifndef  _GOGAME_H_
#include "goGame.h"
#endif
#ifndef  _GOPIC_H_
#include "goPic.h"
#endif
#ifndef  _GOBAN_H_
#include "goban.h"
#endif


/**********************************************************************
 * Forward declarations
 **********************************************************************/
#ifndef  _ABUT_FSEL_H_
#ifndef  AbutFsel
#define  AbutFsel  void
#endif
#endif
#ifndef  _ABUT_MSG_H_
#ifndef  AbutMsg
#define  AbutMsg  void
#endif
#endif


/**********************************************************************
 * Data types
 **********************************************************************/

typedef struct Local_struct  {
  Cgoban  *cg;

  GoGame  *game;
  bool  modified;
  Goban  *goban;
  Sgf  *moves;
  SgfElem  *endGame;  /* Used to backtrack after a dispute. */
  SgfElem  *lastComment;
  AbutFsel  *fsel;
  AbutMsg  *reallyQuit;

  MAGIC_STRUCT
} Local;


/**********************************************************************
 * Functions
 **********************************************************************/
extern Local  *local_create(Cgoban *cg, GoRules rules,
			    int size, int hcap, float komi,
			    const char *white, const char *black,
			    GoTimeType timeType, int mainTime,
			    int byTime, int auxTime);
extern Local  *local_createFile(Cgoban *cg, const char *fname);
extern Local  *local_createSgf(Cgoban *cg, Sgf *mc);
extern void  local_destroy(Local *l);


#endif  /* _LOCAL_H_ */
