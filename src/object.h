/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: object.h,v 1.81 2004/03/15 22:47:15 mast Exp $
*/

#ifndef OBJECT_H
#define OBJECT_H

#include "global.h"
#include "svalue.h"

/* a destructed object has no program */

#ifndef STRUCT_OBJECT_DECLARED
#define STRUCT_OBJECT_DECLARED
#endif
struct object
{
  PIKE_MEMORY_OBJECT_MEMBERS; /* Must be first */
  struct program *prog;
  struct object *next;
  struct object *prev;
#if PIKE_DEBUG
  INT32 program_id;
#endif
  char *storage;
};

PMOD_EXPORT extern struct object *first_object;
extern struct object *gc_internal_object;
extern struct object *objects_to_destruct;
extern struct object *master_object;
extern struct program *master_program;
extern struct program *magic_index_program;
extern struct program *magic_set_index_program;
extern struct program *magic_indices_program;
extern struct program *magic_values_program;

#define free_object(O) do{ struct object *o_=(O); debug_malloc_touch(o_); debug_malloc_touch(o_->storage); if(!sub_ref(o_)) schedule_really_free_object(o_); }while(0)

#ifdef DEBUG_MALLOC
#define PIKE_OBJ_STORAGE(O) ((char *)debug_malloc_pass( (O)->storage ))
#else
#define PIKE_OBJ_STORAGE(O) ((O)->storage)
#endif

#define LOW_GET_GLOBAL(O,I,ID) (PIKE_OBJ_STORAGE((O))+INHERIT_FROM_INT((O)->prog, (I))->storage_offset+(ID)->func.offset)
#define GET_GLOBAL(O,I) LOW_GET_GLOBAL(O,I,ID_FROM_INT((O)->prog,I))

#define this_object() (add_ref(Pike_fp->current_object), Pike_fp->current_object)

#include "block_alloc_h.h"
/* Prototypes begin here */
BLOCK_ALLOC_FILL_PAGES(object, 2)
PMOD_EXPORT struct object *low_clone(struct program *p);
PMOD_EXPORT void call_c_initializers(struct object *o);
void call_prog_event(struct object *o, int event);
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
PMOD_EXPORT struct object *get_master(void);
PMOD_EXPORT struct object *debug_master(void);
struct destroy_called_mark;
PTR_HASH_ALLOC(destroy_called_mark,128)
PMOD_EXPORT struct program *get_program_for_object_being_destructed(struct object * o);
void destruct(struct object *o);
void low_destruct_objects_to_destruct(void);
void destruct_objects_to_destruct_cb(void);
PMOD_EXPORT void schedule_really_free_object(struct object *o);
PMOD_EXPORT void low_object_index_no_free(struct svalue *to,
					  struct object *o,
					  ptrdiff_t f);
PMOD_EXPORT void object_index_no_free2(struct svalue *to,
			  struct object *o,
			  struct svalue *key);
PMOD_EXPORT void object_index_no_free(struct svalue *to,
			   struct object *o,
			   struct svalue *key);
PMOD_EXPORT void object_low_set_index(struct object *o,
			  int f,
			  struct svalue *from);
PMOD_EXPORT void object_set_index2(struct object *o,
		      struct svalue *key,
		      struct svalue *from);
PMOD_EXPORT void object_set_index(struct object *o,
		       struct svalue *key,
		       struct svalue *from);
union anything *object_get_item_ptr(struct object *o,
				    struct svalue *key,
				    TYPE_T type);
PMOD_EXPORT int object_equal_p(struct object *a, struct object *b, struct processing *p);
PMOD_EXPORT struct array *object_indices(struct object *o);
PMOD_EXPORT struct array *object_values(struct object *o);
PMOD_EXPORT void gc_mark_object_as_referenced(struct object *o);
PMOD_EXPORT void real_gc_cycle_check_object(struct object *o, int weak);
unsigned gc_touch_all_objects(void);
void gc_check_all_objects(void);
void gc_mark_all_objects(void);
void gc_cycle_check_all_objects(void);
void gc_zap_ext_weak_refs_in_objects(void);
size_t gc_free_all_unreferenced_objects(void);
struct magic_index_struct;
void push_magic_index(struct program *type, int inherit_no, int parent_level);
void init_object(void);
void exit_object(void);
void check_object_context(struct object *o,
			  struct program *context_prog,
			  char *current_storage);
void check_object(struct object *o);
void check_all_objects(void);
/* Prototypes end here */

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

#define gc_cycle_check_object(X, WEAK) \
  gc_cycle_enqueue((gc_cycle_check_cb *) real_gc_cycle_check_object, (X), (WEAK))

#define PIKE_OBJ_DESTRUCTED(o) (o->prog)
#define PIKE_OBJ_INITED(o) (o->prog && (o->prog->flags & PROGRAM_PASS_1_DONE) && !((o->prog->flags & PROGRAM_AVOID_CHECK)))
#define destruct_objects_to_destruct() do{ if(objects_to_destruct) low_destruct_objects_to_destruct(); }while(0)

#endif /* OBJECT_H */
