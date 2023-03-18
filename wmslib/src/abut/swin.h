/*
 * wmslib/src/abut/swin.h, part of wmslib (Library functions)
 * Copyright (C) 1994-1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 *
 * Includes for auxiliary button code.
 */

#ifndef  _ABUT_SWIN_H_
#define  _ABUT_SWIN_H_  1

#ifndef  _WMS_H_
#include <wms.h>
#endif
#ifndef  _BUT_BUT_H_
#include <but/but.h>
#endif


/**********************************************************************
 * Constants
 **********************************************************************/
#define  ABUT_UPPIC     "\2\1"
#define  ABUT_DOWNPIC   "\2\2"
#define  ABUT_LEFTPIC   "\2\3"
#define  ABUT_RIGHTPIC  "\2\4"


/**********************************************************************
 * Data types
 **********************************************************************/
typedef struct AbutSwin_struct  {
  ButWin  *win;
  void  *packet;
  But  *up, *down, *left, *right, *hSlide, *vSlide, *box;
  float  xCenter, yCenter;
  uint  flags;
  ButTimer  *timer;
  int  x, y, w, h, slideW, lineH;  /* Backups for resizing. */
  int  canButW, canButH;
  MAGIC_STRUCT
} AbutSwin;
#define  ABUTSWIN_LSLIDE  0x1
#define  ABUTSWIN_RSLIDE  0x2
#define  ABUTSWIN_TSLIDE  0x4
#define  ABUTSWIN_BSLIDE  0x8


/**********************************************************************
 * Functions
 **********************************************************************/
extern AbutSwin  *abutSwin_create(void *packet, ButWin *parent,
				  int layer, uint flags,
				  ButWinFunc resize);
extern void  abutSwin_resize(AbutSwin *swin, int x, int y, int w, int h,
			     int slidew, int lineh);
extern void  abutSwin_destroy(AbutSwin *swin);
extern void  abutSwin_vMove(AbutSwin *swin, int newLoc);


#endif  /* _ABUT_SWIN_H_ */
