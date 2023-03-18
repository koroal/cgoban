/*
 * src/gohash.h, part of Complete Goban (game program)
 * Copyright (C) 1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _GOHASH_H_
#define  _GOHASH_H_  1

#ifndef  _WMS_RND_H_
#include <wms/rnd.h>
#endif


/**********************************************************************
 * Data types
 **********************************************************************/

#if  SIZEOF_LONG >= 8
#define  GoHash_isScalar  1
typedef ulong  GoHash;
#elif  SIZEOF_LONG_LONG >= 8
#define  GoHash_isScalar  1
typedef unsigned long long  GoHash;
#else
#define  GoHash_isScalar  0
typedef struct  {
  uint32  lo, hi;
} GoHash;
#endif

/**********************************************************************
 * Global variables
 **********************************************************************/
extern const GoHash  goHash_zero;
extern const GoHash  goHash_pass;

/**********************************************************************
 * Functions
 **********************************************************************/

/* In all of these functions, passing zero as the "loc" means a pass. */
GoHash  goHash_init(Rnd *r);

#if  GoHash_isScalar

#define  goHash_xor(v1, v2)  ((v1) ^ (v2))
#define  goHash_eq(v1, v2)  ((v1) == (v2))
#define  goHash_loBits(v, m)  ((v) & (m))
#define  goHash_print(v)  printf("0x%x%08x",(int)(v>>32),(int)v)

#else

extern GoHash  goHash_xor(GoHash v1, GoHash v2);
#define  goHash_eq(v1, v2)  (((v1).lo == (v2).lo) && ((v1).hi == (v2).hi))
#define  goHash_loBits(v, m)  ((v).lo & (m))

#endif  /* GoHash_isScalar */

#endif  /*  _GOHASH_H_ */
