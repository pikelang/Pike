/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
#include "svalue.h"
#include "array.h"
#include "object.h"
#include "las.h"
#include "stralloc.h"
#include "interpret.h"
#include "language.h"
#include "error.h"
#include "lpc_types.h"
#include "fsort.h"
#include "builtin_efuns.h"
#include "memory.h"

struct array empty_array=
{
  1,                     /* Never free */
  &empty_array,          /* Next */
  &empty_array,          /* previous (circular) */
  0,                     /* Size = 0 */
  0,                     /* malloced Size = 0 */
  0,                     /* no types */
  T_MIXED,                 /* mixed array */
  0,                     /* no flags */
};



/* Allocate an array, this might be changed in the future to
 * allocate linked lists or something
 * NOTE: the new array have zero references
 */

struct array *allocate_array_no_init(INT32 size,INT32 extra_space,TYPE_T type)
{
  struct array *v;

  if(size == 0 && type == T_MIXED)
  {
    empty_array.refs++;
    return &empty_array;
  }

  if(type == T_FUNCTION || type == T_MIXED)
  {
    v=(struct array *)malloc(sizeof(struct array_of_svalues)+
                             (size+extra_space-1)*sizeof(struct svalue));
    if(!v)
      error("Couldn't allocate array, out of memory.\n");
    v->array_type=T_MIXED;
    /* for now, we don't know what will go in here */
    v->type_field=BIT_MIXED;
  }else{
    v=(struct array *)malloc(sizeof(struct array_of_short_svalues)+
                             (size+extra_space-1)*sizeof(union anything));
    if(!v)
      error("Couldn't allocate array, out of memory.\n");
    v->array_type=type;
    /* This array can only contain zeros and 'type' */
    v->type_field=BIT_INT | (1 << type);
  }
  v->malloced_size=size+extra_space;
  v->size=size;
  v->flags=0;
  v->refs=1;
  v->prev=&empty_array;
  v->next=empty_array.next;
  empty_array.next=v;
  v->next->prev=v;

  return v;
}

struct array *allocate_array(INT32 size,TYPE_T type)
{
  INT32 e;
  struct array *a;
  a=allocate_array_no_init(size,0,type);
  if(a->array_type==T_MIXED)
  {
    for(e=0;e<a->size;e++)
    {
      ITEM(a)[e].type=T_INT;
      ITEM(a)[e].subtype=NUMBER_NUMBER;
      ITEM(a)[e].u.integer=0;
    }
  }else{
    MEMSET((char *)SHORT_ITEM(a),0,sizeof(union anything)*a->size);
  }
  return a;
}

/*
 * Free an array without freeing the values inside it
 */
static void array_free_no_free(struct array *v)
{
  struct array *next,*prev;

  next = v->next;
  prev = v->prev;

  v->prev->next=next;
  v->next->prev=prev;

  free((char *)v);
}

/*
 * Free an array, call this when the array has zero references
 */
void really_free_array(struct array *v)
{

#ifdef DEBUG
  if(v == & empty_array)
    fatal("Tried to free the empty_array.\n");
#endif

  if(v->array_type == T_MIXED)
  {
    free_svalues(ITEM(v), v->size);
  }else{
    free_short_svalues(SHORT_ITEM(v), v->size, v->array_type);
  }

  array_free_no_free(v);
}

/*
 * Extract an svalue from an array
 */
void array_index_no_free(struct svalue *s,struct array *v,INT32 index)
{
#ifdef DEBUG
  if(index<0 || index>=v->size)
    fatal("Illegal index in low level index routine.\n");
#endif

  if(v->array_type == T_MIXED)
  {
    assign_svalue_no_free(s, ITEM(v) + index);
  }else{
    assign_from_short_svalue_no_free(s, SHORT_ITEM(v) + index, v->array_type);
  }
}

/*
 * Extract an svalue from an array
 */
void array_index(struct svalue *s,struct array *v,INT32 index)
{
#ifdef DEBUG
  if(index<0 || index>=v->size)
    fatal("Illegal index in low level index routine.\n");
#endif

  v->refs++;
  if(v->array_type == T_MIXED)
  {
    assign_svalue(s, ITEM(v) + index);
  }else{
    free_svalue(s);
    assign_from_short_svalue_no_free(s, SHORT_ITEM(v) + index, v->array_type);
  }
  free_array(v);
}

void simple_array_index(struct svalue *s,struct array *a,struct svalue *ind)
{
  INT32 i;
  if(ind->type != T_INT)
    error("Index is not an integer.\n");
  i=ind->u.integer;
  if(i<0) i+=a->size;
  if(i<0 || i>=a->size) error("Index out of range.\n");
  array_index(s,a,i);
}

void simple_array_index_no_free(struct svalue *s,struct array *a,struct svalue *ind)
{
  INT32 i;
  if(ind->type != T_INT)
    error("Index is not an integer.\n");
  i=ind->u.integer;
  if(i<0) i+=a->size;
  if(i<0 || i>=a->size) error("Index out of range.\n");
  array_index_no_free(s,a,i);
}

/*
 * Extract an svalue from an array
 */
void array_free_index(struct array *v,INT32 index)
{
#ifdef DEBUG
  if(index<0 || index>=v->size)
    fatal("Illegal index in low level free index routine.\n");
#endif

  if(v->array_type == T_MIXED)
  {
    free_svalue(ITEM(v) + index);
  }else{
    free_short_svalue(SHORT_ITEM(v) + index, v->array_type);
  }
}

/*
 * Set an index in an array
 */
void array_set_index(struct array *v,INT32 index, struct svalue *s)
{
#ifdef DEBUG
  if(index<0 || index>v->size)
    fatal("Illegal index in low level array set routine.\n");
#endif

  v->refs++;
  check_destructed(s);
  if(v->array_type == T_MIXED)
  {
    v->type_field |= 1 << s->type;
    assign_svalue( ITEM(v) + index, s);
  }else if(IS_ZERO(s)){
    v->type_field |= BIT_INT;
    SHORT_ITEM(v)[index].refs=0;
  }else if(v->array_type == s->type){
    assign_to_short_svalue( SHORT_ITEM(v)+index, v->array_type, s);
  }else{
    free_array(v);
    error("Wrong type in array assignment (%s != %s)\n",
	  get_name_of_type(v->array_type),
	  get_name_of_type(s->type));
  }
  free_array(v);
}


void simple_set_index(struct array *a,struct svalue *ind,struct svalue *s)
{
  INT32 i;
  if(ind->type != T_INT)
    error("Index is not an integer.\n");
  i=ind->u.integer;
  if(i<0) i+=a->size;
  if(i<0 || i>=a->size) error("Index out of range.\n");
  array_set_index(a,i,s);
}

/*
 * Insert an svalue into an array, grow the array if nessesary
 */
struct array *array_insert(struct array *v,struct svalue *s,INT32 index)
{
#ifdef DEBUG
  if(index<0 || index>v->size)
    fatal("Illegal index in low level insert routine.\n");
#endif

  /* Can we fit it into the existing block? */
  if(v->refs<=1 && v->malloced_size > v->size)
  {
    if(v->array_type == T_MIXED)
    {
      MEMMOVE((char *)(ITEM(v)+index+1),
              (char *)(ITEM(v)+index),
              (v->size-index) * sizeof(struct svalue));
      ITEM(v)[index].type=T_INT;
    }else{
      MEMMOVE((char *)(SHORT_ITEM(v)+index+1),
              (char *)(SHORT_ITEM(v)+index),
              (v->size-index) * sizeof(union anything));
      SHORT_ITEM(v)[index].refs=0;
    }
    v->size++;
  }else{
    struct array *ret;

    ret=allocate_array_no_init(v->size+1, (v->size >> 3) + 1, v->array_type);
    ret->type_field = v->type_field;

    if(v->array_type == T_MIXED)
    {
      MEMCPY(ITEM(ret), ITEM(v), sizeof(struct svalue) * index);
      MEMCPY(ITEM(ret)+index+1, ITEM(v)+index, sizeof(struct svalue) * (v->size-index));
      ITEM(ret)[index].type=T_INT;
    }else{
      MEMCPY(SHORT_ITEM(ret), SHORT_ITEM(v), sizeof(union anything) * index);

      MEMCPY(SHORT_ITEM(ret)+index+1,
             SHORT_ITEM(v)+index,
             sizeof(union anything) * (v->size-index));
      SHORT_ITEM(ret)[index].refs=0;
    }
    v->size=0;
    free_array(v);
    v=ret;
  }

  array_set_index(v,index,s);

  return v;
}

/*
 * resize array, resize an array destructively
 */
static struct array *resize_array(struct array *a, INT32 size)
{
  if(a->size == size) return a;
  if(size > a->size)
  {
    /* We should grow the array */
    if(a->malloced_size >= size)
    {
      if(a->array_type == T_MIXED)
      {
	for(;a->size < size; a->size++)
	{
	  ITEM(a)[a->size].type=T_INT;
	  ITEM(a)[a->size].subtype=NUMBER_NUMBER;
	  ITEM(a)[a->size].u.integer=0;
	}
      }else{
	MEMSET(SHORT_ITEM(a)+a->size,
	       0,
	       sizeof(union anything)*(size-a->size));
	a->size=size;
      }
      return a;
    }else{
      struct array *ret;
      ret=allocate_array_no_init(size, (size>>3)+1, a->array_type);
      if(a->array_type == T_MIXED)
      {
	MEMCPY(ITEM(ret),ITEM(a),sizeof(struct svalue)*a->size);
      }else{
	MEMCPY(SHORT_ITEM(ret),SHORT_ITEM(a),sizeof(union anything)*a->size);
      }
      a->size=0;
      free_array(a);
      return ret;
    }
  }else{
    /* We should shrink the array */
    if(a->array_type == T_MIXED)
    {
      free_svalues(ITEM(a)+size, a->size - size);
    }else{
      free_short_svalues(SHORT_ITEM(a)+size, a->size - size, a->array_type);
    }
    a->size = size;
    return a;
  }
}

/*
 * Shrink an array destructively
 */
struct array *array_shrink(struct array *v,INT32 size)
{
  struct array *a;

#ifdef DEBUG
  if(v->refs>2) /* Odd, but has to be two */
    fatal("Array shrink on array with many references.\n");

  if(size > v->size)
    fatal("Illegal argument to array_shrink.\n");
#endif

  if(size*2 < v->malloced_size + 4) /* Should we realloc it? */
  {
    a=allocate_array_no_init(size,0,v->array_type);
    a->type_field = v->type_field;

    if(v->array_type == T_MIXED)
    {
      free_svalues(ITEM(v) + size, v->size - size);
      MEMCPY(ITEM(a), ITEM(v), size*sizeof(struct svalue));
    }else{
      free_short_svalues(SHORT_ITEM(v) + size, v->size - size, v->array_type);
      MEMCPY(ITEM(a), ITEM(v), size*sizeof(union anything));
    }
    v->size=0;
    free_array(v);
    return a;
  }else{
    if(v->array_type == T_MIXED)
    {
      free_svalues(ITEM(v) + size, v->size - size);
    }else{
      free_short_svalues(SHORT_ITEM(v) + size, v->size - size, v->array_type);
    }
    v->size=size;
    return v;
  }
}

/*
 * Remove an index from an array and shrink the array
 */
struct array *array_remove(struct array *v,INT32 index)
{
  struct array *a;

#ifdef DEBUG
  if(v->refs>1)
    fatal("Array remove on array with many references.\n");

  if(index<0 || index >= v->size)
    fatal("Illegal argument to array_remove.\n");
#endif

  array_free_index(v, index);
  if(v->size!=1 &&
     v->size*2 + 4 < v->malloced_size ) /* Should we realloc it? */
  {
    a=allocate_array_no_init(v->size-1, 0, v->array_type);
    a->type_field = v->type_field;

    if(v->array_type == T_MIXED)
    {
      if(index>0)
	MEMCPY(ITEM(a), ITEM(v), index*sizeof(struct svalue));
      if(v->size-index>1)
	MEMCPY(ITEM(a)+index,
	       ITEM(v)+index+1,
	       (v->size-index-1)*sizeof(struct svalue));
    }else{
      if(index>0)
	MEMCPY(SHORT_ITEM(a), SHORT_ITEM(v), index*sizeof(struct svalue));
      if(v->size-index>1)
	MEMCPY(SHORT_ITEM(a)+index,
	       SHORT_ITEM(v)+index+1,
	       (v->size-index-1)*sizeof(union anything));
    }
    v->size=0;
    free_array(v);
    return a;
  }else{
    if(v->size-index>1)
    {
      if(v->array_type == T_MIXED)
      {
	MEMMOVE((char *)(ITEM(v)+index),
		(char *)(ITEM(v)+index+1),
	        (v->size-index-1)*sizeof(struct svalue));
      }else{
	MEMMOVE((char *)(SHORT_ITEM(v)+index),
		(char *)(SHORT_ITEM(v)+index+1),
		(v->size-index-1)*sizeof(union anything));
      }
    }
    v->size--;
    return v;
  }
}

/*
 * Search for in svalue in an array.
 * return the index if found, -1 otherwise
 */
INT32 array_search(struct array *v, struct svalue *s,INT32 start)
{
  INT32 e;
#ifdef DEBUG
  if(start<0)
    fatal("Start of find_index is less than zero.\n");
#endif

  check_destructed(s);

  if(v->type_field & (1 << s->type)) /* Why search for something that is not there? */
  {
    if(v->array_type == T_MIXED)
    {
      TYPE_FIELD t=0;
      for(e=start;e<v->size;e++)
      {
	if(is_eq(ITEM(v)+e,s)) return e;
	t |= 1<<ITEM(v)[e].type;
      }
      v->type_field=t;
      return -1;
    }else{
      if(v->array_type == T_FLOAT)
      {
	if(s->type == T_FLOAT)
	  for(e=start;e<v->size;e++)
	    if(SHORT_ITEM(v)[e].float_number == s->u.float_number)
	      return e;
	return -1;
      }
      if(v->array_type == s->type)
      {
	for(e=start;e<v->size;e++)
	  if(SHORT_ITEM(v)[e].refs == s->u.refs)
	    return e;
	return -1;
      }

      if(IS_ZERO(s))
	for(e=start;e<v->size;e++)
	  if(!SHORT_ITEM(v)[e].refs)
	    return e;

      return -1;
    }
  }

  return -1;
}

/*
 * Slice a pice of an array (nondestructively)
 * return an array consisting of v[start..end-1]
 */
struct array *slice_array(struct array *v,INT32 start,INT32 end)
{
  struct array *a;

#ifdef DEBUG
  if(start > end || end>v->size || start<0)
    fatal("Illegal arguments to slice_array()\n");
#endif

  if(start==0 && v->refs==1)	/* Can we use the same array? */
  {
    v->refs++;
    return array_shrink(v,end);
  }

  a=allocate_array_no_init(end-start,0,v->array_type);
  a->type_field = v->type_field;

  if(v->array_type == T_MIXED)
  {
    assign_svalues_no_free(ITEM(a), ITEM(v)+start, end-start);
  }else{
    assign_short_svalues_no_free(SHORT_ITEM(a),
				 SHORT_ITEM(v)+start,
				 v->array_type,
				 end-start);
  }

  return a;
}

/*
 * Copy an array
 */
struct array *copy_array(struct array *v)
{
  struct array *a;

  a=allocate_array_no_init(v->size, 0, v->array_type);
  a->type_field = v->type_field;

  if(v->array_type == T_MIXED)
  {
    assign_svalues_no_free(ITEM(a), ITEM(v), v->size);
  }else{
    assign_short_svalues_no_free(SHORT_ITEM(a),
				 SHORT_ITEM(v),
				 v->array_type,
				 v->size);
  }

  return a;
}

/*
 * Clean an array from destructed objects
 */
void check_array_for_destruct(struct array *v)
{
  int e;
  INT16 types;

  types = 0;
  if(v->type_field & (BIT_OBJECT | BIT_FUNCTION))
  {
    if(v->array_type == T_MIXED)
    {
      for(e=0; e<v->size; e++)
      {
        if((ITEM(v)[e].type == T_OBJECT ||
	    (ITEM(v)[e].type == T_FUNCTION && ITEM(v)[e].subtype!=-1)) &&
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
    }else{
      for(e=0; e<v->size; e++)
      {
	if(!SHORT_ITEM(v)[e].object)
	{
	  types |= BIT_INT;
	}
	else if(!SHORT_ITEM(v)[e].object->prog)
	{
	  free_short_svalue(SHORT_ITEM(v)+e,v->array_type);
	  SHORT_ITEM(v)[e].refs=0;
	  types |= BIT_INT;
	}
      }
    }
    v->type_field = types;
  }
}

/*
 * This function finds the index of any destructed object in a set
 * it could be optimized to search out the object part with a binary 
 * search lookup if the array is mixed
 */
INT32 array_find_destructed_object(struct array *v)
{
  INT32 e;
  TYPE_FIELD types;
  if(v->type_field & (BIT_OBJECT | BIT_FUNCTION))
  {
    if(v->array_type == T_MIXED)
    {
      types=0;
      for(e=0; e<v->size; e++)
      {
        if((ITEM(v)[e].type == T_OBJECT ||
	    (ITEM(v)[e].type == T_FUNCTION && ITEM(v)[e].subtype!=-1)) &&
	   (!ITEM(v)[e].u.object->prog))
	  return e;
	types |= 1<<ITEM(v)[e].type;
      }
    }else{
      types=1<<(v->array_type);
      for(e=0; e<v->size; e++)
      {
	if(SHORT_ITEM(v)[e].object)
	{
	  if(!SHORT_ITEM(v)[e].object->prog)
	    return e;
	}else{
	  types |= BIT_INT;
	}
      }
    }
    v->type_field = types;
  }
  return -1;
}


static short_cmpfun current_short_cmpfun;
static union anything *current_short_array_p;

static int internal_short_cmpfun(INT32 *a,INT32 *b)
{
  return current_short_cmpfun(current_short_array_p + *a,
			      current_short_array_p + *b);
}

static struct svalue *current_array_p;
static cmpfun current_cmpfun;

static int internal_cmpfun(INT32 *a,INT32 *b)
{
  return current_cmpfun(current_array_p + *a, current_array_p + *b);
}

INT32 *get_order(struct array *v, cmpfun fun,cmpfun_getter backfun)
{
  INT32 e, *current_order;

  if(!v->size) return 0;

  current_cmpfun = fun;
  current_order=(INT32 *)xalloc(v->size * sizeof(INT32));
  for(e=0; e<v->size; e++) current_order[e]=e;

  if(v->array_type == T_MIXED)
  {
    current_array_p = ITEM(v);
    current_cmpfun = fun;
    fsort((char *)current_order,
	  v->size,
	  sizeof(INT32),
	  (fsortfun)internal_cmpfun);
  }else{
    current_short_array_p = SHORT_ITEM(v);
    current_short_cmpfun = backfun(v->array_type);
    fsort((char *)current_order,
	  v->size,
	  sizeof(INT32),
	  (fsortfun)internal_short_cmpfun);
  }

  return current_order;
}


static int set_svalue_cmpfun(struct svalue *a, struct svalue *b)
{
  INT32 tmp;
  if(tmp=(a->type - b->type)) return tmp;
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
    return a->u.integer - b->u.integer;

  default:
    if(a->u.refs < b->u.refs) return -1;
    if(a->u.refs > b->u.refs) return 1;
    return 0;
  }
}

static int set_anything_cmpfun_int(union anything *a, union anything *b)
{
  return a->integer - b->integer;
}

static int set_anything_cmpfun_ptr(union anything *a, union anything *b)
{
  if(a->refs < b->refs) return -1;
  if(a->refs > b->refs) return 1;
  return 0;
}

static int set_anything_cmpfun_float(union anything *a, union anything *b)
{
  if(a->float_number < b->float_number) return -1;
  if(a->float_number > b->float_number) return 1;
  return 0;
}

static short_cmpfun get_set_cmpfun(TYPE_T t)
{
  switch(t)
  {
  case T_FLOAT: return set_anything_cmpfun_float;
  case T_INT: return set_anything_cmpfun_int;
  default: return set_anything_cmpfun_ptr;
  }
}

static int switch_anything_cmpfun_string(union anything *a, union anything *b)
{
  if(!a->string || !b->string)
    return set_anything_cmpfun_ptr(a,b);
  return my_strcmp(a->string, b->string);
}


static short_cmpfun get_switch_cmpfun(TYPE_T t)
{
  switch(t)
  {
  case T_INT: return set_anything_cmpfun_int;
  case T_FLOAT: return set_anything_cmpfun_float;
  case T_STRING: return switch_anything_cmpfun_string;
  default:
    error("Illegal type in switch.\n");
    return 0; /* Make apcc happy */
  }
}

static int switch_svalue_cmpfun(struct svalue *a, struct svalue *b)
{
  if(a->type != b->type) return a->type - b->type;
  switch(a->type)
  {
  case T_INT:
    return a->u.integer - b->u.integer;

  case T_FLOAT:
    if(a->u.float_number < b->u.float_number) return -1;
    if(a->u.float_number > b->u.float_number) return 1;
    return 0;

  case T_STRING:
    return my_strcmp(a->u.string, b->u.string);
    
  default:
    return set_svalue_cmpfun(a,b);
  }
}

/*
 * return an 'order' suitable for making mappings, lists other sets
 */
INT32 *get_set_order(struct array *a)
{
  return get_order(a, set_svalue_cmpfun, get_set_cmpfun);
}

/*
 * return an 'order' suitable for switches.
 */
INT32 *get_switch_order(struct array *a)
{
  return get_order(a, switch_svalue_cmpfun, get_switch_cmpfun);
}


static INT32 low_lookup(struct array *v,
			struct svalue *s,
			cmpfun fun,
			cmpfun_getter backfun)
{
  INT32 a,b,c;
  int q;
  if(v->array_type == T_MIXED)
  {
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

  }else if(s->type == v->array_type ||
	   (s->type==T_INT && v->array_type != T_FLOAT)){
    short_cmpfun fun;
    fun=backfun(v->array_type);

    a=0;
    b=v->size;
    while(b > a)
    {
      c=(a+b)/2;
      q=fun(SHORT_ITEM(v)+c,&s->u);

      if(q < 0)
	a=c+1;
      else if(q > 0)
	b=c;
      else
	return c;
    }
    if(a<v->size && fun(SHORT_ITEM(v)+a,&s->u)<0) a++;
    return ~a;

  }else{
    /* face it, it's not there */
    if((long)s->type < (long)v->array_type) return -1;
    return ~v->size;
  }
}

INT32 set_lookup(struct array *a, struct svalue *s)
{
  return low_lookup(a,s,set_svalue_cmpfun,get_set_cmpfun);
}

INT32 switch_lookup(struct array *a, struct svalue *s)
{
  return low_lookup(a,s,switch_svalue_cmpfun,get_switch_cmpfun);
}


/*
 * reorganize an array in the order specifyed by 'order'
 */
struct array *order_array(struct array *v, INT32 *order)
{
  if(v->array_type == T_MIXED)
    reorder((char *)ITEM(v),v->size,sizeof(struct svalue),order);
  else
    reorder((char *)SHORT_ITEM(v),v->size,sizeof(union anything),order);

  return v;
}


/*
 * copy and reorganize an array
 */
struct array *reorder_and_copy_array(struct array *v, INT32 *order)
{
  INT32 e;
  struct array *ret;
  ret=allocate_array_no_init(v->size, 0, v->array_type);
  ret->type_field = v->type_field;

  if(v->array_type == T_MIXED)
  {
    for(e=0;e<v->size;e++)
      assign_svalue_no_free(ITEM(ret)+e, ITEM(v)+order[e]);
  }else{
    for(e=0;e<v->size;e++)
      assign_short_svalue_no_free(SHORT_ITEM(ret)+e,
				  SHORT_ITEM(v)+order[e],
				  v->array_type);
  }

  return ret;
}

/* Maybe I should have a 'clean' flag for this computation */
void array_fix_type_field(struct array *v)
{
  int e;
  TYPE_FIELD t;

  t=0;
  switch(v->array_type)
  {
  case T_MIXED:
    for(e=0; e<v->size; e++) t |= 1 << ITEM(v)[e].type;
    break;

  case T_INT:
  case T_FLOAT:
    t=1 << v->array_type; 
   break;

  default:
    v->type_field=1 << v->array_type;
    for(e=0; e<v->size; e++)
    {
      if(! SHORT_ITEM(v)[e].refs)
      {
	t |= 1 << T_INT;
	break;
      }
    }

  }
  v->type_field = t;
}

/*
 * Replace a large array with a small one if possible
 */
struct array *compact_array(struct array *v)
{
  INT32 e;
  int type;
  struct array *ret;
  if(v->array_type != T_MIXED) return v;

  if(!v->size) return v; /* won't become smaller */

  array_fix_type_field(v);

  type=-1;
  switch(v->type_field)
  {
  case BIT_INT | BIT_STRING:  type=T_STRING; goto check_possible;
  case BIT_INT | BIT_ARRAY:   type=T_ARRAY; goto check_possible;
  case BIT_INT | BIT_MAPPING: type=T_MAPPING; goto check_possible;
  case BIT_INT | BIT_LIST:    type=T_LIST; goto check_possible;
  case BIT_INT | BIT_OBJECT:  type=T_OBJECT; goto check_possible;
  case BIT_INT | BIT_PROGRAM: type=T_PROGRAM;

  check_possible:
    for(e=0; e<v->size; e++)
      if(ITEM(v)[e].type == T_INT)
	if(ITEM(v)[e].u.integer != 0)
	  return v;

    goto do_compact;

  case BIT_INT:     type=T_INT; goto do_compact;
  case BIT_FLOAT:   type=T_FLOAT; goto do_compact;
  case BIT_STRING:  type=T_STRING; goto do_compact;
  case BIT_ARRAY:   type=T_ARRAY; goto do_compact;
  case BIT_MAPPING: type=T_MAPPING; goto do_compact;
  case BIT_LIST:    type=T_LIST; goto do_compact;
  case BIT_OBJECT:  type=T_OBJECT; goto do_compact;
  case BIT_PROGRAM: type=T_PROGRAM; goto do_compact;

  do_compact:
    ret=allocate_array_no_init(v->size, 0, type);
    for(e=0; e<v->size; e++)
      assign_to_short_svalue_no_free(SHORT_ITEM(ret)+e,
				     ITEM(v)[e].type,
				     ITEM(v)+e);
    free_array(v);
    return ret;

  default:
    return v;
  }
}

/*
 * Get a pointer to the 'union anything' specified IF it is of the specified
 * type. The 'union anything' may be changed, but not the type.
 */
union anything *low_array_get_item_ptr(struct array *a,
				       INT32 ind,
				       TYPE_T t)
{
  if(a->array_type == T_MIXED)
  {
    if(ITEM(a)[ind].type == t)
      return & (ITEM(a)[ind].u);
  }else if(a->array_type == t){
    return SHORT_ITEM(a)+ind;
  }
  return 0;
}

/*
 * Get a pointer to the 'union anything' specified IF it is of the specified
 * type. The 'union anything' may be changed, but not the type.
 * The differance between this routine and the one above is that this takes
 * the index as an svalue.
 */
union anything *array_get_item_ptr(struct array *a,
				   struct svalue *ind,
				   TYPE_T t)
{
  INT32 i;
  if(ind->type != T_INT)
    error("Index is not an integer.\n");
  i=ind->u.integer;
  if(i<0) i+=a->size;
  if(i<0 || i>=a->size) error("Index out of range.\n");
  return low_array_get_item_ptr(a,i,t);
}

/*
 * organize an array of INT32 to specify how to zip two arrays together
 * to maintain the order.
 * the first item in this array is the size of the result
 * the rest is n >= 0 for a[ n ]
 * or n < 0 for b[ ~n ]
 */
INT32 * merge(struct array *a,struct array *b,INT32 opcode)
{
  INT32 ap,bp,i,*ret,*ptr;
  
  ap=bp=0;
  if(!(a->type_field & b->type_field))
  {
    /* do smart optimizations */
    switch(opcode)
    {
    case OP_AND:
      ret=(INT32 *)xalloc(sizeof(INT32));
      *ret=0;
      return ret;

    case OP_SUB:
      ptr=ret=(INT32 *)xalloc(sizeof(INT32)*(a->size+1));
      *(ptr++)=a->size;
      for(i=0;i<a->size;i++) *(ptr++)=i;
      return ret;
    }
  }

  ptr=ret=(INT32 *)xalloc(sizeof(INT32)*(a->size + b->size + 1));
  ptr++;
  if(a->array_type == T_MIXED && b->array_type == T_MIXED)
  {
    while(ap < a->size && bp < b->size)
    {
      i=set_svalue_cmpfun(ITEM(a)+ap,ITEM(b)+bp);
      if(i < 0)
	i=opcode >> 8;
      else if(i > 0)
	i=opcode;
      else
	i=opcode >> 4;

      if(i & OP_A) *(ptr++)=ap;
      if(i & OP_B) *(ptr++)=~bp;
      if(i & OP_SKIP_A) ap++;
      if(i & OP_SKIP_B) bp++;
    }
  }else if(a->array_type == b->array_type)
  {
    short_cmpfun short_alist_cmp;
    short_alist_cmp=get_set_cmpfun(a->array_type);
    while(ap < a->size && bp < b->size)
    {
      i=short_alist_cmp(SHORT_ITEM(a)+ap,SHORT_ITEM(b)+bp);
      if(i < 0)
	i=opcode >> 8;
      else if(i > 0)
	i=opcode;
      else
	i=opcode >> 4;

      if(i & OP_A) *(ptr++)=ap;
      if(i & OP_B) *(ptr++)=~bp;
      if(i & OP_SKIP_A) ap++;
      if(i & OP_SKIP_B) bp++;
    }
  }else{
    struct svalue sa,sb;
    while(ap < a->size && bp < b->size)
    {
      if(a->array_type == T_MIXED)
      {
	sa=ITEM(a)[ap];
      }else{
	sa.u = SHORT_ITEM(a)[ap];
	if(!sa.u.refs )
	{
	  if( (sa.type=a->array_type) != T_FLOAT)  sa.type=T_INT;
	}else{
	  sa.type=a->array_type;
	}
      }

      if(b->array_type == T_MIXED)
      {
	sb=ITEM(b)[bp];
      }else{
	sb.u=SHORT_ITEM(b)[bp];
	if(!sb.u.refs)
	{
	  if( (sb.type=b->array_type) != T_FLOAT)  sb.type=T_INT;
	}else{
	  sb.type=b->array_type;
	}
      }

      i=set_svalue_cmpfun(&sa, &sb);
      if(i < 0)
	i=opcode >> 8;
      else if(i > 0)
	i=opcode;
      else
	i=opcode >> 4;

      if(i & OP_A) *(ptr++)=ap;
      if(i & OP_B) *(ptr++)=~bp;
      if(i & OP_SKIP_A) ap++;
      if(i & OP_SKIP_B) bp++;
    }
  }

  if((opcode >> 8) & OP_A) while(ap<a->size) *(ptr++)=ap++;
  if(opcode & OP_B) while(bp<b->size) *(ptr++)=~(bp++);

  *ret=(ptr-ret-1);

  return ret;
}

/*
 * This routine merges two arrays in the order specified by 'zipper'
 * zipper normally produced by merge() above
 */
struct array *array_zip(struct array *a, struct array *b,INT32 *zipper)
{
  INT32 size,e;
  struct array *ret;
  size=zipper[0];
  zipper++;

  if(a->array_type == T_MIXED && b->array_type == T_MIXED)
  {
    ret=allocate_array_no_init(size,0, T_MIXED);
    for(e=0; e<size; e++)
    {
      if(*zipper >= 0)
	assign_svalue_no_free(ITEM(ret)+e, ITEM(a)+*zipper);
      else
	assign_svalue_no_free(ITEM(ret)+e, ITEM(b)+~*zipper);
      zipper++;
    }
  }else if(a->array_type == b->array_type)
  {
    ret=allocate_array_no_init(size, 0, a->array_type);
    for(e=0; e<size; e++)
    {
      if(*zipper >= 0)
      {
	assign_short_svalue_no_free(SHORT_ITEM(ret)+e,
			    SHORT_ITEM(a)+*zipper,
			    a->array_type);
      }else{
	assign_short_svalue_no_free(SHORT_ITEM(ret)+e,
			    SHORT_ITEM(b)+~*zipper,
			    b->array_type);
      }
      zipper++;
    }
  }else{
    ret=allocate_array_no_init(size, 0, T_MIXED);
    for(e=0; e<size; e++)
    {
      if(*zipper >= 0)
      {
	if(a->array_type == T_MIXED)
	{
	  assign_svalue_no_free(ITEM(ret)+e, ITEM(a)+*zipper);
	}else{
	  assign_from_short_svalue_no_free(ITEM(ret)+e,
				   SHORT_ITEM(a)+*zipper,
				   a->array_type);
	}
      }else{
	if(b->array_type == T_MIXED)
	{
	  assign_svalue(ITEM(ret)+e, ITEM(a)+*zipper);
	}else{
	  assign_from_short_svalue_no_free(ITEM(ret)+e,
				   SHORT_ITEM(b)+~*zipper,
				   b->array_type);
	}
      }
      zipper++;
    }
  }
  ret->type_field = a->type_field | b->type_field;
  return ret;
}

struct array *add_arrays(struct svalue *argp, INT32 args)
{
  INT32 e, size;
  struct array *v;
  TYPE_T array_type;

  array_type=args ? argp[0].u.array->array_type : T_MIXED;
  for(size=e=0; e<args; e++)
  {
    check_array_for_destruct(argp[e].u.array);
    size+=argp[e].u.array->size;
    if(array_type != argp[e].u.array->array_type)
      array_type=T_MIXED;
  }

  if(args &&
     argp[0].u.array->refs==1 &&
     argp[0].u.array->array_type == array_type)
  {
    e=argp[0].u.array->size;
    v=resize_array(argp[0].u.array, size);
    argp[0].type=T_INT;
    size=e;
    e=1;
  }else{
    v=allocate_array_no_init(size, 0, array_type);
    v->type_field=0;
    e=size=0;
  }
  if(array_type == T_MIXED)
  {
    for(; e<args; e++)
    {
      v->type_field|=argp[e].u.array->type_field;
      if(argp[e].u.array->array_type == T_MIXED)
      {
	assign_svalues_no_free(ITEM(v)+size,
			     ITEM(argp[e].u.array),
			     argp[e].u.array->size);
      }else{
	assign_from_short_svalues_no_free(ITEM(v)+size,
					  SHORT_ITEM(argp[e].u.array),
					  argp[e].u.array->array_type,
					  argp[e].u.array->size);
      }
      size+=argp[e].u.array->size;
    }
  }else{
    for(;e<args;e++)
    {
      v->type_field |= argp[e].u.array->type_field;
      assign_short_svalues_no_free(SHORT_ITEM(v)+size,
				   SHORT_ITEM(argp[e].u.array),
				   argp[e].u.array->array_type,
				   argp[e].u.array->size);
      size+=argp[e].u.array->size;
    }
  }

  return v;
}

int array_equal_p(struct array *a, struct array *b, struct processing *p)
{
  struct processing curr;
  INT32 e;

  if(a == b) return 1;
  if(a->size != b->size) return 0;

  curr.pointer_a = a;
  curr.pointer_b = b;
  curr.next = p;

  for( ;p ;p=p->next)
    if(p->pointer_a == (void *)a && p->pointer_b == (void *)b)
      return 1;

  if(a->array_type == T_MIXED && b->array_type==T_MIXED)
  {
    for(e=0; e<a->size; e++)
      if(!low_is_equal(ITEM(a)+e, ITEM(b)+e, &curr))
	return 0;
  }else{
    for(e=0; e<a->size; e++)
    {
      struct svalue sa,sb;

      if(a->array_type == T_MIXED)
      {
	sa=ITEM(a)[e];
      }else{
	sa.u=SHORT_ITEM(a)[e];
	if(!sa.u.refs)
	{
	  if( (sa.type=a->array_type) != T_FLOAT)  sa.type=T_INT;
	}else{
	  sa.type=a->array_type;
	}
      }

      if(b->array_type == T_MIXED)
      {
	sb=ITEM(b)[e];
      }else{
	sb.u=SHORT_ITEM(b)[e];
	if(!sb.u.refs)
	{
	  if( (sb.type=b->array_type) != T_FLOAT)  sb.type=T_INT;
	}else{
	  sb.type=b->array_type;
	}
      }
      
      if(!low_is_equal(&sa, &sb, &curr))
	return 0;
    }
  }
  return 1;
}

static INT32 *ordera=0, *orderb=0;
/*
 * this is used to rearrange the zipper so that the order is retained
 * as it was before (check merge_array_with_order below)
 */
static int array_merge_fun(INT32 *a, INT32 *b)
{
  if(*a<0)
  {
    if(*b<0)
    {
      return orderb[~*a] - orderb[~*b];
    }else{
      return -1;
    }
  }else{
    if(*b<0)
    {
      return 1;
    }else{
      return ordera[*a] - ordera[*b];
    }
  }
}

/*
 * merge two arrays and retain their order, this is done by arranging them
 * into ordered sets, merging them as sets and then rearranging the zipper
 * before zipping the sets together. 
 */
struct array *merge_array_with_order(struct array *a, struct array *b,INT32 op)
{
  INT32 *zipper;
  struct array *tmpa,*tmpb,*ret;

  if(ordera) { free((char *)ordera); ordera=0; }
  if(orderb) { free((char *)orderb); orderb=0; }

  ordera=get_set_order(a);
  tmpa=reorder_and_copy_array(a,ordera);

  orderb=get_set_order(b);
  tmpb=reorder_and_copy_array(b,orderb);

  zipper=merge(tmpa,tmpb,op);

  fsort((char *)(zipper+1),*zipper,sizeof(INT32),(fsortfun)array_merge_fun);
  free((char *)ordera);
  free((char *)orderb);
  orderb=ordera=0;
  ret=array_zip(tmpa,tmpb,zipper);
  free_array(tmpa);
  free_array(tmpb);
  free((char *)zipper);
  return ret;
}


/* merge two arrays without paying attention to the order
 * the elements has presently
 */
struct array *merge_array_without_order(struct array *a,
					struct array *b,
					INT32 op)
{
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
}

/* subtract an array from another */
struct array *subtract_arrays(struct array *a, struct array *b)
{
  if(a->type_field & b->type_field)
  {
    return merge_array_with_order(a, b, OP_SUB);
  }else{
    if(a->refs == 1)
    {
      a->refs++;
      return a;
    }
    return slice_array(a,0,a->size);
  }
}

/* and two arrays */
struct array *and_arrays(struct array *a, struct array *b)
{
  if(a->type_field & b->type_field)
  {
    return merge_array_without_order(a, b, OP_AND);
  }else{
    return allocate_array_no_init(0,0,T_MIXED);
  }
}

int check_that_array_is_constant(struct array *a)
{
  array_fix_type_field(a);
  if(a->type_field & ((1 << T_FUNCTION) | (1 << T_OBJECT)))
    return 0;
  return 1;
}

node *make_node_from_array(struct array *a)
{
  struct svalue s;
  char *str;
  INT32 e;

  array_fix_type_field(a);
  if(a->type_field == (1 << T_INT))
  {
    if(a->array_type == T_MIXED)
    {
      for(e=0; e<a->size; e++)
	if(ITEM(a)[e].u.integer != 0)
	  break;
      if(e == a->size)
      {
	return mkefuncallnode("allocate",
			      mknode(F_ARG_LIST,
				     mkintnode(a->size),
				     mkstrnode(make_shared_string("mixed"))
				     ));
      }
    }else{
      e=a->size;
      switch(a->array_type)
      {
      case T_INT:
	str="int";
	for(e=0; e<a->size; e++)
	  if(SHORT_ITEM(a)[e].integer != 0)
	    break;
	break;
      case T_FLOAT:    str="float"; break;
      case T_STRING:   str="string"; break;
      case T_ARRAY:    str="array"; break;
      case T_LIST:     str="list"; break;
      case T_MAPPING:  str="mapping"; break;
      case T_OBJECT:   str="object"; break;
      case T_FUNCTION: str="function"; break;
      case T_PROGRAM:  str="program"; break;
      default:         str="mixed";
      }
      if(e==a->size)
      {
	return mkefuncallnode("allocate",
			      mknode(F_ARG_LIST,
				     mkintnode(a->size),
				     mkstrnode(make_shared_string(str))
				     ));
      }
    }
  }
  if(check_that_array_is_constant(a))
  {
    s.type=T_ARRAY;
    s.subtype=0;
    s.u.array=a;
    return mkconstantsvaluenode(&s);
  }else{
    node *ret=0;
    if(a->array_type == T_MIXED)
    {
      for(e=0; e<a->size; e++)
	ret=mknode(F_ARG_LIST,ret,mksvaluenode(ITEM(a)+e));
    }else{
      s.type=a->array_type;
      s.subtype=0;
      for(e=0; e<a->size; e++)
      {
	s.u=SHORT_ITEM(a)[e];
	if(s.u.refs)
	{
	  ret=mknode(F_ARG_LIST,ret,mksvaluenode(&s));
	}else{
	  ret=mknode(F_ARG_LIST,ret,mkintnode(0));
	}
      }
    }
    return mkefuncallnode("aggregate",ret);
  }
}

void push_array_items(struct array *a)
{
  if(sp + a->size >= &evaluator_stack[EVALUATOR_STACK_SIZE])
    error("Array does not fit on stack.\n");
  check_array_for_destruct(a);
  if(a->array_type == T_MIXED)
  {
    if(a->refs == 1)
    {
      MEMCPY(sp,ITEM(a),sizeof(struct svalue)*a->size);
      sp += a->size;
      a->size=0;
      free_array(a);
      return;
    }else{
      assign_svalues_no_free(sp, ITEM(a), a->size);
    }
  }else{
    assign_from_short_svalues_no_free(sp, SHORT_ITEM(a), a->array_type, a->size);
  }
  sp += a->size;
  free_array(a);
}

void describe_array_low(struct array *a, struct processing *p, int indent)
{
  INT32 e,d;
  indent += 2;
  if(a->array_type == T_MIXED)
  {
    for(e=0; e<a->size; e++)
    {
      if(e) my_strcat(",\n");
      for(d=0; d<indent; d++) my_putchar(' ');
      describe_svalue(ITEM(a)+e,indent,p);
    }
  }else{
    struct svalue s;
    for(e=0; e<a->size; e++)
    {
      if(e) my_strcat(",\n");
      for(d=0; d<indent; d++) my_putchar(' ');
      if(SHORT_ITEM(a)[e].refs)
      {
	s.type=a->array_type;
	s.u=SHORT_ITEM(a)[e];
      }else{
	s.type=T_INT;
	s.subtype=NUMBER_NUMBER;
	s.u.integer=0;
      }
      describe_svalue(&s, indent, p);
    }
  }
}

void simple_describe_array(struct array *a)
{
  char *s;
  init_buf();
  describe_array_low(a,0,0);
  s=simple_free_buf();
  fprintf(stderr,"({\n%s\n})\n",s);
  free(s);
}

void describe_index(struct array *a,
		    int e,
		    struct processing *p,
		    int indent)
{
  if(a->array_type == T_MIXED)
  {
    describe_svalue(ITEM(a)+e, indent, p);
  }else{
    struct svalue s;
    if(SHORT_ITEM(a)[e].refs)
    {
      s.type=a->array_type;
      s.u=SHORT_ITEM(a)[e];
    }else{
      s.type=T_INT;
      s.subtype=NUMBER_NUMBER;
      s.u.integer=0;
    }
    describe_svalue(&s, indent, p);
  }
}


void describe_array(struct array *a,struct processing *p,int indent)
{
  struct processing doing;
  INT32 e;
  char buf[40];
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
  
  sprintf(buf,"({ /* %ld elements */\n",(long)a->size);
  my_strcat(buf);
  describe_array_low(a,&doing,indent);
  my_putchar('\n');
  for(e=2; e<indent; e++) my_putchar(' ');
  my_strcat("})");
}

struct array *aggregate_array(INT32 args, TYPE_T type)
{
  struct array *a;

  a=allocate_array_no_init(args,0,type);
  if(type == T_MIXED)
  {
    MEMCPY((char *)ITEM(a),(char *)(sp-args),args*sizeof(struct svalue));
    a->type_field=BIT_MIXED;
    sp-=args;
  }else{
    struct svalue *save_sp;
    save_sp=sp;
    while(--args >= 0)
    {
      sp--;
      if(sp->type == type)
      {
	SHORT_ITEM(a)[args].refs = sp->u.refs;
      }else if(IS_ZERO(sp)){
	SHORT_ITEM(a)[args].refs = 0;
      }else{
	sp=save_sp;
	array_free_no_free(a);
	error("Bad type when constructing array.\n");
      }
    }
  }
  return a;
}

struct array *explode(struct lpc_string *str,
		       struct lpc_string *del)
{
  INT32 e,d;
  struct array *ret;
  char *s, *end, *tmp;

  if(!del->len)
  {
    ret=allocate_array_no_init(str->len,0,T_STRING);
    ret->type_field |= 1<<T_STRING;
    for(e=0;e<str->len;e++)
      SHORT_ITEM(ret)[e].string=make_shared_binary_string(str->str+e,1);
  }else{

    s=str->str;
    end=s+str->len;
    e=0;

    while((s=MEMMEM(del->str, del->len, s, end-s)))
    {
      s+=del->len;
      e++;
    }

    ret=allocate_array_no_init(e+1,0,T_STRING);
    ret->type_field |= 1<<T_STRING;

    s=str->str;
    for(d=0;d<e;d++)
    {
      tmp=MEMMEM((char *)(del->str), del->len, (char *)s, end-s);
      SHORT_ITEM(ret)[d].string=make_shared_binary_string(s,tmp-s);
      s=tmp+del->len;
    }
    SHORT_ITEM(ret)[d].string=make_shared_binary_string(s,end-s);
  }
  return ret;
}

struct lpc_string *implode(struct array *a,struct lpc_string *del)
{
  INT32 len,e, inited;
  char *r;
  struct lpc_string *ret,*tmp;

  len=0;
  if(a->array_type==T_STRING)
  {
    for(e=0;e<a->size;e++)
      if(SHORT_ITEM(a)[e].string)
	len+=SHORT_ITEM(a)[e].string->len + del->len;
    if(len) len-=del->len;

    ret=begin_shared_string(len);
    r=ret->str;
    inited=0;
    for(e=0;e<a->size;e++)
    {
      if(SHORT_ITEM(a)[e].string)
      {
	if(inited)
	{
	  MEMCPY(r,del->str,del->len);
	  r+=del->len;
	}
	inited=1;
	tmp=SHORT_ITEM(a)[e].string;
	MEMCPY(r,tmp->str,tmp->len);
	r+=tmp->len;
	len++;
      }
    }
    return end_shared_string(ret);
  }

  if(a->array_type==T_MIXED)
  {
    for(e=0;e<a->size;e++)
      if(ITEM(a)[e].type==T_STRING)
	len+=ITEM(a)[e].u.string->len + del->len;
    if(len) len-=del->len;

    ret=begin_shared_string(len);
    r=ret->str;
    inited=0;
    for(e=0;e<a->size;e++)
    {
      if(ITEM(a)[e].type==T_STRING)
      {
	if(inited)
	{
	  MEMCPY(r,del->str,del->len);
	  r+=del->len;
	}
	inited=1;
	tmp=ITEM(a)[e].u.string;
	MEMCPY(r,tmp->str,tmp->len);
	r+=tmp->len;
	len++;
      }
    }
    return end_shared_string(ret);
  }

  return make_shared_string("");
}

struct array *copy_array_recursively(struct array *a,struct processing *p)
{
  struct processing doing;
  struct array *ret;

  doing.next=p;
  doing.pointer_a=(void *)a;
  for(;p;p=p->next)
  {
    if(p->pointer_a == (void *)a)
    {
      ret=(struct array *)p->pointer_b;
      ret->refs++;
      return ret;
    }
  }

  ret=allocate_array_no_init(a->size,0,a->array_type);
  doing.pointer_b=(void *)ret;
  if(a->array_type == T_MIXED)
  {
    copy_svalues_recursively_no_free(ITEM(ret),ITEM(a),a->size,&doing);
  }else{
    copy_short_svalues_recursively_no_free(SHORT_ITEM(ret),
					   SHORT_ITEM(a),
					   a->array_type,
					   a->size,&doing);
  }
  return ret;
}

void apply_array(struct array *a, INT32 args)
{
  struct svalue *argp;
  INT32 e;
  struct array *ret;
  argp=sp-args;
  if(a->array_type == T_MIXED)
  {
    for(e=0;e<a->size;e++)
    {
      assign_svalues_no_free(sp,argp,args);
      sp+=args;
      apply_svalue(ITEM(a)+e,args);
    }
  }else{
    for(e=0;e<a->size;e++)
    {
      array_index_no_free(sp++,a,e);
      assign_svalues_no_free(sp,argp,args);
      sp+=args;
      f_call_function(args+1);
    }
  }
  ret=aggregate_array(a->size,T_MIXED);
  pop_n_elems(args);
  push_array(ret);
}

struct array *reverse_array(struct array *a)
{
  INT32 e;
  struct array *ret;
  ret=allocate_array_no_init(a->size,0,a->array_type);
  if(a->array_type == T_MIXED)
  {
    for(e=0;e<a->size;e++)
      assign_svalue_no_free(ITEM(ret)+e,ITEM(a)+a->size+~e);
  }else{
    for(e=0;e<a->size;e++)
      assign_short_svalue_no_free(SHORT_ITEM(ret)+e,
				  SHORT_ITEM(a)+a->size+~e,
				  a->array_type);
  }
  return ret;
}

void array_replace(struct array *a,
		   struct svalue *from,
		   struct svalue *to)
{
  INT32 i = -1;

  while((i=array_search(a,from,i+1)) >= 0) array_set_index(a,i,to);
}

#ifdef DEBUG
void check_array(struct array *a, int pass)
{
  INT32 e;
  if(pass)
  {
    e=checked((void *)a,0);
    if(e!=a->refs)
    {
      simple_describe_array(a);
      fatal("Above array has wrong number of references. (%ld != %ld)\n",
	    (long)e,(long)a->refs);
    }
    return;
  }

  if(a->next->prev != a)
    fatal("Array check: a->next->prev != a\n");

  if(a->size > a->malloced_size)
    fatal("Array is larger than malloced block!\n");

  if(a->refs <=0 )
    fatal("Array has zero refs.\n");

  if(a->array_type == T_MIXED)
  {
    for(e=0;e<a->size;e++)
    {
      if(! ( (1 << ITEM(a)[e].type) & (a->type_field) ))
	fatal("Type field lies.\n");

      check_svalue(ITEM(a)+e);
    }
  }
  else if(a->array_type <= MAX_TYPE)
  {
    if(a->type_field & ~(BIT_INT | (1<<a->array_type)))
      fatal("Type field in short array lies!\n");

    for(e=0;e<a->size;e++)
      check_short_svalue(SHORT_ITEM(a)+e,a->array_type);
  }
  else
  {
    fatal("Array type out of range.\n");
  }
}

void check_all_arrays(int pass)
{
  struct array *a;

  a=&empty_array;
  do
  {
    check_array(a, pass);

    a=a->next;
    if(!a)
      fatal("Null pointer in array list.\n");
  } while (a != & empty_array);

  if(!pass)
  {
    checked((void *)&empty_array,1);
  }
}
#endif

