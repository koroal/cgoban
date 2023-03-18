/*
 * src/client/conn.c, part of Complete Goban (game program)
 * Copyright (C) 1995-1996 William Shubert
 * See "configure.h.in" for more copyright information.
 */

#include <configure.h>

#ifdef  STDC_HEADERS
#include <stdlib.h>
#include <unistd.h>
#endif  /* STDC_HEADERS */
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#if  HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if  HAVE_SYS_IN_H
#include <sys/in.h>
#endif
#if  HAVE_SYS_INET_H
#include <sys/inet.h>
#endif
#include <netdb.h>
#if  HAVE_ARPA_NAMESER_H

#include <arpa/nameser.h>
#endif
#if  HAVE_RESOLV_H
#include <resolv.h>
#endif
#include <pwd.h>
#include <arpa/telnet.h>

#include <wms.h>
#include <wms/clp.h>
#include <wms/str.h>
#include <but/but.h>
#include <abut/msg.h>
#include "../cgoban.h"
#include "../msg.h"
#include "server.h"
#ifdef  _CLIENT_CONN_H_
        LEVELIZATION ERROR
#endif
#include "conn.h"


/**********************************************************************
 * Constants
 **********************************************************************/
#define  TRACELOG  0
static const int  defaultInLen = (8 * 1024);
static const int  defaultOutLen = 1024;


/**********************************************************************
 * Forward Declarations
 **********************************************************************/
static ButOut  newNetData(void *packet, int fd);
static void  expandBuf(char **buf, int *size, int len);
static int  telnetMunge(CliConn *cc, int amtRead);
static ButOut  writeData(void *packet, int fd);
static void  sendNextCommand(CliConn *conn);
static void  transmitData(CliConn *cc, const char *str, int strLen);
static void  sockConn(CliConn *conn);
static void  progConn(CliConn *conn);


/**********************************************************************
 * Functions
 **********************************************************************/
CliConn  *cliConn_init(CliConn *cc, Cgoban *cg, int serverNum,
		       CliServer protocol,
		       uchar loChar, uchar hiChar,
		       void (*newData)(void *packet, const char *dataIn,
				      int dataLen),
		       void *packet)  {
  const char  *cliDirect;

  assert(MAGIC(cg));
  MAGIC_SET(cc);
  cc->cg = cg;
  cc->fd = -1;
  cc->inBuf = NULL;
  cc->outBuf = NULL;
  cc->err = cliConnErr_ok;
  cc->serverNum = serverNum;
  cc->server = protocol;
  cc->loginMode = TRUE;
  if (protocol == cliServer_igs)
    str_init(&cc->outMsgs);

  cliDirect = clp_getStrNum(cg->clp, "client.direct", serverNum);
  cc->directConn = (cliDirect[0] == 't');
  if (cc->directConn)  {
    sockConn(cc);
  } else  {
    assert(cliDirect[0] == 'f');
    progConn(cc);
  }    
  if (cc->err != cliConnErr_ok)
    return(cc);

  fcntl(cc->fd, F_SETFL, O_NONBLOCK);
  butEnv_addFile(cg->env, BUT_READFILE, cc->fd, cc, newNetData);
  
  cc->inLen = 0;
  cc->telnetState = 0;
  cc->inBuf = wms_malloc(cc->inSize = defaultInLen);
  cc->outLen = 0;
  cc->outBuf = wms_malloc(cc->outSize = defaultOutLen);
  cc->promptReady = TRUE;
  if (loChar <= '\r')   /* DON'T pass those damn \r's! */
    loChar = '\r' + 1;
  cc->loChar = loChar;
  cc->hiChar = hiChar;

  cc->newData = newData;
  cc->packet = packet;
  return(cc);
}


CliConn  *cliConn_deinit(CliConn *cc)  {
  assert(MAGIC(cc));
  if (cc->fd >= 0)  {
    butEnv_rmFile(cc->cg->env, BUT_READFILE, cc->fd);
    close(cc->fd);
  }
  if (cc->inBuf)
    wms_free(cc->inBuf);
  if (cc->outBuf)
    wms_free(cc->outBuf);
  if (cc->server == cliServer_igs)
    str_deinit(&cc->outMsgs);
  MAGIC_UNSET(cc);
  return(cc);
}

  
static ButOut  newNetData(void *packet, int fd)  {
  CliConn  *cc = packet;
  int  amtRead, totalAmtRead, readLen, lineStart, lineEnd;

  assert(MAGIC(cc));
  totalAmtRead = 0;
  do  {
    if (cc->inLen + totalAmtRead + 1 >= cc->inSize)
      expandBuf(&cc->inBuf, &cc->inSize, cc->inLen);
    readLen = cc->inSize - (cc->inLen + totalAmtRead + 1);
    amtRead = read(cc->fd, cc->inBuf + cc->inLen, readLen);
    if (amtRead < 0)  {
      if (errno == EAGAIN)
	amtRead = 0;
      else  {
	cc->newData(cc->packet, NULL, errno);
	return(0);
      }
    } else if (amtRead == 0)  {
      cc->newData(cc->packet, NULL, errno);
      return(0);
    }
    totalAmtRead += amtRead;
  } while (amtRead == readLen);
  totalAmtRead = telnetMunge(cc, totalAmtRead);
  if (totalAmtRead < 0)  {
    cc->newData(cc->packet, NULL, EPIPE);
    return(0);
  }
  if (totalAmtRead > 0)  {
    cc->inLen += totalAmtRead;
    assert(cc->inLen < cc->inSize);
    lineStart = 0;
    for (;;)  {
      lineEnd = lineStart;
      while ((lineEnd < cc->inLen) && (cc->inBuf[lineEnd] != '\n'))
	++lineEnd;
      if ((lineEnd < cc->inLen) ||
	  (cc->loginMode &&
	   (((lineEnd - lineStart == 7) &&
	     !strncmp(cc->inBuf + lineStart, "Login: ", 7)) ||
	    ((lineEnd - lineStart == 10) &&
	     !strncmp(cc->inBuf + lineStart, "Password: ", 10)) ||
	    ((lineEnd - lineStart == 3) &&
	     !strncmp(cc->inBuf + lineStart, "#> ", 3)))))  {
	cc->inBuf[lineEnd] = '\0';
#if  TRACELOG
    printf("%s\n", cc->inBuf + lineStart);
#endif
	cc->newData(cc->packet, cc->inBuf + lineStart, lineEnd - lineStart);
	lineStart = lineEnd + 1;
      } else
	break;
    }
    if (lineStart >= cc->inLen)  {
      cc->inLen = 0;
    } else if (lineStart > 0)  {
      memmove(cc->inBuf, cc->inBuf + lineStart, cc->inLen - lineStart);
      cc->inLen -= lineStart;
    }
  }
  return(0);
}


static void  expandBuf(char **buf, int *size, int len)  {
  char  *newBuf;
  int  newSize;

  newSize = *size * 2;
  newBuf = wms_malloc(newSize);
  memcpy(newBuf, *buf, len);
  if (*buf)
    wms_free(*buf);
  *buf = newBuf;
  *size = newSize;
}


/*
 * This returns the number of bytes left after stripping out \r's, telnet
 *   escape codes, etc.
 */
static int  telnetMunge(CliConn *cc, int amtRead)  {
  unsigned char  c;
  char  *src, *dest;

  src = dest = cc->inBuf + cc->inLen;
  while (src < cc->inBuf + cc->inLen + amtRead)  {
    c = (unsigned char)*(src++);
    switch (cc->telnetState) {
    case 0:                     /* Haven't skipped over any control chars or
                                   telnet commands */
      if (c == IAC) {
        cc->telnetState = 1;
      } else if ((c == '\n') || ((c >= cc->loChar) && (c <= cc->hiChar)))  {
	*(dest++) = c;
      }
      break;
    case 1:                /* got telnet IAC */
      if (c == IP)  {
	return(-1);            /* ^C = logout */
      } else if (c == DO)  {
        cc->telnetState = 4;
      } else if ((c == WILL) || (c == DONT) || (c == WONT))  {
        cc->telnetState = 3;   /* this is cheesy, but we aren't using 'em */
      } else  {
        cc->telnetState = 0;
      }
      break;
    case 3:                     /* some telnet junk we're ignoring */
      cc->telnetState = 0;
      break;
    case 4:                     /* got IAC DO */
      /*
       * We really should handle this right.  But it doesn't seem to matter
       *   when you're connecting to one of the servers.  I know that it
       *   doesn't matter if you're using NNGS, because I wrote the NNGS
       *   network code and I don't know what these escapes are supposed
       *   to mean.
       */
      cc->telnetState = 0;
      break;
    default:
      assert(0);
    }
  }
  return(dest - (cc->inBuf + cc->inLen));
}


void  cliConn_send(CliConn *conn, const char *str)  {
#if  TRACELOG
  printf("***SEND  {\n%s***SEND  }\n", str);
#endif
  if ((conn->server == cliServer_nngs) || conn->loginMode)  {
    transmitData(conn, str, strlen(str));
  } else  {
    str_catChars(&conn->outMsgs, str);
    if (conn->promptReady)
      sendNextCommand(conn);
  }
}


void  cliConn_prompt(CliConn *conn)  {
  if (conn->server == cliServer_igs)  {
    conn->promptReady = TRUE;
    sendNextCommand(conn);
  }
}


/*
 * The IGS has a bug where only the first command in a TCP packet is executed;
 *   the rest are ignored.  Yes, it really is that stupid.  Here I take out
 *   a single command and transmit it.  If you're connected to the NNGS, then
 *   this is all unnecessary so cliConn_send() would have jumped straight
 *   to transmitData().
 */
static void  sendNextCommand(CliConn *conn)  {
  const char  *crPos;
  int  sendLen;
  Str  temp;

  assert(conn->server == cliServer_igs);
  assert(conn->promptReady);
  if (str_len(&conn->outMsgs))  {
    crPos = strchr(str_chars(&conn->outMsgs), '\n');
    assert(crPos != NULL);
    sendLen = (crPos - str_chars(&conn->outMsgs)) + 1;
    transmitData(conn, str_chars(&conn->outMsgs), sendLen);
    if (sendLen == str_len(&conn->outMsgs))  {
      str_clip(&conn->outMsgs, 0);
    } else  {
      str_initStr(&temp, &conn->outMsgs);
      str_copyCharsLen(&conn->outMsgs,
		       str_chars(&temp) + sendLen, str_len(&temp) - sendLen);
      str_deinit(&temp);
    }
    conn->promptReady = FALSE;
  }
}


static void  transmitData(CliConn *cc, const char *str, int strLen)  {
  int  amtWritten;

  assert(MAGIC(cc));
  if (cc->outLen == 0)  {
    amtWritten = write(cc->fd, str, strLen);
    if (amtWritten < 0)
      amtWritten = 0;
  } else
    amtWritten = 0;
  if (amtWritten < strLen)  {
    while (cc->outLen + amtWritten > cc->outSize)
      expandBuf(&cc->outBuf, &cc->outSize, cc->outLen);
    memcpy(cc->outBuf + cc->outLen, str + amtWritten, strLen - amtWritten);
    if (cc->outLen == 0)  {
      butEnv_addFile(cc->cg->env, BUT_WRITEFILE, cc->fd, cc, writeData);
    }
    cc->outLen += strLen - amtWritten;
  }
}


static ButOut  writeData(void *packet, int fd)  {
  CliConn  *cc = packet;
  int  amtWritten;

  assert(MAGIC(cc));
  assert(cc->fd == fd);
  assert(cc->outLen > 0);
  amtWritten = write(fd, cc->outBuf, cc->outLen);
  if (amtWritten < 0)
    amtWritten = 0;
  if (amtWritten == cc->outLen)  {
    cc->outLen = 0;
    butEnv_rmFile(cc->cg->env, BUT_WRITEFILE, cc->fd);
  } else  {
    memmove(cc->outBuf, cc->outBuf + amtWritten, cc->outLen - amtWritten);
    cc->outLen -= amtWritten;
  }
  return(0);
}


static void  sockConn(CliConn *conn)  {
  Cgoban *cg = conn->cg;
  const char  *address, *portStr;
  struct hostent  *hp;
  struct sockaddr_in  sa;
  int port;
  bool  lookupNeeded;
  int  matches, addrA, addrB, addrC, addrD;

  address = clp_getStrNum(cg->clp, "client.address", conn->serverNum);
  matches = sscanf(address, "%d.%d.%d.%d", &addrA, &addrB, &addrC, &addrD);
  lookupNeeded = (matches != 4);

  portStr = clp_getStrNum(cg->clp, "client.port", conn->serverNum);
  port = atoi(portStr);
  conn->fd = socket(AF_INET, SOCK_STREAM, 0);
  if (conn->fd < 0)  {
    conn->err = cliConnErr_openSocket;
    conn->errOut.errNum = errno;
    return;
  }
  sa.sin_family = AF_INET;
  if (lookupNeeded)  {
    if ((hp = gethostbyname(address)) == NULL)  {
      close(conn->fd);
      conn->fd = -1;
      conn->err = cliConnErr_lookup;
      conn->errOut.serverName = address;
      return;
    }
    memcpy(&sa.sin_addr, hp->h_addr, hp->h_length);
  } else  {
    sa.sin_addr.s_addr = htonl((addrA << IN_CLASSA_NSHIFT) |
			       (addrB << IN_CLASSB_NSHIFT) |
			       (addrC << IN_CLASSC_NSHIFT) |
			       addrD);
  }
  sa.sin_port = htons(port);
  if (connect(conn->fd, (struct sockaddr *)&sa, sizeof(sa)) < 0)  {
    close(conn->fd);
    conn->fd = -1;
    conn->err = cliConnErr_connect;
    conn->errOut.errNum = errno;
    return;
  }
}


static void  progConn(CliConn *conn)  {
  Cgoban *cg = conn->cg;
  const char  *cmd;
  int  sockets[2];
  int  result;
  int  pid, fd;

  cmd = clp_getStrNum(cg->clp, "client.connCmd", conn->serverNum);
  result = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
  if (result < 0)  {
    conn->err = cliConnErr_connect;
    conn->errOut.errNum = errno;
    return;
  }
  pid = fork();
  if (pid < 0)  {
    close(sockets[0]);
    close(sockets[1]);
    conn->err = cliConnErr_connect;
    conn->errOut.errNum = errno;
    return;
  }
  if (pid == 0)  {
    /* The child. */
    for (fd = 0;  fd < 100;  ++fd)  {
      if ((fd != 2) &&  /* Don't close stderr! */
	  (fd != sockets[1]))
	close(fd);
    }
    if (dup(sockets[1]) != 0)  {
      fprintf(stderr, "I couldn't fdup into stdin!\n");
      exit(1);
    }
    if (dup(sockets[1]) != 1)  {
      fprintf(stderr, "I couldn't fdup into stdout!\n");
      exit(1);
    }
    result = system(cmd);
    if (result != 0)  {
      Str  msg;

      str_init(&msg);
      str_print(&msg, msg_commandError, cmd, result);
      fprintf(stderr, "%s\n", str_chars(&msg));
      /*
       * createMsgWindow doesn't work here, since I'm in a subprocess.
       * Oh well.
       *
       * cgoban_createMsgWindow(cg, "Cgoban Error", str_chars(&msg));
       */
      str_deinit(&msg);
    }
    exit(result);
  } else  {
    close(sockets[1]);
    conn->fd = sockets[0];
  }
}
