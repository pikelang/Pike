/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

#include "global.h"

#ifdef GC2

struct callback *gc_evaluator_callback=0;

#include "array.h"
#include "multiset.h"
#include "mapping.h"
#include "object.h"
#include "program.h"
#include "stralloc.h"

#include "gc.h"
#include "main.h"

/* Run garbage collect approximate every time we have
 * 20 percent of all arrays, objects and programs is
 * garbage.
 */

#define GC_CONST 20
#define MIN_ALLOC_THRESHOLD 1000
#define MAX_ALLOC_THRESHOLD 10000000
#define MULTIPLIER 0.9
#define MARKER_CHUNK_SIZE 1023

INT32 num_objects;
INT32 num_allocs;
INT32 alloc_threshold = MIN_ALLOC_THRESHOLD;

static double objects_alloced;
static double objects_freed;

struct callback_list gc_callbacks;

struct callback *add_gc_callback(callback_func call,
				 void *arg,
				 callback_func free_func)
{
  return add_to_callback(&gc_callbacks, call, arg, free_func);
}

#define GC_REFERENCED 1

struct marker
{
  struct marker *next;
  void *marked;
  INT32 refs;
  INT32 flags;
};

struct marker_chunk
{
  struct marker_chunk *next;
  struct marker markers[MARKER_CHUNK_SIZE];
};

static struct marker_chunk *chunk=0;
static int markers_left_in_chunk=0;

static struct marker *new_marker()
{
  if(!markers_left_in_chunk)
  {
    struct marker_chunk *m;
    m=(struct marker_chunk *)xalloc(sizeof(struct marker_chunk));
    m->next=chunk;
    chunk=m;
    markers_left_in_chunk=MARKER_CHUNK_SIZE;
  }
  markers_left_in_chunk--;

  return chunk->markers + markers_left_in_chunk;
}

static struct marker **hash=0;
static int hashsize=0;

static struct marker *getmark(void *a)
{
  int hashval;
  struct marker *m;

  hashval=((long)a)%hashsize;

  for(m=hash[hashval];m;m=m->next)
    if(m->marked == a)
      return m;

  m=new_marker();
  m->marked=a;
  m->refs=0;
  m->flags=0;
  m->next=hash[hashval];
  hash[hashval]=m;

  return m;
}

#ifdef DEBUG
static void *check_for =0;

static void gdb_gc_stop_here(void *a)
{
  fprintf(stderr,"One ref found.\n");
}
#endif

INT32 gc_check(void *a)
{
#ifdef DEBUG
  if(check_for)
  {
    if(check_for == a)
    {
      gdb_gc_stop_here(a);
    }
    return 0;
  }
#endif
  return getmark(a)->refs++;
}

int gc_is_referenced(void *a)
{
  struct marker *m;
  m=getmark(a);
#ifdef DEBUG
  if(m->refs > *(INT32 *)a)
  {
    check_for=a;

    gc_check_all_arrays();
    gc_check_all_multisets();
    gc_check_all_mappings();
    gc_check_all_programs();
    gc_check_all_objects();
    call_callback(& gc_callbacks, (void *)0);

    check_for=0;
    fatal("Ref counts are totally wrong!!!\n");
  }
#endif
  return m->refs < *(INT32 *)a;
}

int gc_mark(void *a)
{
  struct marker *m;
  m=getmark(a);

  if(m->flags & GC_REFERENCED)
  {
    return 0;
  }else{
    m->flags |= GC_REFERENCED;
    return 1;
  }
}

int gc_do_free(void *a)
{
  struct marker *m;
  m=getmark(a);
  return !(m->flags & GC_REFERENCED);
}

/* Not all of these are primes, but they should be adequate */
static INT32 hashprimes[] =
{
  31,        /* ~ 2^0  = 1 */
  31,        /* ~ 2^1  = 2 */
  31,        /* ~ 2^2  = 4 */
  31,        /* ~ 2^3  = 8 */
  31,        /* ~ 2^4  = 16 */
  31,        /* ~ 2^5  = 32 */
  61,        /* ~ 2^6  = 64 */
  127,       /* ~ 2^7  = 128 */
  251,       /* ~ 2^8  = 256 */
  541,       /* ~ 2^9  = 512 */
  1151,      /* ~ 2^10 = 1024 */
  2111,      /* ~ 2^11 = 2048 */
  4327,      /* ~ 2^12 = 4096 */
  8803,      /* ~ 2^13 = 8192 */
  17903,     /* ~ 2^14 = 16384 */
  32321,     /* ~ 2^15 = 32768 */
  65599,     /* ~ 2^16 = 65536 */
  133153,    /* ~ 2^17 = 131072 */
  270001,    /* ~ 2^18 = 264144 */
  547453,    /* ~ 2^19 = 524288 */
  1109891,   /* ~ 2^20 = 1048576 */
  2000143,   /* ~ 2^21 = 2097152 */
  4561877,   /* ~ 2^22 = 4194304 */
  9248339,   /* ~ 2^23 = 8388608 */
  16777215,  /* ~ 2^24 = 16777216 */
  33554431,  /* ~ 2^25 = 33554432 */
  67108863,  /* ~ 2^26 = 67108864 */
  134217727, /* ~ 2^27 = 134217728 */
  268435455, /* ~ 2^28 = 268435456 */
  536870911, /* ~ 2^29 = 536870912 */
  1073741823,/* ~ 2^30 = 1073741824 */
  2147483647,/* ~ 2^31 = 2147483648 */
};

void do_gc()
{
  static int in_gc = 0;
  double tmp;
  INT32 tmp2;
  struct marker_chunk *m;

  if(in_gc) return;
  in_gc=1;

  if(gc_evaluator_callback)
  {
    remove_callback(gc_evaluator_callback);
    gc_evaluator_callback=0;
  }

  tmp2=num_objects;

#ifdef DEBUG
  if(t_flag)
    fprintf(stderr,"Garbage collecting ... ");
  if(num_objects < 0)
    fatal("Panic, less than zero objects!\n");
#endif

  objects_alloced*=MULTIPLIER;
  objects_alloced += (double) num_allocs;
  
  objects_freed*=MULTIPLIER;
  objects_freed += (double) num_objects;


  /* init hash , hashsize will be a prime between num_objects/8 and
   * num_objects/4, this will assure that no re-hashing is needed.
   */
  hashsize=my_log2(num_objects);

  if(!d_flag) hashsize-=2;

  if(hashsize<0) hashsize=0;
  hashsize=hashprimes[hashsize];
  hash=(struct marker **)xalloc(sizeof(struct marker **)*hashsize);
  MEMSET((char *)hash,0,sizeof(struct marker **)*hashsize);
  markers_left_in_chunk=0;
  
  gc_check_all_arrays();
  gc_check_all_multisets();
  gc_check_all_mappings();
  gc_check_all_programs();
  gc_check_all_objects();
  call_callback(& gc_callbacks, (void *)0);

  gc_mark_all_arrays();
  gc_mark_all_multisets();
  gc_mark_all_mappings();
  gc_mark_all_programs();
  gc_mark_all_objects();

  if(d_flag)
    gc_mark_all_strings();

  gc_free_all_unreferenced_arrays();
  gc_free_all_unreferenced_multisets();
  gc_free_all_unreferenced_mappings();
  gc_free_all_unreferenced_programs();
  gc_free_all_unreferenced_objects();


  /* Free hash table */
  free((char *)hash);
  while(m=chunk)
  {
    chunk=m->next;
    free((char *)m);
  }

  destruct_objects_to_destruct();
  
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
  if(alloc_threshold > MAX_ALLOC_THRESHOLD)
    alloc_threshold = MAX_ALLOC_THRESHOLD;
  num_allocs=0;

#ifdef DEBUG
  if(t_flag)
    fprintf(stderr,"done (freed %ld of %ld objects).\n",
	    (long)(tmp2-num_objects),(long)tmp2);
#endif

#ifdef ALWAYS_GC
  ADD_GC_CALLBACK();
#else
  if(d_flag > 3) ADD_GC_CALLBACK();
#endif
  in_gc=0;
}

#endif

