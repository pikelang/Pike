/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
/**/
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
#include "interpret.h"

#include "gc.h"
#include "main.h"
#include <math.h>

#include "block_alloc.h"

RCSID("$Id: gc.c,v 1.80 2000/04/23 03:17:11 mast Exp $");

/* Run garbage collect approximate every time we have
 * 20 percent of all arrays, objects and programs is
 * garbage.
 */

#define GC_CONST 20
#define MIN_ALLOC_THRESHOLD 1000
#define MAX_ALLOC_THRESHOLD 10000000
#define MULTIPLIER 0.9
#define MARKER_CHUNK_SIZE 1023

INT32 num_objects = 1;		/* Account for empty_array. */
INT32 num_allocs =0;
INT32 alloc_threshold = MIN_ALLOC_THRESHOLD;
int Pike_in_gc = 0;
struct pike_queue gc_mark_queue;
time_t last_gc;


static double objects_alloced = 0.0;
static double objects_freed = 0.0;

struct callback_list gc_callbacks;

struct callback *debug_add_gc_callback(callback_func call,
				 void *arg,
				 callback_func free_func)
{
  return add_to_callback(&gc_callbacks, call, arg, free_func);
}


#undef INIT_BLOCK
#ifdef PIKE_DEBUG
#define INIT_BLOCK(X) (X)->flags=(X)->refs=(X)->xrefs=0; (X)->saved_refs=-1;
#else
#define INIT_BLOCK(X) (X)->flags=(X)->refs=0
#endif

PTR_HASH_ALLOC(marker,MARKER_CHUNK_SIZE)

#ifdef PIKE_DEBUG

void dump_gc_info(void)
{
  fprintf(stderr,"Current number of objects: %ld\n",(long)num_objects);
  fprintf(stderr,"Objects allocated total  : %ld\n",(long)num_allocs);
  fprintf(stderr," threshold for next gc() : %ld\n",(long)alloc_threshold);
  fprintf(stderr,"Average allocs per gc()  : %f\n",objects_alloced);
  fprintf(stderr,"Average frees per gc()   : %f\n",objects_freed);
  fprintf(stderr,"Second since last gc()   : %ld\n", (long)TIME(0) - (long)last_gc);
  fprintf(stderr,"Projected garbage        : %f\n", objects_freed * (double) num_allocs / (double) alloc_threshold);
  fprintf(stderr,"in_gc                    : %d\n", Pike_in_gc);
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

void *check_for =0;
static char *found_where="";
static void *found_in=0;
static int found_in_type=0;
void *gc_svalue_location=0;
char *fatal_after_gc=0;
static int gc_debug = 0;

#define DESCRIBE_MEM 1
#define DESCRIBE_NO_REFS 2
#define DESCRIBE_SHORT 4
#define DESCRIBE_NO_DMALLOC 8

/* type == -1 means that memblock is a char* and should be
 * really be printed..
 */
void describe_location(void *real_memblock,
		       int real_type,
		       void *location,
		       int indent,
		       int depth,
		       int flags)
{
  struct program *p;
  void *memblock=0;
  int type=real_type;
  if(!location) return;
/*  fprintf(stderr,"**Location of (short) svalue: %p\n",location); */

  if(real_type!=-1) memblock=real_memblock;

#ifdef DEBUG_MALLOC
  if(memblock == 0 || type == -1)
  {
    extern void *dmalloc_find_memblock_base(void *);
    memblock=dmalloc_find_memblock_base(location);
  }
#endif

  if(type==T_UNKNOWN)
    type=attempt_to_identify(memblock);

  if(memblock)
    fprintf(stderr,"%*s-> from %s %p offset %ld\n",
	    indent,"",
	    get_name_of_type(type),
	    memblock,
	    ((long)location - (long)memblock));
  else
    fprintf(stderr,"%*s-> at location %p in unknown memblock (mmaped?)\n",
	    indent,"",
	    location);


  if(memblock && depth>0)
    describe_something(memblock,type,indent+2,depth-1,flags | DESCRIBE_MEM);

 again:
  switch(type)
  {
    case T_UNKNOWN:
      for(p=first_program;p;p=p->next)
      {
	if(memblock == (void *)p->program)
	{
	  fprintf(stderr,"%*s  **In memory block for program at %p\n",
		  indent,"",
		  p);
	  memblock=p;
	  type=T_PROGRAM;
	  goto again;
	}
      }
      break;
      
    case T_PROGRAM:
    {
      long e;
      char *ptr=(char *)location;
      p=(struct program *)memblock;

      if(location == (void *)&p->prev)
	fprintf(stderr,"%*s  **In p->prev\n",indent,"");

      if(location == (void *)&p->next)
	fprintf(stderr,"%*s  **In p->next\n",indent,"");

      if(p->inherits &&
	 ptr >= (char *)p->inherits  &&
	 ptr<(char*)(p->inherits+p->num_inherits)) 
      {
	e=((long)ptr - (long)(p->inherits)) / sizeof(struct inherit);
	fprintf(stderr,"%*s  **In p->inherits[%ld] (%s)\n",indent,"",
		e,
		p->inherits[e].name ? p->inherits[e].name->str : "no name");
	return;
      }

      if(p->constants &&
	 ptr >= (char *)p->constants  &&
	 ptr<(char*)(p->constants+p->num_constants))
      {
	e=((long)ptr - (long)(p->constants)) / sizeof(struct program_constant);
	fprintf(stderr,"%*s  **In p->constants[%ld] (%s)\n",indent,"",
		e,
		p->constants[e].name ? p->constants[e].name->str : "no name");
	return;
      }


      if(p->identifiers && 
	 ptr >= (char *)p->identifiers  &&
	 ptr<(char*)(p->identifiers+p->num_identifiers))
      {
	e=((long)ptr - (long)(p->identifiers)) / sizeof(struct identifier);
	fprintf(stderr,"%*s  **In p->identifiers[%ld] (%s)\n",indent,"",
		e,
		p->identifiers[e].name ? p->identifiers[e].name->str : "no name");
	return;
      }

#define FOO(NTYP,TYP,NAME) \
    if(location == (void *)&p->NAME) fprintf(stderr,"%*s  **In p->" #NAME "\n",indent,""); \
    if(ptr >= (char *)p->NAME  && ptr<(char*)(p->NAME+p->PIKE_CONCAT(num_,NAME))) \
      fprintf(stderr,"%*s  **In p->" #NAME "[%ld]\n",indent,"",((long)ptr - (long)(p->NAME)) / sizeof(TYP));
#include "program_areas.h"
      
      return;
    }
    
    case T_OBJECT:
    {
      struct object *o=(struct object *)memblock;
      struct program *p;

      if(location == (void *)&o->parent) fprintf(stderr,"%*s  **In o->parent\n",indent,"");
      if(location == (void *)&o->prog)  fprintf(stderr,"%*s  **In o->prog\n",indent,"");
      if(location == (void *)&o->next)  fprintf(stderr,"%*s  **In o->next\n",indent,"");
      if(location == (void *)&o->prev)  fprintf(stderr,"%*s  **In o->prev\n",indent,"");

      p=o->prog;

      if(!o->prog)
      {
	p=id_to_program(o->program_id);
	if(p)
	  fprintf(stderr,"%*s  **(We are lucky, found program for destructed object)\n",indent,"");
      }

      if(p)
      {
	INT32 e,d;
	for(e=0;e<(INT32)p->num_inherits;e++)
	{
	  struct inherit tmp=p->inherits[e];
	  char *base=o->storage + tmp.storage_offset;
	  
	  for(d=0;d<(INT32)tmp.prog->num_identifiers;d++)
	  {
	    struct identifier *id=tmp.prog->identifiers+d;
	    if(!IDENTIFIER_IS_VARIABLE(id->identifier_flags)) continue;
	    
	    if(location == (void *)(base + id->func.offset))
	    {
	      fprintf(stderr,"%*s  **In variable %s\n",indent,"",id->name->str);
	    }
	  }

	  if((char *)location >= base && (char *)location <= base +
	     ( tmp.prog->storage_needed - tmp.prog->inherits[0].storage_offset ))
	  {
	    fprintf(stderr,"%*s  **In storage for inherit %d",indent,"",e);
	    if(tmp.name)
	      fprintf(stderr," (%s)",tmp.name->str);
	    fprintf(stderr,"\n");
	  }
	     
	}
      }
      return;
    }

    case T_ARRAY:
    {
      struct array *a=(struct array *)memblock;
      struct svalue *s=(struct svalue *)location;
      fprintf(stderr,"%*s  **In index %ld\n",indent,"",(long)(s-ITEM(a)));
      return;
    }
  }
}

static void gdb_gc_stop_here(void *a)
{
  fprintf(stderr,"***One ref found%s.\n",found_where?found_where:"");
  describe_something(found_in, found_in_type, 2, 1, DESCRIBE_NO_DMALLOC);
  describe_location(found_in , found_in_type, gc_svalue_location,2,1,0);
  fprintf(stderr,"----------end------------\n");
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

void low_describe_something(void *a,
			    int t,
			    int indent,
			    int depth,
			    int flags)
{
  struct program *p=(struct program *)a;

  if(depth<0) return;

  switch(t)
  {
    case T_FUNCTION:
      if(attempt_to_identify(a) != T_OBJECT)
      {
	fprintf(stderr,"%*s**Builtin function!\n",indent,"");
	break;
      }

    case T_OBJECT:
      p=((struct object *)a)->prog;
      fprintf(stderr,"%*s**Parent identifier: %d\n",indent,"",((struct object *)a)->parent_identifier);
      fprintf(stderr,"%*s**Program id: %ld\n",indent,"",((struct object *)a)->program_id);

      if (((struct object *)a)->next == ((struct object *)a))
	fprintf(stderr, "%*s**The object is fake.\n",indent,"");

      if(!p)
      {
	fprintf(stderr,"%*s**The object is destructed.\n",indent,"");
	p=id_to_program(((struct object *)a)->program_id);
      }
      if (p) {
	fprintf(stderr,"%*s**Attempting to describe program object was instantiated from:\n",indent,"");
	low_describe_something(p, T_PROGRAM, indent, depth, flags);
      }

      if( ((struct object *)a)->parent)
      {
	fprintf(stderr,"%*s**Describing object's parent:\n",indent,"");
	describe_something( ((struct object *)a)->parent, t, indent+2,depth-1,
			    (flags | DESCRIBE_SHORT | DESCRIBE_NO_REFS )
			    & ~ (DESCRIBE_MEM));
      }else{
	fprintf(stderr,"%*s**There is no parent (any longer?)\n",indent,"");
      }
      break;
      
    case T_PROGRAM:
    {
      char *tmp;
      INT32 line,pos;
      int foo=0;

      fprintf(stderr,"%*s**Program id: %ld\n",indent,"",(long)(p->id));
      if(p->flags & PROGRAM_HAS_C_METHODS)
      {
	fprintf(stderr,"%*s**The program was written in C.\n",indent,"");
      }
      for(pos=0;pos<100;pos++)
      {
	tmp=get_line(p->program+pos, p, &line);
	if(tmp && line)
	{
	  fprintf(stderr,"%*s**Location: %s:%ld\n",indent,"",tmp,(long)line);
	  foo=1;
	  break;
	}
	if(pos+1>=(long)p->num_program)
	  break;
      }
#if 0
      if(!foo && p->num_linenumbers>1 && EXTRACT_UCHAR(p->linenumbers)=='\177')
      {
	fprintf(stderr,"%*s**From file: %s\n",indent,"",p->linenumbers+1);
	foo=1;
      }
#endif

      if(!foo)
      {
	int e;
	fprintf(stderr,"%*s**identifiers:\n",indent,"");
	for(e=0;e<p->num_identifier_references;e++)
	  fprintf(stderr,"%*s**** %s\n",indent,"",ID_FROM_INT(p,e)->name->str);
	
	fprintf(stderr,"%*s**num inherits: %d\n",indent,"",p->num_inherits);
      }

      if(flags & DESCRIBE_MEM)
      {
#define FOO(NUMTYPE,TYPE,NAME) \
      fprintf(stderr,"%*s* " #NAME " %p[%d]\n",indent,"",p->NAME,p->PIKE_CONCAT(num_,NAME));
#include "program_areas.h"
      }

      break;
    }
      
    case T_ARRAY:
      fprintf(stderr,"%*s**Describing array:\n",indent,"");
      debug_dump_array((struct array *)a);
      break;

    case T_MAPPING:
      fprintf(stderr,"%*s**Describing mapping:\n",indent,"");
      debug_dump_mapping((struct mapping *)a);
      fprintf(stderr,"%*s**Describing mapping data block:\n",indent,"");
      describe_something( ((struct mapping *)a)->data, -2, indent+2,depth-1,flags);
      break;

    case T_STRING:
    {
      struct pike_string *s=(struct pike_string *)a;
      fprintf(stderr,"%*s**String length is %d:\n",indent,"",s->len);
      if(s->len>77)
      {
	fprintf(stderr,"%*s** \"%60s ...\"\n",indent,"",s->str);
      }else{
	fprintf(stderr,"%*s** \"%s\"\n",indent,"",s->str);
      }
      break;
    }
  }
}

void describe_something(void *a, int t, int indent, int depth, int flags)
{
  int tmp;
  struct program *p=(struct program *)a;
  if(!a) return;

  if(t==-1)
  {
    fprintf(stderr,"%*s**Location description: %s\n",indent,"",(char *)a);
    return;
  }

  /* Disable debug, this may help reduce recursion bugs */
  tmp=d_flag;
  d_flag=0;

#ifdef DEBUG_MALLOC
  if (((int)a) == 0x55555555) {
    fprintf(stderr,"%*s**Location: %p  Type: %s  Zapped pointer\n",indent,"",a,
	    get_name_of_type(t));
  } else
#endif /* DEBUG_MALLOC */
  if (((int)a) & 3) {
    fprintf(stderr,"%*s**Location: %p  Type: %s  Misaligned address\n",indent,"",a,
	    get_name_of_type(t));
  } else {
    fprintf(stderr,"%*s**Location: %p  Type: %s  Refs: %d\n",indent,"",a,
	    get_name_of_type(t),
	    *(INT32 *)a);
  }

#ifdef DEBUG_MALLOC
  if(!(flags & DESCRIBE_NO_DMALLOC))
    debug_malloc_dump_references(a,indent+2,depth-1,flags);
#endif

  low_describe_something(a,t,indent,depth,flags);

  
  fprintf(stderr,"%*s*******************\n",indent,"");
  d_flag=tmp;
}

void describe(void *x)
{
  describe_something(x, attempt_to_identify(x), 0, 2, 0);
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
	  struct program *p=id_to_program(s->u.object->program_id);
	  if(p)
	  {
	    fprintf(stderr,"    Function (destructed) name: %s\n",ID_FROM_INT(p,s->subtype)->name->str);
	  }else{
	    fprintf(stderr,"    Function in destructed object.\n");
	  }
	}else{
	  fprintf(stderr,"    Function name: %s\n",ID_FROM_INT(s->u.object->prog,s->subtype)->name->str);
	}
      }
  }
  describe_something(s->u.refs,s->type,0,2,0);
}

void debug_gc_touch(void *a)
{
  struct marker *m;
  if (!a) fatal("real_gc_check(): Got null pointer.\n");

  m = find_marker(a);
  if (Pike_in_gc == GC_PASS_PRETOUCH) {
    if (m) {
      fprintf(stderr,"**Object touched twice.\n");
      describe(a);
      fatal("Object touched twice.\n");
    }
    get_marker(a)->flags |= GC_TOUCHED;
  }
  else if (Pike_in_gc == GC_PASS_POSTTOUCH) {
    if (m) {
      if (!(m->flags & GC_TOUCHED)) {
	fprintf(stderr,"**An existing but untouched marker found for object in linked lists. flags: %x.\n", m->flags);
	describe(a);
	fatal("An existing but untouched marker found for object in linked lists.\n");
      }
      if ((m->flags & (GC_REFERENCED|GC_CHECKED)) == GC_CHECKED) {
	fprintf(stderr,"**An object to garb is still around. flags: %x.\n", m->flags);
	fprintf(stderr,"    has %ld references, while gc() found %ld + %ld external.\n",(long)*(INT32 *)a,(long)m->refs,(long)m->xrefs);
	describe(a);
	locate_references(a);
	fprintf(stderr,"##### Continuing search for more bugs....\n");
	fatal_after_gc="An object to garb is still around.\n";
      }
    }
  }
  else
    fatal("debug_gc_touch() used in invalid gc pass.\n");
}

#endif /* PIKE_DEBUG */

INT32 real_gc_check(void *a)
{
  struct marker *m;

#ifdef PIKE_DEBUG
  if (!a) fatal("real_gc_check(): Got null pointer.\n");
  if(check_for)
  {
    if(check_for == a)
    {
      gdb_gc_stop_here(a);
    }

    m=get_marker(a);
    if(check_for == (void *)1 &&
       m && (m->flags & (GC_REFERENCED|GC_CHECKED)) == GC_CHECKED)
    {
      int t=attempt_to_identify(a);
      if(t != T_STRING && t != T_UNKNOWN)
      {
	fprintf(stderr,"**Reference to object to free in referenced object!\n");
	fprintf(stderr,"    has %ld references, while gc() found %ld + %ld external.\n",(long)*(INT32 *)a,(long)m->refs,(long)m->xrefs);
	describe(a);
	locate_references(a);
#if 1
	fatal("Reference to object to free in referenced object!\n");
#else
	fprintf(stderr,"##### Continuing search for more bugs....\n");
	fatal_after_gc="Reference to object to free in referenced object!\n";
#endif
      }
    }
    return 0;
  }

  if (Pike_in_gc != GC_PASS_CHECK)
    fatal("gc check attempted in invalid pass.\n");

  m = get_marker(a);

  if(m->saved_refs != -1)
    if(m->saved_refs != *(INT32 *)a) {
      fprintf(stderr,"**Refs changed in gc() pass %d. Expected %ld, got %ld.\n",
	      Pike_in_gc, (long)m->saved_refs, (long)*(INT32 *)a);
      describe(a);
      locate_references(a);
      fprintf(stderr,"##### Continuing search for more bugs....\n");
      fatal_after_gc="Refs changed in gc()\n";
    }
  m->saved_refs = *(INT32 *)a;
#else
  m = get_marker(a);
#endif

  m->flags |= GC_CHECKED;
  return add_ref(m);
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
/*  init_marker_hash(num_objects*8); */
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
  int tmp, orig_in_gc = Pike_in_gc;
  void *orig_check_for=check_for;
  if(!Pike_in_gc)
    init_gc();
  Pike_in_gc = GC_PASS_LOCATE;

  /* Disable debug, this may help reduce recursion bugs */
  tmp=d_flag;
  d_flag=0;

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

#ifdef PIKE_DEBUG
  if(master_object) gc_external_mark2(master_object,0," &master_object");
  {
    extern struct mapping *builtin_constants;
    if(builtin_constants)
      gc_external_mark2(builtin_constants,0," &builtin_constants");
  }
#endif
  
  found_where=" in a module";
  call_callback(& gc_callbacks, (void *)0);
  
  found_where="";
  check_for=orig_check_for;

#ifdef DEBUG_MALLOC
  {
    extern void dmalloc_find_references_to(void *);
#if 0
    fprintf(stderr,"**DMALLOC Looking for references:\n");
    dmalloc_find_references_to(a);
#endif
  }
#endif

  Pike_in_gc = orig_in_gc;
  if(!Pike_in_gc)
    exit_gc();
  d_flag=tmp;
}
#endif

#ifdef PIKE_DEBUG

int debug_gc_is_referenced(void *a)
{
  struct marker *m;
  if (!a) fatal("real_gc_check(): Got null pointer.\n");

  m=get_marker(a);
  if(m->refs + m->xrefs > *(INT32 *)a ||
     (!(m->refs < *(INT32 *)a) && m->xrefs)  ||
     (Pike_in_gc >= GC_PASS_CHECK && Pike_in_gc <= GC_PASS_MARK &&
      m->saved_refs != -1 && m->saved_refs != *(INT32 *)a))
  {
    INT32 refs=m->refs;
    INT32 xrefs=m->xrefs;
    TYPE_T t=attempt_to_identify(a);
    d_flag=0;

    fprintf(stderr,"**Something has %ld refs, while gc() found %ld + %ld external.\n",
	    (long)*(INT32 *)a,
	    (long)refs,
	    (long)xrefs);

    if(m->saved_refs != *(INT32 *)a)
      fprintf(stderr,"**In check pass it had %ld refs!!!\n",(long)m->saved_refs);

    describe_something(a, t, 0,2,0);

    locate_references(a);

    fatal("Ref counts are wrong (has %d, found %d + %d external)\n",
	  *(INT32 *)a,
	  refs,
	  xrefs);
  }

  return m->refs < *(INT32 *)a;
}
#endif

#ifdef PIKE_DEBUG
int gc_external_mark3(void *a, void *in, char *where)
{
  struct marker *m;
  if (!a) fatal("real_gc_check(): Got null pointer.\n");

  if(check_for)
  {
    if(a==check_for)
    {
      char *tmp=found_where;
      void *tmp2=found_in;

      if(where) found_where=where;
      if(in) found_in=in;

      gdb_gc_stop_here(a);

      found_where=tmp;
      found_in=tmp2;

      return 1;
    }

    m=get_marker(a);
    if(check_for == (void *)1 &&
       m && (m->flags & (GC_REFERENCED|GC_CHECKED)) == GC_CHECKED)
    {
      int t=attempt_to_identify(a);
      if(t != T_STRING && t != T_UNKNOWN)
      {
	fprintf(stderr,"EXTERNAL Reference to object to free%s!\n",in?(char *)in:"");
	fprintf(stderr,"    has %ld references, while gc() found %ld + %ld external.\n",(long)*(INT32 *)a,(long)m->refs,(long)m->xrefs);
	if(where) describe_location(0,T_UNKNOWN,where,4,1,0);
	describe(a);
	locate_references(a);
#if 1
	fatal("EXTERNAL Reference to object to free.\n");
#else
	fprintf(stderr,"##### Continuing search for more bugs....\n");
	fatal_after_gc="EXTERNAL Reference to object to free.\n";
#endif
      }
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

#ifdef PIKE_DEBUG
  if (!a) fatal("real_gc_check(): Got null pointer.\n");
  if (Pike_in_gc != GC_PASS_MARK)
    fatal("gc mark attempted in invalid pass.\n");
#endif

  m = get_marker(debug_malloc_pass(a));
  if(m->flags & GC_REFERENCED)
  {
    return 0;
  }else{
    m->flags |= GC_REFERENCED;
    return 1;
  }
}

#ifdef PIKE_DEBUG
int debug_gc_do_free(void *a)
{
  struct marker *m;
  if (!a) fatal("real_gc_check(): Got null pointer.\n");

  m=find_marker(debug_malloc_pass(a));
  if (!m) return 0;		/* Object created after mark pass. */

  if( (m->flags & (GC_REFERENCED|GC_CHECKED)) == GC_CHECKED &&
      (m->flags & GC_XREFERENCED) )
  {
    INT32 refs=m->refs;
    INT32 xrefs=m->xrefs;
    TYPE_T t=attempt_to_identify(a);
    if(t != T_STRING && t != T_UNKNOWN)
    {
      fprintf(stderr,
	      "**gc_is_referenced failed, object has %ld references,\n"
	      "** while gc() found %ld + %ld external. (type=%d, flags=%x)\n",
	      (long)*(INT32 *)a,(long)refs,(long)xrefs,t,m->flags);
      describe_something(a, t, 4,1,0);

      locate_references(a);

      fatal("GC failed object (has %d, found %d + %d external)\n",
	    *(INT32 *)a,
	    refs,
	    xrefs);
    }
  }

  return (m->flags & (GC_REFERENCED|GC_CHECKED)) == GC_CHECKED;
}
#endif

void do_gc(void)
{
  double tmp;
  INT32 tmp2;
  double multiplier;
  int destroyed, destructed;
#ifdef HAVE_GETHRTIME
#ifdef PIKE_DEBUG
  hrtime_t gcstarttime;
#endif
#endif

  if(Pike_in_gc) return;
  init_gc();
  Pike_in_gc=GC_PASS_PREPARE;
#ifdef PIKE_DEBUG
  gc_debug = d_flag;
#endif

  /* Make sure there will be no callback to this while we're in the
   * gc. That can be fatal since this function links objects back to
   * the object list, which causes that list to be reordered and the
   * various gc loops over it might then miss things. */
  destruct_objects_to_destruct();

  if(gc_evaluator_callback)
  {
    remove_callback(gc_evaluator_callback);
    gc_evaluator_callback=0;
  }

  tmp2=num_objects;

#ifdef PIKE_DEBUG
  if(t_flag) {
    fprintf(stderr,"Garbage collecting ... ");
#ifdef HAVE_GETHRTIME
    gcstarttime = gethrtime();
#endif
  }
  if(num_objects < 0)
    fatal("Panic, less than zero objects!\n");
#endif

  last_gc=TIME(0);

  multiplier=pow(MULTIPLIER, (double) num_allocs / (double) alloc_threshold);
  objects_alloced*=multiplier;
  objects_alloced += (double) num_allocs;
  
  objects_freed*=multiplier;
  objects_freed += (double) num_objects;

#ifdef PIKE_DEBUG
  if (gc_debug) {
    INT32 n;
    Pike_in_gc = GC_PASS_PRETOUCH;
    n = gc_touch_all_arrays();
    n += gc_touch_all_multisets();
    n += gc_touch_all_mappings();
    n += gc_touch_all_programs();
    n += gc_touch_all_objects();
    if (n != num_objects)
      fatal("Object count wrong before gc; expected %d, got %d.\n", num_objects, n);
  }
#endif

  Pike_in_gc=GC_PASS_CHECK;
  /* First we count internal references */
  gc_check_all_arrays();
  gc_check_all_multisets();
  gc_check_all_mappings();
  gc_check_all_programs();
  gc_check_all_objects();

#ifdef PIKE_DEBUG
  if(master_object) gc_external_mark2(master_object,0," &master_object");
  {
    extern struct mapping *builtin_constants;
    if(builtin_constants)
      gc_external_mark2(builtin_constants,0," &builtin_constants");
  }
#endif

  /* These callbacks are mainly for the check pass, but can also
   * do things that are normally associated with the mark pass
   */
  call_callback(& gc_callbacks, (void *)0);

  Pike_in_gc=GC_PASS_MARK;
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

#ifdef PIKE_DEBUG
  check_for=(void *)1;
#endif
  Pike_in_gc=GC_PASS_FREE;
  /* Now we free the unused stuff */
  gc_free_all_unreferenced_arrays();
  gc_free_all_unreferenced_multisets();
  gc_free_all_unreferenced_mappings();
  gc_free_all_unreferenced_programs();
  Pike_in_gc=GC_PASS_DESTROY;	/* Pike code allowed in this pass. */
  /* This is intended to happen before the freeing done above. But
   * it's put here for the time being, since the problem of non-object
   * objects getting external references from destroy code isn't
   * solved yet. */
  destroyed = gc_destroy_all_unreferenced_objects();
  Pike_in_gc=GC_PASS_FREE;
  destructed = gc_free_all_unreferenced_objects();

#ifdef PIKE_DEBUG
  if (destroyed != destructed)
    fatal("destroy() called in %d objects in gc, but %d destructed.\n",
	  destroyed, destructed);
  check_for=0;
  if(fatal_after_gc) fatal(fatal_after_gc);
#endif

  Pike_in_gc=GC_PASS_DESTRUCT;
  destruct_objects_to_destruct();

#ifdef PIKE_DEBUG
  if (gc_debug) {
    INT32 n;
    Pike_in_gc=GC_PASS_POSTTOUCH;
    n = gc_touch_all_arrays();
    n += gc_touch_all_multisets();
    n += gc_touch_all_mappings();
    n += gc_touch_all_programs();
    n += gc_touch_all_objects();
    if (n != num_objects)
      fatal("Object count wrong after gc; expected %d, got %d.\n", num_objects, n);
    if(fatal_after_gc) fatal(fatal_after_gc);
  }
#endif

  Pike_in_gc=0;
  exit_gc();

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
  {
#ifdef HAVE_GETHRTIME
    fprintf(stderr,"done (freed %ld of %ld objects), %ld ms.\n",
	    (long)(tmp2-num_objects),(long)tmp2,
	    (long)((gethrtime() - gcstarttime)/1000000));
#else
    fprintf(stderr,"done (freed %ld of %ld objects)\n",
	    (long)(tmp2-num_objects),(long)tmp2);
#endif
  }
#endif

#ifdef ALWAYS_GC
  ADD_GC_CALLBACK();
#else
  if(d_flag > 3) ADD_GC_CALLBACK();
#endif
}


void f__gc_status(INT32 args)
{
  pop_n_elems(args);

  push_constant_text("num_objects");
  push_int(num_objects);

  push_constant_text("num_allocs");
  push_int(num_allocs);

  push_constant_text("alloc_threshold");
  push_int(alloc_threshold);

  push_constant_text("objects_alloced");
  push_int(objects_alloced);

  push_constant_text("objects_freed");
  push_int(objects_freed);

  push_constant_text("last_gc");
  push_int(last_gc);

  push_constant_text("projected_garbage");
  push_float(objects_freed * (double) num_allocs / (double) alloc_threshold);

  f_aggregate_mapping(14);
}
