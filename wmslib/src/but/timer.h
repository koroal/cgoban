/*
 * wmslib/src/but/timer.h, part of wmslib (Library functions)
 * Copyright (C) 1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 *
 * Includes for timer.c
 */

#ifndef  _BUT_TIMER_H_
#define  _BUT_TIMER_H_  1

/**********************************************************************
 * Constants
 **********************************************************************/
#define  BUTTIMER_HIST  4
#define  BUTTIMER_MAXFREQ  400  /* Hz...the most we'll ever allow. */
#define  BUTTIMER_STDFREQ  100  /* Hz...the "standard" value. */

/**********************************************************************
 * Data types
 **********************************************************************/
typedef enum  {
  butTimer_on, butTimer_off, butTimer_dead
} ButTimerState;

/* Private. */
struct  ButTimer_struct  {
  void  *packet;
  int  eventNum;
  struct timeval  nextFiring, period;
  bool  winOnly;  /* False=shut off when iconified. */
  ButTimerState  state;
  struct ButWin_struct  *win;
  But  *but;
  ButOut  (*timerFunc)(struct ButTimer_struct *timer);
  struct ButTimer_struct  *next;
  int  ticksPerPeriod;
  bool  freqCounter;
  int  lastRes[BUTTIMER_HIST];
  int  resnum, ticksLeft;
  MAGIC_STRUCT
};

/**********************************************************************
 * Functions
 **********************************************************************/
/* Public. */
extern ButTimer  *butTimer_create(void *packet, But *but,
				  struct timeval delay,
				  struct timeval period,
				  bool  win_only,
				  ButOut (*timerfunc)(struct ButTimer_struct
						      *timer));
extern ButTimer  *butTimer_fCreate(void *packet, But *but,
				   struct timeval delay, int frequency,
				   bool win_only,
				   ButOut (*timerfunc)(struct ButTimer_struct
						       *timer));
extern void  butTimer_destroy(ButTimer *timer);
extern void  butTimer_reset(ButTimer *timer);
#define  butTimer_ticks(bt)        ((bt)->eventNum)
#define  butTimer_setTicks(bt, v)  ((bt)->eventNum = (v))
#define  butTimer_packet(bt)  ((bt)->packet)

/* Private. */
extern struct timeval  but_timerAdd(struct timeval t1, struct timeval t2);
extern struct timeval  but_timerSub(struct timeval t1, struct timeval t2);
extern int  but_timerDiv(struct timeval t1, struct timeval t2,
			 struct timeval *remainder);
extern int  butEnv_checkTimers(ButEnv *env, struct timeval *timeout);

#endif  /* _BUT_TIMER_H_ */
