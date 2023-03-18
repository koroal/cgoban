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
#include <but/checkbox.h>
#include <but/box.h>

typedef struct Cb_struct  {
  bool  on;
  ButOut (*press)(But *but, bool value);
} Cb;

static ButOut  mmove(But *but, int x, int y);
static ButOut  mleave(But *but);
static ButOut  mpress(But *but, int butnum, int x, int y);
static ButOut  mrelease(But *but, int butnum, int x, int y);
static ButOut  kpress(But *but, const char *keystr, KeySym sym);
static ButOut  krelease(But *but, const char *keystr, KeySym sym);
static void  draw(But *but, int x, int y, int w, int h);
static ButOut  destroy(But *but);
static void  flags(But *but, uint flags);

static const ButAction  action = {
  mmove, mleave, mpress, mrelease,
  kpress, krelease, draw, destroy, flags, NULL};


/**********************************************************************
 * Functions
 **********************************************************************/
But  *butCb_create(ButOut (*func)(But *but, bool value), void *packet,
		    ButWin *win, int layer, int flags, bool on)  {
  But  *but;
  Cb  *cb;

  cb = wms_malloc(sizeof(Cb));
  but = but_create(win, cb, &action);
  MAGIC_SET(but);
  but->uPacket = packet;
  but->layer = layer;
  but->flags = flags | BUT_OPAQUE;

  cb->on = on;
  cb->press = func;
  but_init(but);
  return(but);
}


bool  butCb_get(But *but)  {
  Cb  *cb = but->iPacket;

  assert(but->action == &action);
  assert(MAGIC(but));
  return(cb->on);
}

  
void  butCb_set(But *but, bool on, bool makeCallback)  {
  Cb  *cb = but->iPacket;

  assert(but->action == &action);
  assert(MAGIC(but));
  if (cb->on != on)  {
    cb->on = on;
    but_draw(but);
    if (makeCallback && (cb->press != NULL))
      cb->press(but, on);
  }
}


static ButOut  destroy(But *but)  {
  Cb *cb = but->iPacket;
  
  assert(but->action == &action);
  assert(MAGIC(but));
  wms_free(cb);
  return(0);
}


static void  draw(But *but, int x, int y, int w, int h)  {
  ButWin  *win = but->win;
  Cb  *cb = but->iPacket;
  uint  flags = but->flags;
  ButEnv  *env = win->env;
  XPoint  check[6];
  int  i, ix, iy, iw, ih, delta;
  
  assert(but->action == &action);
  assert(MAGIC(but));
  ix = but->x + env->stdButBw - win->xOff;
  iy = but->y + env->stdButBw - win->yOff;
  iw = but->w - 2*env->stdButBw;
  ih = but->h - 2*env->stdButBw;
  but_drawBox(win, but->x,but->y, but->w,but->h,
	      flags & BUT_PRESSED, env->stdButBw,
	      BUT_SLEFT|BUT_SRIGHT, BUT_LIT, BUT_SHAD, None, None);
  if (flags & BUT_PRESSED)
    butEnv_setXFg(env, BUT_PBG);
  else  {
    if (cb->on && (but->flags & BUT_PRESSABLE))  {
      butEnv_setXFg(env, BUT_CHOICE);
    } else
      butEnv_setXFg(env, BUT_BG);
  }
  XFillRectangle(env->dpy, win->win, env->gc, ix,iy, iw,ih);
  if (((flags & BUT_PRESSED) && !cb->on) ||
      (!(flags & BUT_PRESSED) && cb->on))  {
    check[0].x = ix + (iw + 1) / 3;
    check[0].y = iy + ih;
    check[1].x = ix;
    check[1].y = iy + (2*ih + 1) / 3;
    check[2].x = ix + (iw + 3) / 6;
    check[2].y = iy + (3*ih + 3) / 6;
    check[3].x = check[0].x;
    check[3].y = check[1].y;
    check[4].x = ix + iw;
    check[4].y = iy + (ih + 1) / 3;
    check[5] = check[0];
    if (flags & BUT_PRESSED)  {
      delta = (env->stdButBw + 1) / 2;
      for (i = 1;  i < 5;  ++i)
	check[i].y += delta;
      check[0].x -= delta;
      check[5].x += delta;
    }
    butEnv_setXFg(env, BUT_FG);
    XFillPolygon(env->dpy, win->win, env->gc, check, 6, Nonconvex,
		 CoordModeOrigin);
  }
}


static void  flags(But *but, uint flags)  {
  uint  ofl = but->flags;

  but->flags = flags;
  if (((flags & (BUT_PRESSABLE|BUT_DRAWABLE|BUT_PRESSED)) !=
       (ofl   & (BUT_PRESSABLE|BUT_DRAWABLE|BUT_PRESSED))) ||
      (((flags & BUT_TWITCHED) != (ofl & BUT_TWITCHED)) &&
       (flags & BUT_PRESSED)))
    but_draw(but);
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
  ButOut  retval = BUTOUT_CAUGHT;
  int  newflags = but->flags;
  
  if (!newflags & BUT_TWITCHED)
    retval &= ~BUTOUT_CAUGHT;
  else  {
    if (butnum == 1)
      newflags |= BUT_PRESSED | BUT_LOCKED;
    else
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
  Cb  *cb;
  
  if (butnum != 1)  {
    if (but->flags & BUT_TWITCHED)
      return(BUTOUT_CAUGHT);
    else
      return(0);
  }
  if (!(but->flags & BUT_PRESSED))
    return(0);
  if (but->flags & BUT_TWITCHED)  {
    cb = but->iPacket;
    cb->on = !cb->on;
    if (cb->press != NULL)
      retval |= cb->press(but, cb->on);
  } else
    retval |= BUTOUT_ERR;
  newflags &= ~(BUT_PRESSED|BUT_LOCKED);
  if ((but->flags & BUT_PRESSED) && !(newflags & BUT_PRESSED) &&
      !(retval & BUTOUT_ERR))
    snd_play(&but_upSnd);
  if (newflags != but->flags)
    but_newFlags(but, newflags);
  return(retval);
}


static ButOut  kpress(But *but, const char *keystr, KeySym sym)  {
  ButOut  retval = BUTOUT_CAUGHT;
  int  newflags = but->flags;
  
  newflags |= BUT_KEYPRESSED;
  if (!(but->flags & BUT_KEYPRESSED))  {
    snd_play(&but_downSnd);
    but_newFlags(but, newflags);
  }
  return(retval);
}


static ButOut  krelease(But *but, const char *keystr, KeySym sym)  {
  ButOut  retval = BUTOUT_CAUGHT;
  int  newflags = but->flags;
  Cb  *cb = but->iPacket;
  
  newflags &= ~(BUT_KEYPRESSED);
  cb->on = !cb->on;
  if (cb->press != NULL)
    retval |= cb->press(but, cb->on);
  snd_play(&but_upSnd);
  if (newflags != but->flags)
    but_newFlags(but, newflags);
  return(retval);
}


#endif
