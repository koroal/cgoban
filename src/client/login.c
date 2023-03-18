/*
 * src/client/login.c, part of Complete Goban (game program)
 * Copyright (C) 1995-1996 William Shubert
 * See "configure.h.in" for more copyright information.
 */

#include <wms.h>
#include <but/but.h>
#include <abut/msg.h>
#include <wms/clp.h>
#include <wms/str.h>
#include <but/textin.h>
#include <abut/term.h>
#include <but/tbin.h>
#include "../cgoban.h"
#include "login.h"
#include "../msg.h"
#include "conn.h"
#include "main.h"


/**********************************************************************
 * Forward Declarations
 **********************************************************************/
static void  cliLogin_dataIn(void *packet, const char *buf, int bufLen);
static void  mainDied(CliMain *main, void *packet);


/**********************************************************************
 * Functions
 **********************************************************************/
CliLogin  *cliLogin_create(CliData *cd, const char *user, const char *pass)  {
  CliConn  *conn;
  CliLogin  *cl;
  Str  errStr;
  XFontStruct  *fs;

  cl = wms_malloc(sizeof(CliLogin));
  MAGIC_SET(cl);
  cl->cd = cd;
  fs = butEnv_fontStr(cd->cg->env, 0);
  conn = cliConn_init(&cd->conn, cd->cg, cd->serverNum, cd->server,
		      fs->min_char_or_byte2, fs->max_char_or_byte2,
		      cliLogin_dataIn, cl);
  cd->connValid = TRUE;
  if (conn->err == cliConnErr_ok)  {
    str_print(&cd->cmdBuild, msg_login, cd->serverName);
    cl->main = cliMain_create(cd, mainDied, cl);
    cliMain_message(cl->main, str_chars(&cd->cmdBuild));
    cl->keepMainMessage = FALSE;
    cd->state = cliData_login;
    str_init(&cl->user);
    str_print(&cl->user, "%s\n", user);
    str_init(&cl->pass);
    str_print(&cl->pass, "%s\n", pass);
    cl->state = cliLogin_waitForUser;
    return(cl);
  } else  {
    MAGIC_UNSET(cl);
    wms_free(cl);
    str_init(&errStr);
    switch(conn->err)  {
    case cliConnErr_openSocket:
      str_print(&errStr, msg_cliOpenSocket,
		strerror(conn->errOut.errNum), cd->serverName);
      break;
    case cliConnErr_lookup:
      str_print(&errStr, msg_cliLookup, conn->errOut.serverName);
      break;
    case cliConnErr_connect:
      str_print(&errStr, msg_cliConnect,
		strerror(conn->errOut.errNum), cd->serverName);
      break;
    default:
      assert(0);
      break;
    }
    cgoban_createMsgWindow(cd->cg, "Cgoban Error", str_chars(&errStr));
    str_deinit(&errStr);
    return(NULL);
  }
}


void  cliLogin_destroy(CliLogin *cl, bool propagate)  {
  assert(MAGIC(cl));
  assert(MAGIC(cl->main));
  str_deinit(&cl->user);
  str_deinit(&cl->pass);
  cl->main->destroyCallback = NULL;
  if (propagate && cl->cd->conn.loginMode)
    cliMain_destroy(cl->main);
  MAGIC_UNSET(cl);
  wms_free(cl);
}




static void  cliLogin_dataIn(void *packet, const char *buf, int bufLen)  {
  CliLogin  *cl = packet;
  Str  errStr;

  assert(MAGIC(cl));
  assert(MAGIC(cl->cd));
  assert(MAGIC(cl->cd->cg));
  if (buf == NULL)  {
    str_init(&errStr);
    str_print(&errStr, msg_cliHangup,
	      cl->cd->serverName, strerror(bufLen));
    abutMsg_winCreate(cl->cd->cg->abut, "Cgoban Error",
		      str_chars(&errStr));
    str_deinit(&errStr);
    cliLogin_destroy(cl, TRUE);
    return;
  }
  cliMain_log(cl->main, buf);
  cliMain_log(cl->main, "\n");
  switch(cl->state)  {
  case cliLogin_waitForUser:
    if (!strcmp(buf, "Login: "))  {
      cl->state = cliLogin_waitForPassword;
      cliConn_send(&cl->cd->conn, str_chars(&cl->user));
      cliMain_log(cl->main, str_chars(&cl->user));
      cliMain_log(cl->main, "\n");
    }
    break;
  case cliLogin_waitForPassword:
    if (!strcmp(buf, "1 1") ||
	!strcmp(buf, "Password: "))  {
      cl->state = cliLogin_waitForPrompt;
      if ((str_len(&cl->pass) == 0) && (cl->cd->server == cliServer_nngs))  {
	str_init(&errStr);
	str_print(&errStr, msg_notAGuest,
		  str_chars(&cl->user));
	cliMain_message(cl->main, str_chars(&errStr));
	str_deinit(&errStr);
	cliLogin_destroy(cl, TRUE);
	return;
      } else  {
	cliConn_send(&cl->cd->conn, str_chars(&cl->pass));
	cliMain_log(cl->main, "******\n");
      }
    } else if (!strcmp(buf, "#> "))  {
      cliConn_send(&cl->cd->conn, "toggle client 1\n");
      cliMain_message(cl->main, msg_guest);
      cl->state = cliLogin_waitForPrompt;
      cl->keepMainMessage = TRUE;
    }
    break;
  case cliLogin_waitForPrompt:
    if (!strcmp(buf, "Login: "))  {
      cliMain_message(cl->main, msg_loginFailed);
      cliLogin_destroy(cl, TRUE);
      return;
    } else if (!strcmp(buf, "#> "))  {
      cliConn_send(&cl->cd->conn, "toggle client 1\n");
      cliMain_log(cl->main, "toggle client 1\n");
    } else if (!strcmp(buf, "1 5"))  {
      if (!cl->keepMainMessage)
	cliMain_clearMessage(cl->main);
      cliMain_activate(cl->main);
      cliLogin_destroy(cl, TRUE);
    }
    break;
  }
}


static void  mainDied(CliMain *main, void *packet)  {
  CliLogin  *login = packet;

  assert(MAGIC(login));
  cliLogin_destroy(login, FALSE);
}
