/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: las.h,v 1.30 1999/11/23 03:24:47 grubba Exp $
 */
#ifndef LAS_H
#define LAS_H

#include "global.h"
#include "pike_types.h"
#include "svalue.h"
#include "dynamic_buffer.h"
#include "program.h"

#define MAX_GLOBAL_VARIABLES 1000
typedef void (*c_fun)(INT32);


void yyerror(char *s);
int islocal(struct pike_string *str);
int verify_declared(struct pike_string *str);


extern node *init_node;
extern int num_parse_error;
extern int cumulative_parse_error;
extern struct compiler_frame *compiler_frame;

struct local_variable
{
  struct pike_string *name;
  struct pike_string *type;
};

struct compiler_frame
{
  struct compiler_frame *previous;

  struct pike_string *current_type;
  struct pike_string *current_return_type;
  int current_number_of_locals;
  int max_number_of_locals;
  int lexical_scope;
  struct local_variable variable[MAX_LOCAL];
};

#ifdef SHARED_NODES

struct node_hash_table
{
  node **table;
  INT32 size;
};

extern struct node_hash_table node_hash;

#endif /* SHARED_NODES */

#define OPT_OPTIMIZED       0x1    /* has been processed by optimize(),
				    * only used in node_info
				    */
#define OPT_NOT_CONST       0x2    /* isn't constant */
#define OPT_SIDE_EFFECT     0x4    /* has side effects */
#define OPT_ASSIGNMENT      0x8    /* does assignments */
#define OPT_TRY_OPTIMIZE    0x10   /* might be worth optimizing */
#define OPT_EXTERNAL_DEPEND 0x20   /* the value depends on external
				    * influences (such as read_file or so)
				    */
#define OPT_CASE            0x40   /* contains case(s) */
#define OPT_CONTINUE        0x80   /* contains continue(s) */
#define OPT_BREAK           0x100  /* contains break(s) */
#define OPT_RETURN          0x200  /* contains return(s) */
#define OPT_TYPE_NOT_FIXED  0x400  /* type-field might be wrong */

#define OPT_DEFROSTED	    0x4000 /* Node may be a duplicate */
#define OPT_NOT_SHARED	    0x8000 /* Node is not to be shared */

/* Prototypes begin here */
int car_is_node(node *n);
int cdr_is_node(node *n);
void check_tree(node *n, int depth);
INT32 count_args(node *n);
struct pike_string *find_return_type(node *n);
struct node_chunk;
void free_all_nodes(void);
void debug_free_node(node *n);
node *debug_check_node_hash(node *n);
node *debug_mknode(short token,node *a,node *b);
node *debug_mkstrnode(struct pike_string *str);
node *debug_mkintnode(int nr);
node *debug_mknewintnode(int nr);
node *debug_mkfloatnode(FLOAT_TYPE foo);
node *debug_mkprgnode(struct program *p);
node *debug_mkapplynode(node *func,node *args);
node *debug_mkefuncallnode(char *function, node *args);
node *debug_mkopernode(char *oper_id, node *arg1, node *arg2);
node *debug_mklocalnode(int var, int depth);
node *debug_mkidentifiernode(int i);
node *debug_mktrampolinenode(int i);
node *debug_mkexternalnode(int level,
			   int i,
			   struct identifier *id);
node *debug_mkcastnode(struct pike_string *type,node *n);
node *debug_mksoftcastnode(struct pike_string *type,node *n);
void resolv_constant(node *n);
void resolv_class(node *n);
void resolv_class(node *n);
node *index_node(node *n, char *node_name, struct pike_string *id);
int node_is_eq(node *a,node *b);
node *debug_mkconstantsvaluenode(struct svalue *s);
node *debug_mkliteralsvaluenode(struct svalue *s);
node *debug_mksvaluenode(struct svalue *s);
node *copy_node(node *n);
int is_const(node *n);
int node_is_tossable(node *n);
int node_is_true(node *n);
int node_is_false(node *n);
int node_may_overload(node *n, int lfun);
node **last_cmd(node **a);
node **my_get_arg(node **a,int n);
node **is_call_to(node *n, c_fun f);
void print_tree(node *n);
struct used_vars;
void fix_type_field(node *n);
struct timer_oflo;
int eval_low(node *n);
int dooptcode(struct pike_string *name,
	      node *n,
	      struct pike_string *type,
	      int modifiers);
void resolv_program(node *n);
/* Prototypes end here */

/* Handling of nodes */
#define free_node(n)        debug_free_node(dmalloc_touch(node *,n))

#define mknode(token, a, b) dmalloc_touch(node *, debug_mknode(token, dmalloc_touch(node *, a), dmalloc_touch(node *, b)))
#define mkstrnode(str)      dmalloc_touch(node *, debug_mkstrnode(str))
#define mkintnode(nr)       dmalloc_touch(node *, debug_mkintnode(nr))
#define mknewintnode(nr)    dmalloc_touch(node *, debug_mknewintnode(nr))
#define mkfloatnode(foo)    dmalloc_touch(node *, debug_mkfloatnode(foo))
#define mkprgnode(p)        dmalloc_touch(node *, debug_mkprgnode(p))
#define mkapplynode(func, args) dmalloc_touch(node *, debug_mkapplynode(dmalloc_touch(node *, func),dmalloc_touch(node *, args)))
#define mkefuncallnode(function, args) dmalloc_touch(node *, debug_mkefuncallnode(function, dmalloc_touch(node *, args)))
#define mkopernode(oper_id, arg1, arg2) dmalloc_touch(node *, debug_mkopernode(oper_id, dmalloc_touch(node *, arg1), dmalloc_touch(node *, arg2)))
#define mklocalnode(var, depth) dmalloc_touch(node *, debug_mklocalnode(var, depth))
#define mkidentifiernode(i) dmalloc_touch(node *, debug_mkidentifiernode(i))
#define mktrampolinenode(i) dmalloc_touch(node *, debug_mktrampolinenode(i))
#define mkexternalnode(level, i, id) dmalloc_touch(node *, debug_mkexternalnode(level, i, id))
#define mkcastnode(type, n) dmalloc_touch(node *, debug_mkcastnode(type, dmalloc_touch(node *, n)))
#define mksoftcastnode(type, n) dmalloc_touch(node *, debug_mksoftcastnode(type, dmalloc_touch(node *, n)))
#define mkconstantsvaluenode(s) dmalloc_touch(node *, debug_mkconstantsvaluenode(dmalloc_touch(struct svalue *, s)))
#define mkliteralsvaluenode(s) dmalloc_touch(node *, debug_mkliteralsvaluenode(dmalloc_touch(struct svalue *, s)))
#define mksvaluenode(s)     dmalloc_touch(node *, debug_mksvaluenode(dmalloc_touch(struct svalue *, s)))


#if defined(PIKE_DEBUG) && defined(SHARED_NODES)
#define check_node_hash(X)	debug_check_node_hash(X)
#else /* !PIKE_DEBUG || !SHARED_NODES */
#define check_node_hash(X)	(X)
#endif /* PIKE_DEBUG && SHARED_NODES */

/* lvalue variants of CAR(n) & CDR(n) */
#define _CAR(n) (dmalloc_touch(node *,(n))->u.node.a)
#define _CDR(n) (dmalloc_touch(node *,(n))->u.node.b)
#define _CAAR(n) _CAR(_CAR(n))
#define _CADR(n) _CAR(_CDR(n))
#define _CDAR(n) _CDR(_CAR(n))
#define _CDDR(n) _CDR(_CDR(n))

#ifdef SHARED_NODES
#define ADD_NODE_REF(n)	(n?add_ref(n):0)
#define ADD_NODE_REF2(n, code)	do { n?add_ref(n):0; code; } while(0)
#else /* !SHARED_NODES */
#define ADD_NODE_REF(n)	(n = 0)
#define ADD_NODE_REF2(n, code)	do { code; n = 0;} while(0)
#endif /* SHARED_NODES */

#ifdef SHARED_NODES
#define CAR(n)  (check_node_hash(dmalloc_touch(node *, (n)->u.node.a)))
#define CDR(n)  (check_node_hash(dmalloc_touch(node *, (n)->u.node.b)))
#else /* !SHARED_NODES */
#define CAR(n) _CAR(n)
#define CDR(n) _CDR(n)
#endif /* SHARED_NODES */ 
#define CAAR(n) CAR(CAR(n))
#define CADR(n) CAR(CDR(n))
#define CDAR(n) CDR(CAR(n))
#define CDDR(n) CDR(CDR(n))
#define CAAAR(n) CAR(CAR(CAR(n)))
#define CAADR(n) CAR(CAR(CDR(n)))
#define CADAR(n) CAR(CDR(CAR(n)))
#define CADDR(n) CAR(CDR(CDR(n)))
#define CDAAR(n) CDR(CAR(CAR(n)))
#define CDADR(n) CDR(CAR(CDR(n)))
#define CDDAR(n) CDR(CDR(CAR(n)))
#define CDDDR(n) CDR(CDR(CDR(n)))
#define CAAAAR(n) CAR(CAR(CAR(CAR(n))))
#define CAAADR(n) CAR(CAR(CAR(CDR(n))))
#define CAADAR(n) CAR(CAR(CDR(CAR(n))))
#define CAADDR(n) CAR(CAR(CDR(CDR(n))))
#define CADAAR(n) CAR(CDR(CAR(CAR(n))))
#define CADADR(n) CAR(CDR(CAR(CDR(n))))
#define CADDAR(n) CAR(CDR(CDR(CAR(n))))
#define CADDDR(n) CAR(CDR(CDR(CDR(n))))
#define CDAAAR(n) CDR(CAR(CAR(CAR(n))))
#define CDAADR(n) CDR(CAR(CAR(CDR(n))))
#define CDADAR(n) CDR(CAR(CDR(CAR(n))))
#define CDADDR(n) CDR(CAR(CDR(CDR(n))))
#define CDDAAR(n) CDR(CDR(CAR(CAR(n))))
#define CDDADR(n) CDR(CDR(CAR(CDR(n))))
#define CDDDAR(n) CDR(CDR(CDR(CAR(n))))
#define CDDDDR(n) CDR(CDR(CDR(CDR(n))))

#define GAUGE_RUSAGE_INDEX 0

#define add_to_mem_block(N,Data,Size) low_my_binary_strcat(Data,Size,areas+N)
#define IDENTIFIERP(i) (new_program->identifier_references+(i))
#define INHERIT(i) (new_program->inherits+(i))
#define PC (new_program->num_program)

#ifndef PIKE_DEBUG
#define check_tree(X,Y)
#endif

#endif
