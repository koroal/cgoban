/*
 * $Source: /cvsroot/cgoban1/cgoban1/src/main.c,v $
 * $Revision: 1.2 $
 * $Date: 2000/02/26 23:03:46 $
 *
 * src/main.c, part of Complete Goban (game program)
 * Copyright © 1995-2000 William Shubert
 * See "configure.h.in" for more copyright information.
 */

#include <wms.h>
#include <wms/clp.h>
#include <wms/rnd.h>
#include <but/but.h>
#include <abut/abut.h>
#include <abut/msg.h>
#include <wms/str.h>
#include "cgoban.h"
#include "control.h"
#include "main.h"
#include "editBoard.h"
#include "crwin.h"
#include "client/setup.h"
#include "arena.h"


/**********************************************************************
 * Forward Declarations
 **********************************************************************/
static void  stealth(int exitVal);
#if  !DEBUG
static RETSIGTYPE  sigHandler();
#endif  /* !DEBUG */


int  main(int argc, char *argv[], char *envp[])  {
  Cgoban  *cg;
  int  retVal = 0;
  Control  *ctrl;
  EditBoard  *e;
  CliSetup  *client;

  cg = cgoban_create(argc, argv, envp);
  if (cg == NULL)  {
    fprintf(stderr, "cgoban: Couldn't open X display.\n");
    return(1);
  }
  if (clp_getInt(cg->clp, "arena.games") > 0)  {
    arena(cg,
	  clp_getStr(cg->clp, "arena.prog1"),
	  clp_getStr(cg->clp, "arena.prog2"),
	  clp_getInt(cg->clp, "arena.size"),
	  (float)clp_getDouble(cg->clp, "arena.komi"),
	  clp_getInt(cg->clp, "arena.handicap"));
    exit(0);
  }

#if  !DEBUG
  signal(SIGTERM, sigHandler);  /* Catch these signals gracefully. */
  signal(SIGINT, sigHandler);
  signal(SIGHUP, sigHandler);
#endif
  /*
   * We have to always catch SIGPIPE.  Otherwise the GMP will freak out
   *   on us.
   */
  signal(SIGPIPE, SIG_IGN);  /* Seems like you can't catch a SIGPIPE. */
#ifdef  DEBUG
#define  STEALTH_VALID  (!(DEBUG))
#else
#define  STEALTH_VALID  1
#endif
  if (clp_getStr(cg->clp, "edit")[0])  {
    if ((e = editBoard_create(cg, clp_getStr(cg->clp, "edit"))) == NULL)  {
      cg->env->minWindows = 0;
      retVal = 1;
    } else  {
      crwin_create(cg, e->goban->win, 4);
      control_create(cg, TRUE);
    }
  } else if (clp_getBool(cg->clp, "nngs"))  {
    if ((client = cliSetup_create(cg, cliServer_nngs)) == NULL)  {
      cg->env->minWindows = 0;
      retVal = 1;
    } else  {
      crwin_create(cg, client->login->win, 3);
      control_create(cg, TRUE);
    }
  } else if (clp_getBool(cg->clp, "igs"))  {
    if ((client = cliSetup_create(cg, cliServer_igs)) == NULL)  {
      cg->env->minWindows = 0;
      retVal = 1;
    } else  {
      crwin_create(cg, client->login->win, 3);
      control_create(cg, TRUE);
    }
  } else  {
    ctrl = control_create(cg, clp_getBool(cg->clp, "iconic"));
    crwin_create(cg, ctrl->win, 3);
  }
  if (clp_getBool(cg->clp, "stealth") && STEALTH_VALID)
    stealth(0);
  butEnv_events(cg->env);
  cgoban_destroy(cg);

  return(retVal);
}


static void  stealth(int exitVal)  {
  int pid;
#ifdef  TIOCNOTTY
  int  tty;
#endif
  
  pid = fork();
  if (pid < 0)  {
    perror("cgoban: fork() failed");
    return;
  } else if (pid > 0)
    /* Parent just exits. */
    exit(0);
  /* Go stealth (ditch our controlling tty). */
#ifdef  TIOCNOTTY
  tty = open("/dev/tty", 0);
  if (tty < 0)
    return;
  ioctl(tty, TIOCNOTTY, 0);
  close(tty);
#endif
}


#if  !DEBUG
static RETSIGTYPE  sigHandler()  {
  fprintf(stderr, "Signal caught.\n");
  abort();
}
#endif  /* !DEBUG */
