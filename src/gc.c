/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

#include "global.h"

struct callback *gc_evaluator_callback=0;

#include "array.h"
#include "multiset.h"
#include "mapping.h"
#include "object.h"
#include "program.h"
#include "stralloc.h"
#include "stuff.h"
#include "error.h"
#include "pike_memory.h"
#include "pike_macros.h"
#include "pike_types.h"
#include "time_stuff.h"
#include "constants.h"
#include "block_alloc.h"

#include "gc.h"
#include "main.h"
#include <math.h>

RCSID("$Id: gc.c,v 1.42 1999/05/02 08:11:41 hubbe Exp $");

/* Run garbage collect approximate every time we have
 * 20 percent of all arrays, objects and programs is
 * garbage.
 */

#define GC_CONST 20
#define MIN_ALLOC_THRESHOLD 1000
#define MAX_ALLOC_THRESHOLD 10000000
#define MULTIPLIER 0.9
#define MARKER_CHUNK_SIZE 1023

INT32 num_objects =0;
INT32 num_allocs =0;
INT32 alloc_threshold = MIN_ALLOC_THRESHOLD;
static int in_gc = 0;
struct pike_queue gc_mark_queue;

static double objects_alloced = 0.0;
static double objects_freed = 0.0;

struct callback_list gc_callbacks;

struct callback *debug_add_gc_callback(callback_func call,
				 void *arg,
				 callback_func free_func)
{
  return add_to_callback(&gc_callbacks, call, arg, free_func);
}

#define GC_REFERENCED 1
#define GC_XREFERENCED 2

struct marker
{
  INT32 refs;
#ifdef PIKE_DEBUG
  INT32 xrefs;
#endif
  INT32 flags;
  struct marker *next;
  void *data;
};

#undef INIT_BLOCK
#ifdef PIKE_DEBUG
#define INIT_BLOCK(X) (X)->flags=(X)->refs=(X)->xrefs=0
#else
#define INIT_BLOCK(X) (X)->flags=(X)->refs=0
#endif

PTR_HASH_ALLOC(marker,MARKER_CHUNK_SIZE)

#ifdef PIKE_DEBUG

time_t last_gc;

void dump_gc_info(void)
{
  fprintf(stderr,"Current number of objects: %ld\n",(long)num_objects);
  fprintf(stderr,"Objects allocated total  : %ld\n",(long)num_allocs);
  fprintf(stderr," threshold for next gc() : %ld\n",(long)alloc_threshold);
  fprintf(stderr,"Average allocs per gc()  : %f\n",objects_alloced);
  fprintf(stderr,"Average frees per gc()   : %f\n",objects_freed);
  fprintf(stderr,"Second since last gc()   : %ld\n", (long)TIME(0) - (long)last_gc);
  fprintf(stderr,"Projected garbage        : %f\n", objects_freed * (double) num_allocs / (double) alloc_threshold);
  fprintf(stderr,"in_gc                    : %d\n", in_gc);
}

TYPE_T attempt_to_identify(void *something)
{
  struct array *a;
  struct object *o;
  struct program *p;
  struct mapping *m;
  struct multiset *mu;

  a=&empty_array;
  do
  {
    if(a==(struct array *)something) return T_ARRAY;
    a=a->next;
  }while(a!=&empty_array);

  for(o=first_object;o;o=o->next)
    if(o==(struct object *)something)
      return T_OBJECT;

  for(p=first_program;p;p=p->next)
    if(p==(struct program *)something)
      return T_PROGRAM;

  for(m=first_mapping;m;m=m->next)
    if(m==(struct mapping *)something)
      return T_MAPPING;

  for(mu=first_multiset;mu;mu=mu->next)
    if(mu==(struct multiset *)something)
      return T_MULTISET;

  if(safe_debug_findstring((struct pike_string *)something))
    return T_STRING;

  return T_UNKNOWN;
}

static void *check_for =0;
static char *found_where="";
static void *found_in=0;
static int found_in_type=0;
void *gc_svalue_location=0;

void describe_location(void *memblock, TYPE_T type, void *location)
{
  if(!location) return;
  fprintf(stderr,"**Location of (short) svalue: %p\n",location);

  switch(type)
  {
    case T_PROGRAM:
    {
      struct program *p=(struct program *)memblock;
      char *ptr=(char *)location;
      if(ptr >= (char *)p->inherits  && ptr<(char*)(p->inherits+p->num_inherits))
	fprintf(stderr,"**In inherit block.\n");

      if(ptr >= (char *)p->strings  && ptr<(char*)(p->strings+p->num_strings))
	fprintf(stderr,"**In string block.\n");

      if(ptr >= (char *)p->identifiers  && ptr<(char*)(p->identifiers+p->num_identifiers))
	fprintf(stderr,"**In identifier block.\n");
      
      return;
    }
    
    case T_OBJECT:
    {
      struct object *o=(struct object *)memblock;
      if(o->prog)
      {
	INT32 e,d;
	for(e=0;e<(INT32)o->prog->num_inherits;e++)
	{
	  struct inherit tmp=o->prog->inherits[e];
	  char *base=o->storage + tmp.storage_offset;
	  
	  for(d=0;d<(INT32)tmp.prog->num_identifiers;d++)
	  {
	    struct identifier *id=tmp.prog->identifiers+d;
	    if(!IDENTIFIER_IS_VARIABLE(id->identifier_flags)) continue;
	    
	    if(location == (void *)(base + id->func.offset))
	    {
	      fprintf(stderr,"**In variable %s\n",id->name->str);
	    }
	  }
	}
      }
      return;
    }

    case T_ARRAY:
    {
      struct array *a=(struct array *)memblock;
      struct svalue *s=(struct svalue *)location;
      fprintf(stderr,"**In index %ld\n",(long)(s-ITEM(a)));
      return;
    }
  }
}

static void gdb_gc_stop_here(void *a)
{
  fprintf(stderr,"***One ref found%s.\n",found_where);
  describe_something(found_in, found_in_type, 0);
  describe_location(found_in, found_in_type, gc_svalue_location);
}

void debug_gc_xmark_svalues(struct svalue *s, int num, char *fromwhere)
{
  found_in=(void *)fromwhere;
  found_in_type=-1;
  gc_xmark_svalues(s,num);
  found_in_type=T_UNKNOWN;
  found_in=0;
}

TYPE_FIELD debug_gc_check_svalues(struct svalue *s, int num, TYPE_T t, void *data)
{
  TYPE_FIELD ret;
  found_in=data;
  found_in_type=t;
  ret=gc_check_svalues(s,num);
  found_in_type=T_UNKNOWN;
  found_in=0;
  return ret;
}

void debug_gc_check_short_svalue(union anything *u, TYPE_T type, TYPE_T t, void *data)
{
  found_in=data;
  found_in_type=t;
  gc_check_short_svalue(u,type);
  found_in_type=T_UNKNOWN;
  found_in=0;
}


int debug_gc_check(void *x, TYPE_T t, void *data)
{
  int ret;
  found_in=data;
  found_in_type=t;
  ret=gc_check(x);
  found_in_type=T_UNKNOWN;
  found_in=0;
  return ret;
}

void describe_something(void *a, int t, int dm)
{
  struct program *p=(struct program *)a;
  if(!a) return;

#ifdef DEBUG_MALLOC
  if(dm)
    debug_malloc_dump_references(a);
#endif

  if(t==-1)
  {
    fprintf(stderr,"**Location description: %s\n",(char *)a);
    return;
  }

  fprintf(stderr,"**Location: %p  Type: %s  Refs: %d\n",a,
	  get_name_of_type(t),
	  *(INT32 *)a);

  switch(t)
  {
    case T_FUNCTION:
      if(attempt_to_identify(a) != T_OBJECT)
      {
	fprintf(stderr,"**Builtin function!\n");
	break;
      }

    case T_OBJECT:
      p=((struct object *)a)->prog;
      fprintf(stderr,"**Parent identifier: %d\n",((struct object *)a)->parent_identifier);
      if( ((struct object *)a)->parent)
      {
	fprintf(stderr,"**Describing object's parent:\n");
	describe_something( ((struct object *)a)->parent, t, 1);
      }else{
	fprintf(stderr,"**There is no parent (any longer?)\n");
      }
      if(!p)
      {
	fprintf(stderr,"**The object is destructed.\n");
	break;
      }
      fprintf(stderr,"**Attempting to describe program object was instantiated from:\n");
      
    case T_PROGRAM:
    {
      char *tmp;
      INT32 line,pos;
      int foo=0;

      fprintf(stderr,"**Program id: %ld\n",(long)(p->id));
      if(p->flags & PROGRAM_HAS_C_METHODS)
      {
	fprintf(stderr,"**The program was written in C.\n");
      }
      for(pos=0;pos<100;pos++)
      {
	tmp=get_line(p->program+pos, p, &line);
	if(tmp && line)
	{
	  fprintf(stderr,"**Location: %s:%ld\n",tmp,(long)line);
	  foo=1;
	  break;
	}
	if(pos+1>=(long)p->num_program)
	  break;
      }
#if 0
      if(!foo && p->num_linenumbers>1 && EXTRACT_UCHAR(p->linenumbers)=='\177')
      {
	fprintf(stderr,"**From file: %s\n",p->linenumbers+1);
	foo=1;
      }
#endif

      if(!foo)
      {
	int e;
#if 0
	fprintf(stderr,"**identifiers:\n");
	for(e=0;e<p->num_identifiers;e++)
	  fprintf(stderr,"**** %s\n",p->identifiers[e].name->str);
#else
	fprintf(stderr,"**identifiers:\n");
	for(e=0;e<p->num_identifier_references;e++)
	  fprintf(stderr,"**** %s\n",ID_FROM_INT(p,e)->name->str);
	
#endif

	fprintf(stderr,"**num inherits: %d\n",p->num_inherits);
      }

      break;
    }
      
    case T_ARRAY:
      fprintf(stderr,"**Describing array:\n");
      debug_dump_array((struct array *)a);
      break;

    case T_MAPPING:
      fprintf(stderr,"**Describing mapping:\n");
      debug_dump_mapping((struct mapping *)a);
      break;

    case T_STRING:
    {
      struct pike_string *s=(struct pike_string *)a;
      fprintf(stderr,"**String length is %d:\n",s->len);
      if(s->len>77)
      {
	fprintf(stderr,"** \"%60s ...\"\n",s->str);
      }else{
	fprintf(stderr,"** \"%s\"\n",s->str);
      }
      break;
    }
  }
  fprintf(stderr,"*******************\n");
}

void describe(void *x)
{
  describe_something(x, attempt_to_identify(x),1);
}

void debug_describe_svalue(struct svalue *s)
{
  fprintf(stderr,"Svalue at %p is:\n",s);
  switch(s->type)
  {
    case T_INT:
      fprintf(stderr,"    %ld\n",(long)s->u.integer);
      break;

    case T_FLOAT:
      fprintf(stderr,"    %f\n",s->u.float_number);
      break;

    case T_FUNCTION:
      if(s->subtype == FUNCTION_BUILTIN)
      {
	fprintf(stderr,"    Builtin function: %s\n",s->u.efun->name->str);
      }else{
	if(!s->u.object->prog)
	{
	  fprintf(stderr,"    Function in destructed object.\n");
	}else{
	  fprintf(stderr,"    Function name: %s\n",ID_FROM_INT(s->u.object->prog,s->subtype)->name->str);
	}
      }
  }
  describe_something(s->u.refs,s->type,1);
}

#endif

INT32 gc_check(void *a)
{
#ifdef PIKE_DEBUG
  if(check_for)
  {
    if(check_for == a)
    {
      gdb_gc_stop_here(a);
    }
    return 0;
  }
#endif
  return add_ref(get_marker(a));
}

static void init_gc(void)
{
#if 0
  INT32 tmp3;
  /* init hash , hashsize will be a prime between num_objects/8 and
   * num_objects/4, this will assure that no re-hashing is needed.
   */
  tmp3=my_log2(num_objects);

  if(!d_flag) tmp3-=2;
  if(tmp3<0) tmp3=0;
  if(tmp3>=(long)NELEM(hashprimes)) tmp3=NELEM(hashprimes)-1;
  hashsize=hashprimes[tmp3];

  hash=(struct marker **)xalloc(sizeof(struct marker **)*hashsize);
  MEMSET((char *)hash,0,sizeof(struct marker **)*hashsize);
  markers_left_in_chunk=0;
#else
  init_marker_hash();
#endif
}

static void exit_gc(void)
{
#if 0
  struct marker_chunk *m;
  /* Free hash table */
  free((char *)hash);
  while((m=chunk))
  {
    chunk=m->next;
    free((char *)m);
  }
#else
#ifdef DO_PIKE_CLEANUP
  int e=0;
  struct marker *h;
  for(e=0;e<marker_hash_table_size;e++)
    while(marker_hash_table[e])
      remove_marker(marker_hash_table[e]->data);
#endif
  exit_marker_hash();
#endif
}

#ifdef PIKE_DEBUG
void locate_references(void *a)
{
  if(!in_gc)
    init_gc();
  
  fprintf(stderr,"**Looking for references:\n");
  
  check_for=a;

  found_where=" in an array";
  gc_check_all_arrays();
  
  found_where=" in a multiset";
  gc_check_all_multisets();
  
  found_where=" in a mapping";
  gc_check_all_mappings();
  
  found_where=" in a program";
  gc_check_all_programs();
  
  found_where=" in an object";
  gc_check_all_objects();
  
  found_where=" in a module";
  call_callback(& gc_callbacks, (void *)0);
  
  found_where="";
  check_for=0;
  
  if(!in_gc)
    exit_gc();
}
#endif

int gc_is_referenced(void *a)
{
  struct marker *m;
  m=get_marker(a);
#ifdef PIKE_DEBUG
  if(m->refs + m->xrefs > *(INT32 *)a ||
     (!(m->refs < *(INT32 *)a) && m->xrefs) )
  {
    INT32 refs=m->refs;
    INT32 xrefs=m->xrefs;
    TYPE_T t=attempt_to_identify(a);

    fprintf(stderr,"**Something has %ld references, while gc() found %ld + %ld external.\n",(long)*(INT32 *)a,(long)refs,(long)xrefs);
    describe_something(a, t, 1);

    locate_references(a);

    fatal("Ref counts are wrong (has %d, found %d + %d external)\n",
	  *(INT32 *)a,
	  refs,
	  xrefs);
  }
#endif
  return m->refs < *(INT32 *)a;
}

#ifdef PIKE_DEBUG
int gc_external_mark(void *a)
{
  struct marker *m;
  if(check_for)
  {
    if(a==check_for)
    {
      char *tmp=found_where;
      found_where=" externally";
      gdb_gc_stop_here(a);
      found_where=tmp;

      return 1;
    }
    return 0;
  }
  m=get_marker(a);
  m->xrefs++;
  m->flags|=GC_XREFERENCED;
  gc_is_referenced(a);
  return 0;
}
#endif

int gc_mark(void *a)
{
  struct marker *m;
  m=get_marker(a);

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
  m=get_marker(a);
#ifdef PIKE_DEBUG
  if( !(m->flags & GC_REFERENCED)  && m->flags & GC_XREFERENCED )
  {
    INT32 refs=m->refs;
    INT32 xrefs=m->xrefs;
    TYPE_T t=attempt_to_identify(a);

    fprintf(stderr,"**gc_is_referenced failed, object has %ld references, while gc() found %ld + %ld external.\n",(long)*(INT32 *)a,(long)refs,(long)xrefs);
    describe_something(a, t, 1);

    locate_references(a);

    fatal("GC failed object (has %d, found %d + %d external)\n",
	  *(INT32 *)a,
	  refs,
	  xrefs);
  }
#endif
  return !(m->flags & GC_REFERENCED);
}

void do_gc(void)
{
  double tmp;
  INT32 tmp2;
  double multiplier;

  if(in_gc) return;
  in_gc=1;

  if(gc_evaluator_callback)
  {
    remove_callback(gc_evaluator_callback);
    gc_evaluator_callback=0;
  }

  tmp2=num_objects;

#ifdef PIKE_DEBUG
  if(t_flag)
    fprintf(stderr,"Garbage collecting ... ");
  if(num_objects < 0)
    fatal("Panic, less than zero objects!\n");

  last_gc=TIME(0);

#endif

  multiplier=pow(MULTIPLIER, (double) num_allocs / (double) alloc_threshold);
  objects_alloced*=multiplier;
  objects_alloced += (double) num_allocs;
  
  objects_freed*=multiplier;
  objects_freed += (double) num_objects;


  init_gc();

  /* First we count internal references */
  gc_check_all_arrays();
  gc_check_all_multisets();
  gc_check_all_mappings();
  gc_check_all_programs();
  gc_check_all_objects();
  call_callback(& gc_callbacks, (void *)0);

  /* Next we mark anything with external references */
  gc_mark_all_arrays();
  run_queue(&gc_mark_queue);
  gc_mark_all_multisets();
  run_queue(&gc_mark_queue);
  gc_mark_all_mappings();
  run_queue(&gc_mark_queue);
  gc_mark_all_programs();
  run_queue(&gc_mark_queue);
  gc_mark_all_objects();
  run_queue(&gc_mark_queue);

  if(d_flag)
    gc_mark_all_strings();

  /* Now we free the unused stuff */
  gc_free_all_unreferenced_arrays();
  gc_free_all_unreferenced_multisets();
  gc_free_all_unreferenced_mappings();
  gc_free_all_unreferenced_programs();
  gc_free_all_unreferenced_objects();

  exit_gc();

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

#ifdef PIKE_DEBUG
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


