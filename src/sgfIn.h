/*
 * src/sgfIn.h, part of Complete Goban (game program)
 * Copyright (C) 1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _SGFIN_H_
#define  _SGFIN_H_  1

#ifndef  _SGF_H_
#include "sgf.h"
#endif


/**********************************************************************
 * Functions
 **********************************************************************/
extern Sgf  *sgf_createFile(Cgoban *cg, const char *fName, const char **err,
			    bool *noFile);

#endif  /* _SGFIN_H_ */
