/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <sys/stat.h>
#include "time_stuff.h"
#include <fcntl.h>
#include <errno.h>
#include "rusage.h"

#ifdef HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

static INT32 rusage_values[30];
/*
 * Here comes a long blob with stuff to see how to find out about
 * cpu usage.
 */

#ifdef GETRUSAGE_THROUGH_PROCFS
#include <sys/procfs.h>
static INLINE int get_time_int(timestruc_t * val) 
{
  return val->tv_sec * 1000 + val->tv_nsec / 1000000;
}

int proc_fd = -1;

INT32 *low_rusage()
{
  prusage_t  pru;
  prstatus_t prs;

  while(proc_fd < 0)
  {
    char proc_name[30];

    sprintf(proc_name, "/proc/%05ld", (long)getpid());
    proc_fd = open(proc_name, O_RDONLY);
    if(proc_fd >= 0) break;
    if(errno != EINTR) return 0;
  }

  while(ioctl(proc_fd, PIOCUSAGE, &pru) < 0)
  {
    if(errno == EINTR)
      continue;

    return 0;
  }

  while(ioctl(proc_fd, PIOCSTATUS, &prs) < 0)
  {
    if(errno == EINTR)
      continue;

    return 0;
  }

  rusage_values[0] = get_time_int(&pru.pr_utime);  /* user time */
  rusage_values[1] = get_time_int(&pru.pr_stime);  /* system time */
  rusage_values[2] = 0;                           /* maxrss */
  rusage_values[3] = 0;                           /* idrss */
  rusage_values[4] = 0;                           /* isrss */
  rusage_values[5] = 0;                           /* minflt */
  rusage_values[6] = pru.pr_minf;           /* minor pagefaults */
  rusage_values[7] = pru.pr_majf;           /* major pagefaults */
  rusage_values[8] = pru.pr_nswap;          /* swaps */
  rusage_values[9] = pru.pr_inblk;          /* block input op. */
  rusage_values[10] = pru.pr_oublk;         /* block outout op. */
  rusage_values[11] = pru.pr_msnd;          /* messages sent */
  rusage_values[12] = pru.pr_mrcv;          /* messages received */
  rusage_values[13] = pru.pr_sigs;          /* signals received */
  rusage_values[14] = pru.pr_vctx;          /* voluntary context switches */
  rusage_values[15] = pru.pr_ictx;         /* involuntary  "        " */
  rusage_values[16] = pru.pr_sysc;
  rusage_values[17] = pru.pr_ioch;
  rusage_values[18] = get_time_int(&pru.pr_rtime);
  rusage_values[19] = get_time_int(&pru.pr_ttime);
  rusage_values[20] = get_time_int(&pru.pr_tftime);
  rusage_values[21] = get_time_int(&pru.pr_dftime);
  rusage_values[22] = get_time_int(&pru.pr_kftime);
  rusage_values[23] = get_time_int(&pru.pr_ltime);
  rusage_values[24] = get_time_int(&pru.pr_slptime);
  rusage_values[25] = get_time_int(&pru.pr_wtime);
  rusage_values[26] = get_time_int(&pru.pr_stoptime);
  rusage_values[27] = prs.pr_brksize;
  rusage_values[28] = prs.pr_stksize;

  return & rusage_values[0];
}

#else /* GETRUSAGE_THROUGH_PROCFS */
#ifdef HAVE_GETRUSAGE
#include <sys/resource.h>
#ifdef HAVE_SYS_RUSAGE
#include <sys/rusage.h>
#endif

INT32 *low_rusage()
{
  struct rusage rus;
  long utime, stime;
  int maxrss;

  if (getrusage(RUSAGE_SELF, &rus) < 0) return 0;

  utime = rus.ru_utime.tv_sec * 1000 + rus.ru_utime.tv_usec / 1000;
  stime = rus.ru_stime.tv_sec * 1000 + rus.ru_stime.tv_usec / 1000;

  maxrss = rus.ru_maxrss;
#ifdef sun
  maxrss *= getpagesize() / 1024;
#endif
  rusage_values[0] = utime;
  rusage_values[1] = stime;
  rusage_values[2] = maxrss;
#ifndef GETRUSAGE_RESTRICTED
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
  return rusage_values;
}
#else /* HAVE_GETRUSAGE */

#if defined(HAVE_TIMES) && (defined(CLK_TCK) || defined(_SC_CLK_TCK))
#ifndef CLK_TCK
#define CLK_TCK sysconf(_SC_CLK_TCK)
#endif

#define NEED_CONVERT_TIME
static long convert_time(long t,long tick);
INT32 *low_rusage()
{
  struct tms tms;
  rusage_values[18] = convert_time(times(&tms), CLK_TCK);
  rusage_values[0] = convert_time(tms.tms_utime, CLK_TCK);
  rusage_values[1] = convert_time(tms.tms_utime, CLK_TCK);

  return rusage_values;
}
#else /*HAVE_TIMES */
#if defined(HAVE_CLOCK) && defined(CLOCKS_PER_SECOND)

#define NEED_CONVERT_TIME
static long convert_time(long t,long tick);
INT32 *low_rusage()
{
  rusage_values[0]= convert_time(clock(), CLOCKS_PER_SECOND);
  return rusage_values;
}

#else /* HAVE_CLOCK */

INT32 *low_rusage()
{
  /* This is totally wrong, but hey, if you can't do it _right_... */
  struct timeval tm;
  GETTIMEOFDAY(&tm);
  rusage_values[0]=tm.tv_sec*1000 + tm.tv_usec/1000;
  return rusage_values;
}
#endif /* HAVE_CLOCK */
#endif /* HAVE_TIMES */
#endif /* HAVE_GETRUSAGE */
#endif /* GETRUSAGE_THROUGH_PROCFS */


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

INT32 internal_rusage()
{
  low_rusage();
  return rusage_values[0];
}
