/*
 * src/client/match.h, part of Complete Goban (player program)
 * Copyright (C) 1996 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _CLIENT_MATCH_H_

#ifndef  _CLIENT_DATA_H_
#include "data.h"
#endif

#define  _CLIENT_MATCH_H_  1


/**********************************************************************
 * Data Types
 **********************************************************************/
typedef struct CliMatch_struct  CliMatch;

typedef enum  {
  cliMatch_nil, cliMatch_sent, cliMatch_recvd
} CliMatchState;

struct CliMatch_struct  {
  CliMatch  *next, **prev;
  CliData  *data;

  CliMatchState  state;
  ButWin  *win;
  But  *bg;
  But  *title;

  But  *namesBox;
  But  *wBox, *wTitle;
  But  *bBox, *bTitle;
  But  *swap;

  But  *rulesBox;
  But  *sizeStr, *sizeIn, *sizeDown, *sizeUp;
  But  *hcapStr, *hcapIn, *hcapDown, *hcapUp;
  But  *komiStr, *komiIn, *komiDown, *komiUp;
  int  size, hcap;
  float  komi;

  But  *timeBox;
  But  *mainStr, *mainIn, *mainDown1, *mainDown5, *mainUp1, *mainUp5;
  But  *byStr,  *byIn, *byDown1, *byDown5, *byUp1, *byUp5;
  But  *freeGame, *freeTitle;
  int  mainTime, byTime;

  But  *help, *ok, *cancel;

  Str  wName, bName;
  bool  meFirst;

  MAGIC_STRUCT
};


/**********************************************************************
 * Functions
 **********************************************************************/
extern CliMatch  *cliMatch_create(CliData *data, const char *oppName,
				  CliMatch **next, int rankDiff);
extern CliMatch  *cliMatch_matchCommand(CliData *data, const char *command,
					CliMatch **next, int rankDiff);
extern void  cliMatch_declineCommand(CliMatch *match, const char *buf);
extern void  cliMatch_destroy(CliMatch *match);
/*
 * If oppName is non-NULL, then destroyChain will fill in hcap etc. with
 *   the game results if it finds the appropriate match form.
 * It returns TRUE if it does find the right form, FALSE otherwise.
 */
extern bool  cliMatch_destroyChain(CliMatch *match,
				   const char *oppName, int *hcap,
				   float *komi, bool *free);
extern void  cliMatch_extraInfo(CliMatch *match,
				const char *oppName, int hcap,
				float komi, bool free);
  
#endif  /* _CLIENT_MATCH_H_ */
