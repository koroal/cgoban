/*
 * $Source: /cvsroot/cgoban1/cgoban1/src/sgfIn.c,v $
 * $Revision: 1.3 $
 * $Author: wmshub $
 * $Date: 2002/05/31 23:40:54 $
 *
 * src/sgfInchain.c, part of Complete Goban (game program)
 * Copyright (C) 1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#include <ctype.h>
#include <wms.h>
#include <wms/rnd.h>
#include <wms/str.h>
#include "cgoban.h"
#include "sgf.h"
#include "sgfIn.h"
#include "msg.h"
#include "goGame.h"


/**********************************************************************
 * Constants
 **********************************************************************/
#define  POOLSIZE  100


/**********************************************************************
 * Forward declarations
 **********************************************************************/
static uint  getToken(FILE *f, const char **arg, int *lineCount);
static const char  *readFile(Sgf *mc, FILE *f, int *lineCount);
static const char  *arg_getLoc(const char *arg, char *loc);
static int  arg_getInt(const char *arg, bool *err);
static double  arg_getDouble(const char *arg, bool *err);
static const char  *arg_getStr(const char *arg, Str *result);
static void  arg_getMove(const char *arg, Sgf *mc, GoStone gval, bool *errOut);
static void  arg_multi(const char *arg, Sgf *mc, SgfType type,
		       GoStone gval, bool *err);
static void  arg_multiRange(const char *arg, Sgf *mc, SgfType type,
			    GoStone gval, bool *err);
static void  arg_multiLabel(const char *arg, Sgf *mc, bool *errOut);


/**********************************************************************
 * Functions
 **********************************************************************/
#define  TOKEN1(c)  ((int)c & 0xff)
#define  TOKEN2(c1, c2)  (((int)c1 & 0xff) | (((int)c2 << 8) & 0xff00))

Sgf  *sgf_createFile(Cgoban *cg, const char *fName, const char **err,
		     bool *noFile)  {
  static Str  strErr;
  static bool  firstTime = TRUE;
  Sgf  *mc;
  SgfElem  *me;
  FILE *f;
  int  c, crCount = 1;
  const char  *problem;
  bool  dummy;

  if (noFile == NULL)
    noFile = &dummy;
  *noFile = FALSE;
  if (firstTime)  {
    str_init(&strErr);
    firstTime = FALSE;
  }
  mc = sgf_create();
  assert(MAGIC(mc));
  mc->mode = sgfInsert_variant;
  f = fopen(fName, "r");
  if (f == NULL)  {
    if (err)  {
      if (errno == ENOENT)
	*noFile = TRUE;
      str_print(&strErr, msg_mcReadErr, strerror(errno), fName);
      *err = str_chars(&strErr);
    }
    sgf_destroy(mc);
    return(NULL);
  }
  /*
   * Start reading after the "(;" bit that marks the first node.
   */
  do  {
    do  {
      c = getc(f);
      if (c == '\n')
	++crCount;
    } while((c != '(') && (c != EOF));
    do  {
      c = getc(f);
      if (c == '\n')
	++crCount;
    } while(isspace(c));
  } while ((c != EOF) && (c != ';'));
  problem = readFile(mc, f, &crCount);
  if (problem)  {
    if (err)  {
      str_print(&strErr, msg_badSGFFile, fName, problem, crCount);
      *err = str_chars(&strErr);
    }
    fclose(f);
    sgf_destroy(mc);
    return(NULL);
  }
  fclose(f);
  mc->mode = sgfInsert_main;
  for (me = &mc->top;  me;  me = me->activeChild)  {
    mc->active = me;
  }
  return(mc);
}


/*
 * Returns FALSE if there was an error.
 */
static const char  *readFile(Sgf *mc, FILE *f, int *crCount)  {
  uint  token;
  const char  *arg;
  SgfElem  *me;
  int  i;
  bool  err = FALSE;
  const char  *problem;
  static Str  strOut;
  static bool  firstTime = TRUE;
  float  komi;

  if (firstTime)  {
    str_init(&strOut);
    firstTime = FALSE;
  }
  for (;;)  {
    token = getToken(f, &arg, crCount);
    switch(token)  {
    case(0):
      return(msg_sgfBadToken);
      break;
    case(TOKEN1(')')):
      return(NULL);
      break;
    case(TOKEN1(';')):
      sgf_addNode(mc);
      break;
    case(TOKEN1('(')):
      me = mc->active;
      problem = readFile(mc, f, crCount);
      if (problem)
	return(problem);
      else  {
	mc->active = me;
	me->activeChild = me->childH;
      }
      break;

    case(TOKEN2('S','Z')):
      i = arg_getInt(arg, &err);
      if ((err) || (i < 2) || (i > GOBOARD_MAXSIZE))
	return(msg_sgfBadArg);
      sgf_setSize(mc, i);
      break;
    case(TOKEN2('R','U')):
      arg_getStr(arg, &strOut);
      for (i = 0;  i < (int)goRules_num;  ++i)  {
	if (!strcmp(str_chars(&strOut), msg_ruleNames[i]))  {
	  break;
	}
      }
      if (i == goRules_num)  {
	sgf_setRules(mc, goRules_japanese);
      } else  {
	sgf_setRules(mc, (GoRules)i);
      }
      break;
    case(TOKEN2('H','A')):
      i = arg_getInt(arg, &err);
      if (err)  {
	return(msg_sgfBadArg);
      }
      sgf_setHandicap(mc, i);
      break;
    case(TOKEN2('K','M')):
      arg_getStr(arg, &strOut);
      if (str_len(&strOut) == 0)  {
	sgf_setKomi(mc, 0.0);
      } else  {
	komi = wms_atof(str_chars(&strOut), &err);
	if (err)  {
	  if ((str_chars(&strOut)[0] == '-') ||
	      isdigit(str_chars(&strOut)[0]))  {
	    komi = atoi(str_chars(&strOut));
	    if (komi >= 0.0)
	      komi += 0.5;
	    else
	      komi -= 0.5;
	  } else  {
	    return(msg_sgfBadArg);
	  }
	}
	sgf_setKomi(mc, komi);
      }
      break;
    case(TOKEN2('T','M')):
      sgf_setTimeFormat(mc, arg_getStr(arg, &strOut));
      break;
    case(TOKEN2('C','P')):
      sgf_copyright(mc, arg_getStr(arg, &strOut));
      break;
    case(TOKEN2('P','W')):
      sgf_setPlayerName(mc, goStone_white, arg_getStr(arg, &strOut));
      break;
    case(TOKEN2('P','B')):
      sgf_setPlayerName(mc, goStone_black, arg_getStr(arg, &strOut));
      break;
    case(TOKEN2('D','T')):
      sgf_addSElem(mc, sgfType_date, arg_getStr(arg, &strOut));
      break;
    case(TOKEN2('G','N')):
      arg_getStr(arg, &strOut);
      if (str_len(&strOut) > 0)
	sgf_addSElem(mc, sgfType_title, str_chars(&strOut));
      break;
    case(TOKEN2('W','R')):
      sgf_playerRank(mc, goStone_white, arg_getStr(arg, &strOut));
      break;
    case(TOKEN2('B','R')):
      sgf_playerRank(mc, goStone_black, arg_getStr(arg, &strOut));
      break;
    case(TOKEN2('E','V')):
      sgf_event(mc, arg_getStr(arg, &strOut));
      break;
    case(TOKEN2('S','O')):
      sgf_source(mc, arg_getStr(arg, &strOut));
      break;
    case(TOKEN2('G','C')):
      sgf_gameComment(mc, arg_getStr(arg, &strOut));
      break;

    case(TOKEN2('P','L')):
      arg_getStr(arg, &strOut);
      if (str_len(&strOut) != 1)  {
	return(msg_sgfBadArg);
      }
      switch(str_chars(&strOut)[0])  {
      case 'B':
      case 'b':
      case '1':
	sgf_setWhoseMove(mc, goStone_black);
	break;
      case 'W':
      case 'w':
      case '2':
	sgf_setWhoseMove(mc, goStone_white);
	break;
      default:
	return(msg_sgfBadArg);
      }
      break;
    case(TOKEN1('W')):
      arg_getMove(arg, mc, goStone_white, &err);
      if (err)
	return(msg_sgfBadLoc);
      break;
    case(TOKEN1('B')):
      arg_getMove(arg, mc, goStone_black, &err);
      if (err)
	return(msg_sgfBadLoc);
      break;
    case(TOKEN2('W','L')):
      i = (int)arg_getDouble(arg, &err);
      if (err)  {
	return(msg_sgfBadArg);
      }
      sgf_timeLeft(mc, goStone_white, i);
      break;
    case(TOKEN2('B','L')):
      i = (int)arg_getDouble(arg, &err);
      if (err)  {
	return(msg_sgfBadArg);
      }
      sgf_timeLeft(mc, goStone_black, i);
      break;
    case(TOKEN2('O','W')):
      i = arg_getInt(arg, &err);
      if (err)  {
	return(msg_sgfBadArg);
      }
      sgf_stonesLeft(mc, goStone_white, i);
      break;
    case(TOKEN2('O','B')):
      i = arg_getInt(arg, &err);
      if (err)  {
	return(msg_sgfBadArg);
      }
      sgf_stonesLeft(mc, goStone_black, i);
      break;

    case(TOKEN2('A','E')):
      arg_multi(arg, mc, sgfType_setBoard, goStone_empty, &err);
      if (err)
	return(msg_sgfBadLoc);
      break;
    case(TOKEN2('A','W')):
      arg_multi(arg, mc, sgfType_setBoard, goStone_white, &err);
      if (err)
	return(msg_sgfBadLoc);
      break;
    case(TOKEN2('A','B')):
      arg_multi(arg, mc, sgfType_setBoard, goStone_black, &err);
      if (err)  {
	return(msg_sgfBadLoc);
      }
      break;
    case(TOKEN2('T','W')):
      arg_multiRange(arg, mc, sgfType_territory, goStone_white, &err);
      if (err)
	return(msg_sgfBadLoc);
      break;
    case(TOKEN2('T','B')):
      arg_multiRange(arg, mc, sgfType_territory, goStone_black, &err);
      if (err)
	return(msg_sgfBadLoc);
      break;

    case(TOKEN2('T','R')):
    case(TOKEN1('M')):      /* Some games have marks as "M[..]". */
      arg_multi(arg, mc, sgfType_triangle, goStone_empty, &err);
      if (err)
	return(msg_sgfBadLoc);
      break;
    case(TOKEN2('C','R')):
      arg_multi(arg, mc, sgfType_circle, goStone_empty, &err);
      if (err)
	return(msg_sgfBadLoc);
      break;
    case(TOKEN2('M','A')):  /* I draw "MA" (mark) as a square.   */
      arg_multi(arg, mc, sgfType_mark, goStone_empty, &err);
      if (err)
	return(msg_sgfBadLoc);
      break;
    case(TOKEN2('S','Q')):
      arg_multi(arg, mc, sgfType_square, goStone_empty, &err);
      if (err)
	return(msg_sgfBadLoc);
      break;
    case(TOKEN2('L','B')):
      arg_multiLabel(arg, mc, &err);
      if (err)
	return(msg_sgfBadLoc);
      break;
    case(TOKEN1('L')):
      /* Obsolete version of the LB token. */
      me = mc->active;
      arg_multi(arg, mc, sgfType_label, goStone_empty, &err);
      if (err)
	return(msg_sgfBadLoc);
      i = 'A';
      while (me != mc->active)  {
	me = me->activeChild;
	me->sVal = str_createChar(i++);
      }
      break;

    case(TOKEN1('C')):
      sgf_comment(mc, arg_getStr(arg, &strOut));
      break;
    case(TOKEN2('R','E')):
      sgf_result(mc, arg_getStr(arg, &strOut));
      break;

    case(TOKEN2('G','M')):
      /*
       * if (arg_getInt(arg, &err) != 1)
       *   return(msg_sgfBadArg);
       */
      break;
    case(TOKEN2('F','F')):
      break;
    case(TOKEN2('P','C')):
      sgf_place(mc, arg_getStr(arg, &strOut));
      break;
    case(TOKEN2('S','Y')):
      sgf_style(mc, arg_getStr(arg, &strOut));
      break;
    default:
      sgf_unknown(mc, arg);
      break;
    }
  }
}

/*
 * Read a position.
 * Called with arg pointing past the [ or :
 * Returns a pointer to : or ] (and NULL on error)
 * The position is returned via posp (NULL for no position).
 */
static const char *arg_getLoc(const char *arg, char *posp) {
  *posp = '\0';
  if (*arg == ']')
    return arg;
  if ((arg[0] == 't') && (arg[1] == 't') && (arg[2] == ']'))
    return arg+2;
  
  if (!islower(arg[0]) || !islower(arg[1]))
    return NULL;
  
  if ((arg[2] == ']') || (arg[2] == ':'))  {
    posp[0] = posp[2] = 'a';
    posp[1] = arg[0];
    posp[3] = arg[1];
    posp[4] = '\0';
    return arg+2;
  }
}

/*
 * Call sgf_move or sgf_pass for a location [xx] or []
 */
static void arg_getMove(const char *arg, Sgf *mc, GoStone gval, bool *errOut) {
  char loc[5];
  
  arg = strchr(arg, '[');
  if (arg)
    arg = arg_getLoc(arg+1, loc);
  if (!arg) {
    *errOut = TRUE;
    return;
  }
  if (loc[0])
    sgf_move(mc, gval, loc);
  else
    sgf_pass(mc, gval);
}
  
/*
 * Call sgf_addCLElem() for a series of locations [xx][xx]...
 */
static void arg_multi(const char *arg, Sgf *mc, SgfType type,
		      GoStone gval, bool *errOut)  {
  char loc[5];
  
  for (;;)  {
    arg = strchr(arg, '[');
    if (!arg)
      return;
    arg = arg_getLoc(arg+1, loc);
    if (arg == NULL)  {
      *errOut = TRUE;
      return;
    }
    if (loc[0])
      sgf_addCLElem(mc, type, gval, loc);
    ++arg;
  }
}

/*
 * Call sgf_addCLElem() for a series of locations [xx][yy:zz]...
 */
static void arg_multiRange(const char *arg, Sgf *mc, SgfType type,
			   GoStone gval, bool *errOut)  {
  char loc[5], loc2[5];
  char xlo,xhi,ylo,yhi,x,y;

  for (;;)  {
    arg = strchr(arg, '[');
    if (!arg)
      return;
    arg = arg_getLoc(arg+1, loc);
    if (arg == NULL) {
      *errOut = TRUE;
      return;
    }
    if (*arg == ':') {
      arg = arg_getLoc(arg+1, loc2);
      if (arg == NULL) {
	*errOut = TRUE;
	return;
      }
      
      /* expect auav, axay */
      if (!(loc[0] == 'a' && loc[2] == 'a' &&
	    loc2[0] == 'a' && loc2[2] == 'a')) {
	*errOut = TRUE;
	return;
      }
      
      xlo = loc[1];
      xhi = loc2[1];
      ylo = loc[3];
      yhi = loc2[3];
      for(x=xlo; x<=xhi; x++) {
	for(y=ylo; y<=yhi; y++) {
	  loc[1] = x;
	  loc[3] = y;
	  sgf_addCLElem(mc, type, gval, loc);
	}
      }
    } else if (loc[0])
      sgf_addCLElem(mc, type, gval, loc);
    ++arg;
  }
}


static void  arg_multiLabel(const char *arg, Sgf *mc, bool *errOut)  {
  char loc[5];

  for (;;)  {
    arg = strchr(arg, '[');
    if (!arg)
      return;
    arg = arg_getLoc(arg+1, loc);
    if (!arg)  {
      *errOut = TRUE;
      return;
    }
    arg = strchr(arg, ':');
    if (!arg)  {
      *errOut = TRUE;
      return;
    }
    sgf_label(mc, loc, "");
    ++arg;
    while ((*arg != ']') && (*arg != '\0'))  {
      str_catChar(mc->active->sVal, *arg);
      ++arg;
    }
    if (*arg == '\0')  {
      *errOut = TRUE;
      return;
    }
  }
}


static int  arg_getInt(const char *arg, bool *err)  {
  int  len;
  char  tmp[11];

  while (*arg != '[')
    ++arg;
  if (arg[1] == ']')  {
    /*
     * Is this an error?  I think so.
     * But apparently some lame ass SGF editor uses [] as zero at times,
     *   so I gotta accept it.
     */
    return(0);
  }
  ++arg;
  for (len = 0;  arg[len] != ']';  ++len)  {
    tmp[len] = arg[len];
    if (len + 1 >= sizeof(tmp))  {
      *err = TRUE;
      return(0);
    }
  }
  tmp[len] = '\0';
  return(wms_atoi(tmp, err));
}


static double  arg_getDouble(const char *arg, bool *err)  {
  int  len;
  char  tmp[20];

  while (*arg != '[')
    ++arg;
  ++arg;
  for (len = 0;  arg[len] != ']';  ++len)  {
    tmp[len] = arg[len];
    if (len + 1 >= sizeof(tmp))  {
      *err = TRUE;
      return(0);
    }
  }
  tmp[len] = '\0';
  return(wms_atof(tmp, err));
}


static const char  *arg_getStr(const char *arg, Str *result)  {
  str_clip(result, 0);
  while (*arg != '[')
    ++arg;
  ++arg;
  while (*arg != ']')  {
    assert(*arg);
    if (*arg == '\\')
      ++arg;
    str_catChar(result, *arg);
    ++arg;
  }
  return(str_chars(result));
}


static uint  getToken(FILE *f, const char **arg, int *lineCount)  {
  static bool  firstTime = TRUE;
  static Str  buf;
  int  c;
  enum  { state_token, state_arg, state_nextArg } state;
  bool  escaping = FALSE, escaped;
  uint  token = 0;
  int  shift = 0;
  int  countAdd = 0;
  int  lastCloseBracket = 0;

  if (firstTime)  {
    str_init(&buf);
    firstTime = FALSE;
  }
  state = state_token;
  str_copyChars(&buf, "");
  for (;;)  {
    c = getc(f);
    if (c == '\n')
      ++countAdd;
    if (c == EOF)  {
      *arg = str_chars(&buf);
      if (state == state_nextArg)  {
	*lineCount += countAdd;
	return(token);
      } else
	return(0);
    }
    if (!escaping)  {
      escaped = FALSE;
      if (c == '\\')
	escaping = TRUE;
    } else  {
      escaped = TRUE;
      escaping = FALSE;
    }
    if ((state == state_token) && !escaped && !token &&
	((c == ';') || (c == '(') || (c == ')')))  {
      str_catChar(&buf, c);
      token = c;
      *arg = str_chars(&buf);
      *lineCount += countAdd;
      return(token);
    } else if ((state == state_token) && !islower(c) && !isspace(c) &&
	       (c != '['))  {
      str_catChar(&buf, c);
      token |= ((int)c & 0xff) << shift;
      shift += 8;
      if (shift > 16)
	return(0);
    } else if (((state == state_token) || (state == state_nextArg)) &&
	       !escaped && (c == '['))  {
      *lineCount += countAdd;
      countAdd = 0;
      str_catChar(&buf, c);
      state = state_arg;
    } else if ((state == state_arg) && !escaped && (c == ']'))  {
      str_catChar(&buf, c);
      lastCloseBracket = str_len(&buf);
      state = state_nextArg;
    } else if ((state == state_nextArg) && !isspace(c) && (c != '['))  {
      ungetc(c, f);
      *arg = str_chars(&buf);
      *lineCount += countAdd;
      assert(lastCloseBracket > 0);
      str_clip(&buf, lastCloseBracket);
      return(token);
    } else  {
      str_catChar(&buf, c);
    }
  }
}
