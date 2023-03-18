/*
 * $Source: /cvsroot/cgoban1/cgoban1/src/gmp/play.c,v $
 * $Revision: 1.2 $
 * $Date: 2000/02/09 06:50:05 $
 *
 * Part of Complete Goban (game program)
 * Copyright © 1995-1996,2000 William Shubert
 * See "configure.h.in" for more copyright information.
 *
 * Most of this file was mercilessly cut and pasted from local.c.  I probably
 *   should have found a way to make the local.c code do double-duty, but hey
 *   I'm lazy and it's late at night and I want to get this done.
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
#include <sys/wait.h>
#include <but/textin.h>
#include "../cgoban.h"
#include "../goban.h"
#include "../msg.h"
#include "../goGame.h"
#include "../goPic.h"
#include "../cgbuts.h"
#include "../sgf.h"
#include "../sgfIn.h"
#include "../sgfOut.h"
#include "../sgfPlay.h"
#include "engine.h"
#include "../editBoard.h"
#if  _GMP_PLAY_H_
#error  LEVELIZATION ERROR
#endif
#include "play.h"


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
static void  writeGobanComments(GmpPlay *l), readGobanComments(GmpPlay *l);
static void  addDisputedTriangles(GoGame *game, Sgf *sgf);
static void  addTimeInfoToSgf(GmpPlay *l, GoStone color);
static void  recordTimeLoss(GmpPlay *l);
static bool  rewOk(void *packet), backOk(void *packet), fwdOk(void *packet);
static void  gobanDestroyed(void *packet);
static GmpPlay  *gmpPlay_createSgf(Cgoban *cg, Sgf *mc,
				   int inFiles[2], int outFiles[2],
				   int pids[2]);
static ButOut  recvNewGame(GmpEngine *ge, void *packet, int size, int handicap,
			 float komi, bool chineseRules, bool iAmWhite);
static ButOut  recvMove(GmpEngine *ge, void *packet, GoStone color,
		      int x, int y);
static ButOut  recvUndo(GmpEngine *ge, void *packet, int numUndos);
static ButOut  recvError(GmpEngine *ge, void *packet, const char *errStr);


/**********************************************************************
 * Global variables
 **********************************************************************/
static const GobanActions  gmpPlay_actions = {
  gridPressed, quitPressed, passPressed, rewPressed, backPressed,
  fwdPressed, ffPressed, donePressed, disputePressed, savePressed,
  editPressed, NULL, /* gameInfo */
  &help_gmpBoard,
  gobanDestroyed,
  rewOk, backOk, fwdOk, fwdOk};


/**********************************************************************
 * Functions
 **********************************************************************/
GmpPlay  *gmpPlay_create(Cgoban *cg,
			 int inFiles[2], int outFiles[2],
			 int pids[2], GoRules rules,
			 int size, int hcap, float komi, const char *white,
			 const char *black, GoTimeType timeType, int mainTime,
			 int byTime, int auxTime)  {
  Sgf  *mc;
  Str  tmp;
  GoTime  time;

  mc = sgf_create();
  str_init(&tmp);
  if (rules != goRules_japanese)  {
    /* 
     * GMP says that chinese rules let you play handicap stones anywhere.
     * I call that New Zealand rules instead.
     */
    rules = goRules_nz;
  }
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
  return(gmpPlay_createSgf(cg, mc, inFiles, outFiles, pids));
}


static GmpPlay  *gmpPlay_createSgf(Cgoban *cg, Sgf *mc,
				   int inFiles[2], int outFiles[2],
				   int pids[2])  {
  static const GmpActions  gmpCallbacks = {
    recvNewGame, recvMove, recvUndo, recvError};
  GmpPlay  *l;
  SgfElem  *me;
  GoRules  rules;
  int  size, hcap;
  float  komi;
  GoTime  time;
  const char  *title;
  GoStone  s;

  assert(MAGIC(cg));
  l = wms_malloc(sizeof(GmpPlay));
  MAGIC_SET(l);
  l->cg = cg;
  goStoneIter(s)  {
    l->players[s].in = inFiles[s];
    l->players[s].out = outFiles[s];
    l->players[s].pid = pids[s];
    l->players[s].ready = (inFiles[s] < 0);
  }
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
  me = sgf_findFirstType(mc, sgfType_time);
  if (me)  {
    goTime_parseDescribeChars(&time, str_chars(me->sVal));
  } else  {
    time.type = goTime_none;
  }
  l->game = goGame_create(size, rules, hcap, komi, &time, FALSE);
  sgf_play(mc, l->game, NULL, -1, NULL);
      
  me = sgf_findFirstType(mc, sgfType_title);
  if (me)  {
    title = str_chars(me->sVal);
  } else  {
    title = msg_noTitle;
  }

  l->goban = goban_create(cg, &gmpPlay_actions, l, l->game, "local",
			  title);
  l->goban->pic->allowedMoves |= goPicMove_noWhite | goPicMove_noBlack;
  goPic_update(l->goban->pic);
  goban_message(l->goban, msg_waitingForGame);
  goban_startTimer(l->goban, goGame_whoseMove(l->game));
  l->goban->iDec1 = grid_create(&cg->cgbuts, NULL, NULL, l->goban->iWin, 2,
				BUT_DRAWABLE, 0);
  grid_setStone(l->goban->iDec1, goStone_white, FALSE);
  l->goban->iDec2 = grid_create(&cg->cgbuts, NULL, NULL, l->goban->iWin, 2,
				BUT_DRAWABLE, 0);
  grid_setStone(l->goban->iDec2, goStone_black, FALSE);
  l->lastComment = NULL;
  assert((hcap == 0) || ((hcap >= 2) && (hcap <= 27)));
  
  goStoneIter(s)  {
    if (l->players[s].in >= 0)  {
      assert(l->players[s].out >= 0);
      l->players[s].engine = gmpEngine_create(l->cg,
					      l->players[s].in,
					      l->players[s].out, &gmpCallbacks,
					      l);
      gmpEngine_startGame(l->players[s].engine, size, hcap, komi,
			  (rules != goRules_japanese), (s != goStone_white));
    }
  }

  l->fsel = NULL;
  return(l);
}


void  gmpPlay_destroy(GmpPlay *l)  {
  GoStone  s;
  int  dummy;

  assert(MAGIC(l));
  if (l->goban)
    goban_destroy(l->goban, FALSE);
  if (l->game)
    goGame_destroy(l->game);
  if (l->fsel)
    abutFsel_destroy(l->fsel, FALSE);
  goStoneIter(s)  {
    if (l->players[s].in >= 0)  {
      if (l->players[s].pid >= 0)  {
	kill(l->players[s].pid, SIGTERM);
	waitpid(l->players[s].pid, &dummy, 0);
      }
      close(l->players[s].in);
      if (l->players[s].out != l->players[s].in) {
	close(l->players[s].out);
      }
      l->players[s].in = -1;
      l->players[s].out = -1;
      l->players[s].pid = -1;
      gmpEngine_destroy(l->players[s].engine);
    }
  }
  MAGIC_UNSET(l);
  wms_free(l);
}


static GobanOut  gridPressed(void *packet, int loc)  {
  GmpPlay  *l = packet;
  SgfElem  *startActive;
  GoStone  moveColor;
  bool  timeLeft;

  assert(MAGIC(l));
  moveColor = goGame_whoseMove(l->game);
  if ((l->game->moveNum < l->game->maxMoves) &&
      (l->players[goStone_opponent(moveColor)].in >= 0))  {
    gmpEngine_sendUndo(l->players[goStone_opponent(moveColor)].engine,
		       l->game->maxMoves - l->game->moveNum);
  }
  if (l->game->state <= goGameState_dispute)  {
    readGobanComments(l);
    sgf_addNode(l->moves);
    startActive = l->moves->active;
    if (l->game->state == goGameState_dispute)  {
      addDisputedTriangles(l->game, l->moves);
    }
    timeLeft = goban_stopTimer(l->goban);
    if (!timeLeft)  {
      recordTimeLoss(l);
    } else  {
      sgf_move(l->moves, moveColor, goBoard_loc2Sgf(l->game->board, loc));
      if (l->players[goStone_opponent(moveColor)].in >= 0)  {
	/* Send the move out the GMP. */
	gmpEngine_sendMove(l->players[goStone_opponent(moveColor)].engine,
			   goBoard_loc2X(l->game->board, loc),
			   goBoard_loc2Y(l->game->board, loc));
      }
      goGame_move(l->game, moveColor, loc, &l->goban->timers[moveColor]);
      addTimeInfoToSgf(l, moveColor);
      goban_startTimer(l->goban, goGame_whoseMove(l->game));
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
static void  writeGobanComments(GmpPlay *l)  {
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
static void  readGobanComments(GmpPlay *l)  {
  const char  *comm;

  comm = goban_getComments(l->goban);
  if (comm[0])  {
    l->moves->mode = sgfInsert_inline;
    sgf_comment(l->moves, comm);
    l->moves->mode = sgfInsert_main;
    l->lastComment = l->moves->active;
  } else if (l->lastComment) {
    sgfElem_snip(l->lastComment, l->moves);
    l->lastComment = NULL;
  }
}

  
static GobanOut  quitPressed(void *packet)  {
  GmpPlay  *l = packet;

  assert(MAGIC(l));
  gmpPlay_destroy(l);
  return(gobanOut_noDraw);
}


static GobanOut  passPressed(void *packet)  {
  GmpPlay  *l = packet;
  GoGameState  oldState = l->game->state;
  SgfElem  *startActive;
  GoStone  moveColor;
  bool  timeLeft;

  assert(MAGIC(l));
  readGobanComments(l);
  moveColor = goGame_whoseMove(l->game);
  if ((l->game->moveNum < l->game->maxMoves) &&
      (l->players[goStone_opponent(moveColor)].in >= 0))  {
    gmpEngine_sendUndo(l->players[goStone_opponent(moveColor)].engine,
		       l->game->maxMoves - l->game->moveNum);
  }
  sgf_addNode(l->moves);
  startActive = l->moves->active;
  if (oldState == goGameState_dispute)
    addDisputedTriangles(l->game, l->moves);
  sgf_pass(l->moves, moveColor);
  if (l->players[goStone_opponent(moveColor)].in >= 0)  {
    /* Send the move out the GMP. */
    gmpEngine_sendPass(l->players[goStone_opponent(moveColor)].engine);
  }
  addTimeInfoToSgf(l, moveColor);
  
  timeLeft = goban_stopTimer(l->goban);
  if (!timeLeft)
    recordTimeLoss(l);
  else  {
    if (goGame_pass(l->game, moveColor, &l->goban->timers[moveColor]))  {
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
	/* Stop the GMP engines! */
	if (l->players[goStone_white].in >= 0)
	  gmpEngine_stop(l->players[goStone_white].engine);
	if (l->players[goStone_black].in >= 0)
	  gmpEngine_stop(l->players[goStone_black].engine);
      }
    }
  }
  if (l->game->state <= goGameState_dispute)  {
    goban_startTimer(l->goban, goGame_whoseMove(l->game));
  }
  writeGobanComments(l);
  return(gobanOut_draw);
}


static GobanOut  rewPressed(void *packet)  {
  GmpPlay  *l = packet;

  assert(MAGIC(l));
  readGobanComments(l);
  goban_noMessage(l->goban);
  if (l->game->state == goGameState_dispute)
    goGame_moveTo(l->game, l->game->setWhoseMoveNum);
  else
    goGame_moveTo(l->game, 0);
  sgf_setActiveNodeNumber(l->moves, l->game->moveNum);
  writeGobanComments(l);
  return(gobanOut_draw);
}


static GobanOut  backPressed(void *packet)  {
  GmpPlay  *l = packet;

  assert(MAGIC(l));
  readGobanComments(l);
  goban_noMessage(l->goban);
  if (l->game->state != goGameState_selectDead)  {
    goGame_moveTo(l->game, l->game->moveNum - 1);
  } else  {
    /* We must be in the endgame.  Let's continue the game now instead! */
    goGame_resume(l->game);
  }
  sgf_setActiveNodeNumber(l->moves, l->game->moveNum);
  writeGobanComments(l);
  return(gobanOut_draw);
}


static GobanOut  fwdPressed(void *packet)  {
  GmpPlay  *l = packet;

  assert(MAGIC(l));
  readGobanComments(l);
  goban_noMessage(l->goban);
  goGame_moveTo(l->game, l->game->moveNum + 1);
  sgf_setActiveNodeNumber(l->moves, l->game->moveNum);
  writeGobanComments(l);
  return(gobanOut_draw);
}


static GobanOut  ffPressed(void *packet)  {
  GmpPlay  *l = packet;

  assert(MAGIC(l));
  readGobanComments(l);
  goban_noMessage(l->goban);
  goGame_moveTo(l->game, l->game->maxMoves);
  sgf_setActiveNodeNumber(l->moves, l->game->moveNum);
  writeGobanComments(l);
  return(gobanOut_draw);
}


static GobanOut  editPressed(void *packet)  {
  GmpPlay  *play = packet;

  assert(MAGIC(play));
  editBoard_createSgf(play->cg, play->moves);
  return(gobanOut_noDraw);
}


static GobanOut  donePressed(void *packet)  {
  GmpPlay *l = packet;
  int  i;
  Str  *scoreComment, winner, doneMessage, result;
  float  wScore, bScore;
  SgfElem  *resultElem;
  
  assert(MAGIC(l));
  assert(l->game->state == goGameState_selectDead);
  readGobanComments(l);
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
  GmpPlay *l = packet;

  assert(MAGIC(l));
  goGame_selectDisputed(l->game);
  goban_message(l->goban, msg_selectDisputedMsg);
  return(gobanOut_draw);
}


static GobanOut  savePressed(void *packet)  {
  GmpPlay  *l = packet;

  assert(MAGIC(l));
  if (l->fsel)
    abutFsel_destroy(l->fsel, FALSE);
  l->fsel = abutFsel_create(l->cg->abut, saveFile, l, "CGoban",
			    msg_saveGameName,
			    clp_getStr(l->cg->clp, "local.sgfName"));
  return(gobanOut_noDraw);
}


static void  saveFile(AbutFsel *fsel, void *packet, const char *fname)  {
  GmpPlay  *l = packet;

  assert(MAGIC(l));
  str_copy(&l->cg->lastDirAccessed, &fsel->pathVal);
  if (fname != NULL)  {
    clp_setStr(l->cg->clp, "local.sgfName", butTextin_get(fsel->in));
    sgf_writeFile(l->moves, fname, NULL);
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


static void  addTimeInfoToSgf(GmpPlay *l, GoStone who)  {
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


static void  recordTimeLoss(GmpPlay *l)  {
  Str  timeMsg;
  GoStone  loser = l->game->whoseMove;

  sgf_timeLeft(l->moves, loser, -1);
  if ((l->game->time.type == goTime_ing) ||
      ((l->game->time.type == goTime_canadian) &&
       l->goban->timers[loser].aux))  {
    sgf_stonesLeft(l->moves, loser,
		   l->goban->timers[loser].aux +
		   (l->game->time.type == goTime_canadian));
  }
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
  GmpPlay  *l = packet;
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
    if (!goStone_isStone(game->whoseMove))
      return(TRUE);
    return((game->moveNum > 0) &&
	   ((l->players[goStone_opponent(game->whoseMove)].in >= 0) ||
	    (game->moveNum < game->maxMoves)));
    break;
  }
}


static bool  backOk(void *packet)  {
  GmpPlay  *l = packet;
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
    if (!goStone_isStone(game->whoseMove))
      return(TRUE);
    return((game->moveNum > 0) &&
	   ((l->players[goStone_opponent(game->whoseMove)].in >= 0) ||
	    (game->moveNum < game->maxMoves)));
    break;
  }
}


static bool  fwdOk(void *packet)  {
  GmpPlay  *l = packet;
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
  GmpPlay  *l = packet;

  assert(MAGIC(l));
  l->goban = NULL;
  gmpPlay_destroy(l);
}


static ButOut  recvNewGame(GmpEngine *ge, void *packet, int size, int handicap,
			   float komi, bool chineseRules, bool iAmWhite)  {
  Str  errStr;
  GoStone  s;
  GmpPlay  *gp = packet;

  assert(MAGIC(gp));
  if (ge == gp->players[goStone_white].engine)
    s = goStone_white;
  else  {
    assert(ge == gp->players[goStone_black].engine);
    s = goStone_black;
  }
  if ((size != gp->game->size) || (handicap != gp->game->handicap) ||
      (chineseRules != (gp->game->rules == goRules_nz)) ||
      (iAmWhite != (s == goStone_black)))  {
    str_init(&errStr);
    str_print(&errStr, msg_badNewGameReq,
	      msg_stoneNames[s]);
    recvError(ge, gp, str_chars(&errStr));
    str_deinit(&errStr);
  } else  {
    gp->players[s].ready = TRUE;
    if (gp->players[goStone_opponent(s)].ready) {
      goban_noMessage(gp->goban);
      if (gp->players[goStone_white].in < 0) {
	gp->goban->pic->allowedMoves &= ~goPicMove_noWhite;
      }
      if (gp->players[goStone_black].in < 0) {
	gp->goban->pic->allowedMoves &= ~goPicMove_noBlack;
      }
      goPic_update(gp->goban->pic);
    }
  }
  return(0);
}


static ButOut  recvMove(GmpEngine *ge, void *packet, GoStone color,
			int x, int y)  {
  GmpPlay  *gp = packet;
  Str  error;
  char  locStr[10];
  int  loc, oldMoveNum = -1;

  assert(MAGIC(gp));
  assert(color == gp->game->whoseMove);
  loc = goBoard_xy2Loc(gp->game->board, x, y);
  if (gp->game->state != goGameState_play)  {
    recvError(ge, gp, msg_gmpMoveOutsideGame);
  } else  {
    if (gp->game->moveNum < gp->game->maxMoves)  {
      oldMoveNum = gp->game->moveNum;
      goGame_moveTo(gp->game, gp->game->maxMoves);
      sgf_setActiveNodeNumber(gp->moves, gp->game->moveNum);
    }
    if (x >= 0)  {
      if (goGame_isLegal(gp->game, color, loc))  {
	gridPressed(gp, goBoard_xy2Loc(gp->game->board, x, y));
      } else  {
	goBoard_loc2Str(gp->game->board, loc, locStr);
	str_init(&error);
	str_print(&error, msg_gmpBadMove, locStr);
	recvError(ge, gp, str_chars(&error));
	str_deinit(&error);
      }
    } else  {
      /* A pass. */
      passPressed(gp);
    }
    if (oldMoveNum != -1)  {
      goGame_moveTo(gp->game, oldMoveNum);
      sgf_setActiveNodeNumber(gp->moves, gp->game->moveNum);
    }
  }
  goban_update(gp->goban);
  return(0);
}


static ButOut  recvUndo(GmpEngine *ge, void *packet, int numUndos)  {
  Str  undoMessage;
  GoStone  s;
  GmpPlay  *gp = packet;

  assert(MAGIC(gp));
  if (ge == gp->players[goStone_white].engine)
    s = goStone_white;
  else  {
    assert(ge == gp->players[goStone_black].engine);
    s = goStone_black;
  }
  str_init(&undoMessage);
  str_print(&undoMessage, msg_undoRequested,
	    msg_stoneNames[s], numUndos);
  goban_message(gp->goban, str_chars(&undoMessage));
  str_deinit(&undoMessage);
  readGobanComments(gp);
  if (numUndos > gp->game->moveNum)
    numUndos = gp->game->moveNum;
  if (gp->game->moveNum > gp->game->maxMoves - numUndos)  {
    goGame_moveTo(gp->game, gp->game->moveNum - numUndos);
  }
  gp->game->maxMoves -= numUndos;
  sgf_setActiveNodeNumber(gp->moves, gp->game->moveNum);
  writeGobanComments(gp);
  return(0);
}


static ButOut  recvError(GmpEngine *ge, void *packet, const char *errStr)  {
  GmpPlay  *gp = packet;
  GmpPlayInfo  *p;
  int  exitStatus = 0;
  bool  alreadyDead = FALSE;
  const char  *progClp;
  Str  exitMsg;

  assert(MAGIC(gp));
  if (ge == gp->players[goStone_white].engine)  {
    p = &gp->players[goStone_white];
    progClp = "gmp.WProgram";
  } else  {
    assert(ge == gp->players[goStone_black].engine);
    p = &gp->players[goStone_black];
    progClp = "gmp.BProgram";
  }
  if (p->pid >= 0)  {
    alreadyDead = (waitpid(p->pid, &exitStatus, WNOHANG) != 0);
    assert(alreadyDead != -1);
    if (!alreadyDead)  {
      kill(p->pid, SIGKILL);
      waitpid(p->pid, &exitStatus, 0);
    }
  }
  gmpEngine_destroy(p->engine);
  close(p->in);
  if (p->out != p->in)
    close(p->out);
  p->in = -1;
  p->out = -1;
  p->pid = -1;
  if (alreadyDead)  {
    str_init(&exitMsg);
    if (WIFEXITED(exitStatus) &&
	(WEXITSTATUS(exitStatus) == GMP_EXECVEFAILED))  {
      str_print(&exitMsg, msg_gmpCouldntStart,
		clp_getStr(gp->cg->clp, progClp));
    } else if (WIFSIGNALED(exitStatus))  {
      str_print(&exitMsg, msg_gmpProgKilled,
		clp_getStr(gp->cg->clp, progClp),
		WTERMSIG(exitStatus));
    } else  {
      str_print(&exitMsg, msg_gmpProgWhyDead,
		clp_getStr(gp->cg->clp, progClp));
    }
    goban_message(gp->goban, str_chars(&exitMsg));
    str_deinit(&exitMsg);
  } else
    goban_message(gp->goban, errStr);
  return(BUTOUT_ERR);
}
