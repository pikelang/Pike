/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
RCSID("$Id: docode.c,v 1.24 2003/09/19 13:57:49 grubba Exp $");
#include "las.h"
#include "program.h"
#include "language.h"
#include "pike_types.h"
#include "stralloc.h"
#include "interpret.h"
#include "constants.h"
#include "array.h"
#include "pike_macros.h"
#include "error.h"
#include "pike_memory.h"
#include "svalue.h"
#include "main.h"
#include "lex.h"
#include "builtin_functions.h"
#include "peep.h"
#include "docode.h"
#include "operators.h"

INT32 current_break=-1;
INT32 current_continue=-1;

static INT32 current_switch_case;
static INT32 current_switch_default;
static INT32 current_switch_values_on_stack;
static INT32 *current_switch_jumptable =0;

void ins_byte(unsigned char b,int area)
{
  add_to_mem_block(area, (char *)&b, 1);
}

void ins_signed_byte(char b,int area)
{
  add_to_mem_block(area, (char *)&b, 1);
}

void ins_short(INT16 l,int area)
{
  add_to_mem_block(area, (char *)&l, sizeof(INT16));
}

/*
 * Store an INT32.
 */
void ins_int(INT32 l,int area)
{
  add_to_mem_block(area, (char *)&l+0, sizeof(INT32));
}

void upd_int(int offset, INT32 tmp)
{
#ifdef HANDLES_UNALIGNED_MEMORY_ACCESS
  *((int *)(areas[A_PROGRAM].s.str+offset))=tmp;
#else
  MEMCPY(areas[A_PROGRAM].s.str+offset, (char *)&tmp,sizeof(tmp));
#endif
}

INT32 read_int(int offset)
{
  INT32 tmp;
#ifdef HANDLES_UNALIGNED_MEMORY_ACCESS
  tmp=*((int *)(areas[A_PROGRAM].s.str+offset));
#else
  MEMCPY((char *)&tmp, areas[A_PROGRAM].s.str+offset,sizeof(tmp));
#endif
  return tmp;
}

int store_linenumbers=1;

/*
 * A mechanism to remember addresses on a stack.
 */
int comp_stackp;
INT32 comp_stack[COMPILER_STACK_SIZE];

void push_address(void)
{
 if (comp_stackp >= COMPILER_STACK_SIZE)
 {
   yyerror("Compiler stack overflow");
   comp_stackp++;
   return;
 }
 comp_stack[comp_stackp++] = PC;
}

void push_explicit(INT32 address)
{
  if (comp_stackp >= COMPILER_STACK_SIZE)
  {
    yyerror("Compiler stack overflow");
    comp_stackp++;
    return;
  }
  comp_stack[comp_stackp++] = address;
}

INT32 pop_address(void)
{
  if (comp_stackp == 0)
     fatal("Compiler stack underflow.\n");
  if (comp_stackp > COMPILER_STACK_SIZE)
  {
    --comp_stackp;
    return 0;
  }
  return comp_stack[--comp_stackp];
}


static int label_no=0;

int alloc_label(void) { return ++label_no; }

int do_jump(int token,INT32 lbl)
{
  if(lbl==-1) lbl=alloc_label();
  emit(token, lbl);
  return lbl;
}

static int do_docode2(node *n,int flags);

#define ins_label(L) do_jump(F_LABEL, L)

void do_pop(int x)
{
  switch(x)
  {
  case 0: return;
  case 1: emit2(F_POP_VALUE); break;
  default: emit(F_POP_N_ELEMS,x); break;
  }
}

#define DO_CODE_BLOCK(X) do_pop(do_docode((X),DO_NOT_COPY | DO_POP));

int do_docode(node *n,INT16 flags)
{
  int i;
  int save_current_line=current_line;
  if(!n) return 0;
  current_line=n->line_number;
  i=do_docode2(n, flags);

  current_line=save_current_line;
  return i;
}


static int is_efun(node *n, c_fun fun)
{
  return n && n->token == F_CONSTANT &&
     n->u.sval.subtype == FUNCTION_BUILTIN &&
    n->u.sval.u.efun->function == fun;
}

static void code_expression(node *n, int flags, char *err)
{
  switch(do_docode(n, flags & ~ DO_POP))
  {
  case 0: my_yyerror("Void expression for %s",err);
  case 1: return;
  case 2:
    fatal("Internal compiler error (%s), line %ld, file %s\n",
	  err,
	  (long)current_line,
	  current_file?current_file->str:"Unknown");
  }
}

void do_cond_jump(node *n, int label, int iftrue, int flags)
{
  iftrue=!!iftrue;
  if((flags & DO_POP) && node_is_tossable(n))
  {
    int t,f;
    t=!!node_is_true(n);
    f=!!node_is_false(n);
    if(t || f)
    {
      if(t == iftrue) do_jump(F_BRANCH, label);
      return;
    }
  }

  switch(n->token)
  {
  case F_LAND:
  case F_LOR:
    if(iftrue == (n->token==F_LAND))
    {
      int tmp=alloc_label();
      do_cond_jump(CAR(n), tmp, !iftrue, flags | DO_POP);
      do_cond_jump(CDR(n), label, iftrue, flags);
      emit(F_LABEL,tmp);
    }else{
      do_cond_jump(CAR(n), label, iftrue, flags);
      do_cond_jump(CDR(n), label, iftrue, flags);
    }
    return;
    
  case F_APPLY:
    if(!is_efun(CAR(n), f_not)) break;

  case F_NOT:
    if(!(flags & DO_POP)) break;
    do_cond_jump(CDR(n), label , !iftrue, flags | DO_NOT_COPY);
    return;
  }

  code_expression(n, flags | DO_NOT_COPY, "condition");
  
  if(flags & DO_POP)
  {
    if(iftrue)
      do_jump(F_BRANCH_WHEN_NON_ZERO, label);
    else
      do_jump(F_BRANCH_WHEN_ZERO, label);
  }else{
    if(iftrue)
      do_jump(F_LOR, label);
    else
      do_jump(F_LAND, label);
  }
}

#define do_jump_when_zero(N,L) do_cond_jump(N,L,0,DO_POP|DO_NOT_COPY)
#define do_jump_when_non_zero(N,L) do_cond_jump(N,L,1,DO_POP|DO_NOT_COPY)

static INT32 count_cases(node *n)
{
  INT32 ret;
  if(!n) return 0;
  switch(n->token)
  {
  case F_DO:
  case F_FOR:
  case F_FOREACH:
  case F_INC_LOOP:
  case F_DEC_LOOP:
  case F_INC_NEQ_LOOP:
  case F_DEC_NEQ_LOOP:
  case F_SWITCH:
  case '?':
    return 0;

  case F_CASE:
    return !!CAR(n)+!!CDR(n);

  default:
    ret=0;
    if(car_is_node(n)) ret += count_cases(CAR(n));
    if(cdr_is_node(n)) ret += count_cases(CDR(n));
    return ret;
  }
}

static int do_docode2(node *n,int flags)
{
  INT32 tmp1,tmp2,tmp3;
  int ret;

  if(!n) return 0;

  if(flags & DO_LVALUE)
  {
    switch(n->token)
    {
    default:
      yyerror("Illegal lvalue.");
      emit(F_NUMBER,0);
      emit(F_NUMBER,0);
      return 2;

    case F_LVALUE_LIST:
    case F_LOCAL:
    case F_GLOBAL:
    case F_IDENTIFIER:
    case F_INDEX:
    case F_ARROW:
    case F_ARG_LIST:
      break;
    }
  }

  switch(n->token)
  {
  case F_PUSH_ARRAY:
    code_expression(CAR(n), 0, "`@");
    emit2(F_PUSH_ARRAY);
    return -0x7ffffff;

  case '?':
  {
    INT32 *prev_switch_jumptable = current_switch_jumptable;
    int adroppings , bdroppings;
    current_switch_jumptable=0;


    if(!CDDR(n))
    {
      tmp1=alloc_label();
      do_jump_when_zero(CAR(n), tmp1);
      DO_CODE_BLOCK(CADR(n));
      emit(F_LABEL, tmp1);
      current_switch_jumptable = prev_switch_jumptable;
      return 0;
    }

    if(!CADR(n))
    {
      tmp1=alloc_label();
      do_jump_when_non_zero(CAR(n), tmp1);
      DO_CODE_BLOCK(CDDR(n));
      emit(F_LABEL,tmp1);
      current_switch_jumptable = prev_switch_jumptable;
      return 0;
    }

    tmp1=alloc_label();
    do_jump_when_zero(CAR(n),tmp1);

    adroppings=do_docode(CADR(n), flags);
    tmp3=emit(F_POP_N_ELEMS,0);

    /* Else */
    tmp2=do_jump(F_BRANCH,-1);
    emit(F_LABEL, tmp1);

    bdroppings=do_docode(CDDR(n), flags);
    if(adroppings < bdroppings)
    {
      do_pop(bdroppings - adroppings);
    }

    if(adroppings > bdroppings)
    {
      update_arg(tmp3,adroppings-bdroppings);
      adroppings=bdroppings;
    }

    emit(F_LABEL, tmp2);

    current_switch_jumptable = prev_switch_jumptable;
    return adroppings;
  }
      
  case F_AND_EQ:
  case F_OR_EQ:
  case F_XOR_EQ:
  case F_LSH_EQ:
  case F_RSH_EQ:
  case F_ADD_EQ:
  case F_SUB_EQ:
  case F_MULT_EQ:
  case F_MOD_EQ:
  case F_DIV_EQ:
    tmp1=do_docode(CAR(n),DO_LVALUE);
#ifdef DEBUG
    if(tmp1 != 2)
      fatal("HELP! FATAL INTERNAL COMPILER ERROR\n");
#endif

    if(match_types(CAR(n)->type,array_type_string) ||
       match_types(CAR(n)->type,string_type_string))
    {
      code_expression(CDR(n), 0, "assignment");
      emit2(F_LTOSVAL2);
    }else{
      emit2(F_LTOSVAL);
      code_expression(CDR(n), 0, "assignment");
    }


    switch(n->token)
    {
    case F_ADD_EQ: emit2(F_ADD); break;
    case F_AND_EQ: emit2(F_AND); break;
    case F_OR_EQ:  emit2(F_OR);  break;
    case F_XOR_EQ: emit2(F_XOR); break;
    case F_LSH_EQ: emit2(F_LSH); break;
    case F_RSH_EQ: emit2(F_RSH); break;
    case F_SUB_EQ: emit2(F_SUBTRACT); break;
    case F_MULT_EQ:emit2(F_MULTIPLY);break;
    case F_MOD_EQ: emit2(F_MOD); break;
    case F_DIV_EQ: emit2(F_DIVIDE); break;
    }

    if(flags & DO_POP)
    {
      emit2(F_ASSIGN_AND_POP);
      return 0;
    }else{
      emit2(F_ASSIGN);
      return 1;
    }

  case F_ASSIGN:
    switch(CAR(n)->token)
    {
    case F_AND:
    case F_OR:
    case F_XOR:
    case F_LSH:
    case F_RSH:
    case F_ADD:
    case F_MOD:
    case F_SUBTRACT:
    case F_DIVIDE:
    case F_MULTIPLY:
      if(node_is_eq(CDR(n),CAAR(n)))
      {
	tmp1=do_docode(CDR(n),DO_LVALUE);
	if(match_types(CDR(n)->type,array_type_string) ||
	   match_types(CDR(n)->type,string_type_string))
	{
	  code_expression(CDAR(n), 0, "binary operand");
	  emit2(F_LTOSVAL2);
	}else{
	  emit2(F_LTOSVAL);
	  code_expression(CDAR(n), 0, "binary operand");
	}

	emit2(CAR(n)->token);

	emit2(n->token);
	return n->token==F_ASSIGN;
      }

    default:
      switch(CDR(n)->token)
      {
      case F_LOCAL:
	if(CDR(n)->u.number >= local_variables->max_number_of_locals)
	  yyerror("Illegal to use local variable here.");

	code_expression(CAR(n), 0, "RHS");
	emit(flags & DO_POP ? F_ASSIGN_LOCAL_AND_POP:F_ASSIGN_LOCAL,
	     CDR(n)->u.number );
	break;

      case F_IDENTIFIER:
	if(!IDENTIFIER_IS_VARIABLE( ID_FROM_INT(& fake_program, CDR(n)->u.number)->identifier_flags))
	{
	  yyerror("Cannot assign functions or constants.\n");
	}else{
	  code_expression(CAR(n), 0, "RHS");
	  emit(flags & DO_POP ? F_ASSIGN_GLOBAL_AND_POP:F_ASSIGN_GLOBAL,
	       CDR(n)->u.number);
	}
	break;

      default:
	tmp1=do_docode(CDR(n),DO_LVALUE);
	if(do_docode(CAR(n),0)!=1) yyerror("RHS is void!");
	emit2(flags & DO_POP ? F_ASSIGN_AND_POP:F_ASSIGN);
	break;
      }
      return flags & DO_POP ? 0 : 1;
    }

  case F_LAND:
  case F_LOR:
    tmp1=alloc_label();
    do_cond_jump(CAR(n), tmp1, n->token == F_LOR, 0);
    if(do_docode(CDR(n),0)!=1) fatal("Compiler logical error.\n");
    emit(F_LABEL,tmp1);
    return 1;

  case F_EQ:
  case F_NE:
  case F_ADD:
  case F_LT:
  case F_LE:
  case F_GT:
  case F_GE:
  case F_SUBTRACT:
  case F_MULTIPLY:
  case F_DIVIDE:
  case F_MOD:
  case F_LSH:
  case F_RSH:
  case F_XOR:
  case F_OR:
  case F_AND:
  case F_NOT:
  case F_COMPL:
  case F_NEGATE:
    fatal("Optimizer errror.\n");

  case F_RANGE:
    tmp1=do_docode(CAR(n),DO_NOT_COPY);
    if(do_docode(CDR(n),DO_NOT_COPY)!=2)
      fatal("Compiler internal error (at %ld).\n",(long)current_line);
    emit2(n->token);
    return tmp1;

  case F_INC:
  case F_POST_INC:
    tmp1=do_docode(CAR(n),DO_LVALUE);
#ifdef DEBUG
    if(tmp1 != 2)
      fatal("HELP! FATAL INTERNAL COMPILER ERROR (again)\n");
#endif

    if(flags & DO_POP)
    {
      emit2(F_INC_AND_POP);
      return 0;
    }else{
      emit2(n->token);
      return 1;
    }

  case F_DEC:
  case F_POST_DEC:
    tmp1=do_docode(CAR(n),DO_LVALUE);
#ifdef DEBUG
    if(tmp1 != 2)
      fatal("HELP! FATAL INTERNAL COMPILER ERROR (yet again)\n");
#endif
    if(flags & DO_POP)
    {
      emit2(F_DEC_AND_POP);
      return 0;
    }else{
      emit2(n->token);
      return 1;
    }

  case F_FOR:
  {
    INT32 *prev_switch_jumptable = current_switch_jumptable;
    INT32 break_save = current_break;
    INT32 continue_save = current_continue;
    
    current_switch_jumptable=0;
    current_break=alloc_label();
    current_continue=alloc_label();

    if(CDR(n))
    {
      do_jump_when_zero(CAR(n),current_break);
      tmp2=ins_label(-1);
      DO_CODE_BLOCK(CADR(n));
      ins_label(current_continue);
      DO_CODE_BLOCK(CDDR(n));
    }else{
      tmp2=ins_label(-1);
    }
    do_jump_when_non_zero(CAR(n),tmp2);
    ins_label(current_break);

    current_switch_jumptable = prev_switch_jumptable;
    current_break=break_save;
    current_continue=continue_save;
    return 0;
  }

  case ' ':
    ret = do_docode(CAR(n),0);
    return ret + do_docode(CDR(n),DO_LVALUE);

  case F_FOREACH:
  {
    INT32 *prev_switch_jumptable = current_switch_jumptable;
    INT32 break_save = current_break;
    INT32 continue_save = current_continue;

    current_switch_jumptable=0;
    current_break=alloc_label();
    current_continue=alloc_label();

    tmp2=do_docode(CAR(n),DO_NOT_COPY);
    emit2(F_CONST0);
    tmp3=do_jump(F_BRANCH,-1);
    tmp1=ins_label(-1);
    DO_CODE_BLOCK(CDR(n));
    ins_label(current_continue);
    emit(F_LABEL,tmp3);
    do_jump(n->token,tmp1);
    ins_label(current_break);

    current_switch_jumptable = prev_switch_jumptable;
    current_break=break_save;
    current_continue=continue_save;
    do_pop(4);
    return 0;
  }

  case F_INC_NEQ_LOOP:
  case F_DEC_NEQ_LOOP:
  case F_INC_LOOP:
  case F_DEC_LOOP:
  {
    INT32 *prev_switch_jumptable = current_switch_jumptable;
    INT32 break_save = current_break;
    INT32 continue_save = current_continue;

    current_switch_jumptable=0;
    current_break=alloc_label();
    current_continue=alloc_label();

    tmp2=do_docode(CAR(n),0);
    tmp3=do_jump(F_BRANCH,-1);
    tmp1=ins_label(-1);

    DO_CODE_BLOCK(CDR(n));
    ins_label(current_continue);
    emit(F_LABEL,tmp3);
    do_jump(n->token,tmp1);
    ins_label(current_break);

    current_switch_jumptable = prev_switch_jumptable;
    current_break=break_save;
    current_continue=continue_save;
    do_pop(3);
    return 0;
  }

  case F_DO:
  {
    INT32 *prev_switch_jumptable = current_switch_jumptable;
    INT32 break_save = current_break;
    INT32 continue_save = current_continue;

    current_switch_jumptable=0;
    current_break=alloc_label();
    current_continue=alloc_label();

    tmp2=ins_label(-1);
    DO_CODE_BLOCK(CAR(n));
    ins_label(current_continue);
    do_jump_when_non_zero(CDR(n),tmp2);
    ins_label(current_break);

    current_switch_jumptable = prev_switch_jumptable;
    current_break=break_save;
    current_continue=continue_save;
    return 0;
  }

  case F_CAST:
    if(n->type==void_type_string)
    {
      DO_CODE_BLOCK(CAR(n));
      return 0;
    }

    tmp1=do_docode(CAR(n),0);
    if(!tmp1) { emit2(F_CONST0); tmp1=1; }
    if(tmp1>1) do_pop(tmp1-1);

    tmp1=store_prog_string(n->type);
    emit(F_STRING,tmp1);
    emit2(F_CAST);
    return 1;

  case F_APPLY:
    if(CAR(n)->token == F_CONSTANT)
    {
      if(CAR(n)->u.sval.type == T_FUNCTION)
      {
	if(CAR(n)->u.sval.subtype == FUNCTION_BUILTIN) /* driver fun? */
	{
	  if(!CAR(n)->u.sval.u.efun->docode || 
	     !CAR(n)->u.sval.u.efun->docode(n))
	  {
	    emit2(F_MARK);
	    do_docode(CDR(n),0);
	    tmp1=store_constant(& CAR(n)->u.sval,
				!(CAR(n)->tree_info & OPT_EXTERNAL_DEPEND));
	    emit(F_APPLY,tmp1);
	  }
	  if(n->type == void_type_string) return 0;
	  return 1;
	}else{
	  if(CAR(n)->u.sval.u.object == &fake_object)
	  {
	    emit2(F_MARK);
	    do_docode(CDR(n),0);
	    emit(F_CALL_LFUN, CAR(n)->u.sval.subtype);
	    return 1;
	  }
       	}
      }

      emit2(F_MARK);
      do_docode(CDR(n),0);
      tmp1=store_constant(& CAR(n)->u.sval,
			  !(CAR(n)->tree_info & OPT_EXTERNAL_DEPEND));
      emit(F_APPLY,tmp1);
      
      return 1;
    }
    else if(CAR(n)->token == F_IDENTIFIER &&
	    IDENTIFIER_IS_FUNCTION(ID_FROM_INT(& fake_program, CAR(n)->u.number)->identifier_flags))
    {
      emit2(F_MARK);
      do_docode(CDR(n),0);
      emit(F_CALL_LFUN, CAR(n)->u.number);
      return 1;
    }
    else
    {
      struct pike_string *tmp;
      struct efun *fun;
      node *foo;

      emit2(F_MARK);
      do_docode(CAR(n),0);
      do_docode(CDR(n),0);

      tmp=findstring("call_function");
      if(!tmp) yyerror("No call_function efun.");
      if(!find_module_identifier(tmp))
      {
	yyerror("No call_function efun.");
      }else{
	tmp1=store_constant(sp-1, 1);
	pop_stack();
	emit(F_APPLY, tmp1);
      }
      return 1;
    }

  case F_ARG_LIST:
    tmp1=do_docode(CAR(n),flags & ~DO_LVALUE);
    tmp1+=do_docode(CDR(n),flags);
    return tmp1;


    /* Switch:
     * So far all switches are implemented with a binsearch lookup.
     * It stores the case values in the programs area for constants.
     * It also has a jump-table in the program itself, for every index in
     * the array of cases, there is 2 indexes in the jumptable, and one extra.
     * The first entry in the jumptable is used if you call switch with
     * a value that is ranked lower than all the indexes in the array of
     * cases. (Ranked by the binsearch that is) The second is used if it
     * is equal to the first index. The third if it is greater than the
     * first, but lesser than the second. The fourth if it is equal to
     * the second.... etc. etc.
     */

  case F_SWITCH:
  {
    INT32 e,cases,*order;
    INT32 *jumptable;
    INT32 prev_switch_values_on_stack = current_switch_values_on_stack;
    INT32 prev_switch_case = current_switch_case;
    INT32 prev_switch_default = current_switch_default;
    INT32 *prev_switch_jumptable = current_switch_jumptable;
    INT32 break_save = current_break;
#ifdef DEBUG
    struct svalue *save_sp=sp;
#endif

    if(do_docode(CAR(n),0)!=1)
      fatal("Internal compiler error, time to panic\n");

    current_break=alloc_label();

    cases=count_cases(CDR(n));

    tmp1=emit(F_SWITCH,0);
    emit(F_ALIGN,sizeof(INT32));

    current_switch_values_on_stack=0;
    current_switch_case=0;
    current_switch_default=-1;
    current_switch_jumptable=(INT32 *)xalloc(sizeof(INT32)*(cases*2+1));
    jumptable=(INT32 *)xalloc(sizeof(INT32)*(cases*2+1));

    for(e=0; e<cases*2+1; e++)
    {
      jumptable[e]=emit(F_POINTER, 0);
      current_switch_jumptable[e]=-1;
    }

    current_switch_jumptable[current_switch_case++]=-1;

    DO_CODE_BLOCK(CDR(n));

#ifdef DEBUG
    if(sp-save_sp != cases)
      fatal("Count cases is wrong!\n");
#endif
    
    f_aggregate(cases);
    order=get_switch_order(sp[-1].u.array);

    /* Check for cases inside a range */
    for(e=0; e<cases-1; e++)
    {
      if(order[e] < cases-1)
      {
	int o1=order[e]*2+1;
	if(current_switch_jumptable[o1]==current_switch_jumptable[o1+1] &&
	   current_switch_jumptable[o1]==current_switch_jumptable[o1+2])
	{
	  if(order[e]+1 != order[e+1])
	    yyerror("Case inside range.");
          e++;
	}
      }
    }

    if(current_switch_default < 0)
      current_switch_default = ins_label(-1);

    for(e=0;e<cases*2+1;e++)
      if(current_switch_jumptable[e]==-1)
	current_switch_jumptable[e]=current_switch_default;

    sp[-1].u.array=order_array(sp[-1].u.array,order);

    reorder((void *)(current_switch_jumptable+1),cases,sizeof(INT32)*2,order);
    free((char *)order);

    for(e=0; e<cases*2+1; e++)
      update_arg(jumptable[e], current_switch_jumptable[e]);

    update_arg(tmp1, store_constant(sp-1,1));

    pop_stack();
    free((char *)jumptable);
    free((char *)current_switch_jumptable);

    current_switch_jumptable = prev_switch_jumptable;
    current_switch_default = prev_switch_default;
    current_switch_case = prev_switch_case;
    current_switch_values_on_stack = prev_switch_values_on_stack ;

    emit(F_LABEL, current_break);

    current_break=break_save;
#ifdef DEBUG
    if(recoveries && sp-evaluator_stack < recoveries->sp)
      fatal("Stack error after F_SWITCH (underflow)\n");
#endif
    return 0;
  }

  case F_CASE:
  {
    if(!current_switch_jumptable)
    {
      yyerror("Case outside switch.");
    }else{
      node *lower=CAR(n);
      if(!lower) lower=CDR(n);

      if(!is_const(lower))
	yyerror("Case label isn't constant.");

      tmp1=eval_low(lower);
      if(tmp1<1)
      {
	yyerror("Error in case label.");
	push_int(0);
	tmp1=1;
      }
      pop_n_elems(tmp1-1);
      current_switch_values_on_stack++;
      for(tmp1=current_switch_values_on_stack; tmp1 > 1; tmp1--)
	if(is_equal(sp-tmp1, sp-1))
	  yyerror("Duplicate case.");

      current_switch_jumptable[current_switch_case++]=ins_label(-1);

      if(CDR(n))
      {
	current_switch_jumptable[current_switch_case]=
	  current_switch_jumptable[current_switch_case-1];
	current_switch_case++;

	if(CAR(n))
	{
	  if(!is_const(CDR(n)))
	    yyerror("Case label isn't constant.");
	  
	  current_switch_jumptable[current_switch_case]=
	    current_switch_jumptable[current_switch_case-1];
	  current_switch_case++;

	  tmp1=eval_low(CDR(n));
	  if(tmp1<1)
	  {
	    yyerror("Error in second half of case label.");
	    push_int(0);
	    tmp1=1;
	  }
	  pop_n_elems(tmp1-1);
	  current_switch_values_on_stack++;
	  for(tmp1=current_switch_values_on_stack; tmp1 > 1; tmp1--)
	    if(is_equal(sp-tmp1, sp-1))
	      yyerror("Duplicate case.");
	  current_switch_jumptable[current_switch_case++]=-1;
	}
      }else{
	current_switch_jumptable[current_switch_case++]=-1;
      }
    }
    return 0;
  }

  case F_DEFAULT:
    if(!current_switch_jumptable)
    {
      yyerror("Default outside switch.");
    }else if(current_switch_default!=-1){
      yyerror("Duplicate switch default.");
    }else{
      current_switch_default = ins_label(-1);
    }
    return 0;

  case F_BREAK:
    if(current_break == -1)
    {
      yyerror("Break outside loop or switch.");
    }else{
      do_jump(F_BRANCH, current_break);
    }
    return 0;

  case F_CONTINUE:
    if(current_continue == -1)
    {
      yyerror("continue outside loop or switch.");
    }else{
      do_jump(F_BRANCH, current_continue);
    }
    return 0;

  case F_RETURN:
    do_docode(CAR(n),0);
    emit2(F_RETURN);
    return 0;

  case F_SSCANF:
    tmp1=do_docode(CAR(n),DO_NOT_COPY);
    tmp2=do_docode(CDR(n),DO_NOT_COPY | DO_LVALUE);
    emit(F_SSCANF,tmp1+tmp2);
    return 1;

  case F_CATCH:
  {
    INT32 break_save = current_break;
    INT32 continue_save = current_continue;
    INT32 *prev_switch_jumptable = current_switch_jumptable;

    current_switch_jumptable=0;
    current_break=alloc_label();
    current_continue=alloc_label();

    tmp1=do_jump(F_CATCH,-1);
    DO_CODE_BLOCK(CAR(n));
    ins_label(current_continue);
    ins_label(current_break);
    emit2(F_THROW_ZERO);
    ins_label(tmp1);

    current_break=break_save;
    current_continue=continue_save;
    current_switch_jumptable = prev_switch_jumptable;
    return 1;
  }

  case F_LVALUE_LIST:
    ret = do_docode(CAR(n),DO_LVALUE);
    return ret + do_docode(CDR(n),DO_LVALUE);

  case F_ARROW:
    if(CDR(n)->token != F_CONSTANT || CDR(n)->u.sval.type!=T_STRING)
      fatal("Bugg in F_ARROW, index not string.");
    if(flags & DO_LVALUE)
    {
      /* FIXME!!!! */
      tmp1=do_docode(CAR(n), 0);
      emit(F_ARROW_STRING, store_prog_string(CDR(n)->u.sval.u.string));
      return 2;
    }else{
      tmp1=do_docode(CAR(n), DO_NOT_COPY);
      emit(F_ARROW, store_prog_string(CDR(n)->u.sval.u.string));
      if(!(flags & DO_NOT_COPY))
      {
	while(n && (n->token==F_INDEX || n->token==F_ARROW)) n=CAR(n);
	if(n->token==F_CONSTANT && !(n->node_info & OPT_EXTERNAL_DEPEND))
	  emit2(F_COPY_VALUE);
      }
    }
    return tmp1;

  case F_INDEX:
    if(flags & DO_LVALUE)
    {
      tmp1=do_docode(CAR(n), 0);
      if(do_docode(CDR(n),0) != 1)
	fatal("Internal compiler error, please report this (1).");
      if(CDR(n)->token != F_CONSTANT &&
	match_types(CDR(n)->type, string_type_string))
	emit2(F_CLEAR_STRING_SUBTYPE);
      return 2;
    }else{
      tmp1=do_docode(CAR(n), DO_NOT_COPY);
      code_expression(CDR(n), DO_NOT_COPY, "index");
      emit2(F_INDEX);
      if(!(flags & DO_NOT_COPY))
      {
	while(n && (n->token==F_INDEX || n->token==F_ARROW)) n=CAR(n);
	if(n->token==F_CONSTANT && !(n->node_info & OPT_EXTERNAL_DEPEND))
	  emit2(F_COPY_VALUE);
      }
    }
    return tmp1;

  case F_CONSTANT:
    switch(n->u.sval.type)
    {
    case T_INT:
      emit(F_NUMBER,n->u.sval.u.integer);
      return 1;

    case T_STRING:
      tmp1=store_prog_string(n->u.sval.u.string);
      emit(F_STRING,tmp1);
      return 1;

    case T_FUNCTION:
      if(n->u.sval.subtype!=FUNCTION_BUILTIN)
      {
	if(n->u.sval.u.object == &fake_object)
	{
	  emit(F_LFUN,n->u.sval.subtype);
	  return 1;
	}
      }

    default:
      tmp1=store_constant(&(n->u.sval),!(n->tree_info & OPT_EXTERNAL_DEPEND));
      emit(F_CONSTANT,tmp1);
      return 1;

    case T_ARRAY:
    case T_MAPPING:
    case T_MULTISET:
      tmp1=store_constant(&(n->u.sval),!(n->tree_info & OPT_EXTERNAL_DEPEND));
      emit(F_CONSTANT,tmp1);
      
      /* copy now or later ? */
      if(!(flags & DO_NOT_COPY) && !(n->tree_info & OPT_EXTERNAL_DEPEND))
	emit2(F_COPY_VALUE);
      return 1;

    }

  case F_LOCAL:
    if(n->u.number >= local_variables->max_number_of_locals)
      yyerror("Illegal to use local variable here.");
    if(flags & DO_LVALUE)
    {
      emit(F_LOCAL_LVALUE,n->u.number);
      return 2;
    }else{
      emit(F_LOCAL,n->u.number);
      return 1;
    }

  case F_IDENTIFIER:
    if(IDENTIFIER_IS_FUNCTION(ID_FROM_INT(& fake_program, n->u.number)->identifier_flags))
    {
      if(flags & DO_LVALUE)
      {
	yyerror("Cannot assign functions.\n");
      }else{
	emit(F_LFUN,n->u.number);
      }
    }else{
      if(flags & DO_LVALUE)
      {
	emit(F_GLOBAL_LVALUE,n->u.number);
	return 2;
      }else{
	emit(F_GLOBAL,n->u.number);
      }
    }
    return 1;

  case F_VAL_LVAL:
    ret = do_docode(CAR(n),flags);
    return ret + do_docode(CDR(n),flags | DO_LVALUE);
    
  default:
    fatal("Infernal compiler error (unknown parse-tree-token).\n");
    return 0;			/* make gcc happy */
  }
}

void do_code_block(node *n)
{
  init_bytecode();
  label_no=0;
  DO_CODE_BLOCK(n);
  assemble();
}

int docode(node *n)
{
  int tmp;
  int label_no_save = label_no;
  dynamic_buffer instrbuf_save = instrbuf;

  instrbuf.s.str=0;
  label_no=0;
  init_bytecode();

  tmp=do_docode(n,0);
  assemble();

  instrbuf=instrbuf_save;
  label_no = label_no_save;
  return tmp;
}
