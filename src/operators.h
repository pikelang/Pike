/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef OPERATORS_H
#define OPERATORS_H

#define COMPARISON(ID,NAME,X) void ID(INT32 num_arg);

/* Prototypes begin here */
COMPARISON(f_eq,"`==", is_eq(sp-2,sp-1))
COMPARISON(f_ne,"`!=",!is_eq(sp-2,sp-1))
COMPARISON(f_lt,"`<" , is_lt(sp-2,sp-1))
COMPARISON(f_le,"`<=",!is_gt(sp-2,sp-1))
COMPARISON(f_gt,"`>" , is_gt(sp-2,sp-1))
COMPARISON(f_ge,"`>=",!is_lt(sp-2,sp-1))
void f_add(INT32 args);
void o_subtract();
void f_minus(INT32 args);
void o_and();
void f_and(INT32 args);
void o_or();
void f_or(INT32 args);
void o_xor();
void f_xor(INT32 args);
void o_lsh();
void f_lsh(INT32 args);
void o_rsh();
void f_rsh(INT32 args);
void o_multiply();
void f_multiply(INT32 args);
void o_divide();
void f_divide(INT32 args);
void o_mod();
void f_mod(INT32 args);
void o_not();
void f_not(INT32 args);
void o_compl();
void f_compl(INT32 args);
void o_negate();
void o_range();
void init_operators();
/* Prototypes end here */

#undef COMPARISON
#endif
