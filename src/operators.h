/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
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

/* Prototypes begin here */
PMOD_EXPORT void f_ne(INT32 args);
COMPARISON(f_eq,"`==", is_eq)
COMPARISON(f_lt,"`<" , is_lt)
COMPARISON(f_le,"`<=",!is_gt)
COMPARISON(f_gt,"`>" , is_gt)
COMPARISON(f_ge,"`>=",!is_lt)

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
PMOD_EXPORT void o_divide(void);
PMOD_EXPORT void f_divide(INT32 args);
PMOD_EXPORT void o_mod(void);
PMOD_EXPORT void f_mod(INT32 args);
PMOD_EXPORT void o_not(void);
PMOD_EXPORT void f_not(INT32 args);
PMOD_EXPORT void o_compl(void);
PMOD_EXPORT void f_compl(INT32 args);
PMOD_EXPORT void o_negate(void);
PMOD_EXPORT void o_range(void);
PMOD_EXPORT void f_index(INT32 args);
PMOD_EXPORT void f_index_assign(INT32 args);
PMOD_EXPORT void f_arrow(INT32 args);
PMOD_EXPORT void f_arrow_assign(INT32 args);
PMOD_EXPORT void f_sizeof(INT32 args);
void init_operators(void);
void exit_operators(void);
/* Prototypes end here */

#undef COMPARISON
#endif
