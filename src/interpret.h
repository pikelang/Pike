/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef INTERPRET_H
#define INTERPRET_H

#include "global.h"
#include "program.h"
#include "pike_error.h"
#include "object.h"
#include "pike_rusage.h"
#include "pikecode.h"

struct catch_context
{
  struct catch_context *prev;
  JMP_BUF recovery;
  struct svalue *save_expendible;
  PIKE_OPCODE_T *next_addr;
  ptrdiff_t continue_reladdr;
#ifdef PIKE_DEBUG
  struct pike_frame *frame;
#endif
};

struct Pike_interpreter_struct {
  /* Swapped variables */
  struct svalue *stack_pointer;
  struct svalue *evaluator_stack;
  struct svalue **mark_stack_pointer;
  struct svalue **mark_stack;
  struct pike_frame *frame_pointer;
  JMP_BUF *recoveries;
#ifdef PIKE_THREADS
  struct thread_state *thread_state;
#endif
  char *stack_top;
  DO_IF_SECURITY(struct object *current_creds;)

  struct catch_context *catch_ctx;
  LOW_JMP_BUF *catching_eval_jmpbuf;

  int svalue_stack_margin;
  int c_stack_margin;

  INT16 evaluator_stack_malloced;
  INT16 mark_stack_malloced;

#ifdef PROFILING
  cpu_time_t accounted_time;	/** Time spent and accounted for so far. */
  cpu_time_t unlocked_time;	/** Time spent unlocked so far. */
  char *stack_bottom;
#endif

  int trace_level;
};

#ifndef STRUCT_FRAME_DECLARED
#define STRUCT_FRAME_DECLARED
#endif
struct pike_frame
{
  INT32 refs;/* must be first */
  INT32 args;			/** Actual number of arguments. */
  unsigned INT16 fun;		/** Function number. */
  INT16 num_locals;		/** Number of local variables. */
  INT16 num_args;		/** Number of argument variables. */
  unsigned INT16 flags;		/** PIKE_FRAME_* */
  INT16 ident;
  struct pike_frame *next;
  struct pike_frame *scope;
  PIKE_OPCODE_T *pc;		/** Address of current opcode. */
  PIKE_OPCODE_T *return_addr;	/** Address of opcode to continue at after call. */
  struct svalue *locals;	/** Start of local variables. */

  /** This is <= locals, and this is where the
   * return value should go.
   */
  struct svalue *save_sp;

  /**
   * This tells us the current level of svalues on the stack that can
   * be discarded once the current function is done with them
   */
  struct svalue *expendible;
  struct svalue **save_mark_sp;
  struct svalue **mark_sp_base;
  struct object *current_object;
  struct program *current_program;	/* program containing the context. */
  struct inherit *context;
  char *current_storage;

  DO_IF_SECURITY(struct object *current_creds;)
#if defined(PROFILING)
  cpu_time_t children_base;	/** Accounted time when the frame started. */
  cpu_time_t start_time;	/** Adjusted time when thr frame started. */
#endif
};

#define PIKE_FRAME_RETURN_INTERNAL 1
#define PIKE_FRAME_RETURN_POP 2
#define PIKE_FRAME_MALLOCED_LOCALS 0x8000

struct external_variable_context
{
  struct object *o;
  struct inherit *inherit;
  int parent_identifier;
};

#ifdef HAVE_COMPUTED_GOTO
extern PIKE_OPCODE_T *fcode_to_opcode;
extern struct op_2_f {
  PIKE_OPCODE_T opcode;
  INT32 fcode;
} *opcode_to_fcode;
#endif /* HAVE_COMPUTED_GOTO */



#ifdef PIKE_DEBUG
PMOD_EXPORT extern const char msg_stack_error[];
#define debug_check_stack() do{if(Pike_sp<Pike_interpreter.evaluator_stack)Pike_fatal("%s", msg_stack_error);}while(0)
#define check__positive(X,Y) if((X)<0) Pike_fatal Y
#else
#define check__positive(X,Y)
#define debug_check_stack() 
#endif

#define low_stack_check(X) \
  (Pike_sp - Pike_interpreter.evaluator_stack + \
   Pike_interpreter.svalue_stack_margin + (X) >= Pike_stack_size)

PMOD_EXPORT extern const char Pike_check_stack_errmsg[];

#define check_stack(X) do { \
  if(low_stack_check(X)) \
    ((void (*)(const char *, ...))Pike_error)( \
               Pike_check_stack_errmsg, \
	       PTRDIFF_T_TO_LONG(Pike_sp - Pike_interpreter.evaluator_stack), \
	       PTRDIFF_T_TO_LONG(Pike_stack_size), \
	       PTRDIFF_T_TO_LONG(X)); \
  }while(0)

PMOD_EXPORT extern const char Pike_check_mark_stack_errmsg[];

#define check_mark_stack(X) do {		\
  if(Pike_mark_sp - Pike_interpreter.mark_stack + (X) >= Pike_stack_size) \
    ((void (*)(const char*, ...))Pike_error)(Pike_check_mark_stack_errmsg); \
  }while(0)

PMOD_EXPORT extern const char Pike_check_c_stack_errmsg[];

#define low_check_c_stack(MIN_BYTES, RUN_IF_LOW) do {			\
    ptrdiff_t x_= (((char *)&x_) - Pike_interpreter.stack_top) +	\
      STACK_DIRECTION * (MIN_BYTES);					\
    x_*=STACK_DIRECTION;						\
    if(x_>0) {RUN_IF_LOW;}						\
  } while (0)


#define check_c_stack(MIN_BYTES) do {					\
    low_check_c_stack (Pike_interpreter.c_stack_margin + (MIN_BYTES), {	\
	low_error(Pike_check_c_stack_errmsg);				\
	/* Pike_fatal("C stack overflow: x_:%p &x_:%p top:%p margin:%p\n", \
	   x_, &x_, Pike_interpreter.stack_top,				\
	   Pike_interpreter.c_stack_margin + (MIN_BYTES)); */		\
      });								\
  }while(0)

#define fatal_check_c_stack(MIN_BYTES) do {				\
    low_check_c_stack ((MIN_BYTES), {					\
	((void (*)(const char*, ...))Pike_fatal)(Pike_check_c_stack_errmsg); \
      });								\
  }while(0)


#ifdef PIKE_DEBUG
#define STACK_LEVEL_START(depth)	\
  do { \
    struct svalue *save_stack_level = (Pike_sp - (depth))

#define STACK_LEVEL_DONE(depth)		\
    STACK_LEVEL_CHECK(depth);		\
  } while(0)

#define STACK_LEVEL_CHECK(depth)					\
  do {									\
    if (Pike_sp != save_stack_level + (depth)) {			\
      Pike_fatal("Unexpected stack level! "				\
		 "Actual: %d, expected: %d\n",				\
		 DO_NOT_WARN((int)(Pike_sp - save_stack_level)),	\
		 (depth));						\
    }									\
  } while(0)
#else /* !PIKE_DEBUG */
#define STACK_LEVEL_START(depth)	do {
#define STACK_LEVEL_DONE(depth)		} while(0)
#define STACK_LEVEL_CHECK(depth)
#endif /* PIKE_DEBUG */

#ifdef __CHECKER__
#define SET_SVAL_TYPE_CHECKER(S,T) SET_SVAL_TYPE_SUBTYPE(S,T,0)
#else
#define SET_SVAL_TYPE_CHECKER(S,T) SET_SVAL_TYPE_DC(S,T)
#endif

#define pop_stack() do{ free_svalue(--Pike_sp); debug_check_stack(); }while(0)
#define pop_2_elems() do { pop_stack(); pop_stack(); }while(0)

#ifdef __ECL
#define MAYBE_CAST_TO_LONG(X)	(X)
#else /* !__ECL */
#define MAYBE_CAST_TO_LONG(X)	((long)(X))
#endif /* __ECL */

PMOD_EXPORT extern const char msg_pop_neg[];
#define pop_n_elems(X)							\
 do {									\
   ptrdiff_t x_=(X);							\
   if(x_) {								\
     check__positive(x_, (msg_pop_neg, x_));				\
     if (x_ > 131072) Pike_fatal("Popping too much!\n");		\
     while (x_--) pop_stack();						\
     debug_check_stack();						\
   }									\
 } while (0)

/* This pops a number of arguments from the stack but keeps the top
 * element on top. Used for popping the arguments while keeping the
 * return value.
 */
#define stack_unlink(X) do {						\
    ptrdiff_t x2_ = (X);						\
    if (x2_) {								\
      struct svalue top;                                                \
      struct svalue *_sp_ = --Pike_sp;					\
      move_svalue (&top, _sp_);                                         \
      free_svalue (_sp_ - x2_);						\
      move_svalue (_sp_ - x2_, &top);					\
      pop_n_elems (x2_ - 1);						\
    }									\
  }while(0)

#define stack_pop_n_elems_keep_top(X) stack_unlink(X)

#define stack_pop_keep_top() do {					\
    struct svalue *_sp_ = --Pike_sp;					\
    free_svalue (_sp_ - 1);						\
    move_svalue (_sp_ - 1, _sp_);					\
    debug_check_stack();						\
  } while (0)

#define stack_pop_2_elems_keep_top() do {				\
    struct svalue *_sp_ = Pike_sp = Pike_sp - 2;			\
    free_svalue (_sp_ - 1);						\
    free_svalue (_sp_);							\
    move_svalue (_sp_ - 1, _sp_ + 1);					\
    debug_check_stack();						\
  } while (0)

#define stack_pop_to_no_free(X) move_svalue(X, --Pike_sp)

#define stack_pop_to(X) do {						\
    struct svalue *_=(X);						\
    free_svalue(_);							\
    stack_pop_to_no_free(_);						\
  }while(0)

#define push_program(P) do{						\
    struct program *_=(P);						\
    struct svalue *_sp_ = Pike_sp++;					\
    debug_malloc_touch(_);						\
    SET_SVAL_TYPE_CHECKER(*_sp_, PIKE_T_PROGRAM);			\
    _sp_->u.program=_;							\
  }while(0)

#define push_int(I) do{							\
    INT_TYPE _=(I);							\
    struct svalue *_sp_ = Pike_sp++;					\
    SET_SVAL_TYPE_SUBTYPE(*_sp_, PIKE_T_INT,NUMBER_NUMBER);		\
    _sp_->u.integer=_;							\
  }while(0)

#define push_undefined() do{						\
    struct svalue *_sp_ = Pike_sp++;					\
    SET_SVAL_TYPE_SUBTYPE(*_sp_, PIKE_T_INT,NUMBER_UNDEFINED);		\
    _sp_->u.integer=0;							\
  }while(0)

#define push_obj_index(I) do{						\
    int _=(I);								\
    struct svalue *_sp_ = Pike_sp++;					\
    SET_SVAL_TYPE_CHECKER(*_sp_, T_OBJ_INDEX);				\
    _sp_->u.identifier=_;						\
  }while(0)

#define push_mapping(M) do{						\
    struct mapping *_=(M);						\
    struct svalue *_sp_ = Pike_sp++;					\
    debug_malloc_touch(_);						\
    SET_SVAL_TYPE_CHECKER(*_sp_, PIKE_T_MAPPING);			\
    _sp_->u.mapping=_;							\
  }while(0)

#define push_array(A) do{						\
    struct array *_=(A);						\
    struct svalue *_sp_ = Pike_sp++;					\
    debug_malloc_touch(_);						\
    SET_SVAL_TYPE_CHECKER(*_sp_, PIKE_T_ARRAY);				\
    _sp_->u.array=_ ;							\
  }while(0)

#define push_empty_array() ref_push_array(&empty_array)

#define push_multiset(L) do{						\
    struct multiset *_=(L);						\
    struct svalue *_sp_ = Pike_sp++;					\
    debug_malloc_touch(_);						\
    SET_SVAL_TYPE_CHECKER(*_sp_, PIKE_T_MULTISET);			\
    _sp_->u.multiset=_;							\
  }while(0)

#define push_string(S) do {						\
    struct pike_string *_=(S);						\
    struct svalue *_sp_ = Pike_sp++;					\
    debug_malloc_touch(_);						\
    DO_IF_DEBUG(if(_->size_shift & ~3) {				\
		  Pike_fatal("Pushing string with bad shift: %d\n",	\
			     _->size_shift);				\
		});							\
    SET_SVAL_TYPE_SUBTYPE(*_sp_, PIKE_T_STRING,0);			\
    _sp_->u.string=_;							\
  }while(0)

#define push_empty_string() ref_push_string(empty_pike_string)

#define push_type_value(S) do{						\
    struct pike_type *_=(S);						\
    struct svalue *_sp_ = Pike_sp++;					\
    debug_malloc_touch(_);						\
    SET_SVAL_TYPE_CHECKER(*_sp_, PIKE_T_TYPE);				\
    _sp_->u.type=_;							\
  }while(0)

#define push_object(O) push_object_inherit(O,0)

#define push_object_inherit(O, INH_NUM) do {				\
    struct object *_ = (O);						\
    struct svalue *_sp_ = Pike_sp++;					\
    int _inh_ = (INH_NUM);						\
    debug_malloc_touch(_);						\
    SET_SVAL_TYPE_SUBTYPE(*_sp_, PIKE_T_OBJECT,_inh_);			\
    _sp_->u.object = _;							\
  }while(0)

#define push_float(F) do{						\
    FLOAT_TYPE _=(F);							\
    struct svalue *_sp_ = Pike_sp++;					\
    SET_SVAL_TYPE_CHECKER(*_sp_, PIKE_T_FLOAT);				\
    _sp_->u.float_number=_;						\
  }while(0)

PMOD_EXPORT extern void push_text( const char *x );

#define push_constant_text(T) do{					\
    struct svalue *_sp_ = Pike_sp;					\
    SET_SVAL_TYPE_SUBTYPE(*_sp_, PIKE_T_STRING,0);			\
    REF_MAKE_CONST_STRING(_sp_->u.string,T);				\
    Pike_sp = _sp_ + 1;                                                 \
  }while(0)

#define push_constant_string_code(STR, CODE) do{			\
    struct pike_string *STR;						\
    REF_MAKE_CONST_STRING_CODE (STR, CODE);				\
    push_string (STR);							\
  }while(0)

#define push_function(OBJ, FUN) do {					\
    INT16 _fun_ = (FUN);                                                \
    struct object *_=(OBJ);						\
    struct svalue *_sp_ = Pike_sp++;					\
    debug_malloc_touch(_);						\
    SET_SVAL_TYPE_SUBTYPE(*_sp_, PIKE_T_FUNCTION, _fun_);  	        \
    _sp_->u.object=_;							\
  } while (0)

#define ref_push_program(P) do{						\
    struct program *_=(P);						\
    struct svalue *_sp_ = Pike_sp++;					\
    add_ref(_);								\
    SET_SVAL_TYPE_CHECKER(*_sp_, PIKE_T_PROGRAM);			\
    _sp_->u.program=_;							\
  }while(0)

#define ref_push_mapping(M) do{						\
    struct mapping *_=(M);						\
    struct svalue *_sp_ = Pike_sp++;					\
    add_ref(_);								\
    SET_SVAL_TYPE_CHECKER(*_sp_, PIKE_T_MAPPING);			\
    _sp_->u.mapping=_;							\
  }while(0)

#define ref_push_array(A) do{						\
    struct array *_=(A);						\
    struct svalue *_sp_ = Pike_sp++;					\
    add_ref(_);								\
    SET_SVAL_TYPE_CHECKER(*_sp_, PIKE_T_ARRAY);				\
    _sp_->u.array=_ ;							\
  }while(0)

#define ref_push_multiset(L) do{					\
    struct multiset *_=(L);						\
    struct svalue *_sp_ = Pike_sp++;					\
    add_ref(_);								\
    SET_SVAL_TYPE_CHECKER(*_sp_, PIKE_T_MULTISET);			\
    _sp_->u.multiset=_;							\
  }while(0)

#define ref_push_string(S) do{						\
    struct pike_string *_=(S);						\
    struct svalue *_sp_ = Pike_sp++;					\
    DO_IF_DEBUG(if(_->size_shift & ~3) {				\
		  Pike_fatal("Pushing string with bad shift: %d\n",	\
			     _->size_shift);				\
		});							\
    add_ref(_);								\
    SET_SVAL_TYPE_SUBTYPE(*_sp_, PIKE_T_STRING,0);			\
    _sp_->u.string=_;							\
  }while(0)

#define ref_push_type_value(S) do{					\
    struct pike_type *_=(S);						\
    struct svalue *_sp_ = Pike_sp++;					\
    add_ref(_);								\
    SET_SVAL_TYPE_SUBTYPE(*_sp_, PIKE_T_TYPE,0);			\
    _sp_->u.type=_;							\
  }while(0)

#define ref_push_object(O) ref_push_object_inherit(O,0)

#define ref_push_object_inherit(O, INH_NUM) do{				\
    INT16 _inherit_ = (INH_NUM);                                        \
    struct object  *_ = (O);						\
    struct svalue *_sp_ = Pike_sp++;					\
    add_ref(_);								\
    SET_SVAL_TYPE_SUBTYPE(*_sp_, PIKE_T_OBJECT, _inherit_);		\
    _sp_->u.object = _;							\
  }while(0)

#define ref_push_function(OBJ, FUN) do {				\
    INT16 _fun_ = (FUN);                                                \
    struct object *_=(OBJ);						\
    struct svalue *_sp_ = Pike_sp++;					\
    add_ref(_);								\
    SET_SVAL_TYPE_SUBTYPE(*_sp_, PIKE_T_FUNCTION, _fun_);		\
    _sp_->u.object=_;							\
  } while (0)

#define push_svalue(S) do {						\
    const struct svalue *_=(S);						\
    struct svalue *_sp_ = Pike_sp;					\
    assign_svalue_no_free(_sp_,_);					\
    Pike_sp = _sp_ + 1;                                                 \
  }while(0)

#define stack_dup() push_svalue(Pike_sp-1)

#define stack_swap() do {						\
    struct svalue *_sp_ = Pike_sp;					\
    struct svalue _=_sp_[-1];						\
    _sp_[-1]=_sp_[-2];							\
    _sp_[-2]=_;								\
  } while(0)

#define stack_revroll(args) do {					\
    struct svalue *_sp_ = Pike_sp;					\
    int _args_ = (args); struct svalue _=_sp_[-1];			\
    memmove(_sp_-_args_+1, _sp_-_args_, (_args_-1)*sizeof(struct svalue)); \
    _sp_[-_args_]=_;							\
  } while(0)

#if PIKE_T_INT+NUMBER_NUMBER==0 && defined(HAS___BUILTIN_MEMSET)
#define push_zeroes(N) do{					\
    ptrdiff_t num_ = (N);					\
    __builtin_memset(Pike_sp,0,sizeof(struct svalue)*(num_));	\
    Pike_sp+=num_;						\
  } while(0);
#else
#define push_zeroes(N) do{			\
    struct svalue *s_ = Pike_sp;		\
    ptrdiff_t num_= (N);			\
    for(;num_-- > 0;s_++)			\
    {						\
      SET_SVAL_TYPE_SUBTYPE(*s_, PIKE_T_INT,NUMBER_NUMBER);	\
      s_->u.integer=0;				\
    }						\
    Pike_sp=s_;					\
}while(0)
#endif

#define push_undefines(N) do{			\
    struct svalue *s_ = Pike_sp;		\
    ptrdiff_t num_= (N);			\
    for(;num_-- > 0;s_++)			\
    {						\
      SET_SVAL_TYPE_SUBTYPE(*s_, PIKE_T_INT,NUMBER_UNDEFINED);	\
      s_->u.integer=0;				\
    }						\
    Pike_sp=s_;					\
}while(0)


#define free_pike_frame(F) do{ struct pike_frame *f_=(F); if(!sub_ref(f_)) really_free_pike_frame(f_); }while(0)

/* A scope is any frame which may have malloced locals */
#define free_pike_scope(F) do{ struct pike_frame *f_=(F); if(!sub_ref(f_)) really_free_pike_scope(f_); }while(0)

/* Without fancy accounting stuff. This one can't assume there is an
 * identifier corresponding to the frame (i.e. _fp_->ident might be
 * bogus). */
#define LOW_POP_PIKE_FRAME(_fp_) do {					\
    struct pike_frame *tmp_=_fp_->next;					\
    if(!sub_ref(_fp_))							\
    {									\
      really_free_pike_frame(_fp_);					\
    }else{								\
      ptrdiff_t num_expendible = _fp_->expendible - _fp_->locals;	\
      DO_IF_DEBUG(							\
	if( (_fp_->locals + _fp_->num_locals > Pike_sp) ||		\
	    (Pike_sp < _fp_->expendible) ||				\
	    (num_expendible < 0) || (num_expendible > _fp_->num_locals)) \
	  Pike_fatal("Stack failure in POP_PIKE_FRAME %p+%d=%p %p %p!\n", \
		     _fp_->locals, _fp_->num_locals,			\
		     _fp_->locals+_fp_->num_locals,			\
		     Pike_sp,_fp_->expendible));			\
      debug_malloc_touch(_fp_);						\
      if(num_expendible)						\
      {									\
	struct svalue *s=(struct svalue *)xalloc(sizeof(struct svalue)*	\
						 num_expendible);	\
	_fp_->num_locals = num_expendible;				\
	assign_svalues_no_free(s, _fp_->locals, num_expendible,		\
			       BIT_MIXED);				\
	_fp_->locals=s;							\
	_fp_->flags|=PIKE_FRAME_MALLOCED_LOCALS;			\
      }else{								\
	_fp_->locals=0;							\
      }									\
      _fp_->next=0;							\
    }									\
    Pike_fp=tmp_;							\
  } while (0)

#define POP_PIKE_FRAME() do {						\
  struct pike_frame *_fp_ = Pike_fp;					\
  DO_IF_PROFILING({							\
      /* Time spent in this frame + children. */			\
      cpu_time_t time_passed =						\
	get_cpu_time() - Pike_interpreter.unlocked_time;		\
      /* Time spent in children to this frame. */			\
      cpu_time_t time_in_children;					\
      /* Time spent in just this frame. */				\
      cpu_time_t self_time;						\
      struct identifier *function;					\
      DO_IF_PROFILING_DEBUG({						\
	  fprintf(stderr, "%p}: Pop got %" PRINT_CPU_TIME		\
		  " (%" PRINT_CPU_TIME ")"				\
		  " %" PRINT_CPU_TIME " (%" PRINT_CPU_TIME ")\n",	\
		  Pike_interpreter.thread_state, time_passed,		\
		  _fp_->start_time, Pike_interpreter.accounted_time,	\
		  _fp_->children_base);					\
	});								\
      time_passed -= _fp_->start_time;					\
      DO_IF_DEBUG(if (time_passed < 0) {				\
		    Pike_fatal("Negative time_passed: %" PRINT_CPU_TIME	\
			       " now: %" PRINT_CPU_TIME			\
			       " unlocked_time: %" PRINT_CPU_TIME	\
			       " start_time: %" PRINT_CPU_TIME		\
			       "\n", time_passed, get_cpu_time(),	\
			       Pike_interpreter.unlocked_time,		\
			       _fp_->start_time);			\
		  });							\
      time_in_children =						\
	Pike_interpreter.accounted_time - _fp_->children_base;		\
      DO_IF_DEBUG(if (time_in_children < 0) {				\
		    Pike_fatal("Negative time_in_children: %"		\
			       PRINT_CPU_TIME				\
			       " accounted_time: %" PRINT_CPU_TIME	\
			       " children_base: %" PRINT_CPU_TIME	\
			       "\n", time_in_children,			\
			       Pike_interpreter.accounted_time,		\
			       _fp_->children_base);			\
		  });							\
      self_time = time_passed - time_in_children;			\
      DO_IF_DEBUG(if (self_time < 0) {					\
		    Pike_fatal("Negative self_time: %" PRINT_CPU_TIME	\
			       " time_passed: %" PRINT_CPU_TIME		\
			       " time_in_children: %" PRINT_CPU_TIME	\
			       "\n", self_time, time_passed,		\
			       time_in_children);			\
		  });							\
      Pike_interpreter.accounted_time += self_time;			\
      /* FIXME: Can context->prog be NULL? */				\
      function = _fp_->context->prog->identifiers + _fp_->ident;	\
      if (!--function->recur_depth)					\
	function->total_time += time_passed;				\
      function->self_time += self_time;					\
    });									\
  LOW_POP_PIKE_FRAME (_fp_);						\
 }while(0)

#define ASSIGN_CURRENT_STORAGE(VAR, TYPE, INH, EXPECTED_PROGRAM)	\
  do {									\
    int inh__ = (INH);							\
    DO_IF_DEBUG(							\
      struct program *prog__ = (EXPECTED_PROGRAM);			\
      if ((inh__ < 0) ||						\
	  (inh__ >= Pike_fp->context->prog->num_inherits))		\
	Pike_fatal("Inherit #%d out of range [0..%d]\n",		\
		   inh__, Pike_fp->context->prog->num_inherits-1);	\
      if (prog__ && (Pike_fp->context[inh__].prog != prog__))		\
	Pike_fatal("Inherit #%d has wrong program %p != %p.\n",		\
		   Pike_fp->context[inh__].prog, prog__);		\
    );									\
    VAR = ((TYPE *)(Pike_fp->current_object->storage +			\
		    Pike_fp->context[inh__].storage_offset));		\
  } while(0)


enum apply_type
{
  APPLY_STACK, /* The function is the first argument */
  APPLY_SVALUE, /* arg1 points to an svalue containing the function */
  APPLY_SVALUE_STRICT, /* Like APPLY_SVALUE, but does not return values for void functions */
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
  if (fun_ >= 0) { \
    apply_low(master_ob, fun_, ARGS); \
  } else { \
    Pike_error("Cannot call undefined function \"%s\" in master.\n", FUN); \
  } \
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
  safe_apply_low2(master_ob, fun_, ARGS, FUN); \
}while(0)

#define SAFE_APPLY_HANDLER(FUN, HANDLER, COMPAT, ARGS) do {	\
    static int h_fun_=-1, h_id_=0;				\
    static int c_fun_=-1, c_fun_id_=0;				\
    struct object *h_=(HANDLER), *c_=(COMPAT);			\
    if (h_ && h_->prog) {					\
      if (h_->prog->id != h_id_) {				\
	h_fun_ = find_identifier(fun, h_->prog);		\
	h_id_ = h_->prog->id;					\
      }								\
      if (h_fun_ != -1) {					\
	safe_apply_low(h_, h_fun_, ARGS);			\
	break;							\
      }								\
    }								\
    if (c_ && c_->prog) {					\
      if (c_->prog->id != c_id_) {				\
	c_fun_ = find_identifier(fun, c_->prog);		\
	c_id_ = c_->prog->id;					\
      }								\
      if (c_fun_ != -1) {					\
	safe_apply_low(c_, c_fun_, ARGS);			\
	break;							\
      }								\
    }								\
    SAFE_APPLY_MASTER(FUN, ARGS);				\
  } while(0)


#ifdef INTERNAL_PROFILING
PMOD_EXPORT extern unsigned long evaluator_callback_calls;
#endif

#define low_check_threads_etc() do { \
  DO_IF_INTERNAL_PROFILING (evaluator_callback_calls++); \
  call_callback(& evaluator_callbacks, NULL); \
}while(0) 

#define check_threads_etc() do {					\
    DO_IF_DEBUG (if (Pike_interpreter.trace_level > 2)			\
		   fprintf (stderr, "- thread yield point\n"));		\
    low_check_threads_etc();						\
  } while (0)

PMOD_EXPORT extern int fast_check_threads_counter;

#define fast_check_threads_etc(X) do {					\
    DO_IF_DEBUG (if (Pike_interpreter.trace_level > 2)			\
		   fprintf (stderr, "- thread yield point\n"));		\
    if (++fast_check_threads_counter >= (1 << (X))) {			\
      fast_check_threads_counter = 0;					\
      low_check_threads_etc();						\
    }									\
  } while(0)

/* Used before any sort of pike level function call. This covers most
 * code paths. An interval with magnitude 6 on a 2 GHz AMD 64 averages
 * on 1000-3000 check_threads_etc calls per second in mixed code, but
 * it can vary greatly - from 0 to 30000+ calls/sec. In a test case
 * with about 22000 calls/sec, it took 0.042% of the cpu. */
#define FAST_CHECK_THREADS_ON_CALL() do {				\
    DO_IF_DEBUG (if (Pike_interpreter.trace_level > 2)			\
		   fprintf (stderr, "- thread yield point\n"));		\
    if (++fast_check_threads_counter >= (1 << 6)) {			\
      fast_check_threads_counter = 0;					\
      low_check_threads_etc();						\
    }									\
    else if (objects_to_destruct)					\
      /* De facto pike semantics requires that freed objects are */	\
      /* destructed before function calls. Otherwise done through */	\
      /* evaluator_callbacks. */					\
      destruct_objects_to_destruct_cb();				\
  } while (0)

/* Used before any sort of backward branch. This is only a safeguard
 * for some corner cases with loops without calls - not relevant in
 * ordinary code. */
#define FAST_CHECK_THREADS_ON_BRANCH() fast_check_threads_etc (8)

/* Prototypes begin here */
void push_sp_mark(void);
ptrdiff_t pop_sp_mark(void);
void gc_mark_stack_external (struct pike_frame *frame,
			     struct svalue *stack_p, struct svalue *stack);
PMOD_EXPORT int low_init_interpreter(struct Pike_interpreter_struct *interpreter);
PMOD_EXPORT void init_interpreter(void);
int lvalue_to_svalue_no_free(struct svalue *to, struct svalue *lval);
PMOD_EXPORT void assign_lvalue(struct svalue *lval,struct svalue *from);
PMOD_EXPORT union anything *get_pointer_if_this_type(struct svalue *lval, TYPE_T t);
void print_return_value(void);
void reset_evaluator(void);
struct backlog;
void dump_backlog(void);
struct catch_context *alloc_catch_context(void);
PMOD_EXPORT void really_free_catch_context( struct catch_context *data );
PMOD_EXPORT void really_free_pike_frame( struct pike_frame *X );
void count_memory_in_catch_contexts(size_t*, size_t*);
void count_memory_in_pike_frames(size_t*, size_t*);

/*BLOCK_ALLOC (catch_context, 0);*/
/*BLOCK_ALLOC(pike_frame,128);*/

#ifdef PIKE_USE_MACHINE_CODE
void call_check_threads_etc(void);
#if defined(OPCODE_INLINE_BRANCH) || defined(INS_F_JUMP) || \
    defined(INS_F_JUMP_WITH_ARG) || defined(INS_F_JUMP_WITH_TWO_ARGS)
void branch_check_threads_etc(void);
#endif
#ifdef OPCODE_INLINE_RETURN
PIKE_OPCODE_T *inter_return_opcode_F_CATCH(PIKE_OPCODE_T *addr);
#endif
#ifdef OPCODE_INLINE_CATCH
PIKE_OPCODE_T *setup_catch_context(PIKE_OPCODE_T *addr);
PIKE_OPCODE_T *handle_caught_exception(void);
#endif
#ifdef PIKE_DEBUG
void simple_debug_instr_prologue_0 (PIKE_INSTR_T instr);
void simple_debug_instr_prologue_1 (PIKE_INSTR_T instr, INT32 arg);
void simple_debug_instr_prologue_2 (PIKE_INSTR_T instr, INT32 arg1, INT32 arg2);
#endif
#endif	/* PIKE_USE_MACHINE_CODE */

PMOD_EXPORT void find_external_context(struct external_variable_context *loc,
				       int arg2);
struct pike_frame *alloc_pike_frame(void);
void really_free_pike_scope(struct pike_frame *scope);
void *lower_mega_apply( INT32 args, struct object *o, ptrdiff_t fun );
void *low_mega_apply(enum apply_type type, INT32 args, void *arg1, void *arg2);
void low_return(void);
void low_return_pop(void);
void unlink_previous_frame(void);
int apply_low_safe_and_stupid(struct object *o, INT32 offset);

PMOD_EXPORT struct Pike_interpreter_struct * pike_get_interpreter_pointer();
PMOD_EXPORT void mega_apply(enum apply_type type, INT32 args, void *arg1, void *arg2);
PMOD_EXPORT void mega_apply_low(INT32 args, void *arg1, ptrdiff_t arg2);
PMOD_EXPORT void f_call_function(INT32 args);
PMOD_EXPORT void call_handle_error(void);
PMOD_EXPORT int safe_apply_low(struct object *o,int fun,int args);
PMOD_EXPORT int safe_apply_low2(struct object *o,int fun,int args,
				 const char *fun_name);
PMOD_EXPORT int safe_apply(struct object *o, const char *fun ,INT32 args);
int low_unsafe_apply_handler(const char *fun,
					 struct object *handler,
					 struct object *compat,
					 INT32 args);
void low_safe_apply_handler(const char *fun,
					struct object *handler,
					struct object *compat,
					INT32 args);
PMOD_EXPORT int safe_apply_handler(const char *fun,
				   struct object *handler,
				   struct object *compat,
				   INT32 args,
				   TYPE_FIELD rettypes);
PMOD_EXPORT void apply_lfun(struct object *o, int fun, int args);
PMOD_EXPORT void apply_shared(struct object *o,
		  struct pike_string *fun,
		  int args);
PMOD_EXPORT void apply(struct object *o, const char *fun, int args);
PMOD_EXPORT void apply_svalue(struct svalue *s, INT32 args);
PMOD_EXPORT void safe_apply_svalue (struct svalue *s, INT32 args, int handle_errors);
PMOD_EXPORT void apply_external(int depth, int fun, INT32 args);
void slow_check_stack(void);
PMOD_EXPORT void custom_check_stack(ptrdiff_t amount, const char *fmt, ...)
  ATTRIBUTE((format (printf, 2, 3)));
PMOD_EXPORT void cleanup_interpret(void);
PMOD_EXPORT void low_cleanup_interpret(struct Pike_interpreter_struct *interpreter);
void really_clean_up_interpret(void);
/* Prototypes end here */

/* These need to be after the prototypes,
 * to avoid implicit declaration of mega_apply().
 */
#ifdef __ECL
static INLINE void apply_low(struct object *o, ptrdiff_t fun, INT32 args)
{
  mega_apply_low(args, (void*)o, fun);
}

static INLINE void strict_apply_svalue(struct svalue *sval, INT32 args)
{
  mega_apply(APPLY_SVALUE_STRICT, args, (void*)sval, 0);
}
#else /* !__ECL */
#define apply_low(O,FUN,ARGS) \
  mega_apply_low((ARGS), (void*)(O),(FUN))

#define strict_apply_svalue(SVAL,ARGS) \
  mega_apply(APPLY_SVALUE, (ARGS), (void*)(SVAL),0)
#endif /* __ECL */

#define apply_current(FUN, ARGS)			\
  apply_low(Pike_fp->current_object,			\
	    (FUN) + Pike_fp->context->identifier_level,	\
	    (ARGS))

#define safe_apply_current(FUN, ARGS)				\
  safe_apply_low(Pike_fp->current_object,			\
		 (FUN) + Pike_fp->context->identifier_level,	\
		 (ARGS))

#define safe_apply_current2(FUN, ARGS, FUNNAME)			\
  safe_apply_low2(Pike_fp->current_object,			\
		  (FUN) + Pike_fp->context->identifier_level,	\
		  (ARGS), (FUNNAME))

PMOD_EXPORT extern int d_flag; /* really in main.c */

PMOD_EXPORT extern int Pike_stack_size;
struct callback;
PMOD_EXPORT extern struct callback_list evaluator_callbacks;

PMOD_EXPORT extern struct Pike_interpreter_struct *
#if defined(__GNUC__) && __GNUC__ >= 3
    __restrict
#endif
    Pike_interpreter_pointer;
#define Pike_interpreter (*Pike_interpreter_pointer)

#define Pike_sp Pike_interpreter.stack_pointer
#define Pike_fp Pike_interpreter.frame_pointer
#define Pike_mark_sp Pike_interpreter.mark_stack_pointer


#define CURRENT_STORAGE (dmalloc_touch(struct pike_frame *,Pike_fp)->current_storage)


#define PIKE_STACK_MMAPPED

struct Pike_stack
{
  struct svalue *top;
  int flags;
  struct Pike_stack *previous;
  struct svalue *save_ptr;
  struct svalue stack[1];
};


#define PIKE_STACK_REQUIRE_BEGIN(num, base) do {			\
  struct Pike_stack *old_stack_;					\
  if(Pike_interpreter.current_stack->top - Pike_sp < num)		\
  {									\
    old_stack_=Pike_interpreter.current_stack;				\
    old_stack_->save_ptr=Pike_sp;					\
    Pike_interpreter.current_stack=allocate_array(MAXIMUM(num, 8192));	\
    while(old_sp > base) *(Pike_sp++) = *--old_stack_->save_ptr;	\
  }

#define PIKE_STACK_REQUIRE_END()					   \
  while(Pike_sp > Pike_interpreter.current_stack->stack)		   \
    *(old_stack_->save_ptr++) = *--Pike_sp;				   \
  Pike_interpreter.current_stack=Pike_interpreter.current_stack->previous; \
}while(0)  

#endif
