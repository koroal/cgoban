/*
 * src/gmp/setup.h, part of Complete Goban (game program)
 * Copyright (C) 1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _GMP_SETUP_H_
#define  _GMP_SETUP_H_  1


/**********************************************************************
 * Data types
 **********************************************************************/

typedef enum  {
  gmpSetup_program, gmpSetup_device, gmpSetup_human, gmpSetup_nngs,
  gmpSetup_igs, gmpSetup_local, gmpSetup_remote
} GmpSetupType;


typedef struct  GmpSetupPlayer_struct  {
  GmpSetupType  type;
  ClpEntry  *optionEntry, *progNameEntry, *devNameEntry;
  Str  progName, devName;
  But  *box;
  But  *typeMenu;
  But  *strDesc1, *strIn1;
  But  *strDesc2, *strIn2;
} GmpSetupPlayer;


typedef struct GmpSetup_struct  {
  Cgoban  *cg;

  /* TRUE if you are waiting for a callback from gameSetup. */
  bool  gameWaiting;

  ButWin  *win;
  But  *bg;
  But  *title;
  But  *ok, *help, *cancel;

  GmpSetupPlayer  players[2];

  MAGIC_STRUCT
} GmpSetup;


/**********************************************************************
 * Functions
 **********************************************************************/
extern GmpSetup  *gmpSetup_create(Cgoban *cg);
extern void  gmpSetup_destroy(GmpSetup *gs);
extern int  gmp_forkProgram(Cgoban *cg, int *inFile, int *outFile,
			    const char *progName, int mainTime, int byTime);


#endif  /* _GMP_SETUP_H_ */
