/*
 * $Id: gc.h,v 1.55 2000/07/18 05:48:20 mast Exp $
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
    if(Pike_in_gc == GC_PASS_CHECK)					\
      fatal("Freeing objects in this gc pass is not allowed.\n");	\
    if(num_objects < 1)							\
      fatal("Panic!! less than zero objects!\n");			\
  )									\
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

#ifdef PIKE_DEBUG
#define GC_TOUCHED		0x0100
#define GC_IS_REFERENCED	0x0200
#define GC_XREFERENCED		0x0400
#define GC_DO_FREE		0x0800
#define GC_GOT_EXTRA_REF	0x1000
#endif

#include "block_alloc_h.h"
PTR_HASH_ALLOC(marker,MARKER_CHUNK_SIZE)

extern size_t gc_ext_weak_refs;

typedef void gc_cycle_check_cb (void *data, int weak);

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
void debug_gc_check_svalues(struct svalue *s, int num, TYPE_T t, void *data);
void debug_gc_check_weak_svalues(struct svalue *s, int num, TYPE_T t, void *data);
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
void gc_set_rec_last(struct marker *m);
void do_gc_recurse_svalues(struct svalue *s, int num);
void do_gc_recurse_short_svalue(union anything *u, TYPE_T type);
int gc_do_free(void *a);
int gc_is_internal(void *a);
int do_gc(void);
void f__gc_status(INT32 args);
void gc_cycle_enqueue(gc_cycle_check_cb *checkfn, void *data, int weak);
void gc_cycle_run_queue();
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

#define GC_RECURSE_THING(V, T)						\
  (Pike_in_gc == GC_PASS_MARK ?						\
   PIKE_CONCAT3(gc_mark_, T, _as_referenced)(V) :			\
   PIKE_CONCAT(gc_cycle_check_, T)(V, 0))
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
#define gc_is_referenced(X) !(get_marker(X)->flags & GC_NOT_REFERENCED)
#define gc_do_weak_free(X) (Pike_in_gc != GC_PASS_ZAP_WEAK ?		\
			    get_marker(X)->weak_refs == -1 :		\
			    !(get_marker(X)->flags & GC_MARKED))
#endif

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
