/*
 * wmslib/src/but/list.c, part of wmslib (Library functions)
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
#include <wms/str.h>
#include <but/write.h>
#ifdef  _BUT_LIST_H_
  Levelization Error.
#endif
#include <but/list.h>


/**********************************************************************
 * Data types
 **********************************************************************/
typedef struct ListEl_struct  {
  Str  str;
  bool  pressable;
} ListEl;


typedef struct List_struct  {
  ButOut  (*func)(But *but, int line);
  int  numLines, maxLines, selectLine, pressLine;
  ListEl  *lines;
  int  numTabs, *tabs;
  ButTextAlign  *aligns;

  MAGIC_STRUCT
} List;


/**********************************************************************
 * Forward declarations
 **********************************************************************/
static ButOut  mmove(But *but, int x, int y);
static ButOut  mleave(But *but);
static ButOut  mpress(But *but, int butnum, int x, int y);
static ButOut  mrelease(But *but, int butnum, int x, int y);
static void  draw(But *but, int x, int y, int w, int h);
static ButOut  destroy(But *but);
static void  newFlags(But *but, uint flags);
static void  drawLine(But *but, int lineNum);


/**********************************************************************
 * Functions
 **********************************************************************/
But  *butList_create(ButOut (*func)(But *but, int line), void *packet,
		     ButWin *win, int layer, int flags)  {
  static const ButAction  action = {
    mmove, mleave, mpress, mrelease,
    NULL, NULL, draw, destroy, newFlags, NULL, NULL};
  List  *l;
  But  *but;
  int  i;
  const int  len = 1;

  l = wms_malloc(sizeof(List));
  MAGIC_SET(l);
  but = but_create(win, l, &action);
  but->uPacket = packet;
  but->layer = layer;
  but->flags = flags;

  l->func = func;
  l->numLines = l->maxLines = len;
  l->selectLine = -1;
  l->pressLine = -1;
  l->lines = wms_malloc(len * sizeof(ListEl));
  l->numTabs = 0;
  l->tabs = NULL;
  l->aligns = NULL;
  for (i = 0;  i < len;  ++i)  {
    l->lines[i].pressable = FALSE;
    str_init(&l->lines[i].str);
  }
  but_init(but);
  return(but);
}


void  butList_setTabs(But *but, const int *tabs,
		      const ButTextAlign *tabAligns, int numTabs)  {
  List  *l = but->iPacket;
  int  i;

  assert(MAGIC(l));
  if (l->numTabs < numTabs)  {
    if (l->tabs)  {
      wms_free(l->tabs);
      wms_free(l->aligns);
    }
    l->tabs = wms_malloc(numTabs * sizeof(int));
    l->aligns = wms_malloc(numTabs * sizeof(ButTextAlign));
    l->numTabs = numTabs;
  }
  for (i = 0;  i < numTabs;  ++i)  {
    l->tabs[i] = tabs[i];
    if (tabAligns)
      l->aligns[i] = tabAligns[i];
    else
      l->aligns[i] = butText_left;
  }
}


void  butList_resize(But *but, int x, int y, int w)  {
  List  *list = but->iPacket;
  int  listLen;

  assert(MAGIC(list));
  listLen = list->numLines;
  if (listLen == 0) {
    listLen = 1;
    str_copyCharsLen(&list->lines[0].str, "", 0);
  }
  but_resize(but, x, y, w,
	     listLen * butEnv_fontH(but->win->env, 0));
}


void  butList_changeLine(But *but, int line, const char *text)  {
  List  *l = but->iPacket;

  assert(MAGIC(l));
  if (line >= l->numLines)  {
    butList_setLen(but, line + 1);
    butList_resize(but, but->x, but->y, but->w);
  }
  str_copyChars(&l->lines[line].str, text);
  if (str_len(&l->lines[line].str) == 0)  {
    l->lines[line].pressable = FALSE;
    if (l->selectLine == line)
      l->selectLine = -1;
    if (l->pressLine == line)
      l->pressLine = -1;
  } else  {
    l->lines[line].pressable = TRUE;
  }
  drawLine(but, line);
}


void  butList_setLen(But *but, int newLen)  {
  List  *list = but->iPacket;
  int  i;
  ListEl  *newLines;

  assert(MAGIC(list));
  if (newLen > list->maxLines)  {
    newLines = wms_malloc(newLen * sizeof(ListEl));
    for (i = 0;  i < list->maxLines;  ++i)  {
      newLines[i] = list->lines[i];
    }
    for (;  i < newLen;  ++i)  {
      newLines[i].pressable = FALSE;
      str_init(&newLines[i].str);
    }
    wms_free(list->lines);
    list->maxLines = newLen;
    list->lines = newLines;
  }
  if (newLen > list->numLines)  {
    for (i = list->numLines;  i < newLen;  ++i)  {
      str_copyCharsLen(&list->lines[i].str, "", 0);
      list->lines[i].pressable = FALSE;
    }
  }
  list->numLines = newLen;
  butList_resize(but, but->x, but->y, but->w);
}


static ButOut  destroy(But *but)  {
  List  *l = but->iPacket;
  int  i;
  
  for (i = 0;  i < l->maxLines;  ++i)  {
    str_deinit(&l->lines[i].str);
  }
  wms_free(l->lines);
  if (l->tabs)  {
    wms_free(l->tabs);
    wms_free(l->aligns);
  }
  MAGIC_UNSET(l);
  wms_free(l);
  return(0);
}


static void  draw(But *but, int x, int y, int w, int h)  {
  ButWin  *win = but->win;
  ButEnv  *env = win->env;
  List  *l = but->iPacket;
  int  i, fontH;
  int  txtY;
  int  startLine;

  butEnv_setXFg(env, BUT_FG);
  fontH = butEnv_fontH(env, 0);
  startLine = (y - but->y) / fontH;
  if (startLine < 0)
    startLine = 0;
  txtY = but->y + startLine * fontH;
  for (i = startLine;  i < l->numLines;  ++i)  {
    if (i == l->selectLine)  {
      if (i == l->pressLine)  {
	butEnv_setXFg(env, BUT_FG);
	XFillRectangle(env->dpy, win->win, env->gc,
		       but->x - win->xOff, txtY - win->yOff, but->w, fontH);
	butEnv_setXFg(env, BUT_BG);
      } else if (l->pressLine == -1)  {
	if (env->colorp)
	  butEnv_setXFg(env, BUT_HIBG);
	else
	  butEnv_setXFg(env, BUT_PBG);
	XFillRectangle(env->dpy, win->win, env->gc,
		       but->x - win->xOff, txtY - win->yOff, but->w, fontH);
	butEnv_setXFg(env, BUT_FG);
      }
    }
    butWin_writeTabs(win, but->x, txtY, str_chars(&l->lines[i].str),
		     0, l->tabs, l->aligns);
    if ((i == l->selectLine) && (i == l->pressLine))
      butEnv_setXFg(env, BUT_FG);
    txtY += fontH;
    if (txtY >= y + h)
      break;
  }
}


static void  newFlags(But *but, uint flags)  {
  uint  ofl = but->flags;

  but->flags = flags;
  if ((flags & BUT_NETPRESS) && !(ofl & BUT_NETPRESS))  {
    snd_play(&but_downSnd);
  }
  if (((flags & (BUT_PRESSABLE|BUT_DRAWABLE|BUT_PRESSED|BUT_NETPRESS)) !=
       (ofl   & (BUT_PRESSABLE|BUT_DRAWABLE|BUT_PRESSED|BUT_NETPRESS))) ||
      (((flags & (BUT_TWITCHED|BUT_NETTWITCH)) !=
	(ofl & (BUT_TWITCHED|BUT_NETTWITCH))) &&
       (flags & (BUT_PRESSED|BUT_NETPRESS))))
    but_draw(but);
}


static ButOut  mmove(But *but, int x, int y)  {
  ButOut  retval = BUTOUT_CAUGHT;
  uint  newflags = but->flags;
  bool  overBut;
  List  *list = but->iPacket;
  int  oldSelect;
  
  overBut = ((x >= but->x) && (x < but->x + but->w) &&
	     (y >= but->y) &&
	     (y < but->y + list->numLines * butEnv_fontH(but->win->env, 0)));
  if (but->flags & BUT_PRESSABLE)  {
    oldSelect = list->selectLine;
    list->selectLine = (y - but->y) / butEnv_fontH(but->win->env, 0);
    if ((list->selectLine < 0) || (list->selectLine >= list->numLines))
      list->selectLine = -1;
    else if (!list->lines[list->selectLine].pressable)
      list->selectLine = -1;
    if (oldSelect != list->selectLine)  {
      drawLine(but, oldSelect);
      drawLine(but, list->selectLine);
    }
  }
  if (overBut)  {
    if (!(but->flags & BUT_PRESSABLE))
      return(BUTOUT_CAUGHT);
    if (list->selectLine >= 0)
      newflags |= BUT_TWITCHED;
    else
      newflags &= ~BUT_TWITCHED;
  } else  {
    if (!(but->flags & BUT_PRESSABLE))
      return(0);
    newflags &= ~BUT_TWITCHED;
    if (!(newflags & BUT_LOCKED))
      retval &= ~BUTOUT_CAUGHT;
  }
  if (!(but->flags & BUT_TWITCHED) && (newflags & BUT_TWITCHED))
    butEnv_setCursor(but->win->env, but, butCur_twitch);
  else if ((but->flags & BUT_TWITCHED) && !(newflags & BUT_TWITCHED))
    butEnv_setCursor(but->win->env, but, butCur_idle);
  if (newflags != but->flags)  {
    but_newFlags(but, newflags);
  }
  return(retval);
}


static ButOut  mleave(But *but)  {
  List  *list = but->iPacket;
  int  newflags = but->flags;
  
  newflags &= ~BUT_TWITCHED;
  if (but->flags & BUT_TWITCHED)
    butEnv_setCursor(but->win->env, but, butCur_idle);
  if (newflags != but->flags)  {
    but_newFlags(but, newflags);
  }
  if (list->selectLine != -1)  {
    drawLine(but, list->selectLine);
    list->selectLine = -1;
  }
  return(BUTOUT_CAUGHT);
}


static ButOut  mpress(But *but, int butnum, int x, int y)  {
  ButOut  retval = BUTOUT_CAUGHT;
  uint  newflags = but->flags;
  List  *list = but->iPacket;
  int  oldPressLine;
  
  assert(MAGIC(list));
  if (!(newflags & BUT_TWITCHED))  {
    retval = BUTOUT_ERR;
  } else  {
    if (butnum == 1)  {
      newflags |= BUT_PRESSED | BUT_LOCKED;
      oldPressLine = list->pressLine;
      list->pressLine = list->selectLine;
      if (oldPressLine != list->pressLine)  {
	drawLine(but, oldPressLine);
	drawLine(but, list->pressLine);
      }
      if (list->pressLine < 0)  {
	return(BUTOUT_CAUGHT | BUTOUT_ERR);
      }
    } else
      return(BUTOUT_CAUGHT | BUTOUT_ERR);
  }
  if (!(but->flags & BUT_PRESSED) && (newflags & BUT_PRESSED))
    snd_play(&but_downSnd);
  if (newflags != but->flags)
    but_newFlags(but, newflags);
  return(retval);
}


static ButOut  mrelease(But *but, int butnum, int x, int y)  {
  ButOut  retval = BUTOUT_CAUGHT;
  int  newflags = but->flags;
  List  *list = but->iPacket;
  int  pressLine = -1;
  
  assert(MAGIC(list));
  if (butnum != 1)  {
    if (but->flags & BUT_TWITCHED)
      return(BUTOUT_CAUGHT);
    else
      return(0);
  }
  if (!(but->flags & BUT_PRESSED))
    return(0);
  if (but->flags & BUT_TWITCHED)  {
    pressLine = (y - but->y) / butEnv_fontH(but->win->env, 0);
  } else  {
    retval |= BUTOUT_ERR;
  }
  newflags &= ~(BUT_PRESSED|BUT_LOCKED);
  if ((but->flags & BUT_PRESSED) && !(newflags & BUT_PRESSED) &&
      !(retval & BUTOUT_ERR))
    snd_play(&but_upSnd);
  if (newflags != but->flags)
    but_newFlags(but, newflags);
  if ((pressLine >= 0) && (pressLine == list->pressLine))  {
    list->pressLine = -1;
    retval |= list->func(but, pressLine);
  } else
    list->pressLine = -1;
  return(retval);
}


int  butList_len(But *but)  {
  List  *list = but->iPacket;

  assert(MAGIC(list));
  return(list->numLines);
}


static void  drawLine(But *but, int lineNum)  {
  if (lineNum >= 0)  {
    butWin_redraw(but->win, but->x,
		  but->y + lineNum * butEnv_fontH(but->win->env, 0),
		  but->w, butEnv_fontH(but->win->env, 0));
  }
}


void  butList_setPress(But *but, int line, bool pressable)  {
  List  *list;

  assert(MAGIC(but));
  list = but->iPacket;
  assert(MAGIC(list));
  assert(line < list->numLines);
  list->lines[line].pressable = pressable;
}

		  
const char  *butList_get(But *but, int line)  {
  List  *list;

  assert(MAGIC(but));
  list = but->iPacket;
  assert(MAGIC(list));
  assert(line < list->numLines);
  return(str_chars(&list->lines[line].str));
}


#endif
