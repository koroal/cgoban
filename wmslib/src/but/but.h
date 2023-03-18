/*
 * wmslib/include/but/but.h, part of wmslib (Library functions)
 * Copyright (C) 1994-1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 *
 * Includes for auxiliary button code.
 */

#ifndef  _BUT_BUT_H_
#define  _BUT_BUT_H_  1

#include <wms.h>
#include <wms/snd.h>
#if  HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

/**********************************************************************
 * Forward declarations
 **********************************************************************/
typedef struct ButTimer_struct ButTimer;
typedef struct ButWin_struct ButWin;
typedef struct ButLnet_struct ButLnet;
typedef struct ButRnet_struct ButRnet;
typedef struct ButEnv_struct  ButEnv;
typedef struct But_struct  But;

#include <but/rcur.h>

/**********************************************************************
 * Constants
 **********************************************************************/
#define  BUT_DCLICK  350  /* Time between double clicks; should probably */
                          /*   be settable by the user.                  */

/**********************************************************************
 * Types
 **********************************************************************/
#define  BUTOUT_STOPWAIT   0x01
#define  BUTOUT_ERR        0x02
#define  BUTOUT_CAUGHT     0x04
#define  BUTOUT_STILLBUTS  0x08
#define  BUTOUT_IGNORE     0x10  /* Used by canvases. */
typedef uint  ButOut;
typedef ButOut  ButWinFunc(struct ButWin_struct *win);


typedef struct ButAction_struct  {
  ButOut  (*mMove)(But *but, int x, int y);
  ButOut  (*mLeave)(But *but);
  ButOut  (*mPress)(But *but, int butnum, int x, int y);
  ButOut  (*mRelease)(But *but, int butnum, int x, int y);
  ButOut  (*kPress)(But *but, const char *str, KeySym sym);
  ButOut  (*kRelease)(But *but, const char *str, KeySym sym);
  void    (*draw)(But *but, int x, int y, int w, int h);
  ButOut  (*destroy)(But *but);
  void    (*newFlags)(But *but, uint nfl);
  ButOut  (*netMessage)(But *but, void *msg, int msgLen);
  /* resize returns TRUE if redraws are needed for old & new position. */
  bool    (*resize)(But *but, int oldX, int oldY, int oldW, int oldH);
} ButAction;

typedef struct ButColor_struct {
  int  red, green, blue, greyLevel;
} ButColor;

typedef struct ButSet_struct  {
  int  numButs;
  bool  dynamic;  /* If TRUE, then buts should be freed when you're done. */
  But  **buts;
  int  maxButs;
  MAGIC_STRUCT
} ButSet;


typedef struct ButWinArea_struct  {
  int x, y, w, h;
} ButWinArea;


struct ButWin_struct  {
  struct ButWin_struct  *parent;  /* NULL unless you're a canvas. */
  struct But_struct  *parentBut;  /* NULL unless you're a canvas. */
  void  *packet, *iPacket;
  Window  win, physWin;
  struct ButWin_struct  *iconWin;

  const char *name;
  bool  iconic, isIcon;
  int  x,y,w,h, xOff, yOff, minWRatio, minHRatio, maxWRatio, maxHRatio;
  int  minW, minH, maxW, maxH, wStep, hStep;
  int  logicalW, logicalH;  /* Same as w&h except for canvases. */

  bool  resized, resizeNeeded, redrawReady;
  ButWinArea  *redraws;
  int  numRedraws, maxRedraws;

  int  id;
  bool  mapped;
  ButWinFunc  *unmap, *map, *resize, *destroy, *quit;
  ButEnv  *env;
  int  minLayer, maxLayer;
  struct ButSet_struct  butsNoDraw;
  But  *lock, *butIn;
  But *keyBut;
  int  numCols, maxCols;
  struct ButCol_struct  *cols;
  MAGIC_STRUCT
};

typedef struct ButWrite_struct  {
  double  width;
  void  *packet;
  void (*draw)(void *packet, ButWin *win, int x, int y, int w, int h);
} ButWrite;
  
#define  BUTWRITE_MINPRINT  '\t'
#define  BUTWRITE_MAXCHARS  (BUTWRITE_MINPRINT*256-256)

typedef struct ButFdCallback_struct  {
  ButOut  (*callback)(void *packet, int fd);
  void  *packet;
} ButFdCallback;

struct  ButEnv_struct  {
  char  *protocol;
  void  *packet;
  Display  *dpy;
  int  (*shutdown)(Display *dpy);
  GC  gc, gc2;
  Pixmap  stipDisable;
  bool  colorp;  /* TRUE if color, FALSE if b/w. */
  int  numColors;
  ulong  *colors;
  Pixmap greyMaps[17];
  Pixmap *colorPmaps;
  int  numFonts;
  XFontStruct **fonts;
  int  font0h, depth, rootW, rootH;
  int  stdButBw;
  Atom  prop;
  bool  (*sReq)(ButEnv *env, XSelectionRequestEvent *xsre);
  int  (*sClear)(ButEnv *env);
  int  (*sNotify)(ButEnv *env, XSelectionEvent *xsnot);
  struct ButWin_struct  **winlist;
  int  minWindows, wllen, wlmax;
  But  *butIn, *lockBut;
  struct ButWin_struct  *last_mwin;
  int  last_mx, last_my;
  ButEnv  *next;
  struct ButWrite_struct  write[BUTWRITE_MAXCHARS];
  uint  keyModifiers;
  Time  eventTime;  /* But & kbd PRESS or RELEASE or MOVE events. */
  int  eventNum;   /* But & kbd PRESS events ONLY. */

  int  maxFd, maxGFds[3];
  fd_set  fMasks[3];
  ButFdCallback  *fCallbacks[3];

  /* These are set up in the rc (remote cusor) file. */
  int  cur_mnum, new_mnum;
  Cursor  cursors[BUTCUR_NUM];
  Pixmap  cpic[BUTCUR_NUM], cmask[BUTCUR_NUM], *cstore;
  struct ButRnet_struct  **partners;

  int  partner, numPartners;
  int  maxButIds, maxWinIds;
  But  **id2But;
  struct ButWin_struct  **id2Win;

  ButCur  curnum, curlast;
  Window  curwin;
  But  *curhold;
  MAGIC_STRUCT
};


typedef struct ButKey_struct  {
  KeySym key;
  uint  modifiers;
  uint  modMask;
} ButKey;


struct But_struct  {
  void  *uPacket;  /* Packet of user information.        */
  void  *iPacket;  /* Packet unique to this button type. */
  struct ButWin_struct  *win;
  int  layer;
  int  x, y, w, h, id;
  unsigned  flags;
  const ButKey  *keys;  /* A list of keys that map to this button. */
  const ButAction *action;
  void  (*destroyCallback)(struct But_struct *but);
  MAGIC_STRUCT
};

typedef struct ButRow_struct {
  int  startY, numButs, maxButs;
  But  **buts;
  MAGIC_STRUCT
} ButRow;

typedef struct ButCol_struct {
  int  startX;
  int  numRows, maxRows;
  struct ButRow_struct  *rows;
  MAGIC_STRUCT
} ButCol;


#define  butEnv_setXFg(env, colornum)                            \
  do  {                                                          \
    if (env->colorp)                                             \
      XSetForeground(env->dpy, env->gc, env->colors[colornum]);  \
    else                                                         \
      XSetTile(env->dpy, env->gc, env->colorPmaps[colornum]);    \
  } while (0)

#define  butEnv_stdFill(env)                        \
  do  {                                             \
    if (env->colorp)                                \
      XSetFillStyle(env->dpy, env->gc, FillSolid);  \
    else                                            \
      XSetFillStyle(env->dpy, env->gc, FillTiled);  \
  } while (0)

#define  butEnv_setXFg2(env, colornum)                            \
  do  {                                                           \
    if (env->colorp)                                              \
      XSetForeground(env->dpy, env->gc2, env->colors[colornum]);  \
    else                                                          \
      XSetTile(env->dpy, env->gc2, env->colorPmaps[colornum]);    \
  } while (0)

#define  butEnv_stdFill2(env)                        \
  do  {                                              \
    if (env->colorp)                                 \
      XSetFillStyle(env->dpy, env->gc2, FillSolid);  \
    else                                             \
      XSetFillStyle(env->dpy, env->gc2, FillTiled);  \
  } while (0)

extern Atom  but_myProp, but_wmDeleteWindow, but_wmProtocols;
extern struct ButTimer_struct  *but_timerList;
extern bool  but_inEvent;


extern void  butWin_turnOnTimers(ButWin *win);
extern void  butWin_turnOffTimers(ButWin *win);

extern ButOut  butWin_delete(ButWin *win);
extern ButOut  but_dList(But *but);
extern ButOut  butWin_dList(ButWin *win);

/* From "write.c" */
extern void  butWin_write(ButWin *win, int x, int y, const char *text,
			  int font);

/* Functions available to the buttons. */
extern void  butEnv_deinit(ButEnv *env);
extern But   *but_create(ButWin *win, void *iPacket,
			 const ButAction *action);
extern void  but_init(But *but);
extern ButOut   but_delete(But *but);
#define  but_getFlags(b)  ((b)->flags)
extern void  but_newFlags(But *but, uint newflags);
extern void  but_flags(But *but, uint newflags);  /* Private. */
extern void  but_draw(But *but);
extern void  butWin_redraw(ButWin *win, int x, int y, int w, int h);
extern void  butWin_performDraws(ButWin *win);
extern void  but_resize(But *but, int x, int y, int w, int h);
extern ButOut  butWin_mMove(ButWin *win, int x, int y);
extern ButOut  butWin_mPress(ButWin *win, int x, int y, int butnum);
extern ButOut  butWin_mRelease(ButWin *win, int x, int y, int butnum);
extern ButOut  butWin_kPress(ButWin *win, const char *kstring, KeySym sym);
extern ButOut  butWin_kRelease(ButWin *env, const char *kstring, KeySym sym);

/* Functions for internal use only. */
/* In but_finder.c */
void  butSet_destroy(ButSet *butset);
extern ButWin  *butEnv_findWin(ButEnv *env, Window win);
extern void  butWin_addToTable(ButWin *win);
extern void  butWin_rmFromTable(ButWin *win);
extern void  but_addToTable(But *but);
extern void  but_delFromTable(But *but);
extern void  butWin_findButSet(ButWin *win, int x, int y, ButSet *butset);
extern void  butWin_findButSetInRegion(ButWin *win, int x,int y,
				       int w,int h, ButSet *butset);
extern But  *butWin_findButsInRegion(ButWin *win, int x,int y, int w,int h,
				     bool (*action)(But *but, void *packet),
				     void *packet);
void  butSet_addBut(ButSet *butset, But *but);
void  butSet_delBut(ButSet *butset, But *but);


/* From "ctext.c" */
extern Snd  but_downSnd, but_upSnd;


/**********************************************************************
 * Forward declarations
 **********************************************************************/

/* These are all bits for the flags arguments to button functions. */
#define  BUT_DRAWABLE    0x0001  /* Can be drawn. */
#define  BUT_PRESSABLE   0x0002  /* Can be pressed. */
#define  BUT_TWITCHABLE  0x0004  /* Changes when mouse goes over it. */
#define  BUT_KEYED       0x0008  /* Keys pressed land in this button. */
#define  BUT_PRESSTHRU   0x0010

#define  BUT_PRESSED     0x0020
#define  BUT_TWITCHED    0x0040
#define  BUT_KEYPRESSED  0x0080
#define  BUT_KEYACTIVE   0x0100  /* Read only; set if recvs keypresses. */
#define  BUT_TABLED      0x0200  /* Private.  Don't touch. */
#define  BUT_LOCKED      0x0400
#define  BUT_OPAQUE      0x0800  /* Solidly fills rectangle. */
#define  BUT_DEAD        0x1000  /* Marked for death! */
#define  BUT_NETPRESS    0x2000
#define  BUT_NETTWITCH   0x4000
#define  BUT_NETKEY      0x8000
#define  BUT_MAXBITS  16
#define  BUT_NODRAW     (BUT_DRAWABLE<<BUT_MAXBITS)    /* Write only. */
#define  BUT_NOPRESS    (BUT_PRESSABLE<<BUT_MAXBITS)   /* Write only. */
#define  BUT_NOTWITCH   (BUT_TWITCHABLE<<BUT_MAXBITS)  /* Write only. */
#define  BUT_NOKEY      (BUT_KEYED<<BUT_MAXBITS)       /* Write only. */
#define  BUT_NOTHRU     (BUT_PRESSTHRU<<BUT_MAXBITS)   /* Write only. */
#define  BUT_NETMASK    0x00e0
#define  BUT_NETSHIFT   8

#define  BUT_FG       0   /* The standard foreground for buttons. */
#define  BUT_BG       1   /* The standard background for buttons. */
#define  BUT_PBG      2   /* Background for pressed buttons. */
#define  BUT_HIBG     3   /* Highlighted buttons backgrounds. */
#define  BUT_LIT      4   /* "Lit up" edges of buttons. */
#define  BUT_SHAD     5   /* "Shadowed" edges of buttons. */
#define  BUT_ENTERBG  6   /* bg for text the user is typing. */
#define  BUT_SELBG    7   /* bg for selected text. */
#define  BUT_CHOICE   8   /* bg for chosen things. */
#define  BUT_WHITE    9   /* These are always white and black. */
#define  BUT_BLACK    10

#define  BUT_DCOLORS  11  /* The number of default colors. */

#define  butEnv_packet(env)  ((env)->packet)
#define  butEnv_dpy(env)     ((env)->dpy)
#define  butEnv_gc(env)      ((env)->gc)
#define  butEnv_color(env, cnum)   ((env)->colors[cnum])
#define  butEnv_isColor(env) ((env)->colorp)
#define  butEnv_pixmap(env, cnum)  ((env)->colorPmaps[cnum])
#define  butEnv_fontStr(env, fnum) ((env)->fonts[fnum])
#define  butEnv_stdBw(env)   ((env)->stdButBw)
#define  butEnv_keyModifiers(env)  ((env)->keyModifiers)

#define  butWin_packet(win)  ((win)->packet)
#define  butWin_env(win)     ((win)->env)
#define  butWin_xwin(bwin)   ((bwin)->win)
#define  butWin_w(win)       ((win)->logicalW)
#define  butWin_h(win)       ((win)->logicalH)
#define  butWin_viewW(win)   ((win)->w)
#define  butWin_viewH(win)   ((win)->h)
#define  butWin_x(win)       ((win)->x)
#define  butWin_y(win)       ((win)->y)
#define  butWin_xoff(win)    ((win)->xOff)
#define  butWin_yoff(win)    ((win)->yOff)
#define  butWin_setPacket(win, p)  ((win)->packet = (p))
#define  butWin_setQuit(win, q)  ((win)->quit = (q))
#define  butWin_setDestroy(win, d)  ((win)->destroy = (d))

#define  but_win(but)         ((but)->win)
#define  but_packet(but)      ((but)->uPacket)
#define  but_setPacket(but, val)  ((but)->uPacket = (val))
#define  but_x(but)           ((but)->x)
#define  but_y(but)           ((but)->y)
#define  but_w(but)           ((but)->w)
#define  but_h(but)           ((but)->h)
#define  but_setDestroyCallback(but, v)  ((but)->destroyCallback = (v))


/*
 * but_events keeps servicing events until one returns BUT_OUT_STOPWAIT.
 */
extern void  butEnv_events(ButEnv *env);
extern ButEnv  *butEnv_create(const char *protocol, const char *dpyname,
			      int shutdown(Display *dpy));
extern ButEnv  *butEnv_createNoDpy(const char *protocol);
/*
 * but_env_init() returns:
 *   0 - Black and white display or color=FALSE.
 *   1 - Couldn't allocate standard colors.  Failed.
 *   2 - Color successful.
 *   3 - Truecolor display.  Color will always be successful.
 */
extern int  butEnv_init(ButEnv *env, void *packet, const char *atomname,
			bool color);
extern void  butEnv_destroy(ButEnv *env);
extern bool  butEnv_setFont(ButEnv *env, int fontnum, const char *fontname,
			    int fparam);
#define  butEnv_fontH(env, fnum)  \
          ((env)->fonts[fnum]->ascent + (env)->fonts[fnum]->descent)
/* drawall is typically used after changing fonts, etc. */
extern void  butEnv_drawAll(ButEnv *env);
extern void  butEnv_resizeAll(ButEnv *env);
extern void  but_draw(But *but);
extern void  but_destroy(But *but);
extern void  but_resize(But *but, int x, int y, int w, int h);

#define  butEnv_localContext(env)  ((env)->localContext)
#define  butEnv_context(env)       ((env)->context)
extern void  but_setId(But *but, int id);
extern void  butWin_setId(ButWin *win, int id);

#define  BUT_READFILE   0
#define  BUT_WRITEFILE  1
#define  BUT_XFILE      2
extern void  butEnv_addFile(ButEnv *env, int group, int fd, void *packet,
			    ButOut (*callback)(void *packet, int fd));
extern void  butEnv_rmFile(ButEnv *env, int group, int fd);

extern ButWin  *butWin_iCreate(void *packet, ButEnv *env,
			       const char *name, int w, int h,
			       ButWin **iWin, bool iconic, int iW, int iH,
			       ButWinFunc *unmap,
			       ButWinFunc *map,
			       ButWinFunc *resize,
			       ButWinFunc *iResize,
			       ButWinFunc *destroy);
#define  butWin_create(p,e,n,w,h,un,ma,re,de)  \
  butWin_iCreate(p,e,n,w,h,NULL,FALSE,0,0,un,ma,re,NULL,de)
extern void  butWin_setGeom(ButWin *win, const char *geometry);
extern void  butWin_setX(ButWin *win, int x);
extern void  butWin_setY(ButWin *win, int y);
extern void  butWin_setMinW(ButWin *win, int minW);
extern void  butWin_setMinH(ButWin *win, int minH);
extern void  butWin_setMaxW(ButWin *win, int maxW);
extern void  butWin_setMaxH(ButWin *win, int maxH);
extern void  butWin_setWHRatio(ButWin *win, int w, int h);
extern void  butWin_setWHRatios(ButWin *win,
				int minW, int minH, int maxW, int maxH);
extern void  butWin_setWStep(ButWin *win, int wStep);
extern void  butWin_setHStep(ButWin *win, int hStep);
#define  butWin_getMinW(w)   ((w)->minW)
#define  butWin_getMinH(w)   ((w)->minH)
#define  butWin_getMaxW(w)   ((w)->maxW)
#define  butWin_getMaxH(w)   ((w)->maxH)
#define  butWin_getWStep(w)  ((w)->wStep)
#define  butWin_getHStep(w)  ((w)->hStep)
extern void  butWin_activate(ButWin *win);
extern void  butWin_checkDims(ButWin *win);  /* Private. */
extern void  butWin_resize(ButWin *win, int newW, int newH);
extern void  butWin_destroy(ButWin *xw);

/* grey is 0..16 */
extern ButColor  butColor_create(int r, int g, int b, int grey);
/* The output color will have bw_greyscale same as c1's. */
extern ButColor  butColor_mix(ButColor c1, int r1,
			      ButColor c2, int r2);
extern bool  butEnv_setColor(ButEnv *env, int colornum, ButColor color);
XImage  *butEnv_imageCreate(ButEnv *env, int w, int h);
void  butEnv_imageDestroy(XImage *image);

/* From "i_general.c" */
extern void  but_setFlags(But *but, uint fch);
extern void  but_setKeys(But *but, const ButKey *keys);

/* From "but_write.c" */
extern void  butEnv_setChar(ButEnv *env, double wratio, const char *id,
			    void (*draw)(void *packet, ButWin *win,
					 int x, int y, int w, int h),
			    void *packet);
extern int  butEnv_textWidth(ButEnv *env, const char *text, int fnum);
extern int  butEnv_charWidth(ButEnv *env, const char *text, int fnum);

/* From "but_textin.c" */

#define  BUT_NOCHANGE  (1 << (sizeof(int)*8-2))

#endif  /* _BUT_BUT_H_ */
