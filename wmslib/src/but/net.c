/*
 * wmslib/src/but/net.c, part of wmslib (Library functions)
 * Copyright (C) 1995-1997 William Shubert.
 * See "configure.h.in" for more copyright information.
 */

#include <configure.h>

#if  X11_DISP

#ifdef  STDC_HEADERS
#include <stdlib.h>
#include <unistd.h>
#endif  /* STDC_HEADERS */
#include <ctype.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <sys/time.h>
#ifdef  HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#include <sys/types.h>

#if  HAVE_SOCKETS

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
#if  HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif
#include <pwd.h>

#endif  /* HAVE_SOCKETS */

#include <fcntl.h>
#include <wms.h>
#include <but/but.h>
#include <but/net.h>

#if  HAVE_SOCKETS

/**********************************************************************
 * Forward Declarations
 **********************************************************************/

static ButOut  newConn(void *packet, int fd);
static ButRnet  *newRnet(ButEnv *env);
static ButOut  getNetData(void *packet, int fd);
static ButOut  finishWrite(void *packet, int fd);
static void  putProtoWho(ButRnet *net);
static void  butRnet_close(ButRnet *net, const char *reason, bool forced);
static bool  butRnet_write(ButRnet *net, ButNetMsg *msg);
static void  butRnet_sendMMove(ButRnet *net, int winId, int lx, int ly,
			       int lw, int lh, ButCur type);
static void  newFlags(ButRnet *net, int id, uint newFlags);
static ButOut  butSpec(ButEnv *env, int butId, void *buf, int bufLen);


/**********************************************************************
 * Functions
 **********************************************************************/

ButLnet  *butLnet_create(ButEnv *env, int port, void *packet,
			 ButOut ocallback(ButRnet *net),
			 ButOut callback(ButRnet *net, void *cmd, int cmdLen),
			 ButOut ccallback(ButRnet *net, const char *reason))  {
  ButLnet  *net = wms_malloc(sizeof(ButLnet));
  struct sockaddr_in  sa;

  MAGIC_SET(net);
  net->env = env;
  net->packet = packet;
  net->valid = net->error = FALSE;
  net->fd = socket(AF_INET, SOCK_STREAM, 0);
  net->ocallback = ocallback;
  net->callback = callback;
  net->ccallback = ccallback;
  if (net->fd == -1)  {
    net->error = TRUE;
    net->errNum = errno;
    return(net);
  }
  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = INADDR_ANY;
  sa.sin_port = htons(port);
  if (bind(net->fd, (struct sockaddr *)&sa, sizeof(sa)) != 0)  {
    net->error = TRUE;
    net->errNum = errno;
    close(net->fd);
    return(net);
  }
  if (listen(net->fd, 2) != 0)  {
    net->error = TRUE;
    net->errNum = errno;
    close(net->fd);
    return(net);
  }
  butEnv_addFile(env, BUT_READFILE, net->fd, net, newConn);
  net->valid = TRUE;
  return(net);
}


ButRnet  *butRnet_create(ButEnv *env, const char *address, int port,
			 void *packet, ButOut ocallback(ButRnet *net),
			 ButOut callback(ButRnet *net, void *cmd, int cmdLen),
			 ButOut ccallback(ButRnet *net, const char *reason))  {
  ButRnet  *net;
  struct sockaddr_in  sa;
  struct hostent  *hp;

  net = newRnet(env);
  net->packet = packet;
  net->state = butRnetState_closed;
  net->error = net->lookupError = net->butNetError = FALSE;
  net->ocallback = ocallback;
  net->callback = callback;
  net->ccallback = ccallback;
  if ((net->fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)  {
    net->error = TRUE;
    net->errNum = errno;
    return(net);
  }
  sa.sin_family = AF_INET;
  if ((hp = gethostbyname(address)) == NULL)  {
    net->lookupError = TRUE;
#if  HAVE_H_ERRNO
    net->errNum = h_errno;
#else
    net->errNum = HOST_NOT_FOUND;
#endif
    return(net);
  }
  memcpy(&sa.sin_addr, hp->h_addr, hp->h_length);
  sa.sin_port = htons(port);
  if (connect(net->fd, (struct sockaddr *)&sa, sizeof(sa)) < 0)  {
    net->error = TRUE;
    net->errNum = errno;
    return(net);
  }
  net->state = butRnetState_pwWait;
  fcntl(net->fd, F_SETFL, O_NONBLOCK);
  putProtoWho(net);
  butEnv_addFile(net->env, BUT_READFILE, net->fd, net, getNetData);
  return(net);
}


void  butRnet_accept(ButRnet *net)  {
  assert(net->state == butRnetState_accWait);
  net->state = butRnetState_open;
  net->lType = net->env->curlast;
  net->env->curlast = butCur_bogus;  /* Force the cursor type to be sent. */
  putProtoWho(net);
}


void  butLnet_destroy(ButLnet *net)  {
  assert(MAGIC(net));
  if (net->valid)  {
    close(net->fd);
    butEnv_rmFile(net->env, BUT_READFILE, net->fd);
  }
  MAGIC_UNSET(net);
  wms_free(net);
}


void  butRnet_destroy(ButRnet *net, const char *reason)  {
  ButNetMsg  msgOut;

  assert(MAGIC(net));
  if (net->state < butRnetState_closing)  {
    msgOut.type = int_stdInt32(BUTNET_CLOSE);
    msgOut.length = int_stdInt32(2*4+strlen(reason)+1);
    strcpy(msgOut.perType.close, reason);
    butRnet_write(net, &msgOut);
    butRnet_close(net, NULL, FALSE);
  }
}


static ButOut  newConn(void *packet, int fd)  {
  ButLnet  *lnet = packet;
  ButRnet  *rnet;
  struct sockaddr_in  addr;
  int  addrlen = sizeof(addr);

  assert(MAGIC(lnet));
  rnet = newRnet(lnet->env);
  rnet->fd = accept(lnet->fd, (struct sockaddr *)&addr, &addrlen);
  if (rnet->fd < 0)  {
    rnet->env->partners[rnet->partner] = NULL;
    MAGIC_UNSET(rnet);
    wms_free(rnet);
    return(0);
  }
  rnet->packet = lnet->packet;
  rnet->ocallback = lnet->ocallback;
  rnet->callback = lnet->callback;
  rnet->ccallback = lnet->ccallback;
  fcntl(rnet->fd, F_SETFL, O_NONBLOCK);
  rnet->state = butRnetState_pwAccWait;
  butEnv_addFile(rnet->env, BUT_READFILE, rnet->fd, rnet, getNetData);
  return(0);
}


static ButRnet  *newRnet(ButEnv *env)  {
  ButRnet  *net = wms_malloc(sizeof(ButRnet));
  int  newNumPartners, i, partner;
  ButRnet  **newPartners;

  MAGIC_SET(net);
  for (partner = 0;  partner < env->numPartners;  ++partner)  {
    if (!env->partners[partner])
      break;
  }
  if (partner == env->numPartners)  {
    newNumPartners = (env->numPartners + 1) * 2;
    newPartners = wms_malloc(newNumPartners * sizeof(ButRnet *));
    for (i = 0;  i < env->numPartners;  ++i)  {
      newPartners[i] = env->partners[i];
    }
    for (;  i < newNumPartners;  ++i)
      newPartners[i] = NULL;
    if (env->partners)
      wms_free(env->partners);
    env->numPartners = newNumPartners;
    env->partners = newPartners;
  }
  env->partners[partner] = net;
  net->partner = partner;
  net->env = env;
  net->error = FALSE;
  net->wBufSize = net->wBufStart = net->wBufLen = 0;
  net->wBuffer = NULL;
  net->rBufAmt = 0;
  net->rBufLen = BUTNET_MAXCMD * 2 + 2;
  net->fd = -1;
  net->ackNeeded = net->mouseMove = FALSE;
  net->who = "Unknown";
  net->protocol = "Unknown";
  if ((env->last_mwin) && (env->last_mx >= 0))  {
    assert(MAGIC(env->last_mwin));
    net->lWinId = env->last_mwin->id;
    net->lx = env->last_mx;
    net->ly = env->last_my;
    net->lw = env->last_mwin->w;
    net->lh = env->last_mwin->h;
    net->lType = env->curnum;
  } else
    net->lWinId = -2;
  net->lPress = net->lTwitch = -1;
  butRcur_create(&net->rc, net->env);
  return(net);
}


/* Returns TRUE if success. */
static bool  butRnet_write(ButRnet *net, ButNetMsg *msg)  {
  int  len = stdInt32_int(msg->length) + 8;
  int  amtWritten;
  char  *remainder = (char *)msg;

  assert(MAGIC(net));
  if (net->wBufLen == 0)  {
    /* Nothing queued up.  Try to write directly! */
    amtWritten = write(net->fd, msg, len);
    if (amtWritten == len)
      return(TRUE);
    if (amtWritten < 0)  {
      butRnet_close(net, strerror(errno), TRUE);
      return(FALSE);
    }
    remainder += amtWritten;
    len -= amtWritten;
  }
  /* Not all got written.  Put some into the wBuffer. */
  if (len + net->wBufStart + net->wBufLen > net->wBufSize)  {
    char  *newBuf;

    newBuf = wms_malloc(net->wBufSize = len + net->wBufStart + net->wBufLen);
    if (net->wBufLen)
      memcpy(newBuf, net->wBuffer+net->wBufStart, net->wBufLen);
    net->wBufStart = 0;
    if (net->wBuffer)
      wms_free(net->wBuffer);
    net->wBuffer = newBuf;
  }
  memcpy(remainder, net->wBuffer + net->wBufStart + net->wBufLen, len);
  net->wBufLen += len;
  butEnv_addFile(net->env, BUT_WRITEFILE, net->fd, net, finishWrite);
  return(TRUE);
}
      

static ButOut  finishWrite(void *packet, int fd)  {
  ButRnet  *net = packet;
  int  amtWritten;

  assert(MAGIC(net));
  amtWritten = write(net->fd, net->wBuffer + net->wBufStart, net->wBufLen);
  if (amtWritten == net->wBufLen)  {
    butEnv_rmFile(net->env, BUT_WRITEFILE, net->fd);
    net->wBufLen = 0;
    net->wBufStart = 0;
    if (net->state == butRnetState_closing)  {
      close(net->fd);
      net->fd = -1;
      MAGIC_UNSET(net);
      wms_free(net);
    }
  } else if (amtWritten < 0)  {
    butRnet_close(net, strerror(errno), TRUE);
    return(0);
  } else  {
    net->wBufStart += amtWritten;
    net->wBufLen -= amtWritten;
  }    
  return(0);
}


static ButOut  getNetData(void *packet, int fd)  {
  ButRnet  *net = packet;
  int  readLen, i, msgType;
  ButOut  res = 0;
  char  *temp;

  assert(MAGIC(net));
  if (net->rBufAmt < 4)  {
    /* Waiting for the length count. */
    readLen = read(fd, (char *)&net->rBuffer + net->rBufAmt, 4 - net->rBufAmt);
    if (readLen <= 0)  {
      if (readLen == 0)
	butRnet_close(net, "Connection closed", TRUE);
      else if ((readLen < 0) && (errno != EAGAIN))
	butRnet_close(net, strerror(errno), TRUE);
      return(res);
    }
    if ((net->rBufAmt += readLen) < 4)
      return(0);
    net->rBufMsgLen = stdInt32_int(net->rBuffer.length);
    if (net->rBufMsgLen > BUTNET_MAXCMD)  {
      fprintf(stderr, "Bogus command\n");
      for (;;);
    }
    net->rBufMsgLen += 8;
    return(0);
  }
  if ((net->rBufMsgLen > BUTNET_MAXCMD+8) ||
      (net->rBufMsgLen < 8))  {
    butRnet_close(net, "Invalid data on connection", TRUE);
    return(res);
  }
  readLen = read(fd, (char *)&net->rBuffer + net->rBufAmt,
		 net->rBufMsgLen - net->rBufAmt);
  if (readLen <= 0)  {
    if (readLen == 0)
      butRnet_close(net, "Connection closed", TRUE);
    else if ((readLen < 0) && (errno != EAGAIN))
      butRnet_close(net, strerror(errno), TRUE);
    return(res);
  }
  net->rBufAmt += readLen;
  if (net->rBufAmt == net->rBufMsgLen)  {
    msgType = stdInt32_int(net->rBuffer.type);
    net->rBufAmt = 0;
    switch(msgType)  {
    case BUTNET_PROTOWHO:
      if (net->state == butRnetState_pwWait)  {
	net->state = butRnetState_open;
	/* Force the cursor type to be sent. */
	net->lType = net->env->curlast;
	net->env->curlast = butCur_bogus;
      } else if (net->state == butRnetState_pwAccWait)  {
	net->state = butRnetState_accWait;
      } else  {
	butRnet_close(net, "Invalid data on connection", TRUE);
	break;
      }
      if (net->rBufMsgLen != sizeof(net->rBuffer.perType.protoWho) + 8)  {
	butRnet_close(net, "Invalid data on connection", TRUE);
	break;
      }
      net->rmtPartner = stdInt32_int(net->rBuffer.perType.protoWho.context);
      temp = wms_malloc(i = (strlen(net->rBuffer.perType.protoWho.proto) + 1));
      strcpy(temp, net->rBuffer.perType.protoWho.proto);
      net->protocol = temp;
      temp = wms_malloc(strlen(net->rBuffer.perType.protoWho.who) + 1);
      strcpy(temp, net->rBuffer.perType.protoWho.who);
      net->who = temp;
      return(net->ocallback(net));
      break;
    case BUTNET_CLOSE:
      butRnet_close(net, net->rBuffer.perType.close, FALSE);
      break;
    case BUTNET_USERDATA:
      if (net->callback)
	return(net->callback(net, net->rBuffer.perType.userData,
			     stdInt32_int(net->rBuffer.length)));
      break;
    case BUTNET_MOUSE:
      if (net->rBufMsgLen != sizeof(net->rBuffer.perType.mouse) + 8)  {
	butRnet_close(net, "Invalid data on connection", TRUE);
	break;
      }
      butRcur_move(&net->rc, stdInt32_int(net->rBuffer.perType.mouse.win),
		   stdInt32_int(net->rBuffer.perType.mouse.x),
		   stdInt32_int(net->rBuffer.perType.mouse.y),
		   stdInt32_int(net->rBuffer.perType.mouse.w),
		   stdInt32_int(net->rBuffer.perType.mouse.h),
		   stdInt32_int(net->rBuffer.perType.mouse.type));
      net->rBuffer.length = int_stdInt32(0);
      net->rBuffer.type = int_stdInt32(BUTNET_MOUSEACK);
      butRnet_write(net, &net->rBuffer);
      break;
    case BUTNET_MOUSEACK:
      if (net->rBufMsgLen != 8)  {
	butRnet_close(net, "Invalid data on connection", TRUE);
	break;
      }
      net->ackNeeded = FALSE;
      if (net->mouseMove)  {
	net->mouseMove = FALSE;
	butRnet_sendMMove(net, -1, -1,-1, -1,-1, -1);
      }
      break;
    case BUTNET_NEWFLAGS:
      if (net->rBufMsgLen != sizeof(net->rBuffer.perType.newFlags) + 8)  {
	butRnet_close(net, "Invalid data on connection", TRUE);
	break;
      }
      newFlags(net, stdInt32_int(net->rBuffer.perType.newFlags.butId),
	       stdInt32_int(net->rBuffer.perType.newFlags.newFlags));
      break;
    case BUTNET_BUTSPEC:
      res |= butSpec(net->env,
		     stdInt32_int(net->rBuffer.perType.butSpec.butId),
		     net->rBuffer.perType.butSpec.butData,
		     stdInt32_int(net->rBuffer.length) - sizeof(StdInt32));
      break;
    default:
      butRnet_close(net, "Invalid data on connection", TRUE);
      break;
    }
  }
  return(res);
}


static void  newFlags(ButRnet *net, int id, uint newFlags)  {
  ButEnv  *env = net->env;
  But  *but;

  if (id < env->maxButIds)  {
    but = env->id2But[id];
    if (but)  {
      if (newFlags & BUT_PRESSED)
	net->lPress = id;
      else if (net->lPress == id)
	net->lPress = -1;
      if (newFlags & BUT_TWITCHED)
	net->lTwitch = id;
      else if (net->lTwitch == id)
	net->lTwitch = -1;
      but_newFlags(but, (newFlags & (BUT_NETMASK<<BUT_NETSHIFT)) |
		   (but->flags & ~(BUT_NETMASK<<BUT_NETSHIFT)));
    }
  }
}


static void  putProtoWho(ButRnet *net)  {
#if  HAVE_SYS_UTSNAME_H
  struct utsname  unameOut;
#endif
  struct passwd  *pw;
  char  *loginName;
  ButEnv  *env = net->env;
  ButNetMsg  protoOut;

  assert(MAGIC(net));
  protoOut.length = int_stdInt32(sizeof(protoOut.perType.protoWho));
  protoOut.type = int_stdInt32(BUTNET_PROTOWHO);
#if  HAVE_SYS_UTSNAME_H
  uname(&unameOut);
#endif
  if ((loginName = getlogin()) == NULL)  {
    pw = getpwuid(getuid());
    loginName = pw->pw_name;
  }
  strcpy(protoOut.perType.protoWho.proto, env->protocol);
  protoOut.perType.protoWho.proto[sizeof(protoOut.perType.protoWho.proto) -
				  1] = '\0';
  sprintf(protoOut.perType.protoWho.who, "%s@%s",
	  loginName,
#if  HAVE_SYS_UTSNAME_H
	  unameOut.nodename
#else
	  "unknown"
#endif
	  );
  butRnet_write(net, &protoOut);
}


static void  butRnet_close(ButRnet *net, const char *reason, bool force)  {
  ButEnv  *env = net->env;

  assert(MAGIC(net));
  if (net->lPress != -1)  {
    if (env->id2But[net->lPress])  {
      but_newFlags(env->id2But[net->lPress], env->id2But[net->lPress]->flags &
		   ~BUT_NETPRESS);
    }
  }
  if (net->lTwitch != -1)  {
    if (env->id2But[net->lTwitch])  {
      but_newFlags(env->id2But[net->lTwitch],
		   env->id2But[net->lTwitch]->flags & ~BUT_NETTWITCH);
    }
  }
  if (net->state < butRnetState_closing)  {
    if (reason && net->ccallback)
      net->ccallback(net, reason);
    net->state = butRnetState_closing;
    butEnv_rmFile(net->env, BUT_READFILE, net->fd);
    if (force || (net->wBufLen == 0))  {
      close(net->fd);
      net->fd = -1;
      if (net->wBufLen != 0)
	butEnv_rmFile(net->env, BUT_WRITEFILE, net->fd);
    }
  } 
  net->env->partners[net->partner] = NULL;
  MAGIC_UNSET(net);
  wms_free(net);
}


void  butRnet_send(ButRnet *net, void *buffer, int len)  {
  ButNetMsg  msg;

  assert(len <= BUTNET_MAXCMD);
  msg.length = int_stdInt32(len);
  msg.type = int_stdInt32(BUTNET_USERDATA);
  memcpy(msg.perType.userData, buffer, len);
  butRnet_write(net, &msg);
}


void  butRnet_mMove(ButEnv *env, int winId, int lx,int ly, int lw,int lh,
		    ButCur curnum)  {
  int  i;

  for (i = 0;  i < env->numPartners;  ++i)  {
    if (env->partners[i])  {
      if (butRnet_valid(env->partners[i]) &&
	  ((winId != -2) || (env->partners[i]->lWinId != -2)))
	butRnet_sendMMove(env->partners[i], winId, lx,ly, lw,lh, curnum);
    }
  }
}


static void  butRnet_sendMMove(ButRnet *net, int winId, int lx, int ly,
			       int lw, int lh, ButCur curnum)  {
  ButNetMsg  msg;

  if (winId == -1)
    winId = net->lWinId;
  if (lx == -1)
    lx = net->lx;
  if (ly == -1)
    ly = net->ly;
  if (lw == -1)
    lw = net->lw;
  if (lh == -1)
    lh = net->lh;
  if (curnum == -1)
    curnum = net->lType;
  net->lWinId = winId;
  net->lx = lx;
  net->ly = ly;
  net->lw = lw;
  net->lh = lh;
  net->lType = curnum;
  if (net->ackNeeded)
    net->mouseMove = TRUE;
  else  {
    msg.length = int_stdInt32(sizeof(msg.perType.mouse));
    msg.type = int_stdInt32(BUTNET_MOUSE);
    msg.perType.mouse.context = int_stdInt32(0);
    msg.perType.mouse.win = int_stdInt32(winId);
    msg.perType.mouse.x = int_stdInt32(lx);
    msg.perType.mouse.y = int_stdInt32(ly);
    msg.perType.mouse.w = int_stdInt32(lw);
    msg.perType.mouse.h = int_stdInt32(lh);
    msg.perType.mouse.type = int_stdInt32(curnum);
    assert((curnum == -1) || 
	   (curnum < butCur_bogus));
    butRnet_write(net, &msg);
  }
}


void  butRnet_newFlags(ButEnv *env, int butId, uint newFlags)  {
  int  i;
  ButNetMsg  msg;

  if ((butId != -2) && (env->numPartners > 0))  {
    assert(butId >= 0);
    assert(butId < env->maxButIds);
    newFlags = (newFlags & ~(BUT_NETTWITCH|BUT_NETPRESS|BUT_NETKEY)) |
      ((newFlags & BUT_NETMASK) << BUT_NETSHIFT);
    msg.length = int_stdInt32(sizeof(msg.perType.newFlags));
    msg.type = int_stdInt32(BUTNET_NEWFLAGS);
    msg.perType.newFlags.butId = int_stdInt32(butId);
    msg.perType.newFlags.newFlags = int_stdInt32(newFlags);
    for (i = 0;  i < env->numPartners;  ++i)  {
      if (env->partners[i])
	if (butRnet_valid(env->partners[i]))
	  butRnet_write(env->partners[i], &msg);
    }
  }
}


static ButOut  butSpec(ButEnv *env, int butId, void *buf, int bufLen)  {
  But  *but;

  but = env->id2But[butId];
  assert(but->action->netMessage);
  return(but->action->netMessage(but, buf, bufLen));
}


#else  /* !HAVE_SOCKETS */

ButLnet  *butLnet_create(ButEnv *env, int port, void *packet,
			 ButOut ocallback(ButRnet *net),
			 ButOut callback(ButRnet *net, void *cmd, int cmdLen),
			 ButOut ccallback(ButRnet *net, const char *reason))  {
  ButLnet  *net = wms_malloc(sizeof(ButLnet));

  net->error = TRUE;
  net->valid = FALSE;
  net->errNum = EPERM;
  return(net);
}


ButRnet  *butRnet_create(ButEnv *env, char *address, int port,
			 void *packet, ButOut ocallback(ButRnet *net),
			 ButOut callback(ButRnet *net, void *cmd, int cmdLen),
			 ButOut ccallback(ButRnet *net, const char *reason))  {
  ButRnet  *net = wms_malloc(sizeof(ButRnet));

  net->error = TRUE;
  net->errNum = EPERM;
  net->state = butRnetState_closed;
  return(net);
}


void  butLnet_destroy(ButLnet  *net)  { wms_free(net); }
void  butRnet_destroy(ButRnet *net, char *reason)  { wms_free(net); }
bool  butRnet_write(ButRnet *net, ButNetMsg *msg)  { return(FALSE); }
void  butRnet_send(ButRnet *net, void *buffer, int len)  {}
void  butRnet_mMove(ButEnv *env, int winId, int lx,int ly, int lw,int lh,
		    ButCur curnum)  {}
void  butRnet_newFlags(ButEnv *env, int butId, uint newFlags)  {}
void  butRnet_accept(ButRnet *net)  {}


#endif  /* HAVE_SOCKETS */


void  but_setId(But *but, int id)  {
  ButEnv  *env = but->win->env;
  int  i;

  assert(MAGIC(but));
  if (id >= env->maxButIds)  {
    But  **newId2But;
    int  newMaxButIds = (id + 1)*2;

    newId2But = wms_malloc(newMaxButIds * sizeof(But *));
    for (i = 0;  i < env->maxButIds;  ++i)
      newId2But[i] = env->id2But[i];
    for (;  i < newMaxButIds;  ++i)
      newId2But[i] = NULL;
    env->maxButIds = newMaxButIds;
    if (env->id2But)
      wms_free(env->id2But);
    env->id2But = newId2But;
  }
  if (but->id != -1)  {
    assert(but->id < env->maxButIds);
    env->id2But[id] = NULL;
  }
  env->id2But[id] = but;
  but->id = id;
}


void  butWin_setId(ButWin *win, int id)  {
  ButEnv  *env = win->env;
  int  i;

  assert(MAGIC(win));
  if (id >= env->maxWinIds)  {
    ButWin  **newId2Win;
    int  newMaxWinIds = (id + 1)*2;

    newId2Win = wms_malloc(newMaxWinIds * sizeof(ButWin *));
    for (i = 0;  i < env->maxWinIds;  ++i)
      newId2Win[i] = env->id2Win[i];
    for (;  i < newMaxWinIds;  ++i)
      newId2Win[i] = NULL;
    env->maxWinIds = newMaxWinIds;
    if (env->id2Win)
      wms_free(env->id2Win);
    env->id2Win = newId2Win;
  }
  if (win->id != -1)  {
    assert(win->id < env->maxWinIds);
    env->id2Win[id] = NULL;
  }
  env->id2Win[id] = win;
  win->id = id;
}


void  butRnet_butSpecSend(But *but, void *buffer, int len)  {
  ButEnv *env;
  ButNetMsg  msg;
  int  i;

  assert(len < BUTNET_MAXCMD - sizeof(StdInt32));
  if (but->id >= 0)  {
    env = but->win->env;
    if (env->numPartners > 0)  {
      msg.length = int_stdInt32(len + 4);
      msg.type = int_stdInt32(BUTNET_BUTSPEC);
      msg.perType.butSpec.butId = int_stdInt32(but->id);
      if (len > 0)
	memcpy(msg.perType.butSpec.butData, buffer, len);
      for (i = 0;  i < env->numPartners;  ++i)  {
	if (env->partners[i])  {
	  if (butRnet_valid(env->partners[i]))
	    butRnet_write(env->partners[i], &msg);
	}
      }
    }
  }
}


#endif  /* X11_DISP */
