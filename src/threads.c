/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: threads.c,v 1.205 2004/04/26 16:21:18 mast Exp $
*/

#include "global.h"
RCSID("$Id: threads.c,v 1.205 2004/04/26 16:21:18 mast Exp $");

PMOD_EXPORT int num_threads = 1;
PMOD_EXPORT int threads_disabled = 0;

/* #define PICKY_MUTEX */

#ifdef _REENTRANT
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
#include "rusage.h"

#include <errno.h>

#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif /* HAVE_SYS_PRCTL_H */

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifndef PIKE_THREAD_C_STACK_SIZE
#define PIKE_THREAD_C_STACK_SIZE (256 * 1024)
#endif

PMOD_EXPORT int live_threads = 0, disallow_live_threads = 0;
PMOD_EXPORT COND_T live_threads_change;
PMOD_EXPORT COND_T threads_disabled_change;
PMOD_EXPORT size_t thread_stack_size=PIKE_THREAD_C_STACK_SIZE;

PMOD_EXPORT void thread_low_error (int errcode)
{
  Pike_fatal ("Unexpected error from thread function: %d\n", errcode);
}

/* SCO magic... */
int  __thread_sys_behavior = 1;

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

#ifdef __NT__

int low_nt_create_thread(unsigned Pike_stack_size,
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
    return 1;
  }
}


#ifdef PIKE_DEBUG
PMOD_EXPORT HANDLE CheckValidHandle(HANDLE h);
#endif

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

#endif


#define THIS_THREAD ((struct thread_state *)CURRENT_STORAGE)

static struct callback *threads_evaluator_callback=0;
int thread_id_result_variable;

int th_running = 0;
#ifdef PIKE_DEBUG
int debug_interpreter_is_locked = 0;
THREAD_T debug_locking_thread;
THREAD_T threads_disabled_thread = 0;
#endif
#ifdef INTERNAL_PROFILING
PMOD_EXPORT unsigned long thread_yields = 0;
#endif
PMOD_EXPORT MUTEX_T interpreter_lock;
MUTEX_T thread_table_lock, interleave_lock;
struct program *mutex_key = 0;
PMOD_EXPORT struct program *thread_id_prog = 0;
struct program *thread_local_prog = 0;
#ifdef POSIX_THREADS
pthread_attr_t pattr;
pthread_attr_t small_pattr;
#endif
PMOD_EXPORT ptrdiff_t thread_storage_offset;
#ifdef USE_CLOCK_FOR_SLICES
PMOD_EXPORT clock_t thread_start_clock = 0;
#endif

struct thread_starter
{
  struct object *id;
  struct array *args;
#ifdef HAVE_BROKEN_LINUX_THREAD_EUID
  int euid, egid;
#endif /* HAVE_BROKEN_LINUX_THREAD_EUID */
};

struct thread_local
{
  INT32 id;
};

static volatile IMUTEX_T *interleave_list = NULL;

/* This is a variant of init_threads_disable that blocks all other
 * threads that might run pike code, but still doesn't block the
 * THREADS_ALLOW_UID threads. */
void low_init_threads_disable(void)
{
  /* Serious black magic to avoid dead-locks */

  if (!threads_disabled) {
    THREADS_FPRINTF(0,
		    (stderr, "low_init_threads_disable(): Locking IM's...\n"));

    if (Pike_interpreter.thread_id) {
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
 */
void init_threads_disable(struct object *o)
{
  disallow_live_threads = 1;

  if(live_threads) {
    SWAP_OUT_CURRENT_THREAD();
    while (live_threads) {
      THREADS_FPRINTF(0,
		      (stderr,
		       "_disable_threads(): Waiting for %d threads to finish\n",
		       live_threads));
      low_co_wait_interpreter(&live_threads_change);
    }
    SWAP_IN_CURRENT_THREAD();
  }

  low_init_threads_disable();
}

void exit_threads_disable(struct object *o)
{
  THREADS_FPRINTF(0, (stderr, "exit_threads_disable(): threads_disabled:%d\n",
		      threads_disabled));
  if(threads_disabled) {
    if(!--threads_disabled) {
      IMUTEX_T *im = (IMUTEX_T *)interleave_list;

      /* Order shouldn't matter for unlock, so no need to do it backwards. */
      while(im) {
	THREADS_FPRINTF(0,
			(stderr,
			 "exit_threads_disable(): Unlocking IM 0x%p\n", im));
	mt_unlock(&(im->lock));
	im = im->next;
      }

      mt_unlock(&interleave_lock);

      THREADS_FPRINTF(0, (stderr, "_exit_threads_disable(): Wake up!\n"));
      disallow_live_threads = 0;
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

  THREADS_FPRINTF(0, (stderr,
		      "init_interleave_mutex(): Locking IM 0x%p\n", im));

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

#define THREAD_TABLE_SIZE 127  /* Totally arbitrary prime */

static struct thread_state *thread_table_chains[THREAD_TABLE_SIZE];
static int num_pike_threads=0;

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

#ifdef PIKE_DEBUG
void dumpmem(char *desc, void *x, int size)
{
  int e;
  unsigned char *tmp=(unsigned char *)x;
  fprintf(stderr,"%s: ",desc);
  for(e=0;e<size;e++)
    fprintf(stderr,"%02x",tmp[e]);
  fprintf(stderr,"\n");
}
#endif


PMOD_EXPORT void thread_table_insert(struct object *o)
{
  struct thread_state *s = OBJ2THREAD(o);
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
/*  dumpmem("thread_table_insert",&s->id, sizeof(THREAD_T)); */
#endif
  mt_lock( & thread_table_lock );
  num_pike_threads++;
  if((s->hashlink = thread_table_chains[h]) != NULL)
    s->hashlink->backlink = &s->hashlink;
  thread_table_chains[h] = s;
  s->backlink = &thread_table_chains[h];
  mt_unlock( & thread_table_lock );  
}

PMOD_EXPORT void thread_table_delete(struct object *o)
{
  struct thread_state *s = OBJ2THREAD(o);
/*  dumpmem("thread_table_delete",&s->id, sizeof(THREAD_T)); */
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
    dumpmem("thread_state_for_id: ",&tid,sizeof(tid));
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
    dumpmem("thread_state_for_id return value: ",&s->id,sizeof(tid));
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

  INT32 x;
  struct svalue *oldsp;
  struct thread_state *s;

  pop_n_elems(args);
  oldsp = Pike_sp;
  mt_lock( & thread_table_lock );
  for(x=0; x<THREAD_TABLE_SIZE; x++)
    for(s=thread_table_chains[x]; s; s=s->hashlink) {
      struct object *o = THREADSTATE2OBJ(s);
      if (o) {
	ref_push_object(o);
      }
    }
  mt_unlock( & thread_table_lock );
  f_aggregate(DO_NOT_WARN(Pike_sp - oldsp));
}

#ifdef PIKE_DEBUG
void debug_list_all_threads(void)
{
  INT32 x;
  struct thread_state *s;
  THREAD_T self = th_self();

  fprintf(stderr,"--Listing all threads--\n");
  dumpmem("Current thread: ",&self, sizeof(self));
  fprintf(stderr,"Current thread obj: %p\n",Pike_interpreter.thread_id);
  fprintf(stderr,"Current thread hash: %d\n",thread_table_hash(&self));
  fprintf(stderr,"Current stack pointer: %p\n",&self);
  for(x=0; x<THREAD_TABLE_SIZE; x++)
  {
    for(s=thread_table_chains[x]; s; s=s->hashlink) {
      struct object *o = THREADSTATE2OBJ(s);
      fprintf(stderr,"ThTab[%d]: %p (stackbase=%p)",x,o,s->state.stack_top);
      dumpmem(" ",&s->id, sizeof(s->id));
    }
  }
  fprintf(stderr,"-----------------------\n");
}
#endif

PMOD_EXPORT int count_pike_threads(void)
{
  return num_pike_threads;
}

static void check_threads(struct callback *cb, void *arg, void * arg2)
{
#ifndef HAVE_NO_YIELD
  /* If we have no yield we can't cut calls here since it's possible
   * that a thread switch will take place only occasionally in the
   * window below. */
  static int div_;
  if(div_++ & 255)
    return;
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
    
    /* Get user time and test if 50usec has passed since last check. */
    if (task_info(mach_task_self(), TASK_THREAD_TIMES_INFO,
		  (task_info_t) &info, &info_size) == 0) {
      /* Compute difference by converting kernel time_info_t to timeval. */
      struct timeval now;
      struct timeval diff;
      now.tv_sec = info.user_time.seconds;
      now.tv_usec = info.user_time.microseconds;
      timersub(&now, &last_check, &diff);
      if (diff.tv_usec < 50000 && diff.tv_sec == 0)
	return;
      last_check = now;
    }
  }
#elif defined (USE_CLOCK_FOR_SLICES)
  if (clock() - thread_start_clock < (clock_t) (CLOCKS_PER_SEC / 20))
    return;
#endif
#endif

#ifdef DEBUG
  if(thread_for_id(th_self()) != Pike_interpreter.thread_id) {
    debug_list_all_threads();
    Pike_fatal("thread_for_id() (or Pike_interpreter.thread_id) failed! %p != %p\n",thread_for_id(th_self()),Pike_interpreter.thread_id) ;
  }

  if(Pike_interpreter.backlink != OBJ2THREAD(Pike_interpreter.thread_id))
    Pike_fatal("Hashlink is wrong!\n");
#endif

  THREADS_ALLOW();
  /* Allow other threads to run */
  th_yield();
  THREADS_DISALLOW();

#ifdef USE_CLOCK_FOR_SLICES
  /* Must set the base time for the slice here since clock() returns
   * thread local time. */
  thread_start_clock = clock();
#endif

  DO_IF_DEBUG(
    if(thread_for_id(th_self()) != Pike_interpreter.thread_id) {
      debug_list_all_threads();
      Pike_fatal("thread_for_id() (or Pike_interpreter.thread_id) failed! %p != %p\n",thread_for_id(th_self()),Pike_interpreter.thread_id) ;
    } )
}

TH_RETURN_TYPE new_thread_func(void * data)
{
  struct thread_starter arg = *(struct thread_starter *)data;
  JMP_BUF back;
  INT32 tmp;

  THREADS_FPRINTF(0, (stderr,"THREADS_DISALLOW() Thread %08x created...\n",
		      (unsigned int)arg.id));

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
    setegid(arg.egid);
    seteuid(arg.euid);
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
  
  mt_lock_interpreter();

  SWAP_IN_THREAD(OBJ2THREAD(arg.id)); /* Init struct */
  init_interpreter();
  Pike_interpreter.thread_id=arg.id;
#ifdef PROFILING
  Pike_interpreter.stack_bottom=((char *)&data);
#endif
  Pike_interpreter.stack_top=((char *)&data)+ (thread_stack_size-16384) * STACK_DIRECTION;
  Pike_interpreter.recoveries = NULL;
  SWAP_OUT_THREAD(OBJ2THREAD(Pike_interpreter.thread_id)); /* Init struct */
  Pike_interpreter.thread_id=arg.id;
  OBJ2THREAD(Pike_interpreter.thread_id)->swapped=0;

#if defined(PIKE_DEBUG)
  if(d_flag)
    {
      THREAD_T self = th_self();

      if( Pike_interpreter.thread_id && !th_equal( OBJ2THREAD(Pike_interpreter.thread_id)->id, self) )
	Pike_fatal("Current thread is wrong. %lx %lx\n",
	      (long)OBJ2THREAD(Pike_interpreter.thread_id)->id, (long)self);
	
      if(thread_for_id(th_self()) != Pike_interpreter.thread_id) {
	debug_list_all_threads();
	Pike_fatal("thread_for_id() (or Pike_interpreter.thread_id) failed! %p != %p\n",thread_for_id(th_self()),Pike_interpreter.thread_id) ;
      }
    }
#endif

#ifdef THREAD_TRACE
  {
    t_flag = default_t_flag;
  }
#endif /* THREAD_TRACE */

  THREADS_FPRINTF(0, (stderr,"THREAD %08x INITED\n",(unsigned int)Pike_interpreter.thread_id));

  DO_IF_DEBUG(
    if(thread_for_id(th_self()) != Pike_interpreter.thread_id) {
      debug_list_all_threads();
      Pike_fatal("thread_for_id() (or Pike_interpreter.thread_id) failed! %p != %p\n",thread_for_id(th_self()),Pike_interpreter.thread_id) ;
    } )


  if(SETJMP(back))
  {
    if(throw_severity < THROW_EXIT)
      call_handle_error();
    if(throw_severity == THROW_EXIT)
    {
      free((char *) data);
      pike_do_exit(throw_value.u.integer);
    }
  } else {
    INT32 args=arg.args->size;
    back.severity=THROW_EXIT;
    push_array_items(arg.args);
    arg.args=0;
    f_call_function(args);

    /* copy return value to the Pike_interpreter.thread_id here */
    object_low_set_index(Pike_interpreter.thread_id,
			 thread_id_result_variable,
			 Pike_sp-1);
    pop_stack();
  }

  DO_IF_DEBUG(
    if(thread_for_id(th_self()) != Pike_interpreter.thread_id) {
      debug_list_all_threads();
      Pike_fatal("thread_for_id() (or Pike_interpreter.thread_id) failed! %p != %p\n",thread_for_id(th_self()),Pike_interpreter.thread_id) ;
    } )


  if(OBJ2THREAD(Pike_interpreter.thread_id)->thread_local != NULL) {
    free_mapping(OBJ2THREAD(Pike_interpreter.thread_id)->thread_local);
    OBJ2THREAD(Pike_interpreter.thread_id)->thread_local = NULL;
  }

   OBJ2THREAD(Pike_interpreter.thread_id)->status=THREAD_EXITED;
   co_broadcast(& OBJ2THREAD(Pike_interpreter.thread_id)->status_change);

  free((char *)data); /* Moved by per, to avoid some bugs.... */
  UNSETJMP(back);

  THREADS_FPRINTF(0, (stderr,"THREADS_ALLOW() Thread %08x done\n",
		      (unsigned int)Pike_interpreter.thread_id));

  cleanup_interpret();
  DO_IF_DMALLOC({
    struct object *o = Pike_interpreter.thread_id;
    SWAP_OUT_THREAD(OBJ2THREAD(Pike_interpreter.thread_id)); /* de-Init struct */
    Pike_interpreter.thread_id = o;
    OBJ2THREAD(Pike_interpreter.thread_id)->swapped=0;
  })
  thread_table_delete(Pike_interpreter.thread_id);
  free_object(Pike_interpreter.thread_id);
  Pike_interpreter.thread_id=0;
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
  return(0);
}

#ifdef UNIX_THREADS
int num_lwps = 1;
#endif

/*! @class Thread
 */

/*! @decl void create(function(mixed...:void) f, mixed ... args)
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
  struct thread_starter *arg;
  int tmp;
  arg = ALLOC_STRUCT(thread_starter);
  arg->args=aggregate_array(args);
  arg->id=clone_object(thread_id_prog,0);
  OBJ2THREAD(arg->id)->status=THREAD_RUNNING;

#ifdef HAVE_BROKEN_LINUX_THREAD_EUID
  arg->euid = geteuid();
  arg->egid = getegid();
#endif /* HAVE_BROKEN_LINUX_THREAD_EUID */

  do {
    tmp = th_create(& OBJ2THREAD(arg->id)->id,
		    new_thread_func,
		    arg);
    if (tmp == EINTR) check_threads_etc();
  } while( tmp == EINTR );

  if(!tmp)
  {
    num_threads++;
    thread_table_insert(arg->id);

    if(!threads_evaluator_callback)
    {
      threads_evaluator_callback=add_to_callback(&evaluator_callbacks,
						 check_threads, 0,0);
      dmalloc_accept_leak(threads_evaluator_callback);
    }
    ref_push_object(arg->id);
    THREADS_FPRINTF(0, (stderr, "THREAD_CREATE -> t:%08x\n",
			(unsigned int)arg->id));
  } else {
    free_object(arg->id);
    free_array(arg->args);
    free((char *)arg);
    Pike_error("Failed to create thread (errno = %d).\n",tmp);
  }
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
  if(args) c=Pike_sp[-args].u.integer;
  else Pike_error("No argument to thread_set_concurrency(int concurrency);\n");
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
  ref_push_object(Pike_interpreter.thread_id);
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
  struct object *owner;
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
 *! @seealso
 *!   @[trylock()]
 */
void f_mutex_lock(INT32 args)
{
  struct mutex_storage  *m;
  struct object *o;
  INT_TYPE type;

  DO_IF_DEBUG(
    if(thread_for_id(th_self()) != Pike_interpreter.thread_id) {
      debug_list_all_threads();
      Pike_fatal("thread_for_id() (or Pike_interpreter.thread_id) failed! %p != %p\n",thread_for_id(th_self()),Pike_interpreter.thread_id) ;
    } )

  m=THIS_MUTEX;
  if(!args)
    type=0;
  else
    get_all_args("mutex->lock",args,"%i",&type);

  switch(type)
  {
    default:
      bad_arg_error("mutex->lock", Pike_sp-args, args, 2, "int(0..2)", Pike_sp+1-args,
		  "Unknown mutex locking style: %d\n",type);
      

    case 0:
    case 2:
      if(m->key && OB2KEY(m->key)->owner == Pike_interpreter.thread_id)
      {
	THREADS_FPRINTF(0,
			(stderr, "Recursive LOCK k:%08x, m:%08x(%08x), t:%08x\n",
			 (unsigned int)OB2KEY(m->key),
			 (unsigned int)m,
			 (unsigned int)OB2KEY(m->key)->mut,
			 (unsigned int) Pike_interpreter.thread_id));

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
  o=fast_clone_object(mutex_key,0);

  DO_IF_DEBUG(
    if(thread_for_id(th_self()) != Pike_interpreter.thread_id) {
      debug_list_all_threads();
      Pike_fatal("thread_for_id() (or Pike_interpreter.thread_id) failed! %p != %p\n",thread_for_id(th_self()),Pike_interpreter.thread_id) ;
    } )

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
      THREADS_FPRINTF(1, (stderr,"WAITING TO LOCK m:%08x\n",(unsigned int)m));
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
    if (!m->num_waiting)
      co_destroy (&m->condition);
    Pike_error ("Mutex was destructed while waiting for lock.\n");
  }
#endif

  m->key=o;
  OB2KEY(o)->mut=m;
  add_ref (OB2KEY(o)->mutex_obj = Pike_fp->current_object);

  DO_IF_DEBUG(
    if(thread_for_id(th_self()) != Pike_interpreter.thread_id) {
      debug_list_all_threads();
      Pike_fatal("thread_for_id() (or Pike_interpreter.thread_id) failed! %p != %p\n",thread_for_id(th_self()),Pike_interpreter.thread_id) ;
    } )

  THREADS_FPRINTF(1, (stderr, "LOCK k:%08x, m:%08x(%08x), t:%08x\n",
		      (unsigned int)OB2KEY(o),
		      (unsigned int)m,
		      (unsigned int)OB2KEY(m->key)->mut,
		      (unsigned int)Pike_interpreter.thread_id));
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
    get_all_args("mutex->trylock",args,"%i",&type);

  switch(type)
  {
    default:
      bad_arg_error("mutex->trylock", Pike_sp-args, args, 2, "int(0..2)", Pike_sp+1-args,
		  "Unknown mutex locking style: %d\n",type);

    case 0:
      if(m->key && OB2KEY(m->key)->owner == Pike_interpreter.thread_id)
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
    ref_push_object(OB2KEY(m->key)->owner);
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

void init_mutex_obj(struct object *o)
{
  co_init(& THIS_MUTEX->condition);
  THIS_MUTEX->key=0;
  THIS_MUTEX->num_waiting = 0;
}

void exit_mutex_obj(struct object *o)
{
  struct mutex_storage *m = THIS_MUTEX;
  struct object *key = m->key;

  THREADS_FPRINTF(1, (stderr, "DESTROYING MUTEX m:%08x\n",
		      (unsigned int)m));

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
      /* exit_mutex_key_obj has already signalled, but since the
       * waiting threads will throw an error instead of making a new
       * lock we need to double it to a broadcast. The last thread
       * that stops waiting will destroy m->condition. */
      co_broadcast (&m->condition);
    }
  }
  else
    co_destroy(& m->condition);
#endif
}

/*! @endclass
 */

#define THIS_KEY ((struct key_storage *)(CURRENT_STORAGE))
void init_mutex_key_obj(struct object *o)
{
  THREADS_FPRINTF(1, (stderr, "KEY k:%08x, o:%08x\n",
		      (unsigned int)THIS_KEY, (unsigned int)Pike_interpreter.thread_id));
  THIS_KEY->mut=0;
  THIS_KEY->mutex_obj = NULL;
  add_ref(THIS_KEY->owner=Pike_interpreter.thread_id);
  THIS_KEY->initialized=1;
}

void exit_mutex_key_obj(struct object *o)
{
  THREADS_FPRINTF(1, (stderr, "UNLOCK k:%08x m:(%08x) t:%08x o:%08x\n",
		      (unsigned int)THIS_KEY,
		      (unsigned int)THIS_KEY->mut,
		      (unsigned int)Pike_interpreter.thread_id,
		      (unsigned int)THIS_KEY->owner));
  if(THIS_KEY->mut)
  {
    struct mutex_storage *mut = THIS_KEY->mut;
    struct object *mutex_obj;

#ifdef PIKE_DEBUG
    /* Note: mut->key can be NULL if our corresponding mutex
     *       has been destructed.
     */
    if(mut->key && (mut->key != o))
      Pike_fatal("Mutex unlock from wrong key %p != %p!\n", mut->key, o);
#endif
    mut->key=0;
    if (THIS_KEY->owner) {
      free_object(THIS_KEY->owner);
      THIS_KEY->owner=0;
    }
    THIS_KEY->mut=0;
    THIS_KEY->initialized=0;
    mutex_obj = THIS_KEY->mutex_obj;
    THIS_KEY->mutex_obj = NULL;
    if (mut->num_waiting)
      co_signal(&mut->condition);
    else if (!mutex_obj->prog)
      co_destroy (&mut->condition);
    free_object(mutex_obj);
  }
}

#define THIS_COND ((COND_T *)(CURRENT_STORAGE))

/*! @class Condition
 *!
 *! Implementation of condition variables.
 *!
 *! Condition variables are used by threaded programs
 *! to wait for events happening in other threads.
 *!
 *! @note
 *!   Condition variables are only available on systems with thread
 *!   support. The Condition class is not simulated otherwise, since that
 *!   can't be done accurately without continuations.
 *!
 *! @seealso
 *!   @[Mutex]
 */

/*! @decl void wait(Thread.MutexKey mutex_key)
 *!
 *! Wait for contition.
 *!
 *! This function makes the current thread sleep until the condition
 *! variable is signalled. The argument should be a @[Thread.MutexKey]
 *! object for a @[Thread.Mutex]. It will be unlocked atomically
 *! before waiting for the signal and then relocked atomically when
 *! the signal is received.
 *!
 *! The thread that sends the signal should have the mutex locked
 *! while sending it. Otherwise it's impossible to avoid races where
 *! signals are sent while the listener(s) haven't arrived to the
 *! @[wait] calls yet.
 *!
 *! @note
 *!   In Pike 7.2 and earlier it was possible to call @[wait()]
 *!   without arguments. This possibility was removed in later
 *!   versions since it unavoidably leads to programs with races
 *!   and/or deadlocks.
 *!
 *! @seealso
 *!   @[Mutex->lock()]
 */
void f_cond_wait(INT32 args)
{
  struct object *key, *mutex_obj;
  struct mutex_storage *mut;
  COND_T *c;

  if(threads_disabled)
    Pike_error("Cannot wait for conditions when threads are disabled!\n");

  get_all_args("condition->wait", args, "%o", &key);
      
  if ((key->prog != mutex_key) ||
      (!(OB2KEY(key)->initialized)) ||
      (!(mut = OB2KEY(key)->mut))) {
    Pike_error("Bad argument 1 to condition->wait()\n");
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
  co_wait_interpreter(c);
  SWAP_IN_CURRENT_THREAD();
    
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

#ifdef PICKY_MUTEX
  if (!mutex_obj->prog) {
    if (!mut->num_waiting)
      co_destroy (&mut->condition);
    Pike_error ("Mutex was destructed while waiting for lock.\n");
  }
#endif
      
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
void f_cond_signal(INT32 args) { pop_n_elems(args); co_signal(THIS_COND); }

/*! @decl void broadcast()
 *!
 *! @[broadcast()] wakes up all threads currently waiting for this condition.
 *!
 *! @seealso
 *!   @[signal()]
 */
void f_cond_broadcast(INT32 args) { pop_n_elems(args); co_broadcast(THIS_COND); }

void init_cond_obj(struct object *o) { co_init(THIS_COND); }
void exit_cond_obj(struct object *o) { co_destroy(THIS_COND); }

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
  void low_backtrace(struct Pike_interpreter *);
  struct thread_state *foo = THIS_THREAD;

  pop_n_elems(args);

  if(foo->state.thread_id == Pike_interpreter.thread_id)
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
 */
void f_thread_id_status(INT32 args)
{
  pop_n_elems(args);
  push_int(THIS_THREAD->status);
}

/*! @decl static string _sprintf(int c)
 *!
 *! Returns a string identifying the thread.
 */
void f_thread_id__sprintf (INT32 args)
{
  int c = 0;
  if(args>0 && Pike_sp[-args].type == PIKE_T_INT)
    c = Pike_sp[-args].u.integer;
  pop_n_elems (args);
  if(c != 'O') {
    push_undefined();
    return;
  }
  push_constant_text ("Thread.Thread(");
  push_int64((ptrdiff_t)THIS_THREAD->id);
  push_constant_text (")");
  f_add (3);
}

/*! @decl static int id_number()
 *!
 *! Returns an id number identifying the thread.
 *!
 *! @note
 *!   This function was added in Pike 7.2.204.
 */
void f_thread_id_id_number(INT32 args)
{
  pop_n_elems(args);
  push_int64((ptrdiff_t)THIS_THREAD->id);
}

/*! @decl mixed result()
 *!
 *! Waits for the thread to complete, and then returns
 *! the value returned from the thread function.
 */
static void f_thread_id_result(INT32 args)
{
  struct thread_state *th=THIS_THREAD;

  if (threads_disabled) {
    Pike_error("Cannot wait for threads when threads are disabled!\n");
  }

  while(th->status != THREAD_EXITED) {
    SWAP_OUT_CURRENT_THREAD();
    co_wait_interpreter(&th->status_change);
    SWAP_IN_CURRENT_THREAD();
    check_threads_etc();
  }

  low_object_index_no_free(Pike_sp,
			   Pike_fp->current_object, 
			   thread_id_result_variable);
  Pike_sp++;
}

void init_thread_obj(struct object *o)
{
  MEMSET(&THIS_THREAD->state, 0, sizeof(struct Pike_interpreter));
  THIS_THREAD->swapped = 0;
  THIS_THREAD->status=THREAD_NOT_STARTED;
  co_init(& THIS_THREAD->status_change);
  THIS_THREAD->thread_local=NULL;
}


void exit_thread_obj(struct object *o)
{
  if(THIS_THREAD->thread_local != NULL) {
    free_mapping(THIS_THREAD->thread_local);
    THIS_THREAD->thread_local = NULL;
  }
  co_destroy(& THIS_THREAD->status_change);
  th_destroy(& THIS_THREAD->id);
}

/*! @endclass
 */

static void thread_was_recursed(struct object *o)
{
  struct thread_state *tmp=THIS_THREAD;
  if(tmp->thread_local != NULL)
    gc_recurse_mapping(tmp->thread_local);
}

static void thread_was_checked(struct object *o)
{
  struct thread_state *tmp=THIS_THREAD;
  if(tmp->thread_local != NULL)
    debug_gc_check2(tmp->thread_local, T_OBJECT, o,
		    " as mapping for thread local values in thread");

#ifdef PIKE_DEBUG
  if(tmp->swapped)
  {
    struct pike_frame *f;
    debug_malloc_touch(o);
    debug_gc_xmark_svalues(tmp->state.evaluator_stack,
			   tmp->state.stack_pointer-tmp->state.evaluator_stack-1,
			   " in idle thread stack");
    
    for(f=tmp->state.frame_pointer;f;f=f->next)
    {
      debug_malloc_touch(f);
      if(f->context.parent)
	gc_external_mark2(f->context.parent,0," in Pike_fp->context.parent of idle thread");
      gc_external_mark2(f->current_object,0," in Pike_fp->current_object of idle thread");
      gc_external_mark2(f->context.prog,0," in Pike_fp->context.prog of idle thread");
    }
  }
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
  ((struct thread_local *)CURRENT_STORAGE)->id =
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
  key.u.integer = ((struct thread_local *)CURRENT_STORAGE)->id;
  key.type = T_INT;
  key.subtype = NUMBER_NUMBER;
  pop_n_elems(args);
  if(Pike_interpreter.thread_id != NULL &&
     (m = OBJ2THREAD(Pike_interpreter.thread_id)->thread_local) != NULL)
    mapping_index_no_free(Pike_sp++, m, &key);
  else {
    push_int(0);
    Pike_sp[-1].subtype=NUMBER_UNDEFINED;
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
  key.u.integer = ((struct thread_local *)CURRENT_STORAGE)->id;
  key.type = T_INT;
  key.subtype = NUMBER_NUMBER;
  if(args>1)
    pop_n_elems(args-1);
  else if(args<1)
    Pike_error("Too few arguments to Thread.Local.set()\n");

  if(Pike_interpreter.thread_id == NULL)
    Pike_error("Trying to set Thread.Local without thread!\n");

  if((m = OBJ2THREAD(Pike_interpreter.thread_id)->thread_local) == NULL)
    m = OBJ2THREAD(Pike_interpreter.thread_id)->thread_local =
      allocate_mapping(4);

  mapping_insert(m, &key, &Pike_sp[-1]);
}

#ifdef PIKE_DEBUG
void gc_check_thread_local (struct object *o)
{
  /* Only used by with locate_references. */
  if (Pike_in_gc == GC_PASS_LOCATE) {
    struct svalue key, *val;
    INT32 x;
    struct thread_state *s;

    key.u.integer = ((struct thread_local *)CURRENT_STORAGE)->id;
    key.type = T_INT;
    key.subtype = NUMBER_NUMBER;

    /* Hmm, should this be used here? We know we always got the
     * interpreter lock. */
    /* mt_lock( & thread_table_lock ); */
    for(x=0; x<THREAD_TABLE_SIZE; x++)
      for(s=thread_table_chains[x]; s; s=s->hashlink) {
	if (s->thread_local &&
	    (val = low_mapping_lookup(s->thread_local, &key)))
	  debug_gc_check_svalues2(val, 1, T_OBJECT, o,
				  " as thread local value in Thread.Local object"
				  " (indirect ref)");
      }
    /* mt_unlock( & thread_table_lock ); */
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
    setegid(me->egid);
    seteuid(me->euid);
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
  if(!fun) Pike_fatal("The farmers don't known how to handle empty fields\n");
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
#ifdef SGI_SPROC_THREADS
#error /* Need to specify a filename */
  us_cookie = usinit("");
#endif /* SGI_SPROC_THREADS */

  THREADS_FPRINTF(0, (stderr, "THREADS_DISALLOW() Initializing threads.\n"));

#ifdef POSIX_THREADS
#ifdef HAVE_PTHREAD_INIT
  pthread_init();
#endif /* HAVE_PTHREAD_INIT */
#endif /* POSIX_THREADS */

  mt_init( & interpreter_lock);
  low_mt_lock_interpreter();
  mt_init( & thread_table_lock);
  mt_init( & interleave_lock);
  mt_init( & rosie);
  co_init( & live_threads_change);
  co_init( & threads_disabled_change);

  thread_table_init();
#ifdef POSIX_THREADS
  pthread_attr_init(&pattr);
#ifdef HAVE_PTHREAD_ATTR_SETSTACKSIZE
  pthread_attr_setstacksize(&pattr, thread_stack_size);
#endif
  pthread_attr_setdetachstate(&pattr, PTHREAD_CREATE_DETACHED);

  pthread_attr_init(&small_pattr);
#ifdef HAVE_PTHREAD_ATTR_SETSTACKSIZE
  pthread_attr_setstacksize(&small_pattr, 4096*sizeof(char *));
#endif
  pthread_attr_setdetachstate(&small_pattr, PTHREAD_CREATE_DETACHED);

  th_running = 1;
#endif
}

void th_init(void)
{
  ptrdiff_t mutex_key_offset;

#ifdef UNIX_THREADS
  
/* function(int:void) */
  ADD_EFUN("thread_set_concurrency",f_thread_set_concurrency,tFunc(tInt,tVoid), OPT_SIDE_EFFECT);
#endif

  START_NEW_PROGRAM_ID(THREAD_MUTEX_KEY);
  mutex_key_offset = ADD_STORAGE(struct key_storage);
  /* This is needed to allow the gc to find the possible circular reference.
   * It also allows a thread to take over ownership of a key.
   */
  PIKE_MAP_VARIABLE("_owner", mutex_key_offset + OFFSETOF(key_storage, owner),
		    tObjIs_THREAD_ID, T_OBJECT, 0);
  PIKE_MAP_VARIABLE("_mutex", mutex_key_offset + OFFSETOF(key_storage, mutex_obj),
		    tObjIs_THREAD_MUTEX, T_OBJECT, ID_STATIC|ID_PRIVATE);
  set_init_callback(init_mutex_key_obj);
  set_exit_callback(exit_mutex_key_obj);
  mutex_key=Pike_compiler->new_program;
  add_ref(mutex_key);
  end_class("mutex_key", 0);
  mutex_key->flags|=PROGRAM_DESTRUCT_IMMEDIATE;
#ifdef PIKE_DEBUG
  if(!mutex_key)
    Pike_fatal("Failed to initialize mutex_key program!\n");
#endif

  START_NEW_PROGRAM_ID(THREAD_MUTEX);
  ADD_STORAGE(struct mutex_storage);
  /* function(int(0..2)|void:object(mutex_key)) */
  ADD_FUNCTION("lock",f_mutex_lock,
	       tFunc(tOr(tInt02,tVoid),tObjIs_THREAD_MUTEX_KEY),0);
  /* function(int(0..2)|void:object(mutex_key)) */
  ADD_FUNCTION("trylock",f_mutex_trylock,
	       tFunc(tOr(tInt02,tVoid),tObjIs_THREAD_MUTEX_KEY),0);
  /* function(:object(Pike_interpreter.thread_id)) */
  ADD_FUNCTION("current_locking_thread",f_mutex_locking_thread,
	   tFunc(tNone,tObjIs_THREAD_ID), 0);
  /* function(:object(Pike_interpreter.thread_id)) */
  ADD_FUNCTION("current_locking_key",f_mutex_locking_key,
	   tFunc(tNone,tObjIs_THREAD_MUTEX_KEY), 0);
  set_init_callback(init_mutex_obj);
  set_exit_callback(exit_mutex_obj);
  end_class("mutex", 0);

  START_NEW_PROGRAM_ID(THREAD_CONDITION);
  ADD_STORAGE(COND_T);
  /* function(object(mutex_key):void) */
  ADD_FUNCTION("wait",f_cond_wait,
	       tFunc(tObjIs_THREAD_MUTEX_KEY,tVoid),0);
  /* function(:void) */
  ADD_FUNCTION("signal",f_cond_signal,tFunc(tNone,tVoid),0);
  /* function(:void) */
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
  ADD_STORAGE(struct thread_local);
  ADD_FUNCTION("get",f_thread_local_get,tFunc(tNone,tMix),0);
  ADD_FUNCTION("set",f_thread_local_set,tFunc(tSetvar(1,tMix),tVar(1)),0);
  ADD_FUNCTION("create", f_thread_local_create,
	       tFunc(tVoid,tVoid), ID_STATIC);
#ifdef PIKE_DEBUG
  set_gc_check_callback(gc_check_thread_local);
#endif
  thread_local_prog=Pike_compiler->new_program;
  add_ref(thread_local_prog);
  end_class("thread_local", 0);
  if(!thread_local_prog)
    Pike_fatal("Failed to initialize thread_local program!\n");
  ADD_EFUN("thread_local", f_thread_local,
	   tFunc(tNone,tObjIs_THREAD_LOCAL),
	   OPT_EXTERNAL_DEPEND);

  START_NEW_PROGRAM_ID(THREAD_ID);
  thread_storage_offset=ADD_STORAGE(struct thread_state);
  thread_id_result_variable=simple_add_variable("result","mixed",0);
  /* function(:array) */
  ADD_FUNCTION("backtrace",f_thread_backtrace,tFunc(tNone,tArray),0);
  /* function(:mixed) */
  ADD_FUNCTION("wait",f_thread_id_result,tFunc(tNone,tMix),0);
  /* function(:int) */
  ADD_FUNCTION("status",f_thread_id_status,tFunc(tNone,tInt),0);
  ADD_FUNCTION("_sprintf",f_thread_id__sprintf,tFunc(tNone,tStr),0);
  ADD_FUNCTION("id_number",f_thread_id_id_number,tFunc(tNone,tInt),0);
  set_gc_recurse_callback(thread_was_recursed);
  set_gc_check_callback(thread_was_checked);
  set_init_callback(init_thread_obj);
  set_exit_callback(exit_thread_obj);
  thread_id_prog=Pike_compiler->new_program;
  thread_id_prog->flags |= PROGRAM_NO_EXPLICIT_DESTRUCT;
  add_ref(thread_id_prog);
  end_class("thread_id", 0);

  /* function(mixed ...:object(Pike_interpreter.thread_id)) */
  ADD_EFUN("thread_create",f_thread_create,
	   tFuncV(tNone,tMixed,tObjIs_THREAD_ID),
	   OPT_SIDE_EFFECT);

  /* function(:object(Pike_interpreter.thread_id)) */
  ADD_EFUN("this_thread",f_this_thread,
	   tFunc(tNone,tObjIs_THREAD_ID),
	   OPT_EXTERNAL_DEPEND);

  /* function(:array(object(Pike_interpreter.thread_id))) */
  ADD_EFUN("all_threads",f_all_threads,
	   tFunc(tNone,tArr(tObjIs_THREAD_ID)),
	   OPT_EXTERNAL_DEPEND);

  /* Some constants... */
  add_integer_constant("THREAD_NOT_STARTED", THREAD_NOT_STARTED, 0);
  add_integer_constant("THREAD_RUNNING", THREAD_RUNNING, 0);
  add_integer_constant("THREAD_EXITED", THREAD_EXITED, 0);

  if(!mutex_key)
    Pike_fatal("Failed to initialize thread program!\n");

  {
    struct object *o = Pike_interpreter.thread_id =
      clone_object(thread_id_prog,0);
    SWAP_OUT_THREAD(OBJ2THREAD(Pike_interpreter.thread_id)); /* Init struct */
    Pike_interpreter.thread_id = o;
    OBJ2THREAD(Pike_interpreter.thread_id)->id=th_self();
    OBJ2THREAD(Pike_interpreter.thread_id)->swapped=0;
    thread_table_insert(Pike_interpreter.thread_id);
  }
}

void th_cleanup(void)
{
  th_running = 0;

  if(Pike_interpreter.thread_id)
  {
    thread_table_delete(Pike_interpreter.thread_id);
    destruct(Pike_interpreter.thread_id);
    free_object(Pike_interpreter.thread_id);
    Pike_interpreter.thread_id=0;
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

#endif
