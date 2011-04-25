/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#include "global.h"
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif
#include <sys/stat.h>
#include "time_stuff.h"
#include <fcntl.h>
#include <errno.h>
#include "rusage.h"

RCSID("$Id$");

#ifdef HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "fd_control.h"

/*
 * Here comes a long blob with stuff to see how to find out about
 * cpu usage.
 */

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
      rusage_values[0] = (INT32)userTime.ft_scalar/10000;  /* user time */
      rusage_values[1] = (INT32)kernelTime.ft_scalar/10000;  /* system time */
    }
  else
    {
      rusage_values[0] = 0;  /* user time */
      rusage_values[1] = 0;  /* system time */
    }
  return 1;
}

#else /* __NT__ */
#ifdef GETRUSAGE_THROUGH_PROCFS
#include <sys/procfs.h>
#include "fdlib.h"

static INLINE int get_time_int(timestruc_t * val) 
{
  return val->tv_sec * 1000 + val->tv_nsec / 1000000;
}

int proc_fd = -1;

int pike_get_rusage(pike_rusage_t rusage_values)
{
  prusage_t  pru;
#ifdef GETRUSAGE_THROUGH_PROCFS_PRS
  prstatus_t prs;
#endif
  MEMSET(rusage_values, 0, sizeof(pike_rusage_t));

  while(proc_fd < 0)
  {
    char proc_name[30];

    sprintf(proc_name, "/proc/%05ld", (long)getpid());
    proc_fd = open(proc_name, O_RDONLY);
    if(proc_fd >= 0) break;
    if(errno != EINTR) return 0;
  }

  set_close_on_exec(proc_fd, 1);

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

  utime = rus.ru_utime.tv_sec * 1000 + rus.ru_utime.tv_usec / 1000;
  stime = rus.ru_stime.tv_sec * 1000 + rus.ru_stime.tv_usec / 1000;

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

#if defined(HAVE_TIMES) && (defined(CLK_TCK) || defined(_SC_CLK_TCK))
#ifndef CLK_TCK
#define CLK_TCK sysconf(_SC_CLK_TCK)
#endif

#define NEED_CONVERT_TIME
static long convert_time(long t,long tick);
int pike_get_rusage(pike_rusage_t rusage_values)
{
  struct tms tms;
  MEMSET(rusage_values, 0, sizeof(pike_rusage_t));
  rusage_values[18] = convert_time(times(&tms), CLK_TCK);
  rusage_values[0] = convert_time(tms.tms_utime, CLK_TCK);
  rusage_values[1] = convert_time(tms.tms_utime, CLK_TCK);

  return 1;
}
#else /*HAVE_TIMES */
#if defined(HAVE_CLOCK) && defined(CLOCKS_PER_SECOND)

#define NEED_CONVERT_TIME
static long convert_time(long t,long tick);
int pike_get_rusage(pike_rusage_t rusage_values)
{
  MEMSET(rusage_values, 0, sizeof(pike_rusage_t));
  rusage_values[0]= convert_time(clock(), CLOCKS_PER_SECOND);
  return 1;
}

#else /* HAVE_CLOCK */

int pike_get_rusage(pike_rusage_t rusage_values)
{
  /* This is totally wrong, but hey, if you can't do it _right_... */
  struct timeval tm;
  MEMSET(rusage_values, 0, sizeof(pike_rusage_t));
  GETTIMEOFDAY(&tm);
  rusage_values[0]=tm.tv_sec*1000 + tm.tv_usec/1000;
  return 1;
}
#endif /* HAVE_CLOCK */
#endif /* HAVE_TIMES */
#endif /* HAVE_GETRUSAGE */
#endif /* GETRUSAGE_THROUGH_PROCFS */
#endif /* __NT__ */


#ifdef NEED_CONVERT_TIME
/*
 * Here's a trick to do t * 1000 / tick without
 * causing arethmic overflow
 */
static long convert_time(long t,long tick)
{
  return (t / tick) * 1000 + (t % tick) * 1000 / tick;
}
#endif

INT32 *low_rusage(void)
{
  static pike_rusage_t rusage_values;
  if (pike_get_rusage (rusage_values))
    return &rusage_values[0];
  else
    return NULL;
}

INT32 internal_rusage(void)
{
  pike_rusage_t rusage_values;
  pike_get_rusage (rusage_values);
  return rusage_values[0];
}

#if defined(PIKE_DEBUG) || defined(INTERNAL_PROFILING)
void debug_print_rusage(FILE *out)
{
  pike_rusage_t rusage_values;
  pike_get_rusage (rusage_values);
  fprintf (out,
	   " [utime: %d, stime: %d] [maxrss: %d, ixrss: %d, idrss: %d, isrss: %d]\n"
	   " [minflt: %d, majflt: %d] [nswap: %d] [inblock: %d, oublock: %d]\n"
	   " [msgsnd: %d, msgrcv: %d] [nsignals: %d] [nvcsw: %d, nivcsw: %d]\n"
	   " [sysc: %d] [ioch: %d] [rtime: %d, ttime: %d]\n"
	   " [tftime: %d, dftime: %d, kftime: %d, ltime: %d, slptime: %d]\n"
	   " [wtime: %d, stoptime: %d] [brksize: %d, stksize: %d]\n",
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
