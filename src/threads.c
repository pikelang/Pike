#include "global.h"
RCSID("$Id: threads.c,v 1.23.2.2 1997/05/19 09:04:57 hubbe Exp $");

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


struct object *thread_id;
static struct callback *threads_evaluator_callback=0;

MUTEX_T interpreter_lock = PTHREAD_MUTEX_INITIALIZER;
struct program *mutex_key = 0;
struct program *thread_id_prog = 0;
#ifdef POSIX_THREADS
pthread_attr_t pattr;
#endif

struct thread_starter
{
  struct object *id;
  struct array *args;
};

struct thread_id {
  int status;
  COND_T status_change;
};

static int thread_id_result_variable;

static MUTEX_T thread_id_kluge = PTHREAD_MUTEX_INITIALIZER;

#define THREAD_UNKNOWN 0
#define THREAD_RUNNING 1
#define THREAD_EXITED 2

static void check_threads(struct callback *cb, void *arg, void * arg2)
{
  THREADS_ALLOW();

  /* Allow other threads to run */

  THREADS_DISALLOW();
}

#define THIS_THREAD ((struct thread_id *)fp->current_storage)
#define THREAD_INFO ((struct thread_id *)thread_id->storage)

static void init_thread_id(struct object *o)
{
  THIS_THREAD->status=THREAD_UNKNOWN;
  co_init(& THIS_THREAD->status_change);
}

static void exit_thread_id(struct object *o)
{
  co_destroy(& THIS_THREAD->status_change);
}

static void f_thread_id_status(INT32 args)
{
  pop_n_elems(args);
  push_int(THIS_THREAD->status);
}

static void f_thread_id_result(INT32 args)
{
  struct thread_id *th=THIS_THREAD;
  THREADS_ALLOW();
  mt_lock(&thread_id_kluge);
  while(th->status != THREAD_EXITED)
    co_wait(&th->status_change, &thread_id_kluge);
  mt_unlock(&thread_id_kluge);
  THREADS_DISALLOW();

  low_object_index_no_free(sp,
			   fp->current_object, 
			   thread_id_result_variable);
  sp++;
}

void *new_thread_func(void * data)
{
/*  static int dbt;*/
  struct thread_starter arg = *(struct thread_starter *)data;
  JMP_BUF back;
  INT32 tmp;

/*  fprintf(stderr, "Thread create[%d]...",dbt++);*/
  if((tmp=mt_lock( & interpreter_lock)))
    fatal("Failed to lock interpreter, errno %d\n",tmp);
/*  fprintf(stderr,"Created[%d]...",dbt);*/
  free((char *)data); /* Moved by per, to avoid some bugs.... */
  init_interpreter();

  thread_id=arg.id;
  THREAD_INFO->status=THREAD_RUNNING;

  if(SETJMP(back))
  {
    ONERROR tmp;
    SET_ONERROR(tmp,exit_on_error,"Error in handle_error in master object!");
    assign_svalue_no_free(sp++, & throw_value);
    APPLY_MASTER("handle_error", 1);
    pop_stack();
    UNSET_ONERROR(tmp);
  } else {
    INT32 args=arg.args->size;
    push_array_items(arg.args);
    arg.args=0;
    f_call_function(args);

    /* copy return value to the thread_id here */
    object_low_set_index(thread_id,
			 thread_id_result_variable,
			 sp-1);
    pop_stack();
  }

  UNSETJMP(back);

  THREAD_INFO->status=THREAD_EXITED;
  co_signal(& THREAD_INFO->status_change);

  free_object(thread_id);
  thread_id=0;

  cleanup_interpret();
  num_threads--;
  if(!num_threads)
  {
    remove_callback(threads_evaluator_callback);
    threads_evaluator_callback=0;
  }
/*  fprintf(stderr,"Done[%d]\n",dbt--);*/
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
  arg->id->refs++;

  tmp=th_create(&dummy,new_thread_func,arg);

  if(!tmp)
  {
    num_threads++;

    if(num_threads == 1 && !threads_evaluator_callback)
    {
      threads_evaluator_callback=add_to_callback(&evaluator_callbacks,
						 check_threads, 0,0);
    }
#ifdef UNIX_THREADS
    if((num_lwps==1) || num_threads/3 > num_lwps)
      th_setconcurrency(++num_lwps);
#endif
    push_object(arg->id);
  } else {
    free_object(arg->id);
    free_object(arg->id);
    free_array(arg->args);
    free((char *)arg);
    push_int(0);
  }
}

void f_thread_set_concurrency(INT32 args)
{
  int c=1;
  if(args) c=sp[-args].u.integer;
  else error("No argument to thread_set_concurrency(int concurrency);\n");
  pop_n_elems(args);
  num_threads=c;
  th_setconcurrency(c);
}

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
  int initialized;
};

static MUTEX_T mutex_kluge = PTHREAD_MUTEX_INITIALIZER;

#define OB2KEY(X) ((struct key_storage *)((X)->storage))

void f_mutex_lock(INT32 args)
{
  struct mutex_storage  *m;
  struct object *o;

  pop_n_elems(args);
  m=THIS_MUTEX;
  o=clone_object(mutex_key,0);
  mt_lock(& mutex_kluge);
  THREADS_ALLOW();
  while(m->key) co_wait(& m->condition, & mutex_kluge);
  OB2KEY(o)->mut=m;
  m->key=o;
  mt_unlock(&mutex_kluge);
  THREADS_DISALLOW();
  push_object(o);
}

void f_mutex_trylock(INT32 args)
{
  struct mutex_storage  *m;
  struct object *o;
  int i=0;
  pop_n_elems(args);

  o=clone_object(mutex_key,0);
  m=THIS_MUTEX;
  mt_lock(& mutex_kluge);
  THREADS_ALLOW();
  if(!m->key)
  {
    OB2KEY(o)->mut=m;
    m->key=o;
    i=1;
  }
  mt_unlock(&mutex_kluge);
  THREADS_DISALLOW();
  
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
  if(THIS_MUTEX->key) destruct(THIS_MUTEX->key);
  co_destroy(& THIS_MUTEX->condition);
}

#define THIS_KEY ((struct key_storage *)(fp->current_storage))
void init_mutex_key_obj(struct object *o)
{
  THIS_KEY->mut=0;
  THIS_KEY->initialized=1;
}

void exit_mutex_key_obj(struct object *o)
{
  mt_lock(& mutex_kluge);
  if(THIS_KEY->mut)
  {
#ifdef DEBUG
    if(THIS_KEY->mut->key != o)
      fatal("Mutex unlock from wrong key %p != %p!\n",THIS_KEY->mut->key,o);
#endif
    THIS_KEY->mut->key=0;
    co_signal(& THIS_KEY->mut->condition);
    THIS_KEY->mut=0;
    THIS_KEY->initialized=0;
  }
  mt_unlock(& mutex_kluge);
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
    
    mt_lock(&mutex_kluge);
    mut=OB2KEY(key)->mut;
    THREADS_ALLOW();

    /* Unlock mutex */
    mut->key=0;
    OB2KEY(key)->mut=0;
    co_signal(& mut->condition);

    /* Wait and allow mutex operations */
    co_wait(c,&mutex_kluge);

    if(OB2KEY(key)->initialized)
    {
      /* Lock mutex */
      while(mut->key) co_wait(& mut->condition, & mutex_kluge);
      mut->key=key;
      OB2KEY(key)->mut=mut;
    }
    mt_unlock(&mutex_kluge);
    THREADS_DISALLOW();
    pop_stack();
  } else {
    THREADS_ALLOW();
    co_wait(c, 0);
    THREADS_DISALLOW();
  }
}

void f_cond_signal(INT32 args) { pop_n_elems(args); co_signal(THIS_COND); }
void f_cond_broadcast(INT32 args) { pop_n_elems(args); co_broadcast(THIS_COND); }
void init_cond_obj(struct object *o) { co_init(THIS_COND); }
void exit_cond_obj(struct object *o) { co_destroy(THIS_COND); }

void th_init()
{
  struct program *tmp;

#ifdef SGI_SPROC_THREADS
#error /* Need to specify a filename */
  us_cookie = usinit("");
#endif /* SGI_SPROC_THREADS */

  mt_lock( & interpreter_lock);
#ifdef POSIX_THREADS
  pthread_attr_init(&pattr);
#ifdef HAVE_PTHREAD_ATTR_SETSTACKSIZE
  pthread_attr_setstacksize(&pattr, 2 * 1024 * 1204);
#endif
  pthread_attr_setdetachstate(&pattr, PTHREAD_CREATE_DETACHED);
#endif

  add_efun("thread_create",f_thread_create,"function(mixed ...:object)",
           OPT_SIDE_EFFECT);
#ifdef UNIX_THREADS
  add_efun("thread_set_concurrency",f_thread_set_concurrency,
	   "function(int:void)", OPT_SIDE_EFFECT);
#endif
  add_efun("this_thread",f_this_thread,"function(:object)",
           OPT_EXTERNAL_DEPEND);

  start_new_program();
  add_storage(sizeof(struct mutex_storage));
  add_function("lock",f_mutex_lock,"function(:object)",0);
  add_function("trylock",f_mutex_trylock,"function(:object)",0);
  set_init_callback(init_mutex_obj);
  set_exit_callback(exit_mutex_obj);
  end_class("mutex", 0);

  start_new_program();
  add_storage(sizeof(struct key_storage));
  set_init_callback(init_mutex_key_obj);
  set_exit_callback(exit_mutex_key_obj);
  mutex_key=end_program();
  if(!mutex_key)
    fatal("Failed to initialize mutex_key program!\n");

  start_new_program();
  add_storage(sizeof(COND_T));
  add_function("wait",f_cond_wait,"function(void|object:void)",0);
  add_function("signal",f_cond_signal,"function(:void)",0);
  add_function("broadcast",f_cond_broadcast,"function(:void)",0);
  set_init_callback(init_cond_obj);
  set_exit_callback(exit_cond_obj);
  end_class("condition", 0);

  start_new_program();
  add_storage(sizeof(struct thread_id));
  thread_id_result_variable=simple_add_variable("result","mixed",0);
  add_function("wait",f_thread_id_result,"function(:int)",0);
  add_function("status",f_thread_id_status,"function(:int)",0);
  set_init_callback(init_thread_id);
  set_exit_callback(exit_thread_id);
  thread_id_prog=end_program();
  if(!mutex_key)
    fatal("Failed to initialize thread program!\n");

  thread_id=clone_object(thread_id_prog,0);
}

void th_cleanup()
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
    free_object(thread_id);
    thread_id=0;
  }
}

#endif
