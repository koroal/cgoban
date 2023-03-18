/*
 * src/crwin.c, part of Complete Goban (game program)
 * Copyright (C) 1995 William Shubert
 * See "configure.h.in" for more copyright information.
 */

#include <wms.h>
#include <but/but.h>
#include <but/text.h>
#include <but/tblock.h>
#include <but/box.h>
#include <but/timer.h>
#include "cgoban.h"
#include "msg.h"
#include "crwin.h"


/**********************************************************************
 * Forward declarations
 **********************************************************************/
static void  destroyed(But *but);
static ButOut  killCr(ButTimer *timer);


/**********************************************************************
 * Functions
 **********************************************************************/
Crwin  *crwin_create(Cgoban *cg, ButWin *win, int layer)  {
  Crwin  *cr;
  ButEnv  *env;
  int  x, y, w, h, textH;
  struct timeval  fiveSeconds;

  assert(MAGIC(cg));
  cr = wms_malloc(sizeof(Crwin));
  MAGIC_SET(cr);
  cr->env = env = cg->env;
  cr->box = butBoxFilled_create(win, layer, BUT_DRAWABLE);
  but_setPacket(cr->box, cr);
  but_setDestroyCallback(cr->box, destroyed);
  cr->title = butText_create(win, layer+1, BUT_DRAWABLE,
			     "CGoban " VERSION, butText_center);
  butText_setFont(cr->title, 2);
  cr->byBill = butTblock_create(win, layer+1, BUT_DRAWABLE,
				msg_byBillShubert, butText_center);
  cr->noWarr = butTblock_create(win, layer+1, BUT_DRAWABLE,
				msg_noWarranty, butText_center);
  cr->seeHelp = butTblock_create(win, layer+1, BUT_DRAWABLE,
				 msg_seeHelp, butText_center);

  textH = butEnv_fontH(env, 0);
  h = textH * 8;
  w = h * 2;
  x = (butWin_w(win) - w) / 2;
  y = (butWin_h(win) - h) / 3;
  but_resize(cr->box, x,y, w,h);
  butText_resize(cr->title, x + w/2, y += textH, textH);
  x += butEnv_stdBw(env) * 2;
  w -= butEnv_stdBw(env) * 4;
  y += textH;
  y += butTblock_resize(cr->byBill, x, y, w);
  y += butTblock_resize(cr->noWarr, x, y, w);
  butTblock_resize(cr->seeHelp, x, y, w);

  fiveSeconds.tv_usec = 0;
  fiveSeconds.tv_sec = 5;
  cr->timer = butTimer_create(cr, cr->box, fiveSeconds, fiveSeconds, FALSE,
			      killCr);

  return(cr);
}


static void  destroyed(But *but)  {
  Crwin  *cr = but_packet(but);

  assert(MAGIC(cr));
  MAGIC_UNSET(cr);
  wms_free(cr);
}


static ButOut  killCr(ButTimer *timer)  {
  Crwin  *cr = butTimer_packet(timer);

  assert(MAGIC(cr));
  butTimer_destroy(cr->timer);
  but_destroy(cr->title);
  but_destroy(cr->byBill);
  but_destroy(cr->noWarr);
  but_destroy(cr->seeHelp);
  /*
   * Kill the box last.  This is important, because killing the box
   *   triggers the destruction of the Crwin data structure.
   */
  but_destroy(cr->box);
  return(0);
}
