/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: pike_types.h,v 1.29 1999/12/13 12:08:13 mast Exp $
 */
#ifndef PIKE_TYPES_H
#define PIKE_TYPES_H

#include "svalue.h"

struct node_s
{
#ifdef SHARED_NODES
  unsigned INT32 refs;
  unsigned INT32 hash;
  struct node_s *next;
#endif /* SHARED_NODES */
#ifdef PIKE_DEBUG
  struct pike_string *current_file;
#endif
  struct pike_string *type;
  struct pike_string *name;
  struct node_s *parent;
  unsigned INT16 line_number;
  unsigned INT16 node_info;
  unsigned INT16 tree_info;
  /* The stuff from this point on is hashed. */
  unsigned INT16 token;
  union 
  {
    struct
    {
      int number;
      struct program *prog;
    } id;
    struct svalue sval;
    struct
    {
      struct node_s *a,*b;
    } node;
    struct
    {
      int a,b;
    } integer;
  } u;
};

#ifndef STRUCT_NODE_S_DECLARED
#define STRUCT_NODE_S_DECLARED
#endif


typedef struct node_s node;

#define PIKE_TYPE_STACK_SIZE 100000

extern unsigned char type_stack[PIKE_TYPE_STACK_SIZE];
extern unsigned char *type_stackp;
extern unsigned char *pike_type_mark_stack[PIKE_TYPE_STACK_SIZE/4];
extern unsigned char **pike_type_mark_stackp;

extern int max_correct_args;
extern struct pike_string *string_type_string;
extern struct pike_string *int_type_string;
extern struct pike_string *float_type_string;
extern struct pike_string *object_type_string;
extern struct pike_string *function_type_string;
extern struct pike_string *program_type_string;
extern struct pike_string *array_type_string;
extern struct pike_string *list_type_string;
extern struct pike_string *mapping_type_string;
extern struct pike_string *mixed_type_string;
extern struct pike_string *void_type_string;
extern struct pike_string *zero_type_string;
extern struct pike_string *any_type_string;

#define CONSTTYPE(X) make_shared_binary_string(X,CONSTANT_STRLEN(X))

#ifdef PIKE_DEBUG
#define init_type_stack() type_stack_mark()
#define exit_type_stack() do {\
  int q_q_q_q=pop_stack_mark(); \
  if(q_q_q_q) fatal("Type stack out of wack! %d\n",q_q_q_q); \
  } while(0)
#else
#define init_type_stack type_stack_mark
#define exit_type_stack pop_stack_mark
#endif

/* Hmm, these will cause fatals if they fail... */
#define push_type(X) do {				\
  if(type_stackp >= type_stack + sizeof(type_stack))	\
    yyerror("Type stack overflow.");			\
  else {						\
    *type_stackp=(X);					\
    type_stackp++;					\
  }							\
} while(0)

#define unsafe_push_type(X) do {			\
  *type_stackp=(X);					\
  type_stackp++;					\
} while(0)

#define type_stack_mark() do {				\
  if(pike_type_mark_stackp >= pike_type_mark_stack + NELEM(pike_type_mark_stack))	\
    yyerror("Type mark stack overflow.");		\
  else {						\
    *pike_type_mark_stackp=type_stackp;				\
    pike_type_mark_stackp++;					\
  }							\
} while(0)

#define unsafe_type_stack_mark() do {				\
  *pike_type_mark_stackp=type_stackp;				\
  pike_type_mark_stackp++;					\
} while(0)

#define reset_type_stack() do {			\
   type_stack_pop_to_mark();			\
  type_stack_mark();				\
} while(0)

/* Prototypes begin here */
void check_type_string(struct pike_string *s);
void init_types(void);
INT32 pop_stack_mark(void);
void pop_type_stack(void);
void type_stack_pop_to_mark(void);
void type_stack_reverse(void);
void push_type_int(INT32 i);
void push_type_int_backwards(INT32 i);
INT32 extract_type_int(char *p);
void push_unfinished_type(char *s);
void push_finished_type(struct pike_string *type);
void push_finished_type_backwards(struct pike_string *type);
struct pike_string *debug_pop_unfinished_type(void);
struct pike_string *debug_pop_type(void);
struct pike_string *debug_compiler_pop_type(void);
struct pike_string *parse_type(char *s);
void stupid_describe_type(char *a,INT32 len);
void simple_describe_type(struct pike_string *s);
char *low_describe_type(char *t);
struct pike_string *describe_type(struct pike_string *type);
TYPE_T compile_type_to_runtime_type(struct pike_string *s);
struct pike_string *or_pike_types(struct pike_string *a,
				  struct pike_string *b,
				  int zero_implied);
struct pike_string *and_pike_types(struct pike_string *a,
				   struct pike_string *b);
int match_types(struct pike_string *a,struct pike_string *b);
int pike_types_le(struct pike_string *a,struct pike_string *b);
struct pike_string *index_type(struct pike_string *type,
			       struct pike_string *index_type,
			       node *n);
struct pike_string *key_type(struct pike_string *type, node *n);
int check_indexing(struct pike_string *type,
		   struct pike_string *index_type,
		   node *n);
int count_arguments(struct pike_string *s);
int minimum_arguments(struct pike_string *s);
struct pike_string *check_call(struct pike_string *args,
			       struct pike_string *type);
INT32 get_max_args(struct pike_string *type);
struct pike_string *zzap_function_return(char *a, INT32 id);
struct pike_string *get_type_of_svalue(struct svalue *s);
char *get_name_of_type(int t);
void cleanup_pike_types(void);
int type_may_overload(char *type, int lfun);
/* Prototypes end here */

/* "Dynamic types" - use with ADD_FUNCTION_DTYPE */
#define dtStore(TYPE) {int e; for (e=0; e<CONSTANT_STRLEN(TYPE); e++) unsafe_push_type((TYPE)[e]);}
#define dtArr(VAL) {unsafe_push_type(PIKE_T_ARRAY); {VAL}}
#define dtArray dtArr(dtMix)
#define dtMap(IND,VAL) {unsafe_push_type(PIKE_T_MAPPING); {VAL} {IND}}
#define dtMapping dtMap(dtMix,dtMix)
#define dtSet(IND) {unsafe_push_type(PIKE_T_MULTISET); {IND}}
#define dtMultiset dtSet(dtMix)
#define dtObjImpl(PROGRAM) {unsafe_push_type(PIKE_T_OBJECT); unsafe_push_type(0); push_type_int_backwards((PROGRAM)->id);}
#define dtObjInher(PROGRAM) {unsafe_push_type(PIKE_T_OBJECT); unsafe_push_type(1); push_type_int_backwards((PROGRAM)->id);}
#define dtObj dtStore(tObj)
#define dtFuncV(ARGS,REST,RET) MagicdtFuncV(RET,REST,ARGS)
#define dtFunc(ARGS,RET) MagicdtFunc(RET,ARGS)
#define MagicdtFuncV(RET,REST,ARGS) {unsafe_push_type(PIKE_T_FUNCTION); {ARGS} unsafe_push_type(T_MANY); {REST} {RET}}
#define MagicdtFunc(RET,ARGS) dtFuncV(ARGS {}, dtVoid, RET)
#define dtFunction dtFuncV({},dtAny,dtAny)
#define dtNone {}
#define dtPrg {unsafe_push_type(PIKE_T_PROGRAM);}
#define dtProgram {unsafe_push_type(PIKE_T_PROGRAM);}
#define dtStr {unsafe_push_type(PIKE_T_STRING);}
#define dtString {unsafe_push_type(PIKE_T_STRING);}
#define dtType {unsafe_push_type(PIKE_T_TYPE);}
#define dtFlt {unsafe_push_type(PIKE_T_FLOAT);}
#define dtFloat {unsafe_push_type(PIKE_T_FLOAT);}
#define dtIntRange(LOW,HIGH) {unsafe_push_type(PIKE_T_INT); push_type_int_backwards(LOW); push_type_int_backwards(HIGH);}
#define dtInt dtStore(tInt)
#define dtZero {unsafe_push_type(PIKE_T_ZERO);}
#define dtVoid {unsafe_push_type(T_VOID);}
#define dtVar(X) {unsafe_push_type(X);}
#define dtSetvar(X,TYPE) {unsafe_push_type(T_ASSIGN); {TYPE}}
#define dtNot(TYPE) {unsafe_push_type(T_NOT); {TYPE}}
#define dtAnd(A,B) {unsafe_push_type(T_AND); {A} {B}}
#define dtOr(A,B) {unsafe_push_type(T_OR); {A} {B}}
#define dtOr3(A,B,C) dtOr(A,dtOr(B,C))
#define dtOr4(A,B,C,D) dtOr(A,dtOr3(B,C,D))
#define dtOr5(A,B,C,D,E) dtOr(A,dtOr4(B,C,D,E))
#define dtOr6(A,B,C,D,E,F) dtOr(A,dtOr5(B,C,D,E,F))
#define dtMix {unsafe_push_type(T_MIXED);}
#define dtMixed {unsafe_push_type(T_MIXED);}
#define dtComplex dtStore(tComplex)
#define dtStringIndicable dtStore(tStringIndicable)
#define dtRef dtStore(tRef)
#define dtIfnot(A,B) dtAnd(dtNot(A),B)
#define dtAny dtStore(tAny)
#define DTYPE_START do {						\
  unsafe_type_stack_mark();						\
  unsafe_type_stack_mark();						\
} while (0)
#define DTYPE_END(TYPESTR) do {						\
  if(type_stackp >= type_stack + sizeof(type_stack))			\
    fatal("Type stack overflow.");					\
  type_stack_reverse();							\
  (TYPESTR)=pop_unfinished_type();					\
} while (0)

#ifdef DEBUG_MALLOC
#define pop_type() ((struct pike_string *)debug_malloc_update_location(debug_pop_type(),__FILE__,__LINE__))
#define compiler_pop_type() ((struct pike_string *)debug_malloc_update_location(debug_compiler_pop_type(),__FILE__,__LINE__))
#define pop_unfinished_type() \
 ((struct pike_string *)debug_malloc_update_location(debug_pop_unfinished_type(),__FILE__,__LINE__))
#else
#define pop_type debug_pop_type
#define compiler_pop_type debug_compiler_pop_type
#define pop_unfinished_type debug_pop_unfinished_type
#endif

#ifndef PIKE_DEBUG
#define check_type_string(X)
#endif

#endif
