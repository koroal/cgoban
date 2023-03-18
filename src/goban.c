/*
 * $Source: /cvsroot/cgoban1/cgoban1/src/goban.c,v $
 * $Revision: 1.2 $
 * $Date: 2002/02/28 03:34:35 $
 *
 * src/goban.c, part of Complete Goban (game program)
 * Copyright (C) 1995-1997 William Shubert
 * See "configure.h.in" for more copyright information.
 */


#include <wms.h>
#include <wms/rnd.h>
#include <but/but.h>
#include <but/plain.h>
#include <but/ctext.h>
#include <but/text.h>
#include <abut/term.h>
#include <abut/msg.h>
#include <wms/clp.h>
#include <wms/str.h>
#include <but/timer.h>
#include <but/textin.h>
#include <but/radio.h>
#include "cgoban.h"
#include "msg.h"
#include "goGame.h"
#include "cgbuts.h"
#include "goPic.h"
#ifdef  _GOBAN_H_
#error  Levelization Error.
#endif
#include "goban.h"


/**********************************************************************
 * Forward references
**********************************************************************/
static ButOut  resize(ButWin *win);
static ButOut  iResize(ButWin *win);
static ButOut  destroy(ButWin *win);
static ButOut  msgBoxKilled(But *okBut);
static void  startMoveTimer(Goban *g);
static void  stopMoveTimer(Goban *g);
static ButOut  newTime(ButTimer *t);
static ButOut  dispatchGrid(void *packet, int loc);
static ButOut  dispatchQuit(But *but);
static ButOut  dispatchPass(But *but);
static ButOut  dispatchRew(But *but);
static ButOut  dispatchBack(But *but);
static ButOut  dispatchFwd(But *but);
static ButOut  dispatchFf(But *but);
static ButOut  dispatchDone(But *but);
static ButOut  dispatchDispute(But *but);
static ButOut  dispatchSave(But *but);
static ButOut  dispatchEdit(But *but);
static ButOut  dispatchInfo(But *but);
static ButOut  gotKibitz(But *but, const char *value);
static ButOut  newKibVal(But *but, int value);


/**********************************************************************
 * Functions
 **********************************************************************/
/*
 * This is an ugly kludge.
 */
#define  WIN_W(fontH, bw)  \
  (fontH*3+bw*2 + fontH*12+bw*5 +  \
   (fontH*2+bw)*12+fontH*2+bw*6)


Goban  *goban_create(Cgoban *cg, const GobanActions *actions,void *packet,
		     GoGame *game, const char *clpName, const char *title)  {
  static ButKey  rewKeys[] = {{XK_Home, 0,0}, {0,0,0}};
  static ButKey  backKeys[] = {{XK_Left, 0,0}, {0,0,0}};
  static ButKey  fwdKeys[] = {{XK_Right, 0,0}, {0,0,0}};
  static ButKey  ffKeys[] = {{XK_End, 0,0}, {0,0,0}};
  Goban  *g;
  int  winSize, i;
  GoStone  s;
  GobanPlayerInfo  *pi;
  Str  clpStrs;
  bool  err;
  uint  butFlags;

  assert(MAGIC(cg));
  g = wms_malloc(sizeof(Goban));
  MAGIC_SET(g);
  g->cg = cg;
  g->actions = actions;
  g->packet = packet;
  g->propagateDestroy = TRUE;
  g->game = game;
  goTimer_init(&g->timers[goStone_black], &game->time);
  goTimer_init(&g->timers[goStone_white], &game->time);
  g->activeTimer = goStone_empty;

  g->ingTimePenalties[goStone_black] = g->ingTimePenalties[goStone_white] = 0;
  g->lastMove = 0;
  g->lastScores[goStone_black] = g->lastScores[goStone_white] = 0.0;
  str_init(&clpStrs);
  str_print(&clpStrs, "%s.bProp", clpName);
  g->bProp = clp_lookup(cg->clp, str_chars(&clpStrs));
  str_print(&clpStrs, "%s.bPropW", clpName);
  g->bPropW = clp_lookup(cg->clp, str_chars(&clpStrs));
  str_print(&clpStrs, "%s.x", clpName);
  g->clpX = clp_lookup(cg->clp, str_chars(&clpStrs));
  str_print(&clpStrs, "%s.y", clpName);
  g->clpY = clp_lookup(cg->clp, str_chars(&clpStrs));
  str_deinit(&clpStrs);
  winSize = WIN_W(cg->fontH, butEnv_stdBw(cg->env));
  
  g->win = butWin_iCreate(g, cg->env, title,
			  (int)(winSize * clpEntry_getDouble(g->bPropW) + 0.5),
			  (int)(winSize * clpEntry_getDouble(g->bProp) + 0.5),
			  &g->iWin, FALSE, 64,64, NULL, NULL, resize,
			  iResize, destroy);
  i = clpEntry_iGetInt(g->clpX, &err);
  if (!err)  {
    butWin_setX(g->win, i);
  }
  i = clpEntry_iGetInt(g->clpY, &err);
  if (!err)  {
    butWin_setY(g->win, i);
  }
  butWin_setMinH(g->win, winSize);
  butWin_setMaxH(g->win, 0);
  butWin_setMinW(g->win, winSize / 2);
  butWin_setMaxW(g->win, 0);
  butWin_setWHRatios(g->win, 1, 1000, 1, 1);
  butWin_activate(g->win);

  g->bg = butBoxFilled_create(g->win, 0, BUT_DRAWABLE);
  butBoxFilled_setPixmaps(g->bg, cg->bgLitPixmap, cg->bgShadPixmap,
			  cg->bgPixmap);
  g->labelBox = butBoxFilled_create(g->win, 1, BUT_DRAWABLE);
  g->labelText = butText_create(g->win, 2, BUT_DRAWABLE,
				title, butText_center);
  butText_setFont(g->labelText, 2);
  g->infoText = butText_create(g->win, 2, BUT_DRAWABLE, "", butText_center);
  if (actions->gameInfoPressed)
    butFlags = BUT_DRAWABLE | BUT_PRESSABLE;
  else
    butFlags = BUT_DRAWABLE;
  g->gameInfo = butCt_create(dispatchInfo, g, g->win, 1, butFlags,
			     msg_gameInfo);
  g->help = butCt_create(cgoban_createHelpWindow, actions->helpInfo, g->win,
			 1, BUT_DRAWABLE|BUT_PRESSABLE,
			 msg_help);

  g->pass = butCt_create(dispatchPass, g, g->win, 1,
			 BUT_DRAWABLE|BUT_PRESSABLE,
			 msg_pass);
  if ((game->rules == goRules_japanese) || (game->rules == goRules_tibetan))
    g->resume = butCt_create(dispatchDispute, g, g->win, 1,
			     BUT_DRAWABLE,
			     msg_dispute);
  else
    g->resume = butCt_create(dispatchBack, g, g->win, 1,
			     BUT_DRAWABLE,
			     msg_resume);
  g->done = butCt_create(dispatchDone, g, g->win, 1,
			 BUT_DRAWABLE,
			 msg_done);
  g->rew = butAct_create(dispatchRew, g, g->win, 1,
			 BUT_DRAWABLE|BUT_PRESSABLE,
			 CGBUTS_REWCHAR, BUT_RLEFT|BUT_SRIGHT);
  but_setKeys(g->rew, rewKeys);
  g->back = butAct_create(dispatchBack, g, g->win, 1,
			  BUT_DRAWABLE|BUT_PRESSABLE,
			  CGBUTS_BACKCHAR, BUT_SLEFT|BUT_SRIGHT);
  but_setKeys(g->back, backKeys);
  g->fwd = butAct_create(dispatchFwd, g, g->win, 1,
			 BUT_DRAWABLE|BUT_PRESSABLE,
			 CGBUTS_FWDCHAR, BUT_SLEFT|BUT_SRIGHT);
  but_setKeys(g->fwd, fwdKeys);
  g->ff = butAct_create(dispatchFf, g, g->win, 1,
			BUT_DRAWABLE|BUT_PRESSABLE,
			CGBUTS_FFCHAR, BUT_SLEFT|BUT_RRIGHT);
  but_setKeys(g->ff, ffKeys);

  for (s = goStone_white;  s <= goStone_black;  ++s)  {
    pi = &g->playerInfos[s];
    pi->box = butBoxFilled_create(g->win, 1, BUT_DRAWABLE);
    for (i = 0;  i < 5;  ++i)  {
      assert(i < CGBUTS_NUMWHITE);
      pi->stones[i] = grid_create(&g->cg->cgbuts, NULL, NULL, g->win, 2,
				  BUT_DRAWABLE, 0);
      grid_setStone(pi->stones[i], s, FALSE);
      grid_setLineGroup(pi->stones[i], gridLines_none);
      grid_setVersion(pi->stones[i], i);
    }
    pi->capsLabel = butText_create(g->win, 2, BUT_DRAWABLE,
				   msg_score, butText_left);
    pi->capsOut = butText_create(g->win, 3, BUT_DRAWABLE, "0", butText_right);
    butText_setColor(pi->capsOut, CGBUTS_COLORREDLED, FALSE);
    pi->capsBox = butBoxFilled_create(g->win, 2, BUT_DRAWABLE);
    butBoxFilled_setColors(pi->capsBox, BUT_SHAD,BUT_LIT,BUT_FG);
    pi->timeLabel = butText_create(g->win, 2, BUT_DRAWABLE,
				   msg_time, butText_left);
    pi->timeOut = butText_create(g->win, 3, BUT_DRAWABLE, "",
				 butText_right);
    butText_setColor(pi->timeOut, CGBUTS_COLORREDLED, FALSE);
    pi->timeBox = butBoxFilled_create(g->win, 2, BUT_DRAWABLE);
    butBoxFilled_setColors(pi->timeBox, BUT_SHAD,BUT_LIT,BUT_FG);
  }

  if (actions->savePressed)
    butFlags = BUT_DRAWABLE | BUT_PRESSABLE;
  else
    butFlags = BUT_DRAWABLE;
  g->save = butCt_create(dispatchSave, g, g->win, 1,
			 butFlags, msg_saveGame);
  if (actions->editPressed)
    butFlags = BUT_DRAWABLE | BUT_PRESSABLE;
  else
    butFlags = BUT_DRAWABLE;
  g->edit = butCt_create(dispatchEdit, g, g->win, 1, butFlags,
			 msg_editGame);
  g->quit = butCt_create(dispatchQuit, g, g->win, 1,
			 BUT_DRAWABLE|BUT_PRESSABLE,
			 msg_close);

  g->comments = abutTerm_create(cg->abut, g->win, 1, TRUE);
  g->kibIn = NULL;
  g->newKib = NULL;

  g->boardBox = butPixmap_create(g->win, 1, BUT_DRAWABLE, cg->bgShadPixmap);
  butPixmap_setPic(g->boardBox, cg->bgShadPixmap,
		   cg->fontH + butEnv_stdBw(cg->env) + cg->fontH / 2,
		   cg->fontH + butEnv_stdBw(cg->env) + cg->fontH / 2);
  g->pic = goPic_create(cg, g, game, g->win, 2, dispatchGrid,
			goBoard_size(game->board));

  g->iBg = butBoxFilled_create(g->iWin, 0, BUT_DRAWABLE);
  butBoxFilled_setPixmaps(g->iBg, cg->bgLitPixmap, cg->bgShadPixmap,
			  cg->bgPixmap);
  g->iBoard = gobanPic_create(&cg->cgbuts, g->iWin, 1, BUT_DRAWABLE);
  g->iDec1 = NULL;
  g->iDec2 = NULL;

  g->msgBox = NULL;
  g->clock = NULL;

  goban_update(g);
  return(g);
}


void  goban_destroy(Goban *g, bool propagate)  {
  assert(MAGIC(g));
  goPic_destroy(g->pic);
  g->pic = NULL;
  g->propagateDestroy = propagate;
  butWin_destroy(g->win);
}


static ButOut  resize(ButWin *win)  {
  Goban  *g = butWin_packet(win);
  Cgoban  *cg;
  int  butX,butY, butW,butH, butSpc, boxH, x, w;
  int  bigStoneW;
  int  winW, winH, boardW;
  int  bw;
  int  i, boardXPic, boardYPic;
  GoStone  s;
  GobanPlayerInfo  *pi;
  int  fontH;

  assert(MAGIC(g));
  cg = g->cg;
  assert(MAGIC(cg));
  bw = butEnv_stdBw(cg->env);
  winW = butWin_w(win);
  winH = butWin_h(win);
  fontH = cg->fontH;
  butW = fontH * 10 + bw*4;
  boardW = winW - butW - fontH*3 - bw*2;
  clpEntry_setDouble(g->bProp, (double)winH / (double)butWin_getMinH(win));
  clpEntry_setDouble(g->bPropW, (double)winW / (double)butWin_getMinW(win)/2.0);

  butH = fontH * 2;
  butSpc = fontH * 2 + bw;
  butY = fontH*2 + bw + butH + butSpc;
  bigStoneW = fontH;
  boxH = butSpc*3 + bw*3;
  boardXPic = fontH + bw;
  boardYPic = butY;
  butX = boardXPic + boardW + fontH;

  but_resize(g->bg, 0,0, winW,butWin_h(win));
  but_resize(g->labelBox, boardXPic,bw + fontH,
	     boardW,butH+butSpc);
  but_resize(g->labelText, bw*2 + fontH,bw*2 + fontH,
	     boardW-bw*2,(butH+butSpc-bw*2)/2);
  but_resize(g->infoText, bw*2 + fontH, bw*2 + fontH+(butH+butSpc-bw*2)/2,
	     boardW-bw*2,(butH+butSpc-bw*2)/2);
  but_resize(g->gameInfo, butX, fontH + bw, butW,butH);
  but_resize(g->help, butX, fontH + bw + butSpc, butW,butH);

  but_resize(g->pass, butX,butY, butW,butH);
  but_resize(g->resume, butX,butY+butSpc, butW,butH);
  but_resize(g->done, butX,butY+2*butSpc, butW,butH);

  x = butX;
  w = butW/4;
  but_resize(g->rew, x,butY+3*butSpc, w,butH);
  x += w;
  w = butW/2 - w;
  but_resize(g->back, x,butY+3*butSpc, w,butH);
  x += w;
  w = butW - (w + butW/2);
  but_resize(g->fwd, x,butY+3*butSpc, w,butH);
  x += w;
  but_resize(g->ff, x,butY+3*butSpc, butW/4,butH);

  butY += 4*butSpc + fontH;
  x = butEnv_textWidth(win->env, msg_time, 0);
  w = butEnv_textWidth(win->env, msg_score, 0);
  if (x < w)
    x = w;
  w = butW - x - 5*bw;
  x += butX + bw*3;
  for (s = goStone_white;  s <= goStone_black;  ++s)  {
    pi = &g->playerInfos[s];
    but_resize(pi->box, butX,butY, butW,boxH);
    for (i = 0;  i < 5;  ++i)
      but_resize(pi->stones[i], butX+bw*2+bigStoneW*2*i + fontH/2,
		 butY+bw*2 + fontH/2, fontH, fontH);
    butText_resize(pi->capsLabel, butX+bw*2,butY+butSpc+bw*2, butH);
    but_resize(pi->capsOut, x+bw*2,butY+bw*3+butSpc, w-bw*4,butH-bw*2);
    but_resize(pi->capsBox, x,butY+bw*2+butSpc, w,butH);
    butText_resize(pi->timeLabel, butX+bw*2,butY+butSpc*2+bw*2, butH);
    but_resize(pi->timeOut, x+bw*2,butY+butSpc*2+bw*3, w-bw*4,butH-bw*2);
    but_resize(pi->timeBox, x,butY+butSpc*2+bw*2, w,butH);
    butY += boxH+bw;
  }

  butY += fontH - bw;
  but_resize(g->save, butX,butY         , butW,butH);
  but_resize(g->edit, butX,butY+butSpc  , butW,butH);
  but_resize(g->quit, butX,butY+butSpc*2, butW,butH);

  butY += butSpc*3 + fontH - bw;
  if (butY < boardYPic + boardW + fontH)
    butY = boardYPic + boardW + fontH;
  if (g->kibIn == NULL)  {
    abutTerm_resize(g->comments, fontH+bw,butY,
		    winW - bw*2 - fontH*2,
		    winH - butY - fontH - bw);
  } else  {
    abutTerm_resize(g->comments, fontH+bw,butY,
		    winW - bw*2 - fontH*2,
		    winH - butY - fontH*3 - bw);
    but_resize(g->kibIn, fontH+bw, winH - fontH * 3 - bw,
	       winW - bw*2 - fontH*2 - butW, fontH * 2);
    but_resize(g->kibType, butX, winH - fontH * 3 - bw,
	       butW, fontH * 2);
    but_resize(g->kibSay, butX, winH - fontH * 3 - bw,
	       (butW + 1) / 3, fontH * 2);
    but_resize(g->kibBoth, butX + (butW + 1) / 3, winH - fontH * 3 - bw,
	       butW - ((butW + 1) / 3) * 2, fontH * 2);
    but_resize(g->kibKib, butX + butW - (butW + 1) / 3, winH - fontH * 3 - bw,
	       (butW + 1) / 3, fontH * 2);
  }
  but_resize(g->boardBox, boardXPic + fontH/2,boardYPic + fontH/2,
	     boardW,boardW);
  goPic_resize(g->pic, boardXPic,boardYPic, boardW,boardW);
  return(0);
}


static ButOut  iResize(ButWin *win)  {
  Goban  *g = butWin_packet(win);
  int  bw = butEnv_stdBw(win->env);
  int  w = butWin_w(win), h = butWin_h(win);

  assert(MAGIC(g));
  but_resize(g->iBg, 0,0, butWin_w(win),butWin_h(win));
  but_resize(g->iBoard, bw*2,bw*2, butWin_w(win)-bw*4,butWin_h(win)-bw*4);
  if (g->iDec1 && !g->iDec2)  {
    but_resize(g->iDec1, (w * 1 + 2) / 5, (h + 4) / 10,
	       (w * 3 + 2) / 5, (w * 3 + 2) / 5);
  } else if (g->iDec1 && g->iDec2)  {
    but_resize(g->iDec1, bw*2, (h + 4) / 10, (w + 1) / 3, (w + 1) / 3);
    but_resize(g->iDec2, w - bw*2 - (w + 1) / 3, (h + 4) / 10,
	       (w + 1) / 3, (w + 1) / 3);
  }
  return(0);
}


static ButOut  destroy(ButWin *win)  {
  Goban  *g = butWin_packet(win);
  const GobanActions  *actions;
  void  *packet;
  bool  propagate;

  assert(MAGIC(g));
  propagate = g->propagateDestroy;
  actions = g->actions;
  packet = g->packet;
  g->win = NULL;  /* It's dead already. */
  clpEntry_setInt(g->clpX, butWin_x(win));
  clpEntry_setInt(g->clpY, butWin_y(win));
  if (g->pic)
    goPic_destroy(g->pic);
  if (g->msgBox)
    abutMsg_destroy(g->msgBox, FALSE);
  MAGIC_UNSET(g);
  wms_free(g);
  if (propagate)  {
    actions->destroyCallback(packet);
  }
  return(0);
}


void  goban_update(Goban *g)  {
  Str  newText;
  char  newScore[40], locDesc[5];
  const char  *statusStr;
  GoStone  color;
  float  score;
  uint  defaultPassFlags;

  assert(MAGIC(g));
  str_init(&newText);
  if (g->game->state > goGameState_dispute)  {
    goScore_compute(&g->score, g->game);
  } else  {
    goScore_zero(&g->score);
  }
  goPic_update(g->pic);
  goban_updateTimeReadouts(g);
  goStoneIter(color)  {
    if (g->game->state <= goGameState_dispute)  {
      score = goGame_caps(g->game, color) - g->ingTimePenalties[color];
      if (color == goStone_white)
	score += g->game->komi;
    } else  {
      score = g->score.scores[color];
    }
    if (score != g->lastScores[color])  {
      g->lastScores[color] = score;
      sprintf(newScore, "%g", score);
      butText_set(g->playerInfos[color].capsOut, newScore);
    }
  }
  if ((g->pic->allowedMoves & goPicMove_noPass) ||
      ((g->game->whoseMove == goStone_white) &&
       (g->pic->allowedMoves & goPicMove_noWhite)) ||
      ((g->game->whoseMove == goStone_black) &&
       (g->pic->allowedMoves & goPicMove_noBlack)))
    defaultPassFlags = BUT_NOPRESS;
  else
    defaultPassFlags = BUT_PRESSABLE;
  switch(g->game->state)  {
  case goGameState_play:
    switch(g->game->moveNum)  {
    case 0:
      str_copyChars(&newText, msg_gameStartDesc);
      break;
    case 1:
      goBoard_loc2Str(g->game->board, g->game->moves[0].move, locDesc);
      if (g->game->maxMoves == 1)  {
	str_print(&newText, msg_move1Desc,
		  (int)goStone_char(goGame_whoseTurnOnMove(g->game, 0)),
		  locDesc);
      } else  {
	str_print(&newText, msg_move1OfDesc,
		  (int)goStone_char(goGame_whoseTurnOnMove(g->game, 0)),
		  locDesc, g->game->maxMoves);
      }
      break;
    default:
      goBoard_loc2Str(g->game->board,
		      g->game->moves[g->game->moveNum - 1].move, locDesc);
      if (g->game->maxMoves == g->game->moveNum)  {
	str_print(&newText, msg_moveNDesc, g->game->moveNum,
		  (int)goStone_char(goGame_whoseTurnOnMove(g->game,
							   g->game->moveNum -
							   1)),
		  locDesc);
      } else  {
	str_print(&newText, msg_moveNOfDesc, g->game->moveNum,
		  (int)goStone_char(goGame_whoseTurnOnMove(g->game,
							   g->game->moveNum -
							   1)),
		  locDesc, g->game->maxMoves);
      }
      break;
    }
    assert(goStone_isStone(goGame_whoseMove(g->game)));
    str_catChars(&newText,
		 msg_stoneNames[goGame_whoseMove(g->game)]);
    str_catChars(&newText, msg_toPlay);
    butText_set(g->infoText, str_chars(&newText));
    but_setFlags(g->pass, defaultPassFlags);
    but_setFlags(g->resume, BUT_NOPRESS);
    but_setFlags(g->done, BUT_NOPRESS);
    break;
  case goGameState_dispute:
    color = goGame_whoseMove(g->game);
    if (g->game->disputeAlive)
      statusStr = msg_alive;
    else
      statusStr = msg_dead;
    str_print(&newText, msg_disputeAnnounce,
	      statusStr, msg_stoneNames[color]);
    butText_set(g->infoText, str_chars(&newText));
    but_setFlags(g->pass, defaultPassFlags);
    but_setFlags(g->resume, BUT_NOPRESS);
    but_setFlags(g->done, BUT_NOPRESS);
    break;
  case goGameState_selectDead:
    butText_set(g->infoText, msg_selectDead);
    but_setFlags(g->pass, BUT_NOPRESS);
    if (g->actions->disputePressed)
      but_setFlags(g->resume, BUT_PRESSABLE);
    else
      but_setFlags(g->resume, BUT_NOPRESS);
    but_setFlags(g->done, BUT_PRESSABLE);
    break;
  case goGameState_selectDisputed:
    butText_set(g->infoText, msg_selectDisputed);
    but_setFlags(g->pass, BUT_NOPRESS);
    but_setFlags(g->resume, BUT_NOPRESS);
    but_setFlags(g->done, BUT_NOPRESS);
    break;
  case goGameState_done:
    if (!goTime_checkTimer(&g->game->time,
			   &g->timers[g->game->whoseMove]))  {
      str_print(&newText, msg_timeLossInfo,
		msg_stoneNames[g->game->whoseMove]);
      butText_set(g->infoText, str_chars(&newText));
    } else if (g->lastScores[goStone_white] > g->lastScores[goStone_black])
      butText_set(g->infoText, msg_whiteWon);
    else if ((g->lastScores[goStone_white] < g->lastScores[goStone_black]) ||
	     (g->game->rules == goRules_ing))
      butText_set(g->infoText, msg_blackWon);
    else
      butText_set(g->infoText, msg_jigo);
    but_setFlags(g->pass, BUT_NOPRESS);
    but_setFlags(g->resume, BUT_NOPRESS);
    but_setFlags(g->done, BUT_NOPRESS);
    break;
  }
  if (g->actions->rewOk(g->packet))
    but_setFlags(g->rew, BUT_PRESSABLE);
  else
    but_setFlags(g->rew, BUT_NOPRESS);
  if (g->actions->backOk(g->packet))
    but_setFlags(g->back, BUT_PRESSABLE);
  else
    but_setFlags(g->back, BUT_NOPRESS);
  if (g->actions->fwdOk(g->packet))  {
    but_setFlags(g->fwd, BUT_PRESSABLE);
  } else  {
    but_setFlags(g->fwd, BUT_NOPRESS);
  }
  if (g->actions->ffwdOk(g->packet))  {
    but_setFlags(g->ff, BUT_PRESSABLE);
  } else  {
    but_setFlags(g->ff, BUT_NOPRESS);
  }
  str_deinit(&newText);
}  


static ButOut  msgBoxKilled(But *okBut)  {
  Goban  *g = but_packet(okBut);

  assert(MAGIC(g));
  abutMsg_destroy(g->msgBox, FALSE);
  g->msgBox = NULL;
  return(0);
}


void  goban_message(Goban *g, const char *str)  {
  AbutMsgOpt  ok;
  
  assert(MAGIC(g));
  ok.name = msg_ok;
  ok.callback = msgBoxKilled;
  ok.packet = g;
  ok.keyEq = cg_return;
  if (g->msgBox)
    abutMsg_destroy(g->msgBox, FALSE);
  g->msgBox = abutMsg_optCreate(g->cg->abut, g->win, 4, str, NULL, NULL, 1,
				&ok);
}


void  goban_noMessage(Goban *g)  {
  if (g->msgBox)  {
    abutMsg_destroy(g->msgBox, FALSE);
    g->msgBox = NULL;
  }
}


static void  startMoveTimer(Goban *g)  {
  struct timeval  delay, period;

  /*
   * If we are in passive mode, then time does not go down.  This is used
   *   when editing SGF files, etc.
   */
  if (!g->game->passive)  {
    stopMoveTimer(g);
    delay.tv_sec = 0;
    delay.tv_usec = g->timers[g->activeTimer].usLeft + 100000;
    while (delay.tv_usec >= 1000000)
      delay.tv_usec -= 1000000;
    period.tv_sec = 1;
    period.tv_usec = 0;
    g->clock = butTimer_create(g, g->bg, delay, period, FALSE, newTime);
  }
}


static void  stopMoveTimer(Goban *g)  {
  if (g->clock)  {
    butTimer_destroy(g->clock);
    g->clock = NULL;
  }
}


static ButOut newTime(ButTimer *t)  {
  Goban  *g = butTimer_packet(t);
  int  newIngPenalty;

  assert(MAGIC(g));
  goTime_checkTimer(&g->game->time, &g->timers[g->activeTimer]);
  newIngPenalty = goTime_ingPenalty(&g->game->time, 
				    &g->timers[g->activeTimer]);
  if (newIngPenalty == g->ingTimePenalties[g->activeTimer])
    goban_updateTimeReadouts(g);
  else  {
    g->ingTimePenalties[g->activeTimer] = newIngPenalty;
    goban_update(g);
  }
  return(0);
}


void  goban_updateTimeReadouts(Goban *g)  {
  GoStone  color;
  Str  newText;
  int tLeft;

  assert(MAGIC(g));
  str_init(&newText);
  goStoneIter(color)  {
    goTime_remainStr(&g->game->time, &g->timers[color], &newText);
    if (strcmp(str_chars(&newText),
	       butText_get(g->playerInfos[color].timeOut))) {
      butText_set(g->playerInfos[color].timeOut, str_chars(&newText));
      tLeft = g->timers[color].timeLeft;
      if ((g->cg->cgbuts.timeWarn > 0) &&
	  goTime_low(&g->game->time,
		     &g->timers[color], g->cg->cgbuts.timeWarn) &&
	  ((tLeft & 1) == 0)) {
	butText_setColor(g->playerInfos[color].timeOut, BUT_BLACK, FALSE);
	butBoxFilled_setColors(g->playerInfos[color].timeBox,
			       BUT_SHAD, BUT_LIT, BUT_WHITE);
      } else {
	butText_setColor(g->playerInfos[color].timeOut, CGBUTS_COLORREDLED,
			 FALSE);
	butBoxFilled_setColors(g->playerInfos[color].timeBox,
			       BUT_SHAD, BUT_LIT, BUT_BLACK);
      }
    }
    if (g->game->whoseMove != color) {
      butText_setColor(g->playerInfos[color].timeOut, CGBUTS_COLORREDLED,
		       FALSE);
      butBoxFilled_setColors(g->playerInfos[color].timeBox,
			     BUT_SHAD, BUT_LIT, BUT_BLACK);
    }
  }
  str_deinit(&newText);
}


static ButOut  dispatchGrid(void *packet, int loc)  {
  Goban  *g = packet;
  GobanOut  out;

  assert(MAGIC(g));
  out = g->actions->gridPressed(g->packet, loc);
  if (out == gobanOut_draw)
    goban_update(g);
  if (out == gobanOut_err)
    return(BUTOUT_ERR);
  else
    return(0);
}


static ButOut  dispatchQuit(But *but)  {
  Goban  *g = but_packet(but);
  GobanOut  out;

  assert(MAGIC(g));
  out = g->actions->quitPressed(g->packet);
  if (out == gobanOut_draw)
    goban_update(g);
  if (out == gobanOut_err)
    return(BUTOUT_ERR);
  else
    return(0);
}


static ButOut  dispatchPass(But *but)  {
  Goban  *g = but_packet(but);
  GobanOut  out;

  assert(MAGIC(g));
  out = g->actions->passPressed(g->packet);
  if (out == gobanOut_draw)
    goban_update(g);
  if (out == gobanOut_err)
    return(BUTOUT_ERR);
  else
    return(0);
}


static ButOut  dispatchRew(But *but)  {
  Goban  *g = but_packet(but);
  GobanOut  out;

  assert(MAGIC(g));
  out = g->actions->rewPressed(g->packet);
  if (out == gobanOut_draw)
    goban_update(g);
  if (out == gobanOut_err)
    return(BUTOUT_ERR);
  else
    return(0);
}


static ButOut  dispatchBack(But *but)  {
  Goban  *g = but_packet(but);
  GobanOut  out;

  assert(MAGIC(g));
  out = g->actions->backPressed(g->packet);
  if (out == gobanOut_draw)
    goban_update(g);
  if (out == gobanOut_err)
    return(BUTOUT_ERR);
  else
    return(0);
}


static ButOut  dispatchFwd(But *but)  {
  Goban  *g = but_packet(but);
  GobanOut  out;

  assert(MAGIC(g));
  out = g->actions->fwdPressed(g->packet);
  if (out == gobanOut_draw)
    goban_update(g);
  if (out == gobanOut_err)
    return(BUTOUT_ERR);
  else
    return(0);
}


static ButOut  dispatchFf(But *but)  {
  Goban  *g = but_packet(but);
  GobanOut  out;

  assert(MAGIC(g));
  out = g->actions->ffPressed(g->packet);
  if (out == gobanOut_draw)
    goban_update(g);
  if (out == gobanOut_err)
    return(BUTOUT_ERR);
  else
    return(0);
}


static ButOut  dispatchDone(But *but)  {
  Goban  *g = but_packet(but);
  GobanOut  out;

  assert(MAGIC(g));
  out = g->actions->donePressed(g->packet);
  if (out == gobanOut_draw)
    goban_update(g);
  if (out == gobanOut_err)
    return(BUTOUT_ERR);
  else
    return(0);
}


static ButOut  dispatchDispute(But *but)  {
  Goban  *g = but_packet(but);
  GobanOut  out;

  assert(MAGIC(g));
  out = g->actions->disputePressed(g->packet);
  if (out == gobanOut_draw)
    goban_update(g);
  if (out == gobanOut_err)
    return(BUTOUT_ERR);
  else
    return(0);
}


static ButOut  dispatchSave(But *but)  {
  Goban  *g = but_packet(but);
  GobanOut  out;

  assert(MAGIC(g));
  out = g->actions->savePressed(g->packet);
  if (out == gobanOut_draw)
    goban_update(g);
  if (out == gobanOut_err)
    return(BUTOUT_ERR);
  else
    return(0);
}


static ButOut  dispatchEdit(But *but)  {
  Goban  *g = but_packet(but);
  GobanOut  out;

  assert(MAGIC(g));
  out = g->actions->editPressed(g->packet);
  if (out == gobanOut_draw)
    goban_update(g);
  if (out == gobanOut_err)
    return(BUTOUT_ERR);
  else
    return(0);
}


static ButOut  dispatchInfo(But *but)  {
  Goban  *g = but_packet(but);
  GobanOut  out;

  assert(MAGIC(g));
  out = g->actions->gameInfoPressed(g->packet);
  if (out == gobanOut_draw)
    goban_update(g);
  if (out == gobanOut_err)
    return(BUTOUT_ERR);
  else
    return(0);
}


void  goban_setKibitz(Goban *g, GobanOut (*newKib)(void *packet,
						   const char *txt),
		      int kibVal)  {
  uint  kibFlags;

  assert(MAGIC(g));
  if (kibVal == -1)  {
    kibFlags = BUT_DRAWABLE;
    kibVal = 2;
  } else
    kibFlags = BUT_PRESSABLE|BUT_DRAWABLE;
  butTbin_setMaxLines(g->comments->tbin, 100);
  butTbin_setReadOnly(g->comments->tbin, TRUE);
  butPlain_setColor(g->comments->bg, BUT_BG);
  g->kibIn = butTextin_create(gotKibitz, g, g->win, 1,
			      BUT_PRESSABLE|BUT_DRAWABLE, "", 200);
  g->newKib = newKib;
  g->kibType = butRadio_create(newKibVal, g, g->win, 1,
			       kibFlags, kibVal, 3);
  g->kibSay = butText_create(g->win, 2, BUT_DRAWABLE|BUT_PRESSTHRU,
			     msg_say, butText_center);
  g->kibBoth = butText_create(g->win, 2, BUT_DRAWABLE|BUT_PRESSTHRU,
			      msg_both, butText_center);
  g->kibKib = butText_create(g->win, 2, BUT_DRAWABLE|BUT_PRESSTHRU,
			     msg_kib, butText_center);
  if (!(kibFlags & BUT_PRESSABLE))  {
    butText_setColor(g->kibSay, BUT_FG, TRUE);
    butText_setColor(g->kibBoth, BUT_FG, TRUE);
  }
}


static ButOut  gotKibitz(But *but, const char *value)  {
  Goban  *g = but_packet(but);
  
  assert(MAGIC(g));
  if (value[0] == '\0')  {
    but_setFlags(g->kibIn, BUT_NOKEY);
  } else  {
    g->newKib(g->packet, value);
    butTextin_set(g->kibIn, "", FALSE);
  }
  return(0);
}


static ButOut  newKibVal(But *but, int value)  {
  Goban  *g = but_packet(but);

  assert(MAGIC(g));
  clp_setInt(g->cg->clp, "client.saykib", value);
  return(0);
}


void  goban_startTimer(Goban *g, GoStone whose)  {
  goTime_startTimer(&g->game->time, &g->timers[whose]);
  g->activeTimer = whose;
  startMoveTimer(g);
}


/* stopTimer returns TRUE if there is still time left. */
bool  goban_stopTimer(Goban *g)  {
  GoStone  color = g->activeTimer;

  if (color == goStone_empty)  {
    /* Timers were never started! */
    return(TRUE);
  }
  assert(goStone_isStone(color));
  stopMoveTimer(g);
  g->activeTimer = goStone_empty;
  return(goTime_endTimer(&g->game->time, &g->timers[color]));
}
