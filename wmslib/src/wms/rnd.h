/*
 * wmslib/src/wms/rnd.h, part of wmslib (Library functions)
 * Copyright (C) 1994-1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 */

#ifndef  _WMS_RND_H_
#define  _WMS_RND_H_  1

#ifndef  _WMS_H_
#include <wms.h>
#endif

#define  RND_DATASIZE  31
#define  RND_INDENT    3
typedef struct Rnd_struct  {
	int  indent;
	uint32  data[RND_DATASIZE];

	MAGIC_STRUCT
} Rnd;


extern int  rnd_int(Rnd *state);  /* No negative numbers. */
extern uint  rnd_uint(Rnd *state);  /* Random bit field. */
extern int  rnd_int32(Rnd *state);
extern uint  rnd_uint32(Rnd *state);
extern Rnd  *rnd_create(uint seed);
extern void  rnd_destroy(Rnd *state);
extern float  rnd_float(Rnd *state);

#endif  /* _WMS_RND_H_ */
