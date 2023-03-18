/*
 * src/help.h, part of wmslib library
 * Copyright (C) 1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 */

#ifndef  _ABUT_MSG_H_
#define  _ABUT_MSG_H_  1

#ifndef  _WMS_H_
#include <wms.h>
#endif
#ifndef  _BUT_BUT_H_
#include <but/but.h>
#endif
#ifndef  _BUT_BOX_H_
#include <but/box.h>
#endif
#ifndef  _ABUT_ABUT_H_
#include <abut/abut.h>
#endif


/**********************************************************************
 * Types
 **********************************************************************/

/* Public. */
typedef struct AbutMsgOpt_struct  {
  const char  *name;
  ButOut  (*callback)(But *but);
  void  *packet;
  const ButKey  *keyEq;
} AbutMsgOpt;


typedef struct AbutMsgTin_struct  {
  const char  *name;
  const char  *def;
  ButOut  (*callback)(But *but, const char *value);
  uint  flags;
} AbutMsgTin;

#define  abutMsgTinFlags_secret  0x1


/* Private. */
typedef struct AbutMsg_struct  {
  Abut  *abut;
  ButWin  *win;
  int  numButs, numTins;
  But  **buts, **tins, **tinTitles;
  const AbutMsgOpt  *butDesc;
  ButOut  (*destroy)(void *packet);
  void  *packet;
  int  layer;
  int  w, h, textH;
  But  *box, *text;
  MAGIC_STRUCT
} AbutMsg;


/**********************************************************************
 * Functions available
 **********************************************************************/
extern AbutMsg  *abutMsg_inCreate(Abut *a, ButWin *win, int layer,
				  const char *text,
				  void *packet, int numTins,
				  const AbutMsgTin *tinList);
extern AbutMsg  *abutMsg_optInCreate(Abut *a, ButWin *win, int layer,
				     const char *text,
				     ButOut (*destroy)(void *packet),
				     void *packet, int numButs,
				     const AbutMsgOpt *optList,
				     int numTins,
				     const AbutMsgTin *tinList);
extern AbutMsg  *abutMsg_winInCreate(Abut *a, const char *title,
				     const char *text,
				     void *packet, int numTins,
				     const AbutMsgTin *tinList);
extern AbutMsg  *abutMsg_winOptInCreate(Abut *a, const char *title,
					const char *text,
					ButOut (*destroy)(void *packet),
					void *packet, int numButs,
					const AbutMsgOpt *optList,
					int numTins,
					const AbutMsgTin *tinList);
#define  abutMsg_create(a, w, l, t)  abutMsg_inCreate(a, w, l, t, NULL,0,NULL)
#define  abutMsg_optCreate(a, w, l, t, d, p, nb, b)  \
  abutMsg_optInCreate(a, w, l, t, d, p, nb, b, 0,NULL)
#define  abutMsg_winCreate(a, ti, t)  \
  abutMsg_winInCreate(a, ti, t, NULL,0,NULL)
#define  abutMsg_winOptCreate(a, ti, t, d, p, nb, b)  \
  abutMsg_winOptInCreate(a, ti, t, d, p, nb, b, 0,NULL)
extern void  abutMsg_destroy(AbutMsg *msg, bool propagate);

#define  abutMsg_setColors(m, ul, lr, f)  \
           butBoxFilled_setColors((m)->box, (ul), (lr), (f))
#define  abutMsg_setPixmaps(m, ul, lr, f)  \
           butBoxFilled_setPixmaps((m)->box, (ul), (lr), (f))
#define  abutMsg_tin(m, t)  ((m)->tins[t])

#endif  /* _ABUT_MSG_H_ */
