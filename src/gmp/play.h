/*
 * src/gmp/play.h, part of Complete Goban (game program)
 * Copyright (C) 1995-1997 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _GMP_PLAY_H_

#ifndef  _SGF_H_
#include "../sgf.h"
#endif
#ifndef  _GOGAME_H_
#include "../goGame.h"
#endif
#ifndef  _GOPIC_H_
#include "../goPic.h"
#endif
#ifndef  _GOBAN_H_
#include "../goban.h"
#endif
#ifndef  _GMP_ENGINE_H_
#include "engine.h"
#endif
#define  _GMP_PLAY_H_  1


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

typedef struct GmpPlayInfo_struct  {
  int  in, out, pid;
  bool  ready;
  GmpEngine  *engine;
} GmpPlayInfo;


typedef struct GmpPlay_struct  {
  Cgoban  *cg;
  GmpPlayInfo  players[2];
  GoGame  *game;
  Goban  *goban;
  Sgf  *moves;
  SgfElem  *endGame;  /* Used to backtrack after a dispute. */
  SgfElem  *lastComment;
  AbutFsel  *fsel;

  MAGIC_STRUCT
} GmpPlay;


/**********************************************************************
 * Functions
 **********************************************************************/
extern GmpPlay  *gmpPlay_create(Cgoban *cg,
				int inFiles[2], int outFiles[2],
				int pids[2], GoRules rules,
				int size, int hcap, float komi,
				const char *white, const char *black,
				GoTimeType timeType, int mainTime,
				int byTime, int auxTime);
extern void  gmpPlay_destroy(GmpPlay *l);


#endif  /* _GMP_PLAY_H_ */
