/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"

#include "interpret.h"
#include "program.h"
#include "object.h"
#include "svalue.h"
#include "pike_error.h"

#include "bignum.h"

#define sp Pike_sp

PMOD_EXPORT struct svalue auto_bignum_program = SVALUE_INIT_FREE;

PMOD_EXPORT struct program *get_auto_bignum_program(void)
{
  return program_from_svalue(&auto_bignum_program);
}

PMOD_EXPORT void convert_stack_top_to_bignum(void)
{
  apply_svalue(&auto_bignum_program, 1);
}

PMOD_EXPORT void convert_stack_top_with_base_to_bignum(void)
{
  apply_svalue(&auto_bignum_program, 2);
}

int is_bignum_object(struct object *o)
{
  return o->prog == program_from_svalue(&auto_bignum_program);
}

PMOD_EXPORT int is_bignum_object_in_svalue(struct svalue *sv)
{
  /* FIXME: object subtype? */
  return TYPEOF(*sv) == T_OBJECT && is_bignum_object(sv->u.object);
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
  
  if(TYPEOF(sp[-1]) != T_STRING)
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
