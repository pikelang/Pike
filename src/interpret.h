/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: interpret.h,v 1.82 2004/05/01 16:46:02 marcus Exp $
 */
#ifndef INTERPRET_H
#define INTERPRET_H

#include "global.h"
#include "program.h"
#include "pike_error.h"

struct Pike_interpreter {
  /* Swapped variables */
  struct svalue *stack_pointer;
  struct svalue *evaluator_stack;
  struct svalue **mark_stack_pointer;
  struct svalue **mark_stack;
  struct pike_frame *frame_pointer;
  int evaluator_stack_malloced;
  int mark_stack_malloced;
  JMP_BUF *recoveries;
#ifdef PIKE_THREADS
  struct object * thread_id;
#endif
  char *stack_top;
  DO_IF_SECURITY(struct object *current_creds;)

  int svalue_stack_margin;
  int c_stack_margin;

#ifdef PROFILING
#ifdef HAVE_GETHRTIME
  long long accounted_time;
  long long time_base;
#endif
  char *stack_bottom;
#endif

#ifdef THREAD_TRACE
  int t_flag;
#endif /* THREAD_TRACE */
};

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

struct external_variable_context
{
  struct object *o;
  struct inherit *inherit;
  int parent_identifier;
};

#ifdef PIKE_DEBUG
#define debug_check_stack() do{if(Pike_sp<Pike_interpreter.evaluator_stack)fatal("Stack error.\n");}while(0)
#define check__positive(X,Y) if((X)<0) fatal Y
#include "pike_error.h"
#else
#define check__positive(X,Y)
#define debug_check_stack() 
#endif

#define low_stack_check(X) \
  (Pike_sp - Pike_interpreter.evaluator_stack + \
   Pike_interpreter.svalue_stack_margin + (X) >= Pike_stack_size)

extern const char *Pike_check_stack_errmsg;

#define check_stack(X) do { \
  if(low_stack_check(X)) \
    ((void (*)(const char *, ...))Pike_error)( \
               Pike_check_stack_errmsg, \
	       PTRDIFF_T_TO_LONG(Pike_sp - Pike_interpreter.evaluator_stack), \
	       PTRDIFF_T_TO_LONG(Pike_stack_size), \
	       PTRDIFF_T_TO_LONG(X)); \
  }while(0)

extern const char *Pike_check_mark_stack_errmsg;

#define check_mark_stack(X) do {		\
  if(Pike_mark_sp - Pike_interpreter.mark_stack + (X) >= Pike_stack_size) \
    ((void (*)(const char*, ...))Pike_error)(Pike_check_mark_stack_errmsg); \
  }while(0)

extern const char *Pike_check_c_stack_errmsg;

#define check_c_stack(X) do {						\
  ptrdiff_t x_= ((char *)&x_) +						\
    STACK_DIRECTION * (Pike_interpreter.c_stack_margin + (X)) -		\
    Pike_interpreter.stack_top ;					\
  x_*=STACK_DIRECTION;							\
  if(x_>0)								\
    low_error(Pike_check_c_stack_errmsg);				\
  }while(0)

#define fatal_check_c_stack(X) do { 			\
    ptrdiff_t x_= 					\
      ((char *)&x_) + STACK_DIRECTION * (X) - Pike_interpreter.stack_top ; \
    x_*=STACK_DIRECTION;						\
    if(x_>0) {								\
      ((void (*)(const char*, ...))fatal)(Pike_check_c_stack_errmsg);	\
    }									\
  }while(0)


#define pop_stack() do{ free_svalue(--Pike_sp); debug_check_stack(); }while(0)

#ifdef __ECL
#define MAYBE_CAST_TO_LONG(X)	(X)
#else /* !__ECL */
#define MAYBE_CAST_TO_LONG(X)	((long)(X))
#endif /* __ECL */

#define pop_n_elems(X)							\
 do { ptrdiff_t x_=(X); if(x_) { 					\
   check__positive(x_, ("Popping negative number of args.... (%ld) \n",	\
		   MAYBE_CAST_TO_LONG(x_)));				\
   Pike_sp -= x_; debug_check_stack();					\
   free_svalues(Pike_sp, x_, BIT_MIXED);				\
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
#define push_float(F) do{ FLOAT_TYPE _=(F); Pike_sp->u.float_number=_; Pike_sp++->type=PIKE_T_FLOAT; }while(0)
#define push_text(T) push_string(make_shared_string((T)))
#define push_constant_text(T) do{ Pike_sp->subtype=0; MAKE_CONSTANT_SHARED_STRING(Pike_sp->u.string,T); Pike_sp++->type=PIKE_T_STRING; }while(0)

#define ref_push_program(P) do{ struct program *_=(P); debug_malloc_touch(_); _->refs++; Pike_sp->u.program=_; Pike_sp++->type=PIKE_T_PROGRAM; }while(0)
#define ref_push_mapping(M) do{ struct mapping *_=(M); debug_malloc_touch(_); _->refs++; Pike_sp->u.mapping=_; Pike_sp++->type=PIKE_T_MAPPING; }while(0)
#define ref_push_array(A) do{ struct array *_=(A); debug_malloc_touch(_); _->refs++; Pike_sp->u.array=_ ;Pike_sp++->type=PIKE_T_ARRAY; }while(0)
#define ref_push_multiset(L) do{ struct multiset *_=(L); debug_malloc_touch(_); _->refs++; Pike_sp->u.multiset=_; Pike_sp++->type=PIKE_T_MULTISET; }while(0)
#define ref_push_string(S) do{ struct pike_string *_=(S); debug_malloc_touch(_); _->refs++; Pike_sp->subtype=0; Pike_sp->u.string=_; Pike_sp++->type=PIKE_T_STRING; }while(0)
#define ref_push_type_value(T) do{ ref_push_string(T); Pike_sp[-1].type = PIKE_T_TYPE; }while(0)
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
    DO_IF_DEBUG(if( Pike_fp->locals + Pike_fp->num_locals > Pike_sp || Pike_sp < Pike_fp->expendible) fatal("Stack failure in POP_PIKE_FRAME %p+%d=%p %p %p!\n",Pike_fp->locals,Pike_fp->num_locals,Pike_fp->locals+Pike_fp->num_locals,Pike_sp,Pike_fp->expendible));                      \
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

#define APPLY_MASTER(FUN,ARGS) \
do{ \
  static int fun_, master_cnt=0; \
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
  static int fun_, master_cnt=0; \
  struct object *master_ob=master(); \
  if(master_cnt != master_ob->prog->id) \
  { \
    fun_=find_identifier(FUN,master_ob->prog); \
    master_cnt = master_ob->prog->id; \
  } \
  safe_apply_low2(master_ob, fun_, ARGS, 1); \
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
ptrdiff_t pop_sp_mark(void);
PMOD_EXPORT void init_interpreter(void);
void lvalue_to_svalue_no_free(struct svalue *to,struct svalue *lval);
PMOD_EXPORT void assign_lvalue(struct svalue *lval,struct svalue *from);
PMOD_EXPORT union anything *get_pointer_if_this_type(struct svalue *lval, TYPE_T t);
void print_return_value(void);
void reset_evaluator(void);
struct backlog;
void dump_backlog(void);
BLOCK_ALLOC(pike_frame,128)

PMOD_EXPORT void find_external_context(struct external_variable_context *loc,
				       int arg2);
PMOD_EXPORT void mega_apply(enum apply_type type, INT32 args, void *arg1, void *arg2);
PMOD_EXPORT void f_call_function(INT32 args);
PMOD_EXPORT void call_handle_error(void);
PMOD_EXPORT int apply_low_safe_and_stupid(struct object *o, INT32 offset);
PMOD_EXPORT void safe_apply_low(struct object *o,int fun,int args);
PMOD_EXPORT void safe_apply_low2(struct object *o,int fun,int args, int handle_errors);
PMOD_EXPORT void safe_apply(struct object *o, char *fun ,INT32 args);
PMOD_EXPORT void safe_apply_handler(const char *fun,
				    struct object *handler,
				    struct object *compat,
				    INT32 args);
PMOD_EXPORT void apply_lfun(struct object *o, int fun, int args);
PMOD_EXPORT void apply_shared(struct object *o,
		  struct pike_string *fun,
		  int args);
PMOD_EXPORT void apply(struct object *o, char *fun, int args);
PMOD_EXPORT void apply_svalue(struct svalue *s, INT32 args);
PMOD_EXPORT void slow_check_stack(void);
PMOD_EXPORT void custom_check_stack(ptrdiff_t amount, const char *fmt, ...)
  ATTRIBUTE((format (printf, 2, 3)));
PMOD_EXPORT void cleanup_interpret(void);
void really_clean_up_interpret(void);
/* Prototypes end here */

/* These need to be after the prototypes,
 * to avoid implicit declaration of mega_apply().
 */
#ifdef __ECL
static inline void apply_low(struct object *o, ptrdiff_t fun, INT32 args)
{
  mega_apply(APPLY_LOW, args, (void*)o, (void*)fun);
}

static inline void strict_apply_svalue(struct svalue *sval, INT32 args)
{
  mega_apply(APPLY_SVALUE, args, (void*)sval, 0);
}
#else /* !__ECL */
#define apply_low(O,FUN,ARGS) \
  mega_apply(APPLY_LOW, (ARGS), (void*)(O),(void*)(ptrdiff_t)(FUN))

#define strict_apply_svalue(SVAL,ARGS) \
  mega_apply(APPLY_SVALUE, (ARGS), (void*)(SVAL),0)
#endif /* __ECL */

PMOD_EXPORT extern int d_flag; /* really in main.c */

PMOD_EXPORT extern int Pike_stack_size;
PMOD_EXPORT struct callback;
PMOD_EXPORT extern struct callback_list evaluator_callbacks;
PMOD_EXPORT extern void call_callback(struct callback_list *, void *);

/* Things to try:
 * we could reduce thread swapping to a pointer operation if
 * we do something like:
 *   #define Pike_interpreter (*Pike_interpreter_pointer)
 *
 * Since global variables are usually accessed through indirection
 * anyways, it might not make any speed differance.
 *
 * The above define could also be used to facilitate dynamic loading
 * on Win32..
 */
PMOD_EXPORT extern struct Pike_interpreter Pike_interpreter;

#define Pike_sp Pike_interpreter.stack_pointer
#define Pike_fp Pike_interpreter.frame_pointer
#define Pike_mark_sp Pike_interpreter.mark_stack_pointer
#ifdef PIKE_THREADS
#define Pike_thread_id Pike_interpreter.thread_id
#endif

#ifndef NO_PIKE_SHORTHAND

/* Shouldn't this be in Pike_interpreter? - Hubbe */
#define sp Pike_sp
#define fp Pike_fp
#define mark_sp Pike_mark_sp

#endif /* !NO_PIKE_SHORTHAND */

#define CURRENT_STORAGE (dmalloc_touch(struct pike_frame *,Pike_fp)->current_storage)

#endif



