/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: mpz_glue.c,v 1.160 2004/03/21 18:04:34 grubba Exp $
*/

#include "global.h"
RCSID("$Id: mpz_glue.c,v 1.160 2004/03/21 18:04:34 grubba Exp $");
#include "gmp_machine.h"
#include "module.h"

/* Disable this for now to check that the fallbacks work correctly. */
#undef HAVE_MPZ_IMPORT

#if defined(HAVE_GMP2_GMP_H) && defined(HAVE_LIBGMP2)
#define USE_GMP2
#else /* !HAVE_GMP2_GMP_H || !HAVE_LIBGMP2 */
#if defined(HAVE_GMP_H) && defined(HAVE_LIBGMP)
#define USE_GMP
#endif /* HAVE_GMP_H && HAVE_LIBGMP */
#endif /* HAVE_GMP2_GMP_H && HAVE_LIBGMP2 */

#if defined(USE_GMP) || defined(USE_GMP2)

#include "interpret.h"
#include "svalue.h"
#include "stralloc.h"
#include "array.h"
#include "pike_macros.h"
#include "program.h"
#include "stralloc.h"
#include "object.h"
#include "pike_types.h"
#include "pike_error.h"
#include "builtin_functions.h"
#include "module_support.h"
#include "bignum.h"
#include "operators.h"

#include "my_gmp.h"

#include <limits.h>

#define sp Pike_sp
#define fp Pike_fp

#ifdef _MSC_VER
/* No random()... provide one for gmp
 * This should possibly be a configure test
 * /Hubbe
 */
long random(void)
{
  return my_rand();
}
#endif

#undef THIS
#define THIS ((MP_INT *)(fp->current_storage))
#define THIS_PROGRAM (fp->context.prog)

struct program *mpzmod_program = NULL;
#ifdef AUTO_BIGNUM
struct program *bignum_program = NULL;
#endif

#ifdef AUTO_BIGNUM
static mpz_t mpz_int_type_min;

void mpzmod_reduce(struct object *o)
{
  MP_INT *mpz = OBTOMPZ (o);
  int neg = mpz_sgn (mpz) < 0;
  INT_TYPE res = 0;

  /* Note: Similar code in gmp_int64_from_bignum. */

  /* Get the index of the highest limb that has bits within the range
   * of the INT_TYPE. */
  size_t pos = (INT_TYPE_BITS + GMP_NUMB_BITS - 1) / GMP_NUMB_BITS - 1;

  if (mpz_size (mpz) <= pos + 1) {
    /* NOTE: INT_TYPE is signed, while GMP_NUMB is unsigned.
     *       Thus INT_TYPE_BITS is usually 31 and GMP_NUMB_BITS 32.
     */
#if INT_TYPE_BITS == GMP_NUMB_BITS
    /* NOTE: Overflow is not possible. */
    res = MPZ_GETLIMBN (mpz, 0) & GMP_NUMB_MASK;
#elif INT_TYPE_BITS < GMP_NUMB_BITS
    mp_limb_t val = MPZ_GETLIMBN (mpz, 0) & GMP_NUMB_MASK;
    if (val >= (mp_limb_t) 1 << INT_TYPE_BITS) goto overflow;
    res = val;
#else
    for (;; pos--) {
      res |= MPZ_GETLIMBN (mpz, pos) & GMP_NUMB_MASK;
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
}

#define PUSH_REDUCED(o) do { struct object *reducetmp__=(o);	\
   if(THIS_PROGRAM == bignum_program)				\
     mpzmod_reduce(reducetmp__);					\
   else								\
     push_object(reducetmp__);					\
}while(0)

#ifdef INT64

static void gmp_reduce_stack_top_bignum (void)
{
  struct object *o;
#ifdef PIKE_DEBUG
  if (sp[-1].type != T_OBJECT || sp[-1].u.object->prog != bignum_program)
    Pike_fatal ("Not a Gmp.bignum.\n");
#endif
  o = (--sp)->u.object;
  debug_malloc_touch (o);
  mpzmod_reduce (o);
}

static void gmp_push_int64 (INT64 i)
{
  if(i == DO_NOT_WARN((INT_TYPE)i))
  {
    push_int(DO_NOT_WARN((INT_TYPE)i));
  }
  else
  {
    MP_INT *mpz;

    push_object (fast_clone_object (bignum_program));
    mpz = OBTOMPZ (sp[-1].u.object);

#if SIZEOF_LONG >= SIZEOF_INT64
    PIKE_MPZ_SET_SI (mpz, i);
#else
    {
      int neg = i < 0;
      unsigned INT64 bits = (unsigned INT64) (neg ? -i : i);

#ifdef HAVE_MPZ_IMPORT
      mpz_import (mpz, 1, 1, SIZEOF_INT64, 0, 0, &bits);
#else
      {
	size_t n =
	  ((SIZEOF_INT64 + SIZEOF_LONG - 1) / SIZEOF_LONG - 1)
	  /* The above is the position of the top unsigned long in the INT64. */
	  * ULONG_BITS;
	mpz_set_ui (mpz, (unsigned long) (bits >> n));
	while (n) {
	  n -= ULONG_BITS;
	  mpz_mul_2exp (mpz, mpz, ULONG_BITS);
	  mpz_add_ui (mpz, mpz, (unsigned long) (bits >> n));
	}
      }
#endif	/* !HAVE_MPZ_IMPORT */

      if (neg) mpz_neg (mpz, mpz);
    }
#endif	/* SIZEOF_LONG < SIZEOF_INT64 */
  }
}

static mpz_t mpz_int64_min;

static int gmp_int64_from_bignum (INT64 *i, struct object *bignum)
{
  MP_INT *mpz = OBTOMPZ (bignum);
  int neg = mpz_sgn (mpz) < 0;
  INT64 res = 0;

  /* Note: Similar code in mpzmod_reduce. */

  /* Get the index of the highest limb that have bits within the range
   * of the INT64. */
  size_t pos = (INT64_BITS + GMP_NUMB_BITS - 1) / GMP_NUMB_BITS - 1;

#ifdef PIKE_DEBUG
  if (bignum->prog != bignum_program) Pike_fatal ("Not a Gmp.bignum.\n");
#endif

  if (mpz_size (mpz) <= pos + 1) {
#if INT64_BITS == GMP_NUMB_BITS
    res = MPZ_GETLIMBN (mpz, 0) & GMP_NUMB_MASK;
#elif INT64_BITS < GMP_NUMB_BITS
    mp_limb_t val = MPZ_GETLIMBN (mpz, 0) & GMP_NUMB_MASK;
    if (val >= (mp_limb_t) 1 << INT64_BITS) goto overflow;
    res = val;
#else
    for (;; pos--) {
      res |= MPZ_GETLIMBN (mpz, pos) & GMP_NUMB_MASK;
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
  return 0;
}

#endif /* INT64 */

#else
#define PUSH_REDUCED(o) push_object(o)
#endif /* AUTO_BIGNUM */

/*! @module Gmp
 */

/*! @class bignum
 *! This program is used by the internal auto-bignum conversion. It
 *! can be used to explicitly type integers that are to big to be
 *! INT_TYPE. Best is however to not use this program unless you
 *! really know what you are doing.
 *! @endclass
 */

/*! @class mpz
 *! Gmp.mpz implements very large integers. In fact,
 *! the only limitation on these integers is the available memory.
 *! The mpz object implements all the normal integer operations.
 */

void get_mpz_from_digits(MP_INT *tmp,
			 struct pike_string *digits,
			 int base)
{
  if(!base || ((base >= 2) && (base <= 36)))
  {
    int offset = 0;
    int neg = 0;

    if(digits->len > 1)
    {
      if(INDEX_CHARP(digits->str, 0, digits->size_shift) == '+')
	offset += 1;
      else if(INDEX_CHARP(digits->str, 0, digits->size_shift) == '-')
      {
	offset += 1;
	neg = 1;
      }

      /* We need to fix the case with binary
       * 0b101... and -0b101... numbers.
       *
       * What about hexadecimal and octal?
       *	/grubba 2003-05-16
       *
       * No sweat - they are handled by mpz_set_str. /mast
       */
      if(!base && digits->len > 2)
      {
	if((INDEX_CHARP(digits->str, offset, digits->size_shift) == '0') &&
	   ((INDEX_CHARP(digits->str, offset+1, digits->size_shift) == 'b') ||
	    (INDEX_CHARP(digits->str, offset+1, digits->size_shift) == 'B')))
	{
	  offset += 2;
	  base = 2;
	}
      }
    }

    if (mpz_set_str(tmp, digits->str + offset, base))
      Pike_error("Invalid digits, cannot convert to Gmp.mpz.\n");

    if(neg)
      mpz_neg(tmp, tmp);
  }
  else if(base == 256)
  {
    if (digits->size_shift)
      Pike_error("Invalid digits, cannot convert to Gmp.mpz.\n");

#ifdef HAVE_MPZ_IMPORT
    mpz_import (tmp, digits->len, 1, 1, 0, 0, digits->str);
#else
    {
      int i;
      mpz_t digit;
    
      mpz_init(digit);
      mpz_set_ui(tmp, 0);
      for (i = 0; i < digits->len; i++)
      {
	mpz_set_ui(digit, EXTRACT_UCHAR(digits->str + i));
	mpz_mul_2exp(digit, digit,
		     DO_NOT_WARN((unsigned long)(digits->len - i - 1) * 8));
	mpz_ior(tmp, tmp, digit);
      }
      mpz_clear(digit);
    }
#endif
  }
  else
  {
    Pike_error("Invalid base.\n");
  }
}

int get_new_mpz(MP_INT *tmp, struct svalue *s,
		int throw_error, const char *arg_func, int arg, int args)
{
  switch(s->type)
  {
  case T_INT:
#ifndef BIG_PIKE_INT
    PIKE_MPZ_SET_SI(tmp, (signed long int) s->u.integer);
#else
    {
      INT_TYPE i = s->u.integer;
      int neg = i < 0;
      if (neg) i = -i;

#ifdef HAVE_MPZ_IMPORT
      mpz_import (tmp, 1, 1, SIZEOF_INT_TYPE, 0, 0, &i);
#else
      {
	size_t n =
	  ((SIZEOF_INT_TYPE + SIZEOF_LONG - 1) / SIZEOF_LONG - 1)
	  /* The above is the position of the top unsigned long in the INT_TYPE. */
	  * ULONG_BITS;
	mpz_set_ui (tmp, (unsigned long) (i >> n));
	while (n) {
	  n -= ULONG_BITS;
	  mpz_mul_2exp (tmp, tmp, ULONG_BITS);
	  mpz_add_ui (tmp, tmp, (unsigned long) (i >> n));
	}
      }
#endif

      if (neg) mpz_neg (tmp, tmp);
    }
#endif
    break;
    
  case T_FLOAT:
    mpz_set_d(tmp, (double) s->u.float_number);
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
	 || (ITEM(s->u.array)[0].type != T_STRING)
	 || (ITEM(s->u.array)[1].type != T_INT))
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
    s->u.object=o;
    s->type=T_OBJECT;
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
 *! @decl void create(string value, int(2..36)|int(256..256) base)
 *!
 *!   Create and initialize a @[Gmp.mpz] object.
 *!
 *! @param value
 *!   Initial value. If no value is specified, the object will be initialized
 *!   to zero.
 *!
 *! @param base
 *!   Base the value is specified in. The default base is base 10.
 *!   The base can be either a value in the range @tt{[2..36]@} (inclusive),
 *!   in which case the numbers are taken from the ASCII range
 *!   @tt{0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ@} (case-insensitive),
 *!   or the value 256, in which case @[value] is taken to be the binary
 *!   representation in network byte order.
 *!
 *! @note
 *!   Leading zeroes in @[value] are not significant. In particular leading
 *!   NUL characters are not preserved in base 256 mode.
 */
static void mpzmod_create(INT32 args)
{
  switch(args)
  {
  case 1:
    if(sp[-args].type == T_STRING)
      get_mpz_from_digits(THIS, sp[-args].u.string, 0);
    else
      get_new_mpz(THIS, sp-args, 1, "Gmp.mpz", 1, args);
    break;

  case 2: /* Args are string of digits and integer base */
    if(sp[-args].type != T_STRING)
      SIMPLE_ARG_TYPE_ERROR ("Gmp.mpz", 1, "string");

    if (sp[1-args].type != T_INT)
      SIMPLE_ARG_TYPE_ERROR ("Gmp.mpz", 2, "int");

    get_mpz_from_digits(THIS, sp[-args].u.string, sp[1-args].u.integer);
    break;

  case 0:
    break;	/* Needed by AIX cc */
  }
  pop_n_elems(args);
}

/*! @decl int cast_to_int()
 */
static void mpzmod_get_int(INT32 args)
{
  pop_n_elems(args);
#ifdef AUTO_BIGNUM
  add_ref(fp->current_object);
  mpzmod_reduce(fp->current_object);
#else
  push_int(mpz_get_si(THIS));
#endif /* AUTO_BIGNUM */
}

/*! @decl int __hash()
 */
static void mpzmod___hash(INT32 args)
{
  pop_n_elems(args);
  push_int(mpz_get_si(THIS));
}

/*! @decl float cast_to_float()
 */
static void mpzmod_get_float(INT32 args)
{
  pop_n_elems(args);
  push_float((FLOAT_TYPE)mpz_get_d(THIS));
}

struct pike_string *low_get_mpz_digits(MP_INT *mpz, int base)
{
  struct pike_string *s = 0;   /* Make gcc happy. */
  ptrdiff_t len;
  
  if ( (base >= 2) && (base <= 36))
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
  else if (base == 256)
  {
    size_t i;

    if (mpz_sgn(mpz) < 0)
      Pike_error("Only non-negative numbers can be converted to base 256.\n");

    /* lets optimize this /Mirar & Per */

    /* len = mpz_size(mpz)*sizeof(mp_limb_t); */
    /* This function should not return any leading zeros. /Nisse */
    len = (mpz_sizeinbase(mpz, 2) + 7) / 8;
    s = begin_shared_string(len);

    if (!mpz_size (mpz))
    {
      /* Zero is a special case. There are no limbs at all, but
       * the size is still 1 bit, and one digit should be produced. */
      if (len != 1)
	Pike_fatal("mpz->low_get_mpz_digits: strange mpz state!\n");
      s->str[0] = 0;
    } else {
#if GMP_NUMB_BITS != SIZEOF_MP_LIMB_T * CHAR_BIT
#error Cannot cope with GMP using nail bits.
#endif
      size_t pos = 0;
      unsigned char *dst = (unsigned char *)s->str+s->len;

      while (len > 0)
      {
	mp_limb_t x = MPZ_GETLIMBN (mpz, pos++);
	for (i=0; i<sizeof(mp_limb_t); i++)
	{
	  *(--dst) = DO_NOT_WARN((unsigned char)(x & 0xff));
	  x >>= 8;
	  if (!--len)
	    break;
	  
	}
      }
    }
    s = end_shared_string(s);
  }
  else
  {
    Pike_error("Invalid base.\n");
    return 0; /* Make GCC happy */
  }

  return s;
}

/*! @decl string cast_to_string()
 */
static void mpzmod_get_string(INT32 args)
{
  pop_n_elems(args);
  push_string(low_get_mpz_digits(THIS, 10));
}

/*! @decl string digits(void|int(2..36)|int(256..256) base)
 *! This function converts an mpz to a string. If a @[base] is given the
 *! number will be represented in that base. Valid bases are 2-36 and
 *! 256. The default base is 10.
 *! @seealso
 *!   @[cast_to_string]
 */
static void mpzmod_digits(INT32 args)
{
  INT32 base;
  struct pike_string *s;
  
  if (!args)
  {
    base = 10;
  }
  else
  {
    if (sp[-args].type != T_INT)
      SIMPLE_ARG_TYPE_ERROR ("Gmp.mpz->digits", 1, "int");
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
  INT_TYPE precision, width, width_undecided, base = 0, mask_shift = 0;
  struct pike_string *s = 0;
  INT_TYPE flag_left, method;

  debug_malloc_touch(Pike_fp->current_object);
  
  if(args < 2)
    SIMPLE_TOO_FEW_ARGS_ERROR("Gmp.mpz->_sprintf", 2);
  if(sp[-args].type != T_INT)
    SIMPLE_ARG_TYPE_ERROR ("Gmp.mpz->_sprintf", 1, "int");
  if(sp[1-args].type != T_MAPPING)
    SIMPLE_ARG_TYPE_ERROR ("Gmp.mpz->_sprintf", 2, "mapping");

  push_svalue(&sp[1-args]);
  push_constant_text("precision");
  f_index(2);
  if(sp[-1].type != T_INT)
    SIMPLE_ARG_ERROR ("Gmp.mpz->_sprintf", 2,
		      "The field \"precision\" doesn't hold an integer.");
  precision = (--sp)->u.integer;
  
  push_svalue(&sp[1-args]);
  push_constant_text("width");
  f_index(2);
  if(sp[-1].type != T_INT)
    SIMPLE_ARG_ERROR ("Gmp.mpz->_sprintf", 2,
		      "The field \"width\" doesn't hold an integer.");
  width_undecided = ((sp-1)->subtype != NUMBER_NUMBER);
  width = (--sp)->u.integer;

  push_svalue(&sp[1-args]);
  push_constant_text("flag_left");
  f_index(2);
  if(sp[-1].type != T_INT)
    SIMPLE_ARG_ERROR ("Gmp.mpz->_sprintf", 2,
		      "The field \"flag_left\" doesn't hold an integer.");
  flag_left=sp[-1].u.integer;
  pop_stack();

  debug_malloc_touch(Pike_fp->current_object);

  switch(method = sp[-args].u.integer)
  {
#ifdef AUTO_BIGNUM
    case 't':
      pop_n_elems(args);
      if(THIS_PROGRAM == bignum_program)
	push_constant_text("int");
      else
	push_constant_text("object");
      return;
#endif

  case 'O':
#ifdef AUTO_BIGNUM
    if (THIS_PROGRAM == mpzmod_program) {
#endif
      push_constant_text ("Gmp.mpz(");
      push_string (low_get_mpz_digits (THIS, 10));
      push_constant_text (")");
      f_add (3);
      s = (--sp)->u.string;
      break;
#ifdef AUTO_BIGNUM
    }
#endif
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
#if GMP_NUMB_BITS != SIZEOF_MP_LIMB_T * CHAR_BIT
#error Cannot cope with GMP using nail bits.
#endif

      mp_limb_t x = (length-->0? MPZ_GETLIMBN(n, pos++) : 0);

      if (!flag_left)
	 for(i = 0; i < (INT_TYPE)sizeof(mp_limb_t); i++)
	 {
	    *(--dst) = DO_NOT_WARN((unsigned char)((neg ? ~x : x) & 0xff));
	    x >>= 8;
	    if(!--width)
	       break;
	 }
      else
	 for(i = 0; i < (INT_TYPE)sizeof(mp_limb_t); i++)
	 {
	    *(dst++) = DO_NOT_WARN((unsigned char)((neg ? ~x : x) & 0xff));
	    x >>= 8;
	    if(!--width)
	       break;
	 }
    }
    
    if(neg)
    {
      mpz_clear(tmp);
    }
    
    s = end_shared_string(s);
  }
  break;
  }

  debug_malloc_touch(Pike_fp->current_object);

  pop_n_elems(args);

  if(s) {
    push_string(s);
    if (method == 'X') {
      f_upper_case(1);
    }
  } else {
    push_int(0);   /* Push false? */
    sp[-1].subtype = 1;
  }
}

/*! @decl int(0..1) _is_type(string type)
 */
static void mpzmod__is_type(INT32 args)
{
  if(args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("Gmp.mpz->_is_type", 1);
  if(sp[-args].type != T_STRING)
    SIMPLE_ARG_TYPE_ERROR ("Gmp.mpz->_is_type", 1, "string");

  pop_n_elems(args-1);
  push_constant_text("int");
  f_eq(2);
}

/*! @decl int size(void|int base)
 *! This function returns how long the mpz would be represented in the
 *! specified @[base]. The default base is 2.
 */
static void mpzmod_size(INT32 args)
{
  int base;
  if (!args)
  {
    /* Default is number of bits */
    base = 2;
  }
  else
  {
    if (sp[-args].type != T_INT)
      SIMPLE_ARG_TYPE_ERROR ("Gmp.mpz->size", 1, "int");
    base = sp[-args].u.integer;
    if ((base != 256) && ((base < 2) || (base > 36)))
      SIMPLE_ARG_ERROR ("Gmp.mpz->size", 1, "Invalid base.");
  }
  pop_n_elems(args);

  if (base == 256)
    push_int(DO_NOT_WARN((INT32)((mpz_sizeinbase(THIS, 2) + 7) / 8)));
  else
    push_int(DO_NOT_WARN((INT32)(mpz_sizeinbase(THIS, base))));
}

/*! @decl mixed cast(string type)
 *! It is possible to cast an mpz to a string, int or float.
 *! @seealso
 *!   @[cast_to_int], @[cast_to_float], @[cast_to_string]
 */
static void mpzmod_cast(INT32 args)
{
  struct pike_string *s;

  if(args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("Gmp.mpz->cast", 1);
  if(sp[-args].type != T_STRING)
    SIMPLE_ARG_TYPE_ERROR ("Gmp.mpz->cast", 1, "string");

  s = sp[-args].u.string;
  add_ref(s);

  pop_n_elems(args);

  switch(s->str[0])
  {
  case 'i':
    if(!strncmp(s->str, "int", 3))
    {
      free_string(s);
      mpzmod_get_int(0);
      return;
    }
    break;

  case 's':
    if(!strcmp(s->str, "string"))
    {
      free_string(s);
      mpzmod_get_string(0);
      return;
    }
    break;

  case 'f':
    if(!strcmp(s->str, "float"))
    {
      free_string(s);
      mpzmod_get_float(0);
      return;
    }
    break;

  case 'o':
    if(!strcmp(s->str, "object"))
    {
      push_object(this_object());
    }
    break;

  case 'm':
    if(!strcmp(s->str, "mixed"))
    {
      push_object(this_object());
    }
    break;
    
  }

  push_string(s);	/* To get it freed when Pike_error() pops the stack. */

  SIMPLE_ARG_ERROR ("Gmp.mpz->cast", 1,
		    "Cannot cast to other type than int, string or float.");
}

/* Non-reentrant */
#if 0
/* These two functions are here so we can allocate temporary
 * objects without having to worry about them leaking in
 * case of errors..
 */
static struct object *temporary;
MP_INT *get_tmp(void)
{
  if(!temporary)
    temporary=clone_object(mpzmod_program,0);

  return (MP_INT *)temporary->storage;
}

static void return_temporary(INT32 args)
{
  pop_n_elems(args);
  push_object(temporary);
  temporary=0;
}
#endif

#ifdef AUTO_BIGNUM

#define DO_IF_AUTO_BIGNUM(X) X
double double_from_sval(struct svalue *s)
{
  switch(s->type)
  {
    case T_INT: return (double)s->u.integer;
    case T_FLOAT: return (double)s->u.float_number;
    case T_OBJECT: 
      if(IS_MPZ_OBJ (s->u.object))
	return mpz_get_d(OBTOMPZ(s->u.object));
    default:
      Pike_error("Bad argument, expected a number of some sort.\n");
  }
  /* NOT_REACHED */
  return (double)0.0;	/* Keep the compiler happy. */
}

#else
#define DO_IF_AUTO_BIGNUM(X)
#endif

#define BINFUN2(name, errmsg_op, fun, OP)				\
static void name(INT32 args)						\
{									\
  INT32 e;								\
  struct object *res;							\
  DO_IF_AUTO_BIGNUM(                                                    \
  if(THIS_PROGRAM == bignum_program)					\
  {									\
    double ret;								\
    for(e=0; e<args; e++)						\
    {									\
      switch(sp[e-args].type)						\
      {									\
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
        MEMMOVE(sp-args+1, sp-args, sizeof(struct svalue)*args);        \
        sp++; args++;                                                   \
        sp[-args].type=T_INT;                                           \
        sp[-args].u.string=low_get_mpz_digits(THIS, 10);                \
        sp[-args].type=T_STRING;                                        \
        f_add(args);                                                    \
	return; )							\
      }									\
    }									\
  } )									\
  for(e=0; e<args; e++)							\
    if(sp[e-args].type != T_INT || !FITS_ULONG (sp[e-args].u.integer))	\
     get_mpz(sp+e-args, 1, "Gmp.mpz->`" errmsg_op, e + 1, args);	\
  res = fast_clone_object(THIS_PROGRAM);				\
  mpz_set(OBTOMPZ(res), THIS);						\
  for(e=0;e<args;e++)							\
    if(sp[e-args].type != T_INT)					\
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
  INT32 e;								\
  struct object *res;							\
  DO_IF_AUTO_BIGNUM(                                                    \
  if(THIS_PROGRAM == bignum_program)					\
  {									\
    double ret;								\
    for(e=0; e<args; e++)						\
    {									\
      switch(sp[e-args].type)						\
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
  } )									\
  for(e=0; e<args; e++)							\
    if(sp[e-args].type != T_INT || !FITS_ULONG (sp[e-args].u.integer))	\
     get_mpz(sp+e-args, 1, "Gmp.mpz->``" errmsg_op, e + 1, args);	\
  res = fast_clone_object(THIS_PROGRAM);				\
  mpz_set(OBTOMPZ(res), THIS);						\
  for(e=0;e<args;e++)							\
    if(sp[e-args].type != T_INT)					\
      fun(OBTOMPZ(res), OBTOMPZ(res), OBTOMPZ(sp[e-args].u.object));	\
    else								\
      PIKE_CONCAT(fun,_ui)(OBTOMPZ(res), OBTOMPZ(res),			\
                           sp[e-args].u.integer);			\
									\
  pop_n_elems(args);							\
  PUSH_REDUCED(res);							\
}									\
)									\
									\
static void PIKE_CONCAT(name,_eq)(INT32 args)				\
{									\
  INT32 e;								\
  DO_IF_AUTO_BIGNUM(                                                    \
  if(THIS_PROGRAM == bignum_program)					\
  {									\
    double ret;								\
    for(e=0; e<args; e++)						\
    {									\
      switch(sp[e-args].type)						\
      {									\
       case T_FLOAT:                                                    \
	ret=mpz_get_d(THIS);						\
	for(e=0; e<args; e++)						\
	  ret = ret OP double_from_sval(sp-args);	        	\
									\
	pop_n_elems(args);						\
	push_float( (FLOAT_TYPE)ret );					\
	return;								\
       case T_STRING:                                                   \
        MEMMOVE(sp-args+1, sp-args, sizeof(struct svalue)*args);        \
        sp++; args++;                                                   \
        sp[-args].type=T_INT;                                           \
        sp[-args].u.string=low_get_mpz_digits(THIS, 10);                \
        sp[-args].type=T_STRING;                                        \
        f_add(args);                                                    \
	return;								\
      }									\
    }									\
  } )									\
  for(e=0; e<args; e++)							\
    if(sp[e-args].type != T_INT || !FITS_ULONG (sp[e-args].u.integer))	\
     get_mpz(sp+e-args, 1, "Gmp.mpz->`" errmsg_op "=", e + 1, args);	\
  for(e=0;e<args;e++)							\
    if(sp[e-args].type != T_INT)					\
      fun(THIS, THIS, OBTOMPZ(sp[e-args].u.object));			\
    else								\
      PIKE_CONCAT(fun,_ui)(THIS,THIS, sp[e-args].u.integer);		\
  add_ref(fp->current_object);						\
  PUSH_REDUCED(fp->current_object);					\
}

#define STRINGCONV(X) X

/*! @decl Gmp.mpz `+(int|float|Gmp.mpz ... x)
 */
/*! @decl Gmp.mpz ``+(int|float|Gmp.mpz ... x)
 */
/*! @decl Gmp.mpz `+=(int|float|Gmp.mpz ... x)
 */
BINFUN2(mpzmod_add, "+", mpz_add, +);

#undef STRINGCONV
#define STRINGCONV(X)

/*! @decl Gmp.mpz `*(int|float|Gmp.mpz ... x)
 */
/*! @decl Gmp.mpz ``*(int|float|Gmp.mpz ... x)
 */
/*! @decl Gmp.mpz `*=(int|float|Gmp.mpz ... x)
 */
BINFUN2(mpzmod_mul, "*", mpz_mul, *);

/*! @decl Gmp.mpz gcd(object|int|float|string arg)
 *! This function returns the greatest common divisor for @[arg] and mpz.
 */
static void mpzmod_gcd(INT32 args)
{
  INT32 e;
  struct object *res;
  for(e=0; e<args; e++)
   if(sp[e-args].type != T_INT || sp[e-args].u.integer<=0)
     get_mpz(sp+e-args, 1, "Gmp.mpz->gcd", e + 1, args);
  res = fast_clone_object(THIS_PROGRAM);
  mpz_set(OBTOMPZ(res), THIS);
  for(e=0;e<args;e++)
    if(sp[e-args].type != T_INT)
     mpz_gcd(OBTOMPZ(res), OBTOMPZ(res), OBTOMPZ(sp[e-args].u.object));
    else
      mpz_gcd_ui(OBTOMPZ(res), OBTOMPZ(res),sp[e-args].u.integer);

  pop_n_elems(args);
  PUSH_REDUCED(res);
}

/*! @decl Gmp.mpz `-(int|float|Gmp.mpz ... x)
 */
static void mpzmod_sub(INT32 args)
{
  INT32 e;
  struct object *res;

  if (args)
    for (e = 0; e<args; e++)
      get_mpz(sp + e - args, 1, "Gmp.mpz->`-", e + 1, args);
  
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

/*! @decl Gmp.mpz ``-(int|float|Gmp.mpz ... x)
 */
static void mpzmod_rsub(INT32 args)
{
  struct object *res = NULL;
  MP_INT *a;
  
  if(args!=1)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("Gmp.mpz->``-", 1);

  a=get_mpz(sp-1, 1, "Gmp.mpz->``-", 1, 1);
  
  res = fast_clone_object(THIS_PROGRAM);

  mpz_sub(OBTOMPZ(res), a, THIS);
  pop_n_elems(args);
  PUSH_REDUCED(res);
}

/*! @decl Gmp.mpz `/(int|float|Gmp.mpz ... x)
 */
static void mpzmod_div(INT32 args)
{
  INT32 e;
  struct object *res;
  
  for(e=0;e<args;e++)
  {
    if(sp[e-args].type != T_INT || sp[e-args].u.integer<=0)
      if (!mpz_sgn(get_mpz(sp+e-args, 1, "Gmp.mpz->`/", e + 1, args)))
	SIMPLE_DIVISION_BY_ZERO_ERROR ("Gmp.mpz->`/");
  }
  
  res = fast_clone_object(THIS_PROGRAM);
  mpz_set(OBTOMPZ(res), THIS);
  for(e=0;e<args;e++)	
  {
    if(sp[e-args].type == T_INT)
#ifdef BIG_PIKE_INT
    {
       INT_TYPE i=sp[e-args].u.integer;
       if ( (unsigned long int)i == i)
       {
	  mpz_fdiv_q_ui(OBTOMPZ(res), OBTOMPZ(res), i);
       }
       else
       {
	  MP_INT *tmp=get_mpz(sp+e-args,1,"Gmp.mpz->`/",e,e);
	  mpz_fdiv_q(OBTOMPZ(res), OBTOMPZ(res), tmp);
/* will this leak? there is no simple way of poking at the references to tmp */
       }
    }
#else
      mpz_fdiv_q_ui(OBTOMPZ(res), OBTOMPZ(res), sp[e-args].u.integer);
#endif
   else
      mpz_fdiv_q(OBTOMPZ(res), OBTOMPZ(res), OBTOMPZ(sp[e-args].u.object));
  }

  pop_n_elems(args);
  PUSH_REDUCED(res);
}

/*! @decl Gmp.mpz ``/(int|float|Gmp.mpz ... x)
 */
static void mpzmod_rdiv(INT32 args)
{
  MP_INT *a;
  struct object *res = NULL;

  if(args!=1)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("Gmp.mpz->``/", 1);

  if(!mpz_sgn(THIS))
    SIMPLE_DIVISION_BY_ZERO_ERROR ("Gmp.mpz->``/");

  a=get_mpz(sp-1, 1, "Gmp.mpz->``/", 1, 1);
  
  res=fast_clone_object(THIS_PROGRAM);
  mpz_fdiv_q(OBTOMPZ(res), a, THIS);
  pop_n_elems(args);
  PUSH_REDUCED(res);
}

/*! @decl Gmp.mpz `%(int|float|Gmp.mpz ... x)
 */
static void mpzmod_mod(INT32 args)
{
  INT32 e;
  struct object *res;
  
  for(e=0;e<args;e++)
    if (!mpz_sgn(get_mpz(sp+e-args, 1, "Gmp.mpz->`%", e + 1, args)))
      SIMPLE_DIVISION_BY_ZERO_ERROR ("Gmp.mpz->`%");

  res = fast_clone_object(THIS_PROGRAM);
  mpz_set(OBTOMPZ(res), THIS);
  for(e=0;e<args;e++)	
    mpz_fdiv_r(OBTOMPZ(res), OBTOMPZ(res), OBTOMPZ(sp[e-args].u.object));

  pop_n_elems(args);
  PUSH_REDUCED(res);
}

/*! @decl Gmp.mpz ``%(int|float|Gmp.mpz ... x)
 */
static void mpzmod_rmod(INT32 args)
{
  MP_INT *a;
  struct object *res = NULL;

  if(args!=1)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("Gmp.mpz->``%", 1);

  if(!mpz_sgn(THIS))
    SIMPLE_DIVISION_BY_ZERO_ERROR ("Gmp.mpz->``%");

  a=get_mpz(sp-1, 1, "Gmp.mpz->``%", 1, 1);
  
  res=fast_clone_object(THIS_PROGRAM);
  mpz_fdiv_r(OBTOMPZ(res), a, THIS);
  pop_n_elems(args);
  PUSH_REDUCED(res);
}

/*! @decl array(Gmp.mpz) gcdext(int|float|Gmp.mpz x)
 */
static void mpzmod_gcdext(INT32 args)
{
  struct object *g, *s, *t;
  MP_INT *a;

  if (args != 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("Gmp.mpz->gcdext", 1);

  a = get_mpz(sp-1, 1, "Gmp.mpz->gcdext", 1, 1);
  
  g = fast_clone_object(THIS_PROGRAM);
  s = fast_clone_object(THIS_PROGRAM);
  t = fast_clone_object(THIS_PROGRAM);

  mpz_gcdext(OBTOMPZ(g), OBTOMPZ(s), OBTOMPZ(t), THIS, a);
  pop_n_elems(args);
  PUSH_REDUCED(g); PUSH_REDUCED(s); PUSH_REDUCED(t);
  f_aggregate(3);
}

/*! @decl array(Gmp.mpz) gcdext2(int|float|Gmp.mpz x)
 */
static void mpzmod_gcdext2(INT32 args)
{
  struct object *g, *s;
  MP_INT *a;

  if (args != 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("Gmp.mpz->gcdext2", 1);

  a = get_mpz(sp-args, 1, "Gmp.mpz->gcdext2", 1, 1);
  
  g = fast_clone_object(THIS_PROGRAM);
  s = fast_clone_object(THIS_PROGRAM);

  mpz_gcdext(OBTOMPZ(g), OBTOMPZ(s), NULL, THIS, a);
  pop_n_elems(args);
  PUSH_REDUCED(g); PUSH_REDUCED(s); 
  f_aggregate(2);
}

/*! @decl Gmp.mpz invert(int|float|Gmp.mpz x)
 */
static void mpzmod_invert(INT32 args)
{
  MP_INT *modulo;
  struct object *res;

  if (args != 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("Gmp.mpz->invert", 1);
  modulo = get_mpz(sp-args, 1, "Gmp.mpz->invert", 1, 1);

  if (!mpz_sgn(modulo))
    SIMPLE_DIVISION_BY_ZERO_ERROR ("Gmp.mpz->invert");
  res = fast_clone_object(THIS_PROGRAM);
  if (mpz_invert(OBTOMPZ(res), THIS, modulo) == 0)
  {
    free_object(res);
    Pike_error("Not invertible.\n");
  }
  pop_n_elems(args);
  PUSH_REDUCED(res);
}

#define BINFUN(name, errmsg_name, fun)			\
static void name(INT32 args)				\
{							\
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

/*! @decl Gmp.mpz `&(int|float|Gmp.mpz ... x)
 */
BINFUN(mpzmod_and, "Gmp.mpz->`&", mpz_and);

/*! @decl Gmp.mpz `|(int|float|Gmp.mpz ... x)
 */
BINFUN(mpzmod_or, "Gmp.mpz->`|", mpz_ior);

/*! @decl Gmp.mpz `^(int|float|Gmp.mpz ... x)
 */
BINFUN(mpzmod_xor, "Gmp.mpz->`^", mpz_xor);

/*! @decl Gmp.mpz `~()
 */
static void mpzmod_compl(INT32 args)
{
  struct object *o;
  pop_n_elems(args);
  o=fast_clone_object(THIS_PROGRAM);
  mpz_com(OBTOMPZ(o), THIS);
  PUSH_REDUCED(o);
}

#define CMPEQU(name,errmsg_name,cmp,default)		\
static void name(INT32 args)				\
{							\
  INT32 i;						\
  MP_INT *arg;						\
  if(!args) SIMPLE_TOO_FEW_ARGS_ERROR (errmsg_name, 1);	\
  if (!(arg = get_mpz(sp-args, 0, NULL, 0, 0)))		\
    default;						\
  else							\
    i=mpz_cmp(THIS, arg) cmp 0;				\
  pop_n_elems(args);					\
  push_int(i);						\
}

#define RET_UNDEFINED do{pop_n_elems(args);push_undefined();return;}while(0)

/*! @decl int(0..1) `>(mixed with)
 */
CMPEQU(mpzmod_gt, "Gmp.mpz->`>", >, RET_UNDEFINED);

/*! @decl int(0..1) `<(mixed with)
 */
CMPEQU(mpzmod_lt, "Gmp.mpz->`<", <, RET_UNDEFINED);

/*! @decl int(0..1) `>=(mixed with)
 */
CMPEQU(mpzmod_ge, "Gmp.mpz->`>=", >=, RET_UNDEFINED);

/*! @decl int(0..1) `<=(mixed with)
 */
CMPEQU(mpzmod_le, "Gmp.mpz->`<=", <=, RET_UNDEFINED);

/*! @decl int(0..1) `==(mixed with)
 */
CMPEQU(mpzmod_eq, "Gmp.mpz->`==", ==, RET_UNDEFINED);

/*! @decl int(0..1) `!=(mixed with)
 */
CMPEQU(mpzmod_nq, "Gmp.mpz->`!=", !=, i=1);

/*! @decl int(0..1) probably_prime_p()
 *! This function returns 1 if mpz is a prime, and 0 most of the time
 *! if it is not.
 */
static void mpzmod_probably_prime_p(INT32 args)
{
  INT_TYPE count;
  if (args)
  {
    if (sp[-args].type != T_INT)
      SIMPLE_ARG_TYPE_ERROR ("Gmp.mpz->probably_prime_p", 1, "int");
    count = sp[-args].u.integer;
    if (count <= 0)
      SIMPLE_ARG_ERROR ("Gmp.mpz->probably_prime_p", 1,
			"The count must be positive.");
  } else
    count = 25;
  pop_n_elems(args);
  push_int(mpz_probab_prime_p(THIS, count));
}

/*! @decl int small_factor(void|int(1..) limit)
 */
static void mpzmod_small_factor(INT32 args)
{
  INT_TYPE limit;

  if (args)
    {
      if (sp[-args].type != T_INT)
	SIMPLE_ARG_TYPE_ERROR ("Gmp.mpz->small_factor", 1, "int");
      limit = sp[-args].u.integer;
      if (limit < 1)
	SIMPLE_ARG_ERROR ("Gmp.mpz->small_factor", 1,
			  "The limit must be at least 1.");
    }
  else
    limit = INT_MAX;
  pop_n_elems(args);
  push_int(mpz_small_factor(THIS, limit));
}

/*! @decl Gmp.mpz next_prime(void|int count, void|int limit)
 */
static void mpzmod_next_prime(INT32 args)
{
  INT_TYPE count = 25;
  INT_TYPE limit = INT_MAX;
  struct object *o;

  switch(args)
  {
  case 0:
    break;
  case 1:
    get_all_args("Gmp.mpz->next_prime", args, "%i", &count);
    break;
  default:
    get_all_args("Gmp.mpz->next_prime", args, "%i%i", &count, &limit);
    break;
  }
  pop_n_elems(args);
  
  o = fast_clone_object(THIS_PROGRAM);
  mpz_next_prime(OBTOMPZ(o), THIS, count, limit);
  
  PUSH_REDUCED(o);
}

/*! @decl int sgn()
 */
static void mpzmod_sgn(INT32 args)
{
  pop_n_elems(args);
  push_int(mpz_sgn(THIS));
}

/*! @decl Gmp.mpz sqrt()
 *! This function return the the truncated integer part of the square
 *! root of the value of mpz.
 */
static void mpzmod_sqrt(INT32 args)
{
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
  struct object *res = NULL;
  MP_INT *mi;

  if (args != 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("Gmp.mpz->`<<", 1);

  if(sp[-1].type == T_INT) {
    if(sp[-1].u.integer < 0)
      SIMPLE_ARG_ERROR ("Gmp.mpz->`<<", 1, "Got negative shift count.");
#ifdef BIG_PIKE_INT
    if (!FITS_ULONG (sp[-1].u.integer) && mpz_sgn (THIS))
      SIMPLE_ARG_ERROR ("Gmp.mpz->`<<", 1, "Shift count too large.");
#endif
    res = fast_clone_object(THIS_PROGRAM);
    mpz_mul_2exp(OBTOMPZ(res), THIS, sp[-1].u.integer);
  } else {
    mi = get_mpz(sp-1, 1, "Gmp.mpz->`<<", 1, 1);
    if(mpz_sgn(mi)<0)
      SIMPLE_ARG_ERROR ("Gmp.mpz->`<<", 1, "Got negative shift count.");
    /* Cut off at 1MB ie 0x800000 bits. */
    if(!mpz_fits_ulong_p(mi) || (mpz_get_ui(THIS) > 0x800000))
    {
       if(mpz_sgn(THIS))
	 SIMPLE_ARG_ERROR ("Gmp.mpz->`<<", 1, "Shift count too large.");
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
  struct object *res = NULL;

  if (args != 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("Gmp.mpz->`>>", 1);

  if(sp[-1].type == T_INT)
  {
     if (sp[-1].u.integer < 0)
       SIMPLE_ARG_ERROR ("Gmp.mpz->`>>", 1, "Got negative shift count.");
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
     MP_INT *mi = get_mpz(sp-1, 1, "Gmp.mpz->`>>", 1, 1);
     if(!mpz_fits_ulong_p (mi)) {
       if(mpz_sgn(mi)<0)
	 SIMPLE_ARG_ERROR ("Gmp.mpz->`>>", 1, "Got negative shift count.");
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
  struct object *res = NULL;
  MP_INT *mi;

  if (args != 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("Gmp.mpz->``<<", 1);
  if(mpz_sgn(THIS) < 0)
    Pike_error ("Gmp.mpz->``<<(): Got negative shift count.\n");

  mi = get_mpz(sp-1, 1, "Gmp.mpz->``<<", 1, 1);

  /* Cut off at 1MB ie 0x800000 bits. */
  if(!mpz_fits_ulong_p(THIS) || (mpz_get_ui(THIS) > 0x800000)) {
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
  struct object *res = NULL;
  MP_INT *mi;

  if (args != 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("Gmp.mpz->``>>", 1);
  mi = get_mpz(sp-1, 1, "Gmp.mpz->``>>", 1, 1);

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

/*! @decl Gmp.mpz powm(int|string|float|Gmp.mpz a,@
 *!                    int|string|float|Gmp.mpz b)
 *! This function returns @tt{( mpz ** a ) % b@}.
 */
static void mpzmod_powm(INT32 args)
{
  struct object *res = NULL;
  MP_INT *n, *e;
  
  if(args != 2)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("Gmp.mpz->powm", 2);

  e = get_mpz(sp - 2, 1, "Gmp.mpz->powm", 1, 2);
  n = get_mpz(sp - 1, 1, "Gmp.mpz->powm", 2, 2);

  if (!mpz_sgn(n))
    SIMPLE_DIVISION_BY_ZERO_ERROR ("Gmp.mpz->powm");
  res = fast_clone_object(THIS_PROGRAM);
  mpz_powm(OBTOMPZ(res), THIS, e, n);
  pop_n_elems(args);
  PUSH_REDUCED(res);
}

/*! @decl Gmp.mpz pow(int|float|Gmp.mpz x)
 */
static void mpzmod_pow(INT32 args)
{
  struct object *res = NULL;
  INT32 i;
  MP_INT *mi;
  
  if (args != 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("Gmp.mpz->pow", 1);
  if (sp[-1].type == T_INT) {
    if (sp[-1].u.integer < 0)
      SIMPLE_ARG_ERROR ("Gmp.mpz->pow", 1, "Negative exponent.");
    /* Cut off at 1 MB. */
    if ((mpz_size(THIS)*sp[-1].u.integer>(0x100000/sizeof(mp_limb_t))))
      if(mpz_cmp_si(THIS, -1)<0 || mpz_cmp_si(THIS, 1)>0)
	 goto too_large;
    res = fast_clone_object(THIS_PROGRAM);
    mpz_pow_ui(OBTOMPZ(res), THIS, sp[-1].u.integer);
  } else {
too_large:
    mi = get_mpz(sp-1, 1, "Gmp.mpz->pow", 1, 1);
    if(mpz_sgn(mi)<0)
      SIMPLE_ARG_ERROR ("Gmp.mpz->pow", 1, "Negative exponent.");
    i=mpz_get_si(mi);
    /* Cut off at 1 MB. */
    if(mpz_cmp_si(mi, i) ||
       (mpz_size(THIS)*i>(0x100000/sizeof(mp_limb_t))))
    {
       if(mpz_cmp_si(THIS, -1)<0 || mpz_cmp_si(THIS, 1)>0)
	 SIMPLE_ARG_ERROR ("Gmp.mpz->pow", 1, "Exponent too large.");
       else {
    /* Special case: these three integers can be raised to any power
       without overflowing.						 */
	  res = fast_clone_object(THIS_PROGRAM);
	  switch(mpz_get_si(THIS)) {
	     case 0:
		mpz_set_si(OBTOMPZ(res), 0);
		break;
	     case 1:
		mpz_set_si(OBTOMPZ(res), 1);
		break;
	     case -1:
		mpz_set_si(OBTOMPZ(res), mpz_odd_p(mi)? -1:1);
		break;
	  }
       }
    }
    else {
      res = fast_clone_object(THIS_PROGRAM);
      mpz_pow_ui(OBTOMPZ(res), THIS, i);
    }
  }
  pop_n_elems(args);
  PUSH_REDUCED(res);
}

/*! @decl int(0..1) `!()
 */
static void mpzmod_not(INT32 args)
{
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
  pop_n_elems(args);
#ifdef HAVE_MPZ_POPCOUNT
  push_int(mpz_popcount(THIS));  
#ifdef BIG_PIKE_INT
/* need conversion from MAXUINT32 to -1 (otherwise it's done already) */
  if (Pike_sp[-1].u.integer==0xffffffffLL)
     Pike_sp[-1].u.integer=-1;
#endif
#else
  switch (mpz_sgn(THIS))
  {
  case 0:
    push_int(0);
    break;
  case -1:
    push_int(-1);
    break;
  case 1:
    push_int(mpn_popcount(THIS->_mp_d, THIS->_mp_size));
#ifdef BIG_PIKE_INT
    if (Pike_sp[-1].u.integer==0xffffffffLL)
       Pike_sp[-1].u.integer=-1;
#endif
    break;
  default:
#ifdef PIKE_DEBUG
    Pike_fatal("Gmp.mpz->popcount: Unexpected sign!\n");
#endif
  }
#endif
}

/*! @decl Gmp.mpz _random()
 */
static void mpzmod_random(INT32 args)
{
  struct object *res = 0;   /* Make gcc happy. */
  pop_n_elems(args);
  args = 0;
  if(mpz_sgn(THIS) <= 0)
    Pike_error("Random on negative number.\n");

  push_object(res=fast_clone_object(THIS_PROGRAM));
  /* We add four to assure reasonably uniform randomness */
  push_int(mpz_size(THIS)*sizeof(mp_limb_t) + 4);
  f_random_string(1);
  if (sp[-1].type != T_STRING) {
    Pike_error("random_string(%d) returned non string.\n",
	       mpz_size(THIS)*sizeof(mp_limb_t) + 4);
  }
  get_mpz_from_digits(OBTOMPZ(res), sp[-1].u.string, 256);
  pop_stack();
  mpz_fdiv_r(OBTOMPZ(res), OBTOMPZ(res), THIS); /* modulo */
  Pike_sp--;
  dmalloc_touch_svalue(Pike_sp);
  PUSH_REDUCED(res);
}
  
/*! @endclass
 */

/*! @decl Gmp.mpz fac(int x)
 *! Returns the factorial of @[x] (@[x]!).
 */
static void gmp_fac(INT32 args)
{
  struct object *res = NULL;
  if (args != 1)
    Pike_error("Gmp.fac: Wrong number of arguments.\n");
  if (sp[-1].type != T_INT)
    SIMPLE_ARG_TYPE_ERROR ("Gmp.fac", 1, "int");
  if (sp[-1].u.integer < 0)
    SIMPLE_ARG_ERROR ("Gmp.fac", 1, "Got negative exponent.");
  res = fast_clone_object(mpzmod_program);
  mpz_fac_ui(OBTOMPZ(res), sp[-1].u.integer);
  pop_n_elems(args);
  PUSH_REDUCED(res);
}

static void init_mpz_glue(struct object *o)
{
#ifdef PIKE_DEBUG
  if(!fp) Pike_fatal("ZERO FP\n");
  if(!THIS) Pike_fatal("ZERO THIS\n");
#endif
  mpz_init(THIS);
}

static void exit_mpz_glue(struct object *o)
{
#ifdef PIKE_DEBUG
  if(!fp) Pike_fatal("ZERO FP\n");
  if(!THIS) Pike_fatal("ZERO THIS\n");
#endif
  mpz_clear(THIS);
}
#endif

PIKE_MODULE_EXIT
{
  pike_exit_mpf_module();
  pike_exit_mpq_module();
#if defined(USE_GMP) || defined(USE_GMP2)
  if(mpzmod_program)
  {
    free_program(mpzmod_program);
    mpzmod_program=0;
  }
#ifdef AUTO_BIGNUM
  {
    extern struct svalue auto_bignum_program;
    free_svalue(&auto_bignum_program);
    auto_bignum_program.type=T_INT;
    if(bignum_program)
    {
      free_program(bignum_program);
      bignum_program=0;
    }
    mpz_clear (mpz_int_type_min);
#ifdef INT64
    mpz_clear (mpz_int64_min);
    hook_in_int64_funcs (NULL, NULL, NULL);
#endif
  }
#endif
#endif
}


#define tMpz_arg tOr3(tInt,tFloat,tObj)
#define tMpz_ret tObjIs_GMP_MPZ
#define tMpz_shift_type tFunc(tMpz_arg,tMpz_ret)
#define tMpz_binop_type tFuncV(tNone, tMpz_arg, tMpz_ret)
#define tMpz_cmpop_type tFunc(tMixed, tInt01)

#define MPZ_DEFS()							\
  ADD_STORAGE(MP_INT);							\
  									\
  /* function(void|string|int|float|object:void)" */			\
  /* "|function(string,int:void) */					\
  ADD_FUNCTION("create", mpzmod_create,					\
	       tOr(tFunc(tOr5(tVoid,tStr,tInt,tFlt,			\
			      tObj),tVoid),				\
		   tFunc(tStr tInt,tVoid)), ID_STATIC);			\
									\
  ADD_FUNCTION("`+",mpzmod_add,tMpz_binop_type, ID_STATIC);		\
  ADD_FUNCTION("`+=",mpzmod_add_eq,tMpz_binop_type, ID_STATIC);		\
  ADD_FUNCTION("``+",mpzmod_add_rhs,tMpz_binop_type, ID_STATIC);	\
  ADD_FUNCTION("`-",mpzmod_sub,tMpz_binop_type, ID_STATIC);		\
  ADD_FUNCTION("``-",mpzmod_rsub,tMpz_binop_type, ID_STATIC);		\
  ADD_FUNCTION("`*",mpzmod_mul,tMpz_binop_type, ID_STATIC);		\
  ADD_FUNCTION("``*",mpzmod_mul,tMpz_binop_type, ID_STATIC);		\
  ADD_FUNCTION("`*=",mpzmod_mul_eq,tMpz_binop_type, ID_STATIC);		\
  ADD_FUNCTION("`/",mpzmod_div,tMpz_binop_type, ID_STATIC);		\
  ADD_FUNCTION("``/",mpzmod_rdiv,tMpz_binop_type, ID_STATIC);		\
  ADD_FUNCTION("`%",mpzmod_mod,tMpz_binop_type, ID_STATIC);		\
  ADD_FUNCTION("``%",mpzmod_rmod,tMpz_binop_type, ID_STATIC);		\
  ADD_FUNCTION("`&",mpzmod_and,tMpz_binop_type, ID_STATIC);		\
  ADD_FUNCTION("``&",mpzmod_and,tMpz_binop_type, ID_STATIC);		\
  ADD_FUNCTION("`|",mpzmod_or,tMpz_binop_type, ID_STATIC);		\
  ADD_FUNCTION("``|",mpzmod_or,tMpz_binop_type, ID_STATIC);		\
  ADD_FUNCTION("`^",mpzmod_xor,tMpz_binop_type, ID_STATIC);		\
  ADD_FUNCTION("``^",mpzmod_xor,tMpz_binop_type, ID_STATIC);		\
  ADD_FUNCTION("`~",mpzmod_compl,tFunc(tNone,tObj), ID_STATIC);		\
									\
  ADD_FUNCTION("`<<",mpzmod_lsh,tMpz_shift_type, ID_STATIC);		\
  ADD_FUNCTION("`>>",mpzmod_rsh,tMpz_shift_type, ID_STATIC);		\
  ADD_FUNCTION("``<<",mpzmod_rlsh,tMpz_shift_type, ID_STATIC);		\
  ADD_FUNCTION("``>>",mpzmod_rrsh,tMpz_shift_type, ID_STATIC);		\
									\
  ADD_FUNCTION("`>", mpzmod_gt,tMpz_cmpop_type, ID_STATIC);		\
  ADD_FUNCTION("`<", mpzmod_lt,tMpz_cmpop_type, ID_STATIC);		\
  ADD_FUNCTION("`>=",mpzmod_ge,tMpz_cmpop_type, ID_STATIC);		\
  ADD_FUNCTION("`<=",mpzmod_le,tMpz_cmpop_type, ID_STATIC);		\
									\
  ADD_FUNCTION("`==",mpzmod_eq,tMpz_cmpop_type, ID_STATIC);		\
  ADD_FUNCTION("`!=",mpzmod_nq,tMpz_cmpop_type, ID_STATIC);		\
									\
  ADD_FUNCTION("`!",mpzmod_not,tFunc(tNone,tInt01), ID_STATIC);		\
									\
  ADD_FUNCTION("__hash",mpzmod___hash,tFunc(tNone,tInt), ID_STATIC);	\
  ADD_FUNCTION("cast",mpzmod_cast,tFunc(tStr,tMix), ID_STATIC);		\
									\
  ADD_FUNCTION("_is_type", mpzmod__is_type, tFunc(tStr,tInt01),         \
               ID_STATIC);                                              \
  									\
  ADD_FUNCTION("digits", mpzmod_digits,tFunc(tOr(tVoid,tInt),tStr), 0);	\
  ADD_FUNCTION("_sprintf", mpzmod__sprintf, tFunc(tInt tMapping,tStr),  \
               ID_STATIC);                                              \
  ADD_FUNCTION("size", mpzmod_size,tFunc(tOr(tVoid,tInt),tInt), 0);	\
									\
  ADD_FUNCTION("cast_to_int",mpzmod_get_int,tFunc(tNone,tInt),0);	\
  ADD_FUNCTION("cast_to_string",mpzmod_get_string,tFunc(tNone,tStr),0);	\
  ADD_FUNCTION("cast_to_float",mpzmod_get_float,tFunc(tNone,tFlt),0);	\
									\
  ADD_FUNCTION("probably_prime_p",mpzmod_probably_prime_p,		\
	       tFunc(tNone,tInt01),0);					\
  ADD_FUNCTION("small_factor", mpzmod_small_factor,			\
	       tFunc(tOr(tInt,tVoid),tInt), 0);				\
  ADD_FUNCTION("next_prime", mpzmod_next_prime,				\
	       tFunc(tOr(tInt,tVoid) tOr(tInt,tVoid),tMpz_ret), 0);	\
  									\
  ADD_FUNCTION("gcd",mpzmod_gcd, tMpz_binop_type, 0);			\
  ADD_FUNCTION("gcdext",mpzmod_gcdext,tFunc(tMpz_arg,tArr(tMpz_ret)),0);\
  ADD_FUNCTION("gcdext2",mpzmod_gcdext2,tFunc(tMpz_arg,tArr(tMpz_ret)),0);\
  ADD_FUNCTION("invert", mpzmod_invert,tFunc(tMpz_arg,tMpz_ret),0);	\
									\
  ADD_FUNCTION("sgn", mpzmod_sgn, tFunc(tNone,tInt), 0);		\
  ADD_FUNCTION("sqrt", mpzmod_sqrt,tFunc(tNone,tMpz_ret),0);		\
  ADD_FUNCTION("_sqrt", mpzmod_sqrt,tFunc(tNone,tMpz_ret),0);		\
  ADD_FUNCTION("sqrtrem",mpzmod_sqrtrem,tFunc(tNone,tArr(tMpz_ret)),0);\
  ADD_FUNCTION("powm",mpzmod_powm,tFunc(tMpz_arg tMpz_arg,tMpz_ret),0);	\
  ADD_FUNCTION("pow", mpzmod_pow,tMpz_shift_type, 0);			\
									\
  ADD_FUNCTION("popcount", mpzmod_popcount,tFunc(tVoid,tInt), 0);	\
									\
  ADD_FUNCTION("_random",mpzmod_random,tFunc(tNone,tMpz_ret),0);	\
  									\
  set_init_callback(init_mpz_glue);					\
  set_exit_callback(exit_mpz_glue);


PIKE_MODULE_INIT
{
#if defined(USE_GMP) || defined(USE_GMP2)
#ifdef PIKE_DEBUG
  if (mpzmod_program) {
    fatal("Gmp.mpz initialized twice!\n");
  }
#endif /* PIKE_DEBUG */
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

  /* function(int:object) */
  ADD_FUNCTION("fac", gmp_fac,tFunc(tInt,tObj), 0);

#ifdef AUTO_BIGNUM
  {
    int id;
    extern struct svalue auto_bignum_program;

#ifdef PIKE_DEBUG
    if (bignum_program) {
      fatal("Gmp.bignum initialized twice!\n");
    }
#endif /* PIKE_DEBUG */
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

    id=add_program_constant("bignum", bignum_program=end_program(), 0);
    bignum_program->flags |= 
      PROGRAM_NO_WEAK_FREE |
      PROGRAM_NO_EXPLICIT_DESTRUCT |
      PROGRAM_CONSTANT ;

    mpz_init (mpz_int_type_min);
    mpz_setbit (mpz_int_type_min, INT_TYPE_BITS);
    mpz_neg (mpz_int_type_min, mpz_int_type_min);
    
    /* Magic hook in... */
#ifdef PIKE_DEBUG
    if (auto_bignum_program.type <= MAX_REF_TYPE) {
      Pike_fatal("Strange initial value for auto_bignum_program\n");
    }
#endif /* PIKE_DEBUG */
    free_svalue(&auto_bignum_program);
    add_ref(auto_bignum_program.u.program = bignum_program);
    auto_bignum_program.type = PIKE_T_PROGRAM;

#ifdef INT64
    mpz_init (mpz_int64_min);
    mpz_setbit (mpz_int64_min, INT64_BITS);
    mpz_neg (mpz_int64_min, mpz_int64_min);
    hook_in_int64_funcs (gmp_push_int64, gmp_int64_from_bignum,
			 gmp_reduce_stack_top_bignum);
#endif

#if 0
    /* magic /Hubbe
     * This seems to break more than it fixes though... /Hubbe
     */
    free_type(ID_FROM_INT(Pike_compiler->new_program, id)->type);
    ID_FROM_INT(Pike_compiler->new_program, id)->type=CONSTTYPE(tOr(tFunc(tOr5(tVoid,tStr,tInt,tFlt,tObj),tInt),tFunc(tStr tInt,tInt)));
#endif
  }

#endif

#endif

  pike_init_mpq_module();
  pike_init_mpf_module();
}

/*! @endmodule
 */
