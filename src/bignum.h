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
 *
 *  The family of DO_*_OVERFLOW functions sets the result only if no overflow occured.
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

#define GEN_USUB_OF(type)                                                               \
static INLINE int DO_U ## type ## _SUB_OVERFLOW(unsigned type a, unsigned type b,       \
                                                unsigned type * res) {                  \
    if (b > a)                                                                          \
        return 1;                                                                       \
    *res = a - b;                                                                       \
    return 0;                                                                           \
}                                                                                       \

#define GEN_OF1(size)					\
  GEN_USUB_OF(INT ## size)                              \
  _GEN_OF1(INT ## size, size)				\
  GENERIC_OVERFLOW_CHECKS(INT ## size)
#define GEN_OF2(s1, s2, utype2)				\
  GEN_USUB_OF(INT ## s1)                                \
  _GEN_OF2(INT ## s1, INT ## s2, utype2, s1)            \
  GENERIC_OVERFLOW_CHECKS(INT ## s1)

#if PIKE_CLANG_BUILTIN(__builtin_uadd_overflow)
#define DO_CLANG_OF(name, type, builtin)                \
static INLINE int name(type a, type b, type * res) {    \
    type tmp;                                           \
    if (builtin(a, b, &tmp)) return 1;                  \
    *res = tmp;                                         \
    return 0;                                           \
}

DO_CLANG_OF(DO_INT32_ADD_OVERFLOW, INT32, __builtin_sadd_overflow)
DO_CLANG_OF(DO_INT32_SUB_OVERFLOW, INT32, __builtin_ssub_overflow)
DO_CLANG_OF(DO_INT32_MUL_OVERFLOW, INT32, __builtin_smul_overflow)
DO_CLANG_OF(DO_UINT32_ADD_OVERFLOW, unsigned INT32, __builtin_uadd_overflow)
DO_CLANG_OF(DO_UINT32_SUB_OVERFLOW, unsigned INT32, __builtin_usub_overflow)
DO_CLANG_OF(DO_UINT32_MUL_OVERFLOW, unsigned INT32, __builtin_umul_overflow)

GENERIC_OVERFLOW_CHECKS(INT32)

#if defined(INT64)
# if SIZEOF_LONG == 8
DO_CLANG_OF(DO_INT64_ADD_OVERFLOW, INT64, __builtin_saddl_overflow)
DO_CLANG_OF(DO_INT64_SUB_OVERFLOW, INT64, __builtin_ssubl_overflow)
DO_CLANG_OF(DO_INT64_MUL_OVERFLOW, INT64, __builtin_smull_overflow)
DO_CLANG_OF(DO_UINT64_ADD_OVERFLOW, UINT64, __builtin_uaddl_overflow)
DO_CLANG_OF(DO_UINT64_SUB_OVERFLOW, UINT64, __builtin_usubl_overflow)
DO_CLANG_OF(DO_UINT64_MUL_OVERFLOW, UINT64, __builtin_umull_overflow)
# elif SIZEOF_LONG_LONG == 8
DO_CLANG_OF(DO_INT64_ADD_OVERFLOW, INT64, __builtin_saddll_overflow)
DO_CLANG_OF(DO_INT64_SUB_OVERFLOW, INT64, __builtin_ssubll_overflow)
DO_CLANG_OF(DO_INT64_MUL_OVERFLOW, INT64, __builtin_smulll_overflow)
DO_CLANG_OF(DO_UINT64_ADD_OVERFLOW, UINT64, __builtin_uaddll_overflow)
DO_CLANG_OF(DO_UINT64_SUB_OVERFLOW, UINT64, __builtin_usubll_overflow)
DO_CLANG_OF(DO_UINT64_MUL_OVERFLOW, UINT64, __builtin_umulll_overflow)
#endif
GENERIC_OVERFLOW_CHECKS(INT64)
#endif

#else /* PIKE_CLANG_BUILTIN(__builtin_uadd_overflow) */

#if defined(INT128) && defined(UINT128)
GEN_OF2(64, 128, UINT128)
GEN_OF2(32, 64, UINT64)
#elif defined(INT64) && defined(UINT64)
GEN_OF1(64)
GEN_OF2(32, 64, UINT64)
#else
GEN_OF1(32)
#endif

#endif /* PIKE_CLANG_BUILTIN(__builtin_uadd_overflow) */

GEN_OF2(16, 32, unsigned INT32)
GEN_OF2(8, 32, unsigned INT32)


/* NB: GCC 4.1.2 doesn't alias pointers to INT_TYPE and to INT32/INT64,
 *     so the DO_INT_TYPE_*_OVERFLOW variants can't just be cpp-renames
 *     to the corresponding INT32/INT64 variant, as the stores to the
 *     target variable may get lost. We fix this by having variables
 *     of the correct type as temporaries, and then copying the result.
 */
#define _GEN_UNOP(OP, type1, TYPE1, type2, name)			\
  static INLINE int name ## _ ## OP ## _OVERFLOW(type2 a) {		\
    type1 tmp;							        \
      return (DO_## TYPE1 ## _ ## OP ## _OVERFLOW(a, &tmp));		\
  }									\
  static INLINE int DO_ ## name ## _ ## OP ## _OVERFLOW(type2 a,	\
						        type2 *res) {	\
    type1 tmp;							        \
    if (DO_ ## TYPE1 ## _ ## OP ## _OVERFLOW(a, &tmp)) return 1;	\
    *res = tmp;								\
    return 0;								\
  }
#define _GEN_BINOP(OP, type1, TYPE1, type2, name)			\
  static INLINE int name ## _ ## OP ## _OVERFLOW(type2 a, type2 b) {	\
    type1 tmp;							        \
    return (DO_ ## TYPE1 ## _ ## OP ## _OVERFLOW(a, b, &tmp));	        \
  }									\
  static INLINE int DO_ ## name ## _ ## OP ## _OVERFLOW(type2 a,	\
                                                        type2 b,	\
                                                        type2 *res) {	\
    type1 tmp;	        						\
    if (DO_ ## TYPE1 ## _ ## OP ## _OVERFLOW(a, b, &tmp)) return 1;	\
    *res = tmp;								\
    return 0;								\
  }

#define GEN_INT_TYPE(size)					\
  _GEN_UNOP(NEG, INT ## size, INT ## size, INT_TYPE, INT_TYPE)	\
  _GEN_BINOP(MUL, INT ## size, INT ## size, INT_TYPE, INT_TYPE)	\
  _GEN_BINOP(ADD, INT ## size, INT ## size, INT_TYPE, INT_TYPE)	\
  _GEN_BINOP(SUB, INT ## size, INT ## size, INT_TYPE, INT_TYPE)	\
  _GEN_BINOP(DIV, INT ## size, INT ## size, INT_TYPE, INT_TYPE)	\
  _GEN_BINOP(MOD, INT ## size, INT ## size, INT_TYPE, INT_TYPE)	\
  _GEN_BINOP(LSH, INT ## size, INT ## size, INT_TYPE, INT_TYPE)	\
  _GEN_BINOP(RSH, INT ## size, INT ## size, INT_TYPE, INT_TYPE)

#if SIZEOF_INT_TYPE == 8
GEN_INT_TYPE(64)
#elif SIZEOF_INT_TYPE == 4
GEN_INT_TYPE(32)
#endif

/* let's assume that sizeof(char*) == sizeof(size_t) */
#if SIZEOF_CHAR_P == 8
_GEN_BINOP(ADD, UINT64, UINT64, size_t, SIZE_T)
_GEN_BINOP(SUB, UINT64, UINT64, size_t, SIZE_T)
_GEN_BINOP(MUL, UINT64, UINT64, size_t, SIZE_T)
#elif SIZEOF_CHAR_P == 4
_GEN_BINOP(ADD, unsigned INT32, UINT32, size_t, SIZE_T)
_GEN_BINOP(SUB, unsigned INT32, UINT32, size_t, SIZE_T)
_GEN_BINOP(MUL, unsigned INT32, UINT32, size_t, SIZE_T)
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
