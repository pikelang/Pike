#ifndef ERROR_H
#define ERROR_H

#include <setjmp.h>
#include <stdarg.h>
#include "machine.h"
#include "svalue.h"


typedef void (*function)(INT32);

struct frame;

typedef struct JMP_BUF
{
  struct JMP_BUF *previous;
  jmp_buf recovery;
  struct frame *fp;
  struct svalue *sp;
  struct svalue **mark_sp;
} JMP_BUF;

extern JMP_BUF *recoveries;
extern struct svalue throw_value;
extern char *automatic_fatal;

#define SETJMP(X) setjmp((init_recovery(&X)[0]))
#define UNSETJMP(X) recoveries=X.previous;

jmp_buf *init_recovery(JMP_BUF *r);
int fix_recovery(int i, JMP_BUF *r);
void throw() ATTRIBUTE((noreturn));
void va_error(char *fmt, va_list args) ATTRIBUTE((noreturn));
void error(char *fmt,...) ATTRIBUTE((noreturn,format (printf, 1, 2)));
void fatal(char *fmt, ...) ATTRIBUTE((noreturn,format (printf, 1, 2)));

#endif



