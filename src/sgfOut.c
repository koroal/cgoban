/*
 * $Source: /cvsroot/cgoban1/cgoban1/src/sgfOut.c,v $
 * $Revision: 1.3 $
 * $Author: wmshub $
 * $Date: 2002/05/31 23:40:54 $
 *
 * src/sgfOut.c, part of Complete Goban (game program)
 * Copyright (C) 1995-1996 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#include <ctype.h>
#include <wms.h>
#include <wms/rnd.h>
#include <wms/str.h>
#include "cgoban.h"
#include "sgf.h"
#include "msg.h"
#ifdef  _SGFOUT_H_
        LEVELIZATION ERROR
#endif
#include "sgfOut.h"


/**********************************************************************
 * Forward Declarations
 **********************************************************************/
static void  printColor(Str *out, SgfElem *me);
static void  printPoint(Str *out, Sgf *mc, SgfElem *me);
static bool  writeNode(FILE *f, Sgf *mc, SgfElem *me, int *err,
		       int col, Str *tmp1, Str *tmp2);
static void  printString(Str *out, SgfElem *me);
static SgfElem  *printPoints(Str *out, Sgf *mc, SgfElem *me);
static SgfElem  *printLabelPoints(Str *out, Sgf *mc, SgfElem *me);


/**********************************************************************
 * Functions
 **********************************************************************/
bool sgf_writeFile(Sgf *mc, const char *fname, int *err)  {
  FILE *f;
  SgfElem  *me;
  Str  nodeOut, tmp;

  assert(MAGIC(mc));
  f = fopen(fname, "w");
  if (f == NULL)  {
    if (err)
      *err = errno;
    return(FALSE);
  }
  fprintf(f, "(;GM[1]FF[3]\n");
  str_init(&nodeOut);
  str_init(&tmp);
  if (mc->top.childH == mc->top.childT)  {
    if (!writeNode(f, mc, mc->top.childH, err, 0, &nodeOut, &tmp))  {
      fclose(f);
      str_deinit(&nodeOut);
      str_deinit(&tmp);
      return(FALSE);
    }
  } else  {
    for (me = mc->top.childH;  me;  me = me->sibling)  {
      fprintf(f, "\n(");
      if (!writeNode(f, mc, me, err, 0, &nodeOut, &tmp))  {
	fclose(f);
	str_deinit(&nodeOut);
	str_deinit(&tmp);
	return(FALSE);
      }
      fprintf(f, ")");
    }
  }
  fprintf(f, "\n)\n");
  fclose(f);
  str_deinit(&nodeOut);
  str_deinit(&tmp);
  return(TRUE);
}


static bool  writeNode(FILE *f, Sgf *mc, SgfElem *me, int *err,
		       int col, Str *nodeOut, Str *tmp)  {
  bool  crNeeded;  /* Flag; put this on its own line? */

  while (me)  {
    crNeeded = FALSE;
    switch(me->type)  {
    case sgfType_node:
      str_copyChar(nodeOut, ';');
      break;
    case sgfType_unknown:
      str_copyChars(nodeOut, str_chars(me->sVal));
      break;

    case sgfType_size:
      str_print(nodeOut, "SZ[%d]", me->iVal);
      break;
    case sgfType_rules:
      str_print(nodeOut, "RU[%s]", msg_ruleNames[(int)(me->iVal)]);
      break;
    case sgfType_handicap:
      str_print(nodeOut, "HA[%d]", me->iVal);
      break;
    case sgfType_komi:
      str_print(nodeOut, "KM[%g]", (double)me->iVal / 2.0);
      break;
    case sgfType_time:
      str_print(nodeOut, "TM[%s]", str_chars(me->sVal));
      break;
    case sgfType_copyright:
      str_copyChars(nodeOut, "CP");
      printString(nodeOut, me);
      break;
    case sgfType_playerName:
      crNeeded = TRUE;
      str_copyChar(nodeOut, 'P');
      printColor(nodeOut, me);
      printString(nodeOut, me);
      break;
    case sgfType_title:
      crNeeded = TRUE;
      str_copyChars(nodeOut, "GN");
      printString(nodeOut, me);
      break;
    case sgfType_playerRank:
      str_clip(nodeOut, 0);
      printColor(nodeOut, me);
      str_catChar(nodeOut, 'R');
      printString(nodeOut, me);
      break;
    case sgfType_event:
      crNeeded = TRUE;
      str_copyChars(nodeOut, "EV");
      printString(nodeOut, me);
      break;
    case sgfType_source:
      crNeeded = TRUE;
      str_copyChars(nodeOut, "SO");
      printString(nodeOut, me);
      break;
    case sgfType_gameComment:
      str_copyChars(nodeOut, "GC");
      printString(nodeOut, me);
      break;
    case sgfType_date:
      crNeeded = TRUE;
      str_copyChars(nodeOut, "DT");
      printString(nodeOut, me);
      break;

    case sgfType_whoseMove:
      assert(goStone_isStone(me->gVal));
      str_print(nodeOut, "PL[%c]", (me->gVal == goStone_white) ? 'W' : 'B');
      break;
    case sgfType_move:
      str_clip(nodeOut, 0);
      printColor(nodeOut, me);
      printPoint(nodeOut, mc, me);
      break;
    case sgfType_pass:
      str_clip(nodeOut, 0);
      printColor(nodeOut, me);
      if (mc->longLoc)
	str_catChars(nodeOut, "[]");
      else
	str_catChars(nodeOut, "[tt]");
      break;
    case sgfType_timeLeft:
      str_clip(tmp, 0);
      printColor(tmp, me);
      str_print(nodeOut, "%sL[%d]", str_chars(tmp), me->iVal);
      break;
    case sgfType_stonesLeft:
      str_copyChar(tmp, 'O');
      printColor(tmp, me);
      str_print(nodeOut, "%s[%d]", str_chars(tmp), me->iVal);
      break;

    case sgfType_setBoard:
      str_copyChar(nodeOut, 'A');
      printColor(nodeOut, me);
      me = printPoints(nodeOut, mc, me);
      break;
    case sgfType_territory:
      str_copyChar(nodeOut, 'T');
      printColor(nodeOut, me);
      me = printPoints(nodeOut, mc, me);
      break;

    case sgfType_triangle:
      str_copyChars(nodeOut, "TR");
      me = printPoints(nodeOut, mc, me);
      break;
    case sgfType_circle:
      str_copyChars(nodeOut, "CR");
      me = printPoints(nodeOut, mc, me);
      break;
    case sgfType_square:
      str_copyChars(nodeOut, "SQ");
      me = printPoints(nodeOut, mc, me);
      break;
    case sgfType_mark:
      str_copyChars(nodeOut, "MA");
      me = printPoints(nodeOut, mc, me);
      break;
    case sgfType_label:
      str_copyChars(nodeOut, "LB");
      me = printLabelPoints(nodeOut, mc, me);
      break;

    case sgfType_comment:
      str_copyChars(nodeOut, "C");
      printString(nodeOut, me);
      break;
    case sgfType_result:
      crNeeded = TRUE;
      str_copyChars(nodeOut, "RE");
      printString(nodeOut, me);
      break;

    case sgfType_place:
      str_copyChars(nodeOut, "PC");
      printString(nodeOut, me);
      break;
    case sgfType_style:
      str_copyChars(nodeOut, "SY");
      printString(nodeOut, me);
      break;

   default:
      fprintf(stderr, "BOGUS PROPERTY TO PRINT!\n");
      for (;;);
      break;
    }
    if (col && (crNeeded || (col + str_len(nodeOut) > 70)))  {
      fprintf(f, "\n");
      col = 0;
    }
    col += str_len(nodeOut);
    fprintf(f, "%s", str_chars(nodeOut));
    if (crNeeded)  {
      fprintf(f, "\n");
      col = 0;
    }
    if (me->childH == me->childT)  {
      me = me->childH;
    } else  {
      for (me = me->childH;  me;  me = me->sibling)  {
	fprintf(f, "\n(");
	if (!writeNode(f, mc, me, err, 1, nodeOut, tmp))
	  return(FALSE);
	fprintf(f, ")\n");
	col = 0;
      }
    }
  }
  return(TRUE);
}


static void  printColor(Str *out, SgfElem *me)  {
  if (me->gVal == goStone_white)
    str_catChar(out, 'W');
  else if (me->gVal == goStone_black)
    str_catChar(out, 'B');
  else  {
    assert(me->gVal == goStone_empty);
    str_catChar(out, 'E');
  }
}


static void  printPoint(Str *out, Sgf *mc, SgfElem *me)  {
  if (mc->longLoc)  {
    str_catChar(out, '[');
    str_catChar(out, me->lVal[0]);
    str_catChar(out, me->lVal[1]);
    str_catChar(out, me->lVal[2]);
    str_catChar(out, me->lVal[3]);
    str_catChar(out, ']');
  } else  {
    str_catChar(out, '[');
    str_catChar(out, me->lVal[1]);
    str_catChar(out, me->lVal[3]);
    str_catChar(out, ']');
  }
}


static SgfElem  *printPoints(Str *out, Sgf *mc, SgfElem *me)  {
  printPoint(out, mc, me);
  while ((me->childH != NULL) &&
	 (me->childH == me->childT) &&
	 (me->childH->type == me->type) &&
	 (me->childH->gVal == me->gVal))  {
    me = me->childH;
    printPoint(out, mc, me);
  }
  return(me);
}


static void  printLabelPoint(Str *out, Sgf *mc, SgfElem *me)  {
  if (mc->longLoc)  {
    str_catChar(out, '[');
    str_catChar(out, me->lVal[0]);
    str_catChar(out, me->lVal[1]);
    str_catChar(out, me->lVal[2]);
    str_catChar(out, me->lVal[3]);
    str_catChar(out, ':');
    str_cat(out, me->sVal);
    str_catChar(out, ']');
  } else  {
    str_catChar(out, '[');
    str_catChar(out, me->lVal[1]);
    str_catChar(out, me->lVal[3]);
    str_catChar(out, ':');
    str_cat(out, me->sVal);
    str_catChar(out, ']');
  }
}


static SgfElem  *printLabelPoints(Str *out, Sgf *mc, SgfElem *me)  {
  printLabelPoint(out, mc, me);
  while (me->childH &&
	 (me->childH == me->childT) &&
	 (me->childH->type == me->type) &&
	 (me->childH->gVal == me->gVal))  {
    me = me->childH;
    printLabelPoint(out, mc, me);
  }
  return(me);
}


static void  printString(Str *out, SgfElem *me)  {
  const char  *str = str_chars(me->sVal);

  str_catChar(out, '[');
  while (*str)  {
    if ((*str == ']') || (*str == '[') || (*str == '\\'))
      str_catChar(out, '\\');
    str_catChar(out, *str);
    ++str;
  }
  str_catChar(out, ']');
}
