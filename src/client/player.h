/*
 * src/client/player.h, part of Complete Goban (player program)
 * Copyright (C) 1995-1996 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _CLIENT_PLAYER_H_

#ifndef  _CLIENT_DATA_H_
#include "data.h"
#endif
#ifndef  _CLIENT_BOARD_H_
#include "board.h"
#endif
#ifndef  _CLIENT_MATCH_H_
#include "match.h"
#endif

#ifdef  _CLIENT_PLAYER_H_
  Levelization Error.
#endif
#define  _CLIENT_PLAYER_H_  1


/**********************************************************************
 * Data Types
 **********************************************************************/
typedef enum  {
  /*
   * It is important to note that when a player list is sorted by state, they
   *   will be sorted with the lowest valued state on the bottom of the
   *   player list.  In addition, there's a character array in the
   *   getListEl() function that depends on the order of this enum type.
   */
  cliPlayer_unknown, cliPlayer_closed, cliPlayer_open, cliPlayer_looking,
  cliPlayer_noPlayer
} CliPlayerState;


typedef enum  {
  cliPlayerSort_alphabet, cliPlayerSort_rank, cliPlayerSort_open
} CliPlayerSort;


typedef struct CliPlayer_struct  {
  CliPlayerState  state;
  CliPlayerSort  sort;  /* Needed for the qsort.  :-( */
  Str  name, rank, idleTime;
  But  *but;
  bool matchBut;
  int  gameIn, gameOb;
} CliPlayer;


typedef struct  CliPlayerList_struct  {
  CliData  *data;
  int  maxPlayers;
  CliPlayer  *players;
  CliPlayerSort  sort;
  int  sortEnd;
  CliMatch  *match;

  ButWin  *win;
  AbutSwin  *swin;
  But  *sortType, *nameSort, *rankSort, *stateSort;
  But  *bg, *sBg, *list, *num, *titleBox, *title, *reload;
  bool  elfBugReported;
  MAGIC_STRUCT
} CliPlayerList;


/**********************************************************************
 * Functions
 **********************************************************************/
extern CliPlayerList  *cliPlayerList_init(CliPlayerList *gl, CliData *data);
extern CliPlayerList  *cliPlayerList_deinit(CliPlayerList *gl);
extern void  cliPlayerList_openWin(CliPlayerList *gl);
extern void  cliPlayerList_whoOutput(CliPlayerList *gl, const char *buf);
extern void  cliPlayerList_disconnected(CliPlayerList *pl, const char *buf);
extern void  cliPlayerList_connected(CliPlayerList *pl, const char *buf);
extern void  cliPlayerList_sort(CliPlayerList *pl);
extern void  cliPlayerList_playerInGame(CliPlayerList *pl,
					const char *player, int gameNum);
extern int  cliPlayerList_lookupPlayer(CliPlayerList *pl, const char *player);
extern void  cliPlayerList_setState(CliPlayerList *pl, int playerNum,
				    CliPlayerState state);
extern const char  *cliPlayerList_getRank(CliPlayerList *pl,
					  const char *name);

#endif  /* _CLIENT_MAIN_H_ */
