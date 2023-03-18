/*
 * src/gohash.c, part of Complete Goban (game program)
 * Copyright (C) 1995 William Shubert
 * See "configure.h.in" for more copyright information.
 */

#include <wms.h>
#include <wms/rnd.h>
#include "goHash.h"


#if  GoHash_isScalar
const GoHash  goHash_zero = 0;
const GoHash  goHash_pass = 1;
#else
const GoHash  goHash_zero = {0, 0};
const GoHash  goHash_pass = {1, 0};
#endif  /* GoHash_isScalar */


GoHash  goHash_init(Rnd *r)  {
  GoHash  v;
  int  i;

#if  GoHash_isScalar
  v = 0;
  for (i = 0;  i < sizeof(GoHash) / sizeof(uint32);  ++i)
    v = (v << 32) | (GoHash)rnd_uint32(r);
  v &= ~1;
#else
  v.hi = rnd_uint32(r);
  v.lo = rnd_uint32(r) & ~1;
#endif  /* GoHash_isScalar */
  return(v);
}


#if  !GoHash_isScalar


GoHash  goHash_xor(GoHash v1, GoHash v2)  {
  v1.lo ^= v2.lo;
  v1.hi ^= v2.hi;
  return(v1);
}


#endif  /* !GoHash_isScalar */
