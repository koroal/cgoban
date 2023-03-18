/*
 * src/lsetup.h, part of Complete Goban (game program)
 * Copyright (C) 1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _GAMESETUP_H_
#define  _GAMESETUP_H_  1

#ifndef  _GOGAME_H_
#include "goGame.h"
#endif
#ifndef  _GOTIME_H_
#include "goTime.h"
#endif


/**********************************************************************
 * Data types
 **********************************************************************/
typedef struct GameSetup_struct  {
  Cgoban  *cg;

  void  (*callback)(void *packet, struct GameSetup_struct *gs);
  void  *packet;

  ButWin  *win;
  But  *bg;
  But  *title;

  But  *namesBox;
  But  *wStr, *wIn;
  But  *bStr, *bIn;

  But  *rulesBox;
  But  *rulesMenu;
  But  *sizeStr, *sizeIn;
  But  *hcapStr, *hcapIn;
  But  *komiStr, *komiIn;
  GoRules  rules;
  int  size, hcap;
  float  komi;
  bool  sizeSet, hcapSet, komiSet;

  But  *timeBox;
  But  *timeMenu;
  But  *mainStr, *mainIn;
  But  *byStr,  *byIn;
  But  *auxStr, *auxIn;
  GoTimeType  timeType;
  int  numTimeArgs, mainTime, byTime, aux;

  But  *help, *ok, *cancel;
  MAGIC_STRUCT
} GameSetup;


/**********************************************************************
 * Functions
 **********************************************************************/
extern GameSetup  *gameSetup_create(Cgoban *cg,
				    void (*callback)(void *packet,
						     GameSetup *gs),
				    void *packet);
extern void  gameSetup_destroy(GameSetup *ls);
#define  gameSetup_rules(gs)     ((gs)->rules)
#define  gameSetup_size(gs)      ((gs)->size)
#define  gameSetup_handicap(gs)  ((gs)->hcap)
#define  gameSetup_komi(gs)      ((gs)->komi)
#define  gameSetup_wName(gs)     (butTextin_get((gs)->wIn))
#define  gameSetup_bName(gs)     (butTextin_get((gs)->bIn))
#define  gameSetup_timeType(gs)  ((gs)->timeType)
#define  gameSetup_mainTime(gs)  ((gs)->mainTime)
#define  gameSetup_byTime(gs)    ((gs)->byTime)
#define  gameSetup_timeAux(gs)   ((gs)->aux)


#endif  /* _GAMESETUP_H_ */
