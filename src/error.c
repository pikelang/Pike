/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
/**/
#define NO_PIKE_SHORTHAND
#include "global.h"
#include "pike_macros.h"
#include "pike_error.h"
#include "interpret.h"
#include "stralloc.h"
#include "builtin_functions.h"
#include "array.h"
#include "object.h"
#include "main.h"
#include "builtin_functions.h"
#include "backend.h"
#include "operators.h"
#include "module_support.h"
#include "threads.h"
#include "gc.h"

RCSID("$Id: error.c,v 1.66 2000/12/04 19:38:26 mast Exp $");

#undef ATTRIBUTE
#define ATTRIBUTE(X)

/*
 * Backtrace handling.
 */

struct pike_backtrace
{
  struct pike_frame *trace;
  struct pike_frame *iterator;
  int iterator_index;
};

static struct program *frame_program = NULL;

#define THIS_BT ((struct pike_backtrace *)Pike_fp->current_storage)

static void init_backtrace(struct object *o)
{
  MEMSET(THIS_BT, 0, sizeof(struct pike_backtrace));
}

static void exit_backtrace(struct object *o)
{
  if (THIS_BT->trace) {
    free_pike_frame(THIS_BT->trace);
    THIS_BT->trace = NULL;
    THIS_BT->iterator = NULL;
  }
}

/* void create() */
void f_bt_create(INT32 args)
{
  if (THIS_BT->trace) {
    free_pike_frame(THIS_BT->trace);
  }
  add_ref(THIS_BT->trace = Pike_fp);
  THIS_BT->iterator = THIS_BT->trace;
  THIS_BT->iterator_index = 0;
  pop_n_elems(args);
  push_int(0);
}

/* int _sizeof() */
static void f_bt__sizeof(INT32 args)
{
  int size = 0;
  struct pike_frame *f = THIS_BT->iterator;
  size = THIS_BT->iterator_index;
  while (f) {
    size++;
    f = f->next;
  }
  pop_n_elems(args);
  push_int(size);
}

/* array(int) _indices() */
static void f_bt__indices(INT32 args)
{
  /* Not really optimal, but who performs indices() on a backtrace? */
  f_bt__sizeof(args);
  f_allocate(1);
  f_indices(1);
}

/* array _values() */
static void f_bt__values(INT32 args)
{
  struct pike_frame *f = THIS_BT->trace;
  struct array *a;
  int i;
  int sz;
  f_bt__sizeof(args);
  f_allocate(1);
  for (i=0, a=Pike_sp[-1].u.array, sz=a->size; (i < sz) && f; i++, f=f->next) {
    struct object *o = low_clone(frame_program);
    call_c_initializers(o);
    add_ref(*((struct pike_frame **)(o->storage)) = f);
    push_object(o);
    a->item[i] = Pike_sp[-1];
    Pike_sp--;
  }
}

/* object(frame) `[](int index) */
static void f_bt__index(INT32 args)
{
  INT_TYPE index;
  int i;
  struct pike_frame *f;
  struct object *o;

  get_all_args("backtrace->`[]", args, "%i", &index);

  if (index < 0) {
    /* Indexing from end not supported. */
    index_error("backtrace->`[]", Pike_sp-args, args, NULL, Pike_sp-args,
		"Indexing with negative value (%d).\n", index);
  }

  if (index < THIS_BT->iterator_index) {
    THIS_BT->iterator = THIS_BT->trace;
    THIS_BT->iterator_index = 0;
  }

  f = THIS_BT->iterator;
  i = THIS_BT->iterator_index;

  while (f && (i < index)) {
    f = f->next;
    i++;
  }
  if (!f) {
    index_error("backtrace->`[]", Pike_sp-args, args, NULL, Pike_sp-args,
		"Index out of range [0..%d].\n", i-1);
  }
  THIS_BT->iterator = f;
  THIS_BT->iterator_index = i;
  o = low_clone(frame_program);
  call_c_initializers(o);
  add_ref(*((struct pike_frame **)(o->storage)) = f);
  pop_n_elems(args);
  push_object(o);
}

/*
 * Frame handling.
 */

#define THIS_PF		((struct pike_frame **)Pike_fp->current_storage)

static void init_pike_frame(struct object *o)
{
  THIS_PF[0] = NULL;
}

static void exit_pike_frame(struct object *o)
{
  if (THIS_PF[0]) {
    free_pike_frame(THIS_PF[0]);
    THIS_PF[0] = NULL;
  }
}

/* mixed `[](int index) */
static void f_pf__index(INT32 args)
{
  INT_TYPE index;
  struct pike_frame *f;

  get_all_args("pike_frame->`[]", args, "%i", &index);

  f = THIS_PF[0];

  if (!f) {
    index_error("pike_frame->`[]", Pike_sp-args, args, NULL, Pike_sp-args,
		"Indexing the empty array with %d.\n", index);
  }
  if (index < 0) {
    index_error("pike_frame->`[]", Pike_sp-args, args, NULL, Pike_sp-args,
		"Indexing with negative index (%d)\n", index);
  }
  if (!(f->current_object && f->context.prog)) {
    index_error("pike_frame->`[]", Pike_sp-args, args, NULL, Pike_sp-args,
		"Indexing the NULL value with %d.\n", index);
  }
  switch(index) {
  case 0:	/* Filename */
  case 1:	/* Linenumber */
    break;
  case 2:	/* Function */
    if (f->current_object->prog) {
      pop_n_elems(args);
      ref_push_object(f->current_object);
      Pike_sp[-1].subtype = f->fun;
      Pike_sp[-1].type = PIKE_T_FUNCTION;
    } else {
      pop_n_elems(args);
      push_int(0);
      Pike_sp[-1].subtype = NUMBER_DESTRUCTED;
    }
    return;
  default:	/* Arguments */
    break;
  }
}

/*
 * Recoveries handling.
 */

JMP_BUF *recoveries=0;

#ifdef PIKE_DEBUG
PMOD_EXPORT void check_recovery_context(void)
{
  char foo;
#define TESTILITEST ((((char *)Pike_interpreter.recoveries)-((char *)&foo))*STACK_DIRECTION)
  if(Pike_interpreter.recoveries && TESTILITEST > 0) {
    fprintf(stderr, "Recoveries is out biking (Pike_interpreter.recoveries=%p, Pike_sp=%p, %ld)!\n",
	    Pike_interpreter.recoveries, &foo,
	    DO_NOT_WARN((long)TESTILITEST));
    fprintf(stderr, "Last recovery was added at %s:%d\n",
	    Pike_interpreter.recoveries->file,
	    Pike_interpreter.recoveries->line);
    fatal("Recoveries is out biking (Pike_interpreter.recoveries=%p, Pike_sp=%p, %ld)!\n",
	  Pike_interpreter.recoveries, &foo,
	  DO_NOT_WARN((long)TESTILITEST));
  }

  /* Add more stuff here when required */
}

PMOD_EXPORT void pike_gdb_breakpoint(void) 
{
}
#endif

PMOD_EXPORT JMP_BUF *init_recovery(JMP_BUF *r DEBUG_LINE_ARGS)
{
  check_recovery_context();
#ifdef PIKE_DEBUG
  r->line=line;
  r->file=file;
  OED_FPRINTF((stderr, "init_recovery(%p) %s:%d\n", r, file, line));
#endif
  r->frame_pointer=Pike_fp;
  r->stack_pointer=Pike_sp-Pike_interpreter.evaluator_stack;
  r->mark_sp=Pike_mark_sp - Pike_interpreter.mark_stack;
  r->previous=Pike_interpreter.recoveries;
  r->onerror=0;
  r->severity=THROW_ERROR;
  Pike_interpreter.recoveries=r;
  check_recovery_context();
  return r;
}

PMOD_EXPORT DECLSPEC(noreturn) void pike_throw(void) ATTRIBUTE((noreturn))
{
  while(Pike_interpreter.recoveries && throw_severity > Pike_interpreter.recoveries->severity)
  {
    while(Pike_interpreter.recoveries->onerror)
    {
      (*Pike_interpreter.recoveries->onerror->func)(Pike_interpreter.recoveries->onerror->arg);
      Pike_interpreter.recoveries->onerror=Pike_interpreter.recoveries->onerror->previous;
    }
    
    Pike_interpreter.recoveries=Pike_interpreter.recoveries->previous;
  }

  if(!Pike_interpreter.recoveries)
    fatal("No Pike_error recovery context.\n");

#ifdef PIKE_DEBUG
  if(Pike_sp - Pike_interpreter.evaluator_stack < Pike_interpreter.recoveries->stack_pointer)
    fatal("Stack Pike_error in Pike_error.\n");
#endif

  while(Pike_fp != Pike_interpreter.recoveries->frame_pointer)
  {
#ifdef PIKE_DEBUG
    if(!Pike_fp)
      fatal("Popped out of stack frames.\n");
#endif
    POP_PIKE_FRAME();
  }

  pop_n_elems(Pike_sp - Pike_interpreter.evaluator_stack - Pike_interpreter.recoveries->stack_pointer);
  Pike_mark_sp = Pike_interpreter.mark_stack + Pike_interpreter.recoveries->mark_sp;

  while(Pike_interpreter.recoveries->onerror)
  {
    (*Pike_interpreter.recoveries->onerror->func)(Pike_interpreter.recoveries->onerror->arg);
    Pike_interpreter.recoveries->onerror=Pike_interpreter.recoveries->onerror->previous;
  }

  longjmp(Pike_interpreter.recoveries->recovery,1);
}

PMOD_EXPORT void push_error(char *description)
{
  push_text(description);
  f_backtrace(0);
  f_aggregate(2);
}

PMOD_EXPORT struct svalue throw_value = { PIKE_T_INT };
int throw_severity;
static const char *in_error;

PMOD_EXPORT DECLSPEC(noreturn) void low_error(const char *buf) ATTRIBUTE((noreturn))
{
  push_error(buf);
  free_svalue(& throw_value);
  throw_value = *--Pike_sp;
  throw_severity = THROW_ERROR;
  in_error=0;
  pike_throw();  /* Hope someone is catching, or we will be out of balls. */
}

/* FIXME: NOTE: This function uses a static buffer.
 * Check sizes of arguments passed!
 */
void DECLSPEC(noreturn) va_error(const char *fmt, va_list args) ATTRIBUTE((noreturn))
{
  char buf[4096];
  SWAP_IN_THREAD_IF_REQUIRED();
  if(in_error)
  {
    const char *tmp=in_error;
    in_error=0;
    fatal("Recursive Pike_error() calls, original Pike_error: %s",tmp);
  }

  in_error=buf;

#ifdef HAVE_VSNPRINTF
  vsnprintf(buf, 4090, fmt, args);
#else /* !HAVE_VSNPRINTF */
  VSPRINTF(buf, fmt, args);
#endif /* HAVE_VSNPRINTF */

  if(!Pike_interpreter.recoveries)
  {
#ifdef PIKE_DEBUG
    dump_backlog();
#endif

    fprintf(stderr,"No Pike_error recovery context!\n%s",buf);
    exit(99);
  }

  if((size_t)strlen(buf) >= (size_t)sizeof(buf))
    fatal("Buffer overflow in Pike_error()\n");
  
  low_error(buf);
}

PMOD_EXPORT DECLSPEC(noreturn) void new_error(const char *name, const char *text, struct svalue *oldsp,
	       INT32 args, const char *file, int line) ATTRIBUTE((noreturn))
{
  int i;

  ASSERT_THREAD_SWAPPED_IN();

  if(in_error)
  {
    const char *tmp=in_error;
    in_error=0;
    fatal("Recursive Pike_error() calls, original Pike_error: %s",tmp);
  }

  in_error=text;

  if(!Pike_interpreter.recoveries)
  {
#ifdef PIKE_DEBUG
    dump_backlog();
#endif

    fprintf(stderr,"No Pike_error recovery context!\n%s():%s",name,text);
    if(file)
      fprintf(stderr,"at %s:%d\n",file,line);
    exit(99);
  }

  push_text(text);

  f_backtrace(0);

  if (file) {
    push_text(file);
    push_int(line);
  } else {
    push_int(0);
    push_int(0);
  }
  push_text(name);

  for (i=-args; i; i++) {
    push_svalue(oldsp + i);
  }

  f_aggregate(args + 3);
  f_aggregate(1);

  f_add(2);

  f_aggregate(2);

  free_svalue(& throw_value);
  throw_value = *--Pike_sp;
  throw_severity=THROW_ERROR;

  in_error=0;
  pike_throw();  /* Hope someone is catching, or we will be out of balls. */
}

PMOD_EXPORT void exit_on_error(void *msg)
{
  ONERROR tmp;
  SET_ONERROR(tmp,fatal_on_error,"Fatal in exit_on_error!");
  d_flag=0;

  fprintf(stderr,"%s\n",(char *)msg);
#ifdef PIKE_DEBUG
  dump_backlog();
#endif
  fprintf(stderr,"%s\n",(char *)msg);
#ifdef PIKE_DEBUG
  {
    char *s;
    fprintf(stderr,"Attempting to dump raw Pike_error: (may fail)\n");
    init_buf();
    describe_svalue(&throw_value,0,0);
    s=simple_free_buf();
    fprintf(stderr,"%s\n",s);
    free(s);
  }
#endif
  exit(1);
}

#ifdef __NT__
/* Wrapper around abort() to avoid interactive requesters on NT. */
static void do_abort()
{
  if (!d_flag && !getenv("PIKE_DEBUG")) {
    exit(-6);	/* -SIGIOT */
  }
  abort();
}
#else /* !__NT__ */
#define do_abort()	abort()
#endif /* __NT__ */

PMOD_EXPORT void fatal_on_error(void *msg)
{
#ifdef PIKE_DEBUG
  dump_backlog();
#endif
  fprintf(stderr,"%s\n",(char *)msg);
  do_abort();
}

PMOD_EXPORT DECLSPEC(noreturn) void Pike_error(const char *fmt,...) ATTRIBUTE((noreturn,format (printf, 1, 2)))
{
  va_list args;
  va_start(args,fmt);
  va_error(fmt,args);
  va_end(args);
}

PMOD_EXPORT DECLSPEC(noreturn) void debug_fatal(const char *fmt, ...) ATTRIBUTE((noreturn,format (printf, 1, 2)))
{
  va_list args;
  static int in_fatal = 0;

  va_start(args,fmt);
  /* Prevent double fatal. */
  if (in_fatal)
  {
    (void)VFPRINTF(stderr, fmt, args);
    do_abort();
  }

  in_fatal = 1;
#ifdef PIKE_DEBUG
  dump_backlog();
#endif

  {
    extern int Pike_in_gc;
    if(Pike_in_gc)
    {
      fprintf(stderr,"Pike was in GC stage %d when this fatal occured:\n",Pike_in_gc);
      Pike_in_gc=0;
    }
  }

  (void)VFPRINTF(stderr, fmt, args);

  d_flag=t_flag=0;
  if(Pike_sp && Pike_interpreter.evaluator_stack)
  {
    fprintf(stderr,"Attempting to dump backlog (may fail)...\n");
    push_error("Backtrace at time of fatal:\n");
    APPLY_MASTER("describe_backtrace",1);
    if(Pike_sp[-1].type==PIKE_T_STRING)
      write_to_stderr(Pike_sp[-1].u.string->str, Pike_sp[-1].u.string->len);
  }else{
    fprintf(stderr,"No stack - no backtrace.\n");
  }
  fflush(stderr);
  do_abort();
}

#if 1

#define ERR_DECLARE
#include "errors.h"


void f_error_cast(INT32 args)
{
  char *s;
  get_all_args("Pike_error->cast",args,"%s",&s);
  if(!strncmp(s,"array",5))
  {
    pop_n_elems(args);
    ref_push_string(GENERIC_ERROR_THIS->desc);
    ref_push_array(GENERIC_ERROR_THIS->backtrace);
    f_aggregate(2);
  }else{
    SIMPLE_BAD_ARG_ERROR("Pike_error->cast", 1, "the value \"array\"");
  }
}

void f_error_index(INT32 args)
{
  INT_TYPE ind;
  get_all_args("Pike_error->`[]",args,"%i",&ind);

  switch(ind)
  {
    case 0:
      pop_n_elems(args);
      ref_push_string(GENERIC_ERROR_THIS->desc);
      break;
    case 1:
      pop_n_elems(args);
      ref_push_array(GENERIC_ERROR_THIS->backtrace);
      break;
    default:
      index_error("Pike_error->`[]", Pike_sp-args, args, NULL, Pike_sp-args,
		  "Index %d is out of range 0 - 1.\n", ind);
      break;
  }
}


void f_error_describe(INT32 args)
{
  pop_n_elems(args);
  ref_push_object(Pike_fp->current_object);
  APPLY_MASTER("describe_backtrace",1);
}

void f_error_backtrace(INT32 args)
{
  pop_n_elems(args);
  ref_push_array(GENERIC_ERROR_THIS->backtrace);
}

#ifdef ERROR_DEBUG
#define DWERROR(X)	fprintf X
#else /* !ERROR_DEBUG */
#define DWERROR(X)
#endif /* ERROR_DEBUG */

#define INIT_ERROR(FEL)\
  va_list foo; \
  struct object *o; \
  va_start(foo,desc); \
  ASSERT_THREAD_SWAPPED_IN(); \
  o=low_clone(PIKE_CONCAT(FEL,_error_program)); \
  DWERROR((stderr, "%s(): Throwing a " #FEL " Pike_error\n", func))

#define ERROR_DONE(FOO) \
  PIKE_CONCAT(FOO,_error_va(o,func, \
			      base_sp,  args, \
			      desc,foo)); \
  va_end(foo)

#define ERROR_STRUCT(STRUCT,O) \
 ((struct PIKE_CONCAT(STRUCT,_error_struct) *)((O)->storage + PIKE_CONCAT(STRUCT,_error_offset)))

#define ERROR_COPY(STRUCT,X) \
  ERROR_STRUCT(STRUCT,o)->X=X

#define ERROR_COPY_SVALUE(STRUCT,X) do { \
    if (X) { \
      assign_svalue_no_free( & ERROR_STRUCT(STRUCT,o)->X, X); \
    } else { \
      ERROR_STRUCT(STRUCT, o)->X.type = PIKE_T_INT; \
      ERROR_STRUCT(STRUCT, o)->X.subtype = 0; \
      ERROR_STRUCT(STRUCT, o)->X.u.integer = 0; \
    } \
  } while (0)


#define ERROR_COPY_REF(STRUCT,X) \
  add_ref( ERROR_STRUCT(STRUCT,o)->X=X )


DECLSPEC(noreturn) void generic_error_va(struct object *o,
					 char *func,
					 struct svalue *base_sp,  int args,
					 char *fmt,
					 va_list foo)
     ATTRIBUTE((noreturn))
{
  char buf[8192];
  struct pike_string *desc;
  struct array *backtrace;
  int i;

#ifdef HAVE_VSNPRINTF
  vsnprintf(buf, sizeof(buf)-1, fmt, foo);
#else /* !HAVE_VSNPRINTF */
  /* Sentinel that will be overwritten on buffer overflow. */
  buf[sizeof(buf)-1] = '\0';

  VSPRINTF(buf, fmt, foo);

  if(buf[sizeof(buf)-1])
    fatal("Buffer overflow in Pike_error()\n");
#endif /* HAVE_VSNPRINTF */
  in_error=buf;

  if (!master_program) {
    fprintf(stderr, "ERROR: %s\n", buf);
  }

  ERROR_STRUCT(generic,o)->desc=make_shared_string(buf);
  f_backtrace(0);

  if(func)
  {
    push_int(0);
    push_int(0);
    push_text(func);

    for (i=0;i<args;i++)
      push_svalue(base_sp + i);
    f_aggregate(args + 3);
    f_aggregate(1);
    f_add(2);
  }

  if(Pike_sp[-1].type!=PIKE_T_ARRAY)
    fatal("Error failed to generate a backtrace!\n");

  ERROR_STRUCT(generic,o)->backtrace=Pike_sp[-1].u.array;
  Pike_sp--;
  dmalloc_touch_svalue(Pike_sp);

  free_svalue(& throw_value);
  throw_value.type=PIKE_T_OBJECT;
  throw_value.u.object=o;
  throw_severity = THROW_ERROR;
  in_error=0;
  pike_throw();  /* Hope someone is catching, or we will be out of balls. */
}

PMOD_EXPORT DECLSPEC(noreturn) void generic_error(
  char *func,
  struct svalue *base_sp,  int args,
  char *desc, ...) ATTRIBUTE((noreturn,format (printf, 4, 5)))
{
  INIT_ERROR(generic);
  ERROR_DONE(generic);
}

PMOD_EXPORT DECLSPEC(noreturn) void index_error(
  char *func,
  struct svalue *base_sp,  int args,
  struct svalue *val,
  struct svalue *ind,
  char *desc, ...) ATTRIBUTE((noreturn,format (printf, 6, 7)))
{
  INIT_ERROR(index);
  ERROR_COPY_SVALUE(index, val);
  ERROR_COPY_SVALUE(index, ind);
  ERROR_DONE(generic);
}

PMOD_EXPORT DECLSPEC(noreturn) void bad_arg_error(
  char *func,
  struct svalue *base_sp,  int args,
  int which_arg,
  char *expected_type,
  struct svalue *got,
  char *desc, ...)  ATTRIBUTE((noreturn,format (printf, 7, 8)))
{
  INIT_ERROR(bad_arg);
  ERROR_COPY(bad_arg, which_arg);
  ERROR_STRUCT(bad_arg,o)->expected_type=make_shared_string(expected_type);
  if(got)
  {
    ERROR_COPY_SVALUE(bad_arg, got);
  }else{
    ERROR_STRUCT(bad_arg,o)->got.type=PIKE_T_INT;
    ERROR_STRUCT(bad_arg,o)->got.subtype=NUMBER_UNDEFINED;
    ERROR_STRUCT(bad_arg,o)->got.u.integer=0;
  }
  DWERROR((stderr, "%s():Bad arg %d (expected %s)\n",
	   func, which_arg, expected_type));
  ERROR_DONE(generic);
}

PMOD_EXPORT DECLSPEC(noreturn) void math_error(
  char *func,
  struct svalue *base_sp,  int args,
  struct svalue *number,
  char *desc, ...) ATTRIBUTE((noreturn,format (printf, 5, 6)))
{
  INIT_ERROR(math);
  if(number)
  {
    ERROR_COPY_SVALUE(math, number);
  }else{
    ERROR_STRUCT(math,o)->number.type=PIKE_T_INT;
    ERROR_STRUCT(math,o)->number.subtype=NUMBER_UNDEFINED;
    ERROR_STRUCT(math,o)->number.u.integer=0;
  }
  ERROR_DONE(generic);
}

PMOD_EXPORT DECLSPEC(noreturn) void resource_error(
  char *func,
  struct svalue *base_sp,  int args,
  char *resource_type,
  size_t howmuch_,
  char *desc, ...) ATTRIBUTE((noreturn,format (printf, 6, 7)))
{
  INT_TYPE howmuch = DO_NOT_WARN((INT_TYPE)howmuch_);
  INIT_ERROR(resource);
  ERROR_COPY(resource, howmuch);
  ERROR_STRUCT(resource,o)->resource_type=make_shared_string(resource_type);
  ERROR_DONE(generic);
}

PMOD_EXPORT DECLSPEC(noreturn) void permission_error(
  char *func,
  struct svalue *base_sp, int args,
  char *permission_type,
  char *desc, ...) ATTRIBUTE((noreturn, format(printf, 5, 6)))
{
  INIT_ERROR(permission);
  ERROR_STRUCT(permission,o)->permission_type=
    make_shared_string(permission_type);
  ERROR_DONE(generic);
}

PMOD_EXPORT void wrong_number_of_args_error(char *name, int args, int expected)
{
  char *msg;
  if(expected>args)
  {
    msg="Too few arguments";
  }else{
    msg="Too many arguments";
  }

  new_error(name, msg, Pike_sp-args, args, 0,0);
}

#ifdef PIKE_DEBUG
static void gc_check_throw_value(struct callback *foo, void *bar, void *gazonk)
{
  debug_gc_xmark_svalues(&throw_value,1," in the throw value");
}
#endif

void init_error(void)
{
#define ERR_SETUP
#include "errors.h"

#ifdef PIKE_DEBUG
  dmalloc_accept_leak(add_gc_callback(gc_check_throw_value,0,0));
#endif
}

void cleanup_error(void)
{
#define ERR_CLEANUP
#include "errors.h"
}
#endif
