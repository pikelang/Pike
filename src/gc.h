/*
 * $Id: gc.h,v 1.17 1998/04/06 04:25:27 hubbe Exp $
 */
#ifndef GC_H
#define GC_H

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

/* Prototypes begin here */
struct callback *add_gc_callback(callback_func call,
				 void *arg,
				 callback_func free_func);
struct marker;
struct marker_chunk;
void dump_gc_info(void);
TYPE_T attempt_to_identify(void *something);
void describe_location(void *memblock, TYPE_T type, void *location);
void debug_gc_xmark_svalues(struct svalue *s, int num, char *fromwhere);
TYPE_FIELD debug_gc_check_svalues(struct svalue *s, int num, TYPE_T t, void *data);
void debug_gc_check_short_svalue(union anything *u, TYPE_T type, TYPE_T t, void *data);
int debug_gc_check(void *x, TYPE_T t, void *data);
void describe_something(void *a, int t, int dm);
void describe(void *x);
INT32 gc_check(void *a);
void locate_references(void *a);
int gc_is_referenced(void *a);
int gc_external_mark(void *a);
int gc_mark(void *a);
int gc_do_free(void *a);
void do_gc(void);
/* Prototypes end here */

#ifdef DEBUG
#define GC_FREE() do { num_objects-- ; if(num_objects < 0) fatal("Panic!! less than zero objects!\n"); }while(0)
#else
#define debug_gc_check_svalues(S,N,T,V) gc_check_svalues(S,N)
#define debug_gc_check_short_svalue(S,N,T,V) gc_check_short_svalue(S,N)
#define debug_gc_xmark_svalue(S,N,X) gc_xmark_svalue(S,N)
#define debug_gc_check(VP,T,V) gc_check(VP)
#define GC_FREE() do { num_objects-- ; }while(0)
#endif


#endif
