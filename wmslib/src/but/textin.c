/*
 * wmslib/src/but/textin.c, part of wmslib (Library functions)
 * Copyright (C) 1994-1997 William Shubert.
 * See "configure.h.in" for more copyright information.
 */

/*
 * I don't really like this button.
 *   It is slower than it has to be (this may or may not matter).
 */

#include <configure.h>

#ifdef  X11_DISP

#ifdef  STDC_HEADERS
#include <stdlib.h>
#include <unistd.h>
#endif  /* STDC_HEADERS */
#include <ctype.h>
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
#include <but/textin.h>
#include <but/box.h>
#include <but/timer.h>


/**********************************************************************
 * Data Types
 **********************************************************************/
typedef struct SpecKey_struct {
  struct SpecKey_struct *next;
  KeySym keysym;
  uint keyModifiers, modMask;
  ButOut (*callback)(But *but, KeySym keysym, uint keyModifiers,
		     void *context);
  void *context;
  MAGIC_STRUCT
} SpecKey;

typedef struct Txtin_struct  {
  ButOut (*callback)(But *but, const char *value);
  bool  hidden;
  char  *str, *dispStr;  /* dispStr is only used if hidden is TRUE. */
  int  maxlen, len, loc, cutend;
  int  mousePress, origMousePress, clicks;
  int  xoffset;
  Pixmap  pm;
  int  pm_w, pm_h, slideDir, slideFreq;
  int  but3StartMouse, but3StartXoff;
  bool  mouseInBut, cursorVisible;
  ButTimer  *cTimer, *sTimer;
  SpecKey  *specKeys;
  MAGIC_STRUCT
} Txtin;


/**********************************************************************
 * Forward Declarations
 **********************************************************************/
static int  locateMouse(XFontStruct *fs, char *text, int len, int x,
			bool *rightSide);
static void  ti_cursor(But *but);
static void  ti_redrawCursor(But *but);
static ButOut  mmove(But *but, int x, int y);
static ButOut  mleave(But *but);
static ButOut  mpress(But *but, int butnum, int x, int y);
static ButOut  mrelease(But *but, int butnum, int x, int y);
static ButOut  kpress(But *but, const char *keystr, KeySym sym);
static void  draw(But *but, int x,int y, int w,int h);
static ButOut  destroy(But *but);
static bool  insert(ButEnv *env, const char *src, Txtin *but);
static void  cut(ButEnv *env, char *cutstr, int cutlen, But *but);
static void  paste(ButEnv *env, But *but);
static void  enableCTimer(But *but);
static void  disableCTimer(But *but);
static void  curInView(But *but);
static void  textInView(But *but, bool maxInView);

static bool  sreq(ButEnv *env, XSelectionRequestEvent *xsre);
static int   sclear(ButEnv *env);
static int   snotify(ButEnv *env, XSelectionEvent *xsre);
static void  flags(But *but, uint flags);

static void  wordsel_adjust(Txtin *but);
static void  startSlide(But  *but, int dir, bool fast);
static ButOut  slide(ButTimer *timer);


/**********************************************************************
 * Globals
 **********************************************************************/
static int  cutstr_maxlen = 0, cutstr_len;
static char  *cutdata = NULL;
static But  *cut_butnum = NULL;
static But  *paste_butnum = NULL;
static const ButAction  action = {
  mmove, mleave, mpress, mrelease,
  kpress, NULL, draw, destroy, flags, NULL};


/**********************************************************************
 * Functions
 **********************************************************************/
But  *butTextin_create(ButOut (*callback)(But *but, const char *value),
		       void *packet, ButWin *win, int layer, int flags,
		       const char *text, int maxlen)  {
  But  *but;
  Txtin  *ti;
  
  ti = wms_malloc(sizeof(Txtin));
  MAGIC_SET(ti);
  but = but_create(win, ti, &action);
  but->uPacket = packet;
  but->layer = layer;
  but->flags = flags | BUT_OPAQUE;

  if (text == NULL)
    text = "";
  ti->callback = callback;
  ti->hidden = FALSE;
  ti->maxlen = maxlen;
  ti->str = (char *)wms_malloc(ti->maxlen + 1);
  ti->dispStr = ti->str;
  ti->xoffset = 0;
  ti->pm_w = ti->pm_h = 0;
  ti->pm = None;
  ti->mousePress = -1;
  ti->mouseInBut = FALSE;
  strcpy(ti->str, text);
  ti->len = ti->loc = ti->cutend = strlen(text);
  ti->cTimer = NULL;
  if (flags & BUT_KEYED)  {
    ti->cursorVisible = TRUE;
  } else
    ti->cursorVisible = FALSE;
  ti->sTimer = NULL;
  ti->but3StartXoff = -1;
  ti->specKeys = NULL;
  but_init(but);
  return(but);
}


const char  *butTextin_get(But *but)  {
  Txtin  *ti = but->iPacket;
  
  assert(but->action == &action);
  assert(MAGIC(ti));
  return(ti->str);
}


static ButOut  destroy(But *but)  {
  Txtin  *ti = but->iPacket;
  ButEnv  *env = but->win->env;
  SpecKey  *spec, *nextSpec;
  
  assert(but->action == &action);
  assert(MAGIC(ti));
  spec = ti->specKeys;
  while (spec != NULL) {
    assert(MAGIC(spec));
    nextSpec = spec->next;
    MAGIC_UNSET(spec);
    wms_free(spec);
    spec = nextSpec;
  }
  if (cut_butnum == but)  {
    if (env->sClear == sclear)
      env->sClear = NULL;
    if (env->sReq == sreq)
      env->sReq = NULL;
    cut_butnum = NULL;
  }
  if (paste_butnum == but)  {
    if (env->sNotify == snotify)
      env->sNotify = NULL;
    paste_butnum = NULL;
  }
  if (ti->pm != None)
    XFreePixmap(but->win->env->dpy, ti->pm);
  wms_free(ti->str);
  if (ti->hidden)
    wms_free(ti->dispStr);
  else  {
    assert(ti->dispStr == ti->str);
  }
  MAGIC_UNSET(ti);
  wms_free(ti);
  return(0);
}


static void  draw(But *but, int x,int y, int w,int h)  {
  ButWin  *win = but->win;
  ButEnv  *env = win->env;
  Txtin  *ti = but->iPacket;
  int  txtw;
  int  butbw = env->stdButBw;
  XFontStruct  *fs = env->fonts[0];
  
  assert(but->action == &action);
  assert(MAGIC(ti));
  XSetFont(env->dpy, env->gc2, env->fonts[0]->fid);
  if ((ti->pm_w != but->w - 2*butbw) || (ti->pm_h != but->h - 2*butbw))  {
    if (ti->pm != None)
      XFreePixmap(env->dpy, ti->pm);
    ti->pm = XCreatePixmap(env->dpy, win->win,
			   ti->pm_w = but->w - 2*butbw,
			   ti->pm_h = but->h - 2*butbw,
			   DefaultDepth(env->dpy, DefaultScreen(env->dpy)));
  }
  but_drawBox(win, but->x,but->y, but->w,but->h, 1, butbw,
	      BUT_SRIGHT|BUT_SLEFT, BUT_LIT, BUT_SHAD, None, None);
  txtw = XTextWidth(fs, ti->dispStr, ti->loc);
  butEnv_setXFg2(env, BUT_HIBG);
  XFillRectangle(env->dpy, ti->pm, env->gc2,
		 0,0, ti->pm_w,ti->pm_h);
  if (but->flags & BUT_PRESSABLE)
    butEnv_setXFg2(env, BUT_FG);
  else  {
    XSetFillStyle(env->dpy, env->gc2, FillStippled);
    XSetForeground(env->dpy, env->gc2, env->colors[BUT_FG]);
  }
  if (ti->loc == ti->cutend)  {
    XDrawString(env->dpy, ti->pm, env->gc2,
		x = butbw - ti->xoffset,
		y = (fs->ascent + ti->pm_h - fs->descent) / 2,
		ti->dispStr, ti->len);
  } else  {
    int  tw1, tw2;
    int  cl1, cl2;
    
    if (ti->cutend >= ti->loc)  {
      cl1 = ti->loc;
      cl2 = ti->cutend;
    } else  {
      cl1 = ti->cutend;
      cl2 = ti->loc;
    }
    XDrawString(env->dpy, ti->pm, env->gc2,
		x = butbw - ti->xoffset,
		y = (fs->ascent + ti->pm_h - fs->descent) / 2,
		ti->dispStr, cl1);
    tw1 = XTextWidth(fs, ti->dispStr, cl1);
    tw2 = XTextWidth(fs, ti->dispStr + cl1, cl2 - cl1);
    x += tw1 + tw2;
    XDrawString(env->dpy, ti->pm, env->gc2, x,y,
		ti->dispStr + cl2, ti->len - cl2);
    x -= tw2;
    XSetBackground(env->dpy, env->gc2, env->colors[BUT_SELBG]);
    if (!env->colorp)  {
      XSetTile(env->dpy, env->gc2, env->colorPmaps[BUT_WHITE]);
      XSetForeground(env->dpy, env->gc2, env->colors[BUT_WHITE]);
    }
    XDrawImageString(env->dpy, ti->pm, env->gc2, x,y,
		     ti->dispStr + cl1, cl2 - cl1);
    
  }		
  if (!(but->flags & BUT_PRESSABLE))  {
    butEnv_stdFill2(env);
  }
  XCopyArea(env->dpy, ti->pm, win->win, env->gc,
	    0,0, ti->pm_w,ti->pm_h,
	    but->x + butbw - win->xOff, but->y + butbw - win->yOff);
  if ((ti->loc == ti->cutend) && ti->cursorVisible &&
      (but->flags & BUT_PRESSABLE))
    ti_cursor(but);
}


static ButOut  mmove(But *but, int x, int y)  {
  Txtin  *ti = but->iPacket;
  ButWin  *win = but->win;
  ButEnv  *env = win->env;
  int  newMousePress;
  bool  rightSide;

  if (!(but->flags & BUT_PRESSABLE))
    return(BUTOUT_CAUGHT);
  if (!ti->mouseInBut)  {
    ti->mouseInBut = TRUE;
    butEnv_setCursor(env, but, butCur_text);
  }
  if (ti->mousePress != -1)  {
    if (ti->clicks < 2)  {
      if (x < but->x)  {
	startSlide(but, -1, (x < but->x - but->h + env->stdButBw*2));
	newMousePress = 0;
      } else if (x > but->x + but->w)  {
	startSlide(but, 1, (x > but->x + but->w + but->h - env->stdButBw*2));
	newMousePress = ti->len;
      } else  {
	if (ti->sTimer != NULL)  {
	  butTimer_destroy(ti->sTimer);
	  ti->sTimer = NULL;
	}
	if (y < but->y + env->stdButBw*2)
	  newMousePress = 0;
	else if (y > but->y+but->h - env->stdButBw*2)
	  newMousePress = ti->len;
	else
	  newMousePress = locateMouse(env->fonts[0], ti->dispStr, ti->len,
				      x - (but->x + 2*env->stdButBw -
					   ti->xoffset), &rightSide);
      }
      if (ti->clicks == 1)  {
	if (rightSide && (newMousePress >= ti->origMousePress) &&
	    (newMousePress < ti->len))
	  ++newMousePress;
	else if (!rightSide && (newMousePress <= ti->origMousePress)
		 && (newMousePress > 0))
	  --newMousePress;
      }
      if (newMousePress != ti->mousePress)  {
	ti->mousePress = ti->loc = newMousePress;
	if (ti->clicks == 1)  {
	  ti->cutend = ti->origMousePress;
	  wordsel_adjust(ti);
	}
	but_draw(but);
      }
    }
  }
  if (ti->but3StartXoff != -1)  {
    int  oldXoff = ti->xoffset;

    ti->xoffset = ti->but3StartXoff - 10*(x - ti->but3StartMouse);
    textInView(but, FALSE);
    if (oldXoff != ti->xoffset)
      but_draw(but);
  }
  return(BUTOUT_CAUGHT);
}


static ButOut  mleave(But *but)  {
  Txtin  *ti = but->iPacket;

  if (ti->mouseInBut)  {
    ti->mouseInBut = FALSE;
    butEnv_setCursor(but->win->env, but, butCur_idle);
  }
  return(BUTOUT_CAUGHT);
}


static ButOut  mpress(But *but, int butnum, int x, int y)  {
  Txtin  *ti = but->iPacket;
  ButEnv  *env = but->win->env;
  static Time  lastPressTime = -1;
  static int   lastPressNum = -2;
  int  rightSide;
  
  if (ti->cTimer)  {
    butTimer_reset(ti->cTimer);
    ti->cursorVisible = TRUE;
  }
  switch (butnum)  {
  case 1:
    /* Button 1 selects, drags, etc. */
    if (ti->but3StartXoff != -1)
      return(BUTOUT_ERR);
    if (!(but->flags & BUT_KEYED))
      but_newFlags(but, but->flags | BUT_KEYED);
    but_newFlags(but, but->flags | BUT_LOCKED);
    ti->mousePress = locateMouse(env->fonts[0], ti->dispStr, ti->len,
				 x - (but->x + 2*env->stdButBw -
				      ti->xoffset), &rightSide);
    ti->loc = ti->cutend = ti->origMousePress = ti->mousePress;
    if ((lastPressTime + BUT_DCLICK > env->eventTime) &&
	(lastPressNum + 1 == env->eventNum))  {
      if (ti->clicks == 0)  {
	/* It's a double click! */
	++ti->clicks;
	if (rightSide)
	  ++ti->loc;
	else
	  --ti->loc;
	wordsel_adjust(ti);
      } else if (ti->clicks == 1)  {
	/* It's a triple click! */
	++ti->clicks;
	ti->loc = 0;
	ti->cutend = ti->len;
      } else  {
	/* Quadruple click!  Restart! */
	ti->clicks = 0;
      }
    } else
      ti->clicks = 0;
    lastPressTime = env->eventTime;
    lastPressNum = env->eventNum;
    but_draw(but);
    break;
  case 2:
    /* Paste! */
    ti->cutend = ti->loc = locateMouse(env->fonts[0], ti->dispStr, ti->len,
				       x - (but->x + 2*env->stdButBw -
					    ti->xoffset), NULL);
    paste(env, but);
    break;
  case 3:
    /*
     * Button 3 scrolls as if this input window was a mini-canvas.  I'm not
     *   sure how useful this is but I'm doing it anyway just to make my
     *   interface more consistent.
     */
    if (ti->mousePress != -1)
      return(BUTOUT_ERR);
    but_newFlags(but, but->flags | BUT_LOCKED);
    butEnv_setCursor(env, but, butCur_lr);
    ti->but3StartMouse = x;
    ti->but3StartXoff = ti->xoffset;
    break;
  default:
    /*
     * Weird.  I don't know what to do with mice with more than 3 buttons
     *   so I guess I might as well beep.  I like beeping.
     */
    return(BUTOUT_ERR);
    break;
  }
  return(BUTOUT_CAUGHT);
}


static ButOut  mrelease(But *but, int butnum, int x, int y)  {
  Txtin  *ti = but->iPacket;
  char  *cutstr;
  int  cutlen;

  if (ti->cTimer)  {
    butTimer_reset(ti->cTimer);
    ti->cursorVisible = TRUE;
  }
  if ((butnum == 1) && (ti->mousePress != -1))  {
    if (ti->sTimer != NULL)  {
      butTimer_destroy(ti->sTimer);
      ti->sTimer = NULL;
    }
    if (ti->loc != ti->cutend)  {
      if (ti->loc < ti->cutend)  {
	cutstr = ti->str + ti->loc;
	cutlen = ti->cutend - ti->loc;
      } else  {
	cutstr = ti->str + ti->cutend;
	cutlen = ti->loc - ti->cutend;
      }      
      cut(but->win->env, cutstr, cutlen, but);
    }
    ti->mousePress = -1;
    but_newFlags(but, but->flags & ~BUT_LOCKED);
  } else if ((butnum == 3) && (ti->but3StartXoff != -1))  {
    butEnv_setCursor(but->win->env, but, butCur_text);
    ti->but3StartXoff = -1;
    but_newFlags(but, but->flags & ~BUT_LOCKED);
  }
  return(BUTOUT_CAUGHT);
}


static ButOut  kpress(But *but, const char *newtext, KeySym keysym)  {
  Txtin  *ti = but->iPacket;
  ButEnv  *env = but->win->env;
  bool  need_draw = FALSE, needCallback = FALSE;
  int  i;
  ButOut  result = 0;
  SpecKey  *spec;
  
  butTimer_reset(ti->cTimer);
  ti->cursorVisible = TRUE;
  for (spec = ti->specKeys;  spec != NULL;  spec = spec->next) {
    if ((keysym == spec->keysym) &&
	((env->keyModifiers & spec->modMask) == spec->keyModifiers)) {
      return(spec->callback(but, keysym, env->keyModifiers, spec->context) |
	     BUTOUT_CAUGHT);
    }
  }
  if (((keysym >= XK_KP_Space) && (keysym <= XK_KP_9)) ||
      ((keysym >= XK_space) && (keysym <= XK_ydiaeresis)))  {
    need_draw = TRUE;
    switch(newtext[0])  {
    case '\001':  /* Ctrl-A: Beginning of line. */
    case '\020':  /* Ctrl-P: Up a line. */
      ti->loc = ti->cutend = 0;
      break;
    case '\002':  /* Ctrl-B: Back a character. */
      if (ti->loc != ti->cutend)  {
	ti->cutend = ti->loc;
      } else if (ti->loc)  {
	ti->cutend = --ti->loc;
      } else  {
	need_draw = FALSE;
	result |= BUTOUT_ERR | BUTOUT_CAUGHT;
      }
      break;
    case '\004':  /* Ctrl-D: Delete right. */
      if (ti->loc != ti->cutend)
	need_draw = insert(env, "", ti);
      else if (ti->loc < ti->len)  {
	for (i = ti->loc;  ti->str[i];  ++i)
	  ti->str[i] = ti->str[i+1];
	--ti->len;
	need_draw = TRUE;
      } else
	result |= BUTOUT_ERR | BUTOUT_CAUGHT;
      break;
    case '\005':  /* Ctrl-E: End of line. */
    case '\016':  /* Ctrl-N: Down a line. */
      ti->loc = ti->cutend = ti->len;
      break;
    case '\006':  /* Ctrl-F: Forward a character. */
      if (ti->loc != ti->cutend)  {
	ti->loc = ti->cutend;
      } else if (ti->loc < ti->len)  {
	ti->cutend = ++ti->loc;
      } else  {
	result |= BUTOUT_ERR | BUTOUT_CAUGHT;
	need_draw = FALSE;
      }
      break;
    case '\013':  /* Ctrl-K: Kill to end of line. */
      ti->str[ti->len = ti->loc = ti->cutend] = '\0';
      break;
    case '\025':  /* Ctrl-U: Kill from beginning of line. */
      for (i = ti->loc;  i < ti->len;  ++i)
	ti->str[i - ti->loc] = ti->str[i];
      ti->str[ti->len -= ti->loc] = '\0';
      ti->loc = ti->cutend = 0;
      break;
    default:
      if (!(need_draw = insert(env, newtext, ti)))
	result |= BUTOUT_ERR | BUTOUT_CAUGHT;
      break;
    }
  } else if ((keysym >= XK_Shift_L) && (keysym <= XK_Hyper_R))  {
  } else if ((keysym >= XK_F1) && (keysym <= XK_F35))  {
    if (!(need_draw = insert(env, newtext, ti)))
      result |= BUTOUT_ERR | BUTOUT_CAUGHT;
  } else if ((keysym == XK_BackSpace) || (keysym == XK_Delete))  {
    if (ti->loc != ti->cutend)
      need_draw = insert(env, "", ti);
    else if (ti->loc > 0)  {
      for (i = ti->cutend = --ti->loc;  ti->str[i];  ++i)
	ti->str[i] = ti->str[i+1];
      --ti->len;
      need_draw = TRUE;
    } else
      result |= BUTOUT_ERR | BUTOUT_CAUGHT;
#if  0  /* People don't like it when delete works the way I like it.  :-( */
  } else if (keysym == XK_Delete)  {
    if (ti->loc != ti->cutend)
      need_draw = insert(env, "", ti);
    else if (ti->loc < ti->len)  {
      for (i = ti->loc;  ti->str[i];  ++i)
	ti->str[i] = ti->str[i+1];
      --ti->len;
      need_draw = TRUE;
    } else
      result |= BUTOUT_ERR | BUTOUT_CAUGHT;
#endif
  }	else if (keysym == XK_Left)  {
    if (ti->loc != ti->cutend)  {
      if (ti->loc < ti->cutend)
	ti->cutend = ti->loc;
      else
	ti->loc = ti->cutend;
      need_draw = TRUE;
    } else if (ti->loc)  {
      ti->cutend = --ti->loc;
      need_draw = TRUE;
    } else
      result |= BUTOUT_ERR | BUTOUT_CAUGHT;
  } else if (keysym == XK_Right)  {
    if (ti->loc != ti->cutend)  {
      if (ti->loc > ti->cutend)
	ti->cutend = ti->loc;
      else
	ti->loc = ti->cutend;
      need_draw = TRUE;
    } else if (ti->loc < ti->len)  {
      ti->cutend = ++ti->loc;
      need_draw = TRUE;
    } else
      result |= BUTOUT_ERR | BUTOUT_CAUGHT;
  } else if (keysym == XK_Up)  {
    ti->loc = ti->cutend = 0;
    need_draw = TRUE;
  } else if (keysym == XK_Down)  {
    ti->loc = ti->cutend = ti->len;
    need_draw = TRUE;
  } else if ((keysym == XK_Return) || (keysym == XK_Linefeed) ||
	     (keysym == XK_KP_Enter))  {
    result |= BUTOUT_CAUGHT;
    if (ti->callback == NULL)  {
      but_setFlags(but, BUT_NOKEY);
    } else  {
      needCallback = TRUE;
    }
  }
  /* Void all mouse movement until it is pressed again. */
  if (ti->mousePress != -1)  {
    ti->mousePress = -1;
    if (but->flags & BUT_LOCKED)
      but_newFlags(but, but->flags & ~BUT_LOCKED);
  }
  if (need_draw)  {
    result |= BUTOUT_CAUGHT;
    curInView(but);
    but_draw(but);
  }
  if (needCallback)
    result |= ti->callback(but, ti->str);
  return(result);
}


static bool  insert(ButEnv *env, const char *src, Txtin *ti)  {
  int  i, j, ntLen, cl1, cl2;
  XFontStruct  *fs = env->fonts[0];
  
  if (src == NULL)  {
    return(FALSE);
  }
  if (ti->loc <= ti->cutend)  {
    cl1 = ti->loc;
    cl2 = ti->cutend;
  } else  {
    cl1 = ti->cutend;
    cl2 = ti->loc;
  }
  for (i = 0, ntLen = 0;  src[i];  ++i)  {
    if ((((uchar)src[i] >= fs->min_char_or_byte2) &&
	 ((uchar)src[i] <= fs->max_char_or_byte2)) ||
	(fs->all_chars_exist &&
	 fs->per_char[(uchar)src[i] - fs->min_char_or_byte2].width))
      ++ntLen;
  }
  if ((ntLen == 0) && src[0])
    return(FALSE);
  if (ntLen + ti->len + cl1 - cl2 > ti->maxlen)  {
    return(FALSE);
  }
  if (cl1 != cl2)  {
    for (i = cl1;  i < ti->len;  ++i)
      ti->str[i] = ti->str[i + cl2 - cl1];
  }
  for (i = ti->len + ntLen;  i >= cl1 + ntLen;  --i)
    ti->str[i] = ti->str[i - ntLen];
  for (i = 0, j = cl1;  src[i];  ++i)  {
    if ((((uchar)src[i] >= fs->min_char_or_byte2) &&
	 ((uchar)src[i] <= fs->max_char_or_byte2)) ||
	(fs->all_chars_exist &&
	 fs->per_char[(uchar)src[i] - fs->min_char_or_byte2].width))  {
      ti->str[j++] = src[i];
    }
  }
  ti->len += cl1 - cl2 + ntLen;
  ti->loc = ti->cutend = cl1 + ntLen;
  return(TRUE);
}


static void  ti_cursor(But *but)  {
  Txtin  *ti = but->iPacket;
  ButWin  *win = but->win;
  ButEnv  *env = win->env;
  XFontStruct  *fs = env->fonts[0];
  int  x, y;
  int  rw;

  if (ti->loc != ti->cutend)
    return;
  x = but->x - ti->xoffset + env->stdButBw*2 +
    XTextWidth(fs, ti->dispStr, ti->loc);
  y = but->y + env->stdButBw*2;
  if ((rw = (fs->ascent + fs->descent + 10) / 20) < 1)
    rw = 1;
  x -= rw/2;
  if (x < but->x + env->stdButBw)  {
    rw -= (but->x + env->stdButBw - x);
    x = but->x + env->stdButBw;
  } else if (x+rw > but->x + but->w - env->stdButBw)  {
    rw = but->x + but->w - env->stdButBw - x;
  }
  if (rw <= 0)
    return;
  if (ti->cursorVisible)
    butEnv_setXFg2(env, BUT_FG);
  else
    butEnv_setXFg2(env, BUT_HIBG);
  XFillRectangle(env->dpy, win->win, env->gc2, x - win->xOff, y - win->yOff,
		 rw, ti->pm_h - env->stdButBw*2);
}


static void  ti_redrawCursor(But *but)  {
  Txtin  *ti = but->iPacket;
  ButWin  *win = but->win;
  ButEnv  *env = win->env;
  XFontStruct  *fs = env->fonts[0];
  int  x, y;
  int  rw;

  if (ti->loc != ti->cutend)
    return;
  x = but->x - ti->xoffset + env->stdButBw*2 +
    XTextWidth(fs, ti->dispStr, ti->loc);
  y = but->y + env->stdButBw*2;
  if ((rw = (fs->ascent + fs->descent + 10) / 20) < 1)
    rw = 1;
  x -= rw/2;
  if (x < but->x + env->stdButBw)  {
    rw -= (but->x + env->stdButBw - x);
    x = but->x + env->stdButBw;
  } else if (x+rw > but->x + but->w - env->stdButBw)  {
    rw = but->x + but->w - env->stdButBw - x;
  }
  if (rw <= 0)
    return;
  butWin_redraw(win, x, y,
		rw, ti->pm_h - env->stdButBw*2);
}


static void  cut(ButEnv *env, char *cutstr, int cutlen, But *but)  {
  ButWin  *realWin;

  if (cutstr_maxlen < cutlen)  {
    if (cutdata != NULL)
      wms_free(cutdata);
    cutdata = (char *)wms_malloc(cutlen);
    cutstr_maxlen = cutlen;
  }
  XStoreBytes(env->dpy, cutstr, cutlen);
  memcpy(cutdata, cutstr, cutlen);
  cutstr_len = cutlen;
  realWin = but->win;
  while (realWin->parent != NULL)
    realWin = realWin->parent;
  if ((env->sReq == NULL) ||
      ((env->sReq == sreq) && (cut_butnum == but)))
    XSetSelectionOwner(env->dpy, XA_PRIMARY, realWin->win, 0);
  else
    env->sClear(env);
  if (XGetSelectionOwner(env->dpy, XA_PRIMARY) == realWin->win)  {
    env->sReq = sreq;
    env->sClear = sclear;
    cut_butnum = but;
  }
}


static bool  sreq(ButEnv *env, XSelectionRequestEvent *xsre)  {
  if (xsre->target != XA_STRING)
    return(FALSE);
  XChangeProperty(env->dpy, xsre->requestor, xsre->property, xsre->target,
		  8, PropModeReplace, cutdata, cutstr_len);
  return(TRUE);
}


static int  sclear(ButEnv *env)  {
  Txtin  *ti;

  assert(MAGIC(cut_butnum));
  ti = cut_butnum->iPacket;
  assert(MAGIC(ti));
  ti->loc = ti->cutend;
  but_draw(cut_butnum);
  env->sReq = NULL;
  env->sClear = NULL;
  return(0);
}


static void  paste(ButEnv *env, But *but)  {
  ButWin  *topWin;

  paste_butnum = but;
  env->sNotify = snotify;
  topWin = but->win;
  while (topWin->parent != NULL)
    topWin = topWin->parent;
  XConvertSelection(env->dpy, XA_PRIMARY, XA_STRING, env->prop,
		    topWin->win, 0);
}


static int  snotify(ButEnv *env, XSelectionEvent *xsnot)  {
  unsigned char  *propbuf;
  unsigned long  proplen, propleft;
  int  blen;
  int  pformat;
  Atom  ptype;
  Txtin  *ti;
  char  *pb2;
  
  ti = paste_butnum->iPacket;
  if (xsnot->property != None)  {
    XGetWindowProperty(env->dpy, xsnot->requestor,
		       xsnot->property, 0, ti->maxlen, True, AnyPropertyType,
		       &ptype, &pformat, &proplen, &propleft,
		       &propbuf);
    pb2 = (char *)wms_malloc(proplen * pformat + 1);
    pb2[proplen * pformat] = '\0';
    memcpy(pb2, propbuf, proplen * pformat);
    XFree(propbuf);
    if (insert(env, pb2, ti))  {
      textInView(paste_butnum, FALSE);
      but_draw(paste_butnum);
    } else
      XBell(env->dpy, 0);
    wms_free(pb2);
  } else  {
    propbuf = (unsigned char *)XFetchBytes(env->dpy, &blen);
    if (blen > 0)  {
      pb2 = (char *)wms_malloc(blen + 1);
      pb2[blen] = '\0';
      memcpy(pb2, propbuf, blen);
      XFree(propbuf);
      if (insert(env, pb2, ti))  {
	textInView(paste_butnum, FALSE);
	but_draw(paste_butnum);
      } else
	XBell(env->dpy, 0);
      wms_free(pb2);
    } else
      XBell(env->dpy, 0);
  }		
  env->sNotify = NULL;
  return(0);
}


/*
 * This adjusts your loc and your cutend for when you're in word select mode.
 */
static void  wordsel_adjust(Txtin *but)  {
  int  a, b;
  
  if (but->loc < but->cutend)  {
    a = but->loc;
    b = but->cutend;
  } else  {
    a = but->cutend;
    b = but->loc;
  }
  for (;;)  {
    if (a <= 0)  {
      a = 0;
      break;
    }
    if (((isalnum(but->str[a]) || (but->str[a] == '_')) &&
	 (isalnum(but->str[a-1]) || (but->str[a-1] == '_'))) ||
	(but->str[a] == but->str[a-1]) ||
	(isdigit(but->str[a]) && (but->str[a-1] == '.') &&
	 (a >= 2) && isdigit(but->str[a-2])) ||
	(isdigit(but->str[a+1]) && (but->str[a] == '.') &&
	  isdigit(but->str[a-1])))
      --a;
    else
      break;
  }
  if (b <= 0)
    b = 1;
  for (;;)  {
    if (b >= but->len)  {
      b = but->len;
      break;
    }
    if (((isalnum(but->str[b-1]) || (but->str[b-1] == '_')) &&
	 (isalnum(but->str[b]) || (but->str[b] == '_'))) ||
	(but->str[b-1] == but->str[b]) ||
	(isdigit(but->str[b-1]) && (but->str[b] == '.') &&
	 isdigit(but->str[b+1])) ||
	((b >= 2) && isdigit(but->str[b-2]) && (but->str[b-1] == '.') &&
	 isdigit(but->str[b])))
      ++b;
    else
      break;
  }
  but->loc = a;
  but->cutend = b;
}


/* Returns the index of the character just to the right of the cursor. */
static int  locateMouse(XFontStruct *fs, char *text, int len, int x,
			bool  *rightSide)  {
  int  i;
  int prev_x = x;
  bool  dummy;

  if (rightSide == NULL)
    rightSide = &dummy;
  for (i = 0;  (x > 0) && (i < len);  ++i)  {
    prev_x = x;
    if (fs->per_char == NULL)
      /* Monospace font. */
      x -= fs->min_bounds.width;
    else
      x -= fs->per_char[text[i] - fs->min_char_or_byte2].width;
  }
  if ((*rightSide = ((i > 0) && (x < 0) && (prev_x < -x))))
    --i;
  if (i == len)
    *rightSide = FALSE;
  return(i);
}


/* Set up xoffset so that the cursor will be in view. */
static void  curInView(But *but)  {
  Txtin  *ti = but->iPacket;
  ButEnv  *env = but->win->env;
  int  curIn;
  
  curIn = XTextWidth(env->fonts[0], ti->dispStr, ti->loc);
  if (curIn < ti->xoffset)
    ti->xoffset = curIn;
  if (curIn - ti->xoffset > ti->pm_w - env->stdButBw * 2)
    ti->xoffset = curIn - ti->pm_w + env->stdButBw * 2;
}


/*
 * Set up xoffset so that the text will be in view.  If maxInView is set,
 *   then make sure that as much text is in the window as possible (that is,
 *   there is no unnecessary empty space to the right of the text).
 */
static void  textInView(But *but, bool maxInView)  {
  Txtin  *ti = but->iPacket;
  ButEnv  *env = but->win->env;
  int  curIn;
  
  curIn = XTextWidth(env->fonts[0], ti->dispStr, ti->len);
  if (maxInView)  {
    if (ti->xoffset > curIn - (but->w - env->stdButBw*4))
      ti->xoffset = curIn - (but->w - env->stdButBw * 4);
  }
  if (ti->xoffset < 0)
    ti->xoffset = 0;
  if (ti->xoffset > curIn)
    ti->xoffset = curIn;
}


static void  flags(But *but, uint flags)  {
  Txtin  *ti = but->iPacket;

  if ((but->flags & BUT_KEYED) != (flags & BUT_KEYED))  {
    if (flags & BUT_KEYED)  {
      enableCTimer(but);
      ti->cursorVisible = TRUE;
    } else  {
      ti->cursorVisible = FALSE;
      disableCTimer(but);
    }
  }
  but->flags = flags;
  assert((but->flags & BUT_PRESSABLE) || !(but->flags & BUT_KEYED));
  but_draw(but);
}


static ButOut  blinkCursor(ButTimer *timer)  {
  But  *but = butTimer_packet(timer);
  Txtin  *ti = but->iPacket;

  ti->cursorVisible = !(butTimer_ticks(timer) & 1);
  ti_redrawCursor(but);
  return(0);
}


static ButOut  slide(ButTimer *timer)  {
  But  *but = butTimer_packet(timer);
  Txtin  *ti = but->iPacket;
  int  newXoff, oldXoff;

  if (ti->slideDir > 0)  {
    oldXoff = ti->xoffset;
    textInView(but, TRUE);
    if (oldXoff != ti->xoffset)  {
      ti->xoffset = oldXoff;
      butTimer_destroy(ti->sTimer);
      ti->sTimer = NULL;
      return(0);
    }
  }
  newXoff = ti->xoffset + ti->slideDir * butTimer_ticks(timer);
  ti->xoffset = newXoff;
  textInView(but, ti->slideDir > 0);
  if (ti->xoffset != newXoff)  {
    butTimer_destroy(ti->sTimer);
    ti->sTimer = NULL;
  }
  butTimer_setTicks(timer, 0);
  but_draw(but);
  return(0);
}


static void  startSlide(But  *but, int dir, bool fast)  {
  struct timeval  zero;
  Txtin  *ti = but->iPacket;
  int  freq;

  zero.tv_sec = 0;
  zero.tv_usec = 0;
  freq = but->h;
  if (fast)
    freq *= 10;
  if (ti->sTimer != NULL)  {
    if ((ti->slideFreq == freq) && (ti->slideDir == dir))
      return;
    butTimer_destroy(ti->sTimer);
  }
  ti->slideDir = dir;
  ti->slideFreq = freq;
  ti->sTimer = butTimer_fCreate(but, but, zero, freq, FALSE, slide);
}


static void  enableCTimer(But *but)  {
  Txtin *ti = but->iPacket;
  struct timeval  halfSec;

  assert(ti->cTimer == NULL);
  halfSec.tv_sec = 0;
  halfSec.tv_usec = 500000;
  ti->cTimer = butTimer_create(but, but, halfSec, halfSec, TRUE,
			       blinkCursor);
}


static void  disableCTimer(But *but)  {
  Txtin *ti = but->iPacket;

  if (ti->cTimer != NULL)  {
    butTimer_destroy(ti->cTimer);
    ti->cTimer = NULL;
  }
}


void  butTextin_set(But *but, const char *newStr, bool propagate)  {
  Txtin  *ti;

  assert(MAGIC(but));
  assert(but->action == &action);
  ti = but->iPacket;
  assert(MAGIC(ti));
  assert(strlen(newStr) <= ti->maxlen);
  strcpy(ti->str, newStr);
  ti->len = ti->loc = ti->cutend = strlen(newStr);
  textInView(but, TRUE);
  but_draw(but);
  if (propagate)
    ti->callback(but, ti->str);
}


void  butTextin_setHidden(But *but, bool hidden)  {
  Txtin  *ti;
  int  i;

  assert(MAGIC(but));
  assert(but->action == &action);
  ti = but->iPacket;
  assert(MAGIC(ti));
  if (hidden == ti->hidden)
    return;
  ti->hidden = hidden;
  if (hidden)  {
    ti->dispStr = wms_malloc(ti->maxlen);
    for (i = 0;  i < ti->maxlen;  ++i)  {
      ti->dispStr[i] = '*';
    }
  } else  {
    wms_free(ti->dispStr);
    ti->dispStr = ti->str;
  }
}

void  butTextin_setSpecialKey(But *but, KeySym keysym,
			      uint keyModifiers, uint modMask,
			      ButOut callback(But *but,
					      KeySym keysym, uint keyModifiers,
					      void *context),
			      void *context) {
  Txtin  *ti;
  SpecKey *newSpec;

  assert(MAGIC(but));
  assert(but->action == &action);
  ti = but->iPacket;
  assert(MAGIC(ti));
  assert((keyModifiers & modMask) == keyModifiers);
  newSpec = wms_malloc(sizeof(SpecKey));
  MAGIC_SET(newSpec);
  newSpec->next = ti->specKeys;
  ti->specKeys = newSpec;
  newSpec->keysym = keysym;
  newSpec->keyModifiers = keyModifiers;
  newSpec->modMask = modMask;
  newSpec->callback = callback;
  newSpec->context = context;
}

#endif  /* X11_DISP */
