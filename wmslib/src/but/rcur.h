/*
 * wmslib/include/but/rcur.h, part of wmslib (Library functions)
 * Copyright (C) 1994-1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 *
 * Includes for remote cursors.
 */

#ifndef  _BUTRCUR_H_
#define  _BUTRCUR_H_  1

/**********************************************************************
 * Constants
 **********************************************************************/

#define  BUTCUR_NUM  10

/**********************************************************************
 * Types
 **********************************************************************/

/*
 * Do not use butCur_bogus.  It is a bogus cursor, _NOT_ a bogus-looking
 *   cursor.
 */
typedef enum  {
  butCur_idle, butCur_text, butCur_twitch, butCur_right, butCur_left,
  butCur_up, butCur_down, butCur_lr, butCur_ud, butCur_grab, butCur_bogus}
ButCur;

typedef struct ButRcur_struct  {
  ButEnv  *env;
  int  winId, rx,ry, rw,rh;
  int  lx,ly;
  ButCur  type;
  bool  drawn;
  Pixmap  under;
  MAGIC_STRUCT
} ButRcur;

/**********************************************************************
 * Functions
 **********************************************************************/

/* Public. */
extern void  butEnv_rcInit(ButEnv *env);

extern void  butRcur_create(ButRcur *rc, ButEnv *env);
#define  butRcur_destroy(rc) butRcur_move(-2, 0,0, 0,0);

extern void  butRcur_move(ButRcur *rc, int winId, int rx,int ry,
			  int rw,int rh, ButCur type);
extern void  butRcur_redraw(ButEnv *env, int winId, int x,int y, int w,int h);

/* The type of cursor has changed. */
extern void  butEnv_setCursor(ButEnv *env, But *but, ButCur newCursor);

/* Set up any cursor changes that may have occurred. */
extern void  butEnv_rcActivate(ButEnv *env);

#endif  /* _BUTRCUR_H_ */
