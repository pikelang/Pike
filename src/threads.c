#include "global.h"
#include "threads.h"
#include "array.h"
#include "object.h"

int num_threads = 1;
int threads_disabled = 0;

#ifdef _REENTRANT

MUTEX_T interpreter_lock;
struct program *mutex_key = 0;
pthread_attr_t pattr;

void *new_thread_func(void * data)
{
  JMP_BUF back;
  struct array *foo;
  INT32 args;
  foo=(struct array *)data;
  args=foo->size;
  mt_lock( & interpreter_lock);
  init_interpreter();


  if(SETJMP(back))
  {
    exit_on_error="Error in handle_error in master object!\nPrevious error:";
    assign_svalue_no_free(sp++, & throw_value);
    APPLY_MASTER("handle_error", 1);
    pop_stack();
    automatic_fatal=0;
  } else {
    push_array_items(foo);
    f_call_function(args);
  }

  UNSETJMP(back);

  cleanup_interpret();
  mt_unlock(& interpreter_lock);
  num_threads--;
  th_exit(0);
}

void f_thread_create(INT32 args)
{
  pthread_t dummy;
  int tmp;
  struct array *a=aggregate_array(args);
  num_threads++;
  tmp=th_create(&dummy,new_thread_func,a);
  if(tmp) num_threads--;
  push_int(tmp);
}

void th_init()
{
  thr_setconcurrency(9);
  mt_lock( & interpreter_lock);
  pthread_attr_init(&pattr);
  pthread_attr_setstacksize(&pattr, 2 * 1024 * 1204);
  pthread_attr_setdetachstate(&pattr, PTHREAD_CREATE_DETACHED);

  add_efun("thread_create",f_thread_create,"function(mixed ...:int)",OPT_SIDE_EFFECT);
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

static MUTEX_T mutex_kluge;

#define OB2KEY(X) ((struct key_storage *)((X)->storage))

void f_mutex_lock(INT32 args)
{
  struct mutex_storage  *m;
  struct object *o;

  pop_n_elems(args);
  m=THIS_MUTEX;
  o=clone(mutex_key,0);
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

  o=clone(mutex_key,0);
  m=THIS_MUTEX;
  mt_lock(& mutex_kluge);
  THREADS_ALLOW();
  if(!m->key)
  {
    OB2KEY(o)->mut=THIS_MUTEX;
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

void th_init_programs()
{
  start_new_program();
  add_storage(sizeof(struct mutex_storage));
  add_function("lock",f_mutex_lock,"function(:object)",0);
  add_function("trylock",f_mutex_trylock,"function(:object)",0);
  set_init_callback(init_mutex_obj);
  set_exit_callback(exit_mutex_obj);
  end_c_program("/precompiled/mutex");

  start_new_program();
  add_storage(sizeof(struct key_storage));
  set_init_callback(init_mutex_key_obj);
  set_exit_callback(exit_mutex_key_obj);
  mutex_key=end_c_program("/precompiled/mutex_key");

  start_new_program();
  add_storage(sizeof(COND_T));
  add_function("wait",f_cond_wait,"function(void|object:void)",0);
  add_function("signal",f_cond_signal,"function(:void)",0);
  add_function("broadcast",f_cond_broadcast,"function(:void)",0);
  set_init_callback(init_cond_obj);
  set_exit_callback(exit_cond_obj);
  end_c_program("/precompiled/condition");
}

#endif
