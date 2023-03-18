/*
 * src/sgf.c, part of Complete Goban (game program)
 * Copyright (C) 1995-1996 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#include <ctype.h>
#include <wms.h>
#include <wms/rnd.h>
#include <wms/str.h>
#include <but/but.h>
#include "cgoban.h"
#include "msg.h"
#ifdef  _SGF_H_
        LEVELIZATION ERROR
#endif
#include "sgf.h"


/**********************************************************************
 * Constants
 **********************************************************************/
#define  POOLSIZE  100


/**********************************************************************
 * Data Types
 **********************************************************************/
typedef struct ElemPool_struct  {
  struct ElemPool_struct  *next;
  SgfElem  elems[POOLSIZE];

  MAGIC_STRUCT
} ElemPool;


/**********************************************************************
 * Globals
 **********************************************************************/
static ElemPool  *elemPoolList = NULL;
static SgfElem  *freeElem = NULL;


/**********************************************************************
 * Forward Declarations
 **********************************************************************/
static SgfElem  *sgfElem_create(Sgf *mc, SgfType type);
static void  sgfElem_destroy(SgfElem *me);
static void  sgfElem_copy(Sgf *destSgf, SgfElem *dest, const SgfElem *src);


/**********************************************************************
 * Functions
 **********************************************************************/
Sgf  *sgf_create(void)  {
  Sgf  *mc;

  mc = wms_malloc(sizeof(Sgf));
  MAGIC_SET(mc);
  MAGIC_SET(&(mc->top));
  mc->longLoc = FALSE;
  mc->mode = sgfInsert_main;
  mc->top.type = sgfType_unknown;
  mc->top.childH = NULL;
  mc->top.childT = NULL;
  mc->top.activeChild = NULL;
  mc->top.sibling = NULL;
  mc->top.parent = NULL;
  mc->top.numChildren = 0;
  mc->top.mapX = -1;
  mc->top.mapY = -1;
  mc->active = &(mc->top);
  mc->moveNum = 0;
  return(mc);
}


void  sgf_destroy(Sgf *mc)  {
  SgfElem  *me, *nextMe;

  assert(MAGIC(mc));
  me = mc->top.childH;
  while (me)  {
    assert(MAGIC(me));
    nextMe = me->sibling;
    sgfElem_destroy(me);
    me = nextMe;
  }
  MAGIC_UNSET(mc);
  wms_free(mc);
}


void  sgf_addCElem(Sgf *mc, SgfType type, GoStone gVal)  {
  SgfElem  *me;

  assert(MAGIC(mc));
  me = sgfElem_create(mc, type);
  assert(MAGIC(me));
  me->gVal = gVal;
}


void  sgf_addCIElem(Sgf *mc, SgfType type,
			  GoStone c, int i)  {
  sgf_addCElem(mc, type, c);
  mc->active->iVal = i;
  if ((type == sgfType_size) && (i > 19))
    mc->longLoc = TRUE;
}


void  sgf_addCSElem(Sgf *mc, SgfType type,
			  GoStone c, const char *s)  {
  SgfElem  *se;

  if (type == sgfType_comment)  {
    se = sgfElem_findTypeInNode(mc->active, sgfType_comment);
    if (se)  {
      str_copyChars(se->sVal, s);
      return;
    }
  }
  sgf_addCElem(mc, type, c);
  mc->active->sVal = str_createChars(s);
}


void  sgf_catComment(Sgf *sgf, const char *s)  {
  SgfElem  *se;

  se = sgfElem_findTypeInNode(sgf->active, sgfType_comment);
  if (se)  {
    str_catChars(se->sVal, s);
  } else  {
    sgf_addCElem(sgf, sgfType_comment, goStone_empty);
    sgf->active->sVal = str_createChars(s);
  }
}


void  sgf_addCLElem(Sgf *mc, SgfType type,
		    GoStone c, const char *loc)  {
  sgf_addCElem(mc, type, c);
  assert((strlen(loc) == 4) || (strlen(loc) == 2));
  strcpy(mc->active->lVal, loc);
}


void  sgf_addCLSElem(Sgf *mc, SgfType type,
		     GoStone c, const char *loc, const char *str)  {
  sgf_addCElem(mc, type, c);
  assert((strlen(loc) == 4) || (strlen(loc) == 2));
  strcpy(mc->active->lVal, loc);
  mc->active->sVal = str_createChars(str);
}


static SgfElem  *sgfElem_create(Sgf *mc, SgfType type)  {
  ElemPool  *newPool;
  int  i;
  SgfElem  *result, *parent, *childOfResult;

  if (freeElem == NULL)  {
    newPool = wms_malloc(sizeof(ElemPool));
    MAGIC_SET(newPool);
    newPool->next = elemPoolList;
    elemPoolList = newPool;
    for (i = 0;  i < POOLSIZE;  ++i)  {
      newPool->elems[i].childH = &(newPool->elems[i+1]);
      newPool->elems[i].sVal = NULL;
    }
    newPool->elems[POOLSIZE - 1].childH = NULL;
    freeElem = &newPool->elems[0];
  }
  result = freeElem;
  freeElem = freeElem->childH;
  result->type = type;
  result->childH = NULL;
  result->childT = NULL;
  result->sibling = NULL;
  result->activeChild = NULL;
  result->numChildren = 0;
  MAGIC_SET(result);
  parent = mc->active;
  result->mapX = parent->mapX;
  result->mapY = parent->mapY;
  result->parent = parent;
#if  DEBUG
  if (mc->mode != sgfInsert_inline)  {
    if (type == sgfType_node)  {
      assert((parent->childH == NULL) ||
	     (parent->childH->type == sgfType_node));
    } else
      assert(parent->childH == NULL);
  }
#endif  /* DEBUG */
  switch(mc->mode)  {
  case sgfInsert_main:
    result->sibling = parent->childH;
    parent->childH = result;
    if (result->sibling == NULL)
      parent->childT = result;
    break;
  case sgfInsert_variant:
    if (parent->childH == NULL)
      parent->childH = result;
    else
      parent->childT->sibling = result;
    parent->childT = result;
    break;
  case sgfInsert_inline:
    result->childH = parent->childH;
    result->childT = parent->childT;
    result->activeChild = parent->activeChild;
    result->sibling = NULL;
    parent->childH = parent->childT = result;
    for (childOfResult = result->childH;
	 childOfResult;
	 childOfResult = childOfResult->sibling)  {
      childOfResult->parent = result;
    }
    break;
  }
  parent->activeChild = result;
  assert((parent->childH != parent->childT) ||
	 (parent->activeChild == parent->childH));
  mc->active = result;
  return(result);
}


static void  sgfElem_destroy(SgfElem *me)  {
  SgfElem  *child, *nextChild;

  assert(MAGIC(me));
  child = me->childH;
  while (child)  {
    assert(MAGIC(child));
    nextChild = child->sibling;
    sgfElem_destroy(child);
    child = nextChild;
  }
  if (me->sVal)  {
    str_destroy(me->sVal);
    me->sVal = NULL;
  }
  MAGIC_UNSET(me);
  me->childH = freeElem;
  freeElem = me;
}


void  sgfElem_destroyActiveChild(SgfElem *parent)  {
  SgfElem  *prevChild;
  SgfElem  *deadChild = parent->activeChild;

  assert(MAGIC(parent));
  assert(MAGIC(deadChild));
  if (parent->childH == deadChild)  {
    prevChild = NULL;
    parent->childH = deadChild->sibling;
  } else  {
    for (prevChild = parent->childH;
	 prevChild->sibling != deadChild;
	 prevChild = prevChild->sibling);
    prevChild->sibling = deadChild->sibling;
  }
  if (parent->childT == deadChild)
    parent->childT = prevChild;
  sgfElem_destroy(deadChild);
  parent->activeChild = parent->childH;
}


void  sgf_setDate(Sgf *mc)  {
  time_t  curTime;
  struct tm  *tmTime;
  char  dateStr[20];

  time(&curTime);
  tmTime = localtime(&curTime);
  sprintf(dateStr, "%04d-%02d-%02d",
	  tmTime->tm_year + 1900,
	  tmTime->tm_mon + 1,
	  tmTime->tm_mday);
  sgf_addCSElem(mc, sgfType_date, goStone_empty, dateStr);
}
  

/*
 * 0 = Before the first move.
 * 1 = The first move.
 * etc.
 */
void  sgf_setActiveNodeNumber(Sgf *mc, int moveNum)  {
  int  curMove = 0;
  SgfElem  *me;

  assert(MAGIC(mc));
  me = &mc->top;
  for (;;)  {
    assert(MAGIC(me));
    if (me->activeChild == NULL)  {
      assert(curMove == moveNum);
      break;
    }
    if (me->activeChild->type == sgfType_node)  {
      ++curMove;
      if (curMove > moveNum)
	break;
    }
    me = me->activeChild;
  }
  mc->active = me;
}


SgfElem  *sgf_findType(Sgf *mc, SgfType t)  {
  SgfElem  *me;

  assert(MAGIC(mc));
  if (mc->active == &mc->top)
    return(NULL);
  for (me = mc->active;  me;  me = me->parent)  {
    assert(MAGIC(me));
    if (me->type == t)
      return(me);
  }
  return(NULL);
}


SgfElem  *sgfElem_findTypeInNode(SgfElem *me, SgfType t)  {
  assert(MAGICNULL(me));
  while (me && (me->type != sgfType_node))  {
    assert(MAGIC(me));
    if (me->type == t)
      return(me);
    me = me->parent;
  }
  return(NULL);
}


SgfElem  *sgf_elemFindType(Sgf *mc, SgfElem *me,
				  SgfType t)  {
  SgfElem  *result = NULL;

  assert(MAGIC(me));
  if (me == mc->active)
    return(NULL);
  for (me = me->activeChild;  (me != NULL) && (me != mc->active);
       me = me->activeChild)  {
    assert(MAGIC(me));
    if (me->type == t)
      result = me;
  }
  if ((me != NULL) && (me->type == t))
    result = me;
  return(result);
}


SgfElem  *sgfElem_findType(SgfElem *me, SgfType t)  {
  SgfElem  *result = NULL;

  assert(MAGIC(me));
  for (me = me->activeChild;  me != NULL;  me = me->activeChild)  {
    assert(MAGIC(me));
    if (me->type == t)
      result = me;
  }
  return(result);
}


SgfElem  *sgf_findFirstType(Sgf *mc, SgfType t)  {
  SgfElem  *me;

  assert(MAGIC(mc));
  if (mc->active == &mc->top)
    return(NULL);
  if (mc->active->type == t)
    return(mc->active);
  for (me = mc->top.activeChild;  me != mc->active;  me = me->activeChild)  {
    assert(MAGIC(me));
    if (me->type == t)
      return(me);
  }
  return(NULL);
}


SgfElem  *sgf_elemFindFirstType(Sgf *mc, SgfElem *me,
				       SgfType t)  {
  assert(MAGIC(me));
  if (me == mc->active)
    return(NULL);
  for (me = me->activeChild;  (me != NULL) && (me != mc->active);
       me = me->activeChild)  {
    assert(MAGIC(me));
    if (me->type == t)
      return(me);
  }
  if ((me != NULL) && (me->type == t))
    return(me);
  return(NULL);
}


SgfElem  *sgfElem_findFirstType(SgfElem *me, SgfType t)  {
  assert(MAGIC(me));
  for (me = me->activeChild;  me != NULL;  me = me->activeChild)  {
    assert(MAGIC(me));
    if (me->type == t)
      return(me);
  }
  return(NULL);
}


void  sgfElem_newString(SgfElem *me, const char *str)  {
  assert(MAGIC(me));
  str_copyChars(me->sVal, str);
}


void  sgfElem_snip(SgfElem *se, Sgf *sgf)  {
  SgfElem  *parent, *child, *sibling;

  if (sgf->active == se)
    sgf->active = se->parent;
  parent = se->parent;
  if (parent->activeChild == se)
    parent->activeChild = se->activeChild;
  if (parent->childH == se)  {
    if (se->sibling)  {
      if (se->childH)  {
	se->childT->sibling = se->sibling;
	se->childT = parent->childT;
      } else  {
	se->childH = se->sibling;
	se->childT = parent->childT;
      }
    }
    if (parent->childT == se)  {
      parent->childH = se->childH;
      parent->childT = se->childT;
    }
  } else  {
    for (sibling = parent->childH;  sibling->sibling != se;
	 sibling = sibling->sibling);
    if (se->childH)  {
      sibling->sibling = se->childH;
      se->childT->sibling = se->sibling;
    } else  {
      sibling->sibling = se->sibling;
    }
  }
  for (child = se->childH;  child;  child = child->sibling)  {
    child->parent = parent;
  }
  se->childH = se->childT = NULL;
  sgfElem_destroy(se);
}


int  sgfElem_findMove(SgfElem *se, const char *move, int dir)  {
  int  steps;

  assert(MAGIC(se));
  assert((dir == 1) || (dir == -1));
  if (dir == 1)
    steps = 0;
  else
    steps = -1;
  for (;;)  {
    if (dir == 1)  {
      se = se->activeChild;
    } else  {
      if (steps >= 0)
	se = se->parent;
    }
    if (se == NULL)
      return(-1);
    switch(se->type)  {
    case sgfType_node:
      ++steps;
      break;
    case sgfType_setBoard:
      if (se->gVal == goStone_empty)
	break;
      /* Fall through to sgfType_move. */
    case sgfType_move:
      if (!strcmp(move, se->lVal))  {
	if (steps < 0)
	  steps = 0;
	return(steps);
      }
      break;
    default:
      break;
    }
    if (steps < 0)
      steps = 0;
  }
}


void  sgf_addHandicapStones(Sgf *sgf, GoBoard *board)  {
  int  i;

  for (i = 0;  i < goBoard_area(board);  ++i)  {
    if (goBoard_stone(board, i) == goStone_black)  {
      sgf_addStone(sgf, goStone_black, goBoard_loc2Sgf(board, i));
    }
  }
}


void  sgf_setHandicap(Sgf *sgf, int handicap)  {
  if (handicap == 1)
    handicap = 0;
  sgf_addIElem(sgf, sgfType_handicap, handicap);
}


Sgf  *sgf_copy(const Sgf *src)  {
  Sgf  *dest;

  dest = wms_malloc(sizeof(Sgf));
  *dest = *src;
  dest->mode = sgfInsert_variant;
  dest->top.childH = dest->top.childT = dest->top.activeChild = NULL;
  sgfElem_copy(dest, &dest->top, &src->top);
  dest->active = &dest->top;
  while (dest->active->activeChild)  {
    dest->active = dest->active->childH;
    assert(dest->active != NULL);
  }
  return(dest);
}


static void  sgfElem_copy(Sgf *destSgf, SgfElem *dest, const SgfElem *src)  {
  SgfElem  *srcChild, *destChild;
  int  i;

  for (srcChild = src->childH;  srcChild;  srcChild = srcChild->sibling)  {
    destSgf->active = dest;
    sgfElem_create(destSgf, srcChild->type);
    destChild = destSgf->active;
    destChild->gVal = srcChild->gVal;
    destChild->iVal = srcChild->iVal;
    assert(destChild->sVal == NULL);
    if (srcChild->sVal != NULL)  {
      destChild->sVal = str_createStr(srcChild->sVal);
    }
    for (i = 0;  i < 5;  ++i)  {
      destChild->lVal[i] = srcChild->lVal[i];
    }
    sgfElem_copy(destSgf, destChild, srcChild);
  }
  dest->activeChild = dest->childH;
}
