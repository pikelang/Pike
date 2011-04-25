/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#ifndef THREADS_H
#define THREADS_H

#include "machine.h"
#include "object.h"
#include "pike_error.h"
#include "interpret.h"
#include "pike_threadlib.h"

#ifdef PIKE_THREADS

struct svalue;
struct pike_frame;

extern PIKE_MUTEX_T interleave_lock;

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


/* Prototypes begin here */
int low_nt_create_thread(unsigned stack_size,
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

#endif


/* for compatibility */
#include "interpret.h"

#endif /* THREADS_H */
