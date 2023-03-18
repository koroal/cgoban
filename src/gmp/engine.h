/*
 * src/gmp.h, part of Complete Goban (game program)
 * Copyright (C) 1995-1997 William Shubert.
 * See "configure.h.in" for more copyright information.
 */

#ifndef  _GMP_ENGINE_H_

#ifndef  _BUT_BUT_H_
#include <but/but.h>
#endif
#ifndef  _GOBOARD_H_
#include "../goBoard.h"
#endif
#ifndef  _CGOBAN_H_
#include "../cgoban.h"
#endif

#ifdef  _GMP_ENGINE_H_
#error  Levelization Error.
#endif
#define  _GMP_ENGINE_H_  1


/**********************************************************************
 * Constants
 **********************************************************************/

#define  GMP_SENDBUFSIZE  16
#define  GMP_EXECVEFAILED  127


/**********************************************************************
 * Data types
 **********************************************************************/

typedef struct GmpEngine_struct  GmpEngine;


typedef struct GmpActions_struct  {
  ButOut  (*newGame)(GmpEngine *ge, void *packet, int size, int handicap,
		   float komi, bool chineseRules, bool iAmWhite);
  ButOut  (*moveRecvd)(GmpEngine *ge, void *packet, GoStone color,
		       int x, int y);
  ButOut  (*undoMoves)(GmpEngine *ge, void *packet, int numUndos);
  ButOut  (*errorRecvd)(GmpEngine *ge, void *packet, const char *errStr);
} GmpActions;


struct GmpEngine_struct  {
  bool  stopped;  /* Prevents you from reading stuff after the game ends. */
  int  inFile, outFile;
  ButTimer  *heartbeat;
  int  boardSize;
  int  handicap;
  float  komi;
  bool  chineseRules;
  bool  iAmWhite;  /* send white's moves out, get black's moves in. */
  int  queriesAcked, lastQuerySent;
  bool  gameStarted;

  int  recvSoFar, sendsQueued, sendFailures;
  bool  waitingHighAck;
  time_t  lastSendTime;
  int  myLastSeq, hisLastSeq;
  unsigned char  recvData[4];
  /* Store up 1024 cmds.  Should be enough. */
  unsigned char  sendData[4];
  struct  {
    int  cmd, val;
  } sendsPending[GMP_SENDBUFSIZE];

  Cgoban *cg;
  ButEnv *env;
  void  *packet;
  const GmpActions  *actions;

  MAGIC_STRUCT
};


/**********************************************************************
 * Fuctions
 **********************************************************************/
extern GmpEngine  *gmpEngine_init(Cgoban *cg, GmpEngine *ge,
				  int inFile, int outFile,
				  const GmpActions *actions,
				  void *packet);
#define gmpEngine_create(e, i, o, a, p)  \
  gmpEngine_init(e, wms_malloc(sizeof(GmpEngine)), i, o, a, p)

#define  gmpEngine_stop(e)  ((e)->stopped = TRUE)

extern GmpEngine  *gmpEngine_deinit(GmpEngine *ge);
#define gmpEngine_destroy(ge)  wms_free(gmpEngine_deinit(ge))

extern void  gmpEngine_startGame(GmpEngine *ge, int size, int handicap,
				 float komi, bool chineseRules, bool iAmWhite);
extern void  gmpEngine_sendPass(GmpEngine *ge);
extern void  gmpEngine_sendMove(GmpEngine *ge, int x, int y);
extern void  gmpEngine_sendUndo(GmpEngine *ge, int numUndos);
extern int  gmp_forkProgram(Cgoban *cg, int *inFile, int *outFile,
			    const char *progName, int mainTime, int byTime);


#endif  /* _GMP_ENGINE_H_ */
