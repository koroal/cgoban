/*
 * wmslib/include/but/net.h, part of wmslib (Library functions)
 * Copyright (C) 1994-1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 *
 * Includes for button networking.
 */

#ifndef  _BUTNET_H_
#define  _BUTNET_H_  1

#include <but/rcur.h>

/**********************************************************************
 * Constants
 **********************************************************************/

/* Public. */
#define  BUTNET_MAXCMD  (16*1024)  /* Max length of 1 net command. */

/* Private. */
/*
 * These really should be an enum, but then I'd be treating the enum like an
 *   int when I do int_stdInt32() to it, so I may as well just leave it as
 *   an int.
 */
#define  BUTNET_PROTOWHO  0
#define  BUTNET_CLOSE     1
#define  BUTNET_USERDATA  2
#define  BUTNET_MOUSE     3
#define  BUTNET_MOUSEACK  4
#define  BUTNET_NEWFLAGS  5
#define  BUTNET_BUTSPEC   6

/**********************************************************************
 * Types
 **********************************************************************/

/* Private. */
struct ButLnet_struct  {
  struct ButEnv_struct  *env;
  void  *packet;
  bool  valid, error;
  int   errNum;
  char  *protocol;
  ButOut  (*ocallback)(struct ButRnet_struct *net);
  ButOut  (*callback)(struct ButRnet_struct *net, void *cmd, int cmdLen);
  ButOut  (*ccallback)(struct ButRnet_struct *net, const char *reason);
  int   fd;
  MAGIC_STRUCT
};  /* ButLnet typedef in but.h */

typedef enum  {
  butRnetState_pwWait, butRnetState_pwAccWait, butRnetState_accWait,
  butRnetState_open, butRnetState_closing, butRnetState_closed
} ButRnetState;

typedef struct ButNetMsg_struct  {
  StdInt32  length;
  StdInt32  type;
  union  {
    struct  {
      StdInt32 context;  /* This is the client's new context. */
      char  proto[100];
      char  who[100];
    } protoWho;
    char  close[BUTNET_MAXCMD];
    char  userData[BUTNET_MAXCMD];
    struct  {
      StdInt32  context, win, type, x,y, w,h;
    } mouse;
    /* mouseAck needs no data. */
    struct  {
      StdInt32  butId;
      StdInt32  newFlags;
    } newFlags;
    struct  {
      StdInt32  butId;
      char  butData[BUTNET_MAXCMD - sizeof(StdInt32)];
    } butSpec;
  } perType;
} ButNetMsg;

struct ButRnet_struct  {
  struct ButEnv_struct  *env;
  int  partner, rmtPartner;
  void  *packet;
  ButRnetState state;
  bool  error, lookupError, butNetError;
  int   errNum;
  const char  *who, *protocol;
  ButOut  (*ocallback)(struct ButRnet_struct *net);
  ButOut  (*callback)(struct ButRnet_struct *net, void *cmd, int cmdLen);
  ButOut  (*ccallback)(struct ButRnet_struct *net, const char *reason);
  int  wBufSize, wBufStart, wBufLen;
  char  *wBuffer;
  int  rBufAmt;  /* How much stuff in the rBuffer? */
  int  rBufLen;
  int  rBufMsgLen;
  struct ButNetMsg_struct  rBuffer;
  int   fd;

  int  lWinId, lx,ly, lw,lh, lType;
  int  lPress, lTwitch;
  ButRcur  rc;
  bool  ackNeeded, mouseMove;
  MAGIC_STRUCT
};  /* ButLnet typedef in but.h */

/**********************************************************************
 * Functions
 **********************************************************************/

/* Public. */
#define  butRnet_valid(net)     ((net)->state == butRnetState_open)
#define  butRnet_error(net)     ((net)->error)
#define  butRnet_lookupError(net)   ((net)->lookupError)
#define  butRnet_specialError(net)  ((net)->specialError)
#define  butRnet_errNum(net)    ((net)->errNum)
#define  butRnet_packet(net)    ((net)->packet)
#define  butRnet_who(net)       ((net)->who)
#define  butRnet_protocol(net)  ((net)->protocol)

#define  butLnet_valid(net)   ((net)->valid)
#define  butLnet_error(net)   ((net)->error)
#define  butLnet_errNum(net)  ((net)->errNum)
#define  butLnet_packet(net)  ((net)->packet)
/*
 * butLnet_create and butRnet_create will return NULL if the system was
 *   compiled without sockets available.
 */
extern ButLnet  *butLnet_create(ButEnv *env, int port, void *packet,
				ButOut ocallback(ButRnet *net),
				ButOut callback(ButRnet *net, void *cmd,
						int cmdLen),
				ButOut ccallback(ButRnet *net,
						 const char *reason));
extern void  butLnet_destroy(ButLnet *net);

extern ButRnet  *butRnet_create(ButEnv *env, const char *address, int port,
				void *packet,
				ButOut ocallback(ButRnet *net),
				ButOut callback(ButRnet *net, void *cmd,
						int cmdLen),
				ButOut ccallback(ButRnet *net,
						 const char *reason));
extern void  butRnet_accept(ButRnet *net);
extern void  butRnet_send(ButRnet *net, void *buffer, int len);
extern void  butRnet_destroy(ButRnet *net, const char *reason);

/* Private. */
extern void  butRnet_mMove(ButEnv *env, int winId, int lx, int ly,
			   int lw, int lh, ButCur type);
extern void  butRnet_butSpecSend(But *but, void *buffer, int len);
extern void  butRnet_newFlags(ButEnv *env, int butId, uint newFlags);

#endif  /* _BUTNET_H_ */
