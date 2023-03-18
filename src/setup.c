/*
 * src/lsetup.c, part of Complete Goban (game program)
 * Copyright (C) 1995-1996 William Shubert
 * See "configure.h.in" for more copyright information.
 */


#include <wms.h>
#include <but/but.h>
#include <but/plain.h>
#include <but/menu.h>
#include <but/text.h>
#include <but/textin.h>
#include <but/ctext.h>
#include <but/checkbox.h>
#include <abut/msg.h>
#include <but/canvas.h>
#include <wms/clp.h>
#include <wms/str.h>
#include <but/menu.h>
#include <but/radio.h>
#include "cm2pm.h"
#include "msg.h"
#include "plasma.h"
#ifdef  _SETUP_H_
   Levelization ERror
#endif
#include "setup.h"


/**********************************************************************
 * Forward Declarations
 **********************************************************************/
static ButOut  unmap(ButWin *win);
static ButOut  map(ButWin *win);
static ButOut  resize(ButWin *win);
static ButOut  subwinResize(ButWin *win);
static ButOut  destroy(ButWin *win);
static ButOut  newComp(But *but, const char *val);
static ButOut  newPort(But *but, const char *val);
static ButOut  newConnCmd(But *but, const char *val);
static ButOut  newDirect(But *but, bool value);
static ButOut  newCoord(But *but, bool value);
static ButOut  newNumKibs(But *but, bool value);
static ButOut  newNoTypo(But *but, bool value);
static ButOut  ok(But *but);
static ButOut  newSrvNum(But *but, int value);
static ButOut  newName(But *but, const char *newVal);
static ButOut  newProto(But *but, int newVal);
static ButOut  newHiContrast(But *but, bool value);
static ButOut newWarnLimit(But *but, const char *val);

/**********************************************************************
 * Functions
 **********************************************************************/
#define  boxHeight(lines, fh, bw)  ((lines) * (2 * (fh) + (bw)) + 3 * (bw))
#define  textOff(line, fh, bw)  ((line) * (2 * (fh) + (bw)) + 2 * (bw))


Setup  *setup_create(Cgoban *cg,
		     void (*destroyCallback)(Setup *setup, void *packet),
		     void  (*newServerCallback)(Setup *setup, void *packet),
		     void *packet)  {
  Setup  *setup;
  bool  isDirect;
  bool  err;
  int  i;
  int  h, minH, bw;
  ButWin  *subwin;
  const char  *menuOpts[SETUP_MAXSERVERS + 1];
  char tmpWarnLimit[10];
  
  assert(MAGIC(cg));
  setup = wms_malloc(sizeof(Setup));
  MAGIC_SET(setup);
  setup->cg = cg;
  setup->destroyCallback = destroyCallback;
  setup->newServerCallback = newServerCallback;
  setup->packet = packet;
  bw = butEnv_stdBw(cg->env);
  minH = cg->fontH * 12 + bw * 16;
  h = (int)(minH * 2 * clp_getDouble(cg->clp, "config.h") + 0.5);
  setup->win = butWin_create(setup, cg->env, "Cgoban Setup", minH * 2, h,
			     unmap, map, resize, destroy);
  butWin_setMinH(setup->win, minH);
  butWin_setMaxH(setup->win, cg->fontH * 4 + bw * 8 +
		 boxHeight(8, cg->fontH, bw) + cg->fontH * 2 +
		 boxHeight(4, cg->fontH, bw));
  i = clp_iGetInt(cg->clp, "config.x", &err);
  if (!err)
    butWin_setX(setup->win, i);
  i = clp_iGetInt(cg->clp, "config.y", &err);
  if (!err)
    butWin_setY(setup->win, i);
  butWin_activate(setup->win);
  setup->bg = butBoxFilled_create(setup->win, 0, BUT_DRAWABLE);
  butBoxFilled_setPixmaps(setup->bg, cg->bgLitPixmap, cg->bgShadPixmap,
			  cg->bgPixmap);
  setup->title = butText_create(setup->win, 1, BUT_DRAWABLE,
				msg_setupTitle, butText_center);
  butText_setFont(setup->title, 2);
  setup->swin = abutSwin_create(setup, setup->win, 1, ABUTSWIN_LSLIDE,
				subwinResize);
  subwin = setup->swin->win;

  setup->srvBox = butBoxFilled_create(subwin, 0, BUT_DRAWABLE);
  setup->srvTitle = butText_create(subwin, 1, BUT_DRAWABLE,
				    msg_srvConfig,
				    butText_center);
  butText_setFont(setup->srvTitle, 2);
  setup->srvNum = 0;
  for (i = 0;  i < SETUP_MAXSERVERS;  ++i)  {
    menuOpts[i] = clp_getStrNum(cg->clp, "client.server", i);
  }
  menuOpts[i] = BUTMENU_OLEND;
  setup->srvMenu = butMenu_downCreate(newSrvNum, setup, subwin, 2, 3,
				      BUT_DRAWABLE|BUT_PRESSABLE,
				      msg_selServer, menuOpts, 0);
  setup->srvName = butTextin_create(newName, setup, subwin, 1,
				    BUT_DRAWABLE|BUT_PRESSABLE,
				    clp_getStrNum(cg->clp, "client.server",
						  setup->srvNum), 100);
  setup->srvNameLabel = butText_create(subwin, 1, BUT_DRAWABLE,
				       msg_setupSrvName,
				       butText_left);
  setup->srvProto = butRadio_create(newProto, setup, subwin, 1,
				    BUT_DRAWABLE|BUT_PRESSABLE,
				    (clp_getStrNum(cg->clp, "client.protocol",
						   setup->srvNum)[0] == 'n'),
				    2);
  setup->srvProtoLabel = butText_create(subwin, 1, BUT_DRAWABLE,
					msg_setupProtocol,
					butText_left);
  setup->igsLabel = butText_create(subwin, 2, BUT_DRAWABLE|BUT_PRESSTHRU,
				   "IGS", butText_center);
  setup->nngsLabel = butText_create(subwin, 2, BUT_DRAWABLE|BUT_PRESSTHRU,
				    "NNGS", butText_center);
  isDirect = (clp_getStrNum(cg->clp, "client.direct",
			    setup->srvNum)[0] == 't');
  setup->srvDirectLabel = butText_create(subwin, 1, BUT_DRAWABLE,
					 msg_directConn,
					 butText_left);
  setup->srvDirect = butCb_create(newDirect, setup, subwin, 1,
				  BUT_DRAWABLE|BUT_PRESSABLE, isDirect);
  setup->srvComp = butText_create(subwin, 1, BUT_DRAWABLE,
				  msg_gsCompName, butText_left);
  setup->srvCompIn =
    butTextin_create(newComp, setup, subwin, 1, BUT_DRAWABLE,
		     clp_getStrNum(cg->clp, "client.address", setup->srvNum),
		     100);
  setup->srvPort = butText_create(subwin, 1, BUT_DRAWABLE,
				  msg_gsPortNum, butText_left);
  setup->srvPortIn =
    butTextin_create(newPort, setup, subwin, 1, BUT_DRAWABLE,
		     clp_getStrNum(cg->clp, "client.port", setup->srvNum), 20);
  setup->srvCmdLabel = butText_create(subwin, 1, BUT_DRAWABLE,
				      msg_connCmdLabel,
				      butText_left);
  setup->srvCmd = butTextin_create(newConnCmd, setup, subwin, 1,
				   BUT_DRAWABLE,
				   clp_getStrNum(cg->clp, "client.connCmd",
						 setup->srvNum), 100);
  newDirect(setup->srvDirect, isDirect);

  setup->miscBox = butBoxFilled_create(subwin, 0, BUT_DRAWABLE);
  setup->miscTitle = butText_create(subwin, 1, BUT_DRAWABLE,
				    msg_miscellaneous,
				    butText_center);
  butText_setFont(setup->miscTitle, 2);
  setup->coordLabel = butText_create(subwin, 1, BUT_DRAWABLE,
				      msg_showCoordinates,
				      butText_left);
  setup->coord = butCb_create(newCoord, setup, subwin, 1,
			      BUT_DRAWABLE|BUT_PRESSABLE,
			      clp_getBool(cg->clp, "board.showCoords"));
  setup->hiLabel = butText_create(subwin, 1, BUT_DRAWABLE,
				  msg_hiContrast,
				  butText_left);
  setup->hi = butCb_create(newHiContrast, setup, subwin, 1,
			   BUT_DRAWABLE|BUT_PRESSABLE,
			   clp_getBool(cg->clp, "hiContrast"));
  setup->numKibsLabel = butText_create(subwin, 1, BUT_DRAWABLE,
				       msg_numberKibitzes,
				       butText_left);
  setup->numKibs = butCb_create(newNumKibs, setup, subwin, 1,
				BUT_DRAWABLE|BUT_PRESSABLE,
				clp_getBool(cg->clp, "client.numberKibitz"));

  setup->noTypoLabel = butText_create(subwin, 1, BUT_DRAWABLE,
				      msg_noTypo, butText_left);
  setup->noTypo = butCb_create(newNoTypo, setup, subwin, 1,
			       BUT_DRAWABLE|BUT_PRESSABLE,
			       clp_getBool(cg->clp, "client.noTypo"));

  setup->warnLabel = butText_create(subwin, 1, BUT_DRAWABLE,
				    msg_warnLimit, butText_left);
  i = clp_getInt(cg->clp, "client.warnLimit");
  sprintf(tmpWarnLimit, "%d", i);
  setup->warnLimit = butTextin_create(newWarnLimit, setup, subwin, 1,
				      BUT_DRAWABLE|BUT_PRESSABLE,
				      tmpWarnLimit, 10);

  setup->help = butCt_create(cgoban_createHelpWindow, &help_configure,
			     setup->win, 1,
			     BUT_DRAWABLE|BUT_PRESSABLE, msg_help);
  setup->ok = butCt_create(ok, setup, setup->win, 1,
			   BUT_DRAWABLE|BUT_PRESSABLE, msg_ok);
			     
  return(setup);
}


void  setup_destroy(Setup *setup, bool propagate)  {
  assert(MAGIC(setup));
  if (propagate)
    setup->destroyCallback(setup, setup->packet);
  if (setup->swin)
    abutSwin_destroy(setup->swin);
  if (setup->win)
    butWin_destroy(setup->win);
  MAGIC_UNSET(setup);
  wms_free(setup);
}


static ButOut  unmap(ButWin *win)  {
  return(0);
}


static ButOut  map(ButWin *win)  {
  return(0);
}


static ButOut  resize(ButWin *win)  {
  ButEnv  *env = butWin_env(win);
  Setup  *setup = butWin_packet(win);
  int  y, w, h;
  int  fontH, bw;

  assert(MAGIC(setup));
  w = butWin_w(win);
  h = butWin_h(win);
  fontH = setup->cg->fontH;
  bw = butEnv_stdBw(env);
  but_resize(setup->bg, 0, y = 0, w, h);
  but_resize(setup->title, bw*2, y += bw*2, w - bw*4, fontH * 2);

  abutSwin_resize(setup->swin, bw*2, y += fontH * 2 + bw,
		  w - bw*4, h - fontH * 4 - bw * 6,
		  (butEnv_fontH(env, 0) * 3) / 2,
		  butEnv_fontH(env, 0));

  but_resize(setup->help, bw * 2, h - (fontH * 2 + bw * 2),
	     (w - bw * 5) / 2, fontH * 2);
  but_resize(setup->ok, (w + bw) / 2, h - (fontH * 2 + bw * 2),
	     w - bw*2 - (w + bw) / 2, fontH * 2);
	     
  return(0);
}


static ButOut  subwinResize(ButWin *win)  {
  AbutSwin  *swin;
  ButEnv  *env = butWin_env(win);
  Setup  *setup;
  int  y, w;
  int  fontH, bw;

  swin = butWin_packet(win);
  assert(MAGIC(swin));
  setup = swin->packet;
  assert(MAGIC(setup));
  w = butWin_w(win);
  fontH = setup->cg->fontH;
  bw = butEnv_stdBw(env);

  y = 0;
  but_resize(setup->srvBox, 0, y, w, boxHeight(8, fontH, bw) + fontH*2);
  but_resize(setup->srvTitle, bw*2, y + textOff(0, fontH, bw),
	     w - bw*4, fontH * 2);
  but_resize(setup->srvMenu, bw * 2, y + textOff(1, fontH, bw),
	     w - bw * 4, fontH * 4);
  but_resize(setup->srvNameLabel, bw * 2,
	     y + textOff(2, fontH, bw) + fontH * 2, w/2 - bw*2, fontH * 2);
  but_resize(setup->srvName, w/2, y + textOff(2, fontH, bw) + fontH * 2,
	     w - (w/2 + bw*2), fontH * 2);
  but_resize(setup->srvProtoLabel, bw * 2,
	     y + textOff(3, fontH, bw) + fontH * 2, w/2 - bw*2, fontH * 2);
  but_resize(setup->srvProto, w/2, y + textOff(3, fontH, bw) + fontH * 2,
	     w - (w/2 + bw*2), fontH * 2);
  butText_resize(setup->igsLabel, w/2 + (w - (w/2 + bw*2))/4,
		 y + textOff(3, fontH, bw) + fontH * 2, fontH * 2);
  butText_resize(setup->nngsLabel,
		 w/2 + (w - (w/2 + bw*2)) - (w - (w/2 + bw*2))/4,
		 y + textOff(3, fontH, bw) + fontH * 2, fontH * 2);
  but_resize(setup->srvDirectLabel, bw * 2,
	     y + textOff(4, fontH, bw) + fontH*2,
	     w - (bw * 4 + fontH * 2), fontH * 2);
  but_resize(setup->srvDirect,
	     w - (bw*2 + fontH * 2), y + textOff(4, fontH, bw) + fontH * 2,
	     fontH * 2, fontH * 2);
  but_resize(setup->srvComp, bw * 2,
	     y + textOff(5, fontH, bw) + fontH * 2, w/2 - bw*2, fontH * 2);
  but_resize(setup->srvCompIn, w/2, y + textOff(5, fontH, bw) + fontH * 2,
	     w - (w/2 + bw*2), fontH * 2);
  but_resize(setup->srvPort, bw * 2, y + textOff(6, fontH, bw) + fontH * 2,
	     w/2 - bw*2, fontH * 2);
  but_resize(setup->srvPortIn, w/2, y + textOff(6, fontH, bw) + fontH * 2,
	     w - (w/2 + bw*2), fontH * 2);
  but_resize(setup->srvCmdLabel, bw * 2, y + textOff(7, fontH, bw) + fontH * 2,
	     w/2 - bw*2, fontH * 2);
  but_resize(setup->srvCmd, w/2, y + textOff(7, fontH, bw) + fontH * 2,
	     w - (w/2 + bw*2), fontH * 2);

  y += boxHeight(8, fontH, bw) + fontH * 2;
  but_resize(setup->miscBox, 0, y, w, boxHeight(6, fontH, bw));
  but_resize(setup->miscTitle, bw*2, y + textOff(0, fontH, bw),
	     w - bw*4, fontH * 2);
  but_resize(setup->coordLabel, bw * 2, y + textOff(1, fontH, bw),
	     w - (bw * 4 + fontH * 2), fontH * 2);
  but_resize(setup->coord,
	     w - (bw*2 + fontH * 2), y + textOff(1, fontH, bw),
	     fontH * 2, fontH * 2);
  but_resize(setup->hiLabel, bw * 2, y + textOff(2, fontH, bw),
	     w - (bw * 4 + fontH * 2), fontH * 2);
  but_resize(setup->hi,
	     w - (bw*2 + fontH * 2), y + textOff(2, fontH, bw),
	     fontH * 2, fontH * 2);
  but_resize(setup->numKibsLabel, bw * 2, y + textOff(3, fontH, bw),
	     w - (bw * 4 + fontH * 2), fontH * 2);
  but_resize(setup->numKibs,
	     w - (bw*2 + fontH * 2), y + textOff(3, fontH, bw),
	     fontH * 2, fontH * 2);
  but_resize(setup->noTypoLabel, bw * 2, y + textOff(4, fontH, bw),
	     w - (bw * 4 + fontH * 2), fontH * 2);
  but_resize(setup->noTypo,
	     w - (bw*2 + fontH * 2), y + textOff(4, fontH, bw),
	     fontH * 2, fontH * 2);
  but_resize(setup->warnLabel, bw * 2, y + textOff(5, fontH, bw),
	     w - (bw * 4 + fontH * 2), fontH * 2);
  but_resize(setup->warnLimit,
	     w/2, y + textOff(5, fontH, bw),
	     w - (w/2 + bw*2), fontH * 2);

  y += boxHeight(6, fontH, bw);
  if (y != butWin_h(win))  {
    butCan_resizeWin(win, butWin_w(win), y, TRUE);
  }

  return(0);
}


static ButOut  destroy(ButWin *win)  {
  Setup  *setup = butWin_packet(win);

  assert(MAGIC(setup));
  clp_setInt(setup->cg->clp, "config.x", butWin_x(win));
  clp_setInt(setup->cg->clp, "config.y", butWin_y(win));
  clp_setDouble(setup->cg->clp, "config.h",
		(double)butWin_h(win) / (double)butWin_w(win));
  setup->win = NULL;  /* It's dead already. */
  setup_destroy(setup, TRUE);
  return(0);
}


static ButOut  newComp(But *but, const char *val)  {
  Setup  *setup = but_packet(but);

  assert(MAGIC(setup));
  clp_setStrNum(setup->cg->clp, "client.address", val, setup->srvNum);
  but_setFlags(but, BUT_NOKEY);
  return(0);
}


static ButOut  newPort(But *but, const char *val)  {
  Setup  *setup = but_packet(but);
  ButOut  result = 0;
  Str  errorMsg;
  int  test;
  bool  err;

  assert(MAGIC(setup));
  err = FALSE;
  test = wms_atoi(val, &err);
  if ((test < 0) || (test > 65535) || err)  {
    str_init(&errorMsg);
    str_print(&errorMsg, msg_clientBadPortNum, val);
    cgoban_createMsgWindow(setup->cg, "Cgoban Error", str_chars(&errorMsg));
    str_deinit(&errorMsg);
    result = BUTOUT_ERR;
  } else
    clp_setStrNum(setup->cg->clp, "client.port", val, setup->srvNum);
  but_setFlags(but, BUT_NOKEY);
  return(result);
}


static ButOut  ok(But *but)  {
  Setup  *setup = but_packet(but);
  ButOut  result = 0;

  assert(MAGIC(setup));
  result |= newComp(setup->srvCompIn, butTextin_get(setup->srvCompIn));
  result |= newPort(setup->srvPortIn, butTextin_get(setup->srvPortIn));
  result |= newConnCmd(setup->srvCmd, butTextin_get(setup->srvCmd));
  result |= newName(setup->srvName, butTextin_get(setup->srvName));
  newWarnLimit(setup->warnLimit, butTextin_get(setup->warnLimit));
  if (!(result & BUTOUT_ERR))  {
    butWin_destroy(but_win(but));
  }
  return(result);
}


static ButOut  newConnCmd(But *but, const char *val)  {
  Setup  *setup = but_packet(but);

  assert(MAGIC(setup));
  but_setFlags(but, BUT_NOKEY);
  clp_setStrNum(setup->cg->clp, "client.connCmd", val, setup->srvNum);
  return(0);
}


static ButOut  newDirect(But *but, bool value)  {
  Setup  *setup = but_packet(but);
  uint  directFlags;

  assert(MAGIC(setup));
  if (value)
    directFlags = BUT_DRAWABLE | BUT_PRESSABLE;
  else
    directFlags = BUT_DRAWABLE | BUT_NOPRESS | BUT_NOKEY;
  clp_setStrNum(setup->cg->clp, "client.direct", (value ? "t" : "f"),
		setup->srvNum);
  butText_setColor(setup->srvComp, BUT_FG, !value);
  but_setFlags(setup->srvCompIn, directFlags);
  butText_setColor(setup->srvPort, BUT_FG, !value);
  but_setFlags(setup->srvPortIn, directFlags);
  butText_setColor(setup->srvCmdLabel, BUT_FG, value);
  but_setFlags(setup->srvCmd, directFlags ^
	       (BUT_PRESSABLE|BUT_NOPRESS|BUT_NOKEY));
  return(0);
}


static ButOut  newCoord(But *but, bool value)  {
  Setup  *setup = but_packet(but);

  assert(MAGIC(setup));
  clp_setBool(setup->cg->clp, "board.showCoords", value);
  setup->cg->showCoords = value;
  butEnv_resizeAll(butWin_env(but_win(but)));
  return(0);
}


static ButOut  newHiContrast(But *but, bool value)  {
  Setup  *setup = but_packet(but);

  assert(MAGIC(setup));
  clp_setBool(setup->cg->clp, "hiContrast", value);
  setup->cg->cgbuts.hiContrast = value;
  cm2OldPm(setup->cg->env,
	   plasma(),
	   PLASMA_SIZE,PLASMA_SIZE,
	   CGBUTS_COLORBOARD(0), 256,
	   setup->cg->boardPixmap, value);
  cgbuts_redraw(&setup->cg->cgbuts);
  return(0);
}


static ButOut  newNumKibs(But *but, bool value)  {
  Setup  *setup = but_packet(but);

  assert(MAGIC(setup));
  clp_setBool(setup->cg->clp, "client.numberKibitz", value);
  return(0);
}


static ButOut  newNoTypo(But *but, bool value)  {
  Setup  *setup = but_packet(but);

  assert(MAGIC(setup));
  clp_setBool(setup->cg->clp, "client.noTypo", value);
  setup->cg->cgbuts.holdEnabled = value;
  return(0);
}


static ButOut  newProto(But *but, int newVal)  {
  Setup  *setup = but_packet(but);
  static const char  *protos[] = {"i", "n"};

  assert(MAGIC(setup));
  clp_setStrNum(setup->cg->clp, "client.protocol",
		protos[newVal], setup->srvNum);
  return(0);
}


static ButOut  newName(But *but, const char *newVal)  {
  Setup  *setup = but_packet(but);

  assert(MAGIC(setup));
  but_setFlags(but, BUT_NOKEY);
  clp_setStrNum(setup->cg->clp, "client.server", newVal, setup->srvNum);
  butMenu_setOptionName(setup->srvMenu, newVal, setup->srvNum);
  setup->newServerCallback(setup, setup->packet);
  return(0);
}


static ButOut  newSrvNum(But *but, int value)  {
  Setup  *setup = but_packet(but);
  ButOut  result = 0;
  Clp  *clp = setup->cg->clp;

  result |= newComp(setup->srvCompIn, butTextin_get(setup->srvCompIn));
  result |= newPort(setup->srvPortIn, butTextin_get(setup->srvPortIn));
  result |= newConnCmd(setup->srvCmd, butTextin_get(setup->srvCmd));
  result |= newName(setup->srvName, butTextin_get(setup->srvName));
  setup->srvNum = value;
  butTextin_set(setup->srvName, clp_getStrNum(clp, "client.server", value),
		FALSE);
  butRadio_set(setup->srvProto, (clp_getStrNum(clp, "client.protocol",
					       value)[0] == 'n'), FALSE);
  butCb_set(setup->srvDirect, (clp_getStrNum(clp, "client.direct",
					    value)[0] == 't'), TRUE);
  butTextin_set(setup->srvCompIn, clp_getStrNum(clp, "client.address", value),
		FALSE);
  butTextin_set(setup->srvPortIn, clp_getStrNum(clp, "client.port", value),
		FALSE);
  butTextin_set(setup->srvCmd, clp_getStrNum(clp, "client.connCmd", value),
		FALSE);
  return(result);
}

static ButOut newWarnLimit(But *but, const char *val) {
  Setup *setup = but_packet(but);
  int newVal;
  Clp *clp = setup->cg->clp;

  newVal = atoi(val);
  setup->cg->cgbuts.timeWarn = newVal;
  clp_setInt(clp, "client.warnLimit", newVal);
  return(0);
}
