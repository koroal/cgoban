/*
 * src/client/board.c, part of Complete Goban (game program)
 * Copyright (C) 1995-1996 William Shubert
 * See "configure.h.in" for more copyright information.
 */

#include <wms.h>
#include <but/but.h>
#include <but/ctext.h>
#include <but/plain.h>
#include <but/radio.h>
#include "../msg.h"
#include "../sgfPlay.h"
#include "../editBoard.h"
#ifdef  _CLIENT_BOARD_H_
     LEVELIZATION ERROR
#endif
#include "board.h"


/**********************************************************************
 * Forward Declarations
 **********************************************************************/
static bool  rewAndBackOk(void *packet), fwdOk(void *packet);
static bool  ffwdOk(void *packet);
static void  gobanDestroyed(void *packet);
static GobanOut  quitClosePressed(void *packet);
static GobanOut  adjournPressed(void *packet);
static GobanOut  newKibitz(void *packet, const char *kib);
static GobanOut  rewPressed(void *packet);
static GobanOut  backPressed(void *packet);
static GobanOut  fwdPressed(void *packet);
static GobanOut  ffPressed(void *packet);
static GobanOut  donePressed(void *packet);
static GobanOut  gridPressed(void *packet, int loc);
static GobanOut  editPressed(void *packet);
static ButOut  shiftPressed(But *but, bool press);
static ButOut  ctrlPressed(But *but, bool press);
static ButOut  dontResign(But *but);
static ButOut  yesResign(But *but);
static void  cliBoard_updateWithToggle(CliBoard *cli, uint toggle);


/**********************************************************************
 * Functions
 **********************************************************************/
CliBoard  *cliBoard_create(CliData *data, int gameNum,
			   const Str *wName, const Str *wRank,
			   const Str *bName, const Str *bRank,
			   int size, int handicap, float komi, int mainTime,
			   int byTime, void (*destroy)(CliBoard *cli,
						       void *packet),
			   void *packet)  {
  static const GobanActions  actions = {
    gridPressed, quitClosePressed, cliBoard_passPressed, rewPressed,
    backPressed, fwdPressed, ffPressed, donePressed,
    NULL,adjournPressed,  /* dispute, save */
    editPressed, NULL, /* gameInfo */
    &help_cliBoard,  /* Help */

    gobanDestroyed,
    rewAndBackOk, rewAndBackOk, fwdOk, ffwdOk};
  static const ButKey  shiftKeys[] = {{XK_Shift_L, 0,0},
				      {XK_Shift_R,0,0}, {0,0,0}};
  static const ButKey  ctrlKeys[] = {{XK_Control_L, 0,0},
				     {XK_Control_R,0,0}, {0,0,0}};
  CliBoard  *cli;
  Str  gameName;

  cli = wms_malloc(sizeof(CliBoard));
  MAGIC_SET(cli);
  cli->data = data;
  cliData_incRef(cli->data);
  cli->cg = data->cg;
  cli->sgf = sgf_create();
  sgf_setSize(cli->sgf, size);
  sgf_setKomi(cli->sgf, komi);
  sgf_setPlayerName(cli->sgf, goStone_white, str_chars(wName));
  sgf_playerRank(cli->sgf, goStone_white, str_chars(wRank));
  sgf_setPlayerName(cli->sgf, goStone_black, str_chars(bName));
  sgf_playerRank(cli->sgf, goStone_black, str_chars(bRank));
  sgf_setDate(cli->sgf);
  sgf_style(cli->sgf, "Cgoban " VERSION);
  cli->gameNum = gameNum;
  cli->lastMoveRead = -1;
  cli->gameEnded = FALSE;
  cli->onTrack = TRUE;
  cli->timer.type = goTime_canadian;
  cli->timer.main = 360000;
  cli->timer.by = byTime;
  cli->timer.aux = 25;
  cli->game = goGame_create(size, goRules_japanese, handicap, komi,
			    &cli->timer, FALSE);
  if (handicap)  {
    sgf_setHandicap(cli->sgf, handicap);
    sgf_addHandicapStones(cli->sgf, cli->game->board);
  }
  cli->gameEnd = cli->sgf->active;
  cli->game->forcePlay = TRUE;
  cli->sgf->mode = sgfInsert_variant;
  str_init(&gameName);
  str_print(&gameName, msg_cliGameName, gameNum,
	    str_chars(wName), str_chars(wRank),
	    str_chars(bName), str_chars(bRank));
  cli->goban = goban_create(cli->cg, &actions, cli, cli->game,
			    "client", str_chars(&gameName));
  goban_startTimer(cli->goban, goGame_whoseMove(cli->game));
  goban_setHold(cli->goban, TRUE);
  but_setFlags(cli->goban->save, BUT_NOPRESS);
  str_deinit(&gameName);
  cli->goban->iDec1 = grid_create(&cli->cg->cgbuts, NULL, NULL,
				  cli->goban->iWin, 2, BUT_DRAWABLE, 0);
  grid_setStone(cli->goban->iDec1, goStone_white, FALSE);
  if (data->server == cliServer_nngs)
    grid_setVersion(cli->goban->iDec1, CGBUTS_WORLDWEST(0));
  else
    grid_setVersion(cli->goban->iDec1, CGBUTS_WORLDEAST(0));
  cli->shiftKeytrap = butKeytrap_create(shiftPressed, cli, cli->goban->win,
					BUT_DRAWABLE|BUT_PRESSABLE);
  but_setKeys(cli->shiftKeytrap, shiftKeys);
  butKeytrap_setHold(cli->shiftKeytrap, FALSE);
  cli->ctrlKeytrap = butKeytrap_create(ctrlPressed, cli, cli->goban->win,
				       BUT_DRAWABLE|BUT_PRESSABLE);
  but_setKeys(cli->ctrlKeytrap, ctrlKeys);
  butKeytrap_setHold(cli->ctrlKeytrap, FALSE);
  cli->destroy = destroy;
  cli->packet = packet;
  cli->moveWhite = !strcmp(str_chars(wName), str_chars(&data->userName));
  cli->moveBlack = !strcmp(str_chars(bName), str_chars(&data->userName));
  str_init(&cli->result);

  str_initStr(&cli->wName, wName);
  str_initStr(&cli->bName, bName);
  if (cli->moveWhite || cli->moveBlack)  {
    goban_setKibitz(cli->goban, newKibitz, clp_getInt(data->cg->clp,
						      "client.saykib"));
  } else  {
    goban_setKibitz(cli->goban, newKibitz, -1);
  }

  if (cli->moveWhite)  {
    if (data->actions)  {
      cli->goban->pic->allowedMoves |= goPicMove_noWhite;
      data->actions->newGame(data->packet, goStone_white, size, handicap, komi,
			     mainTime, byTime, cli);
    }
  } else  {
    cli->goban->pic->allowedMoves |= goPicMove_noWhite;
  }
  if (cli->moveBlack)  {
    if (data->actions)  {
      cli->goban->pic->allowedMoves |= goPicMove_noBlack;
      data->actions->newGame(data->packet, goStone_black, size, handicap, komi,
			     mainTime, byTime, cli);
    }
  } else  {
    cli->goban->pic->allowedMoves |= goPicMove_noBlack;
  }
  butCt_setText(cli->goban->save, msg_adjourn);
  if (cli->moveWhite || cli->moveBlack)  {
    but_setFlags(cli->goban->save, BUT_PRESSABLE);
    butCt_setText(cli->goban->quit, msg_resign);
  }
  cliBoard_update(cli);
  
  return(cli);
}


void  cliBoard_destroy(CliBoard *cli, bool propagate)  {
  assert(MAGIC(cli));
  sgf_destroy(cli->sgf);
  if (cli->data->actions && (cli->moveWhite || cli->moveBlack))  {
    cli->data->actions->endGame(cli->data->packet);
  }
  if (cli->goban)  {
    goban_destroy(cli->goban, FALSE);
    cli->goban = NULL;
  }
  if (cli->game)  {
    goGame_destroy(cli->game);
    cli->game = NULL;
  }
  if (propagate && cli->destroy)  {
    cli->destroy(cli, cli->packet);
  }
  cliData_decRef(cli->data);
  MAGIC_UNSET(cli);
  wms_free(cli);
}


Bool  cliBoard_gotMove(CliBoard *cli, const char *locStr, GoStone color,
		       int moveNum)  {
  int  loc, oldMoveNum = 0;
  SgfElem  *oldSgf = NULL;
  Bool  result = TRUE;

  assert(MAGIC(cli));
  if (cli->game->handicap != 0)
    --moveNum;
  if (moveNum != cli->lastMoveRead + 1)  {
    return(TRUE);
  }
  cli->lastMoveRead = moveNum;
  if (!strcmp(locStr, "Pass"))
    loc = 0;
  else  {
    loc = goBoard_str2Loc(cli->game->board, locStr);
    if (loc == -1)  {
      return(FALSE);
    }
  }
  if (color == goStone_white)  {
    if (cli->moveBlack && cli->data->actions)  {
      cli->data->actions->gotMove(cli->data->packet, goStone_white, loc);
    }
  } else  {
    assert(color == goStone_black);
    if (cli->moveWhite && cli->data->actions)  {
      cli->data->actions->gotMove(cli->data->packet, goStone_black, loc);
    }
  }
  cli->sgf->mode = sgfInsert_main;
  if (cli->gameEnd != cli->sgf->active)  {
    oldSgf = cli->sgf->active;
    cli->sgf->active = cli->gameEnd;
    if (cli->onTrack)  {
      oldMoveNum = cli->game->moveNum;
      goGame_moveTo(cli->game, cli->game->maxMoves);
    }
  }
  goban_stopTimer(cli->goban);
  if (cli->onTrack)  {
    if (!goGame_isLegal(cli->game, color, loc))
      result = FALSE;
    else
      goGame_move(cli->game, color, loc, &cli->goban->timers[color]);
  }
  goban_startTimer(cli->goban, goGame_whoseMove(cli->game));
  sgf_addNode(cli->sgf);
  assert(cli->sgf->active == cli->gameEnd->childH);
  if (loc == 0)
    sgf_pass(cli->sgf, color);
  else
    sgf_move(cli->sgf, color, goBoard_loc2Sgf(cli->game->board, loc));
  cli->gameEnd = cli->sgf->active;
  if (oldSgf)  {
    cli->sgf->active = oldSgf;
    if (cli->onTrack)
      goGame_moveTo(cli->game, oldMoveNum);
  }
  cli->sgf->mode = sgfInsert_variant;
  if (cli->onTrack)
    cliBoard_update(cli);
  return(result);
}


static bool  rewAndBackOk(void *packet)  {
  CliBoard  *cli = packet;

  assert(MAGIC(cli));
  return(cli->game->moveNum > 0);
}


static bool  fwdOk(void *packet)  {
  CliBoard  *cli = packet;

  assert(MAGIC(cli));
  if (cli->onTrack)  {
    return(cli->sgf->active != cli->gameEnd);
  } else  {
    return(cli->sgf->active->childH != NULL);
  }
}


static bool  ffwdOk(void *packet)  {
  CliBoard  *cli = packet;

  assert(MAGIC(cli));
  return((cli->game->moveNum < cli->game->maxMoves) ||
	 (cli->gameEnd != cli->sgf->active));
}


static void  gobanDestroyed(void *packet)  {
  CliBoard  *cli = packet;

  assert(MAGIC(cli));
  cli->goban = NULL;
  cliBoard_destroy(cli, TRUE);
}


static GobanOut  adjournPressed(void *packet)  {
  CliBoard  *cli = packet;

  assert(MAGIC(cli));
  assert(cli->moveBlack || cli->moveWhite);
  cliConn_send(&cli->data->conn, "adjourn\n");
  return(gobanOut_noDraw);
}


static GobanOut  quitClosePressed(void *packet)  {
  CliBoard  *cli = packet;
  AbutMsgOpt  buttons[2];

  assert(MAGIC(cli));
  if (cli->moveBlack || cli->moveWhite)  {
    buttons[0].name = msg_noCancel;
    buttons[0].callback = dontResign;
    buttons[0].packet = cli;
    buttons[0].keyEq = NULL;
    buttons[1].name = msg_yesResign;
    buttons[1].callback = yesResign;
    buttons[1].packet = cli;
    buttons[1].keyEq = NULL;
    if (cli->goban->msgBox)
      abutMsg_destroy(cli->goban->msgBox, FALSE);
    cli->goban->msgBox = abutMsg_optCreate(cli->cg->abut, cli->goban->win,
					   4, msg_reallyResign,
					   NULL, NULL, 2, buttons);
  } else  {
    cliBoard_destroy(cli, TRUE);
  }
  return(gobanOut_noDraw);
}


static GobanOut  newKibitz(void *packet, const char *kib)  {
  CliBoard  *cli = packet;
  const char  *opponent;

  assert(MAGIC(cli));
  if (clpEntry_getBool(cli->data->numKibitz))
    str_print(&cli->data->cmdBuild, "%d %s: %s\n",
	      cli->game->maxMoves,
	      str_chars(&cli->data->userName), kib);
  else
    str_print(&cli->data->cmdBuild, "%s: %s\n",
	      str_chars(&cli->data->userName), kib);
  goban_catComments(cli->goban, str_chars(&cli->data->cmdBuild));
  if (cli->moveWhite || cli->moveBlack)  {
    switch(goban_kibType(cli->goban))  {
    case 0:
      str_print(&cli->data->cmdBuild, "say %s\n", kib);
      break;
    case 1:
      assert(MAGIC(&cli->bName));
      assert(MAGIC(&cli->wName));
      if (cli->moveWhite)
	opponent = str_chars(&cli->bName);
      else
	opponent = str_chars(&cli->wName);
      str_print(&cli->data->cmdBuild, "tell %s %s\nkibitz %d %s\n",
		opponent, kib, cli->gameNum, kib);
      break;
    case 2:
      str_print(&cli->data->cmdBuild, "kibitz %d %s\n", cli->gameNum, kib);
      break;
    }
  } else
    str_print(&cli->data->cmdBuild, "kibitz %d %s\n", cli->gameNum, kib);
  if (cli->destroy)
    cliConn_send(&cli->data->conn, str_chars(&cli->data->cmdBuild));
  return(gobanOut_noDraw);
}


static GobanOut  rewPressed(void *packet)  {
  CliBoard  *cli = packet;

  assert(MAGIC(cli));
  if (cli->game->state > goGameState_play)  {
    cli->game->state = goGameState_play;
  }
  goGame_moveTo(cli->game, 0);
  sgf_setActiveNodeNumber(cli->sgf, 0);
  cliBoard_update(cli);
  return(gobanOut_noDraw);
}


static GobanOut  backPressed(void *packet)  {
  CliBoard  *cli = packet;

  assert(MAGIC(cli));
  if (cli->game->state > goGameState_play)  {
    cli->game->state = goGameState_play;
  }
  goGame_moveTo(cli->game, cli->game->moveNum - 1);
  sgf_setActiveNodeNumber(cli->sgf, cli->game->moveNum);
  cliBoard_update(cli);
  return(gobanOut_noDraw);
}


static GobanOut  fwdPressed(void *packet)  {
  CliBoard  *cli = packet;

  assert(MAGIC(cli));
  assert(cli->game->moveNum < cli->game->maxMoves);
  goGame_moveTo(cli->game, cli->game->moveNum + 1);
  cli->sgf->active = cli->sgf->active->activeChild;
  while ((cli->sgf->active->activeChild != NULL) &&
	 (cli->sgf->active->activeChild->type != sgfType_node))  {
    cli->sgf->active = cli->sgf->active->activeChild;
  }
  cliBoard_update(cli);
  return(gobanOut_noDraw);
}


static GobanOut  ffPressed(void *packet)  {
  CliBoard  *cli = packet;
  SgfElem  *elem;

  assert(MAGIC(cli));
  for (elem = &cli->sgf->top;  elem;  elem = elem->childH)  {
    elem->activeChild = elem->childH;
  }
  sgf_play(cli->sgf, cli->game, NULL, -1, cli->gameEnd);
  assert(cli->sgf->active == cli->gameEnd);
  goGame_moveTo(cli->game, cli->game->maxMoves);
  cli->onTrack = TRUE;
  cliBoard_update(cli);
  return(gobanOut_noDraw);
}


static GobanOut  gridPressed(void *packet, int loc)  {
  return(cliBoard_gridPressed(packet, loc, FALSE));
}


GobanOut  cliBoard_gridPressed(void *packet, int loc, bool forceMove)  {
  CliBoard  *cli = packet;
  char  move[10];
  int  newNode, i;

  assert(MAGIC(cli));
  if (!forceMove && (butEnv_keyModifiers(cli->cg->env) & ShiftMask))  {
    strcpy(move, goBoard_loc2Sgf(cli->game->board, loc));
    newNode = -1;
    if (goBoard_stone(cli->game->board, loc) == goStone_empty)  {
      i = sgfElem_findMove(cli->sgf->active, move, 1);
      if (i >= 0)  {
	newNode = cli->game->moveNum + i;
      } else  {
	i = sgfElem_findMove(cli->sgf->active, move, -1);
	if (i >= 0)
	  newNode = cli->game->moveNum - i;
      }
    } else  {
      i = sgfElem_findMove(cli->sgf->active, move, -1);
      assert(i >= 0);
      newNode = cli->game->moveNum - i;
    }
    if (newNode >= 0)  {
      if (cli->game->state > goGameState_play)  {
	cli->game->state = goGameState_play;
      }
      goGame_moveTo(cli->game, newNode);
      sgf_setActiveNodeNumber(cli->sgf, newNode);
      if (cli->gameEnded && (cli->game->moveNum == cli->game->maxMoves))  {
	if (cli->goban->activeTimer != goStone_empty)
	  goban_stopTimer(cli->goban);
	cli->game->state = goGameState_selectDead;
      }
      cliBoard_update(cli);
      return(gobanOut_noDraw);
    } else  {
      return(gobanOut_err);
    }
  } else if (!forceMove &&
	     (butEnv_keyModifiers(cli->cg->env) & ControlMask))  {
    sgf_addNode(cli->sgf);
    sgf_move(cli->sgf, cli->game->whoseMove,
	     goBoard_loc2Sgf(cli->game->board, loc));
    goGame_move(cli->game, cli->game->whoseMove, loc,
		&cli->goban->timers[cli->game->whoseMove]);
    cli->onTrack = FALSE;
    cliBoard_update(cli);
    return(gobanOut_noDraw);
  } else  {
    goBoard_loc2Str(cli->game->board, loc, move);
    strcat(move, "\n");
    cliConn_send(&cli->data->conn, move);
    return(gobanOut_noDraw);
  }
}


GobanOut  cliBoard_passPressed(void *packet)  {
  CliBoard  *cli = packet;

  assert(MAGIC(cli));
  cliConn_send(&cli->data->conn, "pass\n");
  return(gobanOut_noDraw);
}


static GobanOut  donePressed(void *packet)  {
  CliBoard  *cli = packet;

  assert(MAGIC(cli));
  cliConn_send(&cli->data->conn, "done\n");
  return(gobanOut_noDraw);
}  


static GobanOut  editPressed(void *packet)  {
  CliBoard  *cli = packet;

  assert(MAGIC(cli));
  assert(MAGIC(cli->data));
  editBoard_createSgf(cli->data->cg, cli->sgf);
  return(gobanOut_noDraw);
}  


static ButOut  shiftPressed(But *but, bool press)  {
  CliBoard  *cli = but_packet(but);

  assert(MAGIC(cli));
  /*
   * Alas, the modifier flags don't change until AFTER the press, so we
   *   have to invert the shift here.
   */
  cliBoard_updateWithToggle(cli, ShiftMask);
  return(0);
}


static ButOut  ctrlPressed(But *but, bool press)  {
  CliBoard  *cli = but_packet(but);

  assert(MAGIC(cli));
  /*
   * Alas, the modifier flags don't change until AFTER the press, so we
   *   have to invert the shift here.
   */
  cliBoard_updateWithToggle(cli, ControlMask);
  return(0);
}


static ButOut  dontResign(But *but)  {
  CliBoard  *cli = but_packet(but);

  assert(MAGIC(cli));
  goban_noMessage(cli->goban);
  return(0);
}


static ButOut  yesResign(But *but)  {
  CliBoard  *cli = but_packet(but);

  assert(MAGIC(cli));
  cliConn_send(&cli->data->conn, "resign\n");
  goban_noMessage(cli->goban);
  return(0);
}


void  cliBoard_update(CliBoard *cli)  {
  cliBoard_updateWithToggle(cli, 0);
}


static void  cliBoard_updateWithToggle(CliBoard *cli, uint toggle)  {
  uint  moves = 0;  /* To shut up gcc warning. */

  if (cli->gameEnded && (cli->gameEnd == cli->sgf->active))  {
    if (cli->goban->activeTimer != goStone_empty)
      goban_stopTimer(cli->goban);
    cli->game->state = goGameState_selectDead;
  } else
    cli->game->state = goGameState_play;
  switch((butEnv_keyModifiers(cli->cg->env) & (ShiftMask|ControlMask)) ^
	 toggle)  {
  case 0:
    moves = goPicMove_legal;
    if (!cli->moveWhite || cli->data->actions ||
	(cli->sgf->active != cli->gameEnd))  {
      moves |= goPicMove_noWhite;
    }
    if (!cli->moveBlack || cli->data->actions ||
	(cli->sgf->active != cli->gameEnd))  {
      moves |= goPicMove_noBlack;
    }
    break;
  case ShiftMask:
    moves = goPicMove_empty | goPicMove_stone;
    if (!(but_getFlags(cli->goban->pass) & BUT_PRESSABLE))
      moves |= goPicMove_noPass;
    break;
  case ControlMask:
    moves = goPicMove_legal;
    break;
  case ShiftMask|ControlMask:
    moves = 0;
    break;
  }
  cli->goban->pic->allowedMoves = moves;
  goban_update(cli->goban);
  if (str_len(&cli->result) &&
      (cli->sgf->active == cli->gameEnd))  {
    butText_set(cli->goban->infoText, str_chars(&cli->result));
  }
}
