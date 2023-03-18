/*
 * $Source: /cvsroot/cgoban1/cgoban1/src/client/main.c,v $
 * $Revision: 1.2 $
 * $Date: 2000/02/26 22:53:49 $
 *
 * src/client/main.c, part of Complete Goban (game program)
 * Copyright © 1995-2000 William Shubert
 * See "configure.h.in" for more copyright information.
 */

#include <wms.h>
#include <wms/clp.h>
#include <wms/str.h>
#include <but/but.h>
#include <but/ctext.h>
#include <abut/msg.h>
#include <abut/term.h>
#include <but/tbin.h>
#include <but/textin.h>
#include <but/radio.h>
#include "../cgoban.h"
#include "../msg.h"
#include "../help.h"
#ifdef _CLIENT_MAIN_H_
#error LEVELIZATION ERROR
#endif
#include "main.h"


/**********************************************************************
 * Forward Declarations
 **********************************************************************/
static ButOut  unmap(ButWin *win);
static ButOut  map(ButWin *win);
static ButOut  resize(ButWin *win);
static ButOut  destroy(ButWin *win);
static void  dataIn(void *packet, const char *buf, int bufLen);
static ButOut  userInput(But *but, const char *input);
static ButOut  quitPressed(But *but);
static ButOut  gamesPressed(But *but);
static ButOut  playersPressed(But *but);
static ButOut  iResize(ButWin *win);
static ButOut  newState(But *but, int value);
static ButOut  msgBoxKilled(But *okBut);
static void  client2client(CliMain *cMain, const char *buf);
static ButOut  prevHistory(But *but,
			   KeySym keysym, uint keyModifiers, void *context);
static ButOut  nextHistory(But *but,
			   KeySym keysym, uint keyModifiers, void *context);

/**********************************************************************
 * Functions
 **********************************************************************/
CliMain  *cliMain_create(CliData *data,
			 void (*destroyCallback)(CliMain *cMain, void *packet),
			 void *packet)  {
  Str  winTitle;
  CliMain *cMain;
  int  winW, winH;
  Cgoban *cg;
  int  i;
  bool  err;

  assert(MAGIC(data));
  cg = data->cg;
  cMain = wms_malloc(sizeof(CliMain));
  MAGIC_SET(cMain);
  cliData_incRef(data);
  cMain->data = data;
  winW = (clp_getDouble(cg->clp, "client.main.w") * cg->fontH + 0.5);
  winH = (clp_getDouble(cg->clp, "client.main.h") * cg->fontH + 0.5);
  str_init(&winTitle);
  str_print(&winTitle, "%s Client", data->serverName);
  cMain->win = butWin_iCreate(cMain, cg->env, str_chars(&winTitle),
			     winW, winH, &cMain->iWin, FALSE, 48, 48,
			     unmap, map, resize, iResize, destroy);
  i = clpEntry_iGetInt(clp_lookup(cg->clp, "client.main.x"), &err);
  if (!err)
    butWin_setX(cMain->win, i);
  i = clpEntry_iGetInt(clp_lookup(cg->clp, "client.main.y"), &err);
  if (!err)
    butWin_setY(cMain->win, i);
  butWin_setMinW(cMain->win, cg->fontH * 10);
  butWin_setMinH(cMain->win, cg->fontH * 10);
  butWin_setMaxW(cMain->win, 0);
  butWin_setMaxH(cMain->win, 0);
  butWin_activate(cMain->win);
  str_deinit(&winTitle);
  cMain->bg = butBoxFilled_create(cMain->win, 0, BUT_DRAWABLE);
  butBoxFilled_setPixmaps(cMain->bg, cg->bgLitPixmap, cg->bgShadPixmap,
			  cg->bgPixmap);
  cMain->quit = butCt_create(quitPressed, cMain, cMain->win, 1,
			    BUT_DRAWABLE|BUT_PRESSABLE, msg_quit);
  cMain->games = butCt_create(gamesPressed, cMain, cMain->win, 1,
			     BUT_DRAWABLE, msg_games);
  cMain->players = butCt_create(playersPressed, cMain, cMain->win, 1,
			       BUT_DRAWABLE, msg_players);
  cMain->help = butCt_create(cgoban_createHelpWindow,
			    &help_cliMain, cMain->win, 1,
			    BUT_DRAWABLE|BUT_PRESSABLE, msg_help);
  cMain->script = abutTerm_create(cg->abut, cMain->win, 1, FALSE);
  cMain->message = NULL;
  butTbin_setMaxLines(cMain->script->tbin, 250);
  if (data->conn.directConn)
    cMain->tin = butTextin_create(userInput, cMain, cMain->win, 1,
				 BUT_DRAWABLE, "", 200);
  else
    cMain->tin = butTextin_create(userInput, cMain, cMain->win, 1,
				 BUT_DRAWABLE|BUT_PRESSABLE|BUT_KEYED,
				 "", 200);

  cMain->userState = butRadio_create(newState, cMain, cMain->win, 1,
				    BUT_DRAWABLE,
				    cliPlayer_unknown - cliPlayer_closed, 3);
  cMain->looking = butText_create(cMain->win, 2, BUT_DRAWABLE | BUT_PRESSTHRU,
				 "!", butText_center);
  cMain->open = butText_create(cMain->win, 2, BUT_DRAWABLE | BUT_PRESSTHRU,
			      "O", butText_center);
  cMain->closed = butText_create(cMain->win, 2, BUT_DRAWABLE | BUT_PRESSTHRU,
				"X", butText_center);
  cliPlayerList_init(&cMain->playerList, data);
  cliGameList_init(&cMain->gameList, data, &cMain->playerList);
  cMain->state = cliMain_idle;
  cMain->promptVisible = FALSE;
  str_init(&cMain->outStr);
  cMain->iBg = butBoxFilled_create(cMain->iWin, 0, BUT_DRAWABLE);
  butBoxFilled_setPixmaps(cMain->iBg, cg->bgLitPixmap, cg->bgShadPixmap,
			  cg->bgPixmap);
  cMain->iPic = grid_create(&cg->cgbuts, NULL, NULL, cMain->iWin, 1,
			   BUT_DRAWABLE, 0);
  grid_setStone(cMain->iPic, goStone_white, FALSE);
  grid_setLineGroup(cMain->iPic, gridLines_none);
  if (data->server == cliServer_nngs)
    grid_setVersion(cMain->iPic, CGBUTS_WORLDWEST(3));
  else
    grid_setVersion(cMain->iPic, CGBUTS_WORLDEAST(4));
  cliLook_init(&cMain->look, cg, data->server);
  cMain->historyLen = 0;
  cMain->historyBeingFilled = 0;
  cMain->historyBeingViewed = 0;
  for (i = 0;  i < MAIN_HISTORYMAXLEN;  ++i) {
    str_init(&cMain->history[i]);
  }
  butTextin_setSpecialKey(cMain->tin, XK_p,
			  ControlMask, ShiftMask|ControlMask,
			  prevHistory, cMain);
  butTextin_setSpecialKey(cMain->tin, XK_Up, 0, ShiftMask|ControlMask,
			  prevHistory, cMain);
  butTextin_setSpecialKey(cMain->tin, XK_n,
			  ControlMask, ShiftMask|ControlMask,
			  nextHistory, cMain);
  butTextin_setSpecialKey(cMain->tin, XK_Down, 0, ShiftMask|ControlMask,
			  nextHistory, cMain);
  cMain->destroyCallback = destroyCallback;
  cMain->packet = packet;
  return(cMain);
}


void  cliMain_activate(CliMain *cMain)  {
  assert(MAGIC(cMain));
  cMain->data->state = cliData_main;
  cMain->data->conn.newData = dataIn;
  cMain->data->conn.packet = cMain;
  cMain->data->conn.loginMode = FALSE;
  but_setFlags(cMain->games, BUT_PRESSABLE);
  but_setFlags(cMain->players, BUT_PRESSABLE);
  but_setFlags(cMain->tin, BUT_PRESSABLE|BUT_KEYED);
  but_setFlags(cMain->games, BUT_PRESSABLE);
  but_setFlags(cMain->userState, BUT_PRESSABLE);
  cliConn_prompt(&cMain->data->conn);
  cliConn_send(&cMain->data->conn,
	       "toggle quiet 0\ntoggle verbose 0\ngames\nwho\n");
}


void  cliMain_destroy(CliMain *cMain)  {
  Cgoban  *cg;

  assert(MAGIC(cMain));
  if (cMain->destroyCallback)
    cMain->destroyCallback(cMain, cMain->packet);
  cliMain_clearMessage(cMain);
  if (cMain->win)  {
    cg = cMain->data->cg;
    clp_setDouble(cg->clp, "client.main.w",
		  (double)butWin_w(cMain->win) / (double)cg->fontH);
    clp_setDouble(cg->clp, "client.main.h",
		  (double)butWin_h(cMain->win) / (double)cg->fontH);
    clp_setInt(cg->clp, "client.main.x", butWin_x(cMain->win));
    clp_setInt(cg->clp, "client.main.y", butWin_y(cMain->win));
    butWin_setDestroy(cMain->win, NULL);
    butWin_destroy(cMain->win);
  }
  abutTerm_destroy(cMain->script);
  cliGameList_deinit(&cMain->gameList);
  cliPlayerList_deinit(&cMain->playerList);
  cliLook_deinit(&cMain->look);
  str_deinit(&cMain->outStr);
  cliData_closeConns(cMain->data);
  cliData_decRef(cMain->data);
  MAGIC_UNSET(cMain);
  wms_free(cMain);
}


static ButOut  unmap(ButWin *win)  {
  return(0);
}


static ButOut  map(ButWin *win)  {
  return(0);
}


static ButOut  resize(ButWin *win)  {
  CliMain  *cMain = butWin_packet(win);
  int  w = butWin_w(win), h = butWin_h(win);
  int  bw = butEnv_stdBw(butWin_env(win));
  int  fontH = cMain->data->cg->fontH;
  int  midY;

  assert(MAGIC(cMain));
  but_resize(cMain->bg, 0, 0, w, h);
  midY = (w - (bw * 2 + fontH * 6)) / 2;
  but_resize(cMain->games, bw * 2, bw * 2, midY - bw * 2, fontH * 2);
  but_resize(cMain->userState, midY + bw, bw * 2, fontH * 6, fontH * 2);
  but_resize(cMain->closed, midY + bw, bw * 2, fontH * 2, fontH * 2);
  but_resize(cMain->open, midY + bw + fontH * 2, bw * 2, fontH * 2, fontH * 2);
  but_resize(cMain->looking, midY + bw + fontH * 4, bw * 2,
	     fontH * 2, fontH * 2);
  but_resize(cMain->players, midY + bw * 2 + fontH * 6, bw * 2,
	     w - (midY + bw * 4 + fontH * 6), fontH * 2);
  abutTerm_resize(cMain->script, bw * 2, bw * 3 + fontH * 2,
		  w - bw * 4, h - fontH * 6 - bw * 6);
  but_resize(cMain->tin, bw * 2, h - fontH * 4 - bw * 3, w - bw * 4, fontH * 2);
  midY = (w - bw) / 2;
  but_resize(cMain->help, bw * 2, h - fontH * 2 - bw * 2,
	     midY - bw * 2, fontH * 2);
  but_resize(cMain->quit, midY + bw, h - fontH * 2 - bw * 2,
	     w - (midY + bw * 3), fontH * 2);
  return(0);
}


static ButOut  destroy(ButWin *win)  {
  CliMain  *cMain = win->packet;
  Cgoban  *cg;

  assert(MAGIC(cMain));
  cg = cMain->data->cg;
  clp_setDouble(cg->clp, "client.main.w",
		(double)butWin_w(win) / (double)cg->fontH);
  clp_setDouble(cg->clp, "client.main.h",
		(double)butWin_h(win) / (double)cg->fontH);
  clp_setInt(cg->clp, "client.main.x", butWin_x(win));
  clp_setInt(cg->clp, "client.main.y", butWin_y(win));
  cMain->win = NULL;
  cliMain_destroy(cMain);
  return(0);
}


static void  dataIn(void *packet, const char *buf, int bufLen)  {
  CliMain  *cMain = packet;
  int  dataType, args, userNum;
  CliGameState  oldState;
  const char  *temp;

  assert(MAGIC(cMain));
  if (buf == NULL)  {
    str_print(&cMain->outStr, msg_cliHangup,
	      cMain->data->serverName, strerror(bufLen));
    abutMsg_winCreate(cMain->data->cg->abut, "Cgoban Error",
		      str_chars(&cMain->outStr));
    cliMain_destroy(cMain);
  } else  {
    oldState = cMain->state;
    str_copyCharsLen(&cMain->outStr, buf, bufLen);
    str_catChar(&cMain->outStr, '\n');
    if (cMain->state == cliMain_inFile)  {
      if (!strcmp(buf, cMain->fileEnd))
	cMain->state = cliMain_idle;
      else
	cliMain_log(cMain, str_chars(&cMain->outStr));
    } else if (bufLen)  {
      args = sscanf(buf, "%d", &dataType);
      if (args != 1)  {
	dataType = 0;
	args = 1;
#if  DEBUG
	printf("Did not pass: \"%s\"\n", buf);
#endif
      } else  {
	while (isdigit(*buf))  {
	  ++buf;
	  --bufLen;
	}
	++buf;
	--bufLen;
	str_copyCharsLen(&cMain->outStr, buf, bufLen);
	str_catChar(&cMain->outStr, '\n');
      }
      assert(args == 1);
      /*
       * Set promptReady to FALSE.  If we got a 1, we'll immediately set it to
       *   TRUE so no harm done.
       */
      cMain->data->conn.promptReady = FALSE;
      switch(dataType)  {
      case 1:  /* Prompt. */
	cMain->state = cliMain_idle;
	if (!cMain->promptVisible)  {
	  butTbin_insert(cMain->script->tbin, "#> ");
	  cMain->promptVisible = TRUE;
	}
	cliConn_prompt(&cMain->data->conn);
	break;
      case 2:  /* Beep. */
	XBell(butEnv_dpy(cMain->data->cg->env), 0);
	break;
      case 5:  /* Error. */
	XBell(butEnv_dpy(cMain->data->cg->env), 0);
	cliMain_log(cMain, str_chars(&cMain->outStr));
	break;
      case 7:  /* Game info. */
	cliGameList_gotGameInfo(&cMain->gameList, buf);
	break;
      case 8:  /* Help. */
	assert(!strcmp(buf, "File"));
	cMain->state = cliMain_inFile;
	cMain->fileEnd = "8 File";
	break;
      case 9:  /* Misc. */
	/*
	 * 9 Use <match wms W 19 21 11> or <decline wms> to respond.
	 * 9 Requesting match in 21 min with wwms as White.
	 * 9 wwms declines your request for a match.
	 * 9 You decline the match offer from wms.
	 */
	if (!strcmp(buf, "File"))  {
	  cMain->state = cliMain_inFile;
	  cMain->fileEnd = "9 File";
	} else if (!strncmp(buf, "Removing game ", 14))
	  cliGameList_notObserving(&cMain->gameList, buf);
	else if (!strncmp(buf, "Removing @ ", 11))
	  cliGameList_deadStone(&cMain->gameList, buf);
	else if (!strncmp(buf, "You can check ", 14))
	  cliGameList_selectDead(&cMain->gameList, buf);
	else if (!strncmp(buf, "Use <match", 10))  {
	  char  oppName[21];
	  const char  *oppRank;
	  int  rankDiff;

	  args = sscanf(buf, "Use <match %20s", oppName);
	  oppRank = cliPlayerList_getRank(&cMain->playerList, oppName);
	  if (oppRank == NULL)
	    rankDiff = 1;
	  else
	    rankDiff = strcmp(oppRank,
			      cliPlayerList_getRank(&cMain->playerList,
						    str_chars(&cMain->data->
							      userName)));
	  cliMatch_matchCommand(cMain->data, buf, &cMain->playerList.match,
				rankDiff);
	} else if (((bufLen > 34) &&
		  !strcmp(buf + bufLen - 34,
			  "declines your request for a match.")) ||
		 ((bufLen > 26) &&
		  !strcmp(buf + bufLen - 26,
			  "withdraws the match offer.")))
	  cliMatch_declineCommand(cMain->playerList.match, buf);
	else if (strncmp(buf, "Adding game to", 14) &&
		 strncmp(buf, "{Game ", 6) &&
		 strncmp(buf, "Requesting ", 11) &&
		 strncmp(buf, "You decline", 11) &&
		 strncmp(buf, "Match [", 7) &&
		 strncmp(buf, "Updating ", 9) &&
		 strncmp(buf, "Declining offer", 15) &&
		 ((bufLen < 26) || strcmp(buf + bufLen - 26,
					  "updates the match request.")))  {
	  cliMain_log(cMain, str_chars(&cMain->outStr));
	}
	break;
      case 11:  /* Kibitzes. */
	cliGameList_kibitz(&cMain->gameList, buf);
	break;
      case 15:  /* Game moves. */
	cliGameList_gotMove(&cMain->gameList, buf);
	/*
	 * On NNGS, if you start a game when your state is closed, you
	 *   become open.  Update the cMain window's radio button here.
	 */
	if ((cMain->gameList.playGame >= 0) &&
	    (cliPlayer_closed + butRadio_get(cMain->userState) ==
	     cliPlayer_closed))  {
	  butRadio_set(cMain->userState, cliPlayer_open - cliPlayer_closed,
		       FALSE);
	}
	break;
      case 19:  /* Say. */
	cliGameList_say(&cMain->gameList, buf);
	break;
      case 21:  /* Shouts & game start/stop info & user log in/out. */
	if (!strncmp(buf, "{Game ", 6))  {
	  if (strchr(buf, '@') != NULL)  {
	    /* A continued game. */
	    cliGameList_gameStarts(&cMain->gameList, buf);
	  } else  {
	    cliGameList_gameGone(&cMain->gameList, buf, bufLen);
	  }
	} else if (!strncmp(buf, "{Match ", 7))  {
	  /* A new game. */
	  cliGameList_gameStarts(&cMain->gameList, buf);
	} else if (!strcmp(buf + bufLen - 17, "has disconnected}"))  {
	  cliPlayerList_disconnected(&cMain->playerList, buf);
	} else if (!strcmp(buf + bufLen - 15, "has connected.}"))  {
	  cliPlayerList_connected(&cMain->playerList, buf);
	} else
	  cliMain_log(cMain, str_chars(&cMain->outStr));
	break;
      case 22:  /* A board. */
	cliLook_gotData(&cMain->look, str_chars(&cMain->outStr));
	break;
      case 24:  /* Tells. */
	for (temp = buf;  *temp != ':';  ++temp);
	temp += 2;
	if (!strncmp(temp, "CLIENT: ", 8))  {
	  client2client(cMain, temp);
	} else if (!cliGame_tellIsSay(&cMain->gameList, &cMain->outStr))
	  cliMain_log(cMain, str_chars(&cMain->outStr));
	break;
      case 25:  /* Results list. */
	assert(!strcmp(buf, "File"));
	cMain->state = cliMain_inFile;
	cMain->fileEnd = "25 File";
	break;
      case 27:  /* Who output. */
	cMain->state = cliMain_gotWho;
	cliPlayerList_whoOutput(&cMain->playerList, buf);
	break;
      case 28:  /* Undo. */
	cliGameList_gotUndo(&cMain->gameList, buf);
	break;

      case 20:  /* Game results. */
      case 23:  /* A list of stored games. */
      case 32:  /* Channel tell. */
      case 39:  /* Version number. */
      case 40:  /* Confirming who you spoke to, or something like that. */
      case 42:  /* IGS "user" output. */
      case 500:  /* NNGS emotes. */
      case 501:  /* NNGS tell you who the emote went to. */
      default:
	cliMain_log(cMain, str_chars(&cMain->outStr));
	break;
      }
      if ((oldState == cliMain_gotWho) && (cMain->state != cliMain_gotWho))  {
	cliPlayerList_sort(&cMain->playerList);
	userNum = cliPlayerList_lookupPlayer(&cMain->playerList,
					     str_chars(&cMain->data->userName));
	if ((userNum >= 0) &&
	    (butRadio_get(cMain->userState) ==
	     cliPlayer_unknown - cliPlayer_closed))  {
	  butRadio_set(cMain->userState,
		       cMain->playerList.players[userNum].state -
		       cliPlayer_closed,
		       FALSE);
	}
      }
    }
  }
}


static ButOut  userInput(But *but, const char *input)  {
  CliMain  *cMain = but_packet(but);

  assert(MAGIC(cMain));
  if (input[0] != '\0') {
    str_copyChars(&cMain->history[cMain->historyBeingFilled], input);
    if (++cMain->historyBeingFilled >= MAIN_HISTORYMAXLEN)
      cMain->historyBeingFilled = 0;
    cMain->historyBeingViewed = 0;
    if (++cMain->historyLen >= MAIN_HISTORYMAXLEN)
      cMain->historyLen = MAIN_HISTORYMAXLEN - 1;
  }
  str_print(&cMain->data->cmdBuild, "%s\n", input);
  cliConn_send(&cMain->data->conn, str_chars(&cMain->data->cmdBuild));
  if (!cMain->promptVisible)
    str_print(&cMain->data->cmdBuild, "#> %s\n", input);
  butTbin_insert(cMain->script->tbin, str_chars(&cMain->data->cmdBuild));
  butTextin_set(but, "", FALSE);
  cMain->promptVisible = FALSE;
  return(0);
}


static ButOut  quitPressed(But *but)  {
  cliMain_destroy(but_packet(but));
  return(0);
}


static ButOut  iResize(ButWin *win)  {
  CliMain  *cMain = butWin_packet(win);
  int  w, h, size;
  int  border;

  assert(MAGIC(cMain));
  w = butWin_w(win);
  h = butWin_h(win);
  but_resize(cMain->iBg, 0,0, w,h);
  border = 2*butEnv_stdBw(butWin_env(win));
  size = w - border * 2;
  but_resize(cMain->iPic, border, border, size, size);
  return(0);
}


static ButOut  gamesPressed(But *but)  {
  CliMain  *cMain = but_packet(but);

  assert(MAGIC(cMain));
  cliGameList_openWin(&cMain->gameList);
  return(0);
}


static ButOut  playersPressed(But *but)  {
  CliMain  *cMain = but_packet(but);

  assert(MAGIC(cMain));
  cliPlayerList_openWin(&cMain->playerList);
  return(0);
}


static ButOut  newState(But *but, int value)  {
  CliPlayerState  newState = value + cliPlayer_closed;
  CliMain  *cMain = but_packet(but);
  int  userNum;

  assert(MAGIC(but));
  switch(newState)  {
  case cliPlayer_looking:
    cliConn_send(&cMain->data->conn, "toggle open 1\ntoggle looking 1\n");
    break;
  case cliPlayer_open:
    cliConn_send(&cMain->data->conn, "toggle open 1\ntoggle looking 0\n");
    break;
  case cliPlayer_closed:
    cliConn_send(&cMain->data->conn, "toggle open 0\ntoggle looking 0\n");
    break;
  default:
    assert(0);
    break;
  }
  userNum = cliPlayerList_lookupPlayer(&cMain->playerList,
				       str_chars(&cMain->data->userName));
  if (userNum >= 0)  {
    cliPlayerList_setState(&cMain->playerList, userNum, newState);
  }
  return(0);
}


void  cliMain_message(CliMain *cMain, const char *message)  {
  AbutMsgOpt  ok;

  assert(MAGIC(cMain));
  cliMain_clearMessage(cMain);
  ok.name = msg_ok;
  ok.callback = msgBoxKilled;
  ok.packet = cMain;
  ok.keyEq = cg_return;
  cMain->message = abutMsg_optCreate(cMain->data->cg->abut, cMain->win, 4,
				    message, NULL, NULL, 1, &ok);
}


void  cliMain_clearMessage(CliMain *cMain)  {
  assert(MAGIC(cMain));
  if (cMain->message)
    abutMsg_destroy(cMain->message, FALSE);
  cMain->message = NULL;
}


static ButOut  msgBoxKilled(But *okBut)  {
  CliMain  *cMain = but_packet(okBut);

  assert(MAGIC(cMain));
  assert(cMain->message != NULL);
  abutMsg_destroy(cMain->message, FALSE);
  cMain->message = NULL;
  return(0);
}


void  cliMain_log(CliMain *cMain, const char *str)  {
  if (cMain->promptVisible)  {
    butTbin_delete(cMain->script->tbin,
		   butTbin_len(cMain->script->tbin) - 3, 3);
    cMain->promptVisible = FALSE;
  }
  butTbin_insert((cMain)->script->tbin, (str));
}


static void  client2client(CliMain *cMain, const char *buf)  {
  char  oppName[31], freeChar = 'x';
  int  matches, hcap;
  float  komi;

  if (strncmp(buf, "CLIENT: <cgoban ", 16))
    return;
  while ((*buf != '>') && (*buf != '\0'))
    ++buf;
  if ((buf[0] == '\0') || (buf[1] == '\0') || (buf[2] == '\0'))
    return;  /* Not actually from cgoban.  Oops. */
  buf += 2;
  matches = sscanf(buf, "match %30s wants handicap %d, komi %f, %c",
		   oppName, &hcap, &komi, &freeChar);
  if (matches >= 3)  {
    /* Extra match info. */
    cliMatch_extraInfo(cMain->playerList.match, oppName, hcap, komi,
		       freeChar == 'f');
  }
}


static ButOut  prevHistory(But *but,
			   KeySym keysym, uint keyModifiers, void *context) {
  CliMain  *cMain = context;
  int  i;

  assert(MAGIC(cMain));
  if (cMain->historyBeingViewed == -cMain->historyLen) {
    return(BUTOUT_ERR);
  }
  if (cMain->historyBeingViewed == 0) {
    str_copyChars(&cMain->history[cMain->historyBeingFilled],
		  butTextin_get(cMain->tin));
  }
  --cMain->historyBeingViewed;
  i = cMain->historyBeingFilled + cMain->historyBeingViewed;
  if (i < 0)
    i += MAIN_HISTORYMAXLEN;
  butTextin_set(cMain->tin, str_chars(&cMain->history[i]), FALSE);
  return(0);
}


static ButOut  nextHistory(But *but,
			   KeySym keysym, uint keyModifiers, void *context) {
  CliMain  *cMain = context;
  int  i;

  assert(MAGIC(cMain));
  if (cMain->historyBeingViewed == 0) {
    return(BUTOUT_ERR);
  }
  ++cMain->historyBeingViewed;
  i = cMain->historyBeingFilled + cMain->historyBeingViewed;
  if (i < 0)
    i += MAIN_HISTORYMAXLEN;
  butTextin_set(cMain->tin, str_chars(&cMain->history[i]), FALSE);
  return(0);
}
