/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: pike_rusage.h,v 1.17 2004/05/19 20:57:04 grubba Exp $
*/

#ifndef PIKE_RUSAGE_H
#define PIKE_RUSAGE_H

#ifdef HAVE_TIMES
extern long pike_clk_tck;
#define init_rusage() (pike_clk_tck = sysconf (_SC_CLK_TCK))
#else
#define init_rusage()
#endif

/* Prototypes begin here */
typedef long pike_rusage_t[29];
int pike_get_rusage(pike_rusage_t rusage_values);
long *low_rusage(void);

/* get_cpu_time returns the consumed cpu time (both in kernel and user
 * space, if applicable), or -1 if it couldn't be read. Note that
 * many systems have fairly poor resolution, e.g. on Linux x86 it's
 * only 0.01 second. gettimeofday can therefore be a better choice to
 * measure small time intervals. */
#ifdef INT64
/* The time is returned in nanoseconds. */
typedef INT64 cpu_time_t;
#define LONG_CPU_TIME
#define CPU_TIME_TICKS /* per second */ ((cpu_time_t) 1000000000)
#define CPU_TIME_UNIT "ns"
#define PRINT_CPU_TIME PRINTINT64 "d"
#else
/* The time is returned in milliseconds. (Note that the value will
 * wrap after about 49 days.) */
typedef unsigned long cpu_time_t;
#define CPU_TIME_TICKS /* per second */ ((cpu_time_t) 1000)
#define CPU_TIME_UNIT "ms"
#define PRINT_CPU_TIME "lu"
#endif
cpu_time_t get_cpu_time (void);
cpu_time_t get_real_time(void);

INT32 internal_rusage(void);	/* For compatibility. */

#if defined(PIKE_DEBUG) || defined(INTERNAL_PROFILING)
void debug_print_rusage(FILE *out);
#endif
/* Prototypes end here */

#endif /* !PIKE_RUSAGE_H */
