/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
/**/
#include "global.h"
#include "pike_macros.h"
#include "error.h"
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

RCSID("$Id: error.c,v 1.35 1999/05/02 08:11:39 hubbe Exp $");

#undef ATTRIBUTE
#define ATTRIBUTE(X)

JMP_BUF *recoveries=0;

#ifdef PIKE_DEBUG
void check_recovery_context(void)
{
  char foo;
#define TESTILITEST ((((char *)recoveries)-((char *)&foo))*STACK_DIRECTION)
  if(recoveries && TESTILITEST > 0)
    fatal("Recoveries is out biking (recoveries=%p, sp=%p, %d)!\n",recoveries, &foo,TESTILITEST);

  /* Add more stuff here when required */
}
#endif

JMP_BUF *init_recovery(JMP_BUF *r DEBUG_LINE_ARGS)
{
  check_recovery_context();
#ifdef PIKE_DEBUG
  r->line=line;
  r->file=file;
  OED_FPRINTF((stderr, "init_recovery(%p) %s:%d\n", r, file, line));
#endif
  r->fp=fp;
  r->sp=sp-evaluator_stack;
  r->mark_sp=mark_sp - mark_stack;
  r->previous=recoveries;
  r->onerror=0;
  r->severity=THROW_ERROR;
  recoveries=r;
  check_recovery_context();
  return r;
}

void pike_throw(void) ATTRIBUTE((noreturn))
{
  while(recoveries && throw_severity > recoveries->severity)
  {
    while(recoveries->onerror)
    {
      (*recoveries->onerror->func)(recoveries->onerror->arg);
      recoveries->onerror=recoveries->onerror->previous;
    }
    
    recoveries=recoveries->previous;
  }

  if(!recoveries)
    fatal("No error recovery context.\n");

#ifdef PIKE_DEBUG
  if(sp - evaluator_stack < recoveries->sp)
    fatal("Stack error in error.\n");
#endif

  while(fp != recoveries->fp)
  {
#ifdef PIKE_DEBUG
    if(!fp)
      fatal("Popped out of stack frames.\n");
#endif
    POP_PIKE_FRAME();
  }

  pop_n_elems(sp - evaluator_stack - recoveries->sp);
  mark_sp = mark_stack + recoveries->mark_sp;

  while(recoveries->onerror)
  {
    (*recoveries->onerror->func)(recoveries->onerror->arg);
    recoveries->onerror=recoveries->onerror->previous;
  }

  longjmp(recoveries->recovery,1);
}

void push_error(char *description)
{
  push_text(description);
  f_backtrace(0);
  f_aggregate(2);
}

struct svalue throw_value = { T_INT };
int throw_severity;
static const char *in_error;

void low_error(char *buf)
{
  push_error(buf);
  free_svalue(& throw_value);
  throw_value = *--sp;
  throw_severity=THROW_ERROR;
  in_error=0;
  pike_throw();  /* Hope someone is catching, or we will be out of balls. */
}

/* FIXME: NOTE: This function uses a static buffer.
 * Check sizes of arguments passed!
 */
void va_error(const char *fmt, va_list args) ATTRIBUTE((noreturn))
{
  char buf[4096];
  SWAP_IN_THREAD_IF_REQUIRED();
  if(in_error)
  {
    const char *tmp=in_error;
    in_error=0;
    fatal("Recursive error() calls, original error: %s",tmp);
  }

  in_error=buf;

#ifdef HAVE_VSNPRINTF
  vsnprintf(buf, 4090, fmt, args);
#else /* !HAVE_VSNPRINTF */
  VSPRINTF(buf, fmt, args);
#endif /* HAVE_VSNPRINTF */

  if(!recoveries)
  {
#ifdef PIKE_DEBUG
    dump_backlog();
#endif

    fprintf(stderr,"No error recovery context!\n%s",buf);
    exit(99);
  }

  if((long)strlen(buf) >= (long)sizeof(buf))
    fatal("Buffer overflow in error()\n");
  
  low_error(buf);
}

void new_error(const char *name, const char *text, struct svalue *oldsp,
	       INT32 args, const char *file, int line) ATTRIBUTE((noreturn))
{
  int i;

  ASSERT_THREAD_SWAPPED_IN();

  if(in_error)
  {
    const char *tmp=in_error;
    in_error=0;
    fatal("Recursive error() calls, original error: %s",tmp);
  }

  in_error=text;

  if(!recoveries)
  {
#ifdef PIKE_DEBUG
    dump_backlog();
#endif

    fprintf(stderr,"No error recovery context!\n%s():%s",name,text);
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
  throw_value = *--sp;
  throw_severity=THROW_ERROR;

  in_error=0;
  pike_throw();  /* Hope someone is catching, or we will be out of balls. */
}

void exit_on_error(void *msg)
{
#ifdef PIKE_DEBUG
  dump_backlog();
#endif
  fprintf(stderr,"%s\n",(char *)msg);
  exit(1);
}

void fatal_on_error(void *msg)
{
#ifdef PIKE_DEBUG
  dump_backlog();
#endif
  fprintf(stderr,"%s\n",(char *)msg);
  abort();
}

void error(const char *fmt,...) ATTRIBUTE((noreturn,format (printf, 1, 2)))
{
  va_list args;
  va_start(args,fmt);
  va_error(fmt,args);
  va_end(args);
}

void debug_fatal(const char *fmt, ...) ATTRIBUTE((noreturn,format (printf, 1, 2)))
{
  va_list args;
  static int in_fatal = 0;

  va_start(args,fmt);
  /* Prevent double fatal. */
  if (in_fatal)
  {
    (void)VFPRINTF(stderr, fmt, args);
    abort();
  }
  in_fatal = 1;
#ifdef PIKE_DEBUG
  dump_backlog();
#endif

  (void)VFPRINTF(stderr, fmt, args);

  d_flag=t_flag=0;
  push_error("Attempting to dump backlog (may fail).\n");
  APPLY_MASTER("describe_backtrace",1);
  if(sp[-1].type==T_STRING)
    write_to_stderr(sp[-1].u.string->str, sp[-1].u.string->len);

  fflush(stderr);
  abort();
}

#if 1

#define ERR_DECLARE
#include "errors.h"


void f_error_cast(INT32 args)
{
  char *s;
  get_all_args("error->cast",args,"%s",&s);
  if(!strncmp(s,"array",5))
  {
    pop_n_elems(args);
    ref_push_string(GENERIC_ERROR_THIS->desc);
    ref_push_array(GENERIC_ERROR_THIS->backtrace);
    f_aggregate(2);
  }else{
    /* do an error here! */
  }
}

void f_error_index(INT32 args)
{
  int ind;
  get_all_args("error->`[]",args,"%i",&ind);

  pop_n_elems(args);

  switch(ind)
  {
    case 0: ref_push_string(GENERIC_ERROR_THIS->desc); break;
    case 1: ref_push_array(GENERIC_ERROR_THIS->backtrace); break;
    default:
    /* do an index out of range error here! */
      ;
  }
}


void f_error_describe(INT32 args)
{
  pop_n_elems(args);
  ref_push_object(fp->current_object);
  APPLY_MASTER("describe_backtrace",1);
}

void f_error_backtrace(INT32 args)
{
  pop_n_elems(args);
  ref_push_array(GENERIC_ERROR_THIS->backtrace);
}

#define INIT_ERROR(FEL)\
  va_list foo; \
  struct object *o; \
  va_start(foo,desc); \
  ASSERT_THREAD_SWAPPED_IN(); \
  o=low_clone(PIKE_CONCAT(FEL,_error_program));

#define ERROR_DONE(FOO) \
  PIKE_CONCAT(FOO,_error_va(o,func, \
			      base_sp,  args, \
			      desc,foo)); \
  va_end(foo)

#define ERROR_STRUCT(STRUCT,O) \
 ((struct PIKE_CONCAT(STRUCT,_error_struct) *)((O)->storage + PIKE_CONCAT(STRUCT,_error_offset)))

#define ERROR_COPY(STRUCT,X) \
  ERROR_STRUCT(STRUCT,o)->X=X

#define ERROR_COPY_SVALUE(STRUCT,X) \
  assign_svalue_no_free( & ERROR_STRUCT(STRUCT,o)->X, X)


#define ERROR_COPY_REF(STRUCT,X) \
  add_ref( ERROR_STRUCT(STRUCT,o)->X=X )


void generic_error_va(struct object *o,
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
  VSPRINTF(buf, fmt, foo);

  if((long)strlen(buf) >= (long)sizeof(buf))
    fatal("Buffer overflow in error()\n");
#endif /* HAVE_VSNPRINTF */
  in_error=buf;

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

  if(sp[-1].type!=T_ARRAY)
    fatal("Error failed to generate a backtrace!\n");

  ERROR_STRUCT(generic,o)->backtrace=sp[-1].u.array;
  sp--;

  free_svalue(& throw_value);
  throw_value.type=T_OBJECT;
  throw_value.u.object=o;
  in_error=0;
  pike_throw();  /* Hope someone is catching, or we will be out of balls. */
}

void generic_error(
  char *func,
  struct svalue *base_sp,  int args,
  char *desc, ...) ATTRIBUTE((noreturn,format (printf, 4, 5)))
{
  INIT_ERROR(generic);
  ERROR_DONE(generic);
}

void index_error(
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

void bad_arg_error(
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
    ERROR_STRUCT(bad_arg,o)->got.type=T_INT;
    ERROR_STRUCT(bad_arg,o)->got.subtype=NUMBER_UNDEFINED;
    ERROR_STRUCT(bad_arg,o)->got.u.integer=0;
  }
  ERROR_DONE(generic);
}

void math_error(
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
    ERROR_STRUCT(math,o)->number.type=T_INT;
    ERROR_STRUCT(math,o)->number.subtype=NUMBER_UNDEFINED;
    ERROR_STRUCT(math,o)->number.u.integer=0;
  }
  ERROR_DONE(generic);
}

void resource_error(
  char *func,
  struct svalue *base_sp,  int args,
  char *resource_type,
  long howmuch,
  char *desc, ...) ATTRIBUTE((noreturn,format (printf, 6, 7)))
{
  INIT_ERROR(resource);
  ERROR_COPY(resource,howmuch);
  ERROR_STRUCT(resource,o)->resource_type=make_shared_string(resource_type);
  ERROR_DONE(generic);
}

void permission_error(
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

void init_error(void)
{
#define ERR_SETUP
#include "errors.h"
}

void cleanup_error(void)
{
#define ERR_CLEANUP
#include "errors.h"
}
#endif
