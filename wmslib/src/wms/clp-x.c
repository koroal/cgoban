/*
 * wmslib/src/wms/clp-x.c, part of wmslib (Library functions)
 * Copyright (C) 1995-1996 William Shubert.
 * See "configure.h.in" for more copyright information.
 *
 * Command Line Parser.
 */

#include <wms.h>
#if  X11_DISP
#include <wms/str.h>
#include <X11/Xlib.h>
#include "clp.h"


/**********************************************************************
 * Functions
 **********************************************************************/
void  clp_rXDefaults(Clp *clp, Display *dpy, char *name)  {
  int  i, j;
  ClpEntry  *ci;
  char  *xdef, wname[80];

  for (i = 0;  i < clp->numInfos;  ++i)  {
    ci = &clp->infos[i];
    if (!ci->name || (ci->where == clpWhere_cmdline))
      continue;
    strncpy(wname, ci->name, sizeof(wname) - 1);
    wname[sizeof(wname) - 1] = '\0';
    for (j = 0;  wname[j];  ++j)
      if (wname[j] == ',')
	wname[j] = '\0';
    xdef = XGetDefault(dpy, name, wname);
    if (xdef != NULL)  {
      ci->where = clpWhere_xdefs;
      ci->type = clpDtype_string;
      clpEntry_setStr(ci, xdef);
    }
  }
}

#endif  /* X11_DISP */
