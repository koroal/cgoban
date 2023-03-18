/*
 * src/gotime.h, part of Complete Goban (game program)
 * Copyright (C) 1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _GOTIME_H_
#define  _GOTIME_H_  1

#ifndef  _WMS_STR_H_
#include <wms/str.h>
#endif


/**********************************************************************
 * Data types
 **********************************************************************/
typedef enum  {
  goTime_none, goTime_absolute, goTime_japanese, goTime_canadian,
  goTime_ing
} GoTimeType;


typedef struct  {
  GoTimeType  type;
  int  main;
  int  by;
  int  aux;
} GoTime;


typedef struct  {
  int  timeLeft, aux;
  int  usLeft;
  struct timeval startTime;
} GoTimer;


/**********************************************************************
 * Functions
 **********************************************************************/
void  goTime_str(int time, Str *out);
int  goTime_parseChars(const char *t, bool ignoreExtra, bool *err);
void  goTime_describeStr(const GoTime *time, Str *out);
void  goTime_parseDescribeChars(GoTime *time, const char *desc);
void  goTime_remainStr(const GoTime *time, const GoTimer *timer, Str *out);

void  goTimer_init(GoTimer *timer, const GoTime *time);

void  goTime_startTimer(const GoTime *time, GoTimer *timer);
/*
 * goTime_checkTimer() and goTime_endTimer() return FALSE if you are out of
 *   time.
 */
bool  goTime_checkTimer(const GoTime *time, GoTimer *timer);
bool  goTime_endTimer(const GoTime *time, GoTimer *timer);
int  goTime_ingPenalty(const GoTime *time, const GoTimer *timer);
#define  goTime_out(tm, tmr)  (((tm)->type != goTime_none) &&  \
			       ((tmr)->timeLeft < 0))
#define goTime_low(tm, tmr, limit)					\
  ((((tm)->type == goTime_absolute) && ((tmr)->timeLeft < limit)) ||	\
   (((tm)->type == goTime_canadian) && ((tmr)->aux > 0) &&		\
    ((tmr)->timeLeft < limit)) ||					\
   (((tm)->type == goTime_japanese) && ((tmr)->timeLeft < limit)))

#endif  /* _GOTIME_H_ */
