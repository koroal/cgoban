/*
 * src/gmp/setup.c, part of Complete Goban (game program)
 * Copyright (C) 1995 William Shubert
 * See "configure.h.in" for more copyright information.
 */


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
#include "client.h"
#include "../gameSetup.h"
#include "../msg.h"
#include "play.h"
#include "../client/setup.h"


/**********************************************************************
 * Forward declarations
 **********************************************************************/
static void  newGame(void *packet, GoStone localColor, int size, int handicap,
		     float komi, int mainTime, int byTime, void *board);
static void  gotMoveFromServer(void *packet, GoStone color, int loc);
static void  endGame(void *packet);
static void  logout(void *packet);
static void  gotUndoFromServer(void *packet);
static ButOut  gotMoveFromProg(GmpEngine *ge, void *packet, GoStone color,
			       int x, int y);
static ButOut  gotErrFromProg(GmpEngine *ge, void *packet, const char *errStr);
static ButOut  undoMoves(GmpEngine *ge, void *packet, int numUndos);


/**********************************************************************
 * Functions
 **********************************************************************/
GmpClient  *gmpClient_create(Cgoban *cg, const char *progName,
			     CliServer server, const char *user,
			     const char *password)  {
  GmpClient  *cli;
  static const CliActions  actions = {
    newGame, gotMoveFromServer, gotUndoFromServer, endGame, logout};

  assert(MAGIC(cg));
  cli = wms_malloc(sizeof(GmpClient));
  MAGIC_SET(cli);
  cli->cg = cg;
  cli->inGame = FALSE;
  str_initChars(&cli->progName, progName);
  cliSetup_createGmp(cg, server, user, password, &actions, cli);
  return(cli);
}


static void newGame(void *packet, GoStone localColor, int size, int handicap,
		    float komi, int mainTime, int byTime, void *board)  {
  static const GmpActions  actions =
    {NULL, gotMoveFromProg, undoMoves, gotErrFromProg};
  GmpClient  *cli = packet;
  Str  error;

  assert(MAGIC(cli));
  if (cli->inGame)
    endGame(cli);
  cli->size = size;
  cli->cliBoard = board;
  cli->pid = gmp_forkProgram(cli->cg, &cli->inFile, &cli->outFile,
			     str_chars(&cli->progName),
			     mainTime, byTime);
  if (cli->pid < 0)  {
    str_init(&error);
    str_print(&error, msg_progRunErr,
	      strerror(errno), str_chars(&cli->progName));
    cgoban_createMsgWindow(cli->cg, "Cgoban Error", str_chars(&error));
    str_deinit(&error);
    close(cli->inFile);
    close(cli->outFile);
  } else  {
    gmpEngine_init(cli->cg, &cli->ge, cli->inFile, cli->outFile,
		   &actions, cli);
    cli->inGame = TRUE;
    gmpEngine_startGame(&cli->ge, size, handicap, komi, FALSE,
			(localColor != goStone_white));
  }
}


static void  gotMoveFromServer(void *packet, GoStone color, int loc)  {
  GmpClient  *gc = packet;

  assert(MAGIC(gc));
  if (gc->inGame)  {
    if (loc == 0)
      gmpEngine_sendPass(&gc->ge);
    else  {
      gmpEngine_sendMove(&gc->ge,
			 goBoard_loc2X(gc->cliBoard->game->board, loc),
			 goBoard_loc2Y(gc->cliBoard->game->board, loc));
    }
  }
}


static void  endGame(void *packet)  {
  GmpClient  *gc = packet;
  int  dummy;

  assert(MAGIC(gc));
  if (gc->inGame)  {
    gc->inGame = FALSE;
    kill(gc->pid, SIGTERM);
    waitpid(gc->pid, &dummy, 0);
    close(gc->inFile);
    close(gc->outFile);
    gmpEngine_deinit(&gc->ge);
  }
}


static void  logout(void *packet)  {
  GmpClient  *gc = packet;

  assert(MAGIC(gc));
  gmpClient_destroy(gc);
}


void  gmpClient_destroy(GmpClient *gc)  {
  assert(MAGIC(gc));
  if (gc->inGame)
    endGame(gc);
  str_deinit(&gc->progName);
  MAGIC_UNSET(gc);
  wms_free(gc);
}


static ButOut  gotMoveFromProg(GmpEngine *ge, void *packet, GoStone color,
			       int x, int y)  {
  GmpClient  *gc = packet;

  assert(MAGIC(gc));
  if (x >= 0)  {
    cliBoard_gridPressed(gc->cliBoard,
			 goBoard_xy2Loc(gc->cliBoard->game->board, x, y),
			 TRUE);
  } else
    cliBoard_passPressed(gc->cliBoard);
  return(0);
}


static ButOut  gotErrFromProg(GmpEngine *ge,
			      void *packet, const char *errStr)  {
  GmpClient  *gc = packet;
  int  exitStatus = 0;
  bool  alreadyDead = FALSE;
  Str  exitMsg;

  assert(MAGIC(gc));
  alreadyDead = (waitpid(gc->pid, &exitStatus, WNOHANG) != 0);
  if (!alreadyDead)  {
    kill(gc->pid, SIGKILL);
    waitpid(gc->pid, &exitStatus, 0);
  }
  close(gc->inFile);
  close(gc->outFile);
  gmpEngine_deinit(&gc->ge);
  gc->inGame = FALSE;
  if (alreadyDead)  {
    str_init(&exitMsg);
    if (WIFEXITED(exitStatus) &&
	(WEXITSTATUS(exitStatus) == GMP_EXECVEFAILED))  {
      str_print(&exitMsg, msg_gmpCouldntStart,
		str_chars(&gc->progName));
    } else if (WIFSIGNALED(exitStatus))  {
      str_print(&exitMsg, msg_gmpProgKilled,
		str_chars(&gc->progName),
		WTERMSIG(exitStatus));
    } else  {
      str_print(&exitMsg, msg_gmpProgWhyDead,
		str_chars(&gc->progName));
    }
    goban_message(gc->cliBoard->goban, str_chars(&exitMsg));
    str_deinit(&exitMsg);
  } else
    goban_message(gc->cliBoard->goban, errStr);
  return(BUTOUT_ERR);
}


static void  gotUndoFromServer(void *packet)  {
  GmpClient  *gc = packet;

  assert(MAGIC(gc));
  gmpEngine_sendUndo(&gc->ge, 1);
}


static ButOut  undoMoves(GmpEngine *ge, void *packet, int numUndos) {
  GmpClient  *gc = packet;

  assert(MAGIC(gc));
  goban_message(gc->cliBoard->goban, msg_noUndoFromGmpToClient);
  return(BUTOUT_ERR);
}
