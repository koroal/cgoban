/*
 * src/sgfOut.h, part of Complete Goban (game program)
 * Copyright (C) 1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _SGFOUT_H_
#define  _MOVECHAIN_H_  1

#ifndef  _SGF_H_
#include "sgf.h"
#endif


/**********************************************************************
 * Functions
 **********************************************************************/
/**
 * Write an sgf file out.
 *
 * @param mc The SGF tree.
 * @param fname The file name to save to.
 * @param err Where to write the error code when there is an error.
 * @return TRUE on success, FALSE on failure.
 */
extern bool sgf_writeFile(Sgf *mc, const char *fname, int *err);


#endif  /* _SGFOUT_H_ */
