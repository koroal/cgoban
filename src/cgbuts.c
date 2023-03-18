/*
 * $Source: /cvsroot/cgoban1/cgoban1/src/cgbuts.c,v $
 * $Revision: 1.2 $
 * $Author: wmshub $
 * $Date: 2002/05/31 23:40:54 $
 *
 * src/cgbuts.c, part of Complete Goban (game program)
 * Copyright (C) 1995-1996 William Shubert.
 * See "configure.h.in" for more copyright information.
 *
 * This code extends the wmslib/but library to add special buttons needed
 *   for cgoban.
 *
 * The globe data was extracted from the CIA World Data Bank II map database.
 *   via XEarth.  See the main copyright for information about this.
 */

#include <math.h>
#include <wms.h>
#include <but/but.h>
#include <but/net.h>
#include <wms/rnd.h>
#include <wms/str.h>
#include "goBoard.h"
#include "drawStone.h"
#ifdef  _CGBUTS_H_
  Levelization Error.
#endif
#include "cgbuts.h"


/**********************************************************************
 * Forward Declarations
 **********************************************************************/
static ButOut  grid_mmove(But *but, int x, int y);
static ButOut  grid_mleave(But *but);
static ButOut  grid_mpress(But *but, int butnum, int x, int y);
static ButOut  grid_mrelease(But *but, int butnum, int x, int y);
static void  grid_draw(But *but, int x,int y,int w,int h);
static ButOut  grid_destroy(But *but);
static void  grid_newflags(But *but, uint fl);
static ButOut  gridNetPress(But *but, void *buf, int bufLen);
static void  drawRewChar(void *packet, ButWin *win,
			 int x, int y, int w, int h);
static void  drawBackChar(void *packet, ButWin *win,
			  int x, int y, int w, int h);
static void  drawFwdChar(void *packet, ButWin *win,
			 int x, int y, int w, int h);
static void  drawFfChar(void *packet, ButWin *win,
			int x, int y, int w, int h);
static void  drawWStoneChar(void *packet, ButWin *win,
			    int x, int y, int w, int h);
static void  drawBStoneChar(void *packet, ButWin *win,
			    int x, int y, int w, int h);
static void  grid_morePixmaps(Cgbuts *b, int newNumPixmaps);
static void  gobanPic_draw(But *but, int x,int y, int w,int h);
static ButOut  gobanPic_destroy(But *but);
static void  stoneGroup_draw(But *but, int x,int y, int w,int h);
static ButOut  stoneGroup_destroy(But *but);
static void  computerPic_draw(But *but, int x,int y, int w,int h);
static ButOut  computerPic_destroy(But *but);
static void  toolPic_draw(But *but, int x,int y, int w,int h);
static ButOut  toolPic_destroy(But *but);


/**********************************************************************
 * Global Variables
 **********************************************************************/
const char  *cgbuts_stoneChar[2] = {CGBUTS_WSTONECHAR, CGBUTS_BSTONECHAR};

static const ButAction  grid_action = {
  grid_mmove, grid_mleave, grid_mpress, grid_mrelease,
  NULL, NULL, grid_draw, grid_destroy, grid_newflags, gridNetPress};

static const ButAction  gobanPic_action = {
  NULL, NULL, NULL, NULL,
  NULL, NULL, gobanPic_draw, gobanPic_destroy, but_flags, NULL};

static const ButAction  stoneGroup_action = {
  NULL, NULL, NULL, NULL,
  NULL, NULL, stoneGroup_draw, stoneGroup_destroy, but_flags, NULL};

static const ButAction  computerPic_action = {
  NULL, NULL, NULL, NULL,
  NULL, NULL, computerPic_draw, computerPic_destroy, but_flags, NULL};

static const ButAction  toolPic_action = {
  NULL, NULL, NULL, NULL,
  NULL, NULL, toolPic_draw, toolPic_destroy, but_flags, NULL};


/**********************************************************************
 * Functions
 **********************************************************************/
void  cgbuts_init(Cgbuts *cgbuts, ButEnv *env, Rnd *rnd, int color,
		  bool  hold, bool hiContrast, int timeWarn)  {
  MAGIC_SET(cgbuts);

  cgbuts->env = env;
  cgbuts->dpyDepth = DefaultDepth(butEnv_dpy(env),
				  DefaultScreen(butEnv_dpy(env)));
  cgbuts->dpyRootWindow = RootWindow(butEnv_dpy(env),
				     DefaultScreen(butEnv_dpy(env)));
  cgbuts->pics = NULL;
  cgbuts->numPics = 0;
  cgbuts->rnd = rnd;
  cgbuts->color = color;
  cgbuts->holdStart = 0;
  cgbuts->hiContrast = hiContrast;
  cgbuts->holdEnabled = hold;
  cgbuts->timeWarn = timeWarn;

  butEnv_setChar(env, 1.0, CGBUTS_REWCHAR, drawRewChar, NULL);
  butEnv_setChar(env, 0.5, CGBUTS_BACKCHAR, drawBackChar, NULL);
  butEnv_setChar(env, 0.5, CGBUTS_FWDCHAR, drawFwdChar, NULL);
  butEnv_setChar(env, 1.0, CGBUTS_FFCHAR, drawFfChar, NULL);
  butEnv_setChar(env, 1.0, CGBUTS_WSTONECHAR, drawWStoneChar, cgbuts);
  butEnv_setChar(env, 1.0, CGBUTS_BSTONECHAR, drawBStoneChar, cgbuts);
}


But  *grid_create(Cgbuts *b, ButOut (*func)(But *but), void *packet,
		  ButWin *win, int layer, int flags, int pos)  {
  Grid  *grid;
  But  *but;

  grid = wms_malloc(sizeof(Grid));
  MAGIC_SET(grid);
  but = but_create(win, grid, &grid_action);
  but->uPacket = packet;
  but->layer = layer;
  but->flags = flags;

  grid->b = b;
  grid->callback = func;
  grid->lineGroup = gridLines_center;
  grid->starPoint = FALSE;
  grid->pos = pos;
  grid->pressColor = goStone_black;
  grid->stoneVersion = rnd_int(b->rnd) % CGBUTS_NUMWHITE;
  grid->ptype = goStone_empty;
  grid->grey = FALSE;
  grid->holdRequired = FALSE;
  grid->markType = goMark_none;
  grid->markAux = 0;
  but_init(but);
  return(but);
}


void  grid_setStone(But *but, GoStone piece, bool grey)  {
  Grid  *grid = but->iPacket;

  assert(but->action == &grid_action);
  if ((piece != grid->ptype) || (grey != grid->grey))  {
    grid->ptype = piece;
    grid->grey = grey;
    but_draw(but);
  }
}


void  grid_setMark(But *but, GoMarkType mark, int aux)  {
  Grid  *grid = but->iPacket;

  assert(but->action == &grid_action);
  if ((grid->markType != mark) || (grid->markAux != aux))  {
    grid->markType = mark;
    grid->markAux = aux;
    but_draw(but);
  }
}


static ButOut  grid_destroy(But *but)  {
  Grid  *grid = but->iPacket;

  wms_free(grid);
  return(0);
}


static void  grid_newflags(But *but, uint fl)  {
  uint  ofl = but->flags;

  but->flags = fl;
  if ((ofl & (BUT_TWITCHED|BUT_PRESSED|BUT_NETTWITCH|BUT_NETPRESS)) !=
      (fl & (BUT_TWITCHED|BUT_PRESSED|BUT_NETTWITCH|BUT_NETPRESS)))  {
    but_draw(but);
  }
}


static ButOut  grid_mmove(But *but, int x, int y)  {
  uint  newflags = but->flags, retval = BUTOUT_CAUGHT;
  Grid  *grid;

  if (!(but->flags & BUT_PRESSABLE))
    return(BUTOUT_CAUGHT);
  if ((x < but->x) || (y < but->y) ||
      (x >= but->x + but->w) || (y >= but->y + but->h))  {
    newflags &= ~BUT_TWITCHED;
    if (newflags != but->flags)  {
      butEnv_setCursor(but->win->env, but, butCur_idle);
      but_newFlags(but, newflags);
    }
    if (!(newflags & BUT_LOCKED))
      retval = 0;
  } else  {
    newflags |= BUT_TWITCHED;
    if (newflags != but->flags)  {
      grid = but->iPacket;
      assert(MAGIC(grid));
      grid->b->holdStart = butWin_env(but_win(but))->eventTime;
      butEnv_setCursor(but->win->env, but, butCur_twitch);
      but_newFlags(but, newflags);
    }
  }
  return(retval);
}
      

static ButOut  grid_mleave(But *but)  {
  int  newflags = but->flags;

  newflags &= ~BUT_TWITCHED;
  if (newflags != but->flags)  {
    butEnv_setCursor(but->win->env, but, butCur_idle);
    but_newFlags(but, newflags);
  }
  return(BUTOUT_CAUGHT);
}
      

static ButOut  grid_mpress(But *but, int butnum, int x, int y)  {
  int  newflags = but->flags, retval = BUTOUT_CAUGHT;
  Grid  *grid;

  if (butnum == 1)  {
    grid = but->iPacket;
    assert(MAGIC(grid));
    /*
     * Check to make sure that the user has been holding the mouse there
     *   long enough.
     * Holding the mouse is only necessare if all of these are true:
     *   - Cgbuts.holdEnabled is TRUE.  This lets the user turn on and off
     *     all holds globally.
     *   - This particular button has hold enabled.  This lets cgoban turn
     *     off hold when you are in a situation where an undo is no problem.
     *   - No modifiers are pressed.  Same reason as the previous.
     */
    if (((but->win->env->keyModifiers & (ShiftMask | ControlMask)) == 0) &&
	grid->holdRequired && grid->b->holdEnabled &&
	(butWin_env(but_win(but))->eventTime - grid->b->holdStart <
	 CGBUTS_HOLDTIME))
      retval |= BUTOUT_ERR;
    else
      newflags |= BUT_PRESSED|BUT_LOCKED;
  } else
    retval |= BUTOUT_ERR;
  /*
   * if (!(but->flags & BUT_PRESSED) && (newflags & BUT_PRESSED))
   *   snd_play(&pe_move_snd);
   */
  if (newflags != but->flags)
    but_newFlags(but, newflags);
  return(retval);
}
      

static ButOut  grid_mrelease(But *but, int butnum, int x, int y)  {
  int  newflags = but->flags, retval = BUTOUT_CAUGHT;
  Grid  *grid = but->iPacket;
  bool  makeCallback = FALSE;

  if (butnum != 1)
    return(retval);
  if (but->flags & BUT_TWITCHED)  {
    if (but->flags & BUT_PRESSED)  {
      if (but->id)
	butRnet_butSpecSend(but, NULL, 0);
      makeCallback = TRUE;
    }
  } else
    retval |= BUTOUT_ERR;
  newflags &= ~(BUT_PRESSED|BUT_LOCKED);
  if (newflags != but->flags)
    but_newFlags(but, newflags);
  if (makeCallback)
    retval |= grid->callback(but);
  return(retval);
}
      

static void  grid_draw(But *but, int x,int y,int w,int h)  {
  static const struct  {
    int  numPoints, points[5];
  } lineFormats[9] = {
    {3, {4,2,3}},     /* ul */
    {4, {1,3,2,4}},   /* top */
    {3, {1,2,4}},     /* ur */
    {4, {0,4,2,3}},   /* left */
    {5, {1,3,2,0,4}}, /* center */
    {4, {0,4,2,1}},   /* right */
    {3, {0,2,3}},     /* ll */
    {4, {1,3,2,0}},   /* bottom */
    {3, {0,2,1}}};    /* lr */
  Grid  *grid = but->iPacket;
  int  i, lineWidth, starSize, flags = but->flags;
  Display  *dpy;
  XPoint  points[5], lines[5];
  GC  gc = but->win->env->gc;
  GoStone  markColor;
  Cgbuts  *b = grid->b;
  
  assert(MAGIC(b));
  dpy = but->win->env->dpy;
  /* Points are:
   *     0
   *     |
   * 1 - 2 - 3
   *     |
   *     4
   */
  if ((grid->markType != goMark_letter) && (grid->markType != goMark_number) &&
      (grid->lineGroup != gridLines_none) &&
      ((grid->ptype == goStone_empty) || grid->grey))  {
    butEnv_setXFg(but->win->env, BUT_FG);
    lineWidth = (but->w + 15) / 30;
    if (lineWidth < 1)
      lineWidth = 1;
    XSetLineAttributes(dpy, gc, lineWidth, LineSolid, CapButt, JoinMiter);
    points[0].x = points[2].x = points[4].x = but->x + but->w/2;
    points[1].x = but->x;
    points[3].x = but->x + but->w;
    points[0].y = but->y;
    points[1].y = points[2].y = points[3].y = but->y + but->h/2;
    points[4].y = but->y + but->h;
    for (i = 0;  i < lineFormats[grid->lineGroup].numPoints;  ++i)
      lines[i] = points[lineFormats[grid->lineGroup].points[i]];
    XDrawLines(dpy, but->win->win, gc, lines,
	       lineFormats[grid->lineGroup].numPoints, CoordModeOrigin);
    if (grid->starPoint)  {
      butEnv_setXFg(but->win->env, BUT_FG);
      starSize = (((but->w + 6 - lineWidth*6) / 12) * 2) + lineWidth+1;
      if (starSize > 1)  {
	XFillArc(dpy, but->win->win, gc, but->x+(but->w - starSize)/2,
		 but->y + (but->h-starSize)/2, starSize, starSize,
		 0, 360*64);
      }
    }
  }
  if ((grid->ptype == goStone_empty) &&
      !(flags & (BUT_TWITCHED|BUT_NETTWITCH)) &&
      (grid->markType == goMark_none))
    return;
  markColor = grid->ptype;
  if (grid->ptype != goStone_empty)  {
    if (flags & (BUT_PRESSED|BUT_NETPRESS))  {
      /* Pressed.  Invert the greyness. */
      cgbuts_drawp(b, but->win, grid->ptype, !grid->grey, but->x, but->y,
		   but->w, grid->stoneVersion, x,y,w,h);
    } else  {
      cgbuts_drawp(b, but->win, grid->ptype, grid->grey, but->x, but->y,
		   but->w, grid->stoneVersion, x,y,w,h);
      if (flags & (BUT_TWITCHED|BUT_NETTWITCH))  {
	/* Twitched.  Draw a small stone in the middle of it. */
	if (grid->grey)  {
	  cgbuts_drawp(b, but->win, grid->ptype, FALSE,
		       but->x+(but->w+1)/4, but->y+(but->w+1)/4,
		       but->w/2, grid->stoneVersion, x,y,w,h);
	} else  {
	  cgbuts_drawp(b, but->win, goStone_opponent(grid->ptype), TRUE,
		       but->x+(but->w+1)/4, but->y+(but->w+1)/4,
		       but->w/2, grid->stoneVersion, x,y,w,h);
	}
      }
    }
  } else  {
    assert(grid->ptype == goStone_empty);
    if (flags & (BUT_PRESSED|BUT_NETPRESS|BUT_TWITCHED|BUT_NETTWITCH))  {
      markColor = grid->pressColor;
      if (flags & (BUT_PRESSED |BUT_NETPRESS))  {
	cgbuts_drawp(b, but->win, grid->pressColor, FALSE,
		     but->x, but->y, but->w, grid->stoneVersion, x,y,w,h);
      } else  {
	/* Grid is twitched only. */
	cgbuts_drawp(b, but->win, grid->pressColor, TRUE,
		     but->x, but->y, but->w, grid->stoneVersion, x,y,w,h);
      }
    }
  }
  if ((grid->markType != goMark_none) &&
      (!(flags & (BUT_PRESSED|BUT_NETPRESS))))  {
    cgbuts_markPiece(b, but->win, markColor, grid->markType, grid->markAux,
		     but->x, but->y, but->w, grid->stoneVersion, x,y,w,h);
  }
}


static ButOut  gridNetPress(But *but, void *buf, int bufLen)  {
  Grid  *grid = but->iPacket;

  assert(bufLen == 0);
  /* snd_play(&pe_move_snd); */
  return(grid->callback(but));
}


void  cgbuts_drawp(Cgbuts *b, ButWin *win, GoStone piece, bool grey,
		   int x, int y, int size, int stoneVersion,
		   int dx,int dy, int dw,int dh)  {
  ButEnv  *env = win->env;
  Display  *dpy = env->dpy;
  GC  gc = env->gc2;
  int  copyStart;
  int  worldNum = 0;

  if (size >= b->numPics)
    grid_morePixmaps(b, size + 1);
  if (b->pics[size].stonePixmaps == None)
    drawStone_newPics(env, b->rnd, CGBUTS_GREY(0),
		      &b->pics[size].stonePixmaps,
		      &b->pics[size].maskBitmaps, size,
		      butEnv_isColor(env) && !b->hiContrast);
  if (stoneVersion >= CGBUTS_NUMWHITE)  {
    worldNum = 2 + stoneVersion / CGBUTS_NUMWHITE;
    stoneVersion %= CGBUTS_NUMWHITE;
  }
  
  if ((dx >= x+size) || (dy >= y+size) ||
      (dx+dw <= x) || (dy+dh <= y))
    return;
  if (dx < x)  {
    dw -= x - dx;
    dx = x;
  }
  if (dy < y)  {
    dh -= y - dy;
    dy = y;
  }
  if (dx + dw > x + size)
    dw = x + size - dx;
  if (dy + dh > y + size)
    dh = y + size - dy;
  XSetClipMask(dpy, gc, b->pics[size].maskBitmaps);
  if (grey)  {
    if ((x & 1) == (y & 1))
      XSetClipOrigin(dpy, gc, x - size - win->xOff, y - win->yOff);
    else
      XSetClipOrigin(dpy, gc, x - size * 2 - win->xOff, y - win->yOff);
  } else  {
    XSetClipOrigin(dpy, gc, x - win->xOff, y - win->yOff);
  }
  copyStart = 0;
  if (piece == goStone_white)
    copyStart = (stoneVersion + 1) * size;
  XCopyArea(dpy, b->pics[size].stonePixmaps, win->win, gc,
	    copyStart+dx-x,dy-y, dw,dh, dx - win->xOff, dy - win->yOff);
  if (worldNum)  {
    copyStart = 0;
    if (piece == goStone_black)
      copyStart = (stoneVersion + 1) * size;
    XSetClipOrigin(dpy, gc, x-size*worldNum - win->xOff, y - win->yOff);
    XCopyArea(dpy, b->pics[size].stonePixmaps, win->win, gc,
	      copyStart+dx-x,dy-y, dw,dh, dx - win->xOff, dy - win->yOff);
  }
  XSetClipMask(dpy, gc, None);
}


void  cgbuts_markPiece(Cgbuts *b, ButWin *win, GoStone piece,
		       GoMarkType markType, int markAux,
		       int x, int y, int w, int stoneVersion,
		       int dx,int dy, int dw,int dh)  {
  ButEnv *env = win->env;
  Display *dpy = env->dpy;
  GC  gc = env->gc;
  int  lw = 0;
  XPoint  points[4];
  int  off;
  char  text[4];

  if ((markType >= goMark_lastMove) && (markType <= goMark_circle))  {
    /* Must set up line attributes. */
    lw = (w + 7) / 15;
    if (lw < 1)
      lw = 1;
    XSetLineAttributes(dpy, gc, lw, LineSolid, CapButt, JoinMiter);
  }
  if (piece == goStone_black)
    butEnv_setXFg(env, BUT_WHITE);
  else
    butEnv_setXFg(env, BUT_BLACK);
  switch(markType)  {
  case goMark_none:
    break;
  case goMark_triangle:
    points[0].x = x+w/2 - win->xOff;
    points[0].y = y+lw - win->yOff;
    points[1].x = x+((w*0.9330127)-(lw*0.8660254)+0.5) - win->xOff;
    points[1].y = y+((w*0.75)-(lw*0.5)+0.5) - win->yOff;
    points[2].x = x+((w*0.0669873)+(lw*0.8660254)+0.5) - win->xOff;
    points[2].y = points[1].y;
    points[3] = points[0];
    XDrawLines(dpy, win->win, gc, points, 4, CoordModeOrigin);
    break;
  case goMark_x:
    off = (int)(w*0.14644661+lw*0.5+1.5);
    XDrawLine(dpy, win->win, gc,
	      x + off - win->xOff, y + off - win->yOff,
	      x + w - off - win->xOff-(lw&1), y + w - off - win->yOff-(lw&1));
    XDrawLine(dpy, win->win, gc,
	      x + off - win->xOff,  y + w - off  - win->yOff-(lw&1),
	      x + w - off - win->xOff-(lw&1), y + off - win->yOff);
    break;
  case goMark_ko:
  case goMark_square:
    off = (int)(w*0.14644661+lw*0.5+0.5);
    XDrawRectangle(dpy, win->win, gc, x + off - win->xOff, y + off - win->yOff,
		   w-off*2-(lw&1),w-off*2-(lw&1));
    break;
  case goMark_lastMove:
  case goMark_circle:
    off = (w + 1) / 4;
    XDrawArc(dpy, win->win, gc, x + off - win->xOff, y + off - win->yOff,
	     w-off*2-1,w-off*2-1, 0,360*64);
    break;
  case goMark_smBlack:
  case goMark_smWhite:
    if (markType == goMark_smBlack)
      cgbuts_drawp(b, win, goStone_black, FALSE, x+(w+1)/4,y+(w+1)/4, w/2,
		   stoneVersion, dx,dy,dw,dh);
    else
      cgbuts_drawp(b, win, goStone_white, FALSE, x+(w+1)/4,y+(w+1)/4, w/2,
		   stoneVersion, dx,dy,dw,dh);
    break;
  case goMark_letter:
    text[0] = markAux;
    text[1] = '\0';
    butWin_write(win, x + (w - butEnv_textWidth(env, text, 1)) / 2,
		 y + (w - butEnv_fontH(env, 1)) / 2, text, 1);
    break;
  case goMark_number:
    assert(markAux >= 0);
    if (markAux > 999)
      markAux = 999;
    if ((markAux > 99) && (w < butEnv_fontH(env, 0) * 2))
      sprintf(text, "%02d", markAux % 100);
    else
      sprintf(text, "%d", markAux);
    butWin_write(win, x + (w - butEnv_textWidth(env, text, 1)) / 2,
		 y + (w - butEnv_fontH(env, 1)) / 2, text, 1);
    break;
  }
}


void  grid_setPress(But *but, GoStone pressColor)  {
  Grid  *grid;

  assert(but->action == &grid_action);
  if (pressColor == goStone_empty)  {
    if (but->flags & BUT_PRESSABLE)
      but_setFlags(but, BUT_NOPRESS);
  } else  {
    if (!(but->flags & BUT_PRESSABLE))
      but_setFlags(but, BUT_PRESSABLE);
    grid = but->iPacket;
    grid->pressColor = pressColor;
  }
  if (but->flags & BUT_TWITCHED)
    but_draw(but);
}


static void  drawRewChar(void *packet, ButWin *win,
			 int x, int y, int w, int h)  {
  XPoint  points[8];
  Display  *dpy = butEnv_dpy(butWin_env(win));
  Window  xwin = butWin_xwin(win);
  GC  gc = butEnv_gc(butWin_env(win));

  h &= ~1;
  w &= ~1;
  points[0].x = x;
  points[0].y = y+h/2;
  points[1].x = x+w/2;
  points[1].y = y;
  points[2].x = x+w/2;
  points[2].y = y+h/2;
  points[3].x = x+w;
  points[3].y = y;
  points[4].x = x+w;
  points[4].y = y+h;
  points[5] = points[2];
  points[6].x = x+w/2;
  points[6].y = y+h;
  points[7] = points[0];
  XFillPolygon(dpy, xwin, gc, points, 8, Nonconvex, CoordModeOrigin);
}


static void  drawBackChar(void *packet, ButWin *win,
			  int x, int y, int w, int h)  {
  XPoint  points[4];
  Display  *dpy = butEnv_dpy(butWin_env(win));
  Window  xwin = butWin_xwin(win);
  GC  gc = butEnv_gc(butWin_env(win));

  h &= ~1;
  points[0].x = x;
  points[0].y = y+h/2;
  points[1].x = x+w;
  points[1].y = y;
  points[2].x = x+w;
  points[2].y = y+h;
  points[3] = points[0];
  XFillPolygon(dpy, xwin, gc, points, 4, Nonconvex, CoordModeOrigin);
}


static void  drawFwdChar(void *packet, ButWin *win,
			 int x, int y, int w, int h)  {
  XPoint  points[4];
  Display  *dpy = butEnv_dpy(butWin_env(win));
  Window  xwin = butWin_xwin(win);
  GC  gc = butEnv_gc(butWin_env(win));

  h &= ~1;
  points[0].x = x;
  points[0].y = y;
  points[1].x = x+w;
  points[1].y = y+h/2;
  points[2].x = x;
  points[2].y = y+h;
  points[3] = points[0];
  XFillPolygon(dpy, xwin, gc, points, 4, Nonconvex, CoordModeOrigin);
}


static void  drawFfChar(void *packet, ButWin *win,
			int x, int y, int w, int h)  {
  XPoint  points[8];
  Display  *dpy = butEnv_dpy(butWin_env(win));
  Window  xwin = butWin_xwin(win);
  GC  gc = butEnv_gc(butWin_env(win));

  h &= ~1;
  w &= ~1;
  points[0].x = x;
  points[0].y = y;
  points[1].x = x+w/2;
  points[1].y = y+h/2;
  points[2].x = x+w/2;
  points[2].y = y;
  points[3].x = x+w;
  points[3].y = y+h/2;
  points[4].x = x+w/2;
  points[4].y = y+h;
  points[5] = points[1];
  points[6].x = x;
  points[6].y = y+h;
  points[7] = points[0];
  XFillPolygon(dpy, xwin, gc, points, 8, Nonconvex, CoordModeOrigin);
}


static void  drawWStoneChar(void *packet, ButWin *win,
			    int x, int y, int w, int h)  {
  Cgbuts  *b = packet;

  cgbuts_drawp(b, win, goStone_white, FALSE, x,y, w, 1, x,y,w,h);
}


static void  drawBStoneChar(void *packet, ButWin *win,
			    int x, int y, int w, int h)  {
  Cgbuts  *b = packet;

  cgbuts_drawp(b, win, goStone_black, FALSE, x,y, w, 1, x,y,w,h);
}


static void  grid_morePixmaps(Cgbuts *b, int newNumPics)  {
  int  i;
  CgbutsPic  *newPics;

  assert(MAGIC(b));
  newPics = wms_malloc(newNumPics * sizeof(CgbutsPic));
  for (i = 0;  i < b->numPics;  ++i)  {
    newPics[i] = b->pics[i];
  }
  for (;  i < newNumPics;  ++i)  {
    newPics[i].stonePixmaps = None;
    newPics[i].maskBitmaps = None;
  }
  b->numPics = newNumPics;
  if (b->pics)  {
    wms_free(b->pics);
  }
  b->pics = newPics;
}

    
But  *gobanPic_create(Cgbuts *b, ButWin *win, int layer, int flags)  {
  But  *but;

  but = but_create(win, b, &gobanPic_action);
  but->uPacket = NULL;
  but->layer = layer;
  but->flags = flags;
  return(but);
}


static void  gobanPic_draw(But *but, int x,int y, int w,int h)  {
  ButWin  *win = but->win;
  ButEnv  *env = win->env;
  int  w13 = (but->w+1)/3;
  int  w16 = (but->w + 2) / 6;
  int  stonew = but->w/3;
  int  bw = butEnv_stdBw(env);
  int  cgtop = but->y + (but->w - stonew - bw - (w13 + w16)) / 2;
  int  bdtop = cgtop + stonew + bw;
  XPoint  points[4];

  butEnv_setXFg(env, CGBUTS_COLORBOARD(0));
  XFillRectangle(env->dpy, win->win, env->gc, but->x,bdtop+w13,
		 but->w,w16);
  butEnv_setXFg(env, CGBUTS_COLORBOARD(225));
  points[0].x = but->x;
  points[0].y = bdtop+w13;
  points[1].x = but->x + w16;
  points[1].y = bdtop;
  points[2].x = but->x + but->w - w16;
  points[2].y = points[1].y;
  points[3].x = but->x + but->w;
  points[3].y = points[0].y;
  XFillPolygon(env->dpy, win->win, env->gc, points, 4,
	       Convex, CoordModeOrigin);
}


static ButOut  gobanPic_destroy(But *but)  {
  return(0);
}


But  *stoneGroup_create(Cgbuts *b, ButWin *win, int layer, int flags)  {
  But  *but;

  but = but_create(win, b, &stoneGroup_action);
  but->uPacket = NULL;
  but->layer = layer;
  but->flags = flags;
  return(but);
}


static void  stoneGroup_draw(But *but, int x,int y, int w,int h)  {
  ButWin  *win = but->win;
  int  stoneW;

  stoneW = but->w >> 1;
  cgbuts_drawp(but->iPacket, win, goStone_black, FALSE,
	       but->x, but->y, stoneW, 0, x, y, w, h);
  cgbuts_drawp(but->iPacket, win, goStone_white, FALSE,
	       but->x + stoneW, but->y, stoneW, 1, x, y, w, h);
  cgbuts_drawp(but->iPacket, win, goStone_white, FALSE,
	       but->x, but->y + stoneW, stoneW, 2, x, y, w, h);
  cgbuts_drawp(but->iPacket, win, goStone_black, FALSE,
	       but->x + stoneW, but->y + stoneW, stoneW, 3, x, y, w, h);
}


static ButOut  stoneGroup_destroy(But *but)  {
  return(0);
}


But  *computerPic_create(Cgbuts *b, ButWin *win, int layer, int flags)  {
  But  *but;

  but = but_create(win, b, &computerPic_action);
  but->uPacket = NULL;
  but->layer = layer;
  but->flags = flags;
  return(but);
}


static void  computerPic_draw(But *but, int x,int y, int w,int h)  {
  XPoint  points[4];
  ButWin  *win = but->win;
  Display  *dpy = butEnv_dpy(butWin_env(win));
  Window  xwin = butWin_xwin(win);
  GC  gc = butEnv_gc(butWin_env(win));
  int  h2 = but->h / 2, h3 = but->h / 3, h12 = but->h / 12;
  int  w6 = but->w / 6, w9 = but->w / 9;

  x = but->x;
  y = but->y;
  w = but->w;
  h = but->h;
  y += (h - (h12 + h2 + h12 + h3)) / 2;
  points[0].x = x;
  points[0].y = y + h12 + h2 + h3;
  points[1].x = x + w6;
  points[1].y = y + h12 + h2 + h12;
  points[2].x = x + w - w6;
  points[2].y = y + h12 + h2 + h12;
  points[3].x = x + w;
  points[3].y = y + h12 + h2 + h3;
  butEnv_setXFg(butWin_env(win), BUT_BLACK);
  XFillPolygon(dpy, xwin, gc, points, 4, Nonconvex, CoordModeOrigin);
  XFillRectangle(dpy, xwin, gc, x+w6,y+h12, w-2*w6,h2);
  butEnv_setXFg(butWin_env(win), BUT_PBG);
  XFillRectangle(dpy, xwin, gc, x+w6+w9,y+h12*2, w-2*w6-2*w9,h2-h12*2);
}


static ButOut  computerPic_destroy(But *but)  {
  return(0);
}


But  *toolPic_create(Cgbuts *b, ButWin *win, int layer, int flags)  {
  But  *but;

  but = but_create(win, b, &toolPic_action);
  but->uPacket = NULL;
  but->layer = layer;
  but->flags = flags;
  return(but);
}


static void  toolPic_draw(But *but, int x,int y, int w,int h)  {
  XPoint  points[12];
  ButWin  *win = but->win;
  Display  *dpy = butEnv_dpy(butWin_env(win));
  Window  xwin = butWin_xwin(win);
  GC  gc = butEnv_gc(butWin_env(win));

  /* Size of hammer or screwdriver relative to whole picture. */
  const float  subBoxSize = 0.75;
  const float  bevel = 0.25;
  const float  hr2 = 0.70710678;  /* Half of Root 2 */

  /* Hammer constants. */
  const float  hHandleW = 0.1;
  const float  hJointW = 0.2;
  const float  hJointL = 0.2;
  const float  hClawL = 0.4;
  const float  hNeckW = 0.07;
  const float  hHeadL = 0.1;

  /* Screwdriver constants. */
  const float  sHandleW = 0.2;
  const float  sHandleL = 0.5;
  const float  sHEndW = 0.15;
  const float  sBladeW = 0.04;

  float  th;
  float  bx, by;

  x = but->x;
  y = but->y;
  h = but->h;
  th = h * subBoxSize;
  bx = x + h * (1.0 - subBoxSize);
  by = y + h * (1.0 - subBoxSize);

  /* The hammer's handle. */
  points[0].x = x;
  points[0].y = y + th * (1.0 - hHandleW) + 0.5;
  points[1].x = x + th * hHandleW + 0.5;
  points[1].y = points[0].y + (points[1].x - points[0].x);
  points[2].x = x + th * (1.0 - hJointL * hr2) + 0.5;
  points[2].y = points[1].y - (points[2].x - points[1].x);
  points[3].x = points[2].x - (points[1].x - points[0].x);
  points[3].y = points[2].y - (points[1].x - points[0].x);


  /* Add beveling to the handle. */
  points[4] = points[0];
  points[5].x = x + th * hHandleW * bevel + 0.5;
  points[5].y = points[4].y + (points[5].x - points[4].x);
  points[6].x = x + th * (1.0 - hJointL * hr2) + 0.5;
  points[6].y = points[5].y - (points[6].x - points[5].x);
  points[7] = points[3];

  points[9] = points[1];
  points[10] = points[2];
  points[8].x = points[9].x - th * hHandleW * bevel + 0.5;
  points[8].y = points[9].y - (points[9].x - points[8].x);
  points[11].x = points[10].x - (points[9].x - points[8].x);
  points[11].y = points[10].y - (points[9].x - points[8].x);

  butEnv_setXFg(butWin_env(win), CGBUTS_COLORBOARD(200));
  XFillPolygon(dpy, xwin, gc, points, 4, Nonconvex, CoordModeOrigin);
  butEnv_setXFg(butWin_env(win), CGBUTS_COLORBOARD(0));
  XFillPolygon(dpy, xwin, gc, points+4, 4, Nonconvex, CoordModeOrigin);
  butEnv_setXFg(butWin_env(win), CGBUTS_COLORBOARD(255));
  XFillPolygon(dpy, xwin, gc, points+8, 4, Nonconvex, CoordModeOrigin);

  /* The hammer's head. */
  points[0].x = x + th + 0.5;
  points[0].y = y + th * (hJointL * hr2) + 0.5;
  points[1].x = points[0].x - (points[0].y - y);
  points[1].y = y;
  points[2].x = x + th * (1.0 - hJointL * hr2 - hClawL) + 0.5;
  points[2].y = y;
  points[3].x = x + th * (1.0 - (hJointL + hJointW) * hr2) + 0.5;
  points[3].y = y + (points[1].x - points[3].x);
  points[4].x = points[3].x + (points[0].x - points[1].x);
  points[4].y = points[3].y + (points[4].x - points[3].x);
  points[5].x = x + th * (1.0 - (hJointW + hNeckW) * hr2 * 0.5) + 0.5;
  points[5].y = points[4].y - (points[5].x - points[4].x);
  points[6].x = points[5].x;
  points[6].y = points[4].y + (points[6].x - points[4].x);
  points[7].x = x + th * (1.0 + (hHeadL * 2.0 - hJointW) * hr2) + 0.5;
  points[7].y = points[6].y + (points[7].x - points[6].x);
  points[8].x = points[7].x + (points[0].x - points[4].x);
  points[8].y = points[7].y - (points[8].x - points[7].x);
  points[9].x = points[8].x - (points[7].x - points[6].x);
  points[9].y = points[8].y - (points[7].x - points[6].x);
  points[10].x = points[0].x - (points[9].y - points[0].y);
  points[10].y = points[9].y;

  butEnv_setXFg(butWin_env(win), BUT_BLACK);
  XFillPolygon(dpy, xwin, gc, points, 11, Nonconvex, CoordModeOrigin);

  /* The screwdriver's blade. */
  points[0].x = bx + th * (1.0 - sBladeW * hr2) + 0.5;
  points[0].y = by + 0.5;
  points[1].x = bx + th * sHandleW + 0.5;
  points[1].y = points[0].y + (points[0].x - points[1].x);
  points[3].x = bx + th + 0.5;
  points[3].y = points[0].y + (points[3].x - points[0].x);
  points[2].x = points[1].x + (points[3].x - points[0].x);
  points[2].y = points[1].y + (points[3].x - points[0].x);

  butEnv_setXFg(butWin_env(win), CGBUTS_GREY(120));
  XFillPolygon(dpy, xwin, gc, points, 4, Nonconvex, CoordModeOrigin);

  /* The screwdriver's handle. */
  points[4].x = bx + 0.5;
  points[4].y = by + th * (1.0 - sHandleW * hr2) + 0.5;
  points[5].x = points[4].x;
  points[5].y = by + th * (1.0 - sHEndW * hr2) + 0.5;
  points[6].y = by + th + 0.5;
  points[6].x = points[5].x + (points[6].y - points[5].y);
  points[7].y = points[6].y;
  points[7].x = points[6].x + (points[5].y - points[4].y);
  points[8].x = bx + th * (sHandleL + sHandleW) * hr2 + 0.5;
  points[8].y = points[7].y - (points[8].x - points[7].x);
  points[3].x = points[4].x + (points[8].x - points[7].x);
  points[3].y = points[4].y - (points[8].x - points[7].x);

  points[2].x = points[3].x + th * bevel * sHandleW * hr2 + 0.5;
  points[2].y = points[3].y + (points[2].x - points[3].x);
  points[9].x = points[8].x - (points[2].x - points[3].x);
  points[9].y = points[8].y - (points[2].x - points[3].x);
  points[1].x = points[4].x + (points[2].x - points[3].x);
  points[1].y = points[4].y + (points[2].x - points[3].x);
  points[10].x = points[7].x - (points[2].x - points[3].x);
  points[10].y = points[7].y - (points[2].x - points[3].x);
  points[0].x = points[5].x + th * bevel * sHEndW * hr2 + 0.5;
  points[0].y = points[5].y + (points[0].x - points[5].x);
  points[11].x = points[6].x - (points[0].x - points[5].x);
  points[11].y = points[6].y - (points[0].x - points[5].x);

  butEnv_setXFg(butWin_env(win), CGBUTS_COLORBOARD(200));
  XFillPolygon(dpy, xwin, gc, points+3, 6, Nonconvex, CoordModeOrigin);
  butEnv_setXFg(butWin_env(win), CGBUTS_COLORBOARD(0));
  XFillPolygon(dpy, xwin, gc, points, 6, Nonconvex, CoordModeOrigin);
  butEnv_setXFg(butWin_env(win), CGBUTS_COLORBOARD(255));
  XFillPolygon(dpy, xwin, gc, points+6, 6, Nonconvex, CoordModeOrigin);
}


static ButOut  toolPic_destroy(But *but)  {
  return(0);
}


void  cgbuts_redraw(Cgbuts *b)  {
  int  i;

  assert(MAGIC(b));
  for (i = 0;  i < b->numPics;  ++i)  {
    if (b->pics[i].stonePixmaps != None)  {
      XFreePixmap(butEnv_dpy(b->env), b->pics[i].stonePixmaps);
      XFreePixmap(butEnv_dpy(b->env), b->pics[i].maskBitmaps);
    }
  }
  wms_free(b->pics);
  b->pics = NULL;
  b->numPics = 0;
  butEnv_drawAll(b->env);
}

