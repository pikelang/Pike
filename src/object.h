/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef OBJECT_H
#define OBJECT_H

#include "types.h"
#include "svalue.h"

/* a destructed object has no program */

struct object
{
  INT32 refs;                    /* Reference count, must be first. */
  struct program *prog;
  struct object *next;
  struct object *prev;
  char storage[1];
};

extern struct object fake_object;
extern struct object *first_object;
extern struct object *objects_to_destruct;

#define free_object(O) do{ struct object *o_=(O); if(!--o_->refs) really_free_object(o_); }while(0)

#define LOW_GET_GLOBAL(O,I,ID) ((O)->storage+INHERIT_FROM_INT((O)->prog, (I))->storage_offset+(ID)->func.offset)
#define GET_GLOBAL(O,I) LOW_GET_GLOBAL(O,I,ID_FROM_INT((O)->prog,I))
#define GLOBAL_FROM_INT(I) GET_GLOBAL(fp->current_object, I)

#define this_object() (fp->current_object->refs++,fp->current_object)

/* Prototypes begin here */
void setup_fake_object();
struct object *clone(struct program *p, int args);
struct object *get_master();
struct object *master();
void destruct(struct object *o);
void really_free_object(struct object *o);
void destruct_objects_to_destruct();
void object_index_no_free(struct svalue *to,
			  struct object *o,
			  struct svalue *index);
void object_index_no_free2(struct svalue *to,
			   struct object *o,
			   struct svalue *index);
void object_set_index(struct object *o,
		      struct svalue *index,
		      struct svalue *from);
void object_set_index2(struct object *o,
		       struct svalue *index,
		       struct svalue *from);
union anything *object_get_item_ptr(struct object *o,
				    struct svalue *index,
				    TYPE_T type);
void verify_all_objects();
int object_equal_p(struct object *a, struct object *b, struct processing *p);
void cleanup_objects();
struct array *object_indices(struct object *o);
struct array *object_values(struct object *o);
void gc_mark_object_as_referenced(struct object *o);
void gc_check_all_objects();
void gc_mark_all_objects();
void gc_free_all_unreferenced_objects();
/* Prototypes end here */

#endif /* OBJECT_H */

