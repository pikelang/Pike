/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
/**/
#include "global.h"
RCSID("$Id: interpret.c,v 1.146 2003/01/30 13:45:16 grubba Exp $");
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

#ifdef PIKE_DEBUG
static char trace_buffer[200];
#endif


/* sp points to first unused value on stack
 * (much simpler than letting it point at the last used value.)
 */
struct svalue *sp;     /* Current position */
struct svalue *evaluator_stack; /* Start of stack */
int stack_size = EVALUATOR_STACK_SIZE;
int evaluator_stack_malloced = 0;
char *stack_top;

/* mark stack, used to store markers into the normal stack */
struct svalue **mark_sp; /* Current position */
struct svalue **mark_stack; /* Start of stack */
int mark_stack_malloced = 0;

#ifdef PROFILING
#ifdef HAVE_GETHRTIME
long long accounted_time =0;
long long time_base =0;
#endif
#endif

void push_sp_mark(void)
{
  if(mark_sp == mark_stack + stack_size)
    error("No more mark stack!\n");
  *mark_sp++=sp;
}
int pop_sp_mark(void)
{
#ifdef PIKE_DEBUG
  if(mark_sp < mark_stack)
    fatal("Mark stack underflow!\n");
#endif
  return sp - *--mark_sp;
}

struct pike_frame *fp; /* pike_frame pointer */

#ifdef PIKE_DEBUG
static void gc_check_stack_callback(struct callback *foo, void *bar, void *gazonk)
{
  struct pike_frame *f;
  debug_gc_xmark_svalues(evaluator_stack,sp-evaluator_stack-1,"interpreter stack");

  for(f=fp;f;f=f->next)
  {
    if(f->context.parent)
      gc_external_mark(f->context.parent);
    gc_external_mark(f->current_object);
    gc_external_mark(f->context.prog);
  }

}
#endif

void init_interpreter(void)
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
	evaluator_stack=0;
	mark_stack=0;
	goto use_malloc;
      }
    }
    /* Don't keep this fd on exec() */
    set_close_on_exec(fd, 1);
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
  time_base = gethrtime();
  accounted_time =0;
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
      error("Index permission denied.\n");
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

void assign_lvalue(struct svalue *lval,struct svalue *from)
{
#ifdef PIKE_SECURITY
  if(lval->type <= MAX_COMPLEX)
    if(!CHECK_DATA_SECURITY(lval->u.array, SECURITY_BIT_SET_INDEX))
      error("Assign index permission denied.\n");
#endif

  switch(lval->type)
  {
    case T_ARRAY_LVALUE:
    {
      INT32 e;
      if(from->type != T_ARRAY)
	error("Trying to assign combined lvalue from non-array.\n");

      if(from->u.array->size < (lval[1].u.array->size>>1))
	error("Not enough values for multiple assign.\n");

      if(from->u.array->size > (lval[1].u.array->size>>1))
	error("Too many values for multiple assign.\n");

      for(e=0;e<from->u.array->size;e++)
	assign_lvalue(lval[1].u.array->item+(e<<1),from->u.array->item+e);
    }
    break;

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
      error("Assign index permission denied.\n");
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

  fprintf(stderr,"PROGRAM[%d]: inherits=%d identifers_refs=%d\n",
	  p->id,
	  p->num_inherits,
	  p->num_identifier_references);
  for(e=0;e<p->num_identifier_references;e++)
  {
    in=INHERIT_FROM_INT(p,e);
    while(last < in)
    {
      last++;
      fprintf(stderr,
	      "[%ld]%*s parent{ offset=%d ident=%d } "
	      "id{ level=%d } prog=%d\n",
	      (long)(last - p->inherits),
	      last->inherit_level*2,"",
	      last->parent_offset,
	      last->parent_identifier,
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

void find_external_context(struct external_variable_context *loc,
			   int arg2)
{
  struct program *p;
  INT32 e,off;
  TRACE((4,"-find_external_context(%d, inherit=%d)\n",arg2,
	 loc->o->prog ? loc->inherit - loc->o->prog->inherits :0));

  if(!loc->o)
    error("Current object is destructed\n");

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
      error("Parent was lost during cloning.\n");
    
    if(!(p=loc->o->prog))
      error("Attempting to access variable in destructed object\n");
    
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

    TRACE((5,"-   Parent identifier = %d (%s), inherit # = %d\n",
	   loc->parent_identifier,
	   ID_FROM_INT(p, loc->parent_identifier)->name->str,
	   loc->inherit - p->inherits));
    
#ifdef DEBUG_MALLOC
    if (loc->inherit->storage_offset == 0x55555555) {
      fprintf(stderr, "The inherit %p has been zapped!\n", loc->inherit);
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
    describe_svalue(sp-1,0,0);
    s=simple_free_buf();
    if((long)strlen(s) > (long)TRACE_LEN)
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
  union anything *i=get_pointer_if_this_type(sp-2, T_INT);	\
  if(i && !AUTO_BIGNUM_LOOP_TEST(i->integer,INC) &&		\
     sp[-3].type == T_INT)					\
  {								\
    i->integer += INC;						\
    if(i->integer OP2 sp[-3].u.integer)				\
    {								\
      pc+=EXTRACT_INT(pc);					\
      fast_check_threads_etc(8);				\
    }else{							\
      pc+=sizeof(INT32);					\
    }                                                           \
  }else{							\
    lvalue_to_svalue_no_free(sp,sp-2); sp++;			\
    push_int(INC);						\
    f_add(2);							\
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

#ifdef PIKE_DEBUG
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
}

static void do_trace_call(INT32 args)
{
  char *file,*s;
  INT32 linep,e;
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
  if(fp && fp->pc)
  {
    char *f;
    file=get_line(fp->pc,fp->context.prog,&linep);
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


void mega_apply2(enum apply_type type, INT32 args, void *arg1, void *arg2)
{
  struct object *o;
  struct pike_frame *scope=0;
  int fun, tailrecurse=-1;
  struct svalue *save_sp=sp-args;

#ifdef PROFILING
#ifdef HAVE_GETHRTIME
  long long children_base = accounted_time;
  long long start_time = gethrtime() - time_base;
  unsigned INT32 self_time_base;
  if(start_time < 0)
  {
    fatal("gethrtime() shrunk\n start_time=%ld\n gethrtime()=%ld\n time_base=%ld\n",
	  (long)(start_time/100000), 
	  (long)(gethrtime()/100000), 
	  (long)(time_base/100000));
  }
#endif
#endif

#if defined(PIKE_DEBUG) && defined(_REENTRANT)
  if(d_flag)
    {
      THREAD_T self = th_self();

      if( thread_id && !th_equal( OBJ2THREAD(thread_id)->id, self) )
	fatal("Current thread is wrong.\n");
	
      if(thread_for_id(th_self()) != thread_id)
	fatal("thread_for_id() (or thread_id) failed in mega_apply! "
	      "%p != %p\n", thread_for_id(self), thread_id);
    }
#endif

  switch(type)
  {
  case APPLY_STACK:
  apply_stack:
    if(!args)
      PIKE_ERROR("`()", "Too few arguments.\n", sp, 0);
    args--;
    if(sp-save_sp-args > (args<<2) + 32)
    {
      /* The test above assures these two areas
       * are not overlapping
       */
      assign_svalues(save_sp, sp-args-1, args+1, BIT_MIXED);
      pop_n_elems(sp-save_sp-args-1);
    }
    arg1=(void *)(sp-args-1);

  case APPLY_SVALUE:
  apply_svalue:
  {
    struct svalue *s=(struct svalue *)arg1;
    switch(s->type)
    {
    case T_INT:
      if (!s->u.integer) {
	PIKE_ERROR("0", "Attempt to call the NULL-value\n", sp, args);
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
    default:
      error("Call to non-function value type:%s.\n",
	    get_name_of_type(s->type));
      
    case T_FUNCTION:
      if(s->subtype == FUNCTION_BUILTIN)
      {
#ifdef PIKE_DEBUG
	struct svalue *expected_stack = sp-args;
	if(t_flag>1)
	{
	  init_buf();
	  describe_svalue(s,0,0);
	  do_trace_call(args);
	}
#endif
	(*(s->u.efun->function))(args);

#ifdef PIKE_DEBUG
	if(sp != expected_stack + !s->u.efun->may_return_void)
	{
	  if(sp < expected_stack)
	    fatal("Function popped too many arguments: %s\n",
		  s->u.efun->name->str);
	  if(sp>expected_stack+1)
	    fatal("Function left droppings on stack: %s\n",
		  s->u.efun->name->str);
	  if(sp == expected_stack && !s->u.efun->may_return_void)
	    fatal("Non-void function returned without return value on stack: %s %d\n",
		  s->u.efun->name->str,s->u.efun->may_return_void);
	  if(sp==expected_stack+1 && s->u.efun->may_return_void)
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
      PIKE_ERROR("destructed object", "Apply on destructed object.\n", sp, args);
    fun=FIND_LFUN(o->prog,fun);
    goto apply_low;
  

  case APPLY_LOW:
    o=(struct object *)arg1;
    fun=(long)arg2;

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
	pop_n_elems(sp-save_sp);
	push_int(0);
	return;
      }

      check_stack(256);
      check_mark_stack(256);
      check_c_stack(8192);

      new_frame=alloc_pike_frame();
      debug_malloc_touch(new_frame);

#ifdef PIKE_DEBUG
      if(d_flag>2) do_debug();
#endif

      p=o->prog;
      if(!p)
	PIKE_ERROR("destructed object->function",
	      "Cannot call functions in destructed objects.\n", sp, args);
#ifdef PIKE_DEBUG
      if(fun>=(int)p->num_identifier_references)
      {
	fprintf(stderr, "Function index out of range. %d >= %d\n",
		fun, (int)p->num_identifier_references);
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
      new_frame->next = fp;
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

	  fprintf(stderr,"-- context: prog->id=%d inlev=%d idlev=%d pi=%d po=%d so=%d name=%s\n",
		  new_frame->context.prog->id,
		  new_frame->context.inherit_level,
		  new_frame->context.identifier_level,
		  new_frame->context.parent_identifier,
		  new_frame->context.parent_offset,
		  new_frame->context.storage_offset,
		  new_frame->context.name ? new_frame->context.name->str  : "NULL");
	  if(t_flag>19)
	  {
	    describe(new_frame->context.prog);
	  }
	}
#endif


#ifdef PIKE_SECURITY
      CHECK_DATA_SECURITY_OR_ERROR(o, SECURITY_BIT_CALL,
				   ("Function call permission denied.\n"));

      if(!CHECK_DATA_SECURITY(o, SECURITY_BIT_NOT_SETUID))
	SET_CURRENT_CREDS(o->prot);
#endif


#ifdef PROFILING
      function->num_calls++;
#endif
  
      new_frame->locals = sp - args;
      new_frame->expendible = new_frame->locals;
      new_frame->args = args;
      new_frame->fun = fun;
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
	sprintf(buf,"%lx->",(long)o);
	my_strcat(buf);
	my_strcat(function->name->str);
	do_trace_call(args);
      }
      
      fp = new_frame;
      
      if(function->func.offset == -1)
	generic_error(NULL, sp, args,
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
	debug_malloc_touch(fp);
	fp->num_args=args;
	new_frame->num_locals=args;
	check_threads_etc();
	(*function->func.c_fun)(args);
	break;
	
      case IDENTIFIER_CONSTANT:
      {
	struct svalue *s=&(fp->context.prog->
			   constants[function->func.offset].sval);
	debug_malloc_touch(fp);
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
	debug_malloc_touch(fp);
	debug_malloc_touch(o);
	if(sp-save_sp-args<=0)
	{
	  /* Create an extra svalue for tail recursion style call */
	  sp++;
	  MEMMOVE(sp-args,sp-args-1,sizeof(struct svalue)*args);
	  sp[-args-1].type=T_INT;
	}else{
	  free_svalue(sp-args-1);
	  sp[-args-1].type=T_INT;
	}
	low_object_index_no_free(sp-args-1,o,fun);
	tailrecurse=args+1;
	break;
      }

      case IDENTIFIER_PIKE_FUNCTION:
      {
	int num_args;
	int num_locals;
	unsigned char *pc;
	debug_malloc_touch(fp);
	pc=new_frame->context.prog->program + function->func.offset;
	
	num_locals=EXTRACT_UCHAR(pc++);
	num_args=EXTRACT_UCHAR(pc++);

	/* FIXME: this is only needed if this function contains
	 * trampolines
	 */
	new_frame->expendible+=num_locals;
	
	/* adjust arguments on stack */
	if(args < num_args) /* push zeros */
	{
	  clear_svalues_undefined(sp, num_args-args);
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
#ifdef PIKE_DEBUG
	if(num_locals < num_args)
	  fatal("Wrong number of arguments or locals in function def.\n");
#endif
	new_frame->num_locals=num_locals;
	new_frame->num_args=num_args;

	check_threads_etc();

	{
	  struct svalue **save_mark_sp=mark_sp;
	  tailrecurse=eval_instruction(pc);
#ifdef PIKE_DEBUG
	  if(mark_sp < save_mark_sp)
	    fatal("Popped below save_mark_sp!\n");
#endif
	  mark_sp=save_mark_sp;
	}
#ifdef PIKE_DEBUG
	if(sp<evaluator_stack)
	  fatal("Stack error (also simple).\n");
#endif
	break;
      }
      
      }
#ifdef PROFILING
#ifdef HAVE_GETHRTIME
      {
	long long time_passed, time_in_children, self_time;
	time_in_children=  accounted_time - children_base;
	time_passed = gethrtime() - time_base - start_time;
	self_time=time_passed - time_in_children;
	accounted_time+=self_time;
#ifdef PIKE_DEBUG
	if(self_time < 0 || children_base <0 || accounted_time <0)
	  fatal("Time is negative\n  self_time=%ld\n  time_passed=%ld\n  time_in_children=%ld\n  children_base=%ld\n  accounted_time=%ld!\n  time_base=%ld\n  start_time=%ld\n",
		(long)(self_time/100000),
		(long)(time_passed/100000),
		(long)(time_in_children/100000),
		(long)(children_base/100000),
		(long)(accounted_time/100000),
		(long)(time_base/100000),
		(long)(start_time/100000)
		);
#endif
	function->total_time=self_time_base + (INT32)(time_passed /1000);
	function->self_time+=(INT32)( self_time /1000);
      }
#endif
#endif

#if 0
      if(sp - new_frame->locals > 1)
      {
	pop_n_elems(sp - new_frame->locals -1);
      }else if(sp - new_frame->locals < 1){
#ifdef PIKE_DEBUG
	if(sp - new_frame->locals<0) fatal("Frame underflow.\n");
#endif
	sp->u.integer = 0;
	sp->subtype=NUMBER_NUMBER;
	sp->type = T_INT;
	sp++;
      }
#endif

#ifdef PIKE_DEBUG
      if(fp!=new_frame)
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

  if(save_sp+1 < sp)
  {
    assign_svalue(save_sp,sp-1);
    pop_n_elems(sp-save_sp-1);

    destruct_objects_to_destruct();
  }

  if(save_sp+1 > sp)
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
  if(current_creds)
    free_object(current_creds);
  current_creds = creds;
}

void mega_apply(enum apply_type type, INT32 args, void *arg1, void *arg2)
{
  ONERROR tmp;
  if(current_creds)
    add_ref(current_creds);

  SET_ONERROR(tmp, restore_creds, current_creds);
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
  struct svalue *expendible=fp->expendible;
  debug_malloc_touch(fp);
  if(SETJMP(tmp))
  {
    *sp=throw_value;
    throw_value.type=T_INT;
    sp++;
    UNSETJMP(tmp);
    fp->expendible=expendible;
    return 0;
  }else{
    int x;
    fp->expendible=fp->locals + fp->num_locals;
    x=eval_instruction(pc);
    fp->expendible=expendible;
    if(x!=-1) mega_apply(APPLY_STACK, x, 0,0);
    UNSETJMP(tmp);
    return 1;
  }
}

void f_call_function(INT32 args)
{
  mega_apply(APPLY_STACK,args,0,0);
}

int apply_low_safe_and_stupid(struct object *o, INT32 offset)
{
  JMP_BUF tmp;
  struct pike_frame *new_frame=alloc_pike_frame();
  int ret;

  new_frame->next = fp;
  new_frame->current_object = o;
  new_frame->context=o->prog->inherits[0];
  new_frame->locals = evaluator_stack;
  new_frame->expendible=new_frame->locals;
  new_frame->args = 0;
  new_frame->num_args=0;
  new_frame->num_locals=0;
  new_frame->fun = o->prog->num_identifier_references?o->prog->num_identifier_references-1:0;
  new_frame->pc = 0;
  new_frame->current_storage=o->storage;
  new_frame->context.parent=0;
  fp = new_frame;

  add_ref(new_frame->current_object);
  add_ref(new_frame->context.prog);

  if(SETJMP(tmp))
  {
    ret=1;
  }else{
    int tmp=eval_instruction(o->prog->program + offset);
    if(tmp!=-1) mega_apply(APPLY_STACK, tmp, 0,0);
    
#ifdef PIKE_DEBUG
    if(sp<evaluator_stack)
      fatal("Stack error (simple).\n");
#endif
    ret=0;
  }
  UNSETJMP(tmp);

  POP_PIKE_FRAME();

  return ret;
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
      static int inside=0;
      if(!inside)
      {
	ONERROR tmp;
	/* We silently ignore errors if we are already describing one.. */
        inside=1;
	SET_ONERROR(tmp,exit_on_error,"Error in handle_error in master object!");
	assign_svalue_no_free(sp++, & throw_value);
	APPLY_MASTER("handle_error", 1);
	pop_stack();
	UNSET_ONERROR(tmp);
	inside=0;
      }
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
#ifdef PIKE_DEBUG
  if(!o->prog) fatal("Apply safe on destructed object.\n");
#endif
  safe_apply_low(o, find_identifier(fun, o->prog), args);
}

void apply_lfun(struct object *o, int fun, int args)
{
#ifdef PIKE_DEBUG
  if(fun < 0 || fun >= NUM_LFUNS)
    fatal("Apply lfun on illegal value!\n");
#endif
  if(!o->prog)
    PIKE_ERROR("destructed object", "Apply on destructed object.\n", sp, args);

  apply_low(o, (int)FIND_LFUN(o->prog,fun), args);
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
#ifdef PIKE_DEBUG
    if(sp < (expected_stack + evaluator_stack))
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

  if(sp > &(evaluator_stack[stack_size]))
    fatal("Svalue stack overflow. "
	  "(%d entries on stack, stack_size is %d entries)\n",
	  sp-evaluator_stack,stack_size);

  if(mark_sp > &(mark_stack[stack_size]))
    fatal("Mark stack overflow.\n");

  if(mark_sp < mark_stack)
    fatal("Mark stack underflow.\n");

  for(s=evaluator_stack;s<sp;s++) check_svalue(s);

  s=evaluator_stack;
  for(m=mark_stack;m<mark_sp;m++)
  {
    if(*m < s)
      fatal("Mark stack failure.\n");

    s=*m;
  }

  if(s > &(evaluator_stack[stack_size]))
    fatal("Mark stack exceeds svalue stack\n");

  for(f=fp;f;f=f->next)
  {
    if(f->locals)
    {
      if(f->locals < evaluator_stack ||
	f->locals > &(evaluator_stack[stack_size]))
      fatal("Local variable pointer points to Finspång.\n");

      if(f->args < 0 || f->args > stack_size)
	fatal("FEL FEL FEL! HELP!! (corrupted pike_frame)\n");
    }
  }
}
#endif

void cleanup_interpret(void)
{
#ifdef PIKE_DEBUG
  int e;
#endif

  while(fp)
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
