/*
 * $Source: /cvsroot/cgoban1/cgoban1/src/sgfPlay.c,v $
 * $Revision: 1.2 $
 * $Author: wmshub $
 * $Date: 2002/05/31 23:40:54 $
 *
 * src/sgfPlay.c, part of Complete Goban (game program)
 * Copyright © 1995,2002 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#include <ctype.h>
#include <wms.h>
#include <wms/rnd.h>
#include <wms/str.h>
#include <but/but.h>
#include "cgoban.h"
#include "goPic.h"
#include "sgf.h"
#include "sgfPlay.h"
#include "msg.h"


/**********************************************************************
 * Forward declarations
 **********************************************************************/
static void  processMoves(SgfElem *me, GoGame *game, int *handicap,
			  bool *handicapPlayed, Cgoban *cg);
static void  processNodeMarks(SgfElem *me, GoGame *game, GoPic *pic,
			      bool *changed);
static void  eraseObsoleteMarks(GoPic *pic, GoGame *game, bool *changed);


/**********************************************************************
 * Functions
 **********************************************************************/
int  sgf_play(Sgf *mc, GoGame *game, GoPic *pic, int numNodes,
	      SgfElem *terminator)  {
  int  handicap = 0, i, currentNode = 0;
  SgfElem  *me;
  bool  handicapPlayed = FALSE;
  SgfElem  *nodeStart = mc->top.activeChild;
  bool  *changed;

  goGame_moveTo(game, 0);
  if (numNodes >= 0)
    ++numNodes;
  if (terminator != NULL)  {
    terminator = terminator->activeChild;
  }
  for (me = mc->top.activeChild;
       (me != terminator) && numNodes;  me = me->activeChild)  {
    assert(MAGIC(me));
    mc->active = me;
    if (me->type == sgfType_node)  {
      assert((me->childH == NULL) || (me->childH != me->childT) ||
	     (me->activeChild == me->childH));
      --numNodes;
      ++currentNode;
      if (numNodes)  {
	nodeStart = me->activeChild;
	/*
	 * We take out the last move at each node to make sure that
	 *   you don't see that silly ring forever.  If there's another
	 *   move coming then this will get undone when we move.
	 */
	goGame_noLastMove(game);
      } else  {
	mc->active = me->parent;
      }
    } else  {
      assert(me->parent->childH == me->parent->childT);
      assert(me->parent->activeChild == me->parent->childH);
      if (pic == NULL)
	processMoves(me, game, &handicap, &handicapPlayed, NULL);
      else
	processMoves(me, game, &handicap, &handicapPlayed, pic->cg);
    }
  }

  /*
   * Now set the clock to match the time that we had left.

  for (i = game->moveNum - 1;  i >= 0;  --i)  {
    if (game->moves[i].color == goStone_white)
      game->timers[goStone_white] = game->moves[i].time;
  }
  for (i = game->moveNum - 1;  i >= 0;  --i)  {
    if (game->moves[i].color == goStone_black)
      game->timers[goStone_black] = game->moves[i].time;
  }
   */  
  /*
   * If there are handicap stones that were added automatically and aren't
   *   in the movechain, then add them now.
   * This is really stupid.  There is no way that adding handicap stones
   *   should be in the sgf play code!  :-(
   */
  if (!handicapPlayed && handicap && !game->passive)  {
    for (i = 0;  i < goBoard_area(game->board);  ++i)  {
      if (goBoard_stone(game->board, i) == goStone_black)
	sgf_addStone(mc, goStone_black, goBoard_loc2Sgf(game->board, i));
    }
  }

  /*
   * If we were given a GoPic, then add marks to it now.
   */
  if (pic)  {
    changed = wms_malloc(goBoard_area(game->board) * sizeof(bool));
    memset(changed, 0, goBoard_area(game->board) * sizeof(bool));
    while (nodeStart)  {
      if (nodeStart->type == sgfType_node)
	break;
      processNodeMarks(nodeStart, game, pic, changed);
      nodeStart = nodeStart->activeChild;
    }
    eraseObsoleteMarks(pic, game, changed);
    wms_free(changed);
  }

  if (me)
    --currentNode;
  return(currentNode);
}


static void  processMoves(SgfElem *me, GoGame *game, int *handicap,
			  bool *handicapPlayed, Cgoban *cg)  {
  int  i;

  switch(me->type)  {
  case sgfType_node:
    assert(0);
    break;
  case sgfType_handicap:
    *handicap = me->iVal;
    break;
  case sgfType_whoseMove:
    game->setWhoseMoveNum = game->moveNum;
    game->setWhoseMoveColor = me->gVal;
    game->whoseMove = me->gVal;
    break;
  case sgfType_move:
    *handicapPlayed = TRUE;
    i = goBoard_sgf2Loc(game->board, me->lVal);
    if (!*handicap || (goBoard_stone(game->board, i) != goStone_black))  {
      assert(game->passive || (me->gVal == game->whoseMove));
      if (!goGame_isLegal(game, me->gVal, i))  {
	Str  errmsg;
	char  moveStr[5];

	str_init(&errmsg);
	goBoard_loc2Str(game->board, i, moveStr),
	str_print(&errmsg, msg_badMoveInSgf, moveStr,
		  game->moveNum);
	if (cg != NULL)
	  cgoban_createMsgWindow(cg, "CGoban: Error in SGF file",
				 str_chars(&errmsg));
	str_deinit(&errmsg);
      } else  {
	goGame_move(game, me->gVal, i, NULL);
      }
    }
    if (*handicap)
      --*handicap;
    break;
  case sgfType_pass:
    if (game->state != goGameState_selectDead)  {
      /*
       * IGS sends out games with more than two passes at the end of the
       *   game.  If you've already gone to non-play state and you get
       *   passes, then ignore them.
       */
      goGame_pass(game, me->gVal, NULL);
    }
    break;
  case sgfType_setBoard:
    i = goBoard_sgf2Loc(game->board, me->lVal);
    if ((me->gVal == goStone_empty) &&
	((game->state == goGameState_selectDead) ||
	 game->forcePlay))  {
      if (!(game->flags[i] & GOGAMEFLAGS_MARKDEAD))  {
	goGame_markDead(game, i);
      }
    } else  {
      if (goBoard_stone(game->board, i) != me->gVal)  {
	goGame_setBoard(game, me->gVal, i);
      }
      assert(goBoard_stone(game->board, i) == me->gVal);
    }
    break;
  case sgfType_timeLeft:
    if (me->iVal < 0)  {
      /*
       * This is marking a time loss.  Fake up the game structure to
       *   make it clear that this is what happened.
       */
      game->maxMoves = ++game->moveNum;
    }
    game->moves[game->moveNum - 1].time.timeLeft = me->iVal;
    break;
  case sgfType_stonesLeft:
    game->moves[game->moveNum - 1].time.aux = me->iVal;
    break;
  default:
    break;
  }
}	


static void  processNodeMarks(SgfElem *me, GoGame *game, GoPic *pic,
			      bool *changed)  {
  int  loc, i;
  GoMarkType  mark;
  bool  err;

  assert(MAGIC(me));
  switch(me->type)  {
  case sgfType_territory:
    loc = goBoard_sgf2Loc(game->board, me->lVal);
    changed[loc] = TRUE;
    mark = goMarkType_stone2sm(me->gVal);
    if (goStone_isStone(goBoard_stone(game->board, loc)))  {
      if (!grid_grey(pic->boardButs[loc]))  {
	grid_setStone(pic->boardButs[loc], goBoard_stone(game->board, loc),
		      TRUE);
      }
    }
    if (grid_markType(pic->boardButs[loc]) != mark)  {
      grid_setMark(pic->boardButs[loc], mark, 0);
    }
    break;
  case sgfType_triangle:
  case sgfType_circle:
  case sgfType_square:
  case sgfType_mark:
    if (me->type == sgfType_triangle)
      mark = goMark_triangle;
    else if (me->type == sgfType_circle)
      mark = goMark_circle;
    else if (me->type == sgfType_mark)
      mark = goMark_x;
    else  /* me->type == sgfType_square */
      mark = goMark_square;
    loc = goBoard_sgf2Loc(game->board, me->lVal);
    changed[loc] = TRUE;
    if (grid_grey(pic->boardButs[loc]))  {
      grid_setStone(pic->boardButs[loc], goBoard_stone(game->board, loc),
		    FALSE);
    }
    if (grid_markType(pic->boardButs[loc]) != mark)  {
      grid_setMark(pic->boardButs[loc], mark, 0);
    }
    break;
  case sgfType_label:
    loc = goBoard_sgf2Loc(game->board, me->lVal);
    changed[loc] = TRUE;
    /* See if it is a numeric label. */
    i = wms_atoi(str_chars(me->sVal), &err);
    if (err)
      grid_setMark(pic->boardButs[loc], goMark_letter, str_chars(me->sVal)[0]);
    else
      grid_setMark(pic->boardButs[loc], goMark_number, i);
    break;
  default:
    break;
  }
}


static void  eraseObsoleteMarks(GoPic *pic, GoGame *game, bool *changed)  {
  int  loc;

  for (loc = 0;  loc < goBoard_area(game->board);  ++loc)  {
    if (!changed[loc] && pic->boardButs[loc])  {
      if (grid_grey(pic->boardButs[loc]) ||
	  (grid_markType(pic->boardButs[loc]) != goMark_none))  {
	grid_setStone(pic->boardButs[loc], goBoard_stone(game->board, loc),
		      FALSE);
	grid_setMark(pic->boardButs[loc], goMark_none, 0);
      }
    }
  }
}

