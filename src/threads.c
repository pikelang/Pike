#include "global.h"
RCSID("$Id: threads.c,v 1.176 2004/03/09 14:12:58 grubba Exp $");

PMOD_EXPORT int num_threads = 1;
PMOD_EXPORT int threads_disabled = 0;

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

#include <errno.h>

#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif /* HAVE_SYS_PRCTL_H */

PMOD_EXPORT int live_threads = 0;
PMOD_EXPORT COND_T live_threads_change;
PMOD_EXPORT COND_T threads_disabled_change;
PMOD_EXPORT size_t thread_stack_size=1024 * 1204;
 
PMOD_EXPORT void thread_low_error (int errcode, const char *cmd,
                                   const char *fname, int lineno)
{
  fatal ("%s:%d: %s\n"
	 "Unexpected error from thread function: %d\n",
	 fname, lineno, cmd, errcode);
}

#ifdef USE_CLOCK_FOR_SLICES
PMOD_EXPORT clock_t thread_start_clock = 0;
#endif

#ifndef HAVE_PTHREAD_ATFORK
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
static int IsValidHandle(HANDLE h)
{
  __try {
    HANDLE ret;
    if(DuplicateHandle(GetCurrentProcess(),
			h,
			GetCurrentProcess(),
			&ret,
			0,
			0,
			DUPLICATE_SAME_ACCESS))
    {
      CloseHandle(ret);
    }
  }

  __except (1) {
    return 0;
  }

  return 1;
}

PMOD_EXPORT HANDLE CheckValidHandle(HANDLE h)
{
  if(!IsValidHandle(h))
    fatal("Invalid handle!\n");
  return h;
}

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
    fatal("Wait on event return prematurely!!\n");
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
  } else {
    threads_disabled++;
  }

  THREADS_FPRINTF(0,
		  (stderr, "low_init_threads_disable(): threads_disabled:%d\n",
		   threads_disabled));
}

void init_threads_disable(struct object *o)
{
  low_init_threads_disable();

  if(live_threads) {
    SWAP_OUT_CURRENT_THREAD();
    while (live_threads) {
      THREADS_FPRINTF(0,
		      (stderr,
		       "_disable_threads(): Waiting for %d threads to finish\n",
		       live_threads));
      co_wait_interpreter(&live_threads_change);
    }
    SWAP_IN_CURRENT_THREAD();
  }
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
			 "exit_threads_disable(): Unlocking IM 0x%08p\n", im));
	mt_unlock(&(im->lock));
	im = im->next;
      }

      mt_unlock(&interleave_lock);

      THREADS_FPRINTF(0, (stderr, "_exit_threads_disable(): Wake up!\n"));
      co_broadcast(&threads_disabled_change);
    }
#ifdef PIKE_DEBUG
  } else {
    fatal("exit_threads_disable() called too many times!\n");
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
		      "init_interleave_mutex(): Locking IM 0x%08p\n", im));

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
static void dumpmem(char *desc, void *x, int size)
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
    fatal("thread_table_hash failed miserably!\n");
  if(thread_state_for_id(s->id))
  {
    if(thread_state_for_id(s->id) == s)
      fatal("Registring thread twice!\n");
    else
      fatal("Forgot to unregister thread!\n");
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
    fatal("thread_table_hash failed miserably!\n");
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

PMOD_EXPORT struct object *thread_for_id(THREAD_T tid)
{
  struct thread_state *s = thread_state_for_id(tid);
  return (s == NULL? NULL : THREADSTATE2OBJ(s));
  /* See NB in thread_state_for_id.  Lifespan of result can be prolonged
     by incrementing refcount though. */
}


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
    static hrtime_t last_;
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
#elif defined(USE_CLOCK_FOR_SLICES)
  if (clock() - thread_start_clock < (clock_t) (CLOCKS_PER_SEC / 20))
    return;
#endif
#endif

#ifdef DEBUG
  if(thread_for_id(th_self()) != Pike_interpreter.thread_id)
    fatal("thread_for_id() (or Pike_interpreter.thread_id) failed!\n")

  if(Pike_interpreter.backlink != OBJ2THREAD(Pike_interpreter.thread_id))
    fatal("Hashlink is wrong!\n");
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

#ifdef DEBUG
  if(thread_for_id(th_self()) != Pike_interpreter.thread_id)
    fatal("thread_for_id() (or Pike_interpreter.thread_id) failed!\n")
#endif
}

TH_RETURN_TYPE new_thread_func(void * data)
{
  struct thread_starter arg = *(struct thread_starter *)data;
  JMP_BUF back;
  INT32 tmp;

  THREADS_FPRINTF(0, (stderr,"new_thread_func(): Thread %08x created...\n",
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

  while (threads_disabled) {
    THREADS_FPRINTF(1, (stderr,
			"new_thread_func(): Threads disabled\n"));
    co_wait_interpreter(&threads_disabled_change);
  }

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
	fatal("Current thread is wrong. %lx %lx\n",
	      (long)OBJ2THREAD(Pike_interpreter.thread_id)->id, (long)self);
	
      if(thread_for_id(th_self()) != Pike_interpreter.thread_id)
	fatal("thread_for_id() (or Pike_interpreter.thread_id) failed in new_thread_func! "
	      "%p != %p\n", thread_for_id(self), Pike_interpreter.thread_id);
    }
#endif

#ifdef THREAD_TRACE
  {
    t_flag = default_t_flag;
  }
#endif /* THREAD_TRACE */

  THREADS_FPRINTF(0, (stderr,"THREAD %08x INITED\n",(unsigned int)Pike_interpreter.thread_id));
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

    /* FIXME: Check threads_disable. */

    /* copy return value to the Pike_interpreter.thread_id here */
    object_low_set_index(Pike_interpreter.thread_id,
			 thread_id_result_variable,
			 Pike_sp-1);
    pop_stack();
  }

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
  /* FIXME: What about threads_disable? */
  mt_unlock_interpreter();
  th_exit(0);
  /* NOT_REACHED, but removes a warning */
  return(0);
}

#ifdef UNIX_THREADS
int num_lwps = 1;
#endif

void f_thread_create(INT32 args)
{
  THREAD_T dummy;
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

#ifdef UNIX_THREADS
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
  /* Key-related data. */
  struct object *owner;	/* NULL: Unlocked. other: locked with owner. */
};

struct key_storage
{
  struct mutex_storage *mut; /* Only valid if mutex_obj is not destructed. */
  struct object *mutex_obj;
};

#define OB2KEY(X) ((struct key_storage *)((X)->storage))

void f_mutex_lock(INT32 args)
{
  struct mutex_storage *m;
  struct object *o;
  INT_TYPE type;

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
      if(m->owner == Pike_interpreter.thread_id)
      {
	THREADS_FPRINTF(0,
			(stderr, "Recursive LOCK m:%08x, t:%08x(%08x)\n",
			 (unsigned int)m,
			 (unsigned int)Pike_interpreter.thread_id,
			 (unsigned int)m->owner));

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
  o=clone_object(mutex_key,0);

  DO_IF_DEBUG( if(thread_for_id(th_self()) != Pike_interpreter.thread_id)
	       fatal("thread_for_id() (or Pike_interpreter.thread_id) failed! %p != %p\n",thread_for_id(th_self()),Pike_interpreter.thread_id) ; )

  if(m->owner)
  {
    if(threads_disabled)
    {
      free_object(o);
      Pike_error("Cannot wait for mutexes when threads are disabled!\n");
    }
    SWAP_OUT_CURRENT_THREAD();
    do
    {
      THREADS_FPRINTF(1, (stderr,"WAITING TO LOCK m:%08x\n",(unsigned int)m));
      co_wait_interpreter(& m->condition);
      while (threads_disabled) {
	THREADS_FPRINTF(1, (stderr,
			    "f_mutex_lock(): Threads disabled\n"));
	co_wait_interpreter(&threads_disabled_change);
      }
    }while(m->owner);
    SWAP_IN_CURRENT_THREAD();
  }
  add_ref(m->owner = Pike_interpreter.thread_id);	/* Locked. */
  add_ref(OB2KEY(o)->mutex_obj = Pike_fp->current_object);
  OB2KEY(o)->mut = m;

  DO_IF_DEBUG( if(thread_for_id(th_self()) != Pike_interpreter.thread_id)
	       fatal("thread_for_id() (or Pike_interpreter.thread_id) failed! %p != %p\n",thread_for_id(th_self()),Pike_interpreter.thread_id) ; )

  THREADS_FPRINTF(1, (stderr, "LOCK k:%08x, m:%08x, t:%08x\n",
		      (unsigned int)OB2KEY(o),
		      (unsigned int)m,
		      (unsigned int)Pike_interpreter.thread_id));
  pop_n_elems(args);
  push_object(o);
}

void f_mutex_trylock(INT32 args)
{
  struct mutex_storage *m;
  struct object *o;
  INT_TYPE type;

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
      if(m->owner == Pike_interpreter.thread_id)
      {
	Pike_error("Recursive mutex locks!\n");
      }

    case 2:
    case 1:
      break;
  }

  o=clone_object(mutex_key,0);

  if(!m->owner)
  {
    add_ref(m->owner = Pike_interpreter.thread_id);
    add_ref(OB2KEY(o)->mutex_obj = Pike_fp->current_object);
    OB2KEY(o)->mut=m;
    pop_n_elems(args);
    push_object(o);
  } else {
    pop_n_elems(args);
    destruct(o);
    free_object(o);
    push_int(0);
  }
}

void init_mutex_obj(struct object *o)
{
  co_init(& THIS_MUTEX->condition);
  THIS_MUTEX->owner = NULL;
}

void exit_mutex_obj(struct object *o)
{
  THREADS_FPRINTF(1, (stderr, "DESTROYING MUTEX m:%08x\n",
		      (unsigned int)THIS_MUTEX));
  if(THIS_MUTEX->owner) {
    free_object(THIS_MUTEX->owner);
  }
  THIS_MUTEX->owner = NULL;
  co_destroy(& THIS_MUTEX->condition);
}

#define THIS_KEY ((struct key_storage *)(CURRENT_STORAGE))
void init_mutex_key_obj(struct object *o)
{
  THREADS_FPRINTF(1, (stderr, "KEY k:%08x, o:%08x\n",
		      (unsigned int)THIS_KEY,
		      (unsigned int)Pike_interpreter.thread_id));
  THIS_KEY->mut = NULL;
  THIS_KEY->mutex_obj = NULL;
}

void exit_mutex_key_obj(struct object *o)
{
  struct mutex_storage *mut;

  THREADS_FPRINTF(1, (stderr, "UNLOCK k:%08x m:(%08x) t:%08x\n",
		      (unsigned int)THIS_KEY,
		      (unsigned int)THIS_KEY->mut,
		      (unsigned int)Pike_interpreter.thread_id));

  if((mut = THIS_KEY->mut) &&
     // Don't look at mut if the mutex object has been destructed.
     (THIS_KEY->mutex_obj) && (THIS_KEY->mutex_obj->prog))
  {
    struct object *owner = mut->owner;

    THIS_KEY->mut = NULL;
    mut->owner = NULL;

    co_signal(& mut->condition);

    if (owner) {
      free_object(mut->owner);
    }
  }
  if (THIS_KEY->mutex_obj) {
    free_object(THIS_KEY->mutex_obj);
    THIS_KEY->mutex_obj = NULL;
  }
}

#define THIS_COND ((COND_T *)(CURRENT_STORAGE))
void f_cond_wait(INT32 args)
{
  COND_T *c;

  if(threads_disabled)
    Pike_error("Cannot wait for conditions when threads are disabled!\n");
      
  if(args > 1) {
    pop_n_elems(args - 1);
    args = 1;
  }

  c=THIS_COND;

  if((args > 0) && !IS_ZERO(Pike_sp-1))
  {
    struct object *key;
    struct object *owner;
    struct mutex_storage *mut;

    if(Pike_sp[-1].type != T_OBJECT)
      Pike_error("Bad argument 1 to condition->wait()\n");
    
    key = Pike_sp[-1].u.object;
    
    if(key->prog != mutex_key)
      Pike_error("Bad argument 1 to condition->wait()\n");

    if ((mut = OB2KEY(key)->mut) && (owner = mut->owner)) {
      /* Unlock mutex */
      mut->owner = NULL;
      OB2KEY(key)->mut = NULL;
      co_signal(& mut->condition);
    
      /* Wait and allow mutex operations */
      SWAP_OUT_CURRENT_THREAD();
      co_wait_interpreter(c);
      while (threads_disabled) {
	THREADS_FPRINTF(1, (stderr,
			    "f_cond_wait(): Threads disabled\n"));
	co_wait_interpreter(&threads_disabled_change);
      }
    
      /* Lock mutex */
      while(mut->owner) {
	co_wait_interpreter(& mut->condition);
	while (threads_disabled) {
	  THREADS_FPRINTF(1, (stderr,
			      "f_cond_wait(): Threads disabled\n"));
	  co_wait_interpreter(&threads_disabled_change);
	}
      }
      mut->owner = owner;
      OB2KEY(key)->mut = mut;
      
      SWAP_IN_CURRENT_THREAD();
      pop_n_elems(args);
      return;
    }
    else if (mut) {
      Pike_error("Bad argument 1 to condition->wait()\n");
    } else {
      Pike_error("Bad argument 1 to condition->wait(): Mutex not locked.\n");
    }
  }

  SWAP_OUT_CURRENT_THREAD();
  co_wait_interpreter(c);
  while (threads_disabled) {
    THREADS_FPRINTF(1, (stderr,
			"f_cond_wait(): Threads disabled\n"));
    co_wait_interpreter(&threads_disabled_change);
  }
  SWAP_IN_CURRENT_THREAD();

  pop_n_elems(args);
}

void f_cond_signal(INT32 args) { pop_n_elems(args); co_signal(THIS_COND); }
void f_cond_broadcast(INT32 args) { pop_n_elems(args); co_broadcast(THIS_COND); }
void init_cond_obj(struct object *o) { co_init(THIS_COND); }
void exit_cond_obj(struct object *o) { co_destroy(THIS_COND); }

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

void f_thread_id_status(INT32 args)
{
  pop_n_elems(args);
  push_int(THIS_THREAD->status);
}

void f_thread_id__sprintf (INT32 args)
{
  pop_n_elems (args);
  push_constant_text ("Thread.Thread(");
  push_int64((ptrdiff_t)THIS_THREAD->id);
  push_constant_text (")");
  f_add (3);
}

void f_thread_id_id_number(INT32 args)
{
  pop_n_elems(args);
  push_int64((ptrdiff_t)THIS_THREAD->id);
}

static void f_thread_id_result(INT32 args)
{
  struct thread_state *th=THIS_THREAD;

  if (threads_disabled) {
    Pike_error("Cannot wait for threads when threads are disabled!\n");
  }

  SWAP_OUT_CURRENT_THREAD();

  while(th->status != THREAD_EXITED) {
    co_wait_interpreter(&th->status_change);
    while (threads_disabled) {
      THREADS_FPRINTF(1, (stderr,
			  "f_thread_id_result(): Threads disabled\n"));
      co_wait_interpreter(&threads_disabled_change);
    }
  }

  SWAP_IN_CURRENT_THREAD();

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
    debug_gc_check(tmp->thread_local, T_OBJECT, o);

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
      fatal ("Didn't expect prctl to go wrong. errno=%d\n", errno);
#endif
  }
#endif /* HAVE_BROKEN_LINUX_THREAD_EUID */

  do
  {
/*     if(farmers == me) fatal("Ouch!\n"); */
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
    fatal("new_farmer(): Out of memory!\n");
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
  if(!fun) fatal("The farmers don't known how to handle empty fields\n");
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
  mt_lock_interpreter();
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
  struct program *tmp;
  ptrdiff_t mutex_offset;

#ifdef UNIX_THREADS
  
/* function(int:void) */
  ADD_EFUN("thread_set_concurrency",f_thread_set_concurrency,tFunc(tInt,tVoid), OPT_SIDE_EFFECT);
#endif

  START_NEW_PROGRAM_ID(THREAD_MUTEX_KEY);
  ADD_STORAGE(struct key_storage);
  set_init_callback(init_mutex_key_obj);
  set_exit_callback(exit_mutex_key_obj);
  mutex_key=Pike_compiler->new_program;
  add_ref(mutex_key);
  end_class("mutex_key", 0);
  mutex_key->flags|=PROGRAM_DESTRUCT_IMMEDIATE;
#ifdef PIKE_DEBUG
  if(!mutex_key)
    fatal("Failed to initialize mutex_key program!\n");
#endif

  START_NEW_PROGRAM_ID(THREAD_MUTEX);
  mutex_offset = ADD_STORAGE(struct mutex_storage);
  /* This is needed to allow the gc to find the possible circular reference.
   */
  map_variable("_owner", "object", ID_PRIVATE|ID_STATIC,
	       mutex_offset + OFFSETOF(mutex_storage, owner), T_OBJECT);
  /* function(int(0..2)|void:object(mutex_key)) */
  ADD_FUNCTION("lock",f_mutex_lock,
	       tFunc(tOr(tInt02,tVoid),tObjIs_THREAD_MUTEX_KEY),0);
  /* function(int(0..2)|void:object(mutex_key)) */
  ADD_FUNCTION("trylock",f_mutex_trylock,
	       tFunc(tOr(tInt02,tVoid),tObjIs_THREAD_MUTEX_KEY),0);
  set_init_callback(init_mutex_obj);
  set_exit_callback(exit_mutex_obj);
  end_class("mutex", 0);

  START_NEW_PROGRAM_ID(THREAD_CONDITION);
  ADD_STORAGE(COND_T);
  /* function(void|object(mutex_key):void) */
  ADD_FUNCTION("wait",f_cond_wait,
	       tFunc(tOr(tVoid,tObjIs_THREAD_MUTEX_KEY),tVoid),0);
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
	       tFunc(tNone,tVoid), ID_STATIC);
  thread_local_prog=Pike_compiler->new_program;
  add_ref(thread_local_prog);
  end_class("thread_local", 0);
  ADD_EFUN("thread_local", f_thread_local,
	   tFunc(tNone, tObjIs_THREAD_LOCAL),
	   OPT_EXTERNAL_DEPEND);
  if(!thread_local_prog)
    fatal("Failed to initialize thread_local program!\n");

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
    fatal("Failed to initialize thread program!\n");

  {
    struct object *o = Pike_interpreter.thread_id =
      clone_object(thread_id_prog,0);
    SWAP_OUT_THREAD(OBJ2THREAD(Pike_interpreter.thread_id)); /* Init struct */
    Pike_interpreter.thread_id = o;
  }
  OBJ2THREAD(Pike_interpreter.thread_id)->id=th_self();
  OBJ2THREAD(Pike_interpreter.thread_id)->swapped=0;
  thread_table_insert(Pike_interpreter.thread_id);
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

