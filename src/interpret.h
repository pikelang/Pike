/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef INTERPRET_H
#define INTERPRET_H

#include "global.h"
#include "program.h"

#ifndef STRUCT_FRAME_DECLARED
#define STRUCT_FRAME_DECLARED
#endif
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
  INT16 num_locals;
  INT16 num_args;
};

#ifdef DEBUG
#define debug_check_stack() do{if(sp<evaluator_stack)fatal("Stack error.\n");}while(0)
#else
#define debug_check_stack() 
#endif

#define pop_stack() do{ free_svalue(--sp); debug_check_stack(); }while(0)

#define push_program(P) do{ struct program *_=(P); sp->u.program=_; sp++->type=T_PROGRAM; }while(0)
#define push_int(I) do{ INT32 _=(I); sp->u.integer=_;sp->type=T_INT;sp++->subtype=NUMBER_NUMBER; }while(0)
#define push_mapping(M) do{ struct mapping *_=(M); sp->u.mapping=_; sp++->type=T_MAPPING; }while(0)
#define push_array(A) do{ struct array *_=(A); sp->u.array=_ ;sp++->type=T_ARRAY; }while(0)
#define push_multiset(L) do{ struct multiset *_=(L); sp->u.multiset=_; sp++->type=T_MULTISET; }while(0)
#define push_string(S) do{ struct pike_string *_=(S); sp->subtype=0; sp->u.string=_; sp++->type=T_STRING; }while(0)
#define push_object(O) do{ struct object  *_=(O); sp->u.object=_; sp++->type=T_OBJECT; }while(0)
#define push_float(F) do{ float _=(F); sp->u.float_number=_; sp++->type=T_FLOAT; }while(0)
#define push_text(T) push_string(make_shared_string((T)))

#define ref_push_program(P) do{ struct program *_=(P); _->refs++; sp->u.program=_; sp++->type=T_PROGRAM; }while(0)
#define ref_push_mapping(M) do{ struct mapping *_=(M); _->refs++; sp->u.mapping=_; sp++->type=T_MAPPING; }while(0)
#define ref_push_array(A) do{ struct array *_=(A); _->refs++; sp->u.array=_ ;sp++->type=T_ARRAY; }while(0)
#define ref_push_multiset(L) do{ struct multiset *_=(L); _->refs++; sp->u.multiset=_; sp++->type=T_MULTISET; }while(0)
#define ref_push_string(S) do{ struct pike_string *_=(S); _->refs++; sp->subtype=0; sp->u.string=_; sp++->type=T_STRING; }while(0)
#define ref_push_object(O) do{ struct object  *_=(O); _->refs++; sp->u.object=_; sp++->type=T_OBJECT; }while(0)

#define push_svalue(S) do { struct svalue *_=(S); assign_svalue_no_free(sp,_); sp++; }while(0)

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

#define check_threads_etc() \
  call_callback(& evaluator_callbacks, (void *)0)

#define fast_check_threads_etc(X) do { \
  static int div_; if(!(div_++& ((1<<(X))-1))) check_threads_etc(); } while(0)

/* Prototypes begin here */
void push_sp_mark(void);
int pop_sp_mark(void);
void init_interpreter(void);
void check_stack(INT32 size);
void check_mark_stack(INT32 size);
void lvalue_to_svalue_no_free(struct svalue *to,struct svalue *lval);
void assign_lvalue(struct svalue *lval,struct svalue *from);
union anything *get_pointer_if_this_type(struct svalue *lval, TYPE_T t);
void print_return_value(void);
void pop_n_elems(INT32 x);
void reset_evaluator(void);
struct backlog;
void dump_backlog(void);
int apply_low_safe_and_stupid(struct object *o, INT32 offset);
void apply_low(struct object *o, int fun, int args);
void safe_apply_low(struct object *o,int fun,int args);
void safe_apply(struct object *o, char *fun ,INT32 args);
void apply_lfun(struct object *o, int fun, int args);
void apply_shared(struct object *o,
		  struct pike_string *fun,
		  int args);
void apply(struct object *o, char *fun, int args);
void strict_apply_svalue(struct svalue *s, INT32 args);
void apply_svalue(struct svalue *s, INT32 args);
void slow_check_stack(void);
void cleanup_interpret(void);
/* Prototypes end here */

extern struct svalue *sp;
extern struct svalue **mark_sp;
extern struct svalue *evaluator_stack;
extern struct svalue **mark_stack;
extern struct frame *fp; /* frame pointer */
extern int stack_size;
extern int evaluator_stack_malloced, mark_stack_malloced;
struct callback;
extern struct callback_list evaluator_callbacks;
extern void call_callback(struct callback_list *, void *);
#endif

