#ifndef GC_H
#define GC_H

#ifdef GC2

#include "types.h"

extern INT32 num_objects;
extern INT32 num_allocs;
extern INT32 alloc_threshold;

#define GC_ALLOC() do{ num_objects++; if(++num_allocs > alloc_threshold) do_gc(); } while(0);
#define GC_FREE() num_objects--

#else

#define GC_ALLOC()
#define GC_FREE()
#define do_gc()

#endif

/* Prototypes begin here */
struct marker;
struct marker_chunk;
void gc_check(void *a);
int gc_is_referenced(void *a);
int gc_mark(void *a);
int gc_do_free(void *a);
void do_gc();
/* Prototypes end here */

#endif
