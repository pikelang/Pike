/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef INTERPRET_H
#define INTERPRET_H

#include "program.h"
#include "config.h"

struct frame
{
  unsigned char *pc;
  struct frame *parent_frame;
  struct svalue *locals;
  INT32 args;
  struct object *current_object;
  struct inherit context;
  char *current_storage;
  INT32 fun;
#ifdef DEBUG
  INT16 num_locals;
  INT16 num_args;
#endif
};

#ifdef DEBUG
#define check_stack() do{if(sp<evaluator_stack)fatal("Stack error.\n");}while(0)
#else
#define check_stack() 
#endif
#define pop_stack() do{ free_svalue(--sp); check_stack(); }while(0)
#define push_program(P) sp->u.program=(P),sp++->type=T_PROGRAM
#define push_int(I) sp->u.integer=(I),sp->type=T_INT,sp++->subtype=NUMBER_NUMBER
#define push_mapping(M) sp->u.mapping=(M),sp++->type=T_MAPPING
#define push_array(A) sp->u.array=(A),sp++->type=T_ARRAY
#define push_list(L) sp->u.list=(L),sp++->type=T_LIST
#define push_string(S) sp->u.string=(S),sp++->type=T_STRING
#define push_object(O) sp->u.object=(O),sp++->type=T_OBJECT
#define push_float(F) sp->u.float_number=(F),sp++->type=T_FLOAT
#define push_text(T) sp->u.string=make_shared_string(T),sp++->type=T_STRING

#define APPLY_MASTER(FUN,ARGS) \
do{ \
  static int fun_,master_cnt=0; \
  struct object *master_ob=master(); \
  if(master_cnt != master_ob->prog->id) \
  { \
    fun_=find_identifier(FUN,master_ob->prog); \
    master_cnt = master_ob->prog->id; \
  } \
  apply_low(master_ob, fun_, ARGS); \
}while(0)

#define SAFE_APPLY_MASTER(FUN,ARGS) \
do{ \
  static int fun_,master_cnt=0; \
  struct object *master_ob=master(); \
  if(master_cnt != master_ob->prog->id) \
  { \
    fun_=find_identifier(FUN,master_ob->prog); \
    master_cnt = master_ob->prog->id; \
  } \
  safe_apply_low(master_ob, fun_, ARGS); \
}while(0)

/* Prototypes begin here */
void lvalue_to_svalue_no_free(struct svalue *to,struct svalue *lval);
void assign_lvalue(struct svalue *lval,struct svalue *from);
union anything *get_pointer_if_this_type(struct svalue *lval, TYPE_T t);
void pop_n_elems(INT32 x);
void reset_evaluator();
void f_catch(unsigned char *pc);
struct backlog;
void dump_backlog(void);
int apply_low_safe_and_stupid(struct object *o, INT32 offset);
void apply_low(struct object *o, int fun, int args);
void safe_apply_low(struct object *o,int fun,int args);
void safe_apply(struct object *o, char *fun ,INT32 args);
void apply_shared(struct object *o,
		  struct lpc_string *fun,
		  int args);
void apply(struct object *o, char *fun, int args);
void strict_apply_svalue(struct svalue *s, INT32 args);
void apply_svalue(struct svalue *s, INT32 args);
void slow_check_stack();
void cleanup_interpret();
/* Prototypes end here */

extern struct svalue *sp;
extern struct svalue **mark_sp;
extern struct svalue evaluator_stack[EVALUATOR_STACK_SIZE];
extern struct frame *fp; /* frame pointer */
#endif

