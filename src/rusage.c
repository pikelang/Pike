/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "pike_macros.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "pike_rusage.h"
#include "pike_threadlib.h"

#ifdef HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#ifdef HAVE_MACH_MACH_H
#include <mach/mach.h>
#ifdef HAVE_PTHREAD_MACH_THREAD_NP
#include <pthread.h>
#endif
#endif

#ifdef HAVE_MACH_THREAD_ACT_H
#include <mach/thread_act.h>
#endif

#ifdef HAVE_MACH_CLOCK_H
#include <mach/clock.h>
#endif

#ifndef CONFIGURE_TEST
#include "time_stuff.h"
#include "fd_control.h"
#endif

#include "pike_error.h"

#ifdef __amigaos__
#undef HAVE_TIMES
#endif

/*
 * Here comes a long blob with stuff to see how to find out about
 * cpu usage.
 */

#ifdef HAVE_TIMES
long pike_clk_tck = 0;
#endif

#ifdef __NT__
LARGE_INTEGER perf_freq;
#endif

/*
 * Here's a trick to do TIME * BASE / TICKS without
 * causing arithmetic overflow
 */
#define CONVERT_TIME(TIME, TICKS, BASE) \
  (((TIME) / (TICKS)) * (BASE) + ((TIME) % (TICKS)) * (BASE) / (TICKS))

#if CPU_TIME_TICKS_LOW > 1000000000L
# if CPU_TIME_TICKS_LOW % 1000000000L == 0
#  define NSEC_TO_CPU_TIME_T(TIME) ((TIME) * (CPU_TIME_TICKS / 1000000000L))
# endif
#elif CPU_TIME_TICKS_LOW < 1000000000L
# if 1000000000L % CPU_TIME_TICKS_LOW == 0
#  define NSEC_TO_CPU_TIME_T(TIME) ((TIME) / (1000000000L / CPU_TIME_TICKS))
# endif
#elif CPU_TIME_TICKS_LOW == 1000000000L
# define NSEC_TO_CPU_TIME_T(TIME) (TIME)
#else
# error cpp evaluation problem
#endif

#if CPU_TIME_TICKS_LOW > 1000000
# if CPU_TIME_TICKS_LOW % 1000000 == 0
#  define USEC_TO_CPU_TIME_T(TIME) ((TIME) * (CPU_TIME_TICKS / 1000000))
# endif
#elif CPU_TIME_TICKS_LOW < 1000000
# if 1000000 % CPU_TIME_TICKS_LOW == 0
#  define USEC_TO_CPU_TIME_T(TIME) ((TIME) / (1000000 / CPU_TIME_TICKS))
# endif
#elif CPU_TIME_TICKS_LOW == 1000000
# define USEC_TO_CPU_TIME_T(TIME) (TIME)
#else
# error cpp evaluation problem
#endif

#if 0
/* Enable this if it ever becomes necessary to have another strange
 * CPU_TIME_TICKS value. */
# ifndef NSEC_TO_CPU_TIME_T
#  define NSEC_TO_CPU_TIME_T(TIME) CONVERT_TIME ((TIME), 1000000000L,	\
						 CPU_TIME_TICKS)
# endif
# ifndef USEC_TO_CPU_TIME_T
#  define USEC_TO_CPU_TIME_T(TIME) CONVERT_TIME ((TIME), 1000000,	\
						 CPU_TIME_TICKS)
# endif
#endif

#ifdef __NT__
PMOD_EXPORT int pike_get_rusage(pike_rusage_t rusage_values)
{
  union {
    unsigned __int64 ft_scalar;
    FILETIME ft_struct;
  } creationTime, exitTime, kernelTime, userTime;
  memset(rusage_values, 0, sizeof(pike_rusage_t));
  if (GetProcessTimes(GetCurrentProcess(),
                      &creationTime.ft_struct,
                      &exitTime.ft_struct,
                      &kernelTime.ft_struct,
                      &userTime.ft_struct))
    {
      rusage_values[0] = userTime.ft_scalar/10000;  /* user time */
      rusage_values[1] = kernelTime.ft_scalar/10000;  /* system time */
      return 1;
    }
  else
    {
      rusage_values[0] = 0;  /* user time */
      rusage_values[1] = 0;  /* system time */
      return 0;
    }
}

#else /* __NT__ */
#ifdef HAVE_GETRUSAGE
#include <sys/resource.h>
#ifdef HAVE_SYS_RUSAGE
#include <sys/rusage.h>
#endif

PMOD_EXPORT int pike_get_rusage(pike_rusage_t rusage_values)
{
  struct rusage rus;
  long utime, stime;
  int maxrss;
  memset(rusage_values, 0, sizeof(pike_rusage_t));

  if (getrusage(RUSAGE_SELF, &rus) < 0) return 0;

  utime = rus.ru_utime.tv_sec * 1000L + rus.ru_utime.tv_usec / 1000;
  stime = rus.ru_stime.tv_sec * 1000L + rus.ru_stime.tv_usec / 1000;

#ifndef GETRUSAGE_RESTRICTED
  /* ru_maxrss on Linux and most BSDs is in KB. */
  maxrss = rus.ru_maxrss;
#ifdef sun
  /* ru_maxrss on Solaris is in pages. */
  maxrss *= getpagesize() / 1024;
#elif defined(__APPLE__) && defined(__MACH__)
  /* ru_maxrss on MacOS X is in bytes. */
  maxrss /= 1024;
#endif
#endif
  rusage_values[0] = utime;
  rusage_values[1] = stime;
#ifndef GETRUSAGE_RESTRICTED
  rusage_values[2] = maxrss;
  rusage_values[3] = rus.ru_ixrss;
  rusage_values[4] = rus.ru_idrss;
  rusage_values[5] = rus.ru_isrss;
  rusage_values[6] = rus.ru_minflt;
  rusage_values[7] = rus.ru_majflt;
  rusage_values[8] = rus.ru_nswap;
  rusage_values[9] = rus.ru_inblock;
  rusage_values[10] = rus.ru_oublock;
  rusage_values[11] = rus.ru_msgsnd;
  rusage_values[12] = rus.ru_msgrcv;
  rusage_values[13] = rus.ru_nsignals;
  rusage_values[14] = rus.ru_nvcsw;
  rusage_values[15] = rus.ru_nivcsw;
#endif
  return 1;
}

#else /* HAVE_GETRUSAGE */

#ifdef HAVE_TIMES

PMOD_EXPORT int pike_get_rusage(pike_rusage_t rusage_values)
{
  struct tms tms;
  clock_t ret = times (&tms);

#ifdef PIKE_DEBUG
  if (!pike_clk_tck) error ("Called before init_pike.\n");
#endif

  memset(rusage_values, 0, sizeof(pike_rusage_t));
  if (ret == (clock_t) -1) return 0;

  rusage_values[0] = CONVERT_TIME (tms.tms_utime, pike_clk_tck, 1000);
  rusage_values[1] = CONVERT_TIME (tms.tms_stime, pike_clk_tck, 1000);

  /* It's not really clear if ret is real time; in at least Linux it
   * is, but GNU libc says "The return value is the calling process'
   * CPU time (the same value you get from `clock()'" and "In the GNU
   * system, the CPU time is defined to be equivalent to the sum of
   * the `tms_utime' and `tms_stime' fields returned by `times'." */
  rusage_values[18] = CONVERT_TIME (ret, pike_clk_tck, 1000);

  return 1;
}

#else /*HAVE_TIMES */

#ifndef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC	1000000
#endif /* !CLOCKS_PER_SEC */

PMOD_EXPORT int pike_get_rusage(pike_rusage_t rusage_values)
{
  memset(rusage_values, 0, sizeof(pike_rusage_t));
  rusage_values[0]= CONVERT_TIME (clock(), CLOCKS_PER_SEC, 1000);
  return 1;
}

#endif /* HAVE_TIMES */
#endif /* HAVE_GETRUSAGE */
#endif /* __NT__ */

/*
 * Fix a good get_cpu_time and get_real_time.
 */

#ifdef GCT_RUNTIME_CHOICE
#ifndef cpu_time_is_thread_local
PMOD_EXPORT int cpu_time_is_thread_local;
#endif
PMOD_EXPORT const char *get_cpu_time_impl;
PMOD_EXPORT cpu_time_t (*get_cpu_time) (void);
PMOD_EXPORT cpu_time_t (*get_cpu_time_res) (void);
#endif

#ifdef GRT_RUNTIME_CHOICE
PMOD_EXPORT int real_time_is_monotonic;
PMOD_EXPORT const char *get_real_time_impl = NULL;
PMOD_EXPORT cpu_time_t (*get_real_time) (void);
PMOD_EXPORT cpu_time_t (*get_real_time_res) (void);
#endif

/* First see if we can use the POSIX standard interface. */

#ifdef _POSIX_TIMERS
#if _POSIX_TIMERS > 0 && !defined(__NT__)

#if defined (MIGHT_HAVE_POSIX_THREAD_GCT) &&				\
  (defined (GCT_IS_POSIX_THREAD) || defined (GCT_RUNTIME_CHOICE))

PMOD_EXPORT const char posix_thread_gct_impl[] =
  "CLOCK_THREAD_CPUTIME_ID";

PMOD_EXPORT cpu_time_t posix_thread_gct (void)
{
  struct timespec res;
  if (clock_gettime (CLOCK_THREAD_CPUTIME_ID, &res))
    return (cpu_time_t) -1;
  return res.tv_sec * CPU_TIME_TICKS + NSEC_TO_CPU_TIME_T (res.tv_nsec);
}

PMOD_EXPORT cpu_time_t posix_thread_gct_res (void)
{
  struct timespec res;
  cpu_time_t t;
  if (clock_getres (CLOCK_THREAD_CPUTIME_ID, &res))
    return (cpu_time_t) -1;
  t = res.tv_sec * CPU_TIME_TICKS + NSEC_TO_CPU_TIME_T (res.tv_nsec);
  return t ? t : 1;
}

#endif

#if defined (MIGHT_HAVE_POSIX_PROCESS_GCT) &&				\
  (defined (GCT_IS_POSIX_PROCESS) || defined (GCT_RUNTIME_CHOICE))

PMOD_EXPORT const char posix_process_gct_impl[] =
  "CLOCK_PROCESS_CPUTIME_ID";

PMOD_EXPORT cpu_time_t posix_process_gct (void)
{
  struct timespec res;
  if (clock_gettime (CLOCK_PROCESS_CPUTIME_ID, &res))
    return (cpu_time_t) -1;
  return res.tv_sec * CPU_TIME_TICKS + NSEC_TO_CPU_TIME_T (res.tv_nsec);
}

PMOD_EXPORT cpu_time_t posix_process_gct_res (void)
{
  struct timespec res;
  cpu_time_t t;
  if (clock_getres (CLOCK_PROCESS_CPUTIME_ID, &res))
    return (cpu_time_t) -1;
  t = res.tv_sec * CPU_TIME_TICKS + NSEC_TO_CPU_TIME_T (res.tv_nsec);
  return t ? t : 1;
}

#endif

#if defined(GCT_RUNTIME_CHOICE) &&	   \
  (defined(MIGHT_HAVE_POSIX_THREAD_GCT) || \
   defined(MIGHT_HAVE_POSIX_PROCESS_GCT))
/* From Linux man page clock_gettime(3), dated 2003-08-24:
 *
 * NOTE for SMP systems
 *       The CLOCK_PROCESS_CPUTIME_ID and CLOCK_THREAD_CPUTIME_ID
 *       clocks are realized on many platforms using timers from the
 *       CPUs (TSC on i386, AR.ITC on Itanium). These registers may
 *       differ between CPUs and as a consequence these clocks may
 *       return bogus results if a process is migrated to another CPU.
 *
 *       If the CPUs in an SMP system have different clock sources
 *       then there is no way to maintain a correlation between the
 *       timer registers since each CPU will run at a slightly
 *       different frequency. If that is the case then
 *       clock_getcpuclockid(0) will return ENOENT to signify this
 *       condition. The two clocks will then only be useful if it can
 *       be ensured that a process stays on a certain CPU.
 */
#ifdef HAVE_CLOCK_GETCPUCLOCKID
static int posix_cputime_is_reliable (void)
{
  clockid_t res;
  return clock_getcpuclockid (0, &res) != ENOENT;
}
#else
#define posix_cputime_is_reliable() 1
#endif
#endif

#if defined (MIGHT_HAVE_POSIX_MONOTONIC_GRT) &&				\
  (defined (GRT_IS_POSIX_MONOTONIC) || defined (GRT_RUNTIME_CHOICE))

PMOD_EXPORT const char posix_monotonic_grt_impl[] = "CLOCK_MONOTONIC";

PMOD_EXPORT cpu_time_t posix_monotonic_grt (void)
{
  struct timespec res;
  if (clock_gettime (CLOCK_MONOTONIC, &res))
    return (cpu_time_t) -1;
  return res.tv_sec * CPU_TIME_TICKS + NSEC_TO_CPU_TIME_T (res.tv_nsec);
}

PMOD_EXPORT cpu_time_t posix_monotonic_grt_res (void)
{
  struct timespec res;
  cpu_time_t t;
  if (clock_getres (CLOCK_MONOTONIC, &res))
    return (cpu_time_t) -1;
  t = res.tv_sec * CPU_TIME_TICKS + NSEC_TO_CPU_TIME_T (res.tv_nsec);
  return t ? t : 1;
}

#endif

#if defined (MIGHT_HAVE_POSIX_REALTIME_GRT) &&				\
  (defined (GRT_IS_POSIX_REALTIME) || defined (GRT_RUNTIME_CHOICE))

PMOD_EXPORT const char posix_realtime_grt_impl[] = "CLOCK_REALTIME";

PMOD_EXPORT cpu_time_t posix_realtime_grt (void)
{
  struct timespec res;
  if (clock_gettime (CLOCK_REALTIME, &res)) {
#ifdef PIKE_DEBUG
    Pike_fatal ("CLOCK_REALTIME should always work! errno=%d\n", errno);
#endif
  }
  return res.tv_sec * CPU_TIME_TICKS + NSEC_TO_CPU_TIME_T (res.tv_nsec);
}

PMOD_EXPORT cpu_time_t posix_realtime_grt_res (void)
{
  struct timespec res;
  cpu_time_t t;
  if (clock_getres (CLOCK_REALTIME, &res)) {
#ifdef PIKE_DEBUG
    Pike_fatal ("CLOCK_REALTIME should always work! errno=%d\n", errno);
#endif
  }
  t = res.tv_sec * CPU_TIME_TICKS + NSEC_TO_CPU_TIME_T (res.tv_nsec);
  return t ? t : 1;
}

#endif

#endif	/* _POSIX_TIMERS > 0 */
#endif	/* _POSIX_TIMERS */

/* Now various fallback interfaces in order of preference. */

#if (defined (GCT_IS_FALLBACK) || defined (GCT_RUNTIME_CHOICE))
#define DEFINE_FALLBACK_GCT
#endif

#if (defined (GRT_IS_FALLBACK) || defined (GRT_RUNTIME_CHOICE))
#define DEFINE_FALLBACK_GRT
#endif

#ifdef __NT__

/* cpu_time_t is known to be 64 bits here, i.e. with nanosecond resolution. */

#ifdef DEFINE_FALLBACK_GCT

PMOD_EXPORT const char fallback_gct_impl[] = "GetThreadTimes()";

PMOD_EXPORT cpu_time_t fallback_gct (void)
{
  union {
    unsigned __int64 ft_scalar;
    FILETIME ft_struct;
  } creationTime, exitTime, kernelTime, userTime;
  if (GetThreadTimes(GetCurrentThread(),
		     &creationTime.ft_struct,
		     &exitTime.ft_struct,
		     &kernelTime.ft_struct,
		     &userTime.ft_struct))
    return ((cpu_time_t) userTime.ft_scalar + kernelTime.ft_scalar) * 100;
  else
    return (cpu_time_t) -1;
}

#define HAVE_FALLBACK_GCT_RES
PMOD_EXPORT cpu_time_t fallback_gct_res (void)
{
  /* Got 100 ns resolution according to docs. */
  return NSEC_TO_CPU_TIME_T (100);
}

#endif

#ifdef DEFINE_FALLBACK_GRT

PMOD_EXPORT const char fallback_grt_impl[] = "QueryPerformanceCounter()";

PMOD_EXPORT int fallback_grt_is_monotonic = 1;
/* The doc isn't outspoken on this, but it's a reasonable assumption
 * since its expressed use is to measure small time intervals by
 * calculating differences. */

/* Disclaimer from MSDN:
 *
 * On a multiprocessor computer, it should not matter which processor
 * is called. However, you can get different results on different
 * processors due to bugs in the basic input/output system (BIOS) or
 * the hardware abstraction layer (HAL).
 */

PMOD_EXPORT cpu_time_t fallback_grt (void)
{
  LARGE_INTEGER res;
  if (!perf_freq.QuadPart || !QueryPerformanceCounter (&res))
    return (cpu_time_t) -1;
  return CONVERT_TIME (res.QuadPart, perf_freq.QuadPart, CPU_TIME_TICKS);
}

#define HAVE_FALLBACK_GRT_RES
PMOD_EXPORT cpu_time_t fallback_grt_res (void)
{
  if (!perf_freq.QuadPart) return (cpu_time_t) -1;
  return CPU_TIME_TICKS / perf_freq.QuadPart;
}

#endif

#elif 0 && defined (HAVE_WORKING_GETHRVTIME)

#ifdef DEFINE_FALLBACK_GCT
PMOD_EXPORT const char fallback_gct_impl[] = "gethrvtime()";
PMOD_EXPORT cpu_time_t fallback_gct (void)
{
  return NSEC_TO_CPU_TIME_T (gethrvtime());
}
#endif

#ifdef DEFINE_FALLBACK_GRT
PMOD_EXPORT const char fallback_grt_impl[] = "gethrtime()";
PMOD_EXPORT int fallback_grt_is_monotonic = 1;
PMOD_EXPORT cpu_time_t fallback_grt (void)
{
  return NSEC_TO_CPU_TIME_T (gethrtime());
}
#endif

#else  /* !__NT__ && !HAVE_WORKING_GETHRVTIME */

#ifdef DEFINE_FALLBACK_GCT
#ifdef HAVE_THREAD_INFO

/* Mach */

PMOD_EXPORT const char fallback_gct_impl[] = "thread_info()";

PMOD_EXPORT cpu_time_t fallback_gct (void)
{
  thread_basic_info_data_t tbid;
  mach_msg_type_number_t tbid_len = THREAD_BASIC_INFO_COUNT;

  /*  Try to get kernel thread via special OS X extension since it's faster
   *  (~40x on PPC G5) than the context switch caused by mach_thread_self().
   */
#ifdef HAVE_PTHREAD_MACH_THREAD_NP
  mach_port_t self = pthread_mach_thread_np(pthread_self());
#else
  mach_port_t self = mach_thread_self();
#endif
  int err = thread_info (self, THREAD_BASIC_INFO,
						 (thread_info_t) &tbid, &tbid_len);
#ifndef HAVE_PTHREAD_MACH_THREAD_NP
  /*  Adjust refcount on new port returned from mach_thread_self(). Not
   *  needed for pthread_mach_thread_np() since we're reusing an existing
   *  port.
   */
  mach_port_deallocate(mach_task_self(), self);
#endif
  if (err)
    return (cpu_time_t) -1;
  return
    tbid.user_time.seconds * CPU_TIME_TICKS +
    USEC_TO_CPU_TIME_T (tbid.user_time.microseconds) +
    tbid.system_time.seconds * CPU_TIME_TICKS +
    USEC_TO_CPU_TIME_T (tbid.system_time.microseconds);
}

#define HAVE_FALLBACK_GCT_RES
PMOD_EXPORT cpu_time_t fallback_gct_res (void)
{
  /* Docs aren't clear on the resolution, so assume it's accurate to
   * the extent of time_value_t. */
  return MAXIMUM (CPU_TIME_TICKS / 1000000, 1);
}

#elif defined (HAVE_TIMES)

/* Prefer times() over clock() since the ticks per second isn't
 * defined by POSIX to some constant and it thus lies closer to the
 * real accurancy. (CLOCKS_PER_SEC is always defined to 1000000 which
 * means that clock() wraps much more often than necessary.) */

PMOD_EXPORT const char fallback_gct_impl[] = "times()";

PMOD_EXPORT cpu_time_t fallback_gct (void)
{
  struct tms tms;
#ifdef PIKE_DEBUG
  if (!pike_clk_tck) Pike_fatal ("Called before init_pike.\n");
#endif
  if (times (&tms) == (clock_t) -1)
    return (cpu_time_t) -1;
  return CONVERT_TIME (tms.tms_utime + tms.tms_stime, pike_clk_tck, CPU_TIME_TICKS);
}

#define HAVE_FALLBACK_GCT_RES
PMOD_EXPORT cpu_time_t fallback_gct_res (void)
{
  return MAXIMUM (CPU_TIME_TICKS / pike_clk_tck, 1);
}

#elif defined (HAVE_CLOCK) && defined (CLOCKS_PER_SEC)

PMOD_EXPORT const char fallback_gct_impl[] = "clock()";

PMOD_EXPORT cpu_time_t fallback_gct (void)
{
  clock_t t = clock();
  if (t == (clock_t) -1)
    return (cpu_time_t) -1;
  else
    return CONVERT_TIME (t, CLOCKS_PER_SEC, CPU_TIME_TICKS);
}

#define HAVE_FALLBACK_GCT_RES
PMOD_EXPORT cpu_time_t fallback_gct_res (void)
{
  return MAXIMUM (CPU_TIME_TICKS / CLOCKS_PER_SEC, 1);
}

#elif defined (HAVE_GETRUSAGE)

PMOD_EXPORT const char fallback_gct_impl[] = "getrusage()";

PMOD_EXPORT cpu_time_t fallback_gct (void)
{
  struct rusage rus;
  if (getrusage(RUSAGE_SELF, &rus) < 0) return (cpu_time_t) -1;
  return
    rus.ru_utime.tv_sec * CPU_TIME_TICKS +
    USEC_TO_CPU_TIME_T (rus.ru_utime.tv_usec) +
    rus.ru_stime.tv_sec * CPU_TIME_TICKS +
    USEC_TO_CPU_TIME_T (rus.ru_stime.tv_usec);
}

#define HAVE_FALLBACK_GCT_RES
PMOD_EXPORT cpu_time_t fallback_gct_res (void)
{
  /* struct timeval holds microseconds, so assume the resolution is on
   * that level. */
  return MAXIMUM (CPU_TIME_TICKS / 1000000, 1);
}

#else

PMOD_EXPORT const char fallback_gct_impl[] = "none";

PMOD_EXPORT cpu_time_t fallback_gct (void)
{
  return (cpu_time_t) -1;
}

#endif
#endif	/* DEFINE_FALLBACK_GCT */

#ifdef DEFINE_FALLBACK_GRT
#if defined (HAVE_HOST_GET_CLOCK_SERVICE) && defined (GRT_RUNTIME_CHOICE)

/* Mach */

/* NB: clock_map_time looks like an interesting alternative. */

static clock_serv_t mach_clock_port;

static cpu_time_t mach_clock_get_time (void)
{
  mach_timespec_t ts;
#ifdef PIKE_DEBUG
  if (!get_real_time_impl) Pike_fatal ("Called before init_pike.\n");
#endif
  if (clock_get_time (mach_clock_port, &ts)) return (cpu_time_t) -1;
  return ts.tv_sec * CPU_TIME_TICKS + NSEC_TO_CPU_TIME_T (ts.tv_nsec);
}

static cpu_time_t mach_clock_get_time_res (void)
{
  natural_t res;
  mach_msg_type_number_t res_size = 1;
#ifdef PIKE_DEBUG
  if (!get_real_time_impl) Pike_fatal ("Called before init_pike.\n");
#endif
  if (clock_get_attributes (mach_clock_port, CLOCK_GET_TIME_RES,
			    (clock_attr_t) &res, &res_size) ||
      res_size < 1)
    return (cpu_time_t) -1;
  return MAXIMUM (NSEC_TO_CPU_TIME_T (res), 1);
}

/* Defined since init_mach_clock always succeeds. */
#define HAVE_FALLBACK_GRT_RES

static void init_mach_clock (void)
{
  /* We assume noone messes with clock_set_time. */
  real_time_is_monotonic = 1;

  get_real_time = mach_clock_get_time;
  get_real_time_res = mach_clock_get_time_res;

#ifdef HIGHRES_CLOCK
  if (!host_get_clock_service (mach_host_self(), HIGHRES_CLOCK,
			       &mach_clock_port)) {
    get_real_time_impl = "host_get_clock_service(HIGHRES_CLOCK)";
    return;
  }
#endif

  /* SYSTEM_CLOCK should always exist. */
  if (host_get_clock_service (mach_host_self(), SYSTEM_CLOCK,
			      &mach_clock_port)) {
#ifdef PIKE_DEBUG
    Pike_fatal ("Unexpected failure in host_get_clock_service().\n");
#endif
  }
  get_real_time_impl = "host_get_clock_service(SYSTEM_CLOCK)";
}

#else

PMOD_EXPORT const char fallback_grt_impl[] = "gettimeofday()";
PMOD_EXPORT int fallback_grt_is_monotonic = 0;
PMOD_EXPORT cpu_time_t fallback_grt(void)
{
  struct timeval tv;
  int gtod_rval;
#ifndef CONFIGURE_TEST
  ACCURATE_GETTIMEOFDAY_RVAL(&tv, gtod_rval);
  if (gtod_rval < 0) return -1;
#else
  if (GETTIMEOFDAY(&tv) < 0) return -1;
#endif
  return tv.tv_sec * CPU_TIME_TICKS + USEC_TO_CPU_TIME_T (tv.tv_usec);
}

#define HAVE_FALLBACK_GRT_RES
PMOD_EXPORT cpu_time_t fallback_grt_res (void)
{
  /* struct timeval holds microseconds, so assume the resolution is on
   * that level. */
  return MAXIMUM (CPU_TIME_TICKS / 1000000, 1);
}

#endif
#endif	/* DEFINE_FALLBACK_GRT */

#endif	/* !__NT__ && !HAVE_WORKING_GETHRVTIME */

#if defined (DEFINE_FALLBACK_GCT) && !defined (HAVE_FALLBACK_GCT_RES)
PMOD_EXPORT cpu_time_t fallback_gct_res (void)
{
  return (cpu_time_t) -1;
}
#endif

#if defined (DEFINE_FALLBACK_GRT) && !defined (HAVE_FALLBACK_GRT_RES)
PMOD_EXPORT cpu_time_t fallback_grt_res (void)
{
  return (cpu_time_t) -1;
}
#endif

long *low_rusage(void)
{
  static pike_rusage_t rusage_values;
  if (pike_get_rusage (rusage_values))
    return &rusage_values[0];
  else
    return NULL;
}

INT32 internal_rusage(void)
{
  cpu_time_t t = get_cpu_time();
  return t != (cpu_time_t) -1 ? t / (CPU_TIME_TICKS / 1000) : 0;
}

#if defined(PIKE_DEBUG) || defined(INTERNAL_PROFILING)
void debug_print_rusage(FILE *out)
{
  pike_rusage_t rusage_values;
  pike_get_rusage (rusage_values);
  fprintf (out,
	   " [utime: %ld, stime: %ld] [maxrss: %ld, ixrss: %ld, idrss: %ld, isrss: %ld]\n"
	   " [minflt: %ld, majflt: %ld] [nswap: %ld] [inblock: %ld, oublock: %ld]\n"
	   " [msgsnd: %ld, msgrcv: %ld] [nsignals: %ld] [nvcsw: %ld, nivcsw: %ld]\n"
	   " [sysc: %ld] [ioch: %ld] [rtime: %ld, ttime: %ld]\n"
	   " [tftime: %ld, dftime: %ld, kftime: %ld, ltime: %ld, slptime: %ld]\n"
	   " [wtime: %ld, stoptime: %ld] [brksize: %ld, stksize: %ld]\n",
	   rusage_values[0], rusage_values[1],
	   rusage_values[2], rusage_values[3], rusage_values[4], rusage_values[5],
	   rusage_values[6], rusage_values[7],
	   rusage_values[8],
	   rusage_values[9], rusage_values[10],
	   rusage_values[11], rusage_values[12],
	   rusage_values[13],
	   rusage_values[14], rusage_values[15],
	   rusage_values[16],
	   rusage_values[17],
	   rusage_values[18], rusage_values[19],
	   rusage_values[20], rusage_values[21], rusage_values[22], rusage_values[23], rusage_values[24],
	   rusage_values[25], rusage_values[26],
	   rusage_values[27], rusage_values[28]);
}
#endif

void init_rusage (void)
{
#ifdef HAVE_TIMES
  pike_clk_tck = sysconf (_SC_CLK_TCK);
#endif

#ifdef __NT__
  if (!QueryPerformanceFrequency (&perf_freq))
    perf_freq.QuadPart = 0;
#endif

#ifdef GCT_RUNTIME_CHOICE

#ifdef MIGHT_HAVE_POSIX_THREAD_GCT
  if (sysconf (_SC_THREAD_CPUTIME) > 0 && posix_cputime_is_reliable()) {
#ifndef cpu_time_is_thread_local
    cpu_time_is_thread_local = 1;
#endif
    get_cpu_time_impl = posix_thread_gct_impl;
    get_cpu_time = posix_thread_gct;
    get_cpu_time_res = posix_thread_gct_res;
  }
  else
#endif

#ifdef MIGHT_HAVE_POSIX_PROCESS_GCT
    if (sysconf (_SC_CPUTIME) > 0 && posix_cputime_is_reliable()) {
#ifndef cpu_time_is_thread_local
      cpu_time_is_thread_local = 0;
#endif
      get_cpu_time_impl = posix_process_gct_impl;
      get_cpu_time = posix_process_gct;
      get_cpu_time_res = posix_process_gct_res;
    }
    else
#endif

    {
#ifndef cpu_time_is_thread_local
#if FB_CPU_TIME_IS_THREAD_LOCAL == PIKE_YES
      cpu_time_is_thread_local = 1;
#else
      cpu_time_is_thread_local = 0;
#endif
#endif
      get_cpu_time_impl = fallback_gct_impl;
      get_cpu_time = fallback_gct;
      get_cpu_time_res = fallback_gct_res;
    }

#endif  /* GCT_RUNTIME_CHOICE */

#ifdef GRT_RUNTIME_CHOICE

#ifdef MIGHT_HAVE_POSIX_MONOTONIC_GRT
  if (sysconf (_SC_MONOTONIC_CLOCK) > 0) {
    get_real_time_impl = posix_monotonic_grt_impl;
    real_time_is_monotonic = 1;
    get_real_time = posix_monotonic_grt;
    get_real_time_res = posix_monotonic_grt_res;
  }
  else
#endif

  {
#ifdef HAVE_HOST_GET_CLOCK_SERVICE
#ifdef _REENTRANT
    th_atfork(NULL, NULL, init_mach_clock);
#endif
    init_mach_clock();
#elif defined (MIGHT_HAVE_POSIX_REALTIME_GRT)
    /* Always exists according to POSIX - no need to check with sysconf. */
    get_real_time_impl = posix_realtime_grt_impl;
    real_time_is_monotonic = 0;
    get_real_time = posix_realtime_grt;
    get_real_time_res = posix_realtime_grt_res;
#else
    get_real_time_impl = fallback_grt_impl;
    real_time_is_monotonic = fallback_grt_is_monotonic;
    get_real_time = fallback_grt;
    get_real_time_res = fallback_grt_res;
#endif
  }

#endif	/* GRT_RUNTIME_CHOICE */

#if 0
  fprintf (stderr, "get_cpu_time %s choice: %s, "
	   "resolution: %"PRINT_CPU_TIME"%s\n",
#ifdef GCT_RUNTIME_CHOICE
	   "runtime",
#else
	   "compile time",
#endif
	   get_cpu_time_impl,
	   get_cpu_time_res(),
	   cpu_time_is_thread_local > 0 ? ", thread local" :
	   !cpu_time_is_thread_local ? ", not thread local" : "");

  fprintf (stderr, "get_real_time %s choice: %s, "
	   "resolution: %"PRINT_CPU_TIME"%s\n",
#ifdef GRT_RUNTIME_CHOICE
	   "runtime",
#else
	   "compile time",
#endif
	   get_real_time_impl,
	   get_real_time_res(),
	   real_time_is_monotonic > 0 ? ", monotonic" :
	   !real_time_is_monotonic ? ", not monotonic" : "");
#endif
}
