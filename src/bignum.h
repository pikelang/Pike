/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef BIGNUM_H
#define BIGNUM_H

#include "global.h"
#include "pike_int_types.h"

/* Note: These functions assume some properties of the CPU. */

#define INT_TYPE_SIGN(x)             ((x) < 0)

/*
 * Arithmetic operations which try to be standard compliant when checking for overflow.
 * The first set of functions uses a second larger integer type to perform computations.
 * The second uses manual multiplication for unsigned multiply or checks for overflow
 * as recommended in
 *  https://www.securecoding.cert.org/confluence/display/seccode/INT32-C.+Ensure+that+operations+on+signed+integers+do+not+result+in+overflow
 *  When using clang with builtin support for checking arithmetic overflow, those builtins will
 *  be used.
 *  These functions will also detect and try to avoid undefined behavior, e.g. shifts of
 *  negative integers.
 */
#define GENERIC_OVERFLOW_CHECKS(type)                                                   \
static INLINE int DO_## type ## _NEG_OVERFLOW(type a, type * res) {                     \
    if (a == MIN_ ## type) return 1;                                                    \
    *res = -a;                                                                          \
    return 0;                                                                           \
}                                                                                       \
static INLINE int type ## _NEG_OVERFLOW(type a) {                                       \
    return a == MIN_ ## type;                                                           \
}                                                                                       \
static INLINE int type ## _DIV_OVERFLOW(type a, type b) {                               \
    return a == MIN_ ## type && b == -1;                                                \
}                                                                                       \
static INLINE int DO_ ## type ## _DIV_OVERFLOW(type a, type b, type * res) {            \
    if (a == MIN_ ## type && b == -1) return 1;                                         \
    *res = a/b;                                                                         \
    return 0;                                                                           \
}                                                                                       \
static INLINE int U ## type ## _MUL_OVERFLOW(unsigned type a, unsigned type b) {        \
    unsigned type res;                                                                  \
    return DO_U ## type ## _MUL_OVERFLOW(a, b, &res);                                   \
}                                                                                       \
static INLINE int U ## type ## _ADD_OVERFLOW(unsigned type a, unsigned type b) {        \
    unsigned type res;                                                                  \
    return DO_U ## type ## _ADD_OVERFLOW(a, b, &res);                                   \
}                                                                                       \
static INLINE int U ## type ## _SUB_OVERFLOW(unsigned type a, unsigned type b) {        \
    unsigned type res;                                                                  \
    return DO_U ## type ## _SUB_OVERFLOW(a, b, &res);                                   \
}                                                                                       \
static INLINE int type ## _MUL_OVERFLOW(type a, type b) {                               \
    type res;                                                                           \
    return DO_ ## type ## _MUL_OVERFLOW(a, b, &res);                                    \
}                                                                                       \
static INLINE int type ## _ADD_OVERFLOW(type a, type b) {                               \
    type res;                                                                           \
    return DO_ ## type ## _ADD_OVERFLOW(a, b, &res);                                    \
}                                                                                       \
static INLINE int type ## _SUB_OVERFLOW(type a, type b) {                               \
    type res;                                                                           \
    return DO_ ## type ## _SUB_OVERFLOW(a, b, &res);                                    \
}                                                                                       \
static INLINE int type ## _MOD_OVERFLOW(type a, type b) {                               \
    return type ## _DIV_OVERFLOW(a, b);                                                 \
}                                                                                       \
static INLINE int DO_ ## type ## _MOD_OVERFLOW(type a, type b, type * res) {            \
    if (type ## _MOD_OVERFLOW(a, b)) {                                                  \
        return 1;                                                                       \
    }                                                                                   \
    *res = a % b;                                                                       \
    return 0;                                                                           \
}                                                                                       \
static INLINE int type ## _LSH_OVERFLOW(type a, type b) {                               \
    type size = (type)sizeof(type)*CHAR_BIT;                                            \
    return (b < 0 || b >= size - 1 || a < 0 || (b && (a >> (size - 1 - b))));           \
}                                                                                       \
static INLINE int type ## _RSH_OVERFLOW(type a, type b) {                               \
    return (b < 0 || a < 0 || b >= (type)sizeof(type)*CHAR_BIT);                        \
}                                                                                       \
static INLINE int DO_ ## type ## _LSH_OVERFLOW(type a, type b, type * res) {            \
    if (type ## _LSH_OVERFLOW(a, b)) return 1;                                          \
    *res = a << b;                                                                      \
    return 0;                                                                           \
}                                                                                       \
static INLINE int DO_ ## type ## _RSH_OVERFLOW(type a, type b, type * res) {            \
    if (type ## _RSH_OVERFLOW(a, b)) return 1;                                          \
    *res = a >> b;                                                                      \
    return 0;                                                                           \
}

#if PIKE_CLANG_BUILTIN(__builtin_uadd_overflow)
#define DO_INT32_ADD_OVERFLOW   __builtin_sadd_overflow
#define DO_INT32_SUB_OVERFLOW   __builtin_ssub_overflow
#define DO_INT32_MUL_OVERFLOW   __builtin_smul_overflow
#define DO_UINT32_ADD_OVERFLOW   __builtin_uadd_overflow
#define DO_UINT32_SUB_OVERFLOW   __builtin_usub_overflow
#define DO_UINT32_MUL_OVERFLOW   __builtin_umul_overflow
#define DO_INT64_ADD_OVERFLOW   __builtin_saddl_overflow
#define DO_INT64_SUB_OVERFLOW   __builtin_ssubl_overflow
#define DO_INT64_MUL_OVERFLOW   __builtin_smull_overflow
#define DO_UINT64_ADD_OVERFLOW   __builtin_uaddl_overflow
#define DO_UINT64_SUB_OVERFLOW   __builtin_usubl_overflow
#define DO_UINT64_MUL_OVERFLOW   __builtin_umull_overflow

GENERIC_OVERFLOW_CHECKS(INT32)
#if defined(INT64)
GENERIC_OVERFLOW_CHECKS(INT64)
#endif

#else
#define _GEN_OF2(type, type2, utype2, size)                                             \
static INLINE int DO_ ## type ## _ADD_OVERFLOW(type a, type b, type * res) {            \
    type2 tmp;                                                                          \
    tmp = (type2)a + (type2)b;                                                          \
    if (tmp < MIN_ ## type || tmp > MAX_ ## type)                                       \
        return 1;                                                                       \
    *res = (type)tmp;                                                                   \
    return 0;                                                                           \
}                                                                                       \
static INLINE int DO_ ## type ## _SUB_OVERFLOW(type a, type b, type * res) {            \
    type2 tmp;                                                                          \
    tmp = (type2)a - (type2)b;                                                          \
    if (tmp < MIN_ ## type || tmp > MAX_ ## type)                                       \
        return 1;                                                                       \
    *res = (type)tmp;                                                                   \
    return 0;                                                                           \
}                                                                                       \
static INLINE int DO_ ## type ## _MUL_OVERFLOW(type a, type b, type * res) {            \
    type2 tmp;                                                                          \
    tmp = (type2)a * (type2)b;                                                          \
    if (tmp < MIN_ ## type || tmp > MAX_ ## type)                                       \
        return 1;                                                                       \
    *res = (type)tmp;                                                                   \
    return 0;                                                                           \
}                                                                                       \
static INLINE int DO_U ## type ## _ADD_OVERFLOW(unsigned type a, unsigned type b,       \
                                                unsigned type * res) {                  \
    utype2 tmp;                                                                         \
    tmp = (utype2)a + (utype2)b;                                                        \
    if (tmp >> size) return 1;                                                          \
    *res = (unsigned type)tmp;                                                          \
    return 0;                                                                           \
}                                                                                       \
static INLINE int DO_U ## type ## _SUB_OVERFLOW(unsigned type a, unsigned type b,       \
                                                unsigned type * res) {                  \
    utype2 tmp;                                                                         \
    tmp = (utype2)a - (utype2)b;                                                        \
    if (tmp >> size) return 1;                                                          \
    *res = (unsigned type)tmp;                                                          \
    return 0;                                                                           \
}                                                                                       \
static INLINE int DO_U ## type ## _MUL_OVERFLOW(unsigned type a, unsigned type b,       \
                                                unsigned type * res) {                  \
    utype2 tmp;                                                                         \
    tmp = (utype2)a * (utype2)b;                                                        \
    if (tmp >> size) return 1;                                                          \
    *res = (unsigned type)tmp;                                                          \
    return 0;                                                                           \
}

#define _GEN_OF1(type, size)                                                            \
static INLINE int DO_ ## type ## _ADD_OVERFLOW(type a, type b, type * res) {            \
    if ((b > 0 && a > MAX_ ## type - b) ||                                              \
        (b < 0 && a < MIN_ ## type - b))                                                \
        return 1;                                                                       \
    *res = a + b;                                                                       \
    return 0;                                                                           \
}                                                                                       \
static INLINE int DO_ ## type ## _SUB_OVERFLOW(type a, type b, type * res) {            \
    if ((b > 0 && a < MIN_ ## type + b) ||                                              \
        (b < 0 && a > MAX_ ## type + b))                                                \
        return 1;                                                                       \
    *res = a - b;                                                                       \
    return 0;                                                                           \
}                                                                                       \
static INLINE int DO_## type ## _MUL_OVERFLOW(type a, type b, type * res) {             \
    if (a > 0) {                                                                        \
      if (b > 0) {                                                                      \
        if (a > (MAX_ ## type / b)) {                                                   \
          return 1;                                                                     \
        }                                                                               \
      } else {                                                                          \
        if (b < (MIN_ ## type / a)) {                                                   \
          return 1;                                                                     \
        }                                                                               \
      }                                                                                 \
    } else {                                                                            \
      if (b > 0) {                                                                      \
        if (a < (MAX_ ## type / b)) {                                                   \
          return 1;                                                                     \
        }                                                                               \
      } else {                                                                          \
        if ( (a != 0) && (b < (MAX_ ## type / a))) {                                    \
          return 1;                                                                     \
        }                                                                               \
      }                                                                                 \
    }                                                                                   \
    *res = a * b;                                                                       \
    return 0;                                                                           \
}                                                                                       \
static INLINE int DO_U ## type ## _ADD_OVERFLOW(unsigned type a, unsigned type b,       \
                                                unsigned type * res) {                  \
    if (a > MAX_U ## type - b)                                                          \
        return 1;                                                                       \
    *res = a + b;                                                                       \
    return 0;                                                                           \
}                                                                                       \
static INLINE int DO_U ## type ## _MUL_OVERFLOW(unsigned type a, unsigned type b,       \
                                                unsigned type * res) {                  \
    unsigned type tmp = 0;                                                              \
    unsigned type bits = size/2;                                                        \
    unsigned type low_mask = ((1 << bits)-1);                                           \
    unsigned type a1 = a >> bits;                                                       \
    unsigned type b1 = b >> bits;                                                       \
    unsigned type a0 = a & low_mask;                                                    \
    unsigned type b0 = b & low_mask;                                                    \
    tmp = a1 * b1;                                                                      \
    a1 *= b0;                                                                           \
    b1 *= a0;                                                                           \
    if (tmp || (a1|b1) & ~low_mask) return 1;                                           \
    tmp = a1<<bits;                                                                     \
    if (DO_U ## type ## _ADD_OVERFLOW(tmp, b1<<bits, &tmp)                              \
      || DO_U ## type ## _ADD_OVERFLOW(tmp, a0*b0, &tmp)) return 1;                     \
    *res = tmp;                                                                         \
    return 0;                                                                           \
}                                                                                       \

#define GEN_OF1(size)					\
  _GEN_OF1(INT ## size, size)				\
  GENERIC_OVERFLOW_CHECKS(INT ## size)
#define GEN_OF2(s1, s2)					\
  _GEN_OF2(INT ## s1, INT ## s2, UINT ## s2, s1)	\
  GENERIC_OVERFLOW_CHECKS(INT ## s1)

#if defined(INT128) && defined(UINT128)
GEN_OF2(64, 128)
GEN_OF2(32, 64)
#elif defined(INT64) && defined(UINT64)
GEN_OF1(64)
GEN_OF2(32, 64)
#else
GEN_OF1(32)
#endif

#endif

#if SIZEOF_INT_TYPE == 8
#define INT_TYPE_MUL_OVERFLOW	    INT64_MUL_OVERFLOW
#define INT_TYPE_ADD_OVERFLOW	    INT64_ADD_OVERFLOW
#define INT_TYPE_SUB_OVERFLOW	    INT64_SUB_OVERFLOW
#define INT_TYPE_NEG_OVERFLOW	    INT64_NEG_OVERFLOW
#define INT_TYPE_DIV_OVERFLOW	    INT64_DIV_OVERFLOW
#define INT_TYPE_MOD_OVERFLOW	    INT64_MOD_OVERFLOW
#define INT_TYPE_LSH_OVERFLOW       INT64_LSH_OVERFLOW
#define INT_TYPE_RSH_OVERFLOW       INT64_RSH_OVERFLOW
#define DO_INT_TYPE_MUL_OVERFLOW    DO_INT64_MUL_OVERFLOW
#define DO_INT_TYPE_ADD_OVERFLOW    DO_INT64_ADD_OVERFLOW
#define DO_INT_TYPE_SUB_OVERFLOW    DO_INT64_SUB_OVERFLOW
#define DO_INT_TYPE_NEG_OVERFLOW    DO_INT64_NEG_OVERFLOW
#define DO_INT_TYPE_DIV_OVERFLOW    DO_INT64_DIV_OVERFLOW
#define DO_INT_TYPE_MOD_OVERFLOW    DO_INT64_MOD_OVERFLOW
#define DO_INT_TYPE_LSH_OVERFLOW    DO_INT64_LSH_OVERFLOW
#define DO_INT_TYPE_RSH_OVERFLOW    DO_INT64_RSH_OVERFLOW
#elif SIZEOF_INT_TYPE == 4
#define INT_TYPE_MUL_OVERFLOW	    INT32_MUL_OVERFLOW
#define INT_TYPE_ADD_OVERFLOW	    INT32_ADD_OVERFLOW
#define INT_TYPE_SUB_OVERFLOW	    INT32_SUB_OVERFLOW
#define INT_TYPE_NEG_OVERFLOW	    INT32_NEG_OVERFLOW
#define INT_TYPE_DIV_OVERFLOW	    INT32_DIV_OVERFLOW
#define INT_TYPE_MOD_OVERFLOW	    INT32_MOD_OVERFLOW
#define INT_TYPE_LSH_OVERFLOW       INT32_LSH_OVERFLOW
#define INT_TYPE_RSH_OVERFLOW       INT32_RSH_OVERFLOW
#define DO_INT_TYPE_MUL_OVERFLOW    DO_INT32_MUL_OVERFLOW
#define DO_INT_TYPE_ADD_OVERFLOW    DO_INT32_ADD_OVERFLOW
#define DO_INT_TYPE_SUB_OVERFLOW    DO_INT32_SUB_OVERFLOW
#define DO_INT_TYPE_NEG_OVERFLOW    DO_INT32_NEG_OVERFLOW
#define DO_INT_TYPE_DIV_OVERFLOW    DO_INT32_DIV_OVERFLOW
#define DO_INT_TYPE_MOD_OVERFLOW    DO_INT32_MOD_OVERFLOW
#define DO_INT_TYPE_LSH_OVERFLOW    DO_INT32_LSH_OVERFLOW
#define DO_INT_TYPE_RSH_OVERFLOW    DO_INT32_RSH_OVERFLOW
#endif

/* Prototypes begin here */
PMOD_EXPORT extern struct svalue auto_bignum_program;
PMOD_EXPORT struct program *get_auto_bignum_program(void);
PMOD_EXPORT struct program *get_auto_bignum_program_or_zero(void);
void init_auto_bignum(void);
void exit_auto_bignum(void);
PMOD_EXPORT void convert_stack_top_to_bignum(void);
PMOD_EXPORT void convert_stack_top_with_base_to_bignum(void);
int is_bignum_object(struct object *o);
PMOD_EXPORT int is_bignum_object_in_svalue(struct svalue *sv);
PMOD_EXPORT struct object *make_bignum_object(void);
PMOD_EXPORT struct object *bignum_from_svalue(struct svalue *s);
PMOD_EXPORT struct pike_string *string_from_bignum(struct object *o, int base);
PMOD_EXPORT void convert_svalue_to_bignum(struct svalue *s);

#ifdef INT64
PMOD_EXPORT extern void (*push_int64)(INT64 i);

/* Returns nonzero iff conversion is successful. */
PMOD_EXPORT extern int (*int64_from_bignum) (INT64 *i, struct object *bignum);

PMOD_EXPORT extern void (*reduce_stack_top_bignum) (void);
#else
#define push_int64(i) push_int((INT_TYPE)(i))
#define int64_from_bignum(I,BIGNUM)	0
#endif /* INT64 */

PMOD_EXPORT extern void (*push_ulongest) (unsigned LONGEST i);
PMOD_EXPORT extern int (*ulongest_from_bignum) (unsigned LONGEST *i,
						struct object *bignum);
#ifndef __MPN
#define MP_INT void
#endif

PMOD_EXPORT extern int (*mpz_from_svalue)(MP_INT *, struct svalue *);
PMOD_EXPORT extern void (*push_bignum)(MP_INT *);

PMOD_EXPORT void hook_in_gmp_funcs (
#ifdef INT64
  void (*push_int64_val)(INT64),
  int (*int64_from_bignum_val) (INT64 *, struct object *),
  void (*reduce_stack_top_bignum_val) (void),
#endif
  void (*push_ulongest_val) (unsigned LONGEST),
  int (*ulongest_from_bignum_val) (unsigned LONGEST *, struct object *),
  int (*mpz_from_svalue_val)(MP_INT *, struct svalue *),
  void (*push_bignum_val)(MP_INT *));
/* Prototypes end here */

/* Less confusing name, considering that push_int64 pushes a 32 bit
 * int if INT64 isn't available. */
#define push_longest push_int64

#endif /* BIGNUM_H */
