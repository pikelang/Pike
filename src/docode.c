/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
#include "las.h"
#include "program.h"
#include "language.h"
#include "lpc_types.h"
#include "stralloc.h"
#include "interpret.h"
#include "add_efun.h"
#include "array.h"
#include "macros.h"
#include "error.h"
#include "memory.h"
#include "svalue.h"
#include "main.h"
#include "lex.h"
#include "builtin_efuns.h"

int *break_stack=0;
static int current_break,break_stack_size;
int *continue_stack=0;
static int current_continue,continue_stack_size;

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

static void upd_short(int offset, INT16 l)
{
#ifdef HANDLES_UNALIGNED_MEMORY_ACCESS
  *((INT16 *)(areas[A_PROGRAM].s.str+offset))=l;
#else
  MEMCPY(areas[A_PROGRAM].s.str+offset, (char *)&l,sizeof(l));
#endif
}

static void upd_int(int offset, INT32 tmp)
{
#ifdef HANDLES_UNALIGNED_MEMORY_ACCESS
  *((int *)(areas[A_PROGRAM].s.str+offset))=tmp;
#else
  MEMCPY(areas[A_PROGRAM].s.str+offset, (char *)&tmp,sizeof(tmp));
#endif
}

/*
 * Store an INT32.
 */
void ins_long(INT32 l,int area)
{
  add_to_mem_block(area, (char *)&l+0, sizeof(INT32));
}

int store_linenumbers=1;

static void low_ins_f_byte(unsigned int b)
{
  if(store_linenumbers) store_linenumber();

#if defined(LASDEBUG)
  if(lasdebug>1)
    if(store_linenumbers)
      fprintf(stderr,"Inserting f_byte %s (%d) at %ld\n",
	      get_instruction_name(b), b,
	      (long)PC);
#endif
  b-=F_OFFSET;
#ifdef OPCPROF
  if(store_linenumbers) add_compiled(b);
#endif
  if(b>255)
  {
    switch(b >> 8)
    {
    case 1: low_ins_f_byte(F_ADD_256); break;
    case 2: low_ins_f_byte(F_ADD_512); break;
    case 3: low_ins_f_byte(F_ADD_768); break;
    case 4: low_ins_f_byte(F_ADD_1024); break;
    default:
      low_ins_f_byte(F_ADD_256X);
      ins_byte(b/256,A_PROGRAM);
    }
    b&=255;
  }
  ins_byte((unsigned char)b,A_PROGRAM);
}

void ins_f_byte(unsigned int b)
{
#ifdef DEBUG
  if(a_flag>2)
    fprintf(stderr,">%6lx: %s\n",(long)PC,get_f_name(b));
#endif
  low_ins_f_byte(b);
}

static void ins_f_byte_with_numerical_arg(unsigned int a,unsigned int b)
{
  switch(b >> 8)
  {
  case 0 : break;
  case 1 : low_ins_f_byte(F_PREFIX_256); break;
  case 2 : low_ins_f_byte(F_PREFIX_512); break;
  case 3 : low_ins_f_byte(F_PREFIX_768); break;
  case 4 : low_ins_f_byte(F_PREFIX_1024); break;
  default:
    if( b < 256*256)
    {
      low_ins_f_byte(F_PREFIX_CHARX256);
      ins_byte(b>>8, A_PROGRAM);
    }else if(b < 256*256*256) {
      low_ins_f_byte(F_PREFIX_WORDX256);
      ins_byte(b >> 16, A_PROGRAM);
      ins_byte(b >> 8, A_PROGRAM);
    }else{
      low_ins_f_byte(F_PREFIX_24BITX256);
      ins_byte(b >> 24, A_PROGRAM);
      ins_byte(b >> 16, A_PROGRAM);
      ins_byte(b >> 8, A_PROGRAM);
    }
  }
  ins_f_byte(a);
#ifdef DEBUG
  if(a_flag>2)
    fprintf(stderr,">%6lx: argument = %u\n",(long)PC,b);
#endif
  ins_byte(b, A_PROGRAM);
}


static void ins_int(int i)
{
  switch(i)
  {
  case 0: ins_f_byte(F_CONST0); break;
  case 1: ins_f_byte(F_CONST1); break;
  case -1: ins_f_byte(F_CONST_1); break;
  default:
    if(i<0)
    {
      ins_f_byte_with_numerical_arg(F_NEG_NUMBER,-i);
    }else{
      ins_f_byte_with_numerical_arg(F_NUMBER,i);
    }
  }
}

static void ins_float(FLOAT_TYPE f)
{
  ins_f_byte(F_FLOAT);
  add_to_mem_block(A_PROGRAM,(char *)&f,sizeof(FLOAT_TYPE));
}

/*
 * A mechanism to remember addresses on a stack. The size of the stack is
 * defined in config.h.
 */
int comp_stackp;
INT32 comp_stack[COMPILER_STACK_SIZE];

void push_address()
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

INT32 pop_address()
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

static void do_pop(int nr)
{
  if(!nr) return;
  if(nr==1)
  {
    ins_f_byte(F_POP_VALUE);
  }else if(nr<256){
    ins_f_byte(F_POP_N_ELEMS);
    ins_byte(nr,A_PROGRAM);
  }else{
    ins_f_byte(F_POP_N_ELEMS);
    ins_byte(255,A_PROGRAM);
    do_pop(nr-255);
  }
}

/* routines to optimize jumps */

#define JUMP_CONDITIONAL 1
#define JUMP_UNSET 2

struct jump
{
  INT32 relative;
  INT32 whereto;
  INT32 wherefrom;
  INT32 address;
  unsigned char flags;
  short token;
};

static int jump_ptr=0, max_jumps=0;
static struct jump jumps[256];

static void low_set_branch(INT32 address,INT32 whereto,INT32 relative)
{
  int j,e;

  if(address<0) return;
  j=jump_ptr;
  e=max_jumps;
  while(e>=0)
  {
    if(jumps[j].address==address && (jumps[j].flags & JUMP_UNSET))
      break;
    if(j) j--; else j=max_jumps;
    e--;
  }
  if(e<0) j=-1; /* not found */
    
  for(e=0;e<=max_jumps;e++)
  {
    if(jumps[e].flags & JUMP_UNSET) continue;
    if(jumps[e].flags & JUMP_CONDITIONAL) continue;
    if(jumps[e].wherefrom!=whereto) continue;
#if defined(LASDEBUG)
    if(lasdebug>1) printf("Optimized Jump to a jump\n");
#endif
    whereto=jumps[e].whereto;
    break;
  }
  
  if(j>=0)
  {
    if(!(jumps[j].flags & JUMP_CONDITIONAL))
    {
      for(e=0;e<=max_jumps;e++)
      {
	if(jumps[e].flags & JUMP_UNSET) continue;
	if(jumps[e].whereto==jumps[j].wherefrom)
	{
	  upd_int(jumps[e].address,whereto - jumps[e].relative);
	  jumps[e].whereto=whereto;
#if defined(LASDEBUG)
	  if(lasdebug>1) printf("Optimized Jump to a jump\n");
#endif
	}
      }
    }
    jumps[j].relative=relative;
    jumps[j].whereto=whereto;
    jumps[j].flags&=~JUMP_UNSET;
  }
  upd_int(address,whereto - relative);
}

static void set_branch(INT32 address,INT32 whereto)
{
  low_set_branch(address,whereto,address);
}

static INT32 do_jump(int token,INT32 whereto)
{
  jump_ptr=(jump_ptr+1)%NELEM(jumps);
  if(jump_ptr>max_jumps) max_jumps=jump_ptr;

  jumps[jump_ptr].flags=JUMP_UNSET;
  jumps[jump_ptr].whereto=-1;
  jumps[jump_ptr].token=token;

  if(token!=F_BRANCH)
    jumps[jump_ptr].flags|=JUMP_CONDITIONAL;

  if(token>=0)
  {
    jumps[jump_ptr].wherefrom=PC;
    ins_f_byte(token);
  }else{
    jumps[jump_ptr].wherefrom=-1;
  }
  jumps[jump_ptr].relative=PC;
  jumps[jump_ptr].address=PC;
  ins_long(0, A_PROGRAM);
  if(whereto!=-1) set_branch(jumps[jump_ptr].address, whereto);
  return jumps[jump_ptr].address;
}

static void clean_jumptable() { max_jumps=jump_ptr=-1; }

struct jump_list
{
  int *stack;
  int current;
  int size;
};

static void push_break_stack(struct jump_list *x)
{
  x->stack=break_stack;
  x->current=current_break;
  x->size=break_stack_size;
  break_stack_size=10;
  break_stack=(int *)xalloc(sizeof(int)*break_stack_size);
  current_break=0;
}

static void pop_break_stack(struct jump_list *x,int jump)
{
  for(current_break--;current_break>=0;current_break--)
    set_branch(break_stack[current_break],jump);

  free((char *)break_stack);
  
  break_stack_size=x->size;
  current_break=x->current;
  break_stack=x->stack;
}

static void push_continue_stack(struct jump_list *x)
{
  x->stack=continue_stack;
  x->current=current_continue;
  x->size=continue_stack_size;
  continue_stack_size=10;
  continue_stack=(int *)xalloc(sizeof(int)*continue_stack_size);
  current_continue=0;
}

static void pop_continue_stack(struct jump_list *x,int jump)
{
  for(current_continue--;current_continue>=0;current_continue--)
    set_branch(continue_stack[current_continue],jump);

  free((char *)continue_stack);
  continue_stack_size=x->size;
  current_continue=x->current;
  continue_stack=x->stack;
}

static int do_docode2(node *n,int flags);

#define DO_LVALUE 1
#define DO_NOT_COPY 2
#define DO_POP 4

#define DO_CODE_BLOCK(N) do_pop(do_docode(N,DO_NOT_COPY | DO_POP))

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


int docode(node *n)
{
  clean_jumptable();
  return do_docode(n,0);
}

static INT32 do_jump_when_zero(node *n,int j);

static int do_jump_when_non_zero(node *n,int j)
{
  if(!node_is_tossable(n))
  {
    if(node_is_true(n))
      return do_jump(F_BRANCH,j);

    if(node_is_false(n))
      return -1;
  }

  if(n->token == F_NOT)
    return do_jump_when_zero(CAR(n), j);

  if(do_docode(n, DO_NOT_COPY)!=1)
    fatal("Infernal compiler skiterror.\n");
  return do_jump(F_BRANCH_WHEN_NON_ZERO,j);
}

static INT32 do_jump_when_zero(node *n,int j)
{
  if(!node_is_tossable(n))
  {
    if(node_is_true(n))
      return -1;

    if(node_is_false(n))
      return do_jump(F_BRANCH,j);
  }

  if(n->token == F_NOT)
    return do_jump_when_non_zero(CAR(n), j);

  if(do_docode(n, DO_NOT_COPY)!=1)
    fatal("Infernal compiler skiterror.\n");
  return do_jump(F_BRANCH_WHEN_ZERO,j);
}

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

  if(!n) return 0;

  if(flags & DO_LVALUE)
  {
    switch(n->token)
    {
    default:
      yyerror("Illegal lvalue.");
      ins_int(0);
      return 1;

    case F_LVALUE_LIST:
    case F_LOCAL:
    case F_GLOBAL:
    case F_IDENTIFIER:
    case F_INDEX:
    case F_ARG_LIST:
      break;
    }
  }

  switch(n->token)
  {
  case F_PUSH_ARRAY:
    tmp1=do_docode(CAR(n),0);
    if(tmp1!=1)
    {
      fatal("Internal compiler error, Yikes!\n");
    }
    ins_f_byte(F_PUSH_ARRAY);
    return -0x7ffffff;

  case '?':
  {
    if(!CDDR(n))
    {
      tmp1=do_jump_when_zero(CAR(n), -1);
      DO_CODE_BLOCK(CADR(n));
      set_branch(tmp1, PC);
      return 0;
    }

    if(!CADR(n))
    {
      tmp1=do_jump_when_non_zero(CAR(n), -1);
      DO_CODE_BLOCK(CDDR(n));
      set_branch(tmp1, PC);
      return 0;
    }

    tmp1=count_args(CDDR(n));
    tmp2=count_args(CADR(n));

    if(tmp2 < tmp1) tmp1=tmp2;

    if(tmp1 == -1)
      fatal("Unknown number of args in ? :\n");

    tmp2=do_jump_when_zero(CAR(n),-1);

    tmp3=do_docode(CADR(n), flags);
    if(tmp3 < tmp1) fatal("Count arguments was wrong.\n");
    do_pop(tmp3 - tmp1);

    tmp3=do_jump(F_BRANCH,-1);
    set_branch(tmp2, PC);

    tmp2=do_docode(CDDR(n), flags);
    if(tmp2 < tmp1) fatal("Count arguments was wrong.\n");
    do_pop(tmp2 - tmp1);
    set_branch(tmp3, PC);
    return tmp1;
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

    if(CAR(n)->type->str[0] == T_ARRAY)
    {
      if(do_docode(CDR(n), 0)!=1)
	fatal("Internal compiler error, shit happens\n");
      ins_f_byte(F_LTOSVAL2);
    }else{
      ins_f_byte(F_LTOSVAL);
      if(do_docode(CDR(n), 0)!=1)
	fatal("Internal compiler error, shit happens (again)\n");
    }


    switch(n->token)
    {
    case F_ADD_EQ: ins_f_byte(F_ADD); break;
    case F_AND_EQ: ins_f_byte(F_AND); break;
    case F_OR_EQ:  ins_f_byte(F_OR);  break;
    case F_XOR_EQ: ins_f_byte(F_XOR); break;
    case F_LSH_EQ: ins_f_byte(F_LSH); break;
    case F_RSH_EQ: ins_f_byte(F_RSH); break;
    case F_SUB_EQ: ins_f_byte(F_SUBTRACT); break;
    case F_MULT_EQ:ins_f_byte(F_MULTIPLY);break;
    case F_MOD_EQ: ins_f_byte(F_MOD); break;
    case F_DIV_EQ: ins_f_byte(F_DIVIDE); break;
    }

    if(flags & DO_POP)
    {
      ins_f_byte(F_ASSIGN_AND_POP);
      return 0;
    }else{
      ins_f_byte(F_ASSIGN);
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
	if(match_types(CDR(n)->type,array_type_string))
	{
	  if(do_docode(CDAR(n),DO_NOT_COPY)!=1)
	    fatal("Infernal compiler error (dumpar core |ver hela mattan).\n");
	  ins_f_byte(F_LTOSVAL2);
	}else{
	  ins_f_byte(F_LTOSVAL);
	  if(do_docode(CDAR(n),DO_NOT_COPY)!=1)
	    fatal("Infernal compiler error (dumpar core).\n");
	}

	ins_f_byte(CAR(n)->token);

	ins_f_byte(n->token);
	return n->token==F_ASSIGN;
      }

    default:
      if(CDR(n)->token!=F_LOCAL && CDR(n)->token!=F_GLOBAL)
	tmp1=do_docode(CDR(n),DO_LVALUE);

      if(do_docode(CAR(n),0)!=1)
	yyerror("RHS is void!");

      switch(CDR(n)->token)
      {
      case F_LOCAL:
	ins_f_byte(flags & DO_POP ? F_ASSIGN_LOCAL_AND_POP:F_ASSIGN_LOCAL);
	ins_byte(CDR(n)->u.number,A_PROGRAM);
	break;

      case F_GLOBAL:
	ins_f_byte(flags & DO_POP ? F_ASSIGN_GLOBAL_AND_POP:F_ASSIGN_GLOBAL);
	ins_byte(CDR(n)->u.number,A_PROGRAM);
	break;

      default:
	ins_f_byte(flags & DO_POP ? F_ASSIGN_AND_POP:F_ASSIGN);
	break;
      }
      return flags & DO_POP ? 0 : 1;
    }

  case F_LAND:
  case F_LOR:
    if(do_docode(CAR(n),0)!=1)
      fatal("Compiler internal error.\n");
    tmp1=do_jump(n->token,-1);
    if(do_docode(CDR(n),0)!=1)
      fatal("Compiler internal error.\n");
    set_branch(tmp1,PC);
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
      fatal("Compiler internal error.\n");
    ins_f_byte(n->token);
    return tmp1;

  case F_INC:
  case F_POST_INC:
    tmp1=do_docode(CAR(n),DO_LVALUE);
#ifdef DEBUG
    if(tmp1 != 2)
      fatal("HELP! FATAL INTERNAL COMPILER ERROR\n");
#endif

    if(flags & DO_POP)
    {
      ins_f_byte(F_INC_AND_POP);
      return 0;
    }else{
      ins_f_byte(n->token);
      return 1;
    }

  case F_DEC:
  case F_POST_DEC:
    tmp1=do_docode(CAR(n),DO_LVALUE);
#ifdef DEBUG
    if(tmp1 != 2)
      fatal("HELP! FATAL INTERNAL COMPILER ERROR\n");
#endif
    if(flags & DO_POP)
    {
      ins_f_byte(F_DEC_AND_POP);
      return 0;
    }else{
      ins_f_byte(n->token);
      return 1;
    }

  case F_FOR:
  {
    struct jump_list brk,cnt;
    INT32 *prev_switch_jumptable = current_switch_jumptable;
    current_switch_jumptable=0;

    push_break_stack(&brk);
    push_continue_stack(&cnt);
    if(CDR(n))
    {
      tmp1=do_jump(F_BRANCH,-1);
      tmp2=PC;
      if(CDR(n)) DO_CODE_BLOCK(CADR(n));
      pop_continue_stack(&cnt,PC);
      if(CDR(n)) DO_CODE_BLOCK(CDDR(n));
      set_branch(tmp1,PC);
    }else{
      tmp2=PC;
    }
    do_jump_when_non_zero(CAR(n),tmp2);
    pop_break_stack(&brk,PC);

    current_switch_jumptable = prev_switch_jumptable;
    return 0;
  }

  case ' ':
    return do_docode(CAR(n),0)+do_docode(CDR(n),DO_LVALUE);

  case F_FOREACH:
  {
    struct jump_list cnt,brk;
    INT32 *prev_switch_jumptable = current_switch_jumptable;
    current_switch_jumptable=0;

    tmp2=do_docode(CAR(n),DO_NOT_COPY);
    ins_f_byte(F_CONST0);
    tmp3=do_jump(F_BRANCH,-1);
    tmp1=PC;
    push_break_stack(&brk);
    push_continue_stack(&cnt);
    DO_CODE_BLOCK(CDR(n));
    pop_continue_stack(&cnt,PC);
    set_branch(tmp3,PC);
    do_jump(n->token,tmp1);
    pop_break_stack(&brk,PC);

    current_switch_jumptable = prev_switch_jumptable;
    return 0;
  }

  case F_INC_NEQ_LOOP:
  case F_DEC_NEQ_LOOP:
  case F_INC_LOOP:
  case F_DEC_LOOP:
  {
    struct jump_list cnt,brk;
    INT32 *prev_switch_jumptable = current_switch_jumptable;
    current_switch_jumptable=0;

    tmp2=do_docode(CAR(n),0);
    tmp3=do_jump(F_BRANCH,-1);
    tmp1=PC;
    push_break_stack(&brk);
    push_continue_stack(&cnt);
    DO_CODE_BLOCK(CDR(n));
    pop_continue_stack(&cnt,PC);
    set_branch(tmp3,PC);
    do_jump(n->token,tmp1);
    pop_break_stack(&brk,PC);

    current_switch_jumptable = prev_switch_jumptable;
    return 0;
  }

  case F_DO:
  {
    struct jump_list cnt,brk;
    INT32 *prev_switch_jumptable = current_switch_jumptable;
    current_switch_jumptable=0;

    tmp2=PC;
    push_break_stack(&brk);
    push_continue_stack(&cnt);
    DO_CODE_BLOCK(CAR(n));
    pop_continue_stack(&cnt,PC);
    do_jump_when_non_zero(CDR(n),tmp2);
    pop_break_stack(&brk,PC);

    current_switch_jumptable = prev_switch_jumptable;
    return 0;
  }

  case F_CAST:
    if(n->type==void_type_string)
    {
      DO_CODE_BLOCK(CAR(n));
      return 0;
    }

    tmp1=do_docode(CAR(n),0);
    if(!tmp1) { ins_f_byte(F_CONST0); tmp1=1; }
    if(tmp1>1) do_pop(tmp1-1);

    tmp1=store_prog_string(n->type);
    ins_f_byte_with_numerical_arg(F_STRING,tmp1);
    ins_f_byte(F_CAST);
    return 1;

  case F_APPLY:
    if(CAR(n)->token == F_CONSTANT)
    {
      if(CAR(n)->u.sval.type == T_FUNCTION)
      {
	if(CAR(n)->u.sval.subtype == -1) /* driver fun? */
	{
	  if(!CAR(n)->u.sval.u.efun->docode || 
	     !CAR(n)->u.sval.u.efun->docode(n))
	  {
	    ins_f_byte(F_MARK);
	    do_docode(CDR(n),0);
	    tmp1=store_constant(& CAR(n)->u.sval,
				!(CAR(n)->tree_info & OPT_EXTERNAL_DEPEND));
	    ins_f_byte(F_MAX_OPCODE + tmp1);
	  }
	  if(n->type == void_type_string) return 0;
	  return 1;
	}else{
	  if(CAR(n)->u.sval.u.object == &fake_object)
	  {
	    ins_f_byte(F_MARK);
	    do_docode(CDR(n),0);
	    ins_f_byte_with_numerical_arg(F_CALL_LFUN, CAR(n)->u.sval.subtype);
	    return 1;
	  }
       	}
      }

      ins_f_byte(F_MARK);
      do_docode(CDR(n),0);
      tmp1=store_constant(& CAR(n)->u.sval,
			  !(CAR(n)->tree_info & OPT_EXTERNAL_DEPEND));
      ins_f_byte(F_MAX_OPCODE + tmp1);
      
      return 1;
    }
    else if(CAR(n)->token == F_IDENTIFIER &&
	    ID_FROM_INT(& fake_program, CAR(n)->u.number)->flags & IDENTIFIER_FUNCTION)
    {
      ins_f_byte(F_MARK);
      do_docode(CDR(n),0);
      ins_f_byte_with_numerical_arg(F_CALL_LFUN, CAR(n)->u.number);
      return 1;
    }
    else
    {
      struct lpc_string *tmp;
      struct efun *fun;

      ins_f_byte(F_MARK);
      tmp=make_shared_string("call_function");
      if(!tmp) yyerror("No call_function efun.");
      fun=lookup_efun(tmp);
      if(!fun) yyerror("No call_function efun.");
      free_string(tmp);

      do_docode(CAR(n),0);
      do_docode(CDR(n),0);
      tmp1=store_constant(& fun->function, 1);
      ins_f_byte(tmp1 + F_MAX_OPCODE);
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
    struct jump_list brk;
    INT32 e,cases,*order;
    INT32 *jumptable;
    INT32 prev_switch_values_on_stack = current_switch_values_on_stack;
    INT32 prev_switch_case = current_switch_case;
    INT32 prev_switch_default = current_switch_default;
    INT32 *prev_switch_jumptable = current_switch_jumptable;

    if(do_docode(CAR(n),0)!=1)
      fatal("Internal compiler error, time to panic\n");

    push_break_stack(&brk);

    cases=count_cases(CDR(n));

    ins_f_byte(F_SWITCH);
    tmp1=PC;
    ins_short(0, A_PROGRAM);
    while(PC != (unsigned INT32)MY_ALIGN(PC))
      ins_byte(0, A_PROGRAM);
    tmp2=PC;
    current_switch_values_on_stack=0;
    current_switch_case=0;
    current_switch_default=-1;
    current_switch_jumptable=(INT32 *)xalloc(sizeof(INT32)*(cases*2+1));
    jumptable=(INT32 *)xalloc(sizeof(INT32)*(cases*2+1));

    for(e=0; e<cases*2+1; e++)
    {
      jumptable[e]=do_jump(-1,-1);
      current_switch_jumptable[e]=-1;
    }

    current_switch_jumptable[current_switch_case++]=-1;

    DO_CODE_BLOCK(CDR(n));

    f_aggregate(cases);
    sp[-1].u.array=compact_array(sp[-1].u.array);
    order=get_switch_order(sp[-1].u.array);
    
    for(e=0; e<cases-1; e++)
    {
      if(current_switch_jumptable[order[e]*2+2] != -1)
      {
	if(current_switch_jumptable[order[e]*2+2] !=
	   current_switch_jumptable[order[e+1]*2+1])
	  yyerror("Case inside range.");
      }
    }

    if(current_switch_default < 0) current_switch_default = PC;

    for(e=0;e<cases*2+1;e++)
      if(current_switch_jumptable[e]==-1)
	current_switch_jumptable[e]=current_switch_default;

    sp[-1].u.array=order_array(sp[-1].u.array,order);

    reorder((void *)(current_switch_jumptable+1),cases,sizeof(INT32)*2,order);
    free((char *)order);

    for(e=0; e<cases*2+1; e++)
      low_set_branch(jumptable[e], current_switch_jumptable[e],tmp2);

    e=store_constant(sp-1,1);
    upd_short(tmp1,e);

    pop_stack();
    free((char *)jumptable);
    free((char *)current_switch_jumptable);

    current_switch_jumptable = prev_switch_jumptable;
    current_switch_default = prev_switch_default;
    current_switch_case = prev_switch_case;
    current_switch_values_on_stack = prev_switch_values_on_stack ;

    pop_break_stack(&brk,PC);

    return 0;
  }

  case F_CASE:
  {
    if(!current_switch_jumptable)
    {
      yyerror("Case outside switch.");
    }else{
      if(!is_const(CAR(n)))
	yyerror("Case label isn't constant.");

      tmp1=eval_low(CAR(n));
      if(tmp1<1)
      {
	yyerror("Error in case label.");
	return 0;
      }
      pop_n_elems(tmp1-1);
      current_switch_values_on_stack++;
      for(tmp1=current_switch_values_on_stack; tmp1 > 1; tmp1--)
	if(is_equal(sp-tmp1, sp-1))
	  yyerror("Duplicate case.");

      current_switch_jumptable[current_switch_case++]=PC;

      if(CDR(n))
      {
	current_switch_jumptable[current_switch_case++]=PC;
	tmp1=eval_low(CDR(n));
	if(tmp1<1)
	{
	  pop_stack();
	  yyerror("Error in case label.");
	  return 0;
	}
	pop_n_elems(tmp1-1);
	current_switch_values_on_stack++;
	for(tmp1=current_switch_values_on_stack; tmp1 > 1; tmp1--)
	  if(is_equal(sp-tmp1, sp-1))
	    yyerror("Duplicate case.");
	current_switch_jumptable[current_switch_case++]=PC;
      }
      current_switch_jumptable[current_switch_case++]=-1;
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
      current_switch_default = PC;
    }
    return 0;

  case F_BREAK:
    if(!break_stack)
    {
      yyerror("Break outside loop or switch.");
    }else{
      if(current_break>=break_stack_size)
      {
	break_stack_size*=2;
	break_stack=(int *)realloc((char *)break_stack,
				   sizeof(int)*break_stack_size);
	if(!break_stack)
	  fatal("Out of memory.\n");
      }
      break_stack[current_break++]=do_jump(F_BRANCH,-1);
    }
    return 0;

  case F_CONTINUE:
    if(!continue_stack)
    {
      yyerror("continue outside loop or switch.");
    }else{
      if(current_continue>=continue_stack_size)
      {
	continue_stack_size*=2;
	continue_stack=(int *)realloc((char *)continue_stack,
				      sizeof(int)*continue_stack_size);
      }
      continue_stack[current_continue++]=do_jump(F_BRANCH,-1);
    }
    return 0;

  case F_RETURN:
    if(!CAR(n) ||
       (CAR(n)->token == F_CONSTANT && IS_ZERO(&CAR(n)->u.sval)) ||
       do_docode(CAR(n),0)<0)
    {
      ins_f_byte(F_RETURN_0);
    }else{
      ins_f_byte(F_RETURN);
    }
    return 0;

  case F_SSCANF:
    tmp1=do_docode(CAR(n),DO_NOT_COPY);
    tmp2=do_docode(CDR(n),DO_NOT_COPY | DO_LVALUE);
    ins_f_byte_with_numerical_arg(F_SSCANF,tmp1+tmp2);
    return 1;

  case F_CATCH:
  {
    struct jump_list cnt,brk;
    INT32 *prev_switch_jumptable = current_switch_jumptable;
    current_switch_jumptable=0;

    tmp1=do_jump(F_CATCH,-1);
    push_break_stack(&brk);
    push_continue_stack(&cnt);
    DO_CODE_BLOCK(CAR(n));
    pop_continue_stack(&cnt,PC);
    pop_break_stack(&brk,PC);
    ins_f_byte(F_DUMB_RETURN);
    set_branch(tmp1,PC);

    current_switch_jumptable = prev_switch_jumptable;
    return 1;
  }

  case F_LVALUE_LIST:
    return do_docode(CAR(n),DO_LVALUE)+do_docode(CDR(n),DO_LVALUE);

  case F_INDEX:
    if(flags & DO_LVALUE)
    {
      tmp1=do_docode(CAR(n), 0);
      if(do_docode(CDR(n),0) != 1)
	fatal("Internal compiler error, please report this (1).");
      return 2;
    }else{
      tmp1=do_docode(CAR(n), DO_NOT_COPY);
      if(do_docode(CDR(n),DO_NOT_COPY) != 1)
	fatal("Internal compiler error, please report this (1).");
      ins_f_byte(F_INDEX);
      if(!(flags & DO_NOT_COPY))
      {
	while(n && n->token==F_INDEX) n=CAR(n);
	if(n->token==F_CONSTANT && !(n->node_info & OPT_EXTERNAL_DEPEND))
	  ins_f_byte(F_COPY_VALUE);
      }
    }
    return tmp1;

  case F_CONSTANT:
    switch(n->u.sval.type)
    {
    case T_INT:
      ins_int(n->u.sval.u.integer);
      return 1;

    case T_FLOAT:
      ins_float(n->u.sval.u.float_number);
      return 1;

    case T_STRING:
      tmp1=store_prog_string(n->u.sval.u.string);
      ins_f_byte_with_numerical_arg(F_STRING,tmp1);
      return 1;

    case T_FUNCTION:
      if(n->u.sval.subtype!=-1)
      {
	if(n->u.sval.u.object == &fake_object)
	{
	  ins_f_byte_with_numerical_arg(F_LFUN,n->u.sval.subtype);
	  return 1;
	}
      }

    default:
      tmp1=store_constant(&(n->u.sval),!(n->tree_info & OPT_EXTERNAL_DEPEND));
      ins_f_byte_with_numerical_arg(F_CONSTANT,tmp1);
      return 1;

    case T_ARRAY:
    case T_MAPPING:
    case T_LIST:
      tmp1=store_constant(&(n->u.sval),!(n->tree_info & OPT_EXTERNAL_DEPEND));
      ins_f_byte_with_numerical_arg(F_CONSTANT,tmp1);
      
      /* copy now or later ? */
      if(!(flags & DO_NOT_COPY) && !(n->tree_info & OPT_EXTERNAL_DEPEND))
	ins_f_byte(F_COPY_VALUE);
      return 1;

    }

  case F_LOCAL:
    if(flags & DO_LVALUE)
    {
      ins_f_byte_with_numerical_arg(F_LOCAL_LVALUE,n->u.number);
      return 2;
    }else{
      ins_f_byte_with_numerical_arg(F_LOCAL,n->u.number);
      return 1;
    }

  case F_IDENTIFIER:
    if(ID_FROM_INT(& fake_program, n->u.number)->flags & IDENTIFIER_FUNCTION)
    {
      if(flags & DO_LVALUE)
      {
	yyerror("Cannot assign functions.\n");
      }else{
	ins_f_byte_with_numerical_arg(F_LFUN,n->u.number);
      }
    }else{
      if(flags & DO_LVALUE)
      {
	ins_f_byte_with_numerical_arg(F_GLOBAL_LVALUE,n->u.number);
	return 2;
      }else{
	ins_f_byte_with_numerical_arg(F_GLOBAL,n->u.number);
      }
    }
    return 1;

  case F_EFUN:
    ins_f_byte_with_numerical_arg(n->token,n->u.number);
    return 1;

  case F_VAL_LVAL:
    return do_docode(CAR(n),flags)+do_docode(CDR(n),flags | DO_LVALUE);
    
  default:
    fatal("Infernal compiler error (unknown parse-tree-token).\n");
    return 0;			/* make gcc happy */
  }
}

void do_code_block(node *n)
{
  clean_jumptable();
  DO_CODE_BLOCK(n);
}
