/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: opcodes.c,v 1.155 2003/11/14 09:06:06 mast Exp $
*/

#include "global.h"
#include "mapping.h"
#include "multiset.h"
#include "pike_error.h"
#include "pike_types.h"
#include "cyclic.h"
#include "builtin_functions.h"

#define sp Pike_sp

RCSID("$Id: opcodes.c,v 1.155 2003/11/14 09:06:06 mast Exp $");

void index_no_free(struct svalue *to,struct svalue *what,struct svalue *ind)
{
#ifdef PIKE_SECURITY
  if(what->type <= MAX_COMPLEX)
    if(!CHECK_DATA_SECURITY(what->u.array, SECURITY_BIT_INDEX))
      Pike_error("Index permission denied.\n");
#endif

  switch(what->type)
  {
  case T_ARRAY:
    simple_array_index_no_free(to,what->u.array,ind);
    break;

  case T_MAPPING:
    mapping_index_no_free(to,what->u.mapping,ind);
    break;

  case T_OBJECT:
    object_index_no_free(to, what->u.object, ind);
    break;

  case T_MULTISET: {
    int i=multiset_member(what->u.multiset, ind);
    to->type=T_INT;
    to->subtype=i ? 0 : NUMBER_UNDEFINED;
    to->u.integer=i;
    break;
  }

  case T_STRING:
    if(ind->type==T_INT)
    {
      ptrdiff_t len = what->u.string->len;
      INT_TYPE p = ind->u.integer;
      INT_TYPE i = p < 0 ? p + len : p;
      if(i<0 || i>=len)
      {
	if(len == 0)
	  Pike_error("Attempt to index the empty string with %"PRINTPIKEINT"d.\n", i);
	else
	  Pike_error("Index %"PRINTPIKEINT"d is out of string range "
		     "%"PRINTPTRDIFFT"d..%"PRINTPTRDIFFT"d.\n",
		     i, -len, len - 1);
      } else
	i=index_shared_string(what->u.string,i);
      to->type=T_INT;
      to->subtype=NUMBER_NUMBER;
      to->u.integer=i;
      break;
    }else{
      if (ind->type == T_STRING && !ind->u.string->size_shift)
	Pike_error ("Expected integer as string index, got \"%s\".\n",
		    ind->u.string->str);
      else
	Pike_error ("Expected integer as string index, got %s.\n",
		    get_name_of_type (ind->type));
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

#ifdef AUTO_BIGNUM
  case T_INT:
    if (ind->type == T_STRING) {
      INT_TYPE val = what->u.integer;

      convert_svalue_to_bignum(what);
      index_no_free(to, what, ind);
      if(IS_UNDEFINED(to)) {
	if (val) {
	  if (!ind->u.string->size_shift)
	    Pike_error("Indexing the integer %"PRINTPIKEINT"d "
		       "with unknown method \"%s\".\n",
		       val, ind->u.string->str);
	  else
	    Pike_error("Indexing the integer %"PRINTPIKEINT"d "
		       "with a wide string.\n",
		       val);
	} else {
	  if(!ind->u.string->size_shift)
            Pike_error("Indexing the NULL value with \"%s\".\n",
		       ind->u.string->str);
          else
	    Pike_error("Indexing the NULL value with a wide string.\n");
	}
      }
      break;
    }

    /* FALL_THROUGH */
#endif /* AUTO_BIGNUM */    

  default:
    if (ind->type == T_INT)
      Pike_error ("Cannot index %s with %"PRINTPIKEINT"d.\n",
		  (what->type == T_INT && !what->u.integer)?
		  "the NULL value":get_name_of_type(what->type),
		  ind->u.integer);
    else if (ind->type == T_FLOAT)
      Pike_error ("Cannot index %s with %"PRINTPIKEFLOAT"g.\n",
		  (what->type == T_INT && !what->u.integer)?
		  "the NULL value":get_name_of_type(what->type),
		  ind->u.float_number);
    else if (ind->type == T_STRING && !ind->u.string->size_shift)
      Pike_error ("Cannot index %s with \"%s\".\n",
		  (what->type == T_INT && !what->u.integer)?
		  "the NULL value":get_name_of_type(what->type),
		  ind->u.string->str);
    else
      Pike_error ("Cannot index %s with %s.\n",
		  (what->type == T_INT && !what->u.integer)?
		  "the NULL value":get_name_of_type(what->type),
		  get_name_of_type (ind->type));
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
  dmalloc_touch_svalue(Pike_sp-1);
}

/*! @class MasterObject
 */

/*! @decl object cast_to_object(string str, string|void current_file)
 *!
 *!   Called by the Pike runtime to cast strings to objects.
 *!
 *! @param str
 *!   String to cast to object.
 *!
 *! @param current_file
 *!   Filename of the file that attempts to perform the cast.
 *!
 *! @returns
 *!   Returns the resulting object.
 *!
 *! @seealso
 *!   @[cast_to_program()]
 */

/*! @decl program cast_to_program(string str, string|void current_file)
 *!
 *!   Called by the Pike runtime to cast strings to programs.
 *!
 *! @param str
 *!   String to cast to object.
 *!
 *! @param current_file
 *!   Filename of the file that attempts to perform the cast.
 *!
 *! @returns
 *!   Returns the resulting program.
 *!
 *! @seealso
 *!   @[cast_to_object()]
 */

/*! @endclass
 */

/* Special case for casting to int. */
void o_cast_to_int(void)
{
  switch(sp[-1].type)
  {
  case T_OBJECT:
    {
      struct pike_string *s;
      REF_MAKE_CONST_STRING(s, "int");
      push_string(s);
      if(!sp[-2].u.object->prog)
	Pike_error("Cast called on destructed object.\n");
      if(FIND_LFUN(sp[-2].u.object->prog,LFUN_CAST) == -1)
	Pike_error("No cast method in object.\n");
      apply_lfun(sp[-2].u.object, LFUN_CAST, 1);
      free_svalue(sp-2);
      sp[-2]=sp[-1];
      sp--;
      dmalloc_touch_svalue(sp);
    }
    if(sp[-1].type != PIKE_T_INT)
    {
      if(sp[-1].type == T_OBJECT && sp[-1].u.object->prog)
      {
	int f=FIND_LFUN(sp[-1].u.object->prog, LFUN__IS_TYPE);
	if( f != -1)
	{
	  struct pike_string *s;
	  REF_MAKE_CONST_STRING(s, "int");
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

/* Special case for casting to string. */
void o_cast_to_string(void)
{
  char buf[200];
  switch(sp[-1].type)
  {
  case PIKE_T_STRING:
    return;

  case T_OBJECT:
    {
      struct pike_string *s;
      REF_MAKE_CONST_STRING(s, "string");
      push_string(s);
      if(!sp[-2].u.object->prog)
	Pike_error("Cast called on destructed object.\n");
      if(FIND_LFUN(sp[-2].u.object->prog,LFUN_CAST) == -1)
	Pike_error("No cast method in object.\n");
      apply_lfun(sp[-2].u.object, LFUN_CAST, 1);
      free_svalue(sp-2);
      sp[-2]=sp[-1];
      sp--;
      dmalloc_touch_svalue(sp);
    }
    if(sp[-1].type != PIKE_T_STRING)
    {
      if(sp[-1].type == T_OBJECT && sp[-1].u.object->prog)
      {
	int f=FIND_LFUN(sp[-1].u.object->prog, LFUN__IS_TYPE);
	if( f != -1)
	{
	  struct pike_string *s;
	  REF_MAKE_CONST_STRING(s, "string");
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
	    
  case T_INT:
    sprintf(buf, "%"PRINTPIKEINT"d", sp[-1].u.integer);
    break;
	    
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

    if(sp[-1].type == T_OBJECT)
    {
      struct pike_string *s;
      s=describe_type(type);
      push_string(s);
      if(!sp[-2].u.object->prog)
	Pike_error("Cast called on destructed object.\n");
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
	    /* FIXME: Ought to allow compile_handler to override.
	     */
	    APPLY_MASTER("cast_to_object",2);
	    return;
	  }
	    
	  case T_FUNCTION:
	    if (Pike_sp[-1].subtype == FUNCTION_BUILTIN) {
	      Pike_error("Cannot cast builtin functions to object.\n");
	    } else if (Pike_sp[-1].u.object->prog == pike_trampoline_program) {
	      ref_push_object(((struct pike_trampoline *)
			       (Pike_sp[-1].u.object->storage))->
			      frame->current_object);
	      stack_pop_keep_top();
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
	  /* FIXME: Ought to allow compile_handler to override.
	   */
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
	  TYPE_FIELD types = 0;
#ifdef PIKE_DEBUG
	  struct svalue *save_sp=sp+1;
#endif
	  push_array(a=allocate_array(tmp->size));
	  SET_CYCLIC_RET(a);
	  
	  for(e=0;e<a->size;e++)
	  {
	    push_svalue(tmp->item+e);
	    o_cast(itype, run_time_itype);
	    stack_pop_to_no_free (ITEM(a) + e);
	    types |= 1 << ITEM(a)[e].type;
	  }
	  a->type_field = types;
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
	  TYPE_FIELD types = 0;
	  push_multiset(m=allocate_multiset(a=allocate_array(tmp->size)));
	  
	  SET_CYCLIC_RET(m);
	  
	  for(e=0;e<a->size;e++)
	  {
	    push_svalue(tmp->item+e);
	    o_cast(itype, run_time_itype);
	    stack_pop_to_no_free (ITEM(a) + e);
	    types |= 1 << ITEM(a)[e].type;
	  }
	  a->type_field = types;
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

void o_breakpoint(void)
{
  /* Does nothing */
}
