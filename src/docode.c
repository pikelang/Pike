/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
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
#include "pike_embed.h"
#include "builtin_functions.h"
#include "peep.h"
#include "docode.h"
#include "operators.h"
#include "object.h"
#include "opcodes.h"
#include "lex.h"
#include "mapping.h"
#include "multiset.h"
#include "pike_compiler.h"

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
  INT_TYPE line_number;
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

void upd_int(int offset, INT32 tmp)
{
  memcpy(Pike_compiler->new_program->program+offset, &tmp, sizeof(tmp));
}

INT32 read_int(int offset)
{
  return EXTRACT_INT(Pike_compiler->new_program->program+offset);
}

static int label_no=0;

int alloc_label(void) { return ++label_no; }

int do_jump(int token,INT32 lbl)
{
  struct compilation *c = THIS_COMPILATION;
  if(lbl==-1) lbl=alloc_label();
  emit1(token, lbl);
  return lbl;
}


#define LBLCACHESIZE 4711
#define CURRENT_INSTR ((long)instrbuf.s.len / (long)sizeof(p_instr))
#define MAX_UNWIND 100

static int lbl_cache[LBLCACHESIZE];

static int do_branch(INT32 lbl)
{
  struct compilation *c = THIS_COMPILATION;
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

static void low_insert_label(int lbl)
{
  struct compilation *c = THIS_COMPILATION;
  lbl_cache[ lbl % LBLCACHESIZE ] = CURRENT_INSTR;
  emit1(F_LABEL, lbl);
}

static int ins_label(int lbl)
{
  if(lbl==-1) lbl=alloc_label();
  low_insert_label(lbl);
  return lbl;
}

void modify_stack_depth(int delta)
{
  current_stack_depth += delta;
#ifdef PIKE_DEBUG
  if (current_stack_depth < 0) {
    Pike_fatal("Popped out of virtual stack.\n");
  }
#endif
}

void do_pop(int x)
{
  struct compilation *c = THIS_COMPILATION;
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

static void do_pop_mark(void *UNUSED(ignored))
{
  struct compilation *c = THIS_COMPILATION;
  emit0(F_POP_MARK);
}

static void do_pop_to_mark(void *UNUSED(ignored))
{
  struct compilation *c = THIS_COMPILATION;
  emit0(F_POP_TO_MARK);
}

#ifdef PIKE_DEBUG
static void do_cleanup_synch_mark(void)
{
  struct compilation *c = THIS_COMPILATION;
  if (d_flag > 2)
    emit0(F_CLEANUP_SYNCH_MARK);
}
#endif

static void do_escape_catch(void)
{
  struct compilation *c = THIS_COMPILATION;
  emit0(F_ESCAPE_CATCH);
}

#define DO_CODE_BLOCK(X) do_pop(do_docode((X),DO_NOT_COPY | DO_POP ))

int do_docode(node *n, int flags)
{
  int i;
  int stack_depth_save = current_stack_depth;
  struct compilation *c = THIS_COMPILATION;
  INT_TYPE save_current_line = c->lex.current_line;
  struct pike_string *save_current_file = c->lex.current_file;
  if(!n) return 0;
  c->lex.current_line=n->line_number;
  c->lex.current_file = n->current_file;
#ifdef PIKE_DEBUG
  if (current_stack_depth == -4711) Pike_fatal("do_docode() used outside docode().\n");
#endif
  i=do_docode2(n, flags);
  current_stack_depth = stack_depth_save + i;

  c->lex.current_file = save_current_file;
  c->lex.current_line=save_current_line;
  return i;
}

static int is_efun(node *n, c_fun fun)
{
  return n && n->token == F_CONSTANT &&
    SUBTYPEOF(n->u.sval) == FUNCTION_BUILTIN &&
    n->u.sval.u.efun->function == fun;
}

static void code_expression(node *n, int flags, char *err)
{
  switch(do_docode(n, flags & ~DO_POP))
  {
  case 0: my_yyerror("Void expression for %s",err);
  case 1: return;
  case 2:
    Pike_fatal("Internal compiler error (%s), line %ld, file %s\n",
	       err,
	       (long)THIS_COMPILATION->lex.current_line,
	       THIS_COMPILATION->lex.current_file->str);
  }
}

static void do_cond_jump(node *n, int label, int iftrue, int flags)
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

static int has_automap(node *n)
{
  if(!n) return 0;
  switch(n->token)
  {
  case F_AUTO_MAP_MARKER:
  case F_AUTO_MAP:
    return 1;

  default:
    if(car_is_node(n) && has_automap(CAR(n)) )
      return 1;
    if( cdr_is_node(n) && has_automap(CDR(n)) )
      return 1;
  }
  return 0;
}


int generate_call_function(node *n)
{
  struct compilation *c = THIS_COMPILATION;
  emit0(F_MARK);
  PUSH_CLEANUP_FRAME(do_pop_mark, 0);
  do_docode(CDR(n),DO_NOT_COPY);
  emit0(F_CALL_FUNCTION);
  POP_AND_DONT_CLEANUP;
  return 1;
}

static struct compiler_frame *find_local_frame(INT32 depth)
{
  struct compiler_frame *f=Pike_compiler->compiler_frame;
  while(--depth>=0) f=f->previous;
  return f;
}

/* Emit code for a function call to the identifier reference #id,
 * with the arguments specified by args.
 */
static int do_lfun_call(int id, node *args)
{
  struct compilation *c = THIS_COMPILATION;
  struct reference *ref =
    Pike_compiler->new_program->identifier_references + id;

  if((Pike_compiler->compiler_frame->current_function_number >= 0) &&
     ((id == Pike_compiler->compiler_frame->current_function_number) ||
      ((!ref->inherit_offset) &&
       (ref->identifier_offset ==
	Pike_compiler->new_program->
	identifier_references[Pike_compiler->compiler_frame->
			      current_function_number].identifier_offset))) &&
     !(Pike_compiler->new_program->
       identifiers[ref->identifier_offset].identifier_flags &
       (IDENTIFIER_VARARGS|IDENTIFIER_SCOPE_USED)) &&
     !(Pike_compiler->compiler_frame->lexical_scope & SCOPE_SCOPE_USED))
  {
    PUSH_CLEANUP_FRAME(do_pop_mark, 0);
    emit0(F_MARK);
    do_docode(args,0);

  /* Test description:
   *
   * * Test if we have a valid current function.
   *
   * * Quick check if id is the current function.
   *
   * * Check if id is an alternate reference to the current function.
   *
   * * Check that the function isn't varargs or contains scoped functions.
   *
   * * Check that the current function doesn't contain scoped functions.
   */
    if(Pike_compiler->compiler_frame->is_inline || (ref->id_flags & (ID_INLINE|ID_PRIVATE)))
    {
      /* Identifier is declared inline/local
       * or in inlining pass.
       */
      if ((ref->id_flags & (ID_INLINE|ID_PRIVATE)) &&
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
	Pike_compiler->compiler_frame->recur_label =
	  do_jump(F_RECUR, Pike_compiler->compiler_frame->recur_label);
      }
    } else {
      /* Recur if not overloaded. */
      emit1(F_COND_RECUR,id);
      Pike_compiler->compiler_frame->recur_label =
	do_jump(F_POINTER, Pike_compiler->compiler_frame->recur_label);
    }
    POP_AND_DONT_CLEANUP;
    return 1;
  }
 else {
#ifdef USE_APPLY_N
   int nargs = count_args(args);
    if( nargs == -1 )
    {
#endif
     PUSH_CLEANUP_FRAME(do_pop_mark, 0);
      emit0(F_MARK);
      do_docode(args,0);
      emit1(F_CALL_LFUN, id);
    POP_AND_DONT_CLEANUP;
    return 1;
#ifdef USE_APPLY_N
    }
    else
    {
      do_docode(args,0);
      emit2(F_CALL_LFUN_N, id, nargs);
    }
#endif
  }
  return 1;
}

/*
 * FIXME: this can be optimized, but is not really used
 * enough to be worth it yet.
 */
static void emit_apply_builtin(char *func)
{
  INT32 tmp1;
  struct compilation *c = THIS_COMPILATION;
  struct pike_string *n1=make_shared_string(func);
  node *n=find_module_identifier(n1,0);
  free_string(n1);

  switch(n?n->token:0)
  {
    case F_CONSTANT:
      tmp1=store_constant(&n->u.sval,
			  !(n->tree_info & OPT_EXTERNAL_DEPEND),
			  n->name);
      if(TYPEOF(n->u.sval) == T_FUNCTION &&
	 SUBTYPEOF(n->u.sval) == FUNCTION_BUILTIN)
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
  struct compilation *c = THIS_COMPILATION;
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
      PUSH_CLEANUP_FRAME(do_pop_mark, 0);
      code_expression(n, 0, "[*]");
      emit1(F_NUMBER, depth);
      emit_apply_builtin("__builtin.automap_marker");
      POP_AND_DONT_CLEANUP;
      return 1;
    }
  }
}

static void emit_builtin_svalue(char *func)
{
  INT32 tmp1;
  struct compilation *c = THIS_COMPILATION;
  struct pike_string *n1=make_shared_string(func);
  node *n=find_module_identifier(n1,0);
  free_string(n1);

  switch(n?n->token:0)
  {
    case F_CONSTANT:
      tmp1=store_constant(&n->u.sval,
			  (!(n->tree_info & OPT_EXTERNAL_DEPEND)) &&
			  (TYPEOF(n->u.sval) != T_TYPE),
			  n->name);
      emit1(F_CONSTANT, DO_NOT_WARN((INT32)tmp1));
      break;

    default:
      my_yyerror("docode: Failed to make svalue for builtin %s",func);
  }
  free_node(n);
}

static void emit_range (node *n DO_IF_DEBUG (COMMA int num_args))
{
  struct compilation *c = THIS_COMPILATION;
  node *low = CADR (n), *high = CDDR (n);
  int bound_types = 0;		/* Got bogus gcc warning here. */

  switch (low->token) {
    case F_RANGE_FROM_BEG: bound_types = RANGE_LOW_FROM_BEG; break;
    case F_RANGE_FROM_END: bound_types = RANGE_LOW_FROM_END; break;
    case F_RANGE_OPEN:     bound_types = RANGE_LOW_OPEN; break;
#ifdef PIKE_DEBUG
    default:
      Pike_fatal ("Unexpected node %d as range lower bound.\n", low->token);
#endif
  }

  switch (high->token) {
    case F_RANGE_FROM_BEG: bound_types |= RANGE_HIGH_FROM_BEG; break;
    case F_RANGE_FROM_END: bound_types |= RANGE_HIGH_FROM_END; break;
    case F_RANGE_OPEN:     bound_types |= RANGE_HIGH_OPEN; break;
#ifdef PIKE_DEBUG
    default:
      Pike_fatal ("Unexpected node %d as range upper bound.\n", high->token);
#endif
  }

#ifdef PIKE_DEBUG
  {
    int expected_args = 0;
    switch (bound_types & (RANGE_LOW_OPEN|RANGE_HIGH_OPEN)) {
      case 0:
	expected_args = 2; break;
      case RANGE_LOW_OPEN:
      case RANGE_HIGH_OPEN:
	expected_args = 1; break;
      case RANGE_LOW_OPEN|RANGE_HIGH_OPEN:
	expected_args = 0; break;
    }
    if (num_args != expected_args)
      Pike_fatal ("Wrong number of args to o_range opcode. Expected %d, got %d.\n",
		  expected_args, num_args);
  }
#endif

  emit1 (F_RANGE, bound_types);
}

static void emit_global( int n )
{
  struct compilation *c = THIS_COMPILATION;
  struct reference *ref = PTR_FROM_INT(Pike_compiler->new_program, n);
  struct identifier *id = ID_FROM_PTR(Pike_compiler->new_program, ref);

  if(!(id->identifier_flags & IDENTIFIER_NO_THIS_REF)
     && !ref->inherit_offset
     && !IDENTIFIER_IS_ALIAS(id->identifier_flags)
     && IDENTIFIER_IS_VARIABLE(id->identifier_flags))
  {
    /* fprintf( stderr, "private global %d\n", (INT32)id->func.offset  ); */
    if( ref->id_flags & (ID_PRIVATE|ID_FINAL) )
    {
      if(  id->run_time_type == PIKE_T_MIXED  )
	emit1(F_PRIVATE_GLOBAL, id->func.offset);
      else
	emit2(F_PRIVATE_TYPED_GLOBAL, id->func.offset, id->run_time_type);
      return;
    }

    if( id->run_time_type == PIKE_T_MIXED )
    {
      emit2(F_PRIVATE_IF_DIRECT_GLOBAL, id->func.offset, n);
      return;
    }
/*  else if( (id->func.offset < 65536) && (n<65536) ) */
/*  { */
/* 	INT32 mix = id->func.offset | (n<<16); */
/* 	emit2(F_PRIVATE_IF_DIRECT_TYPED_GLOBAL, mix, id->run_time_type); */
/*  } */
  }
  emit1(F_GLOBAL, n);
}

static void emit_assign_global( int n, int and_pop )
{
  struct compilation *c = THIS_COMPILATION;
  struct reference *ref = PTR_FROM_INT(Pike_compiler->new_program, n);
  struct identifier *id = ID_FROM_PTR(Pike_compiler->new_program, ref);

  if( !(id->identifier_flags & IDENTIFIER_NO_THIS_REF)
      && !ref->inherit_offset
      && !IDENTIFIER_IS_ALIAS(id->identifier_flags)
      && IDENTIFIER_IS_VARIABLE(id->identifier_flags))
  {
    if( (ref->id_flags & (ID_PRIVATE|ID_FINAL)) )
    {
      if( id->run_time_type == PIKE_T_MIXED )
        emit1((and_pop?F_ASSIGN_PRIVATE_GLOBAL_AND_POP:F_ASSIGN_PRIVATE_GLOBAL),
              id->func.offset);
      else
        emit2((and_pop?F_ASSIGN_PRIVATE_TYPED_GLOBAL_AND_POP:F_ASSIGN_PRIVATE_TYPED_GLOBAL),
              id->func.offset, id->run_time_type);
      return;
    }
    if( id->run_time_type == PIKE_T_MIXED )
    {
      emit2(F_ASSIGN_PRIVATE_IF_DIRECT_GLOBAL, id->func.offset, n );
      if( and_pop )
	emit0(F_POP_VALUE);
      return;
    }
  }
  emit1((and_pop?F_ASSIGN_GLOBAL_AND_POP:F_ASSIGN_GLOBAL), n);
}

static int emit_ltosval_call_and_assign( node *lval, node *func, node *args )
{
  struct compilation *c = THIS_COMPILATION;
  node **arg;
  int no = 0;
  int tmp1=store_constant(&func->u.sval,
                          !(func->tree_info & OPT_EXTERNAL_DEPEND),
                          func->name);


#ifdef PIKE_DEBUG
  arg = my_get_arg(&args,0);
  if( !node_is_eq(*arg,lval) )
    Pike_fatal("lval should be the same as arg1, or this will not work.\n");
#endif
  do_docode(lval, DO_LVALUE);
  emit0(F_MARK);
  emit0(F_CONST0);
  PUSH_CLEANUP_FRAME(do_pop_mark, 0);
  while ((arg = my_get_arg(&args, ++no)) && *arg) {
     do_docode(*arg, 0);
   }
   emit1(F_LTOSVAL_CALL_BUILTIN_AND_ASSIGN, DO_NOT_WARN((INT32)tmp1));
   POP_AND_DONT_CLEANUP;
   return 1;
}

static int is_apply_constant_function_arg0( node *n, node *target )
{
    if (/*n->token == F_APPLY &&*/ 
        (CAR(n)->token == F_CONSTANT) &&
        (TYPEOF(CAR(n)->u.sval) == T_FUNCTION) &&
        (SUBTYPEOF(CAR(n)->u.sval) == FUNCTION_BUILTIN) &&
        (CAR(n)->u.sval.u.efun->function != f_map) &&
        (CAR(n)->u.sval.u.efun->function != f_filter)) {
	/* efuns typically don't access object variables. */
        node *args = CDR(n), **arg;
	if (args)
        {
            arg = my_get_arg(&args, 0);
            if (arg && node_is_eq(target, *arg) &&
                !(args->tree_info & OPT_ASSIGNMENT))
            {
                if(match_types(target->type, array_type_string) ||
                   match_types(target->type, string_type_string) ||
                   match_types(target->type, object_type_string) ||
                   match_types(target->type, multiset_type_string) ||
                   match_types(target->type, mapping_type_string))
                {
                    return emit_ltosval_call_and_assign(target,CAR(n),args);
                }
            }
	}
    }
    return 0;
}

static void emit_multi_assign(node *vals, node *vars, int no)
{
  struct compilation *c = THIS_COMPILATION;
  node *var;
  node *val;
  node **valp = my_get_arg(&vals, no);

  if (!vars && (!valp || !*valp)) return;
  if (!(vars && valp && (val = *valp))) {
    yyerror("Argument count mismatch for multi-assignment.\n");
    return;
  }

  if (vars->token == F_LVALUE_LIST) {
    var = CAR(vars);
    vars = CDR(vars);
  } else {
    var = vars;
    vars = NULL;
  }

  switch(var->token) {
  case F_LOCAL:
    if(var->u.integer.a >= 
       find_local_frame(var->u.integer.b)->max_number_of_locals)
      yyerror("Illegal to use local variable here.");

    if(var->u.integer.b) goto normal_assign;

    if (var->node_info & OPT_ASSIGNMENT) {
      /* Initialize. */
      emit0(F_CONST0);
      emit1(F_ASSIGN_LOCAL_AND_POP, var->u.integer.a);
    }
    code_expression(val, 0, "RHS");
    emit_multi_assign(vals, vars, no+1);
    emit1(F_ASSIGN_LOCAL_AND_POP, var->u.integer.a );
    break;

  case F_GET_SET:
    {
      /* Check for the setter function. */
      struct program_state *state = Pike_compiler;
      int program_id = var->u.integer.a;
      int level = 0;
      while (state && (state->new_program->id != program_id)) {
	state = state->previous;
	level++;
      }
      if (!state) {
	yyerror("Lost parent.");
      } else {
	struct reference *ref =
	  PTR_FROM_INT(state->new_program, var->u.integer.b);
	struct identifier *id =
	  ID_FROM_PTR(state->new_program, ref);
	struct inherit *inh =
	  INHERIT_FROM_PTR(state->new_program, ref);
	int f;
#ifdef PIKE_DEBUG
	if (!IDENTIFIER_IS_VARIABLE(id->identifier_flags) ||
	    (id->run_time_type != PIKE_T_GET_SET)) {
	  Pike_fatal("Not a getter/setter in a F_GET_SET node!\n"
		     "  identifier_flags: 0x%08x\n"
		     "  run_time_type; %s (%d)\n",
		     id->identifier_flags,
		     get_name_of_type(id->run_time_type),
		     id->run_time_type);
	}
#endif /* PIKE_DEBUG */
	f = id->func.gs_info.setter;
	if (f == -1) {
	  yywarning("Variable %S lacks a setter.", id->name);
	} else if (!level) {
	  f += inh->identifier_level;
	  emit0(F_MARK);
	  PUSH_CLEANUP_FRAME(do_pop_mark, 0);
	  code_expression(val, 0, "RHS");
	  emit_multi_assign(vals, vars, no+1);
	  emit1(F_CALL_LFUN, f);
	  POP_AND_DONT_CLEANUP;
	  emit0(F_POP_VALUE);
	}
      }
    }
    /* FALL_THROUGH */
  case F_EXTERNAL:
    /* Check that it is in this context */
    if(Pike_compiler ->new_program->id == var->u.integer.a)
    {
      /* Check that it is a variable */
      if(var->u.integer.b != IDREF_MAGIC_THIS &&
	 IDENTIFIER_IS_VARIABLE( ID_FROM_INT(Pike_compiler->new_program, var->u.integer.b)->identifier_flags))
      {
	code_expression(val, 0, "RHS");
	emit_multi_assign(vals, vars, no+1);
        emit_assign_global( var->u.integer.b, 1 );
	break;
      }
    }
    /* fall through */

  default:
  normal_assign:
    do_docode(var, DO_LVALUE);
    if(do_docode(val, 0) != 1) yyerror("RHS is void!");
    emit_multi_assign(vals, vars, no+1);
    emit0(F_ASSIGN_AND_POP);
    break;
  }
}

static int do_docode2(node *n, int flags)
{
  struct compilation *c = THIS_COMPILATION;
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
      case F_INDEX:
      case F_ARROW:
      case F_ARG_LIST:
      case F_COMMA_EXPR:
      case F_EXTERNAL:
      case F_GET_SET:
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
  case F_MAGIC_TYPES:
    emit2(n->token,
	  n->u.node.b->u.sval.u.integer,
	  n->u.node.a->u.sval.u.integer);
    return 1;
      
  case F_EXTERNAL:
  case F_GET_SET:
    {
      int level = 0;
      struct program_state *state = Pike_compiler;
      while (state && (state->new_program->id != n->u.integer.a)) {
	if ((flags & WANT_LVALUE) ||
	    (n->node_info & (OPT_EXTERNAL_DEPEND|OPT_NOT_CONST))) {
	  /* Not a reference to a locally bound external constant.
	   * We will thus need true parent pointers.  */
	  state->new_program->flags |=
	    PROGRAM_USES_PARENT | PROGRAM_NEEDS_PARENT;
	}
	state = state->previous;
	level++;
      }
      if (!state) {
	my_yyerror("Program parent %d lost during compiling.", n->u.integer.a);
	emit1(F_NUMBER,0);
      } else if (flags & WANT_LVALUE) {
	if (n->u.integer.b == IDREF_MAGIC_THIS) {
	  my_yyerror("this is not an lvalue.");
	}
	if (level) {
	  emit2(F_EXTERNAL_LVALUE, n->u.integer.b, level);
	} else {
	  emit1(F_GLOBAL_LVALUE, n->u.integer.b);
	}
	return 2;
      } else {
	struct reference *ref =
	  PTR_FROM_INT(state->new_program, n->u.integer.b);
	struct identifier *id =
	  ID_FROM_PTR(state->new_program, ref);
	if (n->token == F_GET_SET) {
	  struct inherit *inh =
	    INHERIT_FROM_PTR(state->new_program, ref);
	  int f;
#ifdef PIKE_DEBUG
	  if (!IDENTIFIER_IS_VARIABLE(id->identifier_flags) ||
	      (id->run_time_type != PIKE_T_GET_SET)) {
	    Pike_fatal("Not a getter/setter in a F_GET_SET node!\n"
		       "  identifier_flags: 0x%08x\n"
		       "  run_time_type; %s (%d)\n",
		     id->identifier_flags,
		       get_name_of_type(id->run_time_type),
		       id->run_time_type);
	  }
#endif /* PIKE_DEBUG */
	  f = id->func.gs_info.getter;
	  if (f == -1) {
	    yywarning("Variable %S lacks a getter.", id->name);
	  } else if (!level) {
	    return do_lfun_call(f + inh->identifier_level, NULL);
	  } else {
	    /* FIXME: Support inlining for the parent case.
	     *
	     * do_call_external(n->u.integer.a, f + inh->identifier_level,
	     *                  NULL);
	     */
	    emit2(F_EXTERNAL, n->u.integer.b, level);
	  }
	} else if (level) {
	  if (IDENTIFIER_IS_CONSTANT(id->identifier_flags) &&
	      (ref->id_flags & ID_INLINE) &&
	      (id->func.const_info.offset >= 0)) {
	    /* An inline, local or final constant identifier in
	     * a lexically surrounding (aka parent) class.
	     * Avoid vtable traversal during runtime by moving
	     * the constant to this class.
	     */
	    struct svalue *s = &state->new_program->
	      inherits[ref->inherit_offset].prog->
	      constants[id->func.const_info.offset].sval;
	    if (TYPEOF(*s) == T_PROGRAM &&
		s->u.program->flags & PROGRAM_USES_PARENT) {
	      /* An external reference is required. */
	      emit2(F_EXTERNAL, n->u.integer.b, level);
	    } else {
	      int tmp1 = store_constant(s, 1, NULL);
	      emit1(F_CONSTANT, tmp1);
	    }
	  } else {
	    struct program_state *state = Pike_compiler;
	    int e;
	    for (e = level; e; e--) {
	      state->new_program->flags |=
		PROGRAM_USES_PARENT|PROGRAM_NEEDS_PARENT;
	      state = state->previous;
	    }
	    emit2(F_EXTERNAL, n->u.integer.b, level);
	  }
	} else if (n->u.integer.b == IDREF_MAGIC_THIS) {
	  emit1(F_THIS_OBJECT, 0);
	} else if(IDENTIFIER_IS_FUNCTION(id->identifier_flags) &&
		  id->identifier_flags & IDENTIFIER_HAS_BODY)
	{
	  /* Only use this opcode when it's certain that the result
	   * can't zero, i.e. when we know the function isn't just a
	   * prototype. */
	  emit1(F_LFUN, n->u.integer.b);
	} else if (IDENTIFIER_IS_CONSTANT(id->identifier_flags) &&
		   (ref->id_flags & (ID_INLINE|ID_PRIVATE)) && !ref->inherit_offset &&
		   (id->func.const_info.offset >= 0)) {
	  /* An inline, local or final constant identifier.
	   * No need for vtable traversal during runtime.
	   */
	  struct svalue *s = &state->new_program->
	    constants[id->func.const_info.offset].sval;
	  if (TYPEOF(*s) == T_PROGRAM &&
	      s->u.program->flags & PROGRAM_USES_PARENT) {
	    /* Program using parent. Convert to an LFUN. */
	    emit1(F_LFUN, n->u.integer.b);
	  } else {
	    emit1(F_CONSTANT, id->func.const_info.offset);
	  }
	}else{
          emit_global( n->u.integer.b );
	}
      }
    }
    return 1;

  case F_THIS:
    {
      int level = 0;
      struct program_state *state = Pike_compiler;
      int inh = n->u.integer.b;
      while (state && (state->new_program->id != n->u.integer.a)) {
	state = state->previous;
	level++;
      }
      if (!state) {
	my_yyerror("Program parent %d lost during compiling.", n->u.integer.a);
	emit1(F_NUMBER,0);
      } else if (!level && (inh < 0)) {
	emit1(F_THIS_OBJECT, 0);
      } else {
	emit2(F_THIS, level, inh);
      }
      return 1;
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

  case F_APPEND_ARRAY: {
    emit0(F_MARK);
    PUSH_CLEANUP_FRAME(do_pop_mark, 0);
    do_docode(CAR(n),DO_LVALUE);
    emit0(F_CONST0);	/* Reserved for svalue. */
    do_docode(CDR(n),0);
    emit0(F_APPEND_ARRAY);
    POP_AND_DONT_CLEANUP;
    return 1;
  }

  case F_APPEND_MAPPING: {
    emit0(F_MARK);
    PUSH_CLEANUP_FRAME(do_pop_mark, 0);
    do_docode(CAR(n),DO_LVALUE);
    emit0(F_CONST0);	/* Reserved for svalue. */
    do_docode(CDR(n),0);
    emit0(F_APPEND_MAPPING);
    POP_AND_DONT_CLEANUP;
    return 1;
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

  case F_MULTI_ASSIGN:
    if (flags & DO_POP) {
      emit_multi_assign(CAR(n), CDR(n), 0);
      return 0;
    } else {
      /* Fall back to the normal assign case. */
      tmp1=do_docode(CDR(n),DO_LVALUE);
#ifdef PIKE_DEBUG
      if(tmp1 & 1)
	Pike_fatal("Very internal compiler error.\n");
#endif
      emit1(F_ARRAY_LVALUE, DO_NOT_WARN((INT32)(tmp1>>1)));
      emit0(F_MARK);
      PUSH_CLEANUP_FRAME(do_pop_mark, 0);
      do_docode(CAR(n), 0);
      emit_apply_builtin("aggregate");
      POP_AND_DONT_CLEANUP;
      emit0(F_ASSIGN);
      return 1;
    }

  case F_ASSIGN_SELF:
    /* in assign self we know this:
     *
     * car(n) = lvalue
     * cdr(n)= softcast(apply(efun, arglist(car(n),one more arg)))
     *
     * The first argument of the arglist is equal to the lvalue.
     *
     * We only want to evaluate car(n) once.
     */
    if( CDR(n)->token == F_AUTO_MAP_MARKER )
      yyerror("[*] is not yet supported here\n");
    return emit_ltosval_call_and_assign( CDR(n), CAAAR(n), CDAAR(n) );

  case F_ASSIGN:

    if( CDR(n)->token == F_AUTO_MAP_MARKER )
    {
        int depth = 0;
        node *lval = CDR(n);
        while( lval->token == F_AUTO_MAP_MARKER )
        {
            lval = CAR(lval);
            depth++;
        }
        do_docode(lval,0); /* note: not lvalue */
        if(do_docode(CAR(n),0)!=1)
            yyerror("RHS is void!");

        if( CAR(n)->token == F_AUTO_MAP_MARKER ||
            CAR(n)->token == F_AUTO_MAP ||
            /* Well, hello there... ;) */
            /* This is what is generated by a[*] += 10 and such. */
            (CAR(n)->token == F_SOFT_CAST &&
             has_automap(CAR(n))))
        {
          emit1(F_ASSIGN_INDICES,depth);
        }
        else
        {
          emit1(F_ASSIGN_ALL_INDICES,depth);
        }
        if( flags & DO_POP )
            emit0( F_POP_VALUE );
        return !(flags&DO_POP);
    }

    switch(CAR(n)->token)
    {
    case F_RANGE:
      if(node_is_eq(CDR(n),CAAR(n)))
      {
        int num_args;
	/* tmp1=do_docode(CDR(n),DO_LVALUE); */
	if(match_types(CDR(n)->type, array_type_string) ||
	   match_types(CDR(n)->type, string_type_string) ||
	   match_types(CDR(n)->type, object_type_string) ||
	   match_types(CDR(n)->type, multiset_type_string) ||
	   match_types(CDR(n)->type, mapping_type_string))
	{
          do_docode(CDR(n),DO_LVALUE);
	  num_args = do_docode(CDAR(n), 0);
	  switch (num_args)
	  {
	    case 0: emit0(F_LTOSVAL_AND_FREE); break;
	    case 1: emit0(F_LTOSVAL2_AND_FREE); break;
	    case 2: emit0(F_LTOSVAL3_AND_FREE); break;
#ifdef PIKE_DEBUG
	    default:
	      Pike_fatal("Arglebargle glop-glyf?\n");
#endif
	  }
	}else{
          goto do_not_suboptimize_assign;
	  emit0(F_LTOSVAL);
	  num_args = do_docode(CDAR(n), 0);
	}

	if (CAR (n)->token == F_RANGE)
	  emit_range (CAR (n) DO_IF_DEBUG (COMMA num_args));
	else
	  emit0(CAR(n)->token);

	emit0(n->token);
	return n->token==F_ASSIGN; /* So when is this false? /mast */
      }
      goto do_not_suboptimize_assign;

    case F_SOFT_CAST:
        /*  a  = [type]`oper(a,*) */
        if( CAAR(n)->token == F_APPLY &&
            is_apply_constant_function_arg0( CAAR(n), CDR(n) ))
            return 1;
        goto do_not_suboptimize_assign;
    case F_APPLY:
        /*  a  = `oper(a,*) */
        if (is_apply_constant_function_arg0( CAR(n), CDR(n) ))
            return 1;
      /* FALL_THROUGH */
    default:
      do_not_suboptimize_assign:
      switch(CDR(n)->token)
      {
      case F_GLOBAL:
  	  if(CDR(n)->u.integer.b) goto normal_assign;
	  code_expression(CAR(n), 0, "RHS");
          emit_assign_global( CDR(n)->u.integer.a, flags & DO_POP );
          break;
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
	emit1(flags & DO_POP ? F_ASSIGN_LOCAL_AND_POP:F_ASSIGN_LOCAL,
	     CDR(n)->u.integer.a );
	break;

      case F_GET_SET:
      {
        /* Check for the setter function. */
        struct program_state *state = Pike_compiler;
        int program_id = CDR(n)->u.integer.a;
        int level = 0;
        while (state && (state->new_program->id != program_id)) {
          state = state->previous;
          level++;
        }
        if (!state) {
          yyerror("Lost parent.");
        } else {
          struct reference *ref =
            PTR_FROM_INT(state->new_program, CDR(n)->u.integer.b);
          struct identifier *id =
            ID_FROM_PTR(state->new_program, ref);
          struct inherit *inh =
            INHERIT_FROM_PTR(state->new_program, ref);
          int f;
#ifdef PIKE_DEBUG
          if (!IDENTIFIER_IS_VARIABLE(id->identifier_flags) ||
              (id->run_time_type != PIKE_T_GET_SET)) {
            Pike_fatal("Not a getter/setter in a F_GET_SET node!\n"
                       "  identifier_flags: 0x%08x\n"
                       "  run_time_type; %s (%d)\n",
                       id->identifier_flags,
                       get_name_of_type(id->run_time_type),
                       id->run_time_type);
          }
#endif /* PIKE_DEBUG */
          f = id->func.gs_info.setter;
          if (f == -1) {
            yywarning("Variable %S lacks a setter.", id->name);
          } else if (!level) {
            f += inh->identifier_level;
	    PUSH_CLEANUP_FRAME(do_pop_mark, 0);
            if (flags & DO_POP) {
#ifndef USE_APPLY_N
              emit0(F_MARK);
#endif
              code_expression(CAR(n), 0, "RHS");
            } else {
              code_expression(CAR(n), 0, "RHS");
#ifndef USE_APPLY_N
              emit0(F_MARK);
#endif
              emit0(F_DUP);
            }
#ifdef USE_APPLY_N
            emit2(F_CALL_LFUN_N, f, 1);
#else
            emit1(F_CALL_LFUN, f);
#endif
	    POP_AND_DONT_CLEANUP;
            emit0(F_POP_VALUE);
            return !(flags & DO_POP);
          }
        }
      }
      /* FALL_THROUGH */
      case F_EXTERNAL:
        /* Check that it is in this context */
        if(Pike_compiler ->new_program->id == CDR(n)->u.integer.a)
        {
          /* Check that it is a variable */
          if(CDR(n)->u.integer.b != IDREF_MAGIC_THIS &&
             IDENTIFIER_IS_VARIABLE( ID_FROM_INT(Pike_compiler->new_program, CDR(n)->u.integer.b)->identifier_flags))
          {
            code_expression(CAR(n), 0, "RHS");
            emit_assign_global(CDR(n)->u.integer.b, flags & DO_POP );
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

  case F_RANGE:
    tmp1=do_docode(CAR(n),DO_NOT_COPY_TOPLEVEL);
    {
#ifdef PIKE_DEBUG
      int num_args =
#endif
	do_docode (CDR (n), DO_NOT_COPY);
      emit_range (n DO_IF_DEBUG (COMMA num_args));
      return DO_NOT_WARN((INT32)tmp1);
    }

    /* The special bound type nodes are simply ignored when the
     * arglist to the range operator is coded. emit_range looks at
     * them later on instead. */
  case F_RANGE_FROM_BEG:
  case F_RANGE_FROM_END:
    return do_docode (CAR (n), flags);
  case F_RANGE_OPEN:
    return 0;

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

      PUSH_CLEANUP_FRAME(do_pop_mark, 0);
      emit0(F_MARK);
      PUSH_CLEANUP_FRAME(do_pop_mark, 0);
      emit0(F_MARK);
      emit0(F_LTOSVAL);
      emit1(F_NUMBER, depth);
      emit_apply_builtin("__builtin.automap_marker");
      POP_AND_DONT_CLEANUP;
      emit_builtin_svalue("`+");
      emit2(F_REARRANGE,1,1);
      emit1(F_NUMBER, 1);
      emit_apply_builtin("__automap__");
      POP_AND_DONT_CLEANUP;

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

      PUSH_CLEANUP_FRAME(do_pop_mark, 0);
      emit0(F_MARK);
      PUSH_CLEANUP_FRAME(do_pop_mark, 0);
      emit0(F_MARK);
      emit0(F_LTOSVAL);
      emit1(F_NUMBER, depth);
      emit_apply_builtin("__builtin.automap_marker");
      POP_AND_DONT_CLEANUP;
      emit_builtin_svalue("`-");
      emit2(F_REARRANGE,1,1);
      emit1(F_NUMBER, 1);
      emit_apply_builtin("__automap__");
      POP_AND_DONT_CLEANUP;

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

    if(CAR(arr) && CAR(arr)->token==F_RANGE)
    {
      node *range = CAR(arr);
      node *low = CADR(range);
      node *high = CDDR(range);
      if(high->token == F_RANGE_OPEN &&
	 low->token == F_RANGE_FROM_BEG &&
	 match_types (low->type, int_type_string))
      {
	/* Optimize foreach(x[start..],y). */
	do_docode (CAR(range), DO_NOT_COPY_TOPLEVEL);
	do_docode (CDR(arr), DO_NOT_COPY|DO_LVALUE);
	if ((low->token == F_CONSTANT) && (TYPEOF(low->u.sval) == PIKE_T_INT)) {
	  if (low->u.sval.u.integer < 0) {
	    emit0(F_CONST0);
	    goto foreach_arg_pushed;
	  }
	  do_docode (CAR(low), DO_NOT_COPY);
	  goto foreach_arg_pushed;
	}
	do_docode (CAR(low), DO_NOT_COPY);
	tmp1 = alloc_label();
	emit0(F_DUP);
	emit0(F_CONST0);
	do_jump(F_BRANCH_WHEN_GE, tmp1);
	/* The value is negative. replace it with zero. */
	emit0(F_POP_VALUE);
	emit0(F_CONST0);
	low_insert_label(DO_NOT_WARN((INT32)tmp1));
	goto foreach_arg_pushed;
      }
    }
    do_docode(arr,DO_NOT_COPY);
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
    switch(n->type->type) {
    case T_VOID:
      DO_CODE_BLOCK(CAR(n));
      return 0;
    case T_INT:
      /* FIXME: Integer range? */
      tmp1 = do_docode(CAR(n), 0);
      if(!tmp1)
	emit0(F_CONST0);
      else {
	if(tmp1>1)
	  do_pop(DO_NOT_WARN((INT32)(tmp1-1)));
	emit0(F_CAST_TO_INT);
      }
      return 1;
    case T_STRING:
      /* FIXME: String width? */
      tmp1 = do_docode(CAR(n), 0);
      if(!tmp1)
	emit0(F_CONST0);
      else if(tmp1>1)
	do_pop(DO_NOT_WARN((INT32)(tmp1-1)));
      emit0(F_CAST_TO_STRING);
      return 1;
    default:
      if (compile_type_to_runtime_type(n->type) == PIKE_T_MIXED) {
	tmp1 = do_docode(CAR(n), 0);
	if(!tmp1) 
	  emit0(F_CONST0);
	else if(tmp1>1)
	  do_pop(DO_NOT_WARN((INT32)(tmp1-1)));
	return 1;
      }
    }
    {
      struct svalue sv;
      SET_SVAL(sv, T_TYPE, 0, type, n->type);
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
	SET_SVAL(sv, T_TYPE, 0, type, n->type);
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
      int args = count_args(CDR(n));
      if(TYPEOF(CAR(n)->u.sval) == T_FUNCTION)
      {
	if(SUBTYPEOF(CAR(n)->u.sval) == FUNCTION_BUILTIN) /* driver fun? */
	{
	  if(!CAR(n)->u.sval.u.efun->docode || 
	     !CAR(n)->u.sval.u.efun->docode(n))
	  {
	    if(args==1)
	    {
	      do_docode(CDR(n),0);
	      tmp1=store_constant(& CAR(n)->u.sval,
				  !(CAR(n)->tree_info & OPT_EXTERNAL_DEPEND),
				  CAR(n)->name);
	      emit1(F_CALL_BUILTIN1, DO_NOT_WARN((INT32)tmp1));
#ifdef USE_APPLY_N
	    }else if(args>0){
	      do_docode(CDR(n),0);
	      tmp1=store_constant(& CAR(n)->u.sval,
				  !(CAR(n)->tree_info & OPT_EXTERNAL_DEPEND),
				  CAR(n)->name);
	      emit2(F_CALL_BUILTIN_N, DO_NOT_WARN((INT32)tmp1), args);
#endif
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
	    return do_lfun_call(SUBTYPEOF(CAR(n)->u.sval), CDR(n));
       	}
      }
#ifdef USE_APPLY_N
      if( args <= 1 )
#endif
      {
        emit0(F_MARK);
        PUSH_CLEANUP_FRAME(do_pop_mark, 0);
        do_docode(CDR(n),0);
        tmp1=store_constant(& CAR(n)->u.sval,
                            !(CAR(n)->tree_info & OPT_EXTERNAL_DEPEND),
                            CAR(n)->name);
        emit1(F_APPLY, DO_NOT_WARN((INT32)tmp1));
        POP_AND_DONT_CLEANUP;
      }
#ifdef USE_APPLY_N
      else
      {
        do_docode(CDR(n),0);
        tmp1=store_constant(& CAR(n)->u.sval,
                            !(CAR(n)->tree_info & OPT_EXTERNAL_DEPEND),
                            CAR(n)->name);
        emit2(F_APPLY_N, DO_NOT_WARN((INT32)tmp1), args);
      }
#endif
      return 1;
    }
    else if(CAR(n)->token == F_EXTERNAL &&
	    CAR(n)->u.integer.a == Pike_compiler->new_program->id &&
	    CAR(n)->u.integer.b != IDREF_MAGIC_THIS)
    {
      return do_lfun_call(CAR(n)->u.integer.b, CDR(n));
    }
    else if(CAR(n)->token == F_GET_SET &&
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
      if(!foo || foo->token!=F_CONSTANT)
      {
	yyerror("No call_function efun.");
      }else{
	if(TYPEOF(foo->u.sval) == T_FUNCTION &&
	   SUBTYPEOF(foo->u.sval) == FUNCTION_BUILTIN &&
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
  case ':':
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
    current_switch.jumptable=xalloc(sizeof(INT32)*(cases*2+2));
    jumptable=xalloc(sizeof(INT32)*(cases*2+2));

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
    free(order);

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
    free(jumptable);
    free(current_switch.jumptable);

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

	  if (case_val->type) {
	    if (!pike_types_le(case_val->type, current_switch.type)) {
	      if (!match_types(case_val->type, current_switch.type)) {
		yytype_error("Type mismatch in case.",
			     current_switch.type, case_val->type, 0);
	      } else if (c->lex.pragmas & ID_STRICT_TYPES) {
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
      my_yyerror("No surrounding statement labeled %S.", name);
      return 0;

    label_found_1:
      if (n->token == F_CONTINUE && label->continue_label < 0) {
	my_yyerror("Cannot continue the non-loop statement on line %ld.",
		   (long)lbl_name->line_number);
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
	  INT_TYPE save_line = c->lex.current_line;
	  c->lex.current_line = name.line_number;
	  my_yyerror("Duplicate nested labels, previous one on line %d.",
		     lbl_name->line_number);
	  c->lex.current_line = save_line;
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

  case F_RETURN: {
    struct statement_label *p;
    int in_catch = 0;
    do_docode(CAR(n),0);

    /* Insert the appropriate number of F_ESCAPE_CATCH. The rest of
     * the cleanup is handled wholesale in low_return et al.
     * Alternatively we could handle this too in low_return and
     * then allow tail recursion of these kind of returns too. */
    for (p = current_label; p; p = p->prev) {
      struct cleanup_frame *q;
      for (q = p->cleanups; q; q = q->prev) {
	if (q->cleanup == (cleanup_func) do_escape_catch) {
	  in_catch = 1;
	  do_escape_catch();
	}
#ifdef PIKE_DEBUG
	/* Have to pop marks from F_SYNCH_MARK too if the debug level
	 * is high enough to get them inserted, otherwise we'll get
	 * false alarms from debug checks in e.g. POP_CATCH_CONTEXT. */
	else if (d_flag > 2 &&
		 q->cleanup == (cleanup_func) do_cleanup_synch_mark) {
	  /* Use the ordinary pop mark instruction here since we know
	   * the stack isn't in synch and we don't want debug checks
	   * for that. */
	  do_pop_mark (NULL);
	}
#endif
      }
    }

    emit0(in_catch ? F_VOLATILE_RETURN : F_RETURN);
    return 0;
  }

  case F_SSCANF:
    tmp1=do_docode(CDAR(n),DO_NOT_COPY);
    tmp2=do_docode(CDR(n),DO_NOT_COPY | DO_LVALUE);
    emit2(F_SSCANF, DO_NOT_WARN((INT32)(tmp1+tmp2)), CAAR(n)->u.sval.u.integer);
    return 1;

  case F_CATCH: {
    INT32 *prev_switch_jumptable = current_switch.jumptable;

    tmp1=do_jump(F_CATCH,-1);
    PUSH_CLEANUP_FRAME(do_escape_catch, 0);

    /* Entry point called via catching_eval_instruction(). */
    emit0(F_ENTRY);

    PUSH_STATEMENT_LABEL;
    current_switch.jumptable=0;
    current_label->break_label=alloc_label();

    DO_CODE_BLOCK(CAR(n));

    ins_label(current_label->break_label);
    emit0(F_EXIT_CATCH);
    POP_STATEMENT_LABEL;
    current_switch.jumptable = prev_switch_jumptable;
    do_branch (tmp1);

    current_stack_depth++;
    /* Entry point called via catching_eval_instruction() after
     * catching an error.
     *
     * NB: This is reached by subtracting ENTRY_PROLOGUE_SIZE
     *     from the label below.
     * NB: The label must be after the entry, since it may expand to code
     *     that requires the entry code to have run.
     */
    emit0(F_ENTRY);
    ins_label(DO_NOT_WARN((INT32)tmp1));

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
    if(CDR(n)->token != F_CONSTANT || TYPEOF(CDR(n)->u.sval) != T_STRING)
      Pike_fatal("Bug in F_ARROW, index not string.\n");
    if(flags & WANT_LVALUE)
    {
      /* FIXME!!!! ??? I wonder what needs fixing... /Hubbe */
      tmp1=do_docode(CAR(n), 0);
      emit1(F_ARROW_STRING, store_prog_string(CDR(n)->u.sval.u.string));
      return 2;
    }else{
      tmp1 = do_docode(CAR(n), DO_NOT_COPY);
      if ((tmp2 = lfun_lookup_id(CDR(n)->u.sval.u.string)) != -1 ) {
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
	if(n && (n->token==F_CONSTANT) && !(n->node_info & OPT_EXTERNAL_DEPEND))
	  emit0(F_COPY_VALUE);
      }
    }
    return DO_NOT_WARN((INT32)tmp1);

  case F_CONSTANT:
    switch(TYPEOF(n->u.sval))
    {
    case T_INT:
      if(!n->u.sval.u.integer && SUBTYPEOF(n->u.sval) == NUMBER_UNDEFINED)
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
      if(SUBTYPEOF(n->u.sval) != FUNCTION_BUILTIN)
      {
	if(n->u.sval.u.object == Pike_compiler->fake_object)
	{
	  /* When does this occur? /mast */
	  emit1(F_GLOBAL, SUBTYPEOF(n->u.sval));
	  return 1;
	}

	if(n->u.sval.u.object->next == n->u.sval.u.object)
	{
	  int x=0;
#if 0
	  struct object *o;

	  for(o=Pike_compiler->fake_object;o!=n->u.sval.u.object;o=o->parent) {
	    state->new_program->flags |=
	      PROGRAM_USES_PARENT | PROGRAM_NEEDS_PARENT;
	    x++;
	  }
#else
	  struct program_state *state=Pike_compiler;
	  for(;state->fake_object!=n->u.sval.u.object;state=state->previous) {
	    state->new_program->flags |=
	      PROGRAM_USES_PARENT | PROGRAM_NEEDS_PARENT;
	    x++;
	  }
#endif
	  emit2(F_EXTERNAL, SUBTYPEOF(n->u.sval), x);
	  Pike_compiler->new_program->flags |=
	    PROGRAM_USES_PARENT | PROGRAM_NEEDS_PARENT;
	  return 1;
	}
      }
      /* FALL_THROUGH */
    default:
#ifdef PIKE_DEBUG
      if((TYPEOF(n->u.sval) == T_OBJECT) &&
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
	  switch(TYPEOF(n->u.sval))
	  {
	    case T_ARRAY:
	      if(array_fix_type_field(n->u.sval.u.array) & BIT_COMPLEX)
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
	emit2(F_LEXICAL_LOCAL_LVALUE, n->u.integer.a, n->u.integer.b);
	return 2;
      }else{
	emit2(F_LEXICAL_LOCAL, n->u.integer.a, n->u.integer.b);
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
	emit1(F_LOCAL_LVALUE, n->u.integer.a);
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
    {
      struct compiler_frame *f;
      int depth=0;
      for(f=Pike_compiler->compiler_frame;
	  f!=n->u.trampoline.frame;f=f->previous)
	depth++;

      emit2(F_TRAMPOLINE,n->u.trampoline.ident,depth);
      return 1;
    }

  case F_VAL_LVAL:
    ret = do_docode(CAR(n),flags);
    return ret + do_docode(CDR(n), flags | DO_LVALUE);

  case F_AUTO_MAP:
    emit0(F_MARK);
    PUSH_CLEANUP_FRAME(do_pop_mark, 0);
    code_expression(CAR(n), 0, "automap function");
    do_encode_automap_arg_list(CDR(n),0);
    emit_apply_builtin("__automap__");
    POP_AND_DONT_CLEANUP;
    return 1;

  case F_AUTO_MAP_MARKER:
    if( flags & DO_LVALUE )
    {
        do_docode(CAR(n),DO_LVALUE);
    }
    else
    {
        yyerror("[*] not supported here.\n");
        emit0(F_CONST0);
    }
    return 1;

  default:
    Pike_fatal("Infernal compiler error (unknown parse-tree-token %d).\n", n->token);
    return 0;			/* make gcc happy */
  }
}

/* Used to generate code for functions. */
INT32 do_code_block(node *n)
{
  struct compilation *c = THIS_COMPILATION;
  struct reference *id = NULL;
  struct identifier *i = NULL;
  INT32 entry_point;
  int aggregate_cnum = -1;
#ifdef PIKE_DEBUG
  if (current_stack_depth != -4711) Pike_fatal("Reentrance in do_code_block().\n");
  current_stack_depth = 0;
#endif

  if (Pike_compiler->compiler_frame->current_function_number >= 0) {
    id = Pike_compiler->new_program->identifier_references +
      Pike_compiler->compiler_frame->current_function_number;
    i = ID_FROM_PTR(Pike_compiler->new_program, id);
  }

  init_bytecode();
  label_no=1;

  /* NOTE: This is no ordinary label... */
  low_insert_label(0);
  emit0(F_ENTRY);
  emit0(F_START_FUNCTION);

  if (Pike_compiler->compiler_frame->num_args) {
    emit2(F_FILL_STACK, Pike_compiler->compiler_frame->num_args, 1);
  }
  emit1(F_MARK_AT, Pike_compiler->compiler_frame->num_args);
  if (i && i->identifier_flags & IDENTIFIER_VARARGS) {
    struct svalue *sval =
      simple_mapping_string_lookup(get_builtin_constants(), "aggregate");
    if (!sval) {
      yyerror("predef::aggregate() is missing.\n");
      Pike_fatal("No aggregate!\n");
      return 0;
    }
    aggregate_cnum = store_constant(sval, 0, NULL);
    emit1(F_CALL_BUILTIN, aggregate_cnum);
    if (Pike_compiler->compiler_frame->max_number_of_locals !=
	Pike_compiler->compiler_frame->num_args+1) {
      emit2(F_FILL_STACK,
	    Pike_compiler->compiler_frame->max_number_of_locals, 0);
    }
  } else {
    emit0(F_POP_TO_MARK);
    if (Pike_compiler->compiler_frame->max_number_of_locals !=
	Pike_compiler->compiler_frame->num_args) {
      emit2(F_FILL_STACK,
	    Pike_compiler->compiler_frame->max_number_of_locals, 0);
    }
  }
  emit2(F_INIT_FRAME, Pike_compiler->compiler_frame->num_args,
        Pike_compiler->compiler_frame->max_number_of_locals);
  if (Pike_compiler->compiler_frame->lexical_scope & SCOPE_SCOPE_USED) {
    emit1(F_PROTECT_STACK, Pike_compiler->compiler_frame->max_number_of_locals);
  }

  if(id && (id->id_flags & ID_INLINE))
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
    emit0(F_ENTRY);
    emit0(F_START_FUNCTION);

    if (Pike_compiler->compiler_frame->num_args) {
      emit2(F_FILL_STACK, Pike_compiler->compiler_frame->num_args, 1);
    }
    emit1(F_MARK_AT, Pike_compiler->compiler_frame->num_args);
    if (i && i->identifier_flags & IDENTIFIER_VARARGS) {
      emit1(F_CALL_BUILTIN, aggregate_cnum);
      if (Pike_compiler->compiler_frame->max_number_of_locals !=
	  Pike_compiler->compiler_frame->num_args+1) {
	emit2(F_FILL_STACK,
	      Pike_compiler->compiler_frame->max_number_of_locals, 0);
      }
      emit2(F_INIT_FRAME, Pike_compiler->compiler_frame->num_args+1,
	    Pike_compiler->compiler_frame->max_number_of_locals);
    } else {
      emit0(F_POP_TO_MARK);
      if (Pike_compiler->compiler_frame->max_number_of_locals !=
	  Pike_compiler->compiler_frame->num_args) {
	emit2(F_FILL_STACK,
	      Pike_compiler->compiler_frame->max_number_of_locals, 0);
      }
      emit2(F_INIT_FRAME, Pike_compiler->compiler_frame->num_args,
	    Pike_compiler->compiler_frame->max_number_of_locals);
    }
    if (Pike_compiler->compiler_frame->lexical_scope & SCOPE_SCOPE_USED) {
      emit1(F_PROTECT_STACK,
	    Pike_compiler->compiler_frame->max_number_of_locals);
    }

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
