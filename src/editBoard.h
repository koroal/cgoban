/*
 * src/editBoard.h, part of Complete Goban (game program)
 * Copyright (C) 1995-1996 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _EDITBOARD_H_

#ifndef  _CGOBAN_H_
#include "cgoban.h"
#endif
#ifndef  _SGF_H_
#include "sgf.h"
#endif
#ifndef  _GOGAME_H_
#include "goGame.h"
#endif
#ifndef  _GOBAN_H_
#include "goban.h"
#endif
#ifndef  _EDITTOOL_H_
#include "editTool.h"
#endif
#ifndef  _ABUT_FSEL_H_
#include <abut/fsel.h>
#endif
#ifndef  _EDITINFO_H_
#include "editInfo.h"
#endif
#ifdef  _EDITBOARD_H_
  Levelization Error.
#endif
#define  _EDITBOARD_H_  1


/**********************************************************************
 * Data types
 **********************************************************************/
typedef struct EditBoard_struct  {
  Cgoban  *cg;
  Str  fName;
  bool  modified;
  AbutFsel  *fsel;
  AbutMsg  *reallyQuit;
  Sgf  *sgf;
  int  currentNodeNum;
  GoGame  *game;
  Goban  *goban;
  SgfElem  *lastComment;

  EditToolWin  tools;
  EditTool  oldTool;
  But  *shiftKeytrap, *prevVar, *nextVar;
  bool  invertShift;  /* Kludgey.  :-( */
  EditInfo  *info;

  MAGIC_STRUCT
} EditBoard;


/**********************************************************************
 * Functions
 **********************************************************************/
EditBoard  *editBoard_create(Cgoban *cg, const char *fName);
EditBoard  *editBoard_createSgf(Cgoban *cg, const Sgf *sgf);
void  editBoard_destroy(EditBoard *eb);


#endif  /* _EDITBOARD_H_ */
