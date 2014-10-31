/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef MY_GMP_H_INCLUDED
#define MY_GMP_H_INCLUDED

#include <gmp.h>

#ifdef PIKE_GMP_LIMB_BITS_INVALID
/* Attempt to repair the header file... */
#undef GMP_LIMB_BITS
#define GMP_LIMB_BITS (SIZEOF_MP_LIMB_T * CHAR_BIT)
#ifdef PIKE_GMP_NUMB_BITS
#undef GMP_NUMB_BITS
#define GMP_NUMB_BITS PIKE_GMP_NUMB_BITS
#endif /* PIKE_GMP_NUMB_BITS */
#endif /* PIKE_GMP_LIMB_BITS_INVALID */

struct pike_string;

/* MPZ protos */

void get_mpz_from_digits(MP_INT *tmp,
			 struct pike_string *digits,
			 int base);
int get_new_mpz(MP_INT *tmp, struct svalue *s,
		int throw_error, const char *arg_func, int arg, int args);
MP_INT *debug_get_mpz(struct svalue *s,
		      int throw_error, const char *arg_func, int arg, int args);
void mpzmod_reduce(struct object *o);
struct pike_string *low_get_mpz_digits(MP_INT *mpz, int base);

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

extern struct program *mpzmod_program;
extern struct program *mpq_program;
extern struct program *mpf_program;
extern struct program *bignum_program;

#ifdef DEBUG_MALLOC
#define get_mpz(S, THROW_ERROR, ARG_FUNC, ARG, ARGS)			\
  (REFCOUNTED_TYPE(TYPEOF(*(S))) ? debug_malloc_touch((S)->u.object) : 0, \
   debug_get_mpz((S), (THROW_ERROR), (ARG_FUNC), (ARG), (ARGS)))
#else
#define get_mpz debug_get_mpz 
#endif

#define MP_FLT __mpf_struct

#define OBTOMPZ(o) ((MP_INT *)(o->storage))
#define OBTOMPQ(o) ((MP_RAT *)(o->storage))
#define OBTOMPF(o) ((MP_FLT *)(o->storage))

#define IS_MPZ_OBJ(O) ((O)->prog == bignum_program || (O)->prog == mpzmod_program || ((O)->storage && get_storage((O),mpzmod_program)==(O)->storage))
#ifndef GMP_NUMB_BITS
#define GMP_NUMB_BITS (SIZEOF_MP_LIMB_T * CHAR_BIT)
#endif
#ifndef GMP_NUMB_MASK
#define GMP_NUMB_MASK ((mp_limb_t) -1)
#endif

/* Bits excluding the sign bit, if any. */
#define ULONG_BITS (SIZEOF_LONG * CHAR_BIT)
#define ULONGEST_BITS (SIZEOF_LONGEST * CHAR_BIT)
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
