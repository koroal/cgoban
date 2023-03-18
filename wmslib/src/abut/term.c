/*
 * wmslib/src/abut/term.c, part of wmslib (Library functions)
 * Copyright (C) 1995-1996 William Shubert
 * See "configure.h.in" for more copyright information.
 *
 * Source code for "term" scrolling terminal-like widgety thing.
 */


#include <wms.h>
#include <but/but.h>
#include <but/plain.h>
#include <but/tbin.h>
#include <but/slide.h>
#include <but/canvas.h>
#ifdef  _ABUT_TERM_H_
  Levelization Error.
#endif
#include "term.h"


/**********************************************************************
 * Forward declarations
 **********************************************************************/
static ButWinFunc  resize;
static void  curOffWin(But *but, int activeLine, int passiveLine,
		       int mouseY);


/**********************************************************************
 * Functions
 **********************************************************************/
AbutTerm  *abutTerm_create(Abut *abut, ButWin *parent,
			   int layer, bool editable)  {
  AbutTerm  *term;

  assert(MAGIC(abut));
  term = wms_malloc(sizeof(AbutTerm));
  MAGIC_SET(term);
  term->abut = abut;
  term->swin = abutSwin_create(term, parent, layer,
			       BUT_DRAWABLE|BUT_PRESSABLE, resize);
  if (editable)
    term->bg = butPlain_create(term->swin->win, 0, BUT_DRAWABLE,
			       BUT_HIBG);
  else
    term->bg = butPlain_create(term->swin->win, 0, BUT_DRAWABLE,
			       BUT_BG);
  term->tbin = butTbin_create(term->swin->win, 1,
			      BUT_DRAWABLE|BUT_PRESSABLE, "");
  if (!editable)
    butTbin_setReadOnly(term->tbin, TRUE);
  butTbin_setOffWinCallback(term->tbin, curOffWin);
  term->state = abutTermState_steady;
  return(term);
}


void  abutTerm_destroy(AbutTerm *term)  {
  assert(MAGIC(term));
  MAGIC_UNSET(term);
  abutSwin_destroy(term->swin);
  wms_free(term);
}


void  abutTerm_set(AbutTerm *term, const char *text)  {
  ButEnv  *env;

  assert(MAGIC(term));
  env = term->abut->env;
  butTbin_set(term->tbin, text);
  butCan_resizeWin(term->swin->win, 0, butTbin_numLines(term->tbin) *
		   butEnv_fontH(env, 0) + butEnv_stdBw(env) * 2, TRUE);
}


void  abutTerm_resize(AbutTerm *term, int x, int y, int w, int h)  {
  int  fontH = butEnv_fontH(term->abut->env, 0);
  int  bw = butEnv_stdBw(term->abut->env);

  assert(MAGIC(term));
  abutSwin_resize(term->swin, x, y, w, h, (fontH * 3) / 2, fontH);
  butCan_resizeWin(term->swin->win, 0, butTbin_numLines(term->tbin) * fontH +
		   bw * 2, TRUE);
}


static ButOut  resize(ButWin *win)  {
  AbutSwin  *swin = butWin_packet(win);
  AbutTerm  *term = swin->packet;
  char  *temp;
  int  w, h, len;
  int  bw;

  assert(MAGIC(swin));
  assert(MAGIC(term));
  w = butWin_w(win);
  h = butWin_h(win);
  but_resize(term->bg, 0,0, w,h);
  bw = butEnv_stdBw(term->abut->env);
  if (but_w(term->tbin) == w)  {
    but_resize(term->tbin, 0,bw, w,h - bw);
  } else  {
    /*
     * This is a mega-cheesy kludge to work around the non-resizability of
     *   a tbin widget.  Basically I cut the whole terminal script, clear it,
     *   resize the tbin, then paste it all back in.  Ugh.
     */
    len = butTbin_len(term->tbin);
    temp = wms_malloc(len + 1);
    memcpy(temp, butTbin_get(term->tbin), len);
    temp[len] = '\0';
    butTbin_set(term->tbin, "");
    but_resize(term->tbin, 0,bw, w,h - bw);
    butTbin_set(term->tbin, temp);
    wms_free(temp);
  }
  return(0);
}


static void  curOffWin(But *but, int activeLine, int passiveLine,
		       int mouseY)  {
  ButWin  *win = but_win(but);
  ButEnv  *env = butWin_env(win);
  AbutSwin  *swin = butWin_packet(win);
  AbutTerm  *term;
  int  y, h, bw = butEnv_stdBw(env);
  int  lines;
  AbutTermState  newState;

  assert(MAGIC(swin));
  term = swin->packet;
  assert(MAGIC(term));
  newState = abutTermState_steady;
  h = butEnv_fontH(env, 0);
  y = activeLine * h + bw;
  if (passiveLine != activeLine)  {
    if (y <= butCan_yOff(win) + bw)  {
      if (mouseY < butCan_yOff(win) - h)
	newState = abutTermState_fastUp;
      else if (mouseY < butCan_yOff(win))
	newState = abutTermState_slowUp;
    } else if (y + h + bw >= butCan_yOff(win) + butWin_viewH(win))  {
      if (mouseY > butCan_yOff(win) + butWin_viewH(win) + h)
	newState = abutTermState_fastDown;
      else if (mouseY > butCan_yOff(win) + butWin_viewH(win))
	newState = abutTermState_slowDown;
    }
  }
  if (newState != term->state)  {
    term->state = newState;
    switch(newState)  {
    case abutTermState_fastUp:
      butSlide_startSlide(swin->vSlide, FALSE, -h*10, FALSE);
      break;
    case abutTermState_slowUp:
      butSlide_startSlide(swin->vSlide, FALSE, -h, FALSE);
      break;
    case abutTermState_steady:
      butSlide_stopSlide(swin->vSlide);
      break;
    case abutTermState_slowDown:
      butSlide_startSlide(swin->vSlide, FALSE, h, FALSE);
      break;
    case abutTermState_fastDown:
      butSlide_startSlide(swin->vSlide, FALSE, h*10, FALSE);
      break;
    }
  }
  if (passiveLine == activeLine)  {
    lines = butTbin_numLines(but);
    if (activeLine >= lines)
      lines = activeLine + 1;
    if (lines * h + bw * 2 > butWin_h(win))  {
      butCan_resizeWin(swin->win, 0, lines * h + bw * 2, TRUE);
    }
    if (y < butCan_yOff(win))  {
      y -= bw;
      butCan_slide(win, BUT_NOCHANGE, y, TRUE);
    } else if (y + h > butCan_yOff(win) + butWin_viewH(win))  {
      y = y + h + bw - butWin_viewH(win);
      butCan_slide(win, BUT_NOCHANGE, y, TRUE);
    }
  }
}
