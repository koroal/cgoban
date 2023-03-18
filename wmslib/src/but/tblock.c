/*
 * wmslib/src/but/tblock.c, part of wmslib (Library functions)
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
#include <wms/snd.h>
#include <but/but.h>
#include <but/box.h>
#include <but/tblock.h>
#include <but/text.h>


/**********************************************************************
 * Data Types
 **********************************************************************/
typedef struct Word_struct  {
  int  start, len, xoff, yoff, w;
} Word;

typedef struct  {
  char  *str;
  int  nwords;
  Word  *words;
  ButTextAlign  align;
  int  font;
  int  setup_w;
} Tb;


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
But  *butTblock_create(ButWin *win, int layer, int flags,
		       const char *text, ButTextAlign align)  {
  But  *but;
  Tb  *tb;
  
  assert(MAGIC(win));
  assert(MAGIC(&win->butsNoDraw));
  tb = wms_malloc(sizeof(Tb));
  but = but_create(win, tb, &action);
  but->layer = layer;
  but->flags = flags;

  tb->str = NULL;
  tb->nwords = 0;
  tb->words = NULL;
  tb->align = align;
  tb->font = 0;
  tb->setup_w = 0;
  but_init(but);
  if (text != NULL)
    butTblock_setText(but, text);
  return(but);
}


void  butTblock_setFont(But *but, int fontnum)  {
  Tb  *tb = but->iPacket;

  assert(but->action == &action);
  tb->font = fontnum;
  but_draw(but);
}


int  butTblock_getH(But *but)  {
  Tb  *tb = but->iPacket;
  int  fonth;

  assert(but->action == &action);
  fonth = but->win->env->fonts[tb->font]->ascent +
    but->win->env->fonts[tb->font]->descent;
  if (tb->nwords == 0)
    return(fonth);
  else
    return(tb->words[tb->nwords - 1].yoff + fonth);
}

  
const char  *butTblock_getText(But *but)  {
  Tb  *tb;

  assert(but->action == &action);
  tb = but->iPacket;
  return(tb->str);
}


void  butTblock_setText(But *but, const char *text)  {
  Tb  *tb = but->iPacket;
  ButEnv  *env = but->win->env;
  int  w = tb->setup_w = but->w, txtH;
  int  len, numLines, wordNum, totalWidth, i, j;
  int  lastSpaceSeen, spacesAtLastSpace, spacesThisLine;
  int  widthAtLastSpace, widthNonSpace, widthOfCurrentChar;
  int  widthNonSpaceAtLastSpace;
  bool  nonSpaceSeen, prevCharWasSpace;
  Word  *words;
  int  *widths;
  int  wordWMax, wordWMin;
  
  assert(but->action == &action);
  txtH = env->fonts[tb->font]->ascent + env->fonts[tb->font]->descent;
  if (text != NULL)  {
    len = strlen(text);
    if (tb->str != NULL)
      wms_free(tb->str);
    tb->str = wms_malloc((len + 1) * sizeof(char));
    strcpy(tb->str, text);
  }
  if (w == 0)
    return;
  text = tb->str;
  len = strlen(text);
  if (tb->words != NULL)
    wms_free(tb->words);
  if (text == NULL)  {
    tb->words = NULL;
    return;
  }
  tb->nwords = strlen(tb->str);
  if (tb->nwords == 0)
    return;
  tb->words = words = wms_malloc(tb->nwords * sizeof(Word));
  widths = wms_malloc(len * sizeof(int));
  words[0].start = 0;
  words[0].xoff = 0;
  words[0].yoff = 0;
  words[0].w = 0;
  wordNum = totalWidth = widthNonSpace = 0;
  widthAtLastSpace = 0;
  spacesAtLastSpace = 0;
  widthNonSpaceAtLastSpace = 0;
  wordWMax = wordWMin = 0;
  numLines = 1;
  spacesThisLine = lastSpaceSeen = 0;
  nonSpaceSeen = prevCharWasSpace = FALSE;
  for (i = 0;  i < len;  ++i)  {
    widthOfCurrentChar = butEnv_charWidth(env, tb->str+i, tb->font);
    assert(widthOfCurrentChar > 0);
    widths[i] = widthOfCurrentChar;
    if (tb->str[i] == ' ')  {
      if (!prevCharWasSpace)  {
	lastSpaceSeen = i;
	widthAtLastSpace = totalWidth;
	widthNonSpaceAtLastSpace = widthNonSpace;
	spacesAtLastSpace = spacesThisLine;
      }
      if (nonSpaceSeen)
	++spacesThisLine;
      else
	widthNonSpace += widthOfCurrentChar;
      prevCharWasSpace = TRUE;
    } else  {
      if ((uchar)tb->str[i] < BUTWRITE_MINPRINT)  {
	++i;
	widths[i] = 0;
      }
      nonSpaceSeen = TRUE;
      prevCharWasSpace = FALSE;
      widthNonSpace += widthOfCurrentChar;
    }
    totalWidth += widthOfCurrentChar;
    if (totalWidth > w)  {
      if (lastSpaceSeen != words[wordNum].start)
	i = lastSpaceSeen;
      words[wordNum].len = i - words[wordNum].start;
      switch(tb->align)  {
      case butText_center:
	words[wordNum].xoff += (w - widthAtLastSpace) / 2;
	words[wordNum].w = widthAtLastSpace;
	break;
      case butText_right:
	words[wordNum].xoff += (w - widthAtLastSpace);
	words[wordNum].w = widthAtLastSpace;
	break;
      case butText_just:
	widthNonSpace = widthNonSpaceAtLastSpace;
	totalWidth = 0;
	j = words[wordNum].start;
	nonSpaceSeen = FALSE;
	while (spacesAtLastSpace > 0)  {
	  assert(j <= i);
	  if ((tb->str[j] == ' ') && nonSpaceSeen)  {
	    widthOfCurrentChar = (w - widthNonSpace +
				  spacesAtLastSpace/2) / spacesAtLastSpace;
	    --spacesAtLastSpace;
	    widthNonSpace += widthOfCurrentChar;
	    totalWidth += widthOfCurrentChar;
	    if (tb->str[j - 1] != ' ')  {
	      words[wordNum].len = j - words[wordNum].start;
	      assert(words[wordNum].w > 0);
	      assert(words[wordNum].w < w);
	      ++wordNum;
	    }
	    if (tb->str[j + 1] != ' ')  {
	      assert(totalWidth < w);
	      words[wordNum].start = j + 1;
	      words[wordNum].xoff = totalWidth;
	      words[wordNum].yoff = txtH * (numLines - 1);
	      words[wordNum].w = 0;
	    }
	  } else  {
	    if (tb->str[j] != ' ')
	      nonSpaceSeen = TRUE;
	    totalWidth += widths[j];
	    words[wordNum].w += widths[j];
	    if ((uchar)tb->str[j] < BUTWRITE_MINPRINT)
	      ++j;
	  }
	  ++j;
	}
	words[wordNum].len = i - tb->words[wordNum].start;
	while ((tb->str[j] != ' ') && (tb->str[j] != '\0'))
	  words[wordNum].w += widths[j++];
	break;
      case butText_left:
	words[wordNum].w = widthAtLastSpace;
	break;
      }
      ++wordNum;
      while (tb->str[i] == ' ')  {
	++i;
	widths[i] = butEnv_charWidth(env, tb->str+i, tb->font);
      }
      lastSpaceSeen = i;
      widthNonSpace = totalWidth = widths[i];
      spacesThisLine = 0;
      nonSpaceSeen = TRUE;
      prevCharWasSpace = FALSE;
      words[wordNum].start = i;
      words[wordNum].xoff = 0;
      words[wordNum].yoff = txtH * numLines;
      words[wordNum].w = 0;
      ++numLines;

      /*
       * If this was a special character then step over it's second byte.
       */
      if ((uchar)tb->str[i] < BUTWRITE_MINPRINT)  {
	++i;
	widths[i] = 0;
      }
    }
  }
  words[wordNum].len = i - words[wordNum].start;
  if (tb->align == butText_center)
    words[wordNum].xoff += (w - totalWidth) / 2;
  else if (tb->align == butText_right)
    words[wordNum].xoff += (w - totalWidth);
  words[wordNum].w = totalWidth;
  tb->nwords = wordNum + 1;
  
  /* We probably allocated way too many words, so allocate a new block
   *   of exactly the right size and move to it.
   */
  words = wms_malloc(tb->nwords * sizeof(Word));
  memcpy(words, tb->words, tb->nwords * sizeof(Word));
  for (i = 0;  i < tb->nwords;  ++i)  {
    words[i].w = 0;
    for (j = words[i].start;  j < words[i].start+words[i].len;  ++j)
      words[i].w += widths[j];
  }
  wms_free(widths);
  wms_free(tb->words);
  tb->words = words;
}


/* Returns the height of the text block after being resized. */
int  butTblock_resize(But *but, int x, int y, int w)  {
  int  old_w = but->w, h;

  assert(MAGIC(but));
  assert(but->action == &action);
  but->w = w;
  butTblock_setText(but, NULL);
  but->w = old_w;
  but_resize(but, x, y, w, h = butTblock_getH(but));
  return(h);
}


static ButOut  destroy(But *but)  {
  Tb *tb = but->iPacket;
  
  if (tb->str != NULL)
    wms_free(tb->str);
  if (tb->words != NULL)
    wms_free(tb->words);
  wms_free(tb);
  return(0);
}


static void  draw(But *but, int dx, int dy, int dw, int dh)  {
  Tb  *tb = but->iPacket;
  ButEnv  *env = but->win->env;
  int  i, x, y;
  char  temp;
  int  fonth;

  if (but->w != tb->setup_w)
    butTblock_setText(but, NULL);
  fonth = env->fonts[tb->font]->ascent + env->fonts[tb->font]->descent;
  butEnv_setXFg(env, BUT_FG);
  x = but->x;
  y = but->y;
  for (i = 0;  i < tb->nwords;  ++i)  {
    assert((tb->words[i].xoff >= 0) &&
	   (tb->words[i].xoff < but->w) &&
	   (tb->words[i].yoff >= 0) &&
	   (tb->words[i].yoff < but->h));
    assert((tb->words[i].w > 0) && (tb->words[i].w <= but->w));
    if ((x+tb->words[i].xoff < dx+dw) &&
	(y+tb->words[i].yoff < dy+dh) &&
	(x+tb->words[i].xoff + tb->words[i].w >= dx) &&
	(y+tb->words[i].yoff + fonth >= dy))  {
      temp = tb->str[tb->words[i].start + tb->words[i].len];
      tb->str[tb->words[i].start + tb->words[i].len] = '\0';
      butWin_write(but->win, x+tb->words[i].xoff, y+tb->words[i].yoff,
		   tb->str+tb->words[i].start, tb->font);
      tb->str[tb->words[i].start + tb->words[i].len] = temp;
    }
  }
}


int  butTblock_guessH(ButEnv *env, const char *text, int w, int fontNum)  {
  int  curW = 0, curH;
  int  wLastSpace = 0;
  int  fontH;
  int  i;

  fontH = butEnv_fontH(env, fontNum);
  curH = fontH;
  for (i = 0;  text[i];  ++i)  {
    if (text[i] == ' ')  {
      curW += butEnv_charWidth(env, text+i, fontNum);
      wLastSpace = curW;
      if (curW >= w)  {
	while (i+1 == ' ')
	  ++i;
	curH += fontH;
	wLastSpace = curW = 0;
      }
    } else  {
      curW += butEnv_charWidth(env, text+i, fontNum);
      if (text[i] <= '\2')
	++i;
      if (curW > w)  {
	curH += fontH;
	if (wLastSpace)  {
	  curW -= wLastSpace;
	  wLastSpace = 0;
	} else  {
	  /* No spaces, break the word in half. */
	  curW = butEnv_charWidth(env, text+i, fontNum);
	}
      }
    }
  }
  return(curH);
}
      

#endif
