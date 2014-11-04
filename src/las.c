/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "interpret.h"
#include "las.h"
#include "array.h"
#include "object.h"
#include "stralloc.h"
#include "dynamic_buffer.h"
#include "lex.h"
#include "pike_types.h"
#include "constants.h"
#include "mapping.h"
#include "multiset.h"
#include "pike_error.h"
#include "docode.h"
#include "main.h"
#include "pike_memory.h"
#include "operators.h"
#include "callback.h"
#include "pike_macros.h"
#include "peep.h"
#include "builtin_functions.h"
#include "cyclic.h"
#include "opcodes.h"
#include "pikecode.h"
#include "gc.h"
#include "pike_compiler.h"
#include "block_allocator.h"

/* Define this if you want the optimizer to be paranoid about aliasing
 * effects to to indexing.
 */
/* #define PARANOID_INDEXING */

static node *eval(node *);
static void optimize(node *n);

int cumulative_parse_error=0;
extern char *get_type_name(int);

#define MAX_GLOBAL 2048

/* #define yywarning my_yyerror */

int car_is_node(node *n)
{
  switch(n->token)
  {
  case F_EXTERNAL:
  case F_GET_SET:
  case F_TRAMPOLINE:
  case F_CONSTANT:
  case F_LOCAL:
  case F_THIS:
  case F_VERSION:
    return 0;

  default:
    return !!_CAR(n);
  }
}

int cdr_is_node(node *n)
{
  switch(n->token)
  {
  case F_EXTERNAL:
  case F_GET_SET:
  case F_TRAMPOLINE:
  case F_CONSTANT:
  case F_LOCAL:
  case F_THIS:
  case F_VERSION:
    return 0;

  default:
    return !!_CDR(n);
  }
}

int node_is_leaf(node *n)
{
  switch(n->token)
  {
  case F_EXTERNAL:
  case F_GET_SET:
  case F_TRAMPOLINE:
  case F_CONSTANT:
  case F_LOCAL:
  case F_VERSION:
    return 1;
  }
  return 0;
}

#ifdef PIKE_DEBUG
void check_tree(node *n, int depth)
{
  node *orig_n = n;
  node *parent;

  if(!d_flag) return;

  if (!n) return;

  parent = n->parent;
  n->parent = NULL;

  while(n) {
    if(n->token==USHRT_MAX)
      Pike_fatal("Free node in tree.\n");

    switch(n->token)
    {
    case F_EXTERNAL:
    case F_GET_SET:
      if(n->type)
      {
	int parent_id = n->u.integer.a;
	int id_no = n->u.integer.b;
	struct program_state *state = Pike_compiler;
	while (state && (state->new_program->id != parent_id)) {
	  state = state->previous;
	}
	if (state && id_no != IDREF_MAGIC_THIS) {
	  struct identifier *id = ID_FROM_INT(state->new_program, id_no);
	  if (id) {
#if 0
#ifdef PIKE_DEBUG
	    /* FIXME: This test crashes on valid code because the type of the
	     * identifier can change in pass 2 - Hubbe
	     */
	    if(id->type != n->type)
	    {
	      fputs("Type of external node "
		    "is not matching its identifier.\nid->type: ",stderr);
	      simple_describe_type(id->type);
	      fputs("\nn->type : ", stderr);
	      simple_describe_type(n->type);
	      fputc('\n', stderr);

	      Pike_fatal("Type of external node is not matching its identifier.\n");
	    }
#endif
#endif
	  }
	}
      }
    }

    if(d_flag<2) break;

#ifdef PIKE_DEBUG
    if(!(depth & 1023))
    {
      node *q;
      for(q=n->parent;q;q=q->parent)
	if(q->parent==n)
	  Pike_fatal("Cyclic node structure found.\n");
    }
#endif

    if(car_is_node(n))
    {
      /* Update parent for CAR */
      CAR(n)->parent = n;
      depth++;
      n = CAR(n);
      continue;
    }

    if(cdr_is_node(n))
    {
      /* Update parent for CDR */
      CDR(n)->parent = n;
      depth++;
      n = CDR(n);
      continue;
    }

    while(n->parent &&
	  (!cdr_is_node(n->parent) || (CDR(n->parent) == n))) {
      /* Backtrack */
      n = n->parent;
      depth--;
    }

    if (n->parent && cdr_is_node(n->parent)) {
      /* Jump to the sibling */
      CDR(n->parent)->parent = n->parent;
      n = CDR(n->parent);
      continue;
    }
    break;
  }

  if (n != orig_n) {
    fputs("check_tree() lost track.\n", stderr);
    d_flag = 0;
    fputs("n:", stderr);
    print_tree(n);
    fputs("orig_n:", stderr);
    print_tree(orig_n);
    Pike_fatal("check_tree() lost track.\n");
  }
  n->parent = parent;
}
#endif

static int low_count_args(node *n)
{
  int a,b;

  if(!n) return 0;
  switch(n->token)
  {
  case F_COMMA_EXPR:
  case F_VAL_LVAL:
  case F_ARG_LIST:
    a=count_args(CAR(n));
    if(a==-1) return -1;
    b=count_args(CDR(n));
    if(b==-1) return -1;
    return a+b;

  case F_CAST:
    if(n->type == void_type_string)
      return 0;
    return count_args(CAR(n));

  case F_SOFT_CAST:
    return count_args(CAR(n));

  case F_CASE:
  case F_CASE_RANGE:
  case F_FOR:
  case F_DO:
  case F_LOOP:
  case F_INC_LOOP:
  case F_DEC_LOOP:
  case F_DEC_NEQ_LOOP:
  case F_INC_NEQ_LOOP:
  case F_BREAK:
  case F_RETURN:
  case F_CONTINUE:
  case F_FOREACH:
    return 0;

  case '?':
  {
    int tmp1,tmp2;
    tmp1=count_args(CADR(n));
    tmp2=count_args(CDDR(n));
    if(tmp1 < tmp2) return tmp1;
    return tmp2;
  }

  case F_PUSH_ARRAY:
    return -1;

  case F_APPLY:
    if(CAR(n)->token == F_CONSTANT &&
       TYPEOF(CAR(n)->u.sval) == T_FUNCTION &&
       SUBTYPEOF(CAR(n)->u.sval) == FUNCTION_BUILTIN &&
       n->type == void_type_string)
      return 0;
    return 1;

  case F_RANGE_FROM_BEG:
  case F_RANGE_FROM_END:
    return 1;
  case F_RANGE_OPEN:
    return 0;

  default:
    if(n->type == void_type_string) return 0;
    return 1;
  }
  /* NOT_REACHED */
}

INT32 count_args(node *n)
{
  int total = 0;
  int a,b;
  node *orig = n;
  node *orig_parent;
  node *prev = NULL;
  check_tree(n,0);

  fatal_check_c_stack(16384);

  if(!n) return 0;

  orig_parent = n->parent;
  n->parent = NULL;

  while(1) {
    int val;
    while ((n->token == F_COMMA_EXPR) ||
	   (n->token == F_VAL_LVAL) ||
	   (n->token == F_ARG_LIST)) {
      if (CAR(n)) {
	CAR(n)->parent = n;
	n = CAR(n);
      } else if (CDR(n)) {
	CDR(n)->parent = n;
	n = CDR(n);
      } else {
	/* Unlikely, but... */
	goto backtrack;
      }
    }

    /* Leaf. */
    val = low_count_args(n);
    if (val == -1) {
      total = -1;
      break;
    }
    if (n->parent && (CAR(n->parent) == CDR(n->parent))) {
      /* Same node in both CDR and CAR ==> count twice. */
      val *= 2;
    }
    total += val;

  backtrack:
    while (n->parent &&
	   (!CDR(n->parent) || (n == CDR(n->parent)))) {
      n = n->parent;
    }
    if (!(n = n->parent)) break;
    /* Found a parent where we haven't visited CDR. */
    CDR(n)->parent = n;
    n = CDR(n);
  }

  orig->parent = orig_parent;

  return total;
}

/* FIXME: Ought to use parent pointer to avoid recursion. */
struct pike_type *find_return_type(node *n)
{
  struct pike_type *a, *b;

  check_tree(n,0);

  fatal_check_c_stack(16384);

  if(!n) return 0;

  optimize(n);

  if (n->token == F_RETURN) {
    if (CAR(n)) {
      if (CAR(n)->type) {
	copy_pike_type(a, CAR(n)->type);
      } else {
#ifdef PIKE_DEBUG
	if (l_flag > 2) {
	  fputs("Return with untyped argument.\n", stderr);
	  print_tree(n);
	}
#endif /* PIKE_DEBUG */
	copy_pike_type(a, mixed_type_string);
      }
    } else {
      copy_pike_type(a, zero_type_string);
    }
    return a;
  }

  if(!(n->tree_info & OPT_RETURN)) return 0;

  if(car_is_node(n))
    a=find_return_type(CAR(n));
  else
    a=0;

  if(cdr_is_node(n))
    b=find_return_type(CDR(n));
  else
    b=0;

  if(a)
  {
    if(b) {
      if (a != b) {
	struct pike_type *res = or_pike_types(a, b, 1);
	free_type(a);
	free_type(b);
	return res;
      }
      free_type(b);
    }
    return a;
  }
  return b;
}

int check_tailrecursion(void)
{
  int e;
  if (Pike_compiler->compiler_frame->lexical_scope & SCOPE_SCOPE_USED) {
    /* There might be a lambda around that has references to the old context
     * in which case we can't reuse it with a tail-recursive call.
     */
    return 0;
  }
  if(debug_options & NO_TAILRECURSION) return 0;
  for(e=0;e<Pike_compiler->compiler_frame->max_number_of_locals;e++)
  {
    if(!pike_type_allow_premature_toss(
      Pike_compiler->compiler_frame->variable[e].type))
      return 0;
  }
  return 1;
}

static int check_node_type(node *n, struct pike_type *t, const char *msg)
{
  if (pike_types_le(n->type, t)) return 1;
  if (!match_types(n->type, t)) {
    yytype_report(REPORT_ERROR, NULL, 0, t, NULL, 0, n->type, 0, msg);
    return 0;
  }
  if (THIS_COMPILATION->lex.pragmas & ID_STRICT_TYPES) {
    yytype_report(REPORT_WARNING, NULL, 0, t, NULL, 0, n->type, 0, msg);
  }
  if (runtime_options & RUNTIME_CHECK_TYPES) {
    node *p = n->parent;
    if (CAR(p) == n) {
      (_CAR(p) = mksoftcastnode(t, mkcastnode(mixed_type_string, n)))
	->parent = p;
    } else if (CDR(p) == n) {
      (_CDR(p) = mksoftcastnode(t, mkcastnode(mixed_type_string, n)))
	->parent = p;
    } else {
      yywarning("Failed to find place to insert soft cast.");
    }
  }
  return 1;
}

void init_node_s_blocks() { }

void really_free_node_s(node * n) {
    ba_free(&Pike_compiler->node_allocator, n);
}

MALLOC_FUNCTION
node * alloc_node_s() {
    return ba_alloc(&Pike_compiler->node_allocator);
}

void count_memory_in_node_ss(size_t * num, size_t * size) {
    struct program_state * state = Pike_compiler;

    *num = 0;
    *size = 0;

    while (state) {
        size_t _num, _size;
        ba_count_all(&state->node_allocator, &_num, &_size);
        *num += _num;
        *size += _size;
        state = state->previous;
    }
}

void node_walker(struct ba_iterator * it, void * UNUSED(data)) {
  do {
    node * tmp = ba_it_val(it);

    /*
     * since we free nodes from here, we might iterate over them again.
     * to avoid that we check for the free mark.
     */
    if (tmp->token == USHRT_MAX) continue;

#ifdef PIKE_DEBUG
    if(!cumulative_parse_error)
    {
      fprintf(stderr,"Free node at %p, (%s:%ld) (token=%d).\n",
              (void *)tmp,
              tmp->current_file->str, (long)tmp->line_number,
              tmp->token);

      debug_malloc_dump_references(tmp,0,2,0);

      if(tmp->token==F_CONSTANT)
        print_tree(tmp);
    }
    /* else */
#endif
    {
      /* Free the node and be happy */
      /* Make sure we don't free any nodes twice */
      if(car_is_node(tmp)) _CAR(tmp)=0;
      if(cdr_is_node(tmp)) _CDR(tmp)=0;
#ifdef PIKE_DEBUG
      if (l_flag > 3) {
        fprintf(stderr, "Freeing node that had %d refs.\n",
                tmp->refs);
      }
#endif /* PIKE_DEBUG */
      /* Force the node to be freed. */
      tmp->refs = 1;
      debug_malloc_touch(tmp->type);
      free_node(tmp);
    }
  } while (ba_it_step(it));
}

void free_all_nodes(void)
{
  node *tmp;

#ifndef PIKE_DEBUG
  if(cumulative_parse_error) {
#endif
      ba_walk(&Pike_compiler->node_allocator, &node_walker, NULL);
#ifdef PIKE_DEBUG
      if(!cumulative_parse_error) {
        size_t n, s;
        ba_count_all(&Pike_compiler->node_allocator, &n, &s);
        if (n)
            Pike_fatal("Failed to free %"PRINTSIZET"d nodes when compiling!\n",n);
      }
#else
  }
#endif
  cumulative_parse_error=0;
}

void debug_free_node(node *n)
{
  if(!n) return;

  if (sub_ref(n)) {
#ifdef PIKE_DEBUG
    if(l_flag>9)
      print_tree(n);
#endif /* PIKE_DEBUG */
    return;
  }

  n->parent = NULL;

  do {
#ifdef PIKE_DEBUG
   if(l_flag>9)
      print_tree(n);
#endif /* PIKE_DEBUG */

    debug_malloc_touch(n);

#ifdef PIKE_DEBUG
    if (n->refs) {
      Pike_fatal("Node with refs left about to be killed: %8p\n", n);
    }
#endif /* PIKE_DEBUG */

    switch(n->token)
    {
    case USHRT_MAX:
      Pike_fatal("Freeing node again!\n");
      break;

    case F_CONSTANT:
      free_svalue(&(n->u.sval));
      break;
    }

    if (car_is_node(n)) {
      /* Free CAR */

      if (sub_ref(_CAR(n))) {
	_CAR(n) = NULL;
      } else {
	_CAR(n)->parent = n;
	n = _CAR(n);
	_CAR(n->parent) = NULL;
	continue;
      }
    }
    if (cdr_is_node(n)) {
      /* Free CDR */

      if (sub_ref(_CDR(n))) {
	_CDR(n) = NULL;
      } else {
	_CDR(n)->parent = n;
	n = _CDR(n);
	_CDR(n->parent) = NULL;
	continue;
      }
    }
  backtrack:
    while (n->parent && !cdr_is_node(n->parent)) {
      /* Kill the node and backtrack */
      node *dead = n;

#ifdef PIKE_DEBUG
      if (dead->refs) {
	print_tree(dead);
	Pike_fatal("Killed node %p (%d) still has refs: %d\n",
		   dead, dead->token, dead->refs);
      }
#endif /* PIKE_DEBUG */

      n = n->parent;

      if(dead->type) free_type(dead->type);
      if(dead->name) free_string(dead->name);
      if(dead->current_file) free_string(dead->current_file);
      dead->token=USHRT_MAX;
      really_free_node_s(dead);
    }
    if (n->parent && cdr_is_node(n->parent)) {
      /* Kill node and jump to the sibling. */
      node *dead = n;

#ifdef PIKE_DEBUG
      if (dead->refs) {
	Pike_fatal("Killed node %p still has refs: %d\n", dead, dead->refs);
      }
#endif /* PIKE_DEBUG */

      n = n->parent;
      if(dead->type) free_type(dead->type);
      if(dead->name) free_string(dead->name);
      if(dead->current_file) free_string(dead->current_file);
      dead->token=USHRT_MAX;
      really_free_node_s(dead);

      if (sub_ref(_CDR(n))) {
	_CDR(n) = NULL;
	goto backtrack;
      } else {
	_CDR(n)->parent = n;
	n = _CDR(n);
	_CDR(n->parent) = NULL;
	continue;
      }
    }

    /* Kill root node. */

#ifdef PIKE_DEBUG
    if (n->refs) {
      Pike_fatal("Killed node %p still has refs: %d\n", n, n->refs);
    }
#endif /* PIKE_DEBUG */

    if(n->type) free_type(n->type);
    if(n->name) free_string(n->name);
    if(n->current_file) free_string(n->current_file);

    n->token=USHRT_MAX;
    really_free_node_s(n);
    break;
  } while (n->parent);
}


/* here starts routines to make nodes */
static node *debug_mkemptynode(void)
{
  node *res=alloc_node_s();

  CHECK_COMPILER();

#ifdef __CHECKER__
  memset(res, 0, sizeof(node));
#endif /* __CHECKER__ */

  res->refs = 0;
  add_ref(res);	/* For DMALLOC... */
  res->token=0;
  res->line_number=THIS_COMPILATION->lex.current_line;
  copy_shared_string(res->current_file, THIS_COMPILATION->lex.current_file);
  res->type=0;
  res->name=0;
  res->node_info=0;
  res->tree_info=0;
  res->parent=0;
  return res;
}

#define mkemptynode()	dmalloc_touch(node *, debug_mkemptynode())


static int is_automap_arg_list(node *n)
{
  if(!n) return 0;
  switch(n->token)
  {
    default: return 0;
    case F_ARG_LIST:
      return is_automap_arg_list(CAR(n)) ||
	is_automap_arg_list(CDR(n));

    case F_AUTO_MAP_MARKER: return 1;
  }
}


node *debug_mknode(int token, node *a, node *b)
{
  node *res;

  switch(token)
  {
    case F_APPLY:
      if(is_automap_arg_list(b))
	token=F_AUTO_MAP;
      break;

    case F_INDEX:
      switch((is_automap_arg_list(a) << 1) |
	     is_automap_arg_list(b))
      {
	case 1:
	  res=mkefuncallnode("rows",mknode(F_ARG_LIST,a,copy_node(CAR(b))));
	  free_node(b);
	  return res;

	case 2:
	  res=mkefuncallnode("column",mknode(F_ARG_LIST,copy_node(CAR(a)),b));
	  free_node(a);
	  return res;

	case 3:
	  return mkefuncallnode("`[]",mknode(F_ARG_LIST,a,b));
      }
      break;

#ifdef PIKE_DEBUG
    case F_CAST:
    case F_SOFT_CAST:
      Pike_fatal("Attempt to create a cast-node with mknode()!\n");
    case F_CONSTANT:
      Pike_fatal("Attempt to create an F_CONSTANT-node with mknode()!\n");
    case F_LOCAL:
      Pike_fatal("Attempt to create an F_LOCAL-node with mknode()!\n");
    case F_TRAMPOLINE:
      Pike_fatal("Attempt to create an F_TRAMPOLINE-node with mknode()!\n");
    case F_EXTERNAL:
      Pike_fatal("Attempt to create an F_EXTERNAL-node with mknode()!\n");
    case F_GET_SET:
      Pike_fatal("Attempt to create an F_GET_SET-node with mknode()!\n");
#endif /* PIKE_DEBUG */

#define OPERNODE(X,Y) case X: return mkopernode(("`" #Y), a, b )
      OPERNODE(F_LT,<);
      OPERNODE(F_GT,>);
      OPERNODE(F_LE,<=);
      OPERNODE(F_GE,>=);
      OPERNODE(F_EQ,==);
      OPERNODE(F_NE,!=);
      OPERNODE(F_ADD,+);
      OPERNODE(F_SUBTRACT,-);
      OPERNODE(F_DIVIDE,/);
      OPERNODE(F_MULTIPLY,*);
      OPERNODE(F_MOD,%);
      OPERNODE(F_LSH,<<);
      OPERNODE(F_RSH,>>);
      OPERNODE(F_OR,|);
      OPERNODE(F_AND,&);
      OPERNODE(F_XOR,^);
      OPERNODE(F_NOT,!);
      OPERNODE(F_COMPL,~);
#if 0
      OPERNODE(F_NEGATE,-);
#endif
#undef OPERNODE
  }

  check_tree(a,0);
  check_tree(b,0);

  res = mkemptynode();
  _CAR(res) = dmalloc_touch(node *, a);
  _CDR(res) = dmalloc_touch(node *, b);
  if(a) {
    a->parent = res;
  }
  if(b) {
    b->parent = res;
  }

  res->token = token;
  res->type = 0;

  switch(token)
  {
  case F_CATCH:
    res->node_info |= OPT_SIDE_EFFECT;
    if (a) {
      res->tree_info |= a->tree_info & ~OPT_BREAK;
    }
    break;

  case F_AUTO_MAP:
  case F_APPLY:
    {
      unsigned INT16 opt_flags = OPT_SIDE_EFFECT | OPT_EXTERNAL_DEPEND;
      struct identifier *i = NULL;

      if (a) {
	switch(a->token) {
	case F_CONSTANT:
	  switch(TYPEOF(a->u.sval))
	  {
	    case T_FUNCTION:
	      if (SUBTYPEOF(a->u.sval) == FUNCTION_BUILTIN)
	      {
		opt_flags = a->u.sval.u.efun->flags;
	      } else if (a->u.sval.u.object->prog) {
		i = ID_FROM_INT(a->u.sval.u.object->prog, SUBTYPEOF(a->u.sval));
	      } else {
		yyerror("Calling function in destructed module.");
	      }
	      break;

	    case T_PROGRAM:
	      if(a->u.sval.u.program->flags & PROGRAM_CONSTANT) {
		opt_flags=0;
	      }
	      if (a->u.sval.u.program->flags & PROGRAM_USES_PARENT) {
		yyerror("Can not clone program without parent context.");
	      }
	      break;
	  }
	  break;
	case F_EXTERNAL:
	case F_GET_SET:
	  if (a->u.integer.b != IDREF_MAGIC_THIS) {
	    struct program_state *state = Pike_compiler;
	    int program_id = a->u.integer.a;
	    while (state && (state->new_program->id != program_id)) {
	      state = state->previous;
	    }
	    if (state) {
	      i = ID_FROM_INT(state->new_program, a->u.integer.b);
	    } else {
	      yyerror("Parent has left.");
	    }
	  }
	  break;
	case F_LOCAL:
	  /* FIXME: Should lookup functions in the local scope. */
	default:
	  res->tree_info |= a->tree_info;
	}
	if (i && IDENTIFIER_IS_FUNCTION(i->identifier_flags)) {
	  res->node_info |= i->opt_flags;
	} else {
	  res->node_info |= opt_flags;
	}
      } else {
	res->node_info |= opt_flags;
      }
      res->node_info |= OPT_APPLY;
      if(b) res->tree_info |= b->tree_info;
    }
    break;

  case F_POP_VALUE:
    copy_pike_type(res->type, void_type_string);
    
    if(a) res->tree_info |= a->tree_info;
    if(b) res->tree_info |= b->tree_info;
    break;

  case F_MAGIC_SET_INDEX:
    res->node_info |= OPT_ASSIGNMENT;
    /* FALL_THROUGH */
  case F_MAGIC_INDEX:
  case F_MAGIC_INDICES:
  case F_MAGIC_VALUES:
  case F_MAGIC_TYPES:
  {
    int e;
    struct program_state *state = Pike_compiler;
    res->node_info |= OPT_EXTERNAL_DEPEND;
    if (!b) break;	/* Paranoia; probably compiler error. */
    for(e=0;e<b->u.sval.u.integer;e++)
    {
      state->new_program->flags |= PROGRAM_USES_PARENT | PROGRAM_NEEDS_PARENT;
      state=state->previous;
    }
      
    break;
  }

  case F_UNDEFINED:
    res->node_info |= OPT_EXTERNAL_DEPEND | OPT_SIDE_EFFECT;
    break;

  case F_RETURN:
    res->node_info |= OPT_RETURN;
    break;

  case F_BREAK:
    res->node_info |= OPT_BREAK;
    break;

  case F_CONTINUE:
    res->node_info |= OPT_CONTINUE;
    break;

  case F_DEFAULT:
  case F_CASE:
  case F_CASE_RANGE:
    res->node_info |= OPT_CASE;
    break;

  case F_INC_LOOP:
  case F_INC_NEQ_LOOP:
  case F_DEC_LOOP:
  case F_DEC_NEQ_LOOP:
    res->node_info |= OPT_ASSIGNMENT;
    if (a) {
      res->tree_info |= a->tree_info;
    }
    if (b) {
      res->tree_info |= (b->tree_info & ~(OPT_BREAK|OPT_CONTINUE));
    }
    break;

  case F_SSCANF:
    if(!b || count_args(b) == 0) break;
    res->node_info |= OPT_ASSIGNMENT;
    break;

  case F_APPEND_ARRAY:
  case F_APPEND_MAPPING:
  case F_MULTI_ASSIGN:
  case F_ASSIGN:
  case F_ASSIGN_SELF:
    res->node_info |= OPT_ASSIGNMENT;
    if (a) {
      res->tree_info |= a->tree_info;
    }
    if (b) {
      res->tree_info |= b->tree_info;
    }
    break;

  case F_INC:
  case F_DEC:
  case F_POST_INC:
  case F_POST_DEC:
    res->node_info |= OPT_ASSIGNMENT;
    if (a) {
      res->tree_info |= a->tree_info;
    }
    break;

  case ':':
  case F_RANGE_FROM_BEG:
  case F_RANGE_FROM_END:
  case F_RANGE_OPEN:
    res->node_info |= OPT_FLAG_NODE;
    break;

  default:
    if(a) res->tree_info |= a->tree_info;
    if(b) res->tree_info |= b->tree_info;
  }

  /* We try to optimize most things, but argument lists are hard... */
  if((token != F_ARG_LIST) && (a || b))
    res->node_info |= OPT_TRY_OPTIMIZE;

  res->tree_info |= res->node_info;

#ifdef PIKE_DEBUG
  if(d_flag > 3)
    verify_shared_strings_tables();
#endif

  check_tree(res,0);

#ifdef PIKE_DEBUG
  if(d_flag > 3)
    verify_shared_strings_tables();
#endif

  return res;
}

node *debug_mkstrnode(struct pike_string *str)
{
  node *res = mkemptynode();
  res->token = F_CONSTANT;
  SET_SVAL(res->u.sval, T_STRING, 0, string, str);
  add_ref(str);
  res->type = get_type_of_svalue(&res->u.sval);
  res->tree_info = OPT_SAFE;
  return res;
}

node *debug_mkintnode(INT_TYPE nr)
{
  node *res = mkemptynode();
  res->token = F_CONSTANT;
  SET_SVAL(res->u.sval, T_INT, NUMBER_NUMBER, integer, nr);
  res->type=get_type_of_svalue( & res->u.sval);
  res->tree_info = OPT_SAFE;

  return res;
}

node *debug_mknewintnode(INT_TYPE nr)
{
  node *res = mkemptynode();
  res->token = F_CONSTANT;
  SET_SVAL(res->u.sval, T_INT, NUMBER_NUMBER, integer, nr);
  res->type=get_type_of_svalue( & res->u.sval);
  res->tree_info = OPT_SAFE;
  return res;
}

node *debug_mkfloatnode(FLOAT_TYPE foo)
{
  node *res = mkemptynode();
  res->token = F_CONSTANT;
  copy_pike_type(res->type, float_type_string);
  SET_SVAL(res->u.sval, T_FLOAT, 0, float_number, foo);
  res->tree_info = OPT_SAFE;

  return res;
}


node *debug_mkprgnode(struct program *p)
{
  struct svalue s;
  SET_SVAL(s, T_PROGRAM, 0, program, p);
  return mkconstantsvaluenode(&s);
}

node *debug_mkapplynode(node *func,node *args)
{
  return mknode(F_APPLY, func, args);
}

node *debug_mkefuncallnode(char *function, node *args)
{
  struct pike_string *name;
  node *n;
  /* Force resolving since we don't want to get tangled up in the
   * placeholder object here. The problem is really that the
   * placeholder purport itself to contain every identifier, which
   * makes it hide the real ones in find_module_identifier. This
   * kludge will fail if the class being placeholded actually contains
   * these identifiers, but then again I think it's a bit odd in the
   * first place to look up these efuns in the module being compiled.
   * Wouldn't it be better if this function consulted
   * compiler_handler->get_default_module? /mast */
  int orig_flags;
  SET_FORCE_RESOLVE(orig_flags);
  name = findstring(function);
  if(!name || !(n=find_module_identifier(name,0)))
  {
    UNSET_FORCE_RESOLVE(orig_flags);
    my_yyerror("Internally used efun undefined: %s",function);
    return mkintnode(0);
  }
  UNSET_FORCE_RESOLVE(orig_flags);
  n = mkapplynode(n, args);
  return n;
}

node *debug_mkopernode(char *oper_id, node *arg1, node *arg2)
{
  if(arg1 && arg2)
    arg1=mknode(F_ARG_LIST,arg1,arg2);

  return mkefuncallnode(oper_id, arg1);
}

node *debug_mkversionnode(int major, int minor)
{
  node *res = mkemptynode();
  res->token = F_VERSION;
#ifdef __CHECKER__
  _CDR(res) = 0;
#endif
  res->u.integer.a = major;
  res->u.integer.b = minor;
  return res;
}

node *debug_mklocalnode(int var, int depth)
{
  struct compiler_frame *f;
  int e;
  node *res = mkemptynode();
  res->token = F_LOCAL;

  f=Pike_compiler->compiler_frame;
  for(e=0;e<depth;e++) f=f->previous;
  copy_pike_type(res->type, f->variable[var].type);

  res->node_info = OPT_NOT_CONST;
  res->tree_info = res->node_info;
#ifdef __CHECKER__
  _CDR(res) = 0;
#endif
  res->u.integer.a = var;
  if (depth < 0) {
    /* First appearance of this variable.
     * Add initialization code.
     */
    res->node_info |= OPT_ASSIGNMENT;
    res->u.integer.b = 0;
  } else {
    res->u.integer.b = depth;
  }

  return res;
}

node *debug_mkidentifiernode(int i)
{
  node *res = mkexternalnode(Pike_compiler->new_program, i);
  check_tree(res,0);
  return res;
}

node *debug_mktrampolinenode(int i, struct compiler_frame *frame)
{
  struct compiler_frame *f;
  node *res = mkemptynode();

  res->token = F_TRAMPOLINE;
  copy_pike_type(res->type, ID_FROM_INT(Pike_compiler->new_program, i)->type);

  /* FIXME */
  if(IDENTIFIER_IS_CONSTANT(ID_FROM_INT(Pike_compiler->new_program, i)->identifier_flags))
  {
    res->node_info = OPT_EXTERNAL_DEPEND;
  }else{
    res->node_info = OPT_NOT_CONST;
  }
  res->tree_info=res->node_info;

#ifdef __CHECKER__
  _CDR(res) = 0;
#endif
  res->u.trampoline.ident=i;
  res->u.trampoline.frame=frame;
  
  for(f=Pike_compiler->compiler_frame;f != frame;f=f->previous)
    f->lexical_scope|=SCOPE_SCOPED;
  f->lexical_scope|=SCOPE_SCOPE_USED;

#ifdef SHARED_NODES
  res->u.trampoline.prog = Pike_compiler->new_program;
#endif /* SHARED_NODES */

  check_tree(res,0);
  return res;
}

node *debug_mkexternalnode(struct program *parent_prog, int i)
{
#if 0
  return mkidentifiernode(add_ext_ref(Pike_compiler, parent_prog, i));

#else /* !0 */
  node *res = mkemptynode();
  res->token = F_EXTERNAL;

  if (i == IDREF_MAGIC_THIS) {
    type_stack_mark();
    push_object_type (0, parent_prog->id);
    res->type = pop_unfinished_type();
    res->node_info = OPT_NOT_CONST;
    Pike_compiler->compiler_frame->opt_flags |= OPT_EXTERNAL_DEPEND;
  }
  else {
    struct identifier *id = ID_FROM_INT(parent_prog, i);
#ifdef PIKE_DEBUG
    if(d_flag)
    {
      check_type_string(id->type);
      check_string(id->name);
    }
#endif

    /* Mark the identifier reference as used. */
    PTR_FROM_INT(parent_prog, i)->id_flags |= ID_USED;

    copy_pike_type(res->type, id->type);

    /* FIXME: The IDENTIFIER_IS_ALIAS case isn't handled! */
    if(IDENTIFIER_IS_CONSTANT(id->identifier_flags))
    {
      if (!(PTR_FROM_INT(parent_prog, i)->id_flags & ID_LOCAL)) {
	/* It's possible to overload the identifier. */
	res->node_info = OPT_EXTERNAL_DEPEND;
      } else if (id->func.const_info.offset != -1) {
	struct program *p = PROG_FROM_INT(parent_prog, i);
	struct svalue *s = &p->constants[id->func.const_info.offset].sval;
	if ((TYPEOF(*s) == T_PROGRAM) &&
	    (s->u.program->flags & PROGRAM_USES_PARENT)) {
	  /* The constant program refers to its parent, so we need as well. */
	  res->node_info = OPT_EXTERNAL_DEPEND;
	}
      }
    }else{
      res->node_info = OPT_NOT_CONST;
      if (IDENTIFIER_IS_VARIABLE(id->identifier_flags) &&
	  (id->run_time_type == PIKE_T_GET_SET)) {
	/* Special case of F_EXTERNAL for ease of detection. */
	res->token = F_GET_SET;
      }
    }
    if (i) {
      Pike_compiler->compiler_frame->opt_flags |= OPT_EXTERNAL_DEPEND;
    }
  }
  res->tree_info = res->node_info;

#ifdef __CHECKER__
  _CDR(res) = 0;
#endif
  res->u.integer.a = parent_prog->id;
  res->u.integer.b = i;

#if 0
  /* Don't do this if res about to get inherited, since the inherit won't
   * be affected by later overloading of the inherited class in our parents.
   */
/*   if (!(Pike_compiler->flags & COMPILATION_FORCE_RESOLVE)) { */
    /* Bzot-i-zot */
    state = Pike_compiler;
    while(parent_prog != state->new_program)
    {
      state->new_program->flags |= PROGRAM_USES_PARENT | PROGRAM_NEEDS_PARENT;
      state=state->previous;
    }
/*   } */
#endif /* 0 */

  return res;
#endif /* 0 */
}

node *debug_mkthisnode(struct program *parent_prog, int inherit_num)
{
  struct program_state *state;
  node *res;

#ifdef PIKE_DEBUG
  if ((inherit_num < -1) || (inherit_num > 65535)) {
    Pike_fatal("This is bad: %p, %d\n", parent_prog, inherit_num);
  }
#endif /* PIKE_DEBUG */

  res = mkemptynode();
  res->token = F_THIS;
  type_stack_mark();
  if (inherit_num >= 0) {
    push_object_type(1, parent_prog->inherits[inherit_num].prog->id);
  } else {
    push_object_type(0, parent_prog->id);
  }
  res->type = pop_unfinished_type();
  res->tree_info = res->node_info = OPT_NOT_CONST;

#ifdef __CHECKER__
  _CDR(res) = 0;
#endif
  res->u.integer.a = parent_prog->id;
  res->u.integer.b = inherit_num;

  /* Bzot-i-zot */
  state = Pike_compiler;
  while(parent_prog != state->new_program)
  {
    state->new_program->flags |= PROGRAM_USES_PARENT | PROGRAM_NEEDS_PARENT;
    state=state->previous;
  }

  return res;
}

node *debug_mkcastnode(struct pike_type *type, node *n)
{
  node *res;

  if(!n) return 0;

#ifdef PIKE_DEBUG
  if (!type) {
    Pike_fatal("Casting to no type!\n");
  }
#endif /* PIKE_DEBUG */

  if (type == void_type_string) return mknode(F_POP_VALUE, n, 0);

#if 0
  /* It's not always safe to ignore the cast in this case. E.g. if n
   * has type program, the value can contain a function style program
   * pointer which the cast will turn into a real program
   * reference. */
  if(type==n->type) return n;
#endif

  res = mkemptynode();
  res->token = F_CAST;

  /* FIXME: Consider strengthening the node type [bug 4435].
   *        E.g. the cast in the code
   *
   *          mapping(string:string) m = (["a":"A", "b":"B"]);
   *          return (array)m;
   *
   *        should have a result type of array(array(string)),
   *        rather than array(mixed).
   */     
  copy_pike_type(res->type, type);

  if((type != zero_type_string) &&
     (match_types(object_type_string, type) ||
      match_types(program_type_string, type)))
    res->node_info |= OPT_SIDE_EFFECT;

  res->tree_info |= n->tree_info;

  _CAR(res) = n;
  _CDR(res) = mktypenode(type);

  n->parent = res;

  return res;
}

node *debug_mksoftcastnode(struct pike_type *type, node *n)
{
  node *res;
  struct pike_type *result_type = NULL;

  if(!n) return 0;

#ifdef PIKE_DEBUG
  if (!type) {
    Pike_fatal("Soft cast to no type!\n");
  }
#endif /* PIKE_DEBUG */

  if (Pike_compiler->compiler_pass == 2) {
    if (type == void_type_string) {
      yywarning("Soft cast to void.");
      return mknode(F_POP_VALUE, n, 0);
    }

    if(type==n->type) {
      struct pike_string *t1 = describe_type(type);
      yywarning("Soft cast to %S is a noop.", t1);
      free_string(t1);
      return n;
    }

    if (n->type) {
      if (!(result_type = soft_cast(type, n->type, 0))) {
	ref_push_type_value(n->type);
	ref_push_type_value(type);
	yytype_report(REPORT_ERROR,
		      NULL, 0, type,
		      NULL, 0, n->type,
		      2, "Soft cast of %O to %O isn't a valid cast.");
      } else if (result_type == n->type) {
	ref_push_type_value(n->type);
	ref_push_type_value(type);
	yytype_report(REPORT_WARNING,
		      NULL, 0, NULL,
		      NULL, 0, NULL,
		      2, "Soft cast of %O to %O is a noop.");
      }
    }
  }

  res = mkemptynode();
  res->token = F_SOFT_CAST;
  if (result_type) {
    res->type = result_type;
  } else {
    copy_pike_type(res->type, type);
  }

  res->tree_info |= n->tree_info;

  _CAR(res) = n;
  _CDR(res) = mktypenode(type);

  n->parent = res;

  return res;
}

void resolv_constant(node *n)
{
  struct identifier *i;
  struct program *p;
  INT32 numid;

  check_tree(n,0);

  if(!n)
  {
    push_int(0);
  }else{
    switch(n->token)
    {
    case F_CONSTANT:
      push_svalue(& n->u.sval);
      return;

    case F_EXTERNAL:
    case F_GET_SET:
      if (n->u.integer.b == IDREF_MAGIC_THIS) {
	yyerror ("Expected constant, got reference to this");
	push_int (0);
	return;
      }

      else {
	struct program_state *state = Pike_compiler;
	while (state && (state->new_program->id != n->u.integer.a)) {
	  state = state->previous;
	}
	if(!state)
	{
	  yyerror("Failed to resolve external constant.");
	  push_int(0);
	  return;
	}
	p = state->new_program;
	numid=n->u.integer.b;
      }
      break;

    case F_LOCAL:
      /* FIXME: Ought to have the name of the identifier in the message. */
      yyerror("Expected constant, got local variable.");
      push_int(0);
      return;

    case F_GLOBAL:
      /* FIXME: Ought to have the name of the identifier in the message. */
      yyerror("Expected constant, got global variable.");
      push_int(0);
      return;

    case F_UNDEFINED:
      if(Pike_compiler->compiler_pass==2) {
	/* FIXME: Ought to have the name of the identifier in the message. */
	yyerror("Expected constant, got undefined identifier.");
      }
      push_int(0);
      return;

    default:
    {
      if(is_const(n))
      {
	ptrdiff_t args=eval_low(n,1);
	if(args==1) return;

	if(args!=-1)
	{
	  if(!args)
	  {
	    yyerror("Expected constant, got void expression.");
	  }else{
	    yyerror("Possible internal error!!!");
	    pop_n_elems(DO_NOT_WARN(args-1));
	    return;
	  }
	} else {
	  yyerror("Failed to evaluate constant expression.");
	}
      } else {
	yyerror("Expected constant expression.");
      }
      push_int(0);
      return;
    }
    }

    i = ID_FROM_INT(p, numid);
    
    /* Warning:
     * This code doesn't produce function pointers for class constants,
     * which can be harmful...
     * /Hubbe
     */
    
    if (IDENTIFIER_IS_ALIAS(i->identifier_flags)) {
      struct external_variable_context loc;

      loc.o = Pike_compiler->fake_object;
      do {
	loc.inherit = INHERIT_FROM_INT(p, numid);
	loc.parent_identifier = 0;

	find_external_context(&loc, i->func.ext_ref.depth);
	numid = i->func.ext_ref.id;
	p = loc.o->prog;
	i = ID_FROM_INT(p, numid);
      } while (IDENTIFIER_IS_ALIAS(i->identifier_flags));
    }
    if(IDENTIFIER_IS_CONSTANT(i->identifier_flags))
    {
      if(i->func.const_info.offset != -1)
      {
	push_svalue(&PROG_FROM_INT(p, numid)->
		    constants[i->func.const_info.offset].sval);
      }else{
	if(Pike_compiler->compiler_pass!=1)
	  yyerror("Constant is not defined yet.");
	push_int(0);
      }
    } else {
      my_yyerror("Identifier %S is not a constant", i->name);
      push_int(0);
    }
  }
}

/* Leaves a function or object on the stack */
void resolv_class(node *n)
{
  check_tree(n,0);

  resolv_constant(n);
  switch(TYPEOF(Pike_sp[-1]))
  {
    case T_OBJECT:
      if(!Pike_sp[-1].u.object->prog)
      {
	pop_stack();
	push_int(0);
      }else{
	o_cast(program_type_string, T_PROGRAM);
      }
      break;

    default:
      if (Pike_compiler->compiler_pass!=1)
	yyerror("Illegal program identifier");
      pop_stack();
      push_int(0);
      
    case T_FUNCTION:
    case T_PROGRAM:
      break;
  }
}

/* This one always leaves a program if possible */
void resolv_program(node *n)
{
  if (!n) return;

  check_tree(n,0);

  fix_type_field(n);

  if (!pike_types_le(n->type, inheritable_type_string) &&
      (THIS_COMPILATION->lex.pragmas & ID_STRICT_TYPES)) {
    yytype_report(REPORT_WARNING,
		  n->current_file, n->line_number, inheritable_type_string,
		  n->current_file, n->line_number, n->type,
		  0, "Not a valid object type.\n");
  }

  resolv_class(n);
  switch(TYPEOF(Pike_sp[-1]))
  {
    case T_FUNCTION:
      if(program_from_function(Pike_sp-1))
	break;
      
    default:
      if (Pike_compiler->compiler_pass!=1)
	yyerror("Illegal program identifier");
      pop_stack();
      push_int(0);
      
    case T_PROGRAM:
      break;
  }
}

node *index_node(node *n, char *node_name, struct pike_string *id)
{
  node *ret;
  JMP_BUF tmp;

  check_tree(n,0);

  if (!is_const(n) && !TEST_COMPAT(7, 6)) {
    /* Index dynamically. */
    return mknode(F_INDEX, copy_node(n), mkstrnode(id));
  }

  if(SETJMP(tmp))
  {
    if (node_name) {
      handle_compile_exception ("Couldn't index module %s.", node_name);
    } else {
      handle_compile_exception ("Couldn't index module.");
    }
  }else{
    resolv_constant(n);
    switch(TYPEOF(Pike_sp[-1]))
    {
    case T_INT:
      if (!Pike_sp[-1].u.integer) {
	if(!Pike_compiler->num_parse_error) {
	  if (node_name) {
	    my_yyerror("Failed to index module %s with '%S'. "
		       "(Module doesn't exist?)",
		       node_name, id);
	  } else {
	    my_yyerror("Failed to index module with '%S'. "
		       "(Module doesn't exist?)",
		       id);
	  }
	}
	break;
      }
      /* Fall through. */

    case T_FLOAT:
    case T_STRING:
    case T_ARRAY:
      if (node_name) {
	my_yyerror("Failed to index module %s, got %s. (Not a module?)",
		   node_name, get_name_of_type (TYPEOF(Pike_sp[-1])));
      } else {
	my_yyerror("Failed to index a module, got %s. (Not a module?)",
		   get_name_of_type (TYPEOF(Pike_sp[-1])));
      }
      pop_stack();
      push_int(0);
      break;

    case T_OBJECT:
    case T_PROGRAM:
      if(!(Pike_compiler->new_program->flags & PROGRAM_PASS_1_DONE))
      {
	struct program *p;
	if(TYPEOF(Pike_sp[-1]) == T_OBJECT)
	  p=Pike_sp[-1].u.object->prog;
	else
	  p=Pike_sp[-1].u.program;

	if(p && !(p->flags & PROGRAM_PASS_1_DONE))
	{
	  if(report_compiler_dependency(p))
	  {
	    pop_stack();
#if 0
	    fprintf(stderr, "Placeholder deployed for %p when indexing ", p);
	    print_tree(n);
	    fprintf(stderr, "with %s\n", id->str);
#endif
	    ref_push_object(placeholder_object);
	    break;
	  }
	}
      }

    default:
    {
      ptrdiff_t c;
      DECLARE_CYCLIC();
      c = PTR_TO_INT(BEGIN_CYCLIC(Pike_sp[-1].u.refs, id));
      if(c>1)
      {
	my_yyerror("Recursive module dependency when indexing with '%S'.", id);
	pop_stack();
	push_int(0);
      }else{
	volatile int exception = 0;
	SET_CYCLIC_RET(c+1);
	ref_push_string(id);
	{
	  JMP_BUF recovery;
	  STACK_LEVEL_START(2);
	  if (SETJMP_SP(recovery, 2)) {
	    if (node_name) {
	      handle_compile_exception ("Error looking up '%S' in module %s.",
					id, node_name);
	    } else {
	      handle_compile_exception ("Error looking up '%S' in module.",
					id);
	    }
	    push_undefined();
	    exception = 1;
	  } else {
	    f_index(2);
	  }
	  STACK_LEVEL_DONE(1);
	  UNSETJMP(recovery);
	}
      
	if(TYPEOF(Pike_sp[-1]) == T_INT &&
	   !Pike_sp[-1].u.integer &&
	   SUBTYPEOF(Pike_sp[-1]) == NUMBER_UNDEFINED)
	{
	  if(Pike_compiler->new_program->flags & PROGRAM_PASS_1_DONE)
	  {
	    if (!exception) {
	      struct compilation *c = THIS_COMPILATION;
	      if (node_name) {
		my_yyerror("Index '%S' not present in module %s.",
			   id, node_name);
	      } else {
		my_yyerror("Index '%S' not present in module.", id);
	      }
	      resolv_constant(n);
	      low_yyreport(REPORT_ERROR, NULL, 0, parser_system_string,
			   1, "Indexed module was: %O.");
	    }
	  }else if (!(Pike_compiler->flags & COMPILATION_FORCE_RESOLVE)) {
	    /* Hope it's there in pass 2 */
	    pop_stack();
#if 0
	    fprintf(stderr, "Placeholder deployed when indexing ");
	    print_tree(n);
	    fprintf(stderr, "with %s\n", id->str);
#endif
	    ref_push_object(placeholder_object);
	  }
	}

	else if (Pike_compiler->new_program->flags & PROGRAM_PASS_1_DONE) {
	  if (((TYPEOF(Pike_sp[-1]) == T_OBJECT &&
		Pike_sp[-1].u.object == placeholder_object) ||
	       (TYPEOF(Pike_sp[-1]) == T_PROGRAM &&
		Pike_sp[-1].u.program == placeholder_program)) &&
	      /* Ugly special case: We must be able to get
	       * predef::__placeholder_object. */
	      (!node_name || strcmp (node_name, "predef"))) {
	    if (node_name)
	      my_yyerror("Got placeholder %s when indexing "
			 "module %s with '%S'. (Resolver problem.)",
			 get_name_of_type (TYPEOF(Pike_sp[-1])),
			 node_name, id);
	    else
	      my_yyerror("Got placeholder %s when indexing "
			 "module with '%S'. (Resolver problem.)",
			 get_name_of_type (TYPEOF(Pike_sp[-1])),
			 id);
	  }
	}

	else {
	  /* If we get a program that hasn't gone through pass 1 yet
	   * then we have to register a dependency now in our pass 1
	   * so that our pass 2 gets delayed. Otherwise the other
	   * program might still be just as unfinished when we come
	   * back here in pass 2. */
	  struct program *p = NULL;
	  if (TYPEOF(Pike_sp[-1]) == T_PROGRAM)
	    p = Pike_sp[-1].u.program;
	  else if (TYPEOF(Pike_sp[-1]) == T_OBJECT ||
		   (TYPEOF(Pike_sp[-1]) == T_FUNCTION &&
		    SUBTYPEOF(Pike_sp[-1]) != FUNCTION_BUILTIN))
	    p = Pike_sp[-1].u.object->prog;
	  if (p && !(p->flags & PROGRAM_PASS_1_DONE))
	    report_compiler_dependency (p);
	}
      }
      END_CYCLIC();
    }
    }
  }
  UNSETJMP(tmp);
  ret=mkconstantsvaluenode(Pike_sp-1);
  pop_stack();
  return ret;
}


/* FIXME: Ought to use parent pointer to avoid recursion. */
int node_is_eq(node *a,node *b)
{
  check_tree(a,0);
  check_tree(b,0);

  if(a == b) return 1;
  if(!a || !b) return 0;
  if(a->token != b->token) return 0;

  fatal_check_c_stack(16384);

  switch(a->token)
  {
  case F_TRAMPOLINE: /* FIXME, the context has to be the same! */
#ifdef SHARED_NODES
    if(a->u.trampoline.prog != b->u.trampoline.prog)
      return 0;
#endif
    return a->u.trampoline.ident == b->u.trampoline.ident &&
      a->u.trampoline.frame == b->u.trampoline.frame;

  case F_EXTERNAL:
  case F_GET_SET:
  case F_LOCAL:
    return a->u.integer.a == b->u.integer.a &&
      a->u.integer.b == b->u.integer.b;

  case F_CAST:
  case F_SOFT_CAST:
    return a->type == b->type && node_is_eq(CAR(a), CAR(b));

  case F_CONSTANT:
    return is_equal(&(a->u.sval), &(b->u.sval));

  default:
    if( a->type != b->type ) return 0;
    if(car_is_node(a) && !node_is_eq(CAR(a), CAR(b))) return 0;
    if(cdr_is_node(a) && !node_is_eq(CDR(a), CDR(b))) return 0;
    return 1;
  }
}

node *debug_mktypenode(struct pike_type *t)
{
  node *res = mkemptynode();
  res->token = F_CONSTANT;
  SET_SVAL(res->u.sval, T_TYPE, 0, type, t);
  add_ref(t);
  /* FIXME: Should be type(val) */
  type_stack_mark();
  push_finished_type(t);
  push_type(T_TYPE);
  res->type = pop_unfinished_type();
  return res;
}

node *low_mkconstantsvaluenode(const struct svalue *s)
{
  node *res = mkemptynode();
  res->token = F_CONSTANT;
  assign_svalue_no_free(& res->u.sval, s);
  if(TYPEOF(*s) == T_OBJECT ||
     (TYPEOF(*s) == T_FUNCTION && SUBTYPEOF(*s) != FUNCTION_BUILTIN))
  {
    if(!(s->u.object->prog && (s->u.object->prog->flags & PROGRAM_CONSTANT)))
      res->node_info|=OPT_EXTERNAL_DEPEND;
  }
  res->type = get_type_of_svalue(s);
  res->tree_info |= OPT_SAFE;
  return res;
}

node *debug_mkconstantsvaluenode(const struct svalue *s)
{
  return low_mkconstantsvaluenode(s);
}

node *debug_mkliteralsvaluenode(const struct svalue *s)
{
  node *res = low_mkconstantsvaluenode(s);

  if(TYPEOF(*s) != T_STRING && TYPEOF(*s) != T_INT && TYPEOF(*s) != T_FLOAT)
    res->node_info|=OPT_EXTERNAL_DEPEND;

  return res;
}

node *debug_mksvaluenode(struct svalue *s)
{
  switch(TYPEOF(*s))
  {
  case T_ARRAY:
    return make_node_from_array(s->u.array);
    
  case T_MULTISET:
    return make_node_from_multiset(s->u.multiset);

  case T_MAPPING:
    return make_node_from_mapping(s->u.mapping);

  case T_OBJECT:
#ifdef PIKE_DEBUG
    if (s->u.object->prog == placeholder_program &&
	Pike_compiler->compiler_pass == 2)
      Pike_fatal("Got placeholder object in second pass.\n");
#endif
    if(s->u.object == Pike_compiler->fake_object)
    {
      return mkefuncallnode("this_object", 0);
    }
    if(s->u.object->next == s->u.object)
    {
      int x=0;
      node *n=mkefuncallnode("this_object", 0);
#ifndef PARENT_INFO
      struct object *o;
      for(o=Pike_compiler->fake_object;o!=s->u.object;o=o->parent)
      {
	n=mkefuncallnode("function_object",
			 mkefuncallnode("object_program",n));
      }
#else
      struct program_state *state=Pike_compiler;;
      for(;state->fake_object!=s->u.object;state=state->previous)
      {
	state->new_program->flags |= PROGRAM_USES_PARENT | PROGRAM_NEEDS_PARENT;
	n=mkefuncallnode("function_object",
			 mkefuncallnode("object_program",n));
      }
#endif
      return n;
    }
    break;

  case T_FUNCTION:
  {
    if(SUBTYPEOF(*s) != FUNCTION_BUILTIN)
    {
      if(s->u.object == Pike_compiler->fake_object)
	return mkidentifiernode(SUBTYPEOF(*s));

      if(s->u.object->next == s->u.object)
      {
	return mkexternalnode(s->u.object->prog, SUBTYPEOF(*s));
      }

/*      yyerror("Non-constant function pointer! (should not happen!)"); */
    }
  }
  }

  return mkconstantsvaluenode(s);
}


/* these routines operates on parsetrees and are mostly used by the
 * optimizer
 */


node *copy_node(node *n)
{
  node *b;
  debug_malloc_touch(n);
  debug_malloc_touch(n->type);
  check_tree(n,0);
  if(!n) return n;
  switch(n->token)
  {
  case F_LOCAL:
  case F_TRAMPOLINE:
    b=mknewintnode(0);
    if(b->type) free_type(b->type);
    *b=*n;
    copy_pike_type(b->type, n->type);
    return b;

  default:
    add_ref(n);
    return n;
  }
}

int is_const(node *n)
{
  if(!n) return 1;
  return !(n->tree_info & (OPT_SIDE_EFFECT |
			   OPT_NOT_CONST |
			   OPT_ASSIGNMENT |
			   OPT_CASE |
			   OPT_CONTINUE |
			   OPT_BREAK |
			   OPT_RETURN
			   ));
}

int node_is_tossable(node *n)
{
  if (!(n->tree_info & (OPT_SIDE_EFFECT |
			OPT_ASSIGNMENT |
			OPT_CASE |
			OPT_CONTINUE |
			OPT_BREAK |
			OPT_RETURN
		       ))) {
    ptrdiff_t args;
    if (n->tree_info & (OPT_NOT_CONST|OPT_SAFE))
      return 1;
    args = eval_low (n, 0);
    if (args == -1) {
      n->tree_info |= OPT_SIDE_EFFECT; /* A constant that throws. */
      return 0;
    }
    else {
      pop_n_elems (args);
      n->tree_info |= OPT_SAFE;
      return 1;
    }
  }
  return 0;
}

/* this one supposes that the value is optimized */
int node_is_true(node *n)
{
  if(!n) return 0;
  switch(n->token)
  {
    case F_CONSTANT: return !SAFE_IS_ZERO(& n->u.sval);
    default: return 0;
  }
}

/* this one supposes that the value is optimized */
int node_is_false(node *n)
{
  if(!n) return 0;
  switch(n->token)
  {
    case F_CONSTANT: return SAFE_IS_ZERO(& n->u.sval);
    default: return 0;
  }
}

int node_may_overload(node *n, int lfun)
{
  if(!n) return 0;
  if(!n->type) return 1;
  return type_may_overload(n->type, lfun);
}

/* FIXME: Ought to use parent pointer to avoid recursion. */
node **last_cmd(node **a)
{
  node **n;
  if(!a || !*a) return (node **)NULL;

  fatal_check_c_stack(16384);

  if(((*a)->token == F_CAST) ||
     ((*a)->token == F_SOFT_CAST) ||
     ((*a)->token == F_POP_VALUE)) return last_cmd(&_CAR(*a));
  if(((*a)->token != F_ARG_LIST) &&
     ((*a)->token != F_COMMA_EXPR)) return a;
  if(CDR(*a))
  {
    if(CDR(*a)->token != F_CAST &&
       CDR(*a)->token != F_SOFT_CAST &&
       CDR(*a)->token != F_POP_VALUE &&
       CDR(*a)->token != F_ARG_LIST &&
       CDR(*a)->token != F_COMMA_EXPR)
      return &_CDR(*a);
    if((n=last_cmd(&_CDR(*a))))
      return n;
  }
  if(CAR(*a))
  {
    if(CAR(*a)->token != F_CAST &&
       CAR(*a)->token != F_SOFT_CAST &&
       CAR(*a)->token != F_POP_VALUE &&
       CAR(*a)->token != F_ARG_LIST &&
       CAR(*a)->token != F_COMMA_EXPR)
      return &_CAR(*a);
    if((n=last_cmd(&_CAR(*a))))
      return n;
  }
  return 0;
}

/* FIXME: Ought to use parent pointer to avoid recursion. */
static node **low_get_arg(node **a,int *nr)
{
  node **n;
  if (!a[0]) return NULL;
  if(a[0]->token != F_ARG_LIST)
  {
    if(!(*nr)--)
      return a;
    else
      return NULL;
  }

  fatal_check_c_stack(16384);

  if(CAR(*a))
    if((n=low_get_arg(&_CAR(*a),nr)))
      return n;

  if(CDR(*a))
    if((n=low_get_arg(&_CDR(*a),nr)))
      return n;

  return 0;
}

node **my_get_arg(node **a,int n) { return low_get_arg(a,&n); }

node **is_call_to(node *n, c_fun f)
{
  switch(n->token)
  {
    case F_AUTO_MAP:
    case F_APPLY:
      if(CAR(n) &&
	 CAR(n)->token == F_CONSTANT &&
	 TYPEOF(CAR(n)->u.sval) == T_FUNCTION &&
	 SUBTYPEOF(CAR(n)->u.sval) == FUNCTION_BUILTIN &&
	 CAR(n)->u.sval.u.efun->function == f)
	return &_CDR(n);
  }
  return 0;
}

#ifdef PIKE_DEBUG
/* FIXME: Ought to use parent pointer to avoid recursion. */
static void low_print_tree(node *foo,int needlval)
{
  if(!foo) return;
  if(l_flag>9)
  {
    fprintf(stderr, "/*%x*/",foo->tree_info);
  }

  fatal_check_c_stack(16384);

  switch(l_flag > 99 ? -1 : foo->token)
  {
  case USHRT_MAX:
    fputs("FREED_NODE", stderr);
    break;
  case F_LOCAL:
    if(needlval) fputc('&', stderr);
    if(foo->u.integer.b)
    {
      fprintf(stderr, "$<%ld>%ld",(long)foo->u.integer.b,(long)foo->u.integer.a);
    }else{
      fprintf(stderr, "$%ld",(long)foo->u.integer.a);
    }
    break;

  case '?':
    fputc('(', stderr);
    low_print_tree(_CAR(foo),0);
    fputs(")?(", stderr);
    if (_CDR(foo)) {
      low_print_tree(_CADR(foo),0);
      fputs("):(", stderr);
      low_print_tree(_CDDR(foo),0);
    } else {
      fputs("0:0", stderr);
    }
    fputc(')', stderr);
    break;

  case F_EXTERNAL:
  case F_GET_SET:
    if(needlval) fputc('&', stderr);
    {
      struct program_state *state = Pike_compiler;
      char *name = "?";
      int program_id = foo->u.integer.a;
      int level = 0;
      int id_no = foo->u.integer.b;
      while(state && (state->new_program->id != program_id)) {
	state = state->previous;
	level++;
      }
      if (id_no == IDREF_MAGIC_THIS)
	name = "this";
      else if (state) {
	struct identifier *id = ID_FROM_INT(state->new_program, id_no);
	if (id && id->name) {
	  name = id->name->str;
	}
      }
      fprintf(stderr, "ext(%d:%s)", level, name);
    }
    break;

  case F_TRAMPOLINE:
    if (Pike_compiler->new_program) {
      fprintf(stderr, "trampoline<%s>",
	      ID_FROM_INT(Pike_compiler->new_program, foo->u.trampoline.ident)->name->str);
    } else {
      fputs("trampoline<unknown identifier>", stderr);
    }
    break;

  case F_ASSIGN:
    low_print_tree(_CDR(foo),1);
    fputc('=', stderr);
    low_print_tree(_CAR(foo),0);
    break;

  case F_ASSIGN_SELF:
    low_print_tree(_CDR(foo),1);
    fputc(':', stderr);
    fputc('=', stderr);
    low_print_tree(_CAR(foo),0);
    break;

  case F_POP_VALUE:
    fputc('{', stderr);
    low_print_tree(_CAR(foo), 0);
    fputc('}', stderr);
    break;

  case F_CAST:
  {
    dynamic_buffer save_buf;
    char *s;
    init_buf(&save_buf);
    my_describe_type(foo->type);
    s=simple_free_buf(&save_buf);
    fprintf(stderr, "(%s){",s);
    free(s);
    low_print_tree(_CAR(foo),0);
    fputc('}', stderr);
    break;
  }

  case F_SOFT_CAST:
  {
    dynamic_buffer save_buf;
    char *s;
    init_buf(&save_buf);
    my_describe_type(foo->type);
    s=simple_free_buf(&save_buf);
    fprintf(stderr, "[%s(", s);
    free(s);
    low_print_tree(_CDR(foo), 0);
    fprintf(stderr, ")]{");
    low_print_tree(_CAR(foo),0);
    fputc('}', stderr);
    break;
  }

  case F_COMMA_EXPR:
    low_print_tree(_CAR(foo),0);
    if(_CAR(foo) && _CDR(foo))
    {
      if(_CAR(foo)->type == void_type_string &&
	 _CDR(foo)->type == void_type_string)
	fputs(";\n", stderr);
      else
	fputs(",\n", stderr);
    }
    low_print_tree(_CDR(foo),needlval);
    return;

  case F_ARG_LIST:
    low_print_tree(_CAR(foo),0);
    if(_CAR(foo) && _CDR(foo))
    {
      if(_CAR(foo)->type == void_type_string &&
	 _CDR(foo)->type == void_type_string)
	fputs(";\n", stderr);
      else
	fputc(',', stderr);
    }
    low_print_tree(_CDR(foo),needlval);
    return;

    case F_ARRAY_LVALUE:
      fputc('[', stderr);
      low_print_tree(_CAR(foo),1);
      fputc(']', stderr);
      break;

  case F_LVALUE_LIST:
    low_print_tree(_CAR(foo),1);
    if(_CAR(foo) && _CDR(foo)) fputc(',', stderr);
    low_print_tree(_CDR(foo),1);
    return;

  case F_CONSTANT:
  {
    dynamic_buffer save_buf;
    char *s;
    init_buf(&save_buf);
    describe_svalue(& foo->u.sval, 0, 0);
    s=simple_free_buf(&save_buf);
    fprintf(stderr, "const(%s)",s);
    free(s);
    break;
  }

  case F_VAL_LVAL:
    low_print_tree(_CAR(foo),0);
    fputs(",&", stderr);
    low_print_tree(_CDR(foo),0);
    return;

  case F_AUTO_MAP:
    fputs("__automap__ ", stderr);
    low_print_tree(_CAR(foo),0);
    fputc('(', stderr);
    low_print_tree(_CDR(foo),0);
    fputc(')', stderr);
    return;
  case F_AUTO_MAP_MARKER:
    low_print_tree(_CAR(foo),0);
    fputs("[*]", stderr);
    return;
  case F_APPLY:
    low_print_tree(_CAR(foo),0);
    fputc('(', stderr);
    low_print_tree(_CDR(foo),0);
    fputc(')', stderr);
    return;

  case F_NORMAL_STMT_LABEL:
  case F_CUSTOM_STMT_LABEL:
    fprintf(stderr, "%s:", _CAR(foo)->u.sval.u.string->str);
    low_print_tree(_CDR(foo),0);
    return;

  case F_LOOP:
    fputs("loop(", stderr);
    if(car_is_node(foo)) low_print_tree(_CAR(foo),0);
    fputs(",{", stderr);
    if(cdr_is_node(foo)) low_print_tree(_CDR(foo),0);
    fputs("})", stderr);
    return;

  default:
    if(!car_is_node(foo) && !cdr_is_node(foo))
    {
      fputs(get_token_name(foo->token), stderr);
      return;
    }
    if(foo->token<256)
    {
      fputc(foo->token, stderr);
    }else{
      fputs(get_token_name(foo->token), stderr);
    }
    fputc('(', stderr);
    if(car_is_node(foo)) low_print_tree(_CAR(foo),0);
    if(car_is_node(foo) && cdr_is_node(foo))
      fputc(',', stderr);
    if(cdr_is_node(foo)) low_print_tree(_CDR(foo),0);
    fputc(')', stderr);
    return;
  }
}

void print_tree(node *n)
{
  check_tree(n,0);
  low_print_tree(n,0);
  fputc('\n', stderr);
}
#endif

/* The following routines need much better commenting. */
/* They also needed to support lexical scoping and external variables.
 * /grubba 2000-08-27
 */

/*
 * Known bugs:
 *  * Aliasing is not handled.
 *  * Called functions are assumed not to use lexical scoping.
 */

#if MAX_LOCAL > MAX_GLOBAL
#define MAX_VAR	MAX_LOCAL
#else /* MAX_LOCAL <= MAX_GLOBAL */
#define MAX_VAR MAX_GLOBAL
#endif /* MAX_LOCAL > MAX_GLOBAL */

/* FIXME: Should perhaps use BLOCK_ALLOC for struct scope_info? */
struct scope_info
{
  struct scope_info *next;
  int scope_id;
  char vars[MAX_VAR];
};

struct used_vars
{
  int err;
  int ext_flags;
  /* Note that the locals and externals linked lists are sorted on scope_id. */
  struct scope_info *locals;	/* Lexical scopes. scope_id == depth */
  struct scope_info *externals;	/* External scopes. scope_id == program_id */
};

#define VAR_BLOCKED   0
#define VAR_UNUSED    1
#define VAR_USED      3

/* FIXME: Shouldn't the following two functions be named "*_or_vars"? */

/* Perform a merge into a.
 * Note that b is freed.
 */
static void low_and_vars(struct scope_info **a, struct scope_info *b)
{
  while (*a && b) {
    if ((*a)->scope_id < b->scope_id) {
      a = &((*a)->next);
    } else if ((*a)->scope_id > b->scope_id) {
      struct scope_info *tmp = *a;
      *a = b;
      b = b->next;
      (*a)->next = tmp;
    } else {
      struct scope_info *tmp = b;
      int e;
      for (e = 0; e < MAX_VAR; e++) {
	(*a)->vars[e] |= b->vars[e];
      }
      a = &((*a)->next);
      b = b->next;
      free(tmp);
    }
  }
  if (!*a) {
    *a = b;
  }
}

/* NOTE: The second argument will be freed. */
static void do_and_vars(struct used_vars *a,struct used_vars *b)
{
  low_and_vars(&(a->locals), b->locals);
  low_and_vars(&(a->externals), b->externals);

  a->err |= b->err;
  a->ext_flags |= b->ext_flags;
  free(b);
}

/* Makes a copy of a.
 * Note: Can throw errors on out of memory.
 */
static struct used_vars *copy_vars(struct used_vars *a)
{
  struct used_vars *ret;
  struct scope_info *src;
  struct scope_info **dst;
  ret=xalloc(sizeof(struct used_vars));
  src = a->locals;
  dst = &(ret->locals);
  *dst = NULL;
  while (src) {
    *dst = malloc(sizeof(struct scope_info));
    if (!*dst) {
      src = ret->locals;
      while(src) {
	struct scope_info *tmp = src->next;
	free(src);
	src = tmp;
      }
      free(ret);
      Pike_error("Out of memory in copy_vars.\n");
      return NULL;	/* Make sure that the optimizer knows we exit here. */
    }
    memcpy(*dst, src, sizeof(struct scope_info));
    src = src->next;
    dst = &((*dst)->next);
    *dst = NULL;
  }
  src = a->externals;
  dst = &(ret->externals);
  *dst = NULL;
  while (src) {
    *dst = malloc(sizeof(struct scope_info));
    if (!*dst) {
      src = ret->locals;
      while(src) {
	struct scope_info *tmp = src->next;
	free(src);
	src = tmp;
      }
      src = ret->externals;
      while(src) {
	struct scope_info *tmp = src->next;
	free(src);
	src = tmp;
      }
      free(ret);
      Pike_error("Out of memory in copy_vars.\n");
      return NULL;	/* Make sure that the optimizer knows we exit here. */
    }
    memcpy(*dst, src, sizeof(struct scope_info));
    src = src->next;
    dst = &((*dst)->next);
    *dst = NULL;
  }
  
  ret->err = a->err;
  ret->ext_flags = a->ext_flags;
  return ret;
}

/* Find the insertion point for the variable a:scope_id:num.
 * Allocates a new scope if needed.
 * Can throw errors on out of memory.
 */
char *find_q(struct scope_info **a, int num, int scope_id)
{
  struct scope_info *new;

#ifdef PIKE_DEBUG
  if (l_flag > 3) {
    fprintf(stderr, "find_q %d:%d\n", scope_id, num);
  }
#endif /* PIKE_DEBUG */
  while (*a && ((*a)->scope_id < scope_id)) {
    a = &((*a)->next);
  }
  if ((*a) && ((*a)->scope_id == scope_id)) {
#ifdef PIKE_DEBUG
    if (l_flag > 4) {
      fputs("scope found.\n", stderr);
    }
#endif /* PIKE_DEBUG */
    return (*a)->vars + num;
  }
#ifdef PIKE_DEBUG
  if (l_flag > 4) {
    fputs("Creating new scope.\n", stderr);
  }
#endif /* PIKE_DEBUG */
  new = (struct scope_info *)xalloc(sizeof(struct scope_info));
  memset(new, VAR_UNUSED, sizeof(struct scope_info));
  new->next = *a;
  new->scope_id = scope_id;
  *a = new;
  return new->vars + num;
}

/* FIXME: Ought to use parent pointer to avoid recursion. */
/* Find the variables that are used in the tree n. */
/* noblock: Don't mark unused variables that are written to as blocked.
 * overwrite: n is an lvalue that is overwritten.
 */
static int find_used_variables(node *n,
			       struct used_vars *p,
			       int noblock,
			       int overwrite)
{
  struct used_vars *a;
  char *q;

  if(!n) return 0;

  fatal_check_c_stack(16384);

  switch(n->token)
  {
  case F_LOCAL:
    q = find_q(&(p->locals), n->u.integer.a, n->u.integer.b);
#ifdef PIKE_DEBUG
    if (l_flag > 2) {
      fprintf(stderr, "local %d:%d is ",
	      n->u.integer.b, n->u.integer.a);
    }
#endif /* PIKE_DEBUG */
    goto set_pointer;

  case F_EXTERNAL:
  case F_GET_SET:
    q = find_q(&(p->externals), n->u.integer.b, n->u.integer.a);
#ifdef PIKE_DEBUG
    if (l_flag > 2) {
      fprintf(stderr, "external %d:%d is ",
	      n->u.integer.a, n->u.integer.b);
    }
#endif /* PIKE_DEBUG */
  set_pointer:
    if(overwrite)
    {
      if(*q == VAR_UNUSED && !noblock) {
	*q = VAR_BLOCKED;
#ifdef PIKE_DEBUG
	if (l_flag > 2) {
	  fputs("blocked\n", stderr);
	}
      } else {
	if (l_flag > 2) {
	  fputs("overwritten\n", stderr);
	}
#endif /* PIKE_DEBUG */
      }
    }
    else
    {
      if(*q == VAR_UNUSED) {
	*q = VAR_USED;
#ifdef PIKE_DEBUG
	if (l_flag > 2) {
	  fputs("used\n", stderr);
	}
      } else {
	if (l_flag > 2) {
	  fputs("kept\n", stderr);
	}
#endif /* PIKE_DEBUG */
      }
    }
    break;

  case F_ARROW:
  case F_INDEX:
#ifdef PARANOID_INDEXING
    /* Be paranoid, and assume aliasing. */
    p->ext_flags = VAR_USED;
#endif /* PARANOID_INDEXING */
    if(car_is_node(n)) find_used_variables(CAR(n),p,noblock,0);
    if(cdr_is_node(n)) find_used_variables(CDR(n),p,noblock,0);
    break;

  case F_ASSIGN:
  case F_ASSIGN_SELF:
    find_used_variables(CAR(n),p,noblock,0);
    find_used_variables(CDR(n),p,noblock,1);
    break;

  case '?':
    find_used_variables(CAR(n),p,noblock,0);
    a=copy_vars(p);
    find_used_variables(CADR(n),a,noblock,0);
    find_used_variables(CDDR(n),p,noblock,0);
    do_and_vars(p, a);
    break;

  case F_INC_NEQ_LOOP:
  case F_DEC_NEQ_LOOP:
  case F_INC_LOOP:
  case F_DEC_LOOP:
  case F_LOOP:
  case F_FOREACH:
  case F_FOR:
    find_used_variables(CAR(n),p,noblock,0);
    a=copy_vars(p);
    find_used_variables(CDR(n),a,noblock,0);
    do_and_vars(p, a);
    break;

  case F_SWITCH:
    find_used_variables(CAR(n),p,noblock,0);
    a=copy_vars(p);
    find_used_variables(CDR(n),a,1,0);
    do_and_vars(p, a);
    break;

   case F_DO:
    a=copy_vars(p);
    find_used_variables(CAR(n),a,noblock,0);
    do_and_vars(p, a);
    find_used_variables(CDR(n),p,noblock,0);
    break;

  default:
    if(car_is_node(n)) find_used_variables(CAR(n),p,noblock,0);
    if(cdr_is_node(n)) find_used_variables(CDR(n),p,noblock,0);
  }
  return 0;
}

/* FIXME: Ought to use parent pointer to avoid recursion. */
/* no subtility needed */
static void find_written_vars(node *n,
			      struct used_vars *p,
			      int lvalue)
{
  if(!n) return;

  fatal_check_c_stack(16384);

  switch(n->token)
  {
  case F_LOCAL:
    if(lvalue) {
#ifdef PIKE_DEBUG
      if (l_flag > 2) {
	fprintf(stderr, "local %d:%d is written\n",
		n->u.integer.b, n->u.integer.a);
      }
#endif /* PIKE_DEBUG */
      *find_q(&(p->locals), n->u.integer.a, n->u.integer.b) = VAR_USED;
    }
    break;

  case F_EXTERNAL:
  case F_GET_SET:
    if(lvalue) {
#ifdef PIKE_DEBUG
      if (l_flag > 2) {
	fprintf(stderr, "external %d:%d is written\n",
		n->u.integer.a, n->u.integer.b);
      }
#endif /* PIKE_DEBUG */
      *find_q(&(p->externals), n->u.integer.b, n->u.integer.a) = VAR_USED;
    }
    break;

  case F_APPLY:
  case F_AUTO_MAP:
    if(n->tree_info & OPT_SIDE_EFFECT) {
      p->ext_flags = VAR_USED;
    }
    find_written_vars(CAR(n), p, 0);
    find_written_vars(CDR(n), p, 0);
    break;

  case F_AUTO_MAP_MARKER:
    find_written_vars(CAR(n), p, lvalue);
    break;

  case F_INDEX:
  case F_ARROW:
#ifdef PARANOID_INDEXING
    /* Be paranoid and assume aliasing. */
    if (lvalue)
      p->ext_flags = VAR_USED;
    find_written_vars(CAR(n), p, 0);
#else /* !PARAONID_INDEXING */
    /* Propagate the change to the indexed value.
     * Note: This is sensitive to aliasing effects.
     */
    find_written_vars(CAR(n), p, lvalue);
#endif /* PARANOID_INDEXING */
    find_written_vars(CDR(n), p, 0);
    break;

  case F_SOFT_CAST:
    find_written_vars(CAR(n), p, lvalue);
    break;

  case F_INC:
  case F_DEC:
  case F_POST_INC:
  case F_POST_DEC:
    find_written_vars(CAR(n), p, 1);
    break;

  case F_ASSIGN:
  case F_ASSIGN_SELF:
  case F_MULTI_ASSIGN:
    find_written_vars(CAR(n), p, 0);
    find_written_vars(CDR(n), p, 1);
    break;

  case F_APPEND_MAPPING:
  case F_APPEND_ARRAY:
      find_written_vars(CAR(n), p, 1);
      find_written_vars(CDR(n), p, 0);
      break;

  case F_SSCANF:
    find_written_vars(CAR(n), p, 0);
    /* FIXME: Marks arg 2 as written for now.
     */
    find_written_vars(CDR(n), p, 1);
    break;

    case F_ARRAY_LVALUE:
    find_written_vars(CAR(n), p, 1);
    break;
      
  case F_LVALUE_LIST:
    find_written_vars(CAR(n), p, 1);
    find_written_vars(CDR(n), p, 1);
    break;

  case F_VAL_LVAL:
    find_written_vars(CAR(n), p, 0);
    find_written_vars(CDR(n), p, 1);
    break;
    
  default:
    if(car_is_node(n)) find_written_vars(CAR(n), p, 0);
    if(cdr_is_node(n)) find_written_vars(CDR(n), p, 0);
  }
}

void free_vars(struct used_vars *a)
{
  struct scope_info *tmp;

  tmp = a->locals;
  while(tmp) {
    struct scope_info *next = tmp->next;
    free(tmp);
    tmp = next;
  }
  tmp = a->externals;
  while(tmp) {
    struct scope_info *next = tmp->next;
    free(tmp);
    tmp = next;
  }
}

/* return 1 if A depends on B */
static int depend_p2(node *a, node *b)
{
  struct used_vars aa, bb;
  int e;
  ONERROR free_aa;
  ONERROR free_bb;

  if(!a || !b || is_const(a)) return 0;
  aa.err = 0;
  bb.err = 0;
  aa.ext_flags = 0;
  bb.ext_flags = 0;
  aa.locals = NULL;
  bb.locals = NULL;
  aa.externals = NULL;
  bb.externals = NULL;

  SET_ONERROR(free_aa, free_vars, &aa);
  SET_ONERROR(free_bb, free_vars, &bb);

  /* A depends on B if A uses stuff that is written to by B. */

  find_used_variables(a, &aa, 0, 0);
  find_written_vars(b, &bb, 0);

#ifdef PIKE_DEBUG
  if (l_flag > 2) {
    struct scope_info *aaa = aa.locals;
    while (aaa) {
      fputs("Used locals:\n", stderr);
      for (e = 0; e < MAX_VAR; e++) {
	if (aaa->vars[e] == VAR_USED) {
	  fprintf(stderr, "\t%d:%d\n", aaa->scope_id, e);
	}
      }
      aaa = aaa->next;
    }
    aaa = bb.locals;
    while (aaa) {
      fputs("Written locals:\n", stderr);
      for (e = 0; e < MAX_VAR; e++) {
	if (aaa->vars[e] != VAR_UNUSED) {
	  fprintf(stderr, "\t%d:%d\n", aaa->scope_id, e);
	}
      }
      aaa = aaa->next;
    }
  }
#endif /* PIKE_DEBUG */

  UNSET_ONERROR(free_bb);
  UNSET_ONERROR(free_aa);

  /* If there was an error or
   * If A has external dependencies due to indexing, we won't
   * investigate further.
   */
  if(aa.err || bb.err || aa.ext_flags == VAR_USED) {
    free_vars(&aa);
    free_vars(&bb);
    return 1;
  }

  /* Check for overlap in locals. */
  {
    struct scope_info *aaa = aa.locals;
    struct scope_info *bbb = bb.locals;

    while (aaa) {
      while (bbb && (bbb->scope_id < aaa->scope_id)) {
	bbb = bbb->next;
      }
      if (!bbb) break;
      if (bbb->scope_id == aaa->scope_id) {
	for (e = 0; e < MAX_VAR; e++) {
	  if ((aaa->vars[e] == VAR_USED) &&
	      (bbb->vars[e] != VAR_UNUSED)) {
	    free_vars(&aa);
	    free_vars(&bb);
	    return 1;
	  }	  
	}
      }
      aaa = aaa->next;
    }
  }

  if (bb.ext_flags == VAR_USED) {
    /* B has side effects.
     *
     * A is dependant if A uses any externals at all.
     */

    /* No need to look closer at b */
    struct scope_info *aaa = aa.externals;

    /* FIXME: Probably only needed to check if aaa is NULL or not. */

    while (aaa) {
      for (e = 0; e < MAX_VAR; e++) {
	if (aaa->vars[e] == VAR_USED) {
	  free_vars(&aa);
	  free_vars(&bb);
	  return 1;
	}
      }
      aaa = aaa->next;
    }    
  } else {
    /* Otherwise check for overlaps. */
    struct scope_info *aaa = aa.externals;
    struct scope_info *bbb = bb.externals;

    while (aaa) {
      while (bbb && (bbb->scope_id < aaa->scope_id)) {
	bbb = bbb->next;
      }
      if (!bbb) break;
      if (bbb->scope_id == aaa->scope_id) {
	for (e = 0; e < MAX_VAR; e++) {
	  if ((aaa->vars[e] == VAR_USED) &&
	      (bbb->vars[e] != VAR_UNUSED)) {
	    free_vars(&aa);
	    free_vars(&bb);
	    return 1;
	  }	  
	}
      }
      aaa = aaa->next;
    }
  }
  free_vars(&aa);
  free_vars(&bb);
  return 0;
}

static int depend_p3(node *a,node *b)
{
  if(!b) return 0;
#if 0
  if(!(b->tree_info & OPT_SIDE_EFFECT) && 
     (b->tree_info & OPT_EXTERNAL_DEPEND))
    return 1;
#endif

  if((a->tree_info & OPT_EXTERNAL_DEPEND)) return 1;
    
  return depend_p2(a,b);
}

#ifdef PIKE_DEBUG
static int depend_p(node *a,node *b)
{
  if(l_flag > 3)
  {
    fputs("Checking if: ", stderr);
    print_tree(a);
    fputs("Depends on: ", stderr);
    print_tree(b);
    if(depend_p3(a,b))
    {
      fputs("The answer is (drumroll) : yes\n", stderr);
      return 1;
    }else{
      fputs("The answer is (drumroll) : no\n", stderr);
      return 0;
    }
  }
  return depend_p3(a,b);
}
#else
#define depend_p depend_p3
#endif

/* Check if n depends on the lvalue lval */
static int depend2_p(node *n, node *lval)
{
  node *tmp;
  int ret;

  /* Make a temporary node (lval = 0), so that we can use depend_p(). */
  ADD_NODE_REF2(lval,
		tmp = mknode(F_ASSIGN, mkintnode(0), lval));
  ret = depend_p(n, tmp);
  free_node(tmp);
  return ret;
}

static int function_type_max=0;

static struct pike_string *get_name_of_function(node *n)
{
  struct pike_string *name = NULL;
  if (!n) {
    MAKE_CONST_STRING(name, "NULL");
    return name;
  }
  switch(n->token)
  {
#if 0 /* FIXME */
  case F_TRAMPOLINE:
#endif
  case F_ARROW:
  case F_INDEX:
    if(!CDR(n))
    {
      MAKE_CONST_STRING(name, "NULL");
    }
    else if(CDR(n)->token == F_CONSTANT &&
       TYPEOF(CDR(n)->u.sval) == T_STRING)
    {
      name = CDR(n)->u.sval.u.string;
    }else{
      MAKE_CONST_STRING(name, "dynamically resolved function");
    }
    break;

  case F_CONSTANT:
    switch(TYPEOF(n->u.sval))
    {
    case T_FUNCTION:
      if(SUBTYPEOF(n->u.sval) == FUNCTION_BUILTIN)
      {
	name = n->u.sval.u.efun->name;
      }else{
	name =
	  ID_FROM_INT(n->u.sval.u.object->prog, SUBTYPEOF(n->u.sval))->name;
      }
      break;

    case T_ARRAY:
      MAKE_CONST_STRING(name, "array call");
      break;

    case T_PROGRAM:
      MAKE_CONST_STRING(name, "clone call");
      break;

    default:
      MAKE_CONST_STRING(name, "`() (function call)");
      break;
    }
    break;

  case F_EXTERNAL:
  case F_GET_SET:
    {
      int id_no = n->u.integer.b;

      if (id_no == IDREF_MAGIC_THIS) {
	MAKE_CONST_STRING(name, "this");	/* Should perhaps qualify it. */
      } else {
	int program_id = n->u.integer.a;
	struct program_state *state = Pike_compiler;

	while (state && (state->new_program->id != program_id)) {
	  state = state->previous;
	}

	if (state) {
	  struct identifier *id = ID_FROM_INT(state->new_program, id_no);
	  if (id && id->name) {
	    name = id->name;
#if 0
#ifdef PIKE_DEBUG
	    /* FIXME: This test crashes on valid code because the type of the
	     * identifier can change in pass 2 -Hubbe
	     */
	    if(id->type != f)
	    {
	      printf("Type of external node is not matching it's identifier.\nid->type: ");
	      simple_describe_type(id->type);
	      printf("\nf       : ");
	      simple_describe_type(f);
	      printf("\n");
	      
	      Pike_fatal("Type of external node is not matching it's identifier.\n");
	    }
#endif
#endif
	  }
	}
	if (!name) {
	  MAKE_CONST_STRING(name, "external symbol");
	}
      }
    }
    break;

  case F_CAST:
  case F_SOFT_CAST:
    name = get_name_of_function(CAR(n));
    break;

  case F_TRAMPOLINE:
    MAKE_CONST_STRING(name, "trampoline function");
    break;

  case F_LOCAL:
    MAKE_CONST_STRING(name, "local variable");
    break;

  case F_APPLY:
    if ((CAR(n)->token == F_CONSTANT) &&
	(TYPEOF(CAR(n)->u.sval) == T_FUNCTION) &&
	(SUBTYPEOF(CAR(n)->u.sval) == FUNCTION_BUILTIN) &&
	(CAR(n)->u.sval.u.efun->function == debug_f_aggregate)) {
      if (CDR(n)) {
	n = CDR(n);
	while (n && (n->token == F_ARG_LIST)) n = CAR(n);
	if (n) {
	  /* FIXME: Should really join the names of all the args. */
	  name = get_name_of_function(n);
	} else {
	  MAKE_CONST_STRING(name, "dynamic array");
	}
      } else {
	MAKE_CONST_STRING(name, "empty array");
      }
    } else {
      MAKE_CONST_STRING(name, "returned value");
    }
    break;
	  
  default:
    /* fprintf(stderr, "Node token: %s(%d)\n",
       get_f_name(n->token), n->token); */
    MAKE_CONST_STRING(name, "unknown function");
  }
#ifdef PIKE_DEBUG
  if (!name) {
    Pike_fatal("Failed to get name of function.\n");
  }
#endif
  return name;
}

void fix_type_field(node *n)
{
  struct compilation *c = THIS_COMPILATION;
  struct pike_type *type_a, *type_b;
  struct pike_type *old_type;

  if (n->type && !(n->node_info & OPT_TYPE_NOT_FIXED))
    return; /* assume it is correct */

  old_type = n->type;
  n->type = 0;
  n->node_info &= ~OPT_TYPE_NOT_FIXED;

  /*
    These two are needed if we want to extract types
    from nodes while building the tree.
  */
  if( CAR(n) ) fix_type_field(CAR(n));
  if( CDR(n) ) fix_type_field(CDR(n));

  switch(n->token)
  {
  case F_SOFT_CAST:
    if (CAR(n) && CAR(n)->type) {
      struct pike_type *soft_type = NULL;
      if (CDR(n) && (CDR(n)->token == F_CONSTANT) &&
	  (TYPEOF(CDR(n)->u.sval) == T_TYPE)) {
	soft_type = CDR(n)->u.sval.u.type;
	if ((n->type = soft_cast(soft_type, CAR(n)->type, 0))) {
	  /* Success. */
	  break;
	}
	ref_push_type_value(CAR(n)->type);
	ref_push_type_value(soft_type);
	yytype_report(REPORT_ERROR, NULL, 0, NULL, NULL, 0, NULL,
		      2, "Soft cast of %O to %O isn't a valid cast.");
      } else {
	yytype_report(REPORT_ERROR, NULL, 0, type_type_string,
		      NULL, 0, CDR(n)->type, 0,
		      "Soft cast with non-type.");
      }
    }
    /* FALL_THROUGH */
  case F_CAST:
    /* Type-field is correct by definition. */
    copy_pike_type(n->type, old_type);
    break;

  case F_LAND:
  case F_LOR:
    if (!CAR(n) || CAR(n)->type == void_type_string) {
      yyerror("Conditional uses void expression.");
      copy_pike_type(n->type, mixed_type_string);
      break;
    }

    if(!match_types(CAR(n)->type, mixed_type_string))
      yyerror("Bad conditional expression.");

    if (!CDR(n) || CDR(n)->type == void_type_string)
      copy_pike_type(n->type, void_type_string);
    else if(n->token == F_LAND || CAR(n)->type == CDR(n)->type)
    {
      copy_pike_type(n->type, CDR(n)->type);
    }else{
      n->type = or_pike_types(CAR(n)->type, CDR(n)->type, 0);
    }
    break;

  case F_APPEND_MAPPING:
    if (!CAR(n) || (CAR(n)->type == void_type_string)) {
      yyerror("Assigning a void expression.");
      copy_pike_type(n->type, void_type_string);
    }
    else
      /* FIXME: Not really correct, should calculate type of RHS. */
      copy_pike_type(n->type, CAR(n)->type);
   break;

  case F_APPEND_ARRAY:
    if (!CAR(n) || (CAR(n)->type == void_type_string)) {
      yyerror("Assigning a void expression.");
      copy_pike_type(n->type, void_type_string);
    } else if (!CDR(n)) {
      copy_pike_type(n->type, CAR(n)->type);
    } else {
      struct pike_type *tmp;
      /* Ensure that the type-fields are up to date. */
      fix_type_field(CAR(n));
      fix_type_field(CDR(n));
      type_stack_mark();
      push_finished_type(CDR(n)->type);
      push_type(T_ARRAY);
      n->type = and_pike_types(CAR(n)->type, tmp = pop_unfinished_type());
      free_type(tmp);
    }
    break;

  case F_ASSIGN:
  case F_ASSIGN_SELF:
    if (!CAR(n) || (CAR(n)->type == void_type_string)) {
      yyerror("Assigning a void expression.");
      copy_pike_type(n->type, void_type_string);
    } else if (!CDR(n)) {
      copy_pike_type(n->type, CAR(n)->type);
    } else {
      /* Ensure that the type-fields are up to date. */
      fix_type_field(CAR(n));
      fix_type_field(CDR(n));
#if 0
      /* This test isn't sufficient, see below. */
      check_node_type(CAR(n), CDR(n)->type, "Bad type in assignment.");
#else /* !0 */
      if (!pike_types_le(CAR(n)->type, CDR(n)->type)) {
	/* a["b"]=c and a->b=c can be valid when a is an array.
	 *   
	 * FIXME: Exactly what case is the problem?
	 *	/grubba 2005-02-15
	 *
	 * Example:
	 *   array tmp = ({([]),([])});
	 *   tmp->foo = 7;		// Multi-assign.
	 *	/grubba 2007-04-27
	 */
	if (((CDR(n)->token != F_INDEX && CDR(n)->token != F_ARROW) ||
	     !((TEST_COMPAT (7, 6) && /* Bug compatibility. */
		match_types(array_type_string, CDR(n)->type)) ||
	       match_types(array_type_string, CADR(n)->type))) &&
	    !match_types(CDR(n)->type,CAR(n)->type)) {
	  yytype_report(REPORT_ERROR, NULL, 0, CDR(n)->type,
			NULL, 0, CAR(n)->type,
			0, "Bad type in assignment.");
	} else {
	  if (c->lex.pragmas & ID_STRICT_TYPES) {
	    struct pike_string *t1 = describe_type(CAR(n)->type);
	    struct pike_string *t2 = describe_type(CDR(n)->type);
#ifdef PIKE_DEBUG
	    if (l_flag > 0) {
	      fputs("Warning: Invalid assignment: ", stderr);
	      print_tree(n);
	    }
#endif /* PIKE_DEBUG */
	    yywarning("An expression of type %S cannot be assigned to "
		      "a variable of type %S.", t1, t2);
	    free_string(t2);
	    free_string(t1);
	  }
	  if (runtime_options & RUNTIME_CHECK_TYPES) {
	    _CAR(n) = mksoftcastnode(CDR(n)->type,
				     mkcastnode(mixed_type_string, CAR(n)));
	  }
	}
      }
#endif /* 0 */
      n->type = and_pike_types(CAR(n)->type, CDR(n)->type);
    }
    break;

  case F_ARRAY_LVALUE:
    {
      node *lval_list;
      if (!(lval_list = CAR(n))) {
	copy_pike_type(n->type, mixed_type_string);
      } else {
	struct pike_type *t;
	node *n2;

	if (lval_list->token == F_LVALUE_LIST) {
	  n2 = CAR(lval_list);
	} else {
	  n2 = lval_list;
	}

	if (n2) {
	  copy_pike_type(t, n2->type);
	} else {
	  copy_pike_type(t, zero_type_string);
	}
	while ((n2 != lval_list) && (lval_list = CDR(lval_list))) {
	  if (lval_list->token == F_LVALUE_LIST) {
	    n2 = CAR(lval_list);
	  } else {
	    n2 = lval_list;
	  }
	  if (n2) {
	    struct pike_type *tmp = or_pike_types(t, n2->type, 1);
	    free_type(t);
	    t = tmp;
	  }
	}
	type_stack_mark();
	push_finished_type(t);
	push_type(T_ARRAY);
	free_type(t);
	n->type = pop_unfinished_type();
      }
    }
    break;

  case F_INDEX:
  case F_ARROW:
    if (!CAR(n) || (CAR(n)->type == void_type_string)) {
      yyerror("Indexing a void expression.");
      /* The optimizer converts this to an expression returning 0. */
      copy_pike_type(n->type, zero_type_string);
    } else if (CDR(n)) {
      int valid;
      type_a=CAR(n)->type;
      type_b=CDR(n)->type;
      if((valid = check_indexing(type_a, type_b, n)) <= 0)
	if(!Pike_compiler->catch_level) {
	  yytype_report((!valid)?REPORT_ERROR:REPORT_WARNING,
			NULL, 0, NULL, NULL, 0, type_a,
			0, "Indexing on illegal type.");
	  ref_push_type_value(type_b);
	  low_yyreport((!valid)?REPORT_ERROR:REPORT_WARNING, NULL, 0,
		       type_check_system_string, 1,
		       "Index   : %O.");
	}
      n->type = index_type(type_a, type_b, n);
    } else {
      copy_pike_type(n->type, mixed_type_string);
    }
    break;

  case F_RANGE:
    if (!CAR(n)) {
      /* Unlikely to occur, and if it does, it has probably
       * already been complained about.
       */
      copy_pike_type(n->type, mixed_type_string);
    }
    else {
      node *low = CADR (n), *high = CDDR (n);
      n->type = range_type(CAR(n)->type,
			   ((low->token == F_RANGE_OPEN) || !CAR(low)) ?
			   NULL : CAR (low)->type,
			   ((high->token == F_RANGE_OPEN) || !CAR(high)) ?
			   NULL : CAR (high)->type);
    }
    break;

  case F_PUSH_ARRAY:
    if (CAR(n)) {
      struct pike_type *array_type;
      MAKE_CONSTANT_TYPE(array_type, tArr(tZero));
      if (!pike_types_le(array_type, CAR(n)->type)) {
	yytype_report(REPORT_ERROR, NULL, 0, array_type,
		      NULL, 0, CAR(n)->type,
		      0, "Bad argument to splice operator.");
      }
      free_type(array_type);
      /* FIXME: The type field of the splice operator is not yet utilized.
       *
       * It probably ought to be something similar to MANY(..., VOID).
       */
      n->type = index_type(CAR(n)->type, int_type_string, n);
    } else {
      copy_pike_type(n->type, mixed_type_string);
    }
    break;

  case F_AUTO_MAP_MARKER:
    if (!CAR(n) || (CAR(n)->type == void_type_string)) {
      yyerror("Indexing a void expression.");
      /* The optimizer converts this to an expression returning 0. */
      copy_pike_type(n->type, zero_type_string);
    } else {
      type_a=CAR(n)->type;
      if(!match_types(type_a, array_type_string))
	if(!Pike_compiler->catch_level)
	  yytype_report(REPORT_ERROR,
			NULL, 0, array_type_string,
			NULL, 0, type_a,
			0, "[*] on non-array.");
      n->type=index_type(type_a, int_type_string, n);
    }
    break;

  case F_AUTO_MAP:
  case F_APPLY:
    if (!CAR(n) || (CAR(n)->type == void_type_string)) {
      yyerror("Calling a void expression.");
    } else {
      struct pike_type *f;	/* Expected type. */
      struct pike_type *s;	/* Actual type */
      struct pike_string *name = NULL;
      INT32 args;

      args = 0;
      name = get_name_of_function(CAR(n));

#ifdef PIKE_DEBUG
      if (l_flag>2)
	safe_pike_fprintf (stderr, "Checking call to %S at %S:%ld.\n", name,
			   n->current_file, (long)n->line_number);
#endif /* PIKE_DEBUG */

      /* NOTE: new_check_call() steals a reference from f! */
      copy_pike_type(f, CAR(n)->type);
      f = debug_malloc_pass(new_check_call(name, f, CDR(n), &args, 0));

      if (!f) {
	/* Errors have been generated. */
	copy_pike_type(n->type, mixed_type_string);
	break;
      }

      if ((n->type = new_get_return_type(dmalloc_touch(struct pike_type *, f),
					 0))) {
	/* Type/argument-check OK. */
	debug_malloc_touch(n->type);

	free_type(f);
	if(n->token == F_AUTO_MAP)
	{
	  push_finished_type(n->type);
	  push_type(T_ARRAY);
	  free_type(n->type);
	  n->type = pop_type();
	}
	break;
      }

      /* Too few arguments or similar. */
      copy_pike_type(n->type, mixed_type_string);

      if ((s = get_first_arg_type(dmalloc_touch(struct pike_type *, f),
				  CALL_NOT_LAST_ARG))) {
	yytype_report(REPORT_ERROR, NULL, 0, s,
		      NULL, 0, NULL,
		      0, "Too few arguments to %S (got %d).",
		      name, args);
	free_type(s);
	yytype_report(REPORT_ERROR, NULL, 0, NULL,
		      NULL, 0, CAR(n)->type,
		      0, "Function type:");
      } else {
	yytype_report(REPORT_ERROR, NULL, 0, function_type_string,
		      NULL, 0, f,
		      0, "Attempt to call a non function value %S.",
		      name);
      }
      free_type(f);
      break;
    }
    copy_pike_type(n->type, mixed_type_string);
    break;

  case '?':
    if (!CAR(n) || (CAR(n)->type == void_type_string)) {
      yyerror("Conditional expression is void.");
    } else if(!match_types(CAR(n)->type, mixed_type_string))
      yyerror("Bad conditional expression.");

    if(!CDR(n) || !CADR(n) || !CDDR(n) ||
       CADR(n)->type == void_type_string ||
       CDDR(n)->type == void_type_string)
    {
      copy_pike_type(n->type, void_type_string);
      break;
    }
    
    if(CADR(n)->type == CDDR(n)->type)
    {
      copy_pike_type(n->type, CADR(n)->type);
      break;
    }

    n->type = or_pike_types(CADR(n)->type, CDDR(n)->type, 0);
    break;

  case F_INC:
  case F_DEC:
  case F_POST_INC:
  case F_POST_DEC:
    if (CAR(n)) {
      /* The expression gets the type from the variable. */
      /* FIXME: Ought to strip non-applicable subtypes from the type. */
      copy_pike_type(n->type, CAR(n)->type);
    } else {
      copy_pike_type(n->type, mixed_type_string);
    }
    break;

  case F_RETURN:
    if (!CAR(n) || (CAR(n)->type == void_type_string)) {
      yywarning("Returning a void expression. Converted to zero.");
      if (!CAR(n)) {
	_CAR(n) = mkintnode(0);
	copy_pike_type(n->type, CAR(n)->type);
      } else {
	_CAR(n) = mknode(F_COMMA_EXPR, CAR(n), mkintnode(0));
	copy_pike_type(n->type, CDAR(n)->type);
      }
      break;
    } else if(Pike_compiler->compiler_frame &&
	      Pike_compiler->compiler_frame->current_return_type) {
      if ((Pike_compiler->compiler_frame->current_return_type !=
	   void_type_string) ||
	  (CAR(n)->token != F_CONSTANT) ||
	  !SAFE_IS_ZERO(& CAR(n)->u.sval)) {
	check_node_type(CAR(n),
			Pike_compiler->compiler_frame->current_return_type,
			"Wrong return type.");
      }
    }
    copy_pike_type(n->type, void_type_string);
    break;

  case F_CASE_RANGE:
    if (CDR(n) && CAR(n)) {
      fix_type_field(CAR(n));
      fix_type_field(CDR(n));
      /* case 1 .. 2: */
      if (!match_types(CAR(n)->type, CDR(n)->type)) {
	if (!match_types(CAR(n)->type, int_type_string) ||
	    !match_types(CDR(n)->type, int_type_string)) {
	  yytype_report(REPORT_ERROR,
			NULL, 0, CAR(n)->type,
			NULL, 0, CDR(n)->type,
			0, "Type mismatch in case range.");
	}
      } else if ((c->lex.pragmas & ID_STRICT_TYPES) &&
		 (CAR(n)->type != CDR(n)->type)) {
	/* The type should be the same for both CAR & CDR. */
	if (!pike_types_le(CDR(n)->type, CAR(n)->type)) {
	  /* Note that zero should be handled as int(0..0) here. */
	  if (!(CAR(n)->type == zero_type_string) ||
	      !(pike_types_le(CDR(n)->type, int_type_string))) {
	    yytype_report(REPORT_ERROR,
			  NULL, 0, CAR(n)->type,
			  NULL, 0, CDR(n)->type,
			  0, "Type mismatch in case range.");
	  }
	} else if (!pike_types_le(CAR(n)->type, CDR(n)->type)) {
	  if (!(CDR(n)->type == zero_type_string) ||
	      !(pike_types_le(CAR(n)->type, int_type_string))) {
	    yytype_report(REPORT_WARNING,
			  NULL, 0, CAR(n)->type,
			  NULL, 0, CDR(n)->type,
			  0, "Type mismatch in case range.");
	  }
	}
      }
    }
    if (CDR(n) && (Pike_compiler->compiler_pass == 2)) {
      fix_type_field(CDR(n));
      if (!match_types(CDR(n)->type, enumerable_type_string)) {
	yytype_report(REPORT_WARNING,
		      NULL, 0, enumerable_type_string,
		      NULL, 0, CDR(n)->type,
		      0, "Case value is not an enumerable type.");
      }
    }
    /* FALL_THROUGH */
  case F_CASE:
    if (CAR(n) && (Pike_compiler->compiler_pass == 2)) {
      fix_type_field(CAR(n));
      if (!match_types(CAR(n)->type, enumerable_type_string)) {
	yytype_report(REPORT_WARNING,
		      NULL, 0, enumerable_type_string,
		      NULL, 0, CAR(n)->type,
		      0, "Case value is not an enumerable type.");
      }
    }
    /* FALL_THROUGH */
  case F_INC_LOOP:
  case F_DEC_LOOP:
  case F_DEC_NEQ_LOOP:
  case F_INC_NEQ_LOOP:
  case F_LOOP:
  case F_CONTINUE:
  case F_BREAK:
  case F_DEFAULT:
  case F_POP_VALUE:
    copy_pike_type(n->type, void_type_string);
    break;

  case F_DO:
    if (!CDR(n) || (CDR(n)->type == void_type_string)) {
      yyerror("do - while(): Conditional expression is void.");
    } else if(!match_types(CDR(n)->type, mixed_type_string))
      yyerror("Bad conditional expression do - while().");
    copy_pike_type(n->type, void_type_string);
    break;
    
  case F_FOR:
    if (!CAR(n) || (CAR(n)->type == void_type_string)) {
      yyerror("for(): Conditional expression is void.");
    } else if(!match_types(CAR(n)->type, mixed_type_string))
      yyerror("Bad conditional expression for().");
    copy_pike_type(n->type, void_type_string);
    break;

  case F_SWITCH:
    if (!CAR(n) || (CAR(n)->type == void_type_string)) {
      yyerror("switch(): Conditional expression is void.");
    } else if(!match_types(CAR(n)->type, mixed_type_string))
      yyerror("Bad switch expression.");
    copy_pike_type(n->type, void_type_string);
    break;

  case F_CONSTANT:
    n->type = get_type_of_svalue(&n->u.sval);
    break;

  case F_FOREACH:
    if (!CAR(n) || (CAR(n)->token != F_VAL_LVAL)) {
      yyerror("foreach(): No expression to loop over.");
    } else {
      if (!CAAR(n) || pike_types_le(CAAR(n)->type, void_type_string)) {
	yyerror("foreach(): Looping over a void expression.");
      } else {
	if(CDAR(n) && CDAR(n)->token == ':')
	{
	  /* Check the iterator type */
	  struct pike_type *iterator_type;
	  struct pike_type *foreach_call_type;
	  MAKE_CONSTANT_TYPE(iterator_type,
			     tOr5(tArray, tStr, tObj,
				  tMapping, tMultiset));
	  if (!check_node_type(CAAR(n), iterator_type,
			       "Bad argument 1 to foreach()")) {
	    /* No use checking the index and value types if
	     * the iterator type is bad.
	     */
	    free_type(iterator_type);
	    goto foreach_type_check_done;
	  }
	  free_type(iterator_type);

	  push_type(T_MIXED);
	  push_type(T_VOID);
	  push_type(T_MANY);
	  push_finished_type(CAAR(n)->type);
	  push_type(T_FUNCTION);

	  foreach_call_type = pop_type();

	  if (CADAR(n)) {
	    /* Check the index type */
	    struct pike_type *index_fun_type;
	    struct pike_type *index_type;
	    MAKE_CONSTANT_TYPE(index_fun_type,
			       tOr4(tFunc(tOr(tArray, tStr), tZero),
				    tFunc(tMap(tSetvar(0, tMix),
					       tMix), tVar(0)),
				    tFunc(tSet(tSetvar(1, tMix)),
					  tVar(1)),
				    tFunc(tObj, tZero)));
	    index_type = check_call(foreach_call_type, index_fun_type, 0);
	    if (!index_type) {
	      /* Should not happen. */
	      yytype_report(REPORT_ERROR,
			    NULL, 0, NULL,
			    NULL, 0, NULL,
			    0, "Bad iterator type for index in foreach().");
	    } else {
	      if (!pike_types_le(index_type, CADAR(n)->type)) {
		int level = REPORT_NOTICE;
		if (!match_types(CADAR(n)->type, index_type)) {
		  level = REPORT_ERROR;
		} else if (c->lex.pragmas & ID_STRICT_TYPES) {
		  level = REPORT_WARNING;
		}
		yytype_report(level,
			      NULL, 0, index_type,
			      NULL, 0, CADAR(n)->type,
			      0, "Type mismatch for index in foreach().");
	      }
	      free_type(index_type);
	    }
	    free_type(index_fun_type);
	  }
	  if (CDDAR(n)) {
	    /* Check the value type */
	    struct pike_type *value_fun_type;
	    struct pike_type *value_type;
	    MAKE_CONSTANT_TYPE(value_fun_type,
			       tOr5(tFunc(tArr(tSetvar(0, tMix)),
					  tVar(0)),
				    tFunc(tStr, tZero),
				    tFunc(tMap(tMix,tSetvar(1, tMix)),
					  tVar(1)),
				    tFunc(tMultiset, tInt1),
				    tFunc(tObj, tZero)));
	    value_type = check_call(foreach_call_type, value_fun_type, 0);
	    if (!value_type) {
	      /* Should not happen. */
	      yytype_report(REPORT_ERROR,
			    NULL, 0, NULL,
			    NULL, 0, NULL,
			    0, "Bad iterator type for value in foreach().");
	    } else {
	      if (!pike_types_le(value_type, CDDAR(n)->type)) {
		int level = REPORT_NOTICE;
		if (!match_types(CDDAR(n)->type, value_type)) {
		  level = REPORT_ERROR;
		} else if (c->lex.pragmas & ID_STRICT_TYPES) {
		  level = REPORT_WARNING;
		}
		yytype_report(level,
			      NULL, 0, value_type,
			      NULL, 0, CDDAR(n)->type,
			      0, "Type mismatch for value in foreach().");
	      }
	      free_type(value_type);
	    }
	    free_type(value_fun_type);
	  }
	  free_type(foreach_call_type);
	} else {
	  /* Old-style foreach */
	  struct pike_type *array_zero;
	  MAKE_CONSTANT_TYPE(array_zero, tArr(tZero));
	  
	  if (!pike_types_le(array_zero, CAAR(n)->type)) {
	    yytype_report(REPORT_ERROR,
			  NULL, 0, array_zero,
			  NULL, 0, CAAR(n)->type,
			  0, "Bad argument 1 to foreach().");
	  } else {
	    if ((c->lex.pragmas & ID_STRICT_TYPES) &&
		!pike_types_le(CAAR(n)->type, array_type_string)) {
	      yytype_report(REPORT_WARNING,
			    NULL, 0, CAAR(n)->type,
			    NULL, 0, array_type_string,
			    0,
			    "Argument 1 to foreach() is not always an array.");
	    }
	    
	    if (!CDAR(n)) {
	      /* No loop variable. Will be converted to a counted loop
	       * by treeopt. */
	    } else if (pike_types_le(CDAR(n)->type, void_type_string)) {
	      yyerror("Bad argument 2 to foreach().");
	    } else {
	      struct pike_type *array_value_type;

	      type_stack_mark();
	      push_finished_type(CDAR(n)->type);
	      push_type(T_ARRAY);
	      array_value_type = pop_unfinished_type();

	      check_node_type(CAAR(n), array_value_type,
			      "Bad argument 1 to foreach().");
	      free_type(array_value_type);
	    }
	  }
	  free_type(array_zero);
	}
      }
    }
  foreach_type_check_done:
    copy_pike_type(n->type, void_type_string);
    break;

  case F_SSCANF:
    if (!CAR(n) || (CAR(n)->token != ':') ||
	!CDAR(n) || (CDAR(n)->token != F_ARG_LIST) ||
	!CADAR(n) || !CDDAR(n)) {
      yyerror("Too few arguments to sscanf().");
      MAKE_CONSTANT_TYPE(n->type, tIntPos);
    } else {
      struct pike_string *sscanf_name;
      struct pike_type *sscanf_type;
      node *args;
      INT32 argno = 0;
      if (CAAR(n)->u.sval.u.integer & SSCANF_FLAG_76_COMPAT) {
	MAKE_CONST_STRING(sscanf_name, "sscanf_76");
	add_ref(sscanf_type = sscanf_76_type_string);
      } else {
	MAKE_CONST_STRING(sscanf_name, "sscanf");
	add_ref(sscanf_type = sscanf_type_string);
      }	
      args = mknode(F_ARG_LIST, CDAR(n), CDR(n));
      add_ref(CDAR(n));
      if (CDR(n)) add_ref(CDR(n));
      sscanf_type = new_check_call(sscanf_name, sscanf_type, args, &argno, 0);
      free_node(args);
      if (sscanf_type) {
	if (!(n->type = new_get_return_type(sscanf_type, 0))) {
	  struct pike_type *expected;
	  if ((expected = get_first_arg_type(sscanf_type, CALL_NOT_LAST_ARG))) {
	    yytype_report(REPORT_ERROR,
			  NULL, 0, expected,
			  NULL, 0, NULL,
			  0, "Too few arguments to %S (got %d).",
			  sscanf_name, argno);
	    free_type(expected);
	  } else {
	    /* Most likely not reached. */
	    yytype_report(REPORT_ERROR,
			  NULL, 0, function_type_string,
			  NULL, 0, sscanf_type,
			  0, "Attempt to call a non function value %S.",
			  sscanf_name);
	  }
	}
	free_type(sscanf_type);
      }
      if (!n->type) {
	MAKE_CONSTANT_TYPE(n->type, tIntPos);
      }
    }
    break;

  case F_UNDEFINED:
    copy_pike_type(n->type, zero_type_string);
    break;

  case F_ARG_LIST:
    if (n->parent) {
      /* Propagate the changed type all the way up to the apply node. */
      n->parent->node_info |= OPT_TYPE_NOT_FIXED;
    }
    /* FALL_THROUGH */
  case F_COMMA_EXPR:
    if(!CAR(n) || CAR(n)->type==void_type_string)
    {
      if(CDR(n))
	copy_pike_type(n->type, CDR(n)->type);
      else
	copy_pike_type(n->type, void_type_string);
      break;
    }

    if(!CDR(n) || CDR(n)->type == void_type_string)
    {
      if(CAR(n))
	copy_pike_type(n->type, CAR(n)->type);
      else
	copy_pike_type(n->type, void_type_string);
      break;
    }
    if (n->token == F_ARG_LIST) {
      n->type = or_pike_types(CAR(n)->type, CDR(n)->type, 0);
    } else {
      copy_pike_type(n->type, CDR(n)->type);
    }
    break;

  case F_MAGIC_INDEX:
    /* FIXME: Could have a stricter type for ::`->(). */
    /* FIXME: */
    MAKE_CONSTANT_TYPE(n->type, tFunc(tStr tOr3(tVoid,tObj,tDeprecated(tInt))
				      tOr(tVoid,tInt), tMix));
    break;
  case F_MAGIC_SET_INDEX:
    /* FIXME: Could have a stricter type for ::`->=(). */
    /* FIXME: */
    MAKE_CONSTANT_TYPE(n->type, tFunc(tMix tSetvar(0,tMix) tOr(tVoid,tInt), tVar(0)));
    break;
  case F_MAGIC_INDICES:
    MAKE_CONSTANT_TYPE(n->type, tFunc(tOr(tVoid,tInt), tArr(tString)));
    break;
  case F_MAGIC_VALUES:
    /* FIXME: Could have a stricter type for ::_values. */
    MAKE_CONSTANT_TYPE(n->type, tFunc(tOr(tVoid,tInt), tArray));
    break;
  case F_MAGIC_TYPES:
    /* FIXME: Could have a stricter type for ::_types. */
    MAKE_CONSTANT_TYPE(n->type, tFunc(tOr(tVoid,tInt), tArr(tType(tMix))));
    break;

  case F_CATCH:
    /* FALL_THROUGH */
  default:
    copy_pike_type(n->type, mixed_type_string);
  }

  if (n->type != old_type) {
    if (n->parent) {
      n->parent->node_info |= OPT_TYPE_NOT_FIXED;
    }
  }
  if (old_type) {
    free_type(old_type);
  }
#ifdef PIKE_DEBUG
  check_type_string(n->type);
#endif /* PIKE_DEBUG */
}

static void zapp_try_optimize(node *n)
{
  node *parent;
  node *orig_n = n;

  if(!n) return;

  parent = n->parent;
  n->parent = NULL;

  while(1) {
    n->node_info &= ~OPT_TRY_OPTIMIZE;
    n->tree_info &= ~OPT_TRY_OPTIMIZE;

    if (car_is_node(n)) {
      CAR(n)->parent = n;
      n = CAR(n);
      continue;
    }
    if (cdr_is_node(n)) {
      CDR(n)->parent = n;
      n = CDR(n);
      continue;
    }
    while (n->parent && 
	   (!cdr_is_node(n->parent) || (CDR(n->parent) == n))) {
      n = n->parent;
    }
    if (n->parent && cdr_is_node(n->parent)) {
      CDR(n->parent)->parent = n->parent;
      n = CDR(n->parent);
      continue;
    }
    break;
  }
#ifdef PIKE_DEBUG
  if (n != orig_n) {
    Pike_fatal("zzap_try_optimize() lost track of parent.\n");
  }
#endif /* PIKE_DEBUG */
  n->parent = parent;
}

#if defined(SHARED_NODES) && 0
/* FIXME: Ought to use parent pointer to avoid recursion. */
static void find_usage(node *n, unsigned char *usage,
		       unsigned char *switch_u,
		       const unsigned char *cont_u,
		       const unsigned char *break_u,
		       const unsigned char *catch_u)
{
  if (!n)
    return;

  fatal_check_c_stack(16384);

  switch(n->token) {
  case F_ASSIGN:
  case F_ASSIGN_SELF:
    if ((CDR(n)->token == F_LOCAL) && (!CDR(n)->u.integer.b)) {
      usage[CDR(n)->u.integer.a] = 0;
    } else if (CDR(n)->token == F_ARRAY_LVALUE) {
      find_usage(CDR(n), usage, switch_u, cont_u, break_u, catch_u);
    }
    find_usage(CAR(n), usage, switch_u, cont_u, break_u, catch_u);
    return;

  case F_SSCANF:
    {
      int i;

      /* catch_usage is restored if sscanf throws an error. */
      for (i=0; i < MAX_LOCAL; i++) {
	usage[i] |= catch_u[i];
      }
      /* Only the first two arguments are evaluated. */
      if (CAR(n) && CDAR(n)) {
	find_usage(CDDAR(n), usage, switch_u, cont_u, break_u, catch_u);
	find_usage(CADAR(n), usage, switch_u, cont_u, break_u, catch_u);
      }
      return;
    }

  case F_CATCH:
    {
      unsigned char catch_usage[MAX_LOCAL];
      int i;

      memcpy(catch_usage, usage, MAX_LOCAL);
      find_usage(CAR(n), usage, switch_u, cont_u, catch_usage, catch_usage);
      for(i=0; i < MAX_LOCAL; i++) {
	usage[i] |= catch_usage[i];
      }
      return;
    }

  case F_AUTO_MAP:
  case F_APPLY:
    {
      int i;

      /* catch_usage is restored if the function throws an error. */
      for (i=0; i < MAX_LOCAL; i++) {
	usage[i] |= catch_u[i];
      }
      find_usage(CDR(n), usage, switch_u, cont_u, break_u, catch_u);
      find_usage(CAR(n), usage, switch_u, cont_u, break_u, catch_u);
      return;
    }

  case F_LVALUE_LIST:
    find_usage(CDR(n), usage, switch_u, cont_u, break_u, catch_u);
    if (CAR(n)) {
      if ((CAR(n)->token == F_LOCAL) && (!CAR(n)->u.integer.b)) {
	usage[CAR(n)->u.integer.a] = 0;
      }
    }
    return;

  case F_ARRAY_LVALUE:
    find_usage(CAR(n), usage, switch_u, cont_u, break_u, catch_u);
    return;

  case F_CONTINUE:
    memcpy(usage, cont_u, MAX_LOCAL);
    return;

  case F_BREAK:
    memcpy(usage, break_u, MAX_LOCAL);
    return;

  case F_DEFAULT:
  case F_CASE:
  case F_CASE_RANGE:
    {
      int i;

      find_usage(CDR(n), usage, switch_u, cont_u, break_u, catch_u);
      find_usage(CAR(n), usage, switch_u, cont_u, break_u, catch_u);
      for(i = 0; i < MAX_LOCAL; i++) {
	switch_u[i] |= usage[i];
      }
      return;
    }

  case F_SWITCH:
    {
      unsigned char break_usage[MAX_LOCAL];
      unsigned char switch_usage[MAX_LOCAL];
      int i;

      memset(switch_usage, 0, MAX_LOCAL);
      memcpy(break_usage, usage, MAX_LOCAL);

      find_usage(CDR(n), usage, switch_usage, cont_u, break_usage, catch_u);

      for(i = 0; i < MAX_LOCAL; i++) {
	usage[i] |= switch_usage[i];
      }

      find_usage(CAR(n), usage, switch_u, cont_u, break_u, catch_u);
      return;
    }

  case F_RETURN:
    memset(usage, 0, MAX_LOCAL);
    /* FIXME: The function arguments should be marked "used", since
     * they are seen in backtraces.
     */
    return;

  case F_LOR:
  case F_LAND:
    {
      unsigned char trail_usage[MAX_LOCAL];
      int i;

      memcpy(trail_usage, usage, MAX_LOCAL);

      find_usage(CDR(n), usage, switch_u, cont_u, break_u, catch_u);

      for(i=0; i < MAX_LOCAL; i++) {
	usage[i] |= trail_usage[i];
      }

      find_usage(CAR(n), usage, switch_u, cont_u, break_u, catch_u);
      return;
    }

  case '?':
    {
      unsigned char cadr_usage[MAX_LOCAL];
      unsigned char cddr_usage[MAX_LOCAL];
      int i;

      memcpy(cadr_usage, usage, MAX_LOCAL);
      memcpy(cddr_usage, usage, MAX_LOCAL);

      find_usage(CADR(n), cadr_usage, switch_u, cont_u, break_u, catch_u);
      find_usage(CDDR(n), cddr_usage, switch_u, cont_u, break_u, catch_u);

      for (i=0; i < MAX_LOCAL; i++) {
	usage[i] = cadr_usage[i] | cddr_usage[i];
      }
      find_usage(CAR(n), usage, switch_u, cont_u, break_u, catch_u);
      return;
    }
    
  case F_DO:
    {
      unsigned char break_usage[MAX_LOCAL];
      unsigned char continue_usage[MAX_LOCAL];

      memcpy(break_usage, usage, MAX_LOCAL);

      find_usage(CDR(n), usage, switch_u, cont_u, break_usage, catch_u);

      memcpy(continue_usage, usage, MAX_LOCAL);

      find_usage(CAR(n), usage, switch_u, break_usage, continue_usage,
		 catch_u);
      return;
    }

  case F_FOR:
    {
      unsigned char loop_usage[MAX_LOCAL];
      unsigned char break_usage[MAX_LOCAL];
      unsigned char continue_usage[MAX_LOCAL];
      int i;

      memcpy(break_usage, usage, MAX_LOCAL);

      /* for(;a;b) c; is handled like:
       *
       * if (a) { do { c; b; } while(a); }
       */

      memset(loop_usage, 0, MAX_LOCAL);

      find_usage(CAR(n), loop_usage, switch_u, cont_u, break_u, catch_u);
      if (CDR(n)) {
	find_usage(CDDR(n), loop_usage, switch_u, cont_u, break_usage,
		   catch_u);

	memcpy(continue_usage, loop_usage, MAX_LOCAL);

	find_usage(CADR(n), loop_usage, switch_u, continue_usage, break_usage,
		   catch_u);
      }

      for (i = 0; i < MAX_LOCAL; i++) {
	usage[i] |= loop_usage[i];
      }

      find_usage(CAR(n), usage, switch_u, cont_u, break_u, catch_u);
      return;
    }

  case F_FOREACH:
    {
      unsigned char loop_usage[MAX_LOCAL];
      unsigned char break_usage[MAX_LOCAL];
      unsigned char continue_usage[MAX_LOCAL];
      int i;
      
      memcpy(break_usage, usage, MAX_LOCAL);

      /* Find the usage from the loop */

      memset(loop_usage, 0, MAX_LOCAL);

      memcpy(continue_usage, usage, MAX_LOCAL);

      find_usage(CDR(n), loop_usage, switch_u, continue_usage, break_usage,
		 catch_u);
      if (CDAR(n)->token == F_LOCAL) {
	if (!(CDAR(n)->u.integer.b)) {
	  loop_usage[CDAR(n)->u.integer.a] = 0;
	}
      } else if (CDAR(n)->token == F_LVALUE_LIST) {
	find_usage(CDAR(n), loop_usage, switch_u, cont_u, break_u, catch_u);
      }

      for(i=0; i < MAX_LOCAL; i++) {
	usage[i] |= loop_usage[i];
      }

      find_usage(CAAR(n), usage, switch_u, cont_u, break_u, catch_u);
      return;
    }

  case F_LOCAL:
    /* Use of local variable. */
    if (!n->u.integer.b) {
      /* Recently used, and used at all */
      usage[n->u.integer.a] = 3;
    }
    return;

  default:
    if (cdr_is_node(n)) {
      find_usage(CDR(n), usage, switch_u, cont_u, break_u, catch_u);
    }
    if (car_is_node(n)) {
      find_usage(CAR(n), usage, switch_u, cont_u, break_u, catch_u);
    }
    return;
  }
}

/* Note: Always builds a new tree. */
static node *low_localopt(node *n,
			  unsigned char *usage,
			  unsigned char *switch_u,
			  const unsigned char *cont_u,
			  const unsigned char *break_u,
			  const unsigned char *catch_u)
{
  node *car, *cdr;

  if (!n)
    return NULL;
  
  switch(n->token) {
    /* FIXME: Does not support F_LOOP yet. */
  case F_ASSIGN:
  case F_ASSIGN_SELF:
    if ((CDR(n)->token == F_LOCAL) && (!CDR(n)->u.integer.b)) {
      /* Assignment of local variable */
      if (!(usage[CDR(n)->u.integer.a] & 1)) {
	/* Value isn't used. */
	struct pike_type *ref_type;
	MAKE_CONSTANT_TYPE(ref_type, tOr(tComplex, tString));
	if (!match_types(CDR(n)->type, ref_type)) {
	  /* The variable doesn't hold a refcounted value. */
	  free_type(ref_type);
	  return low_localopt(CAR(n), usage, switch_u, cont_u,
			      break_u, catch_u);
	}
	free_type(ref_type);
      }
      usage[CDR(n)->u.integer.a] = 0;
      cdr = CDR(n);
      ADD_NODE_REF(cdr);
    } else if (CDR(n)->token == F_ARRAY_LVALUE) {
      cdr = low_localopt(CDR(n), usage, switch_u, cont_u, break_u, catch_u);
    } else {
      cdr = CDR(n);
      ADD_NODE_REF(cdr);
    }
    return mknode(n->token, low_localopt(CAR(n), usage, switch_u, cont_u,
					 break_u, catch_u), cdr);

  case F_SSCANF:
    {
      int i;
      
      /* catch_usage is restored if sscanf throws an error. */
      for (i=0; i < MAX_LOCAL; i++) {
	usage[i] |= catch_u[i];
      }
      /* Only the first two arguments are evaluated. */
      if (CAR(n) && CDAR(n)) {
	cdr = low_localopt(CDDAR(n), usage, switch_u, cont_u, break_u, catch_u);
	car = low_localopt(CADAR(n), usage, switch_u, cont_u, break_u, catch_u);
	
	if (CDR(n)) {
	  ADD_NODE_REF(CDR(n));
	}
	return mknode(F_SSCANF, mknode(':', CAAR(n),
				       mknode(F_ARG_LIST, car, cdr)), CDR(n));
      }
      ADD_NODE_REF(n);
      return n;
    }

  case F_CATCH:
    {
      unsigned char catch_usage[MAX_LOCAL];
      int i;

      memcpy(catch_usage, usage, MAX_LOCAL);
      car = low_localopt(CAR(n), usage, switch_u, cont_u, catch_usage,
			 catch_usage);
      for(i=0; i < MAX_LOCAL; i++) {
	usage[i] |= catch_usage[i];
      }
      return mknode(F_CATCH, car, 0);
    }
    break;

  case F_AUTO_MAP:
  case F_APPLY:
    {
      int i;

      /* catch_usage is restored if the function throws an error. */
      for (i=0; i < MAX_LOCAL; i++) {
	usage[i] |= catch_u[i];
      }
      cdr = low_localopt(CDR(n), usage, switch_u, cont_u, break_u, catch_u);
      car = low_localopt(CAR(n), usage, switch_u, cont_u, break_u, catch_u);
      return mknode(n->token, car, cdr);
    }

  case F_LVALUE_LIST:
    cdr = low_localopt(CDR(n), usage, switch_u, cont_u, break_u, catch_u);
    if (CAR(n)) {
      if ((CAR(n)->token == F_LOCAL) && (!CAR(n)->u.integer.b)) {
	/* Array assignment of local variable. */
	if (!(usage[CDR(n)->u.integer.a] & 1)) {
	  /* Variable isn't used. */
	  /* FIXME: Warn? */
	}
	usage[CAR(n)->u.integer.a] = 0;
      }
      ADD_NODE_REF(CAR(n));
    }
    return mknode(F_LVALUE_LIST, CAR(n), cdr);

  case F_ARRAY_LVALUE:
    return mknode(F_ARRAY_LVALUE, low_localopt(CAR(n), usage, switch_u, cont_u,
					       break_u, catch_u), 0);

    

  case F_CAST:
    return mkcastnode(n->type, low_localopt(CAR(n), usage, switch_u, cont_u,
					    break_u, catch_u));

  case F_SOFT_CAST:
    return mksoftcastnode(n->type, low_localopt(CAR(n), usage, switch_u,
						cont_u, break_u, catch_u));

  case F_CONTINUE:
    memcpy(usage, cont_u, MAX_LOCAL);
    ADD_NODE_REF(n);
    return n;

  case F_BREAK:
    memcpy(usage, break_u, MAX_LOCAL);
    ADD_NODE_REF(n);
    return n;

  case F_DEFAULT:
  case F_CASE:
  case F_CASE_RANGE:
    {
      int i;

      cdr = low_localopt(CDR(n), usage, switch_u, cont_u, break_u, catch_u);
      car = low_localopt(CAR(n), usage, switch_u, cont_u, break_u, catch_u);
      for(i = 0; i < MAX_LOCAL; i++) {
	switch_u[i] |= usage[i];
      }
      return mknode(n->token, car, cdr);
    }

  case F_SWITCH:
    {
      unsigned char break_usage[MAX_LOCAL];
      unsigned char switch_usage[MAX_LOCAL];
      int i;

      memset(switch_usage, 0, MAX_LOCAL);
      memcpy(break_usage, usage, MAX_LOCAL);

      cdr = low_localopt(CDR(n), usage, switch_usage, cont_u, break_usage,
			 catch_u);

      for(i = 0; i < MAX_LOCAL; i++) {
	usage[i] |= switch_usage[i];
      }

      car = low_localopt(CAR(n), usage, switch_u, cont_u, break_u, catch_u);
      return mknode(F_SWITCH, car, cdr);
    }

  case F_RETURN:
    memset(usage, 0, MAX_LOCAL);
    /* FIXME: The function arguments should be marked "used", since
     * they are seen in backtraces.
     */
    return mknode(F_RETURN, low_localopt(CAR(n), usage, switch_u, cont_u,
					 break_u, catch_u), 0);

  case F_LOR:
  case F_LAND:
    {
      unsigned char trail_usage[MAX_LOCAL];
      int i;

      memcpy(trail_usage, usage, MAX_LOCAL);

      cdr = low_localopt(CDR(n), usage, switch_u, cont_u, break_u, catch_u);

      for(i=0; i < MAX_LOCAL; i++) {
	usage[i] |= trail_usage[i];
      }

      car = low_localopt(CAR(n), usage, switch_u, cont_u, break_u, catch_u);

      return mknode(n->token, car, cdr);
    }

  case '?':
    {
      unsigned char cadr_usage[MAX_LOCAL];
      unsigned char cddr_usage[MAX_LOCAL];
      int i;

      memcpy(cadr_usage, usage, MAX_LOCAL);
      memcpy(cddr_usage, usage, MAX_LOCAL);

      car = low_localopt(CADR(n), cadr_usage, switch_u, cont_u, break_u,
			 catch_u);
      cdr = low_localopt(CDDR(n), cddr_usage, switch_u, cont_u, break_u,
			 catch_u);

      for (i=0; i < MAX_LOCAL; i++) {
	usage[i] = cadr_usage[i] | cddr_usage[i];
      }
      cdr = mknode(':', car, cdr);
      car = low_localopt(CAR(n), usage, switch_u, cont_u, break_u, catch_u);
      return mknode('?', car, cdr);
    }
    
  case F_DO:
    {
      unsigned char break_usage[MAX_LOCAL];
      unsigned char continue_usage[MAX_LOCAL];
      int i;

      memcpy(break_usage, usage, MAX_LOCAL);

      /* Find the usage from the loop */
      find_usage(CDR(n), usage, switch_u, cont_u, break_u, catch_u);

      memcpy(continue_usage, usage, MAX_LOCAL);

      find_usage(CAR(n), usage, switch_u, continue_usage, break_usage,
		 catch_u);

      for (i = 0; i < MAX_LOCAL; i++) {
	usage[i] |= break_usage[i];
      }

      cdr = low_localopt(CDR(n), usage, switch_u, cont_u, break_usage,
			 catch_u);

      memcpy(continue_usage, usage, MAX_LOCAL);

      car = low_localopt(CAR(n), usage, switch_u, continue_usage, break_usage,
			 catch_u);

      return mknode(F_DO, car, cdr);
    }

  case F_FOR:
    {
      unsigned char loop_usage[MAX_LOCAL];
      unsigned char break_usage[MAX_LOCAL];
      unsigned char continue_usage[MAX_LOCAL];
      int i;

      memcpy(break_usage, usage, MAX_LOCAL);

      /*
       * if (a A|B) {
       *     B
       *   do {
       *       B
       *     c;
       *   continue:
       *       D
       *     b;
       *       C
       *   } while (a A|B);
       *     A
       * }
       * break:
       *   A
       */

      /* Find the usage from the loop. */

      memset(loop_usage, 0, MAX_LOCAL);

      find_usage(CAR(n), loop_usage, switch_u, cont_u, break_u, catch_u);
      if (CDR(n)) {
	find_usage(CDDR(n), loop_usage, switch_u, cont_u, break_usage,
		   catch_u);

	memcpy(continue_usage, loop_usage, MAX_LOCAL);

	find_usage(CADR(n), loop_usage, switch_u, continue_usage, break_usage,
		   catch_u);
      }

      for (i = 0; i < MAX_LOCAL; i++) {
	usage[i] |= loop_usage[i];
      }

      /* The last thing to be evaluated is the conditional */
      car = low_localopt(CAR(n), usage, switch_u, cont_u, break_u, catch_u);

      if (CDR(n)) {
	node *cadr, *cddr;

	/* The incrementor */
	cddr = low_localopt(CDDR(n), usage, switch_u, cont_u, break_usage,
			    catch_u);

	memcpy(continue_usage, usage, MAX_LOCAL);

	/* The body */
	cadr = low_localopt(CADR(n), usage, switch_u, continue_usage,
			    break_usage, catch_u);
	cdr = mknode(':', cadr, cddr);
      } else {
	cdr = 0;
      }

      for (i = 0; i < MAX_LOCAL; i++) {
	usage[i] |= break_usage[i];
      }

      /* The conditional is also the first thing to be evaluated. */
      find_usage(car, usage, switch_u, cont_u, break_u, catch_u);

      return mknode(F_FOR, car, cdr);
    }

  case F_FOREACH:
    {
      unsigned char loop_usage[MAX_LOCAL];
      unsigned char break_usage[MAX_LOCAL];
      unsigned char continue_usage[MAX_LOCAL];
      int i;

      memcpy(break_usage, usage, MAX_LOCAL);

      /*
       *   D
       * arr = copy_value(arr);
       * int i = 0;
       *   A|B
       * while (i < sizeof(arr)) {
       *     B
       *   loopvar = arr[i];
       *     C
       *   body;
       *   continue:
       *     A|B
       * }
       * break:
       *   A
       */

      /* Find the usage from the loop */
      memset(loop_usage, 0, MAX_LOCAL);

      memcpy(continue_usage, usage, MAX_LOCAL);

      find_usage(CDR(n), loop_usage, switch_u, continue_usage, break_usage,
		 catch_u);
      if (CDAR(n)->token == F_LOCAL) {
	if (!(CDAR(n)->u.integer.b)) {
	  loop_usage[CDAR(n)->u.integer.a] = 0;
	}
      } else if (CDAR(n)->token == F_LVALUE_LIST) {
	find_usage(CDAR(n), loop_usage, switch_u, cont_u, break_u, catch_u);
      }

      for (i = 0; i < MAX_LOCAL; i++) {
	usage[i] |= loop_usage[i];
      }

      memcpy(continue_usage, usage, MAX_LOCAL);
      cdr = low_localopt(CDR(n), usage, switch_u, continue_usage, break_usage,
			 catch_u);
      if (CDAR(n)->token == F_LOCAL) {
	if (!(CDAR(n)->u.integer.b)) {
	  usage[CDAR(n)->u.integer.a] = 0;
	}
      } else if (CDAR(n)->token == F_LVALUE_LIST) {
	find_usage(CDAR(n), usage, switch_u, cont_u, break_u, catch_u);
      }

      for (i = 0; i < MAX_LOCAL; i++) {
	usage[i] |= break_usage[i];
      }

      car = low_localopt(CAAR(n), usage, switch_u, cont_u, break_u, catch_u);
      ADD_NODE_REF(CDAR(n));
      return mknode(F_FOREACH, mknode(F_VAL_LVAL, car, CDAR(n)), cdr);
    }

  case F_LOCAL:
    /* Use of local variable. */
    if (!n->u.integer.b) {
      /* Recently used, and used at all */
      usage[n->u.integer.a] = 3;
    }
    ADD_NODE_REF(n);
    return n;

  default:
    if (cdr_is_node(n)) {
      cdr = low_localopt(CDR(n), usage, switch_u, cont_u, break_u, catch_u);
      return mknode(n->token, low_localopt(CAR(n), usage, switch_u, cont_u,
					   break_u, catch_u),
		    cdr);
    }
    if (car_is_node(n)) {
      ADD_NODE_REF(CDR(n));
      return mknode(n->token, low_localopt(CAR(n), usage, switch_u, cont_u,
					   break_u, catch_u),
		    CDR(n));
    }
    ADD_NODE_REF(n);
    return n;
  }
}

static node *localopt(node *n)
{
  unsigned char usage[MAX_LOCAL];
  unsigned char b_usage[MAX_LOCAL];
  unsigned char c_usage[MAX_LOCAL];
  unsigned char s_usage[MAX_LOCAL];
  unsigned char catch_usage[MAX_LOCAL];
  node *n2;

  memset(usage, 0, MAX_LOCAL);
  memset(b_usage, 0, MAX_LOCAL);
  memset(c_usage, 0, MAX_LOCAL);
  memset(s_usage, 0, MAX_LOCAL);
  memset(catch_usage, 0, MAX_LOCAL);

  n2 = low_localopt(n, usage, s_usage, c_usage, b_usage, catch_usage);

#ifdef PIKE_DEBUG
  if (l_flag > 0) {
    if ((n2 != n) || (l_flag > 4)) {
      fputs("\nBefore localopt: ", stderr);
      print_tree(n);

      fputs("After localopt:  ", stderr);
      print_tree(n2);
    }
  }
#endif /* PIKE_DEBUG */

  free_node(n);
  return n2;
}
#endif /* SHARED_NODES && 0 */

static void optimize(node *n)
{
  node *tmp1, *tmp2, *tmp3;
  struct compilation *c = THIS_COMPILATION;
  struct pike_string *save_file =
    dmalloc_touch(struct pike_string *, c->lex.current_file);
  INT_TYPE save_line = c->lex.current_line;

  do
  {
    if(car_is_node(n) && !(CAR(n)->node_info & OPT_OPTIMIZED))
    {
      CAR(n)->parent = n;
      n = CAR(n);
      continue;
    }
    if(cdr_is_node(n) && !(CDR(n)->node_info & OPT_OPTIMIZED))
    {
      CDR(n)->parent = n;
      n = CDR(n);
      continue;
    }

    c->lex.current_line = n->line_number;
    c->lex.current_file = dmalloc_touch(struct pike_string *, n->current_file);

    n->tree_info = n->node_info;
    if(car_is_node(n)) n->tree_info |= CAR(n)->tree_info;
    if(cdr_is_node(n)) n->tree_info |= CDR(n)->tree_info;

    if(!n->parent) break;
    
    if(n->tree_info & (OPT_NOT_CONST|
		       OPT_SIDE_EFFECT|
		       OPT_EXTERNAL_DEPEND|
		       OPT_ASSIGNMENT|
		       OPT_RETURN|
		       OPT_FLAG_NODE))
    {
      if(car_is_node(n) &&
	 !(CAR(n)->tree_info & (OPT_NOT_CONST|
				OPT_SIDE_EFFECT|
				OPT_EXTERNAL_DEPEND|
				OPT_ASSIGNMENT|
				OPT_RETURN|
				OPT_FLAG_NODE)) &&
	 (CAR(n)->tree_info & OPT_TRY_OPTIMIZE) &&
	 CAR(n)->token != F_VAL_LVAL)
      {
	_CAR(n) = eval(CAR(n));
	if(CAR(n)) CAR(n)->parent = n;
	zapp_try_optimize(CAR(n)); /* avoid infinite loops */
	continue;
      }
      if(cdr_is_node(n) &&
	 !(CDR(n)->tree_info & (OPT_NOT_CONST|
				OPT_SIDE_EFFECT|
				OPT_EXTERNAL_DEPEND|
				OPT_ASSIGNMENT|
				OPT_RETURN|
				OPT_FLAG_NODE)) &&
	 (CDR(n)->tree_info & OPT_TRY_OPTIMIZE))
      {
	_CDR(n) = eval(CDR(n));
	if(CDR(n)) CDR(n)->parent = n;
	zapp_try_optimize(CDR(n)); /* avoid infinite loops */
	continue;
      }
    }
    if (!n->type || (n->node_info & OPT_TYPE_NOT_FIXED)) {
      fix_type_field(n);
    }
    debug_malloc_touch(n->type);

#ifdef PIKE_DEBUG
    if(l_flag > 3 && n)
    {
      fprintf(stderr,"Optimizing (tree info=%04x):",n->tree_info);
      print_tree(n);
    }
#endif    

    switch(n->token)
    {
/* Unfortunately GCC doesn't ignore #pragma clang yet. */
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wparentheses-equality"
#include "treeopt.h"
#pragma clang diagnostic pop
#else
#include "treeopt.h"
#endif
    use_car:
      ADD_NODE_REF2(CAR(n), tmp1 = CAR(n));
      goto use_tmp1;

    use_cdr:
      ADD_NODE_REF2(CDR(n), tmp1 = CDR(n));
      goto use_tmp1;

    zap_node:
      tmp1 = 0;
      goto use_tmp1;

    use_tmp1:
#ifdef PIKE_DEBUG
      if (l_flag > 4) {
	fputs("Optimized: ", stderr);
	print_tree(n);
	fputs("Result:    ", stderr);
	print_tree(tmp1);
      }
#endif /* PIKE_DEBUG */

      if(CAR(n->parent) == n)
	_CAR(n->parent) = tmp1;
      else
	_CDR(n->parent) = tmp1;

      if (!tmp1 || (tmp1->type != n->type)) {
	n->parent->node_info |= OPT_TYPE_NOT_FIXED;
      }

      if(tmp1)
	tmp1->parent = n->parent;
      else
	tmp1 = n->parent;

      free_node(n);
      n = tmp1;

#ifdef PIKE_DEBUG
      if(l_flag > 3)
      {
	fputs("Result:    ", stderr);
	print_tree(n);
      }
#endif    
      continue;

    }
    n->node_info |= OPT_OPTIMIZED;
    n=n->parent;
  }while(n);

  c->lex.current_line = save_line;
  c->lex.current_file = dmalloc_touch(struct pike_string *, save_file);
}

void optimize_node(node *n)
{
  if(n &&
     Pike_compiler->compiler_pass==2 &&
     (n->node_info & OPT_TRY_OPTIMIZE))
  {
    optimize(n);
    check_tree(n,0);
  }
}

struct timer_oflo
{
  INT32 counter;
  int yes;
};

static void check_evaluation_time(struct callback *UNUSED(cb), void *tmp, void *UNUSED(ignored))
{
  struct timer_oflo *foo=(struct timer_oflo *)tmp;
  if(foo->counter-- < 0)
  {
    foo->yes=1;
    pike_throw();
  }
}

ptrdiff_t eval_low(node *n,int print_error)
{
  unsigned INT16 num_strings, num_constants;
  unsigned INT32 num_program;
  size_t jump;
  struct svalue *save_sp = Pike_sp;
  ptrdiff_t ret;
  struct program *prog = Pike_compiler->new_program;
#ifdef PIKE_USE_MACHINE_CODE
  size_t num_relocations;
#endif /* PIKE_USE_MACHINE_CODE */

#ifdef PIKE_DEBUG
  if(l_flag > 3 && n)
  {
    fprintf(stderr,"Evaluating (tree info=%x):",n->tree_info);
    print_tree(n);
  }
#endif

  fix_type_field(n);

  if(Pike_compiler->num_parse_error) {
    return -1;
  }

  num_strings = prog->num_strings;
  num_constants = prog->num_constants;
  num_program = prog->num_program;
#ifdef PIKE_USE_MACHINE_CODE
  num_relocations = prog->num_relocations;
#endif /* PIKE_USE_MACHINE_CODE */

  jump = docode(dmalloc_touch(node *, n));

  ret=-1;
  if(!Pike_compiler->num_parse_error)
  {
    struct callback *tmp_callback;
    struct timer_oflo foo;

    /* This is how long we try to optimize before giving up... */
    foo.counter=10000;
    foo.yes=0;

#ifdef PIKE_USE_MACHINE_CODE
    make_area_executable ((char *) (prog->program + num_program),
			  (prog->num_program - num_program) *
			  sizeof (prog->program[0]));
#endif

    tmp_callback=add_to_callback(&evaluator_callbacks,
				 check_evaluation_time,
				 (void *)&foo,0);
				 
    if(apply_low_safe_and_stupid(Pike_compiler->fake_object, jump))
    {
      /* Assume the node will throw errors at runtime too. */
      n->tree_info |= OPT_SIDE_EFFECT;
      n->node_info |= OPT_SIDE_EFFECT;
      if(print_error)
	/* Generate error message */
	if(!Pike_compiler->catch_level)
	  handle_compile_exception("Error evaluating constant.");
	else {
	  free_svalue(&throw_value);
	  mark_free_svalue (&throw_value);
	}
      else {
	free_svalue(&throw_value);
	mark_free_svalue (&throw_value);
      }
    }else{
      if(foo.yes)
	pop_n_elems(Pike_sp-save_sp);
      else
	ret=Pike_sp-save_sp;
      n->tree_info |= OPT_SAFE;
    }

    remove_callback(tmp_callback);
  }

  while(prog->num_strings > num_strings)
  {
    prog->num_strings--;
    free_string(prog->strings[prog->num_strings]);
  }

  while(prog->num_constants > num_constants)
  {
    struct program_constant *p_const;

    prog->num_constants--;

    p_const = prog->constants + prog->num_constants;

    free_svalue(&p_const->sval);
#if 0
    if (p_const->name) {
      free_string(p_const->name);
      p_const->name = NULL;
    }
#endif /* 0 */
  }

#ifdef PIKE_USE_MACHINE_CODE
  prog->num_relocations = num_relocations;

#ifdef VALGRIND_DISCARD_TRANSLATIONS
  /* We won't use this machine code any more... */
  VALGRIND_DISCARD_TRANSLATIONS(prog->program + num_program,
				(prog->num_program - num_program)*sizeof(PIKE_OPCODE_T));
#endif /* VALGRIND_DISCARD_TRANSLATIONS */
#endif /* PIKE_USE_MACHINE_CODE */

  prog->num_program = num_program;

#ifdef PIKE_DEBUG
  if(l_flag > 3 && n)
  {
    fprintf(stderr,"Evaluation ==> %"PRINTPTRDIFFT"d\n", ret);
  }
#endif

  return ret;
}

static node *eval(node *n)
{
  node *new;
  ptrdiff_t args;

  if(!is_const(n) || n->node_info & OPT_FLAG_NODE)
    return n;
  
  args=eval_low(n,0);

  switch(args)
  {
  case -1:
    return n;
    break;

  case 0:
    if(Pike_compiler->catch_level) return n;
    free_node(n);
    n=0;
    break;

  case 1:
    if(Pike_compiler->catch_level && SAFE_IS_ZERO(Pike_sp-1))
    {
      pop_stack();
      return n;
    }
    if (n->token == F_SOFT_CAST) {
      new = mksoftcastnode(n->type, mksvaluenode(Pike_sp-1));
    } else {
      new = mksvaluenode(Pike_sp-1);
      if (n->type && (!new->type || ((n->type != new->type) &&
				     pike_types_le(n->type,new->type)))) {
	if (new->type)
	  free_type(new->type);
	copy_pike_type(new->type, n->type);
      }
    }
    free_node(n);
    n = new;
    pop_stack();
    break;

  default:
    if (n->token != F_SOFT_CAST) {
      free_node(n);
      n=NULL;
      while(args--)
      {
	n=mknode(F_ARG_LIST,mksvaluenode(Pike_sp-1),n);
	pop_stack();
      }
    } else {
      node *nn = n;
      n = NULL;
      while(args--)
      {
	n=mknode(F_ARG_LIST,mksvaluenode(Pike_sp-1),n);
	pop_stack();
      }
      n = mksoftcastnode(nn->type, n);
      free_node(nn);
    }
  }
  return dmalloc_touch(node *, n);
}

INT32 last_function_opt_info;

/* FIXME: Ought to use parent pointer to avoid recursion. */
static int stupid_args(node *n, int expected,int vargs)
{
  if(!n) return expected;

  fatal_check_c_stack(16384);

  switch(n->token)
  {
  case F_PUSH_ARRAY:
    if(!vargs) return -1;

    if(stupid_args(CAR(n), expected,vargs) == expected+1)
      return 65535;
    return -1;

  case F_ARG_LIST:
    expected=stupid_args(CAR(n), expected,vargs);
    if(expected==-1) return -1;
    return stupid_args(CDR(n), expected,vargs);
  case F_LOCAL:
    return (!n->u.integer.b && n->u.integer.a==expected) ? expected + 1 : -1;
  default:
    return -1;
  }
}

/* FIXME: Ought to use parent pointer to avoid recursion. */
static int is_null_branch(node *n)
{
  if(!n) return 1;

  fatal_check_c_stack(16384);

  if((n->token==F_CAST && n->type == void_type_string) ||
     n->token == F_POP_VALUE)
    return is_null_branch(CAR(n));
  if(n->token==F_ARG_LIST)
    return is_null_branch(CAR(n)) && is_null_branch(CDR(n));
  return 0;
}

static struct svalue *is_stupid_func(node *n,
				     int args,
				     int vargs,
				     struct pike_type *type)
{
  int tmp;
  while(1)
  {
    if(!n) return 0;

    if(n->token == F_ARG_LIST)
    {
      if(is_null_branch(CAR(n)))
	n=CDR(n);
      else
	n=CAR(n);
      continue;
    }

    if((n->token == F_CAST && n->type == void_type_string) ||
       n->token == F_POP_VALUE)
    {
      n=CAR(n);
      continue;
    }
    break;
  }

  if(!n || n->token != F_RETURN) return 0;
  n=CAR(n);

  if(!n || n->token != F_APPLY) return 0;

  tmp=stupid_args(CDR(n),0,vargs);
  if(!(vargs?tmp==65535:tmp==args)) return 0;

  n=CAR(n);
  if(!n || n->token != F_CONSTANT) return 0;

  if((count_arguments(n->type) < 0) == !vargs)
    return 0;
  
  if(minimum_arguments(type) < minimum_arguments(n->type))
    return 0;

  return &n->u.sval;
}

int dooptcode(struct pike_string *name,
	      node *n,
	      struct pike_type *type,
	      int modifiers)
{
  union idptr tmp;
  int args, vargs, ret;
  struct svalue *foo;

  CHECK_COMPILER();

  optimize_node(n);

  check_tree(n, 0);

#ifdef PIKE_DEBUG
  if(a_flag > 1)
    fprintf(stderr, "Doing function '%s' at %lx\n", name->str,
	    DO_NOT_WARN((unsigned long)PIKE_PC));
#endif

  args=count_arguments(type);
  if(args < 0) 
  {
    args=~args;
    vargs=IDENTIFIER_VARARGS;
  }else{
    vargs=0;
  }

  if(Pike_compiler->compiler_frame->lexical_scope & SCOPE_SCOPED)
    vargs|=IDENTIFIER_SCOPED;

  if(Pike_compiler->compiler_frame->lexical_scope & SCOPE_SCOPE_USED)
    vargs|=IDENTIFIER_SCOPE_USED;

#ifdef PIKE_DEBUG
  if(a_flag > 5)
    fprintf(stderr, "Extra identifier flags:0x%02x\n", vargs);
#endif

  if(Pike_compiler->compiler_pass==1)
  {
    tmp.offset=-1;
#ifdef PIKE_DEBUG
    if(a_flag > 4)
    {
      fputs("Making prototype (pass 1) for: ", stderr);
      print_tree(n);
    }
#endif
  }else{
#if defined(SHARED_NODES) && 0
    /* Try the local variable usage analyser. */
    n = localopt(n);
    /* Try optimizing some more. */
    optimize(n);
#endif /* SHARED_NODES && 0 */
    n = mknode(F_ARG_LIST, n, 0);
    
    if((foo=is_stupid_func(n, args, vargs, type)))
    {
      if(TYPEOF(*foo) == T_FUNCTION && SUBTYPEOF(*foo) == FUNCTION_BUILTIN)
      {
	tmp.c_fun=foo->u.efun->function;
	if(tmp.c_fun != f_destruct &&
	   tmp.c_fun != f_this_object &&
	   tmp.c_fun != f_backtrace)
	{
#ifdef PIKE_DEBUG
	  struct compilation *c = THIS_COMPILATION;

	  if(a_flag > 1)
	    fprintf(stderr,"%s:%ld: IDENTIFIER OPTIMIZATION %s == %s\n",
		    c->lex.current_file->str,
		    (long)c->lex.current_line,
		    name->str,
		    foo->u.efun->name->str);
#endif
	  ret=define_function(name,
			      type,
			      (unsigned INT16)modifiers,
			      (unsigned INT8)(IDENTIFIER_C_FUNCTION |
					      IDENTIFIER_HAS_BODY |
					      vargs),
			      &tmp,
			      foo->u.efun->flags);
	  free_node(n);
	  return ret;
	}
      }
    }

    tmp.offset=PIKE_PC;
    Pike_compiler->compiler_frame->num_args=args;
  
#ifdef PIKE_DEBUG
    if(a_flag > 2)
    {
      fputs("Coding: ", stderr);
      print_tree(n);
    }
#endif
    if(!Pike_compiler->num_parse_error)
    {
      extern int remove_clear_locals;
      remove_clear_locals=args;
      if(vargs) remove_clear_locals++;
      tmp.offset=do_code_block(n);
      remove_clear_locals=0x7fffffff;
    }
  }
  
  ret=define_function(name,
		      type,
		      (unsigned INT16)modifiers,
		      (unsigned INT8)(IDENTIFIER_PIKE_FUNCTION |
				      IDENTIFIER_HAS_BODY |
				      vargs),
		      Pike_compiler->num_parse_error?NULL:&tmp,
		      (unsigned INT16)
		      (Pike_compiler->compiler_frame->opt_flags));


#ifdef PIKE_DEBUG
  if(a_flag > 1)
    fprintf(stderr,"Identifer = %d\n",ret);
#endif

  free_node(n);
  return ret;
}
