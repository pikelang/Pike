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

RCSID("$Id: gc.c,v 1.100 2000/07/03 18:38:26 mast Exp $");

/* Run garbage collect approximately every time
 * 20 percent of all arrays, objects and programs is
 * garbage.
 */

#define GC_CONST 20
#define MIN_ALLOC_THRESHOLD 1000
#define MAX_ALLOC_THRESHOLD 10000000
#define MULTIPLIER 0.9
#define MARKER_CHUNK_SIZE 1023
#define REF_CYCLE_CHUNK_SIZE 32

/* The gc will free all things with no external references that isn't
 * referenced by undestructed objects with destroy() lfuns (known as
 * "live" objects). Live objects without external references are then
 * destructed and garbage collected with normal refcount garbing
 * (which might leave dead garbage around for the next gc). These live
 * objects are destructed in an order that tries to be as well defined
 * as possible using several rules:
 *
 * o  If an object A references B single way, then A is destructed
 *    before B.
 * o  If A and B are in a cycle, and there is a reference somewhere
 *    from B to A that is weaker than any reference from A to B, then
 *    A is destructed before B.
 * o  Weak references are considered weaker than normal ones, and both
 *    are considered weaker than strong references.
 * o  Strong references are used in special cases like parent object
 *    references. There can never be a cycle consisting only of strong
 *    references. (This means the gc will never destruct a parent
 *    object before all childs has been destructed.)
 *
 * The gc tries to detect and warn about cases where there are live
 * objects with no well defined order between them. There are cases
 * that are missed by this detection, though.
 *
 * Things that aren't live objects but are referenced from them are
 * still intact during this destruct pass, so it's entirely possible
 * to save these things by adding external references to them.
 * However, it's not possible for live objects to save themselves or
 * other live objects; all live objects that didn't have external
 * references at the start of the gc pass will be destructed
 * regardless of added references.
 *
 * Things that have only weak references at the start of the gc pass
 * will be freed. That's done before the live object destruct pass.
 */

/* #define GC_VERBOSE */
/* #define GC_CYCLE_DEBUG */

#if defined(GC_VERBOSE) && !defined(PIKE_DEBUG)
#undef GC_VERBOSE
#endif
#ifdef GC_VERBOSE
#define GC_VERBOSE_DO(X) X
#else
#define GC_VERBOSE_DO(X)
#endif

INT32 num_objects = 1;		/* Account for empty_array. */
INT32 num_allocs =0;
INT32 alloc_threshold = MIN_ALLOC_THRESHOLD;
int Pike_in_gc = 0;
struct pike_queue gc_mark_queue;
time_t last_gc;

static struct marker rec_list = {0, 0, 0};
struct marker *gc_rec_last = &rec_list;
static struct marker *kill_list = 0;
static unsigned last_cycle;

/* rec_list is a linked list of the markers currently being recursed
 * through in the cycle check pass. gc_rec_last points at the
 * innermost marker being visited. A new marker is linked in after
 * gc_rec_last, except when that's inside a cycle, in which case it's
 * linked in after that cycle. A cycle is always treated as one atomic
 * unit, e.g. it's either popped whole or not at all.
 *
 * Two ranges of markers next to each other may swap places to break a
 * cyclic reference at a chosen point. Some markers thus get a place
 * earlier on the list even though their corresponding stack frames
 * are later than some other markers they have references to. These
 * markers are flagged to make them stay on the list when they're
 * popped from the stack. They'll get popped eventually since all
 * markers in the gap between the top one and it's previous
 * gc_rec_last point are popped.
 *
 * Markers for live objects are linked into the beginning of kill_list
 * when they're popped from rec_list.
 *
 * The cycle check functions might recurse another round through the
 * markers that have been recursed already, to propagate the GC_LIVE
 * flag to things that have been found to be referenced from live
 * objects. rec_list is not touched at all in this extra round.
 */

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
#define INIT_BLOCK(X)					\
  (X)->flags=(X)->refs=(X)->weak_refs=(X)->xrefs=0;	\
  (X)->saved_refs=-1;					\
  (X)->cycle = (unsigned INT16) -1;			\
  (X)->link = (struct marker *) -1;
#else
#define INIT_BLOCK(X)					\
  (X)->flags=(X)->refs=(X)->weak_refs=0;
#endif

PTR_HASH_ALLOC(marker,MARKER_CHUNK_SIZE)

#ifdef PIKE_DEBUG

int gc_in_cycle_check = 0;
static unsigned weak_freed, checked, marked, cycle_checked, live_ref;
static unsigned gc_extra_refs = 0;

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
int gc_debug = 0;

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
	break;
      }

      if(p->constants &&
	 ptr >= (char *)p->constants  &&
	 ptr<(char*)(p->constants+p->num_constants))
      {
	e=((long)ptr - (long)(p->constants)) / sizeof(struct program_constant);
	fprintf(stderr,"%*s  **In p->constants[%ld] (%s)\n",indent,"",
		e,
		p->constants[e].name ? p->constants[e].name->str : "no name");
	break;
      }


      if(p->identifiers && 
	 ptr >= (char *)p->identifiers  &&
	 ptr<(char*)(p->identifiers+p->num_identifiers))
      {
	e=((long)ptr - (long)(p->identifiers)) / sizeof(struct identifier);
	fprintf(stderr,"%*s  **In p->identifiers[%ld] (%s)\n",indent,"",
		e,
		p->identifiers[e].name ? p->identifiers[e].name->str : "no name");
	break;
      }

#define FOO(NTYP,TYP,NAME) \
    if(location == (void *)&p->NAME) fprintf(stderr,"%*s  **In p->" #NAME "\n",indent,""); \
    if(ptr >= (char *)p->NAME  && ptr<(char*)(p->NAME+p->PIKE_CONCAT(num_,NAME))) \
      fprintf(stderr,"%*s  **In p->" #NAME "[%ld]\n",indent,"",((long)ptr - (long)(p->NAME)) / sizeof(TYP));
#include "program_areas.h"
      
      break;
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
      break;
    }

    case T_ARRAY:
    {
      struct array *a=(struct array *)memblock;
      struct svalue *s=(struct svalue *)location;
      fprintf(stderr,"%*s  **In index %ld\n",indent,"",(long)(s-ITEM(a)));
      break;
    }
  }

#ifdef DEBUG_MALLOC
  dmalloc_describe_location(memblock, location, indent);
#endif
}

static void describe_marker(struct marker *m)
{
  if (m)
    fprintf(stderr, "marker at %p: flags=0x%06x, refs=%d, weak=%d, "
	    "xrefs=%d, saved=%d, cycle=%d, link=%p\n",
	    m, m->flags, m->refs, m->weak_refs,
	    m->xrefs, m->saved_refs, m->cycle, m->link);
  else
    fprintf(stderr, "no marker\n");
}

void debug_gc_fatal(void *a, int flags, const char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);

  fprintf(stderr, "**");
  (void) VFPRINTF(stderr, fmt, args);

  describe(a);
  if (flags & 1) locate_references(a);
  if (flags & 2)
    fatal_after_gc = "Fatal in garbage collector.\n";
  else
    debug_fatal("Fatal in garbage collector.\n");
}

static void gdb_gc_stop_here(void *a, int weak)
{
  fprintf(stderr,"***One %sref found%s.\n",
	  weak ? "weak " : "",
	  found_where?found_where:"");
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

TYPE_FIELD debug_gc_check_weak_svalues(struct svalue *s, int num, TYPE_T t, void *data)
{
  TYPE_FIELD ret;
  found_in=data;
  found_in_type=t;
  ret=gc_check_weak_svalues(s,num);
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

void debug_gc_check_weak_short_svalue(union anything *u, TYPE_T type, TYPE_T t, void *data)
{
  found_in=data;
  found_in_type=t;
  gc_check_weak_short_svalue(u,type);
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
  struct marker *m;

  if(depth<0) return;

  if ((m = find_marker(a))) {
    fprintf(stderr,"%*s**Got gc ",indent,"");
    describe_marker(m);
  }

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

      {
	struct object *o;
	for (o = first_object; o && o != (struct object *) a; o = o->next) {}
	if (!o)
	  fprintf(stderr,"%*s**The object is not on the object link list.\n",indent,"");
	for (o = objects_to_destruct; o && o != (struct object *) a; o = o->next) {}
	if (o)
	  fprintf(stderr,"%*s**The object is on objects_to_destruct.\n",indent,"");
      }

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

    case T_MULTISET:
      fprintf(stderr,"%*s**Describing array of multiset:\n",indent,"");
      debug_dump_array(((struct multiset *)a)->ind);
      break;

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
  if (!a) fatal("Got null pointer.\n");

  m = find_marker(a);
  if (Pike_in_gc == GC_PASS_PRETOUCH) {
    if (m) gc_fatal(a, 0, "Object touched twice.\n");
    get_marker(a)->flags |= GC_TOUCHED;
  }
  else if (Pike_in_gc == GC_PASS_POSTTOUCH) {
    if (!*(INT32 *) a)
      gc_fatal(a, 1, "Found a thing without refs.\n");
    if (m) {
      if (!(m->flags & GC_TOUCHED))
	gc_fatal(a, 2, "An existing but untouched marker found "
		 "for object in linked lists.\n");
      else if (m->flags & (GC_ON_STACK|GC_IN_REC_LIST|GC_DONT_POP|
			   GC_LIVE_RECURSE|GC_WEAK_REF|GC_STRONG_REF))
	gc_fatal(a, 2, "Marker still got flag from recurse list.\n");
      else if (m->flags & GC_MARKED)
	return;
      else if (!(m->flags & GC_NOT_REFERENCED) || m->flags & GC_XREFERENCED)
	gc_fatal(a, 3, "A thing with external references "
		 "got missed by mark pass.\n");
      else if (!(m->flags & GC_CYCLE_CHECKED))
	gc_fatal(a, 2, "A thing was missed by "
		 "both mark and cycle check pass.\n");
      else if (!(m->flags & GC_IS_REFERENCED))
	gc_fatal(a, 2, "An unreferenced thing "
		 "got missed by gc_is_referenced().\n");
      else if (!(m->flags & GC_DO_FREE))
	gc_fatal(a, 2, "An unreferenced thing "
		 "got missed by gc_do_free().\n");
      else if (m->flags & GC_GOT_EXTRA_REF)
	gc_fatal(a, 2, "A thing still got an extra ref.\n");
      else if (m->weak_refs == -1)
	gc_fatal(a, 3, "A thing which had only weak references is "
		 "still around after gc.\n");
      else if (!(m->flags & GC_LIVE))
	gc_fatal(a, 3, "A thing to garb is still around.\n");
    }
  }
  else
    fatal("debug_gc_touch() used in invalid gc pass.\n");
}

static INLINE struct marker *gc_check_debug(void *a, int weak)
{
  struct marker *m;

  if (!a) fatal("Got null pointer.\n");
  if(check_for)
  {
    if(check_for == a)
    {
      gdb_gc_stop_here(a, weak);
    }
    return 0;
  }

  if (Pike_in_gc != GC_PASS_CHECK)
    fatal("gc check attempted in invalid pass.\n");

  m = get_marker(a);

  if (!*(INT32 *)a)
    gc_fatal(a, 1, "GC check on thing without refs.\n");
  if (m->saved_refs != -1 && m->saved_refs != *(INT32 *)a)
    gc_fatal(a, 1, "Refs changed in gc check pass.\n");
  m->saved_refs = *(INT32 *)a;
  if (m->refs + m->xrefs >= *(INT32 *) a)
    /* m->refs will be incremented by the caller. */
    gc_fatal(a, 1, "Thing is getting more internal refs than refs.\n");
  checked++;

  return m;
}

#endif /* PIKE_DEBUG */

INT32 real_gc_check(void *a)
{
  struct marker *m;
  INT32 ret;

#ifdef PIKE_DEBUG
  if (!(m = gc_check_debug(a, 0))) return 0;
#else
  m = get_marker(a);
#endif

  ret = add_ref(m);
  if (m->refs >= *(INT32 *) a)
    m->flags |= GC_NOT_REFERENCED;
  return ret;
}

INT32 real_gc_check_weak(void *a)
{
  struct marker *m;
  INT32 ret;

#ifdef PIKE_DEBUG
  if (!(m = gc_check_debug(a, 1))) return 0;
  if (m->weak_refs == -1)
    gc_fatal(a, 1, "Thing has already reached threshold for weak free.\n");
  if (m->weak_refs >= *(INT32 *) a)
    gc_fatal(a, 1, "Thing has gotten more weak refs than refs.\n");
  if (m->weak_refs > m->refs + 1)
    gc_fatal(a, 1, "Thing has gotten more weak refs than internal refs.\n");
#else
  m = get_marker(a);
#endif

  m->weak_refs++;
  if (m->weak_refs >= *(INT32 *) a)
    m->weak_refs = -1;

  ret = add_ref(m);
  if (m->refs >= *(INT32 *) a)
    m->flags |= GC_NOT_REFERENCED;
  return ret;
}

static void init_gc(void)
{
  init_marker_hash();
}

static void exit_gc(void)
{
#ifdef DO_PIKE_CLEANUP
  int e=0;
  struct marker *h;
  for(e=0;e<marker_hash_table_size;e++)
    while(marker_hash_table[e])
      remove_marker(marker_hash_table[e]->data);
#endif
  exit_marker_hash();
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

void gc_add_extra_ref(void *a)
{
  struct marker *m = get_marker(a);
  if (m->flags & GC_GOT_EXTRA_REF)
    gc_fatal(a, 0, "Thing already got an extra gc ref.\n");
  m->flags |= GC_GOT_EXTRA_REF;
  gc_extra_refs++;
  ++*(INT32 *) a;
}

void gc_free_extra_ref(void *a)
{
  struct marker *m = get_marker(a);
  if (!(m->flags & GC_GOT_EXTRA_REF))
    gc_fatal(a, 0, "Thing haven't got an extra gc ref.\n");
  m->flags &= ~GC_GOT_EXTRA_REF;
  gc_extra_refs--;
}

int debug_gc_is_referenced(void *a)
{
  struct marker *m;
  if (!a) fatal("Got null pointer.\n");
  if (Pike_in_gc != GC_PASS_MARK)
    fatal("gc_is_referenced() called in invalid gc pass.\n");

  if (gc_debug) {
    m = find_marker(a);
    if ((!m || !(m->flags & GC_TOUCHED)) &&
	!safe_debug_findstring((struct pike_string *) a))
      gc_fatal(a, 0, "Doing gc_is_referenced() on invalid object.\n");
    if (!m) m = get_marker(a);
  }
  else m = get_marker(a);

  if (m->flags & GC_IS_REFERENCED)
    gc_fatal(a, 0, "gc_is_referenced() called twice for thing.\n");
  m->flags |= GC_IS_REFERENCED;

  return !(m->flags & GC_NOT_REFERENCED);
}

int gc_external_mark3(void *a, void *in, char *where)
{
  struct marker *m;
  if (!a) fatal("Got null pointer.\n");
  if (Pike_in_gc != GC_PASS_CHECK && Pike_in_gc != GC_PASS_LOCATE)
    fatal("gc_external_mark() called in invalid gc pass.\n");

  if(check_for)
  {
    if(a==check_for)
    {
      char *tmp=found_where;
      void *tmp2=found_in;

      if(where) found_where=where;
      if(in) found_in=in;

      gdb_gc_stop_here(a, 0);

      found_where=tmp;
      found_in=tmp2;

      return 1;
    }
    return 0;
  }
  m=get_marker(a);
  m->xrefs++;
  m->flags|=GC_XREFERENCED;
  if(Pike_in_gc == GC_PASS_CHECK &&
     (m->refs + m->xrefs > *(INT32 *)a ||
      (m->saved_refs != -1 && m->saved_refs != *(INT32 *)a)))
    gc_fatal(a, 1, "Ref counts are wrong.\n");
  return 0;
}

int gc_do_weak_free(void *a)
{
  struct marker *m;

  if (!a) fatal("Got null pointer.\n");
  if (Pike_in_gc != GC_PASS_MARK && Pike_in_gc != GC_PASS_CYCLE)
    fatal("gc_do_weak_free() called in invalid gc pass.\n");
  if (gc_debug) {
    if (!(m = find_marker(a)))
      gc_fatal(a, 0, "gc_do_weak_free() got unknown object.\n");
  }
  else m = get_marker(a);
  debug_malloc_touch(a);

  if (m->weak_refs > m->refs)
    gc_fatal(a, 0, "More weak references than internal references.\n");

  return m->weak_refs == -1;
}

#endif /* PIKE_DEBUG */

int gc_mark(void *a)
{
  struct marker *m = get_marker(debug_malloc_pass(a));

#ifdef PIKE_DEBUG
  if (!a) fatal("Got null pointer.\n");
  if (Pike_in_gc != GC_PASS_MARK)
    fatal("gc mark attempted in invalid pass.\n");
  if (!*(INT32 *) a)
    gc_fatal(a, 0, "Marked a thing without refs.\n");
#endif

  if(m->flags & GC_MARKED)
  {
    return 0;
  }else{
    m->flags = (m->flags & ~GC_NOT_REFERENCED) | GC_MARKED;
    DO_IF_DEBUG(marked++);
    return 1;
  }
}

#ifdef GC_CYCLE_DEBUG
static int gc_cycle_indent = 0;
#define CYCLE_DEBUG_MSG(M, TXT) do {					\
  fprintf(stderr, "%*s%-33s %p [%p] ", gc_cycle_indent, "",		\
	  (TXT), (M) ? (M)->data : 0, gc_rec_last->data);		\
  describe_marker(M);							\
} while (0)
#else
#define CYCLE_DEBUG_MSG(M, TXT) do {} while (0)
#endif

static struct marker *break_cycle (struct marker *beg, struct marker *pos)
{
  /* There's a cycle from beg to gc_rec_last which should be broken at
   * pos. Do it by switching places of the markers before and after pos. */
  struct marker *p, *q;
#ifdef PIKE_DEBUG
  if (beg == pos)
    gc_fatal(beg->data, 0, "Cycle already broken at requested position.\n");
#endif

  if (beg->cycle) {
    for (q = &rec_list; q->link->cycle != beg->cycle; q = q->link) {}
    beg = q->link;
    if (beg->cycle == pos->cycle) {
      /* Breaking something previously marked as a cycle. Clear it
       * since we're no longer sure it's an ambigious cycle. */
      unsigned cycle = beg->cycle;
      for (p = beg; p && p->cycle == cycle; p = p->link)
	p->cycle = 0;
    }
  }
  else
    for (q = &rec_list; q->link != beg; q = q->link) {}

  q->link = pos;

  CYCLE_DEBUG_MSG(beg, "> break_cycle, begin at");

  for (p = beg; p->link != pos; p = p->link) {}

  for (q = pos;; q = q->link) {
    q->flags |= GC_MOVED_BACK|GC_DONT_POP;
    CYCLE_DEBUG_MSG(q, "> break_cycle, mark for don't pop");
    if (q == gc_rec_last) break;
  }

  {
    struct marker *m = q->link;
    q->link = beg;
    p->link = m;
  }

  if (beg->flags & GC_WEAK_REF) {
    beg->flags &= ~GC_WEAK_REF;
    pos->flags |= GC_WEAK_REF;
    CYCLE_DEBUG_MSG(pos, "> break_cycle, moved weak flag");
  }

  return beg;
}

int gc_cycle_push(void *x, struct marker *m, int weak)
{
#ifdef PIKE_DEBUG
  if (!x) fatal("Got null pointer.\n");
  if (m->data != x) fatal("Got wrong marker.\n");
  if (Pike_in_gc != GC_PASS_CYCLE)
    fatal("GC cycle push attempted in invalid pass.\n");
  if (gc_debug && !(m->flags & GC_TOUCHED))
    gc_fatal(x, 0, "gc_cycle_push() called for untouched thing.\n");
  if ((!(m->flags & GC_NOT_REFERENCED) || m->flags & GC_MARKED) &&
      *(INT32 *) x)
    gc_fatal(x, 1, "Got a referenced marker to gc_cycle_push.\n");
  if (m->flags & GC_XREFERENCED)
    gc_fatal(x, 1, "Doing cycle check in externally referenced thing "
	     "missed in mark pass.\n");
  if (gc_debug) {
    struct array *a;
    struct object *o;
    struct program *p;
    struct mapping *m;
    struct multiset *l;
    for(a = gc_internal_array; a != &empty_array; a = a->next)
      if(a == (struct array *) x) goto on_gc_internal_lists;
    for(o = gc_internal_object; o; o = o->next)
      if(o == (struct object *) x) goto on_gc_internal_lists;
    for(p = gc_internal_program; p; p = p->next)
      if(p == (struct program *) x) goto on_gc_internal_lists;
    for(m = gc_internal_mapping; m; m = m->next)
      if(m == (struct mapping *) x) goto on_gc_internal_lists;
    for(l = gc_internal_multiset; l; l = l->next)
      if(l == (struct multiset *) x) goto on_gc_internal_lists;
    gc_fatal(x, 0, "gc_cycle_check() called for thing not on gc_internal lists.\n");
  on_gc_internal_lists:
    ; /* We must have a least one expression after a label! - Hubbe */
  }
#endif

  if (gc_rec_last->flags & GC_LIVE_RECURSE) {
#ifdef PIKE_DEBUG
    if (!(gc_rec_last->flags & GC_LIVE))
      gc_fatal(x, 0, "Doing live recursion from a dead thing.\n");
#endif

    if (m->flags & GC_CYCLE_CHECKED) {
      if (!(m->flags & GC_LIVE)) {
	/* Only recurse through things already handled; we'll get to the
	 * other later in the normal recursion. */
#ifdef PIKE_DEBUG
	if (m->flags & GC_LIVE_RECURSE)
	  gc_fatal(x, 0, "Mark live recursion attempted twice into thing.\n");
#endif
	goto live_recurse;
      }
      CYCLE_DEBUG_MSG(m, "gc_cycle_push, no live recurse");
    }

    else {
      /* Nothing more to do. Unwind the live recursion. */
      int flags;
      CYCLE_DEBUG_MSG(m, "gc_cycle_push, live rec done");
      do {
	gc_rec_last->flags &= ~GC_LIVE_RECURSE;
#ifdef GC_CYCLE_DEBUG
	gc_cycle_indent -= 2;
	CYCLE_DEBUG_MSG(gc_rec_last, "> gc_cycle_push, unwinding live");
#endif
	gc_rec_last = (struct marker *)
	  dequeue_lifo(&gc_mark_queue, (queue_call) gc_set_rec_last);
#ifdef PIKE_DEBUG
	if (!gc_rec_last)
	  fatal("Expected a gc_set_rec_last entry in gc_mark_queue.\n");
#endif
      } while (gc_rec_last->flags & GC_LIVE_RECURSE);
      if (!dequeue_lifo(&gc_mark_queue, (queue_call) gc_cycle_pop)) {
#ifdef PIKE_DEBUG
	fatal("Expected a gc_cycle_pop entry in gc_mark_queue.\n");
#endif
      }
    }

    return 0;
  }

#ifdef PIKE_DEBUG
  if (weak >= 0 && gc_rec_last->flags & GC_FOLLOWED_STRONG)
    gc_fatal(x, 0, "Followed strong link too early.\n");
  if (weak < 0) gc_rec_last->flags |= GC_FOLLOWED_STRONG;
#endif

  if (m->flags & GC_IN_REC_LIST) { /* A cyclic reference is found. */
#ifdef PIKE_DEBUG
    if (m == &rec_list || gc_rec_last == &rec_list)
      gc_fatal(x, 0, "Cyclic ref involves dummy rec_list marker.\n");
#endif

    if (m != gc_rec_last) {
      struct marker *p, *weak_ref = 0, *nonstrong_ref = 0;
      if (!weak) {
	struct marker *q;
	CYCLE_DEBUG_MSG(m, "gc_cycle_push, search normal");
	for (q = m, p = m->link; p; q = p, p = p->link) {
	  if (p->flags & (GC_WEAK_REF|GC_STRONG_REF)) {
	    if (p->flags & GC_WEAK_REF) weak_ref = p;
	    else if (!nonstrong_ref) nonstrong_ref = q;
	  }
	  if (p == gc_rec_last) break;
	}
      }

      else if (weak < 0) {
	CYCLE_DEBUG_MSG(m, "gc_cycle_push, search strong");
	for (p = m->link; p; p = p->link) {
	  if (p->flags & GC_WEAK_REF) weak_ref = p;
	  if (!(p->flags & GC_STRONG_REF)) nonstrong_ref = p;
	  if (p == gc_rec_last) break;
	}
#ifdef PIKE_DEBUG
	if (p == gc_rec_last && !nonstrong_ref)
	  gc_fatal(x, 0, "Only strong links in cycle.\n");
#endif
      }

      else {
	struct marker *q;
	CYCLE_DEBUG_MSG(m, "gc_cycle_push, search weak");
	for (q = m, p = m->link; p; q = p, p = p->link) {
	  if (!(p->flags & GC_WEAK_REF) && !nonstrong_ref) nonstrong_ref = q;
	  if (p == gc_rec_last) break;
	}
      }

      if (p == gc_rec_last) {	/* It was a backward reference. */
	if (weak_ref) {
	  /* The backward link is normal or strong and there are one
	   * or more weak links in the cycle. Let's break it at the
	   * last one (to ensure that a sequence of several weak links
	   * are broken at the last one). */
	  CYCLE_DEBUG_MSG(weak_ref, "gc_cycle_push, weak break");
	  break_cycle (m, weak_ref);
	}

	else if (weak < 0) {
	  /* The backward link is strong. Must break the cycle at the
	   * last nonstrong link. */
	  if (m->flags & GC_STRONG_REF)
	    nonstrong_ref->flags =
	      (nonstrong_ref->flags & ~GC_WEAK_REF) | GC_STRONG_REF;
	  else
	    m->flags = (m->flags & ~GC_WEAK_REF) | GC_STRONG_REF;
	  CYCLE_DEBUG_MSG(nonstrong_ref, "gc_cycle_push, nonstrong break");
	  break_cycle (m, nonstrong_ref);
	}

	else if (nonstrong_ref) {
	  /* Either a nonweak cycle with a strong link in it or a weak
	   * cycle with a nonweak link in it. Break before the first
	   * link that's stronger than the others. */
	  if (nonstrong_ref != m) {
	    CYCLE_DEBUG_MSG(nonstrong_ref, "gc_cycle_push, weaker break");
	    break_cycle (m, nonstrong_ref);
	  }
	}

	else if (!weak) {
	  /* A normal cycle which will be destructed in arbitrary
	   * order. For reasons having to do with strong links we
	   * can't mark weak cycles this way. */
	  unsigned cycle = m->cycle ? m->cycle : ++last_cycle;
	  if (cycle == gc_rec_last->cycle)
	    CYCLE_DEBUG_MSG(m, "gc_cycle_push, old cycle");
	  else {
	    unsigned replace_cycle = gc_rec_last->cycle;
	    CYCLE_DEBUG_MSG(m, "gc_cycle_push, cycle");
	    for (p = m;; p = p->link) {
	      p->cycle = cycle;
	      CYCLE_DEBUG_MSG(p, "> gc_cycle_push, mark cycle");
	      if (p == gc_rec_last) break;
	    }
	    if (replace_cycle && replace_cycle != cycle)
	      for (p = p->link; p && p->cycle == replace_cycle; p = p->link) {
		p->cycle = cycle;
		CYCLE_DEBUG_MSG(p, "> gc_cycle_push, re-mark cycle");
	      }}}}		/* Mmm.. lisp ;) */

      else {			/* A forward reference. */
	CYCLE_DEBUG_MSG(m, "gc_cycle_push, forward ref");
	if (m->flags & GC_ON_STACK &&
	    !(gc_rec_last->cycle && gc_rec_last->cycle == m->cycle)) {
	  /* Since it's not part of the same cycle it's a reference to
	   * a marker that has been swapped further down the list by
	   * break_cycle(). In that case we must mark gc_rec_last to
	   * stay on the list. */
	  gc_rec_last->flags |= GC_DONT_POP;
	  CYCLE_DEBUG_MSG(gc_rec_last, "gc_cycle_push, mark for don't pop");
	}
      }
    }
  }

  else
    if (!(m->flags & GC_CYCLE_CHECKED)) {
      struct marker *p;
      m->flags |= gc_rec_last->flags & GC_LIVE;
      if (weak) {
	if (weak > 0) m->flags |= GC_WEAK_REF;
	else m->flags |= GC_STRONG_REF;
      }
#ifdef PIKE_DEBUG
      cycle_checked++;
      if (m->flags & GC_ON_STACK)
	gc_fatal(x, 0, "Recursing twice into thing.\n");
      if (m->flags & GC_LIVE_RECURSE)
	gc_fatal(x, 0, "GC_LIVE_RECURSE set in normal recursion.\n");
#endif

      p = gc_rec_last;
      if (p->cycle)
	for (; p->link && p->link->cycle == p->cycle; p = p->link) {}
      m->link = p->link;
      p->link = m;
      m->flags |= GC_CYCLE_CHECKED|GC_ON_STACK|GC_IN_REC_LIST;
      m->cycle = 0;

#ifdef GC_CYCLE_DEBUG
      if (weak > 0) CYCLE_DEBUG_MSG(m, "gc_cycle_push, recurse weak");
      else if (weak < 0) CYCLE_DEBUG_MSG(m, "gc_cycle_push, recurse strong");
      else CYCLE_DEBUG_MSG(m, "gc_cycle_push, recurse");
      gc_cycle_indent += 2;
#endif
      gc_rec_last = m;
      return 1;
    }

  /* Should normally not recurse now, but got to do that anyway if we
   * must mark live things. */
  if (!(gc_rec_last->flags & GC_LIVE) || m->flags & GC_LIVE) {
    CYCLE_DEBUG_MSG(m, "gc_cycle_push, no recurse");
    return 0;
  }

live_recurse:
#ifdef PIKE_DEBUG
  if (m->flags & GC_LIVE)
    fatal("Shouldn't live recurse when there's nothing to do.\n");
#endif
  m->flags |= GC_LIVE|GC_LIVE_RECURSE;

  if (m->flags & GC_GOT_DEAD_REF) {
    /* A thing previously popped as dead is now being marked live.
     * Have to remove the extra ref added by gc_cycle_pop(). */
    gc_free_extra_ref(x);
    if (!--*(INT32 *) x) {
#ifdef PIKE_DEBUG
      gc_fatal(x, 0, "Thing got zero refs after removing the dead gc ref.\n");
#endif
    }
  }

  /* Recurse without linking m onto rec_list. */
#ifdef GC_CYCLE_DEBUG
  CYCLE_DEBUG_MSG(m, "gc_cycle_push, live recurse");
  gc_cycle_indent += 2;
#endif
  gc_rec_last = m;
  return 1;
}

/* Add an extra ref when a dead thing is popped. It's taken away in
 * the free pass. This is done to not refcount garb the cycles
 * themselves recursively, which in bad cases can consume a lot of C
 * stack. */
#define ADD_REF_IF_DEAD(M)						\
  if (!(M->flags & GC_LIVE)) {						\
    DO_IF_DEBUG(							\
      if (M->flags & GC_GOT_DEAD_REF)					\
	gc_fatal(M->data, 0, "A thing already got an extra dead cycle ref.\n"); \
    );									\
    gc_add_extra_ref(M->data);						\
    M->flags |= GC_GOT_DEAD_REF;					\
  }

void gc_cycle_pop(void *a)
{
  struct marker *m = find_marker(a), *p, *q, *base;

#ifdef PIKE_DEBUG
  if (Pike_in_gc != GC_PASS_CYCLE)
    fatal("GC cycle pop attempted in invalid pass.\n");
  if (!(m->flags & GC_CYCLE_CHECKED))
    gc_fatal(a, 0, "Marker being popped doesn't have GC_CYCLE_CHECKED.\n");
  if ((!(m->flags & GC_NOT_REFERENCED) || m->flags & GC_MARKED) &&
      *(INT32 *) a)
    gc_fatal(a, 1, "Got a referenced marker to gc_cycle_pop.\n");
  if (m->flags & GC_XREFERENCED)
    gc_fatal(a, 1, "Doing cycle check in externally referenced thing "
	     "missed in mark pass.\n");
#endif
#ifdef GC_CYCLE_DEBUG
  gc_cycle_indent -= 2;
#endif

  if (m->flags & GC_LIVE_RECURSE) {
    m->flags &= ~GC_LIVE_RECURSE;
    CYCLE_DEBUG_MSG(m, "gc_cycle_pop, live pop");
    return;
  }

#ifdef PIKE_DEBUG
  if (!(m->flags & GC_ON_STACK))
    gc_fatal(a, 0, "Marker being popped isn't on stack.\n");
  if (m->flags & GC_GOT_DEAD_REF)
    gc_fatal(a, 0, "Didn't expect a dead extra ref.\n");
#endif
  m->flags &= ~GC_ON_STACK;

  if (m->flags & GC_DONT_POP) {
#ifdef PIKE_DEBUG
    if (!m->link)
      gc_fatal(a, 0, "Found GC_DONT_POP marker on top of list.\n");
#endif
    CYCLE_DEBUG_MSG(m, "gc_cycle_pop, keep in list");
    if (!(m->flags & GC_MOVED_BACK)) {
      gc_rec_last->flags |= GC_DONT_POP;
      CYCLE_DEBUG_MSG(gc_rec_last, "gc_cycle_pop, propagate don't pop");
    }
    return;
  }

  for (base = gc_rec_last, p = base->link;; base = p, p = p->link) {
    if (base == m) {
#ifdef GC_CYCLE_DEBUG
      CYCLE_DEBUG_MSG(m, "gc_cycle_pop, keep cycle");
#endif
      return;
    }
    if (!(p->cycle && p->cycle == base->cycle))
      break;
  }

  q = base;
  while (1) {
    p = q->link;
#ifdef PIKE_DEBUG
    if (p->flags & (GC_ON_STACK|GC_LIVE_RECURSE))
      gc_fatal(p->data, 0, "Marker still got an on-stack flag at pop.\n");
#endif

    if (!p->cycle && !(p->flags & GC_LIVE_OBJ)) {
      ADD_REF_IF_DEAD(p);
      p->flags &= ~(GC_IN_REC_LIST|GC_DONT_POP|GC_WEAK_REF|GC_STRONG_REF);
      q->link = p->link;
      DO_IF_DEBUG(p->link = (struct marker *) -1);
      CYCLE_DEBUG_MSG(p, "gc_cycle_pop, pop off");
    }
    else q = p;

    if (p == m) break;
  }

  if (base != q) {
    q = base;
    while ((p = q->link)) {
#ifdef PIKE_DEBUG
      if (!(p->flags & GC_IN_REC_LIST))
	gc_fatal(p->data, 0, "Marker being cycle popped doesn't have GC_IN_REC_LIST.\n");
      if (p->flags & GC_GOT_DEAD_REF)
	gc_fatal(p->data, 0, "Didn't expect a dead extra ref.\n");
#endif
      p->flags &= ~GC_IN_REC_LIST;

      p->flags &= ~(GC_DONT_POP|GC_WEAK_REF|GC_STRONG_REF);
      if (p->flags & GC_LIVE_OBJ) {
	/* This extra ref is taken away in the kill pass. */
	gc_add_extra_ref(p->data);
	q = p;
	CYCLE_DEBUG_MSG(p, "gc_cycle_pop, put on kill list");
      }
      else {
	ADD_REF_IF_DEAD(p);
	q->link = p->link;
	DO_IF_DEBUG(p->link = (struct marker *) -1);
	CYCLE_DEBUG_MSG(p, "gc_cycle_pop, cycle pop off");
      }
    }

    q->link = kill_list;
    kill_list = base->link;
    base->link = p;
  }
}

void gc_set_rec_last(struct marker *m)
{
  gc_rec_last = m;
}

void do_gc_recurse_svalues(struct svalue *s, int num)
{
  gc_recurse_svalues(s, num);
}

void do_gc_recurse_short_svalue(union anything *u, TYPE_T type)
{
  gc_recurse_short_svalue(u, type);
}

int gc_do_free(void *a)
{
  struct marker *m;
#ifdef PIKE_DEBUG
  if (!a) fatal("Got null pointer.\n");
  if (Pike_in_gc != GC_PASS_FREE)
    fatal("gc free attempted in invalid pass.\n");
#endif

  m=find_marker(debug_malloc_pass(a));
  if (!m) return 0;		/* Object created after cycle pass. */

#ifdef PIKE_DEBUG
  if (*(INT32 *) a > !!(m->flags & GC_GOT_EXTRA_REF)) {
    if (!(m->flags & GC_NOT_REFERENCED) || m->flags & GC_MARKED)
      gc_fatal(a, 0, "gc_do_free() called for referenced thing.\n");
    if (gc_debug &&
	(m->flags & (GC_TOUCHED|GC_MARKED|GC_IS_REFERENCED)) == GC_TOUCHED)
      gc_fatal(a, 0, "gc_do_free() called without prior call to "
	       "gc_mark() or gc_is_referenced().\n");
  }
  if((m->flags & (GC_MARKED|GC_XREFERENCED)) == GC_XREFERENCED)
    gc_fatal(a, 1, "Thing with external reference missed in gc mark pass.\n");
  if ((m->flags & (GC_DO_FREE|GC_LIVE)) == GC_LIVE) live_ref++;
  m->flags |= GC_DO_FREE;
#endif

  return !(m->flags & GC_LIVE);
}

static void warn_bad_cycles()
{
  JMP_BUF uwp;
  struct array *obj_arr = 0;
  if (!SETJMP(uwp)) {
    struct marker *p;
    unsigned cycle = 0;
    obj_arr = allocate_array(0);
    for (p = kill_list; p;) {
      if ((cycle = p->cycle)) {
	push_object((struct object *) p->data);
	obj_arr = append_array(obj_arr, --sp);
      }
      p = p->link;
      if (p ? p->cycle != cycle : cycle) {
	if (obj_arr->size >= 2) {
	  push_constant_text("gc");
	  push_constant_text("bad_cycle");
	  push_array(obj_arr);
	  SAFE_APPLY_MASTER("runtime_warning", 3);
	  pop_stack();
	  obj_arr = allocate_array(0);
	}
	else obj_arr = resize_array(obj_arr, 0);
      }
      if (!p) break;
    }
  }
  UNSETJMP(uwp);
  if (obj_arr) free_array(obj_arr);
}

int do_gc(void)
{
  double tmp;
  int objs, pre_kill_objs;
  double multiplier;
  struct array *a;
  struct multiset *l;
  struct mapping *m;
  struct program *p;
  struct object *o;
#ifdef PIKE_DEBUG
#ifdef HAVE_GETHRTIME
  hrtime_t gcstarttime;
#endif
  unsigned destroy_count, obj_count;
#endif

  if(Pike_in_gc) return 0;
  init_gc();
  Pike_in_gc=GC_PASS_PREPARE;
#ifdef PIKE_DEBUG
  gc_debug = d_flag;
#endif

  destruct_objects_to_destruct();

  if(gc_evaluator_callback)
  {
    remove_callback(gc_evaluator_callback);
    gc_evaluator_callback=0;
  }

  objs=num_objects;
  last_cycle = 0;

#ifdef PIKE_DEBUG
  if(GC_VERBOSE_DO(1 ||) t_flag) {
    fprintf(stderr,"Garbage collecting ... ");
    GC_VERBOSE_DO(fprintf(stderr, "\n"));
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

  /* Thread switches, object alloc/free and any reference changes
   * (except by the gc itself) are disallowed now. */

#ifdef PIKE_DEBUG
  weak_freed = checked = marked = cycle_checked = live_ref = 0;
  if (gc_debug) {
    unsigned n;
    Pike_in_gc = GC_PASS_PRETOUCH;
    n = gc_touch_all_arrays();
    n += gc_touch_all_multisets();
    n += gc_touch_all_mappings();
    n += gc_touch_all_programs();
    n += gc_touch_all_objects();
    if (n != (unsigned) num_objects)
      fatal("Object count wrong before gc; expected %d, got %d.\n", num_objects, n);
    GC_VERBOSE_DO(fprintf(stderr, "| pretouch: %u things\n", n));
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

  GC_VERBOSE_DO(fprintf(stderr, "| check: %u references checked\n", checked));

  Pike_in_gc=GC_PASS_MARK;

  /* Anything after and including gc_internal_foo in the linked lists
   * are considered to lack external references. The mark pass move
   * externally referenced things in front of these pointers. */
  gc_internal_array = empty_array.next;
  gc_internal_multiset = first_multiset;
  gc_internal_mapping = first_mapping;
  gc_internal_program = first_program;
  gc_internal_object = first_object;

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
/*   if(gc_debug) */
/*     gc_mark_all_strings(); */

  GC_VERBOSE_DO(fprintf(stderr,
			"| mark: %u markers referenced,\n"
			"|       %u weak references freed, %d things really freed\n",
			marked, weak_freed, objs - num_objects));

  Pike_in_gc=GC_PASS_CYCLE;
#ifdef PIKE_DEBUG
  obj_count = num_objects;
#endif

  /* Now find all cycles in the internal structures */
  /* Note: The order between types here is normally not significant,
   * but the permuting destruct order tests in the testsuite won't be
   * really effective unless objects are handled first. :P */
  gc_cycle_check_all_objects();
  gc_cycle_check_all_arrays();
  gc_cycle_check_all_multisets();
  gc_cycle_check_all_mappings();
  gc_cycle_check_all_programs();

#ifdef PIKE_DEBUG
  if (gc_mark_queue.first)
    fatal("gc_mark_queue not empty at end of cycle check pass.\n");
  if (rec_list.link || gc_rec_last != &rec_list)
    fatal("Recurse list not empty or inconsistent after cycle check pass.\n");
#endif

  GC_VERBOSE_DO(fprintf(stderr,
			"| cycle: %u internal things visited, %u cycle ids used,\n"
			"|        %u weak references freed, %d things really freed\n",
			cycle_checked, last_cycle, weak_freed, obj_count - num_objects));

  /* Thread switches, object alloc/free and reference changes are
   * allowed again now. */

  Pike_in_gc=GC_PASS_FREE;
#ifdef PIKE_DEBUG
  weak_freed = 0;
  obj_count = num_objects;
#endif

  /* Now we free the unused stuff */
  gc_free_all_unreferenced_arrays();
  gc_free_all_unreferenced_multisets();
  gc_free_all_unreferenced_mappings();
  gc_free_all_unreferenced_programs();
  gc_free_all_unreferenced_objects();

  GC_VERBOSE_DO(fprintf(stderr, "| free: %d really freed, %u left with live references\n",
			obj_count - num_objects, live_ref));

  gc_internal_array = &empty_array;
  gc_internal_multiset = 0;
  gc_internal_mapping = 0;
  gc_internal_program = 0;
  gc_internal_object = 0;

#ifdef PIKE_DEBUG
  if(fatal_after_gc) fatal(fatal_after_gc);
#endif

  Pike_in_gc=GC_PASS_KILL;
  /* Destruct the live objects in cycles, but first warn about any bad
   * cycles. */
  pre_kill_objs = num_objects;
  if (last_cycle) {
    objs -= num_objects;
    warn_bad_cycles();
    objs += num_objects;
  }
#ifdef PIKE_DEBUG
  destroy_count = 0;
#endif
  for (; kill_list; kill_list = kill_list->link) {
    struct object *o = (struct object *) kill_list->data;
#ifdef PIKE_DEBUG
    if ((kill_list->flags & (GC_LIVE|GC_LIVE_OBJ)) != (GC_LIVE|GC_LIVE_OBJ))
      gc_fatal(o, 0, "Invalid thing in kill list.\n");
#endif
    GC_VERBOSE_DO(fprintf(stderr, "|   Killing %p with %d refs\n",
			  o, o->refs));
    destruct(o);
    free_object(o);
    gc_free_extra_ref(o);
#ifdef PIKE_DEBUG
    destroy_count++;
#endif
  }

  GC_VERBOSE_DO(fprintf(stderr, "| kill: %u objects killed, %d things really freed\n",
			destroy_count, pre_kill_objs - num_objects));

  Pike_in_gc=GC_PASS_DESTRUCT;
  /* Destruct objects on the destruct queue. */
  GC_VERBOSE_DO(obj_count = num_objects);
  destruct_objects_to_destruct();
  GC_VERBOSE_DO(fprintf(stderr, "| destruct: %d things really freed\n",
			obj_count - num_objects));

#ifdef PIKE_DEBUG
  if (gc_debug) {
    unsigned n;
    Pike_in_gc=GC_PASS_POSTTOUCH;
    n = gc_touch_all_arrays();
    n += gc_touch_all_multisets();
    n += gc_touch_all_mappings();
    n += gc_touch_all_programs();
    n += gc_touch_all_objects();
    if (n != (unsigned) num_objects)
      fatal("Object count wrong after gc; expected %d, got %d.\n", num_objects, n);
    GC_VERBOSE_DO(fprintf(stderr, "| posttouch: %u things\n", n));
    if(fatal_after_gc) fatal(fatal_after_gc);
  }
  if (gc_extra_refs)
    fatal("Lost track of %d extra refs to things in gc.\n", gc_extra_refs);
#endif

  Pike_in_gc=0;
  exit_gc();

  /* It's possible that more things got allocated in the kill pass
   * than were freed. The count before that is a better measurement
   * then. */
  if (pre_kill_objs < num_objects) objs -= pre_kill_objs;
  else objs -= num_objects;

  objects_freed += (double) objs;

  tmp=(double)num_objects;
  tmp=tmp * GC_CONST/100.0 * (objects_alloced+1.0) / (objects_freed+1.0);

  if(alloc_threshold + num_allocs <= tmp)
    tmp = (double)(alloc_threshold + num_allocs);

  if(tmp < MIN_ALLOC_THRESHOLD)
    tmp = (double)MIN_ALLOC_THRESHOLD;
  if(tmp > MAX_ALLOC_THRESHOLD)
    tmp = (double)MAX_ALLOC_THRESHOLD;

  alloc_threshold = (int)tmp;
  
  num_allocs=0;

#ifdef PIKE_DEBUG
  if(GC_VERBOSE_DO(1 ||) t_flag)
  {
#ifdef HAVE_GETHRTIME
    fprintf(stderr,"done (freed %ld of %ld objects), %ld ms.\n",
	    (long)objs,(long)objs + num_objects,
	    (long)((gethrtime() - gcstarttime)/1000000));
#else
    fprintf(stderr,"done (freed %ld of %ld objects)\n",
	    (long)objs,(long)objs + num_objects);
#endif
  }
#endif

#ifdef ALWAYS_GC
  ADD_GC_CALLBACK();
#else
  if(d_flag > 3) ADD_GC_CALLBACK();
#endif

  return objs;
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
