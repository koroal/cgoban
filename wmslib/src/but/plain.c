/*
 * wmslib/src/but/plain.c, part of wmslib (Library functions)
 * Copyright (C) 1994-1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 *
 * File for "plain" buttons...pixmaps, solid colors, etc.
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


static void  pix_draw(But *but, int x, int y, int w, int h);
static void  plain_draw(But *but, int x, int y, int w, int h);
static void  dummy_draw(But *but, int x, int y, int w, int h);
static void  keytrap_draw(But *but, int x, int y, int w, int h);
static ButOut  pixmap_destroy(But *but);
static ButOut  plain_destroy(But *but);
static ButOut  dummy_destroy(But *but);
static ButOut  keytrap_destroy(But *but);
static ButOut  plain_mmove(But *but, int x, int y);
static ButOut  keytrap_kpress(But *but, const char *keystr, KeySym sym);
static ButOut  keytrap_krelease(But *but, const char *keystr, KeySym sym);


static ButAction  pix_action = {
  NULL, NULL, NULL, NULL,
  NULL, NULL, pix_draw, pixmap_destroy, but_flags, NULL};

static ButAction  plain_action = {
  plain_mmove, NULL, NULL, NULL,
  NULL, NULL, plain_draw, plain_destroy, but_flags, NULL};

static ButAction  dummy_action = {
  NULL, NULL, NULL, NULL,
  NULL, NULL, dummy_draw, dummy_destroy, but_flags, NULL};

static ButAction  keytrap_action = {
  NULL, NULL, NULL, NULL, keytrap_kpress, keytrap_krelease,
  keytrap_draw, keytrap_destroy, but_flags, NULL};

typedef struct Pix_struct  {
  Pixmap  pic;
  int  x,y;
} Pix;

typedef struct Plain_struct  {
  int  color;
} Plain;

typedef struct Keytrap_struct  {
  ButOut  (*callback)(But *but, int press);
  bool  holdKey;
} Keytrap;
  

But  *butPixmap_create(ButWin *win, int layer, int flags, Pixmap pic)  {
  Pix  *pix;
  But  *but;

  pix = wms_malloc(sizeof(Pix));
  but = but_create(win, pix, &pix_action);
  but->win = win;
  but->layer = layer;
  but->flags = flags | BUT_OPAQUE;

  pix->pic = pic;
  pix->x = 0;
  pix->y = 0;
  but_init(but);
  return(but);
}
  

void  butPixmap_setPic(But *but, Pixmap pic, int x,int y)  {
  Pix  *pix = but->iPacket;

  assert(but->action == &pix_action);
  pix->pic = pic;
  pix->x = x;
  pix->y = y;
  but_draw(but);
}


static ButOut  pixmap_destroy(But *but)  {
  wms_free(but->iPacket);
  return(0);
}


static void  pix_draw(But *but, int x, int y, int w, int h)  {
  ButWin  *win = but->win;
  ButEnv  *env = win->env;
  Pix  *pix = but->iPacket;

  if (pix->pic == None)  {
    return;
  }
  XSetFillStyle(env->dpy, env->gc, FillTiled);
  XSetTile(env->dpy, env->gc, pix->pic);
  if ((but->x != pix->x + win->xOff) || (but->y != pix->y + win->yOff))
    XSetTSOrigin(env->dpy, env->gc, but->x - pix->x - win->xOff,
		 but->y - pix->y - win->yOff);
  if (but->x > x)  {
    w -= but->x - x;
    x = but->x;
  }
  if (but->y > y)  {
    h -= but->y - y;
    y = but->y;
  }
  if (but->x + but->w < x + w)
    w = but->x + but->w - x;
  if (but->y + but->h < y + h)
    h = but->y + but->h - y;
  XFillRectangle(env->dpy, win->win, env->gc, x - win->xOff, y - win->yOff,
		 w, h);
  butEnv_stdFill(env);
  if ((but->x != pix->x) || (but->y != pix->y))
    XSetTSOrigin(env->dpy, env->gc, 0,0);
}


But  *butPlain_create(ButWin *win, int layer, int flags, int cnum)  {
  Plain  *plain;
  But  *but;

  plain = wms_malloc(sizeof(Plain));
  but = but_create(win, plain, &plain_action);
  but->layer = layer;
  but->flags = flags | BUT_OPAQUE;

  plain->color = cnum;
  but_init(but);
  return(but);
}
  

void  butPlain_setColor(But *but, int cnum)  {
  Plain  *plain = but->iPacket;

  plain->color = cnum;
  but_draw(but);
}


static ButOut  plain_destroy(But *but)  {
  wms_free(but->iPacket);
  return(0);
}


static void  plain_draw(But *but, int x, int y, int w, int h)  {
  ButWin  *win = but->win;
  ButEnv  *env = win->env;
  Plain  *color = but->iPacket;

  if (but->x > x)  {
    w -= (but->x - x);
    x = but->x;
  }
  if (but->y > y)  {
    h -= (but->y - y);
    y = but->y;
  }
  if (but->x+but->w < x+w)
    w = but->x+but->w - x;
  if (but->y+but->h < y+h)
    h = but->y+but->h - y;
  butEnv_setXFg(env, color->color);
  XFillRectangle(env->dpy, win->win, env->gc, x-win->xOff, y-win->yOff, w,h);
}


static ButOut  plain_mmove(But *but, int x, int y)  {
  return(BUTOUT_CAUGHT);
}


But  *butDummy_create(ButWin *win, int layer, int flags)  {
  But  *but;

  but = but_create(win, NULL, &dummy_action);
  but->layer = layer;
  but->flags = flags | BUT_OPAQUE;
  but_init(but);
  return(but);
}
  

static ButOut  dummy_destroy(But *but)  {
  return(0);
}


static void  dummy_draw(But *but, int x, int y, int w, int h)  {
}


But  *butKeytrap_create(ButOut (*func)(But *but, int press), void *uPacket,
			ButWin *win, int flags)  {
  Keytrap  *ktrap;
  But  *but;

  but = but_create(win, NULL, &keytrap_action);
  ktrap = wms_malloc(sizeof(Keytrap));
  but->uPacket = uPacket;
  but->iPacket = ktrap;
  ktrap->callback = func;
  ktrap->holdKey = TRUE;
  but->flags = flags;
  but->w = 1;
  but->h = 1;
  but_init(but);
  return(but);
}
  

static ButOut  keytrap_destroy(But *but)  {
  return(0);
}


void  butKeytrap_setHold(But *but, bool hold)  {
  Keytrap  *k;

  assert(but->action == &keytrap_action);
  k = but->iPacket;
  k->holdKey = hold;
}


static ButOut  keytrap_kpress(But *but, const char *keystr, KeySym sym)  {
  ButOut  retval = BUTOUT_CAUGHT;
  Keytrap  *k = but->iPacket;
  
  if (!(but->flags & BUT_KEYPRESSED))  {
    snd_play(&but_downSnd);
    if (k->holdKey)
      but_newFlags(but, but->flags | BUT_KEYPRESSED|BUT_PRESSED|BUT_LOCKED);
    retval |= k->callback(but, TRUE);
  }
  return(retval);
}


static ButOut  keytrap_krelease(But *but, const char *keystr, KeySym sym)  {
  ButOut  retval = BUTOUT_CAUGHT;
  int  newflags = but->flags;
  Keytrap  *k = but->iPacket;
  
  if (!(newflags & BUT_KEYPRESSED) && k->holdKey)
    return(0);
  if (k->holdKey)  {
    newflags &= ~(BUT_KEYPRESSED|BUT_PRESSED|BUT_LOCKED);
    if (newflags != but->flags)
      but_newFlags(but, newflags);
  }
  snd_play(&but_upSnd);
  if (keystr != NULL)  {
    /* If keystr is NULL, the key wasn't released - we lost focus instead. */
    retval |= k->callback(but, FALSE);
  }
  return(retval);
}


static void  keytrap_draw(But *but, int x, int y, int w, int h)  {
}


#endif  /* X11_DISP */
