/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: operators.h,v 1.5 1998/05/25 10:38:46 hubbe Exp $
 */
#ifndef OPERATORS_H
#define OPERATORS_H

#define COMPARISON(ID,NAME,X) void ID(INT32 num_arg);

extern struct program *string_assignment_program;
struct string_assignment_storage
{
  struct svalue lval[2];
  struct pike_string *s;
};

/* Prototypes begin here */
void f_ne(INT32 args);
COMPARISON(f_eq,"`==", is_eq)
COMPARISON(f_lt,"`<" , is_lt)
COMPARISON(f_le,"`<=",!is_gt)
COMPARISON(f_gt,"`>" , is_gt)
COMPARISON(f_ge,"`>=",!is_lt)

void f_add(INT32 args);
void o_subtract(void);
void f_minus(INT32 args);
void o_and(void);
void f_and(INT32 args);
void o_or(void);
void f_or(INT32 args);
void o_xor(void);
void f_xor(INT32 args);
void o_lsh(void);
void f_lsh(INT32 args);
void o_rsh(void);
void f_rsh(INT32 args);
void o_multiply(void);
void f_multiply(INT32 args);
void o_divide(void);
void f_divide(INT32 args);
void o_mod(void);
void f_mod(INT32 args);
void o_not(void);
void f_not(INT32 args);
void o_compl(void);
void f_compl(INT32 args);
void o_negate(void);
void o_range(void);
void f_index(INT32 args);
void f_arrow(INT32 args);
void f_sizeof(INT32 args);
void init_operators(void);
void exit_operators();
/* Prototypes end here */

#undef COMPARISON
#endif
