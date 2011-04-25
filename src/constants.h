/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#ifndef ADD_EFUN_H
#define ADD_EFUN_H

#include "svalue.h"
#include "hashtable.h"
#include "las.h" /* For OPT_SIDE_EFFECT etc. */
#include "block_alloc_h.h"

typedef int (*docode_fun)(node *n);
typedef node *(*optimize_fun)(node *n);

#define CALLABLE_DYNAMIC 1

struct callable
{
  PIKE_MEMORY_OBJECT_MEMBERS;
  c_fun function;
  struct pike_type *type;
  struct pike_string *name;
  struct program *prog;
  INT16 flags; /* OPT_* */
  INT16 internal_flags;
#ifdef PIKE_DEBUG
  INT8 may_return_void;
  long compiles;
  long runs;
  struct callable *prev;
#endif
  optimize_fun optimize;
  docode_fun docode;
  struct callable *next;
};

#ifdef PIKE_DEBUG
/* We have a double-linked list in debug mode for identification
 * purposes. */
extern struct callable *first_callable;
#endif

/* Prototypes begin here */
PMOD_EXPORT struct mapping *get_builtin_constants(void);
void low_add_efun(struct pike_string *name, struct svalue *fun);
void low_add_constant(const char *name, struct svalue *fun);
PMOD_EXPORT void add_global_program(const char *name, struct program *p);
BLOCK_ALLOC_FILL_PAGES(callable,2)
PMOD_EXPORT struct callable *low_make_callable(c_fun fun,
				   struct pike_string *name,
				   struct pike_type *type,
				   int flags,
				   optimize_fun optimize,
				   docode_fun docode);
PMOD_EXPORT struct callable *make_callable(c_fun fun,
			       const char *name,
			       const char *type,
			       int flags,
			       optimize_fun optimize,
			       docode_fun docode);
PMOD_EXPORT struct callable *add_efun2(const char *name,
			    c_fun fun,
			    const char *type,
			    int flags,
			    optimize_fun optimize,
			    docode_fun docode);
PMOD_EXPORT struct callable *add_efun(const char *name, c_fun fun, const char *type, int flags);
PMOD_EXPORT struct callable *quick_add_efun(const char *name, ptrdiff_t name_length,
					    c_fun fun,
					    const char *type, ptrdiff_t type_length,
					    int flags,
					    optimize_fun optimize,
					    docode_fun docode);
void init_builtin_constants(void);
void exit_builtin_constants(void);
/* Prototypes end here */


#include "pike_macros.h"

#define ADD_EFUN2(NAME,FUN,TYPE,OPT_FLAGS,OPTIMIZE,DOCODE) \
    quick_add_efun(NAME,CONSTANT_STRLEN(NAME),FUN, \
                   TYPE,CONSTANT_STRLEN(TYPE),OPT_FLAGS,OPTIMIZE,DOCODE)

#define ADD_EFUN(NAME,FUN,TYPE,OPT_FLAGS) \
    ADD_EFUN2(NAME,FUN,TYPE,OPT_FLAGS,0,0)

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
