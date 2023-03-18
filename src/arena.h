/*
 * src/arena.h, part of Complete Goban (game program)
 * Copyright (C) 1996 William Shubert.
 * See "configure.h.in" for more copyright information.
 */

#ifndef  _ARENA_H_

#ifndef  _GMP_ENGINE_H_
#include "gmp/engine.h"
#endif
#ifndef  _GOGAME_H_
#include "goGame.h"
#endif

#ifdef  _ARENA_H_
  Levelization Error.
#endif
#define  _ARENA_H_  1


/**********************************************************************
 * Data Types
 **********************************************************************/
typedef struct Arena_struct  {
  Cgoban  *cg;
  GmpEngine  engines[2];
  GoGame  *game;

  MAGIC_STRUCT
} Arena;


/**********************************************************************
 * Functions
 **********************************************************************/

extern void  arena(Cgoban *cg, const char *prog1, const char *prog2,
		   int size, float komi, int handicap);

#endif
