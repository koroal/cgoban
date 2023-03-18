/*
 * src/client/board.h, part of Complete Goban (game program)
 * Copyright (C) 1995-1996 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _CLIENT_BOARD_H_

#ifndef  _GOBOARD_H_
#include "../goBoard.h"
#endif
#ifndef  _CLIENT_DATA_H_
#include "data.h"
#endif
#ifndef  _GOGAME_H_
#include "../goGame.h"
#endif
#ifndef  _GOBAN_H_
#include "../goban.h"
#endif
#ifndef  _SGF_H_
#include "../sgf.h"
#endif
#ifdef  _CLIENT_BOARD_H_
        LEVELIZATION ERROR
#endif
#define  _CLIENT_BOARD_H_  1


/**********************************************************************
 * Data Types
 **********************************************************************/
typedef struct CliBoard_struct  {
  CliData  *data;
  Cgoban  *cg;
  Sgf  *sgf;
  SgfElem  *gameEnd;
  int  gameNum, lastMoveRead;
  bool  gameEnded, onTrack;
  GoGame  *game;
  Goban  *goban;
  But  *shiftKeytrap, *ctrlKeytrap;
  void  (*destroy)(struct CliBoard_struct *cli, void *packet);
  void  *packet;
  GoTime  timer;
  Str  wName, bName;
  bool  moveWhite, moveBlack;
  Str  result;

  MAGIC_STRUCT
} CliBoard;


/**********************************************************************
 * Functions
 **********************************************************************/
extern CliBoard  *cliBoard_create(CliData *data, int gameNum,
				  const Str *wName, const Str *wRank,
				  const Str *bName, const Str *bRank,
				  int size, int handicap, float komi,
				  int mainTime, int byTime,
				  void (*destroy)(CliBoard *cli,
						  void *packet),
				  void *packet);
extern void  cliBoard_destroy(CliBoard *board, bool propagate);
extern Bool  cliBoard_gotMove(CliBoard *board, const char *locStr,
			      GoStone color, int moveNum);
#define  cliBoard_addKibitz(cli, kib)  goban_catComments((cli)->goban, (kib))
GobanOut  cliBoard_gridPressed(void *packet, int loc, bool forcePress);
GobanOut  cliBoard_passPressed(void *packet);
extern void  cliBoard_update(CliBoard *cli);


#endif  /* _CLIENT_BOARD_H_ */
