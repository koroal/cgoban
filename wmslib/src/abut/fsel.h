/*
 * src/fsel.h, part of wmslib library
 * Copyright (C) 1995-1997 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _ABUT_FSEL_H_

#ifndef  _ABUT_ABUT_H_
#include <abut/abut.h>
#endif
#ifndef  _ABUT_SWIN_H_
#include <abut/swin.h>
#endif
#ifndef  _WMS_STR_H_
#include <wms/str.h>
#endif
#ifndef  _ABUT_MSG_H_
#include <abut/msg.h>
#endif
#ifdef  _ABUT_FSEL_H_
#error  Levelization Error.
#endif
#define  _ABUT_FSEL_H_  1


/**********************************************************************
 * Data types
 **********************************************************************/
typedef struct AbutFsel_struct  {
  Abut  *abut;

  bool  propagate;
  void  (*callback)(struct AbutFsel_struct *fsel,
		    void *packet, const char *fname);
  void  *packet;

  ButWin  *win;

  But  *title;
  But  *box;
  But  *path, *pathIn;
  But  *file, *in;
  But  *mask, *maskIn;
  But  *dirs, *files;
  AbutSwin  *dSwin, *fSwin;
  But  *dList, *dlBg, *fList, *flBg;
  But  *ok;
  But  *cancel;

  Str  pathVal;

  AbutMsg  *msg;

  MAGIC_STRUCT
} AbutFsel;


/**********************************************************************
 * Globals
 **********************************************************************/
extern const char  *abutFsel_pathMessage;
extern const char  *abutFsel_fileMessage;
extern const char  *abutFsel_maskMessage;
extern const char  *abutFsel_dirsMessage;
extern const char  *abutFsel_filesMessage;
extern const char  *abutFsel_dirErrMessage;


/**********************************************************************
 * Functions
 **********************************************************************/
extern AbutFsel  *abutFsel_create(Abut *a,
				  void (*callback)(AbutFsel *fsel,
						   void *packet,
						   const char *fname),
				  void *packet,
				  const char *app, const char *title,
				  const char *startName);
extern AbutFsel  *abutFsel_createDir(Abut *a,
				     void (*callback)(AbutFsel *fsel,
						      void *packet,
						      const char *fname),
				     void *packet,
				     const char *app, const char *title,
				     const char *startDir,
				     const char *startName);
extern void  abutFsel_destroy(AbutFsel *fsel, bool propagate);

#define  abutFsel_setColors(f, ul, lr, c)  \
           butBoxFilled_setColors((f)->box, (ul), (lr), (c))
#define  abutFsel_setPixmaps(f, ul, lr, c)  \
           butBoxFilled_setPixmaps((f)->box, (ul), (lr), (c))

#endif  /* _ABUT_FSEL_H_ */
