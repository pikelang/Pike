/*
 * $Id: pike_embed.h,v 1.1 2004/12/29 10:16:43 grubba Exp $
 *
 * Pike embedding API.
 *
 * Henrik Grubbström 2004-12-27
 */

#include "global.h"
#include "callback.h"

#ifdef TRY_USE_MMX
extern int try_use_mmx;
#endif

#ifndef MAXPATHLEN
#define MAXPATHLEN 32768
#endif

extern char master_location[MAXPATHLEN * 2];
extern const char *master_file;
extern char **ARGV;

void init_pike(char **argv);
void pike_set_default_master();
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
