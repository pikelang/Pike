/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
#include "constants.h"
#include "pike_macros.h"
#include "program.h"
#include "pike_types.h"
#include "stralloc.h"
#include "pike_memory.h"
#include "interpret.h"
#include "mapping.h"
#include "error.h"
#include "block_alloc.h"

RCSID("$Id: constants.c,v 1.14 1999/02/10 21:46:39 hubbe Exp $");

static INT32 num_callable=0;
static struct mapping *builtin_constants = 0;

struct mapping *get_builtin_constants(void)
{
  if(!builtin_constants)
    builtin_constants=allocate_mapping(20);

  return builtin_constants;
}

void low_add_efun(struct pike_string *name, struct svalue *fun)
{
  struct svalue s;

  if(!builtin_constants)
    builtin_constants=allocate_mapping(20);

  s.type=T_STRING;
  s.subtype=0;
  s.u.string=name;

  if(fun)
  {
    mapping_insert(builtin_constants, &s, fun);
  }else{
    map_delete(builtin_constants, &s);
  }
}

void low_add_constant(char *name, struct svalue *fun)
{
  struct pike_string *p;
  p=make_shared_string(name);
  low_add_efun(p, fun);
  free_string(p);
}

void add_global_program(char *name, struct program *p)
{
  struct svalue s;
  s.type=T_PROGRAM;
  s.subtype=0;
  s.u.program=p;
  low_add_constant(name, &s);
}

#undef INIT_BLOCK
#define INIT_BLOCK(X) num_callable++
#undef EXIT_BLOCK
#define EXIT_BLOCK(X) do {		\
  free_string(X->type);			\
  free_string(X->name);			\
  num_callable--;			\
}while(0)
BLOCK_ALLOC(callable,128)

/* Eats one ref to 'type' and 'name' */
struct callable *low_make_callable(c_fun fun,
				   struct pike_string *name,
				   struct pike_string *type,
				   INT16 flags,
				   optimize_fun optimize,
				   docode_fun docode)
{
  struct callable *f=alloc_callable();
  f->refs=1;
  f->function=fun;
  f->name=name;
  f->type=type;
  f->flags=flags;
  f->docode=docode;
  f->optimize=optimize;
  return f;
}

struct callable *make_callable(c_fun fun,
			       char *name,
			       char *type,
			       INT16 flags,
			       optimize_fun optimize,
			       docode_fun docode)
{
  return low_make_callable(fun,make_shared_string(name),parse_type(type),flags,optimize,docode);
}

void add_efun2(char *name,
	       c_fun fun,
	       char *type,
	       INT16 flags,
	       optimize_fun optimize,
	       docode_fun docode)
{
  struct svalue s;
  struct pike_string *n;

  n=make_shared_string(name);
  s.type=T_FUNCTION;
  s.subtype=FUNCTION_BUILTIN;
  s.u.efun=make_callable(fun, name, type, flags, optimize, docode);
  low_add_efun(n, &s);
  free_svalue(&s);
  free_string(n);
}

void add_efun(char *name, c_fun fun, char *type, INT16 flags)
{
  add_efun2(name,fun,type,flags,0,0);
}

void quick_add_efun(char *name, int name_length,
		    c_fun fun,
		    char *type, int type_length,
		    INT16 flags,
		    optimize_fun optimize,
		    docode_fun docode)
{
  struct svalue s;
  struct pike_string *n,*t;

  n=make_shared_binary_string(name,name_length);
  t=make_shared_binary_string(type,type_length);
#ifdef DEBUG
  check_type_string(t);
#endif
  s.type=T_FUNCTION;
  s.subtype=FUNCTION_BUILTIN;
  add_ref(n);
  s.u.efun=low_make_callable(fun, n, t, flags, optimize, docode);
  low_add_efun(n, &s);
  free_svalue(&s);
  free_string(n);
}

void cleanup_added_efuns(void)
{
  if(builtin_constants)
  {
    free_mapping(builtin_constants);
    builtin_constants=0;
  }
}
void count_memory_in_callables(INT32 *num_, INT32 *size_)
{
  *num_=num_callable;
  *size_=num_callable*sizeof(struct callable);
}
