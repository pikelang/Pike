/*
 * $Id: pike_embed.h,v 1.3 2005/01/01 14:35:45 grubba Exp $
 *
 * Pike embedding API.
 *
 * Henrik Grubbström 2004-12-27
 */

#ifndef PIKE_EMBED_H
#define PIKE_EMBED_H

#include "global.h"
#include "callback.h"

#ifdef TRY_USE_MMX
extern int try_use_mmx;
#endif /* TRY_USE_MMX */

PMOD_EXPORT extern int d_flag, a_flag, l_flag, c_flag, p_flag;
PMOD_EXPORT extern int debug_options, runtime_options;
PMOD_EXPORT extern int default_t_flag;
#if defined(YYDEBUG) || defined(PIKE_DEBUG)
extern int yydebug;
#endif /* YYDEBUG || PIKE_DEBUG */

/* Debug options */
#define DEBUG_SIGNALS 1
#define NO_TAILRECURSION 2
#define NO_PEEP_OPTIMIZING 4
#define GC_RESET_DMALLOC 8
#define ERRORCHECK_MUTEXES 16

int set_pike_debug_options(int bits, int mask);

/* Runtime options */
#define RUNTIME_CHECK_TYPES  1
#define RUNTIME_STRICT_TYPES 2

int set_pike_runtime_options(int bits, int mask);

extern const char *master_file;
extern char **ARGV;

void init_pike(char **argv, const char *file);
void pike_set_default_master(void);
void pike_set_master_file(const char *file);
void init_pike_runtime(void (*exit_cb)(int));
#ifdef PROFILING
#ifdef PIKE_DEBUG
void gdb_break_on_pike_stack_record(long stack_size);
#endif
extern long record;
#endif
void pike_enable_stack_profiling(void);
PMOD_EXPORT struct callback *add_exit_callback(callback_func call,
					       void *arg,
					       callback_func free_func);
DECLSPEC(noreturn) void pike_do_exit(int num) ATTRIBUTE((noreturn));

void pike_push_argv(int argc, char **argv);
void pike_push_env(void);

#endif /* PIKE_EMBED_H */
