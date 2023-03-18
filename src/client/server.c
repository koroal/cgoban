/*
 * src/cliServer.c, part of Complete Goban (game program)
 * Copyright (C) 1995-1996 William Shubert
 * See "configure.h.in" for more copyright information.
 */

#include <wms.h>
#include "server.h"


/**********************************************************************
 * Functions
 **********************************************************************/
/*
 * It seems that gcc has an error.  If no _functions_ in a library file are
 *   called, then the file is not linked in.  Since I want "cliServer_names[]"
 *   to get pulled in, I need to have a function here.  :-(
 * This file no longer has *any* data in it, so I could just scrap this, but
 *   what the hell.
 */
void  cliServer_create(void)  {
}
