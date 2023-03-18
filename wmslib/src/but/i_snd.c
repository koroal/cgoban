/*
 * wmslib/src/but/i_snd.c, part of wmslib (Library functions)
 * Copyright (C) 1994 William Shubert.
 * See "configure.h.in" for more copyright information.
 */

#include <wms.h>

#if  X11_DISP

#include <wms/snd.h>
#include <but/but.h>

static uchar  butdown[] = {
  /*
   * Actual noise: The final click as you slam a pair of scissors
   *   shut.
   */
  88,93,188,250,62,199,229,69,77,197,111,68,206,217,77,59,198,62,
  79,203,216,95,207,188,167,199,35,78,89,159,163,45,155,177,106,47,
  159,169,31,170,58,90,45,73,45,216,42,75,58,186,55,120,175,61,69,
  191,121,44,181,122,86,64,189,93,74,187,189,100,185,183,239,196};

static uchar  butup[] = {
  /*
   * Actual noise: The final click as you slam a pair of scissors
   *   shut.
   * This is a slightly different sample than butdown; more echoish.
   */
  171,71,87,89,55,224,211,203,66,91,90,53,175,188,70,72,76,76,230,
  190,174,56,55,213,79,91,193,216,77,87,202,199,220,95,79,70,58,
  187,175,61,172,86,48,189,198,92,96,205,67,78,198,179,56,55,191,
  44,207,180,54,186,206,69,180,185,43,187,184,67,195,93,164,208,71,
  156,229,163,44,24,156,168,71,23,235,166,144,173,207,149,221,237,
  30,49,26};

Snd  but_downSnd = snd_define(butdown);
Snd  but_upSnd = snd_define(butup);

#endif  /* X11_DISP */
