/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
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
#include "cyclic.h"
#include "multiset.h"
#include "mapping.h"
#include "bignum.h"
#include "pike_search.h"

/** The empty array. */
PMOD_EXPORT struct array empty_array=
{
  PIKE_CONSTANT_MEMOBJ_INIT(1, PIKE_T_ARRAY), /* Never free */
  0,                     /* Size = 0 */
  0,                     /* malloced Size = 0 */
  0,                     /* no types */
  0,			 /* no flags */
  &weak_empty_array,     /* Next */
  0,			 /* previous */
  empty_array.u.real_item, /* Initialize the item pointer. */
  {{SVALUE_INIT_FREE}},
};

/** The empty weak array. */
PMOD_EXPORT struct array weak_empty_array=
{
  PIKE_CONSTANT_MEMOBJ_INIT(1, PIKE_T_ARRAY), /* Never free */
  0,                     /* Size = 0 */
  0,                     /* malloced Size = 0 */
  0,                     /* no types */
  ARRAY_WEAK_FLAG,	 /* weak */
  0,                     /* next */
  &empty_array,		 /* previous */
  weak_empty_array.u.real_item, /* Initialize the item pointer. */
  {{SVALUE_INIT_FREE}},
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
  size_t length = size;

  if (DO_SIZE_T_ADD_OVERFLOW(length, (size_t)extra_space, &length)) goto TOO_BIG;

  if(length == 0)
  {
    add_ref(&empty_array);
    return &empty_array;
  }

  /*
   * Do we really need this limit?
   *    - arne
   */
  if (length > 1U<<29) goto TOO_BIG;

  /* struct array contains one svalue already */
  length --;

  if (DO_SIZE_T_MUL_OVERFLOW(length, sizeof(struct svalue), &length) ||
      DO_SIZE_T_ADD_OVERFLOW(length, sizeof(struct array), &length)) goto TOO_BIG;

  v=xcalloc(length, 1);

  GC_ALLOC(v);
  gc_init_marker(v);

  /* for now, we don't know what will go in here */
  v->type_field = BIT_MIXED | BIT_UNFINISHED;

  v->malloced_size = (INT32)(size + extra_space);
  v->item=v->u.real_item;
  v->size = (INT32)size;
  INIT_PIKE_MEMOBJ(v, T_ARRAY);
  DOUBLELINK (first_array, v);

  return v;
TOO_BIG:
  Pike_error("Too large array (size %"PRINTSIZET"d is too big).\n", length);
}

/**
 * Free an array without freeing the values inside it.
 * Any values inside of the array will be kept.
 * @param v The array to be freed.
 */
static void array_free_no_free(struct array *v)
{
  DOUBLEUNLINK (first_array, v);

  free(v);

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
 *  Free all elements in an array and set them to zero.
 */
PMOD_EXPORT void clear_array(struct array *a)
{
  if (!a->size) return;
  free_svalues(ITEM(a), a->size, a->type_field);
  /* NB: We know that INT_T == 0. */
  memset(ITEM(a), 0, a->size * sizeof(struct svalue));
  a->type_field = BIT_INT;
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
      types |= 1 << TYPEOF(sval);
      if (!(types & BIT_INT) && (TYPEOF(sval) == PIKE_T_OBJECT) &&
	  (sval.u.object->prog == bignum_program)) {
	/* Lie, and claim that the array contains integers too. */
	types |= BIT_INT;
      }
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
      types |= 1 << TYPEOF(ITEM(a)[e]);
      if (!(types & BIT_INT) && (TYPEOF(ITEM(a)[e]) == PIKE_T_OBJECT) &&
	  (ITEM(a)[e].u.object->prog == bignum_program)) {
	/* Lie, and claim that the array contains integers too. */
	types |= BIT_INT;
      }
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
  switch(TYPEOF(*ind))
  {
    case T_INT: {
      INT_TYPE p = ind->u.integer;
      INT_TYPE i = p < 0 ? p + a->size : p;
      if(i<0 || i>=a->size) {
	struct svalue tmp;
	SET_SVAL(tmp, T_ARRAY, 0, array, a);
	if (a->size) {
          index_error(0,0,&tmp,ind,
		      "Index %"PRINTPIKEINT"d is out of array range "
		      "%d..%d.\n", p, -a->size, a->size-1);
	} else {
          index_error(0,0,&tmp,ind,
		      "Attempt to index the empty array with %"PRINTPIKEINT"d.\n", p);
	}
      }
      array_index_no_free(s,a,i);
      break;
    }

    case T_STRING:
    {
      SET_SVAL(*s, T_ARRAY, 0, array, array_column(a, ind, 0));
      break;
    }

    default:
      {
	struct svalue tmp;
	SET_SVAL(tmp, T_ARRAY, 0, array, a);
        index_error(0,0,&tmp,ind,"Array index is neither int nor string.\n");
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
  /* NB: The following test ought to be within PIKE_DEBUG, but it is enabled
   *     for all in order to catch an intermittent failure in ADT.Sequence
   *     where ITEM(a) appears to be NULL. The failure is triggered by the
   *     pikefarm testsuite on several platforms, but it is uncommon
   *     (~ once every 100 builds).
   *
   * /grubba 2024-10-23
   */
  if (!a || !ITEM(a)) {
    pike_fprintf(stderr, "simple_set_index(%px, %pO, %pO): Invalid array.\n",
                 a, ind, s);
    if (a) fprintf(stderr, "ITEM(%p) is NULL!\n", a);
    Pike_fatal("Invalid array.\n");
  }
  switch (TYPEOF(*ind)) {
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
      mark_free_svalue (Pike_sp++);
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
      SET_SVAL(tmp, T_ARRAY, 0, array, a);
      index_error(0,0,&tmp,ind,"Array index is neither int nor string.\n");
    }
  }
}

/**
 *  Atomically set an element in an array to a value and get
 *  the previous value.
 *
 *  @param a the array whose element is to be set
 *  @param i the index to get and set
 *  @param from_to the value to get and set
 *
 *  On return the @ref svalue at position @b i of the array @b a has been
 *  swapped with the @ref svalue at @b from_to.
 */
PMOD_EXPORT void array_atomic_get_set(struct array *a, INT32 i,
				      struct svalue *from_to)
{
  INT32 p = i;
  struct svalue tmp;

  if (i < 0) i += a->size;

  if(i<0 || i>=a->size) {
    if (a->size) {
      Pike_error("Index %d is out of array range "
		 "%d..%d.\n", p, -a->size, a->size-1);
    } else {
      Pike_error("Attempt to index the empty array with %d.\n", p);
    }
  }

  tmp = ITEM(a)[i];
  ITEM(a)[i] = *from_to;
  *from_to = tmp;
}

/**
 * Insert an svalue into an array and grow the array if necessary.
 */
PMOD_EXPORT struct array *array_insert(struct array *v,struct svalue *s,INT32 index)
{
#ifdef PIKE_DEBUG
  if(index<0 || index>v->size)
    Pike_fatal("Illegal index in low level insert routine.\n");
#endif

  /* Can we fit it into the existing block? */
  if(v->refs<=1 && (v->malloced_size > v->size))
  {
    if ((v->item != v->u.real_item) &&
	(((index<<1) < v->size) ||
	 ((v->item + v->size) == (v->u.real_item + v->malloced_size)))) {
      memmove(ITEM(v)-1, ITEM(v), index * sizeof(struct svalue));
      v->item--;
    } else {
      memmove(ITEM(v)+index+1, ITEM(v)+index,
	      (v->size-index) * sizeof(struct svalue));
    }
    assert_free_svalue (ITEM(v) + index);
    v->size++;
  }else{
    struct array *ret;

    ret = array_set_flags(allocate_array_no_init(v->size+1, v->size + 1),
			  v->flags);
    ret->type_field = v->type_field;

    memcpy(ITEM(ret), ITEM(v), sizeof(struct svalue) * index);
    memcpy(ITEM(ret)+index+1, ITEM(v)+index,
	   sizeof(struct svalue) * (v->size-index));
    assert_free_svalue (ITEM(ret) + index);
    if (v->refs == 1) {
      /* Optimization: Steal the references. */
      v->size = 0;
    } else if (v->type_field & BIT_REF_TYPES) {
      /* Adjust the references. */
      int e = v->size;
      struct svalue *s = ITEM(ret);
      while (e--) {
	if (REFCOUNTED_TYPE(TYPEOF(*s))) add_ref(s->u.dummy);
	s++;
      }
    }
    free_array(v);
    v=ret;
  }

  array_set_index_no_free (v,index,s);

  return v;
}

/*
 * lval += ({ @args });
 *
 * Stack is lvalue followed by a zero placeholder followed by arguments.
 */
void o_append_array(INT32 args)
{
  struct svalue *lval = Pike_sp - args;
  struct svalue *val = lval + 2;
  int lval_type;
#ifdef PIKE_DEBUG
  if (args < 3) {
    Pike_fatal("Too few arguments to o_append_array(): %d\n", args);
  }
#endif
  args -= 3;
  /* Note: val should always be a zero here! */
  lval_type = lvalue_to_svalue_no_free(val, lval);

  if (TYPEOF(*val) == T_ARRAY) {
    struct svalue tmp;
    struct array *v = val->u.array;
    /* simple case: if refs == 2 and there is space, just add the
       element and do not do the assign.  This can be done because the
       lvalue already has the array as it's value.
    */
    if( (v->refs == 2) && (lval_type != PIKE_T_GET_SET) ) {
      if ((TYPEOF(*lval) == T_OBJECT) &&
	  lval->u.object->prog &&
	  ((FIND_LFUN(lval->u.object->prog, LFUN_ASSIGN_INDEX) >= 0) ||
	   (FIND_LFUN(lval->u.object->prog, LFUN_ASSIGN_ARROW) >= 0))) {
	/* There's a function controlling assignments in this object,
	 * so we can't alter the array in place.
	 */
      } else if( v->u.real_item+v->malloced_size >= v->item+v->size+args ) {
        struct svalue *from = val+1;
        int i;
        for( i = 0; i<args; i++,from++ )
        {
          v->item[v->size++] = *from;
          v->type_field |= 1<<TYPEOF(*from);
	  if (!(v->type_field & BIT_INT) && (TYPEOF(*from) == PIKE_T_OBJECT) &&
	      (from->u.object->prog == bignum_program)) {
	    /* Lie, and claim that the array contains integers too. */
	    v->type_field |= BIT_INT;
	  }
        }
        Pike_sp -= args;
        stack_pop_2_elems_keep_top();
        return;
      }
    }
    /* This is so that we can minimize the number of references
     * to the array, and be able to use destructive operations.
     * It's done by freeing the old reference to foo after it has been
     * pushed on the stack. That way foo can have only 1 reference if we
     * are lucky, and then the low array manipulation routines can
     * be destructive if they like.
     */
    SET_SVAL(tmp, PIKE_T_INT, NUMBER_NUMBER, integer, 0);
    assign_lvalue(lval, &tmp);

    if (args == 1) {
      val->u.array = array_insert(v, Pike_sp - 1, v->size);
      pop_stack();
    } else if (!args) {
      /* FIXME: Weak? */
      if ((v->refs > 1) && (v->size)) {
	val->u.array = copy_array(v);
	free_array(v);
      }
    } else {
      int i;
      for (i = 0; i < args; i++) {
	v = array_insert(v, val + 1 + i, v->size);
      }
      val->u.array = v;
      pop_n_elems(args);
    }
    assign_lvalue(lval, val);
  } else {
    int i;
    struct object *o;
    struct program *p;
    /* Fall back to aggregate(). */
    f_aggregate(args);
    if ((TYPEOF(*val) == T_OBJECT) &&
	/* One ref in the lvalue, and one on the stack. */
	((o = val->u.object)->refs <= 2) &&
	(p = o->prog) &&
	(i = FIND_LFUN(p->inherits[SUBTYPEOF(Pike_sp[-2])].prog,
		       LFUN_ADD_EQ)) != -1) {
      apply_low(o, i + p->inherits[SUBTYPEOF(Pike_sp[-2])].identifier_level, 1);
      /* NB: The lvalue already contains the object, so
       *     no need to reassign it.
       */
      pop_stack();
    } else {
      f_add(2);
      assign_lvalue(lval, val);
    }
  }
  stack_pop_2_elems_keep_top();
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

  /* Ensure that one of the empty arrays are returned if size is zero. */
  if( !size )
  {
    struct array *e = (v->flags & ARRAY_WEAK_FLAG ?
		       &weak_empty_array : &empty_array);
    if (e != v) {
      free_array (v);
      add_ref (e);
    }
    return e;
  }

  if (size == v->size) return v;

  /* Free items outside the new array. */
  free_svalues(ITEM(v) + size, v->size - size, v->type_field);
  v->size=size;

  if(size*4 < v->malloced_size + 4) /* Should we realloc it? */
  {
    a = array_set_flags(allocate_array_no_init(size, 0), v->flags);
    if (a->size) {
      a->type_field = v->type_field;
    }

    memcpy(ITEM(a), ITEM(v), size*sizeof(struct svalue));
    v->size=0;
    free_array(v);
    return a;
  }else{
    return v;
  }
}

/**
 * Resize an array destructively, with the exception that a may be one
 * of the static empty arrays.
 */
PMOD_EXPORT struct array *resize_array(struct array *a, INT32 size)
{
#ifdef PIKE_DEBUG
  if(d_flag > 1)  array_check_type_field(a);
#endif

  /* Ensure that one of the empty arrays are returned if size is zero. */
  if (!size && a->malloced_size) return array_shrink (a, size);

  if(a->size == size) return a;
  if(size > a->size)
  {
    /* We should grow the array */

    if((a->malloced_size >= size) &&
       ((a->item + size) <= (a->u.real_item + a->malloced_size)))
    {
      for(;a->size < size; a->size++)
      {
	SET_SVAL(ITEM(a)[a->size], T_INT, NUMBER_NUMBER, integer, 0);
      }
      a->type_field |= BIT_INT;
      return a;
    } else {
      struct array *ret;
      ret = array_set_flags(low_allocate_array(size, size + 1), a->flags);
      memcpy(ITEM(ret), ITEM(a), sizeof(struct svalue)*a->size);
      ret->type_field = (TYPE_FIELD)(a->type_field | BIT_INT);
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
      memcpy(ITEM(a), ITEM(v), index*sizeof(struct svalue));
    if(v->size-index>1)
      memcpy(ITEM(a)+index,
	     ITEM(v)+index+1,
	     (v->size-index-1)*sizeof(struct svalue));
    v->size=0;
    free_array(v);
    return a;
  } else {
    if(v->size-index>1)
    {
      memmove(ITEM(v)+index, ITEM(v)+index+1,
	      (v->size-index-1)*sizeof(struct svalue));
    }
    v->size--;
    return v;
  }
}

static ptrdiff_t fast_array_search( struct array *v, const struct svalue *s,
                                    ptrdiff_t start )
{
  ptrdiff_t e;
  struct svalue *ip = ITEM(v);
  for(e=start;e<v->size;e++)
    if(is_eq(ip+e,s))
      return e;
  return -1;
}

/**
 * Search for in svalue in an array.
 * @param v the array to search
 * @param s the value to search for
 * @param start the index to start search at
 * @return the index if found, -1 otherwise
 */
PMOD_EXPORT ptrdiff_t array_search(struct array *v, const struct svalue *s,
				   ptrdiff_t start)
{
#ifdef PIKE_DEBUG
  if(start<0)
    Pike_fatal("Start of find_index is less than zero.\n");
#endif
#ifdef PIKE_DEBUG
  if(d_flag > 1)  array_check_type_field(v);
#endif
  safe_check_destructed(s);

  /* Why search for something that is not there?
   * however, we must explicitly check for searches
   * for destructed objects/functions
   */
  if((v->type_field & (1 << TYPEOF(*s)))  ||
     (UNSAFE_IS_ZERO(s) && (v->type_field & (BIT_FUNCTION|BIT_OBJECT))) ||
     ( (v->type_field | (1<<TYPEOF(*s)))  & BIT_OBJECT )) /* for overloading */
    return fast_array_search( v, s, start );
  return -1;
}

/**
 * Slice a piece of an array (conditionally destructively)
 * @param v the array to slice
 * @param start the beginning element to be included
 * @param end the element beyond the end of the slice
 * @return an array consisting of v[start..end-1]
 *
 * NOTE: Dangerous as it has an unconventional API!
 * The original array will keep its reference regardless
 * of whether it has been reused (and thus been altered)
 * or not.
 *
 * The conventional API would have been to steal/free a reference
 * from v. This is NOT done here!
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

  if(v->refs==1)	/* Can we use the same array? */
  {
    if((end-start)*4 > v->malloced_size) /* don't waste too much memory */
    {
      add_ref(v);
      free_svalues(ITEM(v) + end, v->size - end, v->type_field);
      free_svalues(ITEM(v), start, v->type_field);
      v->item+=start;
      v->size=end-start;
#ifdef PIKE_DEBUG
      if(d_flag>1)
	check_array(v);
#endif
      return v;
    }
  }

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
      if((TYPEOF(ITEM(v)[e]) == T_OBJECT ||
	  (TYPEOF(ITEM(v)[e]) == T_FUNCTION &&
	   SUBTYPEOF(ITEM(v)[e]) != FUNCTION_BUILTIN)) &&
	 (!ITEM(v)[e].u.object->prog))
      {
	free_svalue(ITEM(v)+e);
	SET_SVAL(ITEM(v)[e], T_INT, NUMBER_DESTRUCTED, integer, 0);

	types |= BIT_INT;
      }else{
	types |= 1<<TYPEOF(ITEM(v)[e]);
	if (!(types & BIT_INT) && (TYPEOF(ITEM(v)[e]) == PIKE_T_OBJECT) &&
	    (ITEM(v)[e].u.object->prog == bignum_program)) {
	  /* Lie, and claim that the array contains integers too. */
	  types |= BIT_INT;
	}
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
      if((TYPEOF(ITEM(v)[e]) == T_OBJECT ||
	  (TYPEOF(ITEM(v)[e]) == T_FUNCTION &&
	   SUBTYPEOF(ITEM(v)[e]) != FUNCTION_BUILTIN)) &&
	 (!ITEM(v)[e].u.object->prog))
	return e;
      types |= 1<<TYPEOF(ITEM(v)[e]);
      if (!(types & BIT_INT) && (TYPEOF(ITEM(v)[e]) == PIKE_T_OBJECT) &&
	  (ITEM(v)[e].u.object->prog == bignum_program)) {
	/* Lie, and claim that the array contains integers too. */
	types |= BIT_INT;
      }
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
  current_order=xalloc(v->size * sizeof(INT32));
  SET_ONERROR(tmp, free, current_order);
  for(e=0; e<v->size; e++) current_order[e]=e;

  get_order_fsort(current_order,
		  current_order+v->size-1,
		  fun,
		  ITEM(v));

  UNSET_ONERROR(tmp);
  return current_order;
}

/* Returns CMPFUN_UNORDERED if no relation is established through lfun
 * calls, or -CMPFUN_UNORDERED if no order defining lfuns (i.e. `< or
 * `>) were found. */
static int lfun_cmp (const struct svalue *a, const struct svalue *b)
{
  struct program *p;
  int default_res = -CMPFUN_UNORDERED, fun;

  if (TYPEOF(*a) == T_OBJECT && (p = a->u.object->prog)) {
    if ((fun = FIND_LFUN(p->inherits[SUBTYPEOF(*a)].prog, LFUN_LT)) != -1) {
      push_svalue(b);
      apply_low(a->u.object,
		fun + p->inherits[SUBTYPEOF(*a)].identifier_level, 1);
      if(!UNSAFE_IS_ZERO(Pike_sp-1))
      {
	pop_stack();
	return -1;
      }
      pop_stack();
      default_res = CMPFUN_UNORDERED;
    }

    if ((fun = FIND_LFUN(p->inherits[SUBTYPEOF(*a)].prog, LFUN_GT)) != -1) {
      push_svalue(b);
      apply_low(a->u.object,
		fun + p->inherits[SUBTYPEOF(*a)].identifier_level, 1);
      if(!UNSAFE_IS_ZERO(Pike_sp-1))
      {
	pop_stack();
	return 1;
      }
      pop_stack();
      default_res = CMPFUN_UNORDERED;
    }

    /* NB: It's not a good idea to use LFUN_EQ here if
     *     there is neither LFUN_LT nor LFUN_GT, since
     *     the sorting order may get confused, which
     *     will cause merge_array_with_order() to fail.
     */
    if ((default_res == CMPFUN_UNORDERED) &&
	(fun = FIND_LFUN(p->inherits[SUBTYPEOF(*a)].prog, LFUN_EQ)) != -1) {
      push_svalue(b);
      apply_low(a->u.object,
		fun + p->inherits[SUBTYPEOF(*a)].identifier_level, 1);
      if (!UNSAFE_IS_ZERO(Pike_sp-1)) {
	pop_stack();
	return 0;
      }
      pop_stack();
    }
  }

  if(TYPEOF(*b) == T_OBJECT && (p = b->u.object->prog)) {
    if ((fun = FIND_LFUN(p->inherits[SUBTYPEOF(*b)].prog, LFUN_LT)) != -1) {
      push_svalue(a);
      apply_low(b->u.object,
		fun + p->inherits[SUBTYPEOF(*b)].identifier_level, 1);
      if(!UNSAFE_IS_ZERO(Pike_sp-1))
      {
	pop_stack();
	return 1;
      }
      pop_stack();
      default_res = CMPFUN_UNORDERED;
    }

    if ((fun = FIND_LFUN(p->inherits[SUBTYPEOF(*b)].prog, LFUN_GT)) != -1) {
      push_svalue(a);
      apply_low(b->u.object,
		fun + p->inherits[SUBTYPEOF(*b)].identifier_level, 1);
      if(!UNSAFE_IS_ZERO(Pike_sp-1))
      {
	pop_stack();
	return -1;
      }
      pop_stack();
      default_res = CMPFUN_UNORDERED;
    }

    /* NB: It's not a good idea to use LFUN_EQ here if
     *     there is neither LFUN_LT nor LFUN_GT, since
     *     the sorting order may get confused, which
     *     will cause merge_array_with_order() to fail.
     */
    if ((default_res == CMPFUN_UNORDERED) &&
	(fun = FIND_LFUN(p->inherits[SUBTYPEOF(*b)].prog, LFUN_EQ)) != -1) {
      push_svalue(a);
      apply_low(b->u.object,
		fun + p->inherits[SUBTYPEOF(*b)].identifier_level, 1);
      if (!UNSAFE_IS_ZERO(Pike_sp-1)) {
	pop_stack();
	return 0;
      }
      pop_stack();
    }
  }

  return default_res;
}

static int obj_or_func_cmp (const struct svalue *a, const struct svalue *b)
/* Call with either two T_OBJECT or two T_FUNCTION. */
{
  int a_subtype, b_subtype, res;
  struct svalue tmp_a, tmp_b;

  assert ((TYPEOF(*a) == T_OBJECT && TYPEOF(*b) == T_OBJECT) ||
	  (TYPEOF(*a) == T_FUNCTION && TYPEOF(*b) == T_FUNCTION));

  if (a->u.object == b->u.object)
    return SUBTYPEOF(*a) - SUBTYPEOF(*b);

  /* Destructed objects are considered equal to each other, and
   * greater than others. That makes them sort close to real zeroes,
   * which are sorted after objects without compare functions. */
  if (!a->u.object->prog)
    return !b->u.object->prog ? 0 : 1;
  else if (!b->u.object->prog)
    return -1;

  if (TYPEOF(*a) == T_FUNCTION) {
    /* Sort pike functions before builtins. */
    if (SUBTYPEOF(*a) == FUNCTION_BUILTIN) {
      if (SUBTYPEOF(*b) == FUNCTION_BUILTIN)
	return a->u.efun < b->u.efun ? -1 : (a->u.efun == b->u.efun ? 0 : 1);
      else
	return 1;
    }
    else
      if (SUBTYPEOF(*b) == FUNCTION_BUILTIN)
	return -1;

    if (a->u.object->prog != b->u.object->prog)
      return a->u.object->prog < b->u.object->prog ? -1 : 1;
    if (SUBTYPEOF(*a) != SUBTYPEOF(*b))
      return SUBTYPEOF(*a) - SUBTYPEOF(*b);

    /* We have the same function but in different objects. Compare the
     * objects themselves. */
    /* FIXME: Should we try to convert the subtypes to the ones for
     * the closest inherits? That'd make some sense if the functions
     * are private, but otherwise it's doubtful. */
    a_subtype = b_subtype = SUBTYPEOF(*a);
    SET_SVAL(tmp_a, T_OBJECT, 0, object, a->u.object);
    a = &tmp_a;
    SET_SVAL(tmp_b, T_OBJECT, 0, object, b->u.object);
    b = &tmp_b;
  }

  else {
    a_subtype = SUBTYPEOF(*a);
    b_subtype = SUBTYPEOF(*b);
  }

  res = lfun_cmp (a, b);

  if (res == -CMPFUN_UNORDERED) {
    /* If the objects had no inequality comparison lfuns to call, use
     * their pointers to get a well defined internal sort order. Let's
     * also group objects cloned from the same program. */
    if (a->u.object->prog == b->u.object->prog)
      return a->u.object < b->u.object ? -1 : 1;
    else
      return a->u.object->prog < b->u.object->prog ? -1 : 1;
  }
  else if (!res)
    return a_subtype - b_subtype;

  return res;
}

int set_svalue_cmpfun(const struct svalue *a, const struct svalue *b)
{
  int typediff = TYPEOF(*a) - TYPEOF(*b);
  if (typediff) {
    if (TYPEOF(*a) == T_OBJECT || TYPEOF(*b) == T_OBJECT) {
      int res = lfun_cmp (a, b);
      if (res != -CMPFUN_UNORDERED) return res;
    }
    return typediff;
  }

  switch(TYPEOF(*a))
  {
    case T_FLOAT:
      if(a->u.float_number < b->u.float_number) return -1;
      if(a->u.float_number > b->u.float_number) return 1;
      return 0;

    case T_INT:
      if(a->u.integer < b->u.integer) return -1;
      if(a->u.integer > b->u.integer) return 1;
      return 0;

    case T_OBJECT:
    case T_FUNCTION:
      return obj_or_func_cmp (a, b);

    default:
      if(a->u.refs < b->u.refs) return -1;
      if(a->u.refs > b->u.refs) return 1;
      return 0;
  }
}

static int switch_svalue_cmpfun(const struct svalue *a, const struct svalue *b)
{
  int typediff = TYPEOF(*a) - TYPEOF(*b);
  if (typediff)
    return typediff;

  switch(TYPEOF(*a))
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
      return (int)my_quick_strcmp(a->u.string, b->u.string);

    case T_OBJECT:
    case T_FUNCTION:
      return obj_or_func_cmp (a, b);

    default:
      if(a->u.refs < b->u.refs) return -1;
      if(a->u.refs > b->u.refs) return 1;
      return 0;
  }
}

int alpha_svalue_cmpfun(const struct svalue *a, const struct svalue *b)
{
  int typediff = TYPEOF(*a) - TYPEOF(*b);
  if (typediff) {
    if (TYPEOF(*a) == T_OBJECT || TYPEOF(*b) == T_OBJECT) {
      int res = lfun_cmp (a, b);
      if (res != -CMPFUN_UNORDERED) return res;
    }
    return typediff;
  }

  switch(TYPEOF(*a))
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
      return (int)my_quick_strcmp(a->u.string, b->u.string);

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
      check_c_stack(1024);
      return alpha_svalue_cmpfun(ITEM(a->u.array), ITEM(b->u.array));

    case T_MULTISET:
      if (a == b) return 0;
      {
        /* Note that multiset_first takes a reference to the multiset
         * data if it does not return -1.
         */
	ptrdiff_t a_pos = multiset_first (a->u.multiset);
	ptrdiff_t b_pos = multiset_first (b->u.multiset);
	int res;
	struct svalue ind_a, ind_b;
	if (a_pos < 0)
        {
          res = (b_pos < 0) ? 0 : -1;
        }
        else if (b_pos < 0)
        {
          res = 1;
        }
        else
        {
          ONERROR a_uwp, b_uwp;
          SET_ONERROR(a_uwp, do_sub_msnode_ref, a->u.multiset);
          SET_ONERROR(b_uwp, do_sub_msnode_ref, b->u.multiset);

          check_c_stack(1024);

          res = alpha_svalue_cmpfun (
            use_multiset_index (a->u.multiset, a_pos, ind_a),
            use_multiset_index (b->u.multiset, b_pos, ind_b));

          UNSET_ONERROR(b_uwp);
          UNSET_ONERROR(a_uwp);
        }
        if (b_pos >= 0) sub_msnode_ref (b->u.multiset);
        if (a_pos >= 0) sub_msnode_ref (a->u.multiset);
	return res;
      }

    case T_OBJECT:
    case T_FUNCTION:
      return obj_or_func_cmp (a, b);

    default:
      if(a->u.ptr < b->u.ptr) return -1;
      if(a->u.ptr > b->u.ptr) return 1;
      return 0;
  }
}

#define CMP(X,Y) alpha_svalue_cmpfun(X,Y)
#define TYPE struct svalue
#define ID low_sort_svalues
#include "fsort_template.h"
#undef CMP
#undef TYPE
#undef ID

/* Same, but only integers. */
static int alpha_int_svalue_cmpfun(const struct svalue *a, const struct svalue *b)
{
#ifdef PIKE_DEBUG
  if ((TYPEOF(*a) != T_INT) || (TYPEOF(*b) != T_INT)) {
    Pike_fatal("Invalid elements in supposedly integer array.\n");
  }
#endif /* PIKE_DEBUG */
  if(a->u.integer < b->u.integer) return -1;
  if(a->u.integer > b->u.integer) return  1;
  return 0;
}

#define CMP(X,Y) alpha_int_svalue_cmpfun(X,Y)
#define TYPE struct svalue
#define ID low_sort_int_svalues
#include "fsort_template.h"
#undef CMP
#undef TYPE
#undef ID

/** This sort is unstable. */
PMOD_EXPORT void sort_array_destructively(struct array *v)
{
  if(!v->size) return;
  if (v->type_field == BIT_INT) {
    low_sort_int_svalues(ITEM(v), ITEM(v)+v->size-1);
  } else {
    low_sort_svalues(ITEM(v), ITEM(v)+v->size-1);
  }
}

#define SORT_BY_INDEX
#define EXTRA_LOCALS int cmpfun_res;
#define CMP(X,Y) ((cmpfun_res =						\
		     (alpha_svalue_cmpfun(svals + X, svals + Y) &	\
		      ~CMPFUN_UNORDERED)) ?				\
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

  /* Overflow safe: ((1<<29)-4)*4 < ULONG_MAX */
  current_order=xalloc(v->size * sizeof(INT32));
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
    c=((unsigned INT32)a+(unsigned INT32)b)/2;
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
  if(TYPEOF(*s) != T_OBJECT && !(a->type_field & BIT_OBJECT))
  {
    /* face it, it's not there */
    if( (((2 << TYPEOF(*s)) -1) & a->type_field) == 0)
      return -1;

  /* face it, it's not there */
    if( ((BIT_MIXED << TYPEOF(*s)) & BIT_MIXED & a->type_field) == 0)
      return ~a->size;
  }

  return low_lookup(a,s,set_svalue_cmpfun);
}

/**
 * Lookup an svalue in a switch table.
 *
 * Returns the array index if found, and
 * if not found the inverse of the index
 * for the first larger element or the
 * inverse of the size of the array if
 * the svalue is larger than all elements
 * of the array.
 */
INT32 switch_lookup(struct svalue *table, struct svalue *s)
{
  struct array *a;

  if (TYPEOF(*table) == PIKE_T_MAPPING) {
    s = low_mapping_lookup(table->u.mapping, s);
    if (s && (TYPEOF(*s) == PIKE_T_INT)) {
      return s->u.integer;
    }
    return -1;
  }

#ifdef PIKE_DEBUG
  if (TYPEOF(*table) != PIKE_T_ARRAY) {
    Pike_fatal("Unsupported switch lookup table type: %s.\n",
               get_name_of_type(TYPEOF(*table)));
  }
#endif
  a = table->u.array;

  /* face it, it's not there */
#ifdef PIKE_DEBUG
  if(d_flag > 1)  array_check_type_field(a);
#endif
  /* objects may have `< `> operators, evil stuff! */
  if(TYPEOF(*s) != T_OBJECT && !(a->type_field & BIT_OBJECT))
  {
    if( (((2 << TYPEOF(*s)) -1) & a->type_field) == 0)
      return -1;

    /* face it, it's not there */
    if( ((BIT_MIXED << TYPEOF(*s)) & BIT_MIXED & a->type_field) == 0)
      return ~a->size;
  }

  return low_lookup(a,s,switch_svalue_cmpfun);
}


/**
 * Reorganize an array in the order specified by 'order'.
 */
PMOD_EXPORT struct array *order_array(struct array *v, const INT32 *order)
{
  reorder((char *)ITEM(v),v->size,sizeof(struct svalue),order);
  return v;
}


/**
 * Copy and reorganize an array.
 */
PMOD_EXPORT struct array *reorder_and_copy_array(const struct array *v, const INT32 *order)
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
    t |= BITOF(ITEM(v)[e]);
    if (!(t & BIT_INT) &&
	(((TYPEOF(ITEM(v)[e]) == PIKE_T_OBJECT) &&
	  ((ITEM(v)[e].u.object->prog == bignum_program) ||
	   !ITEM(v)[e].u.object->prog)) ||
	 ((TYPEOF(ITEM(v)[e]) == PIKE_T_FUNCTION) &&
	  (SUBTYPEOF(ITEM(v)[e]) != FUNCTION_BUILTIN) &&
	  !ITEM(v)[e].u.object->prog))) {
      /*
       * Bignum or destructed object or destructed function.
       *
       * Lie, and claim that the array contains integers too.
       */
      t |= BIT_INT;
#ifdef PIKE_DEBUG
      v->type_field |= BIT_INT;
#endif
    }
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
PMOD_EXPORT void array_check_type_field(const struct array *v)
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
    if(TYPEOF(ITEM(v)[e]) > MAX_TYPE)
      Pike_fatal("Type is out of range.\n");

    t |= 1 << TYPEOF(ITEM(v)[e]);

    if (!(t & BIT_INT) && (TYPEOF(ITEM(v)[e]) == PIKE_T_OBJECT) &&
	(ITEM(v)[e].u.object->prog == bignum_program)) {
      /* Lie, and claim that the array contains integers too. */
      t |= BIT_INT;
    }
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
  if(TYPEOF(ITEM(a)[ind]) == t) return & (ITEM(a)[ind].u);
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
  if(TYPEOF(*ind) != T_INT)
    Pike_error("Expected integer as array index, got %s.\n",
	       get_name_of_type (TYPEOF(*ind)));
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
      ret=xalloc(sizeof(INT32));
      *ret=0;
      return ret;

    case PIKE_ARRAY_OP_SUB:
      /* Overlow safe: ((1<<29)-4+1)*4 < ULONG_MAX */
      ptr=ret=xalloc(sizeof(INT32)*(a->size+1));
      *(ptr++)=a->size;
      for(i=0;i<a->size;i++) *(ptr++)=i;
      return ret;
    }
  }

  /* Note: The following is integer overflow safe as long as
   *       sizeof(struct svalue) >= 2*sizeof(INT32).
   */
  ptr=ret=xalloc(sizeof(INT32)*(a->size + b->size + 1));
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

  *ret = (INT32)(ptr-ret-1);

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

/** Add an arbitrary number of arrays together (destructively).
* @param argp An array of svalues containing the arrays to be concatenated
*             Note that the svalues may get modified by this function.
* @param args The number of elements in argp
* @returns The resulting struct array.
*/
PMOD_EXPORT struct array *add_arrays(struct svalue *argp, INT32 args)
{
  INT32 e, size;
  struct array *v;
  struct array *v2 = NULL;

  for(size=e=0;e<args;e++)
    size+=argp[e].u.array->size;

#if 1
  {
    INT32 tmp=0;	/* Svalues needed so far. */
    INT32 tmp2 = 0;
    INT32 e2 = -1;

    for(e=0;e<args;e++)
    {
      v=argp[e].u.array;
      if(v->refs == 1 && v->malloced_size >= size)
      {
	if (((v->item - v->u.real_item) >= tmp) &&
	    ((v->item + size - tmp) <= (v->u.real_item + v->malloced_size))) {
	  /* There's enough space before and after. */
	  debug_malloc_touch(v);
	  mark_free_svalue(argp + e);
	  for(tmp=e-1;tmp>=0;tmp--)
	  {
	    v2 = argp[tmp].u.array;
	    debug_malloc_touch(v2);
	    v->type_field |= v2->type_field;
	    assign_svalues_no_free(ITEM(v) - v2->size, ITEM(v2),
				   v2->size, v2->type_field);
	    v->item -= v2->size;
	    v->size += v2->size;
	  }

	  for(tmp=e+1;tmp<args;tmp++)
	  {
	    v2 = argp[tmp].u.array;
	    debug_malloc_touch(v2);
	    v->type_field |= v2->type_field;
	    assign_svalues_no_free(ITEM(v) + v->size, ITEM(v2),
				   v2->size, v2->type_field);
	    v->size += v2->size;
	  }
#ifdef PIKE_DEBUG
	  if(d_flag>1)
	    check_array(v);
#endif
	  return v;
	}
	if (!v2 || (v->size > v2->size)) {
	  /* Got a potential candidate.
	   *
	   * Optimize for maximum memmove()
	   * (ie minimum assign_svalues_no_free()).
	   */
	  tmp2 = tmp;
	  v2 = v;
	  e2 = e;
	}
      }
      tmp+=v->size;
    }
    if (v2) {
      debug_malloc_touch(v2);
      mark_free_svalue(argp + e2);
      memmove(v2->u.real_item + tmp2, ITEM(v2),
              v2->size * sizeof(struct svalue));
      v2->item = v2->u.real_item + tmp2;
      for(tmp=e2-1;tmp>=0;tmp--)
      {
	v = argp[tmp].u.array;
	debug_malloc_touch(v);
	v2->type_field |= v->type_field;
	assign_svalues_no_free(ITEM(v2) - v->size, ITEM(v),
			       v->size, v->type_field);
	v2->item -= v->size;
	v2->size += v->size;
      }
      for(tmp=e2+1;tmp<args;tmp++)
      {
	v = argp[tmp].u.array;
	debug_malloc_touch(v);
	v2->type_field |= v->type_field;
	assign_svalues_no_free(ITEM(v2) + v2->size, ITEM(v),
			       v->size, v->type_field);
	v2->size += v->size;
      }
#ifdef PIKE_DEBUG
      if(d_flag>1)
	check_array(v2);
#endif
      return v2;
    }
  }
#endif

  if(args && (v2 = argp[0].u.array)->refs==1)
  {
    e = v2->size;
    v = resize_array(v2, size);
    mark_free_svalue(argp);
    size=e;
    e=1;
  }else{
    v=allocate_array_no_init(size, 0);
    v->type_field=0;
    e=size=0;
  }

  for(; e<args; e++)
  {
    v2 = argp[e].u.array;
    v->type_field |= v2->type_field;
    assign_svalues_no_free(ITEM(v)+size, ITEM(v2), v2->size, v2->type_field);
    size += v2->size;
  }

  return v;
}

PMOD_EXPORT int array_equal_p(struct array *a, struct array *b, struct processing *p)
{
  struct processing curr;
  INT32 e;

  if(a == b) return 1;
  if(!a || !b) return 0;
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
     !( (a->type_field | b->type_field) & (BIT_OBJECT|BIT_FUNCTION) ))
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
  UNSET_ONERROR(r3);  free(zipper);
  UNSET_ONERROR(r2);  free_array(tmpb);
  UNSET_ONERROR(r1);  free_array(tmpa);
  UNSET_ONERROR(r5);  free(orderb);
  UNSET_ONERROR(r4);  free(ordera);
  return ret;
}


#define CMP(X,Y) set_svalue_cmpfun(X,Y)
#define TYPE struct svalue
#define ID set_sort_svalues
#include "fsort_template.h"
#undef CMP
#undef TYPE
#undef ID

/** Remove all instances of an svalue from an array
*/
PMOD_EXPORT struct array *subtract_array_svalue(struct array *a,
						struct svalue *b)
{
  size_t size = a->size;
  size_t from=0, to=0;
  TYPE_FIELD to_type = 1<<TYPEOF(*b);
  TYPE_FIELD type_field = 0;
  ONERROR ouch;
  struct svalue *ip=ITEM(a), *dp=ip;
  int destructive = 1;

  if( size == 0 )
    return copy_array(a);

  if( a->refs > 1 )
  {
    /* We only need to do anything if the value exists in the array. */
    ptrdiff_t off  = fast_array_search( a, b, 0 );
    TYPE_FIELD tmp;

    if( off == -1 )
      /* We still need to return a new array. */
      return copy_array(a);

    /* In this case we generate a new array and modify that one. */
    destructive = 0;
    from = (size_t)off;
    tmp = a->type_field;
    a = allocate_array_no_init(size-1,0);
    a->type_field = tmp;
    SET_ONERROR( ouch, do_free_array, a );
    dp = ITEM(a);

    /* Copy the part of the array that is not modified first.. */
    for( to=0; to<from; to++, ip++, dp++)
    {
      assign_svalue_no_free(dp, ip);
      type_field |= 1<<TYPEOF(*dp);
      if (!(type_field & BIT_INT) && (TYPEOF(*dp) == PIKE_T_OBJECT) &&
	  (dp->u.object->prog == bignum_program)) {
	/* Lie, and claim that the array contains integers too. */
	type_field |= BIT_INT;
      }
    }
    a->size = from;
  }

#define MATCH_COPY(X)  do {                                                 \
    if( X )                                                                 \
    {  /* include entry */                                                  \
      type_field|=1<<TYPEOF(*ip);                                           \
      if (!(type_field & BIT_INT) && (TYPEOF(*ip) == PIKE_T_OBJECT) &&	    \
	  (ip->u.object->prog == bignum_program)) {			    \
	/* Lie, and claim that the array contains integers too. */	    \
	type_field |= BIT_INT;						    \
      }									    \
      if(!destructive)                                                      \
        assign_svalue_no_free(dp,ip);                                       \
      else if(ip!=dp)                                                       \
        *dp=*ip;                                                            \
      dp++;                                                                 \
      if( !destructive ) a->size++;                                         \
    }                                                                       \
    else if( destructive )                                                  \
      free_svalue( ip );                                                    \
  } while(0)


  if( UNSAFE_IS_ZERO( b ) )
  {
    /* Remove 0-valued elements.
       Rather common, so a special case is motivated.

       This saves time becase there is no need to check if 'b' is zero
       for each loop.
    */
    for( ;from<size; from++, ip++ )
      MATCH_COPY( !UNSAFE_IS_ZERO(ip) );
  }
  else if((a->type_field & to_type) || ((a->type_field | to_type) & BIT_OBJECT))
  {
    for( ; from<size; from++, ip++ )
      MATCH_COPY( !is_eq(ip,b) );
  }
  else /* b does not exist in the array. */
  {
    add_ref(a);
    return a;
  }
#undef MATCH_COPY

  if( dp != ip )
  {
    a->type_field = type_field;
    a->size = dp-ITEM(a);
  }

  if( !destructive )
    UNSET_ONERROR( ouch );
  else
    add_ref(a);

  if( a->size )
    return a;

  free_array(a);
  add_ref(&empty_array);
  return &empty_array;
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
  if( b->size == 1 )
    return subtract_array_svalue( a, ITEM(b) );

  if(b->size &&
     ((a->type_field & b->type_field) ||
      ((a->type_field | b->type_field) & BIT_OBJECT)))
  {
    return merge_array_with_order(a, b, PIKE_ARRAY_OP_SUB);
  }else{
    if(a->refs == 1)
    {
      add_ref(a);
      return a;
    }
    return copy_array(a);
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

/* Return true for integers with more than one bit set */
static inline int is_more_than_one_bit(unsigned INT32 x)
{
  return !!(x & (x-1));
}

node *make_node_from_array(struct array *a)
{
  struct svalue s;
  INT32 e;

  if(!a->size)
    return mkefuncallnode("aggregate",0);
  if (a->size == 1)
    return mkefuncallnode("aggregate", mksvaluenode(ITEM(a)));

  if(array_fix_type_field(a) == BIT_INT)
  {
    debug_malloc_touch(a);
    for(e=0; e<a->size; e++)
      if(ITEM(a)[e].u.integer || SUBTYPEOF(ITEM(a)[e]))
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
	  if((ITEM(a)[e].u.integer != ITEM(a)[0].u.integer) ||
	     (SUBTYPEOF(ITEM(a)[e]) != SUBTYPEOF(ITEM(a)[0]))) {
	    break;
	  }
	if(e==a->size && ITEM(a)[0].u.integer==0 && !SUBTYPEOF(ITEM(a)[0]))
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
	     SUBTYPEOF(ITEM(a)[e]) != SUBTYPEOF(ITEM(a)[0]))
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
    SET_SVAL(s, T_ARRAY, 0, array, a);
    return mkconstantsvaluenode(&s);
  }else{
    node *ret=0;
    debug_malloc_touch(a);
    for(e = a->size; e--;) {
      if (ret) {
	ret = mknode(F_ARG_LIST, mksvaluenode(ITEM(a)+e), ret);
      } else {
	ret = mksvaluenode(ITEM(a)+e);
      }
    }
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
    memcpy(Pike_sp,ITEM(a),sizeof(struct svalue)*a->size);
    Pike_sp += a->size;
    a->size=0;
    free_array(a);
  }else{
    assign_svalues_no_free(Pike_sp, ITEM(a), a->size, a->type_field);
    Pike_sp += a->size;
    free_array(a);
  }
}

void describe_array_low(struct byte_buffer *buf, struct array *a, struct processing *p, int indent)
{
  INT32 e,d;
  indent += 2;

  for(e=0; e<a->size; e++)
  {
    buffer_ensure_space(buf, indent + 2);
    if(e) buffer_add_str_unsafe(buf, ",\n");
    for(d=0; d<indent; d++) buffer_add_char_unsafe(buf, ' ');
    describe_svalue(buf, ITEM(a)+e,indent,p);
  }
}


#ifdef PIKE_DEBUG
void simple_describe_array(struct array *a)
{
  char *s;
  if (!a) {
    fputs("NULL-array\n", stderr);
  } else if (a->size) {
    struct byte_buffer buf = BUFFER_INIT();
    describe_array_low(&buf,a,0,0);
    fprintf(stderr,"({\n%s\n})\n",buffer_get_string(&buf));
    buffer_free(&buf);
  }
  else
    fputs ("({ })\n", stderr);
}
#endif


void describe_array(struct byte_buffer *buffer,struct array *a,struct processing *p,int indent)
{
  struct processing doing;
  INT32 e;
  char buf[60];

  if (!a) {
    buffer_add_str(buffer, "UNDEFINED");
    return;
  }

  if(! a->size)
  {
    buffer_add_str(buffer, "({ })");
    return;
  }

  doing.next=p;
  doing.pointer_a=(void *)a;
  for(e=0;p;e++,p=p->next)
  {
    if(p->pointer_a == (void *)a)
    {
      sprintf(buf,"@%ld",(long)e);
      buffer_add_str(buffer, buf);
      return;
    }
  }

  if (a->size == 1) {
    sprintf(buf, "({ /* 1 element */\n");
  } else {
    sprintf(buf, "({ /* %ld elements */\n", (long)a->size);
  }
  buffer_add_str(buffer, buf);
  describe_array_low(buffer,a,&doing,indent);
  buffer_add_char(buffer, '\n');
  for(e=2; e<indent; e++) buffer_add_char(buffer, ' ');
  buffer_add_str(buffer, "})");
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
    memcpy(ITEM(a),Pike_sp-args,args*sizeof(struct svalue));
    array_fix_type_field (a);
    Pike_sp-=args;
    DO_IF_DMALLOC(while(args--) dmalloc_touch_svalue(Pike_sp + args));
  }
  return a;
}

/**
 * Add an element to the end of an array by resizing the array.
 *
 * @param a the array to be appended
 * @param s the value to be added to the new element in the array
 */
PMOD_EXPORT struct array *append_array(struct array *a, struct svalue *s)
{
  INT32 size = a->size;
  a=resize_array(a, size+1);
  array_set_index(a, size, s);
  return a;
}

/**
 * Automap assignments
 * This implements X[*] = ...[*]..
 * Assign elements in a at @level to elements from b at the same @level.
 * This will not actually modify any of the arrays, only change the
 * values in them.
 */
void assign_array_level( struct array *a, struct array *b, int level )
{
    if( a->size != b->size )
      /* this should not really happen. */
        Pike_error("Source and destination differs in size in automap?!\n");

    if( level > 1 )
    {
        /* recurse. */
        INT32 i;
        for( i=0; i<a->size; i++ )
        {
            if( TYPEOF(a->item[i]) != PIKE_T_ARRAY )
                Pike_error("Too many automap levels.\n");
            if( TYPEOF(b->item[i]) != PIKE_T_ARRAY ) /* obscure messages much? */
                Pike_error("Not enough levels of mapping in RHS\n");
            assign_array_level( a->item[i].u.array, b->item[i].u.array, level-1 );
        }
    }
    else {
      assign_svalues( a->item, b->item, a->size,
		      a->type_field|b->type_field );
      a->type_field = b->type_field;
    }
}

/* Assign all elemnts in a at level to b.
 * This implements X[*] = expression without automap.
 */
void assign_array_level_value( struct array *a, struct svalue *b, int level )
{
    INT32 i;
    if( level > 1 )
    {
        /* recurse. */
        for( i=0; i<a->size; i++ )
        {
            if( TYPEOF(a->item[i]) != PIKE_T_ARRAY )
                Pike_error("Too many automap levels.\n");
            assign_array_level_value( a->item[i].u.array, b, level-1 );
        }
    }
    else
    {
        if( a->type_field & BIT_REF_TYPES )  free_mixed_svalues( a->item, a->size );
        if( REFCOUNTED_TYPE(TYPEOF(*b)) )     *b->u.refs+=a->size;
        for( i=0; i<a->size; i++)
            a->item[i] = *b;
	a->type_field = 1 << TYPEOF(*b);
    }
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
      SET_SVAL(ITEM(ret)[e], T_STRING, 0, string, string_slice(str,e,1));
    }
  }else{
    SearchMojt mojt;
    ONERROR uwp;
    explode_searchfunc f = (explode_searchfunc)0;

    s=str->str;
    end=s+(str->len << str->size_shift);

    ret=allocate_array(2);
    ret->size=0;

    mojt=compile_memsearcher(MKPCHARP_STR(del),
			     del->len,
			     str->len,
			     del);
    SET_ONERROR (uwp, do_free_object, mojt.container);

    switch(str->size_shift)
    {
      case eightbit:     f=(explode_searchfunc)mojt.vtab->func0; break;
      case sixteenbit:   f=(explode_searchfunc)mojt.vtab->func1; break;
      case thirtytwobit: f=(explode_searchfunc)mojt.vtab->func2; break;
      default: Pike_fatal("Invalid size_shift: %d.\n", str->size_shift);
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

      SET_SVAL(ITEM(ret)[ret->size], T_STRING, 0, string,
	       string_slice(str,
			    (s-str->str)>>str->size_shift,
			    (tmp-s)>>str->size_shift));
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

    SET_SVAL(ITEM(ret)[ret->size], T_STRING, 0, string,
	     string_slice(str,
			  (s-str->str)>>str->size_shift,
			  (end-s)>>str->size_shift));
    ret->size++;

    CALL_AND_UNSET_ONERROR (uwp);
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
  INT32 len, e, delims;
  PCHARP r;
  struct pike_string *ret;
  struct svalue *ae;
  int max_shift = del->size_shift;

  len=0;
  delims = 0;



  for(e=a->size, ae=a->item; e--; ae++)
    switch(TYPEOF(*ae))
    {
      case T_INT:
	 if(!ae->u.integer)
	   continue;		    /* skip zero (strings) */
	 /* FALLTHROUGH */
      default:
        Pike_error("Array element %td is not a string\n", ae-a->item);
	break;
      case T_STRING:
	delims++;
        len+=ae->u.string->len + del->len;
        if(ae->u.string->size_shift > max_shift)
	  max_shift=ae->u.string->size_shift;
	break;
    }

  if(delims)
  {
    len-=del->len;
    delims--;
  }

  if( a->size == 1 && TYPEOF(*ITEM(a)) == PIKE_T_STRING )
  {
      struct pike_string * res = ITEM(a)->u.string;
      res->refs++;
      return res;
  }

  ret=begin_wide_shared_string(len,max_shift);
  r=MKPCHARP_STR(ret);
  len = del->len;
  if((e = a->size))
    for(ae=a->item;e--;ae++)
    {
      if (TYPEOF(*ae) == T_STRING)
      {
	struct pike_string *tmp = ae->u.string;
	pike_string_cpy(r,tmp);
	INC_PCHARP(r,tmp->len);
	if(len && delims)
	{
	  delims--;
	  pike_string_cpy(r,del);
	  INC_PCHARP(r,len);
	}
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
    ret = (a->flags & ARRAY_WEAK_FLAG) ? &weak_empty_array : &empty_array;
    add_ref(ret);
    return ret;
  }

  ret=allocate_array_no_init(a->size,0);

  if (m) {
    SET_SVAL(aa, T_ARRAY, 0, array, a);
    SET_SVAL(bb, T_ARRAY, 0, array, ret);
    low_mapping_insert(m, &aa, &bb, 1);
  }

  ret->flags = a->flags & ~ARRAY_LVALUE;

  copy_svalues_recursively_no_free(ITEM(ret),ITEM(a),a->size,m);

  ret->type_field=a->type_field;
  return ret;
}

/** Apply the elements of an array. Arguments the array should be
 *  applied with should be on the stack before the call and the
 *  resulting array will be on the stack after the call.
 *
 *  Note that the array a may be modified destructively if it has
 *  only a single reference.
 */
PMOD_EXPORT void apply_array(struct array *a, INT32 args, int flags)
{
  INT32 e, hash = 0;
  struct svalue *argp = Pike_sp-args;
  struct array *cycl;
  DECLARE_CYCLIC();

  check_stack(args);
  check_array_for_destruct(a);
  for (e=0; e<args; e++)
    hash = hash * 33 + (INT32) PTR_TO_INT (Pike_sp[-e-1].u.ptr);

  if (!(cycl = (struct array *)BEGIN_CYCLIC(a, (ptrdiff_t)hash))) {
    TYPE_FIELD new_types = 0;
    struct array *aa;
    if ((flags & 1) && (a->refs == 1)) {
      /* Destructive operation possible. */
      add_ref(aa = a);
      aa->type_field |= BIT_UNFINISHED;
    } else {
      aa = allocate_array(a->size);
    }
    SET_CYCLIC_RET(aa);
    push_array(aa);
    for (e=0; e < a->size; e++)
    {
      assign_svalues_no_free(Pike_sp, argp, args, BIT_MIXED);
      Pike_sp+=args;
      /* FIXME: Don't throw apply errors from apply_svalue here. */
      apply_svalue(ITEM(a)+e, args);
      new_types |= 1 << TYPEOF(Pike_sp[-1]);
      if (!(new_types & BIT_INT) && (TYPEOF(Pike_sp[-1]) == PIKE_T_OBJECT) &&
	  (Pike_sp[-1].u.object->prog == bignum_program)) {
	/* Lie, and claim that the array contains integers too. */
	new_types |= BIT_INT;
      }
      assign_svalue(ITEM(aa)+e, &Pike_sp[-1]);
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
PMOD_EXPORT struct array *reverse_array(struct array *a, int start, int end)
{
  INT32 e;
  struct array *ret;

  if ((end <= start) || (start >= a->size)) {
    add_ref(a);
    return a;
  }
  if (end >= a->size) {
    end = a->size;
  } else {
    end++;
  }

  if(a->refs == 1)
    /* Reverse in-place. */
  {
    struct svalue *tmp0, *tmp1, swap;

    tmp0 = ITEM(a) + start;
    tmp1 = ITEM(a) + end;
    while (tmp0 < tmp1) {
      swap = *tmp0;
      *(tmp0++) = *(--tmp1);
      *tmp1 = swap;
    }

    /* FIXME: What about the flags field? */

    add_ref(a);
    return a;
  }

  ret=allocate_array_no_init(a->size,0);
  for(e=0;e<start;e++)
    assign_svalue_no_free(ITEM(ret)+e,ITEM(a)+e);
  for(;e<end;e++)
    assign_svalue_no_free(ITEM(ret)+e,ITEM(a)+end+~e-start);
  for(;e<a->size;e++)
    assign_svalue_no_free(ITEM(ret)+e,ITEM(a)+e);
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
  check_array_for_destruct(a);
  while((i=fast_array_search(a,from,i+1)) >= 0) array_set_index(a,i,to);
}

/**
 * Perform a quick gc of the specified weak array.
 *
 * @param a The weak array to be garbage collected.
 * @return The number of freed elements.
 *
 * @see do_gc
 */
ptrdiff_t do_gc_weak_array(struct array *a)
{
  INT32 e;
  ptrdiff_t res = 0;

  if (!(a->flags & ARRAY_WEAK_FLAG)) {
    return 0;
  }

  for (e = 0; e < a->size; e++) {
    struct svalue *s = ITEM(a) + e;
    if (!REFCOUNTED_TYPE(TYPEOF(*s)) || (*s->u.refs > 1)) {
      continue;
    }
    /* NB: cf svalue.c:ZAP_SVALUE(). */
    free_svalue(s);
    SET_SVAL(*s, T_INT, NUMBER_DESTRUCTED, integer, 0);
    res++;
  }
  return res;
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

  if((a->item + a->size) > (a->u.real_item + a->malloced_size))
    Pike_fatal("Array uses memory outside of the malloced block!\n");

  if(a->item < a->u.real_item)
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
    if(! ( (1 << TYPEOF(ITEM(a)[e])) & (a->type_field) ) &&
       TYPEOF(ITEM(a)[e])<16)
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


PMOD_EXPORT void visit_array (struct array *a, int action, void *extra)
{
  visit_enter(a, T_ARRAY, extra);
  switch (action & VISIT_MODE_MASK) {
#ifdef PIKE_DEBUG
    default:
      Pike_fatal ("Unknown visit action %d.\n", action);
    case VISIT_NORMAL:
    case VISIT_COMPLEX_ONLY:
      break;
#endif
    case VISIT_COUNT_BYTES:
      mc_counted_bytes += sizeof (struct array) +
	(a->malloced_size - 1) * sizeof (struct svalue);
      break;
  }

  if (!(action & VISIT_NO_REFS) &&
      a->type_field &
      (action & VISIT_COMPLEX_ONLY ? BIT_COMPLEX : BIT_REF_TYPES)) {
    size_t e, s = a->size;
    int ref_type = a->flags & ARRAY_WEAK_FLAG ? REF_TYPE_WEAK : REF_TYPE_NORMAL;
    for (e = 0; e < s; e++)
      visit_svalue (ITEM (a) + e, ref_type, extra);
  }
  visit_leave(a, T_ARRAY, extra);
}

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

PMOD_EXPORT void gc_mark_array_as_referenced(struct array *a)
{
  if(gc_mark(a, T_ARRAY))
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

PMOD_EXPORT void real_gc_cycle_check_array(struct array *a, int weak)
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
void count_memory_in_arrays(size_t *num_, size_t *size_)
{
  size_t num=0, size=0;
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
    if(TYPEOF(ITEM(a)[e]) != T_ARRAY)
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
