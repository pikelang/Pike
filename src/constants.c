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

RCSID("$Id: constants.c,v 1.13 1998/03/28 15:37:45 grubba Exp $");

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

struct callable *make_callable(c_fun fun,
			       char *name,
			       char *type,
			       INT16 flags,
			       optimize_fun optimize,
			       docode_fun docode)
{
  struct callable *f;
  f=ALLOC_STRUCT(callable);
  num_callable++;
  f->refs=1;
  f->function=fun;
  f->name=make_shared_string(name);
  f->type=parse_type(type);
  f->flags=flags;
  f->docode=docode;
  f->optimize=optimize;
  return f;
}

void really_free_callable(struct callable *fun)
{
  free_string(fun->type);
  free_string(fun->name);
  free((char *)fun);
  num_callable--;
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
