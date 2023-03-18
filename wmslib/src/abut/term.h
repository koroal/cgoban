/*
 * wmslib/src/abut/term.h, part of wmslib (Library functions)
 * Copyright (C) 1995 William Shubert
 * See "configure.h.in" for more copyright information.
 *
 * Includes for "term" scrolling terminal-like widgety thing.
 */

#ifndef  _ABUT_TERM_H_
#define  _ABUT_TERM_H_  1

#ifndef  _ABUT_ABUT_H_
#include  <abut/abut.h>
#endif
#ifndef  _ABUT_SWIN_H_
#include <abut/swin.h>
#endif
#ifndef  _BUT_TBIN_H_
#include <but/tbin.h>
#endif


/**********************************************************************
 * Data types
 **********************************************************************/
typedef enum  {
  abutTermState_fastUp, abutTermState_slowUp, abutTermState_steady,
  abutTermState_slowDown, abutTermState_fastDown
} AbutTermState;


typedef struct  AbutTerm_struct  {
  Abut  *abut;
  AbutSwin  *swin;
  But  *bg;
  But  *tbin;
  AbutTermState state;
  MAGIC_STRUCT
} AbutTerm;


/**********************************************************************
 * Functions
 **********************************************************************/
extern AbutTerm  *abutTerm_create(Abut *abut, ButWin *parent,
				  int layer, bool editable);
extern void  abutTerm_destroy(AbutTerm *term);
extern void  abutTerm_resize(AbutTerm *term, int x, int y, int w, int h);
extern void  abutTerm_set(AbutTerm *term, const char *text);
#define  abutTerm_get(term)  butTbin_get((term)->tbin)
extern void  abutTerm_append(AbutTerm *term, const char *appText);

#endif  /* _ABUT_TERM_H_ */
