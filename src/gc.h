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

#define ADD_GC_CALLBACK() gc_evaluator_callback=add_to_callback(&evaluator_callbacks,(callback_func)do_gc,0,0)

#ifdef ALWAYS_GC
#define GC_ALLOC() do{ num_objects++; num_allocs++;  if(!gc_evaluator_callback) ADD_GC_CALLBACK(); } while(0)
#else
#define GC_ALLOC() do{ num_objects++; num_allocs++;  if(num_allocs == alloc_threshold && !gc_evaluator_callback) ADD_GC_CALLBACK(); } while(0)
#endif

#ifdef DEBUG
#define GC_FREE() do { num_objects-- ; if(num_objects < 0) fatal("Panic!! less than zero objects!\n"); }while(0)
#else
#define GC_FREE() do { num_objects-- ; }while(0)
#endif

/* Prototypes begin here */
struct callback *add_gc_callback(callback_func call,
				 void *arg,
				 callback_func free_func);
struct marker;
struct marker_chunk;
INT32 gc_check(void *a);
int gc_is_referenced(void *a);
int gc_mark(void *a);
int gc_do_free(void *a);
void do_gc();
/* Prototypes end here */

#else

#define GC_ALLOC()
#define GC_FREE()
#define do_gc()

#endif
#endif
