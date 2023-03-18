/*
 * src/client/conn.h, part of Complete Goban (game program)
 * Copyright (C) 1995-1996 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _CLIENT_CONN_H_
#define  _CLIENT_CONN_H_  1

#ifndef  _CLIENT_SERVER_H_
#include "server.h"
#endif
#ifndef  _CGOBAN_H_
#include "../cgoban.h"
#endif
#ifndef  _WMS_STR_H_
#include <wms/str.h>
#endif


/**********************************************************************
 * Data Types
 **********************************************************************/
typedef enum  {
  cliConnErr_openSocket, cliConnErr_lookup, cliConnErr_connect, cliConnErr_ok
} CliConnErr;


typedef struct CliConn_struct  {
  Cgoban  *cg;
  int  fd;
  CliConnErr  err;
  int  serverNum;
  CliServer  server;
  bool  directConn;
  bool  loginMode;
  union  {
    int  errNum;  /* Only valid when err == openSocket or connect */
    const char  *serverName;  /* Only valid when err = lookup */
  } errOut;
  int  inLen, inSize, outLen, outSize;
  int  telnetState;  /* Should probably be an enum. */
  char  *inBuf, *outBuf;
  Str  outMsgs;
  bool  promptReady;
  uchar  loChar, hiChar;  /* Bounds of chars you let through. */

  void  (*newData)(void *packet, const char *dataIn, int dataLen);
  void  *packet;

  MAGIC_STRUCT
} CliConn;


/**********************************************************************
 * Functions
 **********************************************************************/

/*
 * newdata should return the amount of data that has been read and can
 *   be thrown out of the buffer.  0 is a perfectly legitimate amount to
 *   return, in which case you won't be bothered again until more data
 *   shows up.
 * If newData is called with NULL as the dataIn value, then dataLen contains
 *   the errno that killed the connection.
 */
extern CliConn  *cliConn_init(CliConn *cc, Cgoban *cg, int serverNum,
			      CliServer protocol, uchar loChar, uchar hiChar,
			      void (*newData)(void *packet, const char *dataIn,
					      int dataLen),
			      void *packet);
#define cliConn_create(cg, s, nd, p)  \
  cliConn_init(wms_malloc(sizeof(CliConn)), (cg), (s), (nd), (p))
						   
extern CliConn  *cliConn_deinit(CliConn *cc);
#define cliConn_destroy(cc)  wms_free(cliConn_deinit(cc))
extern void  cliConn_send(CliConn *conn, const char *str);
extern void  cliConn_prompt(CliConn *conn);

#endif  /* _CLIENT_CONN_H_ */
