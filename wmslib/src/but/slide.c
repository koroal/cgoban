/*
 * wmslib/src/but/slide.c, part of wmslib (Library functions)
 * Copyright (C) 1994-1997 William Shubert.
 * See "configure.h.in" for more copyright information.
 */

#include <configure.h>

#ifdef  X11_DISP

#ifdef  STDC_HEADERS
#include <stdlib.h>
#include <unistd.h>
#endif  /* STDC_HEADERS */
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <sys/time.h>
#ifdef  HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#include <wms.h>
#include <but/but.h>
#include <but/slide.h>
#include <but/box.h>
#include <but/timer.h>


/**********************************************************************
 * Data Types
 **********************************************************************/
typedef struct Slide_struct  {
  int  maxval;
  int  cval;
  int  size;
  bool  horiz;
  bool  grip;
  int  start_mloc, start_val, start_loc;
  ButTimer  *timer;
  int  timer_jump;
  int  oldcur;

  /*
   * Stuff calculated from dimensions.
   * w and h are for a horizontal slider.  Reverse for vertical!
   */
  int  boxW, boxH, boxLoc;
  int  lastW, lastH;

  ButOut  (*callback)(But *but, int setting, bool newPress);
} Slide;


/**********************************************************************
 * Forward Declarations
 **********************************************************************/
static ButOut  mmove(But *but, int x, int y);
static ButOut  mleave(But *but);
static ButOut  mpress(But *but, int butnum, int x, int y);
static ButOut  mrelease(But *but, int butnum, int x, int y);
static void  draw(But *but, int x, int y, int w, int h);
static ButOut  destroy(But *but);
static void  flags(But *but, uint flags);
static ButOut  slide_now(ButTimer *bt);
static void  specialDraw(But *but, Slide *slide, int x, int y, int w, int h);
static void  drawNewLoc(But *but);
static void  calcBoxLoc(But *but);


/**********************************************************************
 * Globals
 **********************************************************************/
static const ButAction  action = {
  mmove, mleave, mpress, mrelease,
  NULL, NULL, draw, destroy, flags, NULL};


/**********************************************************************
 * Functions
 **********************************************************************/
But  *butSlide_hCreate(ButOut (*func)(But *but, int setting, bool newPress),
		       void *packet, ButWin *win, int layer, int flags,
		       int maxval, int cval, int size)  {
  Slide  *slide;
  But  *but;

  slide = wms_malloc(sizeof(Slide));
  but = but_create(win, slide, &action);
  but->uPacket = packet;
  but->layer = layer;
  but->flags = flags | BUT_OPAQUE;

  slide->maxval = maxval;
  slide->cval = cval;
  slide->size = size;
  slide->horiz = TRUE;
  slide->grip = FALSE;
  slide->start_mloc = 0;
  slide->start_val = 0;
  slide->start_loc = 0;
  slide->timer = NULL;
  slide->timer_jump = 0;
  slide->oldcur = butCur_idle;
  slide->boxW = slide->boxH = slide->boxLoc = 0;
  slide->lastW = 0;
  slide->lastH = 0;
  slide->callback = func;
  but_init(but);
  return(but);
}


But  *butSlide_vCreate(ButOut (*func)(But *but, int setting, bool newPress),
		       void *packet, ButWin *win, int layer, int flags,
		       int maxval, int cval, int size)  {
  Slide  *slide;
  But  *but;

  slide = wms_malloc(sizeof(Slide));
  but = but_create(win, slide, &action);
  but->uPacket = packet;
  but->layer = layer;
  but->flags = flags | BUT_OPAQUE;

  slide->maxval = maxval;
  slide->cval = cval;
  slide->size = size;
  slide->horiz = FALSE;
  slide->grip = FALSE;
  slide->start_mloc = 0;
  slide->start_val = 0;
  slide->start_loc = 0;
  slide->timer = NULL;
  slide->timer_jump = 0;
  slide->oldcur = butCur_idle;
  slide->boxW = slide->boxH = slide->boxLoc = 0;
  slide->lastW = 0;
  slide->lastH = 0;
  slide->callback = func;
  but_init(but);
  return(but);
}


void  butSlide_set(But *but, int maxval, int cval, int size)  {
  Slide  *slide = but->iPacket;

  assert(but->action == &action);
  if (maxval == BUT_NOCHANGE)
    maxval = slide->maxval;
  if (cval == BUT_NOCHANGE)
    cval = slide->cval;
  if (size == BUT_NOCHANGE)
    size = slide->size;
  if ((maxval != slide->maxval) || (cval != slide->cval) ||
      (size != slide->size))  {
    slide->maxval = maxval;
    slide->cval = cval;
    slide->size = size;
    drawNewLoc(but);
  }
}


int  butSlide_get(But *but)  {
  Slide  *slide = but->iPacket;

  return(slide->cval);
}


static ButOut  destroy(But *but)  {
  Slide *slide = but->iPacket;
  
  if (slide->timer != NULL)
    butTimer_destroy(slide->timer);
  wms_free(slide);
  return(0);
}


static void  draw(But *but, int x, int y, int w, int h)  {
  ButWin *win = but->win;
  ButEnv *env = win->env;
  Slide  *slide = but->iPacket;
  int  bw = env->stdButBw;

  if ((w != slide->lastW) || (h != slide->lastH))
    calcBoxLoc(but);
  but_drawBox(win, but->x,but->y, but->w,but->h, 1, bw,
	      BUT_SLEFT|BUT_SRIGHT, BUT_LIT, BUT_SHAD, None, None);
  butEnv_setXFg(env, BUT_PBG);
  XFillRectangle(env->dpy, win->win, env->gc, but->x+bw,but->y+bw,
		 but->w-2*bw, but->h-2*bw);
  if (slide->horiz)  {
    but_drawBox(win, but->x+bw+slide->boxLoc, but->y+bw,
		slide->boxW, slide->boxH,
		but->flags & BUT_LOCKED, bw,
		BUT_SLEFT|BUT_SRIGHT, BUT_LIT, BUT_SHAD, None, None);
    butEnv_setXFg(env, BUT_BG);
    XFillRectangle(env->dpy, win->win, env->gc,
		   but->x + bw*2 + slide->boxLoc, but->y + bw * 2,
		   slide->boxW - bw * 2, slide->boxH - bw * 2);
  } else /* !but->horiz */ {
    but_drawBox(win, but->x+bw, but->y+bw+slide->boxLoc,
		slide->boxH, slide->boxW,
		but->flags & BUT_LOCKED, bw,
		BUT_SLEFT|BUT_SRIGHT, BUT_LIT, BUT_SHAD, None, None);
    butEnv_setXFg(env, BUT_BG);
    XFillRectangle(env->dpy, win->win, env->gc,
		   but->x + bw * 2, but->y + bw*2 + slide->boxLoc,
		   slide->boxH - bw * 2, slide->boxW - bw * 2);
  }
}


static void  calcBoxLoc(But *but)  {
  Slide  *slide = but->iPacket;
  ButEnv  *env = but->win->env;
  int  allW, allH;
  int  bw;

  bw = butEnv_stdBw(env);
  if (slide->horiz)  {
    allW = but->w - 2*bw;
    allH = but->h - 2*bw;
  } else  {
    allW = but->h - 2*bw;
    allH = but->w - 2*bw;
  }
  if (allW <= allH)  {
    slide->boxW = allW;
    slide->boxH = allH;
    slide->boxLoc = 0;
  } else  {
    slide->boxW = (slide->size * allW + (slide->size + slide->maxval) / 2) /
      (slide->size + slide->maxval);
    slide->boxH = allH;
    if (slide->boxW < slide->boxH)
      slide->boxW = slide->boxH;
    if (slide->maxval == 0)
      slide->boxLoc = 0;
    else
      slide->boxLoc = ((allW - slide->boxW) * slide->cval +
		       slide->maxval / 2) / slide->maxval;
  }
  slide->lastW = but->w;
  slide->lastH = but->h;
}


static void  drawNewLoc(But *but)  {
  ButEnv *env = but->win->env;
  Slide  *slide = but->iPacket;
  int  bw = env->stdButBw;
  int  boxW, boxH, boxLoc;
  int  oldW, oldLoc;
  
  if ((but->w < 1) || (but->h < 1))
    return;
  oldW = slide->boxW;
  oldLoc = slide->boxLoc;
  calcBoxLoc(but);
  boxW = slide->boxW;
  boxH = slide->boxH;
  boxLoc = slide->boxLoc;
  if (boxLoc < oldLoc)  {
    specialDraw(but, slide, boxLoc + bw, bw,
		oldLoc + bw - boxLoc, boxH);
  } else if (boxLoc > oldLoc)  {
    specialDraw(but, slide, oldLoc + bw, bw,
		boxLoc + bw - oldLoc, boxH);
  }
  if (boxLoc + boxW < oldLoc + oldW)  {
    specialDraw(but, slide, boxLoc + boxW, bw,
		oldLoc + oldW + bw - boxLoc - boxW, boxH);
  } else if (boxLoc + boxW > oldLoc + oldW)  {
    specialDraw(but, slide, oldLoc + oldW, bw,
		boxLoc + boxW + bw - oldLoc - oldW, boxH);
  }
}


static void  specialDraw(But *but, Slide *slide, int x, int y, int w, int h)  {
  if (slide->horiz)  {
    butWin_redraw(but->win, but->x + x, but->y + y, w, h);
  } else  {
    butWin_redraw(but->win, but->x + y, but->y + x, h, w);
  }
}


static void  flags(But *but, uint flags)  {
  uint  ofl = but->flags;

  but->flags = flags;
  if ((flags & (BUT_PRESSABLE|BUT_DRAWABLE|BUT_PRESSED|BUT_LOCKED)) !=
      (ofl   & (BUT_PRESSABLE|BUT_DRAWABLE|BUT_PRESSED|BUT_LOCKED)))
    but_draw(but);
}


static ButOut  mmove(But *but, int x, int y)  {
  ButOut  retval = BUTOUT_CAUGHT;
  int  newflags = but->flags;
  ButEnv  *env = butWin_env(but_win(but));
  Slide  *slide = but->iPacket;
  int  newval, newloc, pitmax;
  int  newcur = butCur_idle;
  
  if (slide->horiz)  {
    if ((newflags & BUT_LOCKED) && slide->grip)  {
      /* Slide it around! */
      if (x == BUT_NOCHANGE)
	return(retval);
      newloc = x - slide->start_mloc + slide->start_loc;
      if (newloc < 0)
	newloc = 0;
      pitmax = but->w - slide->boxW - env->stdButBw*2;
      if (newloc > pitmax)
	newloc = pitmax;
      if (pitmax == 0)
	newval = 0;
      else
	newval = (newloc * slide->maxval + pitmax/2) / pitmax;
      if (newval != slide->cval)  {
	slide->cval = newval;
	drawNewLoc(but);
	if (slide->callback != NULL)
	  return(retval | slide->callback(but, slide->cval, FALSE));
      }
      return(retval);
    }
    if ((x >= but->x+env->stdButBw) && (x < but->x+but->w-env->stdButBw) &&
	(y >= but->y+env->stdButBw) && (y < but->y+but->h-env->stdButBw))  {
      newflags |= BUT_TWITCHED;
      if (x < but->x + env->stdButBw + slide->boxLoc)
	newcur = butCur_left;
      else if (x < but->x + env->stdButBw + slide->boxLoc + slide->boxW)
	newcur = butCur_lr;
      else
	newcur = butCur_right;
    } else  {
      newflags &= ~BUT_TWITCHED;
      if (!(newflags & BUT_LOCKED))
	retval &= ~BUTOUT_CAUGHT;
    }
    if (!(newflags & BUT_LOCKED) && (newcur != slide->oldcur))  {
      butEnv_setCursor(env, but, newcur);
      slide->oldcur = newcur;
    }
    if (newflags != but->flags)  {
      but_newFlags(but, newflags);
    }
    return(retval);
  } else /* !slide->horiz */ {
    if ((newflags & BUT_LOCKED) && slide->grip)  {
      /* Slide it around! */
      if (y == BUT_NOCHANGE)
	return(retval);
      newloc = y - slide->start_mloc + slide->start_loc;
      if (newloc < 0)
	newloc = 0;
      pitmax = but->h - slide->boxW - env->stdButBw*2;
      if (newloc > pitmax)
	newloc = pitmax;
      if (pitmax == 0)
	newval = 0;
      else
	newval = (newloc * slide->maxval + pitmax/2) / pitmax;
      if (newval != slide->cval)  {
	slide->cval = newval;
	drawNewLoc(but);
	if (slide->callback != NULL)
	  return(retval | slide->callback(but, slide->cval, FALSE));
      }
      return(retval);
    }
    if ((x >= but->x+env->stdButBw) && (x < but->x+but->w-env->stdButBw) &&
	(y >= but->y+env->stdButBw) && (y < but->y+but->h-env->stdButBw))  {
      newflags |= BUT_TWITCHED;
      if (y < but->y + env->stdButBw + slide->boxLoc)
	newcur = butCur_up;
      else if (y < but->y + env->stdButBw + slide->boxLoc + slide->boxW)
	newcur = butCur_ud;
      else
	newcur = butCur_down;
    } else  {
      newflags &= ~BUT_TWITCHED;
      if (!(newflags & BUT_LOCKED))
	retval &= ~BUTOUT_CAUGHT;
    }
    if (!(newflags & BUT_LOCKED) && (newcur != slide->oldcur))  {
      butEnv_setCursor(env, but, newcur);
      slide->oldcur = newcur;
    }
    if (newflags != but->flags)  {
      but_newFlags(but, newflags);
    }
    return(retval);
  }
}


static ButOut  mleave(But *but)  {
  int  newflags = but->flags;
  Slide  *slide = but->iPacket;
  
  newflags &= ~BUT_TWITCHED;
  butEnv_setCursor(but->win->env, but, butCur_idle);
  slide->oldcur = butCur_idle;
  if (newflags != but->flags)  {
    but_newFlags(but, newflags);
  }
  return(BUTOUT_CAUGHT);
}


static ButOut  mpress(But *but, int butnum, int x, int y)  {
  int  retval = BUTOUT_CAUGHT, newflags = but->flags;
  Slide  *slide = but->iPacket;
  ButEnv  *env = but->win->env;
  
  if (!newflags & BUT_TWITCHED)
    retval &= ~BUTOUT_CAUGHT;
  else  {
    if (slide->horiz)  {
      if (but->w <= but->h)
	return(BUTOUT_CAUGHT | BUTOUT_ERR);
      if (butnum == 1)  {
	newflags |= BUT_LOCKED;
	if (x < but->x + env->stdButBw + slide->boxLoc)  {
	  butSlide_startSlide(but, TRUE, -slide->size*2, FALSE);
	} else if (x >= but->x + env->stdButBw + slide->boxLoc +
		   slide->boxW)  {
	  butSlide_startSlide(but, TRUE, slide->size*2, FALSE);
	} else  {
	  /* The box has been "gripped." */
	  slide->grip = TRUE;
	  slide->start_mloc = x;
	  slide->start_loc = slide->boxLoc;
	  slide->start_val = slide->cval;
	}
      } else
	return(BUTOUT_CAUGHT | BUTOUT_ERR);
    } else /* !slide->horiz */ {
      if (but->h <= but->w)
	return(BUTOUT_CAUGHT | BUTOUT_ERR);
      if (butnum == 1)  {
	newflags |= BUT_LOCKED;
	if (y < but->y + env->stdButBw + slide->boxLoc)  {
	  butSlide_startSlide(but, TRUE, -slide->size*2, FALSE);
	} else if (y >= but->y + env->stdButBw + slide->boxLoc +
		   slide->boxW)  {
	  butSlide_startSlide(but, TRUE, slide->size*2, FALSE);
	} else  {
	  /* The box has been "gripped." */
	  slide->grip = TRUE;
	  slide->start_mloc = y;
	  slide->start_loc = slide->boxLoc;
	  slide->start_val = slide->cval;
	}
      } else
	return(BUTOUT_CAUGHT | BUTOUT_ERR);
    }
    snd_play(&but_downSnd);
    if (newflags != but->flags)
      but_newFlags(but, newflags);
    if (slide->callback != NULL)
      return(retval | slide->callback(but, slide->cval, TRUE));
  }
  return(retval);
}


void  butSlide_startSlide(But *but, bool pause, int ssize, bool propagate)  {
  Slide  *slide = but->iPacket;
  struct timeval  delay;
  int  atj;
  bool  skipTimer = FALSE;

  if (slide->timer != NULL)
    butTimer_destroy(slide->timer);
  if (pause)  {
    slide->cval += ssize/2;
    if (slide->cval <= 0)  {
      slide->cval = 0;
      skipTimer = TRUE;
    } else if (slide->cval >= slide->maxval - 1)  {
      slide->cval = slide->maxval - 1;
      skipTimer = TRUE;
    }
    drawNewLoc(but);
    if (propagate && slide->callback)
      slide->callback(but, slide->cval, TRUE);
  }
  if (!skipTimer)  {
    slide->timer_jump = ssize;
    if (pause)  {
      delay.tv_sec = 0;
      delay.tv_usec = 500000;
    } else  {
      delay.tv_sec = 0;
      delay.tv_usec = 0;
    }
    if (slide->timer_jump > 0)
      atj = slide->timer_jump;
    else
      atj = -slide->timer_jump;
    slide->timer = butTimer_fCreate(NULL, but, delay, atj*2, FALSE, slide_now);
  }
    
}


static ButOut  slide_now(ButTimer *bt)  {
  But  *but = bt->but;
  Slide  *slide = but->iPacket;

  if (slide->timer_jump > 0)  {
    slide->cval += bt->eventNum;
    if (slide->cval > slide->maxval)  {
      slide->cval = slide->maxval;
      butTimer_destroy(bt);
      slide->timer = NULL;
    }
  } else  {
    slide->cval -= bt->eventNum;
    if (slide->cval < 0)  {
      slide->cval = 0;
      butTimer_destroy(bt);
      slide->timer = NULL;
    }
  }
  bt->eventNum = 0;
  drawNewLoc(but);
  if (slide->callback != NULL)
    return(slide->callback(but, slide->cval, FALSE));
  return(0);
}


static ButOut  mrelease(But *but, int butnum, int x, int y)  {
  ButOut  retval = BUTOUT_CAUGHT;
  uint  newflags = but->flags;
  Slide  *slide = but->iPacket;
  
  if (!newflags & BUT_TWITCHED)
    butEnv_setCursor(but->win->env, but, slide->oldcur = butCur_idle);
  if (slide->timer != NULL)  {
    butTimer_destroy(slide->timer);
    slide->timer = NULL;
  }
  slide->grip = FALSE;
  newflags &= ~BUT_LOCKED;
  if (newflags != but->flags)
    but_newFlags(but, newflags);
  snd_play(&but_upSnd);
  return(retval);
}


void  butSlide_stopSlide(But *but)  {
  Slide  *slide = but->iPacket;

  if (slide->timer)  {
    butTimer_destroy(slide->timer);
    slide->timer = NULL;
  }
}


#endif
