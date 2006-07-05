/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: bignum.h,v 1.31 2006/07/05 21:01:04 mast Exp $
*/

#include "global.h"

#ifndef BIGNUM_H
#define BIGNUM_H

/* Note: These functions assume some properties of the CPU. */

#define INT_TYPE_SIGN(x)             ((x) < 0)

#define UNSIGNED_INT_TYPE_MUL_OVERFLOW(a, b) ((b) && ((a)*(b))/(b) != (a))

#ifdef HAVE_NICE_FPU_DIVISION
#define INT_TYPE_MUL_OVERFLOW(a, b) UNSIGNED_INT_TYPE_MUL_OVERFLOW(a, b)
#else
#define INT_TYPE_MUL_OVERFLOW(a, b)                                        \
        ((b) && (INT_TYPE_DIV_OVERFLOW(a, b) || ((a)*(b))/(b) != (a)))
#endif

#define INT_TYPE_DIV_OVERFLOW(a, b)  (INT_TYPE_NEG_OVERFLOW(a) && (b) == -1)

#define INT_TYPE_NEG_OVERFLOW(x)     ((x) && (x) == -(x))

#define INT_TYPE_ADD_OVERFLOW(a, b)		\
  ((((a)^(b)) >= 0) && (((a)^((a)+(b))) < 0))

#define INT_TYPE_SUB_OVERFLOW(a, b)                                        \
  ((((a)^(b)) < 0) && (((a)^((a)-(b))) < 0))

#define INT_TYPE_LSH_OVERFLOW(a, b)                                        \
        ((((INT_TYPE)sizeof(INT_TYPE))*CHAR_BIT <= (b) && (a)) ||          \
	 (((a)<<(b))>>(b)) != (a))

/* Note: If this gives overflow, set the result to zero. */
#define INT_TYPE_RSH_OVERFLOW(a, b)                                        \
        (((INT_TYPE)sizeof(INT_TYPE))*CHAR_BIT <= (b) && (a))

#ifdef AUTO_BIGNUM

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
PMOD_EXPORT extern int (*int64_from_bignum) (INT64 *i, struct object *bignum);
PMOD_EXPORT extern void (*reduce_stack_top_bignum) (void);
PMOD_EXPORT void hook_in_int64_funcs (
  void (*push_int64_val)(INT64),
  int (*int64_from_bignum_val) (INT64 *, struct object *),
  void (*reduce_stack_top_bignum_val) (void));
#else
#define push_int64(i) push_int((INT_TYPE)(i))
#define int64_from_bignum(I,BIGNUM)	0
#endif /* INT64 */
/* Prototypes end here */

#else

#define push_int64(i) push_int((INT_TYPE)(i))
#define int64_from_bignum(I,BIGNUM)	0

#endif /* AUTO_BIGNUM */

#endif /* BIGNUM_H */
