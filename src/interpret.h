/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: interpret.h,v 1.34 1999/10/21 21:34:32 hubbe Exp $
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
  INT16 fun;
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
#define debug_check_stack() do{if(sp<evaluator_stack)fatal("Stack error.\n");}while(0)
#define check__positive(X,Y) if((X)<0) fatal(Y)
#include "error.h"
#else
#define check__positive(X,Y)
#define debug_check_stack() 
#endif

#define check_stack(X) do {			\
  if(sp - evaluator_stack + (X) >= stack_size)	\
    error("Svalue stack overflow. " \
	  "(%d of %d entries on stack, needed %d more entries)\n", \
	  sp-evaluator_stack,stack_size,(X)); \
  }while(0)

#define check_mark_stack(X) do {		\
  if(mark_sp - mark_stack + (X) >= stack_size)	\
    error("Mark stack overflow.\n");		\
  }while(0)

#define check_c_stack(X) do { 			\
  long x_= ((char *)&x_) + STACK_DIRECTION * (X) - stack_top ;	\
  x_*=STACK_DIRECTION;							\
  if(x_>0)								\
    low_error("C stack overflow.\n");					\
  }while(0)


#define pop_stack() do{ free_svalue(--sp); debug_check_stack(); }while(0)

#define pop_n_elems(X)						\
 do { int x_=(X); if(x_) { 					\
   check__positive(x_,"Popping negative number of args....\n");	\
   sp-=x_; debug_check_stack();					\
  free_svalues(sp,x_,BIT_MIXED);				\
 } } while (0)

#define stack_pop_n_elems_keep_top(X) \
 do { struct svalue s=sp[-1]; sp[-1]=sp[-1-(X)]; sp[-1-(X)]=s; \
      pop_n_elems(X); } while (0)

#define push_program(P) do{ struct program *_=(P); debug_malloc_touch(_); sp->u.program=_; sp++->type=T_PROGRAM; }while(0)
#define push_int(I) do{ INT32 _=(I); sp->u.integer=_;sp->type=T_INT;sp++->subtype=NUMBER_NUMBER; }while(0)
#define push_mapping(M) do{ struct mapping *_=(M); debug_malloc_touch(_); sp->u.mapping=_; sp++->type=T_MAPPING; }while(0)
#define push_array(A) do{ struct array *_=(A); debug_malloc_touch(_); sp->u.array=_ ;sp++->type=T_ARRAY; }while(0)
#define push_multiset(L) do{ struct multiset *_=(L); debug_malloc_touch(_); sp->u.multiset=_; sp++->type=T_MULTISET; }while(0)
#define push_string(S) do{ struct pike_string *_=(S); debug_malloc_touch(_); sp->subtype=0; sp->u.string=_; sp++->type=T_STRING; }while(0)
#define push_object(O) do{ struct object  *_=(O); debug_malloc_touch(_); sp->u.object=_; sp++->type=T_OBJECT; }while(0)
#define push_float(F) do{ float _=(F); sp->u.float_number=_; sp++->type=T_FLOAT; }while(0)
#define push_text(T) push_string(make_shared_string((T)))
#define push_constant_text(T) do{ sp->subtype=0; MAKE_CONSTANT_SHARED_STRING(sp->u.string,T); sp++->type=T_STRING; }while(0)

#define ref_push_program(P) do{ struct program *_=(P); debug_malloc_touch(_); _->refs++; sp->u.program=_; sp++->type=T_PROGRAM; }while(0)
#define ref_push_mapping(M) do{ struct mapping *_=(M); debug_malloc_touch(_); _->refs++; sp->u.mapping=_; sp++->type=T_MAPPING; }while(0)
#define ref_push_array(A) do{ struct array *_=(A); debug_malloc_touch(_); _->refs++; sp->u.array=_ ;sp++->type=T_ARRAY; }while(0)
#define ref_push_multiset(L) do{ struct multiset *_=(L); debug_malloc_touch(_); _->refs++; sp->u.multiset=_; sp++->type=T_MULTISET; }while(0)
#define ref_push_string(S) do{ struct pike_string *_=(S); debug_malloc_touch(_); _->refs++; sp->subtype=0; sp->u.string=_; sp++->type=T_STRING; }while(0)
#define ref_push_object(O) do{ struct object  *_=(O); debug_malloc_touch(_); _->refs++; sp->u.object=_; sp++->type=T_OBJECT; }while(0)

#define push_svalue(S) do { struct svalue *_=(S); assign_svalue_no_free(sp,_); sp++; }while(0)

#define stack_dup() push_svalue(sp-1)
#define stack_swap() do { struct svalue _=sp[-1]; sp[-1]=sp[-2]; sp[-2]=_; } while(0)

/* This pops a number of arguments from the stack but keeps the top
 * element on top. Used for popping the arguments while keeping the
 * return value.
 */
#define stack_unlink(X) do { if(X) { free_svalue(sp-(X)-1); sp[-(X)-1]=sp[-1]; sp--; pop_n_elems(X-1); } }while(0)

#define free_pike_frame(F) do{ struct pike_frame *f_=(F); debug_malloc_touch(f_); if(!--f_->refs) really_free_pike_frame(f_); }while(0)

#define POP_PIKE_FRAME() do {						\
  struct pike_frame *tmp_=fp->next;					\
  if(!--fp->refs)							\
  {									\
    really_free_pike_frame(fp);						\
  }else{								\
    DO_IF_DEBUG(if( fp->locals+fp->num_locals>sp) fatal("Stack failure in POP_PIKE_FRAME!\n"));                                                      \
    debug_malloc_touch(fp); \
    if(fp->num_locals)							\
    {									\
      struct svalue *s=(struct svalue *)xalloc(sizeof(struct svalue)*	\
					       fp->num_locals);		\
      assign_svalues_no_free(s,fp->locals,fp->num_locals,BIT_MIXED);	\
      fp->locals=s;							\
      fp->malloced_locals=1;						\
    }else{								\
      fp->locals=0;							\
    }									\
    fp->next=0;								\
  }									\
  fp=tmp_;								\
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

static void restore_creds(struct object *creds);
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

extern struct svalue *sp;
extern struct svalue **mark_sp;
extern struct svalue *evaluator_stack;
extern struct svalue **mark_stack;
extern struct pike_frame *fp; /* pike_frame pointer */
extern char *stack_top;
extern int stack_size;
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

#endif

