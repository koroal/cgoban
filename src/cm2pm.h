/*
 * src/cm2pm.h, part of Complete Goban (game program)
 * Copyright (C) 1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _CM2PM_H_
#define  _CM2PM_H_  1

/**********************************************************************
 * Constants
 **********************************************************************/

/**********************************************************************
 * Functions
 **********************************************************************/
extern Pixmap  cm2Pm(ButEnv *env, uchar *cmap, uint cmapw,uint cmaph,
		     uint w,uint h,
		     int pic0, int npic, bool flat);
extern void  cm2OldPm(ButEnv *env, uchar *cmap, uint cmapw,uint cmaph,
		      int pic0, int npic, Pixmap pic, bool flat);

#endif  /* _CM2PM_H_ */
