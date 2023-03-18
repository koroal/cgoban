/*
 * wmslib/src/wms/snd.c, part of wmslib (Library functions)
 * Copyright (C) 1995 William Shubert.
 * See "configure.h.in" for more copyright information.
 */

#include <wms.h>
#include <wms/snd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#if  SUN_SOUND
#include <sun/audioio.h>
#elif  LINUX_SOUND
#if  !HAVE_GETDTABLESIZE
#include <sys/resource.h>
#endif
#include <linux/soundcard.h>
#include <sys/wait.h>
#endif

#if  SUN_SOUND
#define  sound_dev_name  "/dev/audio"
#elif  LINUX_SOUND
#define  sound_dev_name  "/dev/dsp"
#endif

#if  SUN_SOUND
#define  SND_PHYSMAXVOL  255
#elif  LINUX_SOUND
#define  SND_PHYSMAXVOL  261
#endif

#ifndef  SUN_SOUND
#define  SUN_SOUND  0
#endif

#ifndef  LINUX_SOUND
#define  LINUX_SOUND  0
#endif

const bool  sndAvail = (SUN_SOUND || LINUX_SOUND);

static int  sound_dev = -1, volume = 0;
static SndState  state = sndState_off;

#if  SUN_SOUND

static audio_info_t  old_ai;

#elif  LINUX_SOUND

static Snd  *cvtlist = NULL;
static pid_t  child;

static int  ulaw_to_16bit(uchar ulawbyte);
static void  audio_child(int in, int out);

#endif


void  (*snd_errHandler)(SndInit errType, char *errStr) = NULL;
char  snd_error[1000] = "Sound functioning properly.";


static void  snd_deinitChild(void);


#if  SUN_SOUND || LINUX_SOUND
SndInit  snd_init(SndState newState, int newVolume)  {
#if  SUN_SOUND
  audio_info_t  ai;
#endif  /* SUN_SOUND */
  SndInit  result = sndInit_ok;
  
  assert((newVolume >= -1) && (newVolume <= SND_MAXVOL));
  if (newVolume >= 0)
    newVolume = ((newVolume * SND_PHYSMAXVOL + SND_MAXVOL/2) / SND_MAXVOL);
  if (state == sndState_iOff)
    return(sndInit_broken);
  if (state == sndState_iWantOpen)  {
    if (newState == sndState_oldState)
      newState = sndState_fullOpen;
    state = sndState_off;
  }
  if (newVolume == -1)
    newVolume = volume;
  if (volume != newVolume)  {
    if (volume == 0)  {
      if (newState == sndState_oldState)
	newState = state;
      state = sndState_off;
    }
#if  LINUX_SOUND
    /* Free up all old converted sound buffers. */
    while (cvtlist != NULL)  {
      cvtlist->converted = FALSE;
      wms_free(cvtlist->cvtdata.dsp);
      cvtlist = cvtlist->next;
    }
#endif  /* LINUX_SOUND */
    volume = newVolume;
  }
  if (volume == 0)
    snd_deinit();
  if ((newState != sndState_oldState) && (newState != state))  {
    snd_deinit();
    state = newState;
    if ((volume == 0) || (state < sndState_fullOpen))  {
      /*
       * Try to open it, just to make sure that we can.
       */
      sound_dev = open(sound_dev_name, O_WRONLY|O_NDELAY);
      if (sound_dev != -1)  {
	close(sound_dev);
	sound_dev = -1;
	return(result);
      }
    } else
      sound_dev = open(sound_dev_name, O_WRONLY|O_NDELAY);
    if (sound_dev == -1)  {  /* Can't open the sound device.  Drag. */
      if (errno == EBUSY)  {
	result = sndInit_busy;
	if (state == sndState_fullOpen)
	  state = sndState_iWantOpen;
	/* Somebody else has it open!  Print an error and try again. */
	sprintf(snd_error, "Sound device \"%s\" is busy.", sound_dev_name);
      } else  {
	/* Some other reason...give up forever. */
	result = sndInit_broken;
	volume = 0;
	state = sndState_iOff;
	sprintf(snd_error, "Couldn't open sound device \"%s\".  "
		"Error code: \"%s\".", sound_dev_name, strerror(errno));
      }
      return(result);
    }
    if (state == sndState_fullOpen)  {
#if  LINUX_SOUND
      /* Sound device opened successfully.  Spawn the audio child. */
      int  pipes[2], i;

      pipe(pipes);
      child = fork();
      if (child == 0)  {
	int  maxFiles = getdtablesize();

	/*
	 * Close off all file descriptors that the audio child doesn't need.
	 */
	for (i = 2;  i < maxFiles;  ++i)  {
	  if ((i != pipes[0]) && (i != sound_dev))
	    close(i);
	}
	audio_child(pipes[0], sound_dev);
      } else  {
	close(pipes[0]);
	close(sound_dev);
	sound_dev = pipes[1];
      }
#endif  /* LINUX_SOUND */
    }
  }
#if  SUN_SOUND
  /* Cofigure the device for us. */
  ioctl(sound_dev, AUDIO_GETINFO, &ai);
  old_ai = ai;
  AUDIO_INITINFO(&ai);
  ai.play.sample_rate = 8000;
  ai.play.channels = 1;
  ai.play.precision = 8;
  ai.play.encoding = AUDIO_ENCODING_ULAW;
  if (volume != -1)
    ai.play.gain = (volume * AUDIO_MAX_GAIN + SND_MAXVOL/2) / SND_MAXVOL;
  if (ioctl(sound_dev, AUDIO_SETINFO, &ai) == -1)  {
    sprintf(snd_error, "Sound device \"%s\" cannot use desired "
	    "data format.", sound_dev_name);
    result = sndInit_broken;
    volume = 0;
    close(sound_dev);
    state = sndState_iOff;
  }
#endif  /* SUN_SOUND */
  return(result);
}


void  snd_play(Snd *sound)  {
  if ((volume == 0) || (state <= sndState_off))
    return;
#if  LINUX_SOUND
  if (!sound->converted)  {
    int  i;

    sound->cvtdata.dsp = wms_malloc(sound->len);
    sound->cvtlen = sound->len;
    for (i = 0;  i < sound->len;  ++i)
      sound->cvtdata.dsp[i] = ((ulaw_to_16bit(sound->data[i]) * volume) >> 16)
	+ 128;
    sound->converted = TRUE;
    sound->next = cvtlist;
    cvtlist = sound;
  }
#endif  /* LINUX_SOUND */
  if (state == sndState_partOpen)
    snd_init(sndState_iTempOpen, -1);
  if (sound_dev != -1)
    write(sound_dev,
#if  SUN_SOUND
	  sound->data, sound->len
#elif  LINUX_SOUND
	  sound->cvtdata.dsp, sound->cvtlen
#endif
	  );
  if (state == sndState_iTempOpen)  {
#if  LINUX_SOUND
    if (sound_dev != -1)
      ioctl(sound_dev, SNDCTL_DSP_SYNC, NULL);
#endif
    snd_deinitChild();
    state = sndState_partOpen;
  }
}


void  snd_deinit(void)  {
#if  LINUX_SOUND
  int  status;
#endif  /* LINUX_SOUND */

  if ((sound_dev != -1) && (state == sndState_fullOpen))  {
#if  SUN_SOUND
    ioctl(sound_dev, AUDIO_SETINFO, &old_ai);
#endif  /* SUN_SOUND */
    close(sound_dev);
#if  LINUX_SOUND
    kill(child, SIGKILL);
    wait(&status);
#endif  /* LINUX_SOUND */
  }
  sound_dev = -1;
}


static void  snd_deinitChild(void)  {
  if (sound_dev != -1)  {
#if  SUN_SOUND
    ioctl(sound_dev, AUDIO_SETINFO, &old_ai);
#endif  /* SUN_SOUND */
    close(sound_dev);
  }
  sound_dev = -1;
}


#if  LINUX_SOUND


static void  audio_child(int in, int out)  {
  size_t  size_in;
  char  buf[1024*8];  /* 1 second...should be enough! */

  /* Make sure that we don't save OUR state! */
  signal(SIGTERM, SIG_DFL);
  signal(SIGINT, SIG_DFL);
  signal(SIGPIPE, SIG_DFL);
  signal(SIGHUP, SIG_DFL);
  for (;;)  {
    size_in = read(in, buf, 1024*8);
    if (size_in == 0)
      exit(0);
    if (size_in == -1)  {
      perror("Pente sound demon: Error ");
      exit(1);
    }
    write(out, buf, size_in);
    ioctl(out, SNDCTL_DSP_SYNC, NULL);
  }
}


static int  ulaw_to_16bit(uchar ulawbyte)  {
  static int exp_lut[8] = { 0, 132, 396, 924, 1980, 4092, 8316, 16764 };
  int sign, exponent, mantissa, sample;

  ulawbyte = ~ ulawbyte;
  sign = ( ulawbyte & 0x80 );
  exponent = ( ulawbyte >> 4 ) & 0x07;
  mantissa = ulawbyte & 0x0F;
  sample = exp_lut[exponent] + ( mantissa << ( exponent + 3 ) );
  if ( sign != 0 ) sample = -sample;
  
  return sample;
}


#endif  /* LINUX_SOUND */
#else  /* No audio. */


SndInit  snd_init(SndState newstate, int vol)  {
  sprintf(snd_error, "%s was compiled with sound turned off.",
	  wms_progname);
  return(sndInit_broken);
}
void  snd_play(Snd *sound)  {}
void  snd_deinit(void)  {}

#endif  /* SUN_SOUND || LINUX_SOUND */
