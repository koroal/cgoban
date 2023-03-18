/*
 * $Source: /cvsroot/cgoban1/cgoban1/src/editTool.h,v $
 * $Revision: 1.2 $
 * $Date: 2000/02/09 06:50:02 $
 *
 * src/editTool.h, part of Complete Goban (game program)
 * Copyright © 1995,2000 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _EDITTOOL_H_
#define  _EDITTOOL_H_  1

#ifndef  _CGOBAN_H_
#include "cgoban.h"
#endif
#ifndef  _SGF_H_
#include "sgf.h"
#endif


/**********************************************************************
 * Data types
 **********************************************************************/
typedef enum  {
  editTool_play, editTool_changeBoard, editTool_score,
  editTool_triangle, editTool_square, editTool_circle,
  editTool_letter, editTool_number
} EditTool;
#define editTool_min editTool_play
#define editTool_max (editTool_number + 1)


typedef struct EditToolWin_struct  {
  Cgoban  *cg;
  bool  modified;

  ButWin  *toolWin, *toolIWin;
  AbutSwin  *mapWin;
  But  *toolBg, *toolIBg, *toolIPic;

  But  *toolSel, *selDesc[editTool_max];
  But  *toolBox, *toolName, *toolDesc1, *toolDesc2;
  But  *help, *killNode, *moveNode;

  But  *mapBg, *sgfMap, *prevVar, *nextVar;

  EditTool  tool;
  GoStone  lastColor;  /* Color of the editTool_play icon. */
  Sgf  *sgf;

  void  (*quitRequested)(void *packet);
  ButOut  (*newToolCallback)(void *packet);
  ButOut  (*mapCallback)(void *packet, int nodeNum);
  void  *packet;

  MAGIC_STRUCT
} EditToolWin;


/**********************************************************************
 * Functions
 **********************************************************************/
extern void  editToolWin_init(EditToolWin *etw, Cgoban *cg, Sgf *sgf,
			      void (*quitRequested)(void *packet),
			      ButOut (*newToolCallback)(void *packet),
			      ButOut (*newActiveNode)(void *packet,
						      int nodeNum),
			      void *packet);
extern void  editToolWin_deinit(EditToolWin *etw);
extern void  editToolWin_newColor(EditToolWin *etw, GoStone color);
extern void  editToolWin_newTool(EditToolWin *etw, EditTool tool,
				 bool propagate);
extern void  editToolWin_newActiveNode(EditToolWin *etw, SgfElem *newNode);
extern void  editToolWin_nodeAdded(EditToolWin *etw, SgfElem *newNode);
extern ButOut  editToolWin_shiftUpPressed(But *but, bool press);
extern ButOut  editToolWin_shiftDownPressed(But *but, bool press);

#endif  /* _EDITTOOL_H_ */
