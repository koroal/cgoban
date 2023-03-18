/*
 * src/client/login.h, part of Complete Goban (game program)
 * Copyright (C) 1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _CLIENT_LOGIN_H_
#define  _CLIENT_LOGIN_H_  1

#ifndef  _CGOBAN_H_
#include "../cgoban.h"
#endif
#ifndef  _CLIENT_DATA_H_
#include "data.h"
#endif
#ifndef  _CLIENT_SERVER_H_
#include "server.h"
#endif
#ifndef  _CLIENT_MAIN_H_
#include "main.h"
#endif


/**********************************************************************
 * Data Types
 **********************************************************************/
typedef enum  {
  cliLogin_waitForUser, cliLogin_waitForPassword, cliLogin_waitForPrompt
} CliLoginState;


typedef struct CliLogin_struct  {
  CliData  *cd;
  CliMain  *main;
  bool  keepMainMessage;
  Str  user, pass;
  CliLoginState  state;
  MAGIC_STRUCT
} CliLogin;


/**********************************************************************
 * Functions
 **********************************************************************/
extern CliLogin  *cliLogin_create(CliData *cd, const char *user,
				  const char *pass);
extern void  cliLogin_destroy(CliLogin *cl, bool propagate);


#endif  /* _CLIENT_LOGIN_H_ */
