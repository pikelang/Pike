/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: error.h,v 1.26 1999/01/21 09:15:00 hubbe Exp $
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

#ifdef ONERROR_DEBUG
#define OED_FPRINTF(X)	fprintf X
#else /* !ONERROR_DEBUG */
#define OED_FPRINTF(X)
#endif /* ONERROR_DEBUG */

typedef struct ONERROR
{
  struct ONERROR *previous;
#ifdef PIKE_DEBUG
  const char *file;
  int line;
#endif /* PIKE_DEBUG */
  error_call func;
  void *arg;
} ONERROR;

typedef struct JMP_BUF
{
#ifdef PIKE_DEBUG
  int line;
  char *file;
#endif
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

#ifdef PIKE_DEBUG
#define UNSETJMP(X) do{ \
  if(recoveries != &X) { \
    if(recoveries) \
      fatal("UNSETJMP out of sync! (last SETJMP at %s:%d)!\n",recoveries->file,recoveries->line); \
    else \
      fatal("UNSETJMP out of sync! (recoveries = 0)\n"); \
    } \
    recoveries=X.previous; \
  }while (0)
#define DEBUG_LINE_ARGS ,int line, char *file
#define SETJMP(X) setjmp((init_recovery(&X,__LINE__,__FILE__)->recovery))
#else
#define DEBUG_LINE_ARGS 
#define SETJMP(X) setjmp((init_recovery(&X)->recovery))
#define UNSETJMP(X) recoveries=X.previous;
#endif


#ifdef PIKE_DEBUG
#define SET_ONERROR(X,Y,Z) \
  do{ \
     OED_FPRINTF((stderr, "SET_ONERROR(%p, %p, %p) %s:%d\n", \
                  &(X), (Y), (Z), __FILE__, __LINE__)); \
     X.func=(error_call)(Y); \
     X.arg=(void *)(Z); \
     if(!recoveries) break; \
     X.previous=recoveries->onerror; \
     X.file = __FILE__; \
     X.line = __LINE__; \
     recoveries->onerror=&X; \
  }while(0)

#define UNSET_ONERROR(X) do {\
    OED_FPRINTF((stderr, "UNSET_ONERROR(%p) %s:%d\n", \
                 &(X), __FILE__, __LINE__)); \
    if(!recoveries) break; \
    if(recoveries->onerror != &(X)) { \
      fprintf(stderr,"LAST SETJMP: %s:%d\n",recoveries->file,recoveries->line); \
      if (recoveries->onerror) { \
        fatal("UNSET_ONERROR out of sync.\n" \
              "Last SET_ONERROR is from %s:%d\n", \
              recoveries->onerror->file, recoveries->onerror->line ); \
      } else { \
        fatal("UNSET_ONERROR out of sync. No recoveries left.\n"); \
      } \
    } \
    recoveries->onerror=(X).previous; \
  } while(0)

#define CALL_AND_UNSET_ONERROR(X) do {		\
     X.func(X.arg);				\
     UNSET_ONERROR(X);				\
  }while(0)

#define ASSERT_ONERROR(X) \
  do{ \
    if (!recoveries) break; \
    if (recoveries->onerror != &X) { \
      fatal("%s:%d ASSERT_ONERROR(%p) failed\n", \
            __FILE__, __LINE__, &(X)); \
    } \
  }while(0)
#else /* !PIKE_DEBUG */
#define SET_ONERROR(X,Y,Z) \
  do{ \
     if(!recoveries) break; \
     X.func=(error_call)(Y); \
     X.arg=(void *)(Z); \
     X.previous=recoveries->onerror; \
     recoveries->onerror=&X; \
  }while(0)

#define UNSET_ONERROR(X) recoveries && (recoveries->onerror=X.previous)

#define ASSERT_ONERROR(X)
#endif /* PIKE_DEBUG */

#if defined(PIKE_DEBUG) && 0
/* Works, but probably not interresting for most people
 *	/grubba 1998-04-11
 */
#define PIKE_ERROR(NAME, TEXT, SP, ARGS)	new_error(NAME, TEXT, SP, ARGS, __FILE__, __LINE__);
#else
#define PIKE_ERROR(NAME, TEXT, SP, ARGS)	new_error(NAME, TEXT, SP, ARGS, NULL, 0);
#endif /* PIKE_DEBUG */

/* Prototypes begin here */
JMP_BUF *init_recovery(JMP_BUF *r DEBUG_LINE_ARGS);
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



