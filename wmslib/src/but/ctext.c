/*
 * wmslib/src/but/ctext.c, part of wmslib (Library functions)
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
#include <but/net.h>
#include <but/ctext.h>
#include <but/box.h>


/**********************************************************************
 * Data Types
 **********************************************************************/
typedef struct Ct_struct  {
  char  *str;
  int  maxlen;
  int  angles;
  ButOut  (*pfunc)(But *but), (*rfunc)(But *but);
  bool  netAction;
  int  textX, textY, textW, textH;
} Ct;


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
static bool  mouse_over_but(But *but, int x, int y);
static void  flags(But *but, uint flags);
static ButOut  remPress(But *but, void *msg, int msgLen);


/**********************************************************************
 * Globals
 **********************************************************************/
static ButAction  action = {
  mmove, mleave, mpress, mrelease,
  kpress, krelease, draw, destroy, flags, remPress};


/**********************************************************************
 * Functions
 **********************************************************************/
But  *butCt_create(ButOut (*func)(But *but), void *packet, ButWin *win,
		   int layer, int flags, const char *text)  {
  return(butAct_vCreate(NULL, func, packet, win, layer, flags, text,
			 BUT_RLEFT|BUT_RRIGHT));
}


But  *butAct_create(ButOut (*func)(But *but), void *packet,
		    ButWin *win, int layer, int flags,
		    const char *text, int angleflags)  {
  return(butAct_vCreate(NULL, func, packet, win, layer, flags, text,
			angleflags));
}


But  *butAct_vCreate(ButOut (*pfunc)(But *but), ButOut (*rfunc)(But *but),
		     void *packet, ButWin *win, int layer, int flags,
		     const char *text, int angleflags)  {
  Ct  *ct;
  But  *but;

  ct = wms_malloc(sizeof(Ct));
  but = but_create(win, ct, &action);
  but->uPacket = packet;
  but->layer = layer;
  but->flags = flags;

  if (text == NULL)
    text = "";
  ct->rfunc = rfunc;
  ct->maxlen = strlen(text);
  ct->str = (char *)wms_malloc(sizeof(char) * (ct->maxlen + 1));
  strcpy(ct->str, text);
  ct->angles = angleflags;
  ct->pfunc = pfunc;
  ct->netAction = TRUE;
  ct->textW = ct->textH = 0;
  but_init(but);
  return(but);
}


void  butCt_setText(But *but, const char *text)  {
  Ct  *ct = but->iPacket;
  int  newlen;
  
  assert(but->action == &action);
  newlen = strlen(text);
  if (newlen > ct->maxlen)  {
    wms_free(ct->str);
    ct->maxlen = newlen;
    ct->str = (char *)wms_malloc((newlen + 1) * sizeof(char));
  }
  strcpy(ct->str, text);
  but_draw(but);
}


static ButOut  destroy(But *but)  {
  Ct *ct = but->iPacket;
  
  wms_free(ct->str);
  wms_free(ct);
  return(0);
}


void  butCt_setTextLoc(But *but, int x, int y, int w, int h)  {
  Ct  *ct = but->iPacket;

  ct->textX = x;
  ct->textY = y;
  ct->textW = w;
  ct->textH = h;
}


static void  draw(But *but, int x, int y, int w, int h)  {
  ButWin  *win = but->win;
  ButEnv  *env = win->env;
  Ct  *ct = but->iPacket;
  uint  flags = but->flags;
  int  tx, ty, tw, th;
  int  textWidth;

  but_drawCtb(win, flags, BUT_FG, BUT_BG, BUT_PBG,
	      but->x, but->y, but->w, but->h, env->stdButBw, ct->angles);
  tx = but->x;
  ty = but->y;
  if (ct->textW != 0)  {
    tx += ct->textX;
    ty += ct->textY;
    tw = ct->textW;
    th = ct->textH;
  } else  {
    tw = but->w;
    th = but->h;
  }
  if (!(but->flags & BUT_PRESSABLE))  {
    XSetFillStyle(env->dpy, env->gc, FillStippled);
    XSetForeground(env->dpy, env->gc, env->colors[BUT_FG]);
  } else
    butEnv_setXFg(env, BUT_FG);
  if ((but->flags & BUT_PRESSED) && (but->flags & BUT_TWITCHED))  {
    tx += env->stdButBw/2;
    ty += env->stdButBw/2;
  }
  textWidth = butEnv_textWidth(win->env, ct->str, 0);
  butWin_write(win, tx + (tw - textWidth) / 2,
	       ty + (th - butEnv_fontH(win->env, 0))/2,
	       ct->str, 0);
  if (!(but->flags & BUT_PRESSABLE))  {
    butEnv_stdFill(env);
  }
}


static void  flags(But *but, uint flags)  {
  uint  ofl = but->flags;

  but->flags = flags;
  if ((flags & BUT_NETPRESS) && !(ofl & BUT_NETPRESS))  {
    snd_play(&but_downSnd);
  }
  if (((flags & (BUT_PRESSABLE|BUT_DRAWABLE|BUT_PRESSED|BUT_NETPRESS)) !=
       (ofl   & (BUT_PRESSABLE|BUT_DRAWABLE|BUT_PRESSED|BUT_NETPRESS))) ||
      (((flags & (BUT_TWITCHED|BUT_NETTWITCH)) !=
	(ofl & (BUT_TWITCHED|BUT_NETTWITCH))) &&
       (flags & (BUT_PRESSED|BUT_NETPRESS))))
    but_draw(but);
}


static ButOut  mmove(But *but, int x, int y)  {
  ButOut  retval = BUTOUT_CAUGHT;
  uint  newflags = but->flags;
  
  if (mouse_over_but(but, x,y))  {
    if (!(but->flags & BUT_PRESSABLE))
      return(BUTOUT_CAUGHT);
    newflags |= BUT_TWITCHED;
  } else  {
    if (!(but->flags & BUT_PRESSABLE))
      return(0);
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
  uint  newflags = but->flags;
  Ct  *ct = but->iPacket;
  bool  pressPf = FALSE;
  
  if (!(newflags & BUT_TWITCHED))
    retval &= ~BUTOUT_CAUGHT;
  else  {
    if (butnum == 1)  {
      newflags |= BUT_PRESSED | BUT_LOCKED;
      pressPf = TRUE;
    } else
      return(BUTOUT_CAUGHT | BUTOUT_ERR);
  }
  if (!(but->flags & BUT_PRESSED) && (newflags & BUT_PRESSED))
    snd_play(&but_downSnd);
  if (newflags != but->flags)
    but_newFlags(but, newflags);
  if (pressPf && ct->pfunc)
    retval |= ct->pfunc(but);
  return(retval);
}


static ButOut  mrelease(But *but, int butnum, int x, int y)  {
  ButOut  retval = BUTOUT_CAUGHT;
  int  newflags = but->flags;
  Ct  *ct = but->iPacket;
  bool  pressRf = FALSE;
  
  if (butnum != 1)  {
    if (but->flags & BUT_TWITCHED)
      return(BUTOUT_CAUGHT);
    else
      return(0);
  }
  if (!(but->flags & BUT_PRESSED))
    return(0);
  if (but->flags & BUT_TWITCHED)  {
    pressRf = TRUE;
  } else  {
    if (ct->pfunc != NULL)
      pressRf = TRUE;
    retval |= BUTOUT_ERR;
  }
  newflags &= ~(BUT_PRESSED|BUT_LOCKED);
  if ((but->flags & BUT_PRESSED) && !(newflags & BUT_PRESSED) &&
      !(retval & BUTOUT_ERR))
    snd_play(&but_upSnd);
  if (newflags != but->flags)
    but_newFlags(but, newflags);
  if (but->id >= 0)
    butRnet_butSpecSend(but, NULL, 0);
  if (pressRf && ct->rfunc)
    retval |= ct->rfunc(but);
  return(retval);
}


static ButOut  kpress(But *but, const char *keystr, KeySym sym)  {
  ButOut  retval = BUTOUT_CAUGHT;
  int  newflags = but->flags;
  Ct  *ct = but->iPacket;
  
  newflags |= BUT_KEYPRESSED|BUT_PRESSED|BUT_LOCKED;
  if (!(but->flags & BUT_KEYPRESSED))  {
    snd_play(&but_downSnd);
    but_newFlags(but, newflags);
    if (ct->pfunc != NULL)
      retval |= ct->pfunc(but);
  }
  return(retval);
}


static ButOut  krelease(But *but, const char *keystr, KeySym sym)  {
  ButOut  retval = BUTOUT_CAUGHT;
  int  newflags = but->flags;
  Ct  *ct = but->iPacket;
  bool  pressRf = FALSE;
  
  if (!(newflags & BUT_KEYPRESSED))
    return(0);
  newflags &= ~(BUT_KEYPRESSED|BUT_PRESSED|BUT_LOCKED);
  if (keystr != NULL)  {
    /* If keystr is NULL, the key wasn't released - we lost focus instead. */
    pressRf = TRUE;
  }
  snd_play(&but_upSnd);
  if (newflags != but->flags)
    but_newFlags(but, newflags);
  if (but->id >= 0)
    butRnet_butSpecSend(but, NULL, 0);
  if (pressRf && ct->rfunc)
    retval |= ct->rfunc(but);
  return(retval);
}


static bool  mouse_over_but(But *but, int x, int y)  {
  int  angleflags = ((Ct *)(but->iPacket))->angles;
  int  radlimit, radius, dx, dy;

  if (x < but->x + but->w/2)  {
    if (angleflags & BUT_ALEFT)  {
      if ((y < but->y) || (y >= but->y + but->h))
	return(0);
      if (y < but->y + but->h/2) {
	if (x <= but->x + (but->y + but->h/2 - y)) {
	  return(0);
	}
      } else {
	return(x > but->x + (y - (but->y + but->h/2)));
      }
    } else if (angleflags & BUT_SLEFT) {
      return((x >= but->x) && (y >= but->y) && (y < but->y + but->h));
    }
  } else  {
    if (angleflags & BUT_ARIGHT)  {
      if ((y < but->y) || (y >= but->y + but->h)) {
	return(0);
      }
      if (y < but->y + but->h/2) {
	return(x < but->x + but->w - (but->y + but->h/2 - y));
      } else {
	return(x < but->x + but->w - (y - (but->y + but->h/2)));
      }
    } else if (angleflags & BUT_SRIGHT) {
      return((x < but->x + but->w) && (y >= but->y) && (y < but->y + but->h));
    }
  }
  /* We're dealing with the tough case...rounded edges! */
  radius = but->h / 4;
  radlimit = (radius + 1);
  radlimit *= radlimit;
  if (y < but->y + radius)  {
    if (x < but->x + radius)  {
      dx = but->x + radius - x;
      dy = but->y + radius - y;
      return(dx*dx + dy*dy < radlimit);
    } else if (x < but->x + but->w - radius)
      return(y >= but->y);
    else  {
      dx = x - (but->x + but->w - radius - 1);
      dy = but->y + radius - y;
      return(dx*dx + dy*dy < radlimit);
    }
  } else if (y < but->y + but->h - radius)
    return((x >= but->x) && (x < but->x + but->w));
  else  {
    if (x < but->x + radius)  {
      dx = but->x + radius - x;
      dy = y - (but->y + but->h - radius - 1);
      return(dx*dx + dy*dy < radlimit);
    } else if (x < but->x + but->w - radius)
      return(y < but->y + but->h);
    else  {
      dx = x - (but->x + but->w - radius - 1);
      dy = y - (but->y + but->h - radius - 1);
      return(dx*dx + dy*dy < radlimit);
    }
  }
}		


static ButOut  remPress(But *but, void *msg, int msgLen)  {
  Ct  *ct = but->iPacket;
  ButOut  res = 0;

  assert(msgLen == 0);
  snd_play(&but_upSnd);
  if ((ct->rfunc != NULL) && ct->netAction)
    res = ct->rfunc(but);
  return(res);
}

    
void  butCt_setNetAction(But *but, bool netAction)  {
  Ct  *ct = but->iPacket;

  ct->netAction = netAction;
}


#endif
