/*
 * wmslib/src/but/timer.c, part of wmslib (Library functions)
 * Copyright (C) 1994 William Shubert.
 * See "configure.h.in" for more copyright information.
 */

#include <configure.h>

#ifdef  X11_DISP

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <sys/time.h>
#include <wms.h>
#include <but/but.h>
#include <but/timer.h>


ButTimer  *butTimer_create(void *packet, But *but, struct timeval delay,
			   struct timeval period, bool winOnly,
			   ButOut (*timerFunc)(ButTimer *timer))  {
  ButTimer *bt;
  struct timeval  now;
  struct timezone  tzone;

  bt = wms_malloc(sizeof(ButTimer));
  MAGIC_SET(bt);
  bt->packet = packet;
  bt->eventNum = 0;
  bt->period = period;
  bt->state = butTimer_on;
  gettimeofday(&now, &tzone);
  bt->nextFiring = but_timerAdd(now, delay);
  if (but)
    bt->win = but->win;
  else
    bt->win = NULL;
  bt->but = but;
  bt->timerFunc = timerFunc;
  bt->winOnly = winOnly;
  bt->next = but_timerList;
  bt->freqCounter = FALSE;
  bt->ticksLeft = 0;
  but_timerList = bt;
  return(bt);
}


ButTimer  *butTimer_fCreate(void *packet, But *but, struct timeval delay,
			    int frequency, bool winOnly,
			    ButOut (*timerFunc)(ButTimer *timer))  {
  ButTimer  *bt;
  struct timeval  now;
  struct timezone  tzone;
  int  i;

  bt = wms_malloc(sizeof(ButTimer));
  MAGIC_SET(bt);
  bt->packet = packet;
  bt->eventNum = 0;
  bt->period.tv_sec = 0;
  bt->winOnly = winOnly;
  if (frequency < BUTTIMER_MAXFREQ)  {
    bt->period.tv_usec = 1000000 / frequency;
    bt->ticksPerPeriod = 1;
  } else  {
    bt->period.tv_usec = 1000000 / BUTTIMER_STDFREQ;
    bt->ticksPerPeriod = (frequency + BUTTIMER_STDFREQ/2) /
      BUTTIMER_STDFREQ;
  }
  bt->state = butTimer_on;
  gettimeofday(&now, &tzone);
  bt->nextFiring = but_timerAdd(now, delay);
  if (but)
    bt->win = but->win;
  else
    bt->win = NULL;
  bt->but = but;
  bt->timerFunc = timerFunc;
  bt->next = but_timerList;
  bt->freqCounter = TRUE;
  for (i = 0;  i < BUTTIMER_HIST;  ++i)
    bt->lastRes[i] = 1;
  bt->resnum = 0;
  bt->ticksLeft = 0;
  but_timerList = bt;
  return(bt);
}


void  butTimer_destroy(ButTimer *timer)  {
  assert(MAGIC(timer));
  timer->state = butTimer_dead;
}


void  butTimer_reset(ButTimer *timer)  {
  struct timeval  now;
  struct timezone  tzone;

  assert(MAGIC(timer));
  timer->eventNum = 0;
  gettimeofday(&now, &tzone);
  timer->nextFiring = but_timerAdd(now, timer->period);
}


struct timeval  but_timerAdd(struct timeval t1, struct timeval t2)  {
  struct timeval  r;

  r.tv_sec = t1.tv_sec + t2.tv_sec;
  if ((r.tv_usec = t1.tv_usec + t2.tv_usec) > 1000000)  {
    r.tv_usec -= 1000000;
    ++r.tv_sec;
  }
  return(r);
}


struct timeval  but_timerSub(struct timeval t1, struct timeval t2)  {
  struct timeval r;

  r.tv_sec = t1.tv_sec - t2.tv_sec;
  if ((r.tv_usec = t1.tv_usec - t2.tv_usec) < 0)  {
    r.tv_usec += 1000000;
    --r.tv_sec;
  }
  if (r.tv_sec < 0)
    r.tv_sec += 60*60*24;
  return(r);
}


int  but_timerDiv(struct timeval t1, struct timeval t2,
		     struct timeval *remainder)  {
  int  result;
  long it1, it2, irem;

  it1 = (t1.tv_sec*1000000L) + t1.tv_usec;
  it2 = (t2.tv_sec*1000000L) + t2.tv_usec;
  result = it1 / it2;
  if (remainder != NULL)  {
    irem = it1 % it2;
    remainder->tv_usec = irem % 1000000;
    remainder->tv_sec = irem / 1000000;
  }
  return(result);
}


int  butEnv_checkTimers(ButEnv *env, struct timeval *timeout)  {
  struct timeval  current_time, next_timer, timerem;
  struct timezone  tzone;
  bool  take_timer, timer_set = FALSE;
  ButTimer  *bt, *bt2;
  int  ticks, i, orig_ticks;
  int  result = 0;

  /* First, kill off all the dead timers. */
  for (bt = but_timerList;  bt != NULL;)  {
    if (bt->state == butTimer_dead)  {
      if (bt == but_timerList)
	but_timerList = bt->next;
      else  {
	for (bt2 = but_timerList;  bt2->next != bt;  bt2 = bt2->next);
	bt2->next = bt->next;
      }
      bt2 = bt->next;
      MAGIC_UNSET(bt);
      wms_free(bt);
      bt = bt2;
    } else  {
      bt = bt->next;
    }
  }
  gettimeofday(&current_time, &tzone);
  for (bt = but_timerList;  bt != NULL;  bt = bt->next)  {
    if (bt->state == butTimer_on)  {
      if (timercmp(&bt->nextFiring, &current_time, <))  {
	take_timer = TRUE;
	ticks = but_timerDiv(but_timerSub(current_time, bt->nextFiring),
			     bt->period, &timerem) + 1 + bt->ticksLeft;
	take_timer = TRUE;
	orig_ticks = ticks;
	if (bt->freqCounter)  {
	  for (i = 0;  i < BUTTIMER_HIST;  ++i)  {
	    if (bt->lastRes[i] > ticks)  {
	      take_timer = FALSE;
	    }
	  }
	  if (!bt->ticksLeft)  {
	    bt->lastRes[bt->resnum] = ticks;
	    bt->resnum = (bt->resnum + 1) & (BUTTIMER_HIST - 1);
	  }
	  ticks *= bt->ticksPerPeriod;
	}
	bt->nextFiring = but_timerSub(but_timerAdd(current_time, bt->period),
				       timerem);
	if (take_timer)  {
	  bt->eventNum += ticks;
	  result |= bt->timerFunc(bt);
	  XSync(env->dpy, False);
	  bt->ticksLeft = 0;
	} else
	  bt->ticksLeft += orig_ticks;
      }
      if (!timer_set || timercmp(&bt->nextFiring, &next_timer, <))  {
	timer_set = TRUE;
	next_timer = bt->nextFiring;
      }
    }
  }
  if (timer_set)
    *timeout = but_timerSub(next_timer, current_time);
  else  {
    timeout->tv_usec = 0;
    timeout->tv_sec = 365*24*60*60;  /* 1 spurious interrupt per year. */
  }
  return(result);
}

#endif  /* X11_DISP */
