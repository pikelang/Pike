/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: interpret.c,v 1.299 2004/08/19 15:00:50 grubba Exp $
*/

#include "global.h"
RCSID("$Id: interpret.c,v 1.299 2004/08/19 15:00:50 grubba Exp $");
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
#include "pikecode.h"

#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

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
/* Observed in 7.1: 40 was enough, 30 wasn't. */
#define SVALUE_STACK_MARGIN (100 + LOW_SVALUE_STACK_MARGIN)
/* Observed in 7.4: 11000 was enough, 10000 wasn't. */
#define C_STACK_MARGIN (20000 + LOW_C_STACK_MARGIN)

/* Another extra margin to use while dumping the raw error in
 * exit_on_error, so that backtrace_frame._sprintf can be called
 * then. */
#define LOW_SVALUE_STACK_MARGIN 20
#define LOW_C_STACK_MARGIN 500

#ifdef HAVE_COMPUTED_GOTO
PIKE_OPCODE_T *fcode_to_opcode = NULL;
struct op_2_f *opcode_to_fcode = NULL;
#endif /* HAVE_COMPUTED_GOTO */

PMOD_EXPORT const char Pike_check_stack_errmsg[] =
  "Svalue stack overflow. "
  "(%ld of %ld entries on stack, needed %ld more entries)\n";
PMOD_EXPORT const char Pike_check_mark_stack_errmsg[] =
  "Mark stack overflow.\n";
PMOD_EXPORT const char Pike_check_c_stack_errmsg[] =
  "C stack overflow.\n";
#ifdef PIKE_DEBUG
PMOD_EXPORT const char msg_stack_error[] =
  "Stack error.\n";
PMOD_EXPORT const char msg_pop_neg[] =
  "Popping negative number of args.... (%"PRINTPTRDIFFT"d) \n";
#endif


#ifdef PIKE_DEBUG
static char trace_buffer[2000];
#endif

#ifdef INTERNAL_PROFILING
PMOD_EXPORT unsigned long evaluator_callback_calls = 0;
#endif


/* Pike_sp points to first unused value on stack
 * (much simpler than letting it point at the last used value.)
 */
PMOD_EXPORT struct Pike_interpreter Pike_interpreter;
PMOD_EXPORT int Pike_stack_size = EVALUATOR_STACK_SIZE;

static void trace_return_value(void);
static void do_trace_call(INT32);

void gdb_stop_here(void)
{
  ;
}


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
    Pike_fatal("Mark stack underflow!\n");
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

/* Execute Pike code starting at pc.
 *
 * Called once with NULL to initialize tables.
 *
 * Returns 0 if pc is NULL.
 *
 * Returns -1 if the code terminated due to a RETURN.
 *
 * Returns -2 if the code terminated due to EXIT_CATCH or ESCAPE_CATCH.
 */
static int eval_instruction(PIKE_OPCODE_T *pc);

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

use_malloc:

#else
  Pike_interpreter.evaluator_stack=0;
  Pike_interpreter.mark_stack=0;
#endif

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
#if defined(HAVE_COMPUTED_GOTO) || defined(PIKE_USE_MACHINE_CODE)
  {
    static int tables_need_init=1;
    if(tables_need_init) {
      /* Initialize the fcode_to_opcode table / jump labels. */
      eval_instruction(NULL);
#if defined(PIKE_USE_MACHINE_CODE) && !defined(PIKE_DEBUG)
      /* Simple operator opcodes... */
#define SET_INSTR_ADDRESS(X, Y)	(instrs[(X)-F_OFFSET].address = (void *)Y)
      SET_INSTR_ADDRESS(F_COMPL,		o_compl);
      SET_INSTR_ADDRESS(F_LSH,			o_lsh);
      SET_INSTR_ADDRESS(F_RSH,			o_rsh);
      SET_INSTR_ADDRESS(F_SUBTRACT,		o_subtract);
      SET_INSTR_ADDRESS(F_AND,			o_and);
      SET_INSTR_ADDRESS(F_OR,			o_or);
      SET_INSTR_ADDRESS(F_XOR,			o_xor);
      SET_INSTR_ADDRESS(F_MULTIPLY,		o_multiply);
      SET_INSTR_ADDRESS(F_DIVIDE,		o_divide);
      SET_INSTR_ADDRESS(F_MOD,			o_mod);
      SET_INSTR_ADDRESS(F_CAST,			f_cast);
      SET_INSTR_ADDRESS(F_CAST_TO_INT,		o_cast_to_int);
      SET_INSTR_ADDRESS(F_CAST_TO_STRING,	o_cast_to_string);
      SET_INSTR_ADDRESS(F_RANGE,		o_range);
      SET_INSTR_ADDRESS(F_SSCANF,		o_sscanf);
#endif /* PIKE_USE_MACHINE_CODE && !PIKE_DEBUG */
      tables_need_init=0;
    }
  }
#endif /* HAVE_COMPUTED_GOTO || PIKE_USE_MACHINE_CODE */
}


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
      if (lval[1].type == T_LVALUE) {
	low_object_index_no_free(to, lval->u.object, lval[1].u.integer);
      } else {
	object_index_no_free(to, lval->u.object, lval+1);
      }
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
      if(SAFE_IS_ZERO(lval))
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
    if (lval[1].type == T_LVALUE) {
      object_low_set_index(lval->u.object, lval[1].u.integer, from);
    } else {
      object_set_index(lval->u.object, lval+1, from);
    }
    break;

  case T_ARRAY:
    simple_set_index(lval->u.array, lval+1, from);
    break;

  case T_MAPPING:
    mapping_insert(lval->u.mapping, lval+1, from);
    break;

  case T_MULTISET:
    if(UNSAFE_IS_ZERO(from))
      multiset_delete(lval->u.multiset, lval+1);
    else
      multiset_insert(lval->u.multiset, lval+1);
    break;
    
  default:
   if(SAFE_IS_ZERO(lval))
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
      if(SAFE_IS_ZERO(lval))
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
	  p->parent ? p->parent->id : -1);
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
	      last->prog->parent ? last->prog->parent->id : -1,
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

static struct inherit dummy_inherit
#ifdef PIKE_DEBUG
  = {-4711, -4711, -4711, -4711, (size_t) -4711, -4711, NULL, NULL, NULL}
#endif
;

PMOD_EXPORT void find_external_context(struct external_variable_context *loc,
				       int depth)
{
  struct program *p;

  TRACE((4, "-find_external_context(%d, inherit=%ld)\n", depth,
	 DO_NOT_WARN((long)(loc->o->prog ? loc->inherit - loc->o->prog->inherits : 0))));

#ifdef PIKE_DEBUG
  if(!loc->o)
    Pike_fatal("No object\n");
#endif

  if (!(p = loc->o->prog)) {
    /* magic fallback */
    p = get_program_for_object_being_destructed(loc->o);
    if(!p)
    {
      Pike_error("Cannot access parent of destructed object.\n");
    }
  }

#ifdef DEBUG_MALLOC
  if (loc->o->refs == 0x55555555) {
    fprintf(stderr, "The object %p has been zapped!\n", loc->o);
    describe(p);
    Pike_fatal("Object zapping detected.\n");
  }
  if (p && p->refs == 0x55555555) {
    fprintf(stderr, "The program %p has been zapped!\n", p);
    describe(p);
    fprintf(stderr, "Which taken from the object %p\n", loc->o);
    describe(loc->o);
    Pike_fatal("Looks like the program %p has been zapped!\n", p);
  }
#endif /* DEBUG_MALLOC */

  while(--depth>=0)
  {
    struct inherit *inh = loc->inherit;

    if (!p)
      Pike_error("Attempting to access parent of destructed object.\n");

#ifdef PIKE_DEBUG  
    if(t_flag>8 && p)
      my_describe_inherit_structure(p);
#endif

    TRACE((4,"-   i->parent_offset=%d i->parent_identifier=%d\n",
	   inh->parent_offset,
	   inh->parent_identifier));

      TRACE((4,"-   o->parent_identifier=%d inherit->identifier_level=%d\n",
	     (p->flags & PROGRAM_USES_PARENT) ?
	     LOW_PARENT_INFO(loc->o, p)->parent_identifier : -1,
	   inh->identifier_level));

    switch(inh->parent_offset)
    {
      default:
	{
	  int my_level = inh->inherit_level;
#ifdef PIKE_DEBUG
	  if(!my_level)
	    Pike_fatal("Gahhh! inherit level zero in wrong place!\n");
#endif
	  while(loc->inherit->inherit_level >= my_level)
	  {
	    TRACE((5,"-   inherit-- (%d >= %d)\n",loc->inherit->inherit_level, my_level));
	    loc->inherit--;
	  }

	  find_external_context(loc, inh->parent_offset);
	  loc->parent_identifier =
	    inh->parent_identifier+
	    loc->inherit->identifier_level;
	}
	break;

      case -17:
	TRACE((5,"-   Following inherit->parent\n"));
	loc->parent_identifier=inh->parent_identifier;
	loc->o=inh->parent;
	break;

      case -18:
	TRACE((5,"-   Following o->parent\n"));

#ifdef PIKE_DEBUG
	if (!(p->flags & PROGRAM_USES_PARENT))
	  Pike_error ("Attempting to access parent of object without parent pointer.\n");
#endif

	loc->parent_identifier=LOW_PARENT_INFO(loc->o,p)->parent_identifier;
	loc->o=LOW_PARENT_INFO(loc->o,p)->parent;
	break;
    }
    
    if(!loc->o)
      Pike_error("Parent was lost during cloning.\n");
    
    p = loc->o->prog;

#ifdef DEBUG_MALLOC
    if (loc->o->refs == 0x55555555) {
      fprintf(stderr, "The object %p has been zapped!\n", loc->o);
      describe(p);
      Pike_fatal("Object zapping detected.\n");
    }
    if (p && p->refs == 0x55555555) {
      fprintf(stderr, "The program %p has been zapped!\n", p);
      describe(p);
      fprintf(stderr, "Which taken from the object %p\n", loc->o);
      describe(loc->o);
      Pike_fatal("Looks like the program %p has been zapped!\n", p);
    }
#endif /* DEBUG_MALLOC */

    if (p) {
#ifdef PIKE_DEBUG
      if(loc->parent_identifier < 0 ||
	 loc->parent_identifier > p->num_identifier_references)
	Pike_fatal("Identifier out of range, loc->parent_identifer=%d!\n",
		   loc->parent_identifier);
#endif
      loc->inherit=INHERIT_FROM_INT(p, loc->parent_identifier);
    }
    else
      /* Return a valid pointer to a dummy inherit for the convenience
       * of the caller. Identifier offsets will be bogus but it'll
       * never get to that since the object is destructed. */
      loc->inherit = &dummy_inherit;

#ifdef PIKE_DEBUG  
    if(t_flag>28)
      my_describe_inherit_structure(p);
#endif

    TRACE((5,"-   Parent identifier = %d (%s), inherit # = %ld\n",
	   loc->parent_identifier,
	   p ? ID_FROM_INT(p, loc->parent_identifier)->name->str : "N/A",
	   p ? DO_NOT_WARN((long)(loc->inherit - p->inherits)) : -1));
    
#ifdef DEBUG_MALLOC
    if (p && loc->inherit->storage_offset == 0x55555555) {
      fprintf(stderr, "The inherit %p has been zapped!\n", loc->inherit);
      debug_malloc_dump_references(loc->inherit,0,2,0);
      fprintf(stderr, "It was extracted from the program %p %d\n", p, loc->parent_identifier);
      describe(p);
      fprintf(stderr, "Which was in turn taken from the object %p\n", loc->o);
      describe(loc->o);
      Pike_fatal("Looks like the program %p has been zapped!\n", p);
    }
#endif /* DEBUG_MALLOC */
  }

  TRACE((4,"--find_external_context: parent_id=%d (%s)\n",
	 loc->parent_identifier,
	 p ? ID_FROM_INT(p,loc->parent_identifier)->name->str : "N/A"
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
  PIKE_INSTR_T instruction;
  INT32 arg,arg2;
  struct program *program;
  PIKE_OPCODE_T *pc;
#ifdef _REENTRANT
  struct object *thread_id;
#endif
  ptrdiff_t stack;
  ptrdiff_t mark_stack;
};

struct backlog backlog[BACKLOG];
int backlogp=BACKLOG-1;

static inline void low_debug_instr_prologue (PIKE_INSTR_T instr)
{
  if(t_flag > 2)
  {
    char *file, *f;
    struct pike_string *filep;
    INT32 linep;

    filep = get_line(Pike_fp->pc,Pike_fp->context.prog,&linep);
    file = filep->str;
    while((f=STRCHR(file,'/')))
      file=f+1;
    fprintf(stderr,"- %s:%4ld:(%lx): %-25s %4ld %4ld\n",
	    file,(long)linep,
	    DO_NOT_WARN((long)(Pike_fp->pc - Pike_fp->context.prog->program)),
#ifdef HAVE_COMPUTED_GOTO
	    get_opcode_name(instr),
#else /* !HAVE_COMPUTED_GOTO */
	    get_f_name(instr + F_OFFSET),
#endif /* HAVE_COMPUTED_GOTO */
	    DO_NOT_WARN((long)(Pike_sp-Pike_interpreter.evaluator_stack)),
	    DO_NOT_WARN((long)(Pike_mark_sp-Pike_interpreter.mark_stack)));
    free_string(filep);
  }

#ifdef HAVE_COMPUTED_GOTO
  if (instr) 
    ADD_RUNNED(instr);
  else
    Pike_fatal("NULL Instruction!\n");
#else /* !HAVE_COMPUTED_GOTO */
  if(instr + F_OFFSET < F_MAX_OPCODE) 
    ADD_RUNNED(instr + F_OFFSET);
#endif /* HAVE_COMPUTED_GOTO */

  if(d_flag)
  {
    backlogp++;
    if(backlogp >= BACKLOG) backlogp=0;

    if(backlog[backlogp].program)
      free_program(backlog[backlogp].program);

    backlog[backlogp].program=Pike_fp->context.prog;
    add_ref(Pike_fp->context.prog);
    backlog[backlogp].instruction=instr;
    backlog[backlogp].pc = Pike_fp->pc;
    backlog[backlogp].stack = Pike_sp - Pike_interpreter.evaluator_stack;
    backlog[backlogp].mark_stack = Pike_mark_sp - Pike_interpreter.mark_stack;
#ifdef _REENTRANT
    backlog[backlogp].thread_id=Pike_interpreter.thread_id;
#endif

#ifdef _REENTRANT
    CHECK_INTERPRETER_LOCK();
    if(OBJ2THREAD(Pike_interpreter.thread_id)->state.thread_id !=
       Pike_interpreter.thread_id)
      Pike_fatal("Arglebargle glop glyf, thread swap problem!\n");

    if(d_flag>1 && thread_for_id(th_self()) != Pike_interpreter.thread_id)
      Pike_fatal("thread_for_id() (or Pike_interpreter.thread_id) failed in interpreter.h! %p != %p\n",thread_for_id(th_self()),Pike_interpreter.thread_id);
#endif

    Pike_sp[0].type=99; /* an invalid type */
    Pike_sp[1].type=99;
    Pike_sp[2].type=99;
    Pike_sp[3].type=99;
      
    if(Pike_sp<Pike_interpreter.evaluator_stack ||
       Pike_mark_sp < Pike_interpreter.mark_stack || Pike_fp->locals>Pike_sp)
      Pike_fatal("Stack error (generic) sp=%p/%p mark_sp=%p/%p locals=%p.\n",
		 Pike_sp,
		 Pike_interpreter.evaluator_stack,
		 Pike_mark_sp,
		 Pike_interpreter.mark_stack,
		 Pike_fp->locals);
      
    if(Pike_mark_sp > Pike_interpreter.mark_stack+Pike_stack_size)
      Pike_fatal("Mark Stack error (overflow).\n");


    if(Pike_mark_sp < Pike_interpreter.mark_stack)
      Pike_fatal("Mark Stack error (underflow).\n");

    if(Pike_sp > Pike_interpreter.evaluator_stack+Pike_stack_size)
      Pike_fatal("stack error (overflow).\n");
      
    if(/* Pike_fp->fun>=0 && */ Pike_fp->current_object->prog &&
       Pike_fp->locals+Pike_fp->num_locals > Pike_sp)
      Pike_fatal("Stack error (stupid!).\n");

    if(Pike_interpreter.recoveries &&
       (Pike_sp-Pike_interpreter.evaluator_stack <
	Pike_interpreter.recoveries->stack_pointer))
      Pike_fatal("Stack error (underflow).\n");

    if(Pike_mark_sp > Pike_interpreter.mark_stack &&
       Pike_mark_sp[-1] > Pike_sp)
      Pike_fatal("Stack error (underflow?)\n");
      
    if(d_flag > 9) do_debug();

    debug_malloc_touch(Pike_fp->current_object);
    switch(d_flag)
    {
      default:
      case 3:
	check_object(Pike_fp->current_object);
	/*	  break; */

      case 2:
	check_object_context(Pike_fp->current_object,
			     Pike_fp->context.prog,
			     Pike_fp->current_object->storage+
			     Pike_fp->context.storage_offset);
      case 1:
      case 0:
	break;
    }
  }
}

#define DEBUG_LOG_ARG(ARG)					\
  (backlog[backlogp].arg = (ARG),				\
   (t_flag>3 ?							\
    sprintf(trace_buffer, "-    Arg = %ld\n",			\
	    (long) backlog[backlogp].arg),			\
    write_to_stderr(trace_buffer,strlen(trace_buffer)) : 0))

#define DEBUG_LOG_ARG2(ARG2)					\
  (backlog[backlogp].arg2 = (ARG2),				\
   (t_flag>3 ?							\
    sprintf(trace_buffer, "-    Arg2 = %ld\n",			\
	    (long) backlog[backlogp].arg2),			\
    write_to_stderr(trace_buffer,strlen(trace_buffer)) : 0))

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
      struct pike_string *file;
      INT32 line;

#ifdef _REENTRANT
      if(thread != backlog[e].thread_id)
      {
	fprintf(stderr,"[Thread swap, Pike_interpreter.thread_id=%p]\n",backlog[e].thread_id);
	thread = backlog[e].thread_id;
      }
#endif

      file = get_line(backlog[e].pc,backlog[e].program, &line);
#ifdef HAVE_COMPUTED_GOTO
      fprintf(stderr,"%s:%ld:(%"PRINTPTRDIFFT"d): %s",
	      file->str,
	      (long)line,
	      backlog[e].pc - backlog[e].program->program,
	      get_opcode_name(backlog[e].instruction));
#else /* !HAVE_COMPUTED_GOTO */
      if(backlog[e].instruction+F_OFFSET > F_MAX_OPCODE)
      {
	fprintf(stderr,"%s:%ld:(%"PRINTPTRDIFFT"d): ILLEGAL INSTRUCTION %d\n",
		file->str,
		(long)line,
		backlog[e].pc - backlog[e].program->program,
		backlog[e].instruction + F_OFFSET);
	free_string(file);
	continue;
      }

      fprintf(stderr,"%s:%ld:(%"PRINTPTRDIFFT"d): %s",
	      file->str,
	      (long)line,
	      backlog[e].pc - backlog[e].program->program,
	      low_get_f_name(backlog[e].instruction + F_OFFSET, backlog[e].program));
      if(instrs[backlog[e].instruction].flags & I_HASARG2)
      {
	fprintf(stderr,"(%ld,%ld)",
		(long)backlog[e].arg,
		(long)backlog[e].arg2);
      }
      else if(instrs[backlog[e].instruction].flags & I_JUMP)
      {
	fprintf(stderr,"(%+ld)", (long)backlog[e].arg);
      }
      else if(instrs[backlog[e].instruction].flags & I_HASARG)
      {
	fprintf(stderr,"(%ld)", (long)backlog[e].arg);
      }
      fprintf(stderr," %ld, %ld\n",
	      DO_NOT_WARN((long)backlog[e].stack),
	      DO_NOT_WARN((long)backlog[e].mark_stack));
#endif /* HAVE_COMPUTED_GOTO */
      free_string(file);
    }
  }while(e!=backlogp);
}

#else  /* PIKE_DEBUG */

#define DEBUG_LOG_ARG(arg) 0
#define DEBUG_LOG_ARG2(arg2) 0

#endif	/* !PIKE_DEBUG */

static int o_catch(PIKE_OPCODE_T *pc);


#ifdef PIKE_DEBUG
#define EVAL_INSTR_RET_CHECK(x)						\
  if (x == -2)								\
    Pike_fatal("Return value -2 from eval_instruction is not handled here.\n"\
	  "Probable cause: F_ESCAPE_CATCH outside catch block.\n")
#else
#define EVAL_INSTR_RET_CHECK(x)
#endif


#ifdef PIKE_USE_MACHINE_CODE

/* Labels to jump to to cause eval_instruction to return */
/* FIXME: Replace these with assembler lables */
void *do_inter_return_label = NULL;
void *do_escape_catch_label;
void *dummy_label = NULL;

#ifndef DEF_PROG_COUNTER
#define DEF_PROG_COUNTER
#endif /* !DEF_PROG_COUNTER */

#ifndef CALL_MACHINE_CODE
#define CALL_MACHINE_CODE(pc)					\
  do {								\
    /* The test is needed to get the labels to work... */	\
    if (pc) {							\
      /* No extra setup needed!					\
       */							\
      return ((int (*)(void))(pc))();				\
    }								\
  } while(0)
#endif /* !CALL_MACHINE_CODE */

#if defined(OPCODE_INLINE_BRANCH) || defined(INS_F_JUMP) || \
    defined(INS_F_JUMP_WITH_ARG) || defined(INS_F_JUMP_WITH_TWO_ARGS)
/* Intended to be called from machine code on backward branch jumps,
 * to ensure thread switching. */
void branch_check_threads_etc()
{
  fast_check_threads_etc (6);
}
#endif

#ifdef PIKE_DEBUG

static void debug_instr_prologue (PIKE_INSTR_T instr)
{
  low_debug_instr_prologue (instr);
}

#define DEBUG_PROLOGUE(OPCODE, EXTRA) do {				\
    if (d_flag || t_flag > 2) {						\
      debug_instr_prologue ((OPCODE) - F_OFFSET);			\
      EXTRA;								\
    }									\
  } while (0)

/* The following are intended to be called directly from generated
 * machine code. */
void simple_debug_instr_prologue_0 (PIKE_INSTR_T instr)
{
  if (d_flag || t_flag > 2)
    low_debug_instr_prologue (instr);
}
void simple_debug_instr_prologue_1 (PIKE_INSTR_T instr, INT32 arg)
{
  if (d_flag || t_flag > 2) {
    low_debug_instr_prologue (instr);
    DEBUG_LOG_ARG (arg);
  }
}
void simple_debug_instr_prologue_2 (PIKE_INSTR_T instr, INT32 arg1, INT32 arg2)
{
  if (d_flag || t_flag > 2) {
    low_debug_instr_prologue (instr);
    DEBUG_LOG_ARG (arg1);
    DEBUG_LOG_ARG2 (arg2);
  }
}

#else  /* !PIKE_DEBUG */
#define DEBUG_PROLOGUE(OPCODE, EXTRA) do {} while (0)
#endif        /* !PIKE_DEBUG */

#define OPCODE0(O,N,F,C) \
void PIKE_CONCAT(opcode_,O)(void) { \
  DEF_PROG_COUNTER; \
  DEBUG_PROLOGUE (O, ;);						\
C }

#define OPCODE1(O,N,F,C) \
void PIKE_CONCAT(opcode_,O)(INT32 arg1) {\
  DEF_PROG_COUNTER; \
  DEBUG_PROLOGUE (O, DEBUG_LOG_ARG (arg1));				\
C }


#define OPCODE2(O,N,F,C) \
void PIKE_CONCAT(opcode_,O)(INT32 arg1,INT32 arg2) { \
  DEF_PROG_COUNTER; \
  DEBUG_PROLOGUE (O, DEBUG_LOG_ARG (arg1); DEBUG_LOG_ARG2 (arg2));	\
C }

#ifdef OPCODE_INLINE_BRANCH
#define TEST_OPCODE0(O,N,F,C) \
int PIKE_CONCAT(test_opcode_,O)(void) { \
    int branch_taken = 0;	\
    DEF_PROG_COUNTER; \
    DEBUG_PROLOGUE (O, ;);						\
    C; \
    return branch_taken; \
  }

#define TEST_OPCODE1(O,N,F,C) \
int PIKE_CONCAT(test_opcode_,O)(INT32 arg1) {\
    int branch_taken = 0;	\
    DEF_PROG_COUNTER; \
    DEBUG_PROLOGUE (O, DEBUG_LOG_ARG (arg1));				\
    C; \
    return branch_taken; \
  }


#define TEST_OPCODE2(O,N,F,C) \
int PIKE_CONCAT(test_opcode_,O)(INT32 arg1, INT32 arg2) { \
    int branch_taken = 0;	\
    DEF_PROG_COUNTER; \
    DEBUG_PROLOGUE (O, DEBUG_LOG_ARG (arg1); DEBUG_LOG_ARG2 (arg2));	\
    C; \
    return branch_taken; \
  }

#define DO_BRANCH()	(branch_taken = -1)
#define DONT_BRANCH()	(branch_taken = 0)
#else /* !OPCODE_INLINE_BRANCH */
#define TEST_OPCODE0	OPCODE0
#define TEST_OPCODE1	OPCODE1
#define TEST_OPCODE2	OPCODE2
#endif /* OPCODE_INLINE_BRANCH */

#define OPCODE0_JUMP(O,N,F,C) OPCODE0(O,N,F,C)
#define OPCODE1_JUMP(O,N,F,C) OPCODE1(O,N,F,C)
#define OPCODE2_JUMP(O,N,F,C) OPCODE2(O,N,F,C)

#define OPCODE0_TAIL(O,N,F,C) OPCODE0(O,N,F,C)
#define OPCODE1_TAIL(O,N,F,C) OPCODE1(O,N,F,C)
#define OPCODE2_TAIL(O,N,F,C) OPCODE2(O,N,F,C)

#define OPCODE0_TAILJUMP(O,N,F,C) OPCODE0(O,N,F,C)
#define OPCODE1_TAILJUMP(O,N,F,C) OPCODE1(O,N,F,C)
#define OPCODE2_TAILJUMP(O,N,F,C) OPCODE2(O,N,F,C)

#define OPCODE0_RETURN(O,N,F,C) OPCODE0(O,N,F,C)
#define OPCODE1_RETURN(O,N,F,C) OPCODE1(O,N,F,C)
#define OPCODE2_RETURN(O,N,F,C) OPCODE2(O,N,F,C)

#define OPCODE0_RETURNJUMP(O,N,F,C) OPCODE0(O,N,F,C)
#define OPCODE1_RETURNJUMP(O,N,F,C) OPCODE1(O,N,F,C)
#define OPCODE2_RETURNJUMP(O,N,F,C) OPCODE2(O,N,F,C)

/* BRANCH opcodes only generate code for the test,
 * so that the branch instruction can be inlined.
 */
#define OPCODE0_BRANCH(O,N,F,C) TEST_OPCODE0(O,N,F,C)
#define OPCODE1_BRANCH(O,N,F,C) TEST_OPCODE1(O,N,F,C)
#define OPCODE2_BRANCH(O,N,F,C) TEST_OPCODE2(O,N,F,C)
#define OPCODE0_TAILBRANCH(O,N,F,C) TEST_OPCODE0(O,N,F,C)
#define OPCODE1_TAILBRANCH(O,N,F,C) TEST_OPCODE1(O,N,F,C)
#define OPCODE2_TAILBRANCH(O,N,F,C) TEST_OPCODE2(O,N,F,C)

#define OPCODE0_ALIAS(O,N,F,C)
#define OPCODE1_ALIAS(O,N,F,C)
#define OPCODE2_ALIAS(O,N,F,C)

#undef HAVE_COMPUTED_GOTO

#ifdef __GNUC__

/* Define the program counter if necessary. */
DEF_PROG_COUNTER;

#ifdef PIKE_DEBUG
/* Note: The debug code is extracted, to keep the frame size constant. */
static int eval_instruction_low(PIKE_OPCODE_T *pc);
#endif /* PIKE_DEBUG */

static int eval_instruction(PIKE_OPCODE_T *pc)
#ifdef PIKE_DEBUG
{
  if (t_flag > 5 && pc) {
    int i;
    fprintf(stderr, "Calling code at %p:\n", pc);
#ifdef PIKE_OPCODE_ALIGN
    if (((INT32)pc) % PIKE_OPCODE_ALIGN) {
      Pike_fatal("Odd offset!\n");
    }
#endif /* PIKE_OPCODE_ALIGN */
    for (i=0; i < 16; i+=4) {
      fprintf(stderr,
	      "  0x%08x 0x%08x 0x%08x 0x%08x\n",
	      ((int *)pc)[i],
	      ((int *)pc)[i+1],
	      ((int *)pc)[i+2],
	      ((int *)pc)[i+3]);
    }
  }
  return eval_instruction_low(pc);
}

static int eval_instruction_low(PIKE_OPCODE_T *pc)
#endif /* PIKE_DEBUG */
{
  if(pc == NULL) {

    if(do_inter_return_label != NULL)
      Pike_fatal("eval_instruction called with NULL (twice).\n");

    do_inter_return_label = && inter_return_label;
    do_escape_catch_label = && inter_escape_catch_label;

    /* Trick optimizer */
    if(!dummy_label)
      return 0;
  }

  CALL_MACHINE_CODE(pc);

  /* This code is never reached, but will
   * prevent gcc from optimizing the labels below too much
   */

  fprintf(stderr,"We have reached the end of the world!\n");
  goto *dummy_label;

  /* %%esp will be slightly buggered after
   * returning from the function code (8 bytes off), but that
   * should not matter to these return statements. -Hubbe
   */

 inter_escape_catch_label: return -2;
 inter_return_label: return -1;
}

#endif /* __GNUC__ */

#ifndef SET_PROG_COUNTER
#define SET_PROG_COUNTER(X)	(PROG_COUNTER=(X))
#endif /* SET_PROG_COUNTER */

#undef DONE
#undef FETCH
#undef INTER_RETURN
#undef INTER_ESCAPE_CATCH

#define DONE return
#define FETCH
#define INTER_RETURN {SET_PROG_COUNTER(do_inter_return_label);DONE;}
#define INTER_ESCAPE_CATCH {SET_PROG_COUNTER(do_escape_catch_label);DONE;}

#include "interpret_functions_fixed.h"


#else /* PIKE_USE_MACHINE_CODE */


#ifndef SET_PROG_COUNTER
#define SET_PROG_COUNTER(X)	(PROG_COUNTER=(X))
#endif /* SET_PROG_COUNTER */

#ifdef HAVE_COMPUTED_GOTO
int lookup_sort_fun(const void *a, const void *b)
{
  return (int)(((ptrdiff_t)((struct op_2_f *)a)->opcode) -
	       ((ptrdiff_t)((struct op_2_f *)b)->opcode));
}
#endif /* HAVE_COMPUTED_GOTO */

/* NOTE: Due to the implementation of computed goto,
 *       interpreter.h may only be included once.
 */
#if defined(PIKE_DEBUG) && !defined(HAVE_COMPUTED_GOTO)
#define eval_instruction eval_instruction_with_debug
#include "interpreter_debug.h"

#undef eval_instruction
#define eval_instruction eval_instruction_without_debug

#undef PIKE_DEBUG
#undef NDEBUG
#undef DO_IF_DEBUG
#define DO_IF_DEBUG(X)
#define print_return_value()
#include "interpreter.h"

#define PIKE_DEBUG
#define NDEBUG
#undef DO_IF_DEBUG
#define DO_IF_DEBUG(X) X
#undef print_return_value

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


#endif /* PIKE_USE_MACHINE_CODE */

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
  struct pike_string *filep = NULL;
  char *file, *s;
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
    filep = get_line(Pike_fp->pc,Pike_fp->context.prog,&linep);
    file = filep->str;
    while((f=STRCHR(file,'/')))
      file=f+1;
  }else{
    linep=0;
    file="-";
  }
  fprintf(stderr,"- %s:%4ld: %s\n",file,(long)linep,s);
  if (filep) {
    free_string(filep);
  }
  free(s);
}


#undef INIT_BLOCK
#define INIT_BLOCK(X) do {			\
  X->refs=1;					\
  X->flags=0; 					\
  X->scope=0;					\
  DO_IF_SECURITY( if(CURRENT_CREDS) {		\
    add_ref(X->current_creds=CURRENT_CREDS);	\
  } else {					\
    X->current_creds = 0;			\
  })						\
}while(0)

#undef EXIT_BLOCK
#define EXIT_BLOCK(X) do {						\
  free_object(X->current_object);					\
  if(X->context.prog) free_program(X->context.prog);			\
  if(X->context.parent) free_object(X->context.parent);			\
  if(X->scope) free_pike_scope(X->scope);				\
  DO_IF_SECURITY( if(X->current_creds) {				\
    free_object(X->current_creds);					\
  })									\
  DO_IF_DEBUG(								\
  if(X->flags & PIKE_FRAME_MALLOCED_LOCALS)				\
  Pike_fatal("Pike frame is not supposed to have malloced locals here!\n"));	\
 									\
  DO_IF_DMALLOC(							\
    X->context.prog=0;							\
    X->context.parent=0;						\
    X->context.name=0;							\
    X->scope=0;								\
    X->current_object=0;						\
    X->flags=0;								\
    X->expendible=0;							\
    X->locals=0;							\
    DO_IF_SECURITY( X->current_creds=0; )				\
 )									\
}while(0)

BLOCK_ALLOC_FILL_PAGES(pike_frame, 4)


void really_free_pike_scope(struct pike_frame *scope)
{
  if(scope->flags & PIKE_FRAME_MALLOCED_LOCALS)
  {
    free_mixed_svalues(scope->locals,scope->num_locals);
    free((char *)(scope->locals));
#ifdef PIKE_DEBUG
    scope->flags&=~PIKE_FRAME_MALLOCED_LOCALS;
#endif
  }
  really_free_pike_frame(scope);
}

/* Apply a function.
 *
 * Application types:
 *
 *   APPLY_STACK:         Apply Pike_sp[-args] with args-1 arguments.
 *
 *   APPLY_SVALUE:        Apply the svalue at arg1, and adjust the stack
 *                        to leave a return value.
 *
 *   APPLY_SVALUE_STRICT: Apply the svalue at arg1, and don't adjust the
 *                        stack for functions that return void.
 *
 *   APPLY_LOW:		  Apply function #arg2 in object arg1.
 *
 * Return values:
 *
 *   Returns zero if the function was invalid or has been executed.
 *
 *   Returns one if a frame has been set up to start the function
 *   with eval_instruction(Pike_fp->pc - ENTRY_PROLOGUE_SIZE). After
 *   eval_instruction() is done the frame needs to be removed by a call
 *   to low_return() or low_return_pop().
 */
int low_mega_apply(enum apply_type type, INT32 args, void *arg1, void *arg2)
{
  struct object *o;
  struct pike_frame *scope=0;
  ptrdiff_t fun;
  struct svalue *save_sp=Pike_sp-args;

#if defined(PIKE_DEBUG) && defined(_REENTRANT)
  if(d_flag)
    {
      THREAD_T self = th_self();

      CHECK_INTERPRETER_LOCK();

      if( Pike_interpreter.thread_id && !th_equal( OBJ2THREAD(Pike_interpreter.thread_id)->id, self) )
	Pike_fatal("Current thread is wrong.\n");
	
      if(thread_for_id(th_self()) != Pike_interpreter.thread_id)
	Pike_fatal("thread_for_id() (or Pike_interpreter.thread_id) failed in mega_apply! "
	      "%p != %p\n", thread_for_id(self), Pike_interpreter.thread_id);
    }
#endif

  switch(type)
  {
  case APPLY_STACK:
    if(!args)
      PIKE_ERROR("`()", "Too few arguments (apply stack).\n", Pike_sp, 0);
    args--;
    arg1=(void *)(Pike_sp-args-1);

  case APPLY_SVALUE:
  case APPLY_SVALUE_STRICT:
  apply_svalue:
  {
    struct svalue *s=(struct svalue *)arg1;
    switch(s->type)
    {
    case T_INT:
      if (!s->u.integer) {
	PIKE_ERROR("0", "Attempt to call the NULL-value\n", Pike_sp, args);
      } else {
	Pike_error("Attempt to call the value %"PRINTPIKEINT"d\n", 
		   s->u.integer);
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
	check_threads_etc();
	(*(s->u.efun->function))(args);

#ifdef PIKE_DEBUG
	s->u.efun->runs++;
	if(Pike_sp != expected_stack + !s->u.efun->may_return_void)
	{
	  if(Pike_sp < expected_stack)
	    Pike_fatal("Function popped too many arguments: %s\n",
		  s->u.efun->name->str);
	  if(Pike_sp>expected_stack+1)
	    Pike_fatal("Function left droppings on stack: %s\n",
		  s->u.efun->name->str);
	  if(Pike_sp == expected_stack && !s->u.efun->may_return_void)
	    Pike_fatal("Non-void function returned without return value on stack: %s %d\n",
		  s->u.efun->name->str,s->u.efun->may_return_void);
	  if(Pike_sp==expected_stack+1 && s->u.efun->may_return_void)
	    Pike_fatal("Void function returned with a value on the stack: %s %d\n",
		  s->u.efun->name->str, s->u.efun->may_return_void);
	}
#endif

	break;
      }else{
	type=APPLY_SVALUE;
	o=s->u.object;
	if(o->prog == pike_trampoline_program &&
	   s->subtype == QUICK_FIND_LFUN(pike_trampoline_program, LFUN_CALL))
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
      type=APPLY_SVALUE;
      goto call_lfun;
    }
    break;
  }

  call_lfun:
#ifdef PIKE_DEBUG
    if(fun < 0 || fun >= NUM_LFUNS)
      Pike_fatal("Apply lfun on illegal value!\n");
#endif
    if(!o->prog)
      PIKE_ERROR("destructed object", "Apply on destructed object.\n", Pike_sp, args);
    fun = FIND_LFUN(o->prog, fun);
    goto apply_low;
  

  case APPLY_LOW:
    o = (struct object *)arg1;
    fun = (ptrdiff_t)arg2;
    if(o->prog == pike_trampoline_program &&
       fun == QUICK_FIND_LFUN(pike_trampoline_program, LFUN_CALL))
    {
      fun=((struct pike_trampoline *)(o->storage))->func;
      scope=((struct pike_trampoline *)(o->storage))->frame;
      o=scope->current_object;
      goto apply_low_with_scope;
    }

  apply_low:
#undef SCOPE
#include "apply_low.h"
    break;

  apply_low_with_scope:
#define SCOPE scope
#include "apply_low.h"
    break;
  }

  if(save_sp+1 > Pike_sp)
  {
    if(type != APPLY_SVALUE_STRICT)
      push_int(0);
  }else{
    if(save_sp+1 < Pike_sp)
    {
      assign_svalue(save_sp,Pike_sp-1);
      pop_n_elems(Pike_sp-save_sp-1);
      low_destruct_objects_to_destruct(); /* consider using a flag for immediate destruct instead... */
      
    }
    if(t_flag>1) trace_return_value();
  }
  return 0;
}



#define low_return_profiling()

#ifdef PROFILING
#ifdef HAVE_GETHRTIME
#undef low_return_profiling
#define low_return_profiling() do {					      \
  struct identifier *function;						      \
  long long time_passed, time_in_children, self_time;			      \
  time_in_children=Pike_interpreter.accounted_time-Pike_fp->children_base;    \
  time_passed = gethrtime()-Pike_interpreter.time_base - Pike_fp->start_time; \
  self_time=time_passed - time_in_children;				      \
  Pike_interpreter.accounted_time+=self_time;				      \
  function = Pike_fp->context.prog->identifiers + Pike_fp->ident;	      \
  function->total_time=Pike_fp->self_time_base + (INT32)(time_passed /1000);  \
  function->self_time+=(INT32)( self_time /1000);			      \
}while(0)
#endif
#endif


#define basic_low_return() 				\
  struct svalue *save_sp=Pike_fp->save_sp;		\
  DO_IF_DEBUG(						\
    if(Pike_mark_sp < Pike_fp->save_mark_sp)		\
      Pike_fatal("Popped below save_mark_sp!\n");		\
    if(Pike_sp<Pike_interpreter.evaluator_stack)	\
      Pike_fatal("Stack error (also simple).\n");		\
    )							\
							\
    Pike_mark_sp=Pike_fp->save_mark_sp;			\
							\
  POP_PIKE_FRAME()


void low_return(void)
{
  basic_low_return();
  if(save_sp+1 > Pike_sp)
  {
    push_int(0);
  }else{
    if(save_sp+1 < Pike_sp)
    {
      assign_svalue(save_sp,Pike_sp-1);
      pop_n_elems(Pike_sp-save_sp-1);
      
      /* consider using a flag for immediate destruct instead... */
      destruct_objects_to_destruct();
    }
    if(t_flag>1) trace_return_value();
  }
}

void low_return_pop(void)
{
  basic_low_return();

  if(save_sp < Pike_sp)
  {
    pop_n_elems(Pike_sp-save_sp);
    /* consider using a flag for immediate destruct instead... */
    destruct_objects_to_destruct();
  }
}


void unlink_previous_frame(void)
{
  struct pike_frame *current, *prev;
  struct svalue *target, **smsp;
  int freespace;

  current=Pike_interpreter.frame_pointer;
  prev=current->next;
#ifdef PIKE_DEBUG
  {
    JMP_BUF *rec;
    
    if((rec=Pike_interpreter.recoveries))
    {
      while(rec->frame_pointer == current) rec=rec->previous;
      if(rec->frame_pointer == current->next)
	Pike_fatal("You can't touch this!\n");
    }
  }
#endif

  Pike_interpreter.frame_pointer=prev;

  target=prev->save_sp;
  smsp=prev->save_mark_sp;
  current->flags=prev->flags;
  POP_PIKE_FRAME();
  
  prev=current->next=Pike_interpreter.frame_pointer;
  Pike_interpreter.frame_pointer=current;

  current->save_sp=target;
  current->save_mark_sp=smsp;

  /* Move svalues down */
  freespace=Pike_fp->locals - target;
  if(freespace > ((Pike_sp - Pike_fp->locals)<<2) + 32)
  {
    assign_svalues(target,
		   Pike_fp->locals,
		   Pike_sp - Pike_fp->locals,
		   BIT_MIXED);

    Pike_fp->locals-=freespace;
    Pike_fp->expendible-=freespace;
    pop_n_elems(freespace);
  }

  /* Move pointers down */
  freespace=Pike_fp->mark_sp_base - smsp;
  if(freespace > ((Pike_mark_sp - Pike_fp->mark_sp_base)<<2)+32)
  {
    MEMMOVE(smsp,
	    Pike_fp->mark_sp_base,
	    sizeof(struct svalue **)*(Pike_mark_sp - Pike_fp->mark_sp_base));
    Pike_fp->mark_sp_base-=freespace;
    Pike_mark_sp-=freespace;
  }
}


void mega_apply(enum apply_type type, INT32 args, void *arg1, void *arg2)
{
  /* The C stack margin is normally 8 kb, but if we get here during a
   * lowered margin then don't fail just because of that, unless it's
   * practically zero. */
  check_c_stack(Pike_interpreter.c_stack_margin ?
		Pike_interpreter.c_stack_margin : 100);

  if(low_mega_apply(type, args, arg1, arg2))
  {
    eval_instruction(Pike_fp->pc
#ifdef ENTRY_PROLOGUE_SIZE
		     - ENTRY_PROLOGUE_SIZE
#endif /* ENTRY_PROLOGUE_SIZE */
		     );
    low_return();
  }
}


/* Put catch outside of eval_instruction, so
 * the setjmp won't affect the optimization of
 * eval_instruction
 *
 * Returns 0 on throw.
 *
 * Returns 1 if the code performed a RETURN.
 *
 * Returns 2 if the code performed EXIT_CATCH or ESCAPE_CATCH.
 */
static int o_catch(PIKE_OPCODE_T *pc)
{
  JMP_BUF tmp;
  struct svalue *expendible=Pike_fp->expendible;
  int flags=Pike_fp->flags;

  debug_malloc_touch(Pike_fp);
  if(SETJMP(tmp))
  {
    *Pike_sp=throw_value;
    throw_value.type=T_INT;
    Pike_sp++;
    UNSETJMP(tmp);
    Pike_fp->expendible=expendible;
    Pike_fp->flags=flags;
    low_destruct_objects_to_destruct();
    return 0;
  }else{
    struct svalue **save_mark_sp=Pike_mark_sp;
    int x;
    Pike_fp->expendible=Pike_fp->locals + Pike_fp->num_locals;

    Pike_fp->flags&=~PIKE_FRAME_RETURN_INTERNAL;

    x=eval_instruction(pc);
#ifdef PIKE_DEBUG
    if(Pike_mark_sp < save_mark_sp)
      Pike_fatal("mark Pike_sp underflow in catch.\n");
#endif
    Pike_mark_sp=save_mark_sp;
    Pike_fp->expendible=expendible;
    Pike_fp->flags=flags;
    if(x>=0) mega_apply(APPLY_STACK, x, 0,0); /* Should never happen */
    UNSETJMP(tmp);
    return x == -2 ? 2 : 1;
  }
}

/*! @decl mixed call_function(function fun, mixed ... args)
 *!
 *! Call a function.
 */
PMOD_EXPORT void f_call_function(INT32 args)
{
  mega_apply(APPLY_STACK,args,0,0);
}

PMOD_EXPORT void call_handle_error(void)
{
  dmalloc_touch_svalue(&throw_value);

  if (Pike_interpreter.svalue_stack_margin > LOW_SVALUE_STACK_MARGIN) {
    int old_t_flag = t_flag;
    t_flag = 0;
    Pike_interpreter.svalue_stack_margin = LOW_SVALUE_STACK_MARGIN;
    Pike_interpreter.c_stack_margin = LOW_C_STACK_MARGIN;
    *(Pike_sp++) = throw_value;
    throw_value.type=T_INT;

    if (get_master()) {
      ONERROR tmp;
      SET_ONERROR(tmp,exit_on_error,"Error in handle_error in master object!");
      APPLY_MASTER("handle_error", 1);
      UNSET_ONERROR(tmp);
    }
    else {
      char *s;
      fprintf (stderr, "There's no master to handle the error. Dumping it raw:\n");
      init_buf();
      describe_svalue (Pike_sp - 1, 0, 0);
      s=simple_free_buf();
      fprintf(stderr,"%s\n",s);
      free(s);
    }

    pop_stack();
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

  /* FIXME: Is this up-to-date with mega_apply? */
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
    int tmp;
    new_frame->mark_sp_base=new_frame->save_mark_sp=Pike_mark_sp;
    tmp=eval_instruction(o->prog->program + offset);
    EVAL_INSTR_RET_CHECK(tmp);
    Pike_mark_sp=new_frame->save_mark_sp;
    
#ifdef PIKE_DEBUG
    if(Pike_sp<Pike_interpreter.evaluator_stack)
      Pike_fatal("Stack error (simple).\n");
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

  free_svalue(& throw_value);
  throw_value.type=T_INT;
  if(SETJMP_SP(recovery, args))
  {
    if(handle_errors) call_handle_error();
    push_int(0);
  }else{
    apply_low(o,fun,args);
  }
  UNSETJMP(recovery);
}

PMOD_EXPORT void safe_apply_low(struct object *o,int fun,int args)
{
  safe_apply_low2(o, fun, args, 1);
}

PMOD_EXPORT void safe_apply(struct object *o, const char *fun ,INT32 args)
{
#ifdef PIKE_DEBUG
  if(!o->prog) Pike_fatal("Apply safe on destructed object.\n");
#endif
  safe_apply_low2(o, find_identifier(fun, o->prog), args, 1);
}

/* Returns nonzero if the function was called in some handler. */
PMOD_EXPORT int low_unsafe_apply_handler(const char *fun,
					 struct object *handler,
					 struct object *compat,
					 INT32 args)
{
  int i;
#if 0
  fprintf(stderr, "low_unsafe_apply_handler(\"%s\", 0x%08p, 0x%08p, %d)\n",
	  fun, handler, compat, args);
#endif /* 0 */
  if (handler && handler->prog &&
      (i = find_identifier(fun, handler->prog)) != -1) {
    apply_low(handler, i, args);
  } else if (compat && compat->prog &&
	     (i = find_identifier(fun, compat->prog)) != -1) {
    apply_low(compat, i, args);
  } else {
    struct object *master_obj = get_master();
    if (master_obj && (i = find_identifier(fun, master_obj->prog)) != -1)
      apply_low(master_obj, i, args);
    else {
      pop_n_elems(args);
      push_undefined();
      return 0;
    }
  }
  return 1;
}

PMOD_EXPORT void low_safe_apply_handler(const char *fun,
					struct object *handler,
					struct object *compat,
					INT32 args)
{
  int i;
#if 0
  fprintf(stderr, "low_safe_apply_handler(\"%s\", 0x%08p, 0x%08p, %d)\n",
	  fun, handler, compat, args);
#endif /* 0 */
  if (handler && handler->prog &&
      (i = find_identifier(fun, handler->prog)) != -1) {
    safe_apply_low2(handler, i, args, 1);
  } else if (compat && compat->prog &&
	     (i = find_identifier(fun, compat->prog)) != -1) {
    safe_apply_low2(compat, i, args, 1);
  } else {
    struct object *master_obj = master();
    if ((i = find_identifier(fun, master_obj->prog)) != -1)
      safe_apply_low2(master_obj, i, args, 1);
    else {
      pop_n_elems(args);
      push_undefined();
    }
  }
}

/* NOTE: Returns 1 if result on stack, 0 otherwise. */
PMOD_EXPORT int safe_apply_handler(const char *fun,
				   struct object *handler,
				   struct object *compat,
				   INT32 args,
				   TYPE_FIELD rettypes)
{
  JMP_BUF recovery;
  int ret;

  STACK_LEVEL_START(args);

#if 0
  fprintf(stderr, "safe_apply_handler(\"%s\", 0x%08p, 0x%08p, %d)\n",
	  fun, handler, compat, args);
#endif /* 0 */

  free_svalue(& throw_value);
  throw_value.type=T_INT;

  if (SETJMP_SP(recovery, args)) {
    ret = 0;
  } else {
    if (low_unsafe_apply_handler (fun, handler, compat, args) &&
	rettypes && !((1 << Pike_sp[-1].type) & rettypes)) {
      if ((rettypes & BIT_ZERO) && SAFE_IS_ZERO (Pike_sp - 1)) {
	pop_stack();
	push_int(0);
      }
      else {
	push_constant_text("Invalid return value from %s: %O\n");
	push_text(fun);
	push_svalue(Pike_sp - 3);
	f_sprintf(3);
	if (!Pike_sp[-1].u.string->size_shift)
	  Pike_error("%s", Pike_sp[-1].u.string->str);
	else
	  Pike_error("Invalid return value from %s\n", fun);
      }
    }

    ret = 1;
  }

  UNSETJMP(recovery);

  STACK_LEVEL_DONE(ret);

  return ret;
}

PMOD_EXPORT void apply_lfun(struct object *o, int fun, int args)
{
#ifdef PIKE_DEBUG
  if(fun < 0 || fun >= NUM_LFUNS)
    Pike_fatal("Apply lfun on illegal value!\n");
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

PMOD_EXPORT void apply(struct object *o, const char *fun, int args)
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
      Pike_fatal("Stack underflow!\n");
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
    Pike_fatal("Svalue stack overflow. "
	  "(%ld entries on stack, stack_size is %ld entries)\n",
	  PTRDIFF_T_TO_LONG(Pike_sp - Pike_interpreter.evaluator_stack),
	  PTRDIFF_T_TO_LONG(Pike_stack_size));

  if(Pike_mark_sp > &(Pike_interpreter.mark_stack[Pike_stack_size]))
    Pike_fatal("Mark stack overflow.\n");

  if(Pike_mark_sp < Pike_interpreter.mark_stack)
    Pike_fatal("Mark stack underflow.\n");

  for(s=Pike_interpreter.evaluator_stack;s<Pike_sp;s++) check_svalue(s);

  s=Pike_interpreter.evaluator_stack;
  for(m=Pike_interpreter.mark_stack;m<Pike_mark_sp;m++)
  {
    if(*m < s)
      Pike_fatal("Mark stack failure.\n");

    s=*m;
  }

  if(s > &(Pike_interpreter.evaluator_stack[Pike_stack_size]))
    Pike_fatal("Mark stack exceeds svalue stack\n");

  for(f=Pike_fp;f;f=f->next)
  {
    if(f->locals)
    {
      if(f->locals < Pike_interpreter.evaluator_stack ||
	f->locals > &(Pike_interpreter.evaluator_stack[Pike_stack_size]))
      Pike_fatal("Local variable pointer points to Finspång.\n");

      if(f->args < 0 || f->args > Pike_stack_size)
	Pike_fatal("FEL FEL FEL! HELP!! (corrupted pike_frame)\n");
    }
  }
}
#endif

static const char *safe_idname_from_int(struct program *prog, int func)
{
  /* ID_FROM_INT with a thick layer of checks. */
  struct reference *ref;
  struct inherit *inher;
  struct identifier *id;
  if (!prog)
    return "<null program *>";
  if (func < 0 || func >= prog->num_identifier_references)
    return "<offset outside prog->identifier_references>";
  if (!prog->identifier_references)
    return "<null prog->identifier_references>";
  ref = prog->identifier_references + func;
  if (ref->inherit_offset >= prog->num_inherits)
    return "<offset outside prog->inherits>";
  if (!prog->inherits)
    return "<null prog->inherits>";
  inher = prog->inherits + ref->inherit_offset;
  prog = inher->prog;
  if (!prog)
    return "<null inherited prog>";
  if (ref->identifier_offset >= prog->num_identifiers)
    return "<offset outside inherited prog->identifiers>";
  if (!prog->identifiers)
    return "<null inherited prog->identifiers>";
  id = prog->identifiers + ref->identifier_offset;
  if (!id->name)
    return "<null identifier->name>";
  if (!id->name->str)
    return "<null identifier->name->str>";
  /* FIXME: Wide string identifiers. */
  return id->name->str;
}

/*: Prints the Pike backtrace for the interpreter context in the given
 *: thread to stderr, without messing in the internals (doesn't even
 *: use dynamic_buffer).
 *:
 *: This function is intended only for convenient use inside a
 *: debugger session; it can't be used from inside the code.
 */
void gdb_backtrace (
#ifdef PIKE_THREADS
  THREAD_T thread_id
#endif
)
{
  struct pike_frame *f, *of;

#ifdef PIKE_THREADS
  extern struct thread_state *gdb_thread_state_for_id(THREAD_T);
  struct thread_state *ts = gdb_thread_state_for_id(thread_id);
  if (!ts) {
    fputs ("Not a Pike thread.\n", stderr);
    return;
  }
  if (ts->swapped)
    f = ts->state.frame_pointer;
  else
    f = Pike_fp;
#else
  f = Pike_fp;
#endif

  for (of = 0; f; f = (of = f)->next)
    if (f->refs) {
      int args, i;

      if (f->context.prog) {
	INT32 line;
	char *file = debug_get_line (of ? f->pc - 1 : f->pc, f->context.prog, &line);
	fprintf (stderr, "%s:%d: ", file, line);
      }
      else
	fputs ("unknown program: ", stderr);

      if (f->current_object && f->current_object->prog) {
	/* FIXME: Wide string identifiers. */
	fputs (safe_idname_from_int(f->current_object->prog, f->fun), stderr);
	fputc ('(', stderr);
      }
      else
	fputs ("unknown function(", stderr);

      if(!f->locals)
      {
	args=0;
      }else{
	args=f->num_args;
	args = DO_NOT_WARN((INT32) MINIMUM(f->num_args, Pike_sp - f->locals));
	if(of)
	  args = DO_NOT_WARN((INT32)MINIMUM(f->num_args,of->locals - f->locals));
	args=MAXIMUM(args,0);
      }

      for (i = 0; i < args; i++) {
	struct svalue *arg = f->locals + i;

	switch (arg->type) {
	  case T_LVALUE:
	    fputs ("lvalue", stderr);
	    break;

	  case T_INT:
	    fprintf (stderr, "%ld", (long) arg->u.integer);
	    break;

	  case T_TYPE:
	    /* FIXME: */
	    fputs("type-value", stderr);
	    break;

	  case T_STRING: {
	    int i,j=0;
	    fputc ('"', stderr);
	    for(i=0; i < arg->u.string->len && i < 100; i++)
	    {
	      switch(j=index_shared_string(arg->u.string,i))
	      {
		case '\n':
		  fputc ('\\', stderr);
		  fputc ('n', stderr);
		  break;

		case '\t':
		  fputc ('\\', stderr);
		  fputc ('t', stderr);
		  break;

		case '\b':
		  fputc ('\\', stderr);
		  fputc ('b', stderr);
		  break;

		case '\r':
		  fputc ('\\', stderr);
		  fputc ('r', stderr);
		  break;


		case '"':
		case '\\':
		  fputc ('\\', stderr);
		  fputc (j, stderr);
		  break;

		default:
		  if(j>=0 && j<256 && isprint(j))
		  {
		    fputc (j, stderr);
		    break;
		  }

		  fputc ('\\', stderr);
		  fprintf (stderr, "%o", j);

		  switch(index_shared_string(arg->u.string,i+1))
		  {
		    case '0': case '1': case '2': case '3':
		    case '4': case '5': case '6': case '7':
		    case '8': case '9':
		      fputc ('"', stderr);
		      fputc ('"', stderr);
		  }
		  break;
	      } 
	    }
	    fputc ('"', stderr);
	    if (i < arg->u.string->len)
	      fprintf (stderr, "+[%ld]", (long) (arg->u.string->len - i));
	    break;
	  }

	  case T_FUNCTION:
	    /* FIXME: Wide string identifiers. */
	    if(arg->subtype == FUNCTION_BUILTIN)
	      fputs (arg->u.efun->name->str, stderr);
	    else if(arg->u.object->prog)
	      fputs (safe_idname_from_int(arg->u.object->prog,arg->subtype), stderr);
	    else
	      fputc ('0', stderr);
	    break;

	  case T_OBJECT: {
	    struct program *p = arg->u.object->prog;
	    if (p && p->num_linenumbers) {
	      INT32 line;
	      char *file = debug_get_line (NULL, p, &line);
	      fprintf (stderr, "object(%s:%d)", file, line);
	    }
	    else
	      fputs ("object", stderr);
	    break;
	  }

	  case T_PROGRAM: {
	    struct program *p = arg->u.program;
	    if (p->num_linenumbers) {
	      INT32 line;
	      char *file = debug_get_line (NULL, p, &line);
	      fprintf (stderr, "program(%s:%d)", file, line);
	    }
	    else
	      fputs ("program", stderr);
	    break;
	  }

	  case T_FLOAT:
	    fprintf (stderr, "%f",(double) arg->u.float_number);
	    break;

	  case T_ARRAY:
	    fprintf (stderr, "array[%ld]", (long) arg->u.array->size);
	    break;

	  case T_MULTISET:
	    fprintf (stderr, "multiset[%ld]", (long) multiset_sizeof (arg->u.multiset));
	    break;

	  case T_MAPPING:
	    fprintf (stderr, "mapping[%ld]", (long) m_sizeof (arg->u.mapping));
	    break;

	  default:
	    fprintf (stderr, "<Unknown %d>", arg->type);
	}

	if (i < args - 1) fputs (", ", stderr);
      }
      fputs (")\n", stderr);
    }
    else
      fputs ("frame with no references\n", stderr);
}

/*: Prints the Pike backtraces for the interpreter contexts in all
 *: Pike threads to stderr, using @[gdb_backtrace].
 *:
 *: This function is intended only for convenient use inside a
 *: debugger session; it can't be used from inside the program.
 */
void gdb_backtraces()
{
#ifdef PIKE_THREADS
  extern INT32 gdb_next_thread_state(INT32, struct thread_state **);
  INT32 i = 0;
  struct thread_state *ts = 0;
  while ((i = gdb_next_thread_state (i, &ts)), ts) {
    fprintf (stderr, "\nTHREAD_ID %ld (swapped %s):\n",
	     (long) ts->id, ts->swapped ? "out" : "in");
    gdb_backtrace (ts->id);
  }
#else
  gdb_backtrace();
#endif
}

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
