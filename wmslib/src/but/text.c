/*
 * wmslib/src/but/text.c, part of wmslib (Library functions)
 * Copyright (C) 1994-1996 William Shubert.
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
#include <but/text.h>
#include <wms/str.h>


/**********************************************************************
 * Data Structures
 **********************************************************************/
typedef struct Txt_struct  {
  Str  text;
  ButTextAlign  align;
  int  font;
  int  color;
  bool  stipple;
} Txt;


/**********************************************************************
 * Forward Declarations
 **********************************************************************/
static void  draw(But *but, int x,int y, int w,int h);
static ButOut  destroy(But *but);


/**********************************************************************
 * Globals
 **********************************************************************/
static const ButAction  action = {
  NULL,NULL,NULL,NULL,
  NULL,NULL, draw, destroy, but_flags, NULL};


/**********************************************************************
 * Functions
 **********************************************************************/
But  *butText_create(ButWin *win, int layer, int flags,
		     const char *text, ButTextAlign align)  {
  But  *but;
  Txt  *t;
  
  t = wms_malloc(sizeof(Txt));
  but = but_create(win, t, &action);
  but->layer = layer;
  but->flags = flags;

  str_init(&t->text);
  t->align = align;
  t->font = 0;
  t->color = BUT_FG;
  t->stipple = FALSE;
  but_init(but);
  if (text != NULL)
    butText_set(but, text);
  return(but);
}


void  butText_set(But *but, const char *text)  {
  Txt  *t = but->iPacket;
  
  assert(but->action == &action);
  if (text == NULL)
    text = "";
  str_copyChars(&t->text, text);
  but_draw(but);
}


void  butText_setFont(But *but, int fontnum)  {
  Txt  *t = but->iPacket;
  
  assert(but->action == &action);
  t->font = fontnum;
  but_draw(but);
}


void  butText_setColor(But *but, int color, bool stipple)  {
  Txt  *t = but->iPacket;

  if (color == BUT_NOCHANGE)
    color = t->color;
  if ((color != t->color) || (stipple != t->stipple))  {
    t->color = color;
    t->stipple = stipple;
    but_draw(but);
  }
}


static ButOut  destroy(But *but)  {
  Txt *t = but->iPacket;
  
  str_deinit(&t->text);
  return(0);
}


static void  draw(But *but, int x,int y, int w,int h)  {
  Txt  *t = but->iPacket;
  ButEnv  *env = but->win->env;
  XFontStruct  *fs = env->fonts[t->font];
  int  th;
  const char  *text = str_chars(&t->text);
  
  if (t->stipple)  {
    XSetFillStyle(env->dpy, env->gc, FillStippled);
    XSetForeground(env->dpy, env->gc, env->colors[t->color]);
  } else  {
    butEnv_setXFg(env, t->color);
  }
  x = but->x;
  if (t->align == butText_center)
    x += (but->w - butEnv_textWidth(env, text, t->font)) / 2;
  else if (t->align == butText_right)
    x += (but->w - butEnv_textWidth(env, text, t->font));
  th = fs->ascent + fs->descent;
  butWin_write(but->win, x, but->y + (but->h - th) / 2, text, t->font);
  if (t->stipple)  {
    butEnv_stdFill(env);
  }
}


int  butText_resize(But *but, int x, int y, int h)  {
  Txt  *txt = but->iPacket;
  int  w = butEnv_textWidth(but->win->env, str_chars(&txt->text), txt->font);

  switch (txt->align)  {
  case butText_left:
    break;
  case butText_center:
    x -= w/2;
    break;
  case butText_right:
    x -= w;
    break;
  default:
    break;
  }
  but_resize(but, x, y, w, h);
  return(w);
}


const char  *butText_get(But *but)  {
  Txt  *t;
  
  assert(MAGIC(but));
  assert(but->action == &action);
  t = but->iPacket;
  return(str_chars(&t->text));
}
  
#endif
