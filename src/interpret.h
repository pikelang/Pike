/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: interpret.h,v 1.40 2000/04/08 02:01:08 hubbe Exp $
 */
#ifndef INTERPRET_H
#define INTERPRET_H

#include "global.h"
#include "program.h"

#ifndef STRUCT_FRAME_DECLARED
#define STRUCT_FRAME_DECLARED
#endif
struct pike_frame
{
  INT32 refs; /* must be first */
  INT32 args;
  unsigned INT16 fun;
  INT16 num_locals;
  INT16 num_args;
  INT16 malloced_locals;
  struct pike_frame *next;
  struct pike_frame *scope;
  unsigned char *pc;
  struct svalue *locals;
  struct svalue *expendible;
  struct object *current_object;
  struct inherit context;
  char *current_storage;
};

#ifdef PIKE_DEBUG
#define debug_check_stack() do{if(Pike_sp<Pike_evaluator_stack)fatal("Stack error.\n");}while(0)
#define check__positive(X,Y) if((X)<0) fatal Y
#include "error.h"
#else
#define check__positive(X,Y)
#define debug_check_stack() 
#endif

#define check_stack(X) do {			\
  if(Pike_sp - Pike_evaluator_stack + (X) >= Pike_stack_size)	\
    error("Svalue stack overflow. " \
	  "(%d of %d entries on stack, needed %d more entries)\n", \
	  Pike_sp-Pike_evaluator_stack,Pike_stack_size,(X)); \
  }while(0)

#define check_mark_stack(X) do {		\
  if(Pike_mark_sp - Pike_mark_stack + (X) >= Pike_stack_size)	\
    error("Mark stack overflow.\n");		\
  }while(0)

#define check_c_stack(X) do { 			\
  long x_= ((char *)&x_) + STACK_DIRECTION * (X) - Pike_stack_top ;	\
  x_*=STACK_DIRECTION;							\
  if(x_>0)								\
    low_error("C stack overflow.\n");					\
  }while(0)


#define pop_stack() do{ free_svalue(--Pike_sp); debug_check_stack(); }while(0)

#define pop_n_elems(X)						\
 do { int x_=(X); if(x_) { 					\
   check__positive(x_,("Popping negative number of args.... (%d) \n",x_)); \
   Pike_sp-=x_; debug_check_stack();					\
  free_svalues(Pike_sp,x_,BIT_MIXED);				\
 } } while (0)

#define stack_pop_n_elems_keep_top(X) \
 do { struct svalue s=Pike_sp[-1]; Pike_sp[-1]=Pike_sp[-1-(X)]; Pike_sp[-1-(X)]=s; \
      pop_n_elems(X); } while (0)

#define push_program(P) do{ struct program *_=(P); debug_malloc_touch(_); Pike_sp->u.program=_; Pike_sp++->type=PIKE_T_PROGRAM; }while(0)
#define push_int(I) do{ INT32 _=(I); Pike_sp->u.integer=_;Pike_sp->type=PIKE_T_INT;Pike_sp++->subtype=NUMBER_NUMBER; }while(0)
#define push_mapping(M) do{ struct mapping *_=(M); debug_malloc_touch(_); Pike_sp->u.mapping=_; Pike_sp++->type=PIKE_T_MAPPING; }while(0)
#define push_array(A) do{ struct array *_=(A); debug_malloc_touch(_); Pike_sp->u.array=_ ;Pike_sp++->type=PIKE_T_ARRAY; }while(0)
#define push_multiset(L) do{ struct multiset *_=(L); debug_malloc_touch(_); Pike_sp->u.multiset=_; Pike_sp++->type=PIKE_T_MULTISET; }while(0)
#define push_string(S) do{ struct pike_string *_=(S); debug_malloc_touch(_); Pike_sp->subtype=0; Pike_sp->u.string=_; Pike_sp++->type=PIKE_T_STRING; }while(0)
#define push_object(O) do{ struct object  *_=(O); debug_malloc_touch(_); Pike_sp->u.object=_; Pike_sp++->type=PIKE_T_OBJECT; }while(0)
#define push_float(F) do{ float _=(F); Pike_sp->u.float_number=_; Pike_sp++->type=PIKE_T_FLOAT; }while(0)
#define push_text(T) push_string(make_shared_string((T)))
#define push_constant_text(T) do{ Pike_sp->subtype=0; MAKE_CONSTANT_SHARED_STRING(Pike_sp->u.string,T); Pike_sp++->type=PIKE_T_STRING; }while(0)

#define ref_push_program(P) do{ struct program *_=(P); debug_malloc_touch(_); _->refs++; Pike_sp->u.program=_; Pike_sp++->type=PIKE_T_PROGRAM; }while(0)
#define ref_push_mapping(M) do{ struct mapping *_=(M); debug_malloc_touch(_); _->refs++; Pike_sp->u.mapping=_; Pike_sp++->type=PIKE_T_MAPPING; }while(0)
#define ref_push_array(A) do{ struct array *_=(A); debug_malloc_touch(_); _->refs++; Pike_sp->u.array=_ ;Pike_sp++->type=PIKE_T_ARRAY; }while(0)
#define ref_push_multiset(L) do{ struct multiset *_=(L); debug_malloc_touch(_); _->refs++; Pike_sp->u.multiset=_; Pike_sp++->type=PIKE_T_MULTISET; }while(0)
#define ref_push_string(S) do{ struct pike_string *_=(S); debug_malloc_touch(_); _->refs++; Pike_sp->subtype=0; Pike_sp->u.string=_; Pike_sp++->type=PIKE_T_STRING; }while(0)
#define ref_push_object(O) do{ struct object  *_=(O); debug_malloc_touch(_); _->refs++; Pike_sp->u.object=_; Pike_sp++->type=PIKE_T_OBJECT; }while(0)

#define push_svalue(S) do { struct svalue *_=(S); assign_svalue_no_free(Pike_sp,_); Pike_sp++; }while(0)

#define stack_dup() push_svalue(Pike_sp-1)
#define stack_swap() do { struct svalue _=Pike_sp[-1]; Pike_sp[-1]=Pike_sp[-2]; Pike_sp[-2]=_; } while(0)

/* This pops a number of arguments from the stack but keeps the top
 * element on top. Used for popping the arguments while keeping the
 * return value.
 */
#define stack_unlink(X) do { if(X) { free_svalue(Pike_sp-(X)-1); Pike_sp[-(X)-1]=Pike_sp[-1]; Pike_sp--; pop_n_elems(X-1); } }while(0)

#define free_pike_frame(F) do{ struct pike_frame *f_=(F); debug_malloc_touch(f_); if(!--f_->refs) really_free_pike_frame(f_); }while(0)

#define POP_PIKE_FRAME() do {						\
  struct pike_frame *tmp_=Pike_fp->next;					\
  if(!--Pike_fp->refs)							\
  {									\
    really_free_pike_frame(Pike_fp);						\
  }else{								\
    DO_IF_DEBUG(if( Pike_fp->locals+Pike_fp->num_locals>Pike_sp) fatal("Stack failure in POP_PIKE_FRAME!\n"));                                                      \
    debug_malloc_touch(Pike_fp); \
    if(Pike_fp->num_locals)							\
    {									\
      struct svalue *s=(struct svalue *)xalloc(sizeof(struct svalue)*	\
					       Pike_fp->num_locals);		\
      assign_svalues_no_free(s,Pike_fp->locals,Pike_fp->num_locals,BIT_MIXED);	\
      Pike_fp->locals=s;							\
      Pike_fp->malloced_locals=1;						\
    }else{								\
      Pike_fp->locals=0;							\
    }									\
    Pike_fp->next=0;								\
  }									\
  Pike_fp=tmp_;								\
 }while(0)


enum apply_type
{
  APPLY_STACK, /* The function is the first argument */
  APPLY_SVALUE, /* arg1 points to an svalue containing the function */
   APPLY_LOW    /* arg1 is the object pointer,(int)arg2 the function */
};

#define apply_low(O,FUN,ARGS) \
  mega_apply(APPLY_LOW, (ARGS), (void*)(O),(void*)(FUN))

#define strict_apply_svalue(SVAL,ARGS) \
  mega_apply(APPLY_SVALUE, (ARGS), (void*)(SVAL),0)

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

#ifdef PIKE_DEBUG
#define fast_check_threads_etc(X) do { \
  static int div_; if(d_flag || !(div_++& ((1<<(X))-1))) check_threads_etc(); } while(0)

#else
#define fast_check_threads_etc(X) do { \
  static int div_; if(!(div_++& ((1<<(X))-1))) check_threads_etc(); } while(0)
#endif

#include "block_alloc_h.h"
/* Prototypes begin here */
void push_sp_mark(void);
int pop_sp_mark(void);
void init_interpreter(void);
void lvalue_to_svalue_no_free(struct svalue *to,struct svalue *lval);
void assign_lvalue(struct svalue *lval,struct svalue *from);
union anything *get_pointer_if_this_type(struct svalue *lval, TYPE_T t);
void print_return_value(void);
void reset_evaluator(void);
struct backlog;
void dump_backlog(void);
BLOCK_ALLOC(pike_frame,128)

void mega_apply(enum apply_type type, INT32 args, void *arg1, void *arg2);
void f_call_function(INT32 args);
int apply_low_safe_and_stupid(struct object *o, INT32 offset);
void safe_apply_low(struct object *o,int fun,int args);
void safe_apply(struct object *o, char *fun ,INT32 args);
void apply_lfun(struct object *o, int fun, int args);
void apply_shared(struct object *o,
		  struct pike_string *fun,
		  int args);
void apply(struct object *o, char *fun, int args);
void apply_svalue(struct svalue *s, INT32 args);
void slow_check_stack(void);
void cleanup_interpret(void);
void really_clean_up_interpret(void);
/* Prototypes end here */

extern struct svalue *Pike_sp;
extern struct svalue **Pike_mark_sp;
extern struct svalue *Pike_evaluator_stack;
extern struct svalue **Pike_mark_stack;
extern struct pike_frame *Pike_fp; /* pike_frame pointer */
extern char *Pike_stack_top;
extern int Pike_stack_size;
extern int evaluator_stack_malloced, mark_stack_malloced;
struct callback;
extern struct callback_list evaluator_callbacks;
extern void call_callback(struct callback_list *, void *);

#ifdef PROFILING
#ifdef HAVE_GETHRTIME
extern long long accounted_time;
extern long long time_base;
#endif
#endif

#ifndef NO_PIKE_SHORTHAND

#define sp Pike_sp
#define fp Pike_fp
#define evaluator_stack Pike_evaluator_stack
#define stack_top Pike_stack_top
#define mark_sp Pike_mark_sp
#define mark_stack Pike_mark_stack
#define stack_size Pike_stack_size

#endif /* !NO_PIKE_SHORTHAND */

#define CURRENT_STORAGE (dmalloc_touch(struct pike_frame *,Pike_fp)->current_storage)

#endif


