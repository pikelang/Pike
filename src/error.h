/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: error.h,v 1.18 1998/04/17 05:08:01 hubbe Exp $
 */
#ifndef ERROR_H
#define ERROR_H

#include "machine.h"

#ifdef HAVE_SETJMP_H
#include <setjmp.h>
#undef HAVE_SETJMP_H
#endif

#include <stdarg.h>

#include "svalue.h"


typedef void (*error_call)(void *);

#ifndef STRUCT_FRAME_DECLARED
#define STRUCT_FRAME_DECLARED
struct frame;
#endif

#define THROW_ERROR 10
#define THROW_THREAD_EXIT 20
#define THROW_THREAD_KILLED 30
#define THROW_EXIT 40
#define THROW_MAX_SEVERITY 100

typedef struct ONERROR
{
  struct ONERROR *previous;
  error_call func;
  void *arg;
} ONERROR;

typedef struct JMP_BUF
{
  struct JMP_BUF *previous;
  jmp_buf recovery;
  struct frame *fp;
  INT32 sp;
  INT32 mark_sp;
  INT32 severity;
  ONERROR *onerror;
} JMP_BUF;

extern JMP_BUF *recoveries;
extern struct svalue throw_value;
extern int throw_severity;

#define SETJMP(X) setjmp((init_recovery(&X)->recovery))
#define UNSETJMP(X) recoveries=X.previous;

#define SET_ONERROR(X,Y,Z) \
  do{ \
     if(!recoveries) break; \
     X.func=(error_call)(Y); \
     X.arg=(void *)(Z); \
     X.previous=recoveries->onerror; \
     recoveries->onerror=&X; \
  }while(0)

#ifdef DEBUG
#define UNSET_ONERROR(X) do {\
  if(!recoveries) break; \
  if(recoveries->onerror != &(X)) fatal("UNSET_ONERROR out of sync.\n"); \
  recoveries->onerror=(X).previous; \
  } while(0)
#else
#define UNSET_ONERROR(X) recoveries && (recoveries->onerror=X.previous)
#endif

#if defined(DEBUG) && 0
/* Works, but probably not interresting for most people
 *	/grubba 1998-04-11
 */
#define PIKE_ERROR(NAME, TEXT, SP, ARGS)	new_error(NAME, TEXT, SP, ARGS, __FILE__, __LINE__);
#else
#define PIKE_ERROR(NAME, TEXT, SP, ARGS)	new_error(NAME, TEXT, SP, ARGS, NULL, 0);
#endif /* DEBUG */

/* Prototypes begin here */
JMP_BUF *init_recovery(JMP_BUF *r);
void pike_throw(void) ATTRIBUTE((noreturn));
void va_error(const char *fmt, va_list args) ATTRIBUTE((noreturn));
void exit_on_error(void *msg);
void fatal_on_error(void *msg);
void error(const char *fmt,...) ATTRIBUTE((noreturn,format (printf, 1, 2)));
void new_error(const char *name, const char *text, struct svalue *oldsp,
	       INT32 args, const char *file, int line) ATTRIBUTE((noreturn));
void debug_fatal(const char *fmt, ...) ATTRIBUTE((noreturn,format (printf, 1, 2)));
/* Prototypes end here */

#define fatal \
 fprintf(stderr,"Fatal error at %s:%d\n",__FILE__,__LINE__),debug_fatal

#endif



