#ifndef GC_H
#define GC_H

#ifdef GC2

#include "types.h"

extern INT32 num_objects;
extern INT32 num_allocs;
extern INT32 alloc_threshold;

#ifdef ALWAYS_GC
#define GC_ALLOC() do{ num_objects++; ++num_allocs; do_gc(); } while(0)
#else
#define GC_ALLOC() do{ num_objects++; if(++num_allocs > alloc_threshold) do_gc(); } while(0)
#endif
#define GC_FREE() num_objects--

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
