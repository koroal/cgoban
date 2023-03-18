/*
 * src/fsel.c, part of wmslib library
 * Copyright (C) 1995-1997 William Shubert
 * See "configure.h.in" for more copyright information.
 */

#include <wms.h>
/*
 * Here goes the dirent kludge.  Ugh.  The things I go through for portability.
 */
#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif
#include <sys/stat.h>

#include <but/but.h>
#include <but/plain.h>
#include <but/ctext.h>
#include <but/tblock.h>
#include <but/textin.h>
#include <but/list.h>
#include <but/canvas.h>
#ifdef  _ABUT_FSEL_H_
#error  Levelization Error.
#endif
#include <abut/fsel.h>


/**********************************************************************
 * Globals
 **********************************************************************/
const char  *abutFsel_pathMessage = NULL;
const char  *abutFsel_fileMessage = NULL;
const char  *abutFsel_maskMessage = NULL;
const char  *abutFsel_dirsMessage = NULL;
const char  *abutFsel_filesMessage = NULL;
const char  *abutFsel_dirErrMessage = NULL;


/**********************************************************************
 * Forward Declarations
 **********************************************************************/
static ButOut  resize(ButWin *win);
static ButOut  dswinResize(ButWin *win), fswinResize(ButWin *win);
static ButOut  unmap(ButWin *win);
static ButOut  wDestroy(ButWin *win);
static ButOut  okPressed(But *but);
static ButOut  cancelPressed(But *but);
static ButOut  newName(But *but, const char *text);
static ButOut  newPath(But *but, const char *text);
static ButOut  newMask(But *but, const char *text);
static bool  dirScan(AbutFsel *f, const char *path);
static ButOut  dirPressed(But *but, int line);
static ButOut  filePressed(But *but, int line);
static void  fixPath(Str *pathOut, const char *pathIn);
static void  addCwd(Str *pathOut);
static bool  maskMatch(const char *mask, const char *fName, int fLen);
static void  dirError(AbutFsel *f, int errnov, const char *badDir);
static ButOut  msgDead(void *packet);
static ButOut  msgOkBut(But *but);


/**********************************************************************
 * Functions
 **********************************************************************/
AbutFsel  *abutFsel_createDir(Abut *a,
			      void (*callback)(AbutFsel *fsel, void *packet,
					       const char *fname),
			      void *packet,
			      const char *app, const char *title,
			      const char *startDir,
			      const char *startName)  {
  Str  fullName;
  AbutFsel  *result;

  if (strchr(startName, '/') == NULL)  {
    str_initChars(&fullName, startDir);
    str_catChar(&fullName, '/');
    str_catChars(&fullName, startName);
    result = abutFsel_create(a, callback, packet, app, title,
			     str_chars(&fullName));
    str_deinit(&fullName);
    return(result);
  } else  {
    return(abutFsel_create(a, callback, packet, app, title, startName));
  }
}


AbutFsel  *abutFsel_create(Abut *a,
			   void (*callback)(AbutFsel *fsel, void *packet,
					    const char *fname),
			   void *packet,
			   const char *app, const char *title,
			   const char *startName)  {
  ButEnv  *env;
  AbutFsel  *f;
  int  butH, bw;
  int  i, minW, minH, w, h;
  Str  titleString, pathString;
  bool  err;

  assert(MAGIC(a));
  assert(abutFsel_pathMessage != NULL);
  assert(abutFsel_fileMessage != NULL);
  assert(abutFsel_maskMessage != NULL);
  assert(abutFsel_dirsMessage != NULL);
  assert(abutFsel_filesMessage != NULL);
  assert(abutFsel_dirErrMessage != NULL);
  env = a->env;
  f = wms_malloc(sizeof(AbutFsel));
  MAGIC_SET(f);
  f->abut = a;
  f->propagate = TRUE;
  f->callback = callback;
  f->packet = packet;
  str_init(&f->pathVal);
  f->msg = NULL;
  
  butH = a->butH;
  bw = butEnv_stdBw(env);
  minH = butH*10 + bw*10;
  minW = minH;
  if (a->clp)  {
    w = (int)(minW * clp_getDouble(a->clp, "abut.fsel.w") + 0.5);
    h = (int)(minH * clp_getDouble(a->clp, "abut.fsel.h") + 0.5);
    if (w < minW)
      w = minW;
    if (h < minH)
      h = minH;
  } else  {
    w = minW * 1.2;
    h = minH * 1.2;
  }
  str_initChars(&titleString, app);
  str_catChars(&titleString, " File Selector");
  f->win = butWin_create(f, env, str_chars(&titleString), w, h, unmap, NULL,
			 resize, wDestroy);
  i = clp_iGetInt(a->clp, "abut.fsel.x", &err);
  if (!err)
    butWin_setX(f->win, i);
  i = clp_iGetInt(a->clp, "abut.fsel.y", &err);
  if (!err)
    butWin_setY(f->win, i);
  butWin_setMinW(f->win, minW);
  butWin_setMinH(f->win, minH);
  butWin_setMaxW(f->win, 0);
  butWin_setMaxH(f->win, 0);
  butWin_activate(f->win);

  str_initChars(&pathString, startName);
  for (i = str_len(&pathString);
       (i > 0) && (str_chars(&pathString)[i - 1] != '/');
       --i);
  str_copyChars(&titleString, str_chars(&pathString) + i);
  if (i > 0)
    str_clip(&pathString, i);
  else
    str_copyChars(&pathString, "./");
  fixPath(&f->pathVal, str_chars(&pathString));
  
  f->box = butBoxFilled_create(f->win, 0, BUT_DRAWABLE);
  butBoxFilled_setColors(f->box, a->ulColor, a->lrColor, a->bgColor);
  butBoxFilled_setPixmaps(f->box, a->ulPixmap, a->lrPixmap, a->bgPixmap);
  f->title = butText_create(f->win, 1, BUT_DRAWABLE,
			    title, butText_left);
  f->path = butText_create(f->win, 1, BUT_DRAWABLE,
			   abutFsel_pathMessage, butText_left);
  f->pathIn = butTextin_create(newPath, f, f->win, 1,
			       BUT_DRAWABLE|BUT_PRESSABLE,
			       str_chars(&f->pathVal), 100);
  f->file = butText_create(f->win, 1, BUT_DRAWABLE,
			   abutFsel_fileMessage, butText_left);
  f->in = butTextin_create(newName, f, f->win, 1, BUT_DRAWABLE|BUT_PRESSABLE,
			   str_chars(&titleString), 100);
  f->mask = butText_create(f->win, 1, BUT_DRAWABLE,
			   abutFsel_maskMessage, butText_left);
  f->maskIn = butTextin_create(newMask, f, f->win, 1,
			       BUT_DRAWABLE|BUT_PRESSABLE,
			       "*.sgf", 100);
  f->dirs = butText_create(f->win, 1, BUT_DRAWABLE,
			   abutFsel_dirsMessage, butText_left);
  f->dSwin = abutSwin_create(f, f->win, 1, ABUTSWIN_LSLIDE,
			     dswinResize);
  f->dlBg = butPlain_create(f->dSwin->win, 0, BUT_DRAWABLE, BUT_BG);
  f->dList = butList_create(dirPressed, f, f->dSwin->win, 1,
			    BUT_DRAWABLE|BUT_PRESSABLE);

  f->files = butText_create(f->win, 1, BUT_DRAWABLE,
			    abutFsel_filesMessage, butText_left);
  f->fSwin = abutSwin_create(f, f->win, 1, ABUTSWIN_LSLIDE,
			     fswinResize);
  f->flBg = butPlain_create(f->fSwin->win, 0, BUT_DRAWABLE, BUT_BG);
  f->fList = butList_create(filePressed, f, f->fSwin->win, 1,
			    BUT_DRAWABLE|BUT_PRESSABLE);

  f->ok = butCt_create(okPressed, f, f->win, 1, BUT_DRAWABLE|BUT_PRESSABLE,
		       a->ok);
  f->cancel = butCt_create(cancelPressed, f, f->win, 1,
			   BUT_DRAWABLE|BUT_PRESSABLE, a->cancel);
  str_deinit(&pathString);
  str_deinit(&titleString);

  dirScan(f, butTextin_get(f->pathIn));

  return(f);
}


void  abutFsel_destroy(AbutFsel *f, bool propagate)  {
  assert(MAGIC(f));
  f->propagate = propagate;
  butWin_destroy(f->win);
}


static ButOut  resize(ButWin *win)  {
  AbutFsel  *f = butWin_packet(win);
  ButEnv  *env = butWin_env(win);
  int  butH, bw;
  int  w, h;
  int  textMaxW, textW;
  int  dirsW, filesW;

  assert(MAGIC(f));
  w = butWin_w(win);
  h = butWin_h(win);
  butH = f->abut->butH;
  bw = butEnv_stdBw(env);
  but_resize(f->box, 0,0, w,h);
  but_resize(f->title, bw*2,bw*2, w-bw*4,butH);

  textMaxW = butText_resize(f->path, bw*2, butH+bw*3, butH);
  textW = butText_resize(f->file, bw*2, butH*2+bw*4, butH);
  if (textW > textMaxW)
    textMaxW = textW;
  textW = butText_resize(f->mask, bw*2, butH*3+bw*5, butH);
  if (textW > textMaxW)
    textMaxW = textW;

  but_resize(f->pathIn, bw*3 + textMaxW, butH+bw*3,
	     w - (bw*5 + textMaxW), butH);
  but_resize(f->in, bw*3 + textMaxW, butH*2+bw*4,
	     w - (bw*5 + textMaxW), butH);
  but_resize(f->maskIn, bw*3 + textMaxW, butH*3+bw*5,
	     w - (bw*5 + textMaxW), butH);
  dirsW = (w - bw*5) / 2;
  filesW = w - (bw*5 + dirsW);
  but_resize(f->dirs, bw*2, butH*4 + bw*6, dirsW, butH);
  abutSwin_resize(f->dSwin, bw*2, butH*5 + bw*7,
		  dirsW, h - butH*6 - bw*10, 3 * butEnv_fontH(env, 0) / 2,
		  butEnv_fontH(env, 0));
  but_resize(f->files, bw*3 + dirsW, butH*4 + bw*6, filesW, butH);
  abutSwin_resize(f->fSwin, bw*3 + dirsW, butH*5 + bw*7,
		  filesW, h - butH*6 - bw*10, 3 * butEnv_fontH(env, 0) / 2,
		  butEnv_fontH(env, 0));
  but_resize(f->cancel, bw*2, h-butH-bw*2,
	     (w-bw*5)/2, butH);
  but_resize(f->ok, (w-bw*5)/2+bw*3, h-butH-bw*2,
	     (w-bw*5+1)/2, butH);
  butCan_resizeWin(f->dSwin->win, 0,
		   butList_len(f->dList) * butEnv_fontH(env, 0) +
		   butEnv_stdBw(env) * 2, TRUE);
  butCan_resizeWin(f->fSwin->win, 0,
		   butList_len(f->fList) * butEnv_fontH(env, 0) +
		   butEnv_stdBw(env) * 2, TRUE);

  return(0);
}


static ButOut  okPressed(But *but)  {
  AbutFsel  *f = but_packet(but);
  Str  result;

  assert(MAGIC(f));
  str_initChars(&result, butTextin_get(f->pathIn));
  str_catChars(&result, butTextin_get(f->in));
  if (f->callback)  {
    f->callback(f, f->packet, str_chars(&result));
    f->callback = NULL;
  }
  str_deinit(&result);
  butWin_destroy(f->win);
  return(0);
}


static ButOut  cancelPressed(But *but)  {
  AbutFsel  *f = but_packet(but);

  assert(MAGIC(f));
  if (f->callback)  {
    f->callback(f, f->packet, NULL);
    f->callback = NULL;
  }
  butWin_destroy(f->win);
  return(0);
}


static ButOut  unmap(ButWin *win)  {
  butWin_destroy(win);
  return(0);
}


static ButOut  wDestroy(ButWin *win)  {
  AbutFsel  *f = butWin_packet(win);

  assert(MAGIC(f));
  if (f->abut->clp)  {
    clp_setInt(f->abut->clp, "abut.fsel.x", butWin_x(win));
    clp_setInt(f->abut->clp, "abut.fsel.y", butWin_y(win));
    clp_setDouble(f->abut->clp, "abut.fsel.w",
		  (double)butWin_w(win) / (double)butWin_getMinW(win));
    clp_setDouble(f->abut->clp, "abut.fsel.h",
		  (double)butWin_h(win) / (double)butWin_getMinH(win));
  }
  if (f->callback && f->propagate)  {
    f->callback(f, f->packet, NULL);
    f->callback = NULL;
  }
  abutSwin_destroy(f->dSwin);
  abutSwin_destroy(f->fSwin);
  str_deinit(&f->pathVal);
  if (f->msg)  {
    abutMsg_destroy(f->msg, FALSE);
    f->msg = NULL;
  }
  MAGIC_UNSET(f);
  wms_free(f);
  return(0);
}


static ButOut  newName(But *but, const char *text)  {
  AbutFsel  *f = but_packet(but);

  assert(MAGIC(f));
  if (f->callback)  {
    f->callback(f, f->packet, text);
    f->callback = NULL;
  }
  butWin_destroy(f->win);
  return(0);
}


static ButOut  newPath(But *but, const char *text)  {
  AbutFsel  *f = but_packet(but);
  Str  newDir;
  bool  ok;
  uint  result = 0;

  assert(MAGIC(f));
  str_init(&newDir);
  fixPath(&newDir, butTextin_get(but));
  ok = dirScan(f, str_chars(&newDir));
  if (ok)  {
    but_setFlags(but, BUT_NOKEY);
    butTextin_set(but, str_chars(&newDir), FALSE);
    str_copy(&f->pathVal, &newDir);
  } else  {
    result = BUTOUT_ERR;
  }
  str_deinit(&newDir);
  return(result);
}


static ButOut  newMask(But *but, const char *text)  {
  AbutFsel  *f = but_packet(but);

  assert(MAGIC(f));
  but_setFlags(but, BUT_NOKEY);
  dirScan(f, str_chars(&f->pathVal));
  return(0);
}


static ButOut  dswinResize(ButWin *win)  {
  AbutSwin  *swin = butWin_packet(win);
  AbutFsel  *f;
  int  bw = butEnv_stdBw(butWin_env(win));

  assert(MAGIC(swin));
  f = swin->packet;
  assert(MAGIC(f));
  but_resize(f->dlBg, 0, 0, butWin_w(win), butWin_h(win));
  butList_resize(f->dList, bw, bw, butWin_w(win) - bw);
  return(0);
}


static ButOut  fswinResize(ButWin *win)  {
  AbutSwin  *swin = butWin_packet(win);
  AbutFsel  *f;
  int  bw = butEnv_stdBw(butWin_env(win));

  assert(MAGIC(swin));
  f = swin->packet;
  assert(MAGIC(f));
  but_resize(f->flBg, 0, 0, butWin_w(win), butWin_h(win));
  butList_resize(f->fList, bw, bw, butWin_w(win) - bw);
  return(0);
}


static bool  dirScan(AbutFsel *f, const char *path)  {
  DIR  *d;
  struct dirent  *ent;
  struct stat  fStat;
  Str  fName;
  int  pathLen, i;
  int  numDirs = 0, maxDirs = 10, numFiles = 0, maxFiles = 10;
  Str  *dirs, *files, *newList;

  d = opendir(path);
  if (d == NULL)  {
    dirError(f, errno, path);
    return(FALSE);
  }
  dirs = wms_malloc(maxDirs * sizeof(Str));
  files = wms_malloc(maxFiles * sizeof(Str));
  str_init(&fName);
  pathLen = strlen(path);
  for (ent = readdir(d);  ent != NULL;  ent = readdir(d))  {
    str_copyCharsLen(&fName, path, pathLen);
    str_catCharsLen(&fName, ent->d_name, NAMLEN(ent));
    if (stat(str_chars(&fName), &fStat) == 0)  {
      if (S_ISDIR(fStat.st_mode))  {
	if ((NAMLEN(ent) != 1) || (ent->d_name[0] != '.'))  {
	  if (numDirs == maxDirs)  {
	    newList = wms_malloc(maxDirs * 2 * sizeof(Str));
	    memcpy(newList, dirs, maxDirs * sizeof(Str));
	    wms_free(dirs);
	    dirs = newList;
	    maxDirs *= 2;
	  }
	  assert(numDirs < maxDirs);
	  str_initCharsLen(&dirs[numDirs++], ent->d_name, NAMLEN(ent));
	}
      } else  {
	if (maskMatch(butTextin_get(f->maskIn), ent->d_name, NAMLEN(ent)))  {
	  if (numFiles == maxFiles)  {
	    newList = wms_malloc(maxFiles * 2 * sizeof(Str));
	    memcpy(newList, files, maxFiles * sizeof(Str));
	    wms_free(files);
	    files = newList;
	    maxFiles *= 2;
	  }
	  assert(numFiles < maxFiles);
	  str_initCharsLen(&files[numFiles++], ent->d_name, NAMLEN(ent));
	}
      }
    }
  }
  str_deinit(&fName);
  closedir(d);
  if (numDirs > 1)  {
    qsort(dirs, numDirs, sizeof(Str), str_alphaCmp);
  }
  if (numFiles > 1)  {
    qsort(files, numFiles, sizeof(Str), str_alphaCmp);
  }
  butCan_resizeWin(f->dSwin->win, 0,
		   numDirs * butEnv_fontH(f->abut->env, 0) +
		   butEnv_stdBw(f->abut->env) * 2, TRUE);
  butList_setLen(f->dList, numDirs);
  for (i = 0;  i < numDirs;  ++i)  {
    butList_changeLine(f->dList, i, str_chars(&dirs[i]));
    str_deinit(&dirs[i]);
  }
  wms_free(dirs);
  butCan_resizeWin(f->fSwin->win, 0,
		   numFiles * butEnv_fontH(f->abut->env, 0) +
		   butEnv_stdBw(f->abut->env) * 2, TRUE);
  butList_setLen(f->fList, numFiles);
  for (i = 0;  i < numFiles;  ++i)  {
    butList_changeLine(f->fList, i, str_chars(&files[i]));
    str_deinit(&files[i]);
  }
  wms_free(files);
  return(TRUE);
}


static ButOut  dirPressed(But *but, int line)  {
  AbutFsel  *f = but_packet(but);
  Str  newDir;

  assert(MAGIC(f));
  but_setFlags(f->pathIn, BUT_NOKEY);
  str_initChars(&newDir, str_chars(&f->pathVal));
  str_catChars(&newDir, butList_get(but, line));
  fixPath(&f->pathVal, str_chars(&newDir));
  butTextin_set(f->pathIn, str_chars(&f->pathVal), FALSE);
  dirScan(f, str_chars(&f->pathVal));
  str_deinit(&newDir);
  return(0);
}


static ButOut  filePressed(But *but, int line)  {
  AbutFsel  *f = but_packet(but);

  assert(MAGIC(f));
  if (strcmp(butTextin_get(f->in), butList_get(but, line)))  {
    /* Change the default file name. */
    butTextin_set(f->in, butList_get(but, line), FALSE);
  } else  {
    /* Accept this file. */
    okPressed(f->ok);
  }
  return(0);
}


static void  fixPath(Str *pathOut, const char *pathIn)  {
  int  i;
  
  str_clip(pathOut, 0);
  if (pathIn[0] != '/')  {
    addCwd(pathOut);
  } else
    ++pathIn;
  str_catChar(pathOut, '/');
  assert(str_len(pathOut) > 0);
  while (*pathIn)  {
    if ((pathIn[0] == '.') && (pathIn[1] == '.') &&
	((pathIn[2] == '/') || (pathIn[2] == '\0')))  {
      for (i = str_len(pathOut) - 1;
	   (i >= 1) && (str_chars(pathOut)[i - 1] != '/');
	   --i);
      if (i >= 1)
	str_clip(pathOut, i);
      pathIn += 2;
      if (*pathIn)
	++pathIn;
    } else if ((pathIn[0] == '.') &&
	       ((pathIn[1] == '/') || (pathIn[1] == '\0')))  {
      ++pathIn;
      if (*pathIn)
	++pathIn;
    } else if (pathIn[0] == '/')  {
      ++pathIn;
    } else  {
      while ((*pathIn != '/') && (*pathIn != '\0'))  {
	str_catChar(pathOut, *(pathIn++));
      }
      str_catChar(pathOut, '/');
      if (*pathIn)
	++pathIn;
    }
  }
  assert(str_len(pathOut) > 0);
  assert(str_chars(pathOut)[0] == '/');
}


static void  addCwd(Str *pathOut)  {
  char  dirBuf[1024], *result, *tmpBuf;
  int  len = sizeof(dirBuf);

  result = getcwd(dirBuf, sizeof(dirBuf));
  if (result == NULL)  {
    while (errno == ERANGE)  {
      len *= 2;
      tmpBuf = wms_malloc(len);
      result = getcwd(tmpBuf, len);
      if (result != NULL)  {
	str_catChars(pathOut, tmpBuf);
	wms_free(tmpBuf);
	return;
      }
      wms_free(tmpBuf);
    }
    /* Another error.  We can't get the cwd. */
    str_catChars(pathOut, "/");
    return;
  }
  str_catChars(pathOut, dirBuf);
}


static bool  maskMatch(const char *mask, const char *fName, int fLen)  {
  assert(fLen >= 0);
  while (fLen > 0)  {
    switch(*mask)  {
    case '\0':
      return(FALSE);
    case '*':
      while (mask[1] == '*')
	++mask;
      if (fLen == 1)  {
	return(mask[1] == '\0');
      }
      if (fName[1] == mask[1])  {
	if (maskMatch(mask + 1, fName + 1, fLen - 1))
	  return(TRUE);
      }
      break;
    default:
      if (*mask != *fName)
	return(FALSE);
      ++mask;
      break;
    }
    ++fName;
    --fLen;
  }
  return(*mask == '\0');
}


static void  dirError(AbutFsel *f, int errnov, const char *badDir)  {
  Str  fullError;
  AbutMsgOpt  okBut;

  if (f->msg)  {
    abutMsg_destroy(f->msg, FALSE);
    f->msg = NULL;
  }
  okBut.name = f->abut->ok;
  okBut.callback = msgOkBut;
  okBut.packet = f;
  okBut.keyEq = NULL;
  str_init(&fullError);
  str_print(&fullError, abutFsel_dirErrMessage, strerror(errnov), badDir);
  f->msg = abutMsg_optCreate(f->abut, f->win, 2, str_chars(&fullError),
			     msgDead, f, 1, &okBut);
}


static ButOut  msgDead(void *packet)  {
  AbutFsel  *f = packet;

  assert(MAGIC(f));
  f->msg = NULL;
  return(0);
}


static ButOut  msgOkBut(But *but)  {
  AbutFsel  *f = but_packet(but);

  assert(MAGIC(f));
  abutMsg_destroy(f->msg, FALSE);
  f->msg = NULL;
  return(0);
}
