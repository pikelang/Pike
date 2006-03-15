/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: interpret.c,v 1.374 2006/03/15 09:03:38 grubba Exp $
*/

#include "global.h"
#include "interpret.h"
#include "object.h"
#include "program.h"
#include "svalue.h"
#include "array.h"
#include "mapping.h"
#include "pike_error.h"
#include "stralloc.h"
#include "constants.h"
#include "pike_macros.h"
#include "multiset.h"
#include "backend.h"
#include "operators.h"
#include "opcodes.h"
#include "pike_embed.h"
#include "lex.h"
#include "builtin_functions.h"
#include "signal_handler.h"
#include "gc.h"
#include "threads.h"
#include "callback.h"
#include "fd_control.h"
#include "pike_security.h"
#include "bignum.h"
#include "pike_types.h"
#include "pikecode.h"

#include "block_alloc.h"

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

#define TRACE_LEN (100 + Pike_interpreter.trace_level * 10)

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

static void do_trace_call(INT32 args, dynamic_buffer *old_buf);
static void do_trace_func_return (int got_retval, struct object *o, int fun);
static void do_trace_return (int got_retval, dynamic_buffer *old_buf);

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
void gc_mark_stack_external (struct pike_frame *f,
			     struct svalue *stack_p, struct svalue *stack)
{
  for (; f; f = f->next)
    GC_ENTER (f, T_PIKE_FRAME) {
      if (!debug_gc_check (f, " as frame on stack")) {
	if(f->context.parent)
	  gc_mark_external (f->context.parent, " in context.parent in frame on stack");
	gc_mark_external (f->current_object, " in current_object in frame on stack");
	gc_mark_external (f->context.prog, " in context.prog in frame on stack");
	if (f->locals) {		/* Check really needed? */
	  if (f->flags & PIKE_FRAME_MALLOCED_LOCALS) {
	    gc_mark_external_svalues(f->locals, f->num_locals,
				     " in malloced locals of trampoline frame on stack");
	  } else {
	    if (f->locals > stack_p || (stack_p - f->locals) >= 0x10000) {
	      fatal("Unreasonable locals: stack:%p locals:%p\n",
		    stack_p, f->locals);
	    }
	    gc_mark_external_svalues (f->locals, stack_p - f->locals, " on svalue stack");
	    stack_p = f->locals;
	  }
	}
      }
    } GC_LEAVE;
  if (stack != stack_p)
    gc_mark_external_svalues (stack, stack_p - stack, " on svalue stack");
}

static void gc_check_stack_callback(struct callback *foo, void *bar, void *gazonk)
{
  if (Pike_interpreter.evaluator_stack)
    gc_mark_stack_external (Pike_fp, Pike_sp, Pike_interpreter.evaluator_stack);
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
 * Note: All callers except catching_eval_instruction need to save
 * Pike_interpreter.catching_eval_jmpbuf, zero it, and restore it
 * afterwards.
 */
static int eval_instruction(PIKE_OPCODE_T *pc);

PMOD_EXPORT int low_init_interpreter(struct Pike_interpreter *interpreter)
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
	interpreter->evaluator_stack=0;
	interpreter->mark_stack=0;
	goto use_malloc;
#define NEED_USE_MALLOC_LABEL
      }
    }
    /* Don't keep this fd on exec() */
    set_close_on_exec(fd, 1);
  }
#endif

#define MMALLOC(X,Y)							\
  (Y *)mmap(0, (X)*sizeof(Y), PROT_READ|PROT_WRITE,			\
	    MAP_NORESERVE|MAP_PRIVATE|MAP_ANONYMOUS, fd, 0)

  interpreter->evaluator_stack_malloced = 0;
  interpreter->mark_stack_malloced = 0;
  interpreter->evaluator_stack = MMALLOC(Pike_stack_size,struct svalue);
  interpreter->mark_stack = MMALLOC(Pike_stack_size, struct svalue *);
  if((char *)MAP_FAILED == (char *)interpreter->evaluator_stack) {
    interpreter->evaluator_stack = 0;
    interpreter->evaluator_stack_malloced = 1;
  }
  if((char *)MAP_FAILED == (char *)interpreter->mark_stack) {
    interpreter->mark_stack = 0;
    interpreter->mark_stack_malloced = 1;
  }

#ifdef NEED_USE_MALLOC_LABEL
use_malloc:
#endif /* NEED_USE_MALLOC_LABEL */

#else /* !USE_MMAP_FOR_STACK */
  interpreter->evaluator_stack = 0;
  interpreter->evaluator_stack_malloced = 1;
  interpreter->mark_stack = 0;
  interpreter->mark_stack_malloced = 1;
#endif /* USE_MMAP_FOR_STACK */

  if(!interpreter->evaluator_stack)
  {
    if (!(interpreter->evaluator_stack =
	  (struct svalue *)malloc(Pike_stack_size*sizeof(struct svalue))))
      return 1;	/* Out of memory (evaluator stack). */
  }

  if(!interpreter->mark_stack)
  {
    if (!(interpreter->mark_stack =
	  (struct svalue **)malloc(Pike_stack_size*sizeof(struct svalue *))))
      return 2;	/* Out of memory (mark stack). */
  }

  interpreter->stack_pointer = interpreter->evaluator_stack;
  interpreter->mark_stack_pointer = interpreter->mark_stack;
  interpreter->frame_pointer = 0;
  interpreter->catch_ctx = NULL;
  interpreter->catching_eval_jmpbuf = NULL;

  interpreter->svalue_stack_margin = SVALUE_STACK_MARGIN;
  interpreter->c_stack_margin = C_STACK_MARGIN;

#ifdef PROFILING
  interpreter->unlocked_time = 0;
  interpreter->accounted_time = 0;
#endif

  return 0;	/* OK. */
}

PMOD_EXPORT void init_interpreter(void)
{
  if (low_init_interpreter(&Pike_interpreter)) {
    Pike_fatal("Out of memory initializing the interpreter stack.\n");
  }

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
      SET_INSTR_ADDRESS(F_RANGE,		o_range2);
      SET_INSTR_ADDRESS(F_SSCANF,		o_sscanf);
#endif /* PIKE_USE_MACHINE_CODE && !PIKE_DEBUG */
      tables_need_init=0;
#ifdef INIT_INTERPRETER_STATE
      INIT_INTERPRETER_STATE();
#endif
    }
  }
#endif /* HAVE_COMPUTED_GOTO || PIKE_USE_MACHINE_CODE */
}

/*
 * lvalues are stored in two svalues in one of these formats:
 * array[index]   : { array, index } 
 * mapping[index] : { mapping, index } 
 * multiset[index] : { multiset, index } 
 * object[index] : { object, index } (external object indexing)
 * local variable : { svalue pointer (T_SVALUE_PTR), nothing (T_VOID) }
 * global variable : { object, identifier index (T_OBJ_INDEX) } (internal object indexing)
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
      TYPE_FIELD types = 0;
      ONERROR err;
      a=allocate_array(lval[1].u.array->size>>1);
      SET_ONERROR(err, do_free_array, a);
      for(e=0;e<a->size;e++) {
	lvalue_to_svalue_no_free(ITEM(a)+e, ITEM(lval[1].u.array)+(e<<1));
	types |= 1 << ITEM(a)[e].type;
      }
      a->type_field = types;
      to->type = T_ARRAY;
      to->u.array=a;
      UNSET_ONERROR(err);
      break;
    }
      
    case T_SVALUE_PTR:
      dmalloc_touch_svalue(lval->u.lval);
      assign_svalue_no_free(to, lval->u.lval);
      break;

    case T_OBJECT:
      /* FIXME: Object subtypes! */
      if (lval[1].type == T_OBJ_INDEX)
	low_object_index_no_free (to, lval->u.object, lval[1].u.identifier);
      else
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

  case T_SVALUE_PTR:
    dmalloc_touch_svalue(from);
    dmalloc_touch_svalue(lval->u.lval);
    assign_svalue(lval->u.lval,from);
    break;

  case T_OBJECT:
    /* FIXME: Object subtypes! */
    if (lval[1].type == T_OBJ_INDEX)
      object_low_set_index (lval->u.object, lval[1].u.identifier, from);
    else
      object_set_index(lval->u.object, lval+1, from);
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
      
    case T_SVALUE_PTR:
      dmalloc_touch_svalue(lval->u.lval);
      if(lval->u.lval->type == t) return & ( lval->u.lval->u );
      return 0;

    case T_OBJECT:
      /* FIXME: What about object subtypes? */
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

INLINE void pike_trace(int level,char *fmt, ...) ATTRIBUTE((format (printf, 2, 3)));
INLINE void pike_trace(int level,char *fmt, ...)
{
  if(Pike_interpreter.trace_level > level)
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

/* Find the lexical scope @[depth] levels out.
 *
 * @[loc]:
 *   Input:
 *     struct object *o		// object to start from.
 *     struct inherit *inherit	// inherit in o->prog.
 *     int parent_identifier	// identifier in o to start from.
 *
 *   Output:
 *     struct object *o		// object containing the scope.
 *     struct inherit *inherit	// inherit in o->prog being the scope.
 *     int parent_identifier	// identifier in o from the inherit.
 */
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
    if(Pike_interpreter.trace_level>8)
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
	  /* Find the program that inherited us. */
	  int my_level = inh->inherit_level;
#ifdef PIKE_DEBUG
	  if(!my_level)
	    Pike_fatal("Gahhh! inherit level zero in wrong place!\n");
#endif
	  while(loc->inherit->inherit_level >= my_level)
	  {
	    TRACE((5,"-   inherit-- (%d >= %d)\n",
		   loc->inherit->inherit_level, my_level));
	    loc->inherit--;
	    TRACE((5, "-   identifier_level: %d\n",
		   loc->inherit->identifier_level));
	  }

	  find_external_context(loc, inh->parent_offset);
	  TRACE((5,
		 "-    inh->parent_identifier: %d\n"
		 "-    inh->identifier_level: %d\n"
		 "-    loc->parent_identifier: %d\n"
		 "-    loc->inherit->parent_offset: %d\n"
		 "-    loc->inherit->identifier_level: %d\n",
		 inh->parent_identifier,
		 inh->identifier_level,
		 loc->parent_identifier,
		 loc->inherit->parent_offset,
		 loc->inherit->identifier_level));

	  loc->parent_identifier =
	    inh->parent_identifier +
	    loc->inherit->identifier_level;
	  TRACE((5, "-    parent_identifier: %d\n", loc->parent_identifier));
	}
	break;

      case INHERIT_PARENT:
	TRACE((5,"-   Following inherit->parent\n"));
	loc->parent_identifier=inh->parent_identifier;
	loc->o=inh->parent;
#ifdef PIKE_DEBUG  
	TRACE((5, "-   parent_identifier: %d\n"
	       "-   o: %p\n"
	       "-   inh: %d\n",
	       loc->parent_identifier,
	       loc->o,
	       loc->inherit - loc->o->prog->inherits));
	if(Pike_interpreter.trace_level>5) {
	  dump_program_tables(loc->o->prog, 4);
	}
#endif
	break;

      case OBJECT_PARENT:
	TRACE((5,"-   Following o->parent\n"));

#ifdef PIKE_DEBUG
	/* Can this happen legitimately? Well, someone will hopefully
	 * let me know in that case. /mast */
	if (!(p->flags & PROGRAM_USES_PARENT))
	  Pike_fatal ("Attempting to access parent of object without parent pointer.\n");
#endif

	loc->parent_identifier=LOW_PARENT_INFO(loc->o,p)->parent_identifier;
	loc->o=LOW_PARENT_INFO(loc->o,p)->parent;
#ifdef PIKE_DEBUG  
	TRACE((5, "-   parent_identifier: %d\n"
	       "-   o: %p\n",
	       loc->parent_identifier,
	       loc->o));
	if(Pike_interpreter.trace_level>5) {
	  dump_program_tables(loc->o->prog, 4);
	}
#endif
	break;
    }

#ifdef PIKE_DEBUG
    /* I don't think this should happen either. The gc doesn't zap the
     * pointer even if the object is destructed, at least. /mast */
    if (!loc->o) Pike_fatal ("No parent object.\n");
#endif

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
      TRACE((5, "-   loc->inherit: %d\n",
	     loc->inherit - loc->o->prog->inherits));
    }
    else
      /* Return a valid pointer to a dummy inherit for the convenience
       * of the caller. Identifier offsets will be bogus but it'll
       * never get to that since the object is destructed. */
      loc->inherit = &dummy_inherit;

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
  if(Pike_interpreter.trace_level>3)
  {
    char *s;
    dynamic_buffer save_buf;

    init_buf(&save_buf);
    safe_describe_svalue(Pike_sp-1,0,0);
    s=simple_free_buf(&save_buf);
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

#ifdef PIKE_DEBUG
  if (Pike_interpreter.catch_ctx)
    Pike_fatal ("Catch context spillover.\n");
  if (Pike_interpreter.catching_eval_jmpbuf)
    Pike_fatal ("Got an active catching_eval_jmpbuf.\n");
#endif
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
  struct thread_state *thread_state;
#endif
  ptrdiff_t stack;
  ptrdiff_t mark_stack;
};

struct backlog backlog[BACKLOG];
int backlogp=BACKLOG-1;

static INLINE void low_debug_instr_prologue (PIKE_INSTR_T instr)
{
  if(Pike_interpreter.trace_level > 2)
  {
    char *file = NULL, *f;
    struct pike_string *filep;
    INT32 linep;

    filep = get_line(Pike_fp->pc,Pike_fp->context.prog,&linep);
    if (filep && !filep->size_shift) {
      file = filep->str;
      while((f=STRCHR(file,'/')))
	file=f+1;
    }
    fprintf(stderr,"- %s:%4ld:(%"PRINTPTRDIFFT"d): "
	    "%-25s %4"PRINTPTRDIFFT"d %4"PRINTPTRDIFFT"d\n",
	    file ? file : "-",(long)linep,
	    Pike_fp->pc - Pike_fp->context.prog->program,
	    get_opcode_name(instr),
	    Pike_sp-Pike_interpreter.evaluator_stack,
	    Pike_mark_sp-Pike_interpreter.mark_stack);
    free_string(filep);
  }

#ifdef HAVE_COMPUTED_GOTO
  if (instr) 
    ADD_RUNNED(instr);
  else
    Pike_fatal("NULL Instruction!\n");
#else /* !HAVE_COMPUTED_GOTO */
  if(instr + F_OFFSET < F_MAX_OPCODE) 
    ADD_RUNNED(instr);
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
    backlog[backlogp].thread_state=Pike_interpreter.thread_state;
#endif

#ifdef _REENTRANT
    CHECK_INTERPRETER_LOCK();
    if(d_flag>1) DEBUG_CHECK_THREAD();
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
   (Pike_interpreter.trace_level>3 ?				\
    sprintf(trace_buffer, "-    Arg = %ld\n",			\
	    (long) backlog[backlogp].arg),			\
    write_to_stderr(trace_buffer,strlen(trace_buffer)) : 0))

#define DEBUG_LOG_ARG2(ARG2)					\
  (backlog[backlogp].arg2 = (ARG2),				\
   (Pike_interpreter.trace_level>3 ?				\
    sprintf(trace_buffer, "-    Arg2 = %ld\n",			\
	    (long) backlog[backlogp].arg2),			\
    write_to_stderr(trace_buffer,strlen(trace_buffer)) : 0))

void dump_backlog(void)
{
#ifdef _REENTRANT
  struct thread_state *thread=0;
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
      if(thread != backlog[e].thread_state)
      {
	fprintf(stderr,"[Thread swap, Pike_interpreter.thread_state=%p]\n",backlog[e].thread_state);
	thread = backlog[e].thread_state;
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
      else if(instrs[backlog[e].instruction].flags & I_POINTER)
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

#undef BLOCK_ALLOC_NEXT
#define BLOCK_ALLOC_NEXT prev

BLOCK_ALLOC_FILL_PAGES (catch_context, 1)

#define POP_CATCH_CONTEXT do {						\
    struct catch_context *cc = Pike_interpreter.catch_ctx;		\
    DO_IF_DEBUG (							\
      TRACE((3,"-   Popping catch context %p ==> %p\n",			\
	     cc, cc ? cc->prev : NULL));				\
      if (!Pike_interpreter.catching_eval_jmpbuf)			\
	Pike_fatal ("Not in catching eval.\n");				\
      if (!cc)								\
	Pike_fatal ("Catch context dropoff.\n");			\
      if (cc->frame != Pike_fp)						\
	Pike_fatal ("Catch context doesn't belong to this frame.\n");	\
      if (Pike_mark_sp != cc->recovery.mark_sp + Pike_interpreter.mark_stack) \
	Pike_fatal ("Mark sp diff in catch context pop.\n");		\
    );									\
    debug_malloc_touch (cc);						\
    UNSETJMP (cc->recovery);						\
    Pike_fp->expendible = cc->save_expendible;				\
    Pike_interpreter.catch_ctx = cc->prev;				\
    really_free_catch_context (cc);					\
  } while (0)

static int catching_eval_instruction (PIKE_OPCODE_T *pc);


#ifdef PIKE_USE_MACHINE_CODE
/* Labels to jump to to cause eval_instruction to return */
/* FIXME: Replace these with assembler lables */
void *do_inter_return_label = NULL;
void *dummy_label = NULL;

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

#ifndef EXIT_MACHINE_CODE
#define EXIT_MACHINE_CODE()
#endif

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
    if (d_flag || Pike_interpreter.trace_level > 2) {			\
      debug_instr_prologue ((OPCODE) - F_OFFSET);			\
      EXTRA;								\
    }									\
  } while (0)

/* The following are intended to be called directly from generated
 * machine code. */
void simple_debug_instr_prologue_0 (PIKE_INSTR_T instr)
{
  if (d_flag || Pike_interpreter.trace_level > 2)
    low_debug_instr_prologue (instr);
}
void simple_debug_instr_prologue_1 (PIKE_INSTR_T instr, INT32 arg)
{
  if (d_flag || Pike_interpreter.trace_level > 2) {
    low_debug_instr_prologue (instr);
    DEBUG_LOG_ARG (arg);
  }
}
void simple_debug_instr_prologue_2 (PIKE_INSTR_T instr, INT32 arg1, INT32 arg2)
{
  if (d_flag || Pike_interpreter.trace_level > 2) {
    low_debug_instr_prologue (instr);
    DEBUG_LOG_ARG (arg1);
    DEBUG_LOG_ARG2 (arg2);
  }
}

#endif	/* !PIKE_DEBUG */

#endif /* PIKE_USE_MACHINE_CODE */

/* These don't change when eval_instruction_without_debug is compiled. */
#ifdef PIKE_DEBUG
#define REAL_PIKE_DEBUG
#define DO_IF_REAL_DEBUG(X) X
#define DO_IF_NOT_REAL_DEBUG(X)
#else
#define DO_IF_REAL_DEBUG(X)
#define DO_IF_NOT_REAL_DEBUG(X) X
#endif

#ifdef PIKE_SMALL_EVAL_INSTRUCTION
#undef PROG_COUNTER
#define PROG_COUNTER	Pike_fp->pc+1
#endif /* PIKE_SMALL_EVAL_INSTRUCTION */

#if defined(PIKE_USE_MACHINE_CODE) || defined(PIKE_SMALL_EVAL_INSTRUCTION)

#ifndef DEF_PROG_COUNTER
#define DEF_PROG_COUNTER
#endif /* !DEF_PROG_COUNTER */

#ifndef DEBUG_PROLOGUE
#define DEBUG_PROLOGUE(OPCODE, EXTRA) do {} while (0)
#endif

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

#if defined(OPCODE_RETURN_JUMPADDR) || defined(PIKE_SMALL_EVAL_INSTRUCTION)

#define OPCODE0_JUMP(O,N,F,C)						\
  void *PIKE_CONCAT(jump_opcode_,O)(void) {				\
    void *jumpaddr DO_IF_DEBUG(= NULL);					\
    DEF_PROG_COUNTER;							\
    DEBUG_PROLOGUE (O, ;);						\
    C;									\
    JUMP_DONE;								\
  }

#define OPCODE1_JUMP(O,N,F,C)						\
  void *PIKE_CONCAT(jump_opcode_,O)(INT32 arg1) {			\
    void *jumpaddr DO_IF_DEBUG(= NULL);					\
    DEF_PROG_COUNTER;							\
    DEBUG_PROLOGUE (O, DEBUG_LOG_ARG (arg1));				\
    C;									\
    JUMP_DONE;								\
  }

#define OPCODE2_JUMP(O,N,F,C)						\
  void *PIKE_CONCAT(jump_opcode_,O)(INT32 arg1, INT32 arg2) {		\
    void *jumpaddr DO_IF_DEBUG(= NULL);					\
    DEF_PROG_COUNTER;							\
    DEBUG_PROLOGUE (O, DEBUG_LOG_ARG (arg1); DEBUG_LOG_ARG2 (arg2));	\
    C;									\
    JUMP_DONE;								\
  }

#define SET_PROG_COUNTER(X) (jumpaddr = (X))

#ifdef PIKE_DEBUG
#define JUMP_DONE do {							\
    if (!jumpaddr)							\
      Pike_fatal ("Instruction didn't set jump address.\n");		\
    return jumpaddr;							\
  } while (0)
#else
#define JUMP_DONE return jumpaddr
#endif

#else  /* !OPCODE_RETURN_JUMPADDR && !PIKE_SMALL_EVAL_INSTRUCTION */
#define OPCODE0_JUMP OPCODE0
#define OPCODE1_JUMP OPCODE1
#define OPCODE2_JUMP OPCODE2
#define JUMP_DONE DONE
#endif	/* OPCODE_RETURN_JUMPADDR || PIKE_SMALL_EVAL_INSTRUCTION */

#if defined(OPCODE_INLINE_BRANCH) || defined(PIKE_SMALL_EVAL_INSTRUCTION)
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
#else /* !OPCODE_INLINE_BRANCH && !PIKE_SMALL_EVAL_INSTRUCTION */
#define TEST_OPCODE0(O,N,F,C) OPCODE0_PTRJUMP(O,N,F,C)
#define TEST_OPCODE1(O,N,F,C) OPCODE1_PTRJUMP(O,N,F,C)
#define TEST_OPCODE2(O,N,F,C) OPCODE2_PTRJUMP(O,N,F,C)
#endif /* OPCODE_INLINE_BRANCH || PIKE_SMALL_EVAL_INSTRUCTION */

#define OPCODE0_TAIL(O,N,F,C) OPCODE0(O,N,F,C)
#define OPCODE1_TAIL(O,N,F,C) OPCODE1(O,N,F,C)
#define OPCODE2_TAIL(O,N,F,C) OPCODE2(O,N,F,C)

#define OPCODE0_PTRJUMP(O,N,F,C) OPCODE0_JUMP(O,N,F,C)
#define OPCODE1_PTRJUMP(O,N,F,C) OPCODE1_JUMP(O,N,F,C)
#define OPCODE2_PTRJUMP(O,N,F,C) OPCODE2_JUMP(O,N,F,C)
#define OPCODE0_TAILPTRJUMP(O,N,F,C) OPCODE0_PTRJUMP(O,N,F,C)
#define OPCODE1_TAILPTRJUMP(O,N,F,C) OPCODE1_PTRJUMP(O,N,F,C)
#define OPCODE2_TAILPTRJUMP(O,N,F,C) OPCODE2_PTRJUMP(O,N,F,C)

#define OPCODE0_RETURN(O,N,F,C) OPCODE0_JUMP(O,N,F,C)
#define OPCODE1_RETURN(O,N,F,C) OPCODE1_JUMP(O,N,F,C)
#define OPCODE2_RETURN(O,N,F,C) OPCODE2_JUMP(O,N,F,C)
#define OPCODE0_TAILRETURN(O,N,F,C) OPCODE0_RETURN(O,N,F,C)
#define OPCODE1_TAILRETURN(O,N,F,C) OPCODE1_RETURN(O,N,F,C)
#define OPCODE2_TAILRETURN(O,N,F,C) OPCODE2_RETURN(O,N,F,C)

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

#ifdef GLOBAL_DEF_PROG_COUNTER
GLOBAL_DEF_PROG_COUNTER;
#endif

#ifndef SET_PROG_COUNTER
#define SET_PROG_COUNTER(X)	(PROG_COUNTER=(X))
#endif /* SET_PROG_COUNTER */

#undef DONE
#undef FETCH
#undef INTER_RETURN

#define DONE return
#define FETCH
#define INTER_RETURN {SET_PROG_COUNTER(do_inter_return_label);JUMP_DONE;}

#if defined(PIKE_USE_MACHINE_CODE) && defined(_M_IX86)
/* Disable frame pointer optimization */
#pragma optimize("y", off)
#endif

#include "interpret_functions_fixed.h"

#if defined(PIKE_USE_MACHINE_CODE) && defined(_M_IX86)
/* Restore optimization */
#pragma optimize("", on)
#endif

#ifdef PIKE_SMALL_EVAL_INSTRUCTION
#undef SET_PROG_COUNTER
#undef PROG_COUNTER
#define PROG_COUNTER	pc
#endif

#endif /* PIKE_USE_MACHINE_CODE || PIKE_SMALL_EVAL_INSTRUCTION */

#ifdef PIKE_USE_MACHINE_CODE

#ifdef PIKE_DEBUG
/* Note: The debug code is extracted, to keep the frame size constant. */
static int eval_instruction_low(PIKE_OPCODE_T *pc);
#endif /* PIKE_DEBUG */

static int eval_instruction(PIKE_OPCODE_T *pc)
#ifdef PIKE_DEBUG
{
  int x;
  if (Pike_interpreter.trace_level > 5 && pc) {
    int i;
    fprintf(stderr, "Calling code at %p:\n", pc);
#ifdef PIKE_OPCODE_ALIGN
    if (((INT32)pc) % PIKE_OPCODE_ALIGN) {
      Pike_fatal("Odd offset!\n");
    }
#endif /* PIKE_OPCODE_ALIGN */
#ifdef DISASSEMBLE_CODE
    DISASSEMBLE_CODE(pc, 16*4);
#else /* !DISASSEMBLE_CODE */
    for (i=0; i < 16; i+=4) {
      fprintf(stderr,
	      "  0x%08x 0x%08x 0x%08x 0x%08x\n",
	      ((int *)pc)[i],
	      ((int *)pc)[i+1],
	      ((int *)pc)[i+2],
	      ((int *)pc)[i+3]);
    }
#endif /* DISASSEMBLE_CODE */
  }
  x = eval_instruction_low(pc);
  pike_trace(3, "-    eval_instruction(%p) ==> %d\n", pc, x);
  return x;
}

static int eval_instruction_low(PIKE_OPCODE_T *pc)
#endif /* PIKE_DEBUG */
{
  if(pc == NULL) {

    if(do_inter_return_label != NULL)
      Pike_fatal("eval_instruction called with NULL (twice).\n");

#ifdef __GNUC__
    do_inter_return_label = && inter_return_label;
#elif defined (_M_IX86)
    /* MSVC. */
    _asm
    {
      lea eax,inter_return_label
      mov do_inter_return_label,eax
    }
#else
#error Machine code not supported with this compiler.
#endif

#if 0
    /* Paranoia.
     *
     * This can happen on systems with delay slots if the labels aren't
     * used explicitly.
     */
    if (do_inter_return_label == do_escape_catch_label) {
      Pike_fatal("Inter return and escape catch labels are equal: %p\n",
		 do_inter_return_label);
    }
#endif

    /* Trick optimizer */
    if(!dummy_label)
      return 0;
  }

  /* This else is important to avoid an overoptimization bug in (at
   * least) gcc 4.0.2 20050808 which caused the address stored in
   * do_inter_return_label to be at the CALL_MACHINE_CODE below. */
  else {
    CALL_MACHINE_CODE(pc);

    /* This code is never reached, but will
     * prevent gcc from optimizing the labels below too much
     */

#ifdef PIKE_DEBUG
    fprintf(stderr,"We have reached the end of the world!\n");
#endif
  }

#ifdef __GNUC__
  goto *dummy_label;
#endif

#if 0
  if (dummy_label) {
  inter_escape_catch_label:
    EXIT_MACHINE_CODE();
    return -2;
  }
#endif

 inter_return_label:
  EXIT_MACHINE_CODE();
#ifdef PIKE_DEBUG
  pike_trace(3, "-    Inter return\n");
#endif
  return -1;
}

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

static INLINE int eval_instruction(unsigned char *pc)
{
  if(d_flag || Pike_interpreter.trace_level>2)
    return eval_instruction_with_debug(pc);
  else
    return eval_instruction_without_debug(pc);
}


#else /* !PIKE_DEBUG || HAVE_COMPUTED_GOTO */
#include "interpreter.h"
#endif


#endif /* PIKE_USE_MACHINE_CODE */

#undef REAL_PIKE_DEBUG
#undef DO_IF_REAL_DEBUG
#undef DO_IF_NOT_REAL_DEBUG


static void do_trace_call(INT32 args, dynamic_buffer *old_buf)
{
  struct pike_string *filep = NULL;
  char *file, *s;
  INT32 linep,e;

  my_strcat("(");
  for(e=0;e<args;e++)
  {
    if(e) my_strcat(",");
    safe_describe_svalue(Pike_sp-args+e,0,0);
  }
  my_strcat(")");

  s=simple_free_buf(old_buf);
  if((size_t)strlen(s) > (size_t)TRACE_LEN)
  {
    s[TRACE_LEN]=0;
    s[TRACE_LEN-1]='.';
    s[TRACE_LEN-2]='.';
    s[TRACE_LEN-3]='.';
  }

  if(Pike_fp && Pike_fp->pc)
  {
    char *f;
    filep = get_line(Pike_fp->pc,Pike_fp->context.prog,&linep);
    if (filep->size_shift)
      file = "...";
    else {
      file = filep->str;
      while((f=STRCHR(file,'/')))
	file=f+1;
    }
  }else{
    linep=0;
    file="-";
  }

  {
    char buf[40];
    if (linep)
      SNPRINTF(buf, sizeof (buf), "%s:%ld:", file, (long)linep);
    else
      SNPRINTF(buf, sizeof (buf), "%s:", file);
    fprintf(stderr,"- %-20s %s\n",buf,s);
  }

  if (filep) {
    free_string(filep);
  }
  free(s);
}

static void do_trace_func_return (int got_retval, struct object *o, int fun)
{
  dynamic_buffer save_buf;
  init_buf (&save_buf);
  if (o) {
    if (o->prog) {
      struct identifier *id = ID_FROM_INT (o->prog, fun);
      char buf[50];
      sprintf(buf, "%lx->", DO_NOT_WARN((long) PTR_TO_INT (o)));
      my_strcat(buf);
      if (id->name->size_shift)
	my_strcat ("[widestring function name]");
      else
	my_strcat(id->name->str);
      my_strcat ("() ");
    }
    else
      my_strcat ("function in destructed object ");
  }
  do_trace_return (got_retval, &save_buf);
}

static void do_trace_return (int got_retval, dynamic_buffer *old_buf)
{
  struct pike_string *filep = NULL;
  char *file, *s;
  INT32 linep;

  if (got_retval) {
    my_strcat ("returns: ");
    safe_describe_svalue(Pike_sp-1,0,0);
  }
  else
    my_strcat ("returns with no value");

  s=simple_free_buf(old_buf);
  if((size_t)strlen(s) > (size_t)TRACE_LEN)
  {
    s[TRACE_LEN]=0;
    s[TRACE_LEN-1]='.';
    s[TRACE_LEN-2]='.';
    s[TRACE_LEN-3]='.';
  }

  if(Pike_fp && Pike_fp->pc)
  {
    char *f;
    filep = get_line(Pike_fp->pc,Pike_fp->context.prog,&linep);
    if (filep->size_shift)
      file = "...";
    else {
      file = filep->str;
      while((f=STRCHR(file,'/')))
	file=f+1;
    }
  }else{
    linep=0;
    file="-";
  }

  {
    char buf[40];
    if (linep)
      SNPRINTF(buf, sizeof (buf), "%s:%ld:", file, (long)linep);
    else
      SNPRINTF(buf, sizeof (buf), "%s:", file);
    fprintf(stderr,"- %-20s %s\n",buf,s);
  }

  if (filep) {
    free_string(filep);
  }
  free(s);
}


#undef BLOCK_ALLOC_NEXT
#define BLOCK_ALLOC_NEXT next

#undef INIT_BLOCK
#define INIT_BLOCK(X) do {			\
  X->refs=0;					\
  add_ref(X);	/* For DMALLOC... */		\
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
  struct object *o = NULL;
  struct pike_frame *scope=0;
  ptrdiff_t fun=0;
  struct svalue *save_sp=Pike_sp-args;

#if defined(PIKE_DEBUG) && defined(_REENTRANT)
  if(d_flag)
    {
      THREAD_T self = th_self();

      CHECK_INTERPRETER_LOCK();

      if( Pike_interpreter.thread_state &&
	  !th_equal(Pike_interpreter.thread_state->id, self) )
	Pike_fatal("Current thread is wrong.\n");

      DEBUG_CHECK_THREAD();
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
	Pike_error("Attempt to call the string \"%20S\"...\n", s->u.string);
      } else {
	Pike_error("Attempt to call the string \"%S\"\n", s->u.string);
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
#endif
	if(Pike_interpreter.trace_level>1)
	{
	  dynamic_buffer save_buf;
	  init_buf(&save_buf);
	  if (s->u.efun->name->size_shift)
	    my_strcat ("[widestring function name]");
	  else
	    my_strcat (s->u.efun->name->str);
	  do_trace_call(args, &save_buf);
	}
	check_threads_etc();
	(*(s->u.efun->function))(args);

#ifdef PIKE_DEBUG
	s->u.efun->runs++;
	if(Pike_sp != expected_stack + !s->u.efun->may_return_void)
	{
	  if(Pike_sp < expected_stack)
	    Pike_fatal("Function popped too many arguments: %S\n",
		       s->u.efun->name);
	  if(Pike_sp>expected_stack+1)
	    Pike_fatal("Function left droppings on stack: %S\n",
		       s->u.efun->name);
	  if(Pike_sp == expected_stack && !s->u.efun->may_return_void)
	    Pike_fatal("Non-void function returned without return value on stack: %S %d\n",
		       s->u.efun->name, s->u.efun->may_return_void);
	  if(Pike_sp==expected_stack+1 && s->u.efun->may_return_void)
	    Pike_fatal("Void function returned with a value on the stack: %S %d\n",
		       s->u.efun->name, s->u.efun->may_return_void);
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
      if(Pike_interpreter.trace_level)
      {
	dynamic_buffer save_buf;
	init_buf(&save_buf);
	safe_describe_svalue(s,0,0);
	do_trace_call(args, &save_buf);
      }
      apply_array(s->u.array,args);
      break;

    case T_PROGRAM:
      if(Pike_interpreter.trace_level)
      {
	dynamic_buffer save_buf;
	init_buf(&save_buf);
	safe_describe_svalue(s,0,0);
	do_trace_call(args, &save_buf);
      }
      push_object(clone_object(s->u.program,args));
      break;

    case T_OBJECT:
      /* FIXME: Object subtypes! */
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

  call_lfun: {
    int lfun;
#ifdef PIKE_DEBUG
    if(fun < 0 || fun >= NUM_LFUNS)
      Pike_fatal("Apply lfun on illegal value!\n");
#endif
    if(!o->prog)
      PIKE_ERROR("destructed object", "Apply on destructed object.\n", Pike_sp, args);
    lfun = FIND_LFUN(o->prog, fun);
    if (lfun < 0)
      Pike_error ("Cannot call undefined lfun %s.\n", lfun_names[fun]);
    fun = lfun;
    goto apply_low;
  }

  case APPLY_LOW:
    o = (struct object *)arg1;
    fun = PTR_TO_INT(arg2);
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
    if(type != APPLY_SVALUE_STRICT) {
      push_int(0);
      if(Pike_interpreter.trace_level>1)
	do_trace_func_return (1, o, fun);
    }
    else
      if(Pike_interpreter.trace_level>1)
	do_trace_func_return (0, o, fun);
  }else{
    if(save_sp+1 < Pike_sp)
    {
      assign_svalue(save_sp,Pike_sp-1);
      pop_n_elems(Pike_sp-save_sp-1);
      low_destruct_objects_to_destruct(); /* consider using a flag for immediate destruct instead... */
      
    }
    if(Pike_interpreter.trace_level>1)
      do_trace_func_return (1, o, fun);
  }
  return 0;
}



#define basic_low_return(save_sp)			\
  DO_IF_DEBUG(						\
    if(Pike_mark_sp < Pike_fp->save_mark_sp)		\
      Pike_fatal("Popped below save_mark_sp!\n");	\
    if(Pike_sp<Pike_interpreter.evaluator_stack)	\
      Pike_fatal("Stack error (also simple).\n");	\
    )							\
							\
    Pike_mark_sp=Pike_fp->save_mark_sp;			\
							\
  POP_PIKE_FRAME()


void low_return(void)
{
  struct svalue *save_sp = Pike_fp->save_sp;
  int trace_level = Pike_interpreter.trace_level;
  struct object *o;
  int fun = 0;

  if (trace_level > 1) {
    o = Pike_fp->current_object;
    fun = Pike_fp->fun;
  }

#if defined (PIKE_USE_MACHINE_CODE) && defined (OPCODE_RETURN_JUMPADDR)
  /* If the function that returns is the only ref to the current
   * object and its program then the program would be freed in
   * destruct_objects_to_destruct below. However, we're still
   * executing in an opcode in its code so we need prog->program to
   * stick around for a little while more to handle the returned
   * address. We therefore add a ref to the current object so that
   * it'll live through this function. */
  o = Pike_fp->current_object;
  add_ref (o);
#endif

  basic_low_return (save_sp);
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
  }

  if(trace_level>1)
    do_trace_func_return (1, o, fun);

#if defined (PIKE_USE_MACHINE_CODE) && defined (OPCODE_RETURN_JUMPADDR)
  free_object (o);
#endif
}

void low_return_pop(void)
{
  struct svalue *save_sp = Pike_fp->save_sp;
#if defined (PIKE_USE_MACHINE_CODE) && defined (OPCODE_RETURN_JUMPADDR)
  /* See note above. */
  struct object *o = Pike_fp->current_object;
  add_ref (o);
#endif

  basic_low_return (save_sp);

  if(save_sp < Pike_sp)
  {
    pop_n_elems(Pike_sp-save_sp);
    /* consider using a flag for immediate destruct instead... */
    destruct_objects_to_destruct();
  }

#if defined (PIKE_USE_MACHINE_CODE) && defined (OPCODE_RETURN_JUMPADDR)
  free_object (o);
#endif
}


void unlink_previous_frame(void)
{
  struct pike_frame *current, *prev;

  current=Pike_interpreter.frame_pointer;
  prev=current->next;
#ifdef PIKE_DEBUG
  {
    JMP_BUF *rec;

    /* Check if any recoveries belong to the frame we're
     * about to unlink.
     */
    if((rec=Pike_interpreter.recoveries))
    {
      while(rec->frame_pointer == current) rec=rec->previous;
      /* FIXME: Wouldn't a simple return be ok? */
      if(rec->frame_pointer == current->next)
	Pike_fatal("You can't touch this!\n");
    }
  }
#endif
  /* Save various fields from the previous frame.
   */
  current->save_sp=prev->save_sp;
  current->save_mark_sp=prev->save_mark_sp;
  current->flags=prev->flags;

  /* Unlink the top frame temporarily. */
  Pike_interpreter.frame_pointer=prev;

#ifdef PROFILING
  {
    /* We must update the profiling info of the previous frame
     * to account for that the current frame has gone away.
     */
    cpu_time_t total_time =
      get_cpu_time() - (Pike_interpreter.unlocked_time + current->start_time);
    cpu_time_t child_time =
      Pike_interpreter.accounted_time - current->children_base;
    struct identifier *function =
      current->context.prog->identifiers + current->ident;
    function->total_time += total_time;
    total_time -= child_time;
    function->self_time += total_time;
    Pike_interpreter.accounted_time += total_time;
#ifdef PROFILING_DEBUG
    fprintf(stderr, "%p: Unlinking previous frame.\n"
	    "Previous: %" PRINT_CPU_TIME " %" PRINT_CPU_TIME "\n"
	    "Current:  %" PRINT_CPU_TIME " %" PRINT_CPU_TIME "\n",
	    Pike_interpreter.thread_state,
	    prev->start_time, prev->children_base,
	    current->start_time, current->children_base);
#endif /* PROFILING_DEBUG */
  }
#endif /* PROFILING */

  /* Unlink the frame. */
  POP_PIKE_FRAME();

  /* Hook our frame again. */
  current->next=Pike_interpreter.frame_pointer;
  Pike_interpreter.frame_pointer=current;

#ifdef PROFILING
  current->children_base = Pike_interpreter.accounted_time;
  current->start_time = get_cpu_time() - Pike_interpreter.unlocked_time;
#endif /* PROFILING */

#if 0
  /* FIXME: This code is questionable, and the Pike_sp
   *        adjustment ought to modify the mark stack.
   */
  {
    int freespace;
    /* Move svalues down */
    freespace=current->locals - current->save_sp;
    if(freespace > ((Pike_sp - current->locals)<<2) + 32)
    {
      assign_svalues(current->save_sp,
		     current->locals,
		     Pike_sp - current->locals,
		     BIT_MIXED);

      current->locals-=freespace;
      current->expendible-=freespace;
      pop_n_elems(freespace);
    }

    /* Move pointers down */
    freespace=current->mark_sp_base - current->save_mark_sp;
    if(freespace > ((Pike_mark_sp - current->mark_sp_base)<<2)+32)
    {
      MEMMOVE(current->save_mark_sp,
	      current->mark_sp_base,
	      sizeof(struct svalue **)*(Pike_mark_sp - current->mark_sp_base));
      current->mark_sp_base-=freespace;
      Pike_mark_sp-=freespace;
    }
  }
#endif /* 0 */
}

static void restore_catching_eval_jmpbuf (LOW_JMP_BUF *p)
{
  Pike_interpreter.catching_eval_jmpbuf = p;
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
    /* Save and clear Pike_interpreter.catching_eval_jmpbuf so that the
     * following eval_instruction will install a LOW_JMP_BUF of its
     * own to handle catches. */
    LOW_JMP_BUF *saved_jmpbuf = Pike_interpreter.catching_eval_jmpbuf;
    ONERROR uwp;
    Pike_interpreter.catching_eval_jmpbuf = NULL;
    SET_ONERROR (uwp, restore_catching_eval_jmpbuf, saved_jmpbuf);

    eval_instruction(Pike_fp->pc
#ifdef ENTRY_PROLOGUE_SIZE
		     - ENTRY_PROLOGUE_SIZE
#endif /* ENTRY_PROLOGUE_SIZE */
		     );
    low_return();

    Pike_interpreter.catching_eval_jmpbuf = saved_jmpbuf;
    UNSET_ONERROR (uwp);
  }
}

/* Put catch outside of eval_instruction, so the setjmp won't affect
 * the optimization of eval_instruction.
 */
static int catching_eval_instruction (PIKE_OPCODE_T *pc)
{
  LOW_JMP_BUF jmpbuf;
#ifdef PIKE_DEBUG
  if (Pike_interpreter.catching_eval_jmpbuf)
    Pike_fatal ("catching_eval_jmpbuf already active.\n");
#endif
  Pike_interpreter.catching_eval_jmpbuf = &jmpbuf;
  if (LOW_SETJMP (jmpbuf))
  {
    Pike_interpreter.catching_eval_jmpbuf = NULL;
#ifdef PIKE_DEBUG
    pike_trace(3, "-    catching_eval_instruction(%p) caught error ==> -3\n",
	       pc);
#endif
    return -3;
  }else{
    int x = eval_instruction(pc);
    Pike_interpreter.catching_eval_jmpbuf = NULL;
#ifdef PIKE_DEBUG
    pike_trace(3, "-    catching_eval_instruction(%p) ==> %d\n", pc, x);
#endif
    return x;
  }
}

/*! @decl mixed `()(function fun, mixed ... args)
 *! @decl mixed call_function(function fun, mixed ... args)
 *!
 *! Call a function.
 *!
 *! Calls the function @[fun] with the arguments specified by @[args].
 *!
 *! @seealso
 *!   @[lfun::`()()]
 */
PMOD_EXPORT void f_call_function(INT32 args)
{
  mega_apply(APPLY_STACK,args,0,0);
}

/*! @class MasterObject
 */

/*! @decl void handle_error(mixed exception)
 *!
 *!   Called by the Pike runtime if an exception isn't caught.
 *!
 *! @param exception
 *!   Value that was @[throw()]'n.
 *!
 *! @seealso
 *!   @[describe_backtrace()]
 */

/*! @endclass
 */

PMOD_EXPORT void call_handle_error(void)
{
  dmalloc_touch_svalue(&throw_value);

  if (Pike_interpreter.svalue_stack_margin > LOW_SVALUE_STACK_MARGIN) {
    int old_t_flag = Pike_interpreter.trace_level;
    Pike_interpreter.trace_level = 0;
    Pike_interpreter.svalue_stack_margin = LOW_SVALUE_STACK_MARGIN;
    Pike_interpreter.c_stack_margin = LOW_C_STACK_MARGIN;
    *(Pike_sp++) = throw_value;
    dmalloc_touch_svalue(Pike_sp-1);
    throw_value.type=T_INT;

    if (get_master()) {		/* May return NULL at odd times. */
      ONERROR tmp;
      SET_ONERROR(tmp,exit_on_error,"Error in handle_error in master object!");
      APPLY_MASTER("handle_error", 1);
      UNSET_ONERROR(tmp);
    }
    else {
      dynamic_buffer save_buf;
      char *s;
      fprintf (stderr, "There's no master to handle the error. Dumping it raw:\n");
      init_buf(&save_buf);
      safe_describe_svalue (Pike_sp - 1, 0, 0);
      s=simple_free_buf(&save_buf);
      fprintf(stderr,"%s\n",s);
      free(s);
      if (Pike_sp[-1].type == PIKE_T_OBJECT && Pike_sp[-1].u.object->prog) {
	int fun = find_identifier("backtrace", Pike_sp[-1].u.object->prog);
	if (fun != -1) {
	  fprintf(stderr, "Attempting to extract the backtrace.\n");
	  safe_apply_low2(Pike_sp[-1].u.object, fun, 0, 0);
	  init_buf(&save_buf);
	  safe_describe_svalue(Pike_sp - 1, 0, 0);
	  pop_stack();
	  s=simple_free_buf(&save_buf);
	  fprintf(stderr,"%s\n",s);
	  free(s);
	}
      }
    }

    pop_stack();
    Pike_interpreter.svalue_stack_margin = SVALUE_STACK_MARGIN;
    Pike_interpreter.c_stack_margin = C_STACK_MARGIN;
    Pike_interpreter.trace_level = old_t_flag;
  }

  else {
    free_svalue(&throw_value);
    throw_value.type=T_INT;
  }
}

/* NOTE: This function may only be called from the compiler! */
PMOD_EXPORT int apply_low_safe_and_stupid(struct object *o, INT32 offset)
{
  JMP_BUF tmp;
  struct pike_frame *new_frame=alloc_pike_frame();
  int ret;
  int use_dummy_reference = !o->prog->num_identifier_references;
  int p_flags = o->prog->flags;
  LOW_JMP_BUF *saved_jmpbuf;

  /* This is needed for opcodes that use INHERIT_FROM_*
   * (eg F_EXTERN) to work.
   */
  if (use_dummy_reference) {
    struct reference dummy_ref = {
      0, 0, ID_HIDDEN,
    };
    /* check_program() doesn't like our identifier... */
    o->prog->flags |= PROGRAM_AVOID_CHECK;
    add_to_identifier_references(dummy_ref);
  }

  /* FIXME: Is this up-to-date with mega_apply? */
  new_frame->next = Pike_fp;
  new_frame->current_object = o;
  new_frame->context=o->prog->inherits[0];
  new_frame->locals = Pike_sp;
  new_frame->expendible=new_frame->locals;
  new_frame->args = 0;
  new_frame->num_args=0;
  new_frame->num_locals=0;
  new_frame->fun = o->prog->num_identifier_references-1;
  new_frame->pc = 0;
  new_frame->current_storage=o->storage;
  new_frame->context.parent=0;

#ifdef PIKE_DEBUG      
  if (Pike_fp && (new_frame->locals < Pike_fp->locals)) {
    fatal("New locals below old locals: %p < %p\n",
	  new_frame->locals, Pike_fp->locals);
  }
#endif /* PIKE_DEBUG */

  Pike_fp = new_frame;

  add_ref(new_frame->current_object);
  add_ref(new_frame->context.prog);

  saved_jmpbuf = Pike_interpreter.catching_eval_jmpbuf;
  Pike_interpreter.catching_eval_jmpbuf = NULL;

  if(SETJMP(tmp))
  {
    ret=1;
  }else{
    int tmp;
    new_frame->mark_sp_base=new_frame->save_mark_sp=Pike_mark_sp;
    tmp=eval_instruction(o->prog->program + offset);
    Pike_mark_sp=new_frame->save_mark_sp;
    
#ifdef PIKE_DEBUG
    if (tmp != -1)
      Pike_fatal ("Unexpected return value from eval_instruction: %d\n", tmp);
    if(Pike_sp<Pike_interpreter.evaluator_stack)
      Pike_fatal("Stack error (simple).\n");
#endif
    ret=0;
  }
  UNSETJMP(tmp);

  Pike_interpreter.catching_eval_jmpbuf = saved_jmpbuf;

  if (use_dummy_reference) {
    Pike_compiler->new_program->num_identifier_references--;
    o->prog->flags = p_flags;
  }

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
  int id;
#ifdef PIKE_DEBUG
  if(!o->prog) Pike_fatal("Apply safe on destructed object.\n");
#endif
  id = find_identifier(fun, o->prog);
  if (id >= 0)
    safe_apply_low2(o, id, args, 1);
  else {
    char buf[4096];
    /* FIXME: Ought to use string_buffer_vsprintf(). */
    SNPRINTF(buf, sizeof (buf), "Cannot call unknown function \"%s\".\n", fun);
    push_error (buf);
    free_svalue (&throw_value);
    move_svalue (&throw_value, --Pike_sp);
    call_handle_error();
    push_int (0);
  }
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
	Pike_error("Invalid return value from %s: %O\n",
		   fun, Pike_sp-1);
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
  int id = find_shared_string_identifier(fun, o->prog);
  if (id >= 0)
    apply_low(o, id, args);
  else
    Pike_error("Cannot call unknown function \"%S\".\n", fun);
}

PMOD_EXPORT void apply(struct object *o, const char *fun, int args)
{
  int id = find_identifier(fun, o->prog);
  if (id >= 0)
    apply_low(o, id, args);
  else
    Pike_error ("Cannot call unknown function \"%s\".\n", fun);
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

PMOD_EXPORT void safe_apply_svalue(struct svalue *s, int args, int handle_errors)
{
  JMP_BUF recovery;
  free_svalue(& throw_value);
  throw_value.type=T_INT;
  if(SETJMP_SP(recovery, args))
  {
    if(handle_errors) call_handle_error();
    push_int(0);
  }else{
    apply_svalue (s, args);
  }
  UNSETJMP(recovery);
}

/* Apply function @[fun] in parent @[depth] levels up with @[args] arguments.
 */
PMOD_EXPORT void apply_external(int depth, int fun, INT32 args)
{
  struct external_variable_context loc;

  loc.o = Pike_fp->current_object;
  loc.parent_identifier = Pike_fp->fun;
  if (loc.o->prog)
    loc.inherit = INHERIT_FROM_INT(loc.o->prog, loc.parent_identifier);

  find_external_context(&loc, depth);

  apply_low(loc.o, fun + loc.inherit->identifier_level, args);
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
      char *file = NULL;
      INT32 line;

      if (f->context.prog) {
	if (f->pc)
	  file = low_get_line_plain (f->pc, f->context.prog, &line, 0);
	else
	  file = low_get_program_line_plain (f->context.prog, &line, 0);
      }
      if (file)
	fprintf (stderr, "%s:%d: ", file, line);
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
	      file = low_get_program_line_plain (p, &line, 0);
	      fprintf (stderr, "object(%s:%d)", file, line);
	    }
	    else
	      fputs ("object", stderr);
	    break;
	  }

	  case T_PROGRAM: {
	    struct program *p = arg->u.program;
	    if (p->num_linenumbers) {
	      file = low_get_program_line_plain (p, &line, 0);
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
    fprintf (stderr, "\nTHREAD_ID %p (swapped %s):\n",
	     (void *)ts->id, ts->swapped ? "out" : "in");
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

PMOD_EXPORT void low_cleanup_interpret(struct Pike_interpreter *interpreter)
{
#ifdef USE_MMAP_FOR_STACK
  if(!interpreter->evaluator_stack_malloced)
  {
    munmap((char *)interpreter->evaluator_stack,
	   Pike_stack_size*sizeof(struct svalue));
    interpreter->evaluator_stack = 0;
  }
  if(!interpreter->mark_stack_malloced)
  {
    munmap((char *)interpreter->mark_stack,
	   Pike_stack_size*sizeof(struct svalue *));
    interpreter->mark_stack = 0;
  }
#endif

  if(interpreter->evaluator_stack)
    free((char *)interpreter->evaluator_stack);
  if(interpreter->mark_stack)
    free((char *)interpreter->mark_stack);

  interpreter->mark_stack = 0;
  interpreter->evaluator_stack = 0;
  interpreter->mark_stack_malloced = 0;
  interpreter->evaluator_stack_malloced = 0;

  interpreter->stack_pointer = 0;
  interpreter->mark_stack_pointer = 0;
  interpreter->frame_pointer = 0;
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

  low_cleanup_interpret(&Pike_interpreter);
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
  free_callback_list (&evaluator_callbacks);
  free_all_pike_frame_blocks();
  free_all_catch_context_blocks();
#endif
}
