/*
 * src/drawStone.h, part of Complete Goban (game program)
 * Copyright (C) 1996 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _DRAWSTONE_H_

#ifdef  _DRAWSTONE_H_
  Levelization Error.
#endif
#define  _DRAWSTONE_H_  1


/**********************************************************************
 * Constants
 **********************************************************************/
#define  DRAWSTONE_NUMWHITE  5
#define  DRAWSTONE_YINYANG(x)    (DRAWSTONE_NUMWHITE+(x))
#define  DRAWSTONE_WORLDWEST(x)  (DRAWSTONE_YINYANG(DRAWSTONE_NUMWHITE)+(x))
#define  DRAWSTONE_WORLDEAST(x)  (DRAWSTONE_WORLDWEST(DRAWSTONE_NUMWHITE)+(x))


/**********************************************************************
 * Functions
 **********************************************************************/
extern void  drawStone_newPics(ButEnv *env, Rnd *rnd, int baseColor,
			       Pixmap *stonePixmap, Pixmap *maskBitmap,
			       int size, bool color);


#endif  /* _DRAWSTONE_H_ */
