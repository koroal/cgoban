/*
 * src/editBoard.c, part of Complete Goban (game program)
 * Copyright (C) 1995-1997 William Shubert
 * See "configure.h.in" for more copyright information.
 */

#include <wms.h>
#include <wms/str.h>
#include <wms/clp.h>
#include <but/but.h>
#include <abut/msg.h>
#include <but/text.h>
#include <abut/term.h>
#include <abut/fsel.h>
#include <but/plain.h>
#include <but/ctext.h>
#include <but/textin.h>
#include "cgoban.h"
#include "sgf.h"
#include "sgfIn.h"
#include "sgfPlay.h"
#include "sgfOut.h"
#include "goban.h"
#include "msg.h"
#include "sgfMap.h"
#ifdef  _EDITBOARD_H_
#error  Levelization Error.
#endif
#include "editBoard.h"


/**********************************************************************
 * Data types
 **********************************************************************/
typedef enum  {
  setMark_toggle, setMark_forceOn, setMark_forceOff
} SetMarkAction;


/**********************************************************************
 * Forward declarations
 **********************************************************************/
static void  toolsQuit(void *packet);
static GobanOut  gridPressed(void *packet, int loc);
static GobanOut  quitPressed(void *packet);
static GobanOut  passPressed(void *packet);
static GobanOut  rewPressed(void *packet);
static GobanOut  backPressed(void *packet);
static GobanOut  fwdPressed(void *packet);
static GobanOut  ffPressed(void *packet);
static GobanOut  donePressed(void *packet);
static GobanOut  disputePressed(void *packet);
static GobanOut  savePressed(void *packet);
static GobanOut  gameInfoPressed(void *packet);
static void  writeGobanComments(EditBoard *e), readGobanComments(EditBoard *e);
static void  clearComments(EditBoard *eb);
static bool  backOk(void *packet);
static bool  fwdOk(void *packet);
static void  saveFile(AbutFsel *fsel, void *packet, const char *fname);
static void  gobanDestroyed(void *packet);
static GobanOut  jumpToLoc(EditBoard *eb, int loc);
static void  addPattern(EditBoard *eb, GoStone color, int loc);
static void  setMark(EditBoard *eb, SgfType sType, int loc,
		     SetMarkAction action);
static void  addLetter(EditBoard *eb, int loc);
static bool  addNumber(EditBoard *eb, int loc, bool moveNum);
static bool  markGroup(EditBoard *eb, SgfType type, int loc);
static ButOut  newTool(void *packet);
static ButOut  shiftPressed(But *but, bool press);
static ButOut  newActiveSgfNode(void *packet, int nodeNum);
static void  changeWhoseMove(EditBoard *eb);
static ButOut  reallyQuit(But *but);
static ButOut  dontQuit(But *but);
static ButOut  quitWinDead(void *packet);
static EditBoard  *createSgf(Cgoban *cg, Sgf *sgf, const char *fName);
static void  infoDead(EditInfo *info, void *packet);


/**********************************************************************
 * Global variables
 **********************************************************************/
static const GobanActions  editBoard_actions = {
  gridPressed, quitPressed, passPressed, rewPressed, backPressed,
  fwdPressed, ffPressed, donePressed, disputePressed, savePressed,
  NULL, /* editPressed / printPressed */ gameInfoPressed,
  &help_editBoard,
  gobanDestroyed,
  backOk, backOk, fwdOk, fwdOk};


/**********************************************************************
 * Functions
 **********************************************************************/
EditBoard  *editBoard_create(Cgoban *cg, const char *fName)  {
  Sgf  *sgf;
  const char  *err;
  Str  *tmpTitle;
  bool  noFile = FALSE;

  sgf = sgf_createFile(cg, fName, &err, &noFile);
  if (sgf == NULL)  {
    if (noFile)  {
      tmpTitle = str_create();
      str_print(tmpTitle, msg_noSuchGame, fName);
      abutMsg_winCreate(cg->abut, "Cgoban Error", str_chars(tmpTitle));
      str_destroy(tmpTitle);
    } else  {
      abutMsg_winCreate(cg->abut, "Cgoban Error", err);
    }
    return(FALSE);
  }
  return(createSgf(cg, sgf, fName));
}


EditBoard  *editBoard_createSgf(Cgoban *cg, const Sgf *sgf)  {
  EditBoard  *eb;

  eb = createSgf(cg, sgf_copy(sgf), NULL);
  if (eb != NULL) {
    ffPressed(eb);
    goban_update(eb->goban);
  }
  return(eb);
}


static EditBoard  *createSgf(Cgoban *cg, Sgf *sgf, const char *fName)  {
  EditBoard  *eb;
  SgfElem  *me;
  const char  *white = NULL, *black = NULL, *title;
  GoRules  rules;
  int  size;
  int  hcap;
  float  komi;
  GoTime  time;
  static const ButKey  shiftKeys[] = {{XK_Shift_L, 0,0},
				      {XK_Shift_R,0,0}, {0,0,0}};
  static const ButKey  shiftUpKeys[] = {{XK_Up, ShiftMask, ShiftMask},
					{0,0,0}};
  static const ButKey  shiftDownKeys[] = {{XK_Down, ShiftMask, ShiftMask},
					  {0,0,0}};
  Str  *tmpTitle = NULL;

  sgf->mode = sgfInsert_variant;
  eb = wms_malloc(sizeof(EditBoard));
  MAGIC_SET(eb);
  eb->cg = cg;
  eb->fsel = NULL;
  eb->reallyQuit = NULL;
  eb->sgf = sgf;

  me = sgf_findType(sgf, sgfType_rules);
  if (me)  {
    rules = (GoRules)me->iVal;
  } else
    rules = goRules_japanese;
  me = sgf_findType(sgf, sgfType_size);
  if (me)  {
    size = me->iVal;
  } else  {
    size = 19;
  }
  me = sgf_findType(sgf, sgfType_handicap);
  if (me)  {
    hcap = me->iVal;
  } else  {
    hcap = 0;
  }
  me = sgf_findType(sgf, sgfType_komi);
  if (me)  {
    komi = (float)me->iVal / 2.0;
  } else  {
    komi = 0.0;
  }
  me = sgf_findFirstType(sgf, sgfType_playerName);
  while (me)  {
    if (me->gVal == goStone_white)
      white = str_chars(me->sVal);
    else  {
      assert(me->gVal == goStone_black);
      black = str_chars(me->sVal);
    }
    me = sgfElem_findFirstType(me, sgfType_playerName);
  }
  str_init(&eb->fName);
  if (fName != NULL)  {
    str_copyChars(&eb->fName, fName);
  } else if (white && black)  {
    str_print(&eb->fName, "%s-%s.sgf", white, black);
  } else  {
    str_copyChars(&eb->fName, "game.sgf");
  }
  me = sgf_findFirstType(sgf, sgfType_time);
  if (me)  {
    goTime_parseDescribeChars(&time, str_chars(me->sVal));
  } else  {
    time.type = goTime_none;
  }
  eb->game = goGame_create(size, rules, hcap, komi, &time, TRUE);
  sgf_play(sgf, eb->game, NULL, eb->currentNodeNum = 0, NULL);
  ++eb->game->maxMoves;
      
  me = sgf_findFirstType(sgf, sgfType_title);
  if (me)  {
    title = str_chars(me->sVal);
  } else  {
    if (white && black)  {
      tmpTitle = str_create();
      str_print(tmpTitle, msg_localTitle, white, black);
      title = str_chars(tmpTitle);
    } else
      title = msg_noTitle;
  }

  eb->goban = goban_create(cg, &editBoard_actions, eb, eb->game, "edit",
			   title);
  if (tmpTitle)
    str_destroy(tmpTitle);
  eb->goban->iDec1 = stoneGroup_create(&cg->cgbuts, eb->goban->iWin, 2,
				       BUT_DRAWABLE);
  assert((hcap == 0) || ((hcap >= 2) && (hcap <= 27)));
  butCt_setText(eb->goban->edit, msg_printGame);
  
  eb->lastComment = NULL;

  editToolWin_init(&eb->tools, cg, eb->sgf, toolsQuit,
		   newTool, newActiveSgfNode, eb);
  eb->oldTool = editTool_changeBoard;
  eb->shiftKeytrap = butKeytrap_create(shiftPressed, eb, eb->goban->win,
				       BUT_DRAWABLE|BUT_PRESSABLE);
  but_setKeys(eb->shiftKeytrap, shiftKeys);
  eb->prevVar = butKeytrap_create(editToolWin_shiftUpPressed, &eb->tools,
				  eb->goban->win, BUT_DRAWABLE|BUT_PRESSABLE);
  but_setKeys(eb->prevVar, shiftUpKeys);
  eb->nextVar = butKeytrap_create(editToolWin_shiftDownPressed, &eb->tools,
				  eb->goban->win, BUT_DRAWABLE|BUT_PRESSABLE);
  but_setKeys(eb->nextVar, shiftDownKeys);
  eb->invertShift = FALSE;
  butKeytrap_setHold(eb->shiftKeytrap, FALSE);
  writeGobanComments(eb);
  newTool(eb);
  eb->info = NULL;

  return(eb);
}


void  editBoard_destroy(EditBoard *eb)  {
  assert(MAGIC(eb));
  str_deinit(&eb->fName);
  if (eb->reallyQuit)  {
    abutMsg_destroy(eb->reallyQuit, FALSE);
    eb->reallyQuit = NULL;
  }
  if (eb->sgf)
    sgf_destroy(eb->sgf);
  if (eb->goban)
    goban_destroy(eb->goban, FALSE);
  if (eb->game)
    goGame_destroy(eb->game);
  editToolWin_deinit(&eb->tools);
  if (eb->info)  {
    editInfo_destroy(eb->info, FALSE);
    eb->info = NULL;
  }
  MAGIC_UNSET(eb);
  wms_free(eb);
}


static void  toolsQuit(void *packet)  {
  quitPressed(packet);
}


static GobanOut  gridPressed(void *packet, int loc)  {
  EditBoard  *eb = packet;
  ButEnv  *env;
  GobanOut  result = gobanOut_draw;

  assert(MAGIC(eb));
  assert(eb->sgf->active != NULL);
  env = eb->cg->env;
  switch(eb->tools.tool)  {
  case editTool_play:
    if (butEnv_keyModifiers(env) & ShiftMask)  {
      result = jumpToLoc(eb, loc);
    } else  {
      if (goBoard_stone(eb->game->board, loc) != goStone_empty)  {
	result = gobanOut_err;
      } else  {
	eb->tools.modified = TRUE;
	readGobanComments(eb);
	sgf_addNode(eb->sgf);
	assert(eb->sgf->active->mapX < 1000);
	editToolWin_nodeAdded(&eb->tools, eb->sgf->active);
	sgf_move(eb->sgf, goGame_whoseMove(eb->game),
		 goBoard_loc2Sgf(eb->game->board, loc));
	sgf_play(eb->sgf, eb->game, eb->goban->pic, ++eb->currentNodeNum,
		 NULL);
	editToolWin_newActiveNode(&eb->tools, eb->sgf->active);
	writeGobanComments(eb);
      }
    }
    break;
  case editTool_changeBoard:
    eb->tools.modified = TRUE;
    if (goStone_isStone(goBoard_stone(eb->game->board, loc)))
      addPattern(eb, goStone_empty, loc);
    else if (butEnv_keyModifiers(env) & ShiftMask)
      addPattern(eb, goStone_black, loc);
    else
      addPattern(eb, goStone_white, loc);
    break;
  case editTool_score:
    if (goStone_isStone(goBoard_stone(eb->game->board, loc)))
      goGame_markDead(eb->game, loc);
    else
      result = gobanOut_err;
    break;
  case editTool_triangle:
    if (butEnv_keyModifiers(env) & ShiftMask)  {
      if (!markGroup(eb, sgfType_triangle, loc))
	result = gobanOut_err;
      else  {
	eb->tools.modified = TRUE;
      }
    } else  {
      eb->tools.modified = TRUE;
      setMark(eb, sgfType_triangle, loc, setMark_toggle);
    }
    break;
  case editTool_square:
    if (butEnv_keyModifiers(env) & ShiftMask)  {
      if (!markGroup(eb, sgfType_square, loc))
	result = gobanOut_err;
      else  {
	eb->tools.modified = TRUE;
      }
    } else  {
      eb->tools.modified = TRUE;
      setMark(eb, sgfType_square, loc, setMark_toggle);
    }
    break;
  case editTool_circle:
    if (butEnv_keyModifiers(env) & ShiftMask)  {
      if (!markGroup(eb, sgfType_circle, loc))
	result = gobanOut_err;
      else  {
	eb->tools.modified = TRUE;
      }
    } else  {
      eb->tools.modified = TRUE;
      setMark(eb, sgfType_circle, loc, setMark_toggle);
    }
    break;
  case editTool_letter:
    eb->tools.modified = TRUE;
    addLetter(eb, loc);
    break;
  case editTool_number:
    if (addNumber(eb, loc, butEnv_keyModifiers(env) & ShiftMask))  {
      eb->tools.modified = TRUE;
    } else
      result = gobanOut_err;
    break;
  }
  editToolWin_newColor(&eb->tools, goGame_whoseMove(eb->game));
  assert(eb->sgf->active != NULL);
  sgfMap_changeNode(eb->tools.sgfMap, eb->sgf->active);
  return(result);
}


static GobanOut  jumpToLoc(EditBoard *eb, int loc)  {
  char  moveTo[5];
  int  newNode, i;

  strcpy(moveTo, goBoard_loc2Sgf(eb->game->board, loc));
  newNode = -1;
  if (goBoard_stone(eb->game->board, loc) == goStone_empty)  {
    i = sgfElem_findMove(eb->sgf->active, moveTo, 1);
    if (i >= 0)  {
      newNode = eb->currentNodeNum + i;
    } else  {
      i = sgfElem_findMove(eb->sgf->active, moveTo, -1);
      if (i >= 0)
	newNode = eb->currentNodeNum - i;
    }
  } else  {
    i = sgfElem_findMove(eb->sgf->active, moveTo, -1);
    assert(i >= 0);
    newNode = eb->currentNodeNum - i;
  }
  if (newNode > 0)  {
    readGobanComments(eb);
    sgf_play(eb->sgf, eb->game, eb->goban->pic, eb->currentNodeNum = newNode,
	     NULL);
    editToolWin_newActiveNode(&eb->tools, eb->sgf->active);
    writeGobanComments(eb);
    return(gobanOut_draw);
  } else
    return(gobanOut_err);
}


static GobanOut  quitPressed(void *packet)  {
  EditBoard  *eb = packet;
  AbutMsgOpt  buttons[2];
  Str  saveMessage;

  assert(MAGIC(eb));
  readGobanComments(eb);
  if (eb->tools.modified)  {
    buttons[0].name = msg_noCancel;
    buttons[0].callback = dontQuit;
    buttons[0].packet = eb;
    buttons[0].keyEq = NULL;
    buttons[1].name = msg_yesQuit;
    buttons[1].callback = reallyQuit;
    buttons[1].packet = eb;
    buttons[1].keyEq = NULL;
    str_init(&saveMessage);
    str_print(&saveMessage, msg_reallyQuit,
	      str_chars(&eb->fName));
    if (eb->reallyQuit)
      abutMsg_destroy(eb->reallyQuit, FALSE);
    eb->reallyQuit = abutMsg_winOptCreate(eb->cg->abut, "Cgoban Warning",
					  str_chars(&saveMessage),
					  quitWinDead, eb, 2, buttons);
    str_deinit(&saveMessage);
  } else
    editBoard_destroy(packet);
  return(gobanOut_noDraw);
}


static GobanOut  passPressed(void *packet)  {
  EditBoard  *eb = packet;

  assert(MAGIC(eb));
  switch(eb->tools.tool)  {
  case editTool_play:
    readGobanComments(eb);
    sgf_addNode(eb->sgf);
    assert(eb->sgf->active->parent->activeChild == eb->sgf->active);
    editToolWin_nodeAdded(&eb->tools, eb->sgf->active);
    assert(eb->sgf->active->parent->activeChild == eb->sgf->active);
    readGobanComments(eb);
    assert(eb->sgf->active->type == sgfType_node);
    assert(eb->sgf->active->parent->activeChild == eb->sgf->active);
    assert((eb->sgf->active->parent->childH !=
	    eb->sgf->active->parent->childT) ||
	   (eb->sgf->active->parent->activeChild ==
	    eb->sgf->active->parent->childH));
    sgf_pass(eb->sgf, goGame_whoseMove(eb->game));
    assert(eb->sgf->active->type == sgfType_pass);
    assert(eb->sgf->active->parent->activeChild == eb->sgf->active);
    assert(eb->sgf->active->parent->activeChild ==
	   eb->sgf->active->parent->childH);
    assert(eb->sgf->active->parent->activeChild ==
	   eb->sgf->active->parent->childT);
    assert((eb->sgf->active->parent->parent->childH !=
	    eb->sgf->active->parent->parent->childT) ||
	   (eb->sgf->active->parent->parent->activeChild ==
	    eb->sgf->active->parent->parent->childH));
    sgf_play(eb->sgf, eb->game, eb->goban->pic, ++eb->currentNodeNum, NULL);
    editToolWin_newActiveNode(&eb->tools, eb->sgf->active);
    writeGobanComments(eb);
    break;
  case editTool_changeBoard:
    readGobanComments(eb);
    sgf_addNode(eb->sgf);
    clearComments(eb);
    editToolWin_newActiveNode(&eb->tools, eb->sgf->active);
    writeGobanComments(eb);
    break;
  default:
    break;
  }    
  editToolWin_newColor(&eb->tools, goGame_whoseMove(eb->game));
  sgfMap_changeNode(eb->tools.sgfMap, eb->sgf->active);
  return(gobanOut_draw);
}


static GobanOut  rewPressed(void *packet)  {
  EditBoard  *e = packet;

  assert(MAGIC(e));
  readGobanComments(e);
  sgf_play(e->sgf, e->game, e->goban->pic, e->currentNodeNum = 0, NULL);
  editToolWin_newActiveNode(&e->tools, e->sgf->active);
  writeGobanComments(e);
  editToolWin_newColor(&e->tools, goGame_whoseMove(e->game));
  return(gobanOut_draw);
}


static GobanOut  backPressed(void *packet)  {
  EditBoard  *eb = packet;
  SgfElem  *active;
  int  backCount = 1;

  assert(MAGIC(eb));
  readGobanComments(eb);
  if (butEnv_keyModifiers(eb->cg->env) & ShiftMask)  {
    /*
     * Jump to next node with comments or a variation.
     */
    backCount = 0;
    active = eb->sgf->active;
    while (active)  {
      if (active->type == sgfType_node)  {
	++backCount;
      } else if ((active->type >= sgfType_setBoard) &&
		 (active->type <= sgfType_comment) && backCount)
	break;
      if ((backCount > 0) && (active->childH != active->childT))
	break;
      active = active->parent;
    }
  }
  sgf_play(eb->sgf, eb->game, eb->goban->pic, eb->currentNodeNum -= backCount,
	   NULL);
  editToolWin_newActiveNode(&eb->tools, eb->sgf->active);
  writeGobanComments(eb);
  editToolWin_newColor(&eb->tools, goGame_whoseMove(eb->game));
  return(gobanOut_draw);
}


static GobanOut  fwdPressed(void *packet)  {
  EditBoard  *eb = packet;
  SgfElem  *active;
  int  fwdCount = 1;

  assert(MAGIC(eb));
  readGobanComments(eb);
  if (butEnv_keyModifiers(eb->cg->env) & ShiftMask)  {
    /*
     * Jump to next node with comments or a variation.
     */
    active = eb->sgf->active->activeChild->activeChild;
    while (active)  {
      if (active->type == sgfType_node)  {
	++fwdCount;
      } else if ((active->type >= sgfType_setBoard) &&
		 (active->type <= sgfType_comment))
	break;
      if (active->childH != active->childT)
	break;
      active = active->activeChild;
    }
  }
  sgf_play(eb->sgf, eb->game, eb->goban->pic, eb->currentNodeNum += fwdCount,
	   NULL);
  editToolWin_newActiveNode(&eb->tools, eb->sgf->active);
  writeGobanComments(eb);
  editToolWin_newColor(&eb->tools, goGame_whoseMove(eb->game));
  return(gobanOut_draw);
}


static GobanOut  ffPressed(void *packet)  {
  EditBoard  *e = packet;

  assert(MAGIC(e));
  readGobanComments(e);
  e->currentNodeNum = sgf_play(e->sgf, e->game, e->goban->pic, -1, NULL);
  editToolWin_newActiveNode(&e->tools, e->sgf->active);
  writeGobanComments(e);
  editToolWin_newColor(&e->tools, goGame_whoseMove(e->game));
  return(gobanOut_draw);
}


static GobanOut  donePressed(void *packet)  {
  SgfElem  *otherTerrMarks;
  int  i;
  EditBoard  *eb = packet;
  Str  *scoreComment;

  assert(MAGIC(eb));
  readGobanComments(eb);
  while ((otherTerrMarks = sgfElem_findTypeInNode(eb->sgf->active,
						  sgfType_territory)))  {
    sgfElem_snip(otherTerrMarks, eb->sgf);
  }
  for (i = 0;  i < goBoard_area(eb->game->board);  ++i)  {
    if (((eb->game->flags[i] & GOGAMEFLAGS_SEEN) == GOGAMEFLAGS_SEEWHITE) &&
	((goBoard_stone(eb->game->board, i) == goStone_empty) ||
	 (eb->game->flags[i] & GOGAMEFLAGS_MARKDEAD)))
      sgf_addTerritory(eb->sgf, goStone_white,
		       goBoard_loc2Sgf(eb->game->board, i));
  }
  for (i = 0;  i < goBoard_area(eb->game->board);  ++i)  {
    if (((eb->game->flags[i] & GOGAMEFLAGS_SEEN) == GOGAMEFLAGS_SEEBLACK) &&
	((goBoard_stone(eb->game->board, i) == goStone_empty) ||
	 (eb->game->flags[i] & GOGAMEFLAGS_MARKDEAD)))
      sgf_addTerritory(eb->sgf, goStone_black,
		       goBoard_loc2Sgf(eb->game->board, i));
  }
  scoreComment = goScore_str(&eb->goban->score, eb->game,
			     &eb->game->time, eb->goban->timers);
  sgf_catComment(eb->sgf, str_chars(scoreComment));
  sgfMap_changeNode(eb->tools.sgfMap, eb->sgf->active);
  str_destroy(scoreComment);
  editToolWin_newTool(&eb->tools, editTool_play, TRUE);
  writeGobanComments(eb);
  sgfMap_changeNode(eb->tools.sgfMap, eb->sgf->active);
  return(gobanOut_noDraw);
}


static GobanOut  disputePressed(void *packet)  {
  return(gobanOut_noDraw);
}


static GobanOut  savePressed(void *packet)  {
  EditBoard  *e = packet;

  assert(MAGIC(e));
  if (e->fsel)
    abutFsel_destroy(e->fsel, FALSE);
  e->fsel = abutFsel_create(e->cg->abut, saveFile, e, "CGoban",
			    msg_saveGameName,
			    str_chars(&e->fName));
  return(gobanOut_noDraw);
}


static void  saveFile(AbutFsel *fsel, void *packet, const char *fname)  {
  EditBoard  *e = packet;
  int error;
  Str str;

  assert(MAGIC(e));
  str_copy(&e->cg->lastDirAccessed, &fsel->pathVal);
  if (fname != NULL)  {
    str_copyChars(&e->fName, fname);
    clp_setStr(e->cg->clp, "edit.sgfName", butTextin_get(fsel->in));
    readGobanComments(e);
    e->tools.modified = FALSE;
    if (e->info)
      editInfo_updateSgf(e->info);
    if (!sgf_writeFile(e->sgf, fname, &error)) {
      str_init(&str);
      str_print(&str, "Error saving file \"%s\": %s",
		fname, strerror(errno));
      cgoban_createMsgWindow(e->cg, "Cgoban Error", str_chars(&str));
      str_deinit(&str);
    }
  }
  e->fsel = NULL;
}


static void  clearComments(EditBoard *eb)  {
  assert(MAGIC(eb));
  goban_setComments(eb->goban, "");
}


/*
 * This transfers comments from the SGF move chain to the goban.
 */
static void  writeGobanComments(EditBoard *e)  {
  SgfElem  *comElem;

  assert(MAGIC(e));
  comElem = sgfElem_findTypeInNode(e->sgf->active, sgfType_comment);
  if (comElem)  {
    if (strcmp(str_chars(comElem->sVal), goban_getComments(e->goban)))
      goban_setComments(e->goban, str_chars(comElem->sVal));
  } else  {
    if (e->lastComment)
      goban_setComments(e->goban, "");
  }
  e->lastComment = comElem;
}


/*
 * This transfers comments from the goban to the SGF move chain.
 */
static void  readGobanComments(EditBoard *eb)  {
  const char  *comm;
  int  cmpLen;

  assert(MAGIC(eb));
  assert(eb->sgf->active->parent->activeChild == eb->sgf->active);
  comm = goban_getComments(eb->goban);
  if (comm[0])  {
    if (eb->lastComment)  {
      cmpLen = str_len(eb->lastComment->sVal);
      if (str_chars(eb->lastComment->sVal)[cmpLen - 1] != '\n')  {
	/* A "\n" was added. */
	if ((strlen(comm) == cmpLen + 1) &&
	    !strncmp(comm, str_chars(eb->lastComment->sVal), cmpLen))
	  return;
      } else  {
	if (!strcmp(comm, str_chars(eb->lastComment->sVal)))
	  return;
      }
    }
    eb->tools.modified = TRUE;
    eb->sgf->mode = sgfInsert_inline;
    sgf_comment(eb->sgf, comm);
    eb->sgf->mode = sgfInsert_variant;
    eb->lastComment = sgfElem_findTypeInNode(eb->sgf->active, sgfType_comment);
    assert(eb->lastComment != NULL);
    assert(eb->sgf->active->parent->activeChild == eb->sgf->active);
  } else if (eb->lastComment) {
    eb->tools.modified = TRUE;
    sgfElem_snip(eb->lastComment, eb->sgf);
    eb->lastComment = NULL;
    assert(eb->sgf->active->parent->activeChild == eb->sgf->active);
  }
  sgfMap_setMapPointer(eb->tools.sgfMap, eb->sgf->active);
}


static bool  backOk(void *packet)  {
  EditBoard  *e = packet;

  assert(MAGIC(e));
  return((e->game->state == goGameState_play) && (e->currentNodeNum > 0));
}


static bool  fwdOk(void *packet)  {
  EditBoard  *e = packet;

  assert(MAGIC(e));
  return((e->game->state == goGameState_play) &&
	 (e->sgf->active->activeChild != NULL));
}


static void  gobanDestroyed(void *packet)  {
  EditBoard  *eb = packet;

  assert(MAGIC(eb));
  eb->goban = NULL;
  editBoard_destroy(eb);
}


static void  addPattern(EditBoard *eb, GoStone color, int loc)  {
  SgfElem  *otherEdits, *oldActive;
  GoBoard  *newBoard, *tmp;
  int  i;

  otherEdits = sgfElem_findTypeInNode(eb->sgf->active, sgfType_setBoard);
  if (otherEdits == NULL)  {
    /*
     * We always add a node if there are no other edits, 'cause they
     *   shouldn't be in the same node as moves.
     */
    readGobanComments(eb);
    sgf_addNode(eb->sgf);
    editToolWin_nodeAdded(&eb->tools, eb->sgf->active);
    ++eb->currentNodeNum;
    editToolWin_newActiveNode(&eb->tools, eb->sgf->active);
    writeGobanComments(eb);
  }
  /*
   * What we do is add the stone, copy the board, then recreate the game
   *   just before this group of stones.  Then we scan through the two boards,
   *   looking for differences, and recording them in the sgf file.  This
   *   is very time consuming, but it's the easient way to do things so for
   *   now I'll leave it as is.
   */
  newBoard = goBoard_create(eb->game->size);
  goGame_setBoard(eb->game, color, loc);
  goBoard_copy(eb->game->board, newBoard);
  if (eb->currentNodeNum == 0)  {
    /* We must start from a fresh board! */
    goGame_moveTo(eb->game, 0);
  } else  {
    /* We want to preserve the active node across this call to "play()". */
    oldActive = eb->sgf->active;
    sgf_play(eb->sgf, eb->game, NULL, eb->currentNodeNum - 1, NULL);
    eb->sgf->active = oldActive;
  }

  eb->sgf->mode = sgfInsert_inline;

  /*
   * Now we have to snip out all previous edits.
   */
  while ((otherEdits = sgfElem_findTypeInNode(eb->sgf->active,
					      sgfType_setBoard)))  {
    sgfElem_snip(otherEdits, eb->sgf);
  }

  /* First add all the empties... */
  for (i = 0;  i < goBoard_area(newBoard);  ++i)  {
    if ((goBoard_stone(newBoard, i) == goStone_empty) &&
	(goBoard_stone(eb->game->board, i) != goStone_empty))
      sgf_addStone(eb->sgf, goStone_empty, goBoard_loc2Sgf(newBoard, i));
  }
  /* Now the white stones... */
  for (i = 0;  i < goBoard_area(newBoard);  ++i)  {
    if ((goBoard_stone(newBoard, i) == goStone_white) &&
	(goBoard_stone(eb->game->board, i) != goStone_white))
      sgf_addStone(eb->sgf, goStone_white, goBoard_loc2Sgf(newBoard, i));
  }
  /* And finally the black stones. */
  for (i = 0;  i < goBoard_area(newBoard);  ++i)  {
    if ((goBoard_stone(newBoard, i) == goStone_black) &&
	(goBoard_stone(eb->game->board, i) != goStone_black))
      sgf_addStone(eb->sgf, goStone_black, goBoard_loc2Sgf(newBoard, i));
  }
  /* Now put the board back the way that it should be. */
  tmp = eb->game->board;
  eb->game->board = newBoard;
  eb->sgf->mode = sgfInsert_variant;
  goBoard_destroy(tmp);
}


static void  setMark(EditBoard *eb, SgfType sType, int loc,
		     SetMarkAction action)  {
  SgfElem  *search;
  bool  remove;
  GoMarkType  markAdded;

  for (search = eb->sgf->active;  search && (search->type != sgfType_node);
       search = search->parent)  {
    if (((search->type == sgfType_triangle) ||
	 (search->type == sgfType_circle) ||
	 (search->type == sgfType_square)) &&
	(goBoard_sgf2Loc(eb->game->board, search->lVal) == loc))  {
      remove = (((search->type == sType) && (action == setMark_toggle)) ||
		(action == setMark_forceOff));
      sgfElem_snip(search, eb->sgf);
      if (remove)  {
	grid_setMark(eb->goban->pic->boardButs[loc], goMark_none, 0);
	return;
      }
      break;
    }
  }
  search = sgfElem_findTypeInNode(eb->sgf->active, sType);
  if (search)
    eb->sgf->active = search;
  eb->sgf->mode = sgfInsert_inline;
  sgf_addLElem(eb->sgf, sType, goBoard_loc2Sgf(eb->game->board, loc));
  eb->sgf->mode = sgfInsert_variant;
  if (sType == sgfType_triangle)  {
    markAdded = goMark_triangle;
  } else if (sType == sgfType_square)  {
    markAdded = goMark_square;
  } else  {
    assert(sType == sgfType_circle);
    markAdded = goMark_circle;
  }
  grid_setMark(eb->goban->pic->boardButs[loc], markAdded, 0);
}


static void  addLetter(EditBoard *eb, int loc)  {
  SgfElem  *search;
  int  i;
  char  letter[2];

  for (search = eb->sgf->active;  search && (search->type != sgfType_node);
       search = search->parent)  {
    if ((search->type == sgfType_label) &&
	(goBoard_sgf2Loc(eb->game->board, search->lVal) == loc))  {
      if (eb->sgf->active == search)
	eb->sgf->active = search->parent;
      sgfElem_snip(search, eb->sgf);
      grid_setMark(eb->goban->pic->boardButs[loc], goMark_none, 0);
      return;
    }
  }
  letter[0] = 'A';
  letter[1] = '\0';
  for (i = 0;  i < goBoard_area(eb->game->board);  ++i)  {
    if (eb->goban->pic->boardButs[i])  {
      if ((grid_markType(eb->goban->pic->boardButs[i]) == goMark_letter) &&
	  (grid_markAux(eb->goban->pic->boardButs[i]) >= letter[0]))
	letter[0] = grid_markAux(eb->goban->pic->boardButs[i]) + 1;
    }
  }
  if (letter[0] == 'Z' + 1)
    letter[0] = 'a';
  else if (letter[0] > 'z')
    letter[0] = 'z';
  eb->sgf->mode = sgfInsert_inline;
  sgf_label(eb->sgf, goBoard_loc2Sgf(eb->game->board, loc), letter);
  eb->sgf->mode = sgfInsert_variant;
  grid_setMark(eb->goban->pic->boardButs[loc], goMark_letter, letter[0]);
}


static bool  addNumber(EditBoard *eb, int loc, bool moveNum)  {
  SgfElem  *search;
  int  i, val;
  char  labelStr[4];
  const char  *sgfLoc;

  for (search = eb->sgf->active;  search && (search->type != sgfType_node);
       search = search->parent)  {
    if ((search->type == sgfType_label) &&
	(goBoard_sgf2Loc(eb->game->board, search->lVal) == loc))  {
      if (eb->sgf->active == search)
	eb->sgf->active = search->parent;
      sgfElem_snip(search, eb->sgf);
      grid_setMark(eb->goban->pic->boardButs[loc], goMark_none, 0);
      return(TRUE);
    }
  }
  if (moveNum)  {
    for (val = eb->game->moveNum - 1;  val >= 0;  --val)  {
      if (eb->game->moves[val].move == loc)
	break;
    }
    if (val < 0)
      return(FALSE);
    else
      ++val;
  } else  {
    val = 1;
    for (i = 0;  i < goBoard_area(eb->game->board);  ++i)  {
      if (eb->goban->pic->boardButs[i])  {
	if ((grid_markType(eb->goban->pic->boardButs[i]) == goMark_number) &&
	    (grid_markAux(eb->goban->pic->boardButs[i]) >= val))
	  val = grid_markAux(eb->goban->pic->boardButs[i]) + 1;
      }
    }
  }
  if (val > 999)
    val = 999;
  sprintf(labelStr, "%d", val);
  eb->sgf->mode = sgfInsert_inline;
  sgfLoc = goBoard_loc2Sgf(eb->game->board, loc);
  sgf_label(eb->sgf, sgfLoc, labelStr);
  eb->sgf->mode = sgfInsert_variant;
  grid_setMark(eb->goban->pic->boardButs[loc], goMark_number, val);
  return(TRUE);
}


static bool  markGroup(EditBoard *eb, SgfType type, int loc)  {
  GoMarkType  oldMark;
  SetMarkAction  action;
  GoBoardGroupIter  i;

  if (!goStone_isStone(goBoard_stone(eb->game->board, loc)))
    return(FALSE);
  oldMark = grid_markType(eb->goban->pic->boardButs[loc]);
  if ((oldMark == goMark_triangle) || (oldMark == goMark_square) ||
      (oldMark == goMark_circle))
    action = setMark_forceOff;
  else
    action = setMark_forceOn;
  goBoardGroupIter(i, eb->game->board, loc)  {
    setMark(eb, type, goBoardGroupIter_loc(i, eb->game->board), action);
  }
  return(TRUE);
}


static ButOut  shiftPressed(But *but, bool press)  {
  EditBoard  *eb = but_packet(but);
  ButOut  result;

  assert(MAGIC(eb));
  /*
   * Alas, the modifier flags don't change until AFTER the press, so we
   *   have to invert the shift here.
   */
  eb->invertShift = TRUE;
  /*
   * Change the oldTool thingy so we don't change color if we're on the
   *   play tool.
   */
  eb->oldTool = editTool_changeBoard;
  result = newTool(but_packet(but));
  eb->invertShift = FALSE;
  return(result);
}


static ButOut  newTool(void *packet)  {
  EditBoard  *eb = packet;
  bool  shiftPressed, updateGoban = FALSE;
  uint  pressAllowed = goPicMove_empty | goPicMove_stone;
  GoGameState  gameState = goGameState_play;

  assert(MAGIC(eb));
  shiftPressed = butEnv_keyModifiers(eb->cg->env) & ShiftMask;
  if (eb->invertShift)
    shiftPressed = !shiftPressed;
  switch(eb->tools.tool)  {
  case editTool_play:
    if (!shiftPressed)
      pressAllowed = goPicMove_legal;
    if (eb->oldTool == editTool_play)  {
      /*
       * Change whose turn it is.
       */
      changeWhoseMove(eb);
      updateGoban = TRUE;
    }
    break;
  case editTool_changeBoard:
    if (!shiftPressed)
      pressAllowed |= goPicMove_forceWhite;
    else
      pressAllowed |= goPicMove_forceBlack;
    break;
  case editTool_score:
    pressAllowed = goPicMove_stone | goPicMove_noPass;
    gameState = goGameState_selectDead;
    break;
  case editTool_triangle:
  case editTool_square:
  case editTool_circle:
  case editTool_number:
    pressAllowed |= goPicMove_noPass;
    if (shiftPressed)
      pressAllowed = goPicMove_stone;
    break;
  case editTool_letter:
    pressAllowed |= goPicMove_noPass;
    break;
  }
  eb->oldTool = eb->tools.tool;
  if (pressAllowed != eb->goban->pic->allowedMoves)  {
    eb->goban->pic->allowedMoves = pressAllowed;
    updateGoban = TRUE;
  }
  if (gameState != eb->game->state)  {
    eb->game->state = gameState;
    if (eb->game->state == goGameState_play)  {
      /*
       * Replay the game to get rid of all those nasty territory markers.
       */
      sgf_play(eb->sgf, eb->game, eb->goban->pic, eb->currentNodeNum,
	       NULL);
    }
    updateGoban = TRUE;
  }
  if (updateGoban)
    goban_update(eb->goban);
  return(0);
}


static ButOut  newActiveSgfNode(void *packet, int nodeNum)  {
  EditBoard  *eb = packet;

  assert(MAGIC(eb));
  if (eb->game->state == goGameState_selectDead) {
    abutMsg_winCreate(eb->cg->abut, "Cgoban Error",
		      msg_changeNodeWhileScoring);
    return(BUTOUT_ERR);
  } else {
    readGobanComments(eb);
    sgf_play(eb->sgf, eb->game, eb->goban->pic, nodeNum, NULL);
    eb->currentNodeNum = nodeNum;
    editToolWin_newActiveNode(&eb->tools, eb->sgf->active);
    writeGobanComments(eb);
    editToolWin_newColor(&eb->tools, goGame_whoseMove(eb->game));
    goban_update(eb->goban);
    return(0);
  }
}


static void  changeWhoseMove(EditBoard *eb)  {
  SgfElem  *oldSetTurn;
  GoStone  newColor;
  
  newColor = goStone_opponent(eb->game->whoseMove);
  oldSetTurn = sgfElem_findTypeInNode(eb->sgf->active, sgfType_whoseMove);
  if (oldSetTurn)  {
    assert(oldSetTurn->gVal != newColor);
    oldSetTurn->gVal = newColor;
  } else  {
    eb->sgf->mode = sgfInsert_inline;
    sgf_setWhoseMove(eb->sgf, newColor);
    eb->sgf->mode = sgfInsert_variant;
  }
  eb->game->setWhoseMoveNum = eb->game->moveNum;
  eb->game->whoseMove = eb->game->setWhoseMoveColor = newColor;
  editToolWin_newColor(&eb->tools, newColor);
  but_draw(eb->tools.selDesc[editTool_play]);
}


static ButOut  reallyQuit(But *but)  {
  editBoard_destroy(but_packet(but));
  return(0);
}


static ButOut  dontQuit(But *but)  {
  EditBoard  *eb = but_packet(but);

  assert(MAGIC(eb));
  abutMsg_destroy(eb->reallyQuit, FALSE);
  eb->reallyQuit = NULL;
  return(0);
}


static ButOut  quitWinDead(void *packet)  {
  EditBoard  *eb = packet;

  assert(MAGIC(eb));
  eb->reallyQuit = NULL;
  return(0);
}


static GobanOut  gameInfoPressed(void *packet)  {
  EditBoard  *eb = packet;

  assert(MAGIC(eb));
  if (eb->info)  {
    XRaiseWindow(butEnv_dpy(eb->cg->env),
		 butWin_xwin(eb->info->win));
  } else  {
    eb->info = editInfo_create(eb->cg, eb->goban, eb->sgf, infoDead, eb);
  }
  return(gobanOut_noDraw);
}


static void  infoDead(EditInfo *info, void *packet)  {
  EditBoard  *eb = packet;

  assert(MAGIC(eb));
  assert(eb->info == info);
  eb->info = NULL;
}
