/*
 * src/client/look.h, part of Complete Goban (game program)
 * Copyright (C) 1996 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _CLIENT_LOOK_H_

#ifndef  _GOGAME_H_
#include "../goGame.h"
#endif
#ifndef  _CGOBAN_H_
#include "../cgoban.h"
#endif
#ifndef  _GOBAN_H_
#include "../goban.h"
#endif
#ifndef  _CLIENT_SERVER_H_
#include "server.h"
#endif
#ifdef  _CLIENT_LOOK_H_
        LEVELIZATION ERROR
#endif
#define  _CLIENT_LOOK_H_  1


/**********************************************************************
 * Data Types
 **********************************************************************/
typedef enum  {
  cliLook_ready, cliLook_info, cliLook_body
} CliLookStatus;


typedef struct CliLook_struct  {
  Cgoban  *cg;
  CliServer  server;
  int  handicap, captures[2];
  float  komi;
  Str  name[2], rank[2], boardData;
  CliLookStatus  state;
  bool  skip;

  MAGIC_STRUCT
} CliLook;


typedef struct  CliLookChild_struct  {
  Cgoban  *cg;
  GoGame  *game;
  Goban  *goban;
  MAGIC_STRUCT
} CliLookChild;
  

/**********************************************************************
 * Functions
 **********************************************************************/
extern void  cliLook_init(CliLook *look, Cgoban *cg, CliServer server);
extern void  cliLook_gotData(CliLook *look, const char *buf);
extern void  cliLook_deinit(CliLook *look);


#endif  /* _CLIENT_BOARD_H_ */
