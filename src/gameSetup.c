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
#include <abut/msg.h>
#include <wms/clp.h>
#include <wms/str.h>
#include "cgoban.h"
#include "gameSetup.h"
#include "msg.h"
#include "help.h"


/**********************************************************************
 * Forward Declarations
 **********************************************************************/
static ButOut  resize(ButWin *win);
static ButOut  destroy(ButWin *win);
static ButOut  newRules(But *but, int newVal);
static ButOut  newSize(But *but, const char *value);
static ButOut  newHcap(But *but, const char *value);
static ButOut  newKomi(But *but, const char *value);
static ButOut  newName(But *but, const char *value);
static ButOut  okPressed(But *but);
static ButOut  cancelPressed(But *but);
static ButOut  newTimeType(But *but, int newVal);
static ButOut  newMainTime(But *but, const char *value);
static ButOut  newBYTime(But *but, const char *value);
static ButOut  newAuxTime(But *but, const char *value);


/**********************************************************************
 * Functions
 **********************************************************************/
GameSetup  *gameSetup_create(Cgoban *cg,
			     void (*callback)(void *packet, GameSetup *gs),
			     void *packet)  {
  GameSetup *ls;
  int  w, h;
  char  curText[10];

  assert(MAGIC(cg));
  ls = wms_malloc(sizeof(GameSetup));
  MAGIC_SET(ls);
  ls->callback = callback;
  ls->packet = packet;
  ls->cg = cg;
  h = cg->fontH * 22 + butEnv_stdBw(cg->env) * 20;
  w = h * 1.618034 + 0.5;
  ls->win = butWin_create(ls, cg->env, "Game Setup", w,h,
			  NULL, NULL, resize, destroy);
  butWin_activate(ls->win);
  ls->bg = butBoxFilled_create(ls->win, 0, BUT_DRAWABLE);
  butBoxFilled_setPixmaps(ls->bg, cg->bgLitPixmap, cg->bgShadPixmap,
			  cg->bgPixmap);
  ls->title = butText_create(ls->win, 1, BUT_DRAWABLE, msg_gSetup,
			     butText_center);
  butText_setFont(ls->title, 2);

  ls->namesBox = butBox_create(ls->win, 1, BUT_DRAWABLE);
  butBox_setPixmaps(ls->namesBox, cg->bgLitPixmap, cg->bgShadPixmap);
  ls->wStr = butText_create(ls->win, 1, BUT_DRAWABLE,
			    msg_wName, butText_left);
  ls->wIn = butTextin_create(newName, ls, ls->win, 1,
			     BUT_PRESSABLE|BUT_DRAWABLE,
			     clp_getStr(cg->clp, "local.wName"), 30);
  ls->bStr = butText_create(ls->win, 1, BUT_DRAWABLE,
			    msg_bName, butText_left);
  ls->bIn = butTextin_create(newName, ls, ls->win, 1,
			     BUT_PRESSABLE|BUT_DRAWABLE,
			     clp_getStr(cg->clp, "local.bName"), 30);

  ls->rules = (GoRules)clp_getInt(cg->clp, "game.rules");
  ls->size = clp_getInt(cg->clp, "game.size");
  ls->hcap = clp_getInt(cg->clp, "game.handicap");
  ls->komi = clp_getDouble(cg->clp, "game.komi");
  ls->sizeSet = FALSE;
  ls->hcapSet = FALSE;
  ls->komiSet = FALSE;
  ls->rulesBox = butBox_create(ls->win, 1, BUT_DRAWABLE);
  butBox_setPixmaps(ls->rulesBox, cg->bgLitPixmap, cg->bgShadPixmap);
  ls->rulesMenu = butMenu_downCreate(newRules, ls, ls->win, 1, 2,
				     BUT_PRESSABLE|BUT_DRAWABLE,
				     msg_ruleSet,
				     msg_ruleNames,
				     ls->rules);
  butMenu_setFlags(ls->rulesMenu, (int)goRules_tibetan, BUTMENU_DISABLED);
  ls->sizeStr = butText_create(ls->win, 1, BUT_DRAWABLE,
			       msg_boardSize, butText_center);
  sprintf(curText, "%d", ls->size);
  ls->sizeIn = butTextin_create(newSize, ls, ls->win, 1,
				BUT_PRESSABLE|BUT_DRAWABLE, curText, 10);
  ls->hcapStr = butText_create(ls->win, 1, BUT_DRAWABLE,
			       msg_handicap, butText_center);
  sprintf(curText, "%d", ls->hcap);
  ls->hcapIn = butTextin_create(newHcap, ls, ls->win, 1,
				BUT_PRESSABLE|BUT_DRAWABLE, curText, 10);
  ls->komiStr = butText_create(ls->win, 1, BUT_DRAWABLE,
			       msg_komi, butText_center);
  sprintf(curText, "%g", ls->komi);
  ls->komiIn = butTextin_create(newKomi, ls, ls->win, 1,
				BUT_PRESSABLE|BUT_DRAWABLE, curText, 10);

  ls->timeBox = butBox_create(ls->win, 1, BUT_DRAWABLE);
  butBox_setPixmaps(ls->timeBox, cg->bgLitPixmap, cg->bgShadPixmap);
  ls->timeType = 2;
  ls->numTimeArgs = 3;
  ls->timeMenu = butMenu_downCreate(newTimeType, ls, ls->win, 1, 2,
				    BUT_PRESSABLE|BUT_DRAWABLE,
				    msg_timeSystem,
				    msg_timeSystems,
				    clp_getInt(cg->clp, "setup.timeType"));
  ls->mainStr = butText_create(ls->win, 1, BUT_DRAWABLE,
			       msg_primaryTime, butText_center);
  ls->mainIn = butTextin_create(newMainTime, ls, ls->win, 1,
				BUT_PRESSABLE|BUT_DRAWABLE,
				clp_getStr(cg->clp, "setup.mainTime"), 10);
  ls->byStr = butText_create(ls->win, 1, BUT_DRAWABLE,
			     msg_byoYomi, butText_center);
  ls->byIn = butTextin_create(newBYTime, ls, ls->win, 1,
			      BUT_PRESSABLE|BUT_DRAWABLE,
			      clp_getStr(cg->clp, "setup.igsBYTime"), 10);
  ls->auxStr = butText_create(ls->win, 1, BUT_DRAWABLE,
			      msg_byoYomiStones, butText_center);
  ls->auxIn = butTextin_create(newAuxTime, ls, ls->win, 1,
			       BUT_PRESSABLE|BUT_DRAWABLE,
			       clp_getStr(cg->clp, "setup.igsBYStones"), 10);
  ls->timeType = goTime_canadian;
  ls->mainTime = goTime_parseChars(butTextin_get(ls->mainIn), FALSE, NULL);
  ls->byTime = goTime_parseChars(butTextin_get(ls->byIn), FALSE, NULL);
  ls->aux = wms_atoi(butTextin_get(ls->auxIn), NULL);
  newTimeType(ls->timeMenu, butMenu_get(ls->timeMenu));

  ls->help = butCt_create(cgoban_createHelpWindow,
			  &help_gameSetup, ls->win, 1,
			  BUT_DRAWABLE|BUT_PRESSABLE, msg_help);
  ls->ok = butCt_create(okPressed, ls, ls->win, 1,
			BUT_DRAWABLE|BUT_PRESSABLE, msg_ok);
  ls->cancel = butCt_create(cancelPressed, ls, ls->win, 1,
			    BUT_DRAWABLE|BUT_PRESSABLE, msg_cancel);
  return(ls);
}


void  gameSetup_destroy(GameSetup *ls)  {
  assert(MAGIC(ls));
  if (ls->callback)  {
    ls->callback(ls->packet, NULL);
    ls->callback = NULL;
  }
  if (ls->win)
    butWin_destroy(ls->win);
  MAGIC_UNSET(ls);
  wms_free(ls);
}
  


static ButOut  resize(ButWin *win)  {
  GameSetup  *ls = butWin_packet(win);
  Cgoban  *cg;
  int  w, butW, h, x, y;
  int  bw, fh;
  ButEnv  *env;
  int  wStrSize, bStrSize;

  assert(MAGIC(ls));
  cg = ls->cg;
  assert(MAGIC(cg));
  env = cg->env;
  bw = butEnv_stdBw(env);
  fh = cg->fontH;
  w = butWin_w(win);
  h = butWin_h(win);
  butW = w/2-5*bw;

  but_resize(ls->bg, 0,0, w,h);
  butText_resize(ls->title, w/2,bw*2, fh*2);

  but_resize(ls->namesBox, bw,bw*3+fh*2, w-bw*2,bw*4+fh*2);
  wStrSize = butText_resize(ls->wStr, bw*3, bw*5+fh*2, fh*2);
  bStrSize = butText_resize(ls->bStr, w/2+bw*2, bw*5+fh*2, fh*2);
  if (bStrSize > wStrSize)
    wStrSize = bStrSize;
  but_resize(ls->wIn, bw*4+wStrSize, bw*5+fh*2,
	     w/2 - (bw*6+wStrSize), fh*2);
  but_resize(ls->bIn, w/2+bw*3+wStrSize, bw*5+fh*2,
	     w - (w/2+bw*6+wStrSize), fh*2);

  y = bw*7+fh*4;
  but_resize(ls->rulesBox, bw,y, w/2-bw,fh*16+bw*10);
  but_resize(ls->rulesMenu, bw*3,y+bw*2+fh*0, butW,fh*4);

  but_resize(ls->sizeStr, bw*3,y+bw*3+fh*4, butW,fh*2);
  but_resize(ls->sizeIn, bw*3,y+bw*4+fh*6, butW,fh*2);

  but_resize(ls->hcapStr, bw*3,y+bw*5+fh*8, butW,fh*2);
  but_resize(ls->hcapIn, bw*3,y+bw*6+fh*10, butW,fh*2);

  but_resize(ls->komiStr, bw*3,y+bw*7+fh*12, butW,fh*2);
  but_resize(ls->komiIn, bw*3,y+bw*8+fh*14, butW,fh*2);

  but_resize(ls->timeBox, w/2,y, w-bw-w/2,fh*16+bw*10);
  x = w/2 + bw*2;
  butW = w-x-bw*3;
  but_resize(ls->timeMenu, x,y+bw*2+fh*0, butW,fh*4);

  but_resize(ls->mainStr, x,y+bw*3+fh*4, butW,fh*2);
  but_resize(ls->mainIn, x,y+bw*4+fh*6, butW,fh*2);

  but_resize(ls->byStr, x,y+bw*5+fh*8, butW,fh*2);
  but_resize(ls->byIn, x,y+bw*6+fh*10, butW,fh*2);

  but_resize(ls->auxStr, x,y+bw*7+fh*12, butW,fh*2);
  but_resize(ls->auxIn, x,y+bw*8+fh*14, butW,fh*2);

  but_resize(ls->help, bw*2,y+bw*11+fh*16, (w-bw*6+1)/3,fh*2);
  but_resize(ls->cancel, bw*3+(w-bw*6+1)/3,y+bw*11+fh*16,
	     w-2*(bw*3+(w-bw*6+1)/3),fh*2);
  but_resize(ls->ok, w-(bw*2+(w-bw*6+1)/3),y+bw*11+fh*16,
	     (w-bw*6+1)/3,fh*2);

  return(0);
}


static ButOut  destroy(ButWin *win)  {
  GameSetup  *ls = butWin_packet(win);

  assert(MAGIC(ls));
  ls->win = NULL;  /* It's dead already. */
  gameSetup_destroy(ls);
  return(0);
}


static ButOut  newRules(But *but, int newVal)  {
  GameSetup  *ls = but_packet(but);

  assert(MAGIC(ls));
  clp_setInt(ls->cg->clp, "game.rules", newVal);
  if (!ls->sizeSet)  {
    if (newVal != goRules_tibetan)  {
      if ((ls->size != 9) && (ls->size != 13) && (ls->size != 19))  {
	butTextin_set(ls->sizeIn, "19", FALSE);
	clp_setInt(ls->cg->clp, "game.size", 19);
	ls->size = 19;
      }
    } else  {
      butTextin_set(ls->sizeIn, "17", FALSE);
      clp_setInt(ls->cg->clp, "game.size", 17);
      ls->size = 17;
    }
  }
  if (!ls->komiSet && (ls->hcap == 0) &&
      (((ls->rules == goRules_ing) && (ls->komi == 8.0)) ||
       ((ls->rules != goRules_ing) && (ls->komi == 5.5))))  {
    if (newVal != goRules_ing)  {
      butTextin_set(ls->komiIn, "5.5", FALSE);
      clp_setDouble(ls->cg->clp, "game.komi", 5.5);
      ls->komi = 5.5;
    } else  {
      butTextin_set(ls->komiIn, "8", FALSE);
      clp_setDouble(ls->cg->clp, "game.komi", 8.0);
      ls->komi = 8.0;
    }
  }
  if (!ls->komiSet && (ls->hcap != 0) &&
      (((ls->rules == goRules_ing) && (ls->komi == 0.0)) ||
       ((ls->rules != goRules_ing) && (ls->komi == 0.5))))  {
    if (newVal != goRules_ing)  {
      butTextin_set(ls->komiIn, "0.5", FALSE);
      clp_setDouble(ls->cg->clp, "game.komi", 0.5);
      ls->komi = 0.5;
    } else  {
      butTextin_set(ls->komiIn, "0", FALSE);
      clp_setDouble(ls->cg->clp, "game.komi", 0.0);
      ls->komi = 0.0;
    }
  }
  ls->rules = newVal;
  return(0);
}


static ButOut  newSize(But *but, const char *value)  {
  GameSetup  *ls = but_packet(but);
  int  newSize;
  char  curValue[200];

  assert(MAGIC(ls));
  newSize = atoi(value);
  if (!clp_setStr(ls->cg->clp, "game.size", value))  {
    sprintf(curValue, msg_badSize, value, GOBOARD_MAXSIZE);
    cgoban_createMsgWindow(ls->cg, "Cgoban Error", curValue);
    sprintf(curValue, "%d", ls->size);
    butTextin_set(but, curValue, FALSE);
    return(BUTOUT_ERR);
  }
  ls->sizeSet = TRUE;
  ls->size = newSize;
  but_setFlags(ls->hcapIn, BUT_KEYED);
  return(0);
}


static ButOut  newHcap(But *but, const char *value)  {
  GameSetup  *ls = but_packet(but);
  int  newVal = atoi(value);
  Str  curValue;

  assert(MAGIC(ls));
  if (!clp_setStr(ls->cg->clp, "game.handicap", value))  {
    str_init(&curValue);
    str_print(&curValue, msg_badHcap, value, 27);
    cgoban_createMsgWindow(ls->cg, "Cgoban Error", str_chars(&curValue));
    str_print(&curValue, "%d", ls->hcap);
    butTextin_set(but, str_chars(&curValue), FALSE);
    str_deinit(&curValue);
    return(BUTOUT_ERR);
  }
  ls->hcapSet = TRUE;
  if ((newVal == 1) ||
      (!ls->komiSet && (ls->hcap == 0) && (newVal != 0) &&
       (((ls->rules == goRules_ing) && (ls->komi == 8.0)) ||
	((ls->rules != goRules_ing) && (ls->komi == 5.5)))))  {
    ls->komiSet = FALSE;
    if (ls->rules == goRules_ing)  {
      butTextin_set(ls->komiIn, "0", FALSE);
      clp_setDouble(ls->cg->clp, "game.komi", 0.0);
      ls->komi = 0.0;
    } else  {
      butTextin_set(ls->komiIn, "0.5", FALSE);
      clp_setDouble(ls->cg->clp, "game.komi", 0.5);
      ls->komi = 0.5;
    }
  }
  if (!ls->komiSet && (ls->hcap > 0) && (newVal == 0) &&
      ((ls->komi == 0.0) || (ls->komi == 0.5)))  {
    if (ls->rules == goRules_ing)  {
      butTextin_set(ls->komiIn, "8", FALSE);
      clp_setDouble(ls->cg->clp, "game.komi", 8.0);
      ls->komi = 8.0;
    } else  {
      butTextin_set(ls->komiIn, "5.5", FALSE);
      clp_setDouble(ls->cg->clp, "game.komi", 5.5);
      ls->komi = 5.5;
    }
  }
  ls->hcap = newVal;
  but_setFlags(ls->komiIn, BUT_KEYED);
  return(0);
}



static ButOut  newKomi(But *but, const char *value)  {
  GameSetup  *ls = but_packet(but);
  char  curValue[200];
  bool  komiErr;
  double  newKomi;

  assert(MAGIC(ls));
  newKomi = wms_atof(value, &komiErr);
  if (!clp_setStr(ls->cg->clp, "game.komi", value))  {
    sprintf(curValue, msg_badKomi, value);
    cgoban_createMsgWindow(ls->cg, "Cgoban Error", curValue);
    sprintf(curValue, "%g", ls->komi);
    butTextin_set(but, curValue, FALSE);
    return(BUTOUT_ERR);
  }
  ls->komiSet = TRUE;
  ls->komi = newKomi;
  but_setFlags(ls->komiIn, BUT_NOKEY);
  return(0);
}



static ButOut  okPressed(But *but)  {
  GameSetup  *ls = but_packet(but);
  Clp  *clp = ls->cg->clp;

  assert(MAGIC(ls));
  if (newSize(ls->sizeIn, butTextin_get(ls->sizeIn)) & BUTOUT_ERR)
    return(BUTOUT_ERR);
  if (newHcap(ls->hcapIn, butTextin_get(ls->hcapIn)) & BUTOUT_ERR)
    return(BUTOUT_ERR);
  if (newKomi(ls->komiIn, butTextin_get(ls->komiIn)) & BUTOUT_ERR)
    return(BUTOUT_ERR);
  if (ls->numTimeArgs >= 1)
    if (newMainTime(ls->mainIn, butTextin_get(ls->mainIn)) & BUTOUT_ERR)
      return(BUTOUT_ERR);
  if (ls->numTimeArgs >= 2)
    if (newBYTime(ls->byIn, butTextin_get(ls->byIn)) & BUTOUT_ERR)
      return(BUTOUT_ERR);
  if (ls->numTimeArgs >= 3)
    if (newAuxTime(ls->auxIn, butTextin_get(ls->auxIn)) & BUTOUT_ERR)
      return(BUTOUT_ERR);
  clp_setInt(clp, "game.size", ls->size);
  clp_setInt(clp, "game.handicap", ls->hcap);
  clp_setDouble(clp, "game.komi", ls->komi);
  clp_setStr(clp, "local.wName", butTextin_get(ls->wIn));
  clp_setStr(clp, "local.bName", butTextin_get(ls->bIn));

  if (ls->callback)  {
    ls->callback(ls->packet, ls);
    ls->callback = NULL;
  }
  butWin_destroy(but_win(but));
  return(0);
}



static ButOut  cancelPressed(But *but)  {
  GameSetup  *gs = but_packet(but);

  assert(MAGIC(gs));
  if (gs->callback)  {
    gs->callback(gs->packet, NULL);
    gs->callback = NULL;
  }
  butWin_destroy(but_win(but));
  return(0);
}


static ButOut  newName(But *but, const char *value)  {
  GameSetup  *ls = but_packet(but);

  assert(MAGIC(ls));
  if (but == ls->wIn)  {
    clp_setStr(ls->cg->clp, "local.wName", value);
    but_setFlags(ls->bIn, BUT_KEYED);
  } else  {
    assert(but == ls->bIn);
    but_setFlags(but, BUT_NOKEY);
    clp_setStr(ls->cg->clp, "local.bName", value);
  }
  return(0);
}


static ButOut  newTimeType(But *but, int newVal)  {
  GameSetup  *ls = but_packet(but);
  GoTimeType  newTime = (GoTimeType)newVal;
  int  nargs = 0;

  assert(MAGIC(ls));
  if (newTime == ls->timeType)
    return(0);
  switch(ls->timeType = newTime)  {
  case goTime_none:
    nargs = ls->numTimeArgs = 0;
    break;
  case goTime_absolute:
    nargs = ls->numTimeArgs = 1;
    ls->byTime = 0;  /* Just to make things clear. */
    break;
  case goTime_canadian:
    nargs = ls->numTimeArgs = 3;
    butTextin_set(ls->byIn, clp_getStr(ls->cg->clp, "setup.igsBYTime"), FALSE);
    butText_set(ls->auxStr, msg_byoYomiStones);
    butTextin_set(ls->auxIn, clp_getStr(ls->cg->clp, "setup.igsBYStones"),
		  FALSE);
    break;
  case goTime_ing:
    nargs = ls->numTimeArgs = 3;
    butTextin_set(ls->byIn, clp_getStr(ls->cg->clp, "setup.ingBYTime"), FALSE);
    butText_set(ls->auxStr, msg_byoYomiCount);
    butTextin_set(ls->auxIn, clp_getStr(ls->cg->clp, "setup.ingBYPeriods"),
		  FALSE);
    break;
  case goTime_japanese:
    nargs = ls->numTimeArgs = 3;
    butTextin_set(ls->byIn, clp_getStr(ls->cg->clp, "setup.jBYTime"), FALSE);
    butText_set(ls->auxStr, msg_byoYomiCount);
    butTextin_set(ls->auxIn, clp_getStr(ls->cg->clp, "setup.jBYPeriods"),
		  FALSE);
  }
  if (nargs >= 1)  {
    butText_setColor(ls->mainStr, BUT_FG, FALSE);
    but_setFlags(ls->mainIn, BUT_PRESSABLE);
    if (nargs >= 2)  {
      butText_setColor(ls->byStr, BUT_FG, FALSE);
      but_setFlags(ls->byIn, BUT_PRESSABLE);
      if (nargs >= 3)  {
	butText_setColor(ls->auxStr, BUT_FG, FALSE);
	but_setFlags(ls->auxIn, BUT_PRESSABLE);
      }
    }
  }
  if (nargs < 3)  {
    butText_setColor(ls->auxStr, BUT_FG, TRUE);
    but_setFlags(ls->auxIn, BUT_NOPRESS|BUT_NOKEY);
    if (nargs < 2)  {
      butText_setColor(ls->byStr, BUT_FG, TRUE);
      but_setFlags(ls->byIn, BUT_NOPRESS|BUT_NOKEY);
      if (nargs < 1)  {
	butText_setColor(ls->mainStr, BUT_FG, TRUE);
	but_setFlags(ls->mainIn, BUT_NOPRESS|BUT_NOKEY);
      }
    }
  }
  clp_setInt(ls->cg->clp, "setup.timeType", newVal);
  return(0);
}


static ButOut  newMainTime(But *but, const char *value)  {
  GameSetup  *ls = but_packet(but);
  Str  errMsg;

  if (!clp_setStr(ls->cg->clp, "setup.mainTime", value))  {
    str_init(&errMsg);
    str_print(&errMsg, msg_badTime, value);
    cgoban_createMsgWindow(ls->cg, "Cgoban Error", str_chars(&errMsg));
    goTime_str(ls->mainTime, &errMsg);
    butTextin_set(but, str_chars(&errMsg), FALSE);
    str_deinit(&errMsg);
    return(BUTOUT_ERR);
  }
  ls->mainTime = goTime_parseChars(value, FALSE, NULL);
  if (but_getFlags(ls->byIn) & BUT_PRESSABLE)
    but_setFlags(ls->byIn, BUT_KEYED);
  else
    but_setFlags(but, BUT_NOKEY);
  return(0);
}


static ButOut  newBYTime(But *but, const char *value)  {
  GameSetup  *ls = but_packet(but);
  Str  errMsg;

  if (!clp_setStr(ls->cg->clp, "setup.igsBYTime", value))  {
    str_init(&errMsg);
    str_print(&errMsg, msg_badTime, value);
    cgoban_createMsgWindow(ls->cg, "Cgoban Error", str_chars(&errMsg));
    goTime_str(ls->byTime, &errMsg);
    butTextin_set(but, str_chars(&errMsg), FALSE);
    str_deinit(&errMsg);
    return(BUTOUT_ERR);
  }
  ls->byTime = goTime_parseChars(value, FALSE, NULL);
  but_setFlags(but, BUT_NOKEY);
  if (but_getFlags(ls->auxIn) & BUT_PRESSABLE)
    but_setFlags(ls->auxIn, BUT_KEYED);
  else
    but_setFlags(but, BUT_NOKEY);
  return(0);
}


static ButOut  newAuxTime(But *but, const char *value)  {
  GameSetup  *ls = but_packet(but);
  Str  errMsg;

  if (!clp_setStr(ls->cg->clp, "setup.igsBYStones", value))  {
    str_init(&errMsg);
    if (ls->timeType == goTime_ing)
      str_print(&errMsg, msg_badBYCount, value);
    else
      str_print(&errMsg, msg_badBYStones, value);
    cgoban_createMsgWindow(ls->cg, "Cgoban Error", str_chars(&errMsg));
    str_print(&errMsg, "%d", ls->aux);
    butTextin_set(but, str_chars(&errMsg), FALSE);
    str_deinit(&errMsg);
    return(BUTOUT_ERR);
  }
  ls->aux = wms_atoi(value, NULL);
  but_setFlags(but, BUT_NOKEY);
  return(0);
}
