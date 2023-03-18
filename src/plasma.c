/*
 * src/xio/plasma.c, part of Complete Goban (game program)
 * Copyright (C) 1994-1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 */

#include <wms.h>
#include <wms/rnd.h>
#include "plasma.h"


#define  FACTOR  0.7
#define  SIZE  PLASMA_SIZE
#define  STRETCH  3

static void  vert(int *cloud, int step, int skew, Rnd *rnd);
static void  diag(int *, int, int, Rnd *), perp(int *, int, int, Rnd *);
static int  my_random(int limit, Rnd *rnd);


uchar  *plasma(void)  {
  static bool  done = FALSE;
  int  *cloud, x;
  Rnd  *rnd;
  static unsigned char  *cout = NULL;
  int  step, i;
  float  fskew;
  
  if (done)
    return(cout);
  done = TRUE;
  rnd = rnd_create(time(NULL));
  cout = wms_malloc(SIZE * SIZE * sizeof(char));
  fskew = (float)(750 << 8);
  cloud = wms_malloc(SIZE * (SIZE+1) * sizeof(int));
  assert(cloud != NULL);
  if (cloud == NULL)  {
    fprintf(stderr, "Could not allocate enough memory.\n");
    exit(1);
  }
  *cloud = my_random(256 << 16, rnd);
  for (step = SIZE/2;  step;  step /= 2)  {
    diag(cloud, step, (int)fskew, rnd);
    fskew *= FACTOR;
    if (fskew < 1.0)
      fskew = 1.0;
    perp(cloud, step, (int)fskew, rnd);
    fskew *= FACTOR;
    if (fskew < 1.0)
      fskew = 1.0;
  }
#if  STRETCH
  for (i = PLASMA_SIZE*PLASMA_SIZE/(1<<STRETCH);  i >= 0;  i -= PLASMA_SIZE)  {
    memcpy(cloud+(i<<STRETCH),cloud+i, PLASMA_SIZE*sizeof(*cloud));
  }
  for (step = 1<<(STRETCH-1);  step;  step >>= 1)  {
    fskew = 1.0;
    vert(cloud, step, (int)fskew, rnd);
    fskew *= FACTOR*FACTOR;
    if (fskew < 1.0)
      fskew = 1.0;
  }
#endif  /* STRETCH */
  for (i = 0;  i < SIZE*SIZE;  ++i)  {
    x = (cloud[i] >> 8) & 511;
    if (x > 255)
      x = 511 - x;
    cout[i] = x;
  }
  wms_free(cloud);
  rnd_destroy(rnd);
  return(cout);
}


#define  CLACC(x, y)  cloud[(x) + ((y)*SIZE)]
static void  diag(int *cloud, int step, int skew, Rnd *rnd)  {
  int  i, j;
  int  sum;
  
  for (i = 0;  i < SIZE;  i += step)
    for (j = 0;  j < SIZE;  j += step)  {
      if (((i & step) != 0) && ((j & step) != 0))  {
	sum = (CLACC(i-step, j-step) +
	       CLACC(i-step, (j+step)&(SIZE-1)) +
	       CLACC((i+step)&(SIZE-1), j-step) +
	       CLACC((i+step)&(SIZE-1), (j+step)&(SIZE-1))) / 4;
	CLACC(i, j) = sum + my_random(skew, rnd);
      }
    }
}


static void  perp(int *cloud, int step, int skew, Rnd *rnd)  {
  int  i, j;
  int  sum;
  
  for (i = 0;  i < SIZE;  i += step)
    for (j = 0;  j < SIZE;  j += step)  {
      if ((i & step) != (j & step))  {
	sum = (CLACC((i-step)&(SIZE-1), j) +
	       CLACC(i, (j+step)&(SIZE-1)) +
	       CLACC((i+step)&(SIZE-1), j) +
	       CLACC(i, (j-step)&(SIZE-1))) / 4.0;
	CLACC(i, j) = sum + my_random(skew, rnd);
      }
    }
}


static void  vert(int *cloud, int step, int skew, Rnd *rnd)  {
  int  i, j;
  int  sum;
  
  for (i = 0;  i < SIZE;  ++i)
    for (j = step;  j < SIZE;  j += (step<<1))  {
      sum = (CLACC((i-1)&(SIZE-1), j-step) +
	     CLACC(i, j-step) +
	     CLACC((i+1)&(SIZE-1), j-step) +
	     CLACC((i-1)&(SIZE-1), j+step) +
	     CLACC(i, j+step) +
	     CLACC((i+1)&(SIZE-1), j+step)) / 6.0;
      CLACC(i, j) = sum + my_random(skew, rnd);
    }
}


static int  my_random(int limit, Rnd *rnd)  {
  int  i;
  
  i = rnd_int(rnd);
  if (i & 8)
    return(rnd_int(rnd) % limit);
  else
    return(-(rnd_int(rnd) % limit));
}
