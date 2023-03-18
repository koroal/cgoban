/*
 * wmslib/src/abut/help.c, part of wmslib library
 * Copyright (C) 1994-1996 William Shubert.
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
#include <but/menu.h>
#include <but/canvas.h>
#ifdef  _ABUT_HELP_H_
  Levelization Error.
#endif
#include "help.h"


/**********************************************************************
 * Forward declarations
 **********************************************************************/
static ButOut  unmap(ButWin *win);
static ButOut  map(ButWin *win);
static ButOut  resize(ButWin *win);
static ButOut  subWinResize(ButWin *win);
static ButOut  destroy(ButWin *win);
static ButOut  ok(But *but);
static ButOut  newMenuOpt(But *but, int value);


/**********************************************************************
 * Functions
 **********************************************************************/
AbutHelp  *abutHelp_create(Abut *a, const char *title, const char *menuTitle,
			   const AbutHelpPage *pages, int numPages,
			   int x, int y, int w, int h, int minW, int minH)  {
  AbutHelp  *help;

  help = wms_malloc(sizeof(AbutHelp));
  MAGIC_SET(help);
  help->abut = a;
  help->pages = pages;
  help->maxMenuOpts = 0;
  help->menuOpts = NULL;
  help->curPage = 0;
  help->numPages = 0;
  help->win = butWin_create(help, a->env, title, w, h,
			    unmap, map, resize, destroy);
  if (x != BUT_NOCHANGE)  {
    butWin_setX(help->win, x);
    butWin_setY(help->win, y);
  }
  if (minW != BUT_NOCHANGE)
    butWin_setMinW(help->win, minW);
  if (minH != BUT_NOCHANGE)
    butWin_setMinH(help->win, minH);
  butWin_setMaxW(help->win, 0);
  butWin_setMaxH(help->win, 0);
  butWin_activate(help->win);
  help->bg = butBoxFilled_create(help->win, 0, BUT_DRAWABLE);
  if (a->ulPixmap != None)
    butBoxFilled_setPixmaps(help->bg, a->ulPixmap, a->lrPixmap, a->bgPixmap);
  help->menu = NULL;
  help->swin = abutSwin_create(help, help->win, 1, ABUTSWIN_LSLIDE,
			       subWinResize);
  help->swinW = 0;
  help->swinH = 0;
  help->subBg = butPlain_create(help->swin->win, 0, BUT_DRAWABLE, BUT_BG);
  help->ok = butCt_create(ok, NULL, help->win, 1,
			  BUT_DRAWABLE|BUT_PRESSABLE, a->ok);
  help->numTextButs = 0;
  help->maxTextButs = 0;
  help->textButs = NULL;
  help->destroyCallback = NULL;
  help->packet = NULL;
  abutHelp_newPages(help, menuTitle, pages, numPages);
  return(help);
}


void  abutHelp_destroy(AbutHelp *help)  {
  assert(MAGIC(help));
  butWin_destroy(help->win);
}


void  abutHelp_setDestroyCallback(AbutHelp *help,
				  ButOut (*callback)(AbutHelp *help,
						     void *packet),
				  void *packet)  {
  assert(MAGIC(help));
  help->destroyCallback = callback;
  help->packet = packet;
}


void  abutHelp_newPages(AbutHelp *help, const char *menuTitle,
			const AbutHelpPage *pages, int numPages)  {
  int  i, bw;

  assert(MAGIC(help));
  if (help->menu != NULL)  {
    but_destroy(help->menu);
    help->menu = NULL;
  }
  help->pages = pages;
  if (help->maxMenuOpts < numPages + 1)  {
    if (help->menuOpts != NULL)
      wms_free(help->menuOpts);
    help->maxMenuOpts = numPages + 1;
    help->menuOpts = wms_malloc(help->maxMenuOpts * sizeof(const char *));
  }
  for (i = 0;  i < numPages;  ++i)  {
    if (pages[i].pageName == NULL)
      help->menuOpts[i] = BUTMENU_OLBREAK;
    else
      help->menuOpts[i] = pages[i].pageName;
  }
  help->menuOpts[i] = BUTMENU_OLEND;
  help->menu = butMenu_downCreate(newMenuOpt, help, help->win, 1, 2,
				  BUT_DRAWABLE|BUT_PRESSABLE,
				  menuTitle, help->menuOpts, 0);
  bw = butEnv_stdBw(butWin_env(help->win));
  but_resize(help->menu, bw*2, bw*2,
	     butWin_w(help->win) - bw*4, help->abut->butH * 2);
  abutHelp_newPage(help, 0);
}


void  abutHelp_newPage(AbutHelp *help, int pageNum)  {
  int  i;
  int  numTbs;

  assert(MAGIC(help));
  abutSwin_vMove(help->swin, 0);
  help->curPage = pageNum;
  for (i = 0;  i < help->numTextButs;  ++i)
    but_destroy(help->textButs[i]);
  for (numTbs = 0;  help->pages[pageNum].text[numTbs].text;  ++numTbs);
  if (numTbs > help->maxTextButs)  {
    if (help->textButs)
      wms_free(help->textButs);
    help->textButs = wms_malloc(numTbs * sizeof(But *));
    help->maxTextButs = numTbs;
  }
  help->numTextButs = numTbs;
  for (i = 0;  i < numTbs;  ++i)  {
    help->textButs[i] = butTblock_create(help->swin->win, 1, BUT_DRAWABLE,
					 help->pages[pageNum].text[i].text,
					 help->pages[pageNum].text[i].align);
    butTblock_setFont(help->textButs[i], help->pages[pageNum].text[i].fontNum);
  }
  help->swinW = 0;
  subWinResize(help->swin->win);
}


static ButOut  unmap(ButWin *win)  {
  return(0);
}


static ButOut  map(ButWin *win)  {
  return(0);
}


static ButOut  resize(ButWin *win)  {
  AbutHelp  *help = butWin_packet(win);
  int  w, h;
  int  bw = butEnv_stdBw(butWin_env(win));
  int  physFontH = butEnv_fontH(butWin_env(win), 0);

  assert(MAGIC(help));
  w = butWin_w(win);
  h = butWin_h(win);
  but_resize(help->bg, 0, 0, w, h);
  but_resize(help->menu, bw*2, bw*2, w - bw*4, help->abut->butH * 2);
  abutSwin_resize(help->swin, bw*2, bw*3 + help->abut->butH * 2,
		  w - bw*4, h - help->abut->butH * 3 - bw*6,
		  (physFontH * 3) / 2, physFontH);
  butCan_resizeWin(help->swin->win, 0, butWin_w(help->swin->win), TRUE);
  but_resize(help->ok, bw*2, h - help->abut->butH - bw*2,
	     w - bw*4, help->abut->butH);
  return(0);
}


static ButOut  destroy(ButWin *win)  {
  AbutHelp  *help = butWin_packet(win);
  ButOut  result = 0;

  assert(MAGIC(help));
  if (help->destroyCallback)
    result = help->destroyCallback(help, help->packet);
  abutSwin_destroy(help->swin);
  MAGIC_UNSET(help);
  wms_free(help);
  return(result);
}


static ButOut  subWinResize(ButWin *win)  {
  AbutSwin  *swin;
  AbutHelp  *help;
  int  i, bw, w, h, y;

  swin = butWin_packet(win);
  assert(MAGIC(swin));
  help = swin->packet;
  assert(MAGIC(help));
  w = butWin_w(win);
  h = butWin_h(win);
  but_resize(help->subBg, 0, 0, w, h);
  help->swinW = w;
  bw = butEnv_stdBw(butWin_env(win));
  y = bw;
  for (i = 0;  i < help->numTextButs;  ++i)
    y += butTblock_resize(help->textButs[i], bw, y, w - bw*2);
  if ((help->swinH != y + bw) ||
      (help->swinH != h))  {
    help->swinH = y + bw;
    butCan_resizeWin(help->swin->win, help->swinW, help->swinH, TRUE);
  }
  return(0);
}


static ButOut  ok(But *but)  {
  butWin_destroy(but_win(but));
  return(0);
}


static ButOut  newMenuOpt(But *but, int value)  {
  AbutHelp  *help = but_packet(but);

  assert(MAGIC(help));
  if (value != help->curPage)  {
    abutHelp_newPage(help, value);
  }
  return(0);
}
