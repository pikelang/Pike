/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
/**/
#include "global.h"
#include <math.h>
RCSID("$Id: operators.c,v 1.123 2003/05/15 15:08:38 mast Exp $");
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

PMOD_EXPORT void f_ne(INT32 args)
{
  f_eq(args);
  o_not();
}

COMPARISON(f_eq,"`==", is_eq)
COMPARISON(f_lt,"`<" , is_lt)
COMPARISON(f_le,"`<=",!is_gt)
COMPARISON(f_gt,"`>" , is_gt)
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
	    stack_unlink(1);
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
      }
      /* Perform the addition. */
      f_add(args+e);
      save_sp[--e] = *(--sp);
#ifdef PIKE_DEBUG
      if (sp != save_sp) {
	fatal("f_add(): Lost track of stack %p != %p\n", sp, save_sp);
      }
#endif /* PIKE_DEBUG */
      /* Perform the rest of the addition. */
      f_add(-e);
#ifdef PIKE_DEBUG
      if (sp != save_sp + 1 + e) {
	fatal("f_add(): Lost track of stack (2) %p != %p\n",
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
      fatal("Couldn't find argument!\n");
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
  }
  return 0;
}

static node *optimize_not(node *n)
{
  node **first_arg, **more_args;
  int e;

  if(count_args(CDR(n))==1)
  {
    first_arg=my_get_arg(&_CDR(n), 0);
#ifdef PIKE_DEBUG
    if(!first_arg)
      fatal("Couldn't find argument!\n");
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


static node *optimize_binary(node *n)
{
  node **first_arg, **second_arg, *ret;
  if(count_args(CDR(n))==2)
  {
    first_arg=my_get_arg(&_CDR(n), 0);
    second_arg=my_get_arg(&_CDR(n), 1);

#ifdef PIKE_DEBUG
    if(!first_arg || !second_arg)
      fatal("Couldn't find argument!\n");
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
      fatal("Count args was wrong in generate_comparison.\n");

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
      fatal("Couldn't generate comparison!\n");
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
    push_constant_text(tFloat);
    stack_swap();
    f_cast();
    stack_swap();
    return 1;
  }
  else if(is_bignum_object_in_svalue(sp-1) && sp[-2].type==T_FLOAT)
  {
    push_constant_text(tFloat);
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

	     m=merge_mapping_array_ordered(sp[-2].u.mapping,
					   sp[-1].u.multiset->ind,
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

  default:
    {
      int args = 2;
      SIMPLE_BAD_ARG_ERROR("`-", 1,
			   "int|float|string|mapping|multiset|array|object");
    }
  }
}

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
	   push_type_int(p->id);
	   push_type(0);
	   push_type(T_OBJECT);
	   free_svalue(sp - 2);
	   sp[-2].u.string = pop_unfinished_type();
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
	   push_type_int(p->id);
	   push_type(0);
	   push_type(T_OBJECT);
	   free_svalue(sp - 1);
	   sp[-1].u.string = pop_unfinished_type();
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

	      m=merge_mapping_array_ordered(sp[-2].u.mapping,
					    sp[-1].u.multiset->ind,
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
    struct pike_string *t;
    t = and_pike_types(sp[-2].u.string, sp[-1].u.string);
    pop_n_elems(2);
    push_string(t);
    sp[-1].type = T_TYPE;
    return;
  }

  case T_FUNCTION:
  case T_PROGRAM:
  {
    struct program *p;
    struct pike_string *a;
    struct pike_string *b;
    struct pike_string *t;

    p = program_from_svalue(sp - 2);
    if (!p) {
      int args = 2;
      SIMPLE_BAD_ARG_ERROR("`&", 1, "type");
    }    
    type_stack_mark();
    push_type_int(p->id);
    push_type(0);
    push_type(T_OBJECT);
    a = pop_unfinished_type();

    p = program_from_svalue(sp - 1);
    if (!p) {
      int args = 2;
      SIMPLE_BAD_ARG_ERROR("`&", 2, "type");
    }    
    type_stack_mark();
    push_type_int(p->id);
    push_type(0);
    push_type(T_OBJECT);
    b = pop_unfinished_type();

    t = and_pike_types(a, b);

    pop_n_elems(2);
    push_string(t);
    sp[-1].type = T_TYPE;
    free_string(a);
    free_string(b);
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
    PIKE_ERROR("`&", "Bitwise and on illegal type.\n", sp, 2);
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
  int num;
  struct svalue tmp;
  ONERROR err;

  switch(args)
  {
    case 3: func();
    case 2: func();
    case 1: return;

    default:
      r_speedup((args+1)>>1,func);
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
	push_type_int(p->id);
	push_type(0);
	push_type(T_OBJECT);
	free_svalue(sp - 2);
	sp[-2].u.string = pop_unfinished_type();
	sp[-2].type = T_TYPE;
      }
      if (sp[-1].type != T_TYPE) {
	struct program *p = program_from_svalue(sp - 1);
	if (!p) {
	  int args = 2;
	  SIMPLE_BAD_ARG_ERROR("`|", 2, "type");
	}
	type_stack_mark();
	push_type_int(p->id);
	push_type(0);
	push_type(T_OBJECT);
	free_svalue(sp - 1);
	sp[-1].u.string = pop_unfinished_type();
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
    struct pike_string *t;
    t = or_pike_types(sp[-2].u.string, sp[-1].u.string, 0);
    pop_n_elems(2);
    push_string(t);
    sp[-1].type = T_TYPE;
    return;
  }

  case T_FUNCTION:
  case T_PROGRAM:
  {
    struct program *p;
    struct pike_string *a;
    struct pike_string *b;
    struct pike_string *t;

    p = program_from_svalue(sp - 2);
    if (!p) {
      int args = 2;
      SIMPLE_BAD_ARG_ERROR("`|", 1, "type");
    }
    type_stack_mark();
    push_type_int(p->id);
    push_type(0);
    push_type(T_OBJECT);
    a = pop_unfinished_type();

    p = program_from_svalue(sp - 1);
    if (!p) {
      int args = 2;
      SIMPLE_BAD_ARG_ERROR("`|", 2, "type");
    }
    type_stack_mark();
    push_type_int(p->id);
    push_type(0);
    push_type(T_OBJECT);
    b = pop_unfinished_type();

    t = or_pike_types(a, b, 0);

    pop_n_elems(2);
    push_string(t);
    sp[-1].type = T_TYPE;
    free_string(a);
    free_string(b);
    return;
  }

  STRING_BITOP(|,"OR")

  default:
    PIKE_ERROR("`|", "Bitwise or on illegal type.\n", sp, 2);
  }
}

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
	push_type_int(p->id);
	push_type(0);
	push_type(T_OBJECT);
	free_svalue(sp - 2);
	sp[-2].u.string = pop_unfinished_type();
	sp[-2].type = T_TYPE;
      }
      if (sp[-1].type != T_TYPE) {
	struct program *p = program_from_svalue(sp - 1);
	if (!p) {
	  int args = 2;
	  SIMPLE_BAD_ARG_ERROR("`^", 2, "type");
	}
	type_stack_mark();
	push_type_int(p->id);
	push_type(0);
	push_type(T_OBJECT);
	free_svalue(sp - 1);
	sp[-1].u.string = pop_unfinished_type();
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
    push_type_int(p->id);
    push_type(0);
    push_type(T_OBJECT);
    pop_stack();
    push_string(pop_unfinished_type());
    sp[-1].type = T_TYPE;

    stack_swap();

    p = program_from_svalue(sp - 1);
    if (!p) {
      int args = 2;
      stack_swap();
      SIMPLE_BAD_ARG_ERROR("`^", 1, "type");
    }
    type_stack_mark();
    push_type_int(p->id);
    push_type(0);
    push_type(T_OBJECT);
    pop_stack();
    push_string(pop_unfinished_type());
    sp[-1].type = T_TYPE;
  }
  /* FALL_THROUGH */
  case T_TYPE:
  {
    /* a ^ b  ==  (a&~b)|(~a&b) */
    struct pike_string *a;
    struct pike_string *b;
    copy_shared_string(a, sp[-2].u.string);
    copy_shared_string(b, sp[-1].u.string);
    o_compl();		/* ~b */
    o_and();		/* a&~b */
    push_string(a);
    sp[-1].type = T_TYPE;
    o_compl();		/* ~a */
    push_string(b);
    sp[-1].type = T_TYPE;
    o_and();		/* ~a&b */
    o_or();		/* (a&~b)|(~a&b) */
    return;
  }

  STRING_BITOP(^,"XOR")

  default:
    PIKE_ERROR("`^", "Bitwise XOR on illegal type.\n", sp, 2);
  }
}

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
  if((sp[-1].type == T_INT) && (sp[-2].type == T_INT) &&
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
    SIMPLE_BAD_ARG_ERROR("`<<", 2, "int|object");
  }
  sp--;
  sp[-1].u.integer = sp[-1].u.integer << sp->u.integer;
}

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
    SIMPLE_BAD_ARG_ERROR("`>>", 2, "int|object");
  }
  
#ifdef AUTO_BIGNUM
  if(INT_TYPE_RSH_OVERFLOW(sp[-2].u.integer, sp[-1].u.integer))
  {
    sp--;
    sp[-1].u.integer = 0;
    return;
  }
#endif /* AUTO_BIGNUM */
  
  sp--;
  sp[-1].u.integer = sp[-1].u.integer >> sp->u.integer;
}

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
	  assign_svalues_no_free(pos,
				 src->item,
				 asize,
				 src->type_field);
	  array_fix_type_field(ret);
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
    foo=sp[-1].u.float_number / sp[0].u.float_number;
    foo=sp[-1].u.float_number - sp[0].u.float_number * floor(foo);
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
    if(IS_ZERO(sp-1))
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
    if (EXTRACT_UCHAR(sp[-1].u.string->str) == T_NOT) {
      push_unfinished_type(sp[-1].u.string->str + 1);
    } else {
      push_unfinished_type(sp[-1].u.string->str);
      push_type(T_NOT);
    }
    pop_stack();
    push_string(pop_unfinished_type());
    sp[-1].type = T_TYPE;
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
      push_type_int(p->id);
      push_type(0);
      push_type(T_OBJECT);
      push_type(T_NOT);
      pop_stack();
      push_string(pop_unfinished_type());
      sp[-1].type = T_TYPE;
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
      fatal("Error in o_range.\n");
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

PMOD_EXPORT void f_sizeof(INT32 args)
{
  INT32 tmp;
  if(args<1)
    PIKE_ERROR("sizeof", "Too few arguments.\n", sp, args);

  tmp=pike_sizeof(sp-args);

  pop_n_elems(args);
  push_int(tmp);
}

static int generate_sizeof(node *n)
{
  node **arg;
  if(count_args(CDR(n)) != 1) return 0;
  if(do_docode(CDR(n),DO_NOT_COPY) != 1)
    fatal("Count args was wrong in sizeof().\n");
  emit0(F_SIZEOF);
  return 1;
}

extern int generate_call_function(node *n);
struct program *string_assignment_program;

#undef THIS
#define THIS ((struct string_assignment_storage *)(CURRENT_STORAGE))
static void f_string_assignment_index(INT32 args)
{
  INT_TYPE i;
  get_all_args("string[]",args,"%i",&i);
  if(i<0) i+=THIS->s->len;
  if(i<0)
    i+=THIS->s->len;
  if(i<0 || i>=THIS->s->len)
    Pike_error("Index %d is out of range 0 - %ld.\n",
	  i, PTRDIFF_T_TO_LONG(THIS->s->len - 1));
  else
    i=index_shared_string(THIS->s,i);
  pop_n_elems(args);
  push_int(i);
}

static void f_string_assignment_assign_index(INT32 args)
{
  INT_TYPE i,j;
  union anything *u;
  get_all_args("string[]=",args,"%i%i",&i,&j);
  if((u=get_pointer_if_this_type(THIS->lval, T_STRING)))
  {
    if(i<0) i+=u->string->len;
    if(i<0 || i>=u->string->len)
      Pike_error("String index out of range %ld\n",(long)i);
    free_string(THIS->s);
    u->string=modify_shared_string(u->string,i,j);
    copy_shared_string(THIS->s, u->string);
  }else{
    lvalue_to_svalue_no_free(sp,THIS->lval);
    sp++;
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

void init_operators(void)
{
  /* function(string,int:int)|function(object,string:mixed)|function(array(0=mixed),int:0)|function(mapping(mixed:1=mixed),mixed:1)|function(multiset,mixed:int)|function(string,int,int:string)|function(array(2=mixed),int,int:array(2))|function(program:mixed) */
  ADD_EFUN2("`[]",f_index,tOr7(tFunc(tStr tInt,tInt),tFunc(tObj tStr,tMix),tFunc(tArr(tSetvar(0,tMix)) tInt,tVar(0)),tFunc(tMap(tMix,tSetvar(1,tMix)) tMix,tVar(1)),tFunc(tMultiset tMix,tInt),tFunc(tStr tInt tInt,tStr),tOr(tFunc(tArr(tSetvar(2,tMix)) tInt tInt,tArr(tVar(2))),tFunc(tPrg,tMix))),OPT_TRY_OPTIMIZE,0,0);

  /* function(array(object|mapping|multiset|array),string:array(mixed))|function(object|mapping|multiset|program,string:mixed) */
  ADD_EFUN2("`->",f_arrow,tOr(tFunc(tArr(tOr4(tObj,tMapping,tMultiset,tArray)) tStr,tArr(tMix)),tFunc(tOr4(tObj,tMapping,tMultiset,tPrg) tStr,tMix)),OPT_TRY_OPTIMIZE,0,0);

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
		 tFuncV(tOr3(tObj,tProgram,tFunction) tMix,tMix,tInt01),
		 tFuncV(tMix tOr3(tObj,tProgram,tFunction),tMix,tInt01),
		 tFuncV(tType tType,tOr3(tProgram,tFunction,tType),tInt01)),
	    OPT_WEAK_TYPE|OPT_TRY_OPTIMIZE,optimize_eq,generate_comparison);
  /* function(mixed...:int) */
  ADD_EFUN2("`!=",f_ne,
	    tOr5(tFuncV(tOr(tInt,tFloat) tOr(tInt,tFloat),
			tOr(tInt,tFloat),tInt01),
		 tFuncV(tSetvar(0,tOr4(tString,tMapping,tMultiset,tArray))
			tVar(0), tVar(0),tInt01),
		 tFuncV(tOr3(tObj,tProgram,tFunction) tMix,tMix,tInt01),
		 tFuncV(tMix tOr3(tObj,tProgram,tFunction),tMix,tInt01),
		 tFuncV(tType tType,tOr3(tProgram,tFunction,tType),tInt01)),
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
		     F_AND_TYPE(tOr(tType,tPrg)) ),

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
       tFuncV(tOr(tType,tPrg),tOr(tType,tPrg),tType))

  ADD_EFUN2("`|",f_or,LOG_TYPE,OPT_TRY_OPTIMIZE,optimize_binary,generate_or);

  ADD_EFUN2("`^",f_xor,LOG_TYPE,OPT_TRY_OPTIMIZE,optimize_binary,generate_xor);

#define SHIFT_TYPE							\
  tOr(tOr(tFuncV(tMix tObj,tMix,tMix),					\
	  tFuncV(tObj tMix,tMix,tMix)),					\
      tFuncV(tInt,tInt,tInt))

  ADD_EFUN2("`<<",f_lsh,SHIFT_TYPE,OPT_TRY_OPTIMIZE,0,generate_lsh);
  ADD_EFUN2("`>>",f_rsh,SHIFT_TYPE,OPT_TRY_OPTIMIZE,0,generate_rsh);

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
	    tOr5(tFunc(tObj,tMix),
		 tFunc(tInt,tInt),
		 tFunc(tFlt,tFlt),
		 tFunc(tStr,tStr),
		 tFunc(tOr(tType,tProgram),tType)),
	    OPT_TRY_OPTIMIZE,0,generate_compl);
  /* function(string|multiset|array|mapping|object:int) */
  ADD_EFUN2("sizeof", f_sizeof,tFunc(tOr5(tStr,tMultiset,tArray,tMapping,tObj),tInt),0,0,generate_sizeof);

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
