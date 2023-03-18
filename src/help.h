/*
 * src/msg.h, part of Complete Goban (game program)
 * Copyright (C) 1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _HELP_H_
#define  _HELP_H_  1

#ifndef  _ABUT_HELP_H_
#include <abut/help.h>
#endif


/**********************************************************************
 * Data Types
 **********************************************************************/
typedef struct  {
  int  numPages;
  const char  *menuTitle;
  const AbutHelpPage  *pages;
} Help;


/**********************************************************************
 * Global variables
 **********************************************************************/

extern Help  help_control;
extern Help  help_gameSetup;
extern Help  help_editTool;
extern Help  help_cliMain;
extern Help  help_cliMatch;
extern Help  help_editBoard;
extern Help  help_localBoard;
extern Help  help_cliBoard;
extern Help  help_cliSetup;
extern Help  help_gmpBoard;
extern Help  help_gmpSetup;
extern Help  help_configure;
extern Help  help_cliLook;

#endif  /* _HELP_H_ */
