/*
 * src/sgfMap.h, part of Complete Goban (game program)
 * Copyright (C) 1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _SGFMAP_H_
#define  _SGFMAP_H_  1

#ifndef  _CGBUTS_H_
#include "cgbuts.h"
#endif
#ifndef  _SGF_H_
#include "sgf.h"
#endif


/**********************************************************************
 * Constants
 **********************************************************************/

/**********************************************************************
 * Data types
 **********************************************************************/
typedef enum  {
  sgfMap_next, sgfMap_prev
} SgfMapDirection;

typedef enum  {
  sgfMap_black, sgfMap_white, sgfMap_bPass, sgfMap_wPass,
  sgfMap_edit, sgfMap_none
} SgfMapType;


#define  SGFMAPFLAGS_MARKED  0x01
#define  SGFMAPFLAGS_MAIN    0x02
#define  SGFMAPFLAGS_CONNL   0x04
#define  SGFMAPFLAGS_CONNR   0x08
#define  SGFMAPFLAGS_CONNU   0x10
#define  SGFMAPFLAGS_CONND   0x20
#define  SGFMAPFLAGS_CONNDR  0x40
#define  SGFMAPFLAGS_CONNUL  0x80
#define  SGFMAPFLAGS_CONN   (SGFMAPFLAGS_CONNL | SGFMAPFLAGS_CONNR | \
			     SGFMAPFLAGS_CONNU | SGFMAPFLAGS_CONND | \
			     SGFMAPFLAGS_CONNDR | SGFMAPFLAGS_CONNUL)

typedef struct SgfMapElem_struct  {
  SgfElem  *node;
  SgfMapType  type;
  uint  flags;
  int  moveNum;
} SgfMapElem;
  

typedef struct SgfMap_struct  {
  Cgbuts  *cgbuts;
  ButOut  (*callback)(But *but, int newNodeNum);
  int  mapW, mapH, maxW, maxH;
  int  activeX, activeY;  /* [0,0] = ul node, [1,0] = next node, etc. */
  int  pressX, pressY;  /* [0,0] = ul node, [1,0] = next node, etc. */
  int  activeCtrX, activeCtrY;  /* In pixels. */
  SgfMapElem  *els;

  MAGIC_STRUCT
} SgfMap;


/**********************************************************************
 * Functions
 **********************************************************************/
extern But  *sgfMap_create(Cgbuts *b,
			   ButOut (*callback)(But *but, int newNodeNum),
			   void *packet, ButWin *win, int layer, int flags,
			   Sgf *sgf);
extern void  sgfMap_resize(But *but, int x, int y);
extern void  sgfMap_newActive(But *but, SgfElem *new);
#define  sgfMap_activeCtrX(b)  (((SgfMap *)(b->iPacket))->activeCtrX)
#define  sgfMap_activeCtrY(b)  (((SgfMap *)(b->iPacket))->activeCtrY)
extern bool  sgfMap_changeVar(But *but, SgfMapDirection dir);
extern void  sgfMap_remap(But *but, Sgf *sgf);
extern bool  sgfMap_newNode(But *but, SgfElem *new);
extern void  sgfMap_changeNode(But *but, SgfElem *changed);
extern void  sgfMap_setMapPointer(But *but, SgfElem *activeElem);

#endif  /* _sgfMap_H_ */
