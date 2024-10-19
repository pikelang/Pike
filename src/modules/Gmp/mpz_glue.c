/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "module.h"
#include "gmp_machine.h"
#include "pike_float.h"

#if !defined(HAVE_GMP_H)
#error "Gmp is required to build Pike!"
#endif

#include "my_gmp.h"

#include "interpret.h"
#include "pike_macros.h"
#include "program.h"
#include "pike_types.h"
#include "pike_error.h"
#include "builtin_functions.h"
#include "module_support.h"
#include "bignum.h"
#include "operators.h"
#include "gc.h"
#include "constants.h"

#if GMP_NUMB_BITS != SIZEOF_MP_LIMB_T * CHAR_BIT
#error Cannot cope with GMP using nail bits.
#endif

#define sp Pike_sp

#undef THIS
#define DECLARE_THIS() struct pike_frame *_fp = Pike_fp
#define THIS ((MP_INT *)(_fp->current_storage))
#define THIS_PROGRAM (_fp->context->prog)
#define THIS_OBJECT (_fp->current_object)

struct program *mpzmod_program = NULL;
PMOD_EXPORT struct program *bignum_program = NULL;

static mpz_t mpz_int_type_min;

PMOD_EXPORT int mpz_from_svalue(MP_INT *dest, struct svalue *s)
{
  if (!s) return 0;
  if (TYPEOF(*s) == T_INT) {
#if SIZEOF_LONG >= SIZEOF_INT64
    mpz_set_si(dest, s->u.integer);
#else
    INT_TYPE i = s->u.integer;
    int neg = i < 0;
    UINT64 bits = (UINT64) (neg ? -i : i);

    mpz_import(dest, 1, 1, SIZEOF_INT64, 0, 0, &bits);
    if (neg) mpz_neg(dest, dest);
#endif	/* SIZEOF_LONG < SIZEOF_INT64 */
    return 1;
  }

  if ((TYPEOF(*s) != T_OBJECT) || !IS_MPZ_OBJ(s->u.object)) return 0;
  mpz_set(dest, OBTOMPZ(s->u.object));
  return 1;
}

PMOD_EXPORT void push_bignum(MP_INT *mpz)
{
  push_object(fast_clone_object(bignum_program));
  mpz_set(OBTOMPZ(Pike_sp[-1].u.object), mpz);
}

void mpzmod_reduce(struct object *o)
{
  MP_INT *mpz = OBTOMPZ (o);
#if SIZEOF_INT_TYPE == SIZEOF_LONG
  if( mpz_fits_slong_p( mpz ) )
  {
    push_int( mpz_get_si( mpz ) );
    free_object(o);
  }
  else
    push_object(o);
  return;
#else /*int type is not signed long. */
  int neg = mpz_sgn (mpz) < 0;
  INT_TYPE res = 0;

  /* Note: Similar code in int64_from_bignum. */

  /* Get the index of the highest limb that has bits within the range
   * of the INT_TYPE. */
  size_t pos = (INT_TYPE_BITS + GMP_NUMB_BITS - 1) / GMP_NUMB_BITS - 1;

  if (mpz_size (mpz) <= pos + 1) {
    /* NOTE: INT_TYPE is signed, while GMP_NUMB is unsigned.
     *       Thus INT_TYPE_BITS is usually 31 and GMP_NUMB_BITS 32.
     */
#if INT_TYPE_BITS == GMP_NUMB_BITS
    /* NOTE: Overflow is not possible. */
    res = mpz_getlimbn (mpz, 0) & GMP_NUMB_MASK;
#elif INT_TYPE_BITS < GMP_NUMB_BITS
    mp_limb_t val = mpz_getlimbn (mpz, 0) & GMP_NUMB_MASK;
    if (val >= (mp_limb_t) 1 << INT_TYPE_BITS) goto overflow;
    res = val;
#else
    for (;; pos--) {
      res |= mpz_getlimbn (mpz, pos) & GMP_NUMB_MASK;
      if (pos == 0) break;
      if (res >= (INT_TYPE) 1 << (INT_TYPE_BITS - GMP_NUMB_BITS)) goto overflow;
      res <<= GMP_NUMB_BITS;
    }
#endif
    if (neg) res = -res;
    free_object (o);
    push_int (res);
    return;
  }
overflow:
  if (neg && !mpz_cmp (mpz, mpz_int_type_min)) {
    /* No overflow afterall; it's MIN_INT_TYPE, the only valid integer
     * whose absolute value is INT_TYPE_BITS long. */
    free_object (o);
    push_int (MIN_INT_TYPE);
  }
  else
    push_object (o);
#endif /* sizeof int_type == sizeof long */
}

#define PUSH_REDUCED(o) do { struct object *reducetmp__=(o);	\
   if(THIS_PROGRAM == bignum_program)				\
     mpzmod_reduce(reducetmp__);					\
   else								\
     push_object(reducetmp__);					\
}while(0)

PMOD_EXPORT void reduce_stack_top_bignum (void)
{
  struct object *o;
#ifdef PIKE_DEBUG
  if (TYPEOF(sp[-1]) != T_OBJECT || sp[-1].u.object->prog != bignum_program)
    Pike_fatal ("Not a Gmp.bignum.\n");
#endif
  o = (--sp)->u.object;
  debug_malloc_touch (o);
  mpzmod_reduce (o);
}

PMOD_EXPORT void push_int64 (INT64 i)
{
  if(i == (INT_TYPE)i)
  {
    push_int((INT_TYPE)i);
  }
  else
  {
    MP_INT *mpz;

    push_object (fast_clone_object (bignum_program));
    mpz = OBTOMPZ (sp[-1].u.object);

#if SIZEOF_LONG >= SIZEOF_INT64
    mpz_set_si (mpz, i);
#else
    {
      int neg = i < 0;
      UINT64 bits = (UINT64) (neg ? -i : i);

      mpz_import (mpz, 1, 1, SIZEOF_INT64, 0, 0, &bits);
      if (neg) mpz_neg (mpz, mpz);
    }
#endif	/* SIZEOF_LONG < SIZEOF_INT64 */
  }
}

#if SIZEOF_INT64 != SIZEOF_LONG || SIZEOF_INT_TYPE != SIZEOF_LONG 
static mpz_t mpz_int64_min;
#endif

/**
 * Set i to the lowest 64bits of the value and return non-zero on success.
 *
 * Similar to int64_from_bignum(), but does not perform any clamping.
 */
PMOD_EXPORT int low_int64_from_bignum(INT64 *i, struct object *bignum)
{
  INT64 res = 0;
  MP_INT *mpz = OBTOMPZ(bignum);

  if (!IS_MPZ_OBJ(bignum)) return 0;
#if GMP_LIMB_BITS >= 64
  res = (INT64)mpz_getlimbn(mpz, 0);
#else
  {
    size_t pos, bits = 0;
    for (pos = 0, bits = 0; bits < 64; pos++, bits += GMP_LIMB_BITS) {
      res |= ((INT64)mpz_getlimbn(mpz, pos)) << bits;
    }
  }
#endif
  if (mpz_sgn(mpz) < 0) {
    res = -res;
  }
  *i = res;
  return 1;
}

/**
 * Convert a bignum to an INT64.
 *
 * Returns nonzero and sets i on success.
 *
 * Returns zero on failoure.
 *
 * Failure causes include:
 *
 *   o bignum not being a Gmp.mpz or Gmp.bignum object.
 *
 *   o The value being larger than 64 bits, in which case
 *     it is clamped to {MAX,MIN}_INT64
 */
PMOD_EXPORT int int64_from_bignum (INT64 *i, struct object *bignum)
{
  MP_INT *mpz = OBTOMPZ(bignum);
  int neg;
  size_t pos;

  if (!IS_MPZ_OBJ(bignum)) return 0;
#if SIZEOF_INT64 == SIZEOF_LONG
  if( !mpz_fits_slong_p( mpz ) )
    return 0;
  *i = mpz_get_si( mpz );
  return 1;
#else /*int64 is not signed long. */

  neg = mpz_sgn (mpz) < 0;

  /* Note: Similar code in mpzmod_reduce */

  /* Get the index of the highest limb that have bits within the range
   * of the INT64. */
  pos = (INT64_BITS + GMP_NUMB_BITS - 1) / GMP_NUMB_BITS - 1;

#ifdef PIKE_DEBUG
  if ((bignum->prog != bignum_program) &&
      (bignum->prog != mpzmod_program)) {
    Pike_fatal("cast(): Not a Gmp.bignum or Gmp.mpz.\n");
  }
#endif

  if (mpz_size (mpz) <= pos + 1) {
    INT64 res;
#if INT64_BITS == GMP_NUMB_BITS
    res = mpz_getlimbn (mpz, 0) & GMP_NUMB_MASK;
#elif INT64_BITS < GMP_NUMB_BITS
    mp_limb_t val = mpz_getlimbn (mpz, 0) & GMP_NUMB_MASK;
    if (val >= (mp_limb_t) 1 << INT64_BITS) goto overflow;
    res = (INT64) val;
#else
    res = 0;
    for (;; pos--) {
      res |= mpz_getlimbn (mpz, pos) & GMP_NUMB_MASK;
      if (pos == 0) break;
      if (res >= (INT64) 1 << (INT64_BITS - GMP_NUMB_BITS)) goto overflow;
      res <<= GMP_NUMB_BITS;
    }
#endif

    if (neg) res = -res;
    *i = res;
    return 1;
  }

overflow:
  if (neg && !mpz_cmp (mpz, mpz_int64_min)) {
    /* No overflow afterall; it's MIN_INT64, the only valid integer
     * whose absolute value is INT64_BITS long. */
    *i = MIN_INT64;
    return 1;
  }
  *i = neg ? MIN_INT64 : MAX_INT64;
#endif /*int64 is not signed long. */
  return 0;
}

PMOD_EXPORT void push_ulongest (UINT64 i)
{
  if (i <= MAX_INT_TYPE) {
    push_int((INT_TYPE)i);
  }
  else {
    MP_INT *mpz;

    push_object (fast_clone_object (bignum_program));
    mpz = OBTOMPZ (sp[-1].u.object);

#if SIZEOF_LONG >= SIZEOF_INT64
    mpz_set_ui (mpz, i);
#else
    mpz_import (mpz, 1, 1, SIZEOF_INT64, 0, 0, &i);
#endif	/* SIZEOF_LONG < SIZEOF_INT64 */
  }
}

PMOD_EXPORT void ulongest_to_svalue_no_free(struct svalue *sv, UINT64 i)
{
  if (i <= MAX_INT_TYPE) {
    SET_SVAL(*sv, PIKE_T_INT, NUMBER_NUMBER, integer, (INT_TYPE)i);
  }
  else {
    MP_INT *mpz;

    SET_SVAL(*sv, PIKE_T_OBJECT, 0, object, fast_clone_object(bignum_program));
    mpz = OBTOMPZ(sv->u.object);
#if SIZEOF_LONG >= SIZEOF_INT64
    mpz_set_ui (mpz, i);
#else
    mpz_import (mpz, 1, 1, SIZEOF_INT64, 0, 0, &i);
#endif	/* SIZEOF_LONG < SIZEOF_INT64 */
  }
}

/*! @module Gmp
 *! GMP is a free library for arbitrary precision arithmetic,
 *! operating on signed integers, rational numbers, and floating point
 *! numbers. There is no practical limit to the precision except the
 *! ones implied by the available memory in the machine GMP runs on.
 *! @url{http://www.swox.com/gmp/@}
 */

/*! @class bignum
 *! This program is used by the internal auto-bignum conversion. It
 *! can be used to explicitly type integers that are too big to be
 *! INT_TYPE. Best is however to not use this program unless you
 *! really know what you are doing.
 *!
 *! Due to the auto-bignum conversion, all integers can be treated as
 *! @[Gmp.mpz] objects insofar as that they can be indexed with the
 *! functions in the @[Gmp.mpz] class. For instance, to calculate the
 *! greatest common divisor between @expr{51@} and @expr{85@}, you can
 *! do @expr{51->gcd(85)@}. In other words, all the functions in
 *! @[Gmp.mpz] are also available here.
 *! @endclass
 */

/*! @class mpz
 *! Gmp.mpz implements very large integers. In fact,
 *! the only limitation on these integers is the available memory.
 *! The mpz object implements all the normal integer operations.
 *!
 *! Note that the auto-bignum feature also makes these operations
 *! available "in" normal integers. For instance, to calculate the
 *! greatest common divisor between @expr{51@} and @expr{85@}, you can
 *! do @expr{51->gcd(85)@}.
 */

void get_mpz_from_digits(MP_INT *tmp,
			 struct pike_string *digits,
			 int base)
{
  if (digits->size_shift)
    Pike_error("Invalid digits, cannot convert to Gmp.mpz.\n");

  if(!base || ((base >= 2) && (base <= 62)))
  {
    int offset = 0;
    int neg = 0;

    if(digits->len > 1)
    {
      if(STR0(digits)[0] == '+')
	offset += 1;
      else if(STR0(digits)[0] == '-')
      {
	offset += 1;
	neg = 1;
      }

#ifndef HAVE_GMP5
      /* mpz_set_str() will parse leading "0x" and "0X" as hex and
       * leading "0" as octal. "0b" and "0B" for binary are supported
       * from GMP 5.0.
       */
      if(!base && digits->len > 2)
      {
	if((STR0(digits)[offset] == '0') &&
	   ((STR0(digits)[offset+1] == 'b') ||
	    (STR0(digits)[offset+1] == 'B')))
	{
	  offset += 2;
	  base = 2;
	}
      }
#endif
    }

    if (mpz_set_str(tmp, digits->str + offset, base))
      Pike_error("Invalid digits, cannot convert to Gmp.mpz.\n");

    if(neg)
      mpz_neg(tmp, tmp);
  }
  else if(base == 256)
  {
    mpz_import (tmp, digits->len, 1, 1, 0, 0, digits->str);
  }
  else if(base == -256)
  {
    mpz_import (tmp, digits->len, -1, 1, 0, 0, digits->str);
  }
  else
  {
    Pike_error("Invalid base.\n");
  }
}

int get_new_mpz(MP_INT *tmp, struct svalue *s,
		int throw_error, const char *arg_func, int arg, int args)
{
  switch(TYPEOF(*s))
  {
  case T_INT:
#ifndef BIG_PIKE_INT
    mpz_set_si(tmp, (signed long int) s->u.integer);
#else
    {
      INT_TYPE i = s->u.integer;
      int neg = i < 0;

      if (neg) i = -i;
      mpz_import (tmp, 1, 1, SIZEOF_INT_TYPE, 0, 0, &i);
      if (neg) mpz_neg (tmp, tmp);
    }
#endif
    break;

  case T_FLOAT:
    {
      double val = (double)s->u.float_number;
      if (PIKE_ISNAN(val) || PIKE_ISINF(val)) return 0;
      mpz_set_d(tmp, val);
    }
    break;

  case T_OBJECT:
    if(IS_MPZ_OBJ (s->u.object)) {
      mpz_set(tmp, OBTOMPZ(s->u.object));
      break;
    }

    if(s->u.object->prog == mpf_program)
    {
      mpz_set_f(tmp, OBTOMPF(s->u.object));
      break;
    }

    if(s->u.object->prog == mpq_program)
    {
      mpz_set_q(tmp, OBTOMPQ(s->u.object));
      break;
    }

    if (s->u.object->prog) {
      if (throw_error)
	SIMPLE_ARG_TYPE_ERROR (arg_func, arg, "int|float|Gmp.mpz|Gmp.mpf|Gmp.mpq");
      else
	return 0;
    } else {
      /* Destructed object. Use as zero. */
      mpz_set_si(tmp, 0);
    }
    break;
#if 0
  case T_STRING:
    mpz_set_str(tmp, s->u.string->str, 0);
    break;

  case T_ARRAY:   /* Experimental */
    if ( (s->u.array->size != 2)
	 || (TYPEOF(ITEM(s->u.array)[0]) != T_STRING)
	 || (TYPEOF(ITEM(s->u.array)[1]) != T_INT))
      Pike_error("Cannot convert array to Gmp.mpz.\n");
    get_mpz_from_digits(tmp, ITEM(s->u.array)[0].u.string,
			ITEM(s->u.array)[1]);
    break;
#endif

  default:
    if (throw_error)
      SIMPLE_ARG_TYPE_ERROR (arg_func, arg, "int|float|Gmp.mpz|Gmp.mpf|Gmp.mpq");
    else
      return 0;
  }

  return 1;
}

PMOD_EXPORT struct object *create_double_bignum( INT_TYPE low, INT_TYPE high )
{
  struct object *res = fast_clone_object( bignum_program );
  MP_INT *mpz = (MP_INT*)res->storage;
  INT_TYPE data[2];
  if( UNLIKELY(high < 0) )
  {
    data[0] = -low;
    data[1] = -high;
    mpz_import( mpz, 2, -1, sizeof(INT_TYPE), 0, 0, data );
    mpz_neg( mpz,mpz );
  }
  else
  {
    data[0] = low;
    data[1] = high;
    mpz_import( mpz, 2, -1, sizeof(INT_TYPE), 0, 0, data );
  }
  return res;
}

/* Converts an svalue, located on the stack, to an mpz object */
MP_INT *debug_get_mpz(struct svalue *s,
		      int throw_error, const char *arg_func, int arg, int args)
{
  struct object *o = fast_clone_object (mpzmod_program);
  ONERROR uwp;
  SET_ONERROR (uwp, do_free_object, o);
  if (get_new_mpz (OBTOMPZ (o), s, throw_error, arg_func, arg, args)) {
    UNSET_ONERROR (uwp);
    free_svalue(s);
    SET_SVAL(*s, T_OBJECT, 0, object, o);
    return OBTOMPZ (o);
  }
  else {
    UNSET_ONERROR (uwp);
    free_object (o);
    return NULL;
  }
}

/*! @decl void create()
 *! @decl void create(string|int|float|object value)
 *! @decl void create(string value, @
 *!                   int(2..62)|int(256..256)|int(-256..-256) base)
 *!
 *!   Create and initialize a @[Gmp.mpz] object.
 *!
 *! @param value
 *!   Initial value. If no value is specified, the object will be initialized
 *!   to zero.
 *!
 *! @param base
 *!   Base the value is specified in. The default base is base 10. The
 *!   base can be either a value in the range @tt{[2..36]@}
 *!   (inclusive), in which case the numbers are taken from the ASCII
 *!   range @tt{0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ@}
 *!   (case-insensitive), in the range @tt{[37..62]@} (inclusive), in
 *!   which case the ASCII range will be case sensitive, or either of
 *!   the values @expr{256@} or @expr{-256@}, in which case @[value]
 *!   is taken to be the unsigned binary representation in network
 *!   byte order or reversed byte order respectively.
 *!
 *!   Values in base @tt{[2..62]@} can be prefixed with @expr{"+"@} or
 *!   @expr{"-"@}. If no base is given, values prefixed with
 *!   @expr{"0b"@} or @expr{"0B"@} will be interpreted as binary.
 *!   Values prefixed with @expr{"0x"@} or @expr{"0X"@} will be
 *!   interpreted as hexadecimal. Values prefixed with @expr{"0"@}
 *!   will be interpreted as octal.
 *!
 *! @note
 *!   Leading zeroes in @[value] are not significant when a base is
 *!   explicitly given. In particular leading NUL characters are not
 *!   preserved in the base 256 modes.
 *!
 *!   Before GMP 5.0 only bases 2-36 and 256 were supported.
 */
static void mpzmod_create(INT32 args)
{
  DECLARE_THIS();
  switch(args)
  {
  case 1:
    if(TYPEOF(sp[-args]) == T_STRING) {
      if (sp[-args].u.string->flags & STRING_CLEAR_ON_EXIT) {
	THIS_OBJECT->flags |= OBJECT_CLEAR_ON_EXIT;
      }
      get_mpz_from_digits(THIS, sp[-args].u.string, 0);
    } else {
      if ((TYPEOF(sp[-args]) == T_OBJECT) &&
	  (sp[-args].u.object->flags & OBJECT_CLEAR_ON_EXIT)) {
	THIS_OBJECT->flags |= OBJECT_CLEAR_ON_EXIT;
      }
      get_new_mpz(THIS, sp-args, 1, "Gmp.mpz", 1, args);
    }
    break;

  case 2: /* Args are string of digits and integer base */
    if(TYPEOF(sp[-args]) != T_STRING)
      SIMPLE_ARG_TYPE_ERROR ("create", 1, "string");

    if (TYPEOF(sp[1-args]) != T_INT)
      SIMPLE_ARG_TYPE_ERROR ("create", 2, "int");

    if (sp[-args].u.string->flags & STRING_CLEAR_ON_EXIT) {
      THIS_OBJECT->flags |= OBJECT_CLEAR_ON_EXIT;
    }

    get_mpz_from_digits(THIS, sp[-args].u.string, sp[1-args].u.integer);
    break;

  case 0:
    break;	/* Needed by AIX cc */
  }
  pop_n_elems(args);
}

/*! @decl int __hash()
 *!
 *!   Calculate a hash of the value.
 *!
 *! @note
 *!   Prior to Pike 7.8.359 this function returned the low
 *!   32-bits as an unsigned integer. This could in some
 *!   common cases lead to very unbalanced mappings.
 *!
 *! @seealso
 *!   @[hash_value()]
 */
static void mpzmod___hash(INT32 args)
{
  DECLARE_THIS();
  MP_INT *mpz = THIS;
  size_t len = mpz_size(mpz) * sizeof(mp_limb_t);
  size_t h = hashmem(mpz->_mp_d, len, len);

  pop_n_elems(args);
  if (mpz_sgn(mpz) < 0)
    push_int((INT_TYPE)~h);
  else
    push_int((INT_TYPE)h);
  return;
}

struct pike_string *low_get_mpz_digits(MP_INT *mpz, int base)
{
  struct pike_string *s = 0;   /* Make gcc happy. */
  ptrdiff_t len;

  if (
#ifdef HAVE_GMP5
      (base >= 2) && (base <= 62)
      /* -2..-36 is also available for upper case conversions, but
          it's just confusing together with the -256 notation, so
          let's leave that out of the Pike API. */
#else
      (base >= 2) && (base <= 36)
#endif
      )
  {
    len = mpz_sizeinbase(mpz, base) + 2;
    s = begin_shared_string(len);
    mpz_get_str(s->str, base, mpz);
    /* Find NULL character */
    len-=4;
    if (len < 0) len = 0;
    while(s->str[len]) len++;
    s=end_and_resize_shared_string(s, len);
  }
  else if ((base == 256) || (base == -256))
  {
    if (mpz_sgn(mpz) < 0)
      Pike_error("Only non-negative numbers can be converted to base 256.\n");

    len = (mpz_sizeinbase(mpz, 2) + 7) / 8;
    s = begin_shared_string(len);

    if (!mpz_size (mpz))
    {
      /* Zero is a special case. There are no limbs at all, but
       * the size is still 1 bit, and one digit should be produced. */
#ifdef PIKE_DEBUG
      if (len != 1)
	Pike_fatal("mpz->low_get_mpz_digits: strange mpz state!\n");
#endif
      s->str[0] = 0;
    } else if (base < 0) {
      mpz_export(s->str, NULL, -1, 1, 0, 0, mpz);
    } else {
      mpz_export(s->str, NULL, 1, 1, 0, 0, mpz);
    }
    s = end_shared_string(s);
  }
  else
  {
    Pike_error("Invalid base.\n");
    UNREACHABLE();
  }

  return s;
}

/*! @decl string encode_json()
 *  Casts the object to a string.
 */
static void mpzmod_encode_json(INT32 args)
{
  DECLARE_THIS();
  pop_n_elems(args);
  push_string(low_get_mpz_digits(THIS, 10));
}

/*! @decl string digits(void|int(2..62)|int(256..256)|int(-256..-256) base)
 *!
 *! Convert this mpz object to a string. If a @[base] is given the
 *! number will be represented in that base. Valid bases are 2-62 and
 *! @expr{256@} and @expr{-256@}. The default base is 10.
 *!
 *! @note
 *!   The bases 37 to 62 are not available when compiled with GMP
 *!   earlier than version 5.
 *! @seealso
 *!   @[cast]
 */
static void mpzmod_digits(INT32 args)
{
  DECLARE_THIS();
  INT32 base;
  struct pike_string *s;

  if (!args)
  {
    base = 10;
  }
  else
  {
    if (TYPEOF(sp[-args]) != T_INT)
      SIMPLE_ARG_TYPE_ERROR ("digits", 1, "int");
    base = sp[-args].u.integer;
  }

  s = low_get_mpz_digits(THIS, base);
  pop_n_elems(args);

  push_string(s);
}

/*! @decl string _sprintf(int ind, mapping opt)
 */
static void mpzmod__sprintf(INT32 args)
{
  DECLARE_THIS();
  INT_TYPE precision, width, width_undecided, base = 0, mask_shift = 0;
  struct pike_string *s = 0;
  INT_TYPE flag_left, method;

  debug_malloc_touch(THIS_OBJECT);

  if(args < 2)
    SIMPLE_WRONG_NUM_ARGS_ERROR("_sprintf", 2);
  if(TYPEOF(sp[-args]) != T_INT)
    SIMPLE_ARG_TYPE_ERROR ("_sprintf", 1, "int");
  if(TYPEOF(sp[1-args]) != T_MAPPING)
    SIMPLE_ARG_TYPE_ERROR ("_sprintf", 2, "mapping");

  push_svalue(&sp[1-args]);
  push_static_text("precision");
  f_index(2);
  if(TYPEOF(sp[-1]) != T_INT)
    SIMPLE_ARG_ERROR ("_sprintf", 2,
		      "The field \"precision\" doesn't hold an integer.");
  precision = (--sp)->u.integer;

  push_svalue(&sp[1-args]);
  push_static_text("width");
  f_index(2);
  if(TYPEOF(sp[-1]) != T_INT)
    SIMPLE_ARG_ERROR ("_sprintf", 2,
		      "The field \"width\" doesn't hold an integer.");
  width_undecided = (SUBTYPEOF(sp[-1]) != NUMBER_NUMBER);
  width = (--sp)->u.integer;

  push_svalue(&sp[1-args]);
  push_static_text("flag_left");
  f_index(2);
  if(TYPEOF(sp[-1]) != T_INT)
    SIMPLE_ARG_ERROR ("_sprintf", 2,
		      "The field \"flag_left\" doesn't hold an integer.");
  flag_left=sp[-1].u.integer;
  pop_stack();

  debug_malloc_touch(THIS_OBJECT);

  switch(method = sp[-args].u.integer)
  {
  case 't':
    pop_n_elems(args);
    if(THIS_PROGRAM == bignum_program)
      push_static_text("int");
    else
      push_static_text("object");
    return;

  case 'O':
    if (THIS_PROGRAM == mpzmod_program) {
      push_static_text ("Gmp.mpz(");
      push_string (low_get_mpz_digits (THIS, 10));
      push_static_text (")");
      f_add (3);
      s = (--sp)->u.string;
      break;
    }
    /* Fall through */

  case 'u': /* Note: 'u' is not really supported. */
  case 'd':
    s = low_get_mpz_digits(THIS, 10);
    break;

  case 'x':
  case 'X':
    base += 8;
    mask_shift += 1;
    /* Fall-through. */
  case 'o':
    base += 6;
    mask_shift += 2;
    /* Fall-through. */
  case 'b':
    base += 2;
    mask_shift += 1;

    if(precision > 0)
    {
      mpz_t mask;

      mpz_init_set_ui(mask, 1);
      mpz_mul_2exp(mask, mask, precision * mask_shift);
      mpz_sub_ui(mask, mask, 1);
      mpz_and(mask, mask, THIS);
      s = low_get_mpz_digits(mask, base);
      mpz_clear(mask);
    }
    else
      s = low_get_mpz_digits(THIS, base);
    break;

  case 'c':
    {
      INT_TYPE neg = mpz_sgn (THIS) < 0;
      unsigned char *dst;
      size_t pos, length = mpz_size (THIS);
      mpz_t tmp;
      MP_INT *n;
      INT_TYPE i;

      if(width_undecided)
      {
        p_wchar2 ch = mpz_get_ui(THIS);
        if(neg)
          ch = (~ch)+1;
        s = make_shared_binary_string2(&ch, 1);
        break;
      }

      if (neg)
      {
        mpz_init_set(tmp, THIS);
        mpz_add_ui(tmp, tmp, 1);
        length = mpz_size (tmp);
        n = tmp;
      }
      else
        n = THIS;

      if(width < 1)
        width = 1;

      s = begin_shared_string(width);

      if (!flag_left)
        dst = (unsigned char *)STR0(s) + width;
      else
        dst = (unsigned char *)STR0(s);

      pos = 0;
      while(width > 0)
      {
        mp_limb_t x = (length-->0? mpz_getlimbn(n, pos++) : 0);

        if (!flag_left)
          for(i = 0; i < (INT_TYPE)sizeof(mp_limb_t); i++)
          {
            *(--dst) = (unsigned char)((neg ? ~x : x) & 0xff);
	    x >>= 8;
	    if(!--width)
              break;
          }
        else
          for(i = 0; i < (INT_TYPE)sizeof(mp_limb_t); i++)
          {
            *(dst++) = (unsigned char)((neg ? ~x : x) & 0xff);
	    x >>= 8;
	    if(!--width)
              break;
          }
      }

      if(neg)
        mpz_clear(tmp);

      s = end_shared_string(s);
    }
    break;
  }

  debug_malloc_touch(THIS_OBJECT);

  pop_n_elems(args);

  if(s) {
    push_string(s);
    if (method == 'X')
      f_upper_case(1);
  }
  else
    push_undefined();
}

/* protected int(0..1) _is_type(string type)
 */
static void mpzmod__is_type(INT32 UNUSED(args))
{
    int is_int;
    is_int = Pike_sp[-1].u.string == literal_int_string ? 1 : 0;
    pop_stack();
    push_int( is_int );
}

/*! @decl int(0..) size(void|int base)
 *!
 *! Return how long this mpz would be represented in the specified
 *! @[base]. The default base is 2.
 */
static void mpzmod_size(INT32 args)
{
  DECLARE_THIS();
  int base;
  if (!args)
  {
    /* Default is number of bits */
    base = 2;
  }
  else
  {
    if (TYPEOF(sp[-args]) != T_INT)
      SIMPLE_ARG_TYPE_ERROR ("size", 1, "int");
    base = sp[-args].u.integer;
    if ((base != 256) && (base != -256) && ((base < 2) || (base > 62)))
      SIMPLE_ARG_ERROR ("size", 1, "Invalid base.");
  }
  pop_n_elems(args);

  if (base == 256 || base == -256)
    push_int((mpz_sizeinbase(THIS, 2) + 7) / 8);
  else
    push_int(mpz_sizeinbase(THIS, base));
}

/*! @decl string|int|float cast(string type)
 *!
 *! Cast this mpz object to another type. Allowed types are string,
 *! int and float.
 */
static void mpzmod_cast(INT32 args)
{
  DECLARE_THIS();
  struct pike_string *s = sp[-args].u.string;
  if( args ) pop_stack(); /* s have at least one more reference. */

  if( s == literal_int_string )
  {
    add_ref(THIS_OBJECT);
    mpzmod_reduce(THIS_OBJECT);
    if( TYPEOF(Pike_sp[-1]) == T_OBJECT &&
        Pike_sp[-1].u.object->prog != bignum_program )
      {
        push_object(clone_object(bignum_program, 1));
      }
    return;
  }
  else if( s == literal_string_string )
    push_string(low_get_mpz_digits(THIS, 10));
  else if( s == literal_float_string )
    push_float((FLOAT_TYPE)mpz_get_d(THIS));
  else
    push_undefined();
}

double double_from_sval(struct svalue *s)
{
  switch(TYPEOF(*s))
  {
    case T_INT: return (double)s->u.integer;
    case T_FLOAT: return (double)s->u.float_number;
    case T_OBJECT:
      if(IS_MPZ_OBJ (s->u.object))
	return mpz_get_d(OBTOMPZ(s->u.object));
      /* Fallthrough */
    default:
      Pike_error("Bad argument, expected a number of some sort.\n");
  }
  UNREACHABLE();
}

#define BINFUN2(name, errmsg_op, fun, OP, f_op, LFUN)			\
static void name(INT32 args)						\
{									\
  DECLARE_THIS();                                                       \
  INT32 e;								\
  struct object *res;							\
  if(THIS_PROGRAM == bignum_program)					\
  {									\
    double ret;								\
    for(e=0; e<args; e++)						\
    {									\
      switch(TYPEOF(sp[e-args]))					\
      {									\
      case T_OBJECT:							\
	{								\
	  struct object *o = sp[e-args].u.object;			\
	  struct program *p = NULL;					\
	  int fun = -1;							\
	  if (o->prog &&						\
	      ((p = o->prog->inherits[SUBTYPEOF(sp[e-args])].prog) !=	\
	       bignum_program) &&					\
	      ((fun = FIND_LFUN(p, PIKE_CONCAT(LFUN_R, LFUN))) !=	\
	       -1)) {							\
	    /* Found non-bignum program with double back operator. */	\
	    memmove(Pike_sp+1-args, Pike_sp-args,			\
		    args * sizeof(struct svalue));			\
	    Pike_sp++;							\
	    args++;							\
	    e++;							\
	    SET_SVAL(Pike_sp[-args], T_OBJECT, 0 /* FIXME? */, object,	\
		     THIS_OBJECT);                                      \
	    add_ref(THIS_OBJECT);                                       \
	    args = low_rop(o, fun, e, args);				\
	    if (args > 1) {						\
	      f_op(args);						\
	    }								\
	    return;							\
	  }								\
	}								\
	break;								\
       case T_FLOAT:                                                    \
	ret=mpz_get_d(THIS);						\
	for(e=0; e<args; e++)						\
	  ret = ret OP double_from_sval(sp-args);		        \
									\
	pop_n_elems(args);						\
	push_float( (FLOAT_TYPE)ret );					\
	return;								\
 STRINGCONV(                                                            \
       case T_STRING:                                                   \
        memmove(sp-args+1, sp-args, sizeof(struct svalue)*args);        \
        sp++; args++;                                                   \
        SET_SVAL_TYPE(sp[-args], PIKE_T_FREE);				\
        SET_SVAL(sp[-args], T_STRING, 0, string,			\
		 low_get_mpz_digits(THIS, 10));				\
        f_add(args);                                                    \
	return; )							\
      }									\
    }									\
  }									\
  for(e=0; e<args; e++)							\
    if(TYPEOF(sp[e-args]) != T_INT || !FITS_ULONG (sp[e-args].u.integer)) \
      get_mpz(sp+e-args, 1, "`" errmsg_op, e + 1, args);	\
  res = fast_clone_object(THIS_PROGRAM);				\
  mpz_set(OBTOMPZ(res), THIS);						\
  for(e=0;e<args;e++)							\
    if(TYPEOF(sp[e-args]) != T_INT)					\
      fun(OBTOMPZ(res), OBTOMPZ(res), OBTOMPZ(sp[e-args].u.object));	\
    else								\
      PIKE_CONCAT(fun,_ui)(OBTOMPZ(res), OBTOMPZ(res),			\
                           sp[e-args].u.integer);			\
									\
  pop_n_elems(args);                                                    \
  PUSH_REDUCED(res);                                                    \
}									\
                                                                        \
STRINGCONV(                                                             \
static void PIKE_CONCAT(name,_rhs)(INT32 args)				\
{									\
  DECLARE_THIS();                                                       \
  INT32 e;								\
  struct object *res;							\
  if(THIS_PROGRAM == bignum_program)					\
  {									\
    double ret;								\
    for(e=0; e<args; e++)						\
    {									\
      switch(TYPEOF(sp[e-args]))					\
      {									\
       case T_FLOAT:                                                    \
	ret=mpz_get_d(THIS);						\
	for(e=0; e<args; e++)						\
	  ret = ret OP double_from_sval(sp-args);		        \
									\
	pop_n_elems(args);						\
	push_float( (FLOAT_TYPE)ret );					\
	return;								\
       case T_STRING:                                                   \
        push_string(low_get_mpz_digits(THIS, 10));                      \
        f_add(args+1);                                                  \
	return; 							\
      }									\
    }									\
  }									\
  for(e=0; e<args; e++)							\
    if(TYPEOF(sp[e-args]) != T_INT || !FITS_ULONG (sp[e-args].u.integer)) \
      get_mpz(sp+e-args, 1, "``" errmsg_op, e + 1, args);	\
  res = fast_clone_object(THIS_PROGRAM);				\
  mpz_set(OBTOMPZ(res), THIS);						\
  for(e=0;e<args;e++)							\
    if(TYPEOF(sp[e-args]) != T_INT)					\
      fun(OBTOMPZ(res), OBTOMPZ(res), OBTOMPZ(sp[e-args].u.object));	\
    else								\
      PIKE_CONCAT(fun,_ui)(OBTOMPZ(res), OBTOMPZ(res),			\
                           sp[e-args].u.integer);			\
									\
  pop_n_elems(args);							\
  PUSH_REDUCED(res);							\
}									\
)

#define STRINGCONV(X) X

static void mpzmod_add_eq(INT32 args)
{
  DECLARE_THIS();
  if(args!=1)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("`+=", 1);

  if(THIS_PROGRAM == bignum_program)
  {
    double ret;
    switch(TYPEOF(sp[-1]))
    {
    case T_FLOAT:
      ret=mpz_get_d(THIS) + double_from_sval(sp-1);
      pop_stack();
      push_float( (FLOAT_TYPE)ret );
      return;
    case T_STRING:
      memmove(sp-2, sp-args, sizeof(struct svalue));
      sp++;
      SET_SVAL_TYPE(sp[-2], PIKE_T_FREE);
      SET_SVAL(sp[-2], T_STRING, 0, string,
               low_get_mpz_digits(THIS, 10));
      f_add(2);
      return;
    }
  }
  if(TYPEOF(sp[-1]) != T_INT || !FITS_ULONG (sp[-1].u.integer))
    get_mpz(sp-1, 1, "`+", 1, 1);

  if(TYPEOF(sp[-1]) != T_INT)
    mpz_add(THIS, THIS, OBTOMPZ(sp[-1].u.object));
  else
    mpz_add_ui(THIS,THIS, sp[-1].u.integer);
  add_ref(THIS_OBJECT);
  PUSH_REDUCED(THIS_OBJECT);
}

/*! @decl Gmp.mpz `+(int|float|Gmp.mpz x)
 */
/*! @decl Gmp.mpz ``+(int|float|Gmp.mpz x)
 */
BINFUN2(mpzmod_add, "+", mpz_add, +, f_add, ADD)

#undef STRINGCONV
#define STRINGCONV(X)

/*! @decl Gmp.mpz `*(int|float|Gmp.mpz x)
 */
/*! @decl Gmp.mpz ``*(int|float|Gmp.mpz x)
 */
BINFUN2(mpzmod_mul, "*", mpz_mul, *, f_multiply, MULTIPLY)

/*! @decl Gmp.mpz gcd(object|int|float|string ... args)
 *!
 *! Return the greatest common divisor between this mpz object and
 *! all the arguments.
 */
static void mpzmod_gcd(INT32 args)
{
  DECLARE_THIS();
  INT32 e;
  struct object *res;
  for(e=0; e<args; e++)
    if(TYPEOF(sp[e-args]) != T_INT || sp[e-args].u.integer<=0)
      get_mpz(sp+e-args, 1, "gcd", e + 1, args);
  res = fast_clone_object(THIS_PROGRAM);
  mpz_set(OBTOMPZ(res), THIS);
  for(e=0;e<args;e++)
    if(TYPEOF(sp[e-args]) != T_INT)
      mpz_gcd(OBTOMPZ(res), OBTOMPZ(res), OBTOMPZ(sp[e-args].u.object));
    else
      mpz_gcd_ui(OBTOMPZ(res), OBTOMPZ(res),sp[e-args].u.integer);

  pop_n_elems(args);
  PUSH_REDUCED(res);
}

/*! @decl Gmp.mpz `-(int|float|Gmp.mpz x)
 */
static void mpzmod_sub(INT32 args)
{
  DECLARE_THIS();
  INT32 e;
  struct object *res;

  if (args)
    for (e = 0; e<args; e++)
      get_mpz(sp + e - args, 1, "`-", e + 1, args);

  res = fast_clone_object(THIS_PROGRAM);
  mpz_set(OBTOMPZ(res), THIS);

  if(args)
  {
    for(e=0;e<args;e++)
      mpz_sub(OBTOMPZ(res), OBTOMPZ(res), OBTOMPZ(sp[e-args].u.object));
  }else{
    mpz_neg(OBTOMPZ(res), OBTOMPZ(res));
  }
  pop_n_elems(args);
  debug_malloc_touch(res);
  PUSH_REDUCED(res);
}

/*! @decl Gmp.mpz ``-(int|float|Gmp.mpz x)
 */
static void mpzmod_rsub(INT32 args)
{
  DECLARE_THIS();
  struct object *res = NULL;

  if(args!=1)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("``-", 1);

  res = fast_clone_object(THIS_PROGRAM);

  mpz_sub(OBTOMPZ(res), get_mpz(sp-1, 1, "``-", 1, 1), THIS);
  pop_n_elems(args);
  PUSH_REDUCED(res);
}

/*! @decl Gmp.mpz `/(int|float|Gmp.mpz x)
 */
static void mpzmod_div(INT32 args)
{
  DECLARE_THIS();
  struct object *res;

  if(args!=1)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("`/", 1);

  res = fast_clone_object(THIS_PROGRAM);
  mpz_set(OBTOMPZ(res), THIS);

  switch(TYPEOF(sp[-1]))
  {
  case T_FLOAT:
    {
      double me = mpz_get_d( OBTOMPZ(res) );
      push_float( me / sp[-1].u.float_number );
      free_object(res);
      return;
    }
  case T_INT:
    {
      INT_TYPE i = sp[-1].u.integer;
      int negate = 0;

      if(!i)
      {
        free_object(res);
        SIMPLE_DIVISION_BY_ZERO_ERROR ("`/");
      }

      if (i < 0) {
        i = -i;
        negate = 1;
      }
#ifdef BIG_PIKE_INT
      if ( (unsigned long int)i == (unsigned INT_TYPE)i)
      {
        mpz_fdiv_q_ui(OBTOMPZ(res), OBTOMPZ(res), (unsigned long int)i);
      }
      else
      {
        MP_INT *tmp=get_mpz(sp-1,1,"`/",e,e);
        mpz_fdiv_q(OBTOMPZ(res), OBTOMPZ(res), tmp);
        /* will this leak? there is no simple way of poking at the
           references to tmp */
      }
#else
      mpz_fdiv_q_ui(OBTOMPZ(res), OBTOMPZ(res), i);
#endif
      if (negate) {
        mpz_neg(OBTOMPZ(res), OBTOMPZ(res));
      }
    }
    break;

  case T_OBJECT:
    if(sp[-1].u.object->prog == mpf_program ||
       sp[-1].u.object->prog == mpq_program )
    {
      /* Use rdiv in other object.
         Then continue div in result.*/
      push_object( res ); /* Gives ref to rdiv. */
      apply_lfun( sp[-2].u.object, LFUN_RDIVIDE, 1 );
      return;
    }
    /* Fallthrough */
  default:
    if (!mpz_sgn(get_mpz(sp-1, 1, "`/", 1, 1)))
    {
      free_object(res);
      SIMPLE_DIVISION_BY_ZERO_ERROR ("`/");
    }

    mpz_fdiv_q(OBTOMPZ(res), OBTOMPZ(res), OBTOMPZ(sp[-1].u.object));
  }

  PUSH_REDUCED(res);
}

/*! @decl Gmp.mpz ``/(int|float|Gmp.mpz x)
 */
static void mpzmod_rdiv(INT32 args)
{
  DECLARE_THIS();
  MP_INT *a;
  struct object *res = NULL;

  if(args!=1)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("``/", 1);

  if(!mpz_sgn(THIS))
    SIMPLE_DIVISION_BY_ZERO_ERROR ("``/");

  if( TYPEOF(sp[-1]) == PIKE_T_FLOAT )
  {
    sp[-1].u.float_number /= mpz_get_d( THIS  );
    return;
  }
  /* mpq and mpf handled by div in said classes. */

  a=get_mpz(sp-1, 1, "``/", 1, 1);

  res=fast_clone_object(THIS_PROGRAM);
  mpz_fdiv_q(OBTOMPZ(res), a, THIS);
  pop_stack();
  PUSH_REDUCED(res);
}

/*! @decl Gmp.mpz `%(int|float|Gmp.mpz x)
 */
static void mpzmod_mod(INT32 args)
{
  DECLARE_THIS();
  struct object *res;

  if(args!=1)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("`%", 1);

  if (!mpz_sgn(get_mpz(sp-1, 1, "`%", 1, 1)))
      SIMPLE_DIVISION_BY_ZERO_ERROR ("`%");

  res = fast_clone_object(THIS_PROGRAM);
  mpz_set(OBTOMPZ(res), THIS);
  mpz_fdiv_r(OBTOMPZ(res), OBTOMPZ(res), OBTOMPZ(sp[-1].u.object));

  pop_stack();
  PUSH_REDUCED(res);
}

/*! @decl Gmp.mpz ``%(int|float|Gmp.mpz x)
 */
static void mpzmod_rmod(INT32 args)
{
  DECLARE_THIS();
  MP_INT *a;
  struct object *res = NULL;

  if(args!=1)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("``%", 1);

  if(!mpz_sgn(THIS))
    SIMPLE_DIVISION_BY_ZERO_ERROR ("``%");

  a=get_mpz(sp-1, 1, "``%", 1, 1);

  res=fast_clone_object(THIS_PROGRAM);
  mpz_fdiv_r(OBTOMPZ(res), a, THIS);
  pop_stack();
  PUSH_REDUCED(res);
}

/*! @decl array(Gmp.mpz) gcdext(int|float|Gmp.mpz x)
 *!
 *! Compute the greatest common divisor between this mpz object and
 *! @[x]. An array @expr{({g,s,t})@} is returned where @expr{g@} is
 *! the greatest common divisor, and @expr{s@} and @expr{t@} are the
 *! coefficients that satisfies
 *!
 *! @code
 *!                this * s + @[x] * t = g
 *! @endcode
 *!
 *! @seealso
 *!   @[gcdext2], @[gcd]
 */
static void mpzmod_gcdext(INT32 args)
{
  DECLARE_THIS();
  struct object *g, *s, *t;
  MP_INT *a;

  if (args != 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("gcdext", 1);

  a = get_mpz(sp-1, 1, "gcdext", 1, 1);

  g = fast_clone_object(THIS_PROGRAM);
  s = fast_clone_object(THIS_PROGRAM);
  t = fast_clone_object(THIS_PROGRAM);

  mpz_gcdext(OBTOMPZ(g), OBTOMPZ(s), OBTOMPZ(t), THIS, a);
  pop_n_elems(args);
  PUSH_REDUCED(g); PUSH_REDUCED(s); PUSH_REDUCED(t);
  f_aggregate(3);
}

/*! @decl array(Gmp.mpz) gcdext2(int|float|Gmp.mpz x)
 *!
 *! Compute the greatest common divisor between this mpz object and
 *! @[x]. An array @expr{({g,s})@} is returned where @expr{g@} is the
 *! greatest common divisor, and @expr{s@} is a coefficient that
 *! satisfies
 *!
 *! @code
 *!                this * s + @[x] * t = g
 *! @endcode
 *!
 *! where @expr{t@} is some integer value.
 *!
 *! @seealso
 *!   @[gcdext], @[gcd]
 */
static void mpzmod_gcdext2(INT32 args)
{
  DECLARE_THIS();
  struct object *g, *s;
  MP_INT *a;

  if (args != 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("gcdext2", 1);

  a = get_mpz(sp-args, 1, "gcdext2", 1, 1);

  g = fast_clone_object(THIS_PROGRAM);
  s = fast_clone_object(THIS_PROGRAM);

  mpz_gcdext(OBTOMPZ(g), OBTOMPZ(s), NULL, THIS, a);
  pop_n_elems(args);
  PUSH_REDUCED(g); PUSH_REDUCED(s);
  f_aggregate(2);
}

/*! @decl Gmp.mpz invert(int|float|Gmp.mpz x)
 *!
 *! Return the inverse of this mpz value modulo @[x]. The returned
 *! value satisfies @expr{0 <= result < x@}.
 *!
 *! @throws
 *!   An error is thrown if no inverse exists.
 */
static void mpzmod_invert(INT32 args)
{
  DECLARE_THIS();
  MP_INT *modulo;
  struct object *res;

  if (args != 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("invert", 1);
  modulo = get_mpz(sp-1, 1, "invert", 1, 1);

  if (!mpz_sgn(modulo))
    SIMPLE_DIVISION_BY_ZERO_ERROR ("invert");
  res = fast_clone_object(THIS_PROGRAM);
  if (mpz_invert(OBTOMPZ(res), THIS, modulo) == 0)
  {
    free_object(res);
    Pike_error("Not invertible.\n");
  }
  pop_n_elems(args);
  PUSH_REDUCED(res);
}

/*! @decl Gmp.mpz fac()
 *!
 *! Return the factorial of this mpz object.
 *!
 *! @throws
 *!   Since factorials grow very quickly, only small integers are
 *!   supported. An error is thrown if the value in this mpz object is
 *!   too large.
 */
static void mpzmod_fac(INT32 args)
{
  DECLARE_THIS();
  struct object *res;
  if (mpz_sgn (THIS) < 0)
    Pike_error ("Cannot calculate factorial for negative integer.\n");
  if (!mpz_fits_ulong_p (THIS))
    Pike_error ("Integer too large for factorial calculation.\n");
  res = fast_clone_object (THIS_PROGRAM);
  mpz_fac_ui (OBTOMPZ(res), mpz_get_ui (THIS));
  pop_n_elems (args);
  PUSH_REDUCED (res);
}

/*! @decl Gmp.mpz bin(int k)
 *!
 *! Return the binomial coefficient @expr{n@} over @[k], where
 *! @expr{n@} is the value of this mpz object. Negative values of
 *! @expr{n@} are supported using the identity
 *!
 *! @code
 *!     (-n)->bin(k) == (-1)->pow(k) * (n+k-1)->bin(k)
 *! @endcode
 *!
 *! (See Knuth volume 1, section 1.2.6 part G.)
 *!
 *! @throws
 *!   The @[k] value can't be arbitrarily large. An error is thrown if
 *!   it's too large.
 */
static void mpzmod_bin(INT32 args)
{
  DECLARE_THIS();
  MP_INT *k;
  struct object *res;

  if (args != 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("bin", 1);
  k = get_mpz (sp-1, 1, "bin", 1, 1);
  if (mpz_sgn (k) < 0)
    Pike_error ("Cannot calculate binomial with negative k value.\n");
  if (!mpz_fits_ulong_p (k))
    SIMPLE_ARG_ERROR ("bin", 1, "Argument too large.\n");

  res = fast_clone_object(THIS_PROGRAM);
  mpz_bin_ui (OBTOMPZ (res), THIS, mpz_get_ui (k));

  pop_n_elems(args);
  PUSH_REDUCED(res);
}

#define BINFUN(name, errmsg_name, fun)			\
static void name(INT32 args)				\
{							\
  DECLARE_THIS();                                       \
  INT32 e;						\
  struct object *res;					\
  for(e=0; e<args; e++)					\
    get_mpz(sp+e-args, 1, errmsg_name, e + 1, args);	\
  res = fast_clone_object(THIS_PROGRAM);		\
  mpz_set(OBTOMPZ(res), THIS);				\
  for(e=0;e<args;e++)					\
    fun(OBTOMPZ(res), OBTOMPZ(res),			\
	OBTOMPZ(sp[e-args].u.object));			\
  pop_n_elems(args);					\
  PUSH_REDUCED(res);					\
}

/*! @decl Gmp.mpz `&(int|float|Gmp.mpz x)
 */
BINFUN(mpzmod_and, "`&", mpz_and)

/*! @decl Gmp.mpz `|(int|float|Gmp.mpz x)
 */
BINFUN(mpzmod_or, "`|", mpz_ior)

/*! @decl Gmp.mpz `^(int|float|Gmp.mpz x)
 */
BINFUN(mpzmod_xor, "`^", mpz_xor)

/*! @decl Gmp.mpz `~()
 */
static void mpzmod_compl(INT32 args)
{
  DECLARE_THIS();
  struct object *o;
  if (args)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("`~", 0);
  o=fast_clone_object(THIS_PROGRAM);
  mpz_com(OBTOMPZ(o), THIS);
  PUSH_REDUCED(o);
}

#define CMPEQU(name,errmsg_name,cmp,default)              \
static void name(INT32 args)                              \
{                                                         \
  DECLARE_THIS();                                         \
  INT32 i;                                                \
  MP_INT *arg;                                            \
  if(!args) SIMPLE_WRONG_NUM_ARGS_ERROR (errmsg_name, 1); \
  if (!(arg = get_mpz(sp-args, 0, NULL, 0, 0)))		  \
    default;                                              \
  else                                                    \
    i=mpz_cmp(THIS, arg) cmp 0;                           \
  pop_n_elems(args);                                      \
  push_int(i);                                            \
}

#define RET_UNDEFINED do{push_undefined();return;}while(0)

/*! @decl int(0..1) `>(mixed with)
 */
CMPEQU(mpzmod_gt, "`>", >, RET_UNDEFINED)

/*! @decl int(0..1) `<(mixed with)
 */
CMPEQU(mpzmod_lt, "`<", <, RET_UNDEFINED)

/*! @decl int(0..1) `==(mixed with)
 */
CMPEQU(mpzmod_eq, "`==", ==, RET_UNDEFINED)

/*! @decl int(0..2) probably_prime_p(void|int count)
 *!
 *! Return 2 if this mpz object is a prime, 1 if it probably is a
 *! prime, and 0 if it definitely is not a prime. Testing values below
 *! 1000000 will only return 2 or 0.
 *!
 *! @param count
 *!   The prime number testing is using Donald Knuth's probabilistic
 *!   primality test. The chance for a false positive is
 *!   pow(0.25,count). Default value is 25 and resonable values are
 *!   between 15 and 50.
 */
static void mpzmod_probably_prime_p(INT32 args)
{
  DECLARE_THIS();
  INT_TYPE count;
  if (args)
  {
    if (TYPEOF(sp[-args]) != T_INT)
      SIMPLE_ARG_TYPE_ERROR ("probably_prime_p", 1, "int(1..)");
    count = sp[-args].u.integer;
    if (count <= 0)
      SIMPLE_ARG_TYPE_ERROR ("probably_prime_p", 1, "int(1..)");
  } else
    count = 25;
  pop_n_elems(args);
  push_int(mpz_probab_prime_p(THIS, count));
}

/* Define NUMBER_OF_PRIMES and primes[] */
#include "prime_table.h"

/* Returns a small factor of n, or 0 if none is found.*/
static unsigned long mpz_small_factor(mpz_t n, int limit)
{
  int i;
  unsigned long stop;

  if (limit > NUMBER_OF_PRIMES)
    limit = NUMBER_OF_PRIMES;

  stop = mpz_get_ui(n);

  if (mpz_cmp_ui(n, stop) != 0)
    stop = ULONG_MAX;
  stop = (long)sqrt(stop)+1;

  for (i = 0; (i < limit) && primes[i] < stop; i++)
    if (mpz_fdiv_ui(n, primes[i]) == 0)
      return primes[i];

  return 0;
}

/*! @decl int small_factor(void|int(1..) limit)
 */
static void mpzmod_small_factor(INT32 args)
{
  DECLARE_THIS();
  INT_TYPE limit;

  if (args)
    {
      if (TYPEOF(sp[-args]) != T_INT)
	SIMPLE_ARG_TYPE_ERROR ("small_factor", 1, "int(1..)");
      limit = sp[-args].u.integer;
      if (limit < 1)
	SIMPLE_ARG_TYPE_ERROR ("small_factor", 1, "int(1..)");
    }
  else
    limit = INT_MAX;
  pop_n_elems(args);
  push_int(mpz_small_factor(THIS, limit));
}

/*! @decl Gmp.mpz next_prime()
 *!
 *! Returns the next higher prime for positive numbers and the next
 *! lower for negative.
 *!
 *! The prime number testing is using Donald Knuth's probabilistic
 *! primality test. The chance for a false positive is pow(0.25,25).
 */
static void mpzmod_next_prime(INT32 args)
{
  DECLARE_THIS();
  struct object *o;

  pop_n_elems(args);

  o = fast_clone_object(THIS_PROGRAM);
  mpz_nextprime(OBTOMPZ(o), THIS);

  PUSH_REDUCED(o);
}

/*! @decl int sgn()
 *!
 *! Return the sign of the integer, i.e. @expr{1@} for positive
 *! numbers and @expr{-1@} for negative numbers.
 */
static void mpzmod_sgn(INT32 args)
{
  DECLARE_THIS();
  pop_n_elems(args);
  push_int(mpz_sgn(THIS));
}

/*! @decl Gmp.mpz sqrt()
 *!
 *! Return the the truncated integer part of the square root of this
 *! mpz object.
 */
static void mpzmod_sqrt(INT32 args)
{
  DECLARE_THIS();
  struct object *o = 0;   /* Make gcc happy. */
  pop_n_elems(args);
  if(mpz_sgn(THIS)<0)
    Pike_error("Gmp.mpz->sqrt() on negative number.\n");

  o=fast_clone_object(THIS_PROGRAM);
  mpz_sqrt(OBTOMPZ(o), THIS);
  PUSH_REDUCED(o);
}

/*! @decl array(Gmp.mpz) sqrtrem()
 */
static void mpzmod_sqrtrem(INT32 args)
{
  DECLARE_THIS();
  struct object *root = 0, *rem = 0;   /* Make gcc happy. */

  pop_n_elems(args);
  if(mpz_sgn(THIS)<0)
    Pike_error("Gmp.mpz->sqrtrem() on negative number.\n");

  root = fast_clone_object(THIS_PROGRAM);
  rem = fast_clone_object(THIS_PROGRAM);
  mpz_sqrtrem(OBTOMPZ(root), OBTOMPZ(rem), THIS);
  PUSH_REDUCED(root); PUSH_REDUCED(rem);
  f_aggregate(2);
}

/*! @decl Gmp.mpz `<<(int|float|Gmp.mpz x)
 */
static void mpzmod_lsh(INT32 args)
{
  DECLARE_THIS();
  struct object *res = NULL;
  MP_INT *mi;

  if (args != 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("`<<", 1);

  if(TYPEOF(sp[-1]) == T_INT) {
    if(sp[-1].u.integer < 0)
      SIMPLE_ARG_ERROR ("`<<", 1, "Got negative shift count.");
#ifdef BIG_PIKE_INT
    if (!FITS_ULONG (sp[-1].u.integer) && mpz_sgn (THIS))
      SIMPLE_ARG_ERROR ("`<<", 1, "Shift count too large.");
#endif
    res = fast_clone_object(THIS_PROGRAM);
    mpz_mul_2exp(OBTOMPZ(res), THIS, sp[-1].u.integer);
  } else {
    mi = get_mpz(sp-1, 1, "`<<", 1, 1);
    if(mpz_sgn(mi)<0)
      SIMPLE_ARG_ERROR ("`<<", 1, "Got negative shift count.");
    if(!mpz_fits_ulong_p(mi))
    {
       if(mpz_sgn(THIS))
	 SIMPLE_ARG_ERROR ("`<<", 1, "Shift count too large.");
       else {
    /* Special case: shifting 0 left any number of bits still yields 0 */
	  res = fast_clone_object(THIS_PROGRAM);
	  mpz_set_si(OBTOMPZ(res), 0);
       }
    } else {
       res = fast_clone_object(THIS_PROGRAM);
       mpz_mul_2exp(OBTOMPZ(res), THIS, mpz_get_ui (mi));
    }
  }

  pop_n_elems(args);
  PUSH_REDUCED(res);
}

/*! @decl Gmp.mpz `>>(int|float|Gmp.mpz x)
 */
static void mpzmod_rsh(INT32 args)
{
  DECLARE_THIS();
  struct object *res = NULL;

  if (args != 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("`>>", 1);

  if(TYPEOF(sp[-1]) == T_INT)
  {
     if (sp[-1].u.integer < 0)
       SIMPLE_ARG_ERROR ("`>>", 1, "Got negative shift count.");
#ifdef BIG_PIKE_INT
     if (!FITS_ULONG (sp[-1].u.integer))
     {
	res = fast_clone_object(THIS_PROGRAM);
	mpz_set_si(OBTOMPZ(res), mpz_sgn(THIS)<0? -1:0);
     }
     else
#endif
     {
	res = fast_clone_object(THIS_PROGRAM);
	mpz_fdiv_q_2exp(OBTOMPZ(res), THIS, sp[-1].u.integer);
     }
  }
  else
  {
     MP_INT *mi = get_mpz(sp-1, 1, "`>>", 1, 1);
     if(!mpz_fits_ulong_p (mi)) {
       if(mpz_sgn(mi)<0)
	 SIMPLE_ARG_ERROR ("`>>", 1, "Got negative shift count.");
       res = fast_clone_object(THIS_PROGRAM);
       mpz_set_si(OBTOMPZ(res), mpz_sgn(THIS)<0? -1:0);
     }
     else {
	res = fast_clone_object(THIS_PROGRAM);
	mpz_fdiv_q_2exp(OBTOMPZ(res), THIS, mpz_get_ui (mi));
     }
  }

  pop_n_elems(args);
  PUSH_REDUCED(res);
}

/*! @decl Gmp.mpz ``<<(int|float|Gmp.mpz x)
 */
static void mpzmod_rlsh(INT32 args)
{
  DECLARE_THIS();
  struct object *res = NULL;
  MP_INT *mi;

  if (args != 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("``<<", 1);
  if(mpz_sgn(THIS) < 0)
    Pike_error ("Gmp.mpz->``<<(): Got negative shift count.\n");

  mi = get_mpz(sp-1, 1, "``<<", 1, 1);

  /* Cut off at 1MB ie 0x800000 bits. */
  if(!mpz_fits_ulong_p(THIS)) {
    if(mpz_sgn(mi))
      Pike_error ("Gmp.mpz->``<<(): Shift count too large.\n");
    else {
      /* Special case: shifting 0 left any number of bits still yields 0 */
      res = fast_clone_object(THIS_PROGRAM);
      mpz_set_si(OBTOMPZ(res), 0);
    }
  } else {
    res = fast_clone_object(THIS_PROGRAM);
    mpz_mul_2exp(OBTOMPZ(res), mi, mpz_get_ui (THIS));
  }

  pop_n_elems(args);
  PUSH_REDUCED(res);
}

/*! @decl Gmp.mpz ``>>(int|float|Gmp.mpz x)
 */
static void mpzmod_rrsh(INT32 args)
{
  DECLARE_THIS();
  struct object *res = NULL;
  MP_INT *mi;

  if (args != 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("``>>", 1);
  mi = get_mpz(sp-1, 1, "``>>", 1, 1);

  if (!mpz_fits_ulong_p (THIS)) {
    if(mpz_sgn(THIS) < 0)
      Pike_error ("Gmp.mpz->``>>(): Got negative shift count.\n");
    res = fast_clone_object(THIS_PROGRAM);
    mpz_set_si(OBTOMPZ(res), mpz_sgn(mi)<0? -1:0);
  }
  else {
    res = fast_clone_object(THIS_PROGRAM);
    mpz_fdiv_q_2exp(OBTOMPZ(res), mi, mpz_get_ui (THIS));
  }

  pop_n_elems(args);
  PUSH_REDUCED(res);
}

/*! @decl Gmp.mpz powm(int|string|float|Gmp.mpz exp,@
 *!                    int|string|float|Gmp.mpz mod)
 *!
 *! Return @expr{( this->pow(@[exp]) ) % @[mod]@}.
 *!
 *! @seealso
 *!   @[pow]
 */
static void mpzmod_powm(INT32 args)
{
  DECLARE_THIS();
  struct object *res = NULL;
  MP_INT *n, *e;

  if(args != 2)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("powm", 2);

  e = get_mpz(sp - 2, 1, "powm", 1, 2);
  n = get_mpz(sp - 1, 1, "powm", 2, 2);

  if (!mpz_sgn(n))
    SIMPLE_DIVISION_BY_ZERO_ERROR ("powm");
  res = fast_clone_object(THIS_PROGRAM);
  mpz_powm(OBTOMPZ(res), THIS, e, n);
  pop_n_elems(args);
  PUSH_REDUCED(res);
}

/*! @decl Gmp.mpz|Gmp.mpq ``**(int|float|Gmp.mpz|Gmp.mpq x)
 *!
 *! Return @[x] raised to the value of this object. The case when zero is
 *! raised to zero yields one. When this object has a negative value and
 *! @[x] is not a float, a @[Gmp.mpq] object will be returned.
 *!
 *! @seealso
 *!   @[pow]
 */
static void mpzmod_rpow(INT32 args)
{
  DECLARE_THIS();

  if (args != 1) {
    SIMPLE_WRONG_NUM_ARGS_ERROR("``**", 1);
  }

  if( TYPEOF(sp[-1]) == PIKE_T_FLOAT )
  {
    sp[-1].u.float_number = pow(sp[-1].u.float_number,
                                mpz_get_d(THIS));
    return;
  }

  if( mpz_sgn(THIS) == 0 )
  {
    push_int(1);
    return;
  }

  /* FIXME: Add a special case for us being == 1.
   *        in which case we can just return.
   */

  if (TYPEOF(sp[-1]) == PIKE_T_INT) {
    INT_TYPE val = sp[-1].u.integer;
    if (!val || val == 1) {
      /* Identity. */
      return;
    } else if (val == -1) {
      if (!(mpz_getlimbn(THIS, 0) & 1)) {
	/* Even. */
	sp[-1].u.integer = 1;
      }
      return;
    }
  }

  /* Convert the argument to a bignum. */
  get_mpz(sp-1, 1, "``**", 1, args);
  assert(TYPEOF(sp[-1]) == PIKE_T_OBJECT);

  ref_push_object(Pike_fp->current_object);
  apply_lfun(sp[-2].u.object, LFUN_POW, 1);
}

/*! @decl Gmp.mpz pow(int|float|Gmp.mpz x)
 *!
 *! Return this mpz object raised to @[x]. The case when zero is
 *! raised to zero yields one.
 *!
 *! @seealso
 *!   @[powm]
 */
static void mpzmod_pow(INT32 args)
{
  DECLARE_THIS();
  struct object *res = NULL;
  MP_INT *mi;
  unsigned long exponent = 0;
  size_t size;
  double ep;

  if (args != 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("pow", 1);

  if (TYPEOF(sp[-1]) == T_INT) {
    exponent = sp[-1].u.integer;
    if (sp[-1].u.integer < 0) {
    negative_exponent:
      if (!mpz_sgn(THIS)) {
	Pike_error("Division by zero.\n");
      }
      if(mpz_cmp_si(THIS, -1)<0 || mpz_cmp_si(THIS, 1)>0) {
	/* Not -1 or +1. */
	o_negate();	/* NB: Use o_negate() to handle MIN_INT_TYPE. */
	mpzmod_pow(1);
	push_int(1);
	stack_swap();
	push_object(clone_object(mpq_program, 2));
	return;
      }
      /* We are either -1 or +1 here. Flipping the sign of the
       * exponent is ok. And we only care about the exponent
       * being even or odd.
       */
      exponent &= 1;
    }
  } else if( TYPEOF(sp[-1]) == PIKE_T_FLOAT )
  {
      sp[-1].u.float_number = pow(mpz_get_d(THIS),sp[-1].u.float_number);
      return;
  }
  if( TYPEOF(sp[-1]) == PIKE_T_OBJECT )
  {
      double tmp;
      if( sp[-1].u.object->prog == mpq_program )
      {
          // could use rpow in mpq.. But it does not handle mpz objects.
          tmp = mpq_get_d( (MP_RAT*)sp[-1].u.object->storage );
          push_float( pow(mpz_get_d(THIS),tmp) );
          return;
      }
      else if( sp[-1].u.object->prog == mpf_program )
      {
          tmp = mpf_get_d( (MP_FLT*)sp[-1].u.object->storage );
          push_float( pow(mpz_get_d(THIS),tmp) );
          return;
      }
      else
      {
          mi = get_mpz(sp-1, 1, "pow", 1, 1);
	  if (mpz_fits_ulong_p(mi) ||
	      ((mpz_cmp_si(THIS, -1) >= 0) && (mpz_cmp_si(THIS, 1) <= 0))) {
	      /* The exponent fits in an unsigned long, or
	       * the base is -1, 0, or 1 in which case the higher bits
	       * of the exponent are not relevant.
	       */
	      exponent=mpz_get_ui(mi);
	      if(mpz_sgn(mi)<0)
	      {
		goto negative_exponent;
	      }
	  } else {
	      SIMPLE_ARG_ERROR ("pow", 1, "Exponent too large.");
	  }
      }
  }

  /* Cut off at 1GB. */
  size = mpz_size(THIS);
  if (INT_TYPE_MUL_OVERFLOW(exponent, size) ||
      size * exponent > (INT_TYPE)(0x40000000/sizeof(mp_limb_t))) {
    if ((mpz_cmp_si(THIS, -1) < 0) || (mpz_cmp_si(THIS, 1) > 0)) {
      /* Could be a problem. Let's look closer... */
      long e;
      double mantissa = mpz_get_d_2exp(&e, THIS);
      if (mantissa < 0) mantissa = -mantissa;
      ep = (e + log2(mantissa)) * exponent;	/* ~= log2(result) */
      ep /= 8.0;	/* bits ==> bytes */
      if (ep > ((double)0x40000000)) {	/* 1GB */
	SIMPLE_ARG_ERROR ("pow", 1, "Exponent too large.");
      }
    }
  }

  res = fast_clone_object(THIS_PROGRAM);
  mpz_pow_ui(OBTOMPZ(res), THIS, exponent);
  pop_n_elems(args);
  PUSH_REDUCED(res);
}

/*! @decl int(0..1) `!()
 */
static void mpzmod_not(INT32 args)
{
  DECLARE_THIS();
  pop_n_elems(args);
  push_int(!mpz_sgn(THIS));
}

/*! @decl int popcount()
 *! For values >= 0, returns the population count (the number of set bits).
 *! For negative values (who have an infinite number of leading ones in a
 *! binary representation), -1 is returned.
 */
static void mpzmod_popcount(INT32 args)
{
  DECLARE_THIS();
  pop_n_elems(args);
  push_int(mpz_popcount(THIS));
#ifdef BIG_PIKE_INT
/* need conversion from MAXUINT32 to -1 (otherwise it's done already) */
  if (Pike_sp[-1].u.integer==0xffffffffLL)
     Pike_sp[-1].u.integer=-1;
#endif
}

/*! @decl int hamdist(int x)
 *! Calculates the hamming distance between this number and @[x].
 */
static void mpzmod_hamdist(INT32 args)
{
  DECLARE_THIS();
  MP_INT *x;
  if (args != 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("hamdist", 1);
  x = get_mpz(sp-1, 1, "hamdist", 1, 1);
  push_int(mpz_hamdist(THIS, x));
  stack_swap();
  pop_stack();
}

/*! @decl Gmp.mpz _random(function(int:string(8bit)) random_string, @
 *!                       function(mixed:mixed) random)
 */
static void mpzmod_random(INT32 args)
{
  struct object *res;
  MP_INT *mpz_res;
  unsigned bits, bytes;
  mp_limb_t mask;
  int i, fast=0;
  DECLARE_THIS();

  /* NB: Nominally we could survive with just one argument too, but... */
  if (args != 2)
    SIMPLE_WRONG_NUM_ARGS_ERROR("_random", 2);
  if(mpz_sgn(THIS) <= 0)
  {
    push_int(0);
    return;
  }

  /* On stack: random_string, random */
  pop_stack();
  if(TYPEOF(sp[-1]) != T_FUNCTION)
    Pike_error("_random called with non-function argument.\n");

  res = fast_clone_object(THIS_PROGRAM);
  push_object(res);
  stack_swap();

  /* On stack: res, random_string */

  mpz_res = OBTOMPZ(res);
  bits = mpz_sizeinbase(THIS, 2);
  bytes = ((bits-1)>>3)+1;
  mask = (1UL<<(bits & ((sizeof(mp_limb_t)<<3) - 1)))-1;

  if (mpz_popcount(THIS) == 1) {
    /* The number of bits is a power of 2.
     * Ie the most significant bit of the masked result
     * should always be zero, so we can shrink the mask.
     */
    if (!mask) {
      mask = ~0;
    }
    mask >>= 1;
    bits--;

    /* There's also no need to compare the limit, as
     * all bitpatterns are valid after masking.
     */
    fast=1;

    if (!(bits & 0x07)) {
      /* One less byte of random data needed. */
      bytes--;
      fast = 2;	/* No need for masking either. */
    }
  }

  if (!(bits & 0x07)) {
    /* Whole number of bytes. No need for masking. */
    mask = 0;
  }

  i = 0;
  do {
    // FIXME: We replace the entire string if we are too large. We
    // could be smarter here, but it's easy to introduce bias by
    // mistake.

    push_int((int)bytes);
    apply_svalue(&sp[-2], 1);
    if (TYPEOF(sp[-1]) != T_STRING)
      Pike_error("random_string(%u) returned non string.\n", bytes);
    if ((unsigned)sp[-1].u.string->len != bytes ||
        sp[-1].u.string->size_shift != 0)
      Pike_error("Wrong size random string generated.\n");

    mpz_import(mpz_res, bytes, 1, 1, 0, 0, sp[-1].u.string->str);
    pop_stack();

    if (fast == 2) {
      /* We've decreased the number of bytes above.
       * Ie we have the special case of an even number of random bytes.
       */
      goto done;
    }

    if (mask && (mpz_res->_mp_size == THIS->_mp_size)) {
      /* Apply the mask. */
      /*
       * CAVEAT: We're messing with Gmp internals here...
       */
      /* NB: res is always positive, so the subtraction is safe. */
      mpz_res->_mp_d[mpz_res->_mp_size - 1] &= mask;
      /* Get rid of leading zero limbs. Otherwise gmp will get
       * sad and zap innocent memory.
       */
      while (mpz_res->_mp_size && !mpz_res->_mp_d[mpz_res->_mp_size - 1]) {
	mpz_res->_mp_size--;
      }
    }

    if( fast || mpz_cmp(THIS, mpz_res) > 0 )
      goto done;
    /* NB: With a rectangular distribution we have a probability
     *     of success of at least 50% in the test above, so the
     *     failure rate is less than 1 in 2^1000.
     */
  } while (++i < 1000);
  Pike_error("Unable to generate random data.\n");

 done:
  /* Pop the random_string() function. */
  pop_stack();

  /* res is now at the top of the stack. Reduce it. */
  Pike_sp--;
  dmalloc_touch_svalue(Pike_sp);
  PUSH_REDUCED(res);
}

/*! @decl string(0..255) _encode()
 */
static void mpzmod__encode(INT32 args)
{
  pop_n_elems(args);
  /* 256 would be better, but then negative numbers
   * won't work... /Hubbe
   */
  push_int(36);
  mpzmod_digits(1);
}

/*! @decl void _decode(string(0..255))
 */
static void mpzmod__decode(INT32 args)
{
  /* 256 would be better, but then negative numbers
   * won't work... /Hubbe
   */
  push_int(36);
  mpzmod_create(args+1);
}

/*! @endclass
 */

/*! @decl Gmp.mpz fac(int x)
 *! Returns the factorial of @[x] (@[x]!).
 */
static void gmp_fac(INT32 args)
{
  DECLARE_THIS();
  struct object *res = NULL;
  if (args != 1)
    Pike_error("Gmp.fac: Wrong number of arguments.\n");
  if (TYPEOF(sp[-1]) != T_INT)
    SIMPLE_ARG_TYPE_ERROR ("fac", 1, "int");
  if (sp[-1].u.integer < 0)
    SIMPLE_ARG_ERROR ("fac", 1, "Got negative factorial.");
  res = fast_clone_object(mpzmod_program);
  mpz_fac_ui(OBTOMPZ(res), sp[-1].u.integer);
  pop_n_elems(args);
  PUSH_REDUCED(res);
}

static void mpzmod__size_object(INT32 UNUSED(args))
{
  DECLARE_THIS();
  push_int(ALIMBS(THIS) * sizeof(mp_limb_t) + sizeof(mpz_t));
}

static void init_mpz_glue(struct object * UNUSED(o))
{
  DECLARE_THIS();
  mpz_init(THIS);
}

static void exit_mpz_glue(struct object *UNUSED(o))
{
  DECLARE_THIS();
  if( THIS_OBJECT->flags & OBJECT_CLEAR_ON_EXIT )
    memset(LIMBS(THIS), 0, ALIMBS(THIS) * sizeof(mp_limb_t));
  mpz_clear(THIS);
}

static void gc_recurse_mpz (struct object *o)
{
  DECLARE_THIS();
  if (mc_count_bytes (o))
    mc_counted_bytes += ALIMBS(THIS) * sizeof(mp_limb_t) + sizeof(mpz_t);
}

static void *pike_mp_alloc (size_t alloc_size)
{
  void *ret = malloc (alloc_size);
  if (!ret)
    /* According to gmp docs, we're neither allowed to return zero nor
     * longjmp here. */
    Pike_fatal ("Failed to allocate %"PRINTSIZET"db in gmp library.\n",
		alloc_size);
  return ret;
}

static void *pike_mp_realloc (void *ptr, size_t old_size, size_t new_size)
{
  void *ret = realloc (ptr, new_size);
  if (!ret)
    /* According to gmp docs, we're neither allowed to return zero nor
     * longjmp here. */
    Pike_fatal ("Failed to reallocate %"PRINTSIZET"db block "
		"to %"PRINTSIZET"db in gmp library.\n", old_size, new_size);
  return ret;
}

static void pike_mp_free (void *ptr, size_t UNUSED(size))
{
  free (ptr);
}

#define MPZ_DEFS()							\
  ADD_STORAGE(MP_INT);							\
  									\
  /* function(void|string|int|float|object:void)" */			\
  /* "|function(string,int:void) */					\
  ADD_FUNCTION("create", mpzmod_create,					\
	       tOr(tFunc(tOr5(tVoid,tStr,tInt,tFlt,			\
			      tObj),tVoid),				\
		   tFunc(tStr tInt,tVoid)), ID_PROTECTED);		\
									\
  ADD_FUNCTION("`+",mpzmod_add,tMpz_binop_type, ID_PROTECTED);		\
  ADD_FUNCTION("`+=",mpzmod_add_eq,tMpz_binop_type, ID_PROTECTED);	\
  ADD_FUNCTION("``+",mpzmod_add_rhs,tMpz_binop_type, ID_PROTECTED);	\
  ADD_FUNCTION("`-",mpzmod_sub,tMpz_binop_type, ID_PROTECTED);		\
  ADD_FUNCTION("``-",mpzmod_rsub,tMpz_binop_type, ID_PROTECTED);	\
  ADD_FUNCTION("`*",mpzmod_mul,tMpz_binop_type, ID_PROTECTED);		\
  ADD_FUNCTION("``*",mpzmod_mul,tMpz_binop_type, ID_PROTECTED);		\
  ADD_FUNCTION("`/",mpzmod_div,tMpz_binop_type, ID_PROTECTED);		\
  ADD_FUNCTION("``/",mpzmod_rdiv,tMpz_binop_type, ID_PROTECTED);	\
  ADD_FUNCTION("`%",mpzmod_mod,tMpz_binop_type, ID_PROTECTED);		\
  ADD_FUNCTION("``%",mpzmod_rmod,tMpz_binop_type, ID_PROTECTED);	\
  ADD_FUNCTION("`&",mpzmod_and,tMpz_binop_type, ID_PROTECTED);		\
  ADD_FUNCTION("``&",mpzmod_and,tMpz_binop_type, ID_PROTECTED);		\
  ADD_FUNCTION("`|",mpzmod_or,tMpz_binop_type, ID_PROTECTED);		\
  ADD_FUNCTION("``|",mpzmod_or,tMpz_binop_type, ID_PROTECTED);		\
  ADD_FUNCTION("`^",mpzmod_xor,tMpz_binop_type, ID_PROTECTED);		\
  ADD_FUNCTION("``^",mpzmod_xor,tMpz_binop_type, ID_PROTECTED);		\
  ADD_FUNCTION("`~",mpzmod_compl,tFunc(tNone,tObj), ID_PROTECTED);	\
									\
  ADD_FUNCTION("`<<",mpzmod_lsh,tMpz_shift_type, ID_PROTECTED);		\
  ADD_FUNCTION("`>>",mpzmod_rsh,tMpz_shift_type, ID_PROTECTED);		\
  ADD_FUNCTION("``<<",mpzmod_rlsh,tMpz_shift_type, ID_PROTECTED);	\
  ADD_FUNCTION("``>>",mpzmod_rrsh,tMpz_shift_type, ID_PROTECTED);	\
									\
  ADD_FUNCTION("`>", mpzmod_gt,tMpz_cmpop_type, ID_PROTECTED);		\
  ADD_FUNCTION("`<", mpzmod_lt,tMpz_cmpop_type, ID_PROTECTED);		\
									\
  ADD_FUNCTION("`==",mpzmod_eq,tMpz_cmpop_type, ID_PROTECTED);		\
									\
  ADD_FUNCTION("`!",mpzmod_not,tFunc(tNone,tInt01), ID_PROTECTED);	\
									\
  ADD_FUNCTION("__hash",mpzmod___hash,tFunc(tNone,tInt), ID_PROTECTED);	\
  ADD_FUNCTION("cast",mpzmod_cast,tFunc(tStr,tMix), ID_PROTECTED);	\
									\
  ADD_FUNCTION("_is_type", mpzmod__is_type, tFunc(tStr,tInt01),         \
               ID_PROTECTED);                                           \
  									\
  ADD_FUNCTION("digits", mpzmod_digits,tFunc(tOr(tVoid,tInt),tStr8), 0);\
  ADD_FUNCTION("encode_json", mpzmod_encode_json,			\
	       tFunc(tOr(tVoid,tInt) tOr(tVoid,tInt),tStr7), 0);	\
  ADD_FUNCTION("_sprintf", mpzmod__sprintf, tFunc(tInt tMapping,tStr),  \
               ID_PROTECTED);                                           \
  ADD_FUNCTION("_size_object",mpzmod__size_object, tFunc(tVoid,tInt),0);\
  ADD_FUNCTION("size", mpzmod_size,tFunc(tOr(tVoid,tInt),tInt1Plus), 0);\
									\
  ADD_FUNCTION("probably_prime_p",mpzmod_probably_prime_p,		\
               tFunc(tOr(tVoid,tIntPos),tInt01),0);                     \
  ADD_FUNCTION("small_factor", mpzmod_small_factor,			\
	       tFunc(tOr(tInt,tVoid),tInt), 0);				\
  ADD_FUNCTION("next_prime", mpzmod_next_prime,				\
	       tFunc(tNone, tMpz_ret), 0);				\
  									\
  ADD_FUNCTION("gcd",mpzmod_gcd, tMpz_binop_type, 0);			\
  ADD_FUNCTION("gcdext",mpzmod_gcdext,tFunc(tMpz_arg,tArr(tMpz_ret)),0);\
  ADD_FUNCTION("gcdext2",mpzmod_gcdext2,tFunc(tMpz_arg,tArr(tMpz_ret)),0);\
  ADD_FUNCTION("invert", mpzmod_invert,tFunc(tMpz_arg,tMpz_ret),0);	\
									\
  ADD_FUNCTION("fac", mpzmod_fac, tFunc(tNone,tMpz_ret), 0);		\
  ADD_FUNCTION("bin", mpzmod_bin, tFunc(tMpz_arg,tMpz_ret), 0);         \
  ADD_FUNCTION("sgn", mpzmod_sgn, tFunc(tNone,tInt), 0);		\
  ADD_FUNCTION("sqrt", mpzmod_sqrt,tFunc(tNone,tMpz_ret),0);		\
  ADD_FUNCTION("_sqrt", mpzmod_sqrt,tFunc(tNone,tMpz_ret),0);		\
  ADD_FUNCTION("sqrtrem",mpzmod_sqrtrem,tFunc(tNone,tArr(tMpz_ret)),0); \
  ADD_FUNCTION("powm",mpzmod_powm,tFunc(tMpz_arg tMpz_arg,tMpz_ret),0);	\
  ADD_FUNCTION("pow", mpzmod_pow,tMpz_shift_type, 0);			\
  ADD_FUNCTION("`**", mpzmod_pow,tMpz_shift_type, 0);			\
  ADD_FUNCTION("``**", mpzmod_rpow,tMpz_shift_type, 0);			\
									\
  ADD_FUNCTION("popcount", mpzmod_popcount,tFunc(tVoid,tInt), 0);	\
  ADD_FUNCTION("hamdist", mpzmod_hamdist,tFunc(tMpz_arg,tInt), 0);	\
									\
  ADD_FUNCTION("_random",mpzmod_random,tFunc(tFunction tFunction,tMpz_ret),0); \
  									\
  ADD_FUNCTION("_encode",mpzmod__encode,tFunc(tNone,tStr8),0);		\
  									\
  ADD_FUNCTION("_decode",mpzmod__decode,tFunc(tStr8,tVoid),0);		\
  									\
  set_init_callback(init_mpz_glue);					\
  set_exit_callback(exit_mpz_glue);					\
  set_gc_recurse_callback (gc_recurse_mpz);


PIKE_MODULE_INIT
{
  /* Make sure that gmp uses the same malloc functions as we do since
   * we got code that frees blocks allocated inside gmp (e.g.
   * mpf.get_string). This also ensures that gmp uses dlmalloc if we
   * do on Windows. In case gmp already uses the same malloc, this is
   * essentially just a NOP. */
  mp_set_memory_functions (pike_mp_alloc, pike_mp_realloc, pike_mp_free);

  start_new_program();

  MPZ_DEFS();

#if 0
  /* These are not implemented yet */
  /* function(:int) */
  ADD_FUNCTION("squarep", mpzmod_squarep,tFunc(tNone,tInt), 0);
  ADD_FUNCTION("divmod", mpzmod_divmod, tFunc(tMpz_arg,tArr(tMpz_ret)), 0);
  ADD_FUNCTION("divm", mpzmod_divm, tFunc(tMpz_arg tMpz_arg,tMpz_ret), 0);
#endif

  mpzmod_program=end_program();
  mpzmod_program->id = PROG_GMP_MPZ_ID;
  add_program_constant("mpz", mpzmod_program, 0);

  ADD_FUNCTION("fac", gmp_fac,tFunc(tInt,tObj), 0);

  /* This program autoconverts to integers, Gmp.mpz does not!!
   * magic? no, just an if statement :)              /Hubbe
   */
  start_new_program();

#undef tMpz_ret
#define tMpz_ret tInt

  /* I first tried to just do an inherit here, but it becomes too hard
   * to tell the programs apart when I do that..          /Hubbe
   */
  MPZ_DEFS();

  add_program_constant("bignum", bignum_program=end_program(), 0);
  bignum_program->id = PROG_GMP_BIGNUM_ID;
  bignum_program->flags |=
    PROGRAM_NO_WEAK_FREE |
    PROGRAM_NO_EXPLICIT_DESTRUCT |
    PROGRAM_CONSTANT ;

  mpz_init (mpz_int_type_min);
  mpz_setbit (mpz_int_type_min, INT_TYPE_BITS);
  mpz_neg (mpz_int_type_min, mpz_int_type_min);

#if SIZEOF_INT64 != SIZEOF_LONG || SIZEOF_INT_TYPE != SIZEOF_LONG 
  mpz_init (mpz_int64_min);
  mpz_setbit (mpz_int64_min, INT64_BITS);
  mpz_neg (mpz_int64_min, mpz_int64_min);
#endif
  pike_init_mpq_module();
  pike_init_mpf_module();
  pike_init_smpz_module();

  /*! @decl constant version
   *! The version of the current GMP library, e.g. @expr{"6.1.0"@}.
   */
#ifdef __NT__
  /* NB: <gmp.h> lacks sufficient export declarations to export
   *     and import variables like gmp_version to gmp.lib.
   *     We thus need to look up the symbol by hand.
   */
  {
    HINSTANCE gmp_dll = LoadLibraryA("gmp");
    if (gmp_dll) {
      const char **gmp_version_var =
        (void *)GetProcAddress(gmp_dll, DEFINETOSTR(gmp_version));
      if (gmp_version_var) {
	const char *gmp_version = *gmp_version_var;
	add_string_constant("version", gmp_version, 0);
      }
      FreeLibrary(gmp_dll);
    }
  }
#else
  add_string_constant("version", gmp_version, 0);
#endif
}

PIKE_MODULE_EXIT
{
  pike_exit_smpz_module();
  pike_exit_mpf_module();
  pike_exit_mpq_module();

  free_program(mpzmod_program);
  free_program(bignum_program);
  bignum_program = NULL;

  mpz_clear (mpz_int_type_min);
#if SIZEOF_INT64 != SIZEOF_LONG || SIZEOF_INT_TYPE != SIZEOF_LONG 
  mpz_clear (mpz_int64_min);
#endif
}

/*! @endmodule
 */
