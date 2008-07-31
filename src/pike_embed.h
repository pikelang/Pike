/*
 * $Id: pike_embed.h,v 1.12 2008/07/31 18:13:17 mast Exp $
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
#define DEBUG_SIGNALS		0x0001
#define NO_TAILRECURSION	0x0002
#define NO_PEEP_OPTIMIZING	0x0004
#define GC_RESET_DMALLOC	0x0008
#define ERRORCHECK_MUTEXES	0x0010
#define WINDOWS_ERROR_DIALOGS	0x0020

int set_pike_debug_options(int bits, int mask);

/* Runtime options */
#define RUNTIME_CHECK_TYPES	1
#define RUNTIME_STRICT_TYPES	2

int set_pike_runtime_options(int bits, int mask);

extern const char *master_file;
extern char **ARGV;

void init_pike(char **argv, const char *file);
void pike_set_default_master(void);
void pike_set_master_file(const char *file);
void init_pike_runtime(void (*exit_cb)(int));
void set_pike_evaluator_limit(unsigned long num_instrs);
PMOD_EXPORT struct callback *add_post_master_callback(callback_func call,
						      void *arg,
						      callback_func free_func);
struct object *load_pike_master(void);
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
void pike_do_exit(int num);

void pike_push_argv(int argc, char **argv);

#endif /* PIKE_EMBED_H */
