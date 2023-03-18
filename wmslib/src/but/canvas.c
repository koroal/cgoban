/*
 * wmslib/src/but/canvas.c, part of wmslib (Library functions)
 * Copyright (C) 1994-1996 William Shubert.
 * See "configure.h.in" for more copyright information.
 */

#include <wms.h>

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
#ifdef  _BUT_CANVAS_H_
  Levelization Error.
#endif
#include "canvas.h"


/**********************************************************************
 * Data Types
 **********************************************************************/
typedef struct Can_struct  {
  bool  grabbed;
  int  grab_mx,grab_my, grab_ox,grab_oy;
  ButWin  *parent, *win;
  But  *but;
  Pixmap  pmap;
  void  (*change)(void *packet, int xOff, int yOff,
		  int w, int h, int viewW, int viewH);
  MAGIC_STRUCT
} Can;


/**********************************************************************
 * Forward Declarations
 **********************************************************************/
static ButOut  mmove(But *but, int x, int y);
static ButOut  mleave(But *but);
static ButOut  mpress(But *but, int butnum, int x, int y);
static ButOut  mrelease(But *but, int butnum, int x, int y);
static ButOut  kpress(But *but, const char *keystr, KeySym sym);
static ButOut  krelease(But *but, const char *keystr, KeySym sym);
static void  draw(But *but, int x, int y, int w, int h);
static ButOut  destroy(But *but);
static bool  reviseDims(Can *can);


/**********************************************************************
 * Globals
 **********************************************************************/
static const ButAction  action = {
  mmove, mleave, mpress, mrelease,
  kpress, krelease, draw, destroy, but_flags, NULL};


/**********************************************************************
 * Functions
 **********************************************************************/
ButWin  *butCan_create(void *packet, ButWin *parent, int layer,
		       ButWinFunc *resize, ButWinFunc *destroy,
		       void (*change)(void *packet, int xOff, int yOff,
				      int w, int h,
				      int viewW, int viewH))  {
  ButEnv  *env;
  ButWin  *win;
  But  *but;
  Can  *can;

  assert(MAGIC(parent));
  env = parent->env;
  assert(MAGIC(env));

  win = wms_malloc(sizeof(ButWin));
  can = wms_malloc(sizeof(Can));
  but = but_create(parent, can, &action);

  MAGIC_SET(win);
  MAGIC_SET(can);
  MAGIC_SET(&win->butsNoDraw);

  win->parent = parent;
  win->parentBut = but;
  win->packet = packet;
  win->iPacket = can;
  win->win = None;
  win->physWin = butCan_xWin(parent);
  win->iconWin = NULL;

  win->name = NULL;
  win->iconic = FALSE;
  win->isIcon = FALSE;
  win->x = 0;
  win->y = 0;
  win->w = win->minW = win->maxW = win->logicalW = 0;
  win->h = win->minH = win->maxH = win->logicalH = 0;
  win->wStep = 1;
  win->hStep = 1;
  win->xOff = 0;
  win->yOff = 0;
  win->minWRatio = win->minHRatio = win->maxWRatio = win->maxHRatio = 0;

  win->resized = FALSE;
  win->resizeNeeded = FALSE;
  win->redrawReady = TRUE;
  win->redraws = NULL;
  win->numRedraws = win->maxRedraws = 0;

  win->id = 0;
  win->mapped = TRUE;
  win->unmap = NULL;
  win->map = NULL;
  win->resize = resize;
  win->destroy = destroy;
  win->quit = NULL;
  win->env = env;
  win->minLayer = 1;
  win->maxLayer = 0;
  win->butsNoDraw.buts = NULL;
  win->butsNoDraw.numButs = 0;
  win->butsNoDraw.maxButs = 0;
  win->butsNoDraw.dynamic = TRUE;
  win->lock = NULL;
  win->butIn = NULL;
  win->keyBut = NULL;
  win->numCols = 0;
  win->maxCols = 0;
  win->cols = NULL;
  butWin_addToTable(win);

  but->uPacket = packet;
  but->layer = layer;
  but->flags = BUT_DRAWABLE|BUT_PRESSABLE|BUT_OPAQUE;

  can->grabbed = FALSE;
  can->parent = parent;
  can->win = win;
  can->but = but;
  can->pmap = None;
  can->change = change;

  but_init(but);
  return(win);
}


void  butCan_resizeView(ButWin *win, int x, int y, int w, int h,
			bool propagate)  {
  int  oldh, oldw, oldx, oldy;
  Can  *can = win->iPacket;
  But  *but = can->but;

  assert(MAGIC(win));
  assert(MAGIC(can));
  win->resized = TRUE;
  oldx = but->x;
  oldy = but->y;
  oldw = but->w;
  oldh = but->h;
  if (x == BUT_NOCHANGE)
    x = oldx;
  if (y == BUT_NOCHANGE)
    y = oldy;
  if (w == BUT_NOCHANGE)
    w = oldw;
  if (h == BUT_NOCHANGE)
    h = oldh;
  
  /*
   * Check to see if the porthole size has changed.  If it has, hit it
   *   with a redraw.  Not real necessary for these, but what the heck.
   */
  if ((w != oldw) || (h != oldh))  {
    win->w = w;
    win->h = h;
    reviseDims(can);
    if (win->resize != NULL)  {
      win->resize(win);
    }
    if (can->pmap != None)  {
      butWin_rmFromTable(win);
      XFreePixmap(win->env->dpy, can->pmap);
    }
    can->pmap = XCreatePixmap(win->env->dpy, win->physWin,
			      w, h, win->env->depth);
    win->win = can->pmap;
    butWin_addToTable(win);
    but_resize(but, x,y, w,h);
    butWin_redraw(win, win->xOff,win->yOff, w,h);
    if (propagate && can->change)  {
      can->change(can->but->uPacket, win->xOff, win->yOff,
		  win->logicalW, win->logicalH, win->w, win->h);
    }
  } else if ((x != oldx) || (y != oldy))  {
    /* Check if the porthole geometry has changed, necessitating a redraw. */
    but_resize(but, x,y, w,h);
  }
}


void  butCan_slide(ButWin *win, int xOff, int yOff, bool propagate)  {
  int  oldox, oldoy;
  Can  *can = win->iPacket;
  int  csx, csy, cdx, cdy, cw, ch;
  int  rdx, rdw;

  assert(MAGIC(win));
  assert(MAGIC(can));
  oldox = win->xOff;
  oldoy = win->yOff;
  if (xOff != BUT_NOCHANGE)
    win->xOff = xOff;
  if (yOff != BUT_NOCHANGE)
    win->yOff = yOff;
  reviseDims(can);
  if (((oldox == win->xOff) && (oldoy == win->yOff)) || (win->win == None))
    return;
  
  if (win->xOff > oldox)  {
    csx = win->xOff - oldox;
    cdx = 0;
    cw = win->w - csx;
  } else if (win->xOff == oldox)  {
    csx = 0;
    cdx = 0;
    cw = win->w;
  } else  {  /* win->xOff < oldox */
    csx = 0;
    cdx = oldox - win->xOff;
    cw = win->w - csx;
  }

  if (win->yOff > oldoy)  {
    csy = win->yOff - oldoy;
    cdy = 0;
    ch = win->h - csy;
  } else if (win->yOff == oldoy)  {
    csy = 0;
    cdy = 0;
    ch = win->h;
  } else  {  /* win->yOff < oldoy */
    csy = 0;
    cdy = oldoy - win->yOff;
    ch = win->h - csy;
  }

  if ((cw < 0) || (ch < 0))  {
    /* Must redraw entire pixmap. */
    butWin_redraw(win, win->xOff, win->yOff, win->w, win->h);
  } else  {
    XCopyArea(win->env->dpy, can->pmap, can->pmap, win->env->gc,
	      csx,csy, cw,ch, cdx,cdy);
    if (cdx > csx)  {
      butWin_redraw(win, win->xOff, win->yOff, cdx-csx, win->h);
      rdx = win->xOff+cdx-csx;
      rdw = win->w - (cdx-csx);
      if (rdw < 0)
	rdw = 0;
    } else if (cdx == csx)  {
      rdx = win->xOff;
      rdw = win->w;
    } else  {  /* cdx < csx */
      butWin_redraw(win, win->xOff+win->w-(csx-cdx), win->yOff,
		    csx-cdx, win->h);
      rdx = win->xOff;
      rdw = win->w - (csx - cdx);
      if (rdw < 0)
	rdw = 0;
    }
    if (cdy > csy)  {
      butWin_redraw(win, rdx, win->yOff, rdw, cdy-csy);
    } else if (cdy < csy)  {
      butWin_redraw(win, rdx, win->yOff+win->h-(csy-cdy), rdw, csy-cdy);
    }
  }
  but_draw(can->but);
  if (propagate)  {
    can->change(can->but->uPacket, win->xOff, win->yOff,
		win->logicalW, win->logicalH, win->w, win->h);
  }    
  if (can->parent->butIn == can->but)  {
    butWin_mMove(win->env->last_mwin, win->env->last_mx, win->env->last_my);
  }
}


void  butCan_destroy(ButWin *win)  {
}


static ButOut  mmove(But *but, int x, int y)  {
  Can  *can = but->iPacket;
  ButWin  *win = can->win;
  ButEnv  *env = but->win->env;
  ButOut  result;
  int  new_ox, new_oy;

  assert(MAGIC(but));
  assert(MAGIC(can));
  if (can->grabbed)  {
    if (env->last_mwin != but->win)  {
      return(0);
    }
    new_ox = can->grab_ox + 10*(can->grab_mx - x);
    new_oy = can->grab_oy + 10*(can->grab_my - y);
    if ((new_ox != win->xOff) || (new_oy != win->yOff))  {
      butCan_slide(can->win, new_ox, new_oy, TRUE);
    }
    env->last_mwin = but->win;
    env->last_mx = x - but->x + win->xOff;
    env->last_my = y - but->y + win->yOff;
    return(BUTOUT_CAUGHT);
  } else  {
    result = butWin_mMove(can->win, x-but->x+win->xOff, y-but->y+win->yOff);
    return(result | BUTOUT_CAUGHT | BUTOUT_IGNORE);
  }
}


static ButOut  mleave(But *but)  {
  /*
   * This isn't quite right...not sure how to handle this.
   *   I don't want to get a "leave" when I propogate a mouse move down
   *   to the area below, but I get one anyway!
   * can_t  *can = but->iPacket;
   * int  result;
   *
   * assert(MAGIC(can));
   * result = but_mmove(but->win->env, can->win, 1,-1);
   * return(result);
   */
  return(0);
}


static ButOut  mpress(But *but, int butnum, int x, int y)  {
  Can  *can = but->iPacket;
  ButWin  *win = can->win;
  int  result;

  assert(MAGIC(can));
  if (butnum == 3)  {
    can->grabbed = TRUE;
    can->grab_mx = x;
    can->grab_my = y;
    can->grab_ox = win->xOff;
    can->grab_oy = win->yOff;
    butEnv_setCursor(but->win->env, but, butCur_grab);
    but_newFlags(but, but->flags | BUT_LOCKED);
    return(BUTOUT_CAUGHT);
  } else  {
    result = butWin_mPress(can->win, x - but->x + win->xOff,
			   y - but->y + win->yOff, butnum);
    return(result);
  }
}


static ButOut  mrelease(But *but, int butnum, int x, int y)  {
  Can  *can = but->iPacket;
  ButWin  *win = can->win;
  ButOut  result;

  assert(MAGIC(can));
  if (butnum == 3)  {
    can->grabbed = FALSE;
    but_newFlags(but, but->flags & ~BUT_LOCKED);
    butEnv_setCursor(but->win->env, but, butCur_idle);
    return(BUTOUT_CAUGHT);
  } else  {
    result = butWin_mRelease(can->win, x - but->x + win->xOff,
			     y - but->y + win->yOff, butnum);
    return(result);
  }
}


static ButOut  kpress(But *but, const char *keystr, KeySym sym)  {
  Can  *can = but->iPacket;
  ButOut  result;

  assert(MAGIC(can));
  result = butWin_kPress(can->win, keystr, sym);
  return(result);
}


static ButOut  krelease(But *but, const char *keystr, KeySym sym)  {
  Can  *can = but->iPacket;
  ButOut  result;

  assert(MAGIC(can));
  result = butWin_kRelease(can->win, keystr, sym);
  return(result);
}


static void  draw(But *but, int x, int y, int w, int h)  {
  Can  *can = but->iPacket;
  ButWin  *win = but->win;
  ButEnv  *env = win->env;
  int  csx,csy, cdx,cdy, cw,ch;
  
  assert(MAGIC(can));
  assert(MAGIC(win));
  csx = 0;
  csy = 0;
  cdx = but->x;
  cdy = but->y;
  cw = but->w;
  ch = but->h;
  if (x > cdx)  {
    csx += x - cdx;
    cw  -= x - cdx;
    cdx = x;
  }
  if (y > cdy)  {
    csy += y - cdy;
    ch  -= y - cdy;
    cdy = y;
  }
  if (x+w < cdx+cw)
    cw = x+w-cdx;
  if (y+h < cdy+ch)
    ch = y+h-cdy;
  if ((cw > 0) && (ch > 0))
    XCopyArea(env->dpy, can->pmap, win->win, env->gc, csx,csy, cw,ch, cdx,cdy);
}


void  butCan_redrawn(ButWin *win, int x,int y, int w,int h)  {
  Can  *can = win->iPacket;

  assert(MAGIC(win));
  if (can == NULL)
    return;
  assert(MAGIC(can));
  if (can->but == NULL)
    return;
  assert(MAGIC(can->but));
  assert(MAGIC(can->parent));
  if (x < win->xOff)  {
    w -= win->xOff - x;
    x = win->xOff;
  }
  if (y < win->yOff)  {
    h -= win->yOff - y;
    y = win->yOff;
  }
  if (x + w > win->xOff + win->w)
    w = win->xOff + win->w - x;
  if (y + h > win->yOff + win->h)
    h = win->yOff + win->h - y;
  butWin_redraw(can->parent, can->but->x + x - win->xOff,
		can->but->y + y - win->yOff, w, h);
}


static ButOut  destroy(But *but)  {
  Can  *can = but->iPacket;

  assert(MAGIC(but));
  assert(MAGIC(can));
  assert((can->win == NULL) || MAGIC(can->win));
  can->but = NULL;
  but->iPacket = NULL;
  if (can->win == NULL)  {
    MAGIC_UNSET(can);
    wms_free(can);
  } else
    butWin_destroy(can->win);
  return(0);
}


void  butCan_winDead(ButWin *win)  {
  Can  *can = win->iPacket;

  assert(MAGIC(win));
  assert(MAGIC(can));
  assert((can->but == NULL) || MAGIC(can->but));
  can->win = NULL;
  win->iPacket = NULL;
  if (can->but == NULL)  {
    MAGIC_UNSET(can);
    wms_free(can);
  } else
    but_destroy(can->but);
}


void  butCan_resizeWin(ButWin *win, int w, int h, bool propagate)  {
  Can  *can = win->iPacket;
  int  oldW, oldH;

  assert(MAGIC(win));
  assert(MAGIC(can));
  oldW = win->logicalW;
  oldH = win->logicalH;
  if (w < win->w)
    w = win->w;
  if (h < win->h)
    h = win->h;
  win->logicalW = w;
  win->logicalH = h;
  reviseDims(can);
  if ((win->logicalW <= 0) || (win->logicalH <= 0))
    return;
  if ((win->logicalW != oldW) || (win->logicalH != oldH))  {
    win->resize(win);
    if (propagate && can->change)  {
      can->change(can->but->uPacket, win->xOff, win->yOff,
		  win->logicalW, win->logicalH, win->w, win->h);
    }
  }
}


Window  butCan_xWin(ButWin *win)  {
  assert(MAGIC(win));
  while (win->parent)  {
    win = win->parent;
    assert(MAGIC(win));
  }
  return(win->win);
}
  

static bool  reviseDims(Can *can)  {
  ButWin  *win = can->win;
  bool change = FALSE;

  if (win->logicalW < win->w)  {
    change = TRUE;
    win->logicalW = win->w;
  }
  if (win->logicalH < win->h)  {
    change = TRUE;
    win->logicalH = win->h;
  }
  if (win->xOff < 0)  {
    change = TRUE;
    win->xOff = 0;
  } else if (win->xOff + win->w > win->logicalW)  {
    change = TRUE;
    win->xOff = win->logicalW - win->w;
  }
  if (win->yOff < 0)  {
    change = TRUE;
    win->yOff = 0;
  } else if (win->yOff + win->h > win->logicalH)  {
    change = TRUE;
    win->yOff = win->logicalH - win->h;
  }
  return(change);
}


#endif  /* X11_DISP */
