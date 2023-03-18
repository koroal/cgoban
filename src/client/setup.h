/*
 * src/client/setup.h, part of Complete Goban (game program)
 * Copyright (C) 1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _CLIENT_SETUP_H_
#define  _CLIENT_SETUP_H_  1

#ifndef  _CGOBAN_H_
#include "../cgoban.h"
#endif
#ifndef  _CLIENT_DATA_H_
#include "data.h"
#endif
#ifndef  _CLIENT_SERVER_H_
#include "server.h"
#endif


/**********************************************************************
 * Data Types
 **********************************************************************/
typedef struct CliSetup_struct  {
  CliData  *cd;
  AbutMsg  *login;
  int  serverNum;
  ClpEntry  *userClp, *passwordClp;

  MAGIC_STRUCT
} CliSetup;


/**********************************************************************
 * Functions
 **********************************************************************/

extern CliSetup  *cliSetup_create(Cgoban *cg, int serverNum);
extern void  cliSetup_createGmp(Cgoban *cg, int serverNum,
				const char *username, const char *password,
				const CliActions *actions, void *packet);
extern void  cliSetup_destroy(CliSetup *c);


#endif  /* _CLIENT_SETUP_H_ */
