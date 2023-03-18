/*
 * src/client/data.h, part of Complete Goban (game program)
 * Copyright (C) 1995-1997 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _CLIENT_DATA_H_

#ifndef  _CLIENT_SERVER_H_
#include "server.h"
#endif
#ifndef  _CGOBAN_H_
#include "../cgoban.h"
#endif
#ifndef  _CLIENT_CONN_H_
#include "conn.h"
#endif
#ifndef  _WMS_STR_H_
#include <wms/str.h>
#endif
#ifdef  _CLIENT_DATA_H_
#error  LEVELIZATION ERROR
#endif

#define  _CLIENT_DATA_H_  1


/**********************************************************************
 * Data Types
 **********************************************************************/
typedef struct CliActions_struct  {
  /*
   * Passing board as a void is really ugly.  What I should do is move
   *   this data type into board.h and keep it in the main data structure,
   *   passing it down as needed.
   */
  void (*newGame)(void *packet, GoStone localColor, int size, int handicap,
		  float komi, int mainTime, int byTime, void *board);
  void (*gotMove)(void *packet, GoStone color, int loc);
  void (*gotUndo)(void *packet);
  void (*endGame)(void *packet);
  void (*logout)(void *packet);
} CliActions;


typedef enum  {
  cliData_setup, cliData_login, cliData_main
} CliDataState;


typedef struct CliData_struct  {
  CliDataState  state;
  Cgoban  *cg;
  int  serverNum;
  CliServer  server;
  const char  *serverName;
  Str  userName;
  Str  cmdBuild;
  ClpEntry  *numKibitz;

  bool  connValid;
  CliConn  conn;

  const CliActions  *actions;
  void  *packet;

  int  refCount;

  /* Not sure I like this. */
  ButOut (*observeGame)(void *obPacket, int boardId);
  void *obPacket;

  MAGIC_STRUCT
} CliData;


/**********************************************************************
 * Functions
 **********************************************************************/
extern CliData  *cliData_create(Cgoban *cg, int serverNum,
				const CliActions *actions, void *packet);
#define  cliData_incRef(cd)  (++((cd)->refCount))
extern void  cliData_closeConns(CliData *cd);
extern void  cliData_decRef(CliData *cd);

#endif  /* _CLIENT_DATA_H_ */
