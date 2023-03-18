/*
 * $Source: /cvsroot/cgoban1/cgoban1/src/goban.h,v $
 * $Revision: 1.2 $
 * $Date: 2002/02/28 03:34:35 $
 *
 * src/goban.h, part of Complete Goban (game program)
 * Copyright (C) 1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _GOBAN_H_
#define  _GOBAN_H_  1

#ifndef  _ABUT_TERM_H_
#include <abut/term.h>
#endif
#ifndef  _GOGAME_H_
#include "goGame.h"
#endif
#ifndef  _GOPIC_H_
#include "goPic.h"
#endif
#ifndef  _GOSCORE_H_
#include "goScore.h"
#endif
#ifndef  _ABUT_MSG_H_
#include <abut/msg.h>
#endif


/**********************************************************************
 * Data types
 **********************************************************************/
typedef enum  {
  gobanOut_draw, gobanOut_noDraw, gobanOut_err
} GobanOut;

typedef struct GobanActions_struct  {
  /*
   * Callbacks when buttons are pressed.  Returns TRUE if you have to
   *   update the grid afterwards.
   */
  GobanOut  (*gridPressed)(void *packet, int loc);
  GobanOut  (*quitPressed)(void *packet);
  GobanOut  (*passPressed)(void *packet);
  GobanOut  (*rewPressed)(void *packet);
  GobanOut  (*backPressed)(void *packet);
  GobanOut  (*fwdPressed)(void *packet);
  GobanOut  (*ffPressed)(void *packet);
  GobanOut  (*donePressed)(void *packet);
  GobanOut  (*disputePressed)(void *packet);
  GobanOut  (*savePressed)(void *packet);
  GobanOut  (*editPressed)(void *packet);
  GobanOut  (*gameInfoPressed)(void *packet);
  Help  *helpInfo;

  /*
   * Called when goban_destroy is called with propagate set to TRUE, or
   *   when the window is suddenly destroyed.
   */
  void  (*destroyCallback)(void *packet);

  /* Returns TRUE if the specified button should be pressable. */
  bool  (*rewOk)(void *packet);
  bool  (*backOk)(void *packet);
  bool  (*fwdOk)(void *packet);
  bool  (*ffwdOk)(void *packet);
} GobanActions;
  

typedef struct GobanPlayerInfo_struct  {
  But  *box, *stones[5], *capsLabel, *capsOut, *timeLabel, *timeOut;
  But  *capsBox, *timeBox;
} GobanPlayerInfo;


typedef struct Goban_struct  {
  Cgoban  *cg;
  const GobanActions  *actions;
  void  *packet;
  bool  propagateDestroy;

  GoGame  *game;
  GoTimer  timers[2];
  GoStone  activeTimer;
  GoScore  score;
  int  ingTimePenalties[2];
  int  lastMove;
  float  lastScores[2];
  ClpEntry  *bProp, *bPropW, *clpX, *clpY;

  ButWin  *win;
  But  *bg;
  But  *labelBox, *labelText;
  But  *infoText;
  But  *gameInfo;
  But  *help;
  AbutTerm  *comments;
  But  *kibIn, *kibType, *kibSay, *kibBoth, *kibKib;
  GobanOut  (*newKib)(void *packet, const char *txt);

  But  *pass;
  But  *resume;
  But  *done;

  GobanPlayerInfo  playerInfos[2];

  But  *save;
  But  *edit;
  But  *quit;
  But  *rew, *back, *fwd, *ff;

  GoPic  *pic;
  But  *boardBox;

  ButWin  *iWin;
  But  *iBg, *iBoard, *iDec1, *iDec2;  /* Icon Decorations. */

  AbutMsg  *msgBox;
  ButTimer  *clock;

  MAGIC_STRUCT
} Goban;


/**********************************************************************
 * Functions
 **********************************************************************/
extern Goban  *goban_create(Cgoban *cg, const GobanActions *actions,
			    void *packet, GoGame *game, const char *bPropName,
			    const char *title);
#define  goban_setTitle(g, n)  butText_set((g)->labelText, (n))
#define  goban_getComments(g)  abutTerm_get((g)->comments)
#define  goban_setComments(g, c)  abutTerm_set((g)->comments, (c))
#define  goban_catComments(g, c)  butTbin_insert((g)->comments->tbin, (c))
extern void  goban_destroy(Goban *g, bool propagate);
extern void  goban_message(Goban *g, const char *message);
extern void  goban_noMessage(Goban *g);
extern void  goban_update(Goban *g);
extern void  goban_startTimer(Goban *g, GoStone whose);
/* stopTimer returns TRUE if there is still time left. */
extern bool  goban_stopTimer(Goban *g);
#define  goban_setHold(g, h)  goPic_setButHold((g)->pic, h)
#define  goban_score(g, p)  ((g)->score.scores[p] - (g)->ingTimePenalties[p])
#define  goban_dame(g)  ((g)->score.dame)
#define  goban_territory(g, p)  ((g)->score.territories[p])
#define  goban_livingStones(g, p)  ((g)->score.livingStones[p])
#define  goban_caps(g, p)  (goGame_caps((g)->game, p) + \
                            (g)->score.deadStones[p])
extern void  goban_setKibitz(Goban *g, GobanOut (*newKib)(void *packet,
							  const char *txt),
			     int kibVal);
#define  goban_kibType(g)  butRadio_get((g)->kibType)
extern void  goban_updateTimeReadouts(Goban *g);

#endif  /* _GOBAN_H_ */
