#ifndef GC_H
#define GC_H

#ifdef GC2

#include "global.h"
#include "callback.h"

extern INT32 num_objects;
extern INT32 num_allocs;
extern INT32 alloc_threshold;

extern struct callback *gc_evaluator_callback;
extern struct callback_list evaluator_callbacks;
#ifdef DEBUG
extern void *gc_svalue_location;
#endif

#define ADD_GC_CALLBACK() gc_evaluator_callback=add_to_callback(&evaluator_callbacks,(callback_func)do_gc,0,0)

#ifdef ALWAYS_GC
#define GC_ALLOC() do{ num_objects++; num_allocs++;  if(!gc_evaluator_callback) ADD_GC_CALLBACK(); } while(0)
#else
#define GC_ALLOC() do{ num_objects++; num_allocs++;  if(num_allocs == alloc_threshold && !gc_evaluator_callback) ADD_GC_CALLBACK(); } while(0)
#endif

#ifdef DEBUG
#define GC_FREE() do { num_objects-- ; if(num_objects < 0) fatal("Panic!! less than zero objects!\n"); }while(0)
#else
#define debug_gc_check_svalues(S,N,T,V) gc_check_svalues(S,N)
#define debug_gc_check_short_svalues(S,N,T,V) gc_check_short_svalues(S,N)
#define GC_FREE() do { num_objects-- ; }while(0)
#endif

/* Prototypes begin here */
struct callback *add_gc_callback(callback_func call,
				 void *arg,
				 callback_func free_func);
struct marker;
struct marker_chunk;
TYPE_T attempt_to_identify(void *something);
TYPE_FIELD debug_gc_check_svalues(struct svalue *s, int num, TYPE_T t, void *data);
void debug_gc_check_short_svalue(union anything *u, TYPE_T type, TYPE_T t, void *data);
void describe_something(void *a, TYPE_T t);
INT32 gc_check(void *a);
int gc_is_referenced(void *a);
int gc_external_mark(void *a);
int gc_mark(void *a);
int gc_do_free(void *a);
void do_gc(void);
/* Prototypes end here */

#else

#define GC_ALLOC()
#define GC_FREE()
#define do_gc()

#endif
#endif
