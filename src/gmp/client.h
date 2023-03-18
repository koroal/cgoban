/*
 * src/gmp/client.h, part of Complete Goban (game program)
 * Copyright (C) 1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _GMP_CLIENT_H_
#define  _GMP_CLIENT_H_  1

#ifndef  _GMP_ENGINE_H_
#include "engine.h"
#endif
#ifndef  _CLIENT_BOARD_H_
#include "../client/board.h"
#endif


/**********************************************************************
 * Data types
 **********************************************************************/

typedef struct GmpClient_struct  {
  Cgoban *cg;
  bool  inGame;
  int  size, pid, inFile, outFile;
  GmpEngine  ge;
  Str  progName;
  CliBoard  *cliBoard;

  MAGIC_STRUCT
} GmpClient;


/**********************************************************************
 * Functions
 **********************************************************************/
extern GmpClient  *gmpClient_create(Cgoban *cg, const char *progName,
				    CliServer server, const char *user,
				    const char *password);
extern void  gmpClient_destroy(GmpClient *gc);


#endif  /* _GMP_SETUP_H_ */
