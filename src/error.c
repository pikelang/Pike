/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
#include "macros.h"
#include "error.h"
#include "interpret.h"
#include "stralloc.h"
#include "builtin_efuns.h"
#include "array.h"
#include "object.h"

char *automatic_fatal;
JMP_BUF *recoveries=0;
ONERROR *onerror_stack=0;

jmp_buf *init_recovery(JMP_BUF *r)
{
  r->fp=fp;
  r->sp=sp;
  r->mark_sp=mark_sp;
  r->previous=recoveries;
  r->onerror=onerror_stack;
  recoveries=r;
  return & r->recovery;
}

void throw()
{
  if(!recoveries)
    fatal("No error recovery context.\n");

#ifdef DEBUG
  if(sp < recoveries->sp)
    fatal("Error in error.\n");
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

  pop_n_elems(sp - recoveries->sp);
  mark_sp = recoveries->mark_sp;

  while(recoveries->onerror != onerror_stack)
  {
#ifdef DEBUG
    if(!onerror_stack)
      fatal("Popped out of onerror stack!\n");
#endif    
    (*onerror_stack->func)(onerror_stack->arg);
    onerror_stack=onerror_stack->previous;
  }

  longjmp(recoveries->recovery,1);
}

struct svalue throw_value = { T_INT };

void va_error(char *fmt, va_list args)
{
  char buf[2000];
  static int in_error;
  if(in_error)
  {
    in_error=0;
    fatal("Recursive error() calls.\n");
  }

  in_error=1;

  VSPRINTF(buf, fmt, args);

  if(!recoveries || automatic_fatal)
  {
    if(!automatic_fatal)
      automatic_fatal="No error recovery context: ";

    fprintf(stderr,"%s %s",automatic_fatal,buf);
    abort();
  }

  if(strlen(buf) >= sizeof(buf))
    fatal("Buffer overflow in error()\n");
  
  push_string(make_shared_string(buf));
  f_backtrace(0);
  f_aggregate(2);
  free_svalue(& throw_value);
  sp--;
  throw_value = *sp;

  in_error=0;
  throw();  /* Hope someone is catching, or we will be out of balls. */
}

void error(char *fmt,...)
{
  va_list args;
  va_start(args,fmt);
  va_error(fmt,args);
  va_end(args);
}


void fatal(char *fmt, ...)
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
  /* Insert dump routine call here */
  fflush(stderr);
  abort();
}
