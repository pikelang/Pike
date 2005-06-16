/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: my_gmp.h,v 1.24 2005/06/16 09:21:43 grubba Exp $
*/

/*
 * These functions or something similar will hopefully be included
 * with Gmp-2.1 .
 */

#ifndef MY_GMP_H_INCLUDED
#define MY_GMP_H_INCLUDED

/* Kludge for some compilers only defining __STDC__ in strict mode,
 * which leads to <gmp.h> using the wrong token concat method.
 */
#if !defined(__STDC__) && defined(HAVE_ANSI_CONCAT) && defined(PIKE_MPN_PREFIX)
#define PIKE_LOW_MPN_CONCAT(x,y)	x##y
#define PIKE_MPN_CONCAT(x,y)	PIKE_LOW_MPN_CONCAT(x,y)
#define __MPN(x)	PIKE_MPN_CONCAT(PIKE_MPN_PREFIX,x)
#endif /* !__STDC__ && HAVE_ANSI_CONCAT && PIKE_MPN_PREFIX */

#undef _PROTO
#define _PROTO(x) x

#ifdef USE_GMP2
#include <gmp2/gmp.h>
#else /* !USE_GMP2 */
#include <gmp.h>
#endif /* USE_GMP2 */

#ifdef PIKE_GMP_LIMB_BITS_INVALID
/* Attempt to repair the header file... */
#undef GMP_LIMB_BITS
#define GMP_LIMB_BITS (SIZEOF_MP_LIMB_T * CHAR_BIT)
#ifdef PIKE_GMP_NUMB_BITS
#undef GMP_NUMB_BITS
#define GMP_NUMB_BITS PIKE_GMP_NUMB_BITS
#endif /* PIKE_GMP_NUMB_BITS */
#endif /* PIKE_GMP_LIMB_BITS_INVALID */

#ifndef mpz_odd_p
#define mpz_odd_p(z)   ((int) ((z)->_mp_size != 0) & (int) (z)->_mp_d[0])
#endif

struct pike_string;

/* MPZ protos */

unsigned long mpz_small_factor(mpz_t n, int limit);

void mpz_next_prime(mpz_t p, mpz_t n, int count, int prime_limit);
void my_mpz_xor _PROTO ((mpz_ptr, mpz_srcptr, mpz_srcptr));
void get_mpz_from_digits(MP_INT *tmp,
			 struct pike_string *digits,
			 int base);
int get_new_mpz(MP_INT *tmp, struct svalue *s,
		int throw_error, const char *arg_func, int arg, int args);
MP_INT *debug_get_mpz(struct svalue *s,
		      int throw_error, const char *arg_func, int arg, int args);
void mpzmod_reduce(struct object *o);
struct pike_string *low_get_mpz_digits(MP_INT *mpz, int base);

#ifndef HAVE_MPZ_XOR
#define mpz_xor my_mpz_xor
#endif

#ifndef HAVE_MPZ_FITS_ULONG_P
#define mpz_fits_ulong_p(n) (!mpz_cmp_ui ((n), mpz_get_ui (n)))
#endif

#ifdef MPZ_GETLIMBN_WORKS
#define MPZ_GETLIMBN(MPZ, N) mpz_getlimbn(MPZ, N)
#else  /* !MPZ_GETLIMBN_WORKS */
/* In old gmp versions (at least 2.0) mpz_getlimbn doesn't understand
 * that negative numbers are stored with negative sizes, so it regards
 * all N to be outside the mantissa then. */
static inline mp_limb_t MPZ_GETLIMBN (mpz_srcptr gmp_z, mp_size_t gmp_n)
{
  if (mpz_size (gmp_z) <= gmp_n || gmp_n < 0)
    return 0;
  else
    return gmp_z->_mp_d[gmp_n];
}
#endif	/* !MPZ_GETLIMBN_WORKS */

#ifdef MPZ_SET_SI_WORKS
#define PIKE_MPZ_SET_SI(MPZ_VAL, VALUE)	mpz_set_si((MPZ_VAL), (VALUE))
#else  /* !MPZ_SET_SI_WORKS */
/* Old gmp libraries (at least 3.0.0) have a broken implementation of
 * mpz_set_si(), which handles -0x80000000 incorrectly (it results in
 * -0xffffffff80000000 if the limb size is larger than the long size)...
 */
#define PIKE_MPZ_SET_SI(MPZ_VAL, VALUE)	do {		\
    MP_INT *mpz_ = (MPZ_VAL);				\
    long val_ = (VALUE);				\
    if (val_ < 0) {					\
      mpz_set_ui(mpz_, (unsigned long) -val_);		\
      mpz_neg(mpz_, mpz_);				\
    } else {						\
      mpz_set_ui(mpz_, (unsigned long) val_);		\
    }							\
  } while(0)
#endif /* !MPZ_SET_SI_WORKS */

extern struct program *mpzmod_program;
extern struct program *mpq_program;
extern struct program *mpf_program;
#ifdef AUTO_BIGNUM
extern struct program *bignum_program;
#endif

#ifdef DEBUG_MALLOC
#define get_mpz(S, THROW_ERROR, ARG_FUNC, ARG, ARGS)			\
  ((S)->type <= MAX_REF_TYPE ? debug_malloc_touch((S)->u.object) : 0,	\
   debug_get_mpz((S), (THROW_ERROR), (ARG_FUNC), (ARG), (ARGS)))
#else
#define get_mpz debug_get_mpz 
#endif

#define MP_FLT __mpf_struct

#define OBTOMPZ(o) ((MP_INT *)(o->storage))
#define OBTOMPQ(o) ((MP_RAT *)(o->storage))
#define OBTOMPF(o) ((MP_FLT *)(o->storage))

#ifdef AUTO_BIGNUM
#define IS_MPZ_OBJ(O) ((O)->prog == bignum_program || (O)->prog == mpzmod_program)
#else
#define IS_MPZ_OBJ(O) ((O)->prog == mpzmod_program)
#endif

#ifndef GMP_NUMB_BITS
#define GMP_NUMB_BITS (SIZEOF_MP_LIMB_T * CHAR_BIT)
#endif
#ifndef GMP_NUMB_MASK
#define GMP_NUMB_MASK ((mp_limb_t) -1)
#endif

/* Bits excluding the sign bit, if any. */
#define ULONG_BITS (SIZEOF_LONG * 8)
#define INT_TYPE_BITS (SIZEOF_INT_TYPE * CHAR_BIT - 1)
#ifdef INT64
#define INT64_BITS (SIZEOF_INT64 * CHAR_BIT - 1)
#endif

#if SIZEOF_INT_TYPE > SIZEOF_LONG
/* INT_TYPE is too big to feed directly to mpz_set_si etc. */
#define BIG_PIKE_INT
#endif

#ifdef BIG_PIKE_INT
#define FITS_LONG(VAL) ((VAL) >= LONG_MIN && (VAL) <= LONG_MAX)
#define FITS_ULONG(VAL) ((VAL) >= 0 && (VAL) <= ULONG_MAX)
#else
#define FITS_LONG(VAL) 1
#define FITS_ULONG(VAL) ((VAL) >= 0)
#endif

/* MPQ protos */
void pike_init_mpq_module(void);
void pike_exit_mpq_module(void);

/* MPF protos */
void pike_init_mpf_module(void);
void pike_exit_mpf_module(void);

#endif /* MY_GMP_H_INCLUDED */
