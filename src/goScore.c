/*
 * Part of Complete Goban (game program)
 *
 * $Source: /cvsroot/cgoban1/cgoban1/src/goScore.c,v $
 * $Revision: 1.3 $
 * $Date: 2000/02/04 02:10:21 $
 *
 * Copyright (C) 1995-1996,2000 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#include <wms.h>
#include <wms/rnd.h>
#include <wms/str.h>
#include "goBoard.h"
#include "goGame.h"
#include "goScore.h"
#include "msg.h"


/**********************************************************************
 * Forward Declarations
 **********************************************************************/
static void  checkIfSeki(GoGame *game, int loc);
static void  markNeighborEmptySpecial(GoGame *game, int loc, GoStone s);
static void  markSpecialStones(GoGame *game, GoStone s, uint mark);
static int  findSpecialEyes(GoGame *game, GoStone s);
static int  countNeighbors(GoGame *g, GoBoard *b, int loc, GoStone s);
static void  removeFakeEyes(GoGame *g, GoStone s);
static void  testGroupForFake(GoGame *g, int loc, GoStone s);
static void  paintRelatedGroupsFaketest(GoGame *g, int loc, GoStone s);


/**********************************************************************
 * Functions
 **********************************************************************/
Str  *goScore_str(GoScore *score, GoGame *game,
		  const GoTime *time, const GoTimer  *timers)  {
  Str  *result, ingTimes[2], winner;
  GoRules  rules = game->rules;
  float  scores[2];
  int  timePenalties[2];
  GoStone  s;

  result = str_create();
  goStoneIter(s)  {
    str_init(&ingTimes[s]);
    if (time != NULL)
      timePenalties[s] = goTime_ingPenalty(time, &timers[s]);
    if (timePenalties[s])  {
      str_print(&ingTimes[s], msg_ingPenaltyComment, timePenalties[s]);
    }
    scores[s] = score->scores[s] - timePenalties[s];
  }
  if (goRules_scoreKills[rules])  {
    str_print(result, msg_scoreKillsComment,
	      score->territories[goStone_white],
	      goGame_caps(game, goStone_white) +
	      score->deadStones[goStone_white],
	      game->komi,
	      str_chars(&ingTimes[goStone_white]),
	      scores[goStone_white],

	      score->territories[goStone_black],
	      goGame_caps(game, goStone_black) +
	      score->deadStones[goStone_black],
	      str_chars(&ingTimes[goStone_black]),
	      scores[goStone_black]);
  } else  {
    str_print(result, msg_scoreNoKillsComment,
	      score->territories[goStone_white],
	      score->livingStones[goStone_white],
	      (double)score->dame / 2.0,
	      game->komi,
	      str_chars(&ingTimes[goStone_white]),
	      scores[goStone_white],
	      
	      score->territories[goStone_black],
	      score->livingStones[goStone_black],
	      (double)score->dame / 2.0,
	      str_chars(&ingTimes[goStone_black]),
	      scores[goStone_black]);
  }
  str_init(&winner);
  if (scores[goStone_white] > scores[goStone_black])  {
    str_print(&winner, msg_winnerComment,
	      msg_stoneNames[goStone_white],
	      scores[goStone_white] - scores[goStone_black]);
  } else if ((scores[goStone_black] > scores[goStone_white]) ||
	     (game->rules == goRules_ing))  {
    str_print(&winner, msg_winnerComment,
	      msg_stoneNames[goStone_black],
	      scores[goStone_black] - scores[goStone_white]);
  } else  {
    str_print(&winner, msg_jigoComment);
  }
  str_cat(result, &winner);
  str_catChar(result, '\n');
  str_deinit(&winner);
  str_deinit(&ingTimes[goStone_white]);
  str_deinit(&ingTimes[goStone_black]);
  return(result);
}
  

void  goScore_compute(GoScore *score, GoGame *game)  {
  int  sekiArea[2], dame = 0;
  GoBoard  *b = game->board;
  bool  change;
  int  i, j, min, max;
  uint  oldFlags;
  GoStone  s;

  assert(MAGIC(game));
  assert(game->state >= goGameState_selectDead);
  game->flagsCleared = FALSE;
  min = goBoard_minLoc(game->board);
  max = goBoard_maxLoc(game->board);
  score->livingStones[goStone_black] = score->livingStones[goStone_white] = 0;
  score->territories[goStone_black]  = score->territories[goStone_white] = 0;
  score->deadStones[goStone_black]   = score->deadStones[goStone_white] = 0;
  score->dame = 0;
  sekiArea[goStone_black] = sekiArea[goStone_white] = 0;
  /* Add up the number of living stones. */
  for (i = min;  i < max;  ++i)  {
    game->flags[i] &= ~(GOGAMEFLAGS_SEEN |
			GOGAMEFLAGS_SEKI | GOGAMEFLAGS_NOSEKI);
    s = goBoard_stone(b, i);
    if (goStone_isStone(s))  {
      if (game->flags[i] & GOGAMEFLAGS_MARKDEAD)  {
	++score->deadStones[goStone_opponent(s)];
      } else  {
	game->flags[i] |= goGameFlags_see(s);
	++score->livingStones[s];
      }
    }
  }
  /*
   * Now iterate over the board, figuring out what empty spaces see
   *   different stones.
   */
  do  {
    change = FALSE;
    for (i = min;  i < max;  ++i)  {
      if ((goBoard_stone(b, i) == goStone_empty) ||
	  (game->flags[i] & GOGAMEFLAGS_MARKDEAD))  {
	/* It's an empty space. */
	oldFlags = game->flags[i];
	for (j = 0;  j < 4;  ++j)  {
	  game->flags[i] |=
	    game->flags[i + goBoard_dir(b, j)] & GOGAMEFLAGS_SEEN;
	}
	change = (change || (game->flags[i] != oldFlags));
      }
    }
  } while (change);
  /*
   * Go through and find all the sekis.
   */
  if (game->rules == goRules_japanese)  {
    for (i = 0;  i < goBoard_area(game->board);  ++i)  {
      if (goStone_isStone(goBoard_stone(game->board, i)) &&
	  !(game->flags[i] &
	    (GOGAMEFLAGS_SEKI | GOGAMEFLAGS_NOSEKI | GOGAMEFLAGS_MARKDEAD)))
	checkIfSeki(game, i);
    }
  }
  for (i = min;  i < max;  ++i)  {
    if ((goBoard_stone(b, i) == goStone_empty) || 
	(game->flags[i] & GOGAMEFLAGS_MARKDEAD))  {
      if (((game->flags[i] & GOGAMEFLAGS_SEEBLACK) == 0) ==
	  ((game->flags[i] & GOGAMEFLAGS_SEEWHITE) == 0))  {
	++dame;
      } else if (game->flags[i] & GOGAMEFLAGS_SEEBLACK)  {
	if (game->flags[i] & GOGAMEFLAGS_SEKI)  {
	  ++sekiArea[goStone_black];
	} else  {
	  ++score->territories[goStone_black];
	}
      } else  {
	assert(game->flags[i] & GOGAMEFLAGS_SEEWHITE);
	if (game->flags[i] & GOGAMEFLAGS_SEKI)  {
	  ++sekiArea[goStone_white];
	} else  {
	  ++score->territories[goStone_white];
	}
      }
    }
  }
  score->dame = dame;
  switch(game->rules)  {
  case goRules_chinese:
  case goRules_ing:
  case goRules_nz:
    goStoneIter(s)  {
      score->territories[s] += sekiArea[s];
      score->scores[s] = (float)dame / 2.0 + score->territories[s] +
	score->livingStones[s];
    }
    break;
  case goRules_aga:
  case goRules_tibetan:
    goStoneIter(s)  {
      score->territories[s] += sekiArea[s];
      score->scores[s] = score->territories[s] + goGame_caps(game, s) +
	score->deadStones[s];
    }
    break;
  case goRules_japanese:
    goStoneIter(s)  {
      score->scores[s] = score->territories[s] + goGame_caps(game, s) +
	score->deadStones[s];
    }
    break;
  }
  score->scores[goStone_white] += game->komi;
}


static void  checkIfSeki(GoGame *game, int loc)  {
  GoStone  s;
  int  i;

  s = goBoard_stone(game->board, loc);
  goBoard_copy(game->board, game->tmpBoard);
  for (i = 0;  i < goBoard_area(game->tmpBoard);  ++i)  {
    if (goStone_isStone(goBoard_stone(game->tmpBoard, i)) &&
	(game->flags[i] & GOGAMEFLAGS_MARKDEAD))
      goBoard_rmGroup(game->tmpBoard, i);
  }
  markNeighborEmptySpecial(game, loc, s);
  removeFakeEyes(game, s);
  if (findSpecialEyes(game, s) > 1)  {
    markSpecialStones(game, s, GOGAMEFLAGS_NOSEKI);
    return;
  }
  markSpecialStones(game, s, GOGAMEFLAGS_SEKI);
}


static void  markNeighborEmptySpecial(GoGame *game, int loc, GoStone s)  {
  int  i, d;
  bool  change;
  GoStone  stoneHere;
  uint  oldFlags, enemySeen;

  for (i = 0;  i < goBoard_area(game->tmpBoard);  ++i)
    game->flags[i] &= ~(GOGAMEFLAGS_SPECIAL | GOGAMEFLAGS_EYE |
			GOGAMEFLAGS_NOTFAKE | GOGAMEFLAGS_FAKETEST);
  game->flags[loc] |= GOGAMEFLAGS_SPECIAL;
  do  {
    change = FALSE;
    for (i = 0;  i < goBoard_area(game->tmpBoard);  ++i)  {
      stoneHere = goBoard_stone(game->tmpBoard, i);
      if (!(game->flags[i] & GOGAMEFLAGS_SPECIAL) &&
	  ((stoneHere == goStone_empty) || (stoneHere == s)))  {
	oldFlags = game->flags[i];
	for (d = 0;  d < 4;  ++d)
	  game->flags[i] |= game->flags[i+goBoard_dir(game->tmpBoard, d)] &
	    GOGAMEFLAGS_SPECIAL;
	change = (change || (oldFlags != game->flags[i]));
      }
    }
  } while (change);
  enemySeen = goGameFlags_see(goStone_opponent(s));
  for (i = 0;  i < goBoard_area(game->tmpBoard);  ++i)  {
    if (game->flags[i] & enemySeen)
      game->flags[i] &= ~GOGAMEFLAGS_SPECIAL;
  }
}


static void  markSpecialStones(GoGame *game, GoStone s, uint mark)  {
  int  i;

  for (i = 0;  i < goBoard_area(game->tmpBoard);  ++i)  {
    if (game->flags[i] & GOGAMEFLAGS_SPECIAL)
      game->flags[i] |= mark;
  }
}


static int  findSpecialEyes(GoGame *game, GoStone s)  {
  uint  oldFlags;
  int  i, d, eyeSize, n;
  int  deadEnemyLoc, enemySize = 0, deadEnemyConn[5];
  int  specialConn[5];
  bool  change, emptyAwayFromEnemy = FALSE;
  GoStone  opponent = goStone_opponent(s);

  deadEnemyLoc = 0;
  for (i = 0;  i < 5;  ++i)  {
    deadEnemyConn[i] = 0;
    specialConn[i] = 0;
  }
  for (i = 0;  i < goBoard_area(game->tmpBoard);  ++i)  {
    if ((game->flags[i] & GOGAMEFLAGS_SPECIAL) &&
	(goBoard_stone(game->tmpBoard, i) == goStone_empty))  {
      game->flags[i] |= GOGAMEFLAGS_EYE;
      break;
    }
  }
  if (i == goBoard_area(game->tmpBoard))  {
    return(0);
  }
  /*
   * If this is a dead enemy stone, we want to know!
   */
  if (goBoard_stone(game->board, i) == opponent)  {
    deadEnemyLoc = i;
    ++deadEnemyConn[countNeighbors(game, game->board, i, opponent)];
    ++enemySize;
  } else  {
    n = countNeighbors(game, game->board, i, goStone_empty);
    ++specialConn[n];
    if (n >= 3)
      return(2);
    if (countNeighbors(game, game->board, i, opponent) == 0)
      emptyAwayFromEnemy = TRUE;
  }
  eyeSize = 1;
  do  {
    change = FALSE;
    for (i = 0;  i < goBoard_area(game->tmpBoard);  ++i)  {
      if (((game->flags[i] & (GOGAMEFLAGS_EYE|GOGAMEFLAGS_SPECIAL)) ==
	   GOGAMEFLAGS_SPECIAL) &&
	  (goBoard_stone(game->tmpBoard, i) == goStone_empty))  {
	oldFlags = game->flags[i];
	for (d = 0;  d < 4;  ++d)
	  game->flags[i] |= game->flags[i+goBoard_dir(game->tmpBoard, d)] &
	      GOGAMEFLAGS_EYE;
	if (game->flags[i] != oldFlags)  {
	  change = TRUE;
	  ++eyeSize;
	  if (goBoard_stone(game->board, i) == opponent)  {
	    if (deadEnemyLoc)  {
	      if (!goBoard_groupEq(game->board, deadEnemyLoc, i))
		return(2);
	    } else
	      deadEnemyLoc = i;
	    ++deadEnemyConn[countNeighbors(game, game->board, i, opponent)];
	    ++enemySize;
	  } else  {
	    n = countNeighbors(game, game->board, i, goStone_empty);
	    ++specialConn[n];
	    if (n >= 3)
	      return(2);
	    if (countNeighbors(game, game->board, i, opponent) == 0)
	      emptyAwayFromEnemy = TRUE;
	  }
	}
      }
    }
  } while (change);

  /*
   * Look for a second eye.
   */
  for (i = 0;  i < goBoard_area(game->tmpBoard);  ++i)  {
    if (((game->flags[i] & (GOGAMEFLAGS_SPECIAL |
			    GOGAMEFLAGS_EYE)) == GOGAMEFLAGS_SPECIAL) &&
	(goBoard_stone(game->tmpBoard, i) == goStone_empty))  {
      return(2);
    }
  }

  /*
   * We have one big eye with no dead enemies in in.  It can make unless it
   *   is less than 3 in size or it is a 4 space 2x2 eye.
   */
  if (enemySize == 0)  {
    switch(eyeSize)  {
    case(1):
    case(2):
      return(1);
    case(4):
      if (specialConn[2] == 4)
	return(1);
      else
	return(2);
    default:
      return(2);
    }
  }

  /*
   * If there are enemies and you have a liberty away from them, then you
   *   can get two eyes.
   */
  if (emptyAwayFromEnemy)
    return(2);

  /*
   * We have some dead enemies.  See if they're in a dead shape.
   */

  /*
   * First test: Matches      ?
   *                        ? # ?
   *                          ?
   */
  if (deadEnemyConn[4] + deadEnemyConn[3] + deadEnemyConn[2] < 2)
    return(1);

  /*
   * Second test: Matches       # #
   *                          ? # #
   *                            ?
   */
  if ((deadEnemyConn[4] == (deadEnemyConn[1] == 2)) &&
      (deadEnemyConn[3] == (deadEnemyConn[1] == 1)) &&
      (deadEnemyConn[4] + deadEnemyConn[3] + deadEnemyConn[2] == 4) &&
      (deadEnemyConn[1] < 3))
    return(1);

  /*
   * Second test: Matches       # #
   *                          # # #
   *                          # #
   */
  if ((deadEnemyConn[4] == 1) && (deadEnemyConn[2] == 6) &&
      (enemySize == 7))
    return(1);
  return(2);
}


static int  countNeighbors(GoGame *g, GoBoard *b, int loc, GoStone s)  {
  int  d, newLoc, count;

  for (d = 0, count = 0;  d < 4;  ++d)  {
    newLoc = loc + goBoard_dir(b, d);
    if ((goBoard_stone(b, newLoc) == s) &&
	(g->flags[newLoc] & GOGAMEFLAGS_SPECIAL))
      ++count;
  }
  return(count);
}


static void  removeFakeEyes(GoGame *g, GoStone s)  {
  int  i;
  bool  change;

  do  {
    change = FALSE;
    for (i = 0;  i < goBoard_area(g->tmpBoard);  ++i)  {
      if ((goBoard_stone(g->tmpBoard, i) == s) &&
	  ((g->flags[i] & (GOGAMEFLAGS_SPECIAL | GOGAMEFLAGS_NOTFAKE)) ==
	    GOGAMEFLAGS_SPECIAL))  {
	change = TRUE;
	testGroupForFake(g, i, s);
	break;
      }
    }
  } while (change);
}


static void  testGroupForFake(GoGame *g, int loc, GoStone s)  {
  int  i, d, faketests, conns, newLoc;
  int  emptySeen = 0;

  paintRelatedGroupsFaketest(g, loc, s);
  /*
   * All SPECIAL empties touching FAKETEST stones are fake UNLESS:
   *   - There are more than one SPECIAL empties that touch FAKETEST
   *     stones.
   * or
   *   - There is at least one SPECIAL empty that is bounded on all four
   *     sides by FAKETEST stones.
   */
  for (i = 0;  i < goBoard_area(g->tmpBoard);  ++i)  {
    if ((goBoard_stone(g->tmpBoard, i) == goStone_empty) &&
	(g->flags[i] & GOGAMEFLAGS_SPECIAL))  {
      faketests = 0;
      conns = 0;
      for (d = 0;  d < 4;  ++d)  {
	newLoc = i + goBoard_dir(g->tmpBoard, d);
	if (g->flags[newLoc] & GOGAMEFLAGS_FAKETEST) {
	  ++faketests;
	} else if (goBoard_stone(g->tmpBoard, newLoc) == goStone_edge) {
	  ++conns;
	}
      }
      if (faketests)
	++emptySeen;
      if (faketests + conns == 4) {
	++emptySeen;
      }
    }
  }
  if (emptySeen != 1)  {
    /* No fakes associated with this group. */
    for (i = 0;  i < goBoard_area(g->tmpBoard);  ++i)  {
      if ((goBoard_stone(g->tmpBoard, i) == s) &&
	  (g->flags[i] & GOGAMEFLAGS_FAKETEST))  {
	g->flags[i] |= GOGAMEFLAGS_NOTFAKE;
      }
      g->flags[i] &= ~GOGAMEFLAGS_FAKETEST;
    }
  } else  {
    /*
     * Fake.  Turn off the specialness of all adjacent empties, and
     *   also turn off all NOTFAKE flags.
     * Turn on the SEEBLACK and the SEEWHITE flags to make sure that
     *   the fake eyes don't count for territory.
     */
#ifdef PRINT_OUT_BOARD
    int x, y;
    printf("--------------------------------------------------------------\n");
    for (y = 0; y < 9; ++y) {
      for (x = 0; x < 9; ++x) {
	printf("%c%c%c%c ",
	       (g->flags[x+y*10+11] & GOGAMEFLAGS_FAKETEST ? 'F' : '.'),
	       (g->flags[x+y*10+11] & GOGAMEFLAGS_SPECIAL ? 'S' : '.'),
	       (g->flags[x+y*10+11] & GOGAMEFLAGS_SEEBLACK ? 'B' : '.'),
	       (g->flags[x+y*10+11] & GOGAMEFLAGS_SEEWHITE ? 'W' : '.'));
      }
      printf("\n");
    }
    printf("--------------------------------------------------------------\n");
#endif
    for (i = 0;  i < goBoard_area(g->tmpBoard);  ++i)  {
      if ((goBoard_stone(g->tmpBoard, i) == s) &&
	  (g->flags[i] & GOGAMEFLAGS_FAKETEST))  {
	for (d = 0;  d < 4;  ++d)  {
	  newLoc = i + goBoard_dir(g->tmpBoard, d);
	  if ((g->flags[newLoc] & GOGAMEFLAGS_SPECIAL) &&
	      (goBoard_stone(g->tmpBoard, newLoc) == goStone_empty))  {
	    g->flags[newLoc] = (g->flags[newLoc] & ~GOGAMEFLAGS_SPECIAL) |
	      (GOGAMEFLAGS_SEEBLACK|GOGAMEFLAGS_SEEWHITE);
	  }
	}
      }
      g->flags[i] &= ~(GOGAMEFLAGS_FAKETEST | GOGAMEFLAGS_NOTFAKE);
    }
  }
}


/*
 * All stones that are connected by non-eye spaces are related.  Here
 *   we paint all such groups FAKETEST.
 */
static void  paintRelatedGroupsFaketest(GoGame *g, int loc, GoStone s)  {
  bool  change;
  uint  oldFlags;
  int  i, d;

  g->flags[loc] |= GOGAMEFLAGS_FAKETEST;
  do  {
    change = FALSE;
    for (i = 0;  i < goBoard_area(g->tmpBoard);  ++i)  {
      if ((goBoard_stone(g->tmpBoard, i) == s) ||
	  ((goBoard_stone(g->tmpBoard, i) == goStone_empty) &&
	  !(g->flags[i] & GOGAMEFLAGS_SPECIAL)))  {
	oldFlags = g->flags[i];
	for (d = 0;  d < 4;  ++d)
	  g->flags[i] |= (g->flags[i + goBoard_dir(g->tmpBoard, d)] &
			  GOGAMEFLAGS_FAKETEST);
	change = (change || (oldFlags != g->flags[i]));
      }
    }
  } while (change);
}


void  goScore_zero(GoScore *score)  {
  GoStone  color;

  score->dame = 0;
  goStoneIter(color)  {
    score->territories[color] = 0;
    score->livingStones[color] = 0;
    score->deadStones[color] = 0;
    score->scores[color] = 0.0;
  }
}
