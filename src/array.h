/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef ARRAY_H
#define ARRAY_H

#include "svalue.h"
#include "dmalloc.h"
#include "gc_header.h"

/* This debug tool writes out messages whenever arrays with unfinished
 * type fields are encountered. */
/* #define TRACE_UNFINISHED_TYPE_FIELDS */

/**
 * A Pike array is represented as a 'struct array' with all the
 * needed svalues malloced in the same block.
 *
 * @see type_field
 */
struct array
{
  GC_MARKER_MEMBERS;
  INT32 size;		/**< number of svalues in this array */
  INT32 malloced_size;	/**< number of svalues that can fit in this array */
  TYPE_FIELD type_field;/**< A bitfield with one bit for each type of
         	* data in this array.
                * Bits can be set that don't exist in the array. type_field is
                * initialized to BIT_MIXED|BIT_UNFINISHED for newly allocated
                * arrays so that they can be modified without having to update
                * this. It should be set accurately when that's done, though.
                */
  INT16 flags;          /**< ARRAY_* flags */

  struct array *next;	/**< we need to keep track of all arrays */
  struct array *prev;	/**< Another pointer, so we don't have to search
			 * when freeing arrays */
  struct svalue *item;  /**< the array of svalues */
  union {
    struct svalue real_item[1];
#if (SIZEOF_CHAR_P == 8) && (SIZEOF_LONG_DOUBLE == 16)
    /* Force the real_item array to be 16-byte aligned, so that we can
     * use single 16-byte operations to access the elements.
     */
    long double force_align16;
#else
    double force_align8;
#endif
  } u;
};

#define ARRAY_WEAK_FLAG 1
#define ARRAY_CYCLIC 2
#define ARRAY_LVALUE 4

PMOD_EXPORT extern struct array empty_array, weak_empty_array;
extern struct array *first_array;
extern struct array *gc_internal_array;

#if defined(DEBUG_MALLOC) && defined(PIKE_DEBUG)
#define ITEM(X) (((struct array *)(debug_malloc_pass((X))))->item)
#else
#define ITEM(X) ((X)->item)
#endif

/* These are arguments for the function 'merge' which merges two sorted
 * set stored in arrays in the way you specify
 */
#define PIKE_ARRAY_OP_A 1
#define PIKE_ARRAY_OP_SKIP_A 2
#define PIKE_ARRAY_OP_TAKE_A 3
#define PIKE_ARRAY_OP_B 4
#define PIKE_ARRAY_OP_SKIP_B 8
#define PIKE_ARRAY_OP_TAKE_B 12
#define PIKE_MINTERM(X,Y,Z) (((X)<<8)+((Y)<<4)+(Z))

#define PIKE_ARRAY_OP_AND PIKE_MINTERM(PIKE_ARRAY_OP_SKIP_A,PIKE_ARRAY_OP_SKIP_A | PIKE_ARRAY_OP_TAKE_B,PIKE_ARRAY_OP_SKIP_B)
#define PIKE_ARRAY_OP_AND_LEFT PIKE_MINTERM(PIKE_ARRAY_OP_SKIP_A,PIKE_ARRAY_OP_SKIP_B | PIKE_ARRAY_OP_TAKE_A,PIKE_ARRAY_OP_SKIP_B)
#define PIKE_ARRAY_OP_OR  PIKE_MINTERM(PIKE_ARRAY_OP_TAKE_A,PIKE_ARRAY_OP_SKIP_A | PIKE_ARRAY_OP_TAKE_B,PIKE_ARRAY_OP_TAKE_B)
#define PIKE_ARRAY_OP_OR_LEFT  PIKE_MINTERM(PIKE_ARRAY_OP_TAKE_A,PIKE_ARRAY_OP_SKIP_B | PIKE_ARRAY_OP_TAKE_A,PIKE_ARRAY_OP_TAKE_B)
#define PIKE_ARRAY_OP_XOR PIKE_MINTERM(PIKE_ARRAY_OP_TAKE_A,PIKE_ARRAY_OP_SKIP_A | PIKE_ARRAY_OP_SKIP_B,PIKE_ARRAY_OP_TAKE_B)
#define PIKE_ARRAY_OP_ADD PIKE_MINTERM(PIKE_ARRAY_OP_TAKE_A,PIKE_ARRAY_OP_TAKE_A | PIKE_ARRAY_OP_TAKE_B ,PIKE_ARRAY_OP_TAKE_B)
#define PIKE_ARRAY_OP_SUB PIKE_MINTERM(PIKE_ARRAY_OP_TAKE_A,PIKE_ARRAY_OP_SKIP_A ,PIKE_ARRAY_OP_SKIP_B)

/**
 *  This macro frees one reference to the array V. If the reference
 *  is the last reference to the array, it will free the memory used
 *  by the array V.
 *  @param V an array struct to be freed.
 */
#define free_array(V) do{						\
    struct array *v_=(V);						\
    debug_malloc_touch(v_);						\
    DO_IF_DEBUG (							\
      DO_IF_PIKE_CLEANUP (						\
	if (gc_external_refs_zapped)					\
	  gc_check_zapped (v_, PIKE_T_ARRAY, __FILE__, __LINE__)));	\
    if(!sub_ref(v_))							\
      really_free_array(v_);						\
  }while(0)

#define low_allocate_array(size, extra_space)				\
  dmalloc_touch (struct array *,					\
		 real_allocate_array ((size), (extra_space)))
#define allocate_array(X) low_allocate_array((X),0)
#define allocate_array_no_init(X,Y) low_allocate_array((X),(Y))

/* Special value used for cmpfuns to signify that two values aren't
 * equal and have no order relation, i.e. one is neither greater nor
 * less than the other. It consists of the largest bit (under the sign
 * bit) set. */
#define CMPFUN_UNORDERED (INT_MAX - (INT_MAX >> 1))

typedef int (*cmpfun)(const struct svalue *, const struct svalue *);
typedef int (*short_cmpfun)(union anything *, union anything *);
typedef short_cmpfun (*cmpfun_getter)(TYPE_T);

/* Prototypes begin here */
PMOD_EXPORT struct array *real_allocate_array(ptrdiff_t size, ptrdiff_t extra_space);
PMOD_EXPORT void really_free_array(struct array *v);
PMOD_EXPORT void do_free_array(struct array *a);
PMOD_EXPORT void clear_array(struct array *a);
PMOD_EXPORT struct array *array_set_flags(struct array *a, int flags);
PMOD_EXPORT void array_index(struct svalue *s,struct array *v,INT32 ind);
PMOD_EXPORT struct array *array_column (struct array *data, struct svalue *index,
					int destructive);
PMOD_EXPORT void simple_array_index_no_free(struct svalue *s,
				struct array *a,struct svalue *ind);
PMOD_EXPORT void array_free_index(struct array *v,INT32 ind);
PMOD_EXPORT void simple_set_index(struct array *a,struct svalue *ind,struct svalue *s);
PMOD_EXPORT void array_atomic_get_set(struct array *a, INT32 i,
				      struct svalue *from_to);
PMOD_EXPORT struct array *array_insert(struct array *v,struct svalue *s,INT32 ind);
void o_append_array(INT32 args);
PMOD_EXPORT struct array *resize_array(struct array *a, INT32 size);
PMOD_EXPORT struct array *array_shrink(struct array *v, ptrdiff_t size);
PMOD_EXPORT struct array *array_remove(struct array *v,INT32 ind);
PMOD_EXPORT ptrdiff_t array_search(struct array *v, const struct svalue *s,
				   ptrdiff_t start);
PMOD_EXPORT struct array *slice_array(struct array *v, ptrdiff_t start,
				      ptrdiff_t end);
PMOD_EXPORT struct array *friendly_slice_array(struct array *v,
					       ptrdiff_t start,
					       ptrdiff_t end);
PMOD_EXPORT struct array *copy_array(struct array *v);
PMOD_EXPORT void check_array_for_destruct(struct array *v);
PMOD_EXPORT INT32 array_find_destructed_object(struct array *v);
INT32 *get_order(struct array *v, cmpfun fun);
int set_svalue_cmpfun(const struct svalue *a, const struct svalue *b);
int alpha_svalue_cmpfun(const struct svalue *a, const struct svalue *b);
PMOD_EXPORT void sort_array_destructively(struct array *v);
PMOD_EXPORT INT32 *stable_sort_array_destructively(struct array *v);
PMOD_EXPORT INT32 *get_set_order(struct array *a);
PMOD_EXPORT INT32 *get_switch_order(struct array *a);
PMOD_EXPORT INT32 *get_alpha_order(struct array *a);
INT32 set_lookup(struct array *a, struct svalue *s);
INT32 switch_lookup(struct svalue *table, struct svalue *s);
PMOD_EXPORT struct array *order_array(struct array *v, const INT32 *order);
PMOD_EXPORT struct array *reorder_and_copy_array(const struct array *v, const INT32 *order);
PMOD_EXPORT TYPE_FIELD array_fix_type_field(struct array *v);
#ifdef PIKE_DEBUG
PMOD_EXPORT void array_check_type_field(const struct array *v);
#endif
PMOD_EXPORT union anything *low_array_get_item_ptr(struct array *a,
				       INT32 ind,
				       TYPE_T t);
PMOD_EXPORT union anything *array_get_item_ptr(struct array *a,
				   struct svalue *ind,
				   TYPE_T t);
INT32 * merge(struct array *a,struct array *b,INT32 opcode);
PMOD_EXPORT struct array *array_zip(struct array *a, struct array *b,INT32 *zipper);
PMOD_EXPORT struct array *add_arrays(struct svalue *argp, INT32 args);
PMOD_EXPORT int array_equal_p(struct array *a, struct array *b, struct processing *p);
PMOD_EXPORT struct array *merge_array_with_order(struct array *a,
						 struct array *b, INT32 op);
PMOD_EXPORT struct array *subtract_array_svalue(struct array *a,
						struct svalue *b);
PMOD_EXPORT struct array *subtract_arrays(struct array *a, struct array *b);
PMOD_EXPORT struct array *and_arrays(struct array *a, struct array *b);
int array_is_constant(struct array *a,
		      struct processing *p);
node *make_node_from_array(struct array *a);
PMOD_EXPORT void push_array_items(struct array *a);
void describe_array_low(struct byte_buffer *buf, struct array *a, struct processing *p, int indent);
#ifdef PIKE_DEBUG
void simple_describe_array(struct array *a);
void describe_index(struct array *a,
		    int e,
		    struct processing *p,
		    int indent);
#endif
void describe_array(struct byte_buffer *buf,struct array *a,struct processing *p,int indent);
PMOD_EXPORT struct array *aggregate_array(INT32 args);
PMOD_EXPORT struct array *append_array(struct array *a, struct svalue *s);
PMOD_EXPORT struct array *explode(struct pike_string *str,
		       struct pike_string *del);
PMOD_EXPORT struct pike_string *implode(struct array *a,struct pike_string *del);
PMOD_EXPORT struct array *copy_array_recursively(struct array *a,
						 struct mapping *p);
PMOD_EXPORT void apply_array(struct array *a, INT32 args, int flags);
PMOD_EXPORT struct array *reverse_array(struct array *a, int start, int end);
void array_replace(struct array *a,
		   struct svalue *from,
		   struct svalue *to);
ptrdiff_t do_gc_weak_array(struct array *a);
#ifdef PIKE_DEBUG
PMOD_EXPORT void check_array(struct array *a);
void check_all_arrays(void);
#endif
PMOD_EXPORT void visit_array (struct array *a, int action, void *extra);
PMOD_EXPORT void gc_mark_array_as_referenced(struct array *a);
PMOD_EXPORT void real_gc_cycle_check_array(struct array *a, int weak);
unsigned gc_touch_all_arrays(void);
void gc_check_all_arrays(void);
void gc_mark_all_arrays(void);
void gc_cycle_check_all_arrays(void);
void gc_zap_ext_weak_refs_in_arrays(void);
size_t gc_free_all_unreferenced_arrays(void);
#ifdef PIKE_DEBUG
void debug_dump_type_field(TYPE_FIELD t);
void debug_dump_array(struct array *a);
#endif
void count_memory_in_arrays(size_t *num_, size_t *size_);
PMOD_EXPORT struct array *explode_array(struct array *a, struct array *b);
PMOD_EXPORT struct array *implode_array(struct array *a, struct array *b);

/* Automap internals. */
void assign_array_level_value( struct array *a, struct svalue *b, int level );
void assign_array_level( struct array *a, struct array *b, int level );
/* Prototypes end here */

#define array_get_flags(a) ((a)->flags)

#define visit_array_ref(A, REF_TYPE, EXTRA)				\
  visit_ref (pass_array (A), (REF_TYPE),				\
	     (visit_thing_fn *) &visit_array, (EXTRA))
#define gc_cycle_check_array(X, WEAK) \
  gc_cycle_enqueue((gc_cycle_check_cb *) real_gc_cycle_check_array, (X), (WEAK))


/** Macros for aggregating results built on the stack into an array,
 * while maintaining a bound on stack consumption. Use like this:
 *
 * check_stack(120);
 * BEGIN_AGGREGATE_ARRAY(estimated_size_of_final_array) {
 *   for (...) {
 *     ... stuff that produces a value on the stack ...
 *     DO_AGGREGATE_ARRAY(120);
 *     ...
 *   }
 * } END_AGGREGATE_ARRAY;
 *
 * The array is left on top of the stack.
 */

#define AGGR_ARR_PROLOGUE(base_sval, estimated_size) do {		\
    push_array(allocate_array_no_init(0, (estimated_size)));		\
    base_sval = Pike_sp;						\
    if (base_sval[-1].u.array->malloced_size)				\
      base_sval[-1].u.array->type_field = (BIT_MIXED | BIT_UNFINISHED);	\
  } while (0)

#define AGGR_ARR_CHECK(base_sval, max_keep_on_stack)			\
  do {									\
    ptrdiff_t diff__ = Pike_sp - base_sval;				\
    if (diff__ > (max_keep_on_stack)) {					\
      INT32 oldsize__ = base_sval[-1].u.array->size;			\
      ACCEPT_UNFINISHED_TYPE_FIELDS {					\
	base_sval[-1].u.array =						\
	  resize_array(base_sval[-1].u.array, oldsize__ + diff__);	\
      } END_ACCEPT_UNFINISHED_TYPE_FIELDS;				\
      /* Unless the user does something, the type field will contain */	\
      /* BIT_MIXED|BIT_UNFINISHED from the allocation above. */		\
      memcpy(ITEM(base_sval[-1].u.array) + oldsize__,                   \
	     base_sval, diff__ * sizeof(struct svalue));                \
      if (!oldsize__) {							\
	/* Make sure that BIT_UNFINISHED is set. */			\
	base_sval[-1].u.array->type_field |=				\
	  BIT_MIXED | BIT_UNFINISHED;					\
      }									\
      Pike_sp = base_sval;						\
      DO_IF_DMALLOC(while(diff__--) {					\
	  dmalloc_touch_svalue(Pike_sp + diff__);			\
	});								\
    }									\
  } while (0)

#define AGGR_ARR_EPILOGUE(base_sval) do {				\
    ptrdiff_t diff__ = Pike_sp - base_sval;				\
    if (!diff__) {							\
      if (!base_sval[-1].u.array->size) {				\
	free_array (base_sval[-1].u.array);				\
	add_ref (base_sval[-1].u.array = &empty_array);			\
      }									\
    }									\
    else								\
      AGGR_ARR_CHECK (base_sval, 0);					\
    DO_IF_DEBUG(if (Pike_sp != base_sval) {				\
	Pike_fatal("Lost track of stack depth when aggregating.\n");	\
      });								\
    DO_IF_DEBUG(if (TYPEOF(Pike_sp[-1]) != T_ARRAY) {			\
	Pike_fatal("Lost track of aggregated array.\n");		\
      });								\
    if (base_sval[-1].u.array->type_field & BIT_UNFINISHED)		\
      array_fix_type_field(Pike_sp[-1].u.array);			\
  } while (0)

#define BEGIN_AGGREGATE_ARRAY(estimated_size) do {			\
    struct svalue *base__;						\
    AGGR_ARR_PROLOGUE (base__, estimated_size);

#define DO_AGGREGATE_ARRAY(max_keep_on_stack)				\
    AGGR_ARR_CHECK (base__, max_keep_on_stack)

#define END_AGGREGATE_ARRAY						\
    AGGR_ARR_EPILOGUE (base__);						\
  } while (0)


/**
 * Extract an svalue from an array
 */
#define array_index_no_free(S,A,I) do {				\
  INT32 ind_=(I);						\
  struct array *v_=(A);						\
  DO_IF_DEBUG(							\
    if(ind_<0 || ind_>=v_->size)				\
    Pike_fatal("Illegal index in low level index routine.\n");	\
    )								\
								\
  assign_svalue_no_free((S), ITEM(v_) + ind_);			\
}while(0)


/**
 * Sets an index in an array.
 *
 * @param V the array to modify
 * @param I the index of the array to set
 * @param S the svalue to set
 */
#define array_set_index_no_free(V,I,S) do {				\
  struct array *v_=(V);							 \
  INT32 index_=(I);							 \
  struct svalue *s_=(S);						 \
									 \
  DO_IF_DEBUG(								 \
  if(index_<0 || index_>v_->size)					 \
    Pike_fatal("Illegal index in low level array set routine.\n");	 \
    )									 \
									 \
  check_destructed(s_);							 \
									 \
  v_->type_field |= 1 << TYPEOF(*s_);					 \
  assign_svalue_no_free( ITEM(v_) + index_, s_);			 \
}while(0)
#define array_set_index(V,I,S) do {					\
  struct array *v_=(V);							 \
  INT32 index_=(I);							 \
  struct svalue *s_=(S);						 \
									 \
  DO_IF_DEBUG(								 \
  if(index_<0 || index_>v_->size)					 \
    Pike_fatal("Illegal index in low level array set routine.\n");	 \
    )									 \
									 \
  check_destructed(s_);							 \
									 \
  v_->type_field |= 1 << TYPEOF(*s_);					 \
  assign_svalue( ITEM(v_) + index_, s_);				\
}while(0)

#define array_fix_unfinished_type_field(A) do {				\
    struct array *a_ = (A);						\
    if (a_->type_field & BIT_UNFINISHED) {				\
      DO_IF_DEBUG (array_check_type_field (a_));			\
      array_fix_type_field (a_);					\
    }									\
  } while (0)

#ifdef TRACE_UNFINISHED_TYPE_FIELDS
/* Note: These macros don't support thread switches. */

extern PMOD_EXPORT int accept_unfinished_type_fields;
PMOD_EXPORT void dont_accept_unfinished_type_fields (void *orig);

#define ACCEPT_UNFINISHED_TYPE_FIELDS do {				\
    ONERROR autf_uwp_;							\
    int orig_autf_ = accept_unfinished_type_fields;			\
    accept_unfinished_type_fields++;					\
    SET_ONERROR (autf_uwp_, dont_accept_unfinished_type_fields,		\
		 (void *) orig_autf_);					\
    do

#define END_ACCEPT_UNFINISHED_TYPE_FIELDS				\
    while (0);								\
    accept_unfinished_type_fields = orig_autf_;				\
    UNSET_ONERROR (autf_uwp_);						\
  } while (0)

#else

#define ACCEPT_UNFINISHED_TYPE_FIELDS do
#define END_ACCEPT_UNFINISHED_TYPE_FIELDS while (0)

#endif

#endif /* ARRAY_H */
