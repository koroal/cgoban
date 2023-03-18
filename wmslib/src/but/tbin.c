/*
 * wmslib/src/but/tbin.c, part of wmslib (Library functions)
 * Copyright (C) 1994-1996 William Shubert.
 * See "configure.h.in" for more copyright information.
 */

/*
 * This is absolutely huge and disgusting.  It is no longer a cute little
 *   button.  It has grown into a monstrosity.  Some day I will break it off,
 *   make it into its own module, that reads configuration files, etc.
 * Then it will be cool!  Then it will be its own work processor, and I
 *   will just have to wrap a few extra buttons around it to have my very
 *   own beautiful wysiwyg word processor/editor!  Yay!
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
#include <wms.h>
#include <but/but.h>
#include <but/tbin.h>
#include <but/timer.h>


/**********************************************************************
 * Constants
 **********************************************************************/
#define  MAX_PASTE  (64*1024)


/**********************************************************************
 * Data types
 **********************************************************************/
typedef enum  {
  insert_none, insert_ok, insert_cr, insert_bad
} Insert;


typedef enum  {
  break_loCur, break_eoLoCur, break_eol
} BreakType;


typedef struct  Loc_struct  {
  int  tin, index;
} Loc;


typedef struct  Tin_struct  {
  int  start, len;
  int  width;  /* In pixels. */
} Tin;


typedef enum  {
  tbin_loCur, tbin_hiCur, tbin_mouse, tbin_press
} TbinLoc;
#define  tbin_numLocs  (tbin_press + 1)
#define  tbinLoc_iter(i)  for (i = 0;  i < tbin_numLocs;  ++i)


typedef struct  Tbin_struct  {
  ButTimer  *cTimer;

  int  mouseX;
  Loc  locs[tbin_numLocs];
  bool  mouseInBut;
  int  clicks, prevClickNum;

  bool  cursorVisible;
  bool  readOnly;
  int  lMargin, rMargin;

  int  maxLines;  /* If you exceed this, chop off the top. */
  int  numTins, maxTins, loTinBreak, hiTinBreak;
  Tin  *tins;

  int  bufLen, loBreak, hiBreak;
  char  *buf;

  void  (*offWinCallback)(But *but, int activeLine, int passiveLine,
			  int mouseY);

  MAGIC_STRUCT
} Tbin;


static But  *cut_butnum = NULL;
static But  *paste_butnum = NULL;

/**********************************************************************
 * Forward declarations
 **********************************************************************/
static int  locateMouse(XFontStruct *fs, const char *text, int textLen, int x,
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
static Insert  insert(ButEnv *env, const char *src, Tbin *but,
		      int butW, bool *changePrev);
static void  cut(ButEnv *env, But *but);
static void  paste(ButEnv *env, But *but);
static void  enableCTimer(But *but);
static void  disableCTimer(But *but);

static bool  sreq(ButEnv *env, XSelectionRequestEvent *xsre);
static int   sclear(ButEnv *env);
static int   snotify(ButEnv *env, XSelectionEvent *xsre);
static void  flags(But *but, uint flags);

static void  wordsel_adjust(Tbin *tbin);

static void  tin_draw(But *but, Tbin *tb, int tinNum,
		      int x, int y, int w, int h);
static int  calcTinNum(But *but, Tbin *tbin, int y);
static void  setMouseX(But *but, Tbin *tbin, int tin, int loc);
static void  checkOffWin(But *but, int mouseY, bool force);
#define  tinNum_next(tn, tb)  (((tn)+1 == (tb)->loTinBreak) ? \
			        (tb)->hiTinBreak:(tn)+1)
#define  tinNum_prev(tn, tb)  (((tn) == (tb)->hiTinBreak) ? \
			        (tb)->loTinBreak-1:(tn)-1)
#define  tinNum_line(tn, tb)  ((tn) >= (tb)->loTinBreak ? \
			       (tn) + (tb)->loTinBreak - (tb)->hiTinBreak : \
			       (tn))
#define  tinNum_line2TinNum(l, tb)             \
  ((l) >= (tb)->loTinBreak ?                   \
   (l) + (tb)->hiTinBreak - (tb)->loTinBreak : \
   (l))
#define  tinNum_valid(tn, tb)  (((tn) >= 0) && ((tn) < (tb)->maxTins) && \
				(((tn) < (tb)->loTinBreak) || \
				 ((tn) >= (tb)->hiTinBreak)))
#define  loc_valid(l, tb)  (tinNum_valid((l).tin, (tb)) && \
			    ((l).index >= 0) && \
			    ((l).index <= (tb)->tins[(l).tin].len))
#define  loc_eq(a, b)  (((a).index == (b).index) && ((a).tin == (b).tin))
static void  adjustBreak(Tbin *tb, BreakType bType, int breakTin);
static void  addMoreBufferSpace(Tbin *tb);
static void  addMoreTins(Tbin *tb);
static void  breakLine(Tbin *tb, int tinNum, ButEnv *env, int butW);
static int  calcTinWidth(Tin *tin, const char *buf, ButEnv *env);
static bool  tryJoinLines(Tbin *tb, int tinNum, ButEnv *env, int butW);
static bool  resize(But *but, int oldX, int oldY, int oldW, int oldH);
static void  killTopLines(Tbin *tb, int lines);
static void  setLoc(Tbin *tb, TbinLoc locNum, int position);


/**********************************************************************
 * Global variables
 **********************************************************************/


static ButAction  action = {
  mmove, mleave, mpress, mrelease,
  kpress, NULL, draw, destroy, flags, NULL, resize};


But  *butTbin_create(ButWin *win, int layer, int flags,
		     const char *text)  {
  But  *but;
  Tbin  *tb;
  TbinLoc  tbinLoc;
  
  tb = wms_malloc(sizeof(Tbin));
  MAGIC_SET(tb);
  but = but_create(win, tb, &action);
  but->layer = layer;
  but->flags = flags;

  if (text == NULL)
    text = "";
  tb->cTimer = NULL;
  tb->mouseX = -1;
  tbinLoc_iter(tbinLoc)  {
    tb->locs[tbinLoc].tin = 0;
    tb->locs[tbinLoc].index = 0;
  }
  tb->mouseInBut = FALSE;
  tb->clicks = tb->prevClickNum = 0;

  tb->cursorVisible = FALSE;
  tb->readOnly = FALSE;
  tb->lMargin = tb->rMargin = butEnv_stdBw(win->env);

  tb->maxLines = -1;
  tb->numTins = 1;
  tb->maxTins = 2;
  tb->tins = wms_malloc(tb->maxTins * sizeof(Tin));

  tb->bufLen = 4 * 1024;
  tb->loBreak = 0;
  tb->hiBreak = tb->bufLen;
  tb->buf = wms_malloc(tb->bufLen);

  butTbin_set(but, text);

  tb->offWinCallback = NULL;
  return(but);
}


const char  *butTbin_get(But *but)  {
  Tbin  *tb = but->iPacket;
  
  assert(but->action == &action);
  assert(MAGIC(tb));
  adjustBreak(tb, break_eol, tinNum_prev(tb->maxTins, tb));
  if (tb->loBreak == tb->hiBreak)
    addMoreBufferSpace(tb);
  tb->buf[tb->loBreak] = '\0';
  return(tb->buf);
}


int  butTbin_numLines(But *but)  {
  Tbin  *tbin = but->iPacket;

  assert(MAGIC(tbin));
  return(tbin->numTins);
}
  

void  butTbin_setOffWinCallback(But *but,
				void (*func)(But *but, int activeLine,
					     int passiveLine,
					     int mouseY))  {
  Tbin  *tbin;

  assert(MAGIC(but));
  tbin = but->iPacket;
  assert(MAGIC(tbin));
  tbin->offWinCallback = func;
}


static ButOut  destroy(But *but)  {
  Tbin  *tb = but->iPacket;
  ButEnv  *env = but->win->env;
  
  assert(but->action == &action);
  assert(MAGIC(tb));
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
  wms_free(tb->tins);
  wms_free(tb->buf);
  MAGIC_UNSET(tb);
  wms_free(tb);
  return(0);
}


static void  draw(But *but, int x,int y, int w,int h)  {
  Tbin  *tb = but->iPacket;
  ButEnv  *env = but->win->env;
  int  fontH, i, startLine;
  int  tinY;

  assert(but->action == &action);
  assert(MAGIC(tb));
  fontH = butEnv_fontH(env, 0);
  XSetFont(env->dpy, env->gc, env->fonts[0]->fid);
  startLine = (y - fontH + 1 - but->y) / fontH;
  if (startLine < 0)
    startLine = 0;
  tinY = but->y + startLine * fontH;
  for (i = tinNum_line2TinNum(startLine, tb);
       (i < tb->maxTins) && (tinY < y + h);
       i = tinNum_next(i, tb))  {
    tin_draw(but, tb, i, but->x, tinY, but->w, fontH);
    tinY += fontH;
  }
}


static void  tin_draw(But *but, Tbin *tb, int tinNum,
		      int x, int y, int w, int h)  {
  ButWin  *win = but->win;
  ButEnv  *env = win->env;
  Tin  *ti = &tb->tins[tinNum];
  XFontStruct  *fs = env->fonts[0];
  int  loCur, hiCur;
  int  blockL, blockW;
  
  butEnv_setXFg(env, BUT_FG);
  if (tb->locs[tbin_loCur].tin < tinNum)  {
    loCur = -1;
  } else if (tb->locs[tbin_loCur].tin == tinNum)  {
    loCur = tb->locs[tbin_loCur].index;
  } else  {
    loCur = ti->len + 1;
  }
  if (tb->locs[tbin_hiCur].tin < tinNum)  {
    hiCur = -1;
  } else if (tb->locs[tbin_hiCur].tin == tinNum)  {
    hiCur = tb->locs[tbin_hiCur].index;
  } else  {
    hiCur = ti->len + 1;
  }
  if (loCur != hiCur)  {
    blockW = 0;
    assert(hiCur >= 0);
    if (loCur == -1)  {
      if (hiCur == 0)
	blockL = x + tb->lMargin;
      else  {
	blockL = x;
	blockW += tb->lMargin;
      }
    } else  {
      assert((loCur >= 0) && (loCur <= ti->len));
      blockL = x + tb->lMargin + XTextWidth(fs, tb->buf + ti->start, loCur);
    }
    if (hiCur <= ti->len)  {
      blockW += XTextWidth(fs, tb->buf + ti->start + loCur, hiCur - loCur);
    } else  {
      assert(hiCur == ti->len + 1);
      if (loCur == 0)
	blockL = x;
      blockW = w - blockL;
    }
    butEnv_setXFg(env, BUT_SELBG);
    XFillRectangle(env->dpy, win->win, env->gc, blockL-win->xOff, y-win->yOff,
		   blockW, h);
    butEnv_setXFg(env, BUT_FG);
  }		
  XDrawString(env->dpy, win->win, env->gc,
	      x + tb->lMargin - win->xOff, y + fs->ascent - win->yOff,
	      tb->buf + ti->start, ti->len);
  if (!(but->flags & BUT_PRESSABLE))  {
    butEnv_stdFill2(env);
  }
  if ((tb->locs[tbin_loCur].tin == tinNum) && (loCur == hiCur) && tb->cursorVisible &&
      (but->flags & BUT_PRESSABLE))
    ti_cursor(but);
}


static ButOut  mmove(But *but, int x, int y)  {
  Tbin  *tb = but->iPacket;
  Tin  *ti;
  int  tinNum;
  ButWin  *win = but->win;
  ButEnv  *env = win->env;
  int  newMousePress;
  bool  rightSide;
  int  oldLoTin = tb->locs[tbin_loCur].tin, oldLo = tb->locs[tbin_loCur].index;
  int  oldHiTin = tb->locs[tbin_hiCur].tin, oldHi = tb->locs[tbin_hiCur].index;

  if (!(but->flags & BUT_PRESSABLE))
    return(BUTOUT_CAUGHT);
  if (!tb->mouseInBut)  {
    tb->mouseInBut = TRUE;
    butEnv_setCursor(env, but, butCur_text);
  }
  tinNum = calcTinNum(but, tb, y);
  if ((tinNum >= 0) && (tinNum < tb->maxTins))
    ti = &tb->tins[tinNum];
  else
    ti = NULL;
  switch(tb->clicks)  {
  case(0):
    break;
  case(1):
  case(2):
    if (ti)  {
      if (y < but->y)  {
	newMousePress = 0;
	rightSide = FALSE;
      } else
	newMousePress = locateMouse(env->fonts[0], tb->buf + ti->start,
				    ti->len,
				    x - (but->x+tb->lMargin), &rightSide);
      if (rightSide && (newMousePress == ti->len) &&
	  ((tinNum > tb->locs[tbin_press].tin) ||
	   ((tinNum == tb->locs[tbin_press].tin) &&
	    (tb->locs[tbin_press].index < newMousePress))))  {
	tinNum = tinNum_next(tinNum, tb);
	newMousePress = 0;
	if (tinNum >= tb->maxTins)  {
	  tinNum = tinNum_prev(tb->maxTins, tb);
	  ti = NULL;
	} else  {
	  ti = &tb->tins[tinNum];
	}
      }
    } else  {
      newMousePress = 0;
      rightSide = FALSE;
    }
    if (ti && (tb->clicks == 2))  {
      if (rightSide &&
	  ((tinNum > tb->locs[tbin_press].tin) ||
	   ((tinNum == tb->locs[tbin_press].tin) &&
	    (newMousePress >= tb->locs[tbin_press].index))) &&
	  (newMousePress < ti->len) && (newMousePress > 0))
	++newMousePress;
      else if (!rightSide &&
	       ((tinNum < tb->locs[tbin_press].tin) ||
		((tinNum == tb->locs[tbin_press].tin) &&
		 (newMousePress <= tb->locs[tbin_press].index))) &&
	       (newMousePress > 0))
	--newMousePress;
    }
    if ((newMousePress != tb->locs[tbin_mouse].index) ||
	(tinNum != tb->locs[tbin_mouse].tin))  {
      tb->locs[tbin_mouse].index = newMousePress;
      tb->locs[tbin_mouse].tin = tinNum;
      assert(loc_valid(tb->locs[tbin_mouse], tb));
      if ((tb->locs[tbin_mouse].tin < tb->locs[tbin_press].tin) ||
	  ((tb->locs[tbin_mouse].tin == tb->locs[tbin_press].tin) &&
	   (tb->locs[tbin_mouse].index < tb->locs[tbin_press].index)))  {
	tb->locs[tbin_loCur] = tb->locs[tbin_mouse];
	tb->locs[tbin_hiCur] = tb->locs[tbin_press];
      } else  {
	tb->locs[tbin_loCur] = tb->locs[tbin_press];
	tb->locs[tbin_hiCur] = tb->locs[tbin_mouse];
      }
      if (tb->clicks == 2)  {
	wordsel_adjust(tb);
      }
    }
    break;
  case(3):
    /* Triple-click. */
    tb->locs[tbin_mouse].tin = tinNum;
    tb->locs[tbin_mouse].index = 0;
    tb->locs[tbin_loCur].index = tb->locs[tbin_hiCur].index = 0;
    if (tb->locs[tbin_mouse].tin < tb->locs[tbin_press].tin)  {
      tb->locs[tbin_loCur].tin = tb->locs[tbin_mouse].tin;
      tb->locs[tbin_hiCur].tin = tb->locs[tbin_press].tin;
    } else  {
      tb->locs[tbin_loCur].tin = tb->locs[tbin_press].tin;
      tb->locs[tbin_hiCur].tin = tb->locs[tbin_mouse].tin;
    }
    if (tinNum_next(tb->locs[tbin_hiCur].tin, tb) < tb->maxTins)  {
      tb->locs[tbin_hiCur].tin = tinNum_next(tb->locs[tbin_hiCur].tin, tb);
    } else
      tb->locs[tbin_hiCur].index = tb->tins[tb->locs[tbin_hiCur].tin].len;
    assert(loc_valid(tb->locs[tbin_mouse], tb));
    break;
  default:
    assert(tb->clicks == 4);
    tb->locs[tbin_mouse].tin = tinNum;
    assert(loc_valid(tb->locs[tbin_mouse], tb));
    break;
  }
  if (tb->clicks)  {
    /*
     * We cannot call this if no clicks.  Why?  Because with no clicks, we
     *   _could_ be doing a resize (due to a new line being added).
     *   Resize calls mmove, which would call checkOffWin, which calls
     *   resize, etc...boom!
     * Even this is a bit hairy.
     */
    checkOffWin(but, y, TRUE);
  }
  if ((tb->locs[tbin_loCur].tin != oldLoTin) ||
      (tb->locs[tbin_loCur].index != oldLo) ||
      (tb->locs[tbin_hiCur].tin != oldHiTin) ||
      (tb->locs[tbin_hiCur].index != oldHi))
    but_draw(but);
  return(BUTOUT_CAUGHT);
}


static ButOut  mleave(But *but)  {
  Tbin  *tb = but->iPacket;

  if (tb->mouseInBut)  {
    tb->mouseInBut = FALSE;
    butEnv_setCursor(but->win->env, but, butCur_idle);
  }
  return(BUTOUT_CAUGHT);
}


static ButOut  mpress(But *but, int butnum, int x, int y)  {
  Tbin  *tb = but->iPacket;
  Tin  *ti;
  ButEnv  *env = but->win->env;
  static Time  lastPressTime = -1;
  static int   lastPressNum = -2;
  int  rightSide;
  
  if (tb->cTimer)  {
    butTimer_reset(tb->cTimer);
    tb->cursorVisible = TRUE;
  }
  tb->locs[tbin_mouse].tin = calcTinNum(but, tb, y);
  if ((tb->locs[tbin_mouse].tin >= 0) && (tb->locs[tbin_mouse].tin < tb->maxTins))  {
    ti = &tb->tins[tb->locs[tbin_mouse].tin];
    if (y < but->y)  {
      tb->locs[tbin_mouse].index = 0;
      rightSide = FALSE;
    } else
      tb->locs[tbin_mouse].index = locateMouse(env->fonts[0],
					       tb->buf + ti->start,
					       ti->len,
					       x - (but->x+tb->lMargin),
					       &rightSide);
  } else  {
    ti = NULL;
    tb->locs[tbin_mouse].index = 0;
  }
  switch (butnum)  {
  case 1:
    /* Button 1 selects, drags, etc. */
    assert(loc_valid(tb->locs[tbin_mouse], tb));
    tb->locs[tbin_loCur] = tb->locs[tbin_hiCur] = tb->locs[tbin_press] =
      tb->locs[tbin_mouse];
    if (!tb->readOnly && !(but->flags & BUT_KEYED))
      but_newFlags(but, but->flags | BUT_KEYED);
    but_newFlags(but, but->flags | BUT_LOCKED);
    if ((lastPressTime + BUT_DCLICK > env->eventTime) &&
	(lastPressNum + 1 == env->eventNum))  {
      tb->clicks = tb->prevClickNum;
      if (tb->clicks == 1)  {
	/* It's a double click! */
	++tb->clicks;
	if (rightSide)
	  ++tb->locs[tbin_hiCur].index;
	else
	  --tb->locs[tbin_loCur].index;
	wordsel_adjust(tb);
      } else if (tb->clicks == 2)  {
	/* It's a triple click! */
	++tb->clicks;
	tb->locs[tbin_loCur].index = 0;
	tb->locs[tbin_hiCur].index = 0;
	if (tinNum_next(tb->locs[tbin_mouse].tin, tb) < tb->maxTins)
	  tb->locs[tbin_hiCur].tin = tinNum_next(tb->locs[tbin_hiCur].tin, tb);
      } else if (tb->clicks == 3)  {
	/* Quadruple click!  Mark everything! */
	++tb->clicks;
	tb->locs[tbin_loCur].tin = 0;
	tb->locs[tbin_hiCur].tin = tinNum_prev(tb->maxTins, tb);
	tb->locs[tbin_loCur].index = 0;
	tb->locs[tbin_hiCur].index = tb->tins[tb->locs[tbin_hiCur].tin].len;
      } else  {
	/* Pentuple click!  Restart! */
	tb->clicks = 1;
      }
    } else  {
      tb->clicks = 1;
    }
    lastPressTime = env->eventTime;
    lastPressNum = env->eventNum;
    but_draw(but);
    break;
  case 2:
    /* Paste! */
    if (tb->readOnly)
      return(BUTOUT_ERR);
    else
      paste(env, but);
    break;
  default:
    /*
     * Weird.  I don't know what to do with mice with more than 3 buttons
     *   so I guess I might as well beep.  I like beeping.
     */
    return(BUTOUT_ERR);
    break;
  }
  checkOffWin(but, y, TRUE);
  return(BUTOUT_CAUGHT);
}


static ButOut  mrelease(But *but, int butnum, int x, int y)  {
  Tbin  *tb = but->iPacket;

  if (tb->cTimer)  {
    butTimer_reset(tb->cTimer);
    tb->cursorVisible = TRUE;
  }
  if ((butnum == 1) && tb->clicks)  {
    tb->prevClickNum = tb->clicks;
    tb->clicks = 0;
    if ((tb->locs[tbin_loCur].tin != tb->locs[tbin_hiCur].tin) ||
	(tb->locs[tbin_loCur].index != tb->locs[tbin_hiCur].index))  {
      cut(but->win->env, but);
    }
    but_newFlags(but, but->flags & ~BUT_LOCKED);
  }
  checkOffWin(but, y, TRUE);
  return(BUTOUT_CAUGHT);
}


static ButOut  kpress(But *but, const char *newtext, KeySym keysym)  {
  Tbin  *tb = but->iPacket;
  ButEnv  *env = but->win->env;
  bool  need_draw = FALSE;
  ButOut  result = 0;
  bool  clearMouseX = TRUE;
  int  drawLo = tb->locs[tbin_loCur].tin, drawHi = tb->locs[tbin_hiCur].tin;
  Insert  insertResult = insert_none;
  bool  changePrev = FALSE;
  
  assert(but->flags & BUT_KEYED);
  butTimer_reset(tb->cTimer);
  tb->cursorVisible = TRUE;
  if (((keysym >= XK_KP_Space) && (keysym <= XK_KP_9)) ||
      ((keysym >= XK_space) && (keysym <= XK_ydiaeresis)))  {
    need_draw = TRUE;
    switch(newtext[0])  {
    case '\001':  /* Ctrl-A: Beginning of line. */
      tb->locs[tbin_loCur].index = 0;
      tb->locs[tbin_hiCur] = tb->locs[tbin_loCur];
      break;
    case '\020':  /* Ctrl-P: Up a line. */
      clearMouseX = FALSE;
      if (tb->locs[tbin_loCur].tin == 0)  {
	if (tb->locs[tbin_loCur].index || tb->locs[tbin_hiCur].index ||
	    tb->locs[tbin_hiCur].tin)  {
	  tb->locs[tbin_loCur].index = 0;
	  tb->locs[tbin_hiCur] = tb->locs[tbin_loCur];
	} else  {
	  need_draw = FALSE;
	  result |= BUTOUT_ERR;
	}
      } else  {
	if (tb->mouseX == -1)
	  setMouseX(but, tb, tb->locs[tbin_loCur].tin,
		    tb->locs[tbin_loCur].index);
	tb->locs[tbin_loCur].tin = tinNum_prev(tb->locs[tbin_loCur].tin, tb);
	tb->locs[tbin_loCur].index =
	  locateMouse(env->fonts[0],
		      tb->buf + tb->tins[tb->locs[tbin_loCur].tin].start,
		      tb->tins[tb->locs[tbin_loCur].tin].len,
		      tb->mouseX, NULL);
	tb->locs[tbin_hiCur] = tb->locs[tbin_loCur];
      }
      break;
    case '\016':  /* Ctrl-N: Down a line. */
      clearMouseX = FALSE;
      if (tinNum_next(tb->locs[tbin_hiCur].tin, tb) == tb->maxTins)  {
	if (tb->locs[tbin_loCur].tin != tb->locs[tbin_hiCur].tin)  {
	  tb->locs[tbin_loCur] = tb->locs[tbin_hiCur];
	} else  {
	  need_draw = FALSE;
	  result |= BUTOUT_ERR;
	}
      } else  {
	if (tb->mouseX == -1)
	  setMouseX(but, tb, tb->locs[tbin_hiCur].tin,
		    tb->locs[tbin_hiCur].index);
	tb->locs[tbin_hiCur].tin = tinNum_next(tb->locs[tbin_hiCur].tin, tb);
	tb->locs[tbin_hiCur].index =
	  locateMouse(env->fonts[0],
		      tb->buf + tb->tins[tb->locs[tbin_hiCur].tin].start,
		      tb->tins[tb->locs[tbin_hiCur].tin].len,
		      tb->mouseX, NULL);
	tb->locs[tbin_loCur] = tb->locs[tbin_hiCur];
      }
      break;
    case '\002':  /* Ctrl-B: Back a character. */
      if ((tb->locs[tbin_loCur].tin != tb->locs[tbin_hiCur].tin) ||
	  (tb->locs[tbin_loCur].index != tb->locs[tbin_hiCur].index))  {
	tb->locs[tbin_hiCur] = tb->locs[tbin_loCur];
	need_draw = TRUE;
      } else if (tb->locs[tbin_loCur].index)  {
	tb->locs[tbin_hiCur].index = --tb->locs[tbin_loCur].index;
      } else if (tb->locs[tbin_loCur].tin)  {
	tb->locs[tbin_loCur].tin = tinNum_prev(tb->locs[tbin_loCur].tin, tb);
	tb->locs[tbin_loCur].index = tb->tins[tb->locs[tbin_loCur].tin].len;
	tb->locs[tbin_hiCur] = tb->locs[tbin_loCur];
      } else  {
	need_draw = FALSE;
	result |= BUTOUT_ERR;
      }
      break;
    case '\004':  /* Ctrl-D: Delete right. */
      if ((tb->locs[tbin_loCur].index != tb->locs[tbin_hiCur].index) ||
	  (tb->locs[tbin_loCur].tin != tb->locs[tbin_hiCur].tin))  {
	insertResult = insert(env, "", tb, but->w, &changePrev);
	need_draw = TRUE;
      } else if (tinNum_next(tb->locs[tbin_loCur].tin, tb) < tb->maxTins)  {
	if (tb->locs[tbin_loCur].index <
	    tb->tins[tb->locs[tbin_loCur].tin].len)
	  ++tb->locs[tbin_hiCur].index;
	else if (tb->buf[tb->loBreak - 1] == '\n')  {
	  tb->locs[tbin_hiCur].tin = tinNum_next(tb->locs[tbin_hiCur].tin, tb);
	  tb->locs[tbin_hiCur].index = 0;
	} else  {
	  tb->locs[tbin_hiCur].tin = tinNum_next(tb->locs[tbin_hiCur].tin, tb);
	  tb->locs[tbin_hiCur].index = 1;
	}
	insertResult = insert(env, "", tb, but->w, &changePrev);
	need_draw = TRUE;
      } else
	result |= BUTOUT_ERR;
      break;
    case '\005':  /* Ctrl-E: End of line. */
      tb->locs[tbin_hiCur].index = tb->tins[tb->locs[tbin_hiCur].tin].len;
      tb->locs[tbin_loCur] = tb->locs[tbin_hiCur];
      break;
    case '\006':  /* Ctrl-F: Forward a character. */
      need_draw = TRUE;
      if ((tb->locs[tbin_loCur].tin != tb->locs[tbin_hiCur].tin) ||
	  (tb->locs[tbin_loCur].index != tb->locs[tbin_hiCur].index))  {
	tb->locs[tbin_loCur] = tb->locs[tbin_hiCur];
      } else if (tb->locs[tbin_loCur].index <
		 tb->tins[tb->locs[tbin_loCur].tin].len)  {
	tb->locs[tbin_hiCur].index = ++tb->locs[tbin_loCur].index;
      } else if (tinNum_next(tb->locs[tbin_loCur].tin, tb) < tb->maxTins)  {
	tb->locs[tbin_loCur].tin = tinNum_next(tb->locs[tbin_loCur].tin, tb);
	tb->locs[tbin_loCur].index = 0;
	tb->locs[tbin_hiCur] = tb->locs[tbin_loCur];
      } else  {
	result |= BUTOUT_ERR;
	need_draw = FALSE;
      }
      break;
    case '\013':  /* Ctrl-K: Kill to end of line. */
      need_draw = TRUE;
      if ((tb->locs[tbin_loCur].index != tb->locs[tbin_hiCur].index) ||
	  (tb->locs[tbin_loCur].tin != tb->locs[tbin_hiCur].tin))
	insertResult = insert(env, "", tb, but->w, &changePrev);
      else if (tinNum_next(tb->locs[tbin_loCur].tin, tb) < tb->maxTins)  {
	if (tb->locs[tbin_loCur].index ==
	    tb->tins[tb->locs[tbin_loCur].tin].len)  {
	  tb->locs[tbin_hiCur].tin = tinNum_next(tb->locs[tbin_loCur].tin, tb);
	  tb->locs[tbin_hiCur].index = 0;
	} else  {
	  tb->locs[tbin_hiCur].tin = tb->locs[tbin_loCur].tin;
	  tb->locs[tbin_hiCur].index = tb->tins[tb->locs[tbin_loCur].tin].len;
	}
	insertResult = insert(env, "", tb, but->w, &changePrev);
      } else  {
	result |= BUTOUT_ERR;
	need_draw = FALSE;
      }
      break;
    case '\025':  /* Ctrl-U: Kill from beginning of line. */
      if ((tb->locs[tbin_loCur].index != tb->locs[tbin_hiCur].index) ||
	  (tb->locs[tbin_loCur].tin != tb->locs[tbin_hiCur].tin))
	insertResult = insert(env, "", tb, but->w, &changePrev);
      else if (tb->locs[tbin_loCur].index > 0)  {
	tb->locs[tbin_hiCur].index = 0;
	tb->locs[tbin_hiCur].tin = tb->locs[tbin_loCur].tin;
	insertResult = insert(env, "", tb, but->w, &changePrev);
      } else  {
	result |= BUTOUT_ERR;
	need_draw = FALSE;
      }
      break;
    default:
      insertResult = insert(env, newtext, tb, but->w, &changePrev);
      break;
    }
  } else if ((keysym >= XK_Shift_L) && (keysym <= XK_Hyper_R))  {
  } else if ((keysym >= XK_F1) && (keysym <= XK_F35))  {
    insertResult = insert(env, newtext, tb, but->w, &changePrev);
  } else if ((keysym == XK_BackSpace) || (keysym == XK_Delete))  {
    if ((tb->locs[tbin_loCur].index != tb->locs[tbin_hiCur].index) ||
	(tb->locs[tbin_loCur].tin != tb->locs[tbin_hiCur].tin))  {
      insertResult = insert(env, "", tb, but->w, &changePrev);
      need_draw = TRUE;
    } else if (tb->locs[tbin_loCur].index > 0)  {
      --tb->locs[tbin_loCur].index;
      assert((drawHi < tb->loTinBreak) || (drawHi >= tb->hiTinBreak));
      drawHi = tinNum_line(drawHi, tb);
      insertResult = insert(env, "", tb, but->w, &changePrev);
      drawHi = tinNum_line2TinNum(drawHi, tb);
      assert((drawHi < tb->loTinBreak) || (drawHi >= tb->hiTinBreak));
      need_draw = TRUE;
    } else if (tb->locs[tbin_loCur].tin > 0)  {
      if (tb->buf[tb->tins[tb->locs[tbin_loCur].tin].start - 1] == '\n')  {
	tb->locs[tbin_loCur].tin = tinNum_prev(tb->locs[tbin_loCur].tin, tb);
	tb->locs[tbin_loCur].index = tb->tins[tb->locs[tbin_loCur].tin].len;
      } else  {
	tb->locs[tbin_loCur].tin = tinNum_prev(tb->locs[tbin_loCur].tin, tb);
	tb->locs[tbin_loCur].index = tb->tins[tb->locs[tbin_loCur].tin].len - 1;
      }
      insertResult = insert(env, "", tb, but->w, &changePrev);
      need_draw = TRUE;
    } else {
      result |= BUTOUT_ERR;
    }
#if 0  /* People don't like my delete.  :-( */
  } else if (keysym == XK_Delete)  {
    if ((tb->locs[tbin_loCur].index != tb->locs[tbin_hiCur].index) ||
	(tb->locs[tbin_loCur].tin != tb->locs[tbin_hiCur].tin))  {
      insertResult = insert(env, "", tb, but->w, &changePrev);
      need_draw = TRUE;
    } else if (tinNum_next(tb->locs[tbin_loCur].tin, tb) < tb->maxTins)  {
      if (tb->locs[tbin_loCur].index < tb->tins[tb->locs[tbin_loCur].tin].len)
	++tb->locs[tbin_hiCur].index;
      else if (tb->buf[tb->loBreak - 1] == '\n')  {
	tb->locs[tbin_hiCur].tin = tinNum_next(tb->locs[tbin_hiCur].tin, tb);
	tb->locs[tbin_hiCur].index = 0;
      } else  {
	tb->locs[tbin_hiCur].tin = tinNum_next(tb->locs[tbin_hiCur].tin, tb);
	tb->locs[tbin_hiCur].index = 1;
      }
      insertResult = insert(env, "", tb, but->w, &changePrev);
      need_draw = TRUE;
    } else
      result |= BUTOUT_ERR;
#endif
  } else if (keysym == XK_Left)  {
    need_draw = TRUE;
    if (!loc_eq(tb->locs[tbin_loCur], tb->locs[tbin_hiCur]))  {
      tb->locs[tbin_hiCur] = tb->locs[tbin_loCur];
    } else if (tb->locs[tbin_loCur].index)  {
      tb->locs[tbin_hiCur].index = --tb->locs[tbin_loCur].index;
    } else if (tb->locs[tbin_loCur].tin)  {
      tb->locs[tbin_loCur].tin = tinNum_prev(tb->locs[tbin_loCur].tin, tb);
      tb->locs[tbin_loCur].index = tb->tins[tb->locs[tbin_loCur].tin].len;
      tb->locs[tbin_hiCur] = tb->locs[tbin_loCur];
    } else
      result |= BUTOUT_ERR;
  } else if (keysym == XK_Right)  {
    need_draw = TRUE;
    if (!loc_eq(tb->locs[tbin_loCur], tb->locs[tbin_hiCur]))  {
      tb->locs[tbin_loCur] = tb->locs[tbin_hiCur];
    } else if (tb->locs[tbin_loCur].index <
	       tb->tins[tb->locs[tbin_loCur].tin].len)  {
      tb->locs[tbin_hiCur].index = ++tb->locs[tbin_loCur].index;
    } else if (tinNum_next(tb->locs[tbin_loCur].tin, tb) < tb->maxTins)  {
      tb->locs[tbin_loCur].tin = tinNum_next(tb->locs[tbin_loCur].tin, tb);
      tb->locs[tbin_loCur].index = 0;
      tb->locs[tbin_hiCur] = tb->locs[tbin_loCur];
    } else
      result |= BUTOUT_ERR;
  } else if (keysym == XK_Up)  {
    clearMouseX = FALSE;
    if (tb->locs[tbin_loCur].tin == 0)  {
      if (!loc_eq(tb->locs[tbin_loCur], tb->locs[tbin_hiCur]))  {
	tb->locs[tbin_hiCur] = tb->locs[tbin_loCur];
	need_draw = TRUE;
      } else
	result |= BUTOUT_ERR;
    } else  {
      if (tb->mouseX == -1)
	setMouseX(but, tb, tb->locs[tbin_loCur].tin,
		  tb->locs[tbin_loCur].index);
      tb->locs[tbin_loCur].tin = tinNum_prev(tb->locs[tbin_loCur].tin, tb);
      tb->locs[tbin_loCur].index = locateMouse(env->fonts[0],
					       tb->buf +
					       tb->tins[tb->locs[tbin_loCur].
							tin].start,
					       tb->tins[tb->locs[tbin_loCur].
							tin].len,
					       tb->mouseX, NULL);
      tb->locs[tbin_hiCur] = tb->locs[tbin_loCur];
      need_draw = TRUE;
    }
  } else if (keysym == XK_Down)  {
    clearMouseX = FALSE;
    assert(loc_valid(tb->locs[tbin_loCur], tb));
    assert(loc_valid(tb->locs[tbin_hiCur], tb));
    if (tinNum_next(tb->locs[tbin_hiCur].tin, tb) == tb->maxTins)  {
      if (loc_eq(tb->locs[tbin_loCur], tb->locs[tbin_hiCur]))
	result |= BUTOUT_ERR;
      else  {
	tb->locs[tbin_loCur] = tb->locs[tbin_hiCur];
      }
    } else  {
      if (tb->mouseX == -1)
	setMouseX(but, tb, tb->locs[tbin_hiCur].tin,
		  tb->locs[tbin_hiCur].index);
      tb->locs[tbin_hiCur].tin = tinNum_next(tb->locs[tbin_hiCur].tin, tb);
      tb->locs[tbin_hiCur].index =
	locateMouse(env->fonts[0],
		    tb->buf + tb->tins[tb->locs[tbin_hiCur].tin].start,
		    tb->tins[tb->locs[tbin_hiCur].tin].len,
		    tb->mouseX, NULL);
      tb->locs[tbin_loCur] = tb->locs[tbin_hiCur];
    }
    need_draw = TRUE;
  } else if ((keysym == XK_Return) || (keysym == XK_Linefeed) ||
	     (keysym == XK_KP_Enter))  {
    insertResult = insert(env, "\n", tb, but->w, &changePrev);
  } else {
    result |= BUTOUT_ERR;
  }
  /* Void all mouse movement until it is pressed again. */
  if (tb->clicks)  {
    tb->clicks = 0;
    if (but->flags & BUT_LOCKED)
      but_newFlags(but, but->flags & ~BUT_LOCKED);
  }
  if (need_draw || (insertResult != insert_none))  {
    result |= BUTOUT_CAUGHT;
    if (tb->locs[tbin_loCur].tin < drawLo)
      drawLo = tb->locs[tbin_loCur].tin;
    if (tb->locs[tbin_hiCur].tin > drawHi) {
      drawHi = tb->locs[tbin_hiCur].tin;
      assert((drawHi < tb->loTinBreak) || (drawHi >= tb->hiTinBreak));
    }
    if (changePrev && drawLo)  {
      drawLo = tinNum_prev(drawLo, tb);
    }
    assert(drawHi >= drawLo);
    drawLo = tinNum_line(drawLo, tb) * butEnv_fontH(env, 0);
    if (insertResult == insert_cr) {
      drawHi = but->h;
    } else {
      assert((drawHi < tb->loTinBreak) || (drawHi >= tb->hiTinBreak));
      drawHi = (tinNum_line(drawHi, tb) + 1) * butEnv_fontH(env, 0);
    }
    if (drawLo < but->h)  {
      assert(drawHi >= drawLo);
      if (drawHi > but->h) {
	drawHi = but->h;
	assert(drawHi > drawLo);
      }
      butWin_redraw(but->win, but->x,but->y+drawLo, but->w,drawHi - drawLo);
    }
  }
  if (clearMouseX)
    tb->mouseX = -1;
  if (insertResult == insert_bad)
    result |= BUTOUT_ERR;
  checkOffWin(but, 0, FALSE);
  return(result);
}


static Insert  insert(ButEnv *env, const char *src, Tbin *tb, int butW,
		      bool *drawLo)  {
  int  i, len, loTinWidthCheck;
  Insert  result = insert_ok;
  bool  lineBroken, dummy;

  if (drawLo == NULL)
    drawLo = &dummy;
  butW -= tb->lMargin + tb->rMargin;
  assert(tb->loTinBreak <= tb->hiTinBreak);
  assert(loc_valid(tb->locs[tbin_loCur], tb));
  assert(loc_valid(tb->locs[tbin_hiCur], tb));
  assert(tb->maxTins == tb->numTins + tb->hiTinBreak - tb->loTinBreak);
  adjustBreak(tb, break_loCur, 0);
  assert(tb->tins[tb->locs[tbin_loCur].tin].start +
	 tb->locs[tbin_loCur].index == tb->loBreak);
  assert(loc_valid(tb->locs[tbin_loCur], tb));
  assert(loc_valid(tb->locs[tbin_hiCur], tb));
  assert(tb->maxTins == tb->numTins + tb->hiTinBreak - tb->loTinBreak);
  if (!loc_eq(tb->locs[tbin_loCur], tb->locs[tbin_hiCur]))  {
    /*
     * Cut out any deleted text.
     */
    assert(loc_valid(tb->locs[tbin_mouse], tb));
    assert(loc_valid(tb->locs[tbin_press], tb));
    if (tb->locs[tbin_hiCur].tin != tb->locs[tbin_loCur].tin)  {
      result = insert_cr;
      i = tb->locs[tbin_hiCur].tin + 1 - tb->hiTinBreak;
      tb->numTins -= i,
      tb->hiTinBreak += i;
    }
    tb->tins[tb->locs[tbin_loCur].tin].len = tb->locs[tbin_loCur].index +
      tb->tins[tb->locs[tbin_hiCur].tin].len - tb->locs[tbin_hiCur].index;
    if (tb->locs[tbin_loCur].tin == tb->locs[tbin_hiCur].tin)  {
      tb->hiBreak += tb->locs[tbin_hiCur].index - tb->locs[tbin_loCur].index;
    } else
      tb->hiBreak = tb->tins[tb->locs[tbin_hiCur].tin].start +
	tb->locs[tbin_hiCur].index;
    tb->locs[tbin_hiCur] = tb->locs[tbin_loCur];
    if (!loc_valid(tb->locs[tbin_mouse], tb))
      tb->locs[tbin_mouse] = tb->locs[tbin_loCur];
    if (!loc_valid(tb->locs[tbin_press], tb))
      tb->locs[tbin_press] = tb->locs[tbin_loCur];
  }
  assert(tb->tins[tb->locs[tbin_loCur].tin].start +
	 tb->locs[tbin_loCur].index == tb->loBreak);
  assert(tb->loTinBreak <= tb->hiTinBreak);
  assert(tb->maxTins == tb->numTins + tb->hiTinBreak - tb->loTinBreak);
  if (src == NULL)  {
    adjustBreak(tb, break_eoLoCur, 0);
    return(insert_bad);
  }
  len = strlen(src);
  /*
   * We must use <= in this next condition because we may add an extra \n to
   *   the buffer at the end.
   */
  while (tb->hiBreak - tb->loBreak <= len)  {
    addMoreBufferSpace(tb);
  }
  assert(tb->tins[tb->locs[tbin_loCur].tin].start >= 0);
  loTinWidthCheck = tinNum_line(tb->locs[tbin_loCur].tin, tb);
  for (i = 0;  i < len;  ++i)  {
    assert(tb->loTinBreak <= tb->hiTinBreak);
    assert(tb->tins[tb->locs[tbin_loCur].tin].start +
	   tb->locs[tbin_loCur].index == tb->loBreak);
    tb->buf[tb->loBreak++] = src[i];
    if (src[i] == '\n')  {
      /*
       * If the only line left is the mandatory blank line at the end, then
       *   don't bother inserting the final '\n'.
       */
      if ((i + 1 == len) &&
	  (tinNum_next(tb->locs[tbin_loCur].tin, tb) ==
	   tinNum_prev(tb->maxTins, tb)) &&
	  (tb->locs[tbin_loCur].index ==
	   tb->tins[tb->locs[tbin_loCur].tin].len))  {
	--tb->loBreak;
	tb->locs[tbin_loCur].tin = tinNum_next(tb->locs[tbin_loCur].tin, tb);
	tb->locs[tbin_loCur].index = 0;
	break;
      }
      result = insert_cr;
      if (tb->numTins == tb->maxTins)
	addMoreTins(tb);
      tb->tins[tb->locs[tbin_loCur].tin + 1].start = tb->loBreak;
      tb->tins[tb->locs[tbin_loCur].tin + 1].len =
	tb->tins[tb->locs[tbin_loCur].tin].len - tb->locs[tbin_loCur].index;
      tb->tins[tb->locs[tbin_loCur].tin].len = tb->locs[tbin_loCur].index;
      ++tb->numTins;
      ++tb->loTinBreak;
      ++tb->locs[tbin_loCur].tin;
      tb->locs[tbin_loCur].index = 0;
    } else  {
      ++tb->locs[tbin_loCur].index;
      ++tb->tins[tb->locs[tbin_loCur].tin].len;
    }
  }
  assert(tb->tins[tb->locs[tbin_loCur].tin].start >= 0);
  assert(tb->maxTins == tb->numTins + tb->hiTinBreak - tb->loTinBreak);
  if ((tb->hiBreak == tb->bufLen) && (tb->tins[tb->loTinBreak - 1].len > 0) &&
      !tb->readOnly)  {
    /*
     * Add an extra '\n' to make sure that there's always a blank line at
     *   the end of the buffer.
     */
    tb->buf[tb->loBreak++] = '\n';
    result = insert_cr;
    if (tb->numTins == tb->maxTins)
      addMoreTins(tb);
    assert(tb->tins[tb->locs[tbin_loCur].tin].start >= 0);
    tb->tins[tb->locs[tbin_loCur].tin + 1].start = tb->loBreak;
    tb->tins[tb->locs[tbin_loCur].tin + 1].len = 0;
    tb->tins[tb->locs[tbin_loCur].tin + 1].width = 0;
    ++tb->numTins;
    ++tb->loTinBreak;
  }    
  tb->locs[tbin_hiCur] = tb->locs[tbin_loCur];
  adjustBreak(tb, break_eoLoCur, 0);
  assert(loc_valid(tb->locs[tbin_loCur], tb));
  assert(loc_valid(tb->locs[tbin_hiCur], tb));
  assert(tb->maxTins == tb->numTins + tb->hiTinBreak - tb->loTinBreak);
  assert(tb->hiBreak <= tb->bufLen);
  assert(tb->loBreak <= tb->bufLen);
  assert(tb->hiBreak >= 0);
  assert(tb->loBreak >= 0);
  assert(tb->tins[tb->locs[tbin_loCur].tin].start >= 0);
  /*
   * Now that the text is inserted, it is time to adjust the EOLs if we have
   *   to.
   */
  tryJoinLines(tb, tinNum_line2TinNum(loTinWidthCheck, tb), env, butW);
  lineBroken = TRUE;
  for (i = tinNum_line2TinNum(loTinWidthCheck, tb);
       (i < tb->maxTins) && ((i <= tb->locs[tbin_loCur].tin) || lineBroken);
       i = tinNum_next(i, tb))  {
    if (calcTinWidth(&tb->tins[i], tb->buf, env) > butW)  {
      lineBroken = TRUE;
      breakLine(tb, i, env, butW);
      result = insert_cr;
    }
  }
  if (tryJoinLines(tb, tinNum_next(tb->locs[tbin_loCur].tin, tb), env, butW))
    result = insert_cr;
  if ((tb->maxLines > 0) && (tb->numTins > tb->maxLines))  {
    killTopLines(tb, tb->numTins - tb->maxLines);
  }
  return(result);
}


/*
 * This routine attempts to join the line that you specify with the
 *   previous line.
 * You cannot call this unless the current break is in the middle of neither
 *   of these lines.  It is OK to call this if the current break is in between
 *   these lines.
 * The breakpoint may be changed while in this function.
 */
static bool  tryJoinLines(Tbin *tb, int tinNum, ButEnv *env, int butW)  {
  int  prevTinNum, w, curChar;
  Tin  *a, *b;
  XFontStruct  *fs = env->fonts[0];
  TbinLoc  loc;

  if ((tinNum == 0) || (tinNum == tb->maxTins))
    return(FALSE);
  prevTinNum = tinNum_prev(tinNum, tb);
  a = &tb->tins[prevTinNum];
  b = &tb->tins[tinNum];
  if (tinNum == tb->hiTinBreak)  {
    if (b->start != tb->hiBreak)
      return(FALSE);
  } else  {
    if (b->start != a->start + a->len)
      return(FALSE);
  }
  w = a->width;
  curChar = a->len - 1;
  while ((curChar >= 0) && (tb->buf[a->start + curChar] == ' '))  {
    --curChar;
    if (fs->per_char == NULL)
      w += fs->min_bounds.width;
    else
      w += fs->per_char[(uchar)tb->buf[a->start + curChar] -
			fs->min_char_or_byte2].width;
  }
  curChar = 0;
  while ((curChar < b->len) && (w <= butW))  {
    if (fs->per_char == NULL)
      w += fs->min_bounds.width;
    else
      w += fs->per_char[(uchar)tb->buf[b->start + curChar] -
			fs->min_char_or_byte2].width;
    ++curChar;
  }
  if (w > butW)  {
    if (curChar)  {
      --curChar;
      while (curChar && (tb->buf[b->start + curChar] != ' '))
	--curChar;
      while ((curChar < b->len) &&
	     (tb->buf[b->start + curChar] == ' '))
	++curChar;
    }
  }
  if (!curChar)
    return(FALSE);
  if (curChar == b->len)  {
    /* We can suck the whole damn line up. */
    adjustBreak(tb, break_eol, tinNum);
    tinNum = tb->loTinBreak - 1;
    tbinLoc_iter(loc)  {
      if (tb->locs[loc].tin == tinNum)  {
	tb->locs[loc].index += tb->tins[tinNum - 1].len;
	--tb->locs[loc].tin;
      }
    }
    tb->tins[tinNum - 1].len += tb->tins[tinNum].len;
    calcTinWidth(&tb->tins[tinNum - 1], tb->buf, env);
    --tb->numTins;
    --tb->loTinBreak;
    assert((tb->loTinBreak == 0) ||
	   (tb->tins[tb->loTinBreak - 1].start < tb->loBreak));
  } else  {
    tbinLoc_iter(loc)  {
      if (tb->locs[loc].tin == tinNum)  {
	tb->locs[loc].index -= curChar;
	if (tb->locs[loc].index <= 0)  {
	  tb->locs[loc].tin = prevTinNum;
	  tb->locs[loc].index += a->len + curChar;
	}
      }
    }
    a->len += curChar;
    b->len -= curChar;
    b->start += curChar;
    if (tb->hiTinBreak == tinNum)  {
      adjustBreak(tb, break_eol, prevTinNum);
      calcTinWidth(&tb->tins[tb->loTinBreak - 1], tb->buf, env);
      calcTinWidth(&tb->tins[tb->hiTinBreak], tb->buf, env);
    }
    assert((tb->loTinBreak == 0) ||
	   (tb->tins[tb->loTinBreak - 1].start < tb->loBreak));
  }
  return(TRUE);
}


static void  breakLine(Tbin *tb, int tinNum, ButEnv *env, int butW)  {
  XFontStruct  *fs = env->fonts[0];
  int  breakPoint, w;
  TbinLoc  loc;

  assert(tinNum < tb->maxTins);
  assert((tinNum < tb->loTinBreak) || (tinNum >= tb->hiTinBreak));
  if (butW <= 0)  {
    return;
  }
  assert(loc_valid(tb->locs[tbin_loCur], tb));
  adjustBreak(tb, break_eol, tinNum);
  /*
   * I really should make my mark into a loc, but I'll cheese out instead.
   */
  tinNum = tb->loTinBreak - 1;
  assert(loc_valid(tb->locs[tbin_loCur], tb));
  breakPoint = tb->tins[tinNum].len;
  w = tb->tins[tinNum].width;
  assert(w > butW);
  while (tb->buf[tb->tins[tinNum].start + breakPoint - 1] == ' ')  {
    --breakPoint;
    assert(breakPoint > 0);
  }
  while (breakPoint > 0)  {
    if ((tb->buf[tb->tins[tinNum].start + breakPoint] == ' ') &&
	(tb->buf[tb->tins[tinNum].start + breakPoint - 1] != ' ') &&
	(w <= butW))
      break;
    --breakPoint;
    if (fs->per_char == NULL)
      w -= fs->min_bounds.width;
    else
      w -= fs->per_char[(uchar)tb->buf[tb->tins[tinNum].start + breakPoint] -
			fs->min_char_or_byte2].width;
  }
  if (breakPoint == 0)  {
    adjustBreak(tb, break_eoLoCur, 0);
    return;
  }
  while (tb->buf[tb->tins[tinNum].start + breakPoint] == ' ')
    ++breakPoint;
  if (tb->numTins == tb->maxTins)
    addMoreTins(tb);
  assert(tb->loTinBreak == tinNum + 1);
  if ((tb->hiTinBreak < tb->maxTins) &&
      (tb->tins[tb->hiTinBreak].start == tb->hiBreak))  {
    /*
     * There is no '\n' here, so we can just shuffle the too-long characters
     *   down to the next line.
     */
    tb->tins[tb->hiTinBreak].len += tb->tins[tinNum].len - breakPoint;
    tb->tins[tb->hiTinBreak].start = tb->tins[tinNum].start + breakPoint;
    tb->tins[tb->hiTinBreak].width +=
      XTextWidth(env->fonts[0], tb->buf + tb->tins[tb->hiTinBreak].start,
		 tb->tins[tinNum].len - breakPoint);
    tb->tins[tb->loTinBreak] = tb->tins[tb->hiTinBreak];
    ++tb->loTinBreak;
    ++tb->hiTinBreak;
  } else  {
    tb->tins[tinNum + 1].start = tb->tins[tinNum].start + breakPoint;
    tb->tins[tinNum + 1].len = tb->tins[tinNum].len - breakPoint;
    ++tb->loTinBreak;
    ++tb->numTins;
  }
  tb->tins[tinNum].len = breakPoint;
  tb->tins[tinNum].width = w;
  tbinLoc_iter(loc)  {
    if ((tb->locs[loc].tin == tinNum) &&
	(tb->locs[loc].index > tb->tins[tinNum].len))  {
      tb->locs[loc].index -= tb->tins[tinNum].len;
      tb->locs[loc].tin = tinNum_next(tb->locs[loc].tin, tb);
    }
  }
  ++tinNum;
  calcTinWidth(&tb->tins[tinNum], tb->buf, env);
  assert(loc_valid(tb->locs[tbin_loCur], tb));
  adjustBreak(tb, break_eoLoCur, 0);
  assert(loc_valid(tb->locs[tbin_loCur], tb));
}


static void  ti_cursor(But *but)  {
  Tbin  *tb = but->iPacket;
  Tin  *ti = &tb->tins[tb->locs[tbin_loCur].tin];
  ButWin  *win = but->win;
  ButEnv  *env = win->env;
  XFontStruct  *fs = env->fonts[0];
  int  x, y;
  int  rw, rh;

  if (!loc_eq(tb->locs[tbin_loCur], tb->locs[tbin_hiCur]) || !tb->cursorVisible)
    return;
  x = but->x + XTextWidth(fs, tb->buf + ti->start,
			  tb->locs[tbin_loCur].index) + tb->lMargin;
  y = but->y + tinNum_line(tb->locs[tbin_loCur].tin, tb) * (fs->ascent + fs->descent);
  if ((rw = (fs->ascent + fs->descent + 10) / 20) < 1)
    rw = 1;
  x -= rw/2;
  if (x < but->x)  {
    rw -= but->x - x;
    x = but->x;
  } else if (x+rw > but->x + but->w)  {
    rw = but->x + but->w - x;
  }
  rh = fs->ascent + fs->descent;
  if (y + rh > but->y + but->h)
    rh = but->y + but->h - y;
  if ((rw <= 0) || (rh <= 0))
    return;
  butEnv_setXFg(env, BUT_FG);
  XFillRectangle(env->dpy, win->win, env->gc, x-win->xOff, y-win->yOff,
		 rw,rh);
}


static void  ti_redrawCursor(But *but)  {
  Tbin  *tb = but->iPacket;
  Tin  *ti = &tb->tins[tb->locs[tbin_loCur].tin];
  ButWin  *win = but->win;
  ButEnv  *env = win->env;
  XFontStruct  *fs = env->fonts[0];
  int  fontH = fs->ascent + fs->descent;
  int  x, y;
  int  rw;

  if (!loc_eq(tb->locs[tbin_loCur], tb->locs[tbin_hiCur]))  {
    return;
  }
  x = but->x + XTextWidth(fs, tb->buf + ti->start,
			  tb->locs[tbin_loCur].index) + tb->lMargin;
  y = but->y + tinNum_line(tb->locs[tbin_loCur].tin, tb) * fontH;
  if ((rw = (fs->ascent + fs->descent + 10) / 20) < 1)
    rw = 1;
  x -= rw/2;
  if (x < but->x)  {
    rw -= (but->x - x);
    x = but->x;
  } else if (x+rw > but->x + but->w)  {
    rw = but->x + but->w - x;
  }
  if (rw <= 0)
    return;
  butWin_redraw(win, x,y, rw,fs->ascent + fs->descent);
}


static void  cut(ButEnv *env, But *but)  {
  Tbin  *tbin = but->iPacket;
  int  storeLen;

  adjustBreak(tbin, break_loCur, 0);
  if (tbin->locs[tbin_loCur].tin == tbin->locs[tbin_hiCur].tin)
    storeLen = tbin->locs[tbin_hiCur].index - tbin->locs[tbin_loCur].index;
  else
    storeLen = tbin->locs[tbin_hiCur].index + tbin->tins[tbin->locs[tbin_hiCur].tin].start -
      tbin->hiBreak;
  XStoreBytes(env->dpy, tbin->buf + tbin->hiBreak, storeLen);
  adjustBreak(tbin, break_eoLoCur, 0);
  if ((env->sReq == NULL) ||
      ((env->sReq == sreq) && (cut_butnum == but)))
    XSetSelectionOwner(env->dpy, XA_PRIMARY, but->win->physWin, 0);
  else
    env->sClear(env);
  if (XGetSelectionOwner(env->dpy, XA_PRIMARY) == but->win->physWin)  {
    env->sReq = sreq;
    env->sClear = sclear;
    cut_butnum = but;
  }
}


static bool  sreq(ButEnv *env, XSelectionRequestEvent *xsre)  {
  Tbin  *tbin;
  int  storeLen;

  if (xsre->target != XA_STRING)
    return(FALSE);
  tbin = cut_butnum->iPacket;
  adjustBreak(tbin, break_loCur, 0);
  if (tbin->locs[tbin_loCur].tin == tbin->locs[tbin_hiCur].tin)
    storeLen = tbin->locs[tbin_hiCur].index - tbin->locs[tbin_loCur].index;
  else
    storeLen = tbin->locs[tbin_hiCur].index + tbin->tins[tbin->locs[tbin_hiCur].tin].start -
      tbin->hiBreak;
  XChangeProperty(env->dpy, xsre->requestor, xsre->property, xsre->target,
		  8, PropModeReplace, tbin->buf + tbin->hiBreak, storeLen);
  adjustBreak(tbin, break_eoLoCur, 0);
  return(TRUE);
}


static int  sclear(ButEnv *env)  {
  Tbin  *tbin = cut_butnum->iPacket;
  
  tbin->locs[tbin_hiCur] = tbin->locs[tbin_loCur];
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
		    topWin->physWin, 0);
}


static int  snotify(ButEnv *env, XSelectionEvent *xsnot)  {
  unsigned char  *propbuf;
  unsigned long  proplen, propleft;
  int  blen;
  int  pformat;
  Atom  ptype;
  Tbin  *tbin;
  char  *pb2;

  tbin = paste_butnum->iPacket;
  if (xsnot->property != None)  {
    XGetWindowProperty(env->dpy, xsnot->requestor,
		       xsnot->property, 0, MAX_PASTE, True, AnyPropertyType,
		       &ptype, &pformat, &proplen, &propleft,
		       &propbuf);
    tbin->locs[tbin_loCur] = tbin->locs[tbin_hiCur] = tbin->locs[tbin_mouse];
    if (insert(env, (char *)propbuf, tbin, paste_butnum->w, NULL) !=
	insert_bad)
      but_draw(paste_butnum);
    else
      XBell(env->dpy, 0);
    XFree(propbuf);
  } else  {
    tbin->locs[tbin_loCur] = tbin->locs[tbin_hiCur] = tbin->locs[tbin_mouse];
    propbuf = (unsigned char *)XFetchBytes(env->dpy, &blen);
    if (blen > 0)  {
      pb2 = (char *)wms_malloc(blen + 1);
      pb2[blen] = '\0';
      memcpy(pb2, propbuf, blen);
      XFree(propbuf);
      if (insert(env, pb2, tbin, paste_butnum->w, NULL))
	but_draw(paste_butnum);
      else
	XBell(env->dpy, 0);
      wms_free(pb2);
    } else
      XBell(env->dpy, 0);
  }		
  env->sNotify = NULL;
  return(0);
}


/*
 * This adjusts your loc and your cutEnd for when you're in word select mode.
 */
static void  wordsel_adjust(Tbin *tbin)  {
  char  *atp, *btp;
  int  a, b, at, bt;
  
  a = tbin->locs[tbin_loCur].index;
  at = tbin->locs[tbin_loCur].tin;
  b = tbin->locs[tbin_hiCur].index;
  bt = tbin->locs[tbin_hiCur].tin;
  if ((at >= 0) && (at < tbin->numTins))
    atp = tbin->buf + tbin->tins[at].start;
  else
    atp = NULL;
  if ((bt >= 0) && (bt < tbin->numTins))
    btp = tbin->buf + tbin->tins[bt].start;
  else
    btp = NULL;
  if (atp)  {
    for (;;)  {
      if (a <= 0)  {
	a = 0;
	break;
      }
      if (((isalnum(atp[a]) || (atp[a] == '_')) &&
	   (isalnum(atp[a-1]) || (atp[a-1] == '_'))) ||
	  (atp[a] == atp[a-1]) ||
	  (isdigit(atp[a]) && (atp[a-1] == '.') &&
	   (a >= 2) && isdigit(atp[a-2])) ||
	  (isdigit(atp[a+1]) && (atp[a] == '.') && isdigit(atp[a-1])))
	--a;
      else
	break;
    }
  }
  if (btp && (b > 0))  {
    for (;;)  {
      if (b >= tbin->tins[bt].len)  {
	b = tbin->tins[bt].len;
	break;
      }
      if (((isalnum(btp[b-1]) || (btp[b-1] == '_')) &&
	   (isalnum(btp[b]) || (btp[b] == '_'))) || (btp[b-1] == btp[b]) ||
	  (isdigit(btp[b-1]) && (btp[b] == '.') && isdigit(btp[b+1])) ||
	  ((b >= 2) && isdigit(btp[b-2]) && (btp[b-1] == '.') &&
	   isdigit(btp[b])))
	++b;
      else
	break;
    }
  }
  tbin->locs[tbin_loCur].index = a;
  tbin->locs[tbin_hiCur].index = b;
}


/* Returns the index of the character just to the right of the cursor. */
static int  locateMouse(XFontStruct *fs, const char *text, int textLen, int x,
			bool  *rightSide)  {
  int  i;
  int  prev_x = x;
  int  spaceWidth;
  bool  dummy;

  if (rightSide == NULL)
    rightSide = &dummy;
  for (i = 0;  (x > 0) && (i < textLen);  ++i)  {
    prev_x = x;
    if (fs->per_char == NULL)
      /* Monospace font. */
      x -= fs->min_bounds.width;
    else
      x -= fs->per_char[(uchar)text[i] - fs->min_char_or_byte2].width;
  }
  if (fs->per_char == NULL)
    spaceWidth = fs->min_bounds.width;
  else
    spaceWidth = fs->per_char[' ' - fs->min_char_or_byte2].width;
  if (x >= spaceWidth)
    *rightSide = TRUE;
  else if ((*rightSide = ((i > 0) && (x < 0) && (prev_x < -x))))
    --i;
  return(i);
}


static void  flags(But *but, uint flags)  {
  Tbin  *tb = but->iPacket;

  if ((but->flags & BUT_KEYED) != (flags & BUT_KEYED))  {
    if (flags & BUT_KEYED)  {
      enableCTimer(but);
      tb->cursorVisible = TRUE;
    } else  {
      tb->cursorVisible = FALSE;
      disableCTimer(but);
    }
  }
  assert((but->flags & BUT_PRESSABLE) || !(but->flags & BUT_KEYED));
  but->flags = flags;
  but_draw(but);
}


static ButOut  blinkCursor(ButTimer *timer)  {
  But  *but = butTimer_packet(timer);
  Tbin  *tb = but->iPacket;

  tb->cursorVisible = !(butTimer_ticks(timer) & 1);
  ti_redrawCursor(but);
  return(0);
}


static void  enableCTimer(But *but)  {
  Tbin *tb = but->iPacket;
  struct timeval  halfSec;

  assert(tb->cTimer == NULL);
  halfSec.tv_sec = 0;
  halfSec.tv_usec = 500000;
  tb->cTimer = butTimer_create(but, but, halfSec, halfSec, TRUE,
			       blinkCursor);
}


static void  disableCTimer(But *but)  {
  Tbin *tb = but->iPacket;

  if (tb->cTimer != NULL)  {
    butTimer_destroy(tb->cTimer);
    tb->cTimer = NULL;
  }
}


void  butTbin_set(But *but, const char *newStr)  {
  Tbin  *tb = but->iPacket;

  assert(MAGIC(but));
  assert(but->action == &action);
  if (but->flags & BUT_KEYED)
    but_setFlags(but, BUT_NOKEY);
  tb->numTins = 1;
  tb->loTinBreak = 1;
  tb->hiTinBreak = tb->maxTins;
  tb->locs[tbin_loCur].index = 0;
  tb->locs[tbin_loCur].tin = 0;
  tb->locs[tbin_hiCur] = tb->locs[tbin_loCur];
  tb->tins[0].len = 0;
  tb->tins[0].start = 0;
  tb->loBreak = 0;
  tb->hiBreak = tb->bufLen;
  insert(but->win->env, newStr, tb, but->w, NULL);
  but_draw(but);
}


static int  calcTinNum(But *but, Tbin *tbin, int y)  {
  int  n;

  if (y < but->y)
    return(0);
  n = (y - but->y) / butEnv_fontH(but->win->env, 0);
  if (n >= tbin->loTinBreak)
    n += tbin->hiTinBreak - tbin->loTinBreak;
  if (n >= tbin->maxTins)
    n = tinNum_prev(tbin->maxTins, tbin);
  return(n);
}


static void  setMouseX(But *but, Tbin *tbin, int tin, int loc)  {
  assert((tin >= 0) && (tin < tbin->maxTins));
  assert((loc >= 0) && (loc <= tbin->tins[tin].len));
  tbin->mouseX = XTextWidth(but->win->env->fonts[0],
			    tbin->buf + tbin->tins[tin].start, loc);
}


static void  checkOffWin(But *but, int mouseY, bool force)  {
  ButWin  *win = but->win;
  Tbin  *tbin = but->iPacket;
  int  y, h, cy, ch, maxCy, loLineNum, hiLineNum;

  assert(MAGIC(but));
  assert(MAGIC(tbin));
  assert(tbin->maxTins == tbin->numTins +
	 tbin->hiTinBreak - tbin->loTinBreak);
  assert(loc_valid(tbin->locs[tbin_loCur], tbin));
  assert(loc_valid(tbin->locs[tbin_hiCur], tbin));
  if (!loc_eq(tbin->locs[tbin_loCur], tbin->locs[tbin_hiCur]) &&
      !tbin->clicks)  {
    /*
     * The text is selected, but no mouse is moving around.  Don't scroll
     *   around!
     * Perhaps we should move around in this case if text is inserted or
     *   something.  Tough call, but probably we shouldn't since inserting
     *   text will (of necessity) move around the cutEnd and the loc.
     */
    return;
  }
  if (tbin->offWinCallback)  {
    y = win->yOff;
    h = win->h;
    ch = butEnv_fontH(win->env, 0);
    if (tbin->clicks)  {
      assert(loc_valid(tbin->locs[tbin_mouse], tbin));
      cy = loLineNum = tinNum_line(tbin->locs[tbin_mouse].tin, tbin);
      hiLineNum = tinNum_line(tbin->locs[tbin_press].tin, tbin);
    } else  {
      cy = loLineNum = hiLineNum = tinNum_line(tbin->locs[tbin_loCur].tin,
					       tbin);
      mouseY = 0;
    }
    cy = but->y + cy * ch;
    maxCy = tbin->numTins * ch;
    if (force || (cy < y) || (cy + ch > y + h) || (maxCy + ch > but->h))  {
      tbin->offWinCallback(but, loLineNum, hiLineNum, mouseY);
    }
  }
}


/**
 * Adjust the breakpoint of the text field.
 *
 * Parameters:
 *   tb - The tbin being adjusted.
 *   bType = Ony of:
 *     break_loCur (move break to the lo cursor)
 *     break_eoLoCur (move break to end of the line containing the lo cursor)
 *     break_eol (move break to the end of line breakTin).
 *   breakTin - Used in break_eol. Zero otherwise.
 */
static void  adjustBreak(Tbin *tb, BreakType bType, int breakTin)  {
  int  newLoBreak, breakIndex = 0, newHiBreak;
  int  origHiTinBreak, origLoTinBreak;
  int  breakDiff;
  TbinLoc  loc;

  assert(tb->hiBreak <= tb->bufLen);
  assert(tb->loBreak <= tb->bufLen);
  assert(tb->hiBreak >= 0);
  assert(tb->loBreak >= 0);
  switch(bType)  {
  case break_loCur:
    assert(breakTin == 0);
    breakTin = tb->locs[tbin_loCur].tin;
    breakIndex = tb->locs[tbin_loCur].index;
    break;
  case break_eoLoCur:
    assert(breakTin == 0);
    breakTin = tb->locs[tbin_loCur].tin;
    breakIndex = tb->tins[tb->locs[tbin_loCur].tin].len;
    break;
  case break_eol:
    assert(breakTin < tb->maxTins);
    assert((breakTin < tb->loTinBreak) || (breakTin >= tb->hiTinBreak));
    breakIndex = tb->tins[breakTin].len;
    break;
  }
  newLoBreak = tb->tins[breakTin].start;
  assert(newLoBreak >= 0);
  if (newLoBreak > tb->loBreak)  {
    assert(newLoBreak >= tb->hiBreak);
    newLoBreak -= tb->hiBreak - tb->loBreak;
  }
  newLoBreak += breakIndex;
  assert(newLoBreak >= 0);
  assert(newLoBreak >= 0);
  assert(newLoBreak <= tb->loBreak + tb->bufLen - tb->hiBreak);
  breakDiff = tb->hiBreak - tb->loBreak;
  newHiBreak = newLoBreak + breakDiff;
  assert(newHiBreak <= tb->bufLen);
  if (newLoBreak < tb->loBreak)  {
    memmove(tb->buf + newHiBreak, tb->buf + newLoBreak,
	    tb->loBreak - newLoBreak);
    origHiTinBreak = tb->hiTinBreak;
    while (tb->loTinBreak > breakTin + 1)  {
      --tb->loTinBreak;
      --tb->hiTinBreak;
      assert(tb->tins[tb->loTinBreak].start <= tb->loBreak);
      tb->tins[tb->hiTinBreak] = tb->tins[tb->loTinBreak];
      tb->tins[tb->hiTinBreak].start += tb->hiBreak - tb->loBreak;
      if (breakTin == tb->loTinBreak)
	breakTin = tb->hiTinBreak;
      assert(tb->tins[tb->loTinBreak].start <= tb->bufLen);
    }
    assert(newLoBreak >= 0);
    tb->loBreak = newLoBreak;
    tb->hiBreak = newHiBreak;
    tbinLoc_iter(loc)  {
      if ((tb->locs[loc].tin < origHiTinBreak) &&
	  (tb->locs[loc].tin >= tb->loTinBreak))
	tb->locs[loc].tin += tb->hiTinBreak - tb->loTinBreak;
    }
    assert(tb->tins[tb->locs[tbin_loCur].tin].start >= 0);
    assert((bType != break_loCur) ||
	   (tb->tins[tb->locs[tbin_loCur].tin].start +
	    tb->locs[tbin_loCur].index == tb->loBreak));
  } else /* newLoBreak >= tb->loBreak */  {
    if (newLoBreak != tb->loBreak)
      memmove(tb->buf + tb->loBreak, tb->buf + tb->hiBreak,
	      newLoBreak - tb->loBreak);
    origLoTinBreak = tb->loTinBreak;
    while (tb->hiTinBreak <= breakTin)  {
      assert(tb->tins[tb->hiTinBreak].start >= tb->hiBreak);
      tb->tins[tb->loTinBreak] = tb->tins[tb->hiTinBreak];
      tb->tins[tb->loTinBreak].start -= tb->hiBreak - tb->loBreak;
      if (breakTin == tb->hiTinBreak)
	breakTin = tb->loTinBreak;
      assert(tb->tins[tb->loTinBreak].start >= 0);
      ++tb->loTinBreak;
      ++tb->hiTinBreak;
    }
    assert(newLoBreak >= 0);
    tb->loBreak = newLoBreak;
    tb->hiBreak = newHiBreak;
    tbinLoc_iter(loc)  {
      if ((tb->locs[loc].tin >= origLoTinBreak) &&
	  (tb->locs[loc].tin < tb->hiTinBreak))
	tb->locs[loc].tin -= tb->hiTinBreak - tb->loTinBreak;
    }
    assert(tb->tins[tb->locs[tbin_loCur].tin].start >= 0);
    assert((bType != break_loCur) ||
	   (tb->tins[tb->locs[tbin_loCur].tin].start +
	    tb->locs[tbin_loCur].index == tb->loBreak));
  }
  assert(tb->hiBreak <= tb->bufLen);
  assert(tb->loBreak <= tb->bufLen);
  assert(tb->hiBreak >= 0);
  assert(tb->loBreak >= 0);
}


static void  addMoreBufferSpace(Tbin *tb)  {
  char  *newBuf;
  int  newLen, i;
  int  newHiBreak;

  newLen = tb->bufLen * 2;
  newBuf = wms_malloc(newLen);
  newHiBreak = tb->hiBreak + newLen - tb->bufLen;
  memcpy(newBuf, tb->buf, tb->loBreak);
  memcpy(newBuf + newHiBreak, tb->buf + tb->hiBreak, newLen - newHiBreak);
  for (i = tb->hiTinBreak;  i < tb->maxTins;  ++i)  {
    tb->tins[i].start += newLen - tb->bufLen;
  }
  wms_free(tb->buf);
  tb->buf = newBuf;
  tb->hiBreak = newHiBreak;
  tb->bufLen = newLen;
}


static void  addMoreTins(Tbin *tb)  {
  Tin  *newTins;
  int  newMaxTins, i;
  int  newHiBreak, tinDiff;

  newMaxTins = tb->maxTins * 2;
  newTins = wms_malloc(newMaxTins * sizeof(Tin));
  newHiBreak = tb->hiTinBreak + newMaxTins - tb->maxTins;
  for (i = 0;  i < tb->loTinBreak;  ++i)  {
    newTins[i] = tb->tins[i];
  }
  tinDiff = newMaxTins - tb->maxTins;
  for (i = tb->hiTinBreak;  i < tb->maxTins;  ++i)  {
    newTins[i + tinDiff] = tb->tins[i];
  }
  if (tb->locs[tbin_mouse].tin >= tb->loTinBreak)
    tb->locs[tbin_mouse].tin += tinDiff;
  if (tb->locs[tbin_press].tin >= tb->loTinBreak)
    tb->locs[tbin_press].tin += tinDiff;
  if (tb->locs[tbin_loCur].tin >= tb->loTinBreak)
    tb->locs[tbin_loCur].tin += tinDiff;
  if (tb->locs[tbin_hiCur].tin >= tb->loTinBreak)
    tb->locs[tbin_hiCur].tin += tinDiff;
  wms_free(tb->tins);
  tb->tins = newTins;
  tb->hiTinBreak = newHiBreak;
  tb->maxTins = newMaxTins;
}


static int  calcTinWidth(Tin *tin, const char *buf, ButEnv  *env)  {
  int  textLen;

  textLen = tin->len;
  while ((textLen > 0) && (buf[tin->start + textLen - 1] == ' '))
    --textLen;
  tin->width = XTextWidth(env->fonts[0], buf + tin->start, textLen);
  return(tin->width);
}


static bool  resize(But *but, int oldX, int oldY, int oldW, int oldH)  {
  Tbin  *tb;
  int  newW, i;

  if (oldW != but->w)  {
    tb = but->iPacket;
    newW = but->w - (tb->rMargin + tb->lMargin);
    for (i = 0;  i < tb->maxTins;  i = tinNum_next(i, tb))  {
      if (tb->tins[i].width > newW)
	breakLine(tb, i, but->win->env, newW);
    }
    return(TRUE);
  } else
    return((but->x != oldX) || (but->y != oldY) || (but->h != oldH));
}


static void  killTopLines(Tbin *tb, int lines)  {
  int  charsDead, i;
  TbinLoc  loc;

  assert(lines < tb->numTins);
  adjustBreak(tb, break_eol, tinNum_prev(tb->maxTins, tb));
  charsDead = tb->tins[lines].start;
  memmove(tb->buf, tb->buf + charsDead, tb->loBreak - charsDead);
  tb->loBreak -= charsDead;
  tb->numTins -= lines;
  tb->loTinBreak -= lines;
  for (i = 0;  i < tb->loTinBreak;  ++i)  {
    tb->tins[i].start = tb->tins[i + lines].start - charsDead;
    tb->tins[i].len = tb->tins[i + lines].len;
    tb->tins[i].width = tb->tins[i + lines].width;
  }
  tbinLoc_iter(loc)  {
    tb->locs[loc].tin -= lines;
    if (tb->locs[loc].tin < 0)  {
      tb->locs[loc].tin = 0;
      tb->locs[loc].index = 0;
    }
  }
}


void  butTbin_insert(But *but, const char *appText)  {
  bool  saveOldCurs;
  Tbin  *tb;

  assert(MAGIC(but));
  tb = but->iPacket;
  assert(MAGIC(tb));
  saveOldCurs = (!loc_eq(tb->locs[tbin_loCur], tb->locs[tbin_hiCur]) ||
		 (tinNum_next(tb->locs[tbin_loCur].tin, tb) !=
		  tb->maxTins));
  if (saveOldCurs)  {
    tb->locs[tbin_mouse] = tb->locs[tbin_loCur];
    tb->locs[tbin_press] = tb->locs[tbin_hiCur];
    tb->locs[tbin_loCur].tin = tinNum_prev(tb->maxTins, tb);
    tb->locs[tbin_loCur].index = tb->tins[tb->locs[tbin_loCur].tin].len;
    tb->locs[tbin_hiCur] = tb->locs[tbin_loCur];
  }
  insert(but->win->env, appText, tb,
	 but->w - (tb->lMargin + tb->rMargin), NULL);
  checkOffWin(but, 0, TRUE);
  if (saveOldCurs && !loc_eq(tb->locs[tbin_mouse], tb->locs[tbin_press]))  {
    tb->locs[tbin_loCur] = tb->locs[tbin_mouse];
    tb->locs[tbin_hiCur] = tb->locs[tbin_press];
  }
  but_draw(but);
}  


int  butTbin_len(But *but)  {
  Tbin  *tb;

  assert(MAGIC(but));
  tb = but->iPacket;
  assert(MAGIC(tb));
  return(tb->bufLen - (tb->hiBreak - tb->loBreak));
}


void  butTbin_setMaxLines(But *but, int maxLines)  {
  Tbin  *tb = but->iPacket;

  assert(MAGIC(tb));
  tb->maxLines = maxLines;
}


void  butTbin_setReadOnly(But *but, bool ro)  {
  Tbin  *tb = but->iPacket;

  assert(MAGIC(tb));
  tb->readOnly = ro;
}


void  butTbin_delete(But *but, int delStart, int delLen)  {
  Tbin  *tb = but->iPacket;

  assert(but->action == &action);
  assert(MAGIC(tb));
  tb->locs[tbin_mouse] = tb->locs[tbin_loCur];
  tb->locs[tbin_press] = tb->locs[tbin_hiCur];
  setLoc(tb, tbin_loCur, delStart);
  setLoc(tb, tbin_hiCur, delStart + delLen);
  insert(but->win->env, "", tb, but->w, NULL);
  tb->locs[tbin_loCur] = tb->locs[tbin_mouse];
  tb->locs[tbin_hiCur] = tb->locs[tbin_press];
}


static void  setLoc(Tbin *tb, TbinLoc locNum, int position)  {
  Loc  *l = &tb->locs[locNum];
  int  i;

  if (position > tb->loBreak)
    position += (tb->hiBreak - tb->loBreak);
  i = 0;
  while (tb->tins[i].start + tb->tins[i].len < position)  {
    i = tinNum_next(i, tb);
    assert(i < tb->maxTins);
  }
  l->tin = i;
  l->index = position - tb->tins[i].start;
}


#endif  /* X11_DISP */
