/*
 * $Source: /cvsroot/cgoban1/cgoban1/wmslib/src/but/i_win.c,v $
 * $Revision: 1.2 $
 * $Date: 2000/02/26 23:03:49 $
 *
 * wmslib/src/but/i_win.c, part of wmslib (Library functions)
 * Copyright © 1994-2000 William Shubert.
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
#include <but/canvas.h>

/**********************************************************************
 * Forward declarations
 **********************************************************************/
static ButWin  *create(void *packet, ButEnv *env, const char *name,
		       int w, int h, ButWinFunc *resize);


/**********************************************************************
 * Functions
 **********************************************************************/
ButWin  *butWin_iCreate(void *packet, ButEnv *env,
			const char *name, int w, int h,
			ButWin **iWin, bool iconic, int iW, int iH,
			ButWinFunc *unmap,
			ButWinFunc *map,
			ButWinFunc *resize,
			ButWinFunc *iResize,
			ButWinFunc *destroy)  {
  ButWin  *result;

  result = create(packet, env, name, w, h, resize);
  result->unmap = unmap;
  result->map = map;
  result->destroy = destroy;
  result->quit = NULL;
  if (iWin)  {
    *iWin = create(packet, env, name, iW, iH, iResize);
    (*iWin)->isIcon = TRUE;
    result->iconWin = *iWin;
    result->iconic = iconic;
    butWin_activate(*iWin);
  }
  return(result);
}


static ButWin  *create(void *packet, ButEnv *env, const char *name,
		       int w, int h, ButWinFunc *resize)  {
  ButWin  *win;

  win = wms_malloc(sizeof(ButWin));
  MAGIC_SET(win);
  MAGIC_SET(&win->butsNoDraw);
  win->parent = NULL;
  win->parentBut = NULL;
  win->packet = packet;
  win->iPacket = NULL;
  win->win = None;
  win->physWin = None;
  win->iconWin = NULL;

  win->name = name;
  win->iconic = FALSE;
  win->isIcon = FALSE;
  win->x = int_max;
  win->y = int_max;
  win->w = win->minW = win->maxW = win->logicalW = w;
  win->h = win->minH = win->maxH = win->logicalH = h;
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
  win->mapped = FALSE;
  win->unmap = NULL;
  win->map = NULL;
  win->resize = resize;
  win->destroy = NULL;
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
  return(win);
}


void  butWin_setGeom(ButWin *win, const char *geometry)  {
  char  xdir = '+', ydir = '+';
  int  pcount;
  int  defscr = DefaultScreen(win->env->dpy);

  assert(MAGIC(win));
  if (geometry[0] == '=')
    ++geometry;
  if ((geometry[0] == '+') || (geometry[0] == '-'))  {
    pcount = sscanf(geometry, "%c%d%c%d", &xdir, &win->x, &ydir, &win->y);
    if ((pcount != 4) ||
	((xdir != '-') && (xdir != '+')) ||
	((ydir != '-') && (ydir != '+')))  {
      fprintf(stderr, "Error: Cannot understand geometry \"%s\".\n",
	      geometry);
      abort();
    }
  } else  {
    pcount = sscanf(geometry, "%dx%d%c%d%c%d", &win->w, &win->h,
		    &xdir, &win->x, &ydir, &win->y);
    if (((pcount != 2) && (pcount != 6)) ||
	((xdir != '-') && (xdir != '+')) ||
	((ydir != '-') && (ydir != '+')))  {
      fprintf(stderr, "Error: Cannot understand geometry \"%s\".\n",
	      geometry);
      abort();
    }
  }
  if (xdir == '-')
    win->x = DisplayWidth(win->env->dpy, defscr) - win->x - win->w;
  if (ydir == '-')
    win->y = DisplayHeight(win->env->dpy, defscr) - win->y - win->h;
  butWin_checkDims(win);
}


void  butWin_setX(ButWin *win, int x)  {
  assert(MAGIC(win));
  win->x = x;
  butWin_checkDims(win);
}


void  butWin_setY(ButWin *win, int y)  {
  assert(MAGIC(win));
  win->y = y;
  butWin_checkDims(win);
}


void  butWin_setMinW(ButWin *win, int minW)  {
  assert(MAGIC(win));
  win->minW = minW;
  butWin_checkDims(win);
}


void  butWin_setMinH(ButWin *win, int minH)  {
  assert(MAGIC(win));
  win->minH = minH;
  butWin_checkDims(win);
}


void  butWin_setMaxW(ButWin *win, int maxW)  {
  assert(MAGIC(win));
  if (maxW == 0)
    maxW = int_max / 2;
  win->maxW = maxW;
  butWin_checkDims(win);
}


void  butWin_setMaxH(ButWin *win, int maxH)  {
  assert(MAGIC(win));
  if (maxH == 0)
    maxH = int_max / 2;
  win->maxH = maxH;
  butWin_checkDims(win);
}


void  butWin_setWHRatio(ButWin *win, int w, int h)  {
  assert(MAGIC(win));
  win->minWRatio = w;
  win->maxWRatio = w;
  win->minHRatio = h;
  win->maxHRatio = h;
  butWin_checkDims(win);
}


void  butWin_setWHRatios(ButWin *win,
			 int minW, int minH, int maxW, int maxH)  {
  assert(MAGIC(win));
  assert(minW / minH <= maxW / maxH);
  assert(minH / minW >= maxH / maxW);
  win->minWRatio = minW;
  win->minHRatio = minH;
  win->maxWRatio = maxW;
  win->maxHRatio = maxH;
  butWin_checkDims(win);
}


void  butWin_setWStep(ButWin *win, int wStep)  {
  assert(MAGIC(win));
  win->wStep = wStep;
  butWin_checkDims(win);
}


void  butWin_setHStep(ButWin *win, int hStep)  {
  assert(MAGIC(win));
  win->hStep = hStep;
  butWin_checkDims(win);
}


void  butWin_activate(ButWin *win)  {
  ButEnv  *env = win->env;
  Display *dpy = env->dpy;
  uint  hintmask = USSize;
  int  defscr = DefaultScreen(dpy);
  XSizeHints  *sizeHints;
  XSetWindowAttributes  winattr;
  XWMHints  wm_hints;
  XClassHint  class_hints;
  XTextProperty windowName, iconName;
  static char  *dumb_argv[] = {NULL};
  static Atom  protocols[1];
  /* Stupid X doesn't call their strings const always. */
  char  *annoyance;
  char  annoy2[] = "Basicwin";

  sizeHints = XAllocSizeHints();
  if (win->x != int_max)  {
    hintmask |= USPosition;
    if (win->x + win->w / 2 < 0)
      win->x = -win->w / 2;
    if (win->x + win->w / 2 > env->rootW)
      win->x = env->rootW - win->w/2;
    if (win->y + win->h / 2 < 0)
      win->y = -win->h/2;
    if (win->y + win->h / 2 > env->rootH)
      win->y = env->rootH - win->h / 2;
  }
  winattr.background_pixel = env->colors[BUT_BG];
  winattr.event_mask = ExposureMask | StructureNotifyMask;
  winattr.bit_gravity = NorthWestGravity;
  if (win->isIcon)  {
    win->win = XCreateWindow(dpy, DefaultRootWindow(dpy), win->x,win->y,
			     win->w, win->h, 0, env->depth,
			     InputOutput, CopyFromParent,
			     CWBackPixel|CWEventMask|CWBitGravity, &winattr);
    win->physWin = win->win;
    XSelectInput(dpy, win->win, ExposureMask|StructureNotifyMask);
  } else  {
    winattr.event_mask |= PointerMotionMask | ButtonPressMask | KeyPressMask |
      ButtonReleaseMask | KeyReleaseMask | LeaveWindowMask | FocusChangeMask;
    win->win = XCreateWindow(dpy, RootWindow(dpy, defscr), win->x,win->y,
			     win->w,win->h, 0, env->depth,
			     InputOutput, CopyFromParent,
			     CWBackPixel|CWEventMask|CWBitGravity, &winattr);
    win->physWin = win->win;
    sizeHints->flags = PSize | PMinSize | PMaxSize | hintmask;
    sizeHints->max_width = win->maxW;
    sizeHints->max_height = win->maxH;
    sizeHints->min_width = win->minW;
    sizeHints->min_height = win->minH;
    if (win->minWRatio != 0)  {
      sizeHints->flags |= (PAspect | PBaseSize);
      sizeHints->min_aspect.x = win->minWRatio;
      sizeHints->min_aspect.y = win->minHRatio;
      sizeHints->max_aspect.x = win->maxWRatio;
      sizeHints->max_aspect.y = win->maxHRatio;
      sizeHints->base_width = 0;
      sizeHints->base_height = 0;
    }
    if ((win->wStep != 1) || (win->hStep != 1))  {
      sizeHints->flags |= PResizeInc;
      sizeHints->width_inc = win->wStep;
      sizeHints->height_inc = win->hStep;
    }
    wm_hints.flags = StateHint | InputHint;
    if (win->iconWin != None)  {
      wm_hints.flags |= IconWindowHint;
      wm_hints.icon_window = win->iconWin->win;
    }
    annoyance = wms_malloc(strlen(win->name) + 1);
    strcpy(annoyance, win->name);
    if (XStringListToTextProperty(&annoyance, 1, &iconName) == 0)  {
      fprintf(stderr, "Cannot allocate icon name.\n");
      abort();
    }
    if (XStringListToTextProperty(&annoyance, 1, &windowName) == 0)  {
      fprintf(stderr, "Cannot allocate window name.\n");
      abort();
    }
    if (win->iconic)
      wm_hints.initial_state = IconicState;
    else
      wm_hints.initial_state = NormalState;
    wm_hints.input = True;
    
    class_hints.res_name = annoyance;
    class_hints.res_class = annoy2;
    XSetWMProperties(dpy, win->win, &windowName, &iconName, dumb_argv, 0,
		     sizeHints, &wm_hints, &class_hints);
    protocols[0] = but_wmDeleteWindow;
    XSetWMProtocols(dpy, win->win, protocols, 1);
    XMapWindow(dpy, win->win);
    wms_free(annoyance);
  }
  XFree(sizeHints);
  butWin_addToTable(win);
}


void  butWin_destroy(ButWin *win)  {
  if (but_inEvent)  {
    butWin_dList(win);
  } else  {
    while (butWin_delete(win) == BUTOUT_STILLBUTS);
  }
}


ButOut  butWin_delete(ButWin *win)  {
  ButEnv  *env = win->env;
  ButOut  result = 0;
  int  i, j;
  ButSet  bset;

  assert(MAGIC(win));
  if (win->destroy != NULL)  {
    /*
     * This must be done before killing the buttons in case the user wants
     *   to read some buttons before they go away.
     */
    result |= win->destroy(win);
    win->destroy = NULL;
  }
  butWin_findButSetInRegion(win, int_min/2,int_min/2, int_max,int_max, &bset);
  but_inEvent = TRUE;
  if ((bset.numButs != 0) || (win->butsNoDraw.numButs != 0))  {
    for (i = 0;  i < bset.numButs;  ++i)  {
      but_destroy(bset.buts[i]);
    }
    for (i = 0;  i < win->butsNoDraw.numButs;  ++i)  {
      but_destroy(win->butsNoDraw.buts[i]);
    }
    butSet_destroy(&bset);
    win->butsNoDraw.numButs = 0;
    return(BUTOUT_STILLBUTS);
  }
  if (win->parent)
    butCan_winDead(win);
  but_inEvent = FALSE;
  butSet_destroy(&bset);
  butSet_destroy(&win->butsNoDraw);
  /* Remove all timers from the timer list. */

  /* Clear out the table. */
  /* Since there can be no buttons left, you can easily wms_free the table. */
  for (i = 0;  i < win->numCols;  ++i)  {
    assert(MAGIC(&win->cols[i]));
    MAGIC_UNSET(&win->cols[i]);
    for (j = 0;  j < win->cols[i].numRows;  ++j)  {
      assert(MAGIC(&win->cols[i].rows[j]));
      MAGIC_UNSET(&win->cols[i].rows[j]);
      if (win->cols[i].rows[j].buts != NULL)
	wms_free(win->cols[i].rows[j].buts);
    }
    wms_free(win->cols[i].rows);
  }
  wms_free(win->cols);
  if (env->curwin == win->win)
    env->curwin = None;
  if (win->parent)
    XFreePixmap(env->dpy, win->win);
  else
    XDestroyWindow(env->dpy, win->win);
  if (env->last_mwin == win)
    env->last_mwin = NULL;
  butWin_rmFromTable(win);
  MAGIC_UNSET(&win->butsNoDraw);
  MAGIC_UNSET(win);
  wms_free(win);
  return(result);
}


/*
 * Should only be called by a fixed size window.
 */
void  butWin_resize(ButWin *win, int newW, int newH)  {
  XSizeHints  *sizeHints;

  win->minW = win->maxW = newW;
  win->minH = win->maxH = newH;
  sizeHints = XAllocSizeHints();
  sizeHints->flags = PMaxSize | PMinSize | PSize;
  sizeHints->max_width  = sizeHints->min_width  = sizeHints->width  = newW;
  sizeHints->max_height = sizeHints->min_height = sizeHints->height = newH;
  XSetWMNormalHints(win->env->dpy, win->win, sizeHints);
  XFree(sizeHints);
  XResizeWindow(win->env->dpy, win->win, newW, newH);
}


/*
 * Some window managers (like eXceed/NT) ignore the ratio parts of
 * a window.  This could break some programs, so here we just ignore
 * parts of a window if that's the case.  It's ugly, but hey, get
 * a real window manager.
 */
void  butWin_checkDims(ButWin *win)  {
  if (win->w > win->env->rootW)
    win->w = win->env->rootW;
  if (win->h > win->env->rootH)
    win->h = win->env->rootH;
  if (win->w < win->minW)
    win->w = win->minW;
  if (win->w > win->maxW)
    win->w = win->maxW;
  if (win->h < win->minH)
    win->h = win->minH;
  if (win->h > win->maxH)
    win->h = win->maxH;
  win->w -= ((win->w - win->minW) % win->wStep);
  win->h -= ((win->h - win->minH) % win->hStep);
  if (win->minWRatio)  {
    if ((win->w + 1) * win->minHRatio - 1 < win->h * win->minWRatio)
      win->w = (win->h * win->minWRatio) / win->minHRatio;
    if ((win->h + 1) * win->maxWRatio - 1 < win->w * win->maxHRatio)
      win->h = (win->w * win->maxHRatio) / win->maxWRatio;
  }
}


ButColor  butColor_create(int r, int g, int b, int grey)  {
  ButColor  result;
  
  result.red   = (65535L * r + 127) / 255;
  result.green = (65535L * g + 127) / 255;
  result.blue  = (65535L * b + 127) / 255;
  result.greyLevel = grey;
  return(result);
}


ButColor  butColor_mix(ButColor c1, int r1, ButColor c2, int r2)  {
  ButColor  res;
  
  res.red   = ((c1.red   * r1) + (c2.red   * r2) + (r1 + r2) / 2) / (r1 + r2);
  res.green = ((c1.green * r1) + (c2.green * r2) + (r1 + r2) / 2) / (r1 + r2);
  res.blue  = ((c1.blue  * r1) + (c2.blue  * r2) + (r1 + r2) / 2) / (r1 + r2);
  res.greyLevel = c1.greyLevel;
  return(res);
}


#endif  /* X11_DISP */
