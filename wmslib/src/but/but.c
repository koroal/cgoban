/*
 * wmslib/src/but/but.c, part of wmslib (Library functions)
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
#include <wms.h>
#include <but/but.h>
#include <but/net.h>
#include <but/timer.h>
#include <wms/str.h>


/**********************************************************************
 * Types
 **********************************************************************/
typedef struct MouseEventFinder_struct  {
  bool  found;
  Window  win;
} MouseEventFinder;


#if  XlibSpecificationRelease < 5
/*
 * Older (X11R4 and earlier) releases of X11 used a char * where now an
 *   XPointer is used.
 */
typedef char  *XPointer;
#endif


/**********************************************************************
 * Forward declarations
 **********************************************************************/
static ButOut  keyPress(ButEnv *env, XKeyPressedEvent *evt);
static ButOut  keyRelease(ButEnv *env, XKeyReleasedEvent *evt);
static ButOut  handleEvent(ButEnv *env);
static int  activateEvent(ButEnv *env);
static bool  butEnv_stdColors(ButEnv *env);
static void  getWinXY(Display *dpy, ButWin *win);
static ButOut  serviceXData(void *packet, int fd);
static bool  performQueuedWinStuff(ButEnv *env);
static Bool  anotherMouseEvent(Display *dpy, XEvent *ev, XPointer arg);
#if  DEBUG
static int  butErrors(Display *dpy, XErrorEvent *err);
#endif


/**********************************************************************
 * Globals
 **********************************************************************/
ButTimer  *but_timerList;
Atom  but_wmDeleteWindow, but_wmProtocols;

static fd_set  emptyfds;
static struct timeval  long_timeout;

static void  makeStripes(ButEnv *env, int ssize);

/*
 * This array really should be const, but an error in my Xlib.h forces me
 *   to leave it non-const.  :-(
 */
static char  greymaps[17][4] = {
  {0x00, 0x00, 0x00, 0x00},
  {0x01, 0x00, 0x00, 0x00},
  {0x01, 0x00, 0x04, 0x00},
  {0x05, 0x00, 0x04, 0x00},
  {0x05, 0x00, 0x05, 0x00},
  {0x05, 0x02, 0x05, 0x00},
  {0x05, 0x02, 0x05, 0x08},
  {0x05, 0x0a, 0x05, 0x08},
  {0x05, 0x0a, 0x05, 0x0a},
  {0x07, 0x0a, 0x05, 0x0a},
  {0x07, 0x0a, 0x0d, 0x0a},
  {0x0f, 0x0a, 0x0d, 0x0a},
  {0x0f, 0x0a, 0x0f, 0x0a},
  {0x0f, 0x0b, 0x0f, 0x0a},
  {0x0f, 0x0b, 0x0f, 0x0e},
  {0x0f, 0x0f, 0x0f, 0x0e},
  {0x0f, 0x0f, 0x0f, 0x0f}};


/**********************************************************************
 * Functions
 **********************************************************************/
/* Returns false if the display can't be opened. */
ButEnv  *butEnv_create(const char *protocol,
		       const char *dpyname, int shutdown(Display *dpy))  {
  Display  *dpy;
  ButEnv  *env;

  env = (ButEnv *)wms_malloc(sizeof(ButEnv));
  MAGIC_SET(env);
  if ((dpy = env->dpy = XOpenDisplay(dpyname)) == NULL)  {
    MAGIC_UNSET(env);
    wms_free(env);
    return(NULL);
  }
#if  DEBUG
  XSynchronize(dpy, True);
  XSetErrorHandler(butErrors);
#endif
  env->protocol = (char *)wms_malloc(strlen(protocol)+1);
  /* Probably not a good idea to silently truncate the protocol like this. */
  if (strlen(protocol) > BUTNET_MAXCMD)
    env->protocol[BUTNET_MAXCMD] = '\0';
  strcpy(env->protocol, protocol);
  if (shutdown != NULL)
    XSetIOErrorHandler(shutdown);
  env->shutdown = shutdown;
  env->last_mwin = 0;
  env->last_mx = 0;
  env->last_my = 0;
  return(env);
}


ButEnv  *butEnv_createNoDpy(const char *protocol)  {
  ButEnv  *env;

  env = (ButEnv *)wms_malloc(sizeof(ButEnv));
  MAGIC_SET(env);
  env->dpy = NULL;

  return(env);
}


/*
 * Returns:
 *   0 - Black and white display or color=FALSE.
 *   1 - Couldn't allocate standard colors.  Failed.
 *   2 - Color successful.
 *   3 - Truecolor display.  Color will always be successful.
 */
int  butEnv_init(ButEnv *env, void *packet, const char *atomname,
		 bool color)  {
  Display  *dpy;
  static int  firstInit = TRUE;
  int  i;
  XGCValues  defGc;
  unsigned long  gcVals;
  int  retval;
  XVisualInfo  xvi, *vlr;
  int  depth;
  Window  dummyRoot;
  unsigned int  tempW, tempH, dummyBWRet, dummyDepth;
  int  dummyX, dummyY;

  dpy = env->dpy;
  env->fonts = NULL;
  env->colors = NULL;
  env->colorPmaps = NULL;
  env->winlist = NULL;
  if (dpy)  {
    but_wmDeleteWindow = XInternAtom(dpy, "WM_DELETE_WINDOW", 0);
    but_wmProtocols = XInternAtom(dpy, "WM_PROTOCOLS", 0);
    depth = DefaultDepth(dpy, DefaultScreen(dpy));
    env->depth = depth;
  }
  if (firstInit)  {
    firstInit = FALSE;
    but_timerList = NULL;
    FD_ZERO(&emptyfds);
    long_timeout.tv_sec = 60 * 60 * 24 * 365;  /* 1 year. */
    long_timeout.tv_usec = 0;
  }
  if (dpy && color)  {
    xvi.visual = DefaultVisual(dpy, DefaultScreen(dpy));
    xvi.visualid = XVisualIDFromVisual(xvi.visual);
    vlr = XGetVisualInfo(dpy, VisualIDMask, &xvi, &i); 
    if (vlr[0].class == TrueColor)
      retval = 3;
    else if (vlr[0].class >= 2)  {  /* A color class */
      retval = 2;
    } else  {  /* B&W */
      retval = 0;
      color = FALSE;
    }
    XFree(vlr);
  } else
    retval = 0;
  env->packet = packet;
  if (dpy)  {
    env->prop = XInternAtom(dpy, atomname, False);
    XGetGeometry(dpy, DefaultRootWindow(dpy), &dummyRoot, &dummyX, &dummyY,
		 &tempW, &tempH, &dummyBWRet, &dummyDepth);
    env->rootW = tempW;
    env->rootH = tempH;
  }
  env->sReq = NULL;
  env->sClear = NULL;
  env->sNotify = NULL;
  env->winlist = NULL;
  env->wllen = env->wlmax = 0;
  env->minWindows = 1;
  env->butIn = NULL;
  env->lockBut = NULL;
  env->last_mwin = NULL;
  env->stipDisable = None;
  for (i = 0;  i < BUTWRITE_MAXCHARS;  ++i)
    env->write[i].draw = NULL;
  env->keyModifiers = 0;
  env->eventNum = 0;
  env->maxFd = 0;
  for (i = 0;  i < 3;  ++i)  {
    env->maxGFds[i] = 0;
    FD_ZERO(&env->fMasks[i]);
    env->fCallbacks[i] = NULL;
  }
  if (dpy)
    butEnv_addFile(env, BUT_READFILE, ConnectionNumber(dpy),
		   env, serviceXData);
  else
    return(retval);

  defGc.function = GXcopy;
  defGc.plane_mask = AllPlanes;
  defGc.line_style = LineSolid;
  defGc.cap_style = CapButt;
  defGc.join_style = JoinMiter;
  if (color)
    defGc.fill_style = FillSolid;
  else
    defGc.fill_style = FillTiled;
  defGc.fill_rule = EvenOddRule;
  defGc.graphics_exposures = False;
  gcVals = GCFunction | GCPlaneMask | GCLineStyle | GCCapStyle |
    GCJoinStyle | GCFillStyle | GCFillRule | GCGraphicsExposures;
  env->gc = XCreateGC(dpy, RootWindow(dpy, DefaultScreen(dpy)),
		      gcVals, &defGc);
  env->gc2 = XCreateGC(dpy, RootWindow(dpy, DefaultScreen(dpy)),
		       gcVals, &defGc);

  env->numFonts = 1;
  env->fonts = (XFontStruct **)wms_malloc(env->numFonts *
					  sizeof(XFontStruct *));
  for (i = 0;  i < env->numFonts;  ++i)  {
    env->fonts[i] = NULL;
    butEnv_setFont(env, 0, "fixed", 0);
  }
  env->colorp = color;
  env->numColors = BUT_DCOLORS;
  env->colors = (ulong *)wms_malloc(env->numColors * sizeof(ulong));
  env->colorPmaps = (Pixmap *)wms_malloc(env->numColors * sizeof(Pixmap));
  if (!butEnv_stdColors(env))  {
    retval = 1;
    env->colorp = FALSE;
    XSetFillStyle(dpy, env->gc, FillTiled);
    XSetFillStyle(dpy, env->gc2, FillTiled);
    butEnv_stdColors(env);
  }
  env->partner = 0;
  env->numPartners = 0;
  env->partners = NULL;
  env->maxButIds = env->maxWinIds = 0;
  env->id2But = NULL;
  env->id2Win = NULL;
  butEnv_rcInit(env);
  return(retval);
}


void  butEnv_destroy(ButEnv *env)  {
  int  i;

  assert(MAGIC(env));
  while (env->wllen != 0)  {
    but_inEvent = TRUE;
    for (i = 0;  i < env->wllen;  ++i)
      butWin_destroy(env->winlist[i]);
    but_inEvent = FALSE;
    while (but_dList(NULL) || butWin_dList(NULL));
  }
  for (i = 0;  i < env->numPartners;  ++i)  {
    if (env->partners[i] != NULL)  {
      butRnet_destroy(env->partners[i], "Remote user has quit the program.");
    }
  }
  for (i = 0; i < 3; ++i) {
    if (env->fCallbacks[i] != NULL) {
      wms_free(env->fCallbacks[i]);
    }
  }
  if (env->fonts != NULL) {
    for (i = 0; i < env->numFonts; ++i)
      XFreeFont(env->dpy, env->fonts[i]);
    wms_free(env->fonts);
  }
  if (env->colors != NULL)
    wms_free(env->colors);
  if (env->colorPmaps != NULL)
    wms_free(env->colorPmaps);
  if (env->winlist != NULL)
    wms_free(env->winlist);
  if (env->protocol != NULL)
    wms_free(env->protocol);
  MAGIC_UNSET(env);
  XFreeGC(env->dpy, env->gc2);
  XFreeGC(env->dpy, env->gc);
  XCloseDisplay(env->dpy);
  wms_free(env);
}


void  butEnv_events(ButEnv *env)  {
  int  i, selected_fds, fdGroup;
  fd_set  fdSets[3];
  struct timeval  next_timer;
  ButOut  result, temp;

  if (env->dpy)
    butEnv_rcActivate(env);
  for (;;)  {
    result = 0;
    if (env->dpy)  {
      do  {
	XFlush(env->dpy);
	while (XPending(env->dpy))  {
	  result |= handleEvent(env);
	  if (result & BUTOUT_STOPWAIT)
	    return;
	}
	result |= butEnv_checkTimers(env, &next_timer);
	if (result & BUTOUT_STOPWAIT)  {
	  return;
	}
      } while(performQueuedWinStuff(env));
      if (result & BUTOUT_ERR)  {
	XBell(env->dpy, 0);
	result = 0;
      }
    }
    for (fdGroup = 0;  fdGroup < 3;  ++fdGroup)  {
      fdSets[fdGroup] = env->fMasks[fdGroup];
    }
    if (env->dpy && (env->wllen <= env->minWindows))
      return;
    selected_fds = select(env->maxFd, &fdSets[BUT_READFILE],
			  &fdSets[BUT_WRITEFILE],
			  &fdSets[BUT_XFILE], &next_timer);
    assert(selected_fds >= 0);
    if (selected_fds > 0)  {
      for (fdGroup = 0;  fdGroup < 3;  ++fdGroup)  {
	for (i = 0;  i < env->maxGFds[fdGroup];  ++i)  {
	  if (FD_ISSET(i, &fdSets[fdGroup]))  {
	    assert(env->fCallbacks[fdGroup][i].callback != NULL);
	    but_inEvent = TRUE;
	    result |= env->fCallbacks[fdGroup][i].
	      callback(env->fCallbacks[fdGroup][i].packet, i);
	    but_inEvent = FALSE;
	    do  {
	      temp = but_dList(NULL) | butWin_dList(NULL);
	      result |= temp;
	    } while (temp != 0);
	    if (env->dpy)  {
	      butEnv_rcActivate(env);
	      if (result & BUTOUT_ERR)
		XBell(env->dpy, 0);
	    }
	    if (result & BUTOUT_STOPWAIT)  {
	      return;
	    }
	  }
	} 
      }
    }
  }
}


/*
 * Do all the resizes and redraws that have been queued up now.
 * TRUE is returned if there was anything to do.
 */
static bool  performQueuedWinStuff(ButEnv *env)  {
  bool  anythingDone = FALSE;
  ButWin  *win, *winToDo, *ancestor;
  int  i;

  for (i = 0;  i < env->wllen;  ++i)  {
    win = env->winlist[i];
    if (win->resizeNeeded)  {
      anythingDone = TRUE;
      win->resize(win);
      win->resizeNeeded = FALSE;
      win->resized = TRUE;
    }
  }
  do  {
    winToDo = NULL;
    for (i = 0;  i < env->wllen;  ++i)  {
      win = env->winlist[i];
      if (win->redrawReady && win->numRedraws && win->resized)  {
	if (winToDo == NULL)  {
	  winToDo = win;
	} else  {
	  for (ancestor = win->parent;  ancestor;
	       ancestor = ancestor->parent)  {
	    if (ancestor == winToDo)
	      winToDo = win;
	  }
	}
      }
    }
    if (winToDo)  {
      anythingDone = TRUE;
      butWin_performDraws(winToDo);
    }
  } while (winToDo);
  return(anythingDone);
}


static ButOut  handleEvent(ButEnv *env)  {
  ButOut  result, temp;
  
  but_inEvent = TRUE;
  result = activateEvent(env);
  but_inEvent = FALSE;
  do  {
    temp = but_dList(NULL) | butWin_dList(NULL);
    result |= temp;
  } while (temp != 0);
  butEnv_rcActivate(env);
  return(result);
}


static int  activateEvent(ButEnv *env)  {
  Display  *dpy;
  XEvent  event, ev2;
  ButWin  *win = NULL;
  int  old_w, old_h;
  ButOut  result = 0;
  MouseEventFinder  mef;

  dpy = env->dpy;
  XNextEvent(dpy, &event);
  switch(event.type)  {
  case Expose:
    win = butEnv_findWin(env, event.xexpose.window);
    if (win != NULL)  {
      assert(win->mapped);
      butWin_redraw(win, event.xexpose.x, event.xexpose.y,
		    event.xexpose.width, event.xexpose.height);
      /* Don't do any of the redraws until the expose count hits zero. */
      win->redrawReady = (event.xexpose.count == 0);
    }
    break;
  case MapNotify:
    win = butEnv_findWin(env, event.xmap.window);
    assert(MAGICNULL(win));
    if (win == NULL)
      return(result);
    butWin_turnOnTimers(win);
    win->mapped = TRUE;
    if (win->map != NULL)
      win->map(win);
    if (!win->resized)  {
      /*
       * If you have no window manager, you won't get your ConfigureNotify
       *   when you start up, so we have to fake that first resize when
       *   we get mapped.  Yeah, it's ugly, but deal with it.
       */
      win->resizeNeeded = TRUE;
    }
    break;
  case UnmapNotify:
    win = butEnv_findWin(env, event.xunmap.window);
    if (win == NULL)
      return(result);
    butWin_turnOffTimers(win);
    win->mapped = FALSE;
    if (win->unmap != NULL)  {
      result |= win->unmap(win);
    }
    if (!win->isIcon && (win->iconWin == NULL))  {
      butWin_dList(win);
    }
    break;
  case ConfigureNotify:
    win = butEnv_findWin(env, event.xunmap.window);
    if (win == NULL)
      return(result);
    old_w = win->w;
    old_h = win->h;
    win->w = win->logicalW = event.xconfigure.width;
    win->h = win->logicalH = event.xconfigure.height;
    butWin_checkDims(win);
    if ((win->w != old_w) || (win->h != old_h) || !win->resized)
      win->resizeNeeded = TRUE;
    getWinXY(dpy, win);
    break;
  case MappingNotify:  
  case ReparentNotify:
  case DestroyNotify:  /* I should handle this correctly. */
    break;
  case ClientMessage:
    if ((event.xclient.message_type == but_wmProtocols) &&
	(event.xclient.data.l[0] == but_wmDeleteWindow))  {
      /* The WM asked this window to go away.  Bye! */
      win = butEnv_findWin(env, event.xclient.window);
      if (win == NULL)
	return(result);
      if (win->quit)
	win->quit(win);
      else
	butWin_dList(win);
    }
    break;
  case ButtonPress:
    env->eventTime = event.xbutton.time;
    env->keyModifiers = event.xbutton.state;
    ++env->eventNum;
    win = butEnv_findWin(env, event.xbutton.window);
    if (win)
      result |= butWin_mPress(win, event.xbutton.x, event.xbutton.y,
			      event.xbutton.button);
    return(result);
    break;
  case KeyRelease:
    env->eventTime = event.xbutton.time;
    env->keyModifiers = event.xbutton.state;
    return(result | keyRelease(env, &(event.xkey)));
    break;
  case ButtonRelease:
    env->eventTime = event.xbutton.time;
    env->keyModifiers = event.xbutton.state;
    win = butEnv_findWin(env, event.xbutton.window);
    if (win)
      result |= butWin_mRelease(win, event.xbutton.x, event.xbutton.y,
				event.xbutton.button);
    return(result);
    break;
  case KeyPress:
    env->eventTime = event.xbutton.time;
    env->keyModifiers = event.xbutton.state;
    ++env->eventNum;
    return(result | keyPress(env, &(event.xkey)));
    break;
  case MotionNotify:
    mef.found = FALSE;
    mef.win = event.xmotion.window;
    XCheckIfEvent(dpy, &ev2, anotherMouseEvent, (XPointer)&mef);
    if (!mef.found)  {
      win = butEnv_findWin(env, event.xmotion.window);
      if (win)  {
	env->eventTime = event.xmotion.time;
	butWin_mMove(win, event.xmotion.x, event.xmotion.y);
	butRnet_mMove(env, win->id, event.xmotion.x,event.xmotion.y,
		      win->w,win->h, -1);
      }
    }
    break;
  case LeaveNotify:
    win = butEnv_findWin(env, event.xcrossing.window);
    if (win)  {
      butWin_mMove(win, BUT_NOCHANGE,BUT_NOCHANGE);
      butRnet_mMove(env, -2, -1,-1,-1,-1,-1);
    }
    break;
  case FocusIn:
    break;
  case FocusOut:
    if (env->lockBut)  {
      if (env->lockBut->flags & BUT_KEYPRESSED)  {
	return(result | env->lockBut->action->kRelease(env->lockBut, "", 0));
      }
    }
    break;
  case SelectionRequest:
    ev2.type = SelectionNotify;
    ev2.xselection.type = SelectionNotify;
    ev2.xselection.send_event = True;
    ev2.xselection.display = event.xselectionrequest.display;
    ev2.xselection.requestor = event.xselectionrequest.requestor;
    ev2.xselection.selection = event.xselectionrequest.selection;
    ev2.xselection.target = event.xselectionrequest.target;
    if ((env->sReq == NULL) ||
	!env->sReq(env, &(event.xselectionrequest)))
      ev2.xselection.property = None;
    else
      ev2.xselection.property = event.xselectionrequest.property;
      ev2.xselection.time = event.xselectionrequest.time;
    XSendEvent(env->dpy, event.xselectionrequest.requestor, False,
	       0, &ev2);
    break;
  case SelectionNotify:
    if (env->sNotify != NULL)
      env->sNotify(env, &(event.xselection));
    break;
  case SelectionClear:
    if (env->sClear != NULL)
      env->sClear(env);
    break;
  default:
#if  DEBUG
    printf("UNKNOWN EVENT!  #%d\n", event.type);
#endif
    break;
  }
  return(result);
}


static Bool  anotherMouseEvent(Display *dpy, XEvent *ev, XPointer arg)  {
  MouseEventFinder  *mef = (MouseEventFinder *)arg;

  if (ev->type == MotionNotify)  {
    if (ev->xmotion.window == mef->win)
      mef->found = TRUE;
  } else if ((ev->type == ButtonPress) || (ev->type == ButtonRelease))  {
    if (ev->xbutton.window == mef->win)
      mef->found = TRUE;
  }
  return(False);
}


#ifndef  STR_MAXLEN
#define STR_MAXLEN  100
#endif
static ButOut  keyPress(ButEnv *env, XKeyPressedEvent *evt)  {
  int  slen;
  char  kbuf[STR_MAXLEN];
  KeySym  keysym;
  ButWin  *win = butEnv_findWin(env, evt->window);

  if (!win)
    return(0);
  assert(MAGIC(win));
  slen = XLookupString(evt, kbuf, STR_MAXLEN-1, &keysym, NULL);
  kbuf[slen] = '\0';
  if (kbuf[0] == '\r')
    kbuf[0] = '\n';
  return(butWin_kPress(win, kbuf, keysym));
}


static ButOut  keyRelease(ButEnv *env, XKeyReleasedEvent *evt)  {
  int  slen;
  char  kbuf[STR_MAXLEN];
  KeySym  keysym;
  ButWin  *win = butEnv_findWin(env, evt->window);

  if (win == NULL)
    return(0);
  slen = XLookupString(evt, kbuf, STR_MAXLEN-1, &keysym, NULL);
  kbuf[slen] = '\0';
  if (kbuf[0] == '\r')
    kbuf[0] = '\n';
  if (XPending(env->dpy))  {
    XEvent  nextev;

    XPeekEvent(env->dpy, &nextev);
    if (nextev.type == KeyPress)  {
      if ((nextev.xkey.keycode == evt->keycode) &&
	  (evt->time == nextev.xkey.time))  {
	XNextEvent(env->dpy, &nextev);
	return(butWin_kPress(win, kbuf, keysym));
      }
    }
  }
  return(butWin_kRelease(win, kbuf, keysym));
}


/* Disable all timers used in a particular window.  Useful mostly so that
 *   when you iconify a window, the timers shut off.  Even though the timers
 *   don't take much CPU time, shutting them off makes it possible to
 *   swap the entire application out of memory and this COULD have a
 *   noticeable effect on system performance if other applicaitons need
 *   lots of memory.
 * The timers will stay in the timer queue, but they will not go off.
 */
void  butWin_turnOffTimers(ButWin *win)  {
  ButTimer  *timer;

  for (timer = but_timerList;  timer != NULL;  timer = timer->next)  {
    if ((timer->win == win) && (timer->state == butTimer_on) &&
	timer->winOnly)
      timer->state = butTimer_off;
  }
}


/* Re-enable all timers for a particular window.  This will undo the work
 *   of but_turnoff_timers.
 */
void  butWin_turnOnTimers(ButWin *win)  {
  ButTimer  *timer;

  for (timer = but_timerList;  timer != NULL;  timer = timer->next)  {
    if ((timer->win == win) && (timer->state == butTimer_off) &&
	timer->winOnly)
      timer->state = butTimer_on;
  }
}


static bool  butEnv_stdColors(ButEnv *env)  {
  ButColor  colorset[BUT_DCOLORS];
  int  i, xblack, xwhite;
  ButColor  black, white;
  Display  *dpy = env->dpy;
  Window  rootwin = DefaultRootWindow(dpy);

  xblack = BlackPixel(dpy, DefaultScreen(dpy));
  xwhite = WhitePixel(dpy, DefaultScreen(dpy));
  for (i = 0;  i < 17;  ++i)  {
    env->greyMaps[i] =
      XCreatePixmapFromBitmapData(dpy, rootwin, greymaps[i], 4,4,
				  xwhite, xblack, env->depth);
  }

  black = butColor_create(0,0,0,0);
  white = butColor_create(255,255,255,16);
  colorset[BUT_FG] = black;
  colorset[BUT_BG] = butColor_mix(white,3, black,1);
  colorset[BUT_PBG] = butColor_mix(colorset[BUT_BG],7, black,1);
  colorset[BUT_PBG].greyLevel = 14;
  colorset[BUT_HIBG] = butColor_mix(colorset[BUT_BG],1, white,1);
  colorset[BUT_LIT] = butColor_create(255,255,255, 8);
  colorset[BUT_SHAD] = butColor_create(128,128,128, 0);
  colorset[BUT_ENTERBG] = colorset[BUT_HIBG];
  colorset[BUT_SELBG] = butColor_create(255,255,0,12);  /* Yellow. */
  colorset[BUT_CHOICE] = butColor_create(0,255,0, 16);
  colorset[BUT_WHITE] = white;
  colorset[BUT_BLACK] = black;
  for (i = 0;  i < BUT_DCOLORS;  ++i)  {
    env->colorPmaps[i] = None;
    if (butEnv_setColor(env, i, colorset[i]) == 0)
      return(FALSE);
  }
  return(TRUE);
}


bool  butEnv_setColor(ButEnv *env, int colornum, ButColor color)  {
  int  i;
  Display  *dpy = env->dpy;
  Colormap  cmap;
  XColor  temp;
  static uchar  bm1616[] = {1};
  Window  rootwin = DefaultRootWindow(dpy);

  if (colornum >= env->numColors)  {
    ulong  *newcolors;
    Pixmap  *newpixmaps;
    
    newcolors = (ulong *)wms_malloc((colornum+1)*sizeof(ulong));
    newpixmaps = (Pixmap *)wms_malloc((colornum+1)*sizeof(Pixmap));
    for (i = 0;  i < env->numColors;  ++i)  {
      newcolors[i] = env->colors[i];
      newpixmaps[i] = env->colorPmaps[i];
    }
    if (env->colors)
      wms_free(env->colors);
    env->colors = newcolors;
    if (env->colorPmaps)
      wms_free(env->colorPmaps);
    env->colorPmaps = newpixmaps;
    env->numColors = colornum + 1;
    for (;  i < env->numColors;  ++i)
      env->colorPmaps[i] = None;
  }
  cmap = DefaultColormap(dpy, DefaultScreen(dpy));
  if (env->colorPmaps[colornum] != None)  {
    if (env->colorp)  {
      XFreePixmap(env->dpy, env->colorPmaps[colornum]);
      XFreeColors(env->dpy, cmap, &env->colors[colornum], 1, 0);
    }
  }
  if (env->colorp)  {
    temp.red = color.red;
    temp.green = color.green;
    temp.blue = color.blue;
    temp.flags = DoRed | DoGreen | DoBlue;
    if (XAllocColor(dpy, cmap, &temp) == 0)
      return(FALSE);
    env->colors[colornum] = temp.pixel;
    env->colorPmaps[colornum] =
      XCreatePixmapFromBitmapData(dpy, rootwin, bm1616, 1,1,
				  temp.pixel,temp.pixel, env->depth);
  } else  {
    env->colorPmaps[colornum] = env->greyMaps[color.greyLevel];
    if (color.greyLevel < 16)
      env->colors[colornum] = BlackPixel(dpy, DefaultScreen(dpy));
    else
      env->colors[colornum] = WhitePixel(dpy, DefaultScreen(dpy));
  }
  return(TRUE);
}


int  butEnv_setFont(ButEnv *env, int fontnum, const char *fontname,
		    int fparam)  {
  XFontStruct  *flist;
  char  **fnames;
  int  i, f_avail, minChar;
  Str  fname, temp;
  int  cstart, fontloaded = 0;

  str_init(&fname);
  str_init(&temp);
  if (fontnum >= env->numFonts)  {
    XFontStruct  **newflist;

    newflist = (XFontStruct **)wms_malloc((fontnum+1) * sizeof(XFontStruct *));
    for (i = 0;  i < env->numFonts;  ++i)
      newflist[i] = env->fonts[i];
    wms_free(env->fonts);
    env->fonts = newflist;
    env->numFonts = fontnum + 1;
    for (;  i < env->numFonts;  ++i)
      env->fonts[i] = NULL;
  }
  if (env->fonts[fontnum] != NULL)  {
    XFreeFont(env->dpy, env->fonts[fontnum]);
    env->fonts[fontnum] = NULL;
  }
  for (;;)  {
    ++fontloaded;
    if (*fontname == '/')
      ++fontname;
    if (*fontname == '\0')  {
      fontloaded = 0;
      fontname = "fixed";
    }
    for (cstart = 0;  (fontname[cstart] != '\0') && (fontname[cstart] != '/');
	 ++cstart);
    str_copyCharsLen(&temp, fontname, cstart);
    str_print(&fname, str_chars(&temp), fparam);
    fontname += cstart;
    fnames = XListFontsWithInfo(env->dpy, str_chars(&fname), 1,
				&f_avail, &flist);
    if (f_avail > 0)  {
      env->fonts[fontnum] = XLoadQueryFont(env->dpy, fnames[0]);
      minChar = env->fonts[fontnum]->min_char_or_byte2;
      if ((minChar > ' ') ||
	  (env->fonts[fontnum]->max_char_or_byte2 < 'z')) {
	printf("Char range: %d..%d, should be %d..%d\n",
	       env->fonts[fontnum]->min_char_or_byte2,
	       env->fonts[fontnum]->max_char_or_byte2,
	       ' ', 'z');
	XFreeFontInfo(fnames, flist, f_avail);
	continue;
      }
      if (env->fonts[fontnum]->per_char == NULL) {
	if (env->fonts[fontnum]->min_bounds.width < 1) {
	  XFreeFontInfo(fnames, flist, f_avail);
	  continue;
	}
      } else {
	if (env->fonts[fontnum]->per_char['a' - minChar].width < 1) {
	  printf("Width of a is %d\n",
		 env->fonts[fontnum]->per_char['a'].width);
	  XFreeFontInfo(fnames, flist, f_avail);
	  continue;
	}
      }
      if (fontnum == 0)  {
	env->font0h = env->fonts[0]->ascent + env->fonts[0]->descent;
	env->stdButBw = (env->font0h + 3) / 6;
	makeStripes(env, (env->font0h / 15) * 2);
      }
      XFreeFontInfo(fnames, flist, f_avail);
      str_deinit(&temp);
      str_deinit(&fname);
      return(fontloaded);
    }
  }
}


void  butEnv_drawAll(ButEnv *env)  {
  int  i;
  ButWin  *win;

  for (i = 0;  i < env->wllen;  ++i)  {
    win = env->winlist[i];
    butWin_redraw(win, 0,0, win->w,win->h);
  }
}


void  butEnv_resizeAll(ButEnv *env)  {
  int  i;
  ButWin  *win;

  for (i = 0;  i < env->wllen;  ++i)  {
    win = env->winlist[i];
    win->resize(win);
  }
}


static void  makeStripes(ButEnv *env, int ssize)  {
  uint  j;
  int  x, y;
  uchar  *stripes;

  if (ssize == 0)
    ssize = 2;
  /* "ssize" is the width of a stripe pattern used to grey out text. */
  j = (ssize + 7) / 8;
  stripes = (uchar *)wms_malloc(ssize * j);
  for (x = 0;  x < ssize*j;  ++x)
    stripes[x] = 0;
  for (y = 0;  y < ssize;  ++y)  {
    for (x = 0;  x < ssize;  ++x)  {
      if ((((ssize-x-1) >= y) && ((ssize-x-1) < y+(ssize/2))) ||
	  ((ssize-x-1) < y-(ssize/2)))
	stripes[(x>>3) + (y*j)] |= 1<<(x&7);
    }
  }
  if (env->stipDisable != None)
    XFreePixmap(env->dpy, env->stipDisable);
  env->stipDisable =
    XCreateBitmapFromData(env->dpy,
			  RootWindow(env->dpy, DefaultScreen(env->dpy)),
			  stripes, ssize,ssize);
  wms_free(stripes);
  XSetStipple(env->dpy, env->gc, env->stipDisable);
  XSetStipple(env->dpy, env->gc2, env->stipDisable);
}


#if  DEBUG
static int  butErrors(Display *dpy, XErrorEvent *err)  {
  char  ebuf[1024];

  XGetErrorText(dpy, err->error_code, ebuf, sizeof(ebuf));
  fprintf(stderr, "Error: %s\n", ebuf);
  assert(0);
}
#endif  /* DEBUG */


/*
 * You know, it's really unbelievable to me that this is the only way in
 *   X to find the location on the display of a window.  *sigh*.
 * This code was blatantly stolen from "xwininfo.c".
 */
static void  getWinXY(Display *dpy, ButWin *win)  {
  static bool  errorPrinted = FALSE;
  Status status;
  Window wmframe = win->win;
  XWindowAttributes frame_attr;

  while (True) {
    Window root, parent;
    Window *childlist;
    unsigned int ujunk;
    
    status = XQueryTree(dpy, wmframe, &root, &parent, &childlist, &ujunk);
    if (parent == root || !parent || !status)
      break;
    wmframe = parent;
    if (status && childlist)
      XFree((void *)childlist);
  }
  /* WM may be reparented, so find edges of the frame. */
  /* Only works for ICCCM-compliant WMs, and then only if the
     window has corner gravity.  We would need to know the original width
     of the window to correctly handle the other gravities. */
  if (!XGetWindowAttributes(dpy, wmframe, &frame_attr) && !errorPrinted)  {
    fprintf(stderr, "wmslib: Can't get frame attributes.");
    errorPrinted = TRUE;
  }
  win->x = frame_attr.x;
  win->y = frame_attr.y;
}  


void  butEnv_addFile(ButEnv *env, int group, int fd, void *packet,
		     ButOut (*callback)(void *packet, int fd))  {
  int  i;

  assert((group >= 0) && (group < 3));
  if (fd >= env->maxFd)
    env->maxFd = fd + 1;
  if (fd >= env->maxGFds[group])  {
    ButFdCallback *newFc = wms_malloc((fd+1)*sizeof(ButFdCallback));
    for (i = 0;  i < env->maxGFds[group];  ++i)
      newFc[i] = env->fCallbacks[group][i];
    for (;  i < fd;  ++i)
      newFc[i].callback = NULL;
    if (env->fCallbacks[group] != NULL)
      wms_free(env->fCallbacks[group]);
    env->maxGFds[group] = fd + 1;
    env->fCallbacks[group] = newFc;
  }
  env->fCallbacks[group][fd].callback = callback;
  env->fCallbacks[group][fd].packet = packet;
  FD_SET(fd, &env->fMasks[group]);
}


void  butEnv_rmFile(ButEnv *env, int group, int fd)  {
  assert((group >= 0) && (group < 3));
  assert(MAGIC(env));
  assert(fd < env->maxGFds[group]);
  env->fCallbacks[group][fd].callback = NULL;
  FD_CLR(fd, &env->fMasks[group]);
}


static ButOut  serviceXData(void *packet, int fd)  {
  ButEnv  *env = packet;

  if (XPending(env->dpy))
    return(handleEvent(env));
  else
    return(0);
}


XImage  *butEnv_imageCreate(ButEnv *env, int w, int h)  {
  Display  *dpy = env->dpy;
  XImage  *image;

  image = XCreateImage(dpy, DefaultVisual(dpy, DefaultScreen(dpy)),
		       DefaultDepth(dpy, DefaultScreen(dpy)),
		       ZPixmap, 0, NULL, w, h, 32, 0);
  image->data = wms_malloc(image->bytes_per_line * h);
  return(image);
}


void  butEnv_imageDestroy(XImage *img)  {
  wms_free(img->data);
  img->data = NULL;
  XDestroyImage(img);
}


#endif  /* X11_DISP */
