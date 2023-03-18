/*
 * src/control.c, part of Complete Goban (game program)
 * Copyright (C) 1995 William Shubert
 * See "configure.h.in" for more copyright information.
 */

#include <wms.h>
#include <but/but.h>
#include <but/plain.h>
#include <but/ctext.h>
#include <wms/clp.h>
#include <abut/fsel.h>
#include <abut/msg.h>
#include <wms/str.h>
#include <but/text.h>
#include <but/textin.h>
#include "cgoban.h"
#include "msg.h"
#include "control.h"
#include "gameSetup.h"
#include "local.h"
#include "cgbuts.h"
#include "editBoard.h"
#include "gmp/setup.h"
#include "client/setup.h"
#include "help.h"


/**********************************************************************
 * Forward declarations
 **********************************************************************/
static ButOut  map(ButWin *win);
static ButOut  resize(ButWin *win);
static ButOut  iResize(ButWin *win);
static ButOut  destroy(ButWin *win);
static ButOut  localGoban(But *but);
static ButOut  loadLocalGoban(But *but);
static void  gotLoadName(AbutFsel *fsel, void *packet, const char *fname);
static ButOut  quit(But *but);
static ButOut  setup(But *but);
static ButOut  editPressed(But *but);
static ButOut  gmpPressed(But *but);
static void  editFileName(AbutFsel *fsel, void *packet, const char *fName);
static void  localReady(void *packet, GameSetup *gs);
static ButOut  serverPressed(But *but);
static void  setupGone(Setup *setup, void *packet);
static void  updateServerBut(void *packet, int butNum);
static void  serverChange(Setup *setup, void *packet);


/**********************************************************************
 * Functions
 **********************************************************************/

#define  WIN_W(h,bw)  ((h)*18+(bw)*18)  /* I really want this to be square, */
#define  WIN_H(h,bw)  ((h)*18+(bw)*17)  /*   but it just won't work out.    */

/*
 * Layout:
 *
 * 2*bw
 * Text * 2
 * <BUT> = Text*6+bw*4
 * bw
 * <BUT> = Text*6+bw*4
 * Text * 2
 * bw * 4
 * <but> = Text*2
 * 2*bw
 *
 * Total: Text * 18 + bw * 17
 */
Control  *control_create(Cgoban *cg, bool iconic)  {
  Control  *ctrl;
  int  i, serverNum;
  bool  err;

  assert(MAGIC(cg));
  ctrl = wms_malloc(sizeof(Control));
  MAGIC_SET(ctrl);
  if (!iconic)
    cg->env->minWindows = 1;
  ctrl->cg = cg;
  ctrl->win = butWin_iCreate(ctrl, cg->env, "CGoban Control",
			     WIN_W(cg->fontH, butEnv_stdBw(cg->env)),
			     WIN_H(cg->fontH, butEnv_stdBw(cg->env)),
			     &ctrl->iWin, iconic, 48,48,
			     NULL, map, resize, iResize, destroy);
  i = clp_iGetInt(cg->clp, "control.x", &err);
  if (!err)
    butWin_setX(ctrl->win, i);
  i = clp_iGetInt(cg->clp, "control.y", &err);
  if (!err)
    butWin_setY(ctrl->win, i);
  butWin_activate(ctrl->win);
  ctrl->box = butBoxFilled_create(ctrl->win, 0, BUT_DRAWABLE);
  butBoxFilled_setPixmaps(ctrl->box, cg->bgLitPixmap, cg->bgShadPixmap,
			  cg->bgPixmap);

  serverNum = clp_getInt(cg->clp, "client.def1");
  ctrl->servers[0] = butCt_create(serverPressed, ctrl, ctrl->win, 1,
				  BUT_PRESSABLE|BUT_DRAWABLE,
				  clp_getStrNum(cg->clp, "client.server",
						serverNum));
  ctrl->sPics[0] = grid_create(&cg->cgbuts, NULL, NULL, ctrl->win, 2,
			       BUT_DRAWABLE|BUT_PRESSTHRU, 0);
  grid_setStone(ctrl->sPics[0], goStone_white, FALSE);
  if (clp_getStrNum(cg->clp, "client.protocol", serverNum)[0] == 'n')
    grid_setVersion(ctrl->sPics[0], CGBUTS_WORLDWEST(0));
  else
    grid_setVersion(ctrl->sPics[0], CGBUTS_WORLDEAST(0));
  grid_setLineGroup(ctrl->sPics[0], gridLines_none);

  serverNum = clp_getInt(cg->clp, "client.def2");
  ctrl->servers[1] = butCt_create(serverPressed, ctrl, ctrl->win, 1,
				  BUT_PRESSABLE|BUT_DRAWABLE,
				  clp_getStrNum(cg->clp, "client.server",
						serverNum));
  ctrl->sPics[1] = grid_create(&cg->cgbuts, NULL, NULL, ctrl->win, 2,
			       BUT_DRAWABLE|BUT_PRESSTHRU, 0);
  grid_setStone(ctrl->sPics[1], goStone_white, FALSE);
  grid_setVersion(ctrl->sPics[1], CGBUTS_WORLDEAST(1));
  grid_setLineGroup(ctrl->sPics[1], gridLines_none);

  ctrl->lGame = butCt_create(localGoban, ctrl, ctrl->win, 1,
			     BUT_DRAWABLE|BUT_PRESSABLE, msg_newGame);
  ctrl->lGamePic = grid_create(&cg->cgbuts, NULL, NULL, ctrl->win, 2,
			       BUT_DRAWABLE|BUT_PRESSTHRU, 0);
  grid_setStone(ctrl->lGamePic, goStone_white, FALSE);
  grid_setVersion(ctrl->lGamePic, 2);
  grid_setLineGroup(ctrl->lGamePic, gridLines_none);

  ctrl->lLoad = butCt_create(loadLocalGoban, ctrl, ctrl->win, 1,
			    BUT_DRAWABLE|BUT_PRESSABLE, msg_loadGame);
  ctrl->lLoadPic = grid_create(&cg->cgbuts, NULL, NULL, ctrl->win, 2,
			       BUT_DRAWABLE|BUT_PRESSTHRU, 0);
  grid_setStone(ctrl->lLoadPic, goStone_black, FALSE);
  grid_setLineGroup(ctrl->lLoadPic, gridLines_none);

  ctrl->edit = butCt_create(editPressed, ctrl, ctrl->win, 1,
			    BUT_DRAWABLE|BUT_PRESSABLE, msg_editSGF);
  ctrl->editPic = toolPic_create(&cg->cgbuts, ctrl->win, 2,
				 BUT_DRAWABLE|BUT_PRESSTHRU);

  ctrl->gmp = butCt_create(gmpPressed, ctrl, ctrl->win, 1,
			   BUT_DRAWABLE|BUT_PRESSABLE, msg_goModem);
  ctrl->gmpPic = computerPic_create(&cg->cgbuts, ctrl->win, 2,
				    BUT_DRAWABLE|BUT_PRESSTHRU);

  ctrl->sBox = butBoxFilled_create(ctrl->win, 0, BUT_DRAWABLE);
  butBoxFilled_setPixmaps(ctrl->sBox, cg->bgLitPixmap, cg->bgShadPixmap,
			  cg->bgPixmap);
  ctrl->setupWin = NULL;
  ctrl->help = butCt_create(cgoban_createHelpWindow, &help_control,
			    ctrl->win, 1,
			    BUT_DRAWABLE|BUT_PRESSABLE, msg_help);
  ctrl->setup = butCt_create(setup, ctrl, ctrl->win, 1,
			     BUT_DRAWABLE|BUT_PRESSABLE,
			     msg_setup);
  ctrl->quit = butCt_create(quit, ctrl, ctrl->win, 1,
			    BUT_DRAWABLE|BUT_PRESSABLE, msg_quit);

  ctrl->iBg = butBoxFilled_create(ctrl->iWin, 0, BUT_DRAWABLE);
  butBoxFilled_setPixmaps(ctrl->iBg, cg->bgLitPixmap, cg->bgShadPixmap,
			  cg->bgPixmap);
  ctrl->ig = grid_create(&cg->cgbuts, NULL, NULL, ctrl->iWin, 1,
			 BUT_DRAWABLE, 0);
  grid_setStone(ctrl->ig, goStone_black, FALSE);
  grid_setLineGroup(ctrl->ig, gridLines_none);
  grid_setVersion(ctrl->ig, CGBUTS_YINYANG(2));

  return(ctrl);
}


static ButOut  map(ButWin *win)  {
  butWin_env(win)->minWindows = 1;
  return(0);
}


static ButOut  resize(ButWin *win)  {
  Control  *ctrl = butWin_packet(win);
  ButEnv  *env;
  int  bw, fontH;
  int  x, y, w, h, tbH, totalH, boxH;

  assert(MAGIC(ctrl));
  env = ctrl->cg->env;
  bw = butEnv_stdBw(env);
  fontH = ctrl->cg->fontH;

  x = bw * 2;
  y = bw * 2;
  w = fontH*6 + bw*4;
  h = fontH*8 + bw*4;
  tbH = fontH*2;
  totalH = h + bw;

  boxH = totalH * 2 + bw*3;
  but_resize(ctrl->box, 0,0, butWin_w(win), boxH);
  but_resize(ctrl->sBox, 0,boxH, butWin_w(win), butWin_h(win) - boxH);

  but_resize(ctrl->servers[0], x, y, w, h);
  butCt_setTextLoc(ctrl->servers[0], 0, 0, w, tbH);
  but_resize(ctrl->sPics[0], x+bw*2,y+tbH+bw*2, w-bw*4,w-bw*4);

  y += totalH;
  but_resize(ctrl->servers[1], x, y, w, h);
  but_resize(ctrl->sPics[1], x+bw*2,y+bw*2, w-bw*4,w-bw*4);
  butCt_setTextLoc(ctrl->servers[1], 0, h-tbH, w, tbH);

  x += w + bw;
  y -= totalH;
  but_resize(ctrl->lGame, x, y, w, h);
  butCt_setTextLoc(ctrl->lGame, 0, 0, w, tbH);
  but_resize(ctrl->lGamePic, x+bw*2,y+tbH+bw*2, w-bw*4,w-bw*4);

  y += totalH;
  but_resize(ctrl->lLoad, x, y, w, h);
  butCt_setTextLoc(ctrl->lLoad, 0, h-tbH, w, tbH);
  but_resize(ctrl->lLoadPic, x+bw*2, y+bw*2, w-bw*4, w-bw*4);

  x += w + bw;
  y -= totalH;
  but_resize(ctrl->edit, x, y, w, h);
  butCt_setTextLoc(ctrl->edit, 0, 0, w, tbH);
  but_resize(ctrl->editPic, x+bw*2,y+tbH+bw*2, w-bw*4,w-bw*4);

  y += totalH;
  but_resize(ctrl->gmp, x, y, w, h);
  butCt_setTextLoc(ctrl->gmp, 0, h-tbH, w, tbH);
  but_resize(ctrl->gmpPic, x+bw*2, y+bw*2, w-bw*4, w-bw*4);

  y += totalH + bw*3;
  x = bw*2;
  but_resize(ctrl->help, x, y, w, tbH);
  but_resize(ctrl->setup, x + w + bw, y, w, tbH);
  but_resize(ctrl->quit, x + w*2 + bw*2, y, w, tbH);

  return(0);
}


static ButOut  iResize(ButWin *win)  {
  Control  *ctrl = butWin_packet(win);
  int  w, h, size;
  int  border;

  assert(MAGIC(ctrl));
  w = butWin_w(win);
  h = butWin_h(win);
  but_resize(ctrl->iBg, 0,0, w,h);
  border = 2*butEnv_stdBw(butWin_env(win));
  size = w - border * 2;
  but_resize(ctrl->ig, border, border, size, size);
  return(0);
}


static ButOut  destroy(ButWin *win)  {
  Control  *ctrl = butWin_packet(win);

  assert(MAGIC(ctrl));
  clp_setInt(ctrl->cg->clp, "control.x", butWin_x(win));
  clp_setInt(ctrl->cg->clp, "control.y", butWin_y(win));
  return(BUTOUT_STOPWAIT);
}


static ButOut  localGoban(But *but)  {
  Control  *ctrl = but_packet(but);

  assert(MAGIC(ctrl));
  gameSetup_create(ctrl->cg, localReady, ctrl);
  return(0);
}


static void  localReady(void *packet, GameSetup *gs)  {
  Control  *ctrl = packet;
  
  assert(MAGIC(ctrl));
  if (gs != NULL)  {
    local_create(ctrl->cg, gameSetup_rules(gs), gameSetup_size(gs),
		 gameSetup_handicap(gs), gameSetup_komi(gs),
		 gameSetup_wName(gs), gameSetup_bName(gs),
		 gameSetup_timeType(gs), gameSetup_mainTime(gs),
		 gameSetup_byTime(gs), gameSetup_timeAux(gs));
  }
}


static ButOut  loadLocalGoban(But *but)  {
  Control  *c = but_packet(but);

  assert(MAGIC(c));
  abutFsel_createDir(c->cg->abut, gotLoadName, c, "CGoban",
		     msg_loadGameName,
		     str_chars(&c->cg->lastDirAccessed),
		     clp_getStr(c->cg->clp, "local.sgfName"));
  return(0);
}


static void  gotLoadName(AbutFsel *fsel, void *packet, const char *fname)  {
  Control  *c = packet;

  assert(MAGIC(c));
  str_copy(&c->cg->lastDirAccessed, &fsel->pathVal);
  if (fname != NULL)  {
    clp_setStr(c->cg->clp, "local.sgfName", butTextin_get(fsel->in));
    local_createFile(c->cg, fname);
  }
}


static ButOut  quit(But *but)  {
  Control  *ctrl = but_packet(but);
  Clp  *clp;

  assert(MAGIC(ctrl));
  clp = ctrl->cg->clp;
  clp_setInt(clp, "control.x", butWin_x(ctrl->win));
  clp_setInt(clp, "control.y", butWin_y(ctrl->win));
  clp_wFile(clp, clp_getStr(clp, "adfile"), clp_getStr(clp, "name"));
  return(BUTOUT_STOPWAIT);
}


static ButOut  setup(But *but)  {
  Control  *ctrl = but_packet(but);

  assert(MAGIC(ctrl));
  if (ctrl->setupWin)  {
    XRaiseWindow(butEnv_dpy(ctrl->cg->env), butWin_xwin(ctrl->setupWin->win));
  } else  {
    ctrl->setupWin = setup_create(ctrl->cg, setupGone, serverChange, ctrl);
  }
  return(0);
}


static void  setupGone(Setup *setup, void *packet)  {
  Control  *ctrl = packet;

  assert(MAGIC(ctrl));
  ctrl->setupWin = NULL;
}


static ButOut  editPressed(But *but)  {
  Control  *ctrl = but_packet(but);

  assert(MAGIC(ctrl));
  assert(MAGIC(ctrl->cg));
  abutFsel_createDir(ctrl->cg->abut, editFileName, ctrl, "CGoban",
		     msg_editGameName,
		     str_chars(&ctrl->cg->lastDirAccessed),
		     clp_getStr(ctrl->cg->clp, "edit.sgfName"));
  return(0);
}


static void  editFileName(AbutFsel *fsel, void *packet, const char *fName)  {
  Control  *ctrl = packet;

  assert(MAGIC(ctrl));
  str_copy(&ctrl->cg->lastDirAccessed, &fsel->pathVal);
  if (fName)  {
    clp_setStr(ctrl->cg->clp, "edit.sgfName", butTextin_get(fsel->in));
    editBoard_create(ctrl->cg, fName);
  }
}


static ButOut  gmpPressed(But *but)  {
  Control  *ctrl = but_packet(but);

  gmpSetup_create(ctrl->cg);
  return(0);
}


static ButOut  serverPressed(But *but)  {
  Control  *ctrl = but_packet(but);
  int  butNum, serverNum;
  ClpEntry  *sEntry, *sNames;

  assert(MAGIC(ctrl));
  if (but == ctrl->servers[0])  {
    butNum = 0;
    sEntry = clp_lookup(ctrl->cg->clp, "client.def1");
  } else  {
    assert(but == ctrl->servers[1]);
    butNum = 1;
    sEntry = clp_lookup(ctrl->cg->clp, "client.def2");
  }
  serverNum = clpEntry_getInt(sEntry);
  if (butEnv_keyModifiers(ctrl->cg->env) & ShiftMask)  {
    /* Change which server this is. */
    sNames = clp_lookup(ctrl->cg->clp, "client.server");
    do  {
      ++serverNum;
      if (serverNum == SETUP_MAXSERVERS)  {
	serverNum = 0;
      }
    } while ((serverNum > 0) &&
	     (clpEntry_getStrNum(sNames, serverNum)[0] == '\0'));
    clpEntry_setInt(sEntry, serverNum);
    updateServerBut(ctrl, butNum);
  } else  {
    /* Start up a connection to the server. */
    cliSetup_create(ctrl->cg, serverNum);
  }
  return(0);
}


static void  serverChange(Setup *setup, void *packet)  {
  updateServerBut(packet, 0);
  updateServerBut(packet, 1);
}


static void  updateServerBut(void *packet, int butNum)  {
  Control *ctrl = packet;
  int  sNum;

  assert(MAGIC(ctrl));
  assert((butNum == 0) || (butNum == 1));
  if (butNum == 0)  {
    sNum = clp_getInt(ctrl->cg->clp, "client.def1");
  } else  {
    sNum = clp_getInt(ctrl->cg->clp, "client.def2");
  }
  assert((sNum >= 0) && (sNum < SETUP_MAXSERVERS));
  butCt_setText(ctrl->servers[butNum],
		clp_getStrNum(ctrl->cg->clp, "client.server", sNum));
  if (clp_getStrNum(ctrl->cg->clp, "client.protocol", sNum)[0] == 'n')
    grid_setVersion(ctrl->sPics[butNum], CGBUTS_WORLDWEST(0));
  else
    grid_setVersion(ctrl->sPics[butNum], CGBUTS_WORLDEAST(0));
}
