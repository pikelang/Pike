#include "global.h"

#ifndef BIGNUM_H
#define BIGNUM_H

#ifdef AUTO_BIGNUM

/* Note: These functions assume some properties of the CPU. */

#define INT_TYPE_SIGN(x)             ((x) < 0)

#define INT_TYPE_MUL_OVERFLOW(a, b)  ((b) && ((a)*(b))/(b) != (a))

#define INT_TYPE_NEG_OVERFLOW(x)     ((x) == -(x))

#define INT_TYPE_ADD_OVERFLOW(a, b)                                        \
        (INT_TYPE_SIGN(a) == INT_TYPE_SIGN(b) &&                           \
	 INT_TYPE_SIGN(a) != INT_TYPE_SIGN((a)+(b)))

#define INT_TYPE_SUB_OVERFLOW(a, b)                                        \
        (INT_TYPE_SIGN(a) != INT_TYPE_SIGN(b) &&                           \
	 INT_TYPE_SIGN(a) != INT_TYPE_SIGN((a)-(b)))

#define INT_TYPE_LSH_OVERFLOW(a, b)                                        \
        (((INT_TYPE)sizeof(INT_TYPE))*CHAR_BIT <= (b) ||                   \
	 (((a)<<(b))>>(b)) != (a))

/* Note: If this gives overflow, set the result to zero. */
#define INT_TYPE_RSH_OVERFLOW(a, b)                                        \
        (((INT_TYPE)sizeof(INT_TYPE))*CHAR_BIT <= (b))

/* Prototypes begin here */
void convert_stack_top_to_bignum(void);
void convert_stack_top_with_base_to_bignum(void);
struct object *make_bignum_object(void);
struct object *bignum_from_svalue(struct svalue *s);
struct pike_string *string_from_bignum(struct object *o, int base);
void convert_svalue_to_bignum(struct svalue *s);
int is_bignum_object(struct object *o);
int is_bignum_object_in_svalue(struct svalue *sv);
/* Prototypes end here */

#else

#define INT_TYPE_MUL_OVERFLOW(a, b) 0
#define INT_TYPE_NEG_OVERFLOW(x)    0
#define INT_TYPE_ADD_OVERFLOW(a, b) 0
#define INT_TYPE_SUB_OVERFLOW(a, b) 0
#define INT_TYPE_LSH_OVERFLOW(a, b) 0
#define INT_TYPE_RSH_OVERFLOW(a, b) 0

#endif /* AUTO_BIGNUM */

#endif /* BIGNUM_H */

