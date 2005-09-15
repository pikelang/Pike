/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: operators.c,v 1.205 2005/09/15 12:30:46 grubba Exp $
*/

#include "global.h"
#include <math.h>
#include "interpret.h"
#include "svalue.h"
#include "multiset.h"
#include "mapping.h"
#include "array.h"
#include "stralloc.h"
#include "opcodes.h"
#include "operators.h"
#include "pike_memory.h"
#include "pike_error.h"
#include "docode.h"
#include "constants.h"
#include "peep.h"
#include "lex.h"
#include "program.h"
#include "object.h"
#include "pike_types.h"
#include "module_support.h"
#include "pike_macros.h"
#include "bignum.h"
#include "builtin_functions.h"
#include "cyclic.h"
#include "pike_security.h"

#define sp Pike_sp

#define OP_DIVISION_BY_ZERO_ERROR(FUNC) \
     math_error(FUNC, sp-2, 2, 0, "Division by zero.\n")
#define OP_MODULO_BY_ZERO_ERROR(FUNC) \
     math_error(FUNC, sp-2, 2, 0, "Modulo by zero.\n")

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
      if (ind->type == T_STRING)
	Pike_error ("Expected integer as string index, got \"%S\".\n",
		    ind->u.string);
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
	  Pike_error("Indexing the integer %"PRINTPIKEINT"d "
		     "with unknown method \"%S\".\n",
		     val, ind->u.string);
	} else {
	  Pike_error("Indexing the NULL value with \"%S\".\n",
		     ind->u.string);
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
    else if (ind->type == T_STRING)
      Pike_error ("Cannot index %s with \"%S\".\n",
		  (what->type == T_INT && !what->u.integer)?
		  "the NULL value":get_name_of_type(what->type),
		  ind->u.string);
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
    if(!sp[-1].u.object->prog) {
      /* Casting a destructed object should be like casting a zero. */
      pop_stack();
      push_int (0);
    }

    else {
      {
	struct object *o = sp[-1].u.object;
	struct pike_string *s;
	struct program *p = o->prog->inherits[sp[-1].subtype].prog;
	int f = FIND_LFUN(p, LFUN_CAST);
	if(f == -1)
	  Pike_error("No cast method in object.\n");
	REF_MAKE_CONST_STRING(s, "int");
	push_string(s);
	apply_low(o, f, 1);
	stack_pop_keep_top();
      }

      if(sp[-1].type != PIKE_T_INT)
      {
	if(sp[-1].type == T_OBJECT && sp[-1].u.object->prog)
	{
	  struct object *o = sp[-1].u.object;
	  int f = FIND_LFUN(o->prog->inherits[sp[-1].subtype].prog,
			    LFUN__IS_TYPE);
	  if( f != -1)
	  {
	    struct pike_string *s;
	    REF_MAKE_CONST_STRING(s, "int");
	    push_string(s);
	    apply_low(o, f, 1);
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

/* Special case for casting to string. */
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
      push_constant_text("0");
    } else {
      {
	struct object *o = sp[-1].u.object;
	struct pike_string *s;
	int f = FIND_LFUN(o->prog->inherits[sp[-1].subtype].prog, LFUN_CAST);
	if(f == -1)
	  Pike_error("No cast method in object.\n");
	push_constant_text("string");
	apply_low(o, f, 1);
	stack_pop_keep_top();
      }

      if(sp[-1].type != PIKE_T_STRING)
      {
	if(sp[-1].type == T_OBJECT && sp[-1].u.object->prog)
	{
	  struct object *o = sp[-1].u.object;
	  int f = FIND_LFUN(o->prog->inherits[sp[-1].subtype].prog,
			    LFUN__IS_TYPE);
	  if( f != -1)
	  {
	    struct pike_string *s;
	    REF_MAKE_CONST_STRING(s, "string");
	    push_string(s);
	    apply_low(o, f, 1);
	    f=!UNSAFE_IS_ZERO(sp-1);
	    pop_stack();
	    if(f) return;
	  }
	}
	Pike_error("Cast failed, wanted string, got %s\n",
		   get_name_of_type(sp[-1].type));
      }
    }
    return;
	    
  case T_INT:
    sprintf(buf, "%"PRINTPIKEINT"d", sp[-1].u.integer);
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
      struct object *o = sp[-1].u.object;
      struct pike_string *s;
      int f = FIND_LFUN(o->prog->inherits[sp[-1].subtype].prog, LFUN_CAST);
      if(f == -1)
	Pike_error("No cast method in object.\n");
      push_string(describe_type(type));
      apply_low(o, f, 1);
      stack_pop_keep_top();
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
	      Pike_sp[-1].subtype = 0;
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
      struct object *o = sp[-1].u.object;
      int f = FIND_LFUN(o->prog->inherits[sp[-1].subtype].prog, LFUN__IS_TYPE);
      if( f != -1)
      {
	push_text(get_name_of_type(run_time_type));
	apply_low(o, f, 1);
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
	struct multiset *tmp=sp[-2].u.multiset;
	DECLARE_CYCLIC();
	
	if((m=(struct multiset *)BEGIN_CYCLIC(tmp,0)))
	{
	  ref_push_multiset(m);
	}else{
#ifdef PIKE_DEBUG
	  struct svalue *save_sp=sp+1;
#endif

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
	  struct mapping_data *md;
#ifdef PIKE_DEBUG
	  struct svalue *save_sp=sp+1;
#endif
	  push_mapping(m=allocate_mapping(m_sizeof(tmp)));
	  
	  SET_CYCLIC_RET(m);

	  md = tmp->data;
	  NEW_MAPPING_LOOP(md)
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

#define COMPARISON(ID,NAME,FUN)			\
PMOD_EXPORT void ID(INT32 args)				\
{						\
  int i;					\
  switch(args)					\
  {						\
    case 0: case 1:				\
      SIMPLE_TOO_FEW_ARGS_ERROR(NAME, 2); \
    case 2:					\
      i=FUN (sp-2,sp-1);			\
      pop_n_elems(2);				\
      push_int(i);				\
      break;					\
    default:					\
      for(i=1;i<args;i++)			\
        if(! ( FUN (sp-args+i-1, sp-args+i)))	\
          break;				\
      pop_n_elems(args);			\
      push_int(i==args);			\
  }						\
}

/*! @decl int(0..1) `!=(mixed arg1, mixed arg2, mixed ... extras)
 *!
 *!   Inequality test.
 *!
 *!   Every expression with the @expr{!=@} operator becomes a call to
 *!   this function, i.e. @expr{a!=b@} is the same as
 *!   @expr{predef::`!=(a,b)@}.
 *!
 *!   This is the inverse of @[`==()]; see that function for further
 *!   details.
 *!
 *! @returns
 *!   Returns @expr{1@} if the test is successful, @expr{0@}
 *!   otherwise.
 *!
 *! @seealso
 *!   @[`==()]
 */

PMOD_EXPORT void f_ne(INT32 args)
{
  f_eq(args);
  o_not();
}

/*! @decl int(0..1) `==(mixed arg1, mixed arg2, mixed ... extras)
 *!
 *!   Equality test.
 *!
 *!   Every expression with the @expr{==@} operator becomes a call to
 *!   this function, i.e. @expr{a==b@} is the same as
 *!   @expr{predef::`==(a,b)@}.
 *!
 *!   If more than two arguments are given, each argument is compared
 *!   with the following one as described below, and the test is
 *!   successful iff all comparisons are successful.
 *!
 *!   If the first argument is an object with an @[lfun::`==()], that
 *!   function is called with the second as argument, unless the
 *!   second argument is the same as the first argument. The test is
 *!   successful iff its result is nonzero (according to @[`!]).
 *!
 *!   Otherwise, if the second argument is an object with an
 *!   @[lfun::`==()], that function is called with the first as
 *!   argument, and the test is successful iff its result is nonzero
 *!   (according to @[`!]).
 *!
 *!   Otherwise, if the arguments are of different types, the test is
 *!   unsuccessful. Function pointers to programs are automatically
 *!   converted to program pointers if necessary, though.
 *!
 *!   Otherwise the test depends on the type of the arguments:
 *!   @mixed
 *!     @type int
 *!       Successful iff the two integers are numerically equal.
 *!     @type float
 *!       Successful iff the two floats are numerically equal or if
 *!       both are NaN.
 *!     @type string
 *!       Successful iff the two strings are identical, character for
 *!       character. (Since all strings are kept unique, this is
 *!       actually a test whether the arguments point to the same
 *!       string, and it therefore run in constant time.)
 *!     @type array|mapping|multiset|object|function|program|type
 *!       Successful iff the two arguments point to the same instance.
 *!   @endmixed
 *!
 *! @returns
 *!   Returns @expr{1@} if the test is successful, @expr{0@}
 *!   otherwise.
 *!
 *! @note
 *!   Floats and integers are not automatically converted to test
 *!   against each other, so e.g. @expr{0==0.0@} is false.
 *!
 *! @note
 *!   Programs are not automatically converted to types to be compared
 *!   type-wise.
 *!
 *! @seealso
 *!   @[`!()], @[`!=()]
 */
COMPARISON(f_eq,"`==", is_eq)

/*! @decl int(0..1) `<(mixed arg1, mixed arg2, mixed ... extras)
 *!
 *!   Less than test.
 *!
 *!   Every expression with the @expr{<@} operator becomes a call to
 *!   this function, i.e. @expr{a<b@} is the same as
 *!   @expr{predef::`<(a,b)@}.
 *!
 *! @returns
 *!   Returns @expr{1@} if the test is successful, @expr{0@}
 *!   otherwise.
 *!
 *! @seealso
 *!   @[`<=()], @[`>()], @[`>=()]
 */
COMPARISON(f_lt,"`<" , is_lt)

/*! @decl int(0..1) `<=(mixed arg1, mixed arg2, mixed ... extras)
 *!
 *!   Less than or equal test.
 *!
 *!   Every expression with the @expr{<=@} operator becomes a call to
 *!   this function, i.e. @expr{a<=b@} is the same as
 *!   @expr{predef::`<=(a,b)@}.
 *!
 *! @returns
 *!   Returns @expr{1@} if the test is successful, @expr{0@}
 *!   otherwise.
 *!
 *! @note
 *!   For total orders, e.g. integers, this is the inverse of @[`>()].
 *!
 *! @seealso
 *!   @[`<()], @[`>()], @[`>=()]
 */
COMPARISON(f_le,"`<=",is_le)

/*! @decl int(0..1) `>(mixed arg1, mixed arg2, mixed ... extras)
 *!
 *!   Greater than test.
 *!
 *!   Every expression with the @expr{>@} operator becomes a call to
 *!   this function, i.e. @expr{a>b@} is the same as
 *!   @expr{predef::`>(a,b)@}.
 *!
 *! @returns
 *!   Returns @expr{1@} if the arguments are strictly decreasing, and
 *!   @expr{0@} (zero) otherwise.
 *!
 *! @seealso
 *!   @[`<()], @[`<=()], @[`>=()]
 */
COMPARISON(f_gt,"`>" , is_gt)

/*! @decl int(0..1) `>=(mixed arg1, mixed arg2, mixed ... extras)
 *!
 *!   Greater than or equal test.
 *!
 *!   Every expression with the @expr{>=@} operator becomes a call to
 *!   this function, i.e. @expr{a>=b@} is the same as
 *!   @expr{predef::`>=(a,b)@}.
 *!
 *! @returns
 *!   Returns @expr{1@} if the test is successful, @expr{0@}
 *!   otherwise.
 *!
 *! @note
 *!   For total orders, e.g. integers, this is the inverse of @[`<()].
 *!
 *! @seealso
 *!   @[`<=()], @[`>()], @[`<()]
 */
COMPARISON(f_ge,"`>=",is_ge)


#define CALL_OPERATOR(OP, args) do {					\
    struct object *o_ = sp[-args].u.object;				\
    int i;								\
    if(!o_->prog)							\
      bad_arg_error(lfun_names[OP], sp-args, args, 1, "object", sp-args, \
		    "Called in destructed object.\n");			\
    if((i = FIND_LFUN(o_->prog->inherits[sp[-args].subtype].prog,	\
		      OP)) == -1)					\
      bad_arg_error(lfun_names[OP], sp-args, args, 1, "object", sp-args, \
		    "Operator not in object.\n");			\
    apply_low(o_, i, args-1);						\
    stack_pop_keep_top();						\
  } while (0)

/* Helper function for calling ``-operators.
 *
 * Assumes o is at Pike_sp[e - args].
 *
 * i is the resolved lfun to call.
 *
 * Returns the number of remaining elements on the stack.
 */
PMOD_EXPORT INT32 low_rop(struct object *o, int i, INT32 e, INT32 args)
{
  if (e == args-1) {
    /* The object is the last argument. */
    ONERROR err;
    Pike_sp--;
    SET_ONERROR(err, do_free_object, o);
    apply_low(o, i, e);
    CALL_AND_UNSET_ONERROR(err);
    return args - e;
  } else {
    /* Rotate the stack, so that the @[e] first elements come last.
     */
    struct svalue *tmp;
    if (e*2 < args) {
      tmp = xalloc(e*sizeof(struct svalue));
      memcpy(tmp, Pike_sp-args, e*sizeof(struct svalue));
      memmove(Pike_sp-args, (Pike_sp-args)+e,
	      (args-e)*sizeof(struct svalue));
      memcpy(Pike_sp-e, tmp, e*sizeof(struct svalue));
    } else {
      tmp = xalloc((args-e)*sizeof(struct svalue));
      memcpy(tmp, (Pike_sp-args)+e, (args-e)*sizeof(struct svalue));
      memmove(Pike_sp-e, Pike_sp-args, e*sizeof(struct svalue));
      memcpy(Pike_sp-args, tmp, (args-e)*sizeof(struct svalue));
    }
    free(tmp);
    /* Now the stack is:
     *
     * -args	object with the lfun.
     *  ...
     *  ...	other arguments
     *  ...
     *   -e	first argument.
     *  ...
     *   -1	last argument before the object.
     */
#ifdef PIKE_DEBUG
    if (Pike_sp[-args].type != T_OBJECT ||
	Pike_sp[-args].u.object != o ||
	!o->prog) {
      Pike_fatal("low_rop() Lost track of object.\n");
    }
#endif /* PIKE_DEBUG */
    apply_low(o, i, e);
    args -= e;
    /* Replace the object with the result. */
    assign_svalue(Pike_sp-(args+1), Pike_sp-1);
    pop_stack();
    return args;
  }
}

/*! @decl mixed `+(mixed arg)
 *! @decl mixed `+(object arg, mixed ... more)
 *! @decl int `+(int arg, int ... more)
 *! @decl float `+(float|int arg, float|int ... more)
 *! @decl string `+(string|float|int arg, string|float|int ... more)
 *! @decl array `+(array arg, array ... more)
 *! @decl mapping `+(mapping arg, mapping ... more)
 *! @decl multiset `+(multiset arg, multiset ... more)
 *!
 *!   Addition/concatenation.
 *!
 *!   Every expression with the @expr{+@} operator becomes a call to
 *!   this function, i.e. @expr{a+b@} is the same as
 *!   @expr{predef::`+(a,b)@}. Longer @expr{+@} expressions are
 *!   normally optimized to one call, so e.g. @expr{a+b+c@} becomes
 *!   @expr{predef::`+(a,b,c)@}.
 *!
 *! @returns
 *!   If there's a single argument, that argument is returned.
 *!
 *!   If @[arg] is an object with only one reference and an
 *!   @[lfun::`+=()], that function is called with the rest of the
 *!   arguments, and its result is returned.
 *!
 *!   Otherwise, if @[arg] is an object with an @[lfun::`+()], that
 *!   function is called with the rest of the arguments, and its
 *!   result is returned.
 *!
 *!   Otherwise, if any of the other arguments is an object that has
 *!   an @[lfun::``+()], the first such function is called with the
 *!   arguments leading up to it, and @[`+()] is then called
 *!   recursively with the result and the rest of the arguments.
 *!
 *!   Otherwise, if @[arg] is @[UNDEFINED] and the other arguments are
 *!   either arrays, mappings or multisets, the first argument is
 *!   ignored and the remaining are added together as described below.
 *!   This is useful primarily when appending to mapping values since
 *!   @expr{m[x] += ({foo})@} will work even if @expr{m[x]@} doesn't
 *!   exist yet.
 *!
 *!   Otherwise the result depends on the argument types:
 *!   @mixed
 *!     @type int|float
 *!       The result is the sum of all the arguments. It's a float if
 *!       any argument is a float.
 *!     @type string|int|float
 *!       If any argument is a string, all will be converted to
 *!       strings and concatenated in order to form the result.
 *!     @type array
 *!       The array arguments are concatened in order to form the
 *!       result.
 *!     @type mapping
 *!       The result is like @[arg] but extended with the entries from
 *!       the other arguments. If the same index (according to
 *!       @[hash_value] and @[`==]) occur in several arguments, the
 *!       value from the last one is used.
 *!     @type multiset
 *!       The result is like @[arg] but extended with the entries from
 *!       the other arguments. Subsequences with orderwise equal
 *!       indices (i.e. where @[`<] returns false) are concatenated
 *!       into the result in argument order.
 *!   @endmixed
 *!   The function is not destructive on the arguments - the result is
 *!   always a new instance.
 *!
 *! @note
 *!   In Pike 7.0 and earlier the addition order was unspecified.
 *!
 *!   The treatment of @[UNDEFINED] was new
 *!   in Pike 7.0.
 *!
 *! @seealso
 *!   @[`-()], @[lfun::`+()], @[lfun::``+()]
 */
PMOD_EXPORT void f_add(INT32 args)
{
  INT_TYPE e,size;
  TYPE_FIELD types;

 tail_recurse:
  types=0;
  for(e=-args;e<0;e++) types|=1<<sp[e].type;
    
  switch(types)
  {
  default:
    if(!args)
    {
      SIMPLE_TOO_FEW_ARGS_ERROR("`+", 1);
    }else{
      if(types & BIT_OBJECT)
      {
	struct object *o;
	struct program *p;
	int i;

	if (args == 1)
	  return;

	if(sp[-args].type == T_OBJECT && sp[-args].u.object->prog)
	{
	  /* The first argument is an object. */
	  o = sp[-args].u.object;
	  p = o->prog->inherits[sp[-args].subtype].prog;
	  if(o->refs==1 &&
	     (i = FIND_LFUN(p, LFUN_ADD_EQ)) != -1)
	  {
	    apply_low(o, i, args-1);
	    stack_pop_keep_top();
	    return;
	  }
	  if((i = FIND_LFUN(p, LFUN_ADD)) != -1)
	  {
	    apply_low(o, i, args-1);
	    free_svalue(sp-2);
	    sp[-2]=sp[-1];
	    sp--;
	    dmalloc_touch_svalue(sp);
	    return;
	  }
	}

	for(e=1;e<args;e++)
	{
	  if(sp[e-args].type == T_OBJECT &&
	     (p = (o = sp[e-args].u.object)->prog) &&
	     (i = FIND_LFUN(p->inherits[sp[e-args].subtype].prog,
			    LFUN_RADD)) != -1)
	  {
	    /* There's an object with a lfun::``+() at argument @[e]. */
	    if ((args = low_rop(o, i, e, args)) > 1)
	      goto tail_recurse;
	  }
	}
      }
    }

    switch(sp[-args].type)
    {
      case T_PROGRAM:
      case T_FUNCTION:
	SIMPLE_BAD_ARG_ERROR("`+", 1,
			     "string|object|int|float|array|mapping|multiset");
    }
    bad_arg_error("`+", sp-args, args, 1,
		  "string|object|int|float|array|mapping|multiset", sp-args,
		  "Incompatible types\n");
    return; /* compiler hint */

  case BIT_STRING:
  {
    struct pike_string *r;
    PCHARP buf;
    ptrdiff_t tmp;
    int max_shift=0;

    if(args==1) return;

    size=0;
    for(e=-args;e<0;e++)
    {
      size+=sp[e].u.string->len;
      if(sp[e].u.string->size_shift > max_shift)
	max_shift=sp[e].u.string->size_shift;
    }

    if(size == sp[-args].u.string->len)
    {
      pop_n_elems(args-1);
      return;
    }
    
    tmp=sp[-args].u.string->len;
    r=new_realloc_shared_string(sp[-args].u.string,size,max_shift);
    sp[-args].type=T_INT;
    buf=MKPCHARP_STR_OFF(r,tmp);
    for(e=-args+1;e<0;e++)
    {
      pike_string_cpy(buf,sp[e].u.string);
      INC_PCHARP(buf,sp[e].u.string->len);
    }
    sp[-args].u.string=low_end_shared_string(r);
    sp[-args].type=T_STRING;
    for(e=-args+1;e<0;e++) free_string(sp[e].u.string);
    sp-=args-1;

    break;
  }

  case BIT_STRING | BIT_INT:
  case BIT_STRING | BIT_FLOAT:
  case BIT_STRING | BIT_FLOAT | BIT_INT:
  {
    struct pike_string *r;
    PCHARP buf;
    char buffer[50];
    int max_shift=0;

    if ((sp[-args].type != T_STRING) && (sp[1-args].type != T_STRING)) {
      struct svalue *save_sp = sp;
      /* We need to perform a normal addition first.
       */
      for (e=-args; e < 0; e++) {
	if (save_sp[e].type == T_STRING)
	  break;
	*(sp++) = save_sp[e];
	dmalloc_touch_svalue(Pike_sp-1);
      }
      /* Perform the addition. */
      f_add(args+e);
      dmalloc_touch_svalue(Pike_sp-1);
      save_sp[--e] = *(--sp);
#ifdef PIKE_DEBUG
      if (sp != save_sp) {
	Pike_fatal("f_add(): Lost track of stack %p != %p\n", sp, save_sp);
      }
#endif /* PIKE_DEBUG */
      /* Perform the rest of the addition. */
      f_add(-e);
#ifdef PIKE_DEBUG
      if (sp != save_sp + 1 + e) {
	Pike_fatal("f_add(): Lost track of stack (2) %p != %p\n",
	      sp, save_sp + 1 + e);
      }
#endif /* PIKE_DEBUG */
      /* Adjust the stack. */
      save_sp[-args] = sp[-1];
      sp = save_sp + 1 - args;
      return;
    } else {
      e = -args;
    }
      

    size=0;
    for(e=-args;e<0;e++)
    {
      switch(sp[e].type)
      {
      case T_STRING:
	size+=sp[e].u.string->len;
	if(sp[e].u.string->size_shift > max_shift)
	  max_shift=sp[e].u.string->size_shift;
	break;

      case T_INT:
	size+=14;
	break;

      case T_FLOAT:
	size+=22;
	break;
      }
    }

    r=begin_wide_shared_string(size,max_shift);
    buf=MKPCHARP_STR(r);
    size=0;
    
    for(e=-args;e<0;e++)
    {
      switch(sp[e].type)
      {
      case T_STRING:
	pike_string_cpy(buf,sp[e].u.string);
	INC_PCHARP(buf,sp[e].u.string->len);
	break;

      case T_INT:
	sprintf(buffer,"%"PRINTPIKEINT"d",sp[e].u.integer);
	goto append_buffer;

      case T_FLOAT:
	sprintf(buffer,"%"PRINTPIKEFLOAT"f",sp[e].u.float_number);
      append_buffer:
	switch(max_shift)
	{
	  case 0:
	    convert_0_to_0((p_wchar0 *)buf.ptr,buffer,strlen(buffer));
	    break;

	  case 1:
	    convert_0_to_1((p_wchar1 *)buf.ptr,(p_wchar0 *)buffer,
			   strlen(buffer));
	    break;

	  case 2:
	    convert_0_to_2((p_wchar2 *)buf.ptr,(p_wchar0 *)buffer,
			   strlen(buffer));
	    break;

	}
	INC_PCHARP(buf,strlen(buffer));
      }
    }
    r = realloc_unlinked_string(r, SUBTRACT_PCHARP(buf, MKPCHARP_STR(r)));
    r = low_end_shared_string(r);
    pop_n_elems(args);
    push_string(r);
    break;
  }

  case BIT_INT:
#ifdef AUTO_BIGNUM
    size = 0;
    for(e = -args; e < 0; e++)
    {
      if(INT_TYPE_ADD_OVERFLOW(sp[e].u.integer, size))
      {
	convert_svalue_to_bignum(sp-args);
	f_add(args);
	return;
      }
      else
      {
	size += sp[e].u.integer;
      }
    }
    sp-=args;
    push_int(size);
#else
    size=0;
    for(e=-args; e<0; e++) size+=sp[e].u.integer;
    sp-=args-1;
    sp[-1].u.integer=size;
#endif /* AUTO_BIGNUM */
    break;

  case BIT_FLOAT:
  {
    FLOAT_ARG_TYPE sum;
    sum=0.0;
    for(e=-args; e<0; e++) sum+=sp[e].u.float_number;
    sp-=args-1;
    sp[-1].u.float_number=sum;
    break;
  }

  case BIT_FLOAT|BIT_INT:
  {
    FLOAT_ARG_TYPE sum;
    sum=0.0;
    for(e=-args; e<0; e++)
    {
      if(sp[e].type==T_FLOAT)
      {
	sum+=sp[e].u.float_number;
      }else{
	sum+=(FLOAT_ARG_TYPE)sp[e].u.integer;
      }
    }
    sp-=args-1;
    sp[-1].type=T_FLOAT;
    sp[-1].u.float_number=sum;
    break;
  }

#define ADD_WITH_UNDEFINED(TYPE, T_TYPEID, ADD_FUNC, PUSH_FUNC) do {	\
    int e;								\
    if (sp[-args].type == T_INT) {					\
      if(IS_UNDEFINED(sp-args))						\
      {									\
	struct TYPE *x;							\
									\
	for(e=1;e<args;e++)						\
	  if(sp[e-args].type != T_TYPEID)				\
	    SIMPLE_ARG_TYPE_ERROR("`+", e+1, #TYPE);			\
									\
	x = ADD_FUNC(sp-args+1,args-1);					\
	pop_n_elems(args);						\
	PUSH_FUNC(x);							\
	return;								\
      }									\
      									\
      for(e=1;e<args;e++)						\
	if (sp[e-args].type != T_INT)					\
	  SIMPLE_ARG_TYPE_ERROR("`+", e+1, "int");			\
    }									\
									\
    else {								\
      for(e=1;e<args;e++)						\
	if (sp[e-args].type != T_TYPEID)				\
	  SIMPLE_ARG_TYPE_ERROR("`+", e+1, #TYPE);			\
    }									\
									\
    DO_IF_DEBUG (Pike_fatal ("Shouldn't be reached.\n"));		\
  } while (0)

#define ADD(TYPE, ADD_FUNC, PUSH_FUNC) do {				\
    struct TYPE *x = ADD_FUNC (sp - args, args);			\
    pop_n_elems (args);							\
    PUSH_FUNC (x);							\
    return;								\
  } while (0)

  case BIT_ARRAY|BIT_INT:
    ADD_WITH_UNDEFINED (array, T_ARRAY, add_arrays, push_array);
      
  case BIT_ARRAY:
    ADD (array, add_arrays, push_array);

  case BIT_MAPPING|BIT_INT:
    ADD_WITH_UNDEFINED (mapping, T_MAPPING, add_mappings, push_mapping);

  case BIT_MAPPING:
    ADD (mapping, add_mappings, push_mapping);

  case BIT_MULTISET|BIT_INT:
    ADD_WITH_UNDEFINED (multiset, T_MULTISET, add_multisets, push_multiset);

  case BIT_MULTISET:
    ADD (multiset, add_multisets, push_multiset);

#undef ADD_WITH_UNDEFINED
#undef ADD
  }
}

static int generate_sum(node *n)
{
  node **first_arg, **second_arg, **third_arg;
  int num_args;
  switch(count_args(CDR(n)))
  {
  case 0: return 0;

  case 1:
    do_docode(CDR(n),0);
    return 1;

  case 2:
    first_arg=my_get_arg(&_CDR(n), 0);
    second_arg=my_get_arg(&_CDR(n), 1);
    
    do_docode(CDR(n),DO_NOT_COPY_TOPLEVEL);
    if(first_arg[0]->type == float_type_string &&
       second_arg[0]->type == float_type_string)
    {
      emit0(F_ADD_FLOATS);
    }
    else if(first_arg[0]->type && second_arg[0]->type &&
	    pike_types_le(first_arg[0]->type, int_type_string) &&
	    pike_types_le(second_arg[0]->type, int_type_string))
    {
      emit0(F_ADD_INTS);
    }
    else
    {
      emit0(F_ADD);
    }
    return 1;

  case 3:
    first_arg = my_get_arg(&_CDR(n), 0);
    second_arg = my_get_arg(&_CDR(n), 1);
    third_arg = my_get_arg(&_CDR(n), 2);
    
    if(first_arg[0]->type == float_type_string &&
       second_arg[0]->type == float_type_string)
    {
      do_docode(*first_arg, 0);
      do_docode(*second_arg, 0);
      emit0(F_ADD_FLOATS);
      if (third_arg[0]->type == float_type_string) {
	do_docode(*third_arg, 0);
	emit0(F_ADD_FLOATS);
	return 1;
      }
    }
    else if(first_arg[0]->type && second_arg[0]->type &&
	    pike_types_le(first_arg[0]->type, int_type_string) &&
	    pike_types_le(second_arg[0]->type, int_type_string))
    {
      do_docode(*first_arg, 0);
      do_docode(*second_arg, 0);
      emit0(F_ADD_INTS);
      if (third_arg[0]->type &&
	  pike_types_le(third_arg[0]->type, int_type_string)) {
	do_docode(*third_arg, 0);
	emit0(F_ADD_INTS);
	return 1;
      }
    }
    else
    {
      return 0;
    }
    do_docode(*third_arg, 0);
    emit0(F_ADD);
    
    return 1;

  default:
    return 0;
  }
}

static node *optimize_eq(node *n)
{
  node **first_arg, **second_arg, *ret;
  if(count_args(CDR(n))==2)
  {
    first_arg=my_get_arg(&_CDR(n), 0);
    second_arg=my_get_arg(&_CDR(n), 1);

#ifdef PIKE_DEBUG
    if(!first_arg || !second_arg)
      Pike_fatal("Couldn't find argument!\n");
#endif
    if(node_is_false(*first_arg) && !node_may_overload(*second_arg,LFUN_EQ))
    {
      ret=*second_arg;
      ADD_NODE_REF(*second_arg);
      return mkopernode("`!",ret,0);
    }

    if(node_is_false(*second_arg)  && !node_may_overload(*first_arg,LFUN_EQ))
    {
      ret=*first_arg;
      ADD_NODE_REF(*first_arg);
      return mkopernode("`!",ret,0);
    }

    if (((*second_arg)->token == F_CONSTANT) &&
	((*second_arg)->u.sval.type == T_STRING) &&
	((*first_arg)->token == F_RANGE)) {
      node *low = CADR (*first_arg), *high = CDDR (*first_arg);
      INT_TYPE c;
      if ((low->token == F_RANGE_OPEN ||
	   (low->token == F_RANGE_FROM_BEG &&
	    (CAR (low)->token == F_CONSTANT) &&
	    (CAR (low)->u.sval.type == T_INT) &&
	    (!(CAR (low)->u.sval.u.integer)))) &&
	  (high->token == F_RANGE_OPEN ||
	   (high->token == F_RANGE_FROM_BEG &&
	    (CAR (high)->token == F_CONSTANT) &&
	    (CAR (high)->u.sval.type == T_INT) &&
	    (c = CAR (high)->u.sval.u.integer, 1)))) {
	/* str[..c] == "foo" or str[0..c] == "foo" or
	 * str[..] == "foo" or str[0..] == "foo" */

	if (high->token == F_RANGE_OPEN ||
	    (*second_arg)->u.sval.u.string->len <= c) {
	  /* str[..4] == "foo"
	   *   ==>
	   * str == "foo"
	   */
	  /* FIXME: Warn? */
	  ADD_NODE_REF2(CAR(*first_arg),
	  ADD_NODE_REF2(*second_arg,
	    ret = mkopernode("`==", CAR(*first_arg), *second_arg);
	  ));
	  return ret;
	} else if ((*second_arg)->u.sval.u.string->len == c+1) {
	  /* str[..2] == "foo"
	   *   ==>
	   * has_prefix(str, "foo");
	   */
	  ADD_NODE_REF2(CAR(*first_arg),
	  ADD_NODE_REF2(*second_arg,
	    ret = mkopernode("has_prefix", CAR(*first_arg), *second_arg);
	  ));
	  return ret;
	} else {
	  /* str[..1] == "foo"
	   *   ==>
	   * (str, 0)
	   */
	  /* FIXME: Warn? */
	  ADD_NODE_REF2(CAR(*first_arg),
	    ret = mknode(F_COMMA_EXPR, CAR(*first_arg), mkintnode(0));
	  );
	  return ret;
	}
      }
    }
  }
  return 0;
}

static node *optimize_not(node *n)
{
  node **first_arg, **more_args;

  if(count_args(CDR(n))==1)
  {
    first_arg=my_get_arg(&_CDR(n), 0);
#ifdef PIKE_DEBUG
    if(!first_arg)
      Pike_fatal("Couldn't find argument!\n");
#endif
    if(node_is_true(*first_arg))  return mkintnode(0);
    if(node_is_false(*first_arg)) return mkintnode(1);

#define TMP_OPT(X,Y) do {			\
    if((more_args=is_call_to(*first_arg, X)))	\
    {						\
      node *tmp=*more_args;			\
      if(count_args(*more_args) > 2) return 0;  \
      ADD_NODE_REF(*more_args);			\
      return mkopernode(Y,tmp,0);		\
    } } while(0)

    TMP_OPT(f_eq, "`!=");
    TMP_OPT(f_ne, "`==");
#if 0
    /* The following only work on total orders. We can't assume that. */
    TMP_OPT(f_lt, "`>=");
    TMP_OPT(f_gt, "`<=");
    TMP_OPT(f_le, "`>");
    TMP_OPT(f_ge, "`<");
#endif
#undef TMP_OPT
    if((more_args = is_call_to(*first_arg, f_search)) &&
       (count_args(*more_args) == 2)) {
      node *search_args = *more_args;
      if ((search_args->token == F_ARG_LIST) &&
	  CAR(search_args) &&
	  (CAR(search_args)->type == string_type_string) &&
	  CDR(search_args) &&
	  (CDR(search_args)->type == string_type_string)) {
	/* !search(string a, string b)  =>  has_prefix(a, b) */
	ADD_NODE_REF(*more_args);
	return mkefuncallnode("has_prefix", search_args);
      }
    }
  }

  return 0;
}

static node *may_have_side_effects(node *n)
{
  node **arg;
  int argno;
  for (argno = 0; (arg = my_get_arg(&_CDR(n), argno)); argno++) {
    if (match_types(object_type_string, (*arg)->type)) {
      n->node_info |= OPT_SIDE_EFFECT;
      n->tree_info |= OPT_SIDE_EFFECT;
      return NULL;
    }
  }
  return NULL;
}

static node *optimize_binary(node *n)
{
  node **first_arg, **second_arg, *ret;
  if(count_args(CDR(n))==2)
  {
    first_arg=my_get_arg(&_CDR(n), 0);
    second_arg=my_get_arg(&_CDR(n), 1);

#ifdef PIKE_DEBUG
    if(!first_arg || !second_arg)
      Pike_fatal("Couldn't find argument!\n");
#endif

    if((*second_arg)->type == (*first_arg)->type &&
       compile_type_to_runtime_type((*second_arg)->type) != T_MIXED)
    {
      if((*first_arg)->token == F_APPLY &&
	 CAR(*first_arg)->token == F_CONSTANT &&
	 is_eq(& CAR(*first_arg)->u.sval, & CAR(n)->u.sval))
      {
	/* binop(binop(@a_args), b)  ==>  binop(@a_args, b) */
	ADD_NODE_REF2(CAR(n),
	ADD_NODE_REF2(CDR(*first_arg),
	ADD_NODE_REF2(*second_arg,
		      ret = mknode(F_APPLY,
				   CAR(n),
				   mknode(F_ARG_LIST,
					  CDR(*first_arg),
					  *second_arg))
	)));
	return ret;
      }
      
      if((*second_arg)->token == F_APPLY &&
	 CAR(*second_arg)->token == F_CONSTANT &&
	 is_eq(& CAR(*second_arg)->u.sval, & CAR(n)->u.sval))
      {
	/* binop(a, binop(@b_args))  ==>  binop(a, @b_args) */
	ADD_NODE_REF2(CAR(n),
	ADD_NODE_REF2(*first_arg,
	ADD_NODE_REF2(CDR(*second_arg),
		      ret = mknode(F_APPLY,
				   CAR(n),
				   mknode(F_ARG_LIST,
					  *first_arg,
					  CDR(*second_arg)))
	)));
	return ret;
      }
    }
  }
  return 0;
}


static int generate_comparison(node *n)
{
  if(count_args(CDR(n))==2)
  {
    if(do_docode(CDR(n),DO_NOT_COPY) != 2)
      Pike_fatal("Count args was wrong in generate_comparison.\n");

    if(CAR(n)->u.sval.u.efun->function == f_eq)
      emit0(F_EQ);
    else if(CAR(n)->u.sval.u.efun->function == f_ne)
      emit0(F_NE);
    else if(CAR(n)->u.sval.u.efun->function == f_lt)
      emit0(F_LT);
    else if(CAR(n)->u.sval.u.efun->function == f_le)
      emit0(F_LE);
    else if(CAR(n)->u.sval.u.efun->function == f_gt)
      emit0(F_GT);
    else if(CAR(n)->u.sval.u.efun->function == f_ge)
      emit0(F_GE);
    else
      Pike_fatal("Couldn't generate comparison!\n");
    return 1;
  }
  return 0;
}

static int float_promote(void)
{
  if(sp[-2].type==T_INT && sp[-1].type==T_FLOAT)
  {
    sp[-2].u.float_number=(FLOAT_TYPE)sp[-2].u.integer;
    sp[-2].type=T_FLOAT;
    return 1;
  }
  else if(sp[-1].type==T_INT && sp[-2].type==T_FLOAT)
  {
    sp[-1].u.float_number=(FLOAT_TYPE)sp[-1].u.integer;
    sp[-1].type=T_FLOAT;
    return 1;
  }

#ifdef AUTO_BIGNUM
  if(is_bignum_object_in_svalue(sp-2) && sp[-1].type==T_FLOAT)
  {
    stack_swap();
    ref_push_type_value(float_type_string);
    stack_swap();
    f_cast();
    stack_swap();
    return 1;
  }
  else if(is_bignum_object_in_svalue(sp-1) && sp[-2].type==T_FLOAT)
  {
    ref_push_type_value(float_type_string);
    stack_swap();
    f_cast();
    return 1;
  }
#endif
  return 0;
}

static int call_lfun(int left, int right)
{
  struct object *o;
  struct program *p;
  int i;

  if(sp[-2].type == T_OBJECT &&
     (p = (o = sp[-2].u.object)->prog) &&
     (i = FIND_LFUN(p->inherits[sp[-2].subtype].prog, left)) != -1)
  {
    apply_low(o, i, 1);
    free_svalue(sp-2);
    sp[-2]=sp[-1];
    sp--;
    dmalloc_touch_svalue(sp);
    return 1;
  }

  if(sp[-1].type == T_OBJECT &&
     (p = (o = sp[-1].u.object)->prog) &&
     (i = FIND_LFUN(p->inherits[sp[-1].subtype].prog, right)) != -1)
  {
    push_svalue(sp-2);
    apply_low(o, i, 1);
    free_svalue(sp-3);
    sp[-3]=sp[-1];
    sp--;
    dmalloc_touch_svalue(sp);
    pop_stack();
    return 1;
  }

  return 0;
}

struct mapping *merge_mapping_array_ordered(struct mapping *a, 
					    struct array *b, INT32 op);
struct mapping *merge_mapping_array_unordered(struct mapping *a, 
					      struct array *b, INT32 op);

PMOD_EXPORT void o_subtract(void)
{
  if (sp[-2].type != sp[-1].type && !float_promote())
  {
    if(call_lfun(LFUN_SUBTRACT, LFUN_RSUBTRACT))
      return;

    if (sp[-2].type==T_MAPPING)
       switch (sp[-1].type)
       {
	  case T_ARRAY:
	  {
	     struct mapping *m;

	     m=merge_mapping_array_unordered(sp[-2].u.mapping,
					     sp[-1].u.array,
					     PIKE_ARRAY_OP_SUB);
	     pop_n_elems(2);
	     push_mapping(m);
	     return;
	  }
	  case T_MULTISET:
	  {
	     struct mapping *m;

	     int got_cmp_less = !!multiset_get_cmp_less (sp[-1].u.multiset);
	     struct array *ind = multiset_indices (sp[-1].u.multiset);
	     pop_stack();
	     push_array (ind);
	     if (got_cmp_less)
	       m=merge_mapping_array_unordered(sp[-2].u.mapping,
					       sp[-1].u.array,
					       PIKE_ARRAY_OP_SUB);
	     else
	       m=merge_mapping_array_ordered(sp[-2].u.mapping,
					     sp[-1].u.array,
					     PIKE_ARRAY_OP_SUB);
	     pop_n_elems(2);
	     push_mapping(m);
	     return;
	  }
       }

    bad_arg_error("`-", sp-2, 2, 2, get_name_of_type(sp[-2].type),
		  sp-1, "Subtract on different types.\n");
  }

  switch(sp[-2].type)
  {
  case T_OBJECT:
    CALL_OPERATOR(LFUN_SUBTRACT,2);
    break;

  case T_ARRAY:
  {
    struct array *a;

    check_array_for_destruct(sp[-2].u.array);
    check_array_for_destruct(sp[-1].u.array);
    a = subtract_arrays(sp[-2].u.array, sp[-1].u.array);
    pop_n_elems(2);
    push_array(a);
    return;
  }

  case T_MAPPING:
  {
    struct mapping *m;
    m=merge_mappings(sp[-2].u.mapping, sp[-1].u.mapping,PIKE_ARRAY_OP_SUB);
    pop_n_elems(2);
    push_mapping(m);
    return;
  }

  case T_MULTISET:
  {
    struct multiset *l;
    l=merge_multisets(sp[-2].u.multiset, sp[-1].u.multiset, PIKE_ARRAY_OP_SUB);
    pop_n_elems(2);
    push_multiset(l);
    return;
  }

  case T_FLOAT:
    sp--;
    sp[-1].u.float_number -= sp[0].u.float_number;
    return;

  case T_INT:
#ifdef AUTO_BIGNUM
    if(INT_TYPE_SUB_OVERFLOW(sp[-2].u.integer, sp[-1].u.integer))
    {
      convert_stack_top_to_bignum();
      f_minus(2);
      return;
    }
#endif /* AUTO_BIGNUM */
    sp--;
    sp[-1].u.integer -= sp[0].u.integer;
    return;

  case T_STRING:
  {
    struct pike_string *s,*ret;
    s=make_shared_string("");
    ret=string_replace(sp[-2].u.string,sp[-1].u.string,s);
    free_string(sp[-2].u.string);
    free_string(sp[-1].u.string);
    free_string(s);
    sp[-2].u.string=ret;
    sp--;
    return;
  }

  /* FIXME: Support types? */

  default:
    {
      int args = 2;
      SIMPLE_BAD_ARG_ERROR("`-", 1,
			   "int|float|string|mapping|multiset|array|object");
    }
  }
}

/*! @decl mixed `-(mixed arg1)
 *! @decl mixed `-(mixed arg1, mixed arg2, mixed ... extras)
 *! @decl mixed `-(object arg1, mixed arg2)
 *! @decl mixed `-(mixed arg1, object arg2)
 *! @decl int `-(int arg1, int arg2)
 *! @decl float `-(float arg1, int|float arg2)
 *! @decl float `-(int|float arg1, float arg2)
 *! @decl string `-(string arg1, string arg2)
 *! @decl array `-(array arg1, array arg2)
 *! @decl mapping `-(mapping arg1, array arg2)
 *! @decl mapping `-(mapping arg1, mapping arg2)
 *! @decl mapping `-(mapping arg1, multiset arg2)
 *! @decl multiset `-(multiset arg1, multiset arg2)
 *!
 *!   Negation/subtraction/set difference.
 *!
 *!   Every expression with the @expr{-@} operator becomes a call to
 *!   this function, i.e. @expr{-a@} is the same as
 *!   @expr{predef::`-(a)@} and @expr{a-b@} is the same as
 *!   @expr{predef::`-(a,b)@}. Longer @expr{-@} expressions are
 *!   normally optimized to one call, so e.g. @expr{a-b-c@} becomes
 *!   @expr{predef::`-(a,b,c)@}.
 *!
 *! @returns
 *!   If there's a single argument, that argument is returned negated.
 *!   If @[arg1] is an object with an @[lfun::`-()], that function is
 *!   called without arguments, and its result is returned.
 *!
 *!   If there are more than two arguments the result is:
 *!   @expr{`-(`-(@[arg1], @[arg2]), @@@[extras])@}.
 *!
 *!   Otherwise, if @[arg1] is an object with an @[lfun::`-()], that
 *!   function is called with @[arg2] as argument, and its result is
 *!   returned.
 *!
 *!   Otherwise, if @[arg2] is an object with an @[lfun::``-()], that
 *!   function is called with @[arg1] as argument, and its result is
 *!   returned.
 *!
 *!   Otherwise the result depends on the argument types:
 *!   @mixed arg1
 *!   	@type int|float
 *!   	  The result is @expr{@[arg1] - @[arg2]@}, and is a float if
 *!   	  either @[arg1] or @[arg2] is a float.
 *!   	@type string
 *!   	  The result is @[arg1] with all nonoverlapping occurrences of
 *!   	  the substring @[arg2] removed. In cases with two overlapping
 *!   	  occurrences, the leftmost is removed.
 *!   	@type array|mapping|multiset
 *!   	  The result is like @[arg1] but without the elements/indices
 *!   	  that match any in @[arg2] (according to @[`==] and, in the
 *!   	  case of mappings, @[hash_value]).
 *!   @endmixed
 *!   The function is not destructive on the arguments - the result is
 *!   always a new instance.
 *!
 *! @note
 *!   In Pike 7.0 and earlier the subtraction order was unspecified.
 *!
 *! @seealso
 *!   @[`+()]
 */
PMOD_EXPORT void f_minus(INT32 args)
{
  switch(args)
  {
    case 0: SIMPLE_TOO_FEW_ARGS_ERROR("`-", 1);
    case 1: o_negate(); break;
    case 2: o_subtract(); break;
    default:
    {
      INT32 e;
      struct svalue *s=sp-args;
      push_svalue(s);
      for(e=1;e<args;e++)
      {
	push_svalue(s+e);
	o_subtract();
      }
      assign_svalue(s,sp-1);
      pop_n_elems(sp-s-1);
    }
  }
}

static int generate_minus(node *n)
{
  switch(count_args(CDR(n)))
  {
  case 1:
    do_docode(CDR(n),DO_NOT_COPY);
    emit0(F_NEGATE);
    return 1;

  case 2:
    do_docode(CDR(n),DO_NOT_COPY_TOPLEVEL);
    emit0(F_SUBTRACT);
    return 1;
  }
  return 0;
}

PMOD_EXPORT void o_and(void)
{
  if(sp[-1].type != sp[-2].type)
  {
     if(call_lfun(LFUN_AND, LFUN_RAND)) 
	return;
     else if (((sp[-1].type == T_TYPE) || (sp[-1].type == T_PROGRAM) ||
	       (sp[-1].type == T_FUNCTION)) &&
	      ((sp[-2].type == T_TYPE) || (sp[-2].type == T_PROGRAM) ||
	       (sp[-2].type == T_FUNCTION))) 
     {
	if (sp[-2].type != T_TYPE) 
	{
	   struct program *p = program_from_svalue(sp - 2);
	   if (!p) {
	      int args = 2;
	      SIMPLE_BAD_ARG_ERROR("`&", 1, "type");
	   }
	   type_stack_mark();
	   push_object_type(0, p->id);
	   free_svalue(sp - 2);
	   sp[-2].u.type = pop_unfinished_type();
	   sp[-2].type = T_TYPE;
	}
	if (sp[-1].type != T_TYPE) 
	{
	   struct program *p = program_from_svalue(sp - 1);
	   if (!p) 
	   {
	      int args = 2;
	      SIMPLE_BAD_ARG_ERROR("`&", 2, "type");
	   }
	   type_stack_mark();
	   push_object_type(0, p->id);
	   free_svalue(sp - 1);
	   sp[-1].u.type = pop_unfinished_type();
	   sp[-1].type = T_TYPE;
	}
     } 
     else if (sp[-2].type==T_MAPPING)
	switch (sp[-1].type)
	{
	   case T_ARRAY:
	   {
	      struct mapping *m;

	      m=merge_mapping_array_unordered(sp[-2].u.mapping,
					      sp[-1].u.array,
					      PIKE_ARRAY_OP_AND);
	      pop_n_elems(2);
	      push_mapping(m);
	      return;
	   }
	   case T_MULTISET:
	   {
	      struct mapping *m;

	     int got_cmp_less = !!multiset_get_cmp_less (sp[-1].u.multiset);
	     struct array *ind = multiset_indices (sp[-1].u.multiset);
	     pop_stack();
	     push_array (ind);
	     if (got_cmp_less)
	       m=merge_mapping_array_unordered(sp[-2].u.mapping,
					       sp[-1].u.array,
					       PIKE_ARRAY_OP_AND);
	     else
	       m=merge_mapping_array_ordered(sp[-2].u.mapping,
					     sp[-1].u.array,
					     PIKE_ARRAY_OP_AND);
	      pop_n_elems(2);
	      push_mapping(m);
	      return;
	   }
	   default:
	   {
	      int args = 2;
	      SIMPLE_BAD_ARG_ERROR("`&", 2, "mapping");
	   }
	}
     else 
     {
	int args = 2;
	SIMPLE_BAD_ARG_ERROR("`&", 2, get_name_of_type(sp[-2].type));
     }
  }

  switch(sp[-2].type)
  {
  case T_OBJECT:
    CALL_OPERATOR(LFUN_AND,2);
    break;
    
  case T_INT:
    sp--;
    sp[-1].u.integer &= sp[0].u.integer;
    break;

  case T_MAPPING:
  {
    struct mapping *m;
    m=merge_mappings(sp[-2].u.mapping, sp[-1].u.mapping, PIKE_ARRAY_OP_AND);
    pop_n_elems(2);
    push_mapping(m);
    return;
  }

  case T_MULTISET:
  {
    struct multiset *l;
    l=merge_multisets(sp[-2].u.multiset, sp[-1].u.multiset, PIKE_ARRAY_OP_AND);
    pop_n_elems(2);
    push_multiset(l);
    return;
  }
    
  case T_ARRAY:
  {
    struct array *a;
    a=and_arrays(sp[-2].u.array, sp[-1].u.array);
    pop_n_elems(2);
    push_array(a);
    return;
  }

  case T_TYPE:
  {
    struct pike_type *t;
    t = and_pike_types(sp[-2].u.type, sp[-1].u.type);
    pop_n_elems(2);
    push_type_value(t);
    return;
  }

  case T_FUNCTION:
  case T_PROGRAM:
  {
    struct program *p;
    struct pike_type *a;
    struct pike_type *b;
    struct pike_type *t;

    p = program_from_svalue(sp - 2);
    if (!p) {
      int args = 2;
      SIMPLE_BAD_ARG_ERROR("`&", 1, "type");
    }    
    type_stack_mark();
    push_object_type(0, p->id);
    a = pop_unfinished_type();

    p = program_from_svalue(sp - 1);
    if (!p) {
      int args = 2;
      SIMPLE_BAD_ARG_ERROR("`&", 2, "type");
    }    
    type_stack_mark();
    push_object_type(0, p->id);
    b = pop_unfinished_type();

    t = and_pike_types(a, b);

    pop_n_elems(2);
    push_type_value(t);
    free_type(a);
    free_type(b);
    return;
  }

#define STRING_BITOP(OP,STROP)						  \
  case T_STRING:							  \
  {									  \
    struct pike_string *s;						  \
    ptrdiff_t len, i;							  \
									  \
    len = sp[-2].u.string->len;						  \
    if (len != sp[-1].u.string->len)					  \
      PIKE_ERROR("`" #OP, "Bitwise "STROP				  \
		 " on strings of different lengths.\n", sp, 2);		  \
    if(!sp[-2].u.string->size_shift && !sp[-1].u.string->size_shift)	  \
    {									  \
      s = begin_shared_string(len);					  \
      for (i=0; i<len; i++)						  \
	s->str[i] = sp[-2].u.string->str[i] OP sp[-1].u.string->str[i];	  \
    }else{								  \
      s = begin_wide_shared_string(len,					  \
				   MAXIMUM(sp[-2].u.string->size_shift,	  \
					   sp[-1].u.string->size_shift)); \
      for (i=0; i<len; i++)						  \
	low_set_index(s,i,index_shared_string(sp[-2].u.string,i) OP 	  \
		      index_shared_string(sp[-1].u.string,i));		  \
    }									  \
    pop_n_elems(2);							  \
    push_string(end_shared_string(s));					  \
    return;								  \
  }

  STRING_BITOP(&,"AND")

  default:
    PIKE_ERROR("`&", "Bitwise AND on illegal type.\n", sp, 2);
  }
}

/* This function is used to speed up or/xor/and on
 * arrays multisets and mappings. This is done by
 * calling the operator for each pair of arguments
 * first, then recursively doing the same on the
 * results until only one value remains.
 */
static void r_speedup(INT32 args, void (*func)(void))
{
  struct svalue tmp;
  ONERROR err;

  switch(args)
  {
    case 3: func();
    case 2: func();
    case 1: return;

    default:
      r_speedup((args+1)>>1,func);
      dmalloc_touch_svalue(Pike_sp-1);
      tmp=*--sp;
      SET_ONERROR(err,do_free_svalue,&tmp);
      r_speedup(args>>1,func);
      UNSET_ONERROR(err);
      sp++[0]=tmp;
      func();
  }
}
static void speedup(INT32 args, void (*func)(void))
{
  switch(sp[-args].type)
  {
    /* Binary balanced tree method for types where
     * a op b may or may not be equal to b op a
     */
    case T_ARRAY:
    case T_MAPPING:
      r_speedup(args,func);
      return;

    default:
      while(--args > 0) func();
  }
}

/*! @decl mixed `&(mixed arg1)
 *! @decl mixed `&(mixed arg1, mixed arg2, mixed ... extras)
 *! @decl mixed `&(object arg1, mixed arg2)
 *! @decl mixed `&(mixed arg1, object arg2)
 *! @decl int `&(int arg1, int arg2)
 *! @decl string `&(string arg1, string arg2)
 *! @decl array `&(array arg1, array arg2)
 *! @decl mapping `&(mapping arg1, mapping arg2)
 *! @decl mapping `&(mapping arg1, array arg2)
 *! @decl mapping `&(mapping arg1, multiset arg2)
 *! @decl multiset `&(multiset arg1, multiset arg2)
 *! @decl type `&(type|program arg1, type|program arg2)
 *!
 *!   Bitwise and/intersection.
 *!
 *!   Every expression with the @expr{&@} operator becomes a call to
 *!   this function, i.e. @expr{a&b@} is the same as
 *!   @expr{predef::`&(a,b)@}.
 *!
 *! @returns
 *!   If there's a single argument, that argument is returned.
 *!
 *!   If there are more than two arguments the result is:
 *!   @expr{`&(`&(@[arg1], @[arg2]), @@@[extras])@}.
 *!
 *!   Otherwise, if @[arg1] is an object with an @[lfun::`&()], that
 *!   function is called with @[arg2] as argument, and its result is
 *!   returned.
 *!
 *!   Otherwise, if @[arg2] is an object with an @[lfun::``&()], that
 *!   function is called with @[arg1] as argument, and its result is
 *!   returned.
 *!
 *!   Otherwise the result depends on the argument types:
 *!   @mixed arg1
 *!   	@type int
 *!   	  Bitwise and of @[arg1] and @[arg2].
 *!   	@type string
 *!   	  The result is a string where each character is the bitwise
 *!   	  and of the characters in the same position in @[arg1] and
 *!   	  @[arg2]. The arguments must be strings of the same length.
 *!   	@type array|mapping|multiset
 *!   	  The result is like @[arg1] but only with the
 *!   	  elements/indices that match any in @[arg2] (according to
 *!   	  @[`==] and, in the case of mappings, @[hash_value]).
 *!   	@type type|program
 *!   	  Type intersection of @[arg1] and @[arg2].
 *!   @endmixed
 *!   The function is not destructive on the arguments - the result is
 *!   always a new instance.
 *!
 *! @seealso
 *!   @[`|()], @[lfun::`&()], @[lfun::``&()]
 */
PMOD_EXPORT void f_and(INT32 args)
{
  switch(args)
  {
  case 0: SIMPLE_TOO_FEW_ARGS_ERROR("`&", 1);
  case 1: return;
  case 2: o_and(); return;
  default:
    if(sp[-args].type == T_OBJECT)
    {
      CALL_OPERATOR(LFUN_AND, args);
    }else{
      speedup(args, o_and);
    }
  }
}

static int generate_and(node *n)
{
  switch(count_args(CDR(n)))
  {
  case 1:
    do_docode(CDR(n),0);
    return 1;

  case 2:
    do_docode(CDR(n),0);
    emit0(F_AND);
    return 1;

  default:
    return 0;
  }
}

PMOD_EXPORT void o_or(void)
{
  if(sp[-1].type != sp[-2].type)
  {
    if(call_lfun(LFUN_OR, LFUN_ROR)) {
      return;
    } else if (((sp[-1].type == T_TYPE) || (sp[-1].type == T_PROGRAM) ||
		(sp[-1].type == T_FUNCTION)) &&
	       ((sp[-2].type == T_TYPE) || (sp[-2].type == T_PROGRAM) ||
		(sp[-2].type == T_FUNCTION))) {
      if (sp[-2].type != T_TYPE) {
	struct program *p = program_from_svalue(sp - 2);
	if (!p) {
	  int args = 2;
	  SIMPLE_BAD_ARG_ERROR("`|", 1, "type");
	}
	type_stack_mark();
	push_object_type(0, p->id);
	free_svalue(sp - 2);
	sp[-2].u.type = pop_unfinished_type();
	sp[-2].type = T_TYPE;
      }
      if (sp[-1].type != T_TYPE) {
	struct program *p = program_from_svalue(sp - 1);
	if (!p) {
	  int args = 2;
	  SIMPLE_BAD_ARG_ERROR("`|", 2, "type");
	}
	type_stack_mark();
	push_object_type(0, p->id);
	free_svalue(sp - 1);
	sp[-1].u.type = pop_unfinished_type();
	sp[-1].type = T_TYPE;
      }
    } else {
      int args = 2;
      SIMPLE_BAD_ARG_ERROR("`|", 2, get_name_of_type(sp[-2].type));
    }
  }

  switch(sp[-2].type)
  {
  case T_OBJECT:
    CALL_OPERATOR(LFUN_OR,2);
    break;

  case T_INT:
    sp--;
    sp[-1].u.integer |= sp[0].u.integer;
    break;

  case T_MAPPING:
  {
    struct mapping *m;
    m=merge_mappings(sp[-2].u.mapping, sp[-1].u.mapping, PIKE_ARRAY_OP_OR);
    pop_n_elems(2);
    push_mapping(m);
    return;
  }

  case T_MULTISET:
  {
    struct multiset *l;
    l=merge_multisets(sp[-2].u.multiset, sp[-1].u.multiset, PIKE_ARRAY_OP_OR_LEFT);
    pop_n_elems(2);
    push_multiset(l);
    return;
  }
    
  case T_ARRAY:
  {
    struct array *a;
    a=merge_array_with_order(sp[-2].u.array, sp[-1].u.array, PIKE_ARRAY_OP_OR_LEFT);
    pop_n_elems(2);
    push_array(a);
    return;
  }

  case T_TYPE:
  {
    struct pike_type *t;
    t = or_pike_types(sp[-2].u.type, sp[-1].u.type, 0);
    pop_n_elems(2);
    push_type_value(t);
    return;
  }

  case T_FUNCTION:
  case T_PROGRAM:
  {
    struct program *p;
    struct pike_type *a;
    struct pike_type *b;
    struct pike_type *t;

    p = program_from_svalue(sp - 2);
    if (!p) {
      int args = 2;
      SIMPLE_BAD_ARG_ERROR("`|", 1, "type");
    }
    type_stack_mark();
    push_object_type(0, p->id);
    a = pop_unfinished_type();

    p = program_from_svalue(sp - 1);
    if (!p) {
      int args = 2;
      SIMPLE_BAD_ARG_ERROR("`|", 2, "type");
    }
    type_stack_mark();
    push_object_type(0, p->id);
    b = pop_unfinished_type();

    t = or_pike_types(a, b, 0);

    pop_n_elems(2);
    push_type_value(t);
    free_type(a);
    free_type(b);
    return;
  }

  STRING_BITOP(|,"OR")

  default:
    PIKE_ERROR("`|", "Bitwise OR on illegal type.\n", sp, 2);
  }
}

/*! @decl mixed `|(mixed arg1)
 *! @decl mixed `|(mixed arg1, mixed arg2, mixed ... extras)
 *! @decl mixed `|(object arg1, mixed arg2)
 *! @decl mixed `|(mixed arg1, object arg2)
 *! @decl int `|(int arg1, int arg2)
 *! @decl string `|(string arg1, string arg2)
 *! @decl array `|(array arg1, array arg2)
 *! @decl mapping `|(mapping arg1, mapping arg2)
 *! @decl multiset `|(multiset arg1, multiset arg2)
 *! @decl type `|(program|type arg1, program|type arg2)
 *!
 *!   Bitwise or/union.
 *!
 *!   Every expression with the @expr{|@} operator becomes a call to
 *!   this function, i.e. @expr{a|b@} is the same as
 *!   @expr{predef::`|(a,b)@}.
 *!
 *! @returns
 *!   If there's a single argument, that argument is returned.
 *!
 *!   If there are more than two arguments, the result is:
 *!   @expr{`|(`|(@[arg1], @[arg2]), @@@[extras])@}.
 *!
 *!   Otherwise, if @[arg1] is an object with an @[lfun::`|()], that
 *!   function is called with @[arg2] as argument, and its result is
 *!   returned.
 *!
 *!   Otherwise, if @[arg2] is an object with an @[lfun::``|()], that
 *!   function is called with @[arg1] as argument, and its result is
 *!   returned.
 *!
 *!   Otherwise the result depends on the argument types:
 *!   @mixed arg1
 *!     @type int
 *!       Bitwise or of @[arg1] and @[arg2].
 *!     @type string
 *!       The result is a string where each character is the bitwise
 *!       or of the characters in the same position in @[arg1] and
 *!       @[arg2]. The arguments must be strings of the same length.
 *!     @type array
 *!       The result is an array with the elements in @[arg1]
 *!       concatenated with those in @[arg2] that doesn't occur in
 *!       @[arg1] (according to @[`==]). The order between the
 *!       elements that come from the same argument is kept.
 *!
 *!       Every element in @[arg1] is only matched once against an
 *!       element in @[arg2], so if @[arg2] contains several elements
 *!       that are equal to each other and are more than their
 *!       counterparts in @[arg1], the rightmost remaining elements in
 *!       @[arg2] are kept.
 *!     @type mapping
 *!       The result is like @[arg1] but extended with the entries
 *!       from @[arg2]. If the same index (according to @[hash_value]
 *!       and @[`==]) occur in both, the value from @[arg2] is used.
 *!     @type multiset
 *!       The result is like @[arg1] but extended with the entries in
 *!       @[arg2] that doesn't already occur in @[arg1] (according to
 *!       @[`==]). Subsequences with orderwise equal entries (i.e.
 *!       where @[`<] returns false) are handled just like the array
 *!       case above.
 *!     @type type|program
 *!       Type union of @[arg1] and @[arg2].
 *!   @endmixed
 *!   The function is not destructive on the arguments - the result is
 *!   always a new instance.
 *!
 *! @seealso
 *!   @[`&()], @[lfun::`|()], @[lfun::``|()]
 */
PMOD_EXPORT void f_or(INT32 args)
{
  switch(args)
  {
  case 0: SIMPLE_TOO_FEW_ARGS_ERROR("`|", 1);
  case 1: return;
  case 2: o_or(); return;
  default:
    if(sp[-args].type==T_OBJECT)
    {
      CALL_OPERATOR(LFUN_OR, args);
    } else {
      speedup(args, o_or);
    }
  }
}

static int generate_or(node *n)
{
  switch(count_args(CDR(n)))
  {
  case 1:
    do_docode(CDR(n),0);
    return 1;

  case 2:
    do_docode(CDR(n),0);
    emit0(F_OR);
    return 1;

  default:
    return 0;
  }
}


PMOD_EXPORT void o_xor(void)
{
  if(sp[-1].type != sp[-2].type)
  {
    if(call_lfun(LFUN_XOR, LFUN_RXOR)) {
      return;
    } else if (((sp[-1].type == T_TYPE) || (sp[-1].type == T_PROGRAM) ||
		(sp[-1].type == T_FUNCTION)) &&
	       ((sp[-2].type == T_TYPE) || (sp[-2].type == T_PROGRAM) ||
		(sp[-2].type == T_FUNCTION))) {
      if (sp[-2].type != T_TYPE) {
	struct program *p = program_from_svalue(sp - 2);
	if (!p) {
	  int args = 2;
	  SIMPLE_BAD_ARG_ERROR("`^", 1, "type");
	}
	type_stack_mark();
	push_object_type(0, p->id);
	free_svalue(sp - 2);
	sp[-2].u.type = pop_unfinished_type();
	sp[-2].type = T_TYPE;
      }
      if (sp[-1].type != T_TYPE) {
	struct program *p = program_from_svalue(sp - 1);
	if (!p) {
	  int args = 2;
	  SIMPLE_BAD_ARG_ERROR("`^", 2, "type");
	}
	type_stack_mark();
	push_object_type(0, p->id);
	free_svalue(sp - 1);
	sp[-1].u.type = pop_unfinished_type();
	sp[-1].type = T_TYPE;
      }
    } else {
      int args = 2;
      SIMPLE_BAD_ARG_ERROR("`^", 2, get_name_of_type(sp[-2].type));
    }
  }

  switch(sp[-2].type)
  {
  case T_OBJECT:
    CALL_OPERATOR(LFUN_XOR,2);
    break;

  case T_INT:
    sp--;
    sp[-1].u.integer ^= sp[0].u.integer;
    break;

  case T_MAPPING:
  {
    struct mapping *m;
    m=merge_mappings(sp[-2].u.mapping, sp[-1].u.mapping, PIKE_ARRAY_OP_XOR);
    pop_n_elems(2);
    push_mapping(m);
    return;
  }

  case T_MULTISET:
  {
    struct multiset *l;
    l=merge_multisets(sp[-2].u.multiset, sp[-1].u.multiset, PIKE_ARRAY_OP_XOR);
    pop_n_elems(2);
    push_multiset(l);
    return;
  }
    
  case T_ARRAY:
  {
    struct array *a;
    a=merge_array_with_order(sp[-2].u.array, sp[-1].u.array, PIKE_ARRAY_OP_XOR);
    pop_n_elems(2);
    push_array(a);
    return;
  }

  case T_FUNCTION:
  case T_PROGRAM:
  {
    struct program *p;

    p = program_from_svalue(sp - 1);
    if (!p) {
      int args = 2;
      SIMPLE_BAD_ARG_ERROR("`^", 2, "type");
    }
    type_stack_mark();
    push_object_type(0, p->id);
    pop_stack();
    push_type_value(pop_unfinished_type());

    stack_swap();

    p = program_from_svalue(sp - 1);
    if (!p) {
      int args = 2;
      stack_swap();
      SIMPLE_BAD_ARG_ERROR("`^", 1, "type");
    }
    type_stack_mark();
    push_object_type(0, p->id);
    pop_stack();
    push_type_value(pop_unfinished_type());
  }
  /* FALL_THROUGH */
  case T_TYPE:
  {
    /* a ^ b  ==  (a&~b)|(~a&b) */
    struct pike_type *a;
    struct pike_type *b;
    copy_pike_type(a, sp[-2].u.type);
    copy_pike_type(b, sp[-1].u.type);
    o_compl();		/* ~b */
    o_and();		/* a&~b */
    push_type_value(a);
    o_compl();		/* ~a */
    push_type_value(b);
    o_and();		/* ~a&b */
    o_or();		/* (a&~b)|(~a&b) */
    return;
  }

  STRING_BITOP(^,"XOR")

  default:
    PIKE_ERROR("`^", "Bitwise XOR on illegal type.\n", sp, 2);
  }
}

/*! @decl mixed `^(mixed arg1)
 *! @decl mixed `^(mixed arg1, mixed arg2, mixed ... extras)
 *! @decl mixed `^(object arg1, mixed arg2)
 *! @decl mixed `^(mixed arg1, object arg2)
 *! @decl int `^(int arg1, int arg2)
 *! @decl string `^(string arg1, string arg2)
 *! @decl array `^(array arg1, array arg2)
 *! @decl mapping `^(mapping arg1, mapping arg2)
 *! @decl multiset `^(multiset arg1, multiset arg2)
 *! @decl type `^(program|type arg1, program|type arg2)
 *!
 *!   Exclusive or.
 *!
 *!   Every expression with the @expr{^@} operator becomes a call to
 *!   this function, i.e. @expr{a^b@} is the same as
 *!   @expr{predef::`^(a,b)@}.
 *!
 *! @returns
 *!   If there's a single argument, that argument is returned.
 *!
 *!   If there are more than two arguments, the result is:
 *!   @expr{`^(`^(@[arg1], @[arg2]), @@@[extras])@}.
 *!
 *!   Otherwise, if @[arg1] is an object with an @[lfun::`^()], that
 *!   function is called with @[arg2] as argument, and its result is
 *!   returned.
 *!
 *!   Otherwise, if @[arg2] is an object with an @[lfun::``^()], that
 *!   function is called with @[arg1] as argument, and its result is
 *!   returned.
 *!
 *!   Otherwise the result depends on the argument types:
 *!   @mixed arg1
 *!     @type int
 *!       Bitwise exclusive or of @[arg1] and @[arg2].
 *!     @type string
 *!       The result is a string where each character is the bitwise
 *!       exclusive or of the characters in the same position in
 *!       @[arg1] and @[arg2]. The arguments must be strings of the
 *!       same length.
 *!     @type array
 *!       The result is an array with the elements in @[arg1] that
 *!       doesn't occur in @[arg2] concatenated with those in @[arg2]
 *!       that doesn't occur in @[arg1] (according to @[`==]). The
 *!       order between the elements that come from the same argument
 *!       is kept.
 *!
 *!       Every element is only matched once against an element in the
 *!       other array, so if one contains several elements that are
 *!       equal to each other and are more than their counterparts in
 *!       the other array, the rightmost remaining elements are kept.
 *!     @type mapping
 *!       The result is like @[arg1] but with the entries from @[arg1]
 *!       and @[arg2] whose indices are different between them
 *!       (according to @[hash_value] and @[`==]).
 *!     @type multiset
 *!       The result is like @[arg1] but with the entries from @[arg1]
 *!       and @[arg2] that are different between them (according to
 *!       @[hash_value] and @[`==]). Subsequences with orderwise equal
 *!       entries (i.e. where @[`<] returns false) are handled just
 *!       like the array case above.
 *!     @type type|program
 *!   	  The result is a type computed like this:
 *!   	  @expr{(@[arg1]&~@[arg2])|(~@[arg1]&@[arg2])@}.
 *!   @endmixed
 *!   The function is not destructive on the arguments - the result is
 *!   always a new instance.
 *!
 *! @seealso
 *!   @[`&()], @[`|()], @[lfun::`^()], @[lfun::``^()]
 */
PMOD_EXPORT void f_xor(INT32 args)
{
  switch(args)
  {
  case 0: SIMPLE_TOO_FEW_ARGS_ERROR("`^", 1);
  case 1: return;
  case 2: o_xor(); return;
  default:
    if(sp[-args].type==T_OBJECT)
    {
      CALL_OPERATOR(LFUN_XOR, args);
    } else {
      speedup(args, o_xor);
    }
  }
}

static int generate_xor(node *n)
{
  switch(count_args(CDR(n)))
  {
  case 1:
    do_docode(CDR(n),0);
    return 1;

  case 2:
    do_docode(CDR(n),0);
    emit0(F_XOR);
    return 1;

  default:
    return 0;
  }
}

PMOD_EXPORT void o_lsh(void)
{
#ifdef AUTO_BIGNUM
  if ((sp[-1].type == T_INT) && (sp[-2].type == T_INT) &&
      INT_TYPE_LSH_OVERFLOW(sp[-2].u.integer, sp[-1].u.integer))
    convert_stack_top_to_bignum();
#endif /* AUTO_BIGNUM */
  
  if(sp[-1].type != T_INT || sp[-2].type != T_INT)
  {
    int args = 2;
    if(call_lfun(LFUN_LSH, LFUN_RLSH))
      return;

    if(sp[-2].type != T_INT)
      SIMPLE_BAD_ARG_ERROR("`<<", 1, "int|object");
    SIMPLE_BAD_ARG_ERROR("`<<", 2, "int(0..)|object");
  }
#ifndef AUTO_BIGNUM
  if (sp[-1].u.integer > 31) {
    sp--;
    sp[-1].u.integer = 0;
    return;
  }
#endif /* !AUTO_BIGNUM */
  if (sp[-1].u.integer < 0) {
    int args = 2;
    SIMPLE_BAD_ARG_ERROR("`<<", 2, "int(0..)|object");    
  }
  sp--;
  sp[-1].u.integer = sp[-1].u.integer << sp->u.integer;
}

/*! @decl int `<<(int arg1, int arg2)
 *! @decl mixed `<<(object arg1, int|object arg2)
 *! @decl mixed `<<(int arg1, object arg2)
 *!
 *!   Left shift.
 *!
 *!   Every expression with the @expr{<<@} operator becomes a call to
 *!   this function, i.e. @expr{a<<b@} is the same as
 *!   @expr{predef::`<<(a,b)@}.
 *!
 *!   If @[arg1] is an object that implements @[lfun::`<<()], that
 *!   function will be called with @[arg2] as the single argument.
 *!
 *!   If @[arg2] is an object that implements @[lfun::``<<()], that
 *!   function will be called with @[arg1] as the single argument.
 *!
 *!   Otherwise @[arg1] will be shifted @[arg2] bits left.
 *!
 *! @seealso
 *!   @[`>>()]
 */
PMOD_EXPORT void f_lsh(INT32 args)
{
  if(args != 2) {
    /* FIXME: Not appropriate if too many args. */
    SIMPLE_TOO_FEW_ARGS_ERROR("`<<", 2);
  }
  o_lsh();
}

static int generate_lsh(node *n)
{
  if(count_args(CDR(n))==2)
  {
    do_docode(CDR(n),DO_NOT_COPY_TOPLEVEL);
    emit0(F_LSH);
    return 1;
  }
  return 0;
}

PMOD_EXPORT void o_rsh(void)
{
  if(sp[-2].type != T_INT || sp[-1].type != T_INT)
  {
    int args = 2;
    if(call_lfun(LFUN_RSH, LFUN_RRSH))
      return;
    if(sp[-2].type != T_INT)
      SIMPLE_BAD_ARG_ERROR("`>>", 1, "int|object");
    SIMPLE_BAD_ARG_ERROR("`>>", 2, "int(0..)|object");
  }
  
  if (sp[-1].u.integer < 0) {
    int args = 2;
    SIMPLE_BAD_ARG_ERROR("`>>", 2, "int(0..)|object");
  }

  if(
#ifdef AUTO_BIGNUM
     (INT_TYPE_RSH_OVERFLOW(sp[-2].u.integer, sp[-1].u.integer))
#else /* !AUTO_BIGNUM */
     (sp[-1].u.integer > 31)
#endif /* AUTO_BIGNUM */
     )
  {
    sp--;
    if (sp[-1].u.integer < 0) {
      sp[-1].u.integer = -1;
    } else {
      sp[-1].u.integer = 0;
    }
    return;
  }
  
  sp--;
  sp[-1].u.integer = sp[-1].u.integer >> sp->u.integer;
}

/*! @decl int `>>(int arg1, int arg2)
 *! @decl mixed `>>(object arg1, int|object arg2)
 *! @decl mixed `>>(int arg1, object arg2)
 *!
 *!   Right shift.
 *!
 *!   Every expression with the @expr{>>@} operator becomes a call to
 *!   this function, i.e. @expr{a>>b@} is the same as
 *!   @expr{predef::`>>(a,b)@}.
 *!
 *!   If @[arg1] is an object that implements @[lfun::`>>()], that
 *!   function will be called with @[arg2] as the single argument.
 *!
 *!   If @[arg2] is an object that implements @[lfun::``>>()], that
 *!   function will be called with @[arg1] as the single argument.
 *!
 *!   Otherwise @[arg1] will be shifted @[arg2] bits right.
 *!
 *! @seealso
 *!   @[`<<()]
 */
PMOD_EXPORT void f_rsh(INT32 args)
{
  if(args != 2) {
    /* FIXME: Not appropriate if too many args. */
    SIMPLE_TOO_FEW_ARGS_ERROR("`>>", 2);
  }
  o_rsh();
}

static int generate_rsh(node *n)
{
  if(count_args(CDR(n))==2)
  {
    do_docode(CDR(n),DO_NOT_COPY);
    emit0(F_RSH);
    return 1;
  }
  return 0;
}


#define TWO_TYPES(X,Y) (((X)<<8)|(Y))
PMOD_EXPORT void o_multiply(void)
{
  int args = 2;
  switch(TWO_TYPES(sp[-2].type,sp[-1].type))
  {
    case TWO_TYPES(T_ARRAY, T_INT):
      {
	struct array *ret;
	struct svalue *pos;
	INT32 e;
	if(sp[-1].u.integer < 0)
	  SIMPLE_BAD_ARG_ERROR("`*", 2, "int(0..)");
	ret=allocate_array(sp[-2].u.array->size * sp[-1].u.integer);
	pos=ret->item;
	for(e=0;e<sp[-1].u.integer;e++,pos+=sp[-2].u.array->size)
	  assign_svalues_no_free(pos,
				 sp[-2].u.array->item,
				 sp[-2].u.array->size,
				 sp[-2].u.array->type_field);
	ret->type_field=sp[-2].u.array->type_field;
	pop_n_elems(2);
	push_array(ret);
	return;
      }

    case TWO_TYPES(T_ARRAY, T_FLOAT):
      {
	struct array *src;
	struct array *ret;
	struct svalue *pos;
	ptrdiff_t asize, delta;
	if(sp[-1].u.float_number < 0)
	  SIMPLE_BAD_ARG_ERROR("`*", 2, "float(0..)");

	src = sp[-2].u.array;
	delta = src->size;
	asize = (ptrdiff_t)floor(delta * sp[-1].u.float_number + 0.5);
	ret = allocate_array(asize);
	pos = ret->item;
	if (asize > delta) {
	  ret->type_field = src->type_field;
	  assign_svalues_no_free(pos,
				 src->item,
				 delta,
				 src->type_field);
	  pos += delta;
	  asize -= delta;
	  while (asize > delta) {
	    assign_svalues_no_free(pos, ret->item, delta, ret->type_field);
	    pos += delta;
	    asize -= delta;
	    delta <<= 1;
	  }
	  if (asize) {
	    assign_svalues_no_free(pos, ret->item, asize, ret->type_field);
	  }
	} else if (asize) {
	  ret->type_field =
	    assign_svalues_no_free(pos,
				   src->item,
				   asize,
				   src->type_field);
	}
	pop_n_elems(2);
	push_array(ret);
	return;
      }

    case TWO_TYPES(T_STRING, T_FLOAT):
      {
	struct pike_string *src;
	struct pike_string *ret;
	char *pos;
	ptrdiff_t len, delta;

	if(sp[-1].u.float_number < 0)
	  SIMPLE_BAD_ARG_ERROR("`*", 2, "float(0..)");
	src = sp[-2].u.string;
	len = (ptrdiff_t)floor(src->len * sp[-1].u.float_number + 0.5);
	ret = begin_wide_shared_string(len, src->size_shift);
	len <<= src->size_shift;
	delta = src->len << src->size_shift;
	pos = ret->str;

	if (len > delta) {
	  MEMCPY(pos, src->str, delta);
	  pos += delta;
	  len -= delta;
	  while (len > delta) {
	    MEMCPY(pos, ret->str, delta);
	    pos += delta;
	    len -= delta;
	    delta <<= 1;
	  }
	  if (len) {
	    MEMCPY(pos, ret->str, len);
	  }
	} else if (len) {
	  MEMCPY(pos, src->str, len);
	}
	pop_n_elems(2);
	push_string(low_end_shared_string(ret));
	return;
      }


    case TWO_TYPES(T_STRING, T_INT):
      {
	struct pike_string *ret;
	char *pos;
	INT_TYPE e;
	ptrdiff_t len;
	if(sp[-1].u.integer < 0)
	  SIMPLE_BAD_ARG_ERROR("`*", 2, "int(0..)");
	ret=begin_wide_shared_string(sp[-2].u.string->len * sp[-1].u.integer,
				     sp[-2].u.string->size_shift);
	pos=ret->str;
	len=sp[-2].u.string->len << sp[-2].u.string->size_shift;
	for(e=0;e<sp[-1].u.integer;e++,pos+=len)
	  MEMCPY(pos,sp[-2].u.string->str,len);
	pop_n_elems(2);
	push_string(low_end_shared_string(ret));
	return;
      }

  case TWO_TYPES(T_ARRAY,T_STRING):
    {
      struct pike_string *ret;
      ret=implode(sp[-2].u.array,sp[-1].u.string);
      free_string(sp[-1].u.string);
      free_array(sp[-2].u.array);
      sp[-2].type=T_STRING;
      sp[-2].u.string=ret;
      sp--;
      return;
    }

  case TWO_TYPES(T_ARRAY,T_ARRAY):
  {
    struct array *ret;
    ret=implode_array(sp[-2].u.array, sp[-1].u.array);
    pop_n_elems(2);
    push_array(ret);
    return;
  }

  case TWO_TYPES(T_FLOAT,T_FLOAT):
    sp--;
    sp[-1].u.float_number *= sp[0].u.float_number;
    return;

  case TWO_TYPES(T_FLOAT,T_INT):
    sp--;
    sp[-1].u.float_number *= (FLOAT_TYPE)sp[0].u.integer;
    return;

  case TWO_TYPES(T_INT,T_FLOAT):
    sp--;
    sp[-1].u.float_number= 
      (FLOAT_TYPE) sp[-1].u.integer * sp[0].u.float_number;
    sp[-1].type=T_FLOAT;
    return;

  case TWO_TYPES(T_INT,T_INT):
#ifdef AUTO_BIGNUM
    if(INT_TYPE_MUL_OVERFLOW(sp[-2].u.integer, sp[-1].u.integer))
    {
      convert_stack_top_to_bignum();
      goto do_lfun_multiply;
    }
#endif /* AUTO_BIGNUM */
    sp--;
    sp[-1].u.integer *= sp[0].u.integer;
    return;

  default:
  do_lfun_multiply:
    if(call_lfun(LFUN_MULTIPLY, LFUN_RMULTIPLY))
      return;

    PIKE_ERROR("`*", "Bad arguments.\n", sp, 2);
  }
}

/*! @decl mixed `*(mixed arg1)
 *! @decl mixed `*(object arg1, mixed arg2, mixed ... extras)
 *! @decl mixed `*(mixed arg1, object arg2)
 *! @decl array `*(array arg1, int arg2)
 *! @decl array `*(array arg1, float arg2)
 *! @decl string `*(string arg1, int arg2)
 *! @decl string `*(string arg1, float arg2)
 *! @decl string `*(array(string) arg1, string arg2)
 *! @decl array `*(array(array) arg1, array arg2)
 *! @decl float `*(float arg1, int|float arg2)
 *! @decl float `*(int arg1, float arg2)
 *! @decl int `*(int arg1, int arg2)
 *! @decl mixed `*(mixed arg1, mixed arg2, mixed ... extras)
 *!
 *!   Multiplication/repetition/implosion.
 *!
 *!   Every expression with the @expr{*@} operator becomes a call to
 *!   this function, i.e. @expr{a*b@} is the same as
 *!   @expr{predef::`*(a,b)@}. Longer @expr{*@} expressions are
 *!   normally optimized to one call, so e.g. @expr{a*b*c@} becomes
 *!   @expr{predef::`*(a,b,c)@}.
 *!
 *! @returns
 *!   If there's a single argument, that argument will be returned.
 *!
 *!   If the first argument is an object that implements @[lfun::`*()],
 *!   that function will be called with the rest of the arguments.
 *!
 *!   If there are more than two arguments, the result will be
 *!   @expr{`*(`*(@[arg1], @[arg2]), @@@[extras])@}.
 *!
 *!   If @[arg2] is an object that implements @[lfun::``*()], that
 *!   function will be called with @[arg1] as the single argument.
 *!
 *!   Otherwise the result will be as follows:
 *!   @mixed arg1
 *!   	@type array
 *!   	  @mixed arg2
 *!   	    @type int|float
 *!   	      The result will be @[arg1] concatenated @[arg2] times.
 *!   	    @type string|array
 *!   	      The result will be the elements of @[arg1] concatenated with
 *!   	      @[arg2] interspersed.
 *!   	  @endmixed
 *!   	@type string
 *!   	  The result will be @[arg1] concatenated @[arg2] times.
 *!   	@type int|float
 *!   	  The result will be @expr{@[arg1] * @[arg2]@}, and will be a
 *!   	  float if either @[arg1] or @[arg2] is a float.
 *!   @endmixed
 *!
 *! @note
 *!   In Pike 7.0 and earlier the multiplication order was unspecified.
 *!
 *! @seealso
 *!   @[`+()], @[`-()], @[`/()], @[lfun::`*()], @[lfun::``*()]
 */
PMOD_EXPORT void f_multiply(INT32 args)
{
  switch(args)
  {
  case 0: SIMPLE_TOO_FEW_ARGS_ERROR("`*", 1);
  case 1: return;
  case 2: o_multiply(); return;
  default:
    if(sp[-args].type==T_OBJECT)
    {
      CALL_OPERATOR(LFUN_MULTIPLY, args);
    } else {
      INT32 i = -args, j = -1;
      /* Reverse the arguments */
      while(i < j) {
	struct svalue tmp = sp[i];
	sp[i++] = sp[j];
	sp[j--] = tmp;
      }
      while(--args > 0) {
	/* Restore the order, and multiply */
	stack_swap();
	o_multiply();
      }
    }
  }
}

static int generate_multiply(node *n)
{
  switch(count_args(CDR(n)))
  {
  case 1:
    do_docode(CDR(n),0);
    return 1;

  case 2:
    do_docode(CDR(n),0);
    emit0(F_MULTIPLY);
    return 1;

  default:
    return 0;
  }
}

PMOD_EXPORT void o_divide(void)
{
  if(sp[-2].type!=sp[-1].type && !float_promote())
  {
    if(call_lfun(LFUN_DIVIDE, LFUN_RDIVIDE))
      return;

    switch(TWO_TYPES(sp[-2].type,sp[-1].type))
    {
      case TWO_TYPES(T_STRING,T_INT):
      {
	struct array *a;
	INT_TYPE len;
	ptrdiff_t size,e,pos=0;

	len=sp[-1].u.integer;
	if(!len)
	  OP_DIVISION_BY_ZERO_ERROR("`/");

	if(len<0)
	{
	  len=-len;
	  size=sp[-2].u.string->len / len;
	  pos+=sp[-2].u.string->len % len;
	}else{
	  size=sp[-2].u.string->len / len;
	}
	a=allocate_array(size);
	for(e=0;e<size;e++)
	{
	  a->item[e].u.string=string_slice(sp[-2].u.string, pos,len);
	  a->item[e].type=T_STRING;
	  pos+=len;
	}
	a->type_field=BIT_STRING;
	pop_n_elems(2);
	push_array(a);
	return;
      }

      case TWO_TYPES(T_STRING,T_FLOAT):
      {
	struct array *a;
	ptrdiff_t size, pos, last, e;
	FLOAT_ARG_TYPE len;

	len=sp[-1].u.float_number;
	if(len==0.0)
	  OP_DIVISION_BY_ZERO_ERROR("`/");

	if(len<0)
	{
	  len=-len;
	  size=(ptrdiff_t)ceil( ((double)sp[-2].u.string->len) / len);
	  a=allocate_array(size);
	  
	  for(last=sp[-2].u.string->len,e=0;e<size-1;e++)
	  {
	    pos=sp[-2].u.string->len - (ptrdiff_t)((e+1)*len+0.5);
	    a->item[size-1-e].u.string=string_slice(sp[-2].u.string,
						    pos,
						    last-pos);
	    a->item[size-1-e].type=T_STRING;
	    last=pos;
	  }
	  pos=0;
	  a->item[0].u.string=string_slice(sp[-2].u.string,
					   pos,
					   last-pos);
	  a->item[0].type=T_STRING;
	}else{
	  size=(ptrdiff_t)ceil( ((double)sp[-2].u.string->len) / len);
	  a=allocate_array(size);
	  
	  for(last=0,e=0;e<size-1;e++)
	  {
	    pos = DO_NOT_WARN((ptrdiff_t)((e+1)*len+0.5));
	    a->item[e].u.string=string_slice(sp[-2].u.string,
					     last,
					     pos-last);
	    a->item[e].type=T_STRING;
	    last=pos;
	  }
	  pos=sp[-2].u.string->len;
	  a->item[e].u.string=string_slice(sp[-2].u.string,
					   last,
					   pos-last);
	  a->item[e].type=T_STRING;
	}
	a->type_field=BIT_STRING;
	pop_n_elems(2);
	push_array(a);
	return;
      }
	  

      case TWO_TYPES(T_ARRAY, T_INT):
      {
	struct array *a;
	ptrdiff_t size,e,pos;

	INT_TYPE len=sp[-1].u.integer;
	if(!len)
	  OP_DIVISION_BY_ZERO_ERROR("`/");
	
	if(len<0)
	{
	  len = -len;
	  pos = sp[-2].u.array->size % len;
	}else{
	  pos = 0;
	}
	size = sp[-2].u.array->size / len;

	a=allocate_array(size);
	for(e=0;e<size;e++)
	{
	  a->item[e].u.array=friendly_slice_array(sp[-2].u.array,
						  pos,
						  pos+len);
	  pos+=len;
	  a->item[e].type=T_ARRAY;
	}
	a->type_field=BIT_ARRAY;
	pop_n_elems(2);
	push_array(a);
	return;
      }

      case TWO_TYPES(T_ARRAY,T_FLOAT):
      {
	struct array *a;
	ptrdiff_t last,pos,e,size;
	FLOAT_ARG_TYPE len;

	len=sp[-1].u.float_number;
	if(len==0.0)
	  OP_DIVISION_BY_ZERO_ERROR("`/");

	if(len<0)
	{
	  len=-len;
	  size = (ptrdiff_t)ceil( ((double)sp[-2].u.array->size) / len);
	  a=allocate_array(size);
	  
	  for(last=sp[-2].u.array->size,e=0;e<size-1;e++)
	  {
	    pos=sp[-2].u.array->size - (ptrdiff_t)((e+1)*len+0.5);
	    a->item[size-1-e].u.array=friendly_slice_array(sp[-2].u.array,
						    pos,
						    last);
	    a->item[size-1-e].type=T_ARRAY;
	    last=pos;
	  }
	  a->item[0].u.array=slice_array(sp[-2].u.array,
					 0,
					 last);
	  a->item[0].type=T_ARRAY;
	}else{
	  size = (ptrdiff_t)ceil( ((double)sp[-2].u.array->size) / len);
	  a=allocate_array(size);
	  
	  for(last=0,e=0;e<size-1;e++)
	  {
	    pos = (ptrdiff_t)((e+1)*len+0.5);
	    a->item[e].u.array=friendly_slice_array(sp[-2].u.array,
						    last,
						    pos);
	    a->item[e].type=T_ARRAY;
	    last=pos;
	  }
	  a->item[e].u.array=slice_array(sp[-2].u.array,
					 last,
					 sp[-2].u.array->size);
	  a->item[e].type=T_ARRAY;
	}
	a->type_field=BIT_ARRAY;
	pop_n_elems(2);
	push_array(a);
	return;
      }
    }
      
    PIKE_ERROR("`/", "Division on different types.\n", sp, 2);
  }

  switch(sp[-2].type)
  {
  case T_OBJECT:
  do_lfun_division:
    CALL_OPERATOR(LFUN_DIVIDE,2);
    break;

  case T_STRING:
  {
    struct array *ret;
    ret=explode(sp[-2].u.string,sp[-1].u.string);
    free_string(sp[-2].u.string);
    free_string(sp[-1].u.string);
    sp[-2].type=T_ARRAY;
    sp[-2].u.array=ret;
    sp--;
    return;
  }

  case T_ARRAY:
  {
    struct array *ret=explode_array(sp[-2].u.array, sp[-1].u.array);
    pop_n_elems(2);
    push_array(ret);
    return;
  }

  case T_FLOAT:
    if(sp[-1].u.float_number == 0.0)
      OP_DIVISION_BY_ZERO_ERROR("`/");
    sp--;
    sp[-1].u.float_number /= sp[0].u.float_number;
    return;

  case T_INT:
  {
    INT_TYPE tmp;
    
    if (sp[-1].u.integer == 0)
      OP_DIVISION_BY_ZERO_ERROR("`/");

    if(INT_TYPE_DIV_OVERFLOW(sp[-2].u.integer, sp[-1].u.integer))
    {
#ifdef AUTO_BIGNUM
      stack_swap();
      convert_stack_top_to_bignum();
      stack_swap();
      goto do_lfun_division;
#else
      /* It's not possible to do MININT/-1 (it gives FPU exception on
	 some CPU:s), thus we return what MININT*-1 returns: MININT. */
      tmp = sp[-2].u.integer;
#endif /* AUTO_BIGNUM */
    }
    else
      tmp = sp[-2].u.integer/sp[-1].u.integer;
    sp--;

    /* What is this trying to solve? /Noring */
    /* It fixes rounding towards negative infinity. /mast */
    if((sp[-1].u.integer<0) != (sp[0].u.integer<0))
      if(tmp*sp[0].u.integer!=sp[-1].u.integer)
	tmp--;
    sp[-1].u.integer=tmp;
    return;
  }
    
  default:
    PIKE_ERROR("`/", "Bad argument 1.\n", sp, 2);
  }
}

/*! @decl mixed `/(object arg1, mixed arg2)
 *! @decl mixed `/(mixed arg1, object arg2)
 *! @decl array(string) `/(string arg1, int arg2)
 *! @decl array(string) `/(string arg1, float arg2)
 *! @decl array(array) `/(array arg1, int arg2)
 *! @decl array(array) `/(array arg1, float arg2)
 *! @decl array(string) `/(string arg1, string arg2)
 *! @decl array(array) `/(array arg1, array arg2)
 *! @decl float `/(float arg1, int|float arg2)
 *! @decl float `/(int arg1, float arg2)
 *! @decl int `/(int arg1, int arg2)
 *! @decl mixed `/(mixed arg1, mixed arg2, mixed ... extras)
 *!
 *!   Division/split.
 *!
 *!   Every expression with the @expr{/@} operator becomes a call to
 *!   this function, i.e. @expr{a/b@} is the same as
 *!   @expr{predef::`/(a,b)@}.
 *!
 *! @returns
 *!   If there are more than two arguments, the result will be
 *!   @expr{`/(`/(@[arg1], @[arg2]), @@@[extras])@}.
 *!
 *!   If @[arg1] is an object that implements @[lfun::`/()], that
 *!   function will be called with @[arg2] as the single argument.
 *!
 *!   If @[arg2] is an object that implements @[lfun::``/()], that
 *!   function will be called with @[arg1] as the single argument.
 *!
 *!   Otherwise the result will be as follows:
 *!   @mixed arg1
 *!   	@type string
 *!   	  @mixed arg2
 *!   	    @type int|float
 *!   	      The result will be and array of @[arg1] split in segments
 *!   	      of length @[arg2]. If @[arg2] is negative the splitting
 *!   	      will start from the end of @[arg1].
 *!   	    @type string
 *!   	      The result will be an array of @[arg1] split at each
 *!   	      occurrence of @[arg2]. Note that the segments that
 *!   	      matched against @[arg2] will not be in the result.
 *!   	  @endmixed
 *!   	@type array
 *!   	  @mixed arg2
 *!   	    @type int|float
 *!   	      The result will be and array of @[arg1] split in segments
 *!   	      of length @[arg2]. If @[arg2] is negative the splitting
 *!   	      will start from the end of @[arg1].
 *!   	    @type array
 *!   	      The result will be an array of @[arg1] split at each
 *!   	      occurrence of @[arg2]. Note that the elements that
 *!   	      matched against @[arg2] will not be in the result.
 *!   	  @endmixed
 *!   	@type float|int
 *!   	  The result will be @expr{@[arg1] / @[arg2]@}. If both arguments
 *!   	  are int, the result will be truncated to an int. Otherwise the
 *!   	  result will be a float.
 *!   @endmixed
 *! @note
 *!   Unlike in some languages, the function f(x) = x/n (x and n integers)
 *!   behaves in a well-defined way and is always rounded down. When you
 *!   increase x, f(x) will increase with one for each n:th increment. For
 *!   all x, (x + n) / n = x/n + 1; crossing
 *!   zero is not special. This also means that / and % are compatible, so
 *!   that a = b*(a/b) + a%b for all a and b.
 *! @seealso
 *!   @[`%]
 */
PMOD_EXPORT void f_divide(INT32 args)
{
  switch(args)
  {
    case 0: 
    case 1: SIMPLE_TOO_FEW_ARGS_ERROR("`/", 2);
    case 2: o_divide(); break;
    default:
    {
      INT32 e;
      struct svalue *s=sp-args;
      push_svalue(s);
      for(e=1;e<args;e++)
      {
	push_svalue(s+e);
	o_divide();
      }
      assign_svalue(s,sp-1);
      pop_n_elems(sp-s-1);
    }
  }
}

static int generate_divide(node *n)
{
  if(count_args(CDR(n))==2)
  {
    do_docode(CDR(n),DO_NOT_COPY_TOPLEVEL);
    emit0(F_DIVIDE);
    return 1;
  }
  return 0;
}

PMOD_EXPORT void o_mod(void)
{
  if(sp[-2].type != sp[-1].type && !float_promote())
  {
    if(call_lfun(LFUN_MOD, LFUN_RMOD))
      return;

    switch(TWO_TYPES(sp[-2].type,sp[-1].type))
    {
      case TWO_TYPES(T_STRING,T_INT):
      {
	struct pike_string *s=sp[-2].u.string;
	ptrdiff_t tmp,base;

	if(!sp[-1].u.integer)
	  OP_MODULO_BY_ZERO_ERROR("`%");

	if(sp[-1].u.integer<0)
	{
	  tmp=s->len % -sp[-1].u.integer;
	  base=0;
	}else{
	  tmp=s->len % sp[-1].u.integer;
	  base=s->len - tmp;
	}
	s=string_slice(s, base, tmp);
	pop_n_elems(2);
	push_string(s);
	return;
      }


      case TWO_TYPES(T_ARRAY,T_INT):
      {
	struct array *a=sp[-2].u.array;
	ptrdiff_t tmp,base;
	if(!sp[-1].u.integer)
	  OP_MODULO_BY_ZERO_ERROR("`%");

	if(sp[-1].u.integer<0)
	{
	  tmp=a->size % -sp[-1].u.integer;
	  base=0;
	}else{
	  tmp=a->size % sp[-1].u.integer;
	  base=a->size - tmp;
	}

	a=slice_array(a,base,base+tmp);
	pop_n_elems(2);
	push_array(a);
	return;
      }
    }

    PIKE_ERROR("`%", "Modulo on different types.\n", sp, 2);
  }

  switch(sp[-2].type)
  {
  case T_OBJECT:
    CALL_OPERATOR(LFUN_MOD,2);
    break;

  case T_FLOAT:
  {
    FLOAT_TYPE foo;
    if(sp[-1].u.float_number == 0.0)
      OP_MODULO_BY_ZERO_ERROR("`%");
    sp--;
    foo = DO_NOT_WARN((FLOAT_TYPE)(sp[-1].u.float_number /
				   sp[0].u.float_number));
    foo = DO_NOT_WARN((FLOAT_TYPE)(sp[-1].u.float_number -
				   sp[0].u.float_number * floor(foo)));
    sp[-1].u.float_number=foo;
    return;
  }
  case T_INT:
    if (sp[-1].u.integer == 0)
      OP_MODULO_BY_ZERO_ERROR("`%");
    sp--;
    if(sp[-1].u.integer>=0)
    {
      if(sp[0].u.integer>=0)
      {
	sp[-1].u.integer %= sp[0].u.integer;
      }else{
	sp[-1].u.integer=((sp[-1].u.integer+~sp[0].u.integer)%-sp[0].u.integer)-~sp[0].u.integer;
      }
    }else{
      if(sp[0].u.integer>=0)
      {
	sp[-1].u.integer=sp[0].u.integer+~((~sp[-1].u.integer) % sp[0].u.integer);
      }else{
	sp[-1].u.integer=-(-sp[-1].u.integer % -sp[0].u.integer);
      }
    }
    return;

  default:
    PIKE_ERROR("`%", "Bad argument 1.\n", sp, 2);
  }
}

/*! @decl mixed `%(object arg1, mixed arg2)
 *! @decl mixed `%(mixed arg1, object arg2)
 *! @decl string `%(string arg1, int arg2)
 *! @decl array `%(array arg1, int arg2)
 *! @decl float `%(float arg1, float|int arg2)
 *! @decl float `%(int arg1, float arg2)
 *! @decl int `%(int arg1, int arg2)
 *!
 *!   Modulo.
 *!
 *!   Every expression with the @expr{%@} operator becomes a call to
 *!   this function, i.e. @expr{a%b@} is the same as
 *!   @expr{predef::`%(a,b)@}.
 *!
 *! @returns
 *!   If @[arg1] is an object that implements @[lfun::`%()] then
 *!   that function will be called with @[arg2] as the single argument.
 *!
 *!   If @[arg2] is an object that implements @[lfun::``%()] then
 *!   that function will be called with @[arg2] as the single argument.
 *!
 *!   Otherwise the result will be as follows:
 *!   @mixed arg1
 *!   	@type string|array
 *!   	  If @[arg2] is positive, the result will be the last
 *!   	  @expr{`%(@[sizeof](@[arg1]), @[arg2])@} elements of @[arg1].
 *!   	  If @[arg2] is negative, the result will be the first
 *!   	  @expr{`%(@[sizeof](@[arg1]), -@[arg2])@} elements of @[arg1].
 *!   	@type int|float
 *!   	  The result will be
 *!   	  @expr{@[arg1] - @[arg2]*@[floor](@[arg1]/@[arg2])@}.
 *!   	  The result will be a float if either @[arg1] or @[arg2] is
 *!   	  a float, and an int otherwise.
 *!   @endmixed
 *!
 *!   For numbers, this means that
 *!   @ol
 *!     @item
 *!       a % b always has the same sign as b (typically b is positive;
 *!       array size, rsa modulo, etc, and a varies a lot more than b).
 *!     @item
 *!       The function f(x) = x % n behaves in a sane way; as x increases,
 *!       f(x) cycles through the values 0,1, ..., n-1, 0, .... Nothing
 *!       strange happens when you cross zero.
 *!     @item
 *!       The % operator implements the binary "mod" operation, as defined
 *!       by Donald Knuth (see the Art of Computer Programming, 1.2.4). It
 *!       should be noted that Pike treats %-by-0 as an error rather than
 *!       returning 0, though.
 *!     @item
 *!       / and % are compatible, so that a = b*(a/b) + a%b for all a and b.
 *!   @endol
 *! @seealso
 *!   @[`/]
 */
PMOD_EXPORT void f_mod(INT32 args)
{
  if(args != 2) {
    /* FIXME: Not appropriate when too many args. */
    SIMPLE_TOO_FEW_ARGS_ERROR("`%", 2);
  }
  o_mod();
}

static int generate_mod(node *n)
{
  if(count_args(CDR(n))==2)
  {
    do_docode(CDR(n),DO_NOT_COPY_TOPLEVEL);
    emit0(F_MOD);
    return 1;
  }
  return 0;
}

PMOD_EXPORT void o_not(void)
{
  switch(sp[-1].type)
  {
  case T_INT:
    sp[-1].u.integer = !sp[-1].u.integer;
    break;

  case T_FUNCTION:
  case T_OBJECT:
    if(UNSAFE_IS_ZERO(sp-1))
    {
      pop_stack();
      push_int(1);
    }else{
      pop_stack();
      push_int(0);
    }
    break;

  default:
    free_svalue(sp-1);
    sp[-1].type=T_INT;
    sp[-1].u.integer=0;
  }
}

/*! @decl int(0..1) `!(object|function arg)
 *! @decl int(1..1) `!(int(0..0) arg)
 *! @decl int(0..0) `!(mixed arg)
 *!
 *!   Logical not.
 *!
 *!   Every expression with the @expr{!@} operator becomes a call to
 *!   this function, i.e. @expr{!a@} is the same as
 *!   @expr{predef::`!(a)@}.
 *!
 *!   It's also used when necessary to test truth on objects, i.e. in
 *!   a statement @expr{if (o) ...@} where @expr{o@} is an object, the
 *!   test becomes the equivalent of @expr{!!o@} so that any
 *!   @[lfun::`!()] the object might have gets called.
 *!
 *! @returns
 *!   If @[arg] is an object that implements @[lfun::`!()], that function
 *!   will be called.
 *!
 *!   If @[arg] is @expr{0@} (zero), a destructed object, or a function in a
 *!   destructed object, @expr{1@} will be returned.
 *!
 *!   Otherwise @expr{0@} (zero) will be returned.
 *!
 *! @note
 *!   No float is considered false, not even @expr{0.0@}.
 *!
 *! @seealso
 *!   @[`==()], @[`!=()], @[lfun::`!()]
 */
PMOD_EXPORT void f_not(INT32 args)
{
  if(args != 1) {
    /* FIXME: Not appropriate with too many args. */
    SIMPLE_TOO_FEW_ARGS_ERROR("`!", 1);
  }
  o_not();
}

static int generate_not(node *n)
{
  if(count_args(CDR(n))==1)
  {
    do_docode(CDR(n),DO_NOT_COPY);
    emit0(F_NOT);
    return 1;
  }
  return 0;
}

PMOD_EXPORT void o_compl(void)
{
  switch(sp[-1].type)
  {
  case T_OBJECT:
    CALL_OPERATOR(LFUN_COMPL,1);
    break;
    
  case T_INT:
    sp[-1].u.integer = ~ sp[-1].u.integer;
    break;

  case T_FLOAT:
    sp[-1].u.float_number = -1.0 - sp[-1].u.float_number;
    break;

  case T_TYPE:
    type_stack_mark();
    if (sp[-1].u.type->type == T_NOT) {
      push_finished_type(sp[-1].u.type->car);
    } else {
      push_finished_type(sp[-1].u.type);
      push_type(T_NOT);
    }
    pop_stack();
    push_type_value(pop_unfinished_type());
    break;

  case T_FUNCTION:
  case T_PROGRAM:
    {
      /* !object(p) */
      struct program *p = program_from_svalue(sp - 1);
      if (!p) {
	PIKE_ERROR("`~", "Bad argument.\n", sp, 1);
      }
      type_stack_mark();
      push_object_type(0, p->id);
      push_type(T_NOT);
      pop_stack();
      push_type_value(pop_unfinished_type());
    }
    break;

  case T_STRING:
  {
    struct pike_string *s;
    ptrdiff_t len, i;

    if(sp[-1].u.string->size_shift) {
      bad_arg_error("`~", sp-1, 1, 1, "string(0)", sp-1,
		    "Expected 8-bit string.\n");
    }

    len = sp[-1].u.string->len;
    s = begin_shared_string(len);
    for (i=0; i<len; i++)
      s->str[i] = ~ sp[-1].u.string->str[i];
    pop_n_elems(1);
    push_string(end_shared_string(s));
    break;
  }

  default:
    PIKE_ERROR("`~", "Bad argument.\n", sp, 1);
  }
}

/*! @decl mixed `~(object arg)
 *! @decl int `~(int arg)
 *! @decl float `~(float arg)
 *! @decl type `~(type|program arg)
 *! @decl string `~(string arg)
 *!
 *!   Complement/inversion.
 *!
 *!   Every expression with the @expr{~@} operator becomes a call to
 *!   this function, i.e. @expr{~a@} is the same as
 *!   @expr{predef::`~(a)@}.
 *!
 *! @returns
 *!   The result will be as follows:
 *!   @mixed arg
 *!   	@type object
 *!   	  If @[arg] implements @[lfun::`~()], that function will be called.
 *!   	@type int
 *!   	  The bitwise inverse of @[arg] will be returned.
 *!   	@type float
 *!   	  The result will be @expr{-1.0 - @[arg]@}.
 *!   	@type type|program
 *!   	  The type inverse of @[arg] will be returned.
 *!   	@type string
 *!   	  If @[arg] only contains characters in the range 0 - 255 (8-bit),
 *!   	  a string containing the corresponding 8-bit inverses will be
 *!   	  returned.
 *!   @endmixed
 *!
 *! @seealso
 *!   @[`!()], @[lfun::`~()]
 */
PMOD_EXPORT void f_compl(INT32 args)
{
  if(args != 1) {
    /* FIXME: Not appropriate with too many args. */
    SIMPLE_TOO_FEW_ARGS_ERROR("`~", 1);
  }
  o_compl();
}

static int generate_compl(node *n)
{
  if(count_args(CDR(n))==1)
  {
    do_docode(CDR(n),DO_NOT_COPY);
    emit0(F_COMPL);
    return 1;
  }
  return 0;
}

PMOD_EXPORT void o_negate(void)
{
  switch(sp[-1].type)
  {
  case T_OBJECT:
  do_lfun_negate:
    CALL_OPERATOR(LFUN_SUBTRACT,1);
    break;

  case T_FLOAT:
    sp[-1].u.float_number=-sp[-1].u.float_number;
    return;
    
  case T_INT:
#ifdef AUTO_BIGNUM
    if(INT_TYPE_NEG_OVERFLOW(sp[-1].u.integer))
    {
      convert_stack_top_to_bignum();
      goto do_lfun_negate;
    }
#endif /* AUTO_BIGNUM */
    sp[-1].u.integer = - sp[-1].u.integer;
    return;

  default: 
    PIKE_ERROR("`-", "Bad argument to unary minus.\n", sp, 1);
  }
}

static void string_or_array_range (int bound_types,
				   struct svalue *ind,
				   INT_TYPE low,
				   INT_TYPE high)
/* ind is modified to point to the range. low and high are INT_TYPE to
 * avoid truncation problems when they come from int svalues. */
{
  INT32 from, to, len;		/* to and len are not inclusive. */

  if (ind->type == T_STRING)
    len = ind->u.string->len;
  else {
#ifdef PIKE_DEBUG
    if (!ind || ind->type != T_ARRAY) Pike_fatal ("Invalid ind svalue.\n");
#endif
    len = ind->u.array->size;
  }

  if (bound_types & RANGE_LOW_OPEN)
    from = 0;
  else {
    if (bound_types & RANGE_LOW_FROM_END) low = len - 1 - low;
    if (low < 0) from = 0;
    else if (low > len) from = len;
    else from = low;
  }

  if (bound_types & RANGE_HIGH_OPEN)
    to = len;
  else {
    if (bound_types & RANGE_HIGH_FROM_END) high = len - high;
    else high++;
    if (high < from) to = from;
    else if (high > len) to = len;
    else to = high;
  }

  if (ind->type == T_STRING) {
    struct pike_string *s;
    if (from == 0 && to == len) return;

    s=string_slice(ind->u.string, from, to-from);
    free_string(ind->u.string);
    ind->u.string=s;
  }

  else {
    struct array *a;
    a = slice_array(ind->u.array, from, to);
    free_array(ind->u.array);
    ind->u.array=a;
  }
}

static int call_old_range_lfun (int bound_types, struct object *o,
				struct svalue *low, struct svalue *high)
/* Returns nonzero on errors to let the caller format the appropriate
 * messages to throw. o is assumed to be undestructed on entry. One
 * ref each is consumed to low and high when they're in use. */
{
  struct svalue end_pos;
  ONERROR uwp;
  int f;

  if ((f = FIND_LFUN (o->prog, LFUN_INDEX)) == -1)
    return 1;

  /* FIXME: Check if the `[] lfun accepts at least two arguments. */

  /* o[a..b]    =>  o->`[] (a, b)
   * o[a..<b]   =>  o->`[] (a, o->_sizeof()-1-b)
   * o[a..]     =>  o->`[] (a, Pike.NATIVE_MAX)
   * o[<a..b]   =>  o->`[] (o->_sizeof()-1-a, b)
   * o[<a..<b]  =>  o->`[] (o->_sizeof()-1-a, o->_sizeof()-1-b)
   * o[<a..]    =>  o->`[] (o->_sizeof()-1-a, Pike.NATIVE_MAX)
   * o[..b]     =>  o->`[] (0, b)
   * o[..<b]    =>  o->`[] (0, o->_sizeof()-1-b)
   * o[..]      =>  o->`[] (0, Pike.NATIVE_MAX)
   */

  if (bound_types & (RANGE_LOW_FROM_END|RANGE_HIGH_FROM_END)) {
    int f2 = FIND_LFUN (o->prog, LFUN__SIZEOF);
    if (f2 == -1)
      return 2;
    apply_low (o, f2, 0);
    push_int (1);
    o_subtract();
    move_svalue (&end_pos, --sp);
    SET_ONERROR (uwp, do_free_svalue, &end_pos);
  }

  switch (bound_types & (RANGE_LOW_FROM_BEG|RANGE_LOW_FROM_END|RANGE_LOW_OPEN)) {
    case RANGE_LOW_FROM_BEG:
      move_svalue (sp++, low);
      break;
    case RANGE_LOW_OPEN:
      push_int (0);
      break;
    default:
      push_svalue (&end_pos);
      move_svalue (sp++, low);
      o_subtract();
      break;
  }

  switch (bound_types & (RANGE_HIGH_FROM_BEG|RANGE_HIGH_FROM_END|RANGE_HIGH_OPEN)) {
    case RANGE_HIGH_FROM_BEG:
      move_svalue (sp++, high);
      break;
    case RANGE_HIGH_OPEN:
      push_int (MAX_INT_TYPE);
      break;
    default:
      push_svalue (&end_pos);
      move_svalue (sp++, high);
      o_subtract();
      break;
  }

  if (bound_types & (RANGE_LOW_FROM_END|RANGE_HIGH_FROM_END)) {
    UNSET_ONERROR (uwp);
    free_svalue (&end_pos);
    /* Anything might have happened during the calls to
     * LFUN__SIZEOF and o_subtract above. */
    if (!o->prog)
      return 3;
  }

  apply_low (o, f, 2);
  return 0;
}

static const char *range_func_name (int bound_types)
{
  /* Since the number of arguments on the stack depend on bound_types
   * we have to make some effort to make it show in the backtrace. */
  switch (bound_types) {
    case RANGE_LOW_FROM_BEG|RANGE_HIGH_FROM_BEG: return "arg1[arg2..arg3]";
    case RANGE_LOW_FROM_BEG|RANGE_HIGH_FROM_END: return "arg1[arg2..<arg3]";
    case RANGE_LOW_FROM_BEG|RANGE_HIGH_OPEN:     return "arg1[arg2..]";
    case RANGE_LOW_FROM_END|RANGE_HIGH_FROM_BEG: return "arg1[<arg2..arg3]";
    case RANGE_LOW_FROM_END|RANGE_HIGH_FROM_END: return "arg1[<arg2..<arg3]";
    case RANGE_LOW_FROM_END|RANGE_HIGH_OPEN:     return "arg1[<arg2..]";
    case RANGE_LOW_OPEN|RANGE_HIGH_FROM_BEG:     return "arg1[..arg2]";
    case RANGE_LOW_OPEN|RANGE_HIGH_FROM_END:     return "arg1[..<arg2]";
    case RANGE_LOW_OPEN|RANGE_HIGH_OPEN:         return "arg1[..]";
#ifdef PIKE_DEBUG
    default:
      Pike_fatal ("Unexpected bound_types.\n");
#endif
  }
}

PMOD_EXPORT void o_range2 (int bound_types)
/* This takes between one and three args depending on whether
 * RANGE_LOW_OPEN and/or RANGE_HIGH_OPEN is set in bound_types. */
{
  struct svalue *ind, *low, *high;

  high = bound_types & RANGE_HIGH_OPEN ? sp : sp - 1;
  low = bound_types & RANGE_LOW_OPEN ? high : high - 1;
  ind = low - 1;

  switch (ind->type) {
    case T_OBJECT: {
      struct object *o = ind->u.object;
      int f;
      if (!o->prog)
	bad_arg_error (range_func_name (bound_types),
		       ind, sp - ind, 1, "object", ind,
		       "Cannot call `[..] in destructed object.\n");

      if ((f = FIND_LFUN(o->prog->inherits[ind->subtype].prog,
			 LFUN_RANGE)) != -1) {
	struct svalue h;
	if (!(bound_types & RANGE_HIGH_OPEN)) {
	  move_svalue (&h, high);
	  sp = high;
	}

	if (bound_types & RANGE_LOW_FROM_BEG)
	  push_int (INDEX_FROM_BEG);
	else if (bound_types & RANGE_LOW_OPEN) {
	  push_int (0);
	  push_int (OPEN_BOUND);
	}
	else
	  push_int (INDEX_FROM_END);

	if (bound_types & RANGE_HIGH_FROM_BEG) {
	  move_svalue (sp++, &h);
	  push_int (INDEX_FROM_BEG);
	}
	else if (bound_types & RANGE_HIGH_OPEN) {
	  push_int (0);
	  push_int (OPEN_BOUND);
	}
	else {
	  move_svalue (sp++, &h);
	  push_int (INDEX_FROM_END);
	}

	apply_low (o, f, 4);
	stack_pop_keep_top();
      }

      else
	switch (call_old_range_lfun (bound_types, o, low, high)) {
	  case 1:
	    bad_arg_error (range_func_name (bound_types),
			   ind, sp - ind, 1, "object", ind,
			   "Object got neither `[..] nor `[].\n");
	  case 2:
	    bad_arg_error (range_func_name (bound_types),
			   ind, sp - ind, 1, "object", ind,
			   "Object got no `[..] and there is no _sizeof to "
			   "translate the from-the-end index to use `[].\n");
	  case 3:
	    bad_arg_error (range_func_name (bound_types),
			   ind, 3, 1, "object", ind,
			   "Cannot call `[..] in destructed object.\n");
	  default:
	    free_svalue (ind);
	    move_svalue (ind, sp - 1);
	    /* low and high have lost their refs in call_old_range_lfun. */
	    sp = ind + 1;
	}

      break;
    }

    case T_STRING:
    case T_ARRAY: {
      INT_TYPE l, h;
      if (!(bound_types & RANGE_LOW_OPEN)) {
	if (low->type != T_INT)
	  bad_arg_error (range_func_name (bound_types),
			 ind, sp - ind, 2, "int", low,
			 "Bad lower bound. Expected int, got %s.\n",
			 get_name_of_type (low->type));
	l = low->u.integer;
      }
      if (!(bound_types & RANGE_HIGH_OPEN)) {
	if (high->type != T_INT)
	  bad_arg_error (range_func_name (bound_types),
			 ind, sp - ind, high - ind + 1, "int", high,
			 "Bad upper bound. Expected int, got %s.\n",
			 get_name_of_type (high->type));
	h = high->u.integer;
      }

      /* Can pop off the bounds without fuzz since they're simple integers. */
      sp = ind + 1;

      string_or_array_range (bound_types, ind, l, h);
      break;
    }

    default:
      bad_arg_error (range_func_name (bound_types),
		     ind, sp - ind, 1, "string|array|object", ind,
		     "Cannot use [..] on a %s. Expected string, array or object.\n",
		     get_name_of_type (ind->type));
  }
}

/*! @decl mixed `[..](object arg, mixed start, int start_type, mixed end, int end_type)
 *! @decl string `[..](string arg, int start, int start_type, int end, int end_type)
 *! @decl array `[..](array arg, int start, int start_type, int end, int end_type)
 *!
 *!   Extracts a subrange.
 *!
 *!   This is the function form of expressions with the @expr{[..]@}
 *!   operator. @[arg] is the thing from which the subrange is to be
 *!   extracted. @[start] is the lower bound of the subrange and
 *!   @[end] the upper bound.
 *!
 *!   @[start_type] and @[end_type] specifies how the @[start] and
 *!   @[end] indices, respectively, are to be interpreted. The types
 *!   are either @[Pike.INDEX_FROM_BEG], @[Pike.INDEX_FROM_END] or
 *!   @[Pike.OPEN_BOUND]. In the last case, the index value is
 *!   insignificant.
 *!
 *!   The relation between @expr{[..]@} expressions and this function
 *!   is therefore as follows:
 *!
 *!   @code
 *!     a[i..j]    <=>	`[..] (a, i, Pike.INDEX_FROM_BEG, j, Pike.INDEX_FROM_BEG)
 *!     a[i..<j]   <=>	`[..] (a, i, Pike.INDEX_FROM_BEG, j, Pike.INDEX_FROM_END)
 *!     a[i..]     <=>	`[..] (a, i, Pike.INDEX_FROM_BEG, 0, Pike.OPEN_BOUND)
 *!     a[<i..j]   <=>	`[..] (a, i, Pike.INDEX_FROM_END, j, Pike.INDEX_FROM_BEG)
 *!     a[<i..<j]  <=>	`[..] (a, i, Pike.INDEX_FROM_END, j, Pike.INDEX_FROM_END)
 *!     a[<i..]    <=>	`[..] (a, i, Pike.INDEX_FROM_END, 0, Pike.OPEN_BOUND)
 *!     a[..j]     <=>	`[..] (a, 0, Pike.OPEN_BOUND, j, Pike.INDEX_FROM_BEG)
 *!     a[..<j]    <=>	`[..] (a, 0, Pike.OPEN_BOUND, j, Pike.INDEX_FROM_END)
 *!     a[..]      <=>	`[..] (a, 0, Pike.OPEN_BOUND, 0, Pike.OPEN_BOUND)
 *!   @endcode
 *!
 *!   The subrange is specified as follows by the two bounds:
 *!
 *!   @ul
 *!     @item
 *!       If the lower bound refers to an index before the lowest
 *!       allowable (typically zero) then it's taken as an open bound
 *!       which starts at the first index (without any error).
 *!
 *!     @item
 *!       Correspondingly, if the upper bound refers to an index past
 *!       the last allowable then it's taken as an open bound which
 *!       ends at the last index (without any error).
 *!
 *!     @item
 *!       If the lower bound is less than or equal to the upper bound,
 *!       then the subrange is the inclusive range between them, i.e.
 *!       from and including the element at the lower bound and up to
 *!       and including the element at the upper bound.
 *!
 *!     @item
 *!       If the lower bound is greater than the upper bound then the
 *!       result is an empty subrange (without any error).
 *!   @endul
 *!
 *! @returns
 *!   The returned value depends on the type of @[arg]:
 *!
 *!   @mixed arg
 *!   	@type string
 *!   	  A string with the characters in the range is returned.
 *!
 *!   	@type array
 *!   	  An array with the elements in the range is returned.
 *!
 *!   	@type object
 *!       If the object implements @[lfun::`[..]], that function is
 *!       called with the four remaining arguments.
 *!
 *!       As a compatibility measure, if the object does not implement
 *!       @[lfun::`[..]] but @[lfun::`[]] then the latter is called
 *!       with the bounds transformed to normal from-the-beginning
 *!       indices in array-like fashion:
 *!
 *!       @dl
 *!         @item @{`[..] (a, i, Pike.INDEX_FROM_BEG, j, Pike.INDEX_FROM_BEG)@}
 *!           Calls @expr{a->`[] (i, j)@}
 *!         @item @{`[..] (a, i, Pike.INDEX_FROM_BEG, j, Pike.INDEX_FROM_END)@}
 *!           Calls @expr{a->`[] (i, a->_sizeof()-1-j)@}
 *!         @item @{`[..] (a, i, Pike.INDEX_FROM_BEG, 0, Pike.OPEN_BOUND)@}
 *!           Calls @expr{a->`[] (i, @[Int.NATIVE_MAX])@}
 *!         @item @{`[..] (a, i, Pike.INDEX_FROM_END, j, Pike.INDEX_FROM_BEG)@}
 *!           Calls @expr{a->`[] (a->_sizeof()-1-i, j)@}
 *!         @item @{`[..] (a, i, Pike.INDEX_FROM_END, j, Pike.INDEX_FROM_END)@}
 *!           Calls @expr{a->`[] (a->_sizeof()-1-i, a->_sizeof()-1-j)@},
 *!           except that @expr{a->_sizeof()@} is called only once.
 *!         @item @{`[..] (a, i, Pike.INDEX_FROM_END, 0, Pike.OPEN_BOUND)@}
 *!           Calls @expr{a->`[] (a->_sizeof()-1-i, @[Int.NATIVE_MAX])@}
 *!         @item @{`[..] (a, 0, Pike.OPEN_BOUND, j, Pike.INDEX_FROM_BEG)@}
 *!           Calls @expr{a->`[] (0, j)@}
 *!         @item @{`[..] (a, 0, Pike.OPEN_BOUND, j, Pike.INDEX_FROM_END)@}
 *!           Calls @expr{a->`[] (0, a->_sizeof()-1-j)@}
 *!         @item @{`[..] (a, 0, Pike.OPEN_BOUND, 0, Pike.OPEN_BOUND)@}
 *!           Calls @expr{a->`[] (0, @[Int.NATIVE_MAX])@}
 *!       @enddl
 *!
 *!       Note that @[Int.NATIVE_MAX] might be replaced with an even
 *!       larger integer in the future.
 *!   @endmixed
 *!
 *! @seealso
 *!   @[lfun::`[..]], @[`[]]
 */
PMOD_EXPORT void f_range(INT32 args)
{
  struct svalue *ind;
  if (args != 5)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("predef::`[..]", 5);
  ind = sp - 5;

#define CALC_BOUND_TYPES(bound_types) do {				\
    if (ind[2].type != T_INT)						\
      SIMPLE_ARG_TYPE_ERROR ("predef::`[..]", 3, "int");		\
    switch (ind[2].u.integer) {						\
      case INDEX_FROM_BEG: bound_types = RANGE_LOW_FROM_BEG; break;	\
      case INDEX_FROM_END: bound_types = RANGE_LOW_FROM_END; break;	\
      case OPEN_BOUND:     bound_types = RANGE_LOW_OPEN; break;		\
      default:								\
	SIMPLE_ARG_ERROR ("predef::`[..]", 3, "Unrecognized bound type."); \
    }									\
									\
    if (ind[4].type != T_INT)						\
      SIMPLE_ARG_TYPE_ERROR ("predef::`[..]", 5, "int");		\
    switch (ind[4].u.integer) {						\
      case INDEX_FROM_BEG: bound_types |= RANGE_HIGH_FROM_BEG; break;	\
      case INDEX_FROM_END: bound_types |= RANGE_HIGH_FROM_END; break;	\
      case OPEN_BOUND:     bound_types |= RANGE_HIGH_OPEN; break;	\
      default:								\
	SIMPLE_ARG_ERROR ("predef::`[..]", 5, "Unrecognized bound type."); \
    }									\
  } while (0)

  switch (ind->type) {
    case T_OBJECT: {
      struct object *o = ind->u.object;
      int f;
      if (!o->prog)
	SIMPLE_ARG_ERROR ("predef::`[..]", 1,
			  "Cannot call `[..] in destructed object.\n");

      if ((f = FIND_LFUN(o->prog->inherits[ind->subtype].prog,
			 LFUN_RANGE)) != -1) {
	apply_low (o, f, 4);
	stack_pop_keep_top();
      }

      else {
	int bound_types;
	CALC_BOUND_TYPES (bound_types);
	switch (call_old_range_lfun (bound_types, o, ind + 1, ind + 3)) {
	  case 1:
	    SIMPLE_ARG_ERROR ("predef::`[..]", 1,
			      "Object got neither `[..] nor `[].\n");
	  case 2:
	    SIMPLE_ARG_ERROR ("predef::`[..]", 1,
			      "Object got no `[..] and there is no _sizeof to "
			      "translate the from-the-end index to use `[].\n");
	  case 3:
	    SIMPLE_ARG_ERROR ("predef::`[..]", 1,
			      "Cannot call `[..] in destructed object.\n");
	  default:
	    free_svalue (ind);
	    move_svalue (ind, sp - 1);
	    /* The bound types are simple integers and the bounds
	     * themselves have lost their refs in call_old_range_lfun. */
	    sp = ind + 1;
	}
      }

      break;
    }

    case T_STRING:
    case T_ARRAY: {
      INT_TYPE l, h;
      int bound_types;
      CALC_BOUND_TYPES (bound_types);

      if (!(bound_types & RANGE_LOW_OPEN)) {
	if (ind[1].type != T_INT)
	  SIMPLE_ARG_TYPE_ERROR ("predef::`[..]", 2, "int");
	l = ind[1].u.integer;
      }
      if (!(bound_types & RANGE_HIGH_OPEN)) {
	if (ind[3].type != T_INT)
	  SIMPLE_ARG_TYPE_ERROR ("predef::`[..]", 4, "int");
	h = ind[3].u.integer;
      }

      pop_n_elems (4);
      string_or_array_range (bound_types, ind, l, h);
      break;
    }

    default:
      SIMPLE_ARG_TYPE_ERROR ("predef::`[..]", 1, "string|array|object");
  }
}

/*! @decl mixed `[](object arg, mixed index)
 *! @decl mixed `[](object arg, string index)
 *! @decl function `[](int arg, string index)
 *! @decl int `[](string arg, int index)
 *! @decl mixed `[](array arg, int index)
 *! @decl mixed `[](array arg, mixed index)
 *! @decl mixed `[](mapping arg, mixed index)
 *! @decl int(0..1) `[](multiset arg, mixed index)
 *! @decl mixed `[](program arg, string index)
 *! @decl mixed `[](object arg, mixed start, mixed end)
 *! @decl string `[](string arg, int start, int end)
 *! @decl array `[](array arg, int start, int end)
 *!
 *!   Indexing.
 *!
 *!   This is the function form of expressions with the @expr{[]@}
 *!   operator, i.e. @expr{a[i]@} is the same as
 *!   @expr{predef::`[](a,i)@}.
 *!
 *! @returns
 *!   If @[arg] is an object that implements @[lfun::`[]()], that
 *!   function is called with the @[index] argument.
 *!
 *!   Otherwise, the action depends on the type of @[arg]:
 *!
 *!   @mixed arg
 *!   	@type object
 *!   	  The non-static (i.e. public) symbol named @[index] is looked
 *!   	  up in @[arg].
 *!
 *!   	@type int
 *!   	  The bignum function named @[index] is looked up in @[arg].
 *!   	  The bignum functions are the same as those in the @[Gmp.mpz]
 *!   	  class.
 *!
 *!   	@type string
 *!   	  The character at index @[index] in @[arg] is returned as an
 *!   	  integer. The first character in the string is at index
 *!   	  @expr{0@} and the highest allowed index is therefore
 *!   	  @expr{sizeof(@[arg])-1@}. A negative index number accesses
 *!   	  the string from the end instead, from @expr{-1@} for the
 *!   	  last char back to @expr{-sizeof(@[arg])@} for the first.
 *!
 *!   	@type array
 *!   	  If @[index] is an int, index number @[index] of @[arg] is
 *!   	  returned. Allowed index number are in the range
 *!   	  @expr{[-sizeof(@[arg])..sizeof(@[arg])-1]@}; see the string
 *!   	  case above for details.
 *!
 *!   	  If @[index] is not an int, an array of all elements in
 *!   	  @[arg] indexed with @[index] are returned. I.e. it's the
 *!   	  same as doing @expr{column(@[arg], @[index])@}.
 *!
 *!   	@type mapping
 *!   	  If @[index] exists in @[arg] the corresponding value is
 *!       returned. Otherwise @expr{UNDEFINED@} is returned.
 *!
 *!   	@type multiset
 *!   	  If @[index] exists in @[arg], @expr{1@} is returned.
 *!   	  Otherwise @expr{UNDEFINED@} is returned.
 *!
 *!   	@type program
 *!   	  The non-static (i.e. public) constant symbol @[index] is
 *!   	  looked up in @[arg].
 *!
 *!   @endmixed
 *!
 *!   As a compatibility measure, this function also performs range
 *!   operations if it's called with three arguments. In that case it
 *!   becomes equivalent to:
 *!
 *!   @code
 *!     @[`[..]] (arg, start, @[Pike.INDEX_FROM_BEG], end, @[Pike.INDEX_FROM_BEG])
 *!   @endcode
 *!
 *!   See @[`[..]] for further details.
 *!
 *! @note
 *!   An indexing expression in an lvalue context, i.e. where the
 *!   index is being assigned a new value, uses @[`[]=] instead of
 *!   this function.
 *!
 *! @seealso
 *!   @[`->()], @[lfun::`[]()], @[`[]=], @[`[..]]
 */
PMOD_EXPORT void f_index(INT32 args)
{
  switch(args)
  {
  case 2:
    if(sp[-1].type==T_STRING) sp[-1].subtype=0;
    o_index();
    break;
  case 3:
    move_svalue (sp, sp - 1);
    sp += 2;
    sp[-3].type = sp[-1].type = T_INT;
    sp[-3].subtype = sp[-1].subtype = NUMBER_NUMBER;
    sp[-3].u.integer = sp[-1].u.integer = INDEX_FROM_BEG;
    f_range (5);
    break;
  default:
    SIMPLE_WRONG_NUM_ARGS_ERROR ("predef::`[]", args);
    break;
  }
}

/*! @decl mixed `->(object arg, string index)
 *! @decl mixed `->(int arg, string index)
 *! @decl mixed `->(array arg, string index)
 *! @decl mixed `->(mapping arg, string index)
 *! @decl int(0..1) `->(multiset arg, string index)
 *! @decl mixed `->(program arg, string index)
 *!
 *!   Arrow indexing.
 *!
 *!   Every non-lvalue expression with the @expr{->@} operator becomes
 *!   a call to this function. @expr{a->b@} is the same as
 *!   @expr{predef::`^(a,"b")@} where @expr{"b"@} is the symbol
 *!   @expr{b@} in string form.
 *!
 *!   This function behaves like @[`[]], except that the index is
 *!   passed literally as a string instead of being evaluated.
 *!
 *! @returns
 *!   If @[arg] is an object that implements @[lfun::`->()], that function
 *!   will be called with @[index] as the single argument.
 *!
 *!   Otherwise the result will be as follows:
 *!   @mixed arg
 *!   	@type object
 *!   	  The non-static (ie public) symbol named @[index] will be looked up
 *!   	  in @[arg].
 *!   	@type int
 *!   	  The bignum function named @[index] will be looked up in @[arg].
 *!   	@type array
 *!   	  An array of all elements in @[arg] arrow indexed with @[index]
 *!   	  will be returned.
 *!   	@type mapping
 *!   	  If @[index] exists in @[arg] the corresponding value will be
 *!       returned. Otherwise @expr{UNDEFINED@} will be returned.
 *!   	@type multiset
 *!   	  If @[index] exists in @[arg], @expr{1@} will be returned.
 *!   	  Otherwise @expr{UNDEFINED@} will be returned.
 *!   	@type program
 *!   	  The non-static (ie public) constant symbol @[index] will be
 *!   	  looked up in @[arg].
 *!   @endmixed
 *!
 *! @note
 *!   In an expression @expr{a->b@}, the symbol @expr{b@} can be any
 *!   token that matches the identifier syntax - keywords are
 *!   disregarded in that context.
 *!
 *! @note
 *!   An arrow indexing expression in an lvalue context, i.e. where
 *!   the index is being assigned a new value, uses @[`->=] instead of
 *!   this function.
 *!
 *! @seealso
 *!   @[`[]()], @[lfun::`->()], @[::`->()], @[`->=]
 */
PMOD_EXPORT void f_arrow(INT32 args)
{
  switch(args)
  {
  case 0:
  case 1:
    PIKE_ERROR("`->", "Too few arguments.\n", sp, args);
    break;
  case 2:
    if(sp[-1].type==T_STRING)
      sp[-1].subtype=1;
    o_index();
    break;
  default:
    PIKE_ERROR("`->", "Too many arguments.\n", sp, args);
  }
}

/*! @decl mixed `[]=(object arg, mixed index, mixed val)
 *! @decl mixed `[]=(object arg, string index, mixed val)
 *! @decl mixed `[]=(array arg, int index, mixed val)
 *! @decl mixed `[]=(mapping arg, mixed index, mixed val)
 *! @decl int(0..1) `[]=(multiset arg, mixed index, int(0..1) val)
 *!
 *!   Index assignment.
 *!
 *!   Every lvalue expression with the @expr{[]@} operator becomes a
 *!   call to this function, i.e. @expr{a[b]=c@} is the same as
 *!   @expr{predef::`[]=(a,b,c)@}.
 *!
 *!   If @[arg] is an object that implements @[lfun::`[]=()], that function
 *!   will be called with @[index] and @[val] as the arguments.
 *!
 *!   @mixed arg
 *!   	@type object
 *!   	  The non-static (ie public) variable named @[index] will be looked up
 *!   	  in @[arg], and assigned @[val].
 *!   	@type array|mapping
 *!   	  Index @[index] in @[arg] will be assigned @[val].
 *!   	@type multiset
 *!   	  If @[val] is @expr{0@} (zero), one occurrance of @[index] in
 *!   	  @[arg] will be removed. Otherwise @[index] will be added
 *!   	  to @[arg] if it is not already there.
 *!   @endmixed
 *!
 *! @returns
 *!   @[val] will be returned.
 *!
 *! @note
 *!   An indexing expression in a non-lvalue context, i.e. where the
 *!   index is being queried instead of assigned, uses @[`[]] instead
 *!   of this function.
 *!
 *! @seealso
 *!   @[`->=()], @[lfun::`[]=()], @[`[]]
 */
PMOD_EXPORT void f_index_assign(INT32 args)
{
  switch (args) {
    case 0:
    case 1:
    case 2:
      PIKE_ERROR("`[]=", "Too few arguments.\n", sp, args);
      break;
    case 3:
      if(sp[-2].type==T_STRING) sp[-2].subtype=0;
      assign_lvalue (sp-3, sp-1);
      assign_svalue (sp-3, sp-1);
      pop_n_elems (args-1);
      break;
    default:
      PIKE_ERROR("`[]=", "Too many arguments.\n", sp, args);
  }
}

/*! @decl mixed `->=(object arg, string index, mixed val)
 *! @decl mixed `->=(mapping arg, string index, mixed val)
 *! @decl int(0..1) `->=(multiset arg, string index, int(0..1) val)
 *!
 *!   Arrow index assignment.
 *!
 *!   Every lvalue expression with the @expr{->@} operator becomes a
 *!   call to this function, i.e. @expr{a->b=c@} is the same as
 *!   @expr{predef::`->=(a,"b",c)@} where @expr{"b"@} is the symbol
 *!   @expr{b@} in string form.
 *!
 *!   This function behaves like @[`[]=], except that the index is
 *!   passed literally as a string instead of being evaluated.
 *!
 *!   If @[arg] is an object that implements @[lfun::`->=()], that function
 *!   will be called with @[index] and @[val] as the arguments.
 *!
 *!   @mixed arg
 *!   	@type object
 *!   	  The non-static (ie public) variable named @[index] will be looked up
 *!   	  in @[arg], and assigned @[val].
 *!   	@type array|mapping
 *!   	  Index @[index] in @[arg] will be assigned @[val].
 *!   	@type multiset
 *!   	  If @[val] is @expr{0@} (zero), one occurrance of @[index] in
 *!   	  @[arg] will be removed. Otherwise @[index] will be added
 *!   	  to @[arg] if it is not already there.
 *!   @endmixed
 *!
 *! @returns
 *!   @[val] will be returned.
 *!
 *! @note
 *!   In an expression @expr{a->b=c@}, the symbol @expr{b@} can be any
 *!   token that matches the identifier syntax - keywords are
 *!   disregarded in that context.
 *!
 *! @note
 *!   An arrow indexing expression in a non-lvalue context, i.e. where
 *!   the index is being queried instead of assigned, uses @[`->]
 *!   instead of this function.
 *!
 *! @seealso
 *!   @[`[]=()], @[lfun::`->=()], @[`->]
 */
PMOD_EXPORT void f_arrow_assign(INT32 args)
{
  switch (args) {
    case 0:
    case 1:
    case 2:
      PIKE_ERROR("`->=", "Too few arguments.\n", sp, args);
      break;
    case 3:
      if(sp[-2].type==T_STRING) sp[-2].subtype=1;
      assign_lvalue (sp-3, sp-1);
      assign_svalue (sp-3, sp-1);
      pop_n_elems (args-1);
      break;
    default:
      PIKE_ERROR("`->=", "Too many arguments.\n", sp, args);
  }
}

/*! @decl int sizeof(string arg)
 *! @decl int sizeof(array arg)
 *! @decl int sizeof(mapping arg)
 *! @decl int sizeof(multiset arg)
 *! @decl int sizeof(object arg)
 *! 
 *!   Size query.
 *!
 *! @returns
 *!   The result will be as follows:
 *!   @mixed arg
 *!   	@type string
 *!   	  The number of characters in @[arg] will be returned.
 *!   	@type array|multiset
 *!   	  The number of elements in @[arg] will be returned.
 *!   	@type mapping
 *!   	  The number of key-value pairs in @[arg] will be returned.
 *!   	@type object
 *!   	  If @[arg] implements @[lfun::_sizeof()], that function will
 *!   	  be called. Otherwise the number of non-static (ie public)
 *!   	  symbols in @[arg] will be returned.
 *!   @endmixed
 *!
 *! @seealso
 *!   @[lfun::_sizeof()]
 */
PMOD_EXPORT void f_sizeof(INT32 args)
{
  INT32 tmp;
  if(args<1)
    PIKE_ERROR("sizeof", "Too few arguments.\n", sp, args);

  tmp=pike_sizeof(sp-args);

  pop_n_elems(args);
  push_int(tmp);
}

static node *optimize_sizeof(node *n)
{
  if (CDR(n) && (CDR(n)->token == F_APPLY) &&
      (CADR(n)) && (CADR(n)->token == F_CONSTANT) &&
      (CADR(n)->u.sval.type == T_FUNCTION) &&
      (CADR(n)->u.sval.subtype == FUNCTION_BUILTIN)) {
    extern struct program *string_split_iterator_program;
    /* sizeof(efun(...)) */
    if ((CADR(n)->u.sval.u.efun->function == f_divide) &&
	CDDR(n) && (CDDR(n)->token == F_ARG_LIST) &&
	CADDR(n) && (CADDR(n)->type == string_type_string) &&
	CDDDR(n) && (CDDDR(n)->token == F_CONSTANT) &&
	(CDDDR(n)->u.sval.type == T_STRING) &&
	(CDDDR(n)->u.sval.u.string->len == 1)) {
      p_wchar2 split = index_shared_string(CDDDR(n)->u.sval.u.string, 0);

      /* sizeof(`/(str, "x")) */
      ADD_NODE_REF2(CADDR(n),
        return mkefuncallnode("sizeof",
			      mkapplynode(mkprgnode(string_split_iterator_program),
					  mknode(F_ARG_LIST, CADDR(n),
						 mkintnode(split))));
      );
    }
    if ((CADR(n)->u.sval.u.efun->function == f_minus) &&
	CDDR(n) && (CDDR(n)->token == F_ARG_LIST) &&
	CADDR(n) && (CADDR(n)->token == F_APPLY) &&
	CAADDR(n) && (CAADDR(n)->token == F_CONSTANT) &&
	(CAADDR(n)->u.sval.type == T_FUNCTION) &&
	(CAADDR(n)->u.sval.subtype == FUNCTION_BUILTIN) &&
	(CAADDR(n)->u.sval.u.efun->function == f_divide) &&
	CDADDR(n) && (CDADDR(n)->token == F_ARG_LIST) &&
	CADADDR(n) && (CADADDR(n)->type == string_type_string) &&
	CDDADDR(n) && (CDDADDR(n)->token == F_CONSTANT) &&
	(CDDADDR(n)->u.sval.type == T_STRING) &&
	(CDDADDR(n)->u.sval.u.string->len == 1) &&
	CDDDR(n)) {
      /* sizeof(`-(`/(str, "x"), y)) */
      if (((CDDDR(n)->token == F_CONSTANT) &&
	   (CDDDR(n)->u.sval.type == T_ARRAY) &&
	   (CDDDR(n)->u.sval.u.array->size == 1) &&
	   (CDDDR(n)->u.sval.u.array->item[0].type == T_STRING) &&
	   (CDDDR(n)->u.sval.u.array->item[0].u.string->len == 0)) ||
	  ((CDDDR(n)->token == F_APPLY) &&
	   CADDDR(n) && (CADDDR(n)->token == F_CONSTANT) &&
	   (CADDDR(n)->u.sval.type == T_FUNCTION) &&
	   (CADDDR(n)->u.sval.subtype == FUNCTION_BUILTIN) &&
	   (CADDDR(n)->u.sval.u.efun->function == f_allocate) &&
	   CDDDDR(n) && (CDDDDR(n)->token == F_ARG_LIST) &&
	   CADDDDR(n) && (CADDDDR(n)->token == F_CONSTANT) &&
	   (CADDDDR(n)->u.sval.type == T_INT) &&
	   (CADDDDR(n)->u.sval.u.integer == 1) &&
	   CDDDDDR(n) && (CDDDDDR(n)->token == F_CONSTANT) &&
	   (CDDDDDR(n)->u.sval.type == T_STRING) &&
	   (CDDDDDR(n)->u.sval.u.string->len == 0))) {
	/* sizeof(`-(`/(str, "x"), ({""}))) */
	p_wchar2 split = index_shared_string(CDDADDR(n)->u.sval.u.string, 0);
	ADD_NODE_REF2(CADADDR(n),
          return mkefuncallnode("sizeof",
				mkapplynode(mkprgnode(string_split_iterator_program),
					    mknode(F_ARG_LIST, CADADDR(n),
						   mknode(F_ARG_LIST,
							  mkintnode(split),
							  mkintnode(1)))));
	);
      }
    }
  }
  return NULL;
}

static int generate_sizeof(node *n)
{
  if(count_args(CDR(n)) != 1) return 0;
  if(do_docode(CDR(n),DO_NOT_COPY) != 1)
    Pike_fatal("Count args was wrong in sizeof().\n");
  emit0(F_SIZEOF);
  return 1;
}

extern int generate_call_function(node *n);

/*! @class string_assignment
 */

struct program *string_assignment_program;

#undef THIS
#define THIS ((struct string_assignment_storage *)(CURRENT_STORAGE))
/*! @decl int `[](int i)
 *!
 *! String index operator.
 */
static void f_string_assignment_index(INT32 args)
{
  ptrdiff_t len;
  INT_TYPE i, p;

  get_all_args("string[]", args, "%i", &p);

  if (!THIS->s) {
    Pike_error("Indexing uninitialized string_assignment.\n");
  }

  len = THIS->s->len;
  i = p < 0 ? p + len : p;
  if(i<0 || i>=len)
    Pike_error("Index %"PRINTPIKEINT"d is out of string range "
	       "%"PRINTPTRDIFFT"d..%"PRINTPTRDIFFT"d.\n",
	       p, -len, len - 1);
  else
    i=index_shared_string(THIS->s,i);
  pop_n_elems(args);
  push_int(i);
}

/*! @decl int `[]=(int i, int j)
 *!
 *! String assign index operator.
 */
static void f_string_assignment_assign_index(INT32 args)
{
  INT_TYPE p, i, j;
  union anything *u;
  ptrdiff_t len;

  get_all_args("string[]=",args,"%i%i",&p,&j);

  if((u=get_pointer_if_this_type(THIS->lval, T_STRING)))
  {
    len = u->string->len;
    i = p < 0 ? p + len : p;
    if(i<0 || i>=len)
      Pike_error("Index %"PRINTPIKEINT"d is out of string range "
		 "%"PRINTPTRDIFFT"d..%"PRINTPTRDIFFT"d.\n",
		 p, -len, len - 1);
    if (THIS->s) free_string(THIS->s);
    u->string=modify_shared_string(u->string,i,j);
    copy_shared_string(THIS->s, u->string);
  }

  else{
    lvalue_to_svalue_no_free(sp,THIS->lval);
    sp++;
    dmalloc_touch_svalue(Pike_sp-1);
    if(sp[-1].type != T_STRING) Pike_error("string[]= failed.\n");
    len = sp[-1].u.string->len;
    i = p < 0 ? p + len : p;
    if(i<0 || i>=len)
      Pike_error("Index %"PRINTPIKEINT"d is out of string range "
		 "%"PRINTPTRDIFFT"d..%"PRINTPTRDIFFT"d.\n",
		 p, -len, len - 1);
    sp[-1].u.string=modify_shared_string(sp[-1].u.string,i,j);
    assign_lvalue(THIS->lval, sp-1);
    pop_stack();
  }

  pop_n_elems(args);
  push_int(j);
}


static void init_string_assignment_storage(struct object *o)
{
  THIS->lval[0].type = T_INT;
  THIS->lval[1].type = T_INT;
  THIS->s = NULL;
}

static void exit_string_assignment_storage(struct object *o)
{
  free_svalues(THIS->lval, 2, BIT_MIXED);
  if(THIS->s)
    free_string(THIS->s);
}

/*! @endclass
 */

void init_operators(void)
{
  ADD_EFUN ("`[..]", f_range,
	    tOr3(tFunc(tStr tInt tRangeBound tInt tRangeBound, tStr),
		 tFunc(tArr(tSetvar(0,tMix)) tInt tRangeBound tInt tRangeBound, tArr(tVar(0))),
		 tFunc(tObj tMix tRangeBound tMix tRangeBound, tMix)),
	    OPT_TRY_OPTIMIZE);

  ADD_INT_CONSTANT ("INDEX_FROM_BEG", INDEX_FROM_BEG, 0);
  ADD_INT_CONSTANT ("INDEX_FROM_END", INDEX_FROM_END, 0);
  ADD_INT_CONSTANT ("OPEN_BOUND", OPEN_BOUND, 0);

  ADD_EFUN ("`[]", f_index,
	    tOr9(tFunc(tObj tMix tOr(tVoid,tMix), tMix),
		 tFunc(tInt tString, tFunction),
		 tFunc(tStr tInt, tInt),
		 tFunc(tArr(tSetvar(0,tMix)) tMix, tVar(0)),
		 tFunc(tMap(tMix,tSetvar(1,tMix)) tMix, tVar(1)),
		 tFunc(tMultiset tMix, tInt01),
		 tFunc(tPrg(tObj) tString, tMix),
		 tFunc(tStr tInt tInt, tStr),
		 tFunc(tArr(tSetvar(2,tMix)) tInt tInt, tArr(tVar(2)))),
	    OPT_TRY_OPTIMIZE);

  /* function(array(object|mapping|multiset|array),string:array(mixed))|function(object|mapping|multiset|program,string:mixed) */
  ADD_EFUN2("`->",f_arrow,tOr(tFunc(tArr(tOr4(tObj,tMapping,tMultiset,tArray)) tStr,tArr(tMix)),tFunc(tOr4(tObj,tMapping,tMultiset,tPrg(tObj)) tStr,tMix)),OPT_TRY_OPTIMIZE,0,0);

  ADD_EFUN("`[]=", f_index_assign,
	   tOr4(tFunc(tObj tStr tSetvar(0,tMix), tVar(0)),
		tFunc(tArr(tSetvar(1,tMix)) tInt tVar(1), tVar(1)),
		tFunc(tMap(tSetvar(2,tMix), tSetvar(3,tMix)) tVar(2) tVar(3), tVar(3)),
		tFunc(tSet(tSetvar(4,tMix)) tVar(4) tSetvar(5,tMix), tVar(5))),
	   OPT_SIDE_EFFECT|OPT_TRY_OPTIMIZE);

  ADD_EFUN("`->=", f_arrow_assign,
	   tOr3(tFunc(tArr(tOr4(tArray,tObj,tMultiset,tMapping)) tStr tSetvar(0,tMix), tVar(0)),
		tFunc(tOr(tObj, tMultiset) tStr tSetvar(1,tMix), tVar(1)),
		tFunc(tMap(tMix, tSetvar(2,tMix)) tStr tVar(2), tVar(2))),
	   OPT_SIDE_EFFECT|OPT_TRY_OPTIMIZE);

  /* function(mixed...:int) */
  ADD_EFUN2("`==",f_eq,
	    tOr5(tFuncV(tOr(tInt,tFloat) tOr(tInt,tFloat),
			tOr(tInt,tFloat),tInt01),
		 tFuncV(tSetvar(0,tOr4(tString,tMapping,tMultiset,tArray))
			tVar(0), tVar(0),tInt01),
		 tFuncV(tOr3(tObj,tPrg(tObj),tFunction) tMix,tMix,tInt01),
		 tFuncV(tMix tOr3(tObj,tPrg(tObj),tFunction),tMix,tInt01),
		 tFuncV(tType(tMix) tType(tMix),
			tOr3(tPrg(tObj),tFunction,tType(tMix)),tInt01)),
	    OPT_WEAK_TYPE|OPT_TRY_OPTIMIZE,optimize_eq,generate_comparison);
  /* function(mixed...:int) */
  ADD_EFUN2("`!=",f_ne,
	    tOr5(tFuncV(tOr(tInt,tFloat) tOr(tInt,tFloat),
			tOr(tInt,tFloat),tInt01),
		 tFuncV(tSetvar(0,tOr4(tString,tMapping,tMultiset,tArray))
			tVar(0), tVar(0),tInt01),
		 tFuncV(tOr3(tObj,tPrg(tObj),tFunction) tMix,tMix,tInt01),
		 tFuncV(tMix tOr3(tObj,tPrg(tObj),tFunction),tMix,tInt01),
		 tFuncV(tType(tMix) tType(tMix),
			tOr3(tPrg(tObj),tFunction,tType(tMix)),tInt01)),
	    OPT_WEAK_TYPE|OPT_TRY_OPTIMIZE,0,generate_comparison);
  /* function(mixed:int) */
  ADD_EFUN2("`!",f_not,tFuncV(tMix,tVoid,tInt01),
	    OPT_TRY_OPTIMIZE,optimize_not,generate_not);

#define CMP_TYPE "!function(!(object|mixed)...:mixed)&function(mixed...:int(0..1))|function(int|float...:int(0..1))|function(string...:int(0..1))|function(type|program,type|program,type|program...:int(0..1))"
  add_efun2("`<", f_lt,CMP_TYPE,OPT_TRY_OPTIMIZE,0,generate_comparison);
  add_efun2("`<=",f_le,CMP_TYPE,OPT_TRY_OPTIMIZE,0,generate_comparison);
  add_efun2("`>", f_gt,CMP_TYPE,OPT_TRY_OPTIMIZE,0,generate_comparison);
  add_efun2("`>=",f_ge,CMP_TYPE,OPT_TRY_OPTIMIZE,0,generate_comparison);

  ADD_EFUN2("`+",f_add,
	    tOr7(tIfnot(tFuncV(tNone,tNot(tOr(tObj,tMix)),tMix),tFunction),
		 tFuncV(tInt,tInt,tInt),
		 tIfnot(tFuncV(tNone, tNot(tFlt), tMix),
			tFuncV(tOr(tInt,tFlt),tOr(tInt,tFlt),tFlt)),
		 tIfnot(tFuncV(tNone, tNot(tStr), tMix),
			tFuncV(tOr3(tStr,tInt,tFlt),
			       tOr3(tStr,tInt,tFlt),tStr)),
		 tFuncV(tSetvar(0,tArray),tSetvar(1,tArray),
			tOr(tVar(0),tVar(1))),
		 tFuncV(tSetvar(0,tMapping),tSetvar(1,tMapping),
			tOr(tVar(0),tVar(1))),
		 tFuncV(tSetvar(0,tMultiset),tSetvar(1,tMultiset),
			tOr(tVar(0),tVar(1)))),
	    OPT_TRY_OPTIMIZE,optimize_binary,generate_sum);
  
  ADD_EFUN2("`-",f_minus,
	    tOr7(tIfnot(tFuncV(tNone,tNot(tOr(tObj,tMix)),tMix),tFunction),
		 tFuncV(tInt,tInt,tInt),
		 tIfnot(tFuncV(tNone,tNot(tFlt),tMix),
			tFuncV(tOr(tInt,tFlt),tOr(tInt,tFlt),tFlt)),
		 tFuncV(tArr(tSetvar(0,tMix)),tArray,tArr(tVar(0))),
		 tFuncV(tMap(tSetvar(1,tMix),tSetvar(2,tMix)),
			tOr3(tMapping,tArray,tMultiset),
			tMap(tVar(1),tVar(2))),
		 tFunc(tSet(tSetvar(3,tMix)) tMultiset,tSet(tVar(3))),
		 tFuncV(tStr,tStr,tStr)),
	    OPT_TRY_OPTIMIZE,0,generate_minus);

/*

object & mixed -> mixed
mixed & object -> mixed

int & int -> int
array & array -> array
multiset & multiset -> multiset
mapping & mapping -> mapping
string & string -> string
type|program & type|program -> type|program

mapping & array -> mapping
array & mapping -> mapping
mapping & multiset -> mapping
multiset & mapping -> mapping

 */


#define F_AND_TYPE(Z)						\
	    tOr(tFunc(tSetvar(0,Z),tVar(0)),			\
		tIfnot(tFunc(Z,tMix),				\
		       tFuncV(tSetvar(1,Z),tSetvar(2,Z),	\
			      tOr(tVar(1),tVar(2)))))		
			     

  ADD_EFUN2("`&",f_and,
	    tOr4(
	       tFunc(tSetvar(0,tMix),tVar(0)),

	       tOr(tFuncV(tMix tObj,tMix,tMix),
		   tFuncV(tObj tMix,tMix,tMix)),
	       
	       tOr6( F_AND_TYPE(tInt),
		     F_AND_TYPE(tArray),
		     F_AND_TYPE(tMapping),
		     F_AND_TYPE(tMultiset),
		     F_AND_TYPE(tString),
		     F_AND_TYPE(tOr(tType(tMix),tPrg(tObj))) ),

	       tIfnot(tFuncV(tNone, tNot(tMapping), tMix),
		      tFuncV(tNone,
			     tOr3(tArray,tMultiset,tSetvar(4,tMapping)),
			     tVar(4)) )
	       ),
	       
	    OPT_TRY_OPTIMIZE,optimize_binary,generate_and);

#define LOG_TYPE								\
  tOr7(tOr(tFuncV(tMix tObj,tMix,tMix),						\
	   tFuncV(tObj,tMix,tMix)),						\
       tFuncV(tInt,tInt,tInt),							\
       tFuncV(tSetvar(1,tMapping),tSetvar(2,tMapping),tOr(tVar(1),tVar(2))),	\
       tFuncV(tSetvar(3,tMultiset),tSetvar(4,tMultiset),tOr(tVar(3),tVar(4))),	\
       tFuncV(tSetvar(5,tArray),tSetvar(6,tArray),tOr(tVar(5),tVar(6))),	\
       tFuncV(tString,tString,tString),						\
       tFuncV(tOr(tType(tMix),tPrg(tObj)),tOr(tType(tMix),tPrg(tObj)),tType(tMix)))

  ADD_EFUN2("`|",f_or,LOG_TYPE,OPT_TRY_OPTIMIZE,optimize_binary,generate_or);

  ADD_EFUN2("`^",f_xor,LOG_TYPE,OPT_TRY_OPTIMIZE,optimize_binary,generate_xor);

#define SHIFT_TYPE							\
   tOr(tAnd(tNot(tFuncV(tNone, tNot(tObj), tMix)),			\
	    tOr(tFunc(tMix tObj,tMix),					\
		tFunc(tObj tMix,tMix))),				\
       tFunc(tInt tInt,tInt))

  ADD_EFUN2("`<<", f_lsh, SHIFT_TYPE, OPT_TRY_OPTIMIZE,
	    may_have_side_effects, generate_lsh);
  ADD_EFUN2("`>>", f_rsh, SHIFT_TYPE, OPT_TRY_OPTIMIZE,
	    may_have_side_effects, generate_rsh);

  /* !function(!object...:mixed)&function(mixed...:mixed)|"
	    "function(array(array(1=mixed)),array(1=mixed):array(1))|"
	    "function(int...:int)|"
	    "!function(int...:mixed)&function(float|int...:float)|"
	    "function(string*,string:string)|"
	    "function(array(0=mixed),int:array(0))|"
	    "function(array(0=mixed),float:array(0))|"
	    "function(string,int:string) 
	    "function(string,float:string) 
  */
  ADD_EFUN2("`*", f_multiply,
	    tOr9(tIfnot(tFuncV(tNone,tNot(tOr(tObj,tMix)),tMix),tFunction),
		 tFunc(tArr(tArr(tSetvar(1,tMix))) 
		       tArr(tSetvar(1,tMix)),tArr(tVar(1))),
		 tFuncV(tInt,tInt,tInt),
		 tIfnot(tFuncV(tNone,tNot(tFlt),tMix),
			tFuncV(tOr(tFlt,tInt),tOr(tFlt,tInt),tFlt)),
		 tFunc(tArr(tStr) tStr,tStr),
		 tFunc(tArr(tSetvar(0,tMix)) tInt,tArr(tVar(0))),
		 tFunc(tArr(tSetvar(0,tMix)) tFlt,tArr(tVar(0))),
		 tFunc(tStr tInt,tStr),
		 tFunc(tStr tFlt,tStr)),
	    OPT_TRY_OPTIMIZE,optimize_binary,generate_multiply);

  /* !function(!object...:mixed)&function(mixed...:mixed)|"
	    "function(int,int...:int)|"
	    "!function(int...:mixed)&function(float|int...:float)|"
	    "function(array(0=mixed),array|int|float...:array(array(0)))|"
	    "function(string,string|int|float...:array(string)) */
  ADD_EFUN2("`/", f_divide,
	    tOr5(tIfnot(tFuncV(tNone,tNot(tOr(tObj,tMix)),tMix),tFunction),
		 tFuncV(tInt, tInt, tInt),
		 tIfnot(tFuncV(tNone, tNot(tFlt), tMix),
			tFuncV(tOr(tFlt,tInt),tOr(tFlt,tInt),tFlt)),
		 tFuncV(tArr(tSetvar(0,tMix)),
			tOr3(tArray,tInt,tFlt),
			tArr(tArr(tVar(0)))),
		 tFuncV(tStr,tOr3(tStr,tInt,tFlt),tArr(tStr))),
	    OPT_TRY_OPTIMIZE,0,generate_divide);

  /* function(mixed,object:mixed)|"
	    "function(object,mixed:mixed)|"
	    "function(int,int:int)|"
	    "function(string,int:string)|"
	    "function(array(0=mixed),int:array(0))|"
	    "!function(int,int:mixed)&function(int|float,int|float:float) */
  ADD_EFUN2("`%", f_mod,
	    tOr6(tFunc(tMix tObj,tMix),
		 tFunc(tObj tMix,tMix),
		 tFunc(tInt tInt,tInt),
		 tFunc(tStr tInt,tStr),
		 tFunc(tArr(tSetvar(0,tMix)) tInt,tArr(tVar(0))),
		 tIfnot(tFuncV(tNone, tNot(tFlt), tMix),
			tFunc(tOr(tInt,tFlt) tOr(tInt,tFlt),tFlt))),
	    OPT_TRY_OPTIMIZE,0,generate_mod);

  /* function(object:mixed)|function(int:int)|function(float:float)|function(string:string) */
  ADD_EFUN2("`~",f_compl,
	    tOr6(tFunc(tObj,tMix),
		 tFunc(tInt,tInt),
		 tFunc(tFlt,tFlt),
		 tFunc(tStr,tStr),
		 tFunc(tType(tSetvar(0, tMix)), tType(tNot(tVar(0)))),
		 tFunc(tPrg(tObj), tType(tMix))),
	    OPT_TRY_OPTIMIZE,0,generate_compl);
  /* function(string|multiset|array|mapping|object:int) */
  ADD_EFUN2("sizeof", f_sizeof,
	    tFunc(tOr5(tStr,tMultiset,tArray,tMapping,tObj),tInt),
	    OPT_TRY_OPTIMIZE, optimize_sizeof, generate_sizeof);

  /* function(mixed,mixed ...:mixed) */
  ADD_EFUN2("`()",f_call_function,tFuncV(tMix,tMix,tMix),OPT_SIDE_EFFECT | OPT_EXTERNAL_DEPEND,0,generate_call_function);

  /* This one should be removed */
  /* function(mixed,mixed ...:mixed) */
  ADD_EFUN2("call_function",f_call_function,tFuncV(tMix,tMix,tMix),OPT_SIDE_EFFECT | OPT_EXTERNAL_DEPEND,0,generate_call_function);


  start_new_program();
  ADD_STORAGE(struct string_assignment_storage);
  /* function(int:int) */
  ADD_FUNCTION2("`[]", f_string_assignment_index, tFunc(tInt,tInt), 0,
		OPT_EXTERNAL_DEPEND);
  /* function(int,int:int) */
  ADD_FUNCTION2("`[]=", f_string_assignment_assign_index,
		tFunc(tInt tInt,tInt), 0, OPT_SIDE_EFFECT);
  set_init_callback(init_string_assignment_storage);
  set_exit_callback(exit_string_assignment_storage);
  string_assignment_program=end_program();
}


void exit_operators(void)
{
  if(string_assignment_program)
  {
    free_program(string_assignment_program);
    string_assignment_program=0;
  }
}

void o_breakpoint(void)
{
  /* Does nothing */
}
