/*
 * $Id: gc.h,v 1.77 2003/09/08 15:27:58 mast Exp $
 */
#ifndef GC_H
#define GC_H

#include "global.h"
#include "callback.h"
#include "queue.h"
#include "threads.h"
#include "interpret.h"

/* #define GC_MARK_DEBUG */

extern INT32 num_objects;
extern INT32 num_allocs;
extern ptrdiff_t alloc_threshold;
PMOD_EXPORT extern int Pike_in_gc;
extern int gc_trace, gc_debug;

extern struct callback *gc_evaluator_callback;
#ifdef PIKE_DEBUG
extern void *gc_svalue_location;
#endif

#define ADD_GC_CALLBACK() do { if(!gc_evaluator_callback)  gc_evaluator_callback=add_to_callback(&evaluator_callbacks,(callback_func)do_gc,0,0); }while(0)

#define LOW_GC_ALLOC(OBJ) do {						\
 extern int d_flag;							\
 num_objects++;								\
 num_allocs++;								\
 DO_IF_DEBUG(								\
   if(d_flag) CHECK_INTERPRETER_LOCK();					\
   if(Pike_in_gc > GC_PASS_PREPARE && Pike_in_gc < GC_PASS_FREE)	\
     fatal("Allocating new objects within gc is not allowed!\n");	\
 )									\
 if (Pike_in_gc) remove_marker(OBJ);					\
} while (0)

#ifdef ALWAYS_GC
#define GC_ALLOC(OBJ) do{						\
  LOW_GC_ALLOC(OBJ);							\
  ADD_GC_CALLBACK();				\
} while(0)
#else
#define GC_ALLOC(OBJ)  do{						\
  LOW_GC_ALLOC(OBJ);							\
  if(num_allocs == alloc_threshold)		\
    ADD_GC_CALLBACK();							\
} while(0)
#endif

#ifdef PIKE_DEBUG

/* Use this when freeing blocks that you've used any gc_check or
 * gc_mark function on and that can't contain references. */
#define GC_FREE_SIMPLE_BLOCK(PTR) do {					\
  extern int d_flag;							\
  if(d_flag) CHECK_INTERPRETER_LOCK();					\
  if (Pike_in_gc == GC_PASS_CHECK)					\
    fatal("No free is allowed in this gc pass.\n");			\
  else									\
    remove_marker(PTR);							\
} while (0)

/* Use this when freeing blocks that you've used any gc_check or
 * gc_mark function on and that can contain references. */
#define GC_FREE_BLOCK(PTR) do {						\
  extern int d_flag;							\
  if(d_flag) CHECK_INTERPRETER_LOCK();					\
  if (Pike_in_gc > GC_PASS_PREPARE && Pike_in_gc < GC_PASS_FREE)	\
    fatal("Freeing objects within gc is not allowed.\n");		\
} while (0)

#else
#define GC_FREE_SIMPLE_BLOCK(PTR) do {} while (0)
#define GC_FREE_BLOCK(PTR) do {} while (0)
#endif

#define GC_FREE(PTR) do {						\
  GC_FREE_BLOCK(PTR);							\
  DO_IF_DEBUG(								\
    if(num_objects < 1)							\
      fatal("Panic!! less than zero objects!\n");			\
  );									\
  num_objects-- ;							\
}while(0)

struct gc_frame;

struct marker
{
  struct marker *next;
  struct gc_frame *frame;	/* Pointer into the cycle check stack. */
  void *data;
  INT32 refs;			/* Internal references. */
  INT32 weak_refs;		/* Weak (implying internal) references. */
#ifdef PIKE_DEBUG
  INT32 xrefs;			/* Known external references. */
  INT32 saved_refs;		/* Object refcount during check pass. */
#endif
  unsigned INT16 flags;
};

#define GC_MARKED		0x0001
#define GC_NOT_REFERENCED	0x0002
#define GC_CYCLE_CHECKED	0x0004
#define GC_LIVE			0x0008
#define GC_LIVE_OBJ		0x0010
#define GC_LIVE_RECURSE		0x0020
#define GC_GOT_DEAD_REF		0x0040
#define GC_FREE_VISITED		0x0080

#define GC_PRETOUCHED		0x0100
#define GC_MIDDLETOUCHED	0x0200
#ifdef PIKE_DEBUG
#define GC_IS_REFERENCED	0x0400
#define GC_XREFERENCED		0x0800
#define GC_DO_FREE		0x1000
#define GC_GOT_EXTRA_REF	0x2000
#define GC_WEAK_FREED		0x4000
#define GC_CHECKED_AS_WEAK	0x8000
#endif

#ifdef PIKE_DEBUG
#define get_marker debug_get_marker
#define find_marker debug_find_marker
#endif

#include "block_alloc_h.h"
PTR_HASH_ALLOC(marker,MARKER_CHUNK_SIZE)

#ifdef PIKE_DEBUG
#undef get_marker
#define get_marker(X) ((struct marker *) debug_malloc_pass(debug_get_marker(X)))
#undef find_marker
#define find_marker(X) ((struct marker *) debug_malloc_pass(debug_find_marker(X)))
#endif

extern size_t gc_ext_weak_refs;

typedef void gc_cycle_check_cb (void *data, int weak);

/* Prototypes begin here */
struct gc_frame;
struct callback *debug_add_gc_callback(callback_func call,
				 void *arg,
				 callback_func free_func);
void dump_gc_info(void);
TYPE_T attempt_to_identify(void *something);
void describe_location(void *real_memblock,
		       int real_type,
		       void *location,
		       int indent,
		       int depth,
		       int flags);
void debug_gc_fatal(void *a, int flags, const char *fmt, ...);
void debug_gc_set_where (TYPE_T type, void *data);
void debug_gc_xmark_svalues(struct svalue *s, ptrdiff_t num, char *fromwhere);
void debug_gc_check_svalues(struct svalue *s, ptrdiff_t num, TYPE_T t, void *data);
void debug_gc_check_weak_svalues(struct svalue *s, ptrdiff_t num, TYPE_T t, void *data);
void debug_gc_check_short_svalue(union anything *u, TYPE_T type, TYPE_T t, void *data);
void debug_gc_check_weak_short_svalue(union anything *u, TYPE_T type, TYPE_T t, void *data);
int debug_low_gc_check(void *x, TYPE_T t, void *data);
void low_describe_something(void *a,
			    int t,
			    int indent,
			    int depth,
			    int flags);
void describe_something(void *a, int t, int indent, int depth, int flags);
PMOD_EXPORT void describe(void *x);
void debug_describe_svalue(struct svalue *s);
void debug_gc_touch(void *a);
PMOD_EXPORT INT32 real_gc_check(void *a);
INT32 real_gc_check_weak(void *a);
void locate_references(void *a);
void gc_add_extra_ref(void *a);
void gc_free_extra_ref(void *a);
int debug_gc_is_referenced(void *a);
int gc_external_mark3(void *a, void *in, char *where);
void debug_really_free_gc_frame(struct gc_frame *l);
int gc_do_weak_free(void *a);
void gc_delayed_free(void *a, TYPE_T type);
int gc_mark(void *a);
PMOD_EXPORT void gc_cycle_enqueue(gc_cycle_check_cb *checkfn, void *data, int weak);
void gc_cycle_run_queue();
int gc_cycle_push(void *x, struct marker *m, int weak);
void do_gc_recurse_svalues(struct svalue *s, int num);
void do_gc_recurse_short_svalue(union anything *u, TYPE_T type);
int gc_do_free(void *a);
int do_gc(void);
void f__gc_status(INT32 args);
void cleanup_gc(void);

#ifdef GC_MARK_DEBUG

TYPE_FIELD debug_gc_mark_svalues (struct svalue *s, ptrdiff_t num,
				  TYPE_T in_type, void *in);
TYPE_FIELD debug_gc_mark_weak_svalues (struct svalue *s, ptrdiff_t num,
				       TYPE_T in_type, void *in);
int debug_gc_mark_short_svalue (union anything *u, TYPE_T type,
				TYPE_T in_type, void *in);
int debug_gc_mark_weak_short_svalue (union anything *u, TYPE_T type,
				     TYPE_T in_type, void *in);

void gc_mark_enqueue (queue_call fn, void *data);
void gc_mark_run_queue();
void gc_mark_discard_queue();

#else  /* !GC_MARK_DEBUG */

#define debug_gc_mark_svalues(S, NUM, IN_TYPE, IN)			\
  gc_mark_svalues ((S), (NUM))
#define debug_gc_mark_weak_svalues(S, NUM, IN_TYPE, IN)			\
  gc_mark_weak_svalues ((S), (NUM))
#define debug_gc_mark_short_svalue(U, TYPE, IN_TYPE, IN)		\
  gc_mark_short_svalue ((U), (TYPE))
#define debug_gc_mark_weak_short_svalue(U, TYPE, IN_TYPE, IN)		\
  gc_mark_weak_short_svalue ((U), (TYPE))

extern struct pike_queue gc_mark_queue;
#define gc_mark_enqueue(FN, DATA) enqueue (&gc_mark_queue, (FN), (DATA))
#define gc_mark_run_queue() run_queue (&gc_mark_queue)
#define gc_mark_discard_queue() discard_queue (&gc_mark_queue)

#endif	/* !GC_MARK_DEBUG */

#define gc_fatal \
  fprintf(stderr, "%s:%d: GC fatal:\n", __FILE__, __LINE__), debug_gc_fatal

#ifdef PIKE_DEBUG
#define gc_checked_as_weak(X) (get_marker(X)->flags |= GC_CHECKED_AS_WEAK)
#define gc_assert_checked_as_weak(X) do {				\
  if (!(find_marker(X)->flags & GC_CHECKED_AS_WEAK))			\
    fatal("A thing was checked as nonweak but "				\
	  "marked or cycle checked as weak.\n");			\
} while (0)
#define gc_assert_checked_as_nonweak(X) do {				\
  if (find_marker(X)->flags & GC_CHECKED_AS_WEAK)			\
    fatal("A thing was checked as weak but "				\
	  "marked or cycle checked as nonweak.\n");			\
} while (0)
#else
#define gc_checked_as_weak(X) 0
#define gc_assert_checked_as_weak(X) do {} while (0)
#define gc_assert_checked_as_nonweak(X) do {} while (0)
#endif

#if defined (PIKE_DEBUG) && defined (DEBUG_MALLOC)
#define DMALLOC_TOUCH_MARKER(X, EXPR) (get_marker(X), (EXPR))
#else
#define DMALLOC_TOUCH_MARKER(X, EXPR) (EXPR)
#endif

#define gc_check(VP) \
  DMALLOC_TOUCH_MARKER(VP, real_gc_check(debug_malloc_pass(VP)))
#define gc_check_weak(VP) \
  DMALLOC_TOUCH_MARKER(VP, real_gc_check_weak(debug_malloc_pass(VP)))
#ifdef PIKE_DEBUG
#define debug_gc_check(X, T, DATA) \
  DMALLOC_TOUCH_MARKER(X, debug_low_gc_check(debug_malloc_pass(X), (T), (DATA)))
#endif /* PIKE_DEBUG */

#define gc_recurse_svalues(S,N)						\
  (Pike_in_gc == GC_PASS_CYCLE ?					\
   gc_cycle_check_svalues((S), (N)) : gc_mark_svalues((S), (N)))
#define gc_recurse_short_svalue(U,T)					\
  (Pike_in_gc == GC_PASS_CYCLE ?					\
   gc_cycle_check_short_svalue((U), (T)) : gc_mark_short_svalue((U), (T)))
#define gc_recurse_weak_svalues(S,N)					\
  (Pike_in_gc == GC_PASS_CYCLE ?					\
   gc_cycle_check_weak_svalues((S), (N)) : gc_mark_weak_svalues((S), (N)))
#define gc_recurse_weak_short_svalue(U,T)				\
  (Pike_in_gc == GC_PASS_CYCLE ?					\
   gc_cycle_check_weak_short_svalue((U), (T)) : gc_mark_weak_short_svalue((U), (T)))

#define GC_RECURSE_THING(V, T)						\
  (DMALLOC_TOUCH_MARKER(V, Pike_in_gc == GC_PASS_CYCLE) ?		\
   PIKE_CONCAT(gc_cycle_check_, T)(V, 0) :				\
   PIKE_CONCAT3(gc_mark_, T, _as_referenced)(V))
#define gc_recurse_array(V) GC_RECURSE_THING((V), array)
#define gc_recurse_mapping(V) GC_RECURSE_THING((V), mapping)
#define gc_recurse_multiset(V) GC_RECURSE_THING((V), multiset)
#define gc_recurse_object(V) GC_RECURSE_THING((V), object)
#define gc_recurse_program(V) GC_RECURSE_THING((V), program)

#define debug_gc_mark_without_recurse(S)				\
  gc_mark_without_recurse(S)
#define debug_gc_mark_weak_without_recurse(S)				\
  gc_mark_weak_without_recurse(S)
#define debug_gc_cycle_check_svalues(S, NUM, IN_TYPE, IN)		\
  gc_cycle_check_svalues ((S), (NUM))
#define debug_gc_cycle_check_weak_svalues(S, NUM, IN_TYPE, IN)		\
  gc_cycle_check_weak_svalues ((S), (NUM))
#define debug_gc_cycle_check_short_svalue(U, TYPE, IN_TYPE, IN)		\
  gc_cycle_check_short_svalue ((U), (TYPE))
#define debug_gc_cycle_check_weak_short_svalue(U, TYPE, IN_TYPE, IN)	\
  gc_cycle_check_weak_short_svalue ((U), (TYPE))
#define debug_gc_cycle_check_without_recurse(S)				\
  gc_cycle_check_without_recurse(S)
#define debug_gc_cycle_check_weak_without_recurse(S)			\
  gc_cycle_check_weak_without_recurse(S)

#if !defined (PIKE_DEBUG) && !defined (GC_MARK_DEBUG)
#define debug_gc_set_where(IN_TYPE, IN)
#endif

#ifndef PIKE_DEBUG
#define debug_gc_check_svalues(S,N,T,V) gc_check_svalues((S),N)
#define debug_gc_check_weak_svalues(S,N,T,V) gc_check_weak_svalues((S),N)
#define debug_gc_check_short_svalue(S,N,T,V) gc_check_short_svalue((S),N)
#define debug_gc_check_weak_short_svalue(S,N,T,V) gc_check_weak_short_svalue((S),N)
#define debug_gc_xmark_svalues(S,N,X) gc_xmark_svalues((S),N)
#define debug_gc_check(VP,T,V) gc_check((VP))
#endif

#ifdef PIKE_DEBUG
#define gc_is_referenced(X) \
  DMALLOC_TOUCH_MARKER(X, debug_gc_is_referenced(debug_malloc_pass(X)))
#else
#define gc_is_referenced(X) !(get_marker(X)->flags & GC_NOT_REFERENCED)
#endif

#define gc_external_mark2(X,Y,Z) \
  DMALLOC_TOUCH_MARKER(X, gc_external_mark3( debug_malloc_pass(X),(Y),(Z)))
#define gc_external_mark(X) \
  DMALLOC_TOUCH_MARKER(X, gc_external_mark2( (X),"externally", 0))

#define add_gc_callback(X,Y,Z) \
  dmalloc_touch(struct callback *,debug_add_gc_callback((X),(Y),(Z)))

#ifndef PIKE_DEBUG
#define gc_add_extra_ref(X) (++*(INT32 *)(X))
#define gc_free_extra_ref(X)
#endif

#define GC_PASS_PREPARE		 50
#define GC_PASS_PRETOUCH	 90
#define GC_PASS_CHECK		100
#define GC_PASS_MARK		200
#define GC_PASS_CYCLE		250
#define GC_PASS_MIDDLETOUCH	260
#define GC_PASS_ZAP_WEAK	270
#define GC_PASS_FREE		300
#define GC_PASS_KILL		400
#define GC_PASS_DESTRUCT	500
#define GC_PASS_POSTTOUCH	510

#define GC_PASS_LOCATE -1

#ifdef PIKE_DEBUG
extern int gc_in_cycle_check;
#endif

/* Use WEAK < 0 for strong links. The gc makes these assumptions about
 * those:
 * 1.  All strong links are recursed before any other links, i.e.
 *     strong links must be passed to gc_cycle_check_* last.
 * 2.  There can never be a cycle consisting of only strong links.
 */

#define GC_CYCLE_ENTER(X, WEAK) do {					\
  void *_thing_ = (X);							\
  struct marker *_m_ = get_marker(_thing_);				\
  if (!(_m_->flags & GC_MARKED)) {					\
    DO_IF_DEBUG(							\
      if (gc_in_cycle_check)						\
	fatal("Recursing immediately in gc cycle check.\n");		\
      gc_in_cycle_check = 1;						\
    );									\
    if (gc_cycle_push(_thing_, _m_, (WEAK))) {

#define GC_CYCLE_ENTER_OBJECT(X, WEAK) do {				\
  struct object *_thing_ = (X);						\
  struct marker *_m_ = get_marker(_thing_);				\
  if (!(_m_->flags & GC_MARKED)) {					\
    if (_thing_->prog && FIND_LFUN(_thing_->prog, LFUN_DESTROY) != -1)	\
      _m_->flags |= GC_LIVE|GC_LIVE_OBJ;				\
    DO_IF_DEBUG(							\
      if (gc_in_cycle_check)						\
	fatal("Recursing immediately in gc cycle check.\n");		\
      gc_in_cycle_check = 1;						\
    );									\
    if (gc_cycle_push(_thing_, _m_, (WEAK))) {

#define GC_CYCLE_LEAVE							\
    }									\
    DO_IF_DEBUG(gc_in_cycle_check = 0);					\
  }									\
} while (0)

#endif
