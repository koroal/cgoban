/*
 * src/gopic.c, part of Complete Goban (game program)
 * Copyright (C) 1995 William Shubert
 * See "configure.h.in" for more copyright information.
 */


#include <wms.h>
#include <wms/rnd.h>
#include <but/but.h>
#include <but/plain.h>
#include <but/ctext.h>
#include <but/text.h>
#include <abut/msg.h>
#include "cgoban.h"
#include "goPic.h"
#include "goGame.h"
#include "cgbuts.h"
#include "plasma.h"
#include "cm2pm.h"


static ButOut  gridPressed(But *but);


GoPic  *goPic_create(Cgoban *cg, void *packet, GoGame *game,
		     ButWin *win, int layer,
		     ButOut pressed(void *packet, int loc), int size)  {
  GoPic  *p;
  int  i, x, y, loc;
  char  label[3];

  p = wms_malloc(sizeof(GoPic));
  MAGIC_SET(p);
  p->cg = cg;
  p->game = game;
  p->press = pressed;
  p->packet = packet;
  p->size = size;

  /*
   * if (cg->cgbuts.color == GrayScale && 0)  {
   *   p->boardBg = butBoxFilled_create(win, layer, BUT_DRAWABLE);
   *   butBoxFilled_setColors(p->boardBg, BUT_LIT, BUT_SHAD, BUT_LIT);
   * }
   */
  p->boardBg = butPixmap_create(win, layer, BUT_DRAWABLE, None);
  butPixmap_setPic(p->boardBg, p->cg->boardPixmap, 0,0);
  p->size = size;
  p->allowedMoves = goPicMove_legal;
  p->boardButs = wms_malloc(goBoard_area(game->board) * sizeof(But *));
  for (i = 0;  i < goBoard_area(game->board);  ++i)
    p->boardButs[i] = NULL;
  for (x = 0;  x < size;  ++x)  {
    for (y = 0;  y < size;  ++y)  {
      loc = goBoard_xy2Loc(game->board, x, y);
      p->boardButs[loc] = grid_create(&cg->cgbuts, gridPressed, p, win,
				      layer+1, BUT_DRAWABLE, loc);
    }
  }
  grid_setLineGroup(p->boardButs[goBoard_xy2Loc(game->board, 0, 0)],
		    gridLines_ul);
  grid_setLineGroup(p->boardButs[goBoard_xy2Loc(game->board, size-1, 0)],
		    gridLines_ur);
  grid_setLineGroup(p->boardButs[goBoard_xy2Loc(game->board, 0, size-1)],
		    gridLines_ll);
  grid_setLineGroup(p->boardButs[goBoard_xy2Loc(game->board, size-1, size-1)],
		    gridLines_lr);
  for (x = 1;  x < size-1;  ++x)  {
    grid_setLineGroup(p->boardButs[goBoard_xy2Loc(game->board, x,0)],
		      gridLines_top);
    grid_setLineGroup(p->boardButs[goBoard_xy2Loc(game->board, 0,x)],
		      gridLines_left);
    grid_setLineGroup(p->boardButs[goBoard_xy2Loc(game->board, size-1,x)],
		      gridLines_right);
    grid_setLineGroup(p->boardButs[goBoard_xy2Loc(game->board, x,size-1)],
		      gridLines_bottom);
  }
  if (size == 19)  {
    for (x = 3;  x < 19;  x += 6)  {
      for (y = 3;  y < 19;  y += 6)  {
	grid_setStarPoint(p->boardButs[goBoard_xy2Loc(game->board, x,y)],
			  TRUE);
      }
    }
  } else if (size == 13)  {
    for (x = 3;  x < 12;  x += 3)  {
      for (y = 3;  y < 12;  y += 3)  {
	grid_setStarPoint(p->boardButs[goBoard_xy2Loc(game->board, x,y)],
			  TRUE);
      }
    }
  } else if (size == 9)  {
    grid_setStarPoint(p->boardButs[goBoard_xy2Loc(game->board, 4,4)], TRUE);
    grid_setStarPoint(p->boardButs[goBoard_xy2Loc(game->board, 2,2)], TRUE);
    grid_setStarPoint(p->boardButs[goBoard_xy2Loc(game->board, 2,6)], TRUE);
    grid_setStarPoint(p->boardButs[goBoard_xy2Loc(game->board, 6,2)], TRUE);
    grid_setStarPoint(p->boardButs[goBoard_xy2Loc(game->board, 6,6)], TRUE);
  }

  assert(size < 50);
  p->letters = wms_malloc(size * 2 * sizeof(But *));
  p->numbers = wms_malloc(size * 2 * sizeof(But *));
  for (i = 0;  i < size;  ++i)  {
    label[0] = 'A' + (i % 25);
    label[1] = '\0';
    if (label[0] >= 'I')
      ++label[0];
    if (i > 24)  {
      label[1] = label[0];
      label[2] = '\0';
    }
    p->letters[i] = butText_create(win, layer+1, 0, label, butText_center);
    butText_setFont(p->letters[i], 1);
    p->letters[i+size] = butText_create(win, layer+1, 0, label,
					butText_center);
    butText_setFont(p->letters[i+size], 1);
    sprintf(label, "%d", size-i);
    p->numbers[i] = butText_create(win, layer+1, 0, label, butText_center);
    butText_setFont(p->numbers[i], 1);
    p->numbers[i+size] = butText_create(win, layer+1, 0, label,
					butText_center);
    butText_setFont(p->numbers[i+size], 1);
  }
  return(p);
}


void  goPic_destroy(GoPic *p)  {
  assert(MAGIC(p));
  /*
   * I don't destroy the buttons.  Kill the window to do that.
   */
  wms_free(p->boardButs);
  wms_free(p->letters);
  wms_free(p->numbers);
  MAGIC_UNSET(p);
  wms_free(p);
}


void  goPic_resize(GoPic *p, int x, int y, int w, int h)  {
  int  i, bx, by;
  int  edgeLabelSize, stoneW, stoneH, stoneX0, stoneY0;
  int  loc;

  assert(w == h);
  if (p->cg->showCoords)  {
    edgeLabelSize = butEnv_fontH(p->cg->env, 1);
    i = butEnv_textWidth(p->cg->env, "00", 1);
    if (i > edgeLabelSize)
      edgeLabelSize = i;
  } else
    edgeLabelSize = 0;
  stoneW = (w - edgeLabelSize * 2) / p->size;
  if (stoneW < edgeLabelSize) {
    edgeLabelSize = 0;
    stoneW = (w - edgeLabelSize * 2) / p->size;
  }
  stoneH = (h - edgeLabelSize * 2) / p->size;
  stoneX0 = x + (w - stoneW * p->size) / 2;
  stoneY0 = y + (h - stoneH * p->size) / 2;
  but_resize(p->boardBg, x,y, w,h);

  for (bx = 0;  bx < p->size;  ++bx)  {
    for (by = 0;  by < p->size;  ++by)  {
      loc = goBoard_xy2Loc(p->game->board, bx, by);
      but_resize(p->boardButs[loc], bx*stoneW+stoneX0, by*stoneH+stoneY0,
		 stoneW,stoneH);
    }
  }

  but_draw(p->boardBg);
  if (p->cg->showCoords)  {
    for (i = 0;  i < p->size*2;  ++i)  {
      but_setFlags(p->letters[i], BUT_DRAWABLE);
      but_setFlags(p->numbers[i], BUT_DRAWABLE);
    }
  } else  {
    for (i = 0;  i < p->size*2;  ++i)  {
      but_setFlags(p->letters[i], BUT_NODRAW);
      but_setFlags(p->numbers[i], BUT_NODRAW);
    }
  }
  for (i = 0;  i < p->size;  ++i)  {
    but_resize(p->letters[i], i*stoneW + stoneX0, stoneY0 - edgeLabelSize,
	       stoneW, edgeLabelSize);
    but_resize(p->letters[i+p->size], i*stoneW + stoneX0,
	       stoneY0 + p->size*stoneW, stoneW, edgeLabelSize);
    but_resize(p->numbers[i], stoneX0 - edgeLabelSize, stoneY0 + i*stoneW,
	       edgeLabelSize, stoneW);
    but_resize(p->numbers[i+p->size], stoneX0 + p->size*stoneW,
	       stoneY0 + i*stoneW, edgeLabelSize, stoneW);
  }
}


void  goPic_setButHold(GoPic *p, bool hold)  {
  int  i;

  for (i = 0;  i < goBoard_area(p->game->board);  ++i)  {
    if (p->boardButs[i])  {
      grid_setHold(p->boardButs[i], hold);
    }
  }
}


GoStone  goPic_update(GoPic *p)  {
  int  i;
  GoStone  s, toMove;
  GoGame  *game = p->game;
  GoBoard  *board = game->board;
  bool  setMove;
  GoMarkType  mark;
  bool  grey;
  int  markAux;
  int  whiteVersion;
  uint  moveType, allowed;

  assert(MAGIC(p));
  whiteVersion = rnd_int(p->cg->rnd) % CGBUTS_NUMWHITE;
  setMove = ((game->state == goGameState_play) ||
	     (game->state == goGameState_dispute));
  allowed = p->allowedMoves;
  if (allowed & goPicMove_forceWhite)
    toMove = goStone_white;
  else if (allowed & goPicMove_forceBlack)
    toMove = goStone_black;
  else if (setMove)
    toMove = goGame_whoseMove(game);
  else
    toMove = goStone_empty;
  if ((toMove == goStone_white) && (allowed & goPicMove_noWhite))
    allowed = 0;
  else if ((toMove == goStone_black) && (allowed & goPicMove_noBlack))
    allowed = 0;
  for (i = 0;  i < goBoard_area(board);  ++i)  {
    if (p->boardButs[i])  {
      s = goBoard_stone(board, i);
      mark = goMark_none;
      grey = FALSE;
      markAux = 0;
      switch(game->state)  {
      case goGameState_dispute:
	if (game->flags[i] & GOGAMEFLAGS_DISPUTED)
	  mark = goMark_triangle;
	/* Fall through to goGameState_play */
      case goGameState_play:
	if (goGame_lastMove(game) == i)
	  mark = goMark_lastMove;
	if (goGame_isHot(game, i))
	  mark = goMark_ko;
	break;
      case goGameState_selectDead:
      case goGameState_selectDisputed:
      case goGameState_done:
	if ((game->flags[i] & GOGAMEFLAGS_WANTDEAD) &&
	    (game->state != goGameState_done))
	  mark = goMark_triangle;
	if (game->flags[i] & GOGAMEFLAGS_MARKDEAD)
	  grey = TRUE;
	if ((game->flags[i] & GOGAMEFLAGS_MARKDEAD) ||
	    (s == goStone_empty))  {
	  if (((game->flags[i] & GOGAMEFLAGS_SEEN) == GOGAMEFLAGS_SEEBLACK) &&
	      ((game->rules != goRules_japanese) ||
	       (game->flags[i] & GOGAMEFLAGS_NOSEKI)))
	    mark = goMark_smBlack;
	  if (((game->flags[i] & GOGAMEFLAGS_SEEN) == GOGAMEFLAGS_SEEWHITE) &&
	      ((game->rules != goRules_japanese) ||
	       (game->flags[i] & GOGAMEFLAGS_NOSEKI)))
	    mark = goMark_smWhite;
	}
	break;
      default:
	break;
      }
      /*
       * If it's a passive game, then we can't set the marks of the grid
       *   ourselves.  the sgfPlay module must do it instead.
       */
      if (game->passive && (game->state != goGameState_selectDead))  {
	/*
	 * If there's no other mark there and it's a mark for the last
	 *   move or a ko, then we draw the mark for the last move.
	 *   Otherwise sgfPlay's mark takes precedence.
	 */
	if (((mark != goMark_lastMove) && (mark != goMark_ko)) ||
	    (grid_markType(p->boardButs[i]) != goMark_none))  {
	  grey = grid_grey(p->boardButs[i]);
	  mark = grid_markType(p->boardButs[i]);
	  markAux = grid_markAux(p->boardButs[i]);
	}
      }
      if (goStone_isStone(goBoard_stone(game->board, i)))
	moveType = goPicMove_stone;
      else
	moveType = goPicMove_empty;
      if ((s != grid_stone(p->boardButs[i])) ||
	  (grey != grid_grey(p->boardButs[i])))  {
	if (s == goStone_white)
	  grid_setVersion(p->boardButs[i],
			  (rnd_int(p->cg->rnd) % CGBUTS_NUMWHITE));
	grid_setStone(p->boardButs[i], goBoard_stone(board, i), grey);
      }
      if ((mark != grid_markType(p->boardButs[i])) ||
	  (markAux != grid_markAux(p->boardButs[i])))
	grid_setMark(p->boardButs[i], mark, markAux);
      if (allowed & goPicMove_legal)  {
	if (goGame_isLegal(game, toMove, i))
	  moveType |= goPicMove_legal;
      }
      if (moveType & allowed)  {
	if (setMove)  {
	  if ((toMove == goStone_white) &&
	      (grid_stone(p->boardButs[i]) == goStone_empty))
	    grid_setVersion(p->boardButs[i], whiteVersion);
	  grid_setPress(p->boardButs[i], toMove);
	} else
	  but_setFlags(p->boardButs[i], BUT_PRESSABLE);
      } else  {
	but_setFlags(p->boardButs[i], BUT_NOPRESS);
      }
    }
  }
  return(toMove);
}


static ButOut  gridPressed(But *but)  {
  GoPic  *p;

  p = but_packet(but);
  assert(MAGIC(p));
  return(p->press(p->packet, grid_pos(but)));
}
