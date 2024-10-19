/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef PIKE_ERROR_H
#define PIKE_ERROR_H

#ifdef CONFIGURE_TEST

#include <stdio.h>

static inline void Pike_fatal (const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  vfprintf (stderr, fmt, args);
  abort();
}

#define Pike_error Pike_fatal

#else  /* !CONFIGURE_TEST */

#include "global.h"
#include <setjmp.h>

#if defined(HAVE_SIGSETJMP) && defined(HAVE_SIGLONGJMP)
#define HAVE_AND_USE_SIGSETJMP
#define LOW_JMP_BUF		sigjmp_buf
#define LOW_SETJMP(X)		sigsetjmp(X, 0)
#define LOW_SETJMP_FUNC         sigsetjmp
#define LOW_LONGJMP(X, Y)	siglongjmp(X, Y)
#elif defined(HAVE__SETJMP) && defined(HAVE__LONGJMP)
#define HAVE_AND_USE__SETJMP
#define LOW_JMP_BUF		jmp_buf
#define LOW_SETJMP(X)		_setjmp(X)
#define LOW_SETJMP_FUNC         _setjmp
#define LOW_LONGJMP(X, Y)	_longjmp(X, Y)
#else
/* Assume we have setjmp and longjmp, they are after all defined by ANSI C. */
#define HAVE_AND_USE_SETJMP
#define LOW_JMP_BUF		jmp_buf
#define LOW_SETJMP(X)		setjmp(X)
#define LOW_SETJMP_FUNC         setjmp
#define LOW_LONGJMP(X, Y)	longjmp(X, Y)
#endif

PMOD_EXPORT extern const char msg_fatal_error[];
#define Pike_fatal \
 (fprintf(stderr,msg_fatal_error,__FILE__,(long)__LINE__),debug_fatal)

#define pike_fatal_dloc							\
 (fprintf (stderr, msg_fatal_error, DLOC_ARGS), debug_fatal)

PMOD_EXPORT void pike_atfatal(void (*fun)(void));
PMOD_EXPORT DECLSPEC(noreturn) void debug_va_fatal(const char *fmt, va_list args) ATTRIBUTE((noreturn));
PMOD_EXPORT DECLSPEC(noreturn) void debug_fatal(const char *fmt, ...) ATTRIBUTE((noreturn));


#include "svalue.h"


typedef void (*error_call)(void *);

#ifndef STRUCT_FRAME_DECLARED
#define STRUCT_FRAME_DECLARED
struct pike_frame;
#endif

#define THROW_N_A 0
#define THROW_ERROR 10
#define THROW_THREAD_EXIT 20
#define THROW_THREAD_KILLED 30
#define THROW_EXIT 40
#define THROW_MAX_SEVERITY 100

/* #define ONERROR_DEBUG */

#ifdef ONERROR_DEBUG
#define OED_FPRINTF(...)	fprintf(stderr, __VA_ARGS__)
#else /* !ONERROR_DEBUG */
#define OED_FPRINTF(...)
#endif /* ONERROR_DEBUG */

typedef struct ONERROR
{
  struct ONERROR *previous;
  struct pike_frame *frame_pointer;
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
  LOW_JMP_BUF recovery;
  struct pike_frame *frame_pointer;
  ptrdiff_t stack_pointer;
  ptrdiff_t mark_sp;
  INT32 severity;
  ONERROR *onerror;
#ifdef PIKE_DEBUG
  int line;
  const char *file;
  int on_stack;
#endif
} JMP_BUF;

PMOD_EXPORT extern struct svalue throw_value;
PMOD_EXPORT extern int throw_severity;

#ifdef PIKE_DEBUG
PMOD_EXPORT extern const char msg_unsetjmp_nosync_1[];
PMOD_EXPORT extern const char msg_unsetjmp_nosync_2[];
#define UNSETJMP(X) do{						\
    check_recovery_context();					\
    OED_FPRINTF("unsetjmp(%p) %s:%d\n",                         \
                 &(X),  __FILE__, __LINE__);			\
    if(Pike_interpreter.recoveries != &X) {			\
      if(Pike_interpreter.recoveries)				\
	Pike_fatal(msg_unsetjmp_nosync_1,			\
		   Pike_interpreter.recoveries->file);		\
      else							\
	Pike_fatal("%s", msg_unsetjmp_nosync_2);		\
    }								\
    Pike_interpreter.recoveries=X.previous;			\
    check_recovery_context();					\
  }while (0)

#ifdef DMALLOC_LOCATION
#define PERR_LOCATION() DMALLOC_LOCATION()
#else
#define PERR_LOCATION() ( __FILE__ ":" DEFINETOSTR(__LINE__) )
#endif

#define DEBUG_INIT_REC_ARGS , int on_stack, const char *location
#define SETJMP(X) LOW_SETJMP((init_recovery(&X, 0, 1, PERR_LOCATION())->recovery))
#define SETJMP_SP(jmp, stack_pop_levels)				\
  LOW_SETJMP((init_recovery(&jmp, stack_pop_levels, 1, PERR_LOCATION())->recovery))
#else  /* !PIKE_DEBUG */
#define DEBUG_INIT_REC_ARGS
#define SETJMP(X) LOW_SETJMP((init_recovery(&X, 0)->recovery))
#define SETJMP_SP(jmp, stack_pop_levels)				\
  LOW_SETJMP((init_recovery(&jmp, stack_pop_levels)->recovery))
#define UNSETJMP(X) Pike_interpreter.recoveries=X.previous
#endif	/* !PIKE_DEBUG */


#ifdef PIKE_DEBUG
#define LOW_SET_ONERROR(X,Y,Z)						\
  do{									\
    check_recovery_context();						\
    OED_FPRINTF("SET_ONERROR(%p, %p, %p) %s:%d\n",			\
		(X), (Y), (void *)(Z), __FILE__, __LINE__);		\
    (X)->frame_pointer = Pike_interpreter.frame_pointer;		\
    (X)->func = (error_call)(Y);					\
    DO_IF_DMALLOC( if( (X)->func == free ) (X)->func = dmalloc_free);	\
    (X)->arg = (void *)(Z);						\
    if (!Pike_interpreter.recoveries) {					\
      (X)->previous = NULL;						\
      break;								\
    }									\
    (X)->previous = Pike_interpreter.recoveries->onerror;		\
    (X)->file = __FILE__;						\
    (X)->line = __LINE__;						\
    Pike_interpreter.recoveries->onerror = (X);				\
  }while(0)

#define SET_ONERROR(X,Y,Z) LOW_SET_ONERROR(&X, Y, Z)

PMOD_EXPORT extern const char msg_last_setjmp[];
PMOD_EXPORT extern const char msg_unset_onerr_nosync_1[];
PMOD_EXPORT extern const char msg_unset_onerr_nosync_2[];
#define UNSET_ONERROR(X) do {					\
    check_recovery_context();					\
    OED_FPRINTF("UNSET_ONERROR(%p) %s:%d\n",                    \
                 &(X), __FILE__, __LINE__);			\
    if(!Pike_interpreter.recoveries) break;			\
    if(Pike_interpreter.recoveries->onerror != &(X)) {		\
      fprintf(stderr,msg_last_setjmp,				\
	      Pike_interpreter.recoveries->file);		\
      if (Pike_interpreter.recoveries->onerror) {		\
	Pike_fatal(msg_unset_onerr_nosync_1,			\
	           Pike_interpreter.recoveries->onerror, &(X),	\
                   Pike_interpreter.recoveries->onerror->file); \
      } else {							\
	Pike_fatal("%s", msg_unset_onerr_nosync_2);		\
      }								\
    }								\
    Pike_interpreter.recoveries->onerror=(X).previous;		\
  } while(0)
#else /* !PIKE_DEBUG */
#define LOW_SET_ONERROR(X,Y,Z) \
  do{ \
     (X)->func=(error_call)(Y); \
     (X)->arg=(void *)(Z); \
     (X)->frame_pointer = Pike_interpreter.frame_pointer; \
     if(!Pike_interpreter.recoveries) {		       \
       (X)->previous = NULL;			       \
       break;					       \
     }						       \
     (X)->previous=Pike_interpreter.recoveries->onerror; \
     Pike_interpreter.recoveries->onerror=(X); \
  }while(0)

#define SET_ONERROR(X,Y,Z) LOW_SET_ONERROR(&X, Y, Z)

#define UNSET_ONERROR(X) Pike_interpreter.recoveries && (Pike_interpreter.recoveries->onerror=X.previous)

#endif /* PIKE_DEBUG */

#define CALL_AND_UNSET_ONERROR(X) do {		\
     UNSET_ONERROR(X);				\
     X.func(X.arg);				\
  }while(0)

#define PIKE_ERROR(NAME, TEXT, SP, ARGS) new_error(NAME, TEXT, SP, ARGS)

/* Prototypes begin here */
PMOD_EXPORT void check_recovery_context(void);
PMOD_EXPORT void pike_gdb_breakpoint(INT32 args);
PMOD_EXPORT JMP_BUF *init_recovery(JMP_BUF *r, size_t stack_pop_levels DEBUG_INIT_REC_ARGS);
PMOD_EXPORT DECLSPEC(noreturn) void pike_throw(void) ATTRIBUTE((noreturn));
PMOD_EXPORT DECLSPEC(noreturn) void low_error(const char *buf) ATTRIBUTE((noreturn));
PMOD_EXPORT void va_make_error (const char *fmt, va_list args);
PMOD_EXPORT void DECLSPEC(noreturn) va_error(const char *fmt, va_list args) ATTRIBUTE((noreturn));
PMOD_EXPORT void make_error (const char *fmt, ...)
  ATTRIBUTE((format (printf, 1, 2)));
PMOD_EXPORT DECLSPEC(noreturn) void Pike_error(const char *fmt,...)
  ATTRIBUTE((noreturn)) ATTRIBUTE((format (printf, 1, 2)));
PMOD_EXPORT DECLSPEC(noreturn) void new_error(const char *name, const char *text, struct svalue *oldsp, INT32 args) ATTRIBUTE((noreturn));
PMOD_EXPORT void exit_on_error(const void *msg);
PMOD_EXPORT void fatal_on_error(const void *msg);
PMOD_EXPORT DECLSPEC(noreturn) void debug_fatal(const char *fmt, ...)
  ATTRIBUTE((noreturn)) ATTRIBUTE((format (printf, 1, 2)));
PMOD_EXPORT DECLSPEC(noreturn) void generic_error_va(
  struct object *o, const char *func, const struct svalue *base_sp, int args,
  const char *fmt, va_list *fmt_args)
  ATTRIBUTE((noreturn));
PMOD_EXPORT DECLSPEC(noreturn) void throw_error_object(
  struct object *o,
  const char *func, int args,
  const char *desc, ...)
  ATTRIBUTE((noreturn)) ATTRIBUTE((format (printf, 4, 5)));
PMOD_EXPORT void DECLSPEC(noreturn) generic_error(
  const char *func,
  const struct svalue *base_sp,  int args,
  const char *desc, ...)
  ATTRIBUTE((noreturn)) ATTRIBUTE((format (printf, 4, 5)));
PMOD_EXPORT DECLSPEC(noreturn) void index_error(
  const char *func, int args,
  struct svalue *val,
  struct svalue *ind,
  const char *desc, ...)
  ATTRIBUTE((noreturn)) ATTRIBUTE((format (printf, 5, 6)));
PMOD_EXPORT DECLSPEC(noreturn) void bad_arg_error(
  const char *func, int args,
  int which_arg,
  const char *expected_type,
  struct svalue *got,
  const char *desc, ...)
  ATTRIBUTE((noreturn)) ATTRIBUTE((format (printf, 6, 7)));
PMOD_EXPORT void DECLSPEC(noreturn) math_error(
  const char *func, int args,
  struct svalue *number,
  const char *desc, ...)
  ATTRIBUTE((noreturn)) ATTRIBUTE((format (printf, 4, 5)));
PMOD_EXPORT void DECLSPEC(noreturn) out_of_memory_error (
  const char *func, int args,
  size_t amount)
  ATTRIBUTE((noreturn));
PMOD_EXPORT void wrong_number_of_args_error(const char *name, int args,
                                            int expected)
  ATTRIBUTE((noreturn));
void init_error(void);
void cleanup_error(void);
/* Prototypes end here */

/* Some useful error macros. */

PMOD_EXPORT extern const char msg_bad_arg[];

/**
 * Throw an exception that the type of an argument is wrong. ARG, FUNC
 * and EXPECT will be inserted into "Bad argument %d to %s(). Expected
 * %s.\n"
 *
 * @param FUNC The name of the function, e.g. "create" REQUIRED.
 * @param ARG The number of the argument, e.g. 1 for the first.
 * @param EXPECT The expected type, e.g. "int(0..1)".
 */
#define SIMPLE_ARG_TYPE_ERROR(FUNC, ARG, EXPECT) \
   bad_arg_error(FUNC, args, ARG, EXPECT, Pike_sp+ARG-1-args,\
		 msg_bad_arg, ARG, FUNC, EXPECT)
/* Less descriptive macro name kept for compatibility. */
#define SIMPLE_BAD_ARG_ERROR(FUNC, ARG, EXPECT) \
   bad_arg_error(FUNC, args, ARG, EXPECT, Pike_sp+ARG-1-args,\
		 msg_bad_arg, ARG, FUNC, EXPECT)

PMOD_EXPORT extern const char msg_bad_arg_2[];

/**
 * Throw an exception that there is some problem with the argument
 * other than the wrong type. PROBLEM is a full sentence without a
 * trailing newline. Together with FUNC and ARG they will be inserted
 * into "Bad argument %d to %s(). %s\n"
 *
 * @param FUNC The name of the function, e.g. "create".
 * @param ARG The number of the argument, e.g. 1 for the first.
 * @param PROBLEM Describes the problem with the argument, e.g. "The
 * number of bytes has to be even."
 */
#define SIMPLE_ARG_ERROR(FUNC, ARG, PROBLEM) \
  bad_arg_error (FUNC, args, ARG, NULL, Pike_sp+ARG-1-args, \
		 msg_bad_arg_2, ARG, FUNC, PROBLEM)

/**
 * Throw an exception that the number of arguents to the function is
 * wrong. This macro will use the variable args as the number of given
 * arguments.
 *
 * @param FUNC The name of the function, e.g. "create".
 * @param ARG The number of expected arguments.
 */
#define SIMPLE_WRONG_NUM_ARGS_ERROR(FUNC, ARG) \
  wrong_number_of_args_error (FUNC, args, ARG)
/* The following is for compatibility. */
#define SIMPLE_TOO_FEW_ARGS_ERROR(FUNC, ARG) \
  wrong_number_of_args_error (FUNC, args, ARG)

PMOD_EXPORT extern const char msg_out_of_mem[];
PMOD_EXPORT extern const char msg_out_of_mem_2[];

#define SIMPLE_OUT_OF_MEMORY_ERROR(FUNC, AMOUNT) \
   out_of_memory_error(FUNC, args, AMOUNT)

PMOD_EXPORT extern const char msg_div_by_zero[];
#define SIMPLE_DIVISION_BY_ZERO_ERROR(FUNC) \
  ((void (*)(const char *, int, struct svalue *, const char *, ...)) \
   math_error)(FUNC, args, 0, msg_div_by_zero)

#ifndef PIKE_DEBUG
#define check_recovery_context() ((void)0)
#endif

/* Generic error stuff */
#define ERR_EXT_DECLARE
#include "errors.h"

#endif	/* !CONFIGURE_TEST */

#endif /* PIKE_ERROR_H */
