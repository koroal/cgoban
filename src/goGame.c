/*
 * $Source: /cvsroot/cgoban1/cgoban1/src/goGame.c,v $
 * $Revision: 1.3 $
 * $Date: 2002/05/10 05:30:36 $
 *
 * src/gogame.c, part of Complete Goban (game program)
 * Copyright © 1995-2000 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#include <wms.h>
#include <wms/rnd.h>
#include <wms/str.h>
#include "goBoard.h"
#ifdef  _GOGAME_H_
#error LEVELIZATION ERROR
#endif
#include "goGame.h"


/**********************************************************************
 * Forward Declarations
 **********************************************************************/
static bool  goGame_forcedHandicapPlacement(GoGame *game);
static void  goGame_addHandicapStones(GoGame *game, GoBoard *board);
static GoBoard  *goGame_replay(GoGame *game, int moves, bool separateBoard);
static void  goGame_updateForMove(GoGame *game, GoStone moveColor, int move);
static bool  isDisputeAlive(GoGame *game);
static void  addMoveToMoves(GoGame *game, GoStone moveColor, int move,
			    int moveNum);
static bool  goGame_canMove(GoGame *game, GoStone s, int move);
static bool  goGame_canDispute(GoGame *game, int move);


/**********************************************************************
 * Globals
 **********************************************************************/
const bool  goRules_freeHandicaps[goRules_num] = {
  FALSE, FALSE, TRUE, FALSE, TRUE, TRUE};

const GoRulesKo  goRules_ko[goRules_num] = {
  goRulesKo_super, goRulesKo_japanese, goRulesKo_super, goRulesKo_super,
  goRulesKo_super, goRulesKo_tibetan};

const bool  goRules_suicide[goRules_num] = {
  TRUE, FALSE, TRUE, TRUE, TRUE, FALSE};

const bool  goRules_dispute[goRules_num] = {
  TRUE, FALSE, FALSE, FALSE, FALSE, TRUE};

const bool  goRules_scoreKills[goRules_num] = {
  FALSE, TRUE, FALSE, TRUE, FALSE, TRUE};


/**********************************************************************
 * Functions
 **********************************************************************/
GoGame  *goGame_create(int size, GoRules rules, int handicap, float komi,
		       const GoTime *time, bool passive)  {
  GoGame *game;
  int  i;

  assert((handicap >= 0) && (handicap <= 27));
  game = wms_malloc(sizeof(GoGame));
  MAGIC_SET(game);
  game->size = size;
  game->rules = rules;
  game->passive = passive;
  game->forcePlay = FALSE;
  if (handicap == 1)
    handicap = 0;
  game->handicap = handicap;
  game->komi = komi;
  if (time)  {
    game->time = *time;
  } else  {
    game->time.type = goTime_none;
  }
  game->moveNum = 0;
  game->maxMoves = 0;
  game->passCount = 0;
  game->movesLen = 5 /* 400 */;
  game->moves = wms_malloc(game->movesLen * sizeof(game->moves[0]));
  game->noLastMove = FALSE;

  game->board = goBoard_create(size);
  game->tmpBoard = goBoard_create(size);
  game->flags = wms_malloc(game->board->area * sizeof(uint));
  game->hotMoves = wms_malloc(game->board->area * sizeof(int));
  for (i = 0;  i < goBoard_area(game->board);  ++i)  {
    game->hotMoves[i] = -1;
    game->flags[i] = 0;
  }
  game->flagsCleared = TRUE;
  game->cooloff = 0;
  game->state = goGameState_play;

  game->setWhoseMoveNum = -1;
  game->disputeAlive = FALSE;
  game->setWhoseMoveColor = 0;
  game->disputedLoc = 0;
  if (game->handicap && goGame_forcedHandicapPlacement(game))
    goGame_addHandicapStones(game, game->board);

  game->whoseMove = goGame_whoseTurnOnMove(game, game->moveNum);

  return(game);
}


void  goGame_destroy(GoGame *game)  {
  assert(MAGIC(game));
  goBoard_destroy(game->board);
  goBoard_destroy(game->tmpBoard);
  wms_free(game->flags);
  wms_free(game->hotMoves);
  wms_free(game->moves);
  MAGIC_UNSET(game);
  wms_free(game);
}


bool  goGame_move(GoGame *game, GoStone moveColor, int move, GoTimer *timer)  {
  bool  stateChange = FALSE;
  bool  disputeAnswer;
  int  i;

  assert(MAGIC(game));
  assert(game->passive || (moveColor == game->whoseMove));
  assert((game->state == goGameState_play) ||
	 (game->state == goGameState_dispute));
  assert(move >= 0);
  assert((game->state == goGameState_play) ||
	 (game->setWhoseMoveNum >= 0));
  assert(game->moveNum >= game->setWhoseMoveNum);
  assert(goGame_isLegal(game, moveColor, move));
  game->noLastMove = FALSE;
  if ((game->state == goGameState_play) && !game->flagsCleared)  {
    for (i = 0;  i < goBoard_area(game->board);  ++i)  {
      game->flags[i] = 0;
    }
  }
  addMoveToMoves(game, moveColor, move, game->moveNum);
  /*
   * Save this timer so if we rewind, we can reset the clock.
   */
  if (timer != NULL) {
    game->moves[game->moveNum].time = *timer;
    if (timer->timeLeft < 0)
      game->moves[game->moveNum].time.timeLeft = 0;
  } else {
    GoTimer  timer = {0};
    game->moves[game->moveNum].time = timer;
  }
  /*
   * Place the move on the board.
   */
  goGame_updateForMove(game, moveColor, move);
  game->maxMoves = game->moveNum;
  if (move == GOGAME_PASS)  {
    /* A pass may change your state. */
    if (game->state == goGameState_play)  {
      /* Two passes puts you into selectDead state. */
      if ((game->passCount == 2) && !game->passive && !game->forcePlay)  {
	stateChange = TRUE;
	game->setWhoseMoveNum = game->moveNum;
	game->state = goGameState_selectDead;
      }
    } else  {
      assert(game->state == goGameState_dispute);
      assert(game->setWhoseMoveNum >= 0);
      /*
       * Three passes puts you back into selectDead state.
       */
      if ((game->passCount == 3) && !game->passive && !game->forcePlay)  {
	disputeAnswer = game->disputeAlive;
	for (i = 0;  i < goBoard_area(game->board);  ++i)  {
	  if (game->flags[i] & GOGAMEFLAGS_DISPUTED)  {
	    if (disputeAnswer)  {
	      game->flags[i] = (game->flags[i] & ~(GOGAMEFLAGS_DISPUTED |
						   GOGAMEFLAGS_MARKDEAD |
						   GOGAMEFLAGS_WANTDEAD)) |
						     GOGAMEFLAGS_RESOLVEALIVE;
	    } else  {
	      game->flags[i] = (game->flags[i] & ~GOGAMEFLAGS_DISPUTED) |
		GOGAMEFLAGS_MARKDEAD | GOGAMEFLAGS_RESOLVEDEAD;
	    }
	  }
	}
	stateChange = TRUE;
	game->maxMoves = game->setWhoseMoveNum;
	goGame_moveTo(game, game->setWhoseMoveNum);
	game->state = goGameState_selectDead;
	/*
	 * disputeAlive is sometimes changed when we replay back to the
	 *   end of the game, so we restore it's value here.
	 */
	game->disputeAlive = disputeAnswer;
      }
    }
  }
  game->whoseMove = goGame_whoseTurnOnMove(game, game->moveNum);
  return(stateChange);
}


static void  goGame_updateForMove(GoGame *game, GoStone moveColor, int move)  {
  int  caps;

  assert(MAGIC(game));
  assert((game->state == goGameState_done) ||
	 goGame_isLegal(game, moveColor, move));
  
  caps = goBoard_addStone(game->board, moveColor, move, NULL);
  ++game->moveNum;
  assert(game->moveNum <= game->movesLen);

  /*
   * Update the list of hashes seen.
   * The hashes seen is used to quickly determine whether we have seen this
   *   board position before.  It is used for the superko rule and also to
   *   detect a game with no result under Japanese and Tibetan rules.
   */
  game->moves[game->moveNum].hash = goBoard_hashNoKo(game->board, moveColor);

  /*
   * I use hot stones to decide the Japanese and Tibetan ko rules.
   *   Basically, a hot stone is a stone that cannot be captured.
   * Now decide whether or not to mark this stone as hot, and also whether or
   *   not this was a "cooling" move; that is, one that made all hot stones
   *   no longer hot.
   */
  /* Check for the tibetan anti-snapback rule activation! */
  if ((caps == 1) && (goRules_ko[game->rules] == goRulesKo_tibetan) &&
      (goBoard_liberties(game->board, move) == 1))  {
    game->hotMoves[move] = game->moveNum;
  }
  /* Check for regular Japanese ko. */
  if (goBoard_koLoc(game->board))  {
    game->hotMoves[move] = game->moveNum;
  }
  if ((move == 0) || (game->state == goGameState_play))
    game->cooloff = game->moveNum;

  if (move == 0)  {
    if ((game->state == goGameState_dispute) &&
	(game->moveNum == game->setWhoseMoveNum))
      game->passCount = 0;
    else  {
      ++game->passCount;
      if ((game->state == goGameState_play) && (game->rules == goRules_aga))
	goBoard_addCaps(game->board, goStone_opponent(moveColor), 1);
    }
  } else  {
    game->passCount = 0;
    if (game->state == goGameState_dispute)  {
      game->disputeAlive = isDisputeAlive(game);
    }
  }
}


bool  goGame_isLegal(GoGame *game, GoStone moveColor, int move)  {
  assert(MAGIC(game));
  assert((move >= 0) &&
	 (move < game->board->area));
  switch(game->state)  {
  case goGameState_play:
  case goGameState_dispute:
    if (!game->passive && !game->forcePlay && (game->whoseMove != moveColor))
      return(FALSE);
    return(goGame_canMove(game, moveColor, move));
    break;
  case goGameState_selectDead:
  case goGameState_selectDisputed:
    return(goGame_canDispute(game, move));
    break;
  case goGameState_done:
    return(FALSE);
    break;
  default:
    abort();
    break;
  }
}


static bool  goGame_canMove(GoGame *game, GoStone s, int move)  {
  GoHash  newHash;
  bool  suicide, superKoMatch;
  GoBoard  *b;
  int  i, d;

  if (move == GOGAME_PASS)
    return(TRUE);
  if (goBoard_stone(game->board, move) != goStone_empty)
    return(FALSE);

  newHash = goBoard_quickHash(game->board, s, move, &suicide);

  if (suicide)  {
    if (!goRules_suicide[game->rules])
      return(FALSE);
    /* Single stone suicide is _always_ illegal. */
    for (d = 0;  d < 4;  ++d)  {
      if (goBoard_stone(game->board, move+goBoard_dir(game->board, d)) == s)
	break;
    }
    if (d == 4)
      return(FALSE);
  }

  /* Check for a ko. */
  switch(goRules_ko[game->rules])  {
  case goRulesKo_super:
    assert(game->moveNum <= game->movesLen);
    for (i = 0;  i < game->moveNum;  ++i)  {
      if (goHash_eq(game->moves[i].hash, newHash) &&
	  (goGame_whoseTurnOnMove(game, i) == goStone_opponent(s)))  {
	/*
	 * We _may_ have a match.  Rebuild the board at the move in question
	 *   and see if they are equal.
	 */
	goBoard_copy(game->board, game->tmpBoard);
	goBoard_addStone(game->tmpBoard, s, move, NULL);
	b = goGame_replay(game, i, TRUE);
	superKoMatch = goBoard_eq(game->tmpBoard, b);
	goBoard_destroy(b);
	if (superKoMatch)
	  return(FALSE);
      }
    }
    return(TRUE);
    break;
  case goRulesKo_tibetan:
  case goRulesKo_japanese:
    /*
     * Although these three ko rules are _not_ the same, they all restrict
     *   moves with "hot" stones.  The only difference in them is how to
     *   decide which stones are hot.
     */
    for (d = 0;  d < 4;  ++d)  {
      if (goGame_isHot(game, move + goBoard_dir(game->board, d)) &&
	  (goBoard_stone(game->board,
			 move + goBoard_dir(game->board, d)) != s))
	return(FALSE);
    }
    return(TRUE);
    break;
  default:
    abort();
    break;
  }
}


static bool  goGame_canDispute(GoGame *game, int move)  {
  assert(MAGIC(game));

  assert((move >= 0) && (move < goBoard_area(game->board)));
  if (!goStone_isStone(goBoard_stone(game->board, move)))
    return(FALSE);
  if (game->flags[move] & GOGAMEFLAGS_RESOLVED)
    return(FALSE);
  return(TRUE);
}


GoStone  goGame_whoseTurnOnMove(GoGame *game, int moveNum)  {
  assert(MAGIC(game));
  if ((game->setWhoseMoveNum >= 0) &&
      (game->moveNum >= game->setWhoseMoveNum))  {
    if ((moveNum - game->setWhoseMoveNum) & 1)
      return(goStone_opponent(game->setWhoseMoveColor));
    else
      return(game->setWhoseMoveColor);
  } else if (goGame_forcedHandicapPlacement(game))  {
    if (game->handicap)  {
      if (moveNum & 1)
	return(goStone_black);
      else
	return(goStone_white);
    } else
      if (moveNum & 1)
	return(goStone_white);
      else
	return(goStone_black);
  } else  {
    if (game->handicap)  {
      if (moveNum < game->handicap)
	return(goStone_black);
      else  {
	if ((moveNum - game->handicap) & 1)
	  return(goStone_black);
	else
	  return(goStone_white);
      }
    } else  {
      if (moveNum & 1)
	return(goStone_white);
      else
	return(goStone_black);
    }
  }
}

      
static bool  goGame_forcedHandicapPlacement(GoGame *game)  {
  return(!goRules_freeHandicaps[game->rules] && !game->passive &&
	 (game->size >= 9) && (game->size & 1) && (game->handicap <= 9));
}


static void  goGame_addHandicapStones(GoGame *game, GoBoard *board)  {
  int  locs[9], numLocs = 0;
  int  lo, mid, hi;
  
  assert(goGame_forcedHandicapPlacement(game));
  if (game->size >= 13)
    lo = 3;
  else
    lo = 2;
  hi = game->size - lo - 1;
  mid = game->size / 2;
  locs[numLocs++] = goBoard_xy2Loc(board, hi, lo);
  locs[numLocs++] = goBoard_xy2Loc(board, lo, hi);
  if (game->handicap >= 3)
    locs[numLocs++] = goBoard_xy2Loc(board, lo, lo);
  if (game->handicap >= 4)  {
    locs[numLocs++] = goBoard_xy2Loc(board, hi, hi);
    if (game->handicap & 1)
      locs[numLocs++] = goBoard_xy2Loc(board, mid, mid);
  }
  if (game->handicap >= 6)  {
    locs[numLocs++] = goBoard_xy2Loc(board, lo, mid);
    locs[numLocs++] = goBoard_xy2Loc(board, hi, mid);
  }
  if (game->handicap >= 8)  {
    locs[numLocs++] = goBoard_xy2Loc(board, mid, lo);
    locs[numLocs++] = goBoard_xy2Loc(board, mid, hi);
  }
  while (numLocs)  {
    --numLocs;
    goBoard_addStone(board, goStone_black, locs[numLocs], NULL);
  }
}


bool  goGame_moveTo(GoGame *game, int moveNum)  {
  bool  newState = FALSE;
  int  oldSetColorMove;

  assert(MAGIC(game));
  assert(moveNum >= 0);
  assert(game->passive || game->forcePlay ||
	 ((game->state == goGameState_play) ||
	  (game->state == goGameState_done) ||
	  (game->state == goGameState_dispute)));
  assert((game->state != goGameState_dispute) ||
	 (moveNum >= game->setWhoseMoveNum));
  assert(moveNum <= game->maxMoves);
  game->moveNum = moveNum;
  oldSetColorMove = game->setWhoseMoveNum;
  goGame_replay(game, game->moveNum, FALSE);
  if ((game->state == goGameState_dispute) &&
      (game->moveNum >= oldSetColorMove))
    game->setWhoseMoveNum = oldSetColorMove;
  if ((game->state == goGameState_play) && (game->passCount == 2) &&
      !game->passive && !game->forcePlay)  {
    newState = TRUE;
    game->state = goGameState_done;
  } else if ((game->state == goGameState_done) && (game->passCount < 2)
	     && !game->passive && !game->forcePlay)  {
    newState = TRUE;
    game->state = goGameState_play;
  }
  return(newState);
}


/*
 * If separateBoard is set, then we assume that you only want the way the
 *   board looked at that point so the game structure will be unchanged.
 * Otherwise game will be exactly as if a game were played to that point,
 *   except (of course) maxMoves will be different.
 */
static GoBoard  *goGame_replay(GoGame *game, int moves, bool separateBoard)  {
  GoBoard  *b;
  int  i;

  assert(moves <= game->maxMoves);
  b = goBoard_create(game->size);
  if (!separateBoard)  {
    goBoard_destroy(game->board);
    game->setWhoseMoveNum = -1;
    game->board = b;
    game->moveNum = 0;
    game->passCount = 0;
    for (i = 0;  i < goBoard_area(b);  ++i)  {
      game->hotMoves[i] = -1;
    }
  }
  if (game->handicap && goGame_forcedHandicapPlacement(game))
    goGame_addHandicapStones(game, b);
  assert(moves <= game->movesLen);
  for (i = 0;  i < moves;  ++i)  {
    if (separateBoard) {
      goBoard_addStone(b, game->moves[i].color, game->moves[i].move, NULL);
    } else {
      game->whoseMove = game->moves[i].color;
      if ((game->time.type == goTime_none) ||
	  (game->moves[i].time.timeLeft >= 0))  {
	/*
	 * If there was a time loss, then the last move isn't valid - the
	 *   game ended before the move could be made.
	 */
	goGame_updateForMove(game, game->whoseMove, game->moves[i].move);
      } else {
	++game->moveNum;
	assert(game->moveNum < game->movesLen);
      }
    }
  }
  if (!separateBoard)  {
    game->whoseMove = goGame_whoseTurnOnMove(game, i);
  }
  return(b);
}


void  goGame_markDead(GoGame *game, int loc)  {
  GoBoardGroupIter  i;
  int  iterLoc;

  assert(MAGIC(game));
  assert(goStone_isStone(goBoard_stone(game->board, loc)));
  goBoardGroupIter(i, game->board, loc)  {
    iterLoc = goBoardGroupIter_loc(i, game->board);
    game->flags[iterLoc] ^= GOGAMEFLAGS_MARKDEAD;
  }
}


void  goGame_resume(GoGame *game)  {
  assert(MAGIC(game));
  assert(game->state == goGameState_selectDead);
  game->maxMoves -= 2;
  game->state = goGameState_play;
  goGame_replay(game, game->maxMoves, FALSE);
  game->whoseMove = goGame_whoseTurnOnMove(game, game->moveNum);
}


void  goGame_done(GoGame *game)  {
  assert(MAGIC(game));
  assert(game->state == goGameState_selectDead);
  game->state = goGameState_done;
  game->maxMoves = game->moveNum;
}


void  goGame_dispute(GoGame *game, int loc)  {
  GoBoardGroupIter  i;
  int  stoneLoc;

  assert(MAGIC(game));
  assert(game->state == goGameState_selectDisputed);
  game->state = goGameState_dispute;
  game->disputeAlive = TRUE;
  game->disputedLoc = loc;
  game->setWhoseMoveColor = goStone_opponent(goBoard_stone(game->board, loc));
  game->passCount = 0;
  game->state = goGameState_dispute;
  assert(goStone_isStone(game->setWhoseMoveColor));
  for (stoneLoc = 0;  stoneLoc < goBoard_area(game->board);  ++stoneLoc);
  goBoardGroupIter(i, game->board, loc)  {
    stoneLoc = goBoardGroupIter_loc(i, game->board);
    game->flags[stoneLoc] = GOGAMEFLAGS_DISPUTED;
  }
  game->whoseMove = goGame_whoseTurnOnMove(game, game->moveNum);
}


static bool  isDisputeAlive(GoGame *game)  {
  int  i;

  for (i = 0;  i < goBoard_area(game->board);  ++i)  {
    if ((game->flags[i] & GOGAMEFLAGS_DISPUTED) &&
	(goBoard_stone(game->board, i) ==
	 goStone_opponent(game->setWhoseMoveColor)))
      return(TRUE);
  }
  return(FALSE);
}


static void  addMoveToMoves(GoGame *game, GoStone moveColor, int move,
			    int moveNum)  {
  GoGameMove  *newMoves;
  int  i;

  if (moveNum + 1 == game->movesLen)  {
    newMoves = wms_malloc(game->movesLen * 2 * sizeof(game->moves[0]));
    for (i = 0;  i < game->movesLen;  ++i)  {
      newMoves[i] = game->moves[i];
    }
    wms_free(game->moves);
    game->moves = newMoves;
    game->movesLen *= 2;
  }
  assert(moveNum + 1 < game->movesLen);
  game->moves[moveNum].move = move;
  game->moves[moveNum].color = moveColor;
}


int  goGame_lastMove(GoGame *g)  {
  assert(MAGIC(g));
  if (g->noLastMove || (g->moveNum == 0))
    return(0);
  assert(g->moveNum > 0);
  assert(g->moveNum <= g->movesLen);
  return(g->moves[g->moveNum - 1].move);
}


void  goGame_setBoard(GoGame *game, GoStone color, int loc)  {
  assert(MAGIC(game));
  if (goStone_isStone(color) &&
      goStone_isStone(goBoard_stone(game->board, loc)))  {
    if (goBoard_stone(game->board, loc) == color)
      return;
    goBoard_addStone(game->board, goStone_empty, loc, NULL);
  }
  goBoard_addStone(game->board, color, loc, NULL);
}


const GoTimer  *goGame_getTimer(const GoGame *game, GoStone color)  {
  if (color == game->whoseMove)  {
    if (game->moveNum <= 1) {
      return(NULL);
    } else {
      return(&game->moves[game->moveNum - 2].time);
    }
  } else  {
    if (game->moveNum <= 0) {
      return(NULL);
    } else {
      return(&game->moves[game->moveNum - 1].time);
    }
  }
}

