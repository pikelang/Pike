/*
 * $Id: gc.h,v 1.47 2000/06/12 03:21:11 mast Exp $
 */
#ifndef GC_H
#define GC_H

#include "global.h"
#include "callback.h"
#include "queue.h"
#include "threads.h"

extern struct pike_queue gc_mark_queue;
extern INT32 num_objects;
extern INT32 num_allocs;
extern INT32 alloc_threshold;
extern int Pike_in_gc;
extern int gc_debug;

extern struct callback *gc_evaluator_callback;
extern struct callback_list evaluator_callbacks;
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

#define GC_FREE() do {							\
  DO_IF_DEBUG(								\
    extern int d_flag;							\
    if(d_flag) CHECK_INTERPRETER_LOCK();				\
    if(num_objects < 1)							\
      fatal("Panic!! less than zero objects!\n");			\
  )									\
  num_objects-- ;							\
}while(0)

struct marker
{
  struct marker *next;
  struct marker *link;		/* Next pointer used in rec_list and destroy_list. */
  void *data;
  INT32 refs;			/* Internal references. */
  INT32 weak_refs;		/* Weak (implying internal) references. */
#ifdef PIKE_DEBUG
  INT32 xrefs;			/* Known external references. */
  INT32 saved_refs;		/* Object refcount during check and mark pass. */
#endif
  unsigned INT16 cycle;		/* Cycle id number. */
  unsigned INT16 flags;
};

#define GC_REFERENCED		0x0001
#define GC_CYCLE_CHECKED	0x0002
#define GC_LIVE			0x0004
#define GC_LIVE_OBJ		0x0008
#define GC_GOT_DEAD_REF		0x0010
#define GC_RECURSING		0x0020
#define GC_DONT_POP		0x0040
#define GC_LIVE_RECURSE		0x0080
#define GC_WEAK_REF		0x0100
#define GC_STRONG_REF		0x0200

/* Debug mode flags. */
#define GC_TOUCHED		0x0400
#define GC_IS_REFERENCED	0x0800
#define GC_XREFERENCED		0x1000
#define GC_DO_FREE		0x2000
#define GC_GOT_EXTRA_REF	0x4000
#define GC_FOLLOWED_NONSTRONG	0x8000

#include "block_alloc_h.h"
PTR_HASH_ALLOC(marker,MARKER_CHUNK_SIZE)

extern struct marker *gc_rec_last;

/* Prototypes begin here */
struct callback *debug_add_gc_callback(callback_func call,
				 void *arg,
				 callback_func free_func);
void dump_gc_info(void);
TYPE_T attempt_to_identify(void *something);
void describe_location(void *memblock, int type, void *location,int indent, int depth, int flags);
void debug_gc_fatal(void *a, int flags, const char *fmt, ...)
  ATTRIBUTE((format(printf, 3, 4)));
void debug_gc_xmark_svalues(struct svalue *s, int num, char *fromwhere);
TYPE_FIELD debug_gc_check_svalues(struct svalue *s, int num, TYPE_T t, void *data);
TYPE_FIELD debug_gc_check_weak_svalues(struct svalue *s, int num, TYPE_T t, void *data);
void debug_gc_check_short_svalue(union anything *u, TYPE_T type, TYPE_T t, void *data);
void debug_gc_check_weak_short_svalue(union anything *u, TYPE_T type, TYPE_T t, void *data);
int debug_gc_check(void *x, TYPE_T t, void *data);
void describe_something(void *a, int t, int indent, int depth, int flags);
void describe(void *x);
void debug_describe_svalue(struct svalue *s);
void debug_gc_touch(void *a);
INT32 real_gc_check(void *a);
INT32 real_gc_check_weak(void *a);
void locate_references(void *a);
void gc_add_extra_ref(void *a);
void gc_free_extra_ref(void *a);
int debug_gc_is_referenced(void *a);
int gc_external_mark3(void *a, void *in, char *where);
int gc_do_weak_free(void *a);
int gc_mark(void *a);
int gc_cycle_push(void *x, struct marker *m, int weak);
void gc_cycle_pop(void *a);
void gc_set_rec_last(struct marker *m);
void do_gc_recurse_svalues(struct svalue *s, int num);
void do_gc_recurse_short_svalue(union anything *u, TYPE_T type);
int gc_do_free(void *a);
int gc_is_internal(void *a);
int do_gc(void);
void f__gc_status(INT32 args);
/* Prototypes end here */

#define gc_fatal \
  fprintf(stderr, "%s:%d: GC fatal:\n", __FILE__, __LINE__), debug_gc_fatal

#define gc_check(VP) real_gc_check(debug_malloc_pass(VP))
#define gc_check_weak(VP) real_gc_check_weak(debug_malloc_pass(VP))

#define gc_recurse_svalues(S,N)						\
  (Pike_in_gc == GC_PASS_MARK ?						\
   gc_mark_svalues((S), (N)) : gc_cycle_check_svalues((S), (N)))
#define gc_recurse_short_svalue(U,T)					\
  (Pike_in_gc == GC_PASS_MARK ?						\
   gc_mark_short_svalue((U), (T)) : gc_cycle_check_short_svalue((U), (T)))
#define gc_recurse_weak_svalues(S,N)					\
  (Pike_in_gc == GC_PASS_MARK ?						\
   gc_mark_weak_svalues((S), (N)) : gc_cycle_check_weak_svalues((S), (N)))
#define gc_recurse_weak_short_svalue(U,T)				\
  (Pike_in_gc == GC_PASS_MARK ?						\
   gc_mark_weak_short_svalue((U), (T)) : gc_cycle_check_weak_short_svalue((U), (T)))

#define GC_RECURSE_THING(V,T)						\
  (Pike_in_gc == GC_PASS_MARK ?						\
   PIKE_CONCAT3(gc_mark_, T, _as_referenced)(V) : PIKE_CONCAT(gc_cycle_check_, T)(V))
#define gc_recurse_array(V) GC_RECURSE_THING((V), array)
#define gc_recurse_mapping(V) GC_RECURSE_THING((V), mapping)
#define gc_recurse_multiset(V) GC_RECURSE_THING((V), multiset)
#define gc_recurse_object(V) GC_RECURSE_THING((V), object)
#define gc_recurse_program(V) GC_RECURSE_THING((V), program)

#ifndef PIKE_DEBUG
#define debug_gc_check_svalues(S,N,T,V) gc_check_svalues((S),N)
#define debug_gc_check_weak_svalues(S,N,T,V) gc_check_weak_svalues((S),N)
#define debug_gc_check_short_svalue(S,N,T,V) gc_check_short_svalue((S),N)
#define debug_gc_check_weak_short_svalue(S,N,T,V) gc_check_weak_short_svalue((S),N)
#define debug_gc_xmark_svalues(S,N,X) gc_xmark_svalues((S),N)
#define debug_gc_check(VP,T,V) gc_check((VP))
#endif

#ifdef PIKE_DEBUG
#define gc_is_referenced(X) debug_gc_is_referenced(debug_malloc_pass(X))
#else
#define gc_is_referenced(X) (get_marker(X)->refs < *(INT32 *)(X))
#define gc_do_weak_free(X) (get_marker(X)->weak_refs >= *(INT32 *) (X))
#endif

#define gc_do_weak_free_svalue(S) \
  ((S)->type <= MAX_COMPLEX && gc_do_weak_free((S)->u.refs))

#define gc_external_mark2(X,Y,Z) gc_external_mark3( debug_malloc_pass(X),(Y),(Z))
#define gc_external_mark(X) gc_external_mark2( (X),"externally", 0)

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
 *     strong links should be pushed last into the lifo queue.
 * 2.  There can never be a cycle consisting of only strong links.
 */

#define GC_CYCLE_ENTER(X, WEAK) do {					\
  void *_thing_ = (X);							\
  struct marker *_m_ = get_marker(_thing_);				\
  if (!(_m_->flags & GC_REFERENCED)) {					\
    struct marker *_old_last_ = gc_rec_last;				\
    if (gc_cycle_push(_thing_, _m_, (WEAK))) {				\
      DO_IF_DEBUG(							\
	if (gc_in_cycle_check)						\
	  fatal("Recursing immediately in gc cycle check.\n");		\
	gc_in_cycle_check = 1;						\
      );								\
      enqueue_lifo(&gc_mark_queue,					\
		   (queue_call) gc_cycle_pop, _thing_);			\
      enqueue_lifo(&gc_mark_queue,					\
		   (queue_call) gc_set_rec_last, _old_last_);		\
      {

#define GC_CYCLE_ENTER_OBJECT(X, WEAK) do {				\
  struct object *_thing_ = (X);						\
  struct marker *_m_ = get_marker(_thing_);				\
  if (!(_m_->flags & GC_REFERENCED)) {					\
    struct marker *_old_last_ = gc_rec_last;				\
    if (_thing_->prog && FIND_LFUN(_thing_->prog, LFUN_DESTROY) != -1)	\
      _m_->flags |= GC_LIVE|GC_LIVE_OBJ;				\
    if (gc_cycle_push(_thing_, _m_, (WEAK))) {				\
      DO_IF_DEBUG(							\
	if (gc_in_cycle_check)						\
	  fatal("Recursing immediately in gc cycle check.\n");		\
	gc_in_cycle_check = 1;						\
      );								\
      enqueue_lifo(&gc_mark_queue,					\
		   (queue_call) gc_cycle_pop, _thing_);			\
      enqueue_lifo(&gc_mark_queue,					\
		   (queue_call) gc_set_rec_last, _old_last_);		\
      {

#define GC_CYCLE_LEAVE							\
      }									\
      DO_IF_DEBUG(gc_in_cycle_check = 0);				\
    }									\
  }									\
} while (0)

#endif
