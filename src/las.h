/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: las.h,v 1.16 1998/08/29 22:15:18 grubba Exp $
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
  struct local_variable variable[MAX_LOCAL];
};

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
node *mknode(short token,node *a,node *b);
node *mkstrnode(struct pike_string *str);
node *mkintnode(int nr);
node *mkfloatnode(FLOAT_TYPE foo);
node *mkapplynode(node *func,node *args);
node *mkefuncallnode(char *function, node *args);
node *mkopernode(char *oper_id, node *arg1, node *arg2);
node *mklocalnode(int var);
node *mkidentifiernode(int i);
node *mkexternalnode(int level,
		     int i,
		     struct identifier *id);
node *mkcastnode(struct pike_string *type,node *n);
void resolv_constant(node *n);
void resolv_program(node *n);
node *index_node(node *n, char *node_name, struct pike_string * id);
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
/* Prototypes end here */

#define CAR(n) ((n)->u.node.a)
#define CDR(n) ((n)->u.node.b)
#define CAAR(n) CAR(CAR(n))
#define CADR(n) CAR(CDR(n))
#define CDAR(n) CDR(CAR(n))
#define CDDR(n) CDR(CDR(n))

#define GAUGE_RUSAGE_INDEX 0

#define add_to_mem_block(N,Data,Size) low_my_binary_strcat(Data,Size,areas+N)
#define IDENTIFIERP(i) (new_program->identifier_references+(i))
#define INHERIT(i) (new_program->inherits+(i))
#define PC (new_program->num_program)

#ifndef DEBUG
#define check_tree(X,Y)
#endif

#endif
