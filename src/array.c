/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: array.c,v 1.190 2006/04/02 17:19:17 grubba Exp $
*/

#include "global.h"
#include "svalue.h"
#include "array.h"
#include "object.h"
#include "las.h"
#include "stralloc.h"
#include "interpret.h"
#include "opcodes.h"
#include "pike_error.h"
#include "pike_types.h"
#include "fsort.h"
#include "builtin_functions.h"
#include "pike_memory.h"
#include "gc.h"
#include "main.h"
#include "pike_security.h"
#include "stuff.h"
#include "bignum.h"
#include "cyclic.h"
#include "multiset.h"
#include "mapping.h"

/** The empty array. */
PMOD_EXPORT struct array empty_array=
{
  PIKE_CONSTANT_MEMOBJ_INIT(1), /* Never free */
  &weak_empty_array,     /* Next */
  0,			 /* previous */
  0,                     /* Size = 0 */
  0,                     /* malloced Size = 0 */
  0,                     /* no types */
  0,			 /* no flags */
  empty_array.real_item, /* Initialize the item pointer. */
#ifdef HAVE_UNION_INIT
  {{0, 0, {0}}}, /* Only to avoid warnings. */
#endif
};

/** The empty weak array. */
PMOD_EXPORT struct array weak_empty_array=
{
  PIKE_CONSTANT_MEMOBJ_INIT(1),
  0, &empty_array, 0, 0, 0, ARRAY_WEAK_FLAG,
  weak_empty_array.real_item,
#ifdef HAVE_UNION_INIT
  {{0, 0, {0}}}, /* Only to avoid warnings. */
#endif
};

struct array *first_array = &empty_array;
struct array *gc_internal_array = 0;
static struct array *gc_mark_array_pos;

#ifdef TRACE_UNFINISHED_TYPE_FIELDS
PMOD_EXPORT int accept_unfinished_type_fields = 0;
PMOD_EXPORT void dont_accept_unfinished_type_fields (void *orig)
{
  accept_unfinished_type_fields = (int) orig;
}
#endif


/**
 * Allocate an array. This might be changed in the future to allocate
 * linked lists or something. The new array has zero references.
 *
 * When building arrays, it is recommended that you push the values on
 * the stack and call aggregate_array or f_aggregate instead of
 * allocating and filling in the values 'by hand'.
 *
 * @param size The size of the new array, in elements.
 * @param extra_space The number of extra elements space
 * should be reserved for.
 * @return A pointer to the allocated array struct.
 */
PMOD_EXPORT struct array *real_allocate_array(ptrdiff_t size,
					      ptrdiff_t extra_space)
{
  struct array *v;
  ptrdiff_t e;

  if(size+extra_space == 0)
  {
    add_ref(&empty_array);
    return &empty_array;
  }

  /* Limits size to (1<<29)-4 */
  if( (size_t)(size+extra_space-1) >
      (LONG_MAX-sizeof(struct array))/sizeof(struct svalue) )
    Pike_error("Too large array (size %ld exceeds %ld).\n",
	       (long)(size+extra_space-1),
	       (long)((LONG_MAX-sizeof(struct array))/sizeof(struct svalue)) );
  v=(struct array *)malloc(sizeof(struct array)+
			   (size+extra_space-1)*sizeof(struct svalue));
  if(!v)
    Pike_error(msg_out_of_mem);

  GC_ALLOC(v);


  if (size+extra_space)
    /* for now, we don't know what will go in here */
    v->type_field = BIT_MIXED | BIT_UNFINISHED;
  else
    v->type_field = 0;
  v->flags=0;

  v->malloced_size = DO_NOT_WARN((INT32)(size + extra_space));
  v->item=v->real_item;
  v->size = DO_NOT_WARN((INT32)size);
  INIT_PIKE_MEMOBJ(v);
  DOUBLELINK (first_array, v);

  for(e=0;e<v->size;e++)
  {
    ITEM(v)[e].type=T_INT;
    ITEM(v)[e].subtype=NUMBER_NUMBER;
    ITEM(v)[e].u.integer=0;
  }

  return v;
}

/**
 * Free an array without freeing the values inside it.
 * Any values inside of the array will be kept.
 * @param v The array to be freed.
 */
static void array_free_no_free(struct array *v)
{
  DOUBLEUNLINK (first_array, v);

  free((char *)v);

  GC_FREE(v);
}

/**
 * Free an array. Call this when the array has zero references.
 * @param v The array to free.
 */
PMOD_EXPORT void really_free_array(struct array *v)
{
#ifdef PIKE_DEBUG
  if(v == & empty_array || v == &weak_empty_array)
    Pike_fatal("Tried to free some *_empty_array.\n");
  if (v->refs) {
#ifdef DEBUG_MALLOC
    describe_something(v, T_ARRAY, 0,2,0, NULL);
#endif
    Pike_fatal("Freeing array with %d refs.\n", v->refs);
  }
#endif

#ifdef PIKE_DEBUG
  if(d_flag > 1)  array_check_type_field(v);
#endif

  add_ref(v);
  EXIT_PIKE_MEMOBJ(v);
  free_svalues(ITEM(v), v->size, v->type_field);
  sub_ref(v);
  array_free_no_free(v);
}

/**
 *  Decrement the references (and free if unused) an array if it is not null.
 */
PMOD_EXPORT void do_free_array(struct array *a)
{
  if (a)
    free_array(a);
}

/**
 *  Set the flags on an array. If the array is empty then only the
 *  weak flag is significant.
 */
PMOD_EXPORT struct array *array_set_flags(struct array *a, int flags)
{
  if (a->size)
    a->flags = flags;
  else {
    free_array(a);
    if (flags & ARRAY_WEAK_FLAG)
      add_ref(a = &weak_empty_array);
    else
      add_ref(a = &empty_array);
  }
  return a;
}


/**
 * Extract an svalue from an array. This function frees the contents of 
 * of the svalue 's' and replaces it with a copy of the 
 * contents from index 'index' in the array 'v'.
 *
 * @param index The index of the array to be extracted.
 * @param s The recipient of the extracted array element.
 * @param v The array to extract the element from.
 *
 * This function is similar to
 *    assign_svalue(s, v->item + n);
 * except that it adds debug and safety measures. Usually, this function
 * is not needed.
 *
 * @note If n is out of bounds, Pike will dump core. If Pike was compiled 
 * with DEBUG, a message will be written first stating what the problem was.
 */
PMOD_EXPORT void array_index(struct svalue *s,struct array *v,INT32 index)
{
#ifdef PIKE_DEBUG
  if(index<0 || index>=v->size)
    Pike_fatal("Illegal index in low level index routine.\n");
#endif

  add_ref(v);
  assign_svalue(s, ITEM(v) + index);
  free_array(v);
}

/** Is destructive on data if destructive is set and it only has one ref. */
PMOD_EXPORT struct array *array_column (struct array *data, struct svalue *index,
					int destructive)
{
  int e;
  struct array *a;
  TYPE_FIELD types = 0;

  DECLARE_CYCLIC();

  /* Optimization */
  if(data->refs == 1 && destructive)
  {
    /* An array with one ref cannot possibly be cyclic */
    struct svalue sval;
    data->type_field = BIT_MIXED | BIT_UNFINISHED;
    for(e=0;e<data->size;e++)
    {
      index_no_free(&sval, ITEM(data)+e, index);
      types |= 1 << sval.type;
      free_svalue(ITEM(data)+e);
      move_svalue (ITEM(data) + e, &sval);
    }
    data->type_field = types;
    add_ref (data);
    return data;
  }

  if((a=(struct array *)BEGIN_CYCLIC(data,0)))
  {
    add_ref(a);
  }else{
    push_array(a=allocate_array(data->size));
    SET_CYCLIC_RET(a);

    for(e=0;e<a->size;e++) {
      index_no_free(ITEM(a)+e, ITEM(data)+e, index);
      types |= 1 << ITEM(a)[e].type;
    }
    a->type_field = types;

    dmalloc_touch_svalue(Pike_sp-1);
    Pike_sp--;
  }
  END_CYCLIC();

  return a;
}

PMOD_EXPORT void simple_array_index_no_free(struct svalue *s,
				struct array *a,struct svalue *ind)
{
  switch(ind->type)
  {
    case T_INT: {
      INT_TYPE p = ind->u.integer;
      INT_TYPE i = p < 0 ? p + a->size : p;
      if(i<0 || i>=a->size) {
	struct svalue tmp;
	tmp.type=T_ARRAY;
	tmp.u.array=a;
	if (a->size) {
	  index_error(0,0,0,&tmp,ind,
		      "Index %"PRINTPIKEINT"d is out of array range "
		      "%d..%d.\n", p, -a->size, a->size-1);
	} else {
	  index_error(0,0,0,&tmp,ind,
		      "Attempt to index the empty array with %"PRINTPIKEINT"d.\n", p);
	}
      }
      array_index_no_free(s,a,i);
      break;
    }

    case T_STRING:
    {
      /* Set the type afterwards to avoid a clobbered svalue in case
       * array_column throws. */
      s->u.array = array_column (a, ind, 0);
      s->type = T_ARRAY;
      break;
    }
	
    default:
      {
	struct svalue tmp;
	tmp.type=T_ARRAY;
	tmp.u.array=a;
	index_error(0,0,0,&tmp,ind,"Array index is neither int nor string.\n");
      }
  }
}

/**
 * Extract an svalue from an array.
 */
PMOD_EXPORT void array_free_index(struct array *v,INT32 index)
{
#ifdef PIKE_DEBUG
  if(index<0 || index>=v->size)
    Pike_fatal("Illegal index in low level free index routine.\n");
#endif

  free_svalue(ITEM(v) + index);
}

/** set an element in an array to a value.
 *
 *  @param a the array whose element is to be set
 *  @param ind an int or string containing the index to set
 *  @param s the value to set
 */
PMOD_EXPORT void simple_set_index(struct array *a,struct svalue *ind,struct svalue *s)
{
  switch (ind->type) {
    case T_INT: {
      INT_TYPE p = ind->u.integer;
      INT_TYPE i = p < 0 ? p + a->size : p;
      if(i<0 || i>=a->size) {
	if (a->size) {
	  Pike_error("Index %"PRINTPIKEINT"d is out of array range "
		     "%d..%d.\n", p, -a->size, a->size-1);
	} else {
	  Pike_error("Attempt to index the empty array with %"PRINTPIKEINT"d.\n", p);
	}
      }
      array_set_index(a,i,s);
      break;
    }

    case T_STRING:
    {
      INT32 i, n;
      check_stack(2);
      Pike_sp++->type = T_VOID;
      push_svalue(ind);
      for (i = 0, n = a->size; i < n; i++) {
	assign_svalue(Pike_sp-2, &a->item[i]);
	assign_lvalue(Pike_sp-2, s);
      }
      pop_n_elems(2);
      break;
    }

    default:
    {
      struct svalue tmp;
      tmp.type=T_ARRAY;
      tmp.u.array=a;
      index_error(0,0,0,&tmp,ind,"Array index is neither int nor string.\n");
    }
  }
}

/**
 * Insert an svalue into an array and grow the array if nessesary.
 */
PMOD_EXPORT struct array *array_insert(struct array *v,struct svalue *s,INT32 index)
{
#ifdef PIKE_DEBUG
  if(index<0 || index>v->size)
    Pike_fatal("Illegal index in low level insert routine.\n");
#endif

  /* Can we fit it into the existing block? */
  if(v->refs<=1 && v->malloced_size > v->size)
  {
    MEMMOVE((char *)(ITEM(v)+index+1),
	    (char *)(ITEM(v)+index),
	    (v->size-index) * sizeof(struct svalue));
    ITEM(v)[index].type=T_INT;
#ifdef __CHECKER__
    ITEM(v)[index].subtype=0;
    ITEM(v)[index].u.refs=0;
#endif
    v->size++;
  }else{
    struct array *ret;

    ret = array_set_flags(allocate_array_no_init(v->size+1, (v->size >> 3) + 1),
			  v->flags);
    ret->type_field = v->type_field;

    MEMCPY(ITEM(ret), ITEM(v), sizeof(struct svalue) * index);
    MEMCPY(ITEM(ret)+index+1, ITEM(v)+index, sizeof(struct svalue) * (v->size-index));
    ITEM(ret)[index].type=T_INT;
#ifdef __CHECKER__
    ITEM(ret)[index].subtype=0;
    ITEM(ret)[index].u.refs=0;
#endif
    v->size=0;
    free_array(v);
    v=ret;
  }

  array_set_index(v,index,s);

  return v;
}

/**
 * Shrink an array destructively
 */
PMOD_EXPORT struct array *array_shrink(struct array *v, ptrdiff_t size)
{
  struct array *a;

#ifdef PIKE_DEBUG
  if(v->refs>2) /* Odd, but has to be two */
    Pike_fatal("Array shrink on array with many references.\n");

  if(size > v->size)
    Pike_fatal("Illegal argument to array_shrink.\n");
#endif

  if (size == v->size) return v;

  if( !size )
  {
    free_array(v);
    /* FIXME: What about weak markers etc? */
    add_ref(&empty_array);
    return &empty_array;
  }

  /* Free items outside the new array. */
  free_svalues(ITEM(v) + size, v->size - size, v->type_field);
  v->size=size;

  if(size*4 < v->malloced_size + 4) /* Should we realloc it? */
  {
    a = array_set_flags(allocate_array_no_init(size, 0), v->flags);
    if (a->size) {
      a->type_field = v->type_field;
    }

    MEMCPY(ITEM(a), ITEM(v), size*sizeof(struct svalue));
    v->size=0;
    free_array(v);
    return a;
  }else{
    return v;
  }
}

/**
 * Resize an array destructively.
 */
PMOD_EXPORT struct array *resize_array(struct array *a, INT32 size)
{
#ifdef PIKE_DEBUG
  if(d_flag > 1)  array_check_type_field(a);
#endif

  if(a->size == size) return a;
  if(size > a->size)
  {
    /* We should grow the array */

    if(a->malloced_size >= size)
    {
      for(;a->size < size; a->size++)
      {
	ITEM(a)[a->size].type=T_INT;
	ITEM(a)[a->size].subtype=NUMBER_NUMBER;
	ITEM(a)[a->size].u.integer=0;
      }
      a->type_field |= BIT_INT;
      return a;
    } else {
      struct array *ret;
      ret = array_set_flags(low_allocate_array(size, (size>>1) + 4),
			    a->flags);
      MEMCPY(ITEM(ret), ITEM(a), sizeof(struct svalue)*a->size);
      ret->type_field = DO_NOT_WARN((TYPE_FIELD)(a->type_field | BIT_INT));
      a->size=0;
      free_array(a);
      return ret;
    }
  } else {
    return array_shrink(a, size);
  }
}

/**
 * Remove an index from an array and shrink the array destructively.
 * Because this function is destructive, and might free the region for 'v',
 * do not use this function on arrays that might have been sent to a 
 * Pike function.
 *
 * @param v The array to operate on.
 * @param index The index of the element to remove
 * @return a new array with the contents of the input minus the removed index.
 */
PMOD_EXPORT struct array *array_remove(struct array *v,INT32 index)
{
  struct array *a;

#ifdef PIKE_DEBUG
  if(index<0 || index >= v->size)
    Pike_fatal("Illegal argument to array_remove.\n");
#endif

  array_free_index(v, index);
  if (v->size == 1) {
    v->size = 0;
    /* NOTE: The following uses the fact that array_set_flags()
     *       will reallocate the array if it has zero size!
     */
    return array_set_flags(v, v->flags);
  } else if(v->size*4 + 4 < v->malloced_size ) /* Should we realloc it? */
  {
    a = array_set_flags(allocate_array_no_init(v->size-1, 0), v->flags);
    a->type_field = v->type_field;

    if(index>0)
      MEMCPY(ITEM(a), ITEM(v), index*sizeof(struct svalue));
    if(v->size-index>1)
      MEMCPY(ITEM(a)+index,
	     ITEM(v)+index+1,
	     (v->size-index-1)*sizeof(struct svalue));
    v->size=0;
    free_array(v);
    return a;
  } else {
    if(v->size-index>1)
    {
      MEMMOVE((char *)(ITEM(v)+index),
	      (char *)(ITEM(v)+index+1),
	      (v->size-index-1)*sizeof(struct svalue));
    }
    v->size--;
    return v;
  }
}

/**
 * Search for in svalue in an array.
 * @param v the array to search
 * @param s the value to search for
 * @param start the index to start search at
 * @return the index if found, -1 otherwise
 */
PMOD_EXPORT ptrdiff_t array_search(struct array *v, struct svalue *s,
				   ptrdiff_t start)
{
  ptrdiff_t e;

#ifdef PIKE_DEBUG
  if(start<0)
    Pike_fatal("Start of find_index is less than zero.\n");
#endif

  check_destructed(s);

#ifdef PIKE_DEBUG
  if(d_flag > 1)  array_check_type_field(v);
#endif
  /* Why search for something that is not there? 
   * however, we must explicitly check for searches
   * for destructed objects/functions
   */
  if((v->type_field & (1 << s->type))  ||
     (UNSAFE_IS_ZERO(s) && (v->type_field & (BIT_FUNCTION|BIT_OBJECT))) ||
     ( (v->type_field | (1<<s->type))  & BIT_OBJECT )) /* for overloading */
  {
    if(start)
    {
      for(e=start;e<v->size;e++)
	if(is_eq(ITEM(v)+e,s)) return e;
    }else{
      TYPE_FIELD t=0;
      for(e=0;e<v->size;e++)
      {
	if(is_eq(ITEM(v)+e,s)) return e;
	t |= 1<<ITEM(v)[e].type;
      }
      v->type_field=t;
    }
  }
  return -1;
}

/**
 * Slice a piece of an array (nondestructively)
 * @param v the array to slice
 * @param start the beginning element to be included
 * @param end the element beyond the end of the slice
 * @return an array consisting of v[start..end-1]
 */
PMOD_EXPORT struct array *slice_array(struct array *v, ptrdiff_t start,
				      ptrdiff_t end)
{
  struct array *a;

#ifdef PIKE_DEBUG
  if(start > end || end>v->size || start<0)
    Pike_fatal("Illegal arguments to slice_array()\n");

  if(d_flag > 1)  array_check_type_field(v);
#endif

#if 1
  if(v->refs==1)	/* Can we use the same array? */
  {
    if((end-start) &&
       (end-start)*2 >
       v->malloced_size+4+(v->item-v->real_item) ) /* don't waste too much memory */
    {
      add_ref(v);
      free_svalues(ITEM(v) + end, v->size - end, v->type_field);
      free_svalues(ITEM(v), start, v->type_field);
      v->item+=start;
      v->malloced_size-=start;
      v->size=end-start;
#ifdef PIKE_DEBUG
      if(d_flag>1)
	check_array(v);
#endif
      return v;
    }
  }
#endif

  a=allocate_array_no_init(end-start,0);
  if (end-start) {
    a->type_field = v->type_field;

    assign_svalues_no_free(ITEM(a), ITEM(v)+start, end-start, v->type_field);
  }

  return a;
}

/**
 * Slice a piece of an array (nondestructively).
 * @return an array consisting of v[start..end-1]
 */
PMOD_EXPORT struct array *friendly_slice_array(struct array *v,
					       ptrdiff_t start,
					       ptrdiff_t end)
{
  struct array *a;

#ifdef PIKE_DEBUG
  if(start > end || end>v->size || start<0)
    Pike_fatal("Illegal arguments to slice_array()\n");

  if(d_flag > 1)  array_check_type_field(v);
#endif

  a=allocate_array_no_init(end-start,0);
  a->type_field = v->type_field;

  assign_svalues_no_free(ITEM(a), ITEM(v)+start, end-start, v->type_field);

  return a;
}

/**
 * Copy an array.
 * @param v the array to be copied.
 * @returns the copy of the input array.
 */
PMOD_EXPORT struct array *copy_array(struct array *v)
{
  struct array *a;

  if (!v->size) {
    /* Empty array. */
    add_ref(&empty_array);
    return &empty_array;
  }

  a=allocate_array_no_init(v->size, 0);
  a->type_field = v->type_field;

  assign_svalues_no_free(ITEM(a), ITEM(v), v->size, v->type_field);

  return a;
}

/**
 * Clean an array from destructed objects.
 */
PMOD_EXPORT void check_array_for_destruct(struct array *v)
{
  int e;
  INT16 types;

  types = 0;
#ifdef PIKE_DEBUG
  if(d_flag > 1)  array_check_type_field(v);
#endif
  if(v->type_field & (BIT_OBJECT | BIT_FUNCTION))
  {
    for(e=0; e<v->size; e++)
    {
      if((ITEM(v)[e].type == T_OBJECT ||
	  (ITEM(v)[e].type == T_FUNCTION &&
	   ITEM(v)[e].subtype!=FUNCTION_BUILTIN)) &&
	 (!ITEM(v)[e].u.object->prog))
      {
	free_svalue(ITEM(v)+e);
	ITEM(v)[e].type=T_INT;
	ITEM(v)[e].subtype=NUMBER_DESTRUCTED;
	ITEM(v)[e].u.integer=0;

	types |= BIT_INT;
      }else{
	types |= 1<<ITEM(v)[e].type;
      }
    }
    v->type_field = types;
  }
}

/**
 * This function finds the index of any destructed object in a set.
 * It could be optimized to search out the object part with a binary
 * search lookup if the array is mixed.
 */
PMOD_EXPORT INT32 array_find_destructed_object(struct array *v)
{
  INT32 e;
  TYPE_FIELD types;
#ifdef PIKE_DEBUG
  if(d_flag > 1)  array_check_type_field(v);
#endif
  if(v->type_field & (BIT_OBJECT | BIT_FUNCTION))
  {
    types=0;
    for(e=0; e<v->size; e++)
    {
      if((ITEM(v)[e].type == T_OBJECT ||
	  (ITEM(v)[e].type == T_FUNCTION &&
	   ITEM(v)[e].subtype!=FUNCTION_BUILTIN)) &&
	 (!ITEM(v)[e].u.object->prog))
	return e;
      types |= 1<<ITEM(v)[e].type;
    }
    v->type_field = types;
  }
#ifdef PIKE_DEBUG
  if(d_flag > 1)  array_check_type_field(v);
#endif
  return -1;
}

static int internal_cmpfun(INT32 *a,
			   INT32 *b,
			   cmpfun current_cmpfun,
			   struct svalue *current_array_p)
{
  int res = current_cmpfun(current_array_p + *a, current_array_p + *b);
  /* If the comparison considers the elements equal we compare their
   * positions. Thus we get a stable sort function. */
  return res ? res : *a - *b;
}

#define CMP(X,Y) internal_cmpfun((X),(Y),current_cmpfun, current_array_p)
#define TYPE INT32
#define ID get_order_fsort
#define EXTRA_ARGS ,cmpfun current_cmpfun, struct svalue *current_array_p
#define XARGS ,current_cmpfun, current_array_p
#include "fsort_template.h"
#undef CMP
#undef TYPE
#undef ID
#undef EXTRA_ARGS
#undef XARGS

/* The sort is stable. */
INT32 *get_order(struct array *v, cmpfun fun)
{
  INT32 e, *current_order;
  ONERROR tmp;

  if(!v->size) return 0;

  /* Overlow safe: ((1<<29)-4)*4 < ULONG_MAX */
  current_order=(INT32 *)xalloc(v->size * sizeof(INT32));
  SET_ONERROR(tmp, free, current_order);
  for(e=0; e<v->size; e++) current_order[e]=e;

  get_order_fsort(current_order,
		  current_order+v->size-1,
		  fun,
		  ITEM(v));

  UNSET_ONERROR(tmp);
  return current_order;
}

/* Returns 2 if no relation is established through lfun calls. */
static INLINE int lfun_cmp (const struct svalue *a, const struct svalue *b)
{
  struct program *p;
  int fun;

  if (a->type == T_OBJECT && (p = a->u.object->prog)) {
    if ((fun = FIND_LFUN(p->inherits[a->subtype].prog, LFUN_LT)) != -1) {
      push_svalue(b);
      apply_low(a->u.object,
		fun + p->inherits[a->subtype].identifier_level, 1);
      if(!UNSAFE_IS_ZERO(Pike_sp-1))
      {
	pop_stack();
	return -1;
      }
      pop_stack();
    }

    if ((fun = FIND_LFUN(p->inherits[a->subtype].prog, LFUN_GT)) != -1) {
      push_svalue(b);
      apply_low(a->u.object,
		fun + p->inherits[a->subtype].identifier_level, 1);
      if(!UNSAFE_IS_ZERO(Pike_sp-1))
      {
	pop_stack();
	return 1;
      }
      pop_stack();
    }

    if ((fun = FIND_LFUN(p->inherits[a->subtype].prog, LFUN_EQ)) != -1) {
      push_svalue(b);
      apply_low(a->u.object,
		fun + p->inherits[a->subtype].identifier_level, 1);
      if (!UNSAFE_IS_ZERO(Pike_sp-1)) {
	pop_stack();
	return 0;
      }
      pop_stack();
    }
  }

  if(b->type == T_OBJECT && (p = b->u.object->prog)) {
    if ((fun = FIND_LFUN(p->inherits[b->subtype].prog, LFUN_LT)) != -1) {
      push_svalue(a);
      apply_low(b->u.object,
		fun + p->inherits[b->subtype].identifier_level, 1);
      if(!UNSAFE_IS_ZERO(Pike_sp-1))
      {
	pop_stack();
	return 1;
      }
      pop_stack();
    }

    if ((fun = FIND_LFUN(p->inherits[b->subtype].prog, LFUN_GT)) != -1) {
      push_svalue(a);
      apply_low(b->u.object,
		fun + p->inherits[b->subtype].identifier_level, 1);
      if(!UNSAFE_IS_ZERO(Pike_sp-1))
      {
	pop_stack();
	return -1;
      }
      pop_stack();
    }

    if ((fun = FIND_LFUN(p->inherits[b->subtype].prog, LFUN_EQ)) != -1) {
      push_svalue(a);
      apply_low(b->u.object,
		fun + p->inherits[b->subtype].identifier_level, 1);
      if (!UNSAFE_IS_ZERO(Pike_sp-1)) {
	pop_stack();
	return 0;
      }
      pop_stack();
    }
  }

  return 2;
}

INLINE int set_svalue_cmpfun(const struct svalue *a, const struct svalue *b)
{
  int res, typediff = a->type - b->type;

  if (!typediff)
  {
    switch(a->type)
    {
      case T_FLOAT:
	if(a->u.float_number < b->u.float_number) return -1;
	if(a->u.float_number > b->u.float_number) return 1;
	return 0;
	
      case T_FUNCTION:
	if(a->u.refs < b->u.refs) return -1;
	if(a->u.refs > b->u.refs) return 1;
	return a->subtype - b->subtype;
	
      case T_INT:
	if(a->u.integer < b->u.integer) return -1;
	if(a->u.integer > b->u.integer) return 1;
	return 0;

      default:
	if(a->u.refs < b->u.refs) return -1;
	if(a->u.refs > b->u.refs) return 1;
	return 0;

      case T_OBJECT:
	if((a->u.object == b->u.object) && (a->subtype == b->subtype)) {
	  return 0;
	}
	break;
    }
  }

  res = lfun_cmp (a, b);
  if (res != 2) return res;

  if (typediff) return typediff;

#ifdef PIKE_DEBUG
  if (a->type != T_OBJECT)
    Pike_fatal ("Expected objects when both types are the same.\n");
#endif

  /* FIXME: Take subtype into account! */

  if(a->u.object->prog == b->u.object->prog) {
    if (a->u.object->prog) {
      if(a->u.object < b->u.object) {
	return -1;
      } else {
	return 1;
      }
    } else {
      /* Destructed objects are considered equal. */
      return 0;
    }
  } else {
    /* Attempt to group objects cloned from the same program */
    if (a->u.object->prog < b->u.object->prog) {
      return -1;
    } else {
      return 1;
    }
  }
}

static int switch_svalue_cmpfun(const struct svalue *a, const struct svalue *b)
{
  if(a->type == b->type)
  {
    switch(a->type)
    {
      case T_INT:
	if(a->u.integer < b->u.integer) return -1;
	if(a->u.integer > b->u.integer) return 1;
	return 0;
	
      case T_FLOAT:
	if(a->u.float_number < b->u.float_number) return -1;
	if(a->u.float_number > b->u.float_number) return 1;
	return 0;
	
      case T_STRING:
	return DO_NOT_WARN((int)my_quick_strcmp(a->u.string, b->u.string));
	
      default:
	return set_svalue_cmpfun(a,b);
    }
  }else{
    return a->type - b->type;
  }
}

static int alpha_svalue_cmpfun(const struct svalue *a, const struct svalue *b)
{
  int res, typediff = a->type - b->type;

  if (!typediff)
  {
    switch(a->type)
    {
      case T_INT:
	if(a->u.integer < b->u.integer) return -1;
	if(a->u.integer > b->u.integer) return  1;
	return 0;
	
      case T_FLOAT:
	if(a->u.float_number < b->u.float_number) return -1;
	if(a->u.float_number > b->u.float_number) return  1;
	return 0;
	
      case T_STRING:
	return DO_NOT_WARN((int)my_quick_strcmp(a->u.string, b->u.string));
	
      case T_ARRAY:
	if(a==b) return 0;
	if (!a->u.array->size)
	  if (!b->u.array->size) /* There are several different empty arrays. */
	    return 0;
	  else
	    return -1;
	else
	  if (!b->u.array->size)
	    return 1;
	return alpha_svalue_cmpfun(ITEM(a->u.array), ITEM(b->u.array));

      case T_MULTISET:
	if (a == b) return 0;
	{
	  ptrdiff_t a_pos = multiset_first (a->u.multiset);
	  ptrdiff_t b_pos = multiset_first (b->u.multiset);
	  struct svalue ind_a, ind_b;
	  if (a_pos < 0)
	    if (b_pos < 0)
	      return 0;
	    else
	      return -1;
	  else
	    if (b_pos < 0)
	      return 1;
	  res = alpha_svalue_cmpfun (
	    use_multiset_index (a->u.multiset, a_pos, ind_a),
	    use_multiset_index (b->u.multiset, b_pos, ind_b));
	  sub_msnode_ref (a->u.multiset);
	  sub_msnode_ref (b->u.multiset);
	  return res;
	}

      case T_OBJECT:
	if((a->u.object == b->u.object) && (a->subtype == b->subtype)) {
	  return 0;
	}
	break;
	
      default:
#if 1
	/* I think it would be better to leave the order undefined in
	 * these cases since the addresses aren't observable
	 * properties in pike. /mast */
	if(a->u.refs < b->u.refs) return -1;
	if(a->u.refs > b->u.refs) return 1;
#endif
	return 0;
    }
  }

  res = lfun_cmp (a, b);
  if (res != 2) return res;

  return typediff;
}

#define CMP(X,Y) alpha_svalue_cmpfun(X,Y)
#define TYPE struct svalue
#define ID low_sort_svalues
#include "fsort_template.h"
#undef CMP
#undef TYPE
#undef ID

/** This sort is unstable. */
PMOD_EXPORT void sort_array_destructively(struct array *v)
{
  if(!v->size) return;
  low_sort_svalues(ITEM(v), ITEM(v)+v->size-1);
}

#define SORT_BY_INDEX
#define EXTRA_LOCALS int cmpfun_res;
#define CMP(X,Y) ((cmpfun_res = alpha_svalue_cmpfun(svals + X, svals + Y)) ? \
		  cmpfun_res : pos[X] - pos[Y])
#define SWAP(X,Y) {							\
  {struct svalue tmp = svals[X]; svals[X] = svals[Y]; svals[Y] = tmp;}	\
  {int tmp = pos[X]; pos[X] = pos[Y]; pos[Y] = tmp;}			\
}
#define TYPE struct svalue
#define ID low_stable_sort_svalues
#define EXTRA_ARGS , struct svalue *svals, INT32 *pos, int size
#define XARGS , svals, pos, size
#include "fsort_template.h"
#undef SORT_BY_INDEX
#undef EXTRA_LOCALS
#undef CMP
#undef SWAP
#undef TYPE
#undef ID
#undef EXTRA_ARGS
#undef XARGS

/** This sort is stable. The return value is like the one from
 * get_alpha_order. */
PMOD_EXPORT INT32 *stable_sort_array_destructively(struct array *v)
{
  INT32 *current_order;
  ONERROR tmp;
  int e;

  if(!v->size) return NULL;

  /* Overlow safe: ((1<<29)-4)*4 < ULONG_MAX */
  current_order=(INT32 *)xalloc(v->size * sizeof(INT32));
  SET_ONERROR(tmp, free, current_order);
  for(e=0; e<v->size; e++) current_order[e]=e;

  low_stable_sort_svalues (0, v->size - 1, ITEM (v), current_order, v->size);

  UNSET_ONERROR (tmp);
  return current_order;
}


/**
 * Return an 'order' suitable for making mappings and multisets.
 */
PMOD_EXPORT INT32 *get_set_order(struct array *a)
{
  return get_order(a, set_svalue_cmpfun);
}

/**
 * Return an 'order' suitable for switches.
 *
 * Note: This is used by encode_value_canonic(). It must keep the
 * sorting rules for all the types that function allows in multiset
 * and mapping indices.
 */
PMOD_EXPORT INT32 *get_switch_order(struct array *a)
{
  return get_order(a, switch_svalue_cmpfun);
}


/**
 * Return an 'order' suitable for sorting.
 */
PMOD_EXPORT INT32 *get_alpha_order(struct array *a)
{
  return get_order(a, alpha_svalue_cmpfun);
}


static INT32 low_lookup(struct array *v,
			struct svalue *s,
			cmpfun fun)
{
  INT32 a,b,c;
  int q;

  a=0;
  b=v->size;
  while(b > a)
  {
    c=(a+b)/2;
    q=fun(ITEM(v)+c,s);
    
    if(q < 0)
      a=c+1;
    else if(q > 0)
      b=c;
    else
      return c;
  }
  if(a<v->size && fun(ITEM(v)+a,s)<0) a++;
  return ~a;
}

INT32 set_lookup(struct array *a, struct svalue *s)
{
#ifdef PIKE_DEBUG
  if(d_flag > 1)  array_check_type_field(a);
#endif

  /* objects may have `< `> operators, evil stuff! */
  if(s->type != T_OBJECT && !(a->type_field & BIT_OBJECT))
  {
    /* face it, it's not there */
    if( (((2 << s->type) -1) & a->type_field) == 0)
      return -1;
    
  /* face it, it's not there */
    if( ((BIT_MIXED << s->type) & BIT_MIXED & a->type_field) == 0)
      return ~a->size;
  }

  return low_lookup(a,s,set_svalue_cmpfun);
}

INT32 switch_lookup(struct array *a, struct svalue *s)
{
  /* face it, it's not there */
#ifdef PIKE_DEBUG
  if(d_flag > 1)  array_check_type_field(a);
#endif
  /* objects may have `< `> operators, evil stuff! */
  if(s->type != T_OBJECT && !(a->type_field & BIT_OBJECT))
  {
    if( (((2 << s->type) -1) & a->type_field) == 0)
      return -1;

    /* face it, it's not there */
    if( ((BIT_MIXED << s->type) & BIT_MIXED & a->type_field) == 0)
      return ~a->size;
  }

  return low_lookup(a,s,switch_svalue_cmpfun);
}


/**
 * Reorganize an array in the order specified by 'order'.
 */
PMOD_EXPORT struct array *order_array(struct array *v, INT32 *order)
{
  reorder((char *)ITEM(v),v->size,sizeof(struct svalue),order);
  return v;
}


/**
 * Copy and reorganize an array.
 */
PMOD_EXPORT struct array *reorder_and_copy_array(struct array *v, INT32 *order)
{
  INT32 e;
  struct array *ret;
  ret=allocate_array_no_init(v->size, 0);
  ret->type_field = v->type_field;

  for(e=0;e<v->size;e++)
    assign_svalue_no_free(ITEM(ret)+e, ITEM(v)+order[e]);

  return ret;
}

/* Maybe I should have a 'clean' flag for this computation */
PMOD_EXPORT TYPE_FIELD array_fix_type_field(struct array *v)
{
  int e;
  TYPE_FIELD t;

  if(v->flags & ARRAY_LVALUE)
  {
    v->type_field=BIT_MIXED|BIT_UNFINISHED;
    return BIT_MIXED|BIT_UNFINISHED;
  }

  t=0;

  for(e=0; e<v->size; e++) {
    check_svalue (ITEM(v) + e);
    t |= 1 << ITEM(v)[e].type;
  }

#ifdef PIKE_DEBUG
  if(t & ~(v->type_field))
  {
    describe(v);
    Pike_fatal("Type field out of order (old:0x%04x new:0x%04x)!\n",
	       v->type_field, t);
  }
#endif
  v->type_field = t;
  return t;
}

#ifdef PIKE_DEBUG
/* Maybe I should have a 'clean' flag for this computation */
void array_check_type_field(struct array *v)
{
  int e;
  TYPE_FIELD t;

  t=0;

  if(v->flags & ARRAY_LVALUE)
    return;

#ifdef TRACE_UNFINISHED_TYPE_FIELDS
  if (v->type_field & BIT_UNFINISHED && !accept_unfinished_type_fields) {
    fputs ("Array got an unfinished type field.\n", stderr);
    describe_something (v, T_ARRAY, 2, 2, 0, NULL);
  }
#endif

  for(e=0; e<v->size; e++)
  {
    if(ITEM(v)[e].type > MAX_TYPE)
      Pike_fatal("Type is out of range.\n");
      
    t |= 1 << ITEM(v)[e].type;
  }

  if(t & ~(v->type_field))
  {
    describe(v);
    Pike_fatal("Type field out of order!\n");
  }
}
#endif /* PIKE_DEBUG */

/**
 * Get a pointer to the 'union anything' specified if it is of the specified
 * type. The 'union anything' may be changed, but not the type.
 */
PMOD_EXPORT union anything *low_array_get_item_ptr(struct array *a,
						   INT32 ind,
						   TYPE_T t)
{
  if(ITEM(a)[ind].type == t) return & (ITEM(a)[ind].u);
  return 0;
}

/**
 * Get a pointer to the 'union anything' specified if it is of the specified
 * type. The 'union anything' may be changed, but not the type.
 * The differance between this routine and the one above is that this takes
 * the index as an svalue.
 */
PMOD_EXPORT union anything *array_get_item_ptr(struct array *a,
					       struct svalue *ind,
					       TYPE_T t)
{
  INT_TYPE i, p;
  if(ind->type != T_INT)
    Pike_error("Expected integer as array index, got %s.\n",
	       get_name_of_type (ind->type));
  p = ind->u.integer;
  i = p < 0 ? p + a->size : p;
  if(i<0 || i>=a->size) {
    if (a->size) {
      Pike_error("Index %"PRINTPIKEINT"d is out of array range "
		 "%d..%d.\n", p, -a->size, a->size-1);
    } else {
      Pike_error("Attempt to index the empty array with %"PRINTPIKEINT"d.\n", p);
    }
  }
  return low_array_get_item_ptr(a,i,t);
}

/**
 * Organize an array of INT32 to specify how to zip two arrays together
 * to maintain the order.
 * The first item in this array is the size of the result
 * the rest is n >= 0 for a[ n ]
 * or n < 0 for b[ ~n ].
 */
INT32 * merge(struct array *a,struct array *b,INT32 opcode)
{
  ONERROR r;
  INT32 ap,bp,i,*ret,*ptr;
  
  ap=bp=0;
#ifdef PIKE_DEBUG
  if(d_flag > 1)
  {
    array_check_type_field(a);
    array_check_type_field(b);
  }
#endif
  if(!(a->type_field & b->type_field) &&
     !((a->type_field | b->type_field) & BIT_OBJECT))
  {
    /* do smart optimizations */
    switch(opcode)
    {
    case PIKE_ARRAY_OP_AND:
      /* Trivially overflow safe */
      ret=(INT32 *)xalloc(sizeof(INT32));
      *ret=0;
      return ret;

    case PIKE_ARRAY_OP_SUB:
      /* Overlow safe: ((1<<29)-4+1)*4 < ULONG_MAX */
      ptr=ret=(INT32 *)xalloc(sizeof(INT32)*(a->size+1));
      *(ptr++)=a->size;
      for(i=0;i<a->size;i++) *(ptr++)=i;
      return ret;
    }
  }

  /* Note: The following is integer overflow safe as long as
   *       sizeof(struct svalue) >= 2*sizeof(INT32).
   */
  ptr=ret=(INT32 *)xalloc(sizeof(INT32)*(a->size + b->size + 1));
  SET_ONERROR(r, free,ret);
  ptr++;

  while(ap < a->size && bp < b->size)
  {
    i=set_svalue_cmpfun(ITEM(a)+ap,ITEM(b)+bp);
    if(i < 0)
      i=opcode >> 8;
    else if(i > 0)
      i=opcode;
    else
      i=opcode >> 4;
    
    if(i & PIKE_ARRAY_OP_A) *(ptr++)=ap;
    if(i & PIKE_ARRAY_OP_B) *(ptr++)=~bp;
    if(i & PIKE_ARRAY_OP_SKIP_A) ap++;
    if(i & PIKE_ARRAY_OP_SKIP_B) bp++;
  }

  if((opcode >> 8) & PIKE_ARRAY_OP_A) while(ap<a->size) *(ptr++)=ap++;
  if(opcode & PIKE_ARRAY_OP_B) while(bp<b->size) *(ptr++)=~(bp++);

  *ret = DO_NOT_WARN((INT32)(ptr-ret-1));

  UNSET_ONERROR(r);

  return ret;
}

/**
 * This routine merges two arrays in the order specified by 'zipper'
 * zipper normally produced by merge() above.
 */
PMOD_EXPORT struct array *array_zip(struct array *a, struct array *b,INT32 *zipper)
{
  INT32 size,e;
  struct array *ret;
  size=zipper[0];
  zipper++;

  ret=allocate_array_no_init(size,0);
  for(e=0; e<size; e++)
  {
    if(*zipper >= 0)
      assign_svalue_no_free(ITEM(ret)+e, ITEM(a)+*zipper);
    else
      assign_svalue_no_free(ITEM(ret)+e, ITEM(b)+~*zipper);
    zipper++;
  }
  ret->type_field = a->type_field | b->type_field;
  return ret;
}

/** add an arbitrary number of arrays together
* @param argp an array of svalues containing the arrays to be concatenated
* @param args the number of elements in argp
* @returns the resulting struct array.
*/
PMOD_EXPORT struct array *add_arrays(struct svalue *argp, INT32 args)
{
  INT32 e, size;
  struct array *v;

  for(size=e=0;e<args;e++)
    size+=argp[e].u.array->size;

#if 1
  {
    INT32 tmp=0;
    for(e=0;e<args;e++)
    {
      v=argp[e].u.array;
      if(v->refs == 1 &&
	 (v->item - v->real_item) >= tmp &&
	 v->malloced_size >= size - tmp)
      {
	debug_malloc_touch(v);
	argp[e].type=T_INT;
	for(tmp=e-1;tmp>=0;tmp--)
	{
	  debug_malloc_touch(argp[tmp].u.array);
	  v->type_field|=argp[tmp].u.array->type_field;
	  assign_svalues_no_free(ITEM(v) - argp[tmp].u.array->size,
				 ITEM(argp[tmp].u.array),
				 argp[tmp].u.array->size,
				 argp[tmp].u.array->type_field);
	  v->item-=argp[tmp].u.array->size;
	  v->malloced_size+=argp[tmp].u.array->size;
	}

	for(tmp=e+1;tmp<args;tmp++)
	{
	  debug_malloc_touch(argp[tmp].u.array);
	  v->type_field|=argp[tmp].u.array->type_field;
	  assign_svalues_no_free(ITEM(v) + v->size,
				 ITEM(argp[tmp].u.array),
				 argp[tmp].u.array->size,
				 argp[tmp].u.array->type_field);
	  v->size+=argp[tmp].u.array->size;
	}
#ifdef PIKE_DEBUG
	if(d_flag>1)
	  check_array(v);
#endif
	return v;
      }
      tmp+=v->size;
    }
  }
#endif

  if(args && argp[0].u.array->refs==1)
  {
    e=argp[0].u.array->size;
    v=resize_array(argp[0].u.array, size);
    argp[0].type=T_INT;
    size=e;
    e=1;
  }else{
    v=allocate_array_no_init(size, 0);
    v->type_field=0;
    e=size=0;
  }

  for(; e<args; e++)
  {
    v->type_field|=argp[e].u.array->type_field;
    assign_svalues_no_free(ITEM(v)+size,
			   ITEM(argp[e].u.array),
			   argp[e].u.array->size,
			   argp[e].u.array->type_field);
    size+=argp[e].u.array->size;
  }

  return v;
}

PMOD_EXPORT int array_equal_p(struct array *a, struct array *b, struct processing *p)
{
  struct processing curr;
  INT32 e;

  if(a == b) return 1;
  if(a->size != b->size) return 0;
  if(!a->size) return 1;

#ifdef PIKE_DEBUG
  if(d_flag > 1)
  {
    array_check_type_field(a);
    array_check_type_field(b);
  }
#endif

  /* This could be done much better if I KNEW that
   * the type fields didn't contain types that
   * really aren't in the array
   */
  if(!(a->type_field & b->type_field) &&
     !( (a->type_field | b->type_field) & BIT_OBJECT ))
    return 0;

  curr.pointer_a = a;
  curr.pointer_b = b;
  curr.next = p;

  for( ;p ;p=p->next)
    if(p->pointer_a == (void *)a && p->pointer_b == (void *)b)
      return 1;

  for(e=0; e<a->size; e++)
    if(!low_is_equal(ITEM(a)+e, ITEM(b)+e, &curr))
      return 0;

  return 1;
}

typedef int(*mycmpfun)(INT32*,INT32*,INT32*,INT32*);
#define ID fsort_with_order
#define CMP(X,Y) ((*cmpfun)((X),(Y),oa,ob))
#define EXTRA_ARGS ,mycmpfun cmpfun,INT32 *oa,INT32 *ob
#define XARGS ,cmpfun,oa,ob
#define TYPE INT32
#include "fsort_template.h"
#undef ID
#undef TYPE
#undef XARGS
#undef EXTRA_ARGS
#undef CMP



/**
 * This is used to rearrange the zipper so that the order is retained
 * as it was before (check merge_array_with_order below).
 */
static int array_merge_fun(INT32 *a, INT32 *b,
			   INT32 *ordera, INT32 *orderb)
{
  if(*a<0)
  {
    if(*b<0)
    {
      return orderb[~*a] - orderb[~*b];
    }else{
      return 1;
    }
  }else{
    if(*b<0)
    {
      return -1;
    }else{
      return ordera[*a] - ordera[*b];
    }
  }
}

/**
 * Merge two arrays and retain their order. This is done by arranging them
 * into ordered sets, merging them as sets and then rearranging the zipper
 * before zipping the sets together.
 */
PMOD_EXPORT struct array *merge_array_with_order(struct array *a,
						 struct array *b, INT32 op)
{
  ONERROR r1,r2,r3,r4,r5;
  INT32 *zipper;
  struct array *tmpa,*tmpb,*ret;
  INT32 *ordera, *orderb;

  ordera=get_set_order(a);
  SET_ONERROR(r4,free,ordera);

  orderb=get_set_order(b);
  SET_ONERROR(r5,free,orderb);

  tmpa=reorder_and_copy_array(a,ordera);
  SET_ONERROR(r1,do_free_array,tmpa);

  tmpb=reorder_and_copy_array(b,orderb);
  SET_ONERROR(r2,do_free_array,tmpb);

  zipper=merge(tmpa,tmpb,op);
  SET_ONERROR(r3,free,zipper);

#if 0
  {
    int i;

    simple_describe_array (a);
    simple_describe_array (b);

    fprintf (stderr, "order a: ");
    for (i = 0; i < a->size; i++)
      fprintf (stderr, "%d ", ordera[i]);
    fprintf (stderr, "\n");

    fprintf (stderr, "order b: ");
    for (i = 0; i < b->size; i++)
      fprintf (stderr, "%d ", orderb[i]);
    fprintf (stderr, "\n");

    simple_describe_array (tmpa);
    simple_describe_array (tmpb);

    fprintf (stderr, "zipper: ");
    for (i = 1; i < *zipper + 1; i++)
      fprintf (stderr, "%d ", zipper[i]);
    fprintf (stderr, "\n");
  }
#endif

  fsort_with_order( (zipper+1), zipper+*zipper, array_merge_fun,
		    ordera, orderb );

  ret=array_zip(tmpa,tmpb,zipper);
  UNSET_ONERROR(r3);  free((char *)zipper);
  UNSET_ONERROR(r2);  free_array(tmpb);
  UNSET_ONERROR(r1);  free_array(tmpa);
  UNSET_ONERROR(r5);  free((char *)orderb);
  UNSET_ONERROR(r4);  free((char *)ordera);
  return ret;
}


#define CMP(X,Y) set_svalue_cmpfun(X,Y)
#define TYPE struct svalue
#define ID set_sort_svalues
#include "fsort_template.h"
#undef CMP
#undef TYPE
#undef ID


PMOD_EXPORT struct array *merge_array_without_order2(struct array *a, struct array *b,INT32 op)
{
  ONERROR r1,r2,r3,r4,r5;
  INT32 ap,bp,i;
  struct svalue *arra,*arrb;
  struct array *ret;

#ifdef PIKE_DEBUG
  if(d_flag > 1)
  {
    array_check_type_field(a);
    array_check_type_field(b);
  }
#endif

  SET_ONERROR(r1,do_free_array,a);
  SET_ONERROR(r2,do_free_array,b);

  if(a->refs==1 || !a->size)
  {
    arra=ITEM(a);
  }else{
    /* Overlow safe: ((1<<29)-4)*8 < ULONG_MAX */
    arra=(struct svalue *)xalloc(a->size*sizeof(struct svalue));
    MEMCPY(arra,ITEM(a),a->size*sizeof(struct svalue));
    SET_ONERROR(r3,free,arra);
  }

  if(b->refs==1 || !b->size)
  {
    arrb=ITEM(b);
  }else{
    /* Overlow safe: ((1<<29)-4)*8 < ULONG_MAX */
    arrb=(struct svalue *)xalloc(b->size*sizeof(struct svalue));
    MEMCPY(arrb,ITEM(b),b->size*sizeof(struct svalue));
    SET_ONERROR(r4,free,arrb);
  }

  set_sort_svalues(arra,arra+a->size-1);
  set_sort_svalues(arrb,arrb+b->size-1);

  ret=low_allocate_array(0,32);
  SET_ONERROR(r5,do_free_array,ret);
  ap=bp=0;

  while(ap < a->size && bp < b->size)
  {
    i=set_svalue_cmpfun(arra+ap,arrb+bp);
    if(i < 0)
      i=op >> 8;
    else if(i > 0)
      i=op;
    else
      i=op >> 4;
    
    if(i & PIKE_ARRAY_OP_A) ret=append_array(ret,arra+ap);
    if(i & PIKE_ARRAY_OP_B) ret=append_array(ret,arrb+bp);
    if(i & PIKE_ARRAY_OP_SKIP_A) ap++;
    if(i & PIKE_ARRAY_OP_SKIP_B) bp++;
  }

  if((op >> 8) & PIKE_ARRAY_OP_A)
    while(ap<a->size)
      ret=append_array(ret,arra + ap++);

  if(op & PIKE_ARRAY_OP_B)
    while(bp<b->size)
      ret=append_array(ret,arrb + bp++);

  UNSET_ONERROR(r5);

  if(arrb != ITEM(b))
  {
    UNSET_ONERROR(r4);
    free((char *)arrb);
  }

  if(arra != ITEM(a))
  {
    UNSET_ONERROR(r3);
    free((char *)arra);
  }

  UNSET_ONERROR(r2);
  free_array(b);

  UNSET_ONERROR(r1);
  free_array(a);

  return ret;
}


/** merge two arrays without paying attention to the order
 * the elements has presently
 */
PMOD_EXPORT struct array *merge_array_without_order(struct array *a,
					struct array *b,
					INT32 op)
{
#if 0
  /* FIXME: If this routine is ever reinstated, it has to be
   * fixed to use ONERROR
   */
  INT32 *zipper;
  struct array *tmpa,*tmpb,*ret;

  if(ordera) { free((char *)ordera); ordera=0; }
  if(orderb) { free((char *)orderb); orderb=0; }

  ordera=get_set_order(a);
  tmpa=reorder_and_copy_array(a,ordera);
  free((char *)ordera);
  ordera=0;

  orderb=get_set_order(b);
  tmpb=reorder_and_copy_array(b,orderb);
  free((char *)orderb);
  orderb=0;

  zipper=merge(tmpa,tmpb,op);
  ret=array_zip(tmpa,tmpb,zipper);
  free_array(tmpa);
  free_array(tmpb);
  free((char *)zipper);
  return ret;

#else
  add_ref(a);
  add_ref(b);
  return merge_array_without_order2(a,b,op);
#endif
}

/** Subtract an array from another.
*/
PMOD_EXPORT struct array *subtract_arrays(struct array *a, struct array *b)
{
#ifdef PIKE_DEBUG
  if(d_flag > 1)
  {
    array_check_type_field(b);
  }
#endif
  check_array_for_destruct(a);

  if((a->type_field & b->type_field) ||
     ((a->type_field | b->type_field) & BIT_OBJECT))
  {
    return merge_array_with_order(a, b, PIKE_ARRAY_OP_SUB);
  }else{
    if(a->refs == 1)
    {
      add_ref(a);
      return a;
    }
    return slice_array(a,0,a->size);
  }
}

/** And two arrays together.
 */
PMOD_EXPORT struct array *and_arrays(struct array *a, struct array *b)
{
#ifdef PIKE_DEBUG
  if(d_flag > 1)
    array_check_type_field(b);
#endif
  check_array_for_destruct(a);

  if((a->type_field & b->type_field) ||
     ((a->type_field | b->type_field) & BIT_OBJECT))
    return merge_array_with_order(a, b, PIKE_ARRAY_OP_AND_LEFT);
  else
    return allocate_array_no_init(0,0);
}

int array_is_constant(struct array *a,
		      struct processing *p)
{
  return svalues_are_constant(ITEM(a),
			      a->size,
			      array_fix_type_field(a),
			      p);
}

node *make_node_from_array(struct array *a)
{
  struct svalue s;
  INT32 e;

  if(!a->size)
      return mkefuncallnode("aggregate",0);
    
  if(array_fix_type_field(a) == BIT_INT)
  {
    debug_malloc_touch(a);
    for(e=0; e<a->size; e++)
      if(ITEM(a)[e].u.integer != 0)
	break;
    if(e == a->size)
    {
      return mkefuncallnode("allocate",mkintnode(a->size));
    }
  }
  debug_malloc_touch(a);
  if(!is_more_than_one_bit(a->type_field))
  {
    e=0;
    debug_malloc_touch(a);
    switch(a->type_field)
    {
      case BIT_INT:
	for(e=1; e<a->size; e++)
	  if(ITEM(a)[e].u.integer != ITEM(a)[0].u.integer)
	    break;
	if(e==a->size && ITEM(a)[0].u.integer==0)
	  return mkefuncallnode("allocate",mkintnode(a->size));
	break;
	
      case BIT_STRING:
      case BIT_PROGRAM:
	for(e=1; e<a->size; e++)
	  if(ITEM(a)[e].u.refs != ITEM(a)[0].u.refs)
	    break;
	break;
	
      case BIT_OBJECT:
      case BIT_FUNCTION:
	for(e=1; e<a->size; e++)
	  if(ITEM(a)[e].u.object != ITEM(a)[0].u.object ||
	     ITEM(a)[e].subtype != ITEM(a)[0].subtype)
	    break;
	break;
    }
    debug_malloc_touch(a);
    if(e == a->size)
      return mkefuncallnode("allocate",mknode(F_ARG_LIST,
					      mkintnode(a->size),
					      mksvaluenode(ITEM(a))));
  }
  
  if(array_is_constant(a,0))
  {
    debug_malloc_touch(a);
    s.type=T_ARRAY;
    s.subtype=0;
    s.u.array=a;
    return mkconstantsvaluenode(&s);
  }else{
    node *ret=0;
    debug_malloc_touch(a);
    for(e=0; e<a->size; e++)
      ret=mknode(F_ARG_LIST,ret,mksvaluenode(ITEM(a)+e));
    return mkefuncallnode("aggregate",ret);
  }
}

/** Push elements of an array onto the stack. The array will be freed.
 */
PMOD_EXPORT void push_array_items(struct array *a)
{
  check_stack(a->size);
  check_array_for_destruct(a);
  if(a->refs == 1)
  {
    MEMCPY(Pike_sp,ITEM(a),sizeof(struct svalue)*a->size);
    Pike_sp += a->size;
    a->size=0;
    free_array(a);
  }else{
    assign_svalues_no_free(Pike_sp, ITEM(a), a->size, a->type_field);
    Pike_sp += a->size;
    free_array(a);
  }
}

void describe_array_low(struct array *a, struct processing *p, int indent)
{
  INT32 e,d;
  indent += 2;

  for(e=0; e<a->size; e++)
  {
    if(e) my_strcat(",\n");
    for(d=0; d<indent; d++) my_putchar(' ');
    describe_svalue(ITEM(a)+e,indent,p);
  }
}


#ifdef PIKE_DEBUG
void simple_describe_array(struct array *a)
{
  dynamic_buffer save_buf;
  char *s;
  if (a->size) {
    init_buf(&save_buf);
    describe_array_low(a,0,0);
    s=simple_free_buf(&save_buf);
    fprintf(stderr,"({\n%s\n})\n",s);
    free(s);
  }
  else
    fputs ("({ })\n", stderr);
}

void describe_index(struct array *a,
		    int e,
		    struct processing *p,
		    int indent)
{
  describe_svalue(ITEM(a)+e, indent, p);
}
#endif


void describe_array(struct array *a,struct processing *p,int indent)
{
  struct processing doing;
  INT32 e;
  char buf[60];
  if(! a->size)
  {
    my_strcat("({ })");
    return;
  }

  doing.next=p;
  doing.pointer_a=(void *)a;
  for(e=0;p;e++,p=p->next)
  {
    if(p->pointer_a == (void *)a)
    {
      sprintf(buf,"@%ld",(long)e);
      my_strcat(buf);
      return;
    }
  }

  if (a->size == 1) {
    sprintf(buf, "({ /* 1 element */\n");
  } else {
    sprintf(buf, "({ /* %ld elements */\n", (long)a->size);
  }
  my_strcat(buf);
  describe_array_low(a,&doing,indent);
  my_putchar('\n');
  for(e=2; e<indent; e++) my_putchar(' ');
  my_strcat("})");
}

/**
 * Pops a number of arguments off of the stack an puts them in an array.
 * The 'top' of the stack will be the last element in the array.
 * @param args The number of arguments to aggregate.
 */
PMOD_EXPORT struct array *aggregate_array(INT32 args)
{
  struct array *a;

  a=allocate_array_no_init(args,0);
  if (args) {
    MEMCPY((char *)ITEM(a),(char *)(Pike_sp-args),args*sizeof(struct svalue));
    array_fix_type_field (a);
    Pike_sp-=args;
    DO_IF_DMALLOC(while(args--) dmalloc_touch_svalue(Pike_sp + args));
  }
  return a;
}

/** Add an element to the end of an array by resizing the array.
 *
 * @param a the array to be appended
 * @param s the value to be added to the new element in the array
 */
PMOD_EXPORT struct array *append_array(struct array *a, struct svalue *s)
{
  a=resize_array(a,a->size+1);
  array_set_index(a, a->size-1, s);
  return a;
}

typedef char *(* explode_searchfunc)(void *,void *,size_t);

/** Explode a string into an array by a delimiter.
 * 
 * @param str the string to be split
 * @param del the string to split str by
 * @returns an array containing the elements of the split string
 */
PMOD_EXPORT struct array *explode(struct pike_string *str,
		       struct pike_string *del)
{
  INT32 e;
  struct array *ret;
  char *s, *end, *tmp;

#if 0
  if(!str->len)
  {
    return allocate_array_no_init(0,0);
  }
#endif
  if(!del->len)
  {
    ret=allocate_array_no_init(str->len,0);
    for(e=0;e<str->len;e++)
    {
      ITEM(ret)[e].type=T_STRING;
      ITEM(ret)[e].u.string=string_slice(str,e,1);
    }
  }else{
    SearchMojt mojt;
    explode_searchfunc f = (explode_searchfunc)0;
    
    s=str->str;
    end=s+(str->len << str->size_shift);

    ret=allocate_array(10);
    ret->size=0;

    mojt=compile_memsearcher(MKPCHARP_STR(del),
			     del->len,
			     str->len,
			     del);

    switch(str->size_shift)
    {
      case 0: f=(explode_searchfunc)mojt.vtab->func0; break;
      case 1: f=(explode_searchfunc)mojt.vtab->func1; break;
      case 2: f=(explode_searchfunc)mojt.vtab->func2; break;
#ifdef PIKE_DEBUG
      default: Pike_fatal("Illegal shift.\n");
#endif
    }

    while((tmp = f(mojt.data, s, (end-s)>> str->size_shift)))
    {
      if(ret->size == ret->malloced_size)
      {
	e=ret->size;
	ACCEPT_UNFINISHED_TYPE_FIELDS {
	  ret=resize_array(ret, e * 2);
	} END_ACCEPT_UNFINISHED_TYPE_FIELDS;
	ret->size=e;
      }

      ITEM(ret)[ret->size].u.string=string_slice(str,
						 (s-str->str)>>str->size_shift,
						 (tmp-s)>>str->size_shift);
      ITEM(ret)[ret->size].type=T_STRING;
      ret->size++;

      s=tmp+(del->len << str->size_shift);
    }

    if(ret->size == ret->malloced_size)
    {
      e=ret->size;
      ACCEPT_UNFINISHED_TYPE_FIELDS {
	ret=resize_array(ret, e * 2);
      } END_ACCEPT_UNFINISHED_TYPE_FIELDS;
      ret->size=e;
    }

    ITEM(ret)[ret->size].u.string=string_slice(str,
					       (s-str->str)>>str->size_shift,
					       (end-s)>>str->size_shift);

    ITEM(ret)[ret->size].type=T_STRING;
    ret->size++;
    mojt.vtab->freeme(mojt.data);
  }
  ret->type_field=BIT_STRING;
  return ret;
}

/** Implode an array by creating a string with all of the array's
 *  elements separated by a delimiter.
 *
 * @param a The array containing elements to be imploded
 * @param del The delimiter used to separate the array's elements in the resulting string
 * @return The imploded string
 * 
 */
PMOD_EXPORT struct pike_string *implode(struct array *a,
                                        struct pike_string *del)
{
  INT32 len, e, inited;
  PCHARP r;
  struct pike_string *ret, *tmp;
  int max_shift = del->size_shift;

  len=0;

  for(e=0;e<a->size;e++)
  {
    if(ITEM(a)[e].type==T_STRING)
    {
      len+=ITEM(a)[e].u.string->len + del->len;
      if(ITEM(a)[e].u.string->size_shift > max_shift)
	max_shift=ITEM(a)[e].u.string->size_shift;
    }
  }
  if(len) len-=del->len;
  
  ret=begin_wide_shared_string(len,max_shift);
  r=MKPCHARP_STR(ret);
  inited=0;
  for(e=0;e<a->size;e++)
  {
    if(ITEM(a)[e].type==T_STRING)
    {
      if(inited)
      {
	pike_string_cpy(r,del);
	INC_PCHARP(r,del->len);
      }
      inited=1;
      tmp=ITEM(a)[e].u.string;
      pike_string_cpy(r,tmp);
      INC_PCHARP(r,tmp->len);
      len++;
    }
  }
  return low_end_shared_string(ret);
}

/** Deeply copy an array. The mapping is used for temporary storage.
 */
PMOD_EXPORT struct array *copy_array_recursively(struct array *a,
						 struct mapping *m)
{
  struct array *ret;
  struct svalue aa, bb;

#ifdef PIKE_DEBUG
  if(d_flag > 1)  array_check_type_field(a);
#endif

  if (!a->size) {
    add_ref(&empty_array);
    return array_set_flags(&empty_array, a->flags);
  }

  ret=allocate_array_no_init(a->size,0);

  aa.type = T_ARRAY;
  aa.subtype = 0;
  aa.u.array = a;
  bb.type = T_ARRAY;
  bb.subtype = 0;
  bb.u.array = ret;
  low_mapping_insert(m, &aa, &bb, 1);

  ret->flags = a->flags & ~ARRAY_LVALUE;

  copy_svalues_recursively_no_free(ITEM(ret),ITEM(a),a->size,m);

  ret->type_field=a->type_field;
  return ret;
}

/** Apply the elements of an array. Arguments the array should be
 *  applied with should be on the stack before the call and the
 *  resulting array will be on the stack after the call.
 */
PMOD_EXPORT void apply_array(struct array *a, INT32 args)
{
  INT32 e, hash = 0;
  struct svalue *argp = Pike_sp-args;
  struct array *aa;
  struct array *cycl;
  TYPE_FIELD new_types = 0;
  DECLARE_CYCLIC();

  check_stack(args);
  check_array_for_destruct(a);
  for (e=0; e<args; e++)
    hash = hash * 33 + (INT32)Pike_sp[-e-1].u.ptr;

  if (!(cycl = (struct array *)BEGIN_CYCLIC(a, (ptrdiff_t)hash))) {
    push_array(aa = allocate_array_no_init(0, a->size));
    for (e=0; (e<a->size) && (e < aa->malloced_size); e++)
    {
      assign_svalues_no_free(Pike_sp, argp, args, BIT_MIXED);
      Pike_sp+=args;
      /* FIXME: Don't throw apply errors from apply_svalue here. */
      apply_svalue(ITEM(a)+e,args);
      new_types |= 1 << Pike_sp[-1].type;
      assign_svalue_no_free(ITEM(aa)+e, &Pike_sp[-1]);
      aa->size = e;
      pop_stack();
    }
    aa->type_field = new_types;
#ifdef PIKE_DEBUG
    array_check_type_field(aa);
#endif
    stack_pop_n_elems_keep_top(args);
  }
  else {
    pop_n_elems(args);
    ref_push_array(cycl);
  }

  END_CYCLIC();
}

/** Reverse the elements in an array. If the array has more than one
 *  reference, the array will be reversed into a new array. Otherwise
 *  the array will be destructively reversed in place.
 */
PMOD_EXPORT struct array *reverse_array(struct array *a)
{
  INT32 e;
  struct array *ret;

  if(a->refs == 1)
    /* Reverse in-place. */
  {
    struct svalue *tmp0, *tmp1, swap;
    
    tmp0 = ITEM(a);
    tmp1 = ITEM(a) + a->size;
    for(e = a->size>>1; 0 < e; e--)
    {
      swap = *tmp0;
      *(tmp0++) = *(--tmp1);
      *tmp1 = swap;
    }

    /* FIXME: What about the flags field? */
    
    add_ref(a);
    return a;
  }
  
  ret=allocate_array_no_init(a->size,0);
  for(e=0;e<a->size;e++)
    assign_svalue_no_free(ITEM(ret)+e,ITEM(a)+a->size+~e);
  ret->type_field = a->type_field;
  return ret;
}

/** Replaces all from elements in array a with to elements. Called
 *  from replaces when first argument is an array. The replace is applied
 *  desctructivly.
 */
void array_replace(struct array *a,
		   struct svalue *from,
		   struct svalue *to)
{
  ptrdiff_t i = -1;

  while((i=array_search(a,from,i+1)) >= 0) array_set_index(a,i,to);
}

#ifdef PIKE_DEBUG
PMOD_EXPORT void check_array(struct array *a)
{
  INT32 e;

  if(a->next && a->next->prev != a)
    Pike_fatal("array->next->prev != array.\n");

  if(a->prev)
  {
    if(a->prev->next != a)
      Pike_fatal("array->prev->next != array.\n");
  }else{
    if(first_array != a)
      Pike_fatal("array->prev == 0 but first_array != array.\n");
  }

  if(a->size > a->malloced_size)
    Pike_fatal("Array is larger than malloced block!\n");

  if(a->size < 0)
    Pike_fatal("Array size is negative!\n");

  if(a->malloced_size < 0)
    Pike_fatal("Array malloced size is negative!\n");

  if(a->item < a->real_item)
  {
#ifdef DEBUG_MALLOC
    describe(a);
#endif
    Pike_fatal("Array item pointer is too small!\n");
  }

  if(a->refs <=0 )
    Pike_fatal("Array has zero refs.\n");


  for(e=0;e<a->size;e++)
  {
    if(! ( (1 << ITEM(a)[e].type) & (a->type_field) ) && ITEM(a)[e].type<16)
      Pike_fatal("Type field lies.\n");
    
    check_svalue(ITEM(a)+e);
  }
}

void check_all_arrays(void)
{
  struct array *a;
  for (a = first_array; a; a = a->next)
    check_array(a);
}
#endif /* PIKE_DEBUG */


static void gc_check_array(struct array *a)
{
  GC_ENTER (a, T_ARRAY) {
    if(a->type_field & BIT_COMPLEX)
    {
      if (a->flags & ARRAY_WEAK_FLAG) {
	gc_check_weak_svalues(ITEM(a), a->size);
	gc_checked_as_weak(a);
      }
      else
	gc_check_svalues(ITEM(a), a->size);
    }
  } GC_LEAVE;
}

void gc_mark_array_as_referenced(struct array *a)
{
  if(gc_mark(a))
    GC_ENTER (a, T_ARRAY) {
      if (a == gc_mark_array_pos)
	gc_mark_array_pos = a->next;
      if (a == gc_internal_array)
	gc_internal_array = a->next;
      else {
	DOUBLEUNLINK (first_array, a);
	DOUBLELINK (first_array, a); /* Linked in first. */
      }

      if (a->type_field & BIT_COMPLEX)
      {
	if (a->flags & ARRAY_WEAK_FLAG) {
	  TYPE_FIELD t;
	  if (!(t = gc_mark_weak_svalues(a->item, a->size)))
	    t = a->type_field;

	  /* Ugly, but we are not allowed to change type_field
	   * at the same time as the array is being built...
	   * Actually we just need better primitives for building arrays.
	   */
	  if(!(a->type_field & BIT_UNFINISHED) || a->refs!=1)
	    a->type_field = t;
	  else
	    a->type_field |= t;	/* There might be an additional BIT_INT. */

	  gc_assert_checked_as_weak(a);
	}
	else {
	  TYPE_FIELD t;
	  if ((t = gc_mark_svalues(ITEM(a), a->size))) {
	    if(!(a->type_field & BIT_UNFINISHED) || a->refs!=1)
	      a->type_field = t;
	    else
	      a->type_field |= t;
	  }
	  gc_assert_checked_as_nonweak(a);
	}
      }
    } GC_LEAVE;
}

void real_gc_cycle_check_array(struct array *a, int weak)
{
  GC_CYCLE_ENTER(a, T_ARRAY, weak) {
#ifdef PIKE_DEBUG
    if (!gc_destruct_everything &&
	(a == &empty_array || a == &weak_empty_array))
      Pike_fatal("Trying to gc cycle check some *_empty_array.\n");
#endif

    if (a->type_field & BIT_COMPLEX)
    {
      TYPE_FIELD t = a->flags & ARRAY_WEAK_FLAG ?
	gc_cycle_check_weak_svalues(ITEM(a), a->size) :
	gc_cycle_check_svalues(ITEM(a), a->size);
      if (t) {
	/* In the weak case we should only get here if references to
	 * destructed objects are removed. */
	if(!(a->type_field & BIT_UNFINISHED) || a->refs!=1)
	  a->type_field = t;
	else
	  a->type_field |= t;
      }
#ifdef PIKE_DEBUG
      if (a->flags & ARRAY_WEAK_FLAG)
	gc_assert_checked_as_weak(a);
      else
	gc_assert_checked_as_nonweak(a);
#endif
    }
  } GC_CYCLE_LEAVE;
}

unsigned gc_touch_all_arrays(void)
{
  unsigned n = 0;
  struct array *a;
  if (!first_array || first_array->prev)
    Pike_fatal ("error in array link list.\n");
  for (a = first_array; a; a = a->next) {
    debug_gc_touch(a);
    n++;
    if (a->next && a->next->prev != a)
      Pike_fatal("Error in array link list.\n");
  }
  return n;
}

void gc_check_all_arrays(void)
{
  struct array *a;
  for (a = first_array; a; a = a->next) {
#ifdef PIKE_DEBUG
    if(d_flag > 1)  array_check_type_field(a);
#endif
    gc_check_array(a);
  }
}


void gc_mark_all_arrays(void)
{
  gc_mark_array_pos = gc_internal_array;
  while (gc_mark_array_pos) {
    struct array *a = gc_mark_array_pos;
    gc_mark_array_pos = a->next;
    if(gc_is_referenced(a))
      gc_mark_array_as_referenced(a);
  }
}

void gc_cycle_check_all_arrays(void)
{
  struct array *a;
  for (a = gc_internal_array; a; a = a->next) {
    real_gc_cycle_check_array(a, 0);
    gc_cycle_run_queue();
  }
}

void gc_zap_ext_weak_refs_in_arrays(void)
{
  gc_mark_array_pos = first_array;
  while (gc_mark_array_pos != gc_internal_array && gc_ext_weak_refs) {
    struct array *a = gc_mark_array_pos;
    gc_mark_array_pos = a->next;
    gc_mark_array_as_referenced(a);
  }
  gc_mark_discard_queue();
}

size_t gc_free_all_unreferenced_arrays(void)
{
  struct array *a,*next;
  size_t unreferenced = 0;

  for (a = gc_internal_array; a; a = next)
  {
#ifdef PIKE_DEBUG
    if (!a)
      Pike_fatal("Null pointer in array list.\n");
#endif
    if(gc_do_free(a))
    {
      /* Got an extra ref from gc_cycle_pop(). */
      free_svalues(ITEM(a), a->size, a->type_field);
      a->size=0;

      gc_free_extra_ref(a);
      SET_NEXT_AND_FREE(a, free_array);
    }
    else
    {
      next=a->next;
    }
    unreferenced++;
  }

  return unreferenced;
}


#ifdef PIKE_DEBUG

void debug_dump_type_field(TYPE_FIELD t)
{
  int e;
  for(e=0;e<=MAX_TYPE;e++)
    if(t & (1<<e))
      fprintf(stderr," %s",get_name_of_type(e));

  for(;e<16;e++)
    if(t & (1<<e))
      fprintf(stderr," <%d>",e);
}

void debug_dump_array(struct array *a)
{
  fprintf(stderr,"Location=%p Refs=%d, next=%p, prev=%p, "
	  "flags=0x%x, size=%d, malloced_size=%d%s\n",
	  a,
	  a->refs,
	  a->next,
	  a->prev,
	  a->flags,
	  a->size,
	  a->malloced_size,
	  a == &empty_array ? " (the empty_array)" :
	  a == &weak_empty_array ? " (the weak_empty_array)" :
	  "");
  fprintf(stderr,"Type field =");
  debug_dump_type_field(a->type_field);
  fprintf(stderr,"\n");
  simple_describe_array(a);
}
#endif


/** Returns (by argument) the number of arrays and the total amount of
 *  memory allocated for arrays (array structs + svalues). Called from
 *  _memory_usage, which is exposed through Debug.memory_usage().
 */
void count_memory_in_arrays(INT32 *num_, INT32 *size_)
{
  INT32 num=0, size=0;
  struct array *m;
  for(m=first_array;m;m=m->next)
  {
    num++;
    size+=sizeof(struct array)+
      sizeof(struct svalue) *  (m->malloced_size - 1);
  }
  *num_=num;
  *size_=size;
}

/** Segments an array into several elements in an array based on the
 *  sequence in the second array argument. This function is called
 *  when an array is divided by another array. Pike level example, ({
 *  "hello", " ", "world", "!" }) / ({ " " }) -> ({ ({ "hello" }), ({
 *  "world", "!" }) })
 */
PMOD_EXPORT struct array *explode_array(struct array *a, struct array *b)
{
  INT32 e,d,start;
  struct array *tmp;

  start=0;
#if 0
  if(!a->size)
  {
    return allocate_array_no_init(0,0);
  }
#endif
  if(b->size)
  {
    BEGIN_AGGREGATE_ARRAY(1) {
      for(e=0;e<=a->size - b->size;e++)
      {
	for(d=0;d<b->size;d++)
	{
	  if(!is_eq(ITEM(a)+(e+d),ITEM(b)+d))
	    break;
	}
	if(d==b->size)
	{
	  check_stack(1);
	  push_array(friendly_slice_array(a, start, e));
	  DO_AGGREGATE_ARRAY(120);
	  e+=b->size-1;
	  start=e+1;
	}
      }
      check_stack(1);
      push_array(friendly_slice_array(a, start, a->size));
    } END_AGGREGATE_ARRAY;
  }else{
    check_stack(120);
    BEGIN_AGGREGATE_ARRAY(a->size) {
      for(e=0;e<a->size;e++) {
	push_array(friendly_slice_array(a, e, e+1));
	DO_AGGREGATE_ARRAY(120);
      }
    } END_AGGREGATE_ARRAY;
  }
  tmp=(--Pike_sp)->u.array;
  debug_malloc_touch(tmp);
  if(tmp->size) tmp->type_field=BIT_ARRAY;
  return tmp;
}

/** Joins array elements of an array into a new array with the
 *  elements of the second array as joiners. Performs the opposite
 *  action from explode_array and is called when an array is
 *  multiplied by another array.
 */
PMOD_EXPORT struct array *implode_array(struct array *a, struct array *b)
{
  INT32 e, size;
  struct array *ret;

  if (!a->size) {
    add_ref(a);
    return a;
  }

  size=0;
  for(e=0;e<a->size;e++)
  {
    if(ITEM(a)[e].type!=T_ARRAY)
      Pike_error("Implode array contains non-arrays.\n");
    size+=ITEM(a)[e].u.array->size;
  }

  ret=allocate_array((a->size -1) * b->size + size);
  size=0;
  ret->type_field=0;
  for(e=0;e<a->size;e++)
  {
    if(e)
    {
      ret->type_field|=b->type_field;
      assign_svalues_no_free(ITEM(ret)+size,
			     ITEM(b),
			     b->size,
			     b->type_field);
      size+=b->size;
    }
    ret->type_field|=ITEM(a)[e].u.array->type_field;
    assign_svalues_no_free(ITEM(ret)+size,
			   ITEM(ITEM(a)[e].u.array),
			   ITEM(a)[e].u.array->size,
			   ITEM(a)[e].u.array->type_field);
    size+=ITEM(a)[e].u.array->size;
  }
#ifdef PIKE_DEBUG
  if(size != ret->size)
    Pike_fatal("Implode_array failed miserably (%d != %d)\n", size, ret->size);
#endif
  return ret;
}
