/*
 * $Id: threads.h,v 1.121 2004/03/08 15:02:43 grubba Exp $
 */
#ifndef THREADS_H
#define THREADS_H

#include "machine.h"
#include "object.h"
#include "pike_error.h"
#include "interpret.h"
#include "main.h"

/* Needed for the sigset_t typedef, which is needed for
 * the pthread_sigsetmask() prototype on Solaris 2.x.
 */
#include <signal.h>

#ifdef HAVE_SYS_TYPES_H
/* Needed for pthread_t on OSF/1 */
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */
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

#ifdef HAVE_MACH_TASK_INFO_H
#include <mach/task_info.h>
#endif
#ifdef HAVE_MACH_TASK_H
#include <mach/task.h>
#endif
#ifdef HAVE_MACH_MACH_INIT_H
#include <mach/mach_init.h>
#endif

/* AIX is *STUPID* - Hubbe */
#undef func_data

#undef HAVE_PTHREAD_H
#endif /* _MIT_POSIX_THREADS */

#ifdef _SGI_SPROC_THREADS
/* Not supported yet */
#undef SGI_SPROC_THREADS
#undef HAVE_SPROC
#endif /* _SGI_SPROC_THREADS */



/* Restore the fp macro. */
#ifdef FRAMEPOINTER_WAS_DEFINED
#define fp Pike_fp
#undef FRAMEPOINTER_WAS_DEFINED
#endif /* FRAMEPOINTER_WAS_DEFINED */


extern int num_threads;
PMOD_EXPORT extern int live_threads;
struct object;
PMOD_EXPORT extern size_t thread_stack_size;

PMOD_EXPORT void thread_low_error (int errcode, const char *cmd,
                                   const char *fname, int lineno);

#define LOW_THREAD_CHECK_NONZERO_ERROR(CALL) do {			\
    int thread_errcode_ = (CALL);					\
    if (thread_errcode_)                                                \
      thread_low_error(thread_errcode_, TOSTR(CALL),                    \
                       __FILE__, __LINE__);                             \
  } while (0)

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

#if !defined(HAVE_PTHREAD_MUTEX_ERRORCHECK_NP) && defined(HAVE_PTHREAD_MUTEX_ERRORCHECK)
#define PTHREAD_MUTEX_ERRORCHECK_NP PTHREAD_MUTEX_ERRORCHECK
#define HAVE_PTHREAD_MUTEX_ERRORCHECK_NP
#endif

#ifdef HAVE_PTHREAD_MUTEX_ERRORCHECK_NP
#define mt_init(X) do {							\
    if (debug_options & ERRORCHECK_MUTEXES) {				\
      pthread_mutexattr_t attr;						\
      pthread_mutexattr_init(&attr);					\
      pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK_NP);	\
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

#if !defined(HAVE_PTHREAD_MUTEX_RECURSIVE_NP) && defined(HAVE_PTHREAD_MUTEX_RECURSIVE)
#define PTHREAD_MUTEX_RECURSIVE_NP PTHREAD_MUTEX_RECURSIVE
#define HAVE_PTHREAD_MUTEX_RECURSIVE_NP
#endif

#ifdef HAVE_PTHREAD_MUTEX_RECURSIVE_NP
#define mt_init_recursive(X)						\
    do{ \
      pthread_mutexattr_t attr;					\
      pthread_mutexattr_init(&attr);					\
      pthread_mutexattr_settype(					\
	&attr,								\
	PTHREAD_MUTEX_RECURSIVE_NP					\
	DO_IF_PIKE_MUTEX_ERRORCHECK (| PTHREAD_MUTEX_ERRORCHECK_NP));	\
      pthread_mutex_init((X), &attr);					\
    }while(0)
#endif

#define mt_lock(X) LOW_THREAD_CHECK_NONZERO_ERROR (pthread_mutex_lock(X))
#define mt_trylock(X) pthread_mutex_trylock(X)
#define mt_unlock(X) LOW_THREAD_CHECK_NONZERO_ERROR (pthread_mutex_unlock(X))
#define mt_destroy(X) pthread_mutex_destroy(X)

/* SIGH! No setconcurrency in posix threads. This is more or less
 * needed to make usable multi-threaded programs on solaris machines
 * with only one CPU. Otherwise, only systemcalls are actually
 * threaded.
 */
#define th_setconcurrency(X) 
#ifdef HAVE_PTHREAD_YIELD
#define th_yield()	pthread_yield()
#else
#ifdef HAVE_PTHREAD_YIELD_NP
/* Some pthread libs define yield as non-portable function. */
#define th_yield()	pthread_yield_np()
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
#define th_getpecific pthread_getspecific


#ifdef HAVE_PTHREAD_KILL
#define th_kill(ID,sig) pthread_kill((ID),(sig))
#else /* !HAVE_PTHREAD_KILL */
/* MacOS X (aka Darwin) prior to 10.2 doesn't have pthread_kill. */
#define th_kill(ID,sig)
#endif /* HAVE_PTHREAD_KILL */
#define th_join(ID,res) pthread_join((ID),(res))
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
#define co_destroy(X) pthread_cond_destroy(X)
#else
#error No way to make cond-vars
#endif /* HAVE_PTHREAD_COND_INIT */

#endif /* POSIX_THREADS */




#ifdef UNIX_THREADS
#define THREAD_T thread_t
#define PTHREAD_MUTEX_INITIALIZER DEFAULTMUTEX
#define PIKE_MUTEX_T mutex_t
#define mt_init(X) mutex_init((X),USYNC_THREAD,0)
#define mt_lock(X) LOW_THREAD_CHECK_NONZERO_ERROR (mutex_lock(X))
#define mt_trylock(X) mutex_trylock(X)
#define mt_unlock(X) LOW_THREAD_CHECK_NONZERO_ERROR (mutex_unlock(X))
#define mt_destroy(X) mutex_destroy(X)

#define th_setconcurrency(X) thr_setconcurrency(X)

#define th_create(ID,fun,arg) thr_create(NULL,thread_stack_size,fun,arg,THR_DAEMON|THR_DETACHED,ID)
#define th_create_small(ID,fun,arg) thr_create(NULL,8192*sizeof(char *),fun,arg,THR_DAEMON|THR_DETACHED,ID)
#define th_exit(foo) thr_exit(foo)
#define th_self() thr_self()
#define th_kill(ID,sig) thr_kill((ID),(sig))
#define th_yield() thr_yield()
#define th_join(ID,res) thr_join((ID), NULL, (res))

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
#define th_yield()	sginap(0)
#define th_join(ID,res)	/*********/
#define th_equal(X,Y) ((X)==(Y))
#define th_hash(X) ((unsigned INT32)(X))

/*
 * No cond_vars yet
 */

#endif /* SGI_SPROC_THREADS */


#ifdef NT_THREADS
#include <process.h>
#include <windows.h>
 
#define LOW_THREAD_CHECK_ZERO_ERROR(CALL) do {				\
    if (!(CALL)) thread_low_error (GetLastError());			\
  } while (0)

#define THREAD_T unsigned
#define th_setconcurrency(X)
#define th_create(ID,fun,arg) low_nt_create_thread(2*1024*1024,fun, arg,ID)
#define th_create_small(ID,fun,arg) low_nt_create_thread(8192*sizeof(char *), fun,arg,ID)
#define TH_RETURN_TYPE unsigned __stdcall
#define TH_STDCALL __stdcall
#define th_exit(foo) _endthreadex(foo)
#define th_join(ID,res)	/******************* FIXME! ****************/
#define th_self() GetCurrentThreadId()
#define th_destroy(X)
#define th_yield() Sleep(0)
#define th_equal(X,Y) ((X)==(Y))
#define th_hash(X) (X)

#define PIKE_MUTEX_T HANDLE
#define mt_init(X) CheckValidHandle((*(X)=CreateMutex(NULL, 0, NULL)))
#define mt_lock(X)							\
  LOW_THREAD_CHECK_ZERO_ERROR (						\
    WaitForSingleObject(CheckValidHandle(*(X)), INFINITE) == WAIT_OBJECT_0)
#define mt_trylock(X)							\
  LOW_THREAD_CHECK_ZERO_ERROR (						\
    WaitForSingleObject(CheckValidHandle(*(X)), 0) != WAIT_FAILED)
#define mt_unlock(X) LOW_THREAD_CHECK_ZERO_ERROR (ReleaseMutex(CheckValidHandle(*(X))))
#define mt_destroy(X) CloseHandle(CheckValidHandle(*(X)))

#define EVENT_T HANDLE
#define event_init(X) CheckValidHandle(*(X)=CreateEvent(NULL, 1, 0, NULL))
#define event_signal(X) LOW_THREAD_CHECK_ZERO_ERROR (SetEvent(CheckValidHandle(*(X))))
#define event_destroy(X) CloseHandle(CheckValidHandle(*(X)))
#define event_wait(X)							\
  LOW_THREAD_CHECK_ZERO_ERROR (						\
    WaitForSingleObject(CheckValidHandle(*(X)), INFINITE) == WAIT_OBJECT_0)

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

extern int th_running;

PMOD_EXPORT extern PIKE_MUTEX_T interpreter_lock;

#if defined(PIKE_DEBUG) && !defined(__NT__)

/* This is a debug wrapper to enable checks that the interpreter lock
 * is hold by the current thread. */

extern THREAD_T debug_locking_thread;
#define SET_LOCKING_THREAD (debug_locking_thread = th_self(), 0)

#define mt_lock_interpreter()						\
  do {mt_lock(&interpreter_lock); SET_LOCKING_THREAD;} while (0)
#define mt_trylock_interpreter() (mt_trylock(&interpreter_lock) || SET_LOCKING_THREAD)
#define mt_unlock_interpreter() do {mt_unlock(&interpreter_lock);} while (0)
#define co_wait_interpreter(COND) \
  do {co_wait((COND), &interpreter_lock); SET_LOCKING_THREAD;} while (0)

#define CHECK_INTERPRETER_LOCK() do {					\
  if (th_running) {							\
    THREAD_T self;							\
    if (!mt_trylock(&interpreter_lock))					\
      fatal("Interpreter is not locked.\n");				\
    self = th_self();							\
    if (!th_equal(debug_locking_thread, self))				\
      fatal("Interpreter is not locked by this thread.\n");		\
  }									\
} while (0)

#else

#define mt_lock_interpreter() do {mt_lock(&interpreter_lock);} while (0)
#define mt_trylock_interpreter() (mt_trylock(&interpreter_lock))
#define mt_unlock_interpreter() do {mt_unlock(&interpreter_lock);} while (0)
#define co_wait_interpreter(COND) do {co_wait((COND), &interpreter_lock);} while (0)

#endif

PMOD_EXPORT extern COND_T live_threads_change;		/* Used by _disable_threads */
PMOD_EXPORT extern COND_T threads_disabled_change;		/* Used by _disable_threads */

struct svalue;
struct pike_frame;

extern PIKE_MUTEX_T interleave_lock;

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
      THREADS_FPRINTF(0, (stderr, "Locking IMutex 0x%08p...\n", (im))); \
      THREADS_ALLOW(); \
      mt_lock(&((im)->lock)); \
      THREADS_DISALLOW(); \
    } \
  } while(0)

/* If threads are disabled, the lock will be released later. */
#define UNLOCK_IMUTEX(im) do { \
    if (!threads_disabled) { \
      THREADS_FPRINTF(0, (stderr, "Unlocking IMutex 0x%08p...\n", (im))); \
      mt_unlock(&((im)->lock)); \
    } \
  } while(0)

#define THREAD_NOT_STARTED -1
#define THREAD_RUNNING 0
#define THREAD_EXITED 1

struct thread_state {
  struct Pike_interpreter state;
  char swapped;
  char status;
  COND_T status_change;
  THREAD_T id;
  struct mapping *thread_local;
  struct thread_state *hashlink, **backlink;
#ifdef PROFILING
#if SIZEOF_LONG_LONG - 0 != 0
  long long time_base;
#else
  long time_base;
#endif
#endif /* PROFILING */
};


#ifndef TH_RETURN_TYPE
#define TH_RETURN_TYPE void *
#endif

#ifndef TH_STDCALL
#define TH_STDCALL
#endif

#ifndef th_destroy
#define th_destroy(X)
#endif

#ifndef th_yield
#ifdef HAVE_THR_YIELD
#define th_yield() thr_yield()
#else
#define th_yield() 0
#define HAVE_NO_YIELD
#endif
#endif

#ifndef th_equal
#define th_equal(X,Y) (!MEMCMP(&(X),&(Y),sizeof(THREAD_T)))
#endif

#ifndef th_hash
#define th_hash(X) hashmem((unsigned char *)&(X),sizeof(THREAD_T), 16)
#endif

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
#define THREADS_FPRINTF(L,X)	do { \
    if ((VERBOSE_THREADS_DEBUG + 0) >= (L)) fprintf X; \
  } while(0)
#endif /* VERBOSE_THREADS_DEBUG */

#ifdef THREAD_TRACE
PMOD_EXPORT extern int t_flag;
#define SWAP_OUT_TRACE(_tmp)	do { (_tmp)->state.t_flag = t_flag; } while(0)
#define SWAP_IN_TRACE(_tmp)	do { t_flag = (_tmp)->state.t_flag; } while(0)
#else /* !THREAD_TRACE */
#define SWAP_OUT_TRACE(_tmp)
#define SWAP_IN_TRACE(_tmp)
#endif /* THREAD_TRACE */

#if defined(PROFILING) && defined(HAVE_GETHRTIME)
#define DO_IF_PROFILING(X) X
#else
#define DO_IF_PROFILING(X)
#endif

#define SWAP_OUT_THREAD(_tmp) do {				\
       (_tmp)->state=Pike_interpreter;				\
       (_tmp)->swapped=1;					\
       DO_IF_PROFILING( (_tmp)->time_base += gethrtime() ; )	\
       SWAP_OUT_TRACE(_tmp);					\
       Pike_interpreter.thread_id = (struct object *) (ptrdiff_t) -1;	\
      } while(0)

#define SWAP_IN_THREAD(_tmp) do {					\
       (_tmp)->swapped=0;						\
       Pike_interpreter=(_tmp)->state;					\
       DO_IF_USE_CLOCK_FOR_SLICES (thread_start_clock = 0);		\
       DO_IF_PROFILING(  Pike_interpreter.time_base -=  gethrtime();)	\
       SWAP_IN_TRACE(_tmp);						\
     } while(0)

#define SWAP_OUT_CURRENT_THREAD() \
  do {\
     struct thread_state *_tmp=OBJ2THREAD(Pike_interpreter.thread_id); \
     SWAP_OUT_THREAD(_tmp); \
     THREADS_FPRINTF(1, (stderr, "SWAP_OUT_CURRENT_THREAD() %s:%d t:%08x\n", \
			 __FILE__, __LINE__, (unsigned int)_tmp->thread_id)) \

#define SWAP_IN_CURRENT_THREAD() \
   THREADS_FPRINTF(1, (stderr, "SWAP_IN_CURRENT_THREAD() %s:%d ... t:%08x\n", \
		       __FILE__, __LINE__, (unsigned int)_tmp->thread_id)); \
   SWAP_IN_THREAD(_tmp);\
 } while(0)

#if defined(PIKE_DEBUG) && ! defined(DONT_HIDE_GLOBALS)
/* Note that scalar types are used in place of pointers and vice versa
 * below. This is intended to cause compiler warnings/errors if
 * there is an attempt to use the global variables in an unsafe
 * environment.
 */
#define HIDE_GLOBAL_VARIABLES() do { \
   int Pike_interpreter =0; \
   int pop_n_elems = 0; \
   int push_sp_mark = 0, pop_sp_mark = 0, threads_disabled = 1

/* Note that the semi-colon below is needed to add an empty statement
 * in case there is a label before the macro.
 */
#define REVEAL_GLOBAL_VARIABLES() ; } while(0)
#else /* PIKE_DEBUG */
#define HIDE_GLOBAL_VARIABLES()
#define REVEAL_GLOBAL_VARIABLES()
#endif /* PIKE_DEBUG */

#define	OBJ2THREAD(X) \
  ((struct thread_state *)((X)->storage+thread_storage_offset))

#define THREADSTATE2OBJ(X) ((X)->state.thread_id)

PMOD_EXPORT extern int Pike_in_gc;
#define THREADS_ALLOW() do { \
     struct thread_state *_tmp=OBJ2THREAD(Pike_interpreter.thread_id); \
     DO_IF_DEBUG({ \
       if(thread_for_id(th_self()) != Pike_interpreter.thread_id) \
	 fatal("thread_for_id() (or Pike_interpreter.thread_id) failed!" \
               " %p != %p\n", \
               thread_for_id(th_self()), Pike_interpreter.thread_id); \
       if (Pike_in_gc > 50 && Pike_in_gc < 300) \
	 fatal("Threads allowed during garbage collection.\n"); \
     }) \
     if(num_threads > 1 && !threads_disabled) { \
       SWAP_OUT_THREAD(_tmp); \
       THREADS_FPRINTF(1, (stderr, "THREADS_ALLOW() %s:%d t:%08x(#%d)\n", \
			   __FILE__, __LINE__, \
			   (unsigned int)_tmp->thread_id, live_threads)); \
       mt_unlock_interpreter(); \
     } else {} \
     HIDE_GLOBAL_VARIABLES()

#define THREADS_DISALLOW() \
     REVEAL_GLOBAL_VARIABLES(); \
     if(_tmp->swapped) { \
       mt_lock_interpreter(); \
       THREADS_FPRINTF(1, (stderr, "THREADS_DISALLOW() %s:%d t:%08x(#%d)\n", \
			   __FILE__, __LINE__, \
                           (unsigned int)_tmp->thread_id, live_threads)); \
       while (threads_disabled) { \
         THREADS_FPRINTF(1, (stderr, \
                             "THREADS_DISALLOW(): Threads disabled\n")); \
         co_wait_interpreter(&threads_disabled_change); \
       } \
       SWAP_IN_THREAD(_tmp);\
     } \
     DO_IF_DEBUG( if(thread_for_id(th_self()) != Pike_interpreter.thread_id) \
        fatal("thread_for_id() (or Pike_interpreter.thread_id) failed! %p != %p\n",thread_for_id(th_self()),Pike_interpreter.thread_id) ; ) \
   } while(0)

#define THREADS_ALLOW_UID() do { \
     struct thread_state *_tmp_uid=OBJ2THREAD(Pike_interpreter.thread_id); \
     DO_IF_DEBUG({ \
       if(thread_for_id(th_self()) != Pike_interpreter.thread_id) { \
	 fatal("thread_for_id() (or Pike_interpreter.thread_id) failed! %p != %p\n", \
               thread_for_id(th_self()),Pike_interpreter.thread_id); \
       } \
       if ((Pike_in_gc > 50) && (Pike_in_gc < 300)) { \
         fprintf(stderr, __FILE__ ":" DEFINETOSTR(__LINE__) ": Fatal error:\n"); \
	 debug_fatal("Threads allowed during garbage collection (%d).\n", \
                     Pike_in_gc); \
       } \
     }) \
     if(num_threads > 1 && !threads_disabled) { \
       SWAP_OUT_THREAD(_tmp_uid); \
       live_threads++; \
       THREADS_FPRINTF(1, (stderr, "THREADS_ALLOW_UID() %s:%d t:%08x(#%d)\n", \
			   __FILE__, __LINE__, \
			   (unsigned int)_tmp_uid->thread_id, live_threads)); \
       mt_unlock_interpreter(); \
     } else {} \
     HIDE_GLOBAL_VARIABLES()

#define THREADS_DISALLOW_UID() \
     REVEAL_GLOBAL_VARIABLES(); \
     if(_tmp_uid->swapped) { \
       mt_lock_interpreter(); \
       live_threads--; \
       THREADS_FPRINTF(1, (stderr, \
                           "THREADS_DISALLOW_UID() %s:%d t:%08x(#%d)\n", \
			   __FILE__, __LINE__, \
                           (unsigned int)_tmp_uid->thread_id, live_threads)); \
       co_broadcast(&live_threads_change); \
       while (threads_disabled) { \
         THREADS_FPRINTF(1, (stderr, "THREADS_DISALLOW_UID(): Wait...\n")); \
         co_wait_interpreter(&threads_disabled_change); \
       } \
       SWAP_IN_THREAD(_tmp_uid);\
     } \
   } while(0)

#define SWAP_IN_THREAD_IF_REQUIRED() do { 			\
  struct thread_state *_tmp=thread_state_for_id(th_self());	\
  HIDE_GLOBAL_VARIABLES();					\
  THREADS_DISALLOW()

#ifdef PIKE_DEBUG
#define ASSERT_THREAD_SWAPPED_IN() do {				\
    struct thread_state *_tmp=thread_state_for_id(th_self());	\
    if(_tmp->swapped) fatal("Thread is not swapped in!\n");	\
  }while(0)

#else
#define ASSERT_THREAD_SWAPPED_IN()
#endif

/* Prototypes begin here */
int low_nt_create_thread(unsigned Pike_stack_size,
			 unsigned (TH_STDCALL *func)(void *),
			 void *arg,
			 unsigned *id);
struct thread_starter;
struct thread_local;
void low_init_threads_disable(void);
void init_threads_disable(struct object *o);
void exit_threads_disable(struct object *o);
void init_interleave_mutex(IMUTEX_T *im);
void exit_interleave_mutex(IMUTEX_T *im);
void thread_table_init(void);
unsigned INT32 thread_table_hash(THREAD_T *tid);
PMOD_EXPORT void thread_table_insert(struct object *o);
PMOD_EXPORT void thread_table_delete(struct object *o);
PMOD_EXPORT struct thread_state *thread_state_for_id(THREAD_T tid);
PMOD_EXPORT struct object *thread_for_id(THREAD_T tid);
PMOD_EXPORT void f_all_threads(INT32 args);
PMOD_EXPORT int count_pike_threads(void);
TH_RETURN_TYPE new_thread_func(void * data);
void f_thread_create(INT32 args);
void f_thread_set_concurrency(INT32 args);
PMOD_EXPORT void f_this_thread(INT32 args);
struct mutex_storage;
struct key_storage;
void f_mutex_lock(INT32 args);
void f_mutex_trylock(INT32 args);
void init_mutex_obj(struct object *o);
void exit_mutex_obj(struct object *o);
void init_mutex_key_obj(struct object *o);
void exit_mutex_key_obj(struct object *o);
void f_cond_wait(INT32 args);
void f_cond_signal(INT32 args);
void f_cond_broadcast(INT32 args);
void init_cond_obj(struct object *o);
void exit_cond_obj(struct object *o);
void f_thread_backtrace(INT32 args);
void f_thread_id_status(INT32 args);
void init_thread_obj(struct object *o);
void exit_thread_obj(struct object *o);
PMOD_EXPORT void f_thread_local(INT32 args);
void f_thread_local_get(INT32 args);
void f_thread_local_set(INT32 args);
void low_th_init(void);
void th_init(void);
void th_cleanup(void);
int th_num_idle_farmers(void);
int th_num_farmers(void);
PMOD_EXPORT void th_farm(void (*fun)(void *), void *here);
/* Prototypes end here */

#else

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
#define th_self() ((void*)0)
#define co_wait(X,Y)
#define co_signal(X)
#define co_broadcast(X)

#define low_init_threads_disable()
#define init_threads_disable(X)
#define exit_threads_disable(X)


#endif /* PIKE_THREADS */

#ifndef CHECK_INTERPRETER_LOCK
#define CHECK_INTERPRETER_LOCK() do {} while (0)
#endif

#ifdef __NT__
#ifndef PIKE_DEBUG
#define CheckValidHandle(X) (X)
#else
PMOD_EXPORT HANDLE CheckValidHandle(HANDLE h);
#endif
#endif

extern int threads_disabled;
PMOD_EXPORT extern ptrdiff_t thread_storage_offset;
PMOD_EXPORT extern struct program *thread_id_prog;

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


/* for compatibility */
#include "interpret.h"

#endif /* THREADS_H */

