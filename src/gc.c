/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: gc.c,v 1.251 2004/04/06 15:37:55 nilsson Exp $
*/

#include "global.h"

struct callback *gc_evaluator_callback=0;

#include "array.h"
#include "multiset.h"
#include "mapping.h"
#include "object.h"
#include "program.h"
#include "stralloc.h"
#include "stuff.h"
#include "pike_error.h"
#include "pike_memory.h"
#include "pike_macros.h"
#include "pike_rusage.h"
#include "pike_types.h"
#include "time_stuff.h"
#include "constants.h"
#include "interpret.h"
#include "bignum.h"
#include "pike_threadlib.h"

#include "gc.h"
#include "main.h"
#include <math.h>

#include "block_alloc.h"

RCSID("$Id: gc.c,v 1.251 2004/04/06 15:37:55 nilsson Exp $");

int gc_enabled = 1;

/* These defaults are only guesses and hardly tested at all. Please improve. */
double gc_garbage_ratio_low = 0.2;
double gc_time_ratio = 0.05;
double gc_garbage_ratio_high = 0.5;

/* This slowness factor approximately corresponds to the average over
 * the last ten gc rounds. (0.9 == 1 - 1/10) */
double gc_average_slowness = 0.9;

#define GC_LINK_CHUNK_SIZE 64

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
 *    object before all children have been destructed.)
 *
 * The gc tries to detect and warn about cases where there are live
 * objects with no well defined order between them. There are cases
 * that are missed by this detection, though.
 *
 * Things that aren't live objects but are referenced from them are
 * still intact during this destruct pass, so it's entirely possible
 * to save them by adding external references to them. However, it's
 * not possible for live objects to save themselves or other live
 * objects; all live objects that didn't have external references at
 * the start of the gc pass will be destructed regardless of added
 * references.
 *
 * Things that have only weak external references at the start of the
 * gc pass will be freed. That's done before the live object destruct
 * pass. Internal weak references are however still intact.
 *
 * Note: Keep the doc for lfun::destroy up-to-date with the above.
 */

/* #define GC_VERBOSE */
/* #define GC_CYCLE_DEBUG */

/* #define GC_STACK_DEBUG */

#if defined(GC_VERBOSE) && !defined(PIKE_DEBUG)
#undef GC_VERBOSE
#endif
#ifdef GC_VERBOSE
#define GC_VERBOSE_DO(X) X
#else
#define GC_VERBOSE_DO(X)
#endif

int num_objects = 3;		/* Account for *_empty_array. */
ALLOC_COUNT_TYPE num_allocs =0;
ALLOC_COUNT_TYPE alloc_threshold = GC_MIN_ALLOC_THRESHOLD;
PMOD_EXPORT int Pike_in_gc = 0;
int gc_generation = 0;
time_t last_gc;
int gc_trace = 0, gc_debug = 0;
#ifdef DO_PIKE_CLEANUP
int gc_destruct_everything = 0;
#endif

struct gc_frame
{
  struct gc_frame *back;	/* Previous stack frame. */
  void *data;
  union {
    struct {			/* Pop frame. */
      struct gc_frame *prev;	/* Previous frame in rec_list. */
      struct gc_frame *next;	/* Next pointer in rec_list and kill_list. */
      unsigned INT16 cycle;	/* Cycle id number. */
    } pop;
    struct {			/* Link frame. */
      gc_cycle_check_cb *checkfn;
      int weak;
    } link;
    int free_extra_type;	/* Used on free_extra_list. The type of the thing. */
  } u;
  unsigned INT16 frameflags;
};

#define GC_POP_FRAME		0x01
#define GC_WEAK_REF		0x02
#define GC_STRONG_REF		0x04
#define GC_OFF_STACK		0x08
#define GC_ON_KILL_LIST		0x10
#ifdef PIKE_DEBUG
#define GC_LINK_FREED		0x20
#define GC_FOLLOWED_NONSTRONG	0x40
#endif

#undef BLOCK_ALLOC_NEXT
#define BLOCK_ALLOC_NEXT back

BLOCK_ALLOC(gc_frame,GC_LINK_CHUNK_SIZE)

#define PREV(frame) ((frame)->u.pop.prev)
#define NEXT(frame) ((frame)->u.pop.next)
#define CYCLE(frame) ((frame)->u.pop.cycle)

#ifdef PIKE_DEBUG
#define CHECK_POP_FRAME(frame) do {					\
  if ((frame)->frameflags & GC_LINK_FREED)				\
    gc_fatal((frame)->data, 0, "Accessing freed gc_frame.\n");		\
  if (!((frame)->frameflags & GC_POP_FRAME))				\
    gc_fatal((frame)->data, 0, #frame " is not a pop frame.\n");	\
  if (NEXT(PREV(frame)) != (frame))					\
    gc_fatal((frame)->data, 0,						\
	     "Pop frame pointers are inconsistent.\n");			\
} while (0)
#else
#define CHECK_POP_FRAME(frame) do {} while (0)
#endif

static struct gc_frame rec_list = {0, 0, {{0, 0, 0}}, GC_POP_FRAME};
static struct gc_frame *gc_rec_last = &rec_list, *gc_rec_top = 0;
static struct gc_frame *kill_list = 0;
static struct gc_frame *free_extra_list = 0; /* See note in gc_delayed_free. */

static unsigned last_cycle;
size_t gc_ext_weak_refs;

/* gc_frame objects are used as frames in a recursion stack during the
 * cycle check pass. gc_rec_top points to the current top of the
 * stack. When a thing is recursed, a pop frame is first pushed on the
 * stack and then the gc_cycle_check_* function fills in with link
 * frames for every reference the thing contains.
 *
 * rec_list is a double linked list of the pop frames on the stack,
 * and that list represents the current prospective destruct order.
 * gc_rec_last points at the last frame in the list and new frames are
 * linked in after it. A cycle is always treated as one atomic unit,
 * i.e. it's either popped whole or not at all. That means that
 * rec_list might contain frames that are no longer on the stack.
 *
 * A range of frames which always ends at the end of the list may be
 * rotated a number of slots to break a cyclic reference at a chosen
 * point. The stack of link frames are rotated simultaneously.
 *
 * Frames for live objects are linked into the beginning of kill_list
 * when they're popped from rec_list.
 *
 * The cycle check functions might recurse another round through the
 * frames that have been recursed already, to propagate the GC_LIVE
 * flag to things that have been found to be referenced from live
 * objects. rec_list is not touched at all in this extra round.
 */

static double objects_alloced = 0.0;
static double objects_freed = 0.0;
static double gc_time = 0.0, non_gc_time = 0.0;
static cpu_time_t last_gc_end_time = 0;
#if CPU_TIME_IS_THREAD_LOCAL == PIKE_NO
cpu_time_t auto_gc_time = 0;
#endif

/* These are only collected for the sake of gc_status. */
static double last_garbage_ratio = 0.0;
static enum {
  GARBAGE_RATIO_LOW, GARBAGE_RATIO_HIGH
} last_garbage_strategy = GARBAGE_RATIO_LOW;

struct callback_list gc_callbacks;

/* These callbacks are run early in the check pass of the gc and when
 * locate_references is called. They are typically used to mark
 * external references (using gc_mark_external) for debug purposes. */
struct callback *debug_add_gc_callback(callback_func call,
				 void *arg,
				 callback_func free_func)
{
  return add_to_callback(&gc_callbacks, call, arg, free_func);
}

static void init_gc(void);
static void gc_cycle_pop(void *a);

#undef BLOCK_ALLOC_NEXT
#define BLOCK_ALLOC_NEXT next

#undef INIT_BLOCK
#ifdef PIKE_DEBUG
#define INIT_BLOCK(X)					\
  (X)->flags=(X)->refs=(X)->weak_refs=(X)->xrefs=0;	\
  (X)->saved_refs=-1;					\
  (X)->frame = 0;
#else
#define INIT_BLOCK(X)					\
  (X)->flags=(X)->refs=(X)->weak_refs=0;		\
  (X)->frame = 0;
#endif

#ifdef PIKE_DEBUG
#undef get_marker
#define get_marker debug_get_marker
#undef find_marker
#define find_marker debug_find_marker
#endif

PTR_HASH_ALLOC_FIXED_FILL_PAGES(marker,2)

#if defined (PIKE_DEBUG) || defined (GC_MARK_DEBUG)
void *gc_found_in = NULL;
int gc_found_in_type = PIKE_T_UNKNOWN;
const char *gc_found_place = NULL;
#endif

#ifdef PIKE_DEBUG

#undef get_marker
#define get_marker(X) ((struct marker *) debug_malloc_pass(debug_get_marker(X)))
#undef find_marker
#define find_marker(X) ((struct marker *) debug_malloc_pass(debug_find_marker(X)))

int gc_in_cycle_check = 0;
static unsigned delayed_freed, weak_freed, checked, marked, cycle_checked, live_ref;
static unsigned max_gc_frames, num_gc_frames = 0, live_rec, frame_rot;
static unsigned gc_extra_refs = 0;

static unsigned max_tot_gc_frames = 0;
static unsigned tot_cycle_checked = 0, tot_live_rec = 0, tot_frame_rot = 0;

static int gc_is_watching = 0;

int attempt_to_identify(void *something, void **inblock)
{
  size_t i;
  struct array *a;
  struct object *o;
  struct program *p;
  struct mapping *m;
  struct multiset *mu;
  struct pike_type *t;
  struct callable *c;

  if (inblock) *inblock = 0;

  for (a = first_array; a; a = a->next) {
    if(a==(struct array *)something) return T_ARRAY;
  }

  for(o=first_object;o;o=o->next) {
    if(o==(struct object *)something)
      return T_OBJECT;
    if (o->storage && o->prog &&
	(char *) something >= o->storage &&
	(char *) something < o->storage + o->prog->storage_needed) {
      if (inblock) *inblock = (void *) o;
      return T_STORAGE;
    }
  }

  for(p=first_program;p;p=p->next)
    if(p==(struct program *)something)
      return T_PROGRAM;

  for(m=first_mapping;m;m=m->next)
    if(m==(struct mapping *)something)
      return T_MAPPING;
    else if (m->data == (struct mapping_data *) something)
      return T_MAPPING_DATA;

  for(mu=first_multiset;mu;mu=mu->next)
    if(mu==(struct multiset *)something)
      return T_MULTISET;
    else if (mu->msd == (struct multiset_data *) something)
      return T_MULTISET_DATA;

  if(safe_debug_findstring((struct pike_string *)something))
    return T_STRING;

  for (i = 0; i < pike_type_hash_size; i++)
    for (t = pike_type_hash[i]; t; t = t->next)
      if (t == (struct pike_type *) something)
	return T_TYPE;

  for (c = first_callable; c; c = c->next)
    if (c == (struct callable *) something)
      return T_STRUCT_CALLABLE;

  return PIKE_T_UNKNOWN;
}

void *check_for =0;
void *gc_svalue_location=0;
static size_t found_ref_count;

char *fatal_after_gc=0;

#ifdef DO_PIKE_CLEANUP
/* To keep the markers after the gc. Only used for the leak report at exit. */
int gc_keep_markers = 0;
int gc_external_refs_zapped = 0;
#endif

#define DESCRIBE_MEM 1
#define DESCRIBE_SHORT 4
#define DESCRIBE_NO_DMALLOC 8

/* type == -1 means that memblock is a char* and should be
 * really be printed..
 */
void describe_location(void *real_memblock,
		       int type,
		       void *location,
		       int indent,
		       int depth,
		       int flags)
{
  struct program *p;
  void *memblock=0, *descblock, *inblock;
  if(!location) return;
/*  fprintf(stderr,"**Location of (short) svalue: %p\n",location); */

  if(type!=-1 && real_memblock != (void *) -1) memblock=real_memblock;

#ifdef DEBUG_MALLOC
  if(memblock == 0 || type == -1)
  {
    extern void *dmalloc_find_memblock_base(void *);
    memblock=dmalloc_find_memblock_base(location);
  }
#endif

  if(type==PIKE_T_UNKNOWN)
    type=attempt_to_identify(memblock, &inblock);

  if(memblock)
    fprintf(stderr,"%*s-> from %s %p offset %"PRINTPTRDIFFT"d\n",
	    indent,"",
	    get_name_of_type(type),
	    memblock,
	    (char *)location - (char *)memblock);
  else
    fprintf(stderr,"%*s-> at location %p%s\n",
	    indent,"",
	    location,
	    real_memblock == (void *) -1 ? "" :  " in unknown memblock (mmaped?)");

 again:
  descblock = memblock;
  switch(type)
  {
    case PIKE_T_UNKNOWN:
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
      ptrdiff_t e;
      char *ptr=(char *)location;
      p=(struct program *)memblock;

      if(location == (void *)&p->prev)
	fprintf(stderr,"%*s  **In p->prev\n",indent,"");

      if(location == (void *)&p->next)
	fprintf(stderr,"%*s  **In p->next\n",indent,"");

      if(location == (void *)&p->parent)
	fprintf(stderr,"%*s  **In p->parent\n",indent,"");

      if(p->inherits &&
	 ptr >= (char *)p->inherits  &&
	 ptr < (char*)(p->inherits+p->num_inherits)) 
      {
	e=((char *)ptr - (char *)(p->inherits)) / sizeof(struct inherit);
	fprintf(stderr,"%*s  **In p->inherits[%"PRINTPTRDIFFT"d] (%s)\n",indent,"",
		e, p->inherits[e].name ? p->inherits[e].name->str : "no name");
	break;
      }

      if(p->constants &&
	 ptr >= (char *)p->constants  &&
	 ptr < (char*)(p->constants+p->num_constants))
      {
	e = ((char *)ptr - (char *)(p->constants)) /
	  sizeof(struct program_constant);
	fprintf(stderr,"%*s  **In p->constants[%"PRINTPTRDIFFT"d] (%s)\n",indent,"",
		e, p->constants[e].name ? p->constants[e].name->str : "no name");
	break;
      }


      if(p->identifiers && 
	 ptr >= (char *)p->identifiers  &&
	 ptr < (char*)(p->identifiers+p->num_identifiers))
      {
	e = ((char *)ptr - (char *)(p->identifiers)) /
	  sizeof(struct identifier);

	fprintf(stderr,"%*s  **In p->identifiers[%"PRINTPTRDIFFT"d] (%s)\n",indent,"",
		e, p->identifiers[e].name ?
		(strlen(p->identifiers[e].name->str)<100 ? p->identifiers[e].name->str : "Name too long or already freed.."  )
		: "no name");
	break;
      }

#define FOO(NUMTYPE,TYPE,ARGTYPE,NAME) \
    if(location == (void *)&p->NAME) fprintf(stderr,"%*s  **In p->" #NAME "\n",indent,""); \
    if(ptr >= (char *)p->NAME  && ptr<(char*)(p->NAME+p->PIKE_CONCAT(num_,NAME))) \
      fprintf(stderr,"%*s  **In p->" #NAME "[%"PRINTPTRDIFFT"d]\n",indent,"", \
	      ((char *)ptr - (char *)(p->NAME)) / sizeof(TYPE));
#include "program_areas.h"
      
      break;
    }

    case T_OBJECT:
    {
      struct object *o=(struct object *)memblock;
      struct program *p;

      if(o->prog && o->prog->flags & PROGRAM_USES_PARENT)
      {
	if(location == (void *)&PARENT_INFO(o)->parent)
	  fprintf(stderr,"%*s  **In o->parent\n",indent,"");
      }
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
	    if(tmp.name && !tmp.name->size_shift)
	      fprintf(stderr," (%s)",tmp.name->str);
	    fprintf(stderr,"\n");
	  }
	     
	}
      }
      break;
    }

    case T_STORAGE:
      fprintf(stderr, "%*s  **In storage of object\n", indent, "");
      break;

    case T_MULTISET:
      descblock = ((struct multiset *) memblock)->msd;
      /* FALL THROUGH */

    case T_MULTISET_DATA: {
      struct multiset_data *msd = (struct multiset_data *) descblock;
      union msnode *node = low_multiset_first (msd);
      struct svalue ind;
      int indval = msd->flags & MULTISET_INDVAL;
      for (; node; node = low_multiset_next (node)) {
	if (&node->i.ind == (struct svalue *) location) {
	  fprintf (stderr, "%*s  **In index ", indent, "");
	  print_svalue (stderr, low_use_multiset_index (node, ind));
	  fputc ('\n', stderr);
	  break;
	}
	else if (indval && &node->iv.val == (struct svalue *) location) {
	  fprintf(stderr, "%*s  **In value with index ", indent, "");
	  print_svalue(stderr, low_use_multiset_index (node, ind));
	  fputc('\n', stderr);
	  break;
	}
      }
      break;
    }

    case T_ARRAY:
    {
      struct array *a=(struct array *)descblock;
      struct svalue *s=(struct svalue *)location;

      if(location == (void *)&a->next)
	fprintf(stderr,"%*s  **In a->next\n",indent,"");

      if(location == (void *)&a->prev)
	fprintf(stderr,"%*s  **In a->prev\n",indent,"");

      if( s-ITEM(a) > 0)
	fprintf(stderr,"%*s  **In index number %"PRINTPTRDIFFT"d\n",indent,"",
		s-ITEM(a));
      break;
    }

    case T_MAPPING:
      descblock = ((struct mapping *) memblock)->data;
      /* FALL THROUGH */
    case T_MAPPING_DATA: {
      INT32 e;
      struct keypair *k;
      NEW_MAPPING_LOOP((struct mapping_data *) descblock)
	if (&k->ind == (struct svalue *) location) {
	  fprintf(stderr, "%*s  **In index ", indent, "");
	  print_svalue(stderr, &k->ind);
	  fputc('\n', stderr);
	  break;
	}
	else if (&k->val == (struct svalue *) location) {
	  fprintf(stderr, "%*s  **In value with index ", indent, "");
	  print_svalue(stderr, &k->ind);
	  fputc('\n', stderr);
	  break;
	}
      break;
    }

    case T_PIKE_FRAME: {
      struct pike_frame *f = (struct pike_frame *) descblock;
      if (f->locals) {		/* Paranoia. */
	ptrdiff_t pos = (struct svalue *) location - f->locals;
	if (pos >= 0) {
	  if (pos < f->num_args)
	    fprintf (stderr, "%*s  **In argument %"PRINTPTRDIFFT"d\n",
		     indent, "", pos);
	  else
	    fprintf (stderr, "%*s  **At position %"PRINTPTRDIFFT"d among locals\n",
		     indent, "", pos - f->num_args);
	  /* Don't describe current_object for the frame. */
	  flags |= DESCRIBE_SHORT;
	}
      }
      break;
    }
  }

  if(memblock && depth>0)
    describe_something(memblock,type,indent+2,depth-1,flags,inblock);

#ifdef DEBUG_MALLOC
  /* FIXME: Is the following call correct?
   * Shouldn't the second argument be an offset?
   */
  /* dmalloc_describe_location(descblock, location, indent); */
  /* My attempt to fix it, although I'm not really sure: /mast */
  if (memblock)
    dmalloc_describe_location(memblock, (char *) location - (char *) memblock, indent);
#endif
}

static void describe_gc_frame(struct gc_frame *l)
{
  if (l->frameflags & GC_POP_FRAME)
    fprintf(stderr, "back=%p, prev=%p, next=%p, data=%p, cycle=%u, flags=0x%02x",
	    l->back, PREV(l), NEXT(l), l->data, CYCLE(l), l->frameflags);
  else
    fprintf(stderr, "LINK back=%p, data=%p, weak=%d, flags=0x%02x",
	    l->back, l->data, l->u.link.weak, l->frameflags);
}

static void describe_marker(struct marker *m)
{
  if (m) {
    fprintf(stderr, "marker at %p: flags=0x%05lx, refs=%d, weak=%d, "
	    "xrefs=%d, saved=%d, frame=%p",
	    m, (long) m->flags, m->refs, m->weak_refs,
	    m->xrefs, m->saved_refs, m->frame);
#ifdef PIKE_DEBUG
    if (m->frame) {
      fputs(" [", stderr);
      describe_gc_frame(m->frame);
      putc(']', stderr);
    }
#endif
    putc('\n', stderr);
  }
  else
    fprintf(stderr, "no marker\n");
}

#endif /* PIKE_DEBUG */

void debug_gc_fatal(void *a, int flags, const char *fmt, ...)
{
#ifdef PIKE_DEBUG
  struct marker *m;
#endif
  int orig_gc_pass = Pike_in_gc;
  va_list args;

  va_start(args, fmt);

  fprintf(stderr, "**");
  (void) VFPRINTF(stderr, fmt, args);

  va_end(args);

#ifdef PIKE_DEBUG
  if (a) {
    /* Temporarily jumping out of gc to avoid being catched in debug
     * checks in describe(). */
    Pike_in_gc = 0;
    describe(a);
  
    if (flags & 1) locate_references(a);

    m=find_marker(a);
    if(m)
    {
      fprintf(stderr,"** Describing marker for this thing.\n");
      describe(m);
    }else{
      fprintf(stderr,"** No marker found for this thing.\n");
    }
    Pike_in_gc = orig_gc_pass;
  }

  if (flags & 2)
    fatal_after_gc = "Fatal in garbage collector.\n";
  else
#endif
    debug_fatal("Fatal in garbage collector.\n");
}

#ifdef PIKE_DEBUG

static void gdb_gc_stop_here(void *a, int weak)
{
  found_ref_count++;
  fprintf(stderr,"***One %sref found%s. ",
	  weak ? "weak " : "",
	  gc_found_place ? gc_found_place : "");
  if (gc_found_in) {
    if (gc_svalue_location)
      describe_location(gc_found_in , gc_found_in_type, gc_svalue_location,0,1,
			DESCRIBE_SHORT);
    else {
      fputc('\n', stderr);
      describe_something(gc_found_in, gc_found_in_type, 0, 0, DESCRIBE_SHORT, 0);
    }
  }
  else
    fputc('\n', stderr);
  fprintf(stderr,"----------end------------\n");
}

void low_describe_something(void *a,
			    int t,
			    int indent,
			    int depth,
			    int flags,
			    void *inblock)
{
  struct program *p=(struct program *)a;
  struct marker *m;

  if(depth<0) return;

  if (marker_hash_table && (m = find_marker(a))) {
    fprintf(stderr,"%*s**Got gc ",indent,"");
    describe_marker(m);
  }

again:
  switch(t)
  {
    case T_STORAGE:
      if (!inblock) attempt_to_identify (a, &a);
      t = T_OBJECT;
      goto again;

    case T_FUNCTION:
      if(attempt_to_identify(a, 0) != T_OBJECT)
      {
	fprintf(stderr,"%*s**Builtin function!\n",indent,"");
	break;
      }
      /* FALL THROUGH */

    case T_OBJECT:
      p=((struct object *)a)->prog;
      if(p && (p->flags & PROGRAM_USES_PARENT))
      {
	fprintf(stderr,"%*s**Parent identifier: %d\n",indent,"",PARENT_INFO( ((struct object *)a) )->parent_identifier);
      }
      fprintf(stderr,"%*s**Program id: %d\n",indent,"",((struct object *)a)->program_id);

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
	p=id_to_program(((struct object *)a)->program_id);
	if(p)
	  fprintf(stderr,"%*s**The object is destructed but program found from id.\n",
		  indent,"");
	else
	  fprintf(stderr,"%*s**The object is destructed and program not found from id.\n",
		  indent,"");
      }

      if (p) {
	size_t inh_idx, var_idx, var_count = 0;

	fprintf (stderr, "%*s**Object variables:\n", indent, "");

	for (inh_idx = 0; inh_idx < p->num_inherits; inh_idx++) {
	  struct inherit *inh = p->inherits + inh_idx;
	  struct program *p2 = inh->prog;

	  if (inh->inherit_level) {
	    if (inh->name) {
	      fprintf (stderr, "%*s**%*s=== In inherit ",
		       indent, "", inh->inherit_level + 1, "");
	      print_short_svalue (stderr, (union anything *) &inh->name, T_STRING);
	      fprintf (stderr, ", program %d:\n", inh->prog->id);
	    }
	    else
	      fprintf (stderr, "%*s**%*s=== In nameless inherit, program %d:\n",
		       indent, "", inh->inherit_level + 1, "", inh->prog->id);
	  }

	  for (var_idx = 0; var_idx < p2->num_variable_index; var_idx++) {
	    struct identifier *id = p2->identifiers + p2->variable_index[var_idx];
	    void *ptr;

	    fprintf (stderr, "%*s**%*srtt: %-8s  name: ",
		     indent, "", inh->inherit_level + 1, "",
		     get_name_of_type (id->run_time_type));

	    if (id->name->size_shift)
	      print_short_svalue (stderr, (union anything *) &id->name, T_STRING);
	    else
	      fprintf (stderr, "%-20s", id->name->str);

	    fprintf (stderr, "  off: %4"PRINTPTRDIFFT"d  value: ",
		     inh->storage_offset + id->func.offset);

	    ptr = PIKE_OBJ_STORAGE ((struct object *) a) +
	      inh->storage_offset + id->func.offset;
	    if (id->run_time_type == T_MIXED)
	      print_svalue_compact (stderr, (struct svalue *) ptr);
	    else
	      print_short_svalue_compact (stderr, (union anything *) ptr,
					  id->run_time_type);

	    fputc ('\n', stderr);
	    var_count++;
	  }
	}

	if (!var_count)
	  fprintf (stderr, "%*s** (none)\n", indent, "");

	fprintf(stderr,"%*s**Describing program %p of object:\n",indent,"", p);
#ifdef DEBUG_MALLOC
	if ((int) p == 0x55555555)
	  fprintf(stderr, "%*s**Zapped program pointer.\n", indent, "");
	else
#endif
	  low_describe_something(p, T_PROGRAM, indent, depth,
				 depth ? flags : flags | DESCRIBE_SHORT, 0);

	if((p->flags & PROGRAM_USES_PARENT) &&
	   LOW_PARENT_INFO(((struct object *)a),p)->parent)
	{
	  if (depth) {
	    fprintf(stderr,"%*s**Describing parent of object:\n",indent,"");
	    describe_something( PARENT_INFO((struct object *)a)->parent, T_OBJECT,
				indent+2, depth-1,
				(flags | DESCRIBE_SHORT) & ~DESCRIBE_MEM,
				0);
	  }
	  else
	    fprintf (stderr, "%*s**Object got a parent.\n", indent, "");
	}else{
	  fprintf(stderr,"%*s**There is no parent (any longer?)\n",indent,"");
	}
      }
      break;

    case T_PROGRAM:
    {
      char *tmp;
      INT32 line;
      ptrdiff_t id_idx, id_count = 0;
      struct inherit *inh = p->inherits, *next_inh = p->inherits + 1;
      ptrdiff_t inh_id_end = p->num_identifier_references;

      fprintf(stderr,"%*s**Program id: %ld, flags: %x, parent id: %d\n",
	      indent,"", (long)(p->id), p->flags,
	      p->parent ? p->parent->id : -1);

      if(p->flags & PROGRAM_HAS_C_METHODS)
      {
	fprintf(stderr,"%*s**The program was written in C.\n",indent,"");
      }

      tmp = low_get_program_line_plain(p, &line, 1);
      if (tmp) {
	fprintf(stderr,"%*s**Location: %s:%ld\n",
		indent, "", tmp, (long)line);
	free (tmp);
      }

      if (!(flags & DESCRIBE_SHORT)) {
	fprintf (stderr, "%*s**Identifiers:\n", indent, "");

	for (id_idx = 0; id_idx < p->num_identifier_references; id_idx++) {
	  struct reference *id_ref = p->identifier_references + id_idx;
	  struct inherit *id_inh;
	  struct identifier *id;
	  const char *type;
	  char prot[100], descr[120];

	  while (next_inh < p->inherits + p->num_inherits &&
		 id_idx == next_inh->identifier_level) {
	    inh = next_inh++;
	    inh_id_end = inh->identifier_level + inh->prog->num_identifier_references;
	    if (inh->name) {
	      fprintf (stderr, "%*s**%*s=== In inherit ",
		       indent, "", inh->inherit_level + 1, "");
	      print_short_svalue (stderr, (union anything *) &inh->name, T_STRING);
	      fprintf (stderr, ", program %d:\n", inh->prog->id);
	    }
	    else
	      fprintf (stderr, "%*s**%*s=== In nameless inherit, program %d:\n",
		       indent, "", inh->inherit_level + 1, "", inh->prog->id);
	  }

	  while (id_idx == inh_id_end) {
	    int cur_lvl = inh->inherit_level;
	    if (inh->name) {
	      fprintf (stderr, "%*s**%*s=== End of inherit ",
		       indent, "", inh->inherit_level + 1, "");
	      print_short_svalue (stderr, (union anything *) &inh->name, T_STRING);
	      fputc ('\n', stderr);
	    }
	    else
	      fprintf (stderr, "%*s**%*s=== End of nameless inherit\n",
		       indent, "", inh->inherit_level + 1, "");
	    while (inh > p->inherits) { /* Paranoia. */
	      if ((--inh)->inherit_level < cur_lvl) break;
	    }
	    inh_id_end = inh->identifier_level + inh->prog->num_identifier_references;
	  }

	  if (id_ref->id_flags & ID_HIDDEN ||
	      (id_ref->id_flags & (ID_INHERITED|ID_PRIVATE)) ==
	      (ID_INHERITED|ID_PRIVATE)) continue;

	  id_inh = INHERIT_FROM_PTR (p, id_ref);
	  id = id_inh->prog->identifiers + id_ref->identifier_offset;

	  if (IDENTIFIER_IS_PIKE_FUNCTION (id->identifier_flags)) type = "fun";
	  else if (IDENTIFIER_IS_FUNCTION (id->identifier_flags)) type = "cfun";
	  else if (IDENTIFIER_IS_CONSTANT (id->identifier_flags)) type = "const";
	  else if (IDENTIFIER_IS_ALIAS (id->identifier_flags))    type = "alias";
	  else if (IDENTIFIER_IS_VARIABLE (id->identifier_flags)) type = "var";
	  else type = "???";

	  prot[0] = prot[1] = 0;
	  if (id_ref->id_flags & ID_PRIVATE) {
	    strcat (prot, ",pri");
	    if (!(id_ref->id_flags & ID_STATIC)) strcat (prot, ",!sta");
	  }
	  else
	    if (id_ref->id_flags & ID_STATIC) strcat (prot, ",sta");
	  if (id_ref->id_flags & ID_NOMASK)    strcat (prot, ",nom");
	  if (id_ref->id_flags & ID_PUBLIC)    strcat (prot, ",pub");
	  if (id_ref->id_flags & ID_PROTECTED) strcat (prot, ",pro");
	  if (id_ref->id_flags & ID_INLINE)    strcat (prot, ",inl");
	  if (id_ref->id_flags & ID_OPTIONAL)  strcat (prot, ",opt");
	  if (id_ref->id_flags & ID_EXTERN)    strcat (prot, ",ext");
	  if (id_ref->id_flags & ID_VARIANT)   strcat (prot, ",var");
	  if (id_ref->id_flags & ID_ALIAS)     strcat (prot, ",ali");

	  sprintf (descr, "%s: %s", type, prot + 1);
	  fprintf (stderr, "%*s**%*s%-18s name: ",
		   indent, "", id_inh->inherit_level + 1, "", descr);

	  if (id->name->size_shift)
	    print_short_svalue (stderr, (union anything *) &id->name, T_STRING);
	  else
	    fprintf (stderr, "%-20s", id->name->str);

	  if (id->identifier_flags & IDENTIFIER_C_FUNCTION)
	    fprintf (stderr, "  addr: %p", id->func.c_fun);
	  else if (IDENTIFIER_IS_VARIABLE (id->identifier_flags))
	    fprintf (stderr, "  rtt: %s  off: %"PRINTPTRDIFFT"d",
		     get_name_of_type (id->run_time_type), id->func.offset);
	  else if (IDENTIFIER_IS_PIKE_FUNCTION (id->identifier_flags))
	    fprintf (stderr, "  pc: %"PRINTPTRDIFFT"d", id->func.offset);
	  else if (IDENTIFIER_IS_CONSTANT (id->identifier_flags)) {
	    fputs ("  value: ", stderr);
	    print_svalue_compact (stderr, &id_inh->prog->constants[id->func.offset].sval);
	  }

	  fputc ('\n', stderr);
	  id_count++;
	}

	if (!id_count)
	  fprintf (stderr, "%*s** (none)\n", indent, "");
      }

      if(flags & DESCRIBE_MEM)
      {
#define FOO(NUMTYPE,TYPE,ARGTYPE,NAME) \
      fprintf(stderr, "%*s* " #NAME " %p[%"PRINTSIZET"u]\n", \
              indent, "", p->NAME, p->PIKE_CONCAT(num_,NAME));
#include "program_areas.h"
      }

      break;
    }

    case T_MULTISET_DATA: {
      int found = 0;
      struct multiset *l;
      for (l = first_multiset; l; l = l->next) {
	if (l->msd == (struct multiset_data *) a) {
	  fprintf(stderr, "%*s**Describing multiset %p for this data block:\n",
		  indent, "", l);
	  debug_dump_multiset(l);
	  found = 1;
	}
      }
      if (!found)
	fprintf (stderr, "%*s**Didn't find multiset for this data block!\n", indent, "");
      break;
    }

    case T_MULTISET:
      debug_dump_multiset((struct multiset *) a);
      break;

    case T_ARRAY:
      debug_dump_array((struct array *)a);
      break;

    case T_MAPPING_DATA:
    {
      int found = 0;
      struct mapping *m;
      for(m=first_mapping;m;m=m->next)
      {
	if(m->data == (struct mapping_data *)a)
	{
	  fprintf(stderr,"%*s**Describing mapping for this data block:\n",indent,"");
	  debug_dump_mapping((struct mapping *)m);
	  found = 1;
	}
      }
      if (!found)
	fprintf (stderr, "%*s**Didn't find mapping for this data block!\n", indent, "");
      break;
    }
    
    case T_MAPPING:
      debug_dump_mapping((struct mapping *)a);
      break;

    case T_STRING:
    {
      struct pike_string *s=(struct pike_string *)a;
      fprintf(stderr,"%*s**size_shift: %d, len: %"PRINTPTRDIFFT"d, hash: %"PRINTSIZET"x\n",
	      indent,"", s->len, s->size_shift, s->hval);
      if (!s->size_shift) {
	if(s->len>77)
	{
	  fprintf(stderr,"%*s** \"%60s\"...\n",indent,"",s->str);
	}else{
	  fprintf(stderr,"%*s** \"%s\"\n",indent,"",s->str);
	}
      }
      break;
    }

    case T_PIKE_FRAME: {
      struct pike_frame *f = (struct pike_frame *) a;
      do {
	if (f->current_object) {
	  struct program *p = f->current_object->prog;
	  if (p) {
	    struct identifier *id = ID_FROM_INT(p, f->fun);
	    INT32 line;
	    struct pike_string *file;
	    if (IDENTIFIER_IS_PIKE_FUNCTION(id->identifier_flags) &&
		id->func.offset >= 0 &&
		(file = get_line(p->program + id->func.offset, p, &line))) {
	      fprintf(stderr, "%*s**Function %s at %s:%ld\n",
		      indent, "", id->name->str, file->str, (long) line);
	      free_string(file);
	    }
	    else
	      fprintf(stderr, "%*s**Function %s at unknown location.\n",
		      indent, "", id->name->str);
	  }
	  if (!(flags & DESCRIBE_SHORT)) {
	    fprintf(stderr, "%*s**Describing the current object:\n", indent, "");
	    describe_something(f->current_object, T_OBJECT, indent+2, depth, flags, 0);
	  }
	}
	else
	  fprintf(stderr, "%*s**No current object.\n", indent, "");
	if ((f = f->scope))
	  fprintf(stderr, "%*s**Moving on to outer scope frame %p:\n", indent, "", f);
      } while (f);
      break;
    }

    default:
      fprintf(stderr, "%*s**Cannot describe block of unknown type %d\n",
	      indent, "", t);
  }
}

void describe_something(void *a, int t, int indent, int depth, int flags,
			void *inblock)
{
  int tmp;
  struct program *p=(struct program *)a;
  if(!a) {
    fprintf (stderr, "%*s**NULL pointer\n", indent, "");
    return;
  }

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
    fprintf(stderr,"%*s**Block: %p  Type: %s  Zapped pointer\n",indent,"",a,
	    get_name_of_type(t));
  } else
#endif /* DEBUG_MALLOC */
    if (((ptrdiff_t)a) & 3) {
      fprintf(stderr,"%*s**Block: %p  Type: %s  Misaligned address\n",indent,"",a,
	      get_name_of_type(t));
    } else {
      fprintf(stderr,"%*s**Block: %p  Type: %s  Refs: %d\n",indent,"",a,
	      get_name_of_type(t),
	      *(INT32 *)a);

      low_describe_something(a,t,indent,depth,flags,inblock);

#ifdef DEBUG_MALLOC
      if(!(flags & DESCRIBE_NO_DMALLOC))
	debug_malloc_dump_references(a,indent+2,depth-1,flags);
#endif
    }

  fprintf(stderr,"%*s*******************\n",indent,"");
  d_flag=tmp;
}

PMOD_EXPORT void describe(void *x)
{
  void *inblock;
  int type = attempt_to_identify(x, &inblock);
  describe_something(x, type, 0, 0, 0, inblock);
}

void debug_describe_svalue(struct svalue *s)
{
  fprintf(stderr,"Svalue at %p is:\n",s);
  switch(s->type)
  {
    case T_INT:
      fprintf(stderr,"    %"PRINTPIKEINT"d (subtype %d)\n",s->u.integer,
	      s->subtype);
      break;

    case T_FLOAT:
      fprintf(stderr,"    %"PRINTPIKEFLOAT"f\n",s->u.float_number);
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
	    fprintf(stderr,"    Function in destructed object: %s\n",
		    ID_FROM_INT(p,s->subtype)->name->str);
	  }else{
	    fprintf(stderr,"    Function in destructed object.\n");
	  }
	}else{
	  fprintf(stderr,"    Function name: %s\n",
		  ID_FROM_INT(s->u.object->prog,s->subtype)->name->str);
	}
      }
  }
  describe_something(s->u.refs,s->type,0,1,0,0);
}

void gc_watch(void *a)
{
  struct marker *m;
  init_gc();
  m = get_marker(a);
  if (!(m->flags & GC_WATCHED)) {
    m->flags |= GC_WATCHED;
    fprintf(stderr, "## Watching thing %p.\n", a);
    gc_is_watching++;
  }
  else
    fprintf(stderr, "## Already watching thing %p.\n", a);
}

static void gc_watched_found (struct marker *m, const char *found_in)
{
  fprintf(stderr, "## Watched thing %p with %d refs found in "
	  "%s in pass %d.\n", m->data, *(INT32 *) m->data, found_in, Pike_in_gc);
  describe_marker (m);
}

#endif /* PIKE_DEBUG */

#ifndef GC_MARK_DEBUG
struct pike_queue gc_mark_queue;
#else  /* !GC_MARK_DEBUG */

/* Cut'n'paste from queue.c. */

struct gc_queue_entry
{
  queue_call call;
  void *data;
  int in_type;
  void *in;
  const char *place;
};

#define GC_QUEUE_ENTRIES 8191

struct gc_queue_block
{
  struct gc_queue_block *next;
  int used;
  struct gc_queue_entry entries[GC_QUEUE_ENTRIES];
};

struct gc_queue_block *gc_mark_first = NULL, *gc_mark_last = NULL;

void gc_mark_run_queue()
{
  struct gc_queue_block *b;

  while((b=gc_mark_first))
  {
    int e;
    for(e=0;e<b->used;e++)
    {
      debug_malloc_touch(b->entries[e].data);
      b->entries[e].call(b->entries[e].data);
    }

    gc_mark_first=b->next;
    free((char *)b);
  }
  gc_mark_last=0;
}

void gc_mark_discard_queue()
{
  struct gc_queue_block *b = gc_mark_first;
  while (b)
  {
    struct gc_queue_block *next = b->next;
    free((char *) b);
    b = next;
  }
  gc_mark_first = gc_mark_last = 0;
}

void gc_mark_enqueue (queue_call call, void *data)
{
  struct gc_queue_block *b;

#ifdef PIKE_DEBUG
  if (gc_found_in_type == PIKE_T_UNKNOWN || !gc_found_in)
    gc_fatal (data, 0, "gc_mark_enqueue() called outside GC_ENTER.\n");
  {
    struct marker *m;
    if (gc_is_watching && (m = find_marker(data)) && m->flags & GC_WATCHED) {
      /* This is useful to set breakpoints on. */
      gc_watched_found (m, "gc_mark_enqueue()");
    }
  }
#endif

  b=gc_mark_last;
  if(!b || b->used >= GC_QUEUE_ENTRIES)
  {
    b = (struct gc_queue_block *) malloc (sizeof (struct gc_queue_block));
    if (!b) fatal ("Out of memory in gc.\n");
    b->used=0;
    b->next=0;
    if(gc_mark_first)
      gc_mark_last->next=b;
    else
      gc_mark_first=b;
    gc_mark_last=b;
  }

  b->entries[b->used].call=call;
  b->entries[b->used].data=debug_malloc_pass(data);
  b->entries[b->used].in_type = gc_found_in_type;
  b->entries[b->used].in = debug_malloc_pass (gc_found_in);
  b->entries[b->used].place = gc_found_place;
  b->used++;
}

#endif	/* GC_MARK_DEBUG */

void debug_gc_touch(void *a)
{
  struct marker *m;

#ifdef PIKE_DEBUG
  if (gc_is_watching && (m = find_marker(a)) && m->flags & GC_WATCHED) {
    /* This is useful to set breakpoints on. */
    gc_watched_found (m, "gc_touch()");
  }
#endif

  if (!a) Pike_fatal("Got null pointer.\n");

  switch (Pike_in_gc) {
    case GC_PASS_PRETOUCH:
      m = find_marker(a);
      if (m && !(m->flags & (GC_PRETOUCHED
#ifdef PIKE_DEBUG
			     |GC_WATCHED
#endif
			    )))
	gc_fatal(a, 1, "Thing got an existing but untouched marker.\n");
      m = get_marker(a);
      m->flags |= GC_PRETOUCHED;
#ifdef PIKE_DEBUG
      m->saved_refs = *(INT32 *) a;
#endif
      break;

    case GC_PASS_MIDDLETOUCH: {
#ifdef PIKE_DEBUG
      int extra_ref;
#endif
      m = find_marker(a);
      if (!m)
	gc_fatal(a, 1, "Found a thing without marker.\n");
      else if (!(m->flags & GC_PRETOUCHED))
	gc_fatal(a, 1, "Thing got an existing but untouched marker.\n");
#ifdef PIKE_DEBUG
      extra_ref = (m->flags & GC_GOT_EXTRA_REF) == GC_GOT_EXTRA_REF;
      if (m->saved_refs + extra_ref < *(INT32 *) a)
	if (m->flags & GC_WEAK_FREED)
	  gc_fatal(a, 1, "Something failed to remove weak reference(s) to thing, "
		   "or it has gotten more references since gc start.\n");
	else
	  gc_fatal(a, 1, "Thing has gotten more references since gc start.\n");
      else
	if (m->weak_refs > m->saved_refs)
	  gc_fatal(a, 0, "A thing got more weak references than references.\n");
#endif
      m->flags |= GC_MIDDLETOUCHED;
      break;
    }

    case GC_PASS_POSTTOUCH:
      m = find_marker(a);
      if (!*(INT32 *) a)
	gc_fatal(a, 1, "Found a thing without refs.\n");
      if (m) {
	if (!(m->flags & (GC_PRETOUCHED|GC_MIDDLETOUCHED)))
	  gc_fatal(a, 2, "An existing but untouched marker found "
		   "for object in linked lists.\n");
	else if (m->flags & GC_LIVE_RECURSE ||
		 (m->frame && m->frame->frameflags & (GC_WEAK_REF|GC_STRONG_REF)))
	  gc_fatal(a, 2, "Thing still got flag from recurse list.\n");
#ifdef PIKE_DEBUG
	else if (m->flags & GC_MARKED)
	  return;
	else if (gc_destruct_everything)
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
	else if (m->weak_refs > m->saved_refs)
	  gc_fatal(a, 2, "A thing got more weak references than references.\n");
	else if (!(m->flags & GC_LIVE)) {
	  if (m->weak_refs < 0)
	    gc_fatal(a, 3, "A thing which had only weak references is "
		     "still around after gc.\n");
	  else
	    gc_fatal(a, 3, "A thing to garb is still around.\n");
	}
#endif
      }
      break;

    default:
      Pike_fatal("debug_gc_touch() used in invalid gc pass.\n");
  }
}

#ifdef PIKE_DEBUG

static INLINE struct marker *gc_check_debug(void *a, int weak)
{
  struct marker *m;

  if (!a) Pike_fatal("Got null pointer.\n");
  if(Pike_in_gc == GC_PASS_LOCATE)
  {
    if(check_for == a)
    {
      gdb_gc_stop_here(a, weak);
    }
    return 0;
  }

#if 0
  fprintf (stderr, "Ref: %s %p -> %p%s\n",
	   get_name_of_type (gc_found_in_type), gc_found_in, a,
	   gc_found_place ? gc_found_place : "");
#endif

  if (Pike_in_gc != GC_PASS_CHECK)
    Pike_fatal("gc check attempted in invalid pass.\n");

  m = get_marker(a);

  if (!*(INT32 *)a)
    gc_fatal(a, 1, "GC check on thing without refs.\n");
  if (m->saved_refs == -1) m->saved_refs = *(INT32 *)a;
  else if (m->saved_refs != *(INT32 *)a)
    gc_fatal(a, 1, "Refs changed in gc check pass.\n");
  if (m->refs + m->xrefs >= *(INT32 *) a)
    /* m->refs will be incremented by the caller. */
    gc_fatal(a, 1, "Thing is getting more internal refs than refs "
	     "(a pointer has probably been checked more than once).\n");
  checked++;

  return m;
}

#endif /* PIKE_DEBUG */

PMOD_EXPORT INT32 real_gc_check(void *a)
{
  struct marker *m;
  INT32 ret;

#ifdef PIKE_DEBUG
  if (gc_found_in_type == PIKE_T_UNKNOWN || !gc_found_in)
    gc_fatal (a, 0, "gc_check() called outside GC_ENTER.\n");
  if (gc_is_watching && (m = find_marker(a)) && m->flags & GC_WATCHED) {
    /* This is useful to set breakpoints on. */
    gc_watched_found (m, "gc_check()");
  }
  if (!(m = gc_check_debug(a, 0))) return 0;
#else
  m = get_marker(a);
#endif

  ret=m->refs;
  add_ref(m);
  if (m->refs == *(INT32 *) a)
    m->flags |= GC_NOT_REFERENCED;
  return ret;
}

INT32 real_gc_check_weak(void *a)
{
  struct marker *m;
  INT32 ret;

#ifdef PIKE_DEBUG
  if (gc_found_in_type == PIKE_T_UNKNOWN || !gc_found_in)
    gc_fatal (a, 0, "gc_check_weak() called outside GC_ENTER.\n");
  if (gc_is_watching && (m = find_marker(a)) && m->flags & GC_WATCHED) {
    /* This is useful to set breakpoints on. */
    gc_watched_found (m, "gc_check_weak()");
  }
  if (!(m = gc_check_debug(a, 1))) return 0;
  if (m->weak_refs < 0)
    gc_fatal(a, 1, "Thing has already reached threshold for weak free.\n");
  if (m->weak_refs >= *(INT32 *) a)
    gc_fatal(a, 1, "Thing has gotten more weak refs than refs.\n");
  if (m->weak_refs > m->refs + 1)
    gc_fatal(a, 1, "Thing has gotten more weak refs than internal refs.\n");
#else
  m = get_marker(a);
#endif

  m->weak_refs++;
  gc_ext_weak_refs++;
  if (m->weak_refs == *(INT32 *) a)
    m->weak_refs = -1;

  ret=m->refs;
  add_ref(m);
  if (m->refs == *(INT32 *) a)
    m->flags |= GC_NOT_REFERENCED;
  return ret;
}

static void cleanup_markers (void)
{
#ifdef DO_PIKE_CLEANUP
  size_t e=0;
  struct marker *h;
  for(e=0;e<marker_hash_table_size;e++)
    while(marker_hash_table[e])
      remove_marker(marker_hash_table[e]->data);
#endif
  exit_marker_hash();
}


static void init_gc(void)
{
#ifdef PIKE_DEBUG
  if (!gc_is_watching) {
    /* The marker hash table is left around after a previous gc if
     * gc_keep_markers is set. */
    if (marker_hash_table) cleanup_markers();
#endif

    low_init_marker_hash(num_objects);
    get_marker(rec_list.data);	/* Used to simplify fencepost conditions. */
#ifdef PIKE_DEBUG
  }
#endif
}

static void exit_gc(void)
{
  if (gc_evaluator_callback) {
    remove_callback(gc_evaluator_callback);
    gc_evaluator_callback = NULL;
  }
  if (!gc_keep_markers)
    cleanup_markers();

  free_all_gc_frame_blocks();

#ifdef GC_VERBOSE
  num_gc_frames = 0;
#endif

#ifdef PIKE_DEBUG
  if (gc_is_watching) {
    fprintf(stderr, "## Exiting gc and resetting watches for %d things.\n",
	    gc_is_watching);
    gc_is_watching = 0;
  }
#endif
}

#ifdef PIKE_DEBUG
/* This function marks some known externals. The rest are handled by
 * callbacks added with add_gc_callback. */
static void mark_externals (void)
{
  struct mapping *constants;
  if (master_object)
    gc_mark_external (master_object, " as master_object");
  if ((constants = get_builtin_constants()))
    gc_mark_external (constants, " as global constants mapping");
}

void locate_references(void *a)
{
  int tmp, orig_in_gc = Pike_in_gc;
  const char *orig_gc_found_place = gc_found_place;
  int i=0;
  if(!marker_blocks)
  {
    i=1;
    init_gc();
  }
  Pike_in_gc = GC_PASS_LOCATE;
  gc_found_place = NULL;

  /* Disable debug, this may help reduce recursion bugs */
  tmp=d_flag;
  d_flag=0;

  fprintf(stderr,"**Looking for references to %p:\n", a);
  
  check_for=a;
  found_ref_count = 0;

  GC_ENTER (NULL, PIKE_T_UNKNOWN) {
    mark_externals();
    call_callback(& gc_callbacks, NULL);

    gc_check_all_arrays();
    gc_check_all_multisets();
    gc_check_all_mappings();
    gc_check_all_programs();
    gc_check_all_objects();
  } GC_LEAVE;

#ifdef DEBUG_MALLOC
  {
    extern void dmalloc_find_references_to(void *);
#if 0
    fprintf(stderr,"**DMALLOC Looking for references:\n");
    dmalloc_find_references_to(a);
#endif
  }
#endif

  fprintf(stderr,"**Done looking for references to %p, "
	  "found %"PRINTSIZET"d refs.\n", a, found_ref_count);

  Pike_in_gc = orig_in_gc;
  gc_found_place = orig_gc_found_place;
  if(i) exit_gc();
  d_flag=tmp;
}

void debug_gc_add_extra_ref(void *a)
{
  struct marker *m;

  if (gc_is_watching && (m = find_marker(a)) && m->flags & GC_WATCHED) {
    /* This is useful to set breakpoints on. */
    gc_watched_found (m, "gc_add_extra_ref()");
  }

  if (gc_debug) {
    m = find_marker(a);
    if ((!m || !(m->flags & GC_PRETOUCHED)) &&
	!safe_debug_findstring((struct pike_string *) a))
      gc_fatal(a, 0, "Doing gc_add_extra_ref() on invalid object.\n");
    if (!m) m = get_marker(a);
  }
  else m = get_marker(a);

  if (m->flags & GC_GOT_EXTRA_REF)
    gc_fatal(a, 0, "Thing already got an extra gc ref.\n");
  m->flags |= GC_GOT_EXTRA_REF;
  gc_extra_refs++;
  add_ref( (struct ref_dummy *)a);
}

void debug_gc_free_extra_ref(void *a)
{
  struct marker *m;

  if (gc_is_watching && (m = find_marker(a)) && m->flags & GC_WATCHED) {
    /* This is useful to set breakpoints on. */
    gc_watched_found (m, "gc_free_extra_ref()");
  }

  if (gc_debug) {
    m = find_marker(a);
    if ((!m || !(m->flags & GC_PRETOUCHED)) &&
	!safe_debug_findstring((struct pike_string *) a))
      gc_fatal(a, 0, "Doing gc_add_extra_ref() on invalid object.\n");
    if (!m) m = get_marker(a);
  }
  else m = get_marker(a);

  if (!(m->flags & GC_GOT_EXTRA_REF))
    gc_fatal(a, 0, "Thing haven't got an extra gc ref.\n");
  m->flags &= ~GC_GOT_EXTRA_REF;
  gc_extra_refs--;
}


int debug_gc_is_referenced(void *a)
{
  struct marker *m;

  if (gc_is_watching && (m = find_marker(a)) && m->flags & GC_WATCHED) {
    /* This is useful to set breakpoints on. */
    gc_watched_found (m, "gc_is_referenced()");
  }

  if (!a) Pike_fatal("Got null pointer.\n");
  if (Pike_in_gc != GC_PASS_MARK)
    Pike_fatal("gc_is_referenced() called in invalid gc pass.\n");

  if (gc_debug) {
    m = find_marker(a);
    if ((!m || !(m->flags & GC_PRETOUCHED)) &&
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

int gc_mark_external (void *a, const char *place)
{
  struct marker *m;

  if (gc_is_watching && (m = find_marker(a)) && m->flags & GC_WATCHED) {
    /* This is useful to set breakpoints on. */
    gc_watched_found (m, "gc_mark_external()");
  }

  if (!a) Pike_fatal("Got null pointer.\n");

  if(Pike_in_gc == GC_PASS_LOCATE)
  {
    if(a==check_for) {
      const char *orig_gc_found_place = gc_found_place;
      gc_found_place = place;
      gdb_gc_stop_here(a, 0);
      gc_found_place = orig_gc_found_place;
    }
    return 0;
  }

  if (Pike_in_gc != GC_PASS_CHECK)
    Pike_fatal("gc_mark_external() called in invalid gc pass.\n");

#ifdef DEBUG_MALLOC
  if (gc_external_refs_zapped) {
    fprintf (stderr, "One external ref to %p found%s.\n",
	     a, place ? place : "");
    if (gc_found_in) describe (gc_found_in);
    return 0;
  }
#endif

  m=get_marker(a);
  m->xrefs++;
  m->flags|=GC_XREFERENCED;
  if(Pike_in_gc == GC_PASS_CHECK &&
     (m->refs + m->xrefs > *(INT32 *)a ||
      (m->saved_refs != -1 && m->saved_refs != *(INT32 *)a)))
    gc_fatal(a, 1, "Ref counts are wrong.\n");
  return 0;
}

void debug_really_free_gc_frame(struct gc_frame *l)
{
  if (l->frameflags & GC_LINK_FREED)
    gc_fatal(l->data, 0, "Freeing freed gc_frame.\n");
  l->frameflags |= GC_LINK_FREED;
  l->back = PREV(l) = NEXT(l) = (struct gc_frame *)(ptrdiff_t) -1;
  really_free_gc_frame(l);
#ifdef GC_VERBOSE
  num_gc_frames--;
#endif
}

#else  /* PIKE_DEBUG */

#define debug_really_free_gc_frame(l) really_free_gc_frame(l)

#endif /* PIKE_DEBUG */

int gc_do_weak_free(void *a)
{
  struct marker *m;

#ifdef PIKE_DEBUG
  if (gc_is_watching && (m = find_marker(a)) && m->flags & GC_WATCHED) {
    /* This is useful to set breakpoints on. */
    gc_watched_found (m, "gc_do_weak_free()");
  }
  if (!a) Pike_fatal("Got null pointer.\n");
  if (Pike_in_gc != GC_PASS_MARK && Pike_in_gc != GC_PASS_ZAP_WEAK)
    Pike_fatal("gc_do_weak_free() called in invalid gc pass.\n");
  if (gc_debug) {
    if (!(m = find_marker(a)))
      gc_fatal(a, 0, "gc_do_weak_free() got unknown object.\n");
  }
  else m = get_marker(a);
  debug_malloc_touch(a);

  if (m->weak_refs > m->refs)
    gc_fatal(a, 0, "More weak references than internal references.\n");
#else
  m = get_marker(a);
#endif

  if (Pike_in_gc != GC_PASS_ZAP_WEAK) {
    if (m->weak_refs < 0)
      goto should_free;
  }
  else
    if (!(m->flags & GC_MARKED)) {
#ifdef PIKE_DEBUG
      if (m->weak_refs <= 0)
	gc_fatal(a, 0, "Too many weak refs cleared to thing with external "
		 "weak refs.\n");
#endif
      m->weak_refs--;
      goto should_free;
    }
  return 0;

should_free:
  gc_ext_weak_refs--;
#ifdef PIKE_DEBUG
  m->saved_refs--;
  m->flags |= GC_WEAK_FREED;
#endif

  if (*(INT32 *) a == 1) {
    /* Make sure the thing doesn't run out of refs, since we can't
     * handle cascading frees now. We'll do it in the free pass
     * instead. */
    gc_add_extra_ref(a);
    m->flags |= GC_GOT_DEAD_REF;
#ifdef PIKE_DEBUG
    delayed_freed++;
#endif
  }

  return 1;
}

void gc_delayed_free(void *a, int type)
{
  struct marker *m;

#ifdef PIKE_DEBUG
  if (gc_is_watching && (m = find_marker(a)) && m->flags & GC_WATCHED) {
    /* This is useful to set breakpoints on. */
    gc_watched_found (m, "gc_delayed_free()");
  }
  if (Pike_in_gc != GC_PASS_MARK && Pike_in_gc != GC_PASS_CYCLE &&
      Pike_in_gc != GC_PASS_ZAP_WEAK)
    Pike_fatal("gc_delayed_free() called in invalid gc pass.\n");
  if (gc_debug) {
    if (!(m = find_marker(a)))
      gc_fatal(a, 0, "gc_delayed_free() got unknown object (missed by pretouch pass).\n");
  }
  else m = get_marker(a);
  if (*(INT32 *) a != 1)
    Pike_fatal("gc_delayed_free() called for thing that haven't got a single ref.\n");
  debug_malloc_touch(a);
  delayed_freed++;
#else
  m = get_marker(a);
#endif

  if (m->flags & GC_MARKED) {
    /* Note that we can get marked things here, e.g. if the index in a
     * mapping with weak indices is removed in the zap weak pass, the
     * value will be zapped too, but it will still have a mark from
     * the mark pass. This means that the stuff referenced by the
     * value will only be refcount garbed, which can leave cyclic
     * garbage for the next gc round.
     *
     * Since the value has been marked we won't find it in the free
     * pass, so we have to keep special track of it. :P */
    struct gc_frame *l = alloc_gc_frame();
    l->data = a;
    l->u.free_extra_type = type;
    l->back = free_extra_list;
    l->frameflags = 0;
    free_extra_list = l;
  }

  gc_add_extra_ref(a);
  m->flags |= GC_GOT_DEAD_REF;
}

int gc_mark(void *a)
{
  struct marker *m = get_marker(debug_malloc_pass(a));

#ifdef PIKE_DEBUG
  if (gc_is_watching && m && m->flags & GC_WATCHED) {
    /* This is useful to set breakpoints on. */
    gc_watched_found (m, "gc_mark()");
  }
  if (!a) Pike_fatal("Got null pointer.\n");
  if (Pike_in_gc != GC_PASS_MARK && Pike_in_gc != GC_PASS_ZAP_WEAK)
    Pike_fatal("gc mark attempted in invalid pass.\n");
  if (!*(INT32 *) a)
    gc_fatal(a, 0, "Marked a thing without refs.\n");
  if (m->weak_refs < 0)
    gc_fatal(a, 0, "Marking thing scheduled for weak free.\n");
#endif

  if (Pike_in_gc == GC_PASS_ZAP_WEAK) {
    /* Things are visited in the zap weak pass through the mark
     * functions to free refs to internal things that only got weak
     * external references. That happens only when a thing also have
     * internal cyclic non-weak refs. */
#ifdef PIKE_DEBUG
    if (!(m->flags & GC_MARKED))
      gc_fatal(a, 0, "gc_mark() called for thing in zap weak pass "
	       "that wasn't marked before.\n");
#endif
    if (m->flags & GC_FREE_VISITED)
      return 0;
    else {
      m->flags |= GC_FREE_VISITED;
      return 1;
    }
  }

  else if (m->flags & GC_MARKED) {
#ifdef PIKE_DEBUG
    if (m->weak_refs != 0)
      gc_fatal(a, 0, "weak_refs changed in marker already visited by gc_mark().\n");
#endif
    return 0;
  }
  else {
    if (m->weak_refs) {
      gc_ext_weak_refs -= m->weak_refs;
      m->weak_refs = 0;
    }
    m->flags = (m->flags & ~GC_NOT_REFERENCED) | GC_MARKED;
    DO_IF_DEBUG(marked++);
    return 1;
  }
}

PMOD_EXPORT void gc_cycle_enqueue(gc_cycle_check_cb *checkfn, void *data, int weak)
{
  struct gc_frame *l = alloc_gc_frame();
#ifdef PIKE_DEBUG
  {
    struct marker *m;
    if (gc_is_watching && (m = find_marker(data)) && m->flags & GC_WATCHED) {
      /* This is useful to set breakpoints on. */
      gc_watched_found (m, "gc_cycle_enqueue()");
    }
  }
  if (Pike_in_gc != GC_PASS_CYCLE)
    gc_fatal(data, 0, "Use of the gc frame stack outside the cycle check pass.\n");
#endif
#ifdef GC_VERBOSE
  if (++num_gc_frames > max_gc_frames) max_gc_frames = num_gc_frames;
#endif
  l->data = data;
  l->u.link.checkfn = checkfn;
  l->u.link.weak = weak;
  l->frameflags = 0;
  l->back = gc_rec_top;
#ifdef GC_STACK_DEBUG
  fprintf(stderr, "enqueue %p [%p]: ", l, gc_rec_top);
  describe_gc_frame(l);
  fputc('\n', stderr);
#endif
  gc_rec_top = l;
}

static struct gc_frame *gc_cycle_enqueue_pop(void *data)
{
  struct gc_frame *l = alloc_gc_frame();
#ifdef PIKE_DEBUG
  if (Pike_in_gc != GC_PASS_CYCLE)
    gc_fatal(data, 0, "Use of the gc frame stack outside the cycle check pass.\n");
#endif
#ifdef GC_VERBOSE
  if (++num_gc_frames > max_gc_frames) max_gc_frames = num_gc_frames;
#endif
  l->data = data;
  PREV(l) = gc_rec_last;
  NEXT(l) = 0;
  CYCLE(l) = 0;
  l->frameflags = GC_POP_FRAME;
  l->back = gc_rec_top;
#ifdef GC_STACK_DEBUG
  fprintf(stderr, "enqueue %p [%p]: ", l, gc_rec_top);
  describe_gc_frame(l);
  fputc('\n', stderr);
#endif
  gc_rec_top = l;
  return l;
}

void gc_cycle_run_queue()
{
#ifdef PIKE_DEBUG
  if (Pike_in_gc != GC_PASS_CYCLE)
    Pike_fatal("Use of the gc frame stack outside the cycle check pass.\n");
#endif
  while (gc_rec_top) {
#ifdef GC_STACK_DEBUG
    fprintf(stderr, "dequeue %p [%p]: ", gc_rec_top, gc_rec_top->back);
    describe_gc_frame(gc_rec_top);
    fputc('\n', stderr);
#endif
    if (gc_rec_top->frameflags & GC_POP_FRAME) {
      struct gc_frame *l = gc_rec_top->back;
      gc_cycle_pop(gc_rec_top->data);
      gc_rec_top = l;
    } else {
      struct gc_frame l = *gc_rec_top;
#ifdef PIKE_DEBUG
      if (l.frameflags & GC_LINK_FREED)
	gc_fatal(l.data, 0, "Accessing freed gc_frame.\n");
#endif
      debug_really_free_gc_frame(gc_rec_top);
      gc_rec_top = l.back;
      l.u.link.checkfn(l.data, l.u.link.weak);
    }
  }
}

#ifdef GC_CYCLE_DEBUG
static int gc_cycle_indent = 0;
#define CYCLE_DEBUG_MSG(M, TXT) do {					\
  fprintf(stderr, "%*s%-35s %p [%p] ", gc_cycle_indent, "",		\
	  (TXT), (M) ? (M)->data : 0, gc_rec_last->data);		\
  describe_marker(M);							\
} while (0)
#else
#define CYCLE_DEBUG_MSG(M, TXT) do {} while (0)
#endif

static void rotate_rec_list (struct gc_frame *beg, struct gc_frame *pos)
/* Rotates the marker list and the cycle stack so the bit from pos
 * down to the end gets before the bit from beg down to pos. The beg
 * pos might be moved further down the stack to avoid mixing cycles or
 * breaking strong link sequences. */
{
#ifdef GC_STACK_DEBUG
  struct gc_frame *l;
#endif
  CYCLE_DEBUG_MSG(find_marker(beg->data), "> rotate_rec_list, asked to begin at");

#ifdef PIKE_DEBUG
  if (Pike_in_gc != GC_PASS_CYCLE)
    Pike_fatal("Use of the gc frame stack outside the cycle check pass.\n");
  CHECK_POP_FRAME(beg);
  CHECK_POP_FRAME(pos);
  if (beg == pos)
    gc_fatal(beg->data, 0, "Cycle already broken at requested position.\n");
  if (NEXT(gc_rec_last))
    gc_fatal(gc_rec_last->data, 0, "gc_rec_last not at end.\n");
#endif

#ifdef GC_STACK_DEBUG
  fprintf(stderr,"Stack before:\n");
  for (l = gc_rec_top; l; l = l->back) {
    fprintf(stderr, "  %p ", l);
    describe_gc_frame(l);
    fputc('\n', stderr);
  }
#endif

#if 0
  if (CYCLE(beg)) {
    for (l = beg; CYCLE(PREV(l)) == CYCLE(beg); l = PREV(l))
      CHECK_POP_FRAME(l);
    if (CYCLE(l) == CYCLE(pos)) {
      /* Breaking something previously marked as a cycle. Clear it
       * since we're no longer sure it's an unambiguous cycle. */
      unsigned cycle = CYCLE(l);
      for (; l && CYCLE(l) == cycle; l = NEXT(l)) {
	CHECK_POP_FRAME(l);
#ifdef GC_CYCLE_DEBUG
	if (CYCLE(l))
	  CYCLE_DEBUG_MSG(find_marker(l->data), "> rotate_rec_list, clear cycle");
#endif
	CYCLE(l) = 0;
      }
    }
    else beg = l;		/* Keep the cycle continuous. */
  }
#endif

  /* Always keep chains of strong refs continuous, or else we risk
   * breaking the order in a later rotation. */
  for (; beg->frameflags & GC_STRONG_REF; beg = PREV(beg)) {}

  CYCLE_DEBUG_MSG(find_marker(beg->data), "> rotate_rec_list, begin at");

  {
    struct gc_frame *b = beg, *p = pos, *old_rec_top;
    while (b->frameflags & GC_OFF_STACK) {
      if ((b = NEXT(b)) == pos) goto done;
      CHECK_POP_FRAME(b);
      DO_IF_DEBUG(frame_rot++);
    }
    while (p->frameflags & GC_OFF_STACK) {
      if (!(p = NEXT(p))) goto done;
      CHECK_POP_FRAME(p);
      DO_IF_DEBUG(frame_rot++);
    }
    old_rec_top = gc_rec_top;
    gc_rec_top = p->back;
    p->back = b->back;
    b->back = old_rec_top;
  }
done:
  DO_IF_DEBUG(frame_rot++);

  {
    struct gc_frame *new_rec_last = PREV(pos);
    NEXT(PREV(beg)) = pos;
    PREV(pos) = PREV(beg);
    NEXT(gc_rec_last) = beg;
    PREV(beg) = gc_rec_last;
    gc_rec_last = new_rec_last;
    NEXT(gc_rec_last) = 0;
  }

  if (beg->frameflags & GC_WEAK_REF) {
    beg->frameflags &= ~GC_WEAK_REF;
    pos->frameflags |= GC_WEAK_REF;
    CYCLE_DEBUG_MSG(get_marker(pos->data), "> rotate_rec_list, moved weak flag");
  }

#ifdef GC_STACK_DEBUG
  fprintf(stderr,"Stack after:\n");
  for (l = gc_rec_top; l; l = l->back) {
    fprintf(stderr, "  %p ", l);
    describe_gc_frame(l);
    fputc('\n', stderr);
  }
#endif
}

int gc_cycle_push(void *x, struct marker *m, int weak)
{
  struct marker *last = find_marker(gc_rec_last->data);

#ifdef PIKE_DEBUG
  if (gc_is_watching && m && m->flags & GC_WATCHED) {
    /* This is useful to set breakpoints on. */
    gc_watched_found (m, "gc_cycle_push()");
  }

  debug_malloc_touch(x);

  if (!x) Pike_fatal("Got null pointer.\n");
  if (m->data != x) Pike_fatal("Got wrong marker.\n");
  if (Pike_in_gc != GC_PASS_CYCLE)
    Pike_fatal("GC cycle push attempted in invalid pass.\n");
  if (gc_debug && !(m->flags & GC_PRETOUCHED))
    gc_fatal(x, 0, "gc_cycle_push() called for untouched thing.\n");
  if (!gc_destruct_everything) {
    if ((!(m->flags & GC_NOT_REFERENCED) || m->flags & GC_MARKED) &&
	*(INT32 *) x)
      gc_fatal(x, 1, "Got a referenced marker to gc_cycle_push.\n");
    if (m->flags & GC_XREFERENCED)
      gc_fatal(x, 1, "Doing cycle check in externally referenced thing "
	       "missed in mark pass.\n");
  }
  if (weak && gc_rec_last == &rec_list)
    gc_fatal(x, 1, "weak is %d when on top of stack.\n", weak);
  if (gc_debug > 1) {
    struct array *a;
    struct object *o;
    struct program *p;
    struct mapping *m;
    struct multiset *l;
    for(a = gc_internal_array; a; a = a->next)
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

  if (last->flags & GC_LIVE_RECURSE) {
#ifdef PIKE_DEBUG
    if (!(last->flags & GC_LIVE))
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
      /* We'll get here eventually in the normal recursion. Pop off
       * the remaining live recurse frames for the last thing. */
      CYCLE_DEBUG_MSG(m, "gc_cycle_push, no live recurse");
      last->flags &= ~GC_LIVE_RECURSE;
      while (1) {
	struct gc_frame *l = gc_rec_top;
#ifdef PIKE_DEBUG
	if (!gc_rec_top)
	  Pike_fatal("Expected a gc_cycle_pop entry in gc_rec_top.\n");
#endif
	gc_rec_top = l->back;
	if (l->frameflags & GC_POP_FRAME) {
	  gc_rec_last = PREV(l);
	  debug_really_free_gc_frame(l);
	  break;
	}
	debug_really_free_gc_frame(l);
      }
#ifdef GC_CYCLE_DEBUG
      gc_cycle_indent -= 2;
      CYCLE_DEBUG_MSG(m, "> gc_cycle_push, unwound live rec");
#endif
    }

    return 0;
  }

#ifdef PIKE_DEBUG
  if (weak < 0 && gc_rec_last->frameflags & GC_FOLLOWED_NONSTRONG)
    gc_fatal(x, 0, "Followed strong link too late.\n");
  if (weak >= 0) gc_rec_last->frameflags |= GC_FOLLOWED_NONSTRONG;
#endif

  if (m->frame && !(m->frame->frameflags & GC_ON_KILL_LIST)) {
    /* A cyclic reference is found. */
#ifdef PIKE_DEBUG
    if (gc_rec_last == &rec_list)
      gc_fatal(x, 0, "Cyclic ref involves dummy rec_list marker.\n");
    CHECK_POP_FRAME(gc_rec_last);
    CHECK_POP_FRAME(m->frame);
#endif

    if (m != last) {
      struct gc_frame *p, *weak_ref = 0, *nonstrong_ref = 0;
      if (!weak) {
	struct gc_frame *q;
	CYCLE_DEBUG_MSG(m, "gc_cycle_push, search normal");
	/* Find the last weakly linked thing and the one before the
	 * first strongly linked thing. */
	for (q = m->frame, p = NEXT(q);; q = p, p = NEXT(p)) {
	  CHECK_POP_FRAME(p);
	  if (p->frameflags & (GC_WEAK_REF|GC_STRONG_REF)) {
	    if (p->frameflags & GC_WEAK_REF) weak_ref = p;
	    else if (!nonstrong_ref) nonstrong_ref = q;
	  }
	  if (p == gc_rec_last) break;
	}
      }

      else if (weak < 0) {
	CYCLE_DEBUG_MSG(m, "gc_cycle_push, search strong");
	/* Find the last weakly linked thing and the last one which
	 * isn't strongly linked. */
	for (p = NEXT(m->frame);; p = NEXT(p)) {
	  CHECK_POP_FRAME(p);
	  if (p->frameflags & GC_WEAK_REF) weak_ref = p;
	  if (!(p->frameflags & GC_STRONG_REF)) nonstrong_ref = p;
	  if (p == gc_rec_last) break;
	}
#ifdef PIKE_DEBUG
	if (p == gc_rec_last && !nonstrong_ref) {
	  fprintf(stderr, "Only strong links in cycle:\n");
	  for (p = m->frame;; p = NEXT(p)) {
	    describe(p->data);
	    locate_references(p->data);
	    if (p == gc_rec_last) break;
	    fprintf(stderr, "========= next =========\n");
	  }
	  gc_fatal(0, 0, "Only strong links in cycle.\n");
	}
#endif
      }

      else {
	struct gc_frame *q;
	CYCLE_DEBUG_MSG(m, "gc_cycle_push, search weak");
	/* Find the thing before the first strongly linked one. */
	for (q = m->frame, p = NEXT(q);; q = p, p = NEXT(p)) {
	  CHECK_POP_FRAME(p);
	  if (!(p->frameflags & GC_WEAK_REF) && !nonstrong_ref)
	    nonstrong_ref = q;
	  if (p == gc_rec_last) break;
	}
      }

      if (weak_ref) {
	/* The backward link is normal or strong and there are one
	 * or more weak links in the cycle. Let's break it at the
	 * last one (to ensure that a sequence of several weak links
	 * are broken at the last one). */
	CYCLE_DEBUG_MSG(find_marker(weak_ref->data),
			"gc_cycle_push, weak break");
	rotate_rec_list(m->frame, weak_ref);
      }

      else if (weak < 0) {
	/* The backward link is strong. Must break the cycle at the
	 * last nonstrong link. */
	CYCLE_DEBUG_MSG(find_marker(nonstrong_ref->data),
			"gc_cycle_push, nonstrong break");
	rotate_rec_list(m->frame, nonstrong_ref);
	NEXT(nonstrong_ref)->frameflags =
	  (NEXT(nonstrong_ref)->frameflags & ~GC_WEAK_REF) | GC_STRONG_REF;
      }

      else if (nonstrong_ref) {
	/* Either a nonweak cycle with a strong link in it or a weak
	 * cycle with a nonweak link in it. Break before the first
	 * link that's stronger than the others. */
	if (nonstrong_ref != m->frame) {
	  CYCLE_DEBUG_MSG(find_marker(nonstrong_ref->data),
			  "gc_cycle_push, weaker break");
	  rotate_rec_list(m->frame, nonstrong_ref);
	}
      }

      else if (!weak) {
	/* A normal cycle which will be destructed in arbitrary
	 * order. For reasons having to do with strong links we
	 * can't mark weak cycles this way. */
	unsigned cycle = CYCLE(m->frame) ? CYCLE(m->frame) : ++last_cycle;
	if (cycle == CYCLE(gc_rec_last))
	  CYCLE_DEBUG_MSG(m, "gc_cycle_push, old cycle");
	else {
	  CYCLE_DEBUG_MSG(m, "gc_cycle_push, cycle");
	  for (p = m->frame;; p = NEXT(p)) {
	    CYCLE(p) = cycle;
	    CYCLE_DEBUG_MSG(find_marker(p->data), "> gc_cycle_push, mark cycle");
	    if (p == gc_rec_last) break;
	  }}}}}			/* Mmm.. lisp ;) */

  else
    if (!(m->flags & GC_CYCLE_CHECKED)) {
      struct gc_frame *l;
#ifdef PIKE_DEBUG
      cycle_checked++;
      if (m->frame)
	gc_fatal(x, 0, "Marker already got a frame.\n");
      if (NEXT(gc_rec_last))
	gc_fatal(gc_rec_last->data, 0, "Not at end of list.\n");
#endif

      NEXT(gc_rec_last) = m->frame = l = gc_cycle_enqueue_pop(x);
      m->flags |= GC_CYCLE_CHECKED | (last->flags & GC_LIVE);
      debug_malloc_touch(x);
      if (weak) {
	if (weak > 0) l->frameflags |= GC_WEAK_REF;
	else l->frameflags |= GC_STRONG_REF;
      }

#ifdef GC_CYCLE_DEBUG
      if (weak > 0) CYCLE_DEBUG_MSG(m, "gc_cycle_push, recurse weak");
      else if (weak < 0) CYCLE_DEBUG_MSG(m, "gc_cycle_push, recurse strong");
      else CYCLE_DEBUG_MSG(m, "gc_cycle_push, recurse");
      gc_cycle_indent += 2;
#endif
      gc_rec_last = l;
      return 1;
    }

  /* Should normally not recurse now, but got to do that anyway if we
   * must mark live things. */
  if (!(last->flags & GC_LIVE) || m->flags & GC_LIVE) {
    CYCLE_DEBUG_MSG(m, "gc_cycle_push, no recurse");
    return 0;
  }

live_recurse:
#ifdef PIKE_DEBUG
  if (m->flags & GC_LIVE)
    Pike_fatal("Shouldn't live recurse when there's nothing to do.\n");
#endif
  m->flags |= GC_LIVE|GC_LIVE_RECURSE;
  debug_malloc_touch(x);

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

  {
    /* Recurse without linking onto rec_list. */
    struct gc_frame *l = gc_cycle_enqueue_pop(x);
#ifdef GC_CYCLE_DEBUG
    CYCLE_DEBUG_MSG(m, "gc_cycle_push, live recurse");
    gc_cycle_indent += 2;
#endif
    gc_rec_last = l;
  }

#ifdef PIKE_DEBUG
  live_rec++;
#endif
  return 1;
}

static void gc_cycle_pop(void *a)
{
  struct marker *m = find_marker(a);
  struct gc_frame *here, *base, *p;

#ifdef PIKE_DEBUG
  if (gc_is_watching && m && m->flags & GC_WATCHED) {
    /* This is useful to set breakpoints on. */
    gc_watched_found (m, "gc_cycle_pop()");
  }
  if (!a) Pike_fatal("Got null pointer.\n");
  if (Pike_in_gc != GC_PASS_CYCLE)
    Pike_fatal("GC cycle pop attempted in invalid pass.\n");
  if (!(m->flags & GC_CYCLE_CHECKED))
    gc_fatal(a, 0, "Marker being popped doesn't have GC_CYCLE_CHECKED.\n");
  if (!gc_destruct_everything) {
    if ((!(m->flags & GC_NOT_REFERENCED) || m->flags & GC_MARKED) &&
	*(INT32 *) a)
      gc_fatal(a, 1, "Got a referenced marker to gc_cycle_pop.\n");
    if (m->flags & GC_XREFERENCED)
      gc_fatal(a, 1, "Doing cycle check in externally referenced thing "
	       "missed in mark pass.\n");
  }
#endif
#ifdef GC_CYCLE_DEBUG
  gc_cycle_indent -= 2;
#endif

  if (m->flags & GC_LIVE_RECURSE) {
    m->flags &= ~GC_LIVE_RECURSE;
    CYCLE_DEBUG_MSG(m, "gc_cycle_pop_live");
    gc_rec_last = PREV(gc_rec_top);
    debug_really_free_gc_frame(gc_rec_top);
    return;
  }

  here = m->frame;
#ifdef PIKE_DEBUG
  if (!here || here->data != a)
    gc_fatal(a, 0, "Marker being popped has no or invalid frame.\n");
  CHECK_POP_FRAME(here);
  CHECK_POP_FRAME(gc_rec_last);
  if (here->frameflags & GC_OFF_STACK)
    gc_fatal(a, 0, "Marker being popped isn't on stack.\n");
  here->back = (struct gc_frame *)(ptrdiff_t) -1;
#endif
  here->frameflags |= GC_OFF_STACK;

  for (base = PREV(here), p = here;; base = p, p = NEXT(p)) {
    if (base == here) {
      /* Part of a cycle; wait until the cycle is complete before
       * unlinking it from rec_list. */
      DO_IF_DEBUG(m->frame->back = (struct gc_frame *)(ptrdiff_t) -1);
      CYCLE_DEBUG_MSG(m, "gc_cycle_pop, keep cycle");
      return;
    }
    CHECK_POP_FRAME(p);
    if (!(CYCLE(p) && CYCLE(p) == CYCLE(base)))
      break;
  }

  gc_rec_last = base;
  while ((p = NEXT(base))) {
    struct marker *pm = find_marker(p->data);
#ifdef PIKE_DEBUG
    if (pm->frame != p)
      gc_fatal(p->data, 0, "Bogus marker for thing being popped.\n");
#endif
    p->frameflags &= ~(GC_WEAK_REF|GC_STRONG_REF);
    if (pm->flags & GC_LIVE_OBJ) {
      /* This extra ref is taken away in the kill pass. Don't add one
       * if it got an extra ref already due to weak free. */
      if (!(pm->flags & GC_GOT_DEAD_REF))
	gc_add_extra_ref(p->data);
      base = p;
      p->frameflags |= GC_ON_KILL_LIST;
      DO_IF_DEBUG(PREV(p) = (struct gc_frame *)(ptrdiff_t) -1);
      CYCLE_DEBUG_MSG(pm, "gc_cycle_pop, put on kill list");
    }
    else {
      if (!(pm->flags & GC_LIVE)) {
	/* Add an extra ref which is taken away in the free pass. This
	 * is done to not refcount garb the cycles themselves
	 * recursively, which in bad cases can consume a lot of C
	 * stack. */
	if (!(pm->flags & GC_GOT_DEAD_REF)) {
	  gc_add_extra_ref(pm->data);
	  pm->flags |= GC_GOT_DEAD_REF;
	}
      }
#ifdef PIKE_DEBUG
      else
	if (pm->flags & GC_GOT_DEAD_REF)
	  gc_fatal(p->data, 0, "Didn't expect a dead extra ref.\n");
#endif
      NEXT(base) = NEXT(p);
      CYCLE_DEBUG_MSG(pm, "gc_cycle_pop, pop off");
      pm->frame = 0;
      debug_really_free_gc_frame(p);
    }
  }

  if (base != gc_rec_last) {
    NEXT(base) = kill_list;
    kill_list = NEXT(gc_rec_last);
    NEXT(gc_rec_last) = 0;
  }
}

void do_gc_recurse_svalues(struct svalue *s, int num)
{
  gc_recurse_svalues(s, num);
}

void do_gc_recurse_short_svalue(union anything *u, int type)
{
  gc_recurse_short_svalue(u, type);
}

int gc_do_free(void *a)
{
  struct marker *m;
#ifdef PIKE_DEBUG
  if (gc_is_watching && (m = find_marker(a)) && m->flags & GC_WATCHED) {
    /* This is useful to set breakpoints on. */
    gc_watched_found (m, "gc_do_free()");
  }
  if (!a) Pike_fatal("Got null pointer.\n");
  if (Pike_in_gc != GC_PASS_FREE)
    Pike_fatal("gc free attempted in invalid pass.\n");
#endif

  m=find_marker(debug_malloc_pass(a));
  if (!m) return 0;		/* Object created after cycle pass. */

  if (gc_destruct_everything) {
    /* We don't actually free much in this mode, just destruct
     * objects. So when we normally would return nonzero we just
     * remove the extra ref again. */
    if (!(m->flags & GC_LIVE)) {
      if (*(INT32 *) a == 1)
	return 1;
      else {
	gc_free_extra_ref (a);
	--*(INT32 *) a;
      }
    }
    return 0;
  }

#ifdef PIKE_DEBUG
  if (*(INT32 *) a > !!(m->flags & GC_GOT_EXTRA_REF)) {
    if (!gc_destruct_everything &&
	(!(m->flags & GC_NOT_REFERENCED) || m->flags & GC_MARKED))
      gc_fatal(a, 0, "gc_do_free() called for referenced thing.\n");
    if (gc_debug &&
	(m->flags & (GC_PRETOUCHED|GC_MARKED|GC_IS_REFERENCED)) == GC_PRETOUCHED)
      gc_fatal(a, 0, "gc_do_free() called without prior call to "
	       "gc_mark() or gc_is_referenced().\n");
  }
  if(!gc_destruct_everything &&
     (m->flags & (GC_MARKED|GC_XREFERENCED)) == GC_XREFERENCED)
    gc_fatal(a, 1, "Thing with external reference missed in gc mark pass.\n");
  if ((m->flags & (GC_DO_FREE|GC_LIVE)) == GC_LIVE) live_ref++;
  m->flags |= GC_DO_FREE;
#endif

  return !(m->flags & GC_LIVE);
}

static void free_obj_arr(void *oa)
{
  struct array *obj_arr = *((struct array **)oa);

  if (obj_arr) free_array(obj_arr);
  free(oa);
}

/*! @class MasterObject
 */

/*! @decl void runtime_warning(string subsystem, string msg, mixed|void data)
 *!
 *!   Called by the Pike runtime to warn about data inconsistencies.
 *!
 *! @param subsystem
 *!   Runtime subsystem where the warning was generated.
 *!   Currently the following subsystems may call this function:
 *!   @string
 *!     @value "gc"
 *!       The garbage collector.
 *!   @endstring
 *!
 *! @param msg
 *!   Warning message.
 *!   Currently the following messages may be generated:
 *!   @string
 *!     @value "bad_cycle"
 *!       A cycle where the destruction order isn't deterministic
 *!       was detected by the garbage collector.
 *!
 *!       @[data] will in this case contain an array of the elements
 *!       in the cycle.
 *!   @endstring
 *!
 *! @param data
 *!   Optional data that further describes the warning specified by @[msg].
 */

/*! @endclass
 */

static void warn_bad_cycles()
{
  /* The reason for the extra level of indirection, is that it might
   * be clobbered by the longjump() in SET_ONERROR otherwise.
   * (On some architectures longjump() might restore obj_arr's original
   * value (eg if obj_arr is in a register)).
   */
  struct array **obj_arr_ = (struct array **)xalloc(sizeof(struct array *));
  ONERROR tmp;

  *obj_arr_ = NULL;

  SET_ONERROR(tmp, free_obj_arr, obj_arr_);

  {
    struct gc_frame *p;
    unsigned cycle = 0;
    *obj_arr_ = allocate_array(0);

    for (p = kill_list; p;) {
      if ((cycle = CYCLE(p))) {
	push_object((struct object *) p->data);
	dmalloc_touch_svalue(Pike_sp-1);
	*obj_arr_ = append_array(*obj_arr_, --Pike_sp);
      }
      p = NEXT(p);
      if (p ? ((unsigned)(CYCLE(p) != cycle)) : cycle) {
	if ((*obj_arr_)->size >= 2) {
	  push_constant_text("gc");
	  push_constant_text("bad_cycle");
	  push_array(*obj_arr_);
	  *obj_arr_ = 0;
	  SAFE_APPLY_MASTER("runtime_warning", 3);
	  pop_stack();
	  *obj_arr_ = allocate_array(0);
	}
	else *obj_arr_ = resize_array(*obj_arr_, 0);
      }
      if (!p) break;
    }
  }

  CALL_AND_UNSET_ONERROR(tmp);
}

size_t do_gc(void *ignored, int explicit_call)
{
  ALLOC_COUNT_TYPE start_allocs;
  size_t start_num_objs, unreferenced;
  cpu_time_t gc_start_time;
  ptrdiff_t objs, pre_kill_objs;
#if defined (PIKE_DEBUG) || defined (DO_PIKE_CLEANUP)
  unsigned destroy_count;
#endif
#ifdef PIKE_DEBUG
  unsigned obj_count;
  ONERROR uwp;
#endif

  if(Pike_in_gc) return 0;

  if (gc_enabled <= 0 && (gc_enabled < 0 || !explicit_call)) {
    num_allocs = 0;
    alloc_threshold = GC_MAX_ALLOC_THRESHOLD;
    if (gc_evaluator_callback) {
      remove_callback (gc_evaluator_callback);
      gc_evaluator_callback = NULL;
    }
    return 0;
  }

#ifdef DEBUG_MALLOC
  if(debug_options & GC_RESET_DMALLOC)
    reset_debug_malloc();
#endif
  init_gc();
  gc_generation++;
  Pike_in_gc=GC_PASS_PREPARE;
  gc_start_time = get_cpu_time();
  gc_debug = d_flag;
#ifdef PIKE_DEBUG
  SET_ONERROR(uwp, fatal_on_error, "Shouldn't get an exception inside the gc.\n");
  if (gc_is_watching)
    fprintf(stderr, "## Doing gc while watching for %d things.\n", gc_is_watching);
#endif

  destruct_objects_to_destruct();

  if(gc_evaluator_callback)
  {
    remove_callback(gc_evaluator_callback);
    gc_evaluator_callback=0;
  }

  objs=num_objects;
  last_cycle = 0;

  if(GC_VERBOSE_DO(1 ||) gc_trace) {
    if (gc_destruct_everything)
      fprintf (stderr, "Destructing all objects... ");
    else
      fprintf(stderr,"Garbage collecting... ");
    GC_VERBOSE_DO(fprintf(stderr, "\n"));
  }
#ifdef PIKE_DEBUG
  if(num_objects < 0)
    Pike_fatal("Panic, less than zero objects!\n");
#endif

  last_gc=TIME(0);
  start_num_objs = num_objects;
  start_allocs = num_allocs;
  num_allocs = 0;

  /* Object alloc/free and any reference changes are disallowed now. */

#ifdef PIKE_DEBUG
  delayed_freed = weak_freed = checked = marked = cycle_checked = live_ref = 0;
  live_rec = frame_rot = 0;
#endif
  if (gc_debug) {
    unsigned n;
    Pike_in_gc = GC_PASS_PRETOUCH;
    n = gc_touch_all_arrays();
    n += gc_touch_all_multisets();
    n += gc_touch_all_mappings();
    n += gc_touch_all_programs();
    n += gc_touch_all_objects();
#ifdef PIKE_DEBUG
    gc_touch_all_strings();
#endif
    if (n != (unsigned) num_objects)
      Pike_fatal("Object count wrong before gc; expected %d, got %d.\n", num_objects, n);
    GC_VERBOSE_DO(fprintf(stderr, "| pretouch: %u things\n", n));
  }

  /* First we count internal references */
  Pike_in_gc=GC_PASS_CHECK;
  gc_ext_weak_refs = 0;

#ifdef PIKE_DEBUG
  mark_externals();
#endif
  call_callback(& gc_callbacks, NULL);

  ACCEPT_UNFINISHED_TYPE_FIELDS {
    gc_check_all_arrays();
    gc_check_all_multisets();
    gc_check_all_mappings();
    gc_check_all_programs();
    gc_check_all_objects();
  } END_ACCEPT_UNFINISHED_TYPE_FIELDS;

  GC_VERBOSE_DO(fprintf(stderr, "| check: %u references in %d things, "
			"counted %"PRINTSIZET"u weak refs\n",
			checked, num_objects, gc_ext_weak_refs));

  Pike_in_gc=GC_PASS_MARK;

  /* Anything after and including gc_internal_* in the linked lists
   * are considered to lack external references. The mark pass move
   * externally referenced things in front of these pointers. */
  gc_internal_array = first_array;
  gc_internal_multiset = first_multiset;
  gc_internal_mapping = first_mapping;
  gc_internal_program = first_program;
  gc_internal_object = first_object;

  if (gc_destruct_everything) {
    GC_VERBOSE_DO(fprintf(stderr,
			  "| mark pass skipped - will destruct all objects\n"));
  }
  else {
    /* Next we mark anything with external references. Note that we can
     * follow the same reference several times, e.g. with shared mapping
     * data blocks. */
    ACCEPT_UNFINISHED_TYPE_FIELDS {
      gc_mark_all_arrays();
      gc_mark_run_queue();
      gc_mark_all_multisets();
      gc_mark_run_queue();
      gc_mark_all_mappings();
      gc_mark_run_queue();
      gc_mark_all_programs();
      gc_mark_run_queue();
      gc_mark_all_objects();
      gc_mark_run_queue();
#ifdef PIKE_DEBUG
      if(gc_debug) gc_mark_all_strings();
#endif /* PIKE_DEBUG */
    } END_ACCEPT_UNFINISHED_TYPE_FIELDS;

    GC_VERBOSE_DO(fprintf(stderr,
			  "| mark: %u markers referenced, %u weak references freed,\n"
			  "|       %d things to free, "
			  "got %"PRINTSIZET"u tricky weak refs\n",
			  marked, weak_freed, delayed_freed, gc_ext_weak_refs));
  }

  {
#ifdef PIKE_DEBUG
    size_t orig_ext_weak_refs = gc_ext_weak_refs;
    obj_count = delayed_freed;
    max_gc_frames = 0;
#endif
    Pike_in_gc=GC_PASS_CYCLE;

    /* Now find all cycles in the internal structures. Note that we can
     * follow the same reference several times, just like in the mark
     * pass. */
    /* Note: The order between types here is normally not significant,
     * but the permuting destruct order tests in the testsuite won't be
     * really effective unless objects are handled first. :P */
    gc_cycle_check_all_objects();
    gc_cycle_check_all_arrays();
    gc_cycle_check_all_multisets();
    gc_cycle_check_all_mappings();
    gc_cycle_check_all_programs();

#ifdef PIKE_DEBUG
    if (gc_rec_top)
      Pike_fatal("gc_rec_top not empty at end of cycle check pass.\n");
    if (NEXT(&rec_list) || gc_rec_last != &rec_list || gc_rec_top)
      Pike_fatal("Recurse list not empty or inconsistent after cycle check pass.\n");
    if (gc_ext_weak_refs != orig_ext_weak_refs)
      Pike_fatal("gc_ext_weak_refs changed from %"PRINTSIZET"u "
	    "to %"PRINTSIZET"u in cycle check pass.\n",
	    orig_ext_weak_refs, gc_ext_weak_refs);
#endif

    GC_VERBOSE_DO(fprintf(stderr,
			  "| cycle: %u internal things visited, %u cycle ids used,\n"
			  "|        %u weak references freed, %d more things to free,\n"
			  "|        %u live recursed frames, %u frame rotations,\n"
			  "|        space for %u gc frames used\n",
			  cycle_checked, last_cycle, weak_freed,
			  delayed_freed - obj_count,
			  live_rec, frame_rot, max_gc_frames));
  }

  if (gc_ext_weak_refs) {
    size_t to_free = gc_ext_weak_refs;
#ifdef PIKE_DEBUG
    obj_count = delayed_freed;
#endif
    Pike_in_gc = GC_PASS_ZAP_WEAK;
    /* Zap weak references from external to internal things. That
     * doesn't occur very often; only when something have both
     * external weak refs and nonweak cyclic refs from internal
     * things. */
    gc_zap_ext_weak_refs_in_mappings();
    gc_zap_ext_weak_refs_in_arrays();
    gc_zap_ext_weak_refs_in_multisets();
    gc_zap_ext_weak_refs_in_objects();
    gc_zap_ext_weak_refs_in_programs();
    GC_VERBOSE_DO(
      fprintf(stderr,
	      "| zap weak: freed %"PRINTPTRDIFFT"d external weak refs, "
	      "%"PRINTSIZET"u internal still around,\n"
	      "|           %d more things to free\n",
	      to_free - gc_ext_weak_refs, gc_ext_weak_refs,
	      delayed_freed - obj_count));
  }

  if (gc_debug) {
    unsigned n;
#ifdef DEBUG_MALLOC
    size_t i;
    struct marker *m;
#endif
    Pike_in_gc=GC_PASS_MIDDLETOUCH;
    n = gc_touch_all_arrays();
    n += gc_touch_all_multisets();
    n += gc_touch_all_mappings();
    n += gc_touch_all_programs();
    n += gc_touch_all_objects();
#ifdef PIKE_DEBUG
    gc_touch_all_strings();
#endif
    if (n != (unsigned) num_objects)
      Pike_fatal("Object count wrong in gc; expected %d, got %d.\n", num_objects, n);
    get_marker(rec_list.data)->flags |= GC_MIDDLETOUCHED;
#if 0 /* Temporarily disabled - Hubbe */
#ifdef PIKE_DEBUG
#ifdef DEBUG_MALLOC
    PTR_HASH_LOOP(marker, i, m)
      if (!(m->flags & (GC_MIDDLETOUCHED|GC_WEAK_FREED)) &&
	  dmalloc_is_invalid_memory_block(m->data)) {
	fprintf(stderr, "Found a stray marker after middletouch pass: ");
	describe_marker(m);
	fprintf(stderr, "Describing marker location(s):\n");
	debug_malloc_dump_references(m, 2, 1, 0);
	fprintf(stderr, "Describing thing for marker:\n");
	Pike_in_gc = 0;
	describe(m->data);
	Pike_in_gc = GC_PASS_MIDDLETOUCH;
	Pike_fatal("Fatal in garbage collector.\n");
      }
#endif
#endif
#endif
    GC_VERBOSE_DO(fprintf(stderr, "| middletouch\n"));
  }

  /* Object alloc/free and reference changes are allowed again now. */

  Pike_in_gc=GC_PASS_FREE;
#ifdef PIKE_DEBUG
  weak_freed = 0;
  obj_count = num_objects;
#endif

  /* Now we free the unused stuff. The extra refs to gc_internal_*
   * added above are removed just before the calls so we'll get the
   * correct relative positions in them. */
  unreferenced = 0;
  if (gc_internal_array)
    unreferenced += gc_free_all_unreferenced_arrays();
  if (gc_internal_multiset)
    unreferenced += gc_free_all_unreferenced_multisets();
  if (gc_internal_mapping)
    unreferenced += gc_free_all_unreferenced_mappings();
  if (gc_internal_object)
    unreferenced += gc_free_all_unreferenced_objects();
  /* Note: gc_free_all_unreferenced_objects needs to have the programs
   * around to handle the free (even when they aren't live). So it's
   * necessary to free the objects before the programs. */
  if (gc_internal_program)
    unreferenced += gc_free_all_unreferenced_programs();

  /* We might occasionally get things to gc_delayed_free that the free
   * calls above won't find. They're tracked in this list. */
  while (free_extra_list) {
    struct gc_frame *next = free_extra_list->back;
    union anything u;
    u.refs = (INT32 *) free_extra_list->data;
    gc_free_extra_ref(u.refs);
    free_short_svalue(&u, free_extra_list->u.free_extra_type);
    debug_really_free_gc_frame(free_extra_list);
    free_extra_list = next;
  }

  GC_VERBOSE_DO(fprintf(stderr, "| free: %d unreferenced, %d really freed, "
			"%u left with live references\n",
			unreferenced, obj_count - num_objects, live_ref));

#ifdef PIKE_DEBUG
  gc_internal_array = (struct array *) (ptrdiff_t) -1;
  gc_internal_multiset = (struct multiset *) (ptrdiff_t) -1;
  gc_internal_mapping = (struct mapping *) (ptrdiff_t) -1;
  gc_internal_program = (struct program *) (ptrdiff_t) -1;
  gc_internal_object = (struct object *) (ptrdiff_t) -1;

  if(fatal_after_gc) Pike_fatal("%s", fatal_after_gc);
#endif

  Pike_in_gc=GC_PASS_KILL;
  /* Destruct the live objects in cycles, but first warn about any bad
   * cycles. */
  pre_kill_objs = num_objects;
  if (last_cycle && Pike_interpreter.evaluator_stack &&
      !gc_destruct_everything) {
    objs -= num_objects;
    warn_bad_cycles();
    objs += num_objects;
  }
#if defined (PIKE_DEBUG) || defined (DO_PIKE_CLEANUP)
  destroy_count = 0;
#endif
  while (kill_list) {
    struct gc_frame *next = NEXT(kill_list);
    struct object *o = (struct object *) kill_list->data;
#ifdef PIKE_DEBUG
    if (!(kill_list->frameflags & GC_ON_KILL_LIST))
      gc_fatal(o, 0, "Kill list element hasn't got the proper flag.\n");
    if ((get_marker(kill_list->data)->flags & (GC_LIVE|GC_LIVE_OBJ)) !=
	(GC_LIVE|GC_LIVE_OBJ))
      gc_fatal(o, 0, "Invalid object on kill list.\n");
    if (o->prog && (o->prog->flags & PROGRAM_USES_PARENT) &&
	PARENT_INFO(o)->parent &&
	!PARENT_INFO(o)->parent->prog &&
	get_marker(PARENT_INFO(o)->parent)->flags & GC_LIVE_OBJ)
      gc_fatal(o, 0, "GC destructed parent prematurely.\n");
#endif
    GC_VERBOSE_DO(
      fprintf(stderr, "|   Killing %p with %d refs", o, o->refs);
      if (o->prog) {
	INT32 line;
	struct pike_string *file = get_program_line (o->prog, &line);
	fprintf(stderr, ", prog %s:%d\n", file->str, line);
	free_string(file);
      }
      else fputs(", is destructed\n", stderr);
    );
    destruct(o);
    free_object(o);
    gc_free_extra_ref(o);
#if defined (PIKE_DEBUG) || defined (DO_PIKE_CLEANUP)
    destroy_count++;
#endif
    debug_really_free_gc_frame(kill_list);
    kill_list = next;
  }

  GC_VERBOSE_DO(fprintf(stderr, "| kill: %u objects killed, %d things really freed\n",
			destroy_count, pre_kill_objs - num_objects));

  Pike_in_gc=GC_PASS_DESTRUCT;
  /* Destruct objects on the destruct queue. */
  GC_VERBOSE_DO(obj_count = num_objects);
  destruct_objects_to_destruct();
  GC_VERBOSE_DO(fprintf(stderr, "| destruct: %d things really freed\n",
			obj_count - num_objects));

  if (gc_debug) {
    unsigned n;
    Pike_in_gc=GC_PASS_POSTTOUCH;
    n = gc_touch_all_arrays();
    n += gc_touch_all_multisets();
    n += gc_touch_all_mappings();
    n += gc_touch_all_programs();
    n += gc_touch_all_objects();
    /* gc_touch_all_strings(); */
    if (n != (unsigned) num_objects)
      Pike_fatal("Object count wrong after gc; expected %d, got %d.\n", num_objects, n);
    GC_VERBOSE_DO(fprintf(stderr, "| posttouch: %u things\n", n));
  }
#ifdef PIKE_DEBUG
  if (gc_extra_refs) {
    size_t e;
    fprintf (stderr, "Lost track of %d extra refs to things in gc.\n"
	     "Searching for marker(s) with extra refs:\n", gc_extra_refs);
    for (e = 0; e < marker_hash_table_size; e++) {
      struct marker *s = marker_hash_table[e], *m;
      for (m = s; m;) {
	if (m->flags & GC_GOT_EXTRA_REF) {
	  fprintf (stderr, "========================================\n"
		   "Found marker with extra ref: ");
	  describe_marker (m);
	  fprintf (stderr, "Describing the thing pointed to:\n");
	  describe (m->data);
	}
	m = m->next;
	/* The marker might be moved to the head of the chain via
	 * describe() above, so do this to avoid infinite recursion.
	 * Some entries in the chain might be missed, but I don't want
	 * to bother. */
	if (m == s) break;
      }
    }
    fprintf (stderr, "========================================\n"
	     "Done searching for marker(s) with extra refs.\n");
    Pike_fatal("Lost track of %d extra refs to things in gc.\n", gc_extra_refs);
  }
  if(fatal_after_gc) Pike_fatal("%s", fatal_after_gc);
#endif

  Pike_in_gc=0;
  exit_gc();

  /* Calculate the next alloc_threshold. */
  {
    double multiplier, new_threshold;
    cpu_time_t last_non_gc_time, last_gc_time;

    /* If we're at an automatic and timely gc then start_allocs ==
     * alloc_threshold and we're using gc_average_slowness in the
     * decaying average calculation. Otherwise this is either an
     * explicit call (start_allocs < alloc_threshold) or the gc has
     * been delayed past its due time (start_allocs >
     * alloc_threshold), and in those cases we adjust the multiplier
     * to give the appropriate weight to this last instance. */
    multiplier=pow(gc_average_slowness,
		   (double) start_allocs / (double) alloc_threshold);

    /* Comparisons to avoid that overflows mess up the statistics. */
    if (gc_start_time > last_gc_end_time) {
      last_non_gc_time = gc_start_time - last_gc_end_time;
      non_gc_time = non_gc_time * multiplier +
	last_non_gc_time * (1.0 - multiplier);
    }
    else last_non_gc_time = (cpu_time_t) -1;
    last_gc_end_time = get_cpu_time();
    if (last_gc_end_time > gc_start_time) {
      last_gc_time = last_gc_end_time - gc_start_time;
      gc_time = gc_time * multiplier +
	last_gc_time * (1.0 - multiplier);
    }
    else last_gc_time = (cpu_time_t) -1;

    /* At this point, unreferenced contains the number of things that
     * were without external references during the check and mark
     * passes. In the process of freeing them, destroy functions might
     * have been called which means anything might have happened.
     * Therefore we use that figure instead of the difference between
     * the number of allocated things to measure the amount of
     * garbage. */
    last_garbage_ratio = (double) unreferenced / start_num_objs;

    objects_alloced = objects_alloced * multiplier +
      start_allocs * (1.0 - multiplier);
    objects_freed = objects_freed * multiplier +
      unreferenced * (1.0 - multiplier);

    if (last_non_gc_time == (cpu_time_t) -1 ||
	gc_time / non_gc_time <= gc_time_ratio) {
      /* Calculate the new threshold by adjusting the average
       * threshold (objects_alloced) with the ratio between the wanted
       * garbage at the next gc (gc_garbage_ratio_low *
       * start_num_objs) and the actual average garbage
       * (objects_freed). (Where the +1.0's come from I don't know.
       * Perhaps they're to avoid division by zero. /mast) */
      new_threshold = (objects_alloced+1.0) *
	(gc_garbage_ratio_low * start_num_objs) / (objects_freed+1.0);
      last_garbage_strategy = GARBAGE_RATIO_LOW;
    }
    else {
      new_threshold = (objects_alloced+1.0) *
	(gc_garbage_ratio_high * start_num_objs) / (objects_freed+1.0);
      last_garbage_strategy = GARBAGE_RATIO_HIGH;
    }

#if 0
    /* Afaics this is to limit the growth of the threshold to avoid
     * that a single sudden allocation spike causes a very long gc
     * interval the next time. Now when the bug in the decaying
     * average calculation is fixed there should be no risk for that,
     * at least not in any case when this would help. /mast */
    if(alloc_threshold + start_allocs < new_threshold)
      new_threshold = (double)(alloc_threshold + start_allocs);
#endif

    if(new_threshold < GC_MIN_ALLOC_THRESHOLD)
      alloc_threshold = GC_MIN_ALLOC_THRESHOLD;
    else if(new_threshold > GC_MAX_ALLOC_THRESHOLD)
      alloc_threshold = GC_MAX_ALLOC_THRESHOLD;
    else
      alloc_threshold = (ALLOC_COUNT_TYPE) new_threshold;

    if (!explicit_call && last_gc_time != (cpu_time_t) -1) {
#if CPU_TIME_IS_THREAD_LOCAL == PIKE_YES
      Pike_interpreter.thread_state->auto_gc_time += last_gc_time;
#elif CPU_TIME_IS_THREAD_LOCAL == PIKE_NO
      auto_gc_time += last_gc_time;
#endif
    }

    if(GC_VERBOSE_DO(1 ||) gc_trace)
    {
      char timestr[40];
      if (last_gc_time != (cpu_time_t) -1)
	sprintf (timestr, ", %ld ms",
		 (long) (last_gc_time / (CPU_TIME_TICKS / 1000)));
      else
	timestr[0] = 0;
#ifdef DO_PIKE_CLEANUP
      if (gc_destruct_everything)
	fprintf(stderr, "done (%"PRINTSIZET"d was destructed)%s\n",
		destroy_count, timestr);
      else
#endif
	fprintf(stderr, "done (%"PRINTSIZET"d of %"PRINTSIZET"d "
		"was unreferenced)%s\n",
		unreferenced, start_num_objs, timestr);
    }
  }

#ifdef PIKE_DEBUG
  UNSET_ONERROR (uwp);
  if (max_gc_frames > max_tot_gc_frames) max_tot_gc_frames = max_gc_frames;
  tot_cycle_checked += cycle_checked;
  tot_live_rec += live_rec, tot_frame_rot += frame_rot;
#endif

#ifdef ALWAYS_GC
  ADD_GC_CALLBACK();
#else
  if(d_flag > 3) ADD_GC_CALLBACK();
#endif

#ifdef DO_PIKE_CLEANUP
  if (gc_destruct_everything)
    return destroy_count;
#endif
  return unreferenced;
}

/*! @decl mapping(string:int|float) gc_status()
 *! @belongs Debug
 *!
 *! Get statistics from the garbage collector.
 *!
 *! @returns
 *!   A mapping with the following content will be returned:
 *!   @mapping
 *!     @member int "num_objects"
 *!       Number of arrays, mappings, multisets, objects and programs.
 *!     @member int "num_allocs"
 *!       Number of memory allocations since the last gc run.
 *!     @member int "alloc_threshold"
 *!       Threshold for "num_allocs" when another automatic gc run is
 *!       scheduled.
 *!     @member float "projected_garbage"
 *!       Estimation of the current amount of garbage.
 *!     @member int "objects_alloced"
 *!       Decaying average over the number of allocated objects
 *!       between gc runs.
 *!     @member int "objects_freed"
 *!       Decaying average over the number of freed objects in each gc
 *!       run.
 *!     @member float "last_garbage_ratio"
 *!       Garbage ratio in the last gc run.
 *!     @member int "non_gc_time"
 *!       Decaying average over the CPU milliseconds spent outside the
 *!       garbage collector.
 *!     @member int "gc_time"
 *!       Decaying average over the CPU milliseconds spent inside the
 *!       garbage collector.
 *!     @member string "last_garbage_strategy"
 *!       The garbage accumulation goal that the gc aimed for when
 *!       setting "alloc_threshold" in the last run. The value is
 *!       either "garbage_ratio_low" or "garbage_ratio_high", which
 *!       corresponds to the gc parameters with the same names in
 *!       @[Pike.gc_parameters].
 *!     @member int "last_gc"
 *!       Time when the garbage-collector last ran.
 *!   @endmapping
 *!
 *! @seealso
 *!   @[gc()], @[Pike.gc_parameters()]
 */
void f__gc_status(INT32 args)
{
  int size = 0;

  pop_n_elems(args);

  push_constant_text("num_objects");
  push_int(num_objects);
  size++;

  push_constant_text("num_allocs");
  push_int64(num_allocs);
  size++;

  push_constant_text("alloc_threshold");
  push_int64(alloc_threshold);
  size++;

  push_constant_text("projected_garbage");
  push_float(DO_NOT_WARN((FLOAT_TYPE)(objects_freed * (double) num_allocs /
				      (double) alloc_threshold)));
  size++;

  push_constant_text("objects_alloced");
  push_int64(DO_NOT_WARN((INT64)objects_alloced));
  size++;

  push_constant_text("objects_freed");
  push_int64(DO_NOT_WARN((INT64)objects_freed));
  size++;

  push_constant_text("last_garbage_ratio");
  push_float(DO_NOT_WARN((FLOAT_TYPE) last_garbage_ratio));
  size++;

  push_constant_text("non_gc_time");
  push_int64(DO_NOT_WARN((INT64) non_gc_time));
  size++;

  push_constant_text("gc_time");
  push_int64(DO_NOT_WARN((INT64) gc_time));
  size++;

  push_constant_text ("last_garbage_strategy");
  switch (last_garbage_strategy) {
    case GARBAGE_RATIO_LOW: push_constant_text ("garbage_ratio_low"); break;
    case GARBAGE_RATIO_HIGH: push_constant_text ("garbage_ratio_high"); break;
#ifdef PIKE_DEBUG
    default: Pike_fatal ("Unknown last_garbage_strategy %d\n", last_garbage_strategy);
#endif
  }
  size++;

  push_constant_text("last_gc");
  push_int64(last_gc);
  size++;

  f_aggregate_mapping(size * 2);
}

void dump_gc_info(void)
{
  fprintf(stderr,"Current number of things   : %d\n",num_objects);
  fprintf(stderr,"Allocations since last gc  : "PRINT_ALLOC_COUNT_TYPE"\n",
	  num_allocs);
  fprintf(stderr,"Threshold for next gc      : "PRINT_ALLOC_COUNT_TYPE"\n",
	  alloc_threshold);
  fprintf(stderr,"Projected current garbage  : %f\n",
	  objects_freed * (double) num_allocs / (double) alloc_threshold);

  fprintf(stderr,"Avg allocs between gc      : %f\n",objects_alloced);
  fprintf(stderr,"Avg frees per gc           : %f\n",objects_freed);
  fprintf(stderr,"Garbage ratio in last gc   : %f\n", last_garbage_ratio);
					     
  fprintf(stderr,"Avg cpu "CPU_TIME_UNIT" between gc      : %f\n", non_gc_time);
  fprintf(stderr,"Avg cpu "CPU_TIME_UNIT" in gc           : %f\n", gc_time);
  fprintf(stderr,"Avg time ratio in gc       : %f\n", gc_time / non_gc_time);

  fprintf(stderr,"Garbage strategy in last gc: %s\n",
	  last_garbage_strategy == GARBAGE_RATIO_LOW ? "garbage_ratio_low" :
	  last_garbage_strategy == GARBAGE_RATIO_HIGH ? "garbage_ratio_high" :
	  "???");

#ifdef PIKE_DEBUG
  fprintf(stderr,"Max used gc frames         : %u\n", max_tot_gc_frames);
  fprintf(stderr,"Live recursed ratio        : %g\n",
	  (double) tot_live_rec / tot_cycle_checked);
  fprintf(stderr,"Frame rotation ratio       : %g\n",
	  (double) tot_frame_rot / tot_cycle_checked);
#endif

  fprintf(stderr,"in_gc                      : %d\n", Pike_in_gc);
}

void cleanup_gc(void)
{
#ifdef PIKE_DEBUG
  if (gc_evaluator_callback) {
    remove_callback(gc_evaluator_callback);
    gc_evaluator_callback = NULL;
  }
#endif /* PIKE_DEBUG */
}
