/*
 * src/client/look.c, part of Complete Goban (game program)
 * Copyright (C) 1996 William Shubert
 * See "configure.h.in" for more copyright information.
 */

#include <wms.h>
#include <but/but.h>
#include <but/text.h>
#include <but/ctext.h>
#ifndef  _MSG_H_
#include "../msg.h"
#endif
#ifdef  _CLIENT_LOOK_H_
     LEVELIZATION ERROR
#endif
#include "look.h"


/**********************************************************************
 * Forward Declarations
 **********************************************************************/
static void  gameReady(CliLook *look, int size);
static GobanOut  quit(void *packet);
static void  gobanDead(void *packet);
static bool  moveOk(void *packet);
static void  makeMoves(GoGame *game, const char *board, int size);


/**********************************************************************
 * Functions
 **********************************************************************/
void  cliLook_init(CliLook *look, Cgoban *cg, CliServer server)  {
  GoStone  s;

  MAGIC_SET(look);
  look->cg = cg;
  look->server = server;
  look->state = cliLook_ready;
  goStoneIter(s)  {
    str_init(&look->name[s]);
    str_init(&look->rank[s]);
  }
  str_init(&look->boardData);
}


void  cliLook_deinit(CliLook *look)  {
  GoStone  s;

  assert(MAGIC(look));
  goStoneIter(s)  {
    str_deinit(&look->name[s]);
    str_deinit(&look->rank[s]);
  }
  str_deinit(&look->boardData);
  MAGIC_UNSET(look);
}


void  cliLook_gotData(CliLook *look, const char *buf)  {
  GoStone  s;
  int  lineNum, matches;
  char  data[GOBOARD_MAXSIZE + 1];
  char  name[40], rank[10];

  assert(MAGIC(look));
  switch(look->state)  {
  case cliLook_ready:
    look->skip = FALSE;
    str_clip(&look->boardData, 0);
    /* Fall through to cliLook_info1. */
  case cliLook_info:
    s = (look->state - cliLook_ready) + goStone_white;
    matches = sscanf(buf, "%39s %9s %d %*d %*d %*s %g %d",
		     name, rank, &look->captures[s],
		     &look->komi, &look->handicap);
    assert(matches == 5);
    str_copyChars(&look->name[s], name);
    str_copyChars(&look->rank[s], rank);
    ++look->state;
    break;
  case cliLook_body:
    matches = sscanf(buf, "%d: %s", &lineNum, data);
    assert(matches == 2);
    assert(strlen(data) <= GOBOARD_MAXSIZE);
    str_catChars(&look->boardData, data);
    if (lineNum == strlen(data) - 1)  {
      gameReady(look, lineNum + 1);
      look->state = cliLook_ready;
    }
    break;
  }
}


static void  gameReady(CliLook *look, int size)  {
  static const GobanActions  actions = {
    NULL, quit, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL,
    NULL, NULL, /* edit, gameInfo */
    &help_cliLook, gobanDead,
    moveOk, moveOk, moveOk, moveOk};
  Goban  *goban;
  GoGame  *game;
  Str  title;
  static const GoTime  noTime = {goTime_none, 0, 0, 0};
  CliLookChild  *child;
  int  i;

  /*
   * If it's a game-end board, then don't show it.
   * You can tell these because they have 3s, 4s, and 5s in them.
   */
  for (i = 0;  i < str_len(&look->boardData);  ++i)  {
    assert(str_chars(&look->boardData)[i] >= '0');
    assert(str_chars(&look->boardData)[i] <= '6');
    if ((str_chars(&look->boardData)[i] > '2') &&
	(str_chars(&look->boardData)[i] < '6'))  {
      return;
    }
  }
  child = wms_malloc(sizeof(CliLookChild));
  MAGIC_SET(child);
  str_init(&title);
  str_print(&title, msg_cliLookName,
	    str_chars(&look->name[goStone_white]),
	    str_chars(&look->rank[goStone_white]),
	    str_chars(&look->name[goStone_black]),
	    str_chars(&look->rank[goStone_black]));
  /*
   * I pass in a handicap of 0 even though I know the real handicap.  Why?
   *   because otherwise when I parse over the board, if I find a white
   *   stone where there used to be a handicap stone, I'm screwed!  I'd have
   *   to erase the black stone somehow and add a white one.  Ugh.  Easier
   *   just to never add the handicap stones to begin with; they'll be caught
   *   when I scan the board later.
   */
  game = goGame_create(size, goRules_japanese, 0, look->komi,
		       &noTime, TRUE);
  makeMoves(game, str_chars(&look->boardData), size);
  goban = goban_create(look->cg, &actions, child, game, "client.look",
		       str_chars(&title));
  goban->pic->allowedMoves = goPicMove_noWhite | goPicMove_noBlack;
  goban->iDec1 = grid_create(&look->cg->cgbuts, NULL, NULL,
			     goban->iWin, 2, BUT_DRAWABLE, 0);
  grid_setStone(goban->iDec1, goStone_white, FALSE);
  if (look->server == cliServer_nngs)
    grid_setVersion(goban->iDec1, CGBUTS_WORLDWEST(0));
  else
    grid_setVersion(goban->iDec1, CGBUTS_WORLDEAST(0));
  goban_update(goban);
  butText_set(goban->infoText, "Static Game Board");
  child->game = game;
  child->goban = goban;
  str_deinit(&title);
}


static GobanOut  quit(void *packet)  {
  CliLookChild  *child = packet;

  assert(MAGIC(child));
  goban_destroy(child->goban, TRUE);
  return(gobanOut_noDraw);
}


static void  gobanDead(void *packet)  {
  CliLookChild  *child = packet;

  assert(MAGIC(child));
  goGame_destroy(child->game);
  MAGIC_UNSET(child);
  wms_free(child);
}
  

static bool  moveOk(void *packet)  {
  return(FALSE);
}


static void  makeMoves(GoGame *game, const char *board, int size)  {
  int  x, y;

  for (x = 0;  x < size;  ++x)  {
    for (y = 0;  y < size;  ++y)  {
      switch(*board)  {
      case '0':
	goGame_move(game, goStone_black,
		    goBoard_xy2Loc(game->board, x, y), NULL);
	break;
      case '1':
	goGame_move(game, goStone_white,
		    goBoard_xy2Loc(game->board, x, y), NULL);
	break;
      case '2':
      case '6':
	break;
      default:
	assert(0);
	break;
      }
      ++board;
    }
  }
  goGame_pass(game, goStone_black, NULL);  /* These passes just get rid of   */
  goGame_pass(game, goStone_white, NULL);  /*   the "last move here" marker. */
}
