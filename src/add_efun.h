/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef ADD_EFUN_H
#define ADD_EFUN_H

#include "svalue.h"
#include "hashtable.h"
#include "las.h" /* For OPT_SIDE_EFFECT etc. */

struct efun
{
  struct svalue function;
  struct hash_entry link;
};

typedef void (*c_fun)(INT32);
typedef int (*docode_fun)(node *n);
typedef node *(*optimize_fun)(node *n);

struct callable
{
  INT32 refs;
  c_fun function;
  struct lpc_string *type;
  struct lpc_string *name;
  INT16 flags;
  optimize_fun optimize;
  docode_fun docode;
};

/* Prototypes begin here */
struct efun *lookup_efun(struct lpc_string *name);
void low_add_efun(struct lpc_string *name, struct svalue *fun);
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
void push_all_efuns_on_stack();
void cleanup_added_efuns();
/* Prototypes end here */

#endif
