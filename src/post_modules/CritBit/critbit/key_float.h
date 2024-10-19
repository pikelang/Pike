/* -*- mode: C; c-basic-offset: 4; -*- */
#ifndef CB_FLOAT_H
#define CB_FLOAT_H
#include <math.h>
#include "bitvector.h"
#include "pike_int_types.h"

#if SIZEOF_FLOAT_TYPE == 8
typedef UINT64 CB_NAME(string);
typedef UINT64 CB_NAME(char);
#elif SIZEOF_FLOAT_TYPE == 4
typedef unsigned INT32 CB_NAME(string);
typedef unsigned INT32 CB_NAME(char);
#elif SIZEOF_FLOAT_TYPE == 16 && defined(__SIZEOF_INT128__) && __SIZEOF_INT128__ == 16
typedef unsigned __int128 CB_NAME(string);
typedef unsigned __int128 CB_NAME(char);
#else
#error Size of FLOAT_TYPE not supported.
#endif


#ifdef cb_char
# undef cb_char
#endif
#define cb_char CB_NAME(char)

#ifdef cb_string
# undef cb_string
#endif
#define cb_string CB_NAME(string)

#ifdef CB_SOURCE
#if SIZEOF_FLOAT_TYPE == 4
# define gclz(x) clz32(x)
#elif SIZEOF_FLOAT_TYPE == 8
# define gclz(x) clz64(x)
#else
# define gclz(x) clz128(x)
#endif
#define bitsof(x)	(sizeof(x)*8)
#define int2float(x)	(*(FLOAT_TYPE*)&(x))
#define float2int(x)	(*(cb_char*)&(x))

static inline cb_string cb_encode_float(FLOAT_TYPE f) {
    cb_char str;

    memcpy(&str, &f, sizeof(str));

    if (str & MASK(cb_char, 1)) {
	str = ~str;
    } else {
	str |= MASK(cb_char, 1);
    }

    return str;
}

static inline FLOAT_TYPE cb_decode_float(cb_char s) {
    FLOAT_TYPE f;

    if (s & MASK(cb_char, 1)) {
	s = (s ^ MASK(cb_char, 1));
    } else {
	s = (~s);
    }

    memcpy(&f, &s, sizeof(f));

    return f;
}

#define CB_ADD_KEY_REF(x)	do { } while(0)
#define CB_FREE_KEY(x)		do { } while(0)
#define CB_SET_KEY(node, x)	do { (node)->key = (x); } while (0)

#define CB_KEY_EQ(k1, k2)			\
	( (k1).str == (k2).str ||		\
	  (CB_S_EQ((k1).len, (k2).len) &&	\
	   (k1).len.bits &&			\
	   !(((k1).str ^ (k2).str) & MASK(cb_char, (k1).len.bits))))
#define CB_KEY_LT(k1, k2)			\
	( (k1).str < (k2).str ||		\
	  (((k1).str == (k2).str) && CB_LT((k1).len, (k2).len)) )

#define CB_GET_CHAR(str, n)	(str)
#define CB_WIDTH(s)	(bitsof(cb_string)) /* width in bits */
#define CB_LENGTH(str)	1
#define CB_SIZE(key)	((key)->len)
#define CB_GET_BIT(str, size)			\
	(BITN(cb_char, CB_GET_CHAR((str), (size).chars), (size).bits))
#define CB_COUNT_PREFIX(s1, s2, n)		\
	((size_t)gclz(CB_GET_CHAR((s1), (n)) ^ CB_GET_CHAR((s2), (n))))

static inline cb_size cb_prefix_count_float(const cb_string s1,
					    const cb_string s2,
					    const cb_size len,
                                            const cb_size UNUSED(start)) {
    cb_size ret;

    if (s1 == s2) return len;

    ret.chars = 0;
    ret.bits = (size_t)gclz(s1 ^ s2);

    if (!len.chars) ret.bits = MINIMUM(len.bits, ret.bits);

    return ret;
}

#define cb_prefix_count	cb_prefix_count_float

#endif

#endif
