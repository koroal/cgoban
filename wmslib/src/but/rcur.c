/*
 * wmslib/src/but/rcur.c, part of wmslib (Library functions)
 * Copyright (C) 1994 William Shubert.
 * See "configure.h.in" for more copyright information.
 */

#include <configure.h>

#ifdef  X11_DISP

#ifdef  STDC_HEADERS
#include <stdlib.h>
#include <unistd.h>
#endif  /* STDC_HEADERS */
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <sys/time.h>
#ifdef  HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#include <wms.h>
#include <but/but.h>
#include <but/net.h>
#include <but/canvas.h>


typedef struct Curs_struct  {
  int  fontcurnum;
  int  hotX, hotY, w, h;
  char  *pic, *mask;
} Curs;


#define ntm_width 16
#define ntm_height 16
#define ntm_bits ((char *)ntm_ubits)
static uchar ntm_ubits[] = {
  0x03, 0x00, 0x07, 0x00, 0x0f, 0x00, 0x1f, 0x00, 0x3f, 0x00, 0x7f, 0x00,
  0xff, 0x00, 0xff, 0x01, 0xff, 0x03, 0xff, 0x03, 0x7f, 0x00, 0xf7, 0x00,
  0xf3, 0x00, 0xe0, 0x01, 0xe0, 0x01, 0xc0, 0x01};

#define ntp_width 16
#define ntp_height 16
#define ntp_bits ((char *)ntp_ubits)
static uchar ntp_ubits[] = {
  0x00, 0x00, 0x02, 0x00, 0x06, 0x00, 0x0e, 0x00, 0x1e, 0x00, 0x3e, 0x00,
  0x7e, 0x00, 0xfe, 0x00, 0xfe, 0x01, 0x3e, 0x00, 0x36, 0x00, 0x62, 0x00,
  0x60, 0x00, 0xc0, 0x00, 0xc0, 0x00, 0x00, 0x00};

#define tm_width 16
#define tm_height 16
#define tm_bits ((char *)tm_ubits)
static uchar tm_ubits[] = {
  0x00, 0xe0, 0x00, 0xf8, 0x00, 0xfe, 0x80, 0x7f, 0xe0, 0x7f, 0xf8, 0x3f,
  0xfc, 0x3f, 0xf8, 0x1f, 0xe0, 0x1f, 0xf0, 0x0f, 0xf8, 0x0f, 0x7c, 0x07,
  0x3e, 0x07, 0x1f, 0x02, 0x0e, 0x00, 0x04, 0x00};

#define tp_width 16
#define tp_height 16
#define tp_bits ((char *)tp_ubits)
static uchar tp_ubits[] = {
  0x00, 0x00, 0x00, 0x60, 0x00, 0x78, 0x00, 0x3e, 0x80, 0x3f, 0xe0, 0x1f,
  0xf8, 0x1f, 0x80, 0x0f, 0xc0, 0x0f, 0xe0, 0x06, 0x70, 0x06, 0x38, 0x02,
  0x1c, 0x02, 0x0e, 0x00, 0x04, 0x00, 0x00, 0x00};

#define txtp_width 16
#define txtp_height 16
#define txtp_bits ((char *)txtp_ubits)
static uchar txtp_ubits[] = {
   0x00, 0x00, 0xee, 0x00, 0x38, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00,
   0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00,
   0x10, 0x00, 0x38, 0x00, 0xee, 0x00, 0x00, 0x00};

#define txtm_width 16
#define txtm_height 16
#define txtm_bits ((char *)txtm_ubits)
static uchar txtm_ubits[] = {
   0xef, 0x01, 0xff, 0x01, 0xff, 0x01, 0x7c, 0x00, 0x38, 0x00, 0x38, 0x00,
   0x38, 0x00, 0x38, 0x00, 0x38, 0x00, 0x38, 0x00, 0x38, 0x00, 0x38, 0x00,
   0x7c, 0x00, 0xff, 0x01, 0xff, 0x01, 0xef, 0x01};

static Curs  cursors[BUTCUR_NUM];

static void  draw(ButRcur *rc);
static void  erase(ButRcur *rc);
static void  butRcur_hideForRedraw(ButRcur *rc, int winId, int x,int y,
				   int w,int h);
  

void  butEnv_rcInit(ButEnv *env)  {
  Drawable  d = RootWindow(env->dpy, DefaultScreen(env->dpy));
  static bool  first_time = TRUE;
  ButCur  i;
  int  dataWidth, j, k;

  if (first_time)  {
    first_time = FALSE;
    for (i = butCur_idle;  i < butCur_bogus;  ++i)
      cursors[i].pic = NULL;

    cursors[butCur_idle].fontcurnum = XC_left_ptr;
    cursors[butCur_idle].hotX = 1;
    cursors[butCur_idle].hotY = 1;
    cursors[butCur_idle].w = 16;
    cursors[butCur_idle].h = 16;
    cursors[butCur_idle].pic = ntp_bits;
    cursors[butCur_idle].mask = ntm_bits;

    cursors[butCur_twitch].fontcurnum = XC_arrow;
    cursors[butCur_twitch].hotX = 14;
    cursors[butCur_twitch].hotY = 1;
    cursors[butCur_twitch].w = 16;
    cursors[butCur_twitch].h = 16;
    cursors[butCur_twitch].pic = tp_bits;
    cursors[butCur_twitch].mask = tm_bits;

    cursors[butCur_text].fontcurnum = XC_xterm;
    cursors[butCur_text].hotX = 4;
    cursors[butCur_text].hotY = 8;
    cursors[butCur_text].w = 16;
    cursors[butCur_text].h = 16;
    cursors[butCur_text].pic = txtp_bits;
    cursors[butCur_text].mask = txtm_bits;

    cursors[butCur_up].fontcurnum = XC_sb_up_arrow;
    cursors[butCur_up].hotX = 14;
    cursors[butCur_up].hotY = 14;
    cursors[butCur_up].w = 16;
    cursors[butCur_up].h = 16;
    cursors[butCur_up].pic = ntp_bits;
    cursors[butCur_up].mask = ntm_bits;

    cursors[butCur_down].fontcurnum = XC_sb_down_arrow;
    cursors[butCur_down].hotX = 14;
    cursors[butCur_down].hotY = 14;
    cursors[butCur_down].w = 16;
    cursors[butCur_down].h = 16;
    cursors[butCur_down].pic = ntp_bits;
    cursors[butCur_down].mask = ntm_bits;

    cursors[butCur_left].fontcurnum = XC_sb_left_arrow;
    cursors[butCur_left].hotX = 14;
    cursors[butCur_left].hotY = 14;
    cursors[butCur_left].w = 16;
    cursors[butCur_left].h = 16;
    cursors[butCur_left].pic = ntp_bits;
    cursors[butCur_left].mask = ntm_bits;

    cursors[butCur_right].fontcurnum = XC_sb_right_arrow;
    cursors[butCur_right].hotX = 14;
    cursors[butCur_right].hotY = 14;
    cursors[butCur_right].w = 16;
    cursors[butCur_right].h = 16;
    cursors[butCur_right].pic = ntp_bits;
    cursors[butCur_right].mask = ntm_bits;

    cursors[butCur_lr].fontcurnum = XC_sb_h_double_arrow;
    cursors[butCur_lr].hotX = 14;
    cursors[butCur_lr].hotY = 14;
    cursors[butCur_lr].w = 16;
    cursors[butCur_lr].h = 16;
    cursors[butCur_lr].pic = ntp_bits;
    cursors[butCur_lr].mask = ntm_bits;

    cursors[butCur_ud].fontcurnum = XC_sb_v_double_arrow;
    cursors[butCur_ud].hotX = 14;
    cursors[butCur_ud].hotY = 14;
    cursors[butCur_ud].w = 16;
    cursors[butCur_ud].h = 16;
    cursors[butCur_ud].pic = ntp_bits;
    cursors[butCur_ud].mask = ntm_bits;

    cursors[butCur_grab].fontcurnum = XC_fleur;
    cursors[butCur_grab].hotX = 14;
    cursors[butCur_grab].hotY = 14;
    cursors[butCur_grab].w = 16;
    cursors[butCur_grab].h = 16;
    cursors[butCur_grab].pic = ntp_bits;
    cursors[butCur_grab].mask = ntm_bits;
  }
  
  for (i = butCur_idle;  i < butCur_bogus;  ++i)  {
    /* "Grey out" the remote cursors. */
    dataWidth = (cursors[i].w + 7) / 8;
    for (j = 0;  j < cursors[i].h;  j += 2)  {
      for (k = 0;  k < dataWidth;  ++k)  {
	cursors[i].mask[j*dataWidth + k] &= 0x55;
	if (j+1 < cursors[i].h)  {
	  cursors[i].mask[(j+1)*dataWidth + k] &= 0xaa;
	}
      }
    }
    env->cursors[i] = XCreateFontCursor(env->dpy, cursors[i].fontcurnum);
    env->cpic[i] = XCreateBitmapFromData(env->dpy, d, cursors[i].pic,
					 cursors[i].w, cursors[i].h);
    env->cmask[i] = XCreateBitmapFromData(env->dpy, d, cursors[i].mask,
					  cursors[i].w, cursors[i].h);
  }
  env->curnum = butCur_idle;
  env->curwin = None;
  env->curhold = NULL;
  env->curlast = butCur_bogus;
}


/* Erase the cursor if it is in the area specified. */
void  butRcur_create(ButRcur *rc, ButEnv *env)  {
  MAGIC_SET(rc);
  rc->env = env;
  rc->winId = -2;
  rc->drawn = FALSE;
  rc->under = XCreatePixmap(env->dpy, DefaultRootWindow(env->dpy),
			    16,16, env->depth);
}


void  butRcur_move(ButRcur *rc, int winId, int rx,int ry, int rw,int rh,
		   ButCur type)  {
  ButEnv  *env = rc->env;

  assert(MAGIC(rc));
  if ((winId < 0) || (winId >= env->maxWinIds))
    winId = -2;
  else if (env->id2Win[winId] == NULL)
    winId = -2;
  if ((winId == rc->winId) && (rx == rc->rx) && (ry == rc->ry) &&
      (rw == rc->rw) && (rh == rc->rh) && (type == rc->type))
    return;
  if (rc->drawn)
    erase(rc);
  rc->winId = winId;
  rc->rx = rx;
  rc->ry = ry;
  rc->rw = rw;
  rc->rh = rh;
  rc->type = type;
  if (winId != -2)  {
    rc->lx = (rx * env->id2Win[winId]->w + rw/2) / rw - cursors[type].hotX;
    rc->ly = (ry * env->id2Win[winId]->h + rh/2) / rh - cursors[type].hotY;
    draw(rc);
  }
}


void  butRcur_redraw(ButEnv *env, int winId, int x,int y, int w,int h)  {
  int  i;
  
  if (winId == -2)
    return;
  for (i = 0;  i < env->numPartners;  ++i)  {
    if (env->partners[i])  {
      if (butRnet_valid(env->partners[i]) &&
	  (winId == env->partners[i]->rc.winId))
	butRcur_hideForRedraw(&env->partners[i]->rc, winId, x,y, w,h);
    }
  }
}


static void  butRcur_hideForRedraw(ButRcur *rc, int winId, int x,int y,
				   int w,int h)  {
  assert(MAGIC(rc));
  if (rc->drawn && (rc->winId == winId) &&
      (x < rc->lx+16) &&
      (y < rc->ly+16) &&
      (x+w > rc->lx) && (y+h > rc->ly))  {
    /* We need to erase it.  Will the redraw erase it for us? */
    if ((x <= rc->lx) && (y <= rc->ly) &&
	(x+w >= rc->lx+16) &&
	(y+h >= rc->ly+16))  {
      rc->drawn = FALSE;
    } else
      erase(rc);
  }
}


/* xio is the I/O context that the move got reported in. */
static void  draw(ButRcur *rc)  {
  ButEnv  *env = rc->env;

  assert(!rc->drawn);
  assert(MAGIC(rc));
  XCopyArea(env->dpy, env->id2Win[rc->winId]->win, rc->under, env->gc2,
	    rc->lx,rc->ly, 16,16, 0,0);
  if (!env->colorp)
    XSetFillStyle(env->dpy, env->gc2, FillSolid);
  XSetClipMask(env->dpy, env->gc2, env->cmask[rc->type]);
  XSetClipOrigin(env->dpy, env->gc2, rc->lx,rc->ly);
  XSetForeground(env->dpy, env->gc2, env->colors[BUT_FG]);
  XSetBackground(env->dpy, env->gc2, env->colors[BUT_LIT]);
  XCopyPlane(env->dpy, env->cpic[rc->type],env->id2Win[rc->winId]->win,
	     env->gc2, 0,0, 16,16, rc->lx,rc->ly, 1L);
  XSetClipMask(env->dpy, env->gc2, None);
  if (!env->colorp)
    XSetFillStyle(env->dpy, env->gc2, FillTiled);
  rc->drawn = TRUE;
}


static void  erase(ButRcur *rc)  {
  ButEnv  *env = rc->env;

  assert(MAGIC(rc));
  assert(rc->drawn);
  XCopyArea(env->dpy, rc->under, env->id2Win[rc->winId]->win, env->gc2,
	    0,0, 16,16, rc->lx,rc->ly);
  rc->drawn = FALSE;
}


/* Change the cursor to make it twitched or not twitched. */
void  butEnv_setCursor(ButEnv *env, But *but, ButCur newcursor)  {
  if (newcursor == butCur_idle)  {
    if (but == env->curhold)  {
      env->curnum = newcursor;
      env->curhold = NULL;
    }
  } else  {
    env->curnum = newcursor;
    env->curhold = but;
  }
  env->curwin = but->win->physWin;
}


/* Set up any cursor changes that may have occurred. */
void  butEnv_rcActivate(ButEnv *env)  {
  int  i;

  if ((env->curlast != env->curnum) && (env->curwin != None))  {
    assert(env->curnum < butCur_bogus);
    env->curlast = env->curnum;
    XDefineCursor(env->dpy, env->curwin, env->cursors[env->curlast]);
    butRnet_mMove(env, -1, -1,-1, -1,-1, env->curnum);
  }
  for (i = 0;  i < env->numPartners;  ++i)  {
    if (env->partners[i])  {
      if (butRnet_valid(env->partners[i]) &&
	  (env->partners[i]->rc.winId >= 0) &&
	  !env->partners[i]->rc.drawn)
	draw(&env->partners[i]->rc);
    }
  }
}


#endif
