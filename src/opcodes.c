/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: opcodes.c,v 1.132 2005/08/08 09:44:06 jonasw Exp $
*/

#include "global.h"
#include <errno.h>
#include <math.h>
#include <ctype.h>
#include "interpret.h"
#include "svalue.h"
#include "array.h"
#include "stralloc.h"
#include "mapping.h"
#include "multiset.h"
#include "opcodes.h"
#include "object.h"
#include "pike_error.h"
#include "pike_types.h"
#include "pike_memory.h"
#include "fd_control.h"
#include "cyclic.h"
#include "builtin_functions.h"
#include "module_support.h"
#include "security.h"
#include "bignum.h"
#include "operators.h"

#define sp Pike_sp

RCSID("$Id: opcodes.c,v 1.132 2005/08/08 09:44:06 jonasw Exp $");

void index_no_free(struct svalue *to,struct svalue *what,struct svalue *ind)
{
  INT32 i;

#ifdef PIKE_SECURITY
  if(what->type <= MAX_COMPLEX)
    if(!CHECK_DATA_SECURITY(what->u.array, SECURITY_BIT_INDEX))
      Pike_error("Index permission denied.\n");
#endif

  switch(what->type)
  {
#ifdef AUTO_BIGNUM
  case T_INT:
    {
      INT_TYPE val = what->u.integer;

      convert_svalue_to_bignum(what);
      index_no_free(to, what, ind);
      if(IS_UNDEFINED(to)) {
	if (val) {
	  if (ind->type == T_STRING)
	    Pike_error("Indexing the integer %"PRINTPIKEINT"d "
		       "with unknown method \"%s\".\n", val, ind->u.string->str);
	  else
	    Pike_error("Indexing the integer %"PRINTPIKEINT"d "
		       "with an unknown method.\n", val);
	} else {
          if(ind->type == T_STRING)
            Pike_error("Indexing the NULL value with \"%s\".\n", ind->u.string->str);
          else
            Pike_error("Indexing the NULL value.\n");
       }
      }
    }
    break;
#endif /* AUTO_BIGNUM */
    
  case T_ARRAY:
    simple_array_index_no_free(to,what->u.array,ind);
    break;

  case T_MAPPING:
    mapping_index_no_free(to,what->u.mapping,ind);
    break;

  case T_OBJECT:
    object_index_no_free(to, what->u.object, ind);
    break;

  case T_MULTISET:
    i=multiset_member(what->u.multiset, ind);
    to->type=T_INT;
    to->subtype=i ? 0 : NUMBER_UNDEFINED;
    to->u.integer=i;
    break;

  case T_STRING:
    if(ind->type==T_INT)
    {
      i=ind->u.integer;
      if(i<0)
	i+=what->u.string->len;
      if(i<0 || i>=what->u.string->len)
      {
	if(what->u.string->len == 0)
	  Pike_error("Attempt to index the empty string with %d.\n", i);
	else
	  Pike_error("Index %d is out of string range 0 - %ld.\n",
		i, PTRDIFF_T_TO_LONG(what->u.string->len - 1));
      } else
	i=index_shared_string(what->u.string,i);
      to->type=T_INT;
      to->subtype=NUMBER_NUMBER;
      to->u.integer=i;
      break;
    }else{
      Pike_error("Index is not an integer.\n");
    }

  case T_PROGRAM:
    program_index_no_free(to, what->u.program, ind);
    break;

  case T_FUNCTION:
    {
      struct program *p = program_from_svalue(what);
      if (p) {
	program_index_no_free(to, p, ind);
	break;
      }
    }
    /* FALL THROUGH */

  default:
    Pike_error("Indexing a basic type.\n");
  }
}

void o_index(void)
{
  struct svalue s;
  index_no_free(&s,sp-2,sp-1);
  pop_n_elems(2);
  *sp=s;
  dmalloc_touch_svalue(sp);
  sp++;
}

/* Special case for casting to int. */
void o_cast_to_int(void)
{
  switch(sp[-1].type)
  {
  case T_OBJECT:
    if(!sp[-1].u.object->prog) {
      /* Casting a destructed object should be like casting a zero. */
      pop_stack();
      push_int (0);
    }

    else {
      struct pike_string *s;
      MAKE_CONSTANT_SHARED_STRING(s, "int");
      push_string(s);
      if(FIND_LFUN(sp[-2].u.object->prog,LFUN_CAST) == -1)
	Pike_error("No cast method in object.\n");
      apply_lfun(sp[-2].u.object, LFUN_CAST, 1);
      free_svalue(sp-2);
      move_svalue (sp - 2, sp - 1);
      sp--;

      if(sp[-1].type != PIKE_T_INT)
      {
	if(sp[-1].type == T_OBJECT && sp[-1].u.object->prog)
	{
	  int f=FIND_LFUN(sp[-1].u.object->prog, LFUN__IS_TYPE);
	  if( f != -1)
	  {
	    struct pike_string *s;
	    MAKE_CONSTANT_SHARED_STRING(s, "int");
	    push_string(s);
	    apply_low(sp[-2].u.object, f, 1);
	    f=!UNSAFE_IS_ZERO(sp-1);
	    pop_stack();
	    if(f) return;
	  }
	}
	Pike_error("Cast failed, wanted int, got %s\n",
		   get_name_of_type(sp[-1].type));
      }
    }

    break;

  case T_FLOAT:
    {
      int i=DO_NOT_WARN((int)(sp[-1].u.float_number));
#ifdef AUTO_BIGNUM
      if((i < 0 ? -i : i) < floor(fabs(sp[-1].u.float_number)))
      {
	/* Note: This includes the case when i = 0x80000000, i.e.
	   the absolute value is not computable. */
	convert_stack_top_to_bignum();
	return;   /* FIXME: OK to return? Cast tests below indicates
		     we have to do this, at least for now... /Noring */
	/* Yes, it is ok to return, it is actually an optimization :)
	 * /Hubbe
	 */
      }
      else
#endif /* AUTO_BIGNUM */
      {
	sp[-1].type=T_INT;
	sp[-1].u.integer=i;
      }
    }
    break;
      
  case T_STRING:
    /* This can be here independently of AUTO_BIGNUM. Besides,
       we really want to reduce the number of number parsers
       around here. :) /Noring */
#ifdef AUTO_BIGNUM

    /* The generic function is rather slow, so I added this
     * code for benchmark purposes. :-) /per
     */
    if( sp[-1].u.string->len < 10 &&
	!sp[-1].u.string->size_shift )
    {
      int i=atoi(sp[-1].u.string->str);
      free_string(sp[-1].u.string);
      sp[-1].type=T_INT;
      sp[-1].u.integer=i;
    }
    else
      convert_stack_top_string_to_inumber(10);
    return;   /* FIXME: OK to return? Cast tests below indicates
		 we have to do this, at least for now... /Noring */
    /* Yes, it is ok to return, it is actually an optimization :)
     * /Hubbe
     */
#else
    {
      int i=STRTOL(sp[-1].u.string->str,0,10);
      free_string(sp[-1].u.string);
      sp[-1].type=T_INT;
      sp[-1].u.integer=i;
    }
#endif /* AUTO_BIGNUM */
    break;

  case PIKE_T_INT:
    break;
	    
  default:
    Pike_error("Cannot cast %s to int.\n", get_name_of_type(sp[-1].type));
  }
}

void o_cast_to_string(void)
{
  char buf[200];
  switch(sp[-1].type)
  {
  case PIKE_T_STRING:
    return;

  case T_OBJECT:
    if(!sp[-1].u.object->prog) {
      /* Casting a destructed object should be like casting a zero. */
      pop_stack();
      push_int (0);
    }

    else {
      struct pike_string *s;
      MAKE_CONSTANT_SHARED_STRING(s, "string");
      push_string(s);
      if(FIND_LFUN(sp[-2].u.object->prog,LFUN_CAST) == -1)
	Pike_error("No cast method in object.\n");
      apply_lfun(sp[-2].u.object, LFUN_CAST, 1);
      free_svalue(sp-2);
      sp[-2]=sp[-1];
      sp--;
      dmalloc_touch_svalue(sp);

      if(sp[-1].type != PIKE_T_STRING)
      {
	if(sp[-1].type == T_OBJECT && sp[-1].u.object->prog)
	{
	  int f=FIND_LFUN(sp[-1].u.object->prog, LFUN__IS_TYPE);
	  if( f != -1)
	  {
	    struct pike_string *s;
	    MAKE_CONSTANT_SHARED_STRING(s, "string");
	    push_string(s);
	    apply_low(sp[-2].u.object, f, 1);
	    f=!UNSAFE_IS_ZERO(sp-1);
	    pop_stack();
	    if(f) return;
	  }
	}
	Pike_error("Cast failed, wanted string, got %s\n",
		   get_name_of_type(sp[-1].type));
      }
      return;
    }

    /* Fall through. */

  case T_INT:
    sprintf(buf, "%ld", (long)sp[-1].u.integer);
    break;

  case T_ARRAY:
    {
      int i;
      struct array *a = sp[-1].u.array;
      struct pike_string *s;
      int shift = 0;

      for(i = a->size; i--; ) {
	unsigned INT32 val;
	if (a->item[i].type != T_INT) {
	  Pike_error("cast: Item %d is not an integer.\n", i);
	}
	val = (unsigned INT32)a->item[i].u.integer;
	if (val > 0xff) {
	  shift = 1;
	  if (val > 0xffff) {
	    shift = 2;
	    while(i--)
	      if (a->item[i].type != T_INT)
		Pike_error("cast: Item %d is not an integer.\n", i);
	    break;
	  }
	  while(i--) {
	    if (a->item[i].type != T_INT) {
	      Pike_error("cast: Item %d is not an integer.\n", i);
	    }
	    val = (unsigned INT32)a->item[i].u.integer;
	    if (val > 0xffff) {
	      shift = 2;
	      while(i--)
		if (a->item[i].type != T_INT)
		  Pike_error("cast: Item %d is not an integer.\n", i);
	      break;
	    }
	  }
	  break;
	}
      }
      s = begin_wide_shared_string(a->size, shift);
      switch(shift) {
      case 0:
	for(i = a->size; i--; ) {
	  s->str[i] = a->item[i].u.integer;
	}
	break;
      case 1:
	{
	  p_wchar1 *str1 = STR1(s);
	  for(i = a->size; i--; ) {
	    str1[i] = a->item[i].u.integer;
	  }
	}
	break;
      case 2:
	{
	  p_wchar2 *str2 = STR2(s);
	  for(i = a->size; i--; ) {
	    str2[i] = a->item[i].u.integer;
	  }
	}
	break;
      default:
	free_string(end_shared_string(s));
	Pike_fatal("cast: Bad shift: %d.\n", shift);
	break;
      }
      s = end_shared_string(s);
      pop_stack();
      push_string(s);
    }
    return;

  case T_FLOAT:
    sprintf(buf, "%f", (double)sp[-1].u.float_number);
    break;

  default:
    Pike_error("Cannot cast %s to string.\n", get_name_of_type(sp[-1].type));
  }
	
  sp[-1].type = PIKE_T_STRING;
  sp[-1].u.string = make_shared_string(buf);
}

void o_cast(struct pike_type *type, INT32 run_time_type)
{
  if(run_time_type != sp[-1].type)
  {
    if(run_time_type == T_MIXED)
      return;

    if (sp[-1].type == T_OBJECT && !sp[-1].u.object->prog) {
      /* Casting a destructed object should be like casting a zero. */
      pop_stack();
      push_int (0);
    }

    if(sp[-1].type == T_OBJECT)
    {
      struct pike_string *s;
      s=describe_type(type);
      push_string(s);
      if(FIND_LFUN(sp[-2].u.object->prog,LFUN_CAST) == -1)
	Pike_error("No cast method in object.\n");
      apply_lfun(sp[-2].u.object, LFUN_CAST, 1);
      free_svalue(sp-2);
      sp[-2]=sp[-1];
      sp--;
      dmalloc_touch_svalue(sp);
    }else

    switch(run_time_type)
    {
      default:
	Pike_error("Cannot perform cast to that type.\n");
	
      case T_MIXED:
	return;

      case T_MULTISET:
	switch(sp[-1].type)
	{
	  case T_ARRAY:
	  {
	    extern void f_mkmultiset(INT32);
	    f_mkmultiset(1);
	    break;
	  }

	  default:
	    Pike_error("Cannot cast %s to multiset.\n",get_name_of_type(sp[-1].type));
	}
	break;
	
      case T_MAPPING:
	switch(sp[-1].type)
	{
	  case T_ARRAY:
	  {
	     struct array *a=sp[-1].u.array;
	     struct array *b;
	     struct mapping *m;
	     INT32 i;
	     m=allocate_mapping(a->size); /* MAP_SLOTS(a->size) */
	     push_mapping(m);
	     for (i=0; i<a->size; i++)
	     {
		if (ITEM(a)[i].type!=T_ARRAY)
		   Pike_error("Cast array to mapping: "
			 "element %d is not an array\n", i);
		b=ITEM(a)[i].u.array;
		if (b->size!=2)
		   Pike_error("Cast array to mapping: "
			 "element %d is not an array of size 2\n", i);
		mapping_insert(m,ITEM(b)+0,ITEM(b)+1);
	     }
	     stack_swap();
	     pop_n_elems(1);
	     break;
	  }

	  default:
	    Pike_error("Cannot cast %s to mapping.\n",get_name_of_type(sp[-1].type));
	}
	break;
	
      case T_ARRAY:
	switch(sp[-1].type)
	{
	  case T_MAPPING:
	  {
	    struct array *a=mapping_to_array(sp[-1].u.mapping);
	    pop_stack();
	    push_array(a);
	    break;
	  }

	  case T_STRING:
	    f_values(1);
	    break;

	  case T_MULTISET:
	    f_indices(1);
	    break;

	  default:
	    Pike_error("Cannot cast %s to array.\n",get_name_of_type(sp[-1].type));
	      
	}
	break;
	
    case T_INT:
      o_cast_to_int();
      return;
	
    case T_STRING:
      o_cast_to_string();
      return;

      case T_FLOAT:
      {
	FLOAT_TYPE f = 0.0;
	
	switch(sp[-1].type)
	{
	  case T_INT:
	    f=(FLOAT_TYPE)(sp[-1].u.integer);
	    break;
	    
	  case T_STRING:
	    f =
	      (FLOAT_TYPE)STRTOD_PCHARP(MKPCHARP(sp[-1].u.string->str,
						 sp[-1].u.string->size_shift),
					0);
	    free_string(sp[-1].u.string);
	    break;
	    
	  default:
	    Pike_error("Cannot cast %s to float.\n",get_name_of_type(sp[-1].type));
	}
	
	sp[-1].type=T_FLOAT;
	sp[-1].u.float_number=f;
	break;
      }
      
      case T_OBJECT:
	switch(sp[-1].type)
	{
	  case T_STRING: {
	    struct pike_string *file;
	    INT32 lineno;
	    if(Pike_fp->pc &&
	       (file = low_get_line(Pike_fp->pc, Pike_fp->context.prog, &lineno))) {
	      push_string(file);
	    }else{
	      push_int(0);
	    }
	    APPLY_MASTER("cast_to_object",2);
	    return;
	  }
	    
	  case T_FUNCTION:
	    if (Pike_sp[-1].subtype == FUNCTION_BUILTIN) {
	      Pike_error("Cannot cast builtin functions to object.\n");
	    } else {
	      Pike_sp[-1].type = T_OBJECT;
	    }
	    break;

	  default:
	    Pike_error("Cannot cast %s to object.\n",get_name_of_type(sp[-1].type));
	}
	break;
	
      case T_PROGRAM:
      switch(sp[-1].type)
      {
	case T_STRING: {
	  struct pike_string *file;
	  INT32 lineno;
	  if(Pike_fp->pc &&
	     (file = low_get_line(Pike_fp->pc, Pike_fp->context.prog, &lineno))) {
	    push_string(file);
	  }else{
	    push_int(0);
	  }
	  APPLY_MASTER("cast_to_program",2);
	  return;
	}
	  
	case T_FUNCTION:
	{
	  struct program *p=program_from_function(sp-1);
	  if(p)
	  {
	    add_ref(p);
	    pop_stack();
	    push_program(p);
	  }else{
	    pop_stack();
	    push_int(0);
	  }
	}
	return;

	default:
	  Pike_error("Cannot cast %s to a program.\n",get_name_of_type(sp[-1].type));
      }
    }
  }

  if(run_time_type != sp[-1].type)
  {
    if(sp[-1].type == T_OBJECT && sp[-1].u.object->prog)
    {
      int f=FIND_LFUN(sp[-1].u.object->prog, LFUN__IS_TYPE);
      if( f != -1)
      {
	push_text(get_name_of_type(run_time_type));
	apply_low(sp[-2].u.object, f, 1);
	f=!UNSAFE_IS_ZERO(sp-1);
	pop_stack();
	if(f) goto emulated_type_ok;
      }
    }
    Pike_error("Cast failed, wanted %s, got %s\n",
	  get_name_of_type(run_time_type),
	  get_name_of_type(sp[-1].type));
  }

  emulated_type_ok:

  if (!type) return;

  switch(run_time_type)
  {
    case T_ARRAY:
    {
      struct pike_type *itype;
      INT32 run_time_itype;

      push_type_value(itype = index_type(type, int_type_string, 0));
      run_time_itype = compile_type_to_runtime_type(itype);

      if(run_time_itype != T_MIXED)
      {
	struct array *a;
	struct array *tmp=sp[-2].u.array;
	DECLARE_CYCLIC();
	
	if((a=(struct array *)BEGIN_CYCLIC(tmp,0)))
	{
	  ref_push_array(a);
	}else{
	  INT32 e;
#ifdef PIKE_DEBUG
	  struct svalue *save_sp=sp+1;
#endif
	  push_array(a=allocate_array(tmp->size));
	  SET_CYCLIC_RET(a);
	  
	  for(e=0;e<a->size;e++)
	  {
	    push_svalue(tmp->item+e);
	    o_cast(itype, run_time_itype);
	    array_set_index(a,e,sp-1);
	    pop_stack();
	  }
#ifdef PIKE_DEBUG
	  if(save_sp!=sp)
	    Pike_fatal("o_cast left stack droppings.\n");
#endif
	}
	END_CYCLIC();
	assign_svalue(sp-3,sp-1);
	pop_stack();
      }
      pop_stack();
    }
    break;

    case T_MULTISET:
    {
      struct pike_type *itype;
      INT32 run_time_itype;

      push_type_value(itype = key_type(type, 0));
      run_time_itype = compile_type_to_runtime_type(itype);

      if(run_time_itype != T_MIXED)
      {
	struct multiset *m;
#ifdef PIKE_NEW_MULTISETS
	struct multiset *tmp=sp[-2].u.multiset;
#else
	struct array *tmp=sp[-2].u.multiset->ind;
#endif
	DECLARE_CYCLIC();
	
	if((m=(struct multiset *)BEGIN_CYCLIC(tmp,0)))
	{
	  ref_push_multiset(m);
	}else{
#ifdef PIKE_DEBUG
	  struct svalue *save_sp=sp+1;
#endif

#ifdef PIKE_NEW_MULTISETS
	  ptrdiff_t nodepos;
	  if (multiset_indval (tmp))
	    Pike_error ("FIXME: Casting not implemented for multisets with values.\n");
	  push_multiset (m = allocate_multiset (multiset_sizeof (tmp),
						multiset_get_flags (tmp),
						multiset_get_cmp_less (tmp)));

	  SET_CYCLIC_RET(m);

	  if ((nodepos = multiset_first (tmp)) >= 0) {
	    ONERROR uwp;
	    SET_ONERROR (uwp, do_sub_msnode_ref, tmp);
	    do {
	      push_multiset_index (tmp, nodepos);
	      o_cast(itype, run_time_itype);
	      multiset_insert_2 (m, sp - 1, NULL, 0);
	      pop_stack();
	    } while ((nodepos = multiset_next (tmp, nodepos)) >= 0);
	    UNSET_ONERROR (uwp);
	    sub_msnode_ref (tmp);
	  }

#else  /* PIKE_NEW_MULTISETS */
	  INT32 e;
	  struct array *a;
	  push_multiset(m=allocate_multiset(a=allocate_array(tmp->size)));
	  
	  SET_CYCLIC_RET(m);
	  
	  for(e=0;e<a->size;e++)
	  {
	    push_svalue(tmp->item+e);
	    o_cast(itype, run_time_itype);
	    array_set_index(a,e,sp-1);
	    pop_stack();
	  }
	  order_multiset(m);
#endif

#ifdef PIKE_DEBUG
	  if(save_sp!=sp)
	    Pike_fatal("o_cast left stack droppings.\n");
#endif
	}
	END_CYCLIC();
	assign_svalue(sp-3,sp-1);
	pop_stack();
      }
      pop_stack();
    }
    break;

    case T_MAPPING:
    {
      struct pike_type *itype, *vtype;
      INT32 run_time_itype;
      INT32 run_time_vtype;

      push_type_value(itype = key_type(type, 0));
      run_time_itype = compile_type_to_runtime_type(itype);

      push_type_value(vtype = index_type(type, mixed_type_string, 0));
      run_time_vtype = compile_type_to_runtime_type(vtype);

      if(run_time_itype != T_MIXED ||
	 run_time_vtype != T_MIXED)
      {
	struct mapping *m;
	struct mapping *tmp=sp[-3].u.mapping;
	DECLARE_CYCLIC();
	
	if((m=(struct mapping *)BEGIN_CYCLIC(tmp,0)))
	{
	  ref_push_mapping(m);
	}else{
	  INT32 e;
	  struct keypair *k;
#ifdef PIKE_DEBUG
	  struct svalue *save_sp=sp+1;
#endif
	  push_mapping(m=allocate_mapping(m_sizeof(tmp)));
	  
	  SET_CYCLIC_RET(m);
	  
	  MAPPING_LOOP(tmp)
	  {
	    push_svalue(& k->ind);
	    o_cast(itype, run_time_itype);
	    push_svalue(& k->val);
	    o_cast(vtype, run_time_vtype);
	    mapping_insert(m,sp-2,sp-1);
	    pop_n_elems(2);
	  }
#ifdef PIKE_DEBUG
	  if(save_sp!=sp)
	    Pike_fatal("o_cast left stack droppings.\n");
#endif
	}
	END_CYCLIC();
	assign_svalue(sp-4,sp-1);
	pop_stack();
      }
      pop_n_elems(2);
    }
  }
}


PMOD_EXPORT void f_cast(void)
{
#ifdef PIKE_DEBUG
  struct svalue *save_sp=sp;
  if(sp[-2].type != T_TYPE)
    Pike_fatal("Cast expression destroyed stack or left droppings! (Type:%d)\n",
	  sp[-2].type);
#endif
  o_cast(sp[-2].u.type,
	 compile_type_to_runtime_type(sp[-2].u.type));
#ifdef PIKE_DEBUG
  if(save_sp != sp)
    Pike_fatal("Internal error: o_cast() left droppings on stack.\n");
#endif
  free_svalue(sp-2);
  sp[-2]=sp[-1];
  sp--;
  dmalloc_touch_svalue(sp);
}


/*
  flags:
   *
  operators:
  %d
  %s
  %f
  %c
  %n
  %[
  %%
*/


struct sscanf_set
{
  int neg;
  char c[256];
  struct array *a;
};

/* FIXME:
 * This implementation will break in certain cases, especially cases
 * like this [\1000-\1002\1001] (ie, there is a single character which
 * is also a part of a range
 *   /Hubbe
 */


#define MKREADSET(SIZE)						\
static ptrdiff_t PIKE_CONCAT(read_set,SIZE) (			\
  PIKE_CONCAT(p_wchar,SIZE) *match,				\
  ptrdiff_t cnt,						\
  struct sscanf_set *set,					\
  ptrdiff_t match_len)						\
{								\
  size_t e, last = 0;						\
  CHAROPT( int set_size=0; )					\
								\
  if(cnt>=match_len)						\
    Pike_error("Error in sscanf format string.\n");			\
								\
  MEMSET(set->c, 0, sizeof(set->c));				\
  set->a=0;							\
								\
  if(match[cnt]=='^')						\
  {								\
    set->neg=1;							\
    cnt++;							\
    if(cnt>=match_len)						\
      Pike_error("Error in sscanf format string.\n");		\
  }else{							\
    set->neg=0;							\
  }								\
								\
  if(match[cnt]==']' || match[cnt]=='-')			\
  {								\
    set->c[last=match[cnt]]=1;					\
    cnt++;							\
    if(cnt>=match_len)						\
      Pike_error("Error in sscanf format string.\n");		\
  }								\
								\
  for(;match[cnt]!=']';cnt++)					\
  {								\
    if(match[cnt]=='-')						\
    {								\
      cnt++;							\
      if(cnt>=match_len)					\
	Pike_error("Error in sscanf format string.\n");		\
								\
      if(match[cnt]==']')					\
      {								\
	set->c['-']=1;						\
	break;							\
      }								\
								\
      if(last >= match[cnt])					\
	Pike_error("Error in sscanf format string.\n");		\
								\
CHAROPT(							\
      if(last < (size_t)sizeof(set->c))				\
      {								\
	if(match[cnt] < (size_t)sizeof(set->c))			\
	{							\
)								\
	  for(e=last;e<=match[cnt];e++) set->c[e]=1;		\
CHAROPT(							\
	}else{							\
	  for(e=last;e<(size_t)sizeof(set->c);e++)		\
            set->c[e]=1;					\
								\
	  check_stack(2);					\
	  push_int(256);					\
	  push_int(match[cnt]);					\
	  set_size++;						\
	}							\
      }								\
      else							\
      {								\
	sp[-1].u.integer=match[cnt];				\
      }								\
)								\
      continue;							\
    }								\
    last=match[cnt];						\
    if(last < (size_t)sizeof(set->c))				\
      set->c[last]=1;						\
CHAROPT(							\
    else{							\
      if(set_size &&						\
	 ((size_t)sp[-1].u.integer) == last-1)			\
      {								\
	sp[-1].u.integer++;					\
      }else{							\
	check_stack(2);						\
	push_int64(last);					\
	push_int64(last);					\
	set_size++;						\
      }								\
    }								\
)								\
  }								\
								\
CHAROPT(							\
  if(set_size)							\
  {								\
    INT32 *order;						\
    set->a=aggregate_array(set_size*2);				\
    order=get_switch_order(set->a);				\
    for(e=0;e<(size_t)set->a->size;e+=2)			\
    {								\
      if(order[e]+1 != order[e+1] &&				\
         order[e+1]+1 != order[e]) {				\
        free_array(set->a);					\
        set->a=0;						\
        free((char *)order);					\
        Pike_error("Overlapping ranges in sscanf not supported.\n");	\
      }								\
    }								\
								\
    order_array(set->a,order);					\
    free((char *)order);					\
  }								\
)								\
  return cnt;							\
}



/* Parse binary IEEE strings on a machine which uses a different kind
   of floating point internally */

#ifndef FLOAT_IS_IEEE_BIG
#ifndef FLOAT_IS_IEEE_LITTLE
#define NEED_CUSTOM_IEEE
#endif
#endif
#ifndef NEED_CUSTOM_IEEE
#ifndef DOUBLE_IS_IEEE_BIG
#ifndef DOUBLE_IS_IEEE_LITTLE
#define NEED_CUSTOM_IEEE
#endif
#endif
#endif

#ifdef NEED_CUSTOM_IEEE

#if HAVE_LDEXP
#define LDEXP ldexp
#else
extern double LDEXP(double x, int exp); /* defined in encode.c */
#endif

static INLINE FLOAT_TYPE low_parse_IEEE_float(char *b, int sz)
{
  unsigned INT32 f, extra_f;
  int s, e;
  unsigned char x[4];
  double r;
  DECLARE_INF
  DECLARE_NAN

  x[0] = EXTRACT_UCHAR(b);
  x[1] = EXTRACT_UCHAR(b+1);
  x[2] = EXTRACT_UCHAR(b+2);
  x[3] = EXTRACT_UCHAR(b+3);
  s = ((x[0]&0x80)? 1 : 0);

  if(sz==4) {
    e = (((int)(x[0]&0x7f))<<1)|((x[1]&0x80)>>7);
    f = (((unsigned INT32)(x[1]&0x7f))<<16)|(((unsigned INT32)x[2])<<8)|x[3];
    extra_f = 0;
    if(e==255)
      e = 9999;
    else if(e>0) {
      f |= 0x00800000;
      e -= 127+23;
    } else
      e -= 126+23;
  } else {
    e = (((int)(x[0]&0x7f))<<4)|((x[1]&0xf0)>>4);
    f = (((unsigned INT32)(x[1]&0x0f))<<16)|(((unsigned INT32)x[2])<<8)|x[3];
    extra_f = (((unsigned INT32)EXTRACT_UCHAR(b+4))<<24)|
      (((unsigned INT32)EXTRACT_UCHAR(b+5))<<16)|
      (((unsigned INT32)EXTRACT_UCHAR(b+6))<<8)|
      ((unsigned INT32)EXTRACT_UCHAR(b+7));
    if(e==2047)
      e = 9999;
    else if(e>0) {
      f |= 0x00100000;
      e -= 1023+20;
    } else
      e -= 1022+20;
  }
  if(e>=9999)
    if(f||extra_f) {
      /* NAN */
      return (FLOAT_TYPE)MAKE_NAN();
    } else {
      /* +/- Infinity */
      return (FLOAT_TYPE)MAKE_INF(s? -1:1);
    }

  r = (double)f;
  if(extra_f)
    r += ((double)extra_f)/4294967296.0;
  return (FLOAT_TYPE)(s? -LDEXP(r, e):LDEXP(r, e));
}

#endif

#ifdef PIKE_DEBUG
#define DO_IF_DEBUG(X)		X
#else /* !PIKE_DEBUG */
#define DO_IF_DEBUG(X)
#endif /* PIKE_DEBUG */

#ifdef AUTO_BIGNUM
#define DO_IF_BIGNUM(X)		X
#else /* !AUTO_BIGNUM */
#define DO_IF_BIGNUM(X)
#endif /* AUTO_BIGNUM */

#ifdef __CHECKER__
#define DO_IF_CHECKER(X)	X
#else /* !__CHECKER__ */
#define DO_IF_CHECKER(X)
#endif /* __CHECKER__ */

#ifdef FLOAT_IS_IEEE_BIG
#define EXTRACT_FLOAT(SVAL, INPUT, SHIFT)		\
	    do {					\
	      float f;					\
	      ((char *)&f)[0] = *((INPUT));		\
	      ((char *)&f)[1] = *((INPUT)+1);		\
	      ((char *)&f)[2] = *((INPUT)+2);		\
	      ((char *)&f)[3] = *((INPUT)+3);		\
	      (SVAL).u.float_number = f;		\
	    } while(0)
#else
#ifdef FLOAT_IS_IEEE_LITTLE
#define EXTRACT_FLOAT(SVAL, INPUT, SHIFT)		\
	    do {					\
	      float f;					\
	      ((char *)&f)[3] = *((INPUT));		\
	      ((char *)&f)[2] = *((INPUT)+1);		\
	      ((char *)&f)[1] = *((INPUT)+2);		\
	      ((char *)&f)[0] = *((INPUT)+3);		\
	      (SVAL).u.float_number = f;		\
	    } while(0)
#else
#define EXTRACT_FLOAT(SVAL, INPUT, SHIFT)				\
	    do {							\
	      char x[4];						\
	      x[0] = (INPUT)[0];					\
	      x[1] = (INPUT)[1];					\
	      x[2] = (INPUT)[2];					\
	      x[3] = (INPUT)[3];					\
	      (SVAL).u.float_number = low_parse_IEEE_float(x, 4);	\
            } while(0)
#endif
#endif

#ifdef DOUBLE_IS_IEEE_BIG
#define EXTRACT_DOUBLE(SVAL, INPUT, SHIFT)		\
	    do {					\
	      double d;					\
	      ((char *)&d)[0] = *((INPUT));		\
	      ((char *)&d)[1] = *((INPUT)+1);		\
	      ((char *)&d)[2] = *((INPUT)+2);		\
	      ((char *)&d)[3] = *((INPUT)+3);		\
	      ((char *)&d)[4] = *((INPUT)+4);		\
	      ((char *)&d)[5] = *((INPUT)+5);		\
	      ((char *)&d)[6] = *((INPUT)+6);		\
	      ((char *)&d)[7] = *((INPUT)+7);		\
	      (SVAL).u.float_number = (FLOAT_TYPE)d;	\
	    } while(0)
#else
#ifdef DOUBLE_IS_IEEE_LITTLE
#define EXTRACT_DOUBLE(SVAL, INPUT, SHIFT)		\
	    do {					\
	      double d;					\
	      ((char *)&d)[7] = *((INPUT));		\
	      ((char *)&d)[6] = *((INPUT)+1);		\
	      ((char *)&d)[5] = *((INPUT)+2);		\
	      ((char *)&d)[4] = *((INPUT)+3);		\
	      ((char *)&d)[3] = *((INPUT)+4);		\
	      ((char *)&d)[2] = *((INPUT)+5);		\
	      ((char *)&d)[1] = *((INPUT)+6);		\
	      ((char *)&d)[0] = *((INPUT)+7);		\
	      (SVAL).u.float_number = (FLOAT_TYPE)d;	\
	    } while(0)
#else
#define EXTRACT_DOUBLE(SVAL, INPUT, SHIFT)				\
	    do {							\
	      char x[8];						\
	      x[0] = (INPUT)[0];					\
	      x[1] = (INPUT)[1];					\
	      x[2] = (INPUT)[2];					\
	      x[3] = (INPUT)[3];					\
	      x[4] = (INPUT)[4];					\
	      x[5] = (INPUT)[5];					\
	      x[6] = (INPUT)[6];					\
	      x[7] = (INPUT)[7];					\
	      (SVAL).u.float_number = low_parse_IEEE_float(x, 8);	\
	    } while(0)
#endif
#endif

/* Avoid some warnings about loss of precision */
#ifdef __ECL
static inline INT32 TO_INT32(ptrdiff_t x)
{
  return DO_NOT_WARN((INT32)x);
}
#else /* !__ECL */
#define TO_INT32(x)	((INT32)(x))
#endif /* __ECL */

#define MK_VERY_LOW_SSCANF(INPUT_SHIFT, MATCH_SHIFT)			 \
static INT32 PIKE_CONCAT4(very_low_sscanf_,INPUT_SHIFT,_,MATCH_SHIFT)(	 \
                         PIKE_CONCAT(p_wchar, INPUT_SHIFT) *input,	 \
			 ptrdiff_t input_len,				 \
			 PIKE_CONCAT(p_wchar, MATCH_SHIFT) *match,	 \
			 ptrdiff_t match_len,				 \
			 ptrdiff_t *chars_matched,			 \
			 int *success)					 \
{									 \
  struct svalue sval;							 \
  INT32 matches, arg;							 \
  ptrdiff_t cnt, eye, e, field_length = 0;				 \
  int no_assign = 0, minus_flag = 0;					 \
  struct sscanf_set set;						 \
									 \
									 \
  set.a = 0;								 \
  success[0] = 0;							 \
									 \
  eye = arg = matches = 0;						 \
									 \
  for(cnt = 0; cnt < match_len; cnt++)					 \
  {									 \
    for(;cnt<match_len;cnt++)						 \
    {									 \
      if(match[cnt]=='%')						 \
      {									 \
        if(match[cnt+1]=='%')						 \
        {								 \
          cnt++;							 \
        }else{								 \
          break;							 \
        }								 \
      }									 \
      if(eye>=input_len || input[eye]!=match[cnt])			 \
      {									 \
	chars_matched[0]=eye;						 \
	return matches;							 \
      }									 \
      eye++;								 \
    }									 \
    if(cnt>=match_len)							 \
    {									 \
      chars_matched[0]=eye;						 \
      return matches;							 \
    }									 \
									 \
    DO_IF_DEBUG(							 \
    if(match[cnt]!='%' || match[cnt+1]=='%')				 \
    {									 \
      Pike_fatal("Error in sscanf.\n");					 \
    }									 \
    );									 \
									 \
    no_assign=0;							 \
    field_length=-1;							 \
    minus_flag=0;							 \
									 \
    cnt++;								 \
    if(cnt>=match_len)							 \
      Pike_error("Error in sscanf format string.\n");			 \
									 \
    while(1)								 \
    {									 \
      switch(match[cnt])						 \
      {									 \
	case '*':							 \
	  no_assign=1;							 \
	  cnt++;							 \
	  if(cnt>=match_len)						 \
	    Pike_error("Error in sscanf format string.\n");			 \
	  continue;							 \
									 \
	case '0': case '1': case '2': case '3': case '4':		 \
	case '5': case '6': case '7': case '8': case '9':		 \
	{								 \
	  PCHARP t;							 \
	  field_length = STRTOL_PCHARP(MKPCHARP(match+cnt, MATCH_SHIFT), \
				       &t,10);				 \
	  cnt = SUBTRACT_PCHARP(t, MKPCHARP(match, MATCH_SHIFT));	 \
	  continue;							 \
	}								 \
									 \
        case '-':							 \
	  minus_flag=1;							 \
	  cnt++;							 \
	  continue;							 \
									 \
	case '{':							 \
	{								 \
	  ONERROR err;							 \
	  ptrdiff_t tmp;						 \
	  for(e=cnt+1,tmp=1;tmp;e++)					 \
	  {								 \
	    if(e>=match_len)						 \
	    {								 \
	      Pike_error("Missing %%} in format string.\n");			 \
	      break;		/* UNREACHED */				 \
	    }								 \
	    if(match[e]=='%')						 \
	    {								 \
	      switch(match[e+1])					 \
	      {								 \
		case '%': e++; break;					 \
		case '}': tmp--; break;					 \
		case '{': tmp++; break;					 \
	      }								 \
	    }								 \
	  }								 \
	  sval.type=T_ARRAY;						 \
	  sval.u.array=allocate_array(0);				 \
	  SET_ONERROR(err, do_free_array, sval.u.array);		 \
									 \
	  while(input_len-eye)						 \
	  {								 \
	    int yes;							 \
	    struct svalue *save_sp=sp;					 \
	    PIKE_CONCAT4(very_low_sscanf_, INPUT_SHIFT, _, MATCH_SHIFT)( \
                         input+eye,					 \
			 input_len-eye,					 \
			 match+cnt+1,					 \
			 e-cnt-2,					 \
			 &tmp,						 \
			 &yes);						 \
	    if(yes && tmp)						 \
	    {								 \
	      f_aggregate(TO_INT32(sp-save_sp));			 \
	      sval.u.array=append_array(sval.u.array,sp-1);		 \
	      pop_stack();						 \
	      eye+=tmp;							 \
	    }else{							 \
	      pop_n_elems(sp-save_sp);					 \
	      break;							 \
	    }								 \
	  }								 \
	  cnt=e;							 \
	  UNSET_ONERROR(err);						 \
	  break;							 \
	}								 \
									 \
	case 'c':							 \
        {								 \
CHAROPT2(								 \
          int e;							 \
)									 \
	  sval.type=T_INT;						 \
	  sval.subtype=NUMBER_NUMBER;					 \
          if(field_length == -1)					 \
          { 								 \
	    if(eye+1 > input_len)					 \
	    {								 \
	      chars_matched[0]=eye;					 \
	      return matches;						 \
	    }								 \
            sval.u.integer=input[eye];					 \
	    eye++;							 \
            break;							 \
          }								 \
	  if(eye+field_length > input_len)				 \
	  {								 \
	    chars_matched[0]=eye;					 \
	    return matches;						 \
	  }								 \
CHAROPT2(								 \
          for(e=0;e<field_length;e++)					 \
          {								 \
             if(input[eye+e]>255)					 \
             {								 \
               chars_matched[0]=eye;					 \
               return matches;						 \
             }								 \
          }								 \
)									 \
	  sval.u.integer=0;						 \
	  if (minus_flag)						 \
	  {								 \
	     int x, pos=0;						 \
									 \
	     while(--field_length >= 0)					 \
	     {								 \
	       x = input[eye];						 \
									 \
               DO_IF_BIGNUM(						 \
	       if(INT_TYPE_LSH_OVERFLOW(x, pos))			 \
	       {							 \
		 push_int(sval.u.integer);				 \
		 convert_stack_top_to_bignum();				 \
									 \
		 while(field_length-- >= 0)				 \
		 {							 \
		   push_int(input[eye]);				 \
		   convert_stack_top_to_bignum();			 \
		   push_int(pos);					 \
		   o_lsh();						 \
		   o_or();						 \
		   pos+=8;						 \
		   eye++;						 \
		 }							 \
		 sval=*--sp;						 \
		 break;							 \
	       }							 \
               );							 \
	       sval.u.integer|=x<<pos;					 \
									 \
	       pos+=8;							 \
	       eye++;							 \
	     }								 \
	  }								 \
	  else								 \
	     while(--field_length >= 0)					 \
	     {								 \
               DO_IF_BIGNUM(						 \
	       if(INT_TYPE_LSH_OVERFLOW(sval.u.integer, 8))		 \
	       {							 \
		 push_int(sval.u.integer);				 \
		 convert_stack_top_to_bignum();				 \
									 \
		 while(field_length-- >= 0)				 \
		 {							 \
		   push_int(8);						 \
		   o_lsh();						 \
		   push_int(input[eye]);				 \
		   o_or();						 \
		   eye++;						 \
		 }							 \
		 sval=*--sp;						 \
		 break;							 \
	       }							 \
	       );							 \
	       sval.u.integer<<=8;					 \
	       sval.u.integer |= input[eye];				 \
	       eye++;							 \
	     }								 \
	  break;							 \
        }								 \
									 \
        case 'b':							 \
        case 'o':							 \
        case 'd':							 \
        case 'x':							 \
        case 'D':							 \
        case 'i':							 \
	{								 \
	  int base = 0;							 \
	  PIKE_CONCAT(p_wchar, INPUT_SHIFT) *t;				 \
									 \
	  if(eye>=input_len)						 \
	  {								 \
	    chars_matched[0]=eye;					 \
	    return matches;						 \
	  }								 \
									 \
	  switch(match[cnt])						 \
	  {								 \
	  case 'b': base =  2; break;					 \
	  case 'o': base =  8; break;					 \
	  case 'd': base = 10; break;					 \
	  case 'x': base = 16; break;					 \
	  }								 \
									 \
	  wide_string_to_svalue_inumber(&sval, input+eye, (void **)&t,	 \
					base, field_length,		 \
					INPUT_SHIFT);			 \
									 \
	  if(input + eye == t)						 \
	  {								 \
	    chars_matched[0]=eye;					 \
	    return matches;						 \
	  }								 \
	  eye=t-input;							 \
	  break;							 \
	}								 \
									 \
        case 'f':							 \
	{								 \
	  PIKE_CONCAT(p_wchar, INPUT_SHIFT) *t;				 \
	  PCHARP t2;							 \
									 \
	  if(eye>=input_len)						 \
	  {								 \
	    chars_matched[0]=eye;					 \
	    return matches;						 \
	  }								 \
	  sval.u.float_number =						 \
	    (FLOAT_TYPE)STRTOD_PCHARP(MKPCHARP(input+eye,		 \
					       INPUT_SHIFT),&t2);	 \
	  t = (PIKE_CONCAT(p_wchar, INPUT_SHIFT) *)(t2.ptr);		 \
	  if(input + eye == t)						 \
	  {								 \
	    chars_matched[0]=eye;					 \
	    return matches;						 \
	  }								 \
	  eye=t-input;							 \
	  sval.type=T_FLOAT;						 \
	  DO_IF_CHECKER(sval.subtype=0);				 \
	  break;							 \
	}								 \
									 \
	case 'F':							 \
	  if(field_length == -1) field_length = 4;			 \
	  if(field_length != 4 && field_length != 8)			 \
	    Pike_error("Invalid IEEE width %ld in sscanf format string.\n",	 \
		  PTRDIFF_T_TO_LONG(field_length));			 \
	  if(eye+field_length > input_len)				 \
	  {								 \
	    chars_matched[0]=eye;					 \
	    return matches;						 \
	  }								 \
	  sval.type=T_FLOAT;						 \
	  DO_IF_CHECKER(sval.subtype=0);				 \
	  switch(field_length) {					 \
	    case 4:							 \
	      EXTRACT_FLOAT(sval, input+eye, INPUT_SHIFT);		 \
	      eye += 4;							 \
	      break;							 \
	    case 8:							 \
	      EXTRACT_DOUBLE(sval, input+eye, INPUT_SHIFT);		 \
	      eye += 8;							 \
	      break;							 \
	  }								 \
	  break;							 \
									 \
	case 's':							 \
	  if(field_length != -1)					 \
	  {								 \
	    if(input_len - eye < field_length)				 \
	    {								 \
	      chars_matched[0]=eye;					 \
	      return matches;						 \
	    }								 \
									 \
	    if (no_assign) {						 \
	      no_assign = 2;						 \
	    } else {							 \
	      sval.type=T_STRING;					 \
	      DO_IF_CHECKER(sval.subtype=0);				 \
	      sval.u.string=PIKE_CONCAT(make_shared_binary_string,	 \
					INPUT_SHIFT)(input+eye,		 \
						     field_length);	 \
	    }								 \
	    eye+=field_length;						 \
	    break;							 \
	  }								 \
									 \
	  if(cnt+1>=match_len)						 \
	  {								 \
	    if (no_assign) {						 \
	      no_assign = 2;						 \
	    } else {							 \
	      sval.type=T_STRING;					 \
	      DO_IF_CHECKER(sval.subtype=0);				 \
	      sval.u.string=PIKE_CONCAT(make_shared_binary_string,	 \
					INPUT_SHIFT)(input+eye,		 \
						     input_len-eye);	 \
	    }								 \
	    eye=input_len;						 \
	    break;							 \
	  }else{							 \
	    PIKE_CONCAT(p_wchar, MATCH_SHIFT) *end_str_start;		 \
	    PIKE_CONCAT(p_wchar, MATCH_SHIFT) *end_str_end;		 \
	    PIKE_CONCAT(p_wchar, MATCH_SHIFT) *s=0;			 \
	    PIKE_CONCAT(p_wchar, MATCH_SHIFT) *p=0;			 \
	    int contains_percent_percent;				 \
            ptrdiff_t start, new_eye;					 \
									 \
	    e = cnt;							 \
	    start=eye;							 \
	    end_str_start=match+cnt+1;					 \
									 \
	    s=match+cnt+1;						 \
      test_again:							 \
	    if(*s=='%')							 \
	    {								 \
	      s++;							 \
	      if(*s=='*') s++;						 \
              set.neg=0;						 \
	      switch(*s)						 \
	      {								 \
                case 0:							 \
                  /* FIXME: Should really look at the match len */	 \
		  Pike_error("%% without conversion specifier.\n");	 \
                  break;						 \
									 \
		case 'n':						 \
		  s++;							 \
                  /* Advance the end string start pointer */		 \
                  end_str_start = s;					 \
		  e = s - match;					 \
	          goto test_again;					 \
									 \
		case 's':						 \
		  Pike_error("Illegal to have two adjecent %%s.\n");	 \
		  return 0;		/* make gcc happy */		 \
									 \
	  /* sscanf("foo-bar","%s%d",a,b) might not work as expected */	 \
		case 'd':						 \
		  MEMSET(set.c, 1, sizeof(set.c));			 \
		  for(e='0';e<='9';e++) set.c[e]=0;			 \
		  set.c['-']=0;						 \
		  goto match_set;					 \
									 \
		case 'o':						 \
		  MEMSET(set.c, 1, sizeof(set.c));			 \
		  for(e='0';e<='7';e++) set.c[e]=0;			 \
		  goto match_set;					 \
									 \
		case 'x':						 \
		  MEMSET(set.c, 1, sizeof(set.c));			 \
		  for(e='0';e<='9';e++) set.c[e]=0;			 \
		  for(e='a';e<='f';e++) set.c[e]=0;			 \
		  goto match_set;					 \
									 \
		case 'D':						 \
		  MEMSET(set.c, 1, sizeof(set.c));			 \
		  for(e='0';e<='9';e++) set.c[e]=0;			 \
		  set.c['-']=0;						 \
		  goto match_set;					 \
									 \
		case 'f':						 \
		  MEMSET(set.c, 1, sizeof(set.c));			 \
		  for(e='0';e<='9';e++) set.c[e]=0;			 \
		  set.c['.']=set.c['-']=0;				 \
		  goto match_set;					 \
									 \
		case '[':		/* oh dear */			 \
		  PIKE_CONCAT(read_set,MATCH_SHIFT)(match,		 \
						    s-match+1,		 \
						    &set,		 \
						    match_len);		 \
		  set.neg=!set.neg;					 \
		  goto match_set;					 \
	      }								 \
	    }								 \
									 \
	    contains_percent_percent=0;					 \
									 \
	    for(;e<match_len;e++)					 \
	    {								 \
	      if(match[e]=='%')						 \
	      {								 \
		if(match[e+1]=='%')					 \
		{							 \
		  contains_percent_percent=1;				 \
		  e++;							 \
		}else{							 \
		  break;						 \
		}							 \
	      }								 \
	    }								 \
									 \
	    end_str_end=match+e;					 \
									 \
	    if (end_str_end == end_str_start) {				 \
	      if (no_assign) {						 \
		no_assign = 2;						 \
	      } else {							 \
		sval.type=T_STRING;					 \
		DO_IF_CHECKER(sval.subtype=0);				 \
		sval.u.string=PIKE_CONCAT(make_shared_binary_string,	 \
					  INPUT_SHIFT)(input+eye,	 \
						       input_len-eye);	 \
	      }								 \
	      eye=input_len;						 \
	      break;							 \
	    } else if(!contains_percent_percent)			 \
	    {								 \
	      struct generic_mem_searcher searcher;			 \
	      PIKE_CONCAT(p_wchar, INPUT_SHIFT) *s2;			 \
	      init_generic_memsearcher(&searcher, end_str_start,	 \
				       end_str_end - end_str_start,	 \
				       MATCH_SHIFT, input_len - eye,	 \
				       INPUT_SHIFT);			 \
	      s2 = generic_memory_search(&searcher, input+eye,		 \
					 input_len - eye, INPUT_SHIFT);	 \
	      if(!s2)							 \
	      {								 \
		chars_matched[0]=eye;					 \
		return matches;						 \
	      }								 \
	      eye=s2-input;						 \
	      new_eye=eye+end_str_end-end_str_start;			 \
	    }else{							 \
	      PIKE_CONCAT(p_wchar, INPUT_SHIFT) *p2 = NULL;		 \
	      for(;eye<input_len;eye++)					 \
	      {								 \
		p2=input+eye;						 \
		for(s=end_str_start;s<end_str_end;s++,p2++)		 \
		{							 \
		  if(*s!=*p2) break;					 \
		  if(*s=='%') s++;					 \
		}							 \
		if(s==end_str_end)					 \
		  break;						 \
	      }								 \
	      if(eye==input_len)					 \
	      {								 \
		chars_matched[0]=eye;					 \
		return matches;						 \
	      }								 \
	      new_eye=p2-input;						 \
	    }								 \
									 \
	    if (no_assign) {						 \
	      no_assign = 2;						 \
	    } else {							 \
	      sval.type=T_STRING;					 \
	      DO_IF_CHECKER(sval.subtype=0);				 \
	      sval.u.string=PIKE_CONCAT(make_shared_binary_string,	 \
					INPUT_SHIFT)(input+start,	 \
						     eye-start);	 \
	    }								 \
									 \
	    cnt=end_str_end-match-1;					 \
	    eye=new_eye;						 \
	    break;							 \
	  }								 \
									 \
	case '[':							 \
	  cnt=PIKE_CONCAT(read_set,MATCH_SHIFT)(match,cnt+1,		 \
						&set,match_len);	 \
									 \
  match_set:								 \
	  for(e=eye;eye<input_len;eye++)				 \
	  {								 \
CHAROPT2(								 \
	    if(input[eye]<sizeof(set.c))				 \
	    {								 \
)									 \
	      if(set.c[input[eye]] == set.neg)				 \
		break;							 \
CHAROPT2(								 \
	    }else{							 \
	      if(set.a)							 \
	      {								 \
		INT32 x;						 \
		struct svalue tmp;					 \
		tmp.type=T_INT;						 \
		tmp.u.integer=input[eye];				 \
		x=switch_lookup(set.a, &tmp);				 \
		if( set.neg != (x<0 && (x&1)) ) break;			 \
	      }else{							 \
		if(!set.neg) break;					 \
	      }								 \
	    }								 \
)									 \
	  }								 \
          if(set.a) { free_array(set.a); set.a=0; }			 \
	  if (no_assign) {						 \
	    no_assign = 2;						 \
	  } else {							 \
	    sval.type=T_STRING;						 \
	    DO_IF_CHECKER(sval.subtype=0);				 \
	    sval.u.string=PIKE_CONCAT(make_shared_binary_string,	 \
				      INPUT_SHIFT)(input+e,eye-e);	 \
	  }								 \
	  break;							 \
									 \
	case 'n':							 \
	  sval.type=T_INT;						 \
	  sval.subtype=NUMBER_NUMBER;					 \
	  sval.u.integer=TO_INT32(eye);					 \
	  break;							 \
									 \
	default:							 \
	  Pike_error("Unknown sscanf token %%%c(0x%02x)\n",		 \
		match[cnt], match[cnt]);				 \
      }									 \
      break;								 \
    }									 \
    matches++;								 \
									 \
    if(no_assign)							 \
    {									 \
      if (no_assign == 1)						 \
	free_svalue(&sval);						 \
    } else {								 \
      check_stack(1);							 \
      *sp++=sval;							 \
      DO_IF_DEBUG(sval.type=99);					 \
    }									 \
  }									 \
  chars_matched[0]=eye;							 \
  success[0]=1;								 \
  return matches;							 \
}


/* Confusing? Yes - Hubbe */

/* CHAROPT(X) is X if the match set is wide.
 * CHAROPT2(X) is X if the input is wide.
 */
#define CHAROPT(X)
#define CHAROPT2(X)

MKREADSET(0)
MK_VERY_LOW_SSCANF(0,0)

#undef CHAROPT2
#define CHAROPT2(X) X

MK_VERY_LOW_SSCANF(1,0)
MK_VERY_LOW_SSCANF(2,0)

#undef CHAROPT
#define CHAROPT(X) X

MKREADSET(1)
MKREADSET(2)

#undef CHAROPT2
#define CHAROPT2(X)

MK_VERY_LOW_SSCANF(0,1)
MK_VERY_LOW_SSCANF(0,2)

#undef CHAROPT2
#define CHAROPT2(X) X

MK_VERY_LOW_SSCANF(1,1)
MK_VERY_LOW_SSCANF(2,1)
MK_VERY_LOW_SSCANF(1,2)
MK_VERY_LOW_SSCANF(2,2)

void o_sscanf(INT32 args)
{
#ifdef PIKE_DEBUG
  extern int t_flag;
#endif
  INT32 i=0;
  int x;
  ptrdiff_t matched_chars;
  struct svalue *save_sp=sp;

  if(sp[-args].type != T_STRING)
    Pike_error("Bad argument 1 to sscanf().\n");

  if(sp[1-args].type != T_STRING)
    Pike_error("Bad argument 1 to sscanf().\n");

  switch(sp[-args].u.string->size_shift*3 + sp[1-args].u.string->size_shift) {
    /* input_shift : match_shift */
  case 0:
    /*      0      :      0 */
    i=very_low_sscanf_0_0(STR0(sp[-args].u.string),
			  sp[-args].u.string->len,
			  STR0(sp[1-args].u.string),
			  sp[1-args].u.string->len,
			  &matched_chars,
			  &x);
    break;
  case 1:
    /*      0      :      1 */
    i=very_low_sscanf_0_1(STR0(sp[-args].u.string),
			  sp[-args].u.string->len,
			  STR1(sp[1-args].u.string),
			  sp[1-args].u.string->len,
			  &matched_chars,
			  &x);
    break;
  case 2:
    /*      0      :      2 */
    i=very_low_sscanf_0_2(STR0(sp[-args].u.string),
			  sp[-args].u.string->len,
			  STR2(sp[1-args].u.string),
			  sp[1-args].u.string->len,
			  &matched_chars,
			  &x);
    break;
  case 3:
    /*      1      :      0 */
    i=very_low_sscanf_1_0(STR1(sp[-args].u.string),
			  sp[-args].u.string->len,
			  STR0(sp[1-args].u.string),
			  sp[1-args].u.string->len,
			  &matched_chars,
			  &x);
    break;
  case 4:
    /*      1      :      1 */
    i=very_low_sscanf_1_1(STR1(sp[-args].u.string),
			  sp[-args].u.string->len,
			  STR1(sp[1-args].u.string),
			  sp[1-args].u.string->len,
			  &matched_chars,
			  &x);
    break;
  case 5:
    /*      1      :      2 */
    i=very_low_sscanf_1_2(STR1(sp[-args].u.string),
			  sp[-args].u.string->len,
			  STR2(sp[1-args].u.string),
			  sp[1-args].u.string->len,
			  &matched_chars,
			  &x);
    break;
  case 6:
    /*      2      :      0 */
    i=very_low_sscanf_2_0(STR2(sp[-args].u.string),
			  sp[-args].u.string->len,
			  STR0(sp[1-args].u.string),
			  sp[1-args].u.string->len,
			  &matched_chars,
			  &x);
    break;
  case 7:
    /*      2      :      1 */
    i=very_low_sscanf_2_1(STR2(sp[-args].u.string),
			  sp[-args].u.string->len,
			  STR1(sp[1-args].u.string),
			  sp[1-args].u.string->len,
			  &matched_chars,
			  &x);
    break;
  case 8:
    /*      2      :      2 */
    i=very_low_sscanf_2_2(STR2(sp[-args].u.string),
			  sp[-args].u.string->len,
			  STR2(sp[1-args].u.string),
			  sp[1-args].u.string->len,
			  &matched_chars,
			  &x);
    break;
  default:
    Pike_error("Unsupported shift-combination to sscanf(): %d:%d\n",
	  sp[-args].u.string->size_shift, sp[1-args].u.string->size_shift);
    break;
  }

  if(sp-save_sp > args/2-1)
    Pike_error("Too few arguments for sscanf format.\n");

  for(x=0;x<sp-save_sp;x++)
    assign_lvalue(save_sp-args+2+x*2,save_sp+x);
  pop_n_elems(sp-save_sp +args);

#ifdef PIKE_DEBUG
  if(t_flag >2)
  {
    int nonblock;
    if((nonblock=query_nonblocking(2)))
      set_nonblocking(2,0);
    
    fprintf(stderr,"-    Matches: %ld\n",(long)i);
    if(nonblock)
      set_nonblocking(2,1);
  }
#endif
  push_int(i);
}

/*! @decl array array_sscanf(string data, string format)
 *!
 *! This function works just like @[sscanf()], but returns the matched
 *! results in an array instead of assigning them to lvalues. This is often
 *! useful for user-defined sscanf strings.
 *!
 *! @seealso
 *!   @[sscanf()], @[`/()]
 */
PMOD_EXPORT void f_sscanf(INT32 args)
{
#ifdef PIKE_DEBUG
  extern int t_flag;
#endif
  INT32 i;
  int x;
  ptrdiff_t matched_chars;
  struct svalue *save_sp=sp;
  struct array *a;

  check_all_args("array_sscanf",args,BIT_STRING, BIT_STRING,0);

  switch(sp[-args].u.string->size_shift*3 + sp[1-args].u.string->size_shift) {
    /* input_shift : match_shift */
  case 0:
    /*      0      :      0 */
    i=very_low_sscanf_0_0(STR0(sp[-args].u.string),
			  sp[-args].u.string->len,
			  STR0(sp[1-args].u.string),
			  sp[1-args].u.string->len,
			  &matched_chars,
			  &x);
    break;
  case 1:
    /*      0      :      1 */
    i=very_low_sscanf_0_1(STR0(sp[-args].u.string),
			  sp[-args].u.string->len,
			  STR1(sp[1-args].u.string),
			  sp[1-args].u.string->len,
			  &matched_chars,
			  &x);
    break;
  case 2:
    /*      0      :      2 */
    i=very_low_sscanf_0_2(STR0(sp[-args].u.string),
			  sp[-args].u.string->len,
			  STR2(sp[1-args].u.string),
			  sp[1-args].u.string->len,
			  &matched_chars,
			  &x);
    break;
  case 3:
    /*      1      :      0 */
    i=very_low_sscanf_1_0(STR1(sp[-args].u.string),
			  sp[-args].u.string->len,
			  STR0(sp[1-args].u.string),
			  sp[1-args].u.string->len,
			  &matched_chars,
			  &x);
    break;
  case 4:
    /*      1      :      1 */
    i=very_low_sscanf_1_1(STR1(sp[-args].u.string),
			  sp[-args].u.string->len,
			  STR1(sp[1-args].u.string),
			  sp[1-args].u.string->len,
			  &matched_chars,
			  &x);
    break;
  case 5:
    /*      1      :      2 */
    i=very_low_sscanf_1_2(STR1(sp[-args].u.string),
			  sp[-args].u.string->len,
			  STR2(sp[1-args].u.string),
			  sp[1-args].u.string->len,
			  &matched_chars,
			  &x);
    break;
  case 6:
    /*      2      :      0 */
    i=very_low_sscanf_2_0(STR2(sp[-args].u.string),
			  sp[-args].u.string->len,
			  STR0(sp[1-args].u.string),
			  sp[1-args].u.string->len,
			  &matched_chars,
			  &x);
    break;
  case 7:
    /*      2      :      1 */
    i=very_low_sscanf_2_1(STR2(sp[-args].u.string),
			  sp[-args].u.string->len,
			  STR1(sp[1-args].u.string),
			  sp[1-args].u.string->len,
			  &matched_chars,
			  &x);
    break;
  case 8:
    /*      2      :      2 */
    i=very_low_sscanf_2_2(STR2(sp[-args].u.string),
			  sp[-args].u.string->len,
			  STR2(sp[1-args].u.string),
			  sp[1-args].u.string->len,
			  &matched_chars,
			  &x);
    break;
  default:
    Pike_error("Unsupported shift-combination to sscanf(): %d:%d\n",
	  sp[-args].u.string->size_shift, sp[1-args].u.string->size_shift);
    break;
  }

  a = aggregate_array(DO_NOT_WARN(sp - save_sp));
  pop_n_elems(args);
  push_array(a);
}


void o_breakpoint(void)
{
  /* Does nothing */
}
