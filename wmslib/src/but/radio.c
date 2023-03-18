/*
 * wmslib/src/but/checkbox.c, part of wmslib (Library functions)
 * Copyright (C) 1994-1996 William Shubert.
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
#include <but/box.h>
#ifdef  _BUT_RADIO_H_
        LEVELIZATION ERROR
#endif
#include <but/radio.h>

typedef struct Radio_struct  {
  int  val, maxVal;
  int  pVal;  /* The pressed down value. */
  ButOut (*press)(But *but, int value);
} Radio;

static ButOut  mmove(But *but, int x, int y);
static ButOut  mleave(But *but);
static ButOut  mpress(But *but, int butnum, int x, int y);
static ButOut  mrelease(But *but, int butnum, int x, int y);
static void  draw(But *but, int x, int y, int w, int h);
static ButOut  destroy(But *but);
static void  flags(But *but, uint flags);
static void  redrawOpt(But *but, Radio *radio, int opt);

static ButAction  action = {
  mmove, mleave, mpress, mrelease,
  NULL, NULL, draw, destroy, flags, NULL};


But  *butRadio_create(ButOut (*func)(But *but, bool value), void *packet,
		      ButWin *win, int layer, int flags,
		      int val, int maxVal)  {
  But  *but;
  Radio  *radio;

  radio = wms_malloc(sizeof(Radio));
  but = but_create(win, radio, &action);
  MAGIC_SET(but);
  but->uPacket = packet;
  but->layer = layer;
  but->flags = flags | BUT_OPAQUE;

  radio->val = val;
  radio->maxVal = maxVal;
  radio->pVal = -1;
  radio->press = func;
  but_init(but);
  return(but);
}


int  butRadio_get(But *but)  {
  Radio  *radio = but->iPacket;

  assert(but->action == &action);
  assert(MAGIC(but));
  return(radio->val);
}

  
void  butRadio_set(But *but, int val, bool propagate)  {
  Radio  *radio = but->iPacket;

  assert(but->action == &action);
  assert(MAGIC(but));
  if (radio->val != val)  {
    redrawOpt(but, radio, val);
    redrawOpt(but, radio, radio->val);
    radio->val = val;
    but_draw(but);
    if (propagate && (radio->press != NULL))
      radio->press(but, val);
  }
}


static ButOut  destroy(But *but)  {
  Radio *radio = but->iPacket;
  
  assert(but->action == &action);
  assert(MAGIC(but));
  wms_free(radio);
  return(0);
}


static void  draw(But *but, int x, int y, int w, int h)  {
  ButWin  *win = but->win;
  Radio  *radio = but->iPacket;
  uint  flags = but->flags;
  ButEnv  *env = win->env;
  int  i, ix, ix2, iy, iw, ih;
  
  assert(but->action == &action);
  assert(MAGIC(but));
  ix2 = but->x;
  for (i = 0;  i < radio->maxVal;  ++i)  {
    ix = ix2;
    ix2 = but->x + ((i + 1) * but->w + (radio->maxVal>>1)) / radio->maxVal;
    iy = but->y;
    iw = ix2 - ix;
    ih = but->h;
    but_drawBox(win, ix,iy, iw,ih,
		(flags & BUT_PRESSED) && (i == radio->pVal), env->stdButBw,
		BUT_SLEFT|BUT_SRIGHT, BUT_LIT, BUT_SHAD, None, None);
    if (i == radio->pVal)
      butEnv_setXFg(env, BUT_PBG);
    else if (i == radio->val)
      butEnv_setXFg(env, BUT_CHOICE);
    else
      butEnv_setXFg(env, BUT_BG);
    XFillRectangle(env->dpy, win->win, env->gc,
		   ix + env->stdButBw - win->xOff,
		   iy + env->stdButBw - win->yOff,
		   iw - env->stdButBw * 2, ih - env->stdButBw * 2);
  }
}


static void  flags(But *but, uint flags)  {
  uint  ofl = but->flags;
  Radio  *radio;

  but->flags = flags;
  if ((flags & (BUT_PRESSABLE|BUT_DRAWABLE|BUT_PRESSED)) !=
      (ofl   & (BUT_PRESSABLE|BUT_DRAWABLE|BUT_PRESSED)))  {
    radio = but->iPacket;
    if (radio->pVal >= 0)
      redrawOpt(but, radio, radio->pVal);
  }
}


static ButOut  mmove(But *but, int x, int y)  {
  ButOut  retval = BUTOUT_CAUGHT;
  int  newflags = but->flags;
  ButEnv  *env = but->win->env;
  
  if (!(but->flags & BUT_PRESSABLE))
    return(BUTOUT_CAUGHT);
  if ((x >= but->x + env->stdButBw) && (y >= but->y + env->stdButBw) &&
      (x < but->x + but->w - env->stdButBw) &&
      (y < but->y + but->h - env->stdButBw))
    newflags |= BUT_TWITCHED;
  else  {
    newflags &= ~BUT_TWITCHED;
    if (!(newflags & BUT_LOCKED))
      retval &= ~BUTOUT_CAUGHT;
  }
  if (!(but->flags & BUT_TWITCHED) && (newflags & BUT_TWITCHED))
    butEnv_setCursor(but->win->env, but, butCur_twitch);
  else if ((but->flags & BUT_TWITCHED) && !(newflags & BUT_TWITCHED))
    butEnv_setCursor(but->win->env, but, butCur_idle);
  if (newflags != but->flags)  {
    but_newFlags(but, newflags);
  }
  return(retval);
}


static ButOut  mleave(But *but)  {
  int  newflags = but->flags;
  
  newflags &= ~BUT_TWITCHED;
  if ((but->flags & BUT_TWITCHED) && !(newflags & BUT_TWITCHED))
    butEnv_setCursor(but->win->env, but, butCur_idle);
  if (newflags != but->flags)  {
    but_newFlags(but, newflags);
  }
  return(BUTOUT_CAUGHT);
}


static ButOut  mpress(But *but, int butnum, int x, int y)  {
  Radio  *radio = but->iPacket;
  ButOut  retval = BUTOUT_CAUGHT;
  int  newflags = but->flags;
  
  if (!newflags & BUT_TWITCHED)
    retval &= ~BUTOUT_CAUGHT;
  else  {
    if (butnum == 1)  {
      newflags |= BUT_PRESSED | BUT_LOCKED;
      radio->pVal = ((x - but->x) * radio->maxVal) / but->w;
    } else
      return(BUTOUT_CAUGHT | BUTOUT_ERR);
  }
  if (!(but->flags & BUT_PRESSED) && (newflags & BUT_PRESSED))
    snd_play(&but_downSnd);
  if (newflags != but->flags)
    but_newFlags(but, newflags);
  return(retval);
}


static ButOut  mrelease(But *but, int butnum, int x, int y)  {
  ButOut  retval = BUTOUT_CAUGHT;
  int  newflags = but->flags;
  Radio  *radio = but->iPacket;
  int  oldPVal, newPVal;
  
  oldPVal = radio->pVal;
  if (oldPVal >= 0)
    redrawOpt(but, radio, oldPVal);
  radio->pVal = -1;
  if (butnum != 1)  {
    if (but->flags & BUT_TWITCHED)
      return(BUTOUT_CAUGHT);
    else
      return(0);
  }
  if (!(but->flags & BUT_PRESSED))
    return(0);
  if (but->flags & BUT_TWITCHED)  {
    newPVal = ((x - but->x) * radio->maxVal) / but->w;
    if (newPVal != oldPVal)
      oldPVal = -1;
  } else  {
    oldPVal = -1;
    retval |= BUTOUT_ERR;
  }
  newflags &= ~(BUT_PRESSED|BUT_LOCKED);
  if ((but->flags & BUT_PRESSED) && !(newflags & BUT_PRESSED) &&
      !(retval & BUTOUT_ERR))
    snd_play(&but_upSnd);
  if (newflags != but->flags)
    but_newFlags(but, newflags);
  if (oldPVal != -1)  {
    redrawOpt(but, radio, oldPVal);
    redrawOpt(but, radio, radio->val);
    radio->val = oldPVal;
    if (radio->press)
      radio->press(but, oldPVal);
  } else
    retval |= BUTOUT_ERR;
  return(retval);
}


static void  redrawOpt(But *but, Radio *radio, int opt)  {
  int  x1, x2;

  if ((opt >= 0) && (opt < radio->maxVal))  {
    x1 = (but->w * opt + (radio->maxVal >> 1)) / radio->maxVal;
    x2 = (but->w * (opt + 1) + (radio->maxVal >> 1)) / radio->maxVal;
    butWin_redraw(but->win, but->x + x1, but->y, x2 - x1, but->h);
  }
}


#endif
