/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"

/* #define PICKY_MUTEX */

#include "pike_error.h"

/* Define to get a debug trace of thread operations. Debug levels can be 0-2. */
/* #define VERBOSE_THREADS_DEBUG 1 */

/* #define PROFILE_CHECK_THREADS */

#ifndef CONFIGURE_TEST
#define IN_THREAD_CODE
#include "threads.h"
#include "array.h"
#include "mapping.h"
#include "object.h"
#include "pike_macros.h"
#include "callback.h"
#include "builtin_functions.h"
#include "constants.h"
#include "program.h"
#include "program_id.h"
#include "gc.h"
#include "main.h"
#include "module_support.h"
#include "pike_types.h"
#include "operators.h"
#include "bignum.h"
#include "signal_handler.h"
#include "backend.h"
#include "pike_rusage.h"
#include "pike_cpulib.h"

#include <errno.h>
#include <math.h>

#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif /* HAVE_SYS_PRCTL_H */

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif

/* This is used for strapping the interpreter before the threads
 * are loaded, and when there's no support for threads.
 */
static struct Pike_interpreter_struct static_pike_interpreter;

static cpu_time_t thread_quanta = 0;

PMOD_EXPORT struct Pike_interpreter_struct *
#if defined(__GNUC__) && __GNUC__ >= 3
    __restrict
#endif
Pike_interpreter_pointer = &static_pike_interpreter;

PMOD_EXPORT struct Pike_interpreter_struct * pike_get_interpreter_pointer(void)
{
    return Pike_interpreter_pointer;
}
#else  /* CONFIGURE_TEST */
#include "pike_threadlib.h"
#endif

#ifdef _REENTRANT

#ifndef VERBOSE_THREADS_DEBUG
#define THREADS_FPRINTF(LEVEL,FPRINTF_ARGS)
#else
#define THREADS_FPRINTF(LEVEL,FPRINTF_ARGS) do {			\
    if ((VERBOSE_THREADS_DEBUG + 0) >= (LEVEL)) {			\
      /* E.g. THREADS_DISALLOW is used in numerous places where the */	\
      /* value in errno must not be clobbered. */			\
      int saved_errno = errno;						\
      fprintf (stderr, "[%"PRINTSIZET"x] ", (size_t) th_self());	\
      fprintf FPRINTF_ARGS;						\
      errno = saved_errno;						\
    }									\
  } while(0)
#endif /* VERBOSE_THREADS_DEBUG */

#ifndef PIKE_THREAD_C_STACK_SIZE
#define PIKE_THREAD_C_STACK_SIZE (256 * 1024)
#endif

PMOD_EXPORT size_t thread_stack_size=PIKE_THREAD_C_STACK_SIZE;

PMOD_EXPORT void thread_low_error (int errcode, const char *cmd,
				   const char *fname, int lineno)
{
#ifdef CONFIGURE_TEST
  fprintf (stderr, "%s:%d: %s\n"
	   "Unexpected error from thread function: %d\n",
	   fname, lineno, cmd, errcode);
  abort();
#else
  debug_fatal ("%s:%d: Fatal error: %s\n"
	       "Unexpected error from thread function: %d\n",
	       fname, lineno, cmd, errcode);
#endif
}

/* SCO magic... */
int  __thread_sys_behavior = 1;

#ifndef CONFIGURE_TEST

#if !defined(HAVE_PTHREAD_ATFORK) && !defined(th_atfork)
#include "callback.h"
#define PIKE_USE_OWN_ATFORK


static struct callback_list atfork_prepare_callback;
static struct callback_list atfork_parent_callback;
static struct callback_list atfork_child_callback;

int th_atfork(void (*prepare)(void),void (*parent)(void),void (*child)(void))
{
  if(prepare)
    add_to_callback(&atfork_prepare_callback, (callback_func) prepare, 0, 0);
  if(parent)
    add_to_callback(&atfork_parent_callback, (callback_func) parent, 0, 0);
  if(child)
    add_to_callback(&atfork_child_callback, (callback_func) child, 0, 0);
  return 0;
}
void th_atfork_prepare(void)
{
  call_callback(& atfork_prepare_callback, 0);
}
void th_atfork_parent(void)
{
  call_callback(& atfork_parent_callback, 0);
}
void th_atfork_child(void)
{
  call_callback(& atfork_child_callback, 0);
}
#endif

#endif	/* !CONFIGURE_TEST */

#ifdef __NT__

PMOD_EXPORT int low_nt_create_thread(unsigned Pike_stack_size,
				     unsigned (TH_STDCALL *fun)(void *),
				     void *arg,
				     unsigned *id)
{
  HANDLE h = (HANDLE)_beginthreadex(NULL, Pike_stack_size, fun, arg, 0, id);
  if(h)
  {
    CloseHandle(h);
    return 0;
  }
  else
  {
    return errno;
  }
}

#endif

#ifdef SIMULATE_COND_WITH_EVENT
PMOD_EXPORT int co_wait(COND_T *c, MUTEX_T *m)
{
  struct cond_t_queue me;
  event_init(&me.event);
  me.next=0;
  mt_lock(& c->lock);

  if(c->tail)
  {
    c->tail->next=&me;
    c->tail=&me;
  }else{
    c->head=c->tail=&me;
  }

  mt_unlock(& c->lock);
  mt_unlock(m);

  /* NB: No race here since the event is manually reset. */

  event_wait(&me.event);
  mt_lock(m);

  event_destroy(& me.event);
  /* Cancellation point?? */

#ifdef PIKE_DEBUG
  if(me.next)
    Pike_fatal("Wait on event return prematurely!!\n");
#endif

  return 0;
}

PMOD_EXPORT int co_wait_timeout(COND_T *c, MUTEX_T *m, long s, long nanos)
{
  struct cond_t_queue me;
  event_init(&me.event);
  me.next=0;
  mt_lock(& c->lock);

  if(c->tail)
  {
    c->tail->next=&me;
    c->tail=&me;
  }else{
    c->head=c->tail=&me;
  }

  mt_unlock(& c->lock);
  mt_unlock(m);
  if (s || nanos) {
    DWORD msec;
    /* Check for overflow (0xffffffff/1000). */
    if (s >= 4294967) {
      msec = INFINITE;
    } else {
      msec = s*1000 + nanos/1000000;
      if (!msec) msec = 1;	/* Underflow. */
    }
    event_wait_msec(&me.event, msec);
  } else {
    event_wait(&me.event);
  }
  mt_lock(m);

  event_destroy(& me.event);
  /* Cancellation point?? */

#ifdef PIKE_DEBUG
  if(me.next)
    Pike_fatal("Wait on event return prematurely!!\n");
#endif

  return 0;
}

PMOD_EXPORT int co_signal(COND_T *c)
{
  struct cond_t_queue *t;
  mt_lock(& c->lock);
  if((t=c->head))
  {
    c->head=t->next;
    t->next=0;
    if(!c->head) c->tail=0;
  }
  mt_unlock(& c->lock);
  if(t)
    event_signal(& t->event);
  return 0;
}

PMOD_EXPORT int co_broadcast(COND_T *c)
{
  struct cond_t_queue *t,*n;
  mt_lock(& c->lock);
  n=c->head;
  c->head=c->tail=0;
  mt_unlock(& c->lock);

  while((t=n))
  {
    n=t->next;
    t->next=0;
    event_signal(& t->event);
  }

  return 0;
}

PMOD_EXPORT int co_destroy(COND_T *c)
{
  struct cond_t_queue *t;
  mt_lock(& c->lock);
  t=c->head;
  mt_unlock(& c->lock);
  if(t) return -1;
  mt_destroy(& c->lock);
  return 0;
}

#else /* !SIMULATE_COND_WITH_EVENT */

#ifndef CONFIGURE_TEST
PMOD_EXPORT int co_wait_timeout(COND_T *c, PIKE_MUTEX_T *m, long s, long nanos)
{
  struct timeval ct;
#ifdef POSIX_THREADS
  struct timespec timeout;
#ifdef HAVE_PTHREAD_COND_RELTIMEDWAIT_NP
  /* Solaris extension: relative timeout. */
  timeout.tv_sec = s;
  timeout.tv_nsec = nanos;
  return pthread_cond_reltimedwait_np(c, m, &timeout);
#else /* !HAVE_PTHREAD_COND_RELTIMEDWAIT_NP */
  /* Absolute timeout. */
  ACCURATE_GETTIMEOFDAY(&ct);
  timeout.tv_sec = ct.tv_sec + s;
  timeout.tv_nsec = ct.tv_usec * 1000 + nanos;
  s = timeout.tv_nsec/1000000000;
  if (s) {
    timeout.tv_sec += s;
    timeout.tv_nsec -= s * 1000000000;
  }
  return pthread_cond_timedwait(c, m, &timeout);
#endif /* HAVE_PTHREAD_COND_RELTIMEDWAIT_NP */
#else /* !POSIX_THREADS */
#error co_wait_timeout does not support this thread model.
#endif /* POSIX_THREADS */
}
#endif /* !CONFIGURE_TEST */

#endif /* SIMULATE_COND_WITH_EVENT */

#ifdef POSIX_THREADS
pthread_attr_t pattr;
pthread_attr_t small_pattr;
#endif

static void really_low_th_init(void)
{
#ifdef SGI_SPROC_THREADS
#error /* Need to specify a filename */
  us_cookie = usinit("");
#endif /* SGI_SPROC_THREADS */

#ifdef POSIX_THREADS
#ifdef HAVE_PTHREAD_INIT
  pthread_init();
#endif /* HAVE_PTHREAD_INIT */
#endif /* POSIX_THREADS */

#ifdef POSIX_THREADS
  pthread_attr_init(&pattr);
#ifndef CONFIGURE_TEST
#ifdef HAVE_PTHREAD_ATTR_SETSTACKSIZE
  pthread_attr_setstacksize(&pattr, thread_stack_size);
#endif
#endif
  pthread_attr_setdetachstate(&pattr, PTHREAD_CREATE_DETACHED);

  pthread_attr_init(&small_pattr);
#ifdef HAVE_PTHREAD_ATTR_SETSTACKSIZE
  pthread_attr_setstacksize(&small_pattr, 4096*sizeof(char *));
#endif
  pthread_attr_setdetachstate(&small_pattr, PTHREAD_CREATE_DETACHED);
#endif
}

static void cleanup_thread_state (struct thread_state *th);

#ifndef CONFIGURE_TEST

#if defined (RDTSC) ||							\
     (!defined(HAVE_GETHRTIME) &&					\
      !(defined(HAVE_MACH_TASK_INFO_H) && defined(TASK_THREAD_TIMES_INFO)))
static clock_t thread_start_clock = 0;
static THREAD_T last_clocked_thread = 0;
#define USE_CLOCK_FOR_SLICES
#ifdef RDTSC
static int use_tsc_for_slices;
#define TSC_START_INTERVAL 100000
static INT64 prev_tsc;	   /* TSC and */
static clock_t prev_clock; /* clock() at the beg of the last tsc interval */
#endif
#endif

#define THIS_THREAD ((struct thread_state *)CURRENT_STORAGE)

static struct callback *threads_evaluator_callback=0;

PMOD_EXPORT int num_threads = 1;
PMOD_EXPORT int threads_disabled = 0;
PMOD_EXPORT cpu_time_t threads_disabled_acc_time = 0;
PMOD_EXPORT cpu_time_t threads_disabled_start = 0;

#ifdef PIKE_DEBUG
static THREAD_T threads_disabled_thread = 0;
#endif
#ifdef INTERNAL_PROFILING
PMOD_EXPORT unsigned long thread_yields = 0;
#endif
static int th_running = 0;
static MUTEX_T interpreter_lock;
static MUTEX_T interpreter_lock_wanted;
MUTEX_T thread_table_lock;
static MUTEX_T interleave_lock;
static struct program *mutex_key = 0;
PMOD_EXPORT struct program *thread_id_prog = 0;
static struct program *thread_local_prog = 0;
PMOD_EXPORT ptrdiff_t thread_storage_offset;
static int live_threads = 0;
static COND_T live_threads_change;
static COND_T threads_disabled_change;

struct thread_local_var
{
  INT32 id;
};

static volatile IMUTEX_T *interleave_list = NULL;

#define THREADSTATE2OBJ(X) ((X)->thread_obj)

#if defined(PIKE_DEBUG)

/* This is a debug wrapper to enable checks that the interpreter lock
 * is held by the current thread. */
static int debug_is_locked;
static THREAD_T debug_locking_thread;
#define SET_LOCKING_THREAD (debug_is_locked = 1,			\
			    debug_locking_thread = th_self())
#define UNSET_LOCKING_THREAD (debug_is_locked = 0)
static INLINE void check_interpreter_lock (DLOC_DECL)
{
  if (th_running) {
    THREAD_T self;
    if (!debug_is_locked)
      pike_fatal_dloc ("Interpreter not locked.\n");
    self = th_self();
    if (!th_equal (debug_locking_thread, self))
      pike_fatal_dloc ("Interpreter not locked by this thread.\n");
  }
}

static unsigned LONGEST thread_swaps = 0;
static unsigned LONGEST check_threads_calls = 0;
static unsigned LONGEST check_threads_yields = 0;
static unsigned LONGEST check_threads_swaps = 0;
static void f__thread_swaps (INT32 UNUSED(args))
  {push_ulongest (thread_swaps);}
static void f__check_threads_calls (INT32 UNUSED(args))
  {push_ulongest (check_threads_calls);}
static void f__check_threads_yields (INT32 UNUSED(args))
  {push_ulongest (check_threads_yields);}
static void f__check_threads_swaps (INT32 UNUSED(args))
  {push_ulongest (check_threads_swaps);}

#else

#define SET_LOCKING_THREAD 0
#define UNSET_LOCKING_THREAD 0

#endif

PMOD_EXPORT INLINE void pike_low_lock_interpreter (DLOC_DECL)
{
  /* The double locking here is to ensure that when a thread releases
   * the interpreter lock, a different thread gets it first. Thereby
   * we ensure a thread switch in check_threads, if there are other
   * threads waiting. */
  mt_lock (&interpreter_lock_wanted);
  mt_lock (&interpreter_lock);
  mt_unlock (&interpreter_lock_wanted);

  SET_LOCKING_THREAD;
  USE_DLOC_ARGS();
  THREADS_FPRINTF (1, (stderr, "Got iplock" DLOC_PF(" @ ",) "\n"
		       COMMA_DLOC_ARGS_OPT));
}

PMOD_EXPORT INLINE void pike_low_wait_interpreter (COND_T *cond COMMA_DLOC_DECL)
{
  USE_DLOC_ARGS();
  THREADS_FPRINTF (1, (stderr,
		       "Waiting on cond %p without iplock" DLOC_PF(" @ ",) "\n",
		       cond COMMA_DLOC_ARGS_OPT));
  UNSET_LOCKING_THREAD;

  /* FIXME: Should use interpreter_lock_wanted here as well. The
   * problem is that few (if any) thread libs lets us atomically
   * unlock a mutex and wait, and then lock a different mutex. */
  co_wait (cond, &interpreter_lock);

  SET_LOCKING_THREAD;
  THREADS_FPRINTF (1, (stderr,
		       "Got signal on cond %p with iplock" DLOC_PF(" @ ",) "\n",
		       cond COMMA_DLOC_ARGS_OPT));
}

PMOD_EXPORT INLINE int pike_low_timedwait_interpreter (COND_T *cond,
						       long sec, long nsec
						       COMMA_DLOC_DECL)
{
  int res;
  USE_DLOC_ARGS();
  THREADS_FPRINTF (1, (stderr,
		       "Waiting on cond %p without iplock" DLOC_PF(" @ ",) "\n",
		       cond COMMA_DLOC_ARGS_OPT));
  UNSET_LOCKING_THREAD;

  /* FIXME: Should use interpreter_lock_wanted here as well. The
   * problem is that few (if any) thread libs lets us atomically
   * unlock a mutex and wait, and then lock a different mutex. */
  res = co_wait_timeout (cond, &interpreter_lock, sec, nsec);

  SET_LOCKING_THREAD;
  THREADS_FPRINTF (1, (stderr,
		       "Got signal on cond %p with iplock" DLOC_PF(" @ ",) "\n",
		       cond COMMA_DLOC_ARGS_OPT));
  return res;
}

static void threads_disabled_wait (DLOC_DECL)
{
  assert (threads_disabled);
  USE_DLOC_ARGS();
  do {
    THREADS_FPRINTF (1, (stderr,
			 "Waiting on threads_disabled" DLOC_PF(" @ ",) "\n"
			 COMMA_DLOC_ARGS_OPT));
    UNSET_LOCKING_THREAD;
    co_wait (&threads_disabled_change, &interpreter_lock);
    SET_LOCKING_THREAD;
  } while (threads_disabled);
  THREADS_FPRINTF (1, (stderr,
		       "Continue after threads_disabled" DLOC_PF(" @ ",) "\n"
		       COMMA_DLOC_ARGS_OPT));
}

PMOD_EXPORT INLINE void pike_lock_interpreter (DLOC_DECL)
{
  pike_low_lock_interpreter (DLOC_ARGS_OPT);
  if (threads_disabled) threads_disabled_wait (DLOC_ARGS_OPT);
}

PMOD_EXPORT INLINE void pike_unlock_interpreter (DLOC_DECL)
{
  USE_DLOC_ARGS();
  THREADS_FPRINTF (1, (stderr, "Releasing iplock" DLOC_PF(" @ ",) "\n"
		       COMMA_DLOC_ARGS_OPT));
  UNSET_LOCKING_THREAD;
  mt_unlock (&interpreter_lock);
}

PMOD_EXPORT INLINE void pike_wait_interpreter (COND_T *cond COMMA_DLOC_DECL)
{
  int owner = threads_disabled;
  pike_low_wait_interpreter (cond COMMA_DLOC_ARGS_OPT);
  if (!owner && threads_disabled) threads_disabled_wait (DLOC_ARGS_OPT);
}

PMOD_EXPORT INLINE int pike_timedwait_interpreter (COND_T *cond,
						   long sec, long nsec
						   COMMA_DLOC_DECL)
{
  int owner = threads_disabled;
  int res = pike_low_timedwait_interpreter (cond, sec, nsec
					    COMMA_DLOC_ARGS_OPT);
  if (!owner && threads_disabled) threads_disabled_wait (DLOC_ARGS_OPT);
  return res;
}

PMOD_EXPORT void pike_init_thread_state (struct thread_state *ts)
{
  /* NB: Assumes that there's a temporary Pike_interpreter_struct
   *     in Pike_interpreter_pointer, which we copy, and replace
   *     with ourselves.
   */
  Pike_interpreter.thread_state = ts;
  ts->state = Pike_interpreter;
  Pike_interpreter_pointer = &ts->state;
  ts->id = th_self();
  ts->busy_prev = ts->busy_next = NULL;
  ts->status = THREAD_RUNNING;
  ts->swapped = 0;
  ts->interval_start = get_real_time();
#ifdef PIKE_DEBUG
  ts->debug_flags = 0;
  thread_swaps++;
#endif
#ifdef USE_CLOCK_FOR_SLICES
  /* Initialize thread_start_clock to zero instead of clock() since we
   * don't know for how long the thread already has run. */
  thread_start_clock = 0;
  last_clocked_thread = ts->id;
#ifdef RDTSC
  prev_tsc = 0;
#endif
#ifdef PROFILE_CHECK_THREADS
  fprintf (stderr, "[%d:%f] pike_init_thread_state: tsc reset\n",
	   getpid(), get_real_time() * (1.0 / CPU_TIME_TICKS));
#endif
#endif
}

PMOD_EXPORT void pike_swap_out_thread (struct thread_state *ts
				       COMMA_DLOC_DECL)
{
  USE_DLOC_ARGS();
  THREADS_FPRINTF (2, (stderr, "Swap out %sthread %p" DLOC_PF(" @ ",) "\n",
		       ts == Pike_interpreter.thread_state ? "current " : "",
		       ts
		       COMMA_DLOC_ARGS_OPT));

#ifdef PROFILING
  if (!ts->swapped) {
    cpu_time_t now = get_cpu_time();
#ifdef PROFILING_DEBUG
    fprintf(stderr, "%p: Swap out at: %" PRINT_CPU_TIME
	    " unlocked: %" PRINT_CPU_TIME "\n",
	    ts, now, ts->state.unlocked_time);
#endif
    ts->state.unlocked_time -= now;
  }
#endif

  ts->swapped=1;

  Pike_interpreter_pointer = NULL;
}

PMOD_EXPORT void pike_swap_in_thread (struct thread_state *ts
				      COMMA_DLOC_DECL)
{
  THREADS_FPRINTF (2, (stderr, "Swap in thread %p" DLOC_PF(" @ ",) "\n",
		       ts COMMA_DLOC_ARGS_OPT));

#ifdef PIKE_DEBUG
  if (Pike_interpreter_pointer)
    pike_fatal_dloc ("Thread %"PRINTSIZET"x swapped in "
		     "over existing thread %"PRINTSIZET"x.\n",
		     (size_t) ts->id,
		     (size_t) (Pike_interpreter.thread_state ?
			       Pike_interpreter.thread_state->id : 0));
#endif

#ifdef PROFILING
  if (ts->swapped) {
    cpu_time_t now = get_cpu_time();
#ifdef PROFILING_DEBUG
    fprintf(stderr, "%p: Swap in at: %" PRINT_CPU_TIME
	    " unlocked: %" PRINT_CPU_TIME "\n",
	    ts, now, ts->state.unlocked_time);
#endif
/* This will not work, since Pike_interpreter_pointer is always null here... */
/* #ifdef PIKE_DEBUG */
/*     if (now < -Pike_interpreter.unlocked_time) { */
/*           pike_fatal_dloc("Time at swap in is before time at swap out." */
/*                           " %" PRINT_CPU_TIME " < %" PRINT_CPU_TIME */
/*                           "\n", now, -Pike_interpreter.unlocked_time); */
/*     } */
/* #endif */
    ts->state.unlocked_time += now;
  }
#endif

  ts->swapped=0;
  Pike_interpreter_pointer = &ts->state;
#ifdef PIKE_DEBUG
  thread_swaps++;
#endif

#ifdef USE_CLOCK_FOR_SLICES
  if (last_clocked_thread != ts->id) {
    thread_start_clock = clock();
    last_clocked_thread = ts->id;
#ifdef RDTSC
    RDTSC (prev_tsc);
    prev_clock = thread_start_clock;
#endif
  }
#endif
}

PMOD_EXPORT void pike_swap_in_current_thread (struct thread_state *ts
					      COMMA_DLOC_DECL)
{
#ifdef PIKE_DEBUG
  THREAD_T self = th_self();
  if (!th_equal (ts->id, self))
    pike_fatal_dloc ("Swapped in thread state %p into wrong thread "
		     "%"PRINTSIZET"x - should be %"PRINTSIZET"x.\n",
		     ts, th_self(), ts->id);
#endif

  pike_swap_in_thread (ts COMMA_DLOC_ARGS_OPT);
}

#ifdef PIKE_DEBUG

PMOD_EXPORT void pike_assert_thread_swapped_in (DLOC_DECL)
{
  struct thread_state *ts=thread_state_for_id(th_self());
  if(ts->swapped)
    pike_fatal_dloc ("Thread is not swapped in.\n");
  if (ts->debug_flags & THREAD_DEBUG_LOOSE)
    pike_fatal_dloc ("Current thread is not bound to the interpreter. "
		     "Nested use of ALLOW_THREADS()?\n");
}

PMOD_EXPORT void pike_debug_check_thread (DLOC_DECL)
{
  struct thread_state *ts=thread_state_for_id(th_self());
  if (ts->debug_flags & THREAD_DEBUG_LOOSE)
    pike_fatal_dloc ("Current thread is not bound to the interpreter. "
		     "Nested use of ALLOW_THREADS()?\n");
  if(ts != Pike_interpreter.thread_state) {
    debug_list_all_threads();
    Pike_fatal ("Thread states mixed up between threads.\n");
  }
  check_interpreter_lock (DLOC_ARGS_OPT);
}

#endif	/* PIKE_DEBUG */

/*! @class MasterObject
 */

/*! @decl void thread_quanta_exceeded(Thread.Thread thread, int ns)
 *!
 *! Function called when a thread has exceeded the thread quanta.
 *!
 *! @param thread
 *!   Thread that exceeded the thread quanta.
 *!
 *! @param ns
 *!   Number of nanoseconds that the thread executed before allowing
 *!   other threads to run.
 *!
 *! The default master prints a diagnostic and the thread backtrace
 *! to @[Stdio.stderr].
 *!
 *! @note
 *!   This function runs in a signal handler context, and should thus
 *!   avoid handling of mutexes, etc.
 *!
 *! @seealso
 *!   @[get_thread_quanta()], @[set_thread_quanta()]
 */

/*! @endclass
 */

PMOD_EXPORT void pike_threads_allow (struct thread_state *ts COMMA_DLOC_DECL)
{
  /* Might get here after th_cleanup() when reporting leaks. */
  if (!ts) return;

  if (UNLIKELY(thread_quanta > 0)) {
    cpu_time_t now = get_real_time();

    if (UNLIKELY((now - ts->interval_start) > thread_quanta) &&
	LIKELY(ts->thread_obj)) {
      ref_push_object(ts->thread_obj);
      push_int64(now - ts->interval_start);
      ts->interval_start = now;
#ifndef LONG_CPU_TIME
      push_int(1000000000 / CPU_TIME_TICKS);
      o_multiply();
#endif
      SAFE_APPLY_MASTER("thread_quanta_exceeded", 2);
      pop_stack();
    }
  }

#ifdef PIKE_DEBUG
  pike_debug_check_thread (DLOC_ARGS_OPT);
  if (Pike_in_gc > 50 && Pike_in_gc < 300)
    pike_fatal_dloc ("Threads allowed during garbage collection (pass %d).\n",
		     Pike_in_gc);
  if (pike_global_buffer.s.str)
    pike_fatal_dloc ("Threads allowed while the global dynamic buffer "
		     "is in use.\n");
  ts->debug_flags |= THREAD_DEBUG_LOOSE;
#endif

  if (num_threads > 1 && !threads_disabled) {
    pike_swap_out_thread (ts COMMA_DLOC_ARGS_OPT);
    pike_unlock_interpreter (DLOC_ARGS_OPT);
  }

#if defined (PIKE_DEBUG) && !(defined(__ia64) && defined(__xlc__))
  else {
    /* Disabled in xlc 5.5.0.0/ia64 due to a code generation bug. */
    THREAD_T self = th_self();
    if (threads_disabled && !th_equal(threads_disabled_thread, self))
      pike_fatal_dloc ("Threads allowed from a different thread "
		       "while threads are disabled. "
		       "(self: %"PRINTSIZET"x, disabler: %"PRINTSIZET"x)\n",
		       (size_t) self, (size_t) threads_disabled_thread);
  }
#endif
}

PMOD_EXPORT void pike_threads_disallow (struct thread_state *ts COMMA_DLOC_DECL)
{
  /* Might get here before threads are started on failure
   * to create the backend wakeup pipe.
   *
   * (Via the SWAP_IN_THREADS_IF_REQUIRED() in error.c:va_error().)
   */
  if (!ts) return;

  if (ts->swapped) {
    pike_lock_interpreter (DLOC_ARGS_OPT);
    pike_swap_in_thread (ts COMMA_DLOC_ARGS_OPT);
  }

  if (UNLIKELY(thread_quanta)) {
    ts->interval_start = get_real_time();
  }

#ifdef PIKE_DEBUG
  ts->debug_flags &= ~THREAD_DEBUG_LOOSE;
  pike_debug_check_thread (DLOC_ARGS_OPT);
#endif
}

PMOD_EXPORT void pike_threads_allow_ext (struct thread_state *ts
					 COMMA_DLOC_DECL)
{
  /* Might get here after th_cleanup() when reporting leaks. */
  if (!ts) return;

  if (UNLIKELY(thread_quanta > 0)) {
    cpu_time_t now = get_real_time();

    if (UNLIKELY((now - ts->interval_start) > thread_quanta) &&
	LIKELY(ts->thread_obj)) {
      ref_push_object(ts->thread_obj);
      push_int64(now - ts->interval_start);
      ts->interval_start = now;
#ifndef LONG_CPU_TIME
      push_int(1000000000 / CPU_TIME_TICKS);
      o_multiply();
#endif
      SAFE_APPLY_MASTER("thread_quanta_exceeded", 2);
      pop_stack();
    }
  }

#ifdef PIKE_DEBUG
  pike_debug_check_thread (DLOC_ARGS_OPT);
  if (Pike_in_gc > 50 && Pike_in_gc < 300)
    pike_fatal_dloc ("Threads allowed during garbage collection (pass %d).\n",
		     Pike_in_gc);
  if (pike_global_buffer.s.str)
    pike_fatal_dloc ("Threads allowed while the global dynamic buffer "
		     "is in use.\n");
  ts->debug_flags |= THREAD_DEBUG_LOOSE;
#endif

  if (num_threads > 1 && !threads_disabled) {
    pike_swap_out_thread (ts COMMA_DLOC_ARGS_OPT);
    live_threads++;
    THREADS_FPRINTF (1, (stderr, "Increased live threads to %d\n",
			 live_threads));
    pike_unlock_interpreter (DLOC_ARGS_OPT);
  }

#if defined (PIKE_DEBUG) && !(defined(__ia64) && defined(__xlc__))
  else {
    /* Disabled in xlc 5.5.0.0/ia64 due to a code generation bug. */
    THREAD_T self = th_self();
    if (threads_disabled && !th_equal(threads_disabled_thread, self))
      pike_fatal_dloc ("Threads allowed from a different thread "
		       "while threads are disabled. "
		       "(self: %"PRINTSIZET"x, disabler: %"PRINTSIZET"x)\n",
		       (size_t) self, (size_t) threads_disabled_thread);
  }
#endif
}

PMOD_EXPORT void pike_threads_disallow_ext (struct thread_state *ts
					    COMMA_DLOC_DECL)
{
  /* Might get here before threads are started on failure
   * to create the backend wakeup pipe.
   *
   * (Via the SWAP_IN_THREADS_IF_REQUIRED() in error.c:va_error().)
   */
  if (!ts) return;

  if (ts->swapped) {
    pike_low_lock_interpreter (DLOC_ARGS_OPT);
    live_threads--;
    THREADS_FPRINTF (1, (stderr, "Decreased live threads to %d\n",
			 live_threads));
    co_broadcast (&live_threads_change);
    if (threads_disabled) threads_disabled_wait (DLOC_ARGS_OPT);
    pike_swap_in_thread (ts COMMA_DLOC_ARGS_OPT);
  }

  if (UNLIKELY(thread_quanta)) {
    ts->interval_start = get_real_time();
  }

#ifdef PIKE_DEBUG
  ts->debug_flags &= ~THREAD_DEBUG_LOOSE;
  pike_debug_check_thread (DLOC_ARGS_OPT);
#endif
}

PMOD_EXPORT void pike_lock_imutex (IMUTEX_T *im COMMA_DLOC_DECL)
{
  struct thread_state *ts = Pike_interpreter.thread_state;

  /* If threads are disabled, we already hold the lock. */
  if (threads_disabled) return;

  THREADS_FPRINTF(0, (stderr, "Locking IMutex %p...\n", im));
  pike_threads_allow (ts COMMA_DLOC_ARGS_OPT);
  mt_lock(&((im)->lock));
  pike_threads_disallow (ts COMMA_DLOC_ARGS_OPT);
  THREADS_FPRINTF(1, (stderr, "Locked IMutex %p\n", im));
}

PMOD_EXPORT void pike_unlock_imutex (IMUTEX_T *im COMMA_DLOC_DECL)
{
  /* If threads are disabled, we already hold the lock. */
  if (threads_disabled) return;

  USE_DLOC_ARGS();
  THREADS_FPRINTF(0, (stderr, "Unlocking IMutex %p" DLOC_PF(" @ ",) "\n",
		      im COMMA_DLOC_ARGS_OPT));
  mt_unlock(&(im->lock));
}

/* This is a variant of init_threads_disable that blocks all other
 * threads that might run pike code, but still doesn't block the
 * THREADS_ALLOW_UID threads. */
void low_init_threads_disable(void)
{
  /* Serious black magic to avoid dead-locks */

  if (!threads_disabled) {
    THREADS_FPRINTF(0,
		    (stderr, "low_init_threads_disable(): Locking IM's...\n"));

    lock_pike_compiler();

    if (Pike_interpreter.thread_state) {
      /* Threads have been enabled. */

      IMUTEX_T *im;

      THREADS_ALLOW();

      /* Keep this the entire session. */
      mt_lock(&interleave_lock);

      im = (IMUTEX_T *)interleave_list;

      while(im) {
	mt_lock(&(im->lock));

	im = im->next;
      }

      THREADS_DISALLOW();
    } else {
      /* Threads haven't been enabled yet. */

      IMUTEX_T *im;

      /* Keep this the entire session. */
      mt_lock(&interleave_lock);

      im = (IMUTEX_T *)interleave_list;

      while(im) {
	mt_lock(&(im->lock));

	im = im->next;
      }
    }

    THREADS_FPRINTF(0, (stderr,
			"low_init_threads_disable(): Disabling threads.\n"));

    threads_disabled = 1;
    threads_disabled_start = get_real_time();
#ifdef PIKE_DEBUG
    threads_disabled_thread = th_self();
#endif
  } else {
    threads_disabled++;
  }

  THREADS_FPRINTF(0,
		  (stderr, "low_init_threads_disable(): threads_disabled:%d\n",
		   threads_disabled));
}

/*! @decl object(_disable_threads) _disable_threads()
 *!
 *! This function first posts a notice to all threads that it is time
 *! to stop. It then waits until all threads actually *have* stopped,
 *! and then then returns a lock object. All other threads will be
 *! blocked from running until that object has been freed/destroyed.
 *!
 *! It's mainly useful to do things that require a temporary uid/gid
 *! change, since on many OS the effective user and group applies to
 *! all threads.
 *!
 *! @note
 *! You should make sure that the returned object is freed even if
 *! some kind of error is thrown. That means in practice that it
 *! should only have references (direct or indirect) from function
 *! local variables. Also, it shouldn't be referenced from cyclic
 *! memory structures, since those are only destructed by the periodic
 *! gc. (This advice applies to mutex locks in general, for that
 *! matter.)
 *!
 *! @seealso
 *!   @[gethrdtime()]
 */
void init_threads_disable(struct object *UNUSED(o))
{
  low_init_threads_disable();

  if(live_threads) {
    SWAP_OUT_CURRENT_THREAD();
    while (live_threads) {
      THREADS_FPRINTF(1,
		      (stderr,
		       "_disable_threads(): Waiting for %d threads to finish\n",
		       live_threads));
      low_co_wait_interpreter (&live_threads_change);
    }
    THREADS_FPRINTF(0, (stderr, "_disable_threads(): threads now disabled\n"));
    SWAP_IN_CURRENT_THREAD();
  }
}

void exit_threads_disable(struct object *UNUSED(o))
{
  THREADS_FPRINTF(0, (stderr, "exit_threads_disable(): threads_disabled:%d\n",
		      threads_disabled));
  if(threads_disabled) {
    if(!--threads_disabled) {
      IMUTEX_T *im = (IMUTEX_T *)interleave_list;
      threads_disabled_acc_time += get_real_time() - threads_disabled_start;

      /* Order shouldn't matter for unlock, so no need to do it backwards. */
      while(im) {
	THREADS_FPRINTF(0, (stderr,
			    "exit_threads_disable(): Unlocking IM %p\n", im));
	mt_unlock(&(im->lock));
	im = im->next;
      }

      unlock_pike_compiler();

      mt_unlock(&interleave_lock);

      THREADS_FPRINTF(0, (stderr, "exit_threads_disable(): Wake up!\n"));
      co_broadcast(&threads_disabled_change);
#ifdef PIKE_DEBUG
      threads_disabled_thread = 0;
#endif
    }
#ifdef PIKE_DEBUG
  } else {
    Pike_fatal("exit_threads_disable() called too many times!\n");
#endif /* PIKE_DEBUG */
  }
}

void init_interleave_mutex(IMUTEX_T *im)
{
  mt_init(&(im->lock));

  THREADS_FPRINTF(0, (stderr,
		      "init_interleave_mutex(): init_threads_disable()\n"));

  init_threads_disable(NULL);

  THREADS_FPRINTF(0, (stderr, "init_interleave_mutex(): Locking IM %p\n", im));

  /* Lock it so that it can be unlocked by exit_threads_disable() */
  mt_lock(&(im->lock));

  im->next = (IMUTEX_T *)interleave_list;
  if (interleave_list) {
    interleave_list->prev = im;
  }
  interleave_list = im;
  im->prev = NULL;

  THREADS_FPRINTF(0, (stderr,
		      "init_interleave_mutex(): exit_threads_disable()\n"));

  exit_threads_disable(NULL);
}

void exit_interleave_mutex(IMUTEX_T *im)
{
  init_threads_disable(NULL);

  if (im->prev) {
    im->prev->next = im->next;
  } else {
    interleave_list = im->next;
  }
  if (im->next) {
    im->next->prev = im->prev;
  }

  /* Just to be nice... */
  mt_unlock(&(im->lock));

  exit_threads_disable(NULL);
}

/* Thread hashtable */

struct thread_state *thread_table_chains[THREAD_TABLE_SIZE];
int num_pike_threads=0;

void thread_table_init(void)
{
  INT32 x;
  for(x=0; x<THREAD_TABLE_SIZE; x++)
    thread_table_chains[x] = NULL;
}

unsigned INT32 thread_table_hash(THREAD_T *tid)
{
  return th_hash(*tid) % THREAD_TABLE_SIZE;
}

PMOD_EXPORT void thread_table_insert(struct thread_state *s)
{
  unsigned INT32 h = thread_table_hash(&s->id);
#ifdef PIKE_DEBUG
  if(h>=THREAD_TABLE_SIZE)
    Pike_fatal("thread_table_hash failed miserably!\n");
  if(thread_state_for_id(s->id))
  {
    if(thread_state_for_id(s->id) == s)
      Pike_fatal("Registring thread twice!\n");
    else
      Pike_fatal("Forgot to unregister thread!\n");
  }
/*  fprintf(stderr, "thread_table_insert: %"PRINTSIZET"x\n", (size_t) s->id); */
#endif
  mt_lock( & thread_table_lock );
  num_pike_threads++;
  if((s->hashlink = thread_table_chains[h]) != NULL)
    s->hashlink->backlink = &s->hashlink;
  thread_table_chains[h] = s;
  s->backlink = &thread_table_chains[h];
  mt_unlock( & thread_table_lock );  
}

PMOD_EXPORT void thread_table_delete(struct thread_state *s)
{
/*  fprintf(stderr, "thread_table_delete: %"PRINTSIZET"x\n", (size_t) s->id); */
  mt_lock( & thread_table_lock );
  num_pike_threads--;
  if(s->hashlink != NULL)
    s->hashlink->backlink = s->backlink;
  *(s->backlink) = s->hashlink;
  mt_unlock( & thread_table_lock );
}

PMOD_EXPORT struct thread_state *thread_state_for_id(THREAD_T tid)
{
  unsigned INT32 h = thread_table_hash(&tid);
  struct thread_state *s = NULL;
#if 0
  if(num_threads>1)
    fprintf (stderr, "thread_state_for_id: %"PRINTSIZET"x\n", (size_t) tid);
#endif
#ifdef PIKE_DEBUG
  if(h>=THREAD_TABLE_SIZE)
    Pike_fatal("thread_table_hash failed miserably!\n");
#endif
  mt_lock( & thread_table_lock );
  if(thread_table_chains[h] == NULL)
  {
    /* NULL result */
  }
  else if(th_equal((s=thread_table_chains[h])->id, tid))
  {
    /* Quick return */
  }
  else
  {
    while((s = s->hashlink) != NULL)
      if(th_equal(s->id, tid))
	break;
    if(s != NULL) {
      /* Move the Pike_interpreter to the head of the chain, in case
	 we want to search for it again */

      /* Unlink */
      if(s->hashlink != NULL)
	s->hashlink->backlink = s->backlink;
      *(s->backlink) = s->hashlink;
      /* And relink at the head of the chain */
      if((s->hashlink = thread_table_chains[h]) != NULL)
	s->hashlink->backlink = &s->hashlink;
      thread_table_chains[h] = s;
      s->backlink = &thread_table_chains[h];
    }
  }
  mt_unlock( & thread_table_lock );
#if 0
  if(num_threads>1 && s)
    fprintf (stderr, "thread_state_for_id return value: %"PRINTSIZET"x\n",
	     (size_t) s->id);
#endif
  return s;
  /* NOTEZ BIEN:  Return value only guaranteed to remain valid as long
     as you have the interpreter lock, unless tid == th_self() */
}

struct thread_state *gdb_thread_state_for_id(THREAD_T tid)
/* Should only be used from a debugger session. */
{
  unsigned INT32 h = thread_table_hash(&tid);
  struct thread_state *s;
  for (s = thread_table_chains[h]; s != NULL; s = s->hashlink)
    if(th_equal(s->id, tid))
      break;
  return s;
}

INT32 gdb_next_thread_state(INT32 prev, struct thread_state **ts)
/* Used by gdb_backtraces. */
{
  if (!*ts || !(*ts)->hashlink) {
    if (!*ts) prev = -1;
    while (++prev < THREAD_TABLE_SIZE)
      if ((*ts = thread_table_chains[prev]))
	return prev;
    *ts = NULL;
    return 0;
  }
  *ts = (*ts)->hashlink;
  return prev;
}

PMOD_EXPORT struct object *thread_for_id(THREAD_T tid)
{
  struct thread_state *s = thread_state_for_id(tid);
  return (s == NULL? NULL : THREADSTATE2OBJ(s));
  /* See NB in thread_state_for_id.  Lifespan of result can be prolonged
     by incrementing refcount though. */
}

static inline void CALL_WITH_ERROR_HANDLING(struct thread_state *state,
                                            void (*func)(void *ctx),
                                            void *ctx)
{
  JMP_BUF back;

  if(SETJMP(back))
  {
    if(throw_severity <= THROW_ERROR) {
      if (state->thread_obj) {
	/* Copy the thrown exit value to the thread_state here,
	 * if the thread hasn't been destructed. */
	assign_svalue(&state->result, &throw_value);
      }

      call_handle_error();
    }

    if(throw_severity == THROW_EXIT)
    {
      /* This is too early to get a clean exit if DO_PIKE_CLEANUP is
       * active. Otoh it cannot be done later since it requires the
       * evaluator stacks in the gc calls. It's difficult to solve
       * without handing over the cleanup duty to the main thread. */
      pike_do_exit(throw_value.u.integer);
    }
  } else {
    back.severity=THROW_EXIT;
    func(ctx);
  }

  UNSETJMP(back);
}

PMOD_EXPORT void call_with_interpreter(void (*func)(void *ctx), void *ctx)
{
  struct thread_state *state;

  if((state = thread_state_for_id(th_self()))!=NULL) {
    /* This is a pike thread.  Do we have the interpreter lock? */
    if(!state->swapped) {
      /* Yes.  Go for it... */
#ifdef PIKE_DEBUG
      /* Ensure that the thread isn't loose. */
      int is_loose = state->debug_flags & THREAD_DEBUG_LOOSE;
      state->debug_flags &= ~THREAD_DEBUG_LOOSE;
#endif

      CALL_WITH_ERROR_HANDLING(state, func, ctx);

#ifdef PIKE_DEBUG
      /* Restore the looseness property of the thread. */
      state->debug_flags |= is_loose;
#endif
    } else {
      /* Nope, let's get it... */
      mt_lock_interpreter();
      SWAP_IN_THREAD(state);
      DO_IF_DEBUG(state->debug_flags &= ~THREAD_DEBUG_LOOSE;)

      CALL_WITH_ERROR_HANDLING(state, func, ctx);

      /* Restore */
      DO_IF_DEBUG(state->debug_flags |= THREAD_DEBUG_LOOSE;)
      SWAP_OUT_THREAD(state);
      mt_unlock_interpreter();
    }
  } else {
    /* Not a pike thread.  Create a temporary thread_id... */
    struct object *thread_obj;
    struct Pike_interpreter_struct new_interpreter;

    mt_lock_interpreter();
    Pike_interpreter_pointer = &new_interpreter;
    init_interpreter();
    Pike_interpreter.stack_top=((char *)&state)+ (thread_stack_size-16384) * STACK_DIRECTION;
    Pike_interpreter.recoveries = NULL;
    thread_obj = fast_clone_object(thread_id_prog);
    INIT_THREAD_STATE((struct thread_state *)(thread_obj->storage +
                                              thread_storage_offset));
    num_threads++;
    thread_table_insert(Pike_interpreter.thread_state);

    CALL_WITH_ERROR_HANDLING(Pike_interpreter.thread_state, func, ctx);

#ifdef PIKE_DEBUG
    if (Pike_interpreter.thread_state->busy_prev ||
        Pike_interpreter.thread_state->busy_next) {
      Pike_fatal("Thread is still registered as busy when terminating!\n");
    }
#endif

    cleanup_interpret();        /* Must be done before EXIT_THREAD_STATE */
    Pike_interpreter.thread_state->status=THREAD_EXITED;
    co_signal(&Pike_interpreter.thread_state->status_change);
    thread_table_delete(Pike_interpreter.thread_state);
    EXIT_THREAD_STATE(Pike_interpreter.thread_state);
    Pike_interpreter.thread_state=NULL;
    free_object(thread_obj);
    thread_obj = NULL;
    num_threads--;
#ifdef PIKE_DEBUG
    Pike_interpreter_pointer = NULL;
#endif
    mt_unlock_interpreter();
  }
}

PMOD_EXPORT void enable_external_threads(void)
{
  num_threads++;  
}

PMOD_EXPORT void disable_external_threads(void)
{
  num_threads--;
}

/*! @module Thread
 */

/*! @decl array(Thread.Thread) all_threads()
 *!
 *! This function returns an array with the thread ids of all threads.
 *!
 *! @seealso
 *!   @[Thread()]
 */
PMOD_EXPORT void f_all_threads(INT32 args)
{
  /* Return an unordered array containing all threads that was running
     at the time this function was invoked */

  struct svalue *oldsp;
  struct thread_state *s;

  pop_n_elems(args);
  oldsp = Pike_sp;
  FOR_EACH_THREAD (s, {
      struct object *o = THREADSTATE2OBJ(s);
      if (o) {
	ref_push_object(o);
      }
    });
  f_aggregate(DO_NOT_WARN(Pike_sp - oldsp));
}

#ifdef PIKE_DEBUG
PMOD_EXPORT void debug_list_all_threads(void)
{
  INT32 x;
  struct thread_state *s;
  THREAD_T self = th_self();

  fprintf(stderr,"--Listing all threads--\n");
  fprintf (stderr, "Current thread: %"PRINTSIZET"x\n", (size_t) self);
  fprintf(stderr,"Current interpreter thread state: %p%s\n",
	  Pike_interpreter.thread_state,
	  Pike_interpreter.thread_state == (struct thread_state *) (ptrdiff_t) -1 ?
	  " (swapped)" : "");
  fprintf(stderr,"Current thread state according to thread_state_for_id(): %p\n",
	  thread_state_for_id (self));
  fprintf(stderr,"Current thread obj: %p\n",
	  (Pike_interpreter.thread_state &&
	   Pike_interpreter.thread_state != (struct thread_state *) (ptrdiff_t) -1) ?
	  Pike_interpreter.thread_state->thread_obj : NULL);
  fprintf(stderr,"Current thread hash: %d\n",thread_table_hash(&self));
  fprintf(stderr,"Current stack pointer: %p\n",&self);
  for(x=0; x<THREAD_TABLE_SIZE; x++)
  {
    for(s=thread_table_chains[x]; s; s=s->hashlink) {
      struct object *o = THREADSTATE2OBJ(s);
      fprintf(stderr,"ThTab[%d]: state=%p, obj=%p, "
	      "swapped=%d, sp=%p (%+"PRINTPTRDIFFT"d), fp=%p, stackbase=%p, "
	      "id=%"PRINTSIZET"x\n",
	      x, s, o, s->swapped,
	      s->state.stack_pointer,
	      s->state.stack_pointer - s->state.evaluator_stack,
	      s->state.frame_pointer,
	      s->state.stack_top,
	      (size_t) s->id);
    }
  }
  fprintf(stderr,"-----------------------\n");
}
#endif

PMOD_EXPORT int count_pike_threads(void)
{
  return num_pike_threads;
}

static void check_threads(struct callback *UNUSED(cb), void *UNUSED(arg), void *UNUSED(arg2))
{
#ifdef PROFILE_CHECK_THREADS
  static unsigned long calls = 0, yields = 0;
  static unsigned long clock_checks = 0, no_clock_advs = 0;
#if 0
  static unsigned long slice_int_n = 0; /* Slice interval length. */
  static double slice_int_mean = 0.0, slice_int_m2 = 0.0;
  static unsigned long tsc_int_n = 0; /* Actual tsc interval length. */
  static double tsc_int_mean = 0.0, tsc_int_m2 = 0.0;
  static unsigned long tsc_tgt_n = 0; /* Target tsc interval length. */
  static double tsc_tgt_mean = 0.0, tsc_tgt_m2 = 0.0;
#endif
  static unsigned long tps = 0, tps_int_n = 0;  /* TSC intervals per slice. */
  static double tps_int_mean = 0.0, tps_int_m2 = 0.0;
  calls++;
#endif
#ifdef PIKE_DEBUG
  check_threads_calls++;
#endif

#if defined (USE_CLOCK_FOR_SLICES) && defined (PIKE_DEBUG)
  if (last_clocked_thread != th_self())
    Pike_fatal ("Stale thread %08lx in last_clocked_thread (self is %08lx)\n",
		(unsigned long) last_clocked_thread, (unsigned long) th_self());
#endif

#if defined(RDTSC) && defined(USE_CLOCK_FOR_SLICES)
  /* We can get here as often as 30+ thousand times per second;
     let's try to avoid doing as many clock(3)/times(2) syscalls
     by using the TSC. */

  if (use_tsc_for_slices) {
     static INT64 target_int = TSC_START_INTERVAL;
     INT64 tsc_now, tsc_elapsed;

     /* prev_tsc is normally always valid here, but it can be zero
      * after a tsc jump reset and just after a thread has been
      * "pike-ified" with INIT_THREAD_STATE (in which case
      * thread_start_clock is zero too). */

     RDTSC(tsc_now);
     tsc_elapsed = tsc_now - prev_tsc;

     if (tsc_elapsed < target_int) {
       if (tsc_elapsed < 0) {
	 /* The counter jumped back in time, so reset and continue. In
	  * the worst case this keeps happening all the time, and then
	  * the only effect is that we always fall back to
	  * clock(3). */
#ifdef PROFILE_CHECK_THREADS
	 fprintf (stderr, "[%d:%f] TSC backward jump detected "
		  "(now: %"PRINTINT64"d, prev: %"PRINTINT64"d, "
		  "target_int: %"PRINTINT64"d) - resetting\n",
		  getpid(), get_real_time() * (1.0 / CPU_TIME_TICKS),
		  tsc_now, prev_tsc, target_int);
#endif
	 target_int = TSC_START_INTERVAL;
	 prev_tsc = 0;
       }
       else
	 return;
     }

#ifdef PROFILE_CHECK_THREADS
#if 0
     if (prev_tsc) {
       double delta = tsc_elapsed - tsc_int_mean;
       tsc_int_n++;
       tsc_int_mean += delta / tsc_int_n;
       tsc_int_m2 += delta * (tsc_elapsed - tsc_int_mean);
     }
#endif
     clock_checks++;
     tps++;
#endif

     {
       clock_t clock_now = clock();

       /* Aim tsc intervals at 1/20th of a thread time slice interval,
	* i.e. at 1/400 sec. That means the time is checked with
	* clock(3) approx 20 times per slice. That still cuts the vast
	* majority of the clock() calls and we're still fairly safe
	* against tsc inconsistencies of different sorts, like cpu clock
	* rate changes (on older cpu's with variable tsc),
	* unsynchronized tsc's between cores, OS tsc resets, etc -
	* individual intervals can be off by more than an order of
	* magnitude in either direction without affecting the final time
	* slice length appreciably.
	*
	* Note that the real interval lengths will always be longer.
	* One reason is that we won't get calls exactly when they run
	* out. Another is the often lousy clock(3) resolution. */

       if (prev_tsc) {
	 if (clock_now > prev_clock) {
	   /* Estimate the next interval just by extrapolating the
	    * tsc/clock ratio of the last one. This adapts very
	    * quickly but is also very "jumpy". That shouldn't matter
	    * due to the approach with dividing the time slice into
	    * ~20 tsc intervals.
	    *
	    * Note: The main source of the jumpiness is probably that
	    * clock(3) has so lousy resolution on many platforms, i.e.
	    * it may step forward very large intervals very seldom
	    * (100 times/sec on linux/glibc 2.x). It also has the
	    * effect that the actual tsc intervals will be closer to
	    * 1/200 sec. */
	   clock_t tsc_interval_time = clock_now - prev_clock;
	   INT64 new_target_int =
	     (tsc_elapsed * (CLOCKS_PER_SEC / 400)) / tsc_interval_time;
	   if (new_target_int < target_int << 2)
	     target_int = new_target_int;
	   else {
	     /* The most likely cause for this is high variance in the
	      * interval lengths due to low clock(3) resolution. */
#ifdef PROFILE_CHECK_THREADS
	     fprintf (stderr, "[%d:%f] Capping large TSC interval increase "
		      "(from %"PRINTINT64"d to %"PRINTINT64"d)\n",
		      getpid(), get_real_time() * (1.0 / CPU_TIME_TICKS),
		      target_int, new_target_int);
#endif
	     /* The + 1 is paranoia just in case it has become zero somehow. */
	     target_int = (target_int << 2) + 1;
	   }
	   prev_tsc = tsc_now;
	   prev_clock = clock_now;
#if 0
#ifdef PROFILE_CHECK_THREADS
	   {
	     double delta = target_int - tsc_tgt_mean;
	     tsc_tgt_n++;
	     tsc_tgt_mean += delta / tsc_tgt_n;
	     tsc_tgt_m2 += delta * (target_int - tsc_tgt_mean);
	   }
#endif
#endif
	 }
	 else {
	   /* clock(3) can have pretty low resolution and might not
	    * have advanced during the tsc interval. Just do another
	    * round on the old estimate, keeping prev_tsc and
	    * prev_clock fixed to get a longer interval for the next
	    * measurement. */
	   if (clock_now < prev_clock) {
	     /* clock() wraps around fairly often as well. We still
	      * keep the old interval but update the baselines in this
	      * case. */
	     prev_tsc = tsc_now;
	     prev_clock = clock_now;
	   }
	   target_int += tsc_elapsed;
#ifdef PROFILE_CHECK_THREADS
	   no_clock_advs++;
#endif
	 }
       }
       else {
#ifdef PROFILE_CHECK_THREADS
	 fprintf (stderr, "[%d:%f] Warning: Encountered zero prev_tsc "
		  "(thread_start_clock: %"PRINTINT64"d, "
		  "clock_now: %"PRINTINT64"d)\n",
		  getpid(), get_real_time() * (1.0 / CPU_TIME_TICKS),
		  (INT64) thread_start_clock, (INT64) clock_now);
#endif
	 prev_tsc = tsc_now;
       }

       if (clock_now < thread_start_clock)
	 /* clock counter has wrapped since the start of the time
	  * slice. Let's reset and yield. */
	 thread_start_clock = 0;
       else if (clock_now - thread_start_clock <
		(clock_t) (CLOCKS_PER_SEC / 20))
	 return;
     }

#ifdef PROFILE_CHECK_THREADS
     {
       double delta = tps - tps_int_mean;
       tps_int_n++;
       tps_int_mean += delta / tps_int_n;
       tps_int_m2 += delta * (tps - tps_int_mean);
       tps = 0;
     }
#endif

     goto do_yield;
  }
#endif	/* RDTSC && USE_CLOCK_FOR_SLICES */

#ifdef HAVE_GETHRTIME
  {
    static hrtime_t last_ = 0;
    hrtime_t now = gethrtime();
    if( now-last_ < 50000000 ) /* 0.05s slice */
      return;
    last_ = now;
  }
#elif defined(HAVE_MACH_TASK_INFO_H) && defined(TASK_THREAD_TIMES_INFO)
  {
    static struct timeval         last_check = { 0, 0 };
    task_thread_times_info_data_t info;
    mach_msg_type_number_t        info_size = TASK_THREAD_TIMES_INFO_COUNT;
    
    /* Before making an expensive call to task_info() we perform a
       preliminary check that at least 35 ms real time has passed. If
       not yet true we'll postpone the next check a full interval. */
    struct timeval                tv;
    ACCURATE_GETTIMEOFDAY(&tv);
    {
#ifdef INT64
      static INT64 real_time_last_check = 0;
      INT64 real_time_now = tv.tv_sec * 1000000 + tv.tv_usec;
      if (real_time_now - real_time_last_check < 35000)
	return;
      real_time_last_check = real_time_now;
#else
      static struct timeval real_time_last_check = { 0, 0 };
      struct timeval diff;
      timersub(&real_time_now, &real_time_last_check, &diff);
      if (diff.tv_usec < 35000 && diff.tv_sec == 0)
	return;
      real_time_last_check = tv;
#endif
    }
    
    /* Get user time and test if 50 ms has passed since last check. */
    if (task_info(mach_task_self(), TASK_THREAD_TIMES_INFO,
		  (task_info_t) &info, &info_size) == 0) {
#ifdef INT64
      static INT64 last_check = 0;
      INT64 now =
	info.user_time.seconds * 1000000 +
	info.user_time.microseconds +
	info.system_time.seconds * 1000000 +
	info.system_time.microseconds;
      if (now - last_check < 50000)
	return;
      last_check = now;
#else
      /* Compute difference by converting kernel time_info_t to timeval. */
      static struct timeval last_check = { 0, 0 };
      struct timeval user, sys, now;
      struct timeval diff;
      user.tv_sec = info.user_time.seconds;
      user.tv_usec = info.user_time.microseconds;
      sys.tv_sec = info.system_time.seconds;
      sys.tv_usec = info.system_time.microseconds;
      timeradd (&user, &sys, &now);
      timersub(&now, &last_check, &diff);
      if (diff.tv_usec < 50000 && diff.tv_sec == 0)
	return;
      last_check = now;
#endif
    }
  }
#elif defined (USE_CLOCK_FOR_SLICES)
  {
    clock_t clock_now = clock();
    if (clock_now < thread_start_clock)
      /* clock counter has wrapped since the start of the time slice.
       * Let's reset and yield. */
      thread_start_clock = 0;
    else if (clock_now - thread_start_clock < (clock_t) (CLOCKS_PER_SEC / 20))
      return;
  }
#else
  static int div_;
  if(div_++ & 255)
    return;
#endif

  do_yield:;
#ifdef PIKE_DEBUG
  check_threads_yields++;
#endif

#ifdef PROFILE_CHECK_THREADS
  {
    static long last_time;
    struct timeval now;

    yields++;

#if 0
#ifdef USE_CLOCK_FOR_SLICES
    if (thread_start_clock) {
      double slice_time =
	(double) (clock() - thread_start_clock) / CLOCKS_PER_SEC;
      double delta = slice_time - slice_int_mean;
      slice_int_n++;
      slice_int_mean += delta / slice_int_n;
      slice_int_m2 += delta * (slice_time - slice_int_mean);
    }
#endif
#endif

    ACCURATE_GETTIMEOFDAY (&now);
    if (now.tv_sec > last_time) {
      fprintf (stderr, "[%d:%f] check_threads: %lu calls, "
	       "%lu clocks, %lu no advs, %lu yields"
#if 0
	       ", slice %.3f:%.1e, tsc int %.2e:%.1e, tsc tgt %.2e:%.1e"
#endif
	       ", tps %g:%.1e\n",
	       getpid(), get_real_time() * (1.0 / CPU_TIME_TICKS),
	       calls, clock_checks, no_clock_advs, yields,
#if 0
	       slice_int_mean,
	       slice_int_n > 1 ? sqrt (slice_int_m2 / (slice_int_n - 1)) : 0.0,
	       tsc_int_mean,
	       tsc_int_n > 1 ? sqrt (tsc_int_m2 / (tsc_int_n - 1)) : 0.0,
	       tsc_tgt_mean,
	       tsc_tgt_n > 1 ? sqrt (tsc_tgt_m2 / (tsc_tgt_n - 1)) : 0.0,
#endif
	       tps_int_mean,
	       tps_int_n > 1 ? sqrt (tps_int_m2 / (tps_int_n - 1)) : 0.0);
      last_time = (unsigned long) now.tv_sec;
      calls = yields = clock_checks = no_clock_advs = 0;
    }
  }
#endif

  {
#ifdef PIKE_DEBUG
    unsigned LONGEST old_thread_swaps = thread_swaps;
#endif
    pike_thread_yield();
#ifdef PIKE_DEBUG
    if (thread_swaps != old_thread_swaps)
      check_threads_swaps++;
#endif
  }
}

PMOD_EXPORT void pike_thread_yield(void)
{
  DEBUG_CHECK_THREAD();

  THREADS_ALLOW();
  /* Allow other threads to run */
  /* FIXME: Ought to use condition vars or something to get another
   * thread to run. yield functions are notoriously unreliable and
   * poorly defined. It might not really yield we need it to. It might
   * make us yield to another process instead of just another thread.
   * It might even make us sleep for a short while. */
  th_yield();
  THREADS_DISALLOW();

#ifdef USE_CLOCK_FOR_SLICES
  /* If we didn't yield then give ourselves a new time slice. If we
   * did yield then thread_start_clock is the current clock anyway
   * after the thread swap in. */
  thread_start_clock = clock();
#ifdef RDTSC
  RDTSC (prev_tsc);
  prev_clock = thread_start_clock;
#endif
#ifdef PIKE_DEBUG
  if (last_clocked_thread != th_self())
    Pike_fatal ("Stale thread %08lx in last_clocked_thread (self is %08lx)\n",
		(unsigned long) last_clocked_thread, (unsigned long) th_self());
#endif
#endif

  DEBUG_CHECK_THREAD();
}

struct thread_starter
{
  struct thread_state *thread_state;	/* State of the thread. */
  struct array *args;			/* Arguments passed to the thread. */
#ifdef HAVE_BROKEN_LINUX_THREAD_EUID
  int euid, egid;
#endif /* HAVE_BROKEN_LINUX_THREAD_EUID */
};

/* Thread func starting new Pike-level threads. */
TH_RETURN_TYPE new_thread_func(void *data)
{
  struct thread_starter arg = *(struct thread_starter *)data;
  struct object *thread_obj;
  struct thread_state *thread_state;
  JMP_BUF back;

  THREADS_FPRINTF(0, (stderr,"new_thread_func(): Thread %p created...\n",
		      arg.thread_state));

#ifdef HAVE_BROKEN_LINUX_THREAD_EUID
  /* Work-around for Linux's pthreads not propagating the
   * effective uid & gid.
   */
  if (!geteuid()) {
#if defined(HAVE_PRCTL) && defined(PR_SET_DUMPABLE)
    /* The sete?id calls will clear the dumpable state that we might
     * have set with system.dumpable. */
    int current = prctl(PR_GET_DUMPABLE);
#ifdef PIKE_DEBUG
    if (current == -1)
      fprintf (stderr, "%s:%d: Unexpected error from prctl(2). errno=%d\n",
	       __FILE__, __LINE__, errno);
#endif
#endif
#ifdef HAVE_BROKEN_LINUX_THREAD_EUID
    if( setegid(arg.egid) != 0 || seteuid(arg.euid) != 0 )
    {
      fprintf (stderr, "%s:%d: Unexpected error from setegid(2). errno=%d\n",
	       __FILE__, __LINE__, errno);
    }
#endif
#if defined(HAVE_PRCTL) && defined(PR_SET_DUMPABLE)
    if (current != -1 && prctl(PR_SET_DUMPABLE, current) == -1) {
#if defined(PIKE_DEBUG)
      fprintf (stderr, "%s:%d: Unexpected error from prctl(2). errno=%d\n",
	       __FILE__, __LINE__, errno);
#endif
    }
#endif
  }
#endif /* HAVE_BROKEN_LINUX_THREAD_EUID */

  /* Lock the interpreter now, but don't wait on
   * threads_disabled_change since the spawning thread might be
   * holding it. */
  low_mt_lock_interpreter();

#if defined(PIKE_DEBUG)
  if(d_flag) {
    THREAD_T self = th_self();
    if( !th_equal(arg.thread_state->id, self) )
      Pike_fatal("Current thread is wrong. %lx %lx\n",
		 (long)arg.thread_state->id, (long)self);
  }
#endif

  arg.thread_state->swapped = 0;
  Pike_interpreter_pointer = &arg.thread_state->state;

#ifdef PROFILING
  Pike_interpreter.stack_bottom=((char *)&data);
#endif
  Pike_interpreter.stack_top=((char *)&data)+ (thread_stack_size-16384) * STACK_DIRECTION;
  Pike_interpreter.recoveries = NULL;

  add_ref(thread_obj = arg.thread_state->thread_obj);
  INIT_THREAD_STATE(thread_state = arg.thread_state);

  /* Inform the spawning thread that we are now running. */
  co_broadcast(&thread_state->status_change);

  /* After signalling the status change to the spawning thread we may
   * now wait if threads are disabled. */
  if (threads_disabled) {
    SWAP_OUT_CURRENT_THREAD();
    threads_disabled_wait (DLOC);
    SWAP_IN_CURRENT_THREAD();
  }

  DEBUG_CHECK_THREAD();

  THREADS_FPRINTF(0, (stderr,"new_thread_func(): Thread %p inited\n",
		      arg.thread_state));

  if(SETJMP(back))
  {
    if(throw_severity <= THROW_ERROR) {
      if (thread_state->thread_obj) {
	/* Copy the thrown exit value to the thread_state here,
	 * if the thread hasn't been destructed. */
	assign_svalue(&thread_state->result, &throw_value);
      }

      call_handle_error();
    }

    if(throw_severity == THROW_EXIT)
    {
      /* This is too early to get a clean exit if DO_PIKE_CLEANUP is
       * active. Otoh it cannot be done later since it requires the
       * evaluator stacks in the gc calls. It's difficult to solve
       * without handing over the cleanup duty to the main thread. */
      pike_do_exit(throw_value.u.integer);
    }

    thread_state->status = THREAD_ABORTED;
  } else {
    INT32 args=arg.args->size;
    back.severity=THROW_EXIT;
    push_array_items(arg.args);
    arg.args=0;
    f_call_function(args);

    /* Copy return value to the thread_state here, if the thread
     * object hasn't been destructed. */
    if (thread_state->thread_obj)
      assign_svalue(&thread_state->result, Pike_sp-1);
    pop_stack();

    thread_state->status = THREAD_EXITED;
  }

  UNSETJMP(back);

  DEBUG_CHECK_THREAD();

  if(thread_state->thread_locals != NULL) {
    free_mapping(thread_state->thread_locals);
    thread_state->thread_locals = NULL;
  }

  co_broadcast(&thread_state->status_change);

  THREADS_FPRINTF(0, (stderr,"new_thread_func(): Thread %p done\n",
		      arg.thread_state));

  /* This thread is now officially dead. */

  while(Pike_fp)
    POP_PIKE_FRAME();

  reset_evaluator();

#ifdef PIKE_DEBUG
  if (thread_state->busy_prev || thread_state->busy_next) {
    Pike_fatal("Thread is still registered as busy when terminating!\n");
  }
#endif

  low_cleanup_interpret(&thread_state->state);

  if (!thread_state->thread_obj)
    /* Do this only if exit_thread_obj already has run. */
    cleanup_thread_state (thread_state);

  thread_table_delete(thread_state);
  EXIT_THREAD_STATE(thread_state);
  Pike_interpreter.thread_state = thread_state = NULL;
  Pike_interpreter_pointer = NULL;

  /* Free ourselves.
   * NB: This really ought to run in some other thread...
   */

  free_object(thread_obj);
  thread_obj = NULL;

  num_threads--;
  if(!num_threads && threads_evaluator_callback)
  {
    remove_callback(threads_evaluator_callback);
    threads_evaluator_callback=0;
  }

#ifdef INTERNAL_PROFILING
  fprintf (stderr, "Thread usage summary:\n");
  debug_print_rusage (stderr);
#endif

  /* FIXME: What about threads_disable? */
  mt_unlock_interpreter();
  th_exit(0);
  /* NOT_REACHED, but removes a warning */
  return 0;
}

#ifdef UNIX_THREADS
int num_lwps = 1;
#endif

/*! @class Thread
 */

/*! @decl void create(function(mixed...:mixed|void) f, mixed ... args)
 *!
 *! This function creates a new thread which will run simultaneously
 *! to the rest of the program. The new thread will call the function
 *! @[f] with the arguments @[args]. When @[f] returns the thread will cease
 *! to exist.
 *!
 *! All Pike functions are 'thread safe' meaning that running
 *! a function at the same time from different threads will not corrupt
 *! any internal data in the Pike process.
 *!
 *! @returns
 *!   The returned value will be the same as the return value of
 *!   @[this_thread()] for the new thread.
 *!
 *! @note
 *!   This function is only available on systems with POSIX or UNIX or WIN32
 *!   threads support.
 *!
 *! @seealso
 *!   @[Mutex], @[Condition], @[this_thread()]
 */
void f_thread_create(INT32 args)
{
  struct thread_starter arg;
  struct thread_state *thread_state =
    (struct thread_state *)Pike_fp->current_storage;
  int tmp;

  if (args < 1) {
    SIMPLE_TOO_FEW_ARGS_ERROR("create", 1);
  }
  push_svalue(Pike_sp - args);
  f_callablep(1);
  if (UNSAFE_IS_ZERO(Pike_sp - 1)) {
    SIMPLE_BAD_ARG_ERROR("create", 1, "function");
  }
  pop_stack();

  if (thread_state->status != THREAD_NOT_STARTED) {
    Pike_error("Threads can not be restarted (status:%d).\n",
	       thread_state->status);
  }

  arg.args = aggregate_array(args);
  arg.thread_state = thread_state;

  if (low_init_interpreter(&thread_state->state)) {
    free_array(arg.args);
    Pike_error("Out of memory allocating stack.\n");
  }

#ifdef HAVE_BROKEN_LINUX_THREAD_EUID
  arg.euid = geteuid();
  arg.egid = getegid();
#endif /* HAVE_BROKEN_LINUX_THREAD_EUID */

  do {
    tmp = th_create(&thread_state->id,
		    new_thread_func,
		    &arg);
    if (tmp == EINTR) check_threads_etc();
  } while( tmp == EINTR );

  if(!tmp)
  {
    num_threads++;
    thread_table_insert(thread_state);

    if(!threads_evaluator_callback)
    {
      threads_evaluator_callback=add_to_callback(&evaluator_callbacks,
						 check_threads, 0,0);
      dmalloc_accept_leak(threads_evaluator_callback);
    }

    /* Wait for the thread to start properly.
     * so that we can avoid races.
     *
     * The main race is the one on current_object,
     * since it at this point only has one reference.
     *
     * We also want the stuff in arg to be copied properly
     * before we exit the function...
     */
    SWAP_OUT_CURRENT_THREAD();
    THREADS_FPRINTF(0, (stderr, "f_thread_create %p waiting...\n",
			thread_state));
    while (thread_state->status == THREAD_NOT_STARTED)
      co_wait_interpreter (&thread_state->status_change);
    THREADS_FPRINTF(0, (stderr, "f_thread_create %p continue\n", thread_state));
    SWAP_IN_CURRENT_THREAD();
  } else {
    low_cleanup_interpret(&thread_state->state);
    free_array(arg.args);
    Pike_error("Failed to create thread (errno = %d).\n", tmp);
  }

  THREADS_FPRINTF(0, (stderr, "f_thread_create %p done\n", thread_state));
  push_int(0);
}

/*! @endclass
 */

#ifdef UNIX_THREADS
/*! @decl void thread_set_concurrency(int concurrency)
 *!
 *! @fixme
 *!   Document this function
 */
void f_thread_set_concurrency(INT32 args)
{
  int c=1;
  if(args)
    c=Pike_sp[-args].u.integer;
  else
    SIMPLE_TOO_FEW_ARGS_ERROR("thread_set_concurrency", 1);
  pop_n_elems(args);
  num_lwps=c;
  th_setconcurrency(c);
}
#endif

/*! @decl Thread.Thread this_thread()
 *!
 *! This function returns the object that identifies this thread.
 *!
 *! @seealso
 *! @[Thread()]
 */
PMOD_EXPORT void f_this_thread(INT32 args)
{
  pop_n_elems(args);
  if (Pike_interpreter.thread_state &&
      Pike_interpreter.thread_state->thread_obj) {
    ref_push_object(Pike_interpreter.thread_state->thread_obj);
  } else {
    /* Threads not enabled yet/anylonger */
    push_undefined();
  }
}

/*! @decl int(0..) get_thread_quanta()
 *!
 *! @returns
 *!   Returns the current thread quanta in nanoseconds.
 *!
 *! @seealso
 *!   @[set_thread_quanta()], @[gethrtime()]
 */
static void f_get_thread_quanta(INT32 args)
{
  pop_n_elems(args);
  push_int64(thread_quanta);
#ifndef LONG_CPU_TIME_T
  /* Convert from ticks. */
  push_int(1000000000 / CPU_TIME_TICKS);
  o_multiply();
#endif
}

/*! @decl int(0..) set_thread_quanta(int(0..) ns)
 *!
 *! Set the thread quanta.
 *!
 *! @param ns
 *!   New thread quanta in nanoseconds.
 *!   A value of zero (default) disables the thread quanta checks.
 *!
 *! When enabled @[MasterObject.thread_quanta_exceeded()] will
 *! be called when a thread has spent more time than the quanta
 *! without allowing another thread to run.
 *!
 *! @note
 *!   Setting a non-zero value that is too small to allow for
 *!   @[MasterObject.thread_quanta_exceeded()] to run is NOT a
 *!   good idea.
 *!
 *! @returns
 *!   Returns the previous thread quanta in nanoseconds.
 *!
 *! @seealso
 *!   @[set_thread_quanta()], @[gethrtime()]
 */
static void f_set_thread_quanta(INT32 args)
{
  LONGEST ns = 0;

#ifndef LONG_CPU_TIME_T
  /* Convert to ticks. */
  push_int(1000000000 / CPU_TIME_TICKS);
  o_divide();
#endif
  get_all_args("set_thread_quanta", args, "%l", &ns);
  pop_n_elems(args);

  push_int64(thread_quanta);
#ifndef LONG_CPU_TIME_T
  /* Convert from ticks. */
  push_int(1000000000 / CPU_TIME_TICKS);
  o_multiply();
#endif

  if (ns <= 0) ns = 0;

  thread_quanta = ns;

  if (Pike_interpreter.thread_state) {
    Pike_interpreter.thread_state->interval_start = get_real_time();
  }
}

#define THIS_MUTEX ((struct mutex_storage *)(CURRENT_STORAGE))


/* Note:
 * No reference is kept to the key object, it is destructed if the
 * mutex is destructed. The key pointer is set to zero by the
 * key object when the key is destructed.
 */

struct mutex_storage
{
  COND_T condition;
  struct object *key;
  int num_waiting;
};

struct key_storage
{
  struct mutex_storage *mut;
  struct object *mutex_obj;
  struct thread_state *owner;
  struct object *owner_obj;
  int initialized;
};

#define OB2KEY(X) ((struct key_storage *)((X)->storage))

/*! @class Mutex
 *!
 *! @[Mutex] is a class that implements mutual exclusion locks.
 *! Mutex locks are used to prevent multiple threads from simultaneously
 *! execute sections of code which access or change shared data. The basic
 *! operations for a mutex is locking and unlocking. If a thread attempts
 *! to lock an already locked mutex the thread will sleep until the mutex
 *! is unlocked.
 *!
 *! @note
 *!   This class is simulated when Pike is compiled without thread support,
 *!   so it's always available.
 *!
 *! In POSIX threads, mutex locks can only be unlocked by the same thread
 *! that locked them. In Pike any thread can unlock a locked mutex.
 */

/*! @decl MutexKey lock()
 *! @decl MutexKey lock(int type)
 *!
 *! This function attempts to lock the mutex. If the mutex is already
 *! locked by a different thread the current thread will sleep until the
 *! mutex is unlocked. The value returned is the 'key' to the lock. When
 *! the key is destructed or has no more references the mutex will
 *! automatically be unlocked.
 *!
 *! The @[type] argument specifies what @[lock()] should do if the
 *! mutex is already locked by this thread:
 *! @int
 *!   @value 0
 *!     Throw an error.
 *!   @value 1
 *!     Sleep until the mutex is unlocked. Useful if some
 *!     other thread will unlock it.
 *!   @value 2
 *!     Return zero. This allows recursion within a locked region of
 *!     code, but in conjunction with other locks it easily leads
 *!     to unspecified locking order and therefore a risk for deadlocks.
 *! @endint
 *!
 *! @note
 *! If the mutex is destructed while it's locked or while threads are
 *! waiting on it, it will continue to exist internally until the last
 *! thread has stopped waiting and the last @[MutexKey] has
 *! disappeared, but further calls to the functions in this class will
 *! fail as is usual for destructed objects.
 *!
 *! @note
 *! Pike 7.4 and earlier destructed any outstanding lock when the
 *! mutex was destructed, but threads waiting in @[lock] still got
 *! functioning locks as discussed above. This is inconsistent no
 *! matter how you look at it, so it was changed in 7.6. The old
 *! behavior is retained in compatibility mode for applications that
 *! explicitly destruct mutexes to unlock them.
 *!
 *! @seealso
 *!   @[trylock()]
 */
void f_mutex_lock(INT32 args)
{
  struct mutex_storage  *m;
  struct object *o;
  INT_TYPE type;

  DEBUG_CHECK_THREAD();

  m=THIS_MUTEX;
  if(!args)
    type=0;
  else
    get_all_args("lock",args,"%i",&type);

  switch(type)
  {
    default:
      bad_arg_error("lock", Pike_sp-args, args, 2, "int(0..2)", Pike_sp+1-args,
                    "Unknown mutex locking style: %"PRINTPIKEINT"d\n",type);
      

    case 0:
    case 2:
      if(m->key && OB2KEY(m->key)->owner == Pike_interpreter.thread_state)
      {
	THREADS_FPRINTF(0,
			(stderr, "Recursive LOCK k:%p, m:%p(%p), t:%p\n",
			 OB2KEY(m->key), m, OB2KEY(m->key)->mut,
			 Pike_interpreter.thread_state));

	if(type==0) Pike_error("Recursive mutex locks!\n");

	pop_n_elems(args);
	push_int(0);
	return;
      }
    case 1:
      break;
  }

  /* Needs to be cloned here, since create()
   * might use threads.
   */
  o=fast_clone_object(mutex_key);

  DEBUG_CHECK_THREAD();

  if(m->key)
  {
    m->num_waiting++;
    if(threads_disabled)
    {
      free_object(o);
      Pike_error("Cannot wait for mutexes when threads are disabled!\n");
    }
    do
    {
      THREADS_FPRINTF(1, (stderr,"WAITING TO LOCK m:%p\n",m));
      SWAP_OUT_CURRENT_THREAD();
      co_wait_interpreter(& m->condition);
      SWAP_IN_CURRENT_THREAD();
      check_threads_etc();
    }while(m->key);
    m->num_waiting--;
  }

#ifdef PICKY_MUTEX
  if (!Pike_fp->current_object->prog) {
    free_object (o);
    if (!m->num_waiting) {
      co_destroy (&m->condition);
    }
    Pike_error ("Mutex was destructed while waiting for lock.\n");
  }
#endif

  m->key=o;
  OB2KEY(o)->mut=m;
  add_ref (OB2KEY(o)->mutex_obj = Pike_fp->current_object);

  DEBUG_CHECK_THREAD();

  THREADS_FPRINTF(1, (stderr, "LOCK k:%p, m:%p(%p), t:%p\n",
		      OB2KEY(o), m, OB2KEY(m->key)->mut,
		      Pike_interpreter.thread_state));
  pop_n_elems(args);
  push_object(o);
}

/*! @decl MutexKey trylock()
 *! @decl MutexKey trylock(int type)
 *!
 *! This function performs the same operation as @[lock()], but if the mutex
 *! is already locked, it will return zero instead of sleeping until it's
 *! unlocked.
 *!
 *! @seealso
 *!   @[lock()]
 */
void f_mutex_trylock(INT32 args)
{
  struct mutex_storage  *m;
  struct object *o;
  INT_TYPE type;
  int i=0;

  /* No reason to release the interpreter lock here
   * since we aren't calling any functions that take time.
   */

  m=THIS_MUTEX;

  if(!args)
    type=0;
  else
    get_all_args("trylock",args,"%i",&type);

  switch(type)
  {
    default:
      bad_arg_error("trylock", Pike_sp-args, args, 2, "int(0..2)", Pike_sp+1-args,
                    "Unknown mutex locking style: %"PRINTPIKEINT"d\n",type);

    case 0:
      if(m->key && OB2KEY(m->key)->owner == Pike_interpreter.thread_state)
      {
	Pike_error("Recursive mutex locks!\n");
      }

    case 2:
    case 1:
      break;
  }

  o=clone_object(mutex_key,0);

  if(!m->key)
  {
    OB2KEY(o)->mut=m;
    add_ref (OB2KEY(o)->mutex_obj = Pike_fp->current_object);
    m->key=o;
    i=1;
  }
  
  pop_n_elems(args);
  if(i)
  {
    push_object(o);
  } else {
    destruct(o);
    free_object(o);
    push_int(0);
  }
}

/*! @decl Thread.Thread current_locking_thread()
 *!
 *! This mutex method returns the object that identifies the thread that
 *! has locked the mutex. 0 is returned if the mutex isn't locked.
 *!
 *! @seealso
 *! @[Thread()]
 */
PMOD_EXPORT void f_mutex_locking_thread(INT32 args)
{
  struct mutex_storage *m = THIS_MUTEX;

  pop_n_elems(args);

  if (m->key && OB2KEY(m->key)->owner)
    ref_push_object(OB2KEY(m->key)->owner->thread_obj);
  else
    push_int(0);
}

/*! @decl Thread.MutexKey current_locking_key()
 *!
 *! This mutex method returns the key object currently governing
 *! the lock on this mutex. 0 is returned if the mutex isn't locked.
 *!
 *! @seealso
 *! @[Thread()]
 */
PMOD_EXPORT void f_mutex_locking_key(INT32 args)
{
  struct mutex_storage *m = THIS_MUTEX;

  pop_n_elems(args);

  if (m->key)
    ref_push_object(m->key);
  else
    push_int(0);
}

/*! @decl protected string _sprintf(int c)
 *!
 *! Describes the mutex including the thread that currently holds the lock
 *! (if any).
 */
void f_mutex__sprintf (INT32 args)
{
  struct mutex_storage *m = THIS_MUTEX;
  int c = 0;
  if(args>0 && TYPEOF(Pike_sp[-args]) == PIKE_T_INT)
    c = Pike_sp[-args].u.integer;
  pop_n_elems (args);
  if(c != 'O') {
    push_undefined();
    return;
  }
  if (m->key && OB2KEY(m->key)->owner) {
    push_text("Thread.Mutex(/*locked by 0x");
    push_int64(PTR_TO_INT(THREAD_T_TO_PTR(OB2KEY(m->key)->owner->id)));
    f_int2hex(1);
    push_text("*/)");
    f_add(3);
  } else {
    push_text("Thread.Mutex()");
  }
}

void init_mutex_obj(struct object *UNUSED(o))
{
  co_init(& THIS_MUTEX->condition);
  THIS_MUTEX->key=0;
  THIS_MUTEX->num_waiting = 0;
}

void exit_mutex_obj(struct object *UNUSED(o))
{
  struct mutex_storage *m = THIS_MUTEX;
  struct object *key = m->key;

  THREADS_FPRINTF(1, (stderr, "DESTROYING MUTEX m:%p\n", THIS_MUTEX));

#ifndef PICKY_MUTEX
  if (key) {
    /* The last key will destroy m->condition in its exit hook. */
    THREADS_FPRINTF(1, (stderr, "Destructed mutex is in use - delaying cleanup\n"));
  }
  else {
#ifdef PIKE_DEBUG
    if (m->num_waiting)
      Pike_error ("key/num_waiting confusion.\n");
#endif
    co_destroy(& m->condition);
  }
#else
  if(key) {
    m->key=0;
    destruct(key); /* Will destroy m->condition if m->num_waiting is zero. */
    if(m->num_waiting)
    {
      THREADS_FPRINTF(1, (stderr, "Destructed mutex is being waited on.\n"));
      THREADS_ALLOW();
      /* exit_mutex_key_obj has already signalled, but since the
       * waiting threads will throw an error instead of making a new
       * lock we need to double it to a broadcast. The last thread
       * that stops waiting will destroy m->condition. */
      co_broadcast (&m->condition);

      /* Try to wake up the waiting thread(s) immediately
       * in an attempt to avoid starvation.
       */
#ifdef HAVE_NO_YIELD
      sleep(0);
#else /* HAVE_NO_YIELD */
      th_yield();
#endif /* HAVE_NO_YIELD */
      THREADS_DISALLOW();
    }
  }
  else {
    co_destroy(& m->condition);
  }
#endif
}

void exit_mutex_obj_compat_7_4(struct object *UNUSED(o))
{
  struct mutex_storage *m = THIS_MUTEX;
  struct object *key = m->key;

  THREADS_FPRINTF(1, (stderr, "DESTROYING MUTEX m:%p\n", THIS_MUTEX));

  if(key) {
    m->key=0;
    destruct(key); /* Will destroy m->condition if m->num_waiting is zero. */
  }
  else {
#ifdef PIKE_DEBUG
    if (m->num_waiting)
      Pike_error ("key/num_waiting confusion.\n");
#endif
    co_destroy(& m->condition);
  }
}

/*! @endclass
 */

/*! @class MutexKey
 *!
 *! Objects of this class are returned by @[Mutex()->lock()]
 *! and @[Mutex()->trylock()]. They are also passed as arguments
 *! to @[Condition()->wait()].
 *!
 *! As long as they are held, the corresponding mutex will be locked.
 *!
 *! The corresponding mutex will be unlocked when the object
 *! is destructed (eg by not having any references left).
 *!
 *! @seealso
 *!   @[Mutex], @[Condition]
 */
#define THIS_KEY ((struct key_storage *)(CURRENT_STORAGE))
void init_mutex_key_obj(struct object *UNUSED(o))
{
  THREADS_FPRINTF(1, (stderr, "KEY k:%p, t:%p\n",
		      THIS_KEY, Pike_interpreter.thread_state));
  THIS_KEY->mut=0;
  THIS_KEY->mutex_obj = NULL;
  THIS_KEY->owner = Pike_interpreter.thread_state;
  THIS_KEY->owner_obj = Pike_interpreter.thread_state->thread_obj;
  if (THIS_KEY->owner_obj)
    add_ref(THIS_KEY->owner_obj);
  THIS_KEY->initialized=1;
}

void exit_mutex_key_obj(struct object *DEBUGUSED(o))
{
  THREADS_FPRINTF(1, (stderr, "UNLOCK k:%p m:(%p) t:%p o:%p\n",
		      THIS_KEY, THIS_KEY->mut,
		      Pike_interpreter.thread_state, THIS_KEY->owner));
  if(THIS_KEY->mut)
  {
    struct mutex_storage *mut = THIS_KEY->mut;
    struct object *mutex_obj;

#ifdef PIKE_DEBUG
    /* Note: mut->key can be NULL if our corresponding mutex
     *       has been destructed.
     */
    if(mut->key && (mut->key != o))
      Pike_fatal("Mutex unlock from wrong key %p != %p!\n",
		 THIS_KEY->mut->key, o);
#endif
    mut->key=0;
    if (THIS_KEY->owner) {
      THIS_KEY->owner = NULL;
    }
    if (THIS_KEY->owner_obj) {
      free_object(THIS_KEY->owner_obj);
      THIS_KEY->owner_obj=0;
    }
    THIS_KEY->mut=0;
    THIS_KEY->initialized=0;
    mutex_obj = THIS_KEY->mutex_obj;
    THIS_KEY->mutex_obj = NULL;

    if (mut->num_waiting) {
      THREADS_ALLOW();
      co_signal(&mut->condition);

      /* Try to wake up the waiting thread(s) immediately
       * in an attempt to avoid starvation.
       */
#ifdef HAVE_NO_YIELD
      sleep(0);
#else /* HAVE_NO_YIELD */
      th_yield();
#endif /* HAVE_NO_YIELD */
      THREADS_DISALLOW();
    } else if (mutex_obj && !mutex_obj->prog) {
      co_destroy (&mut->condition);
    }

    if (mutex_obj)
      free_object(mutex_obj);
  }
}

static void f_mutex_key__sprintf(INT32 args)
{
  int c = 0;
  if(args>0 && TYPEOF(Pike_sp[-args]) == PIKE_T_INT)
    c = Pike_sp[-args].u.integer;
  pop_n_elems (args);
  if(c != 'O') {
    push_undefined();
    return;
  }
  if (THIS_KEY->mutex_obj) {
    push_text("MutexKey(/* %O */)");
    ref_push_object(THIS_KEY->mutex_obj);
    f_sprintf(2);
  } else {
    push_text("MutexKey()");
  }
}

/*! @endclass
 */

/*! @class Condition
 *!
 *! Implementation of condition variables.
 *!
 *! Condition variables are used by threaded programs
 *! to wait for events happening in other threads.
 *!
 *! In order to prevent races (which is the whole point of condition
 *! variables), the modification of a shared resource and the signal notifying
 *! modification of the resource must be performed inside the same mutex
 *! lock, and the examining of the resource and waiting for a signal that
 *! the resource has changed based on that examination must also happen
 *! inside the same mutex lock.
 *!
 *! Typical wait operation:
 *! @ol
 *!  @item 
 *!	Take mutex lock
 *!  @item 
 *!	Read/write shared resource
 *!  @item 
 *!	Wait for the signal with the mutex lock in released state
 *!  @item 
 *!	Reacquire mutex lock
 *!  @item 
 *!	If needed, jump back to step 2 again
 *!  @item 
 *!	Release mutex lock
 *! @endol
 *!
 *! Typical signal operation:
 *! @ol
 *!  @item 
 *!	Take mutex lock
 *!  @item 
 *! 	Read/write shared resource
 *!  @item 
 *!	Send signal
 *!  @item 
 *!	Release mutex lock
 *! @endol
 *!
 *! @example
 *!   You have some resource that multiple treads want to use.  To
 *!   protect this resource for simultaneous access, you create a shared mutex.
 *!   Before you read or write the resource, you take the mutex so that you
 *!   get a consistent and private view of / control over it.  When you decide
 *!   that the resource is not in the state you want it, and that you need
 *!   to wait for some other thread to modify the state for you before you
 *!   can continue, you wait on the conditional variable, which will
 *!   temporarily relinquish the mutex during the wait.  This way a
 *!   different thread can take the mutex, update the state of the resource,
 *!   and then signal the condition (which does not in itself release the
 *!   mutex, but the signalled thread will be next in line once the mutex is
 *!   released).
 *!
 *! @note
 *!   Condition variables are only available on systems with thread
 *!   support. The Condition class is not simulated otherwise, since that
 *!   can't be done accurately without continuations.
 *!
 *! @seealso
 *!   @[Mutex]
 */

struct pike_cond {
  COND_T cond;
  int wait_count;
};

#define THIS_COND ((struct pike_cond *)(CURRENT_STORAGE))

/*! @decl void wait(Thread.MutexKey mutex_key)
 *! @decl void wait(Thread.MutexKey mutex_key, int(0..)|float seconds)
 *! @decl void wait(Thread.MutexKey mutex_key, int(0..) seconds, @
 *!                 int(0..999999999) nanos)
 *!
 *! Wait for condition.
 *!
 *! This function makes the current thread sleep until the condition
 *! variable is signalled or the timeout is reached.
 *!
 *! @param mutex_key
 *!   A @[Thread.MutexKey] object for a @[Thread.Mutex]. It will be
 *!   unlocked atomically before waiting for the signal and then
 *!   relocked atomically when the signal is received or the timeout
 *!   is reached.
 *!
 *! @param seconds
 *!   Seconds to wait before the timeout is reached.
 *!
 *! @param nanos
 *!   Nano (1/1000000000) seconds to wait before the timeout is reached.
 *!   This value is added to the number of seconds specified by @[seconds].
 *!
 *! A timeout of zero seconds disables the timeout.
 *!
 *! The thread that sends the signal should have the mutex locked
 *! while sending it. Otherwise it's impossible to avoid races where
 *! signals are sent while the listener(s) haven't arrived to the
 *! @[wait] calls yet.
 *!
 *! @note
 *!   The support for timeouts was added in Pike 7.8.121, which was
 *!   after the first public release of Pike 7.8.
 *!
 *! @note
 *!   Note that the timeout is approximate (best effort), and may
 *!   be exceeded if eg the mutex is busy after the timeout.
 *!
 *! @note
 *!   In Pike 7.2 and earlier it was possible to call @[wait()]
 *!   without arguments. This possibility was removed in later
 *!   versions since it unavoidably leads to programs with races
 *!   and/or deadlocks.
 *!
 *! @note
 *!   Note also that any threads waiting on the condition will be
 *!   woken up when it gets destructed.
 *!
 *! @seealso
 *!   @[Mutex->lock()]
 */
void f_cond_wait(INT32 args)
{
  struct object *key, *mutex_obj;
  struct mutex_storage *mut;
  struct pike_cond *c;
  INT_TYPE seconds = 0, nanos = 0;

  if(threads_disabled)
    Pike_error("Cannot wait for conditions when threads are disabled!\n");

  if (args <= 2) {
    FLOAT_TYPE fsecs = 0.0;
    get_all_args("wait", args, "%o.%F", &key, &fsecs);
    seconds = (INT_TYPE) fsecs;
    nanos = (INT_TYPE)((fsecs - seconds)*1000000000);
  } else {
    /* FIXME: Support bignum nanos. */
    get_all_args("wait", args, "%o%i%i", &key, &seconds, &nanos);
  }
      
  if ((key->prog != mutex_key) ||
      (!(OB2KEY(key)->initialized)) ||
      (!(mut = OB2KEY(key)->mut))) {
    Pike_error("Bad argument 1 to wait()\n");
  }

  if(args > 1) {
    pop_n_elems(args - 1);
    args = 1;
  }

  c = THIS_COND;

  /* Unlock mutex */
  mutex_obj = OB2KEY(key)->mutex_obj;
  mut->key=0;
  OB2KEY(key)->mut=0;
  OB2KEY(key)->mutex_obj = NULL;
  co_signal(& mut->condition);
    
  /* Wait and allow mutex operations */
  SWAP_OUT_CURRENT_THREAD();
  c->wait_count++;
  if (seconds || nanos) {
    co_wait_interpreter_timeout(&(c->cond), seconds, nanos);
  } else {
    co_wait_interpreter(&(c->cond));
  }
  c->wait_count--;
  SWAP_IN_CURRENT_THREAD();
    
  if (!mutex_obj->prog) {
    Pike_error("Mutex was destructed while waiting for cond.\n");
  }

  /* Lock mutex */
  mut->num_waiting++;
  while(mut->key) {
    SWAP_OUT_CURRENT_THREAD();
    co_wait_interpreter(& mut->condition);
    SWAP_IN_CURRENT_THREAD();
    check_threads_etc();
  }
  mut->key=key;
  OB2KEY(key)->mut=mut;
  OB2KEY(key)->mutex_obj = mutex_obj;
  mut->num_waiting--;

  pop_stack();
  return;
}

/*! @decl void signal()
 *!
 *! @[signal()] wakes up one of the threads currently waiting for the
 *! condition.
 *!
 *! @note
 *!   Sometimes more than one thread is woken up.
 *!
 *! @seealso
 *!   @[broadcast()]
 */
void f_cond_signal(INT32 args)
{
  pop_n_elems(args);
  co_signal(&(THIS_COND->cond));
}

/*! @decl void broadcast()
 *!
 *! @[broadcast()] wakes up all threads currently waiting for this condition.
 *!
 *! @seealso
 *!   @[signal()]
 */
void f_cond_broadcast(INT32 args)
{
  pop_n_elems(args);
  co_broadcast(&(THIS_COND->cond));
}

void init_cond_obj(struct object *UNUSED(o))
{
  co_init(&(THIS_COND->cond));
  THIS_COND->wait_count = 0;
}

void exit_cond_obj(struct object *UNUSED(o))
{
  /* Wake up any threads that might be waiting on this cond.
   *
   * Note that we are already destructed (o->prog == NULL),
   * so wait_count can't increase.
   *
   * FIXME: This code wouldn't be needed if exit callbacks were called
   *        only when the ref count reaches zero.
   *	/grubba 2006-01-29
   */
  while (THIS_COND->wait_count) {
    co_broadcast(&(THIS_COND->cond));

    THREADS_ALLOW();
#ifdef HAVE_NO_YIELD
    sleep(0);
#else /* HAVE_NO_YIELD */
    th_yield();
#endif /* HAVE_NO_YIELD */
    THREADS_DISALLOW();
  }
  co_destroy(&(THIS_COND->cond));
}

/*! @endclass
 */

/*! @class Thread
 */

/*! @decl array(mixed) backtrace()
 *!
 *! Returns the current call stack for the thread.
 *!
 *! @returns
 *!   The result has the same format as for @[predef::backtrace()].
 *!
 *! @seealso
 *!   @[predef::backtrace()]
 */
void f_thread_backtrace(INT32 args)
{
  void low_backtrace(struct Pike_interpreter_struct *);
  struct thread_state *foo = THIS_THREAD;

  pop_n_elems(args);

  if(foo == Pike_interpreter.thread_state)
  {
    f_backtrace(0);
  }
  else if(foo->state.stack_pointer)
  {
    low_backtrace(& foo->state);
  }
  else
  {
    push_int(0);
    f_allocate(1);
  }
}

/*! @decl int status()
 *!
 *! Returns the status of the thread.
 *!
 *! @returns
 *!   @int
 *!     @value Thread.THREAD_NOT_STARTED
 *!     @value Thread.THREAD_RUNNING
 *!     @value Thread.THREAD_EXITED
 *!     @value Thread.THREAD_ABORTED
 *!   @endint
 */
void f_thread_id_status(INT32 args)
{
  pop_n_elems(args);
  push_int(THIS_THREAD->status);
}

/*! @decl protected string _sprintf(int c)
 *!
 *! Returns a string identifying the thread.
 */
void f_thread_id__sprintf (INT32 args)
{
  int c = 0;
  if(args>0 && TYPEOF(Pike_sp[-args]) == PIKE_T_INT)
    c = Pike_sp[-args].u.integer;
  pop_n_elems (args);
  if(c != 'O') {
    push_undefined();
    return;
  }
  push_text ("Thread.Thread(0x");
  push_int64(PTR_TO_INT(THREAD_T_TO_PTR(THIS_THREAD->id)));
  f_int2hex(1);
  push_text (")");
  f_add (3);
}

/*! @decl protected int id_number()
 *!
 *! Returns an id number identifying the thread.
 *!
 *! @note
 *!   This function was added in Pike 7.2.204.
 */
void f_thread_id_id_number(INT32 args)
{
  pop_n_elems(args);
  push_int64(PTR_TO_INT(THREAD_T_TO_PTR(THIS_THREAD->id)));
}

/*! @decl mixed wait()
 *!
 *! Waits for the thread to complete, and then returns
 *! the value returned from the thread function.
 *!
 *! @throws
 *!   Rethrows the error thrown by the thread if it exited by
 *!   throwing an error.
 */
static void f_thread_id_result(INT32 UNUSED(args))
{
  struct thread_state *th=THIS_THREAD;
  int th_status;

  if (threads_disabled) {
    Pike_error("Cannot wait for threads when threads are disabled!\n");
  }

  th->waiting++;

  THREADS_FPRINTF(0, (stderr, "Thread->wait(): Waiting for thread_state %p "
		      "(state:%d).\n", th, th->status));
  while(th->status < THREAD_EXITED) {
    SWAP_OUT_CURRENT_THREAD();
    co_wait_interpreter(&th->status_change);
    SWAP_IN_CURRENT_THREAD();
    check_threads_etc();
    THREADS_FPRINTF(0,
		    (stderr, "Thread->wait(): Waiting for thread_state %p "
		     "(state:%d).\n", th, th->status));
  }

  th_status = th->status;

  assign_svalue_no_free(Pike_sp, &th->result);
  Pike_sp++;
  dmalloc_touch_svalue(Pike_sp-1);

  th->waiting--;

  if (!th->thread_obj)
    /* Do this only if exit_thread_obj already has run. */
    cleanup_thread_state (th);

  if (th_status == THREAD_ABORTED) f_throw(1);
}

static int num_pending_interrupts = 0;
static struct callback *thread_interrupt_callback = NULL;

static void check_thread_interrupt(struct callback *foo,
				   void *UNUSED(bar), void *UNUSED(gazonk))
{
  if (Pike_interpreter.thread_state->flags & THREAD_FLAG_INHIBIT) {
    return;
  }
  if (Pike_interpreter.thread_state->flags & THREAD_FLAG_SIGNAL_MASK) {
    if (Pike_interpreter.thread_state->flags & THREAD_FLAG_TERM) {
      throw_severity = THROW_THREAD_EXIT;
    } else {
      throw_severity = THROW_ERROR;
    }
    Pike_interpreter.thread_state->flags &= ~THREAD_FLAG_SIGNAL_MASK;
    if (!--num_pending_interrupts) {
      remove_callback(foo);
      thread_interrupt_callback = NULL;
    }
    if (throw_severity == THROW_ERROR) {
      Pike_error("Interrupted.\n");
    } else {
      push_int(-1);
      assign_svalue(&throw_value, Pike_sp-1);
      pike_throw();
    }
  }
}

/*! @decl void interrupt()
 *! @decl void interrupt(string msg)
 *!
 *! Interrupt the thread with the message @[msg].
 *!
 *! @fixme
 *!   The argument @[msg] is currently ignored.
 *!
 *! @note
 *!   Interrupts are asynchronous, and are currently not queued.
 */
static void f_thread_id_interrupt(INT32 args)
{
  /* FIXME: The msg argument is not supported yet. */
  pop_n_elems(args);

  if (!(THIS_THREAD->flags & THREAD_FLAG_SIGNAL_MASK)) {
    num_pending_interrupts++;
    if (!thread_interrupt_callback) {
      thread_interrupt_callback =
	add_to_callback(&evaluator_callbacks, check_thread_interrupt, 0, 0);
    }
    /* Actually interrupt the thread. */
    th_kill(THIS_THREAD->id, SIGCHLD);
  }
  THIS_THREAD->flags |= THREAD_FLAG_INTR;
  push_int(0);
}

static void low_thread_kill (struct thread_state *th)
{
  if (!(th->flags & THREAD_FLAG_SIGNAL_MASK)) {
    num_pending_interrupts++;
    if (!thread_interrupt_callback) {
      thread_interrupt_callback =
	add_to_callback(&evaluator_callbacks, check_thread_interrupt, 0, 0);
    }
    /* Actually interrupt the thread. */
    th_kill(th->id, SIGCHLD);
  }
  th->flags |= THREAD_FLAG_TERM;
}

/*! @decl void kill()
 *!
 *! Interrupt the thread, and terminate it.
 *!
 *! @note
 *!   Interrupts are asynchronous, and are currently not queued.
 */
static void f_thread_id_kill(INT32 args)
{
  pop_n_elems(args);
  low_thread_kill (THIS_THREAD);
  push_int(0);
}

void init_thread_obj(struct object *UNUSED(o))
{
  memset(&THIS_THREAD->state, 0, sizeof(struct Pike_interpreter_struct));
  THIS_THREAD->thread_obj = Pike_fp->current_object;
  THIS_THREAD->swapped = 0;
  THIS_THREAD->status=THREAD_NOT_STARTED;
  THIS_THREAD->flags = 0;
  THIS_THREAD->waiting = 0;
  SET_SVAL(THIS_THREAD->result, T_INT, NUMBER_UNDEFINED, integer, 0);
  co_init(& THIS_THREAD->status_change);
  THIS_THREAD->thread_locals=NULL;
#ifdef CPU_TIME_MIGHT_BE_THREAD_LOCAL
  THIS_THREAD->auto_gc_time = 0;
#endif
}

static void cleanup_thread_state (struct thread_state *th)
{
#ifdef PIKE_DEBUG
  if (th->thread_obj) Pike_fatal ("Thread state still active.\n");
#endif

  /* Don't clean up if the thread is running or if someone is waiting
   * on status_change. They should call cleanup_thread_state later. */
  if (th->status == THREAD_RUNNING || th->waiting)
    return;

  if (th->flags & THREAD_FLAG_SIGNAL_MASK) {
    Pike_interpreter.thread_state->flags &= ~THREAD_FLAG_SIGNAL_MASK;
    if (!--num_pending_interrupts) {
      remove_callback(thread_interrupt_callback);
      thread_interrupt_callback = NULL;
    }
  }

  co_destroy(& th->status_change);
  th_destroy(& th->id);
}

void exit_thread_obj(struct object *UNUSED(o))
{
  THIS_THREAD->thread_obj = NULL;

  cleanup_thread_state (THIS_THREAD);

  if(THIS_THREAD->thread_locals != NULL) {
    free_mapping(THIS_THREAD->thread_locals);
    THIS_THREAD->thread_locals = NULL;
  }
}

/*! @endclass
 */

static void thread_was_recursed(struct object *UNUSED(o))
{
  struct thread_state *tmp=THIS_THREAD;
  if(tmp->thread_locals != NULL)
    gc_recurse_mapping(tmp->thread_locals);
}

static void thread_was_checked(struct object *UNUSED(o))
{
  struct thread_state *tmp=THIS_THREAD;
  if(tmp->thread_locals != NULL)
    debug_gc_check (tmp->thread_locals,
		    " as mapping for thread local values in thread");

#ifdef PIKE_DEBUG
  if(tmp->swapped)
    gc_mark_stack_external (tmp->state.frame_pointer, tmp->state.stack_pointer,
			    tmp->state.evaluator_stack);
#endif
}

/*! @class Local
 *!
 *! Thread local variable storage.
 *!
 *! This class allows you to have variables which are separate for each
 *! thread that uses it. It has two methods: @[get()] and @[set()]. A value
 *! stored in an instance of @[Local] can only be retrieved by that
 *! same thread.
 *!
 *! @note
 *!   This class is simulated when Pike is compiled without thread support,
 *!   so it's always available.
 */

/* FIXME: Why not use an init callback()? */
void f_thread_local_create( INT32 args )
{
  static INT32 thread_local_id = 0;
  ((struct thread_local_var *)CURRENT_STORAGE)->id =
    thread_local_id++;
  pop_n_elems(args);
  push_int(0);
}

PMOD_EXPORT void f_thread_local(INT32 args)
{
  struct object *loc = clone_object(thread_local_prog,0);
  pop_n_elems(args);
  push_object(loc);
}

/*! @decl mixed get()
 *!
 *! Get the thread local value.
 *!
 *! This returns the value prevoiusly stored in the @[Local] object by
 *! the @[set()] method by this thread.
 *!
 *! @seealso
 *!   @[set()]
 */
void f_thread_local_get(INT32 args)
{
  struct svalue key;
  struct mapping *m;
  SET_SVAL(key, T_INT, NUMBER_NUMBER, integer,
	   ((struct thread_local_var *)CURRENT_STORAGE)->id);
  pop_n_elems(args);
  if(Pike_interpreter.thread_state != NULL &&
     (m = Pike_interpreter.thread_state->thread_locals) != NULL)
    mapping_index_no_free(Pike_sp++, m, &key);
  else {
    push_undefined();
  }
}

/*! @decl mixed set(mixed value)
 *!
 *! Set the thread local value.
 *!
 *! This sets the value returned by the @[get] method.
 *!
 *! Calling this method does not affect the value returned by @[get()] when
 *! it's called by another thread (ie multiple values can be stored at the
 *! same time, but only one value per thread).
 *!
 *! @returns
 *!   This function returns its argument.
 *!
 *! @note
 *!   Note that the value set can only be retreived by the same thread.
 *!
 *! @seealso
 *!   @[get()]
 */
void f_thread_local_set(INT32 args)
{
  struct svalue key;
  struct mapping *m;
  SET_SVAL(key, T_INT, NUMBER_NUMBER, integer,
	   ((struct thread_local_var *)CURRENT_STORAGE)->id);
  if(args>1)
    pop_n_elems(args-1);
  else if(args<1)
    SIMPLE_TOO_FEW_ARGS_ERROR("Thread.Local.set", 1);

  if(Pike_interpreter.thread_state == NULL)
    Pike_error("Trying to set Thread.Local without thread!\n");

  if((m = Pike_interpreter.thread_state->thread_locals) == NULL)
    m = Pike_interpreter.thread_state->thread_locals =
      allocate_mapping(4);

  mapping_insert(m, &key, &Pike_sp[-1]);
}

#ifdef PIKE_DEBUG
void gc_check_thread_local (struct object *UNUSED(o))
{
  /* Only used by with locate_references. */
  if (Pike_in_gc == GC_PASS_LOCATE) {
    struct svalue key, *val;
    struct thread_state *s;

    SET_SVAL(key, T_INT, NUMBER_NUMBER, integer,
	     ((struct thread_local_var *)CURRENT_STORAGE)->id);

    FOR_EACH_THREAD (s, {
	if (s->thread_locals &&
	    (val = low_mapping_lookup(s->thread_locals, &key)))
	  debug_gc_check_svalues (val, 1,
				  " as thread local value in Thread.Local object"
				  " (indirect ref)");
      });
  }
}
#endif

/*! @endclass
 */

/*! @endmodule
 */

/* Thread farm code by Per
 * 
 */
static struct farmer {
  struct farmer *neighbour;
  void *field;
  void (*harvest)(void *);
  THREAD_T me;
  COND_T harvest_moon;
#ifdef HAVE_BROKEN_LINUX_THREAD_EUID
  int euid, egid;
#endif /* HAVE_BROKEN_LINUX_THREAD_EUID */
} *farmers;

static MUTEX_T rosie;


static int _num_farmers, _num_idle_farmers;

static TH_RETURN_TYPE farm(void *_a)
{
  struct farmer *me = (struct farmer *)_a;

#ifdef HAVE_BROKEN_LINUX_THREAD_EUID
  /* Work-around for Linux's pthreads not propagating the
   * effective uid & gid.
   */
  if (!geteuid()) {
#if defined(HAVE_PRCTL) && defined(PR_SET_DUMPABLE)
    /* The sete?id calls will clear the dumpable state that we might
     * have set with system.dumpable. */
    int current = prctl(PR_GET_DUMPABLE);
#endif
#ifdef HAVE_BROKEN_LINUX_THREAD_EUID
    if( setegid(me->egid) != 0 || seteuid(me->euid) != 0 )
    {
      fprintf (stderr, "%s:%d: Unexpected error from setegid(2). errno=%d\n",
	       __FILE__, __LINE__, errno);
    }
#endif
#if defined(HAVE_PRCTL) && defined(PR_SET_DUMPABLE)
    if (prctl(PR_SET_DUMPABLE, current) == -1)
      Pike_fatal ("Didn't expect prctl to go wrong. errno=%d\n", errno);
#endif
  }
#endif /* HAVE_BROKEN_LINUX_THREAD_EUID */

  do
  {
/*     if(farmers == me) Pike_fatal("Ouch!\n"); */
/*     fprintf(stderr, "farm_begin %p\n",me ); */
    me->harvest( me->field );
/*     fprintf(stderr, "farm_end %p\n", me); */

    me->harvest = 0;
    mt_lock( &rosie );
    if( ++_num_idle_farmers > 16 )
    {
      --_num_idle_farmers;
      --_num_farmers;
      mt_unlock( &rosie );
      free( me );
      return 0;
    }
    me->neighbour = farmers;
    farmers = me;
/*     fprintf(stderr, "farm_wait %p\n", me); */
    while(!me->harvest) co_wait( &me->harvest_moon, &rosie );
    --_num_idle_farmers;
    mt_unlock( &rosie );
/*     fprintf(stderr, "farm_endwait %p\n", me); */
  } while(1);
  /* NOT_REACHED */
  return 0;/* Keep the compiler happy. */
}

int th_num_idle_farmers(void)
{
  return _num_idle_farmers;
}


int th_num_farmers(void)
{
  return _num_farmers;
}

static struct farmer *new_farmer(void (*fun)(void *), void *args)
{
  struct farmer *me = malloc(sizeof(struct farmer));

  if (!me) {
    /* Out of memory */
    Pike_fatal("new_farmer(): Out of memory!\n");
  }

  dmalloc_accept_leak(me);

  _num_farmers++;
  me->neighbour = 0;
  me->field = args;
  me->harvest = fun;
  co_init( &me->harvest_moon );

#ifdef HAVE_BROKEN_LINUX_THREAD_EUID
  me->euid = geteuid();
  me->egid = getegid();
#endif /* HAVE_BROKEN_LINUX_THREAD_EUID */

  th_create_small(&me->me, farm, me);
  return me;
}

PMOD_EXPORT void th_farm(void (*fun)(void *), void *here)
{
#ifdef PIKE_DEBUG
  if(!fun) Pike_fatal("The farmers don't known how to handle empty fields\n");
#endif
  mt_lock( &rosie );
  if(farmers)
  {
    struct farmer *f = farmers;
    farmers = f->neighbour;
    f->field = here;
    f->harvest = fun;
    mt_unlock( &rosie );
    co_signal( &f->harvest_moon );
    return;
  }
  mt_unlock( &rosie );
  new_farmer( fun, here );
}

/*
 * Glue code.
 */

void low_th_init(void)
{
  THREADS_FPRINTF(0, (stderr, "Initializing threads.\n"));

  really_low_th_init();

  mt_init( & interpreter_lock);
  mt_init( & interpreter_lock_wanted);
  low_mt_lock_interpreter();
  mt_init( & thread_table_lock);
  mt_init( & interleave_lock);
  mt_init( & rosie);
  co_init( & live_threads_change);
  co_init( & threads_disabled_change);
  thread_table_init();

#if defined(RDTSC) && defined(USE_CLOCK_FOR_SLICES)
  {
    INT32 cpuid[4];
    x86_get_cpuid (1, cpuid);
    /* fprintf (stderr, "cpuid 1: %x\n", cpuid[2]); */
    use_tsc_for_slices = cpuid[2] & 0x10; /* TSC exists */
#if 0
    /* Skip tsc invariant check - the current tsc interval method
     * should be robust enough to cope with variable tsc rates. */
    if (use_tsc_for_slices) {
      x86_get_cpuid (0x80000007, cpuid);
      /* fprintf (stderr, "cpuid 0x80000007: %x\n", cpuid[2]); */
      use_tsc_for_slices = cpuid[2] & 0x100; /* TSC is invariant */
    }
#endif
    /* fprintf (stderr, "use tsc: %d\n", use_tsc_for_slices); */
  }
#endif

  th_running = 1;
}

static struct object *backend_thread_obj = NULL;

static struct Pike_interpreter_struct *original_interpreter = NULL;

void th_init(void)
{
  ptrdiff_t mutex_key_offset;

#ifdef UNIX_THREADS
  
  ADD_EFUN("thread_set_concurrency",f_thread_set_concurrency,tFunc(tInt,tVoid), OPT_SIDE_EFFECT);
#endif

#ifdef PIKE_DEBUG
  ADD_EFUN("_thread_swaps", f__thread_swaps,
	   tFunc(tVoid,tInt), OPT_SIDE_EFFECT);
  ADD_EFUN("_check_threads_calls", f__check_threads_calls,
	   tFunc(tVoid,tInt), OPT_SIDE_EFFECT);
  ADD_EFUN("_check_threads_yields", f__check_threads_yields,
	   tFunc(tVoid,tInt), OPT_SIDE_EFFECT);
  ADD_EFUN("_check_threads_swaps", f__check_threads_swaps,
	   tFunc(tVoid,tInt), OPT_SIDE_EFFECT);
#endif

  START_NEW_PROGRAM_ID(THREAD_MUTEX_KEY);
  mutex_key_offset = ADD_STORAGE(struct key_storage);
  /* This is needed to allow the gc to find the possible circular reference.
   * It also allows a thread to take over ownership of a key.
   */
  PIKE_MAP_VARIABLE("_owner",
		    mutex_key_offset + OFFSETOF(key_storage, owner_obj),
		    tObjIs_THREAD_ID, T_OBJECT, 0);
  PIKE_MAP_VARIABLE("_mutex", mutex_key_offset + OFFSETOF(key_storage, mutex_obj),
		    tObjIs_THREAD_MUTEX, T_OBJECT, ID_PROTECTED|ID_PRIVATE);
  ADD_FUNCTION("_sprintf",f_mutex_key__sprintf,tFunc(tInt,tStr),ID_PROTECTED);
  set_init_callback(init_mutex_key_obj);
  set_exit_callback(exit_mutex_key_obj);
  mutex_key=Pike_compiler->new_program;
  add_ref(mutex_key);
  end_class("mutex_key", 0);
  mutex_key->flags|=PROGRAM_DESTRUCT_IMMEDIATE;

  START_NEW_PROGRAM_ID(THREAD_MUTEX);
  ADD_STORAGE(struct mutex_storage);
  ADD_FUNCTION("lock",f_mutex_lock,
	       tFunc(tOr(tInt02,tVoid),tObjIs_THREAD_MUTEX_KEY),0);
  ADD_FUNCTION("trylock",f_mutex_trylock,
	       tFunc(tOr(tInt02,tVoid),tObjIs_THREAD_MUTEX_KEY),0);
  ADD_FUNCTION("current_locking_thread",f_mutex_locking_thread,
	   tFunc(tNone,tObjIs_THREAD_ID), 0);
  ADD_FUNCTION("current_locking_key",f_mutex_locking_key,
	   tFunc(tNone,tObjIs_THREAD_MUTEX_KEY), 0);
  ADD_FUNCTION("_sprintf",f_mutex__sprintf,tFunc(tNone,tStr),0);
  set_init_callback(init_mutex_obj);
  set_exit_callback(exit_mutex_obj);
  end_class("mutex", 0);

  START_NEW_PROGRAM_ID(THREAD_MUTEX_COMPAT_7_4);
  ADD_STORAGE(struct mutex_storage);
  ADD_FUNCTION("lock",f_mutex_lock,
	       tFunc(tOr(tInt02,tVoid),tObjIs_THREAD_MUTEX_KEY),0);
  ADD_FUNCTION("trylock",f_mutex_trylock,
	       tFunc(tOr(tInt02,tVoid),tObjIs_THREAD_MUTEX_KEY),0);
  ADD_FUNCTION("current_locking_thread",f_mutex_locking_thread,
	   tFunc(tNone,tObjIs_THREAD_ID), 0);
  ADD_FUNCTION("current_locking_key",f_mutex_locking_key,
	   tFunc(tNone,tObjIs_THREAD_MUTEX_KEY), 0);
  set_init_callback(init_mutex_obj);
  set_exit_callback(exit_mutex_obj_compat_7_4);
  end_class("mutex_compat_7_4", 0);

  START_NEW_PROGRAM_ID(THREAD_CONDITION);
  ADD_STORAGE(struct pike_cond);
  ADD_FUNCTION("wait",f_cond_wait,
	       tOr(tFunc(tObjIs_THREAD_MUTEX_KEY tOr3(tVoid, tIntPos, tFloat),
			 tVoid),
		   tFunc(tObjIs_THREAD_MUTEX_KEY tIntPos tIntPos, tVoid)),0);
  ADD_FUNCTION("signal",f_cond_signal,tFunc(tNone,tVoid),0);
  ADD_FUNCTION("broadcast",f_cond_broadcast,tFunc(tNone,tVoid),0);
  set_init_callback(init_cond_obj);
  set_exit_callback(exit_cond_obj);
  end_class("condition", 0);
  
  {
    struct program *tmp;
    START_NEW_PROGRAM_ID(THREAD_DISABLE_THREADS);
    set_init_callback(init_threads_disable);
    set_exit_callback(exit_threads_disable);
    tmp = Pike_compiler->new_program;
    add_ref(tmp);
    end_class("threads_disabled", 0);
    tmp->flags|=PROGRAM_DESTRUCT_IMMEDIATE;
    add_global_program("_disable_threads", tmp);
    free_program(tmp);
  }

  START_NEW_PROGRAM_ID(THREAD_LOCAL);
  ADD_STORAGE(struct thread_local_var);
  ADD_FUNCTION("get",f_thread_local_get,tFunc(tNone,tMix),0);
  ADD_FUNCTION("set",f_thread_local_set,tFunc(tSetvar(1,tMix),tVar(1)),0);
  ADD_FUNCTION("create", f_thread_local_create,
	       tFunc(tVoid,tVoid), ID_PROTECTED);
#ifdef PIKE_DEBUG
  set_gc_check_callback(gc_check_thread_local);
#endif
  thread_local_prog=Pike_compiler->new_program;
  add_ref(thread_local_prog);
  end_class("thread_local", 0);
  ADD_EFUN("thread_local", f_thread_local,
	   tFunc(tNone,tObjIs_THREAD_LOCAL),
	   OPT_EXTERNAL_DEPEND);

  START_NEW_PROGRAM_ID(THREAD_ID);
  thread_storage_offset=ADD_STORAGE(struct thread_state);
  PIKE_MAP_VARIABLE("result", OFFSETOF(thread_state, result),
		    tMix, T_MIXED, 0);
  ADD_FUNCTION("create",f_thread_create,
	       tFuncV(tMixed,tMixed,tVoid),
	       ID_PROTECTED);
  ADD_FUNCTION("backtrace",f_thread_backtrace,tFunc(tNone,tArray),0);
  ADD_FUNCTION("wait",f_thread_id_result,tFunc(tNone,tMix),0);
  ADD_FUNCTION("status",f_thread_id_status,tFunc(tNone,tInt),0);
  ADD_FUNCTION("_sprintf",f_thread_id__sprintf,tFunc(tNone,tStr),0);
  ADD_FUNCTION("id_number",f_thread_id_id_number,tFunc(tNone,tInt),0);
  ADD_FUNCTION("interrupt", f_thread_id_interrupt,
	       tFunc(tOr(tVoid,tStr), tVoid), 0);
  ADD_FUNCTION("kill", f_thread_id_kill, tFunc(tNone, tVoid), 0);
  set_gc_recurse_callback(thread_was_recursed);
  set_gc_check_callback(thread_was_checked);
  set_init_callback(init_thread_obj);
  set_exit_callback(exit_thread_obj);
  thread_id_prog=Pike_compiler->new_program;
  thread_id_prog->flags |= PROGRAM_NO_EXPLICIT_DESTRUCT;
  add_ref(thread_id_prog);
  end_class("thread_id", 0);

  /* Backward compat... */
  add_global_program("thread_create", thread_id_prog);

  ADD_EFUN("this_thread",f_this_thread,
	   tFunc(tNone,tObjIs_THREAD_ID),
	   OPT_EXTERNAL_DEPEND);

  ADD_EFUN("all_threads",f_all_threads,
	   tFunc(tNone,tArr(tObjIs_THREAD_ID)),
	   OPT_EXTERNAL_DEPEND);

  ADD_EFUN("get_thread_quanta", f_get_thread_quanta,
	   tFunc(tNone, tInt),
	   OPT_EXTERNAL_DEPEND);

  ADD_EFUN("set_thread_quanta", f_set_thread_quanta,
	   tFunc(tInt, tInt),
	   OPT_EXTERNAL_DEPEND);

  /* Some constants... */
  add_integer_constant("THREAD_NOT_STARTED", THREAD_NOT_STARTED, 0);
  add_integer_constant("THREAD_RUNNING", THREAD_RUNNING, 0);
  add_integer_constant("THREAD_EXITED", THREAD_EXITED, 0);
  add_integer_constant("THREAD_ABORTED", THREAD_ABORTED, 0);

  original_interpreter = Pike_interpreter_pointer;
  backend_thread_obj = fast_clone_object(thread_id_prog);
  INIT_THREAD_STATE((struct thread_state *)(backend_thread_obj->storage +
					    thread_storage_offset));
  thread_table_insert(Pike_interpreter.thread_state);
}

#ifdef DO_PIKE_CLEANUP
void cleanup_all_other_threads (void)
{
  int i, num_kills = num_pending_interrupts;
  time_t timeout = time (NULL) + 2;

  mt_lock (&thread_table_lock);
  for (i = 0; i < THREAD_TABLE_SIZE; i++) {
    struct thread_state *th;
    for (th = thread_table_chains[i]; th; th = th->hashlink)
      if (th != Pike_interpreter.thread_state) {
	low_thread_kill (th);
	num_kills++;
      }
  }
  mt_unlock (&thread_table_lock);

  while (num_pending_interrupts && time (NULL) < timeout) {
    THREADS_ALLOW();
    sysleep(1.0);
    THREADS_DISALLOW();
  }

#if 0
  if (num_kills) {
    fprintf (stderr, "Killed %d thread(s) in exit cleanup",
	     num_kills - num_pending_interrupts);
    if (num_pending_interrupts)
      fprintf (stderr, ", %d more haven't responded", num_pending_interrupts);
    fputs (".\n", stderr);
  }
#endif
}
#endif

void th_cleanup(void)
{
  th_running = 0;

  if (Pike_interpreter.thread_state) {
    thread_table_delete(Pike_interpreter.thread_state);
    Pike_interpreter.thread_state = NULL;
  }

  if(backend_thread_obj)
  {
    /* Switch back to the original interpreter struct. */
    *original_interpreter = Pike_interpreter;

    destruct(backend_thread_obj);
    free_object(backend_thread_obj);
    backend_thread_obj = NULL;

    Pike_interpreter_pointer = original_interpreter;

    destruct_objects_to_destruct_cb();
  }

  if(mutex_key)
  {
    free_program(mutex_key);
    mutex_key=0;
  }

  if(thread_local_prog)
  {
    free_program(thread_local_prog);
    thread_local_prog=0;
  }

  if(thread_id_prog)
  {
    free_program(thread_id_prog);
    thread_id_prog=0;
  }

#ifdef PIKE_USE_OWN_ATFORK
  free_callback_list(&atfork_prepare_callback);
  free_callback_list(&atfork_parent_callback);
  free_callback_list(&atfork_child_callback);
#endif
}

#endif	/* !CONFIGURE_TEST */

#endif
