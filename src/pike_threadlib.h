/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: pike_threadlib.h,v 1.55 2004/07/16 12:44:56 grubba Exp $
*/

#ifndef PIKE_THREADLIB_H
#define PIKE_THREADLIB_H

/*
 * This file is for the low-level thread interface functions
 * 'threads.h' is for anything that concerns the object interface
 * for pike threads.
 */


#ifndef CONFIGURE_TEST
#include "machine.h"
#include "main.h"
#include "pike_rusage.h"
#endif

/* Needed for the sigset_t typedef, which is needed for
 * the pthread_sigsetmask() prototype on Solaris 2.x.
 */
#include <signal.h>

#ifdef HAVE_SYS_TYPES_H
/* Needed for pthread_t on OSF/1 */
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */

extern int threads_disabled;
PMOD_EXPORT extern ptrdiff_t thread_storage_offset;
PMOD_EXPORT extern struct program *thread_id_prog;

#ifdef PIKE_THREADS

/* The fp macro conflicts with Solaris's <pthread.h>. */
#ifdef fp
#undef fp
#define FRAMEPOINTER_WAS_DEFINED
#endif /* fp */

/*
 * Decide which type of threads to use
 *
 * UNIX_THREADS      : Unix international threads
 * POSIX_THREADS     : POSIX standard threads
 * SGI_SPROC_THREADS : SGI sproc() based threads
 * NT_THREADS        : NT threads
 */

#ifdef _UNIX_THREADS
#ifdef HAVE_THREAD_H
#define UNIX_THREADS
#include <thread.h>
#undef HAVE_PTHREAD_H
#undef HAVE_THREAD_H
#endif
#endif /* _UNIX_THREADS */

#ifdef _MIT_POSIX_THREADS
#define POSIX_THREADS
#include <pthread.h>

/* AIX is *STUPID* - Hubbe */
#undef func_data

/* So is OSF/1 - Marcus */
#undef try
#undef except
#undef finally
#undef leave

#undef HAVE_PTHREAD_H
#endif /* _MIT_POSIX_THREADS */

#ifdef _SGI_SPROC_THREADS
/* Not supported yet */
#undef SGI_SPROC_THREADS
#undef HAVE_SPROC
#endif /* _SGI_SPROC_THREADS */

#ifdef HAVE_THREAD_H
#include <thread.h>
#endif

#ifdef HAVE_MACH_TASK_INFO_H
#include <mach/task_info.h>
#endif
#ifdef HAVE_MACH_TASK_H
#include <mach/task.h>
#endif
#ifdef HAVE_MACH_MACH_INIT_H
#include <mach/mach_init.h>
#endif


/* Restore the fp macro. */
#ifdef FRAMEPOINTER_WAS_DEFINED
#define fp Pike_fp
#undef FRAMEPOINTER_WAS_DEFINED
#endif /* FRAMEPOINTER_WAS_DEFINED */


extern int num_threads;
PMOD_EXPORT extern int live_threads, disallow_live_threads;
struct object;
PMOD_EXPORT extern size_t thread_stack_size;

PMOD_EXPORT void thread_low_error (int errcode, const char *cmd,
				   const char *fname, int lineno);

#define LOW_THREAD_CHECK_NONZERO_ERROR(CALL) do {			\
    int thread_errcode_ = (CALL);					\
    if (thread_errcode_)						\
      thread_low_error(thread_errcode_, TOSTR(CALL),			\
		       __FILE__, __LINE__);				\
  } while (0)

#ifdef CONFIGURE_TEST
#define USE_ERRORCHECK_MUTEX 1
#else
#define USE_ERRORCHECK_MUTEX (debug_options & ERRORCHECK_MUTEXES)
#endif

#define DEFINE_MUTEX(X) PIKE_MUTEX_T X


#ifdef POSIX_THREADS

#ifdef HAVE_PTHREAD_ATFORK
#define th_atfork(X,Y,Z) pthread_atfork((X),(Y),(Z))
#define th_atfork_prepare()
#define th_atfork_parent()
#define th_atfork_child()
#else
int th_atfork(void (*)(void),void (*)(void),void (*)(void));
void th_atfork_prepare(void);
void th_atfork_parent(void);
void th_atfork_child(void);
#endif

#define THREAD_T pthread_t
#define PIKE_MUTEX_T pthread_mutex_t

#ifdef PIKE_MUTEX_ERRORCHECK
#define mt_init(X) do {							\
    if (USE_ERRORCHECK_MUTEX) {						\
      pthread_mutexattr_t attr;						\
      pthread_mutexattr_init(&attr);					\
      pthread_mutexattr_settype(&attr, PIKE_MUTEX_ERRORCHECK);		\
      pthread_mutex_init((X), &attr);					\
    }									\
    else								\
      pthread_mutex_init((X),0);					\
  } while (0)
#define DO_IF_PIKE_MUTEX_ERRORCHECK(X) X
#else
#define mt_init(X) pthread_mutex_init((X),0)
#define DO_IF_PIKE_MUTEX_ERRORCHECK(X)
#endif

#ifdef PIKE_MUTEX_RECURSIVE
#define mt_init_recursive(X)						\
    do{ \
      pthread_mutexattr_t attr;					\
      pthread_mutexattr_init(&attr);					\
      pthread_mutexattr_settype(					\
	&attr,								\
	PIKE_MUTEX_RECURSIVE						\
	DO_IF_PIKE_MUTEX_ERRORCHECK (| PIKE_MUTEX_ERRORCHECK));		\
      pthread_mutex_init((X), &attr);					\
    }while(0)
#endif

#define mt_lock(X) LOW_THREAD_CHECK_NONZERO_ERROR (pthread_mutex_lock(X))
#define mt_trylock(X) pthread_mutex_trylock(X)
#define mt_unlock(X) LOW_THREAD_CHECK_NONZERO_ERROR (pthread_mutex_unlock(X))
#define mt_destroy(X) LOW_THREAD_CHECK_NONZERO_ERROR (pthread_mutex_destroy(X))

/* SIGH! No setconcurrency in posix threads. This is more or less
 * needed to make usable multi-threaded programs on solaris machines
 * with only one CPU. Otherwise, only systemcalls are actually
 * threaded.
 */
#define th_setconcurrency(X) 
#ifdef HAVE_PTHREAD_YIELD
#define low_th_yield()	pthread_yield()
#else
#ifdef HAVE_PTHREAD_YIELD_NP
/* Some pthread libs define yield as non-portable function. */
#define low_th_yield()	pthread_yield_np()
#endif /* HAVE_PTHREAD_YIELD_NP */
#endif /* HAVE_PTHREAD_YIELD */
extern pthread_attr_t pattr;
extern pthread_attr_t small_pattr;

#define th_create(ID,fun,arg) pthread_create(ID,&pattr,fun,arg)
#define th_create_small(ID,fun,arg) pthread_create(ID,&small_pattr,fun,arg)
#define th_exit(foo) pthread_exit(foo)
#define th_self() pthread_self()

#define TH_KEY_T pthread_key_t
#define th_key_create pthread_key_create
#define th_setspecific pthread_setspecific
#define th_getspecific pthread_getspecific


#ifdef HAVE_PTHREAD_KILL
#define th_kill(ID,sig) LOW_THREAD_CHECK_NONZERO_ERROR (pthread_kill((ID),(sig)))
#else /* !HAVE_PTHREAD_KILL */
/* MacOS X (aka Darwin) prior to 10.2 doesn't have pthread_kill. */
#define th_kill(ID,sig)
#endif /* HAVE_PTHREAD_KILL */
#ifdef HAVE_PTHREAD_COND_INIT
#define COND_T pthread_cond_t

#ifdef HAVE_PTHREAD_CONDATTR_DEFAULT_AIX
/* AIX wants the & ... */
#define co_init(X) pthread_cond_init((X), &pthread_condattr_default)
#else /* !HAVE_PTHREAD_CONDATTR_DEFAULT_AIX */
#ifdef HAVE_PTHREAD_CONDATTR_DEFAULT
/* ... while FreeBSD doesn't. */
#define co_init(X) pthread_cond_init((X), pthread_condattr_default)
#else /* !HAVE_PTHREAD_CONDATTR_DEFAULT */
#define co_init(X) pthread_cond_init((X), 0)
#endif /* HAVE_PTHREAD_CONDATTR_DEFAULT */
#endif /* HAVE_PTHREAD_CONDATTR_DEFAULT_AIX */

#define co_wait(COND, MUTEX) pthread_cond_wait((COND), (MUTEX))
#define co_signal(X) pthread_cond_signal(X)
#define co_broadcast(X) pthread_cond_broadcast(X)
#define co_destroy(X) LOW_THREAD_CHECK_NONZERO_ERROR (pthread_cond_destroy(X))
#else
#error No way to make cond-vars
#endif /* HAVE_PTHREAD_COND_INIT */

#endif /* POSIX_THREADS */




#ifdef UNIX_THREADS
#define THREAD_T thread_t
#define PTHREAD_MUTEX_INITIALIZER DEFAULTMUTEX
#define PIKE_MUTEX_T mutex_t
#define mt_init(X) LOW_THREAD_CHECK_NONZERO_ERROR (mutex_init((X),USYNC_THREAD,0))
#define mt_lock(X) LOW_THREAD_CHECK_NONZERO_ERROR (mutex_lock(X))
#define mt_trylock(X) mutex_trylock(X)
#define mt_unlock(X) LOW_THREAD_CHECK_NONZERO_ERROR (mutex_unlock(X))
#define mt_destroy(X) LOW_THREAD_CHECK_NONZERO_ERROR (mutex_destroy(X))

#define th_setconcurrency(X) thr_setconcurrency(X)

#define th_create(ID,fun,arg) thr_create(NULL,thread_stack_size,fun,arg,THR_DAEMON|THR_DETACHED,ID)
#define th_create_small(ID,fun,arg) thr_create(NULL,8192*sizeof(char *),fun,arg,THR_DAEMON|THR_DETACHED,ID)
#define th_exit(foo) thr_exit(foo)
#define th_self() thr_self()
#define th_kill(ID,sig) thr_kill((ID),(sig))
#define low_th_yield() thr_yield()

#define COND_T cond_t
#define co_init(X) cond_init((X),USYNC_THREAD,0)
#define co_wait(COND, MUTEX) cond_wait((COND), (MUTEX))
#define co_signal(X) cond_signal(X)
#define co_broadcast(X) cond_broadcast(X)
#define co_destroy(X) cond_destroy(X)


#endif /* UNIX_THREADS */

#ifdef SGI_SPROC_THREADS

/*
 * Not fully supported yet
 */
#define THREAD_T	int

#define PIKE_MUTEX_T		ulock_t
#define mt_init(X)	(usinitlock(((*X) = usnewlock(/*********/))))
#define mt_lock(X)	ussetlock(*X)
#define mt_unlock(X)	usunsetlock(*X)
#define mt_destroy(X)	usfreelock((*X), /*******/)

#define th_setconcurrency(X)	/*******/

#define PIKE_SPROC_FLAGS	(PR_SADDR|PR_SFDS|PR_SDIR|PS_SETEXITSIG)
#define th_create(ID, fun, arg)	(((*(ID)) = sproc(fun, PIKE_SPROC_FLAGS, arg)) == -1)
#define th_create_small(ID, fun, arg)	(((*(ID)) = sproc(fun, PIKE_SPROC_FLAGS, arg)) == -1)
#define th_exit(X)	exit(X)
#define th_self()	getpid()
#define low_th_yield()	sginap(0)
#define th_equal(X,Y) ((X)==(Y))
#define th_hash(X) ((unsigned INT32)(X))

/*
 * No cond_vars yet
 */

#endif /* SGI_SPROC_THREADS */


#ifdef NT_THREADS
#include <process.h>
#include <windows.h>

#define LOW_THREAD_CHECK_ZERO_ERROR(CALL) do {			\
    if (!(CALL))						\
      thread_low_error(GetLastError(), TOSTR(CALL),		\
		       __FILE__, __LINE__);			\
  } while (0)

#define THREAD_T unsigned
#define th_setconcurrency(X)
#define th_create(ID,fun,arg) low_nt_create_thread(thread_stack_size,fun, arg,ID)
#define th_create_small(ID,fun,arg) low_nt_create_thread(8192*sizeof(char *), fun,arg,ID)
#define TH_RETURN_TYPE unsigned __stdcall
#define TH_STDCALL __stdcall
#define th_exit(foo) _endthreadex(foo)
#define th_self() GetCurrentThreadId()
#define th_destroy(X)
#define low_th_yield() Sleep(0)
#define th_equal(X,Y) ((X)==(Y))
#define th_hash(X) (X)

#define PIKE_MUTEX_T HANDLE
#define mt_init(X) LOW_THREAD_CHECK_ZERO_ERROR ((*(X)=CreateMutex(NULL, 0, NULL)))
#define mt_lock(X)							\
  LOW_THREAD_CHECK_ZERO_ERROR (						\
    WaitForSingleObject(CheckValidHandle(*(X)), INFINITE) == WAIT_OBJECT_0)
#define mt_trylock(X)							\
  LOW_THREAD_CHECK_ZERO_ERROR (						\
    WaitForSingleObject(CheckValidHandle(*(X)), 0) != WAIT_FAILED)
#define mt_unlock(X) LOW_THREAD_CHECK_ZERO_ERROR (ReleaseMutex(CheckValidHandle(*(X))))
#define mt_destroy(X) LOW_THREAD_CHECK_ZERO_ERROR (CloseHandle(CheckValidHandle(*(X))))

#define EVENT_T HANDLE
#define event_init(X) LOW_THREAD_CHECK_ZERO_ERROR (*(X)=CreateEvent(NULL, 1, 0, NULL))
#define event_signal(X) LOW_THREAD_CHECK_ZERO_ERROR (SetEvent(CheckValidHandle(*(X))))
#define event_destroy(X) LOW_THREAD_CHECK_ZERO_ERROR (CloseHandle(CheckValidHandle(*(X))))
#define event_wait(X)							\
  LOW_THREAD_CHECK_ZERO_ERROR (						\
    WaitForSingleObject(CheckValidHandle(*(X)), INFINITE) == WAIT_OBJECT_0)

/* No fork -- no atfork */
#define th_atfork(X,Y,Z)
#define th_atfork_prepare()
#define th_atfork_parent()
#define th_atfork_child()

#endif


#if !defined(COND_T) && defined(EVENT_T) && defined(PIKE_MUTEX_T)

#define SIMULATE_COND_WITH_EVENT

struct cond_t_queue
{
  struct cond_t_queue *next;
  EVENT_T event;
};

typedef struct cond_t_s
{
  PIKE_MUTEX_T lock;
  struct cond_t_queue *head, *tail;
} COND_T;

#define COND_T struct cond_t_s

#define co_init(X) do { mt_init(& (X)->lock); (X)->head=(X)->tail=0; }while(0)

PMOD_EXPORT int co_wait(COND_T *c, PIKE_MUTEX_T *m);
PMOD_EXPORT int co_signal(COND_T *c);
PMOD_EXPORT int co_broadcast(COND_T *c);
PMOD_EXPORT int co_destroy(COND_T *c);

#endif

#ifndef TH_RETURN_TYPE
#define TH_RETURN_TYPE void *
#endif

#ifndef TH_STDCALL
#define TH_STDCALL
#endif

#ifndef th_destroy
#define th_destroy(X)
#endif

#ifndef low_th_yield
#ifdef HAVE_THR_YIELD
#define low_th_yield() thr_yield()
#else
#define low_th_yield() 0
#define HAVE_NO_YIELD
#endif
#endif

#ifndef th_equal
#define th_equal(X,Y) (!MEMCMP(&(X),&(Y),sizeof(THREAD_T)))
#endif

#ifndef th_hash
#define th_hash(X) hashmem((unsigned char *)&(X),sizeof(THREAD_T), 16)
#endif

#ifndef CONFIGURE_TEST

struct interleave_mutex
{
  struct interleave_mutex *next;
  struct interleave_mutex *prev;
  PIKE_MUTEX_T lock;
};

#define IMUTEX_T struct interleave_mutex

#define DEFINE_IMUTEX(name) IMUTEX_T name

/* If threads are disabled, we already hold the lock. */
#define LOCK_IMUTEX(im) do { \
    if (!threads_disabled) { \
      THREADS_FPRINTF(0, (stderr, "Locking IMutex 0x%p...\n", (im))); \
      THREADS_ALLOW(); \
      mt_lock(&((im)->lock)); \
      THREADS_DISALLOW(); \
    } \
  } while(0)

/* If threads are disabled, the lock will be released later. */
#define UNLOCK_IMUTEX(im) do { \
    if (!threads_disabled) { \
      THREADS_FPRINTF(0, (stderr, "Unlocking IMutex 0x%p...\n", (im))); \
      mt_unlock(&((im)->lock)); \
    } \
  } while(0)

extern int th_running;

PMOD_EXPORT extern PIKE_MUTEX_T interpreter_lock;

PMOD_EXPORT extern COND_T live_threads_change;		/* Used by _disable_threads */
PMOD_EXPORT extern COND_T threads_disabled_change;		/* Used by _disable_threads */

#define THREAD_TABLE_SIZE 127  /* Totally arbitrary prime */

extern PIKE_MUTEX_T thread_table_lock;
extern struct thread_state *thread_table_chains[THREAD_TABLE_SIZE];
extern int num_pike_threads;

/* Note: thread_table_lock is held while the code is run. */
#define FOR_EACH_THREAD(TSTATE, CODE)					\
  do {									\
    INT32 __tt_idx__;							\
    mt_lock( & thread_table_lock );					\
    for(__tt_idx__=0; __tt_idx__<THREAD_TABLE_SIZE; __tt_idx__++)	\
      for(TSTATE=thread_table_chains[__tt_idx__];			\
	  TSTATE;							\
	  TSTATE=TSTATE->hashlink) {					\
	CODE;								\
      }									\
    mt_unlock( & thread_table_lock );					\
  } while (0)

#if !defined(HAVE_GETHRTIME) && \
    !(defined(HAVE_MACH_TASK_INFO_H) && defined(TASK_THREAD_TIMES_INFO)) && \
    defined(HAVE_CLOCK) && \
    !defined(HAVE_NO_YIELD)
#ifdef HAVE_TIME_H
#include <time.h>
#endif
PMOD_EXPORT extern clock_t thread_start_clock;
#define USE_CLOCK_FOR_SLICES
#define DO_IF_USE_CLOCK_FOR_SLICES(X) X
#else
#define DO_IF_USE_CLOCK_FOR_SLICES(X)
#endif

/* Define to get a debug-trace of some of the threads operations. */
/* #define VERBOSE_THREADS_DEBUG	0 */ /* Some debug */
/* #define VERBOSE_THREADS_DEBUG	1 */ /* Lots of debug */

#ifndef VERBOSE_THREADS_DEBUG
#define THREADS_FPRINTF(L,X)
#else
#include <errno.h>
#define THREADS_FPRINTF(L,X)	do { \
    if ((VERBOSE_THREADS_DEBUG + 0) >= (L)) {				\
      /* E.g. THREADS_DISALLOW is used in numerous places where the */	\
      /* value in errno must not be clobbered. */			\
      int saved_errno__ = errno;					\
      fprintf X;							\
      errno = saved_errno__;						\
    }									\
  } while(0)
#endif /* VERBOSE_THREADS_DEBUG */

#if defined(PIKE_DEBUG) && !defined(__NT__)

/* This is a debug wrapper to enable checks that the interpreter lock
 * is hold by the current thread. */

extern THREAD_T debug_locking_thread;
#define SET_LOCKING_THREAD (debug_locking_thread = th_self(), 0)

#define low_mt_lock_interpreter()					\
  do {mt_lock(&interpreter_lock); SET_LOCKING_THREAD;} while (0)
#define low_mt_trylock_interpreter()					\
  (mt_trylock(&interpreter_lock) || SET_LOCKING_THREAD)
#define low_co_wait_interpreter(COND) \
  do {co_wait((COND), &interpreter_lock); SET_LOCKING_THREAD;} while (0)

PMOD_EXPORT extern const char msg_ip_not_locked[];
PMOD_EXPORT extern const char msg_ip_not_locked_this_thr[];

#define CHECK_INTERPRETER_LOCK() do {					\
  if (th_running) {							\
    THREAD_T self;							\
    if (!mt_trylock(&interpreter_lock))					\
      Pike_fatal(msg_ip_not_locked);					\
    self = th_self();							\
    if (!th_equal(debug_locking_thread, self))				\
      Pike_fatal(msg_ip_not_locked_this_thr);				\
  }									\
} while (0)

#else

#define low_mt_lock_interpreter() do {mt_lock(&interpreter_lock);} while (0)
#define low_mt_trylock_interpreter() (mt_trylock(&interpreter_lock))
#define low_co_wait_interpreter(COND) do {co_wait((COND), &interpreter_lock);} while (0)

#endif

static INLINE int threads_disabled_wait(void)
{
  do {
    THREADS_FPRINTF(1, (stderr, "Thread %d: Wait on threads_disabled\n",
			(int) th_self()));
    low_co_wait_interpreter(&threads_disabled_change);
  } while (threads_disabled);
  THREADS_FPRINTF(1, (stderr, "Thread %d: Continue after threads_disabled\n",
		      (int) th_self()));
  return 0;
}

#define mt_lock_interpreter() do {					\
    low_mt_lock_interpreter();						\
    (threads_disabled && threads_disabled_wait());			\
  } while (0)
#define mt_trylock_interpreter() \
  (low_mt_trylock_interpreter() || (threads_disabled && threads_disabled_wait()))
#define mt_unlock_interpreter() do {					\
    mt_unlock(&interpreter_lock);					\
  } while (0)
#define co_wait_interpreter(COND) do {					\
    low_co_wait_interpreter(COND);					\
    if (threads_disabled) threads_disabled_wait();			\
  } while (0)

#ifdef INTERNAL_PROFILING
PMOD_EXPORT extern unsigned long thread_yields;
#define th_yield() (thread_yields++, low_th_yield())
#else
#define th_yield() low_th_yield()
#endif

#ifdef PIKE_DEBUG
PMOD_EXPORT extern THREAD_T threads_disabled_thread;
#endif

#define INIT_THREAD_STATE(_tmp) do {					\
    struct thread_state *_th_state = (_tmp);				\
    Pike_interpreter.thread_state = _th_state;				\
    _th_state->state = Pike_interpreter;				\
    _th_state->id = th_self();						\
    _th_state->status = THREAD_RUNNING;					\
    _th_state->swapped = 0;						\
    DO_IF_DEBUG(_th_state->debug_flags = 0;)				\
    DO_IF_USE_CLOCK_FOR_SLICES (thread_start_clock = 0);		\
  } while (0)

#define EXIT_THREAD_STATE(_tmp) do {					\
    DO_IF_DEBUG (Pike_sp = (struct svalue *) (ptrdiff_t) -1);		\
  } while (0)

#define SWAP_OUT_THREAD(_tmp) do {					\
    struct thread_state *_th_state = (_tmp);				\
    _th_state->state=Pike_interpreter;					\
    DO_IF_PROFILING({							\
	if (!_th_state->swapped) {					\
	  cpu_time_t now = get_cpu_time();				\
	  DO_IF_PROFILING_DEBUG({					\
	      fprintf(stderr, "%p: Swap out at: %" PRINT_CPU_TIME	\
		      " unlocked: %" PRINT_CPU_TIME "\n",		\
		      _th_state, now, _th_state->state.unlocked_time);	\
	    });								\
	  _th_state->state.unlocked_time -= now;			\
	}								\
      });								\
    _th_state->swapped=1;						\
    DO_IF_DEBUG (							\
      /* Yo! Yo run now, yo DIE! Hear! */				\
      Pike_sp = (struct svalue *) (ptrdiff_t) -1;			\
      Pike_fp = (struct pike_frame *) (ptrdiff_t) -1;			\
    );									\
    /* Do this one always to catch nested THREADS_ALLOW(), etc. */	\
    Pike_interpreter.thread_state = 					\
      (struct thread_state *) (ptrdiff_t) -1;				\
  } while(0)

PMOD_EXPORT extern const char msg_thr_swapped_over[];

#define SWAP_IN_THREAD(_tmp) do {					\
    struct thread_state *_th_state = (_tmp);				\
    DO_IF_DEBUG (							\
      if (Pike_sp != (struct svalue *) (ptrdiff_t) -1)			\
	Pike_fatal (msg_thr_swapped_over,				\
		    (unsigned int) _th_state->id,			\
		    Pike_interpreter.thread_state ?			\
		    (unsigned int) Pike_interpreter.thread_state->id : 0); \
    );									\
    DO_IF_PROFILING({							\
	if (_th_state->swapped) {					\
	  cpu_time_t now = get_cpu_time();				\
	  DO_IF_DEBUG({							\
	      DO_IF_PROFILING_DEBUG({					\
		  fprintf(stderr, "%p: Swap in at: %" PRINT_CPU_TIME	\
			  " unlocked: %" PRINT_CPU_TIME "\n",		\
			  _th_state, now, _th_state->state.unlocked_time); \
		});							\
	      if (now < -Pike_interpreter.unlocked_time) {		\
		Pike_fatal("Time at swap in is before time at swap out."\
			   " %" PRINT_CPU_TIME " < %" PRINT_CPU_TIME	\
			   "\n", now, -Pike_interpreter.unlocked_time);	\
	      }								\
	    });								\
	  _th_state->state.unlocked_time += now;			\
	}								\
      });								\
    _th_state->swapped=0;						\
    Pike_interpreter=_th_state->state;					\
    DO_IF_USE_CLOCK_FOR_SLICES (thread_start_clock = 0);		\
  } while(0)

#define SWAP_OUT_CURRENT_THREAD() \
  do {\
     struct thread_state *_tmp = Pike_interpreter.thread_state; \
     SWAP_OUT_THREAD(_tmp); \
     THREADS_FPRINTF(1, (stderr, "SWAP_OUT_CURRENT_THREAD() %s:%d t:%08x\n", \
			 __FILE__, __LINE__, (unsigned int)_tmp->id))

extern void debug_list_all_threads(void);
extern void dumpmem(const char *desc, void *x, int size);

PMOD_EXPORT extern const char msg_saved_thread_id[];
PMOD_EXPORT extern const char msg_swap_in_cur_thr_failed[];

#define SWAP_IN_CURRENT_THREAD()					      \
   THREADS_FPRINTF(1, (stderr, "SWAP_IN_CURRENT_THREAD() %s:%d ... t:%08x\n", \
		       __FILE__, __LINE__, (unsigned int)_tmp->id));	      \
   SWAP_IN_THREAD(_tmp);						      \
   DO_IF_DEBUG(								      \
   {									      \
     THREAD_T self=th_self();						      \
     if(MEMCMP( & _tmp->id, &self, sizeof(self)))		    	      \
     {									      \
       dumpmem(msg_saved_thread_id,&self,sizeof(self));			      \
       debug_list_all_threads();					      \
       Pike_fatal(msg_swap_in_cur_thr_failed);				      \
     }									      \
   })									      \
 } while(0)

#if defined(PIKE_DEBUG) && ! defined(DONT_HIDE_GLOBALS)
/* Note that scalar types are used in place of pointers and vice versa
 * below. This is intended to cause compiler warnings/errors if
 * there is an attempt to use the global variables in an unsafe
 * environment.
 */


#ifdef __GCC__
#ifdef __i386__

/* This is a rather drastic measure since it
 * obliterates backtraces, oh well, gcc doesn't work
 * very well sometimes anyways... -Hubbe
 */
#define HIDE_PC								\
  ;void *pc_=(((unsigned char **)__builtin_frame_address(0))[1]);	\
  (((unsigned char **)__builtin_frame_address(0))[1])=0
#define REVEAL_PC \
  (((unsigned char **)__builtin_frame_address(0))[1])=pc_;
#endif
#endif

#ifndef HIDE_PC
#define HIDE_PC
#define REVEAL_PC
#endif

#define HIDE_GLOBAL_VARIABLES() do { \
   int Pike_interpreter =0; \
   int pop_n_elems = 0; \
   int push_sp_mark = 0, pop_sp_mark = 0, threads_disabled = 1 \
   HIDE_PC

/* Note that the semi-colon below is needed to add an empty statement
 * in case there is a label before the macro.
 */
#define REVEAL_GLOBAL_VARIABLES() ; REVEAL_PC } while(0)
#else /* PIKE_DEBUG */
#define HIDE_GLOBAL_VARIABLES()
#define REVEAL_GLOBAL_VARIABLES()
#endif /* PIKE_DEBUG */

#ifdef PIKE_DEBUG

PMOD_EXPORT extern const char msg_thr_not_swapped_in[];
PMOD_EXPORT extern const char msg_cur_thr_not_bound[];
PMOD_EXPORT extern const char msg_thr_states_mixed[];

#define ASSERT_THREAD_SWAPPED_IN() do {					\
    struct thread_state *_tmp=thread_state_for_id(th_self());		\
    if(_tmp->swapped) Pike_fatal(msg_thr_not_swapped_in);		\
    if (_tmp->debug_flags & THREAD_DEBUG_LOOSE) {			\
      Pike_fatal(msg_cur_thr_not_bound);				\
    }									\
  }while(0)
#define DEBUG_CHECK_THREAD() do {					\
    struct thread_state *_tmp=thread_state_for_id(th_self());		\
    if (_tmp->debug_flags & THREAD_DEBUG_LOOSE) {			\
      Pike_fatal(msg_cur_thr_not_bound);				\
    }									\
    if(_tmp != Pike_interpreter.thread_state) {				\
      debug_list_all_threads();						\
      Pike_fatal(msg_thr_states_mixed);					\
    }									\
  } while (0)
#else
#define ASSERT_THREAD_SWAPPED_IN() do { } while (0)
#define DEBUG_CHECK_THREAD() do { } while (0)
#endif

#define THREADSTATE2OBJ(X) ((X)->thread_obj)

#ifdef PIKE_DEBUG
PMOD_EXPORT extern const char msg_thr_allow_in_gc[];
PMOD_EXPORT extern const char msg_thr_allow_in_disabled[];
PMOD_EXPORT extern const char msg_global_dynbuf_in_use[];
extern dynamic_buffer pike_global_buffer;
#if defined(__ia64) && defined(__xlc__)
/* Workaround for a code generation bug in xlc 5.5.0.0/ia64 . */
#define DO_IF_NOT_XLC_IA64(X)
#else /* !ia64 || !xlc */
#define DO_IF_NOT_XLC_IA64(X)	X
#endif
#endif

PMOD_EXPORT extern int Pike_in_gc;
#define THREADS_ALLOW() do { \
     struct thread_state *_tmp = Pike_interpreter.thread_state; \
     DO_IF_PIKE_CLEANUP (					\
       /* Might get here after th_cleanup() when reporting leaks. */	\
       if (_tmp) {)						\
     DEBUG_CHECK_THREAD();					\
     DO_IF_DEBUG({ \
       if (Pike_in_gc > 50 && Pike_in_gc < 300) \
	 Pike_fatal(msg_thr_allow_in_gc, Pike_in_gc);			\
       if (pike_global_buffer.s.str)					\
	 Pike_fatal(msg_global_dynbuf_in_use);				\
     }) \
     if(num_threads > 1 && !threads_disabled) { \
       SWAP_OUT_THREAD(_tmp); \
       THREADS_FPRINTF(1, (stderr, "THREADS_ALLOW() %s:%d t:%08x(#%d)\n", \
			   __FILE__, __LINE__, \
			   (unsigned int)_tmp->id, live_threads)); \
       mt_unlock_interpreter(); \
     } else {								\
       DO_IF_DEBUG(							\
	 THREAD_T self = th_self();					\
	 if (threads_disabled &&					\
	     !th_equal(threads_disabled_thread, self))			\
	   DO_IF_NOT_XLC_IA64(Pike_fatal(msg_thr_allow_in_disabled,	\
					 self,				\
					 threads_disabled_thread));	\
       );								\
     }									\
     DO_IF_DEBUG(_tmp->debug_flags |= THREAD_DEBUG_LOOSE;)		\
     DO_IF_PIKE_CLEANUP (})						\
     HIDE_GLOBAL_VARIABLES()

#define THREADS_DISALLOW() \
     REVEAL_GLOBAL_VARIABLES(); \
     DO_IF_PIKE_CLEANUP (if (_tmp) {) \
     if(_tmp->swapped) { \
       low_mt_lock_interpreter(); \
       THREADS_FPRINTF(1, (stderr, "THREADS_DISALLOW() %s:%d t:%08x(#%d)\n", \
			   __FILE__, __LINE__, \
			   (unsigned int)_tmp->id, live_threads)); \
       if (threads_disabled) threads_disabled_wait(); \
       SWAP_IN_THREAD(_tmp);\
     } \
     DO_IF_DEBUG(_tmp->debug_flags &= ~THREAD_DEBUG_LOOSE;) \
     DEBUG_CHECK_THREAD(); \
     DO_IF_PIKE_CLEANUP (}) \
   } while(0)

#define THREADS_ALLOW_UID() do { \
     struct thread_state *_tmp_uid = Pike_interpreter.thread_state; \
     DO_IF_PIKE_CLEANUP (					\
       /* Might get here after th_cleanup() when reporting leaks. */	\
       if (_tmp_uid) {)						\
     DEBUG_CHECK_THREAD();					    \
     DO_IF_DEBUG({ \
       if ((Pike_in_gc > 50) && (Pike_in_gc < 300)) { \
	 debug_fatal(msg_thr_allow_in_gc, Pike_in_gc); \
       if (pike_global_buffer.s.str)					\
	 Pike_fatal(msg_global_dynbuf_in_use);				\
       } \
     }) \
     if(num_threads > 1 && !threads_disabled) { \
       SWAP_OUT_THREAD(_tmp_uid); \
       while (disallow_live_threads) {					\
	 THREADS_FPRINTF(1, (stderr, "THREADS_ALLOW_UID() %s:%d t:%08x(#%d) " \
			     "live threads disallowed\n",		\
			     __FILE__, __LINE__,			\
			     (unsigned int)_tmp_uid->id, live_threads)); \
	 co_wait_interpreter(&threads_disabled_change);			\
       }								\
       live_threads++; \
       THREADS_FPRINTF(1, (stderr, "THREADS_ALLOW_UID() %s:%d t:%08x(#%d)\n", \
			   __FILE__, __LINE__, \
			   (unsigned int)_tmp_uid->id, live_threads)); \
       mt_unlock_interpreter(); \
     } else {								\
       DO_IF_DEBUG(							\
	 THREAD_T self = th_self();					\
	 if (threads_disabled &&					\
	     !th_equal(threads_disabled_thread, self))			\
	   DO_IF_NOT_XLC_IA64(Pike_fatal(msg_thr_allow_in_disabled,	\
					 self,				\
					 threads_disabled_thread));	\
       );								\
     }									\
     DO_IF_DEBUG(_tmp_uid->debug_flags |= THREAD_DEBUG_LOOSE;)		\
     DO_IF_PIKE_CLEANUP (})						\
     HIDE_GLOBAL_VARIABLES()

#define THREADS_DISALLOW_UID() \
     REVEAL_GLOBAL_VARIABLES(); \
     DO_IF_PIKE_CLEANUP (if (_tmp_uid) {) \
     if(_tmp_uid->swapped) { \
       low_mt_lock_interpreter(); \
       live_threads--; \
       THREADS_FPRINTF(1, (stderr, \
                           "THREADS_DISALLOW_UID() %s:%d t:%08x(#%d)\n", \
			   __FILE__, __LINE__, \
			   (unsigned int)_tmp_uid->id, live_threads)); \
       co_broadcast(&live_threads_change); \
       if (threads_disabled) threads_disabled_wait(); \
       SWAP_IN_THREAD(_tmp_uid);\
     } \
     DO_IF_DEBUG(_tmp_uid->debug_flags &= ~THREAD_DEBUG_LOOSE;) \
     DEBUG_CHECK_THREAD(); \
     DO_IF_PIKE_CLEANUP (}) \
   } while(0)

#define SWAP_IN_THREAD_IF_REQUIRED() do { 			\
  struct thread_state *_tmp=thread_state_for_id(th_self());	\
  HIDE_GLOBAL_VARIABLES();					\
  THREADS_DISALLOW()

#endif	/* !CONFIGURE_TEST */

#endif /* PIKE_THREADS */

#ifndef PIKE_THREADS

#define th_atfork(X,Y,Z)
#define th_atfork_prepare()
#define th_atfork_parent()
#define th_atfork_child()

#define th_setconcurrency(X)
#define DEFINE_MUTEX(X)
#define DEFINE_IMUTEX(X)
#define init_interleave_mutex(X)
#define LOCK_IMUTEX(X)
#define UNLOCK_IMUTEX(X)
#define mt_init(X)
#define mt_lock(X)
#define mt_unlock(X)
#define mt_destroy(X)
#define THREADS_ALLOW()
#define THREADS_DISALLOW()
#define THREADS_ALLOW_UID()
#define THREADS_DISALLOW_UID()
#define HIDE_GLOBAL_VARIABLES()
#define REVEAL_GLOBAL_VARIABLES()
#define ASSERT_THREAD_SWAPPED_IN()
#define SWAP_IN_THREAD_IF_REQUIRED()
#define th_init()
#define low_th_init()
#define th_cleanup()
#define th_init_programs()
#define th_self() NULL
#define co_wait(X,Y)
#define co_signal(X)
#define co_broadcast(X)
#define co_destroy(X)

#define low_init_threads_disable()
#define init_threads_disable(X)
#define exit_threads_disable(X)


#endif /* PIKE_THREADS */

#ifndef CHECK_INTERPRETER_LOCK
#define CHECK_INTERPRETER_LOCK() do {} while (0)
#endif

#ifdef __NT__
#if !defined (PIKE_DEBUG) || defined (CONFIGURE_TEST)
#define CheckValidHandle(X) (X)
#else
PMOD_EXPORT HANDLE CheckValidHandle(HANDLE h);
#endif
#endif

#ifndef NO_PIKE_SHORTHAND
#define MUTEX_T PIKE_MUTEX_T
#endif


/* Initializer macros for static mutex and condition variables */
#ifdef PTHREAD_MUTEX_INITIALIZER
#define STATIC_MUTEX_INIT  = PTHREAD_MUTEX_INITIALIZER
#else
#define STATIC_MUTEX_INIT
#endif
#ifdef PTHREAD_COND_INITIALIZER
#define STATIC_COND_INIT   = PTHREAD_COND_INITIALIZER
#else
#define STATIC_COND_INIT
#endif

#ifndef THREAD_T_TO_PTR
#ifdef PIKE_THREAD_T_IS_POINTER
#define THREAD_T_TO_PTR(X)	((void *)(X))
#else /* !PIKE_THREAD_T_IS_POINTER */
#define THREAD_T_TO_PTR(X)	((void *)(ptrdiff_t)(X))
#endif /* PIKE_THREAD_T_IS_POINTER */
#endif /* !THREAD_T_TO_PTR */

#endif /* PIKE_THREADLIB_H */
