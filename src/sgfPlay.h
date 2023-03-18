/*
 * src/sgfPlay.h, part of Complete Goban (game program)
 * Copyright (C) 1995-1996 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _SGFPLAY_H_

#ifndef  _SGF_H_
#include "sgf.h"
#endif
#ifndef  _GOGAME_H_
#include "goGame.h"
#endif
#ifndef  _GOPIC_H_
#include "goPic.h"
#endif

#ifdef  _SGFPLAY_H_
  Levelization Error.
#endif
#define  _SGFPLAY_H_  1


/**********************************************************************
 * Functions
 **********************************************************************/
/*
 * sgf_play will play until node terminator is hit or numNodes are passed.
 * If numNodes is -1, then it is played to the end.
 * In any case, it returns the node number it played to.  This is
 *   probably only useful if "numNodes" was -1.
 * If GoPic is non-NULL, then it will set the marks on the stones
 *   appropriately.
 */
extern int  sgf_play(Sgf *mc, GoGame *game, GoPic *pic, int numNodes,
		     SgfElem *terminator);


#endif  /* _SGFPLAY_H_ */
