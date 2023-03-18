/*
 * src/gmp/setup.c, part of Complete Goban (game program)
 * Copyright (C) 1995 William Shubert
 * See "configure.h.in" for more copyright information.
 */


#include <termios.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <wms.h>
#include <but/but.h>
#include <but/plain.h>
#include <but/menu.h>
#include <but/text.h>
#include <but/textin.h>
#include <but/ctext.h>
#include <abut/msg.h>
#include <wms/clp.h>
#include <wms/str.h>
#include "../cgoban.h"
#include "setup.h"
#include "client.h"
#include "../gameSetup.h"
#include "../msg.h"
#include "play.h"
#include <but/checkbox.h>


/**********************************************************************
 * Global variables
 **********************************************************************/

/*
 * If this field is TRUE, then we will really, truly be speaking GMP to
 *   the player in question.
 */
static const bool  isGmpType[] = {
  TRUE, TRUE, FALSE, FALSE, FALSE, TRUE, TRUE};


/**********************************************************************
 * Forward declarations
 **********************************************************************/
static ButOut  unmap(ButWin *win);
static ButOut  map(ButWin *win);
static ButOut  resize(ButWin *win);
static ButOut  destroy(ButWin *win);
static ButOut  okPressed(But *but);
static ButOut  cancelPressed(But *but);
static ButOut  newType(But *but, int newVal);
static ButOut  newIn1(But *but, const char *value);
static ButOut  newIn2(But *but, const char *value);
static void  gotGameSetup(void *packet, GameSetup *gameSetup);


/**********************************************************************
 * Functions
 **********************************************************************/

GmpSetup  *gmpSetup_create(Cgoban *cg)  {
  GmpSetup  *gs;
  GoStone  s;
  int  w, h;
  GmpSetupPlayer  *p;
  char  port[10];

  assert(MAGIC(cg));
  gs = wms_malloc(sizeof(GmpSetup));
  MAGIC_SET(gs);
  gs->cg = cg;
  gs->gameWaiting = FALSE;
  h = cg->fontH * 16 + butEnv_stdBw(cg->env) * 14;
  w = h * 2;
  gs->win = butWin_create(gs, cg->env, "Go Modem Protocol Setup", w,h,
			  unmap, map, resize, destroy);
  butWin_activate(gs->win);
  gs->bg = butBoxFilled_create(gs->win, 0, BUT_DRAWABLE);
  butBoxFilled_setPixmaps(gs->bg, cg->bgLitPixmap, cg->bgShadPixmap,
			  cg->bgPixmap);
  gs->title = butText_create(gs->win, 1, BUT_DRAWABLE, msg_gmpSetup,
			     butText_center);
  butText_setFont(gs->title, 2);
  gs->ok = butCt_create(okPressed, gs, gs->win, 1,
			BUT_PRESSABLE|BUT_DRAWABLE, msg_ok);
  gs->cancel = butCt_create(cancelPressed, gs, gs->win, 1,
			    BUT_PRESSABLE|BUT_DRAWABLE, msg_cancel);
  gs->help = butCt_create(cgoban_createHelpWindow, &help_gmpSetup, gs->win, 1,
			  BUT_DRAWABLE|BUT_PRESSABLE, msg_help);

  gs->players[goStone_white].optionEntry = clp_lookup(cg->clp, "gmp.WOption");
  gs->players[goStone_black].optionEntry = clp_lookup(cg->clp, "gmp.BOption");
  gs->players[goStone_white].progNameEntry = clp_lookup(cg->clp,
							"gmp.WProgram");
  gs->players[goStone_black].progNameEntry = clp_lookup(cg->clp,
							"gmp.BProgram");
  gs->players[goStone_white].devNameEntry = clp_lookup(cg->clp, "gmp.WDevice");
  gs->players[goStone_black].devNameEntry = clp_lookup(cg->clp, "gmp.BDevice");
  sprintf(port, "%d", clp_getInt(cg->clp, "gmp.port"));
  /*
   * Set up the goStone_black type because when I set up goStone_white,
   *   below, I will check this value and it must be something sane.
   */
  gs->players[goStone_black].type = gmpSetup_program;
  goStoneIter(s)  {
    p = &gs->players[s];
    str_initChars(&p->progName, clpEntry_getStr(p->progNameEntry));
    str_initChars(&p->devName, clpEntry_getStr(p->devNameEntry));
    p->box = butBox_create(gs->win, 1, BUT_DRAWABLE);
    butBox_setPixmaps(p->box, cg->bgLitPixmap, cg->bgShadPixmap);
    p->typeMenu = butMenu_downCreate(newType, gs, gs->win, 1, 2,
				     BUT_DRAWABLE|BUT_PRESSABLE,
				     msg_gmpPlayers[s],
				     msg_gmpTypes,
				     clpEntry_getInt(p->optionEntry));
    butMenu_setFlags(p->typeMenu, (int)gmpSetup_local, BUTMENU_DISABLED);
    butMenu_setFlags(p->typeMenu, (int)gmpSetup_remote, BUTMENU_DISABLED);
    p->strDesc1 = butText_create(gs->win, 1, BUT_DRAWABLE,
				 msg_program, butText_center);
    butText_setColor(p->strDesc1, BUT_NOCHANGE, TRUE);
    p->strIn1 = butTextin_create(newIn1, gs, gs->win, 1,
				 BUT_DRAWABLE,
				 str_chars(&p->progName), 500);
    p->strDesc2 = butText_create(gs->win, 1, BUT_DRAWABLE,
				 msg_port, butText_center);
    butText_setColor(p->strDesc2, BUT_NOCHANGE, TRUE);
    p->strIn2 = butTextin_create(newIn2, gs, gs->win, 1,
				 BUT_DRAWABLE, port, 100);
    p->type = -1;
    newType(p->typeMenu, clpEntry_getInt(p->optionEntry));
  }
  return(gs);
}


void  gmpSetup_destroy(GmpSetup *gs)  {
  assert(MAGIC(gs));
  if (gs->win)  {
    butWin_setDestroy(gs->win, NULL);
    butWin_destroy(gs->win);
  }
  str_deinit(&gs->players[goStone_white].progName);
  str_deinit(&gs->players[goStone_black].progName);
  str_deinit(&gs->players[goStone_white].devName);
  str_deinit(&gs->players[goStone_black].devName);
  MAGIC_UNSET(gs);
  wms_free(gs);
}
  


static ButOut  unmap(ButWin *win)  {
  return(0);
}


static ButOut  map(ButWin *win)  {
  return(0);
}


static ButOut  resize(ButWin *win)  {
  GmpSetup  *gs = butWin_packet(win);
  Cgoban  *cg;
  int  w, h;
  int  bw, fh;
  ButEnv  *env;
  int  boxX, boxY, boxW, boxH, butX, butW, butY;
  GoStone  s;
  GmpSetupPlayer  *p;

  assert(MAGIC(gs));
  cg = gs->cg;
  assert(MAGIC(cg));
  env = cg->env;
  bw = butEnv_stdBw(env);
  fh = cg->fontH;
  w = butWin_w(win);
  h = butWin_h(win);

  but_resize(gs->bg, 0,0, w,h);
  butText_resize(gs->title, w/2,bw*2, fh*2);
  boxX = bw;
  boxY = bw * 3 + fh * 2;
  boxH = h - bw * 6 - fh * 4;
  goStoneIter(s)  {
    p = &gs->players[s];
    boxW = (w - bw * 2 + (int)s) / 2;
    but_resize(p->box, boxX, boxY, boxW, boxH);
    butX = boxX + bw * 2;
    butW = boxW - bw * 4;
    butY = boxY + bw * 2;
    boxX += boxW;
    but_resize(p->typeMenu, butX, butY, butW, fh * 4);
    butY += fh * 4 + bw;
    but_resize(p->strDesc1, butX, butY, butW, fh * 2);
    butY += fh * 2 + bw;
    but_resize(p->strIn1, butX, butY, butW, fh * 2);
    butY += fh * 2 + bw;
    but_resize(p->strDesc2, butX, butY, butW, fh * 2);
    butY += fh * 2 + bw;
    but_resize(p->strIn2, butX, butY, butW, fh * 2);
  }

  boxX = bw * 2;
  boxW = (w - bw * 6) / 3;
  but_resize(gs->help, boxX, boxY + boxH + bw, boxW, fh * 2);
  boxX += boxW + bw;
  boxW = (w - bw * 6) / 3 + ((w - bw * 6) % 3);
  but_resize(gs->cancel, boxX, boxY + boxH + bw, boxW, fh * 2);
  boxX += boxW + bw;
  boxW = (w - bw * 6) / 3;
  but_resize(gs->ok, boxX, boxY + boxH + bw, boxW, fh * 2);
  return(0);
}


static ButOut  destroy(ButWin *win)  {
  GmpSetup  *gs = butWin_packet(win);

  assert(MAGIC(gs));
  gs->win = NULL;  /* It's dead already. */
  if (!gs->gameWaiting)  {
    gmpSetup_destroy(gs);
  }
  return(0);
}


/*
 * Callback for when "OK" is pressed in the setup window.
 * It starts up the game setup dialogue, which asks for handicap, time
 *   controls, etc.
 */
static ButOut  okPressed(But *but)  {
  GmpSetup  *gs;
  GoStone  s;
  GmpSetupPlayer  *p;
  bool  needSetup = TRUE;
  CliServer  server = cliServer_nngs;
  const char  *userName = NULL, *password = NULL;

  gs = but_packet(but);
  assert(MAGIC(gs));
  goStoneIter(s)  {
    p = &gs->players[s];
    if (but_getFlags(p->strIn1) & BUT_PRESSABLE)  {
      if (newIn1(p->strIn1, butTextin_get(p->strIn1)) & BUTOUT_ERR)
	return(BUTOUT_ERR);
    }
    if (but_getFlags(p->strIn2) & BUT_PRESSABLE)  {
      if (newIn2(p->strIn2, butTextin_get(p->strIn2)) & BUTOUT_ERR)
	return(BUTOUT_ERR);
    }
    if ((p->type == gmpSetup_nngs) || (p->type == gmpSetup_igs))  {
      if (p->type == gmpSetup_nngs)
	server = cliServer_nngs;
      else
	server = cliServer_igs;
      needSetup = FALSE;
      userName = butTextin_get(p->strIn1);
      password = butTextin_get(p->strIn2);
    }
  }
  if (needSetup)  {
    gs->gameWaiting = TRUE;
    butWin_destroy(gs->win);
    gameSetup_create(gs->cg, gotGameSetup, gs);
  } else  {
    /* You need to connect to the server! */
    if (gs->players[goStone_white].type == gmpSetup_program)
      gmpClient_create(gs->cg, str_chars(&gs->players[goStone_white].progName),
		       server, userName, password);
    else
      gmpClient_create(gs->cg, str_chars(&gs->players[goStone_black].progName),
		       server, userName, password);
    gmpSetup_destroy(gs);
  }
  return(0);
}


static void  gotGameSetup(void *packet, GameSetup *gameSetup)  {
  int  inFiles[2], outFiles[2], pids[2];
  GmpSetup  *gs = packet;
  GoStone  s;
  GmpSetupPlayer  *p;
  bool  valid = TRUE;
  Str  error;
  struct termios  newTty;

  assert(MAGIC(gs));
  if (gameSetup)  {
    if (gameSetup_size(gameSetup) > 22)  {
      cgoban_createMsgWindow(gs->cg, "Cgoban Error", msg_gmpTooBig);
      valid = FALSE;
    } else  {
      goStoneIter(s)  {
	p = &gs->players[s];
	switch(p->type)  {
	case gmpSetup_program:
	  pids[s] = gmp_forkProgram(gs->cg, &inFiles[s], &outFiles[s],
				    str_chars(&p->progName),
				    gameSetup->mainTime, gameSetup->byTime);
	  if (pids[s] < 0)  {
	    str_init(&error);
	    str_print(&error, msg_progRunErr,
		      strerror(errno), str_chars(&p->progName));
	    cgoban_createMsgWindow(gs->cg, "Cgoban Error", str_chars(&error));
	    str_deinit(&error);
	    valid = FALSE;
	  }
	  break;
	case gmpSetup_device:
	  pids[s] = -1;
	  inFiles[s] = outFiles[s] = open(str_chars(&p->devName),
					  O_RDWR | O_NDELAY);
	  if (inFiles[s] < 0) {
	    str_init(&error);
	    str_print(&error, msg_devOpenErr,
		      strerror(errno), str_chars(&p->devName));
	    cgoban_createMsgWindow(gs->cg, "Cgoban Error", str_chars(&error));
	    str_deinit(&error);
	    valid = FALSE;
	  }
	  if (isatty(inFiles[s])) {
	    newTty.c_cflag = B2400 | CREAD | CLOCAL | CS8;
	    newTty.c_iflag = 0;
	    newTty.c_oflag = 0;
	    newTty.c_lflag = 0;
	    bzero(newTty.c_cc, NCCS);
	    tcsetattr(inFiles[s], TCSANOW, &newTty);
	  }
	  break;
	case gmpSetup_human:
	  inFiles[s] = outFiles[s] = -1;
	  pids[s] = -1;
	  break;
	case gmpSetup_local:
	case gmpSetup_remote:
	default:
	  assert(0);
	  break;
	}
      }
    }
    if (valid)  {
      gmpPlay_create(gs->cg, inFiles, outFiles, pids,
		     gameSetup_rules(gameSetup), gameSetup_size(gameSetup),
		     gameSetup_handicap(gameSetup), gameSetup_komi(gameSetup),
		     gameSetup_wName(gameSetup), gameSetup_bName(gameSetup),
		     gameSetup_timeType(gameSetup),
		     gameSetup_mainTime(gameSetup),
		     gameSetup_byTime(gameSetup),
		     gameSetup_timeAux(gameSetup));
    }
  }
  gmpSetup_destroy(gs);
}
  

static ButOut  cancelPressed(But *but)  {
  GmpSetup  *gs = but_packet(but);

  assert(MAGIC(gs));
  butWin_destroy(gs->win);
  return(0);
}


static ButOut  newType(But *but, int newVal)  {
  GmpSetup  *gs;
  GoStone  color;
  GmpSetupPlayer  *p;

  gs = but_packet(but);
  assert(MAGIC(gs));
  if (but == gs->players[goStone_white].typeMenu)  {
    color = goStone_white;
  } else  {
    assert(but == gs->players[goStone_black].typeMenu);
    color = goStone_black;
  }
  p = &gs->players[color];
  if (p->type == (GmpSetupType)newVal)
    return(0);
  p->type = (GmpSetupType)newVal;
  clpEntry_setInt(p->optionEntry, newVal);
  switch((GmpSetupType)newVal)  {
  case gmpSetup_program:
    butText_setColor(p->strDesc1, BUT_NOCHANGE, FALSE);
    butText_set(p->strDesc1, msg_program);
    butTextin_set(p->strIn1, str_chars(&p->progName), FALSE);
    but_setFlags(p->strIn1, BUT_PRESSABLE);

    butText_setColor(p->strDesc2, BUT_NOCHANGE, TRUE);
    but_setFlags(p->strIn2, BUT_NOPRESS);
    break;
  case gmpSetup_device:
    butText_setColor(p->strDesc1, BUT_NOCHANGE, FALSE);
    butText_set(p->strDesc1, msg_device);
    butTextin_set(p->strIn1, str_chars(&p->devName), FALSE);
    but_setFlags(p->strIn1, BUT_PRESSABLE);

    butText_setColor(p->strDesc2, BUT_NOCHANGE, TRUE);
    but_setFlags(p->strIn2, BUT_NOPRESS);
    break;
  case gmpSetup_human:
    butText_setColor(p->strDesc1, BUT_NOCHANGE, TRUE);
    but_setFlags(p->strIn1, BUT_NOPRESS);

    butText_setColor(p->strDesc2, BUT_NOCHANGE, TRUE);
    but_setFlags(p->strIn2, BUT_NOPRESS);
    break;
  case gmpSetup_nngs:
    butText_setColor(p->strDesc1, BUT_NOCHANGE, FALSE);
    butText_set(p->strDesc1, msg_username);
    butTextin_set(p->strIn1, clp_getStr(gs->cg->clp, "gmp.nngs.username"),
		  FALSE);
    but_setFlags(p->strIn1, BUT_PRESSABLE);

    butText_setColor(p->strDesc2, BUT_NOCHANGE, FALSE);
    butText_set(p->strDesc2, msg_password);
    but_setFlags(p->strIn2, BUT_DRAWABLE|BUT_PRESSABLE);
    butTextin_set(p->strIn2, clp_getStr(gs->cg->clp, "gmp.nngs.password"),
		  FALSE);
    butTextin_setHidden(p->strIn2, TRUE);
    break;
  case gmpSetup_igs:
    butText_setColor(p->strDesc1, BUT_NOCHANGE, FALSE);
    butText_set(p->strDesc1, msg_username);
    butTextin_set(p->strIn1, clp_getStr(gs->cg->clp, "gmp.igs.username"),
		  FALSE);
    but_setFlags(p->strIn1, BUT_PRESSABLE);

    butText_setColor(p->strDesc2, BUT_NOCHANGE, FALSE);
    butText_set(p->strDesc2, msg_password);
    but_setFlags(p->strIn2, BUT_DRAWABLE|BUT_PRESSABLE);
    butTextin_set(p->strIn2, clp_getStr(gs->cg->clp, "gmp.igs.password"),
		  FALSE);
    butTextin_setHidden(p->strIn2, TRUE);
    break;
  case gmpSetup_local:
    butText_setColor(p->strDesc1, BUT_NOCHANGE, FALSE);
    butText_set(p->strDesc1, msg_port);
    butTextin_set(p->strIn1, clp_getStr(gs->cg->clp, "gmp.port"),
		  FALSE);
    but_setFlags(p->strIn1, BUT_PRESSABLE);

    butText_setColor(p->strDesc2, BUT_NOCHANGE, TRUE);
    but_setFlags(p->strIn2, BUT_NOPRESS);
    break;
  case gmpSetup_remote:
    butText_setColor(p->strDesc1, BUT_NOCHANGE, FALSE);
    butText_set(p->strDesc1, msg_machineName);
    butTextin_set(p->strIn1, clp_getStr(gs->cg->clp, "gmp.machine"), FALSE);
    but_setFlags(p->strIn1, BUT_PRESSABLE);

    butText_setColor(p->strDesc2, BUT_NOCHANGE, FALSE);
    butText_set(p->strDesc2, msg_port);
    but_setFlags(p->strIn2, BUT_DRAWABLE|BUT_PRESSABLE);
    butTextin_set(p->strIn2, clp_getStr(gs->cg->clp, "gmp.port"), FALSE);
    butTextin_setHidden(p->strIn2, FALSE);
    break;
  }
  if (!isGmpType[gs->players[goStone_white].type] &&
      !isGmpType[gs->players[goStone_black].type])  {
    but_setFlags(gs->ok, BUT_NOPRESS);
  } else  {
    but_setFlags(gs->ok, BUT_PRESSABLE);
  }
  return(0);
}


static ButOut  newIn1(But *but, const char *value)  {
  GmpSetup  *gs = but_packet(but);
  GoStone  color;
  GmpSetupPlayer  *p;
  Str  errorMsg;

  assert(MAGIC(gs));
  but_setFlags(but, BUT_NOKEY);
  if (but == gs->players[goStone_white].strIn1)  {
    color = goStone_white;
  } else  {
    assert(but == gs->players[goStone_black].strIn1);
    color = goStone_black;
  }
  p = &gs->players[color];
  switch(p->type)  {
  case gmpSetup_program:
    clpEntry_setStr(p->progNameEntry, value);
    str_copyChars(&p->progName, value);
    break;
  case gmpSetup_device:
    clpEntry_setStr(p->devNameEntry, value);
    str_copyChars(&p->devName, value);
    break;
  case gmpSetup_nngs:
    clp_setStr(gs->cg->clp, "gmp.nngs.username", value);
    break;
  case gmpSetup_igs:
    clp_setStr(gs->cg->clp, "gmp.igs.username", value);
    break;
  case gmpSetup_local:
    if (!clp_setStr(gs->cg->clp, "gmp.port", value))  {
      str_init(&errorMsg);
      str_print(&errorMsg, msg_badPortNum,
		value);
      cgoban_createMsgWindow(gs->cg, "Cgoban Error", str_chars(&errorMsg));
      str_deinit(&errorMsg);
      return(BUTOUT_ERR);
    }
    break;
  case gmpSetup_remote:
    clp_setStr(gs->cg->clp, "gmp.machine", value);
    break;
  default:
    assert(0);
    break;
  }
  return(0);
}


static ButOut  newIn2(But *but, const char *value)  {
  GmpSetup  *gs = but_packet(but);
  GoStone  color;
  GmpSetupPlayer  *p;
  Str  errorMsg;

  assert(MAGIC(gs));
  but_setFlags(but, BUT_NOKEY);
  if (but == gs->players[goStone_white].strIn2)  {
    color = goStone_white;
  } else  {
    assert(but == gs->players[goStone_black].strIn2);
    color = goStone_black;
  }
  p = &gs->players[color];
  switch(p->type)  {
  case gmpSetup_nngs:
    clp_setStr(gs->cg->clp, "gmp.nngs.password", value);
    break;
  case gmpSetup_igs:
    clp_setStr(gs->cg->clp, "gmp.igs.password", value);
    break;
  case gmpSetup_remote:
    if (!clp_setStr(gs->cg->clp, "gmp.port", value))  {
      str_init(&errorMsg);
      str_print(&errorMsg, msg_badPortNum,
		value);
      cgoban_createMsgWindow(gs->cg, "Cgoban Error", str_chars(&errorMsg));
      str_deinit(&errorMsg);
      return(BUTOUT_ERR);
    }
    break;
  default:
    assert(0);
    break;
  }
  return(0);
}
