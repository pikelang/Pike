/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: las.h,v 1.24 1999/11/11 13:53:14 grubba Exp $
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

/* Prototypes begin here */
int car_is_node(node *n);
int cdr_is_node(node *n);
void check_tree(node *n, int depth);
INT32 count_args(node *n);
struct pike_string *find_return_type(node *n);
struct node_chunk;
void free_all_nodes(void);
void free_node(node *n);
node *debug_check_node_hash(node *n);
node *mknode(short token,node *a,node *b);
node *mkstrnode(struct pike_string *str);
node *mkintnode(int nr);
node *mknewintnode(int nr);
node *mkfloatnode(FLOAT_TYPE foo);
node *mkprgnode(struct program *p);
node *mkapplynode(node *func,node *args);
node *mkefuncallnode(char *function, node *args);
node *mkopernode(char *oper_id, node *arg1, node *arg2);
node *mklocalnode(int var, int depth);
node *mkidentifiernode(int i);
node *mkexternalnode(int level,
		     int i,
		     struct identifier *id);
node *mkcastnode(struct pike_string *type,node *n);
void resolv_constant(node *n);
void resolv_class(node *n);
void resolv_class(node *n);
node *index_node(node *n, char *node_name, struct pike_string *id);
int node_is_eq(node *a,node *b);
node *mkconstantsvaluenode(struct svalue *s);
node *mkliteralsvaluenode(struct svalue *s);
node *mksvaluenode(struct svalue *s);
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

#if defined(PIKE_DEBUG) && defined(SHARED_NODES)
#define check_node_hash(X)	debug_check_node_hash(X)
#else /* !PIKE_DEBUG || !SHARED_NODES */
#define check_node_hash(X)	(X)
#endif /* PIKE_DEBUG && SHARED_NODES */

#define _CAR(n) (dmalloc_touch(node *,(n))->u.node.a)
#define _CDR(n) (dmalloc_touch(node *,(n))->u.node.b)
#define _CAAR(n) _CAR(_CAR(n))
#define _CADR(n) _CAR(_CDR(n))
#define _CDAR(n) _CDR(_CAR(n))
#define _CDDR(n) _CDR(_CDR(n))
#define CAR(n)  (check_node_hash(dmalloc_touch(node *, (n)->u.node.a)))
#define CDR(n)  (check_node_hash(dmalloc_touch(node *, (n)->u.node.b)))
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

#define GAUGE_RUSAGE_INDEX 0

#define add_to_mem_block(N,Data,Size) low_my_binary_strcat(Data,Size,areas+N)
#define IDENTIFIERP(i) (new_program->identifier_references+(i))
#define INHERIT(i) (new_program->inherits+(i))
#define PC (new_program->num_program)

#ifndef PIKE_DEBUG
#define check_tree(X,Y)
#endif

#endif
