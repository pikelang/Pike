/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef ERROR_H
#define ERROR_H

#include <setjmp.h>
#include <stdarg.h>
#include "machine.h"
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
  struct svalue *sp;
  struct svalue **mark_sp;
  ONERROR *onerror;
} JMP_BUF;

extern ONERROR *onerror_stack;
extern JMP_BUF *recoveries;
extern struct svalue throw_value;
extern char *automatic_fatal;

#define SETJMP(X) setjmp((init_recovery(&X)[0]))
#define UNSETJMP(X) recoveries=X.previous;

#define SET_ONERROR(X,Y,Z) \
  do{ \
     X.func=(error_call)(Y); \
     X.arg=(void *)(Z); \
     X.previous=onerror_stack; \
     onerror_stack=&X; \
  }while(0)

#define UNSET_ONERROR(X) onerror_stack=X.previous

jmp_buf *init_recovery(JMP_BUF *r);
int fix_recovery(int i, JMP_BUF *r);
void throw() ATTRIBUTE((noreturn));
void va_error(char *fmt, va_list args) ATTRIBUTE((noreturn));
void error(char *fmt,...) ATTRIBUTE((noreturn,format (printf, 1, 2)));
void fatal(char *fmt, ...) ATTRIBUTE((noreturn,format (printf, 1, 2)));

#endif



