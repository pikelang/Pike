/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include <math.h>
#include "global.h"
RCSID("$Id: operators.c,v 1.33 1998/05/25 10:38:46 hubbe Exp $");
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
#include "module_support.h"

#define COMPARISON(ID,NAME,FUN)			\
void ID(INT32 args)				\
{						\
  int i;					\
  switch(args)					\
  {						\
    case 0: case 1:				\
      PIKE_ERROR(NAME, "Too few arguments\n", sp, args); \
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

void f_ne(INT32 args)
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
   PIKE_ERROR(lfun_names[OP], "Called in destructed object.\n", sp, args); \
 if(FIND_LFUN(sp[-args].u.object->prog,OP) == -1) \
   PIKE_ERROR(lfun_names[OP], "Operator not in object.\n", sp, args); \
 apply_lfun(sp[-args].u.object, OP, args-1); \
 free_svalue(sp-2); \
 sp[-2]=sp[-1]; \
 sp--;


void f_add(INT32 args)
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
      PIKE_ERROR("`+", "Too few arguments\n", sp, args);
    }else{
      if(types & BIT_OBJECT)
      {
	if(sp[-args].type == T_OBJECT &&
	  sp[-args].u.object->prog &&
	   FIND_LFUN(sp[-args].u.object->prog,LFUN_ADD) != -1)
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
	     FIND_LFUN(sp[e-args].u.object->prog,LFUN_RADD) != -1)
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
	PIKE_ERROR("`+", "Bad argument 1\n", sp, args);
    }
    PIKE_ERROR("`+", "Incompatible types\n", sp, args);
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
     FIND_LFUN(sp[-2].u.object->prog,left) != -1)
  {
    apply_lfun(sp[-2].u.object, left, 1);
    free_svalue(sp-2);
    sp[-2]=sp[-1];
    sp--;
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
    PIKE_ERROR("`-", "Subtract on different types.\n", sp, 2);
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
    PIKE_ERROR("`-", "Bad argument 1.\n", sp, 2);
  }
}

void f_minus(INT32 args)
{
  switch(args)
  {
    case 0: PIKE_ERROR("`-", "Too few arguments.\n", sp, 0);
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

    PIKE_ERROR("`&", "Bitwise and on different types.\n", sp, 2);
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
      PIKE_ERROR("`&", "Bitwise AND on strings of different lengths.\n", sp, 2);
    s = begin_shared_string(len);
    for (i=0; i<len; i++)
      s->str[i] = sp[-2].u.string->str[i] & sp[-1].u.string->str[i];
    pop_n_elems(2);
    push_string(end_shared_string(s));
    return;
  }
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
  case 0: PIKE_ERROR("`&", "Too few arguments.\n", sp, 0);
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

    PIKE_ERROR("`|", "Bitwise or on different types.\n", sp, 2);
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
      PIKE_ERROR("`|", "Bitwise OR on strings of different lengths.\n", sp, 2);
    s = begin_shared_string(len);
    for (i=0; i<len; i++)
      s->str[i] = sp[-2].u.string->str[i] | sp[-1].u.string->str[i];
    pop_n_elems(2);
    push_string(end_shared_string(s));
    return;
  }

  default:
    PIKE_ERROR("`|", "Bitwise or on illegal type.\n", sp, 2);
  }
}

void f_or(INT32 args)
{
  switch(args)
  {
  case 0: PIKE_ERROR("`|", "Too few arguments.\n", sp, 0);
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
    PIKE_ERROR("`^", "Bitwise XOR on different types.\n", sp, 2);
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
      PIKE_ERROR("`^", "Bitwise XOR on strings of different lengths.\n", sp, 2);
    s = begin_shared_string(len);
    for (i=0; i<len; i++)
      s->str[i] = sp[-2].u.string->str[i] ^ sp[-1].u.string->str[i];
    pop_n_elems(2);
    push_string(end_shared_string(s));
    return;
  }

  default:
    PIKE_ERROR("`^", "Bitwise XOR on illegal type.\n", sp, 2);
  }
}

void f_xor(INT32 args)
{
  switch(args)
  {
  case 0: PIKE_ERROR("`^", "Too few arguments.\n", sp, 0);
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
      PIKE_ERROR("`<<", "Bad argument 1.\n", sp, 2);
    PIKE_ERROR("`<<", "Bad argument 2.\n", sp, 2);
  }
  sp--;
  sp[-1].u.integer = sp[-1].u.integer << sp->u.integer;
}

void f_lsh(INT32 args)
{
  if(args != 2)
    PIKE_ERROR("`<<", "Bad number of args.\n", sp, args);
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
      PIKE_ERROR("`>>", "Bad argument 1.\n", sp, 2);
    PIKE_ERROR("`>>", "Bad argument 2.\n", sp, 2);
  }
  sp--;
  sp[-1].u.integer = sp[-1].u.integer >> sp->u.integer;
}

void f_rsh(INT32 args)
{
  if(args != 2)
    PIKE_ERROR("`>>", "Bad number of args.\n", sp, args);
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
    case TWO_TYPES(T_ARRAY, T_INT):
      {
	struct array *ret;
	struct svalue *pos;
	INT32 e;
	if(sp[-1].u.integer < 0)
	  PIKE_ERROR("`*", "Cannot multiply array by negative number.\n", sp, 2);
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
    case TWO_TYPES(T_STRING, T_INT):
      {
	struct pike_string *ret;
	char *pos;
	INT32 e;
	if(sp[-1].u.integer < 0)
	  PIKE_ERROR("`*", "Cannot multiply string by negative number.\n", sp, 2);
	ret=begin_shared_string(sp[-2].u.string->len * sp[-1].u.integer);
	pos=ret->str;
	for(e=0;e<sp[-1].u.integer;e++,pos+=sp[-2].u.string->len)
	  MEMCPY(pos,sp[-2].u.string->str,sp[-2].u.string->len);
	pop_n_elems(2);
	push_string(end_shared_string(ret));
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
    sp--;
    sp[-1].u.integer *= sp[0].u.integer;
    return;

  default:
    if(call_lfun(LFUN_MULTIPLY, LFUN_RMULTIPLY))
      return;

    PIKE_ERROR("`*", "Bad arguments.\n", sp, 2);
  }
}

void f_multiply(INT32 args)
{
  switch(args)
  {
  case 0: PIKE_ERROR("`*", "Too few arguments.\n", sp, 0);
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

    switch(TWO_TYPES(sp[-2].type,sp[-1].type))
    {
      case TWO_TYPES(T_STRING,T_INT):
      {
	struct array *a;
	char *pos=sp[-2].u.string->str;
	INT32 size,e,len;

	len=sp[-1].u.integer;
	if(!len)
	  PIKE_ERROR("`/", "Division by zero.\n", sp, 2);

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
	  a->item[e].u.string=make_shared_binary_string(pos,len);
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
	INT32 last,pos,e,size;
	double len;

	len=sp[-1].u.float_number;
	if(len==0.0)
	  PIKE_ERROR("`/", "Division by zero.\n", sp, 2);

	if(len<0)
	{
	  len=-len;
	  size=(INT32)ceil( ((double)sp[-2].u.string->len) / len);
	  a=allocate_array(size);
	  
	  for(last=sp[-2].u.string->len,e=0;e<size-1;e++)
	  {
	    pos=sp[-2].u.string->len - (INT32)((e+1)*len);
	    a->item[size-1-e].u.string=make_shared_binary_string(
	      sp[-2].u.string->str + pos,
	      last-pos);
	    a->item[size-1-e].type=T_STRING;
	    last=pos;
	  }
	  pos=0;
	  a->item[0].u.string=make_shared_binary_string(
	    sp[-2].u.string->str + pos,
	    last-pos);
	  a->item[0].type=T_STRING;
	}else{
	  size=(INT32)ceil( ((double)sp[-2].u.string->len) / len);
	  a=allocate_array(size);
	  
	  for(last=0,e=0;e<size-1;e++)
	  {
	    pos=(INT32)((e+1)*len);
	    a->item[e].u.string=make_shared_binary_string(
	      sp[-2].u.string->str + last,
	      pos-last);
	    a->item[e].type=T_STRING;
	    last=pos;
	  }
	  pos=sp[-2].u.string->len;
	  a->item[e].u.string=make_shared_binary_string(
	    sp[-2].u.string->str + last,
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
	INT32 size,e,len,pos=0;

	len=sp[-1].u.integer;
	if(!len)
	  PIKE_ERROR("`/", "Division by zero.\n", sp, 2);

	if(len<0)
	{
	  len=-len;
	  size=sp[-2].u.array->size / len;
	  pos+=sp[-2].u.array->size % len;
	}else{
	  size=sp[-2].u.array->size / len;
	}
	a=allocate_array(size);
	for(e=0;e<size;e++)
	{
	  a->item[e].u.array=friendly_slice_array(sp[-2].u.array,
						  pos,
						  pos+len);
	  a->item[e].type=T_ARRAY;
	  pos+=len;
	}
	a->type_field=BIT_ARRAY;
	pop_n_elems(2);
	push_array(a);
	return;
      }

      case TWO_TYPES(T_ARRAY,T_FLOAT):
      {
	struct array *a;
	INT32 last,pos,e,size;
	double len;

	len=sp[-1].u.float_number;
	if(len==0.0)
	  PIKE_ERROR("`/", "Division by zero.\n", sp, 2);

	if(len<0)
	{
	  len=-len;
	  size=(INT32)ceil( ((double)sp[-2].u.array->size) / len);
	  a=allocate_array(size);
	  
	  for(last=sp[-2].u.array->size,e=0;e<size-1;e++)
	  {
	    pos=sp[-2].u.array->size - (INT32)((e+1)*len);
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
	  size=(INT32)ceil( ((double)sp[-2].u.array->size) / len);
	  a=allocate_array(size);
	  
	  for(last=0,e=0;e<size-1;e++)
	  {
	    pos=(INT32)((e+1)*len);
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
      PIKE_ERROR("`/", "Division by zero.\n", sp, 2);
    sp--;
    sp[-1].u.float_number /= sp[0].u.float_number;
    return;

  case T_INT:
  {
    INT32 tmp;
    if (sp[-1].u.integer == 0)
      PIKE_ERROR("`/", "Division by zero\n", sp, 2);
    sp--;

    tmp=sp[-1].u.integer/sp[0].u.integer;

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

void f_divide(INT32 args)
{
  switch(args)
  {
    case 0: 
    case 1: PIKE_ERROR("`/", "Too few arguments to `/\n", sp, args);
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

    switch(TWO_TYPES(sp[-2].type,sp[-1].type))
    {
      case TWO_TYPES(T_STRING,T_INT):
      {
	struct pike_string *s=sp[-2].u.string;
	INT32 tmp,base;
	if(!sp[-1].u.integer)
	  PIKE_ERROR("`%", "Modulo by zero.\n", sp, 2);

	tmp=sp[-1].u.integer;
	if(tmp<0)
	{
	  tmp=s->len % -tmp;
	  base=0;
	}else{
	  tmp=s->len % tmp;
	  base=s->len - tmp;
	}
	s=make_shared_binary_string(s->str + base, tmp);
	pop_n_elems(2);
	push_string(s);
	return;
      }


      case TWO_TYPES(T_ARRAY,T_INT):
      {
	struct array *a=sp[-2].u.array;
	INT32 tmp,base;
	if(!sp[-1].u.integer)
	  PIKE_ERROR("`%", "Modulo by zero.\n", sp, 2);

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
      PIKE_ERROR("`%", "Modulo by zero.\n", sp, 2);
    sp--;
    foo=sp[-1].u.float_number / sp[0].u.float_number;
    foo=sp[-1].u.float_number - sp[0].u.float_number * floor(foo);
    sp[-1].u.float_number=foo;
    return;
  }
  case T_INT:
    if (sp[-1].u.integer == 0) PIKE_ERROR("`%", "Modulo by zero.\n", sp, 2);
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

void f_mod(INT32 args)
{
  if(args != 2)
    PIKE_ERROR("`%", "Bad number of args\n", sp, args);
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
  if(args != 1) PIKE_ERROR("`!", "Bad number of args.\n", sp, args);
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
    PIKE_ERROR("`~", "Bad argument.\n", sp, 1);
  }
}

void f_compl(INT32 args)
{
  if(args != 1) PIKE_ERROR("`~", "Bad number of args.\n", sp, args);
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
    PIKE_ERROR("`-", "Bad argument to unary minus.\n", sp, 1);
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
    PIKE_ERROR("`[]", "Bad argument 2 to [ .. ]\n", sp, 3);

  if(sp[-1].type != T_INT)
    PIKE_ERROR("`[]", "Bad argument 3 to [ .. ]\n", sp, 3);

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
    PIKE_ERROR("`[]", "[ .. ] on non-scalar type.\n", sp, 3);
  }
}

void f_index(INT32 args)
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

void f_arrow(INT32 args)
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

void f_sizeof(INT32 args)
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
  emit2(F_SIZEOF);
  return 1;
}

static int generate_call_function(node *n)
{
  node **arg;
  emit2(F_MARK);
  do_docode(CDR(n),DO_NOT_COPY);
  emit2(F_CALL_FUNCTION);
  return 1;
}

struct program *string_assignment_program;

#undef THIS
#define THIS ((struct string_assignment_storage *)(fp->current_storage))
static void f_string_assignment_index(INT32 args)
{
  INT32 i;
  get_all_args("string[]",args,"%i",&i);
  if(i<0) i+=THIS->s->len;
  if(i<0)
    i+=THIS->s->len;
  if(i<0 || i>=THIS->s->len)
    error("Index %d is out of range 0 - %d.\n", i, THIS->s->len-1);
  else
    i=EXTRACT_UCHAR(THIS->s->str + i);
  pop_n_elems(args);
  push_int(i);
}

static void f_string_assignment_assign_index(INT32 args)
{
  INT32 i,j;
  union anything *u;
  get_all_args("string[]=",args,"%i%i",&i,&j);
  if((u=get_pointer_if_this_type(THIS->lval, T_STRING)))
  {
    free_string(THIS->s);
    if(i<0) i+=u->string->len;
    if(i<0 || i>=u->string->len)
      error("String index out of range %ld\n",(long)i);
    u->string=modify_shared_string(u->string,i,j);
    copy_shared_string(THIS->s, u->string);
  }else{
    lvalue_to_svalue_no_free(sp,THIS->lval);
    sp++;
    if(sp[-1].type != T_STRING) error("string[]= failed.\n");
    if(i<0) i+=sp[-1].u.string->len;
    if(i<0 || i>=sp[-1].u.string->len)
      error("String index out of range %ld\n",(long)i);
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
  add_efun2("`[]",f_index,
	    "function(string,int:int)|function(object,string:mixed)|function(array(0=mixed),int:0)|function(mapping(mixed:1=mixed),mixed:1)|function(multiset,mixed:int)|function(string,int,int:string)|function(array(2=mixed),int,int:array(2))",OPT_TRY_OPTIMIZE,0,0);

  add_efun2("`->",f_arrow,
	    "function(array(object|mapping|multiset|array),string:array(mixed))|function(object|mapping|multiset,string:mixed)",OPT_TRY_OPTIMIZE,0,0);

  add_efun2("`==",f_eq,"function(mixed...:int)",OPT_TRY_OPTIMIZE,0,generate_comparison);
  add_efun2("`!=",f_ne,"function(mixed...:int)",OPT_TRY_OPTIMIZE,0,generate_comparison);
  add_efun2("`!",f_not,"function(mixed:int)",OPT_TRY_OPTIMIZE,0,generate_not);

#define CMP_TYPE "!function(!object...:mixed)&function(mixed...:int)|function(int|float...:int)|function(string...:int)"
  add_efun2("`<", f_lt,CMP_TYPE,OPT_TRY_OPTIMIZE,0,generate_comparison);
  add_efun2("`<=",f_le,CMP_TYPE,OPT_TRY_OPTIMIZE,0,generate_comparison);
  add_efun2("`>", f_gt,CMP_TYPE,OPT_TRY_OPTIMIZE,0,generate_comparison);
  add_efun2("`>=",f_ge,CMP_TYPE,OPT_TRY_OPTIMIZE,0,generate_comparison);

  add_efun2("`+",f_add,
	    "!function(!object...:mixed)&function(mixed...:mixed)|"
	    "function(int...:int)|"
	    "!function(int...:mixed)&function(int|float...:float)|"
	    "!function(int|float...:mixed)&function(string|int|float...:string)|"
	    "function(0=array...:0)|"
	    "function(mapping(1=mixed:2=mixed)...:mapping(1:2))|"
	    "function(3=multiset...:3)",
	    OPT_TRY_OPTIMIZE,optimize_binary,generate_sum);

  add_efun2("`-",f_minus,
	    "!function(!object...:mixed)&function(mixed...:mixed)|"
	    "function(int:int)|"
	    "function(float:float)|"
	    "function(array(0=mixed),array:array(0))|"
	    "function(mapping(1=mixed:2=mixed),mapping:mapping(1:2))|"
	    "function(multiset(3=mixed),multiset:multiset(3))|"
	    "function(float|int,float:float)|"
	    "function(float,int:float)|"
	    "function(int,int:int)|"
	    "function(string,string:string)",
	    OPT_TRY_OPTIMIZE,0,generate_minus);

#define LOG_TYPE "function(mixed,object...:mixed)|function(object,mixed...:mixed)|function(int...:int)|function(mapping(0=mixed:1=mixed)...:mapping(0:1))|function(multiset(2=mixed)...:multiset(2))|function(array(3=mixed)...:array(3))|function(string...:string)"

  add_efun2("`&",f_and,LOG_TYPE,OPT_TRY_OPTIMIZE,optimize_binary,generate_and);

  add_efun2("`|",f_or,LOG_TYPE,OPT_TRY_OPTIMIZE,optimize_binary,generate_or);

  add_efun2("`^",f_xor,LOG_TYPE,OPT_TRY_OPTIMIZE,optimize_binary,generate_xor);


#define SHIFT_TYPE "function(object,mixed:mixed)|function(mixed,object:mixed)|function(int,int:int)"

  add_efun2("`<<",f_lsh,SHIFT_TYPE,OPT_TRY_OPTIMIZE,0,generate_lsh);
  add_efun2("`>>",f_rsh,SHIFT_TYPE,OPT_TRY_OPTIMIZE,0,generate_rsh);

  add_efun2("`*",f_multiply,
	    "!function(!object...:mixed)&function(mixed...:mixed)|"
	    "function(array(array(1=mixed)),array(1=mixed):array(1))|"
	    "function(int...:int)|"
	    "!function(int...:mixed)&function(float|int...:float)|"
	    "function(string*,string:string)|"
	    "function(array(0=mixed),int:array(0))|"
	    "function(string,int:string)",
	    OPT_TRY_OPTIMIZE,optimize_binary,generate_multiply);

  add_efun2("`/",f_divide,
	    "!function(!object...:mixed)&function(mixed...:mixed)|"
	    "function(int,int...:int)|"
	    "!function(int...:mixed)&function(float|int...:float)|"
	    "function(array(0=mixed),array|int|float...:array(array(0)))|"
	    "function(string,string|int|float...:array(string))",
	    OPT_TRY_OPTIMIZE,0,generate_divide);

  add_efun2("`%",f_mod,
	    "function(mixed,object:mixed)|"
	    "function(object,mixed:mixed)|"
	    "function(int,int:int)|"
	    "function(string,int:string)|"
	    "function(array(0=mixed),int:array(0))|"
	    "!function(int,int:mixed)&function(int|float,int|float:float)"
	    ,OPT_TRY_OPTIMIZE,0,generate_mod);

  add_efun2("`~",f_compl,"function(object:mixed)|function(int:int)|function(float:float)|function(string:string)",OPT_TRY_OPTIMIZE,0,generate_compl);
  add_efun2("sizeof", f_sizeof, "function(string|multiset|array|mapping|object:int)",0,0,generate_sizeof);

  add_efun2("`()",f_call_function,"function(mixed,mixed ...:mixed)",OPT_SIDE_EFFECT | OPT_EXTERNAL_DEPEND,0,generate_call_function);

  /* This one should be removed */
  add_efun2("call_function",f_call_function,"function(mixed,mixed ...:mixed)",OPT_SIDE_EFFECT | OPT_EXTERNAL_DEPEND,0,generate_call_function);


  start_new_program();
  add_storage(sizeof(struct string_assignment_storage));
  add_function("`[]",f_string_assignment_index,"function(int:int)",0);
  add_function("`[]=",f_string_assignment_assign_index,"function(int,int:int)",0);
  set_init_callback(init_string_assignment_storage);
  set_exit_callback(exit_string_assignment_storage);
  string_assignment_program=end_program();
}


void exit_operators()
{
  if(string_assignment_program)
  {
    free_program(string_assignment_program);
    string_assignment_program=0;
  }
}
