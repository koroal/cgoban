/*
 * src/client/game.c, part of Complete Goban (game program)
 * Copyright (C) 1995-1997 William Shubert
 * See "configure.h.in" for more copyright information.
 */

/*
 * Removed the bogus BadElf messages that were caused by messages from
 * the server that weren't being caught.  Dan Niles (-DDN-)
 */

#include <math.h>
#include <wms.h>
#include <but/but.h>
#include <but/list.h>
#include "../msg.h"
#include <but/plain.h>
#include <abut/swin.h>
#include <but/canvas.h>
#include <but/ctext.h>
#ifdef  _CLIENT_GAME_H_
#error LEVELIZATION ERROR
#endif
#include "game.h"


/**********************************************************************
 * Forward Declarations
 **********************************************************************/
static void  expandGames(CliGameList *gl, int gameNum);
static void  boardDead(CliBoard *board, void *packet);
static ButOut  unmap(ButWin *win);
static ButOut  map(ButWin *win);
static ButOut  resize(ButWin *win);
static ButOut  destroy(ButWin *win);
static void  getListEl(CliGameList *gl, int gNum, Str *out);
static ButOut  sResize(ButWin *win);
static ButOut  reload(But *but);
static ButOut  listPressed(But *but, int line);
static ButOut observeGame(void *packet, int boardId);

/**********************************************************************
 * Functions
 **********************************************************************/
CliGameList  *cliGameList_init(CliGameList *gl, CliData *data,
			       CliPlayerList *playerList)  {
  MAGIC_SET(gl);
  gl->data = data;
  gl->playerList = playerList;
  gl->maxGames = 0;
  gl->games = NULL;
  gl->defaultGame = -1;
  gl->playGame = -1;
  gl->kibGame = -1;
  gl->win = NULL;
  gl->elfBugReported = FALSE;
  data->observeGame = observeGame;
  data->obPacket = gl;
  return(gl);
}


CliGameList  *cliGameList_deinit(CliGameList *gl)  {
  int  i;
  Cgoban  *cg;

  assert(MAGIC(gl));
  cg = gl->data->cg;
  for (i = 0;  i < gl->maxGames;  ++i)  {
    if (gl->games[i].board)  {
      cliBoard_destroy(gl->games[i].board, FALSE);
      gl->games[i].board = NULL;
    }
    str_deinit(&gl->games[i].wName);
    str_deinit(&gl->games[i].bName);
    str_deinit(&gl->games[i].wRank);
    str_deinit(&gl->games[i].bRank);
    str_deinit(&gl->games[i].flags);
  }
  if (gl->games)
    wms_free(gl->games);
  if (gl->win)  {
    clp_setInt(cg->clp, "client.games.w",
	       (butWin_w(gl->win) - butWin_getMinW(gl->win)) /
	       butWin_getWStep(gl->win));
    clp_setDouble(cg->clp, "client.games.h2",
		  ((double)butWin_h(gl->win) /
		   (double)(butWin_getMinW(gl->win) + 6 *
			    butWin_getWStep(gl->win))));
    clp_setInt(cg->clp, "client.games.x", butWin_x(gl->win));
    clp_setInt(cg->clp, "client.games.y", butWin_y(gl->win));
    abutSwin_destroy(gl->swin);
    butWin_setDestroy(gl->win, NULL);
    butWin_destroy(gl->win);
  }
  MAGIC_UNSET(gl);
  return(gl);
}


void  cliGameList_openWin(CliGameList *gl)  {
  int  winW, minWinW, winH;
  Cgoban  *cg;
  int  i;
  bool  err;
  Str  listEl;
  int  physH, bw;
  double  wHRatio;

  assert(MAGIC(gl));
  str_init(&listEl);
  if (gl->win)  {
    XRaiseWindow(butEnv_dpy(gl->data->cg->env), butWin_xwin(gl->win));
    return;
  }
  cg = gl->data->cg;
  physH = butEnv_fontH(gl->data->cg->env, 0);
  bw = butEnv_stdBw(gl->data->cg->env);
  minWinW = physH * 18 + bw * 8;
  winW = minWinW + physH * 2 * clp_getInt(cg->clp, "client.games.w");
  wHRatio = clp_getDouble(cg->clp, "client.games.h2");
  winH = ((double)(minWinW + physH * 12) * wHRatio + 0.5);
  str_print(&listEl, "%s Game List", gl->data->serverName);
  gl->win = butWin_create(gl, cg->env, str_chars(&listEl), winW, winH,
			  unmap, map, resize, destroy);
  butWin_setMinW(gl->win, minWinW);
  butWin_setMaxW(gl->win, minWinW + physH * 12);
  butWin_setWStep(gl->win, physH * 2);
  i = clpEntry_iGetInt(clp_lookup(cg->clp, "client.games.x"), &err);
  if (!err)
    butWin_setX(gl->win, i);
  i = clpEntry_iGetInt(clp_lookup(cg->clp, "client.games.y"), &err);
  if (!err)
    butWin_setY(gl->win, i);
  butWin_setMaxH(gl->win, 0);
  butWin_setMinH(gl->win, cg->fontH * 6);
  butWin_activate(gl->win);
  gl->bg = butBoxFilled_create(gl->win, 0, BUT_DRAWABLE);
  butBoxFilled_setPixmaps(gl->bg, cg->bgLitPixmap, cg->bgShadPixmap,
			  cg->bgPixmap);
  gl->swin = abutSwin_create(gl, gl->win, 1, BUT_DRAWABLE|BUT_PRESSABLE,
			     sResize);
  gl->sBg = butPlain_create(gl->swin->win, 0, BUT_DRAWABLE, BUT_BG);
  gl->list = butList_create(listPressed, gl, gl->swin->win, 2,
			    BUT_DRAWABLE|BUT_PRESSABLE);
  gl->titleBox = butBoxFilled_create(gl->win, 1, BUT_DRAWABLE);
  gl->title = butList_create(NULL, NULL, gl->win, 2, BUT_DRAWABLE);
  butList_setLen(gl->title, 1);
  butList_changeLine(gl->title, 0, msg_gameListDesc);
  gl->reload = butCt_create(reload, gl, gl->win, 1, BUT_DRAWABLE|BUT_PRESSABLE,
			    "*");
  for (i = 1;  i < gl->maxGames;  ++i)  {
    if (gl->games[i].state != cliGame_noGame)  {
      getListEl(gl, i, &listEl);
      butList_setLen(gl->list, i);
      butList_changeLine(gl->list, i - 1, str_chars(&listEl));
      if (gl->games[i].state == cliGame_over)  {
	butList_setPress(gl->list, i - 1, FALSE);
      }
    }
  }
  butCan_resizeWin(gl->swin->win, 0, butList_len(gl->list) *
		   butEnv_fontH(gl->data->cg->env, 0) +
		   butEnv_stdBw(gl->data->cg->env) * 2, TRUE);
  str_deinit(&listEl);
}


void  cliGameList_gotMove(CliGameList *gl, const char *buf)  {
  int  args;
  int  moveNum, handicap;
  char  color, locStr[6];
  const char  *oppName;
  GoStone  stone;
  CliGame  *g;
  int  time1, time2, stonesLeft1, stonesLeft2;
  Bool IGS_DeadWorkAround;

  assert(MAGIC(gl));
  if (buf[0] == 'G')  {
    args = sscanf(buf, "Game %d %*s %*s (%*d %d %d) vs %*s (%*d %d %d)",
		  &gl->defaultGame, &time1, &stonesLeft1,
		  &time2, &stonesLeft2);
    assert(args == 5);
    expandGames(gl, gl->defaultGame);
    g = &gl->games[gl->defaultGame];
    switch(g->state)  {
    case cliGame_noGame:
    case cliGame_over:
      str_print(&gl->data->cmdBuild, "games %d\nmoves %d\n",
		gl->defaultGame, gl->defaultGame);
      cliConn_send(&gl->data->conn, str_chars(&gl->data->cmdBuild));
      gl->defaultGame = -1;
      return;
      break;
    case cliGame_waitNoOb:
      gl->defaultGame = -1;
      return;
      break;
    default:
      if (g->board)  {
	g->board->goban->timers[goStone_white].timeLeft = time1;
	g->board->goban->timers[goStone_black].timeLeft = time2;
	if (stonesLeft1 < 0)
	  stonesLeft1 = 0;
	if (stonesLeft2 < 0)
	  stonesLeft2 = 0;
	g->board->goban->timers[goStone_white].aux = stonesLeft1;
	g->board->goban->timers[goStone_black].aux = stonesLeft2;
      } else  {
	/*
	 * This code is also in gotGameInfo().  It really belongs here only
	 *   but a bug in NNGS forces me to put it in both places.
	 */
	g->state = cliGame_observing;
	g->board = cliBoard_create(gl->data, gl->defaultGame, &g->wName,
				   &g->wRank, &g->bName, &g->bRank,
				   g->size, g->handicap, g->komi,
				   time1, g->byTime * 60, boardDead, gl);
	g->gamePressed = FALSE;
	g->fromMatch = FALSE;
	if (g->board->moveWhite || g->board->moveBlack)  {
	  gl->playGame = gl->defaultGame;
	  if (gl->playerList->match)  {
	    if (g->board->moveWhite)
	      oppName = str_chars(&g->bName);
	    else
	      oppName = str_chars(&g->wName);
	    g->fromMatch = cliMatch_destroyChain(gl->playerList->match,
						 oppName,
						 &g->handicap, &g->komi,
						 &g->free);
	  }
	}
	str_print(&gl->data->cmdBuild, "moves %d\n", gl->defaultGame);
	cliConn_send(&gl->data->conn, str_chars(&gl->data->cmdBuild));
      }
      break;
    }
  } else if (gl->defaultGame >= 0)  {
    g = &gl->games[gl->defaultGame];
    args = sscanf(buf, "%d(%c): %5s", &moveNum, &color, locStr);
    assert(args == 3);
    if (!strncmp(locStr, "Handi",5))  {
      sscanf(buf, "%*s %*s %d", &handicap);
      if (handicap != g->handicap)  {
	g->handicap = handicap;
	goGame_destroy(g->board->game);
	g->board->game = goGame_create(g->size, goRules_japanese,
				       g->handicap, g->komi,
				       &g->board->timer, FALSE);
	g->board->goban->game = g->board->game;
	g->board->goban->pic->game = g->board->game;
	while (g->board->sgf->active->childH != NULL)  {
	  sgfElem_snip(g->board->sgf->active->childH,
		       g->board->sgf);
	}
	sgf_setHandicap(g->board->sgf, handicap);
	sgf_addHandicapStones(g->board->sgf, g->board->game->board);
	g->board->gameEnd = g->board->sgf->active;
	g->board->onTrack = TRUE;
	goban_stopTimer(g->board->goban);
	goban_startTimer(g->board->goban, goGame_whoseMove(g->board->game));
	goban_update(g->board->goban);
      }
    }
    if (color == 'W')  {
      stone = goStone_white;
    } else  {
      assert(color == 'B');
      stone = goStone_black;
    }
    if (gl->games[gl->defaultGame].board->goban->timers[stone].aux > 0)
      ++gl->games[gl->defaultGame].board->goban->timers[stone].aux;

    IGS_DeadWorkAround = FALSE;
    /*
     * IGS notifies the client that dead stone selection has begun
     * before it notifies the client of the last move made!  This
     * is a workaround to avoid confusing the client.
     */
    if (gl->games[gl->defaultGame].board->game->state
        == goGameState_selectDead) {
      if (strcmp(locStr, "Pass") == 0) {
        /*
         * We've received a move _after_ the game has already been ended
         * by IGS.  This must be the last move of the game before it ended.
         * apply the workaround.
         */
        gl->games[gl->defaultGame].board->game->state = goGameState_play;
        IGS_DeadWorkAround = TRUE;
      }
    }

    if (!cliBoard_gotMove(gl->games[gl->defaultGame].board, locStr, stone,
			  moveNum))  {
      Str  errStr;

      str_init(&errStr);
      str_print(&errStr, msg_cliGameBadMove, locStr, gl->defaultGame);
      cgoban_createMsgWindow(gl->data->cg,
			     "Client/Server Error Detected",
			     str_chars(&errStr));
      str_deinit(&errStr);
    }

    if (IGS_DeadWorkAround == TRUE) {
      /*
       * We revived a game from dead state to add the final passing
       * move received by from the server.  Return the game to 'select dead'
       * state.
       */
      gl->games[gl->defaultGame].board->game->state = goGameState_selectDead;
    }
  }
}
    

void  cliGameList_gotGameInfo(CliGameList *gl, const char *buf)  {
  int  gameNum, move, size, handicap, byTime, observers;
  float  komi;
  CliGame  *g;
  char  wName[41], bName[41], wRank[10], bRank[10], flags[5], temp[50];
  int  matches;

  if (buf[1] != '#')  {
    /* It's not the header. */
    matches = sscanf(buf, "[%d] %40s [ %[^] ] ] vs. %40s [ %[^] ] ] "
		     "(%d %d %d %g %d %[^)]) (%d)",
		     &gameNum, wName, wRank, bName, bRank,
		     &move, &size, &handicap, &komi, &byTime, flags,
		     &observers);
    if (matches != 12)  {
      if (!gl->elfBugReported)  {
	gl->elfBugReported = TRUE;
	cgoban_createMsgWindow(gl->data->cg, "Cgoban Error",
			       msg_gameBadElf);
      }
      return;
    }
    assert(matches == 12);
    expandGames(gl, gameNum);
    g = &gl->games[gameNum];
    if ((g->state == cliGame_noGame) || (g->state == cliGame_over))
      g->state = cliGame_idle;
    str_copyChars(&g->wName, wName);
    str_copyChars(&g->bName, bName);
    str_copyChars(&g->wRank, wRank);
    str_copyChars(&g->bRank, bRank);
    str_copyChars(&g->flags, flags);
    cliPlayerList_playerInGame(gl->playerList, wName, gameNum);
    cliPlayerList_playerInGame(gl->playerList, bName, gameNum);
    g->moveNum = move;
    if (g->board && g->fromMatch && (move < 2))  {
      if (g->komi != komi)
	str_print(&gl->data->cmdBuild, "komi %g\n", g->komi);
      if (g->board->moveBlack)  {
	if (g->free && (strchr(str_chars(&g->flags), 'F') == NULL))
	  str_catChars(&gl->data->cmdBuild, "free\n");
	if (g->handicap > 0)  {
	  sprintf(temp, "handicap %d\n", g->handicap);
	  str_catChars(&gl->data->cmdBuild, temp);
	}
      }
      cliConn_send(&gl->data->conn, str_chars(&gl->data->cmdBuild));
      g->fromMatch = FALSE;
    }
    assert((size >= 1) && (size <= 38));
    g->size = size;
    assert((handicap >= 0) && (handicap <= 27));
    g->handicap = handicap;
    g->komi = komi;
    g->byTime = byTime;
    g->observers = observers;
    if (gl->win)  {
      if (gameNum > butList_len(gl->list))  {
	butList_setLen(gl->list, gameNum);
	butCan_resizeWin(gl->swin->win, 0, butList_len(gl->list) *
			 butEnv_fontH(gl->data->cg->env, 0) +
			 butEnv_stdBw(gl->data->cg->env) * 2, TRUE);
      }
      getListEl(gl, gameNum, &gl->data->cmdBuild);
      butList_changeLine(gl->list, gameNum - 1,
			 str_chars(&gl->data->cmdBuild));
    }
  }
}


static void  expandGames(CliGameList *gl, int gameNum)  {
  int  newMaxGames;
  CliGame  *newGames;
  int  i;

  if (gameNum >= gl->maxGames)  {
    newMaxGames = (gameNum + 1) * 2;
    newGames = wms_malloc(newMaxGames * sizeof(CliGame));
    for (i = 0;  i < gl->maxGames;  ++i)
      newGames[i] = gl->games[i];
    for (;  i < newMaxGames;  ++i)  {
      newGames[i].state = cliGame_noGame;
      newGames[i].gamePressed = FALSE;
      str_init(&newGames[i].wName);
      str_init(&newGames[i].bName);
      str_init(&newGames[i].wRank);
      str_init(&newGames[i].bRank);
      str_init(&newGames[i].flags);
      newGames[i].moveNum = 0;
      newGames[i].size = 0;
      newGames[i].handicap = 0;
      newGames[i].komi = 0.0;
      newGames[i].byTime = 0;
      newGames[i].observers = 0;
      newGames[i].board = NULL;
    }
    if (gl->games)
      wms_free(gl->games);
    gl->maxGames = newMaxGames;
    gl->games = newGames;
  }
}


static void  boardDead(CliBoard *board, void *packet)  {
  CliGameList  *gl = packet;

  assert(MAGIC(gl));
  assert(gl->games[board->gameNum].board == board);
  str_print(&gl->data->cmdBuild, "observe %d\n", board->gameNum);
  cliConn_send(&gl->data->conn, str_chars(&gl->data->cmdBuild));
  gl->games[board->gameNum].board = NULL;
  gl->games[board->gameNum].state = cliGame_waitNoOb;
  if (gl->defaultGame == board->gameNum)
    gl->defaultGame = -1;
}


void  cliGameList_notObserving(CliGameList *gl, const char *buf)  {
  int  args;
  int  gameNum;

  args = sscanf(buf, "Removing game %d", &gameNum);
  assert(args == 1);
  assert(gameNum >= 0);
  assert(gameNum < gl->maxGames);
  assert(gl->games[gameNum].state == cliGame_waitNoOb);
  gl->games[gameNum].state = cliGame_idle;
}


void  cliGameList_kibitz(CliGameList *gl, const char *buf)  {
  int  args;
  const char  *kibitz;

  assert(MAGIC(gl));
  if (buf[0] == 'K')  {
    args = sscanf(buf, "Kibitz %40[^:]%*[^[][%d", gl->kibPlayer,
		  &gl->kibGame);
    if (args != 2)  {
      if (!gl->elfBugReported)  {
	gl->elfBugReported = TRUE;
	cgoban_createMsgWindow(gl->data->cg, "Cgoban Error",
			       msg_gameBadElf);
      }
      return;
    }
    assert(args == 2);
    expandGames(gl, gl->kibGame);
  } else  {
    if ((gl->kibGame < 0) ||
	(gl->games[gl->kibGame].state != cliGame_observing))  {
      return;
    }
    for (kibitz = buf;  *kibitz == ' ';  ++kibitz);
    if (clpEntry_getBool(gl->data->numKibitz))
      str_print(&gl->data->cmdBuild, "%d %s: %s\n",
		gl->games[gl->kibGame].board->game->maxMoves,
		gl->kibPlayer, kibitz);
    else
      str_print(&gl->data->cmdBuild, "%s: %s\n", gl->kibPlayer, kibitz);
    cliBoard_addKibitz(gl->games[gl->kibGame].board,
		       str_chars(&gl->data->cmdBuild));
  }
}


void  cliGameList_say(CliGameList *gl, const char *buf)  {
  assert(MAGIC(gl));
  if (gl->playGame >= 0)  {
    if (clpEntry_getBool(gl->data->numKibitz))
      str_print(&gl->data->cmdBuild, "%d %s\n",
		gl->games[gl->playGame].board->game->maxMoves, buf);
    else
      str_print(&gl->data->cmdBuild, "%s\n", buf);
    cliBoard_addKibitz(gl->games[gl->playGame].board,
		       str_chars(&gl->data->cmdBuild));
  }
}


static ButOut  unmap(ButWin *win)  {
  return(0);
}


static ButOut  map(ButWin *win)  {
  return(0);
}


static ButOut  resize(ButWin *win)  {
  int  w, h;
  CliGameList  *gl = butWin_packet(win);
  int  bw = butEnv_stdBw(butWin_env(win));
  int  tabs[10];
  static const ButTextAlign  tabAligns[10] =
    {butText_right, butText_left, butText_left, butText_left,
     butText_right, butText_right, butText_right, butText_right,
     butText_right, butText_right};
  int  fontH;
  int  physFontH = butEnv_fontH(butWin_env(win), 0);
  int  slideW = (physFontH * 3)/2;
  int  centerAdd;

  assert(MAGIC(gl));
  fontH = gl->data->cg->fontH;
  w = butWin_w(win);
  h = butWin_h(win);
  but_resize(gl->bg, 0, 0, w, h);
  but_resize(gl->titleBox, bw*2 + slideW, bw*2,
	     w - (bw*4 + slideW), fontH * 2);
  but_resize(gl->reload, bw*2, bw*2, slideW, fontH * 2);
  but_resize(gl->title, bw * 4 + slideW, bw * 2 + (fontH * 2 - physFontH) / 2,
	     w - (bw * 8 + slideW), physFontH);
  abutSwin_resize(gl->swin, bw*2, bw*2 + fontH*2, w - bw*4, h - bw*4 - fontH*2,
		  slideW, physFontH);
  butCan_resizeWin(gl->swin->win, 0, butList_len(gl->list) *
		   physFontH + bw * 2, TRUE);
  centerAdd = (physFontH * 3 + 1) / 4;
  tabs[0] = (physFontH * 2) / 2;
  tabs[1] = (physFontH * 3) / 2;
  tabs[2] = (physFontH * 17) / 2;
  tabs[3] = (physFontH * 20) / 2;
  tabs[4] = (physFontH * 37) / 2;
  tabs[5] = (physFontH * 41) / 2;
  tabs[6] = (physFontH * 45) / 2;
  tabs[7] = (physFontH * 49) / 2;
  tabs[8] = (physFontH * 53) / 2;
  tabs[9] = (physFontH * 57) / 2;
  if (w < physFontH * 20 + bw * 8)
    tabs[4] += physFontH * 20;
  if (w < physFontH * 22 + bw * 8)
    tabs[5] += physFontH * 20;
  if (w < physFontH * 24 + bw * 8)
    tabs[6] += physFontH * 20;
  if (w < physFontH * 26 + bw * 8)
    tabs[7] += physFontH * 20;
  if (w < physFontH * 28 + bw * 8)
    tabs[8] += physFontH * 20;
  if (w < physFontH * 30 + bw * 8)
    tabs[9] += physFontH * 20;
  butList_setTabs(gl->title, tabs, tabAligns, 10);
  butList_setTabs(gl->list, tabs, tabAligns, 10);
  return(0);
}


static ButOut  sResize(ButWin *win)  {
  int  w, h;
  AbutSwin  *swin = butWin_packet(win);
  CliGameList  *gl;
  int  bw = butEnv_stdBw(butWin_env(win));

  assert(MAGIC(swin));
  gl = swin->packet;
  assert(MAGIC(gl));
  w = butWin_w(win);
  h = butWin_h(win);
  but_resize(gl->sBg, 0, 0, w, h);
  butList_resize(gl->list, bw, bw, w - bw*2);
  return(0);
}


static ButOut  destroy(ButWin *win)  {
  CliGameList  *gl = butWin_packet(win);
  Cgoban  *cg = gl->data->cg;

  assert(MAGIC(gl));
  clp_setInt(cg->clp, "client.games.w",
	     (butWin_w(win) - butWin_getMinW(gl->win)) /
	     butWin_getWStep(gl->win));
  clp_setDouble(cg->clp, "client.games.h2",
		((double)butWin_h(win) /
		 (double)(butWin_getMinW(gl->win) + 6 *
			  butWin_getWStep(gl->win))));
  clp_setInt(cg->clp, "client.games.x", butWin_x(win));
  clp_setInt(cg->clp, "client.games.y", butWin_y(win));
  gl->win = NULL;
  return(0);
}


static void  getListEl(CliGameList *gl, int gNum, Str *out)  {
  switch(gl->games[gNum].state)  {
  case cliGame_noGame:
    str_copyCharsLen(out, "", 0);
    break;
  case cliGame_over:
    str_print(out, "\t-\t%s [%s]\tvs.\t%s [%s]\t%d\t%d\t%d\t%g\t\t%s",
	      str_chars(&gl->games[gNum].wName),
	      str_chars(&gl->games[gNum].wRank),
	      str_chars(&gl->games[gNum].bName),
	      str_chars(&gl->games[gNum].bRank),
	      gl->games[gNum].moveNum,
	      gl->games[gNum].size,
	      gl->games[gNum].handicap, gl->games[gNum].komi,
	      str_chars(&gl->games[gNum].flags));
    break;
  default:
    str_print(out, "\t%d\t%s [%s]\tvs.\t%s [%s]\t%d\t%d\t%d\t%g\t%s\t(%d)",
	      gNum,
	      str_chars(&gl->games[gNum].wName),
	      str_chars(&gl->games[gNum].wRank),
	      str_chars(&gl->games[gNum].bName),
	      str_chars(&gl->games[gNum].bRank),
	      gl->games[gNum].moveNum,
	      gl->games[gNum].size,
	      gl->games[gNum].handicap, gl->games[gNum].komi,
	      str_chars(&gl->games[gNum].flags),
	      gl->games[gNum].observers);
    break;
  }
}


void  cliGameList_gameGone(CliGameList *gl, const char *buf, int bufLen)  {
  int  args, gameNum;
  Str  gobanMessage, *winMsg;
  double  bScore, wScore;
  CliBoard  *board;
  GoStone winner = goStone_empty, loser;
  enum { adjourn, time, resign, score } endType;

  /*
   * 21 {Game 7: GoBot vs GoBot : Black resigns.}
   * 21 {Game 2: drwhat vs white : Black resigns. W 88.5 B 87.0}
   * 21 {Game 7: hunanren vs fine2 has adjourned.}
   * 21 {Game 34: waffle vs Benyu : W 55.5 B 72.0}
   * 21 {Game 36: ToLive vs bevin : Black forfeits on time.}
   *
   * Also found these. -DDN-
   * Note: It seems that IGS sends TWO messages at the end of a
   * request game.  One of the next three, followed by an adjourn.
   * 21 {Game 37: otari* vs cleaner* : White lost by Resign}
   * 21 {Game 14: otari* vs ogre* : Black lost by 4.5}
   * 21 {Game 63: Okada* vs Girl* : Black lost by Time}
   */
  args = sscanf(buf, "{Game %d", &gameNum);
  assert(args == 1);
  expandGames(gl, gameNum);
  cliPlayerList_playerInGame(gl->playerList,
			     str_chars(&gl->games[gameNum].wName), -1);
  cliPlayerList_playerInGame(gl->playerList,
			     str_chars(&gl->games[gameNum].bName), -1);
  /* Figure out exactly how the game ended. */
  if (!strcmp(buf + bufLen - 10, " resigns.}"))  {
    endType = resign;
    if (buf[bufLen - 11] == 'k')  {
      winner = goStone_white;
    } else  {
      assert(buf[bufLen - 11] == 'e');
      winner = goStone_black;
    }
  /* Added to catch Resign in replayed games. -DDN- */
  } else if (!strcmp(buf + bufLen - 16, " lost by Resign}")) {
    endType = resign;
    if (buf[bufLen - 17] == 'k')  {
      winner = goStone_white;
    } else  {
      assert(buf[bufLen - 17] == 'e');
      winner = goStone_black;
    }
  } else if (!strcmp(buf + bufLen - 12, " adjourned.}"))  {
    endType = adjourn;
  } else if (!strcmp(buf + bufLen - 19, " forfeits on time.}"))  {
    endType = time;
    if (buf[bufLen - 20] == 'k')  {
      winner = goStone_white;
    } else  {
      assert(buf[bufLen - 20] == 'e');
      winner = goStone_black;
    }
  /* Added to catch Time in replayed games. -DDN- */
  } else if (!strcmp(buf + bufLen - 14, " lost by Time}")) {
    endType = time;
    if (buf[bufLen - 15] == 'k')  {
      winner = goStone_white;
    } else  {
      assert(buf[bufLen - 15] == 'e');
      winner = goStone_black;
    }
  } else  {
    endType = score;
    args = sscanf(buf, "{Game %*d: %*s vs %*s %*[^0-9]%lf B %lf",
		  &wScore, &bScore);
    if (args != 2)  {
      /* Check to see if it is the end of a replayed game. -DDN- */
      char color[6];
      args = sscanf(buf, "{Game %*d: %*s vs %*s : %5s lost by %lf",
		    color, &bScore);
      if(args != 2) {
	if (!gl->elfBugReported)  {
	  gl->elfBugReported = TRUE;
	  cgoban_createMsgWindow(gl->data->cg, "Cgoban Error",
				 msg_gameBadElf);
	}
	return;
      }
      /* Set the score for the replayed game. -DDN- */
      if (color[4] == 'k')  {
	winner = goStone_white;
	wScore = bScore;
	bScore = 0.0;
      } else  {
	assert(color[4] == 'e');
	winner = goStone_black;
	wScore = 0.0;
      }
    } else if (wScore > bScore)  {
      winner = goStone_white;
    } else  {
      winner = goStone_black;
    }
  }

  /* Set up the flags field to describe the end of the game. */
  if (gl->games[gameNum].state != cliGame_noGame)  {
    winMsg = &gl->games[gameNum].flags;
    switch(endType)  {
    case adjourn:
      str_copyChars(winMsg, "Adj.");
      break;
    case time:
      str_print(winMsg, "%c+Time", goStone_char(winner));
      break;
    case resign:
      str_print(winMsg, "%c+Res.", goStone_char(winner));
      break;
    case score:
      str_print(winMsg, "%c+%g", goStone_char(winner),
		fabs(wScore - bScore));
      break;
    }
    gl->games[gameNum].state = cliGame_over;
    if (gl->win)  {
      getListEl(gl, gameNum, &gl->data->cmdBuild);
      butList_changeLine(gl->list, gameNum - 1,
			 str_chars(&gl->data->cmdBuild));
      butList_setPress(gl->list, gameNum - 1, FALSE);
    }
  }

  if (gl->games[gameNum].board)  {
    board = gl->games[gameNum].board;
    if (board->moveBlack || board->moveWhite)  {
      board->moveBlack = FALSE;
      board->moveWhite = FALSE;
      but_setFlags(board->goban->save, BUT_NOPRESS);
      butCt_setText(board->goban->quit, msg_close);
    }
    str_init(&gobanMessage);
    loser = goStone_opponent(winner);
    switch(endType)  {
    case resign:
      str_print(&gobanMessage, msg_gameGoneResign,
		msg_stoneNames[loser]);
      str_print(&board->result, msg_gameResultResign,
		board->game->maxMoves,
		msg_stoneNames[loser]);
      break;
    case adjourn:
      str_copyChars(&gobanMessage, msg_gameGoneAdjourn);
      str_print(&board->result, msg_gameResultAdjourn,
		board->game->maxMoves);
      break;
    case time:
      str_print(&gobanMessage, msg_gameGoneTime,
		msg_stoneNames[loser]);
      str_print(&board->result, msg_gameResultTime,
		board->game->maxMoves,
		msg_stoneNames[loser]);
      break;
    case score:
      str_print(&gobanMessage, msg_gameGoneScore,
		wScore, bScore, msg_stoneNames[winner]);
      if (wScore < bScore)  {
	float  temp;

	temp = wScore;
	wScore = bScore;
	bScore = temp;
      }
      str_print(&board->result, msg_gameResultScore,
		board->game->maxMoves,
		msg_stoneNames[winner], wScore, bScore);
    }
    if (gl->games[gameNum].board->goban->activeTimer != goStone_empty)
      goban_stopTimer(gl->games[gameNum].board->goban);
    goban_message(gl->games[gameNum].board->goban, str_chars(&gobanMessage));
    cliBoard_update(board);
    str_deinit(&gobanMessage);
    gl->games[gameNum].board->destroy = NULL;
    gl->games[gameNum].board = NULL;
  }
  if (gameNum == gl->defaultGame)
    gl->defaultGame = -1;
  if (gameNum == gl->playGame)
    gl->playGame = -1;
  gl->games[gameNum].gamePressed = FALSE;
}


void  cliGameList_gameStarts(CliGameList *gl, const char *buf)  {
  int  args, gameNum;

  args = sscanf(buf, "%*s %d", &gameNum);
  assert(args == 1);
  str_print(&gl->data->cmdBuild, "games %d\n", gameNum);
  cliConn_send(&gl->data->conn, str_chars(&gl->data->cmdBuild));
}


void  cliGameList_selectDead(CliGameList *gl, const char *buf)  {
  CliGame  *g;

  assert(MAGIC(gl));
  assert(gl->playGame >= 0);
  g = &gl->games[gl->playGame];
  if (g->board->gameEnd == g->board->sgf->active)  {
    if (g->board->goban->activeTimer != goStone_empty)
      goban_stopTimer(g->board->goban);
    g->board->game->state = goGameState_selectDead;
  }
  g->board->gameEnded = TRUE;
  goban_update(g->board->goban);
}


/* This really should be in board.c, IMHO */
void  cliGameList_deadStone(CliGameList *gl, const char *buf)  {
  CliGame  *g;
  char  deadStone[5];
  int  args, oldMoveNum, loc;
  SgfElem  *prevActive = NULL;
  GoBoardGroupIter  group;

  assert(MAGIC(gl));
  assert(gl->playGame >= 0);
  g = &gl->games[gl->playGame];
  args = sscanf(buf, "Removing @ %s", deadStone);
  assert(args == 1);
  loc = goBoard_str2Loc(g->board->game->board, deadStone);
  if (g->board->gameEnd != g->board->sgf->active)  {
    prevActive = g->board->sgf->active;
    g->board->sgf->active = g->board->gameEnd;
    g->board->sgf->mode = sgfInsert_main;
  }
  goBoardGroupIter(group, g->board->game->board, loc)  {
    sgf_addStone(g->board->sgf, goStone_empty,
		 goBoard_loc2Sgf(g->board->game->board,
				 goBoardGroupIter_loc(group,
						      g->board->game->board)));
  }
  g->board->gameEnd = g->board->sgf->active;
  if (prevActive != NULL)
    g->board->sgf->active = prevActive;
  if (g->board->onTrack)  {
    if (g->board->game->moveNum < g->board->game->maxMoves)  {
      oldMoveNum = g->board->game->moveNum;
      goGame_moveTo(g->board->game, g->board->game->maxMoves);
      if (g->board->goban->activeTimer != goStone_empty)
	goban_stopTimer(g->board->goban);
      g->board->game->state = goGameState_selectDead;
      goGame_markDead(g->board->game, loc);
      g->board->game->state = goGameState_play;
      goGame_moveTo(g->board->game, oldMoveNum);
    } else  {
      goGame_markDead(g->board->game, loc);
    }
  }
  goban_update(g->board->goban);
}


void  cliGameList_gotUndo(CliGameList *gl, const char *buf)  {
  CliGame  *g;
  int  args, gameNum = -1;

  assert(MAGIC(gl));
  args = sscanf(buf, "Undo in game %d", &gameNum);
  if (args == 0)  {
    gameNum = gl->playGame;
  } else  {
    assert(args == 1);
  }
  assert(gameNum >= 0);
  expandGames(gl, gameNum);
  g = &gl->games[gameNum];
  if (g->board && (g->board->game->moveNum > 0))  {
    --g->board->lastMoveRead;
    if ((g->board->moveWhite || g->board->moveBlack) && gl->data->actions)  {
      gl->data->actions->gotUndo(gl->data->packet);
    }
    while (g->board->gameEnd->type != sgfType_node)  {
      g->board->gameEnd = g->board->gameEnd->parent;
      assert(g->board->gameEnd != NULL);
    }
    g->board->gameEnd = g->board->gameEnd->parent;
    assert(g->board->gameEnd != NULL);
    if (g->board->onTrack)  {
      g->board->gameEnded = FALSE;
      if (g->board->game->moveNum == g->board->game->maxMoves)  {
	goGame_moveTo(g->board->game, g->board->game->moveNum - 1);
	g->board->sgf->active = g->board->gameEnd;
	if (g->board->game->state > goGameState_play)  {
	  g->board->game->state = goGameState_play;
	}
      }
      --g->board->game->maxMoves;
    }
    goban_update(g->board->goban);
  }
}


static ButOut  reload(But *but)  {
  CliGameList  *gl = but_packet(but);
  int  i, newLen;
  bool  lenChanged;

  assert(MAGIC(gl));
  cliConn_send(&gl->data->conn, "games\n");
  for (i = 0;  i < gl->maxGames;  ++i)  {
    if (gl->games[i].state == cliGame_over)  {
      gl->games[i].state = cliGame_noGame;
      butList_changeLine(gl->list, i - 1, "");
    }
  }
  lenChanged = FALSE;
  for (newLen = butList_len(gl->list);
       (newLen > 1) && (gl->games[newLen].state == cliGame_noGame);
       --newLen)  {
    lenChanged = TRUE;
  }
  if (lenChanged)  {
    butList_setLen(gl->list, newLen);
    butCan_resizeWin(gl->swin->win, 0,
		     newLen * butEnv_fontH(gl->data->cg->env, 0) +
		     butEnv_stdBw(gl->data->cg->env) * 2, TRUE);
  }
  return(0);
}


static ButOut  listPressed(But *but, int line)  {
  CliGameList  *gl;

  gl = but_packet(but);
  assert(MAGIC(gl));
  return(observeGame(gl, line + 1));
}

static ButOut observeGame(void *packet, int boardId) {
  CliGameList *gl = packet;

  assert(MAGIC(gl));
  assert((boardId > 0) && (boardId < gl->maxGames));
  if (gl->games[boardId].state == cliGame_noGame)
    return(BUTOUT_ERR);
  if (gl->games[boardId].board)  {
    XRaiseWindow(butEnv_dpy(gl->data->cg->env),
		 butWin_xwin(gl->games[boardId].board->goban->win));
  } else if (!gl->games[boardId].gamePressed)  {
    gl->games[boardId].gamePressed = TRUE;
    str_print(&gl->data->cmdBuild, "observe %d\n", boardId);
    cliConn_send(&gl->data->conn, str_chars(&gl->data->cmdBuild));
  }
  return(0);
}

bool cliGame_tellIsSay(CliGameList *list, Str *tell)  {
  char tellerName[41];
  int matches;

  if (list->playGame >= 0)  {
    matches = sscanf(str_chars(tell), "*%40[^*]", tellerName);
    if (matches != 1)  {
      if (!list->elfBugReported)  {
	list->elfBugReported = TRUE;
	cgoban_createMsgWindow(list->data->cg, "Cgoban Error",
			       msg_gameBadElf);
      }
      return(FALSE);
    }
    assert(matches == 1);
    if (!strcmp(str_chars(&list->games[list->playGame].wName), tellerName) ||
	!strcmp(str_chars(&list->games[list->playGame].bName), tellerName))  {
      cliBoard_addKibitz(list->games[list->playGame].board,
			 str_chars(tell));
      return(TRUE);
    }
  }
  return(FALSE);
}
