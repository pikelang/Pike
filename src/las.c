/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
/**/
#include "global.h"
RCSID("$Id: las.c,v 1.105 1999/11/12 18:34:56 grubba Exp $");

#include "language.h"
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
#include "error.h"
#include "docode.h"
#include "main.h"
#include "pike_memory.h"
#include "operators.h"
#include "callback.h"
#include "pike_macros.h"
#include "peep.h"
#include "builtin_functions.h"
#include "cyclic.h"
#include "block_alloc.h"

#define LASDEBUG

int lasdebug=0;

static node *eval(node *);
static void optimize(node *n);

node *init_node = 0;
int num_parse_error;
int cumulative_parse_error=0;
extern char *get_type_name(int);

#define MAX_GLOBAL 2048

int car_is_node(node *n)
{
  switch(n->token)
  {
  case F_EXTERNAL:
  case F_IDENTIFIER:
  case F_TRAMPOLINE:
  case F_CONSTANT:
  case F_LOCAL:
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
  case F_IDENTIFIER:
  case F_TRAMPOLINE:
  case F_CONSTANT:
  case F_LOCAL:
  case F_CAST:
    return 0;

  default:
    return !!_CDR(n);
  }
}

#ifdef PIKE_DEBUG
void check_tree(node *n, int depth)
{
  if(!d_flag) return;
  if(!n) return;
  if(n->token==USHRT_MAX)
    fatal("Free node in tree.\n");

#ifdef SHARED_NODES
  check_node_hash(n);
#endif /* SHARED_NODES */

  if(d_flag<2) return;

  if(!(depth & 1023))
  {
    node *q;
    for(q=n->parent;q;q=q->parent)
      if(q->parent==n)
	fatal("Cyclic node structure found.\n");
  }
  depth++;

  if(car_is_node(n))
  {
    if(CAR(n)->parent != n)
      fatal("Parent is wrong.\n");

    check_tree(CAR(n),depth);
  }

  if(cdr_is_node(n))
  {
    if(CDR(n)->parent != n)
      fatal("Parent is wrong.\n");

    check_tree(CDR(n),depth);
  }
}
#endif

INT32 count_args(node *n)
{
  int a,b;
  check_tree(n,0);
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
    else
      return count_args(CAR(n));

  case F_CASE:
  case F_FOR:
  case F_DO:
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
    if(tmp1==-1 || tmp2==-2) return -1;
    if(tmp1 < tmp2) return tmp1;
    return tmp2;
  }

  case F_PUSH_ARRAY:
    return -1;

  default:
    if(n->type == void_type_string) return 0;
    return 1;
  }
}

struct pike_string *find_return_type(node *n)
{
  struct pike_string *a,*b;

  check_tree(n,0);

  if(!n) return 0;
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
    if(b && a!=b) return mixed_type_string;
    return a;
  }
  return b;
}


#define NODES 256

#undef BLOCK_ALLOC_NEXT
#define BLOCK_ALLOC_NEXT u.node.a

BLOCK_ALLOC(node_s, NODES);

#undef BLOCK_ALLOC_NEXT
#define BLOCK_ALLOC_NEXT next

#ifdef SHARED_NODES

struct node_hash_table node_hash;

static unsigned INT32 hash_node(node *n)
{
  return hashmem((unsigned char *)&(n->token),
		 sizeof(node) - OFFSETOF(node_s, token), sizeof(node));
}

static void add_node(node *n)
{
  unsigned INT32 hval = n->hash % node_hash.size;

  n->next = node_hash.table[hval];
  node_hash.table[hval] = n;
}

static void sub_node(node *n)
{
  node *prior;

  if (!node_hash.size) {
    return;
  }

  prior = node_hash.table[n->hash % node_hash.size];

  if (!prior) {
    return;
  }
  if (prior == n) {
    node_hash.table[n->hash % node_hash.size] = n->next;
  } else {
    while(prior && (prior->next != n)) {
      prior = prior->next;
    }
    if (!prior) {
      return;
    }
    prior->next = n->next;
  }
}

static node *freeze_node(node *orig)
{
  unsigned INT32 hash = hash_node(orig);
  node *n = node_hash.table[hash % node_hash.size];

  /* free_node() wants a correct hash */
  orig->hash = hash;

  if (orig->node_info & OPT_NOT_SHARED) {
    /* No need to have this node in the hash-table. */
    /* add_node(orig); */
    return check_node_hash(dmalloc_touch(node *, orig));
  }

  while (n) {
    if ((n->hash == hash) &&
	!MEMCMP(&(n->token), &(orig->token),
		sizeof(node) - OFFSETOF(node_s, token))) {
#ifdef PIKE_DEBUG
      if (n == orig) {
	fatal("Node to be frozen was already cold!\n");
      }
#endif /* PIKE_DEBUG */
      free_node(dmalloc_touch(node *, orig));
      n->refs++;
      return check_node_hash(dmalloc_touch(node *, n));
    }
    n = n->next;
  }
  add_node(dmalloc_touch(node *, orig));
  return check_node_hash(orig);
}

#else /* !SHARED_NODES */

#define freeze_node(X)	(X)

#endif /* SHARED_NODES */

void free_all_nodes(void)
{
  if(!compiler_frame)
  {
    node *tmp;
    struct node_s_block *tmp2;
    int e=0;

#ifndef PIKE_DEBUG
    if(cumulative_parse_error)
    {
#endif
      
      for(tmp2=node_s_blocks;tmp2;tmp2=tmp2->next) e+=NODES;
      for(tmp=free_node_ss;tmp;tmp=_CAR(tmp)) e--;
      if(e)
      {
	int e2=e;
	for(tmp2=node_s_blocks;tmp2;tmp2=tmp2->next)
	{
	  for(e=0;e<NODES;e++)
	  {
	    for(tmp=free_node_ss;tmp;tmp=_CAR(tmp))
	      if(tmp==tmp2->x+e)
		break;
	    
	    if(!tmp)
	    {
	      tmp=tmp2->x+e;
#if defined(PIKE_DEBUG)
	      if(!cumulative_parse_error)
	      {
		fprintf(stderr,"Free node at %p, (%s:%d) (token=%d).\n",
			tmp, tmp->current_file->str, tmp->line_number,
			tmp->token);

		debug_malloc_dump_references(tmp);

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
#ifdef SHARED_NODES
		tmp->hash = hash_node(tmp);

		fprintf(stderr, "Freeing node that had %d refs.\n", tmp->refs);
		/* Force the node to be freed. */
		tmp->refs = 1;
#endif /* SHARED_NODES */
		debug_malloc_touch(tmp->type);
		free_node(tmp);
	      }
	    }
	  }
	}
#if defined(PIKE_DEBUG)
	if(!cumulative_parse_error)
	  fatal("Failed to free %d nodes when compiling!\n",e2);
#endif
      }
#ifndef PIKE_DEBUG
    }
#endif
    free_all_node_s_blocks();
    cumulative_parse_error=0;

#ifdef SHARED_NODES
    MEMSET(node_hash.table, 0, sizeof(node *) * node_hash.size);
#endif /* SHARED_NODES */
  }
}

void debug_free_node(node *n)
{
  if(!n) return;
#ifdef PIKE_DEBUG
  if(l_flag>9)
    print_tree(n);

#ifdef SHARED_NODES
  {
    unsigned INT32 hash;
    if ((hash = hash_node(n)) != n->hash) {
      fprintf(stderr, "Hash-value is bad 0x%08x != 0x%08x\n", hash, n->hash);
      print_tree(n);
      fatal("token:%d, car:%p cdr:%p file:%s line:%d\n",
	    n->token, _CAR(n), _CDR(n), n->current_file->str, n->line_number);
    }
  }
#endif /* SHARED_NODES */
#endif /* PIKE_DEBUG */

#ifdef SHARED_NODES
  if (dmalloc_touch(node *, n) && --(n->refs)) {
    return;
  }
  sub_node(dmalloc_touch(node *, n));
#endif /* SHARED_NODES */

  switch(n->token)
  {
  case USHRT_MAX:
    fatal("Freeing node again!\n");
    break;

  case F_CONSTANT:
    free_svalue(&(n->u.sval));
    break;

  default:
    if(car_is_node(n)) free_node(CAR(n));
    if(cdr_is_node(n)) free_node(CDR(n));
  }
  n->token=USHRT_MAX;
  if(n->type) free_string(n->type);
  if(n->name) free_string(n->name);
#ifdef PIKE_DEBUG
  if(n->current_file) free_string(n->current_file);
#endif
  really_free_node_s(n);
}


node *debug_check_node_hash(node *n)
{
#if defined(PIKE_DEBUG) && defined(SHARED_NODES)
  if (n && (n->hash != hash_node(n))) {
    fprintf(stderr,"Bad node hash at %p, (%s:%d) (token=%d).\n",
	    n, n->current_file->str, n->line_number,
	    n->token);
    debug_malloc_dump_references(n);
    print_tree(n);
    fatal("Bad node hash!\n");
  }
#endif /* PIKE_DEBUG && SHARED_NODES */
  return n;
}

/* here starts routines to make nodes */
static node *debug_mkemptynode(void)
{
  node *res=alloc_node_s();

#ifdef SHARED_NODES
  MEMSET(res, 0, sizeof(node));
  res->hash = 0;  
  res->refs = 1;
#endif /* SHARED_NODES */

  res->token=0;
  res->line_number=lex.current_line;
#ifdef PIKE_DEBUG
  copy_shared_string(res->current_file, lex.current_file);
#endif
  res->type=0;
  res->name=0;
  res->node_info=0;
  res->tree_info=0;
  res->parent=0;
  return res;
}

#define mkemptynode()	dmalloc_touch(node *, debug_mkemptynode())

node *debug_mknode(short token, node *a, node *b)
{
  node *res;

#if defined(PIKE_DEBUG) && !defined(SHARED_NODES)
  if(b && a==b)
    fatal("mknode: a and be are the same!\n");
#endif    

  check_tree(a,0);
  check_tree(b,0);

  res = mkemptynode();
  _CAR(res) = dmalloc_touch(node *, a);
  _CDR(res) = dmalloc_touch(node *, b);
  res->node_info = 0;
  res->tree_info = 0;
  if(a) {
    a->parent = res;
  }
  if(b) {
    b->parent = res;
  }

  res->token = token;
  res->type = 0;

#ifdef SHARED_NODES
  {
    node *res2 = freeze_node(res);

    if (res2 != res) {
      return dmalloc_touch(node *, res2);
    }
  }
#endif /* SHARED_NODES */

  switch(token)
  {
  case F_CATCH:
    res->node_info |= OPT_SIDE_EFFECT;
    break;

  case F_APPLY:
    if(a && a->token == F_CONSTANT &&
       a->u.sval.type == T_FUNCTION &&
       a->u.sval.subtype == FUNCTION_BUILTIN)
    {
      res->node_info |= a->u.sval.u.efun->flags;
    }else{
      res->node_info |= OPT_SIDE_EFFECT | OPT_EXTERNAL_DEPEND; /* for now */
    }
    break;

  case F_MAGIC_INDEX:
  case F_MAGIC_SET_INDEX:
    res->node_info |= OPT_EXTERNAL_DEPEND;
    break;

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
    res->node_info |= OPT_CASE;
    break;

  case F_SSCANF:
    if(!b || count_args(b) == 0) break;
    /* fall through */

  case F_ASSIGN:
  case F_INC:
  case F_DEC:
  case F_POST_INC:
  case F_POST_DEC:
    res->node_info |= OPT_ASSIGNMENT;
    break;
    
  default:
    /* We try to optimize most things, but argument lists are hard... */
    if(token != F_ARG_LIST && (a || b))
      res->node_info |= OPT_TRY_OPTIMIZE;

    res->tree_info = res->node_info;
    if(a) res->tree_info |= a->tree_info;
    if(b) res->tree_info |= b->tree_info;
  }

#ifdef PIKE_DEBUG
  if(d_flag > 3)
    verify_shared_strings_tables();
#endif

  if(!num_parse_error && compiler_pass==2)
  {
    check_tree(res,0);
    optimize(res);
    check_tree(res,0);
  }

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
  copy_shared_string(res->type, string_type_string);
  res->node_info = 0;
  res->u.sval.type = T_STRING;
#ifdef __CHECKER__
  res->u.sval.subtype = 0;
#endif
  copy_shared_string(res->u.sval.u.string, str);

  return freeze_node(res);
}

node *debug_mkintnode(int nr)
{
  node *res = mkemptynode();
  res->token = F_CONSTANT;
  res->node_info = 0; 
  res->u.sval.type = T_INT;
  res->u.sval.subtype = NUMBER_NUMBER;
  res->u.sval.u.integer = nr;
  res->type=get_type_of_svalue( & res->u.sval);

  return freeze_node(res);
}

node *debug_mknewintnode(int nr)
{
  node *res = mkemptynode();
  res->token = F_CONSTANT;
  res->node_info = OPT_NOT_SHARED; 
  res->u.sval.type = T_INT;
  res->u.sval.subtype = NUMBER_NUMBER;
  res->u.sval.u.integer = nr;
  res->type=get_type_of_svalue( & res->u.sval);
#ifdef SHARED_NODES
  res->refs = 1;
  res->hash = hash_node(res);
#endif /* SHARED_NODES */

  return res;
}

node *debug_mkfloatnode(FLOAT_TYPE foo)
{
  node *res = mkemptynode();
  res->token = F_CONSTANT;
  copy_shared_string(res->type, float_type_string);
  res->u.sval.type = T_FLOAT;
#ifdef __CHECKER__
  res->u.sval.subtype = 0;
#endif
  res->u.sval.u.float_number = foo;

  return freeze_node(res);
}


node *debug_mkprgnode(struct program *p)
{
  struct svalue s;
  s.u.program=p;
  s.type = T_PROGRAM;
#ifdef __CHECKER__
  s.subtype = 0;
#endif
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
  name = findstring(function);
  if(!name || !(n=find_module_identifier(name)))
  {
    my_yyerror("Internally used efun undefined: %s",function);
    return mkintnode(0);
  }
  n = mkapplynode(n, args);
  return n;
}

node *debug_mkopernode(char *oper_id, node *arg1, node *arg2)
{
  if(arg1 && arg2)
    arg1=mknode(F_ARG_LIST,arg1,arg2);

  return mkefuncallnode(oper_id, arg1);
}

node *debug_mklocalnode(int var, int depth)
{
  struct compiler_frame *f;
  int e;
  node *res = mkemptynode();
  res->token = F_LOCAL;

  f=compiler_frame;
  for(e=0;e<depth;e++) f=f->previous;
  copy_shared_string(res->type, f->variable[var].type);

  res->node_info = OPT_NOT_CONST | OPT_NOT_SHARED;
  res->tree_info = res->node_info;
#ifdef __CHECKER__
  _CDR(res) = 0;
#endif
  res->u.integer.a = var;
  res->u.integer.b = depth;

#ifdef SHARED_NODES
  /* FIXME: Not common-subexpression optimized.
   * Node would need to contain a ref to the current function,
   * and to the current program.
   */
  
  res->hash = hash_node(res);

  /* return freeze_node(res); */
#endif /* SHARED_NODES */

  return res;
}

node *debug_mkidentifiernode(int i)
{
  node *res = mkemptynode();
  res->token = F_IDENTIFIER;
  copy_shared_string(res->type, ID_FROM_INT(new_program, i)->type);

  /* FIXME */
  if(IDENTIFIER_IS_CONSTANT(ID_FROM_INT(new_program, i)->identifier_flags))
  {
    res->node_info = OPT_EXTERNAL_DEPEND;
  }else{
    res->node_info = OPT_NOT_CONST;
  }
  res->tree_info=res->node_info;

#ifdef __CHECKER__
  _CDR(res) = 0;
#endif
  res->u.id.number = i;
#ifdef SHARED_NODES
  res->u.id.prog = new_program;
#endif /* SHARED_NODES */

  res = freeze_node(res);

  check_tree(res,0);
  return res;
}

node *debug_mktrampolinenode(int i)
{
  node *res = mkemptynode();
  res->token = F_TRAMPOLINE;
  copy_shared_string(res->type, ID_FROM_INT(new_program, i)->type);

  /* FIXME */
  if(IDENTIFIER_IS_CONSTANT(ID_FROM_INT(new_program, i)->identifier_flags))
  {
    res->node_info = OPT_EXTERNAL_DEPEND;
  }else{
    res->node_info = OPT_NOT_CONST;
  }
  res->tree_info=res->node_info;

#ifdef __CHECKER__
  _CDR(res) = 0;
#endif
  res->u.id.number = i;
#ifdef SHARED_NODES
  res->u.id.prog = new_program;
#endif /* SHARED_NODES */

  res = freeze_node(res);

  check_tree(res,0);
  return res;
}

node *debug_mkexternalnode(int level,
		     int i,
		     struct identifier *id)
{
  node *res = mkemptynode();
  res->token = F_EXTERNAL;

  copy_shared_string(res->type, id->type);

  /* FIXME */
  if(IDENTIFIER_IS_CONSTANT(ID_FROM_INT(parent_compilation(level),
					i)->identifier_flags))
  {
    res->node_info = OPT_EXTERNAL_DEPEND;
  }else{
    res->node_info = OPT_NOT_CONST;
  }
  res->tree_info = res->node_info;

#ifdef __CHECKER__
  _CDR(res) = 0;
#endif
  res->u.integer.a = level;
  res->u.integer.b = i;

  /* Bzot-i-zot */
  new_program->flags |= PROGRAM_USES_PARENT;

  return freeze_node(res);
}

node *debug_mkcastnode(struct pike_string *type,node *n)
{
  node *res;

  if(!n) return 0;

  if(type==n->type) return n;

  res = mkemptynode();
  res->token = F_CAST;
  copy_shared_string(res->type,type);

  if(match_types(object_type_string, type) ||
     match_types(object_type_string, type))
    res->node_info |= OPT_SIDE_EFFECT;

  _CAR(res) = n;
#ifdef SHARED_NODES
  _CDR(res) = (node *)type;
#else /* !SHARED_NODES */
#ifdef __CHECKER__
  _CDR(res) = 0;
#endif
#endif /* SHARED_NODES */

  n->parent = res;

  return freeze_node(res);
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
      p=parent_compilation(n->u.integer.a);
      if(!p)
      {
	yyerror("Failed to resolv external constant");
	push_int(0);
	return;
      }
      numid=n->u.integer.b;
      break;

    case F_IDENTIFIER:
      p=new_program;
      numid=n->u.id.number;
      break;

    case F_LOCAL:
      /* FIXME: Ought to have the name of the identifier in the message. */
      yyerror("Expected constant, got local variable");
      push_int(0);
      return;

    case F_GLOBAL:
      /* FIXME: Ought to have the name of the identifier in the message. */
      yyerror("Expected constant, got global variable");
      push_int(0);
      return;

    case F_UNDEFINED:
      if(compiler_pass==2) {
	/* FIXME: Ought to have the name of the identifier in the message. */
	yyerror("Expected constant, got undefined identifier");
      }
      push_int(0);
      return;

    default:
    {
      char fnord[1000];
      if(is_const(n))
      {
	int args=eval_low(n);
	if(args==1) return;

	if(args!=-1)
	{
	  if(!args)
	  {
	    yyerror("Expected constant, got void expression");
	  }else{
	    yyerror("Possible internal error!!!");
	    pop_n_elems(args-1);
	    return;
	  }
	}
      }
	
      sprintf(fnord,"Expected constant, got something else (%d)",n->token);
      yyerror(fnord);
      push_int(0);
      return;
    }
    }

    i=ID_FROM_INT(p, numid);
    
    /* Warning:
     * This code doesn't produce function pointers for class constants,
     * which can be harmful...
     * /Hubbe
     */
    if(IDENTIFIER_IS_CONSTANT(i->identifier_flags))
    {
      push_svalue(&PROG_FROM_INT(p, numid)->constants[i->func.offset].sval);
    }else{
      my_yyerror("Identifier '%s' is not a constant", i->name->str);
      push_int(0);
    }
  }
}

/* Leaves a function or object on the stack */
void resolv_class(node *n)
{
  check_tree(n,0);

  resolv_constant(n);
  switch(sp[-1].type)
  {
    case T_OBJECT:
      if(!sp[-1].u.object->prog)
      {
	pop_stack();
	push_int(0);
      }else{
	f_object_program(1);
      }
      break;
      
    default:
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
  check_tree(n,0);

  resolv_class(n);
  switch(sp[-1].type)
  {
    case T_FUNCTION:
      if(program_from_function(sp-1))
	break;
      
    default:
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

  if(SETJMP(tmp))
  {
    ONERROR tmp;
    SET_ONERROR(tmp,exit_on_error,"Error in handle_error in master object!");
    assign_svalue_no_free(sp++, & throw_value);
    APPLY_MASTER("handle_error", 1);
    pop_stack();
    UNSET_ONERROR(tmp);

    if (node_name) {
      my_yyerror("Couldn't index module '%s'.", node_name);
    } else {
      yyerror("Couldn't index module.");
    }
    push_int(0);
  }else{
    resolv_constant(n);
    switch(sp[-1].type)
    {
    case T_INT:
      if(!num_parse_error) {
	if (node_name) {
	  my_yyerror("Failed to index module '%s' (module doesn't exist?)",
		     node_name);
	} else {
	  yyerror("Failed to index module (module doesn't exist?)");
	}
      }
      break;

    case T_FLOAT:
    case T_STRING:
    case T_ARRAY:
      if (node_name) {
	my_yyerror("Failed to index module '%s' (Not a module?)",
		   node_name);
      } else {
	yyerror("Failed to index module (Not a module?)");
      }
      pop_stack();
      push_int(0);
      break;

    default:
    {
      DECLARE_CYCLIC();
      if(BEGIN_CYCLIC(sp[-1].u.refs, id))
      {
	my_yyerror("Recursive module dependency in '%s'.",id->str);
	pop_stack();
	push_int(0);
      }else{
	SET_CYCLIC_RET(1);
	ref_push_string(id);
	{
	  struct svalue *save_sp = sp-2;
	  JMP_BUF recovery;
	  if (SETJMP(recovery)) {
	    /* f_index() threw an error!
	     *
	     * FIXME: Report the error thrown.
	     */
	    if (sp > save_sp) {
	      pop_n_elems(sp - save_sp);
	    } else if (sp != save_sp) {
	      fatal("f_index() munged stack!\n");
	    }
	    push_int(0);
	    sp[-1].subtype = NUMBER_UNDEFINED;
	  } else {
	    f_index(2);
	  }
	  UNSETJMP(recovery);
	}
      
	if(sp[-1].type == T_INT &&
	   !sp[-1].u.integer &&
	   sp[-1].subtype==NUMBER_UNDEFINED)
	{
	  if (node_name) {
	    my_yyerror("Index '%s' not present in module '%s'.",
		       id->str, node_name);
	  } else {
	    my_yyerror("Index '%s' not present in module.", id->str);
	  }
	}
	END_CYCLIC();
      }
    }
    }
  }
  UNSETJMP(tmp);
  ret=mkconstantsvaluenode(sp-1);
  pop_stack();
  return ret;
}


int node_is_eq(node *a,node *b)
{
  check_tree(a,0);
  check_tree(b,0);

  if(a == b) return 1;
  if(!a || !b) return 0;
  if(a->token != b->token) return 0;

  switch(a->token)
  {
  case F_LOCAL:
    return a->u.integer.a == b->u.integer.a &&
      a->u.integer.b == b->u.integer.b;
      
  case F_IDENTIFIER:
  case F_TRAMPOLINE: /* FIXME, the context has to be the same! */
    return a->u.id.number == b->u.id.number;

  case F_CAST:
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

node *debug_mkconstantsvaluenode(struct svalue *s)
{
  node *res = mkemptynode();
  res->token = F_CONSTANT;
  assign_svalue_no_free(& res->u.sval, s);
  if(s->type == T_OBJECT ||
     (s->type==T_FUNCTION && s->subtype!=FUNCTION_BUILTIN))
  {
    res->node_info|=OPT_EXTERNAL_DEPEND;
  }
  res->type = get_type_of_svalue(s);
  return freeze_node(res);
}

node *debug_mkliteralsvaluenode(struct svalue *s)
{
  node *res = mkconstantsvaluenode(s);

  /* FIXME: The following affects other instances of this node,
   * but probably not too much.
   */
  if(s->type!=T_STRING && s->type!=T_INT && s->type!=T_FLOAT)
    res->node_info|=OPT_EXTERNAL_DEPEND;

  return res;
}

node *debug_mksvaluenode(struct svalue *s)
{
  switch(s->type)
  {
  case T_ARRAY:
    return make_node_from_array(s->u.array);
    
  case T_MULTISET:
    return make_node_from_multiset(s->u.multiset);

  case T_MAPPING:
    return make_node_from_mapping(s->u.mapping);

  case T_OBJECT:
    if(s->u.object == fake_object)
    {
      return mkefuncallnode("this_object", 0);
    }
    if(s->u.object->next == s->u.object)
    {
      int x=0;
      struct object *o;
      node *n=mkefuncallnode("this_object", 0);
      for(o=fake_object;o!=s->u.object;o=o->parent)
      {
	n=mkefuncallnode("function_object",
			 mkefuncallnode("object_program",n));
      }
      return n;
    }
    break;

  case T_FUNCTION:
  {
    if(s->subtype != FUNCTION_BUILTIN)
    {
      if(s->u.object == fake_object)
	return mkidentifiernode(s->subtype);

      if(s->u.object->next == s->u.object)
      {
	int x=0;
	struct object *o;
	for(o=fake_object->parent;o!=s->u.object;o=o->parent) x++;
	return mkexternalnode(x, s->subtype,
			      ID_FROM_INT(o->prog, s->subtype));

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

#ifdef DEAD_CODE

node *copy_node(node *n)
{
  node *b;
  check_tree(n,0);
  if(!n) return n;
  switch(n->token)
  {
  case F_LOCAL:
  case F_IDENTIFIER:
  case F_TRAMPOLINE:
    b=mknewintnode(0);
    *b=*n;
    copy_shared_string(b->type, n->type);
    return b;

  default:
#ifdef SHARED_NODES
    n->refs++;
    return n;
#else /* !SHARED_NODES */
  case F_CAST:
    b=mkcastnode(n->type,copy_node(CAR(n)));
    break;

  case F_CONSTANT:
    b=mksvaluenode(&(n->u.sval));
    break;

  default:
    switch((car_is_node(n) << 1) | cdr_is_node(n))
    {
    default: fatal("fooo?\n");

    case 3:
      b=mknode(n->token, copy_node(CAR(n)), copy_node(CDR(n)));
      break;
    case 2:
      b=mknode(n->token, copy_node(CAR(n)), CDR(n));
      break;

    case 1:
      b=mknode(n->token, CAR(n), copy_node(CDR(n)));
      break;

    case 0:
      b=mknode(n->token, CAR(n), CDR(n));
    }
    if(n->type)
      copy_shared_string(b->type, n->type);
    else
      b->type=0;
#endif /* SHARED_NODES */
  }
  if(n->name)
  {
    if(b->name) free_string(b->name);
    add_ref(b->name=n->name);
  }
  /* FIXME: Should b->name be kept if n->name is NULL?
   * /grubba 1999-09-22
   */
  b->line_number = n->line_number;
  b->node_info = n->node_info;
  b->tree_info = n->tree_info;
  return b;
}

#endif /* DEAD_CODE */


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
  return !(n->tree_info & (OPT_SIDE_EFFECT |
			   OPT_ASSIGNMENT |
			   OPT_CASE |
			   OPT_CONTINUE |
			   OPT_BREAK |
			   OPT_RETURN
			   ));
}

/* this one supposes that the value is optimized */
int node_is_true(node *n)
{
  if(!n) return 0;
  switch(n->token)
  {
    case F_CONSTANT: return !IS_ZERO(& n->u.sval);
    default: return 0;
  }
}

/* this one supposes that the value is optimized */
int node_is_false(node *n)
{
  if(!n) return 0;
  switch(n->token)
  {
    case F_CONSTANT: return IS_ZERO(& n->u.sval);
    default: return 0;
  }
}

int node_may_overload(node *n, int lfun)
{
  if(!n) return 0;
  if(!n->type) return 1;
  return type_may_overload(n->type->str, lfun);
}

node **last_cmd(node **a)
{
  node **n;
  if(!a || !*a) return (node **)NULL;
  if((*a)->token == F_CAST) return last_cmd(&_CAR(*a));
  if(((*a)->token != F_ARG_LIST) &&
     ((*a)->token != F_COMMA_EXPR)) return a;
  if(CDR(*a))
  {
    if(CDR(*a)->token != F_CAST && CAR(*a)->token != F_ARG_LIST &&
       CAR(*a)->token != F_COMMA_EXPR)
      return &_CDR(*a);
    if((n=last_cmd(&_CDR(*a))))
      return n;
  }
  if(CAR(*a))
  {
    if(CAR(*a)->token != F_CAST && CAR(*a)->token != F_ARG_LIST &&
       CAR(*a)->token != F_COMMA_EXPR)
      return &_CAR(*a);
    if((n=last_cmd(&_CAR(*a))))
      return n;
  }
  return 0;
}

static node **low_get_arg(node **a,int *nr)
{
  node **n;
  if(a[0]->token != F_ARG_LIST)
  {
    if(!(*nr)--)
      return a;
    else
      return NULL;
  }
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
    case F_APPLY:
      if(CAR(n) &&
	 CAR(n)->token == F_CONSTANT &&
	 CAR(n)->u.sval.type == T_FUNCTION &&
	 CAR(n)->u.sval.subtype == FUNCTION_BUILTIN &&
	 CAR(n)->u.sval.u.efun->function == f)
	return &_CDR(n);
  }
  return 0;
}


static void low_print_tree(node *foo,int needlval)
{
  if(!foo) return;
  if(l_flag>9)
  {
    fprintf(stderr, "/*%x*/",foo->tree_info);
  }
  switch(foo->token)
  {
  case USHRT_MAX:
    fprintf(stderr, "FREED_NODE");
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
    fprintf(stderr, "(");
    low_print_tree(_CAR(foo),0);
    fprintf(stderr, ")?(");
    if (_CDR(foo)) {
      low_print_tree(_CADR(foo),0);
      fprintf(stderr, "):(");
      low_print_tree(_CDDR(foo),0);
    } else {
      fprintf(stderr, "0:0");
    }
    fprintf(stderr, ")");
    break;

  case F_IDENTIFIER:
    if(needlval) fputc('&', stderr);
    if (new_program) {
      fprintf(stderr, "%s",ID_FROM_INT(new_program, foo->u.id.number)->name->str);
    } else {
      fprintf(stderr, "unknown identifier");
    }
    break;

  case F_TRAMPOLINE:
    if (new_program) {
      fprintf(stderr, "trampoline<%s>",
	      ID_FROM_INT(new_program, foo->u.id.number)->name->str);
    } else {
      fprintf(stderr, "trampoline<unknown identifier>");
    }
    break;

  case F_ASSIGN:
    low_print_tree(_CDR(foo),1);
    fprintf(stderr, "=");
    low_print_tree(_CAR(foo),0);
    break;

  case F_CAST:
  {
    char *s;
    init_buf();
    low_describe_type(foo->type->str);
    s=simple_free_buf();
    fprintf(stderr, "(%s){",s);
    free(s);
    low_print_tree(_CAR(foo),0);
    fprintf(stderr, "}");
    break;
  }

  case F_COMMA_EXPR:
  case F_ARG_LIST:
    low_print_tree(_CAR(foo),0);
    if(_CAR(foo) && _CDR(foo))
    {
      if(_CAR(foo)->type == void_type_string &&
	 _CDR(foo)->type == void_type_string)
	fprintf(stderr, ";\n");
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
    char *s;
    init_buf();
    describe_svalue(& foo->u.sval, 0, 0);
    s=simple_free_buf();
    fprintf(stderr, "%s",s);
    free(s);
    break;
  }

  case F_VAL_LVAL:
    low_print_tree(_CAR(foo),0);
    fprintf(stderr, ",&");
    low_print_tree(_CDR(foo),0);
    return;

  case F_APPLY:
    low_print_tree(_CAR(foo),0);
    fprintf(stderr, "(");
    low_print_tree(_CDR(foo),0);
    fprintf(stderr, ")");
    return;

  default:
    if(!car_is_node(foo) && !cdr_is_node(foo))
    {
      fprintf(stderr, "%s",get_token_name(foo->token));
      return;
    }
    if(foo->token<256)
    {
      fprintf(stderr, "%c(",foo->token);
    }else{
      fprintf(stderr, "%s(",get_token_name(foo->token));
    }
    if(car_is_node(foo)) low_print_tree(_CAR(foo),0);
    if(car_is_node(foo) && cdr_is_node(foo))
      fputc(',', stderr);
    if(cdr_is_node(foo)) low_print_tree(_CDR(foo),0);
    fprintf(stderr, ")");
    return;
  }
}

void print_tree(node *n)
{
  check_tree(n,0);
  low_print_tree(n,0);
  fprintf(stderr, "\n");
  fflush(stdout);
}


/* The following routines needs much better commenting */
struct used_vars
{
  int err;
  char locals[MAX_LOCAL];
  char globals[MAX_GLOBAL];
};

#define VAR_BLOCKED   0
#define VAR_UNUSED    1
#define VAR_USED      3

static void do_and_vars(struct used_vars *a,struct used_vars *b)
{
  int e;
  for(e=0;e<MAX_LOCAL;e++) a->locals[e]|=b->locals[e];
  for(e=0;e<MAX_GLOBAL;e++) a->globals[e]|=b->globals[e];
  a->err|=b->err;
  free((char *)b);
}

static struct used_vars *copy_vars(struct used_vars *a)
{
  struct used_vars *ret;
  ret=(struct used_vars *)xalloc(sizeof(struct used_vars));
  MEMCPY((char *)ret,(char *)a,sizeof(struct used_vars));
  return ret;
}

static int find_used_variables(node *n,
			       struct used_vars *p,
			       int noblock,
			       int overwrite)
{
  struct used_vars *a;
  char *q;

  if(!n) return 0;
  switch(n->token)
  {
  case F_LOCAL:
    /* FIXME: handle local variable depth */
    q=p->locals+n->u.integer.a;
    goto set_pointer;

  case F_IDENTIFIER:
    q=p->globals+n->u.id.number;
    if(n->u.id.number > MAX_GLOBAL)
    {
      p->err=1;
      return 0;
    }

  set_pointer:
    if(overwrite)
    {
      if(*q == VAR_UNUSED && !noblock) *q = VAR_BLOCKED;
    }
    else
    {
      if(*q == VAR_UNUSED) *q = VAR_USED;
    }
    break;

  case F_ASSIGN:
    find_used_variables(CAR(n),p,noblock,0);
    find_used_variables(CDR(n),p,noblock,1);
    break;

  case '?':
    find_used_variables(CAR(n),p,noblock,0);
    a=copy_vars(p);
    find_used_variables(CADR(n),a,noblock,0);
    find_used_variables(CDDR(n),p,noblock,0);
    do_and_vars(p,a);
    break;

  case F_INC_NEQ_LOOP:
  case F_DEC_NEQ_LOOP:
  case F_INC_LOOP:
  case F_DEC_LOOP:
  case F_FOREACH:
  case F_FOR:
    find_used_variables(CAR(n),p,noblock,0);
    a=copy_vars(p);
    find_used_variables(CDR(n),a,noblock,0);
    do_and_vars(p,a);
    break;

  case F_SWITCH:
    find_used_variables(CAR(n),p,noblock,0);
    a=copy_vars(p);
    find_used_variables(CDR(n),a,1,0);
    do_and_vars(p,a);
    break;

   case F_DO:
    a=copy_vars(p);
    find_used_variables(CAR(n),a,noblock,0);
    do_and_vars(p,a);
    find_used_variables(CDR(n),p,noblock,0);
    break;

  default:
    if(car_is_node(n)) find_used_variables(CAR(n),p,noblock,0);
    if(cdr_is_node(n)) find_used_variables(CDR(n),p,noblock,0);
  }
  return 0;
}

/* no subtility needed */
static void find_written_vars(node *n,
			      struct used_vars *p,
			      int lvalue)
{
  if(!n) return;

  switch(n->token)
  {
  case F_LOCAL:
    if(lvalue) p->locals[n->u.integer.a]=VAR_USED;
    break;

  case F_IDENTIFIER:
     if(lvalue)
     {
       if(n->u.id.number>=MAX_GLOBAL)
       {
	 p->err=1;
	 return;
       }
       p->globals[n->u.id.number]=VAR_USED;
     }
    break;

  case F_APPLY:
    if(n->tree_info & OPT_SIDE_EFFECT)
      MEMSET(p->globals, VAR_USED, MAX_GLOBAL);
    break;

  case F_INDEX:
  case F_ARROW:
    find_written_vars(CAR(n), p, lvalue);
    find_written_vars(CDR(n), p, 0);
    break;

  case F_INC:
  case F_DEC:
  case F_POST_INC:
  case F_POST_DEC:
    find_written_vars(CAR(n), p, 1);
    break;

  case F_ASSIGN:
    find_written_vars(CAR(n), p, 0);
    find_written_vars(CDR(n), p, 1);
    break;

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
      find_written_vars(CAR(n), p, 1);
      find_written_vars(CDR(n), p, 0);
      break;

  case F_SSCANF:
    find_written_vars(CAR(n), p, 0);
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

/* return 1 if A depends on B */
static int depend_p2(node *a,node *b)
{
  struct used_vars aa,bb;
  int e;

  if(!a || !b || is_const(a)) return 0;
  aa.err=0;
  bb.err=0;
  MEMSET((char *)aa.locals, VAR_UNUSED, MAX_LOCAL);
  MEMSET((char *)bb.locals, VAR_UNUSED,  MAX_LOCAL);
  MEMSET((char *)aa.globals, VAR_UNUSED, MAX_GLOBAL);
  MEMSET((char *)bb.globals, VAR_UNUSED, MAX_GLOBAL);

  find_used_variables(a,&aa,0,0);
  find_written_vars(b,&bb,0);

  if(aa.err || bb.err) return 1;

  for(e=0;e<MAX_LOCAL;e++)
    if(aa.locals[e]==VAR_USED && bb.locals[e]!=VAR_UNUSED)
      return 1;

  for(e=0;e<MAX_GLOBAL;e++)
    if(aa.globals[e]==VAR_USED && bb.globals[e]!=VAR_UNUSED)
      return 1;
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
  int ret;
  if(l_flag > 3)
  {
    fprintf(stderr,"Checking if: ");
    print_tree(a);
    fprintf(stderr,"Depends on: ");
    print_tree(b);
    if(depend_p3(a,b))
    {
      fprintf(stderr,"The answer is (durumroll) : yes\n");
      return 1;
    }else{
      fprintf(stderr,"The answer is (durumroll) : no\n");
      return 0;
    }
  }
  return depend_p3(a,b);
}
#else
#define depend_p depend_p3
#endif

static int cntargs(node *n)
{
  if(!n) return 0;
  switch(n->token)
  {
  case F_CAST:
  case F_APPLY:
    return n->type != void_type_string;

  case F_FOREACH:
  case F_INC_NEQ_LOOP:
  case F_DEC_NEQ_LOOP:
  case F_INC_LOOP:
  case F_DEC_LOOP:  return 0;

  case F_COMMA_EXPR:
  case F_VAL_LVAL:
  case F_LVALUE_LIST:
  case F_ARG_LIST:
    return cntargs(CAR(n))+cntargs(CDR(n));

    /* this might not be true, but it doesn't matter very much */
  default: return 1;
  }
}

static int function_type_max=0;

static void low_build_function_type(node *n)
{
  if(!n) return;
  if(function_type_max++ > 999)
  {
    reset_type_stack();
    push_type(T_MIXED);
    push_type(T_MIXED); /* is varargs */
    push_type(T_MANY);
    return;
  }
  switch(n->token)
  {
  case F_COMMA_EXPR:
  case F_ARG_LIST:
    low_build_function_type(CDR(n));
    low_build_function_type(CAR(n));
    break;

  case F_PUSH_ARRAY: /* We let this ruin type-checking for now.. */
    reset_type_stack();
    push_type(T_MIXED);
    push_type(T_MIXED); /* is varargs */
    push_type(T_MANY);
    return;

  default:
    if(n->type)
    {
      if(n->type == void_type_string) return;
      push_finished_type(n->type);
    }else{
      push_type(T_MIXED);
    }
  }
}


void fix_type_field(node *n)
{
  struct pike_string *type_a,*type_b;

  if(n->type) return; /* assume it is correct */

  switch(n->token)
  {
  case F_LAND:
  case F_LOR:
    if(!match_types(CAR(n)->type,mixed_type_string))
      yyerror("Bad conditional expression.\n");

    if(!match_types(CDR(n)->type,mixed_type_string))
      yyerror("Bad conditional expression.\n");

    if(CAR(n)->type == CDR(n)->type)
    {
      copy_shared_string(n->type,CAR(n)->type);
    }else{
      copy_shared_string(n->type,mixed_type_string);
    }
    break;

  case F_ASSIGN:
    if(CAR(n) && CDR(n) &&
       /* a["b"]=c and a->b=c can be valid when a is an array */
       CDR(n)->token != F_INDEX && CDR(n)->token != F_ARROW &&
       !match_types(CDR(n)->type,CAR(n)->type))
      my_yyerror("Bad type in assignment.");
    copy_shared_string(n->type, CAR(n)->type);
    break;

  case F_INDEX:
  case F_ARROW:
    type_a=CAR(n)->type;
    type_b=CDR(n)->type;
    if(!check_indexing(type_a, type_b, n))
      if(!catch_level)
        my_yyerror("Indexing on illegal type.");
    n->type=index_type(type_a,n);
    break;

  case F_APPLY:
  {
    struct pike_string *s;
    struct pike_string *f;
    INT32 max_args,args;
    push_type(T_MIXED); /* match any return type, even void */
    push_type(T_VOID); /* not varargs */
    push_type(T_MANY);
    function_type_max=0;
    low_build_function_type(CDR(n));
    push_type(T_FUNCTION);
    s=pop_type();
    f=CAR(n)->type?CAR(n)->type:mixed_type_string;
    n->type=check_call(s,f);
    args=count_arguments(s);
    max_args=count_arguments(f);
    if(max_args<0) max_args=0x7fffffff;

    if(!n->type)
    {
      char *name;
      switch(CAR(n)->token)
      {
#if 0 /* FIXME */
	case F_TRAMPOLINE;
#endif
      case F_IDENTIFIER:
	name=ID_FROM_INT(new_program, CAR(n)->u.id.number)->name->str;
	break;
	
      case F_CONSTANT:
	switch(CAR(n)->u.sval.type)
	{
	  case T_FUNCTION:
	    if(CAR(n)->u.sval.subtype == FUNCTION_BUILTIN)
	    {
	      name=CAR(n)->u.sval.u.efun->name->str;
	    }else{
	      name=ID_FROM_INT(CAR(n)->u.sval.u.object->prog,
			       CAR(n)->u.sval.subtype)->name->str;
	    }
	    break;

	  case T_ARRAY:
	    name="array call";
	    break;

	  case T_PROGRAM:
	    name="clone call";
	    break;

	  default:
	    name="`() (function call)";
	    break;
	}
	break;

      default:
	name="unknown function";
      }

      if(max_args < args)
      {
	my_yyerror("Too many arguments to %s.\n",name);
      }
      else if(max_correct_args == args)
      {
	my_yyerror("Too few arguments to %s.\n",name);
      }else{
	my_yyerror("Bad argument %d to %s.",
		   max_correct_args+1, name);
      }
      copy_shared_string(n->type, mixed_type_string);
    }
    free_string(s);
    break;
  }

  case '?':
    if(!match_types(CAR(n)->type,mixed_type_string))
      yyerror("Bad conditional expression.\n");

    if(!CADR(n) || !CDDR(n))
    {
      copy_shared_string(n->type,void_type_string);
      return;
    }
    
    if(CADR(n)->type == CDDR(n)->type)
    {
      copy_shared_string(n->type,CADR(n)->type);
      return;
    }

    copy_shared_string(n->type,mixed_type_string);
    break;

  case F_RETURN:
    if(CAR(n)->type == void_type_string)
    {
      yyerror("You cannot return a void expression");
    }
    if(compiler_frame &&
       compiler_frame->current_return_type &&
       !match_types(compiler_frame->current_return_type,CAR(n)->type) &&
       !(
	 compiler_frame->current_return_type==void_type_string &&
	 CAR(n)->token == F_CONSTANT &&
	 IS_ZERO(& CAR(n)->u.sval)
	 )
       )
    {
      yyerror("Wrong return type.");
    }

    /* Fall through */

  case F_INC_LOOP:
  case F_DEC_LOOP:
  case F_DEC_NEQ_LOOP:
  case F_INC_NEQ_LOOP:
  case F_CASE:
  case F_CONTINUE:
  case F_BREAK:
    copy_shared_string(n->type,void_type_string);
    break;

  case F_DO:
    if(!match_types(CDR(n)->type,mixed_type_string))
      yyerror("Bad conditional expression do - while().\n");
    copy_shared_string(n->type,void_type_string);
    break;
    
  case F_FOR:
    if(!match_types(CAR(n)->type,mixed_type_string))
      yyerror("Bad conditional expression for().\n");
    copy_shared_string(n->type,void_type_string);
    break;

  case F_SWITCH:
    if(!match_types(CAR(n)->type,mixed_type_string))
      yyerror("Bad switch expression.\n");
    copy_shared_string(n->type,void_type_string);
    break;

  case F_CONSTANT:
    n->type = get_type_of_svalue(& n->u.sval);
    break;

  case F_COMMA_EXPR:
  case F_ARG_LIST:
    if(!CAR(n) || CAR(n)->type==void_type_string)
    {
      if(CDR(n))
	copy_shared_string(n->type,CDR(n)->type);
      else
	copy_shared_string(n->type,void_type_string);
      return;
    }

    if(!CDR(n) || CDR(n)->type==void_type_string)
    {
      if(CAR(n))
	copy_shared_string(n->type,CAR(n)->type);
      else
	copy_shared_string(n->type,void_type_string);
      return;
    }

  default:
    copy_shared_string(n->type,mixed_type_string);
  }
}

static void zapp_try_optimize(node *n)
{
  if(!n) return;
  n->node_info &=~ OPT_TRY_OPTIMIZE;
  n->tree_info &=~ OPT_TRY_OPTIMIZE;
  if(car_is_node(n)) zapp_try_optimize(CAR(n));
  if(cdr_is_node(n)) zapp_try_optimize(CDR(n));
}

static void optimize(node *n)
{
  node *tmp1, *tmp2, *tmp3;
  INT32 save_line = lex.current_line;
  do
  {
    if(car_is_node(n) &&
       ((CAR(n)->node_info & (OPT_OPTIMIZED|OPT_DEFROSTED)) != OPT_OPTIMIZED))
    {
      CAR(n)->parent = n;
      n = CAR(n);
      continue;
    }
    if(cdr_is_node(n) &&
       ((CDR(n)->node_info & (OPT_OPTIMIZED|OPT_DEFROSTED)) != OPT_OPTIMIZED))
    {
      CDR(n)->parent = n;
      n = CDR(n);
      continue;
    }

#if defined(SHARED_NODES)
    if ((n->node_info & OPT_DEFROSTED) && (n->parent)) {
#ifndef IN_TPIKE
      /* Add ref since both freeze_node() and use_tmp1 will free it. */
      ADD_NODE_REF(n);
      /* We don't want freeze_node() to find this node in the hash-table. */
      sub_node(n);
      tmp1 = freeze_node(n);
      if (tmp1 != n) {
	/* n was a duplicate node. Use the original. */
	goto use_tmp1;
      }
      /* Remove the extra ref from n */
      free_node(n);
#endif /* !IN_TPIKE */
      /* The node is now frozen again. */
      n->node_info &= ~OPT_DEFROSTED;
      if (n->node_info & OPT_OPTIMIZED) {
	/* No need to check this node any more. */
	n = n->parent;
	continue;
      }
    }
#endif /* SHARED_NODES && !IN_TPIKE */

    lex.current_line = n->line_number;


    n->tree_info = n->node_info;
    if(car_is_node(n)) n->tree_info |= CAR(n)->tree_info;
    if(cdr_is_node(n)) n->tree_info |= CDR(n)->tree_info;

    if(!n->parent) break;
    
    if(n->tree_info & (OPT_NOT_CONST|
		       OPT_SIDE_EFFECT|
		       OPT_EXTERNAL_DEPEND|
		       OPT_ASSIGNMENT))
    {
      if(car_is_node(n) &&
	 !(CAR(n)->tree_info & (OPT_NOT_CONST|
				OPT_SIDE_EFFECT|
				OPT_EXTERNAL_DEPEND|
				OPT_ASSIGNMENT)) &&
	 (CAR(n)->tree_info & OPT_TRY_OPTIMIZE) &&
	 CAR(n)->token != ':')
      {
#ifdef SHARED_NODES
	sub_node(n);
#endif /* SHARED_NODES */
	_CAR(n) = eval(CAR(n));
#ifdef SHARED_NODES
	n->hash = hash_node(n);
	n->node_info |= OPT_DEFROSTED;
	add_node(n);
#endif /* SHARED_NODES */
	if(CAR(n)) CAR(n)->parent = n;
	zapp_try_optimize(CAR(n)); /* avoid infinite loops */
	continue;
      }
      if(cdr_is_node(n) &&
	 !(CDR(n)->tree_info & (OPT_NOT_CONST|
				OPT_SIDE_EFFECT|
				OPT_EXTERNAL_DEPEND|
				OPT_ASSIGNMENT)) &&
	 (CDR(n)->tree_info & OPT_TRY_OPTIMIZE) &&
	 CDR(n)->token != ':')
      {
#ifdef SHARED_NODES
	sub_node(n);
#endif /* SHARED_NODES */
	_CDR(n) = eval(CDR(n));
#ifdef SHARED_NODES
	n->hash = hash_node(n);
	n->node_info |= OPT_DEFROSTED;
	add_node(n);
#endif /* SHARED_NODES */
	if(CDR(n)) CDR(n)->parent = n;
	zapp_try_optimize(CDR(n)); /* avoid infinite loops */
	continue;
      }
    }
    fix_type_field(n);
    debug_malloc_touch(n->type);

#ifdef PIKE_DEBUG
    if(l_flag > 3 && n)
    {
      fprintf(stderr,"Optimizing (tree info=%x):",n->tree_info);
      print_tree(n);
    }
#endif    

#ifndef IN_TPIKE
    switch(n->token)
    {
#include "treeopt.h"
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
	fprintf(stderr, "Optimized: ");
	print_tree(n);
	fprintf(stderr, "Result:    ");
	print_tree(tmp1);
      }
#endif /* PIKE_DEBUG */

#ifdef SHARED_NODES
      sub_node(n->parent);
#endif /* SHARED_NODES */

      if(CAR(n->parent) == n)
	_CAR(n->parent) = tmp1;
      else
	_CDR(n->parent) = tmp1;

#ifdef SHARED_NODES      
      n->parent->hash = hash_node(n->parent);
      add_node(n->parent);
      n->parent->node_info |= OPT_DEFROSTED;
#endif /* SHARED_NODES */
	
      if(tmp1)
	tmp1->parent = n->parent;
      else
	tmp1 = n->parent;

      free_node(n);
      n = tmp1;

#ifdef PIKE_DEBUG
      if(l_flag > 3)
      {
	fprintf(stderr,"Result:    ");
	print_tree(n);
      }
#endif    
      continue;

    }
#endif /* !IN_TPIKE */
    n->node_info |= OPT_OPTIMIZED;
    n=n->parent;
  }while(n);
  lex.current_line = save_line;
}

struct timer_oflo
{
  INT32 counter;
  int yes;
};

static void check_evaluation_time(struct callback *cb,void *tmp,void *ignored)
{
  struct timer_oflo *foo=(struct timer_oflo *)tmp;
  if(foo->counter-- < 0)
  {
    foo->yes=1;
    pike_throw();
  }
}

int eval_low(node *n)
{
  unsigned INT16 num_strings, num_constants;
  INT32 jump;
  struct svalue *save_sp = sp;
  int ret;

#ifdef PIKE_DEBUG
  if(l_flag > 3 && n)
  {
    fprintf(stderr,"Evaluating (tree info=%x):",n->tree_info);
    print_tree(n);
  }
#endif

  if(num_parse_error) return -1; 

  num_strings=new_program->num_strings;
  num_constants=new_program->num_constants;
  jump=PC;

  store_linenumbers=0;
  docode(dmalloc_touch(node *, n));
  ins_f_byte(F_DUMB_RETURN);
  store_linenumbers=1;

  ret=-1;
  if(!num_parse_error)
  {
    struct callback *tmp_callback;
    struct timer_oflo foo;

    /* This is how long we try to optimize before giving up... */
    foo.counter=10000;
    foo.yes=0;

    tmp_callback=add_to_callback(&evaluator_callbacks,
				 check_evaluation_time,
				 (void *)&foo,0);
				 
    if(apply_low_safe_and_stupid(fake_object, jump))
    {
      /* Generate error message */
      if(!catch_level)
      {
        if(throw_value.type == T_ARRAY && throw_value.u.array->size)
        {
	  union anything *a;
	  a=low_array_get_item_ptr(throw_value.u.array, 0, T_STRING);
	  if(a)
	  {
	    yyerror(a->string->str);
	  }else{
	    yyerror("Nonstandard error format.");
	  }
	}
	else if(throw_value.type == T_OBJECT)
	{
	  ref_push_object(throw_value.u.object);
	  push_int(0);
	  f_index(2);
	  if(sp[-1].type != T_STRING)
	    yyerror("Nonstandard error format.");
	  else
	    yyerror(sp[-1].u.string->str);
	  pop_stack();
	}
	else
	{
	  yyerror("Nonstandard error format.");
	}
      }
    }else{
      if(foo.yes)
	pop_n_elems(sp-save_sp);
      else
	ret=sp-save_sp;
    }

    remove_callback(tmp_callback);
  }

  while(new_program->num_strings > num_strings)
  {
    new_program->num_strings--;
    free_string(new_program->strings[new_program->num_strings]);
  }

  while(new_program->num_constants > num_constants)
  {
    struct program_constant *p_const;

    new_program->num_constants--;

    p_const = new_program->constants + new_program->num_constants;

    free_svalue(&p_const->sval);
    if (p_const->name) {
      free_string(p_const->name);
      p_const->name = NULL;
    }
  }

  new_program->num_program=jump;

  return ret;
}

static node *eval(node *n)
{
  int args;
  extern struct svalue *sp;
  if(!is_const(n) || n->token==':')
    return n;
  args=eval_low(n);

  switch(args)
  {
  case -1:
    return n;
    break;

  case 0:
    if(catch_level) return n;
    free_node(n);
    n=0;
    break;

  case 1:
    if(catch_level && IS_ZERO(sp-1))
    {
      pop_stack();
      return n;
    }
    free_node(n);
    n=mksvaluenode(sp-1);
    pop_stack();
    break;

  default:
    free_node(n);
    n=NULL;
    while(args--)
    {
      n=mknode(F_ARG_LIST,mksvaluenode(sp-1),n);
      pop_stack();
    }
  }
  return dmalloc_touch(node *, n);
}

INT32 last_function_opt_info;

static int stupid_args(node *n, int expected,int vargs)
{
  if(!n) return expected;
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

static int is_null_branch(node *n)
{
  if(!n) return 1;
  if(n->token==F_CAST && n->type==void_type_string)
    return is_null_branch(CAR(n));
  if(n->token==F_ARG_LIST)
    return is_null_branch(CAR(n)) && is_null_branch(CDR(n));
  return 0;
}

static struct svalue *is_stupid_func(node *n,
				     int args,
				     int vargs,
				     struct pike_string *type)
{
  node *a,*b;
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

    if(n->token == F_CAST && n->type==void_type_string)
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
	      struct pike_string *type,
	      int modifiers)
{
  union idptr tmp;
  int args, vargs, ret;
  struct svalue *foo;

  check_tree(check_node_hash(n),0);

#ifdef PIKE_DEBUG
  if(a_flag > 1)
    fprintf(stderr,"Doing function '%s' at %x\n",name->str,PC);
#endif

  args=count_arguments(type);
  if(args < 0) 
  {
    args=~args;
    vargs=IDENTIFIER_VARARGS;
  }else{
    vargs=0;
  }
  if(compiler_pass==1)
  {
    tmp.offset=-1;
#ifdef PIKE_DEBUG
    if(a_flag > 4)
    {
      fprintf(stderr,"Making prototype (pass 1) for: ");
      print_tree(n);
    }
#endif
  }else{
    n=mknode(F_ARG_LIST,check_node_hash(n),0);
    
    if((foo=is_stupid_func(check_node_hash(n), args, vargs, type)))
    {
      if(foo->type == T_FUNCTION && foo->subtype==FUNCTION_BUILTIN)
      {
	tmp.c_fun=foo->u.efun->function;
	ret=define_function(name,
			    type,
			    modifiers,
			    IDENTIFIER_C_FUNCTION | vargs,
			    &tmp);
	free_node(n);
#ifdef PIKE_DEBUG
	if(a_flag > 1)
	  fprintf(stderr,"Identifer (C) = %d\n",ret);
#endif
	return ret;
      }
    }

    tmp.offset=PC;
    add_to_program(compiler_frame->max_number_of_locals);
    add_to_program(args);
  
#ifdef PIKE_DEBUG
    if(a_flag > 2)
    {
      fprintf(stderr,"Coding: ");
      print_tree(check_node_hash(n));
    }
#endif
    if(!num_parse_error)
    {
      extern int remove_clear_locals;
      remove_clear_locals=args;
      if(vargs) remove_clear_locals++;
      do_code_block(check_node_hash(n));
      remove_clear_locals=0x7fffffff;
    }
  }
  
  ret=define_function(name,
		      type,
		      modifiers,
		      IDENTIFIER_PIKE_FUNCTION | vargs,
		      &tmp);


#ifdef PIKE_DEBUG
  if(a_flag > 1)
    fprintf(stderr,"Identifer = %d\n",ret);
#endif

  free_node(n);
  return ret;
}


