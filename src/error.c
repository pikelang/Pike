/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
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

RCSID("$Id: error.c,v 1.15 1998/04/06 04:15:52 hubbe Exp $");

#undef ATTRIBUTE
#define ATTRIBUTE(X)

JMP_BUF *recoveries=0;

JMP_BUF *init_recovery(JMP_BUF *r)
{
  r->fp=fp;
  r->sp=sp-evaluator_stack;
  r->mark_sp=mark_sp - mark_stack;
  r->previous=recoveries;
  r->onerror=0;
  r->severity=THROW_ERROR;
  recoveries=r;
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

#ifdef DEBUG
  if(sp - evaluator_stack < recoveries->sp)
    fatal("Stack error in error.\n");
#endif

  while(fp != recoveries->fp)
  {
#ifdef DEBUG
    if(!fp)
      fatal("Popped out of stack frames.\n");
#endif
    free_object(fp->current_object);
    free_program(fp->context.prog);
    
    fp = fp->parent_frame;
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

struct svalue throw_value = { T_INT };
int throw_severity;

void va_error(char *fmt, va_list args) ATTRIBUTE((noreturn))
{
  char buf[2000];
  static char *in_error;
  if(in_error)
  {
    char *tmp=in_error;
    in_error=0;
    fatal("Recursive error() calls, original error: %s",tmp);
  }

  in_error=buf;

  VSPRINTF(buf, fmt, args);

  if(!recoveries)
  {
#ifdef DEBUG
    dump_backlog();
#endif

    fprintf(stderr,"No error recovery context!\n%s",buf);
    exit(99);
  }

  if((long)strlen(buf) >= (long)sizeof(buf))
    fatal("Buffer overflow in error()\n");
  
  push_string(make_shared_string(buf));
  f_backtrace(0);
  f_aggregate(2);
  free_svalue(& throw_value);
  throw_value = *--sp;
  throw_severity=THROW_ERROR;

  in_error=0;
  pike_throw();  /* Hope someone is catching, or we will be out of balls. */
}

void exit_on_error(void *msg)
{
#ifdef DEBUG
  dump_backlog();
#endif
  fprintf(stderr,"%s\n",(char *)msg);
  exit(1);
}

void fatal_on_error(void *msg)
{
#ifdef DEBUG
  dump_backlog();
#endif
  fprintf(stderr,"%s\n",(char *)msg);
  abort();
}

void error(char *fmt,...) ATTRIBUTE((noreturn,format (printf, 1, 2)))
{
  va_list args;
  va_start(args,fmt);
  va_error(fmt,args);
  va_end(args);
}


void debug_fatal(char *fmt, ...) ATTRIBUTE((noreturn,format (printf, 1, 2)))
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
#ifdef DEBUG
  dump_backlog();
#endif

  (void)VFPRINTF(stderr, fmt, args);

  d_flag=t_flag=0;
  push_text("Attempting to dump backlog (may fail).\n");
  f_backtrace(0);
  f_aggregate(2);
  APPLY_MASTER("describe_backtrace",1);
  if(sp[-1].type==T_STRING)
    write_to_stderr(sp[-1].u.string->str, sp[-1].u.string->len);

  fflush(stderr);
  abort();
}
