/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
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
#include "bignum.h"
#include "pike_types.h"
#include "pikecode.h"
#include "pike_compiler.h"

#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

#ifdef HAVE_MMAP

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#ifdef MAP_NORESERVE
#define USE_MMAP_FOR_STACK
#endif
#endif

#ifdef USE_DTRACE
#include "dtrace_probes.h"
#else
#include "dtrace/dtrace_probes_disabled.h"
#endif

/*
 * Define the default evaluator stack size, used for just about everything.
 */
#define EVALUATOR_STACK_SIZE	100000

#define TRACE_LEN (size_t)(100 + Pike_interpreter.trace_level * 10)

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

PMOD_EXPORT extern void check_c_stack_margin(void)
{
  check_c_stack(Pike_interpreter.c_stack_margin);
}

#ifdef PIKE_DEBUG
static char trace_buffer[2000];
#endif

#ifdef INTERNAL_PROFILING
PMOD_EXPORT unsigned long evaluator_callback_calls = 0;
#endif


int fast_check_threads_counter = 0;

PMOD_EXPORT int Pike_stack_size = EVALUATOR_STACK_SIZE;

static void do_trace_call(struct byte_buffer *buf, INT32 args);
static void do_trace_func_return (int got_retval, struct object *o, int fun);
static void do_trace_return (struct byte_buffer *buf, int got_retval);
static void do_trace_efun_call(const struct svalue *s, INT32 args);
#ifdef PIKE_DEBUG
static void do_trace_efun_return(const struct svalue *s, int got_retval);
#endif

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
      if (!debug_gc_check (f, " as frame on stack") &&
          f->current_object && f->current_program) {
	gc_mark_external (f->current_object, " in current_object in frame on stack");
	gc_mark_external (f->current_program, " in current_program in frame on stack");
	if (f->locals) {		/* Check really needed? */
	  if (f->flags & PIKE_FRAME_MALLOCED_LOCALS) {
	    gc_mark_external_svalues(f->locals - 1, 1,
				     " in malloced locals of trampoline frame on stack");
	  } else {
	    if (f->locals > stack_p || (stack_p - f->locals) >= 0x10000) {
              Pike_fatal("Unreasonable locals: stack:%p locals:%p\n",
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

static void gc_check_stack_callback(struct callback *UNUSED(foo), void *UNUSED(bar), void *UNUSED(gazonk))
{
  if (Pike_interpreter.evaluator_stack
#ifdef PIKE_DEBUG
      /* Avoid this if the thread is swapped out. Useful when calling
       * locate_references from gdb. */
      && Pike_sp != (void *) -1
#endif
     )
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

PMOD_EXPORT int low_init_interpreter(struct Pike_interpreter_struct *interpreter)
{
#ifdef USE_MMAP_FOR_STACK
  static int fd = -1;

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

  interpreter->trace_level = default_t_flag;

  return 0;	/* OK. */
}

static struct pike_frame *free_pike_frame;

PMOD_EXPORT void init_interpreter(void)
{
#ifdef USE_VALGRIND
  {
    static int create_mempool = 1;

    if (create_mempool) {
      PIKE_MEMPOOL_CREATE(&free_pike_frame);
      create_mempool = 0;
    }
  }
#endif
  if (low_init_interpreter(Pike_interpreter_pointer)) {
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
#ifdef PIKE_USE_MACHINE_CODE
  {
    static int tables_need_init=1;
    if(tables_need_init) {
      /* Initialize the fcode_to_opcode table / jump labels. */
#if !defined(OPCODE_INLINE_RETURN)
      eval_instruction(NULL);
#endif
      tables_need_init=0;
#ifdef INIT_INTERPRETER_STATE
      INIT_INTERPRETER_STATE();
#endif
    }
  }
#endif /* PIKE_USE_MACHINE_CODE */
}

/*
 * lvalues are stored in two svalues in one of these formats:
 * array[index]   : { array, index }
 * mapping[index] : { mapping, index }
 * multiset[index] : { multiset, index }
 * object[index] : { object, index } (external object indexing)
 * local variable : { svalue pointer (T_SVALUE_PTR), nothing (T_VOID) }
 * global variable : { object, identifier index (T_OBJ_INDEX) } (internal object indexing)
 * lvalue array: { T_ARRAY_LVALUE, array with lvalue pairs }
 * raw getter/setter: { function (T_FUNCTION), reserved (T_VOID) }
 *
 * Notes:
 *   Raw getter/setters were added in Pike 9.0. They return the value
 *   that they had on entry, and (if they get an argument) set their
 *   value to the argument.
 */

int lvalue_to_svalue_no_free(struct svalue *to, struct svalue *lval)
{
  int run_time_type;
  switch(run_time_type = TYPEOF(*lval))
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
	types |= 1 << TYPEOF(ITEM(a)[e]);
      }
      a->type_field = types;
      SET_SVAL(*to, T_ARRAY, 0, array, a);
      UNSET_ONERROR(err);
      break;
    }

    case T_SVALUE_PTR:
      dmalloc_touch_svalue(lval->u.lval);
      assign_svalue_no_free(to, lval->u.lval);
      break;

    case T_OBJECT:
      /* FIXME: Index subtypes! */
      if (TYPEOF(lval[1]) == T_OBJ_INDEX)
	run_time_type = low_object_index_no_free(to, lval->u.object,
						 lval[1].u.identifier);
      else
	run_time_type = object_index_no_free(to, lval->u.object,
					     SUBTYPEOF(*lval), lval+1);
      break;

    case T_ARRAY:
      simple_array_index_no_free(to, lval->u.array, lval+1);
      break;

    case T_MAPPING:
      mapping_index_no_free(to, lval->u.mapping, lval+1);
      break;

    case T_MULTISET:
      if(multiset_member(lval->u.multiset,lval+1))
      {
	SET_SVAL(*to, T_INT, NUMBER_NUMBER, integer, 1);
      }else{
	SET_SVAL(*to, T_INT, NUMBER_UNDEFINED, integer, 0);
      }
      break;

    case T_FUNCTION:
      apply_svalue(lval, 0);
      *to = Pike_sp[-1];
      Pike_sp--;
      break;

    default:
      index_error(0, 0, lval, lval+1, NULL);
  }
  return run_time_type;
}

PMOD_EXPORT void atomic_get_set_lvalue(struct svalue *lval,
				       struct svalue *from_to)
{
  struct svalue tmp;

  switch(TYPEOF(*lval))
  {
#if 0
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
	types |= 1 << TYPEOF(ITEM(a)[e]);
      }
      a->type_field = types;
      SET_SVAL(*to, T_ARRAY, 0, array, a);
      UNSET_ONERROR(err);
      break;
    }
#endif

    case T_SVALUE_PTR:
      dmalloc_touch_svalue(lval->u.lval);
      tmp = *lval->u.lval;
      *lval->u.lval = *from_to;
      *from_to = tmp;
      break;

    case T_OBJECT:
      /* FIXME: Index subtypes! */
      if (TYPEOF(lval[1]) == T_OBJ_INDEX)
	object_low_atomic_get_set_index(lval->u.object,
					lval[1].u.identifier,
					from_to);
      else
	object_atomic_get_set_index(lval->u.object,
				    SUBTYPEOF(*lval), lval+1,
				    from_to);
      break;

    case T_ARRAY:
      if (TYPEOF(lval[1]) == T_INT) {
	array_atomic_get_set(lval->u.array, lval[1].u.integer, from_to);
      } else {
	index_error(0, 0, lval, lval+1,
		    "Invalid array indexing for ?=.\n");
      }
      break;

    case T_MAPPING:
      map_atomic_get_set(lval->u.mapping, lval+1, from_to);
      break;

#if 0
    case T_MULTISET:
      if(multiset_member(lval->u.multiset,lval+1))
      {
	SET_SVAL(*to, T_INT, NUMBER_NUMBER, integer, 1);
      }else{
	SET_SVAL(*to, T_INT, NUMBER_UNDEFINED, integer, 0);
      }
      break;
#endif

    case T_FUNCTION:
      push_svalue(from_to);
      apply_svalue(lval, 1);
      swap_svalues(from_to, Pike_sp-1);
      pop_stack();
      break;

    default:
      index_error(0, 0, lval, lval+1, NULL);
  }
}

PMOD_EXPORT void assign_lvalue(struct svalue *lval,struct svalue *from)
{
  switch(TYPEOF(*lval))
  {
    case T_ARRAY_LVALUE:
    {
      INT32 e;
      int min_len;
      int max_len;
      int more = 0;
      struct svalue *lvals;
      if(TYPEOF(*from) != T_ARRAY)
	Pike_error("Trying to assign combined lvalue from non-array.\n");

      min_len = lval[1].u.array->size;
      max_len = min_len >> 1;
      lvals = ITEM(lval[1].u.array);
      if (min_len & 1) {
        min_len = lvals[0].u.integer;
        if (min_len < 0) {
          more = 1;
          min_len = ~min_len;
          max_len--;
        }
        lvals++;
      } else {
        min_len >>= 1;
      }
      if (from->u.array->size < min_len)
	Pike_error("Not enough values for multiple assign.\n");

      if ((from->u.array->size > max_len) && !more)
	Pike_error("Too many values for multiple assign.\n");

      for(e = 0; e < min_len; e++)
        assign_lvalue(lvals + (e<<1), ITEM(from->u.array) + e);

      if (max_len > min_len) {
        for(;e < max_len; e++) {
          if (e >= from->u.array->size) {
            assign_lvalue(lvals + (e<<1), (struct svalue *)&svalue_undefined);
          } else {
            assign_lvalue(lvals + (e<<1), ITEM(from->u.array) + e);
          }
        }
      }

      if (more) {
        if (e >= from->u.array->size) {
          ref_push_array(&empty_array);
        } else {
          push_array(slice_array(from->u.array, e, from->u.array->size));
        }
        assign_lvalue(lvals + (e<<1), Pike_sp-1);
        pop_stack();
      }
    }
    break;

  case T_SVALUE_PTR:
    dmalloc_touch_svalue(from);
    dmalloc_touch_svalue(lval->u.lval);
    assign_svalue(lval->u.lval,from);
    break;

  case T_OBJECT:
    /* FIXME: Object subtypes! */
    if (TYPEOF(lval[1]) == T_OBJ_INDEX)
      object_low_set_index (lval->u.object, lval[1].u.identifier, from);
    else
      object_set_index(lval->u.object, SUBTYPEOF(*lval), lval+1, from);
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

  case T_FUNCTION:
    push_svalue(from);
    apply_svalue(lval, 1);
    pop_stack();
    break;

  default:
    index_error(0, 0, lval, lval+1, NULL);
  }
}

/* On error callback. lvalue is followed by value to assign. */
static void o_assign_lvalue(struct svalue *lvalue)
{
  assign_lvalue(lvalue, lvalue+2);
}

union anything *get_pointer_if_this_type(struct svalue *lval, TYPE_T t)
{
  switch(TYPEOF(*lval))
  {
    case T_ARRAY_LVALUE:
      return 0;

    case T_SVALUE_PTR:
      dmalloc_touch_svalue(lval->u.lval);
      if(TYPEOF(*(lval->u.lval)) == t) return & ( lval->u.lval->u );
      return 0;

    case T_OBJECT:
      /* FIXME: What about object subtypes? */
      return object_get_item_ptr(lval->u.object, SUBTYPEOF(*lval), lval+1, t);

    case T_ARRAY:
      return array_get_item_ptr(lval->u.array,lval+1,t);

    case T_MAPPING:
      return mapping_get_item_ptr(lval->u.mapping,lval+1,t);

    case T_MULTISET: return 0;

    case T_FUNCTION:
      return 0;

    default:
      index_error(0, 0, lval, lval+1, NULL);
      return 0;
  }
}

#ifdef PIKE_DEBUG

static inline void pike_trace(int level,char *fmt, ...) ATTRIBUTE((format (printf, 2, 3)));
static inline void pike_trace(int level,char *fmt, ...)
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
              (long)(last - p->inherits),
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
= {-4711, -4711, -4711, -4711, (size_t) -4711, -4711,
   NULL, NULL, NULL, NULL, 0, NULL}
#endif
;

/* Find the lexical scope @[depth] levels out.
 *
 * @[loc]:
 *   Input:
 *     struct object *o		// object to start from.
 *     struct inherit *inherit	// inherit in o->prog.
 *    (int parent_identifier)	// identifier in o to start from.
 *                              // Only if depth == 0.
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
         (long)(loc->o->prog ? loc->inherit - loc->o->prog->inherits : 0)));

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
	       "-   inh: %"PRINTPTRDIFFT"d\n",
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
      TRACE((5, "-   loc->inherit: %"PRINTPTRDIFFT"d\n",
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
           p ? (long)(loc->inherit - p->inherits) : -1));

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
    struct byte_buffer buf = BUFFER_INIT();

    safe_describe_svalue(&buf, Pike_sp-1,0,0);
    if(buffer_content_length(&buf) > TRACE_LEN)
    {
      buffer_remove(&buf, buffer_content_length(&buf) - TRACE_LEN);
      buffer_add_str(&buf, "...");
    }
    fprintf(stderr,"-    value: %s\n",buffer_get_string(&buf));
    buffer_free(&buf);
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

#define BACKLOG 100
struct backlog
{
  PIKE_INSTR_T instruction;
  INT32 arg,arg2;
  INT32 program_id;
  PIKE_OPCODE_T *pc;
#ifdef _REENTRANT
  struct thread_state *thread_state;
#endif
  ptrdiff_t stack;
  ptrdiff_t mark_stack;
};

struct backlog backlog[BACKLOG];
int backlogp=BACKLOG-1;

static inline void low_debug_instr_prologue (PIKE_INSTR_T instr)
{
  if(Pike_interpreter.trace_level > 2)
  {
    char *file = NULL, *f;
    struct pike_string *filep;
    INT_TYPE linep;

    filep = get_line(Pike_fp->pc,Pike_fp->context->prog,&linep);
    if (filep && !filep->size_shift) {
      file = filep->str;
      while((f=strchr(file,'/')))
	file=f+1;
    }
    fprintf(stderr,"- %s:%4ld:%p(%"PRINTPTRDIFFT"d): "
	    "%-25s %4"PRINTPTRDIFFT"d %4"PRINTPTRDIFFT"d\n",
	    file ? file : "-",(long)linep,
	    Pike_fp->pc, Pike_fp->pc - Pike_fp->context->prog->program,
	    get_opcode_name(instr),
	    Pike_sp-Pike_interpreter.evaluator_stack,
	    Pike_mark_sp-Pike_interpreter.mark_stack);
    free_string(filep);
  }

  if(instr + F_OFFSET < F_MAX_OPCODE)
    ADD_RUNNED(instr);

  if(d_flag )
  {
    backlogp++;
    if(backlogp >= BACKLOG) backlogp=0;

    backlog[backlogp].program_id = Pike_fp->context->prog->id;
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

    INVALIDATE_SVAL(Pike_sp[0]);
    INVALIDATE_SVAL(Pike_sp[1]);
    INVALIDATE_SVAL(Pike_sp[2]);
    INVALIDATE_SVAL(Pike_sp[3]);

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


    /* The locals will not be correct when running FILL_STACK
       (actually, they will always be incorrect before running FILL_STACK,
       but at least currently that is the first opcode run).
    */

    /* as it turns out, this is no longer true.. */
    /* if( instr+F_OFFSET != F_FILL_STACK ) */
    /* { */
    /*     if(/\* Pike_fp->fun>=0 && *\/ Pike_fp->current_object->prog && */
    /*        Pike_fp->locals+Pike_fp->num_locals > Pike_sp) */
    /*         Pike_fatal("Stack error (stupid! %p %p+%x).\n",Pike_sp, */
    /*                    Pike_fp->locals, Pike_fp->num_locals*sizeof(struct svalue)); */
    /* } */

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
	/* FALLTHRU */

      case 2:
	check_object_context(Pike_fp->current_object,
			     Pike_fp->context->prog,
			     Pike_fp->current_object->storage+
			     Pike_fp->context->storage_offset);
	/* FALLTHRU */

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

PMOD_EXPORT void dump_backlog(void)
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
    struct program *p;
    e++;
    if(e>=BACKLOG) e=0;

    p = id_to_program (backlog[e].program_id);
    if (p)
    {
      struct pike_string *file;
      INT_TYPE line;

#ifdef _REENTRANT
      if(thread != backlog[e].thread_state)
      {
	fprintf(stderr,"[Thread swap, Pike_interpreter.thread_state=%p]\n",backlog[e].thread_state);
	thread = backlog[e].thread_state;
      }
#endif

      file = get_line(backlog[e].pc,p, &line);
      if(backlog[e].instruction+F_OFFSET > F_MAX_OPCODE)
      {
	fprintf(stderr,"%s:%ld:(%"PRINTPTRDIFFT"d): ILLEGAL INSTRUCTION %d\n",
		file->str,
		(long)line,
		backlog[e].pc - p->program,
		backlog[e].instruction + F_OFFSET);
	free_string(file);
	continue;
      }

      fprintf(stderr,"%s:%ld:(%"PRINTPTRDIFFT"d): %s",
	      file->str,
	      (long)line,
	      backlog[e].pc - p->program,
	      low_get_f_name(backlog[e].instruction + F_OFFSET, p));
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
      fprintf(stderr," %ld, %ld\n", (long)backlog[e].stack,
              (long)backlog[e].mark_stack);
      free_string(file);
    }
  }while(e!=backlogp);
}

#else  /* PIKE_DEBUG */

#define DEBUG_LOG_ARG(arg) 0
#define DEBUG_LOG_ARG2(arg2) 0

#endif	/* !PIKE_DEBUG */

#ifdef OPCODE_INLINE_CATCH
#define POP_CATCH_CONTEXT do {                                         \
    struct catch_context *cc = Pike_interpreter.catch_ctx;             \
    DO_IF_DEBUG (                                                      \
      TRACE((3,"-   Popping catch context %p ==> %p\n",                        \
            cc, cc ? cc->prev : NULL));                                \
      if (!cc)                                                         \
       Pike_fatal ("Catch context dropoff.\n");                        \
      if (cc->frame != Pike_fp)                                                \
       Pike_fatal ("Catch context doesn't belong to this frame.\n");   \
      if (Pike_mark_sp != cc->recovery.mark_sp + Pike_interpreter.mark_stack) \
       Pike_fatal ("Mark sp diff in catch context pop.\n");            \
    );                                                                 \
    debug_malloc_touch (cc);                                           \
    UNSETJMP (cc->recovery);                                           \
    Pike_interpreter.catch_ctx = cc->prev;                             \
    really_free_catch_context (cc);                                    \
  } while (0)
#else
#define POP_CATCH_CONTEXT do {                                         \
    struct catch_context *cc = Pike_interpreter.catch_ctx;             \
    DO_IF_DEBUG (                                                      \
      TRACE((3,"-   Popping catch context %p ==> %p\n",                        \
            cc, cc ? cc->prev : NULL));                                \
      if (!Pike_interpreter.catching_eval_jmpbuf)                      \
       Pike_fatal ("Not in catching eval.\n");                         \
      if (!cc)                                                         \
       Pike_fatal ("Catch context dropoff.\n");                        \
      if (cc->frame != Pike_fp)                                                \
       Pike_fatal ("Catch context doesn't belong to this frame.\n");   \
      if (Pike_mark_sp != cc->recovery.mark_sp + Pike_interpreter.mark_stack) \
       Pike_fatal ("Mark sp diff in catch context pop.\n");            \
    );                                                                 \
    debug_malloc_touch (cc);                                           \
    UNSETJMP (cc->recovery);                                           \
    Pike_interpreter.catch_ctx = cc->prev;                             \
    really_free_catch_context (cc);                                    \
  } while (0)
#endif

static struct catch_context *free_catch_context;
static int num_catch_ctx, num_free_catch_ctx;

PMOD_EXPORT void really_free_catch_context( struct catch_context *data )
{
    if( num_free_catch_ctx > 100 && free_catch_context )
    {
      num_catch_ctx--;
      free( data );
    }
    else
    {
      data->prev = free_catch_context;

      num_free_catch_ctx++;
      PIKE_MEM_NA(*data);
      PIKE_MEM_RW(data->prev);
      free_catch_context = data;
    }
}

struct catch_context *alloc_catch_context(void)
{
    struct catch_context *res;
    if( free_catch_context )
    {
        num_free_catch_ctx--;
        res = free_catch_context;
        PIKE_MEM_RW(res->prev);
        free_catch_context = res->prev;
        PIKE_MEM_WO(*res);
    }
    else
    {
        num_catch_ctx++;
        res = ALLOC_STRUCT( catch_context );
    }
    return res;
}

void count_memory_in_catch_contexts(size_t *num, size_t *size )
{
  *num = (num_catch_ctx-num_free_catch_ctx);
  *size = num_catch_ctx * (sizeof(struct catch_context)+8); /* assumes 8 bytes overhead. */
}

#ifdef DO_PIKE_CLEANUP
static void free_all_catch_context_blocks(void)
{
    struct catch_context *x = free_catch_context, *n;
    while( x )
    {
        PIKE_MEM_RW(x->prev);
        n = x->prev;
        free( x );
        x = n;
    }
    free_catch_context = NULL;
}
#endif

static int catching_eval_instruction (PIKE_OPCODE_T *pc);


#ifdef PIKE_USE_MACHINE_CODE
#ifdef OPCODE_INLINE_RETURN
/* Catch notes:
 *
 * Typical F_CATCH use:
 *
 *   F_CATCH
 * 	F_PTR	continue_label
 *
 *   ENTRY
 *
 *   catch body
 *
 *   F_EXIT_CATCH
 *
 *   F_BRANCH
 *	F_PTR	continue_label
 *
 *   ENTRY
 *
 * continue_label:
 *
 *   rest of code.
 */

/* Modified calling-conventions to simplify code-generation when
 * INTER_RETURN is inlined.
 *
 * cf interpret_functions.h:F_CATCH
 *
 * Arguments:
 *   addr:
 *     Address where the continue POINTER (INT32) is stored.
 *     Directly after the POINTER is the ENTRY for the catch block.
 *
 * Returns:
 *   (PIKE_OPCODE_T *)-1 on INTER_RETURN.
 *   jump_destination otherwise.
 */
PIKE_OPCODE_T *inter_return_opcode_F_CATCH(PIKE_OPCODE_T *addr)
{
#ifdef PIKE_DEBUG
  if (d_flag || Pike_interpreter.trace_level > 2) {
    low_debug_instr_prologue (F_CATCH - F_OFFSET);
    if (Pike_interpreter.trace_level>3) {
      sprintf(trace_buffer, "-    Addr = %p\n", addr);
      write_to_stderr(trace_buffer,strlen(trace_buffer));
    }
  }
#endif
  {
    struct catch_context *new_catch_ctx = alloc_catch_context();
#ifdef PIKE_DEBUG
      new_catch_ctx->frame = Pike_fp;
      init_recovery (&new_catch_ctx->recovery, 0, 0, PERR_LOCATION());
#else
      init_recovery (&new_catch_ctx->recovery, 0);
#endif
    new_catch_ctx->continue_reladdr = (INT32)get_unaligned32(addr)
      /* We need to run the entry prologue... */
      - ENTRY_PROLOGUE_SIZE;

    new_catch_ctx->next_addr = addr;
    new_catch_ctx->prev = Pike_interpreter.catch_ctx;
    Pike_interpreter.catch_ctx = new_catch_ctx;
    DO_IF_DEBUG({
	TRACE((3,"-   Pushed catch context %p\n", new_catch_ctx));
      });
  }

  /* Need to adjust next_addr by sizeof(INT32) to skip past the jump
   * address to the continue position after the catch block. */
  addr = (PIKE_OPCODE_T *) ((INT32 *) addr + 1);

  if (Pike_interpreter.catching_eval_jmpbuf) {
    /* There's already a catching_eval_instruction around our
     * eval_instruction, so we can just continue. */
    debug_malloc_touch_named (Pike_interpreter.catch_ctx, "(1)");
    /* Skip past the entry prologue... */
    addr += ENTRY_PROLOGUE_SIZE;
    DO_IF_DEBUG({
	TRACE((3,"-   In active catch; continuing at %p\n", addr));
      });
    return addr;
  }
  else {
    debug_malloc_touch_named (Pike_interpreter.catch_ctx, "(2)");

    while (1) {
      /* Loop here every time an exception is caught. Once we've
       * gotten here and set things up to run eval_instruction from
       * inside catching_eval_instruction, we keep doing it until it's
       * time to return. */

      int res;

      DO_IF_DEBUG({
	  TRACE((3,"-   Activating catch; calling %p in context %p\n",
		 addr, Pike_interpreter.catch_ctx));
	});

      res = catching_eval_instruction (addr);

      DO_IF_DEBUG({
	  TRACE((3,"-   catching_eval_instruction(%p) returned %d\n",
		 addr, res));
	});

      if (res != -3) {
	/* There was an inter return inside the evaluated code. Just
	 * propagate it. */
	DO_IF_DEBUG ({
	    TRACE((3,"-   Returning from catch.\n"));
	    if (res != -1) Pike_fatal ("Unexpected return value from "
				       "catching_eval_instruction: %d\n", res);
	  });
	break;
      }

      else {
	/* Caught an exception. */
	struct catch_context *cc = Pike_interpreter.catch_ctx;

	DO_IF_DEBUG ({
	    TRACE((3,"-   Caught exception. catch context: %p\n", cc));
	    if (!cc) Pike_fatal ("Catch context dropoff.\n");
	    if (cc->frame != Pike_fp)
	      Pike_fatal ("Catch context doesn't belong to this frame.\n");
	  });

	debug_malloc_touch_named (cc, "(3)");
	UNSETJMP (cc->recovery);
	move_svalue (Pike_sp++, &throw_value);
	mark_free_svalue (&throw_value);
	low_destruct_objects_to_destruct();

	if (cc->continue_reladdr < 0)
	  FAST_CHECK_THREADS_ON_BRANCH();
	addr = cc->next_addr + cc->continue_reladdr;

	DO_IF_DEBUG({
	    TRACE((3,"-   Popping catch context %p ==> %p\n",
		   cc, cc->prev));
	    if (!addr) Pike_fatal ("Unexpected null continue addr.\n");
	  });

	Pike_interpreter.catch_ctx = cc->prev;
	really_free_catch_context (cc);
      }
    }

    return (PIKE_OPCODE_T *)(ptrdiff_t)-1;	/* INTER_RETURN; */
  }
}

/* Modified calling-conventions to simplify code-generation when
 * INTER_RETURN is inlined.
 *
 * cf interpret_functions.h:F_CATCH_AT
 *
 * Arguments:
 *   addr:
 *     Address where the continue POINTER (INT32) is stored.
 *     Directly after the POINTER is the ENTRY for the catch block.
 *   Pike_sp[-2]:
 *     PIKE_T_INT containing offset from frame_get_save_sp(Pike_fp)
 *     for restoration point for Pike_sp.
 *   Pike_sp[-1]:
 *     PIKE_T_INT containing offset from Pike_fp->save_mark_sp
 *     for restoration point for Pike_mark_sp.
 *
 * Returns:
 *   Decreases Pike_sp with 2.
 *   (PIKE_OPCODE_T *)-1 on INTER_RETURN.
 *   jump_destination otherwise.
 */
PIKE_OPCODE_T *inter_return_opcode_F_CATCH_AT(PIKE_OPCODE_T *addr)
{
#ifdef PIKE_DEBUG
  if (d_flag || Pike_interpreter.trace_level > 2) {
    low_debug_instr_prologue (F_CATCH - F_OFFSET);
    if (Pike_interpreter.trace_level>3) {
      sprintf(trace_buffer, "-    Addr = %p\n", addr);
      write_to_stderr(trace_buffer,strlen(trace_buffer));
    }
  }
#endif
  {
    struct catch_context *new_catch_ctx;

    if ((TYPEOF(Pike_sp[-1]) != PIKE_T_INT) ||
	(TYPEOF(Pike_sp[-2]) != PIKE_T_INT)) {
      Pike_error("CATCH_AT: Invalid arguments.\n");
    }

    new_catch_ctx = alloc_catch_context();
#ifdef PIKE_DEBUG
      new_catch_ctx->frame = Pike_fp;
      init_recovery (&new_catch_ctx->recovery, 0, 0, PERR_LOCATION());
#else
      init_recovery (&new_catch_ctx->recovery, 0);
#endif

    new_catch_ctx->recovery.stack_pointer =
      frame_get_save_sp(Pike_fp) + Pike_sp[-2].u.integer -
      Pike_interpreter.evaluator_stack;
    new_catch_ctx->recovery.mark_sp =
      Pike_fp->save_mark_sp + Pike_sp[-1].u.integer -
      Pike_interpreter.mark_stack;
    Pike_sp -= 2;

    new_catch_ctx->continue_reladdr = (INT32)get_unaligned32(addr)
      /* We need to run the entry prologue... */
      - ENTRY_PROLOGUE_SIZE;

    new_catch_ctx->next_addr = addr;
    new_catch_ctx->prev = Pike_interpreter.catch_ctx;
    Pike_interpreter.catch_ctx = new_catch_ctx;
    DO_IF_DEBUG({
	TRACE((3,"-   Pushed catch context %p\n", new_catch_ctx));
      });
  }

  /* Need to adjust next_addr by sizeof(INT32) to skip past the jump
   * address to the continue position after the catch block. */
  addr = (PIKE_OPCODE_T *) ((INT32 *) addr + 1);

  if (Pike_interpreter.catching_eval_jmpbuf) {
    /* There's already a catching_eval_instruction around our
     * eval_instruction, so we can just continue. */
    debug_malloc_touch_named (Pike_interpreter.catch_ctx, "(1)");
    /* Skip past the entry prologue... */
    addr += ENTRY_PROLOGUE_SIZE;
    DO_IF_DEBUG({
	TRACE((3,"-   In active catch; continuing at %p\n", addr));
      });
    return addr;
  }
  else {
    debug_malloc_touch_named (Pike_interpreter.catch_ctx, "(2)");

    while (1) {
      /* Loop here every time an exception is caught. Once we've
       * gotten here and set things up to run eval_instruction from
       * inside catching_eval_instruction, we keep doing it until it's
       * time to return. */

      int res;

      DO_IF_DEBUG({
	  TRACE((3,"-   Activating catch; calling %p in context %p\n",
		 addr, Pike_interpreter.catch_ctx));
	});

      res = catching_eval_instruction (addr);

      DO_IF_DEBUG({
	  TRACE((3,"-   catching_eval_instruction(%p) returned %d\n",
		 addr, res));
	});

      if (res != -3) {
	/* There was an inter return inside the evaluated code. Just
	 * propagate it. */
	DO_IF_DEBUG ({
	    TRACE((3,"-   Returning from catch.\n"));
	    if (res != -1) Pike_fatal ("Unexpected return value from "
				       "catching_eval_instruction: %d\n", res);
	  });
	break;
      }

      else {
	/* Caught an exception. */
	struct catch_context *cc = Pike_interpreter.catch_ctx;

	DO_IF_DEBUG ({
	    TRACE((3,"-   Caught exception. catch context: %p\n", cc));
	    if (!cc) Pike_fatal ("Catch context dropoff.\n");
	    if (cc->frame != Pike_fp)
	      Pike_fatal ("Catch context doesn't belong to this frame.\n");
	  });

	debug_malloc_touch_named (cc, "(3)");
	UNSETJMP (cc->recovery);
	move_svalue (Pike_sp++, &throw_value);
	mark_free_svalue (&throw_value);
	low_destruct_objects_to_destruct();

	if (cc->continue_reladdr < 0)
	  FAST_CHECK_THREADS_ON_BRANCH();
	addr = cc->next_addr + cc->continue_reladdr;

	DO_IF_DEBUG({
	    TRACE((3,"-   Popping catch context %p ==> %p\n",
		   cc, cc->prev));
	    if (!addr) Pike_fatal ("Unexpected null continue addr.\n");
	  });

	Pike_interpreter.catch_ctx = cc->prev;
	really_free_catch_context (cc);
      }
    }

    return (PIKE_OPCODE_T *)(ptrdiff_t)-1;	/* INTER_RETURN; */
  }
}

void *do_inter_return_label = (void*)(ptrdiff_t)-1;
#else /* OPCODE_INLINE_RETURN */
/* Labels to jump to to cause eval_instruction to return */
/* FIXME: Replace these with assembler lables */
void *do_inter_return_label = NULL;
void *dummy_label = NULL;
#endif /* OPCODE_INLINE_RETURN */

#ifdef OPCODE_INLINE_CATCH
/* Helper function for F_CATCH machine code.
   For a description of the addr argument, see inter_return_opcode_F_CATCH.
   Returns the jump destination (for the catch body). */
PIKE_OPCODE_T *setup_catch_context(PIKE_OPCODE_T *addr)
{
#ifdef PIKE_DEBUG
  if (d_flag || Pike_interpreter.trace_level > 2) {
    low_debug_instr_prologue (F_CATCH - F_OFFSET);
    if (Pike_interpreter.trace_level>3) {
      sprintf(trace_buffer, "-    Addr = %p\n", addr);
      write_to_stderr(trace_buffer,strlen(trace_buffer));
    }
  }
#endif
  {
    struct catch_context *new_catch_ctx = alloc_catch_context();
#ifdef PIKE_DEBUG
      new_catch_ctx->frame = Pike_fp;
      init_recovery (&new_catch_ctx->recovery, 0, 0, PERR_LOCATION());
#else
      init_recovery (&new_catch_ctx->recovery, 0);
#endif

    /* Note: no prologue. */
    new_catch_ctx->continue_reladdr = (INT32)get_unaligned32(addr);

    new_catch_ctx->next_addr = addr;
    new_catch_ctx->prev = Pike_interpreter.catch_ctx;
    Pike_interpreter.catch_ctx = new_catch_ctx;
    DO_IF_DEBUG({
	TRACE((3,"-   Pushed catch context %p\n", new_catch_ctx));
      });
  }

  /* Need to adjust next_addr by sizeof(INT32) to skip past the jump
   * address to the continue position after the catch block. */
  return (PIKE_OPCODE_T *) ((INT32 *) addr + 1) + ENTRY_PROLOGUE_SIZE;
}

/* Helper function for F_CATCH_AT machine code.
   For a description of the addr argument, see inter_return_opcode_F_CATCH_AT.
   Returns the jump destination (for the catch body). */
PIKE_OPCODE_T *setup_catch_at_context(PIKE_OPCODE_T *addr)
{
#ifdef PIKE_DEBUG
  if (d_flag || Pike_interpreter.trace_level > 2) {
    low_debug_instr_prologue (F_CATCH - F_OFFSET);
    if (Pike_interpreter.trace_level>3) {
      sprintf(trace_buffer, "-    Addr = %p\n", addr);
      write_to_stderr(trace_buffer,strlen(trace_buffer));
    }
  }
#endif
  {
    struct catch_context *new_catch_ctx;

    if ((TYPEOF(Pike_sp[-1]) != PIKE_T_INT) ||
	(TYPEOF(Pike_sp[-2]) != PIKE_T_INT)) {
      Pike_error("CATCH_AT: Invalid arguments.\n");
    }

    new_catch_ctx = alloc_catch_context();
#ifdef PIKE_DEBUG
      new_catch_ctx->frame = Pike_fp;
      init_recovery (&new_catch_ctx->recovery, 0, 0, PERR_LOCATION());
#else
      init_recovery (&new_catch_ctx->recovery, 0);
#endif

    new_catch_ctx->recovery.stack_pointer =
      frame_get_save_sp(Pike_fp) + Pike_sp[-2].u.integer -
      Pike_interpreter.evaluator_stack;
    new_catch_ctx->recovery.mark_sp =
      Pike_fp->save_mark_sp + Pike_sp[-1].u.integer -
      Pike_interpreter.mark_stack;
    Pike_sp -= 2;

    /* Note: no prologue. */
    new_catch_ctx->continue_reladdr = (INT32)get_unaligned32(addr);

    new_catch_ctx->next_addr = addr;
    new_catch_ctx->prev = Pike_interpreter.catch_ctx;
    Pike_interpreter.catch_ctx = new_catch_ctx;
    DO_IF_DEBUG({
	TRACE((3,"-   Pushed catch context %p\n", new_catch_ctx));
      });
  }

  /* Need to adjust next_addr by sizeof(INT32) to skip past the jump
   * address to the continue position after the catch block. */
  return (PIKE_OPCODE_T *) ((INT32 *) addr + 1) + ENTRY_PROLOGUE_SIZE;
}

/* Helper function for F_CATCH machine code. Called when an exception
   is caught. Pops the catch context and returns the continue jump
   destination. */
PIKE_OPCODE_T *handle_caught_exception(void)
{
  /* Caught an exception. */
  struct catch_context *cc = Pike_interpreter.catch_ctx;
  PIKE_OPCODE_T *addr;

  DO_IF_DEBUG ({
      TRACE((3,"-   Caught exception. catch context: %p\n", cc));
      if (!cc) Pike_fatal ("Catch context dropoff.\n");
      if (cc->frame != Pike_fp)
	Pike_fatal ("Catch context doesn't belong to this frame.\n");
    });

  debug_malloc_touch_named (cc, "(3)");
  UNSETJMP (cc->recovery);
  move_svalue (Pike_sp++, &throw_value);
  mark_free_svalue (&throw_value);
  low_destruct_objects_to_destruct();

  if (cc->continue_reladdr < 0)
    FAST_CHECK_THREADS_ON_BRANCH();
  addr = cc->next_addr + cc->continue_reladdr;

  DO_IF_DEBUG({
      TRACE((3,"-   Popping catch context %p ==> %p\n",
	     cc, cc->prev));
      if (!addr) Pike_fatal ("Unexpected null continue addr.\n");
    });

  Pike_interpreter.catch_ctx = cc->prev;
  really_free_catch_context (cc);

  return addr;
}

#endif /* OPCODE_INLINE_CATCH */

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

/* Intended to be called from machine code before inlined function
 * calls (primarily the CALL_BUILTIN opcodes), to ensure thread
 * switching. */
void call_check_threads_etc(void)
{
  FAST_CHECK_THREADS_ON_CALL();
}

#if defined(OPCODE_INLINE_BRANCH) || defined(INS_F_JUMP) || \
    defined(INS_F_JUMP_WITH_ARG) || defined(INS_F_JUMP_WITH_TWO_ARGS)
/* Intended to be called from machine code on backward branch jumps,
 * to ensure thread switching. */
void branch_check_threads_etc(void)
{
  FAST_CHECK_THREADS_ON_BRANCH();
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
ptrdiff_t PIKE_CONCAT(test_opcode_,O)(void) { \
    ptrdiff_t branch_taken = 0;	\
    DEF_PROG_COUNTER; \
    DEBUG_PROLOGUE (O, ;);						\
    C; \
    return branch_taken; \
  }

#define TEST_OPCODE1(O,N,F,C) \
ptrdiff_t PIKE_CONCAT(test_opcode_,O)(INT32 arg1) {\
    ptrdiff_t branch_taken = 0;	\
    DEF_PROG_COUNTER; \
    DEBUG_PROLOGUE (O, DEBUG_LOG_ARG (arg1));				\
    C; \
    return branch_taken; \
  }


#define TEST_OPCODE2(O,N,F,C) \
ptrdiff_t PIKE_CONCAT(test_opcode_,O)(INT32 arg1, INT32 arg2) { \
    ptrdiff_t branch_taken = 0;	\
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

#define OPCODE0_RETURN(O,N,F,C) OPCODE0_JUMP(O,N,F | I_RETURN,C)
#define OPCODE1_RETURN(O,N,F,C) OPCODE1_JUMP(O,N,F | I_RETURN,C)
#define OPCODE2_RETURN(O,N,F,C) OPCODE2_JUMP(O,N,F | I_RETURN,C)
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

#define ADVERTISE_FALLTHROUGH
#include "interpret_functions_fixed.h"
#undef ADVERTISE_FALLTHROUGH

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
    if (((ptrdiff_t)pc) % PIKE_OPCODE_ALIGN) {
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
#ifndef OPCODE_INLINE_RETURN
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
#endif /* !OPCODE_INLINE_RETURN */
    CALL_MACHINE_CODE(pc);

#ifndef OPCODE_INLINE_RETURN
    /* This code is never reached, but will
     * prevent gcc from optimizing the labels below too much
     */

    DWERR("We have reached the end of the world!\n");
  }

#ifdef __GNUC__
  goto *dummy_label;
#endif

 inter_return_label:
#endif /*!OPCODE_INLINE_RETURUN */
#ifdef PIKE_DEBUG
  pike_trace(3, "-    Inter return\n");
#endif
  EXIT_MACHINE_CODE();
  return -1;
}

#else /* PIKE_USE_MACHINE_CODE */


#ifndef SET_PROG_COUNTER
#define SET_PROG_COUNTER(X)	(PROG_COUNTER=(X))
#endif /* SET_PROG_COUNTER */


#if defined(PIKE_DEBUG)
#define eval_instruction eval_instruction_with_debug
#include "interpreter.h"
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
  if(d_flag || Pike_interpreter.trace_level>2)
    return eval_instruction_with_debug(pc);
  else
    return eval_instruction_without_debug(pc);
}


#else /* !PIKE_DEBUG */
#include "interpreter.h"
#endif


#endif /* PIKE_USE_MACHINE_CODE */

#undef REAL_PIKE_DEBUG
#undef DO_IF_REAL_DEBUG
#undef DO_IF_NOT_REAL_DEBUG


ATTRIBUTE((noinline))
static void do_trace_call(struct byte_buffer *b, INT32 args)
{
  struct pike_string *filep = NULL;
  char *file;
  const char *s;
  INT_TYPE linep;
  INT32 e;
  ptrdiff_t len = 0;

  buffer_add_str(b, "(");
  for(e=0;e<args;e++)
  {
    if(e) buffer_add_str(b, ",");
    safe_describe_svalue(b, Pike_sp-args+e,0,0);
  }
  buffer_add_str(b, ")");

  if(buffer_content_length(b) > TRACE_LEN)
  {
    buffer_remove(b, buffer_content_length(b) - TRACE_LEN);
    buffer_add_str(b, "...");
  }

  if(Pike_fp && Pike_fp->pc)
  {
    char *f;
    filep = get_line(Pike_fp->pc,Pike_fp->context->prog,&linep);
    if (filep->size_shift)
      file = "...";
    else {
      file = filep->str;
      while((f = strchr(file, '/'))
#ifdef __NT__
	    || (f = strchr(file, '\\'))
#endif /* __NT__ */
	    )
	file=f+1;
      len = filep->len - (file - filep->str);
    }
  }else{
    linep=0;
    file="-";
  }

  s = buffer_get_string(b);

  if (len < 30)
  {
    char buf[40];
    if (linep)
      snprintf(buf, sizeof (buf), "%s:%ld:", file, (long)linep);
    else
      snprintf(buf, sizeof (buf), "%s:", file);
    fprintf(stderr, "- %-20s %s\n",buf,s);
  } else if (linep) {
    fprintf(stderr, "- %s:%ld: %s\n", file, (long)linep, s);
  } else {
    fprintf(stderr, "- %s: %s\n", file, s);
  }

  if (filep) {
    free_string(filep);
  }
  buffer_free(b);
}

ATTRIBUTE((noinline))
static void do_trace_function_call(const struct object *o, const struct identifier *function, INT32 args) {
  struct byte_buffer buffer = BUFFER_INIT();
  char buf[50];

  sprintf(buf, "%lx->", (long) PTR_TO_INT (o));
  buffer_add_str(&buffer, buf);
  if (function->name->size_shift)
    buffer_add_str (&buffer, "[widestring function name]");
  else
    buffer_add_str(&buffer, function->name->str);
  do_trace_call(&buffer, args);
}

ATTRIBUTE((noinline))
static void do_trace_efun_call(const struct svalue *s, INT32 args) {
  struct byte_buffer buf = BUFFER_INIT();
  if (s->u.efun->name->size_shift)
    buffer_add_str (&buf, "[widestring function name]");
  else
    buffer_add_str (&buf, s->u.efun->name->str);
  do_trace_call(&buf, args);
}

ATTRIBUTE((noinline))
static void do_trace_svalue_call(const struct svalue *s, INT32 args) {
  struct byte_buffer buf = BUFFER_INIT();
  safe_describe_svalue(&buf, s,0,0);
  do_trace_call(&buf, args);
}


ATTRIBUTE((noinline))
static void do_trace_func_return (int got_retval, struct object *o, int fun)
{
  struct byte_buffer b = BUFFER_INIT();
  if (o) {
    if (o->prog) {
      struct identifier *id = ID_FROM_INT (o->prog, fun);
      char buf[50];
      sprintf(buf, "%lx->", (long) PTR_TO_INT (o));
      buffer_add_str(&b, buf);
      if (id->name->size_shift)
	buffer_add_str (&b, "[widestring function name]");
      else
	buffer_add_str(&b, id->name->str);
      buffer_add_str (&b, "() ");
    }
    else
      buffer_add_str (&b, "function in destructed object ");
  }
  do_trace_return (&b, got_retval);
}

#ifdef PIKE_DEBUG
ATTRIBUTE((noinline))
static void do_trace_efun_return(const struct svalue *s, int got_retval) {
  struct byte_buffer buf = BUFFER_INIT();
  if (s->u.efun->name->size_shift)
    buffer_add_str (&buf, "[widestring function name]");
  else
    buffer_add_str (&buf, s->u.efun->name->str);
  buffer_add_str (&buf, "() ");
  do_trace_return (&buf, got_retval);
}
#endif

ATTRIBUTE((noinline))
static void do_trace_return (struct byte_buffer *b, int got_retval)
{
  struct pike_string *filep = NULL;
  char *file;
  const char *s;
  INT_TYPE linep;

  if (got_retval) {
    buffer_add_str (b, "returns: ");
    safe_describe_svalue(b, Pike_sp-1,0,0);
  }
  else
    buffer_add_str (b, "returns with no value");

  if(buffer_content_length(b) > TRACE_LEN)
  {
    buffer_remove(b, buffer_content_length(b) - TRACE_LEN);
    buffer_add_str(b, "...");
  }

  s = buffer_get_string(b);

  if(Pike_fp && Pike_fp->pc)
  {
    char *f;
    filep = get_line(Pike_fp->pc,Pike_fp->context->prog,&linep);
    if (filep->size_shift)
      file = "...";
    else {
      file = filep->str;
      while((f=strchr(file,'/')))
	file=f+1;
    }
  }else{
    linep=0;
    file="-";
  }

  {
    char buf[40];
    if (linep)
      snprintf(buf, sizeof (buf), "%s:%ld:", file, (long)linep);
    else
      snprintf(buf, sizeof (buf), "%s:", file);
    fprintf(stderr,"- %-20s %s\n",buf,s);
  }

  if (filep) {
    free_string(filep);
  }
  buffer_free(b);
}

static struct pike_frame_chunk {
  struct pike_frame_chunk *next;
} *pike_frame_chunks;
static int num_pike_frame_chunks;
static int num_pike_frames;

PMOD_EXPORT void really_free_pike_frame( struct pike_frame *X )
{
    do_free_object(X->current_object);
    if(X->current_program)
        free_program(X->current_program);
    if(X->scope)
        free_pike_scope(X->scope);
    if (X->flags & PIKE_FRAME_MALLOCED_LOCALS) {
      free_array(X->locals[-1].u.array);
      X->flags &= ~PIKE_FRAME_MALLOCED_LOCALS;
    }
    DO_IF_DEBUG(
        if(X->flags & PIKE_FRAME_MALLOCED_LOCALS)
            Pike_fatal("Pike frame is not supposed to have malloced locals here!\n"));
    if (X->flags & PIKE_FRAME_SAVE_LOCALS) {
      free(X->save_locals_bitmask);
      X->flags &= ~PIKE_FRAME_SAVE_LOCALS;
    }
  DO_IF_DMALLOC(
    X->current_program=0;
    X->context=0;
    X->scope=0;
    X->current_object=0;
    X->flags=0;
    X->locals=0;
  );
  X->next = free_pike_frame;
  PIKE_MEMPOOL_FREE(&free_pike_frame, X, sizeof(struct pike_frame));
  free_pike_frame = X;
}

struct pike_frame *alloc_pike_frame(void)
{
    struct pike_frame *res;
    if( free_pike_frame )
    {
      res = free_pike_frame;
      PIKE_MEMPOOL_ALLOC(&free_pike_frame, res, sizeof(struct pike_frame));
      PIKE_MEM_RW_RANGE(&res->next, sizeof(void*));
      free_pike_frame = res->next;
      PIKE_MEM_WO_RANGE(&res->next, sizeof(void*));
      memset(res, 0, sizeof(struct pike_frame));
      gc_init_marker(res);
      res->refs=0;
      add_ref(res);	/* For DMALLOC... */
      res->flags=0;
      res->next=0;
      res->scope=0;
      res->pc = NULL;

      return res;
    }

    /* Need to allocate more. */
    {
      unsigned int i;
#define FRAMES_PER_CHUNK ((4096*4-8-sizeof(struct pike_frame_chunk))/sizeof(struct pike_frame))
#define FRAME_CHUNK_SIZE (FRAMES_PER_CHUNK*sizeof(struct pike_frame))+sizeof(struct pike_frame_chunk)

      void *p = xalloc( FRAME_CHUNK_SIZE );
      num_pike_frame_chunks++;
      ((struct pike_frame_chunk*)p)->next = pike_frame_chunks;
      pike_frame_chunks = p;
      free_pike_frame = res = (struct pike_frame*)((char*)p+sizeof(struct pike_frame_chunk));
      for( i=1; i<FRAMES_PER_CHUNK; i++ )
      {
        res->next = &free_pike_frame[i];
        res = res->next;
      }
      res->next = NULL;
      num_pike_frames+=FRAMES_PER_CHUNK;
    }
    return alloc_pike_frame();
}

void save_locals(struct pike_frame *frame)
{
  int num_new_locals = 0;
  int num_locals;
  int i;

  if (!frame ||
      ((frame->flags & (PIKE_FRAME_SAVE_LOCALS|PIKE_FRAME_MALLOCED_LOCALS)) !=
       PIKE_FRAME_SAVE_LOCALS)) {
    /* Not relevant or already done. */
    return;
  }

  num_locals = frame->num_locals;

  /* find the highest set bit */
  for (i = num_locals - 1; i >= 0; i--) {
    unsigned INT16 bitmask = frame->save_locals_bitmask[i >> 4];

    if (bitmask & (1 << (i & 0xf))) {
      num_new_locals = i + 1;
      break;
    }
  }

  if (num_new_locals)
  {
    struct array *a = allocate_array(num_new_locals + 1);
    struct svalue *locals = frame->locals;
    unsigned INT16 bitmask = 0;
    struct svalue *s = ITEM(a) + 1;

    /* Self-reference for reference-counting purposes. */
    SET_SVAL(ITEM(a)[0], PIKE_T_ARRAY, 0, array, a);
    /* NB: Make sure that the self-reference is not counted as
     *     part of the actual array contents for gc purposes.
     */
    s = ++ITEM(a);
    a->size--;

    for (i = 0; i < num_new_locals; i++) {
      unsigned INT16 bitmask = frame->save_locals_bitmask[i >> 4];

      if (bitmask & (1 << (i & 0xf))) {
        assign_svalue_no_free(s + i, locals + i);
      }
    }

    frame->locals = s;
    frame->flags |= PIKE_FRAME_MALLOCED_LOCALS;
  } else {
    frame->locals = NULL;
  }

  frame->flags &= ~PIKE_FRAME_SAVE_LOCALS;

  free(frame->save_locals_bitmask);

  frame->num_locals = num_new_locals;
}

void LOW_POP_PIKE_FRAME_slow_path(struct pike_frame *frame)
{
  debug_malloc_touch(frame);

  if (frame->flags & PIKE_FRAME_MALLOCED_LOCALS) {
    /* Already done. */
  } else if (frame->flags & PIKE_FRAME_SAVE_LOCALS) {
    save_locals(frame);
  } else if (!(frame->flags & PIKE_FRAME_MALLOCED_LOCALS)) {
    frame->locals = NULL;
    frame->num_locals = 0;
  }

  frame->next=0;
}

void count_memory_in_pike_frames(size_t *num, size_t *size )
{
  *num = num_pike_frames;
  *size = num_pike_frame_chunks * (FRAME_CHUNK_SIZE*8);
}
#undef FRAMES_PER_CHUNK
#undef FRAME_CHUNK_SIZE

#ifdef DO_PIKE_CLEANUP
static void free_all_pike_frame_blocks(void)
{
  struct pike_frame_chunk *x = pike_frame_chunks, *n;
  while( x )
  {
    n = x->next;
    free(x);
    x = n;
  }
  free_pike_frame = NULL;
  pike_frame_chunks = NULL;
  num_pike_frames=0;
  num_pike_frame_chunks=0;
}
#endif

void really_free_pike_scope(struct pike_frame *scope)
{
  if(scope->flags & PIKE_FRAME_MALLOCED_LOCALS)
  {
    struct array *a = scope->locals[-1].u.array;

    free_array(a);
    scope->locals = NULL;

    scope->flags &= ~PIKE_FRAME_MALLOCED_LOCALS;
  }
  really_free_pike_frame(scope);
}

void *lower_mega_apply( INT32 args, struct object *o, ptrdiff_t fun )
{
  struct program *p;
  check_stack(256);
  check_mark_stack(256);

#ifdef PIKE_USE_MACHINE_CODE
  call_check_threads_etc();
#else
  FAST_CHECK_THREADS_ON_CALL();
#endif

  if( (p = o->prog) )
  {
    struct svalue *save_sp = Pike_sp - args;
    struct reference *ref = PTR_FROM_INT(p, fun);
    struct inherit *context = INHERIT_FROM_PTR(p, ref);
    struct identifier *function = ID_FROM_PTR(p, ref);
    struct svalue *constant = NULL;
    struct pike_frame *new_frame = NULL;

    int type = (function->identifier_flags & (IDENTIFIER_TYPE_MASK|IDENTIFIER_ALIAS));
    if( o->prog != pike_trampoline_program &&
        !(
#ifdef USE_VALGRIND
          type & IDENTIFIER_PIKE_FUNCTION &&
#endif
          function->func.offset == -1 ))
    {
      switch( type )
      {
        case IDENTIFIER_CONSTANT:
          constant = &context->prog->constants[function->func.const_info.offset].sval;
          if( TYPEOF(*constant) != PIKE_T_PROGRAM )
            break;
	  /* FALLTHRU */
        case IDENTIFIER_C_FUNCTION:
        case IDENTIFIER_PIKE_FUNCTION:
          if( !new_frame )
          {
            new_frame=alloc_pike_frame();
            debug_malloc_touch(new_frame);
          }
#ifdef PROFILING
          new_frame->children_base = Pike_interpreter.accounted_time;
          new_frame->start_time = get_cpu_time() - Pike_interpreter.unlocked_time;

          /* This is mostly for profiling, but
           * could also be used to find out the name of a function
           * in a destructed object. -hubbe
           *
           * Since it not used for anything but profiling yet, I will
           * put it here until someone needs it. -Hubbe
           */
          new_frame->ident = ref->identifier_offset;
          W_PROFILING_DEBUG("%p{: Push at %" PRINT_CPU_TIME
                            " %" PRINT_CPU_TIME "\n",
                            Pike_interpreter.thread_state,
                            new_frame->start_time,
                            new_frame->children_base);
#endif
          new_frame->next = Pike_fp;
          add_ref(new_frame->current_object = o);
          add_ref(new_frame->current_program = p);
          new_frame->context = context;
          new_frame->fun = (unsigned INT16)fun;
          new_frame->locals = save_sp;
          new_frame->args = args;
          new_frame->save_sp = save_sp;

#ifdef PIKE_DEBUG
          if (Pike_in_gc > GC_PASS_PREPARE && Pike_in_gc < GC_PASS_FREE)
            Pike_fatal("Pike code called within gc.\n");
#endif

          Pike_fp = new_frame;
          debug_malloc_touch(Pike_fp);
#ifdef PROFILING
          function->num_calls++;
          function->recur_depth++;
#endif

          if( !constant )
          {
            if (PIKE_FN_START_ENABLED())
            {
              /* DTrace enter probe
                 arg0: function name
                 arg1: object
              */
              struct byte_buffer obj_name = BUFFER_INIT();
              struct svalue obj_sval;
              SET_SVAL(obj_sval, T_OBJECT, 0, object, o);
              safe_describe_svalue(&obj_name, &obj_sval, 0, NULL);
              PIKE_FN_START(function->name->size_shift == 0 ?
                            function->name->str : "[widestring fn name]",
                            buffer_get_string(obj_name));
              buffer_free(&obj_name);
            }
            if(UNLIKELY(Pike_interpreter.trace_level))
            {
              do_trace_function_call(o, function, args);
            }
            new_frame->current_storage = o->storage+context->storage_offset;
            if( type == IDENTIFIER_C_FUNCTION )
            {
              new_frame->num_args = args;
              new_frame->num_locals = args;
              new_frame->pc = 0;
              (*function->func.c_fun)(args);

              /* .. and below follows what is basically a copy of the
               * low_return function...
               */
              if(save_sp+1 > Pike_sp)
              {
                push_int(0);
              } else if(save_sp+1 < Pike_sp) {
                stack_pop_n_elems_keep_top( Pike_sp-save_sp-1 );
              }
              if(UNLIKELY(Pike_interpreter.trace_level>1))
                do_trace_func_return (1, o, fun);
              goto pop;
            }
            new_frame->save_mark_sp=Pike_mark_sp;
            new_frame->pc = new_frame->context->prog->program + function->func.offset
#ifdef ENTRY_PROLOGUE_SIZE
              + ENTRY_PROLOGUE_SIZE
#endif /* ENTRY_PROLOGUE_SIZE */
              ;
            return new_frame->pc;
          }
          else
          {
            struct object *tmp;
            new_frame->pc = 0;
            new_frame->num_args = 0;
            tmp=parent_clone_object(constant->u.program,
                                    o,
                                    fun,
                                    args);
            push_object(tmp);
          pop:
            POP_PIKE_FRAME();
            return 0;
          }
      }
    }
  }
  return low_mega_apply( APPLY_LOW, args, o, (void*)fun );
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
void* low_mega_apply(enum apply_type type, INT32 args, void *arg1, void *arg2)
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

      if (Pike_interpreter.thread_state) {
	if (!th_equal(Pike_interpreter.thread_state->id, self))
	  Pike_fatal("Current thread is wrong.\n");

	DEBUG_CHECK_THREAD();
      }
    }
#endif

  FAST_CHECK_THREADS_ON_CALL();

  switch(type)
  {
  case APPLY_STACK:
    if(!args)
      PIKE_ERROR("`()", "Too few arguments (apply stack).\n", Pike_sp, 0);
    args--;
    arg1=(void *)(Pike_sp-args-1);

    /* FALLTHRU */

  case APPLY_SVALUE:
  case APPLY_SVALUE_STRICT:
  apply_svalue:
  {
    struct svalue *s=(struct svalue *)arg1;
    switch(TYPEOF(*s))
    {
    case T_INT:
      if (!s->u.integer) {
        PIKE_ERROR("0", "Attempt to call the NULL-value.\n", Pike_sp, args);
      } else {
        Pike_error("Attempt to call the value %"PRINTPIKEINT"d.\n",
		   s->u.integer);
      }
      break;

    case T_STRING:
      if (s->u.string->len > 20) {
        /* NB: Cast in order to suppress warning about using precision
         *     with the %p-format.
         */
        ((void (*)(const char *,...))Pike_error)
          ("Attempt to call the string %.20pq...\n", s->u.string);
      } else {
        Pike_error("Attempt to call the string %pq.\n", s->u.string);
      }
      break;

    case T_MAPPING:
      Pike_error("Attempt to call a mapping.\n");
      break;

    default:
      Pike_error("Call to non-function value type:%s.\n",
		 get_name_of_type(TYPEOF(*s)));
      break;

    case T_FUNCTION:
      if(SUBTYPEOF(*s) == FUNCTION_BUILTIN)
      {
#ifdef PIKE_DEBUG
	struct svalue *expected_stack = Pike_sp-args;
#endif
	if(Pike_interpreter.trace_level>1)
	{
          do_trace_efun_call(s, args);
	}
	if (PIKE_FN_START_ENABLED()) {
	  /* DTrace enter probe
	     arg0: function name
	     arg1: object
	   */
	  PIKE_FN_START(s->u.efun->name->size_shift == 0 ?
			s->u.efun->name->str : "[widestring fn name]",
			"");
	}
	(*(s->u.efun->function))(args);
	if (PIKE_FN_DONE_ENABLED()) {
	  /* DTrace leave probe
	     arg0: function name
	  */
	  PIKE_FN_DONE(s->u.efun->name->size_shift == 0 ?
		       s->u.efun->name->str : "[widestring fn name]");
	}

#ifdef PIKE_DEBUG
	s->u.efun->runs++;
	if(Pike_sp != expected_stack + !s->u.efun->may_return_void)
	{
	  if(Pike_sp < expected_stack)
            Pike_fatal("Function popped too many arguments: %s\n",
                       s->u.efun->name->size_shift == 0 ?
                       s->u.efun->name->str : "[widestring fn name]");
	  if(Pike_sp>expected_stack+1)
            Pike_fatal("Function left droppings on stack: %s\n",
                       s->u.efun->name->size_shift == 0 ?
                       s->u.efun->name->str : "[widestring fn name]");
	  if(Pike_sp == expected_stack && !s->u.efun->may_return_void)
            Pike_fatal("Non-void function returned without return value on stack: %s %d\n",
                       s->u.efun->name->size_shift == 0 ?
                       s->u.efun->name->str : "[widestring fn name]",
                       s->u.efun->may_return_void);
	  if(Pike_sp==expected_stack+1 && s->u.efun->may_return_void)
            Pike_fatal("Void function returned with a value on the stack: %s %d\n",
                       s->u.efun->name->size_shift == 0 ?
                       s->u.efun->name->str : "[widestring fn name]",
                       s->u.efun->may_return_void);
	}
#endif

	break;
      }else{
	type=APPLY_SVALUE;
	o=s->u.object;
        fun = SUBTYPEOF(*s);
	if(o->prog == pike_trampoline_program &&
	   fun == QUICK_FIND_LFUN(pike_trampoline_program, LFUN_CALL))
	{
	  fun=((struct pike_trampoline *)(o->storage))->func;
	  scope=((struct pike_trampoline *)(o->storage))->frame;
	  o=scope->current_object;
	}
        goto apply_low;
      }
      break;

    case T_ARRAY:
      if(Pike_interpreter.trace_level)
      {
        do_trace_svalue_call(s, args);
      }
      if (PIKE_FN_START_ENABLED()) {
	/* DTrace enter probe
	   arg0: function name
	   arg1: object
	*/
	PIKE_FN_START("[array]", "");
      }
      apply_array(s->u.array, args, (type == APPLY_STACK));
      break;

    case PIKE_T_TYPE:
      if (args != 1) {
	/* FIXME: Casts to object ought to propagate to apply program below. */
	SIMPLE_WRONG_NUM_ARGS_ERROR("cast", 1);
      }
      o_cast(s->u.type, compile_type_to_runtime_type(s->u.type));
      break;

    case T_PROGRAM:
      if(Pike_interpreter.trace_level)
      {
        do_trace_svalue_call(s, args);
      }
      if (PIKE_FN_START_ENABLED()) {
	/* DTrace enter probe
	   arg0: function name
	   arg1: object
	*/
        struct byte_buffer buf = BUFFER_INIT();
	safe_describe_svalue(&buf, s,0,0);
	PIKE_FN_START("[program]", buffer_get_string(&buf));
        buffer_free(&buf);
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
	goto apply_low;
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
      Pike_error ("Cannot call undefined lfun %pS.\n", lfun_strings[fun]);
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
    }

  apply_low:
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

  if (PIKE_FN_DONE_ENABLED()) {
    /* DTrace leave probe
       arg0: function name
    */
    char *fn = "(unknown)";
    if (o && o->prog) {
      struct identifier *id = ID_FROM_INT(o->prog, fun);
      fn = id->name->size_shift == 0 ? id->name->str : "[widestring fn name]";
    }
    PIKE_FN_DONE(fn);
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
  struct svalue *save_sp = frame_get_save_sp(Pike_fp)+1;
  struct object *o = Pike_fp->current_object;
  int fun = Pike_fp->fun;

  if (PIKE_FN_DONE_ENABLED()) {
    /* DTrace leave probe
       arg0: function name
    */
    char *fn = "(unknown)";
    if (o && o->prog) {
      struct identifier *id = ID_FROM_INT(o->prog, fun);
      fn = id->name->size_shift == 0 ? id->name->str : "[widestring fn name]";
    }
    PIKE_FN_DONE(fn);
  }

#if defined (PIKE_USE_MACHINE_CODE) && defined (OPCODE_RETURN_JUMPADDR)
  /* If the function that returns is the only ref to the current
   * object and its program then the program would be freed in
   * destruct_objects_to_destruct below. However, we're still
   * executing in an opcode in its code so we need prog->program to
   * stick around for a little while more to handle the returned
   * address. We therefore add a ref to the current object so that
   * it'll live through this function. */
  add_ref (o);
#endif

  basic_low_return (save_sp);

  stack_pop_n_elems_keep_top (Pike_sp - save_sp);
  {
      /* consider using a flag for immediate destruct instead... */
      extern struct object *objects_to_destruct;
      if( objects_to_destruct )
          destruct_objects_to_destruct();
  }

#ifdef PIKE_DEBUG
  if(save_sp > Pike_sp)
      Pike_fatal("Pike function did not leave an return value\n");
#endif

  if(UNLIKELY(Pike_interpreter.trace_level>1))
    do_trace_func_return (1, o, fun);

#if defined (PIKE_USE_MACHINE_CODE) && defined (OPCODE_RETURN_JUMPADDR)
  free_object (o);
#endif
}

void low_return_pop(void)
{
  struct svalue *save_sp = frame_get_save_sp(Pike_fp);
#if defined (PIKE_USE_MACHINE_CODE) && defined (OPCODE_RETURN_JUMPADDR)
  /* See note above. */
  struct object *o = Pike_fp->current_object;
  add_ref (o);
#endif

  if (PIKE_FN_DONE_ENABLED()) {
    /* DTrace leave probe
       arg0: function name
    */
    char *fn = "(unknown)";
    struct object *o = Pike_fp->current_object;
    int fun = Pike_fp->fun;
    if (o && o->prog) {
      struct identifier *id = ID_FROM_INT(o->prog, fun);
      fn = id->name->size_shift == 0 ? id->name->str : "[widestring fn name]";
    }
    PIKE_FN_DONE(fn);
  }

  basic_low_return (save_sp);

  pop_n_elems(Pike_sp-save_sp);
  /* consider using a flag for immediate destruct instead... */
  destruct_objects_to_destruct();

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
  frame_set_save_sp(current, frame_get_save_sp(prev));
  current->save_mark_sp=prev->save_mark_sp;
  current->flags = (current->flags & ~PIKE_FRAME_RETURN_MASK) |
    (prev->flags & PIKE_FRAME_RETURN_MASK);

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
      current->context->identifiers_prog->identifiers +
      current->context->identifiers_offset + current->ident;
    if (!function->recur_depth)
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
}

static void restore_catching_eval_jmpbuf (LOW_JMP_BUF *p)
{
  Pike_interpreter.catching_eval_jmpbuf = p;
}

PMOD_EXPORT void mega_apply(enum apply_type type, INT32 args, void *arg1, void *arg2)
{
  /* Save and clear Pike_interpreter.catching_eval_jmpbuf so that the
   * following eval_instruction will install a LOW_JMP_BUF of its
   * own to handle catches. */
  LOW_JMP_BUF *saved_jmpbuf = Pike_interpreter.catching_eval_jmpbuf;
  ONERROR uwp;
  Pike_interpreter.catching_eval_jmpbuf = NULL;
  SET_ONERROR (uwp, restore_catching_eval_jmpbuf, saved_jmpbuf);

  /* The C stack margin is normally 8 kb, but if we get here during a
   * lowered margin then don't fail just because of that, unless it's
   * practically zero. */
  check_c_stack(Pike_interpreter.c_stack_margin ?
		Pike_interpreter.c_stack_margin : 100);
  if( low_mega_apply(type, args, arg1, arg2) )
  {
    eval_instruction(Pike_fp->pc
#ifdef ENTRY_PROLOGUE_SIZE
		     - ENTRY_PROLOGUE_SIZE
#endif /* ENTRY_PROLOGUE_SIZE */
		     );
    low_return();
  }
  CALL_AND_UNSET_ONERROR(uwp);
}

PMOD_EXPORT void mega_apply_low(INT32 args, void *arg1, ptrdiff_t arg2)
{
  /* Save and clear Pike_interpreter.catching_eval_jmpbuf so that the
   * following eval_instruction will install a LOW_JMP_BUF of its
   * own to handle catches. */
  LOW_JMP_BUF *saved_jmpbuf = Pike_interpreter.catching_eval_jmpbuf;
  ONERROR uwp;
  Pike_interpreter.catching_eval_jmpbuf = NULL;
  SET_ONERROR (uwp, restore_catching_eval_jmpbuf, saved_jmpbuf);

  /* The C stack margin is normally 8 kb, but if we get here during a
   * lowered margin then don't fail just because of that, unless it's
   * practically zero. */
  check_c_stack(Pike_interpreter.c_stack_margin ?
		Pike_interpreter.c_stack_margin : 100);
  if( lower_mega_apply( args, arg1, arg2 ) )
  {
    eval_instruction(Pike_fp->pc
#ifdef ENTRY_PROLOGUE_SIZE
		     - ENTRY_PROLOGUE_SIZE
#endif /* ENTRY_PROLOGUE_SIZE */
		     );
    low_return();
  }
  CALL_AND_UNSET_ONERROR(uwp);
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
    int x;
    check_c_stack(8192);
    x = eval_instruction(pc);
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
    ONERROR tmp;
    SET_ONERROR(tmp,exit_on_error,"Error in handle_error in master object!");

    Pike_interpreter.trace_level = 0;
    Pike_interpreter.svalue_stack_margin = LOW_SVALUE_STACK_MARGIN;
    Pike_interpreter.c_stack_margin = LOW_C_STACK_MARGIN;
    move_svalue (Pike_sp++, &throw_value);
    mark_free_svalue (&throw_value);

    if (get_master()) {		/* May return NULL at odd times. */
      APPLY_MASTER("handle_error", 1);
    }
    else {
      struct byte_buffer buf = BUFFER_INIT();
      fprintf (stderr, "There's no master to handle the error. Dumping it raw:\n");
      describe_svalue (&buf, Pike_sp - 1, 0, 0);
      fprintf(stderr,"%s\n",buffer_get_string(&buf));
      buffer_free(&buf);
      if (TYPEOF(Pike_sp[-1]) == PIKE_T_OBJECT && Pike_sp[-1].u.object->prog) {
	int fun = find_identifier("backtrace", Pike_sp[-1].u.object->prog);
	if (fun != -1) {
	  fprintf(stderr, "Attempting to extract the backtrace.\n");
	  safe_apply_low2(Pike_sp[-1].u.object, fun, 0, 0);
	  describe_svalue(&buf, Pike_sp - 1, 0, 0);
	  pop_stack();
	  fprintf(stderr,"%s\n",buffer_get_string(&buf));
	  buffer_free(&buf);
	}
      }
    }

    UNSET_ONERROR(tmp);

    pop_stack();
    Pike_interpreter.svalue_stack_margin = SVALUE_STACK_MARGIN;
    Pike_interpreter.c_stack_margin = C_STACK_MARGIN;
    Pike_interpreter.trace_level = old_t_flag;
  }

  else {
    free_svalue(&throw_value);
    mark_free_svalue (&throw_value);
  }
}

/* NOTE: This function may only be called from the compiler! */
int apply_low_safe_and_stupid(struct object *o, INT32 offset)
{
  JMP_BUF tmp;
  struct pike_frame *new_frame=alloc_pike_frame();
  int ret;
  volatile int use_dummy_reference = 1;
  struct program *prog = o->prog;
  int p_flags = prog->flags;
  LOW_JMP_BUF *saved_jmpbuf;
  int fun = -1;
  int save_compiler_pass = Pike_compiler->compiler_pass;

  /* Search for a function that belongs to the current program,
   * since this is needed for opcodes that use INHERIT_FROM_*
   * (eg F_EXTERN) to work.
   */
  for (fun = prog->num_identifier_references; fun--;) {
    if (!prog->identifier_references[fun].inherit_offset) {
      use_dummy_reference = 0;
      break;
    }
  }

#ifdef PIKE_DEBUG
  if (Pike_compiler->new_program != prog) {
    Pike_fatal("Invalid use of apply_low_save_and_stupid().\n");
  }
#endif /* PIKE_DEBUG */

  /* NB: add_to_*() are only valid in the first pass! */
  Pike_compiler->compiler_pass = COMPILER_PASS_FIRST;
  if (use_dummy_reference) {
    /* No suitable function was found, so add one. */
    struct identifier dummy;
    struct reference dummy_ref = {
      0, 0, ID_HIDDEN,
      PIKE_T_UNKNOWN, { 0, },
    };
    copy_shared_string(dummy.name, empty_pike_string);
    copy_pike_type(dummy.type, function_type_string);
    dummy.filename_strno = -1;
    dummy.linenumber = 0;
    dummy.run_time_type = PIKE_T_FUNCTION;
    dummy.identifier_flags = IDENTIFIER_PIKE_FUNCTION|IDENTIFIER_HAS_BODY;
    dummy.func.offset = offset;
    dummy.opt_flags = 0;
    dummy_ref.identifier_offset = prog->num_identifiers;
    add_to_identifiers(dummy);
    fun = prog->num_identifier_references;
    add_to_identifier_references(dummy_ref);
  }

  /* FIXME: Is this up-to-date with mega_apply? */
  new_frame->next = Pike_fp;
  add_ref(new_frame->current_object = o);
  add_ref(new_frame->current_program = prog);
  new_frame->context = prog->inherits;
  new_frame->locals = new_frame->save_sp = Pike_sp;
  new_frame->args = 0;
  new_frame->num_args=0;
  new_frame->num_locals=0;
  new_frame->fun = fun;
  new_frame->pc = 0;
  new_frame->current_storage=o->storage;

#ifdef PIKE_DEBUG
  if (Pike_fp && (new_frame->locals < Pike_fp->locals)) {
    Pike_fatal("New locals below old locals: %p < %p\n",
               new_frame->locals, Pike_fp->locals);
  }
#endif /* PIKE_DEBUG */

  Pike_fp = new_frame;

  saved_jmpbuf = Pike_interpreter.catching_eval_jmpbuf;
  Pike_interpreter.catching_eval_jmpbuf = NULL;

  if(SETJMP(tmp))
  {
    ret=1;
  }else{
    int tmp;
    new_frame->save_mark_sp=Pike_mark_sp;
    tmp=eval_instruction(prog->program + offset);
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
  Pike_compiler->compiler_pass = save_compiler_pass;

  if (use_dummy_reference) {
    /* Pop the dummy identifier. */
    free_type(function_type_string);
    free_string(empty_pike_string);
    prog->num_identifier_references--;
    prog->num_identifiers--;
  }

  assert (new_frame == Pike_fp);
  LOW_POP_PIKE_FRAME (new_frame);

  return ret;
}

int safe_apply_low2(struct object *o, int fun, int args,
				const char *fun_name)
{
  JMP_BUF recovery;
  int ret = 0;

  free_svalue(& throw_value);
  mark_free_svalue (&throw_value);
  if(SETJMP_SP(recovery, args))
  {
    if(fun_name) call_handle_error();
    push_int(0);
    ret = 0;
  }else{
    if (fun >= 0) {
      apply_low(o,fun,args);
    } else if (fun_name) {
      Pike_error("Cannot call unknown function \"%s\".\n", fun_name);
    } else {
      pop_n_elems(args);
      push_int(0);
    }
    ret = 1;
  }
  UNSETJMP(recovery);
  return ret;
}

PMOD_EXPORT int safe_apply_low(struct object *o, int fun, int args)
{
  return safe_apply_low2(o, fun, args, "Unknown function.");
}

PMOD_EXPORT int safe_apply(struct object *o, const char *fun, INT32 args)
{
  int id;
#ifdef PIKE_DEBUG
  if(!o->prog) Pike_fatal("Apply safe on destructed object.\n");
#endif
  id = find_identifier(fun, o->prog);
  return safe_apply_low2(o, id, args, fun);
}

/* Returns nonzero if the function was called in some handler. */
int low_unsafe_apply_handler(const char *fun,
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

void low_safe_apply_handler(const char *fun,
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
    safe_apply_low2(handler, i, args, fun);
  } else if (compat && compat->prog &&
	     (i = find_identifier(fun, compat->prog)) != -1) {
    safe_apply_low2(compat, i, args, fun);
  } else {
    struct object *master_obj = master();
    if ((i = find_identifier(fun, master_obj->prog)) != -1)
      safe_apply_low2(master_obj, i, args, fun);
    else {
      pop_n_elems(args);
      push_undefined();
    }
  }
}

PMOD_EXPORT void push_text( const char *x )
{
    struct pike_string *s = make_shared_string(x);
    struct svalue *_sp_ = Pike_sp++;
    SET_SVAL_SUBTYPE(*_sp_, 0);
    _sp_->u.string=s;
    debug_malloc_touch(_sp_->u.string);
    SET_SVAL_TYPE(*_sp_, PIKE_T_STRING);
}

PMOD_EXPORT void push_static_text( const char *x )
{
    struct pike_string *s = make_shared_static_string(x, strlen(x), eightbit);
    struct svalue *_sp_ = Pike_sp++;
    SET_SVAL_SUBTYPE(*_sp_, 0);
    _sp_->u.string=s;
    debug_malloc_touch(_sp_->u.string);
    SET_SVAL_TYPE(*_sp_, PIKE_T_STRING);
}

PMOD_EXPORT void push_utf16_text(const p_wchar1 *x)
{
  struct pike_string *s;
  ptrdiff_t i;

  /* i = wstrlen(x); */
  for (i = 0; x[i]; i++)
    ;
  s = begin_wide_shared_string(i, sixteenbit);
  memcpy(s->str, x, i<<1);
  s->flags |= STRING_CONVERT_SURROGATES;
  push_string(end_shared_string(s));
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
  mark_free_svalue (&throw_value);

  if (SETJMP_SP(recovery, args)) {
    ret = 0;
  } else {
    if (low_unsafe_apply_handler (fun, handler, compat, args) &&
	rettypes && !((1 << TYPEOF(Pike_sp[-1])) & rettypes)) {
      if ((rettypes & BIT_ZERO) && SAFE_IS_ZERO (Pike_sp - 1)) {
	pop_stack();
	push_int(0);
      }
      else {
        Pike_error("Invalid return value from %s: %pO\n",
		   fun, Pike_sp-1);
      }
    }

    ret = 1;
  }

  UNSETJMP(recovery);

  STACK_LEVEL_DONE(ret);

  return ret;
}

PMOD_EXPORT void apply_lfun(struct object *o, int lfun, int args)
{
  int fun;
  if(!o->prog)
    PIKE_ERROR("destructed object", "Apply on destructed object.\n",
	       Pike_sp, args);

  if ((fun = (int)FIND_LFUN(o->prog, lfun)) < 0)
    Pike_error("Calling undefined lfun::%pS.\n", lfun_strings[lfun]);

  apply_low(o, fun, args);
}

PMOD_EXPORT void apply_shared(struct object *o,
		  struct pike_string *fun,
		  int args)
{
  int id = find_shared_string_identifier(fun, o->prog);
  if (id >= 0)
    apply_low(o, id, args);
  else
    Pike_error("Cannot call unknown function %pq.\n", fun);
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
  if(IS_UNDEFINED(s))
  {
    /* FIXME: Why? cf LysLysKOM 22891031. */
    pop_n_elems(args);
    push_undefined();
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
  mark_free_svalue (&throw_value);
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
  if (loc.o->prog) {
    loc.inherit = INHERIT_FROM_INT(loc.o->prog, loc.parent_identifier);

    find_external_context(&loc, depth);

    apply_low(loc.o, fun + loc.inherit->identifier_level, args);
  } else {
    PIKE_ERROR("destructed object", "Apply on parent of destructed object.\n",
	       Pike_sp, args);
  }
}

/* mode=1 : Exception on lookup and on execution
 * mode=2 : Exception on lookup but not execution
 * mode=3 : No exceptions
 */
PMOD_EXPORT void apply_master(const char* fun, INT32 args, int mode)
{
  static int id_=-1;
  struct object *master_ob=master();
  if(master_ob->prog->id != 0)
    id_=find_identifier(fun, master_ob->prog);

  if (id_ >= 0) {
    if( mode==1 )
      apply_low(master_ob, id_, args);
    else
      safe_apply_low2(master_ob, id_, args, fun);
  } else {
    if( mode==3 )
    {
      pop_n_elems(args);
      push_undefined();
    }
    else
      Pike_error("Cannot call undefined function \"%s\" in master.\n", fun);
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
               (long)(Pike_sp - Pike_interpreter.evaluator_stack),
               (long)Pike_stack_size);

  if(Pike_mark_sp > &(Pike_interpreter.mark_stack[Pike_stack_size]))
    Pike_fatal("Mark stack overflow.\n");

  if(Pike_mark_sp < Pike_interpreter.mark_stack)
    Pike_fatal("Mark stack underflow.\n");

  for(s=Pike_interpreter.evaluator_stack;s<Pike_sp;s++) {
    /* NOTE: Freed svalues are allowed on the stack. */
    if (TYPEOF(*s) != PIKE_T_FREE) check_svalue(s);
  }

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
  if (id->name->size_shift)
    return "<wide identifier->name->str>";
  /* FIXME: Convert wide string identifiers to narrow strings? */
  return id->name->str;
}

/*: Prints the Pike backtrace for the interpreter context in the given
 *: thread to stderr, without messing in the internals (doesn't even
 *: use struct byte_buffer).
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
  f = ts->state.frame_pointer;
#else
  f = Pike_fp;
#endif

  for (of = 0; f; f = (of = f)->next)
    if (f->refs) {
      int args, i;
      char *file = NULL;
      INT_TYPE line;

      if (f->context) {
	if (f->pc)
	  file = low_get_line_plain (f->pc, f->context->prog, &line, 0);
	else
	  file = low_get_program_line_plain (f->context->prog, &line, 0);
      }
      if (file)
	fprintf (stderr, "%s:%ld: ", file, (long)line);
      else
	fputs ("unknown program: ", stderr);

      if (f->current_program) {
	/* FIXME: Wide string identifiers. */
	fputs (safe_idname_from_int(f->current_program, f->fun), stderr);
	fputc ('(', stderr);
      }
      else
	fputs ("unknown function(", stderr);

      if(!f->locals)
      {
	args=0;
      }else{
        ptrdiff_t tmp;

	if(of) {
          tmp = of->locals - f->locals;
        } else {
#ifdef PIKE_THREADS
          tmp = ts->state.stack_pointer - f->locals;
#else
          tmp = f->num_args; /* FIXME */
#endif
        }
        args = (INT32)tmp;
	args = MAXIMUM(MINIMUM(args, f->num_args),0);
      }

      for (i = 0; i < args; i++) {
	struct svalue *arg = f->locals + i;

	switch (TYPEOF(*arg)) {
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
	    if(SUBTYPEOF(*arg) == FUNCTION_BUILTIN)
	      fputs (arg->u.efun->name->str, stderr);
	    else if(arg->u.object->prog)
	      fputs (safe_idname_from_int(arg->u.object->prog,
					  SUBTYPEOF(*arg)), stderr);
	    else
	      fputc ('0', stderr);
	    break;

	  case T_OBJECT: {
	    struct program *p = arg->u.object->prog;
	    if (p && p->num_linenumbers) {
	      file = low_get_program_line_plain (p, &line, 0);
	      fprintf (stderr, "object(%s:%ld)", file, (long)line);
	    }
	    else
	      fputs ("object", stderr);
	    break;
	  }

	  case T_PROGRAM: {
	    struct program *p = arg->u.program;
	    if (p->num_linenumbers) {
	      file = low_get_program_line_plain (p, &line, 0);
	      fprintf (stderr, "program(%s:%ld)", file, (long)line);
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
	    fprintf (stderr, "<Unknown %d>", TYPEOF(*arg));
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
 *:
 *: It is also used as a fallback from Pike_fatal() if the master
 *: object doesn't exist or fails to format the backtrace.
 */
void gdb_backtraces()
{
#ifdef PIKE_THREADS
  extern INT32 gdb_next_thread_state(INT32, struct thread_state **);
  INT32 i = 0;
  struct thread_state *ts = 0;
  while ((i = gdb_next_thread_state (i, &ts)), ts) {
    fprintf (stderr, "\nTHREAD_ID %p (swapped %s):\n",
	     (void *)(ptrdiff_t)ts->id, ts->swapped ? "out" : "in");
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

PMOD_EXPORT void low_cleanup_interpret(struct Pike_interpreter_struct *interpreter)
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
    free(interpreter->evaluator_stack);
  if(interpreter->mark_stack)
    free(interpreter->mark_stack);

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
  while(Pike_fp)
    POP_PIKE_FRAME();

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
