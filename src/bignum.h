#include "global.h"

#ifndef BIGNUM_H
#define BIGNUM_H

#ifdef AUTO_BIGNUM


/* NOTE: These functions assume some properties of the CPU. */

#define INT_TYPE_SIGN(x)             ((x) < 0 ? -1 : 1)

#define INT_TYPE_MUL_OVERFLOW(a, b)  ((b) && ((a)*(b))/(b) != (a))

#define INT_TYPE_NEG_OVERFLOW(x)     ((x) == -(x))

#define INT_TYPE_ADD_OVERFLOW(a, b)                                        \
        (INT_TYPE_SIGN(a) == INT_TYPE_SIGN(b) &&                           \
	 INT_TYPE_SIGN(a) != INT_TYPE_SIGN((a)+(b)))

#define INT_TYPE_SUB_OVERFFLOW(a, b) INT_TYPE_ADD_OVERFLOW((a), -(b))

#define INT_TYPE_SUB_OVERFLOW(a, b)  INT_TYPE_ADD_OVERFLOW((a), -(b))
#define INT_TYPE_ASL_OVERFLOW(a, b)  ((((a)<<(b))>>(b)) != (a))


/* Prototypes begin here */
void convert_stack_top_to_bignum(void);
struct object *make_bignum_object(void);
struct object *bignum_from_svalue(struct svalue *s);
void convert_svalue_to_bignum(struct svalue *s);
/* Prototypes end here */

#endif /* AUTO_BIGNUM */

#endif /* BIGNUM_H */

