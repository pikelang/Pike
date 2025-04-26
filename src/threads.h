/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef THREADS_H
#define THREADS_H

#include "object.h"
#include "pike_error.h"
#include "interpret.h"
#include "pike_threadlib.h"

#ifdef PIKE_THREADS

#include "pike_rusage.h"

struct svalue;
struct pike_frame;

/* Status values */
#define THREAD_NOT_STARTED -1
#define THREAD_RUNNING 0
#define THREAD_EXITED 1
#define THREAD_ABORTED 2

/* Thread flags */
#define THREAD_FLAG_TERM	1	/* Pending termination. */
#define THREAD_FLAG_INTR	2	/* Pending interrupt. */

#define THREAD_FLAG_SIGNAL_MASK	3	/* All of the above. */

#define THREAD_FLAG_INHIBIT	4	/* Inhibit signals. */

/* Debug flags */
#define THREAD_DEBUG_LOOSE  1	/* Thread is not bound to the interpreter. */

struct thread_state {
  struct Pike_interpreter_struct state;
  struct object *thread_obj;	/* NOTE: Not ref-counted! */
  struct mapping *thread_locals;
  struct thread_state *hashlink, **backlink;
  struct thread_state *busy_prev, *busy_next;
  struct svalue result;
  COND_T status_change;
  THREAD_T id;
  cpu_time_t interval_start;	/* real_time at THREADS_DISALLOW(). */
#ifdef CPU_TIME_MIGHT_BE_THREAD_LOCAL
  cpu_time_t auto_gc_time;
#endif
  unsigned short waiting;	/* Threads waiting on status_change. */
  unsigned short flags;
  char swapped;			/* Set if thread has been swapped out. */
  signed char status;
#ifdef PIKE_DEBUG
  char debug_flags;
#endif
};


/* Prototypes begin here */
PMOD_EXPORT int low_nt_create_thread(unsigned stack_size,
				     unsigned (TH_STDCALL *func)(void *),
				     void *arg,
				     unsigned *id);
struct thread_starter;
struct thread_local_var;
void low_init_threads_disable(void);
void init_threads_disable(struct object *o);
void exit_threads_disable(struct object *o);
void init_interleave_mutex(IMUTEX_T *im);
void exit_interleave_mutex(IMUTEX_T *im);
void thread_table_init(void);
unsigned INT32 thread_table_hash(THREAD_T *tid);
PMOD_EXPORT void thread_table_insert(struct thread_state *s);
PMOD_EXPORT void thread_table_delete(struct thread_state *s);
PMOD_EXPORT struct thread_state *thread_state_for_id(THREAD_T tid);
PMOD_EXPORT struct object *thread_for_id(THREAD_T tid);
PMOD_EXPORT void f_all_threads(INT32 args);
PMOD_EXPORT int count_pike_threads(void);
PMOD_EXPORT void pike_thread_yield(void);
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
void cleanup_all_other_threads (void);
void th_cleanup(void);
int th_num_idle_farmers(void);
int th_num_farmers(void);
PMOD_EXPORT void th_farm(void (*fun)(void *), void *here);
PMOD_EXPORT void call_with_interpreter(void (*func)(void *ctx), void *ctx);
PMOD_EXPORT void enable_external_threads(void);
PMOD_EXPORT void disable_external_threads(void);

#else

#define pike_thread_yield()
PMOD_EXPORT void call_with_interpreter(void (*func)(void *ctx), void *ctx);
#define enable_external_threads()
#define disable_external_threads()

#endif
/* Prototypes end here */


/* for compatibility */
#include "interpret.h"

#endif /* THREADS_H */
