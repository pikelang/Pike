/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "constants.h"
#include "pike_macros.h"
#include "program.h"
#include "pike_types.h"
#include "stralloc.h"
#include "pike_memory.h"
#include "interpret.h"
#include "mapping.h"
#include "pike_error.h"
#include "pike_security.h"
#include "gc.h"
#include "block_allocator.h"

struct mapping *builtin_constants = 0;

#ifdef PIKE_DEBUG
struct callable *first_callable = NULL;
#endif

/* This is the mapping returned by all_constants(). */
PMOD_EXPORT struct mapping *get_builtin_constants(void)
{
  return builtin_constants;
}

void low_add_efun(struct pike_string *name, struct svalue *fun)
{
  struct svalue s;

  SET_SVAL(s, T_STRING, 0, string, name);

  if(fun)
  {
    mapping_insert(builtin_constants, &s, fun);
  }else{
    map_delete(builtin_constants, &s);
  }
}

void low_add_constant(const char *name, struct svalue *fun)
{
  struct pike_string *p;
  p=make_shared_string(name);
  low_add_efun(p, fun);
  free_string(p);
}

void add_pike_string_constant(const char *name, const char *str, int len)
{
  struct pike_string *key = make_shared_string(name);
  struct pike_string *val = make_shared_binary_string(str, len);
  mapping_string_insert_string(builtin_constants, key, val);
  free_string(val);
  free_string(key);
}

PMOD_EXPORT void add_global_program(const char *name, struct program *p)
{
  struct svalue s;
  SET_SVAL(s, T_PROGRAM, 0, program, p);
  low_add_constant(name, p?&s:NULL);
}

static struct block_allocator callable_allocator
    = BA_INIT_PAGES(sizeof(struct callable), 2);

void really_free_callable(struct callable * c) {
#ifdef PIKE_DEBUG
    DOUBLEUNLINK (first_callable, c);
#endif
    free_type(c->type);
    free_string(c->name);
    c->name=0;
    EXIT_PIKE_MEMOBJ(c);
    ba_free(&callable_allocator, c);
}

void count_memory_in_callables(size_t * num, size_t * size) {
    ba_count_all(&callable_allocator, num, size);
}
void free_all_callable_blocks() {
    ba_destroy(&callable_allocator);
}

int global_callable_flags=0;

/* Eats one ref to 'type' and 'name' */
PMOD_EXPORT struct callable *low_make_callable(c_fun fun,
					       struct pike_string *name,
					       struct pike_type *type,
					       int flags,
					       optimize_fun optimize,
					       docode_fun docode)
{
  struct callable *f=(struct callable*)ba_alloc(&callable_allocator);
#ifdef PIKE_DEBUG
  DOUBLELINK(first_callable, f);
#endif
  INIT_PIKE_MEMOBJ(f, T_STRUCT_CALLABLE);
  f->function=fun;
  f->name=name;
  f->type=type;
  f->prog=Pike_compiler->new_program;
  f->flags=flags;
  f->docode=docode;
  f->optimize=optimize;
  f->internal_flags = global_callable_flags;
#ifdef PIKE_DEBUG
  {
    struct pike_type *z = NULL;
    add_ref(type);
    type = check_splice_call(name, type, 1, mixed_type_string, NULL,
			     CALL_INHIBIT_WARNINGS);
    if (type) {
      z = new_get_return_type(type, CALL_INHIBIT_WARNINGS);
      free_type(type);
    }
    f->may_return_void = (z == void_type_string);
    if(!z) Pike_fatal("Function has no valid return type.\n");
    free_type(z);
  }
  f->runs=0;
#endif
  return f;
}

PMOD_EXPORT struct callable *make_callable(c_fun fun,
			       const char *name,
			       const char *type,
			       int flags,
			       optimize_fun optimize,
			       docode_fun docode)
{
  return low_make_callable(fun, make_shared_string(name), parse_type(type),
			   flags, optimize, docode);
}

PMOD_EXPORT void add_efun2(const char *name,
                           c_fun fun,
                           const char *type,
                           int flags,
                           optimize_fun optimize,
                           docode_fun docode)
{
  struct svalue s;
  struct pike_string *n;

  n=make_shared_string(name);
  SET_SVAL(s, T_FUNCTION, FUNCTION_BUILTIN, efun,
	   make_callable(fun, name, type, flags, optimize, docode));
  low_add_efun(n, &s);
  free_svalue(&s);
  free_string(n);
}

PMOD_EXPORT void add_efun(const char *name, c_fun fun, const char *type, int flags)
{
  add_efun2(name,fun,type,flags,0,0);
}

PMOD_EXPORT void quick_add_efun(const char *name, ptrdiff_t name_length,
                                c_fun fun,
                                const char *type, ptrdiff_t UNUSED(type_length),
                                int flags,
                                optimize_fun optimize,
                                docode_fun docode)
{
  struct svalue s;
  struct pike_string *n;
  struct pike_type *t;

#ifdef PIKE_DEBUG
  if(simple_mapping_string_lookup(builtin_constants, name))
    Pike_fatal("%s added as efun more than once.\n", name);
#endif

  n = make_shared_binary_string(name, name_length);
  t = make_pike_type(type);
#ifdef DEBUG
  check_type_string(t);
#endif
  add_ref(n);
  SET_SVAL(s, T_FUNCTION, FUNCTION_BUILTIN, efun,
	   low_make_callable(fun, n, t, flags, optimize, docode));
  mapping_string_insert(builtin_constants, n, &s);
  free_svalue(&s);
  free_string(n);
}

PMOD_EXPORT void visit_callable (struct callable *c, int action)
{
  switch (action) {
#ifdef PIKE_DEBUG
    default:
      Pike_fatal ("Unknown visit action %d.\n", action);
    case VISIT_NORMAL:
    case VISIT_COMPLEX_ONLY:
      break;
#endif
    case VISIT_COUNT_BYTES:
      mc_counted_bytes += sizeof (struct callable);
      break;
  }

  if (!(action & VISIT_COMPLEX_ONLY)) {
    visit_type_ref (c->type, REF_TYPE_NORMAL);
    visit_string_ref (c->name, REF_TYPE_NORMAL);
  }

  /* Looks like the c->prog isn't refcounted..? */
  /* visit_program_ref (c->prog, REF_TYPE_NORMAL); */
}

#ifdef PIKE_DEBUG
void present_constant_profiling(void)
{
  struct callable *c;
  for (c = first_callable; c; c = c->next) {
    fprintf(stderr,"%010ld @E@: %s\n",c->runs, c->name->str);
  }
}
#endif

void init_builtin_constants(void)
{
  builtin_constants = allocate_mapping(300);
}

void exit_builtin_constants(void)
{
#ifdef DO_PIKE_CLEANUP
  if(builtin_constants)
  {
    free_mapping(builtin_constants);
    builtin_constants=0;
  }
#endif
}
