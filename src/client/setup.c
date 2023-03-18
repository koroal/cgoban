/*
 * src/cliSetup.c, part of Complete Goban (game program)
 * Copyright (C) 1995 William Shubert
 * See "configure.h.in" for more copyright information.
 */

#include <wms.h>
#include <but/but.h>
#include <abut/msg.h>
#include <wms/clp.h>
#include <wms/str.h>
#include <but/textin.h>
#include "../cgoban.h"
#include "setup.h"
#include "../msg.h"
#include "login.h"


/**********************************************************************
 * Forward Declarations
 **********************************************************************/
static ButOut  loginDestroyed(void *packet);
static ButOut  cancelPressed(But *but);
static ButOut  okPressed(But *but);
static ButOut  newUser(But *but, const char *user);
static ButOut  newPassword(But *but, const char *pass);


/**********************************************************************
 * Functions
 **********************************************************************/

CliSetup  *cliSetup_create(Cgoban *cg, int serverNum)  {
  CliSetup  *c;
  Str  title;
  AbutMsgTin  tins[2];
  AbutMsgOpt  buttons[3];

  /* Necessary reference to server.c.  See "server.h" for an explanation. */
  cliServer_create();

  assert(MAGIC(cg));
  str_init(&title);
  c = wms_malloc(sizeof(CliSetup));
  MAGIC_SET(c);
  c->cd = cliData_create(cg, serverNum, NULL, NULL);
  c->userClp = clp_lookup(cg->clp, "client.username");
  c->passwordClp = clp_lookup(cg->clp, "client.password");
  c->serverNum = serverNum;
  str_print(&title, "%s Setup", c->cd->serverName);
  tins[0].name = msg_usernameColon;
  tins[0].def = clpEntry_getStrNum(c->userClp, serverNum);
  tins[0].callback = newUser;
  tins[0].flags = 0;
  tins[1].name = msg_passwordColon;
  tins[1].def = clpEntry_getStrNum(c->passwordClp, serverNum);
  tins[1].callback = newPassword;
  tins[1].flags = abutMsgTinFlags_secret;
  buttons[0].name = msg_help;
  buttons[0].callback = cgoban_createHelpWindow;
  buttons[0].packet = &help_cliSetup;
  buttons[0].keyEq = cg_help;
  buttons[1].name = msg_cancel;
  buttons[1].callback = cancelPressed;
  buttons[1].packet = c;
  buttons[1].keyEq = NULL;
  buttons[2].name = msg_ok;
  buttons[2].callback = okPressed;
  buttons[2].packet = c;
  buttons[2].keyEq = cg_return;
  c->login = abutMsg_winOptInCreate(cg->abut, str_chars(&title),
				    msg_loginDesc[c->cd->server],
				    loginDestroyed, c, 3, buttons, 2, tins);
  str_deinit(&title);
  return(c);
}


static ButOut  newUser(But *but, const char *user)  {
  CliSetup  *setup = but_packet(but);

  assert(MAGIC(setup));
  but_setFlags(setup->login->tins[1], BUT_KEYED);
  return(0);
}


static ButOut  newPassword(But *but, const char *pass)  {
  CliSetup  *setup = but_packet(but);

  assert(MAGIC(setup));
  return(okPressed(setup->login->buts[2]));
}


void  cliSetup_destroy(CliSetup *c)  {
  assert(MAGIC(c));
  if (c->login)  {
    abutMsg_destroy(c->login, FALSE);
    c->login = NULL;
  }
  cliData_decRef(c->cd);
  MAGIC_UNSET(c);
  wms_free(c);
}


static ButOut  loginDestroyed(void *packet)  {
  CliSetup  *c = packet;

  assert(MAGIC(c));
  c->login = NULL;
  cliSetup_destroy(c);
  return(0);
}


static ButOut  cancelPressed(But *but)  {
  CliSetup  *c = but_packet(but);

  assert(MAGIC(c));
  butWin_destroy(c->login->win);
  return(0);
}


static ButOut  okPressed(But *but)  {
  CliSetup  *c = but_packet(but);
  ButOut  result = 0;
  const char  *user, *pass;

  assert(MAGIC(c));
  user = butTextin_get(abutMsg_tin(c->login, 0));
  pass = butTextin_get(abutMsg_tin(c->login, 1));
  clpEntry_setStrNum(c->userClp, user, c->serverNum);
  clpEntry_setStrNum(c->passwordClp, pass, c->serverNum);
  str_copyChars(&c->cd->userName, user);
  if (!cliLogin_create(c->cd, user, pass))
    result = BUTOUT_ERR;
  cliSetup_destroy(c);
  return(result);
}


void  cliSetup_createGmp(Cgoban *cg, int serverNum,
			 const char *username, const char *password,
			 const CliActions *actions, void *packet)  {
  CliData  *data;

  data = cliData_create(cg, serverNum, actions, packet);
  str_copyChars(&data->userName, username);
  cliLogin_create(data, username, password);
}
