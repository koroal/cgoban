/*
 * wmslib/src/but/write.c, part of wmslib (Library functions)
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
#include <but/write.h>


void  butEnv_setChar(ButEnv *env, double width, const char *id,
		     void (*draw)(void *packet, ButWin *win,
				  int x, int y, int w, int h), void *packet)  {
  int  idval;

  idval = ((int)(uchar)id[0] << 8) + (uchar)id[1] - 256;
  env->write[idval].width = width;
  env->write[idval].packet = packet;
  env->write[idval].draw = draw;
}


int  butEnv_textWidth(ButEnv *env, const char *text, int fnum)  {
  uchar  c;
  int  special;
  int  total = 0, min_char, std_width;
  XCharStruct  *charlist;
  
  if (env->fonts[fnum]->per_char != NULL)  {
    charlist = env->fonts[fnum]->per_char;
    min_char = env->fonts[fnum]->min_char_or_byte2;
    for (;;)  {
      c = text[0];
      ++text;
      switch(c)  {
      case '\0':
	return(total);
	break;
      case '\1':
      case '\2':
	special = 256 * (c - 1) + text[0];
	++text;
	assert(env->write[special].draw != NULL);
	total += (env->fonts[fnum]->ascent + env->fonts[fnum]->descent) *
	  env->write[special].width + 0.5;
	break;
      default:
	assert((c >= min_char) && (c <= env->fonts[fnum]->max_char_or_byte2));
	total += charlist[c - min_char].width;
	break;
      }
    }
  } else  {  /* All chars are the same width. */
    std_width = env->fonts[fnum]->min_bounds.width;
    for (;;)  {
      c = text[0];
      ++text;
      switch(c)  {
      case '\0':
	return(total);
	break;
      case '\1':
      case '\2':
	special = 256 * (c - 1) + text[0];
	++text;
	assert(env->write[special].draw != NULL);
	total += (env->fonts[fnum]->ascent + env->fonts[fnum]->descent) *
	  env->write[special].width + 0.5;
	break;
      default:
	total += std_width;
	break;
      }
    }
  }
}


int  butEnv_charWidth(ButEnv *env, const char *text, int fnum)  {
  uchar  c, special;
  int  width;
  
  c = text[0];
  assert(c != '\0');
  switch(c)  {
  case '\1':
  case '\2':
    special = 256 * (c - 1) + text[1];
    assert(env->write[special].draw != NULL);
    width = (env->fonts[fnum]->ascent + env->fonts[fnum]->descent) *
      env->write[special].width + 0.5;
    break;
  default:
    if (env->fonts[fnum]->per_char != NULL)
      width = env->fonts[fnum]->per_char[c -
					 env->fonts[fnum]->min_char_or_byte2].
					   width;
    else
      width = env->fonts[fnum]->min_bounds.width;
    break;
  }
  return(width);
}


void  butWin_write(ButWin *win, int x, int y, const char *text, int font)  {
  int  wlen;
  ButEnv  *env = win->env;
  int  ascent = env->fonts[font]->ascent, special;
  int  w, h = ascent + env->fonts[font]->descent;
  uchar  c;

  XSetFont(env->dpy, env->gc, env->fonts[font]->fid);
  for (;;)  {
    for (wlen = 0;  (uchar)(text[wlen]) > BUTWRITE_MINPRINT;  ++wlen);
    if (wlen > 0)  {
      XDrawString(env->dpy, win->win, env->gc,
		  x-win->xOff, y+ascent-win->yOff, text, wlen);
      x += XTextWidth(env->fonts[font], text, wlen);
      text += wlen;
    }
    switch(c = text[0])  {
    case '\0':
      return;
      break;
    case '\1':
    case '\2':
      special = 256 * (c-1) + text[1];
      text += 2;
      assert(env->write[special].draw != NULL);
      w = h * env->write[special].width + 0.5;
      env->write[special].draw(env->write[special].packet, win,
			       x-win->xOff,y-win->yOff, w,h);
      x += w;
      break;
    }
  }
}
      

void  butWin_writeTabs(ButWin *win, int startX, int y, const char *text,
		       int font, const int *tabList,
		       const ButTextAlign *aligns)  {
  int  wlen;
  ButEnv  *env = win->env;
  int  ascent = env->fonts[font]->ascent, special;
  int  w, h = ascent + env->fonts[font]->descent;
  int  x = startX;
  uchar  c;
  int  tabNum = 0;
  ButTextAlign  align = butText_left;

  XSetFont(env->dpy, env->gc, env->fonts[font]->fid);
  for (;;)  {
    for (wlen = 0;  (uchar)(text[wlen]) > BUTWRITE_MINPRINT;  ++wlen);
    if (wlen > 0)  {
      w = XTextWidth(env->fonts[font], text, wlen);
      switch(align)  {
      case butText_left:
	break;
      case butText_center:
	x -= w / 2;
	break;
      case butText_right:
	x -= w;
	break;
      case butText_just:
	assert(0);
	break;
      }
      XDrawString(env->dpy, win->win, env->gc,
		  x - win->xOff, y+ascent-win->yOff, text, wlen);
      x += w;
      text += wlen;
    }
    switch(c = text[0])  {
    case '\0':
      return;
      break;
    case '\1':
    case '\2':
      special = 256 * (c-1) + text[1];
      text += 2;
      assert(env->write[special].draw != NULL);
      w = h * env->write[special].width + 0.5;
      env->write[special].draw(env->write[special].packet, win,
			       x-win->xOff,y-win->yOff, w,h);
      x += w;
      break;
    case '\t':
      x = startX + tabList[tabNum];
      align = aligns[tabNum];
      ++tabNum;
      ++text;
      break;
    }
  }
}


#endif  /* X11_DISP */
