/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: main.h,v 1.12 1999/12/13 01:21:11 grubba Exp $
 */
#ifndef MAIN_H
#define MAIN_H

#include "callback.h"

extern int d_flag, t_flag, a_flag, l_flag, c_flag, p_flag;
extern int debug_options, runtime_options;
extern int default_t_flag;

#ifdef TRY_USE_MMX
extern int try_use_mmx;
#endif

/* Debug options */
#define DEBUG_SIGNALS 1
#define NO_TAILRECURSION 2

/* Runtime options */
#define RUNTIME_CHECK_TYPES  1
#define RUNTIME_STRICT_TYPES 2

/* Prototypes begin here */
struct callback *add_post_master_callback(callback_func call,
					  void *arg,
					  callback_func free_func);
struct callback *add_exit_callback(callback_func call,
				   void *arg,
				   callback_func free_func);
int dbm_main(int argc, char **argv);
void do_exit(int num) ATTRIBUTE((noreturn));
void low_init_main(void);
void exit_main(void);
void init_main(void);
void low_exit_main(void);
/* Prototypes end here */

#endif
