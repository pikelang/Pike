/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: operators.c,v 1.177 2003/04/28 00:32:43 mast Exp $
*/

#include "global.h"
#include <math.h>
RCSID("$Id: operators.c,v 1.177 2003/04/28 00:32:43 mast Exp $");
#include "interpret.h"
#include "svalue.h"
#include "multiset.h"
#include "mapping.h"
#include "array.h"
#include "stralloc.h"
#include "opcodes.h"
#include "operators.h"
#include "language.h"
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

#define sp Pike_sp

#define OP_DIVISION_BY_ZERO_ERROR(FUNC) \
     math_error(FUNC, sp-2, 2, 0, "Division by zero.\n")
#define OP_MODULO_BY_ZERO_ERROR(FUNC) \
     math_error(FUNC, sp-2, 2, 0, "Modulo by zero.\n")

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
 *!   Inequality operator.
 *!
 *! @returns
 *!   Returns @expr{0@} (zero) if all the arguments are equal, and
 *!   @expr{1@} otherwise.
 *!
 *!   This is the inverse of @[`==()].
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
 *!   Equality operator.
 *!
 *! @returns
 *!   Returns @expr{1@} if all the arguments are equal, and
 *!   @expr{0@} (zero) otherwise.
 *!
 *! @seealso
 *!   @[`!=()]
 */
COMPARISON(f_eq,"`==", is_eq)

/*! @decl int(0..1) `<(mixed arg1, mixed arg2, mixed ... extras)
 *!
 *!   Less than operator.
 *!
 *! @returns
 *!   Returns @expr{1@} if the arguments are strictly increasing, and
 *!   @expr{0@} (zero) otherwise.
 *!
 *! @seealso
 *!   @[`<=()], @[`>()], @[`>=()]
 */
COMPARISON(f_lt,"`<" , is_lt)

/*! @decl int(0..1) `<=(mixed arg1, mixed arg2, mixed ... extras)
 *!
 *!   Less or equal operator.
 *!
 *! @returns
 *!   Returns @expr{1@} if the arguments are not strictly decreasing, and
 *!   @expr{0@} (zero) otherwise.
 *!
 *!   This is the inverse of @[`>()].
 *!
 *! @seealso
 *!   @[`<()], @[`>()], @[`>=()]
 */
COMPARISON(f_le,"`<=",!is_gt)

/*! @decl int(0..1) `>(mixed arg1, mixed arg2, mixed ... extras)
 *!
 *!   Greater than operator.
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
 *! Greater or equal operator.
 *!
 *! @returns
 *!   Returns @expr{1@} if the arguments are not strictly increasing, and
 *!   @expr{0@} (zero) otherwise.
 *!
 *!   This is the inverse of @[`<()].
 *!
 *! @seealso
 *!   @[`<=()], @[`>()], @[`<()]
 */
COMPARISON(f_ge,"`>=",!is_lt)


#define CALL_OPERATOR(OP, args) \
 if(!sp[-args].u.object->prog) \
   bad_arg_error(lfun_names[OP], sp-args, args, 1, "object", sp-args, \
                 "Called in destructed object.\n"); \
 if(FIND_LFUN(sp[-args].u.object->prog,OP) == -1) \
   bad_arg_error(lfun_names[OP], sp-args, args, 1, "object", sp-args, \
                 "Operator not in object.\n"); \
 apply_lfun(sp[-args].u.object, OP, args-1); \
 free_svalue(sp-2); \
 sp[-2]=sp[-1]; \
 sp--; \
 dmalloc_touch_svalue(sp);

/*! @decl mixed `+(mixed arg1)
 *! @decl mixed `+(object arg1, mixed ... extras)
 *! @decl string `+(string arg1, string|int|float arg2)
 *! @decl string `+(int|float arg1, string arg2)
 *! @decl int `+(int arg1, int arg2)
 *! @decl float `+(float arg1, int|float arg2)
 *! @decl float `+(int|float arg1, float arg2)
 *! @decl array `+(array arg1, array arg2)
 *! @decl mapping `+(mapping arg1, mapping arg2)
 *! @decl multiset `+(multiset arg1, multiset arg2)
 *! @decl mixed `+(mixed arg1, mixed arg2, mixed ... extras)
 *!
 *!   Addition operator.
 *!
 *! @returns
 *!   If there's only a single argument, that argument will be returned.
 *!
 *!   If @[arg1] is an object with only one reference and an
 *!   @[lfun::`+=()], that function will be called with the rest of
 *!   the arguments, and its result is returned.
 *!
 *!   Otherwise, if @[arg1] is an object with an @[lfun::`+()], that
 *!   function will be called with the rest of the arguments, and its
 *!   result is returned.
 *!
 *!   Otherwise if any of the other arguments is an object that has
 *!   an @[lfun::``+()] the first such function will be called
 *!   with the arguments leading up to it, and @[`+()] is then called
 *!   recursively with the result and the rest of the arguments.
 *!
 *!   If there are two arguments the result will be:
 *!   @mixed arg1
 *!   	@type string
 *!   	  @[arg2] will be converted to a string, and the result the
 *!   	  strings concatenated.
 *!   	@type int|float
 *!   	  @mixed arg2
 *!   	    @type string
 *!   	      @[arg1] will be converted to string, and the result the
 *!   	      strings concatenated.
 *!   	    @type int|float
 *!   	      The result will be @expr{@[arg1] + @[arg2]@}, and will
 *!   	      be a float if either @[arg1] or @[arg2] is a float.
 *!   	  @endmixed
 *!   	@type array
 *!   	  The arrays will be concatenated.
 *!   	@type mapping
 *!   	  The mappings will be joined.
 *!   	@type multiset
 *!   	  The multisets will be added.
 *!   @endmixed
 *!
 *!   Otherwise if there are more than 2 arguments the result will be:
 *!     @expr{`+(`+(@[arg1], @[arg2]), @@@[extras])@}
 *!
 *! @note
 *!   In Pike 7.0 and earlier the addition order was unspecified.
 *!
 *!   If @[arg1] is @expr{UNDEFINED@} it will behave as the empty
 *!   array/mapping/multiset if needed. This behaviour was new
 *!   in Pike 7.0.
 *!
 *! @seealso
 *!   @[`-()], @[lfun::`+()], @[lfun::``+()]
 */
PMOD_EXPORT void f_add(INT32 args)
{
  INT_TYPE e,size;
  TYPE_FIELD types;

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
	if(sp[-args].type == T_OBJECT && sp[-args].u.object->prog)
	{
	  if(sp[-args].u.object->refs==1 &&
	     FIND_LFUN(sp[-args].u.object->prog,LFUN_ADD_EQ) != -1)
	  {
	    apply_lfun(sp[-args].u.object, LFUN_ADD_EQ, args-1);
	    stack_pop_keep_top();
	    return;
	  }
	  if(FIND_LFUN(sp[-args].u.object->prog,LFUN_ADD) != -1)
	  {
	    apply_lfun(sp[-args].u.object, LFUN_ADD, args-1);
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
	     sp[e-args].u.object->prog &&
	     FIND_LFUN(sp[e-args].u.object->prog,LFUN_RADD) != -1)
	  {
	    struct svalue *tmp=sp+e-args;
	    check_stack(e);
	    assign_svalues_no_free(sp, sp-args, e, -1);
	    sp+=e;
	    apply_lfun(tmp->u.object, LFUN_RADD, e);
	    if(args - e > 1)
	    {
	      assign_svalue(tmp, sp-1);
	      pop_stack();
	      f_add(args - e);
	      assign_svalue(sp-e-1,sp-1);
	      pop_n_elems(e);
	    }else{
	      assign_svalue(sp-args-1,sp-1);
	      pop_n_elems(args);
	    }
	    return;
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
	sprintf(buffer,"%ld",(long)sp[e].u.integer);
	goto append_buffer;

      case T_FLOAT:
	sprintf(buffer,"%f",(double)sp[e].u.float_number);
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
    FLOAT_TYPE sum;
    sum=0.0;
    for(e=-args; e<0; e++) sum+=sp[e].u.float_number;
    sp-=args-1;
    sp[-1].u.float_number=sum;
    break;
  }

  case BIT_FLOAT|BIT_INT:
  {
    FLOAT_TYPE sum;
    sum=0.0;
    for(e=-args; e<0; e++)
    {
      if(sp[e].type==T_FLOAT)
      {
	sum+=sp[e].u.float_number;
      }else{
	sum+=(FLOAT_TYPE)sp[e].u.integer;
      }
    }
    sp-=args-1;
    sp[-1].type=T_FLOAT;
    sp[-1].u.float_number=sum;
    break;
  }

  case BIT_ARRAY|BIT_INT:
  {
    if(IS_UNDEFINED(sp-args))
    {
      int e;
      struct array *a;

      for(e=1;e<args;e++)
	if(sp[e-args].type != T_ARRAY)
	  SIMPLE_BAD_ARG_ERROR("`+", e+1, "array");
      
      a=add_arrays(sp-args+1,args-1);
      pop_n_elems(args);
      push_array(a);
      return;
    }
    if (sp[-args].type == T_INT) {
      int e;
      for(e=1;e<args;e++)
	if (sp[e-args].type != T_INT)
	  SIMPLE_BAD_ARG_ERROR("`+", e+1, "int");
    } else {
      int e;
      for(e=0;e<args;e++)
	if (sp[e-args].type != T_ARRAY)
	  SIMPLE_BAD_ARG_ERROR("`+", e+1, "array");
    }
    /* Probably not reached, but... */
    bad_arg_error("`+", sp-args, args, 1, "array", sp-args,
		  "trying to add integers and arrays.\n");
  }
      
  case BIT_ARRAY:
  {
    struct array *a;
    a=add_arrays(sp-args,args);
    pop_n_elems(args);
    push_array(a);
    break;
  }

  case BIT_MAPPING|BIT_INT:
  {
    if(IS_UNDEFINED(sp-args))
    {
      int e;
      struct mapping *a;

      for(e=1;e<args;e++)
	if(sp[e-args].type != T_MAPPING)
	  SIMPLE_BAD_ARG_ERROR("`+", e+1, "mapping");

      a=add_mappings(sp-args+1,args-1);
      pop_n_elems(args);
      push_mapping(a);
      return;
    }
    if (sp[-args].type == T_INT) {
      int e;
      for(e=1;e<args;e++)
	if (sp[e-args].type != T_INT)
	  SIMPLE_BAD_ARG_ERROR("`+", e+1, "int");
    } else {
      int e;
      for(e=0;e<args;e++)
	if (sp[e-args].type != T_MAPPING)
	  SIMPLE_BAD_ARG_ERROR("`+", e+1, "mapping");
    }
    /* Probably not reached, but... */
    bad_arg_error("`+", sp-args, args, 1, "mapping", sp-args,
		  "Trying to add integers and mappings.\n");
  }

  case BIT_MAPPING:
  {
    struct mapping *m;

    m = add_mappings(sp - args, args);
    pop_n_elems(args);
    push_mapping(m);
    break;
  }

  case BIT_MULTISET:
  {
    struct multiset *l;

    l = add_multisets(sp - args, args);
    pop_n_elems(args);
    push_multiset(l);
    break;
  }
  }
}

static int generate_sum(node *n)
{
  node **first_arg, **second_arg;
  switch(count_args(CDR(n)))
  {
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
    else if(first_arg[0]->type == int_type_string &&
       second_arg[0]->type == int_type_string)
    {
      emit0(F_ADD_INTS);
    }
    else
    {
      emit0(F_ADD);
    }
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
	((*first_arg)->token == F_RANGE) &&
	(CADR(*first_arg)->token == F_CONSTANT) &&
	(CADR(*first_arg)->u.sval.type == T_INT) &&
	(!(CADR(*first_arg)->u.sval.u.integer)) &&
	(CDDR(*first_arg)->token == F_CONSTANT) &&
	(CDDR(*first_arg)->u.sval.type == T_INT)) {
      /* str[..c] == "foo" */
      INT_TYPE c = CDDR(*first_arg)->u.sval.u.integer;

      if ((*second_arg)->u.sval.u.string->len == c+1) {
	/* str[..2] == "foo"
	 *   ==>
	 * has_prefix(str, "foo");
	 */
	ADD_NODE_REF2(CAR(*first_arg),
	ADD_NODE_REF2(*second_arg,
	  ret = mkopernode("has_prefix", CAR(*first_arg), *second_arg);
	));
	return ret;
      } else if ((*second_arg)->u.sval.u.string->len <= c) {
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
    TMP_OPT(f_lt, "`>=");
    TMP_OPT(f_gt, "`<=");
    TMP_OPT(f_le, "`>");
    TMP_OPT(f_ge, "`<");
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
  if(sp[-2].type == T_OBJECT &&
     sp[-2].u.object->prog &&
     FIND_LFUN(sp[-2].u.object->prog,left) != -1)
  {
    apply_lfun(sp[-2].u.object, left, 1);
    free_svalue(sp-2);
    sp[-2]=sp[-1];
    sp--;
    dmalloc_touch_svalue(sp);
    return 1;
  }

  if(sp[-1].type == T_OBJECT &&
     sp[-1].u.object->prog &&
     FIND_LFUN(sp[-1].u.object->prog,right) != -1)
  {
    push_svalue(sp-2);
    apply_lfun(sp[-2].u.object, right, 1);
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

#ifdef PIKE_NEW_MULTISETS
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
#else
	     m=merge_mapping_array_ordered(sp[-2].u.mapping,
					   sp[-1].u.multiset->ind,
					   PIKE_ARRAY_OP_SUB);
#endif

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

  default:
    {
      int args = 2;
      SIMPLE_BAD_ARG_ERROR("`-", 1,
			   "int|float|string|mapping|multiset|array|object");
    }
  }
}

/*! @decl mixed `-(mixed arg1)
 *! @decl mixed `-(object arg1, mixed arg2)
 *! @decl mixed `-(mixed arg1, mixed arg2)
 *! @decl mapping `-(mapping arg1, array arg2)
 *! @decl mapping `-(mapping arg1, multiset arg2)
 *! @decl mapping `-(mapping arg1, mapping arg2)
 *! @decl array `-(array arg1, array arg2)
 *! @decl multiset `-(multiset arg1, multiset arg2)
 *! @decl float `-(float arg1, int|float arg2)
 *! @decl float `-(int arg1, float arg2)
 *! @decl int `-(int arg1, int arg2)
 *! @decl string `-(string arg1, string arg2)
 *! @decl mixed `-(mixed arg1, mixed arg2, mixed ... extras)
 *!
 *!   Negation/subtraction operator.
 *!
 *! @returns
 *!   If there's only a single argument, that argument will be returned
 *!   negated. If @[arg1] was an object, @expr{@[arg1]::`-()@} will be called
 *!   without arguments.
 *!
 *!   If there are more than two arguments the result will be:
 *!   @expr{`-(`-(@[arg1], @[arg2]), @@@[extras])@}.
 *!
 *!   If @[arg1] is an object that overloads @expr{`-()@}, that function will
 *!   be called with @[arg2] as the single argument.
 *!
 *!   If @[arg2] is an object that overloads @expr{``-()@}, that function will
 *!   be called with @[arg1] as the single argument.
 *!
 *!   Otherwise the result will be as follows:
 *!   @mixed arg1
 *!   	@type mapping
 *!   	  @mixed arg2
 *!   	    @type array
 *!   	      The result will be @[arg1] with all occurrances of
 *!   	      @[arg2] removed.
 *!   	    @type multiset|mapping
 *!   	      The result will be @[arg1] with all occurrences of
 *!   	      @expr{@[indices](@[arg2])@} removed.
 *!   	  @endmixed
 *!   	@type array|multiset
 *!   	  The result will be the elements of @[arg1] that are not in @[arg2].
 *!   	@type float|int
 *!   	  The result will be @expr{@[arg1] - @[arg2]@}, and will be a float
 *!   	  if either @[arg1] or @[arg2] is a float.
 *!   	@type string
 *!   	  Result will be the string @[arg1] with all occurrances of the
 *!   	  substring @[arg2] removed.
 *!   @endmixed
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

#ifdef PIKE_NEW_MULTISETS
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
#else
	      m=merge_mapping_array_ordered(sp[-2].u.mapping,
					    sp[-1].u.multiset->ind,
					    PIKE_ARRAY_OP_AND);
#endif

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
    /* This method can be used for types where a op b === b op a */
    case T_MULTISET:
#ifndef PIKE_NEW_MULTISETS
    {
      int e=-1;
      while(args > 1)
      {
	struct svalue tmp;
	func();
	args--;
	e++;
	if(e - args >= -1)
	{
	  e=0;
	}else{
	  tmp=sp[e-args];
	  sp[e-args]=sp[-1];
	  sp[-1]=tmp;
	}
      }
      return;
    }
#endif
    
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
 *! @decl mixed `&(object arg1, mixed arg2)
 *! @decl mixed `&(mixed arg1, object arg2)
 *! @decl int `&(int arg1, int arg2)
 *! @decl array `&(array arg1, array arg2)
 *! @decl multiset `&(multiset arg1, multiset arg2)
 *! @decl mapping `&(mapping arg1, mapping arg2)
 *! @decl string `&(string arg1, string arg2)
 *! @decl type `&(type|program arg1, type|program arg2)
 *! @decl mapping `&(mapping arg1, array arg2)
 *! @decl mapping `&(array arg1, mapping arg2)
 *! @decl mapping `&(mapping arg1, multiset arg2)
 *! @decl mapping `&(multiset arg1, mapping arg2)
 *! @decl mixed `&(mixed arg1, mixed arg2, mixed ... extras)
 *!
 *!   Bitwise and/intersection operator.
 *!
 *! @returns
 *!   If there's a single argument, that argument will be returned.
 *!
 *!   If there are more than two arguments, the result will be:
 *!   @expr{`&(`&(@[arg1], @[arg2]), @@@[extras])@}.
 *!
 *!   If @[arg1] is an object that has an @[lfun::`&()], that function
 *!   will be called with @[arg2] as the single argument.
 *! 
 *!   If @[arg2] is an object that has an @[lfun::``&()], that function
 *!   will be called with @[arg1] as the single argument.
 *!
 *!   Otherwise the result will be as follows:
 *!   @mixed arg1
 *!   	@type int
 *!   	  The result will be the bitwise and of @[arg1] and @[arg2].
 *!   	@type array|multiset|mapping
 *!   	  The result will be the elements of @[arg1] and @[arg2] that
 *!   	  occurr in both.
 *!   	@type type|program
 *!   	  The result will be the type intersection of @[arg1] and @[arg2].
 *!   	@type string
 *!   	  The result will be the string where the elements of @[arg1]
 *!   	  and @[arg2] have been pairwise bitwise anded.
 *!   @endmixed
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
    l=merge_multisets(sp[-2].u.multiset, sp[-1].u.multiset, PIKE_ARRAY_OP_OR);
    pop_n_elems(2);
    push_multiset(l);
    return;
  }
    
  case T_ARRAY:
  {
    struct array *a;
    a=merge_array_with_order(sp[-2].u.array, sp[-1].u.array, PIKE_ARRAY_OP_OR);
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
 *! @decl mixed `|(object arg1, mixed arg2)
 *! @decl mixed `|(mixed arg1, object arg2)
 *! @decl int `|(int arg1, int arg2)
 *! @decl mapping `|(mapping arg1, mapping arg2)
 *! @decl multiset `|(multiset arg1, multiset arg2)
 *! @decl array `|(array arg1, array arg2)
 *! @decl string `|(string arg1, string arg2)
 *! @decl type `|(program|type arg1, program|type arg2)
 *! @decl mixed `|(mixed arg1, mixed arg2, mixed ... extras)
 *!
 *!   Bitwise or/join operator.
 *!
 *! @returns
 *!   If there's a single argument, that argument will be returned.
 *!
 *!   If there are more than two arguments, the result will be:
 *!   @expr{`|(`|(@[arg1], @[arg2]), @@@[extras])@}.
 *!
 *!   If @[arg1] is an object that has an @[lfun::`|()], that function
 *!   will be called with @[arg2] as the single argument.
 *! 
 *!   If @[arg2] is an object that has an @[lfun::``|()], that function
 *!   will be called with @[arg1] as the single argument.
 *!
 *!   Otherwise the result will be as follows:
 *!   @mixed arg1
 *!   	@type int
 *!   	  The result will be the binary or of @[arg1] and @[arg2].
 *!   	@type mapping|multiset
 *!   	  The result will be the join of @[arg1] and @[arg2].
 *!   	@type array
 *!   	  The result will be the concatenation of @[arg1] and @[arg2].
 *!   	@type string
 *!   	  The result will be the pairwise bitwose or of @[arg1] and @[arg2].
 *!   	@type type|program
 *!   	  The result will be the type join of @[arg1] and @[arg2].
 *!   @endmixed
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
 *! @decl mixed `^(object arg1, mixed arg2)
 *! @decl mixed `^(mixed arg1, object arg2)
 *! @decl int `^(int arg1, int arg2)
 *! @decl mapping `^(mapping arg1, mapping arg2)
 *! @decl multiset `^(multiset arg1, multiset arg2)
 *! @decl array `^(array arg1, array arg2)
 *! @decl string `^(string arg1, string arg2)
 *! @decl type `^(program|type arg1, program|type arg2)
 *! @decl mixed `^(mixed arg1, mixed arg2, mixed ... extras)
 *!
 *!   Exclusive or operator.
 *!
 *! @returns
 *!   If there's a single argument, that argument will be returned.
 *!
 *!   If there are more than two arguments, the result will be:
 *!   @expr{`^(`^(@[arg1], @[arg2]), @@@[extras])@}.
 *!
 *!   If @[arg1] is an object that has an @[lfun::`^()], that function
 *!   will be called with @[arg2] as the single argument.
 *! 
 *!   If @[arg2] is an object that has an @[lfun::``^()], that function
 *!   will be called with @[arg1] as the single argument.
 *!
 *!   Otherwise the result will be as follows:
 *!   @mixed arg1
 *!   	@type int
 *!   	  The result will be the bitwise xor of @[arg1] and @[arg2].
 *!   	@type mapping|multiset|array
 *!   	  The result will be the elements of @[arg1] and @[arg2] that
 *!   	  only occurr in one of them.
 *!   	@type string
 *!   	  The result will be the pairwise bitwise xor of @[arg1] and @[arg2].
 *!   	@type type|program
 *!   	  The result will be the result of
 *!   	  @expr{(@[arg1]&~@[arg2])|(~@[arg1]&@[arg2])@}.
 *!   @endmixed
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
 *!   Left shift operator.
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
 *!   Right shift operator.
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
      (FLOAT_TYPE) sp[-1].u.integer * (FLOAT_TYPE)sp[0].u.float_number;
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
 *!   Multiplication operator.
 *!
 *! @returns
 *!   If there's only a single argument, that argument will be returned.
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
	double len;

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
	ptrdiff_t size,e,len,pos;

	len=sp[-1].u.integer;
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
	double len;

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
 *!   Division operator.
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

	tmp=sp[-1].u.integer;
	if(tmp<0)
	{
	  tmp=s->len % -tmp;
	  base=0;
	}else{
	  tmp=s->len % tmp;
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
	INT32 tmp,base;
	if(!sp[-1].u.integer)
	  OP_MODULO_BY_ZERO_ERROR("`%");

	tmp=sp[-1].u.integer;
	if(tmp<0)
	{
	  tmp=a->size % -tmp;
	  base=0;
	}else{
	  tmp=a->size % tmp;
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
 *!   Modulo operator.
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
 *!   Negation operator.
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
 *!   Complement operator.
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

PMOD_EXPORT void o_range(void)
{
  ptrdiff_t from,to;

  if(sp[-3].type==T_OBJECT)
  {
    CALL_OPERATOR(LFUN_INDEX, 3);
    return;
  }

  if(sp[-2].type != T_INT)
    PIKE_ERROR("`[]", "Bad argument 2 to [ .. ]\n", sp, 3);

  if(sp[-1].type != T_INT)
    PIKE_ERROR("`[]", "Bad argument 3 to [ .. ]\n", sp, 3);

  from = sp[-2].u.integer;
  if(from<0) from = 0;
  to = sp[-1].u.integer;
  if(to<from-1) to = from-1;
  dmalloc_touch_svalue(Pike_sp-1);
  dmalloc_touch_svalue(Pike_sp-2);
  sp-=2;

  switch(sp[-1].type)
  {
  case T_STRING:
  {
    struct pike_string *s;
    if(to >= sp[-1].u.string->len-1)
    {
      if(from==0) return;
      to = sp[-1].u.string->len-1;

      if(from>to+1) from=to+1;
    }
#ifdef PIKE_DEBUG
    if(from < 0 || (to-from+1) < 0)
      Pike_fatal("Error in o_range.\n");
#endif

    s=string_slice(sp[-1].u.string, from, to-from+1);
    free_string(sp[-1].u.string);
    sp[-1].u.string=s;
    break;
  }

  case T_ARRAY:
  {
    struct array *a;
    if(to >= sp[-1].u.array->size-1)
    {
      to = sp[-1].u.array->size-1;

      if(from>to+1) from=to+1;
    }

    a = slice_array(sp[-1].u.array, from, to+1);
    free_array(sp[-1].u.array);
    sp[-1].u.array=a;
    break;
  }
    
  default:
    PIKE_ERROR("`[]", "[ .. ] on non-scalar type.\n", sp, 3);
  }
}

/*! @decl mixed `[](object arg, mixed index)
 *! @decl mixed `[](object arg, string index)
 *! @decl mixed `[](int arg, string index)
 *! @decl mixed `[](array arg, int index)
 *! @decl mixed `[](array arg, mixed index)
 *! @decl mixed `[](mapping arg, mixed index)
 *! @decl int(0..1) `[](multiset arg, mixed index)
 *! @decl int `[](string arg, int index)
 *! @decl mixed `[](program arg, string index)
 *! @decl mixed `[](object arg, mixed start, mixed end)
 *! @decl string `[](string arg, int start, int end)
 *! @decl array `[](array arg, int start, int end)
 *!
 *!   Index and range operator.
 *!
 *! @returns
 *!   If @[arg] is an object that implements @[lfun::`[]()], that function
 *!   will be called with the rest of the arguments.
 *!
 *!   If there are 2 arguments the result will be as follows:
 *!   @mixed arg
 *!   	@type object
 *!   	  The non-static (ie public) symbol named @[index] will be looked up
 *!   	  in @[arg].
 *!   	@type int
 *!   	  The bignum function named @[index] will be looked up in @[arg].
 *!   	@type array
 *!   	  If @[index] is an int, index number @[index] of @[arg] will be
 *!   	  returned. Otherwise an array of all elements in @[arg] indexed
 *!   	  with @[index] will be returned.
 *!   	@type mapping
 *!   	  If @[index] exists in @[arg] the corresponding value will be
 *!       returned. Otherwise @expr{UNDEFINED@} will be returned.
 *!   	@type multiset
 *!   	  If @[index] exists in @[arg], @expr{1@} will be returned.
 *!   	  Otherwise @expr{UNDEFINED@} will be returned.
 *!   	@type string
 *!   	  The character (int) at index @[index] in @[arg] will be returned.
 *!   	@type program
 *!   	  The non-static (ie public) constant symbol @[index] will be
 *!   	  looked up in @[arg].
 *!   @endmixed
 *!
 *!   Otherwise if there are 3 arguments the result will be as follows:
 *!   @mixed arg
 *!   	@type string
 *!   	  A string with the characters between @[start] and @[end] (inclusive)
 *!   	  in @[arg] will be returned.
 *!   	@type array
 *!   	  An array with the elements between @[start] and @[end] (inclusive)
 *!   	  in @[arg] will be returned.
 *!   @endmixed
 *!
 *! @seealso
 *!   @[`->()], @[lfun::`[]()]
 */
PMOD_EXPORT void f_index(INT32 args)
{
  switch(args)
  {
  case 0:
  case 1:
    PIKE_ERROR("`[]", "Too few arguments.\n", sp, args);
    break;
  case 2:
    if(sp[-1].type==T_STRING) sp[-1].subtype=0;
    o_index();
    break;
  case 3:
    o_range();
    break;
  default:
    PIKE_ERROR("`[]", "Too many arguments.\n", sp, args);
  }
}

/*! @decl mixed `->(object arg, string index)
 *! @decl mixed `->(int arg, string index)
 *! @decl mixed `->(array arg, string index)
 *! @decl mixed `->(mapping arg, string index)
 *! @decl int(0..1) `->(multiset arg, string index)
 *! @decl mixed `->(program arg, string index)
 *!
 *!   Arrow index operator.
 *!
 *!   This function behaves much like @[`[]()], just that the index is always
 *!   a string.
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
 *! @seealso
 *!   @[`[]()], @[lfun::`->()], @[::`->()]
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
 *!   Index assign operator.
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
 *! @seealso
 *!   @[`->=()], @[lfun::`[]=()]
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
 *!   Arrow assign operator.
 *!
 *!   This function behaves much like @[`[]=()], just that the index is always
 *!   a string.
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
 *! @seealso
 *!   @[`[]=()], @[lfun::`->=()]
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
 *!   Sizeof operator.
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
/*! @decl int `[](int i, int j)
 *!
 *! String index operator.
 */
static void f_string_assignment_index(INT32 args)
{
  INT_TYPE i;
  get_all_args("string[]",args,"%i",&i);
  if(i<0) i+=THIS->s->len;
  if(i<0)
    i+=THIS->s->len;
  if(i<0 || i>=THIS->s->len)
    Pike_error("Index %"PRINTPIKEINT"d is out of range 0 - %ld.\n",
	       i, PTRDIFF_T_TO_LONG(THIS->s->len - 1));
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
  INT_TYPE i,j;
  union anything *u;
  get_all_args("string[]=",args,"%i%i",&i,&j);
  if((u=get_pointer_if_this_type(THIS->lval, T_STRING)))
  {
    free_string(THIS->s);
    if(i<0) i+=u->string->len;
    if(i<0 || i>=u->string->len)
      Pike_error("String index out of range %ld\n",(long)i);
    u->string=modify_shared_string(u->string,i,j);
    copy_shared_string(THIS->s, u->string);
  }else{
    lvalue_to_svalue_no_free(sp,THIS->lval);
    sp++;
    dmalloc_touch_svalue(Pike_sp-1);
    if(sp[-1].type != T_STRING) Pike_error("string[]= failed.\n");
    if(i<0) i+=sp[-1].u.string->len;
    if(i<0 || i>=sp[-1].u.string->len)
      Pike_error("String index out of range %ld\n",(long)i);
    sp[-1].u.string=modify_shared_string(sp[-1].u.string,i,j);
    assign_lvalue(THIS->lval, sp-1);
    pop_stack();
  }
  pop_n_elems(args);
  push_int(j);
}


static void init_string_assignment_storage(struct object *o)
{
  THIS->lval[0].type=T_INT;
  THIS->lval[1].type=T_INT;
  THIS->s=0;
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
  /* function(string,int:int)|function(object,string:mixed)|function(array(0=mixed),int:0)|function(mapping(mixed:1=mixed),mixed:1)|function(multiset,mixed:int)|function(string,int,int:string)|function(array(2=mixed),int,int:array(2))|function(program:mixed) */
  ADD_EFUN2("`[]",f_index,tOr7(tFunc(tStr tInt,tInt),tFunc(tObj tStr,tMix),tFunc(tArr(tSetvar(0,tMix)) tInt,tVar(0)),tFunc(tMap(tMix,tSetvar(1,tMix)) tMix,tVar(1)),tFunc(tMultiset tMix,tInt),tFunc(tStr tInt tInt,tStr),tOr(tFunc(tArr(tSetvar(2,tMix)) tInt tInt,tArr(tVar(2))),tFunc(tPrg(tObj),tMix))),OPT_TRY_OPTIMIZE,0,0);

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
