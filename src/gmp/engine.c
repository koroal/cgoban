/*
 * src/gmp/engine.c, part of Complete Goban (game program)
 * Copyright (C) 1995-1997 William Shubert.
 * See "configure.h.in" for more copyright information.
 */

/*
 * This is based on David Fotland's go modem protocol code.  I've pretty much
 *   completely rewritten it though.
 */

#include <wms.h>
#include <but/but.h>
#include <but/timer.h>
#include <wms/str.h>
#include "../cgoban.h"
#include "../goBoard.h"
#include "../msg.h"
#ifdef  _GMP_ENGINE_H_
#error  Levelization Error.
#endif
#include "engine.h"


/**********************************************************************
 * Constants
 **********************************************************************/

#define  GMP_NUMQUERIESTOASK  4

#define  GMP_TIMEOUTSECS  60


/**********************************************************************
 * Data types
 **********************************************************************/
typedef enum  {
  cmd_ack, cmd_deny, cmd_reset, cmd_query, cmd_respond, cmd_move,
  cmd_undo
} Command;


typedef enum  {
  query_game, query_bufSize, query_protocol, query_stones,
  query_bTime, query_wTime, query_charSet, query_rules, query_handicap,
  query_boardSize, query_timeLimit, query_color, query_who
} Query;


/**********************************************************************
 * Globals
 **********************************************************************/
#if  DEBUG
static const char *cmdNames[] = {
  "ACK", "DENY", "RESET", "QUERY", "RESPOND", "MOVE", "UNDO"};

static bool  showTransfers = TRUE;
#endif


/**********************************************************************
 * Forward Declarations
 **********************************************************************/
static ButOut  dataReadyForRecv(void *packet, int fd);
static ButOut  getPacket(GmpEngine *ge);
static unsigned char  checksum(unsigned char p[4]);
static ButOut  parsePacket(GmpEngine *ge);
static ButOut  processCommand(GmpEngine *ge, Command command, int val);
static ButOut  gotQueryResponse(GmpEngine *ge, int val);
static ButOut  putCommand(GmpEngine *ge, Command cmd, int val);
static ButOut  heartbeat(ButTimer *timer);
static void  respond(GmpEngine *ge, Query query);
static void  askQuery(GmpEngine *ge);
static void  processQ(GmpEngine *ge);
static char  **makeArgv(const char *cmdLine);


/**********************************************************************
 * Functions
 **********************************************************************/

GmpEngine  *gmpEngine_init(Cgoban *cg, GmpEngine *ge,
			   int inFile, int outFile,
			   const GmpActions *actions, void *packet)  {
  struct timeval  oneSecond;

  MAGIC_SET(ge);
  ge->stopped = FALSE;
  ge->inFile = inFile;
  ge->outFile = outFile;
  ge->boardSize = 0;
  ge->handicap = 0;
  ge->komi = 0.0;
  ge->chineseRules = TRUE;
  ge->iAmWhite = 0;
  ge->queriesAcked = 0;
  ge->lastQuerySent = 0;
  
  ge->recvSoFar = 0;
  ge->sendsQueued = 0;
  ge->sendFailures = 0;
  ge->waitingHighAck = FALSE;
  ge->lastSendTime = 0;
  ge->myLastSeq = 0;
  ge->hisLastSeq = 0;

  ge->cg = cg;
  ge->env = cg->env;
  ge->packet = packet;
  ge->actions = actions;

  butEnv_addFile(cg->env, BUT_READFILE, inFile, ge, dataReadyForRecv);
  oneSecond.tv_usec = 0;
  oneSecond.tv_sec = 1;
  ge->heartbeat = butTimer_create(ge, NULL, oneSecond, oneSecond,
				  FALSE, heartbeat);
  return(ge);
}


GmpEngine  *gmpEngine_deinit(GmpEngine *ge)  {
  assert(MAGIC(ge));
  butEnv_rmFile(ge->env, BUT_READFILE, ge->inFile);
  butTimer_destroy(ge->heartbeat);
  MAGIC_UNSET(ge);
  return(ge);
}


static ButOut  dataReadyForRecv(void *packet, int fd)  {
  GmpEngine  *ge = packet;

  assert(MAGIC(ge));
  assert(fd == ge->inFile);
  return(getPacket(ge));
}


static ButOut  getPacket(GmpEngine *ge)  {
  unsigned char  charsIn[4], c;
  int  count = 0, cNum;
  Str  errDesc;
  ButOut  result = 0;

  count = read(ge->inFile, charsIn, 4 - ge->recvSoFar);
  if (ge->stopped)
    return(0);
  if (count < 0)  {
    str_init(&errDesc);
    str_print(&errDesc, msg_gmpDead,
	      strerror(errno));
    ge->actions->errorRecvd(ge, ge->packet, str_chars(&errDesc));
    str_deinit(&errDesc);
    return(BUTOUT_ERR);
  } else if (count == 0)
    return(0);

  for (cNum = 0;  cNum < count;  ++cNum)  {
    c = charsIn[cNum];
    if (!ge->recvSoFar)  {
      /* idle, looking for start of packet */
      if ((c & 0xfc) == 0)  {  /* start of packet */
	ge->recvData[0] = c;
	ge->recvSoFar = 1;
      }
    } else  {
      /* We're in the packet. */
      if ((c & 0x80) == 0)  {  /* error */
	ge->recvSoFar = 0;
	if ((c & 0xfc) == 0)  {
	  ge->recvData[ge->recvSoFar++] = c;
	}
      } else  {
	/* A valid character for in a packet. */
	ge->recvData[ge->recvSoFar++] = c;
	if (ge->recvSoFar == 4)  {  /* check for extra bytes */
	  assert(cNum + 1 == count);
	  if (checksum(ge->recvData) == ge->recvData[1])
	    result = parsePacket(ge);
	  ge->recvSoFar = 0;
	}
      }
    }
  }
  return(result);
}


static unsigned char  checksum(unsigned char p[4])  {
  unsigned char sum;
  sum = p[0] + p[2] + p[3];       
  sum |= 0x80;  /* set sign bit */
  return(sum);
}


static ButOut  parsePacket(GmpEngine *ge)  {
  int  seq, ack, val;
  Command  command;
  ButOut  result = 0;

  seq = ge->recvData[0] & 1;
  ack = (ge->recvData[0] & 2) >> 1;
  if (ge->recvData[2] & 0x08)  /* Not-understood command. */
    return(0);
  command = (ge->recvData[2] >> 4) & 7;
#if DEBUG
  if (showTransfers) {
    fprintf(stderr, "GMP: Command %s received from port %d:%d.\n",
	    cmdNames[command], ge->inFile, ge->outFile);
  }
#endif
  if ((command != cmd_deny) && (command != cmd_reset) &&
      !ge->iAmWhite && !ge->gameStarted) {
    ge->gameStarted = TRUE;
    if (ge->actions->newGame != NULL)
      ge->actions->newGame(ge, ge->packet, ge->boardSize, ge->handicap,
			   ge->komi, ge->chineseRules, ge->iAmWhite);
  }
  val = ((ge->recvData[2] & 7) << 7) | (ge->recvData[3] & 0x7f);
  if (!ge->waitingHighAck)  {
    if ((command == cmd_ack) ||  /* An ack.  We don't need an ack now. */
	(ack != ge->myLastSeq))  {  /* He missed my last message. */
#if DEBUG
      if (showTransfers) {
	fprintf(stderr, "GMP: ACK does not match myLastSeq.\n");
      }
#endif
      return(0);
    } else if (seq == ge->hisLastSeq)  {  /* Seen this one before. */
#if DEBUG
      if (showTransfers) {
	fprintf(stderr, "GMP: Repeated packet.\n");
      }
#endif
      putCommand(ge, cmd_ack, ~0);
    } else  {
      ge->hisLastSeq = seq;
      ge->sendFailures = 0;
      result = processCommand(ge, command, val);
    }
  } else  {
    /* Waiting for OK. */
    if (command == cmd_ack)  {
      if ((ack != ge->myLastSeq) || (seq != ge->hisLastSeq))  {
	/* Sequence error. */
	return(result);
      }
      ge->sendFailures = 0;
      ge->waitingHighAck = FALSE;
      processQ(ge);
    } else if (seq == ge->hisLastSeq)  {
      /* His command is old. */
    } else if (ack == ge->myLastSeq)  {
      ge->sendFailures = 0;
      ge->waitingHighAck = FALSE;
      ge->hisLastSeq = seq;
      result = processCommand(ge, command, val);
      processQ(ge);
    } else  {
      /* Conflict with opponent. */
#if DEBUG
      if (showTransfers) {
	fprintf(stderr, "GMP: Received a packet, expected an ACK.\n");
      }
#endif
      /*
       * This code seems wrong.
       *
       * ge->myLastSeq = 1 - ge->myLastSeq;
       * ge->waitingHighAck = FALSE;
       * processQ(ge);
       */
    }
  }
  return(result);
}


static ButOut  processCommand(GmpEngine *ge, Command command, int val)  {
  int  s, x, y;
  ButOut  result = 0;

  switch(command)  {
  case cmd_deny:
    return(putCommand(ge, cmd_ack, ~0));
    break;
  case cmd_query:
    respond(ge, val);
    break;
  case cmd_reset:  /* New game. */
    ge->queriesAcked = 0;
    askQuery(ge);
    break;
  case cmd_undo:  /* Take back moves. */
    result = putCommand(ge, cmd_ack, ~0);
    assert(ge->actions->undoMoves != NULL);
    result |= ge->actions->undoMoves(ge, ge->packet, val);
    return(result);
    break;
  case cmd_move:
    s = val & 0x1ff;
    if (s == 0)  {
      x = -1;
      y = 0;
    } else  {
      --s;
      x = (s % ge->boardSize);
      y = ge->boardSize - 1 - (s / ge->boardSize);
    }
    result = putCommand(ge, cmd_ack, ~0);
    if (val & 0x200)
      result |= ge->actions->moveRecvd(ge, ge->packet, goStone_white, x, y);
    else
      result |= ge->actions->moveRecvd(ge, ge->packet, goStone_black, x, y);
    return(result);
    break;
  case cmd_respond:
    return(gotQueryResponse(ge, val));
    break;
  default:  /* Don't understand command. */
    return(putCommand(ge, cmd_deny, 0));
    break;
  }
  return(0);
}


static ButOut  putCommand(GmpEngine *ge, Command cmd, int val)  {
  int  writeResult;

  if (ge->waitingHighAck &&
      (cmd != cmd_ack) && (cmd != cmd_respond) && (cmd != cmd_deny))  {
    if (ge->sendsQueued < GMP_SENDBUFSIZE)  {
      ge->sendsPending[ge->sendsQueued].cmd = cmd;
      ge->sendsPending[ge->sendsQueued].val = val;
      ++ge->sendsQueued;
    } else  {
      return(ge->actions->errorRecvd(ge, ge->packet,
				     msg_gmpSendBufFull));
    }
    return(0);
  }
  if ((cmd == cmd_ack) && (ge->sendsQueued))  {
    ge->waitingHighAck = FALSE;
    processQ(ge);
    return(0);
  }
  if (cmd != cmd_ack)
    ge->myLastSeq ^= 1;
  ge->sendData[0] = ge->myLastSeq | (ge->hisLastSeq << 1);
  ge->sendData[2] = 0x80 | (cmd << 4) | ((val >> 7) & 7);
  ge->sendData[3] = 0x80 | val;
  ge->sendData[1] = checksum(ge->sendData);
  ge->lastSendTime = time(NULL);
#if DEBUG
  if (showTransfers) {
    fprintf(stderr,
	    "GMP: Writing command %s to port %d:%d.\n",
	    cmdNames[cmd], ge->inFile, ge->outFile);
  }
#endif
  writeResult = write(ge->outFile, ge->sendData, 4);
  assert(writeResult == 4);
  ge->waitingHighAck = (cmd != cmd_ack);
  return(0);
}


static void  respond(GmpEngine *ge, Query query)  {
  int  response;

  if (query & 0x200)  {
    /*
     * Oops!  This really means "do you support this extended command."
     * I don't support any extended commands, so the response should always
     *   be zero.
     */
    response = 0;
    /*
     * * Do you support this query? *
     *
     * query &= ~0x200;
     * if ((query == query_game) || (query == query_rules) ||
     *     (query == query_handicap) || (query == query_boardSize) ||
     *     (query == query_color))
     *   response = 15;  * Yes. *
     * else
     *   response = 0;  * No. *
     */
  } else  {
    ge->waitingHighAck = TRUE;
    switch(query)  {
    case query_game:
      response = 1;  /* GO */
      break;
    case query_rules:
      if (ge->chineseRules)
	response = 2;
      else
	response = 1;
      break;
    case query_handicap:
      response = ge->handicap;
      if (response == 0)
	response = 1;
      break;
    case query_boardSize:
      response = ge->boardSize;
      break;
    case query_color:
      if (ge->iAmWhite)
	response = 1;
      else
	response = 2;
      break;
    default:
      response = 0;
      break;
    }    
  }
  putCommand(ge, cmd_respond, response);
}


static void  askQuery(GmpEngine *ge)  {
  static const Query  queryList[GMP_NUMQUERIESTOASK] = { 
    query_rules, query_handicap, query_boardSize, query_color};
  int  query;

  assert(ge->queriesAcked < GMP_NUMQUERIESTOASK);
  query = queryList[ge->queriesAcked];
  ge->lastQuerySent = query;
  putCommand(ge, cmd_query, query);
}


static ButOut  gotQueryResponse(GmpEngine *ge, int val)  {
  static const char  *ruleNames[] = {"Japanese", "Chinese"};
  static const char  *colorNames[] = {"Black", "White"};
  Str  err;
  ButOut  result = 0;

  switch(ge->lastQuerySent)  {
  case query_handicap:
    if (val <= 1)
      --val;
    if ((val != -1) && (val != ge->handicap))  {
      str_init(&err);
      str_print(&err, "Handicaps do not agree; I want %d, he wants %d.",
		ge->handicap, val);
      result |= ge->actions->errorRecvd(ge, ge->packet, str_chars(&err));
      str_deinit(&err);
    }
    break;
  case query_boardSize:
    assert(ge->boardSize != 0);
    if ((val != 0) && (val != ge->boardSize))  {
      str_init(&err);
      str_print(&err, "Board sizes do not agree; I want %d, he wants %d.",
		ge->boardSize, val);
      result |= ge->actions->errorRecvd(ge, ge->packet, str_chars(&err));
      str_deinit(&err);
    }
    break;
  case query_rules:
    if (val != 0)  {
      if (ge->chineseRules != (val == 2))  {
	str_init(&err);
	str_print(&err, "Rule sets do not agree; I want %s, he wants %s.",
		  ruleNames[ge->chineseRules], ruleNames[val == 2]);
	result |= ge->actions->errorRecvd(ge, ge->packet, str_chars(&err));
	str_deinit(&err);
      }
    }
    break;
  case query_color:
    if (val != 0)  {
      if (ge->iAmWhite == (val == 1))  {
	str_init(&err);
	str_print(&err, "Colors sets do not agree; we both want to be %s.",
		  colorNames[ge->iAmWhite]);
	result |= ge->actions->errorRecvd(ge, ge->packet, str_chars(&err));
	str_deinit(&err);
      }
    }
  }
  if ((result & BUTOUT_ERR) == 0) {
    ++ge->queriesAcked;
    if (ge->queriesAcked < GMP_NUMQUERIESTOASK)
      askQuery(ge);
    else {
      result |= putCommand(ge, cmd_ack, ~0);
      if (ge->actions->newGame != NULL)
	ge->actions->newGame(ge,
			     ge->packet,
			     ge->boardSize,
			     ge->handicap,
			     ge->komi,
			     ge->chineseRules,
			     ge->iAmWhite);
    }
  }
  return(result);
}


static ButOut heartbeat(ButTimer *timer)  {
  GmpEngine *ge = butTimer_packet(timer);
  int  writeResult;

  assert(MAGIC(ge));
  if (ge->waitingHighAck && (time(NULL) != ge->lastSendTime))  {
    if (++ge->sendFailures > GMP_TIMEOUTSECS)  {
      return(ge->actions->errorRecvd(ge, ge->packet,
				     msg_gmpTimeout));
    } else  {
      ge->lastSendTime = time(NULL);
#if DEBUG
      if (showTransfers) {
	fprintf(stderr,
		"GMP: Writing command %s to port %d:%d (retry %d).\n",
		cmdNames[(ge->sendData[2] >> 4) & 7], ge->inFile, ge->outFile,
		ge->sendFailures);
      }
#endif
      writeResult = write(ge->outFile, ge->sendData, 4);
      if (writeResult != 4) {
	return(BUTOUT_ERR);
      }
    }
  }
  return(0);
}


void  gmpEngine_startGame(GmpEngine *ge, int size, int handicap, float komi,
			  bool chineseRules, bool iAmWhite)  {
  ge->boardSize = size;
  ge->handicap = handicap;
  ge->komi = komi;
  ge->chineseRules = chineseRules;
  ge->iAmWhite = iAmWhite;
  ge->gameStarted = FALSE;
  if (!iAmWhite) {
    putCommand(ge, cmd_reset, 0);
  }
}


void  gmpEngine_sendPass(GmpEngine *ge)  {
  int  arg;

  assert(MAGIC(ge));
  if (ge->iAmWhite)
    arg = 0x200;
  else
    arg = 0;
  putCommand(ge, cmd_move, arg);
}


void  gmpEngine_sendMove(GmpEngine *ge, int x, int y)  {
  int  val;

  assert(MAGIC(ge));
  val = x + ge->boardSize * (ge->boardSize - 1 - y) + 1;
  if (ge->iAmWhite)
    val |= 0x200;
  putCommand(ge, cmd_move, val);
}


void  gmpEngine_sendUndo(GmpEngine *ge, int numUndos)  {
  assert(MAGIC(ge));
  putCommand(ge, cmd_undo, numUndos);
}


static void  processQ(GmpEngine *ge)  {
  int  i;

  if (!ge->waitingHighAck && ge->sendsQueued)  {
    putCommand(ge, ge->sendsPending[0].cmd, ge->sendsPending[0].val);
    --ge->sendsQueued;
    for (i = 0;  i < ge->sendsQueued;  ++i)  {
      ge->sendsPending[i] = ge->sendsPending[i + 1];
    }
  }
}


int  gmp_forkProgram(Cgoban *cg, int *inFile, int *outFile,
		     const char *progName, int mainTime, int byTime)  {
  int  pid, result, fd;
  int  cgToProg[2], progToCg[2];
  int  i;
  char  **argv;
  Str  cmdLine;

  result = pipe(cgToProg);
  if (result)
    return(-1);
  result = pipe(progToCg);
  if (result)  {
    close(cgToProg[0]);
    close(cgToProg[1]);
    return(-1);
  }
  pid = fork();
  if (pid < 0)  {
    close(cgToProg[0]);
    close(cgToProg[1]);
    close(progToCg[0]);
    close(progToCg[1]);
    return(-1);
  }  
  if (pid == 0)  {
    /* The child. */
    for (fd = 0;  fd < 100;  ++fd)  {
      if ((fd != 2) &&  /* Don't close stderr! */
	  (fd != cgToProg[0]) && (fd != progToCg[1]))
	close(fd);
    }
    if (dup(cgToProg[0]) != 0)  {
      fprintf(stderr, "I couldn't fdup into stdin!\n");
      exit(1);
    }
    if (dup(progToCg[1]) != 1)  {
      fprintf(stderr, "I couldn't fdup into stdout!\n");
      exit(1);
    }
    str_init(&cmdLine);
    str_print(&cmdLine, progName, mainTime, byTime);
    argv = makeArgv(str_chars(&cmdLine));
    i = execve(argv[0], argv, cg->envp);
    /*
     * If we got here, then the execve() must have failed.
     * No need to clean up, free cmdLine, etc...we're dead!
     */
    exit(GMP_EXECVEFAILED);
  } else  {
    close(cgToProg[0]);
    close(progToCg[1]);
    *inFile = progToCg[0];
    *outFile = cgToProg[1];
    return(pid);
  }
}


static char  **makeArgv(const char *cmdLine)  {
  char  *buf;
  char  **args;
  int  i, numWords, len;

  len = strlen(cmdLine);
  buf = wms_malloc(len + 1);
  strcpy(buf, cmdLine);
  numWords = 1;
  for (i = 1;  i < len;  ++i)  {
    if ((buf[i - 1] == ' ') && (buf[i] != ' '))
      ++numWords;
  }
  args = wms_malloc((numWords + 1) * sizeof(char *));
  args[0] = buf;
  numWords = 1;
  for (i = 1;  i < len;  ++i)  {
    if (buf[i] == ' ')
      buf[i] = '\0';
    if ((buf[i - 1] == '\0') && (buf[i] != '\0'))  {
      args[numWords++] = buf + i;
    }
  }
  args[numWords++] = NULL;
  return(args);
}
