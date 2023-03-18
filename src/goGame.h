/*
 * src/gogame.h, part of Complete Goban (game program)
 * Copyright (C) 1995-1997 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _GOGAME_H_
#define  _GOGAME_H_  1

#ifndef  _GOHASH_H_
#include "goHash.h"
#endif
#ifndef  _GOBOARD_H_
#include "goBoard.h"
#endif
#ifndef  _GOTIME_H_
#include "goTime.h"
#endif


/**********************************************************************
 * Constants
 **********************************************************************/
#define  GOGAME_PASS  0


/**********************************************************************
 * Data types
 **********************************************************************/

typedef enum  {
  goRules_chinese, goRules_japanese, goRules_ing, goRules_aga,
  goRules_nz, goRules_tibetan
} GoRules;
#define  goRules_num  ((int)goRules_tibetan + 1)


typedef enum  {
  goRulesKo_japanese, goRulesKo_super, goRulesKo_tibetan
    /*
     * goRulesKo_ing is longer supported since it can't be implemented
     *   accurately by a computer without lots of help from the players.
     */
} GoRulesKo;


typedef enum  {
  goGameState_play, goGameState_dispute,
  goGameState_selectDead, goGameState_selectDisputed,
  goGameState_done
} GoGameState;

#define  GOGAMEFLAGS_DISPUTED      0x0001
#define  GOGAMEFLAGS_MARKDEAD      0x0002
#define  GOGAMEFLAGS_WANTDEAD      0x0004
#define  GOGAMEFLAGS_RESOLVEDEAD   0x0008
#define  GOGAMEFLAGS_RESOLVEALIVE  0x0010
#define  GOGAMEFLAGS_SEEWHITE      0x0020
#define  GOGAMEFLAGS_SEEBLACK      0x0040
#define  GOGAMEFLAGS_SEKI          0x0080
#define  GOGAMEFLAGS_NOSEKI        0x0100
#define  GOGAMEFLAGS_SPECIAL       0x0200  /* For seki detection. */
#define  GOGAMEFLAGS_EYE           0x0400  /* For seki detection. */
#define  GOGAMEFLAGS_FAKETEST      0x0800  /* For seki detection. */
#define  GOGAMEFLAGS_NOTFAKE       0x1000  /* For seki detection. */

#define  GOGAMEFLAGS_RESOLVED  (GOGAMEFLAGS_RESOLVEDEAD| \
                                GOGAMEFLAGS_RESOLVEALIVE)
#define  GOGAMEFLAGS_SEEN  (GOGAMEFLAGS_SEEBLACK|GOGAMEFLAGS_SEEWHITE)
#define  goGameFlags_see(s)  (GOGAMEFLAGS_SEEWHITE << (s))


typedef struct GoGameMove_struct  {
  int  move;
  GoStone  color;
  GoHash  hash;  /* Hash value at the beginning of this move. */
  GoTimer  time;  /* Time left at this move. */
} GoGameMove;


typedef struct  {
  int  size;
  GoRules  rules;

  /*
   * Passive and forcePlay are somewhat redundant.  I should clean them up.
   *   But basically, passive means it is controlled by the SGF, and
   *   forcePlay means go server.  Yuck.
   */
  bool  passive;
  bool  forcePlay;

  float  komi;
  GoTime  time;
  int  moveNum, maxMoves, handicap, passCount;
  GoGameMove  *moves;
  int  movesLen;
  bool  noLastMove;
  GoStone  whoseMove;

  GoBoard  *board;
  GoBoard  *tmpBoard;  /* Used for superko rule and seki checks. */
  uint  *flags;
  bool  flagsCleared;
  int  *hotMoves;
  int  cooloff;  /* The last move that cleared the kos. */
  GoGameState  state;

  bool  disputeAlive;
  int  setWhoseMoveNum;
  GoStone  setWhoseMoveColor;
  int  disputedLoc;

  MAGIC_STRUCT
} GoGame;

/**********************************************************************
 * Global variables
 **********************************************************************/
extern const bool  goRules_freeHandicaps[goRules_num];
extern const GoRulesKo  goRules_ko[goRules_num];
extern const bool  goRules_suicide[goRules_num];
extern const bool  goRules_dispute[goRules_num];
extern const bool  goRules_scoreKills[goRules_num];

/**********************************************************************
 * Functions
 **********************************************************************/
extern GoGame  *goGame_create(int size, GoRules rules, int handicap,
			      float komi, const GoTime *time, bool passive);
extern void  goGame_destroy(GoGame *game);

/*
 * Valid in the play or dispute states.
 */
/*
 * goGame_move() returns TRUE if the move caused the game's state to change.
 * Possible changes:
 *   play --> selectDead
 *   dispute --> selectDead
 *   play, dispute --> done (for time-out losses only, move will not be made)
 */
extern bool  goGame_move(GoGame *game, GoStone color, int move,
			 GoTimer *currentTime);
extern void  goGame_setBoard(GoGame *game, GoStone color, int loc);
#define  goGame_pass(g, color, t)  goGame_move((g), color, GOGAME_PASS, t)
#define  goGame_isHot(g, l)  ((g)->hotMoves[l] >= (g)->cooloff)
#define  goGame_whoseMove(g)  ((g)->whoseMove)
extern GoStone  goGame_whoseTurnOnMove(GoGame *game, int moveNum);
extern const GoTimer  *goGame_getTimer(const GoGame *game, GoStone color);

/*
 * Valid in play, dispute, or done states.  Returns TRUE if the game's state
 *   has changed, just like goGame_move.
 * Possible changes:
 *   play --> done
 *   done --> play
 */
extern bool  goGame_moveTo(GoGame *game, int moveNum);

/*
 * Valid in the selectDead state.
 */
extern void  goGame_markDead(GoGame *game, int loc);
/*
 * resume() will change state from selectDead to play.
 */
extern void  goGame_resume(GoGame *game);
/*
 * done() will change state from selectDead to done.
 */
extern void  goGame_done(GoGame *game);
/*
 * selectDisputed() will change state from selectDead to selectDisputed.
 */
#define  goGame_selectDisputed(g)  ((g)->state = goGameState_selectDisputed)

/*
 * dispute() will change state from selectDisputed to dispute.
 */
extern void  goGame_dispute(GoGame *game, int loc);

/*
 * Always valid.
 */
extern bool  goGame_isLegal(GoGame *game, GoStone moveColor, int move);
#define  goGame_caps(g, s)  goBoard_caps((g)->board, (s))
extern int  goGame_lastMove(GoGame *g);
#define  goGame_noLastMove(g)  ((g)->noLastMove = TRUE)

#endif  /* _GOGAME_H_ */
