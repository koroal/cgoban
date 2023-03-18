/*
 * wmslib/src/but/i_finder.c, part of wmslib (Library functions)
 * Copyright (C) 1994 William Shubert.
 * See "configure.h.in" for more copyright information.
 */

#include <configure.h>

#ifdef  X11_DISP

#ifdef  STDC_HEADERS
#include <stdlib.h>
#include <unistd.h>
#endif  /* STDC_HEADERS */
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
#include <wms.h>
#include <but/but.h>

static void  addButToCol(ButCol *col, But *but, int y1, int y2);
static void  addButToRow(ButRow *row, But *but);
static void  copyCol(ButCol *dest, ButCol *src);
static void  copyRow(ButRow *dest, ButRow *src);
static void  findButSetInCol(ButCol *col, int x, int y, ButSet *butset);
static void  butDelFromCol(ButCol *col, But *but);
static void  butDelFromRow(ButRow *row, But *but);
static bool  colEq(ButCol *c1, ButCol *c2);
static bool  rowEq(ButRow *r1, ButRow *r2);
static void  colDel(ButWin *win, int col_num);
static void  rowDel(ButCol *col, int row_num);
static int  markForTable(But *but, void *packet);
static int  addToTable(But *but, void *packet);
static int  butcomp(const void *a, const void *b);

#if  0  /* Useful sometimes for debugging. */
void  printf_table(but_win_t *win);
void  printf_env(but_env_t *win);
#endif

void  butWin_addToTable(ButWin *win)  {
  ButEnv  *env = win->env;
  ButWin  **new_winlist;
  int  i, j, new_pos;

  assert(MAGIC(win));
  if (win->win != None)  {
    /*
     * If win->win is None, then it is a canvas, which does not have to
     *   be entered into the window table yet.
     */
    if (env->wlmax == env->wllen)  {
      new_winlist = wms_malloc(sizeof(ButWin *) * (env->wllen + 1) * 2);
      for (i = 0;  i < env->wllen;  ++i)
	new_winlist[i] = env->winlist[i];
      if (env->winlist != NULL)
	wms_free(env->winlist);
      env->winlist = new_winlist;
      env->wlmax = (env->wllen + 1) * 2;
    }

    for (i = 0;  (i < env->wllen) && (win->win > env->winlist[i]->win);  ++i);
    new_pos = i;
    for (i = env->wllen;  i > new_pos;  --i)
      env->winlist[i] = env->winlist[i-1];
    env->winlist[new_pos] = win;
    ++env->wllen;
  }

  if (win->numCols == 0)  {
    /*
     * A canvas may call this function every time the pixmap changes, so
     *   we have to make sure that we don't re-allocate all this stuff!
     */
    win->numCols = 2;
    win->maxCols = 2;
    win->cols = wms_malloc(2 * sizeof(ButCol));
    win->cols[0].startX = int_min;
    win->cols[1].startX = int_max;
    for (i = 0;  i < 2;  ++i)  {
      MAGIC_SET(&win->cols[i]);
      win->cols[i].numRows = 2;
      win->cols[i].maxRows = 2;
      win->cols[i].rows = wms_malloc(2 * sizeof(ButRow));
      win->cols[i].rows[0].startY = int_min;
      win->cols[i].rows[1].startY = int_max;
      for (j = 0;  j < 2;  ++j)  {
	MAGIC_SET(&win->cols[i].rows[j]);
	win->cols[i].rows[j].numButs = 0;
	win->cols[i].rows[j].maxButs = 0;
	win->cols[i].rows[j].buts = NULL;
      }
    }
  }
#if  DEBUG
  for (i = 1;  i < env->wllen;  ++i)  {
    assert(env->winlist[i]->win > env->winlist[i - 1]->win);
  }
#endif  /* DEBUG */
}


void  butWin_rmFromTable(ButWin *win)  {
  ButEnv  *env = win->env;
  int  i;

  assert(MAGIC(win));
  assert(MAGIC(env));
  for (i = 0;  (env->winlist[i] != win) && (i < env->wllen);  ++i);
  if (i < env->wllen)  {
    --env->wllen;
    while (i < env->wllen)  {
      env->winlist[i] = env->winlist[i+1];
      ++i;
    }
  }
#if  DEBUG
  for (i = 1;  i < env->wllen;  ++i)  {
    assert(env->winlist[i]->win > env->winlist[i - 1]->win);
  }
#endif  /* DEBUG */
}


void  but_addToTable(But *but)  {
  ButWin  *win = but->win;
  int  x1 = but->x, y1 = but->y, x2 = but->x + but->w, y2 = but->y + but->h;
  int  i, src, dest;
  ButCol  *new_cl = win->cols;
  int  new_cll = win->numCols, new_clmax = win->maxCols;
  int  x1done = 0, x2done = 0;
  
  assert(MAGIC(but));
  assert(but->flags & BUT_DRAWABLE);
  butSet_delBut(&win->butsNoDraw, but);
  for (i = 0;  i < new_cll;  ++i)  {
    assert(MAGIC(&new_cl[i]));
    if (new_cl[i].startX == x1)
      x1done = 1;
    else if (new_cl[i].startX == x2)
      x2done = 1;
  }
  new_cll += 2 - (x1done + x2done);
  if (new_cll > win->maxCols)  {
    new_cl = wms_malloc(sizeof(ButCol) * (new_clmax = new_cll * 2));
    for (i = 0;  i < new_clmax;  ++i)  {
      MAGIC_SET(&new_cl[i]);
      new_cl[i].rows = NULL;
    }
  }
  for (src = win->numCols, dest = new_cll;  src > 0;)  {
    --src;
    --dest;
    assert(MAGIC(&win->cols[src]));
    if (!x2done && (win->cols[src].startX < x2))  {
      assert(MAGIC(&new_cl[dest]));
      copyCol(&new_cl[dest], &win->cols[src]);
      new_cl[dest--].startX = x2;
      x2done = 1;
    }
    if (!x1done && (win->cols[src].startX < x1))  {
      assert(MAGIC(&new_cl[dest]));
      copyCol(&new_cl[dest], &win->cols[src]);
      new_cl[dest].startX = x1;
      addButToCol(&new_cl[dest--], but, y1, y2);
      x1done = 1;
    }
    assert(MAGIC(&new_cl[dest]));
    new_cl[dest] = win->cols[src];
    if ((new_cl[dest].startX < x2) && (new_cl[dest].startX >= x1))
      addButToCol(&new_cl[dest], but, y1, y2);
  }
  if (win->cols != new_cl)  {
    for (i = 0;  i < win->maxCols;  ++i)
      MAGIC_UNSET(&win->cols[i]);
    wms_free(win->cols);
    win->cols = new_cl;
  }
  win->maxCols = new_clmax;
  win->numCols = new_cll;
}


static void  addButToCol(ButCol *col, But *but, int y1, int y2)  {
  ButRow  *new_rl = col->rows;
  int  i, src, dest, new_rll = col->numRows, new_rlmax = col->maxRows;
  int  y1done=0, y2done=0;
  
  assert(MAGIC(col));
  assert(MAGIC(but));
  for (i = 0;  i < col->numRows;  ++i)  {
    assert(MAGIC(&col->rows[i]));
    if (col->rows[i].startY == y1)
      y1done = 1;
    if (col->rows[i].startY == y2)
      y2done = 1;
  }
  new_rll += 2 - (y1done + y2done);
  if (new_rll > col->maxRows)  {
    new_rl = wms_malloc(sizeof(ButRow) * (new_rlmax = new_rll * 2));
    for (i = 0;  i < new_rlmax;  ++i)  {
      MAGIC_SET(&new_rl[i]);
      new_rl[i].buts = NULL;
    }
  }
  for (src = col->numRows, dest = new_rll;  src > 0;)  {
    --src;
    --dest;
    assert(MAGIC(&col->rows[src]));
    if (!y2done && (col->rows[src].startY < y2))  {
      assert(MAGIC(&new_rl[dest]));
      copyRow(&new_rl[dest], &col->rows[src]);
      new_rl[dest--].startY = y2;
      y2done = 1;
    }
    if (!y1done && (col->rows[src].startY < y1))  {
      assert(MAGIC(&new_rl[dest]));
      copyRow(&new_rl[dest], &col->rows[src]);
      new_rl[dest].startY = y1;
      addButToRow(&new_rl[dest--], but);
      y1done = 1;
    }
    assert(MAGIC(&new_rl[dest]));
    new_rl[dest] = col->rows[src];
    if ((new_rl[dest].startY < y2) && (new_rl[dest].startY >= y1))
      addButToRow(&new_rl[dest], but);
  }
  if (col->rows != new_rl)  {
    if (col->rows != NULL)  {
      wms_free(col->rows);
      for (i = 0;  i < col->maxRows;  ++i)
	MAGIC_UNSET(&col->rows[i]);
    }
    col->rows = new_rl;
  }
  col->maxRows = new_rlmax;
  col->numRows = new_rll;
}


static void  addButToRow(ButRow *row, But *but)  {
  int  i, new_maxButs = row->maxButs, new_numButs = row->numButs + 1;
  But  **new_bl = row->buts;

  assert(MAGIC(row));
  assert(MAGIC(but));
  if (new_numButs >= row->maxButs)  {
    new_bl = wms_malloc(sizeof(But *) * (new_maxButs = new_numButs * 2));
    for (i = 0;  i < row->numButs;  ++i)
      new_bl[i] = row->buts[i];
  }
  for (i = row->numButs - 1;
       (i >= 0) && (but->layer < row->buts[i]->layer);  --i)  {
    assert(MAGIC(new_bl[i]));
    assert(new_bl[i] != but);
    new_bl[i+1] = new_bl[i];
  }
  new_bl[i+1] = but;
#if  DEBUG
  for (;  i >= 0;  --i)
    assert(new_bl[i] != but);
#endif
  if ((row->buts != NULL) && (row->buts != new_bl))
    wms_free(row->buts);
  row->buts = new_bl;
  row->numButs = new_numButs;
  row->maxButs = new_maxButs;
}


static void  copyCol(ButCol *dest, ButCol *src)  {
  int  i;

  assert(MAGIC(dest));
  assert(MAGIC(src));
  *dest = *src;
  dest->rows = wms_malloc(dest->maxRows * sizeof(ButRow));
  for (i = 0;  i < dest->maxRows;  ++i)
    MAGIC_SET(&dest->rows[i]);
  for (i = 0;  i < dest->numRows;  ++i)
    copyRow(&dest->rows[i], &src->rows[i]);
}


static void  copyRow(ButRow *dest, ButRow *src)  {
  int  i;

  assert(MAGIC(dest));
  assert(MAGIC(src));
  *dest = *src;
  dest->buts = wms_malloc(dest->maxButs * sizeof(But *));
  for (i = 0;  i < dest->numButs;  ++i)
    dest->buts[i] = src->buts[i];
}


void  butWin_findButSet(ButWin *win, int x, int y, ButSet *butset)  {
  unsigned  hlen, result = 0, cllen = win->numCols;
  
  assert(MAGIC(win));
  butset->dynamic = FALSE;
  while ((hlen = (cllen >> 1)))  {
    assert(MAGIC(&win->cols[result+hlen]));
    if (win->cols[result+hlen].startX > x)  {
      cllen = hlen;
    } else  {
      cllen -= hlen;
      result += hlen;
    }
  }
  findButSetInCol(&win->cols[result], x, y, butset);
}


static void  findButSetInCol(ButCol *col, int x, int y, ButSet *butset)  {
  unsigned  hlen, result = 0, rllen = col->numRows;
  
  assert(MAGIC(col));
  while ((hlen = (rllen >> 1)))  {
    assert(MAGIC(&col->rows[result+hlen]));
    if (col->rows[result+hlen].startY > y)  {
      rllen = hlen;
    } else  {
      rllen -= hlen;
      result += hlen;
    }
  }
  butset->numButs = col->rows[result].numButs;
  butset->buts = col->rows[result].buts;
  butset->dynamic = FALSE;
}


void  but_delFromTable(But *but)  {
  ButWin  *win = but->win;
  int  i;
  ButCol  *cl = win->cols;
 
  assert(MAGIC(but));
  assert(MAGIC(win));
  for (i = 1;  i < win->numCols - 1;  ++i)  {
    assert(MAGIC(&cl[i]));
    if ((cl[i].startX >= but->x) &&
	(cl[i].startX < but->x + but->w))  {
      butDelFromCol(&cl[i], but);
      if (colEq(&cl[i-1], &cl[i]))  {
	colDel(win, i);
	--i;
      }
      if ((i+1 < win->numCols - 1) && colEq(&cl[i], &cl[i+1]))
	colDel(win, i+1);
    }
  }
}


static bool  colEq(ButCol *c1, ButCol *c2)  {
  int  i;

  assert(MAGIC(c1));
  assert(MAGIC(c2));
  if (c1->numRows != c2->numRows)
    return(FALSE);
  for (i = 0;  i < c1->numRows;  ++i)  {
    if (!rowEq(&c1->rows[i], &c2->rows[i]))
      return(FALSE);
  }
  return(TRUE);
}


static bool  rowEq(ButRow *r1, ButRow *r2)  {
  int  i;

  assert(MAGIC(r1));
  assert(MAGIC(r2));
  if (r1->numButs != r2->numButs)
    return(FALSE);
  for (i = 0;  i < r1->numButs;  ++i)  {
    if (r1->buts[i] != r2->buts[i])
      return(FALSE);
  }
  return(TRUE);
}


static void  butDelFromCol(ButCol *col, But *but)  {
  int  i;
  ButRow  *rl = col->rows;
 
  assert(MAGIC(col));
  for (i = 1;  i < col->numRows - 1;  ++i)  {
    assert(MAGIC(&rl[i]));
    if ((rl[i].startY >= but->y) &&
	(rl[i].startY < but->y + but->h))  {
      butDelFromRow(&rl[i], but);
      if (rowEq(&rl[i-1], &rl[i]))  {
	rowDel(col, i);
	--i;
      }
      if ((i+1 < col->numRows - 1) && rowEq(&rl[i], &rl[i+1]))
	rowDel(col, i+1);
    }
  }
}


static void  butDelFromRow(ButRow *row, But *but)  {
  int  dest, src;

  assert(MAGIC(row));
  for (dest = 0, src = 0;  src < row->numButs;  ++src)  {
    assert(MAGIC(row->buts[src]));
    row->buts[dest] = row->buts[src];
    if (row->buts[dest] != but)  {
      ++dest;
    } else  {
      assert(src == dest);
    }
  }
  assert(src == dest + 1);
  --row->numButs;
  assert(dest == row->numButs);
}


static void  colDel(ButWin *win, int col_num)  {
  int  i;

  assert(MAGIC(win));
  assert(MAGIC(&win->cols[col_num]));
  for (i = 0;  i < win->cols[col_num].numRows;  ++i)  {
    assert(MAGIC(&win->cols[col_num].rows[i]));
    if (win->cols[col_num].rows[i].buts != NULL)
      wms_free(win->cols[col_num].rows[i].buts);
  }
  wms_free(win->cols[col_num].rows);
  --win->numCols;
  for (i = col_num;  i < win->numCols;  ++i)
    win->cols[i] = win->cols[i+1];
}


static void  rowDel(ButCol *col, int row_num)  {
  int  i;

  assert(MAGIC(col));
  assert(MAGIC(&col->rows[row_num]));
  if (col->rows[row_num].buts != NULL)
    wms_free(col->rows[row_num].buts);
  --col->numRows;
  for (i = row_num;  i < col->numRows;  ++i)
    col->rows[i] = col->rows[i+1];
}


void  butWin_findButSetInRegion(ButWin *win, int x, int y, int w, int h,
				ButSet *butset)  {
  assert(MAGIC(win));
  butset->numButs = 0;
  butset->dynamic = TRUE;
  butWin_findButsInRegion(win, x,y, w,h, markForTable, butset);
  if (butset->numButs > 0)  {
    butset->buts = wms_malloc(butset->numButs * sizeof(But *));
    butset->maxButs = butset->numButs;
    butset->numButs = 0;
    butWin_findButsInRegion(win, x,y, w,h, addToTable, butset);
    qsort(butset->buts, butset->numButs, sizeof(But *), butcomp);
  } else  {
    butset->dynamic = FALSE;
    butset->buts = NULL;
  }
}


static int markForTable(But *but, void *packet)  {
  ButSet *butset = packet;

  if (but == NULL) {
    return(0);
  }
  assert(MAGIC(but));
  if ((but->flags & (BUT_TABLED|BUT_DEAD|BUT_DRAWABLE)) == BUT_DRAWABLE)  {
    ++butset->numButs;
    but->flags |= BUT_TABLED;
  }
  return(0);
}


static int  addToTable(But *but, void *packet)  {
  ButSet  *butset = packet;

  assert(MAGIC(but));
  if (but->flags & BUT_TABLED)  {
    butset->buts[butset->numButs++] = but;
    assert(butset->numButs <= butset->maxButs);
    but->flags &= ~BUT_TABLED;
  }
  return(0);
}


static int  butcomp(const void *a, const void *b)  {
  const But  *but_a = *(But **)a;
  const But  *but_b = *(But **)b;

  assert(MAGIC(but_a));
  assert(MAGIC(but_b));
  if (but_a->layer != but_b->layer)
    return(but_a->layer - but_b->layer);
  else if (but_a > but_b)
    return(-1);
  else
    return(1);
}


ButWin  *butEnv_findWin(ButEnv *env, Window win)  {
  int  wmin = 0, wmax = env->wllen, wnum;

#if  DEBUG
  for (wnum = 1;  wnum < env->wllen;  ++wnum)  {
    assert(env->winlist[wnum]->win > env->winlist[wnum - 1]->win);
  }
#endif  /* DEBUG */
  while (wmin < wmax)  {
    wnum = (wmax + wmin) >> 1;
    assert(MAGIC(env->winlist[wnum]));
    if (env->winlist[wnum]->win == win)  {
      return(env->winlist[wnum]);
    }
    if (env->winlist[wnum]->win > win)  {
      wmax = wnum;
    } else  {
      wmin = wnum + 1;
    }
  }
  return(NULL);
}
    

But  *butWin_findButsInRegion(ButWin *win, int x,int y, int w,int h,
			      bool (*action)(But *but, void *packet),
			      void *packet)  {
  int  r, c, b;
  ButCol  *col;
  ButRow  *row;
  But *but;

  assert(MAGIC(win));
  for (c = 1;  c < win->numCols - 1;  ++c)  {
    col = &win->cols[c];
    assert(MAGIC(col));
    if ((col->startX < x+w) && (win->cols[c+1].startX > x))  {
      for (r = 1;  r < col->numRows - 1;  ++r)  {
	row = &col->rows[r];
	assert(MAGIC(row));
	if ((row->startY < y+h) && (col->rows[r+1].startY > y))  {
	  for (b = 0;  b < row->numButs;  ++b)  {
	    but = row->buts[b];
	    assert(MAGIC(but));
	    if (action(but, packet))
	      return(but);
	  }
	}
      }
    }
  }
  return(NULL);
}


#if  0  /* Sometimes useful for debugging. */
void  printf_env(but_env_t  *env)  {
  int  i;

  printf("Env has %d (max %d) windows:\n", env->wllen, env->wlmax);
  for (i = 0;  i < env->wllen;  ++i)
    printf("  0x%x (x win %d)\n", (int)env->winlist[i],
	   (int)(env->winlist[i]->win));
}


void  printf_table(but_win_t  *win)  {
  int  i, j, k;

  for (i = 0;  i < win->numCols - 1;  ++i)  {
    printf("Col %d of %d: %d..%d\n", i+1, win->numCols - 1,
	   win->cols[i].startX, win->cols[i+1].startX - 1);
    for (j = 0;  j < win->cols[i].numRows - 1;  ++j)  {
      printf("  Row %d of %d: %d..%d\n", j+1, win->cols[i].numRows - 1,
	     win->cols[i].rows[j].startY, win->cols[i].rows[j+1].startY - 1);
      if (win->cols[i].rows[j].numButs == 0)
	printf("    No buttons.\n");
      else  {
	for (k = 0;  k < win->cols[i].rows[j].numButs;  ++k)
	  printf("    But 0x%x\n", (int)win->cols[i].rows[j].buts[k]);
      }
    }
  }
  printf("---\n");
}
#endif
	     

void  butSet_addBut(ButSet *butset, But *but)  {
  int  i, new_maxButs;
  But  **newbuts;

  assert(MAGIC(butset));
  assert(butset->dynamic);
  for (i = 0;  i < butset->numButs;  ++i)  {
    if (butset->buts[i] == but)
      return;
  }
  if (butset->numButs == butset->maxButs)  {
    new_maxButs = (butset->maxButs + 1) * 2;
    newbuts = wms_malloc(new_maxButs * sizeof(But *));
    if (butset->buts != NULL)  {
      memcpy(newbuts, butset->buts, butset->maxButs * sizeof(But *));
      wms_free(butset->buts);
    }
    butset->buts = newbuts;
    butset->maxButs = new_maxButs;
  }
  butset->buts[butset->numButs++] = but;
}


void  butSet_delBut(ButSet *butset, But *but)  {
  int  i;
  
  assert(MAGIC(butset));
  for (i = 0;  i < butset->numButs;  ++i)  {
    if (butset->buts[i] == but)  {
      butset->buts[i] = butset->buts[--butset->numButs];
      return;
    }
  }
}


#endif
