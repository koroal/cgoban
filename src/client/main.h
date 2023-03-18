/*
 * src/client/main.h, part of Complete Goban (game program)
 * Copyright (C) 1995-1997 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _CLIENT_MAIN_H_

#ifndef  _CGOBAN_H_
#include "../cgoban.h"
#endif
#ifndef  _CLIENT_DATA_H_
#include "data.h"
#endif
#ifndef  _CLIENT_BOARD_H_
#include "board.h"
#endif
#ifndef  _CLIENT_GAME_H_
#include "game.h"
#endif
#ifndef  _CLIENT_PLAYER_H_
#include "player.h"
#endif
#ifndef  _CLIENT_LOOK_H_
#include "look.h"
#endif
#ifdef  _CLIENT_MAIN_H_
#error  LEVELIZATION ERROR
#endif
#define  _CLIENT_MAIN_H_  1


/**********************************************************************
 * Constants
 **********************************************************************/
#define  MAIN_HISTORYMAXLEN  100

/**********************************************************************
 * Data Types
 **********************************************************************/
typedef enum  {
  cliMain_idle, cliMain_inFile, cliMain_gotWho
} CliMainState;


typedef struct CliMain_struct  {
  CliData  *data;
  ButWin  *win;
  But  *bg, *quit, *help, *games, *players, *tin;
  But  *userState, *looking, *open, *closed;
  AbutTerm  *script;
  AbutMsg  *message;
  CliGameList  gameList;
  CliPlayerList  playerList;
  CliMainState  state;
  bool  promptVisible;
  CliLook  look;
  Str  outStr;
  const char  *fileEnd;
  ButWin  *iWin;
  But  *iBg, *iPic;

  Str  history[MAIN_HISTORYMAXLEN];
  int  historyLen, historyBeingFilled, historyBeingViewed;

  void (*destroyCallback)(struct CliMain_struct *main, void *packet);
  void *packet;
  MAGIC_STRUCT
} CliMain;


/**********************************************************************
 * Functions
 **********************************************************************/
extern CliMain  *cliMain_create(CliData *cd,
				void (*destroyCallback)(CliMain *main,
							void *packet),
				void *packet);
extern void  cliMain_activate(CliMain *main);
extern void  cliMain_message(CliMain *main, const char *message);
extern void  cliMain_clearMessage(CliMain *main);
extern void  cliMain_destroy(CliMain *cm);
extern void  cliMain_log(CliMain *main, const char *str);

#endif  /* _CLIENT_MAIN_H_ */
