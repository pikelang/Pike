/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: docode.c,v 1.175 2004/10/28 20:15:08 mast Exp $
*/

#include "global.h"
RCSID("$Id: docode.c,v 1.175 2004/10/28 20:15:08 mast Exp $");
#include "las.h"
#include "program.h"
#include "pike_types.h"
#include "stralloc.h"
#include "interpret.h"
#include "constants.h"
#include "array.h"
#include "pike_macros.h"
#include "pike_error.h"
#include "pike_memory.h"
#include "svalue.h"
#include "main.h"
#include "builtin_functions.h"
#include "peep.h"
#include "docode.h"
#include "operators.h"
#include "object.h"
#include "opcodes.h"
#include "language.h"
#include "lex.h"
#include "mapping.h"
#include "multiset.h"

static int do_docode2(node *n, int flags);

typedef void (*cleanup_func)(void *);

struct cleanup_frame
{
  struct cleanup_frame *prev;
  cleanup_func cleanup;
  void *cleanup_arg;
  int stack_depth;
};

struct statement_label_name
{
  struct statement_label_name *next;
  struct pike_string *str;
  unsigned int line_number;
};

struct statement_label
{
  struct statement_label *prev;
  struct statement_label_name *name;
  /* -2 in break_label is used to flag "open" statement_label entries.
   * If an open entry is on top of the stack, it's used instead of a
   * new one. That's used to associate statement labels to the
   * following statement. */
  INT32 break_label, continue_label;
  int emit_break_label;
  int stack_depth;
  struct cleanup_frame *cleanups;
};

static struct statement_label top_statement_label_dummy =
  {0, 0, -1, -1, 0, -1, 0};
static struct statement_label *current_label = &top_statement_label_dummy;
#ifdef PIKE_DEBUG
static int current_stack_depth = -4711;
#else
static int current_stack_depth = 0;
#endif

#define PUSH_CLEANUP_FRAME(func, arg) do {				\
  struct cleanup_frame cleanup_frame__;					\
  cleanup_frame__.cleanup = (cleanup_func) (func);			\
  cleanup_frame__.cleanup_arg = (void *)(ptrdiff_t) (arg);		\
  cleanup_frame__.stack_depth = current_stack_depth;			\
  DO_IF_DEBUG(								\
    if (current_label->cleanups == (void *)(ptrdiff_t) -1)		\
      Pike_fatal("current_label points to an unused statement_label.\n");	\
  )									\
  if (current_label->break_label == -2) {				\
    DO_IF_DEBUG(							\
      if (current_label->prev->break_label == -2)			\
        Pike_fatal("Found two open statement_label entries in a row.\n");	\
    )									\
    cleanup_frame__.prev = current_label->prev->cleanups;		\
    current_label->prev->cleanups = &cleanup_frame__;			\
  }									\
  else {								\
    cleanup_frame__.prev = current_label->cleanups;			\
    current_label->cleanups = &cleanup_frame__;				\
  }

#define POP_AND_DONT_CLEANUP						\
  if (current_label->cleanups == &cleanup_frame__)			\
    current_label->cleanups = cleanup_frame__.prev;			\
  else {								\
    DO_IF_DEBUG(							\
      if (current_label->prev->cleanups != &cleanup_frame__)		\
        Pike_fatal("Cleanup frame lost from statement_label cleanup list.\n");\
    )									\
    current_label->prev->cleanups = cleanup_frame__.prev;		\
  }									\
} while (0)

#define POP_AND_DO_CLEANUP						\
  do_pop(current_stack_depth - cleanup_frame__.stack_depth);		\
  cleanup_frame__.cleanup(cleanup_frame__.cleanup_arg);			\
  POP_AND_DONT_CLEANUP

/* A block in the following sense is a region of code where:
 * o  Execution always enters at the beginning.
 * o  All stack nesting is left intact on exit (both normally and
 *    through jumps, but not through exceptions). This includes the
 *    svalue and mark stacks, and the catch block nesting.
 */
#ifdef PIKE_DEBUG
#define BLOCK_BEGIN							\
  PUSH_CLEANUP_FRAME(do_cleanup_synch_mark, 0);				\
  if (d_flag > 2) emit0(F_SYNCH_MARK);
#define BLOCK_END							\
  if (current_stack_depth != cleanup_frame__.stack_depth) {		\
    print_tree(n);							\
    Pike_fatal("Stack not in synch after block: is %d, should be %d.\n",	\
	  current_stack_depth, cleanup_frame__.stack_depth);		\
  }									\
  if (d_flag > 2) emit0(F_POP_SYNCH_MARK);				\
  POP_AND_DONT_CLEANUP
#else
#define BLOCK_BEGIN
#define BLOCK_END
#endif

#define PUSH_STATEMENT_LABEL do {					\
  struct statement_label new_label__;					\
  new_label__.prev = current_label;					\
  if (current_label->break_label != -2) {				\
    /* Only cover the current label if it's closed. */			\
    new_label__.name = 0;						\
    new_label__.break_label = new_label__.continue_label = -1;		\
    new_label__.emit_break_label = 0;					\
    new_label__.cleanups = 0;						\
    new_label__.stack_depth = current_stack_depth;			\
    current_label = &new_label__;					\
  }									\
  else {								\
    DO_IF_DEBUG(							\
      new_label__.cleanups = (void *)(ptrdiff_t) -1;			\
      new_label__.stack_depth = current_stack_depth;			\
    )									\
    current_label->stack_depth = current_stack_depth;			\
  }

#define POP_STATEMENT_LABEL						\
  current_label = new_label__.prev;					\
  DO_IF_DEBUG(								\
    if (new_label__.cleanups &&						\
	new_label__.cleanups != (void *)(ptrdiff_t) -1)			\
      Pike_fatal("Cleanup frames still left in statement_label.\n"));	\
} while (0)

struct switch_data
{
  INT32 index;
  INT32 less_label, greater_label, default_label;
  INT32 values_on_stack;
  INT32 *jumptable;
  struct pike_type *type;
};

static struct switch_data current_switch = {0, 0, 0, 0, 0, NULL, NULL};
static int in_catch=0;

void upd_int(int offset, INT32 tmp)
{
  MEMCPY(Pike_compiler->new_program->program+offset, (char *)&tmp,sizeof(tmp));
}

INT32 read_int(int offset)
{
  return EXTRACT_INT(Pike_compiler->new_program->program+offset);
}

static int label_no=0;

int alloc_label(void) { return ++label_no; }

int do_jump(int token,INT32 lbl)
{
  if(lbl==-1) lbl=alloc_label();
  emit1(token, lbl);
  return lbl;
}


#define LBLCACHESIZE 4711
#define CURRENT_INSTR ((long)instrbuf.s.len / (long)sizeof(p_instr))
#define MAX_UNWIND 100

static int lbl_cache[LBLCACHESIZE];

int do_branch(INT32 lbl)
{
  if(lbl==-1)
  {
    lbl=alloc_label();
  }else{
    INT32 last,pos=lbl_cache[lbl % LBLCACHESIZE];
    if(pos < (last=CURRENT_INSTR) &&  (CURRENT_INSTR - pos) < MAX_UNWIND)
    {
#define BUF ((p_instr *)instrbuf.s.str)
      if(BUF[pos].opcode == F_LABEL && BUF[pos].arg == lbl)
      {
	for(;pos < last;pos++)
	{
	  if(BUF[pos].opcode != F_LABEL)
	  {
	    insert_opcode2(BUF[pos].opcode,
			   BUF[pos].arg,
			   BUF[pos].arg2,
			   BUF[pos].line,
			   BUF[pos].file);
	  }
	}
      }
    }

  }
  emit1(F_BRANCH, lbl);
  return lbl;
}

void low_insert_label(int lbl)
{
  lbl_cache[ lbl % LBLCACHESIZE ] = CURRENT_INSTR;
  emit1(F_LABEL, lbl);
}

int ins_label(int lbl)
{
  if(lbl==-1) lbl=alloc_label();
  low_insert_label(lbl);
  return lbl;
}

void do_pop(int x)
{
#ifdef PIKE_DEBUG
  if (x < 0) Pike_fatal("Cannot do pop of %d args.\n", x);
#endif
  switch(x)
  {
  case 0: return;
  case 1: emit0(F_POP_VALUE); break;
  default: emit1(F_POP_N_ELEMS,x); break;
  }
  current_stack_depth -= x;
}

void do_pop_mark(void *ignored)
{
  emit0(F_POP_MARK);
}

void do_pop_to_mark(void *ignored)
{
  emit0(F_POP_TO_MARK);
}

void do_cleanup_synch_mark(void)
{
  if (d_flag > 2)
    emit0(F_CLEANUP_SYNCH_MARK);
}

void do_escape_catch(void)
{
  emit0(F_ESCAPE_CATCH);
}

#define DO_CODE_BLOCK(X) do_pop(do_docode((X),DO_NOT_COPY | DO_POP ))

int do_docode(node *n, int flags)
{
  int i;
  int stack_depth_save = current_stack_depth;
  int save_current_line=lex.current_line;
  if(!n) return 0;
  lex.current_line=n->line_number;
#ifdef PIKE_DEBUG
  if (current_stack_depth == -4711) Pike_fatal("do_docode() used outside docode().\n");
#endif
  i=do_docode2(check_node_hash(n), flags);
  current_stack_depth = stack_depth_save + i;

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
  switch(do_docode(check_node_hash(n), flags & ~DO_POP))
  {
  case 0: my_yyerror("Void expression for %s",err);
  case 1: return;
  case 2:
    Pike_fatal("Internal compiler error (%s), line %ld, file %s\n",
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
      if(t == iftrue) do_branch( label);
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
      low_insert_label(tmp);
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
    current_stack_depth--;
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
  case F_LOOP:
  case F_INC_LOOP:
  case F_DEC_LOOP:
  case F_INC_NEQ_LOOP:
  case F_DEC_NEQ_LOOP:
  case F_SWITCH:
  case '?':
    return 0;

  case F_CASE:
    return 1;
  case F_CASE_RANGE:
    return !!CAR(n)+!!CDR(n);

  default:
    ret=0;
    if(car_is_node(n)) ret += count_cases(CAR(n));
    if(cdr_is_node(n)) ret += count_cases(CDR(n));
    return ret;
  }
}


int generate_call_function(node *n)
{
  emit0(F_MARK);
  PUSH_CLEANUP_FRAME(do_pop_mark, 0);
  do_docode(CDR(n),DO_NOT_COPY);
  emit0(F_CALL_FUNCTION);
  POP_AND_DONT_CLEANUP;
  return 1;
}

static inline struct compiler_frame *find_local_frame(INT32 depth)
{
  struct compiler_frame *f=Pike_compiler->compiler_frame;
  while(--depth>=0) f=f->previous;
  return f;
}

/* Emit code for a function call to the identifier reference #id,
 * with the arguments specified by args.
 */
int do_lfun_call(int id, node *args)
{
#if 1
  struct reference *ref =
    Pike_compiler->new_program->identifier_references + id;

  /* Test description:
   *
   * * Test if we have a valid current function.
   *
   * * Quick check if id is the current function.
   *
   * * Check if id is an alternate reference to the current function.
   *
   * * Check that the function isn't varargs.
   */
  if((Pike_compiler->compiler_frame->current_function_number >= 0) &&
     ((id == Pike_compiler->compiler_frame->current_function_number) ||
      ((!ref->inherit_offset) &&
       (ref->identifier_offset ==
	Pike_compiler->new_program->
	identifier_references[Pike_compiler->compiler_frame->
			      current_function_number].identifier_offset))) &&
     (!(Pike_compiler->new_program->
	identifiers[ref->identifier_offset].identifier_flags &
	IDENTIFIER_VARARGS)))
  {
    int n=count_args(args);
    if(n == Pike_compiler->compiler_frame->num_args)
    {
      do_docode(args,0);
      if(Pike_compiler->compiler_frame->is_inline ||
	 (ref->id_flags & ID_INLINE))
      {
	/* Identifier is declared inline/local
	 * or in inlining pass.
	 */
	if ((ref->id_flags & ID_INLINE) &&
	    (!Pike_compiler->compiler_frame->is_inline)) {
	  /* Explicit local:: reference in first pass.
	   *
	   * RECUR directly to label 0.
	   *
	   * Note that we in this case don't know if we are overloaded or
	   * not, and thus can't RECUR to the recur_label.
	   */
	  do_jump(F_RECUR, 0);
	} else {
	  Pike_compiler->compiler_frame->
	    recur_label=do_jump(F_RECUR,
				Pike_compiler->compiler_frame->recur_label);
	}
      } else {
	/* Recur if not overloaded. */
	emit1(F_COND_RECUR,id);
	Pike_compiler->compiler_frame->
	  recur_label=do_jump(F_POINTER,
			      Pike_compiler->compiler_frame->recur_label);
      }
      return 1;
    }
  }
#endif

  emit0(F_MARK);
  PUSH_CLEANUP_FRAME(do_pop_mark, 0);
  do_docode(args,0);
  emit1(F_CALL_LFUN, id);
  POP_AND_DONT_CLEANUP;
  return 1;
}

/*
 * FIXME: this can be optimized, but is not really used
 * enough to be worth it yet.
 */
static void emit_apply_builtin(char *func)
{
  INT32 tmp1;
  struct pike_string *n1=make_shared_string(func);
  node *n=find_module_identifier(n1,0);
  free_string(n1);

  switch(n?n->token:0)
  {
    case F_CONSTANT:
      tmp1=store_constant(&n->u.sval,
			  !(n->tree_info & OPT_EXTERNAL_DEPEND),
			  n->name);
      if(n->u.sval.type == T_FUNCTION &&
	 n->u.sval.subtype == FUNCTION_BUILTIN)
	emit1(F_CALL_BUILTIN, DO_NOT_WARN((INT32)tmp1));
      else
	emit1(F_APPLY, DO_NOT_WARN((INT32)tmp1));
      break;

    default:
      my_yyerror("docode: Failed to make call to %s",func);
  }
  free_node(n);
}

static int do_encode_automap_arg_list(node *n,
				      int flags)
{
  int stack_depth_save = current_stack_depth;
  if(!n) return 0;
  switch(n->token)
  {
    default:
      return do_docode(n, flags);

    case F_ARG_LIST:
    {
      int ret;
      ret=do_encode_automap_arg_list(CAR(n), flags);
      current_stack_depth=stack_depth_save + ret;
      ret+=do_encode_automap_arg_list(CDR(n), flags);
      current_stack_depth=stack_depth_save + ret;
      return ret;
    }

    case F_AUTO_MAP_MARKER:
    {
      int depth=0;
      while(n->token == F_AUTO_MAP_MARKER)
      {
	n=CAR(n);
	depth++;
      }
      emit0(F_MARK);
      code_expression(n, 0, "[*]");
      emit1(F_NUMBER, depth);
      emit_apply_builtin("__builtin.automap_marker");
      return 1;
    }
  }
}

static void emit_builtin_svalue(char *func)
{
  INT32 tmp1;
  struct pike_string *n1=make_shared_string(func);
  node *n=find_module_identifier(n1,0);
  free_string(n1);

  switch(n?n->token:0)
  {
    case F_CONSTANT:
      tmp1=store_constant(&n->u.sval,
			  (!(n->tree_info & OPT_EXTERNAL_DEPEND)) &&
			  (n->u.sval.type != T_TYPE),
			  n->name);
      emit1(F_CONSTANT, DO_NOT_WARN((INT32)tmp1));
      break;

    default:
      my_yyerror("docode: Failed to make svalue for builtin %s",func);
  }
  free_node(n);
}

static int do_docode2(node *n, int flags)
{
  ptrdiff_t tmp1,tmp2,tmp3;
  int ret;

  if(!n) return 0;

  if(flags & DO_LVALUE)
  {
    switch(n->token)
    {
      default:
	yyerror("Illegal lvalue.");
	emit1(F_NUMBER,0);
	emit1(F_NUMBER,0);
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
      case F_AUTO_MAP_MARKER:
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
    ptrdiff_t x_= ((char *)&x_) + STACK_DIRECTION * (32768) -
      Pike_interpreter.stack_top ;
    x_*=STACK_DIRECTION;						
    if(x_>0)
    {
      yyerror("Too deep recursion in compiler. (please report this)");

      emit1(F_NUMBER,0);
      if(flags & DO_LVALUE)
      {
	emit1(F_NUMBER,0);
	return 2;
      }
      return 1;
    }
  }

  switch(n->token)
  {
  case F_MAGIC_INDEX:
  case F_MAGIC_SET_INDEX:
  case F_MAGIC_INDICES:
  case F_MAGIC_VALUES:
    emit2(n->token,
	  n->u.node.b->u.sval.u.integer,
	  n->u.node.a->u.sval.u.integer);
    return 1;
      
  case F_EXTERNAL:
    {
      int level = 0;
      struct program_state *state = Pike_compiler;
      while (state && (state->new_program->id != n->u.integer.a)) {
	state = state->previous;
	level++;
      }
      if (!state) {
	my_yyerror("Program parent %d lost during compiling.", n->u.integer.a);
	emit1(F_NUMBER,0);
	return 1;
      }
      if(level)
      {
	if(flags & WANT_LVALUE)
	{
	  if (n->u.integer.b == IDREF_MAGIC_THIS)
	    yyerror ("Cannot assign to this.");
	  emit2(F_EXTERNAL_LVALUE, n->u.integer.b, level);
	  return 2;
	}else{
	  emit2(F_EXTERNAL, n->u.integer.b, level);
	  return 1;
	}
      }else{
	if(flags & WANT_LVALUE)
	{
	  emit1(F_GLOBAL_LVALUE, n->u.integer.b);
	  return 2;
	}else{
	  if (n->u.integer.b == IDREF_MAGIC_THIS)
	    emit1(F_THIS_OBJECT, 0);
	  else {
	    struct identifier *id = ID_FROM_INT(state->new_program,n->u.integer.b);
	    if(IDENTIFIER_IS_FUNCTION(id->identifier_flags) &&
	       id->identifier_flags & IDENTIFIER_HAS_BODY)
	    {
	      /* Only use this opcode when it's certain that the result
	       * can't zero, i.e. when we know the function isn't just a
	       * prototype. */
	      emit1(F_LFUN, n->u.integer.b);
	    }else{
	      emit1(F_GLOBAL, n->u.integer.b);
	    }
	  }
	  return 1;
	}
      }
    }
    break;

  case F_UNDEFINED:
    yyerror("Undefined identifier");
    emit1(F_NUMBER,0);
    return 1;

  case F_PUSH_ARRAY: {
    if (current_label != &top_statement_label_dummy || current_label->cleanups) {
      /* Might not have a surrounding apply node if evaluated as a
       * constant by the optimizer. */
#ifdef PIKE_DEBUG
      if (!current_label->cleanups ||
	  (current_label->cleanups->cleanup != do_pop_mark &&
	   current_label->cleanups->cleanup != do_pop_to_mark))
	Pike_fatal("F_PUSH_ARRAY unexpected in this context.\n");
#endif
      current_label->cleanups->cleanup = do_pop_to_mark;
    }
    code_expression(CAR(n), 0, "`@");
    emit0(F_PUSH_ARRAY);
    return 0;
  }

  case '?':
  {
    INT32 *prev_switch_jumptable = current_switch.jumptable;
    int adroppings , bdroppings;
    current_switch.jumptable=0;


    if(!CDDR(n))
    {
      tmp1=alloc_label();
      do_jump_when_zero(CAR(n), DO_NOT_WARN((INT32)tmp1));
      DO_CODE_BLOCK(CADR(n));
      low_insert_label( DO_NOT_WARN((INT32)tmp1));
      current_switch.jumptable = prev_switch_jumptable;
      return 0;
    }

    if(!CADR(n))
    {
      tmp1=alloc_label();
      do_jump_when_non_zero(CAR(n), DO_NOT_WARN((INT32)tmp1));
      DO_CODE_BLOCK(CDDR(n));
      low_insert_label( DO_NOT_WARN((INT32)tmp1));
      current_switch.jumptable = prev_switch_jumptable;
      return 0;
    }

    tmp1=alloc_label();
    do_jump_when_zero(CAR(n), DO_NOT_WARN((INT32)tmp1));

    adroppings=do_docode(CADR(n), flags);
    tmp3=emit1(F_POP_N_ELEMS,0);

    /* Else */
    tmp2=do_branch(-1);
    low_insert_label( DO_NOT_WARN((INT32)tmp1));

    bdroppings=do_docode(CDDR(n), flags);
    if(adroppings < bdroppings)
    {
      do_pop(bdroppings - adroppings);
    }

    if(adroppings > bdroppings)
    {
      update_arg(DO_NOT_WARN((INT32)tmp3),
		 adroppings - bdroppings);
      adroppings=bdroppings;
    }

    low_insert_label( DO_NOT_WARN((INT32)tmp2));

    current_switch.jumptable = prev_switch_jumptable;
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
    if(CAR(n)->token == F_AUTO_MAP_MARKER ||
       CDR(n)->token == F_AUTO_MAP_MARKER)
    {
      char *opname;

      if(CAR(n)->token == F_AUTO_MAP_MARKER)
      {
	int depth=0;
	node *tmp=CAR(n);
	while(tmp->token == F_AUTO_MAP_MARKER)
	{
	  depth++;
	  tmp=CAR(tmp);
	}
	tmp1=do_docode(tmp,DO_LVALUE);
	emit0(F_MARK);
	emit0(F_MARK);
	emit0(F_LTOSVAL);
	emit1(F_NUMBER,depth);
	emit_apply_builtin("__builtin.automap_marker");
      }else{
	tmp1=do_docode(CAR(n),DO_LVALUE);
	emit0(F_LTOSVAL);
      }

      switch(n->token)
      {
	case F_ADD_EQ: opname="`+"; break;
	case F_AND_EQ: opname="`&"; break;
	case F_OR_EQ:  opname="`|"; break;
	case F_XOR_EQ: opname="`^"; break;
	case F_LSH_EQ: opname="`<<"; break;
	case F_RSH_EQ: opname="`>>"; break;
	case F_SUB_EQ: opname="`-"; break;
	case F_MULT_EQ:opname="`*"; break;
	case F_MOD_EQ: opname="`%"; break;
	case F_DIV_EQ: opname="`/"; break;
	default:
	  Pike_fatal("Really???\n");
	  opname="`make gcc happy";
      }

      emit_builtin_svalue(opname);
      emit2(F_REARRANGE,1,1);

      if(CDR(n)->token == F_AUTO_MAP)
      {
	do_encode_automap_arg_list(CDR(n), 0);
      }else{
	code_expression(CDR(n), 0, "assignment");
      }
      emit_apply_builtin("__automap__");
    }else{
      tmp1=do_docode(CAR(n),DO_LVALUE);
#ifdef PIKE_DEBUG
      if(tmp1 != 2)
	Pike_fatal("HELP! FATAL INTERNAL COMPILER ERROR (7)\n");
#endif

      if(n->token == F_ADD_EQ && (flags & DO_POP))
      {
	code_expression(CDR(n), 0, "assignment");
	emit0(F_ADD_TO_AND_POP);
	return 0;
      }
      
      if(CAR(n)->token != F_AUTO_MAP &&
	 (match_types(CAR(n)->type, array_type_string) ||
	  match_types(CAR(n)->type, string_type_string) ||
	  match_types(CAR(n)->type, mapping_type_string) ||
	  match_types(CAR(n)->type, object_type_string)))
      {
	code_expression(CDR(n), 0, "assignment");
	emit0(F_LTOSVAL2);
      }else{
	emit0(F_LTOSVAL);
	code_expression(CDR(n), 0, "assignment");
      }
      
      
      switch(n->token)
      {
	case F_ADD_EQ:
	  if(CAR(n)->type == int_type_string &&
	     CDR(n)->type == int_type_string)
	  {
	    emit0(F_ADD_INTS);
	  }
	  else if(CAR(n)->type == float_type_string &&
		  CDR(n)->type == float_type_string)
	  {
	    emit0(F_ADD_FLOATS);
	  }else{
	    emit0(F_ADD);
	  }
	  break;
	case F_AND_EQ: emit0(F_AND); break;
	case F_OR_EQ:  emit0(F_OR);  break;
	case F_XOR_EQ: emit0(F_XOR); break;
	case F_LSH_EQ: emit0(F_LSH); break;
	case F_RSH_EQ: emit0(F_RSH); break;
	case F_SUB_EQ: emit0(F_SUBTRACT); break;
	case F_MULT_EQ:emit0(F_MULTIPLY);break;
	case F_MOD_EQ: emit0(F_MOD); break;
	case F_DIV_EQ: emit0(F_DIVIDE); break;
      }
    }
    
    if(flags & DO_POP)
    {
      emit0(F_ASSIGN_AND_POP);
      return 0;
    }else{
      emit0(F_ASSIGN);
      return 1;
    }

  case F_ASSIGN:
    switch(CAR(n)->token)
    {
    case F_RANGE:
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
	if(match_types(CDR(n)->type, array_type_string) ||
	   match_types(CDR(n)->type, string_type_string) ||
	   match_types(CDR(n)->type, object_type_string) ||
	   match_types(CDR(n)->type, multiset_type_string) ||
	   match_types(CDR(n)->type, mapping_type_string))
	{
	  switch(do_docode(check_node_hash(CDAR(n)), 0))
	  {
	    case 1: emit0(F_LTOSVAL2); break;
	    case 2: emit0(F_LTOSVAL3); break;
#ifdef PIKE_DEBUG
	    default:
	      Pike_fatal("Arglebargle glop-glyf?\n");
#endif
	  }
	}else{
	  emit0(F_LTOSVAL);
	  do_docode(check_node_hash(CDAR(n)), 0);
	}

	emit0(CAR(n)->token);

	emit0(n->token);
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

	code_expression(CAR(n), 0, "RHS");
	emit1(flags & DO_POP ? F_ASSIGN_LOCAL_AND_POP:F_ASSIGN_LOCAL,
	     CDR(n)->u.integer.a );
	break;

	/* FIXME: Make special case for F_EXTERNAL */
      case F_IDENTIFIER:
	if(!IDENTIFIER_IS_VARIABLE( ID_FROM_INT(Pike_compiler->new_program, CDR(n)->u.id.number)->identifier_flags))
	{
	  yyerror("Cannot assign functions or constants.\n");
	}else{
	  code_expression(CAR(n), 0, "RHS");
	  emit1(flags & DO_POP ? F_ASSIGN_GLOBAL_AND_POP:F_ASSIGN_GLOBAL,
	       CDR(n)->u.id.number);
	}
	break;

	case F_EXTERNAL:
	  /* Check that it is in this context */
	  if(Pike_compiler ->new_program->id == CDR(n)->u.integer.a)
	  {
	    /* Check that it is a variable */
	    if(CDR(n)->u.integer.b != IDREF_MAGIC_THIS &&
	       IDENTIFIER_IS_VARIABLE( ID_FROM_INT(Pike_compiler->new_program, CDR(n)->u.integer.b)->identifier_flags))
	    {
	      code_expression(CAR(n), 0, "RHS");
	      emit1(flags & DO_POP ? F_ASSIGN_GLOBAL_AND_POP:F_ASSIGN_GLOBAL,
		    CDR(n)->u.integer.b);
	      break;
	    }
	  }
	  /* fall through */

      default:
      normal_assign:
	tmp1=do_docode(CDR(n),DO_LVALUE);
	if(do_docode(CAR(n),0)!=1) yyerror("RHS is void!");
	emit0(flags & DO_POP ? F_ASSIGN_AND_POP:F_ASSIGN);
	break;
      }
      return flags & DO_POP ? 0 : 1;
    }

  case F_LAND:
  case F_LOR:
    tmp1=alloc_label();
    if(flags & DO_POP)
    {
      do_cond_jump(CAR(n), DO_NOT_WARN((INT32)tmp1), n->token == F_LOR, DO_POP);
      DO_CODE_BLOCK(CDR(n));
      low_insert_label( DO_NOT_WARN((INT32)tmp1));
      return 0;
    }else{
      do_cond_jump(CAR(n), DO_NOT_WARN((INT32)tmp1), n->token == F_LOR, 0);
      code_expression(CDR(n), flags, n->token == F_LOR ? "||" : "&&");
      low_insert_label( DO_NOT_WARN((INT32)tmp1));
      return 1;
    }

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
    Pike_fatal("Optimizer error.\n");

  case F_RANGE:
    tmp1=do_docode(CAR(n),DO_NOT_COPY_TOPLEVEL);
    if(do_docode(CDR(n),DO_NOT_COPY)!=2)
      Pike_fatal("Compiler internal error (at %ld).\n",(long)lex.current_line);
    emit0(n->token);
    return DO_NOT_WARN((INT32)tmp1);

  case F_INC:
  case F_POST_INC:
    if(CAR(n)->token == F_AUTO_MAP_MARKER)
    {
      int depth=0;
      int ret=0;
      node *tmp=CAR(n);
      while(tmp->token == F_AUTO_MAP_MARKER)
      {
	depth++;
	tmp=CAR(tmp);
      }

      tmp1=do_docode(tmp,DO_LVALUE);
      if(n->token == F_POST_INC)
      {
	emit0(F_LTOSVAL);
	emit2(F_REARRANGE,1,2);
	ret++;
	flags|=DO_POP;
      }

#ifdef PIKE_DEBUG
      if(tmp1 != 2)
	Pike_fatal("HELP! FATAL INTERNAL COMPILER ERROR (1)\n");
#endif

      emit0(F_MARK);
      emit0(F_MARK);
      emit0(F_LTOSVAL);
      emit1(F_NUMBER, depth);
      emit_apply_builtin("__builtin.automap_marker");
      emit_builtin_svalue("`+");
      emit2(F_REARRANGE,1,1);
      emit1(F_NUMBER, 1);
      emit_apply_builtin("__automap__");

      if(flags & DO_POP)
      {
	emit0(F_ASSIGN_AND_POP);
      }else{
	emit0(F_ASSIGN);
	ret++;
      }
      return ret;
    }else{
      tmp1=do_docode(CAR(n),DO_LVALUE);
#ifdef PIKE_DEBUG
      if(tmp1 != 2)
	Pike_fatal("HELP! FATAL INTERNAL COMPILER ERROR (1)\n");
#endif

      if(flags & DO_POP)
      {
	emit0(F_INC_AND_POP);
	return 0;
      }else{
	emit0(n->token);
	return 1;
      }
    }

  case F_DEC:
  case F_POST_DEC:
    if(CAR(n)->token == F_AUTO_MAP_MARKER)
    {
      int depth=0;
      int ret=0;
      node *tmp=CAR(n);
      while(tmp->token == F_AUTO_MAP_MARKER)
      {
	depth++;
	tmp=CAR(tmp);
      }

      tmp1=do_docode(tmp,DO_LVALUE);
      if(n->token == F_POST_DEC)
      {
	emit0(F_LTOSVAL);
	emit2(F_REARRANGE,1,2);
	ret++;
	flags|=DO_POP;
      }

#ifdef PIKE_DEBUG
      if(tmp1 != 2)
	Pike_fatal("HELP! FATAL INTERNAL COMPILER ERROR (1)\n");
#endif

      emit0(F_MARK);
      emit0(F_MARK);
      emit0(F_LTOSVAL);
      emit1(F_NUMBER, depth);
      emit_apply_builtin("__builtin.automap_marker");
      emit_builtin_svalue("`-");
      emit2(F_REARRANGE,1,1);
      emit1(F_NUMBER, 1);
      emit_apply_builtin("__automap__");

      if(flags & DO_POP)
      {
	emit0(F_ASSIGN_AND_POP);
      }else{
	emit0(F_ASSIGN);
	ret++;
      }
      return ret;
    }else{
      tmp1=do_docode(CAR(n),DO_LVALUE);
#ifdef PIKE_DEBUG
      if(tmp1 != 2)
	Pike_fatal("HELP! FATAL INTERNAL COMPILER ERROR (2)\n");
#endif
      if(flags & DO_POP)
      {
	emit0(F_DEC_AND_POP);
	return 0;
      }else{
	emit0(n->token);
	return 1;
      }
    }

  case F_FOR:
  {
    INT32 *prev_switch_jumptable = current_switch.jumptable;
    BLOCK_BEGIN;
    PUSH_STATEMENT_LABEL;

    current_switch.jumptable=0;
    current_label->break_label=alloc_label();
    current_label->continue_label=alloc_label();

    if(CDR(n))
    {
      do_jump_when_zero(CAR(n),current_label->break_label);
      tmp2=ins_label(-1);
      DO_CODE_BLOCK(CADR(n));
      ins_label(current_label->continue_label);
      DO_CODE_BLOCK(CDDR(n));
    }else{
      tmp2=ins_label(-1);
    }
    do_jump_when_non_zero(CAR(n), DO_NOT_WARN((INT32)tmp2));
    ins_label(current_label->break_label);

    current_switch.jumptable = prev_switch_jumptable;
    POP_STATEMENT_LABEL;
    BLOCK_END;
    return 0;
  }

  case ' ':
    ret = do_docode(CAR(n),0);
    return ret + do_docode(CDR(n),DO_LVALUE);

  case F_FOREACH:
  {
    node *arr;
    INT32 *prev_switch_jumptable = current_switch.jumptable;
    arr=CAR(n);

    if(CDR(arr) && CDR(arr)->token == ':')
    {
      BLOCK_BEGIN;
      /* New-style */
      tmp1=do_docode(CAR(arr), DO_NOT_COPY_TOPLEVEL);
      emit0(F_MAKE_ITERATOR);

      if(CADR(arr))
      {
	do_docode(CADR(arr), DO_LVALUE);
      }else{
	emit0(F_CONST0);
	emit0(F_CONST0);
	current_stack_depth+=2;
      }

      if(CDDR(arr))
      {
	do_docode(CDDR(arr), DO_LVALUE);
      }else{
	emit0(F_CONST0);
	emit0(F_CONST0);
	current_stack_depth+=2;
      }

      PUSH_CLEANUP_FRAME(do_pop, 5);

      PUSH_STATEMENT_LABEL;
      current_switch.jumptable=0;
      current_label->break_label=alloc_label();
      current_label->continue_label=alloc_label();

      /* Doubt it's necessary to use a label separate from
       * current_label->break_label, but I'm playing safe. /mast */
      tmp3 = alloc_label();
      do_jump(F_FOREACH_START, DO_NOT_WARN((INT32) tmp3));
      tmp1=ins_label(-1);
      DO_CODE_BLOCK(CDR(n));
      ins_label(current_label->continue_label);
      do_jump(F_FOREACH_LOOP, DO_NOT_WARN((INT32)tmp1));
      ins_label(current_label->break_label);
      low_insert_label( DO_NOT_WARN((INT32)tmp3));

      current_switch.jumptable = prev_switch_jumptable;
      POP_STATEMENT_LABEL;
      POP_AND_DO_CLEANUP;
      BLOCK_END;
      return 0;
    }
    

    BLOCK_BEGIN;

#if 0
    /* This is buggy and never gets activated. It's fixed in 7.7. /mast */
    if(CAR(arr) && CAR(arr)->token==F_RANGE)
    {
      node **a1=my_get_arg(&_CAR(arr),0);
      node **a2=my_get_arg(&_CAR(arr),1);
      if(a1 && a2 && a2[0]->token==F_CONSTANT &&
	 a2[0]->u.sval.type==T_INT &&
	 a2[0]->u.sval.u.integer==MAX_INT_TYPE &&
	a1[0]->type == int_type_string)
      {
	/* Optimize foreach(x[start..],y). */
	do_docode(CAR(arr),DO_NOT_COPY_TOPLEVEL);
	do_docode(*a1,DO_NOT_COPY);
	goto foreach_arg_pushed;
      }
    }
#endif
    do_docode(CAR(n),DO_NOT_COPY);
    emit0(F_CONST0);
    current_stack_depth++;
  foreach_arg_pushed:
    PUSH_CLEANUP_FRAME(do_pop, 4);

    PUSH_STATEMENT_LABEL;
    current_switch.jumptable=0;
    current_label->break_label=alloc_label();
    current_label->continue_label=alloc_label();

    tmp3=do_branch(-1);
    tmp1=ins_label(-1);
    DO_CODE_BLOCK(CDR(n));
    ins_label(current_label->continue_label);
    low_insert_label( DO_NOT_WARN((INT32)tmp3));
    do_jump(n->token, DO_NOT_WARN((INT32)tmp1));
    ins_label(current_label->break_label);

    current_switch.jumptable = prev_switch_jumptable;
    POP_STATEMENT_LABEL;
    POP_AND_DO_CLEANUP;
    BLOCK_END;
    return 0;
  }

  case F_INC_NEQ_LOOP:
  case F_DEC_NEQ_LOOP:
  case F_INC_LOOP:
  case F_DEC_LOOP:
  {
    INT32 *prev_switch_jumptable = current_switch.jumptable;
    BLOCK_BEGIN;

    do_docode(CAR(n),0);
    PUSH_CLEANUP_FRAME(do_pop, 3);

    PUSH_STATEMENT_LABEL;
    current_switch.jumptable=0;
    current_label->break_label=alloc_label();
    current_label->continue_label=alloc_label();
    tmp3=do_branch(-1);
    tmp1=ins_label(-1);

    DO_CODE_BLOCK(CDR(n));
    ins_label(current_label->continue_label);
    low_insert_label( DO_NOT_WARN((INT32)tmp3));
    do_jump(n->token, DO_NOT_WARN((INT32)tmp1));
    ins_label(current_label->break_label);

    current_switch.jumptable = prev_switch_jumptable;
    POP_STATEMENT_LABEL;
    POP_AND_DO_CLEANUP;
    BLOCK_END;
    return 0;
  }

  case F_LOOP:
  {
    /* FIXME: No support for break or continue. */
    PUSH_STATEMENT_LABEL;
    tmp1 = do_docode(CAR(n), 0);
    if (tmp1 > 0) {
      do_pop(tmp1-1);
      tmp2 = do_branch(-1);
      tmp3 = ins_label(-1);
      DO_CODE_BLOCK(CDR(n));
      ins_label(tmp2);
      emit1(F_LOOP, tmp3);
    }
    POP_STATEMENT_LABEL;
    return 0;
  }

  case F_DO:
  {
    INT32 *prev_switch_jumptable = current_switch.jumptable;
    BLOCK_BEGIN;
    PUSH_STATEMENT_LABEL;

    current_switch.jumptable=0;
    current_label->break_label=alloc_label();
    current_label->continue_label=alloc_label();

    tmp2=ins_label(-1);
    DO_CODE_BLOCK(CAR(n));
    ins_label(current_label->continue_label);
    do_jump_when_non_zero(CDR(n), DO_NOT_WARN((INT32)tmp2));
    ins_label(current_label->break_label);

    current_switch.jumptable = prev_switch_jumptable;
    POP_STATEMENT_LABEL;
    BLOCK_END;
    return 0;
  }

  case F_POP_VALUE:
    {
      BLOCK_BEGIN;
      DO_CODE_BLOCK(CAR(n));
      BLOCK_END;
      return 0;
    }

  case F_CAST:
    if(n->type==void_type_string)
    {
      DO_CODE_BLOCK(CAR(n));
      return 0;
    } else if (n->type == int_type_string) {
      tmp1 = do_docode(CAR(n), 0);
      if(!tmp1)
	emit0(F_CONST0);
      else {
	if(tmp1>1)
	  do_pop(DO_NOT_WARN((INT32)(tmp1-1)));
	emit0(F_CAST_TO_INT);
      }
      return 1;
    } else if (n->type == string_type_string) {
      tmp1 = do_docode(CAR(n), 0);
      if(!tmp1)
	emit0(F_CONST0);
      else if(tmp1>1)
	do_pop(DO_NOT_WARN((INT32)(tmp1-1)));
      emit0(F_CAST_TO_STRING);
      return 1;
    } else if (compile_type_to_runtime_type(n->type) == PIKE_T_MIXED) {
      tmp1 = do_docode(CAR(n), 0);
      if(!tmp1) 
	emit0(F_CONST0);
      else if(tmp1>1)
	do_pop(DO_NOT_WARN((INT32)(tmp1-1)));
      return 1;
    }
    {
      struct svalue sv;
      sv.type = T_TYPE;
      sv.subtype = 0;
      sv.u.type = n->type;
      tmp1 = store_constant(&sv, 0, n->name);
      emit1(F_CONSTANT, DO_NOT_WARN((INT32)tmp1));
    }

    tmp1=do_docode(CAR(n),0);
    if(!tmp1) { emit0(F_CONST0); tmp1=1; }
    if(tmp1>1) do_pop(DO_NOT_WARN((INT32)(tmp1-1)));

    emit0(F_CAST);
    return 1;

  case F_SOFT_CAST:
    if (runtime_options & RUNTIME_CHECK_TYPES) {
      {
	struct svalue sv;
	sv.type = T_TYPE;
	sv.subtype = 0;
	sv.u.type = n->type;
	tmp1 = store_constant(&sv, 0, n->name);
	emit1(F_CONSTANT, DO_NOT_WARN((INT32)tmp1));
      }
      tmp1 = do_docode(CAR(n), 0);
      if (!tmp1) { emit0(F_CONST0); tmp1 = 1; }
      if (tmp1 > 1) do_pop(DO_NOT_WARN((INT32)(tmp1 - 1)));
      emit0(F_SOFT_CAST);
      return 1;
    }
    tmp1 = do_docode(CAR(n), flags);
    if (tmp1 > 1) do_pop(DO_NOT_WARN((INT32)(tmp1 - 1)));
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
	    if(count_args(CDR(n))==1)
	    {
	      do_docode(CDR(n),0);
	      tmp1=store_constant(& CAR(n)->u.sval,
				  !(CAR(n)->tree_info & OPT_EXTERNAL_DEPEND),
				  CAR(n)->name);
	      emit1(F_CALL_BUILTIN1, DO_NOT_WARN((INT32)tmp1));
	    }else{
	      emit0(F_MARK);
	      PUSH_CLEANUP_FRAME(do_pop_mark, 0);
	      do_docode(CDR(n),0);
	      tmp1=store_constant(& CAR(n)->u.sval,
				  !(CAR(n)->tree_info & OPT_EXTERNAL_DEPEND),
				  CAR(n)->name);
	      emit1(F_CALL_BUILTIN, DO_NOT_WARN((INT32)tmp1));
	      POP_AND_DONT_CLEANUP;
	    }
	  }
	  if(n->type == void_type_string)
	    return 0;

	  return 1;
	}else{
	  if(CAR(n)->u.sval.u.object == Pike_compiler->fake_object)
	    return do_lfun_call(CAR(n)->u.sval.subtype, CDR(n));
       	}
      }

      emit0(F_MARK);
      PUSH_CLEANUP_FRAME(do_pop_mark, 0);
      do_docode(CDR(n),0);
      tmp1=store_constant(& CAR(n)->u.sval,
			  !(CAR(n)->tree_info & OPT_EXTERNAL_DEPEND),
			  CAR(n)->name);
      emit1(F_APPLY, DO_NOT_WARN((INT32)tmp1));
      POP_AND_DONT_CLEANUP;
      
      return 1;
    }
    else if(CAR(n)->token == F_IDENTIFIER)
    {
      return do_lfun_call(CAR(n)->u.id.number, CDR(n));
    }
    else if(CAR(n)->token == F_EXTERNAL &&
	    CAR(n)->u.integer.a == Pike_compiler->new_program->id &&
	    CAR(n)->u.integer.b != IDREF_MAGIC_THIS)
    {
      return do_lfun_call(CAR(n)->u.integer.b, CDR(n));
    }
    else if(CAR(n)->token == F_ARROW)
    {
      emit0(F_MARK);
      PUSH_CLEANUP_FRAME(do_pop_mark, 0);
      do_docode(CAAR(n),0); /* object */
      do_docode(CDR(n),0); /* args */
      emit1(F_CALL_OTHER, store_prog_string(CDAR(n)->u.sval.u.string));
      POP_AND_DONT_CLEANUP;
      return 1;
    }
    else
    {
      struct pike_string *tmp;
      node *foo;

      emit0(F_MARK);
      PUSH_CLEANUP_FRAME(do_pop_mark, 0);
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
	  emit0(F_CALL_FUNCTION);
	}else{
	  /* We might want to put "predef::"+foo->name here /Hubbe */
	  tmp1=store_constant(& foo->u.sval, 1, foo->name);
	  emit1(F_APPLY, DO_NOT_WARN((INT32)tmp1));
	}
      }
      free_node(foo);
      POP_AND_DONT_CLEANUP;
      return 1;
    }

  case F_ARG_LIST:
  case F_COMMA_EXPR:
    {
      node *root = n;
      node *parent = n->parent;

      /* Avoid a bit of recursion by traversing the graph... */
      n->parent = NULL;
      tmp1 = 0;
    next_car:
      while (CAR(n) && 
	     ((CAR(n)->token == F_ARG_LIST) ||
	      (CAR(n)->token == F_COMMA_EXPR))) {
	CAR(n)->parent = n;
	n = CAR(n);
      }
      /* CAR(n) is not F_ARG_LIST or F_COMMA_EXPR */
      tmp1 += do_docode(CAR(n), flags & ~WANT_LVALUE);
      
      do {
	if (CDR(n)) {
	  if ((CDR(n)->token == F_ARG_LIST) ||
	      (CDR(n)->token == F_COMMA_EXPR)) {
	    /* Note: Parent points to the closest preceding CAR node
	     *       on the way to the root.
	     */
	    CDR(n)->parent = n->parent;
	    n = CDR(n);
	    goto next_car;
	  }
	  /* CDR(n) is not F_ARG_LIST or F_COMMA_EXPR */
	  if (n->parent) {
	    tmp1 += do_docode(CDR(n), flags & ~WANT_LVALUE);
	  } else {
	    tmp1 += do_docode(CDR(n), flags);
	  }
	}
	/* Retrace */
	/* Note: n->parent is always a visited CAR node on the
	 *       way to the root.
	 */
	n = n->parent;
      } while (n);

      /* Restore root->parent. */
      root->parent = parent;
    }
    return DO_NOT_WARN((INT32)tmp1);


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
    struct switch_data prev_switch = current_switch;
#ifdef PIKE_DEBUG
    struct svalue *save_sp=Pike_sp;
#endif
    BLOCK_BEGIN;
    PUSH_STATEMENT_LABEL;

    if(do_docode(CAR(n),0)!=1)
      Pike_fatal("Internal compiler error, time to panic\n");

    if (!(CAR(n) && (current_switch.type = CAR(n)->type))) {
      current_switch.type = mixed_type_string;
    }

    current_label->break_label=alloc_label();

    cases=count_cases(CDR(n));

    tmp1=emit1(F_SWITCH,0);
    current_stack_depth--;
    emit1(F_ALIGN,sizeof(INT32));

    current_switch.values_on_stack=0;
    current_switch.index=2;
    current_switch.less_label=-1;
    current_switch.greater_label=-1;
    current_switch.default_label=-1;
    current_switch.jumptable=(INT32 *)xalloc(sizeof(INT32)*(cases*2+2));
    jumptable=(INT32 *)xalloc(sizeof(INT32)*(cases*2+2));

    for(e=1; e<cases*2+2; e++)
    {
      jumptable[e] = DO_NOT_WARN((INT32)emit1(F_POINTER, 0));
      current_switch.jumptable[e]=-1;
    }
    emit0(F_NOTREACHED);

    DO_CODE_BLOCK(CDR(n));

#ifdef PIKE_DEBUG
    if(Pike_sp-save_sp != cases)
      Pike_fatal("Count cases is wrong!\n");
#endif

    f_aggregate(cases);

    /* FIXME: get_switch_order might possibly be able to
     * throw errors, add a catch around this! -Hubbe
     */
    order=get_switch_order(Pike_sp[-1].u.array);

    if (!Pike_compiler->num_parse_error) {
      /* Check for cases inside a range */
      if (cases &&
	  ((current_switch.less_label >= 0 &&
	    current_switch.jumptable[order[0]*2+2] !=
	    current_switch.less_label) ||
	   (current_switch.greater_label >= 0 &&
	    current_switch.jumptable[order[cases-1]*2+2] !=
	    current_switch.greater_label)))
	yyerror("Case inside range.");
      for(e=0; e<cases-1; e++)
      {
	if(order[e] < cases-1)
	{
	  int o1=order[e]*2+2;
	  if(current_switch.jumptable[o1]==current_switch.jumptable[o1+1] &&
	     current_switch.jumptable[o1]==current_switch.jumptable[o1+2])
	  {
	    if(order[e]+1 != order[e+1])
	      yyerror("Case inside range.");
	    e++;
	  }
	}
      }
    }

    order_array(Pike_sp[-1].u.array,order);

    reorder((void *)(current_switch.jumptable+2),cases,sizeof(INT32)*2,order);
    free((char *)order);

    current_switch.jumptable[1] = current_switch.less_label;
    current_switch.jumptable[current_switch.index - 1] = current_switch.greater_label;

    if(current_switch.default_label < 0)
      current_switch.default_label = ins_label(-1);

    for(e=1;e<cases*2+2;e++)
      if(current_switch.jumptable[e]==-1)
	current_switch.jumptable[e]=current_switch.default_label;

    for(e=1; e<cases*2+2; e++)
      update_arg(jumptable[e], current_switch.jumptable[e]);

    update_arg(DO_NOT_WARN((INT32)tmp1),
	       store_constant(Pike_sp-1,1,0));

    pop_stack();
    free((char *)jumptable);
    free((char *)current_switch.jumptable);

    current_switch = prev_switch;

    low_insert_label( current_label->break_label);

    POP_STATEMENT_LABEL;
    BLOCK_END;
#ifdef PIKE_DEBUG
    if(Pike_interpreter.recoveries && Pike_sp-Pike_interpreter.evaluator_stack < Pike_interpreter.recoveries->stack_pointer)
      Pike_fatal("Stack error after F_SWITCH (underflow)\n");
#endif
    return 0;
  }

  case F_CASE:
  case F_CASE_RANGE:
  {
    if(!current_switch.jumptable)
    {
      yyerror("Case outside switch.");
    }else{
      INT32 label = ins_label(-1);
      int i;

      for (i = 0; i < 2; i++) {
	node *case_val = i == 0 ? CAR(n) : CDR(n);

	if (case_val) {
	  if(!is_const(case_val))
	    yyerror("Case label isn't constant.");

	  if (case_val->type && !TEST_COMPAT(0,6)) {
	    if (!pike_types_le(case_val->type, current_switch.type)) {
	      if (!match_types(case_val->type, current_switch.type)) {
		yytype_error("Type mismatch in case.",
			     current_switch.type, case_val->type, 0);
	      } else if (lex.pragmas & ID_STRICT_TYPES) {
		yytype_error("Type mismatch in case.",
			     current_switch.type, case_val->type, YYTE_IS_WARNING);
	      }
	    }
	  }

	  if (!Pike_compiler->num_parse_error) {
	    tmp1=eval_low(case_val,1);
	    if(tmp1<1)
	    {
	      yyerror("Error in case label.");
	      push_int(0);
	      tmp1=1;
	    }
	    pop_n_elems(tmp1-1);
	    current_switch.values_on_stack++;
	    for(tmp1=current_switch.values_on_stack; tmp1 > 1; tmp1--)
	      if(is_equal(Pike_sp-tmp1, Pike_sp-1))
		yyerror("Duplicate case label.");
	  } else {
	    push_int(0);
	    current_switch.values_on_stack++;
	  }
	}
      }

      if (n->token == F_CASE) {
	current_switch.jumptable[current_switch.index++] = label;
	current_switch.jumptable[current_switch.index++] = -1;
      }
      else {
	if (!CAR(n)) current_switch.less_label = label;
	if (!CDR(n)) current_switch.greater_label = label;
	if (CAR(n) && CDR(n)) {
	  current_switch.jumptable[current_switch.index++] = label;
	  current_switch.jumptable[current_switch.index++] = label;
	  current_switch.jumptable[current_switch.index++] = label;
	  current_switch.jumptable[current_switch.index++] = -1;
	}
	else {
	  current_switch.jumptable[current_switch.index++] = label;
	  current_switch.jumptable[current_switch.index++] = -1;
	}
      }
    }
    return 0;
  }

  case F_DEFAULT:
    if(!current_switch.jumptable)
    {
      yyerror("Default outside switch.");
    }else if(current_switch.default_label!=-1){
      yyerror("Duplicate switch default.");
    }else{
      current_switch.default_label = ins_label(-1);
    }
    return 0;

  case F_BREAK:
  case F_CONTINUE: {
    struct statement_label *label, *p;

    if (CAR(n)) {
      struct pike_string *name = CAR(n)->u.sval.u.string;
      struct statement_label_name *lbl_name;
      for (label = current_label; label; label = label->prev)
	for (lbl_name = label->name; lbl_name; lbl_name = lbl_name->next)
	  if (lbl_name->str == name)
	    goto label_found_1;
      my_yyerror("No surrounding statement labeled '%s'.", name->str);
      return 0;

    label_found_1:
      if (n->token == F_CONTINUE && label->continue_label < 0) {
	my_yyerror("Cannot continue the non-loop statement on line %d.",
		   lbl_name->line_number);
	return 0;
      }
    }

    else {
      if (n->token == F_BREAK) {
	for (label = current_label; label; label = label->prev)
	  if (label->break_label >= 0 && !label->emit_break_label)
	    goto label_found_2;
	yyerror("Break outside loop or switch.");
	return 0;
      }
      else {
	for (label = current_label; label; label = label->prev)
	  if (label->continue_label >= 0)
	    goto label_found_2;
	yyerror("Continue outside loop.");
	return 0;
      }
    label_found_2: ;
    }

    for (p = current_label; 1; p = p->prev) {
      struct cleanup_frame *q;
      for (q = p->cleanups; q; q = q->prev) {
	do_pop(current_stack_depth - q->stack_depth);
	q->cleanup(q->cleanup_arg);
      }
      do_pop(current_stack_depth - p->stack_depth);
      if (p == label) break;
    }

    if (n->token == F_BREAK) {
      if (label->break_label < 0) label->emit_break_label = 1;
      label->break_label = do_branch(label->break_label);
    }
    else
      do_branch(label->continue_label);

    return 0;
  }

  case F_NORMAL_STMT_LABEL:
  case F_CUSTOM_STMT_LABEL: {
    struct statement_label *label;
    struct statement_label_name name;
    BLOCK_BEGIN;
    PUSH_STATEMENT_LABEL;
    name.str = CAR(n)->u.sval.u.string;
    name.line_number = n->line_number;

    for (label = current_label; label; label = label->prev) {
      struct statement_label_name *lbl_name;
      for (lbl_name = label->name; lbl_name; lbl_name = lbl_name->next)
	if (lbl_name->str == name.str) {
	  INT32 save_line = lex.current_line;
	  lex.current_line = name.line_number;
	  my_yyerror("Duplicate nested labels, previous one on line %d.",
		     lbl_name->line_number);
	  lex.current_line = save_line;
	  goto label_check_done;
	}
    }
  label_check_done:

    name.next = current_label->name;
    current_label->name = &name;

    if (!name.next) {
      if (n->token == F_CUSTOM_STMT_LABEL)
	/* The statement we precede has custom label handling; leave
	 * the statement_label "open" so the statement will use it
	 * instead of covering it. */
	current_label->break_label = -2;
      else
	current_label->break_label = -1;
    }
    DO_CODE_BLOCK(CDR(n));
    if (!name.next && current_label->emit_break_label)
      low_insert_label(current_label->break_label);
    POP_STATEMENT_LABEL;
    BLOCK_END;
    return 0;
  }

  case F_RETURN:
    do_docode(CAR(n),0);
    emit0(in_catch ? F_VOLATILE_RETURN : F_RETURN);
    return 0;

  case F_SSCANF:
    tmp1=do_docode(CAR(n),DO_NOT_COPY);
    tmp2=do_docode(CDR(n),DO_NOT_COPY | DO_LVALUE);
    emit1(F_SSCANF, DO_NOT_WARN((INT32)(tmp1+tmp2)));
    return 1;

  case F_CATCH: {
    INT32 *prev_switch_jumptable = current_switch.jumptable;

    tmp1=do_jump(F_CATCH,-1);
    PUSH_CLEANUP_FRAME(do_escape_catch, 0);

    /* Entry point called by eval_instruction() via o_catch(). */
    emit0(F_ENTRY);

    PUSH_STATEMENT_LABEL;
    current_switch.jumptable=0;
    current_label->break_label=alloc_label();
    if (TEST_COMPAT(7,0))
      current_label->continue_label = current_label->break_label;

    in_catch++;
    DO_CODE_BLOCK(CAR(n));
    in_catch--;

    ins_label(current_label->break_label);
    emit0(F_EXIT_CATCH);
    POP_STATEMENT_LABEL;
    current_switch.jumptable = prev_switch_jumptable;

    ins_label(DO_NOT_WARN((INT32)tmp1));
    current_stack_depth++;

    POP_AND_DONT_CLEANUP;
    return 1;
  }

  case F_LVALUE_LIST:
    ret = do_docode(CAR(n),DO_LVALUE);
    return ret + do_docode(CDR(n),DO_LVALUE);

    case F_ARRAY_LVALUE:
      tmp1=do_docode(CAR(n),DO_LVALUE);
#ifdef PIKE_DEBUG
      if(tmp1 & 1)
	Pike_fatal("Very internal compiler error.\n");
#endif
      emit1(F_ARRAY_LVALUE, DO_NOT_WARN((INT32)(tmp1>>1)));
      return 2;

  case F_ARROW:
    if(CDR(n)->token != F_CONSTANT || CDR(n)->u.sval.type!=T_STRING)
      Pike_fatal("Bugg in F_ARROW, index not string.\n");
    if(flags & WANT_LVALUE)
    {
      /* FIXME!!!! ??? I wonder what needs fixing... /Hubbe */
      tmp1=do_docode(CAR(n), 0);
      emit1(F_ARROW_STRING, store_prog_string(CDR(n)->u.sval.u.string));
      return 2;
    }else{
      tmp1 = do_docode(CAR(n), DO_NOT_COPY);
      if ((tmp2 = lfun_lookup_id(CDR(n)->u.sval.u.string)) != -1) {
	emit1(F_LOOKUP_LFUN, tmp2);
      } else {
	emit1(F_ARROW, store_prog_string(CDR(n)->u.sval.u.string));
      }
      if(!(flags & DO_NOT_COPY))
      {
	while(n && (n->token==F_INDEX || n->token==F_ARROW)) n=CAR(n);
	if(n->token==F_CONSTANT && !(n->node_info & OPT_EXTERNAL_DEPEND))
	  emit0(F_COPY_VALUE);
      }
    }
    return DO_NOT_WARN((INT32)tmp1);

  case F_INDEX:
    if(flags & WANT_LVALUE)
    {
      int mklval=CAR(n) && match_types(CAR(n)->type, string_type_string);
      tmp1 = do_docode(CAR(n),
		       mklval ? DO_LVALUE_IF_POSSIBLE : 0);
      if(tmp1==2)
      {
#ifdef PIKE_DEBUG
	if(!mklval)
	  Pike_fatal("Unwanted lvalue!\n");
#endif
	emit0(F_INDIRECT);
      }
      
      if(do_docode(CDR(n),0) != 1)
	Pike_fatal("Internal compiler error, please report this (1).\n");
      if(CDR(n)->token != F_CONSTANT &&
	match_types(CDR(n)->type, string_type_string))
	emit0(F_CLEAR_STRING_SUBTYPE);
      return 2;
    }else{
      tmp1=do_docode(CAR(n), DO_NOT_COPY);

      code_expression(CDR(n), DO_NOT_COPY, "index");
      if(CDR(n)->token != F_CONSTANT &&
	match_types(CDR(n)->type, string_type_string))
	emit0(F_CLEAR_STRING_SUBTYPE);

      emit0(F_INDEX);

      if(!(flags & DO_NOT_COPY))
      {
	while(n && (n->token==F_INDEX || n->token==F_ARROW)) n=CAR(n);
	if(n->token==F_CONSTANT && !(n->node_info & OPT_EXTERNAL_DEPEND))
	  emit0(F_COPY_VALUE);
      }
    }
    return DO_NOT_WARN((INT32)tmp1);

  case F_CONSTANT:
    switch(n->u.sval.type)
    {
    case T_INT:
      if(!n->u.sval.u.integer && n->u.sval.subtype==NUMBER_UNDEFINED)
      {
	emit0(F_UNDEFINED);
      }else{
#if SIZEOF_INT_TYPE > 4
	INT_TYPE i=n->u.sval.u.integer;
	if (i != (INT32)i)
	{
	   unsigned INT_TYPE ip=(unsigned INT_TYPE)i;
	   INT32 a,b;
	   a=(INT32)(ip>>32);
	   b=(INT32)(ip&0xffffffff);
	   emit2(F_NUMBER64,a,b);
	}
	else
	   emit1(F_NUMBER,i);
#else
	emit1(F_NUMBER,n->u.sval.u.integer);
#endif
      }
      return 1;

    case T_STRING:
      tmp1=store_prog_string(n->u.sval.u.string);
      emit1(F_STRING, DO_NOT_WARN((INT32)tmp1));
      return 1;

    case T_FUNCTION:
      if(n->u.sval.subtype!=FUNCTION_BUILTIN)
      {
	if(n->u.sval.u.object == Pike_compiler->fake_object)
	{
	  /* When does this occur? /mast */
	  emit1(F_GLOBAL,n->u.sval.subtype);
	  return 1;
	}

	if(n->u.sval.u.object->next == n->u.sval.u.object)
	{
	  int x=0;
#if 0
	  struct object *o;

	  for(o=Pike_compiler->fake_object;o!=n->u.sval.u.object;o=o->parent)
	    x++;
#else
	  struct program_state *state=Pike_compiler;
	  for(;state->fake_object!=n->u.sval.u.object;state=state->previous)
	    x++;
#endif
	  emit2(F_EXTERNAL, n->u.sval.subtype, x);
	  Pike_compiler->new_program->flags |=
	    PROGRAM_USES_PARENT | PROGRAM_NEEDS_PARENT;
	  return 1;
	}
      }
      /* FALL_THROUGH */
    default:
#ifdef PIKE_DEBUG
      if((n->u.sval.type == T_OBJECT) &&
	 (n->u.sval.u.object->next == n->u.sval.u.object))
	Pike_fatal("Internal error: Pointer to parent cannot be a compile time constant!\n");
#endif
      tmp1=store_constant(&(n->u.sval),
			  !(n->tree_info & OPT_EXTERNAL_DEPEND),
			  n->name);
      emit1(F_CONSTANT, DO_NOT_WARN((INT32)tmp1));
      return 1;

    case T_TYPE:
      tmp1 = store_constant(&(n->u.sval), 0, n->name);
      emit1(F_CONSTANT, DO_NOT_WARN((INT32)tmp1));
      return 1;

    case T_ARRAY:
    case T_MAPPING:
    case T_MULTISET:
      tmp1=store_constant(&(n->u.sval),
			  !(n->tree_info & OPT_EXTERNAL_DEPEND),
			  n->name);
      emit1(F_CONSTANT, DO_NOT_WARN((INT32)tmp1));
      
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
		emit0(F_COPY_VALUE);
	      break;

	    case T_MAPPING:
	      mapping_fix_type_field(n->u.sval.u.mapping);
	      if((n->u.sval.u.mapping->data->ind_types |
		  n->u.sval.u.mapping->data->val_types) & BIT_COMPLEX)
		emit0(F_COPY_VALUE);
	      break;

	    case T_MULTISET:
	      multiset_fix_type_field(n->u.sval.u.multiset);
	      if(multiset_ind_types(n->u.sval.u.multiset) & BIT_COMPLEX)
		emit0(F_COPY_VALUE);
	      break;
	  }
	}else{
	  emit0(F_COPY_VALUE);
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
      if(flags & WANT_LVALUE)
      {
	emit2(F_LEXICAL_LOCAL_LVALUE,n->u.id.number,n->u.integer.b);
	return 2;
      }else{
	emit2(F_LEXICAL_LOCAL,n->u.id.number,n->u.integer.b);
	return 1;
      }
    }else{
      if(flags & WANT_LVALUE)
      {
	emit1(F_LOCAL_LVALUE,n->u.id.number);
	return 2;
      }else{
	emit1(F_LOCAL,n->u.id.number);
	return 1;
      }
    }

    case F_TRAMPOLINE:
    {
      struct compiler_frame *f;
      int depth=0;
      for(f=Pike_compiler->compiler_frame;
	  f!=n->u.trampoline.frame;f=f->previous)
	depth++;

      emit2(F_TRAMPOLINE,n->u.trampoline.ident,depth);
      return 1;
    }

  case F_IDENTIFIER: {
    struct identifier *id = ID_FROM_INT(Pike_compiler->new_program, n->u.id.number);
    if(IDENTIFIER_IS_FUNCTION(id->identifier_flags))
    {
      if(flags & WANT_LVALUE)
      {
	yyerror("Cannot assign functions.\n");
      }else{
	if (id->identifier_flags & IDENTIFIER_HAS_BODY)
	  /* Only use this opcode when it's certain that the result
	   * can't zero, i.e. when we know the function isn't just a
	   * prototype. */
	  emit1(F_LFUN,n->u.id.number);
	else
	  emit1(F_GLOBAL,n->u.id.number);
      }
    }else{
      if(flags & WANT_LVALUE)
      {
	emit1(F_GLOBAL_LVALUE,n->u.id.number);
	return 2;
      }else{
	emit1(F_GLOBAL,n->u.id.number);
      }
    }
    return 1;
  }

  case F_VAL_LVAL:
    ret = do_docode(CAR(n),flags);
    return ret + do_docode(CDR(n), flags | DO_LVALUE);

  case F_AUTO_MAP:
    emit0(F_MARK);
    code_expression(CAR(n), 0, "automap function");
    do_encode_automap_arg_list(CDR(n),0);
    emit_apply_builtin("__automap__");
    return 1;

  case F_AUTO_MAP_MARKER:
    yyerror("[*] not supported here.\n");
    emit0(F_CONST0);
    return 1;

  default:
    Pike_fatal("Infernal compiler error (unknown parse-tree-token %d).\n", n->token);
    return 0;			/* make gcc happy */
  }
}

/* Used to generate code for functions. */
INT32 do_code_block(node *n)
{
  INT32 entry_point;
#ifdef PIKE_DEBUG
  if (current_stack_depth != -4711) Pike_fatal("Reentrance in do_code_block().\n");
  current_stack_depth = 0;
#endif

  init_bytecode();
  label_no=1;

  /* NOTE: This is no ordinary label... */
  low_insert_label(0);
  emit1(F_BYTE,Pike_compiler->compiler_frame->max_number_of_locals);
  emit1(F_BYTE,Pike_compiler->compiler_frame->num_args);
  emit0(F_ENTRY);
  emit0(F_START_FUNCTION);

  if(Pike_compiler->compiler_frame->current_function_number >= 0 &&
     (Pike_compiler->new_program->identifier_references[
       Pike_compiler->compiler_frame->current_function_number].id_flags &
      ID_INLINE))
  {
    Pike_compiler->compiler_frame->recur_label=0;
    Pike_compiler->compiler_frame->is_inline=1;
  }

  DO_CODE_BLOCK(n);

  if(Pike_compiler->compiler_frame->recur_label > 0)
  {
#ifdef PIKE_DEBUG
    if(l_flag)
    {
      fprintf(stderr,"Generating inline recursive function.\n");
    }
#endif
    /* generate code again, but this time it is inline */
    Pike_compiler->compiler_frame->is_inline=1;

    /* NOTE: This is no ordinary label... */
    low_insert_label(Pike_compiler->compiler_frame->recur_label);
    emit1(F_BYTE,Pike_compiler->compiler_frame->max_number_of_locals);
    emit1(F_BYTE,Pike_compiler->compiler_frame->num_args);
    emit0(F_ENTRY);
    emit0(F_START_FUNCTION);
    DO_CODE_BLOCK(n);
  }
  entry_point = assemble(1);

#ifdef PIKE_DEBUG
  current_stack_depth = -4711;
#endif
  return entry_point;
}

/* Used by eval_low() to build code for constant expressions. */
INT32 docode(node *n)
{
  INT32 entry_point;
  int label_no_save = label_no;
  dynamic_buffer instrbuf_save = instrbuf;
  int stack_depth_save = current_stack_depth;
  struct statement_label *label_save = current_label;
  struct cleanup_frame *top_cleanups_save = top_statement_label_dummy.cleanups;

  instrbuf.s.str=0;
  label_no=1;
  current_stack_depth = 0;
  current_label = &top_statement_label_dummy;	/* Fix these two to */
  top_statement_label_dummy.cleanups = 0;	/* please F_PUSH_ARRAY. */
  init_bytecode();

  insert_opcode0(F_ENTRY, n->line_number, n->current_file);
  /* FIXME: Should we check that do_docode() returns 1? */
  do_docode(n,0);
  insert_opcode0(F_DUMB_RETURN, n->line_number, n->current_file);
  entry_point = assemble(0);		/* Don't store linenumbers. */

  instrbuf=instrbuf_save;
  label_no = label_no_save;
  current_stack_depth = stack_depth_save;
  current_label = label_save;
  top_statement_label_dummy.cleanups = top_cleanups_save;
  return entry_point;
}
