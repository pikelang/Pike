/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef ARRAY_H
#define ARRAY_H

#include "las.h"

struct array
{
  INT32 refs;		/* Reference count */
  struct array *next;	/* we need to keep track of all arrays */
  struct array *prev;	/* Another pointer, so we don't have to search
			 * when freeing arrays */
  INT32 size;		/* number of items in this array */
  INT32 malloced_size;	/* number of elements that can fit in this array */
  TYPE_FIELD type_field;/* A bitfield with one bit for each type.
			 * Bits can be set that don't exist in the array
			 * though.
			 */
  INT16 flags;          /* ARRAY_* flags */
  struct svalue item[1];
};

#define ARRAY_CYCLIC 1

extern struct array empty_array;

#define ITEM(X) ((X)->item)

/* These are arguments for the function 'merge' which merges two sorted
 * set stored in arrays in the way you specify
 */
#define OP_A 1
#define OP_SKIP_A 2
#define OP_TAKE_A 3
#define OP_B 4
#define OP_SKIP_B 8
#define OP_TAKE_B 12
#define MINTERM(X,Y,Z) (((X)<<8)+((Y)<<4)+(Z))

#define OP_AND MINTERM(OP_SKIP_A,OP_SKIP_A | OP_TAKE_B,OP_SKIP_B)
#define OP_OR  MINTERM(OP_TAKE_A,OP_SKIP_A | OP_TAKE_B,OP_TAKE_B)
#define OP_XOR MINTERM(OP_TAKE_A,OP_SKIP_A | OP_SKIP_B,OP_TAKE_B)
#define OP_ADD MINTERM(OP_TAKE_A,OP_TAKE_A | OP_TAKE_B ,OP_TAKE_B)
#define OP_SUB MINTERM(OP_TAKE_A,OP_SKIP_A ,OP_SKIP_B)


#define free_array(V) do{ struct array *v_=(V); if(!--v_->refs) really_free_array(v_); }while(0)

#define allocate_array(X) low_allocate_array((X),0)
#define allocate_array_no_init(X,Y) low_allocate_array((X),(Y))

typedef int (*cmpfun)(struct svalue *,struct svalue *);
typedef int (*short_cmpfun)(union anything *, union anything *);
typedef short_cmpfun (*cmpfun_getter)(TYPE_T);


/* Prototypes begin here */
struct array *low_allocate_array(INT32 size,INT32 extra_space);
void really_free_array(struct array *v);
void array_index_no_free(struct svalue *s,struct array *v,INT32 index);
void array_index(struct svalue *s,struct array *v,INT32 index);
void simple_array_index(struct svalue *s,struct array *a,struct svalue *ind);
void simple_array_index_no_free(struct svalue *s,struct array *a,struct svalue *ind);
void array_free_index(struct array *v,INT32 index);
void array_set_index(struct array *v,INT32 index, struct svalue *s);
void simple_set_index(struct array *a,struct svalue *ind,struct svalue *s);
struct array *array_insert(struct array *v,struct svalue *s,INT32 index);
struct array *array_shrink(struct array *v,INT32 size);
struct array *array_remove(struct array *v,INT32 index);
INT32 array_search(struct array *v, struct svalue *s,INT32 start);
struct array *slice_array(struct array *v,INT32 start,INT32 end);
struct array *copy_array(struct array *v);
void check_array_for_destruct(struct array *v);
INT32 array_find_destructed_object(struct array *v);
INT32 *get_order(struct array *v, cmpfun fun);
void sort_array_destructively(struct array *v);
INT32 *get_set_order(struct array *a);
INT32 *get_switch_order(struct array *a);
INT32 *get_alpha_order(struct array *a);
INT32 set_lookup(struct array *a, struct svalue *s);
INT32 switch_lookup(struct array *a, struct svalue *s);
struct array *order_array(struct array *v, INT32 *order);
struct array *reorder_and_copy_array(struct array *v, INT32 *order);
void array_fix_type_field(struct array *v);
void array_check_type_field(struct array *v);
struct array *compact_array(struct array *v);
union anything *low_array_get_item_ptr(struct array *a,
				       INT32 ind,
				       TYPE_T t);
union anything *array_get_item_ptr(struct array *a,
				   struct svalue *ind,
				   TYPE_T t);
INT32 * merge(struct array *a,struct array *b,INT32 opcode);
struct array *array_zip(struct array *a, struct array *b,INT32 *zipper);
struct array *add_arrays(struct svalue *argp, INT32 args);
int array_equal_p(struct array *a, struct array *b, struct processing *p);
struct array *merge_array_with_order(struct array *a, struct array *b,INT32 op);
struct array *merge_array_without_order(struct array *a,
					struct array *b,
					INT32 op);
struct array *subtract_arrays(struct array *a, struct array *b);
struct array *and_arrays(struct array *a, struct array *b);
int check_that_array_is_constant(struct array *a);
node *make_node_from_array(struct array *a);
void push_array_items(struct array *a);
void describe_array_low(struct array *a, struct processing *p, int indent);
void simple_describe_array(struct array *a);
void describe_index(struct array *a,
		    int e,
		    struct processing *p,
		    int indent);
void describe_array(struct array *a,struct processing *p,int indent);
struct array *aggregate_array(INT32 args);
struct array *explode(struct pike_string *str,
		       struct pike_string *del);
struct pike_string *implode(struct array *a,struct pike_string *del);
struct array *copy_array_recursively(struct array *a,struct processing *p);
void apply_array(struct array *a, INT32 args);
struct array *reverse_array(struct array *a);
void array_replace(struct array *a,
		   struct svalue *from,
		   struct svalue *to);
void check_array(struct array *a);
void check_all_arrays(void);
void gc_mark_array_as_referenced(struct array *a);
void gc_check_all_arrays(void);
void gc_mark_all_arrays(void);
void gc_free_all_unreferenced_arrays(void);
void debug_dump_type_field(TYPE_FIELD t);
void debug_dump_array(struct array *a);
void zap_all_arrays(void);
void count_memory_in_arrays(INT32 *num_, INT32 *size_);
struct array *explode_array(struct array *a, struct array *b);
struct array *implode_array(struct array *a, struct array *b);
/* Prototypes end here */


#endif
