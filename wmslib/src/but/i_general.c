/*
 * wmslib/src/but/i_general.c, part of wmslib (Library functions)
 * Copyright (C) 1994,1997 William Shubert.
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
#include <but/net.h>
#include <but/timer.h>
#include <but/canvas.h>


/**********************************************************************
 * Forward Declarations
 **********************************************************************/
static void  mousein(ButWin *win, int x, int y, ButSet *butset);


/**********************************************************************
 * Globals
 **********************************************************************/
bool  but_inEvent = FALSE;


/**********************************************************************
 * Functions
 **********************************************************************/
void  but_init(But *but)  {
  ButWin  *win = but->win;
  ButEnv  *env = win->env;
  int  nfl;

  assert(MAGIC(win));
  MAGIC_SET(but);
  if (!(but->flags & BUT_DRAWABLE))
    return;
  if (win->maxLayer < win->minLayer)
    win->maxLayer = win->minLayer = but->layer;
  else  {
    if (but->layer < win->minLayer)
      win->minLayer = but->layer;
    if (but->layer > win->maxLayer)
      win->maxLayer = but->layer;
  }
  butSet_addBut(&win->butsNoDraw, but);
  if (but->w > 0)
    but_addToTable(but);
  if (but->flags & BUT_KEYED)  {
    if (win->keyBut != NULL)  {
      nfl = win->keyBut->flags & ~BUT_KEYED;
      if (env->last_mwin == win)
	nfl &= ~BUT_KEYACTIVE;
      but_newFlags(win->keyBut, nfl);
    }
    win->keyBut = but;
    if (env->last_mwin == win)
      but->flags |= BUT_KEYACTIVE;
  }
  nfl = but->flags;
  but->flags = BUT_DRAWABLE;  /* Force a new flags type redraw. */
  but_newFlags(but, nfl);
  butWin_mMove(env->last_mwin, env->last_mx, env->last_my);
}


void  but_resize(But *but, int x, int y, int w, int h)  {
  int  oldx = but->x, oldy = but->y, oldw = but->w, oldh = but->h;

  assert(MAGIC(but));
  if ((oldx == x) && (oldy == y) && (oldw == w) && (oldh == h))
    return;
  if ((w <= 0) || (h <= 0))
    w = h = 0;
  if ((oldw > 0) && (but->flags & BUT_DRAWABLE))
    but_delFromTable(but);
  but->x = x;
  but->y = y;
  but->w = w;
  but->h = h;
  if ((w > 0) && (but->flags & BUT_DRAWABLE))
    but_addToTable(but);
  if (but->action->resize)  {
    if (but->action->resize(but, oldx, oldy, oldw, oldh) &&
	(but->flags & BUT_DRAWABLE))  {
      butWin_redraw(but->win, oldx,oldy, oldw,oldh);
      butWin_redraw(but->win, x,y, w,h);
    }
  } else if (but->flags & BUT_DRAWABLE)  {
    if ((oldw > 0) && (oldh > 0))
      butWin_redraw(but->win, oldx,oldy, oldw,oldh);
    butWin_redraw(but->win, x,y, w,h);
  }
  if (but->flags & BUT_DRAWABLE)
    butWin_mMove(but->win->env->last_mwin,
		 but->win->env->last_mx, but->win->env->last_my);
}


void  but_destroy(But *but)  {
  ButTimer  *timer;

  if (but != NULL)  {
    assert(MAGIC(but));
    but_newFlags(but, 0);
    for (timer = but_timerList;  timer != NULL;  timer = timer->next)  {
      if (timer->but == but)  {
	butTimer_destroy(timer);
	break;
      }
    }
  }
  if (but_inEvent)
    but_dList(but);
  else
    but_delete(but);
}


But  *but_create(ButWin *win, void *iPacket, const ButAction *action)  {
  But  *but;

  but = (But *)wms_malloc(sizeof(But));
  MAGIC_SET(but);
  assert(win->parent || (win->win != None));
  but->uPacket = NULL;
  but->iPacket = iPacket;
  but->win = win;
  but->layer = 0;
  but->x = 0;
  but->y = 0;
  but->w = 0;
  but->h = 0;
  but->id = -2;
  but->flags = 0;
  but->keys = NULL;
  but->action = action;
  but->destroyCallback = NULL;
  return(but);
}  


ButOut  but_delete(But *but)  {
  ButOut  result = 0;
  ButWin  *win = but->win;
  int  x = but->x, y = but->y, w = but->w, h = but->h;

  assert(but != NULL);
  assert(MAGIC(win));
  assert(MAGIC(but));
  if (but->flags)
    but_newFlags(but, 0);
  if (but->destroyCallback)
    but->destroyCallback(but);
  if (but->action->destroy != NULL)
    result = but->action->destroy(but);
  MAGIC_UNSET(but);
  wms_free(but);
  butWin_redraw(win, x,y,w,h);
  return(result);
}


void  but_setFlags(But *but, uint fch)  {
  uint  fl;

  assert(MAGIC(but));
  fl = but->flags | fch;
  fl &= ~(fch >> BUT_MAXBITS);
  but_newFlags(but, fl);
}


void  but_setKeys(But *but, const ButKey *keys)  {
  assert(MAGIC(but));
  assert((but->action->kPress) || (but->action->kRelease));
  but->keys = keys;
}
  

void  but_newFlags(But *but, uint nfl)  {
  static bool  inNewFlags = FALSE;
  ButWin  *win = but->win, *tmpWin, *ancestor;
  ButEnv  *env = win->env;
  uint  oldflags = but->flags;
  
  assert(MAGIC(win));
  assert(MAGIC(but));
  for (ancestor = win;  ancestor->parent;  ancestor = ancestor->parent)
    assert(MAGIC(ancestor));
  if (!(nfl & BUT_PRESSABLE))  {
    nfl &= ~(BUT_PRESSED|BUT_TWITCHED|BUT_LOCKED);
    if (env->curhold == but)
      butEnv_setCursor(env, but, butCur_idle);
  }
  if (!(oldflags & BUT_DRAWABLE) && (nfl & BUT_DRAWABLE))  {
    but->flags = nfl;
    but_init(but);
    return;
  }
  if (!(nfl & BUT_DRAWABLE) && (env->butIn == but))  {
    env->butIn = NULL;
    for (tmpWin = win;  tmpWin;  tmpWin = tmpWin->parent)  {
      assert(MAGIC(tmpWin));
      tmpWin->butIn = NULL;
    }
  }
  if ((oldflags & BUT_KEYED) && !(nfl & BUT_KEYED))  {
    nfl &= ~BUT_KEYACTIVE;
    ancestor->keyBut = NULL;
  }
  if (!(oldflags & BUT_KEYED) && (nfl & BUT_KEYED))  {
    if (ancestor->keyBut != NULL)  {
      but_newFlags(ancestor->keyBut, ancestor->keyBut->flags & ~BUT_KEYED);
    }
    ancestor->keyBut = but;
    if (env->last_mwin == but->win)  {
      nfl |= BUT_KEYACTIVE;
    }
  }
  if ((but->id != -2) && !inNewFlags &&
      ((nfl & BUT_NETMASK) != (but->flags & BUT_NETMASK)))  {
    inNewFlags = TRUE;
    butRnet_newFlags(env, but->id, nfl);
    inNewFlags = FALSE;
  }
  if (!(oldflags & BUT_LOCKED) && (nfl & BUT_LOCKED))  {
    if (env->lockBut != NULL)
      but_newFlags(env->lockBut, env->lockBut->flags & ~BUT_LOCKED);
    env->lockBut = but;
    win->lock = but;
    for (tmpWin = env->lockBut->win;  tmpWin->parent;
	 tmpWin = tmpWin->parent)  {
      assert(MAGIC(tmpWin));
      tmpWin->parent->lock = tmpWin->parentBut;
    }
  } else if ((oldflags & BUT_LOCKED) && !(nfl & BUT_LOCKED))  {
    for (tmpWin = but->win;  tmpWin;  tmpWin = tmpWin->parent)  {
      assert(MAGIC(tmpWin));
      tmpWin->lock = NULL;
    }
    env->lockBut = NULL;
  }
  if (nfl != oldflags)  {
    assert(but->action->newFlags);
    but->action->newFlags(but, nfl);
  }
  if ((oldflags & BUT_DRAWABLE) && !(nfl & BUT_DRAWABLE))  {
    if (but->w > 0)  {
      but_delFromTable(but);
    }
  }
  butWin_mMove(env->last_mwin, env->last_mx, env->last_my);
}


void  but_draw(But *but)  {
  assert(MAGIC(but));
  if ((but->flags & (BUT_DRAWABLE|BUT_DEAD)) == BUT_DRAWABLE)
    butWin_redraw(but->win, but->x,but->y, but->w,but->h);
}


ButOut  butWin_mMove(ButWin *win, int x, int y)  {
  ButEnv  *env;
  ButOut  result = 0;
  int  i;
  But  *but, *oldbut;
  ButSet  butset;
  ButWin  *tmpWin;

  assert(MAGICNULL(win));
  if (win == NULL)
    return(0);
  env = win->env;
  if ((env->last_mwin != win) &&
      (!env->last_mwin || (env->last_mwin->physWin != win->physWin)))  {
    env->curwin = win->physWin;
    env->curlast = butCur_bogus;
  }
  if (win->parent == NULL)  {
    /* Only do this if you're on a real window, that is, not in a canvas. */
    env->last_mwin = win;
    env->last_mx = x;
    env->last_my = y;
  }
  mousein(win, x, y, &butset);
  for (i = butset.numButs - 1;  i >= 0;  --i)  {
    but = butset.buts[i];
    assert(MAGIC(but));
    if ((but->flags & BUT_DRAWABLE) && !(but->flags & BUT_PRESSTHRU))  {
      if (but->action->mMove == NULL)
	result |= BUTOUT_CAUGHT;
      else
	result |= but->action->mMove(but, x,y);
      if (result & BUTOUT_CAUGHT)  {
	if (result & BUTOUT_IGNORE)
	  return(result);
	if (env->butIn != but)  {
	  if (env->butIn != NULL)  {
	    for (tmpWin = env->butIn->win;  tmpWin;
		 tmpWin = tmpWin->parent)  {
	      tmpWin->butIn = NULL;
	    }
	    if ((env->butIn->flags & BUT_PRESSABLE) &&
		(env->butIn->action->mLeave != NULL))
	      result |= env->butIn->action->mLeave(env->butIn);
	  }
	  env->butIn = but;
	  but->win->butIn = but;
	  for (tmpWin = but->win;  tmpWin->parent;  tmpWin = tmpWin->parent)  {
	    tmpWin->parent->butIn = tmpWin->parentBut;
	  }
	}
	return(result);
      }
    }
  }
  if (env->butIn != NULL)  {
    oldbut = env->butIn;
    for (tmpWin = env->butIn->win;  tmpWin;  tmpWin = tmpWin->parent)  {
      tmpWin->butIn = NULL;
    }
    env->butIn = NULL;
    if ((oldbut->flags & BUT_PRESSABLE) && (oldbut->action->mLeave != NULL))
      result |= oldbut->action->mLeave(oldbut);
  }
  return(result);
}


ButOut  butWin_mPress(ButWin *win, int x, int y, int butnum)  {
  ButOut  result;
  But  *but;
  
  assert(MAGIC(win));
  result = butWin_mMove(win, x, y);
  but = win->butIn;
  if (but != NULL)  {
    assert(MAGIC(but));
    if ((but->flags & BUT_PRESSABLE) && (but->action->mPress != NULL))  {
      return(result | but->action->mPress(but, butnum, x, y));
    }
  }
  return(result | BUTOUT_ERR);
}


ButOut  butWin_mRelease(ButWin *win, int x, int y, int butnum)  {
  ButOut  result;
  But  *but;
  
  assert(MAGIC(win));
  result = butWin_mMove(win, x, y);
  but = win->butIn;
  if (but != NULL)  {
    assert(MAGIC(but));
    if ((but->flags & BUT_PRESSABLE) && (but->action->mRelease != NULL))
    return(result | but->action->mRelease(but, butnum, x, y));
  }
  return(result);
}


static int  keycheck(But *but, void *packet)  {
  int  keycount;
  KeySym  sym = *(KeySym *)packet;
  
  assert(MAGIC(but));
  if (but->keys != NULL)  {
    for (keycount = 0;  but->keys[keycount].key != 0;  ++keycount)  {
      if ((but->keys[keycount].key == sym) &&
	  ((but->win->env->keyModifiers & but->keys[keycount].modMask) ==
	   but->keys[keycount].modifiers))
	return(1);
    }
  }
  return(0);
}


ButOut  butWin_kPress(ButWin *win, const char *keystr, KeySym sym)  {
  ButOut  retVal = 0;
  But  *but;

  assert(MAGIC(win));
  /*
   * The keybut _always_ get a shot at the key when it is pressed.
   */
  if (win->keyBut)  {
    assert(win->keyBut->flags & BUT_PRESSABLE);
    retVal |= win->keyBut->action->kPress(win->keyBut, keystr, sym);
  }
  if (!(retVal & BUTOUT_CAUGHT))  {
    but = butWin_findButsInRegion(win, 0,0, win->w,win->h, keycheck, &sym);
    if ((but != NULL) && (but->flags & BUT_PRESSABLE) &&
	(but->action->kPress != NULL))  {
      assert(MAGIC(but));
      retVal |= but->action->kPress(but, keystr, sym);
    } else  {
      /*
       * A key was pressed that nobody wanted.
       * But only signal an error if it was a "real" key...don't beep when
       *   some poor guy presses shift!
       */
      if (!IsModifierKey(sym))
	retVal |= BUTOUT_ERR;
    }
  }
  return(retVal);
}


ButOut  butWin_kRelease(ButWin *win, const char *keystr, KeySym sym)  {
  ButOut  retval = 0;
  But  *but;

  assert(MAGIC(win));
  but = butWin_findButsInRegion(win, 0,0, win->w,win->h, keycheck, &sym);
  if (but != NULL)  {
    assert(MAGIC(but));
    if ((but->flags & BUT_PRESSABLE) && (but->action->kRelease != NULL))
      retval |= but->action->kRelease(but, keystr, sym);
  }
  return(retval);
}


/* mousein(win, x, y, reason) will return the button number that the mouse is
 *   presently inside of.  If the mouse is not in any button NULL will
 *   be returned.
 * The checkfunc will be called only if the mouse is already inside the
 *   (x,y,w,h) boundaries of the button.  "reason" is one of BUT_RE_PRESS,
 *   BUT_RE_RELEASE, or BUT_RE_MOVE, depending on the reason for the button
 *   to be checked.
 * Non-pressable buttons will never be returned.
 *
 * NOTE: This does a linear search through the buttons.  YUCK!  SLOW!  I'd
 *   like to recode this to do a three-dimensional binary search (the
 *   dimensions being window, x, and y) but that would take some seriously
 *   heavy-duty coding and I have a million other things to do first.
 *
 * NOTE to note: I finally did the 3d binary search.  Works great!
 */
static void  mousein(ButWin *win, int x, int y, ButSet *butset)  {
  static But  *but;
  
  assert(MAGIC(win));
  if (win->lock)  {
    butset->dynamic = FALSE;
    butset->numButs = 1;
    butset->buts = &but;
    but = win->lock;
  } else
    butWin_findButSet(win, x, y, butset);
}


static int  but_compare(const void *a, const void *b)  {
  But **but_a = (But **)a, **but_b = (But **)b;

  assert(MAGIC(*but_a));
  assert(MAGIC(*but_b));
  return((*but_a)->layer - (*but_b)->layer);
}


void  butWin_redraw(ButWin *win, int x, int y, int w, int h)  {
  ButWinArea  *newAreas;
  int  newMaxRedraws, i;
  bool  change;

  assert((x > -1000) && (y > -1000));
  if (!w || !h)
    return;
  assert(w > 0);
  assert(h > 0);
  if (win->numRedraws + 1 >= win->maxRedraws)  {
    newMaxRedraws = (win->maxRedraws + 1) * 2;
    newAreas = wms_malloc(newMaxRedraws * sizeof(ButWinArea));
    for (i = 0;  i < win->numRedraws;  ++i)  {
      newAreas[i] = win->redraws[i];
    }
    if (win->redraws)
      wms_free(win->redraws);
    win->redraws = newAreas;
    win->maxRedraws = newMaxRedraws;
  }
  assert(win->numRedraws < win->maxRedraws);
  do  {
    assert(w > 0);
    assert(h > 0);
    change = FALSE;
    for (i = 0;  i < win->numRedraws;  ++i)  {
      if ((x <= win->redraws[i].x + win->redraws[i].w) &&
	  (y <= win->redraws[i].y + win->redraws[i].h) &&
	  (x + w >= win->redraws[i].x) &&
	  (y + h >= win->redraws[i].y))  {
	change = TRUE;
	if (win->redraws[i].x < x)  {
	  w += x - win->redraws[i].x;
	  x = win->redraws[i].x;
	}
	if (win->redraws[i].y < y)  {
	  h += y - win->redraws[i].y;
	  y = win->redraws[i].y;
	}
	if (x + w < win->redraws[i].x + win->redraws[i].w)
	  w = win->redraws[i].x + win->redraws[i].w - x;
	if (y + h < win->redraws[i].y + win->redraws[i].h)
	  h = win->redraws[i].y + win->redraws[i].h - y;
	win->redraws[i] = win->redraws[--win->numRedraws];
      }
    }
  } while (change);
  win->redraws[win->numRedraws].x = x;
  win->redraws[win->numRedraws].y = y;
  win->redraws[win->numRedraws].w = w;
  win->redraws[win->numRedraws].h = h;
  assert(w > 0);
  assert(h > 0);
  ++win->numRedraws;
  assert(win->numRedraws < win->maxRedraws);
}


void  butWin_performDraws(ButWin *win)  {
  int  drawNum;
  int  i, drx,dry, drw,drh, x, y, w, h;
  ButEnv  *env = win->env;
  XRectangle  cliprect;
  ButSet  butset;
  But  *but;

  assert(MAGIC(win));
  if (!win->mapped)  {
    win->numRedraws = 0;
    return;
  }
  for (drawNum = 0;  drawNum < win->numRedraws;  ++drawNum)  {
    x = win->redraws[drawNum].x;
    y = win->redraws[drawNum].y;
    w = win->redraws[drawNum].w;
    h = win->redraws[drawNum].h;
    assert(w > 0);
    assert(h > 0);
    if (x < win->xOff)  {
      w -= win->xOff - x;
      x = win->xOff;
    }
    if (x + w > win->xOff + win->w)
      w = win->xOff + win->w - x;
    if (y < win->yOff)  {
      h -= win->yOff - y;
      y = win->yOff;
    }
    if (y + h > win->yOff + win->h)
      h = win->yOff + win->h - y;
    if ((w <= 0) || (h <= 0))
      continue;
    if (win->id != -2)
      butRcur_redraw(env, win->id, x,y,w,h);
    butWin_findButSetInRegion(win, x,y, w,h, &butset);
    if (butset.numButs > 0)  {
      cliprect.x = x - win->xOff;
      cliprect.y = y - win->yOff;
      cliprect.width = w;
      cliprect.height = h;
      XSetClipRectangles(env->dpy, env->gc, 0,0, &cliprect, 1, YXSorted);
      qsort(butset.buts, butset.numButs, sizeof(But *), but_compare);
      assert(MAGIC(win));
      for (i = butset.numButs - 1;  i > 0;  --i)  {
	but = butset.buts[i];
	assert(MAGIC(but));
	if ((but->flags & BUT_OPAQUE) && (but->x <= x) && (but->y <= y) &&
	    (but->x + but->w >= x + w) && (but->y + but->h >= y + h))
	  break;
      }
      for (;  i < butset.numButs;  ++i)  {
	but = butset.buts[i];
	assert(MAGIC(but));
	if (!(but->flags & BUT_DEAD))  {
	  if (but->x < x)
	    drx = x;
	  else
	    drx = but->x;
	  if (but->y < y)
	    dry = y;
	  else
	    dry = but->y;
	  if (but->x + but->w < x+w)
	    drw = but->x + but->w - drx;
	  else
	    drw = x + w - drx;
	  if (but->y + but->h < y+h)
	    drh = but->y + but->h - dry;
	  else
	    drh = y+h - dry;
	  but->action->draw(but, x,y, w,h);
	}
      }
      XSetClipMask(env->dpy, env->gc, None);
      butSet_destroy(&butset);
      if (win->parent)  {
	/*
	 * It's a canvas.  Fire off a redraw request so that the changes to
	 *   the canvas pixmap will be seen on the screen.
	 */
	butCan_redrawn(win, x, y, w, h);
      }
    }
  }
  win->numRedraws = 0;
}
    

void  butSet_destroy(ButSet *butset)  {
  if (butset->dynamic && (butset->buts != NULL))
    wms_free(butset->buts);
}


/* If win is NULL it will execute all destroy commands built up. */
ButOut  butWin_dList(ButWin *win)  {
  static ButWin  **dlist = NULL;
  static int  dlist_len = 0, max_dlist_len = 0;
  ButWin  **new_dlist;
  int  i;
  ButOut  result = 0;

  assert(MAGICNULL(win));
  if (win == NULL)  {
    /*
     * It is really important that we start at the end here.  Since our
     *   child windows are added to the _end_ of the dlist, and they _must_
     *   be destroyed first, we have to go backwards.
     */
    while (dlist_len)  {
      i = dlist_len - 1;
      if (dlist[i] != NULL)  {
	assert(MAGIC(dlist[i]));
	result |= butWin_delete(dlist[i]) | BUTOUT_CAUGHT;
	if (result & BUTOUT_STILLBUTS)  {
	  /* Window still has buttons...can't kill it yet! */
	  return(result & ~BUTOUT_STILLBUTS);
	}
	dlist[i] = NULL;
      }
      --dlist_len;
    }
  } else  {
    if (win->iconWin != NULL)  {
      butWin_dList(win->iconWin);
      win->iconWin = NULL;
    }
    if (dlist_len+1 > max_dlist_len)  {
      new_dlist = wms_malloc(sizeof(ButWin *) * (dlist_len * 2 + 2));
      for (i = 0;  i < dlist_len;  ++i)
	new_dlist[i] = dlist[i];
      if (dlist != NULL)
	wms_free(dlist);
      dlist = new_dlist;
      max_dlist_len = (dlist_len * 2 + 2);
    }
    for (i = 0;  i < dlist_len;  ++i)
      if (dlist[i] == win)
	return(result);
    dlist[dlist_len++] = win;
  }
  return(result);
}


/* If but is NULL it will execute all destroy commands built up. */
ButOut  but_dList(But *but)  {
  static But  **dlist = NULL;
  static int  dlist_len = 0, max_dlist_len = 0;
  But  **new_dlist;
  int  i;
  ButOut  result = 0;

  if (but == NULL)  {
    if (dlist_len == 0)
      return(0);
    for (i = 0;  i < dlist_len;  ++i)  {
      result |= but_delete(dlist[i]) | BUTOUT_CAUGHT;
    }
    dlist_len = 0;
  } else  {
    if (dlist_len+1 > max_dlist_len)  {
      new_dlist = wms_malloc(sizeof(But *) * (dlist_len * 2 + 2));
      for (i = 0;  i < dlist_len;  ++i)
	new_dlist[i] = dlist[i];
      if (dlist != NULL)
	wms_free(dlist);
      dlist = new_dlist;
      max_dlist_len = (dlist_len * 2 + 2);
    }
    for (i = 0;  i < dlist_len;  ++i)
      if (dlist[i] == but)
	return(result);
    but_newFlags(but, 0);
    dlist[dlist_len++] = but;
    but->flags |= BUT_DEAD;
  }
  return(result);
}


void  but_flags(But *but, uint flags)  {
  but->flags = flags;
  but_draw(but);
}


#endif  /* X11_DISP */
