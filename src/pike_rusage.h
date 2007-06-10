/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: pike_rusage.h,v 1.19 2007/06/10 19:02:10 mast Exp $
*/

#ifndef PIKE_RUSAGE_H
#define PIKE_RUSAGE_H

#include "global.h"

#ifdef HAVE_TIME_H
#include <time.h>
#endif

#if defined (_POSIX_TIMERS)
#if _POSIX_TIMERS > 0

#ifdef _POSIX_THREAD_CPUTIME
#  if _POSIX_THREAD_CPUTIME != -1
#    if _POSIX_THREAD_CPUTIME != 0
/*     Know it to be available at compile time. */
#      define HAVE_POSIX_THREAD_GCT
#    else
/*     Might be available at run time - have to check with sysconf(3). */
#    endif
#    define MIGHT_HAVE_POSIX_THREAD_GCT
#  endif
#endif

#ifdef _POSIX_CPUTIME
#  if _POSIX_CPUTIME != -1
#    if _POSIX_CPUTIME != 0
#      define HAVE_POSIX_PROCESS_GCT
#    endif
#    define MIGHT_HAVE_POSIX_PROCESS_GCT
#  endif
#endif

#ifdef _POSIX_MONOTONIC_CLOCK
#  if _POSIX_MONOTONIC_CLOCK != -1
#    if _POSIX_MONOTONIC_CLOCK != 0
#      define HAVE_POSIX_MONOTONIC_GRT
#    endif
#    define MIGHT_HAVE_POSIX_MONOTONIC_GRT
#  endif
#endif

/* The POSIX CLOCK_REALTIME clock is guaranteed to exist if
 * _POSIX_TIMERS exist. */
#define HAVE_POSIX_REALTIME_GRT
#define MIGHT_HAVE_POSIX_REALTIME_GRT

#endif	/* _POSIX_TIMERS > 0 */
#endif	/* _POSIX_TIMERS */

#ifdef CONFIGURE_TEST_FALLBACK_GCT
/* In the configure test that tries to figure out whether the fallback
 * get_cpu_time is thread local or not. */
#  define fallback_gct get_cpu_time
#  define GCT_IS_FALLBACK
#else

/* Choose get_cpu_time implementation. Prefer one with thread local time. */
#  ifdef HAVE_POSIX_THREAD_GCT
#    define cpu_time_is_thread_local 1
#    define posix_thread_gct_impl get_cpu_time_impl
#    define posix_thread_gct get_cpu_time
#    define posix_thread_gct_res get_cpu_time_res
#    define GCT_IS_POSIX_THREAD
#  elif FB_CPU_TIME_IS_THREAD_LOCAL == PIKE_YES
#    define cpu_time_is_thread_local 1
#    define fallback_gct_impl get_cpu_time_impl
#    define fallback_gct get_cpu_time
#    define fallback_gct_res get_cpu_time_res
#    define GCT_IS_FALLBACK
#  elif defined (MIGHT_HAVE_POSIX_THREAD_GCT)
#    define GCT_RUNTIME_CHOICE
#  elif defined (HAVE_POSIX_PROCESS_GCT)
#    define cpu_time_is_thread_local 0
#    define posix_process_gct_impl get_cpu_time_impl
#    define posix_process_gct get_cpu_time
#    define posix_process_gct_res get_cpu_time_res
#    define GCT_IS_POSIX_PROCESS
#  elif defined (MIGHT_HAVE_POSIX_PROCESS_GCT)
#    define GCT_RUNTIME_CHOICE
#  else
#    define cpu_time_is_thread_local 0
#    define fallback_gct_impl get_cpu_time_impl
#    define fallback_gct get_cpu_time
#    define fallback_gct_res get_cpu_time_res
#    define GCT_IS_FALLBACK
#  endif

#ifdef GCT_RUNTIME_CHOICE
#  define CPU_TIME_MIGHT_BE_THREAD_LOCAL
#  define CPU_TIME_MIGHT_NOT_BE_THREAD_LOCAL
#elif cpu_time_is_thread_local == 1
#  define CPU_TIME_MIGHT_BE_THREAD_LOCAL
#else
#  define CPU_TIME_MIGHT_NOT_BE_THREAD_LOCAL
#endif

#endif	/* !CONFIGURE_TEST_FALLBACK_GCT */

/* Choose get_real_time implementation. Prefer one that isn't affected
 * by wall clock adjustments. */
#ifdef HAVE_POSIX_MONOTONIC_GRT
#  define real_time_is_monotonic 1
#  define posix_monotonic_grt_impl get_real_time_impl
#  define posix_monotonic_grt get_real_time
#  define posix_monotonic_grt_res get_real_time_res
#  define GRT_IS_POSIX_MONOTONIC
#elif defined (MIGHT_HAVE_POSIX_MONOTONIC_GRT)
#  define GRT_RUNTIME_CHOICE
#elif defined (HAVE_POSIX_REALTIME_GRT)
#  define real_time_is_monotonic 0
#  define posix_realtime_grt_impl get_real_time_impl
#  define posix_realtime_grt get_real_time
#  define posix_realtime_grt_res get_real_time_res
#  define GRT_IS_POSIX_REALTIME
#else
#  define fallback_grt_is_monotonic real_time_is_monotonic
#  define fallback_grt_impl get_real_time_impl
#  define fallback_grt get_real_time
#  define fallback_grt_res get_real_time_res
#  define GRT_IS_FALLBACK
#endif

#ifdef HAVE_TIMES
extern long pike_clk_tck;
#endif

/* Prototypes begin here */
typedef long pike_rusage_t[29];
PMOD_EXPORT int pike_get_rusage(pike_rusage_t rusage_values);
long *low_rusage(void);

/* get_cpu_time returns the consumed cpu time (both in kernel and user
 * space, if applicable), or -1 if it couldn't be read. Note that many
 * systems have fairly poor resolution. gettimeofday can therefore be
 * a better choice to measure small time intervals. */
#ifdef INT64
/* The time is returned in nanoseconds. */
typedef INT64 cpu_time_t;
#define LONG_CPU_TIME
#define CPU_TIME_TICKS /* per second */ 1000000000LL
#define CPU_TIME_UNIT "ns"
#define PRINT_CPU_TIME PRINTINT64 "d"
#else
/* The time is returned in milliseconds. (Note that the value will
 * wrap after about 49 days.) */
typedef unsigned long cpu_time_t;
#define CPU_TIME_TICKS /* per second */ 1000
#define CPU_TIME_UNIT "ms"
#define PRINT_CPU_TIME "lu"
#endif

#ifdef GCT_RUNTIME_CHOICE
PMOD_EXPORT extern int cpu_time_is_thread_local;
PMOD_EXPORT extern const char *get_cpu_time_impl;
PMOD_EXPORT extern cpu_time_t (*get_cpu_time) (void);
PMOD_EXPORT extern cpu_time_t (*get_cpu_time_res) (void);
#else
PMOD_EXPORT extern const char get_cpu_time_impl[];
PMOD_EXPORT cpu_time_t get_cpu_time (void);
PMOD_EXPORT cpu_time_t get_cpu_time_res (void);
#endif

#ifdef GRT_RUNTIME_CHOICE
PMOD_EXPORT extern int real_time_is_monotonic;
PMOD_EXPORT extern const char *get_real_time_impl;
PMOD_EXPORT extern cpu_time_t (*get_real_time) (void);
PMOD_EXPORT extern cpu_time_t (*get_real_time_res) (void);
#else
#ifdef GRT_IS_FALLBACK
PMOD_EXPORT extern int real_time_is_monotonic;
#endif
PMOD_EXPORT extern const char get_real_time_impl[];
PMOD_EXPORT cpu_time_t get_real_time(void);
PMOD_EXPORT cpu_time_t get_real_time_res (void);
#endif

INT32 internal_rusage(void);	/* For compatibility. */

#if defined(PIKE_DEBUG) || defined(INTERNAL_PROFILING)
void debug_print_rusage(FILE *out);
#endif

void init_rusage (void);
/* Prototypes end here */

#endif /* !PIKE_RUSAGE_H */
