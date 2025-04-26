/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef PIKE_THREADLIB_H
#define PIKE_THREADLIB_H

/*
 * This file is for the low-level thread interface functions
 * 'threads.h' is for anything that concerns the object interface
 * for pike threads.
 */

#include "global.h"
#include "pike_embed.h"
#include "pike_rusage.h"

/* Needed for the sigset_t typedef, which is needed for
 * the pthread_sigsetmask() prototype on Solaris 2.x.
 */
#include <signal.h>

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
#ifdef HAVE_SCHED_H
#include <sched.h>
#endif

#ifdef USE_DARWIN_THREADS_WITHOUT_MACH
/* OSX Threads don't get along with mach headers! */
#else
#ifdef HAVE_MACH_PORT_H
#include <mach/port.h>
#endif
#ifdef HAVE_MACH_MESSAGE_H
#include <mach/message.h>
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
#endif /* USE_DARWIN_THREADS_WITHOUT_MACH */

/* Restore the fp macro. */
#ifdef FRAMEPOINTER_WAS_DEFINED
#define fp Pike_fp
#undef FRAMEPOINTER_WAS_DEFINED
#endif /* FRAMEPOINTER_WAS_DEFINED */

PMOD_EXPORT extern int threads_disabled;
PMOD_EXPORT extern cpu_time_t threads_disabled_acc_time;
PMOD_EXPORT extern cpu_time_t threads_disabled_start;
PMOD_EXPORT extern ptrdiff_t thread_storage_offset;
PMOD_EXPORT extern struct program *thread_id_prog;
PMOD_EXPORT extern int num_threads;
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
#ifdef HAVE_SCHED_YIELD
#define low_th_yield() sched_yield()
#elif defined (HAVE_PTHREAD_YIELD)
#define low_th_yield()	pthread_yield()
#elif defined (HAVE_PTHREAD_YIELD_NP)
/* Some pthread libs define yield as non-portable function. */
#define low_th_yield()	pthread_yield_np()
#endif
PMOD_EXPORT extern pthread_attr_t pattr;
PMOD_EXPORT extern pthread_attr_t small_pattr;

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
#define th_kill(ID, sig)

/* FIXME: Check if we can switch to the cheaper CRITICAL_SECTION objects. */

/* Note that Windows mutexes always allow recursive locking. */

#define PIKE_MUTEX_T HANDLE
#define mt_init(X) LOW_THREAD_CHECK_ZERO_ERROR ((*(X)=CreateMutex(NULL, 0, NULL)))
#define mt_lock(X)							\
  LOW_THREAD_CHECK_ZERO_ERROR (						\
    WaitForSingleObject(CheckValidHandle(*(X)), INFINITE) == WAIT_OBJECT_0)
#define mt_trylock(X)							\
  (WaitForSingleObject(CheckValidHandle(*(X)), 0) != WAIT_FAILED)
#define mt_unlock(X) LOW_THREAD_CHECK_ZERO_ERROR (ReleaseMutex(CheckValidHandle(*(X))))
#define mt_destroy(X) LOW_THREAD_CHECK_ZERO_ERROR (CloseHandle(CheckValidHandle(*(X))))

#define EVENT_T HANDLE
#define event_init(X) LOW_THREAD_CHECK_ZERO_ERROR (*(X)=CreateEvent(NULL, 1, 0, NULL))
#define event_signal(X) LOW_THREAD_CHECK_ZERO_ERROR (SetEvent(CheckValidHandle(*(X))))
#define event_destroy(X) LOW_THREAD_CHECK_ZERO_ERROR (CloseHandle(CheckValidHandle(*(X))))
#define event_wait(X)							\
  LOW_THREAD_CHECK_ZERO_ERROR (						\
    WaitForSingleObject(CheckValidHandle(*(X)), INFINITE) == WAIT_OBJECT_0)
#define event_wait_msec(X, MSEC)					\
  WaitForSingleObject(CheckValidHandle(*(X)), (MSEC))

/* No fork -- no atfork */
#define th_atfork(X,Y,Z)
#define th_atfork_prepare()
#define th_atfork_parent()
#define th_atfork_child()

/* FIXME: Use windows condition variables if running on Vista or
 * Windows Server 2008. */

#endif	/* NT_THREADS */


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

#endif	/* !COND_T && EVENT_T && PIKE_MUTEX_T */

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
#define th_equal(X,Y) (!memcmp(&(X),&(Y),sizeof(THREAD_T)))
#endif

#ifndef th_hash
#define th_hash(X) hashmem((unsigned char *)&(X),sizeof(THREAD_T), 16)
#endif

PMOD_EXPORT int co_wait_timeout(COND_T *c, PIKE_MUTEX_T *m, long s, long nanos);

#ifndef CONFIGURE_TEST

struct interleave_mutex
{
  struct interleave_mutex *next;
  struct interleave_mutex *prev;
  PIKE_MUTEX_T lock;
};

#define IMUTEX_T struct interleave_mutex

#define DEFINE_IMUTEX(name) IMUTEX_T name

PMOD_EXPORT void pike_lock_imutex (IMUTEX_T *im COMMA_DLOC_DECL);
PMOD_EXPORT void pike_unlock_imutex (IMUTEX_T *im COMMA_DLOC_DECL);

/* NOTE: Threads are enabled during the locking operation. */
#define LOCK_IMUTEX(IM) pike_lock_imutex ((IM) COMMA_DLOC)

/* NOTE: MUST be called in a THREADS_DISALLOW() context. */
#define UNLOCK_IMUTEX(IM) pike_unlock_imutex ((IM) COMMA_DLOC)

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

PMOD_EXPORT void pike_low_lock_interpreter (DLOC_DECL);
PMOD_EXPORT void pike_low_wait_interpreter (COND_T *cond COMMA_DLOC_DECL);
PMOD_EXPORT int pike_low_timedwait_interpreter (COND_T *cond,
						long sec, long nsec
						COMMA_DLOC_DECL);

PMOD_EXPORT void pike_lock_interpreter (DLOC_DECL);
PMOD_EXPORT void pike_unlock_interpreter (DLOC_DECL);
PMOD_EXPORT void pike_wait_interpreter (COND_T *cond COMMA_DLOC_DECL);
PMOD_EXPORT int pike_timedwait_interpreter (COND_T *cond,
					    long sec, long nsec
					    COMMA_DLOC_DECL);

#define low_mt_lock_interpreter() pike_low_lock_interpreter (DLOC)
#define low_co_wait_interpreter(COND)					\
  pike_low_wait_interpreter (COND COMMA_DLOC)
#define low_co_wait_interpreter_timeout(COND, SEC, NSEC)		\
  pike_low_timedwait_interpreter (COND, SEC, NSEC COMMA_DLOC)

#define mt_lock_interpreter() pike_lock_interpreter (DLOC)
#define mt_unlock_interpreter() pike_unlock_interpreter (DLOC)
#define co_wait_interpreter(COND) pike_wait_interpreter (COND COMMA_DLOC)
#define co_wait_interpreter_timeout(COND, SEC, NSEC)			\
  pike_timedwait_interpreter (COND, SEC, NSEC COMMA_DLOC)

#ifdef INTERNAL_PROFILING
PMOD_EXPORT extern unsigned long thread_yields;
#define th_yield() (thread_yields++, low_th_yield())
#else
#define th_yield() low_th_yield()
#endif

PMOD_EXPORT void pike_init_thread_state (struct thread_state *ts);
#define INIT_THREAD_STATE(TS) pike_init_thread_state (TS)
#define EXIT_THREAD_STATE(_tmp) do {					\
    DO_IF_DEBUG (Pike_sp = (struct svalue *) (ptrdiff_t) -1);		\
  } while (0)

PMOD_EXPORT void pike_swap_out_thread (struct thread_state *ts COMMA_DLOC_DECL);
PMOD_EXPORT void pike_swap_in_thread (struct thread_state *ts COMMA_DLOC_DECL);
PMOD_EXPORT void pike_swap_in_current_thread (struct thread_state *ts
					      COMMA_DLOC_DECL);

#define SWAP_OUT_THREAD(TS) pike_swap_out_thread (TS COMMA_DLOC)
#define SWAP_IN_THREAD(TS) pike_swap_in_thread (TS COMMA_DLOC)

#define SWAP_OUT_CURRENT_THREAD()					\
  do {									\
    struct thread_state *cur_ts__ = Pike_interpreter.thread_state;	\
    pike_swap_out_thread (cur_ts__ COMMA_DLOC);				\
    {

#define SWAP_IN_CURRENT_THREAD()					\
    ;}									\
    pike_swap_in_current_thread (cur_ts__ COMMA_DLOC);			\
  } while (0)

#ifdef PIKE_DEBUG
PMOD_EXPORT void debug_list_all_threads(void);
#endif

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
  void *pc_=(((unsigned char **)__builtin_frame_address(0))[1]);	\
  (((unsigned char **)__builtin_frame_address(0))[1])=0
#define REVEAL_PC \
  (((unsigned char **)__builtin_frame_address(0))[1])=pc_;

#endif	/* __i386__ */
#endif	/* __GCC__ */

#ifndef HIDE_PC
#define HIDE_PC
#define REVEAL_PC
#endif

#define HIDE_GLOBAL_VARIABLES() do {					\
    int Pike_interpreter_pointer =0;					\
    int pop_n_elems = 0;						\
    int push_sp_mark = 0, pop_sp_mark = 0, threads_disabled = 1;	\
    HIDE_PC;								\
    {

/* Note that the semi-colon below is needed to add an empty statement
 * in case there is a label before the macro.
 */
#define REVEAL_GLOBAL_VARIABLES()					\
    ;}									\
    REVEAL_PC;								\
  } while (0)

#else /* PIKE_DEBUG */
#define HIDE_GLOBAL_VARIABLES()
#define REVEAL_GLOBAL_VARIABLES()
#endif /* PIKE_DEBUG */

#ifdef PIKE_DEBUG
PMOD_EXPORT void pike_assert_thread_swapped_in (DLOC_DECL);
PMOD_EXPORT void pike_debug_check_thread (DLOC_DECL);
#define ASSERT_THREAD_SWAPPED_IN() pike_assert_thread_swapped_in (DLOC)
#define DEBUG_CHECK_THREAD() pike_debug_check_thread (DLOC)
#else
#define ASSERT_THREAD_SWAPPED_IN() do { } while (0)
#define DEBUG_CHECK_THREAD() do { } while (0)
#endif

/* THREADS_ALLOW and THREADS_DISALLOW unlocks the interpreter lock so
 * that other threads may run in the pike interpreter. Must be used
 * around code that might block, and may be used around code that just
 * takes a lot of time. Note though in the latter case that the
 * locking overhead is significant - the work should be on the order
 * of microseconds or else it might even get slower.
 *
 * Between THREADS_ALLOW and THREADS_DISALLOW, you may not change any
 * data structure that might be accessible by other threads, and you
 * may not access any data they could possibly change.
 *
 * The following rules are just some special cases of the preceding
 * statement:
 *
 * o  You may not throw pike exceptions, not even if you catch them
 *    yourself.
 * o  You may not create instances of any pike data type, because that
 *    always implicitly modifies global structures (e.g. the doubly
 *    linked lists that link together all arrays, mappings, multisets,
 *    objects, and programs).
 * o  You may not change the refcount of any pike structure, unless
 *    all its other references are from your own code (see also last
 *    item below).
 * o  If you (prior to THREADS_ALLOW) have added your own ref to some
 *    structure, you can assume that it continues to exist. Refs from
 *    your own stack count in this regard, because you know that they
 *    will remain until THREADS_DISALLOW. Note that even objects are
 *    guaranteed to continue to exist, but they may be destructed at
 *    any time.
 * o  Only immutable data in structs you have refs to may be read, and
 *    nothing may be changed.
 * o  If you before THREADS_ALLOW have created your own instance of
 *    some data type, i.e. so that you got the only ref to it, you can
 *    safely change it. But changes that have effect on global
 *    structures are still verboten - that includes freeing any pike
 *    data type, and calling functions like finish_string_builder,
 *    just to name one.
 */

/* The difference between THREADS_ALLOW and THREADS_ALLOW_UID is that
 * _disable_threads waits for the latter to hold in
 * THREADS_DISALLOW_UID before returning. Otoh, THREADS_ALLOW sections
 * might continue to run when threads are officially disabled. That is
 * fine as long as they don't have any effect on the process state
 * that is observable from other threads. Iow, THREADS_ALLOW_UID is
 * necessary when doing any kind of I/O, calling nonreentrant
 * functions, or similar. */

PMOD_EXPORT void pike_threads_allow (struct thread_state *ts
				     COMMA_DLOC_DECL);
PMOD_EXPORT void pike_threads_disallow (struct thread_state *ts
					COMMA_DLOC_DECL);
PMOD_EXPORT void pike_threads_allow_ext (struct thread_state *ts
					 COMMA_DLOC_DECL);
PMOD_EXPORT void pike_threads_disallow_ext (struct thread_state *ts
					    COMMA_DLOC_DECL);

#define THREADS_ALLOW() do {						\
    struct thread_state *cur_ts__ = Pike_interpreter.thread_state;	\
    pike_threads_allow (cur_ts__ COMMA_DLOC);				\
    HIDE_GLOBAL_VARIABLES();						\
    {

#define THREADS_DISALLOW()						\
    ;}									\
    REVEAL_GLOBAL_VARIABLES();						\
    pike_threads_disallow (cur_ts__ COMMA_DLOC);			\
  } while (0)

#define THREADS_ALLOW_UID() do {					\
    struct thread_state *cur_ts_ext__ = Pike_interpreter.thread_state;	\
    pike_threads_allow_ext (cur_ts_ext__ COMMA_DLOC);			\
    HIDE_GLOBAL_VARIABLES();						\
    {

#define THREADS_DISALLOW_UID()						\
    ;}									\
    REVEAL_GLOBAL_VARIABLES();						\
    pike_threads_disallow_ext (cur_ts_ext__ COMMA_DLOC);		\
  } while (0)

/* FIXME! The macro below leaks live_threads!
 *        Avoid if possible!
 */
#define SWAP_IN_THREAD_IF_REQUIRED()					\
  pike_threads_disallow (thread_state_for_id(th_self()) COMMA_DLOC)

#endif	/* !CONFIGURE_TEST */

#endif /* PIKE_THREADS */

#ifndef PIKE_THREADS

#define THREAD_T	void *
#define THREAD_T_IS_POINTER

#define th_atfork(X,Y,Z)
#define th_atfork_prepare()
#define th_atfork_parent()
#define th_atfork_child()

#define th_setconcurrency(X)
#define DEFINE_MUTEX(X)
#define DEFINE_IMUTEX(X)
#define init_interleave_mutex(X)
#define exit_interleave_mutex(X)
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
#define th_kill(ID, sig)
#define co_wait(X,Y)
#define co_wait_timeout(X,Y,S,N)
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

/* Initializer macros for static mutex and condition variables */
#ifdef PTHREAD_MUTEX_INITIALIZER
#define HAS_STATIC_MUTEX_INIT
#define STATIC_MUTEX_INIT  = PTHREAD_MUTEX_INITIALIZER
#else
#define STATIC_MUTEX_INIT
#endif
#ifdef PTHREAD_COND_INITIALIZER
#define HAS_STATIC_COND_INIT
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

#ifdef PIKE_DEBUG
extern THREAD_T threads_disabled_thread;
#endif

#endif /* PIKE_THREADLIB_H */
