/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
/**/
#include "global.h"
RCSID("$Id: interpret.c,v 1.190 2003/03/23 12:35:29 jonasw Exp $");
#include "interpret.h"
#include "object.h"
#include "program.h"
#include "svalue.h"
#include "array.h"
#include "mapping.h"
#include "pike_error.h"
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
#include "security.h"
#include "block_alloc.h"
#include "bignum.h"
#include "pike_types.h"

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

/* Keep some margin on the stack space checks. They're lifted when
 * handle_error runs to give it some room. */
#define SVALUE_STACK_MARGIN 100	/* Tested in 7.1: 40 was enough, 30 wasn't. */
#define C_STACK_MARGIN 8000	/* Tested in 7.1: 3000 was enough, 2600 wasn't. */


PMOD_EXPORT const char *Pike_check_stack_errmsg =
  "Svalue stack overflow. "
  "(%ld of %ld entries on stack, needed %ld more entries)\n";
PMOD_EXPORT const char *Pike_check_mark_stack_errmsg =
  "Mark stack overflow.\n";
PMOD_EXPORT const char *Pike_check_c_stack_errmsg =
  "C stack overflow.\n";


#ifdef PIKE_DEBUG
static char trace_buffer[2000];
#endif


/* Pike_sp points to first unused value on stack
 * (much simpler than letting it point at the last used value.)
 */
PMOD_EXPORT struct Pike_interpreter Pike_interpreter;
PMOD_EXPORT int Pike_stack_size = EVALUATOR_STACK_SIZE;


void push_sp_mark(void)
{
  if(Pike_mark_sp == Pike_interpreter.mark_stack + Pike_stack_size)
    Pike_error("No more mark stack!\n");
  *Pike_mark_sp++=Pike_sp;
}
ptrdiff_t pop_sp_mark(void)
{
#ifdef PIKE_DEBUG
  if(Pike_mark_sp < Pike_interpreter.mark_stack)
    fatal("Mark stack underflow!\n");
#endif
  return Pike_sp - *--Pike_mark_sp;
}


#ifdef PIKE_DEBUG
static void gc_check_stack_callback(struct callback *foo, void *bar, void *gazonk)
{
  struct pike_frame *f;
  debug_gc_xmark_svalues(Pike_interpreter.evaluator_stack,
			 Pike_sp-Pike_interpreter.evaluator_stack-1,
			 " on current interpreter stack");

  for(f=Pike_fp;f;f=f->next)
  {
    if(f->context.parent)
      gc_external_mark2(f->context.parent,0," in Pike_fp->context.parent on current stack");
    gc_external_mark2(f->current_object,0," in Pike_fp->current_object on current stack");
    gc_external_mark2(f->context.prog,0," in Pike_fp->context.prog on current stack");
  }

}
#endif

PMOD_EXPORT void init_interpreter(void)
{
#ifdef USE_MMAP_FOR_STACK
  static int fd = -1;


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
	Pike_interpreter.evaluator_stack=0;
	Pike_interpreter.mark_stack=0;
	goto use_malloc;
      }
    }
    /* Don't keep this fd on exec() */
    set_close_on_exec(fd, 1);
  }
#endif

#define MMALLOC(X,Y) (Y *)mmap(0,X*sizeof(Y),PROT_READ|PROT_WRITE, MAP_NORESERVE | MAP_PRIVATE | MAP_ANONYMOUS, fd, 0)

  Pike_interpreter.evaluator_stack_malloced=0;
  Pike_interpreter.mark_stack_malloced=0;
  Pike_interpreter.evaluator_stack=MMALLOC(Pike_stack_size,struct svalue);
  Pike_interpreter.mark_stack=MMALLOC(Pike_stack_size, struct svalue *);
  if((char *)MAP_FAILED == (char *)Pike_interpreter.evaluator_stack) Pike_interpreter.evaluator_stack=0;
  if((char *)MAP_FAILED == (char *)Pike_interpreter.mark_stack) Pike_interpreter.mark_stack=0;
#else
  Pike_interpreter.evaluator_stack=0;
  Pike_interpreter.mark_stack=0;
#endif

use_malloc:
  if(!Pike_interpreter.evaluator_stack)
  {
    Pike_interpreter.evaluator_stack=(struct svalue *)xalloc(Pike_stack_size*sizeof(struct svalue));
    Pike_interpreter.evaluator_stack_malloced=1;
  }

  if(!Pike_interpreter.mark_stack)
  {
    Pike_interpreter.mark_stack=(struct svalue **)xalloc(Pike_stack_size*sizeof(struct svalue *));
    Pike_interpreter.mark_stack_malloced=1;
  }

  Pike_sp=Pike_interpreter.evaluator_stack;
  Pike_mark_sp=Pike_interpreter.mark_stack;
  Pike_fp=0;

  Pike_interpreter.svalue_stack_margin = SVALUE_STACK_MARGIN;
  Pike_interpreter.c_stack_margin = C_STACK_MARGIN;

#ifdef PIKE_DEBUG
  {
    static struct callback *spcb;
    if(!spcb)
    {
      spcb=add_gc_callback(gc_check_stack_callback,0,0);
      dmalloc_accept_leak(spcb);
    }
  }
#endif
#ifdef PROFILING
#ifdef HAVE_GETHRTIME
  Pike_interpreter.time_base = gethrtime();
  Pike_interpreter.accounted_time =0;
#endif
#endif
}


static int eval_instruction(unsigned char *pc);


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
#ifdef PIKE_SECURITY
  if(lval->type <= MAX_COMPLEX)
    if(!CHECK_DATA_SECURITY(lval->u.array, SECURITY_BIT_INDEX))
      Pike_error("Index permission denied.\n");
#endif
  switch(lval->type)
  {
    case T_ARRAY_LVALUE:
    {
      INT32 e;
      struct array *a;
      ONERROR err;
      a=allocate_array(lval[1].u.array->size>>1);
      SET_ONERROR(err, do_free_array, a);
      for(e=0;e<a->size;e++)
	lvalue_to_svalue_no_free(a->item+e, lval[1].u.array->item+(e<<1));
      to->type = T_ARRAY;
      to->u.array=a;
      UNSET_ONERROR(err);
      break;
    }
      
    case T_LVALUE:
      assign_svalue_no_free(to, lval->u.lval);
      break;
      
    case T_SHORT_LVALUE:
      assign_from_short_svalue_no_free(to, lval->u.short_lval,
				       (TYPE_T)lval->subtype);
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
	to->u.integer=1;
	to->subtype=NUMBER_NUMBER;
      }else{
	to->u.integer=0;
	to->subtype=NUMBER_UNDEFINED;
      }
      break;
      
    default:
      if(IS_ZERO(lval))
	index_error(0,0,0,lval,lval+1,"Indexing the NULL value.\n");
      else
	index_error(0,0,0,lval,lval+1,"Indexing a basic type.\n");
  }
}

PMOD_EXPORT void assign_lvalue(struct svalue *lval,struct svalue *from)
{
#ifdef PIKE_SECURITY
  if(lval->type <= MAX_COMPLEX)
    if(!CHECK_DATA_SECURITY(lval->u.array, SECURITY_BIT_SET_INDEX))
      Pike_error("Assign index permission denied.\n");
#endif

  switch(lval->type)
  {
    case T_ARRAY_LVALUE:
    {
      INT32 e;
      if(from->type != T_ARRAY)
	Pike_error("Trying to assign combined lvalue from non-array.\n");

      if(from->u.array->size < (lval[1].u.array->size>>1))
	Pike_error("Not enough values for multiple assign.\n");

      if(from->u.array->size > (lval[1].u.array->size>>1))
	Pike_error("Too many values for multiple assign.\n");

      for(e=0;e<from->u.array->size;e++)
	assign_lvalue(lval[1].u.array->item+(e<<1),from->u.array->item+e);
    }
    break;

  case T_LVALUE:
    assign_svalue(lval->u.lval,from);
    break;

  case T_SHORT_LVALUE:
    assign_to_short_svalue(lval->u.short_lval, (TYPE_T)lval->subtype, from);
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
     index_error(0,0,0,lval,lval+1,"Indexing the NULL value.\n");
   else
     index_error(0,0,0,lval,lval+1,"Indexing a basic type.\n");
  }
}

union anything *get_pointer_if_this_type(struct svalue *lval, TYPE_T t)
{
#ifdef PIKE_SECURITY
  if(lval->type <= MAX_COMPLEX)
    if(!CHECK_DATA_SECURITY(lval->u.array, SECURITY_BIT_SET_INDEX))
      Pike_error("Assign index permission denied.\n");
#endif

  switch(lval->type)
  {
    case T_ARRAY_LVALUE:
      return 0;
      
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
	index_error(0,0,0,lval,lval+1,"Indexing the NULL value.\n");
      else
	index_error(0,0,0,lval,lval+1,"Indexing a basic type.\n");
      return 0;
  }
}

#ifdef PIKE_DEBUG

inline void pike_trace(int level,char *fmt, ...) ATTRIBUTE((format (printf, 2, 3)));
inline void pike_trace(int level,char *fmt, ...)
{
  if(t_flag > level)
  {
    va_list args;
    va_start(args,fmt);
    vsprintf(trace_buffer,fmt,args);
    va_end(args);
    write_to_stderr(trace_buffer,strlen(trace_buffer));
  }
}

void my_describe_inherit_structure(struct program *p)
{
  struct inherit *in,*last=0;
  int e,i=0;
  last=p->inherits-1;

  fprintf(stderr,"PROGRAM[%d]: inherits=%d identifers_refs=%d ppid=%d\n",
	  p->id,
	  p->num_inherits,
	  p->num_identifier_references,
	  p->parent_program_id);
  for(e=0;e<p->num_identifier_references;e++)
  {
    in=INHERIT_FROM_INT(p,e);
    while(last < in)
    {
      last++;
      fprintf(stderr,
	      "[%ld]%*s parent{ offset=%d ident=%d id=%d } "
	      "id{ level=%d } prog=%d\n",
	      DO_NOT_WARN((long)(last - p->inherits)),
	      last->inherit_level*2,"",
	      last->parent_offset,
	      last->parent_identifier,
	      last->prog->parent_program_id,
	      last->identifier_level,
	      last->prog->id);
      i=0;
    }

    fprintf(stderr,"   %*s %d,%d: %s\n",
	      in->inherit_level*2,"",
	      e,i,
	    ID_FROM_INT(p,e)->name->str);
    i++;
  }
}

#define TRACE(X) pike_trace X
#else
#define TRACE(X)
#endif

PMOD_EXPORT void find_external_context(struct external_variable_context *loc,
				       int arg2)
{
  struct program *p;
  INT32 e,off;
  TRACE((4, "-find_external_context(%d, inherit=%ld)\n", arg2,
	 DO_NOT_WARN((long)(loc->o->prog ? loc->inherit - loc->o->prog->inherits : 0))));

  if(!loc->o)
    Pike_error("Current object is destructed\n");

  while(--arg2>=0)
  {
#ifdef PIKE_DEBUG  
    if(t_flag>8 && loc->o->prog)
      my_describe_inherit_structure(loc->o->prog);
#endif

    TRACE((4,"-   i->parent_offset=%d i->parent_identifier=%d\n",
	   loc->inherit->parent_offset,
	   loc->inherit->parent_identifier));
    TRACE((4,"-   o->parent_identifier=%d inherit->identifier_level=%d\n",
	   loc->o->parent_identifier,
	   loc->inherit->identifier_level));

    switch(loc->inherit->parent_offset)
    {
      default:
	{
	  struct external_variable_context tmp=*loc;
#ifdef PIKE_DEBUG
	  if(!loc->inherit->inherit_level)
	    fatal("Gahhh! inherit level zero in wrong place!\n");
#endif
	  while(tmp.inherit->inherit_level >= loc->inherit->inherit_level)
	  {
	    TRACE((5,"-   inherit-- (%d >= %d)\n",tmp.inherit->inherit_level, loc->inherit->inherit_level));
	    tmp.inherit--;
	  }


	  find_external_context(&tmp,
				loc->inherit->parent_offset);
	  loc->o=tmp.o;
	  loc->parent_identifier =
	    loc->inherit->parent_identifier+
	    tmp.inherit->identifier_level;
	}
	break;

      case -17:
	TRACE((5,"-   Following inherit->parent\n"));
	loc->parent_identifier=loc->inherit->parent_identifier;
	loc->o=loc->inherit->parent;
	break;

      case -18:
	TRACE((5,"-   Following o->parent\n"));
	loc->parent_identifier=loc->o->parent_identifier;
	loc->o=loc->o->parent;
	break;
    }
    
    if(!loc->o)
      Pike_error("Parent was lost during cloning.\n");
    
    if(!(p=loc->o->prog))
      Pike_error("Attempting to access variable in destructed object\n");
    
#ifdef DEBUG_MALLOC
    if (loc->o->refs == 0x55555555) {
      fprintf(stderr, "The object %p has been zapped!\n", loc->o);
      describe(p);
      fatal("Object zapping detected.\n");
    }
    if (p->refs == 0x55555555) {
      fprintf(stderr, "The program %p has been zapped!\n", p);
      describe(p);
      fprintf(stderr, "Which taken from the object %p\n", loc->o);
      describe(loc->o);
      fatal("Looks like the program %p has been zapped!\n", p);
    }
#endif /* DEBUG_MALLOC */
    
#ifdef PIKE_DEBUG
    if(loc->parent_identifier < 0 ||
       loc->parent_identifier > p->num_identifier_references)
      fatal("Identifier out of range, loc->parent_identifer=%d!\n",
	    loc->parent_identifier);
#endif

    loc->inherit=INHERIT_FROM_INT(p, loc->parent_identifier);

#ifdef PIKE_DEBUG  
    if(t_flag>28)
      my_describe_inherit_structure(p);
#endif

    TRACE((5,"-   Parent identifier = %d (%s), inherit # = %ld\n",
	   loc->parent_identifier,
	   ID_FROM_INT(p, loc->parent_identifier)->name->str,
	   DO_NOT_WARN((long)(loc->inherit - p->inherits))));
    
#ifdef DEBUG_MALLOC
    if (loc->inherit->storage_offset == 0x55555555) {
      fprintf(stderr, "The inherit %p has been zapped!\n", loc->inherit);
      debug_malloc_dump_references(loc->inherit,0,2,0);
      fprintf(stderr, "It was extracted from the program %p %d\n", p, loc->parent_identifier);
      describe(p);
      fprintf(stderr, "Which was in turn taken from the object %p\n", loc->o);
      describe(loc->o);
      fatal("Looks like the program %p has been zapped!\n", p);
    }
#endif /* DEBUG_MALLOC */
  }

  TRACE((4,"--find_external_context: parent_id=%d (%s)\n",
	 loc->parent_identifier,
	 ID_FROM_INT(loc->o->prog,loc->parent_identifier)->name->str
	 ));
}


#ifdef PIKE_DEBUG
void print_return_value(void)
{
  if(t_flag>3)
  {
    char *s;
	
    init_buf();
    describe_svalue(Pike_sp-1,0,0);
    s=simple_free_buf();
    if((size_t)strlen(s) > (size_t)TRACE_LEN)
    {
      s[TRACE_LEN]=0;
      s[TRACE_LEN-1]='.';
      s[TRACE_LEN-2]='.';
      s[TRACE_LEN-3]='.';
    }
    fprintf(stderr,"-    value: %s\n",s);
    free(s);
  }
}
#else
#define print_return_value()
#endif

struct callback_list evaluator_callbacks;
#define CASE(X) case (X)-F_OFFSET:

#define DOJUMP() \
 do { int tmp; tmp=EXTRACT_INT(pc); pc+=tmp; if(tmp < 0) fast_check_threads_etc(6); }while(0)

#define SKIPJUMP() pc+=sizeof(INT32)

#define COMPARISMENT(ID,EXPR) \
CASE(ID); \
instr=EXPR; \
pop_n_elems(2); \
push_int(instr); \
break

#ifdef AUTO_BIGNUM
#define AUTO_BIGNUM_LOOP_TEST(X,Y) INT_TYPE_ADD_OVERFLOW(X,Y)
#else
#define AUTO_BIGNUM_LOOP_TEST(X,Y) 0
#endif

#define LOOP(ID, INC, OP2, OP4)					\
CASE(ID)							\
{								\
  union anything *i=get_pointer_if_this_type(Pike_sp-2, T_INT);	\
  if(i && !AUTO_BIGNUM_LOOP_TEST(i->integer,INC) &&		\
     Pike_sp[-3].type == T_INT)					\
  {								\
    i->integer += INC;						\
    if(i->integer OP2 Pike_sp[-3].u.integer)				\
    {								\
      pc+=EXTRACT_INT(pc);					\
      fast_check_threads_etc(8);				\
    }else{							\
      pc+=sizeof(INT32);					\
    }                                                           \
  }else{							\
    lvalue_to_svalue_no_free(Pike_sp,Pike_sp-2); Pike_sp++;	\
    push_int(INC);						\
    f_add(2);							\
    assign_lvalue(Pike_sp-3,Pike_sp-1);				\
    if(OP4 ( Pike_sp-1, Pike_sp-4 ))				\
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
if(Y(Pike_sp-2,Pike_sp-1)) { \
  DOJUMP(); \
}else{ \
  SKIPJUMP(); \
} \
pop_n_elems(2); \
break


/*
 * reset the stack machine.
 */
void reset_evaluator(void)
{
  Pike_fp=0;
  pop_n_elems(Pike_sp - Pike_interpreter.evaluator_stack);
}

#ifdef PIKE_DEBUG
#define BACKLOG 1024
struct backlog
{
  INT32 instruction;
  INT32 arg,arg2;
  struct program *program;
  unsigned char *pc;
#ifdef _REENTRANT
  struct object *thread_id;
#endif
  ptrdiff_t stack;
  ptrdiff_t mark_stack;
};

struct backlog backlog[BACKLOG];
int backlogp=BACKLOG-1;

void dump_backlog(void)
{
#ifdef _REENTRANT
  struct object *thread=0;
#endif

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

#ifdef _REENTRANT
      if(thread != backlog[e].thread_id)
      {
	fprintf(stderr,"[Thread swap, Pike_interpreter.thread_id=%p]\n",backlog[e].thread_id);
	thread = backlog[e].thread_id;
      }
#endif

      file=get_line(backlog[e].pc-1,backlog[e].program, &line);
      if(backlog[e].instruction < 0 || backlog[e].instruction+F_OFFSET > F_MAX_OPCODE)
      {
	fprintf(stderr,"%s:%ld: ILLEGAL INSTRUCTION %d\n",
		file,
		(long)line,
		backlog[e].instruction + F_OFFSET);
	continue;
      }


      fprintf(stderr,"%s:%ld: %s",
	      file,
	      (long)line,
	      low_get_f_name(backlog[e].instruction + F_OFFSET, backlog[e].program));
      if(instrs[backlog[e].instruction].flags & I_HASARG2)
      {
	fprintf(stderr,"(%ld,%ld)",
		(long)backlog[e].arg,
		(long)backlog[e].arg2);
      }
      else if(instrs[backlog[e].instruction].flags & I_HASARG)
      {
	fprintf(stderr,"(%ld)", (long)backlog[e].arg);
      }
      fprintf(stderr," %ld, %ld\n",
	      DO_NOT_WARN((long)backlog[e].stack),
	      DO_NOT_WARN((long)backlog[e].mark_stack));
    }
  }while(e!=backlogp);
}

#endif
static int o_catch(unsigned char *pc);

struct light_frame_info
{
  struct pike_frame *saved_fp;
  struct svalue *expendible;
  struct svalue *locals;
};

static void restore_light_frame_info(struct light_frame_info *info)
{
  if (Pike_fp == info->saved_fp) {
    Pike_fp->expendible = info->expendible;
    Pike_fp->locals = info->locals;
  }
}

#ifdef PIKE_DEBUG
#define EVAL_INSTR_RET_CHECK(x)						\
  if (x == -2)								\
    fatal("Return value -2 from eval_instruction is not handled here.\n"\
	  "Probable cause: F_ESCAPE_CATCH outside catch block.\n")
#else
#define EVAL_INSTR_RET_CHECK(x)
#endif

#ifdef PIKE_DEBUG
#define eval_instruction eval_instruction_with_debug
#include "interpreter.h"

#undef eval_instruction
#define eval_instruction eval_instruction_without_debug
#undef PIKE_DEBUG
#define print_return_value()
#include "interpreter.h"
#undef print_return_value
#define PIKE_DEBUG
#undef eval_instruction

static inline int eval_instruction(unsigned char *pc)
{
  if(d_flag || t_flag>2)
    return eval_instruction_with_debug(pc);
  else
    return eval_instruction_without_debug(pc);
}


#else
#include "interpreter.h"
#endif

static void trace_return_value(void)
{
  char *s;
  
  init_buf();
  my_strcat("Return: ");
  describe_svalue(Pike_sp-1,0,0);
  s=simple_free_buf();
  if((size_t)strlen(s) > (size_t)TRACE_LEN)
  {
    s[TRACE_LEN]=0;
    s[TRACE_LEN-1]='.';
    s[TRACE_LEN-2]='.';
    s[TRACE_LEN-2]='.';
  }
  fprintf(stderr,"%-*s%s\n",4,"-",s);
  free(s);
}

static void do_trace_call(INT32 args)
{
  char *file,*s;
  INT32 linep,e;
  my_strcat("(");
  for(e=0;e<args;e++)
  {
    if(e) my_strcat(",");
    describe_svalue(Pike_sp-args+e,0,0);
  }
  my_strcat(")"); 
  s=simple_free_buf();
  if((size_t)strlen(s) > (size_t)TRACE_LEN)
  {
    s[TRACE_LEN]=0;
    s[TRACE_LEN-1]='.';
    s[TRACE_LEN-2]='.';
    s[TRACE_LEN-2]='.';
  }
  if(Pike_fp && Pike_fp->pc)
  {
    char *f;
    file=get_line(Pike_fp->pc,Pike_fp->context.prog,&linep);
    while((f=STRCHR(file,'/'))) file=f+1;
  }else{
    linep=0;
    file="-";
  }
  fprintf(stderr,"- %s:%4ld: %s\n",file,(long)linep,s);
  free(s);
}


#undef INIT_BLOCK
#define INIT_BLOCK(X) do { X->refs=1; X->malloced_locals=0; X->scope=0; }while(0)

#undef EXIT_BLOCK
#define EXIT_BLOCK(X) do {				\
  free_object(X->current_object);			\
  if(X->context.prog) free_program(X->context.prog);	\
  if(X->context.parent) free_object(X->context.parent);	\
  if(X->scope) free_pike_frame(X->scope);		\
  if(X->malloced_locals)				\
  {							\
    free_svalues(X->locals,X->num_locals,BIT_MIXED);	\
    free((char *)(X->locals));				\
  }							\
  DO_IF_DMALLOC(					\
    X->context.prog=0;					\
    X->context.parent=0;				\
    X->context.name=0;					\
    X->scope=0;						\
    X->current_object=0;				\
    X->malloced_locals=0;				\
    X->expendible=0;					\
    X->locals=0;					\
 )							\
}while(0)

BLOCK_ALLOC(pike_frame,128)



#ifdef PIKE_SECURITY
/* Magic trick */
static

#else
#define mega_apply2 mega_apply
#endif


PMOD_EXPORT void mega_apply2(enum apply_type type, INT32 args, void *arg1, void *arg2)
{
  struct object *o;
  struct pike_frame *scope=0;
  int tailrecurse=-1;
  ptrdiff_t fun;
  struct svalue *save_sp=Pike_sp-args;

#ifdef PROFILING
#ifdef HAVE_GETHRTIME
  long long children_base = Pike_interpreter.accounted_time;
  long long start_time = gethrtime() - Pike_interpreter.time_base;
  unsigned INT32 self_time_base;
#if 0
#ifdef PIKE_DEBUG
  if(start_time < 0)
  {
    fatal("gethrtime() shrunk\n start_time=%ld\n gethrtime()=%ld\n Pike_interpreter.time_base=%ld\n",
	  (long)(start_time/100000), 
	  (long)(gethrtime()/100000), 
	  (long)(Pike_interpreter.time_base/100000));
  }
#endif
#endif
#endif
#endif

#if defined(PIKE_DEBUG) && defined(_REENTRANT)
  if(d_flag)
    {
      THREAD_T self = th_self();

      CHECK_INTERPRETER_LOCK();

      if( Pike_interpreter.thread_id && !th_equal( OBJ2THREAD(Pike_interpreter.thread_id)->id, self) )
	fatal("Current thread is wrong.\n");
	
      if(thread_for_id(th_self()) != Pike_interpreter.thread_id)
	fatal("thread_for_id() (or Pike_interpreter.thread_id) failed in mega_apply! "
	      "%p != %p\n", thread_for_id(self), Pike_interpreter.thread_id);
    }
#endif

  switch(type)
  {
  case APPLY_STACK:
  apply_stack:
    if(!args)
      PIKE_ERROR("`()", "Too few arguments (apply stack).\n", Pike_sp, 0);
    args--;
    if(Pike_sp-save_sp-args > (args<<2) + 32)
    {
      /* The test above assures these two areas
       * are not overlapping
       */
      assign_svalues(save_sp, Pike_sp-args-1, args+1, BIT_MIXED);
      pop_n_elems(Pike_sp-save_sp-args-1);
    }
    arg1=(void *)(Pike_sp-args-1);

  case APPLY_SVALUE:
  apply_svalue:
  {
    struct svalue *s=(struct svalue *)arg1;
    switch(s->type)
    {
    case T_INT:
      if (!s->u.integer) {
	PIKE_ERROR("0", "Attempt to call the NULL-value\n", Pike_sp, args);
      } else {
	Pike_error("Attempt to call the value %d\n", s->u.integer);
      }

    case T_STRING:
      if (s->u.string->len > 20) {
	Pike_error("Attempt to call the string \"%20s\"...\n", s->u.string->str);
      } else {
	Pike_error("Attempt to call the string \"%s\"\n", s->u.string->str);
      }
    case T_MAPPING:
      Pike_error("Attempt to call a mapping\n");
    default:
      Pike_error("Call to non-function value type:%s.\n",
	    get_name_of_type(s->type));
      
    case T_FUNCTION:
      if(s->subtype == FUNCTION_BUILTIN)
      {
#ifdef PIKE_DEBUG
	struct svalue *expected_stack = Pike_sp-args;
	if(t_flag>1)
	{
	  init_buf();
	  describe_svalue(s,0,0);
	  do_trace_call(args);
	}
#endif
	(*(s->u.efun->function))(args);

#ifdef PIKE_DEBUG
	if(Pike_sp != expected_stack + !s->u.efun->may_return_void)
	{
	  if(Pike_sp < expected_stack)
	    fatal("Function popped too many arguments: %s\n",
		  s->u.efun->name->str);
	  if(Pike_sp>expected_stack+1)
	    fatal("Function left droppings on stack: %s\n",
		  s->u.efun->name->str);
	  if(Pike_sp == expected_stack && !s->u.efun->may_return_void)
	    fatal("Non-void function returned without return value on stack: %s %d\n",
		  s->u.efun->name->str,s->u.efun->may_return_void);
	  if(Pike_sp==expected_stack+1 && s->u.efun->may_return_void)
	    fatal("Void function returned with a value on the stack: %s %d\n",
		  s->u.efun->name->str, s->u.efun->may_return_void);
	}
#endif

	break;
      }else{
	o=s->u.object;
	if(o->prog == pike_trampoline_program)
	{
	  fun=((struct pike_trampoline *)(o->storage))->func;
	  scope=((struct pike_trampoline *)(o->storage))->frame;
	  o=scope->current_object;
	  goto apply_low_with_scope;
	}
	fun=s->subtype;
	goto apply_low;
      }
      break;

    case T_ARRAY:
#ifdef PIKE_DEBUG
      if(t_flag>1)
      {
	init_buf();
	describe_svalue(s,0,0);
	do_trace_call(args);
      }
#endif
      apply_array(s->u.array,args);
      break;

    case T_PROGRAM:
#ifdef PIKE_DEBUG
      if(t_flag>1)
      {
	init_buf();
	describe_svalue(s,0,0);
	do_trace_call(args);
      }
#endif
      push_object(clone_object(s->u.program,args));
      break;

    case T_OBJECT:
      o=s->u.object;
      if(o->prog == pike_trampoline_program)
      {
	fun=((struct pike_trampoline *)(o->storage))->func;
	scope=((struct pike_trampoline *)(o->storage))->frame;
	o=scope->current_object;
	goto apply_low_with_scope;
      }
      fun=LFUN_CALL;
      goto call_lfun;
    }
    break;
  }

  call_lfun:
#ifdef PIKE_DEBUG
    if(fun < 0 || fun >= NUM_LFUNS)
      fatal("Apply lfun on illegal value!\n");
#endif
    if(!o->prog)
      PIKE_ERROR("destructed object", "Apply on destructed object.\n", Pike_sp, args);
    fun = FIND_LFUN(o->prog, fun);
    goto apply_low;
  

  case APPLY_LOW:
    o = (struct object *)arg1;
    fun = (ptrdiff_t)arg2;

  apply_low:
    scope=0;
  apply_low_with_scope:
    {
      struct program *p;
      struct reference *ref;
      struct pike_frame *new_frame;
      struct identifier *function;
      
      if(fun<0)
      {
	pop_n_elems(Pike_sp-save_sp);
	push_int(0);
	return;
      }

      check_stack(256);
      check_mark_stack(256);
      check_c_stack(8192);


#ifdef PIKE_DEBUG
      if(d_flag>2) do_debug();
#endif

      p=o->prog;
      if(!p)
	PIKE_ERROR("destructed object->function",
	      "Cannot call functions in destructed objects.\n", Pike_sp, args);

#ifdef PIKE_SECURITY
      CHECK_DATA_SECURITY_OR_ERROR(o, SECURITY_BIT_CALL,
				   ("Function call permission denied.\n"));

      if(!CHECK_DATA_SECURITY(o, SECURITY_BIT_NOT_SETUID))
	SET_CURRENT_CREDS(o->prot);
#endif


#ifdef PIKE_DEBUG
      if(fun>=(int)p->num_identifier_references)
      {
	fprintf(stderr, "Function index out of range. %ld >= %d\n",
		DO_NOT_WARN((long)fun),
		(int)p->num_identifier_references);
	fprintf(stderr,"########Program is:\n");
	describe(p);
	fprintf(stderr,"########Object is:\n");
	describe(o);
	fatal("Function index out of range.\n");
      }
#endif

      ref = p->identifier_references + fun;
#ifdef PIKE_DEBUG
      if(ref->inherit_offset>=p->num_inherits)
	fatal("Inherit offset out of range in program.\n");
#endif

      /* init a new evaluation pike_frame */
      new_frame=alloc_pike_frame();
      debug_malloc_touch(new_frame);

      new_frame->next = Pike_fp;
      new_frame->current_object = o;
      new_frame->context = p->inherits[ ref->inherit_offset ];

      function = new_frame->context.prog->identifiers + ref->identifier_offset;

      
#ifdef PIKE_DEBUG
	if(t_flag > 9)
	{
	  fprintf(stderr,"-- ref: inoff=%d idoff=%d flags=%d\n",
		  ref->inherit_offset,
		  ref->identifier_offset,
		  ref->id_flags);

	  fprintf(stderr,"-- context: prog->id=%d inlev=%d idlev=%d pi=%d po=%d so=%ld name=%s\n",
		  new_frame->context.prog->id,
		  new_frame->context.inherit_level,
		  new_frame->context.identifier_level,
		  new_frame->context.parent_identifier,
		  new_frame->context.parent_offset,
		  DO_NOT_WARN((long)new_frame->context.storage_offset),
		  new_frame->context.name ? new_frame->context.name->str  : "NULL");
	  if(t_flag>19)
	  {
	    describe(new_frame->context.prog);
	  }
	}
#endif


      new_frame->locals = Pike_sp - args;
      new_frame->expendible = new_frame->locals;
      new_frame->args = args;
      new_frame->fun = DO_NOT_WARN((unsigned INT16)fun);
      new_frame->current_storage = o->storage+new_frame->context.storage_offset;
      new_frame->pc = 0;
      new_frame->scope=scope;
      
      add_ref(new_frame->current_object);
      add_ref(new_frame->context.prog);
      if(new_frame->context.parent) add_ref(new_frame->context.parent);
      if(new_frame->scope) add_ref(new_frame->scope);

      if(t_flag)
      {
	char buf[50];

	init_buf();
	sprintf(buf, "%lx->",
		DO_NOT_WARN((long)o));
	my_strcat(buf);
	my_strcat(function->name->str);
	do_trace_call(args);
      }
      
      Pike_fp = new_frame;
      
#ifdef PROFILING
      function->num_calls++;
#endif
  
      if(function->func.offset == -1)
	generic_error(NULL, Pike_sp, args,
		      "Calling undefined function.\n");
      
      tailrecurse=-1;

#ifdef PROFILING
#ifdef HAVE_GETHRTIME
      self_time_base=function->total_time;
#endif
#endif

      switch(function->identifier_flags & (IDENTIFIER_FUNCTION | IDENTIFIER_CONSTANT))
      {
      case IDENTIFIER_C_FUNCTION:
	debug_malloc_touch(Pike_fp);
	Pike_fp->num_args=args;
	new_frame->num_locals=args;
	check_threads_etc();
	(*function->func.c_fun)(args);
	break;
	
      case IDENTIFIER_CONSTANT:
      {
	struct svalue *s=&(Pike_fp->context.prog->
			   constants[function->func.offset].sval);
	debug_malloc_touch(Pike_fp);
	if(s->type == T_PROGRAM)
	{
	  struct object *tmp;
	  check_threads_etc();
	  tmp=parent_clone_object(s->u.program,
				  o,
				  fun,
				  args);
	  push_object(tmp);
	  break;
	}
	/* Fall through */
      }
      
      case 0:
      {
	debug_malloc_touch(Pike_fp);
	debug_malloc_touch(o);
	if(Pike_sp-save_sp-args<=0)
	{
	  /* Create an extra svalue for tail recursion style call */
	  Pike_sp++;
	  MEMMOVE(Pike_sp-args,Pike_sp-args-1,sizeof(struct svalue)*args);
	  Pike_sp[-args-1].type=T_INT;
	}else{
	  free_svalue(Pike_sp-args-1);
	  Pike_sp[-args-1].type=T_INT;
	}
	low_object_index_no_free(Pike_sp-args-1,o,fun);
	tailrecurse=args+1;
	break;
      }

      case IDENTIFIER_PIKE_FUNCTION:
      {
	int num_args;
	int num_locals;
	unsigned char *pc;

#ifdef PIKE_DEBUG
	if (Pike_in_gc > GC_PASS_PREPARE && Pike_in_gc < GC_PASS_FREE)
	  fatal("Pike code called within gc.\n");
#endif

	debug_malloc_touch(Pike_fp);
	pc=new_frame->context.prog->program + function->func.offset;
	
	num_locals=EXTRACT_UCHAR(pc++);
	num_args=EXTRACT_UCHAR(pc++);

	if(function->identifier_flags & IDENTIFIER_SCOPE_USED)
	  new_frame->expendible+=num_locals;
	
	/* adjust arguments on stack */
	if(args < num_args) /* push zeros */
	{
	  clear_svalues_undefined(Pike_sp, num_args-args);
	  Pike_sp += num_args-args;
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

	if(num_locals > args)
	  clear_svalues(Pike_sp, num_locals - args);
	Pike_sp += num_locals - args;
#ifdef PIKE_DEBUG
	if(num_locals < num_args)
	  fatal("Wrong number of arguments or locals in function def.\n");
#endif
	new_frame->num_locals=num_locals;
	new_frame->num_args=num_args;

	check_threads_etc();

	{
	  struct svalue **save_mark_sp=Pike_mark_sp;
	  tailrecurse=eval_instruction(pc);
	  EVAL_INSTR_RET_CHECK(tailrecurse);
	  Pike_mark_sp=save_mark_sp;
#ifdef PIKE_DEBUG
	  if(Pike_mark_sp < save_mark_sp)
	    fatal("Popped below save_mark_sp!\n");
#endif
	}
#ifdef PIKE_DEBUG
	if(Pike_sp<Pike_interpreter.evaluator_stack)
	  fatal("Stack error (also simple).\n");
#endif
	break;
      }
      
      }
#ifdef PROFILING
#ifdef HAVE_GETHRTIME
      {
	long long time_passed, time_in_children, self_time;
	time_in_children=  Pike_interpreter.accounted_time - children_base;
	time_passed = gethrtime() - Pike_interpreter.time_base - start_time;
	self_time=time_passed - time_in_children;
	Pike_interpreter.accounted_time+=self_time;
#if 0
#ifdef PIKE_DEBUG
	if(self_time < 0 || children_base <0 || Pike_interpreter.accounted_time <0)
	  fatal("Time is negative\n  self_time=%ld\n  time_passed=%ld\n  time_in_children=%ld\n  children_base=%ld\n  Pike_interpreter.accounted_time=%ld!\n  Pike_interpreter.time_base=%ld\n  start_time=%ld\n",
		(long)(self_time/100000),
		(long)(time_passed/100000),
		(long)(time_in_children/100000),
		(long)(children_base/100000),
		(long)(Pike_interpreter.accounted_time/100000),
		(long)(Pike_interpreter.time_base/100000),
		(long)(start_time/100000)
		);
#endif
#endif
	function->total_time=self_time_base + (INT32)(time_passed /1000);
	function->self_time+=(INT32)( self_time /1000);
      }
#endif
#endif

#if 0
      if(Pike_sp - new_frame->locals > 1)
      {
	pop_n_elems(Pike_sp - new_frame->locals -1);
      }else if(Pike_sp - new_frame->locals < 1){
#ifdef PIKE_DEBUG
	if(Pike_sp - new_frame->locals<0) fatal("Frame underflow.\n");
#endif
	Pike_sp->u.integer = 0;
	Pike_sp->subtype=NUMBER_NUMBER;
	Pike_sp->type = T_INT;
	Pike_sp++;
      }
#endif

#ifdef PIKE_DEBUG
      if(Pike_fp!=new_frame)
	fatal("Frame stack out of whack!\n");
#endif
      
      POP_PIKE_FRAME();

      if(tailrecurse>=0)
      {
	args=tailrecurse;
	goto apply_stack;
      }

    }
  }

  if(save_sp+1 < Pike_sp)
  {
    assign_svalue(save_sp,Pike_sp-1);
    pop_n_elems(Pike_sp-save_sp-1);

    destruct_objects_to_destruct(); /* consider using a flag for immediate destruct instead... */
  }

  if(save_sp+1 > Pike_sp)
  {
    if(type != APPLY_SVALUE)
      push_int(0);
  }else{
    if(t_flag>1) trace_return_value();
  }
}

#ifdef PIKE_SECURITY
static void restore_creds(struct object *creds)
{
  if(Pike_interpreter.current_creds)
    free_object(Pike_interpreter.current_creds);
  Pike_interpreter.current_creds = creds;
}

PMOD_EXPORT void mega_apply(enum apply_type type, INT32 args, void *arg1, void *arg2)
{
  ONERROR tmp;
  if(Pike_interpreter.current_creds)
    add_ref(Pike_interpreter.current_creds);

  SET_ONERROR(tmp, restore_creds, Pike_interpreter.current_creds);
  mega_apply2(type, args, arg1, arg2);
  CALL_AND_UNSET_ONERROR(tmp);
}
#endif


/* Put catch outside of eval_instruction, so
 * the setjmp won't affect the optimization of
 * eval_instruction
 */
static int o_catch(unsigned char *pc)
{
  JMP_BUF tmp;
  struct svalue *expendible=Pike_fp->expendible;
  debug_malloc_touch(Pike_fp);
  if(SETJMP(tmp))
  {
    *Pike_sp=throw_value;
    throw_value.type=T_INT;
    Pike_sp++;
    UNSETJMP(tmp);
    Pike_fp->expendible=expendible;
    return 0;
  }else{
    struct svalue **save_mark_sp=Pike_mark_sp;
    int x;
    Pike_fp->expendible=Pike_fp->locals + Pike_fp->num_locals;
    x=eval_instruction(pc);
#ifdef PIKE_DEBUG
    if(Pike_mark_sp < save_mark_sp)
      fatal("mark Pike_sp underflow in catch.\n");
#endif
    Pike_mark_sp=save_mark_sp;
    Pike_fp->expendible=expendible;
    if(x>=0) mega_apply(APPLY_STACK, x, 0,0);
    UNSETJMP(tmp);
    return x == -2 ? 2 : 1;
  }
}

PMOD_EXPORT void f_call_function(INT32 args)
{
  mega_apply(APPLY_STACK,args,0,0);
}

PMOD_EXPORT void call_handle_error(void)
{
  if (Pike_interpreter.svalue_stack_margin) {
    ONERROR tmp;
    int old_t_flag = t_flag;
    t_flag = 0;
    Pike_interpreter.svalue_stack_margin = 0;
    Pike_interpreter.c_stack_margin = 0;
    SET_ONERROR(tmp,exit_on_error,"Error in handle_error in master object!");
    *(Pike_sp++) = throw_value;
    throw_value.type=T_INT;
    APPLY_MASTER("handle_error", 1);
    pop_stack();
    UNSET_ONERROR(tmp);
    Pike_interpreter.svalue_stack_margin = SVALUE_STACK_MARGIN;
    Pike_interpreter.c_stack_margin = C_STACK_MARGIN;
    t_flag = old_t_flag;
  }
  else {
    free_svalue(&throw_value);
    throw_value.type=T_INT;
  }
}

PMOD_EXPORT int apply_low_safe_and_stupid(struct object *o, INT32 offset)
{
  JMP_BUF tmp;
  struct pike_frame *new_frame=alloc_pike_frame();
  int ret;

  new_frame->next = Pike_fp;
  new_frame->current_object = o;
  new_frame->context=o->prog->inherits[0];
  new_frame->locals = Pike_interpreter.evaluator_stack;
  new_frame->expendible=new_frame->locals;
  new_frame->args = 0;
  new_frame->num_args=0;
  new_frame->num_locals=0;
  new_frame->fun = o->prog->num_identifier_references?o->prog->num_identifier_references-1:0;
  new_frame->pc = 0;
  new_frame->current_storage=o->storage;
  new_frame->context.parent=0;
  Pike_fp = new_frame;

  add_ref(new_frame->current_object);
  add_ref(new_frame->context.prog);

  if(SETJMP(tmp))
  {
    ret=1;
  }else{
    struct svalue **save_mark_sp=Pike_mark_sp;
    int tmp=eval_instruction(o->prog->program + offset);
    EVAL_INSTR_RET_CHECK(tmp);
    Pike_mark_sp=save_mark_sp;
    if(tmp>=0) mega_apply(APPLY_STACK, tmp, 0,0);
    
#ifdef PIKE_DEBUG
    if(Pike_sp<Pike_interpreter.evaluator_stack)
      fatal("Stack error (simple).\n");
#endif
    ret=0;
  }
  UNSETJMP(tmp);

  POP_PIKE_FRAME();

  return ret;
}

PMOD_EXPORT void safe_apply_low2(struct object *o,int fun,int args, int handle_errors)
{
  JMP_BUF recovery;

  Pike_sp-=args;
  free_svalue(& throw_value);
  throw_value.type=T_INT;
  if(SETJMP(recovery))
  {
    if (handle_errors) call_handle_error();
    Pike_sp->u.integer = 0;
    Pike_sp->subtype=NUMBER_NUMBER;
    Pike_sp->type = T_INT;
    Pike_sp++;
  }else{
    ptrdiff_t expected_stack = Pike_sp - Pike_interpreter.evaluator_stack + 1;
    Pike_sp+=args;
    apply_low(o,fun,args);
    if(Pike_sp - Pike_interpreter.evaluator_stack > expected_stack)
      pop_n_elems(Pike_sp - Pike_interpreter.evaluator_stack - expected_stack);
    if(Pike_sp - Pike_interpreter.evaluator_stack < expected_stack)
    {
      Pike_sp->u.integer = 0;
      Pike_sp->subtype=NUMBER_NUMBER;
      Pike_sp->type = T_INT;
      Pike_sp++;
    }
  }
  UNSETJMP(recovery);
}

PMOD_EXPORT void safe_apply_low(struct object *o,int fun,int args)
{
  safe_apply_low2(o, fun, args, 1);
}

PMOD_EXPORT void safe_apply(struct object *o, char *fun ,INT32 args)
{
#ifdef PIKE_DEBUG
  if(!o->prog) fatal("Apply safe on destructed object.\n");
#endif
  safe_apply_low2(o, find_identifier(fun, o->prog), args, 1);
}

PMOD_EXPORT void safe_apply_handler(const char *fun,
				    struct object *handler,
				    struct object *compat,
				    INT32 args)
{
  int i;
  free_svalue(&throw_value);
  throw_value.type = T_INT;
  if (handler && handler->prog &&
      (i = find_identifier(fun, handler->prog)) != -1) {
    safe_apply_low2(handler, i, args, 0);
  } else if (compat && compat->prog &&
	     (i = find_identifier(fun, compat->prog)) != -1) {
    safe_apply_low2(compat, i, args, 0);
  } else {
    struct object *master_obj = master();
    i = find_identifier(fun, master_obj->prog);
    safe_apply_low2(master_obj, i, args, 0);
  }
  if (throw_value.type != T_STRING && throw_value.type != T_INT) {
    free_svalue(&throw_value);
    throw_value.type = T_INT;
  }
}

PMOD_EXPORT void apply_lfun(struct object *o, int fun, int args)
{
#ifdef PIKE_DEBUG
  if(fun < 0 || fun >= NUM_LFUNS)
    fatal("Apply lfun on illegal value!\n");
#endif
  if(!o->prog)
    PIKE_ERROR("destructed object", "Apply on destructed object.\n", Pike_sp, args);

  apply_low(o, (int)FIND_LFUN(o->prog,fun), args);
}

PMOD_EXPORT void apply_shared(struct object *o,
		  struct pike_string *fun,
		  int args)
{
  apply_low(o, find_shared_string_identifier(fun, o->prog), args);
}

PMOD_EXPORT void apply(struct object *o, char *fun, int args)
{
  apply_low(o, find_identifier(fun, o->prog), args);
}


PMOD_EXPORT void apply_svalue(struct svalue *s, INT32 args)
{
  if(s->type==T_INT)
  {
    pop_n_elems(args);
    push_int(0);
  }else{
    ptrdiff_t expected_stack=Pike_sp-args+1 - Pike_interpreter.evaluator_stack;

    strict_apply_svalue(s,args);
    if(Pike_sp > (expected_stack + Pike_interpreter.evaluator_stack))
    {
      pop_n_elems(Pike_sp-(expected_stack + Pike_interpreter.evaluator_stack));
    }
    else if(Pike_sp < (expected_stack + Pike_interpreter.evaluator_stack))
    {
      push_int(0);
    }
#ifdef PIKE_DEBUG
    if(Pike_sp < (expected_stack + Pike_interpreter.evaluator_stack))
      fatal("Stack underflow!\n");
#endif
  }
}

#ifdef PIKE_DEBUG
void slow_check_stack(void)
{
  struct svalue *s,**m;
  struct pike_frame *f;

  debug_check_stack();

  if(Pike_sp > &(Pike_interpreter.evaluator_stack[Pike_stack_size]))
    fatal("Svalue stack overflow. "
	  "(%ld entries on stack, stack_size is %ld entries)\n",
	  PTRDIFF_T_TO_LONG(Pike_sp - Pike_interpreter.evaluator_stack),
	  PTRDIFF_T_TO_LONG(Pike_stack_size));

  if(Pike_mark_sp > &(Pike_interpreter.mark_stack[Pike_stack_size]))
    fatal("Mark stack overflow.\n");

  if(Pike_mark_sp < Pike_interpreter.mark_stack)
    fatal("Mark stack underflow.\n");

  for(s=Pike_interpreter.evaluator_stack;s<Pike_sp;s++) check_svalue(s);

  s=Pike_interpreter.evaluator_stack;
  for(m=Pike_interpreter.mark_stack;m<Pike_mark_sp;m++)
  {
    if(*m < s)
      fatal("Mark stack failure.\n");

    s=*m;
  }

  if(s > &(Pike_interpreter.evaluator_stack[Pike_stack_size]))
    fatal("Mark stack exceeds svalue stack\n");

  for(f=Pike_fp;f;f=f->next)
  {
    if(f->locals)
    {
      if(f->locals < Pike_interpreter.evaluator_stack ||
	f->locals > &(Pike_interpreter.evaluator_stack[Pike_stack_size]))
      fatal("Local variable pointer points to Finspång.\n");

      if(f->args < 0 || f->args > Pike_stack_size)
	fatal("FEL FEL FEL! HELP!! (corrupted pike_frame)\n");
    }
  }
}
#endif

PMOD_EXPORT void custom_check_stack(ptrdiff_t amount, const char *fmt, ...)
{
  if (low_stack_check(amount)) {
    va_list args;
    va_start(args, fmt);
    va_error(fmt, args);
  }
}

PMOD_EXPORT void cleanup_interpret(void)
{
#ifdef PIKE_DEBUG
  int e;
#endif

  while(Pike_fp)
    POP_PIKE_FRAME();

#ifdef PIKE_DEBUG
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
  if(!Pike_interpreter.evaluator_stack_malloced)
  {
    munmap((char *)Pike_interpreter.evaluator_stack, Pike_stack_size*sizeof(struct svalue));
    Pike_interpreter.evaluator_stack=0;
  }
  if(!Pike_interpreter.mark_stack_malloced)
  {
    munmap((char *)Pike_interpreter.mark_stack, Pike_stack_size*sizeof(struct svalue *));
    Pike_interpreter.mark_stack=0;
  }
#endif

  if(Pike_interpreter.evaluator_stack) free((char *)Pike_interpreter.evaluator_stack);
  if(Pike_interpreter.mark_stack) free((char *)Pike_interpreter.mark_stack);

  Pike_interpreter.mark_stack=0;
  Pike_interpreter.evaluator_stack=0;
  Pike_interpreter.mark_stack_malloced=0;
  Pike_interpreter.evaluator_stack_malloced=0;
}

void really_clean_up_interpret(void)
{
#ifdef DO_PIKE_CLEANUP
#if 0
  struct pike_frame_block *p;
  int e;
  for(p=pike_frame_blocks;p;p=p->next)
    for(e=0;e<128;e++)
      debug_malloc_dump_references( p->x + e);
#endif
  free_all_pike_frame_blocks();
#endif
}

