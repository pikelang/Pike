/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: pike_types.h,v 1.15 1998/11/06 03:08:02 hubbe Exp $
 */
#ifndef PIKE_TYPES_H
#define PIKE_TYPES_H

#include "svalue.h"

struct node_s
{
  unsigned INT16 token;
  INT16 line_number;
  INT16 node_info;
  INT16 tree_info;
#ifdef DEBUG
  struct pike_string *current_file;
#endif
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
extern struct pike_string *any_type_string;

#ifdef DEBUG
#define init_type_stack() type_stack_mark()
#define exit_type_stack() do {\
  int q_q_q_q=pop_stack_mark(); \
  if(q_q_q_q) fatal("Type stack out of wack! %d\n",q_q_q_q); \
  } while(0)
#else
#define init_type_stack type_stack_mark
#define exit_type_stack pop_stack_mark
#endif

#define push_type(X) do {				\
  *type_stackp=(X);					\
  type_stackp++;					\
  if(type_stackp >= type_stack + sizeof(type_stack))	\
    yyerror("Type stack overflow.");			\
} while(0)


#define type_stack_mark() do {				\
  *pike_type_mark_stackp=type_stackp;				\
  pike_type_mark_stackp++;					\
  if(pike_type_mark_stackp >= pike_type_mark_stack + NELEM(pike_type_mark_stack))	\
    yyerror("Type mark stack overflow.");		\
} while(0)

#define reset_type_stack() do {			\
   type_stack_pop_to_mark();			\
  type_stack_mark();				\
} while(0)

/* Prototypes begin here */
void init_types(void);
INT32 pop_stack_mark(void);
void pop_type_stack(void);
void type_stack_pop_to_mark(void);
void type_stack_reverse(void);
void push_type_int(unsigned INT32 i);
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
int match_types(struct pike_string *a,struct pike_string *b);
struct pike_string *index_type(struct pike_string *type, node *n);
int check_indexing(struct pike_string *type,
		   struct pike_string *index_type,
		   node *n);
int count_arguments(struct pike_string *s);
struct pike_string *check_call(struct pike_string *args,
			       struct pike_string *type);
INT32 get_max_args(struct pike_string *type);
struct pike_string *get_type_of_svalue(struct svalue *s);
char *get_name_of_type(int t);
void cleanup_pike_types(void);
int type_may_overload(char *type, int lfun);
/* Prototypes end here */

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

#endif
