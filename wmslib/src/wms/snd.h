/*
 * wmslib/include/snd.h, part of wmslib (Library functions)
 * Copyright (C) 1994 William Shubert.
 * See "configure.h.in" for more copyright information.
 */

#ifndef  _WMS_SND_H_
#define  _WMS_SND_H_  1

/**********************************************************************
 * Data types
 **********************************************************************/

/* Public. */
typedef enum  {
  sndInit_ok, sndInit_busy, sndInit_broken
} SndInit;

/*
 * This is a public type, but the "sndState_i*" values should never be used
 *   by the application - they are for internal use only.
 */
typedef enum  {
  sndState_iOff, sndState_off, sndState_partOpen, sndState_iWantOpen,
  sndState_fullOpen, sndState_oldState, sndState_iTempOpen
} SndState;

/* Opaque. */
typedef struct Snd_struct  {
  int  len;
  uchar  *data;
  bool  converted;
  int  cvtlen;
  union  {
    uchar  *dsp;
    ushort  *dsp1;
  } cvtdata;
  struct Snd_struct  *next;
} Snd;

/**********************************************************************
 * Global variables
 **********************************************************************/

/*
 * Feel free to set this pointer to point to any old handler.  By default
 *   it points to a routine that dumps errStr to stderr for the first
 *   error then ignores the rest.
 * It gets called ONLY by snd_play; snd_init returns an error code that you
 *   should check for.
 * errStr is always the same as snd_error.
 */
extern void  (*snd_errHandler)(SndInit errType, char *errStr);
extern char  snd_error[];

/**********************************************************************
 * Functions
 **********************************************************************/
extern SndInit  snd_init(SndState newState, int newVolume);
extern void  snd_play(Snd *sound);
extern void  snd_deinit(void);

/**********************************************************************
 * Macros, constants, etc.
 **********************************************************************/

#define  snd_define(data)  {sizeof(data), data, FALSE,0,{NULL},NULL}

#define  SND_MAXVOL  10000  /* Just for kicks, you know? */

#if  SUN_SOUND || LINUX_SOUND
#define  SND_AVAILABLE  1
#else
#define  SND_AVAILABLE  0
#endif

#endif  /* _WMS_SND_H_ */
