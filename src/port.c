/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

/*
 * This file defines things that may ought to already exist on the
 * target system, but are missing in some ports. There are #ifdef's
 * that will hopefully take care of everything.
 */

#define PORT_DO_WARN

#include "global.h"
#include "time_stuff.h"
#include "pike_error.h"
#include "pike_macros.h"

#ifdef __MINGW32__
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#endif

#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <time.h>

#ifdef sun
time_t time (time_t *);
#endif

#if (SIZEOF_LONG == 4) && defined(_LP64)
/* Kludge for gcc and the system header files not using the same model... */
#undef LONG_MIN
#undef LONG_MAX
#undef ULONG_MAX
#define LONG_MIN	INT_MIN
#define LONG_MAX	INT_MAX
#define ULONG_MAX	UINT_MAX
#endif

#ifndef HAVE_GETTIMEOFDAY

#ifdef HAVE_GETSYSTEMTIMEASFILETIME
#include <winbase.h>

PMOD_EXPORT void GETTIMEOFDAY(struct timeval *t)
{
  union {
    unsigned __int64 ft_scalar;
    FILETIME ft_struct;
  } ft;

  GetSystemTimeAsFileTime(&ft.ft_struct);

  ft.ft_scalar /= 10;
#ifndef __GNUC__
  ft.ft_scalar -= 11644473600000000i64;
  t->tv_sec = (long)(ft.ft_scalar / 1000000i64);
  t->tv_usec = (long)(ft.ft_scalar % 1000000i64);
#else
  ft.ft_scalar -= 11644473600000000;
  t->tv_sec = (long)(ft.ft_scalar / 1000000);
  t->tv_usec = (long)(ft.ft_scalar % 1000000);
#endif
}

#else
PMOD_EXPORT void GETTIMEOFDAY(struct timeval *t)
{
  t->tv_sec=(long)time(0);
  t->tv_usec=0;
}
#endif
#endif

PMOD_EXPORT void sysleep(double left)
{
#ifdef __NT__
  Sleep((int)(left*1000+0.5));
#elif defined(HAVE_NANOSLEEP)
  {
    struct timespec req;
    left+=5e-10;
    req.tv_nsec=(left - (req.tv_sec=left))*1e9;
    nanosleep(&req,(void*)0);
  }
#elif defined(HAVE_POLL)
  {
    /* MacOS X is stupid, and requires a non-NULL pollfd pointer. */
    struct pollfd sentinel;
    poll(&sentinel, 0, (int)(left*1000+0.5));
  }
#else
  {
    struct timeval t3;
    left+=5e-7;
    t3.tv_usec=(left - (t3.tv_sec=left))*1e6;
    select(0,0,0,0,&t3);
  }
#endif
}

#ifndef CONFIGURE_TEST

#ifndef HAVE_WORKING_REALLOC_NULL
PMOD_EXPORT /*@null@*/ void *pike_realloc(void *ptr, size_t sz)
{
  if (!ptr) return malloc(sz);
#undef realloc
  return realloc(ptr, sz);
#define realloc(PTR, SZ)	pike_realloc((PTR),(SZ))
}
#endif

#endif	/* !CONFIGURE_TEST */

#ifndef HAVE_STRCASECMP
PMOD_EXPORT int STRCASECMP(const char *a,const char *b)
{
  int ac, bc;

  while(1)
  {
    ac=*(a++);
    bc=*(b++);

    if(ac && isupper(ac)) ac=tolower(ac);
    if(bc && isupper(bc)) bc=tolower(bc);
    if(ac - bc) return ac-bc;
    if(!ac) return 0;
  }
}
#endif

#ifdef HAVE_BROKEN_CHKSTK
/* Intels ecl compiler adds calls to _stkchk() in the prologue,
 * MicroSoft seems to have renamed it to __stkchk() in later
 * versions of their SDK. This kludge attempts to workaround
 * that compiler bug.
 *	/grubba 2001-02-17.
 */
extern size_t __chkstk();
size_t _chkstk() { return __chkstk(); }
#endif /* HAVE_BROKEN_CHKSTK */

#ifdef OWN_GETHRTIME

#ifdef OWN_GETHRTIME_RDTSC

/* Estimated errors in max standard deviation in seconds between the
 * realtime and rtsc clocks. */

#define GETTIMEOFDAY_ERR 1e-3
/* Conservative value for gettimeofday resolution: It's taken from a
 * Linux with SMP kernel where it's reliable only down to msec. */

#define CPUINFO_ERR 1e-4
/* Only count on four accurate digits in the MHz figure in
 * /proc/cpuinfo. */

#define SATISFACTORY_ERR 1e-6
/* This is a compromise between final accurancy and the risk that the
 * system clock changes during calibration so that the zero time gets
 * wrong. (It's still better than the typical gethrtime accurancy on
 * Solaris.) */

#define COUNT_THRESHOLD 10
/* Number of measurements before we can trust the variance
 * calculation. */

static long long hrtime_rtsc_zero;
static long long hrtime_rtsc_last;
#ifdef PIKE_DEBUG
static long long hrtime_max = 0;
#endif
static struct timeval hrtime_timeval_zero;
static long long hrtime_rtsc_base, hrtime_nsec_base = 0;
static long double hrtime_conv=0.0;
static double hrtime_conv_var = GETTIMEOFDAY_ERR * GETTIMEOFDAY_ERR;
static int hrtime_is_calibrated = 0;

/*  #define RTSC_DEBUG 1 */
/*  #define RTSC_DEBUG_MORE 1 */

#define RTSC(l,h)							\
   __asm__ __volatile__ (  "rdtsc"					\
			   :"=a" (l),					\
			   "=d" (h))

static inline long long rtsc()
{
   long long now;
   unsigned long nl,nh;
   RTSC(nl,nh);
   return (((long long)nh)<<32)|nl;
}

void own_gethrtime_init()
{
   int fd;

   ACCURATE_GETTIMEOFDAY(&hrtime_timeval_zero);
   hrtime_rtsc_zero=rtsc();
   hrtime_rtsc_last = hrtime_rtsc_base = hrtime_rtsc_zero;
#ifdef RTSC_DEBUG
   fprintf(stderr,"init: %lld\n",hrtime_rtsc_zero);
#endif

   do {
     fd = fd_open("/proc/cpuinfo", fd_RDONLY, 0);
   } while (fd < 0 && errno == EINTR);
   if (fd >= 0) {
     char buf[10240];
     ptrdiff_t len;

     set_nonblocking(fd, 1);	/* Paranoia. */
     do {
       len = fd_read(fd, buf, sizeof(buf) - 1);
     } while (len < 0 && errno == EINTR);
     fd_close(fd);

     if (len > 0) {
       char *p;
       buf[len] = 0;
       p = STRSTR(buf, "\ncpu MHz");
       if (p) {
	 p += sizeof("\ncpu MHz");
	 p += strcspn(p, "0123456789\n");
	 if (*p != '\n') {
	   long long hz = 0;

	   for (; *p >= '0' && *p <= '9'; p++)
	     hz = hz * 10 + (*p - '0') * 1000000;
	   if (*p == '.') {
	     int mult = 100000;
	     for (p++; mult && *p >= '0' && *p <= '9'; p++, mult /= 10)
	       hz += (*p - '0') * mult;

	     hrtime_conv = 1e9 / (long double) hz;
	     hrtime_conv_var = CPUINFO_ERR * CPUINFO_ERR;
#ifdef RTSC_DEBUG
	     fprintf(stderr, "cpu Mhz read from /proc/cpuinfo: %lld.%lld, "
		     "conv=%#.10Lg, est err=%#.3g\n",
		     hz / 1000000, hz % 1000000, hrtime_conv, sqrt(hrtime_conv_var));
#endif

	     if (hrtime_conv_var < GETTIMEOFDAY_ERR * GETTIMEOFDAY_ERR) {
#ifdef RTSC_DEBUG
	       fprintf(stderr, "using rtsc from the start\n");
#endif
	       hrtime_is_calibrated = 1;
	     }
	   }
	 }
       }
     }
   }
}

void own_gethrtime_update(struct timeval *ptr)
{
   long long td,t;
   long long now;
   long double conv;
   double var;
   static long long td_last = 0;
   static long long w_sum = 0.0, w_c_sum = 0.0;
   static double w_c_c_sum = 0.0;
   static int count = -COUNT_THRESHOLD;

   ACCURATE_GETTIMEOFDAY(ptr);
   now=rtsc();

#ifdef RTSC_DEBUG
   fprintf(stderr,"update: rtsc now: %lld\n",now);
#endif

   if (hrtime_is_calibrated == 2) return; /* Calibration done. */

   td=((long long)ptr->tv_sec-hrtime_timeval_zero.tv_sec)*1000000000+
      ((long long)ptr->tv_usec-hrtime_timeval_zero.tv_usec)*1000;

   if (!hrtime_is_calibrated) {
     /* We're in a hurry to make the rtsc kick in - require only
      * hundreds of seconds between measurements. */
     if (td - td_last < 10000000) return;
     /* Drop the count threshold if a second has gone by. That ought
      * to be long enough to get a reasonable reading. */
     if (td - td_last > 1000000000) count = 0;
   }
   else
     /* Doing fine calibration - require an order of whole seconds
      * between measurements. */
     if (now - hrtime_rtsc_last < 1000000000) return;

   t=now-hrtime_rtsc_zero;
   if (t <= 0 || td <= 0)
     /* Ouch, someone must have backed the clock(s). This situation
      * could be handled better. */
     return;
   td_last = td;
   hrtime_rtsc_last=now;
   conv=((long double)td)/t;

/* fixme: add time deviation detection;
   when time is adjusted, this calculation isn't linear anymore,
   so it might be best to reset it */

   /* Calculate the variance. Each conversion factor is weighted by
    * the time it was measured over, since later factors can be
    * considered more exact due to the elapsed time. */
   w_sum += t;
   w_c_sum += td;		/* = t * conv */
   w_c_c_sum += td * conv;	/* = t * conv * conv */

   if (++count >= 0) {
     double var = (w_c_c_sum - (double) w_c_sum * w_c_sum / w_sum) / w_sum;

     if (var > 0.0 && var < hrtime_conv_var) {
       hrtime_nsec_base +=
	 (long long) ( (long double)(now-hrtime_rtsc_base) * hrtime_conv );
       hrtime_rtsc_base = now;
       hrtime_conv_var = var;
       hrtime_conv = (long double) w_c_sum / w_sum;

#ifdef RTSC_DEBUG
       fprintf(stderr, "[%d, %#5.4g] "
	       "last=%#.10Lg, conv=%#.10Lg +/- %#.3g (%#.10Lg MHz)\n",
	       count, td / 1e9,
	       conv, hrtime_conv, sqrt(hrtime_conv_var), 1000.0/hrtime_conv);
#endif

       if (!hrtime_is_calibrated) {
	 if (hrtime_conv_var < GETTIMEOFDAY_ERR * GETTIMEOFDAY_ERR) {
#ifdef RTSC_DEBUG
	   fprintf(stderr, "switching to rtsc after %g sec\n", td / 1e9);
#endif
	   hrtime_is_calibrated = 1;
	   /* Reinitialize the statistics since it's hard to weight
	    * away the initial burst of noisy measurements we might
	    * have done to make rtsc good enough to use. */
	   count = -COUNT_THRESHOLD;
	   w_c_sum = w_sum = 0;
	   w_c_c_sum = 0.0;
	   /* The variance might be too low if we dropped the count
	    * threshold above. */
	   hrtime_conv_var = GETTIMEOFDAY_ERR * GETTIMEOFDAY_ERR;
	 }
       }
       else
	 if (hrtime_conv_var < SATISFACTORY_ERR * SATISFACTORY_ERR) {
#ifdef RTSC_DEBUG
	   fprintf(stderr, "rtsc calibration finished, took %g sec\n", td / 1e9);
#endif
	   hrtime_is_calibrated = 2;
	 }
     }
     else {
#ifdef RTSC_DEBUG
       fprintf(stderr, "[%d, %#5.4g] last=%#.10Lg +/- %#.3g, conv=%#.10Lg\n",
	       count, td / 1e9, conv, var > 0.0 ? sqrt(var) : -1.0, hrtime_conv);
#endif
     }
   }
   else {
#ifdef RTSC_DEBUG
     fprintf(stderr, "[%d, %#5.4g] last=%#.10Lg\n", count, td / 1e9, conv);
#endif
   }
}

long long gethrtime()
{
   long long now;
   struct timeval tv;

   if (!hrtime_is_calibrated) {
     own_gethrtime_update(&tv);
     if (hrtime_is_calibrated) goto use_rtsc;
     hrtime_rtsc_base=rtsc();
     now=hrtime_nsec_base =
       ((long long)tv.tv_sec-hrtime_timeval_zero.tv_sec)*1000000000+
       ((long long)tv.tv_usec-hrtime_timeval_zero.tv_usec)*1000;
   }
   else {
     now=rtsc();
     if ((hrtime_is_calibrated == 1) &
	 (now - hrtime_rtsc_last > 1000000000)) { /* Some seconds between updates. */
       own_gethrtime_update(&tv);
     use_rtsc:
       now=rtsc();
     }
#ifdef RTSC_DEBUG_MORE
     fprintf(stderr,"rtime: rtsc now: %lld\n",now);
#endif
     now = (long long) ( (long double)(now-hrtime_rtsc_base) * hrtime_conv ) +
       hrtime_nsec_base;
   }

#ifdef RTSC_DEBUG_MORE
   fprintf(stderr,"(%lld)\n",now);
#endif

#ifdef PIKE_DEBUG
   if (now<hrtime_max)
     Pike_fatal("Time calculation went backwards with %lld.\n", hrtime_max - now);
   hrtime_max = now;
#endif
   return now;
}

#endif	/* OWN_GETHRTIME_RDTSC */

#endif	/* OWN_GETHRTIME */

#if !defined(HAVE_STRDUP) && !defined(HAVE__STRDUP)
char *strdup(const char *str)
{
  char *res = NULL;
  if (str) {
    int len = strlen(str)+1;

    res = xalloc(len);
    memcpy(res, str, len);
  }
  return(res);
}
#endif /* !HAVE_STRDUP */
