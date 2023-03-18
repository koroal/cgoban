/*
 * src/help.h, part of wmslib library
 * Copyright (C) 1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 */

#ifndef  _ABUT_HELP_H_
#define  _ABUT_HELP_H_  1

#ifndef  _WMS_H_
#include <wms.h>
#endif
#ifndef  _BUT_BUT_H_
#include <but/but.h>
#endif
#ifndef  _BUT_TEXT_H_
#include <but/text.h>
#endif
#ifndef  _ABUT_ABUT_H_
#include <abut/abut.h>
#endif
#ifndef  _ABUT_SWIN_H_
#include <abut/swin.h>
#endif


/**********************************************************************
 * Types
 **********************************************************************/
typedef struct AbutHelpText_struct  {
  ButTextAlign  align;
  int  fontNum;
  const char *text;
} AbutHelpText;


typedef struct AbutHelpPage_struct  {
  const char  *pageName;
  const AbutHelpText  *text;
} AbutHelpPage;


typedef struct AbutHelp_struct  {
  Abut  *abut;
  const AbutHelpPage  *pages;
  int  maxMenuOpts, curPage;
  const char  **menuOpts;
  int  numPages;
  ButWin  *win;
  AbutSwin  *swin;
  int  swinW, swinH;
  But  *bg, *menu, *ok;
  int  numTextButs, maxTextButs;
  But  *subBg, **textButs;
  ButOut  (*destroyCallback)(struct AbutHelp_struct *help, void *packet);
  void  *packet;

  MAGIC_STRUCT
} AbutHelp;


/**********************************************************************
 * Functions
 **********************************************************************/
extern AbutHelp  *abutHelp_create(Abut *a, const char *title,
				  const char *menuTitle,
				  const AbutHelpPage *pages, int numPages,
				  int x, int y, int w, int h,
				  int minW, int minH);
extern void  abutHelp_destroy(AbutHelp *help);
extern void  abutHelp_newPages(AbutHelp *help, const char *menuTitle,
			       const AbutHelpPage *pages, int numPages);
extern void  abutHelp_setDestroyCallback(AbutHelp *help,
					 ButOut (*callback)(AbutHelp *help,
							    void *packet),
					 void *packet);
extern void  abutHelp_newPage(AbutHelp *help, int pageNum);


#endif  /* _ABUT_HELP_H_ */
