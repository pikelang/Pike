/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef LAS_H
#define LAS_H

#include "global.h"
#include "svalue.h"
#include "dynamic_buffer.h"
#include "program.h"

#define MAX_GLOBAL_VARIABLES 1000

struct local_variable
{
  struct pike_string *name;
  struct pike_string *type;
};

struct locals
{
  struct locals *next;
  struct pike_string *current_type;
  struct pike_string *current_return_type;
  int current_number_of_locals;
  int max_number_of_locals;
  struct local_variable variable[MAX_LOCAL];
};

void yyerror(char *s);
int islocal(struct pike_string *str);
int verify_declared(struct pike_string *str);

struct node_s
{
  unsigned INT16 token;
  INT16 line_number;
  INT16 node_info;
  INT16 tree_info;
  struct pike_string *type;
  struct node_s *parent;
  union 
  {
    int number;
    struct svalue sval;
    struct
    {
      struct node_s *a,*b;
    } node;
  } u;
};

typedef struct node_s node;

extern struct locals *local_variables;
extern node *init_node;
extern int num_parse_error;
extern int cumulative_parse_error;

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
INT32 count_args(node *n);
struct node_chunk;
void free_all_nodes();
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
node *mkcastnode(struct pike_string *type,node *n);
void resolv_constant(node *n);
node *index_node(node *n, struct pike_string * id);
int node_is_eq(node *a,node *b);
node *mkconstantsvaluenode(struct svalue *s);
node *mkliteralsvaluenode(struct svalue *s);
node *mksvaluenode(struct svalue *s);
node *copy_node(node *n);
int is_const(node *n);
int node_is_tossable(node *n);
int node_is_true(node *n);
int node_is_false(node *n);
node **last_cmd(node **a);
node **my_get_arg(node **a,int n);
void print_tree(node *n);
struct used_vars;
void fix_type_field(node *n);
struct timer_oflo;
int eval_low(node *n);
int dooptcode(struct pike_string *name,
	      node *n,
	      struct pike_string *type,
	      int modifiers);
INT32 get_opt_info();
/* Prototypes end here */

#define CAR(n) ((n)->u.node.a)
#define CDR(n) ((n)->u.node.b)
#define CAAR(n) CAR(CAR(n))
#define CADR(n) CAR(CDR(n))
#define CDAR(n) CDR(CAR(n))
#define CDDR(n) CDR(CDR(n))

#define GAUGE_RUSAGE_INDEX 0

#define A_PROGRAM 0
#define A_STRINGS 1
#define A_INHERITS 2
#define A_IDENTIFIERS 3
#define A_IDENTIFIER_REFERENCES 4
#define A_CONSTANTS 5
#define A_LINENUMBERS 6
#define NUM_AREAS 7

#define add_to_mem_block(N,Data,Size) low_my_binary_strcat(Data,Size,areas+N)
#define IDENTIFIERP(i) (((struct reference *)areas[A_IDENTIFIER_REFERENCES].s.str)+i)
#define INHERIT(i) (((struct inherit *)areas[A_INHERITS].s.str)+i)
#define PC (areas[A_PROGRAM].s.len)

extern dynamic_buffer areas[NUM_AREAS];

#endif
