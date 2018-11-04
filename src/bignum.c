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

PMOD_EXPORT void convert_stack_top_to_bignum(void)
{
  push_object(clone_object(bignum_program, 1));
}

PMOD_EXPORT void convert_stack_top_with_base_to_bignum(void)
{
  push_object(clone_object(bignum_program, 2));
}

PMOD_EXPORT int is_bignum_object_in_svalue(struct svalue *sv)
{
  /* FIXME: object subtype? */
  return TYPEOF(*sv) == T_OBJECT && is_bignum_object(sv->u.object);
}

PMOD_EXPORT struct object *make_bignum_object(void)
{
  return clone_object(bignum_program, 1);
}

PMOD_EXPORT struct object *bignum_from_svalue(struct svalue *s)
{
  push_svalue(s);
  return make_bignum_object();
}

PMOD_EXPORT void convert_svalue_to_bignum(struct svalue *s)
{
  push_svalue(s);
  convert_stack_top_to_bignum();
  free_svalue(s);
  *s=Pike_sp[-1];
  Pike_sp--;
  dmalloc_touch_svalue(Pike_sp);
}

PMOD_EXPORT int low_compare_bignums(MP_INT *a, MP_INT *b)
{
  return mpz_cmp(a, b);
}

PMOD_EXPORT int compare_bignums(struct object *a, struct object *b)
{
  if (!is_bignum_object(a)) Pike_error("First object is not a bignum.\n");
  if (!is_bignum_object(b)) Pike_error("Second object is not a bignum.\n");
  return low_compare_bignums((MP_INT *)a->storage, (MP_INT *)b->storage);
}
