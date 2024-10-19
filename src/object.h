/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef OBJECT_H
#define OBJECT_H

#include "global.h"
#include "svalue.h"
#include "program.h"
#include "gc_header.h"
#include "pike_error.h"

/* a destructed object has no program */

struct object
{
  GC_MARKER_MEMBERS;
  unsigned INT32 flags;
  struct program *prog;
  struct object *next;
  struct object *prev;
  unsigned INT32 inhibit_destruct;
#ifdef PIKE_DEBUG
  INT32 program_id;
#endif
  char *storage;
};

/* Flags used in object->flags. */
#define OBJECT_CLEAR_ON_EXIT	1	/* Overwrite before free. */

#define OBJECT_PENDING_DESTRUCT	2	/* destruct() has been called on the
					 * object, and it will proceed as
					 * soon as the inhibit_destruct
					 * counter is back down to zero.
					 */

PMOD_EXPORT extern struct object *first_object;
extern struct object *gc_internal_object;
PMOD_EXPORT extern struct object *objects_to_destruct;
extern struct object *master_object;
extern struct program *master_program;
extern struct program *magic_index_program;
extern struct program *magic_set_index_program;
extern struct program *magic_indices_program;
extern struct program *magic_values_program;
extern struct program *magic_types_program;
extern struct program *magic_annotations_program;
#ifdef DO_PIKE_CLEANUP
PMOD_EXPORT extern int gc_external_refs_zapped;
PMOD_EXPORT void gc_check_zapped (void *a, TYPE_T type, const char *file, INT_TYPE line);
#endif

#define free_object(O) do{						\
    struct object *o_=(O);						\
    debug_malloc_touch(o_);						\
    debug_malloc_touch(o_->storage);					\
    DO_IF_DEBUG (							\
      DO_IF_PIKE_CLEANUP (						\
	if (gc_external_refs_zapped)					\
	  gc_check_zapped (o_, PIKE_T_OBJECT, __FILE__, __LINE__)));	\
    if(!sub_ref(o_))							\
      schedule_really_free_object(o_);					\
  }while(0)

#ifdef DEBUG_MALLOC
#define PIKE_OBJ_STORAGE(O) ((char *)debug_malloc_pass( (O)->storage ))
#else
#define PIKE_OBJ_STORAGE(O) ((O)->storage)
#endif

#define LOW_GET_GLOBAL(O,I,ID) (PIKE_OBJ_STORAGE((O))+INHERIT_FROM_INT((O)->prog, (I))->storage_offset+(ID)->func.offset)
#define GET_GLOBAL(O,I) LOW_GET_GLOBAL(O,I,ID_FROM_INT((O)->prog,I))

#define this_object() (add_ref(Pike_fp->current_object), Pike_fp->current_object)

enum object_destruct_reason {
  DESTRUCT_EXPLICIT = 0,
  DESTRUCT_CLEANUP = 1,
  DESTRUCT_NO_REFS,
  DESTRUCT_GC
};

#include "block_alloc_h.h"

ATTRIBUTE((malloc)) struct object * alloc_object();
void really_free_object(struct object * o);
void count_memory_in_objects(size_t *_num, size_t *_size);
void free_all_object_blocks(void);
PMOD_EXPORT struct object *low_clone(struct program *p);
PMOD_EXPORT void call_c_initializers(struct object *o);
PMOD_EXPORT void call_prog_event(struct object *o, int event);
void call_pike_initializers(struct object *o, int args);
PMOD_EXPORT void do_free_object(struct object *o);
PMOD_EXPORT struct object *debug_clone_object(struct program *p, int args);
PMOD_EXPORT struct object *fast_clone_object(struct program *p);
PMOD_EXPORT struct object *parent_clone_object(struct program *p,
					       struct object *parent,
					       ptrdiff_t parent_identifier,
					       int args);
PMOD_EXPORT struct object *clone_object_from_object(struct object *o, int args);
struct object *decode_value_clone_object(struct svalue *prog);
struct pike_string *low_read_file(const char *file);
PMOD_EXPORT struct object *get_master(void);
PMOD_EXPORT struct object *debug_master(void);
struct destruct_called_mark;
PTR_HASH_ALLOC(destruct_called_mark,128);
PMOD_EXPORT struct program *get_program_for_object_being_destructed(struct object * o);
PMOD_EXPORT int destruct_object(struct object *o,
				enum object_destruct_reason reason);
#define destruct(o) destruct_object (o, DESTRUCT_EXPLICIT)
PMOD_EXPORT void low_destruct_objects_to_destruct(void);
void destruct_objects_to_destruct_cb(void);
PMOD_EXPORT void schedule_really_free_object(struct object *o);
PMOD_EXPORT int low_object_index_no_free(struct svalue *to,
					 struct object *o,
					 ptrdiff_t f);
PMOD_EXPORT int object_index_no_free2(struct svalue *to,
				      struct object *o,
				      int inherit_level,
				      struct svalue *key);
PMOD_EXPORT int object_index_no_free(struct svalue *to,
				     struct object *o,
				     int inherit_level,
				     struct svalue *key);
PMOD_EXPORT void object_low_atomic_get_set_index(struct object *o,
						 int f,
						 struct svalue *from_to);
PMOD_EXPORT void object_low_set_index(struct object *o,
				      int f,
                                      const struct svalue *from);
PMOD_EXPORT void object_atomic_get_set_index2(struct object *o,
					      int inherit_level,
					      struct svalue *key,
					      struct svalue *from_to);
PMOD_EXPORT void object_atomic_get_set_index(struct object *o,
					     int inherit_level,
					     struct svalue *key,
					     struct svalue *from_to);
PMOD_EXPORT void object_set_index2(struct object *o,
				   int inherit_level,
                                   const struct svalue *key,
                                   const struct svalue *from);
PMOD_EXPORT void object_set_index(struct object *o,
				  int inherit_level,
                                  const struct svalue *key,
                                  const struct svalue *from);
union anything *object_get_item_ptr(struct object *o,
				    int inherit_level,
				    struct svalue *key,
				    TYPE_T type);
PMOD_EXPORT int object_equal_p(struct object *a, struct object *b, struct processing *p);
PMOD_EXPORT struct array *object_indices(struct object *o, int inherit_level);
PMOD_EXPORT struct array *object_values(struct object *o, int inherit_level);
PMOD_EXPORT struct array *object_types(struct object *o, int inherit_level);
PMOD_EXPORT struct array *object_annotations(struct object *o,
					     int inherit_level,
					     int flags);
PMOD_EXPORT void visit_object (struct object *o, int action, void *extra);
PMOD_EXPORT void visit_function (const struct svalue *s, int ref_type,
				 void *extra);
PMOD_EXPORT void gc_mark_object_as_referenced(struct object *o);
PMOD_EXPORT void real_gc_cycle_check_object(struct object *o, int weak);

enum memobj_type{
    MEMOBJ_NONE,
    MEMOBJ_SYSTEM_MEMORY,
    MEMOBJ_STRING_BUFFER,
    MEMOBJ_STDIO_IOBUFFER,
};

struct pike_memory_object {
  void *ptr;
  size_t len;
  int shift;
};

PMOD_EXPORT enum memobj_type get_memory_object_memory( struct object *o, void **ptr, size_t *len, int *shift );
PMOD_EXPORT enum memobj_type pike_get_memory_object( struct object *o, struct pike_memory_object *m,
                                                     int writeable );
PMOD_EXPORT int pike_advance_memory_object( struct object *o,
                                            enum memobj_type type,
                                            size_t length );


unsigned gc_touch_all_objects(void);
void gc_check_all_objects(void);
void gc_mark_all_objects(void);
void gc_cycle_check_all_objects(void);
void gc_zap_ext_weak_refs_in_objects(void);
size_t gc_free_all_unreferenced_objects(void);
struct magic_index_struct;
void push_magic_index(struct program *type, int inherit_no, int parent_level);
void low_init_object(void);
void init_object(void);
void exit_object(void);
void late_exit_object(void);
void check_object_context(struct object *o,
			  struct program *context_prog,
			  char *current_storage);
void check_object(struct object *o);
void check_all_objects(void);
/* Prototypes end here */

static inline void object_inhibit_destruct(struct object *o)
{
  o->inhibit_destruct++;
}

static inline void object_permit_destruct(struct object *o)
{
#ifdef PIKE_DEBUG
  if (!o->inhibit_destruct) {
    Pike_fatal("Destruction is already permitted for this object.\n"
	       "Is this a duplicate call?\n");
  }
#endif /* PIKE_DEBUG */
  if (--o->inhibit_destruct) return;
  if (o->flags & OBJECT_PENDING_DESTRUCT) {
    /* Perform the delayed explicit destruct(). */
    o->flags &= ~OBJECT_PENDING_DESTRUCT;
    destruct_object(o, DESTRUCT_EXPLICIT);
  }
}

/**
 * Look up the given lfun in the given inherit of the object.
 * Returns the function number in inherit 0 if it exists,
 * or -1 if not found.
 */
static inline int PIKE_UNUSED_ATTRIBUTE FIND_OBJECT_LFUN(struct object *o,
							 int inherit,
							 enum LFUN lfun)
{
  int f = -1;
  if (o->prog && o->prog->num_inherits > inherit) {
    f = FIND_LFUN(o->prog->inherits[inherit].prog, lfun);
    if (f != -1) {
      f += o->prog->inherits[inherit].identifier_level;
    }
  }
  return f;
}

#ifdef DEBUG_MALLOC
#define clone_object(X,Y) ((struct object *)debug_malloc_pass(debug_clone_object((X),(Y))))
#else
#define clone_object debug_clone_object
#endif

#ifdef PIKE_DEBUG
#define master() ( get_master() ? get_master() : ( Pike_fatal("Couldn't load master object at %s:%d.\n",__FILE__,__LINE__), (struct object *)NULL) )
#else
#define master() debug_master()
#endif

#define visit_object_ref(O, REF_TYPE, EXTRA)			\
  visit_ref (pass_object (O), (REF_TYPE),			\
	     (visit_thing_fn *) &visit_object, (EXTRA))
#define gc_cycle_check_object(X, WEAK) \
  gc_cycle_enqueue((gc_cycle_check_cb *) real_gc_cycle_check_object, (X), (WEAK))

#define PIKE_OBJ_DESTRUCTED(o) (!(o)->prog)
#define PIKE_OBJ_INITED(o) (o->prog && (o->prog->flags & PROGRAM_PASS_1_DONE) && !((o->prog->flags & PROGRAM_AVOID_CHECK)))
#define destruct_objects_to_destruct() do{ if(objects_to_destruct) low_destruct_objects_to_destruct(); }while(0)

#define low_index_current_object_no_free(TO, FUN)			\
  low_object_index_no_free((TO), Pike_fp->current_object,		\
			   Pike_fp->context->identifier_level + (FUN))

#define tF_MAGIC_INDEX tFunc(tStr tOr3(tVoid,tObj,tDeprecated(tInt)) \
                             tOr(tVoid,tInt), tMix)
#define tF_MAGIC_SET_INDEX tFunc(tStr tMix tOr3(tVoid,tObj,tDeprecated(tInt)) \
                                 tOr(tVoid,tInt), tVoid)
#define tF_MAGIC_INDICES tFunc(tOr3(tVoid,tObj,tDeprecated(tInt)) \
                               tOr(tVoid,tInt), tArr(tStr))
#define tF_MAGIC_VALUES tFunc(tOr3(tVoid,tObj,tDeprecated(tInt)) \
                              tOr(tVoid,tInt), tArray)
#define tF_MAGIC_TYPES tFunc(tOr3(tVoid,tObj,tDeprecated(tInt)) \
                             tOr(tVoid,tInt), tArr(tType(tMix)))
#define tF_MAGIC_ANNOTATIONS tFunc(tOr3(tVoid,tObj,tDeprecated(tInt)) \
				   tOr(tVoid,tInt) tOr(tVoid,tInt01), \
				   tArr(tType(tMix)))

#endif /* OBJECT_H */
