/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: pike_compiler.h,v 1.2 2008/04/14 12:17:44 grubba Exp $
*/

#ifndef PIKE_COMPILER_H
#define PIKE_COMPILER_H

#include "lex.h"
#include "program.h"

struct compilation
{
  struct Supporter supporter;
  struct pike_string *prog;		/* String to compile. */
  struct object *handler;		/* error_handler */
  struct object *compat_handler;	/* compat_handler */
  int major, minor;			/* compat version */
  struct program *target;		/* Program being compiled. */
  struct object *placeholder;
  
  struct program *p;			/* Compiled program or NULL. */
  struct lex lex;
  int compilation_inherit;		/* Inherit in supporter->self containing
					 * compilation_program. */
  int save_depth;
  int saved_threads_disabled;
  dynamic_buffer used_modules_save;
  INT32 num_used_modules_save;
  struct mapping *resolve_cache_save;

  struct svalue default_module;
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

/* Report levels */
#define REPORT_INFO	0	/* FYI. */
#define REPORT_WARNING	1	/* Compiler warning. */
#define REPORT_ERROR	2	/* Compilation error. */
#define REPORT_FATAL	3	/* Unrecoverable error. */

/* Function numbers. */
#define CE_REPORT_FUN_NUM		0
#define CE_PIKE_COMPILER_FUN_NUM	1
#define CE_COMPILE_FUN_NUM		2

#endif	/* !PIKE_COMPILER_H */
