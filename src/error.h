/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
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

struct frame;

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
  ONERROR *onerror;
} JMP_BUF;

extern JMP_BUF *recoveries;
extern struct svalue throw_value;

#define SETJMP(X) setjmp((init_recovery(&X)->recovery))
#define UNSETJMP(X) recoveries=X.previous;

#define SET_ONERROR(X,Y,Z) \
  do{ \
     X.func=(error_call)(Y); \
     X.arg=(void *)(Z); \
     X.previous=recoveries->onerror; \
     recoveries->onerror=&X; \
  }while(0)

#define UNSET_ONERROR(X) recoveries->onerror=X.previous

/* Prototypes begin here */
JMP_BUF *init_recovery(JMP_BUF *r);
void throw() ATTRIBUTE((noreturn));
void va_error(char *fmt, va_list args) ATTRIBUTE((noreturn));
void exit_on_error(void *msg);
void fatal_on_error(void *msg);
void error(char *fmt,...) ATTRIBUTE((noreturn,format (printf, 1, 2)));
void fatal(char *fmt, ...) ATTRIBUTE((noreturn,format (printf, 1, 2)));
/* Prototypes end here */

#endif



