/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: object.h,v 1.22 1999/01/31 22:50:30 grubba Exp $
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
  INT32 refs;                    /* Reference count, must be first. */
#ifdef PIKE_SECURITY
  struct object *prot;
#endif
  struct program *prog;
  struct object *parent;
  INT16 parent_identifier;
  struct object *next;
  struct object *prev;
#ifdef PIKE_SECURITY
  char *pad;		/* FIXME: Kluge to get longlong alignment of storage */
#endif
  char storage[1];
};

extern struct object *first_object;
extern struct object *master_object;
extern struct program *master_program;

#define free_object(O) do{ struct object *o_=(O); debug_malloc_touch(o_); if(!--o_->refs) really_free_object(o_); }while(0)

#define LOW_GET_GLOBAL(O,I,ID) ((O)->storage+INHERIT_FROM_INT((O)->prog, (I))->storage_offset+(ID)->func.offset)
#define GET_GLOBAL(O,I) LOW_GET_GLOBAL(O,I,ID_FROM_INT((O)->prog,I))
#define GLOBAL_FROM_INT(I) GET_GLOBAL(fp->current_object, I)

#define this_object() (add_ref(fp->current_object), fp->current_object)

/* Prototypes begin here */
struct object *low_clone(struct program *p);
void call_c_initializers(struct object *o);
void do_free_object(struct object *o);
struct object *debug_clone_object(struct program *p, int args);
struct object *parent_clone_object(struct program *p,
				   struct object *parent,
				   int parent_identifier,
				   int args);
struct object *get_master(void);
struct object *master(void);
void destruct(struct object *o);
void destruct_objects_to_destruct(void);
void really_free_object(struct object *o);
void low_object_index_no_free(struct svalue *to,
			      struct object *o,
			      INT32 f);
void object_index_no_free2(struct svalue *to,
			  struct object *o,
			  struct svalue *index);
void object_index_no_free(struct svalue *to,
			   struct object *o,
			   struct svalue *index);
void object_low_set_index(struct object *o,
			  int f,
			  struct svalue *from);
void object_set_index2(struct object *o,
		      struct svalue *index,
		      struct svalue *from);
void object_set_index(struct object *o,
		       struct svalue *index,
		       struct svalue *from);
union anything *object_get_item_ptr(struct object *o,
				    struct svalue *index,
				    TYPE_T type);
void verify_all_objects(void);
int object_equal_p(struct object *a, struct object *b, struct processing *p);
void cleanup_objects(void);
struct array *object_indices(struct object *o);
struct array *object_values(struct object *o);
void gc_mark_object_as_referenced(struct object *o);
void gc_check_all_objects(void);
void gc_mark_all_objects(void);
void gc_free_all_unreferenced_objects(void);
void count_memory_in_objects(INT32 *num_, INT32 *size_);
/* Prototypes end here */

#ifdef MALLOC_DEBUG
#define clone_object(X,Y) ((struct object *)debug_malloc_touch(debug_clone_object((X),(Y))))
#else
#define clone_object debug_clone_object
#endif

#endif /* OBJECT_H */

