/*
 * src/client/match.c, part of Complete Goban (player program)
 * Copyright (C) 1996 William Shubert
 * See "configure.h.in" for more copyright information.
 */

#include <wms.h>
#include "../msg.h"
#include <but/but.h>
#include <but/plain.h>
#include <but/ctext.h>
#include <but/textin.h>
#include <but/checkbox.h>
#include <abut/msg.h>
#include "../goTime.h"
#ifdef  _CLIENT_MATCH_H_
  Levelization Error.
#endif
#include "match.h"


/**********************************************************************
 * Forward Declarations
 **********************************************************************/
static ButOut  resize(ButWin *win);
static ButOut  destroy(ButWin *win);
static ButOut  newSize(But *but, const char *value);
static ButOut  newHcap(But *but, const char *value);
static ButOut  newKomi(But *but, const char *value);
static ButOut  newMainTime(But *but, const char *value);
static ButOut  newBYTime(But *but, const char *value);
static ButOut  okPressed(But *but);
static ButOut  cancelPressed(But *but);
static ButOut  newFree(But *but, bool newVal);
static ButOut  swapPressed(But *but);
static CliMatch  *playerLookup(CliMatch *chain, const char *player);
static ButOut  changeMain(But *but);
static ButOut  changeBy(But *but);
static ButOut  changeSize(But *but);
static ButOut  changeKomi(But *but);
static ButOut  changeHcap(But *but);


/**********************************************************************
 * Functions
 **********************************************************************/
CliMatch  *cliMatch_matchCommand(CliData *data, const char *command,
				 CliMatch **next, int rankDiff)  {
  char  oppName[21];
  char  colorChar;
  int  args, size, mainTime, byTime;
  CliMatch  *match;

  args = sscanf(command, "Use <match %20s %c %d %d %d>",
		oppName, &colorChar, &size, &mainTime, &byTime);
  assert(args == 5);
  assert((colorChar == 'W') || (colorChar == 'B'));
  match = playerLookup(*next, oppName);
  if (match == NULL)  {
    /* Start up a new match window. */
    match = cliMatch_create(data, oppName, next, rankDiff);
    if ((colorChar == 'B') != match->meFirst)
      swapPressed(match->swap);
  } else  {
    /* Modify an old one. */
    XRaiseWindow(butEnv_dpy(butWin_env(match->win)), butWin_xwin(match->win));
    if ((colorChar == 'B') != match->meFirst)
      swapPressed(match->swap);
  }
  str_print(&data->cmdBuild, "%d", match->size = size);
  butTextin_set(match->sizeIn, str_chars(&data->cmdBuild), TRUE);
  goTime_str(match->mainTime = mainTime * 60, &data->cmdBuild);
  butTextin_set(match->mainIn, str_chars(&data->cmdBuild), TRUE);
  goTime_str(match->byTime = byTime * 60, &data->cmdBuild);
  butTextin_set(match->byIn, str_chars(&data->cmdBuild), TRUE);
  match->state = cliMatch_recvd;
  butText_set(match->title, msg_cliGameSetupRecvd);
  return(match);
}


CliMatch  *cliMatch_create(CliData *data, const char *oppName,
			   CliMatch **next, int rankDiff)  {
  CliMatch  *m;
  Cgoban  *cg;
  int  i, h, w;
  char  curText[20];
  bool  err;
  uint  flags;

  assert(MAGIC(data));
  m = playerLookup(*next, oppName);
  if (m != NULL)  {
    XRaiseWindow(butEnv_dpy(butWin_env(m->win)), butWin_xwin(m->win));
    return(m);
  }
  cg = data->cg;
  m = wms_malloc(sizeof(CliMatch));
  MAGIC_SET(m);
  m->next = *next;
  m->prev = next;
  *next = m;
  m->data = data;
  m->state = cliMatch_nil;
  if (rankDiff == 0)  {
    m->meFirst = clp_getBool(cg->clp, "client.match.meFirst");
    m->komi = 5.5;
    m->hcap = 0;
  } else if (rankDiff > 0)  {
    m->meFirst = FALSE;
    m->komi = 0.5;
    m->hcap = 0;
  } else /* rankDiff < 0 */  {
    m->meFirst = TRUE;
    m->komi = 0.5;
    m->hcap = 0;
  }
  if (m->meFirst)  {
    str_initChars(&m->wName, oppName);
    str_initChars(&m->bName, str_chars(&data->userName));
  } else  {
    str_initChars(&m->bName, oppName);
    str_initChars(&m->wName, str_chars(&data->userName));
  }
  h = cg->fontH * 18 + butEnv_stdBw(cg->env) * 19;
  w = (h * 1.4142136) + 0.5;
  m->win = butWin_create(m, cg->env, "Server Game Setup",
			 w, h, NULL, NULL, resize, destroy);
  if (m->next == NULL)  {
    /*
     * We only want to set the window to the "standard" place if it is the
     *   first one.  Otherwise all the windows would stack up on each other,
     *   which is kind of annoying.
     */
    i = clp_iGetInt(cg->clp, "client.match.x", &err);
    if (!err)
      butWin_setX(m->win, i);
    i = clp_iGetInt(cg->clp, "client.match.y", &err);
    if (!err)
      butWin_setY(m->win, i);
  }
  butWin_activate(m->win);
  m->bg = butBoxFilled_create(m->win, 0, BUT_DRAWABLE);
  butBoxFilled_setPixmaps(m->bg, cg->bgLitPixmap, cg->bgShadPixmap,
			  cg->bgPixmap);

  m->title = butText_create(m->win, 1, BUT_DRAWABLE, msg_cliGameSetup,
			     butText_center);
  butText_setFont(m->title, 2);

  m->namesBox = butBox_create(m->win, 1, BUT_DRAWABLE);
  butBox_setPixmaps(m->namesBox, cg->bgLitPixmap, cg->bgShadPixmap);

  m->wBox = butBoxFilled_create(m->win, 1, BUT_DRAWABLE);
  butBoxFilled_setColors(m->wBox, BUT_LIT, BUT_SHAD, BUT_BG);
  str_print(&data->cmdBuild, CGBUTS_WSTONECHAR " %s", str_chars(&m->wName));
  m->wTitle = butText_create(m->win, 2, BUT_DRAWABLE,
			     str_chars(&data->cmdBuild), butText_left);

  m->bBox = butBoxFilled_create(m->win, 1, BUT_DRAWABLE);
  butBoxFilled_setColors(m->bBox, BUT_LIT, BUT_SHAD, BUT_BG);
  str_print(&data->cmdBuild, CGBUTS_BSTONECHAR " %s", str_chars(&m->bName));
  m->bTitle = butText_create(m->win, 2, BUT_DRAWABLE,
			     str_chars(&data->cmdBuild), butText_left);

  m->swap = butAct_create(swapPressed, m, m->win, 1,
			  BUT_DRAWABLE|BUT_PRESSABLE,
			  msg_swapColors, BUT_ALEFT|BUT_ARIGHT);

  m->size = clp_getInt(cg->clp, "client.match.size");
  m->mainTime = clp_getInt(cg->clp, "client.match.mainTime");
  m->byTime = clp_getInt(cg->clp, "client.match.byTime");

  m->rulesBox = butBox_create(m->win, 1, BUT_DRAWABLE);
  butBox_setPixmaps(m->rulesBox, cg->bgLitPixmap, cg->bgShadPixmap);
  m->sizeStr = butText_create(m->win, 1, BUT_DRAWABLE,
			       msg_boardSize, butText_center);
  sprintf(curText, "%d", m->size);
  m->sizeIn = butTextin_create(newSize, m, m->win, 1,
				BUT_DRAWABLE|BUT_PRESSABLE, curText, 10);
  flags = BUT_DRAWABLE;
  if (m->size < 19)
    flags |= BUT_PRESSABLE;
  m->sizeUp = butAct_create(changeSize, m, m->win, 1, flags, CGBUTS_FWDCHAR,
			    BUT_RRIGHT|BUT_SLEFT);
  flags = BUT_DRAWABLE;
  if (m->size > 9)
    flags |= BUT_PRESSABLE;
  m->sizeDown = butAct_create(changeSize, m, m->win, 1, flags, CGBUTS_BACKCHAR,
			      BUT_RLEFT|BUT_SRIGHT);

  m->hcapStr = butText_create(m->win, 1, BUT_DRAWABLE,
			       msg_handicap, butText_center);
  sprintf(curText, "%d", m->hcap);
  m->hcapIn = butTextin_create(newHcap, m, m->win, 1,
			       BUT_DRAWABLE|BUT_PRESSABLE, curText, 10);
  flags = BUT_DRAWABLE;
  if (m->hcap < 9)
    flags |= BUT_PRESSABLE;
  m->hcapUp = butAct_create(changeHcap, m, m->win, 1, flags, CGBUTS_FWDCHAR,
			    BUT_RRIGHT|BUT_SLEFT);
  flags = BUT_DRAWABLE;
  if (m->hcap > 0)
    flags |= BUT_PRESSABLE;
  m->hcapDown = butAct_create(changeHcap, m, m->win, 1, flags, CGBUTS_BACKCHAR,
			      BUT_RLEFT|BUT_SRIGHT);

  m->komiStr = butText_create(m->win, 1, BUT_DRAWABLE,
			       msg_komi, butText_center);
  sprintf(curText, "%g", m->komi);
  m->komiIn = butTextin_create(newKomi, m, m->win, 1,
			       BUT_DRAWABLE|BUT_PRESSABLE, curText, 10);
  m->komiUp = butAct_create(changeKomi, m, m->win, 1,
			    BUT_DRAWABLE|BUT_PRESSABLE, CGBUTS_FWDCHAR,
			    BUT_RRIGHT|BUT_SLEFT);
  m->komiDown = butAct_create(changeKomi, m, m->win, 1,
			      BUT_DRAWABLE|BUT_PRESSABLE, CGBUTS_BACKCHAR,
			      BUT_RLEFT|BUT_SRIGHT);

  m->timeBox = butBox_create(m->win, 1, BUT_DRAWABLE);
  butBox_setPixmaps(m->timeBox, cg->bgLitPixmap, cg->bgShadPixmap);

  m->mainStr = butText_create(m->win, 1, BUT_DRAWABLE,
			      msg_primaryTime, butText_center);
  m->mainIn = butTextin_create(newMainTime, m, m->win, 1,
			       BUT_PRESSABLE|BUT_DRAWABLE,
			       clp_getStr(cg->clp, "client.match.mainTime"),
			       10);
  m->mainUp1 = butAct_create(changeMain, m, m->win, 1,
			     BUT_DRAWABLE|BUT_PRESSABLE, CGBUTS_FWDCHAR,
			     BUT_SRIGHT|BUT_SLEFT);
  flags = BUT_DRAWABLE;
  m->mainTime = goTime_parseChars(butTextin_get(m->mainIn), FALSE, NULL);
  if (m->mainTime > 0)
    flags |= BUT_PRESSABLE;
  m->mainDown1 = butAct_create(changeMain, m, m->win, 1, flags,
			       CGBUTS_BACKCHAR, BUT_SRIGHT|BUT_SLEFT);
  m->mainUp5 = butAct_create(changeMain, m, m->win, 1,
			     BUT_DRAWABLE|BUT_PRESSABLE, CGBUTS_FFCHAR,
			     BUT_RRIGHT|BUT_SLEFT);
  m->mainDown5 = butAct_create(changeMain, m, m->win, 1, flags,
			       CGBUTS_REWCHAR, BUT_RLEFT|BUT_SRIGHT);

  m->byStr = butText_create(m->win, 1, BUT_DRAWABLE,
			     msg_byoYomi, butText_center);
  m->byIn = butTextin_create(newBYTime, m, m->win, 1,
			     BUT_PRESSABLE|BUT_DRAWABLE,
			     clp_getStr(cg->clp, "client.match.byTime"), 10);
  m->byTime = goTime_parseChars(butTextin_get(m->byIn), FALSE, NULL);
  m->byUp1 = butAct_create(changeBy, m, m->win, 1, BUT_DRAWABLE|BUT_PRESSABLE,
			   CGBUTS_FWDCHAR, BUT_SRIGHT|BUT_SLEFT);
  flags = BUT_DRAWABLE;
  if (m->byTime > 0)
    flags |= BUT_PRESSABLE;
  m->byDown1 = butAct_create(changeBy, m, m->win, 1, flags, CGBUTS_BACKCHAR,
			     BUT_SRIGHT|BUT_SLEFT);
  m->byUp5 = butAct_create(changeBy, m, m->win, 1, BUT_DRAWABLE|BUT_PRESSABLE,
			   CGBUTS_FFCHAR, BUT_RRIGHT|BUT_SLEFT);
  m->byDown5 = butAct_create(changeBy, m, m->win, 1, flags, CGBUTS_REWCHAR,
			     BUT_RLEFT|BUT_SRIGHT);

  m->freeTitle = butText_create(m->win, 1, BUT_DRAWABLE|BUT_PRESSABLE,
				msg_freeGame, butText_center);
  m->freeGame = butCb_create(newFree, m, m->win, 1, BUT_DRAWABLE|BUT_PRESSABLE,
			     clp_getBool(cg->clp, "client.match.free"));

  m->help = butCt_create(cgoban_createHelpWindow,
			  &help_cliMatch, m->win, 1,
			  BUT_DRAWABLE|BUT_PRESSABLE, msg_help);
  m->ok = butCt_create(okPressed, m, m->win, 1,
			BUT_DRAWABLE|BUT_PRESSABLE, msg_ok);
  m->cancel = butCt_create(cancelPressed, m, m->win, 1,
			    BUT_DRAWABLE|BUT_PRESSABLE, msg_cancel);

  return(m);
}


void  cliMatch_destroy(CliMatch *match)  {
  assert(MAGIC(match));
  butWin_destroy(match->win);
}


bool  cliMatch_destroyChain(CliMatch *match, const char *oppName,
			    int *hcap, float *komi, bool *free)  {
  bool  result = FALSE;

  /*
   * This code wouldn't work if things were done right in the but library
   *   and the windows were destroyed as soon as I asked for it.  But since
   *   the library is too broken for me to fix right now I'll have to do it
   *   this way.  The second assert in the loop will fail when the but library
   *   is rewritten to do things right.
   */
  while (match != NULL)  {
    assert(MAGIC(match));
    if (oppName != NULL)  {
      if (!strcmp(oppName, str_chars(&match->wName)) ||
	  !strcmp(oppName, str_chars(&match->bName)))  {
	result = TRUE;
	*hcap = match->hcap;
	*komi = match->komi;
	*free = butCb_get(match->freeGame);
	match->state = cliMatch_nil;
      }
    }
    cliMatch_destroy(match);
    assert(MAGIC(match));
    match = match->next;
  }
  return(result);
}


static ButOut  resize(ButWin *win)  {
  CliMatch  *m = butWin_packet(win);
  Cgoban  *cg;
  int  w, butW, h, x, y;
  int  bw, fh;
  ButEnv  *env;

  assert(MAGIC(m));
  cg = m->data->cg;
  assert(MAGIC(cg));
  env = cg->env;
  bw = butEnv_stdBw(env);
  fh = cg->fontH;
  w = butWin_w(win);
  h = butWin_h(win);
  butW = w/2-5*bw;

  but_resize(m->bg, 0,0, w,h);
  but_resize(m->title, 0,bw*2, w, fh*2);

  but_resize(m->namesBox, bw,bw*3+fh*2, w-bw*2,bw*4+fh*2);
  but_resize(m->wBox, bw*3, bw*5+fh*2,
	     w/2 - (bw * 4 + fh * 4), fh * 2);
  but_resize(m->wTitle, bw * 5, bw * 5 + fh * 2,
	     w/2 - (bw * 8 + fh * 4), fh * 2);
  but_resize(m->bBox, w/2 + bw + fh*4, bw*5+fh*2,
	     w - (w/2+bw*4 + fh * 4), fh*2);
  but_resize(m->bTitle, w/2 + bw * 3 + fh*4, bw*5+fh*2,
	     w - (w/2+bw*8 + fh * 4), fh*2);

  but_resize(m->swap, w/2 - fh * 4, bw*5+fh*2, fh * 8, fh * 2);

  y = bw*7+fh*4;
  but_resize(m->rulesBox, bw,y, w/2-bw,fh*12+bw*9);

  but_resize(m->sizeStr, bw*3,y+bw*2+fh*0, butW,fh*2);
  but_resize(m->sizeIn, bw*3+fh*2,y+bw*3+fh*2, butW-fh*4,fh*2);
  but_resize(m->sizeDown, bw*3,y+bw*3+fh*2, fh*2,fh*2);
  but_resize(m->sizeUp, bw*3+butW-fh*2,y+bw*3+fh*2, fh*2,fh*2);

  but_resize(m->hcapStr, bw*3,y+bw*4+fh*4, butW,fh*2);
  but_resize(m->hcapIn, bw*3+fh*2,y+bw*5+fh*6, butW-fh*4,fh*2);
  but_resize(m->hcapDown, bw*3,y+bw*5+fh*6, fh*2,fh*2);
  but_resize(m->hcapUp, bw*3+butW-fh*2,y+bw*5+fh*6, fh*2,fh*2);

  but_resize(m->komiStr, bw*3,y+bw*6+fh*8, butW,fh*2);
  but_resize(m->komiIn, bw*3+fh*2,y+bw*7+fh*10, butW-fh*4,fh*2);
  but_resize(m->komiDown, bw*3,y+bw*7+fh*10, fh*2,fh*2);
  but_resize(m->komiUp, bw*3+butW-fh*2,y+bw*7+fh*10, fh*2,fh*2);

  but_resize(m->timeBox, w/2,y, w-bw-w/2,fh*12+bw*9);
  x = w/2 + bw*2;
  butW = w-x-bw*3;

  but_resize(m->mainStr, x,y+bw*2+fh*0, butW,fh*2);
  but_resize(m->mainIn, x+fh*4,y+bw*3+fh*2, butW-fh*8,fh*2);
  but_resize(m->mainDown5, x,y+bw*3+fh*2, fh*2,fh*2);
  but_resize(m->mainDown1, x+fh*2,y+bw*3+fh*2, fh*2,fh*2);
  but_resize(m->mainUp1, x+butW-fh*4,y+bw*3+fh*2, fh*2,fh*2);
  but_resize(m->mainUp5, x+butW-fh*2,y+bw*3+fh*2, fh*2,fh*2);

  but_resize(m->byStr, x,y+bw*4+fh*4, butW,fh*2);
  but_resize(m->byIn, x+fh*4,y+bw*5+fh*6, butW-fh*8,fh*2);
  but_resize(m->byDown5, x,y+bw*5+fh*6, fh*2,fh*2);
  but_resize(m->byDown1, x+fh*2,y+bw*5+fh*6, fh*2,fh*2);
  but_resize(m->byUp1, x+butW-fh*4,y+bw*5+fh*6, fh*2,fh*2);
  but_resize(m->byUp5, x+butW-fh*2,y+bw*5+fh*6, fh*2,fh*2);

  but_resize(m->freeTitle, x,y+bw*6+fh*8, butW,fh*2);
  but_resize(m->freeGame, x + butW/2 - fh,y+bw*7+fh*10, fh*2,fh*2);

  but_resize(m->help, bw*2,y+bw*10+fh*12, (w-bw*6+1)/3,fh*2);
  but_resize(m->cancel, bw*3+(w-bw*6+1)/3,y+bw*10+fh*12,
	     w-2*(bw*3+(w-bw*6+1)/3),fh*2);
  but_resize(m->ok, w-(bw*2+(w-bw*6+1)/3),y+bw*10+fh*12,
	     (w-bw*6+1)/3,fh*2);

  return(0);
}


static ButOut  destroy(ButWin *win)  {
  CliMatch  *m;
  Clp  *clp;
  Str  *opp;

  m = butWin_packet(win);
  assert(MAGIC(m));
  if (m->state != cliMatch_nil)  {
    if (m->meFirst)
      opp = &m->wName;
    else
      opp = &m->bName;
    str_print(&m->data->cmdBuild, "decline %s\n", str_chars(opp));
    cliConn_send(&m->data->conn, str_chars(&m->data->cmdBuild));
  }
  clp = m->data->cg->clp;
  clp_setInt(clp, "client.match.x", butWin_x(win));
  clp_setInt(clp, "client.match.y", butWin_y(win));
  clp_setBool(clp, "client.match.meFirst", m->meFirst);
  str_deinit(&m->wName);
  str_deinit(&m->bName);
  *(m->prev) = m->next;
  MAGIC_UNSET(m);
  wms_free(m);
  return(0);
}


static ButOut  newSize(But *but, const char *value)  {
  CliMatch  *m = but_packet(but);
  int  newSize;
  char  curValue[3];

  assert(MAGIC(m));
  newSize = atoi(value);
  if ((newSize > 19) ||
      !clp_setStr(m->data->cg->clp, "client.match.size", value))  {
    str_print(&m->data->cmdBuild, msg_badSize, value, 19);
    cgoban_createMsgWindow(m->data->cg, "Cgoban Error",
			   str_chars(&m->data->cmdBuild));
    sprintf(curValue, "%d", m->size);
    butTextin_set(but, curValue, FALSE);
    return(BUTOUT_ERR);
  }
  m->size = newSize;
  if (m->size > 9)  {
    but_setFlags(m->sizeDown, BUT_PRESSABLE);
  } else  {
    but_setFlags(m->sizeDown, BUT_NOPRESS);
  }
  if (m->size < 19)
    but_setFlags(m->sizeUp, BUT_PRESSABLE);
  else
    but_setFlags(m->sizeUp, BUT_NOPRESS);
  return(0);
}


static ButOut  newHcap(But *but, const char *value)  {
  CliMatch  *m = but_packet(but);
  int  newHcap;
  char  curValue[2];
  bool  err;

  assert(MAGIC(m));
  newHcap = wms_atoi(value, &err);
  if ((newHcap > 9) || (newHcap == 1) || (newHcap < 0) || err)  {
    str_print(&m->data->cmdBuild, msg_badHcap, value, 9);
    cgoban_createMsgWindow(m->data->cg, "Cgoban Error",
			   str_chars(&m->data->cmdBuild));
    sprintf(curValue, "%d", m->hcap);
    butTextin_set(but, curValue, FALSE);
    return(BUTOUT_ERR);
  }
  m->hcap = newHcap;
  if (m->hcap > 0)  {
    but_setFlags(m->hcapDown, BUT_PRESSABLE);
  } else  {
    but_setFlags(m->hcapDown, BUT_NOPRESS);
  }
  if (m->hcap < 9)
    but_setFlags(m->hcapUp, BUT_PRESSABLE);
  else
    but_setFlags(m->hcapUp, BUT_NOPRESS);
  return(0);
}


static ButOut  newKomi(But *but, const char *value)  {
  CliMatch  *m = but_packet(but);
  float  newKomi;
  char  curValue[7];
  bool  err;

  assert(MAGIC(m));
  newKomi = wms_atof(value, &err);
  if ((newKomi > 999.5) || (newKomi < -999.5) || err ||
      ((int)(newKomi * 2.0) != newKomi * 2.0))  {
    str_print(&m->data->cmdBuild, msg_badKomi, value);
    cgoban_createMsgWindow(m->data->cg, "Cgoban Error",
			   str_chars(&m->data->cmdBuild));
    sprintf(curValue, "%g", m->komi);
    butTextin_set(but, curValue, FALSE);
    return(BUTOUT_ERR);
  }
  m->komi = newKomi;
  if (m->komi > -99.5)  {
    but_setFlags(m->hcapDown, BUT_PRESSABLE);
  } else  {
    but_setFlags(m->hcapDown, BUT_NOPRESS);
  }
  if (m->hcap < 99.5)
    but_setFlags(m->hcapUp, BUT_PRESSABLE);
  else
    but_setFlags(m->hcapUp, BUT_NOPRESS);
  return(0);
}


static ButOut  newMainTime(But *but, const char *value)  {
  CliMatch  *m = but_packet(but);
  Str  errMsg;

  assert(MAGIC(m));
  if (!clp_setStr(m->data->cg->clp, "client.match.mainTime", value))  {
    str_init(&errMsg);
    str_print(&errMsg, msg_badTime, value);
    cgoban_createMsgWindow(m->data->cg, "Cgoban Error", str_chars(&errMsg));
    goTime_str(m->mainTime, &errMsg);
    butTextin_set(but, str_chars(&errMsg), FALSE);
    str_deinit(&errMsg);
    return(BUTOUT_ERR);
  }
  m->mainTime = goTime_parseChars(value, FALSE, NULL);
  if (but_getFlags(but) & BUT_KEYED)
    but_setFlags(m->byIn, BUT_KEYED);
  if (m->mainTime > 0)  {
    but_setFlags(m->mainDown1, BUT_PRESSABLE);
    but_setFlags(m->mainDown5, BUT_PRESSABLE);
  } else  {
    but_setFlags(m->mainDown1, BUT_NOPRESS);
    but_setFlags(m->mainDown5, BUT_NOPRESS);
  }
  return(0);
}


static ButOut  newBYTime(But *but, const char *value)  {
  CliMatch  *m = but_packet(but);
  Str  errMsg;

  assert(MAGIC(m));
  if (!clp_setStr(m->data->cg->clp, "client.match.byTime", value))  {
    str_init(&errMsg);
    str_print(&errMsg, msg_badTime, value);
    cgoban_createMsgWindow(m->data->cg, "Cgoban Error", str_chars(&errMsg));
    goTime_str(m->byTime, &errMsg);
    butTextin_set(but, str_chars(&errMsg), FALSE);
    str_deinit(&errMsg);
    return(BUTOUT_ERR);
  }
  m->byTime = goTime_parseChars(value, FALSE, NULL);
  but_setFlags(but, BUT_NOKEY);
  if (m->byTime > 0)  {
    but_setFlags(m->byDown1, BUT_PRESSABLE);
    but_setFlags(m->byDown5, BUT_PRESSABLE);
  } else  {
    but_setFlags(m->byDown1, BUT_NOPRESS);
    but_setFlags(m->byDown5, BUT_NOPRESS);
  }
  return(0);
}


static ButOut  okPressed(But *but)  {
  CliMatch  *m = but_packet(but);
  const char  *oppName, *myName;
  ButOut  result;

  assert(MAGIC(m));
  if (m->meFirst)  {
    oppName = str_chars(&m->wName);
    myName = str_chars(&m->bName);
  } else  {
    myName = str_chars(&m->wName);
    oppName = str_chars(&m->bName);
  }
  result = newSize(m->sizeIn, butTextin_get(m->sizeIn)) |
    newMainTime(m->mainIn, butTextin_get(m->mainIn)) |
      newBYTime(m->byIn, butTextin_get(m->byIn));
  if (!(result & BUTOUT_ERR))  {
    str_print(&m->data->cmdBuild,
	      "match %s %c %d %d %d\n"
	      "tell %s CLIENT: <cgoban " VERSION "> match %s wants "
	      "handicap %d, komi %g",
	      oppName,
	      (int)("wb"[m->meFirst]),
	      m->size, (m->mainTime + 59) / 60, (m->byTime + 59) / 60,
	      oppName, myName, m->hcap, m->komi);
    if (butCb_get(m->freeGame))
      str_catChars(&m->data->cmdBuild, ", free\n");
    else
      str_catChar(&m->data->cmdBuild, '\n');
    cliConn_send(&m->data->conn, str_chars(&m->data->cmdBuild));
    m->state = cliMatch_sent;
    butText_set(m->title, msg_cliGameSetupSent);
  }
  return(result);
}


static ButOut  cancelPressed(But *but)  {
  butWin_destroy(but_win(but));
  return(0);
}


static ButOut  newFree(But *but, bool newVal)  {
  CliMatch  *m;

  m = but_packet(but);
  assert(MAGIC(m));
  clp_setBool(m->data->cg->clp, "client.match.free", newVal);
  return(0);
}


static ButOut  swapPressed(But *but)  {
  CliMatch  *m;
  Str  temp, *cmdBuild;

  m = but_packet(but);
  assert(MAGIC(m));
  temp = m->wName;
  m->wName = m->bName;
  m->bName = temp;
  m->meFirst = !m->meFirst;
  cmdBuild = &m->data->cmdBuild;
  str_print(cmdBuild, CGBUTS_WSTONECHAR " %s", str_chars(&m->wName));
  butText_set(m->wTitle, str_chars(cmdBuild));
  str_print(cmdBuild, CGBUTS_BSTONECHAR " %s", str_chars(&m->bName));
  butText_set(m->bTitle, str_chars(cmdBuild));
  return(0);
}


void  cliMatch_declineCommand(CliMatch *match, const char *buf)  {
  char  name[30];

  sscanf(buf, "%s", name);
  match = playerLookup(match, name);
  if (match != NULL)  {
    match->state = cliMatch_nil;
    butText_set(match->title, msg_cliGameSetupRej);
  }
}


static CliMatch  *playerLookup(CliMatch *chain, const char *player)  {
  while (chain != NULL)  {
    if (!strcmp(player, str_chars(&chain->wName)) ||
	!strcmp(player, str_chars(&chain->bName)))
      return(chain);
    chain = chain->next;
  }
  return(NULL);
}


static ButOut  changeMain(But *but)  {
  CliMatch  *m = but_packet(but);

  assert(MAGIC(m));
  if (newMainTime(m->mainIn, butTextin_get(m->mainIn)) & BUTOUT_ERR)
    return(BUTOUT_ERR);
  if (but == m->mainUp1)  {
    m->mainTime += 60;
  } else if (but == m->mainUp5)  {
    m->mainTime += 60 * 5;
  } else if (but == m->mainDown1)  {
    m->mainTime -= 60;
    if (m->mainTime < 0)
      m->mainTime = 0;
  } else  {
    assert(but == m->mainDown5);
    m->mainTime -= 60 * 5;
    if (m->mainTime < 0)
      m->mainTime = 0;
  }
  goTime_str(m->mainTime, &m->data->cmdBuild);
  butTextin_set(m->mainIn, str_chars(&m->data->cmdBuild), TRUE);
  return(0);
}


static ButOut  changeBy(But *but)  {
  CliMatch  *m = but_packet(but);

  assert(MAGIC(m));
  if (newBYTime(m->byIn, butTextin_get(m->byIn)) & BUTOUT_ERR)
    return(BUTOUT_ERR);
  if (but == m->byUp1)  {
    m->byTime += 60;
  } else if (but == m->byUp5)  {
    m->byTime += 60 * 5;
  } else if (but == m->byDown1)  {
    m->byTime -= 60;
    if (m->byTime < 0)
      m->byTime = 0;
  } else  {
    assert(but == m->byDown5);
    m->byTime -= 60 * 5;
    if (m->byTime < 0)
      m->byTime = 0;
  }
  goTime_str(m->byTime, &m->data->cmdBuild);
  butTextin_set(m->byIn, str_chars(&m->data->cmdBuild), TRUE);
  return(0);
}


static ButOut  changeSize(But *but)  {
  CliMatch  *m = but_packet(but);
  char  sizeStr[3];

  assert(MAGIC(m));
  if (newSize(m->sizeIn, butTextin_get(m->sizeIn)) & BUTOUT_ERR)
    return(BUTOUT_ERR);
  if (but == m->sizeUp)  {
    if (m->size < 13)
      m->size = 13;
    else
      m->size = 19;
  } else  {
    assert(but == m->sizeDown);
    if (m->size > 13)
      m->size = 13;
    else
      m->size = 9;
  }
  sprintf(sizeStr, "%d", m->size);
  butTextin_set(m->sizeIn, sizeStr, TRUE);
  return(0);
}


static ButOut  changeKomi(But *but)  {
  CliMatch  *m = but_packet(but);
  char  komiStr[10];

  assert(MAGIC(m));
  if (newKomi(m->komiIn, butTextin_get(m->komiIn)) & BUTOUT_ERR)
    return(BUTOUT_ERR);
  if (but == m->komiUp)  {
    m->komi += 1.0;
    if (m->komi > 99.5)
      m->komi = 99.5;
  } else  {
    assert(but == m->komiDown);
    m->komi -= 1.0;
    if (m->komi < -99.5)
      m->komi = -99.5;
  }
  sprintf(komiStr, "%g", m->komi);
  butTextin_set(m->komiIn, komiStr, TRUE);
  return(0);
}


static ButOut  changeHcap(But *but)  {
  CliMatch  *m = but_packet(but);
  char  hcapStr[3];

  assert(MAGIC(m));
  if (newHcap(m->hcapIn, butTextin_get(m->hcapIn)) & BUTOUT_ERR)
    return(BUTOUT_ERR);
  if (but == m->hcapUp)  {
    ++m->hcap;
    if (m->hcap == 1)
      m->hcap = 2;
    if (m->hcap > 9)
      m->hcap = 9;
  } else  {
    assert(but == m->hcapDown);
    --m->hcap;
    if (m->hcap == 1)
      m->hcap = 0;
    if (m->hcap < 0)
      m->hcap = 0;
  }
  sprintf(hcapStr, "%d", m->hcap);
  butTextin_set(m->hcapIn, hcapStr, TRUE);
  return(0);
}


void  cliMatch_extraInfo(CliMatch *match, const char *oppName, int hcap,
			 float komi, bool free)  {
  char  temp[20];

  match = playerLookup(match, oppName);
  if (match != NULL)  {
    sprintf(temp, "%d", hcap);
    butTextin_set(match->hcapIn, temp, TRUE);
    sprintf(temp, "%g", komi);
    butTextin_set(match->komiIn, temp, TRUE);
    match->komi = komi;
    butCb_set(match->freeGame, free, TRUE);
  }
}
