/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef BIGNUM_TYPE_BITS_H
#define BIGNUM_TYPE_BITS_H

#include "svalue.h"
#include "object.h"
#include "bignum.h"

/* Like BITOF() / `1 << TYPEOF(*s)`, but also sets BIT_INT for bignum
 * objects.
 */
static inline TYPE_FIELD PIKE_UNUSED_ATTRIBUTE bignum_type_bits(const struct svalue *s)
{
  TYPE_FIELD t = (TYPE_FIELD)(1 << TYPEOF(*s));
  if ((TYPEOF(*s) == PIKE_T_OBJECT) && is_bignum_object(s->u.object))
    /* Lie, and claim that the array contains integers too. */
    t |= BIT_INT;
  return t;
}

#endif /* !BIGNUM_TYPE_BITS_H */
