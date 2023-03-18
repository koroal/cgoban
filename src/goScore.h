/*
 * src/goScore.h, part of Complete Goban (game program)
 * Copyright (C) 1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _GOSCORE_H_
#define  _GOSCORE_H_  1

#ifndef  _GOGAME_H_
#include "goGame.h"
#endif


/**********************************************************************
 * Data types
 **********************************************************************/

typedef struct GoScore_struct  {
  int  territories[2], livingStones[2], deadStones[2], dame;
  float  scores[2];
} GoScore;


/**********************************************************************
 * Functions
 **********************************************************************/
extern void  goScore_compute(GoScore *score, GoGame *game);
extern void  goScore_zero(GoScore *score);
extern Str  *goScore_str(GoScore *score, GoGame *game,
			 const GoTime *time, const GoTimer *timers);


#endif  /* _GOSCORE_H_ */
