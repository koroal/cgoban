/*
 * src/client/data.c, part of Complete Goban (game program)
 * Copyright (C) 1995-1997 William Shubert
 * See "configure.h.in" for more copyright information.
 */

#include <wms.h>
#include <wms/clp.h>
#include <wms/str.h>
#include "../cgoban.h"
#ifdef  _CLIENT_DATA_H_
#error   LEVELIZATION ERROR
#endif
#include "data.h"


/**********************************************************************
 * Forward Declarations
 **********************************************************************/

static void  cliData_destroy(CliData *cd);


/**********************************************************************
 * Functions
 **********************************************************************/
CliData  *cliData_create(Cgoban *cg, int serverNum,
			 const CliActions *actions, void *packet)  {
  CliData  *cd;
  const char  *serverId;

  assert(MAGIC(cg));
  cd = wms_malloc(sizeof(CliData));
  MAGIC_SET(cd);
  cd->state = cliData_setup;
  cd->cg = cg;
  cd->serverNum = serverNum;
  serverId = clp_getStrNum(cg->clp, "client.protocol", serverNum);
  if (serverId[0] == 'n')
    cd->server = cliServer_nngs;
  else  {
    assert(serverId[0] == 'i');
    cd->server = cliServer_igs;
  }
  cd->serverName = clp_getStrNum(cg->clp, "client.server", serverNum);
  str_init(&cd->userName);
  str_init(&cd->cmdBuild);
  cd->numKibitz = clp_lookup(cg->clp, "client.numberKibitz");
  cd->connValid = FALSE;
  cd->actions = actions;
  cd->packet = packet;
  cd->refCount = 1;
  return(cd);
}


void  cliData_closeConns(CliData *cd) {
  assert(MAGIC(cd));
  if (cd->connValid) {
    cliConn_deinit(&cd->conn);
    cd->connValid = FALSE;
  }
}


static void  cliData_destroy(CliData *cd)  {
  assert(MAGIC(cd));
  if (cd->actions)  {
    cd->actions->logout(cd->packet);
  }
  str_deinit(&cd->userName);
  str_deinit(&cd->cmdBuild);
  cliData_closeConns(cd);
  MAGIC_UNSET(cd);
  wms_free(cd);
}


void  cliData_decRef(CliData *cd) {
  if (--cd->refCount == 0)
    cliData_destroy(cd);
}
