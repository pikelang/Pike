#ifndef GC_H
#define GC_H

#ifdef GC2

#include "types.h"

extern void *gc_ptr;
extern INT32 gc_refs;
extern INT32 num_objects;
extern INT32 num_allocs;
extern INT32 alloc_threshold;

#define GC_ALLOC() do{ num_objects++; if(++num_allocs > alloc_threshold) do_gc(); } while(0);
#define GC_FREE() num_objects--
#define GC_MARK 1

#else

#define GC_ALLOC()
#define GC_FREE()
#define do_gc()

#endif

/* Prototypes begin here */
void do_gc();
/* Prototypes end here */

#endif
