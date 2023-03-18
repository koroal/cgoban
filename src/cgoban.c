/*
 * $Source: /cvsroot/cgoban1/cgoban1/src/cgoban.c,v $
 * $Revision: 1.4 $
 * $Date: 2002/12/17 22:46:56 $
 *
 * src/cgoban.c, part of Complete Goban (game program)
 * Copyright (C) 1995-1996 William Shubert
 * See "configure.h.in" for more copyright information.
 */

#include <wms.h>
#include <wms/clp.h>
#include <wms/rnd.h>
#include <but/but.h>
#include <abut/abut.h>
#include <abut/msg.h>
#include <wms/str.h>
#include "msg.h"
#include "cgbuts.h"
#include "goGame.h"
#include "goTime.h"
#include "plasma.h"
#include "cm2pm.h"
#include <abut/fsel.h>
#include <wms/clp-x.h>
#ifdef  _CGOBAN_H_
        LEVELIZATION ERROR
#endif
#include "cgoban.h"


/**********************************************************************
 * Forward Declarations
 **********************************************************************/
static bool  setupColors(Cgoban *cg, int colorMode);
static void  setupPixmaps(Cgoban *cg);
static bool  checkRules(ClpEntry *ce);
static bool  checkSize(ClpEntry *ce);
static bool  checkKomi(ClpEntry *ce);
static bool  checkHandicap(ClpEntry *ce);
static bool  checkTimeType(ClpEntry *ce);
static bool  checkTimeValue(ClpEntry *ce);
static bool  checkBYStones(ClpEntry *ce);
static bool  checkBProp(ClpEntry *ce);
static bool  checkGmpOption(ClpEntry *ce);
static bool  checkPortNum(ClpEntry *ce);
static ButOut  helpDead(AbutHelp *help, void *packet);


/**********************************************************************
 * Global variables
 **********************************************************************/
const ButKey  cg_return[] = {{XK_Return, 0, ShiftMask},
			     {XK_KP_Enter, 0, ShiftMask}, {0,0,0}};
const ButKey  cg_help[] = {{XK_Help, 0, 0}, {0,0,0}};


/**********************************************************************
 * Functions
 **********************************************************************/
Cgoban  *cgoban_create(int argc, char *argv[], char *envp[])  {
  Cgoban  *cg;
  static const ClpSetup  commandLine[] = {
    CLPSETUP_MSG("Complete Goban " VERSION " by William Shubert, Kevin Sonney - " DATE),
    CLPSETUP_MSG(""),
    {"VersionNumber", VERSION, NULL, 0, NULL},
    {"version,-version", "f", "Print version information",
       CLPSETUP_BOOL|CLPSETUP_NOSAVE, NULL},
    {"stealth", "t", "Stealth mode", CLPSETUP_SHOWBOOL|CLPSETUP_BOOL, NULL},
    {"display", NULL, "Display to use", CLPSETUP_NOSAVE, NULL},
    {"color", "t", "Use color if possible",
       CLPSETUP_SHOWBOOL|CLPSETUP_BOOL|CLPSETUP_NOSAVE, NULL},
    {"hiContrast", "f", "Use high-contrast graphics instead of rendered",
       CLPSETUP_SHOWBOOL|CLPSETUP_BOOL, NULL},
    {"adfile", "~/.cgobanrc", "Set resource file name", CLPSETUP_NOSAVE, NULL},
    {"name", "cgoban", "Name used to look up X resources", 0, NULL},
    {"fontHeight", "12", "Size (in pixels) of font (default = 12)", 0, NULL},

    CLPSETUP_MSG(""),
    CLPSETUP_MSG("See the README for info about arena mode"),
    {"arena.games", "0", "Number of games to play in arena test",
       CLPSETUP_NOSAVE, NULL},
    {"arena.prog1", "", "Program 1 for arena testing", 0, NULL},
    {"arena.prog2", "", "Program 2 for arena testing", 0, NULL},
    {"arena.size", "19", "Size to use in arena testing", 0, NULL},
    {"arena.komi", "5.5", "Komi for arena testing", 0, NULL},
    {"arena.handicap", "0", "Handicap for arena testing", 0, NULL},

    {"play.markLastMove", "t", NULL, CLPSETUP_SHOWBOOL|CLPSETUP_BOOL, NULL},
    {"play.markHotStones", "t", NULL, CLPSETUP_SHOWBOOL|CLPSETUP_BOOL, NULL},
    {"board.showCoords", "t", NULL, CLPSETUP_SHOWBOOL|CLPSETUP_BOOL, NULL},
    CLPSETUP_MSG(""),
    {"edit", "", "Edit SGF file", CLPSETUP_NOSAVE, NULL},
    {"nngs", "f", "Connect to NNGS", CLPSETUP_BOOL|CLPSETUP_NOSAVE, NULL},
    {"igs", "f", "Connect to IGS", CLPSETUP_BOOL|CLPSETUP_NOSAVE, NULL},
    {"iconic", "f", NULL, CLPSETUP_BOOL|CLPSETUP_NOSAVE, NULL},
    {"control.x", "", NULL, 0, NULL},
    {"control.y", "", NULL, 0, NULL},
    {"help.x", "", NULL, 0, NULL},
    {"help.y", "", NULL, 0, NULL},
    {"help.w", "28.284271", NULL, 0, NULL},
    {"help.h", "40.0", NULL, 0, NULL},
    {"game.rules", "1", NULL, 0, checkRules},
    {"game.size", "19", NULL, 0, checkSize},
    {"game.komi", "5.5", NULL, 0, checkKomi},
    {"game.handicap", "0", NULL, 0, checkHandicap},
    {"local.wName", "White", NULL, 0, NULL},
    {"local.bName", "Black", NULL, 0, NULL},
    {"local.x", "", NULL, 0, NULL},
    {"local.y", "", NULL, 0, NULL},
    {"local.bProp", "1.0", NULL, 0, checkBProp},
    {"local.bPropW", "1.0", NULL, 0, checkBProp},
    {"local.sgfName", "game.sgf", NULL, 0, NULL},
    {"edit.x", "", NULL, 0, NULL},
    {"edit.y", "", NULL, 0, NULL},
    {"edit.toolX", "", NULL, 0, NULL},
    {"edit.toolY", "", NULL, 0, NULL},
    {"edit.toolW", "0.0", NULL, 0, NULL},
    {"edit.toolH", "0.0", NULL, 0, NULL},
    {"edit.bProp", "1.0", NULL, 0, checkBProp},
    {"edit.bPropW", "1.0", NULL, 0, checkBProp},
    {"edit.sgfName", "seigen-minoru.sgf", NULL, 0, NULL},
    {"edit.infoX", "", NULL, 0, NULL},
    {"edit.infoY", "", NULL, 0, NULL},
    {"edit.infoW", "1.0", NULL, 0, NULL},
    {"edit.infoH", "1.0", NULL, 0, NULL},
    {"setup.timeType", "2", NULL, 0, checkTimeType},
    {"setup.mainTime", "30:00", NULL, 0, checkTimeValue},
    {"setup.igsBYTime", "10:00", NULL, 0, checkTimeValue},
    {"setup.igsBYStones", "25", NULL, 0, checkBYStones},
    {"setup.ingBYTime", "15:00", NULL, 0, checkTimeValue},
    {"setup.ingBYPeriods", "3", NULL, 0, checkBYStones},
    {"setup.jBYTime", "1:00", NULL, 0, checkTimeValue},
    {"setup.jBYPeriods", "5", NULL, 0, checkBYStones},
    {"config.x", "", NULL, 0, NULL},
    {"config.y", "", NULL, 0, NULL},
    {"config.h", "0.70710678", NULL, 0, NULL},
    {"gmp.WOption", "0", NULL, 0, checkGmpOption},
    {"gmp.BOption", "0", NULL, 0, checkGmpOption},
    {"gmp.WProgram", "/usr/bin/gnugo --mode gmp --quiet", NULL, 0, NULL},
    {"gmp.BProgram", "/usr/bin/gnugo --mode gmp --quiet", NULL, 0, NULL},
    {"gmp.WDevice", "/dev/cua0", NULL, 0, NULL},
    {"gmp.BDevice", "/dev/cua1", NULL, 0, NULL},
    {"gmp.machine", "localhost", NULL, 0, NULL},
    {"gmp.port", "26276", NULL, 0, checkPortNum},
    {"gmp.nngs.username", "", NULL, 0, NULL},
    {"gmp.nngs.password", "", NULL, 0, NULL},
    {"gmp.igs.username", "", NULL, 0, NULL},
    {"gmp.igs.password", "", NULL, 0, NULL},
    {"client.def1", "0", NULL, 0, NULL},
    {"client.def2", "1", NULL, 0, NULL},
    /*
     * If you change the number of entries in these lists you must also
     *   change the value of SETUP_MAXSERVERS in "src/setup.c".
     */
    {"client.server", "NNGS|IGS|LGS|NCIC|GIGS|||||", NULL, 0, NULL},
    {"client.protocol", "n|i|n|n|n|n|n|n|n|n", NULL, 0, NULL},
    {"client.direct", "t|t|t|t|t|t|t|t|t|t", NULL, 0, NULL},
    {"client.connCmd", "telnet nngs.cosmic.org 9696|"
       "telnet igs.joyjoy.net 6969|"
       "telnet|telnet|telnet|telnet|telnet|telnet|telnet|telnet",
       NULL, 0, NULL},
    {"client.username", "|||||||||", NULL, 0, NULL},
    {"client.password", "|||||||||", NULL, 0, NULL},
    {"client.address", "nngs.cosmic.org|igs.joyjoy.net|micro.ee.nthu.edu.tw|"
       "jet.ncic.ac.cn|grizu.uni-regensburg.de|||||",
       NULL, 0, NULL},
    {"client.port", "9696|6969|9696|9696|9696|9696|9696|9696|9696|9696",
       NULL, 0, NULL},
    {"client.match.meFirst", "t", NULL, 0, NULL},
    {"client.match.size", "19", NULL, 0, checkSize},
    {"client.match.free", "f", NULL, 0, NULL},
    {"client.match.mainTime", "20:00", NULL, 0, checkTimeValue},
    {"client.match.byTime", "10:00", NULL, 0, checkTimeValue},
    {"client.match.x", "", NULL, 0, NULL},
    {"client.match.y", "", NULL, 0, NULL},
    {"client.main.x", "", NULL, 0, NULL},
    {"client.main.y", "", NULL, 0, NULL},
    {"client.main.w", "40", NULL, 0, NULL},
    {"client.main.h", "40", NULL, 0, NULL},
    {"client.games.x", "", NULL, 0, NULL},
    {"client.games.y", "", NULL, 0, NULL},
    {"client.games.w", "6", NULL, 0, NULL},
    {"client.games.h2", "0.5", NULL, 0, NULL},
    {"client.players.x", "", NULL, 0, NULL},
    {"client.players.y", "", NULL, 0, NULL},
    {"client.players.h", "30.742", NULL, 0, NULL},
    {"client.players.sort", "0", NULL, 0, NULL},
    {"client.x", "", NULL, 0, NULL},
    {"client.y", "", NULL, 0, NULL},
    {"client.bProp", "1.0", NULL, 0, checkBProp},
    {"client.bPropW", "1.0", NULL, 0, checkBProp},
    {"client.numberKibitz", "f", NULL, CLPSETUP_BOOL, NULL},
    {"client.noTypo", "f", NULL, CLPSETUP_BOOL, NULL},
    {"client.look.x", "", NULL, 0, NULL},
    {"client.look.y", "", NULL, 0, NULL},
    {"client.look.bProp", "1.0", NULL, 0, checkBProp},
    {"client.look.bPropW", "1.0", NULL, 0, checkBProp},
    {"client.saykib", "1", NULL, 0, NULL},
    {"client.warnLimit", "30", NULL, 0, NULL},
    {"abut.fsel.x", "", NULL, 0, NULL},
    {"abut.fsel.y", "", NULL, 0, NULL},
    {"abut.fsel.w", "1.2", NULL, 0, NULL},
    {"abut.fsel.h", "1.2", NULL, 0, NULL},
    CLPSETUP_END};
  int  envInit, color;
  int  parses;
  int  boardBgSize;
  bool  colorWarning = FALSE;

  /* Create the cg */
  cg = wms_malloc(sizeof(Cgoban));
  MAGIC_SET(cg);
  cg->envp = envp;
  cg->clp = clp_create(commandLine);
  clp_rCmdline(cg->clp, argv);
  if (clp_getBool(cg->clp, "version"))  {
    printf("cgoban version " VERSION " by William Shubert, Kevin Sonney - " DATE "\n");
    exit(0);
  }
  if (clp_getInt(cg->clp, "arena.games") > 0)  {
    cg->env = butEnv_createNoDpy("Complete Goban " VERSION);
    return(cg);
  }

  cg->env = butEnv_create("Complete Goban " VERSION,
			  clp_getStr(cg->clp, "display"), NULL);
  if (cg->env == NULL)  {
    clp_destroy(cg->clp);
    wms_free(cg);
    MAGIC_UNSET(cg);
    return(NULL);
  }
  clp_rXDefaults(cg->clp, butEnv_dpy(cg->env), clp_getStr(cg->clp, "name"));
  clp_rFile(cg->clp, clp_getStr(cg->clp, "adfile"));

  parses = sscanf(clp_getStr(cg->clp, "VersionNumber"),
		  "Complete Goban %d.%d.%d", &cg->rcMajor, &cg->rcMinor,
		  &cg->rcBugFix);
  if (parses != 3)
    cg->rcMajor = cg->rcMinor = cg->rcBugFix = 0;
  clp_setStr(cg->clp, "VersionNumber", VERSION);
  color = TrueColor;
  envInit = butEnv_init(cg->env, cg, "Complete Goban",
			(clp_getBool(cg->clp, "color")));
  cg->env->minWindows = 2;
  if (envInit == 0)
    color = GrayScale;
  if (envInit == 2)
    color = PseudoColor;
  else if (envInit == 1)  {
    color = GrayScale;
    colorWarning = TRUE;
  }

  if (!setupColors(cg, color))  {
    colorWarning = TRUE;
    butEnv_destroy(cg->env);
    cg->env = butEnv_create("Complete Goban " VERSION,
			    clp_getStr(cg->clp, "display"), NULL);
    assert(cg->env != NULL);
    butEnv_init(cg->env, cg, "Complete Goban", FALSE);
    setupColors(cg, color = GrayScale);
  }

  str_initChars(&cg->lastDirAccessed, "./");
  cg->abut = abut_create(cg->env, msg_ok, msg_cancel);
  abut_setClp(cg->abut, cg->clp);
  abutFsel_pathMessage = msg_path;
  abutFsel_fileMessage = msg_file;
  abutFsel_maskMessage = msg_mask;
  abutFsel_dirsMessage = msg_dirs;
  abutFsel_filesMessage = msg_files;
  abutFsel_dirErrMessage = msg_dirErr;
  cg->help = NULL;
    
  cg->fontH = clp_getInt(cg->clp, "fontHeight");
  abut_setButH(cg->abut, cg->fontH * 2);
  cg->markLastMove = clp_getBool(cg->clp, "play.markLastMove");
  cg->markHotStones = clp_getBool(cg->clp, "play.markHotStones");
  cg->showCoords = clp_getBool(cg->clp, "board.showCoords");

  cg->rnd = rnd_create(time(NULL));

  /* Set up the X stuff */
  butEnv_setFont(cg->env, 0, msg_mFonts, cg->fontH);
  butEnv_setFont(cg->env, 1, msg_labelFonts,
		 (cg->fontH*5+3)/6);
  butEnv_setFont(cg->env, 2, msg_bFonts, cg->fontH);
  cgbuts_init(&cg->cgbuts, cg->env, cg->rnd, color,
	      clp_getBool(cg->clp, "client.noTypo"),
	      clp_getBool(cg->clp, "hiContrast"),
	      clp_getInt(cg->clp, "client.warnLimit"));
  if (cg->cgbuts.color == GrayScale)  {
    cg->boardPixmap = butEnv_pixmap(cg->env, BUT_LIT);
  } else  {
    boardBgSize = butEnv_fontH(cg->env, 0) * 28 + butEnv_stdBw(cg->env) * 19;
    cg->boardPixmap = cm2Pm(cg->env,
			    plasma(),
			    PLASMA_SIZE,PLASMA_SIZE,
			    boardBgSize, boardBgSize,
			    CGBUTS_COLORBOARD(0), 256,
			    clp_getBool(cg->clp, "hiContrast"));
  }

  setupPixmaps(cg);
  abut_setPixmaps(cg->abut, cg->bgLitPixmap, cg->bgShadPixmap, cg->bgPixmap);
  if (colorWarning)  {
    cgoban_createMsgWindow(cg, "Cgoban Warning",
			   msg_notEnoughColors);
  }
  return(cg);
}


void  cgoban_destroy(Cgoban *cg)  {
  assert(MAGIC(cg));
  clp_wFile(cg->clp, clp_getStr(cg->clp, "adfile"), "cgoban");
  cgbuts_redraw(&cg->cgbuts);
  butEnv_destroy(cg->env);
  abut_destroy(cg->abut);
  clp_destroy(cg->clp);
  rnd_destroy(cg->rnd);
  str_deinit(&cg->lastDirAccessed);
  MAGIC_UNSET(cg);
  wms_free(cg);
}


static bool  setupColors(Cgoban *cg, int color)  {
  ButColor  bd0, bd255, g0, g255;
  int  i, mix1, mix2;

  assert(MAGIC(cg));
  if (!butEnv_setColor(cg->env, CGBUTS_COLORBG1,
		       butColor_create(46,139,87, 15)))
    return(FALSE);
  if (!butEnv_setColor(cg->env, CGBUTS_COLORBG2,
		       butColor_create(55,167,104, 15)))
    return(FALSE);

  if (!butEnv_setColor(cg->env, CGBUTS_COLORBGLIT1,
		       butColor_create(61,185,116, 15)))
    return(FALSE);
  if (!butEnv_setColor(cg->env, CGBUTS_COLORBGLIT2,
		       butColor_create(74,222,139, 15)))
    return(FALSE);
  
  if (!butEnv_setColor(cg->env, CGBUTS_COLORBGSHAD1,
		       butColor_create(31,93,58, 8)))
    return(FALSE);
  if (!butEnv_setColor(cg->env, CGBUTS_COLORBGSHAD2,
		       butColor_create(37,111,70, 8)))
    return(FALSE);

  if (!butEnv_setColor(cg->env, CGBUTS_COLORREDLED,
		       butColor_create(255,80,80, 16)))
    return(FALSE);
  butEnv_setColor(cg->env, BUT_CHOICE, butColor_create(255,80,80, 8));

  g0 = butColor_create(0,0,0, 0);
  g255 = butColor_create(255,255,255, 16);
  for (i = 0;  i < 256;  ++i)  {
    mix1 = 255-i;
    mix2 = i;
    if (color != TrueColor)  {
      mix1 = (mix1 * 17) / 256;
      mix2 = 16 - mix1;
    }
    if (!butEnv_setColor(cg->env, CGBUTS_GREY(i),
			 butColor_mix(g0, mix1, g255, mix2)))
      return(FALSE);
  }

  bd0 = butColor_create(242,174,106, 8);
  bd255 = butColor_create(212,121,78, 8);
  bd255 = butColor_mix(bd0, 1, bd255, 4);
  for (i = 0;  i < 256;  ++i)  {
    mix2 = (i*i*i + 127) / 255;
    mix1 = 255*255 - mix2;
    if (color != TrueColor)  {
      mix1 = (mix1 * 8) / (255 * 255 + 1);
      mix2 = 7 - mix1;
    }
    if (!butEnv_setColor(cg->env, CGBUTS_COLORBOARD(i),
			 butColor_mix(bd0, mix1, bd255, mix2)))
      return(FALSE);
  }
  return(TRUE);
}


static void  setupPixmaps(Cgoban *cg)  {
  Display  *dpy;
  GC  gc;
  int  picH, i, j, offset, lastOffset = -1, lw, ll;

  assert(MAGIC(cg));
  lw = (cg->fontH + 5) / 10;
  if (lw < 1)
    lw = 1;
  ll = lw * 4;
  picH = (ll+lw) * 3;
  picH = (((cg->fontH * 4) / picH) + 1) * picH;
  dpy = butEnv_dpy(cg->env);
  gc = butEnv_gc(cg->env);
  if (cg->cgbuts.color == GrayScale)  {
    cg->bgPixmap = butEnv_pixmap(cg->env, BUT_BG);
    cg->bgLitPixmap = butEnv_pixmap(cg->env, BUT_LIT);
    cg->bgShadPixmap = butEnv_pixmap(cg->env, BUT_SHAD);
  } else  {
    cg->bgPixmap = XCreatePixmap(dpy, cg->cgbuts.dpyRootWindow,
				 picH, picH, cg->cgbuts.dpyDepth);
    cg->bgLitPixmap = XCreatePixmap(dpy, cg->cgbuts.dpyRootWindow,
				    picH, picH, cg->cgbuts.dpyDepth);
    cg->bgShadPixmap = XCreatePixmap(dpy, cg->cgbuts.dpyRootWindow,
				     picH, picH, cg->cgbuts.dpyDepth);
    XSetFillStyle(dpy, gc, FillSolid);
    XSetForeground(dpy, gc, butEnv_color(cg->env, CGBUTS_COLORBG1));
    XFillRectangle(dpy, cg->bgPixmap, gc, 0,0, picH, picH);
    XSetForeground(dpy, gc, butEnv_color(cg->env, CGBUTS_COLORBGLIT1));
    XFillRectangle(dpy, cg->bgLitPixmap, gc, 0,0, picH, picH);
    XSetForeground(dpy, gc, butEnv_color(cg->env, CGBUTS_COLORBGSHAD1));
    XFillRectangle(dpy, cg->bgShadPixmap, gc, 0,0, picH, picH);

    XSetLineAttributes(dpy, gc, lw, LineSolid, CapButt, JoinMiter);
    for (i = 0;  i < picH;  i += 3*lw)  {
      do  {
	offset = (rnd_int(cg->rnd) % (ll+lw)) + lw;
      } while (offset == lastOffset);
      lastOffset = offset;
      for (j = -offset;  j < picH;  j += (ll+lw))  {
	XSetForeground(dpy, gc, butEnv_color(cg->env, CGBUTS_COLORBG2));
	XDrawLine(dpy, cg->bgPixmap, gc, i-j,j, i-j-ll,j+ll);
	XDrawLine(dpy, cg->bgPixmap, gc, i-j+picH,j, i-j-ll+picH,j+ll);

	XSetForeground(dpy, gc, butEnv_color(cg->env, CGBUTS_COLORBGLIT2));
	XDrawLine(dpy, cg->bgLitPixmap, gc, i-j,j, i-j-ll,j+ll);
	XDrawLine(dpy, cg->bgLitPixmap, gc, i-j+picH,j, i-j-ll+picH,j+ll);

	XSetForeground(dpy, gc, butEnv_color(cg->env, CGBUTS_COLORBGSHAD2));
	XDrawLine(dpy, cg->bgShadPixmap, gc, i-j,j, i-j-ll,j+ll);
	XDrawLine(dpy, cg->bgShadPixmap, gc, i-j+picH,j, i-j-ll+picH,j+ll);
      }
    }
    butEnv_stdFill(cg->env);
  }
}


static bool  checkRules(ClpEntry *ce)  {
  bool  err;
  int  i;

  i = clpEntry_iGetInt(ce, &err);
  return(!err && (i >= 0) && (i < goRules_num));
}


static bool  checkSize(ClpEntry *ce)  {
  bool  err;
  int  i;

  i = clpEntry_iGetInt(ce, &err);
  return(!err && (i >= 2) && (i <= GOBOARD_MAXSIZE));
}


static bool  checkKomi(ClpEntry *ce)  {
  bool  err;
  double  i;

  i = clpEntry_iGetDouble(ce, &err);
  return(!err && (i >= -999.5) && (i <= 999.5) &&
	 (i * 2.0 == (double)((int)(i * 2.0))));
}


static bool  checkHandicap(ClpEntry *ce)  {
  bool  err;
  int  i;

  i = clpEntry_iGetInt(ce, &err);
  return(!err && (i >= 0) && (i <= 27));
}


static bool  checkTimeType(ClpEntry *ce)  {
  bool  err;
  GoTimeType  t;

  t = (GoTimeType)clpEntry_iGetInt(ce, &err);
  return(!err && (t <= goTime_ing));
}


static bool  checkTimeValue(ClpEntry *ce)  {
  bool  err;
  int  time;

  time = goTime_parseChars(clpEntry_iGetStr(ce, NULL), FALSE, &err);
  return ((time >= 0) && !err);
}


static bool  checkBYStones(ClpEntry *ce)  {
  bool  err;
  int  i;

  i = clpEntry_iGetInt(ce, &err);
  return(!err && (i >= 1));
}


static bool  checkBProp(ClpEntry *ce)  {
  bool  err;
  double  d;

  d = clpEntry_iGetDouble(ce, &err);
  return(!err && (d >= 1.0));
}


static bool  checkGmpOption(ClpEntry *ce)  {
  bool  err;
  int  i;

  i = clpEntry_iGetInt(ce, &err);
  return(!err && (i >= 0) && (i <= 4));
}


static bool  checkPortNum(ClpEntry *ce)  {
  bool  err;
  int  i;

  i = clpEntry_iGetInt(ce, &err);
  return(!err && (i <= 65535) && (i > 0));
}


ButOut  cgoban_createHelpWindow(But *but)  {
  Cgoban  *cg;
  const Help  *helpInfo;
  int  x, y, w, h;
  bool  err;

  cg = butEnv_packet(butWin_env(but_win(but)));
  assert(MAGIC(cg));
  helpInfo = but_packet(but);
  assert(helpInfo != NULL);
  if (cg->help)  {
    abutHelp_newPages(cg->help, helpInfo->menuTitle,
		      helpInfo->pages, helpInfo->numPages);
  } else  {
    x = clpEntry_iGetInt(clp_lookup(cg->clp, "help.x"), &err);
    if (err)  {
      x = BUT_NOCHANGE;
      y = BUT_NOCHANGE;
    } else
      y = clp_getInt(cg->clp, "help.y");
    w = (int)(cg->fontH * clp_getDouble(cg->clp, "help.w") + 0.5);
    h = (int)(cg->fontH * clp_getDouble(cg->clp, "help.h") + 0.5);
    cg->help = abutHelp_create(cg->abut, "Cgoban Help",
			       helpInfo->menuTitle,
			       helpInfo->pages,
			       helpInfo->numPages, x, y, w, h,
			       8 * cg->fontH, 8 * cg->fontH);
    abutHelp_setDestroyCallback(cg->help, helpDead, cg);
  }
  return(0);
}


static ButOut  helpDead(AbutHelp *help, void *packet)  {
  Cgoban  *cg = packet;

  assert(MAGIC(cg));
  cg->help = NULL;
  clp_setInt(cg->clp, "help.x", butWin_x(help->win));
  clp_setInt(cg->clp, "help.y", butWin_y(help->win));
  clp_setDouble(cg->clp, "help.w",
		(double)butWin_w(help->win) / (double)cg->fontH);
  clp_setDouble(cg->clp, "help.h",
		(double)butWin_h(help->win) / (double)cg->fontH);
  return(0);
}
