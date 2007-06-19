/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: rusage.c,v 1.48 2007/06/19 18:22:50 mast Exp $
*/

#include "global.h"
#include "pike_macros.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "pike_rusage.h"

#ifdef HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifndef CONFIGURE_TEST
#include "time_stuff.h"
#include "fd_control.h"
#endif
#include "pike_error.h"

/*
 * Here comes a long blob with stuff to see how to find out about
 * cpu usage.
 */

#ifdef HAVE_TIMES
long pike_clk_tck = 0;
#endif

/*
 * Here's a trick to do TIME * BASE / TICKS without
 * causing arithmetic overflow
 */
#define CONVERT_TIME(TIME, TICKS, BASE) \
  (((TIME) / (TICKS)) * (BASE) + ((TIME) % (TICKS)) * (BASE) / (TICKS))

#ifdef __NT__
PMOD_EXPORT int pike_get_rusage(pike_rusage_t rusage_values)
{
  union {
    unsigned __int64 ft_scalar;
    FILETIME ft_struct;
  } creationTime, exitTime, kernelTime, userTime;
  MEMSET(rusage_values, 0, sizeof(pike_rusage_t));
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
#ifdef GETRUSAGE_THROUGH_PROCFS
#include <sys/procfs.h>
#ifndef CONFIGURE_TEST
#include "fd_control.h"
#endif

static INLINE long get_time_int(timestruc_t * val)
{
  return val->tv_sec * 1000L + val->tv_nsec / 1000000;
}

static int proc_fd = -1;

static int open_proc_fd()
{
  do {
    char proc_name[30];
    sprintf(proc_name, "/proc/%05ld", (long)getpid());
    proc_fd = open(proc_name, O_RDONLY);
    if(proc_fd >= 0) break;
    if(errno != EINTR) return 0;
  } while(proc_fd < 0);

#ifndef CONFIGURE_TEST
  set_close_on_exec(proc_fd, 1);
#endif

  return 1;
}

PMOD_EXPORT int pike_get_rusage(pike_rusage_t rusage_values)
{
  prusage_t  pru;
#ifdef GETRUSAGE_THROUGH_PROCFS_PRS
  prstatus_t prs;
#endif
  MEMSET(rusage_values, 0, sizeof(pike_rusage_t));

  if (proc_fd < 0 && !open_proc_fd()) return 0;
  while(ioctl(proc_fd, PIOCUSAGE, &pru) < 0)
  {
    if(errno == EINTR)
      continue;

    return 0;
  }

#ifdef GETRUSAGE_THROUGH_PROCFS_PRS
  while(ioctl(proc_fd, PIOCSTATUS, &prs) < 0)
  {
    if(errno == EINTR)
      continue;

    return 0;
  }
#endif

  rusage_values[0] = get_time_int(&pru.pr_utime);  /* user time */
  rusage_values[1] = get_time_int(&pru.pr_stime);  /* system time */
  rusage_values[2] = 0;                           /* maxrss */
  rusage_values[3] = 0;                           /* ixrss */
  rusage_values[4] = 0;                           /* idrss */
  rusage_values[5] = 0;                           /* isrss */
  rusage_values[6] = pru.pr_minf;           /* minor pagefaults */
  rusage_values[7] = pru.pr_majf;           /* major pagefaults */
  rusage_values[8] = pru.pr_nswap;          /* swaps */
  rusage_values[9] = pru.pr_inblk;          /* block input op. */
  rusage_values[10] = pru.pr_oublk;         /* block outout op. */
  rusage_values[11] = pru.pr_msnd;          /* messages sent */
  rusage_values[12] = pru.pr_mrcv;          /* messages received */
  rusage_values[13] = pru.pr_sigs;          /* signals received */
  rusage_values[14] = pru.pr_vctx;          /* voluntary context switches */
  rusage_values[15] = pru.pr_ictx;          /* involuntary  "        " */
  rusage_values[16] = pru.pr_sysc;          /* system calls */
  rusage_values[17] = pru.pr_ioch;          /* chars read and written */
  rusage_values[18] = get_time_int(&pru.pr_rtime); /* total lwp real (elapsed) time */
  rusage_values[19] = get_time_int(&pru.pr_ttime); /* other system trap CPU time */
  rusage_values[20] = get_time_int(&pru.pr_tftime); /* text page fault sleep time */
  rusage_values[21] = get_time_int(&pru.pr_dftime); /* data page fault sleep time */
  rusage_values[22] = get_time_int(&pru.pr_kftime); /* kernel page fault sleep time */
  rusage_values[23] = get_time_int(&pru.pr_ltime); /* user lock wait sleep time */
  rusage_values[24] = get_time_int(&pru.pr_slptime); /* all other sleep time */
  rusage_values[25] = get_time_int(&pru.pr_wtime); /* wait-cpu (latency) time */
  rusage_values[26] = get_time_int(&pru.pr_stoptime); /* stopped time */
#ifdef GETRUSAGE_THROUGH_PROCFS_PRS
  rusage_values[27] = prs.pr_brksize;
  rusage_values[28] = prs.pr_stksize;
#endif

  return 1;
}

#else /* GETRUSAGE_THROUGH_PROCFS */
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
  MEMSET(rusage_values, 0, sizeof(pike_rusage_t));

  if (getrusage(RUSAGE_SELF, &rus) < 0) return 0;

  utime = rus.ru_utime.tv_sec * 1000L + rus.ru_utime.tv_usec / 1000;
  stime = rus.ru_stime.tv_sec * 1000L + rus.ru_stime.tv_usec / 1000;

#ifndef GETRUSAGE_RESTRICTED
  maxrss = rus.ru_maxrss;
#ifdef sun
  maxrss *= getpagesize() / 1024;
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

#if defined(HAVE_TIMES)

PMOD_EXPORT int pike_get_rusage(pike_rusage_t rusage_values)
{
  struct tms tms;
  clock_t ret = times (&tms);

#ifdef PIKE_DEBUG
  if (!pike_clk_tck) error ("Called before init_pike.\n");
#endif

  MEMSET(rusage_values, 0, sizeof(pike_rusage_t));
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
#if defined(HAVE_CLOCK)

#ifndef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC	1000000
#endif /* !CLOCKS_PER_SEC */

PMOD_EXPORT int pike_get_rusage(pike_rusage_t rusage_values)
{
  MEMSET(rusage_values, 0, sizeof(pike_rusage_t));
  rusage_values[0]= CONVERT_TIME (clock(), CLOCKS_PER_SEC, 1000);
  return 1;
}

#else /* HAVE_CLOCK */

PMOD_EXPORT int pike_get_rusage(pike_rusage_t rusage_values)
{
  /* This is totally wrong, but hey, if you can't do it _right_... */
  struct timeval tm;
  MEMSET(rusage_values, 0, sizeof(pike_rusage_t));
  GETTIMEOFDAY(&tm);
  rusage_values[0]=tm.tv_sec*1000L + tm.tv_usec/1000;
  return 1;
}

#endif /* HAVE_CLOCK */
#endif /* HAVE_TIMES */
#endif /* HAVE_GETRUSAGE */
#endif /* GETRUSAGE_THROUGH_PROCFS */
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
PMOD_EXPORT const char *get_real_time_impl;
PMOD_EXPORT cpu_time_t (*get_real_time) (void);
PMOD_EXPORT cpu_time_t (*get_real_time_res) (void);
#endif

/* First see if we can use the POSIX standard interface. */

#if defined (_POSIX_TIMERS)
#if _POSIX_TIMERS > 0

#if defined (MIGHT_HAVE_POSIX_THREAD_GCT) &&				\
  (defined (GCT_IS_POSIX_THREAD) || defined (GCT_RUNTIME_CHOICE))

PMOD_EXPORT const char posix_thread_gct_impl[] =
  "CLOCK_THREAD_CPUTIME_ID";

PMOD_EXPORT cpu_time_t posix_thread_gct (void)
{
  struct timespec res;
  if (clock_gettime (CLOCK_THREAD_CPUTIME_ID, &res))
    return (cpu_time_t) -1;
  return res.tv_sec * CPU_TIME_TICKS +
    res.tv_nsec / (1000000000 / CPU_TIME_TICKS);
}

PMOD_EXPORT cpu_time_t posix_thread_gct_res (void)
{
  struct timespec res;
  cpu_time_t t;
  if (clock_getres (CLOCK_THREAD_CPUTIME_ID, &res))
    return (cpu_time_t) -1;
  t = res.tv_sec * CPU_TIME_TICKS +
    res.tv_nsec / (1000000000 / CPU_TIME_TICKS);
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
  return res.tv_sec * CPU_TIME_TICKS +
    res.tv_nsec / (1000000000 / CPU_TIME_TICKS);
}

PMOD_EXPORT cpu_time_t posix_process_gct_res (void)
{
  struct timespec res;
  cpu_time_t t;
  if (clock_getres (CLOCK_PROCESS_CPUTIME_ID, &res))
    return (cpu_time_t) -1;
  t = res.tv_sec * CPU_TIME_TICKS +
    res.tv_nsec / (1000000000 / CPU_TIME_TICKS);
  return t ? t : 1;
}

#endif

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
 *       then there is no way to maintain a corre‐lation between the
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

#if defined (MIGHT_HAVE_POSIX_MONOTONIC_GRT) &&				\
  (defined (GRT_IS_POSIX_MONOTONIC) || defined (GRT_RUNTIME_CHOICE))

PMOD_EXPORT const char posix_monotonic_grt_impl[] = "CLOCK_MONOTONIC";

PMOD_EXPORT cpu_time_t posix_monotonic_grt (void)
{
  struct timespec res;
  if (clock_gettime (CLOCK_MONOTONIC, &res))
    return (cpu_time_t) -1;
  return res.tv_sec * CPU_TIME_TICKS +
    res.tv_nsec / (1000000000 / CPU_TIME_TICKS);
}

PMOD_EXPORT cpu_time_t posix_monotonic_grt_res (void)
{
  struct timespec res;
  cpu_time_t t;
  if (clock_getres (CLOCK_MONOTONIC, &res))
    return (cpu_time_t) -1;
  t = res.tv_sec * CPU_TIME_TICKS +
    res.tv_nsec / (1000000000 / CPU_TIME_TICKS);
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
  return res.tv_sec * CPU_TIME_TICKS +
    res.tv_nsec / (1000000000 / CPU_TIME_TICKS);
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
  t = res.tv_sec * CPU_TIME_TICKS +
    res.tv_nsec / (1000000000 / CPU_TIME_TICKS);
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
    return (userTime.ft_scalar + kernelTime.ft_scalar) * 100;
  else
    return (cpu_time_t) -1;
}

#define HAVE_FALLBACK_GCT_RES
PMOD_EXPORT cpu_time_t fallback_gct_res (void)
{
  /* Got 100 ns resolution according to docs. */
  return MAXIMUM (CPU_TIME_TICKS / 10000000, 1);
}

#endif

#ifdef DEFINE_FALLBACK_GRT

PMOD_EXPORT const char fallback_grt_impl[] = "GetThreadTimes()";

PMOD_EXPORT int fallback_grt_is_monotonic = -1;
/* Don't know since the below looks totally bogus. */

PMOD_EXPORT cpu_time_t fallback_grt (void)
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
    /* FIXME: Isn't this completely bogus? /mast */
    return exitTime.ft_scalar * 100;
  else
    return (cpu_time_t) -1;
}

#define HAVE_FALLBACK_GRT_RES
PMOD_EXPORT cpu_time_t fallback_grt_res (void)
{
  /* Got 100 ns resolution according to docs. */
  return MAXIMUM (CPU_TIME_TICKS / 10000000, 1);
}

#endif

#elif 0 && defined (HAVE_WORKING_GETHRVTIME)

#ifdef DEFINE_FALLBACK_GCT
PMOD_EXPORT const char fallback_gct_impl[] = "gethrvtime()";
PMOD_EXPORT cpu_time_t fallback_gct (void)
{
  /* Faster than CONVERT_TIME(gethrvtime(), 1000000000, CPU_TIME_TICKS)
   * It works under the assumtion that CPU_TIME_TICKS <= 1000000000
   * and that 1000000000 % CPU_TIME_TICKS == 0. */
  return gethrvtime() / (1000000000 / CPU_TIME_TICKS);
}
#endif

#ifdef DEFINE_FALLBACK_GRT
PMOD_EXPORT const char fallback_grt_impl[] = "gethrtime()";
PMOD_EXPORT int fallback_grt_is_monotonic = 1;
PMOD_EXPORT cpu_time_t fallback_grt (void)
{
  return gethrtime() * (CPU_TIME_TICKS / 1000000000);
}
#endif

#else  /* !__NT__ && !HAVE_WORKING_GETHRVTIME */

#ifdef DEFINE_FALLBACK_GCT
#if defined (GETRUSAGE_THROUGH_PROCFS)

PMOD_EXPORT const char fallback_gct_impl[] = "/proc/";

PMOD_EXPORT cpu_time_t fallback_gct (void)
{
  prstatus_t  prs;

  if (proc_fd < 0 && !open_proc_fd()) return (cpu_time_t) -1;
  while(ioctl(proc_fd, PIOCSTATUS, &prs) < 0)
  {
    if(errno == EINTR)
      continue;

    return (cpu_time_t) -1;
  }

  return
    prs.pr_utime.tv_sec * CPU_TIME_TICKS +
    prs.pr_utime.tv_nsec / (1000000000 / CPU_TIME_TICKS) +
    prs.pr_stime.tv_sec * CPU_TIME_TICKS +
    prs.pr_stime.tv_nsec / (1000000000 / CPU_TIME_TICKS);
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
#if defined (PIKE_DEBUG)
  if (!pike_clk_tck) Pike_error ("Called before init_pike.\n");
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
  if (CPU_TIME_TICKS > 1000000) {
    return
      rus.ru_utime.tv_sec * CPU_TIME_TICKS +
      rus.ru_utime.tv_usec * (CPU_TIME_TICKS / 1000000) +
      rus.ru_stime.tv_sec * CPU_TIME_TICKS +
      rus.ru_stime.tv_usec * (CPU_TIME_TICKS / 1000000);
  } else {
    return
      rus.ru_utime.tv_sec * CPU_TIME_TICKS +
      rus.ru_utime.tv_usec / (1000000 / CPU_TIME_TICKS) +
      rus.ru_stime.tv_sec * CPU_TIME_TICKS +
      rus.ru_stime.tv_usec * (1000000 / CPU_TIME_TICKS);
  }
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
PMOD_EXPORT const char fallback_grt_impl[] = "gettimeofday()";
PMOD_EXPORT int fallback_grt_is_monotonic = 0;
PMOD_EXPORT cpu_time_t fallback_grt(void)
{
  struct timeval tv;

  if (GETTIMEOFDAY(&tv) < 0) return -1;
#ifdef LONG_CPU_TIME
    return tv.tv_sec * CPU_TIME_TICKS +
      tv.tv_usec * (CPU_TIME_TICKS / 1000000);
#else
    return tv.tv_sec * CPU_TIME_TICKS +
      tv.tv_usec / (1000000 / CPU_TIME_TICKS);
#endif
}

#define HAVE_FALLBACK_GRT_RES
PMOD_EXPORT cpu_time_t fallback_grt_res (void)
{
  /* struct timeval holds microseconds, so assume the resolution is on
   * that level. */
  return MAXIMUM (CPU_TIME_TICKS / 1000000, 1);
}
#endif

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

#ifdef MIGHT_HAVE_POSIX_THREAD_GCT
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
#ifdef MIGHT_HAVE_POSIX_REALTIME_GRT
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
