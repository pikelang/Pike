#include "global.h"
RCSID("$Id: threads.c,v 1.121 2003/03/31 18:43:59 grubba Exp $");

int num_threads = 1;
int threads_disabled = 0;

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

#include <errno.h>

int live_threads = 0;
COND_T live_threads_change;
COND_T threads_disabled_change;
size_t thread_stack_size=1024 * 1204;

#ifndef HAVE_PTHREAD_ATFORK
#include "callback.h"

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

int low_nt_create_thread(unsigned stack_size,
			 unsigned (TH_STDCALL *fun)(void *),
			 void *arg,
			 unsigned *id)
{
  HANDLE h=_beginthreadex(NULL, stack_size, fun, arg, 0, id);
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

HANDLE CheckValidHandle(HANDLE h)
{
  if(!IsValidHandle(h))
    fatal("Invalid handle!\n");
  return h;
}

#endif

#endif

#ifdef SIMULATE_COND_WITH_EVENT
int co_wait(COND_T *c, MUTEX_T *m)
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

int co_signal(COND_T *c)
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

int co_broadcast(COND_T *c)
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

int co_destroy(COND_T *c)
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

struct object *thread_id = NULL;
static struct callback *threads_evaluator_callback=0;
int thread_id_result_variable;

MUTEX_T interpreter_lock, thread_table_lock, interleave_lock;
struct program *mutex_key = 0;
struct program *thread_id_prog = 0;
struct program *thread_local_prog = 0;
#ifdef POSIX_THREADS
pthread_attr_t pattr;
pthread_attr_t small_pattr;
#endif
int thread_storage_offset;

struct thread_starter
{
  struct object *id;
  struct array *args;
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

    if (thread_id) {
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
      co_wait(&live_threads_change, &interpreter_lock);
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


void thread_table_insert(struct object *o)
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

void thread_table_delete(struct object *o)
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

struct thread_state *thread_state_for_id(THREAD_T tid)
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
      /* Move the thread_state to the head of the chain, in case
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

struct object *thread_for_id(THREAD_T tid)
{
  struct thread_state *s = thread_state_for_id(tid);
  return (s == NULL? NULL : THREADSTATE2OBJ(s));
  /* See NB in thread_state_for_id.  Lifespan of result can be prolonged
     by incrementing refcount though. */
}

void f_all_threads(INT32 args)
{
  /* Return an unordered array containing all threads that was running
     at the time this function was invoked */

  INT32 x;
  struct svalue *oldsp;
  struct thread_state *s;

  pop_n_elems(args);
  oldsp = sp;
  mt_lock( & thread_table_lock );
  for(x=0; x<THREAD_TABLE_SIZE; x++)
    for(s=thread_table_chains[x]; s; s=s->hashlink) {
      struct object *o = THREADSTATE2OBJ(s);
      ref_push_object(o);
    }
  mt_unlock( & thread_table_lock );
  f_aggregate(sp-oldsp);
}

int count_pike_threads(void)
{
  return num_pike_threads;
}

static void check_threads(struct callback *cb, void *arg, void * arg2)
{
  static int div_;
  if(div_++ & 255) return;

#ifdef DEBUG
  if(thread_for_id(th_self()) != thread_id)
    fatal("thread_for_id() (or thread_id) failed!\n")
#endif

  THREADS_ALLOW();
  /* Allow other threads to run */
  THREADS_DISALLOW();

#ifdef DEBUG
  if(thread_for_id(th_self()) != thread_id)
    fatal("thread_for_id() (or thread_id) failed!\n")
#endif


}

TH_RETURN_TYPE new_thread_func(void * data)
{
  struct thread_starter arg = *(struct thread_starter *)data;
  JMP_BUF back;
  INT32 tmp;

  THREADS_FPRINTF(0, (stderr,"new_thread_func(): Thread %08x created...\n",
		      (unsigned int)arg.id));
  
  if((tmp=mt_lock( & interpreter_lock)))
    fatal("Failed to lock interpreter, return value=%d, errno=%d\n",tmp,
#ifdef __NT__
	  GetLastError()
#else
	  errno
#endif
	  );

  while (threads_disabled) {
    THREADS_FPRINTF(1, (stderr,
			"new_thread_func(): Threads disabled\n"));
    co_wait(&threads_disabled_change, &interpreter_lock);
  }

  init_interpreter();
  thread_id=arg.id;
  stack_top=((char *)&data)+ (thread_stack_size-16384) * STACK_DIRECTION;
  recoveries = NULL;
  SWAP_OUT_THREAD(OBJ2THREAD(thread_id)); /* Init struct */
  thread_id=arg.id;
  OBJ2THREAD(thread_id)->swapped=0;

#if defined(PIKE_DEBUG)
  if(d_flag)
    {
      THREAD_T self = th_self();

      if( thread_id && !th_equal( OBJ2THREAD(thread_id)->id, self) )
	fatal("Current thread is wrong. %x %x\n",
	      OBJ2THREAD(thread_id)->id, self);
	
      if(thread_for_id(th_self()) != thread_id)
	fatal("thread_for_id() (or thread_id) failed in new_thread_func! "
	      "%p != %p\n", thread_for_id(self), thread_id);
    }
#endif

#ifdef THREAD_TRACE
  {
    t_flag = default_t_flag;
  }
#endif /* THREAD_TRACE */

  THREADS_FPRINTF(0, (stderr,"THREAD %08x INITED\n",(unsigned int)thread_id));
  if(SETJMP(back))
  {
    if(throw_severity < THROW_EXIT)
    {
      ONERROR tmp;
      t_flag=0;
      SET_ONERROR(tmp,exit_on_error,"Error in handle_error in master object!");
      assign_svalue_no_free(sp++, & throw_value);
      APPLY_MASTER("handle_error", 1);
      pop_stack();
      UNSET_ONERROR(tmp);
    }
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

    /* copy return value to the thread_id here */
    object_low_set_index(thread_id,
			 thread_id_result_variable,
			 sp-1);
    pop_stack();
  }

  if(OBJ2THREAD(thread_id)->thread_local != NULL) {
    free_mapping(OBJ2THREAD(thread_id)->thread_local);
    OBJ2THREAD(thread_id)->thread_local = NULL;
  }

   OBJ2THREAD(thread_id)->status=THREAD_EXITED;
   co_broadcast(& OBJ2THREAD(thread_id)->status_change);

  free((char *)data); /* Moved by per, to avoid some bugs.... */
  UNSETJMP(back);

  THREADS_FPRINTF(0, (stderr,"THREADS_ALLOW() Thread %08x done\n",
		      (unsigned int)thread_id));

  cleanup_interpret();
  DO_IF_DMALLOC(
    SWAP_OUT_THREAD(OBJ2THREAD(thread_id)); /* de-Init struct */
    OBJ2THREAD(thread_id)->swapped=0;
    )
  thread_table_delete(thread_id);
  free_object(thread_id);
  thread_id=0;
  num_threads--;
  if(!num_threads && threads_evaluator_callback)
  {
    remove_callback(threads_evaluator_callback);
    threads_evaluator_callback=0;
  }
  mt_unlock(& interpreter_lock);
  th_exit(0);
  /* NOT_REACHED, but removes a warning */
  return(NULL);
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
    error("Failed to create thread (errno = %d).\n",tmp);
  }
}

#ifdef UNIX_THREADS
void f_thread_set_concurrency(INT32 args)
{
  int c=1;
  if(args) c=sp[-args].u.integer;
  else error("No argument to thread_set_concurrency(int concurrency);\n");
  pop_n_elems(args);
  num_lwps=c;
  th_setconcurrency(c);
}
#endif

void f_this_thread(INT32 args)
{
  pop_n_elems(args);
  ref_push_object(thread_id);
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
};

struct key_storage
{
  struct mutex_storage *mut;
  struct object *owner;
  int initialized;
};

#define OB2KEY(X) ((struct key_storage *)((X)->storage))

void f_mutex_lock(INT32 args)
{
  struct mutex_storage  *m;
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
      bad_arg_error("mutex->lock", sp-args, args, 2, "int(0..2)", sp+1-args,
		  "Unknown mutex locking style: %d\n",type);
      

    case 0:
    case 2:
      if(m->key && OB2KEY(m->key)->owner == thread_id)
      {
	THREADS_FPRINTF(0,
			(stderr, "Recursive LOCK k:%08x, m:%08x(%08x), t:%08x\n",
			 (unsigned int)OB2KEY(m->key),
			 (unsigned int)m,
			 (unsigned int)OB2KEY(m->key)->mut,
			 (unsigned int) thread_id));

	if(type==0) error("Recursive mutex locks!\n");

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

  DO_IF_DEBUG( if(thread_for_id(th_self()) != thread_id)
	       fatal("thread_for_id() (or thread_id) failed! %p != %p\n",thread_for_id(th_self()),thread_id) ; )

  if(m->key)
  {
    if(threads_disabled)
    {
      free_object(o);
      error("Cannot wait for mutexes when threads are disabled!\n");
    }
    SWAP_OUT_CURRENT_THREAD();
    do
    {
      THREADS_FPRINTF(1, (stderr,"WAITING TO LOCK m:%08x\n",(unsigned int)m));
      co_wait(& m->condition, & interpreter_lock);
      while (threads_disabled) {
	THREADS_FPRINTF(1, (stderr,
			    "f_mutex_lock(): Threads disabled\n"));
	co_wait(&threads_disabled_change, &interpreter_lock);
      }
    }while(m->key);
    SWAP_IN_CURRENT_THREAD();
  }
  m->key=o;
  OB2KEY(o)->mut=m;

  DO_IF_DEBUG( if(thread_for_id(th_self()) != thread_id)
	       fatal("thread_for_id() (or thread_id) failed! %p != %p\n",thread_for_id(th_self()),thread_id) ; )

  THREADS_FPRINTF(1, (stderr, "LOCK k:%08x, m:%08x(%08x), t:%08x\n",
		      (unsigned int)OB2KEY(o),
		      (unsigned int)m,
		      (unsigned int)OB2KEY(m->key)->mut,
		      (unsigned int)thread_id));
  pop_n_elems(args);
  push_object(o);
}

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
    get_all_args("mutex->lock",args,"%i",&type);

  switch(type)
  {
    default:
      bad_arg_error("mutex->trylock", sp-args, args, 2, "int(0..2)", sp+1-args,
		  "Unknown mutex locking style: %d\n",type);

    case 0:
      if(m->key && OB2KEY(m->key)->owner == thread_id)
      {
	error("Recursive mutex locks!\n");
      }

    case 2:
    case 1:
      break;
  }

  o=clone_object(mutex_key,0);

  if(!m->key)
  {
    OB2KEY(o)->mut=m;
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

void init_mutex_obj(struct object *o)
{
  co_init(& THIS_MUTEX->condition);
  THIS_MUTEX->key=0;
}

void exit_mutex_obj(struct object *o)
{
  THREADS_FPRINTF(1, (stderr, "DESTROYING MUTEX m:%08x\n",
		      (unsigned int)THIS_MUTEX));
  if(THIS_MUTEX->key) destruct(THIS_MUTEX->key);
  THIS_MUTEX->key=0;
  co_destroy(& THIS_MUTEX->condition);
}

#define THIS_KEY ((struct key_storage *)(CURRENT_STORAGE))
void init_mutex_key_obj(struct object *o)
{
  THREADS_FPRINTF(1, (stderr, "KEY k:%08x, o:%08x\n",
		      (unsigned int)THIS_KEY, (unsigned int)thread_id));
  THIS_KEY->mut=0;
  add_ref(THIS_KEY->owner=thread_id);
  THIS_KEY->initialized=1;
}

void exit_mutex_key_obj(struct object *o)
{
  THREADS_FPRINTF(1, (stderr, "UNLOCK k:%08x m:(%08x) t:%08x o:%08x\n",
		      (unsigned int)THIS_KEY,
		      (unsigned int)THIS_KEY->mut,
		      (unsigned int)thread_id,
		      (unsigned int)THIS_KEY->owner));
  if(THIS_KEY->mut)
  {
    struct mutex_storage *mut = THIS_KEY->mut;

#ifdef PIKE_DEBUG
    if(mut->key != o)
      fatal("Mutex unlock from wrong key %p != %p!\n",THIS_KEY->mut->key,o);
#endif
    mut->key=0;
    if (THIS_KEY->owner) {
      free_object(THIS_KEY->owner);
      THIS_KEY->owner=0;
    }
    THIS_KEY->mut=0;
    THIS_KEY->initialized=0;
    co_signal(& mut->condition);
  }
}

#define THIS_COND ((COND_T *)(CURRENT_STORAGE))
void f_cond_wait(INT32 args)
{
  COND_T *c;

  if(threads_disabled)
    error("Cannot wait for conditions when threads are disabled!\n");
      
  if(args > 1) {
    pop_n_elems(args - 1);
    args = 1;
  }

  c=THIS_COND;

  if((args > 0) && !IS_ZERO(sp-1))
  {
    struct object *key;
    struct mutex_storage *mut;

    if(sp[-1].type != T_OBJECT)
      error("Bad argument 1 to condition->wait()\n");
    
    key=sp[-1].u.object;
    
    if(key->prog != mutex_key)
      error("Bad argument 1 to condition->wait()\n");

    if (OB2KEY(key)->initialized) {

      mut = OB2KEY(key)->mut;
      if(!mut)
	error("Bad argument 1 to condition->wait()\n");

      /* Unlock mutex */
      mut->key=0;
      OB2KEY(key)->mut=0;
      co_signal(& mut->condition);
    
      /* Wait and allow mutex operations */
      SWAP_OUT_CURRENT_THREAD();
      co_wait(c, &interpreter_lock);
    
      /* Lock mutex */
      while(mut->key) {
	co_wait(& mut->condition, &interpreter_lock);
	while (threads_disabled) {
	  THREADS_FPRINTF(1, (stderr,
			      "f_cond_wait(): Threads disabled\n"));
	  co_wait(&threads_disabled_change, &interpreter_lock);
	}
      }
      mut->key=key;
      OB2KEY(key)->mut=mut;
      
      SWAP_IN_CURRENT_THREAD();
      pop_n_elems(args);
      return;
    }
  }

  SWAP_OUT_CURRENT_THREAD();
  co_wait(c, &interpreter_lock);
  while (threads_disabled) {
    THREADS_FPRINTF(1, (stderr,
			"f_cond_wait(): Threads disabled\n"));
    co_wait(&threads_disabled_change, &interpreter_lock);
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
  struct thread_state *foo = THIS_THREAD;
  struct thread_state *bar = OBJ2THREAD( thread_id );
  struct svalue *osp = sp;
  pop_n_elems(args);
  if(foo->sp)
  {
    SWAP_OUT_THREAD(bar);
    SWAP_IN_THREAD(foo);
    sp=osp;
    f_backtrace(0);
    osp=sp;
    sp=foo->sp;
    SWAP_OUT_THREAD(foo);
    SWAP_IN_THREAD(bar);
    sp=osp;
  } else {
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
  push_int (THIS_THREAD->id);
  push_constant_text (")");
  f_add (3);
}

static void f_thread_id_result(INT32 args)
{
  struct thread_state *th=THIS_THREAD;

  SWAP_OUT_CURRENT_THREAD();

  while(th->status != THREAD_EXITED) {
    co_wait(&th->status_change, &interpreter_lock);
    while (threads_disabled) {
      THREADS_FPRINTF(1, (stderr,
			  "f_thread_id_result(): Threads disabled\n"));
      co_wait(&threads_disabled_change, &interpreter_lock);
    }
  }

  SWAP_IN_CURRENT_THREAD();

  low_object_index_no_free(sp,
			   fp->current_object, 
			   thread_id_result_variable);
  sp++;
}

void init_thread_obj(struct object *o)
{
  MEMSET(THIS_THREAD, 0, sizeof(struct thread_state));
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

static void thread_was_marked(struct object *o)
{
  struct thread_state *tmp=THIS_THREAD;
  if(tmp->thread_local != NULL)
    gc_mark_mapping_as_referenced(tmp->thread_local);
#ifdef PIKE_DEBUG
  if(tmp->swapped)
  {
    debug_gc_xmark_svalues(tmp->evaluator_stack,tmp->sp-tmp->evaluator_stack-1,"idle thread stack");
  }
#endif
}

static void thread_was_checked(struct object *o)
{
  struct thread_state *tmp=THIS_THREAD;
  if(tmp->thread_local != NULL)
    gc_check(tmp->thread_local);  
}

void f_thread_local(INT32 args)
{
  static INT32 thread_local_id = 0;

  struct object *loc = clone_object(thread_local_prog,0);
  ((struct thread_local *)loc->storage)->id = thread_local_id++;
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
  if(thread_id != NULL &&
     (m = OBJ2THREAD(thread_id)->thread_local) != NULL)
    mapping_index_no_free(sp++, m, &key);
  else {
    push_int(0);
    sp[-1].subtype=NUMBER_UNDEFINED;
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
    error("Too few arguments to thread_local->set()\n");

  if(thread_id == NULL)
    error("Trying to set thread_local without thread!\n");

  if((m = OBJ2THREAD(thread_id)->thread_local) == NULL)
    m = OBJ2THREAD(thread_id)->thread_local =
      allocate_mapping(4);

  mapping_insert(m, &key, &sp[-1]);
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
} *farmers;

static MUTEX_T rosie;

static TH_RETURN_TYPE farm(void *_a)
{
  struct farmer *me = (struct farmer *)_a;
  do
  {
/*     if(farmers == me) fatal("Ouch!\n"); */
/*     fprintf(stderr, "farm_begin %p\n",me ); */
    me->harvest( me->field );
/*     fprintf(stderr, "farm_end %p\n", me); */

    me->harvest = 0;
    mt_lock( &rosie );
    me->neighbour = farmers;
    farmers = me;
/*     fprintf(stderr, "farm_wait %p\n", me); */
    while(!me->harvest) co_wait( &me->harvest_moon, &rosie );
    mt_unlock( &rosie );
/*     fprintf(stderr, "farm_endwait %p\n", me); */
  } while(1);
  /* NOT_REACHED */
  return NULL;	/* Keep the compiler happy. */
}

int th_num_idle_farmers(void)
{
  int q = 0;
  struct farmer *f = farmers;
  while(f) { f = f->neighbour; q++; }
  return q;
}

static int _num_farmers;
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
  th_create_small(&me->me, farm, me);
  return me;
}

void th_farm(void (*fun)(void *), void *here)
{
  if(!fun) fatal("The farmers don't known how to handle empty fields\n");
  mt_lock( &rosie );
  if(farmers)
  {
    struct farmer *f = farmers;
    farmers = f->neighbour;
    mt_unlock( &rosie );
    f->field = here;
    f->harvest = fun;
    co_signal( &f->harvest_moon );
    return;
  }
  mt_unlock( &rosie );
  new_farmer( fun, here );
}

void low_th_init(void)
{
#ifdef SGI_SPROC_THREADS
#error /* Need to specify a filename */
  us_cookie = usinit("");
#endif /* SGI_SPROC_THREADS */

  THREADS_FPRINTF(0, (stderr, "low_th_init() Initializing threads.\n"));

#ifdef POSIX_THREADS
#ifdef HAVE_PTHREAD_INIT
  pthread_init();
#endif /* HAVE_PTHREAD_INIT */
#endif /* POSIX_THREADS */

  mt_init( & interpreter_lock);
  mt_lock( & interpreter_lock);
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
#endif
}

void th_init(void)
{
  struct program *tmp;
  INT32 mutex_key_offset;

#ifdef UNIX_THREADS
  
/* function(int:void) */
  ADD_EFUN("thread_set_concurrency",f_thread_set_concurrency,tFunc(tInt,tVoid), OPT_SIDE_EFFECT);
#endif

  START_NEW_PROGRAM_ID(THREAD_MUTEX_KEY);
  mutex_key_offset = ADD_STORAGE(struct key_storage);
  /* This is needed to allow the gc to find the possible circular reference.
   * It also allows a process to take over ownership of a key.
   */
  map_variable("_owner", "object", 0,
	       mutex_key_offset + OFFSETOF(key_storage, owner), T_OBJECT);
  set_init_callback(init_mutex_key_obj);
  set_exit_callback(exit_mutex_key_obj);
  mutex_key=new_program;
  add_ref(mutex_key);
  end_class("mutex_key", 0);
  mutex_key->flags|=PROGRAM_DESTRUCT_IMMEDIATE;
#ifdef PIKE_DEBUG
  if(!mutex_key)
    fatal("Failed to initialize mutex_key program!\n");
#endif

  START_NEW_PROGRAM_ID(THREAD_MUTEX);
  ADD_STORAGE(struct mutex_storage);
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
    tmp = new_program;
    add_ref(tmp);
    end_class("threads_disabled", 0);
    tmp->flags|=PROGRAM_DESTRUCT_IMMEDIATE;
    add_global_program("_disable_threads", tmp);
    free_program(tmp);
  }

  START_NEW_PROGRAM_ID(THREAD_LOCAL);
  ADD_STORAGE(struct thread_local);
  /* function(:mixed) */
  ADD_FUNCTION("get",f_thread_local_get,tFunc(tNone,tMix),0);
  /* function(mixed:mixed) */
  ADD_FUNCTION("set",f_thread_local_set,tFunc(tMix,tMix),0);
  thread_local_prog=new_program;
  add_ref(thread_local_prog);
  end_class("thread_local", 0);
  if(!thread_local_prog)
    fatal("Failed to initialize thread_local program!\n");
  /* function(:object(thread_local)) */
  ADD_EFUN("thread_local",f_thread_local,
	   tFunc(tNone,tObjIs_THREAD_LOCAL),
	   OPT_SIDE_EFFECT);

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
  set_gc_mark_callback(thread_was_marked);
  set_gc_check_callback(thread_was_checked);
  set_init_callback(init_thread_obj);
  set_exit_callback(exit_thread_obj);
  thread_id_prog=new_program;
  add_ref(thread_id_prog);
  end_class("thread_id", 0);

  /* function(mixed ...:object(thread_id)) */
  ADD_EFUN("thread_create",f_thread_create,
	   tFuncV(tNone,tMixed,tObjIs_THREAD_ID),
	   OPT_SIDE_EFFECT);

  /* function(:object(thread_id)) */
  ADD_EFUN("this_thread",f_this_thread,
	   tFunc(tNone,tObjIs_THREAD_ID),
	   OPT_EXTERNAL_DEPEND);

  /* function(:array(object(thread_id))) */
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
    struct object *o = thread_id = clone_object(thread_id_prog,0);
    SWAP_OUT_THREAD(OBJ2THREAD(thread_id)); /* Init struct */
    thread_id = o;
  }
  OBJ2THREAD(thread_id)->swapped=0;
  OBJ2THREAD(thread_id)->id=th_self();
  thread_table_insert(thread_id);
}

void th_cleanup(void)
{
  if(thread_id)
  {
    thread_table_delete(thread_id);
    destruct(thread_id);
    free_object(thread_id);
    thread_id=0;
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
}
#endif
