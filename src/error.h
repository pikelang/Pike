/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: error.h,v 1.44 2000/06/05 14:29:11 grubba Exp $
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
struct pike_frame;
#endif

#define THROW_ERROR 10
#define THROW_THREAD_EXIT 20
#define THROW_THREAD_KILLED 30
#define THROW_EXIT 40
#define THROW_MAX_SEVERITY 100

/* #define ONERROR_DEBUG */

#ifdef ONERROR_DEBUG
#define OED_FPRINTF(X)	fprintf X
#else /* !ONERROR_DEBUG */
#define OED_FPRINTF(X)
#endif /* ONERROR_DEBUG */

typedef struct ONERROR
{
  struct ONERROR *previous;
  error_call func;
  void *arg;
#ifdef PIKE_DEBUG
  const char *file;
  int line;
#endif /* PIKE_DEBUG */
} ONERROR;

typedef struct JMP_BUF
{
  struct JMP_BUF *previous;
  jmp_buf recovery;
  struct pike_frame *Pike_fp;
  INT32 Pike_sp;
  INT32 Pike_mark_sp;
  INT32 severity;
  ONERROR *onerror;
#ifdef PIKE_DEBUG
  int line;
  char *file;
#endif
} JMP_BUF;

extern JMP_BUF *recoveries;
extern struct svalue throw_value;
extern int throw_severity;

#ifdef PIKE_DEBUG
#define UNSETJMP(X) do{ \
   check_recovery_context(); \
   OED_FPRINTF((stderr, "unsetjmp(%p) %s:%d\n", \
                &(X),  __FILE__, __LINE__)); \
  if(recoveries != &X) { \
    if(recoveries) \
      fatal("UNSETJMP out of sync! (last SETJMP at %s:%d)!\n",recoveries->file,recoveries->line); \
    else \
      fatal("UNSETJMP out of sync! (recoveries = 0)\n"); \
    } \
    recoveries=X.previous; \
   check_recovery_context(); \
  }while (0)
#define DEBUG_LINE_ARGS ,int line, char *file
#define SETJMP(X) setjmp((init_recovery(&X,__LINE__,__FILE__)->recovery))
#else
#define DEBUG_LINE_ARGS 
#define SETJMP(X) setjmp((init_recovery(&X)->recovery))
#define UNSETJMP(X) recoveries=X.previous
#endif


#ifdef PIKE_DEBUG
#define SET_ONERROR(X,Y,Z) \
  do{ \
     check_recovery_context(); \
     OED_FPRINTF((stderr, "SET_ONERROR(%p, %p, %p) %s:%d\n", \
                  &(X), (Y), (void *)(Z), __FILE__, __LINE__)); \
     X.func=(error_call)(Y); \
     DO_IF_DMALLOC( if( X.func == free ) X.func=dmalloc_free;) \
     X.arg=(void *)(Z); \
     if(!recoveries) break; \
     X.previous=recoveries->onerror; \
     X.file = __FILE__; \
     X.line = __LINE__; \
     recoveries->onerror=&X; \
  }while(0)

#define UNSET_ONERROR(X) do {\
    check_recovery_context(); \
    OED_FPRINTF((stderr, "UNSET_ONERROR(%p) %s:%d\n", \
                 &(X), __FILE__, __LINE__)); \
    if(!recoveries) break; \
    if(recoveries->onerror != &(X)) { \
      fprintf(stderr,"LAST SETJMP: %s:%d\n",recoveries->file,recoveries->line); \
      if (recoveries->onerror) { \
        fatal("UNSET_ONERROR out of sync (%p != %p).\n" \
              "Last SET_ONERROR is from %s:%d\n",\
              recoveries->onerror, &(X), \
              recoveries->onerror->file, recoveries->onerror->line ); \
      } else { \
        fatal("UNSET_ONERROR out of sync. No recoveries left.\n"); \
      } \
    } \
    recoveries->onerror=(X).previous; \
  } while(0)

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
     X.func=(error_call)(Y); \
     X.arg=(void *)(Z); \
     if(!recoveries) break; \
     X.previous=recoveries->onerror; \
     recoveries->onerror=&X; \
  }while(0)

#define UNSET_ONERROR(X) recoveries && (recoveries->onerror=X.previous)

#define ASSERT_ONERROR(X)
#endif /* PIKE_DEBUG */

#define CALL_AND_UNSET_ONERROR(X) do {		\
     X.func(X.arg);				\
     UNSET_ONERROR(X);				\
  }while(0)

#if defined(PIKE_DEBUG) && 0
/* Works, but probably not interresting for most people
 *	/grubba 1998-04-11
 */
#define PIKE_ERROR(NAME, TEXT, SP, ARGS)	new_error(NAME, TEXT, SP, ARGS, __FILE__, __LINE__);
#else
#define PIKE_ERROR(NAME, TEXT, SP, ARGS)	new_error(NAME, TEXT, SP, ARGS, NULL, 0);
#endif /* PIKE_DEBUG */

/* Prototypes begin here */
void check_recovery_context(void);
void pike_gdb_breakpoint(void);
JMP_BUF *init_recovery(JMP_BUF *r DEBUG_LINE_ARGS);
void pike_throw(void) ATTRIBUTE((noreturn));
void push_error(char *description);
void low_error(char *buf) ATTRIBUTE((noreturn));
void va_error(const char *fmt, va_list args) ATTRIBUTE((noreturn));
void new_error(const char *name, const char *text, struct svalue *oldsp,
	       INT32 args, const char *file, int line) ATTRIBUTE((noreturn));
void exit_on_error(void *msg);
void fatal_on_error(void *msg);
void error(const char *fmt,...) ATTRIBUTE((noreturn,format (printf, 1, 2)));
void debug_fatal(const char *fmt, ...) ATTRIBUTE((noreturn,format (printf, 1, 2)));
void f_error_cast(INT32 args);
void f_error_index(INT32 args);
void f_error_describe(INT32 args);
void f_error_backtrace(INT32 args);
void generic_error_va(struct object *o,
		      char *func,
		      struct svalue *base_sp,  int args,
		      char *fmt,
		      va_list foo)
  ATTRIBUTE((noreturn));
void generic_error(
  char *func,
  struct svalue *base_sp,  int args,
  char *desc, ...) ATTRIBUTE((noreturn,format (printf, 4, 5)));
void index_error(
  char *func,
  struct svalue *base_sp,  int args,
  struct svalue *val,
  struct svalue *ind,
  char *desc, ...) ATTRIBUTE((noreturn,format (printf, 6, 7)));
void bad_arg_error(
  char *func,
  struct svalue *base_sp,  int args,
  int which_arg,
  char *expected_type,
  struct svalue *got,
  char *desc, ...)  ATTRIBUTE((noreturn,format (printf, 7, 8)));
void math_error(
  char *func,
  struct svalue *base_sp,  int args,
  struct svalue *number,
  char *desc, ...) ATTRIBUTE((noreturn,format (printf, 5, 6)));
void resource_error(
  char *func,
  struct svalue *base_sp,  int args,
  char *resource_type,
  long howmuch,
  char *desc, ...) ATTRIBUTE((noreturn,format (printf, 6, 7)));
void permission_error(
  char *func,
  struct svalue *base_sp, int args,
  char *permission_type,
  char *desc, ...) ATTRIBUTE((noreturn, format(printf, 5, 6)));
void wrong_number_of_args_error(char *name, int args, int expected);
void init_error(void);
void cleanup_error(void);
/* Prototypes end here */

#define fatal \
 fprintf(stderr,"%s:%d: Fatal error:\n",__FILE__,__LINE__),debug_fatal

/* Some useful error macros. */

#define SIMPLE_BAD_ARG_ERROR(FUNC, ARG, EXPECT) \
   bad_arg_error(FUNC, Pike_sp-args, args, ARG, EXPECT, Pike_sp+ARG-1-args,\
                 "Bad argument %d to %s(). Expected %s\n", \
                  ARG, FUNC, EXPECT)

#define SIMPLE_TOO_FEW_ARGS_ERROR(FUNC, ARG) \
   bad_arg_error(FUNC, Pike_sp-args, args, ARG, "void", 0,\
                 "Too few arguments to %s().\n",FUNC)

#define SIMPLE_OUT_OF_MEMORY_ERROR(FUNC, AMOUNT) \
   resource_error(FUNC, Pike_sp-args, args, "memory", AMOUNT, "Out of memory.\n")

#define SIMPLE_DIVISION_BY_ZERO_ERROR(FUNC) \
     math_error(FUNC, Pike_sp-args, args, 0, "Division by zero.\n")

#ifndef PIKE_DEBUG
#define check_recovery_context() ((void)0)
#endif

/* Experimental convenience exception macros. */

#define exception_try \
        do \
        { \
            int __exception_rethrow, __is_exception; \
            JMP_BUF exception; \
            __is_exception = SETJMP(exception); \
            __exception_rethrow = 0; \
            if(__is_exception) /* rethrow needs this */ \
                UNSETJMP(exception); \
            if(!__is_exception)
    
#define exception_catch_if \
            else if

#define exception_catch(e) \
            exception_catch_if(exception->severity = (e))

#define exception_catch_all \
            exception_catch_if(1)

#define exception_semicatch_all \
            exception_catch_if((__exception_rethrow = 1))

#define rethrow \
            pike_throw()

#define exception_endtry \
            else \
                __exception_rethrow = 1; \
            if(!__is_exception) \
                UNSETJMP(exception); \
            if(__exception_rethrow) \
                rethrow; \
        } \
        while(0)

/* Generic error stuff */
#define ERR_EXT_DECLARE
#include "errors.h"

#endif
