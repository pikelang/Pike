/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "pike_int_types.h"

#ifndef BIGNUM_H
#define BIGNUM_H

/* Note: These functions assume some properties of the CPU. */

#define INT_TYPE_SIGN(x)             ((x) < 0)


#define INT_TYPE_DIV_OVERFLOW(a, b)  (INT_TYPE_NEG_OVERFLOW(a) && (b) == -1)

#define INT_TYPE_NEG_OVERFLOW(x)     ((x) && (x) == -(x))

#define _GEN_OF2(type, type2, size)						    \
static INLINE type DO_ ## type ## _ADD_OVERFLOW(type a, type b, int * of) {	    \
    type2 res;									    \
    res = (type2)a + (type2)b;							    \
    if (res < MIN_ ## type || res > MAX_ ## type)				    \
	*of = 1;								    \
    return (type)res;								    \
}										    \
static INLINE type DO_ ## type ## _MUL_OVERFLOW(type a, type b, int * of) {	    \
    type2 res;									    \
    res = (type2)a * (type2)b;							    \
    if (res < MIN_ ## type || res > MAX_ ## type)				    \
	*of = 1;								    \
    return (type)res;								    \
}										    \
static INLINE type DO_U ## type ## _ADD_OVERFLOW(unsigned type a, unsigned type b,  \
						   int * of) {			    \
    unsigned type2 res;								    \
    res = (unsigned type2)a + (unsigned type2)b;				    \
    if (res > MAX_U ## type)							    \
	*of = 1;								    \
    return (unsigned type)res;							    \
}										    \
static INLINE type DO_U ## type ## _MUL_OVERFLOW(unsigned type a,		    \
						    unsigned type b, int * of) {    \
    unsigned type2 res;								    \
    res = (unsigned type2)a * (unsigned type2)b;				    \
    if (res > MAX_U ## type)							    \
	*of = 1;								    \
    return (unsigned type)res;							    \
}

#define _GEN_OF1(type, size)							    \
static INLINE type DO_ ## type ## _ADD_OVERFLOW(type a, type b, int * of) {	    \
    if ((b > 0 && a < MIN_ ## type + b) ||					    \
	(b < 0 && a > MAX_ ## type + b))					    \
	*of = 1;								    \
    return a + b;								    \
}										    \
static INLINE type DO_## type ## _MUL_OVERFLOW(type a, type b, int * of) {	    \
    if (a > 0) {								    \
      if (b > 0) {								    \
	if (a > (MAX_ ## type / b)) {						    \
	  *of = 1;								    \
	}									    \
      } else {									    \
	if (b < (MIN_ ## type / a)) {						    \
	  *of = 1;								    \
	}									    \
      }										    \
    } else {									    \
      if (b > 0) {								    \
	if (a < (MAX_ ## type / b)) {						    \
	  *of = 1;								    \
	}									    \
      } else {									    \
	if ( (a != 0) && (b < (MAX_ ## type / a))) {				    \
	  *of = 1;								    \
	}									    \
      }										    \
    }										    \
    return a*b;									    \
}										    \
static INLINE type DO_U ## type ## _ADD_OVERFLOW(unsigned type a, unsigned type b,  \
						 int * of) {			    \
    if (a > MAX_U ## type - b)							    \
	*of = 1;								    \
    return a + b;								    \
}										    \
static INLINE type DO_U ## type ## _MUL_OVERFLOW(unsigned type a,		    \
						    unsigned type b, int * of) {    \
    unsigned type res = 0;							    \
    const unsigned type bits = size * 8;					    \
    const unsigned type low_mask = ~((1 << (1 + bits/2))-1);			    \
    unsigned type a1 = a & ~low_mask >> bits/2;					    \
    unsigned type b1 = b & ~low_mask >> bits/2;					    \
    a &= low_mask;								    \
    b &= low_mask;								    \
    res = a1 * b1;								    \
    a1 *= b;									    \
    b1 *= a;									    \
    if (res || (a1|b1) & ~low_mask) *of = 1;					    \
    res = DO_U ## type ## _ADD_OVERFLOW(res, a1<<bits/2, of);			    \
    res = DO_U ## type ## _ADD_OVERFLOW(res, b1<<bits/2, of);			    \
    res = DO_U ## type ## _ADD_OVERFLOW(res, a*b, of);				    \
    return res;									    \
}										    \

#define _GEN_OF_CHECK(type)							    \
static INLINE int U ## type ## _MUL_OVERFLOW(unsigned type a, unsigned type b) {    \
    int of = 0;									    \
    DO_U ## type ## _MUL_OVERFLOW(a, b, &of);					    \
    return of;									    \
}										    \
static INLINE int U ## type ## _ADD_OVERFLOW(unsigned type a, unsigned type b) {    \
    int of = 0;									    \
    DO_U ## type ## _ADD_OVERFLOW(a, b, &of);					    \
    return of;									    \
}										    \
static INLINE int type ## _MUL_OVERFLOW(type a, type b) {			    \
    int of = 0;									    \
    DO_ ## type ## _MUL_OVERFLOW(a, b, &of);					    \
    return of;									    \
}										    \
static INLINE int type ## _ADD_OVERFLOW(type a, type b) {			    \
    int of = 0;									    \
    DO_ ## type ## _ADD_OVERFLOW(a, b, &of);					    \
    return of;									    \
}


#define GEN_OF1(size) _GEN_OF1(INT ## size, size)\
		      _GEN_OF_CHECK(INT ## size)
#define GEN_OF2(s1, s2) _GEN_OF2(INT ## s1, INT ## s2, s1)\
			_GEN_OF_CHECK(INT ## s1)

#ifdef INT128
GEN_OF2(64, 128)
GEN_OF2(32, 64)
#elif defined(INT64)
GEN_OF1(64)
GEN_OF2(32, 64)
#else
GEN_OF1(32)
#endif

#if (INT64 == INT_TYPE)
#define INT_TYPE_MUL_OVERFLOW INT64_MUL_OVERFLOW
#define INT_TYPE_ADD_OVERFLOW INT64_ADD_OVERFLOW
#define DO_INT_TYPE_MUL_OVERFLOW DO_INT64_MUL_OVERFLOW
#define DO_INT_TYPE_ADD_OVERFLOW DO_INT64_ADD_OVERFLOW
#elif (INT32 == INT_TYPE)
#define INT_TYPE_MUL_OVERFLOW INT32_MUL_OVERFLOW
#define INT_TYPE_ADD_OVERFLOW INT32_ADD_OVERFLOW
#define DO_INT_TYPE_MUL_OVERFLOW DO_INT32_MUL_OVERFLOW
#define DO_INT_TYPE_ADD_OVERFLOW DO_INT32_ADD_OVERFLOW
#endif
     
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
PMOD_EXPORT void hook_in_gmp_funcs (
#ifdef INT64
  void (*push_int64_val)(INT64),
  int (*int64_from_bignum_val) (INT64 *, struct object *),
  void (*reduce_stack_top_bignum_val) (void),
#endif
  void (*push_ulongest_val) (unsigned LONGEST),
  int (*ulongest_from_bignum_val) (unsigned LONGEST *, struct object *));
/* Prototypes end here */

#else

#define push_int64(i) push_int((INT_TYPE)(i))
#define push_ulongest(i) push_int((INT_TYPE)(i))
#define int64_from_bignum(I,BIGNUM)	0
#define ulongest_from_bignum(I,BIGNUM)	0

#endif /* AUTO_BIGNUM */

/* Less confusing name, considering that push_int64 pushes a 32 bit
 * int if INT64 isn't available. */
#define push_longest push_int64

#endif /* BIGNUM_H */
