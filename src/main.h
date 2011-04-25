/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#ifndef MAIN_H
#define MAIN_H

#include "callback.h"

PMOD_EXPORT extern int d_flag, a_flag, l_flag, c_flag, p_flag;
PMOD_EXPORT extern int debug_options, runtime_options;
PMOD_EXPORT extern int default_t_flag;

#ifdef TRY_USE_MMX
extern int try_use_mmx;
#endif /* TRY_USE_MMX */

/* Debug options */
#define DEBUG_SIGNALS 1
#define NO_TAILRECURSION 2
#define NO_PEEP_OPTIMIZING 4
#define GC_RESET_DMALLOC 8
#define ERRORCHECK_MUTEXES 16

/* Runtime options */
#define RUNTIME_CHECK_TYPES  1
#define RUNTIME_STRICT_TYPES 2

/* Prototypes begin here */
PMOD_EXPORT struct callback *add_post_master_callback(callback_func call,
					  void *arg,
					  callback_func free_func);
PMOD_EXPORT struct callback *add_exit_callback(callback_func call,
				   void *arg,
				   callback_func free_func);
int dbm_main(int argc, char **argv);
DECLSPEC(noreturn) void pike_do_exit(int num) ATTRIBUTE((noreturn));
void low_init_main(void);
void exit_main(void);
void init_main(void);
void low_exit_main(void);
/* Prototypes end here */

#endif /* !MAIN_H */
