/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
RCSID("$Id: las.c,v 1.15 1997/01/28 03:04:31 hubbe Exp $");

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
#include "memory.h"
#include "operators.h"
#include "callback.h"
#include "macros.h"

#define LASDEBUG

int lasdebug=0;

static node *eval(node *);
static void optimize(node *n);

dynamic_buffer areas[NUM_AREAS];
node *init_node = 0;
int num_parse_error;
int cumulative_parse_error=0;
extern char *get_type_name(int);

#define MAX_GLOBAL 256

int car_is_node(node *n)
{
  switch(n->token)
  {
  case F_IDENTIFIER:
  case F_CONSTANT:
  case F_LOCAL:
    return 0;

  default:
    return !!CAR(n);
  }
}

int cdr_is_node(node *n)
{
  switch(n->token)
  {
  case F_IDENTIFIER:
  case F_CONSTANT:
  case F_LOCAL:
  case F_CAST:
    return 0;

  default:
    return !!CDR(n);
  }
}

INT32 count_args(node *n)
{
  int a,b;
  if(!n) return 0;
  switch(n->token)
  {
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


#define NODES 256

struct node_chunk 
{
  struct node_chunk *next;
  node nodes[NODES];
};

static struct node_chunk *node_chunks=0;
static node *free_nodes=0;

void free_all_nodes()
{
  if(!local_variables)
  {
    node *tmp;
    struct node_chunk *tmp2;
    int e=0;


    if(cumulative_parse_error)
    {
      
      for(tmp2=node_chunks;tmp2;tmp2=tmp2->next) e+=NODES;
      for(tmp=free_nodes;tmp;tmp=CAR(tmp)) e--;
      if(e)
      {
	int e2=e;
	for(tmp2=node_chunks;tmp2;tmp2=tmp2->next)
	{
	  for(e=0;e<NODES;e++)
	  {
	    for(tmp=free_nodes;tmp;tmp=CAR(tmp))
	      if(tmp==tmp2->nodes+e)
		break;
	    
	    if(!tmp)
	    {
	      tmp=tmp2->nodes+e;
#ifdef DEBUG
	      if(!cumulative_parse_error)
	      {
		fprintf(stderr,"Free node at %p.\n",tmp);
	      }
	      else
#endif
	      {
		/* Free the node and be happy */
		/* Make sure we don't free any nodes twice */
		if(car_is_node(tmp)) CAR(tmp)=0;
		if(cdr_is_node(tmp)) CDR(tmp)=0;
		free_node(tmp);
	      }
	    }
	  }
	}
#ifdef DEBUG
	if(!cumulative_parse_error)
	  fatal("Failed to free %d nodes when compiling!\n",e2);
#endif
      }
    }
    while(node_chunks)
    {
      tmp2=node_chunks;
      node_chunks=tmp2->next;
      free((char *)tmp2);
    }
    free_nodes=0;
    cumulative_parse_error=0;
  }
}

void free_node(node *n)
{
  if(!n) return;
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
  CAR(n)=free_nodes;
  free_nodes=n;
}


/* here starts routines to make nodes */
static node *mkemptynode()
{
  node *res;
  if(!free_nodes)
  {
    int e;
    struct node_chunk *tmp=ALLOC_STRUCT(node_chunk);
    tmp->next=node_chunks;
    node_chunks=tmp;

    for(e=0;e<NODES-1;e++)
      CAR(tmp->nodes+e)=tmp->nodes+e+1;
    CAR(tmp->nodes+e)=0;
    free_nodes=tmp->nodes;
  }
  res=free_nodes;
  free_nodes=CAR(res);
  res->token=0;
  res->line_number=current_line;
  res->type=0;
  res->node_info=0;
  res->tree_info=0;
  res->parent=0;
  return res;
}

node *mknode(short token,node *a,node *b)
{
  node *res;
  res = mkemptynode();
  CAR(res) = a;
  CDR(res) = b;
  res->node_info = 0;
  res->tree_info = 0;
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
  }
  res->token = token;
  res->type = 0;

  /* We try to optimize most things, but argument lists are hard... */
  if(token != F_ARG_LIST && (a || b))
    res->node_info |= OPT_TRY_OPTIMIZE;

  if(a) a->parent = res;
  if(b) b->parent = res;
  if(!num_parse_error) optimize(res);

  return res;
}

node *mkstrnode(struct pike_string *str)
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
  return res;
}

node *mkintnode(int nr)
{
  node *res = mkemptynode();
  res->token = F_CONSTANT;
  if(nr)
    copy_shared_string(res->type, int_type_string);
  else
    copy_shared_string(res->type, mixed_type_string);
  res->node_info = 0; 
  res->u.sval.type = T_INT;
  res->u.sval.subtype = NUMBER_NUMBER;
  res->u.sval.u.integer = nr;
  return res;
}

node *mkfloatnode(FLOAT_TYPE foo)
{
  node *res = mkemptynode();
  res->token = F_CONSTANT;
  copy_shared_string(res->type, float_type_string);
  res->u.sval.type = T_FLOAT;
#ifdef __CHECKER__
  res->u.sval.subtype = 0;
#endif
  res->u.sval.u.float_number = foo;
  return res;
}

node *mkapplynode(node *func,node *args)
{
  return mknode(F_APPLY, func, args);
}

node *mkefuncallnode(char *function, node *args)
{
  struct pike_string *name;
  node *n;
  name = findstring(function);
  if(!name || !find_module_identifier(name))
  {
    my_yyerror("Internally used efun undefined: %s",function);
    return mkintnode(0);
  }
  n=mkapplynode(mksvaluenode(sp-1), args);
  pop_stack();
  return n;
}

node *mkopernode(char *oper_id, node *arg1, node *arg2)
{
  if(arg1 && arg2)
    arg1=mknode(F_ARG_LIST,arg1,arg2);

  return mkefuncallnode(oper_id, arg1);
}

node *mklocalnode(int var)
{
  node *res = mkemptynode();
  res->token = F_LOCAL;
  copy_shared_string(res->type, local_variables->variable[var].type);
  res->node_info = OPT_NOT_CONST;
#ifdef __CHECKER__
  CDR(res)=0;
#endif
  res->u.number = var;
  return res;
}

node *mkidentifiernode(int i)
{
  node *res = mkemptynode();
  res->token = F_IDENTIFIER;
  setup_fake_program();
  copy_shared_string(res->type, ID_FROM_INT(&fake_program, i)->type);

  /* FIXME */
  if(IDENTIFIER_IS_CONSTANT(ID_FROM_INT(&fake_program, i)->flags))
  {
    res->node_info = OPT_EXTERNAL_DEPEND;
  }else{
    res->node_info = OPT_NOT_CONST;
  }

#ifdef __CHECKER__
  CDR(res)=0;
#endif
  res->u.number = i;
  return res;
}

node *mkcastnode(struct pike_string *type,node *n)
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

  CAR(res) = n;
#ifdef __CHECKER__
  CDR(res)=0;
#endif
  n->parent=res;
  return res;
}

void resolv_constant(node *n)
{
  struct identifier *i;
  if(!n)
  {
    push_int(0);
  }else{
    switch(n->token)
    {
    case F_CONSTANT:
      push_svalue(& n->u.sval);
      break;

    case F_IDENTIFIER:
      setup_fake_program();
      i=ID_FROM_INT(& fake_program, n->u.number);
	
      if(IDENTIFIER_IS_CONSTANT(i->flags))
      {
	push_svalue(PROG_FROM_INT(&fake_program, n->u.number)->constants +
		    i->func.offset);
      }else{
	yyerror("Identifier is not a constant");
	push_int(0);
      }
      break;

    case F_LOCAL:
	yyerror("Expected constant, got local variable");
	push_int(0);

    case F_GLOBAL:
	yyerror("Expected constant, got global variable");
	push_int(0);
    }
  }
}

node *index_node(node *n, struct pike_string * id)
{
  node *ret;
  JMP_BUF tmp;
  if(SETJMP(tmp))
  {
    ONERROR tmp;
    SET_ONERROR(tmp,exit_on_error,"Error in handle_error in master object!");
    assign_svalue_no_free(sp++, & throw_value);
    APPLY_MASTER("handle_error", 1);
    pop_stack();
    UNSET_ONERROR(tmp);
    
    yyerror("Couldn't index module.");
    push_svalue(0);
  }else{
    resolv_constant(n);
    push_string(id);
    reference_shared_string(id);
    f_index(2);
  }
  UNSETJMP(tmp);
  ret=mkconstantsvaluenode(sp-1);
  pop_stack();
  return ret;
}


int node_is_eq(node *a,node *b)
{
  if(a == b) return 1;
  if(!a || !b) return 0;
  if(a->token != b->token) return 0;

  switch(a->token)
  {
  case F_LOCAL:
  case F_IDENTIFIER:
    return a->u.number == b->u.number;

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

node *mkconstantsvaluenode(struct svalue *s)
{
  node *res = mkemptynode();
  res->token = F_CONSTANT;
  assign_svalue_no_free(& res->u.sval, s);
  if(s->type == T_OBJECT || s->type==T_FUNCTION)
    res->node_info|=OPT_EXTERNAL_DEPEND;
  res->type = get_type_of_svalue(s);
  return res;
}

node *mkliteralsvaluenode(struct svalue *s)
{
  node *res = mkconstantsvaluenode(s);

  if(s->type!=T_STRING && s->type!=T_INT && s->type!=T_FLOAT)
    res->node_info|=OPT_EXTERNAL_DEPEND;

  return res;
}

node *mksvaluenode(struct svalue *s)
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
    if(s->u.object == &fake_object)
    {
      return mkefuncallnode("this_object", 0);
/*  }else{
      yyerror("Non-constant object pointer! (should not happen!)");
 */
    }
    break;

  case T_FUNCTION:
  {
    if(s->subtype != FUNCTION_BUILTIN)
    {
      if(s->u.object == &fake_object)
	return mkidentifiernode(s->subtype);

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
  if(!n) return n;
  switch(n->token)
  {
  case F_LOCAL:
  case F_IDENTIFIER:
    b=mkintnode(0);
    *b=*n;
    copy_shared_string(b->type, n->type);
    return b;

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
  }
  b->line_number = n->line_number;
  b->node_info = n->node_info;
  b->tree_info = n->tree_info;
  return b;
}


int is_const(node *n)
{
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

node **last_cmd(node **a)
{
  node **n;
  if(!a || !*a) return (node **)NULL;
  if((*a)->token == F_CAST) return last_cmd(&CAR(*a));
  if((*a)->token != F_ARG_LIST) return a;
  if(CDR(*a))
  {
    if(CDR(*a)->token != F_CAST && CAR(*a)->token != F_ARG_LIST)
      return &CDR(*a);
    if((n=last_cmd(&CDR(*a))))
      return n;
  }
  if(CAR(*a))
  {
    if(CAR(*a)->token != F_CAST && CAR(*a)->token != F_ARG_LIST)
      return &CAR(*a);
    if((n=last_cmd(&CAR(*a))))
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
    if((n=low_get_arg(&CAR(*a),nr)))
      return n;

  if(CDR(*a))
    if((n=low_get_arg(&CDR(*a),nr)))
      return n;

  return 0;
}

node **my_get_arg(node **a,int n) { return low_get_arg(a,&n); }
/* static node **first_arg(node **a) { return my_get_arg(a,0); } */

static void low_print_tree(node *foo,int needlval)
{
  if(!foo) return;
  switch(foo->token)
  {
  case F_LOCAL:
    if(needlval) putchar('&');
    printf("$%ld",(long)foo->u.number);
    break;

  case '?':
    printf("(");
    low_print_tree(CAR(foo),0);
    printf(")?(");
    low_print_tree(CADR(foo),0);
    printf("):(");
    low_print_tree(CDDR(foo),0);
    printf(")");
    break;

  case F_IDENTIFIER:
    if(needlval) putchar('&');
    setup_fake_program();
    printf("%s",ID_FROM_INT(&fake_program, foo->u.number)->name->str);
    break;

  case F_ASSIGN:
    low_print_tree(CDR(foo),1);
    printf("=");
    low_print_tree(CAR(foo),0);
    break;

  case F_CAST:
  {
    char *s;
    init_buf();
    low_describe_type(foo->type->str);
    s=simple_free_buf();
    printf("(%s){",s);
    free(s);
    low_print_tree(CAR(foo),0);
    printf("}");
    break;
  }

  case F_ARG_LIST:
    low_print_tree(CAR(foo),0);
    if(CAR(foo) && CDR(foo))
    {
      if(CAR(foo)->type == void_type_string &&
	 CDR(foo)->type == void_type_string)
	printf(";\n");
      else
	putchar(',');
    }
    low_print_tree(CDR(foo),needlval);
    return;

  case F_LVALUE_LIST:
    low_print_tree(CAR(foo),1);
    if(CAR(foo) && CDR(foo)) putchar(',');
    low_print_tree(CDR(foo),1);
    return;

  case F_CONSTANT:
  {
    char *s;
    init_buf();
    describe_svalue(& foo->u.sval, 0, 0);
    s=simple_free_buf();
    printf("%s",s);
    free(s);
    break;
  }

  case F_VAL_LVAL:
    low_print_tree(CAR(foo),0);
    printf(",&");
    low_print_tree(CDR(foo),0);
    return;

  case F_APPLY:
    low_print_tree(CAR(foo),0);
    printf("(");
    low_print_tree(CDR(foo),0);
    printf(")");
    return;

  default:
    if(!car_is_node(foo) && !cdr_is_node(foo))
    {
      printf("%s",get_token_name(foo->token));
      return;
    }
    if(foo->token<256)
    {
      printf("%c(",foo->token);
    }else{
      printf("%s(",get_token_name(foo->token));
    }
    if(car_is_node(foo)) low_print_tree(CAR(foo),0);
    if(car_is_node(foo) && cdr_is_node(foo))
      putchar(',');
    if(cdr_is_node(foo)) low_print_tree(CDR(foo),0);
    printf(")");
    return;
  }
}

void print_tree(node *n)
{
  low_print_tree(n,0);
  printf("\n");
  fflush(stdout);
}


/* The following routines needs much better commenting */
struct used_vars
{
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
    q=p->locals+n->u.number;
    goto set_pointer;

  case F_IDENTIFIER:
    q=p->globals+n->u.number;

  set_pointer:
    if(overwrite)
    {
      if(*q == VAR_UNUSED && !noblock) *q = VAR_BLOCKED;
    }
    else
    {
      if(*q != VAR_UNUSED) *q = VAR_USED;
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
    if(lvalue) p->locals[n->u.number]=VAR_USED;
    break;

  case F_GLOBAL:
    if(lvalue) p->globals[n->u.number]=VAR_USED;
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

  case F_SSCANF:
    find_written_vars(CAR(n), p, 0);
    find_written_vars(CDR(n), p, 1);
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
  for(e=0;e<MAX_LOCAL;e++) aa.locals[e]=bb.locals[e]=VAR_UNUSED;
  for(e=0;e<MAX_GLOBAL;e++) aa.globals[e]=bb.globals[e]=VAR_UNUSED;

  find_used_variables(a,&aa,0,0);
  find_written_vars(b,&bb,0);

  for(e=0;e<MAX_LOCAL;e++)
    if(aa.locals[e]==VAR_USED && bb.locals[e]!=VAR_UNUSED)
      return 1;

  for(e=0;e<MAX_GLOBAL;e++)
    if(aa.globals[e]==VAR_USED && bb.globals[e]!=VAR_UNUSED)
      return 1;
  return 0;
}

static int depend_p(node *a,node *b)
{
  if(!b) return 0;
  if(!(b->tree_info & OPT_SIDE_EFFECT) && 
     (b->tree_info & OPT_EXTERNAL_DEPEND))
    return 1;
    
  return depend_p2(a,b);
}

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

  case F_VAL_LVAL:
  case F_LVALUE_LIST:
  case F_ARG_LIST:
    return cntargs(CAR(n))+cntargs(CDR(n));

    /* this might not be true, but it doesn't matter very much */
  default: return 1;
  }
}

static void low_build_function_type(node *n)
{
  if(!n) return;
  switch(n->token)
  {
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
       !match_types(CDR(n)->type,CAR(n)->type))
      my_yyerror("Bad type in assignment.");
    copy_shared_string(n->type, CDR(n)->type);
    break;

  case F_INDEX:
    type_a=CAR(n)->type;
    type_b=CDR(n)->type;
    if(!check_indexing(type_a, type_b, n))
      my_yyerror("Indexing on illegal type.");
    n->type=index_type(type_a,n);
    break;

  case F_ARROW:
    type_a=CAR(n)->type;
    type_b=CDR(n)->type;
    if(!check_indexing(type_a, type_b, n))
      my_yyerror("Indexing on illegal type.");
    n->type=index_type(type_a,n);
    break;

  case F_APPLY:
  {
    struct pike_string *s;
    struct pike_string *f;
    push_type(T_MIXED); /* match any return type, even void */
    push_type(T_VOID); /* not varargs */
    push_type(T_MANY);
    low_build_function_type(CDR(n));
    push_type(T_FUNCTION);
    s=pop_type();
    f=CAR(n)->type?CAR(n)->type:mixed_type_string;
    n->type=check_call(s,f);

    if(!n->type)
    {
      char *name;
      int args;
      switch(CAR(n)->token)
      {
      case F_IDENTIFIER:
	setup_fake_program();
	name=ID_FROM_INT(&fake_program, CAR(n)->u.number)->name->str;
	break;
	
      case F_CONSTANT:
      default:
	name="function call";
      }

      if(max_correct_args == count_arguments(s))
      {
	my_yyerror("To few arguments to %s.\n",name);
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
    if(local_variables &&
       local_variables->current_return_type &&
       !match_types(local_variables->current_return_type,CAR(n)->type) &&
       !(
	 local_variables->current_return_type==void_type_string &&
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
  n->node_info &=~ OPT_TRY_OPTIMIZE;
  n->tree_info &=~ OPT_TRY_OPTIMIZE;
  if(car_is_node(n)) zapp_try_optimize(CAR(n));
  if(cdr_is_node(n)) zapp_try_optimize(CDR(n));
}

static void optimize(node *n)
{
  node *tmp1, *tmp2, *tmp3;
  INT32 save_line = current_line;
  do
  {
    if(car_is_node(n) && !(CAR(n)->node_info & OPT_OPTIMIZED))
    {
      n=CAR(n);
      continue;
    }
    if(cdr_is_node(n) && !(CDR(n)->node_info & OPT_OPTIMIZED))
    {
      n=CDR(n);
      continue;
    }
    current_line = n->line_number;

    if(!n->parent) break;
    
    n->tree_info = n->node_info;
    if(car_is_node(n)) n->tree_info |= CAR(n)->tree_info;
    if(cdr_is_node(n)) n->tree_info |= CDR(n)->tree_info;

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
	CAR(n) = eval(CAR(n));
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
	CDR(n) = eval(CDR(n));
	if(CDR(n)) CDR(n)->parent = n;
	zapp_try_optimize(CDR(n)); /* avoid infinite loops */
	continue;
      }
    }
    fix_type_field(n);

#ifdef DEBUG
    if(l_flag > 3)
    {
      fprintf(stderr,"Optimizing: ");
      print_tree(n);
    }
#endif    

    switch(n->token)
    {
    case F_APPLY:
      if(CAR(n)->token == F_CONSTANT &&
	 CAR(n)->u.sval.type == T_FUNCTION &&
	 CAR(n)->u.sval.subtype == FUNCTION_BUILTIN && /* driver fun? */
	 CAR(n)->u.sval.u.efun->optimize)
      {
	if(tmp1=CAR(n)->u.sval.u.efun->optimize(n))
	  goto use_tmp1;
      }
      break;

    case F_ARG_LIST:
    case F_LVALUE_LIST:
      if(!CAR(n)) goto use_cdr;
      if(!CDR(n)) goto use_car;

      /*
       * { X; break;  Y; } -> { X; return; }
       * { X; return;  Y; } -> { X; return; }
       * { X; continue; Y; } -> { X; return; }
       */
      if((CAR(n)->token==F_RETURN ||
	  CAR(n)->token==F_BREAK ||
	  CAR(n)->token==F_CONTINUE ||
	  (CAR(n)->token==F_ARG_LIST &&
	   CDAR(n) &&
	   (CDAR(n)->token==F_RETURN ||
	    CDAR(n)->token==F_BREAK ||
	    CDAR(n)->token==F_CONTINUE))) &&
	 !(CDR(n)->tree_info & OPT_CASE))
	goto use_car;

      break;

    case F_LOR:
      /* !x || !y -> !(x && y) */
      if(CAR(n)->token==F_NOT && CDR(n)->token==F_NOT)
      {
	tmp1=mknode(F_NOT,mknode(F_LAND,CAAR(n),CADR(n)),0);
	CAAR(n)=CADR(n)=0;
	goto use_tmp1;
      }
      break;

    case F_LAND: 
      /* !x && !y -> !(x || y) */
      if(CAR(n)->token==F_NOT && CDR(n)->token==F_NOT)
      {
	tmp1=mknode(F_NOT,mknode(F_LOR,CAAR(n),CADR(n)),0);
	CAAR(n)=CADR(n)=0;
	goto use_tmp1;
      }
      break;

    case '?':
      /* (! X) ? Y : Z  -> X ? Z : Y */
      if(CAR(n)->token == F_NOT)
      {
	tmp1=mknode('?',CAAR(n),mknode(':',CDDR(n),CADR(n)));
	CAAR(n)=CDDR(n)=CADR(n)=0;
	goto use_tmp1;
      }
      break;

    case F_ADD_EQ:
      if(CDR(n)->type == int_type_string)
      {
	if(CDR(n)->token == F_CONSTANT && CDR(n)->u.sval.type == T_INT)
	{
	  /* a+=0 -> a */
	  if(n->u.sval.u.integer == 0) goto use_car;

	  /* a+=1 -> ++a */
	  if(n->u.sval.u.integer == 1)
	  {
	    tmp1=mknode(F_INC,CDR(n),0);
	    CDR(n)=0;
	    goto use_tmp1;
	  }

	  /* a+=-1 -> --a */
	  if(n->u.sval.u.integer == -1)
	  {
	    tmp1=mknode(F_DEC, CDR(n), 0);
	    CDR(n)=0;
	    goto use_tmp1;
	  }
	}
      }
      break;

    case F_SUB_EQ:
      if(CDR(n)->type == int_type_string)
      {
	if(CDR(n)->token == F_CONSTANT && CDR(n)->u.sval.type == T_INT)
	{
	  /* a-=0 -> a */
	  if(n->u.sval.u.integer == 0) goto use_car;

	  /* a-=-1 -> ++a */
	  if(n->u.sval.u.integer == -1)
	  {
	    tmp1=mknode(F_INC, CDR(n), 0);
	    CDR(n)=0;
	    goto use_tmp1;
	  }

	  /* a-=-1 -> --a */
	  if(n->u.sval.u.integer == 1)
	  {
	    tmp1=mknode(F_DEC, CDR(n), 0);
	    CDR(n)=0;
	    goto use_tmp1;
	  }
	}
      }
      break;

    case F_FOR:
    {
      node **last;
      int inc;
      int token;

      /* for(;0; X) Y; -> 0; */
      if(node_is_false(CAR(n)))
      {
	tmp1=mkintnode(0);
	goto use_tmp1;
      }

      /* for(;1;); -> for(;1;) sleep(255); (saves cpu) */
      if(node_is_true(CAR(n)) &&
	 (!CDR(n) || (CDR(n)->token==':' && !CADR(n) && !CDDR(n))))
      {
	tmp1=mknode(F_FOR, CAR(n), mknode(':',mkefuncallnode("sleep",mkintnode(255)),0));
	CAR(n)=0;
	goto use_tmp1;
      }

      /*
       * if X and Y are free from 'continue' or X is null,
       * then the following optimizations can be done:
       * for(;++e; X) Y; -> ++ne_loop(e, -1) { Y ; X }
       * for(;e++; X) Y; -> ++ne_loop(e, 0) { Y; X }
       * for(;--e; X) Y; -> --ne_loop(e, 1) { Y; X }
       * for(;e--; X) Y; -> --ne_loop(e, 0) { Y; X }
       */
      if(CAR(n) &&
	 (CAR(n)->token==F_INC ||
	  CAR(n)->token==F_POST_INC ||
	  CAR(n)->token==F_DEC ||
	  CAR(n)->token==F_POST_DEC ) &&
	 (!CDDR(n) || !(CDDR(n)->tree_info & OPT_CONTINUE)) &&
	 (!CADR(n) || !(CADR(n)->tree_info & OPT_CONTINUE)) )
      {
	/* Check which of the above cases.. */
	switch(CAR(n)->token)
	{
	case F_POST_DEC: token=F_DEC_NEQ_LOOP; inc=-1; break;
	case F_DEC:      token=F_DEC_NEQ_LOOP; inc=0; break;
	case F_POST_INC: token=F_INC_NEQ_LOOP; inc=1; break;
	case F_INC:      token=F_INC_NEQ_LOOP; inc=0; break;
	default: fatal("Impossible error\n"); return;
	}

	/* Build new tree */
	tmp1=mknode(token,
		    mknode(F_VAL_LVAL,
			   mkintnode(inc),
			   CAAR(n)),
		    mknode(F_ARG_LIST,
			   mkcastnode(void_type_string, CADR(n)),
			   mkcastnode(void_type_string, CDDR(n))));
	CDDR(n)=CADR(n)=CAAR(n)=0;
	goto use_tmp1;
      }

      /* Last is a pointer to the place where the incrementor is in the
       * tree. This is needed so we can nullify this pointer later and
       * free the rest of the tree
       */
      last=&(CDDR(n));
      tmp1=*last;

      /* We're not interested in casts to void */
      while(tmp1 && 
	    tmp1->token == F_CAST &&
	    tmp1->type == void_type_string)
      {
	last=&CAR(tmp1);
	tmp1=*last;
      }

      /* If there is an incrementor, and it is one of x++, ++x, x-- or ++x */
      if(tmp1 && (tmp1->token==F_INC ||
		  tmp1->token==F_POST_INC ||
		  tmp1->token==F_DEC ||
		  tmp1->token==F_POST_DEC))
      {
	node *opnode, **arg1, **arg2;
	int oper;

	/* does it increment or decrement ? */
	if(tmp1->token==F_INC || tmp1->token==F_POST_INC)
	  inc=1;
	else
	  inc=0;

	/* for(; arg1 oper arg2; z ++) p; */

	opnode=CAR(n);

	if(opnode->token == F_APPLY &&
	   CAR(opnode) &&
	   CAR(opnode)->token == F_CONSTANT &&
	   CAR(opnode)->u.sval.type == T_FUNCTION &&
	   CAR(opnode)->u.sval.subtype == FUNCTION_BUILTIN)
	{
	  if(CAR(opnode)->u.sval.u.efun->function == f_gt)
	    oper=F_GT;
	  else if(CAR(opnode)->u.sval.u.efun->function == f_ge)
	    oper=F_GE;
	  else if(CAR(opnode)->u.sval.u.efun->function == f_lt)
	    oper=F_LT;
	  else if(CAR(opnode)->u.sval.u.efun->function == f_le)
	    oper=F_LE;
	  else if(CAR(opnode)->u.sval.u.efun->function == f_ne)
	    oper=F_NE;
	  else
	    break;
	}else{
	  break;
	}

	if(count_args(CDR(opnode)) != 2) break;
	arg1=my_get_arg(&CDR(opnode), 0);
	arg2=my_get_arg(&CDR(opnode), 1);

	/* it was not on the form for(; x op y; z++) p; */
	if(!node_is_eq(*arg1,CAR(tmp1)) || /* x == z */
	   depend_p(*arg2,*arg2) ||	/* does y depend on y? */
	   depend_p(*arg2,*arg1) ||	/* does y depend on x? */
	   depend_p(*arg2,CADR(n))) /* does y depend on p? */
	{
	  /* it was not on the form for(; x op y; x++) p; */
	  if(!node_is_eq(*arg2,CAR(tmp1)) || /* y == z */
	     depend_p(*arg1,*arg2) || /* does x depend on y? */
	     depend_p(*arg1,*arg1) || /* does x depend on x? */
	     depend_p(*arg1,CADR(n)) /* does x depend on p? */
	     )
	  {
	    /* it was not on the form for(; x op y; y++) p; */
	    break;
	  }else{
	    node **tmparg;
	    /* for(; x op y; y++) p; -> for(; y op^-1 x; y++) p; */

	    switch(oper)
	    {
	    case F_LT: oper=F_GT; break;
	    case F_LE: oper=F_GE; break;
	    case F_GT: oper=F_LT; break;
	    case F_GE: oper=F_LE; break;
	    }
	    
	    tmparg=arg1;
	    arg1=arg2;
	    arg2=tmparg;
	  }
	}
	if(inc)
	{
	  if(oper==F_LE)
	    tmp3=mkopernode("`+",*arg2,mkintnode(1));
	  else if(oper==F_LT)
	    tmp3=*arg2;
	  else
	    break;
	}else{
	  if(oper==F_GE)
	    tmp3=mkopernode("`-",*arg2,mkintnode(1));
	  else if(oper==F_GT)
	    tmp3=*arg2;
	  else
	    break;
	}

	*last=0;
	if(oper==F_NE)
	{
	  if(inc)
	    token=F_INC_NEQ_LOOP;
	  else
	    token=F_DEC_NEQ_LOOP;
	}else{
	  if(inc)
	    token=F_INC_LOOP;
	  else
	    token=F_DEC_LOOP;
	}
	tmp2=mknode(token,mknode(F_VAL_LVAL,tmp3,*arg1),CADR(n));
	*arg1 = *arg2 = CADR(n) =0;

	if(inc)
	{
	  tmp1->token=F_DEC;
	}else{
	  tmp1->token=F_INC;
	}

	tmp1=mknode(F_ARG_LIST,mkcastnode(void_type_string,tmp1),tmp2);
	goto use_tmp1;
      }
      break;
    }

    use_car:
      tmp1=CAR(n);
      CAR(n)=0;
      goto use_tmp1;

    use_cdr:
      tmp1=CDR(n);
      CDR(n)=0;
      goto use_tmp1;

    use_tmp1:
      if(CAR(n->parent) == n)
	CAR(n->parent) = tmp1;
      else
	CDR(n->parent) = tmp1;
      
      if(tmp1) tmp1->parent = n->parent;
      free_node(n);
      n=tmp1;
#ifdef DEBUG
      if(l_flag > 3)
      {
	fprintf(stderr,"Result: ");
	print_tree(n);
      }
#endif    
      continue;

    }
    n->node_info |= OPT_OPTIMIZED;
    n=n->parent;
  }while(n);
  current_line = save_line;
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
    throw();
  }
}

int eval_low(node *n)
{
  unsigned INT16 num_strings, num_constants;
  INT32 jump;
  struct svalue *save_sp = sp;
  int ret;

  if(num_parse_error) return -1;
  setup_fake_program();

  num_strings=fake_program.num_strings;
  num_constants=fake_program.num_constants;
  jump=PC;

  store_linenumbers=0;
  docode(n);
  ins_f_byte(F_DUMB_RETURN);
  store_linenumbers=1;

  setup_fake_program();
  ret=-1;
  if(!num_parse_error)
  {
    struct callback *tmp_callback;
    struct timer_oflo foo;

    /* This is how long we try to optimize before giving up... */
    foo.counter=10000;
    foo.yes=0;

    setup_fake_object();

    tmp_callback=add_to_callback(&evaluator_callbacks,
				 check_evaluation_time,
				 (void *)&foo,0);
				 
    if(apply_low_safe_and_stupid(&fake_object, jump))
    {
      /* Generate error message */
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
      }else{
	yyerror("Nonstandard error format.");
      }
    }else{
      if(foo.yes)
	pop_n_elems(sp-save_sp);
      else
	ret=sp-save_sp;
    }

    remove_callback(tmp_callback);
  }

  while(fake_program.num_strings > num_strings)
  {
    fake_program.num_strings--;
    free_string(fake_program.strings[fake_program.num_strings]);
    areas[A_STRINGS].s.len-=sizeof(struct pike_string *);
  }

  while(fake_program.num_constants > num_constants)
  {
    fake_program.num_constants--;
    free_svalue(fake_program.constants + fake_program.num_constants);
    areas[A_CONSTANTS].s.len-=sizeof(struct svalue);
  }

  areas[A_PROGRAM].s.len=jump;

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
    free_node(n);
    n=0;
    break;

  case 1:
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
  return n;
}

INT32 last_function_opt_info;

void dooptcode(struct pike_string *name,node *n, int args)
{
#ifdef DEBUG
  if(a_flag > 1)
    fprintf(stderr,"Doing function '%s' at %x\n",name->str,PC);
#endif
  last_function_opt_info=OPT_SIDE_EFFECT;
  n=mknode(F_ARG_LIST,n,0);
#ifdef DEBUG
  if(a_flag > 2)
  {
    fprintf(stderr,"Coding: ");
    print_tree(n);
  }
#endif
  if(!num_parse_error)
  {
    do_code_block(n);
    last_function_opt_info = n->tree_info;
  }
  free_node(n);
}

INT32 get_opt_info() { return last_function_opt_info; }

