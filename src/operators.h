/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef OPERATORS_H
#define OPERATORS_H

#define COMPARISON(ID,X) void ID(void);

/* Prototypes begin here */
COMPARISON(f_eq, is_eq(sp-2,sp-1))
COMPARISON(f_ne,!is_eq(sp-2,sp-1))
COMPARISON(f_lt, is_lt(sp-2,sp-1))
COMPARISON(f_le,!is_gt(sp-2,sp-1))
COMPARISON(f_gt, is_gt(sp-2,sp-1))
COMPARISON(f_ge,!is_lt(sp-2,sp-1))
void f_sum(INT32 args);
void f_add();
void f_subtract();
void f_and();
void f_or();
void f_xor();
void f_lsh();
void f_rsh();
void f_multiply();
void f_divide();
void f_mod();
void f_not();
void f_compl();
void f_negate();
void f_is_equal(int args,struct svalue *argp);
void f_range();
/* Prototypes end here */

#undef COMPARISON
#endif
