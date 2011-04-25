/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#ifndef LAS_H
#define LAS_H

#include "global.h"
#include "svalue.h"
#include "dynamic_buffer.h"

#define MAX_GLOBAL_VARIABLES 1000
typedef void (*c_fun)(INT32);


/* Flags used by yytype_error() */
#define YYTE_IS_WARNING	1

void yytype_error(char *msg, struct pike_type *expected_t,
		  struct pike_type *got_t, unsigned int flags);
void yyerror(char *s);
int islocal(struct pike_string *str);
int verify_declared(struct pike_string *str);
void cleanup_compiler(void);


extern int cumulative_parse_error;


#ifndef STRUCT_NODE_S_DECLARED
#define STRUCT_NODE_S_DECLARED
struct node_s;
typedef struct node_s node;
#endif

struct local_variable
{
  struct pike_string *name;
  struct pike_type *type;
  node *def;
  struct pike_string *file;
  int line;
};

struct compiler_frame
{
  struct compiler_frame *previous;

  struct pike_type *current_type;
  struct pike_type *current_return_type;
  int current_number_of_locals;
  int max_number_of_locals;
  int min_number_of_locals;
  int last_block_level; /* used to detect variables declared in same block */
  int num_args;
  int lexical_scope;
  int current_function_number;
  int recur_label;
  int is_inline;
  unsigned int opt_flags;
  struct local_variable variable[MAX_LOCAL];
};

/* Also used in struct node_identifier */
union node_data
{
  struct
  {
    int number;
    struct program *prog;
  } id;
  struct
  {
    int ident;
    struct compiler_frame *frame;
#ifdef SHARED_NODES
    struct program *prog;
#endif
  } trampoline;
  struct svalue sval;
  struct
  {
    struct node_s *a, *b;
  } node;
  struct
  {
    struct node_identifier *a, *b;
  } node_id;
  struct
  {
    int a, b;
  } integer;
};

struct node_s
{
#if defined(SHARED_NODES)
  unsigned INT32 refs;
  size_t hash;
  struct node_s *next;
#endif /* SHARED_NODES */
  struct pike_string *current_file;
  struct pike_type *type;
  struct pike_string *name;
  struct node_s *parent;
  unsigned INT16 line_number;
  unsigned INT16 node_info;
  unsigned INT16 tree_info;
  /* The stuff from this point on is hashed. */
  unsigned INT16 token;
  union node_data u;
};

#ifdef SHARED_NODES_MK2

struct node_identifier
{
  ptrdiff_t refs;
  struct node_identifier *next;
  size_t hash;
  INT16 token;
  union node_data u;
};

struct node_hash_table
{
  struct node_identifier **table;
  INT32 size;
};

extern struct node_hash_table node_hash;

#endif /* SHARED_NODES_MK2 */

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
#define OPT_WEAK_TYPE	    0x800  /* don't warn even if strict types */
#define OPT_APPLY           0x1000 /* contains apply */

#define OPT_DEFROSTED	    0x4000 /* Node may be a duplicate */
#define OPT_NOT_SHARED	    0x8000 /* Node is not to be shared */


/* This is a statement which got custom break/continue label handling.
 * Set in compiler_frame. Beware: This is not a node flag! -Hubbe */
#define OPT_CUSTOM_LABELS   0x10000

#define SCOPE_LOCAL 1
#define SCOPE_SCOPED 2
#define SCOPE_SCOPE_USED 4

/* Prototypes begin here */
int car_is_node(node *n);
int cdr_is_node(node *n);
void check_tree(node *n, int depth);
INT32 count_args(node *n);
struct pike_type *find_return_type(node *n);
int check_tailrecursion(void);
struct node_chunk;
void free_all_nodes(void);
void debug_free_node(node *n);
node *debug_check_node_hash(node *n);
node *debug_mknode(int token,node *a,node *b);
node *debug_mkstrnode(struct pike_string *str);
node *debug_mkintnode(INT_TYPE nr);
node *debug_mknewintnode(INT_TYPE nr);
node *debug_mkfloatnode(FLOAT_TYPE foo);
node *debug_mkprgnode(struct program *p);
node *debug_mkapplynode(node *func,node *args);
node *debug_mkefuncallnode(char *function, node *args);
node *debug_mkopernode(char *oper_id, node *arg1, node *arg2);
node *debug_mklocalnode(int var, int depth);
node *debug_mkidentifiernode(int i);
node *debug_mktrampolinenode(int i, struct compiler_frame *depth);
node *debug_mkexternalnode(struct program *prog, int i);
node *debug_mkcastnode(struct pike_type *type, node *n);
node *debug_mksoftcastnode(struct pike_type *type, node *n);
void resolv_constant(node *n);
void resolv_class(node *n);
void resolv_class(node *n);
node *recursive_add_call_arg(node *n, node *arg);
node *index_node(node *n, char *node_name, struct pike_string *id);
int node_is_eq(node *a,node *b);
node *debug_mktypenode(struct pike_type *t);
node *debug_mkconstantsvaluenode(struct svalue *s);
node *debug_mkliteralsvaluenode(struct svalue *s);
node *debug_mksvaluenode(struct svalue *s);
node *copy_node(node *n);
node *defrost_node(node *n);
void optimize_node(node *);
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
ptrdiff_t eval_low(node *n,int print_error);
int dooptcode(struct pike_string *name,
	      node *n,
	      struct pike_type *type,
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
#define mktrampolinenode(i,f) dmalloc_touch(node *, debug_mktrampolinenode(i, f))
#define mkexternalnode(parent_prog, i) dmalloc_touch(node *, debug_mkexternalnode(parent_prog, i))
#define mkcastnode(type, n) dmalloc_touch(node *, debug_mkcastnode(type, dmalloc_touch(node *, n)))
#define mksoftcastnode(type, n) dmalloc_touch(node *, debug_mksoftcastnode(type, dmalloc_touch(node *, n)))
#define mktypenode(t)       dmalloc_touch(node *, debug_mktypenode(t))
#define mkconstantsvaluenode(s) dmalloc_touch(node *, debug_mkconstantsvaluenode(dmalloc_touch(struct svalue *, s)))
#define mkliteralsvaluenode(s) dmalloc_touch(node *, debug_mkliteralsvaluenode(dmalloc_touch(struct svalue *, s)))
#define mksvaluenode(s)     dmalloc_touch(node *, debug_mksvaluenode(dmalloc_touch(struct svalue *, s)))

#define COPY_LINE_NUMBER_INFO(TO, FROM) do {				\
    node *to_ = (TO), *from_ = (FROM);					\
    to_->line_number = from_->line_number;				\
    free_string (to_->current_file);					\
    copy_shared_string (to_->current_file, from_->current_file);	\
  } while (0)


#if defined(PIKE_DEBUG) && (defined(SHARED_NODES) || defined(SHARED_NODES_MK2))
#define check_node_hash(X)	debug_check_node_hash(X)
#else /* !PIKE_DEBUG || (!SHARED_NODES && !SHARED_NODES_MK2) */
#define check_node_hash(X)	(X)
#endif /* PIKE_DEBUG && (SHARED_NODES || SHARED_NODES_MK2) */

/* lvalue variants of CAR(n) & CDR(n) */
#define _CAR(n) (dmalloc_touch(node *,(n))->u.node.a)
#define _CDR(n) (dmalloc_touch(node *,(n))->u.node.b)
#define _CAAR(n) _CAR(_CAR(n))
#define _CADR(n) _CAR(_CDR(n))
#define _CDAR(n) _CDR(_CAR(n))
#define _CDDR(n) _CDR(_CDR(n))

#ifdef SHARED_NODES
#define ADD_NODE_REF(n)	do { if (n) add_ref(n); } while(0)
#define ADD_NODE_REF2(n, code)	do { ADD_NODE_REF(n); code; } while(0)
#else /* !SHARED_NODES */
#define defrost_node(n) (n)
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
#define CAAAAAR(n) CAR(CAR(CAR(CAR(CAR(n)))))
#define CAAAADR(n) CAR(CAR(CAR(CAR(CDR(n)))))
#define CAAADAR(n) CAR(CAR(CAR(CDR(CAR(n)))))
#define CAAADDR(n) CAR(CAR(CAR(CDR(CDR(n)))))
#define CAADAAR(n) CAR(CAR(CDR(CAR(CAR(n)))))
#define CAADADR(n) CAR(CAR(CDR(CAR(CDR(n)))))
#define CAADDAR(n) CAR(CAR(CDR(CDR(CAR(n)))))
#define CAADDDR(n) CAR(CAR(CDR(CDR(CDR(n)))))
#define CADAAAR(n) CAR(CDR(CAR(CAR(CAR(n)))))
#define CADAADR(n) CAR(CDR(CAR(CAR(CDR(n)))))
#define CADADAR(n) CAR(CDR(CAR(CDR(CAR(n)))))
#define CADADDR(n) CAR(CDR(CAR(CDR(CDR(n)))))
#define CADDAAR(n) CAR(CDR(CDR(CAR(CAR(n)))))
#define CADDADR(n) CAR(CDR(CDR(CAR(CDR(n)))))
#define CADDDAR(n) CAR(CDR(CDR(CDR(CAR(n)))))
#define CADDDDR(n) CAR(CDR(CDR(CDR(CDR(n)))))
#define CDAAAAR(n) CDR(CAR(CAR(CAR(CAR(n)))))
#define CDAAADR(n) CDR(CAR(CAR(CAR(CDR(n)))))
#define CDAADAR(n) CDR(CAR(CAR(CDR(CAR(n)))))
#define CDAADDR(n) CDR(CAR(CAR(CDR(CDR(n)))))
#define CDADAAR(n) CDR(CAR(CDR(CAR(CAR(n)))))
#define CDADADR(n) CDR(CAR(CDR(CAR(CDR(n)))))
#define CDADDAR(n) CDR(CAR(CDR(CDR(CAR(n)))))
#define CDADDDR(n) CDR(CAR(CDR(CDR(CDR(n)))))
#define CDDAAAR(n) CDR(CDR(CAR(CAR(CAR(n)))))
#define CDDAADR(n) CDR(CDR(CAR(CAR(CDR(n)))))
#define CDDADAR(n) CDR(CDR(CAR(CDR(CAR(n)))))
#define CDDADDR(n) CDR(CDR(CAR(CDR(CDR(n)))))
#define CDDDAAR(n) CDR(CDR(CDR(CAR(CAR(n)))))
#define CDDDADR(n) CDR(CDR(CDR(CAR(CDR(n)))))
#define CDDDDAR(n) CDR(CDR(CDR(CDR(CAR(n)))))
#define CDDDDDR(n) CDR(CDR(CDR(CDR(CDR(n)))))
#define CAAAAAAR(n) CAR(CAR(CAR(CAR(CAR(CAR(n))))))
#define CAAAAADR(n) CAR(CAR(CAR(CAR(CAR(CDR(n))))))
#define CAAAADAR(n) CAR(CAR(CAR(CAR(CDR(CAR(n))))))
#define CAAAADDR(n) CAR(CAR(CAR(CAR(CDR(CDR(n))))))
#define CAAADAAR(n) CAR(CAR(CAR(CDR(CAR(CAR(n))))))
#define CAAADADR(n) CAR(CAR(CAR(CDR(CAR(CDR(n))))))
#define CAAADDAR(n) CAR(CAR(CAR(CDR(CDR(CAR(n))))))
#define CAAADDDR(n) CAR(CAR(CAR(CDR(CDR(CDR(n))))))
#define CAADAAAR(n) CAR(CAR(CDR(CAR(CAR(CAR(n))))))
#define CAADAADR(n) CAR(CAR(CDR(CAR(CAR(CDR(n))))))
#define CAADADAR(n) CAR(CAR(CDR(CAR(CDR(CAR(n))))))
#define CAADADDR(n) CAR(CAR(CDR(CAR(CDR(CDR(n))))))
#define CAADDAAR(n) CAR(CAR(CDR(CDR(CAR(CAR(n))))))
#define CAADDADR(n) CAR(CAR(CDR(CDR(CAR(CDR(n))))))
#define CAADDDAR(n) CAR(CAR(CDR(CDR(CDR(CAR(n))))))
#define CAADDDDR(n) CAR(CAR(CDR(CDR(CDR(CDR(n))))))
#define CADAAAAR(n) CAR(CDR(CAR(CAR(CAR(CAR(n))))))
#define CADAAADR(n) CAR(CDR(CAR(CAR(CAR(CDR(n))))))
#define CADAADAR(n) CAR(CDR(CAR(CAR(CDR(CAR(n))))))
#define CADAADDR(n) CAR(CDR(CAR(CAR(CDR(CDR(n))))))
#define CADADAAR(n) CAR(CDR(CAR(CDR(CAR(CAR(n))))))
#define CADADADR(n) CAR(CDR(CAR(CDR(CAR(CDR(n))))))
#define CADADDAR(n) CAR(CDR(CAR(CDR(CDR(CAR(n))))))
#define CADADDDR(n) CAR(CDR(CAR(CDR(CDR(CDR(n))))))
#define CADDAAAR(n) CAR(CDR(CDR(CAR(CAR(CAR(n))))))
#define CADDAADR(n) CAR(CDR(CDR(CAR(CAR(CDR(n))))))
#define CADDADAR(n) CAR(CDR(CDR(CAR(CDR(CAR(n))))))
#define CADDADDR(n) CAR(CDR(CDR(CAR(CDR(CDR(n))))))
#define CADDDAAR(n) CAR(CDR(CDR(CDR(CAR(CAR(n))))))
#define CADDDADR(n) CAR(CDR(CDR(CDR(CAR(CDR(n))))))
#define CADDDDAR(n) CAR(CDR(CDR(CDR(CDR(CAR(n))))))
#define CADDDDDR(n) CAR(CDR(CDR(CDR(CDR(CDR(n))))))
#define CDAAAAAR(n) CDR(CAR(CAR(CAR(CAR(CAR(n))))))
#define CDAAAADR(n) CDR(CAR(CAR(CAR(CAR(CDR(n))))))
#define CDAAADAR(n) CDR(CAR(CAR(CAR(CDR(CAR(n))))))
#define CDAAADDR(n) CDR(CAR(CAR(CAR(CDR(CDR(n))))))
#define CDAADAAR(n) CDR(CAR(CAR(CDR(CAR(CAR(n))))))
#define CDAADADR(n) CDR(CAR(CAR(CDR(CAR(CDR(n))))))
#define CDAADDAR(n) CDR(CAR(CAR(CDR(CDR(CAR(n))))))
#define CDAADDDR(n) CDR(CAR(CAR(CDR(CDR(CDR(n))))))
#define CDADAAAR(n) CDR(CAR(CDR(CAR(CAR(CAR(n))))))
#define CDADAADR(n) CDR(CAR(CDR(CAR(CAR(CDR(n))))))
#define CDADADAR(n) CDR(CAR(CDR(CAR(CDR(CAR(n))))))
#define CDADADDR(n) CDR(CAR(CDR(CAR(CDR(CDR(n))))))
#define CDADDAAR(n) CDR(CAR(CDR(CDR(CAR(CAR(n))))))
#define CDADDADR(n) CDR(CAR(CDR(CDR(CAR(CDR(n))))))
#define CDADDDAR(n) CDR(CAR(CDR(CDR(CDR(CAR(n))))))
#define CDADDDDR(n) CDR(CAR(CDR(CDR(CDR(CDR(n))))))
#define CDDAAAAR(n) CDR(CDR(CAR(CAR(CAR(CAR(n))))))
#define CDDAAADR(n) CDR(CDR(CAR(CAR(CAR(CDR(n))))))
#define CDDAADAR(n) CDR(CDR(CAR(CAR(CDR(CAR(n))))))
#define CDDAADDR(n) CDR(CDR(CAR(CAR(CDR(CDR(n))))))
#define CDDADAAR(n) CDR(CDR(CAR(CDR(CAR(CAR(n))))))
#define CDDADADR(n) CDR(CDR(CAR(CDR(CAR(CDR(n))))))
#define CDDADDAR(n) CDR(CDR(CAR(CDR(CDR(CAR(n))))))
#define CDDADDDR(n) CDR(CDR(CAR(CDR(CDR(CDR(n))))))
#define CDDDAAAR(n) CDR(CDR(CDR(CAR(CAR(CAR(n))))))
#define CDDDAADR(n) CDR(CDR(CDR(CAR(CAR(CDR(n))))))
#define CDDDADAR(n) CDR(CDR(CDR(CAR(CDR(CAR(n))))))
#define CDDDADDR(n) CDR(CDR(CDR(CAR(CDR(CDR(n))))))
#define CDDDDAAR(n) CDR(CDR(CDR(CDR(CAR(CAR(n))))))
#define CDDDDADR(n) CDR(CDR(CDR(CDR(CAR(CDR(n))))))
#define CDDDDDAR(n) CDR(CDR(CDR(CDR(CDR(CAR(n))))))
#define CDDDDDDR(n) CDR(CDR(CDR(CDR(CDR(CDR(n))))))

#define GAUGE_RUSAGE_INDEX 0

#define add_to_mem_block(N,Data,Size) low_my_binary_strcat(Data,Size,areas+N)
#define IDENTIFIERP(i) (Pike_compiler->new_program->identifier_references+(i))
#define INHERIT(i) (Pike_compiler->new_program->inherits+(i))
#define PIKE_PC (Pike_compiler->new_program->num_program)

#ifndef PIKE_DEBUG
#define check_tree(X,Y)
#endif

#endif
