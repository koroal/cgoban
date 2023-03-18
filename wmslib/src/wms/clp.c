/*
 * wmslib/src/wms/clp.c, part of wmslib (Library functions)
 * Copyright (C) 1995-1997 William Shubert.
 * See "configure.h.in" for more copyright information.
 *
 * Command Line Parse.
 */

#include <wms.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>  /* To get umask()...I hope this is portable! */
#include <wms/str.h>

#if  STDC_HEADERS
#include <stdlib.h>
#endif  /* STDC_HEADERS */

#ifdef  _WMS_CLP_H_
#error  Levelization Error.
#endif
#include "clp.h"


/**********************************************************************
 * Globals
 **********************************************************************/
const char  *wms_progname = "UNKNOWN";

static bool  showErrs = TRUE;


/**********************************************************************
 * Forward Declarations
 **********************************************************************/
static ClpEntry  *clp_iLookup(Clp *clp, const char *key, bool *bval);
static void  clpEntry_free(ClpEntry *ci);
static void  strip(char *argv[]);
static void  clp_help(Clp *clp);
static void  setStrList(ClpEntry *entry, const char *vals);
static void  moreStrs(ClpEntry *ce, int newNumStrs);


/**********************************************************************
 * Functions
 **********************************************************************/
Clp  *clp_create(const ClpSetup vars[])  {
  int  numVars, i;
  Clp  *newClp;

  newClp = wms_malloc(sizeof(Clp));
  MAGIC_SET(newClp);
  for (numVars = 0;  !(vars[numVars].flags & CLPSETUP_ENDFLAG);  ++numVars);
  newClp->numInfos = numVars + 2;
  newClp->infos = wms_malloc((numVars + 2) * sizeof(ClpEntry));
  for (i = 0;  i < numVars;  ++i)  {
    MAGIC_SET(&newClp->infos[i]);
    newClp->infos[i].name = vars[i].name;
    newClp->infos[i].desc = vars[i].desc;
    newClp->infos[i].flags = vars[i].flags;
    newClp->infos[i].test = vars[i].test;
    if (vars[i].name == NULL)
      newClp->infos[i].flags |= CLPSETUP_NOSAVE;
    newClp->infos[i].where = clpWhere_unset;
    newClp->infos[i].type = clpDtype_string;
    newClp->infos[i].numStrs = 0;
    newClp->infos[i].maxStrs = 0;
    newClp->infos[i].storage.strList = NULL;
    if (vars[i].defVal)  {
      setStrList(&newClp->infos[i], vars[i].defVal);
    } else  {
      setStrList(&newClp->infos[i], "");
    }
    assert(newClp->infos[i].numStrs > 0);
  }
  MAGIC_SET(&newClp->infos[i]);
  newClp->infos[i].name = NULL;
  newClp->infos[i].desc = "";
  newClp->infos[i].flags = CLPSETUP_BOOL|CLPSETUP_NOSAVE;
  newClp->infos[i].where = clpWhere_unset;
  newClp->infos[i].type = clpDtype_bool;
  newClp->infos[i].test = NULL;
  
  ++i;
  MAGIC_SET(&newClp->infos[i]);
  newClp->infos[i].name = "help,-help";
  newClp->infos[i].desc = "Show this message";
  newClp->infos[i].flags = CLPSETUP_BOOL|CLPSETUP_NOSAVE|CLPSETUP_HELP;
  newClp->infos[i].where = clpWhere_unset;
  newClp->infos[i].type = clpDtype_bool;
  newClp->infos[i].test = NULL;
  return(newClp);
}


void  clp_destroy(Clp *clp)  {
  int  i;

  assert(MAGIC(clp));
  for (i = 0;  i < clp->numInfos;  ++i)
    clpEntry_free(&clp->infos[i]);
  wms_free(clp->infos);
  wms_free(clp);
}


int   clp_rCmdline(Clp *clp, char *argv[])  {
  int  numArgs;
  ClpEntry  *ce;
  bool  bval;

  assert(MAGIC(clp));
  wms_progname = argv[0];
  strip(argv);
  for (numArgs = 0;  argv[numArgs];)  {
    if (argv[numArgs][0] == '-')  {
      /* It's a switch. */
      ce = clp_iLookup(clp, argv[numArgs]+1, &bval);
      ce->where = clpWhere_cmdline;
      if (ce->flags & CLPSETUP_BOOL)  {
	if (bval && (ce->flags & CLPSETUP_HELP))  {
	  clp_help(clp);
	  exit(1);
	}
	clpEntry_setBool(ce, bval);
      } else  {
	if (argv[numArgs+1])
	  clpEntry_setStr(ce, argv[numArgs+1]);
	else  {
	  fprintf(stderr, "%s: Switch \"%s\" requires an argument.\n",
		  wms_progname, argv[numArgs]);
	  exit(1);
	}
	strip(argv + numArgs);
      }
      strip(argv + numArgs);
    } else
      ++numArgs;
  }
  return(numArgs);
}


/*
 * clp_rFile returns FALSE if it can't find the file at all.
 */
bool  clp_rFile(Clp *clp, const char *fname)  {
  FILE  *ifile;
  char  line_in[1024], *arg;
  Str  full_fn;
  char  *home, *key_start;
  int  i, ch_in;
  bool  retval = FALSE;
  ClpEntry  *ci;

  assert(MAGIC(clp));
  str_initChars(&full_fn, fname);
  showErrs = FALSE;
  if (fname[0] == '~')  {
    /* Prepend "$HOME" */
    home = getenv("HOME");
    if (home == NULL)  {
      fprintf(stderr,
	      "%s: Error: Could not find environment variable \"HOME\".\n",
	      wms_progname);
      showErrs = TRUE;
      str_deinit(&full_fn);
      return(FALSE);
    }
    str_print(&full_fn, "%s/%s", home, fname+1);
  }
  ifile = fopen(str_chars(&full_fn), "r");
  if (ifile != NULL)  {
    retval = TRUE;
    do  {
      ch_in = getc(ifile);
      for (i = 0;  (i < 1023) && (ch_in != '\n') && (ch_in != EOF);  ++i)  {
	line_in[i] = ch_in;
	ch_in = getc(ifile);
      }
      while ((ch_in != '\n') && (ch_in != EOF))
	ch_in = getc(ifile);
      line_in[i] = '\0';
      if ((line_in[0] == '#') || (line_in[0] == '\0'))
	continue;
      key_start = strchr(line_in, '.') + 1;
      if (key_start != NULL)  {
	arg = strchr(line_in, ':');
	*arg = '\0';
	arg += 2;
	ci = clp_iLookup(clp, key_start, NULL);
	if (ci != NULL)  {
	  if ((ci->where == clpWhere_unset) ||
	      (ci->where == clpWhere_xdefs))  {
	    ci->where = clpWhere_rcfile;
	    setStrList(ci, arg);
	  }
	}
      }
    } while (ch_in != EOF);
    fclose(ifile);
  }
  str_deinit(&full_fn);
  showErrs = TRUE;
  return(retval);
}


void  clp_wFile(Clp *clp, const char *fname, const char *pname)  {
  FILE *ofile;
  Str  full_fn;
  char  *home;
  const char  *strOut;
  int  i, prevMask, strNum, charNum;
  ClpEntry  *ci;

  str_initChars(&full_fn, fname);
  if (fname[0] == '~')  {
    /* Prepend "$HOME" */
    home = getenv("HOME");
    if (home == NULL)  {
      fprintf(stderr,
	      "%s: Error: Could not find environment variable \"HOME\".\n",
	      wms_progname);
      str_deinit(&full_fn);
      return;
    }
    str_print(&full_fn, "%s/%s", home, fname+1);
  }
  prevMask = umask(0177);
  ofile = fopen(str_chars(&full_fn), "w");
  umask(prevMask);
  if (ofile == NULL)  {
    fprintf(stderr, "%s: Error: Could not open file \"%s\" for writing.\n",
	    wms_progname, fname);
    perror(wms_progname);
    exit(1);
  }
  fprintf(ofile,
	  "# NOTICE: Please do not edit this file.\n"
	  "# It was automatically generated by \"%s\".  If you want to\n"
	  "#   change one of these values, please use command line switches,\n"
	  "#   X defaults, or a setup window.\n"
	  "# As a last resort you may simply delete this file.\n\n",
	  wms_progname);
  for (i = 0;  i < clp->numInfos;  ++i)  {
    ci = &clp->infos[i];
    if (!(ci->flags & CLPSETUP_NOSAVE))  {
      switch(ci->type)  {
      case clpDtype_int:
	fprintf(ofile, "%s.%s: %d\n", pname, ci->name, ci->storage.ival);
	break;
      case clpDtype_double:
	fprintf(ofile, "%s.%s: %f\n", pname, ci->name, ci->storage.dval);
	break;
      case clpDtype_bool:
	if (ci->storage.bval)
	  fprintf(ofile, "%s.%s: y\n", pname, ci->name);
	else
	  fprintf(ofile, "%s.%s: n\n", pname, ci->name);
	break;
      case clpDtype_string:
	fprintf(ofile, "%s.%s: ", pname, ci->name);
	for (strNum = 0;  strNum < ci->numStrs;  ++strNum)  {
	  if (strNum != 0)
	    putc('|', ofile);
	  strOut = str_chars(&ci->storage.strList[strNum]);
	  for (charNum = 0;  strOut[charNum];  ++charNum)  {
	    if ((strOut[charNum] == '|') ||
		(strOut[charNum] == '\\'))
	      putc('\\', ofile);
	    putc(strOut[charNum], ofile);
	  }
	}
	putc('\n', ofile);
	break;
      default:
	/* Should never reach here. */
	assert(0);
	break;
      }
    }
  }
  fclose(ofile);
  str_deinit(&full_fn);
}


int  clpEntry_iGetInt(ClpEntry *ce, bool *err)  {
  switch(ce->type)  {
  case clpDtype_int:
    if (err)
      *err = FALSE;
    return(ce->storage.ival);
    break;
  case clpDtype_string:
    return(wms_atoi(str_chars(&ce->storage.strList[0]), err));
    break;
  default:
    if (err)
      *err = TRUE;
    return(0);
  }
}


double  clpEntry_iGetDouble(ClpEntry *ce, bool *err)  {
  switch(ce->type)  {
  case clpDtype_double:
    if (err)
      *err = FALSE;
    return(ce->storage.dval);
    break;
  case clpDtype_string:
    return(wms_atof(str_chars(&ce->storage.strList[0]), err));
    break;
  default:
    if (err)
      *err = TRUE;
    return(0.0);
  }
}


const char  *clpEntry_iGetStrNum(ClpEntry *ce, int num, bool *err)  {
  if (ce->type == clpDtype_string)  {
    if (err)
      *err = FALSE;
    assert(num < ce->numStrs);
    return(str_chars(&ce->storage.strList[num]));
  } else  {
    if (err)
      *err = TRUE;
    return(NULL);
  }
}

  
bool  clpEntry_iGetBool(ClpEntry *ce, bool *err)  {
  const char  *str;

  switch(ce->type)  {
  case clpDtype_bool:
    if (err)
      *err = FALSE;
    return(ce->storage.bval);
    break;
  case clpDtype_string:
    str = str_chars(&ce->storage.strList[0]);
    if ((!strcmp(str, "1")) ||
	(!strcmp(str, "t")) ||
	(!strcmp(str, "T")) ||
	(!strcmp(str, "y")) ||
	(!strcmp(str, "Y")) ||
	(!strcmp(str, "true")) ||
	(!strcmp(str, "True")) ||
	(!strcmp(str, "TRUE")) ||
	(!strcmp(str, "yes")) ||
	(!strcmp(str, "Yes")) ||
	(!strcmp(str, "YES")))
      return(TRUE);
    if ((!strcmp(str, "0")) ||
	(!strcmp(str, "f")) ||
	(!strcmp(str, "F")) ||
	(!strcmp(str, "n")) ||
	(!strcmp(str, "N")) ||
	(!strcmp(str, "false")) ||
	(!strcmp(str, "False")) ||
	(!strcmp(str, "FALSE")) ||
	(!strcmp(str, "no")) ||
	(!strcmp(str, "No")) ||
	(!strcmp(str, "NO")))
      return(FALSE);
    if (err)
      *err = TRUE;
    return(FALSE);
    break;
  default:
    if (err)
      *err = TRUE;
    return(FALSE);
  }
}


bool  clpEntry_setInt(ClpEntry *ce, int val)  {
  ClpEntry  newCe;

  MAGIC_SET(&newCe);
  if (ce->test)  {
    newCe.type = clpDtype_int;
    newCe.storage.ival = val;
    if (!ce->test(&newCe))
      return(FALSE);
  }
  clpEntry_free(ce);
  ce->type = clpDtype_int;
  ce->storage.ival = val;
  MAGIC_UNSET(&newCe);
  return(TRUE);
}


bool  clpEntry_setDouble(ClpEntry *ce, double val)  {
  ClpEntry  newCe;

  MAGIC_SET(&newCe);
  if (ce->test)  {
    newCe.type = clpDtype_double;
    newCe.storage.dval = val;
    if (!ce->test(&newCe))
      return(FALSE);
  }
  clpEntry_free(ce);
  ce->type = clpDtype_double;
  ce->storage.dval = val;
  MAGIC_UNSET(&newCe);
  return(TRUE);
}


bool  clpEntry_setStrNum(ClpEntry *ce, const char *val, int num)  {
  ClpEntry  newCe;
  bool  testResult;

  if (ce->test)  {
    assert(num == 0);
    MAGIC_SET(&newCe);
    newCe.type = clpDtype_string;
    newCe.storage.strList = str_createChars(val);
    newCe.numStrs = 1;
    testResult = ce->test(&newCe);
    str_destroy(newCe.storage.strList);
    MAGIC_UNSET(&newCe);
    if (!testResult)
      return(FALSE);
  }
  if (ce->type == clpDtype_string)  {
    if (num >= ce->numStrs)  {
      moreStrs(ce, num+1);
    }
    str_copyChars(&ce->storage.strList[num], val);
  } else  {
    assert(num == 0);
    clpEntry_free(ce);
    ce->type = clpDtype_string;
    ce->numStrs = 1;
    ce->maxStrs = 1;
    ce->storage.strList = str_createChars(val);
  }
  return(TRUE);
}


static void  moreStrs(ClpEntry *ce, int newNumStrs)  {
  Str  *newStrs;
  int  i;

  assert(newNumStrs > ce->numStrs);
  newStrs = wms_malloc(newNumStrs * sizeof(Str));
  for (i = 0;  i < ce->numStrs;  ++i)  {
    newStrs[i] = ce->storage.strList[i];
  }
  for (;  i < newNumStrs;  ++i)  {
    str_init(&newStrs[i]);
  }
  wms_free(ce->storage.strList);
  ce->storage.strList = newStrs;
}


bool  clpEntry_setBool(ClpEntry *ce, bool val)  {
  ClpEntry  newCe;

  MAGIC_SET(&newCe);
  if (ce->test)  {
    newCe.type = clpDtype_bool;
    newCe.storage.bval = val;
    if (!ce->test(&newCe))
      return(FALSE);
  }
  clpEntry_free(ce);
  ce->type = clpDtype_bool;
  ce->storage.bval = val;
  MAGIC_UNSET(&newCe);
  return(TRUE);
}


ClpEntry  *clp_lookup(Clp *clp, const char *key)  {
  assert(MAGIC(clp));
  return(clp_iLookup(clp, key, NULL));
}


static ClpEntry  *clp_iLookup(Clp *clp, const char *key, bool *bval)  {
  ClpEntry  *ci;
  int  i, j, keyLen;
  bool  reversed;

  assert(MAGIC(clp));
  keyLen = strlen(key);
  reversed = !strncmp(key, "no", 2);
  if (bval)
    *bval = TRUE;
  for (i = 0;  i < clp->numInfos;  ++i)  {
    ci = &clp->infos[i];
    if (ci->name)  {
      for (j = 0;  ci->name[j];)  {
	if (!strncmp(ci->name+j, key, keyLen))  {
	  if ((ci->name[j + keyLen] == '\0') ||
	      (ci->name[j + keyLen] == ','))
	    return(ci);
	}
	if (reversed && (ci->flags & CLPSETUP_BOOL) &&
	    !strncmp(ci->name, key+2, keyLen - 2))  {
	  if (bval)
	    *bval = FALSE;
	  return(ci);
	}
	while (ci->name[j] && (ci->name[j] != ','))
	  ++j;
	if (ci->name[j])
	  ++j;
      }
    }
  }
  if (showErrs)  {
    fprintf(stderr, "%s: \"-%s\" is not a valid flag.\n"
	    "   Use \"%s -help\" for a list of valid flags.\n",
	    wms_progname, key, wms_progname);
    exit(1);
  } else
    return(NULL);
}


static void  strip(char *argv[])  {
  do  {
    argv[0] = argv[1];
    ++argv;
  } while (argv[0] != NULL);
}


static void  clpEntry_free(ClpEntry *ci)  {
  int  i;

  assert(MAGIC(ci));
  if (ci->type == clpDtype_string)  {
    for (i = 0;  i < ci->maxStrs;  ++i)
      str_deinit(&ci->storage.strList[i]);
    wms_free(ci->storage.strList);
  }
}


static void  clp_help(Clp *clp)  {
  int  i, j;
  char  name[40];
  const char  *prestr;

  assert(MAGIC(clp));
  for (i = 0;  i < clp->numInfos;  ++i)  {
    if (clp->infos[i].desc)  {
      if (clp->infos[i].name)  {
	name[0] = '\0';
	if (clp->infos[i].flags & CLPSETUP_SHOWBOOL)
	  prestr = "-[no]";
	else
	  prestr = "-";
	strcpy(name, prestr);
	for (j = 0;  clp->infos[i].name[j];  ++j)  {
	  strncat(name, clp->infos[i].name+j, 1);
	  if (clp->infos[i].name[j] == ',')
	    strcat(name, prestr);
	}
	fprintf(stderr, "  %20s %s\n", name, clp->infos[i].desc);
      } else
	fprintf(stderr, "  %s\n", clp->infos[i].desc);
    }
  }
}


static void  setStrList(ClpEntry *entry, const char *vals)  {
  int  numVals = 1;
  int  i;
  Str  *newStrs;
  bool  escaped;

  for (i = 0, escaped = FALSE;  vals[i];  ++i)  {
    if (!escaped && (vals[i] == '|'))
      ++numVals;
    escaped = (!escaped && (vals[i] == '\\'));
  }
  if (numVals > entry->maxStrs)  {
    newStrs = wms_malloc(numVals * sizeof(Str));
    for (i = 0;  i < numVals;  ++i)  {
      str_init(&newStrs[i]);
    }
    if (entry->storage.strList != NULL)
      wms_free(entry->storage.strList);
    entry->maxStrs = numVals;
    entry->storage.strList = newStrs;
  } else
    newStrs = entry->storage.strList;
  entry->numStrs = numVals;
  numVals = 0;
  str_clip(&newStrs[0], 0);
  for (i = 0, escaped = FALSE;  vals[i];  ++i)  {
    if (escaped) {
      str_catChar(&newStrs[numVals], vals[i]);
      escaped = FALSE;
    } else  {
      if (vals[i] == '\\')
	escaped = TRUE;
      else if (vals[i] == '|')  {
	++numVals;
	assert(numVals < entry->numStrs);
	str_clip(&newStrs[numVals], 0);
      } else  {
	str_catChar(&newStrs[numVals], vals[i]);	
      }
    }
  }
}

