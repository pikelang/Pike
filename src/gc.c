/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

#include "global.h"

#ifdef GC2
#include "gc.h"
#include "main.h"

/* Run garbage collect approximate every time we have
 * 20 percent of all arrays, objects and programs is
 * garbage.
 */

#define GC_CONST 20
#define MIN_ALLOC_THRESHOLD 1000
#define MULTIPLIER 0.9

void *gc_ptr;
INT32 gc_refs;
INT32 num_objects;
INT32 num_allocs;
INT32 alloc_threshold = MIN_ALLOC_THRESHOLD;

static double objects_alloced;
static double objects_freed;

void do_gc()
{
  double tmp;
  INT32 tmp2;

  tmp2=num_objects;

#ifdef DEBUG
  if(t_flag)
    fprintf(stderr,"Garbage collecting ... ");
#endif

  objects_alloced*=MULTIPLIER;
  objects_alloced += (double) num_allocs;
  
  objects_freed*=MULTIPLIER;
  objects_freed += (double) num_objects;
  
  gc_clear_array_marks();
  gc_clear_object_marks();
  gc_clear_program_marks();
  
  gc_check_all_arrays();
  gc_check_all_programs();
  gc_check_all_objects();
  
  objects_freed -= (double) num_objects;

  tmp=(double)num_objects;
  tmp=tmp * GC_CONST/100.0 * (objects_alloced+1.0) / (objects_freed+1.0);
  
  if((int)tmp < alloc_threshold + num_allocs)
  {
    alloc_threshold=(int)tmp;
  }else{
    alloc_threshold+=num_allocs;
  }

  if(alloc_threshold < MIN_ALLOC_THRESHOLD)
    alloc_threshold = MIN_ALLOC_THRESHOLD;
  num_allocs=0;

#ifdef DEBUG
  if(t_flag)
    fprintf(stderr,"done (freed %ld of %ld objects).\n",
	    (long)(tmp2-num_objects),(long)tmp2);
#endif
}
#endif










