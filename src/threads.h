#ifndef THREADS_H
#define THREADS_H

#include "machine.h"
#include "interpret.h"
#include "error.h"

#ifdef _REENTRANT

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#undef HAVE_PTHREAD_H
#endif

extern int num_threads;
struct object;
extern struct object *thread_id;

#define MUTEX_T pthread_mutex_t
#define mt_init(X) pthread_mutex_init((X),0)
#define mt_lock(X) pthread_mutex_lock(X)
#define mt_trylock(X) pthread_mutex_trylock(X)
#define mt_unlock(X) pthread_mutex_unlock(X)
#define mt_destroy(X) pthread_mutex_destroy(X)
#define DEFINE_MUTEX(X) MUTEX_T X

extern MUTEX_T interpreter_lock;

#define th_create(ID,fun,arg) pthread_create(ID,&pattr,fun,arg)
#define th_exit(foo) pthread_exit(foo)

#define COND_T pthread_cond_t
#define co_init(X) pthread_cond_init((X), 0)
#define co_wait(COND, MUTEX) pthread_cond_wait((COND), (MUTEX))
#define co_signal(X) pthread_cond_signal(X)
#define co_broadcast(X) pthread_cond_broadcast(X)
#define co_destroy(X) pthread_cond_destroy(X)

struct svalue;
struct frame;

struct thread_state {
  int swapped;
  struct svalue *sp,*evaluator_stack;
  struct svalue **mark_sp,**mark_stack;
  struct frame *fp;
  int evaluator_stack_malloced;
  int mark_stack_malloced;
  JMP_BUF *recoveries;
  struct object * thread_id;
};

#define THREADS_ALLOW() \
  do {\
     struct thread_state _tmp; \
     _tmp.swapped=0; \
     if(num_threads > 1 && !threads_disabled) { \
       _tmp.swapped=1; \
       _tmp.sp=sp; \
       _tmp.evaluator_stack=evaluator_stack; \
       _tmp.mark_sp=mark_sp; \
       _tmp.mark_stack=mark_stack; \
       _tmp.fp=fp; \
       _tmp.recoveries=recoveries; \
       _tmp.evaluator_stack_malloced=evaluator_stack_malloced; \
       _tmp.mark_stack_malloced=mark_stack_malloced; \
       _tmp.thread_id = thread_id; \
       mt_unlock(& interpreter_lock); \
     }

#define THREADS_DISALLOW() \
     if(_tmp.swapped) { \
       mt_lock(& interpreter_lock); \
       sp=_tmp.sp; \
       evaluator_stack=_tmp.evaluator_stack; \
       mark_sp=_tmp.mark_sp; \
       mark_stack=_tmp.mark_stack; \
       fp=_tmp.fp; \
       recoveries=_tmp.recoveries; \
       evaluator_stack_malloced=_tmp.evaluator_stack_malloced; \
       mark_stack_malloced=_tmp.mark_stack_malloced; \
       thread_id = _tmp.thread_id; \
     } \
   } while(0)

/* Prototypes begin here */
struct thread_starter;
void *new_thread_func(void * data);
void f_thread_create(INT32 args);
void f_this_thread(INT32 args);
void th_init();
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
void th_init_programs();
void th_cleanup();
/* Prototypes end here */

#else
#define DEFINE_MUTEX(X)
#define mt_init(X)
#define mt_lock(X)
#define mt_unlock(X)
#define mt_destroy(X)
#define THREADS_ALLOW()
#define THREADS_DISALLOW()
#define th_init()
#define th_cleanup()
#define th_init_programs()
#endif


extern int threads_disabled;
#endif
