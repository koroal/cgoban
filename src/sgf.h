/*
 * src/sgf.h, part of Complete Goban (game program)
 * Copyright (C) 1995-1996 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _SGF_H_

/* We need goboard.h for the GoStone type. */
#ifndef  _WMS_STR_H_
#include <wms/str.h>
#endif
#ifndef  _GOBOARD_H_
#include "goBoard.h"
#endif
#ifdef  _SGF_H_
        LEVELIZATION ERROR
#endif
#define  _SGF_H_  1


/**********************************************************************
 * Data types
 **********************************************************************/


typedef enum  {
  sgfInsert_main, sgfInsert_variant, sgfInsert_inline
} SgfInsert;


typedef enum  {
  sgfType_node, sgfType_unknown, sgfType_style,

  sgfType_size, sgfType_rules, sgfType_handicap,
  sgfType_komi, sgfType_time,

  sgfType_copyright,
  sgfType_playerName, sgfType_date, sgfType_place, sgfType_title,
  sgfType_playerRank, sgfType_event, sgfType_source, sgfType_gameComment,
  sgfType_whoseMove,
  sgfType_move, sgfType_pass,
  sgfType_timeLeft, sgfType_stonesLeft,
  sgfType_setBoard, sgfType_territory,

  sgfType_triangle, sgfType_square, sgfType_mark, sgfType_circle,
  sgfType_label,

  sgfType_comment, sgfType_result
} SgfType;


typedef struct SgfElem_struct  {
  SgfType  type;
  GoStone  gVal;
  int  iVal;
  Str  *sVal;
  char  lVal[5];
  struct SgfElem_struct *childH, *childT, *activeChild;
  struct SgfElem_struct *sibling, *parent;
  int  numChildren;
  int  mapX, mapY;  /* Accessed in "sgfMap.h" */

  MAGIC_STRUCT
} SgfElem;


typedef struct Sgf_struct  {
  SgfElem  top;
  SgfElem  *active;
  SgfInsert  mode;
  bool  longLoc;
  int  moveNum;

  MAGIC_STRUCT
} Sgf;


/**********************************************************************
 * Functions
 **********************************************************************/
extern Sgf  *sgf_create(void);
extern void  sgf_destroy(Sgf *mc);

extern void  sgf_addCElem(Sgf *mc, SgfType type, GoStone gVal);
extern void  sgf_addCIElem(Sgf *mc, SgfType type, GoStone c, int i);
extern void  sgf_addCSElem(Sgf *mc, SgfType type, GoStone gVal, const char *s);
extern void  sgf_addCLElem(Sgf *mc, SgfType type, GoStone gVal, const char *l);
extern void  sgf_addCLSElem(Sgf *mc, SgfType type, GoStone gVal, const char *l,
			    const char *s);

#define  sgf_addElem(mc, t)  sgf_addCElem(mc, t, goStone_empty)
#define  sgf_addIElem(mc, t, i)  sgf_addCIElem(mc, t, goStone_empty, i)
#define  sgf_addSElem(mc, t, s)  sgf_addCSElem(mc, t, goStone_empty, s)
#define  sgf_addLElem(mc, t, l)  sgf_addCLElem(mc, t, goStone_empty, l)
#define  sgf_addLSElem(mc, t, l, s)  sgf_addCLSElem(mc, t, goStone_empty, l, s)

#define  sgf_addNode(mc)  sgf_addElem(mc, sgfType_node)
#define  sgf_style(mc, s)  sgf_addSElem(mc, sgfType_style, s)

#define  sgf_setSize(mc, size)  sgf_addIElem(mc, sgfType_size, size)
#define  sgf_setRules(mc, rules)  sgf_addIElem(mc, sgfType_rules, (int)rules)
extern void  sgf_setHandicap(Sgf *sgf, int handicap);
#define  sgf_setKomi(mc, komi)  \
  sgf_addIElem(mc, sgfType_komi, (int)(komi * 2.0))
#define  sgf_setTimeFormat(mc, time)  sgf_addSElem(mc, sgfType_time, time)

#define  sgf_move(mc, color, loc)  sgf_addCLElem(mc, sgfType_move, color, loc)
#define  sgf_pass(mc, color)  sgf_addCElem(mc, sgfType_pass, color)
#define  sgf_timeLeft(mc, c, t)  sgf_addCIElem(mc, sgfType_timeLeft, c, t)
#define  sgf_stonesLeft(mc, c, s)  sgf_addCIElem(mc, sgfType_stonesLeft, c, s)
#define  sgf_addStone(mc, color, loc)  \
  sgf_addCLElem(mc, sgfType_setBoard, color, loc)
#define  sgf_addTerritory(mc, color, loc)  \
  sgf_addCLElem(mc, sgfType_territory, color, loc)
#define  sgf_addTriangle(mc, loc)  sgf_addLElem(mc, sgfType_triangle, loc)
#define  sgf_addCircle(mc, loc)  sgf_addLElem(mc, sgfType_circle, loc)
#define  sgf_addSquare(mc, loc)  sgf_addLElem(mc, sgfType_square, loc)
#define  sgf_label(mc, loc, label)  \
  sgf_addLSElem(mc, sgfType_label, loc, label)
#define  sgf_rmStone(mc, loc)  \
  sgf_addCLElem(mc, sgfType_setBoard, goStone_empty, loc)
#define  sgf_copyright(mc, s)  sgf_addSElem(mc, sgfType_copyright, s)
#define  sgf_setPlayerName(mc, color, name)  \
  sgf_addCSElem(mc, sgfType_playerName, color, name)
#define  sgf_setTitle(mc, title)  sgf_addSElem(mc, sgfType_title, title)
#define  sgf_playerRank(mc, p, r)  sgf_addCSElem(mc, sgfType_playerRank, p, r)
#define  sgf_event(mc, e)  sgf_addSElem(mc, sgfType_event, e)
#define  sgf_source(mc, e)  sgf_addSElem(mc, sgfType_source, e)
#define  sgf_gameComment(mc, e)  sgf_addSElem(mc, sgfType_gameComment, e)
#define  sgf_setWhoseMove(mc, c)  sgf_addCElem(mc, sgfType_whoseMove, c)
#define  sgf_comment(mc, comment)  sgf_addSElem(mc, sgfType_comment, comment)
#define  sgf_result(mc, r)  sgf_addSElem(mc, sgfType_result, r)
#define  sgf_unknown(mc, u)  sgf_addSElem(mc, sgfType_unknown, u)
#define  sgf_place(mc, p)  sgf_addSElem(mc, sgfType_place, p)

extern void  sgfElem_destroyActiveChild(SgfElem *me);
extern void  sgf_setDate(Sgf *mc);
extern void  sgf_setActiveNodeNumber(Sgf *mc, int sgfNum);
extern SgfElem  *sgf_findType(Sgf *mc, SgfType t);
extern SgfElem  *sgf_elemFindType(Sgf *mc, SgfElem *me,
				  SgfType t);
extern SgfElem  *sgfElem_findType(SgfElem *me, SgfType t);
extern SgfElem  *sgfElem_findTypeInNode(SgfElem *me, SgfType t);
extern SgfElem  *sgf_findFirstType(Sgf *mc, SgfType t);
extern SgfElem  *sgf_elemFindFirstType(Sgf *mc, SgfElem *me,
					      SgfType t);
extern SgfElem  *sgfElem_findFirstType(SgfElem *me, SgfType t);
extern void  sgf_catComment(Sgf *sgf, const char *comments);

/*
 * sgfElem_snip leaves the children where the snipped node used to be.
 */
extern void  sgfElem_snip(SgfElem *se, Sgf *sgf);


/*
 * findMove returns how many nodes you had to step over to find the move.
 *   dir = 1 means look ahead, dir = -1 means look back.
 * The return value is always positive, or -1 if it wasn't found.
 */
extern int  sgfElem_findMove(SgfElem *se, const char *move, int dir);

extern void  sgfElem_newString(SgfElem *me, const char *str);
#define sgf_setActiveToEnd(sgf)  \
  while ((sgf)->active->activeChild)  \
    (sgf)->active = (sgf)->active->activeChild
extern void  sgf_addHandicapStones(Sgf *sgf, GoBoard *board);

extern Sgf  *sgf_copy(const Sgf *src);

#endif  /* _SGF_H_ */
