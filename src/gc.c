/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"

struct callback *gc_evaluator_callback=0;

#include "array.h"
#include "multiset.h"
#include "mapping.h"
#include "object.h"
#include "program.h"
#include "stralloc.h"
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
#include "builtin_functions.h"
#include "block_allocator.h"

#include <math.h>

#include "block_alloc.h"

int gc_enabled = 1;

/* These defaults are only guesses and hardly tested at all. Please improve. */
double gc_garbage_ratio_low = 0.2;
double gc_time_ratio = 0.05;
double gc_garbage_ratio_high = 0.5;
double gc_min_time_ratio = 1.0/10000.0; /* Martys constant. */

/* This slowness factor approximately corresponds to the average over
 * the last ten gc rounds. (0.9 == 1 - 1/10) */
double gc_average_slowness = 0.9;

/* High-level callbacks.
 * NB: These are initialized from builtin.cmod.
 */
/* Callback called when gc() starts. */
struct svalue gc_pre_cb;

/* Callback called when the mark and sweep phase of the gc() is done. */
struct svalue gc_post_cb;

/* Callback called for each object that is to be destructed explicitly
 * by the gc().
 */
struct svalue gc_destruct_cb;

/* Callback called when the gc() is about to exit. */
struct svalue gc_done_cb;

/* The gc has the following passes:
 *
 * GC_PASS_PREPARE
 *
 * GC_PASS_PRETOUCH
 *   Debug.
 *
 * GC_PASS_CHECK
 *   Counts all internal references (strong, normal and weak)
 *   by calling gc_check() or gc_check_weak() (and thus
 *   real_gc_check() or real_gc_check_weak()). They increase
 *   the reference counters in the marker.
 *
 * GC_PASS_MARK
 *   Recursively mark objects that have more references than
 *   internal references for keeping. Sets the flag
 *   GC_MARKED and clears the flag GC_NOT_REFERENCED
 *   in the marker.
 *
 * GC_PASS_CYCLE
 *   Identify cycles in the unmarked objects.
 *
 * GC_PASS_ZAP_WEAK
 *   Zap weak references to unmarked objects.
 *
 * GC_PASS_POSTTOUCH
 *   Debug.
 *
 * GC_PASS_FREE
 *   Free remaining unmarked objects.
 *
 * GC_PASS_KILL
 *   Destruct remaining unmarked live (lfun::_destruct()) objects.
 *
 * GC_PASS_DESTRUCT
 *   Destruct objects to destruct.
 *
 * And the following simulated passes:
 *
 * GC_PASS_LOCATE
 *   Search for check_for. Note that markers are (intentionally)
 *   NOT used in this pass, and the reported number of references
 *   may be thus larger than the actual number of references due
 *   to double traversal of C-structures caused by this.
 *
 * GC_PASS_DISABLED
 */

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
 * Note: Keep the doc for lfun::_destruct up-to-date with the above.
 */

/* #define GC_DEBUG */
/* #define GC_VERBOSE */
/* #define GC_CYCLE_DEBUG */
/* #define GC_STACK_DEBUG */
/* #define GC_INTERVAL_DEBUG */

#if defined(GC_VERBOSE) && !defined(PIKE_DEBUG)
#undef GC_VERBOSE
#endif
#ifdef GC_VERBOSE
#define GC_VERBOSE_DO(X) X
#else
#define GC_VERBOSE_DO(X)
#endif

int num_objects = 2;		/* Account for *_empty_array. */
int got_unlinked_things;
ALLOC_COUNT_TYPE num_allocs =0;
ALLOC_COUNT_TYPE alloc_threshold = GC_MIN_ALLOC_THRESHOLD;
PMOD_EXPORT int Pike_in_gc = 0;
unsigned INT16 gc_generation = 1;
time_t last_gc;
int gc_trace = 0, gc_debug = 0;
#ifdef DO_PIKE_CLEANUP
int gc_destruct_everything = 0;
#endif
size_t gc_ext_weak_refs;

ALLOC_COUNT_TYPE saved_alloc_threshold;
/* Used to backup alloc_threshold if the gc is disabled, so that it
 * can be restored when it's enabled again. This is to not affect the
 * gc interval if it's disabled only for a short duration.
 * alloc_threshold is set to GC_MAX_ALLOC_THRESHOLD while it's
 * disabled, to avoid complicating the test in GC_ALLOC(). */

static double objects_alloced = 0.0;
static double objects_freed = 0.0;
static double gc_time = 0.0, non_gc_time = 0.0;
static cpu_time_t last_gc_end_real_time = -1;
cpu_time_t auto_gc_time = 0;
cpu_time_t auto_gc_real_time = 0;

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
  struct gc_rec_frame *prev;	/* The previous frame in the recursion stack.
				 * NULL for frames not in the stack (i.e. on a
				 * cycle piece list or in the kill list). */
  struct gc_rec_frame *next;	/* The next frame in the recursion stack or the
				 * kill list. Undefined for frames on cycle
				 * piece lists. */
  struct gc_rec_frame *cycle_id;/* For a frame in the recursion stack: The
				 * cycle identifier frame.
				 * For a frame on a cycle piece list: The frame
				 * in the recursion stack whose cycle piece
				 * list this frame is in. */
  struct gc_rec_frame *cycle_piece;/* The start of the cycle piece list for
				 * frames on the recursion stack, or the next
				 * frame in the list for frames in cycle piece
				 * lists. */
  union {
    struct link_frame *link_top;/* The top of the link stack for frames on the
				 * recursion stack. */
    struct gc_rec_frame *last_cycle_piece;/* In the first frame on a cycle
				 * piece list, this is used to point to the
				 * last frame in the list. */
  } u;
};

/* rf_flags bits. */
#define GC_PREV_WEAK		0x0001
#define GC_PREV_STRONG		0x0002
#define GC_PREV_BROKEN		0x0004
#define GC_MARK_LIVE		0x0008
#define GC_ON_KILL_LIST		0x0010
#ifdef PIKE_DEBUG
#define GC_ON_CYCLE_PIECE_LIST	0x0020
#define GC_FRAME_FREED		0x0040
#define GC_FOLLOWED_NONSTRONG	0x0080
#define GC_IS_VALID_CP_CYCLE_ID	0x0100
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
 * gc_cycle_check_* functions and pushed as link_frames onto a link
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
 * A sequence of frames on the recursion stack forms a cycle iff they
 * have the same value in gc_rec_frame.cycle_id. A cycle is always
 * continuous on the stack.
 *
 * Furthermore, the cycle_ids always point to the first (deepest)
 * frame on the stack that is part of the cycle. That frame is called
 * the "cycle identifier frame" since all frames in the cycle will end
 * up there if the cycle pointers are followed transitively. The
 * cycle_id pointer in the cycle identifier frame points to itself.
 * Every frame is initially treated as a cycle containing only itself.
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
 * deepest in the stack is destructed first and the recursion stack
 * has precedence over the cycle piece list (the reason for that is
 * explained later). To illustrate:
 *                                   ,- stack_top
 *                                  v
 *           t1 <=> t2 <=> ... <=> t3
 *            |      |              `-> t4 -> ... -> t5
 *            |      `-> t6 -> ... -> t7
 *            `-> t8 -> ... -> t9
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
 * If a nonweak backward pointer from t5 to t2 is encountered here, we
 * should prefer to break(*) the weak ref between t3 and t4. The stack
 * is therefore rotated to become:
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
 * If a nonweak backward pointer is found and there are no weak refs
 * on the stack to break at, the section from the top of the stack
 * down to the thing referenced by the backward pointer is marked up
 * as a cycle (possibly extending the cycle which that thing already
 * belongs to). Therefore weak refs never occur inside cycles.
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
 * be handled independently of the earlier cycles, and those earlier
 * cycles can also be extended later on when the subcycle has been
 * popped off. If a ref from the subcycle to an earlier cycle is
 * found, that means that both are really the same cycle, and the
 * frames in the former subcycle will instead become a cycle piece
 * list on a frame in the former preceding cycle.
 *
 * Cycles are always kept continuous on the recursion stack. Since
 * breaking a weak ref doesn't mark up a cycle, it's necessary to
 * rotate between whole cycles when a weak ref is broken. I.e:
 *
 *                                             weak
 *  ... <=> t1a <=> t1b <=> t1c <=> ... <=> t2 <-> t3 <=> ... <=> t4
 *
 * Here all the t1 things are members of a cycle, with t1a being the
 * first and t1c the last. Let's say a nonweak pointer is found from
 * t4 to t1b, and the weak link between t2 and t3 is chosen to be
 * broken. In this case the whole t1 cycle is rotated up:
 *
 *     broken
 *  ... <#> t3 <=> ... <=> t4 <=> t1a <=> t1b <=> t1c <=> ... <=> t2
 *
 * This way the t1 cycle can continue to be processed independently
 * and possibly be popped off separately from the segment between t3
 * and t4.
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
 * A nonweak ref is found from t5 to t2. We get this after rotation
 * (assuming t1 and t2 aren't part of the same cycle):
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
 * treated as one unit together with its cycle piece list.
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
 * from. Nothing else happens while this is done, i.e. no rotations
 * and so forth, so the dummy frame always stays at the top until it's
 * removed again.
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

struct block_allocator gc_rec_frame_allocator =
    BA_INIT_PAGES(sizeof(struct gc_rec_frame), 2);

static void really_free_gc_rec_frame(struct gc_rec_frame * f) {
#ifdef PIKE_DEBUG
  if (f->rf_flags & GC_FRAME_FREED)
    gc_fatal (f->data, 0, "Freeing gc_rec_frame twice.\n");
  f->rf_flags |= GC_FRAME_FREED;
  f->u.link_top = (struct link_frame *) (ptrdiff_t) -1;
  f->prev = f->next = f->cycle_id = f->cycle_piece =
    (struct gc_rec_frame *) (ptrdiff_t) -1;
#endif
  rec_frames--;
  ba_free(&gc_rec_frame_allocator, f);
}

void count_memory_in_gc_rec_frames(size_t *num, size_t * size) {
  ba_count_all(&gc_rec_frame_allocator, num, size);
}

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

static struct block_allocator ba_mixed_frame_allocator
    = BA_INIT_PAGES(sizeof(struct ba_mixed_frame), 2);

void count_memory_in_ba_mixed_frames(size_t *num, size_t * size) {
  ba_count_all(&ba_mixed_frame_allocator, num, size);
}

static inline struct link_frame *alloc_link_frame()
{
  struct ba_mixed_frame *f = ba_alloc(&ba_mixed_frame_allocator);
  if (++link_frames > max_link_frames)
    max_link_frames = link_frames;
  return (struct link_frame *) f;
}

static inline struct free_extra_frame *alloc_free_extra_frame()
{
  struct ba_mixed_frame *f = ba_alloc(&ba_mixed_frame_allocator);
  free_extra_frames++;
  return (struct free_extra_frame *) f;
}

static inline void really_free_link_frame (struct link_frame *f)
{
  link_frames--;
  ba_free(&ba_mixed_frame_allocator, f);
}

static inline void really_free_free_extra_frame (struct free_extra_frame *f)
{
  free_extra_frames--;
  ba_free(&ba_mixed_frame_allocator, f);
}

/* These are only collected for the sake of gc_status. */
static double last_garbage_ratio = 0.0;
static enum {
  GARBAGE_RATIO_LOW, GARBAGE_RATIO_HIGH, GARBAGE_MAX_INTERVAL
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

static void gc_cycle_pop();

#if defined (PIKE_DEBUG) || defined (GC_MARK_DEBUG)
PMOD_EXPORT void *gc_found_in = NULL;
PMOD_EXPORT int gc_found_in_type = PIKE_T_UNKNOWN;
PMOD_EXPORT const char *gc_found_place = NULL;
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

/* If p* isn't NULL then p*_name will be written out next to the
 * matching frame in the stack, if any is found. */
static void describe_rec_stack (struct gc_rec_frame *p1, const char *p1_name,
				struct gc_rec_frame *p2, const char *p2_name,
				struct gc_rec_frame *p3, const char *p3_name)
{
  struct gc_rec_frame *l, *cp;
  size_t longest = 0;

  if (p1) longest = strlen (p1_name);
  if (p2) {size_t l = strlen (p2_name); if (l > longest) longest = l;}
  if (p3) {size_t l = strlen (p3_name); if (l > longest) longest = l;}
  longest++;

  /* Note: Stack is listed from top to bottom, but cycle piece lists
   * are lists from first to last, i.e. reverse order. */

  for (l = stack_top; l != &sentinel_frame; l = l->prev) {
    size_t c = 0;

    if (!l) {fputs ("  <broken prev link in rec stack>\n", stderr); break;}
    fprintf (stderr, "  %p", l);

    if (l == p1) {fprintf (stderr, " %s", p1_name); c += strlen (p1_name) + 1;}
    if (l == p2) {fprintf (stderr, " %s", p2_name); c += strlen (p2_name) + 1;}
    if (l == p3) {fprintf (stderr, " %s", p3_name); c += strlen (p3_name) + 1;}
    fprintf (stderr, ": %*s", c < longest ? (int) (longest - c) : 0, "");

    describe_rec_frame (l);
    fputc ('\n', stderr);

    for (cp = l->cycle_piece; cp; cp = cp->cycle_piece) {
      fprintf (stderr, "    %p", cp);

      c = 0;
      if (cp == p1) {fprintf (stderr, " %s", p1_name); c += strlen (p1_name)+1;}
      if (cp == p2) {fprintf (stderr, " %s", p2_name); c += strlen (p2_name)+1;}
      if (cp == p3) {fprintf (stderr, " %s", p3_name); c += strlen (p3_name)+1;}
      fprintf (stderr, ": %*s", c < longest ? (int) (longest - c) : 0, "");

      describe_rec_frame (cp);
      fputc ('\n', stderr);
    }
  }
}

#endif

#ifdef PIKE_DEBUG

int gc_in_cycle_check = 0;

static int gc_is_watching = 0;

int attempt_to_identify(const void *something, const void **inblock)
{
  size_t i;
  const struct array *a;
  const struct object *o;
  const struct program *p;
  const struct mapping *m;
  const struct multiset *mu;
  const struct pike_type *t;
  const struct callable *c;

  if (inblock) *inblock = 0;

  for (a = first_array; a; a = a->next) {
    if(a==(const struct array *)something) return T_ARRAY;
  }

  for(o=first_object;o;o=o->next) {
    if(o==(const struct object *)something)
      return T_OBJECT;
    if (o->storage && o->prog &&
	(char *) something >= o->storage &&
	(char *) something < o->storage + o->prog->storage_needed) {
      if (inblock) *inblock = (void *) o;
      return T_STORAGE;
    }
  }

  for(p=first_program;p;p=p->next)
    if(p==(const struct program *)something)
      return T_PROGRAM;

  for(m=first_mapping;m;m=m->next)
    if(m==(const struct mapping *)something)
      return T_MAPPING;
    else if (m->data == (const struct mapping_data *) something)
      return T_MAPPING_DATA;

  for(mu=first_multiset;mu;mu=mu->next)
    if(mu==(const struct multiset *)something)
      return T_MULTISET;
    else if (mu->msd == (const struct multiset_data *) something)
      return T_MULTISET_DATA;

  if(safe_debug_findstring((const struct pike_string *)something))
    return T_STRING;

  if (pike_type_hash)
    for (i = 0; i <= pike_type_hash_size; i++)
      for (t = pike_type_hash[i]; t; t = t->next)
        if (t == (const struct pike_type *) something)
	  return T_TYPE;

  for (c = first_callable; c; c = c->next)
    if (c == (const struct callable *) something)
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
  void *memblock=0, *descblock;
  const void *inblock = NULL;

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
      /* FALLTHRU */

    case T_MULTISET_DATA: {
      struct multiset_data *msd = (struct multiset_data *) descblock;
      union msnode *node = low_multiset_first (msd);
      struct svalue ind;
      for (; node; node = low_multiset_next (node)) {
	if (&node->i.ind == (struct svalue *) location) {
	  fprintf (stderr, "%*s  **In index ", indent, "");
	  safe_print_svalue (stderr, low_use_multiset_index (node, ind));
	  fputc ('\n', stderr);
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
      /* FALLTHRU */
    case T_MAPPING_DATA: {
      INT32 e;
      struct keypair *k;
      NEW_MAPPING_LOOP((struct mapping_data *) descblock)
	if (&k->ind == (struct svalue *) location) {
	  fprintf(stderr, "%*s  **In index ", indent, "");
	  safe_print_svalue (stderr, &k->ind);
	  fputc('\n', stderr);
	  break;
	}
	else if (&k->val == (struct svalue *) location) {
	  fprintf(stderr, "%*s  **In value with index ", indent, "");
	  safe_print_svalue (stderr, &k->ind);
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

#ifdef GC_STACK_DEBUG
static void describe_link_frame (struct link_frame *f)
{
  fprintf (stderr, "data=%p prev=%p checkfn=%p weak=%d",
	   f->data, f->prev, f->checkfn, f->weak);
}
#endif

static void describe_marker(struct marker *m)
{
  if (m) {
    fprintf(stderr, "marker at %p: flags=0x%05lx refs=%d weak=%d "
	    "xrefs=%d saved=%d frame=%p",
	    m, (long) m->gc_flags, m->gc_refs, m->weak_refs,
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

static void debug_gc_fatal_va (void *DEBUGUSED(a), int DEBUGUSED(type), int DEBUGUSED(flags),
			       const char *fmt, va_list args)
{
  int orig_gc_pass = Pike_in_gc;

  (void) vfprintf(stderr, fmt, args);

#ifdef PIKE_DEBUG
  if (a) {
    const void *inblock = NULL;
    /* Temporarily jumping out of gc to avoid being caught in debug
     * checks in describe(). */
    Pike_in_gc = 0;
    if (type == PIKE_T_UNKNOWN)
      type = attempt_to_identify (a, &inblock);
    describe_something (a, type, 0, 0, 0, inblock);
    if (flags & 1) locate_references(a);
    Pike_in_gc = orig_gc_pass;
  }

  if (flags & 2)
    fatal_after_gc = "Fatal in garbage collector.\n";
  else
#endif
  {
    d_flag = 0; /* The instruction backlog is never of any use here. */
    debug_fatal (NULL);
  }
}

void debug_gc_fatal (void *a, int flags, const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  debug_gc_fatal_va (a, PIKE_T_UNKNOWN, flags, fmt, args);
  va_end (args);
}

void debug_gc_fatal_2 (void *a, int type, int flags, const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  debug_gc_fatal_va (a, type, flags, fmt, args);
  va_end (args);
}

#ifdef PIKE_DEBUG

static void dloc_gc_fatal (const char *file, INT_TYPE line,
			   void *a, int flags, const char *fmt, ...)
{
  va_list args;
  fprintf (stderr, "%s:%ld: GC fatal:\n", file, (long)line);
  va_start (args, fmt);
  debug_gc_fatal_va (a, PIKE_T_UNKNOWN, flags, fmt, args);
  va_end (args);
}

static void rec_stack_fatal (struct gc_rec_frame *DEBUGUSED(err),
                             const char *DEBUGUSED(err_name),
                             struct gc_rec_frame *DEBUGUSED(p1),
                             const char *DEBUGUSED(p1n),
                             struct gc_rec_frame *DEBUGUSED(p2),
                             const char *DEBUGUSED(p2n),
			     const char *file, INT_TYPE line,
			     const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  fprintf (stderr, msg_fatal_error, file, line);
  (void) vfprintf (stderr, fmt, args);
#if defined (PIKE_DEBUG) || defined (GC_CYCLE_DEBUG)
  fputs ("Recursion stack:\n", stderr);
  describe_rec_stack (err, err_name, p1, p1n, p2, p2n);
  if (err) {
    fprintf (stderr, "Describing frame %p: ", err);
    describe_rec_frame (err);
    fputc ('\n', stderr);
  }
#endif
  d_flag = 0; /* The instruction backlog is never of any use here. */
  debug_fatal (NULL);
  va_end (args);
}

static void gdb_gc_stop_here(void *UNUSED(a), int weak)
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

void low_describe_something(const void *a,
			    int t,
			    int indent,
			    int depth,
			    int flags,
                            const void *inblock)
{
  struct program *p=(struct program *)a;
  struct marker *m;

  if(depth<0) return;

  if ((m = find_marker(a))) {
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
      /* FALLTHRU */

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

      {
	struct program_state *ps;
	for (ps = Pike_compiler; ps; ps = ps->previous)
	  if (ps->fake_object == (struct object *) a) {
	    fprintf (stderr, "%*s**The object is a fake for new program %p "
		     "in compiler program state %p.\n",
		     indent, "", ps->new_program, ps);
	    break;
	  }
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

      if (p == pike_trampoline_program && ((struct object *) a)->refs > 0) {
	/* Special hack to get something useful out of trampolines.
	 * Ought to have an event hook for this sort of thing. */
	struct pike_trampoline *t =
	  (struct pike_trampoline *) ((struct object *) a)->storage;
	struct object *o = t->frame->current_object;
	struct program *p = o->prog;
	struct identifier *id;
	INT_TYPE line;
	struct pike_string *file;

	fprintf (stderr, "%*s**The object is a trampoline.\n", indent, "");

	if (!p) {
	  fprintf (stderr, "%*s**The trampoline function's object "
		   "is destructed.\n", indent, "");
	  p = id_to_program (o->program_id);
	}

	if (p) {
	  id = ID_FROM_INT (p, t->func);
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
	} else {
	  fprintf(stderr, "%*s**Function #%d\n", indent, "", t->func);
	}
	fprintf(stderr, "%*s**Context frame: %p (%d refs)\n",
		indent, "", t->frame, t->frame->refs);
	if (p) {
	  id = ID_FROM_INT(p, t->frame->fun);
	  if (IDENTIFIER_IS_PIKE_FUNCTION(id->identifier_flags) &&
	      id->func.offset >= 0 &&
	      (file = get_line(p->program + id->func.offset, p, &line))) {
	    fprintf(stderr, "%*s**Lexical parent function %s at %s:%ld\n",
		    indent, "", id->name->str, file->str, (long) line);
	    free_string(file);
	  }
	  else
	    fprintf(stderr, "%*s**Lexical parent function %s at unknown location.\n",
		    indent, "", id->name->str);
	} else {
	  fprintf(stderr, "%*s**Lexical parent function #%d\n",
		  indent, "", t->frame->fun);
	}
	if (depth && o->prog) {
	  fprintf (stderr, "%*s**Describing function's object:\n",
		   indent, "");
	  describe_something (o, T_OBJECT, indent + 2, depth - 1,
			      (flags & DESCRIBE_SHORT) & ~DESCRIBE_MEM,
			      0);
	}
      }

      else if (((struct object *) a)->refs > 0 && p) {
	size_t inh_idx, var_idx, var_count = 0;

	if (p) {
	  fprintf (stderr, "%*s**Object variables:\n", indent, "");

	  for (inh_idx = 0; inh_idx < p->num_inherits; inh_idx++) {
	    struct inherit *inh = p->inherits + inh_idx;
	    struct program *p2 = inh->prog;

	    if (inh->inherit_level) {
	      if (inh->name) {
		fprintf (stderr, "%*s**%*s=== In inherit ",
			 indent, "", inh->inherit_level + 1, "");
		safe_print_short_svalue (stderr, (union anything *) &inh->name,
					 T_STRING);
		fprintf (stderr, ", program %d:\n", inh->prog->id);
	      }
	      else
		fprintf (stderr,
			 "%*s**%*s=== In nameless inherit, program %d:\n",
			 indent, "", inh->inherit_level + 1, "", inh->prog->id);
	    }

	    for (var_idx = 0; var_idx < p2->num_variable_index; var_idx++) {
	      struct identifier *id =
		p2->identifiers + p2->variable_index[var_idx];
	      if (id->run_time_type != PIKE_T_FREE &&
		  id->run_time_type != PIKE_T_GET_SET) {
		void *ptr;

		fprintf (stderr, "%*s**%*srtt: %-8s  name: ",
			 indent, "", inh->inherit_level + 1, "",
			 get_name_of_type (id->run_time_type));

		if (id->name->size_shift)
		  safe_print_short_svalue (stderr, (union anything *) &id->name,
					   T_STRING);
		else
		  fprintf (stderr, "%-20s", id->name->str);

		fprintf (stderr, "  off: %4"PRINTPTRDIFFT"d  value: ",
			 inh->storage_offset + id->func.offset);

		ptr = PIKE_OBJ_STORAGE ((struct object *) a) +
		  inh->storage_offset + id->func.offset;
		if (id->run_time_type == T_MIXED)
		  safe_print_svalue_compact (stderr, (struct svalue *) ptr);
		else
		  safe_print_short_svalue_compact (stderr,
						   (union anything *) ptr,
						   id->run_time_type);

		fputc ('\n', stderr);
		var_count++;
	      }
	    }
	  }

	  if (!var_count)
	    fprintf (stderr, "%*s** (none)\n", indent, "");
	}

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
      INT_TYPE line;
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
	      safe_print_short_svalue (stderr, (union anything *) &inh->name,
				       T_STRING);
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
	      safe_print_short_svalue (stderr, (union anything *) &inh->name,
				       T_STRING);
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

#if 0
	  /* Can be illuminating to see these too.. */
	  if (id_ref->id_flags & ID_HIDDEN ||
	      (id_ref->id_flags & (ID_INHERITED|ID_PRIVATE)) ==
	      (ID_INHERITED|ID_PRIVATE)) continue;
#endif

	  id_inh = INHERIT_FROM_PTR (p, id_ref);
	  id = id_inh->prog->identifiers + id_ref->identifier_offset;

	  if (IDENTIFIER_IS_ALIAS (id->identifier_flags)) type = "alias";
	  else if (IDENTIFIER_IS_PIKE_FUNCTION (id->identifier_flags)) type = "fun";
	  else if (IDENTIFIER_IS_FUNCTION (id->identifier_flags)) type = "cfun";
	  else if (IDENTIFIER_IS_CONSTANT (id->identifier_flags)) type = "const";
	  else if (IDENTIFIER_IS_VARIABLE (id->identifier_flags)) type = "var";
	  else type = "???";

	  prot[0] = prot[1] = 0;
	  if (id_ref->id_flags & ID_PRIVATE) {
	    strcat (prot, ",pri");
	    if (!(id_ref->id_flags & ID_PROTECTED)) strcat (prot, ",!pro");
	  }
	  else
	    if (id_ref->id_flags & ID_PROTECTED) strcat (prot, ",pro");
	  if (id_ref->id_flags & ID_FINAL)     strcat (prot, ",fin");
	  if (id_ref->id_flags & ID_PUBLIC)    strcat (prot, ",pub");
	  if (id_ref->id_flags & ID_INLINE)    strcat (prot, ",inl");
	  if (id_ref->id_flags & ID_OPTIONAL)  strcat (prot, ",opt");
	  if (id_ref->id_flags & ID_HIDDEN)    strcat (prot, ",hid");
	  if (id_ref->id_flags & ID_INHERITED) strcat (prot, ",inh");
	  if (id_ref->id_flags & ID_EXTERN)    strcat (prot, ",ext");
	  if (id_ref->id_flags & ID_VARIANT)   strcat (prot, ",var");
	  if (id_ref->id_flags & ID_USED)      strcat (prot, ",use");

	  sprintf (descr, "%s: %s", type, prot + 1);
	  fprintf (stderr, "%*s**%*s%-3"PRINTPTRDIFFT"d %-18s name: ",
		   indent, "", id_inh->inherit_level + 1, "", id_idx, descr);

	  if (id->name->size_shift)
	    safe_print_short_svalue (stderr, (union anything *) &id->name,
				     T_STRING);
	  else
	    fprintf (stderr, "%-20s", id->name->str);

	  if (IDENTIFIER_IS_ALIAS(id->identifier_flags)) {
	    fprintf(stderr, "  depth: %d  id: %d",
		    id->func.ext_ref.depth, id->func.ext_ref.id);
	  } else if (id->identifier_flags & IDENTIFIER_C_FUNCTION)
	    fprintf (stderr, "  addr: %p", id->func.c_fun);
	  else if (IDENTIFIER_IS_VARIABLE (id->identifier_flags)) {
	    if (id->run_time_type == PIKE_T_GET_SET) {
	      fprintf (stderr, "  ");
	      if (id->func.gs_info.getter >= 0) {
		fprintf (stderr, "getter: %d(%d)",
			 id->func.gs_info.getter,
			 id->func.gs_info.getter + id_inh->identifier_level);
		if (id->func.gs_info.setter >= 0)
		  fprintf (stderr, ", ");
	      }
	      if (id->func.gs_info.setter >= 0)
		fprintf (stderr, "setter: %d(%d)",
			 id->func.gs_info.setter,
			 id->func.gs_info.setter + id_inh->identifier_level);
	    }
	    else if (id->run_time_type == PIKE_T_FREE)
	      fprintf (stderr, "  extern");
	    else
	      fprintf (stderr, "  rtt: %s  off: %"PRINTPTRDIFFT"d",
		       get_name_of_type (id->run_time_type), id->func.offset);
	  }
	  else if (IDENTIFIER_IS_PIKE_FUNCTION (id->identifier_flags))
	    fprintf (stderr, "  pc: %"PRINTPTRDIFFT"d", id->func.offset);
	  else if (IDENTIFIER_IS_CONSTANT (id->identifier_flags)) {
	    if (id->func.const_info.offset != -1) {
	      fputs ("  value: ", stderr);
	      safe_print_svalue_compact (
	        stderr, &id_inh->prog->constants[id->func.const_info.offset].sval);
	    } else {
	      fputs ("  placeholder constant", stderr);
	    }
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
	    INT_TYPE line;
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
      fprintf(stderr, "%*s**Cannot describe block of type %s (%d)\n",
	      indent, "", get_name_of_type (t), t);
  }
}

void describe_something(const void *a, int t, int indent, int depth, int flags,
                        const void *inblock)
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
#if defined(USE_VALGRIND) && defined(VALGRIND_MONITOR_COMMAND)
  if (PIKE_MEM_CHECKER()) {
      char buf[40];
      snprintf(buf, 40, "v.info location %p", a);
      fprintf(stderr, "**Valgrind info:\n");
      VALGRIND_MONITOR_COMMAND(buf);
  }
#endif


  fprintf(stderr,"%*s*******************\n",indent,"");
  d_flag=tmp;
}

PMOD_EXPORT void describe(const void *x)
{
  const void *inblock;
  int type = attempt_to_identify(x, &inblock);
  describe_something(x, type, 0, 0, 0, inblock);
}

PMOD_EXPORT void debug_describe_svalue(struct svalue *s)
{
  fprintf(stderr,"Svalue at %p is:\n",s);
  switch(TYPEOF(*s))
  {
    case T_INT:
      fprintf(stderr,"    %"PRINTPIKEINT"d (subtype %d)\n",s->u.integer,
	      SUBTYPEOF(*s));
      return;

    case T_FLOAT:
      fprintf(stderr,"    %"PRINTPIKEFLOAT"f\n",s->u.float_number);
      return;

    case T_FUNCTION:
      if(SUBTYPEOF(*s) == FUNCTION_BUILTIN)
      {
	fprintf(stderr,"    Builtin function: %s\n",s->u.efun->name->str);
      }else{
	if(!s->u.object->prog)
	{
	  struct program *p=id_to_program(s->u.object->program_id);
	  if(p)
	  {
	    fprintf(stderr,"    Function in destructed object: %s\n",
		    ID_FROM_INT(p, SUBTYPEOF(*s))->name->str);
	  }else{
	    fprintf(stderr,"    Function in destructed object.\n");
	  }
	}else{
	  fprintf(stderr,"    Function name: %s\n",
		  ID_FROM_INT(s->u.object->prog, SUBTYPEOF(*s))->name->str);
	}
      }
  }
  describe_something(s->u.refs, TYPEOF(*s), 0, 1, 0, 0);
}

PMOD_EXPORT void gc_watch(void *a)
{
  struct marker *m;
  m = get_marker(a);
  if (!(m->gc_flags & GC_WATCHED)) {
    m->gc_flags |= GC_WATCHED;
    fprintf(stderr, "## Watching thing %p.\n", a);
    gc_is_watching++;
  }
  else
    fprintf(stderr, "## Already watching thing %p.\n", a);
}

static void gc_watched_found (struct marker *m, const char *found_in)
{
  fprintf(stderr, "## Watched thing found in "
	  "%s in pass %d.\n", found_in, Pike_in_gc);
  describe_marker (m);
}

#endif /* PIKE_DEBUG */

#ifndef GC_MARK_DEBUG
struct pike_queue gc_mark_queue;
#define CHECK_MARK_QUEUE_EMPTY() assert (!gc_mark_queue.first)
#else  /* GC_MARK_DEBUG */

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

#define CHECK_MARK_QUEUE_EMPTY() assert (!gc_mark_first)

void gc_mark_run_queue(void)
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

void gc_mark_discard_queue(void)
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
  if (Pike_in_gc != GC_PASS_MARK && Pike_in_gc != GC_PASS_ZAP_WEAK)
    gc_fatal (data, 0, "gc_mark_enqueue() called in invalid gc pass.\n");
  if (gc_found_in_type == PIKE_T_UNKNOWN || !gc_found_in)
    gc_fatal (data, 0, "gc_mark_enqueue() called outside GC_ENTER.\n");
  {
    struct marker *m;
    if (gc_is_watching && (m = find_marker(data)) && m->gc_flags & GC_WATCHED) {
      /* This is useful to set breakpoints on. */
      gc_watched_found (m, "gc_mark_enqueue()");
    }
  }
#endif

  b=gc_mark_last;
  if(!b || b->used >= GC_QUEUE_ENTRIES)
  {
    b = (struct gc_queue_block *) malloc (sizeof (struct gc_queue_block));
    if (!b) Pike_fatal ("Out of memory in gc.\n");
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
  if (!a) Pike_fatal("Got null pointer.\n");
  if (gc_is_watching && (m = find_marker(a)) && m->gc_flags & GC_WATCHED) {
    /* This is useful to set breakpoints on. */
    gc_watched_found (m, "gc_touch()");
  }
#endif

  switch (Pike_in_gc) {
    case GC_PASS_PRETOUCH:
      m = find_marker(a);
#ifdef PIKE_DEBUG
      if (
#ifdef DO_PIKE_CLEANUP
	  !gc_keep_markers &&
#endif
          m && !(m->gc_flags & (GC_PRETOUCHED|GC_WATCHED )))
	gc_fatal(a, 1, "Thing got an existing but untouched marker.\n");
#endif /* PIKE_DEBUG */
      m = get_marker(a);
      m->gc_flags |= GC_PRETOUCHED;
#ifdef PIKE_DEBUG
      m->saved_refs = *(INT32 *) a;
#endif
      break;

    case GC_PASS_POSTTOUCH: {
#ifdef PIKE_DEBUG
      int extra_ref;
#endif
      m = find_marker(a);
#ifdef PIKE_DEBUG
      if (!m)
	gc_fatal(a, 1, "Found a thing without marker.\n");
      else if (!(m->gc_flags & GC_PRETOUCHED))
	gc_fatal(a, 1, "Thing got an existing but untouched marker.\n");
      if (gc_destruct_everything && (m->gc_flags & GC_MARKED))
	gc_fatal (a, 1, "Thing got marked in gc_destruct_everything mode.\n");
      extra_ref = (m->gc_flags & GC_GOT_EXTRA_REF) == GC_GOT_EXTRA_REF;
      if (m->saved_refs + extra_ref < *(INT32 *) a)
	if (m->gc_flags & GC_WEAK_FREED)
	  gc_fatal(a, 1, "Something failed to remove weak reference(s) to thing, "
		   "or it has gotten more references since gc start.\n");
	else
	  gc_fatal(a, 1, "Thing has gotten more references since gc start.\n");
      else
	if (m->weak_refs > m->saved_refs)
	  gc_fatal(a, 0, "A thing got more weak references than references.\n");
#endif
      m->gc_flags |= GC_POSTTOUCHED;
      break;
    }

    default:
      Pike_fatal("debug_gc_touch() used in invalid gc pass.\n");
  }
}

#ifdef PIKE_DEBUG

static inline struct marker *gc_check_debug(void *a, int weak)
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
  if (m->gc_refs + m->xrefs >= *(INT32 *) a)
    /* m->gc_refs will be incremented by the caller. */
    gc_fatal (a, 1, "Thing is getting more internal refs (%d + %d) "
	      "than refs (%d).\n"
	      "(Could be an extra free somewhere, or "
	      "a pointer might have been checked more than once.)\n",
	      m->gc_refs, m->xrefs, *(INT32 *) a);
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
  if (gc_is_watching && (m = find_marker(a)) && m->gc_flags & GC_WATCHED) {
    /* This is useful to set breakpoints on. */
    gc_watched_found (m, "gc_check()");
  }
  if (!(m = gc_check_debug(a, 0))) return 0;
#else
  m = get_marker(a);
#endif

  ret=m->gc_refs++;
  if (m->gc_refs == *(INT32 *) a)
    m->gc_flags |= GC_NOT_REFERENCED;
  return ret;
}

PMOD_EXPORT INT32 real_gc_check_weak(void *a)
{
  struct marker *m;
  INT32 ret;

#ifdef PIKE_DEBUG
  if (gc_found_in_type == PIKE_T_UNKNOWN || !gc_found_in)
    gc_fatal (a, 0, "gc_check_weak() called outside GC_ENTER.\n");
  if (gc_is_watching && (m = find_marker(a)) && m->gc_flags & GC_WATCHED) {
    /* This is useful to set breakpoints on. */
    gc_watched_found (m, "gc_check_weak()");
  }
  if (!(m = gc_check_debug(a, 1))) return 0;
  if (m->weak_refs < 0)
    gc_fatal(a, 1, "Thing has already reached threshold for weak free.\n");
  if (m->weak_refs >= *(INT32 *) a)
    gc_fatal(a, 1, "Thing has gotten more weak refs than refs.\n");
  if (m->weak_refs > m->gc_refs + 1)
    gc_fatal(a, 1, "Thing has gotten more weak refs than internal refs.\n");
#else
  m = get_marker(a);
#endif

  m->weak_refs++;
  gc_ext_weak_refs++;
  if (m->weak_refs == *(INT32 *) a)
    m->weak_refs = -1;

  ret=m->gc_refs++;
  if (m->gc_refs == *(INT32 *) a)
    m->gc_flags |= GC_NOT_REFERENCED;
  return ret;
}

static void cleanup_markers (void)
{
}

void exit_gc(void)
{
  if (gc_evaluator_callback) {
    remove_callback(gc_evaluator_callback);
    gc_evaluator_callback = NULL;
  }
  if (!gc_keep_markers)
    cleanup_markers();

  ba_free_all(&gc_rec_frame_allocator);
  ba_free_all(&ba_mixed_frame_allocator);

#ifdef PIKE_DEBUG
  if (gc_is_watching) {
    fprintf(stderr, "## Exiting gc and resetting watches for %d things.\n",
	    gc_is_watching);
    gc_is_watching = 0;
  }
#endif
}

#ifdef PIKE_DEBUG

PMOD_EXPORT void gc_check_zapped (void *a, TYPE_T type, const char *file, INT_TYPE line)
{
  struct marker *m = find_marker (a);
  if (m && (m->gc_flags & GC_CLEANUP_LEAKED))
    fprintf (stderr, "Free of leaked %s %p from %s:%ld, %d refs remaining\n",
	     get_name_of_type (type), a, file, (long)line, *(INT32 *)a - 1);
}

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

PMOD_EXPORT void locate_references(void *a)
{
  int tmp, orig_in_gc = Pike_in_gc;
  const char *orig_gc_found_place = gc_found_place;

  ADD_GC_CALL_FRAME();

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
    gc_check_all_types();
  } GC_LEAVE;

  GC_ASSERT_ZAPPED_CALL_FRAME();

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
	  "found %"PRINTSIZET"u refs.\n", a, found_ref_count);

  Pike_in_gc = orig_in_gc;
  gc_found_place = orig_gc_found_place;

  GC_POP_CALL_FRAME();

  d_flag=tmp;
}

void debug_gc_add_extra_ref(void *a)
{
  struct marker *m;

  if (gc_is_watching && (m = find_marker(a)) && m->gc_flags & GC_WATCHED) {
    /* This is useful to set breakpoints on. */
    gc_watched_found (m, "gc_add_extra_ref()");
  }

  if (gc_debug) {
    m = find_marker(a);
    if ((!m || !(m->gc_flags & GC_PRETOUCHED)) &&
	!safe_debug_findstring((struct pike_string *) a))
      gc_fatal(a, 0, "Doing gc_add_extra_ref() on invalid object.\n");
    if (!m) m = get_marker(a);
  }
  else m = get_marker(a);

  if (m->gc_flags & GC_GOT_EXTRA_REF)
    gc_fatal(a, 0, "Thing already got an extra gc ref.\n");
  m->gc_flags |= GC_GOT_EXTRA_REF;
  gc_extra_refs++;
  add_ref( (struct ref_dummy *)a);
}

void debug_gc_free_extra_ref(void *a)
{
  struct marker *m;

  if (gc_is_watching && (m = find_marker(a)) && m->gc_flags & GC_WATCHED) {
    /* This is useful to set breakpoints on. */
    gc_watched_found (m, "gc_free_extra_ref()");
  }

  if (gc_debug) {
    m = find_marker(a);
    if ((!m || !(m->gc_flags & GC_PRETOUCHED)) &&
	!safe_debug_findstring((struct pike_string *) a))
      gc_fatal(a, 0, "Doing gc_add_extra_ref() on invalid object.\n");
    if (!m) m = get_marker(a);
  }
  else m = get_marker(a);

  if (!(m->gc_flags & GC_GOT_EXTRA_REF))
    gc_fatal(a, 0, "Thing haven't got an extra gc ref.\n");
  m->gc_flags &= ~GC_GOT_EXTRA_REF;
  gc_extra_refs--;
}


int debug_gc_is_referenced(void *a)
{
  struct marker *m;

  if (gc_is_watching && (m = find_marker(a)) && m->gc_flags & GC_WATCHED) {
    /* This is useful to set breakpoints on. */
    gc_watched_found (m, "gc_is_referenced()");
  }

  if (!a) Pike_fatal("Got null pointer.\n");
  if (Pike_in_gc != GC_PASS_MARK)
    Pike_fatal("gc_is_referenced() called in invalid gc pass.\n");

  if (gc_debug) {
    m = find_marker(a);
    if ((!m || !(m->gc_flags & GC_PRETOUCHED)) &&
	!safe_debug_findstring((struct pike_string *) a))
      gc_fatal(a, 0, "Doing gc_is_referenced() on invalid object.\n");
    if (!m) m = get_marker(a);
  }
  else m = get_marker(a);

  if (m->gc_flags & GC_IS_REFERENCED)
    gc_fatal(a, 0, "gc_is_referenced() called twice for thing.\n");
  m->gc_flags |= GC_IS_REFERENCED;

  return !(m->gc_flags & GC_NOT_REFERENCED);
}

int gc_mark_external (void *a, const char *place)
{
  struct marker *m;

  if (gc_is_watching && (m = find_marker(a)) && m->gc_flags & GC_WATCHED) {
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
  m->gc_flags|=GC_XREFERENCED;
  if(Pike_in_gc == GC_PASS_CHECK &&
     (m->gc_refs + m->xrefs > *(INT32 *)a ||
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
				   struct gc_rec_frame *p1, const char *p1n,
				   struct gc_rec_frame *p2, const char *p2n,
				   const char *file, INT_TYPE line)
{
  /* To allow this function to be used after a stack rotation but
   * before cycle_id markup, there are no checks here for cycle_id
   * consistency wrt other frames on the rec stack. */
  LOW_CHECK_REC_FRAME (f, file, line);
  if (f->rf_flags & (GC_ON_CYCLE_PIECE_LIST|GC_ON_KILL_LIST))
    rec_stack_fatal (f, "err", p1, p1n, p2, p2n, file, line,
		     "Frame %p is not on the rec stack (according to flags).\n",
		     f);
  if (!f->prev)
    rec_stack_fatal (f, "err", p1, p1n, p2, p2n, file, line,
		     "Prev pointer not set for rec stack frame %p.\n", f);
  if (f->prev->next != f)
    rec_stack_fatal (f, "err", p1, p1n, p2, p2n, file, line,
		     "Rec stack pointers are inconsistent before %p.\n", f);
  if (f->cycle_id &&
      f->cycle_id->rf_flags & (GC_ON_CYCLE_PIECE_LIST|GC_ON_KILL_LIST))
    /* p2 and p2n gets lost here. No bother. */
    rec_stack_fatal (f->cycle_id, "cycle id", f, "err", p1, p1n, file, line,
		     "Cycle id frame %p for %p not on the rec stack "
		     "(according to flags).\n", f->cycle_id, f);
  if ((f->rf_flags & GC_MARK_LIVE) && f != stack_top)
    rec_stack_fatal (f, "err", p1, p1n, p2, p2n, file, line,
		     "GC_MARK_LIVE frame %p found that "
		     "isn't on the stack top.\n", f);
  if ((f->rf_flags & GC_PREV_STRONG) &&
      (f->rf_flags & (GC_PREV_WEAK|GC_PREV_BROKEN)))
    rec_stack_fatal (f, "err", p1, p1n, p2, p2n, file, line,
		     "GC_PREV_STRONG set together with "
		     "GC_PREV_WEAK or GC_PREV_BROKEN in %p.\n", f);
  if (f->cycle_piece &&
      (!f->cycle_piece->u.last_cycle_piece ||
       f->cycle_piece->u.last_cycle_piece->cycle_piece))
    rec_stack_fatal (f, "err", p1, p1n, p2, p2n, file, line,
		     "Bogus last_cycle_piece %p is %p "
		     "in cycle piece top %p in %p.\n",
		     f->cycle_piece->u.last_cycle_piece,
		     f->cycle_piece->u.last_cycle_piece ?
		     f->cycle_piece->u.last_cycle_piece->cycle_piece : NULL,
		     f->cycle_piece, f);
}
#define CHECK_REC_STACK_FRAME(f)					\
  do check_rec_stack_frame ((f), NULL, NULL, NULL, NULL, __FILE__, __LINE__); \
  while (0)

static void check_cycle_piece_frame (struct gc_rec_frame *f,
				     const char *file, INT_TYPE line)
{
  LOW_CHECK_REC_FRAME (f, file, line);
  if ((f->rf_flags & (GC_ON_CYCLE_PIECE_LIST|GC_ON_KILL_LIST)) !=
      GC_ON_CYCLE_PIECE_LIST)
    dloc_gc_fatal (file, line, f->data, 0,
		   "Frame is not on a cycle piece list "
		   "(according to flags).\n");
  if (f->prev)
    dloc_gc_fatal (file, line, f->data, 0,
		   "Prev pointer set for frame on cycle piece list.\n");
}
#define CHECK_CYCLE_PIECE_FRAME(f)					\
  do check_cycle_piece_frame ((f), __FILE__, __LINE__); while (0)

static void check_kill_list_frame (struct gc_rec_frame *f,
				   const char *file, INT_TYPE line)
{
  LOW_CHECK_REC_FRAME (f, file, line);
  if ((f->rf_flags & (GC_ON_CYCLE_PIECE_LIST|GC_ON_KILL_LIST)) !=
      GC_ON_KILL_LIST)
    dloc_gc_fatal (file, line, f->data, 0,
		   "Frame is not on kill list (according to flags).\n");
  if (f->prev)
    dloc_gc_fatal (file, line, f->data, 0,
		   "Prev pointer set for frame on kill list.\n");
}
#define CHECK_KILL_LIST_FRAME(f)					\
  do check_kill_list_frame ((f), __FILE__, __LINE__); while (0)

static void check_rec_stack (struct gc_rec_frame *p1, const char *p1n,
			     struct gc_rec_frame *p2, const char *p2n,
			     const char *file, int line)
{
  /* This debug check is disabled during the final cleanup since this
   * is O(n^2) on the stack size, and the stack gets a lot larger then. */
  if (gc_debug && !gc_destruct_everything) {
    struct gc_rec_frame *l, *last_cycle_id = NULL;
    for (l = &sentinel_frame; l != stack_top;) {
      l = l->next;
      check_rec_stack_frame (l, p1, p1n, p2, p2n, file, line);
      if (l->cycle_id == l)
	last_cycle_id = l;
      else if (l->cycle_id != last_cycle_id)
	rec_stack_fatal (l, "err", p1, p1n, p2, p2n, file, line,
			 "Unexpected cycle id for frame %p.\n", l);
      else if (l->rf_flags & GC_PREV_WEAK)
	rec_stack_fatal (l, "err", p1, p1n, p2, p2n, file, line,
			 "Unexpected weak ref before %p inside a cycle.\n", l);

      if (l->rf_flags & GC_IS_VALID_CP_CYCLE_ID)
	rec_stack_fatal (l, "err", p1, p1n, p2, p2n, file, line,
			 "Frame %p got stray "
			 "GC_IS_VALID_CP_CYCLE_ID flag.\n", l);
      if (l->cycle_piece) {
	struct gc_rec_frame *cp = l->cycle_piece;
	l->rf_flags |= GC_IS_VALID_CP_CYCLE_ID;

	while (1) {
	  if (!cp->cycle_id ||
	      !(cp->cycle_id->rf_flags & GC_IS_VALID_CP_CYCLE_ID))
	    rec_stack_fatal (cp, "err", p1, p1n, p2, p2n, file, line,
			     "Unexpected cycle id for frame %p "
			     "on cycle piece list.\n", cp);
	  if (cp->rf_flags & GC_IS_VALID_CP_CYCLE_ID)
	    rec_stack_fatal (cp, "err", p1, p1n, p2, p2n, file, line,
			     "Frame %p got stray "
			     "GC_IS_VALID_CP_CYCLE_ID flag.\n", cp);
	  cp->rf_flags |= GC_IS_VALID_CP_CYCLE_ID;
	  check_cycle_piece_frame (cp, file, line);
	  if (!cp->cycle_piece) break;
	  cp = cp->cycle_piece;
	}

	if (l->cycle_piece->u.last_cycle_piece != cp)
	  rec_stack_fatal (l->cycle_piece, "err", p1, p1n, p2, p2n, file, line,
			   "last_cycle_piece is wrong for frame %p, "
			   "expected %p.\n", l->cycle_piece, cp);

	l->rf_flags &= ~GC_IS_VALID_CP_CYCLE_ID;
	cp = l->cycle_piece;
	do {
	  cp->rf_flags &= ~GC_IS_VALID_CP_CYCLE_ID;
	  cp = cp->cycle_piece;
	} while (cp);
      }
    }
  }
}
#define CHECK_REC_STACK(p1, p1n, p2, p2n)				\
  do check_rec_stack ((p1), (p1n), (p2), (p2n), __FILE__, __LINE__); while (0)

#else  /* !PIKE_DEBUG */
#define CHECK_REC_STACK_FRAME(f) do {} while (0)
#define CHECK_CYCLE_PIECE_FRAME(f) do {} while (0)
#define CHECK_KILL_LIST_FRAME(f) do {} while (0)
#define CHECK_REC_STACK(p1, p1n, p2, p2n) do {} while (0)
#endif	/* !PIKE_DEBUG */

int gc_do_weak_free(void *a)
{
  struct marker *m;

#ifdef PIKE_DEBUG
  if (gc_is_watching && (m = find_marker(a)) && m->gc_flags & GC_WATCHED) {
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

  if (m->weak_refs > m->gc_refs)
    gc_fatal(a, 0, "More weak references than internal references.\n");
#else
  m = get_marker(a);
#endif

  if (Pike_in_gc != GC_PASS_ZAP_WEAK) {
    if (m->weak_refs < 0)
      goto should_free;
  }
  else
    if (!(m->gc_flags & GC_MARKED)) {
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
  m->gc_flags |= GC_WEAK_FREED;
#endif

  if (*(INT32 *) a == 1) {
    /* Make sure the thing doesn't run out of refs, since we can't
     * handle cascading frees now. We'll do it in the free pass
     * instead. */
    gc_add_extra_ref(a);
    m->gc_flags |= GC_GOT_DEAD_REF;
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
  if (gc_is_watching && (m = find_marker(a)) && m->gc_flags & GC_WATCHED) {
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

  if (m->gc_flags & GC_MARKED) {
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
  m->gc_flags |= GC_GOT_DEAD_REF;
}

int real_gc_mark(void *a DO_IF_DEBUG (COMMA int type))
{
  struct marker *m;

#ifdef PIKE_DEBUG
  if (Pike_in_gc == GC_PASS_ZAP_WEAK && !find_marker (a))
    gc_fatal_2 (a, type, 0, "gc_mark() called for for thing without marker "
		"in zap weak pass.\n");
#endif

  m = get_marker (a);

  /* Note: m->gc_refs and m->xrefs are useless already here due to how
   * gc_free_(short_)svalue works. */

#ifdef PIKE_DEBUG
  if (gc_is_watching && m && m->gc_flags & GC_WATCHED) {
    /* This is useful to set breakpoints on. */
    gc_watched_found (m, "gc_mark()");
  }
  if (!a) Pike_fatal("Got null pointer.\n");
  if (Pike_in_gc != GC_PASS_MARK && Pike_in_gc != GC_PASS_ZAP_WEAK)
    Pike_fatal("GC mark attempted in invalid pass.\n");
  if (!*(INT32 *) a)
    gc_fatal_2 (a, type, 1, "Marked a thing without refs.\n");
  if (m->weak_refs < 0)
    gc_fatal_2 (a, type, 1, "Marked a thing scheduled for weak free.\n");
#endif

  if (Pike_in_gc == GC_PASS_ZAP_WEAK) {
    /* Things are visited in the zap weak pass through the mark
     * functions to free refs to internal things that only got weak
     * external references. That happens only when a thing also have
     * internal cyclic nonweak refs. */
#ifdef PIKE_DEBUG
    if (!(m->gc_flags & GC_MARKED))
      gc_fatal_2 (a, type, 0, "gc_mark() called for thing in zap weak pass "
		  "that wasn't marked before.\n");
#endif
    if (m->gc_flags & GC_FREE_VISITED) {
      debug_malloc_touch (a);
      return 0;
    }
    else {
      debug_malloc_touch (a);
      m->gc_flags |= GC_FREE_VISITED;
      return 1;
    }
  }

  else if (m->gc_flags & GC_MARKED) {
    debug_malloc_touch (a);
#ifdef PIKE_DEBUG
    if (m->weak_refs != 0)
      gc_fatal_2 (a, type, 0, "weak_refs changed in marker "
		  "already visited by gc_mark().\n");
#endif
    return 0;
  }

  else {
    debug_malloc_touch (a);
    if (m->weak_refs) {
      gc_ext_weak_refs -= m->weak_refs;
      m->weak_refs = 0;
    }
    m->gc_flags = (m->gc_flags & ~GC_NOT_REFERENCED) | GC_MARKED;
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
    if (gc_is_watching && (m = find_marker(data)) && m->gc_flags & GC_WATCHED) {
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
  struct gc_rec_frame *r =
    (struct gc_rec_frame*)ba_alloc(&gc_rec_frame_allocator);
  if (++rec_frames > max_rec_frames) max_rec_frames = rec_frames;
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
        Pike_fatal ("gc_cycle_pop didn't pop the stack.\n");
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
  describe_rec_stack (beg, "beg", pos, "pos", NULL, NULL);
#endif

  /* Always keep chains of strong refs continuous, or else we risk
   * breaking the order in a later rotation. */
  for (; beg->rf_flags & GC_PREV_STRONG; beg = beg->prev)
    CYCLE_DEBUG_MSG (beg, "> rotate_rec_stack, skipping strong");
#ifdef PIKE_DEBUG
  if (beg == &sentinel_frame)
    Pike_fatal ("Strong ref chain ended up off stack.\n");
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
  describe_rec_stack (beg, "ret", pos, "pos", NULL, NULL);
#endif

  return beg;
}

int gc_cycle_push(void *data, struct marker *m, int weak)
{
  struct marker *pm;

#ifdef PIKE_DEBUG
  if (gc_is_watching && m && m->gc_flags & GC_WATCHED) {
    /* This is useful to set breakpoints on. */
    gc_watched_found (m, "gc_cycle_push()");
  }

  debug_malloc_touch (data);

  if (!data) Pike_fatal ("Got null pointer.\n");
  if (Pike_in_gc != GC_PASS_CYCLE)
    Pike_fatal("GC cycle push attempted in invalid pass.\n");
  if (gc_debug && !(m->gc_flags & GC_PRETOUCHED))
    gc_fatal (data, 0, "gc_cycle_push() called for untouched thing.\n");
  if (!gc_destruct_everything) {
    if ((!(m->gc_flags & GC_NOT_REFERENCED) || m->gc_flags & GC_MARKED) &&
	*(INT32 *) data)
      gc_fatal (data, 1, "Got a referenced marker to gc_cycle_push.\n");
    if (m->gc_flags & GC_XREFERENCED)
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
    if (m->gc_flags & GC_CYCLE_CHECKED && !(m->gc_flags & GC_LIVE)) {
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

      else if (weak < 0) {
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
	/* If the backward link points into a cycle, we rotate the
	 * whole cycle up the stack. See the "cycle checking" blurb
	 * above for rationale. */
	rotate_rec_stack (cycle_frame->cycle_id, weakly_refd);
	CHECK_REC_STACK (cycle_frame, "cycle_frame",
			 weakly_refd, "weakly_refd");
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
	  rot_beg->rf_flags &= ~(GC_PREV_WEAK|GC_PREV_BROKEN);
	  if (weak < 0) rot_beg->rf_flags |= GC_PREV_STRONG;

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
	CHECK_REC_STACK (cycle_frame, "cycle_frame", break_pos, "break_pos");
      }
    }
  }

  else
    if (!(m->gc_flags & GC_CYCLE_CHECKED)) {
      struct gc_rec_frame *r;
#ifdef PIKE_DEBUG
      cycle_checked++;
      if (m->frame)
	gc_fatal (data, 0, "Marker already got a frame.\n");
#endif

      m->gc_flags |= GC_CYCLE_CHECKED | (pm ? pm->gc_flags & GC_LIVE : 0);
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
      CHECK_REC_STACK (NULL, NULL, NULL, NULL);
      return 1;
    }

  /* Should normally not recurse now, but got to do that anyway if we
   * must propagate GC_LIVE flags. */
  if (!pm || !(pm->gc_flags & GC_LIVE) || m->gc_flags & GC_LIVE) {
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
  if (m->gc_flags & GC_LIVE)
    Pike_fatal("Shouldn't mark live recurse when there's nothing to do.\n");
#endif
  m->gc_flags |= GC_LIVE;
  debug_malloc_touch (data);

  if (m->gc_flags & GC_GOT_DEAD_REF) {
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
      if (gc_is_watching && m && m->gc_flags & GC_WATCHED) {
	/* This is useful to set breakpoints on. */
	gc_watched_found (m, "gc_cycle_pop()");
      }
      if (!(m->gc_flags & GC_CYCLE_CHECKED))
	gc_fatal (data, 0, "Marker being popped doesn't have GC_CYCLE_CHECKED.\n");
      if (!gc_destruct_everything) {
	if ((!(m->gc_flags & GC_NOT_REFERENCED) || m->gc_flags & GC_MARKED) &&
	    *(INT32 *) data)
	  gc_fatal (data, 1, "Got a referenced marker to gc_cycle_pop.\n");
	if (m->gc_flags & GC_XREFERENCED)
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
#ifdef PIKE_DEBUG
      popped->next = (void *) (ptrdiff_t) -1;
#endif

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

	if (m->gc_flags & GC_LIVE_OBJ) {
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
	  if (!(m->gc_flags & GC_GOT_DEAD_REF))
	    gc_add_extra_ref (popped->data);

	  CHECK_KILL_LIST_FRAME (popped);
	  CYCLE_DEBUG_MSG (popped, "> gc_cycle_pop, move to kill list");
	}

	else {
	  if (!(m->gc_flags & GC_LIVE)) {
	    /* Add an extra ref which is taken away in the free pass. This
	     * is done to not refcount garb the cycles themselves
	     * recursively, which in bad cases can consume a lot of C
	     * stack. */
	    if (!(m->gc_flags & GC_GOT_DEAD_REF)) {
	      gc_add_extra_ref (popped->data);
	      m->gc_flags |= GC_GOT_DEAD_REF;
	    }
	  }
#ifdef PIKE_DEBUG
	  else
	    if (m->gc_flags & GC_GOT_DEAD_REF)
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
  if (gc_is_watching && (m = find_marker(a)) && m->gc_flags & GC_WATCHED) {
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
    if (!(m->gc_flags & GC_LIVE)) {
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
  if (*(INT32 *) a > !!(m->gc_flags & GC_GOT_EXTRA_REF)) {
    if (!gc_destruct_everything &&
	(!(m->gc_flags & GC_NOT_REFERENCED) || m->gc_flags & GC_MARKED))
      gc_fatal(a, 0, "gc_do_free() called for referenced thing.\n");
    if (gc_debug &&
	(m->gc_flags & (GC_PRETOUCHED|GC_MARKED|GC_IS_REFERENCED)) == GC_PRETOUCHED)
      gc_fatal(a, 0, "gc_do_free() called without prior call to "
	       "gc_mark() or gc_is_referenced().\n");
  }
  if(!gc_destruct_everything &&
     (m->gc_flags & (GC_MARKED|GC_XREFERENCED)) == GC_XREFERENCED)
    gc_fatal(a, 1, "Thing with external reference missed in gc mark pass.\n");
  if ((m->gc_flags & (GC_DO_FREE|GC_LIVE)) == GC_LIVE) live_ref++;
  m->gc_flags |= GC_DO_FREE;
#endif

  return !(m->gc_flags & GC_LIVE);
}

#if 0
static void free_obj_arr(void *oa)
{
  struct array *obj_arr = *((struct array **)oa);

  if (obj_arr) free_array(obj_arr);
  free(oa);
}
#endif

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
#if 0
  struct array **obj_arr_ = (struct array **)xalloc(sizeof(struct array *));
  ONERROR tmp;

  *obj_arr_ = NULL;

  SET_ONERROR(tmp, free_obj_arr, obj_arr_);

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
	  push_static_text("gc");
	  push_static_text("bad_cycle");
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
#endif
}

size_t do_gc(int explicit_call)
{
  ALLOC_COUNT_TYPE start_allocs;
  size_t start_num_objs, unreferenced;
  cpu_time_t gc_start_time, gc_start_real_time;
  ptrdiff_t objs, pre_kill_objs;
#if defined (PIKE_DEBUG) || defined (DO_PIKE_CLEANUP)
  unsigned destruct_count;
#endif
#ifdef PIKE_DEBUG
  unsigned obj_count;
  ONERROR uwp;
#endif

  if(Pike_in_gc) return 0;

  if (gc_enabled <= 0 && (gc_enabled < 0 || !explicit_call)) {
    /* If this happens then the gc has been disabled for a very long
     * time and num_allocs > GC_MAX_ALLOC_THRESHOLD. Have to reset
     * num_allocs, but then we also reset saved_alloc_threshold to
     * GC_MIN_ALLOC_THRESHOLD so that a gc is run quickly if it ever
     * is enabled again. */
#ifdef GC_INTERVAL_DEBUG
    fprintf (stderr, "GC disabled: num_allocs %"PRINT_ALLOC_COUNT_TYPE", "
	     ", alloc_threshold %"PRINT_ALLOC_COUNT_TYPE"\n",
	     num_allocs, alloc_threshold);
#endif
    num_allocs = 0;
    saved_alloc_threshold = GC_MIN_ALLOC_THRESHOLD;
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
  /* For paranoia reasons; avoid gc_generation #0. */
  if (gc_generation++ == 0xffff) gc_generation = 1;
  Pike_in_gc=GC_PASS_PREPARE;

  if (!SAFE_IS_ZERO(&gc_pre_cb)) {
    safe_apply_svalue(&gc_pre_cb, 0, 1);
    pop_stack();
  }

  gc_start_time = get_cpu_time();
  gc_start_real_time = get_real_time();
#ifdef GC_DEBUG
  gc_debug = (GC_DEBUG + 0) || 1;
#else
  gc_debug = d_flag;
#endif
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

  ADD_GC_CALL_FRAME();		/* Placeholder frame for gc_check_object(). */

  last_gc=time(0);
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
    if (n != (unsigned) num_objects && !got_unlinked_things)
      Pike_fatal("Object count wrong before gc; expected %d, got %d.\n", num_objects, n);
    GC_VERBOSE_DO(fprintf(stderr, "| pretouch: %u things\n", n));
  }

  GC_ASSERT_ZAPPED_CALL_FRAME();

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
    gc_check_all_types();
#endif
  } END_ACCEPT_UNFINISHED_TYPE_FIELDS;

  GC_VERBOSE_DO(fprintf(stderr, "| check: %u references in %d things, "
			"counted %"PRINTSIZET"u weak refs\n",
			checked, num_objects, gc_ext_weak_refs));

  GC_ASSERT_ZAPPED_CALL_FRAME();

  /* Object alloc/free are still disallowed, but refs might be lowered
   * by gc_free_(short_)svalue. */

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
      CHECK_MARK_QUEUE_EMPTY();
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
      CHECK_MARK_QUEUE_EMPTY();
    } END_ACCEPT_UNFINISHED_TYPE_FIELDS;

    GC_VERBOSE_DO(fprintf(stderr,
			  "| mark: %u markers referenced, %u weak references freed,\n"
			  "|       %d things to free, "
			  "got %"PRINTSIZET"u tricky weak refs\n",
			  marked, weak_freed, delayed_freed, gc_ext_weak_refs));
  }

  GC_ASSERT_ZAPPED_CALL_FRAME();

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
    if (link_frames)
      Pike_fatal ("Leaked %u link frames.\n", link_frames);
#endif
  }

  GC_ASSERT_ZAPPED_CALL_FRAME();

  if (gc_ext_weak_refs) {
    size_t to_free = gc_ext_weak_refs;
#ifdef PIKE_DEBUG
    obj_count = delayed_freed;
#endif
    Pike_in_gc = GC_PASS_ZAP_WEAK;
    CHECK_MARK_QUEUE_EMPTY();
    /* Zap weak references from external to internal things. That
     * occurs when something has both external weak refs and nonweak
     * cyclic refs from internal things. */
    gc_zap_ext_weak_refs_in_mappings();
    gc_zap_ext_weak_refs_in_arrays();
    gc_zap_ext_weak_refs_in_multisets();
    gc_zap_ext_weak_refs_in_objects();
    gc_zap_ext_weak_refs_in_programs();
    CHECK_MARK_QUEUE_EMPTY();
    GC_VERBOSE_DO(
      fprintf(stderr,
	      "| zap weak: freed %"PRINTPTRDIFFT"d external weak refs, "
	      "%"PRINTSIZET"u internal still around,\n"
	      "|           %d more things to free\n",
	      to_free - gc_ext_weak_refs, gc_ext_weak_refs,
	      delayed_freed - obj_count));
  }

  GC_ASSERT_ZAPPED_CALL_FRAME();

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
    if (n != (unsigned) num_objects && !got_unlinked_things)
      Pike_fatal("Object count wrong in gc; expected %d, got %d.\n", num_objects, n);
#if 0 /* Temporarily disabled - Hubbe */
#ifdef PIKE_DEBUG
#ifdef DEBUG_MALLOC
    PTR_HASH_LOOP(marker, i, m)
      if (!(m->gc_flags & (GC_POSTTOUCHED|GC_WEAK_FREED)) &&
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

  GC_ASSERT_ZAPPED_CALL_FRAME();

  /* Object alloc/free and reference changes are allowed again now. */

  Pike_in_gc=GC_PASS_FREE;
#ifdef PIKE_DEBUG
  weak_freed = 0;
  obj_count = num_objects;
#endif

  GC_POP_CALL_FRAME();	/* We can now free the frame. */

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
  if (free_extra_frames)
    Pike_fatal ("Leaked %u free extra frames.\n", free_extra_frames);
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
  destruct_count = 0;
#endif

  if (!SAFE_IS_ZERO(&gc_post_cb)) {
    safe_apply_svalue(&gc_post_cb, 0, 1);
    pop_stack();
  }

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

      /* A helper to locate garbage through trampolines.
       * FIXME: This ought to be accessible even when pike is compiled
       * without rtldebug. */
      if (gc_trace >= 3 && !gc_destruct_everything) {
	struct gc_rec_frame *r;
	for (r = kill_list; r != &sentinel_frame; r = r->next) {
	  struct object *o = (struct object *) r->data;
	  if (o->prog == pike_trampoline_program && o->refs > 1) {
	    fprintf (stderr, "Got trampoline garbage:\n");
	    describe_something (o, T_OBJECT, 0, 0, 0, NULL);
	    locate_references (o);
	  }
	}
      }
#endif

    while (kill_list != &sentinel_frame) {
      struct gc_rec_frame *next = kill_list->next;
      struct object *o = (struct object *) kill_list->data;

#ifdef PIKE_DEBUG
      if ((get_marker(kill_list->data)->gc_flags & (GC_LIVE|GC_LIVE_OBJ)) !=
	  (GC_LIVE|GC_LIVE_OBJ))
	gc_fatal(o, 0, "Invalid object on kill list.\n");
      if (o->prog && (o->prog->flags & PROGRAM_USES_PARENT) &&
	  PARENT_INFO(o)->parent &&
	  !PARENT_INFO(o)->parent->prog &&
	  get_marker(PARENT_INFO(o)->parent)->gc_flags & GC_LIVE_OBJ)
	gc_fatal(o, 0, "GC destructed parent prematurely.\n");
#endif

      GC_VERBOSE_DO(
	fprintf(stderr, "|   Killing %p with %d refs", o, o->refs);
	if (o->prog) {
	  INT_TYPE line;
	  struct pike_string *file = get_program_line (o->prog, &line);
	  fprintf(stderr, ", prog %s:%d\n", file->str, line);
	  free_string(file);
	}
	else fputs(", is destructed\n", stderr);
      );
      if (!SAFE_IS_ZERO(&gc_destruct_cb)) {
	ref_push_object(o);
	push_int(reason);
	push_int(o->refs - 1);
	safe_apply_svalue(&gc_destruct_cb, 3, 1);
	pop_stack();
      }

      destruct_object (o, reason);
      gc_free_extra_ref(o);
      free_object(o);
#if defined (PIKE_DEBUG) || defined (DO_PIKE_CLEANUP)
      destruct_count++;
#endif
      really_free_gc_rec_frame (kill_list);
      kill_list = next;
    }
  }

#ifdef PIKE_DEBUG
  if (rec_frames)
    Pike_fatal ("Leaked %u rec frames.\n", rec_frames);
#endif

  GC_VERBOSE_DO(fprintf(stderr, "| kill: %u objects killed, "
			"%"PRINTSIZET"u things really freed\n",
			destruct_count, pre_kill_objs - num_objects));

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
#ifdef GC_INTERVAL_DEBUG
    double tmp_dbl1, tmp_dbl2;
#endif

    /* If we're at an automatic and timely gc then start_allocs ==
     * alloc_threshold and we're using gc_average_slowness in the
     * decaying average calculation. Otherwise this is either an
     * explicit call (start_allocs < alloc_threshold) or the gc has
     * been delayed past its due time (start_allocs >
     * alloc_threshold), and in those cases we adjust the multiplier
     * to give the appropriate weight to this last instance. */
    multiplier=pow(gc_average_slowness,
		   (double) start_allocs / (double) alloc_threshold);

#ifdef GC_INTERVAL_DEBUG
    if (GC_VERBOSE_DO(1 ||) gc_trace) fputc ('\n', stderr);
    fprintf (stderr, "IN:  GC start @ %"PRINT_CPU_TIME" "CPU_TIME_UNIT"\n"
	     "     avg slow %g, start_allocs %"PRINT_ALLOC_COUNT_TYPE", "
	     "alloc_threshold %"PRINT_ALLOC_COUNT_TYPE" -> mult %g\n",
	     gc_start_real_time,
	     gc_average_slowness, start_allocs, alloc_threshold, multiplier);
    tmp_dbl1 = non_gc_time;
    tmp_dbl2 = gc_time;
#endif

    /* Comparisons to avoid that overflows mess up the statistics. */
    if (last_gc_end_real_time != -1 &&
	gc_start_real_time > last_gc_end_real_time) {
      last_non_gc_time = gc_start_real_time - last_gc_end_real_time;
      non_gc_time = non_gc_time * multiplier +
	last_non_gc_time * (1.0 - multiplier);
    }
    else last_non_gc_time = (cpu_time_t) -1;
    last_gc_end_real_time = get_real_time();
    if (last_gc_end_real_time > gc_start_real_time) {
      gc_time = gc_time * multiplier +
	(last_gc_end_real_time - gc_start_real_time) * (1.0 - multiplier);
    }

#ifdef GC_INTERVAL_DEBUG
    fprintf (stderr,
	     "     non_gc_time: %13"PRINT_CPU_TIME" "CPU_TIME_UNIT", "
	     "%.12g -> %.12g\n"
	     "     gc_time:     %13"PRINT_CPU_TIME" "CPU_TIME_UNIT", "
	     "%.12g -> %.12g\n",
	     last_non_gc_time, tmp_dbl1, non_gc_time,
	     last_gc_end_real_time > gc_start_real_time ?
	     last_gc_end_real_time - gc_start_real_time : (cpu_time_t) -1,
	     tmp_dbl2, gc_time);
    tmp_dbl1 = objects_alloced;
    tmp_dbl2 = objects_freed;
#endif

    {
      cpu_time_t gc_end_time = get_cpu_time();
      if (gc_end_time > gc_start_time)
	last_gc_time = gc_end_time - gc_start_time;
      else
	last_gc_time = (cpu_time_t) -1;
    }

    /* At this point, unreferenced contains the number of things that
     * were without external references during the check and mark
     * passes. In the process of freeing them, _destruct functions might
     * have been called which means anything might have happened.
     * Therefore we use that figure instead of the difference between
     * the number of allocated things to measure the amount of
     * garbage. */
    last_garbage_ratio = (double) unreferenced / start_num_objs;

    objects_alloced = objects_alloced * multiplier +
      start_allocs * (1.0 - multiplier);
    objects_freed = objects_freed * multiplier +
      unreferenced * (1.0 - multiplier);

#ifdef GC_INTERVAL_DEBUG
    fprintf (stderr,
	     "     objects_alloced: %9"PRINT_ALLOC_COUNT_TYPE" allocs, "
	     "%.12g -> %.12g\n"
	     "     objects_freed:   %9"PRINT_ALLOC_COUNT_TYPE" unrefd, "
	     "%.12g -> %.12g\n",
	     start_allocs, tmp_dbl1, objects_alloced,
	     unreferenced, tmp_dbl2, objects_freed);
#endif

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
#ifdef GC_INTERVAL_DEBUG
      fprintf (stderr, "     strategy: low ratio %g, objs %"PRINTSIZET"u, "
	       "new threshold -> %.12g\n",
	       gc_garbage_ratio_low, start_num_objs, new_threshold);
#endif
    }
    else {
      new_threshold = (objects_alloced+1.0) *
	(gc_garbage_ratio_high * start_num_objs) / (objects_freed+1.0);
      last_garbage_strategy = GARBAGE_RATIO_HIGH;
#ifdef GC_INTERVAL_DEBUG
      fprintf (stderr, "     strategy: high ratio %g, objs %"PRINTSIZET"u, "
	       "new threshold -> %.12g\n",
	       gc_garbage_ratio_high, start_num_objs, new_threshold);
#endif
    }

    if (non_gc_time > 0.0 && gc_min_time_ratio > 0.0) {
      /* Upper limit on the new threshold based on gc_min_time_ratio. */
      double max_threshold = (objects_alloced+1.0) *
	gc_time / (gc_min_time_ratio * non_gc_time);
#ifdef GC_INTERVAL_DEBUG
      fprintf (stderr, "     max interval? min time ratio %g, "
	       "max threshold %.12g -> %s\n",
	       gc_min_time_ratio, max_threshold,
	       max_threshold < new_threshold ? "yes" : "no");
#endif
      if (max_threshold < new_threshold) {
	new_threshold = max_threshold;
	last_garbage_strategy = GARBAGE_MAX_INTERVAL;
      }
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

#ifdef GC_INTERVAL_DEBUG
    fprintf (stderr, "OUT: GC end   @ %"PRINT_CPU_TIME" "CPU_TIME_UNIT", "
	     "new capped threshold %"PRINT_ALLOC_COUNT_TYPE"\n",
	     last_gc_end_real_time, alloc_threshold);
#endif

    if (!explicit_call) {
      auto_gc_real_time += get_real_time() - gc_start_real_time;

      if (last_gc_time != (cpu_time_t) -1) {
#ifdef CPU_TIME_MIGHT_BE_THREAD_LOCAL
	if (cpu_time_is_thread_local
#ifdef PIKE_DEBUG
	    /* At high debug levels, the gc may get called before
	     * the threads are initialized.
	     */
	    && Pike_interpreter.thread_state
#endif
	   )
	  Pike_interpreter.thread_state->auto_gc_time += last_gc_time;
#endif
	auto_gc_time += last_gc_time;
      }
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
		destruct_count, destruct_count == 1 ? "was" : "were", timestr);
      else
#endif
	fprintf(stderr, "done (%"PRINTSIZET"u of %"PRINTSIZET"u "
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
    return destruct_count;
#endif

  if (!SAFE_IS_ZERO(&gc_done_cb)) {
    push_int(unreferenced);
    safe_apply_svalue(&gc_done_cb, 1, 1);
    pop_stack();
  }

  return unreferenced;
}

void do_gc_callback(struct callback *UNUSED(cb), void *UNUSED(arg1),
                    void *UNUSED(arg2))
{
    do_gc(0);
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
 *!       Decaying average over the interval between gc runs, measured
 *!       in real time nanoseconds.
 *!     @member int "gc_time"
 *!       Decaying average over the length of the gc runs, measured in
 *!       real time nanoseconds.
 *!     @member string "last_garbage_strategy"
 *!       The garbage accumulation goal that the gc aimed for when
 *!       setting "alloc_threshold" in the last run. The value is
 *!       either "garbage_ratio_low", "garbage_ratio_high" or
 *!       "garbage_max_interval". The first two correspond to the gc
 *!       parameters with the same names in @[Pike.gc_parameters], and
 *!       the last is the minimum gc time limit specified through the
 *!       "min_gc_time_ratio" parameter to @[Pike.gc_parameters].
 *!     @member int "last_gc"
 *!       Time when the garbage-collector last ran.
 *!     @member int "total_gc_cpu_time"
 *!       The total amount of CPU time that has been consumed in
 *!       implicit GC runs, in nanoseconds. 0 on systems where Pike
 *!       lacks support for CPU time measurement.
 *!     @member int "total_gc_real_time"
 *!       The total amount of real time that has been spent in
 *!       implicit GC runs, in nanoseconds.
 *!   @endmapping
 *!
 *! @seealso
 *!   @[gc()], @[Pike.gc_parameters()], @[Pike.implicit_gc_real_time]
 */
void f__gc_status(INT32 args)
{
  int size = 0;

  pop_n_elems(args);

  push_static_text("num_objects");
  push_int(num_objects);
  size++;

  push_static_text("num_allocs");
  push_int64(num_allocs);
  size++;

  push_static_text("alloc_threshold");
  push_int64(alloc_threshold);
  size++;

  push_static_text("projected_garbage");
  push_float((FLOAT_TYPE)(objects_freed * (double) num_allocs /
                          (double) alloc_threshold));
  size++;

  push_static_text("objects_alloced");
  push_int64((INT64)objects_alloced);
  size++;

  push_static_text("objects_freed");
  push_int64((INT64)objects_freed);
  size++;

  push_static_text("last_garbage_ratio");
  push_float((FLOAT_TYPE) last_garbage_ratio);
  size++;

  push_static_text("non_gc_time");
  push_int64((INT64) non_gc_time);
  size++;

  push_static_text("gc_time");
  push_int64((INT64) gc_time);
  size++;

  push_static_text ("last_garbage_strategy");
  switch (last_garbage_strategy) {
    case GARBAGE_RATIO_LOW:
      push_static_text ("garbage_ratio_low"); break;
    case GARBAGE_RATIO_HIGH:
      push_static_text ("garbage_ratio_high"); break;
    case GARBAGE_MAX_INTERVAL:
      push_static_text ("garbage_max_interval"); break;
#ifdef PIKE_DEBUG
    default:
      Pike_fatal ("Unknown last_garbage_strategy %d\n", last_garbage_strategy);
#endif
  }
  size++;

  push_static_text("last_gc");
  push_int64(last_gc);
  size++;

  push_static_text ("total_gc_cpu_time");
  push_int64 (auto_gc_time);
#ifndef LONG_CPU_TIME
  push_int (1000000000 / CPU_TIME_TICKS);
  o_multiply();
#endif
  size++;

  push_static_text ("total_gc_real_time");
  push_int64 (auto_gc_real_time);
#ifndef LONG_CPU_TIME
  push_int (1000000000 / CPU_TIME_TICKS);
  o_multiply();
#endif
  size++;

#ifdef PIKE_DEBUG
  push_static_text ("max_rec_frames");
  push_int64 ((INT64) tot_max_rec_frames);
  size++;

  push_static_text ("max_link_frames");
  push_int64 ((INT64) tot_max_link_frames);
  size++;

  push_static_text ("max_free_extra_frames");
  push_int64 ((INT64) tot_max_free_extra_frames);
  size++;
#endif

  f_aggregate_mapping(size * 2);
}

/*! @decl int implicit_gc_real_time (void|int nsec)
 *! @belongs Pike
 *!
 *! Returns the total amount of real time that has been spent in
 *! implicit GC runs. The time is normally returned in microseconds,
 *! but if the optional argument @[nsec] is nonzero it's returned in
 *! nanoseconds.
 *!
 *! @seealso
 *!   @[Debug.gc_status]
 */
void f_implicit_gc_real_time (INT32 args)
{
  int nsec = args && !UNSAFE_IS_ZERO (Pike_sp - args);
  pop_n_elems (args);
  if (nsec) {
    push_int64 (auto_gc_real_time);
#ifndef LONG_CPU_TIME
    push_int (1000000000 / CPU_TIME_TICKS);
    o_multiply();
#endif
  }
  else {
#if CPU_TIME_TICKS_LOW > 1000000
    push_int64 (auto_gc_real_time / (CPU_TIME_TICKS / 1000000));
#else
    push_int64 (auto_gc_real_time);
    push_int (1000000 / CPU_TIME_TICKS);
    o_multiply();
#endif
  }
}

void dump_gc_info(void)
{
  fprintf(stderr,"Current number of things   : %d\n",num_objects);
  fprintf(stderr,"Allocations since last gc  : %"PRINT_ALLOC_COUNT_TYPE"\n",
	  num_allocs);
  fprintf(stderr,"Threshold for next gc      : %"PRINT_ALLOC_COUNT_TYPE"\n",
	  alloc_threshold);
  fprintf(stderr,"Projected current garbage  : %f\n",
	  objects_freed * (double) num_allocs / (double) alloc_threshold);

  fprintf(stderr,"Avg allocs between gc      : %f\n",objects_alloced);
  fprintf(stderr,"Avg frees per gc           : %f\n",objects_freed);
  fprintf(stderr,"Garbage ratio in last gc   : %f\n", last_garbage_ratio);

  fprintf(stderr,"Avg "CPU_TIME_UNIT" between gc          : %f\n", non_gc_time);
  fprintf(stderr,"Avg "CPU_TIME_UNIT" in gc               : %f\n", gc_time);
  fprintf(stderr,"Avg time ratio in gc       : %f\n", gc_time / non_gc_time);

  fprintf(stderr,"Garbage strategy in last gc: %s\n",
	  last_garbage_strategy == GARBAGE_RATIO_LOW ? "garbage_ratio_low" :
	  last_garbage_strategy == GARBAGE_RATIO_HIGH ? "garbage_ratio_high" :
	  last_garbage_strategy == GARBAGE_MAX_INTERVAL ?
	  "garbage_max_interval" : "???");

  DWERR("Max used recursion frames  : %u\n", tot_max_rec_frames);
  DWERR("Max used link frames       : %u\n", tot_max_link_frames);
  DWERR("Max used free extra frames : %u\n", tot_max_free_extra_frames);
  DWERR("Marked live ratio          : %g\n",
        (double) tot_mark_live / tot_cycle_checked);
  DWERR("Frame rotation ratio       : %g\n",
        (double) tot_frame_rot / tot_cycle_checked);

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

/* Visit things API */

PMOD_EXPORT visit_ref_cb *visit_ref = NULL;
PMOD_EXPORT visit_enter_cb *visit_enter = NULL;
PMOD_EXPORT visit_leave_cb *visit_leave = NULL;

/* Be careful if extending this with internal types like
 * T_MAPPING_DATA and T_MULTISET_DATA; there's code that assumes
 * type_from_visit_fn only returns types that fit in a TYPE_FIELD. */
PMOD_EXPORT visit_thing_fn *const visit_fn_from_type[MAX_TYPE + 1] = {
  (visit_thing_fn *) (ptrdiff_t) -1,
  (visit_thing_fn *) (ptrdiff_t) -1,
  (visit_thing_fn *) (ptrdiff_t) -1,
  (visit_thing_fn *) (ptrdiff_t) -1,
  (visit_thing_fn *) (ptrdiff_t) -1,
  (visit_thing_fn *) (ptrdiff_t) -1,
  (visit_thing_fn *) (ptrdiff_t) -1,
  (visit_thing_fn *) (ptrdiff_t) -1,
  (visit_thing_fn *)&visit_array,
  (visit_thing_fn *)&visit_mapping,
  (visit_thing_fn *)&visit_multiset,
  (visit_thing_fn *)&visit_object,
  /* visit_function must be called with a whole svalue, so it's not
   * included here. */
  (visit_thing_fn *) (ptrdiff_t) -1,
  (visit_thing_fn *)&visit_program,
  (visit_thing_fn *)&visit_string,
  (visit_thing_fn *)&visit_type,
};

PMOD_EXPORT TYPE_T type_from_visit_fn (visit_thing_fn *fn)
{
  /* Since the array to search is so small, linear search is probably
   * fastest. */
  unsigned t;
  for (t = 0; t < NELEM (visit_fn_from_type); t++)
    if (visit_fn_from_type[t] == fn)
      return (TYPE_T) t;
  return PIKE_T_UNKNOWN;
}

PMOD_EXPORT TYPE_FIELD real_visit_svalues (struct svalue *s, size_t num,
					   int ref_type, void *extra)
{
  for (; num; num--, s++)
    visit_svalue (s, ref_type, extra);
  return 0;
}

/* Memory counting
 *
 * This mode is used by f_count_memory, and it's recognized by a
 * nonzero value in mc_pass.
 *
 * The basic idea is to follow and count all refs from the starting
 * point things given to f_count_memory. Whenever the counted refs add
 * up to the refcount for a thing, that thing is known to have only
 * internal refs, and so it's memory counted and then all its refs are
 * followed too.
 *
 * To cope with internal cyclic refs, there's a "lookahead" algorithm
 * which recurses through more things in the hope of finding cycles
 * that otherwise would make us miss internal refs. This lookahead is
 * limited by mc_lookahead, mc_block_lookahead, and the constant or
 * variable "pike_cycle_depth" in objects.
 *
 * All things are categorized as follows:
 *
 * o  Internal things: These are known to have only internal
 *    references and are memory counted. The things given to
 *    f_count_memory as starting points are initially asserted to be
 *    internal regardless of how many refs they got.
 *
 * o  Lookahead things: A lookahead thing is one that has been found
 *    by following refs from an internal thing.
 *
 *    Lookahead things are further divided into three categories:
 *
 *    o  Incomplete: Things whose refcounts (still) are higher than
 *       all found refs to them from both internal and lookahead
 *       things.
 *
 *    o  Complete: Things whose refs from internal and lookahead
 *       things equal their refcounts. I.e. we've found all refs going
 *       to these.
 *
 *       Complete things can also be "candidates", which means they
 *       have a direct ref from an internal thing or another
 *       candidate.
 *
 *    o  Indirectly incomplete: In MC_PASS_MARK_EXTERNAL, these are
 *       all the complete things found to be referenced by incomplete
 *       things.
 *
 *    These sets are tracked through three double linked lists,
 *    mc_incomplete, mc_complete, and mc_indirect, respectively.
 *
 *    The lookahead is controlled by a lookahead count for each thing.
 *    The count is the number of links to follow emanating from that
 *    thing. The count for internal things and candidates default to
 *    mc_lookahead, but if a thing is an object with a
 *    "pike_cycle_depth" variable, that number overrides it.
 *
 *    As links are followed to other things, their lookahead count
 *    gets lowered, and the lookahead stops when it reaches zero (or
 *    when reaching a thing of a type in mc_block_lookahead). If a
 *    lookahead thing is found later on through another path with
 *    fewer links, its lookahead count is raised so that it eventually
 *    reflects the shortest path.
 *
 *    The reason for the "candidate" things which are kept at max
 *    lookahead count is that the lookahead thereby continue as long
 *    as it resolves complete things which eventually might turn out
 *    to be internal. That means the lookahead distance only needs to
 *    be large enough to cover the largest "loop" inside a structure
 *    with many cycles, rather than the longest cyclic path.
 *
 *    E.g. to cover a double linked list which can be arbitrary long,
 *    it's enough that mc_lookahead is 3; that makes the lookahead
 *    account for the two refs to the next node (B) from the previous
 *    complete node (A), by traversing through the next-to-next node
 *    (C):
 *                                      3
 *                    L---.   L---.   L---.   L---.
 *                 ...     (A)     (B)     (C)     ...
 *                    `---7   `---7   `---7   `---7
 *                              1       2
 *
 * o  Unvisited things: Everything else that hasn't been visited yet.
 *
 * For every visited thing we record the number of refs from internal
 * things (int_refs) and from lookahead things (la_refs).
 *
 * The basic algorithm for finding all internal things works like
 * this:
 *
 * First the starting point things are labelled internal and put into
 * the work queue (mc_work_queue).
 *
 * mc_pass is set to MC_PASS_LOOKAHEAD:
 *
 * We do a breadth-first recursion through the things in the work
 * queue until the lookahead count reaches zero, always starting with
 * the things with the highest count.
 *
 * Every time we visit something we calculate its lookahead count as
 * either max (if it's found to be referenced from an internal or
 * candidate thing), the same as the source thing (if the followed ref
 * is REF_TYPE_INTERNAL), or the next lower count (otherwise). If the
 * count is above zero and the thing is either new or its old count
 * was lower, it's added to the work list.
 *
 * mc_work_queue is a priority queue which always has the thing with
 * the highest lookahead count first, thereby ensuring breadth-first
 * recursion also when things have their count raised.
 *
 * int_refs and la_refs are updated when things are visited. They
 * become internal if int_refs add up to the refcount. Otherwise they
 * are put in the incomplete or complete sets as appropriate.
 *
 * mc_pass is set to MC_PASS_MARK_EXTERNAL:
 *
 * At this point the set of lookahead things is complete (as far as we
 * are concerned), and it's divided into complete and incomplete
 * lookahead things. All references in the incomplete list are
 * followed to build up the set of indirectly incomplete things. The
 * incomplete and indirectly incomplete things are referenced
 * externally and should not be memory counted.
 *
 * If there's anything left in the complete list then it's internal
 * cyclic stuff. In that case we put those things into the work list,
 * move the indirectly incomplete list back to complete and repeat
 * MC_PASS_LOOKAHEAD. Otherwise we're done.
 */

/* #define MEMORY_COUNT_DEBUG */

#define MC_WQ_START_SIZE 1024

#ifdef PIKE_THREADS
static IMUTEX_T mc_mutex;
#endif

PMOD_EXPORT int mc_pass;
PMOD_EXPORT size_t mc_counted_bytes;

static int mc_lookahead, mc_block_pike_cycle_depth;
static TYPE_FIELD mc_block_lookahead;
static TYPE_FIELD mc_block_lookahead_default = BIT_PROGRAM|BIT_STRING|BIT_TYPE;
/* Strings are blocked because they don't contain refs. Types are
 * blocked because they are acyclic and don't contain refs to anything
 * but strings and other types. */

static inline int mc_lookahead_blocked(unsigned INT16 type) {
    if (type < sizeof(TYPE_FIELD)*8) {
        return !!(mc_block_lookahead & ((TYPE_FIELD)1 << type));
    }

    return 0;
}

static int mc_block_strings;

static int mc_enqueued_noninternal;
/* Set whenever something is enqueued in MC_PASS_LOOKAHEAD that isn't
 * internal already. This is used to detect whether another
 * MC_PASS_MARK_EXTERNAL is necessary. */

static unsigned mc_ext_toggle_bias = 0;

#define MC_PASS_LOOKAHEAD 1
#define MC_PASS_MARK_EXTERNAL 2

/* Set when a thing has become internal. */
#define MC_FLAG_INTERNAL	0x01

/* Set when an internal thing has been visited, i.e. after its refs
 * has been gone through for the first time. This implies that the
 * thing has been memory counted, and taken off mc_incomplete or
 * mc_complete if it was there. */
#define MC_FLAG_INT_VISITED	0x02

/* Set when a non-internal thing has been visited. If
 * MC_FLAG_INT_VISITED isn't then the thing is on one of mc_incomplete,
 * mc_complete, or (in MC_PASS_MARK_EXTERNAL) mc_indirect. */
#define MC_FLAG_LA_VISITED	0x04

/* Set when a thing has become a candidate (i.e. complete and
 * referenced directly from an internal or candidate thing). This
 * flag is meaningless when MC_FLAG_INTERNAL is set. */
#define MC_FLAG_CANDIDATE	0x08

/* Set when a thing is visited directly from an internal or candidate
 * thing. */
#define MC_FLAG_CANDIDATE_REF	0x10

/* The lookahead count should not change. Use when it has been lowered
 * from pike_cycle_depth. */
#define MC_FLAG_LA_COUNT_FIXED	0x20

/* A toggle flag to mark external (i.e. incomplete and indirectly
 * incomplete) things in MC_PASS_MARK_EXTERNAL so that we don't
 * recurse them repeatedly. If mc_ext_toggle_bias is zero then it's
 * external if this is set. If mc_ext_toggle_bias is one then it's
 * external if this is cleared. mc_ext_toggle_bias toggles every time
 * we leave MC_PASS_MARK_EXTERNAL, thus we avoid the work to go
 * through the externals clear the flag for the next round. */
#define MC_FLAG_EXT_TOGGLE	0x40

/* The value of IS_EXTERNAL is meaningless when MC_FLAG_INTERNAL is set. */
#define IS_EXTERNAL(M)							\
  (((M)->flags ^ mc_ext_toggle_bias) & MC_FLAG_EXT_TOGGLE)

#define INIT_CLEARED_EXTERNAL(M) do {					\
    struct mc_marker *_m = (M);						\
    debug_malloc_touch_named(_m, "INIT_CLEARED_EXTERNAL");		\
    if (mc_ext_toggle_bias) _m->flags |= MC_FLAG_EXT_TOGGLE;		\
  } while (0)
#define FLAG_EXTERNAL(M) do {						\
    struct mc_marker *_m = (M);						\
    debug_malloc_touch_named(_m, "FLAG_EXTERNAL");			\
    assert (!IS_EXTERNAL (_m));						\
    _m->flags ^= MC_FLAG_EXT_TOGGLE;					\
  } while (0)
#define TOGGLE_EXT_FLAGS() do {						\
    mc_ext_toggle_bias ^= MC_FLAG_EXT_TOGGLE;				\
  } while (0)

struct mc_marker
{
  struct mc_marker *hash_next;	/* Used by PTR_HASH_ALLOC. */
  struct mc_marker *dl_prev;	/* For the mc_incomplete, mc_complete and */
  struct mc_marker *dl_next;	/*   mc_indirect lists. Used iff not internal.*/
  void *thing;			/* Referenced thing. */
  visit_thing_fn *visit_fn;	/* Visit function for it */
  void *extra;			/*   and its extra data. */
  INT32 int_refs;		/* These refcounts are bogus */
  INT32 la_refs;		/*   for internal things. */
  unsigned INT32 queuepos;	/* Position in mc_work_queue, or
				 * MAX_UINT32 if not queued. */
  unsigned INT16 la_count;	/* Lookahead count. */
  unsigned INT16 flags;
};

#undef BLOCK_ALLOC_NEXT
#undef BLOCK_ALLOC_NEXT
#define BLOCK_ALLOC_NEXT hash_next
#undef PTR_HASH_ALLOC_DATA
#define PTR_HASH_ALLOC_DATA thing
#undef INIT_BLOCK
#define INIT_BLOCK(f)
#undef EXIT_BLOCK
#define EXIT_BLOCK(f)

PTR_HASH_ALLOC_FILL_PAGES (mc_marker, 2)

static void start_mc(void)
{
  LOCK_IMUTEX(&mc_mutex);
  init_mc_marker_hash();
}

static void stop_mc(void)
{
  exit_mc_marker_hash();
  UNLOCK_IMUTEX(&mc_mutex);
}

static struct mc_marker *my_make_mc_marker (void *thing,
					    visit_thing_fn *visit_fn,
					    void *extra)
{
  struct mc_marker *m = make_mc_marker (thing);
  assert (thing);
  assert (visit_fn);
  m->thing = thing;
  m->visit_fn = visit_fn;
  m->extra = extra;
  m->int_refs = m->la_refs = m->flags = 0;
  INIT_CLEARED_EXTERNAL (m);
  m->queuepos = MAX_UINT32;
#ifdef PIKE_DEBUG
  m->dl_prev = m->dl_next = (void *) (ptrdiff_t) -1;
  m->la_count = ((unsigned INT16) -1) >> 1;
#endif
  assert(find_mc_marker(thing) == m);
  return m;
}

#ifdef MEMORY_COUNT_DEBUG
static void describe_mc_marker (struct mc_marker *m)
{
  fprintf (stderr, "%s %p: refs %d, int %d, la %d, cnt %d",
	   get_name_of_type (type_from_visit_fn (m->visit_fn)),
	   m->thing, *(INT32 *) m->thing, m->int_refs, m->la_refs, m->la_count);
  if (m->queuepos != MAX_UINT32) fprintf (stderr, ", wq %u", m->queuepos);
  if (m->flags & MC_FLAG_INTERNAL) fputs (", I", stderr);
  if (m->flags & MC_FLAG_INT_VISITED) fputs (", IV", stderr);
  if (m->flags & MC_FLAG_LA_VISITED) fputs (", LAV", stderr);
  if (m->flags & MC_FLAG_CANDIDATE) fputs (", C", stderr);
  if (m->flags & MC_FLAG_CANDIDATE_REF) fputs (", CR", stderr);
  if (m->flags & MC_FLAG_LA_COUNT_FIXED) fputs (", CF", stderr);
  if (IS_EXTERNAL (m))
    fputs (m->flags & MC_FLAG_INTERNAL ? ", (E)" : ", E", stderr);
}
#endif

/* Sentinel for the incomplete lookaheads list. */
static struct mc_marker mc_incomplete = {
  (void *) (ptrdiff_t) -1,
  &mc_incomplete, &mc_incomplete,
  (void *) (ptrdiff_t) -1, (visit_thing_fn *) (ptrdiff_t) -1,
  (void *) (ptrdiff_t) -1,
  -1, -1, MAX_UINT32, 0, (unsigned INT16) -1
};

/* Sentinel for the complete lookaheads list. The reason all complete
 * things are tracked and not only the candidates is that elements
 * then can be easily moved to mc_indirect (and back) without special
 * cases when noncandidate complete things become indirectly
 * incomplete. */
static struct mc_marker mc_complete = {
  (void *) (ptrdiff_t) -1,
  &mc_complete, &mc_complete,
  (void *) (ptrdiff_t) -1, (visit_thing_fn *) (ptrdiff_t) -1,
  (void *) (ptrdiff_t) -1,
  -1, -1, MAX_UINT32, 0, (unsigned INT16) -1
};

/* Sentinel for the indirectly incomplete lookaheads list. */
static struct mc_marker mc_indirect = {
  (void *) (ptrdiff_t) -1,
  &mc_indirect, &mc_indirect,
  (void *) (ptrdiff_t) -1, (visit_thing_fn *) (ptrdiff_t) -1,
  (void *) (ptrdiff_t) -1,
  -1, -1, MAX_UINT32, 0, (unsigned INT16) -1
};

#define DL_IS_EMPTY(LIST) (LIST.dl_next == &LIST)

#define DL_ADD_LAST(LIST, M) do {					\
    struct mc_marker *_m = (M);						\
    struct mc_marker *_list_prev = LIST.dl_prev;			\
    DO_IF_DEBUG (							\
      assert (_m->dl_prev == (void *) (ptrdiff_t) -1);			\
      assert (_m->dl_next == (void *) (ptrdiff_t) -1);			\
    );									\
    debug_malloc_touch_named(_m, "DL_ADD_LAST (_m)");			\
    debug_malloc_touch_named(_list_prev, "DL_ADD_LAST (_list_prev)");	\
    _m->dl_prev = _list_prev;						\
    _m->dl_next = &LIST;						\
    LIST.dl_prev = _list_prev->dl_next = _m;				\
  } while (0)

#define DL_REMOVE(M) do {						\
    struct mc_marker *_m = (M);						\
    struct mc_marker *_list_prev = _m->dl_prev;				\
    struct mc_marker *_list_next = _m->dl_next;				\
    assert (_m->dl_prev != (void *) (ptrdiff_t) -1);			\
    assert (_m->dl_next != (void *) (ptrdiff_t) -1);			\
    debug_malloc_touch_named(_m, "DL_REMOVE (_m)");			\
    debug_malloc_touch_named(_list_prev, "DL_REMOVE (_list_prev)");	\
    debug_malloc_touch_named(_list_next, "DL_REMOVE (_list_next)");	\
    _list_prev->dl_next = _list_next;					\
    _list_next->dl_prev = _list_prev;					\
    DO_IF_DEBUG (_m->dl_prev = _m->dl_next = (void *) (ptrdiff_t) -1);	\
  } while (0)

#define DL_MOVE(FROM_LIST, TO_LIST) do {				\
    if (FROM_LIST.dl_next != &FROM_LIST) {				\
      struct mc_marker *to_list_last = TO_LIST.dl_prev;			\
      debug_malloc_touch_named(&FROM_LIST, "DL_MOVE (FROM_LIST");	\
      debug_malloc_touch_named(&TO_LIST, "DL_MOVE (TO_LIST)");		\
      debug_malloc_touch_named(to_list_last, "DL_MOVE (to_list_last)");	\
      TO_LIST.dl_prev = FROM_LIST.dl_prev;				\
      to_list_last->dl_next = FROM_LIST.dl_next;			\
      FROM_LIST.dl_prev->dl_next = &TO_LIST;				\
      FROM_LIST.dl_next->dl_prev = to_list_last;			\
      FROM_LIST.dl_prev = FROM_LIST.dl_next = &FROM_LIST;		\
    }									\
  } while (0)

#define DL_MAKE_EMPTY(LIST) do {					\
    LIST.dl_prev = LIST.dl_next = &LIST;				\
  } while (0)

static struct mc_marker *mc_ref_from = (void *) (ptrdiff_t) -1;

#ifdef MEMORY_COUNT_DEBUG
static void MC_DEBUG_MSG (struct mc_marker *m, const char *msg)
{
  switch (mc_pass) {
    case MC_PASS_LOOKAHEAD: fputs ("LA ", stderr); break;
    case MC_PASS_MARK_EXTERNAL: fputs ("ME ", stderr); break;
  }
  if (m) {
    if (mc_ref_from != (void *) (ptrdiff_t) -1) fputs ("  [", stderr);
    else fputs ("[", stderr);
    describe_mc_marker (m);
    fprintf (stderr, "] %s\n", msg);
  }
  else if (mc_ref_from != (void *) (ptrdiff_t) -1) {
    fputs ("{", stderr);
    describe_mc_marker (mc_ref_from);
    fprintf (stderr, "} %s\n", msg);
  }
}
#define MC_DEBUG_MSG(m, msg) MC_DEBUG_MSG(debug_malloc_pass_named(m, TOSTR(msg)), msg)
#else
#define MC_DEBUG_MSG(m, msg) do {} while (0)
#endif

/* The following is a standard binary heap priority queue implemented
 * using an array. C.f. http://www.sbhatnagar.com/SourceCode/pqueue.html. */

/* Note: 1-based indexing is used in mc_work_queue to avoid
 * off-by-ones in the binary arithmetic. */
static struct mc_marker **mc_work_queue = NULL;
static unsigned INT32 mc_wq_size, mc_wq_used;

#ifdef PIKE_DEBUG
#define CHECK_WQ() if (d_flag) {					\
    unsigned i;								\
    assert (mc_wq_used >= 1);						\
    for (i = 1; i <= (mc_wq_used - 1) / 2; i++) {			\
      assert (mc_work_queue[i]->queuepos == i);				\
      assert (mc_work_queue[i]->la_count >=				\
	      mc_work_queue[2 * i]->la_count);				\
      if (2 * i + 1 < mc_wq_used)					\
	assert (mc_work_queue[i]->la_count >=				\
		mc_work_queue[2 * i + 1]->la_count);			\
    }									\
    for (; i < mc_wq_used; i++)						\
      assert (mc_work_queue[i]->queuepos == i);				\
  }
#else
#define CHECK_WQ() do {} while (0)
#endif

static struct mc_marker *mc_wq_dequeue()
{
  struct mc_marker *m;

  assert (mc_work_queue);

  if (mc_wq_used == 1) return NULL;

  m = mc_work_queue[1];
  m->queuepos = MAX_UINT32;

  if (--mc_wq_used > 1) {
    struct mc_marker *n, *last = mc_work_queue[mc_wq_used];
    int last_la_count = last->la_count;
    unsigned pos = 1;
    debug_malloc_touch_named(last, "last");

    while (pos <= mc_wq_used / 2) {
      unsigned child_pos = 2 * pos;

      if (child_pos < mc_wq_used &&
	  mc_work_queue[child_pos]->la_count <
	  mc_work_queue[child_pos + 1]->la_count)
	child_pos++;
      if (mc_work_queue[child_pos]->la_count <= last_la_count)
	break;

      debug_malloc_touch_named(mc_work_queue[pos], "mc_work_queue[pos]");
      n = mc_work_queue[pos] = mc_work_queue[child_pos];
      n->queuepos = pos;
      debug_malloc_touch_named(n, "n");
      pos = child_pos;
    }

    debug_malloc_touch_named(mc_work_queue[pos], "mc_work_queue[pos] = last");
    mc_work_queue[pos] = last;
    last->queuepos = pos;
  }

  CHECK_WQ();

  return m;
}

static void mc_wq_enqueue (struct mc_marker *m)
/* m may already be in the queue, provided that m->la_count isn't
 * lower than its old value. */
{
  struct mc_marker *n;
  unsigned pos;
  int m_la_count = m->la_count;

  assert (mc_work_queue);
#ifdef PIKE_DEBUG
  assert (mc_lookahead < 0 || m_la_count != ((unsigned INT16) -1) >> 1);
#endif

  if (m->queuepos != MAX_UINT32) {
    assert (m->queuepos < mc_wq_used);
    assert (m->queuepos * 2 >= mc_wq_used ||
	    m_la_count >= mc_work_queue[m->queuepos * 2]->la_count);
    assert (m->queuepos * 2 + 1 >= mc_wq_used ||
	    m_la_count >= mc_work_queue[m->queuepos * 2 + 1]->la_count);
    pos = m->queuepos;
  }

  else {
    if (mc_wq_used > mc_wq_size) {
      void *p;
      mc_wq_size *= 2;
      /* NB: Add 1 to size to compensate for 1-based indexing. */
      p = realloc (mc_work_queue, (mc_wq_size + 1) * sizeof (mc_work_queue[0]));
      if (!p) {
	make_error (msg_out_of_mem_2,
		    (mc_wq_size + 1) * sizeof (mc_work_queue[0]));
	free_svalue (&throw_value);
	move_svalue (&throw_value, --Pike_sp);
	mc_wq_size /= 2;
	return;
      }
      mc_work_queue = p;
    }
    pos = mc_wq_used++;
  }

  while (pos > 1 && (n = mc_work_queue[pos / 2])->la_count < m_la_count) {
    debug_malloc_touch_named(n, "n");
    mc_work_queue[pos] = n;
    n->queuepos = pos;
    pos /= 2;
  }
  debug_malloc_touch_named(m, "m");
  mc_work_queue[pos] = m;
  m->queuepos = pos;

  CHECK_WQ();
}

static struct svalue pike_cycle_depth_str = SVALUE_INIT_FREE;

static int mc_cycle_depth_from_obj (struct object *o)
{
  struct program *p = o->prog;
  struct svalue val;

  if (!p) return 0; /* No need to look ahead in destructed objects. */

  object_index_no_free2 (&val, o, 0, &pike_cycle_depth_str);

  if (TYPEOF(val) != T_INT) {
    int i = find_shared_string_identifier (pike_cycle_depth_str.u.string, p);
    INT_TYPE line;
    struct pike_string *file = get_identifier_line (p, i, &line);
    make_error ("Object got non-integer pike_cycle_depth %pO at %pS:%ld.\n",
		&val, file, (long)line);
    free_svalue (&val);
    free_svalue (&throw_value);
    move_svalue (&throw_value, --Pike_sp);
    return -1;
  }

  if (SUBTYPEOF(val) == NUMBER_UNDEFINED)
    return -1;

  if (val.u.integer > (unsigned INT16) -1)
    return (unsigned INT16) -1;

  if (val.u.integer < 0) {
    int i = find_shared_string_identifier (pike_cycle_depth_str.u.string, p);
    INT_TYPE line;
    struct pike_string *file = get_identifier_line (p, i, &line);
    make_error ("Object got negative pike_cycle_depth %pO at %pS:%ld.\n",
		&val, file, (long)line);
    free_svalue (&throw_value);
    move_svalue (&throw_value, --Pike_sp);
    return -1;
  }

  return val.u.integer;
}

static void pass_lookahead_visit_ref (void *thing, int ref_type,
				      visit_thing_fn *visit_fn, void *extra)
{
  struct mc_marker *ref_to;
  int ref_from_flags, ref_to_flags, old_la_count, ref_to_la_count;
  int ref_added = 0, check_new_candidate = 0, la_count_handled = 0;

  assert (mc_lookahead >= 0);
  assert (mc_pass == MC_PASS_LOOKAHEAD);
#ifdef PIKE_DEBUG
  assert (mc_ref_from != (void *) (ptrdiff_t) -1);
  assert (mc_ref_from->la_count != ((unsigned INT16) -1) >> 1);
#endif

  if (mc_block_strings > 0 &&
      visit_fn == (visit_thing_fn *) &visit_string) {
#ifdef MEMORY_COUNT_DEBUG
    ref_to = find_mc_marker (thing);
#endif
    MC_DEBUG_MSG (ref_to, "ignored string");
    return;
  }

  ref_to = find_mc_marker (thing);
  debug_malloc_touch_named(ref_to, "ref_to");
  ref_from_flags = mc_ref_from->flags;

  /* Create mc_marker if necessary. */

  if (!ref_to) {
    ref_to = my_make_mc_marker (thing, visit_fn, extra);
    debug_malloc_touch(ref_to);
    MC_DEBUG_MSG (ref_to, "visiting new thing");
    assert (!(ref_from_flags & (MC_FLAG_INT_VISITED | MC_FLAG_LA_VISITED)));
    ref_to_la_count = old_la_count = 0;
  }
  else if (ref_to->flags & MC_FLAG_INTERNAL) {
    /* Ignore refs to internal things. Can't treat them like other
     * things anyway since the int_refs aren't valid for the starting
     * points. */
    MC_DEBUG_MSG (ref_to, "ignored internal");
    assert (ref_to->la_count != ((unsigned INT16) -1) >> 1);
    return;
  }
  else {
    MC_DEBUG_MSG (ref_to, "visiting old thing");
    ref_to_la_count = old_la_count = ref_to->la_count;
    assert (ref_to->la_count != ((unsigned INT16) -1) >> 1);
  }

  ref_to_flags = ref_to->flags;

#define SET_LA_COUNT_FOR_INT_OR_CF() do {				\
    if (!(ref_to_flags & MC_FLAG_LA_COUNT_FIXED)) {			\
      int cycle_depth;							\
      if (visit_fn == (visit_thing_fn *) &visit_object &&		\
	  !mc_block_pike_cycle_depth &&					\
	  (cycle_depth =						\
	   mc_cycle_depth_from_obj ((struct object *) thing)) >= 0) {	\
	ref_to_la_count = cycle_depth;					\
	ref_to_flags |= MC_FLAG_LA_COUNT_FIXED;				\
	MC_DEBUG_MSG (ref_to, "la_count set to pike_cycle_depth");	\
      }									\
									\
      else {								\
	int count_from_source = ref_type & REF_TYPE_INTERNAL ?		\
	  mc_ref_from->la_count : mc_ref_from->la_count - 1;		\
									\
	if (ref_to_la_count < mc_lookahead) {				\
	  ref_to_la_count = mc_lookahead;				\
	  MC_DEBUG_MSG (ref_to, "la_count raised to mc_lookahead");	\
	}								\
									\
	if (ref_to_la_count < count_from_source) {			\
	  ref_to_la_count = count_from_source;				\
	  MC_DEBUG_MSG (ref_to, count_from_source == mc_ref_from->la_count ? \
			"la_count raised to source count" :		\
			"la_count raised to source count - 1");		\
	}								\
      }									\
    }									\
  } while (0)

  if ((ref_from_flags & (MC_FLAG_INTERNAL | MC_FLAG_INT_VISITED)) ==
      MC_FLAG_INTERNAL) {
    if (ref_from_flags & MC_FLAG_LA_VISITED) {
      /* mc_ref_from is a former lookahead thing that has become internal. */
      assert (ref_to->la_refs > 0);
      ref_to->la_refs--;
      ref_to->int_refs++;
      MC_DEBUG_MSG (ref_to, "converted lookahead ref to internal");
    }
    else {
      ref_to->int_refs++;
      MC_DEBUG_MSG (ref_to, "added internal ref");
      ref_added = 1;
    }
    assert (ref_to->int_refs + ref_to->la_refs <= *(INT32 *) thing);

    /* Handle the target becoming internal. */

    if (ref_to->int_refs == *(INT32 *) thing ||
	(mc_block_strings < 0 &&
	 visit_fn == (visit_thing_fn *) &visit_string)) {
      assert (!(ref_to_flags & MC_FLAG_INTERNAL));
      assert (!(ref_to_flags & MC_FLAG_INT_VISITED));
      ref_to_flags |= MC_FLAG_INTERNAL;

      SET_LA_COUNT_FOR_INT_OR_CF();

      ref_to->flags = ref_to_flags;
      ref_to->la_count = ref_to_la_count;
      if (ref_to->queuepos != MAX_UINT32 && old_la_count == ref_to_la_count)
	MC_DEBUG_MSG (ref_to, "already in queue");
      else {
	assert (ref_to->la_count >= old_la_count);
	mc_wq_enqueue (ref_to);
	MC_DEBUG_MSG (ref_to, "enqueued internal");
      }
      return;
    }
  }

  if ((ref_from_flags & (MC_FLAG_INTERNAL | MC_FLAG_CANDIDATE)) &&
      !(ref_to_flags & MC_FLAG_CANDIDATE_REF)) {
    ref_to_flags |= MC_FLAG_CANDIDATE_REF;
    MC_DEBUG_MSG (ref_to, "got candidate ref");

    SET_LA_COUNT_FOR_INT_OR_CF();

    check_new_candidate = la_count_handled = 1;
  }

  if (!(ref_from_flags & (MC_FLAG_INTERNAL | MC_FLAG_LA_VISITED))) {
    assert (ref_to->int_refs + ref_to->la_refs < *(INT32 *) thing);
    ref_to->la_refs++;
    MC_DEBUG_MSG (ref_to, "added lookahead ref");
    ref_added = 1;
  }

  if (ref_added && (ref_to->int_refs + ref_to->la_refs == *(INT32 *) thing)) {
    MC_DEBUG_MSG (ref_to, "refs got complete");
    check_new_candidate = 1;

    if (ref_to_flags & MC_FLAG_LA_VISITED) {
      /* Move to mc_complete if it has been lookahead visited. In other
       * cases this is handled after the lookahead visit is done. */
      DL_REMOVE (ref_to);
      DL_ADD_LAST (mc_complete, ref_to);
      MC_DEBUG_MSG (ref_to, "moved to complete list");
    }
  }

  /* Handle the target becoming a candidate. */

  if (check_new_candidate &&
      (ref_to_flags & MC_FLAG_CANDIDATE_REF) &&
      ref_to->int_refs + ref_to->la_refs == *(INT32 *) thing) {
    assert (!(ref_to_flags & MC_FLAG_CANDIDATE));
    assert (ref_to->la_refs > 0);
    ref_to_flags |= MC_FLAG_CANDIDATE;
    MC_DEBUG_MSG (ref_to, "made candidate");

    ref_to->flags = ref_to_flags;
    ref_to->la_count = ref_to_la_count;

    if (mc_lookahead_blocked(type_from_visit_fn (visit_fn))) {
      MC_DEBUG_MSG (ref_to, "type is blocked - not enqueued");
      return;
    }

    if (ref_to_la_count > 0) {
      /* Always enqueue if the count allows it, even if it hasn't
       * increased. That since MC_FLAG_CANDIDATE_REF must be propagated. */
      if (ref_to->queuepos != MAX_UINT32 && old_la_count == ref_to_la_count)
	MC_DEBUG_MSG (ref_to, "already in queue");
      else {
	assert (ref_to->la_count >= old_la_count);
	mc_wq_enqueue (ref_to);
	MC_DEBUG_MSG (ref_to, "enqueued candidate");
	mc_enqueued_noninternal = 1;
      }
    }
    else
      MC_DEBUG_MSG (ref_to, "candidate not enqueued due to zero count");
    return;
  }

  /* Normal handling. */

  if (mc_lookahead_blocked(type_from_visit_fn (visit_fn))) {
    ref_to->flags = ref_to_flags;
    ref_to->la_count = ref_to_la_count;
    MC_DEBUG_MSG (ref_to, "type is blocked - not enqueued");
    return;
  }

  if (!la_count_handled && !(ref_to_flags & MC_FLAG_LA_COUNT_FIXED)) {
    int cycle_depth;
    int count_from_source = ref_type & REF_TYPE_INTERNAL ?
      mc_ref_from->la_count : mc_ref_from->la_count - 1;

    if (ref_to_la_count < count_from_source) {
      ref_to_la_count = count_from_source;
      MC_DEBUG_MSG (ref_to, count_from_source == mc_ref_from->la_count ?
		    "la_count raised to source count" :
		    "la_count raised to source count - 1");
    }

    if (visit_fn == (visit_thing_fn *) &visit_object &&
	!mc_block_pike_cycle_depth &&
	(cycle_depth =
	 mc_cycle_depth_from_obj ((struct object *) thing)) >= 0 &&
	cycle_depth < ref_to_la_count) {
      /* pike_cycle_depth is only allowed to lower the lookahead count
       * for things that aren't internal, candidates, or candidate ref'd. */
      ref_to_la_count = cycle_depth;
      ref_to_flags |= MC_FLAG_LA_COUNT_FIXED;
      MC_DEBUG_MSG (ref_to, "la_count lowered to pike_cycle_depth");
    }
  }

  ref_to->flags = ref_to_flags;
  ref_to->la_count = ref_to_la_count;
  assert (ref_to->la_count >= old_la_count);
  if (ref_to->la_count > old_la_count) {
    mc_wq_enqueue (ref_to);
    MC_DEBUG_MSG (ref_to, "enqueued");
    mc_enqueued_noninternal = 1;
  }
  else
    MC_DEBUG_MSG (ref_to, "not enqueued");
}

static void pass_mark_external_visit_ref (void *thing, int UNUSED(ref_type),
					  visit_thing_fn *UNUSED(visit_fn), void *UNUSED(extra))
{
  struct mc_marker *ref_to = find_mc_marker (thing);

  assert (mc_pass == MC_PASS_MARK_EXTERNAL);

  if (ref_to) {
    if ((ref_to->flags & (MC_FLAG_INT_VISITED | MC_FLAG_LA_VISITED)) ==
	MC_FLAG_LA_VISITED) {
      /* Only interested in existing lookahead things, except those on
       * the "fringe" that haven't been visited. */

      if (IS_EXTERNAL (ref_to))
	MC_DEBUG_MSG (ref_to, "already external");
      else {
	FLAG_EXTERNAL (ref_to);
	DL_REMOVE (ref_to);
	DL_ADD_LAST (mc_indirect, ref_to);
	MC_DEBUG_MSG (ref_to, "marked external - moved to indirect list");
	assert (ref_to->int_refs + ref_to->la_refs == *(INT32 *) thing);
      }
    }
    else
      MC_DEBUG_MSG (ref_to, ref_to->flags & MC_FLAG_INTERNAL ?
		    "ignored internal" : "ignored fringe thing");
  }
}

static void current_only_visit_ref (void *thing, int ref_type,
				    visit_thing_fn *visit_fn, void *extra)
/* This is used when count_memory has a negative lookahead. It only
 * recurses through REF_TYPE_INTERNAL references. Note that most
 * fields in mc_marker aren't used. */
{
  int ref_from_flags;
  struct mc_marker *ref_to;

  assert (mc_pass);
  assert (mc_lookahead < 0);
#ifdef PIKE_DEBUG
  assert (mc_ref_from != (void *) (ptrdiff_t) -1);
#endif

  ref_from_flags = mc_ref_from->flags;
  assert (ref_from_flags & MC_FLAG_INTERNAL);
  assert (!(ref_from_flags & MC_FLAG_INT_VISITED));

#ifndef MEMORY_COUNT_DEBUG
  if (!(ref_type & REF_TYPE_INTERNAL)) {
    /* Return before lookup (or much worse, allocation) in the
       mc_marker hash table. The only reason to allocate a marker in
       this case is, AFAICS, to get the tracing right with
       MEMORY_COUNT_DEBUG enabled. That case is handled below. */
    return;
  }
#endif

  ref_to = find_mc_marker (thing);

  if (!ref_to) {
    ref_to = my_make_mc_marker (thing, visit_fn, extra);
    debug_malloc_touch(ref_to);
    ref_to->la_count = 0; /* initialize so the queue doesn't order on
                             uninitialized memory (... valgrind) */
    MC_DEBUG_MSG (ref_to, "got new thing");
  }
  else if (ref_to->flags & MC_FLAG_INTERNAL) {
    /* Ignore refs to the starting points. Can't treat them like other
     * things anyway since the int_refs aren't valid. */
    MC_DEBUG_MSG (ref_to, "ignored starting point");
    return;
  }
  else
    MC_DEBUG_MSG (ref_to, "got old thing");

#ifdef MEMORY_COUNT_DEBUG
  if (!(ref_type & REF_TYPE_INTERNAL)) {
    MC_DEBUG_MSG (ref_to, "ignored non-internal ref");
    return;
  }
#endif

  ref_to->int_refs++;
  MC_DEBUG_MSG (ref_to, "added really internal ref");
  assert (ref_to->int_refs <= *(INT32 *) thing);

  if (ref_to->int_refs == *(INT32 *) thing) {
    ref_to->flags |= MC_FLAG_INTERNAL;
    mc_wq_enqueue (ref_to);
    MC_DEBUG_MSG (ref_to, "enqueued internal");
  }
}

static void ignore_visit_enter(void *UNUSED(thing), int UNUSED(type), void *UNUSED(extra))
{
}

static void ignore_visit_leave(void *UNUSED(thing), int UNUSED(type), void *UNUSED(extra))
{
}

PMOD_EXPORT int mc_count_bytes (void *thing)
{
  if (mc_pass == MC_PASS_LOOKAHEAD) {
    struct mc_marker *m = find_mc_marker (thing);
#ifdef PIKE_DEBUG
    if (!m) Pike_fatal ("mc_marker not found for %p.\n", thing);
#endif
    MC_DEBUG_MSG(m, "mc_count_bytes (LA)");
    if ((m->flags & (MC_FLAG_INTERNAL|MC_FLAG_INT_VISITED)) == MC_FLAG_INTERNAL)
      return 1;
  }
  return 0;
}

/*! @decl int count_memory (int|mapping(string:int) options, @
 *!         mixed ... things)
 *! @appears Pike.count_memory
 *!
 *! In brief, if you call @expr{Pike.count_memory(0,x)@} you get back
 *! the number of bytes @expr{x@} occupies in memory.
 *!
 *! The detailed story is a bit longer:
 *!
 *! This function calculates the number of bytes that all @[things]
 *! occupy. Or put another way, it calculates the number of bytes that
 *! would be freed if all those things would lose their references at
 *! the same time, i.e. not only the memory in the things themselves,
 *! but also in all the things that are directly and indirectly
 *! referenced from those things and not from anywhere else.
 *!
 *! The memory counted is only that which is directly occupied by the
 *! things in question, including any overallocation for mappings,
 *! multisets and arrays. Other memory overhead that they give rise to
 *! is not counted. This means that if you would count the memory
 *! occupied by all the pike accessible things you would get a figure
 *! significantly lower than what the OS gives for the pike process.
 *!
 *! Also, if you were to actually free the things, you should not
 *! expect the size of the pike process to drop the amount of bytes
 *! returned by this function. That since Pike often retains the
 *! memory to be reused later.
 *!
 *! However, what you should expect is that if you actually free the
 *! things and then later allocates some more things for which this
 *! function returns the same size, there should be essentially no
 *! increase in the size of the pike process (some increase might
 *! occur due to internal fragmentation and memory pooling, but it
 *! should be small in general and over time).
 *!
 *! The search for things only referenced from @[things] can handle
 *! limited cyclic structures. That is done by doing a "lookahead",
 *! i.e. searching through referenced things that apparently have
 *! other outside references. You can control how long this lookahead
 *! should be through @[options] (see below). If the lookahead is too
 *! short to cover the cycles in a structure then a too low value is
 *! returned. If the lookahead is made gradually longer then the
 *! returned value will eventually become accurate and not increase
 *! anymore. If the lookahead is too long then unnecessary time might
 *! be spent searching through things that really have external
 *! references.
 *!
 *! Objects that are known to be part of cyclic structures are
 *! encouraged to have an integer constant or variable
 *! @expr{pike_cycle_depth@} that specifies the lookahead needed to
 *! discover those cycles. When @[Pike.count_memory] visits such
 *! objects, it uses that as the lookahead when going through the
 *! references emanating from them. Thus, assuming objects adhere to
 *! this convention, you should rarely have to specify a lookahead
 *! higher than zero to this function.
 *!
 *! Note that @expr{pike_cycle_depth@} can also be set to zero to
 *! effectively stop the lookahead from continuing through the object.
 *! That can be useful to put in objects you know have global
 *! references, to speed up the traversal.
 *!
 *! @param options
 *!   If this is an integer, it specifies the maximum lookahead
 *!   distance. -1 counts only the memory of the given @[things],
 *!   without following any references. 0 extends the count to all
 *!   their referenced things as long as there are no cycles (except
 *!   if @expr{pike_cycle_depth@} is found in objects - see above). 1
 *!   makes it cover cycles of length 1 (e.g. a thing points to
 *!   itself), 2 handles cycles of length 2 (e.g. where two things
 *!   point at each other), and so on.
 *!
 *!   However, the lookahead is by default blocked by programs, i.e.
 *!   it never follows references emanating from programs. That since
 *!   programs seldom are part of dynamic data structures, and they
 *!   also typically contain numerous references to global data which
 *!   would add a lot of work to the lookahead search.
 *!
 *!   To control the search in more detail, @[options] can be a
 *!   mapping instead:
 *!
 *!   @mapping
 *!     @member int lookahead
 *!       The maximum lookahead distance, as described above. Defaults
 *!       to 0 if missing.
 *!
 *!     @member int block_arrays
 *!     @member int block_mappings
 *!     @member int block_multisets
 *!     @member int block_objects
 *!     @member int block_programs
 *!       When any of these are given with a nonzero value, the
 *!       corresponding type is blocked when lookahead references are
 *!       followed. They are unblocked if the flag is given with a
 *!       zero value. Only programs are blocked by default.
 *!
 *!       These blocks are only active during the lookahead, so
 *!       blocked things are still recursed and memory counted if they
 *!       are given as arguments or only got internal references.
 *!
 *!     @member int block_strings
 *!       If positive then strings are always excluded (except any
 *!       given directly in @[things]), if negative they are always
 *!       included. Otherwise they are counted if they have no other
 *!       refs, but note that since strings are shared they might get
 *!       refs from other unrelated parts of the program.
 *!
 *!     @member int block_pike_cycle_depth
 *!       Do not heed @expr{pike_cycle_depth@} values found in
 *!       objects. This is implicit if the lookahead is negative.
 *!
 *!     @member int return_count
 *!       Return the number of things that memory was counted for,
 *!       instead of the byte count. (This is the same number
 *!       @expr{internal@} contains if @expr{collect_stats@} is set.)
 *!
 *!     @member int collect_internals
 *!       If this is nonzero then its value is replaced with an array
 *!       that contains the things that memory was counted for.
 *!
 *!     @member int collect_externals
 *!       If set then the value is replaced with an array containing
 *!       the things that were visited but turned out to have external
 *!       references (within the limited lookahead).
 *!
 *!     @member int collect_direct_externals
 *!       If set then the value is replaced with an array containing
 *!       the things found during the lookahead that (appears to) have
 *!       direct external references. This list is a subset of the
 *!       @expr{collect_externals@} list. It is useful if you get
 *!       unexpected global references to your data structure which
 *!       you want to track down.
 *!
 *!     @member int collect_stats
 *!       If this is nonzero then the mapping is extended with more
 *!       elements containing statistics from the search; see below.
 *!   @endmapping
 *!
 *!   When the @expr{collect_stats@} flag is set, the mapping is
 *!   extended with these elements:
 *!
 *!   @mapping
 *!     @member int internal
 *!       Number of things that were marked internal and hence memory
 *!       counted. It includes the things given as arguments.
 *!
 *!     @member int cyclic
 *!       Number of things that were marked internal only after
 *!       resolving cycles.
 *!
 *!     @member int external
 *!       Number of things that were visited through the lookahead but
 *!       were found to be external.
 *!
 *!     @member int visits
 *!       Number of times things were visited in total. This figure
 *!       includes visits to various internal things that aren't
 *!       visible from the pike level, so it might be larger than what
 *!       is apparently motivated by the numbers above.
 *!
 *!     @member int revisits
 *!       Number of times the same things were revisited. This can
 *!       occur in the lookahead when a thing is encountered through a
 *!       shorter path than the one it first got visited through. It
 *!       also occurs in resolved cycles. Like @expr{visits@}, this
 *!       count can include things that aren't visible from pike.
 *!
 *!     @member int rounds
 *!       Number of search rounds. This is usually 1 or 2. More rounds
 *!       are necessary only when blocked types turn out to be
 *!       (acyclic) internal, so that they need to be counted and
 *!       recursed anyway.
 *!
 *!     @member int work_queue_alloc
 *!       The number of elements that were allocated to store the work
 *!       queue which is used to keep track of the things to visit
 *!       during the lookahead. This is usually bigger than the
 *!       maximum number of things the queue actually held.
 *!
 *!     @member int size
 *!       The memory occupied by the internal things. This is the same
 *!       as the normal return value, but it's put here too for
 *!       convenience.
 *!   @endmapping
 *!
 *! @param things
 *!   One or more things to count memory size for. Only things passed
 *!   by reference are allowed, except for functions which are
 *!   forbidden because a meaningful size calculation can't be done
 *!   for them.
 *!
 *!   Integers are allowed because they are bignum objects when they
 *!   become sufficiently large. However, passing an integer that is
 *!   small enough to fit into the native integer type will return
 *!   zero.
 *!
 *! @returns
 *!   Returns the number of bytes occupied by the counted things. If
 *!   the @expr{return_count@} option is set then the number of things
 *!   are returned instead.
 *!
 *! @note
 *! The result of @expr{Pike.count_memory(0,a,b)@} might be larger
 *! than the sum of @expr{Pike.count_memory(0,a)@} and
 *! @expr{Pike.count_memory(0,b)@} since @expr{a@} and @expr{b@}
 *! together might reference things that aren't referenced from
 *! anywhere else.
 *!
 *! @note
 *! It's possible that a string that is referenced still isn't
 *! counted, because strings are always shared in Pike and the same
 *! string may be in use in some unrelated part of the program.
 */
void f_count_memory (INT32 args)
{
  struct svalue *collect_internal = NULL;
  unsigned count_internal, count_cyclic, count_visited;
  unsigned count_visits, count_revisits, count_rounds;
  int collect_stats = 0, return_count = 0;

  if (args < 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("count_memory", 1);

  mc_block_lookahead = mc_block_lookahead_default;
  mc_block_pike_cycle_depth = 0;
  mc_block_strings = 0;

  if (TYPEOF(Pike_sp[-args]) == T_MAPPING) {
    struct mapping *opts = Pike_sp[-args].u.mapping;
    struct pike_string *ind;
    struct svalue *val;

    MAKE_CONST_STRING (ind, "lookahead");
    if ((val = low_mapping_string_lookup (opts, ind))) {
      if (TYPEOF(*val) != T_INT)
	SIMPLE_ARG_ERROR ("count_memory", 1,
			  "\"lookahead\" must be an integer.");
      mc_lookahead =
	val->u.integer > (unsigned INT16) -1 ? (unsigned INT16) -1 :
	val->u.integer < 0 ? -1 :
	val->u.integer;
    }
    else
      mc_lookahead = 0;

#define CHECK_BLOCK_FLAG(NAME, TYPE_BIT) do {				\
      MAKE_CONST_STRING (ind, NAME);					\
      if ((val = low_mapping_string_lookup (opts, ind))) {		\
	if (UNSAFE_IS_ZERO (val))					\
	  mc_block_lookahead &= ~TYPE_BIT;				\
	else								\
	  mc_block_lookahead |= TYPE_BIT;				\
      }									\
    } while (0)
    CHECK_BLOCK_FLAG ("block_arrays", BIT_ARRAY);
    CHECK_BLOCK_FLAG ("block_mappings", BIT_MAPPING);
    CHECK_BLOCK_FLAG ("block_multisets", BIT_MULTISET);
    CHECK_BLOCK_FLAG ("block_objects", BIT_OBJECT);
    CHECK_BLOCK_FLAG ("block_programs", BIT_PROGRAM);

    MAKE_CONST_STRING (ind, "block_strings");
    if ((val = low_mapping_string_lookup (opts, ind))) {
      if (TYPEOF(*val) != T_INT)
	SIMPLE_ARG_ERROR ("count_memory", 1,
			  "\"block_strings\" must be an integer.");
      mc_block_strings = val->u.integer > 0 ? 1 : val->u.integer < 0 ? -1 : 0;
    }

    MAKE_CONST_STRING (ind, "block_pike_cycle_depth");
    if ((val = low_mapping_string_lookup (opts, ind)) && !UNSAFE_IS_ZERO (val))
      mc_block_pike_cycle_depth = 1;

    MAKE_CONST_STRING (ind, "return_count");
    if ((val = low_mapping_string_lookup (opts, ind)) && !UNSAFE_IS_ZERO (val))
      return_count = 1;

    MAKE_CONST_STRING (ind, "collect_internals");
    if ((val = low_mapping_string_lookup (opts, ind)) && !UNSAFE_IS_ZERO (val))
      collect_internal = Pike_sp; /* Value doesn't matter. */

    MAKE_CONST_STRING (ind, "collect_stats");
    if ((val = low_mapping_string_lookup (opts, ind)) && !UNSAFE_IS_ZERO (val))
      collect_stats = 1;
  }

  else {
    if (TYPEOF(Pike_sp[-args]) != T_INT)
      SIMPLE_ARG_TYPE_ERROR ("count_memory", 1, "int|mapping(string:int)");
    mc_lookahead =
      Pike_sp[-args].u.integer > (unsigned INT16) -1 ? (unsigned INT16) -1 :
      Pike_sp[-args].u.integer < 0 ? -1 :
      Pike_sp[-args].u.integer;
  }

  start_mc();

  if (TYPEOF(pike_cycle_depth_str) == PIKE_T_FREE) {
    SET_SVAL_TYPE(pike_cycle_depth_str, T_STRING);
    MAKE_CONST_STRING (pike_cycle_depth_str.u.string, "pike_cycle_depth");
  }

  assert (mc_work_queue == NULL);
  /* NB: Add 1 to size to compensate for 1-based indexing. */
  mc_work_queue = malloc ((MC_WQ_START_SIZE + 1) * sizeof (mc_work_queue[0]));
  if (!mc_work_queue) {
    stop_mc();
    SIMPLE_OUT_OF_MEMORY_ERROR ("Pike.count_memory",
				(MC_WQ_START_SIZE + 1) *
				sizeof (mc_work_queue[0]));
  }
  mc_wq_size = MC_WQ_START_SIZE;
  mc_wq_used = 1;

  assert (!mc_pass);
  assert (visit_enter == NULL);
  assert (visit_ref == NULL);
  assert (visit_leave == NULL);

  free_svalue (&throw_value);
  mark_free_svalue (&throw_value);

  {
    int i;
    for (i = -args + 1; i < 0; i++) {
      struct svalue *s = Pike_sp + i;

      if (TYPEOF(*s) == T_INT)
	continue;

      else if (!REFCOUNTED_TYPE(TYPEOF(*s))) {
	/* Not refcounted. Replace with 0 and ignore. */
	SET_SVAL(*s, T_INT, NUMBER_NUMBER, integer, 0);
	continue;
      }
      else {
	if (TYPEOF(*s) == T_FUNCTION) {
	  struct svalue s2;
	  if ((s2.u.program = program_from_function (s))) {
	    add_ref (s2.u.program);
	    SET_SVAL_TYPE(s2, T_PROGRAM);
	    free_svalue (s);
	    move_svalue (s, &s2);
	  } else if (SUBTYPEOF(*s) == FUNCTION_BUILTIN) {
	    free_svalue(s);
	    SET_SVAL(*s, T_INT, NUMBER_NUMBER, integer, 0);
	    continue;
	  } else {
	    /* Convert from function to object. */
	    SET_SVAL_TYPE_SUBTYPE(*s, T_OBJECT, 0);
	  }
	}

	if (!visit_fn_from_type[TYPEOF(*s)]) {
	  /* Unsupported type. Replace with 0 and ignore. */
	  free_svalue(s);
	  SET_SVAL(*s, T_INT, NUMBER_NUMBER, integer, 0);
	  continue;
	}

	if (find_mc_marker (s->u.ptr)) {
	  /* The user passed the same thing several times. Ignore it. */
	}

	else {
	  struct mc_marker *m =
	    my_make_mc_marker (s->u.ptr, visit_fn_from_type[TYPEOF(*s)], NULL);
	  debug_malloc_touch(m);
	  m->flags |= MC_FLAG_INTERNAL;
	  if (!mc_block_pike_cycle_depth && TYPEOF(*s) == T_OBJECT) {
	    int cycle_depth = mc_cycle_depth_from_obj (s->u.object);
	    if (TYPEOF(throw_value) != PIKE_T_FREE) {
	      free(mc_work_queue);
	      mc_work_queue = NULL;
	      stop_mc();
	      throw_severity = THROW_ERROR;
	      pike_throw();
	    }
	    m->la_count = cycle_depth >= 0 ? cycle_depth : mc_lookahead;
	  }
	  else
	    m->la_count = mc_lookahead;
	  mc_wq_enqueue (m);
	  MC_DEBUG_MSG (m, "enqueued starting point");
	}
      }
    }
  }

  if (collect_internal) {
    check_stack (120);
    AGGR_ARR_PROLOGUE (collect_internal, args + 10);
    args++;
  }

  assert (mc_incomplete.dl_prev == &mc_incomplete);
  assert (mc_incomplete.dl_next == &mc_incomplete);
  assert (mc_complete.dl_prev == &mc_complete);
  assert (mc_complete.dl_next == &mc_complete);
#ifdef PIKE_DEBUG
  assert (mc_ref_from == (void *) (ptrdiff_t) -1);
#endif

  mc_counted_bytes = 0;
  count_internal = count_cyclic = count_visited = 0;
  count_visits = count_revisits = count_rounds = 0;

  visit_enter = ignore_visit_enter;
  visit_ref = mc_lookahead < 0 ?
    current_only_visit_ref : pass_lookahead_visit_ref;
  visit_leave = ignore_visit_leave;

  do {
    count_rounds++;
    mc_enqueued_noninternal = 0;

#ifdef MEMORY_COUNT_DEBUG
    fprintf (stderr, "[%d] MC_PASS_LOOKAHEAD\n", count_rounds);
#endif
    mc_pass = MC_PASS_LOOKAHEAD;

    while ((mc_ref_from = mc_wq_dequeue())) {
      int action;

      assert (!(mc_ref_from->flags & MC_FLAG_INT_VISITED));

      if (mc_ref_from->flags & MC_FLAG_INTERNAL) {
	action = VISIT_COUNT_BYTES; /* Memory count this. */
	MC_DEBUG_MSG (NULL, "enter with byte counting");
	if (mc_lookahead < 0) {
	  MC_DEBUG_MSG (NULL, "VISIT_NO_REFS mode");
	  action |= VISIT_NO_REFS;
	}

	mc_ref_from->visit_fn (mc_ref_from->thing, action, mc_ref_from->extra);
	count_visits++;

	if (mc_ref_from->flags & MC_FLAG_LA_VISITED) {
	  count_revisits++;
	  DL_REMOVE (mc_ref_from);
	  MC_DEBUG_MSG (NULL, "leave - removed from list");
	}
	else {
	  if (collect_stats &&
	      type_from_visit_fn (mc_ref_from->visit_fn) <= MAX_TYPE)
	    count_visited++;
	  MC_DEBUG_MSG (NULL, "leave");
	}

	mc_ref_from->flags |= MC_FLAG_INT_VISITED;

	if (return_count || collect_stats || collect_internal) {
	  TYPE_T type = type_from_visit_fn (mc_ref_from->visit_fn);
	  if (type <= MAX_TYPE) {
	    count_internal++;
	    if (collect_internal) {
	      SET_SVAL(*Pike_sp, type, 0, ptr, mc_ref_from->thing);
	      add_ref ((struct ref_dummy *) mc_ref_from->thing);
	      dmalloc_touch_svalue (Pike_sp);
	      Pike_sp++;
	      AGGR_ARR_CHECK (collect_internal, 120);
	    }
	  }
	}
      }

      else {
	assert (mc_lookahead >= 0);
	action = VISIT_NORMAL;
	MC_DEBUG_MSG (NULL, "enter");

	mc_ref_from->visit_fn (mc_ref_from->thing, action, mc_ref_from->extra);
	count_visits++;

	if (mc_ref_from->flags & MC_FLAG_LA_VISITED) {
	  count_revisits++;
	  MC_DEBUG_MSG (NULL, "leave (revisit)");
	}

	else {
	  if (collect_stats &&
	      type_from_visit_fn (mc_ref_from->visit_fn) <= MAX_TYPE)
	    count_visited++;

	  mc_ref_from->flags |= MC_FLAG_LA_VISITED;

	  /* The reason for fixing the lists here is to avoid putting
	   * the "fringe" things that we never visit onto them. */
	  if (mc_ref_from->int_refs + mc_ref_from->la_refs <
	      *(INT32 *) mc_ref_from->thing) {
	    DL_ADD_LAST (mc_incomplete, mc_ref_from);
	    MC_DEBUG_MSG (NULL, "leave - added to incomplete list");
	  }
	  else {
	    DL_ADD_LAST (mc_complete, mc_ref_from);
	    MC_DEBUG_MSG (NULL, "leave - added to complete list");
	  }
	}
      }

      if (TYPEOF(throw_value) != PIKE_T_FREE) {
	free(mc_work_queue);
	mc_work_queue = NULL;
	stop_mc();
	throw_severity = THROW_ERROR;
	pike_throw();
      }
    }
#if defined (PIKE_DEBUG) || defined (MEMORY_COUNT_DEBUG)
    mc_ref_from = (void *) (ptrdiff_t) -1;
#endif

    /* If no things that might be indirectly incomplete have been
     * enqueued then there's no need to do another mark external pass. */
    if (!mc_enqueued_noninternal) {
      DL_MAKE_EMPTY (mc_complete);
      break;
    }

    if (mc_lookahead < 0) {
      assert (mc_incomplete.dl_prev == &mc_incomplete);
      assert (mc_incomplete.dl_next == &mc_incomplete);
      assert (mc_complete.dl_prev == &mc_complete);
      assert (mc_complete.dl_next == &mc_complete);
      break;
    }

#ifdef MEMORY_COUNT_DEBUG
    fprintf (stderr, "[%d] MC_PASS_MARK_EXTERNAL, "
	     "traversing the incomplete list\n", count_rounds);
#endif
    mc_pass = MC_PASS_MARK_EXTERNAL;
    visit_ref = pass_mark_external_visit_ref;

    assert (mc_indirect.dl_next == &mc_indirect);
    assert (mc_indirect.dl_prev == &mc_indirect);

    {
      struct mc_marker *m, *list;

      for (m = mc_incomplete.dl_next; m != &mc_incomplete; m = m->dl_next)
	FLAG_EXTERNAL (m);

      list = &mc_incomplete;
      while (1) {
	/* First go through the incomplete list to visit externals,
	 * then the indirectly incomplete list where all the new
	 * indirect externals appear. */
	for (m = list->dl_next; m != list; m = m->dl_next) {
	  TYPE_T type = type_from_visit_fn (m->visit_fn);
	  assert (!(m->flags & MC_FLAG_INTERNAL));
	  assert (m->flags & MC_FLAG_LA_VISITED);
	  assert (list != &mc_incomplete || !(m->flags & MC_FLAG_CANDIDATE));
	  if (mc_lookahead_blocked(type))
	    MC_DEBUG_MSG (m, "type is blocked - not visiting");
	  else {
#ifdef MEMORY_COUNT_DEBUG
	    mc_ref_from = m;
	    MC_DEBUG_MSG (NULL, "visiting external");
#endif
	    count_visits++;
	    count_revisits++;
	    m->visit_fn (m->thing, VISIT_NORMAL, m->extra);
	  }
	}
	if (list == &mc_incomplete) {
	  list = &mc_indirect;
#ifdef MEMORY_COUNT_DEBUG
	  fprintf (stderr, "[%d] MC_PASS_MARK_EXTERNAL, "
		   "traversing the indirect list\n", count_rounds);
#endif
	}
	else break;
      }

#if defined (PIKE_DEBUG) || defined (MEMORY_COUNT_DEBUG)
      mc_ref_from = (void *) (ptrdiff_t) -1;
#endif
    }

    if (DL_IS_EMPTY (mc_complete)) break;

#ifdef MEMORY_COUNT_DEBUG
    fprintf (stderr, "[%d] MC_PASS_MARK_EXTERNAL, "
	     "enqueuing cyclic internals\n", count_rounds);
#endif

    {
      /* We've found some internal cyclic stuff. Put it in the work
       * list for the next round. */
      struct mc_marker *m = mc_complete.dl_next;
      assert (m != &mc_complete);
      do {
	assert (!(m->flags & (MC_FLAG_INTERNAL | MC_FLAG_INT_VISITED)));
	m->flags |= MC_FLAG_INTERNAL;
	assert (m->flags & (MC_FLAG_CANDIDATE | MC_FLAG_LA_VISITED));
	assert (!(mc_lookahead_blocked(type_from_visit_fn (m->visit_fn))));
	/* The following assertion implies that the lookahead count
	 * already has been raised as it should. */
	assert (m->flags & MC_FLAG_CANDIDATE_REF);
	mc_wq_enqueue (m);
	if (collect_stats && type_from_visit_fn (m->visit_fn) <= MAX_TYPE)
	  count_cyclic++;
	MC_DEBUG_MSG (m, "enqueued cyclic internal");
	m = m->dl_next;
      } while (m != &mc_complete);
    }

    /* We've moved all the markers on mc_complete to the work queue,
     * so we need to empty mc_complete in order to use it again for
     * the next batch of indirect markers.
     */
    DL_MAKE_EMPTY(mc_complete);
    DL_MOVE (mc_indirect, mc_complete);

    TOGGLE_EXT_FLAGS();

#ifdef PIKE_DEBUG
    if (d_flag) {
      struct mc_marker *m;
      for (m = mc_incomplete.dl_next; m != &mc_incomplete; m = m->dl_next) {
	assert (!(m->flags & MC_FLAG_INTERNAL));
	assert (!IS_EXTERNAL (m));
      }
      for (m = mc_complete.dl_next; m != &mc_complete; m = m->dl_next) {
	assert (!(m->flags & MC_FLAG_INTERNAL));
	assert (!IS_EXTERNAL (m));
      }
    }
#endif

    /* Prepare for next MC_PASS_LOOKAHEAD round. */
    visit_ref = pass_lookahead_visit_ref;
  } while (1);

#ifdef MEMORY_COUNT_DEBUG
  fputs ("memory counting done\n", stderr);
#endif

#if 0
  fprintf (stderr, "count_memory stats: %u internal, %u cyclic, %u external\n"
	   "count_memory stats: %u visits, %u revisits, %u rounds\n",
	   count_internal, count_cyclic,
	   count_visited - count_internal,
	   count_visits, count_revisits, count_rounds);
#ifdef PIKE_DEBUG
  {
    size_t num, size;
    count_memory_in_mc_markers (&num, &size);
    fprintf (stderr, "count_memory used %"PRINTSIZET"u bytes "
	     "for %"PRINTSIZET"u markers.\n", size, num);
  }
#endif
#endif

  if (collect_internal) {
    struct pike_string *ind;
    AGGR_ARR_EPILOGUE (collect_internal);
    MAKE_CONST_STRING (ind, "collect_internals");
    mapping_string_insert (Pike_sp[-args].u.mapping, ind, Pike_sp - 1);
  }

  if (TYPEOF(Pike_sp[-args]) == T_MAPPING) {
    struct mapping *opts = Pike_sp[-args].u.mapping;
    struct pike_string *ind;
    struct svalue *val;

    MAKE_CONST_STRING (ind, "collect_stats");
    if ((val = low_mapping_string_lookup (opts, ind)) &&
	!UNSAFE_IS_ZERO (val)) {
#define INSERT_STAT(NAME, VALUE) do {					\
	struct pike_string *ind;					\
	push_ulongest (VALUE);						\
	MAKE_CONST_STRING (ind, NAME);					\
	mapping_string_insert (opts, ind, Pike_sp - 1);			\
	pop_stack();							\
      } while (0)
      INSERT_STAT ("internal", count_internal);
      INSERT_STAT ("cyclic", count_cyclic);
      INSERT_STAT ("external", count_visited - count_internal);
      INSERT_STAT ("visits", count_visits);
      INSERT_STAT ("revisits", count_revisits);
      INSERT_STAT ("rounds", count_rounds);
      INSERT_STAT ("work_queue_alloc", mc_wq_size);
      INSERT_STAT ("size", mc_counted_bytes);
    }

    MAKE_CONST_STRING (ind, "collect_externals");
    if ((val = low_mapping_string_lookup (opts, ind)) &&
	!UNSAFE_IS_ZERO (val)) {
      BEGIN_AGGREGATE_ARRAY (count_visited - count_internal) {
	struct mc_marker *m, *list = &mc_incomplete;
	while (1) {
	  /* Collect things from the mc_incomplete and mc_indirect lists. */
	  for (m = list->dl_next; m != list; m = m->dl_next) {
	    TYPE_T type = type_from_visit_fn (m->visit_fn);
	    assert (!(m->flags & MC_FLAG_INTERNAL));
	    assert (m->flags & MC_FLAG_LA_VISITED);
	    if (type <= MAX_TYPE) {
	      SET_SVAL(*Pike_sp, type, 0, ptr, m->thing);
	      add_ref ((struct ref_dummy *) m->thing);
	      dmalloc_touch_svalue (Pike_sp);
	      Pike_sp++;
	      DO_AGGREGATE_ARRAY (120);
	    }
	  }
	  if (list == &mc_incomplete) list = &mc_indirect;
	  else break;
	}
      } END_AGGREGATE_ARRAY;
      args++;
      mapping_string_insert (opts, ind, Pike_sp - 1);
    }

    MAKE_CONST_STRING (ind, "collect_direct_externals");
    if ((val = low_mapping_string_lookup (opts, ind)) &&
	!UNSAFE_IS_ZERO (val)) {
      BEGIN_AGGREGATE_ARRAY (count_visited - count_internal) {
	/* Collect things from the mc_incomplete list. */
	struct mc_marker *m;
	for (m = mc_incomplete.dl_next; m != &mc_incomplete; m = m->dl_next) {
	  TYPE_T type = type_from_visit_fn (m->visit_fn);
	  assert (!(m->flags & MC_FLAG_INTERNAL));
	  assert (m->flags & MC_FLAG_LA_VISITED);
	  if (type <= MAX_TYPE) {
	    SET_SVAL(*Pike_sp, type, 0, ptr, m->thing);
	    add_ref ((struct ref_dummy *) m->thing);
	    dmalloc_touch_svalue (Pike_sp);
	    Pike_sp++;
	    DO_AGGREGATE_ARRAY (120);
	  }
	}
      } END_AGGREGATE_ARRAY;
      args++;
      mapping_string_insert (opts, ind, Pike_sp - 1);
    }
  }

  mc_pass = 0;
  visit_enter = NULL;
  visit_ref = NULL;
  visit_leave = NULL;

  DL_MAKE_EMPTY (mc_incomplete);
  DL_MAKE_EMPTY (mc_indirect);
#ifdef DO_PIKE_CLEANUP
#endif

  assert (mc_wq_used == 1);
  free(mc_work_queue);
  mc_work_queue = NULL;
  stop_mc();

  pop_n_elems (args);
  push_ulongest (return_count ? count_internal : mc_counted_bytes);
}

static struct mapping *identify_loop_reverse = NULL;

void identify_loop_visit_enter(void *thing, int type, void *UNUSED(extra))
{
  if (type < T_VOID) {
    /* Valid svalue type. */
    SET_SVAL(*Pike_sp, type, 0, refs, thing);
    add_ref(((struct array *)thing));
    Pike_sp++;
  }
}

void identify_loop_visit_ref(void *dst, int UNUSED(ref_type),
			     visit_thing_fn *visit_dst,
			     void *extra)
{
  int type = type_from_visit_fn(visit_dst);
  struct mc_marker *ref_to = find_mc_marker(dst);
  if (ref_to) {
    /* Already visited or queued for visiting. */
    return;
  }

  ref_to = my_make_mc_marker(dst, visit_dst, extra);
  debug_malloc_touch(ref_to);
  ref_to->la_count = 0; /* initialize just so the queue doesn't order on
                           uninitialized memory (... valgrind) */

  MC_DEBUG_MSG(ref_to, "identify_loop_visit_ref()");

  if (type != PIKE_T_UNKNOWN) {
    /* NB: low_mapping_insert() for object indices may throw errors
     *     if eg lfun::`==() throws an error. We therefore instead
     *     use the raw pointers as indices instead.
     */
    struct svalue s;
    SET_SVAL(s, PIKE_T_INT, NUMBER_NUMBER, integer, (INT_TYPE)(ptrdiff_t)dst);
    mc_wq_enqueue(ref_to);
    low_mapping_insert(identify_loop_reverse, &s, Pike_sp-1, 0);
  } else {
    /* Not a valid svalue type.
     *
     * Probably T_MAPPING_DATA or T_MULTISET_DATA or similar.
     *
     * Recurse directly while we have the containing thing on the stack.
     */
    ref_to->flags |= MC_FLAG_INT_VISITED;
    visit_dst(dst, VISIT_COMPLEX_ONLY, extra);
  }
}

void identify_loop_visit_leave(void *UNUSED(thing), int type, void *UNUSED(extra))
{
  if (type < T_VOID) {
    /* Valid svalue type. */
    pop_stack();
  }
}

/*! @decl array(mixed) identify_cycle(mixed x)
 *! @belongs Pike
 *!
 *! Identify reference cycles in Pike datastructures.
 *!
 *! This function is typically used to identify why certain
 *! datastructures need the @[gc] to run to be freed.
 *!
 *! @param x
 *!   Value that is believed to be involved in a reference cycle.
 *!
 *! @returns
 *!   @mixed
 *!     @type zero
 *!       Returns @expr{UNDEFINED@} if @[x] is not member of a reference cycle.
 *!     @type array(mixed)
 *!       Otherwise returns an array identifying a cycle with @[x] as the first
 *!       element, and where the elements refer to each other in order, and the
 *!       last element refers to the first.
 *!   @endmixed
 */
void f_identify_cycle(INT32 args)
{
  struct svalue *s;
  struct svalue *k;

  if (args != 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR("identify_loops", 1);

  s = Pike_sp - 1;

  if (!REFCOUNTED_TYPE(TYPEOF(*s))) {
    SIMPLE_ARG_TYPE_ERROR("identify_loops", 1,
			  "array|multiset|mapping|object|program|string|type");
  }
  if (TYPEOF(*s) == T_FUNCTION) {
    if (SUBTYPEOF(*s) == FUNCTION_BUILTIN) {
      SIMPLE_ARG_TYPE_ERROR("identify_loops", 1,
			    "array|multiset|mapping|object|program|string|type");
    }
    SET_SVAL_TYPE(*s, T_OBJECT);
  }

  start_mc();

  if (TYPEOF(pike_cycle_depth_str) == PIKE_T_FREE) {
    SET_SVAL_TYPE(pike_cycle_depth_str, T_STRING);
    MAKE_CONST_STRING (pike_cycle_depth_str.u.string, "pike_cycle_depth");
  }

  assert (mc_work_queue == NULL);
  /* NB: Add 1 to size to compensate for 1-based indexing. */
  mc_work_queue = malloc ((MC_WQ_START_SIZE + 1) * sizeof (mc_work_queue[0]));
  if (!mc_work_queue) {
    stop_mc();
    SIMPLE_OUT_OF_MEMORY_ERROR ("Pike.count_memory",
				(MC_WQ_START_SIZE + 1) * sizeof (mc_work_queue[0]));
  }
  mc_wq_size = MC_WQ_START_SIZE;
  mc_wq_used = 1;
  mc_lookahead = -1;

  assert (!mc_pass);
  assert (visit_enter == NULL);
  assert (visit_ref == NULL);
  assert (visit_leave == NULL);

  /* There's a fair chance of there being lots of stuff being referenced,
   * so preallocate a reasonable initial size.
   */
  identify_loop_reverse = allocate_mapping(1024);

  visit_enter = identify_loop_visit_enter;
  visit_ref = identify_loop_visit_ref;
  visit_leave = identify_loop_visit_leave;

  /* NB: This initial call will botstrap the wq_queue. */
  visit_fn_from_type[TYPEOF(*s)](s->u.ptr, VISIT_COMPLEX_ONLY, NULL);

#ifdef PIKE_DEBUG
  assert (mc_ref_from == (void *) (ptrdiff_t) -1);
#endif

  while ((mc_ref_from = mc_wq_dequeue())) {
    if (mc_ref_from->flags & MC_FLAG_INT_VISITED) continue;

    mc_ref_from->flags |= MC_FLAG_INT_VISITED;
    mc_ref_from->visit_fn(mc_ref_from->thing, VISIT_COMPLEX_ONLY, NULL);
  }

#if defined (PIKE_DEBUG) || defined (MEMORY_COUNT_DEBUG)
  mc_ref_from = (void *) (ptrdiff_t) -1;
#endif

  free(mc_work_queue);
  mc_work_queue = NULL;

  visit_enter = NULL;
  visit_ref = NULL;
  visit_leave = NULL;

#ifdef PIKE_DEBUG
  if (s != Pike_sp-1) {
    Pike_fatal("Stack error in identify_loops.\n");
  }
#endif

  /* NB: low_mapping_lookup() for object indices may throw errors
   *     if eg lfun::`==() throws an error. We therefore instead
   *     use the raw pointers as indices instead.
   */
  push_int((INT_TYPE)(ptrdiff_t)s->u.refs);
  while ((k = low_mapping_lookup(identify_loop_reverse, Pike_sp-1))) {
    /* NB: Since we entered this loop, we know that there's a
     *     reference loop involving s, as s otherwise wouldn't
     *     have been in the mapping.
     */
    pop_stack();
    push_svalue(k);
    push_int((INT_TYPE)(ptrdiff_t)k->u.refs);
    if (k->u.refs == s->u.refs) {
      /* Found! */
      pop_stack();
      break;
    }
  }

  free_mapping(identify_loop_reverse);

  stop_mc();

  if (!k) {
    push_undefined();
  } else {
    /* NB: We push s an extra time last above, to simplify the
     *     reversing below.
     */
    f_aggregate(Pike_sp - (s + 1));
    f_reverse(1);
  }
}

void init_mc(void)
{
  init_interleave_mutex(&mc_mutex);
}

void exit_mc(void)
{
  exit_interleave_mutex(&mc_mutex);
}
