/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef ADD_EFUN_H
#define ADD_EFUN_H

#include "svalue.h"
#include "hashtable.h"
#include "las.h" /* For OPT_SIDE_EFFECT etc. */

typedef void (*c_fun)(INT32);
typedef int (*docode_fun)(node *n);
typedef node *(*optimize_fun)(node *n);

struct callable
{
  INT32 refs;
  c_fun function;
  struct pike_string *type;
  struct pike_string *name;
  INT16 flags;
  optimize_fun optimize;
  docode_fun docode;
};

/* Prototypes begin here */
struct mapping *get_builtin_constants(void);
void low_add_efun(struct pike_string *name, struct svalue *fun);
void low_add_constant(char *name, struct svalue *fun);
void add_global_program(char *name, struct program *p);
struct callable *make_callable(c_fun fun,
			       char *name,
			       char *type,
			       INT16 flags,
			       optimize_fun optimize,
			       docode_fun docode);
void really_free_callable(struct callable *fun);
void add_efun2(char *name,
	       c_fun fun,
	       char *type,
	       INT16 flags,
	       optimize_fun optimize,
	       docode_fun docode);
void add_efun(char *name, c_fun fun, char *type, INT16 flags);
void cleanup_added_efuns(void);
void count_memory_in_callables(INT32 *num_, INT32 *size_);
/* Prototypes end here */

#endif
