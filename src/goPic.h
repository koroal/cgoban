/*
 * src/gopic.h, part of Complete Goban (game program)
 * Copyright (C) 1995-1996 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _GOPIC_H_

#ifndef  _GOGAME_H_
#include "goGame.h"
#endif
#ifndef  _CGOBAN_H_
#include "cgoban.h"
#endif

#ifdef  _GOPIC_H_
  Levelization Error
#endif
#define  _GOPIC_H_  1


/**********************************************************************
 * Data types
 **********************************************************************/
typedef enum GoPicMoves_enum {
  goPicMove_legal = 0x01,
  goPicMove_empty = 0x02,
  goPicMove_stone = 0x04,
  goPicMove_forceWhite = 0x08,
  goPicMove_forceBlack = 0x10,
  goPicMove_noPass = 0x20,
  goPicMove_noWhite = 0x40,
  goPicMove_noBlack = 0x80
} GoPicMoves;

typedef struct GoPic_struct  {
  Cgoban  *cg;
  GoGame  *game;
  ButOut  (*press)(void *packet, int loc);
  void  *packet;
  int  size;
  /* You must call goPic_update after changing this value. */
  GoPicMoves  allowedMoves;

  But  *boardBg;
  But  **boardButs;
  But  **letters, **numbers;

  MAGIC_STRUCT
} GoPic;


/**********************************************************************
 * Functions
 **********************************************************************/
extern GoPic  *goPic_create(Cgoban *cg, void *packet, GoGame *game,
			    ButWin *win, int layer,
			    ButOut pressed(void *packet, int loc), int size);
extern void  goPic_destroy(GoPic *p);
extern void  goPic_resize(GoPic *p, int x, int y, int w, int h);
extern void  goPic_setButHold(GoPic *p, bool hold);
/*
 * Returns the color whose move it is.
 */
extern GoStone  goPic_update(GoPic *p);


#endif  /* _GOPIC_H_ */
