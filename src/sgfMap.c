/*
 * src/xio/sgfMap.c, part of Complete Goban (game program)
 * Copyright (C) 1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 */

#include <wms.h>
#include <but/but.h>
#include <but/net.h>
#include <wms/rnd.h>
#include <wms/str.h>
#include <but/box.h>
#include "cgbuts.h"
#include "sgfMap.h"


/**********************************************************************
 * Forward declarations
 **********************************************************************/
static ButOut  mmove(But *but, int x, int y);
static ButOut  mleave(But *but);
static ButOut  mpress(But *but, int butNum, int x, int y);
static ButOut  mrelease(But *but, int butNum, int x, int y);
static void  draw(But *but, int x, int y, int w, int h);
static ButOut  destroy(But *but);
static void  relocate(But *but, SgfElem *elem, int x, int y, int numDiag,
		      int moveNum, bool mainLine);
static void  resizeGraph(SgfMap *map, int newX, int newY);
static int  findSubtreeY(SgfMap *map, SgfElem *tree, int x0, int y0,
			 int *numDiagOut);
static void  drawConn(But *but, uint flags, int x, int y, int fontH);
static void  newFlags(But *but, uint flags);
static void  redrawEl(But *but, int x, int y);
static void  newMain(But *but, SgfMap *map, int x, int y);


/**********************************************************************
 * Functions
 **********************************************************************/
But  *sgfMap_create(Cgbuts *b, ButOut (*callback)(But *but, int newNodeNum),
		    void *packet, ButWin *win, int layer, int flags,
		    Sgf *sgf)  {
  static const ButAction  action = {
    mmove, mleave, mpress, mrelease,
    NULL, NULL, draw, destroy, newFlags, NULL};
  SgfMap  *map;
  But  *but;

  map = wms_malloc(sizeof(SgfMap));
  MAGIC_SET(map);
  but = but_create(win, map, &action);
  but->uPacket = packet;
  but->layer = layer;
  but->flags = flags;

  map->cgbuts = b;
  map->callback = callback;
  map->mapW = map->mapH = 0;
  map->maxW = map->maxH = 1;
  map->activeX = map->activeY = 0;
  map->activeCtrX = map->activeCtrY = -1;
  map->els = wms_malloc(sizeof(SgfMapElem));
  map->els[0].node = NULL;
  map->els[0].flags = 0;
  map->els[0].type = sgfMap_none;
  map->els[0].moveNum = 0;

  but_init(but);
  relocate(but, sgf->top.activeChild, 0, 0, 0, 0, TRUE);
  map->els[0].flags &= ~SGFMAPFLAGS_CONNL;

  return(but);
}


void  sgfMap_destroy(But *but)  {
  SgfMap  *map = but->iPacket;

  assert(MAGIC(map));
  wms_free(map);
}


void  sgfMap_resize(But *but, int x, int y)  {
  SgfMap  *map;
  int  fontH;

  map = but->iPacket;
  assert(MAGIC(map));
  fontH = butEnv_fontH(but->win->env, 0);
  but_resize(but, x, y, map->mapW * 3 * fontH + fontH,
	     map->mapH * 3 * fontH + fontH);
}


static ButOut  mmove(But *but, int x, int y)  {
  SgfMap  *map = but->iPacket;
  ButEnv  *env = but->win->env;
  int  gx, gy;
  int  gridSpc = butEnv_fontH(env, 0);
  int  gridW = gridSpc * 3;
  uint  newFlags = but->flags;
  ButOut  result = BUTOUT_CAUGHT;
  bool  overBut = TRUE;

  gx = (x - but->x - gridSpc / 2) / gridW;
  gy = (y - but->y - gridSpc / 2) / gridW;
  if ((gx < 0) || (gy < 0) || (gx >= map->mapW) || (gy >= map->mapH))
    overBut = FALSE;
  else if (map->els[gx + gy * map->maxW].node == NULL)
    overBut = FALSE;
  else if ((but->flags & BUT_PRESSED) &&
	   ((gx != map->pressX) || (gy != map->pressY)))
    overBut = FALSE;
  if (overBut)
    newFlags |= BUT_TWITCHED;
  else  {
    newFlags &= ~BUT_TWITCHED;
    if (!(newFlags & BUT_LOCKED))
      result &= ~BUTOUT_CAUGHT;
  }
  if (!(but->flags & BUT_TWITCHED) && (newFlags & BUT_TWITCHED))
    butEnv_setCursor(env, but, butCur_twitch);
  else if ((but->flags & BUT_TWITCHED) && !(newFlags & BUT_TWITCHED))
    butEnv_setCursor(env, but, butCur_idle);
  if (newFlags != but->flags)  {
    but_newFlags(but, newFlags);
  }
  return(result);
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


static ButOut  mpress(But *but, int butNum, int x, int y)  {
  SgfMap  *map = but->iPacket;
  ButOut  result = BUTOUT_CAUGHT;
  ButEnv  *env = but->win->env;
  int  gx, gy;
  int  gridSpc = butEnv_fontH(env, 0);
  int  gridW = gridSpc * 3;
  uint  newFlags = but->flags;

  gx = (x - but->x - gridSpc / 2) / gridW;
  gy = (y - but->y - gridSpc / 2) / gridW;
  if (!(newFlags & BUT_TWITCHED))
    result &= ~BUTOUT_CAUGHT;
  else  {
    assert(gx >= 0);
    assert(gy >= 0);
    assert(gx < map->mapW);
    assert(gy < map->mapH);
    assert(map->els[gx + gy * map->maxW].node != NULL);
    if (butNum == 1)  {
      map->pressX = gx;
      map->pressY = gy;
      newFlags |= BUT_PRESSED | BUT_LOCKED;
    } else
      return(BUTOUT_CAUGHT | BUTOUT_ERR);
  }
  if (newFlags != but->flags)
    but_newFlags(but, newFlags);
  return(result);
}


static ButOut  mrelease(But *but, int butNum, int x, int y)  {
  ButOut  result = BUTOUT_CAUGHT;
  bool  makeCallback = FALSE;
  uint  newFlags = but->flags;
  SgfMap  *map = but->iPacket;

  if (butNum != 1)  {
    if (but->flags & BUT_TWITCHED)
      return(BUTOUT_CAUGHT);
    else
      return(0);
  }
  if (!(but->flags & BUT_PRESSED))
    return(0);
  if (but->flags & BUT_TWITCHED)  {
    makeCallback = TRUE;
  } else  {
    result |= BUTOUT_ERR;
  }
  newFlags &= ~(BUT_PRESSED|BUT_LOCKED);
  if (newFlags != but->flags)
    but_newFlags(but, newFlags);
  if (makeCallback && map->callback)  {
    if (!(map->els[map->pressX + map->pressY * map->maxW].flags &
	  SGFMAPFLAGS_MAIN))  {
      newMain(but, map, map->pressX, map->pressY);
    }
    result |= map->callback(but, map->pressX);
  }
  return(0);
}


static void  draw(But *but, int dx, int dy, int dw, int dh)  {
  ButWin  *win = but->win;
  ButEnv  *env = win->env;
  SgfMap  *map = but->iPacket;
  SgfMapElem  *el;
  int  gridW, gridSize, gridSep, bw;
  int  startY, endY, startX, endX;
  int  x, y, locX0, stoneX, stoneY;
  GoStone  stone = goStone_empty;
  bool  mark = TRUE, grey;

  assert(MAGIC(env));
  gridSep = butEnv_fontH(env, 0);
  gridSize = butEnv_fontH(env, 0) * 2;
  gridW = gridSep + gridSize;
  bw = butEnv_stdBw(env);

  startX = (dx - but->x - gridSep) / gridW;
  if (startX < 0)
    startX = 0;
  if (startX >= map->mapW)
    return;

  startY = (dy - but->y - gridSep) / gridW;
  if (startY < 0)
    startY = 0;
  if (startY >= map->mapH)
    return;

  endX = (dx + dw + gridSize - but->x) / gridW;
  if (endX < 1)
    return;
  if (endX > map->mapW)
    endX = map->mapW;

  endY = (dy + dh + gridSize - but->y) / gridW;
  if (endY < 1)
    return;
  if (endY > map->mapH)
    endY = map->mapH;

  if ((map->activeX >= startX) && (map->activeX < endX) &&
      (map->activeY >= startY) && (map->activeY < endY))  {
    stoneX = map->activeX * gridW + gridSep + but->x;
    stoneY = map->activeY * gridW + gridSep + but->y;
    butEnv_setXFg(env, BUT_CHOICE);
    XFillRectangle(env->dpy, win->win, env->gc,
		   stoneX - gridSep/2 + bw - win->xOff,
		   stoneY - gridSep/2 + bw - win->yOff,
		   gridW - bw*2, gridW - bw*2);
    but_drawBox(win, stoneX - gridSep/2, stoneY - gridSep/2,
		gridW, gridW, 0, bw,
		BUT_SRIGHT|BUT_SLEFT, BUT_LIT, BUT_SHAD, None, None);
  }
  if ((but->flags & BUT_PRESSED) && (but->flags & BUT_TWITCHED) &&
      (map->pressX >= startX) && (map->pressX < endX) &&
      (map->pressY >= startY) && (map->pressY < endY))  {
    stoneX = map->pressX * gridW + gridSep + but->x;
    stoneY = map->pressY * gridW + gridSep + but->y;
    butEnv_setXFg(env, BUT_PBG);
    XFillRectangle(env->dpy, win->win, env->gc,
		   stoneX - gridSep/2 + bw - win->xOff,
		   stoneY - gridSep/2 + bw - win->yOff,
		   gridW - bw*2, gridW - bw*2);
    but_drawBox(win, stoneX - gridSep/2, stoneY - gridSep/2,
		gridW, gridW, 0, bw,
		BUT_SRIGHT|BUT_SLEFT, BUT_SHAD, BUT_LIT, None, None);
  }
  for (y = startY;  y < endY;  ++y)  {
    locX0 = y * map->maxW;
    for (x = startX;  x < endX;  ++x)  {
      stoneX = x * gridW + gridSep + but->x;
      stoneY = y * gridW + gridSep + but->y;
      el = &map->els[locX0 + x];
      if (el->flags & SGFMAPFLAGS_CONN)
	drawConn(but, el->flags, stoneX, stoneY, gridSep);
      if (el->node == NULL)
	continue;
      switch(el->type)  {
      case sgfMap_white:
      case sgfMap_wPass:
	mark = TRUE;
	stone = goStone_white;
	break;
      case sgfMap_black:
      case sgfMap_bPass:
	mark = TRUE;
	stone = goStone_black;
	break;
      case sgfMap_edit:
	mark = FALSE;
	break;
      case sgfMap_none:
	mark = TRUE;
	stone = goStone_empty;
	break;
      }
      grey = !(el->flags & SGFMAPFLAGS_MAIN);
      if (mark)  {
	if (stone != goStone_empty)  {
	  cgbuts_drawp(map->cgbuts, win, stone, grey,
		       stoneX, stoneY, gridSize,
		       x % CGBUTS_NUMWHITE, dx, dy, dw, dh);
	} if (el->flags & SGFMAPFLAGS_MARKED)
	  cgbuts_markPiece(map->cgbuts, win, stone, goMark_triangle, 0,
			   stoneX, stoneY, gridSize, 1, dx, dy, dw, dh);
	else if (stone != goStone_empty)
	  cgbuts_markPiece(map->cgbuts, win, stone, goMark_number, el->moveNum,
			   stoneX, stoneY, gridSize, 1, dx, dy, dw, dh);
      } else  {
	cgbuts_drawp(map->cgbuts, win, goStone_black, grey,
		     stoneX, stoneY, gridSep,
		     1, dx, dy, dw, dh);
	cgbuts_drawp(map->cgbuts, win, goStone_white, grey,
		     stoneX + gridSep, stoneY, gridSep,
		     2, dx, dy, dw, dh);
	cgbuts_drawp(map->cgbuts, win, goStone_white, grey,
		     stoneX, stoneY + gridSep, gridSep,
		     3, dx, dy, dw, dh);
	cgbuts_drawp(map->cgbuts, win, goStone_black, grey,
		     stoneX + gridSep, stoneY + gridSep, gridSep,
		     4, dx, dy, dw, dh);
      }
    }
  }
}


static void  drawConn(But *but, uint flags, int x, int y, int fontH)  {
  ButWin  *win = but->win;
  ButEnv  *env = win->env;
  int  lineStart, lineEnd;
  int  lineW;

  lineW = (fontH + 3) / 8;
  if (lineW < 1)
    lineW = 1;
  XSetLineAttributes(env->dpy, env->gc, lineW, LineSolid, CapButt, JoinMiter);
  butEnv_setXFg(but->win->env, BUT_BLACK);
  lineStart = lineEnd = y + fontH;
  if (flags & SGFMAPFLAGS_CONNU)
    lineStart = y;
  if (flags & SGFMAPFLAGS_CONND)
    lineEnd += fontH * 2;
  if (lineStart != lineEnd)
    XDrawLine(env->dpy, win->win, env->gc,
	      x + fontH - win->xOff, lineStart - win->yOff,
	      x + fontH - win->xOff, lineEnd - win->yOff);

  lineStart = lineEnd = x + fontH;
  if (flags & SGFMAPFLAGS_CONNL)
    lineStart = x;
  if (flags & SGFMAPFLAGS_CONNR)
    lineEnd += fontH * 2;
  if (lineStart != lineEnd)
    XDrawLine(env->dpy, win->win, env->gc,
	      lineStart - win->xOff, y + fontH - win->yOff,
	      lineEnd - win->xOff, y + fontH - win->yOff);

  lineStart = lineEnd = fontH;
  if (flags & SGFMAPFLAGS_CONNUL)
    lineStart = 0;
  if (flags & SGFMAPFLAGS_CONNDR)
    lineEnd += fontH * 2;
  if (lineStart != lineEnd)
    XDrawLine(env->dpy, win->win, env->gc,
	      x + lineStart - win->xOff, y + lineStart - win->yOff,
	      x + lineEnd - win->xOff, y + lineEnd - win->yOff);
}


static ButOut  destroy(But *but)  {
  SgfMap  *map = but->iPacket;
  if (map->els != NULL)
    wms_free(map->els);
  wms_free(map);
  return(0);
}


static void  relocate(But *but, SgfElem *elem, int x, int y, int numDiag,
		      int moveNum, bool mainLine)  {
  SgfMap  *map = but->iPacket;
  SgfMapElem  *me;
  SgfElem  *child;
  int  connLoc;

  assert(MAGIC(map));
  if (elem == NULL)
    return;
  resizeGraph(map, x, y);
  me = &map->els[x + y * map->maxW];
  if (numDiag)  {
    me->flags |= SGFMAPFLAGS_CONNUL;
    --numDiag;
  } else  {
    me->flags |= SGFMAPFLAGS_CONNL;
  }
  if (mainLine)
    me->flags |= SGFMAPFLAGS_MAIN;
  me->type = sgfMap_none;
  while (elem->childH && (elem->childH->type != sgfType_node))  {
    elem->mapX = x;
    elem->mapY = y;
    assert(elem->childH == elem->childT);
    elem = elem->childH;
    switch(elem->type)  {
    case sgfType_move:
      if (elem->gVal == goStone_white)
	me->type = sgfMap_white;
      else  {
	assert(elem->gVal == goStone_black);
	me->type = sgfMap_black;
      }
      me->moveNum = ++moveNum;
      break;
    case sgfType_pass:
      if (elem->gVal == goStone_white)
	me->type = sgfMap_wPass;
      else  {
	assert(elem->gVal == goStone_black);
	me->type = sgfMap_bPass;
      }
      me->moveNum = ++moveNum;
      break;
    case sgfType_triangle:
    case sgfType_comment:
    case sgfType_square:
    case sgfType_circle:
    case sgfType_label:
      me->flags |= SGFMAPFLAGS_MARKED;
      break;
    case sgfType_setBoard:
      me->type = sgfMap_edit;
      break;
    default:
      break;
    }
  }
  elem->mapX = x;
  elem->mapY = y;
  me->node = elem;
  if (elem->childH)  {
    if (numDiag)  {
      me->flags |= SGFMAPFLAGS_CONNDR;
      relocate(but, elem->childH, x + 1, y + 1, numDiag, moveNum,
	       mainLine && (elem->childH == elem->activeChild));
    } else  {
      me->flags |= SGFMAPFLAGS_CONNR;
      relocate(but, elem->childH, x + 1, y, 0, moveNum,
	       mainLine && (elem->childH == elem->activeChild));
    }
    for (child = elem->childH->sibling;  child;  child = child->sibling)  {
      connLoc = y;
      y = findSubtreeY(map, child, x, y, &numDiag);
      assert(numDiag > 0);
      while (connLoc < y)  {
	map->els[x + connLoc * map->maxW].flags |= SGFMAPFLAGS_CONND;
	map->els[x + ++connLoc * map->maxW].flags |= SGFMAPFLAGS_CONNU;      
      }
      map->els[x + y * map->maxW].flags |= SGFMAPFLAGS_CONNDR;
      relocate(but, child, x + 1, y + 1, numDiag, moveNum,
	       mainLine && (child == elem->activeChild));
    }
  }
}


static void  resizeGraph(SgfMap *map, int newX, int newY)  {
  SgfMapElem  *newEls;
  int  newMaxW, newMaxH;
  int  x, y, oldX0, newX0;

  if ((newX >= map->maxW) || (newY >= map->maxH))  {
    newMaxW = (newX + 1) * 2;
    if (newMaxW < map->maxW)
      newMaxW = map->maxW;
    newMaxH = (newY + 1) * 2;
    if (newMaxH < map->maxH)
      newMaxH = map->maxH;
    newEls = wms_malloc(newMaxW * newMaxH * sizeof(SgfMapElem));
    for (y = 0;  y < map->maxH;  ++y)  {
      oldX0 = y * map->maxW;
      newX0 = y * newMaxW;
      for (x = 0;  x < map->maxW;  ++x)  {
	newEls[newX0 + x] = map->els[oldX0 + x];
      }
      for (x = map->maxW;  x < newMaxW;  ++x)  {
	newEls[newX0 + x].node = NULL;
	newEls[newX0 + x].flags = 0;
	newEls[newX0 + x].moveNum = 0;
	newEls[newX0 + x].type = sgfMap_none;
      }
    }
    for (y = map->maxH;  y < newMaxH;  ++y)  {
      newX0 = y * newMaxW;
      for (x = 0;  x < newMaxW;  ++x)  {
	newEls[newX0 + x].node = NULL;
	newEls[newX0 + x].flags = 0;
	newEls[newX0 + x].type = sgfMap_none;
	newEls[newX0 + x].moveNum = 0;
      }
    }
    if (map->els != NULL)
      wms_free(map->els);
    map->els = newEls;
    map->maxW = newMaxW;
    map->maxH = newMaxH;
  }
  if (newX >= map->mapW)
    map->mapW = newX + 1;
  if (newY >= map->mapH)
    map->mapH = newY + 1;
}


static int  findSubtreeY(SgfMap *map, SgfElem *tree, int x0, int y0,
			 int *numDiagOut)  {
  int  x, y, numDiag;

  x = x0;
  y = y0 + 1;
  numDiag = 1;
  while (tree)  {
    ++x;
    resizeGraph(map, x, y);
    while (map->els[y * map->maxW + x].node ||
	   map->els[y * map->maxW + x].flags)  {
      ++y;
      ++numDiag;
      resizeGraph(map, x, y);
    }
    if (numDiag > x - x0)  {
      numDiag = x - x0;
    }
    do  {
      tree = tree->childH;
    } while (tree && (tree->type != sgfType_node));
  }
  *numDiagOut = numDiag;
  return(y - numDiag);
}


void  sgfMap_newActive(But *but, SgfElem *new)  {
  SgfMap  *map = but->iPacket;
  ButWin  *win = but->win;
  int  butSpc = butEnv_fontH(win->env, 0);

  assert(MAGIC(map));
  assert(new->mapX >= 0);
  assert(new->mapY >= 0);
  assert(new->mapX < map->mapW);
  assert(new->mapY < map->mapH);
  if (map->activeX >= 0)  {
    redrawEl(but, map->activeX, map->activeY);
  }
  map->activeX = new->mapX;
  map->activeY = new->mapY;
  if (map->activeX >= 0)  {
    map->activeCtrX = but->x + butSpc * 3 * map->activeX + butSpc * 2;
    map->activeCtrY = but->y + butSpc * 3 * map->activeY + butSpc * 2;
    redrawEl(but, map->activeX, map->activeY);
  }
}


static void  newFlags(But *but, uint flags)  {
  SgfMap  *map = but->iPacket;
  uint  ofl = but->flags;

  but->flags = flags;
  if (((flags & BUT_PRESSED) != (ofl & BUT_PRESSED)) ||
      (((flags & BUT_TWITCHED) != (ofl & BUT_TWITCHED)) &&
       (flags & BUT_PRESSED)))  {
    redrawEl(but, map->pressX, map->pressY);
  }
}


static void  redrawEl(But *but, int x, int y)  {
  int  elSpc = butEnv_fontH(but->win->env, 0);

  butWin_redraw(but->win, but->x + x * elSpc * 3,
		but->y + y * elSpc * 3, elSpc * 4, elSpc * 4);
}


static void  newMain(But *but, SgfMap *map, int x, int y)  {
  SgfElem  *node, *prevNode, *oldMain;
  int  prevX, prevY;
  SgfMapElem  *el;

  el = &map->els[x + y * map->maxW];
  assert(MAGIC(el->node));
  /*
   * First, go _down_ from the current active node, coloring all 
   *   links as "main".
   */
  prevX = prevY = -1;
  for (node = el->node;  node;  node = node->activeChild)  {
    if ((prevX != node->mapX) || (prevY != node->mapY))  {
      prevX = node->mapX;
      prevY = node->mapY;
      map->els[prevX + prevY * map->maxW].flags |= SGFMAPFLAGS_MAIN;
      redrawEl(but, prevX, prevY);
    }
  }
  /*
   * Now, go _up_ from the current active node, coloring all links as
   *   "main".
   */
  prevX = x;
  prevY = y;
  prevNode = el->node;
  for (node = prevNode->parent;  ;  node = node->parent)  {
    oldMain = node->activeChild;
    node->activeChild = prevNode;
    assert(node == prevNode->parent);
    assert((node->childH != node->childT) ||
	   (node->activeChild == node->childH));
    prevNode = node;
    if ((prevX != node->mapX) || (prevY != node->mapY))  {
      prevX = node->mapX;
      prevY = node->mapY;
      if (map->els[prevX + prevY * map->maxW].flags & SGFMAPFLAGS_MAIN)
	break;
      else  {
	map->els[prevX + prevY * map->maxW].flags |= SGFMAPFLAGS_MAIN;
	redrawEl(but, prevX, prevY);
      }
    }
  }
  assert(oldMain != node->activeChild);
  /*
   * Now, make all the stuff that _used_ to be main as "non-main".
   */
  for (;  oldMain;  oldMain = oldMain->activeChild)  {
    if ((prevX != oldMain->mapX) || (prevY != oldMain->mapY))  {
      prevX = oldMain->mapX;
      prevY = oldMain->mapY;
      map->els[prevX + prevY * map->maxW].flags &= ~SGFMAPFLAGS_MAIN;
      redrawEl(but, prevX, prevY);
    }
  }
}


bool  sgfMap_changeVar(But *but, SgfMapDirection dir)  {
  SgfMap  *map = but->iPacket;
  SgfElem  *active, *curChild, *newChild;

  assert(MAGIC(map));
  active = map->els[map->activeX + map->activeY * map->maxW].node;
  assert(MAGIC(active));
  while ((active->childH != NULL) &&
	 (active->childH->type != sgfType_node))  {
    active = active->childH;
    assert(MAGIC(active));
  }
  if ((active->childH == NULL) ||
      (active->childH == active->childT))
    return(FALSE);
  curChild = active->activeChild;
  assert(MAGIC(curChild));
  if (dir == sgfMap_next)  {
    newChild = curChild->sibling;
    if (!newChild)
      newChild = active->childH;
  } else  {
    assert(dir == sgfMap_prev);
    if (curChild == active->childH)
      newChild = active->childT;
    else  {
      for (newChild = active->childH;
	   newChild->sibling != curChild;
	   newChild = newChild->sibling);
    }
  }
  newMain(but, map, newChild->mapX, newChild->mapY);
  return(TRUE);
}


void  sgfMap_remap(But *but, Sgf *sgf)  {
  SgfMap  *map = but->iPacket;
  int  i, fontH = butEnv_fontH(but->win->env, 0);

  assert(MAGIC(map));
  for (i = 0;  i < map->maxW * map->maxH;  ++i)  {
    map->els[i].node = NULL;
    map->els[i].flags = 0;
    map->els[i].type = sgfMap_none;
    map->els[i].moveNum = 0;
  }
  map->mapW = map->mapH = 0;
  relocate(but, sgf->top.activeChild, 0, 0, 0, 0, TRUE);
  map->els[0].flags &= ~SGFMAPFLAGS_CONNL;
  but_resize(but, but->x, but->y, map->mapW * 3 * fontH + fontH,
	     map->mapH * 3 * fontH + fontH);
  but_draw(but);
}


bool  sgfMap_newNode(But *but, SgfElem *new)  {
  SgfMap  *map = but->iPacket;
  int  newX, newY;
  SgfMapElem  *el;

  assert(MAGIC(map));
  newX = new->mapX;
  newY = new->mapY;
  assert(newX < map->maxW);
  if (newX + 1 == map->mapW)
    return(FALSE);
  if (map->els[newX + newY * map->maxW].flags &
      (SGFMAPFLAGS_CONND | SGFMAPFLAGS_CONNDR | SGFMAPFLAGS_CONNR))
    return(FALSE);
  if (map->els[newX + 1 + newY * map->maxW].flags)
    return(FALSE);
  map->els[newX + newY * map->maxW].flags |=
    SGFMAPFLAGS_CONNR | SGFMAPFLAGS_MAIN;
  el = &map->els[newX + 1 + newY * map->maxW];
  ++newX;
  el->flags |= SGFMAPFLAGS_CONNL | SGFMAPFLAGS_MAIN;
  redrawEl(but, newX, newY);
  new->mapX = newX;
  new->mapY = newY;
  el->node = new;
  el->type = sgfMap_none;
  return(TRUE);
}


void  sgfMap_changeNode(But *but, SgfElem *changed)  {
  SgfMap  *map = but->iPacket;
  SgfMapElem  *me;
  int  moveAdd = 0;

  assert(MAGIC(map));
  assert(changed != NULL);
  me = &map->els[changed->mapX + changed->mapY * map->maxW];
  me->type = sgfMap_none;
  me->flags &= ~SGFMAPFLAGS_MARKED;
  while (changed->parent && (changed->type != sgfType_node))  {
    switch(changed->type)  {
    case sgfType_move:
      if (changed->gVal == goStone_white)
	me->type = sgfMap_white;
      else  {
	assert(changed->gVal == goStone_black);
	me->type = sgfMap_black;
      }
      moveAdd = 1;
      break;
    case sgfType_pass:
      if (changed->gVal == goStone_white)
	me->type = sgfMap_wPass;
      else  {
	assert(changed->gVal == goStone_black);
	me->type = sgfMap_bPass;
      }
      moveAdd = 1;
      break;
    case sgfType_triangle:
    case sgfType_comment:
    case sgfType_square:
    case sgfType_circle:
    case sgfType_label:
      me->flags |= SGFMAPFLAGS_MARKED;
      break;
    case sgfType_setBoard:
      me->type = sgfMap_edit;
      break;
    default:
      break;
    }
    changed = changed->parent;
    assert(changed != NULL);
  }
  if (moveAdd) {
    SgfElem  *parent = changed->parent;
    while (parent)  {
      if (parent->type == sgfType_move || parent->type == sgfType_pass)  {
        moveAdd += map->els[parent->mapX + parent->mapY * map->maxW].moveNum;
        break;
      }
      parent = parent->parent;
    }
  }
  me->moveNum = moveAdd;
  assert(changed != NULL);
  redrawEl(but, changed->mapX, changed->mapY);
}


/*
 * Sometimes, in our frenzy of adding and deleting SgfElems, we could kill
 *   the one that the map points to.  So every time we change active nodes,
 *   we first reset the map to point to the old active node.
 */
void  sgfMap_setMapPointer(But *but, SgfElem *activeElem)  {
  SgfMap  *map = but->iPacket;
  int  x, y;
  
  assert(MAGIC(map));
  x = activeElem->mapX;
  y = activeElem->mapY;
  assert((x >= 0) && (x < map->mapW) && (y >= 0) && (y < map->mapH));
  assert(map->els[x + y * map->maxW].node);
  map->els[x + y * map->maxW].node = activeElem;
}
