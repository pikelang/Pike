#include "global.h"
#include "threads.h"
#include "array.h"
#include "object.h"

int num_threads = 1;
int threads_disabled = 0;

#ifdef _REENTRANT

MUTEX_T interpreter_lock, compiler_lock;
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
  mt_lock( & interpreter_lock);
  pthread_attr_init(&pattr);
  pthread_attr_setstacksize(&pattr, 2 * 1024 * 1204);
  pthread_attr_setdetachstate(&pattr, PTHREAD_CREATE_DETACHED);

  add_efun("thread_create",f_thread_create,"function(mixed ...:int)",OPT_SIDE_EFFECT);
}


#define THIS_MUTEX ((MUTEX_T *)(fp->current_storage))

void f_mutex_lock(INT32 args)
{
  MUTEX_T *m;
  struct object *o;
  pop_n_elems(args);
  m=THIS_MUTEX;
  THREADS_ALLOW();
  mt_lock(m);
  THREADS_DISALLOW();
  o=clone(mutex_key,0);
  ((struct object **)(o->storage))[0]=fp->current_object;
  fp->current_object->refs++;
  push_object(o);
}

void f_mutex_trylock(INT32 args)
{
  MUTEX_T *m;
  int i;
  pop_n_elems(args);

  m=THIS_MUTEX;
  THREADS_ALLOW();
  i=mt_lock(m);
  THREADS_DISALLOW();
  
  if(i)
  {
    struct object *o;
    o=clone(mutex_key,0);
    ((struct object **)o->storage)[0]=fp->current_object;
    fp->current_object->refs++;
    push_object(o);
  } else {
    push_int(0);
  }
}

void init_mutex_obj(struct object *o) { mt_init(THIS_MUTEX); }
void exit_mutex_obj(struct object *o) { mt_destroy(THIS_MUTEX); }

#define THIS_KEY (*(struct object **)fp->current_storage)
void init_mutex_key_obj(struct object *o) { THIS_KEY=0; }

void exit_mutex_key_obj(struct object *o)
{
  if(THIS_KEY)
  {
    mt_unlock((MUTEX_T *)THIS_KEY->storage);
    init_mutex_key_obj(o);
  }
}

#define THIS_COND ((COND_T *)(fp->current_storage))
void f_cond_wait(INT32 args)
{
  COND_T *c;

  if(args > 1) pop_n_elems(args - 1);
  
  if(sp[-1].type != T_OBJECT)
    error("Bad argument 1 to condition->wait()\n");

  if(sp[-1].u.object->prog)
  {
    if(sp[-1].u.object->prog != mutex_key)
      error("Bad argument 1 to condition->wait()\n");

    destruct(sp[-1].u.object);
    pop_stack();
  }

  c=THIS_COND;
  THREADS_ALLOW();
  co_wait(c,0);
  THREADS_DISALLOW();
}

void f_cond_signal(INT32 args) { pop_n_elems(args); co_signal(THIS_COND); }
void f_cond_broadcast(INT32 args) { pop_n_elems(args); co_broadcast(THIS_COND); }
void init_cond_obj(struct object *o) { co_init(THIS_COND); }
void exit_cond_obj(struct object *o) { co_destroy(THIS_COND); }

void th_init_programs()
{
  start_new_program();
  add_storage(sizeof(MUTEX_T));
  add_function("lock",f_mutex_lock,"function(:object)",0);
  add_function("trylock",f_mutex_trylock,"function(:object)",0);
  set_init_callback(init_mutex_obj);
  set_exit_callback(exit_mutex_obj);
  end_c_program("/precompiled/mutex");

  start_new_program();
  add_storage(sizeof(struct object *));
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
