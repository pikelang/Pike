/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include <math.h>
#include "global.h"
#include "interpret.h"
#include "svalue.h"
#include "list.h"
#include "mapping.h"
#include "array.h"
#include "stralloc.h"
#include "opcodes.h"
#include "operators.h"
#include "language.h"
#include "memory.h"
#include "error.h"
#include "docode.h"
#include "add_efun.h"

#define COMPARISON(ID,NAME,EXPR) \
void ID(INT32 args) \
{\
  int i; \
  if(args > 2) \
    pop_n_elems(args-2); \
  else if(args < 2) \
    error("Too few arguments to %s\n",NAME); \
  i=EXPR; \
  pop_n_elems(2); \
  sp->type=T_INT; \
  sp->u.integer=i; \
  sp++; \
}

COMPARISON(f_eq,"`==", is_eq(sp-2,sp-1))
COMPARISON(f_ne,"`!=",!is_eq(sp-2,sp-1))
COMPARISON(f_lt,"`<" , is_lt(sp-2,sp-1))
COMPARISON(f_le,"`<=",!is_gt(sp-2,sp-1))
COMPARISON(f_gt,"`>" , is_gt(sp-2,sp-1))
COMPARISON(f_ge,"`>=",!is_lt(sp-2,sp-1))


void f_add(INT32 args)
{
  INT32 e,size;
  TYPE_FIELD types;

  types=0;
  for(e=-args;e<0;e++) types|=1<<sp[e].type;
    
  switch(types)
  {
  default:
    if(args)
    {
      switch(sp[-args].type)
      {
      case T_OBJECT:
      case T_PROGRAM:
      case T_FUNCTION:
	error("Bad argument 1 to summation\n");
      }
    }
    error("Incompatible types to sum() or +\n");
    return; /* compiler hint */

  case BIT_STRING:
  {
    struct lpc_string *r;
    char *buf;

    if(args==1) return;
    size=0;

    for(e=-args;e<0;e++) size+=sp[e].u.string->len;

    if(args==2)
    {
      r=add_shared_strings(sp[-2].u.string,sp[-1].u.string);
    }else{
      r=begin_shared_string(size);
      buf=r->str;
      for(e=-args;e<0;e++)
      {
	MEMCPY(buf,sp[e].u.string->str,sp[e].u.string->len);
	buf+=sp[e].u.string->len;
      }
      r=end_shared_string(r);
    }
    for(e=-args;e<0;e++)
    {
      free_string(sp[e].u.string);
    }
    sp-=args;
    push_string(r);
    break;
  }

  case BIT_STRING | BIT_INT:
  case BIT_STRING | BIT_FLOAT:
  case BIT_STRING | BIT_FLOAT | BIT_INT:
  {
    struct lpc_string *r;
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

  case BIT_LIST:
  {
    struct list *l;

    l = add_lists(sp - args, args);
    pop_n_elems(args);
    push_list(l);
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
    ins_f_byte(F_ADD);
    return 1;

  default:
    return 0;
  }
}

static int generate_comparison(node *n)
{
  if(count_args(CDR(n))==2)
  {
    do_docode(CDR(n),DO_NOT_COPY);

    if(CAR(n)->u.sval.u.efun->function == f_eq)
      ins_f_byte(F_EQ);
    else if(CAR(n)->u.sval.u.efun->function == f_ne)
      ins_f_byte(F_NE);
    else if(CAR(n)->u.sval.u.efun->function == f_lt)
      ins_f_byte(F_LT);
    else if(CAR(n)->u.sval.u.efun->function == f_le)
      ins_f_byte(F_LE);
    else if(CAR(n)->u.sval.u.efun->function == f_gt)
      ins_f_byte(F_GT);
    else if(CAR(n)->u.sval.u.efun->function == f_ge)
      ins_f_byte(F_GE);
    else
      fatal("Couldn't generate comparison!\n");
    return 1;
  }
  return 0;
}


void o_subtract()
{
  if (sp[-2].type != sp[-1].type )
    error("Subtract on different types.\n");

  switch(sp[-1].type)
  {
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

  case T_LIST:
  {
    struct list *l;
    l=merge_lists(sp[-2].u.list, sp[-1].u.list, OP_SUB);
    pop_n_elems(2);
    push_list(l);
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
    struct lpc_string *s,*ret;
    sp--;
    s=make_shared_string("");
    ret=string_replace(sp[-1].u.string,sp[0].u.string,s);
    free_string(sp[-1].u.string);
    free_string(sp[0].u.string);
    free_string(s);
    sp[-1].u.string=ret;
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
    ins_f_byte(F_NEGATE);
    return 1;

  case 2:
    do_docode(CDR(n),DO_NOT_COPY);
    ins_f_byte(F_SUBTRACT);
    return 1;
  }
  return 0;
}

void o_and()
{
  if(sp[-1].type != sp[-2].type)
    error("Bitwise and on different types.\n");

  switch(sp[-2].type)
  {
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

  case T_LIST:
  {
    struct list *l;
    l=merge_lists(sp[-2].u.list, sp[-1].u.list, OP_AND);
    pop_n_elems(2);
    push_list(l);
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
  default:
    error("Bitwise and on illegal type.\n");
  }
}

void f_and(INT32 args)
{
  switch(args)
  {
  case 0: error("Too few arguments to `&\n");
  case 1: return;
  case 2: o_and(); return;
  default: while(--args > 0) o_and();
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
    ins_f_byte(F_AND);
    return 1;

  default:
    return 0;
  }
}

void o_or()
{
  if(sp[-1].type != sp[-2].type)
    error("Bitwise or on different types.\n");

  switch(sp[-2].type)
  {
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

  case T_LIST:
  {
    struct list *l;
    l=merge_lists(sp[-2].u.list, sp[-1].u.list, OP_OR);
    pop_n_elems(2);
    push_list(l);
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
  default: while(--args > 0) o_or();
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
    ins_f_byte(F_OR);
    return 1;

  default:
    return 0;
  }
}


void o_xor()
{
  if(sp[-1].type != sp[-2].type)
    error("Bitwise xor on different types.\n");

  switch(sp[-2].type)
  {
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

  case T_LIST:
  {
    struct list *l;
    l=merge_lists(sp[-2].u.list, sp[-1].u.list, OP_XOR);
    pop_n_elems(2);
    push_list(l);
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
  default: while(--args > 0) o_xor();
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
    ins_f_byte(F_XOR);
    return 1;

  default:
    return 0;
  }
}

void o_lsh()
{
  if(sp[-2].type != T_INT) error("Bad argument 1 to <<\n");
  if(sp[-1].type != T_INT) error("Bad argument 2 to <<\n");
  sp--;
  sp[-1].u.integer <<= sp[0].u.integer;
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
    ins_f_byte(F_LSH);
    return 1;
  }
  return 0;
}

void o_rsh()
{
  if(sp[-2].type != T_INT) error("Bad argument 1 to >>\n"); 
  if(sp[-1].type != T_INT) error("Bad argument 2 to >>\n");
  sp--;
  sp[-1].u.integer >>= sp[0].u.integer;
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
    ins_f_byte(F_RSH);
    return 1;
  }
  return 0;
}

void o_multiply()
{
  switch(sp[-2].type)
  {
  case T_ARRAY:
    if(sp[-1].type!=T_STRING)
    {
      error("Bad argument 2 to multiply.\n");
    }else{
      struct lpc_string *ret;
      sp--;
      ret=implode(sp[-1].u.array,sp[0].u.string);
      free_string(sp[0].u.string);
      free_array(sp[-1].u.array);
      sp[-1].type=T_STRING;
      sp[-1].u.string=ret;
      return;
    }

  case T_FLOAT:
    if(sp[-1].type!=T_FLOAT) error("Bad argument 2 to multiply.\n");
    sp--;
    sp[-1].u.float_number *= sp[0].u.float_number;
    return;

  case T_INT:
    if(sp[-1].type!=T_INT) error("Bad argument 2 to multiply.\n");
    sp--;
    sp[-1].u.integer *= sp[0].u.integer;
    return;

  default:
    error("Bad argument 1 to multiply.\n");
  }
}

void f_multiply(INT32 args)
{
  switch(args)
  {
  case 0: error("Too few arguments to `*\n");
  case 1: return;
  case 2: o_multiply(); return;
  case 3: while(--args > 0) o_multiply(); 
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
    ins_f_byte(F_MULTIPLY);
    return 1;

  default:
    return 0;
  }
}

void o_divide()
{
  if(sp[-2].type!=sp[-1].type)
    error("Division on different types.\n");

  switch(sp[-2].type)
  {
  case T_STRING:
  {
    struct array *ret;
    sp--;
    ret=explode(sp[-1].u.string,sp[0].u.string);
    free_string(sp[-1].u.string);
    free_string(sp[0].u.string);
    sp[-1].type=T_ARRAY;
    sp[-1].u.array=ret;
    return;
  }

  case T_FLOAT:
    if(sp[-1].u.float_number == 0.0)
      error("Division by zero.\n");
    sp--;
    sp[-1].u.float_number /= sp[0].u.float_number;
    return;

  case T_INT:
    if (sp[-1].u.integer == 0)
      error("Division by zero\n");
    sp--;
    sp[-1].u.integer /= sp[0].u.integer;
    return;
    
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
    ins_f_byte(F_DIVIDE);
    return 1;
  }
  return 0;
}

void o_mod()
{
  if(sp[-2].type != sp[-1].type)
    error("Modulo on different types.\n");

  switch(sp[-1].type)
  {
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
    sp[-1].u.integer %= sp[0].u.integer;
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
    ins_f_byte(F_MOD);
    return 1;
  }
  return 0;
}

void o_not()
{
  if(sp[-1].type==T_INT)
  {
    sp[-1].u.integer = !sp[-1].u.integer;
  }else{
    pop_stack();
    sp->type=T_INT;
    sp->u.integer=0;
    sp++;
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
    ins_f_byte(F_NOT);
    return 1;
  }
  return 0;
}

void o_compl()
{
  if (sp[-1].type != T_INT) error("Bad argument to ~\n");
  sp[-1].u.integer = ~ sp[-1].u.integer;
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
    ins_f_byte(F_COMPL);
    return 1;
  }
  return 0;
}

void o_negate()
{
  switch(sp[-1].type)
  {
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

void o_range()
{
  INT32 from,to;
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
    struct lpc_string *s;
    if(to>=sp[-1].u.string->len-1)
    {
      if(from==0) return;
      to=sp[-1].u.string->len-1;

      if(from>to+1) from=to+1;
    }

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
    error("[ .. ] can only be done on strings and arrays.\n");
  }
}

void init_operators()
{
  

  add_efun2("`==",f_eq,"function(mixed,mixed:int)",0,0,generate_comparison);
  add_efun2("`!=",f_ne,"function(mixed,mixed:int)",0,0,generate_comparison);
  add_efun2("`<", f_lt,"function(int,int:int)|function(float,float:int)|function(string,string:int)",0,0,generate_comparison);
  add_efun2("`<=",f_le,"function(int,int:int)|function(float,float:int)|function(string,string:int)",0,0,generate_comparison);
  add_efun2("`>", f_gt,"function(int,int:int)|function(float,float:int)|function(string,string:int)",0,0,generate_comparison);
  add_efun2("`>=",f_ge,"function(int,int:int)|function(float,float:int)|function(string,string:int)",0,0,generate_comparison);

  add_efun2("`+",f_add,"function(int ...:int)|function(float ...:float)|function(string,string|int|float ...:string)|function(string,string|int|float ...:string)|function(int|float,string,string|int|float:string)|function(array ...:array)|function(mapping ...:mapping)|function(list...:list)",0,0,generate_sum);

  add_efun2("`-",f_minus,"function(int:int)|function(float:float)|function(array,array:array)|function(mapping,mapping:mapping)|function(list,list:list)|function(float,float:float)|function(int,int:int)|function(string,string:string)",0,0,generate_minus);

  add_efun2("`&",f_and,"function(int...:int)|function(mapping...:mapping)|function(list...:list)|function(array...:array)",0,0,generate_and);

  add_efun2("`|",f_and,"function(int...:int)|function(mapping...:mapping)|function(list...:list)|function(array...:array)",0,0,generate_or);

  add_efun2("`^",f_and,"function(int...:int)|function(mapping...:mapping)|function(list...:list)|function(array...:array)",0,0,generate_xor);

  add_efun2("`<<",f_lsh,"function(int,int:int)",0,0,generate_lsh);
  add_efun2("`>>",f_rsh,"function(int,int:int)",0,0,generate_rsh);

  add_efun2("`*",f_multiply,"function(int...:int)|function(float...:float)|function(string*,string:string)",0,0,generate_multiply);

  add_efun2("`/",f_divide,"function(int,int:int)|function(float,float:float)|function(string,string:string*)",0,0,generate_divide);

  add_efun2("`%",f_mod,"function(int,int:int)|function(float,float:float)",0,0,generate_mod);

  add_efun2("`!",f_not,"function(mixed:int)",0,0,generate_not);
  add_efun2("`~",f_compl,"function(int:int)",0,0,generate_compl);
  
}
