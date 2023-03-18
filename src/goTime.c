/*
 * src/gotime.c, part of Complete Goban (game program)
 * Copyright (C) 1995 William Shubert
 * See "configure.h.in" for more copyright information.
 */

#include <wms.h>
#include <wms/str.h>
#include "goTime.h"


int  goTime_parseChars(const char *t, bool ignoreExtra, bool *err)  {
  bool  dummy;
  int  numScans;
  int  v1, v2, v3;
  char  c1, c2, c3;

  if (err == NULL)
    err = &dummy;
  numScans = sscanf(t, "%d%c%d%c%d%c", &v1, &c1, &v2, &c2, &v3, &c3);
  if (ignoreExtra)  {
    if ((numScans > 1) && (c1 != ':'))  {
      numScans = 1;
    } else if ((numScans > 3) && (c2 != ':'))  {
      numScans = 3;
    } else if (numScans > 5)
      numScans = 5;
  }
  switch(numScans)  {
  case 1:
    /* Format "mm" */
    if (v1 >= 0)  {
      *err = FALSE;
      return(v1 * 60);
    }
    break;
  case 3:
    /* Format "mm:ss" */
    if ((c1 == ':') && (v1 >= 0) && (v2 >= 0) && (v2 < 60))  {
      *err = FALSE;
      return(v1 * 60 + v2);
    }
    break;
  case 5:
    /* Format "hh:mm:ss" */
    if ((c1 == ':') && (c2 == ':') && (v1 >= 0) && (v2 >= 0) && (v3 >= 0) &&
	(v2 < 60) && (v3 < 60))  {
      *err = FALSE;
      return(v1*60*60 + v2*60 + v3);
    }
    break;
  default:
    break;
  }
  *err = TRUE;
  return(0);
}


void  goTime_str(int time, Str *out)  {
  assert(time >= 0);
  if (time < 60*60)  {
    str_print(out, "%d:%02d", time/60, time%60);
  } else  {
    str_print(out, "%d:%02d:%02d", time/(60*60), (time/60)%60, time%60);
  }
}


void  goTime_describeStr(const GoTime *time, Str *out)  {
  Str  tmp;

  switch(time->type)  {
  case(goTime_none):
    str_copyChar(out, '-');
    break;
  case(goTime_absolute):
    goTime_str(time->main, out);
    break;
  case(goTime_canadian):
    str_init(&tmp);
    goTime_str(time->main, out);
    goTime_str(time->by, &tmp);
    str_catChar(out, '+');
    str_cat(out, &tmp);
    str_catChar(out, '/');
    str_catInt(out, time->aux);
    str_deinit(&tmp);
    break;
  case(goTime_japanese):
    str_init(&tmp);
    goTime_str(time->main, out);
    goTime_str(time->by, &tmp);
    str_catChar(out, '(');
    str_catInt(out, time->aux);
    str_catChar(out, 'x');
    str_cat(out, &tmp);
    str_catChar(out, ')');
    str_deinit(&tmp);
    break;
  case(goTime_ing):
    str_init(&tmp);
    goTime_str(time->main, out);
    goTime_str(time->by, &tmp);
    str_catChar(out, '+');
    str_cat(out, &tmp);
    str_catChar(out, 'x');
    str_catInt(out, time->aux);
    str_deinit(&tmp);
    break;
  default:
    assert(0);
    break;
  }
}


void  goTime_parseDescribeChars(GoTime *time, const char *desc)  {
  if (isdigit(desc[0]))  {
    time->main = goTime_parseChars(desc, TRUE, NULL);
    for (;;)  {
      switch(*(++desc))  {
      case '\0':
	time->type = goTime_absolute;
	return;
      case '+':
	++desc;
	time->by = goTime_parseChars(desc, TRUE, NULL);
	while (*desc && (*desc != '/') && (*desc != 'x'))
	  ++desc;
	switch(*desc)  {
	case '/':
	  time->type = goTime_canadian;
	  ++desc;
	  time->aux = wms_atoi(desc, NULL);
	  break;
	case 'x':
	  time->type = goTime_ing;
	  ++desc;
	  time->aux = wms_atoi(desc, NULL);
	  break;
	default:
	  time->type = goTime_none;
	  break;
	}
	return;
	break;
      case '(':
	time->type = goTime_japanese;
	++desc;
	/* Can't use wms_atoi here, it will report an error and return 0. */
	time->aux = atoi(desc);
	while (*desc && (*desc != 'x'))
	  ++desc;
	if (*desc == 'x')  {
	  time->by = goTime_parseChars(desc+1, TRUE, NULL);
	} else  {
	  time->type = goTime_none;
	}
	return;
	break;
      default:
	break;
      }
    }
  } else
    time->type = goTime_none;
}


void  goTime_remainStr(const GoTime *time, const GoTimer *timer, Str *out)  {
  int  tLeft;

  tLeft = timer->timeLeft;
  if (timer->usLeft)
    ++tLeft;
  if (tLeft < 0)
    tLeft = 0;
  switch(time->type)  {
  case(goTime_none):
    str_copyChar(out, '-');
    break;
  case(goTime_absolute):
    goTime_str(tLeft, out);
    break;
  case(goTime_canadian):
    goTime_str(tLeft, out);
    str_catChar(out, '/');
    if (timer->aux == 0)  {
      str_catChar(out, '-');
    } else  {
      str_catInt(out, timer->aux);
    }
    break;
  case(goTime_japanese):
    goTime_str(tLeft, out);
    if (tLeft <= time->aux * time->by)  {
      str_catChar(out, '(');
      if (tLeft < 0)
	str_catChar(out, '0');
      else
	str_catInt(out, (tLeft + time->by - 1) / time->by);
      str_catChar(out, ')');
    }
    break;
  case(goTime_ing):
    goTime_str(tLeft, out);
    if (timer->aux > 0)  {
      str_catChar(out, '+');
      str_catInt(out, timer->aux);
    }
    break;
  default:
    assert(0);
    break;
  }
}


void  goTimer_init(GoTimer *timer, const GoTime *time)  {
  timer->timeLeft = time->main;
  if (time->type == goTime_ing)
    timer->aux = time->aux;
  else
    timer->aux = 0;
  timer->usLeft = 0;
}


void  goTime_startTimer(const GoTime *time, GoTimer *timer)  {
  struct timezone  tzone;

  gettimeofday(&timer->startTime, &tzone);
}


/*
 * goTime_checkTimer() returns FALSE if you are out of time.
 * Call it whenever you want the clock to be updated.  Every second is nice.
 */
bool  goTime_checkTimer(const GoTime *time, GoTimer *timer)  {
  struct timeval now;
  struct timezone  tzone;

  gettimeofday(&now, &tzone);
  timer->usLeft -= (now.tv_usec - timer->startTime.tv_usec);
  while (timer->usLeft <= 0)  {
    timer->usLeft += 1000000;
    --timer->timeLeft;
  }
  timer->timeLeft -= (now.tv_sec - timer->startTime.tv_sec);
  timer->startTime = now;
  if (timer->timeLeft < 0)  {
    switch(time->type)  {
    case goTime_none:
      timer->timeLeft = 1;
      break;
    case goTime_absolute:
    case goTime_japanese:
      timer->timeLeft = -1;
      timer->usLeft = 1000000;
      break;
    case goTime_canadian:
      if (timer->aux == 0)  {
	timer->aux = time->aux;
	timer->timeLeft += time->by;
	if (timer->timeLeft < 0)  {
	  timer->timeLeft = -1;
	  timer->usLeft = 1000000;
	}
      } else  {
	timer->timeLeft = -1;
	timer->usLeft = 1000000;
      }
      break;
    case goTime_ing:
      while ((timer->timeLeft < 0) && (timer->aux > 0))  {
	timer->timeLeft += time->by;
	--timer->aux;
      }
      if ((timer->timeLeft < 0) && (timer->aux == 0))  {
	timer->timeLeft = -1;
	timer->usLeft = 1000000;
      }
      break;
    default:
      assert(0);
      break;
    }
  }
  return(timer->timeLeft >= 0);
}


bool  goTime_endTimer(const GoTime *time, GoTimer *timer)  {
  bool  result;
  int  left;

  result = goTime_checkTimer(time, timer);
  if (time->type == goTime_japanese)  {
    left = timer->timeLeft;
    if (timer->usLeft)
      ++left;
    if ((left > 0) && (left <= time->aux * time->by))  {
      left = (left + time->by - 1);
      timer->timeLeft = left - (left % time->by);
      timer->usLeft = 0;
    }
  } else if (time->type == goTime_canadian)  {
    if (timer->aux)  {
      --timer->aux;
      if (timer->aux == 0)  {
	timer->aux = time->aux;
	timer->timeLeft = time->by;
	timer->usLeft = 0;
      }
    }
  }
  return(result);
}


int  goTime_ingPenalty(const GoTime *time, const GoTimer *timer)  {
  if (time->type == goTime_ing)
    return(2 * (time->aux - timer->aux));
  else
    return(0);
}
