/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: bignum.c,v 1.41 2004/12/30 13:23:20 grubba Exp $
*/

#include "global.h"

#ifdef AUTO_BIGNUM

#include "interpret.h"
#include "program.h"
#include "object.h"
#include "svalue.h"
#include "pike_error.h"

#define sp Pike_sp

PMOD_EXPORT struct svalue auto_bignum_program = {
  T_INT, 0,
#ifdef HAVE_UNION_INIT
  {0}, /* Only to avoid warnings. */
#endif
};

PMOD_EXPORT struct program *get_auto_bignum_program(void)
{
  return program_from_function(&auto_bignum_program);
}

PMOD_EXPORT struct program *get_auto_bignum_program_or_zero(void)
{
  if (auto_bignum_program.type == T_INT)
    return 0;
  return program_from_function(&auto_bignum_program);
}

void exit_auto_bignum(void)
{
  free_svalue(&auto_bignum_program);
  auto_bignum_program.type=T_INT;
}

PMOD_EXPORT void convert_stack_top_to_bignum(void)
{
  apply_svalue(&auto_bignum_program, 1);

  if(sp[-1].type != T_OBJECT) {
     if (auto_bignum_program.type!=T_PROGRAM)
	Pike_error("Gmp.mpz conversion failed (Gmp.bignum not loaded).\n");
     else
	Pike_error("Gmp.mpz conversion failed (unknown error).\n");
  }
}

PMOD_EXPORT void convert_stack_top_with_base_to_bignum(void)
{
  apply_svalue(&auto_bignum_program, 2);

  if(sp[-1].type != T_OBJECT) {
     if (auto_bignum_program.type!=T_PROGRAM)
	Pike_error("Gmp.mpz conversion failed (Gmp.bignum not loaded).\n");
     else
	Pike_error("Gmp.mpz conversion failed (unknown error).\n");
  }
}

int is_bignum_object(struct object *o)
{
  /* Note:
   * This function should *NOT* try to resolv Gmp.mpz unless
   * it is already loaded into memory.
   * /Hubbe
   */

  if (auto_bignum_program.type == T_INT)
    return 0; /* not possible */
 
  return o->prog == program_from_svalue(&auto_bignum_program);
}

PMOD_EXPORT int is_bignum_object_in_svalue(struct svalue *sv)
{
  /* FIXME: object subtype? */
  return sv->type == T_OBJECT && is_bignum_object(sv->u.object);
}

PMOD_EXPORT struct object *make_bignum_object(void)
{
  convert_stack_top_to_bignum();
  dmalloc_touch_svalue(sp-1);
  return (--sp)->u.object;
}

PMOD_EXPORT struct object *bignum_from_svalue(struct svalue *s)
{
  push_svalue(s);
  convert_stack_top_to_bignum();
  dmalloc_touch_svalue(sp-1);
  return (--sp)->u.object;
}

PMOD_EXPORT struct pike_string *string_from_bignum(struct object *o, int base)
{
  push_int(base);
  safe_apply(o, "digits", 1);
  
  if(sp[-1].type != T_STRING)
    Pike_error("Gmp.mpz string conversion failed.\n");
  
  dmalloc_touch_svalue(sp-1);
  return (--sp)->u.string;
}

PMOD_EXPORT void convert_svalue_to_bignum(struct svalue *s)
{
  push_svalue(s);
  convert_stack_top_to_bignum();
  free_svalue(s);
  *s=sp[-1];
  sp--;
  dmalloc_touch_svalue(sp);
}

#ifdef INT64
static void bootstrap_push_int64 (INT64 i)
{
  if(i == DO_NOT_WARN((INT_TYPE)i))
  {
    push_int(DO_NOT_WARN((INT_TYPE)i));
  }
  else
    Pike_fatal ("Failed to convert large integer (Gmp.bignum not loaded).\n");
}

PMOD_EXPORT void (*push_int64) (INT64) = bootstrap_push_int64;
PMOD_EXPORT int (*int64_from_bignum) (INT64 *, struct object *) = NULL;
PMOD_EXPORT void (*reduce_stack_top_bignum) (void) = NULL;

PMOD_EXPORT void hook_in_int64_funcs (
  void (*push_int64_val)(INT64),
  int (*int64_from_bignum_val) (INT64 *, struct object *),
  void (*reduce_stack_top_bignum_val) (void))
{
  /* Assigning the pointers above directly from the Gmp module doesn't
   * work in some cases, e.g. NT. */
  push_int64 = push_int64_val ? push_int64_val : bootstrap_push_int64;
  int64_from_bignum = int64_from_bignum_val;
  reduce_stack_top_bignum = reduce_stack_top_bignum_val;
}
#endif

#endif /* AUTO_BIGNUM */
