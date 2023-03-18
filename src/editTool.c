/*
 * src/editTool.c, part of Complete Goban (game program)
 * Copyright (C) 1995 William Shubert
 * See "configure.h.in" for more copyright information.
 */

#include <wms.h>
#include <wms/str.h>
#include <wms/clp.h>
#include <but/but.h>
#include <abut/msg.h>
#include <but/text.h>
#include <abut/term.h>
#include <abut/fsel.h>
#include <abut/swin.h>
#include <but/radio.h>
#include <but/plain.h>
#include <but/canvas.h>
#include <but/ctext.h>
#include "cgoban.h"
#include "editTool.h"
#include "msg.h"
#include "sgfMap.h"
#include "sgf.h"
#include "help.h"


/**********************************************************************
 * Forward declarations
 **********************************************************************/
static ButOut  unmap(ButWin *win);
static ButOut  map(ButWin *win);
static ButOut  resize(ButWin *win);
static ButOut  iResize(ButWin *win);
static ButOut  mapResize(ButWin *win);
static ButOut  destroy(ButWin *win);
static ButOut  newTool(But *but, int val);
static ButOut  windowQuit(ButWin *win);
static ButOut  mapCallback(But *but, int newNodeNum);
static ButOut  cutPressed(But *but);
static ButOut  movePressed(But *but);
static void  updateTreeButtons(EditToolWin *etw);


/**********************************************************************
 * Functions
 **********************************************************************/
#define  editW(bw, fontH)  ((fontH*2 + bw*4) * editTool_max + bw * 4)


void  editToolWin_init(EditToolWin *etw, Cgoban *cg, Sgf *sgf,
		       void (*quitRequested)(void *packet),
		       ButOut (*newToolCallback)(void *packet),
		       ButOut (*newActiveNode)(void *packet,
					       int nodeNum),
		       void *packet)  {
  static const ButKey  shiftUpKeys[] = {{XK_Up, ShiftMask, ShiftMask},
					{0,0,0}};
  static const ButKey  shiftDownKeys[] = {{XK_Down, ShiftMask, ShiftMask},
					  {0,0,0}};
  int  w, h, minW, minH;
  But  *b;
  int  i, fontH;
  bool  err;

  MAGIC_SET(etw);
  etw->cg = cg;
  etw->modified = FALSE;
  fontH = butEnv_fontH(cg->env, 0);
  minW = editW(butEnv_stdBw(cg->env), fontH);
  minH = minW * 0.70710678 + 0.5;
  w = (fontH * clp_getDouble(cg->clp, "edit.toolW") + 0.5);
  h = (fontH * clp_getDouble(cg->clp, "edit.toolH") + 0.5);
  if (w < minW)
    w = minW;
  if (h < minH)
    h = minH;
  etw->toolWin = butWin_iCreate(etw, cg->env, "Cgoban Tools", w, h,
				&etw->toolIWin, FALSE, 48, 48, unmap, map,
				resize, iResize, destroy);
  i = clp_iGetInt(cg->clp, "edit.toolX", &err);
  if (!err)
    butWin_setX(etw->toolWin, i);
  i = clp_iGetInt(cg->clp, "edit.toolY", &err);
  if (!err)
    butWin_setY(etw->toolWin, i);
  butWin_setMinW(etw->toolWin, minW);
  butWin_setMinH(etw->toolWin, minH);
  butWin_setMaxW(etw->toolWin, 0);
  butWin_setMaxH(etw->toolWin, 0);
  butWin_setQuit(etw->toolWin, windowQuit);
  butWin_activate(etw->toolWin);
  etw->toolBg = butBoxFilled_create(etw->toolWin, 0, BUT_DRAWABLE);
  butBoxFilled_setPixmaps(etw->toolBg, cg->bgLitPixmap, cg->bgShadPixmap,
			  cg->bgPixmap);
  etw->toolIBg = butBoxFilled_create(etw->toolIWin, 0, BUT_DRAWABLE);
  butBoxFilled_setPixmaps(etw->toolIBg, cg->bgLitPixmap, cg->bgShadPixmap,
			  cg->bgPixmap);
  etw->toolIPic = toolPic_create(&cg->cgbuts, etw->toolIWin, 1, BUT_DRAWABLE);
  etw->toolSel = butRadio_create(newTool, etw, etw->toolWin, 1,
				 BUT_PRESSABLE|BUT_DRAWABLE,
				 0, editTool_max);
  
  etw->selDesc[editTool_play] = b =
    grid_create(&cg->cgbuts, NULL, NULL, etw->toolWin, 2,
		BUT_DRAWABLE|BUT_PRESSTHRU, 0);
  grid_setStone(b, etw->lastColor = goStone_black, FALSE);
  grid_setLineGroup(b, gridLines_none);
  grid_setVersion(b, 0);
    
  etw->selDesc[editTool_changeBoard] =
    stoneGroup_create(&cg->cgbuts, etw->toolWin,
		      2, BUT_DRAWABLE|BUT_PRESSTHRU);
    
  etw->selDesc[editTool_score] = b = 
    grid_create(&cg->cgbuts, NULL, NULL, etw->toolWin,
		2, BUT_DRAWABLE|BUT_PRESSTHRU, 0);
  grid_setStone(b, goStone_black, FALSE);
  grid_setLineGroup(b, gridLines_none);
  grid_setVersion(b, CGBUTS_YINYANG(1));
    
  etw->selDesc[editTool_triangle] = b =
    grid_create(&cg->cgbuts, NULL, NULL, etw->toolWin,
		2, BUT_DRAWABLE|BUT_PRESSTHRU, 0);
  grid_setStone(b, goStone_black, FALSE);
  grid_setLineGroup(b, gridLines_none);
  grid_setMark(b, goMark_triangle, 0);
    
  etw->selDesc[editTool_square] = b =
    grid_create(&cg->cgbuts, NULL, NULL, etw->toolWin,
		2, BUT_DRAWABLE|BUT_PRESSTHRU, 0);
  grid_setStone(b, goStone_black, FALSE);
  grid_setLineGroup(b, gridLines_none);
  grid_setMark(b, goMark_square, 0);
    
  etw->selDesc[editTool_circle] = b =
    grid_create(&cg->cgbuts, NULL, NULL, etw->toolWin,
		2, BUT_DRAWABLE|BUT_PRESSTHRU, 0);
  grid_setStone(b, goStone_black, FALSE);
  grid_setLineGroup(b, gridLines_none);
  grid_setMark(b, goMark_circle, 0);
    
  etw->selDesc[editTool_letter] = b =
    grid_create(&cg->cgbuts, NULL, NULL, etw->toolWin,
		2, BUT_DRAWABLE|BUT_PRESSTHRU, 0);
  grid_setStone(b, goStone_black, FALSE);
  grid_setLineGroup(b, gridLines_none);
  grid_setMark(b, goMark_letter, 'A');
  grid_setVersion(b, 1);
    
  etw->selDesc[editTool_number] = b =
    grid_create(&cg->cgbuts, NULL, NULL, etw->toolWin,
		2, BUT_DRAWABLE|BUT_PRESSTHRU, 0);
  grid_setStone(b, goStone_black, FALSE);
  grid_setLineGroup(b, gridLines_none);
  grid_setMark(b, goMark_number, 1);
  grid_setVersion(b, 2);

  etw->mapWin = abutSwin_create(etw, etw->toolWin, 1,
				ABUTSWIN_LSLIDE|ABUTSWIN_BSLIDE,
				mapResize);
  etw->mapBg = butPixmap_create(etw->mapWin->win, 0, BUT_DRAWABLE,
				cg->boardPixmap);
  etw->sgfMap = sgfMap_create(&cg->cgbuts, mapCallback, etw,
			      etw->mapWin->win,
			      1, BUT_DRAWABLE|BUT_PRESSABLE, sgf);
  etw->prevVar = butKeytrap_create(editToolWin_shiftUpPressed, etw,
				   etw->toolWin, BUT_DRAWABLE|BUT_PRESSABLE);
  but_setKeys(etw->prevVar, shiftUpKeys);
  etw->nextVar = butKeytrap_create(editToolWin_shiftDownPressed, etw,
				   etw->toolWin, BUT_DRAWABLE|BUT_PRESSABLE);
  but_setKeys(etw->nextVar, shiftDownKeys);

  etw->sgf = sgf;

  etw->tool = editTool_play;
    
  etw->toolBox = butBoxFilled_create(etw->toolWin, 1, BUT_DRAWABLE);
  etw->toolName = butText_create(etw->toolWin, 2, BUT_DRAWABLE,
				 msg_toolNames[etw->tool],
				 butText_center);
  butText_setFont(etw->toolName, 2);
  etw->toolDesc1 = butText_create(etw->toolWin, 2, BUT_DRAWABLE,
				  msg_toolDesc1[etw->tool],
				  butText_center);
  etw->toolDesc2 = butText_create(etw->toolWin, 2, BUT_DRAWABLE,
				  msg_toolDesc2[etw->tool],
				  butText_center);
  etw->help = butCt_create(cgoban_createHelpWindow, &help_editTool,
			   etw->toolWin, 1,
			   BUT_DRAWABLE | BUT_PRESSABLE, msg_help);
  etw->killNode = butCt_create(cutPressed, etw, etw->toolWin, 1,
			       BUT_DRAWABLE | BUT_PRESSABLE,
			       msg_killNode);
  etw->moveNode = butCt_create(movePressed, etw, etw->toolWin, 1,
			       BUT_DRAWABLE | BUT_PRESSABLE,
			       msg_moveNode);
			       
  etw->quitRequested = quitRequested;
  etw->newToolCallback = newToolCallback;
  etw->mapCallback = newActiveNode;
  etw->packet = packet;
  updateTreeButtons(etw);
}


void  editToolWin_deinit(EditToolWin *etw)  {
  int  fontH;

  assert(MAGIC(etw));
  if (etw->toolWin != NULL)  {
    fontH = butEnv_fontH(etw->cg->env, 0);
    clp_setInt(etw->cg->clp, "edit.toolX", butWin_x(etw->toolWin));
    clp_setInt(etw->cg->clp, "edit.toolY", butWin_y(etw->toolWin));
    clp_setDouble(etw->cg->clp, "edit.toolW",
		  (double)butWin_w(etw->toolWin) / (double)fontH);
    clp_setDouble(etw->cg->clp, "edit.toolH",
		  (double)butWin_h(etw->toolWin) / (double)fontH);
    butWin_setPacket(etw->toolWin, NULL);
    butWin_destroy(etw->toolWin);
  }
  MAGIC_UNSET(etw);
}


static ButOut  unmap(ButWin *win)  {
  return(0);
}


static ButOut  map(ButWin *win)  {
  return(0);
}


static ButOut  resize(ButWin *win)  {
  EditToolWin  *etw = butWin_packet(win);
  ButEnv  *env = butWin_env(win);
  int  bw = butEnv_stdBw(env);
  int  fontH = butEnv_fontH(env, 0), font2H = butEnv_fontH(env, 2);
  int  butH;
  int  w = butWin_w(win), h = butWin_h(win);
  EditTool  tool;
  int  x, x2, y, toolW;

  assert(MAGIC(etw));
  butH = etw->cg->fontH * 2;
  but_resize(etw->toolBg, 0, 0, w, h);
  but_resize(etw->toolSel, bw * 2, bw * 2, w - bw * 4, fontH * 2 + bw * 4);
  x2 = bw * 2;
  toolW = fontH * 2;
  for (tool = editTool_min;  tool < editTool_max;  ++tool)  {
    x = x2;
    x2 = bw * 2 + ((tool + 1) * (w - bw * 4)) / editTool_max;
    but_resize(etw->selDesc[tool], (x + x2 - toolW) / 2, bw*4, toolW, toolW);
  }
  y = fontH * 2 + bw * 6;
  but_resize(etw->toolBox, bw * 2, y, w - bw * 4, bw * 4 + font2H + fontH * 2);
  but_resize(etw->toolName, bw * 4, y += bw * 2, w - bw * 8, font2H);
  but_resize(etw->toolDesc1, bw * 4, y += font2H, w - bw * 8, fontH);
  but_resize(etw->toolDesc2, bw * 4, y += fontH, w - bw * 8, fontH);
  but_resize(etw->help, bw*2, y += fontH + bw * 3, (w - bw * 6 + 1) / 3, butH);
  but_resize(etw->killNode, bw*3 + (w - bw * 6 + 1) / 3, y,
	     w - bw * 6 - 2*(w - bw * 6 + 1) / 3, butH);
  but_resize(etw->moveNode, w - bw * 2 - (w - bw * 6 + 1) / 3, y,
	     (w - bw * 6 + 1) / 3, butH);

  y += butH + bw;
  abutSwin_resize(etw->mapWin, bw * 2, y, w - bw * 4, h - bw * 2 - y,
		  (fontH * 3) / 2, fontH * 3);
  sgfMap_resize(etw->sgfMap, 0, 0);
  butCan_resizeWin(etw->mapWin->win, but_w(etw->sgfMap), but_h(etw->sgfMap),
		   TRUE);
	     
  return(0);
}


static ButOut  iResize(ButWin *win)  {
  EditToolWin  *etw = butWin_packet(win);
  int  bw = butEnv_stdBw(win->env);

  assert(MAGIC(etw));
  but_resize(etw->toolIBg, 0, 0, butWin_w(win), butWin_h(win));
  but_resize(etw->toolIPic, bw*2, bw*2, butWin_w(win) - bw*4,
	     butWin_h(win) - bw*4);
  return(0);
}


static ButOut  mapResize(ButWin *win)  {
  AbutSwin  *swin = butWin_packet(win);
  EditToolWin  *etw;

  assert(MAGIC(swin));
  etw = swin->packet;
  assert(MAGIC(etw));
  but_resize(etw->mapBg, 0, 0, butWin_w(win), butWin_h(win));
  return(0);
}


static ButOut  destroy(ButWin *win)  {
  EditToolWin  *etw = butWin_packet(win);

  if (etw)  {
    assert(MAGIC(etw));
    clp_setInt(etw->cg->clp, "edit.toolX", butWin_x(win));
    clp_setInt(etw->cg->clp, "edit.toolY", butWin_y(win));
    etw->toolWin = NULL;
    /* editToolWin_deinit(etw); */
  }
  return(0);
}


static ButOut  windowQuit(ButWin *win)  {
  EditToolWin  *etw = butWin_packet(win);

  assert(MAGIC(etw));
  etw->quitRequested(etw->packet);
  return(0);
}


static ButOut  newTool(But *but, int val)  {
  EditToolWin  *etw;

  etw = but_packet(but);
  assert(MAGIC(etw));
  if ((EditTool)val != etw->tool)  {
    etw->tool = (EditTool)val;
    butText_set(etw->toolName, msg_toolNames[val]);
    butText_set(etw->toolDesc1, msg_toolDesc1[val]);
    butText_set(etw->toolDesc2, msg_toolDesc2[val]);
  }
  return(etw->newToolCallback(etw->packet));
}


void  editToolWin_newColor(EditToolWin *etw, GoStone color)  {
  if (color != etw->lastColor)  {
    grid_setStone(etw->selDesc[editTool_play], etw->lastColor = color, FALSE);
  }
}


void  editToolWin_newTool(EditToolWin *etw, EditTool tool, bool propagate)  {
  butRadio_set(etw->toolSel, tool, propagate);
}


static ButOut  mapCallback(But *but, int newNodeNum)  {
  EditToolWin  *etw = but_packet(but);
  ButOut  result;

  assert(MAGIC(etw));
  result = etw->mapCallback(etw->packet, newNodeNum);
  updateTreeButtons(etw);
  return(result);
}


void  editToolWin_newActiveNode(EditToolWin *etw, SgfElem *newNode)  {
  int  w, h, loX, loY, hiX, hiY;
  int  aX, aY;
  int  slideX, slideY, newX, newY;

  sgfMap_newActive(etw->sgfMap, newNode);
  w = butWin_viewW(etw->mapWin->win);
  h = butWin_viewH(etw->mapWin->win);
  newX = slideX = butCan_xOff(etw->mapWin->win);
  newY = slideY = butCan_yOff(etw->mapWin->win);
  loX = slideX + (w + 1) / 4;
  loY = slideY + (h + 1) / 4;
  hiX = slideX + (w * 3 + 1) / 4;
  hiY = slideY + (h * 3 + 1) / 4;
  aX = sgfMap_activeCtrX(etw->sgfMap);
  aY = sgfMap_activeCtrY(etw->sgfMap);
  if (aX < loX)  {
    newX = aX - (w + 1) / 4;
    if (newX < 0)
      newX = 0;
  } else if (aX > hiX)  {
    newX = aX - (w * 3 + 1) / 4;
    if (newX + w > butWin_w(etw->mapWin->win))
      newX = butWin_w(etw->mapWin->win) - w;
  }
  if (aY < loY)  {
    newY = aY - (h + 1) / 4;
    if (newY < 0)
      newY = 0;
  } else if (aY > hiY)  {
    newY = aY - (h * 3 + 1) / 4;
    if (newY + h > butWin_h(etw->mapWin->win))
      newY = butWin_h(etw->mapWin->win) - h;
  }
  if ((newX != slideX) || (newY != slideY))  {
    butCan_slide(etw->mapWin->win, newX, newY, TRUE);
  }
  updateTreeButtons(etw);
}


static ButOut  cutPressed(But *but)  {
  EditToolWin  *etw = but_packet(but);
  SgfElem  *deadElem, *newActive;
  ButOut  result;

  assert(MAGIC(etw));
  etw->modified = TRUE;
  deadElem = etw->sgf->active->activeChild;
  if (deadElem == NULL)
    return(BUTOUT_ERR);
  newActive = deadElem->sibling;
  if (newActive)  {
    sgfMap_changeVar(etw->sgfMap, sgfMap_next);
  } else  {
    for (newActive = etw->sgf->active->childH;
	 newActive && (newActive->sibling != deadElem);
	 newActive = newActive->sibling);
    if (newActive)
      sgfMap_changeVar(etw->sgfMap, sgfMap_prev);
  }
  etw->sgf->active->activeChild = deadElem;
  assert(newActive != etw->sgf->active->activeChild);
  assert(MAGICNULL(newActive));
  sgfElem_destroyActiveChild(etw->sgf->active);
  assert(MAGICNULL(newActive));
  etw->sgf->active->activeChild = newActive;
  sgfMap_remap(etw->sgfMap, etw->sgf);
  butCan_resizeWin(etw->mapWin->win, but_w(etw->sgfMap), but_h(etw->sgfMap),
		   TRUE);
  result = etw->mapCallback(etw->packet, etw->sgf->active->mapX);
  updateTreeButtons(etw);
  return(result);
}


static ButOut  movePressed(But *but)  {
  EditToolWin  *etw = but_packet(but);
  SgfElem  *active, *loSwap, *hiSwap, *prevChild;

  assert(MAGIC(etw));
  etw->modified = TRUE;
  active = etw->sgf->active;
  assert(active->childH != active->childT);
  assert(active->childH != NULL);
  hiSwap = active->activeChild;
  if (active->childH == hiSwap)  {
    loSwap = hiSwap;
    hiSwap = loSwap->sibling;
  } else  {
    for (loSwap = active->childH;
	 loSwap->sibling != hiSwap;
	 loSwap = loSwap->sibling)
      assert(MAGIC(loSwap));
  }
  if (active->childH == loSwap)  {
    active->childH = hiSwap;
  } else  {
    for (prevChild = active->childH;
	 prevChild->sibling != loSwap;
	 prevChild = prevChild->sibling);
    prevChild->sibling = hiSwap;
  }
  loSwap->sibling = hiSwap->sibling;
  hiSwap->sibling = loSwap;
  if (active->childT == hiSwap)
    active->childT = loSwap;
  sgfMap_remap(etw->sgfMap, etw->sgf);
  butCan_resizeWin(etw->mapWin->win, but_w(etw->sgfMap), but_h(etw->sgfMap),
		   TRUE);
  return(0);
}


void  editToolWin_nodeAdded(EditToolWin *etw, SgfElem *newNode)  {
  bool  addSuccessful;

  addSuccessful = sgfMap_newNode(etw->sgfMap, newNode);
  if (!addSuccessful)  {
    sgfMap_remap(etw->sgfMap, etw->sgf);
    butCan_resizeWin(etw->mapWin->win, but_w(etw->sgfMap), but_h(etw->sgfMap),
		     TRUE);
  }
  updateTreeButtons(etw);
}


ButOut  editToolWin_shiftUpPressed(But *but, bool press)  {
  EditToolWin  *etw = but_packet(but);

  assert(MAGIC(etw));
  if (press)  {
    if (sgfMap_changeVar(etw->sgfMap, sgfMap_prev))
      return(0);
    else
      return(BUTOUT_ERR);
  } else
    return(0);
}


ButOut  editToolWin_shiftDownPressed(But *but, bool press)  {
  EditToolWin  *etw = but_packet(but);

  assert(MAGIC(etw));
  if (press)  {
    if (sgfMap_changeVar(etw->sgfMap, sgfMap_next))
      return(0);
    else
      return(BUTOUT_ERR);
  } else
    return(0);
}


static void  updateTreeButtons(EditToolWin *etw)  {
  if (etw->sgf->active->childH)  {
    but_setFlags(etw->killNode, BUT_PRESSABLE);
    if (etw->sgf->active->childH == etw->sgf->active->childT)  {
      but_setFlags(etw->moveNode, BUT_NOPRESS);
    } else  {
      but_setFlags(etw->moveNode, BUT_PRESSABLE);
    }
  } else  {
    but_setFlags(etw->killNode, BUT_NOPRESS);
    but_setFlags(etw->moveNode, BUT_NOPRESS);
  }
}
