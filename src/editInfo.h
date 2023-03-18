 /*
 * src/editInfo.h, part of Complete Goban (game program)
 * Copyright (C) 1996 William Shubert.
 * See "configure.h.in" for more copyright information.
 */

#ifndef  _EDITINFO_H_

#ifndef  _CGOBAN_H_
#include "cgoban.h"
#endif
#ifndef  _ABUT_TERM_H_
#include <abut/term.h>
#endif
#ifndef  _GOBAN_H_
#include "goban.h"
#endif
#ifdef  _EDITINFO_H_
  Levelization Error.
#endif
#define  _EDITINFO_H_  1


/**********************************************************************
 * Data Types
 **********************************************************************/
typedef enum  {
  editInfo_gameTitle, editInfo_copyright,
  editInfo_wName,     editInfo_bName,
  editInfo_wRank,     editInfo_bRank,
  editInfo_handicap,  editInfo_komi,
  editInfo_time,      editInfo_result,
  editInfo_date,      editInfo_place,
  editInfo_event,     editInfo_source,
  editInfo_count
} EditInfoVal;


typedef struct EditInfo_struct  {
  Cgoban  *cg;
  Goban  *goban;
  ButWin  *win;
  But  *bg;
  Sgf  *sgf;

  But  *winTitle;
  struct  {
    But  *name, *in;
  } infos[editInfo_count];
  AbutTerm  *gameComment;

  void  (*destroy)(struct EditInfo_struct *info, void *packet);
  void  *packet;

  MAGIC_STRUCT
} EditInfo;


/**********************************************************************
 * Functions
 **********************************************************************/
#define  editInfoVal_iter(v)  \
  for ((v) = editInfo_gameTitle;  (v) < editInfo_count;  ++(v))
extern EditInfo  *editInfo_create(Cgoban *cg, Goban *goban, Sgf *sgf,
				  void (*dCallback)(EditInfo *info,
						    void *packet),
				  void *packet);
extern void  editInfo_destroy(EditInfo *info, bool propagate);
extern void  editInfo_updateSgf(EditInfo *info);

#endif
