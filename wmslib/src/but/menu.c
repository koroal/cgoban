/*
 * wmslib/src/but/menu.c, part of wmslib (Library functions)
 * Copyright (C) 1994-1996 William Shubert.
 * See "configure.h.in" for more copyright information.
 *
 * Known problems:
 *   The first and last options MUST be text, not a divider.
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
#include <but/menu.h>
#include <but/box.h>
#include <but/menu_snd.h>
#include <wms/str.h>


/**********************************************************************
 * Data structures
 **********************************************************************/
typedef struct Ol_struct  {  /* Option List */
  Str  text;
  int  x,y,w,h;
  uint  flags;
} Ol;

typedef struct menu_struct  {
  int  fgpic, bgpic;
  int  cval, nvals, nbreaks, tval, xoff;
  int  last_w, llayer;
  bool  active, up, skip_clicks;
  bool  menu_was_twitched;
  bool  clickOpen;  /* TRUE if it's open because of a click. */
  Time  pressTime;
  const char  *title;
  Ol  *options;
  But  *child, *parent;
  ButOut  (*callback)(But *but, int value);
} Menu;


/**********************************************************************
 * Forward Declarations
 **********************************************************************/
static int  menu_width(But *but);
static ButOut  smmove(But *but, int x, int y);
static ButOut  smleave(But *but);
static ButOut  smpress(But *but, int butnum, int x, int y);
static void  sdraw(But *but, int x,int y, int w,int h);
static ButOut  sdestroy(But *but);
static void  snewflags(But *but, uint flags);
static But  *lcreate(But *smenu, bool local);
static ButOut  lmmove(But *but, int x, int y);
static ButOut  lmleave(But *but);
static ButOut  lmpress(But *but, int butnum, int x, int y);
static ButOut  lmrelease(But *but, int butnum, int x, int y);
static void  ldraw(But *but, int x,int y, int w,int h);
static ButOut  ldestroy(But *but);
static void  lnewflags(But *but, uint flags);
static ButOut  netMsg(But *but, void *msg, int msgLen);
static But  *create(ButOut (*func)(But *but, int value), void *packet,
		    ButWin *win, int layer, int toplayer, int flags,
		    const char *title, const char *optlist[], int cur_opt,
		    bool up);


/**********************************************************************
 * Globals
 **********************************************************************/
static const ButAction  saction = {
  smmove, smleave, smpress, NULL,
  NULL, NULL, sdraw, sdestroy, snewflags, netMsg};

static const ButAction  laction = {
  lmmove, lmleave, lmpress, lmrelease,
  NULL, NULL, ldraw, ldestroy, lnewflags, NULL};

char  butMenu_dummy = '\0';


/**********************************************************************
 * Functions
 **********************************************************************/
But  *butMenu_upCreate(ButOut (*func)(But *but, int value), void *packet,
		       ButWin *win, int layer, int toplayer, int flags,
		       const char *title, const char *optlist[],
		       int cur_opt)  {
  return(create(func, packet, win, layer, toplayer, flags, title, optlist,
		cur_opt, TRUE));
}


But  *butMenu_downCreate(ButOut (*func)(But *but, int value), void *packet,
			 ButWin *win, int layer, int toplayer, int flags,
			 const char *title, const char *optlist[],
			 int cur_opt)  {
  return(create(func, packet, win, layer, toplayer, flags, title, optlist,
		cur_opt, FALSE));
}


static But  *create(ButOut (*callback)(But *but, int value), void *packet,
		    ButWin *win, int layer, int toplayer, int flags,
		    const char *title, const char *optlist[],
		    int cur_opt, bool up)  {
  Menu  *m;
  But  *but;
  int  i;
  
  m = wms_malloc(sizeof(Menu));
  but = but_create(win, m, &saction);
  but->uPacket = packet;
  but->layer = layer;
  but->flags = flags | BUT_OPAQUE;

  m->callback = callback;
  m->fgpic = BUT_FG;
  m->bgpic = BUT_BG;
  m->cval = cur_opt;
  m->nbreaks = 0;
  for (i = 0;  optlist[i] != BUTMENU_OLEND;  ++i)
    if (optlist[i] == BUTMENU_OLBREAK)
      ++m->nbreaks;
  m->nvals = i;
  m->options = wms_malloc((i+1) * sizeof(Ol));
  i = 0;
  do  {
    if (optlist[i] == BUTMENU_OLBREAK)  {
      str_init(&m->options[i].text);
      m->options[i].flags = BUTMENU_BREAK;
    } else if (optlist[i] == BUTMENU_OLEND)  {
      str_init(&m->options[i].text);
      m->options[i].flags = BUTMENU_END;
    } else  {
      str_initChars(&m->options[i].text, optlist[i]);
      m->options[i].flags = 0;
    }
  } while (optlist[i++] != BUTMENU_OLEND);
  m->llayer = toplayer;
  m->tval = 0;
  m->active = FALSE;
  m->up = up;
  m->menu_was_twitched = FALSE;
  m->title = title;
  m->last_w = 0;
  m->child = NULL;
  m->parent = but;
  but_init(but);
  return(but);
}


void  butMenu_setText(But *but, const char *title, const char *optlist[],
		      int newopt)  {
  Menu  *m = but->iPacket;
  int  i;

  if (title == NULL)
    title = m->title;
  m->title = title;
  m->nbreaks = 0;
  for (i = 0;  optlist[i] != BUTMENU_OLEND;  ++i)
    if (optlist[i] == BUTMENU_OLBREAK)
      ++m->nbreaks;
  m->nvals = i;
  wms_free(m->options);
  m->options = wms_malloc((i+1) * sizeof(Ol));
  i = 0;
  do  {
    if (optlist[i] == BUTMENU_OLBREAK)  {
      str_init(&m->options[i].text);
      m->options[i].flags = BUTMENU_BREAK;
    } else if (optlist[i] == BUTMENU_OLEND)  {
      str_init(&m->options[i].text);
      m->options[i].flags = BUTMENU_END;
    } else  {
      str_copyChars(&m->options[i].text, optlist[i]);
      m->options[i].flags = 0;
    }
  } while (optlist[i++] != BUTMENU_OLEND);
  if (newopt != BUT_NOCHANGE)
    m->cval = newopt;
  but_draw(but);
}


void  butMenu_setFlags(But *but, int optnum, uint flags)  {
  Menu  *m = but->iPacket;

  m->options[optnum].flags = flags;
  but_draw(but);
}


/* Cannot be called when the menu is open. */
void  butMenu_set(But *but, int new_opt)  {
  Menu  *m = but->iPacket;
  assert(but->action == &saction);
  m->cval = new_opt;
  but_draw(but);
}


int  butMenu_get(But *but)  {
  Menu  *m = but->iPacket;
  
  return(m->cval);
}


void  butMenu_setColor(But *but, int fg, int bg)  {
  Menu  *m = but->iPacket;
  
  assert(but->action == &saction);
  m->fgpic = fg;
  m->bgpic = bg;
  but_draw(but);
}


static ButOut  smmove(But *but, int x, int y)  {
  ButOut  retval = BUTOUT_CAUGHT;
  int  newflags = but->flags;
  int  butbw = but->win->env->stdButBw;
  
  if ((x >= but->x + butbw) && (x < but->x + but->w - butbw) &&
      (y >= but->y + butbw) && (y < but->y + but->h - butbw))  {
    newflags |= BUT_TWITCHED;
  } else  {
    newflags &= ~BUT_TWITCHED;
    if (!newflags & BUT_LOCKED)
      retval &= ~BUTOUT_CAUGHT;
  }
  if (!(but->flags & BUT_TWITCHED) && (newflags & BUT_TWITCHED))
    butEnv_setCursor(but->win->env, but, butCur_twitch);
  else if ((but->flags & BUT_TWITCHED) && !(newflags & BUT_TWITCHED))
    butEnv_setCursor(but->win->env, but, butCur_idle);
  if (newflags != but->flags)
    but_newFlags(but, newflags);
  return(retval);
}


static ButOut  smleave(But *but)  {
  int  newflags = but->flags;

  newflags &= ~BUT_TWITCHED;
  if ((but->flags & BUT_TWITCHED) && !(newflags & BUT_TWITCHED))
    butEnv_setCursor(but->win->env, but, butCur_idle);
  if (newflags != but->flags)
    but_newFlags(but, newflags);
  return(BUTOUT_CAUGHT);
}


static ButOut  smpress(But *but, int butnum, int x, int y)  {
  ButOut  retval = BUTOUT_CAUGHT;
  int  newflags = but->flags;
  Menu  *m = but->iPacket;
  
  if (!(newflags & BUT_TWITCHED))
    retval &= ~BUTOUT_CAUGHT;
  else  {
    if (butnum == 1)  {
      newflags |= BUT_PRESSED;
      m->pressTime = but->win->env->eventTime;
    } else
      retval |= BUTOUT_ERR;
  }
  if (newflags != but->flags)
    but_newFlags(but, newflags);
  return(retval);
}


static void  sdraw(But *but, int x,int y, int w,int h)  {
  ButWin *win = but->win;
  Menu  *m = but->iPacket;
  uint  flags = but->flags;
  int  bwdiff, sbw, bbw;
  
  if (m->child == NULL)  {
    bbw = win->env->stdButBw;
    if (bbw == 0)
      bbw = 1;
    sbw = (bbw + 1) / 2;
    bwdiff = bbw - sbw;
    but_drawBox(win, but->x, but->y, but->w, but->h, 0, bwdiff,
		BUT_SLEFT|BUT_SRIGHT, BUT_LIT, BUT_SHAD, None, None);
    but_drawCt(win, flags, m->fgpic, m->bgpic, BUT_PBG,
	       but->x+bwdiff, but->y+bwdiff,
	       but->w-2*bwdiff, but->h/2 - bwdiff,
	       sbw, m->title, BUT_SLEFT|BUT_SRIGHT, 0);
    but_drawCt(win, flags, BUT_FG,BUT_BG,BUT_PBG,
	       but->x+bwdiff, but->y + but->h/2,
	       but->w-2*bwdiff, but->h - but->h/2 - bwdiff,
	       sbw, str_chars(&m->options[m->cval].text),
	       BUT_SLEFT|BUT_SRIGHT, 0);
  }
}


static ButOut  sdestroy(But *but)  {
  Menu  *m = but->iPacket;
  int  i;

  if (m->child != NULL)
    but_destroy(m->child);
  for (i = 0;  i <= m->nvals;  ++i)  {
    str_deinit(&m->options[i].text);
  }
  wms_free(m->options);
  wms_free(but->iPacket);
  return(0);
}


static void  snewflags(But *but, uint flags)  {
  Menu  *m = but->iPacket;
  bool  local;

  if (flags & (BUT_PRESSED|BUT_NETPRESS))  {
    if (m->child == NULL)  {
      local = flags & BUT_PRESSED;
      but->flags = flags & ~(BUT_PRESSED|BUT_NETPRESS);
      m->child = lcreate(but, local);
    }
  } else  {
    but->flags = flags;
  }
}


/*
 * If local is TRUE, then make the popup unpressable.
 */
static But  *lcreate(But *smenu, bool local)  {
  But  *but;
  Menu  *m = smenu->iPacket;
  int  sh, i, x,y, w,h;
  ButWin  *win = smenu->win;
  ButEnv  *env = win->env;
  int  butbw = env->stdButBw;

  h = (m->nvals - m->nbreaks) *
    (sh = (env->font0h + butbw * 2)) +
    butbw * 3 + smenu->h / 2 + m->nbreaks * ((butbw & ~1) + butbw);
  but = but_create(win, m, &laction);
  but->uPacket = smenu->uPacket;
  but->layer = m->llayer;
  but->x = smenu->x;
  but->y = smenu->y;
  if (m->up)
    but->y += smenu->h - h;
  but->w = smenu->w;
  but->h = h;
  but->flags = BUT_DRAWABLE | BUT_OPAQUE;
  if (local)
    but->flags |= BUT_PRESSABLE | BUT_PRESSED | BUT_LOCKED | BUT_TWITCHED;
  but->keys = NULL;

  x = but->x + env->stdButBw;
  y = but->y + m->parent->h/2 + 2*butbw;
  w = but->w - 2*env->stdButBw;
  h = env->font0h + env->stdButBw;
  for (i = 0;  i < m->nvals;  ++i)  {
    if (m->options[i].flags & BUTMENU_BREAK)  {
      y += (env->stdButBw & ~1) + env->stdButBw;
    } else  {
      m->options[i].x = x;
      m->options[i].y = y;
      m->options[i].w = w;
      m->options[i].h = h;
      y += env->font0h + env->stdButBw * 2;
    }
  }
  m->tval = m->cval;
  m->last_w = 0;
  m->clickOpen = FALSE;
  if (local)  {
    x = but->x + but->w/2;
    y = m->options[m->cval].y - butbw + sh/2;
    while (win->parent != NULL)  {
      x += win->xOff;
      y += win->yOff;
      win = win->parent;
    }
    XWarpPointer(env->dpy, None, win->win, 0,0,0,0, x, y);
  }
  m->skip_clicks = TRUE;
  but_init(but);
  snd_play(&butMenu_openSnd);
  return(but);
}


static ButOut  lmmove(But *but, int x, int y)  {
  ButOut  result = 0;
  int  msel, m_y, newflags = but->flags, otval;
  Menu  *m = but->iPacket;
  ButEnv  *env = but->win->env;
  int  butbw = env->stdButBw;
  bool  click = TRUE;
  StdInt32  netNewVal;

  if (m->skip_clicks)  {
    click = FALSE;
  }
  if (but->flags & BUT_PRESSED)
    result |= BUTOUT_CAUGHT;
  for (msel = 0;  msel < m->nvals;  ++msel)  {
    if (!(m->options[msel].flags & BUTMENU_BREAK))  {
      m_y = m->options[msel].y - butbw;
      if ((y > m_y + butbw / 2) &&
	  (y <= m_y + butbw / 2 + 2*butbw + env->font0h))
	break;
    }
  }
  if (msel < m->nvals)  {
    if (msel == m->cval)
      m->skip_clicks = FALSE;
    if (m->options[msel].flags & BUTMENU_DISABLED)
      msel = m->nvals;
  }
  if (msel == m->nvals)
    newflags &= ~BUT_TWITCHED;
  else
    newflags |= BUT_TWITCHED;
  if (msel != m->tval)  {
    if (m->parent->id >= 0)  {
      netNewVal = int_stdInt32(msel);
      butRnet_butSpecSend(m->parent, &netNewVal, sizeof(netNewVal));
    }
    otval = m->tval;
    m->tval = msel;
    if (((newflags ^ but->flags) & ~BUT_TWITCHED) == 0)  {
      /* Otherwise, it will be drawn by the setflags call. */
      if (otval != m->nvals)  {
	butWin_redraw(but->win, m->options[otval].x,m->options[otval].y-butbw,
		      m->options[otval].w,m->options[otval].h+butbw*2);
      }
      if (msel != m->nvals)  {
	butWin_redraw(but->win, m->options[msel].x,m->options[msel].y-butbw,
		      m->options[msel].w,m->options[msel].h+butbw*2);
      }
    }
    if (click)
      snd_play(&but_downSnd);
  }
  if (newflags != but->flags)
    but_newFlags(but, newflags);
  return(result);
}
    

static ButOut  lmleave(But *but)  {
  Menu  *m = but->iPacket;
  StdInt32  msg;
  
  if (m->parent->id >= 0)  {
    msg = int_stdInt32(-1);
    butRnet_butSpecSend(m->parent, &msg, sizeof(msg));
  }
  but_destroy(but);
  snd_play(&butMenu_closeSnd);
  return(BUTOUT_CAUGHT);
}


static ButOut  lmpress(But *but, int butnum, int x, int y)  {
  Menu  *m = but->iPacket;

  if (m->clickOpen)  {
    m->clickOpen = FALSE;
    snd_play(&but_upSnd);
    return(BUTOUT_CAUGHT);
  } else
    return(BUTOUT_CAUGHT | BUTOUT_ERR);
}


static ButOut  lmrelease(But *but, int butnum, int x, int y)  {
  int  result = BUTOUT_CAUGHT;
  Menu  *m = but->iPacket;
  StdInt32  msg;

  if (but->flags & BUT_PRESSABLE)  {
    if (m->pressTime + BUT_DCLICK > but->win->env->eventTime)  {
      /* Open this with a click. */
      m->clickOpen = TRUE;
      return(BUTOUT_CAUGHT);
    }
    if (m->parent->id >= 0)  {
      msg = int_stdInt32(-1);
      butRnet_butSpecSend(m->parent, &msg, sizeof(msg));
    }
  }
  but_destroy(but);
  if (m->tval == m->nvals)  {
    result |= BUTOUT_ERR;
  } else  {
    m->cval = m->tval;
    if (m->callback != NULL)
      result |= m->callback(m->parent, m->cval);
  }
  snd_play(&butMenu_closeSnd);
  return(result);
}


static void  ldraw(But *but, int x,int y, int w,int h)  {
  Menu  *m = but->iPacket;
  ButWin  *win = but->win;
  ButEnv  *env = win->env;
  But  *sbut = m->parent;
  int  tx, ty, tw, th, i;
  uint  butbw = env->stdButBw;
  int  new_y, old_y;
  
  if (butbw == 0)
    butbw = 1;

  /* If it resizes, you must recalculate how far into the button you should
   *   start drawing the options.  Start with last_w set to 0 to force
   *   this to happen on the first redraw.
   */
  if (m->last_w != but->w)  {
    m->last_w = but->w;
    m->xoff = (but->w - menu_width(but)) / 2;
  }		
  
  /* Draw the title. */
  tw = sbut->w;
  th = sbut->h / 2;  /* Don't forget: sbut is two boxes high! */
  if (y < m->options[0].y)  {
    but_drawCt(but->win, BUT_PRESSABLE, m->fgpic, m->bgpic, 0,
	       but->x, but->y, tw, th,
	       butbw, m->title, BUT_SLEFT|BUT_SRIGHT, 0);
  }
  /* The box around the options. */
  if ((y < m->options[0].y-butbw) ||
      (y+h > m->options[m->nvals-1].y + m->options[m->nvals-1].h+butbw) ||
      (x < m->options[0].x-butbw) ||
      (x > m->options[0].x + m->options[0].w+butbw))  {
    but_drawBox(win, but->x, but->y + th, but->w, but->h - th,
		0, (butbw+1)/2, BUT_SLEFT|BUT_SRIGHT, BUT_LIT, BUT_SHAD,
		None, None);
  }
  new_y = but->y + th + butbw;  /* Top of the current selection. */
  old_y = but->y + th + (butbw+1) / 2;
  /* Fill in the background for the options. */
  butEnv_setXFg(env, BUT_BG);
  XFillRectangle(env->dpy, win->win, env->gc,
		 but->x + butbw - win->xOff, new_y - win->yOff,
		 but->w - butbw * 2, but->y + but->h - new_y - butbw);
  for (i = 0;  i <= m->nvals;  ++i)  {
    if (m->options[i].flags & (BUTMENU_BREAK | BUTMENU_END))  {
      /* Draw the inner box around a group of options. */
      tx = but->x + (butbw+1)/2;
      ty = old_y;
      tw = but->w - ((butbw+1) & ~1);
      th = new_y - old_y + butbw + (butbw / 2);
      if ((ty < y+h) && (ty+th >= y) &&
	  ((ty+butbw/2 > y) || (ty+th-butbw/2 < y+h) ||
	   (tx+butbw/2 > x) || (tx+tw-butbw/2 < x+w)))
	but_drawBox(win, tx,ty, tw,th,
		    0, butbw / 2, BUT_SLEFT|BUT_SRIGHT, BUT_LIT, BUT_SHAD,
		    None, None);
      old_y = new_y + butbw + butbw/2;
      new_y += (butbw & ~1) + butbw;
    } else  {
      /* Draw the text for this option. */
      /* If it's selected, highlight behind it. */
      if (i == m->tval)  {
	if ((m->options[i].y-butbw <= y+h) &&
	    (m->options[i].y+m->options[i].h+butbw > y))  {
	  but_drawBox(win, but->x + butbw, new_y,
		      but->w - 2*butbw, env->font0h + 3*butbw,
		      0, butbw, BUT_SLEFT|BUT_SRIGHT, BUT_LIT, BUT_SHAD,
		      None, None);
	  butEnv_setXFg(env, BUT_HIBG);
	  XFillRectangle(env->dpy, win->win, env->gc,
			 but->x + 2*butbw - win->xOff,
			 new_y + butbw - win->yOff,
			 but->w - 4*butbw, env->font0h + butbw);
	}
      }
      if ((m->options[i].y <= y+h) &&
	  (m->options[i].y+m->options[i].h > y))  {
	if (m->options[i].flags & BUTMENU_DISABLED)  {
	  XSetFillStyle(env->dpy, env->gc, FillStippled);
	  XSetForeground(env->dpy, env->gc, env->colors[BUT_FG]);
	} else
	  butEnv_setXFg(env, BUT_FG);
	butWin_write(win, but->x + m->xoff, m->options[i].y + butbw/2,
		     str_chars(&m->options[i].text), 0);
	if (m->options[i].flags & BUTMENU_DISABLED)  {
	  butEnv_stdFill(env);
	}
      }
      new_y += env->font0h + butbw*2;
    }
  }
}


static ButOut  ldestroy(But *but)  {
  ButWin  *win = but->win;
  ButEnv  *env = win->env;
  Menu  *m = but->iPacket;
  But  *sbut = m->parent;
  int  x, y;
  int  minX, minY;

  minX = but->x;
  minY = but->y;
  x = sbut->x + sbut->w/2;
  y = sbut->y + sbut->h/2;
  while (win->parent != NULL)  {
    minX += win->xOff;
    x += win->xOff;
    minY += win->yOff;
    y += win->yOff;
    win = win->parent;
  }
  XWarpPointer(env->dpy, win->win, win->win,
	       minX, minY, but->w, but->h, x, y);
  m->child = NULL;
  return(0);
}


static void  lnewflags(But *but, uint flags)  {
  uint  ofl = but->flags;

  but->flags = flags;
  if (((flags ^ ofl) & ~BUT_TWITCHED) != 0)
    but_draw(but);
}


static int  menu_width(But *but)  {
  Menu  *m = but->iPacket;
  ButEnv  *env = but->win->env;
  int  i, maxw, tw;
  
  maxw = butEnv_textWidth(env, m->title, 0);
  for (i = 0;  !(m->options[i].flags & BUTMENU_END);  ++i)  {
    if (!(m->options[i].flags & BUTMENU_BREAK))  {
      tw = butEnv_textWidth(env, str_chars(&m->options[i].text), 0);
      if (tw > maxw)
	maxw = tw;
    }
  }
  return(maxw);
}


static ButOut  netMsg(But *but, void *msg, int msgLen)  {
  StdInt32  *i = msg;
  int  msgVal = stdInt32_int(*i);
  int  ctval, butbw = butEnv_stdBw(but->win->env);
  Menu  *m = but->iPacket;
  ButOut  result = 0;

  assert(msgLen == sizeof(StdInt32));
  if (msgVal < 0)
    result |= lmrelease(m->child, 0,0,0);
  else  {
    snd_play(&but_downSnd);
    ctval = m->tval;
    m->tval = msgVal;
    if (ctval != m->nvals)  {
      butWin_redraw(but->win, m->options[ctval].x,
		    m->options[ctval].y-butbw,
		    m->options[ctval].w,m->options[ctval].h+butbw*2);
    }
    if (msgVal != m->nvals)  {
      butWin_redraw(but->win, m->options[msgVal].x,
		    m->options[msgVal].y-butbw,
		    m->options[msgVal].w,
		    m->options[msgVal].h+butbw*2);
    }
  }
  return(result);
}


void  butMenu_setOptionName(But *but, const char *new, int entryNum)  {
  Menu  *m;

  assert(but->action == &saction);
  m = but->iPacket;
  assert(entryNum < m->nvals);
  str_copyChars(&m->options[entryNum].text, new);
  if (entryNum == m->cval)
    but_draw(but);
}


#endif
