/*
 * src/editInfo.c, part of Complete Goban (game program)
 * Copyright (C) 1996 William Shubert
 * See "configure.h.in" for more copyright information.
 */

#include <wms.h>
#include "sgf.h"
#include "msg.h"
#include <but/box.h>
#include <but/textin.h>
#ifdef  _EDITINFO_H_
  Levelization Error.
#endif
#include "editInfo.h"


/**********************************************************************
 * Forward Declarations
 **********************************************************************/
static ButOut  unmap(ButWin *win);
static ButOut  map(ButWin *win);
static ButOut  resize(ButWin *win);
static ButOut  destroy(ButWin *win);
static ButOut  newTitle(But *but, const char *newStr);


/**********************************************************************
 * Globals
 **********************************************************************/
static const struct  {
  const char  *title;
  SgfType  type;
  GoStone  color;
} sgfVals[] = {
  {msg_gameTitle,  sgfType_title,     goStone_empty},
  {msg_copyrightC, sgfType_copyright, goStone_empty},
  {msg_wStoneName, sgfType_playerName, goStone_white},
  {msg_bStoneName, sgfType_playerName, goStone_black},
  {msg_wRank, sgfType_playerRank, goStone_white},
  {msg_bRank, sgfType_playerRank, goStone_black},
  {msg_handicapC, sgfType_handicap, goStone_empty},
  {msg_komiC, sgfType_komi, goStone_empty},
  {msg_time, sgfType_time, goStone_empty},
  {msg_result, sgfType_result, goStone_empty},
  {msg_date, sgfType_date, goStone_empty},
  {msg_place, sgfType_place, goStone_empty},
  {msg_event, sgfType_event, goStone_empty},
  {msg_source, sgfType_source, goStone_empty}};


/**********************************************************************
 * Functions
 **********************************************************************/
EditInfo  *editInfo_create(Cgoban *cg, Goban *goban, Sgf *sgf,
			   void (*dCallback)(EditInfo *info, void *packet),
			   void *packet)  {
  int  i, minW, minH, w, h, bw, fontH;
  ButEnv  *env;
  EditInfo  *info;
  bool  err;
  EditInfoVal  val;
  Str  defValBuf;
  const char  *defVal;
  SgfElem  *elLookup;
  ButOut  (*callback)(But *but, const char *newStr);

  info = wms_malloc(sizeof(EditInfo));
  MAGIC_SET(info);
  env = cg->env;
  info->cg = cg;
  info->goban = goban;
  info->sgf = sgf;
  info->destroy = dCallback;
  info->packet = packet;
  
  fontH = cg->fontH;
  bw = butEnv_stdBw(env);
  minW = fontH * 24 + bw * 14;
  minH = minW;
  w = (double)minW * clp_getDouble(cg->clp, "edit.infoW") + 0.5;
  h = (double)minH * clp_getDouble(cg->clp, "edit.infoH") + 0.5;
  info->win = butWin_create(info, cg->env, "Cgoban Game Info", w, h,
			    unmap, map, resize, destroy);
  i = clpEntry_iGetInt(clp_lookup(cg->clp, "edit.infoX"), &err);
  if (!err)
    butWin_setX(info->win, i);
  i = clpEntry_iGetInt(clp_lookup(cg->clp, "edit.infoY"), &err);
  if (!err)
    butWin_setY(info->win, i);
  butWin_setMinW(info->win, minW);
  butWin_setMinH(info->win, minH);
  butWin_setMaxW(info->win, 0);
  butWin_setMaxH(info->win, 0);
  butWin_activate(info->win);
  info->bg = butBoxFilled_create(info->win, 0, BUT_DRAWABLE);
  butBoxFilled_setPixmaps(info->bg, cg->bgLitPixmap, cg->bgShadPixmap,
			  cg->bgPixmap);

  info->winTitle = butText_create(info->win, 1, BUT_DRAWABLE,
				   msg_editInfoTitle,
				   butText_center);
  butText_setFont(info->winTitle, 2);
  str_init(&defValBuf);
  editInfoVal_iter(val)  {
    info->infos[val].name = butText_create(info->win, 1, BUT_DRAWABLE,
					   sgfVals[val].title,
					   butText_left);
    defVal = "";
    for (elLookup = &sgf->top;
	 (elLookup != NULL) && (elLookup->type != sgfType_node);
	 elLookup = elLookup->activeChild)  {
      if ((elLookup->type == sgfVals[val].type) &&
	  ((sgfVals[val].color == goStone_empty) ||
	   (sgfVals[val].color == elLookup->gVal)))  {
	switch(elLookup->type)  {
	case sgfType_komi:
	  str_print(&defValBuf, "%g", (double)elLookup->iVal / 2.0);
	  defVal = str_chars(&defValBuf);
	  break;
	case sgfType_handicap:
	  str_print(&defValBuf, "%d", elLookup->iVal);
	  defVal = str_chars(&defValBuf);
	  break;
	default:
	  defVal = str_chars(elLookup->sVal);
	  break;
	}
	break;
      }
    }
    if (val == editInfo_gameTitle)
      callback = newTitle;
    else
      callback = NULL;
    info->infos[val].in = butTextin_create(callback, info, info->win, 1,
					   BUT_DRAWABLE|BUT_PRESSABLE,
					   defVal, 1000);
  }
  str_deinit(&defValBuf);
  info->gameComment = abutTerm_create(cg->abut, info->win, 1, TRUE);
  for (elLookup = &sgf->top;
       (elLookup != NULL) && (elLookup->type != sgfType_node);
       elLookup = elLookup->activeChild)  {
    if (elLookup->type == sgfType_gameComment)  {
      abutTerm_set(info->gameComment, str_chars(elLookup->sVal));
      break;
    }
  }

  return(info);
}


void  editInfo_destroy(EditInfo *info, bool propagate)  {
  Cgoban  *cg;

  assert(MAGIC(info));
  if (propagate)  {
    info->destroy(info, info->packet);
    assert(MAGIC(info));
  }
  if (info->win)  {
    cg = info->cg;
    assert(MAGIC(cg));
    clp_setDouble(cg->clp, "edit.infoW",
		  (double)butWin_w(info->win) /
		  (double)butWin_getMinW(info->win));
    clp_setDouble(cg->clp, "edit.infoH",
		  (double)butWin_h(info->win) /
		  (double)butWin_getMinH(info->win));
    clp_setInt(cg->clp, "edit.infoX", butWin_x(info->win));
    clp_setInt(cg->clp, "edit.infoY", butWin_y(info->win));
    butWin_setDestroy(info->win, NULL);
    butWin_destroy(info->win);
  }
  MAGIC_UNSET(info);
  wms_free(info);
}


static ButOut  unmap(ButWin *win)  {
  return(0);
}


static ButOut  map(ButWin *win)  {
  return(0);
}


static ButOut  resize(ButWin *win)  {
  EditInfo  *info = butWin_packet(win);
  int  fontH, bw, w, h;
  int  y;
  int  w2;
  int  lw, rw;
  EditInfoVal  val;
  int  textW, maxTextW;

  assert(MAGIC(info));
  w = butWin_w(info->win);
  h = butWin_h(info->win);
  fontH = info->cg->fontH;
  bw = butEnv_stdBw(butWin_env(win));
  but_resize(info->bg, 0, 0, w, h);
  but_resize(info->winTitle, bw*2, y = bw*2, w - bw*4, fontH*2);

  maxTextW = 0;
  editInfoVal_iter(val)  {
    textW = butEnv_textWidth(info->cg->env, sgfVals[val].title, 0);
    if (textW > maxTextW)
      maxTextW = textW;
  }
  w2 = (w + bw) / 2;
  lw = w2 - bw * 3;
  rw = w - bw * 2 - w2;
  editInfoVal_iter(val)  {
    if (val <= editInfo_copyright)  {
      but_resize(info->infos[val].name, bw*2, y += bw + fontH*2,
		 maxTextW, fontH*2);
      but_resize(info->infos[val].in, bw*3 + maxTextW, y,
		 w - (bw*5 + maxTextW), fontH*2);
    } else  {
      but_resize(info->infos[val].name, bw*2, y += bw + fontH*2,
		 maxTextW, fontH*2);
      but_resize(info->infos[val].in, bw*3 + maxTextW, y,
		 lw - (maxTextW + bw), fontH*2);
      ++val;
      but_resize(info->infos[val].name, w2, y, maxTextW, fontH*2);
      but_resize(info->infos[val].in, w2 + maxTextW + bw, y,
		 rw - (maxTextW + bw), fontH*2);
    }
  }
  y += bw + fontH * 2;
  abutTerm_resize(info->gameComment, bw*2, y, w - bw*4, h - y - bw*2);

  return(0);
}


static ButOut  destroy(ButWin *win)  {
  EditInfo  *info = butWin_packet(win);
  Cgoban  *cg;

  assert(MAGIC(info));
  editInfo_updateSgf(info);
  cg = info->cg;
  assert(MAGIC(cg));
  clp_setDouble(cg->clp, "edit.infoW",
		(double)butWin_w(win) / (double)butWin_getMinW(win));
  clp_setDouble(cg->clp, "edit.infoH",
		(double)butWin_h(win) / (double)butWin_getMinH(win));
  clp_setInt(cg->clp, "edit.infoX", butWin_x(win));
  clp_setInt(cg->clp, "edit.infoY", butWin_y(win));
  info->win = NULL;
  editInfo_destroy(info, TRUE);
  return(0);
}


static ButOut  newTitle(But *but, const char *newStr)  {
  EditInfo  *info = but_packet(but);

  assert(MAGIC(info));
  assert(but == info->infos[editInfo_gameTitle].in);
  editInfo_updateSgf(info);
  butText_set(info->goban->labelText, newStr);
  return(0);
}


/*
 * This function is really badly structured.  I should fix it up.
 */
void  editInfo_updateSgf(EditInfo *info)  {
  EditInfoVal  val;
  const char  *newVal;
  SgfElem  *elLookup;
  int  newIval;
  bool  found, change, err;
  SgfInsert  oldMode;

  newIval = 0;  /* To shut up the warning from gcc. */
  change = FALSE;  /* To shut up the warning from gcc. */
  assert(MAGIC(info));
  editInfoVal_iter(val)  {
    newVal = butTextin_get(info->infos[val].in);
    found = FALSE;
    for (elLookup = &info->sgf->top;
	 (elLookup != NULL) && (elLookup->type != sgfType_node);
	 elLookup = elLookup->activeChild)  {
      if ((elLookup->type == sgfVals[val].type) &&
	  ((sgfVals[val].color == goStone_empty) ||
	   (sgfVals[val].color == elLookup->gVal)))  {
	found = TRUE;
	break;
      }
    }
    err = FALSE;
    if (val == editInfo_komi)  {
      newIval = (int)(wms_atof(newVal, &err) * 2.0);
    } else if (val == editInfo_handicap)  {
      newIval = wms_atoi(newVal, &err);
    }
    if (!found)  {
      if (newVal[0] != '\0')
	change = TRUE;
    } else  {
      if ((val == editInfo_komi) || (val == editInfo_handicap))  {
	change = (newIval != elLookup->iVal);
      } else  {
	change = strcmp(newVal, str_chars(elLookup->sVal));
      }
    }
    if (change)  {
      if (found)  {
	if ((val == editInfo_komi) || (val == editInfo_handicap))  {
	  if (!err)
	    elLookup->iVal = newIval;
	} else  {
	  str_copyChars(elLookup->sVal, newVal);
	}
      } else  {
	elLookup = info->sgf->active;
	info->sgf->active = info->sgf->top.activeChild;
	oldMode = info->sgf->mode;
	info->sgf->mode = sgfInsert_inline;
	if ((val == editInfo_komi) || (val == editInfo_handicap))  {
	  if (!err)
	    sgf_addCIElem(info->sgf, sgfVals[val].type,
			  sgfVals[val].color, newIval);
	} else  {
	  sgf_addCSElem(info->sgf, sgfVals[val].type,
			sgfVals[val].color, newVal);
	}
	info->sgf->mode = oldMode;
	info->sgf->active = elLookup;
      }
    }
  }
  newVal = butTbin_get(info->gameComment->tbin);
  found = FALSE;
  for (elLookup = &info->sgf->top;
       (elLookup != NULL) && (elLookup->type != sgfType_node);
       elLookup = elLookup->activeChild)  {
    if (elLookup->type == sgfType_gameComment)  {
      found = TRUE;
      break;
    }
  }
  if (found)  {
    if (strcmp(newVal, str_chars(elLookup->sVal)))  {
      str_copyChars(elLookup->sVal, newVal);
    }
  } else  {
    if (newVal[0] != '\0')  {
      elLookup = info->sgf->active;
      info->sgf->active = info->sgf->top.activeChild;
      oldMode = info->sgf->mode;
      info->sgf->mode = sgfInsert_inline;
      sgf_addCSElem(info->sgf, sgfType_gameComment, goStone_empty, newVal);
      info->sgf->mode = oldMode;
      info->sgf->active = elLookup;
    }
  }
}
