/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef OPERATORS_H
#define OPERATORS_H

#define COMPARISON(ID,NAME,X) PMOD_EXPORT void ID(INT32 num_arg);

#include "svalue.h"

extern struct program *string_assignment_program;
struct string_assignment_storage
{
  struct svalue lval[2];
  struct pike_string *s;
};

/* Flags for the bitfield to o_range2. */
#define RANGE_LOW_FROM_BEG	0x10
#define RANGE_LOW_FROM_END	0x20
#define RANGE_LOW_OPEN		0x40
#define RANGE_LOW_MASK		0x70
#define RANGE_HIGH_FROM_BEG	0x01
#define RANGE_HIGH_FROM_END	0x02
#define RANGE_HIGH_OPEN		0x04
#define RANGE_HIGH_MASK		0x07

/* The bound type flags passed to `[..] */
#define INDEX_FROM_BEG		0
#define INDEX_FROM_END		1
#define OPEN_BOUND		2
#define tRangeBound tInt02

/* Prototypes begin here */
void index_no_free(struct svalue *to,struct svalue *what,struct svalue *ind);
PMOD_EXPORT void o_index(void);
PMOD_EXPORT void o_cast_to_int(void);
PMOD_EXPORT void o_cast_to_string(void);
PMOD_EXPORT void o_cast(struct pike_type *type, INT32 run_time_type);
PMOD_EXPORT void f_cast(void);
void o_check_soft_cast(struct svalue *s, struct pike_type *type);

PMOD_EXPORT void f_ne(INT32 args);
COMPARISON(f_eq,"`==", is_eq)
COMPARISON(f_lt,"`<" , is_lt)
COMPARISON(f_le,"`<=", is_le)
COMPARISON(f_gt,"`>" , is_gt)
COMPARISON(f_ge,"`>=", is_ge)

PMOD_EXPORT INT32 low_rop(struct object *o, int i, INT32 e, INT32 args);
PMOD_EXPORT void f_add(INT32 args);
PMOD_EXPORT void o_subtract(void);
PMOD_EXPORT void f_minus(INT32 args);
PMOD_EXPORT void o_and(void);
PMOD_EXPORT void f_and(INT32 args);
PMOD_EXPORT void o_or(void);
PMOD_EXPORT void f_or(INT32 args);
PMOD_EXPORT void o_xor(void);
PMOD_EXPORT void f_xor(INT32 args);
PMOD_EXPORT void o_lsh(void);
PMOD_EXPORT void f_lsh(INT32 args);
PMOD_EXPORT void o_rsh(void);
PMOD_EXPORT void f_rsh(INT32 args);
PMOD_EXPORT void o_multiply(void);
PMOD_EXPORT void f_multiply(INT32 args);
PMOD_EXPORT void f_exponent(INT32 args);
PMOD_EXPORT void o_divide(void);
PMOD_EXPORT void f_divide(INT32 args);
PMOD_EXPORT void o_mod(void);
PMOD_EXPORT void f_mod(INT32 args);
PMOD_EXPORT void o_not(void);
PMOD_EXPORT void f_not(INT32 args);
PMOD_EXPORT void o_compl(void);
PMOD_EXPORT void f_compl(INT32 args);
PMOD_EXPORT void o_negate(void);
PMOD_EXPORT void o_range2(int bound_types);
PMOD_EXPORT void f_range (INT32 args);
PMOD_EXPORT void f_index(INT32 args);
PMOD_EXPORT void f_arrow(INT32 args);
PMOD_EXPORT void f_index_assign(INT32 args);
PMOD_EXPORT void f_arrow_assign(INT32 args);
PMOD_EXPORT void f_sizeof(INT32 args);
void init_operators(void);
void exit_operators(void);
/* Prototypes end here */

/* Compat. */
#define o_range() o_range2 (RANGE_LOW_FROM_BEG|RANGE_HIGH_FROM_BEG)

#undef COMPARISON
#endif
