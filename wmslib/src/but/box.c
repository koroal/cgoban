/*
 * wmslib/src/but/box.c, part of wmslib (Library functions)
 * Copyright (C) 1994-1997 William Shubert.
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
#include <but/but.h>
#include <but/box.h>
#include <but/ctext.h>


/**********************************************************************
 * Data Types
 **********************************************************************/
typedef struct  Box_struct  {
  Pixmap  ulmap, lrmap;
  int  ulcolor, lrcolor;
} Box;

typedef struct  BoxFilled_struct  {
  Pixmap  ulmap, lrmap, cmap;
  int  ulcolor, lrcolor, ccolor;
} BoxFilled;


/**********************************************************************
 * Foward Declarations
 **********************************************************************/
static void  box_draw(But *but, int x, int y, int w, int h);
static void  boxFilled_draw(But *but, int x, int y, int w, int h);
static ButOut  curve(int x, int y, int radius, int start, int sweep,
		     XPoint *pts);
static ButOut  box_destroy(But *but);
static ButOut  boxFilled_destroy(But *but);
static bool  boxFilled_resize(But *but, int oldX, int oldY,
			      int oldW, int oldH);
static ButOut  box_mmove(But *but, int x, int y);

#define  CIRCLE  64  /* Points to make a full circle. */


/**********************************************************************
 * Globals
 **********************************************************************/
static ButAction  box_action = {
  box_mmove, NULL, NULL, NULL, NULL, NULL,
  box_draw, box_destroy, but_flags, NULL, NULL};

static ButAction  boxFilled_action = {
  NULL, NULL, NULL, NULL, NULL, NULL,
  boxFilled_draw, boxFilled_destroy, but_flags, NULL, boxFilled_resize};


/**********************************************************************
 * Functions
 **********************************************************************/
But  *butBox_create(ButWin *win, int layer, int flags)  {
  But *but;
  Box *box;
  
  box = (Box *)wms_malloc(sizeof(Box));
  but = but_create(win, box, &box_action);
  but->layer = layer;
  but->flags = flags;
  box->ulcolor = BUT_LIT;
  box->lrcolor = BUT_SHAD;
  box->ulmap = None;
  box->lrmap = None;
  but_init(but);
  return(but);
}


void  butBox_setColors(But *but, int ul, int lr)  {
  Box  *box = but->iPacket;

  assert(but->action == &box_action);
  if (ul >= 0)  {
    box->ulcolor = ul;
    box->ulmap = None;
  }
  if (lr >= 0)  {
    box->lrcolor = lr;
    box->lrmap = None;
  }
  but_draw(but);
}


void  butBox_setPixmaps(But *but, Pixmap ul, Pixmap lr)  {
  Box  *box = but->iPacket;

  assert(but->action == &box_action);
  if (ul != None)
    box->ulmap = ul;
  if (lr != None)
    box->lrmap = lr;
  but_draw(but);
}


static void  box_draw(But *but, int x, int y, int w, int h)  {
  ButEnv  *env = but->win->env;
  int  butbw = env->stdButBw;
  Box  *box = but->iPacket;

  if ((x < but->x + butbw) ||
      (x+w > but->x + but->w - butbw) ||
      (y < but->y + butbw) ||
      (y+h > but->y + but->h - butbw))  {
    but_drawBox(but->win, but->x, but->y, but->w, but->h,0, butbw, 0,
		box->ulcolor, box->lrcolor, box->ulmap, box->lrmap);
  }
}


static ButOut  box_destroy(But *but)  {
  wms_free(but->iPacket);
  return(0);
}


void  but_drawBox(ButWin *win, int x, int y, int w, int h, int bstate,
		  int bw, int angles, int ulcolor, int lrcolor,
		  Pixmap ulmap, Pixmap lrmap)  {
  ButEnv  *env = win->env;
  Display  *dpy = env->dpy;
  XPoint  points[(CIRCLE/8+1)*4 + (CIRCLE/4+1)*2 + 1];
  int  abw = (bw * 1448 + 511) / 1024, h2 = h/2;
  int  rounding = env->font0h / 2;
  int  pnum = 0;
  int  xOff = win->xOff, yOff = win->yOff;
  bool  forceTiled;
     
  /* For arrows:
   *     a-----    h2  = c.y - a.y
   *    / b____    abw = d.x - c.x
   *   / /
   *  c_d
   *   \ \
   */
  if (angles & BUT_ALEFT)  {
    points[pnum  ].x = x + abw + h2 - bw - xOff;
    points[pnum++].y = y + bw - yOff;
    points[pnum  ].x = x + abw - xOff;
    points[pnum++].y = y + h2 - yOff;
    points[pnum  ].x = x - xOff;
    points[pnum++].y = y + h2 - yOff;
    points[pnum  ].x = x + h2 - xOff;
    points[pnum++].y = y - yOff;
  } else if (angles & BUT_RLEFT)  {
    pnum += curve(x+rounding-xOff, y+  rounding-yOff, rounding-bw,
		  90,90, points+pnum);
    pnum += curve(x+rounding-xOff, y+h-rounding-yOff, rounding-bw,
		  180,45, points+pnum);
    pnum += curve(x+rounding-xOff, y+h-rounding-yOff, rounding,
		  225,-45, points+pnum);
    pnum += curve(x+rounding-xOff, y+  rounding-yOff, rounding,
		  180,-90, points+pnum);
  } else  {  /* Square left edge. */
    points[pnum  ].x = x + bw - xOff;
    points[pnum++].y = y + bw - yOff;
    points[pnum  ].x = x + bw - xOff;
    points[pnum++].y = y + h - bw - yOff;
    points[pnum  ].x = x - xOff;
    points[pnum++].y = y + h - yOff;
    points[pnum  ].x = x - xOff;
    points[pnum++].y = y - yOff;
  }
  if (angles & BUT_ARIGHT)  {
    points[pnum  ].x = x + w - h2 - xOff;
    points[pnum++].y = y - yOff;
    points[pnum  ].x = x + w - xOff;
    points[pnum++].y = y + h2 - yOff;
    points[pnum  ].x = x + w - abw - xOff;
    points[pnum++].y = y + h2 - yOff;
    points[pnum  ].x = x + w - (abw + h2) + (abw-bw) - xOff;
    points[pnum++].y = y + bw - yOff;
  } else if (angles & BUT_RRIGHT)  {
    pnum += curve(x+w-rounding-xOff, y+rounding-yOff, rounding,
		  90,-45, points+pnum);
    pnum += curve(x+w-rounding-xOff, y+rounding-yOff, rounding-bw,
		  0,45, points+pnum);
  } else  {  /* Square right edge. */
    points[pnum  ].x = x + w - xOff;
    points[pnum++].y = y - yOff;
    points[pnum  ].x = x + w - bw - xOff;
    points[pnum++].y = y + bw - yOff;
  }

  points[pnum] = points[0];
  assert(pnum < sizeof(points) / sizeof(points[0]));
  forceTiled = FALSE;
  if (bstate == 0)  {
    if (ulmap == None)  {
      butEnv_setXFg(env, ulcolor);
    } else  {
      forceTiled = TRUE;
      XSetFillStyle(env->dpy, env->gc, FillTiled);
      XSetTile(env->dpy, env->gc, ulmap);
    }
  } else  {
    if (lrmap == None)  {
      butEnv_setXFg(env, lrcolor);
    } else  {
      forceTiled = TRUE;
      XSetFillStyle(env->dpy, env->gc, FillTiled);
      XSetTile(env->dpy, env->gc, lrmap);
    }
  }
  XFillPolygon(dpy, win->win, env->gc, points, pnum, Nonconvex,
	       CoordModeOrigin);
  
  h2 = (h + 1) / 2;
  pnum = 0;
  if (angles & BUT_ALEFT)  {
    points[pnum  ].x = x + abw + h2 - bw - xOff;
    points[pnum++].y = y + h - bw - yOff;
    points[pnum  ].x = x + abw - xOff;
    points[pnum++].y = y + h - h2 - yOff;
    points[pnum  ].x = x - xOff;
    points[pnum++].y = y + h - h2 - yOff;
    points[pnum  ].x = x + h2 - xOff;
    points[pnum++].y = y + h - yOff;
  } else if (angles & BUT_RLEFT)  {
    pnum += curve(x+rounding-xOff, y+h-rounding-yOff, rounding-bw,
		  270,-45, points+pnum);
    pnum += curve(x+rounding-xOff, y+h-rounding-yOff, rounding,
		  225,45, points+pnum);
  } else  {  /* Square left edge. */
    points[pnum  ].x = x + bw - xOff;
    points[pnum++].y = y + h - bw - yOff;
    points[pnum  ].x = x - xOff;
    points[pnum++].y = y + h - yOff;
  }
  if (angles & BUT_ARIGHT)  {
    points[pnum  ].x = x + w - h2 - xOff;
    points[pnum++].y = y + h - yOff;
    points[pnum  ].x = x + w - xOff;
    points[pnum++].y = y + h - h2 - yOff;
    points[pnum  ].x = x + w - abw - xOff;
    points[pnum++].y = y + h - h2 - yOff;
    points[pnum  ].x = x + w - (abw + h2) + (abw-bw) - xOff;
    points[pnum++].y = y + h - bw - yOff;
  } else if (angles & BUT_RRIGHT)  {
    pnum += curve(x+w-rounding-xOff, y+h-rounding-yOff, rounding,
		  270,90, points+pnum);
    pnum += curve(x+w-rounding-xOff, y+rounding-yOff, rounding,
		  0,45, points+pnum);
    pnum += curve(x+w-rounding-xOff, y+rounding-yOff, rounding-bw,
		  45,-45, points+pnum);
    pnum += curve(x+w-rounding-xOff, y+h-rounding-yOff, rounding-bw,
		  0,-90, points+pnum);
  } else  {  /* Square right edge. */
    points[pnum  ].x = x + w - xOff;
    points[pnum++].y = y + h - yOff;
    points[pnum  ].x = x + w - xOff;
    points[pnum++].y = y - yOff;
    points[pnum  ].x = x + w - bw - xOff;
    points[pnum++].y = y + bw - yOff;
    points[pnum  ].x = x + w - bw - xOff;
    points[pnum++].y = y + h - bw - yOff;
  }
  points[pnum] = points[0];
  assert(pnum < sizeof(points) / sizeof(points[0]));
  if (bstate == 0)  {
    if (lrmap == None)  {
      if (forceTiled)  {
	XSetFillStyle(env->dpy, env->gc, FillSolid);
	forceTiled = FALSE;
      }
      butEnv_setXFg(env, lrcolor);
    } else  {
      forceTiled = TRUE;
      XSetFillStyle(env->dpy, env->gc, FillTiled);
      XSetTile(env->dpy, env->gc, lrmap);
    }
  } else  {
    if (ulmap == None)  {
      if (forceTiled)  {
	XSetFillStyle(env->dpy, env->gc, FillSolid);
	forceTiled = FALSE;
      }
      butEnv_setXFg(env, ulcolor);
    } else  {
      if (!forceTiled)  {
	XSetFillStyle(env->dpy, env->gc, FillTiled);
	forceTiled = TRUE;
      }
      XSetTile(env->dpy, env->gc, ulmap);
    }
  }
  XFillPolygon(dpy, win->win, env->gc, points, pnum, Nonconvex,
	       CoordModeOrigin);
  if (forceTiled)
    butEnv_stdFill(env);
}


static ButOut  curve(int x, int y, int radius, int start, int sweep,
		     XPoint *pts)  {
  const int  circRadius = 2048;
  static const int  cosine[CIRCLE] = {
    2048, 2038, 2009, 1960,   1892, 1806, 1703, 1583,
    1448, 1299, 1138,  965,    784,  595,  400,  201,
    
    0, -201, -400, -595,   -784, -965,-1138,-1299,
    -1448,-1583,-1703,-1806,  -1892,-1960,-2009,-2038,

    -2048,-2038,-2009,-1960,  -1892,-1806,-1703,-1583,
    -1448,-1299,-1138, -965,   -784, -595, -400, -201,

    0,  201,  400,  595,    784,  965, 1138, 1299,
    1448, 1583, 1703, 1806,   1892, 1960, 2009, 2038};
  int  i, step, count = 0, limit, round, cv;
  
  start = (start * CIRCLE) / 360;
  sweep = (sweep * CIRCLE) / 360;
  if (sweep > 0)
    step = 1;
  else
    step = -1;
  limit = step*sweep;
  for (i = start, count = 0;  count <= limit;  i += step, ++count)  {
    cv = cosine[i & (CIRCLE-1)];
    if (cv < 0)
      round = -circRadius/2;
    else
      round = circRadius/2;
    pts[count].x = x + (cv * radius + round) / circRadius;
    cv = cosine[(CIRCLE/4 + i) & (CIRCLE-1)];
    if (cv < 0)
      round = -circRadius/2;
    else
      round = circRadius/2;
    pts[count].y = y + (cv * radius + round) / circRadius;
  }
  return(count);
}


void  but_drawCt(ButWin *win, int flags, int fgpic, int bgpic, int pbgpic,
		 int x, int y, int w, int h, int bw, const char *text,
		 int angles, int fontnum)  {
  ButEnv  *env = win->env;

  but_drawCtb(win, flags, fgpic, bgpic, pbgpic, x, y, w, h, bw, angles);
  if (!(flags & BUT_PRESSABLE))  {
    XSetFillStyle(env->dpy, env->gc, FillStippled);
    XSetForeground(env->dpy, env->gc, env->colors[fgpic]);
  } else
    butEnv_setXFg(env, fgpic);
  if ((flags & BUT_PRESSED) && (flags & BUT_TWITCHED))  {
    x += bw/2;
    y += bw/2;
  }
  butWin_write(win, x + (w - butEnv_textWidth(env, text, fontnum))/2,
	       y + (h - butEnv_fontH(env, fontnum)) / 2, text, fontnum);
  if (!(flags & BUT_PRESSABLE))  {
    butEnv_stdFill(env);
  }
}
  

void  but_drawCtb(ButWin *win, int flags, int fgpic, int bgpic, int pbgpic,
		  int x, int y, int w, int h, int bw, int angles)  {
  ButEnv  *env = win->env;
  
  but_drawBox(win, x, y, w, h,
	      flags & (BUT_PRESSED|BUT_NETPRESS), bw,
	      angles, BUT_LIT, BUT_SHAD, None, None);
  if ((flags & (BUT_PRESSED|BUT_NETPRESS)) &&
      (flags & (BUT_TWITCHED|BUT_NETTWITCH|BUT_KEYPRESSED|BUT_NETKEY)))
    bgpic = pbgpic;
  butEnv_setXFg(env, bgpic);
  x -= win->xOff;
  y -= win->yOff;
  if (angles)  {
    XPoint  points[CIRCLE + 5];
    int  npoints = 0, abw = (bw * 1448 + 511) / 1024, rnd = env->font0h / 2;
    
    if (angles & BUT_ALEFT)  {
      points[npoints  ].x = x + abw + h/2 - bw;
      points[npoints++].y = y + bw;
      points[npoints  ].x = x + abw;
      points[npoints++].y = y + h/2;
      points[npoints  ].x = x + abw + (h+1)/2 - bw;
      points[npoints++].y = y + h - bw;
    } else if (angles & BUT_RLEFT)  {
      npoints += curve(x+rnd,y+rnd, rnd-bw, 90,90, points+npoints);
      npoints += curve(x+rnd,y+h-rnd, rnd-bw, 180,90, points+npoints);
    } else  {
      points[npoints  ].x = x + bw;
      points[npoints++].y = y + bw;
      points[npoints  ].x = x + bw;
      points[npoints++].y = y + h - bw;
    }
    if (angles & BUT_ARIGHT)  {
      points[npoints  ].x = x + w - abw - (h+1)/2 + bw;
      points[npoints++].y = y + h - bw;
      points[npoints  ].x = x + w - abw;
      points[npoints++].y = y + h/2;
      points[npoints  ].x = x + w - abw - h/2 + bw;
      points[npoints++].y = y + bw;
    } else if (angles & BUT_RRIGHT)  {
      npoints += curve(x+w-rnd,y+h-rnd, rnd-bw, 270,90, points+npoints);
      npoints += curve(x+w-rnd,y+rnd, rnd-bw, 0,90, points+npoints);
    } else  {
      points[npoints  ].x = x + w - bw;
      points[npoints++].y = y + h - bw;
      points[npoints  ].x = x + w - bw;
      points[npoints++].y = y + bw;
    }
    points[npoints] = points[0];
    assert(npoints < CIRCLE+5);
    XFillPolygon(env->dpy, win->win, env->gc, points, npoints, Convex,
		 CoordModeOrigin);
  } else
    XFillRectangle(env->dpy, win->win, env->gc, x+bw, y+bw, w-2*bw, h-2*bw);
}


But  *butBoxFilled_create(ButWin *win, int layer, int flags)  {
  But  *but;
  BoxFilled  *box;
  
  box = (BoxFilled *)wms_malloc(sizeof(BoxFilled));
  but = but_create(win, box, &boxFilled_action);
  but->layer = layer;
  but->flags = flags | BUT_OPAQUE;
  box->ulcolor = BUT_LIT;
  box->lrcolor = BUT_SHAD;
  box->ccolor = BUT_BG;
  box->ulmap = None;
  box->lrmap = None;
  box->cmap = None;
  but_init(but);
  return(but);
}


void  butBoxFilled_setColors(But *but, int ul, int lr, int c)  {
  BoxFilled  *box = but->iPacket;

  assert(but->action == &boxFilled_action);
  if (ul >= 0)  {
    box->ulcolor = ul;
    box->ulmap = None;
  }
  if (lr >= 0)  {
    box->lrcolor = lr;
    box->lrmap = None;
  }
  if (c >= 0)  {
    box->ccolor = c;
    box->cmap = None;
  }
  but_draw(but);
}


void  butBoxFilled_setPixmaps(But *but, Pixmap ul, Pixmap lr, Pixmap c)  {
  BoxFilled  *box = but->iPacket;

  assert(but->action == &boxFilled_action);
  if (ul != None)
    box->ulmap = ul;
  if (lr != None)
    box->lrmap = lr;
  if (c != None)
    box->cmap = c;
  but_draw(but);
}


static void  boxFilled_draw(But *but, int x, int y, int w, int h)  {
  ButWin  *win = but->win;
  ButEnv  *env = win->env;
  int  butbw = env->stdButBw;
  BoxFilled  *box = but->iPacket;

  if ((x < but->x + butbw) ||
      (x+w > but->x + but->w - butbw) ||
      (y < but->y + butbw) ||
      (y+h > but->y + but->h - butbw))  {
    but_drawBox(but->win, but->x, but->y, but->w, but->h,0, butbw, 0,
		box->ulcolor, box->lrcolor, box->ulmap, box->lrmap);
  }
  if ((x >= but->x + but->w - butbw) || (y >= but->y + but->h - butbw) ||
      (x + w <= but->x + butbw) || (y+h <= but->y + butbw))
    return;
  if (x < but->x + butbw)  {
    w -= but->x + butbw - x;
    x = but->x + butbw;
  }
  if (y < but->y + butbw)  {
    h -= but->y + butbw - y;
    y = but->y + butbw;
  }
  if (x+w > but->x + but->w - butbw)
    w = but->x + but->w - butbw - x;
  if (y+h > but->y + but->h - butbw)
    h = but->y + but->h - butbw - y;
  if (box->cmap == None)  {
    butEnv_setXFg(env, box->ccolor);
    XFillRectangle(env->dpy, win->win, env->gc, x-win->xOff, y-win->yOff,
		   w,h);
  } else  {
    if (env->colorp)
      XSetFillStyle(env->dpy, env->gc, FillTiled);
    XSetTile(env->dpy, env->gc, box->cmap);
    XFillRectangle(env->dpy, but->win->win, env->gc,
		   x-win->xOff, y-win->yOff, w,h);
    if (env->colorp)
      XSetFillStyle(env->dpy, env->gc, FillSolid);
  }
}


static ButOut  boxFilled_destroy(But *but)  {
  wms_free(but->iPacket);
  return(0);
}


/*
 * This resize redraws only the needed parts of the screen.
 */
static bool  boxFilled_resize(But *but, int oldX, int oldY,
			      int oldW, int oldH)  {
  int  x, y, w, h, bw;

  bw = but->win->env->stdButBw;
  x = but->x;
  y = but->y;
  w = but->w;
  h = but->h;

  if ((oldW < 1) || (oldH < 1))  {
    butWin_redraw(but->win, x, y, w, h);
  } else if ((x + w < oldX) || (y + h < oldY) ||
	     (oldX + oldW < x) || (oldY + oldH < y))  {
    butWin_redraw(but->win, oldX, oldY, oldW, oldH);
    butWin_redraw(but->win, x, y, w, h);
  } else  {
    if (x < oldX)
      butWin_redraw(but->win, x, y, oldX + bw - x, h);
    else if (oldX < x)
      butWin_redraw(but->win, oldX, oldY, x + bw - oldX, oldH);
    
    if (y < oldY)
      butWin_redraw(but->win, x, y, w, oldY + bw - y);
    else if (oldY <  y)
      butWin_redraw(but->win, oldX, oldY, oldW, y + bw - oldY);
    
    if (x + w < oldX + oldW)
      butWin_redraw(but->win, x + w - bw, oldY,
		    oldX + oldW + bw - x - w, oldH);
    else if (oldX + oldW < x + w)
      butWin_redraw(but->win, oldX + oldW - bw, y,
		    x + w + bw - oldX - oldW, h);

    if (y + h < oldY + oldH)
      butWin_redraw(but->win, oldX, y + h - bw,
		    oldW, oldY + oldH + bw - y - h);
    else if (oldY + oldH < y + h)
      butWin_redraw(but->win, x, oldY + oldH - bw,
		    w, y + h + bw - oldY - oldH);
  }
  return(FALSE);
}

  
static ButOut  box_mmove(But *but, int x, int y)  {
  int  bw = butEnv_stdBw(but->win->env);

  if (((x >= but->x) && (x < but->x + bw)) ||
      ((y >= but->y) && (y < but->y + bw)) ||
      ((x >= but->x + but->w - bw) && (x < but->x + but->w)) ||
      ((y >= but->y + but->h - bw) && (y < but->y + but->h)))
    return(BUTOUT_CAUGHT);
  else
    return(0);
}


#endif
