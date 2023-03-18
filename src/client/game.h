/*
 * src/client/game.h, part of Complete Goban (game program)
 * Copyright (C) 1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _CLIENT_GAME_H_
#define  _CLIENT_GAME_H_  1

#ifndef  _CLIENT_DATA_H_
#include "data.h"
#endif
#ifndef  _CLIENT_BOARD_H_
#include "board.h"
#endif
#ifndef  _CLIENT_PLAYER_H_
#include "player.h"
#endif


/**********************************************************************
 * Data Types
 **********************************************************************/
typedef enum  {
  cliGame_noGame, cliGame_idle, cliGame_observing, cliGame_waitNoOb,
  cliGame_over
} CliGameState;


typedef struct CliGame_struct  {
  CliGameState  state;
  bool  gamePressed;  /* Don't let a double click do "observe <game>" twice! */
  Str  wName, bName, wRank, bRank, flags;
  int  moveNum, size, handicap;
  float  komi;
  int  byTime, observers;
  bool  free, fromMatch;
  CliBoard  *board;
} CliGame;


typedef struct  CliGameList_struct  {
  CliData  *data;
  CliPlayerList  *playerList;
  int  maxGames;
  CliGame  *games;
  int  defaultGame, playGame;
  int  kibGame;
  char  kibPlayer[41];

  ButWin  *win;
  AbutSwin  *swin;
  But  *bg, *sBg, *list, *num, *titleBox, *title, *reload;
  bool  elfBugReported;

  MAGIC_STRUCT
} CliGameList;


/**********************************************************************
 * Functions
 **********************************************************************/
extern CliGameList  *cliGameList_init(CliGameList *gl, CliData *data,
				      CliPlayerList *playerList);
extern CliGameList  *cliGameList_deinit(CliGameList *gl);
extern void  cliGameList_gotMove(CliGameList *gl, const char *move);
extern void  cliGameList_gotGameInfo(CliGameList *gl, const char *buf);
extern void  cliGameList_notObserving(CliGameList *gl, const char *buf);
extern void  cliGameList_kibitz(CliGameList *gl, const char *buf);
extern void  cliGameList_say(CliGameList *gl, const char *buf);
extern void  cliGameList_openWin(CliGameList *gl);
extern void  cliGameList_gameGone(CliGameList *gl, const char *buf,
				  int bufLen);
extern void  cliGameList_gameStarts(CliGameList *gl, const char *buf);
extern void  cliGameList_selectDead(CliGameList *gl, const char *buf);
extern void  cliGameList_deadStone(CliGameList *gl, const char *buf);
extern void  cliGameList_gotUndo(CliGameList *gl, const char *buf);
extern bool  cliGame_tellIsSay(CliGameList *list, Str *tell);

#endif  /* _CLIENT_MAIN_H_ */
