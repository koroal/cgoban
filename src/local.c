/*
 * $Source: /cvsroot/cgoban1/cgoban1/src/local.c,v $
 * $Revision: 1.2 $
 * $Date: 2000/02/09 06:50:02 $
 *
 * Part of Complete Goban (game program)
 * Copyright © 1995-1996,2000 William Shubert
 * See "configure.h.in" for more copyright information.
 */


#include <wms.h>
#include <wms/rnd.h>
#include <but/but.h>
#include <but/plain.h>
#include <but/ctext.h>
#include <but/text.h>
#include <abut/term.h>
#include <abut/msg.h>
#include <abut/fsel.h>
#include <wms/clp.h>
#include <wms/str.h>
#include <but/text.h>
#include <but/textin.h>
#include "cgoban.h"
#include "goban.h"
#include "msg.h"
#include "goGame.h"
#include "goPic.h"
#include "cgbuts.h"
#include "sgf.h"
#include "sgfIn.h"
#include "sgfOut.h"
#include "sgfPlay.h"
#include "editBoard.h"
#ifdef  _LOCAL_H_
   Levelization Error.
#endif
#include "local.h"


/**********************************************************************
 * Forward references
 **********************************************************************/
static GobanOut  gridPressed(void *packet, int loc);
static GobanOut  quitPressed(void *packet);
static GobanOut  passPressed(void *packet);
static GobanOut  rewPressed(void *packet);
static GobanOut  backPressed(void *packet);
static GobanOut  fwdPressed(void *packet);
static GobanOut  ffPressed(void *packet);
static GobanOut  donePressed(void *packet);
static GobanOut  disputePressed(void *packet);
static GobanOut  savePressed(void *packet);
static GobanOut  editPressed(void *packet);
static void  saveFile(AbutFsel *fsel, void *packet, const char *fname);
static void  writeGobanComments(Local *l), readGobanComments(Local *l);
static void  addDisputedTriangles(GoGame *game, Sgf *sgf);
static void  addTimeInfoToSgf(Local *l, GoStone color);
static void  recordTimeLoss(Local *l);
static bool  rewOk(void *packet), backOk(void *packet), fwdOk(void *packet);
static void  gobanDestroyed(void *packet);
static ButOut  reallyQuit(But *but);
static ButOut  dontQuit(But *but);
static ButOut  quitWinDead(void *packet);
static void  setTimers(Local *l);


/**********************************************************************
 * Global variables
 **********************************************************************/
static const GobanActions  local_actions = {
  gridPressed, quitPressed, passPressed, rewPressed, backPressed,
  fwdPressed, ffPressed, donePressed, disputePressed, savePressed,
  editPressed, NULL /* gameInfo */,
  &help_localBoard,
  gobanDestroyed,
  rewOk, backOk, fwdOk, fwdOk};


/**********************************************************************
 * Functions
 **********************************************************************/
Local  *local_create(Cgoban *cg, GoRules rules, int size, int hcap,
		     float komi, const char *white, const char *black,
		     GoTimeType timeType, int mainTime, int byTime,
		     int auxTime)  {
  Sgf  *mc;
  Str  tmp;
  GoTime  time;

  mc = sgf_create();
  str_init(&tmp);
  sgf_setRules(mc, rules);
  sgf_setSize(mc, size);
  sgf_setHandicap(mc, hcap);
  sgf_setKomi(mc, komi);
  sgf_setPlayerName(mc, goStone_white, white);
  sgf_setPlayerName(mc, goStone_black, black);
  str_print(&tmp, msg_localTitle, white, black);
  sgf_setTitle(mc, str_chars(&tmp));
  sgf_setDate(mc);
  sgf_style(mc, "Cgoban " VERSION);
  time.type = timeType;
  time.main = mainTime;
  time.by = byTime;
  time.aux = auxTime;
  goTime_describeStr(&time, &tmp);
  sgf_setTimeFormat(mc, str_chars(&tmp));
  str_deinit(&tmp);
  return(local_createSgf(cg, mc));
}


Local  *local_createFile(Cgoban *cg, const char *fname)  {
  Str  badFile;
  Sgf  *mc;
  SgfElem  *style;
  const char  *err;

  mc = sgf_createFile(cg, fname, &err, NULL);
  if (mc == NULL)  {
    abutMsg_winCreate(cg->abut, "Cgoban Error", err);
    return(NULL);
  }
  style = sgf_findType(mc, sgfType_style);
  if (!style || strncmp(str_chars(style->sVal), "Cgoban", 6))  {
    str_init(&badFile);
    str_print(&badFile, msg_notCgobanFile, fname);
    abutMsg_winCreate(cg->abut, "Cgoban Error", str_chars(&badFile));
    str_deinit(&badFile);
    sgf_destroy(mc);
    return(NULL);
  }
  return(local_createSgf(cg, mc));
}


Local  *local_createSgf(Cgoban *cg, Sgf *mc)  {
  Local  *l;
  SgfElem  *me;
  GoRules  rules;
  int  size, hcap;
  float  komi;
  GoTime  time;
  const char  *title;

  assert(MAGIC(cg));
  l = wms_malloc(sizeof(Local));
  MAGIC_SET(l);
  l->cg = cg;
  l->moves = mc;
  l->endGame = NULL;
  me = sgf_findType(mc, sgfType_rules);
  if (me)  {
    rules = (GoRules)me->iVal;
  } else
    rules = goRules_japanese;
  me = sgf_findType(mc, sgfType_size);
  if (me)  {
    size = me->iVal;
  } else  {
    size = 19;
  }
  me = sgf_findType(mc, sgfType_handicap);
  if (me)  {
    hcap = me->iVal;
  } else  {
    hcap = 0;
  }
  me = sgf_findType(mc, sgfType_komi);
  if (me)  {
    komi = (float)me->iVal / 2.0;
  } else  {
    komi = 0.0;
  }
  me = sgf_findFirstType(mc, sgfType_playerName);
  while (me)  {
    me = sgfElem_findFirstType(me, sgfType_playerName);
  }
  me = sgf_findFirstType(mc, sgfType_time);
  if (me)  {
    goTime_parseDescribeChars(&time, str_chars(me->sVal));
  } else  {
    time.type = goTime_none;
  }
  l->game = goGame_create(size, rules, hcap, komi, &time, FALSE);
  sgf_play(mc, l->game, NULL, -1, NULL);
  l->modified = FALSE;
      
  me = sgf_findFirstType(mc, sgfType_title);
  if (me)  {
    title = str_chars(me->sVal);
  } else  {
    title = msg_noTitle;
  }

  l->goban = goban_create(cg, &local_actions, l, l->game, "local",
			  title);
  l->goban->iDec1 = grid_create(&cg->cgbuts, NULL, NULL, l->goban->iWin, 2,
				BUT_DRAWABLE, 0);
  grid_setStone(l->goban->iDec1, goStone_white, FALSE);
  l->goban->iDec2 = grid_create(&cg->cgbuts, NULL, NULL, l->goban->iWin, 2,
				BUT_DRAWABLE, 0);
  grid_setStone(l->goban->iDec2, goStone_black, FALSE);
  setTimers(l);
  l->lastComment = NULL;
  assert((hcap == 0) || ((hcap >= 2) && (hcap <= 27)));
  
  l->fsel = NULL;
  l->reallyQuit = NULL;
  return(l);
}


void  local_destroy(Local *l)  {
  assert(MAGIC(l));
  if (l->goban)
    goban_destroy(l->goban, FALSE);
  if (l->game)
    goGame_destroy(l->game);
  if (l->fsel)
    abutFsel_destroy(l->fsel, FALSE);
  if (l->reallyQuit)
    abutMsg_destroy(l->reallyQuit, FALSE);
  MAGIC_UNSET(l);
  wms_free(l);
}


static GobanOut  gridPressed(void *packet, int loc)  {
  Local  *l = packet;
  GoStone  moveColor;
  bool  timeLeft;

  assert(MAGIC(l));
  l->modified = TRUE;
  moveColor = goGame_whoseMove(l->game);
  if (l->game->state <= goGameState_dispute)  {
    readGobanComments(l);
    sgf_addNode(l->moves);
    if (l->game->state == goGameState_dispute)  {
      addDisputedTriangles(l->game, l->moves);
    }
    timeLeft = goban_stopTimer(l->goban);
    if (!timeLeft)  {
      l->game->state = goGameState_done;
      recordTimeLoss(l);
    } else  {
      sgf_move(l->moves, moveColor, goBoard_loc2Sgf(l->game->board, loc));
      goGame_move(l->game, moveColor, loc, &l->goban->timers[moveColor]);
      addTimeInfoToSgf(l, moveColor);
      goban_startTimer(l->goban, goStone_opponent(moveColor));
    }
  } else if (l->game->state == goGameState_selectDead)  {
    goGame_markDead(l->game, loc);
  } else if (l->game->state == goGameState_selectDisputed)  {
    l->endGame = l->moves->active;
    goGame_dispute(l->game, loc);
    /*
     * We briefly change to "variant" insert mode so that multiple disputes
     *   will be seen in the order they were made.  Then we switch back to
     *   "main" insert mode so that undos and ends of disputes will be
     *   the main variation.
     */
    l->moves->mode = sgfInsert_variant;
    sgf_addNode(l->moves);
    addDisputedTriangles(l->game, l->moves);
    sgf_setWhoseMove(l->moves, l->game->setWhoseMoveColor);
    l->moves->mode = sgfInsert_main;
  }
  writeGobanComments(l);
  return(gobanOut_draw);
}


/*
 * This transfers comments from the SGF move chain to the goban.
 */
static void  writeGobanComments(Local *l)  {
  SgfElem  *comElem;

  assert(MAGIC(l));
  comElem = sgfElem_findTypeInNode(l->moves->active, sgfType_comment);
  if (comElem)  {
    if (strcmp(str_chars(comElem->sVal), goban_getComments(l->goban)))
      goban_setComments(l->goban, str_chars(comElem->sVal));
  } else  {
    if (l->lastComment)
      goban_setComments(l->goban, "");
  }
  l->lastComment = comElem;
}


/*
 * This transfers comments from the goban to the SGF move chain.
 */
static void  readGobanComments(Local *l)  {
  const char  *comm;

  comm = goban_getComments(l->goban);
  if (comm[0])  {
    if (l->lastComment)  {
      if (!strcmp(comm, str_chars(l->lastComment->sVal)))
	return;
    }
    l->modified = TRUE;
    l->moves->mode = sgfInsert_inline;
    sgf_comment(l->moves, comm);
    l->moves->mode = sgfInsert_main;
    l->lastComment = l->moves->active;
  } else if (l->lastComment) {
    l->modified = TRUE;
    sgfElem_snip(l->lastComment, l->moves);
    l->lastComment = NULL;
  }
}

  
static GobanOut  quitPressed(void *packet)  {
  Local  *l = packet;
  AbutMsgOpt  buttons[2];

  assert(MAGIC(l));
  if (l->modified)  {
    buttons[0].name = msg_noCancel;
    buttons[0].callback = dontQuit;
    buttons[0].packet = l;
    buttons[0].keyEq = NULL;
    buttons[1].name = msg_yesQuit;
    buttons[1].callback = reallyQuit;
    buttons[1].packet = l;
    buttons[1].keyEq = NULL;
    if (l->reallyQuit)
      abutMsg_destroy(l->reallyQuit, FALSE);
    l->reallyQuit = abutMsg_winOptCreate(l->cg->abut, "Cgoban Warning",
					 msg_reallyQuitGame,
					 quitWinDead, l, 2, buttons);
  } else
    local_destroy(l);
  return(gobanOut_noDraw);
}


static GobanOut  passPressed(void *packet)  {
  Local  *l = packet;
  GoGameState  oldState = l->game->state;
  GoStone  moveColor;
  bool  timeLeft;

  assert(MAGIC(l));
  readGobanComments(l);
  moveColor = goGame_whoseMove(l->game);
  sgf_addNode(l->moves);
  if (oldState == goGameState_dispute)
    addDisputedTriangles(l->game, l->moves);
  sgf_pass(l->moves, moveColor);
  addTimeInfoToSgf(l, moveColor);
  timeLeft = goban_stopTimer(l->goban);
  if (!timeLeft)  {
    l->game->state = goGameState_done;
    recordTimeLoss(l);
  } else if (goGame_pass(l->game, moveColor, &l->goban->timers[moveColor]))  {
    /*
     * The game state has changed.
     */
    if (oldState == goGameState_dispute)  {
      assert(MAGIC(l->endGame));
      l->moves->active = l->endGame;
      l->endGame = NULL;
      if (l->game->disputeAlive)
	goban_message(l->goban, msg_disputeOverAlive);
      else
	goban_message(l->goban, msg_disputeOverDead);
    } else  {
      assert(oldState == goGameState_play);
      if ((l->game->rules == goRules_japanese) ||
	  (l->game->rules == goRules_tibetan))
	goban_message(l->goban, msg_localJapRemDead);
      else
	goban_message(l->goban, msg_localChiRemDead);
    }
  } else  {
    goban_startTimer(l->goban, goStone_opponent(moveColor));
  }
  writeGobanComments(l);
  return(gobanOut_draw);
}


static GobanOut  rewPressed(void *packet)  {
  Local  *l = packet;

  assert(MAGIC(l));
  readGobanComments(l);
  goban_noMessage(l->goban);
  goban_stopTimer(l->goban);
  if (l->game->state == goGameState_dispute)
    goGame_moveTo(l->game, l->game->setWhoseMoveNum);
  else
    goGame_moveTo(l->game, 0);
  sgf_setActiveNodeNumber(l->moves, l->game->moveNum);
  setTimers(l);
  writeGobanComments(l);
  return(gobanOut_draw);
}


static GobanOut  backPressed(void *packet)  {
  Local  *l = packet;

  assert(MAGIC(l));
  readGobanComments(l);
  goban_noMessage(l->goban);
  goban_stopTimer(l->goban);
  if (l->game->state != goGameState_selectDead)  {
    goGame_moveTo(l->game, l->game->moveNum - 1);
  } else  {
    /* We must be in the endgame.  Let's continue the game now instead! */
    goGame_resume(l->game);
  }
  sgf_setActiveNodeNumber(l->moves, l->game->moveNum);
  setTimers(l);
  writeGobanComments(l);
  return(gobanOut_draw);
}


static GobanOut  fwdPressed(void *packet)  {
  Local  *l = packet;

  assert(MAGIC(l));
  readGobanComments(l);
  goban_noMessage(l->goban);
  goban_stopTimer(l->goban);
  goGame_moveTo(l->game, l->game->moveNum + 1);
  sgf_setActiveNodeNumber(l->moves, l->game->moveNum);
  setTimers(l);
  writeGobanComments(l);
  return(gobanOut_draw);
}


static GobanOut  ffPressed(void *packet)  {
  Local  *l = packet;

  assert(MAGIC(l));
  readGobanComments(l);
  goban_noMessage(l->goban);
  goban_stopTimer(l->goban);
  goGame_moveTo(l->game, l->game->maxMoves);
  sgf_setActiveNodeNumber(l->moves, l->game->moveNum);
  setTimers(l);
  writeGobanComments(l);
  return(gobanOut_draw);
}


static GobanOut  donePressed(void *packet)  {
  Local *l = packet;
  int  i;
  Str  *scoreComment, winner, doneMessage, result;
  float  wScore, bScore;
  SgfElem  *resultElem;
  
  assert(MAGIC(l));
  assert(l->game->state == goGameState_selectDead);
  readGobanComments(l);
  goban_stopTimer(l->goban);
  str_init(&winner);
  str_init(&doneMessage);
  str_init(&result);
  goban_noMessage(l->goban);
  goGame_done(l->game);
  sgf_addNode(l->moves);
  /*
   * Add a territory list.
   */
  for (i = 0;  i < goBoard_area(l->game->board);  ++i)  {
    if (((l->game->flags[i] & GOGAMEFLAGS_SEEN) == GOGAMEFLAGS_SEEWHITE) &&
	((goBoard_stone(l->game->board, i) == goStone_empty) ||
	 (l->game->flags[i] & GOGAMEFLAGS_MARKDEAD)))
      sgf_addTerritory(l->moves, goStone_white,
			     goBoard_loc2Sgf(l->game->board, i));
  }
  for (i = 0;  i < goBoard_area(l->game->board);  ++i)  {
    if (((l->game->flags[i] & GOGAMEFLAGS_SEEN) == GOGAMEFLAGS_SEEBLACK) &&
	((goBoard_stone(l->game->board, i) == goStone_empty) ||
	 (l->game->flags[i] & GOGAMEFLAGS_MARKDEAD)))
      sgf_addTerritory(l->moves, goStone_black,
			     goBoard_loc2Sgf(l->game->board, i));
  }
  wScore = goban_score(l->goban, goStone_white);
  bScore = goban_score(l->goban, goStone_black);
  if (wScore > bScore)  {
    str_print(&winner, msg_winnerComment,
	      msg_stoneNames[goStone_white],
	      wScore - bScore);
    str_print(&result, "W+%g", wScore - bScore);
  } else if ((bScore > wScore) || (l->game->rules == goRules_ing))  {
    str_print(&winner, msg_winnerComment,
	      msg_stoneNames[goStone_black],
	      bScore - wScore);
    str_print(&result, "B+%g", bScore - wScore);
  } else  {
    str_print(&winner, msg_jigoComment);
    str_copyChar(&result, '0');
  }
  str_print(&doneMessage, msg_gameIsOver, winner);
  goban_message(l->goban, str_chars(&doneMessage));

  scoreComment = goScore_str(&l->goban->score, l->game,
			     &l->game->time, l->goban->timers);
  sgf_catComment(l->moves, str_chars(scoreComment));
  str_destroy(scoreComment);

  resultElem = sgf_findType(l->moves, sgfType_result);
  if (resultElem)  {
    sgfElem_newString(resultElem, str_chars(&result));
  } else  {
    sgf_setActiveNodeNumber(l->moves, 0);
    l->moves->mode = sgfInsert_inline;
    sgf_result(l->moves, str_chars(&result));
    l->moves->mode = sgfInsert_main;
    sgf_setActiveToEnd(l->moves);
  }
  str_deinit(&winner);
  str_deinit(&doneMessage);
  str_deinit(&result);
  writeGobanComments(l);
  return(gobanOut_draw);
}


static GobanOut  disputePressed(void *packet)  {
  Local *l = packet;

  assert(MAGIC(l));
  goGame_selectDisputed(l->game);
  goban_message(l->goban, msg_selectDisputedMsg);
  return(gobanOut_draw);
}


static GobanOut  savePressed(void *packet)  {
  Local  *l = packet;

  assert(MAGIC(l));
  if (l->fsel)
    abutFsel_destroy(l->fsel, FALSE);
  l->fsel = abutFsel_create(l->cg->abut, saveFile, l, "CGoban",
			    msg_saveGameName,
			    clp_getStr(l->cg->clp, "local.sgfName"));
  return(gobanOut_noDraw);
}


static GobanOut  editPressed(void *packet)  {
  Local  *l = packet;

  assert(MAGIC(l));
  editBoard_createSgf(l->cg, l->moves);
  return(gobanOut_noDraw);
}


static void  saveFile(AbutFsel *fsel, void *packet, const char *fname)  {
  Local  *l = packet;
  int error;
  Str str;

  assert(MAGIC(l));
  str_copy(&l->cg->lastDirAccessed, &fsel->pathVal);
  if (fname != NULL)  {
    readGobanComments(l);
    l->modified = FALSE;
    clp_setStr(l->cg->clp, "local.sgfName", butTextin_get(fsel->in));
    if (!sgf_writeFile(l->moves, fname, &error)) {
      str_init(&str);
      str_print(&str, "Error saving file \"%s\": %s",
		fname, strerror(errno));
      cgoban_createMsgWindow(l->cg, "Cgoban Error", str_chars(&str));
      str_deinit(&str);
    }
  }
  l->fsel = NULL;
}


static void  addDisputedTriangles(GoGame *game, Sgf *sgf)  {
  int  i;

  for (i = 0;  i < goBoard_area(game->board);  ++i)  {
    if (game->flags[i] & GOGAMEFLAGS_DISPUTED)
      sgf_addTriangle(sgf, goBoard_loc2Sgf(game->board, i));
  }
}


static void  addTimeInfoToSgf(Local *l, GoStone who)  {
  if (l->game->time.type != goTime_none)  {
    sgf_timeLeft(l->moves, who,
		 l->goban->timers[who].timeLeft +
		 (l->goban->timers[who].usLeft > 0));
    if ((l->game->time.type == goTime_ing) ||
	((l->game->time.type == goTime_canadian) &&
	 l->goban->timers[who].aux))  {
      sgf_stonesLeft(l->moves, who, l->goban->timers[who].aux);
    }
  }
}


static void  recordTimeLoss(Local *l)  {
  Str  timeMsg;
  GoStone  loser = l->game->whoseMove;

  /* I don't think that this code is needed any more.
   * sgf_timeLeft(l->moves, l->game->timeLoss, -1);
   * if ((l->game->time.type == goTime_ing) ||
   *     ((l->game->time.type == goTime_canadian) &&
   *      l->goban->timers[loser].aux))  {
   *   sgf_stonesLeft(l->moves, loser,
   *                  l->goban->timers[loser].aux +
   *                  (l->game->time.type == goTime_canadian));
   * }
   */
  str_init(&timeMsg);
  str_print(&timeMsg, msg_timeLoss,
	    msg_stoneNames[loser],
	    msg_stoneNames[goStone_opponent(loser)]);
  goban_message(l->goban, str_chars(&timeMsg));
  str_catChar(&timeMsg, '\n');
  sgf_catComment(l->moves, str_chars(&timeMsg));
  str_print(&timeMsg, "%c+T",
	    (int)goStone_char(goStone_opponent(loser)));
  sgf_setActiveNodeNumber(l->moves, 0);
  l->moves->mode = sgfInsert_inline;
  sgf_result(l->moves, str_chars(&timeMsg));
  l->moves->mode = sgfInsert_main;
  sgf_setActiveToEnd(l->moves);
  str_deinit(&timeMsg);
}


static bool  rewOk(void *packet)  {
  Local  *l = packet;
  GoGame  *game;

  assert(MAGIC(l));
  game = l->game;
  switch(game->state)  {
  case(goGameState_selectDead):
  case(goGameState_selectDisputed):
    return(FALSE);
    break;
  case(goGameState_dispute):
    return(game->moveNum > game->setWhoseMoveNum);
    break;
  default:
    return(game->moveNum > 0);
    break;
  }
}


static bool  backOk(void *packet)  {
  Local  *l = packet;
  GoGame  *game;

  assert(MAGIC(l));
  game = l->game;
  switch(game->state)  {
  case(goGameState_selectDead):
    return(TRUE);
    break;
  case(goGameState_selectDisputed):
    return(FALSE);
    break;
  case(goGameState_dispute):
    return(game->moveNum > game->setWhoseMoveNum);
    break;
  default:
    return(game->moveNum > 0);
    break;
  }
}


static bool  fwdOk(void *packet)  {
  Local  *l = packet;
  GoGame  *game;

  assert(MAGIC(l));
  game = l->game;
  switch(game->state)  {
  case(goGameState_selectDead):
  case(goGameState_selectDisputed):
    return(FALSE);
    break;
  default:
    return(game->moveNum < game->maxMoves);
    break;
  }
}


static void  gobanDestroyed(void *packet)  {
  Local  *l = packet;

  assert(MAGIC(l));
  l->goban = NULL;
  local_destroy(l);
}


static ButOut  reallyQuit(But *but)  {
  local_destroy(but_packet(but));
  return(0);
}


static ButOut  dontQuit(But *but)  {
  Local  *l = but_packet(but);

  assert(MAGIC(l));
  abutMsg_destroy(l->reallyQuit, FALSE);
  l->reallyQuit = NULL;
  return(0);
}


static ButOut  quitWinDead(void *packet)  {
  Local  *l = packet;

  assert(MAGIC(l));
  l->reallyQuit = NULL;
  return(0);
}


static void  setTimers(Local *l)  {
  GoStone  stone;
  const GoTimer  *timer;

  assert(MAGIC(l));
  goStoneIter(stone)  {
    timer = goGame_getTimer(l->game, stone);
    if (timer == NULL)
      goTimer_init(&l->goban->timers[stone], &l->game->time);
    else
      l->goban->timers[stone] = *timer;
  }
  goban_updateTimeReadouts(l->goban);
  goban_startTimer(l->goban, goGame_whoseMove(l->game));
}
