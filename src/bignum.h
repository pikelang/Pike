#include "global.h"

#ifdef AUTO_BIGNUM

#ifndef BIGNUM_H
#define BIGNUM_H

struct object *make_bignum_object(void);

struct program *get_auto_bignum_program(void);
void exit_auto_bignum(void);

/* NOTE: These functions assume some properties of the CPU. */

#define INT_TYPE_SIGN(x)             ((x) < 0 ? -1 : 1)

#define INT_TYPE_MUL_OVERFLOW(a, b)  ((b) && ((a)*(b))/(b) != (a))

#define INT_TYPE_NEG_OVERFLOW(x)     ((x) == -(x))

#define INT_TYPE_ADD_OVERFLOW(a, b)                                        \
        (INT_TYPE_SIGN(a) == INT_TYPE_SIGN(b) &&                           \
	 INT_TYPE_SIGN(a) != INT_TYPE_SIGN((a)+(b)))

#define INT_TYPE_SUB_OVERFLOW(a, b)  INT_TYPE_ADD_OVERFLOW((a), -(b))

#define INT_TYPE_ASL_OVERFLOW(a, b)  ((((a)<<(b))>>(b)) != (a))

#endif /* BIGNUM_H */

#endif /* AUTO_BIGNUM */
