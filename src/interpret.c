/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
RCSID("$Id: interpret.c,v 1.50 1997/10/14 03:10:10 hubbe Exp $");
#include "interpret.h"
#include "object.h"
#include "program.h"
#include "svalue.h"
#include "array.h"
#include "mapping.h"
#include "error.h"
#include "language.h"
#include "stralloc.h"
#include "constants.h"
#include "pike_macros.h"
#include "multiset.h"
#include "backend.h"
#include "operators.h"
#include "opcodes.h"
#include "main.h"
#include "lex.h"
#include "builtin_functions.h"
#include "signal_handler.h"
#include "gc.h"
#include "threads.h"
#include "callback.h"
#include "fd_control.h"

#include <fcntl.h>
#include <errno.h>

#ifdef HAVE_MMAP
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#ifdef MAP_NORESERVE
#define USE_MMAP_FOR_STACK
#endif
#endif

/*
 * Define the default evaluator stack size, used for just about everything.
 */
#define EVALUATOR_STACK_SIZE	100000

#define TRACE_LEN (100 + t_flag * 10)


/* sp points to first unused value on stack
 * (much simpler than letting it point at the last used value.)
 */
struct svalue *sp;     /* Current position */
struct svalue *evaluator_stack; /* Start of stack */
int stack_size = EVALUATOR_STACK_SIZE;
int evaluator_stack_malloced = 0;

/* mark stack, used to store markers into the normal stack */
struct svalue **mark_sp; /* Current position */
struct svalue **mark_stack; /* Start of stack */
int mark_stack_malloced = 0;

void push_sp_mark(void)
{
  if(mark_sp == mark_stack + stack_size)
    error("No more mark stack!\n");
  *mark_sp++=sp;
}
int pop_sp_mark(void)
{
#ifdef DEBUG
  if(mark_sp < mark_stack)
    fatal("Mark stack underflow!\n");
#endif
  return sp - *--mark_sp;
}

struct frame *fp; /* frame pointer */

#ifdef DEBUG
static void gc_check_stack_callback(struct callback *foo, void *bar, void *gazonk)
{
  gc_xmark_svalues(evaluator_stack,sp-evaluator_stack-1);
}
#endif

void init_interpreter(void)
{
#ifdef USE_MMAP_FOR_STACK
  static int fd = -1;

#ifdef DEBUG
  static struct callback *spcb;
  if(!spcb)
  {
    spcb=add_gc_callback(gc_check_stack_callback,0,0);
  }
#endif

#ifndef MAP_VARIABLE
#define MAP_VARIABLE 0
#endif

#ifndef MAP_PRIVATE
#define MAP_PRIVATE 0
#endif

#ifndef MAP_FAILED
#define MAP_FAILED -1
#endif

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS 0
  if(fd == -1)
  {
    while(1)
    {
      fd=open("/dev/zero",O_RDONLY);
      if(fd >= 0) break;
      if(errno != EINTR)
      {
	evaluator_stack=0;
	mark_stack=0;
	goto use_malloc;
      }
    }
  }
#endif

#define MMALLOC(X,Y) (Y *)mmap(0,X*sizeof(Y),PROT_READ|PROT_WRITE, MAP_NORESERVE | MAP_PRIVATE | MAP_ANONYMOUS, fd, 0)

  evaluator_stack_malloced=0;
  mark_stack_malloced=0;
  evaluator_stack=MMALLOC(stack_size,struct svalue);
  mark_stack=MMALLOC(stack_size, struct svalue *);
  if((char *)MAP_FAILED == (char *)evaluator_stack) evaluator_stack=0;
  if((char *)MAP_FAILED == (char *)mark_stack) mark_stack=0;
#else
  evaluator_stack=0;
  mark_stack=0;
#endif

use_malloc:
  if(!evaluator_stack)
  {
    evaluator_stack=(struct svalue *)xalloc(stack_size*sizeof(struct svalue));
    evaluator_stack_malloced=1;
  }

  if(!mark_stack)
  {
    mark_stack=(struct svalue **)xalloc(stack_size*sizeof(struct svalue *));
    mark_stack_malloced=1;
  }

  sp=evaluator_stack;
  mark_sp=mark_stack;
  fp=0;
}

void check_stack(INT32 size)
{
  if(sp - evaluator_stack + size >= stack_size)
    error("Stack overflow.\n");
}

void check_mark_stack(INT32 size)
{
  if(mark_sp - mark_stack + size >= stack_size)
    error("Stack overflow.\n");
}


static void eval_instruction(unsigned char *pc);


/*
 * lvalues are stored in two svalues in one of these formats:
 * array[index]   : { array, index } 
 * mapping[index] : { mapping, index } 
 * multiset[index] : { multiset, index } 
 * object[index] : { object, index }
 * local variable : { svalue_pointer, nothing } 
 * global variable : { svalue_pointer/short_svalue_pointer, nothing } 
 */

void lvalue_to_svalue_no_free(struct svalue *to,struct svalue *lval)
{
  switch(lval->type)
  {
  case T_LVALUE:
    assign_svalue_no_free(to, lval->u.lval);
    break;

  case T_SHORT_LVALUE:
    assign_from_short_svalue_no_free(to, lval->u.short_lval, lval->subtype);
    break;

  case T_OBJECT:
    object_index_no_free(to, lval->u.object, lval+1);
    break;

  case T_ARRAY:
    simple_array_index_no_free(to, lval->u.array, lval+1);
    break;

  case T_MAPPING:
    mapping_index_no_free(to, lval->u.mapping, lval+1);
    break;

  case T_MULTISET:
    to->type=T_INT;
    if(multiset_member(lval->u.multiset,lval+1))
    {
      to->u.integer=0;
      to->subtype=NUMBER_UNDEFINED;
    }else{
      to->u.integer=0;
      to->subtype=NUMBER_NUMBER;
    }
    break;
    
  default:
   if(IS_ZERO(lval))
     error("Indexing the NULL value.\n"); /* Per */
   else
     error("Indexing a basic type.\n");
  }
}

void assign_lvalue(struct svalue *lval,struct svalue *from)
{
  switch(lval->type)
  {
  case T_LVALUE:
    assign_svalue(lval->u.lval,from);
    break;

  case T_SHORT_LVALUE:
    assign_to_short_svalue(lval->u.short_lval, lval->subtype, from);
    break;

  case T_OBJECT:
    object_set_index(lval->u.object, lval+1, from);
    break;

  case T_ARRAY:
    simple_set_index(lval->u.array, lval+1, from);
    break;

  case T_MAPPING:
    mapping_insert(lval->u.mapping, lval+1, from);
    break;

  case T_MULTISET:
    if(IS_ZERO(from))
      multiset_delete(lval->u.multiset, lval+1);
    else
      multiset_insert(lval->u.multiset, lval+1);
    break;
    
  default:
   if(IS_ZERO(lval))
     error("Indexing the NULL value.\n"); /* Per */
   else
     error("Indexing a basic type.\n");
  }
}

union anything *get_pointer_if_this_type(struct svalue *lval, TYPE_T t)
{
  switch(lval->type)
  {
  case T_LVALUE:
    if(lval->u.lval->type == t) return & ( lval->u.lval->u );
    return 0;

  case T_SHORT_LVALUE:
    if(lval->subtype == t) return lval->u.short_lval;
    return 0;

  case T_OBJECT:
    return object_get_item_ptr(lval->u.object,lval+1,t);

  case T_ARRAY:
    return array_get_item_ptr(lval->u.array,lval+1,t);

  case T_MAPPING:
    return mapping_get_item_ptr(lval->u.mapping,lval+1,t);

  case T_MULTISET: return 0;

  default:
    if(IS_ZERO(lval))
      error("Indexing the NULL value.\n"); /* Per */
    else
      error("Indexing a basic type.\n");
    return 0;
  }
}

#ifdef DEBUG
void print_return_value(void)
{
  if(t_flag>3)
  {
    char *s;
    int nonblock;
	
    if((nonblock=query_nonblocking(2)))
      set_nonblocking(2,0);
	
    init_buf();
    describe_svalue(sp-1,0,0);
    s=simple_free_buf();
    if((long)strlen(s) > (long)TRACE_LEN)
    {
      s[TRACE_LEN]=0;
      s[TRACE_LEN-1]='.';
      s[TRACE_LEN-2]='.';
      s[TRACE_LEN-2]='.';
    }
    fprintf(stderr,"-    value: %s\n",s);
    free(s);
	
    if(nonblock)
      set_nonblocking(2,1);
  }
}
#else
#define print_return_value()
#endif


void pop_n_elems(INT32 x)
{
#ifdef DEBUG
  if(sp - evaluator_stack < x)
    fatal("Popping out of stack.\n");

  if(x < 0) fatal("Popping negative number of args....\n");
#endif
  sp-=x;
  free_svalues(sp,x,BIT_MIXED);
}


struct callback_list evaluator_callbacks;

#ifdef DEBUG
static char trace_buffer[100];
#define GET_ARG() (backlog[backlogp].arg=(\
  instr=prefix,\
  prefix=0,\
  instr+=EXTRACT_UCHAR(pc++),\
  (t_flag>3 ? sprintf(trace_buffer,"-    Arg = %ld\n",(long)instr),write_to_stderr(trace_buffer,strlen(trace_buffer)) : 0),\
  instr))

#else
#define GET_ARG() (instr=prefix,prefix=0,instr+EXTRACT_UCHAR(pc++))
#endif

#define CASE(X) case (X)-F_OFFSET:

#define DOJUMP() \
 do { int tmp; tmp=EXTRACT_INT(pc); pc+=tmp; if(tmp < 0) fast_check_threads_etc(6); }while(0)

#define COMPARISMENT(ID,EXPR) \
CASE(ID); \
instr=EXPR; \
pop_n_elems(2); \
push_int(instr); \
break

#define LOOP(ID, OP1, OP2, OP3, OP4)				\
CASE(ID)							\
{								\
  union anything *i=get_pointer_if_this_type(sp-2, T_INT);	\
  if(i)								\
  {								\
    OP1 ( i->integer );						\
    if(i->integer OP2 sp[-3].u.integer)				\
    {								\
      pc+=EXTRACT_INT(pc);					\
      fast_check_threads_etc(8);				\
    }else{							\
      pc+=sizeof(INT32);					\
    }								\
  }else{							\
    lvalue_to_svalue_no_free(sp-2,sp); sp++;			\
    push_int(1);						\
    OP3;							\
    assign_lvalue(sp-3,sp-1);					\
    if(OP4 ( sp-1, sp-4 ))					\
    {								\
      pc+=EXTRACT_INT(pc);					\
      fast_check_threads_etc(8);				\
    }else{							\
      pc+=sizeof(INT32);					\
    }								\
    pop_stack();						\
  }								\
  break;							\
}

#define CJUMP(X,Y) \
CASE(X); \
if(Y(sp-2,sp-1)) { \
  DOJUMP(); \
}else{ \
  pc+=sizeof(INT32); \
} \
pop_n_elems(2); \
break


/*
 * reset the stack machine.
 */
void reset_evaluator(void)
{
  fp=0;
  pop_n_elems(sp - evaluator_stack);
}

#ifdef DEBUG
#define BACKLOG 512
struct backlog
{
  INT32 instruction;
  INT32 arg;
  struct program *program;
  unsigned char *pc;
};

struct backlog backlog[BACKLOG];
int backlogp=BACKLOG-1;

void dump_backlog(void)
{
  int e;
  if(!d_flag || backlogp<0 || backlogp>=BACKLOG)
    return;

  e=backlogp;
  do
  {
    e++;
    if(e>=BACKLOG) e=0;

    if(backlog[e].program)
    {
      char *file;
      INT32 line;

      file=get_line(backlog[e].pc-1,backlog[e].program, &line);
      fprintf(stderr,"%s:%ld: %s(%ld)\n",
	      file,
	      (long)line,
	      low_get_f_name(backlog[e].instruction + F_OFFSET, backlog[e].program),
	      (long)backlog[e].arg);
    }
  }while(e!=backlogp);
}

#endif

static int o_catch(unsigned char *pc);

static void eval_instruction(unsigned char *pc)
{
  unsigned INT32 instr, prefix=0;
  while(1)
  {
    fp->pc = pc;
    instr=EXTRACT_UCHAR(pc++);

#ifdef DEBUG
    if(d_flag)
    {
#ifdef _REENTRANT
      if(!mt_trylock(& interpreter_lock))
	fatal("Interpreter running unlocked!\n");
#endif
      sp[0].type=99; /* an invalid type */
      sp[1].type=99;
      sp[2].type=99;
      sp[3].type=99;
      
      if(sp<evaluator_stack || mark_sp < mark_stack || fp->locals>sp)
	fatal("Stack error (generic).\n");
      
      if(sp > evaluator_stack+stack_size)
	fatal("Stack error (overflow).\n");
      
      if(fp->fun>=0 && fp->current_object->prog &&
	 fp->locals+fp->num_locals > sp)
	fatal("Stack error (stupid!).\n");

      if(recoveries && sp-evaluator_stack < recoveries->sp)
	fatal("Stack error (underflow)\n");
      
      if(d_flag > 9) do_debug();

      backlogp++;
      if(backlogp >= BACKLOG) backlogp=0;

      if(backlog[backlogp].program)
	free_program(backlog[backlogp].program);

      backlog[backlogp].program=fp->context.prog;
      fp->context.prog->refs++;
      backlog[backlogp].instruction=instr;
      backlog[backlogp].arg=0;
      backlog[backlogp].pc=pc;
    }

    if(t_flag > 2)
    {
      char *file, *f;
      INT32 linep, nonblock;
      if((nonblock=query_nonblocking(2)))
	set_nonblocking(2,0);


      file=get_line(pc-1,fp->context.prog,&linep);
      while((f=STRCHR(file,'/'))) file=f+1;
      fprintf(stderr,"- %s:%4ld:(%lx): %-25s %4ld %4ld\n",
	      file,(long)linep,
	      (long)(pc-fp->context.prog->program-1),
	      get_f_name(instr + F_OFFSET),
	      (long)(sp-evaluator_stack),
	      (long)(mark_sp-mark_stack));
      if(nonblock)
	set_nonblocking(2,1);
    }

    if(instr + F_OFFSET < F_MAX_OPCODE) 
      ADD_RUNNED(instr + F_OFFSET);
#endif

    switch(instr)
    {
      /* Support to allow large arguments */
      CASE(F_PREFIX_256); prefix+=256; break;
      CASE(F_PREFIX_512); prefix+=512; break;
      CASE(F_PREFIX_768); prefix+=768; break;
      CASE(F_PREFIX_1024); prefix+=1024; break;
      CASE(F_PREFIX_24BITX256);
      prefix+=EXTRACT_UCHAR(pc++)<<24;
      CASE(F_PREFIX_WORDX256);
      prefix+=EXTRACT_UCHAR(pc++)<<16;
      CASE(F_PREFIX_CHARX256);
      prefix+=EXTRACT_UCHAR(pc++)<<8;
      break;
      /* Push number */
      CASE(F_CONST0); push_int(0); break;
      CASE(F_CONST1); push_int(1); break;
      CASE(F_CONST_1); push_int(-1); break;
      CASE(F_BIGNUM); push_int(0x7fffffff); break;
      CASE(F_NUMBER); push_int(GET_ARG()); break;
      CASE(F_NEG_NUMBER); push_int(-GET_ARG()); break;

      /* The rest of the basic 'push value' instructions */	
      CASE(F_STRING);
      copy_shared_string(sp->u.string,fp->context.prog->strings[GET_ARG()]);
      sp->type=T_STRING;
      sp->subtype=0;
      sp++;
      print_return_value();
      break;

      CASE(F_ARROW_STRING);
      copy_shared_string(sp->u.string,fp->context.prog->strings[GET_ARG()]);
      sp->type=T_STRING;
      sp->subtype=1; /* Magic */
      sp++;
      print_return_value();
      break;

      CASE(F_CONSTANT);
      assign_svalue_no_free(sp++,fp->context.prog->constants+GET_ARG());
      print_return_value();
      break;

      CASE(F_FLOAT);
      sp->type=T_FLOAT;
      MEMCPY((void *)&sp->u.float_number, pc, sizeof(FLOAT_TYPE));
      pc+=sizeof(FLOAT_TYPE);
      sp++;
      break;

      CASE(F_LFUN);
      sp->u.object=fp->current_object;
      fp->current_object->refs++;
      sp->subtype=GET_ARG()+fp->context.identifier_level;
      sp->type=T_FUNCTION;
      sp++;
      break;

      /* The not so basic 'push value' instructions */
      CASE(F_GLOBAL);
      low_object_index_no_free(sp,
			       fp->current_object,
			       GET_ARG() + fp->context.identifier_level);
      sp++;
      print_return_value();
      break;

      CASE(F_MARK_AND_LOCAL); *(mark_sp++)=sp;
      CASE(F_LOCAL);
      assign_svalue_no_free(sp++,fp->locals+GET_ARG());
      print_return_value();
      break;

      CASE(F_2_LOCALS);
      assign_svalue_no_free(sp++,fp->locals+GET_ARG());
      print_return_value();
      assign_svalue_no_free(sp++,fp->locals+GET_ARG());
      print_return_value();
      break;
      

      CASE(F_LOCAL_LVALUE);
      sp[0].type=T_LVALUE;
      sp[0].u.lval=fp->locals+GET_ARG();
      sp[1].type=T_VOID;
      sp+=2;
      break;

      CASE(F_CLEAR_2_LOCAL);
      instr=GET_ARG();
      free_svalues(fp->locals + instr, 2, -1);
      fp->locals[instr].type=T_INT;
      fp->locals[instr].subtype=0;
      fp->locals[instr].u.integer=0;
      fp->locals[instr+1].type=T_INT;
      fp->locals[instr+1].subtype=0;
      fp->locals[instr+1].u.integer=0;
      break;

      CASE(F_CLEAR_LOCAL);
      instr=GET_ARG();
      free_svalue(fp->locals + instr);
      fp->locals[instr].type=T_INT;
      fp->locals[instr].subtype=0;
      fp->locals[instr].u.integer=0;
      break;

      CASE(F_INC_LOCAL);
      instr=GET_ARG();
      if(fp->locals[instr].type == T_INT)
      {
        fp->locals[instr].u.integer++;
	assign_svalue_no_free(sp++,fp->locals+instr);
      }else{
	assign_svalue_no_free(sp++,fp->locals+instr);
	push_int(1);
	f_add(2);
	assign_svalue(fp->locals+instr,sp-1);
      }
      break;

      CASE(F_POST_INC_LOCAL);
      instr=GET_ARG();
      assign_svalue_no_free(sp++,fp->locals+instr);
      goto inc_local_and_pop;

      CASE(F_INC_LOCAL_AND_POP);
      instr=GET_ARG();
    inc_local_and_pop:
      if(fp->locals[instr].type == T_INT)
      {
	fp->locals[instr].u.integer++;
      }else{
	assign_svalue_no_free(sp++,fp->locals+instr);
	push_int(1);
	f_add(2);
	assign_svalue(fp->locals+instr,sp-1);
	pop_stack();
      }
      break;

      CASE(F_DEC_LOCAL);
      instr=GET_ARG();
      if(fp->locals[instr].type == T_INT)
      {
	fp->locals[instr].u.integer--;
	assign_svalue_no_free(sp++,fp->locals+instr);
      }else{
	assign_svalue_no_free(sp++,fp->locals+instr);
	push_int(1);
	o_subtract();
	assign_svalue(fp->locals+instr,sp-1);
      }
      break;

      CASE(F_POST_DEC_LOCAL);
      instr=GET_ARG();
      assign_svalue_no_free(sp++,fp->locals+instr);
      goto dec_local_and_pop;
      fp->locals[instr].u.integer--;
      break;

      CASE(F_DEC_LOCAL_AND_POP);
      instr=GET_ARG();
    dec_local_and_pop:
      if(fp->locals[instr].type == T_INT)
      {
	fp->locals[instr].u.integer--;
      }else{
	assign_svalue_no_free(sp++,fp->locals+instr);
	push_int(1);
	o_subtract();
	assign_svalue(fp->locals+instr,sp-1);
	pop_stack();
      }
      break;

      CASE(F_LTOSVAL);
      lvalue_to_svalue_no_free(sp,sp-2);
      sp++;
      break;

      CASE(F_LTOSVAL2);
      sp[0]=sp[-1];
      lvalue_to_svalue_no_free(sp-1,sp-3);

      /* this is so that foo+=bar (and similar things) will be faster, this
       * is done by freeing the old reference to foo after it has been pushed
       * on the stack. That way foo can have only 1 reference if we are lucky,
       * and then the low array/multiset/mapping manipulation routines can be
       * destructive if they like
       */
      if( (1 << sp[-1].type) & ( BIT_ARRAY | BIT_MULTISET | BIT_MAPPING | BIT_STRING ))
      {
	struct svalue s;
	s.type=T_INT;
	s.subtype=0;
	s.u.integer=0;
	assign_lvalue(sp-3,&s);
      }
      sp++;
      break;

      CASE(F_GLOBAL_LVALUE)
      {
	struct identifier *i;
	INT32 tmp=GET_ARG() + fp->context.identifier_level;

	if(!fp->current_object->prog)
	  error("Cannot access global variables in destructed object.\n");

	i=ID_FROM_INT(fp->current_object->prog, tmp);

	if(!IDENTIFIER_IS_VARIABLE(i->identifier_flags))
	  error("Cannot re-assign functions or constants.\n");

	if(i->run_time_type == T_MIXED)
	{
	  sp[0].type=T_LVALUE;
	  sp[0].u.lval=(struct svalue *)GLOBAL_FROM_INT(tmp);
	}else{
	  sp[0].type=T_SHORT_LVALUE;
	  sp[0].u.short_lval= (union anything *)GLOBAL_FROM_INT(tmp);
	  sp[0].subtype=i->run_time_type;
	}
	sp[1].type=T_VOID;
	sp+=2;
	break;
      }
      
      CASE(F_INC);
      {
	union anything *u=get_pointer_if_this_type(sp-2, T_INT);
	if(u)
	{
	  instr=++ u->integer;
	  pop_n_elems(2);
	  push_int(u->integer);
	}else{
	  lvalue_to_svalue_no_free(sp, sp-2); sp++;
	  push_int(1);
	  f_add(2);
	  assign_lvalue(sp-3, sp-1);
	  assign_svalue(sp-3, sp-1);
	  pop_n_elems(2);
	}
	break;
      }

      CASE(F_DEC);
      {
	union anything *u=get_pointer_if_this_type(sp-2, T_INT);
	if(u)
	{
	  instr=-- u->integer;
	  pop_n_elems(2);
	  push_int(u->integer);
	}else{
	  lvalue_to_svalue_no_free(sp, sp-2); sp++;
	  push_int(1);
	  o_subtract();
	  assign_lvalue(sp-3, sp-1);
	  assign_svalue(sp-3, sp-1);
	  pop_n_elems(2);
	}
	break;
      }

      CASE(F_DEC_AND_POP);
      {
	union anything *u=get_pointer_if_this_type(sp-2, T_INT);
	if(u)
	{
	  instr=-- u->integer;
	  pop_n_elems(2);
	}else{
	  lvalue_to_svalue_no_free(sp, sp-2); sp++;
	  push_int(1);
	  o_subtract();
	  assign_lvalue(sp-3, sp-1);
	  pop_n_elems(3);
	}
	break;
      }

      CASE(F_INC_AND_POP);
      {
	union anything *u=get_pointer_if_this_type(sp-2, T_INT);
	if(u)
	{
	  instr=++ u->integer;
	  pop_n_elems(2);
	}else{
	  lvalue_to_svalue_no_free(sp, sp-2); sp++;
	  push_int(1);
	  f_add(2);
	  assign_lvalue(sp-3, sp-1);
	  pop_n_elems(3);
	}
	break;
      }

      CASE(F_POST_INC);
      {
	union anything *u=get_pointer_if_this_type(sp-2, T_INT);
	if(u)
	{
	  instr=u->integer ++;
	  pop_n_elems(2);
	  push_int(instr);
	}else{
	  lvalue_to_svalue_no_free(sp, sp-2); sp++;
	  assign_svalue(sp,sp-1); sp++;
	  push_int(1);
	  f_add(2);
	  assign_lvalue(sp-4, sp-1);
	  assign_svalue(sp-4, sp-2);
	  pop_n_elems(3);
	}
	break;
      }

      CASE(F_POST_DEC);
      {
	union anything *u=get_pointer_if_this_type(sp-2, T_INT);
	if(u)
	{
	  instr=u->integer --;
	  pop_n_elems(2);
	  push_int(instr);
	}else{
	  lvalue_to_svalue_no_free(sp, sp-2); sp++;
	  assign_svalue(sp,sp-1); sp++;
	  push_int(1);
	  o_subtract();
	  assign_lvalue(sp-4, sp-1);
	  assign_svalue(sp-4, sp-2);
	  pop_n_elems(3);
	}
	break;
      }

      CASE(F_ASSIGN);
      assign_lvalue(sp-3,sp-1);
      free_svalue(sp-3);
      free_svalue(sp-2);
      sp[-3]=sp[-1];
      sp-=2;
      break;

      CASE(F_ASSIGN_AND_POP);
      assign_lvalue(sp-3,sp-1);
      pop_n_elems(3);
      break;

      CASE(F_ASSIGN_LOCAL);
      assign_svalue(fp->locals+GET_ARG(),sp-1);
      break;

      CASE(F_ASSIGN_LOCAL_AND_POP);
      instr=GET_ARG();
      free_svalue(fp->locals+instr);
      fp->locals[instr]=sp[-1];
      sp--;
      break;

      CASE(F_ASSIGN_GLOBAL)
      {
	struct identifier *i;
	INT32 tmp=GET_ARG() + fp->context.identifier_level;
	if(!fp->current_object->prog)
	  error("Cannot access global variables in destructed object.\n");

	i=ID_FROM_INT(fp->current_object->prog, tmp);
	if(!IDENTIFIER_IS_VARIABLE(i->identifier_flags))
	  error("Cannot assign functions or constants.\n");
	if(i->run_time_type == T_MIXED)
	{
	  assign_svalue((struct svalue *)GLOBAL_FROM_INT(tmp), sp-1);
	}else{
	  assign_to_short_svalue((union anything *)GLOBAL_FROM_INT(tmp),
				 i->run_time_type,
				 sp-1);
	}
      }
      break;

      CASE(F_ASSIGN_GLOBAL_AND_POP)
      {
	struct identifier *i;
	INT32 tmp=GET_ARG() + fp->context.identifier_level;
	if(!fp->current_object->prog)
	  error("Cannot access global variables in destructed object.\n");

	i=ID_FROM_INT(fp->current_object->prog, tmp);
	if(!IDENTIFIER_IS_VARIABLE(i->identifier_flags))
	  error("Cannot assign functions or constants.\n");

	if(i->run_time_type == T_MIXED)
	{
	  struct svalue *s=(struct svalue *)GLOBAL_FROM_INT(tmp);
	  free_svalue(s);
	  sp--;
	  *s=*sp;
	}else{
	  assign_to_short_svalue((union anything *)GLOBAL_FROM_INT(tmp),
				 i->run_time_type,
				 sp-1);
	  pop_stack();
	}
      }
      break;

      /* Stack machine stuff */
      CASE(F_POP_VALUE); pop_stack(); break;
      CASE(F_POP_N_ELEMS); pop_n_elems(GET_ARG()); break;
      CASE(F_MARK2); *(mark_sp++)=sp;
      CASE(F_MARK); *(mark_sp++)=sp; break;

      CASE(F_CLEAR_STRING_SUBTYPE);
      if(sp[-1].type==T_STRING) sp[-1].subtype=0;
      break;

      /* Jumps */
      CASE(F_BRANCH);
      DOJUMP();
      break;

      CASE(F_BRANCH_WHEN_ZERO);
      if(!IS_ZERO(sp-1))
      {
	pc+=sizeof(INT32);
      }else{
	DOJUMP();
      }
      pop_stack();
      break;
      
      CASE(F_BRANCH_WHEN_NON_ZERO);
      if(IS_ZERO(sp-1))
      {
	pc+=sizeof(INT32);
      }else{
	DOJUMP();
      }
      pop_stack();
      break;

      CJUMP(F_BRANCH_WHEN_EQ, is_eq);
      CJUMP(F_BRANCH_WHEN_NE,!is_eq);
      CJUMP(F_BRANCH_WHEN_LT, is_lt);
      CJUMP(F_BRANCH_WHEN_LE,!is_gt);
      CJUMP(F_BRANCH_WHEN_GT, is_gt);
      CJUMP(F_BRANCH_WHEN_GE,!is_lt);

      CASE(F_LAND);
      if(!IS_ZERO(sp-1))
      {
	pc+=sizeof(INT32);
	pop_stack();
      }else{
	DOJUMP();
      }
      break;

      CASE(F_LOR);
      if(IS_ZERO(sp-1))
      {
	pc+=sizeof(INT32);
	pop_stack();
      }else{
	DOJUMP();
      }
      break;

      CASE(F_EQ_OR);
      if(!is_eq(sp-2,sp-1))
      {
	pop_n_elems(2);
	pc+=sizeof(INT32);
      }else{
	pop_n_elems(2);
	push_int(1);
	DOJUMP();
      }
      break;

      CASE(F_EQ_AND);
      if(is_eq(sp-2,sp-1))
      {
	pop_n_elems(2);
	pc+=sizeof(INT32);
      }else{
	pop_n_elems(2);
	push_int(0);
	DOJUMP();
      }
      break;

      CASE(F_CATCH);
      if(o_catch(pc+sizeof(INT32)))
	return; /* There was a return inside the evaluated code */
      else
	pc+=EXTRACT_INT(pc);
      break;

      CASE(F_THROW_ZERO);
      push_int(0);
      f_throw(1);
      break;

      CASE(F_SWITCH)
      {
	INT32 tmp;
	tmp=switch_lookup(fp->context.prog->
			  constants[GET_ARG()].u.array,sp-1);
	pc=(unsigned char *)DO_ALIGN(pc,sizeof(INT32));
	pc+=(tmp>=0 ? 1+tmp*2 : 2*~tmp) * sizeof(INT32);
	if(*(INT32*)pc < 0) fast_check_threads_etc(7);
	pc+=*(INT32*)pc;
	pop_stack();
	break;
      }
      
      LOOP(F_INC_LOOP, ++, <, f_add(2), is_lt);
      LOOP(F_DEC_LOOP, --, >, o_subtract(), is_gt);
      LOOP(F_INC_NEQ_LOOP, ++, !=, f_add(2), !is_eq);
      LOOP(F_DEC_NEQ_LOOP, --, !=, o_subtract(), !is_eq);

      CASE(F_FOREACH) /* array, lvalue , i */
      {
	if(sp[-4].type != T_ARRAY) error("Bad argument 1 to foreach()\n");
	if(sp[-1].u.integer < sp[-4].u.array->size)
	{
	  fast_check_threads_etc(10);
	  index_no_free(sp,sp-4,sp-1);
	  sp++;
	  assign_lvalue(sp-4, sp-1);
	  free_svalue(sp-1);
	  sp--;
	  pc+=EXTRACT_INT(pc);
	  sp[-1].u.integer++;
	}else{
	  pc+=sizeof(INT32);
	}
	break;
      }

      CASE(F_RETURN_1);
      pop_n_elems(sp-fp->locals);
      push_int(1);
      goto do_return;

      CASE(F_RETURN_0);
      pop_n_elems(sp-fp->locals);
      push_int(0);
      goto do_return;

      CASE(F_RETURN);
      if(fp->locals != sp-1)
      {
	assign_svalue(fp->locals, sp-1);
	pop_n_elems(sp - fp->locals - 1);
      }

    do_return:
#if defined(DEBUG) && defined(GC2)
      if(d_flag > 2)
	do_gc();
#endif
      check_threads_etc();

      /* fall through */

      CASE(F_DUMB_RETURN);
      return;

      CASE(F_NEGATE); 
      if(sp[-1].type == T_INT)
      {
	sp[-1].u.integer =- sp[-1].u.integer;
      }else if(sp[-1].type == T_FLOAT)
      {
	sp[-1].u.float_number =- sp[-1].u.float_number;
      }else{
	o_negate();
      }
      break;

      CASE(F_COMPL); o_compl(); break;

      CASE(F_NOT);
      switch(sp[-1].type)
      {
      case T_INT:
	sp[-1].u.integer =! sp[-1].u.integer;
	break;

      case T_FUNCTION:
      case T_OBJECT:
	if(IS_ZERO(sp-1))
	{
	  pop_stack();
	  push_int(1);
	}else{
	  pop_stack();
	  push_int(0);
	}
	break;

      default:
	free_svalue(sp-1);
	sp[-1].type=T_INT;
	sp[-1].u.integer=0;
      }
      break;

      CASE(F_LSH); o_lsh(); break;
      CASE(F_RSH); o_rsh(); break;

      COMPARISMENT(F_EQ, is_eq(sp-2,sp-1));
      COMPARISMENT(F_NE,!is_eq(sp-2,sp-1));
      COMPARISMENT(F_GT, is_gt(sp-2,sp-1));
      COMPARISMENT(F_GE,!is_lt(sp-2,sp-1));
      COMPARISMENT(F_LT, is_lt(sp-2,sp-1));
      COMPARISMENT(F_LE,!is_gt(sp-2,sp-1));

      CASE(F_ADD);      f_add(2);     break;
      CASE(F_SUBTRACT); o_subtract(); break;
      CASE(F_AND);      o_and();      break;
      CASE(F_OR);       o_or();       break;
      CASE(F_XOR);      o_xor();      break;
      CASE(F_MULTIPLY); o_multiply(); break;
      CASE(F_DIVIDE);   o_divide();   break;
      CASE(F_MOD);      o_mod();      break;

      CASE(F_ADD_INT); push_int(GET_ARG()); f_add(2); break;
      CASE(F_ADD_NEG_INT); push_int(-GET_ARG()); f_add(2); break;

      CASE(F_PUSH_ARRAY);
      if(sp[-1].type!=T_ARRAY) error("Bad argument to @\n");
      sp--;
      push_array_items(sp->u.array);
      break;

      CASE(F_LOCAL_INDEX);
      assign_svalue_no_free(sp++,fp->locals+GET_ARG());
      if(sp[-1].type == T_STRING)
	sp[-1].subtype=0;
      print_return_value();
      goto do_index;

      CASE(F_POS_INT_INDEX);
      push_int(GET_ARG());
      print_return_value();
      goto do_index;

      CASE(F_NEG_INT_INDEX);
      push_int(-GET_ARG());
      print_return_value();
      goto do_index;

      CASE(F_ARROW);
      copy_shared_string(sp->u.string,fp->context.prog->strings[GET_ARG()]);
      sp->type=T_STRING;
      sp->subtype=1;
      sp++;
      print_return_value();
      goto do_index;

      CASE(F_STRING_INDEX);
      copy_shared_string(sp->u.string,fp->context.prog->strings[GET_ARG()]);
      sp->type=T_STRING;
      sp->subtype=0;
      sp++;
      print_return_value();
      /* Fall through */

      CASE(F_INDEX);
    do_index:
      o_index();
      print_return_value();
      break;

      CASE(F_CAST); f_cast(); break;

      CASE(F_RANGE); o_range(); break;
      CASE(F_COPY_VALUE);
      {
	struct svalue tmp;
	copy_svalues_recursively_no_free(&tmp,sp-1,1,0);
	free_svalue(sp-1);
	sp[-1]=tmp;
      }
      break;

      CASE(F_SIZEOF);
      instr=pike_sizeof(sp-1);
      pop_stack();
      push_int(instr);
      break;

      CASE(F_SIZEOF_LOCAL);
      push_int(pike_sizeof(fp->locals+GET_ARG()));
      break;

      CASE(F_SSCANF); f_sscanf(GET_ARG()); break;

      CASE(F_CALL_LFUN);
      apply_low(fp->current_object,
		GET_ARG()+fp->context.identifier_level,
		sp - *--mark_sp);
      break;

      CASE(F_CALL_LFUN_AND_POP);
      apply_low(fp->current_object,
		GET_ARG()+fp->context.identifier_level,
		sp - *--mark_sp);
      pop_stack();
      break;

    CASE(F_MARK_APPLY);
      strict_apply_svalue(fp->context.prog->constants + GET_ARG(), 0);
      break;

    CASE(F_MARK_APPLY_POP);
      strict_apply_svalue(fp->context.prog->constants + GET_ARG(), 0);
      pop_stack();
      break;

    CASE(F_APPLY);
      strict_apply_svalue(fp->context.prog->constants + GET_ARG(), sp - *--mark_sp );
      break;

    CASE(F_APPLY_AND_POP);
      strict_apply_svalue(fp->context.prog->constants + GET_ARG(), sp - *--mark_sp );
      pop_stack();
      break;

    default:
      fatal("Strange instruction %ld\n",(long)instr);
    }
  }
}

/* Put catch outside of eval_instruction, so
 * the setjmp won't affect the optimization of
 * eval_instruction
 */
static int o_catch(unsigned char *pc)
{
  JMP_BUF tmp;
  if(SETJMP(tmp))
  {
    *sp=throw_value;
    throw_value.type=T_INT;
    sp++;
    UNSETJMP(tmp);
    return 0;
  }else{
    eval_instruction(pc);
    UNSETJMP(tmp);
    return 1;
  }
}


int apply_low_safe_and_stupid(struct object *o, INT32 offset)
{
  JMP_BUF tmp;
  struct frame new_frame;
  int ret;

  new_frame.parent_frame = fp;
  new_frame.current_object = o;
  new_frame.context=o->prog->inherits[0];
  new_frame.locals = evaluator_stack;
  new_frame.args = 0;
  new_frame.num_args=0;
  new_frame.num_locals=0;
  new_frame.fun = -1;
  new_frame.pc = 0;
  new_frame.current_storage=o->storage;
  fp = & new_frame;

  new_frame.current_object->refs++;
  new_frame.context.prog->refs++;

  if(SETJMP(tmp))
  {
    ret=1;
  }else{
    eval_instruction(o->prog->program + offset);
#ifdef DEBUG
    if(sp<evaluator_stack)
      fatal("Stack error (simple).\n");
#endif
    ret=0;
  }
  UNSETJMP(tmp);

  free_object(new_frame.current_object);
  free_program(new_frame.context.prog);

  fp = new_frame.parent_frame;
  return ret;
}

void apply_low(struct object *o, int fun, int args)
{
  struct program *p;
  struct reference *ref;
  struct frame new_frame;
  struct identifier *function;

  if(fun<0)
  {
    pop_n_elems(args);
    push_int(0);
    return;
  }

  o->refs++;

  fast_check_threads_etc(4);
  check_stack(256);
  check_mark_stack(256);

#ifdef DEBUG
  if(d_flag>2) do_debug();
#endif

  p=o->prog;
  if(!p)
    error("Cannot call functions in destructed objects.\n");
#ifdef DEBUG
  if(fun>=(int)p->num_identifier_references)
    fatal("Function index out of range.\n");
#endif

  ref = p->identifier_references + fun;
#ifdef DEBUG
  if(ref->inherit_offset>=p->num_inherits)
    fatal("Inherit offset out of range in program.\n");
#endif

  /* init a new evaluation frame */
  new_frame.parent_frame = fp;
  new_frame.current_object = o;
  new_frame.context = p->inherits[ ref->inherit_offset ];
  function = new_frame.context.prog->identifiers + ref->identifier_offset;
  
  new_frame.locals = sp - args;
  new_frame.args = args;
  new_frame.fun = fun;
  new_frame.current_storage = o->storage+new_frame.context.storage_offset;
  new_frame.pc = 0;

/* new_frame.current_object->refs++;  Moved to beginning of function / Hubbe */
  new_frame.context.prog->refs++;

#ifdef DEBUG
  if(t_flag)
  {
    char *file, *f;
    INT32 linep,e,nonblock;
    char buf[50],*s;

    if((nonblock=query_nonblocking(2)))
      set_nonblocking(2,0);

    if(fp && fp->pc)
    {
      file=get_line(fp->pc,fp->context.prog,&linep);
      while((f=STRCHR(file,'/'))) file=f+1;
    }else{
      linep=0;
      file="-";
    }

    init_buf();
    sprintf(buf,"%lx->",(long)o);
    my_strcat(buf);
    my_strcat(function->name->str);
    my_strcat("(");
    for(e=0;e<args;e++)
    {
      if(e) my_strcat(",");
      describe_svalue(sp-args+e,0,0);
    }
    my_strcat(")"); 
    s=simple_free_buf();
    if((long)strlen(s) > (long)TRACE_LEN)
    {
      s[TRACE_LEN]=0;
      s[TRACE_LEN-1]='.';
      s[TRACE_LEN-2]='.';
      s[TRACE_LEN-2]='.';
    }
    fprintf(stderr,"- %s:%4ld: %s\n",file,(long)linep,s);
    free(s);

    if(nonblock)
      set_nonblocking(2,1);
  }
#endif

  fp = &new_frame;

#ifdef PROFILING
  function->num_calls++;
#endif /* PROFILING */

  if(function->func.offset == -1)
    error("Calling undefined function '%s'.\n",function->name->str);

  if(function->identifier_flags & IDENTIFIER_C_FUNCTION)
  {
    fp->num_args=args;
    new_frame.num_locals=args;
    (*function->func.c_fun)(args);
  }else{
    int num_args;
    int num_locals;
    unsigned char *pc;
    pc=new_frame.context.prog->program + function->func.offset;

    num_locals=EXTRACT_UCHAR(pc++);
    num_args=EXTRACT_UCHAR(pc++);

    /* adjust arguments on stack */
    if(args < num_args) /* push zeros */
    {
      clear_svalues(sp, num_args-args);
      sp += num_args-args;
      args += num_args-args;
    }

    if(function->identifier_flags & IDENTIFIER_VARARGS)
    {
      f_aggregate(args - num_args); /* make array */
      args = num_args+1;
    }else{
      if(args > num_args)
      {
	/* pop excessive */
	pop_n_elems(args - num_args);
	args=num_args;
      }
    }

    clear_svalues(sp, num_locals - args);
    sp += num_locals - args;
#ifdef DEBUG
    if(num_locals < num_args)
      fatal("Wrong number of arguments or locals in function def.\n");
#endif
    new_frame.num_locals=num_locals;
    new_frame.num_args=num_args;
    eval_instruction(pc);
#ifdef DEBUG
    if(sp<evaluator_stack)
      fatal("Stack error (also simple).\n");
#endif
  }

  if(sp - new_frame.locals > 1)
  {
    pop_n_elems(sp - new_frame.locals -1);
  }else if(sp - new_frame.locals < 1){
#ifdef DEBUG
    if(sp - new_frame.locals<0) fatal("Frame underflow.\n");
#endif
    sp->u.integer = 0;
    sp->subtype=NUMBER_NUMBER;
    sp->type = T_INT;
    sp++;
  }

  free_object(new_frame.current_object);
  free_program(new_frame.context.prog);

  fp = new_frame.parent_frame;

#ifdef DEBUG
  if(t_flag)
  {
    char *s;
    int nonblock;

    if((nonblock=query_nonblocking(2)))
      set_nonblocking(2,0);

    init_buf();
    my_strcat("Return: ");
    describe_svalue(sp-1,0,0);
    s=simple_free_buf();
    if((long)strlen(s) > (long)TRACE_LEN)
    {
      s[TRACE_LEN]=0;
      s[TRACE_LEN-1]='.';
      s[TRACE_LEN-2]='.';
      s[TRACE_LEN-2]='.';
    }
    fprintf(stderr,"%-*s%s\n",4,"-",s);
    free(s);

    if(nonblock)
      set_nonblocking(2,1);
  }
#endif
}

void safe_apply_low(struct object *o,int fun,int args)
{
  JMP_BUF recovery;

  sp-=args;
  free_svalue(& throw_value);
  throw_value.type=T_INT;
  if(SETJMP(recovery))
  {
    if(throw_value.type == T_ARRAY)
    {
      ONERROR tmp;
      SET_ONERROR(tmp,exit_on_error,"Error in handle_error in master object!");
      assign_svalue_no_free(sp++, & throw_value);
      APPLY_MASTER("handle_error", 1);
      pop_stack();
      UNSET_ONERROR(tmp);
    }
      
    sp->u.integer = 0;
    sp->subtype=NUMBER_NUMBER;
    sp->type = T_INT;
    sp++;
  }else{
    INT32 expected_stack = sp - evaluator_stack + 1;
    sp+=args;
    apply_low(o,fun,args);
    if(sp - evaluator_stack > expected_stack)
      pop_n_elems(sp - evaluator_stack - expected_stack);
    if(sp - evaluator_stack < expected_stack)
    {
      sp->u.integer = 0;
      sp->subtype=NUMBER_NUMBER;
      sp->type = T_INT;
      sp++;
    }
  }
  UNSETJMP(recovery);

}

void safe_apply(struct object *o, char *fun ,INT32 args)
{
#ifdef DEBUG
  if(!o->prog) fatal("Apply safe on destructed object.\n");
#endif
  safe_apply_low(o, find_identifier(fun, o->prog), args);
}

void apply_lfun(struct object *o, int fun, int args)
{
#ifdef DEBUG
  if(fun < 0 || fun >= NUM_LFUNS)
    fatal("Apply lfun on illegal value!\n");
#endif
  if(!o->prog)
    error("Apply on destructed object.\n");

  apply_low(o, o->prog->lfuns[fun], args);
}

void apply_shared(struct object *o,
		  struct pike_string *fun,
		  int args)
{
  apply_low(o, find_shared_string_identifier(fun, o->prog), args);
}

void apply(struct object *o, char *fun, int args)
{
  apply_low(o, find_identifier(fun, o->prog), args);
}

void strict_apply_svalue(struct svalue *s, INT32 args)
{
#ifdef DEBUG
  struct svalue *save_sp;
  save_sp=sp-args;
  if(t_flag>1)
  {
    char *file, *f;
    INT32 linep,e,nonblock;
    char *st;

    if((nonblock=query_nonblocking(2)))
      set_nonblocking(2,0);

    if(fp && fp->pc)
    {
      file=get_line(fp->pc,fp->context.prog,&linep);
      while((f=STRCHR(file,'/'))) file=f+1;
    }else{
      linep=0;
      file="-";
    }

    init_buf();
    describe_svalue(s,0,0);
    my_strcat("(");
    for(e=0;e<args;e++)
    {
      if(e) my_strcat(",");
      describe_svalue(sp-args+e,0,0);
    }
    my_strcat(")"); 
    st=simple_free_buf();
    if((long)strlen(st) > (long)TRACE_LEN)
    {
      st[TRACE_LEN]=0;
      st[TRACE_LEN-1]='.';
      st[TRACE_LEN-2]='.';
      st[TRACE_LEN-2]='.';
    }
    fprintf(stderr,"- %s:%4ld: %s\n",file,(long)linep,st);
    free(st);

    if(nonblock)
      set_nonblocking(2,1);
  }
#endif

  switch(s->type)
  {
  case T_FUNCTION:
    if(s->subtype == FUNCTION_BUILTIN)
    {
      (*(s->u.efun->function))(args);
    }else{
      apply_low(s->u.object, s->subtype, args);
    }
    break;

  case T_ARRAY:
    apply_array(s->u.array,args);
    break;

  case T_PROGRAM:
    {
      struct object *o=clone_object(s->u.program,args);
      push_object(o);
    }
    break;

  case T_OBJECT:
    if(!s->u.object->prog)
      error("Calling a destructed object.\n");
    
    if(s->u.object->prog->lfuns[LFUN_CALL] == -1)
      error("Calling object without `() operator\n");

    apply_lfun(s->u.object, LFUN_CALL, args);
    break;

  case T_INT:
    if (!s->u.integer) {
      error("Attempt to call the NULL-value\n");
    } else {
      error("Attempt to call the value %d\n", s->u.integer);
    }
  case T_STRING:
    if (s->u.string->len > 20) {
      error("Attempt to call the string \"%20s\"...\n", s->u.string->str);
    } else {
      error("Attempt to call the string \"%s\"\n", s->u.string->str);
    }
  case T_MAPPING:
    error("Attempt to call a mapping\n");
  case T_MULTISET:
    error("Attempt to call a multiset\n");
  default:
    error("Call to non-function value type:%d.\n", s->type);
  }

#ifdef DEBUG
  if(t_flag>1 && sp>save_sp)
  {
    char *s;
    int nonblock;
    if((nonblock=query_nonblocking(2)))
      set_nonblocking(2,0);

    init_buf();
    my_strcat("Return: ");
    describe_svalue(sp-1,0,0);
    s=simple_free_buf();
    if((long)strlen(s) > (long)TRACE_LEN)
    {
      s[TRACE_LEN]=0;
      s[TRACE_LEN-1]='.';
      s[TRACE_LEN-2]='.';
      s[TRACE_LEN-2]='.';
    }
    fprintf(stderr,"%-*s%s\n",4,"-",s);
    free(s);

    if(nonblock)
      set_nonblocking(2,1);
  }
#endif
}

void apply_svalue(struct svalue *s, INT32 args)
{
  if(s->type==T_INT)
  {
    pop_n_elems(args);
    push_int(0);
  }else{
    INT32 expected_stack=sp-args+1 - evaluator_stack;

    strict_apply_svalue(s,args);
    if(sp > (expected_stack + evaluator_stack))
    {
      pop_n_elems(sp-(expected_stack + evaluator_stack));
    }
    else if(sp < (expected_stack + evaluator_stack))
    {
      push_int(0);
    }
#ifdef DEBUG
    if(sp < (expected_stack + evaluator_stack))
      fatal("Stack underflow!\n");
#endif
  }
}

#ifdef DEBUG
void slow_check_stack(void)
{
  struct svalue *s,**m;
  struct frame *f;

  debug_check_stack();

  if(sp > &(evaluator_stack[stack_size]))
    fatal("Stack overflow\n");

  if(mark_sp > &(mark_stack[stack_size]))
    fatal("Mark stack overflow.\n");

  if(mark_sp < mark_stack)
    fatal("Mark stack underflow.\n");

  for(s=evaluator_stack;s<sp;s++) check_svalue(s);

  s=evaluator_stack;
  for(m=mark_stack;m<mark_sp;m++)
  {
    if(*m < s)
      fatal("Mark stack failiure.\n");

    s=*m;
  }

  if(s > &(evaluator_stack[stack_size]))
    fatal("Mark stack exceeds svalue stack\n");

  for(f=fp;f;f=f->parent_frame)
  {
    if(f->locals)
    {
      if(f->locals < evaluator_stack ||
	f->locals > &(evaluator_stack[stack_size]))
      fatal("Local variable pointer points to Finspång.\n");

      if(f->args < 0 || f->args > stack_size)
	fatal("FEL FEL FEL! HELP!! (corrupted frame)\n");
    }
  }
}
#endif

void cleanup_interpret(void)
{
#ifdef DEBUG
  int e;
#endif

  while(fp)
  {
    free_object(fp->current_object);
    free_program(fp->context.prog);
    
    fp = fp->parent_frame;
  }

#ifdef DEBUG
  for(e=0;e<BACKLOG;e++)
  {
    if(backlog[e].program)
    {
      free_program(backlog[e].program);
      backlog[e].program=0;
    }
  }
#endif
  reset_evaluator();

#ifdef USE_MMAP_FOR_STACK
  if(!evaluator_stack_malloced)
  {
    munmap((char *)evaluator_stack, stack_size*sizeof(struct svalue));
    evaluator_stack=0;
  }
  if(!mark_stack_malloced)
  {
    munmap((char *)mark_stack, stack_size*sizeof(struct svalue *));
    mark_stack=0;
  }
#endif

  if(evaluator_stack) free((char *)evaluator_stack);
  if(mark_stack) free((char *)mark_stack);

  mark_stack=0;
  evaluator_stack=0;
  mark_stack_malloced=0;
  evaluator_stack_malloced=0;
}
