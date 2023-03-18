/*
 * wmslib/src/wms/clp.h, part of wmslib (Library functions)
 * Copyright (C) 1994-1996 William Shubert.
 * See "configure.h.in" for more copyright information.
 *
 * Command Line Parse.
 */

#ifndef  _WMS_CLP_H_

#ifndef  _WMS_STR_H_
#include <wms/str.h>
#endif

#ifdef  _WMS_CLP_H_
  Levelization Error.
#endif
#define  _WMS_CLP_H_  1


/**********************************************************************
 * Data types.  These are all visible to the application.
 **********************************************************************/

struct ClpEntry_struct;

/* Public. */
#define  CLPSETUP_BOOL      0x01  /* Is it "-[no]<x>" or "-<x> <arg>"? */
#define  CLPSETUP_SHOWBOOL  0x02  /* Show "[no]" in the help.          */
#define  CLPSETUP_NOSAVE    0x04
#define  CLPSETUP_ENDFLAG   0x08
#define  CLPSETUP_HELP      0x10  /* This switch gives you help. */
typedef struct ClpSetup_struct  {
  const char *name;       /* NULL for just a label. */
  const char *defVal;
  const char *desc;       /* NULL if it doesn't show up in the help. */
  uint flags;
  bool  (*test)(struct ClpEntry_struct *entry);
} ClpSetup;

typedef enum  {
  clpWhere_unset, clpWhere_rcfile, clpWhere_xdefs, clpWhere_cmdline,
  clpWhere_dup
} ClpWhere;


/* Private. */
typedef enum  {
  clpDtype_int,   clpDtype_double,   clpDtype_bool,   clpDtype_string,
  clpDtype_end
} ClpDtype;

typedef struct ClpEntry_struct  {
  const char  *name;
  const char  *desc;
  uint  flags;
  bool  (*test)(struct ClpEntry_struct *entry);
  ClpWhere  where;
  ClpDtype  type;
  int  numStrs, maxStrs;
  union  {
    int ival;
    double dval;
    Str  *strList;
    bool bval;
  } storage;

  MAGIC_STRUCT
} ClpEntry;


typedef struct Clp_struct  {
  ClpEntry *infos;
  int numInfos;

  MAGIC_STRUCT
} Clp;

/**********************************************************************
 * Routines available
 **********************************************************************/
extern Clp  *clp_create(const ClpSetup vars[]);
extern void  clp_destroy(Clp *clp);

/*
 * clp_rCmdline returns the number of arguments left on the command
 *   line, or CLP_ARGS_NOGOOD if there was an error.
 */
extern int   clp_rCmdline(Clp *cltab, char *argv[]);

/*
 * clp_rFile returns FALSE if it can't find the file at all.
 */
extern bool  clp_rFile(Clp *cltab, const char *fname);
extern void  clp_wFile(Clp *cltab, const char *fname, const char *pname);

extern ClpEntry  *clp_lookup(Clp *clp, const char *varName);

#define  clp_where(clp, str)  (clp_lookup(clp, str)->where)
#define  clpEntry_where(ce)  ((ce)->where)

extern int  clpEntry_iGetInt(ClpEntry *ce, bool *err);
#define  clp_iGetInt(clp, str, e)  clpEntry_iGetInt(clp_lookup(clp, str), e)
#define  clpEntry_getInt(ce)  clpEntry_iGetInt(ce, NULL)
#define  clp_getInt(clp, str)  clpEntry_getInt(clp_lookup(clp, str))

extern double  clpEntry_iGetDouble(ClpEntry *ce, bool *err);
#define  clpEntry_getDouble(ce)  clpEntry_iGetDouble(ce, NULL)
#define  clp_getDouble(clp, str)  clpEntry_getDouble(clp_lookup(clp,str))

#define  clpEntry_numStrs(ce)  ((ce)->numStrs)

extern const char  *clpEntry_iGetStrNum(ClpEntry *ce, int num, bool *err);
#define  clpEntry_getStrNum(ce, n)  clpEntry_iGetStrNum(ce, n, NULL)
#define  clp_getStrNum(clp, s, n)  clpEntry_getStrNum(clp_lookup(clp, s), n)

#define  clpEntry_iGetStr(ce, e)  clpEntry_iGetStrNum(ce, 0, e)
#define  clpEntry_getStr(ce)  clpEntry_iGetStr(ce, NULL)
#define  clp_getStr(clp, str)  clpEntry_getStr(clp_lookup(clp, str))

extern bool  clpEntry_iGetBool(ClpEntry *ce, bool *err);
#define  clpEntry_getBool(ce)  clpEntry_iGetBool(ce, NULL)
#define  clp_getBool(clp, str)  clpEntry_getBool(clp_lookup(clp, str))

extern bool  clpEntry_setInt(ClpEntry *ce, int val);
#define  clp_setInt(c, s, v)  clpEntry_setInt(clp_lookup(c, s), v)

extern bool  clpEntry_setDouble(ClpEntry *ce, double val);
#define  clp_setDouble(c, s, v)  clpEntry_setDouble(clp_lookup(c, s), v)

extern bool  clpEntry_setStrNum(ClpEntry *ce, const char *val, int num);
#define  clp_setStrNum(c, s, v, n)  clpEntry_setStrNum(clp_lookup(c, s), v, n)

#define  clpEntry_setStr(ce, v)  clpEntry_setStrNum(ce, v, 0)
#define  clp_setStr(c, s, v)  clpEntry_setStr(clp_lookup(c, s), v)

extern bool  clpEntry_setBool(ClpEntry *ce, bool val);
#define  clp_setBool(c, s, v)  clpEntry_setBool(clp_lookup(c, s), v)


/**********************************************************************
 * Handy macros and constants.
 **********************************************************************/
#define  CLPSETUP_MSG(message)  {NULL, NULL, message, 0, NULL}
#define  CLPSETUP_END  {NULL,NULL,NULL,CLPSETUP_ENDFLAG, NULL}

#define  CLP_ARGS_NOGOOD  -1

#endif  /* _WMS_CLP_H_ */
