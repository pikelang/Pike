/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
/**/
#include "global.h"
RCSID("$Id: docode.c,v 1.70 2006/03/02 10:37:58 grubba Exp $");
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
#include "object.h"
#include "mapping.h"
#include "multiset.h"

static int do_docode2(node *n,int flags);

INT32 current_break=-1;
INT32 current_continue=-1;

static INT32 current_switch_case;
static INT32 current_switch_default;
static INT32 current_switch_values_on_stack;
static INT32 *current_switch_jumptable =0;
static struct pike_string *current_switch_type = NULL;

void upd_int(int offset, INT32 tmp)
{
  MEMCPY(new_program->program+offset, (char *)&tmp,sizeof(tmp));
}

INT32 read_int(int offset)
{
  return EXTRACT_INT(new_program->program+offset);
}

int store_linenumbers=1;
static int label_no=0;

int alloc_label(void) { return ++label_no; }

int do_jump(int token,INT32 lbl)
{
  if(lbl==-1) lbl=alloc_label();
  emit(token, lbl);
  return lbl;
}

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

#define DO_CODE_BLOCK(X) do_pop(do_docode((X),DO_NOT_COPY | DO_POP))

int do_docode(node *n,INT16 flags)
{
  int i;
  int save_current_line=lex.current_line;
  if(!n) return 0;
  lex.current_line=n->line_number;
  i=do_docode2(check_node_hash(n), flags);

  lex.current_line=save_current_line;
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
  switch(do_docode(check_node_hash(n), flags & ~ DO_POP))
  {
  case 0: my_yyerror("Void expression for %s",err);
  case 1: return;
  case 2:
    fatal("Internal compiler error (%s), line %ld, file %s\n",
	  err,
	  (long)lex.current_line,
	  lex.current_file?lex.current_file->str:"Unknown");
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

static inline struct compiler_frame *find_local_frame(INT32 depth)
{
  struct compiler_frame *f=compiler_frame;
  while(--depth>=0) f=f->previous;
  return f;
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
	
      case F_ARRAY_LVALUE:
      case F_LVALUE_LIST:
      case F_LOCAL:
      case F_GLOBAL:
      case F_IDENTIFIER:
      case F_INDEX:
      case F_ARROW:
      case F_ARG_LIST:
      case F_COMMA_EXPR:
      case F_EXTERNAL:
	  break;
      }
  }

  if(flags & DO_LVALUE_IF_POSSIBLE)
  {
    flags|=DO_INDIRECT;
    flags &=~DO_LVALUE_IF_POSSIBLE;
  }else{
    flags &=~DO_INDIRECT;
  }

  /* Stack check */
  {
    long x_= ((char *)&x_) + STACK_DIRECTION * (32768) - stack_top ;
    x_*=STACK_DIRECTION;						
    if(x_>0)
    {
      yyerror("Too deep recursion in compiler. (please report this)");

      emit(F_NUMBER,0);
      if(flags & DO_LVALUE)
      {
	emit(F_NUMBER,0);
	return 2;
      }
      return 1;
    }
  }

  switch(n->token)
  {
  case F_MAGIC_INDEX:
  case F_MAGIC_SET_INDEX:
    emit(F_LDA, n->u.node.a->u.sval.u.integer);
    emit(n->token, n->u.node.b->u.sval.u.integer);
    return 1;
      
  case F_EXTERNAL:
    emit(F_LDA, n->u.integer.a);
    if(flags & WANT_LVALUE)
    {
      emit(F_EXTERNAL_LVALUE, n->u.integer.b);
      return 2;
    }else{
      emit(F_EXTERNAL, n->u.integer.b);
      return 1;
    }
    break;

  case F_UNDEFINED:
    yyerror("Undefined identifier");
    emit(F_NUMBER,0);
    return 1;

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
#ifdef PIKE_DEBUG
    if(tmp1 != 2)
      fatal("HELP! FATAL INTERNAL COMPILER ERROR (7)\n");
#endif

    if(match_types(CAR(n)->type,array_type_string) ||
       match_types(CAR(n)->type,string_type_string) ||
       match_types(CAR(n)->type,object_type_string))
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
	if(CDR(n)->u.integer.a >= 
	   find_local_frame(CDR(n)->u.integer.b)->max_number_of_locals)
	  yyerror("Illegal to use local variable here.");

	if(CDR(n)->u.integer.b) goto normal_assign;

	if (CDR(n)->node_info & OPT_ASSIGNMENT) {
	  /* Initialize. */
	  emit0(F_CONST0);
	  emit1(F_ASSIGN_LOCAL_AND_POP, CDR(n)->u.integer.a);
	}
	code_expression(CAR(n), 0, "RHS");
	emit(flags & DO_POP ? F_ASSIGN_LOCAL_AND_POP:F_ASSIGN_LOCAL,
	     CDR(n)->u.integer.a );
	break;

      case F_IDENTIFIER:
	if(!IDENTIFIER_IS_VARIABLE( ID_FROM_INT(new_program, CDR(n)->u.id.number)->identifier_flags))
	{
	  yyerror("Cannot assign functions or constants.\n");
	}else{
	  code_expression(CAR(n), 0, "RHS");
	  emit(flags & DO_POP ? F_ASSIGN_GLOBAL_AND_POP:F_ASSIGN_GLOBAL,
	       CDR(n)->u.id.number);
	}
	break;

      default:
      normal_assign:
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
    code_expression(CDR(n), flags, n->token == F_LOR ? "||" : "&&");
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
    fatal("Optimizer error.\n");

  case F_RANGE:
    tmp1=do_docode(CAR(n),DO_NOT_COPY_TOPLEVEL);
    if(do_docode(CDR(n),DO_NOT_COPY)!=2)
      fatal("Compiler internal error (at %ld).\n",(long)lex.current_line);
    emit2(n->token);
    return tmp1;

  case F_INC:
  case F_POST_INC:
    tmp1=do_docode(CAR(n),DO_LVALUE);
#ifdef PIKE_DEBUG
    if(tmp1 != 2)
      fatal("HELP! FATAL INTERNAL COMPILER ERROR (1)\n");
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
#ifdef PIKE_DEBUG
    if(tmp1 != 2)
      fatal("HELP! FATAL INTERNAL COMPILER ERROR (2)\n");
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
    node *arr;
    INT32 *prev_switch_jumptable = current_switch_jumptable;
    INT32 break_save = current_break;
    INT32 continue_save = current_continue;

    current_switch_jumptable=0;
    current_break=alloc_label();
    current_continue=alloc_label();

    arr=CAR(n);
    
    if(arr->token==F_RANGE)
    {
      node **a1=my_get_arg(&_CDR(n),0);
      node **a2=my_get_arg(&_CDR(n),1);
      if(a1 && a2 && a2[0]->token==F_CONSTANT &&
	 a2[0]->u.sval.type==T_INT &&
	 a2[0]->u.sval.type==0x7fffffff &&
	a1[0]->type == int_type_string)
      {
	tmp2=do_docode(CAR(arr),DO_NOT_COPY_TOPLEVEL);
	do_docode(*a1,DO_NOT_COPY);
	goto foreach_arg_pushed;
      }
    }
    tmp2=do_docode(CAR(n),DO_NOT_COPY);
    emit2(F_CONST0);

  foreach_arg_pushed:
#ifdef PIKE_DEBUG
    /* This is really ugly because there is always a chance that the bug
     * will disappear when new instructions are added to the code, but
     * think it is worth it.
     */
    if(d_flag)
      emit2(F_MARK);
#endif
    tmp3=do_jump(F_BRANCH,-1);
    tmp1=ins_label(-1);
    DO_CODE_BLOCK(CDR(n));
    ins_label(current_continue);
    emit(F_LABEL,tmp3);
    do_jump(n->token,tmp1);
    ins_label(current_break);

#ifdef PIKE_DEBUG
    if(d_flag)
      emit2(F_POP_MARK);
#endif

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
#ifdef PIKE_DEBUG
    /* This is really ugly because there is always a chance that the bug
     * will disappear when new instructions are added to the code, but
     * think it is worth it.
     */
    if(d_flag)
      emit2(F_MARK);
#endif
    tmp3=do_jump(F_BRANCH,-1);
    tmp1=ins_label(-1);

    DO_CODE_BLOCK(CDR(n));
    ins_label(current_continue);
    emit(F_LABEL,tmp3);
    do_jump(n->token,tmp1);
    ins_label(current_break);
#ifdef PIKE_DEBUG
    if(d_flag)
      emit2(F_POP_MARK);
#endif

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

  case F_POP_VALUE:
    {
      DO_CODE_BLOCK(CAR(n));
      return 0;
    }

  case F_CAST:
    if(n->type==void_type_string)
    {
      DO_CODE_BLOCK(CAR(n));
      return 0;
    }
    tmp1=store_prog_string(n->type);
    emit(F_STRING,tmp1);

    tmp1=do_docode(CAR(n),0);
    if(!tmp1) { emit2(F_CONST0); tmp1=1; }
    if(tmp1>1) do_pop(tmp1-1);

    emit2(F_CAST);
    return 1;

  case F_SOFT_CAST:
    if (runtime_options & RUNTIME_CHECK_TYPES) {
      tmp1 = store_prog_string(n->type);
      emit(F_STRING, tmp1);
      tmp1 = do_docode(CAR(n), 0);
      if (!tmp1) { emit2(F_CONST0); tmp1 = 1; }
      if (tmp1 > 1) do_pop(tmp1 - 1);
      emit2(F_SOFT_CAST);
      return 1;
    }
    tmp1 = do_docode(CAR(n), flags);
    if (tmp1 > 1) do_pop(tmp1 - 1);
    return !!tmp1;

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
				!(CAR(n)->tree_info & OPT_EXTERNAL_DEPEND),
				CAR(n)->name);
	    emit(F_APPLY,tmp1);
	  }
	  if(n->type == void_type_string)
	    return 0;

	  return 1;
	}else{
	  if(CAR(n)->u.sval.u.object == fake_object)
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
			  !(CAR(n)->tree_info & OPT_EXTERNAL_DEPEND),
			  CAR(n)->name);
      emit(F_APPLY,tmp1);
      
      return 1;
    }
    else if(CAR(n)->token == F_IDENTIFIER &&
	    IDENTIFIER_IS_FUNCTION(ID_FROM_INT(new_program, CAR(n)->u.id.number)->identifier_flags))
    {
      emit2(F_MARK);
      do_docode(CDR(n),0);
      emit(F_CALL_LFUN, CAR(n)->u.id.number);
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
      foo=find_module_identifier(tmp,0);
      if(!foo || !foo->token==F_CONSTANT)
      {
	yyerror("No call_function efun.");
      }else{
	if(foo->u.sval.type == T_FUNCTION &&
	   foo->u.sval.subtype == FUNCTION_BUILTIN &&
	   foo->u.sval.u.efun->function == f_call_function)
	{
	  emit2(F_CALL_FUNCTION);
	}else{
	  /* We might want to put "predef::"+foo->name here /Hubbe */
	  tmp1=store_constant(& foo->u.sval, 1, foo->name);
	  emit(F_APPLY, tmp1);
	}
      }
      free_node(foo);
      return 1;
    }

  case F_ARG_LIST:
  case F_COMMA_EXPR:
    tmp1=do_docode(CAR(n),flags & ~WANT_LVALUE);
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
    struct pike_string *prev_switch_type = current_switch_type;
#ifdef PIKE_DEBUG
    struct svalue *save_sp=sp;
#endif

    if(do_docode(CAR(n),0)!=1)
      fatal("Internal compiler error, time to panic\n");

    if (!(CAR(n) && (current_switch_type = CAR(n)->type))) {
      current_switch_type = mixed_type_string;
    }

    current_break=alloc_label();

    cases=count_cases(CDR(n));

    tmp1=emit(F_SWITCH,0);
    emit(F_ALIGN,sizeof(INT32));

    current_switch_values_on_stack=0;
    current_switch_case=1;
    current_switch_default=-1;
    current_switch_jumptable=(INT32 *)xalloc(sizeof(INT32)*(cases*2+2));
    jumptable=(INT32 *)xalloc(sizeof(INT32)*(cases*2+2));

    for(e=1; e<cases*2+2; e++)
    {
      jumptable[e]=emit(F_POINTER, 0);
      current_switch_jumptable[e]=-1;
    }

    current_switch_jumptable[current_switch_case++]=-1;

    DO_CODE_BLOCK(CDR(n));
    
#ifdef PIKE_DEBUG
    if(sp-save_sp != cases)
      fatal("Count cases is wrong!\n");
#endif

    f_aggregate(cases);
    order=get_switch_order(sp[-1].u.array);

    if (!num_parse_error) {
      /* Check for cases inside a range */
      for(e=0; e<cases-1; e++)
      {
	if(order[e] < cases-1)
	{
	  int o1=order[e]*2+2;
	  if(current_switch_jumptable[o1]==current_switch_jumptable[o1+1] &&
	     current_switch_jumptable[o1]==current_switch_jumptable[o1+2])
	  {
	    if(order[e]+1 != order[e+1])
	      yyerror("Case inside range.");
	    e++;
	  }
	}
      }
    }

    if(current_switch_default < 0)
      current_switch_default = ins_label(-1);

    for(e=1;e<cases*2+2;e++)
      if(current_switch_jumptable[e]==-1)
	current_switch_jumptable[e]=current_switch_default;

    order_array(sp[-1].u.array,order);

    reorder((void *)(current_switch_jumptable+2),cases,sizeof(INT32)*2,order);
    free((char *)order);

    for(e=1; e<cases*2+2; e++)
      update_arg(jumptable[e], current_switch_jumptable[e]);

    update_arg(tmp1, store_constant(sp-1,1,0));

    pop_stack();
    free((char *)jumptable);
    free((char *)current_switch_jumptable);

    current_switch_jumptable = prev_switch_jumptable;
    current_switch_default = prev_switch_default;
    current_switch_case = prev_switch_case;
    current_switch_values_on_stack = prev_switch_values_on_stack;
    current_switch_type = prev_switch_type;

    emit(F_LABEL, current_break);

    current_break=break_save;
#ifdef PIKE_DEBUG
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

      if (lower && lower->type) {
	if (!pike_types_le(lower->type, current_switch_type)) {
	  if (!match_types(lower->type, current_switch_type)) {
	    yytype_error("Type mismatch in case.",
			 current_switch_type, lower->type, 0);
	  } else if (lex.pragmas & ID_STRICT_TYPES) {
	    yytype_error("Type mismatch in case.",
			 current_switch_type, lower->type, YYTE_IS_WARNING);
	  }
	}
      }

      if (!num_parse_error) {
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
      } else {
	push_int(0);
	current_switch_values_on_stack++;
      }
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

	  if (!num_parse_error) {
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
	  } else {
	    push_int(0);
	    current_switch_values_on_stack++;
	  }
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

    case F_ARRAY_LVALUE:
      tmp1=do_docode(CAR(n),DO_LVALUE);
#ifdef PIKE_DEBUG
      if(tmp1 & 1)
	fatal("Very internal compiler error.\n");
#endif
      emit(F_ARRAY_LVALUE, tmp1>>1);
      return 2;

  case F_ARROW:
    if(CDR(n)->token != F_CONSTANT || CDR(n)->u.sval.type!=T_STRING)
      fatal("Bugg in F_ARROW, index not string.");
    if(flags & WANT_LVALUE)
    {
      /* FIXME!!!! ??? I wonder what needs fixing... /Hubbe */
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
    if(flags & WANT_LVALUE)
    {
      int mklval=CAR(n) && match_types(CAR(n)->type, string_type_string);
      tmp1=do_docode(CAR(n),
		     mklval ? DO_LVALUE_IF_POSSIBLE : 0);
      if(tmp1==2)
      {
#ifdef PIKE_DEBUG
	if(!mklval)
	  fatal("Unwanted lvalue!\n");
#endif
	emit2(F_INDIRECT);
      }
      
      if(do_docode(CDR(n),0) != 1)
	fatal("Internal compiler error, please report this (1).");
      if(CDR(n)->token != F_CONSTANT &&
	match_types(CDR(n)->type, string_type_string))
	emit2(F_CLEAR_STRING_SUBTYPE);
      return 2;
    }else{
      tmp1=do_docode(CAR(n), DO_NOT_COPY);

      code_expression(CDR(n), DO_NOT_COPY, "index");
      if(CDR(n)->token != F_CONSTANT &&
	match_types(CDR(n)->type, string_type_string))
	emit2(F_CLEAR_STRING_SUBTYPE);

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
      if(!n->u.sval.u.integer && n->u.sval.subtype==NUMBER_UNDEFINED)
      {
	emit2(F_UNDEFINED);
      }else{
	emit(F_NUMBER,n->u.sval.u.integer);
      }
      return 1;

    case T_STRING:
      tmp1=store_prog_string(n->u.sval.u.string);
      emit(F_STRING,tmp1);
      return 1;

    case T_FUNCTION:
      if(n->u.sval.subtype!=FUNCTION_BUILTIN)
      {
	if(n->u.sval.u.object == fake_object)
	{
	  emit(F_LFUN,n->u.sval.subtype);
	  return 1;
	}

	if(n->u.sval.u.object->next == n->u.sval.u.object)
	{
	  int x=0;
	  struct object *o;
	  
	  for(o=fake_object->parent;o!=n->u.sval.u.object;o=o->parent)
	    x++;
	  emit(F_LDA, x);
	  emit(F_EXTERNAL, n->u.sval.subtype);
	  new_program->flags |= PROGRAM_USES_PARENT;
	  return 1;
	}
      }
      
#ifdef PIKE_DEBUG
      case T_OBJECT:
	if(n->u.sval.u.object->next == n->u.sval.u.object)
	  fatal("Internal error: Pointer to parent cannot be a compile time constant!\n");
#endif

    default:
      tmp1=store_constant(&(n->u.sval),
			  !(n->tree_info & OPT_EXTERNAL_DEPEND),
			  n->name);
      emit(F_CONSTANT,tmp1);
      return 1;

    case T_ARRAY:
    case T_MAPPING:
    case T_MULTISET:
      tmp1=store_constant(&(n->u.sval),
			  !(n->tree_info & OPT_EXTERNAL_DEPEND),
			  n->name);
      emit(F_CONSTANT,tmp1);
      
      /* copy now or later ? */
      if(!(flags & DO_NOT_COPY) && !(n->tree_info & OPT_EXTERNAL_DEPEND))
      {
 	if(flags & DO_NOT_COPY_TOPLEVEL)
 	{
 	  switch(n->u.sval.type)
 	  {
 	    case T_ARRAY:
 	      array_fix_type_field(n->u.sval.u.array);
 	      if(n->u.sval.u.array -> type_field & BIT_COMPLEX)
 		emit2(F_COPY_VALUE);
 	      break;
	      
 	    case T_MAPPING:
 	      mapping_fix_type_field(n->u.sval.u.mapping);
 	      if((n->u.sval.u.mapping->data->ind_types |
 		  n->u.sval.u.mapping->data->val_types) & BIT_COMPLEX)
		emit2(F_COPY_VALUE);
 	      break;
	      
 	    case T_MULTISET:
 	      array_fix_type_field(n->u.sval.u.multiset->ind);
 	      if(n->u.sval.u.multiset->ind-> type_field & BIT_COMPLEX)
 		emit2(F_COPY_VALUE);
 	      break;
 	  }
 	}else{
 	  emit2(F_COPY_VALUE);
 	}
      }
      
      return 1;

    }

  case F_LOCAL:
    if(n->u.integer.a >= 
       find_local_frame(n->u.integer.b)->max_number_of_locals)
      yyerror("Illegal to use local variable here.");

    if(n->u.integer.b)
    {
      emit(F_LDA,n->u.integer.b);
      if(flags & WANT_LVALUE)
      {
	emit(F_LEXICAL_LOCAL_LVALUE,n->u.id.number);
	return 2;
      }else{
	emit(F_LEXICAL_LOCAL,n->u.id.number);
	return 1;
      }
    }else{
      if(flags & WANT_LVALUE)
      {
	if (n->node_info & OPT_ASSIGNMENT) {
	  /* Initialize the variable. */
	  emit0(F_CONST0);
	  emit1(F_ASSIGN_LOCAL_AND_POP, n->u.integer.a);
	}
	emit(F_LOCAL_LVALUE,n->u.id.number);
	return 2;
      }else{
	if (n->node_info & OPT_ASSIGNMENT) {
	  /* Initialize the variable. */
	  emit0(F_CONST0);
	  emit1(F_ASSIGN_LOCAL, n->u.integer.a);
	} else {
	  emit1(F_LOCAL, n->u.integer.a);
	}
	return 1;
      }
    }

    case F_TRAMPOLINE:
      emit(F_TRAMPOLINE,n->u.id.number);
      return 1;

  case F_IDENTIFIER:
    if(IDENTIFIER_IS_FUNCTION(ID_FROM_INT(new_program, n->u.id.number)->identifier_flags))
    {
      if(flags & WANT_LVALUE)
      {
	yyerror("Cannot assign functions.\n");
      }else{
	emit(F_LFUN,n->u.id.number);
      }
    }else{
      if(flags & WANT_LVALUE)
      {
	emit(F_GLOBAL_LVALUE,n->u.id.number);
	return 2;
      }else{
	emit(F_GLOBAL,n->u.id.number);
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
