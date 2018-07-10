/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef PIKE_COMPILER_H
#define PIKE_COMPILER_H

#include "lex.h"
#include "program.h"

extern struct program *reporter_program;
extern struct program *compilation_env_program;
extern struct program *compilation_program;
extern struct object *compilation_environment;

typedef int supporter_callback (void *, int);

struct Supporter
{
#ifdef PIKE_DEBUG
  int magic;
#endif

  struct Supporter *previous;
  /* Makes up a linked list of supporters with the first one in
   * current_supporter. Supporters are linked onto this list during
   * the (recursive) compilation of each compilation unit (i.e.
   * compiled string). Thus nested programs and programs built from C
   * don't have supporters. */

  struct Supporter *depends_on;
  /* The supporter furthest in on the current_supporter linked list
   * that this one depends on. When it gets unlinked from that list,
   * this becomes a back pointer for the dependants linked list
   * below. */

  struct Supporter *dependants, *next_dependant;
  /* dependants points to a linked list of supporters that depends on
   * this one, and next_dependant makes up the links between those
   * supporters. A supporter is linked onto this list when it is
   * unlinked from the current_supporter list. */

  struct object *self;
  supporter_callback *fun;
  void *data;

  struct program *prog;
  /* The top level program in the compilation unit. */
};

struct compilation
{
  struct Supporter supporter;
  struct pike_string *prog;		/* String to compile. */
  struct object *handler;		/* error_handler */
  struct object *compat_handler;	/* compat_handler */
  int major, minor;			/* Base compat version */
  struct program *target;		/* Program being compiled. */
  struct object *placeholder;
  int flags;

  struct program *p;			/* Compiled program or NULL. */
  struct lex lex;
  int compilation_inherit;		/* Inherit in supporter->self containing
					 * compilation_program. */

  struct svalue default_module;		/* predef:: */
  struct byte_buffer used_modules;		/* Stack of svalues with imported
					 * modules. */
  INT32 num_used_modules;		/* Number of entries on the stack. */

  int compilation_depth;		/* Current class nesting depth. */

#ifdef PIKE_THREADS
  int saved_lock_depth;
#endif
  struct mapping *resolve_cache;
};

#ifdef PIKE_DEBUG
#define CHECK_COMPILER()	do {				\
    if (!Pike_fp || !compilation_program ||			\
	Pike_fp->context->prog != compilation_program) {	\
      Pike_fatal("Invalid compilation context!\n");		\
    }								\
  } while(0)
#else
#define CHECK_COMPILER()
#endif
#define THIS_COMPILATION  ((struct compilation *)(Pike_fp->current_storage))
#define MAYBE_THIS_COMPILATION  ((Pike_fp && compilation_program && (Pike_fp->context->prog == compilation_program))?THIS_COMPILATION:NULL)

/* Complier passes */
#define COMPILER_PASS_NONE	0
#define COMPILER_PASS_FIRST	1
#define COMPILER_PASS_EXTRA	2
#define COMPILER_PASS_LAST	3

/* Flags for struct compilation. */
#define COMPILER_BUSY			1	/* The compiler is busy. */
#define COMPILER_DONE			2	/* The compiler is finished. */
#define COMPILER_NEED_EXTRA_PASS	4	/* Run an extra pass. */

/* CompilerEnvironment function numbers. */
#define CE_REPORT_FUN_NUM				0
#define CE_COMPILE_FUN_NUM				1
#define CE_RESOLV_FUN_NUM				2
#define CE_PIKE_COMPILER_FUN_NUM			3
#define CE_GET_COMPILATION_HANDLER_FUN_NUM		4
#define CE_GET_DEFAULT_MODULE_FUN_NUM			5
#define CE_HANDLE_INHERIT_FUN_NUM			6
#define CE_HANDLE_IMPORT_FUN_NUM			7

/* PikeCompiler function numbers. */
#define PC_REPORT_FUN_NUM				0
#define PC_COMPILE_FUN_NUM				1
#define PC_RESOLV_FUN_NUM				2
#define PC_CREATE_FUN_NUM				3
#define PC_GET_COMPILATION_HANDLER_FUN_NUM		4
#define PC_GET_DEFAULT_MODULE_FUN_NUM			5
#define PC_CHANGE_COMPILER_COMPATIBILITY_FUN_NUM	6
#define PC_HANDLE_INHERIT_FUN_NUM			7
#define PC_HANDLE_IMPORT_FUN_NUM			8
#define PC_POP_TYPE_ATTRIBUTE_FUN_NUM			9
#define PC_PUSH_TYPE_ATTRIBUTE_FUN_NUM			10
#define PC_APPLY_TYPE_ATTRIBUTE_FUN_NUM			11
#define PC_APPLY_ATTRIBUTE_CONSTANT_FUN_NUM		12

extern struct program *null_program;
extern struct program *placeholder_program;
extern struct object *placeholder_object;

/* Prototypes begin here */
PMOD_EXPORT void lock_pike_compiler(void);
PMOD_EXPORT void unlock_pike_compiler(void);
void verify_supporters(void);
void init_supporter(struct Supporter *s,
		    supporter_callback *fun,
		    void *data);
int unlink_current_supporter(struct Supporter *c);
int call_dependants(struct Supporter *s, int finish);
int report_compiler_dependency(struct program *p);
struct compilation;
void run_pass2(struct compilation *c);
PMOD_EXPORT void enter_compiler(struct pike_string *filename,
				INT_TYPE linenumber);
PMOD_EXPORT void exit_compiler(void);
PMOD_EXPORT void va_yyreport(int severity_level,
			     struct pike_string *file, INT_TYPE line,
			     struct pike_string *system, INT32 args,
			     const char *fmt, va_list vargs);
PMOD_EXPORT void low_yyreport(int severity_level,
			      struct pike_string *file, INT_TYPE line,
			      struct pike_string *system,
			      INT32 args, const char *fmt, ...);
PMOD_EXPORT void yyreport(int severity_level, struct pike_string *system,
			  INT32 args, const char *fmt, ...);
PMOD_EXPORT void yywarning(char *fmt, ...);
PMOD_EXPORT void my_yyerror(const char *fmt,...);
PMOD_EXPORT void yyerror(const char *s);
void yytype_report(int severity_level,
		   struct pike_string *expect_file, INT_TYPE expect_line,
		   struct pike_type *expected_t,
		   struct pike_string *got_file, INT_TYPE got_line,
		   struct pike_type *got_t,
		   INT32 args, const char *fmt, ...);
void yytype_error(const char *msg, struct pike_type *expected_t,
		  struct pike_type *got_t, unsigned int flags);
struct pike_string *format_exception_for_error_msg (struct svalue *thrown);
void handle_compile_exception (const char *yyerror_fmt, ...);
void push_compiler_frame(int lexical_scope);
node *low_pop_local_variables(int level, node *block);
node *pop_local_variables(int level, node *block);
void pop_compiler_frame(void);
PMOD_EXPORT void change_compiler_compatibility(int major, int minor);
void init_pike_compiler(void);
void cleanup_pike_compiler(void);
/* Prototypes end here */

#endif	/* !PIKE_COMPILER_H */
