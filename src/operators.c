/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
#include <math.h>
RCSID("$Id: operators.c,v 1.22 1998/07/04 17:18:30 grubba Exp $");
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
#include "error.h"
#include "docode.h"
#include "constants.h"
#include "peep.h"
#include "lex.h"
#include "program.h"
#include "object.h"
#include "pike_types.h"

#define COMPARISON(ID,NAME,EXPR) \
void ID(INT32 args) \
{ \
  int i; \
  if(args > 2) \
    pop_n_elems(args-2); \
  else if(args < 2) \
    error("Too few arguments to %s\n",NAME); \
  i=EXPR; \
  pop_n_elems(2); \
  push_int(i); \
}

COMPARISON(f_eq,"`==", is_eq(sp-2,sp-1))
COMPARISON(f_ne,"`!=",!is_eq(sp-2,sp-1))
COMPARISON(f_lt,"`<" , is_lt(sp-2,sp-1))
COMPARISON(f_le,"`<=",!is_gt(sp-2,sp-1))
COMPARISON(f_gt,"`>" , is_gt(sp-2,sp-1))
COMPARISON(f_ge,"`>=",!is_lt(sp-2,sp-1))


#define CALL_OPERATOR(OP, args) \
 if(!sp[-args].u.object->prog) \
   error("Operator %s called in destructed object.\n",lfun_names[OP]); \
 if(sp[-args].u.object->prog->lfuns[OP] == -1) \
   error("No operator %s in object.\n",lfun_names[OP]); \
 apply_lfun(sp[-args].u.object, OP, args-1); \
 free_svalue(sp-2); \
 sp[-2]=sp[-1]; \
 sp--;


void f_add(INT32 args)
{
  INT32 e,size;
  TYPE_FIELD types;

  types=0;
  for(e=-args;e<0;e++) types|=1<<sp[e].type;
    
  switch(types)
  {
  default:
    if(!args)
    {
      error("Too few arguments to `+()\n");
    }else{
      if(types & BIT_OBJECT)
      {
	if(sp[-args].type == T_OBJECT &&
	  sp[-args].u.object->prog &&
	  sp[-args].u.object->prog->lfuns[LFUN_ADD] != -1)
	{
	  apply_lfun(sp[-args].u.object, LFUN_ADD, args-1);
	  free_svalue(sp-2);
	  sp[-2]=sp[-1];
	  sp--;
	  return;
	}
	for(e=1;e<args;e++)
	{
	  if(sp[e-args].type == T_OBJECT &&
	     sp[e-args].u.object->prog &&
	     sp[e-args].u.object->prog->lfuns[LFUN_RADD] != -1)
	  {
	    struct svalue *tmp=sp+e-args;
	    check_stack(e);
	    assign_svalues_no_free(sp, sp-args, e, -1);
	    sp+=e;
	    apply_lfun(tmp->u.object, LFUN_RADD, e);
	    if(args - e > 2)
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
	error("Bad argument 1 to summation\n");
    }
    error("Incompatible types to sum() or +\n");
    return; /* compiler hint */

  case BIT_STRING:
  {
    struct pike_string *r;
    char *buf;
    INT32 tmp;

    switch(args)
    {
    case 1: return;
    default:
      size=0;
      for(e=-args;e<0;e++) size+=sp[e].u.string->len;

      tmp=sp[-args].u.string->len;
      r=realloc_shared_string(sp[-args].u.string,size);
      sp[-args].type=T_INT;
      buf=r->str+tmp;
      for(e=-args+1;e<0;e++)
      {
	MEMCPY(buf,sp[e].u.string->str,sp[e].u.string->len);
	buf+=sp[e].u.string->len;
      }
      sp[-args].u.string=end_shared_string(r);
      sp[-args].type=T_STRING;
      for(e=-args+1;e<0;e++) free_string(sp[e].u.string);
      sp-=args-1;
    }

    break;
  }

  case BIT_STRING | BIT_INT:
  case BIT_STRING | BIT_FLOAT:
  case BIT_STRING | BIT_FLOAT | BIT_INT:
  {
    struct pike_string *r;
    char *buf,*str;
    size=0;
    for(e=-args;e<0;e++)
    {
      switch(sp[e].type)
      {
      case T_STRING:
	size+=sp[e].u.string->len;
	break;

      case T_INT:
	size+=14;
	break;

      case T_FLOAT:
	size+=22;
	break;
      }
    }
    str=buf=xalloc(size+1);
    size=0;
    
    for(e=-args;e<0;e++)
    {
      switch(sp[e].type)
      {
      case T_STRING:
	MEMCPY(buf,sp[e].u.string->str,sp[e].u.string->len);
	buf+=sp[e].u.string->len;
	break;

      case T_INT:
	sprintf(buf,"%ld",(long)sp[e].u.integer);
	buf+=strlen(buf);
	break;

      case T_FLOAT:
	sprintf(buf,"%f",(double)sp[e].u.float_number);
	buf+=strlen(buf);
	break;
      }
    }
    r=make_shared_binary_string(str,buf-str);
    free(str);
    pop_n_elems(args);
    push_string(r);
    break;
  }

  case BIT_INT:
    size=0;
    for(e=-args; e<0; e++) size+=sp[e].u.integer;
    sp-=args-1;
    sp[-1].u.integer=size;
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

  case BIT_FLOAT | BIT_INT:
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

  case BIT_ARRAY:
  {
    struct array *a;
    a=add_arrays(sp-args,args);
    pop_n_elems(args);
    push_array(a);
    break;
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
  switch(count_args(CDR(n)))
  {
  case 1:
    do_docode(CDR(n),0);
    return 1;

  case 2:
    do_docode(CDR(n),DO_NOT_COPY);
    emit2(F_ADD);
    return 1;

  default:
    return 0;
  }
}

static node *optimize_binary(node *n)
{
  node **first_arg, **second_arg, *ret;
  if(count_args(CDR(n))==2)
  {
    first_arg=my_get_arg(&CDR(n), 0);
    second_arg=my_get_arg(&CDR(n), 1);

#ifdef DEBUG
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
	ret=mknode(F_APPLY,
		   CAR(n),
		   mknode(F_ARG_LIST,
			  CDR(*first_arg),
			  *second_arg));
	CAR(n)=0;
	CDR(*first_arg)=0;
	*second_arg=0;
	
	return ret;
      }
      
      if((*second_arg)->token == F_APPLY &&
	 CAR(*second_arg)->token == F_CONSTANT &&
	 is_eq(& CAR(*second_arg)->u.sval, & CAR(n)->u.sval))
      {
	ret=mknode(F_APPLY,
		   CAR(n),
		   mknode(F_ARG_LIST,
			  *first_arg,
			  CDR(*second_arg)));
	CAR(n)=0;
	*first_arg=0;
	CDR(*second_arg)=0;
	
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
      emit2(F_EQ);
    else if(CAR(n)->u.sval.u.efun->function == f_ne)
      emit2(F_NE);
    else if(CAR(n)->u.sval.u.efun->function == f_lt)
      emit2(F_LT);
    else if(CAR(n)->u.sval.u.efun->function == f_le)
      emit2(F_LE);
    else if(CAR(n)->u.sval.u.efun->function == f_gt)
      emit2(F_GT);
    else if(CAR(n)->u.sval.u.efun->function == f_ge)
      emit2(F_GE);
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
  return 0;
}

static int call_lfun(int left, int right)
{
  if(sp[-2].type == T_OBJECT &&
     sp[-2].u.object->prog &&
     sp[-2].u.object->prog->lfuns[left] != -1)
  {
    apply_lfun(sp[-2].u.object, left, 1);
    free_svalue(sp-2);
    sp[-2]=sp[-1];
    sp--;
    return 1;
  }

  if(sp[-1].type == T_OBJECT &&
     sp[-1].u.object->prog &&
     sp[-1].u.object->prog->lfuns[right] != -1)
  {
    push_svalue(sp-2);
    apply_lfun(sp[-2].u.object, right, 1);
    free_svalue(sp-3);
    sp[-3]=sp[-1];
    sp--;
    pop_stack();
    return 1;
  }

  return 0;
}

void o_subtract(void)
{
  if (sp[-2].type != sp[-1].type && !float_promote())
  {
    if(call_lfun(LFUN_SUBTRACT, LFUN_RSUBTRACT))
      return;
    error("Subtract on different types.\n");
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
    m=merge_mappings(sp[-2].u.mapping, sp[-1].u.mapping,OP_SUB);
    pop_n_elems(2);
    push_mapping(m);
    return;
  }

  case T_MULTISET:
  {
    struct multiset *l;
    l=merge_multisets(sp[-2].u.multiset, sp[-1].u.multiset, OP_SUB);
    pop_n_elems(2);
    push_multiset(l);
    return;
  }

  case T_FLOAT:
    sp--;
    sp[-1].u.float_number -= sp[0].u.float_number;
    return;

  case T_INT:
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
    error("Bad argument 1 to subtraction.\n");
  }
}

void f_minus(INT32 args)
{
  switch(args)
  {
  case 0: error("Too few arguments to `-\n");
  case 1: o_negate(); break;
  case 2: o_subtract(); break;
  default: error("Too many arguments to `-\n");
  }
}

static int generate_minus(node *n)
{
  switch(count_args(CDR(n)))
  {
  case 1:
    do_docode(CDR(n),DO_NOT_COPY);
    emit2(F_NEGATE);
    return 1;

  case 2:
    do_docode(CDR(n),DO_NOT_COPY);
    emit2(F_SUBTRACT);
    return 1;
  }
  return 0;
}

void o_and(void)
{
  if(sp[-1].type != sp[-2].type)
  {
    if(call_lfun(LFUN_AND, LFUN_RAND))
      return;

    error("Bitwise and on different types.\n");
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
    m=merge_mappings(sp[-2].u.mapping, sp[-1].u.mapping, OP_AND);
    pop_n_elems(2);
    push_mapping(m);
    return;
  }

  case T_MULTISET:
  {
    struct multiset *l;
    l=merge_multisets(sp[-2].u.multiset, sp[-1].u.multiset, OP_AND);
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
  case T_STRING:
  {
    struct pike_string *s;
    INT32 len, i;

    len = sp[-2].u.string->len;
    if (len != sp[-1].u.string->len)
      error("Bitwise AND on strings of different lengths.\n");
    s = begin_shared_string(len);
    for (i=0; i<len; i++)
      s->str[i] = sp[-2].u.string->str[i] & sp[-1].u.string->str[i];
    pop_n_elems(2);
    push_string(end_shared_string(s));
    return;
  }
  default:
    error("Bitwise and on illegal type.\n");
  }
}

/* This function is used to speed up or/xor/and on
 * arrays multisets and mappings. This is done by
 * calling the operator for each pair of arguments
 * first, then recursively doing the same on the
 * results until only one value remains.
 */
static void speedup(INT32 args, void (*func)(void))
{
  switch(sp[-args].type)
  {
  case T_MAPPING:
  case T_ARRAY:
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

  default:
    while(--args > 0) func();
  }
}

void f_and(INT32 args)
{
  switch(args)
  {
  case 0: error("Too few arguments to `&\n");
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
    emit2(F_AND);
    return 1;

  default:
    return 0;
  }
}

void o_or(void)
{
  if(sp[-1].type != sp[-2].type)
  {
    if(call_lfun(LFUN_OR, LFUN_ROR))
      return;

    error("Bitwise or on different types.\n");
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
    m=merge_mappings(sp[-2].u.mapping, sp[-1].u.mapping, OP_OR);
    pop_n_elems(2);
    push_mapping(m);
    return;
  }

  case T_MULTISET:
  {
    struct multiset *l;
    l=merge_multisets(sp[-2].u.multiset, sp[-1].u.multiset, OP_OR);
    pop_n_elems(2);
    push_multiset(l);
    return;
  }
    
  case T_ARRAY:
  {
    struct array *a;
    a=merge_array_without_order(sp[-2].u.array, sp[-1].u.array, OP_OR);
    pop_n_elems(2);
    push_array(a);
    return;
  }

  case T_STRING:
  {
    struct pike_string *s;
    INT32 len, i;

    len = sp[-2].u.string->len;
    if (len != sp[-1].u.string->len)
      error("Bitwise OR on strings of different lengths.\n");
    s = begin_shared_string(len);
    for (i=0; i<len; i++)
      s->str[i] = sp[-2].u.string->str[i] | sp[-1].u.string->str[i];
    pop_n_elems(2);
    push_string(end_shared_string(s));
    return;
  }

  default:
    error("Bitwise or on illegal type.\n");
  }
}

void f_or(INT32 args)
{
  switch(args)
  {
  case 0: error("Too few arguments to `|\n");
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
    emit2(F_OR);
    return 1;

  default:
    return 0;
  }
}


void o_xor(void)
{
  if(sp[-1].type != sp[-2].type)
  {
    if(call_lfun(LFUN_XOR, LFUN_RXOR))
      return;
    error("Bitwise xor on different types.\n");
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
    m=merge_mappings(sp[-2].u.mapping, sp[-1].u.mapping, OP_XOR);
    pop_n_elems(2);
    push_mapping(m);
    return;
  }

  case T_MULTISET:
  {
    struct multiset *l;
    l=merge_multisets(sp[-2].u.multiset, sp[-1].u.multiset, OP_XOR);
    pop_n_elems(2);
    push_multiset(l);
    return;
  }
    
  case T_ARRAY:
  {
    struct array *a;
    a=merge_array_without_order(sp[-2].u.array, sp[-1].u.array, OP_XOR);
    pop_n_elems(2);
    push_array(a);
    return;
  }

  case T_STRING:
  {
    struct pike_string *s;
    INT32 len, i;

    len = sp[-2].u.string->len;
    if (len != sp[-1].u.string->len)
      error("Bitwise XOR on strings of different lengths.\n");
    s = begin_shared_string(len);
    for (i=0; i<len; i++)
      s->str[i] = sp[-2].u.string->str[i] ^ sp[-1].u.string->str[i];
    pop_n_elems(2);
    push_string(end_shared_string(s));
    return;
  }

  default:
    error("Bitwise xor on illegal type.\n");
  }
}

void f_xor(INT32 args)
{
  switch(args)
  {
  case 0: error("Too few arguments to `^\n");
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
    emit2(F_XOR);
    return 1;

  default:
    return 0;
  }
}

void o_lsh(void)
{
  if(sp[-1].type != T_INT || sp[-2].type != T_INT)
  {
    if(call_lfun(LFUN_LSH, LFUN_RLSH))
      return;

    if(sp[-2].type != T_INT)
      error("Bad argument 1 to <<\n");
    error("Bad argument 2 to <<\n");
  }
  sp--;
  sp[-1].u.integer = sp[-1].u.integer << sp->u.integer;
}

void f_lsh(INT32 args)
{
  if(args != 2)
    error("Bad number of args to `<<\n");
  o_lsh();
}

static int generate_lsh(node *n)
{
  if(count_args(CDR(n))==2)
  {
    do_docode(CDR(n),DO_NOT_COPY);
    emit2(F_LSH);
    return 1;
  }
  return 0;
}

void o_rsh(void)
{
  if(sp[-2].type != T_INT || sp[-1].type != T_INT)
  {
    if(call_lfun(LFUN_RSH, LFUN_RRSH))
      return;
    if(sp[-2].type != T_INT)
      error("Bad argument 1 to >>\n");
    error("Bad argument 2 to >>\n");
  }
  sp--;
  sp[-1].u.integer = sp[-1].u.integer >> sp->u.integer;
}

void f_rsh(INT32 args)
{
  if(args != 2)
    error("Bad number of args to `>>\n");
  o_rsh();
}

static int generate_rsh(node *n)
{
  if(count_args(CDR(n))==2)
  {
    do_docode(CDR(n),DO_NOT_COPY);
    emit2(F_RSH);
    return 1;
  }
  return 0;
}


#define TWO_TYPES(X,Y) (((X)<<8)|(Y))
void o_multiply(void)
{
  switch(TWO_TYPES(sp[-2].type,sp[-1].type))
  {
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
    break;
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
    sp--;
    sp[-1].u.integer *= sp[0].u.integer;
    return;

  default:
    if(call_lfun(LFUN_MULTIPLY, LFUN_RMULTIPLY))
      return;

    error("Bad arguments to multiply.\n");
  }
}

void f_multiply(INT32 args)
{
  switch(args)
  {
  case 0: error("Too few arguments to `*\n");
  case 1: return;
  case 2: o_multiply(); return;
  default:
    if(sp[-args].type==T_OBJECT)
    {
      CALL_OPERATOR(LFUN_MULTIPLY, args);
    } else {
      while(--args > 0) o_multiply(); 
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
    emit2(F_MULTIPLY);
    return 1;

  default:
    return 0;
  }
}

void o_divide(void)
{
  if(sp[-2].type!=sp[-1].type && !float_promote())
  {
    if(call_lfun(LFUN_DIVIDE, LFUN_RDIVIDE))
      return;
    error("Division on different types.\n");
  }

  switch(sp[-2].type)
  {
  case T_OBJECT:
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
      error("Division by zero.\n");
    sp--;
    sp[-1].u.float_number /= sp[0].u.float_number;
    return;

  case T_INT:
  {
    INT32 tmp;
    if (sp[-1].u.integer == 0)
      error("Division by zero\n");
    sp--;

    tmp=sp[-1].u.integer/sp[0].u.integer;

    if((sp[-1].u.integer<0) != (sp[0].u.integer<0))
      if(tmp*sp[0].u.integer!=sp[-1].u.integer)
	tmp--;

    sp[-1].u.integer=tmp;
    return;
  }
    
  default:
    error("Bad argument 1 to divide.\n");
  }
}

void f_divide(INT32 args)
{
  if(args != 2)
    error("Bad number of args to `/\n");
  o_divide();
}

static int generate_divide(node *n)
{
  if(count_args(CDR(n))==2)
  {
    do_docode(CDR(n),DO_NOT_COPY);
    emit2(F_DIVIDE);
    return 1;
  }
  return 0;
}

void o_mod(void)
{
  if(sp[-2].type != sp[-1].type && !float_promote())
  {
    if(call_lfun(LFUN_MOD, LFUN_RMOD))
      return;

    error("Modulo on different types.\n");
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
      error("Modulo by zero.\n");
    sp--;
    foo=sp[-1].u.float_number / sp[0].u.float_number;
    foo=sp[-1].u.float_number - sp[0].u.float_number * floor(foo);
    sp[-1].u.float_number=foo;
    return;
  }
  case T_INT:
    if (sp[-1].u.integer == 0) error("Modulo by zero.\n");
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
    error("Bad argument 1 to modulo.\n");
  }
}

void f_mod(INT32 args)
{
  if(args != 2)
    error("Bad number of args to `%%\n");
  o_mod();
}

static int generate_mod(node *n)
{
  if(count_args(CDR(n))==2)
  {
    do_docode(CDR(n),DO_NOT_COPY);
    emit2(F_MOD);
    return 1;
  }
  return 0;
}

void o_not(void)
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

void f_not(INT32 args)
{
  if(args != 1) error("Bad number of args to `!\n");
  o_not();
}

static int generate_not(node *n)
{
  if(count_args(CDR(n))==1)
  {
    do_docode(CDR(n),DO_NOT_COPY);
    emit2(F_NOT);
    return 1;
  }
  return 0;
}

void o_compl(void)
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

  case T_STRING:
  {
    struct pike_string *s;
    INT32 len, i;

    len = sp[-1].u.string->len;
    s = begin_shared_string(len);
    for (i=0; i<len; i++)
      s->str[i] = ~ sp[-1].u.string->str[i];
    pop_n_elems(1);
    push_string(end_shared_string(s));
    break;
  }

  default:
    error("Bad argument to ~\n");
  }
}

void f_compl(INT32 args)
{
  if(args != 1) error("Bad number of args to `~\n");
  o_compl();
}

static int generate_compl(node *n)
{
  if(count_args(CDR(n))==1)
  {
    do_docode(CDR(n),DO_NOT_COPY);
    emit2(F_COMPL);
    return 1;
  }
  return 0;
}

void o_negate(void)
{
  switch(sp[-1].type)
  {
  case T_OBJECT:
    CALL_OPERATOR(LFUN_SUBTRACT,1);
    break;

  case T_FLOAT:
    sp[-1].u.float_number=-sp[-1].u.float_number;
    return;
    
  case T_INT:
    sp[-1].u.integer = - sp[-1].u.integer;
    return;

  default: 
    error("Bad argument to unary minus\n");
  }
}

void o_range(void)
{
  INT32 from,to;

  if(sp[-3].type==T_OBJECT)
  {
    CALL_OPERATOR(LFUN_INDEX, 3);
    return;
  }

  if(sp[-2].type != T_INT)
    error("Bad argument 1 to [ .. ]\n");

  if(sp[-1].type != T_INT)
    error("Bad argument 2 to [ .. ]\n");

  from=sp[-2].u.integer;
  if(from<0) from=0;
  to=sp[-1].u.integer;
  if(to<from-1) to=from-1;
  sp-=2;

  switch(sp[-1].type)
  {
  case T_STRING:
  {
    struct pike_string *s;
    if(to>=sp[-1].u.string->len-1)
    {
      if(from==0) return;
      to=sp[-1].u.string->len-1;

      if(from>to+1) from=to+1;
    }
#ifdef DEBUG
    if(from < 0 || (to-from+1) < 0)
      fatal("Error in o_range.\n");
#endif

    s=make_shared_binary_string(sp[-1].u.string->str+from,to-from+1);
    free_string(sp[-1].u.string);
    sp[-1].u.string=s;
    break;
  }

  case T_ARRAY:
  {
    struct array *a;
    if(to>=sp[-1].u.array->size-1)
    {
      to=sp[-1].u.array->size-1;

      if(from>to+1) from=to+1;
    }

    a=slice_array(sp[-1].u.array,from,to+1);
    free_array(sp[-1].u.array);
    sp[-1].u.array=a;
    break;
  }
    
  default:
    error("[ .. ] on non-scalar type.\n");
  }
}

void f_index(INT32 args)
{
  switch(args)
  {
  case 0:
  case 1:
    error("Too few arguments to `[]\n");
    break;
  case 2:
    if(sp[-1].type==T_STRING) sp[-1].subtype=0;
    o_index();
    break;
  case 3:
    o_range();
    break;
  default:
    error("Too many arguments to `[]\n");
  }
}

void f_arrow(INT32 args)
{
  switch(args)
  {
  case 0:
  case 1:
    error("Too few arguments to `->\n");
    break;
  case 2:
    if(sp[-1].type==T_STRING)
      sp[-1].subtype=1;
    o_index();
    break;
  default:
    error("Too many arguments to `->\n");
  }
}

void f_sizeof(INT32 args)
{
  INT32 tmp;
  if(args<1)
    error("Too few arguments to sizeof()\n");

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
  emit2(F_SIZEOF);
  return 1;
}

void init_operators(void)
{
  add_efun2("`[]",f_index,
	    "function(string,int:int)|function(object,string:mixed)|function(array,int:mixed)|function(mapping,mixed:mixed)|function(multiset,mixed:int)|function(string,int,int:string)|function(array,int,int:array)",OPT_TRY_OPTIMIZE,0,0);

  add_efun2("`->",f_arrow,
	    "function(array(object|mapping|multiset|array)|object|mapping|multiset,string:mixed)",OPT_TRY_OPTIMIZE,0,0);

  add_efun2("`==",f_eq,"function(mixed,mixed:int)",OPT_TRY_OPTIMIZE,0,generate_comparison);
  add_efun2("`!=",f_ne,"function(mixed,mixed:int)",OPT_TRY_OPTIMIZE,0,generate_comparison);
  add_efun2("`!",f_not,"function(mixed:int)",OPT_TRY_OPTIMIZE,0,generate_not);

#define CMP_TYPE "function(object,mixed:int)|function(mixed,object:int)|function(int|float,int|float:int)|function(string,string:int)"
  add_efun2("`<", f_lt,CMP_TYPE,OPT_TRY_OPTIMIZE,0,generate_comparison);
  add_efun2("`<=",f_le,CMP_TYPE,OPT_TRY_OPTIMIZE,0,generate_comparison);
  add_efun2("`>", f_gt,CMP_TYPE,OPT_TRY_OPTIMIZE,0,generate_comparison);
  add_efun2("`>=",f_ge,CMP_TYPE,OPT_TRY_OPTIMIZE,0,generate_comparison);

  add_efun2("`+",f_add,"!function(!object...:mixed)&function(mixed...:mixed)|function(int...:int)|!function(int...:mixed)&function(int|float...:float)|!function(int|float...:mixed)&function(string|int|float...:string)|function(array...:array)|function(mapping...:mapping)|function(multiset...:multiset)",OPT_TRY_OPTIMIZE,optimize_binary,generate_sum);

  add_efun2("`-",f_minus,"!function(!object...:mixed)&function(mixed...:mixed)|function(int:int)|function(float:float)|function(array,array:array)|function(mapping,mapping:mapping)|function(multiset,multiset:multiset)|function(float|int,float:float)|function(float,int:float)|function(int,int:int)|function(string,string:string)",OPT_TRY_OPTIMIZE,0,generate_minus);

#define LOG_TYPE "function(mixed,object...:mixed)|function(object,mixed...:mixed)|function(int...:int)|function(mapping...:mapping)|function(multiset...:multiset)|function(array...:array)|function(string...:string)"

  add_efun2("`&",f_and,LOG_TYPE,OPT_TRY_OPTIMIZE,optimize_binary,generate_and);

  add_efun2("`|",f_or,LOG_TYPE,OPT_TRY_OPTIMIZE,optimize_binary,generate_or);

  add_efun2("`^",f_xor,LOG_TYPE,OPT_TRY_OPTIMIZE,optimize_binary,generate_xor);


#define SHIFT_TYPE "function(object,mixed:mixed)|function(mixed,object:mixed)|function(int,int:int)"

  add_efun2("`<<",f_lsh,SHIFT_TYPE,OPT_TRY_OPTIMIZE,0,generate_lsh);
  add_efun2("`>>",f_rsh,SHIFT_TYPE,OPT_TRY_OPTIMIZE,0,generate_rsh);

  add_efun2("`*",f_multiply,"!function(!object...:mixed)&function(mixed...:mixed)|function(array(array),array:array)|function(int...:int)|!function(int...:mixed)&function(float|int...:float)|function(string*,string:string)",OPT_TRY_OPTIMIZE,optimize_binary,generate_multiply);

  add_efun2("`/",f_divide,"function(mixed,object:mixed)|function(array,array:array(array))|function(object,mixed:mixed)|function(int,int:int)|function(float|int,float:float)|function(float,int:float)|function(string,string:string*)",OPT_TRY_OPTIMIZE,0,generate_divide);

  add_efun2("`%",f_mod,"function(mixed,object:mixed)|function(object,mixed:mixed)|function(int,int:int)|!function(int,int:mixed)&function(int|float,int|float:float)",OPT_TRY_OPTIMIZE,0,generate_mod);

  add_efun2("`~",f_compl,"function(object:mixed)|function(int:int)|function(float:float)|function(string:string)",OPT_TRY_OPTIMIZE,0,generate_compl);
  add_efun2("sizeof", f_sizeof, "function(string|multiset|array|mapping|object:int)",0,0,generate_sizeof);
}
