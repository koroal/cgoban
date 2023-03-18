/*
 * src/arena.c, part of Complete Goban (game program)
 * Copyright (C) 1996-1997 William Shubert.
 * See "configure.h.in" for more copyright information.
 */

#include <wms.h>
#include "gmp/engine.h"
#include "goScore.h"

#ifdef  _ARENA_H_
#error  Levelization Error.
#endif
#include "arena.h"


/**********************************************************************
 * Forward Declarations
 **********************************************************************/
static void  match(Arena *arena, int whitePlayer, int size, float komi,
		   int handicap);
static ButOut  gotMove(GmpEngine *ge, void *packet, GoStone color,
		       int x, int y);
static ButOut  gotErr(GmpEngine *ge, void *packet, const char *errStr);


/**********************************************************************
 * Functions
 **********************************************************************/

void  arena(Cgoban *cg, const char *prog1, const char *prog2,
	    int size, float komi, int handicap)  {
  Arena  arena;
  int  in, out;
  int  i, numGames;
  static const GmpActions  actions = {NULL, gotMove, NULL, gotErr};

  MAGIC_SET(&arena);
  arena.cg = cg;
  numGames = clp_getInt(cg->clp, "arena.games");
  gmp_forkProgram(cg, &in, &out, prog1, 0, 0);
  gmpEngine_init(cg, &arena.engines[0], in, out, &actions, &arena);
  gmp_forkProgram(cg, &in, &out, prog1, 0, 0);
  gmpEngine_init(cg, &arena.engines[1], in, out, &actions, &arena);
  for (i = 0;  i < numGames;  ++i)  {
    match(&arena, i & 1, size, komi, handicap);
  }
  MAGIC_UNSET(&arena);
}


static void  match(Arena *arena, int whitePlayer, int size, float komi,
		   int handicap)  {
  static const GoTime  noTime = {goTime_none, 0, 0, 0};
  GoScore  score;
  int  winner;

  arena->game = goGame_create(size, goRules_nz, handicap, komi,
			      &noTime, FALSE);
  gmpEngine_startGame(&arena->engines[whitePlayer],
		      size, handicap, komi, TRUE, TRUE);
  gmpEngine_startGame(&arena->engines[whitePlayer ^ 1],
		      size, handicap, komi, TRUE, FALSE);
  butEnv_events(arena->cg->env);
  goScore_compute(&score, arena->game);
  if (score.scores[0] > score.scores[1])
    winner = 0;
  else
    winner = 1;
  printf("%d %g\n", winner ^ whitePlayer,
	 score.scores[winner] - score.scores[winner ^ 1]);
  goGame_destroy(arena->game);
}


static ButOut  gotMove(GmpEngine *ge, void *packet, GoStone color,
		       int x, int y)  {
  Arena  *arena = packet;
  int  move, engineNum;

  assert(MAGIC(arena));
  if (ge == &arena->engines[0])
    engineNum = 0;
  else  {
    assert(ge == &arena->engines[1]);
    engineNum = 1;
  }
  assert(color == arena->game->whoseMove);
  if (color == goStone_white)  {
    assert(!arena->engines[engineNum].iAmWhite);
  } else  {
    assert(arena->engines[engineNum].iAmWhite);
  }
  if (x < 0)  {
    goGame_pass(arena->game, arena->game->whoseMove, NULL);
    gmpEngine_sendPass(&arena->engines[engineNum ^ 1]);
  } else  {
    move = goBoard_xy2Loc(arena->game->board, x, y);
    if (!goGame_isLegal(arena->game, arena->game->whoseMove, move))  {
      fprintf(stderr, "Got an illegal move.\n");  exit(1);
    }
    goGame_move(arena->game, arena->game->whoseMove, move, NULL);
    gmpEngine_sendMove(&arena->engines[engineNum ^ 1], x, y);
  }
  if (arena->game->state == goGameState_play)
    return(0);
  else
    return(BUTOUT_STOPWAIT);
}


static ButOut  gotErr(GmpEngine *ge, void *packet, const char *errStr)  {
  fprintf(stderr, "Error \"%s\" happened.\n", errStr);
  exit(1);
}
