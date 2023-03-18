/*
 * wmslib/src/wms/rnd.c, part of wmslib (Library functions)
 * Copyright (C) 1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#include <wms.h>
#include <wms/rnd.h>


int  rnd_int(Rnd *state)  {
  uint  result;
  int  destIndent;

  assert(MAGIC(state));
  destIndent = state->indent + RND_INDENT;
  if (destIndent >= RND_DATASIZE)
    destIndent -= RND_DATASIZE;
  result = (state->data[destIndent] += state->data[state->indent]);
  if (++state->indent >= RND_DATASIZE)
    state->indent = 0;
  return((int)(result >> 1));
}


uint  rnd_uint(Rnd *state)  {
  uint  result;
  int  destIndent;

  assert(MAGIC(state));
  destIndent = state->indent + RND_INDENT;
  if (destIndent >= RND_DATASIZE)
    destIndent -= RND_DATASIZE;
  result = (state->data[destIndent] += state->data[state->indent]);
  if (++state->indent >= RND_DATASIZE)
    state->indent = 0;
  return(result);
}


int32  rnd_int32(Rnd *state)  {
  uint32  result;
  int  destIndent;

  assert(MAGIC(state));
  destIndent = state->indent + RND_INDENT;
  if (destIndent >= RND_DATASIZE)
    destIndent -= RND_DATASIZE;
  result = (state->data[destIndent] += state->data[state->indent]);
  if (++state->indent >= RND_DATASIZE)
    state->indent = 0;
  return((int32)(result >> 1));
}


uint32  rnd_uint32(Rnd *state)  {
  uint32  result;
  int  destIndent;

  assert(MAGIC(state));
  destIndent = state->indent + RND_INDENT;
  if (destIndent >= RND_DATASIZE)
    destIndent -= RND_DATASIZE;
  result = (state->data[destIndent] += state->data[state->indent]);
  if (++state->indent >= RND_DATASIZE)
    state->indent = 0;
  return(result);
}


Rnd  *rnd_create(uint seed)  {
  int  i;
  Rnd  *state;

  state = wms_malloc(sizeof(Rnd));
  MAGIC_SET(state);
  state->indent = 0;
  for (i = 0;  i < RND_DATASIZE;  ++i)  {
    state->data[i] = seed;
    seed = seed * 13 + 3;
  }
  for (i = 0;  i < 100;  ++i)
    rnd_uint32(state);
  return(state);
}


void  rnd_destroy(Rnd *state)  {
  assert(MAGIC(state));
  MAGIC_UNSET(state);
  wms_free(state);
}


float  rnd_float(Rnd *state)  {
  int  result;
  float  out;

#if  SIZEOF_INT > 2
  result = rnd_int(state) & 0xffffff;
  out = result / 16777216.0;
  return(out);
#else
  result = rnd_int(state) & 0x7fff;
  out = result / 32768.0;
  return(out);
#endif
}
