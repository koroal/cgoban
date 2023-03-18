/*
 * src/msg.c, part of wmslib library
 * Copyright (C) 1994-1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 */

#include <math.h>
#include <wms.h>
#include <but/but.h>
#include <but/text.h>
#include <but/ctext.h>
#include <but/box.h>
#include <but/tblock.h>
#include <but/plain.h>
#include <but/textin.h>
#include <abut/msg.h>


static ButOut  unmap(ButWin *win);
static ButOut  wDestroy(ButWin *win);
static ButOut  resize(ButWin *win);
static ButOut  ok_pressed(But *but);
static void  checkNewSize(AbutMsg *mwin, ButWin *win);
static void  calcDims(Abut *abut, int fontNum, const char *text,
		      int *w, int *h, int *textH, int numTins);
static ButOut  msg_resize(ButWin *win, AbutMsg *mwin);
static void  fillboxDead(But *but);


/*
 * UseMsg is never accessed.  We just need a void * guaranteed to be
 *   different from any the user could pass us.
 */
static int  useMsg = 0;


AbutMsg  *abutMsg_inCreate(Abut *a, ButWin *win, int layer, const char *text,
			   void *packet,
			   int numTins, const AbutMsgTin *tinList)  {
  static AbutMsgOpt  ok[1];
  static ButKey  ka_ok[] = {{XK_Return, 0, ShiftMask},
			    {XK_KP_Enter, 0, ShiftMask}, {0,0,0}};

  ok[0].name = a->ok;
  ok[0].callback = ok_pressed;
  ok[0].keyEq = ka_ok;
  ok[0].packet = &useMsg;
  return(abutMsg_optInCreate(a, win, layer, text, NULL, NULL, 1, ok,
			     numTins, tinList));
}


AbutMsg  *abutMsg_optInCreate(Abut *a, ButWin *win, int layer,
			      const char *text,
			      ButOut (*destroy)(void *packet), void *packet,
			      int numButs, const AbutMsgOpt *optList,
			      int numTins, const AbutMsgTin *tinList)  {
  ButEnv  *env;
  AbutMsg  *mwin;
  int  winW, winH;
  int  i;
  void  *butPacket;

  assert(MAGIC(a));
  assert(a->env == butWin_env(win));
  env = a->env;
  mwin = wms_malloc(sizeof(AbutMsg));
  MAGIC_SET(mwin);
  mwin->abut = a;
  mwin->win = win;
  mwin->numButs = numButs;
  mwin->buts = wms_malloc(numButs * sizeof(But *));
  mwin->numTins = numTins;
  mwin->tins = wms_malloc(numTins * sizeof(But *));
  mwin->tinTitles = wms_malloc(numTins * sizeof(But *));
  mwin->butDesc = optList;
  mwin->destroy = destroy;
  mwin->packet = packet;
  mwin->layer = layer;
  
  calcDims(a, 0, text, &winW, &winH, &mwin->textH, numTins);
  mwin->w = winW;
  mwin->h = winH;
  mwin->box = butBoxFilled_create(win, layer, BUT_DRAWABLE);
  but_setPacket(mwin->box, mwin);
  but_setDestroyCallback(mwin->box, fillboxDead);
  butBoxFilled_setColors(mwin->box, a->ulColor, a->lrColor, a->bgColor);
  butBoxFilled_setPixmaps(mwin->box, a->ulPixmap, a->lrPixmap, a->bgPixmap);
  mwin->text = butTblock_create(win, layer+1, BUT_DRAWABLE,
				text, butText_just);
  for (i = 0;  i < numButs;  ++i)  {
    butPacket = optList[i].packet;
    if (butPacket == &useMsg)
      butPacket = mwin;
    mwin->buts[i] = butCt_create(optList[i].callback, butPacket,
				 win, layer+1, BUT_DRAWABLE|BUT_PRESSABLE,
				 optList[i].name);
    if (optList[i].keyEq)
      but_setKeys(mwin->buts[i], optList[i].keyEq);
  }
  for (i = 0;  i < numTins;  ++i)  {
    if (tinList[i].name)
      mwin->tinTitles[i] = butText_create(win, layer+1, BUT_DRAWABLE,
					  tinList[i].name, butText_left);
    else
      mwin->tinTitles[i] = NULL;
    mwin->tins[i] = butTextin_create(tinList[i].callback, packet,
				     win, layer+1, BUT_DRAWABLE|BUT_PRESSABLE,
				     tinList[i].def, 500);
  }
  if (butWin_w(win))
    msg_resize(win, mwin);
  return(mwin);
}


AbutMsg  *abutMsg_winInCreate(Abut *a, const char *title, const char *text,
			      void *packet,
			      int numTins, const AbutMsgTin *tinList)  {
  static AbutMsgOpt  ok[1];
  static ButKey  ka_ok[] = {{XK_Return, 0, ShiftMask|ControlMask},
			    {XK_KP_Enter, 0, ShiftMask|ControlMask},
			    {0, 0, 0}};

  ok[0].name = a->ok;
  ok[0].callback = ok_pressed;
  ok[0].keyEq = ka_ok;
  ok[0].packet = &useMsg;
  return(abutMsg_winOptInCreate(a, title, text, NULL, NULL, 1, ok,
				numTins, tinList));
}


AbutMsg  *abutMsg_winOptInCreate(Abut *a, const char *title, const char *text,
				 ButOut (*destroy)(void *packet), void *packet,
				 int numButs, const AbutMsgOpt *optList,
				 int numTins, const AbutMsgTin *tinList)  {
  ButEnv  *env;
  AbutMsg  *mwin;
  ButWin  *win;
  int  winW, winH;
  int  i;
  void  *butPacket;

  assert(MAGIC(a));
  env = a->env;
  mwin = wms_malloc(sizeof(AbutMsg));
  MAGIC_SET(mwin);
  mwin->abut = a;
  mwin->numButs = numButs;
  mwin->buts = wms_malloc(numButs * sizeof(But *));
  mwin->numTins = numTins;
  mwin->tins = wms_malloc(numTins * sizeof(But *));
  mwin->tinTitles = wms_malloc(numTins * sizeof(But *));
  mwin->butDesc = optList;
  mwin->destroy = destroy;
  mwin->packet = packet;
  mwin->layer = 0;
  
  calcDims(a, 0, text, &winW, &winH, &mwin->textH, numTins);
  mwin->w = winW;
  mwin->h = winH;
  mwin->win = win = butWin_create(mwin, env, title, winW, winH,
				  unmap, NULL, resize, wDestroy);
  butWin_activate(win);
  mwin->box = butBoxFilled_create(win, 0, BUT_DRAWABLE);
  butBoxFilled_setColors(mwin->box, a->ulColor, a->lrColor, a->bgColor);
  butBoxFilled_setPixmaps(mwin->box, a->ulPixmap, a->lrPixmap, a->bgPixmap);
  mwin->text = butTblock_create(win, 1, BUT_DRAWABLE,
				text, butText_just);
  for (i = 0;  i < numButs;  ++i)  {
    butPacket = optList[i].packet;
    if (butPacket == &useMsg)
      butPacket = mwin;
    mwin->buts[i] = butCt_create(optList[i].callback, butPacket,
				 win, 1, BUT_DRAWABLE|BUT_PRESSABLE,
				 optList[i].name);
    if (optList[i].keyEq)
      but_setKeys(mwin->buts[i], optList[i].keyEq);
  }
  for (i = 0;  i < numTins;  ++i)  {
    mwin->tinTitles[i] = butText_create(win, 1, BUT_DRAWABLE,
					tinList[i].name, butText_left);
    mwin->tins[i] = butTextin_create(tinList[i].callback, packet, win,
				     1, BUT_DRAWABLE|BUT_PRESSABLE,
				     tinList[i].def, 150);
    if (tinList[i].flags & abutMsgTinFlags_secret)  {
      butTextin_setHidden(mwin->tins[i], TRUE);
    }
  }
  if (butWin_w(win))
    msg_resize(win, mwin);
  return(mwin);
}


static ButOut  unmap(ButWin *win)  {
  butWin_destroy(win);
  return(0);
}


static ButOut  wDestroy(ButWin *win)  {
  AbutMsg  *mwin = butWin_packet(win);
  ButOut  result = 0;

  assert(MAGIC(mwin));
  mwin->win = NULL;
  if (mwin->buts)  {
    wms_free(mwin->buts);
    mwin->buts = NULL;
  }
  if (mwin->tins)  {
    wms_free(mwin->tins);
    wms_free(mwin->tinTitles);
    mwin->tins = NULL;
    mwin->tinTitles = NULL;
  }
  if (mwin->destroy)
    result = mwin->destroy(mwin->packet);
  MAGIC_UNSET(mwin);
  wms_free(mwin);
  return(result);
}


static ButOut  resize(ButWin *win)  {
  return(msg_resize(win, butWin_packet(win)));
}


static ButOut  msg_resize(ButWin *win, AbutMsg *mwin)  {
  int  x, y, w, h, buth;
  ButEnv  *env = butWin_env(win);
  int  bw = butEnv_stdBw(env);
  int  butX, butW, i;
  int  maxTinNameLen = 0, newLen;

  assert(MAGIC(mwin));
  checkNewSize(mwin, win);
  x = (butWin_w(win) - mwin->w) / 2;
  y = (butWin_h(win) - mwin->h) / 2;
  w = mwin->w;
  h = mwin->h;
  buth = mwin->abut->butH;
  but_resize(mwin->box, x,y, w,h);
  butTblock_resize(mwin->text, x+bw*2, y+bw*2, w - bw*4);
  for (butX = bw*2, i = 0;  i < mwin->numButs;  ++i)  {
    butW = (w-bw-butX + (mwin->numButs - i) / 2) / (mwin->numButs - i) - bw;
    but_resize(mwin->buts[i], x+butX, y+h-bw*2-buth, butW, buth);
    butX += butW + bw;
  }
  for (i = 0;  i < mwin->numTins;  ++i)  {
    if (mwin->tinTitles[i])  {
      newLen = butText_resize(mwin->tinTitles[i], x + bw*2,
			      y + h - (buth + bw)*(mwin->numTins - i + 1) -
			      bw*2, buth) + bw;
      if (newLen > maxTinNameLen)
	maxTinNameLen = newLen;
    }
  }
  for (i = 0;  i < mwin->numTins;  ++i)  {
    but_resize(mwin->tins[i], x + bw*2 + maxTinNameLen, 
	       y + h - (buth + bw)*(mwin->numTins - i + 1) - bw*2,
	       w - bw*4 - maxTinNameLen, buth);
  }
  return(0);
}


static void  checkNewSize(AbutMsg *mwin, ButWin *win)  {
  ButEnv  *env = butWin_env(win);
  int  textH, butH;

  textH = butEnv_fontH(env, 0);
  butH = mwin->abut->butH;
  if (textH != mwin->textH)  {
    calcDims(mwin->abut, 0, butTblock_getText(mwin->text),
	     &mwin->w, &mwin->h, &mwin->textH, mwin->numTins);
    if (mwin->layer == 0)
      butWin_resize(win, mwin->w, mwin->h);
  }
}
  

static ButOut  ok_pressed(But *but)  {
  AbutMsg  *msg;

  msg = but_packet(but);
  assert(MAGIC(msg));
  if (msg->layer == 0)
    butWin_destroy(msg->win);
  else
    abutMsg_destroy(but_packet(but), TRUE);
  return(0);
}


void  abutMsg_destroy(AbutMsg *msg, bool propagate)  {
  int  i;
  int  x = 0, y = 0, w = 0, h = 0;

  assert(MAGIC(msg));
  if (propagate == FALSE)  {
    msg->destroy = NULL;
  }
  if (msg->box)  {
    x = but_x(msg->box);
    y = but_y(msg->box);
    w = but_w(msg->box);
    h = but_h(msg->box);
  }
  if (msg->buts)  {
    but_setPacket(msg->box, NULL);
    but_destroy(msg->box);
    but_destroy(msg->text);
    for (i = 0;  i < msg->numButs;  ++i)
      but_destroy(msg->buts[i]);
    for (i = 0;  i < msg->numTins;  ++i)  {
      but_destroy(msg->tins[i]);
      but_destroy(msg->tinTitles[i]);
    }
  }
  if ((msg->layer == 0) && msg->win)
    butWin_destroy(msg->win);
  else  {
    butWin_redraw(msg->win, x, y, w, h);
    MAGIC_UNSET(msg);
    wms_free(msg);
  }
}


static void  calcDims(Abut *abut, int fontNum, const char *text,
		      int *w, int *h, int *textHOut, int numTins)  {
  ButEnv  *env = abut->env;
  int  textW, textH, butH, butSpc;
  int  winW, winH, actualH, bw;
  double  b, c;

  textW = butEnv_textWidth(env, text, fontNum);
  *textHOut = textH = butEnv_fontH(env, fontNum);
  butH = abut->butH;
  bw = butEnv_stdBw(env);
  butSpc = butH + bw;

  /*
   * This is based on the calculations:
   *   winW = 2*winH
   *   (winW - 4*bw) * (winH - (4*bw + butSpc*(numTins + 1))) = textW*textH
   * Solve for winW...
   *   (winW - 4*bw) * (0.5*winW - 4*bw - butSpc*numTins - butSpc) =
   *      textW*textH
   *   0.5*winW^2 - winW*(-4*bw-butSpc*numTins-butSpc-2*bw) +
   *      16*bw*bw+4*bw*butSpc*numTins+4*bw*butSpc - textW*textH = 0
   *   winW^2 - winW*(-12*bw-2*butSpc*numTins-2*butSpc) +
   *      32*bw*bw+8*bw*butSpc*numTins+8*bw*butSpc - 2*textW*textH = 0
   * Using the quadratic equation, notice a is 1.0, and solve
   *   winW = (-b+sqrt(b*b-4*a*c))/2*a
   */
  b = (double)(-12 * bw - 2 * butSpc * numTins - 2 * butSpc);
  c = (double)(32*bw*bw + 8*bw*butSpc*numTins + 8*bw*butSpc - 2 * textW*textH);
  winW = (int)((-b + sqrt(b*b-4*c)) * 0.5 + 0.5);
  winH = winW / 2;
  if (winH < 2*textH + butSpc*(numTins + 1) + 4*butEnv_stdBw(env))
    winH = 2*textH + butSpc*(numTins + 1) + 4*butEnv_stdBw(env);
  winW = winH * 2;
  actualH = butTblock_guessH(env, text, winW - 4*butEnv_stdBw(env), fontNum) +
    butSpc*(numTins + 1) + 4*butEnv_stdBw(env);
  if (actualH > winH)  {
    winH = actualH;
    winW = 2*winH;
  }
  *w = winW;
  *h = winH;
}


static void  fillboxDead(But *but)  {
  AbutMsg  *mwin;

  assert(MAGIC(but));
  mwin = but_packet(but);
  if (mwin)  {
    assert(MAGIC(mwin));
    if (mwin->buts)  {
      wms_free(mwin->buts);
      mwin->buts = NULL;
    }
    if (mwin->tins)  {
      wms_free(mwin->tins);
      wms_free(mwin->tinTitles);
      mwin->tins = NULL;
      mwin->tinTitles = NULL;
    }
    mwin->box = mwin->text = NULL;
    mwin->win = NULL;
  }
}
