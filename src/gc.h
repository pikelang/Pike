#ifndef GC_H
#define GC_H

#ifdef GC2

#include "types.h"

extern INT32 num_objects;
extern INT32 num_allocs;
extern INT32 alloc_threshold;

#define GC_ALLOC() do{ num_objects++; num_allocs++;  } while(0)
#ifdef DEBUG
#define GC_FREE() do { num_objects-- ; if(num_objects < 0) fatal("Panic!! less than zero objects!\n"); }while(0)
#else
#define GC_FREE() do { num_objects-- ; }while(0)
#endif

#ifdef ALWAYS_GC
#define CHECK_FOR_GC(); do { do_gc(); } while(0)
#else
#define CHECK_FOR_GC(); do { if(num_allocs > alloc_threshold) do_gc(); } while(0)
#endif

/* Prototypes begin here */
struct marker;
struct marker_chunk;
void gc_check(void *a);
int gc_is_referenced(void *a);
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
