/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: rusage.c,v 1.43 2004/12/30 13:46:22 grubba Exp $
*/

#include "global.h"
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
#ifdef HAVE_TIME_H
#include <time.h>
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
#include "pike_error.h"
#endif

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
int pike_get_rusage(pike_rusage_t rusage_values)
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

int pike_get_rusage(pike_rusage_t rusage_values)
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

int pike_get_rusage(pike_rusage_t rusage_values)
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

int pike_get_rusage(pike_rusage_t rusage_values)
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

int pike_get_rusage(pike_rusage_t rusage_values)
{
  MEMSET(rusage_values, 0, sizeof(pike_rusage_t));
  rusage_values[0]= CONVERT_TIME (clock(), CLOCKS_PER_SEC, 1000);
  return 1;
}

#else /* HAVE_CLOCK */

int pike_get_rusage(pike_rusage_t rusage_values)
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
 * Fix a good get_cpu_time.
 */

#ifdef __NT__

cpu_time_t get_cpu_time (void)
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

cpu_time_t get_real_time (void)
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
    return exitTime.ft_scalar * 100;
  else
    return (cpu_time_t) -1;
}

#elif defined (HAVE_WORKING_GETHRVTIME)

cpu_time_t get_cpu_time (void)
{
  /* Faster than CONVERT_TIME(gethrvtime(), 1000000000, CPU_TIME_TICKS)
   * It works under the assumtion that CPU_TIME_TICKS <= 1000000000
   * and that 1000000000 % CPU_TIME_TICKS == 0. */
  return gethrvtime() / (1000000000 / CPU_TIME_TICKS);
}

cpu_time_t get_real_time (void)
{
  return gethrtime() * (CPU_TIME_TICKS / 1000000000);
}

#else

#if defined (GETRUSAGE_THROUGH_PROCFS)

cpu_time_t get_cpu_time (void)
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

cpu_time_t get_cpu_time (void)
{
  struct tms tms;
#if defined (PIKE_DEBUG) && !defined (CONFIGURE_TEST)
  if (!pike_clk_tck) Pike_error ("Called before init_pike.\n");
#endif
  if (times (&tms) == (clock_t) -1)
    return (cpu_time_t) -1;
  return CONVERT_TIME (tms.tms_utime + tms.tms_stime, pike_clk_tck, CPU_TIME_TICKS);
}

#elif defined (HAVE_CLOCK) && defined (CLOCKS_PER_SEC)

cpu_time_t get_cpu_time (void)
{
  clock_t t = clock();
  if (t == (clock_t) -1)
    return (cpu_time_t) -1;
  else
    return CONVERT_TIME (t, CLOCKS_PER_SEC, CPU_TIME_TICKS);
}

#elif defined (HAVE_GETRUSAGE)

cpu_time_t get_cpu_time (void)
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

#else

cpu_time_t get_cpu_time (void)
{
  return (cpu_time_t) -1;
}

#endif

cpu_time_t get_real_time(void)
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
