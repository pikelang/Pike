#include "global.h"
RCSID("$Id: threads.c,v 1.62 1998/03/25 04:40:48 hubbe Exp $");

int num_threads = 1;
int threads_disabled = 0;

#ifdef _REENTRANT
#include "threads.h"
#include "array.h"
#include "object.h"
#include "pike_macros.h"
#include "callback.h"
#include "builtin_functions.h"
#include "constants.h"
#include "program.h"
#include "gc.h"
#include "main.h"

#ifdef __NT__

#ifdef DEBUG
static int IsValidHandle(HANDLE h)
{
  __try {
    HANDLE ret;
    if(DuplicateHandle(GetCurrentProcess(),
			h,
			GetCurrentProcess(),
			&ret,
			NULL,
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

void CheckValidHandle(HANDLE h)
{
  if(!IsValidHandle((HANDLE)h))
    fatal("Invalid handle!\n");
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

#ifdef DEBUG
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


#define THIS_THREAD ((struct thread_state *)fp->current_storage)

struct object *thread_id;
static struct callback *threads_evaluator_callback=0;
int thread_id_result_variable;

MUTEX_T interpreter_lock, thread_table_lock;
struct program *mutex_key = 0;
struct program *thread_id_prog = 0;
#ifdef POSIX_THREADS
pthread_attr_t pattr;
pthread_attr_t small_pattr;
#endif

struct thread_starter
{
  struct object *id;
  struct array *args;
};

int threads_denied;

void f_thread_disallow(INT32 args)
{
  threads_denied = sp[-1].u.integer;
  pop_n_elems(args);
}

/* Thread hashtable */

#define THREAD_TABLE_SIZE 127  /* Totally arbitrary prime */

static struct thread_state *thread_table_chains[THREAD_TABLE_SIZE];

void thread_table_init()
{
  INT32 x;
  for(x=0; x<THREAD_TABLE_SIZE; x++)
    thread_table_chains[x] = NULL;
}

unsigned INT32 thread_table_hash(THREAD_T *tid)
{
  return th_hash(*tid) % THREAD_TABLE_SIZE;
}

void thread_table_insert(struct object *o)
{
  struct thread_state *s = (struct thread_state *)o->storage;
  unsigned INT32 h = thread_table_hash(&s->id);
  mt_lock( & thread_table_lock );
  if((s->hashlink = thread_table_chains[h]) != NULL)
    s->hashlink->backlink = &s->hashlink;
  thread_table_chains[h] = s;
  s->backlink = &thread_table_chains[h];
  mt_unlock( & thread_table_lock );  
}

void thread_table_delete(struct object *o)
{
  struct thread_state *s = (struct thread_state *)o->storage;
  mt_lock( & thread_table_lock );
  if(s->hashlink != NULL)
    s->hashlink->backlink = s->backlink;
  *(s->backlink) = s->hashlink;
  mt_unlock( & thread_table_lock );
}

struct thread_state *thread_state_for_id(THREAD_T tid)
{
  unsigned INT32 h = thread_table_hash(&tid);
  struct thread_state *s = NULL;
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
  return s;
  /* NOTEZ BIEN:  Return value only guaranteed to remain valid as long
     as you have the interpreter lock, unless tid == th_self() */
}

struct object *thread_for_id(THREAD_T tid)
{
  struct thread_state *s = thread_state_for_id(tid);
  return (s == NULL? NULL :
	  (struct object *)(((char *)s)-((((struct object *)NULL)->storage)-
					 ((char*)NULL))));
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
      struct object *o =
	(struct object *)(((char *)s)-((((struct object *)NULL)->storage)-
				       ((char*)NULL)));
      o->refs++;
      push_object(o);
    }
  mt_unlock( & thread_table_lock );
  f_aggregate(sp-oldsp);
}


static void check_threads(struct callback *cb, void *arg, void * arg2)
{
  static int div_;
  if(div_++ & 255) return;

  if(!threads_denied)
  {
    THREADS_ALLOW();

    /* Allow other threads to run */

    THREADS_DISALLOW();
  }
}

void *new_thread_func(void * data)
{
  struct thread_starter arg = *(struct thread_starter *)data;
  JMP_BUF back;
  INT32 tmp;

  THREADS_FPRINTF((stderr,"THREADS_DISALLOW() Thread %08x created...\n",
		   (unsigned int)arg.id));
  
  if((tmp=mt_lock( & interpreter_lock)))
    fatal("Failed to lock interpreter, errno %d\n",tmp);
  init_interpreter();
  thread_id=arg.id;
  SWAP_OUT_THREAD((struct thread_state *)thread_id->storage); /* Init struct */
  ((struct thread_state *)thread_id->storage)->swapped=0;

  THREADS_FPRINTF((stderr,"THREAD %08x INITED\n",(unsigned int)thread_id));
  if(SETJMP(back))
  {
    if(throw_severity < THROW_EXIT)
    {
      ONERROR tmp;
      SET_ONERROR(tmp,exit_on_error,"Error in handle_error in master object!");
      assign_svalue_no_free(sp++, & throw_value);
      APPLY_MASTER("handle_error", 1);
      pop_stack();
      UNSET_ONERROR(tmp);
    }
    if(throw_severity == THROW_EXIT)
    {
      do_exit(throw_value.u.integer);
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

   ((struct thread_state *)(thread_id->storage))->status=THREAD_EXITED;
   co_signal(& ((struct thread_state *)(thread_id->storage))->status_change);

  free((char *)data); /* Moved by per, to avoid some bugs.... */
  UNSETJMP(back);

  THREADS_FPRINTF((stderr,"THREADS_ALLOW() Thread %08x done\n",
		   (unsigned int)thread_id));

  thread_table_delete(thread_id);
  free_object(thread_id);
  thread_id=0;
  cleanup_interpret();
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
  arg=ALLOC_STRUCT(thread_starter);
  arg->args=aggregate_array(args);
  arg->id=clone_object(thread_id_prog,0);
  ((struct thread_state *)arg->id->storage)->status=THREAD_RUNNING;

  tmp=th_create(&((struct thread_state *)arg->id->storage)->id,
		new_thread_func,
		arg);

  if(!tmp)
  {
    num_threads++;
    thread_table_insert(arg->id);

    if(!threads_evaluator_callback)
    {
      threads_evaluator_callback=add_to_callback(&evaluator_callbacks,
						 check_threads, 0,0);
    }
    push_object(arg->id);
    arg->id->refs++;
    THREADS_FPRINTF((stderr,"THREAD_CREATE -> t:%08x\n",(unsigned int)arg->id));
  } else {
    free_object(arg->id);
    free_array(arg->args);
    free((char *)arg);
    error("Failed to create thread (errno = %d).\n",errno);
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
  push_object(thread_id);
  thread_id->refs++;
}

#define THIS_MUTEX ((struct mutex_storage *)(fp->current_storage))


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

  m=THIS_MUTEX;
  /* Needs to be cloned here, since create()
   * might use threads.
   */
  o=clone_object(mutex_key,0);
  if(!args || IS_ZERO(sp-args))
  {
    if(m->key && OB2KEY(m->key)->owner == thread_id)
    {
      THREADS_FPRINTF((stderr, "Recursive LOCK k:%08x, m:%08x(%08x), t:%08x\n",
		       (unsigned int)OB2KEY(m->key),
		       (unsigned int)m,
		       (unsigned int)OB2KEY(m->key)->mut,
		       (unsigned int) thread_id));
      free_object(o);
      error("Recursive mutex locks!\n");
    }
  }

  if(m->key)
  {
    SWAP_OUT_CURRENT_THREAD();
    do
    {
      THREADS_FPRINTF((stderr,"WAITING TO LOCK m:%08x\n",(unsigned int)m));
      co_wait(& m->condition, & interpreter_lock);
    }while(m->key);
    SWAP_IN_CURRENT_THREAD();
  }
  m->key=o;
  OB2KEY(o)->mut=m;

  THREADS_FPRINTF((stderr, "LOCK k:%08x, m:%08x(%08x), t:%08x\n",
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
  int i=0;

  o=clone_object(mutex_key,0);
  m=THIS_MUTEX;

  /* No reason to release the interpreter lock here
   * since we aren't calling any functions that take time.
   */

  if(!args || IS_ZERO(sp-args))
  {
    if(m->key && OB2KEY(m->key)->owner == thread_id)
    {
      free_object(o);
      error("Recursive mutex locks!\n");
    }
  }

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
  THREADS_FPRINTF((stderr,"DESTROYING MUTEX m:%08x\n",(unsigned int)THIS_MUTEX));
  if(THIS_MUTEX->key) destruct(THIS_MUTEX->key);
  THIS_MUTEX->key=0;
  co_destroy(& THIS_MUTEX->condition);
}

#define THIS_KEY ((struct key_storage *)(fp->current_storage))
void init_mutex_key_obj(struct object *o)
{
  THREADS_FPRINTF((stderr, "KEY k:%08x, o:%08x\n",
		   (unsigned int)THIS_KEY, (unsigned int)thread_id));
  THIS_KEY->mut=0;
  THIS_KEY->owner=thread_id;
  thread_id->refs++;
  THIS_KEY->initialized=1;
}

void exit_mutex_key_obj(struct object *o)
{
  THREADS_FPRINTF((stderr, "UNLOCK k:%08x m:(%08x) t:%08x o:%08x\n",
		   (unsigned int)THIS_KEY,
		   (unsigned int)THIS_KEY->mut,
		   (unsigned int)thread_id,
		   (unsigned int)THIS_KEY->owner));
  if(THIS_KEY->mut)
  {
    struct mutex_storage *mut = THIS_KEY->mut;

#ifdef DEBUG
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

#define THIS_COND ((COND_T *)(fp->current_storage))
void f_cond_wait(INT32 args)
{
  COND_T *c;
  struct object *key;

  if(args > 1) pop_n_elems(args - 1);

  c=THIS_COND;

  if(args > 0)
  {
    struct mutex_storage *mut;

    if(sp[-1].type != T_OBJECT)
      error("Bad argument 1 to condition->wait()\n");
    
    key=sp[-1].u.object;
    
    if(key->prog != mutex_key)
      error("Bad argument 1 to condition->wait()\n");
    
    mut=OB2KEY(key)->mut;
    if(!mut) error("Bad argument 1 to condition->wait()\n");

    /* Unlock mutex */
    mut->key=0;
    OB2KEY(key)->mut=0;
    co_signal(& mut->condition);
    
    /* Wait and allow mutex operations */
    SWAP_OUT_CURRENT_THREAD();
    co_wait(c, &interpreter_lock);
    
    if(OB2KEY(key)->initialized)
    {
      /* Lock mutex */
      while(mut->key) co_wait(& mut->condition, &interpreter_lock);
      mut->key=key;
      OB2KEY(key)->mut=mut;
    }
    SWAP_IN_CURRENT_THREAD();
    pop_stack();
  } else {
    SWAP_OUT_CURRENT_THREAD();
    co_wait(c, &interpreter_lock);
    SWAP_IN_CURRENT_THREAD();
  }
}

void f_cond_signal(INT32 args) { pop_n_elems(args); co_signal(THIS_COND); }
void f_cond_broadcast(INT32 args) { pop_n_elems(args); co_broadcast(THIS_COND); }
void init_cond_obj(struct object *o) { co_init(THIS_COND); }
void exit_cond_obj(struct object *o) { co_destroy(THIS_COND); }

void f_thread_backtrace(INT32 args)
{
  struct thread_state *foo = (struct thread_state *)fp->current_object->storage;
  struct thread_state *bar = (struct thread_state *)thread_id->storage;
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

static void f_thread_id_result(INT32 args)
{
  struct thread_state *th=THIS_THREAD;

  SWAP_OUT_CURRENT_THREAD();

  while(th->status != THREAD_EXITED)
    co_wait(&th->status_change, &interpreter_lock);

  SWAP_IN_CURRENT_THREAD();

  low_object_index_no_free(sp,
			   fp->current_object, 
			   thread_id_result_variable);
  sp++;
}

void init_thread_obj(struct object *o)
{
  MEMSET(o->storage, 0, sizeof(struct thread_state));
  THIS_THREAD->status=THREAD_NOT_STARTED;
  co_init(& THIS_THREAD->status_change);
}


void exit_thread_obj(struct object *o)
{
  co_destroy(& THIS_THREAD->status_change);
  th_destroy(& THIS_THREAD->id);
}

#ifdef DEBUG
static void thread_was_marked(struct object *o)
{
  struct thread_state *tmp=(struct thread_state *)(o->storage);
  if(tmp->swapped)
  {
    debug_gc_xmark_svalues(tmp->evaluator_stack,tmp->sp-tmp->evaluator_stack-1,"idle thread stack");
  }
}
#endif

void th_init(void)
{
  struct program *tmp;
  INT32 mutex_key_offset;

#ifdef SGI_SPROC_THREADS
#error /* Need to specify a filename */
  us_cookie = usinit("");
#endif /* SGI_SPROC_THREADS */

  THREADS_FPRINTF((stderr, "THREADS_DISALLOW() Initializing threads.\n"));

#ifdef POSIX_THREADS
#ifdef HAVE_PTHREAD_INIT
  pthread_init();
#endif /* HAVE_PTHREAD_INIT */
#endif /* POSIX_THREADS */

  mt_init( & interpreter_lock);
  mt_lock( & interpreter_lock);
  mt_init( & thread_table_lock);
  thread_table_init();
#ifdef POSIX_THREADS
  pthread_attr_init(&pattr);
#ifdef HAVE_PTHREAD_ATTR_SETSTACKSIZE
  pthread_attr_setstacksize(&pattr, 1024 * 1204);
#endif
  pthread_attr_setdetachstate(&pattr, PTHREAD_CREATE_DETACHED);

  pthread_attr_init(&small_pattr);
#ifdef HAVE_PTHREAD_ATTR_SETSTACKSIZE
  pthread_attr_setstacksize(&small_pattr, 32768);
#endif
  pthread_attr_setdetachstate(&small_pattr, PTHREAD_CREATE_DETACHED);

#endif
  
  add_efun("thread_disallow", f_thread_disallow, "function(int:void)",
	   OPT_SIDE_EFFECT);

  add_efun("thread_create",f_thread_create,"function(mixed ...:object)",
           OPT_SIDE_EFFECT);
#ifdef UNIX_THREADS
  add_efun("thread_set_concurrency",f_thread_set_concurrency,
	   "function(int:void)", OPT_SIDE_EFFECT);
#endif
  add_efun("this_thread",f_this_thread,"function(:object)",
           OPT_EXTERNAL_DEPEND);
  add_efun("all_threads",f_all_threads,"function(:array(object))",
	   OPT_EXTERNAL_DEPEND);

  start_new_program();
  add_storage(sizeof(struct mutex_storage));
  add_function("lock",f_mutex_lock,"function(int|void:object)",0);
  add_function("trylock",f_mutex_trylock,"function(int|void:object)",0);
  set_init_callback(init_mutex_obj);
  set_exit_callback(exit_mutex_obj);
  end_class("mutex", 0);

  start_new_program();
  mutex_key_offset = add_storage(sizeof(struct key_storage));
  /* This is needed to allow the gc to find the possible circular reference.
   * It also allows a process to take over ownership of a key.
   */
  map_variable("_owner", "object", 0,
	       mutex_key_offset + OFFSETOF(key_storage, owner), T_OBJECT);
  set_init_callback(init_mutex_key_obj);
  set_exit_callback(exit_mutex_key_obj);
  mutex_key=end_program();
  mutex_key->flags|=PROGRAM_DESTRUCT_IMMEDIATE;
#ifdef DEBUG
  if(!mutex_key)
    fatal("Failed to initialize mutex_key program!\n");
#endif

  start_new_program();
  add_storage(sizeof(COND_T));
  add_function("wait",f_cond_wait,"function(void|object:void)",0);
  add_function("signal",f_cond_signal,"function(:void)",0);
  add_function("broadcast",f_cond_broadcast,"function(:void)",0);
  set_init_callback(init_cond_obj);
  set_exit_callback(exit_cond_obj);
  end_class("condition", 0);

  start_new_program();
  add_storage(sizeof(struct thread_state));
  thread_id_result_variable=simple_add_variable("result","mixed",0);
  add_function("backtrace",f_thread_backtrace,"function(:array)",0);
  add_function("wait",f_thread_id_result,"function(:mixed)",0);
  add_function("status",f_thread_id_status,"function(:int)",0);
#ifdef DEBUG
  set_gc_mark_callback(thread_was_marked);
#endif
  set_init_callback(init_thread_obj);
  set_exit_callback(exit_thread_obj);
  thread_id_prog=end_program();
  if(!mutex_key)
    fatal("Failed to initialize thread program!\n");

  thread_id=clone_object(thread_id_prog,0);
  SWAP_OUT_THREAD((struct thread_state *)thread_id->storage); /* Init struct */
  ((struct thread_state *)thread_id->storage)->swapped=0;
  ((struct thread_state *)thread_id->storage)->id=th_self();
  thread_table_insert(thread_id);
}

void th_cleanup(void)
{
  if(mutex_key)
  {
    free_program(mutex_key);
    mutex_key=0;
  }

  if(thread_id_prog)
  {
    free_program(thread_id_prog);
    thread_id_prog=0;
  }

  if(thread_id)
  {
    destruct(thread_id);
    free_object(thread_id);
    thread_id=0;
  }
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

static void *farm(void *_a)
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
}

int th_num_idle_farmers()
{
  int q = 0;
  struct farmer *f = farmers;
  while(f) { f = f->neighbour; q++; }
  return q;
}

static int _num_farmers;
int th_num_farmers()
{
  return _num_farmers;
}

static struct farmer *new_farmer(void (*fun)(void *), void *args)
{
  struct farmer *me = malloc(sizeof(struct farmer));
  _num_farmers++;
  me->neighbour = 0;
  me->field = args;
  me->harvest = fun;
  co_init( &me->harvest_moon );
#ifdef UNIX_THREADS
  thr_create(NULL,8192,farm,(void *)me,THR_DAEMON|THR_DETACHED|THR_BOUND,0);
#else
  th_create_small(&me->me, farm, me);
#endif
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
#endif
