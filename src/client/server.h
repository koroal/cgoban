/*
 * src/client/server.h, part of Complete Goban (game program)
 * Copyright (C) 1995-1996 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _CLIENT_SERVER_H_
#define  _CLIENT_SERVER_H_  1


/**********************************************************************
 * Data Types
 **********************************************************************/
typedef enum  {
  cliServer_nngs, cliServer_igs
} CliServer;


/**********************************************************************
 * Functions
 **********************************************************************/
/*
 * It seems that gcc has an error.  If no _functions_ in a library file are
 *   called, then the file is not linked in.  Since I want "cliServer_names[]"
 *   to get pulled in, I need to have a function here.  :-(
 * This file no longer has *any* data in it, so I could just scrap it, but
 *   what the hell.
 */
extern void  cliServer_create(void);


#endif  /* _CLIENT_SERVER_H_ */
