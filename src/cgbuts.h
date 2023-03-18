/*
 * $Source: /cvsroot/cgoban1/cgoban1/src/cgbuts.h,v $
 * $Revision: 1.2 $
 * $Author: wmshub $
 * $Date: 2002/05/31 23:40:54 $
 *
 * src/cgbuts.h, part of Complete Goban (game program)
 * Copyright (C) 1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _CGBUTS_H_
#define  _CGBUTS_H_  1

#ifndef  _GOBOARD_H_
#include "goBoard.h"
#endif
#ifndef  _BUT_BUT_H_
#include <but/but.h>
#endif


/**********************************************************************
 * Constants
 **********************************************************************/
#define  CGBUTS_COLORBG1       (BUT_DCOLORS+0)
#define  CGBUTS_COLORBG2       (BUT_DCOLORS+1)
#define  CGBUTS_COLORBGLIT1    (BUT_DCOLORS+2)
#define  CGBUTS_COLORBGLIT2    (BUT_DCOLORS+3)
#define  CGBUTS_COLORBGSHAD1   (BUT_DCOLORS+4)
#define  CGBUTS_COLORBGSHAD2   (BUT_DCOLORS+5)
#define  CGBUTS_COLORREDLED    (BUT_DCOLORS+6)
#define  CGBUTS_GREY(n)        (BUT_DCOLORS+7+(n))
#define  CGBUTS_COLORBOARD(n)  (BUT_DCOLORS+7+256+(n))
#define  CGBUTS_NUMCOLORS  (COLOR_BOARD(256))


#define  CGBUTS_REWCHAR   "\1\1"
#define  CGBUTS_BACKCHAR  "\1\2"
#define  CGBUTS_FWDCHAR   "\1\3"
#define  CGBUTS_FFCHAR    "\1\4"

#define  CGBUTS_WSTONECHAR    "\1\5"
#define  CGBUTS_BSTONECHAR    "\1\6"

#define  CGBUTS_NUMWHITE  5
#define  CGBUTS_YINYANG(x)    (CGBUTS_NUMWHITE+(x))
#define  CGBUTS_WORLDWEST(x)  (CGBUTS_YINYANG(CGBUTS_NUMWHITE)+(x))
#define  CGBUTS_WORLDEAST(x)  (CGBUTS_WORLDWEST(CGBUTS_NUMWHITE)+(x))

#define  CGBUTS_HOLDTIME  500  /* ms */


/**********************************************************************
 * Data types
 **********************************************************************/
typedef struct CgbutsPics_struct  {
  Pixmap  stonePixmaps;
  Pixmap  maskBitmaps;
} CgbutsPic;


typedef struct Cgbuts_struct  {
  ButEnv  *env;
  int  dpyDepth;
  Window  dpyRootWindow;
  int  numPics;
  CgbutsPic  *pics;
  Rnd  *rnd;
  int  color;  /* TrueColor, PseudoColor, or GrayScale. */
  bool  hiContrast;
  bool  holdEnabled;
  Time  holdStart;
  int timeWarn;
  
  MAGIC_STRUCT
} Cgbuts;


typedef enum  {
  goMark_none,   goMark_lastMove, goMark_ko,
  goMark_triangle, goMark_x, goMark_square, goMark_circle,
  goMark_letter, goMark_number, goMark_smWhite, goMark_smBlack
} GoMarkType;
#define  goMark_max  (goMark_ko + 1)


typedef enum  {
  gridLines_ul,   gridLines_top,    gridLines_ur,
  gridLines_left, gridLines_center, gridLines_right,
  gridLines_ll,   gridLines_bottom, gridLines_lr,
  gridLines_none
} GridLines;


typedef struct Grid_struct  {
  Cgbuts  *b;
  ButOut (*callback)(But *but);
  GridLines  lineGroup;
  bool  starPoint;
  int  pos;
  GoStone  ptype;  /* The piece at this point on the board. */
  GoStone  pressColor;  /* The color to add when this button is pressed. */
  int  stoneVersion;
  bool  grey, holdRequired;
  GoMarkType  markType;
  int  markAux;

  MAGIC_STRUCT
} Grid;


/**********************************************************************
 * Global Variables
 **********************************************************************/
extern const char  *cgbuts_stoneChar[2];

/**********************************************************************
 * Functions
 **********************************************************************/
extern void  cgbuts_init(Cgbuts *cgbuts, ButEnv *env, Rnd *rnd, int color,
			 bool hold, bool hiContrast, int timeWarn);
extern But  *grid_create(Cgbuts *b, ButOut (*func)(But *but), void *packet,
			 ButWin *win, int layer, int flags, int pos);
extern void  grid_setStone(But *but, GoStone piece, bool grey);
extern void  grid_setMark(But *but, GoMarkType markType, int markAux);
extern void  grid_setPress(But *but, GoStone pressColor);
#define  grid_setHold(b, h)  (((Grid *)((b)->iPacket))->holdRequired = (h))
#define  grid_pressColor(b)  (((Grid *)((b)->iPacket))->pressColor)
#define  grid_setLineGroup(b, g)  (((Grid *)((b)->iPacket))->lineGroup = (g))
#define  grid_setStarPoint(b, s)  (((Grid *)((b)->iPacket))->starPoint = (s))
#define  grid_pos(b)  (((Grid *)((b)->iPacket))->pos)
#define  grid_stone(b)  (((Grid *)((b)->iPacket))->ptype)
#define  grid_grey(b)  (((Grid *)((b)->iPacket))->grey)
#define  grid_markType(b)  (((Grid *)((b)->iPacket))->markType)
#define  grid_markAux(b)  (((Grid *)((b)->iPacket))->markAux)
#define  grid_setVersion(b, v)  (((Grid *)((b)->iPacket))->stoneVersion = v)
#define  goMarkType_stone2sm(s)  \
  ((GoMarkType)((s - goStone_white) + goMark_smWhite))

extern But  *gobanPic_create(Cgbuts *b, ButWin *win, int layer, int flags);
extern But  *stoneGroup_create(Cgbuts *b, ButWin *win, int layer, int flags);
extern But  *computerPic_create(Cgbuts *b, ButWin *win, int layer, int flags);
extern But  *toolPic_create(Cgbuts *b, ButWin *win, int layer, int flags);
extern void  cgbuts_redraw(Cgbuts *b);

/* Functions only for "friend" routines. */
extern void  cgbuts_drawp(Cgbuts *b, ButWin *win, GoStone piece, bool grey,
			  int x, int y, int size, int stoneVersion,
			  int dx, int dy, int dw, int dh);
extern void  cgbuts_markPiece(Cgbuts *b, ButWin *win, GoStone piece,
			      GoMarkType markType, int markAux,
			      int x, int y, int w, int stoneVersion,
			      int dx,int dy, int dw,int dh);

#endif  /* _CGBUTS_H_ */
