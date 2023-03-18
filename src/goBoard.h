/*
 * src/goboard.h, part of Complete Goban (game program)
 * Copyright (C) 1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _GOBOARD_H_
#define  _GOBOARD_H_  1

#ifndef  _GOHASH_H_
#include "goHash.h"
#endif


/**********************************************************************
 * Constants
 **********************************************************************/
#define  GOBOARD_MAXSIZE  38

/**********************************************************************
 * Data types
 **********************************************************************/
typedef enum  {
  goStone_white, goStone_black, goStone_edge, goStone_empty
} GoStone;


typedef struct GoBoardPiece_struct  {
  GoStone  type;
  int  libs, size;
  struct GoBoardPiece_struct  *head, *tail, *next;
  GoHash  hashStone[goStone_black+1], hashKo[goStone_black+1];
} GoBoardPiece;


typedef struct  {
  int  area, dirs[4];
  GoBoardPiece  *pieces;
  int  caps[2];
  GoHash  hashVal;
  int  koLoc;

  MAGIC_STRUCT
} GoBoard;


/**********************************************************************
 * Iterators
 **********************************************************************/
#define  goStoneIter(s)  for ((s) = goStone_white; \
                              (s) <= goStone_black;  \
                              ++(s))

typedef GoBoardPiece  *GoBoardGroupIter;
#define  goBoardGroupIter(i, b, l)  for ((i) = ((b)->pieces[loc].head);  \
					 (i);  \
					 (i) = (i)->next)
#define  goBoardGroupIter_loc(i, b)   ((i) - &(b)->pieces[0])


/**********************************************************************
 * Functions
 **********************************************************************/
#define  goStone_isStone(t)  ((t) <= goStone_black)
#define  goStone_char(t)  ("WB#."[t])
#define  goStone_opponent(s)  (goStone_white + goStone_black - (s))

extern GoBoard  *goBoard_create(int size);
extern void  goBoard_destroy(GoBoard *board);
/*
 * For goBoard_copy, the sizes of src and dest must be the same.
 */
extern void  goBoard_copy(GoBoard *src, GoBoard *dest);
extern bool  goBoard_eq(GoBoard *b1, GoBoard *b2);
extern void  goBoard_print(GoBoard *b);
extern void  goBoard_fprint(GoBoard *b, FILE *fnum);

#define  goBoard_xy2Loc(b,x,y)  ((x)+(y)*(b)->dirs[0]+(b)->dirs[0]+1)
extern void  goBoard_loc2Str(GoBoard *board, int loc, char *str);
extern int  goBoard_str2Loc(GoBoard *board, const char *str);
#define  goBoard_loc2X(b,l)  (((l)%(b)->dirs[0])-1)
#define  goBoard_loc2Y(b,l)  (((l)/(b)->dirs[0])-1)
extern const char  *goBoard_loc2Sgf(GoBoard *b, int l);
extern int  goBoard_sgf2Loc(GoBoard *b, const char *l);

#define  goBoard_size(b)  ((b)->dirs[0] - 1)
#define  goBoard_width(b)  ((b)->dirs[0])
#define  goBoard_area(b)  ((b)->area)
#define  goBoard_minLoc(b)  ((b)->dirs[0])
#define  goBoard_maxLoc(b)  ((b)->area - (b)->dirs[0])
#define  goBoard_dir(b, d)  ((b)->dirs[d])

#define  goBoard_stone(b, l)  ((b)->pieces[l].type)
#define  goBoard_koLoc(b)  ((b)->koLoc)
#define  goBoard_hash(b)  ((b)->hashVal)
#define  goBoard_hashNoKo(b, s)  goHash_xor((b)->hashVal,  \
					    (b)->pieces[(b)->koLoc].hashKo[s])
#define  goBoard_caps(b, p)  ((b)->caps[p])
#define  goBoard_addCaps(b, p, s)  (((b)->caps[p]) += s)
#define  goBoard_liberties(b, l)  ((b)->pieces[l].head->libs)
#define  goBoard_groupSize(b, l)  ((b)->pieces[l].head->size)
#define  goBoard_groupEq(b, l1, l2)  ((b)->pieces[l1].head == \
				      (b)->pieces[l2].head)

/* goBoard_addStone(...) returns the number of stones captured. */
extern int  goBoard_addStone(GoBoard *board, GoStone stone, int loc,
			     int *suicides);
extern void  goBoard_rmGroup(GoBoard *board, int loc);
/* quickHash() always returns the hashNoKo value. */
extern GoHash  goBoard_quickHash(GoBoard *board, GoStone stone, int loc,
				 bool *suicide);

#endif  /* _GOBOARD_H_ */
