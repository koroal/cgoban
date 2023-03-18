 /*
 * src/client/player.c, part of Complete Goan (player program)
 * Copyright (C) 1995-1996 William Shubert
 * See "configure.h.in" for more copyright information.
 */

#include <wms.h>
#include <but/but.h>
#include <but/list.h>
#include "../msg.h"
#include <but/plain.h>
#include <abut/swin.h>
#include <but/canvas.h>
#include <but/ctext.h>
#include <but/radio.h>
#ifdef  _CLIENT_PLAYER_H_
  Levelization Error.
#endif
#include "player.h"


/**********************************************************************
 * Forward Declarations
 **********************************************************************/
static void  expandPlayers(CliPlayerList *gl, int playerNum);
static ButOut  unmap(ButWin *win);
static ButOut  map(ButWin *win);
static ButOut  resize(ButWin *win);
static ButOut  destroy(ButWin *win);
static void  getListEl(CliPlayerList *gl, int gNum, Str *out);
static ButOut  sResize(ButWin *win);
static void  updatePlayer(CliPlayerList *pl, const char *playerInfo);
static ButOut  reload(But *but);
static int  cmpPlayers(const void *a, const void *b);
static int  getRankNum(const Str *rank);
static ButOut  newSort(But *but, int newSortType);
static ButOut  listPressed(But *but, int line);
static void  redrawLine(CliPlayerList *pl, int playerNum);
static ButOut  matchPressed(But *but);
static ButOut  observePressed(But *but);

/**********************************************************************
 * Functions
 **********************************************************************/
CliPlayerList  *cliPlayerList_init(CliPlayerList *gl, CliData *data)  {
  MAGIC_SET(gl);
  gl->data = data;
  gl->maxPlayers = 0;
  gl->players = NULL;
  gl->sort = (CliPlayerSort)clp_getInt(data->cg->clp,
				       "client.players.sort");
  gl->sortEnd = 0;
  gl->win = NULL;
  gl->elfBugReported = FALSE;
  gl->match = NULL;
  return(gl);
}


CliPlayerList  *cliPlayerList_deinit(CliPlayerList *gl)  {
  Cgoban  *cg;
  CliMatch  *m;

  assert(MAGIC(gl));
  cg = gl->data->cg;
  if (gl->players)
    wms_free(gl->players);
  if (gl->win)  {
    clp_setDouble(cg->clp, "client.players.h",
		  (double)butWin_h(gl->win) / (double)cg->fontH);
    clp_setInt(cg->clp, "client.players.x", butWin_x(gl->win));
    clp_setInt(cg->clp, "client.players.y", butWin_y(gl->win));
    butWin_setDestroy(gl->win, NULL);
    butWin_destroy(gl->win);
  }
  /*
   * Set all the match states to nil so they won't try to send declines
   *   out a no-longer-valid conn!
   */
  for (m = gl->match;  m;  m = m->next)
    m->state = cliMatch_nil;
  cliMatch_destroyChain(gl->match, NULL,NULL,NULL,NULL);
  MAGIC_UNSET(gl);
  return(gl);
}


void  cliPlayerList_openWin(CliPlayerList *gl)  {
  int  winW, winH;
  Cgoban  *cg;
  int  i;
  bool  err;
  Str  winName;

  assert(MAGIC(gl));
  if (gl->win)  {
    XRaiseWindow(butEnv_dpy(gl->data->cg->env), butWin_xwin(gl->win));
    return;
  }
  cliPlayerList_sort(gl);
  cg = gl->data->cg;
  winW = (butEnv_fontH(cg->env, 0) * 28) / 2 + 2 * cg->fontH +
    butEnv_stdBw(cg->env) * 8;
  winH = (clp_getDouble(cg->clp, "client.players.h") * cg->fontH + 0.5);
  str_init(&winName);
  str_print(&winName, "%s Player List", gl->data->serverName);
  gl->win = butWin_create(gl, cg->env, str_chars(&winName), winW, winH,
			  unmap, map, resize, destroy);
  i = clpEntry_iGetInt(clp_lookup(cg->clp, "client.players.x"), &err);
  if (!err)
    butWin_setX(gl->win, i);
  i = clpEntry_iGetInt(clp_lookup(cg->clp, "client.players.y"), &err);
  if (!err)
    butWin_setY(gl->win, i);
  butWin_setMaxH(gl->win, 0);
  butWin_setMinH(gl->win, cg->fontH * 5);
  butWin_activate(gl->win);
  str_deinit(&winName);
  gl->bg = butBoxFilled_create(gl->win, 0, BUT_DRAWABLE);
  butBoxFilled_setPixmaps(gl->bg, cg->bgLitPixmap, cg->bgShadPixmap,
			  cg->bgPixmap);
  gl->swin = abutSwin_create(gl, gl->win, 1, BUT_DRAWABLE|BUT_PRESSABLE,
			     sResize);
  gl->sBg = butPlain_create(gl->swin->win, 0, BUT_DRAWABLE, BUT_BG);
  gl->list = butList_create(listPressed, gl, gl->swin->win, 2,
			    BUT_DRAWABLE|BUT_PRESSABLE);
  gl->sortType = butRadio_create(newSort, gl, gl->win, 1,
				 BUT_DRAWABLE|BUT_PRESSABLE, gl->sort, 3);
  gl->nameSort = butText_create(gl->win, 2, BUT_DRAWABLE|BUT_PRESSTHRU,
				msg_name, butText_center);
  gl->rankSort = butText_create(gl->win, 2, BUT_DRAWABLE|BUT_PRESSTHRU,
				msg_braceRank, butText_center);
  gl->stateSort = butText_create(gl->win, 2, BUT_DRAWABLE|BUT_PRESSTHRU,
				 msg_state, butText_center);
  gl->titleBox = butBoxFilled_create(gl->win, 1, BUT_DRAWABLE);
  gl->title = butList_create(NULL, NULL, gl->win, 2, BUT_DRAWABLE);
  butList_setLen(gl->title, 1);
  butList_changeLine(gl->title, 0, msg_playerListDesc);
  gl->reload = butCt_create(reload, gl, gl->win, 1, BUT_DRAWABLE|BUT_PRESSABLE,
			    "*");
  for (i = 0;  i < gl->maxPlayers;  ++i)  {
    if (gl->players[i].state != cliPlayer_noPlayer)  {
      redrawLine(gl, i);
    }
  }
  butCan_resizeWin(gl->swin->win, 0, butList_len(gl->list) *
		   butEnv_fontH(gl->data->cg->env, 0) +
		   butEnv_stdBw(gl->data->cg->env) * 2, TRUE);
}


static void  expandPlayers(CliPlayerList *gl, int playerNum)  {
  int  newMaxPlayers;
  CliPlayer  *newPlayers;
  int  i;

  if (playerNum >= gl->maxPlayers)  {
    newMaxPlayers = (playerNum + 1) * 2;
    newPlayers = wms_malloc(newMaxPlayers * sizeof(CliPlayer));
    for (i = 0;  i < gl->maxPlayers;  ++i)
      newPlayers[i] = gl->players[i];
    for (;  i < newMaxPlayers;  ++i)  {
      newPlayers[i].state = cliPlayer_noPlayer;
      newPlayers[i].sort = gl->sort;
      str_init(&newPlayers[i].name);
      str_init(&newPlayers[i].rank);
      str_init(&newPlayers[i].idleTime);
      newPlayers[i].but = NULL;
      newPlayers[i].gameIn = -1;
    }
    if (gl->players)
      wms_free(gl->players);
    gl->maxPlayers = newMaxPlayers;
    gl->players = newPlayers;
  }
}


static ButOut  unmap(ButWin *win)  {
  return(0);
}


static ButOut  map(ButWin *win)  {
  return(0);
}


static ButOut  resize(ButWin *win)  {
  int  w, h;
  CliPlayerList  *gl = butWin_packet(win);
  int  bw = butEnv_stdBw(butWin_env(win));
  int  tabs[3];
  static const ButTextAlign  tabAligns[3] =
    {butText_center, butText_right, butText_right};
  int  fontH;
  int  physFontH = butEnv_fontH(butWin_env(win), 0);
  int  slideW = (physFontH * 3)/2;
  int  centerAdd;

  assert(MAGIC(gl));
  fontH = gl->data->cg->fontH;
  w = butWin_w(win);
  h = butWin_h(win);
  but_resize(gl->bg, 0, 0, w, h);
  but_resize(gl->sortType, bw*2 + slideW, bw*2,
	     physFontH * 9, fontH * 2);
  but_resize(gl->nameSort, bw*2 + slideW, bw*2,
	     (physFontH * 18 + 2) / 6, fontH * 2);
  but_resize(gl->rankSort, bw*2 + slideW + (physFontH * 18 + 2) / 6, bw*2,
	     (physFontH * 18) / 2 - 2*(physFontH * 18 + 2) / 6, fontH * 2);
  but_resize(gl->stateSort, bw*2 + slideW + (physFontH * 18) / 2 -
	     (physFontH * 18 + 2) / 6, bw*2,
	     (physFontH * 18 + 2) / 6, fontH * 2);
  but_resize(gl->titleBox, bw*2 + slideW + physFontH * 9, bw*2,
	     w - (bw*4 + slideW + physFontH * 9), fontH * 2);
  but_resize(gl->reload, bw*2, bw*2, slideW, fontH * 2);
  but_resize(gl->title, bw * 4 + slideW + (physFontH * 18) / 2,
	     bw * 2 + (fontH * 2 - physFontH) / 2,
	     w - (bw*2 + slideW + (physFontH * 18) / 2), physFontH);
  abutSwin_resize(gl->swin, bw*2, bw*2 + fontH*2, w - bw*4, h - bw*4 - fontH*2,
		  slideW, physFontH);
  butCan_resizeWin(gl->swin->win, 0, butList_len(gl->list) *
		   physFontH + bw * 2, TRUE);
  centerAdd = (physFontH * 3 + 1) / 4;
  tabs[0] = physFontH * 8;
  tabs[1] = (physFontH * 22) / 2;
  tabs[2] = (physFontH * 27) / 2;
  butList_setTabs(gl->list, tabs, tabAligns, 3);
  tabs[1] -= (physFontH * 18) / 2;
  tabs[2] -= (physFontH * 18) / 2;
  butList_setTabs(gl->title, tabs + 1, tabAligns + 1, 2);
  return(0);
}


static ButOut  sResize(ButWin *win)  {
  int  w, h;
  AbutSwin  *swin = butWin_packet(win);
  CliPlayerList  *gl;
  int  bw = butEnv_stdBw(butWin_env(win));

  assert(MAGIC(swin));
  gl = swin->packet;
  assert(MAGIC(gl));
  w = butWin_w(win);
  h = butWin_h(win);
  but_resize(gl->sBg, 0, 0, w, h);
  butList_resize(gl->list, bw, bw, w - bw*2);
  return(0);
}


static ButOut  destroy(ButWin *win)  {
  CliPlayerList  *gl = butWin_packet(win);
  Cgoban  *cg = gl->data->cg;
  int  i;

  assert(MAGIC(gl));
  clp_setDouble(cg->clp, "client.players.h",
		(double)butWin_h(win) / (double)cg->fontH);
  clp_setInt(cg->clp, "client.players.x", butWin_x(win));
  clp_setInt(cg->clp, "client.players.y", butWin_y(win));
  gl->win = NULL;
  for (i = 0;  i < gl->maxPlayers;  ++i)  {
    gl->players[i].but = NULL;
  }
  return(0);
}


static void  getListEl(CliPlayerList *pl, int pNum, Str *out)  {
  static const char  desc[5] = {'?', 'X', 'O', '!', '-'};
  char  thisDesc, obPl, idleType;
  const char  *idle = "";
  int  gameNum;
  CliPlayer  *p = &pl->players[pNum];

  if (p->state == cliPlayer_noPlayer)
    str_copyCharsLen(out, "", 0);
  thisDesc = desc[p->state];
  if (p->gameIn >= 0)  {
    thisDesc = 'X';
    obPl = 'P';
    gameNum = p->gameIn;
  } else if (p->gameOb >= 0)  {
    obPl = 'O';
    gameNum = p->gameOb;
  } else  {
    obPl = ' ';
    gameNum = -1;
  }
  idleType = str_chars(&p->idleTime)[str_len(&p->idleTime) - 1];
  if ((idleType != 's') || (atoi(str_chars(&p->idleTime)) >= 30))
    idle = str_chars(&p->idleTime);
  if (gameNum >= 0)  {
    str_print(out, "%s [%s]\t%c\t%s\t%c %d",
	      str_chars(&p->name), str_chars(&p->rank), (int)thisDesc, idle,
	      (int)obPl, gameNum);
  } else  {
    str_print(out, "%s [%s]\t%c\t%s",
	      str_chars(&p->name), str_chars(&p->rank), (int)thisDesc, idle);
  }
}


void  cliPlayerList_whoOutput(CliPlayerList *pl, const char *buf)  {
  char  onePlayer[36];

  if (buf[1] != 'I')  {
    strncpy(onePlayer, buf, 35);
    onePlayer[35] = '\0';
    updatePlayer(pl, onePlayer);
    if (strlen(buf) > 50)  {
      strncpy(onePlayer, buf+37, 35);
      onePlayer[35] = '\0';
      updatePlayer(pl, onePlayer);
    }
  }
}


static void  updatePlayer(CliPlayerList *pl, const char *playerInfo)  {
  char  f1, f2, f3;
  char  ob[40], play[40], name[40], idle[40], rank[40];
  int  args, playerNum;
  CliPlayer  *p;

  args = sscanf(playerInfo, "%c%c%c %s %s %s %s %s",
		&f1, &f2, &f3, ob, play, name, idle, rank);
  if (args != 8)
    return;
  playerNum = cliPlayerList_lookupPlayer(pl, name);
  if (playerNum == -1)  {
    for (playerNum = pl->sortEnd;  playerNum < pl->maxPlayers;  ++playerNum)  {
      if (pl->players[playerNum].state == cliPlayer_noPlayer)
	break;
    }
    expandPlayers(pl, playerNum);
  }
  p = &pl->players[playerNum];
  switch(f3)  {
  case '!':
    p->state = cliPlayer_looking;
    break;
  case 'X':
    p->state = cliPlayer_closed;
    break;
  default:
    p->state = cliPlayer_open;
    break;
  }
  str_copyChars(&p->name, name);
  str_copyChars(&p->rank, rank);
  str_copyChars(&p->idleTime, idle);
  if (strcmp(ob, "--"))
    p->gameOb = atoi(ob);
  else
    p->gameOb = -1;
  if (strcmp(play, "--"))  {
    p->gameIn = atoi(play);
    p->gameOb = -1;
  } else
    p->gameIn = -1;
  redrawLine(pl, playerNum);
}


int  cliPlayerList_lookupPlayer(CliPlayerList *pl, const char *name)  {
  int  i;

  for (i = 0;  i < pl->maxPlayers;  ++i)  {
    if (pl->players[i].state != cliPlayer_noPlayer)  {
      if (!strcmp(str_chars(&pl->players[i].name), name))
	return(i);
    }
  }
  return(-1);
}


void  cliPlayerList_disconnected(CliPlayerList *pl, const char *buf)  {
  char  name[50];
  int  args, playerNum;

  args = sscanf(buf, "{%s ", name);
  assert(args == 1);
  playerNum = cliPlayerList_lookupPlayer(pl, name);
  if (playerNum >= 0)  {
    pl->players[playerNum].state = cliPlayer_noPlayer;
    while ((pl->sortEnd > 0) &&
	   (pl->players[pl->sortEnd - 1].state == cliPlayer_noPlayer))
      --pl->sortEnd;
    if (pl->win)  {
      butList_changeLine(pl->list, playerNum, "");
      if (pl->players[playerNum].but)  {
	but_destroy(pl->players[playerNum].but);
	pl->players[playerNum].but = NULL;
      }
    }
  }
}

    
void  cliPlayerList_connected(CliPlayerList *pl, const char *buf)  {
  char  name[50], rank[50];
  int  args, i, playerNum;
  CliPlayer  *p;

  args = sscanf(buf, "{%s [ %[^]]", name, rank);
  if (args != 2)  {
    if (!pl->elfBugReported)  {
      pl->elfBugReported = TRUE;
      cgoban_createMsgWindow(pl->data->cg, "Cgoban Error",
			     msg_gameBadElf);
    }
    return;
  }
  assert(args == 2);
  for (i = strlen(rank);  i && (rank[i - 1] == ' ');  --i);
  rank[i] = '\0';
  playerNum = cliPlayerList_lookupPlayer(pl, name);
  if (playerNum == -1)  {
    for (playerNum = pl->sortEnd;  playerNum < pl->maxPlayers;  ++playerNum)  {
      if (pl->players[playerNum].state == cliPlayer_noPlayer)
	break;
    }
    expandPlayers(pl, playerNum);
    p = &pl->players[playerNum];
    p->state = cliPlayer_unknown;
    str_copyChars(&p->name, name);
    str_copyChars(&p->rank, rank);
    str_copyChars(&p->idleTime, "");
    p->gameIn = -1;
    p->gameOb = -1;
    redrawLine(pl, playerNum);
  }
}


static ButOut  reload(But *but)  {
  CliPlayerList  *pl = but_packet(but);

  assert(MAGIC(pl));
  cliConn_send(&pl->data->conn, "who\n");
  return(0);
}


void  cliPlayerList_sort(CliPlayerList *pl)  {
  Str  listEl;
  int  i, physFontH, bw;
  ButEnv  *env;

  qsort(pl->players, pl->maxPlayers, sizeof(CliPlayer), cmpPlayers);
  for (pl->sortEnd = 0; 
       ((pl->sortEnd < pl->maxPlayers) &&
	(pl->players[pl->sortEnd].state != cliPlayer_noPlayer));
       ++pl->sortEnd);
  if (pl->win)  {
    str_init(&listEl);
    env = pl->data->cg->env;
    physFontH = butEnv_fontH(env, 0);
    bw = butEnv_stdBw(env);
    for (i = 0;  i < pl->maxPlayers;  ++i)  {
      if (pl->players[i].state == cliPlayer_noPlayer)
	break;
      getListEl(pl, i, &listEl);
      butList_changeLine(pl->list, i, str_chars(&listEl));
      if (pl->players[i].but != NULL) {
	but_resize(pl->players[i].but,
		   bw + physFontH * (pl->players[i].matchBut ? 7 : 11),
		   bw + physFontH * i,
		   physFontH * (pl->players[i].matchBut ? 2 : 3), physFontH);
      }
    }
    if (i == 0)
      i = 1;
    butList_setLen(pl->list, i);
    butCan_resizeWin(pl->swin->win, 0, i *
		     butEnv_fontH(pl->data->cg->env, 0) +
		     butEnv_stdBw(pl->data->cg->env) * 2, TRUE);
    str_deinit(&listEl);
  }
}


static int  cmpPlayers(const void *a, const void *b)  {
  const CliPlayer  *pa = a;
  const CliPlayer  *pb = b;
  int  rankA, rankB;
  CliPlayerState  stateA, stateB;
  
  if (pa->state == cliPlayer_noPlayer)
    return(1);
  else if (pb->state == cliPlayer_noPlayer)
    return(-1);
  if (pa->sort == cliPlayerSort_open)  {
    stateA = pa->state;
    if (pa->gameIn >= 0)
      stateA = cliPlayer_closed;
    stateB = pb->state;
    if (pb->gameIn >= 0)
      stateB = cliPlayer_closed;
    if (stateA != stateB)
      return(stateB - stateA);
  }
  if (pa->sort >= cliPlayerSort_rank)  {
    rankA = getRankNum(&pa->rank);
    rankB = getRankNum(&pb->rank);
    if (rankA != rankB)
      return(rankB - rankA);
  }
  return(strcasecmp(str_chars(&pa->name), str_chars(&pb->name)));
}


static int  getRankNum(const Str *rank)  {
  char  class;
  int  val;

  if (!strncmp(str_chars(rank), "NR", 2) ||
      !strncmp(str_chars(rank), "?" "?" "?", 3))  /* Avoid the trigraphs. */
    return(-100);
  class = str_chars(rank)[str_len(rank) - 1];
  if (class == '*')  {
    class = str_chars(rank)[str_len(rank) - 2];
  }
  val = atoi(str_chars(rank));
  if (class == 'k')
    return(-val);
  else if (class == 'd')
    return(val);
  else  {
    assert(class == 'p');
    return(val + 100);
  }
}


static ButOut  newSort(But *but, int newSortType)  {
  CliPlayerList *pl;
  int  i;

  pl = but_packet(but);
  assert(MAGIC(pl));
  clp_setInt(pl->data->cg->clp, "client.players.sort", newSortType);
  pl->sort = (CliPlayerSort)newSortType;
  for (i = 0;  i < pl->maxPlayers;  ++i)  {
    pl->players[i].sort = (CliPlayerSort)newSortType;
  }
  cliPlayerList_sort(pl);
  return(0);
}


static ButOut  listPressed(But *but, int line)  {
  CliPlayerList  *pl;

  pl = but_packet(but);
  assert(MAGIC(pl));
  assert((line >= 0) && (line < pl->maxPlayers));
  if (pl->players[line].state == cliPlayer_noPlayer)
    return(BUTOUT_ERR);
  str_print(&pl->data->cmdBuild, "stats %s\n",
	    str_chars(&pl->players[line].name));
  cliConn_send(&pl->data->conn, str_chars(&pl->data->cmdBuild));
  return(0);
}


void  cliPlayerList_playerInGame(CliPlayerList *pl,
				 const char *player, int gameNum)  {
  int  playerNum;

  playerNum = cliPlayerList_lookupPlayer(pl, player);
  if (playerNum >= 0)  {
    if (pl->players[playerNum].gameIn != gameNum)  {
      pl->players[playerNum].gameIn = gameNum;
      pl->players[playerNum].gameOb = -1;
      if (pl->win)  {
	redrawLine(pl, playerNum);
      }
    }
  }
}


void  cliPlayerList_setState(CliPlayerList *pl, int playerNum,
			     CliPlayerState state)  {
  assert(MAGIC(pl));
  assert((playerNum >= 0) && (playerNum < pl->maxPlayers));
  assert(pl->players[playerNum].state != cliPlayer_noPlayer);
  pl->players[playerNum].state = state;
  redrawLine(pl, playerNum);
}


static void  redrawLine(CliPlayerList *pl, int playerNum)  {
  CliPlayer  *p;
  ButEnv  *env;
  int  bw, lastPlayer, physFontH;
  char  state[2];

  assert(MAGIC(pl));
  if (pl->win)  {
    p = &pl->players[playerNum];
    getListEl(pl, playerNum, &pl->data->cmdBuild);
    if (p->state == cliPlayer_noPlayer)  {
      if (p->but)  {
	but_destroy(p->but);
	p->but = NULL;
      }
      if (playerNum == butList_len(pl->list) - 1)  {
	for (lastPlayer = playerNum - 1;
	     ((lastPlayer >= 0) &&
	      (pl->players[lastPlayer].state == cliPlayer_noPlayer));
	     --lastPlayer);
	butList_setLen(pl->list, lastPlayer + 1);
	butCan_resizeWin(pl->swin->win, 0, (lastPlayer + 1) *
			 butEnv_fontH(pl->data->cg->env, 0) +
			 butEnv_stdBw(pl->data->cg->env) * 2, TRUE);
      }
    } else  {
      if (playerNum >= butList_len(pl->list))  {
	butList_setLen(pl->list, playerNum + 1);
	butCan_resizeWin(pl->swin->win, 0, butList_len(pl->list) *
			 butEnv_fontH(pl->data->cg->env, 0) +
			 butEnv_stdBw(pl->data->cg->env) * 2, TRUE);
      }
      if (p->but) {
	if (!((p->matchBut && (p->gameIn == -1) &&
	       (p->state != cliPlayer_closed)) ||
	      (!p->matchBut && (p->gameIn != -1)))) {
	  but_destroy(p->but);
	  p->but = NULL;
	}
      }
      if (p->but == NULL)  {
	if (p->gameIn != -1) {
	  char label[10];
	  sprintf(label, "P%d", p->gameIn);
	  p->but = butCt_create(observePressed, pl, pl->swin->win, 3,
				BUT_DRAWABLE|BUT_PRESSABLE, label);
	  p->matchBut = FALSE;
	} else if (p->state != cliPlayer_closed)  {
	  state[0] = "?XO! "[p->state];
	  state[1] = '\0';
	  p->but = butCt_create(matchPressed, pl, pl->swin->win, 3,
				BUT_DRAWABLE|BUT_PRESSABLE, state);
	  p->matchBut = TRUE;
	}
	if (p->but != NULL) {
	  env = pl->data->cg->env;
	  physFontH = butEnv_fontH(env, 0);
	  bw = butEnv_stdBw(env);
	  but_resize(p->but, bw + physFontH * (p->matchBut ? 7 : 11),
		     bw + physFontH * playerNum,
		     physFontH * (p->matchBut ? 2 : 3), physFontH);
	}
      } else  {
	if (p->matchBut) {
	  state[0] = "?XO! "[p->state];
	  state[1] = '\0';
	  butCt_setText(p->but, state);
	}
      }
    }
    butList_changeLine(pl->list, playerNum,
		       str_chars(&pl->data->cmdBuild));
  }
}


static ButOut  matchPressed(But *but)  {
  CliPlayerList  *pl = but_packet(but);
  int  playerNum, oppRank, myRank;
  CliPlayer  *p, *me;
  const char  *rankStr;
  
  assert(MAGIC(pl));
  /*
   * If there were lots of buttons, hashing the but *'s or something would
   *   be a good idea.  But for now, a linear search is probably OK.
   */
  for (playerNum = 0;  pl->players[playerNum].but != but;  ++playerNum)  {
    assert(playerNum < pl->maxPlayers);
  }
  p = &pl->players[playerNum];
  for (playerNum = 0;
       (pl->players[playerNum].state == cliPlayer_noPlayer) ||
       strcmp(str_chars(&pl->players[playerNum].name),
	      str_chars(&pl->data->userName));
       ++playerNum)  {
    if (playerNum + 1 >= pl->maxPlayers)  {
      /* We didn't find ourselves in the list.  Ouch. */
      cgoban_createMsgWindow(pl->data->cg, "Cgoban Error", msg_youDontExist);
      return(BUTOUT_ERR);
    }
  }
  me = &pl->players[playerNum];
  if (isdigit(str_chars(&p->rank)[0]) && isdigit(str_chars(&me->rank)[0]))  {
    oppRank = atoi(str_chars(&p->rank));
    rankStr = str_chars(&p->rank);
    if ((rankStr[str_len(&p->rank) - 1] == 'k') ||
	(rankStr[str_len(&p->rank) - 2] == 'k'))
      oppRank = -oppRank;
    myRank = atoi(str_chars(&me->rank));
    rankStr = str_chars(&me->rank);
    if ((rankStr[str_len(&me->rank) - 1] == 'k') ||
	(rankStr[str_len(&me->rank) - 2] == 'k'))
      myRank = -myRank;
  } else
    oppRank = myRank = 0;
  cliMatch_create(pl->data, str_chars(&p->name), &pl->match, myRank - oppRank);
  return(0);
}


static ButOut observePressed(But *but) {
  CliPlayerList *pl = but_packet(but);
  CliPlayer *p;
  int playerNum;

  assert(MAGIC(pl));
  for (playerNum = 0; pl->players[playerNum].but != but; ++playerNum) {
    assert(playerNum < pl->maxPlayers);
  }
  p = &pl->players[playerNum];
  if (p->gameIn != -1) {
    pl->data->observeGame(pl->data->obPacket, p->gameIn);
  }
  return(0);
}

extern const char  *cliPlayerList_getRank(CliPlayerList *pl,
					  const char *name)  {
  int  i;

  for (i = 0;  i < pl->maxPlayers;  ++i)  {
    if (pl->players[i].state != cliPlayer_noPlayer)  {
      if (!strcmp(name, str_chars(&pl->players[i].name)))
	return(str_chars(&pl->players[i].rank));
    }
  }
  return(NULL);
}
