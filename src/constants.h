/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: constants.h,v 1.11 1999/12/13 12:08:12 mast Exp $
 */
#ifndef ADD_EFUN_H
#define ADD_EFUN_H

#include "svalue.h"
#include "hashtable.h"
#include "las.h" /* For OPT_SIDE_EFFECT etc. */
#include "block_alloc_h.h"

typedef int (*docode_fun)(node *n);
typedef node *(*optimize_fun)(node *n);

struct callable
{
  INT32 refs;
#ifdef PIKE_SECURITY
  struct object *prot;
#endif
  c_fun function;
  struct pike_string *type;
  struct pike_string *name;
  INT16 flags;
#ifdef PIKE_DEBUG
  INT8 may_return_void;
#endif
  optimize_fun optimize;
  docode_fun docode;
  struct callable *next;
};

/* Prototypes begin here */
struct mapping *get_builtin_constants(void);
void low_add_efun(struct pike_string *name, struct svalue *fun);
void low_add_constant(char *name, struct svalue *fun);
void add_global_program(char *name, struct program *p);
BLOCK_ALLOC(callable,128)
struct callable *low_make_callable(c_fun fun,
				   struct pike_string *name,
				   struct pike_string *type,
				   INT16 flags,
				   optimize_fun optimize,
				   docode_fun docode);
struct callable *make_callable(c_fun fun,
			       char *name,
			       char *type,
			       INT16 flags,
			       optimize_fun optimize,
			       docode_fun docode);
struct callable *add_efun2(char *name,
			    c_fun fun,
			    char *type,
			    INT16 flags,
			    optimize_fun optimize,
			    docode_fun docode);
struct callable *add_efun(char *name, c_fun fun, char *type, INT16 flags);
struct callable *quick_add_efun(char *name, int name_length,
				c_fun fun,
				char *type, int type_length,
				INT16 flags,
				optimize_fun optimize,
				docode_fun docode);
void cleanup_added_efuns(void);
/* Prototypes end here */


#include "pike_macros.h"

#define ADD_EFUN(NAME,FUN,TYPE,FLAGS) \
    quick_add_efun(NAME,CONSTANT_STRLEN(NAME),FUN,TYPE,CONSTANT_STRLEN(TYPE),FLAGS,0,0)

#define ADD_EFUN2(NAME,FUN,TYPE,FLAGS,OPTIMIZE,DOCODE) \
    quick_add_efun(NAME,CONSTANT_STRLEN(NAME),FUN,TYPE,CONSTANT_STRLEN(TYPE),FLAGS,OPTIMIZE,DOCODE)

#define ADD_EFUN_DTYPE(NAME,FUN,DTYPE,FLAGS) do {				\
  DTYPE_START;									\
  {DTYPE}									\
  {										\
    struct pike_string *_t;							\
    DTYPE_END(_t);								\
    quick_add_efun(NAME,CONSTANT_STRLEN(NAME),FUN,_t->str,_t->len,FLAGS,0,0);	\
    free_string(_t);								\
  }										\
} while (0)

#endif
