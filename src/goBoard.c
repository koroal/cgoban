/*
 * src/goboard.c, part of Complete Goban (game program)
 * Copyright (C) 1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 */

#include <wms.h>
#include <wms/rnd.h>
#include "goBoard.h"
#include "goHash.h"


#define  DEBUG_SHOWLINKS  0

static bool  goBoard_isBorder(GoBoard *board, int pos);
static void  addEmpty(GoBoard *board, int pos);
#if  DEBUG_SHOWLINKS
static void  showLinks(GoBoard *board);
#endif


GoBoard  *goBoard_create(int size)  {
  GoBoard  *board;
  int  i;
  Rnd  *r;

  r = rnd_create(50);
  board = wms_malloc(sizeof(GoBoard));
  MAGIC_SET(board);
  board->area = ((size + 2) * (size + 1) + 1);
  board->dirs[0] = size + 1;
  board->dirs[1] = -(size + 1);
  board->dirs[2] = 1;
  board->dirs[3] = -1;
  board->pieces = wms_malloc(board->area * sizeof(GoBoardPiece));
  for (i = 0;  i < board->area;  ++i)  {
    if (goBoard_isBorder(board, i))  {
      board->pieces[i].type = goStone_edge;
      board->pieces[i].head = NULL;
      board->pieces[i].hashStone[goStone_black] = goHash_zero;
      board->pieces[i].hashStone[goStone_white] = goHash_zero;
      board->pieces[i].hashKo[goStone_black] = goHash_zero;
      board->pieces[i].hashKo[goStone_white] = goHash_zero;
    } else  {
      board->pieces[i].type = goStone_empty;
      /*
       * We must set empties to NULL head or else they may trip up the
       *   liberty counting code (it sounds unlikely, but believe me!
       *   I've seen it happen!)
       */
      board->pieces[i].head = NULL;
      board->pieces[i].hashStone[goStone_black] = goHash_init(r);
      board->pieces[i].hashStone[goStone_white] = goHash_init(r);
      board->pieces[i].hashKo[goStone_black] = goHash_init(r);
      board->pieces[i].hashKo[goStone_white] = goHash_init(r);
    }      
  }
  board->caps[0] = 0;
  board->caps[1] = 0;
  board->hashVal = goHash_zero;
  board->koLoc = 0;
  rnd_destroy(r);
  return(board);
}


static bool  goBoard_isBorder(GoBoard *board, int pos)  {
  return((pos < board->dirs[0]) ||
	 (pos >= board->area - board->dirs[0]) ||
	 (pos % board->dirs[0] == 0));
}


void  goBoard_destroy(GoBoard *board)  {
  assert(MAGIC(board));
  wms_free(board->pieces);
  MAGIC_UNSET(board);
  wms_free(board);
}


void  goBoard_copy(GoBoard *src, GoBoard *dest)  {
  int  i;
  GoBoardPiece  *sp, *dp;

  assert(MAGIC(src));
  assert(MAGIC(dest));
  assert(src->area == dest->area);
  for (i = 0, sp = src->pieces, dp = dest->pieces;
       i < src->area;
       ++i, ++sp, ++dp)  {
    dp->type = sp->type;
    dp->libs = sp->libs;
    dp->size = sp->size;
    if (sp->head)
      dp->head = dest->pieces + (sp->head - src->pieces);
    else
      dp->head = NULL;
    if (sp->tail)
      dp->tail = dest->pieces + (sp->tail - src->pieces);
    else
      dp->tail = NULL;
    if (sp->next)
      dp->next = dest->pieces + (sp->next - src->pieces);
    else
      dp->next = NULL;
  }
  dest->caps[0] = src->caps[0];
  dest->caps[1] = src->caps[1];
  dest->hashVal = src->hashVal;
  dest->koLoc = src->koLoc;
}


bool  goBoard_eq(GoBoard *b1, GoBoard *b2)  {
  int  i;

  assert(MAGIC(b1));
  assert(MAGIC(b2));
  if (b1->area != b2->area)
    return(FALSE);
  for (i = 0;  i < b1->area;  ++i)  {
    if (b1->pieces[i].type != b2->pieces[i].type)
      return(FALSE);
  }
  return(TRUE);
}


void  goBoard_print(GoBoard *b)  {
  goBoard_fprint(b, stdout);
}


void  goBoard_fprint(GoBoard *b, FILE *fnum)  {
  int  x, y;

  for (y = 0;  y < goBoard_size(b);  ++y)  {
    for (x = 0;  x < goBoard_size(b);  ++x)  {
      printf("%c ", goStone_char(b->pieces[goBoard_xy2Loc(b, x,y)].type));
    }
    printf("\n");
  }
}


/*
 * Returns the number of captures.
 */
int  goBoard_addStone(GoBoard *board, GoStone stone, int loc, int *suicides)  {
  GoStone  oppType;
  GoBoardPiece  *oppList[4], *new, *neighbor, *group, *groupNeighbor;
  GoBoardPiece  *oppOppList[4];
  GoBoardPiece  *oldHead;
  GoBoardPiece  *libs[GOBOARD_MAXSIZE*GOBOARD_MAXSIZE];
  int  i, j, k, oppListLen, oppOppListLen, numLibs, captures = 0;
  bool  unseen;

  assert(MAGIC(board));
  assert(loc >= 0);
  assert(loc < board->area);
  if (stone == goStone_empty)  {
    assert(suicides == NULL);
    addEmpty(board, loc);
    return(0);
  }
  assert((loc == 0) || (board->pieces[loc].type == goStone_empty));
  assert((stone == goStone_black) || (stone == goStone_white));
  oppType = goStone_opponent(stone);
  new = &board->pieces[loc];
  board->hashVal = goHash_xor(goHash_xor(board->hashVal, goHash_pass),
			      new->hashStone[stone]);
  if (board->koLoc)  {
    board->hashVal = goHash_xor(board->hashVal,
				board->pieces[board->koLoc].hashKo[oppType]);
    board->koLoc = 0;
  }
  if (loc == 0)
    return(0);

  /* Start you out as a simple group. */
  new->type = stone;
  new->libs = 0;
  new->size = 1;
  new->head = new;
  new->tail = new;
  new->next = NULL;

  for (i = 0, oppListLen = 0;  i < 4;  ++i)  {
    neighbor = new + board->dirs[i];

    if (neighbor->type == oppType)  {
      /*
       * An enemy group...remove a liberty.
       * If it now has 0 liberties, kill it!
       */
      unseen = TRUE;
      /*
       * First check; if you've already seen this group then you shouldn't
       *   remove another liberty, you've already done it.
       */
      neighbor = neighbor->head;
      assert(neighbor->libs > 0);
      for (j = 0;  j < oppListLen;  ++j)  {
	if (neighbor == oppList[j])  {
	  unseen = FALSE;
	  break;
	}
      }
      if (unseen)  {
	oppList[oppListLen] = neighbor;
	++oppListLen;
	if (--neighbor->libs == 0)  {
	  if (new == new->head)
	    ++new->libs;
	  captures += neighbor->size;
	  for (group = neighbor;  group;  group = group->next)  {
	    board->hashVal = goHash_xor(board->hashVal,
					group->hashStone[oppType]);
	    group->type = goStone_empty;
	    group->head = NULL;
	    oppOppListLen = 0;
	    for (j = 0;  j < 4;  ++j)  {
	      groupNeighbor = group + board->dirs[j];
	      if (groupNeighbor->type == stone)  {
		if ((groupNeighbor != new) ||
		    (groupNeighbor->head != groupNeighbor))
		  ++groupNeighbor->head->libs;
		groupNeighbor = groupNeighbor->head;
		for (k = 0;  k < oppOppListLen;  ++k)  {
		  if (oppOppList[k] == groupNeighbor)  {
		    --groupNeighbor->libs;
		    break;
		  }
		}
		oppOppList[oppOppListLen++] = groupNeighbor;
	      }
	    }
	  }
	}
      }
    } else if ((neighbor->type == stone) && (neighbor->head != new->head))  {
      neighbor = neighbor->head;
      assert(neighbor->libs > 0);
      oldHead = new->head;
      /*
       * Add the group with the new stone to the tail of the neighbor group.
       */
      neighbor->size += oldHead->size;
      numLibs = 0;
      /* You're touching a friendly stone.  Merge them into a single group. */
      for (group = oldHead;  group;  group = group->next)  {
	group->head = neighbor;
	for (j = 0;  j < 4;  ++j)  {
	  groupNeighbor = group + board->dirs[j];
	  if ((groupNeighbor->type == goStone_empty) &&
	      (groupNeighbor->head != neighbor))  {
	    groupNeighbor->head = neighbor;
	    libs[numLibs++] = groupNeighbor;
	  }
	}
      }
      for (group = neighbor;  group;  group = group->next)  {
	for (j = 0;  j < 4;  ++j)  {
	  groupNeighbor = group + board->dirs[j];
	  if ((groupNeighbor->type == goStone_empty) &&
	      (groupNeighbor->head != neighbor))  {
	    groupNeighbor->head = neighbor;
	    libs[numLibs++] = groupNeighbor;
	  }
	}
      }
      neighbor->tail->next = oldHead;
      neighbor->tail = oldHead->tail;
      assert(neighbor->libs - 1 <= numLibs);
      neighbor->libs = numLibs;
      for (j = 0;  j < numLibs;  ++j)  {
	libs[j]->head = NULL;
      }
      assert(neighbor != new);
      assert(new->head == neighbor);
    } else if ((neighbor->type == goStone_empty) && (new->head == new))
      ++new->libs;
  }
  if (new->head->libs == 0)  {
    if (suicides)
      *suicides = new->head->size;
    board->caps[oppType] += new->head->size;
    for (group = new->head;  group;  group = group->next)  {
      board->hashVal = goHash_xor(board->hashVal,
				  group->hashStone[stone]);
      group->type = goStone_empty;
      group->head = NULL;
      oppOppListLen = 0;
      for (j = 0;  j < 4;  ++j)  {
	groupNeighbor = group + board->dirs[j];
	if (groupNeighbor->type == oppType)  {
	  groupNeighbor = groupNeighbor->head;
	  ++groupNeighbor->libs;
	  for (k = 0;  k < oppOppListLen;  ++k)  {
	    if (oppOppList[k] == groupNeighbor)  {
	      --groupNeighbor->libs;
	      break;
	    }
	  }
	  oppOppList[oppOppListLen++] = groupNeighbor;
	}
      }
    }
  } else if (suicides)
    *suicides = 0;
  board->caps[stone] += captures;
  if ((captures == 1) && (new->head == new) && (new->libs == 1))  {
    board->koLoc = loc;
    board->hashVal = goHash_xor(board->hashVal, new->hashKo[stone]);
  }
#if  DEBUG_SHOWLINKS
  showLinks(board);
#endif
  return(captures);
}


static void  addEmpty(GoBoard *board, int loc)  {
  GoBoardPiece  *remove, *group, *neighbor, *oppList[4];
  GoStone  type;
  int  killedLocs[GOBOARD_MAXSIZE*GOBOARD_MAXSIZE];
  int  i, dir, numKilled, oppListLen;

  /*
   * My database really has no clean way of removing stones, so I take out
   *   the entire group that the empty stone is a part of, then add in
   *   all of the group (except for the stone that I wanted to remove in
   *   first place) one at a time.
   */
  remove = board->pieces + loc;
  type = remove->type;
  if (type == goStone_empty)
    return;
  assert((type == goStone_white) || (type == goStone_black));
  numKilled = 0;
  for (group = remove->head;  group;  group = group->next)  {
    if (group != remove)
      killedLocs[numKilled++] = group - board->pieces;
    board->hashVal = goHash_xor(board->hashVal, group->hashStone[type]);
    group->type = goStone_empty;
    group->head = NULL;
    oppListLen = 0;
    for (dir = 0;  dir < 4;  ++dir)  {
      neighbor = group + board->dirs[dir];
      if (neighbor->type == goStone_opponent(type))  {
	neighbor = neighbor->head;
	++neighbor->libs;
	for (i = 0;  i < oppListLen;  ++i)  {
	  if (oppList[i] == neighbor)  {
	    --neighbor->libs;
	    break;
	  }
	}
	oppList[oppListLen++] = neighbor;
      }
    }
  }
  /* Ok, they're all gone now.  So add them back!  :-) */
  for (i = 0;  i < numKilled;  ++i)  {
    goBoard_addStone(board, type, killedLocs[i], NULL);
  }
}


/*
 * This returns the hash WITHOUT the ko.
 */
GoHash  goBoard_quickHash(GoBoard *board, GoStone stone, int loc,
			  bool *suicide)  {
  GoHash  result;
  int  i, j, deadListLen = 0;
  bool  killSelf = TRUE, killNeighbor;
  GoBoardPiece  *new, *neighbor, *deadList[4];

  assert(MAGIC(board));
  assert(loc >= 0);
  assert(loc < board->area);
  assert((loc == 0) || (board->pieces[loc].type == goStone_empty));
  assert((stone == goStone_black) || (stone == goStone_white));
  result = goHash_xor(goBoard_hashNoKo(board, goStone_opponent(stone)),
		      goHash_xor(goHash_pass,
				 board->pieces[loc].hashStone[stone]));
  if (loc == 0)
    return(result);
  new = &board->pieces[loc];
  for (i = 0;  i < 4;  ++i)  {
    neighbor = new + board->dirs[i];
    if (neighbor->type == goStone_empty)
      killSelf = FALSE;
    else if (neighbor->type == stone)  {
      if (neighbor->head->libs > 1)
	killSelf = FALSE;
    } else if (neighbor->type == goStone_opponent(stone))  {
      neighbor = neighbor->head;
      if (neighbor->libs == 1)  {
	killSelf = FALSE;
	killNeighbor = TRUE;
	for (j = 0;  j < deadListLen;  ++j)  {
	  if (deadList[j] == neighbor)  {
	    killNeighbor = FALSE;
	    break;
	  }
	}
	if (killNeighbor)  {
	  deadList[deadListLen++] = neighbor;
	  do  {
	    result = goHash_xor(result,
				neighbor->hashStone[goStone_opponent(stone)]);
	    neighbor = neighbor->next;
	  } while (neighbor);
	}
      }
    }
  }
  if (suicide)
    *suicide = killSelf;
  if (killSelf)  {
    result = goHash_xor(result, new->hashStone[stone]);
    deadListLen = 0;
    for (i = 0;  i < 4;  ++i)  {
      neighbor = new + board->dirs[i];
      if (neighbor->type == stone)  {
	neighbor = neighbor->head;
	killNeighbor = TRUE;
	for (j = 0;  j < deadListLen;  ++j)  {
	  if (deadList[j] == neighbor)  {
	    killNeighbor = FALSE;
	    break;
	  }
	}
	if (killNeighbor)  {
	  deadList[deadListLen++] = neighbor;
	  do  {
	    result = goHash_xor(result, neighbor->hashStone[stone]);
	    neighbor = neighbor->next;
	  } while (neighbor);
	}
      }
    }
  }
  return(result);
}


void  goBoard_loc2Str(GoBoard *board, int loc, char *str)  {
  if (loc == 0)
    strcpy(str, "Pass");  /* BAD! */
  else  {
    str[0] = 'A' + (loc % board->dirs[0]) - 1;
    if (str[0] >= 'I')
      ++str[0];
    sprintf(str+1, "%d", goBoard_size(board) + 1 - (loc / board->dirs[0]));
  }
}

  
int  goBoard_str2Loc(GoBoard *board, const char *str)  {
  char  xChar;
  int  x, y, yLoc;
  int  args;

  args = sscanf(str, "%c%d", &xChar, &yLoc);
  assert(args == 2);
  if (xChar < 'I')
    x = xChar - 'A';
  else
    x = xChar - 'B';
  y = goBoard_size(board) - yLoc;
  if ((x < 0) || (y < 0) ||
      (x >= goBoard_size(board)) || (y >= goBoard_size(board)))  {
    return(-1);
  }
  return(goBoard_xy2Loc(board, x, y));
}

  
void  goBoard_rmGroup(GoBoard *board, int loc)  {
  GoBoardPiece  *oppList[4];
  GoBoardPiece  *dead, *neighbor;
  GoStone  s, o;
  int  i, j, oppListLen;

  assert(goStone_isStone(board->pieces[loc].type));
  dead = board->pieces[loc].head;
  s = dead->type;
  assert(goStone_isStone(s));
  o = goStone_opponent(s);
  board->caps[o] += dead->size;
  do  {
    board->hashVal = goHash_xor(board->hashVal,
				dead->hashStone[s]);
    dead->type = goStone_empty;
    dead->head = NULL;
    oppListLen = 0;
    for (i = 0;  i < 4;  ++i)  {
      neighbor = dead + board->dirs[i];
      if (neighbor->type == o)  {
	neighbor = neighbor->head;
	++neighbor->libs;
	for (j = 0;  j < oppListLen;  ++j)  {
	  if (oppList[j] == neighbor)  {
	    --neighbor->libs;
	    break;
	  }
	}
	oppList[oppListLen++] = neighbor;
      }
    }
    dead = dead->next;
  } while (dead);
}


const char  *goBoard_loc2Sgf(GoBoard *b, int l)  {
  static char  loc[5] = "xxxx";
  int  x, y;

  assert(MAGIC(b));
  assert((l > 0) && (l < b->area));
  x = goBoard_loc2X(b, l);
  y = goBoard_loc2Y(b, l);
  loc[0] = 'a' + (x / 26);
  loc[1] = 'a' + (x % 26);
  loc[2] = 'a' + (y / 26);
  loc[3] = 'a' + (y % 26);
  return(loc);
}


int  goBoard_sgf2Loc(GoBoard *b, const char *l)  {
  int  x, y;

  assert(MAGIC(b));
  assert((strlen(l) == 2) || (strlen(l) == 4));
  if (l[2] == '\0')  {
    x = l[0] - 'a';
    y = l[1] - 'a';
  } else  {
    x = (l[0] - 'a') * 26 + (l[1] - 'a');
    y = (l[2] - 'a') * 26 + (l[3] - 'a');
  }
  assert((x >= 0) && (x < goBoard_size(b)));
  assert((y >= 0) && (y < goBoard_size(b)));
  return(goBoard_xy2Loc(b, x, y));
}


#if  DEBUG_SHOWLINKS

static void  showLinks(GoBoard *board)  {
  GoBoardPiece  *group;
  int  loc;
  char  locStr[4];

  printf("Links:\n");
  for (loc = 0;  loc < board->area;  ++loc)  {
    if (goStone_isStone(board->pieces[loc].type) &&
	board->pieces[loc].head == &board->pieces[loc])  {
      printf("  Color = %c, libs=%d, size=%d\n",
	     goStone_char(board->pieces[loc].type),
	     board->pieces[loc].libs, board->pieces[loc].size);
      for (group = &board->pieces[loc];  group;  group = group->next)  {
	goBoard_loc2Str(board, group - &board->pieces[0], locStr);
	printf("    %s\n", locStr);
	assert(group->head == &board->pieces[loc]);
      }
    }
  }
  printf("----\n");
}


#endif  /* DEBUG_SHOWLINKS */
