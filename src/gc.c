/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: gc.c,v 1.288 2007/06/10 12:41:59 mast Exp $
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

int gc_enabled = 1;

/* These defaults are only guesses and hardly tested at all. Please improve. */
double gc_garbage_ratio_low = 0.2;
double gc_time_ratio = 0.05;
double gc_garbage_ratio_high = 0.5;

/* This slowness factor approximately corresponds to the average over
 * the last ten gc rounds. (0.9 == 1 - 1/10) */
double gc_average_slowness = 0.9;

/* The gc will free all things with no external nonweak references
 * that isn't referenced by live objects. An object is considered
 * "live" if it contains code that must be executed when it is
 * destructed; see gc_object_is_live for details. Live objects without
 * external references are then destructed and garbage collected with
 * normal refcount garbing (which might leave dead garbage around for
 * the next gc). These live objects are destructed in an order that
 * tries to be as well defined as possible using several rules:
 *
 * o  If an object A references B single way, then A is destructed
 *    before B.
 * o  If A and B are in a cycle, and there is a reference somewhere
 *    from B to A that is weaker than any reference from A to B, then
 *    the cycle is resolved by disregarding the weaker reference, and
 *    A is therefore destructed before B.
 * o  If a cycle is resolved through disregarding a weaker reference
 *    according to the preceding rule, and there is another cycle
 *    without weak references which also gets resolved through
 *    disregarding the same reference, then the other cycle won't be
 *    resolved by disregarding some other reference.
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

int num_objects = 2;		/* Account for *_empty_array. */
ALLOC_COUNT_TYPE num_allocs =0;
ALLOC_COUNT_TYPE alloc_threshold = GC_MIN_ALLOC_THRESHOLD;
PMOD_EXPORT int Pike_in_gc = 0;
int gc_generation = 0;
time_t last_gc;
int gc_trace = 0, gc_debug = 0;
#ifdef DO_PIKE_CLEANUP
int gc_destruct_everything = 0;
#endif
size_t gc_ext_weak_refs;

static double objects_alloced = 0.0;
static double objects_freed = 0.0;
static double gc_time = 0.0, non_gc_time = 0.0;
static cpu_time_t last_gc_end_time = 0;
#ifdef CPU_TIME_MIGHT_NOT_BE_THREAD_LOCAL
cpu_time_t auto_gc_time = 0;
#endif

struct link_frame		/* See cycle checking blurb below. */
{
  void *data;
  struct link_frame *prev;	/* Previous frame in the link stack. */
  gc_cycle_check_cb *checkfn;	/* Function to call to recurse the thing. */
  int weak;			/* Weak flag to checkfn. */
};

struct gc_rec_frame		/* See cycle checking blurb below. */
{
  void *data;
  int rf_flags;
  struct gc_rec_frame *prev;	/* The previous frame in the recursion
				 * stack. NULL for frames not in the stack. */
  struct gc_rec_frame *next;	/* The next frame in the recursion
				 * stack or the kill list. */
  struct gc_rec_frame *cycle_id;/* The cycle identifier frame.
				 * Initially points to self. */
  struct gc_rec_frame *cycle_piece;/* The start of the cycle piece list
				 * for frames on the recursion stack,
				 * or the next frame in the list for
				 * frames in cycle piece lists. */
  union {
    struct link_frame *link_top;/* The top of the link stack for
				 * frames on the recursion stack. */
    struct gc_rec_frame *last_cycle_piece;/* In the first frame on a
				 * cycle piece list, this is used to
				 * point to the last frame in the list. */
  } u;
};

/* rf_flags bits. */
#define GC_PREV_WEAK		0x01
#define GC_PREV_STRONG		0x02
#define GC_PREV_BROKEN		0x04
#define GC_MARK_LIVE		0x08
#define GC_ON_KILL_LIST		0x10
#ifdef PIKE_DEBUG
#define GC_ON_CYCLE_PIECE_LIST	0x20
#define GC_FRAME_FREED		0x40
#define GC_FOLLOWED_NONSTRONG	0x80
#endif

static struct gc_rec_frame sentinel_frame = {
  (void *) (ptrdiff_t) -1,
  0,
  (struct gc_rec_frame *) (ptrdiff_t) -1,
  (struct gc_rec_frame *) (ptrdiff_t) -1,
  &sentinel_frame,		/* Recognize as cycle id frame. */
  (struct gc_rec_frame *) (ptrdiff_t) -1,
  {(struct link_frame *) (ptrdiff_t) -1}
};
static struct gc_rec_frame *stack_top = &sentinel_frame;
static struct gc_rec_frame *kill_list = &sentinel_frame;

/* Cycle checking
 *
 * When a thing is recursed into, a gc_rec_frame is pushed onto the
 * recursion stack whose top pointer is stack_top. After that the
 * links emanating from that thing are collected through the
 * gc_cycle_check_* function and pushed as link_frames onto a link
 * stack that is specific to the rec frame. gc_rec_frame.u.link_top is
 * the top pointer of that stack. The link frames are then popped off
 * again one by one. If the thing that the link points to hasn't been
 * visited already then it's recursed, which means that the link frame
 * is popped off the link stack and a new rec frame is pushed onto the
 * main stack instead.
 *
 * When a reference is followed to a thing which has a rec frame
 * (either on the stack or on a cycle piece list - see below), we have
 * a cycle. However, if that reference is weak (or becomes weak after
 * rotation - see below), it's still not regarded as a cycle since
 * weak refs always are eligible to be broken to resolve cycles.
 *
 * A cycle is marked by setting gc_rec_frame.cycle_id in the rec
 * frames on the stack that are part of the cycle to the first
 * (deepest) one of those frames. That frame is called the "cycle
 * identifier frame" since all frames in the same cycle will end up
 * there if the cycle pointers are followed transitively. The cycle_id
 * pointer in the cycle identifier frame points to itself. Every frame
 * is initially treated as a cycle containing only itself.
 *
 * When the recursion leaves a thing, the rec frame is popped off the
 * stack. If the frame is part of a cycle that isn't finished at that
 * point, it's not freed but instead linked onto the cycle piece list
 * in gc_rec_frame.cycle_piece of the parent rec frame (which
 * necessarily is part of the same cycle). That is done to detect
 * cyclic refs that end up at the popped frame later on.
 *
 * The cycle_id pointers for frames on cycle piece lists point back
 * towards the rec frame that still is on the stack, but not past it
 * to the cycle id frame (which might be further back in the stack).
 * Whenever cycle_id pointer chains are traversed to find the root of
 * a cycle piece list, they are compacted to avoid O(n) complexity.
 *
 * The current tentative destruct order is described by the order on
 * the stack and the attached cycle piece lists: The thing that's
 * deepest in the stack is destructed first and the cycle piece list
 * has precedence over the next frame on the recursion stack. To
 * illustrate:
 *                                   ,- stack_top
 *                                  v
 *           t1 <=> t4 <=> ... <=> t7
 *            |      |              `-> t8 -> ... -> t9
 *            |      `-> t5 -> ... -> t6
 *            `-> t2 -> ... -> t3
 *
 * Here <=> represents links on the recursion stack and -> links in
 * the cycle piece lists. The tentative destruct order for these
 * things is the same as the numbering above.
 *
 * Since we strive to keep the refs intact during destruction, the
 * above means that the refs which have priority to be kept intact
 * should point towards the top of the stack and towards the end of
 * the cycle piece lists.
 *
 * To allow rotations, the recursion stack is a double linked list
 * using gc_rec_frame.prev and gc_rec_frame.next. Rotations are the
 * operation used to manipulate the order to avoid getting a
 * prioritized link pointing in the wrong direction:
 *                                                        ,- stack_top
 *                                   weak                v
 *  t1 <=> ... <=> t2 <=> ... <=> t3 <-> t4 <=> ... <=> t5
 *
 * If a non-weak backward pointer from t5 to t2 is encountered here,
 * we should prefer to break(*) the weak ref between t3 and t4. The
 * stack is therefore rotated to become:
 *                                                        ,- stack_top
 *            broken                                     v
 *  t1 <=> ... <#> t4 <=> ... <=> t5 <=> t2 <=> ... <=> t3
 *
 * The section to rotate always ends at the top of the stack.
 *
 * The strength of the refs along the stack links are represented as
 * follows:
 *
 * o  Things with a strong ref between them are kept next to each
 *    other, and the second (the one being referenced by the strong
 *    ref) has the GC_PREV_STRONG bit set. A rotation never breaks the
 *    list inside a sequence of strong refs.
 *
 * o  The GC_PREV_WEAK bit is set in the next frame for every link on
 *    the stack where no preceding frame reference any following frame
 *    with anything but weak refs.
 *
 * o  GC_PREV_BROKEN is set in frames that are rotated back, i.e. t4
 *    in the example above. This is used to break later cycles in the
 *    same position when they can't be broken at a weak link.
 *
 * Several separate cycles may be present on the stack simultaneously.
 * That happens when a subcycle which is referenced one way from an
 * earlier cycle is encountered. E.g.
 *
 *                          L--.        L--.
 *                        t1    t2 -> t3    t4
 *                          `--7        `--7
 *
 * where the visit order is t1, t2, t3 and then t4. Because of the
 * stack which causes a subcycle to always be added to the top, it can
 * be handled independently of the earlier cycles, and those can also
 * be extended later on when the subcycle has been popped off. If a
 * ref from the subcycle to an earlier cycle is found, that means that
 * both are really the same cycle, and the frames in the former
 * subcycle will instead become a cycle piece list on a frame in the
 * former preceding cycle.
 *
 * Since the link frames are kept in substacks attached to the rec
 * frames, they get rotated with the rec frames. This has the effect
 * that the links from the top rec frame on the stack always are
 * tested first. That is necessary to avoid clobbering weak ref
 * partitions. Example:
 *
 *                             weak          weak
 *                   t1 <=> t2 <-> t3 <=> t4 <-> t5
 *
 * A nonweak ref is found from t5 to t2. We get this after rotation:
 *
 *                     broken         weak
 *                   t1 <#> t5 <=> t2 <-> t3 <=> t4
 *
 * Now, if we would continue to follow the links from t5 and encounter
 * a new thing t7, we'd have to add it to the top. If that ref isn't
 * weak we'd have to blank out the weak flag which could be used in
 * other rotations above t5 (e.g. if a normal ref from t4 to t2 is
 * encountered). To avoid this we do the t4 links instead and continue
 * with t5 when t4, t3 and t2 are done.
 *
 * As said earlier, rec frames are moved to cycle piece lists when
 * they are popped off while being part of unfinished cycles. Since
 * there are no more outgoing refs at that point, there can be no more
 * rotations that affect the order between the rec frame and its
 * predecessor. Therefore the order on a cycle piece list is optimal
 * (in as far as the gc destruct order policy goes). Any further
 * rotations can move the predecessor around, but it can always be
 * treated as one unit together with its cycle piece list. Remaining
 * weak refs inside the cycle piece list are no longer relevant since
 * they don't apply to the links that remain to be followed for the
 * predecessor (i.e. the root of the cycle piece list, still on the
 * rec frame stack).
 *
 * If the preceding frame already has a cycle piece list when a rec
 * frame should be added to it, the rec frame (and its attached cycle
 * piece list) is linked in before that list. That since the rec frame
 * might have refs to the earlier cycle piece list, but the opposite
 * can't happen.
 *
 * When a cycle identifier frame is popped off the stack, the frame
 * together with its cycle piece list represent the complete cycle,
 * and the list holds an optimal order for destructing it. The frames
 * are freed at that point, except for the ones which correspond to
 * live objects, which instead are linked in order into the beginning
 * of the kill list. That list, whose beginning is pointed to by
 * kill_list, holds the final destruct order for all live objects.
 *
 * Note that the complete cycle has to be added to the kill list at
 * once since all live objects that are referenced single way from the
 * cycle should be destructed later and must therefore be put on the
 * kill list before the cycle.
 *
 * The cycle check functions might recurse another round through the
 * frames that have been recursed already, to propagate the GC_LIVE
 * flag to things that have been found to be referenced from live
 * objects. In this mode a single dummy rec frame with the
 * GC_MARK_LIVE bit is pushed on the recursion stack, and all link
 * frames are stacked in it, regardless of the things they originate
 * from.
 *
 * *)  Here "breaking" a ref doesn't mean that it actually gets
 *     zeroed out. It's only disregarded to resolve the cycle to
 *     produce an optimal destruct order. I.e. it will still be intact
 *     when the first object in the cycle is destructed, and it will
 *     only be zeroed when the thing it points to has been destructed.
 */

/* The free extra list. See note in gc_delayed_free. */
struct free_extra_frame
{
  void *data;
  struct free_extra_frame *next; /* Next pointer. */
  int type;			/* The type of the thing. */
};
static struct free_extra_frame *free_extra_list = NULL;

#ifdef PIKE_DEBUG
static unsigned delayed_freed, weak_freed, checked, marked, cycle_checked, live_ref;
static unsigned mark_live, frame_rot, link_search;
static unsigned gc_extra_refs = 0;
static unsigned tot_cycle_checked = 0, tot_mark_live = 0, tot_frame_rot = 0;
static unsigned gc_rec_frame_seq_max;
#endif

static unsigned rec_frames, link_frames, free_extra_frames;
static unsigned max_rec_frames, max_link_frames;
static unsigned tot_max_rec_frames = 0, tot_max_link_frames = 0, tot_max_free_extra_frames = 0;

#undef INIT_BLOCK
#define INIT_BLOCK(f) do {						\
    if (++rec_frames > max_rec_frames)					\
      max_rec_frames = rec_frames;					\
  } while (0)
#undef EXIT_BLOCK
#define EXIT_BLOCK(f) do {						\
    DO_IF_DEBUG ({							\
	if (f->rf_flags & GC_FRAME_FREED)				\
	  gc_fatal (f->data, 0, "Freeing gc_rec_frame twice.\n");	\
	f->rf_flags |= GC_FRAME_FREED;					\
	f->u.link_top = (struct link_frame *) (ptrdiff_t) -1;		\
	f->prev = f->next = f->cycle_id = f->cycle_piece =		\
	  (struct gc_rec_frame *) (ptrdiff_t) -1;			\
      });								\
    rec_frames--;							\
  } while (0)

BLOCK_ALLOC_FILL_PAGES (gc_rec_frame, 2)

/* Link and free_extra frames are approximately the same size, so let
 * them share block_alloc area. */
struct ba_mixed_frame
{
  union {
    struct link_frame link;
    struct free_extra_frame free_extra;
    struct ba_mixed_frame *next; /* For block_alloc internals. */
  } u;
};

#undef BLOCK_ALLOC_NEXT
#define BLOCK_ALLOC_NEXT u.next
#undef INIT_BLOCK
#define INIT_BLOCK(f)
#undef EXIT_BLOCK
#define EXIT_BLOCK(f)

BLOCK_ALLOC_FILL_PAGES (ba_mixed_frame, 2)

static INLINE struct link_frame *alloc_link_frame()
{
  struct ba_mixed_frame *f = alloc_ba_mixed_frame();
  if (++link_frames > max_link_frames)
    max_link_frames = link_frames;
  return (struct link_frame *) f;
}

static INLINE struct free_extra_frame *alloc_free_extra_frame()
{
  struct ba_mixed_frame *f = alloc_ba_mixed_frame();
  free_extra_frames++;
  return (struct free_extra_frame *) f;
}

static INLINE void really_free_link_frame (struct link_frame *f)
{
  link_frames--;
  really_free_ba_mixed_frame ((struct ba_mixed_frame *) f);
}

static INLINE void really_free_free_extra_frame (struct free_extra_frame *f)
{
  free_extra_frames--;
  really_free_ba_mixed_frame ((struct ba_mixed_frame *) f);
}

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
static void gc_cycle_pop();

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

#undef get_marker
#define get_marker debug_get_marker
#undef find_marker
#define find_marker debug_find_marker

PTR_HASH_ALLOC_FIXED_FILL_PAGES(marker,2)

#undef get_marker
#define get_marker(X) ((struct marker *) debug_malloc_pass(debug_get_marker(X)))
#undef find_marker
#define find_marker(X) ((struct marker *) debug_malloc_pass(debug_find_marker(X)))

PMOD_EXPORT struct marker *pmod_get_marker (void *p)
{
  return debug_get_marker (p);
}

PMOD_EXPORT struct marker *pmod_find_marker (void *p)
{
  return debug_find_marker (p);
}

#if defined (PIKE_DEBUG) || defined (GC_MARK_DEBUG)
void *gc_found_in = NULL;
int gc_found_in_type = PIKE_T_UNKNOWN;
const char *gc_found_place = NULL;
#endif

#ifdef DO_PIKE_CLEANUP
/* To keep the markers after the gc. Only used for the leak report at exit. */
int gc_keep_markers = 0;
PMOD_EXPORT int gc_external_refs_zapped = 0;
#endif

#if defined (PIKE_DEBUG) || defined (GC_CYCLE_DEBUG)
static void describe_rec_frame (struct gc_rec_frame *f)
{
  fprintf (stderr, "data=%p rf_flags=0x%02x prev=%p next=%p "
	   "cycle_id=%p cycle_piece=%p link_top/last_cycle_piece=%p",
	   f->data, f->rf_flags, f->prev, f->next,
	   f->cycle_id, f->cycle_piece, f->u.link_top);
}
#endif

#ifdef PIKE_DEBUG

int gc_in_cycle_check = 0;

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

  if (pike_type_hash)
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
#if 0
	fprintf(stderr,"%*s  **In p->constants[%"PRINTPTRDIFFT"d] (%s)\n",indent,"",
		e, p->constants[e].name ? p->constants[e].name->str : "no name");
#else /* !0 */
	fprintf(stderr,"%*s  **In p->constants[%"PRINTPTRDIFFT"d] "
		"(%"PRINTPTRDIFFT"d)\n",indent,"",
		e, p->constants[e].offset);
#endif /* 0 */
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

static void describe_link_frame (struct link_frame *f)
{
  fprintf (stderr, "data=%p prev=%p checkfn=%p weak=%d",
	   f->data, f->prev, f->checkfn, f->weak);
}

static void describe_marker(struct marker *m)
{
  if (m) {
    fprintf(stderr, "marker at %p: flags=0x%05lx refs=%d weak=%d "
	    "xrefs=%d saved=%d frame=%p",
	    m, (long) m->flags, m->refs, m->weak_refs,
	    m->xrefs, m->saved_refs, m->frame);
    if (m->frame) {
      fputs(" [", stderr);
      describe_rec_frame (m->frame);
      putc(']', stderr);
    }
    putc('\n', stderr);
  }
  else
    fprintf(stderr, "no marker\n");
}

#endif /* PIKE_DEBUG */

static void debug_gc_fatal_va (void *a, int flags,
			       const char *fmt, va_list args)
{
#ifdef PIKE_DEBUG
  struct marker *m;
#endif
  int orig_gc_pass = Pike_in_gc;

  fprintf(stderr, "**");
  (void) VFPRINTF(stderr, fmt, args);

#ifdef PIKE_DEBUG
  if (a) {
    /* Temporarily jumping out of gc to avoid being caught in debug
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

void debug_gc_fatal (void *a, int flags, const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  debug_gc_fatal_va (a, flags, fmt, args);
  va_end (args);
}

static void dloc_gc_fatal (const char *file, int line,
			   void *a, int flags, const char *fmt, ...)
{
  va_list args;
  fprintf (stderr, "%s:%d: GC fatal:\n", file, line);
  va_start (args, fmt);
  debug_gc_fatal_va (a, flags, fmt, args);
  va_end (args);
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

      if (((struct object *) a)->refs > 0 && p) {
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
	if ((INT32)(ptrdiff_t) p == 0x55555555)
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

      if (!(flags & DESCRIBE_SHORT) && p->refs > 0) {
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
	      indent, "", p->NAME, (size_t)p->PIKE_CONCAT(num_,NAME));
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
      if (((struct multiset *) a)->refs > 0)
	debug_dump_multiset((struct multiset *) a);
      break;

    case T_ARRAY:
      if (((struct array *) a)->refs > 0)
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
      if (((struct mapping *) a)->refs > 0)
	debug_dump_mapping((struct mapping *)a);
      break;

    case T_STRING:
    {
      struct pike_string *s=(struct pike_string *)a;
      fprintf(stderr,"%*s**size_shift: %d, "
	      "len: %"PRINTPTRDIFFT"d, "
	      "hash: %"PRINTSIZET"x\n",
	      indent,"", s->size_shift, s->len, s->hval);
      if (!s->size_shift && s->refs > 0) {
	if(s->len>77)
	{
	  fprintf(stderr,"%*s** \"%60s\"...\n",indent,"",s->str);
	}else{
	  fprintf(stderr,"%*s** \"%s\"\n",indent,"",s->str);
	}
      }
      break;
    }

    case PIKE_T_TYPE:
    {
      fprintf(stderr, "%*s**type: ", indent, "");
      simple_describe_type((struct pike_type *)a);
      fprintf(stderr, "\n");
      break;
    }

    case T_PIKE_FRAME: {
      struct pike_frame *f = (struct pike_frame *) a;
      do {
	if (f->refs <= 0) break;
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
  if (((INT32)(ptrdiff_t)a) == 0x55555555) {
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
      if (
#ifdef DO_PIKE_CLEANUP
	  !gc_keep_markers &&
#endif
	  m && !(m->flags & (GC_PRETOUCHED
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

    case GC_PASS_POSTTOUCH: {
#ifdef PIKE_DEBUG
      int extra_ref;
#endif
      m = find_marker(a);
      if (!m)
	gc_fatal(a, 1, "Found a thing without marker.\n");
      else if (!(m->flags & GC_PRETOUCHED))
	gc_fatal(a, 1, "Thing got an existing but untouched marker.\n");
      if (gc_destruct_everything && (m->flags & GC_MARKED))
	gc_fatal (a, 1, "Thing got marked in gc_destruct_everything mode.\n");
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
      m->flags |= GC_POSTTOUCHED;
      break;
    }

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

  if (gc_keep_markers) {
    /* Carry over any GC_CLEANUP_FREED flags but reinitialize them
     * otherwise. */
    for(e=0;e<marker_hash_table_size;e++) {
      struct marker *m;
      for (m = marker_hash_table[e]; m; m = m->next) {
#ifdef PIKE_DEBUG
	m->flags &= GC_CLEANUP_FREED;
	m->xrefs = 0;
	m->saved_refs = -1;
#else
	m->flags = 0;
#endif
	m->refs = m->weak_refs = 0;
	m->frame = 0;
      }
    }
    return;
  }

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
#endif
#if defined (PIKE_DEBUG) || defined (DO_PIKE_CLEANUP)
    /* The marker hash table is left around after a previous gc if
     * gc_keep_markers is set. */
    if (marker_hash_table) cleanup_markers();
    if (!marker_hash_table)
#endif
      low_init_marker_hash(num_objects);
#ifdef PIKE_DEBUG
  }
#endif
}

void exit_gc(void)
{
  if (gc_evaluator_callback) {
    remove_callback(gc_evaluator_callback);
    gc_evaluator_callback = NULL;
  }
  if (!gc_keep_markers)
    cleanup_markers();

  free_all_gc_rec_frame_blocks();
  free_all_ba_mixed_frame_blocks();

#ifdef PIKE_DEBUG
  if (gc_is_watching) {
    fprintf(stderr, "## Exiting gc and resetting watches for %d things.\n",
	    gc_is_watching);
    gc_is_watching = 0;
  }
#endif
}

#ifdef PIKE_DEBUG
PMOD_EXPORT void gc_check_zapped (void *a, TYPE_T type, const char *file, int line)
{
  struct marker *m = find_marker (a);
  if (m && (m->flags & GC_CLEANUP_FREED))
    fprintf (stderr, "Free of leaked %s %p from %s:%d, %d refs remaining\n",
	     get_name_of_type (type), a, file, line, *(INT32 *)a - 1);
}
#endif

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
#if defined (PIKE_DEBUG) || defined (DO_PIKE_CLEANUP)
    debug_gc_check_all_types();
#endif
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

#define LOW_CHECK_REC_FRAME(f, file, line) do {				\
    if (f->rf_flags & GC_FRAME_FREED)					\
      dloc_gc_fatal (file, line, f->data, 0,				\
		     "Accessing freed gc_stack_frame %p.\n", f);	\
    if (f->cycle_id->rf_flags & GC_FRAME_FREED) {			\
      fprintf (stderr, "Cycle id frame %p is freed. It is: ", f->cycle_id); \
      describe_rec_frame (f->cycle_id);					\
      fputc ('\n', stderr);						\
      dloc_gc_fatal (file, line, f->data, 0, "Cycle id frame is freed.\n"); \
    }									\
  } while (0)

static void check_rec_stack_frame (struct gc_rec_frame *f,
				   const char *file, int line)
{
  LOW_CHECK_REC_FRAME (f, file, line);
  if (f->rf_flags & (GC_ON_CYCLE_PIECE_LIST|GC_ON_KILL_LIST))
    dloc_gc_fatal (file, line, f->data, 0, "Frame is not on the rec stack.\n");
  if (!f->prev)
    dloc_gc_fatal (file, line, f->data, 0,
		   "Prev pointer not set for rec stack frame.\n");
  if (f->prev->next != f)
    dloc_gc_fatal (file, line, f->data, 0,
		   "Rec stack pointers are inconsistent.\n");
  if (f->cycle_id &&
      f->cycle_id->rf_flags & (GC_ON_CYCLE_PIECE_LIST|GC_ON_KILL_LIST)) {
    fprintf (stderr, "Cycle id frame %p not on the rec stack. It is: ",
	     f->cycle_id);
    describe_rec_frame (f->cycle_id);
    fputc ('\n', stderr);
    dloc_gc_fatal (file, line, f->data, 0,
		   "Cycle id frame not on the rec stack.\n");
  }
  if (f->cycle_piece &&
      (!f->cycle_piece->u.last_cycle_piece ||
       f->cycle_piece->u.last_cycle_piece->cycle_piece))
    dloc_gc_fatal (file, line, f->data, 0,
		   "Bogus last_cycle_piece %p is %p in %p.\n",
		   f->cycle_piece->u.last_cycle_piece,
		   f->cycle_piece->u.last_cycle_piece ?
		   f->cycle_piece->u.last_cycle_piece->cycle_piece : NULL,
		   f->cycle_piece);
  if ((f->rf_flags & GC_PREV_STRONG) &&
      (f->rf_flags & (GC_PREV_WEAK|GC_PREV_BROKEN)))
    dloc_gc_fatal (file, line, f->data, 0,
		   "GC_PREV_STRONG set together with "
		   "GC_PREV_WEAK or GC_PREV_BROKEN.\n");
}
#define CHECK_REC_STACK_FRAME(f)					\
  do check_rec_stack_frame ((f), __FILE__, __LINE__); while (0)

static void check_cycle_piece_frame (struct gc_rec_frame *f,
				     const char *file, int line)
{
  LOW_CHECK_REC_FRAME (f, file, line);
  if ((f->rf_flags & (GC_ON_CYCLE_PIECE_LIST|GC_ON_KILL_LIST)) !=
      GC_ON_CYCLE_PIECE_LIST)
    dloc_gc_fatal (file, line, f->data, 0,
		   "Frame is not on a cycle piece list.\n");
  if (f->prev)
    dloc_gc_fatal (file, line, f->data, 0,
		   "Prev pointer set for frame on cycle piece list.\n");
}
#define CHECK_CYCLE_PIECE_FRAME(f)					\
  do check_cycle_piece_frame ((f), __FILE__, __LINE__); while (0)

static void check_kill_list_frame (struct gc_rec_frame *f,
				   const char *file, int line)
{
  LOW_CHECK_REC_FRAME (f, file, line);
  if ((f->rf_flags & (GC_ON_CYCLE_PIECE_LIST|GC_ON_KILL_LIST)) !=
      GC_ON_KILL_LIST)
    dloc_gc_fatal (file, line, f->data, 0, "Frame is not on kill list.\n");
  if (f->prev)
    dloc_gc_fatal (file, line, f->data, 0,
		   "Prev pointer set for frame on kill list.\n");
}
#define CHECK_KILL_LIST_FRAME(f)					\
  do check_kill_list_frame ((f), __FILE__, __LINE__); while (0)

#else  /* !PIKE_DEBUG */
#define CHECK_REC_STACK_FRAME(f) do {} while (0)
#define CHECK_CYCLE_PIECE_FRAME(f) do {} while (0)
#define CHECK_KILL_LIST_FRAME(f) do {} while (0)
#endif	/* !PIKE_DEBUG */

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
    struct free_extra_frame *l = alloc_free_extra_frame();
    l->data = a;
    l->type = type;
    l->next = free_extra_list;
    free_extra_list = l;
  }

  gc_add_extra_ref(a);
  m->flags |= GC_GOT_DEAD_REF;
}

int gc_mark(void *a)
{
  struct marker *m;

#ifdef PIKE_DEBUG
  if (Pike_in_gc == GC_PASS_ZAP_WEAK && !find_marker (a))
    gc_fatal (a, 0, "gc_mark() called for for thing without marker "
	      "in zap weak pass.\n");
#endif

  m = get_marker (a);

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
    if (m->flags & GC_FREE_VISITED) {
      debug_malloc_touch (a);
      return 0;
    }
    else {
      debug_malloc_touch (a);
      m->flags |= GC_FREE_VISITED;
      return 1;
    }
  }

  else if (m->flags & GC_MARKED) {
    debug_malloc_touch (a);
#ifdef PIKE_DEBUG
    if (m->weak_refs != 0)
      gc_fatal(a, 0, "weak_refs changed in marker already visited by gc_mark().\n");
#endif
    return 0;
  }
  else {
    debug_malloc_touch (a);
    if (m->weak_refs) {
      gc_ext_weak_refs -= m->weak_refs;
      m->weak_refs = 0;
    }
    m->flags = (m->flags & ~GC_NOT_REFERENCED) | GC_MARKED;
    DO_IF_DEBUG(marked++);
    return 1;
  }
}

void gc_move_marker (void *old, void *new)
{
  struct marker *m = find_marker (old);

#ifdef PIKE_DEBUG
  if (!Pike_in_gc || Pike_in_gc >= GC_PASS_FREE)
    Pike_fatal ("gc move mark attempted in invalid pass.\n");
  if (!old) Pike_fatal ("Got null pointer in old.\n");
  if (!new) Pike_fatal ("Got null pointer in new.\n");
  if (!m) Pike_fatal ("Have no marker for old block %p.\n", old);
  if (find_marker (new))
    Pike_fatal ("New block %p already got a marker.\n", new);
#endif

  move_marker (m, debug_malloc_pass (new));
}

PMOD_EXPORT void gc_cycle_enqueue(gc_cycle_check_cb *checkfn, void *data, int weak)
{
  struct link_frame *l = alloc_link_frame();
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
  if (stack_top == &sentinel_frame)
    gc_fatal (data, 0, "No thing on rec stack to follow links from.\n");
#endif
  l->data = data;
  l->checkfn = checkfn;
  l->weak = weak;
  l->prev = stack_top->u.link_top;
#ifdef GC_STACK_DEBUG
  fprintf (stderr, "push link %p [%p in %p]: ", l, stack_top->u.link_top, stack_top);
  describe_link_frame (l);
  fputc('\n', stderr);
#endif
  stack_top->u.link_top = l;
}

static struct gc_rec_frame *gc_cycle_enqueue_rec (void *data)
{
  struct gc_rec_frame *r = alloc_gc_rec_frame();
#ifdef PIKE_DEBUG
  if (Pike_in_gc != GC_PASS_CYCLE)
    gc_fatal(data, 0, "Use of the gc frame stack outside the cycle check pass.\n");
  r->next = (struct gc_rec_frame *) (ptrdiff_t) -1;
#endif
  r->data = data;
  r->u.link_top = NULL;
  r->prev = stack_top;
  r->cycle_id = r;
  r->cycle_piece = NULL;
#ifdef GC_STACK_DEBUG
  fprintf (stderr, "push rec  %p [%p]: ", r, stack_top);
  describe_rec_frame (r);
  fputc('\n', stderr);
#endif
  stack_top->next = r;
  stack_top = r;
  return r;
}

void gc_cycle_run_queue()
{
#ifdef PIKE_DEBUG
  if (Pike_in_gc != GC_PASS_CYCLE)
    Pike_fatal("Use of the gc frame stack outside the cycle check pass.\n");
#endif

  while (stack_top != &sentinel_frame) {
    while (stack_top->u.link_top) {
      struct link_frame l = *stack_top->u.link_top;
#ifdef GC_STACK_DEBUG
      fprintf (stderr, "pop  link %p [%p in %p]: ",
	       stack_top->u.link_top, l.prev, stack_top);
      describe_link_frame (stack_top->u.link_top);
      fputc ('\n', stderr);
#endif
      really_free_link_frame (stack_top->u.link_top);
      stack_top->u.link_top = l.prev;
      l.checkfn (l.data, l.weak); /* Might change stack_top. */
    }

#ifdef GC_STACK_DEBUG
    fprintf (stderr, "pop  rec  %p [%p]: ", stack_top, stack_top->prev);
    describe_rec_frame (stack_top);
    fputc ('\n', stderr);
#endif
    CHECK_REC_STACK_FRAME (stack_top);

#ifdef PIKE_DEBUG
    {
      struct gc_rec_frame *old_stack_top = stack_top;
      gc_cycle_pop();
      if (stack_top == old_stack_top)
	fatal ("gc_cycle_pop didn't pop the stack.\n");
    }
#else
    gc_cycle_pop();
#endif
  }
}

#ifdef GC_CYCLE_DEBUG
static int gc_cycle_indent = 0;
#define CYCLE_DEBUG_MSG(REC, TXT) do {					\
    struct gc_rec_frame *r_ = (REC);					\
    fprintf (stderr, "%*s%-35s %p [%p] ", gc_cycle_indent, "",		\
	     (TXT), r_ ? r_->data : NULL, stack_top->data);		\
    if (r_) describe_rec_frame (r_);					\
    putc ('\n', stderr);						\
  } while (0)
#else
#define CYCLE_DEBUG_MSG(REC, TXT) do {} while (0)
#endif

#ifdef DEBUG_MALLOC
static void check_cycle_ids_on_stack (struct gc_rec_frame *beg,
				      struct gc_rec_frame *pos,
				      const char *where)
{
  struct gc_rec_frame *l, **stack_arr;
  size_t i;
  for (i = 0, l = &sentinel_frame; l != stack_top; i++, l = l->next) {}
  stack_arr = alloca (i * sizeof (struct gc_rec_frame *));
  for (i = 0, l = &sentinel_frame; l != stack_top; i++, l = l->next)
    stack_arr[i] = l->next;
  for (i = 0, l = &sentinel_frame; l != stack_top; i++, l = l->next) {
    size_t j;
    for (j = 0; j <= i; j++)
      if (stack_arr[j] == l->next->cycle_id)
	goto cycle_id_ok;
    {
      struct gc_rec_frame *err = l->next;
      fprintf (stderr, "cycle_id for frame %p not earlier on stack (%s).\n",
	       err, where);
      for (l = stack_top; l != &sentinel_frame; l = l->prev) {
	fprintf (stderr, "  %p%s ", l,
		 l == beg ? " (beg):" : l == pos ? " (pos):" : ":      ");
	describe_rec_frame (l);
	fputc ('\n', stderr);
      }
      fatal ("cycle_id for frame %p not earlier on stack (%s).\n", err, where);
    }
  cycle_id_ok:;
  }
}
#endif

static struct gc_rec_frame *rotate_rec_stack (struct gc_rec_frame *beg,
					      struct gc_rec_frame *pos)
/* Performs a rotation of the recursion stack so the part from pos
 * down to the end gets before the part from beg down to pos. The beg
 * position might be moved further back the list to avoid breaking
 * strong link sequences. Returns the actual beg position. Example:
 *
 *                         strong
 * a1 <=> ... <=> a2 <=> b1 <*> b2 <=> ... <=> b3 <=> c1 <=> ... <=> c2
 *                              ^ beg                 ^ pos
 *
 * becomes
 *
 *                  broken                       strong
 * a1 <=> ... <=> a2 <#> c1 <=> ... <=> c2 <=> b1 <*> b2 <=> ... <=> b3
 *                       ^ pos                 ^      ^ beg
 *                                             returned
 *
 * Note: The part from pos down to the end is assumed to not contain
 * any weak refs. (If it does they must be cleared, unless the link
 * before beg is weak.)
 */
{
  CYCLE_DEBUG_MSG (beg, "> rotate_rec_stack, requested beg");

#ifdef PIKE_DEBUG
  if (Pike_in_gc != GC_PASS_CYCLE)
    Pike_fatal("Use of the gc frame stack outside the cycle check pass.\n");
  CHECK_REC_STACK_FRAME (beg);
  CHECK_REC_STACK_FRAME (pos);
  if (beg == pos)
    gc_fatal (beg->data, 0, "Cycle already broken at requested position.\n");
#endif

#ifdef GC_STACK_DEBUG
  fprintf(stderr,"Stack before:\n");
  {
    struct gc_rec_frame *l;
    for (l = stack_top; l != &sentinel_frame; l = l->prev) {
      fprintf (stderr, "  %p%s ", l,
	       l == beg ? " (beg):" : l == pos ? " (pos):" : ":      ");
      describe_rec_frame (l);
      fputc ('\n', stderr);
    }
  }
#endif

  /* Always keep chains of strong refs continuous, or else we risk
   * breaking the order in a later rotation. */
  for (; beg->rf_flags & GC_PREV_STRONG; beg = beg->prev)
    CYCLE_DEBUG_MSG (beg, "> rotate_rec_stack, skipping strong");
#ifdef PIKE_DEBUG
  if (beg == &sentinel_frame) fatal ("Strong ref chain ended up off stack.\n");
#endif
  CYCLE_DEBUG_MSG (beg, "> rotate_rec_stack, actual beg");

  {
    struct gc_rec_frame *new_stack_top = pos->prev;

    beg->prev->next = pos;
    pos->prev = beg->prev;

    stack_top->next = beg;
    beg->prev = stack_top;

    stack_top = new_stack_top;
#ifdef PIKE_DEBUG
    stack_top->next = (struct gc_rec_frame *) (ptrdiff_t) -1;
#endif
  }

#ifdef PIKE_DEBUG
  frame_rot++;
#endif

  pos->rf_flags |= GC_PREV_BROKEN;

#ifdef GC_STACK_DEBUG
  fprintf(stderr,"Stack after:\n");
  {
    struct gc_rec_frame *l;
    for (l = stack_top; l != &sentinel_frame; l = l->prev) {
      fprintf (stderr, "  %p%s ", l,
	       l == beg ? " (beg):" : l == pos ? " (pos):" : ":      ");
      describe_rec_frame (l);
      fputc ('\n', stderr);
    }
  }
#endif

  return beg;
}

int gc_cycle_push(void *data, struct marker *m, int weak)
{
  struct marker *pm;

#ifdef PIKE_DEBUG
  if (gc_is_watching && m && m->flags & GC_WATCHED) {
    /* This is useful to set breakpoints on. */
    gc_watched_found (m, "gc_cycle_push()");
  }

  debug_malloc_touch (data);

  if (!data) Pike_fatal ("Got null pointer.\n");
  if (m->data != data) Pike_fatal ("Got wrong marker.\n");
  if (Pike_in_gc != GC_PASS_CYCLE)
    Pike_fatal("GC cycle push attempted in invalid pass.\n");
  if (gc_debug && !(m->flags & GC_PRETOUCHED))
    gc_fatal (data, 0, "gc_cycle_push() called for untouched thing.\n");
  if (!gc_destruct_everything) {
    if ((!(m->flags & GC_NOT_REFERENCED) || m->flags & GC_MARKED) &&
	*(INT32 *) data)
      gc_fatal (data, 1, "Got a referenced marker to gc_cycle_push.\n");
    if (m->flags & GC_XREFERENCED)
      gc_fatal (data, 1, "Doing cycle check in externally referenced thing "
		"missed in mark pass.\n");
  }
  if (weak && stack_top == &sentinel_frame)
    gc_fatal (data, 1, "weak is %d when stack is empty.\n", weak);
  if (gc_debug > 1) {
    struct array *a;
    struct object *o;
    struct program *p;
    struct mapping *m;
    struct multiset *l;
    for(a = gc_internal_array; a; a = a->next)
      if(a == (struct array *) data) goto on_gc_internal_lists;
    for(o = gc_internal_object; o; o = o->next)
      if(o == (struct object *) data) goto on_gc_internal_lists;
    for(p = gc_internal_program; p; p = p->next)
      if(p == (struct program *) data) goto on_gc_internal_lists;
    for(m = gc_internal_mapping; m; m = m->next)
      if(m == (struct mapping *) data) goto on_gc_internal_lists;
    for(l = gc_internal_multiset; l; l = l->next)
      if(l == (struct multiset *) data) goto on_gc_internal_lists;
    gc_fatal (data, 0, "gc_cycle_check() called for thing not on gc_internal lists.\n");
  on_gc_internal_lists:
    ; /* We must have a least one expression after a label! - Hubbe */
  }
#endif

  if (stack_top->rf_flags & GC_MARK_LIVE) {
    /* Only recurse through things already handled; we'll get to the
     * other later in the normal recursion. */
    if (m->flags & GC_CYCLE_CHECKED && !(m->flags & GC_LIVE)) {
      CYCLE_DEBUG_MSG (m->frame, "gc_cycle_push, mark live");
      goto mark_live;
    }
    CYCLE_DEBUG_MSG (m->frame, "gc_cycle_push, no mark live");
    return 0;
  }

  if (stack_top == &sentinel_frame)
    pm = NULL;
  else {
    pm = find_marker (stack_top->data);
#ifdef PIKE_DEBUG
    if (!pm)
      gc_fatal (stack_top->data, 0, "No marker for thing on top of the stack.\n");
#endif
  }

#ifdef PIKE_DEBUG
  if (weak < 0 && stack_top->rf_flags & GC_FOLLOWED_NONSTRONG)
    gc_fatal (data, 0, "Followed strong link too late.\n");
  if (weak >= 0) stack_top->rf_flags |= GC_FOLLOWED_NONSTRONG;
#endif

  if (m->frame) {
    /* A cyclic ref or a ref to something on the kill list is found. */
    struct gc_rec_frame *cycle_frame = m->frame;

    if (cycle_frame->rf_flags & GC_ON_KILL_LIST)
      CYCLE_DEBUG_MSG (cycle_frame, "gc_cycle_push, ref to kill list");
    else if (cycle_frame == stack_top)
      CYCLE_DEBUG_MSG (cycle_frame, "gc_cycle_push, self-ref");
    else if (weak > 0)
      /* Ignore weak refs since they always are eligible to be broken anyway. */
      CYCLE_DEBUG_MSG (cycle_frame, "gc_cycle_push, weak cyclic ref");

    else {
      struct gc_rec_frame *weakly_refd = NULL;
      struct gc_rec_frame *brokenly_refd = NULL;
      struct gc_rec_frame *nonstrongly_refd = NULL;
#ifdef PIKE_DEBUG
      if (stack_top == &sentinel_frame)
	gc_fatal (data, 0, "Cyclic ref involves dummy sentinel frame.\n");
      CHECK_REC_STACK_FRAME (stack_top);
#endif

      CYCLE_DEBUG_MSG (cycle_frame, "gc_cycle_push, cyclic ref");

      /* Find the corresponding frame still on the stack and compress
       * indirect cycle_id links. */
      {
	struct gc_rec_frame *r;
	for (r = cycle_frame; !r->prev; r = r->cycle_id)
	  CHECK_CYCLE_PIECE_FRAME (r);
	while (cycle_frame != r) {
	  struct gc_rec_frame *next = cycle_frame->cycle_id;
	  cycle_frame->cycle_id = r;
	  cycle_frame = next;
	}
	CHECK_REC_STACK_FRAME (cycle_frame);
      }

      if (!weak) {
	struct gc_rec_frame *r;
	CYCLE_DEBUG_MSG (cycle_frame, "gc_cycle_push, search normal");
	/* Find the last weakly linked thing and the last thing whose
	 * normal ref already has been broken. */
	for (r = stack_top; r != cycle_frame; r = r->prev) {
	  CHECK_REC_STACK_FRAME (r);
	  DO_IF_DEBUG (link_search++);
	  if (r->rf_flags & GC_PREV_WEAK) {
	    CYCLE_DEBUG_MSG (r, "> gc_cycle_push, found weak");
	    weakly_refd = r;
	    break;
	  }
	  if (!brokenly_refd && (r->rf_flags & GC_PREV_BROKEN)) {
	    CYCLE_DEBUG_MSG (r, "> gc_cycle_push, found broken");
	    brokenly_refd = r;
	  }
	  else
	    CYCLE_DEBUG_MSG (r, "> gc_cycle_push, search");
	}
      }

      else {			/* weak < 0 */
	struct gc_rec_frame *r;
	CYCLE_DEBUG_MSG (cycle_frame, "gc_cycle_push, search strong");
	/* Find the last weakly linked thing and the last one which
	 * isn't strongly linked. */
	for (r = stack_top; r != cycle_frame; r = r->prev) {
	  CHECK_REC_STACK_FRAME (r);
	  DO_IF_DEBUG (link_search++);
	  if (r->rf_flags & GC_PREV_WEAK) {
	    CYCLE_DEBUG_MSG (r, "> gc_cycle_push, found weak");
	    weakly_refd = r;
	    break;
	  }
	  if (!nonstrongly_refd && !(r->rf_flags & GC_PREV_STRONG)) {
	    nonstrongly_refd = r;
	    CYCLE_DEBUG_MSG (r, "> gc_cycle_push, found nonstrong");
	  }
	  if (!brokenly_refd && (r->rf_flags & GC_PREV_BROKEN)) {
	    CYCLE_DEBUG_MSG (r, "> gc_cycle_push, found broken");
	    brokenly_refd = r;
	  }
#ifdef GC_CYCLE_DEBUG
	  else if (r != nonstrongly_refd)
	    CYCLE_DEBUG_MSG (r, "> gc_cycle_push, search");
#endif
	}
#ifdef PIKE_DEBUG
	if (weak && r == cycle_frame && !nonstrongly_refd) {
	  fprintf(stderr, "Only strong links in cycle:\n");
	  for (r = cycle_frame;; r = r->next) {
	    describe (r->data);
	    locate_references (r->data);
	    if (r == stack_top) break;
	    fprintf(stderr, "========= next =========\n");
	  }
	  gc_fatal(0, 0, "Only strong links in cycle.\n");
	}
#endif
      }

      if (weakly_refd) {
	/* The backward link is normal or strong and there are one or
	 * more weak links in the cycle. Let's break it at the last
	 * one (to avoid having to clobber the others after the
	 * rotation). */
	CYCLE_DEBUG_MSG (weakly_refd, "gc_cycle_push, weak break");
	rotate_rec_stack (cycle_frame, weakly_refd);
#ifdef DEBUG_MALLOC
	check_cycle_ids_on_stack (cycle_frame, weakly_refd, "after weak break");
#endif
      }

      else {
	struct gc_rec_frame *cycle_id = cycle_frame->cycle_id;
	struct gc_rec_frame *break_pos;

	if (brokenly_refd) {
	  /* Found a link that already has been broken once, so we
	   * prefer to break at it again. */
	  CYCLE_DEBUG_MSG (brokenly_refd, "gc_cycle_push, break at broken");
	  break_pos = brokenly_refd;
	}
	else if (!weak) {
	  CYCLE_DEBUG_MSG (cycle_frame, "gc_cycle_push, no break spot found");
	  break_pos = NULL;
	}
	else {			/* weak < 0 */
	  /* The backward link is strong. Must break the cycle at the
	   * last nonstrong link. */
	  CYCLE_DEBUG_MSG (nonstrongly_refd, "gc_cycle_push, nonstrong break");
	  break_pos = nonstrongly_refd;
	}

	if (break_pos) {
	  struct gc_rec_frame *rot_beg;
	  rot_beg = rotate_rec_stack (cycle_frame, break_pos);
	  cycle_frame->rf_flags =
	    (cycle_frame->rf_flags & ~(GC_PREV_WEAK|GC_PREV_BROKEN)) | GC_PREV_STRONG;

	  if (rot_beg->cycle_id != break_pos->prev->cycle_id)
	    /* Ensure that the cycle id frame is kept deepest in the
	     * stack: Since there's no already marked cycle that
	     * continues past the beginning of the rotated portion
	     * (rot_beg and break_pos->prev were previously next to
	     * each other), break_pos is now the deepest frame in the
	     * cycle. */
	    cycle_id = break_pos;
	}

	/* Mark the cycle. NB: This causes O(n^2) complexity for some
	 * kinds of data structures. */
	CHECK_REC_STACK_FRAME (cycle_id);
	{
	  struct gc_rec_frame *r, *bottom = break_pos ? break_pos : cycle_frame;
	  CYCLE_DEBUG_MSG (cycle_id, "gc_cycle_push, cycle");
	  for (r = stack_top;; r = r->prev) {
	    r->cycle_id = cycle_id;
	    CHECK_REC_STACK_FRAME (r);
	    CYCLE_DEBUG_MSG (r, "> gc_cycle_push, mark cycle 1");
	    if (r == bottom) break;
	  }
	}
#ifdef DEBUG_MALLOC
	check_cycle_ids_on_stack (cycle_frame, break_pos, "after nonweak break");
#endif
      }
    }
  }

  else
    if (!(m->flags & GC_CYCLE_CHECKED)) {
      struct gc_rec_frame *r;
#ifdef PIKE_DEBUG
      cycle_checked++;
      if (m->frame)
	gc_fatal (data, 0, "Marker already got a frame.\n");
#endif

      m->flags |= GC_CYCLE_CHECKED | (pm ? pm->flags & GC_LIVE : 0);
      m->frame = r = gc_cycle_enqueue_rec (data);
      debug_malloc_touch (data);
      if (weak) {
	if (weak > 0) r->rf_flags = GC_PREV_WEAK;
	else r->rf_flags = GC_PREV_STRONG;
      }
      else
	r->rf_flags = 0;

#ifdef GC_CYCLE_DEBUG
      if (weak > 0) CYCLE_DEBUG_MSG (r, "gc_cycle_push, recurse weak");
      else if (weak < 0) CYCLE_DEBUG_MSG (r, "gc_cycle_push, recurse strong");
      else CYCLE_DEBUG_MSG (r, "gc_cycle_push, recurse");
      gc_cycle_indent += 2;
#endif
      return 1;
    }

  /* Should normally not recurse now, but got to do that anyway if we
   * must propagate GC_LIVE flags. */
  if (!pm || !(pm->flags & GC_LIVE) || m->flags & GC_LIVE) {
    CYCLE_DEBUG_MSG (m->frame ? m->frame : NULL, "gc_cycle_push, no recurse");
    return 0;
  }

  /* Initialize mark live recursion. */
  gc_cycle_enqueue_rec (NULL)->rf_flags = GC_MARK_LIVE;
#ifdef GC_CYCLE_DEBUG
  CYCLE_DEBUG_MSG (m->frame ? m->frame : NULL, "gc_cycle_push, mark live begins");
  gc_cycle_indent += 2;
#endif

mark_live:
#ifdef PIKE_DEBUG
  if (m->flags & GC_LIVE)
    Pike_fatal("Shouldn't mark live recurse when there's nothing to do.\n");
#endif
  m->flags |= GC_LIVE;
  debug_malloc_touch (data);

  if (m->flags & GC_GOT_DEAD_REF) {
    /* A thing previously popped as dead is now being marked live.
     * Have to remove the extra ref added by gc_cycle_pop(). */
    gc_free_extra_ref (data);
    if (!sub_ref ((struct ref_dummy *) data)) {
#ifdef PIKE_DEBUG
      gc_fatal (data, 0, "Thing got zero refs after removing the dead gc ref.\n");
#endif
    }
  }

  /* Visit links without pushing a rec frame. */
#ifdef PIKE_DEBUG
  mark_live++;
#endif
  return 1;
}

static void gc_cycle_pop()
{
#ifdef PIKE_DEBUG
  if (Pike_in_gc != GC_PASS_CYCLE)
    Pike_fatal("GC cycle pop attempted in invalid pass.\n");
  if (stack_top->u.link_top)
    gc_fatal (stack_top->data, 0, "Link list not empty for popped rec frame.\n");
#endif
#ifdef GC_CYCLE_DEBUG
  gc_cycle_indent -= 2;
#endif

  if (stack_top->rf_flags & GC_MARK_LIVE) {
    struct gc_rec_frame *r = stack_top->prev;
    CYCLE_DEBUG_MSG (stack_top, "gc_cycle_pop, mark live ends");
    really_free_gc_rec_frame (stack_top);
    stack_top = r;
  }

  else {
    struct gc_rec_frame *popped = stack_top;

#ifdef PIKE_DEBUG
    {
      void *data = popped->data;
      struct marker *m = find_marker (data);
      if (gc_is_watching && m && m->flags & GC_WATCHED) {
	/* This is useful to set breakpoints on. */
	gc_watched_found (m, "gc_cycle_pop()");
      }
      if (!(m->flags & GC_CYCLE_CHECKED))
	gc_fatal (data, 0, "Marker being popped doesn't have GC_CYCLE_CHECKED.\n");
      if (!gc_destruct_everything) {
	if ((!(m->flags & GC_NOT_REFERENCED) || m->flags & GC_MARKED) &&
	    *(INT32 *) data)
	  gc_fatal (data, 1, "Got a referenced marker to gc_cycle_pop.\n");
	if (m->flags & GC_XREFERENCED)
	  gc_fatal (data, 1, "Doing cycle check in externally referenced thing "
		    "missed in mark pass.\n");
      }
      if (popped->next != (struct gc_rec_frame *) (ptrdiff_t) -1)
	gc_fatal (data, 0, "Popped rec frame got stuff in the next pointer.\n");
    }
#endif

    stack_top = popped->prev;
#ifdef PIKE_DEBUG
    if (stack_top != &sentinel_frame) CHECK_REC_STACK_FRAME (stack_top);
    CHECK_REC_STACK_FRAME (popped);
#endif
    popped->prev = NULL;

    if (popped->cycle_id != popped) {
      /* Part of a cycle that extends further back - move to the cycle
       * piece list of the previous frame. */
      struct gc_rec_frame *this_list_last =
	popped->cycle_piece ? popped->cycle_piece->u.last_cycle_piece : popped;
#ifdef PIKE_DEBUG
      if (this_list_last->cycle_piece)
	gc_fatal (this_list_last->data, 0,
		  "This frame should be last on the cycle piece list.\n");
      popped->rf_flags |= GC_ON_CYCLE_PIECE_LIST;
      CHECK_CYCLE_PIECE_FRAME (this_list_last);
#endif
      CYCLE_DEBUG_MSG (popped, "gc_cycle_pop, keep cycle piece");

      if (!stack_top->cycle_piece)
	popped->u.last_cycle_piece = this_list_last;
      else {
	/* Link in the popped frame and its cycle piece list before
	 * the one that the previous frame has. */
	struct gc_rec_frame *up_list_first = stack_top->cycle_piece;
	struct gc_rec_frame *up_list_last = up_list_first->u.last_cycle_piece;
#ifdef PIKE_DEBUG
	CHECK_CYCLE_PIECE_FRAME (up_list_last);
	if (up_list_last->cycle_piece)
	  gc_fatal (up_list_last->data, 0,
		    "This frame should be last on the cycle piece list.\n");
#endif
	CYCLE_DEBUG_MSG (up_list_first, "> gc_cycle_pop, inserted before");
	this_list_last->cycle_piece = up_list_first;
	popped->u.last_cycle_piece = up_list_last;
      }
      stack_top->cycle_piece = popped;
      popped->cycle_id = stack_top;

      CHECK_CYCLE_PIECE_FRAME (popped);
      CHECK_REC_STACK_FRAME (stack_top);
    }

    else {
      /* Free or move to the kill list the popped frame and its cycle
       * piece list. */
      struct gc_rec_frame **kill_list_ptr = &kill_list;
      struct gc_rec_frame *cycle_id = NULL;

#ifdef PIKE_DEBUG
      {
	struct gc_rec_frame *r;
	for (r = popped->cycle_piece; r; r = r->cycle_piece)
	  /* Can't do this while the list is being freed below. */
	  CHECK_CYCLE_PIECE_FRAME (r);
      }
#endif

      CYCLE_DEBUG_MSG (popped, "gc_cycle_pop, popping cycle");

      do {
	struct marker *m = find_marker (popped->data);
	struct gc_rec_frame *next = popped->cycle_piece;

	if (m->flags & GC_LIVE_OBJ) {
	  /* Move to the kill list. */
#ifdef PIKE_DEBUG
	  popped->rf_flags &= ~GC_ON_CYCLE_PIECE_LIST;
	  popped->cycle_piece = popped->u.last_cycle_piece =
	    (struct gc_rec_frame *) (ptrdiff_t) -1;
#endif
	  popped->next = *kill_list_ptr;
	  *kill_list_ptr = popped;
	  kill_list_ptr = &popped->next;
	  popped->rf_flags |= GC_ON_KILL_LIST;

	  /* Ensure that the frames on the kill list have a valid
	   * cycle id frame and that every frame is linked directly to
	   * it. This is only for the sake of warn_bad_cycles. */
	  if (!cycle_id) cycle_id = popped;
	  popped->cycle_id = cycle_id;

	  /* This extra ref is taken away in the kill pass. Don't add one
	   * if it got an extra ref already due to weak free. */
	  if (!(m->flags & GC_GOT_DEAD_REF))
	    gc_add_extra_ref (popped->data);

	  CHECK_KILL_LIST_FRAME (popped);
	  CYCLE_DEBUG_MSG (popped, "> gc_cycle_pop, move to kill list");
	}

	else {
	  if (!(m->flags & GC_LIVE)) {
	    /* Add an extra ref which is taken away in the free pass. This
	     * is done to not refcount garb the cycles themselves
	     * recursively, which in bad cases can consume a lot of C
	     * stack. */
	    if (!(m->flags & GC_GOT_DEAD_REF)) {
	      gc_add_extra_ref (popped->data);
	      m->flags |= GC_GOT_DEAD_REF;
	    }
	  }
#ifdef PIKE_DEBUG
	  else
	    if (m->flags & GC_GOT_DEAD_REF)
	      gc_fatal (popped->data, 0, "Didn't expect a dead extra ref.\n");
#endif

	  CYCLE_DEBUG_MSG (popped, "> gc_cycle_pop, free");
	  m->frame = NULL;
	  really_free_gc_rec_frame (popped);
	}

	popped = next;
      } while (popped);
    }
  }

#ifdef PIKE_DEBUG
  stack_top->next = (struct gc_rec_frame *) (ptrdiff_t) -1;
#endif
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
	sub_ref ((struct ref_dummy *) a);
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

static void warn_bad_cycles(void)
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

#if 0
  {
    struct gc_pop_frame *p;
    unsigned cycle = 0;
    *obj_arr_ = allocate_array(0);

    for (p = kill_list; p;) {
      if ((cycle = p->cycle)) {
	push_object((struct object *) p->data);
	dmalloc_touch_svalue(Pike_sp-1);
	*obj_arr_ = append_array(*obj_arr_, --Pike_sp);
      }
      p = p->next;
      if (p ? ((unsigned)(p->cycle != cycle)) : cycle) {
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
#endif

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
  mark_live = frame_rot = link_search = 0;
#endif
  rec_frames = link_frames = free_extra_frames = 0;
  max_rec_frames = max_link_frames = 0;
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
#if defined (PIKE_DEBUG) || defined (DO_PIKE_CLEANUP)
    debug_gc_check_all_types();
#endif
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
    if (stack_top != &sentinel_frame)
      Pike_fatal("Frame stack not empty at end of cycle check pass.\n");
    if (gc_ext_weak_refs != orig_ext_weak_refs)
      Pike_fatal("gc_ext_weak_refs changed from %"PRINTSIZET"u "
	    "to %"PRINTSIZET"u in cycle check pass.\n",
	    orig_ext_weak_refs, gc_ext_weak_refs);
#endif

    GC_VERBOSE_DO(fprintf(stderr,
			  "| cycle: %u internal things visited,\n"
			  "|        %u weak references freed, %d more things to free,\n"
			  "|        %u mark live visits, %u frame rotations,\n"
			  "|        %u links searched, used max %u link frames,\n"
			  "|        %u rec frames and %u free extra frames\n",
			  cycle_checked, weak_freed, delayed_freed - obj_count,
			  mark_live, frame_rot, link_search, max_link_frames,
			  max_rec_frames, free_extra_frames));

#ifdef PIKE_DEBUG
    if (link_frames) fatal ("Leaked %u link frames.\n", link_frames);
#endif
  }

  if (gc_ext_weak_refs) {
    size_t to_free = gc_ext_weak_refs;
#ifdef PIKE_DEBUG
    obj_count = delayed_freed;
#endif
    Pike_in_gc = GC_PASS_ZAP_WEAK;
    /* Zap weak references from external to internal things. That
     * occurs when something has both external weak refs and nonweak
     * cyclic refs from internal things. */
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
    Pike_in_gc=GC_PASS_POSTTOUCH;
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
#if 0 /* Temporarily disabled - Hubbe */
#ifdef PIKE_DEBUG
#ifdef DEBUG_MALLOC
    PTR_HASH_LOOP(marker, i, m)
      if (!(m->flags & (GC_POSTTOUCHED|GC_WEAK_FREED)) &&
	  dmalloc_is_invalid_memory_block(m->data)) {
	fprintf(stderr, "Found a stray marker after posttouch pass: ");
	describe_marker(m);
	fprintf(stderr, "Describing marker location(s):\n");
	debug_malloc_dump_references(m, 2, 1, 0);
	fprintf(stderr, "Describing thing for marker:\n");
	Pike_in_gc = 0;
	describe(m->data);
	Pike_in_gc = GC_PASS_POSTTOUCH;
	Pike_fatal("Fatal in garbage collector.\n");
      }
#endif
#endif
#endif
    GC_VERBOSE_DO(fprintf(stderr, "| posttouch\n"));
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

  if (free_extra_frames > tot_max_free_extra_frames)
    tot_max_free_extra_frames = free_extra_frames;

  /* We might occasionally get things to gc_delayed_free that the free
   * calls above won't find. They're tracked in this list. */
  while (free_extra_list) {
    struct free_extra_frame *next = free_extra_list->next;
    union anything u;
    u.refs = (INT32 *) free_extra_list->data;
    gc_free_extra_ref (u.refs);
    free_short_svalue (&u, free_extra_list->type);
    really_free_free_extra_frame (free_extra_list);
    free_extra_list = next;
  }

#ifdef PIKE_DEBUG
  if (free_extra_frames) fatal ("Leaked %u free extra frames.\n", free_extra_frames);
#endif

  GC_VERBOSE_DO(fprintf(stderr, "| free: %"PRINTSIZET"u unreferenced, "
			"%d really freed, %u left with live references\n",
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
  if (Pike_interpreter.evaluator_stack && !gc_destruct_everything) {
    objs -= num_objects;
    warn_bad_cycles();
    objs += num_objects;
  }
#if defined (PIKE_DEBUG) || defined (DO_PIKE_CLEANUP)
  destroy_count = 0;
#endif

  {
    enum object_destruct_reason reason =
#ifdef DO_PIKE_CLEANUP
      gc_destruct_everything ? DESTRUCT_CLEANUP :
#endif
      DESTRUCT_GC;

#ifdef PIKE_DEBUG
      {
	struct gc_rec_frame *r;
	for (r = kill_list; r != &sentinel_frame; r = r->next)
	  /* Can't do this while the list is being freed below. */
	  CHECK_KILL_LIST_FRAME (r);
      }
#endif

    while (kill_list != &sentinel_frame) {
      struct gc_rec_frame *next = kill_list->next;
      struct object *o = (struct object *) kill_list->data;

#ifdef PIKE_DEBUG
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

      destruct_object (o, reason);
      free_object(o);
      gc_free_extra_ref(o);
#if defined (PIKE_DEBUG) || defined (DO_PIKE_CLEANUP)
      destroy_count++;
#endif
      really_free_gc_rec_frame (kill_list);
      kill_list = next;
    }
  }

#ifdef PIKE_DEBUG
  if (rec_frames) fatal ("Leaked %u rec frames.\n", rec_frames);
#endif

  GC_VERBOSE_DO(fprintf(stderr, "| kill: %u objects killed, "
			"%"PRINTSIZET"u things really freed\n",
			destroy_count, pre_kill_objs - num_objects));

  Pike_in_gc=GC_PASS_DESTRUCT;
  /* Destruct objects on the destruct queue. */
  GC_VERBOSE_DO(obj_count = num_objects);
  destruct_objects_to_destruct();
  GC_VERBOSE_DO(fprintf(stderr, "| destruct: %d things really freed\n",
			obj_count - num_objects));

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
      if (cpu_time_is_thread_local)
	Pike_interpreter.thread_state->auto_gc_time += last_gc_time;
      else
	auto_gc_time += last_gc_time;
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
	fprintf(stderr, "done (%u %s destructed)%s\n",
		destroy_count, destroy_count == 1 ? "was" : "were", timestr);
      else
#endif
	fprintf(stderr, "done (%"PRINTSIZET"d of %"PRINTSIZET"d "
		"%s unreferenced)%s\n",
		unreferenced, start_num_objs,
		unreferenced == 1 ? "was" : "were",
		timestr);
    }
  }

#ifdef PIKE_DEBUG
  UNSET_ONERROR (uwp);
  tot_cycle_checked += cycle_checked;
  tot_mark_live += mark_live, tot_frame_rot += frame_rot;
#endif
  if (max_rec_frames > tot_max_rec_frames)
    tot_max_rec_frames = max_rec_frames;
  if (max_link_frames > tot_max_link_frames)
    tot_max_link_frames = max_link_frames;

  Pike_in_gc=0;
  exit_gc();

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

#ifdef PIKE_DEBUG
  push_constant_text ("max_rec_frames");
  push_int64 (DO_NOT_WARN ((INT64) tot_max_rec_frames));

  push_constant_text ("max_link_frames");
  push_int64 (DO_NOT_WARN ((INT64) tot_max_link_frames));

  push_constant_text ("max_free_extra_frames");
  push_int64 (DO_NOT_WARN ((INT64) tot_max_free_extra_frames));
#endif

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
  fprintf(stderr,"Max used recursion frames  : %u\n", tot_max_rec_frames);
  fprintf(stderr,"Max used link frames       : %u\n", tot_max_link_frames);
  fprintf(stderr,"Max used free extra frames : %u\n", tot_max_free_extra_frames);
  fprintf(stderr,"Marked live ratio          : %g\n",
	  (double) tot_mark_live / tot_cycle_checked);
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
