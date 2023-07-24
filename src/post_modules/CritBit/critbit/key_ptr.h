/* -*- mode: C; c-basic-offset: 4; -*- */
#ifndef CB_KEY_PTR_H
#define CB_KEY_PTR_H
#include "global.h"
#include "pike_int_types.h"

#include "bitvector.h"

#ifndef SIZEOF_CHAR_P
# error SIZEOF_CHAR_P is not defined.
#endif

#if SIZEOF_CHAR_P == 8
# define _VOIDP_TYPE UINT64
# define gclz(x) clz64(x)
#elif SIZEOF_CHAR_P == 4
# define _VOIDP_TYPE UINT64
# define gclz(x) clz32(x)
#else
# error UNKNOWN POINTER SIZE
#endif

typedef _VOIDP_TYPE CB_NAME(char);
typedef _VOIDP_TYPE CB_NAME(string);

#ifdef cb_char
# undef cb_char
#endif
#define cb_char CB_NAME(char)

#ifdef cb_string
# undef cb_string
#endif
#define cb_string CB_NAME(string)

#define CB_ADD_KEY_REF(x)	do { } while(0)
#define CB_FREE_KEY(x)		do { } while(0)
#define CB_SET_KEY(node, x)	do { (node)->key = (x); } while (0)

#define CB_KEY_EQ(k1, k2)	( (k1).str == (k2).str )
#define CB_KEY_LT(k1, k2)	( (k1).str < (k2).str \
		  || (((k1).str == (k2).str) && CB_LT((k1).len, (k2).len)) )

#define CB_GET_CHAR(str, n)	(str)
#define CB_WIDTH(s)	(sizeof(cb_char)*8) /* width in byte */
#define CB_LENGTH(str)	1
#define CB_SIZE(key)	((key)->len)
#define CB_GET_BIT(str, size)		\
	(BITN(cb_char, CB_GET_CHAR((str), (size).chars), (size).bits))
#define CB_COUNT_PREFIX(s1, s2, n)	\
	((size_t)gclz(CB_GET_CHAR((s1), (n)) ^ CB_GET_CHAR((s2), (n))))

#define CB_PRINT_KEY(buf, key)		do \
	{ string_builder_sprintf((buf), "%p", (void*)((key).str)); } while(0)

static inline cb_size cb_prefix_count_pointer(const cb_string s1,
					      const cb_string s2,
					      const cb_size len,
					      const cb_size start) {
    cb_size ret;

    if (s1 == s2) return len;

    ret.chars = 0;
    ret.bits = (size_t)gclz(s1 ^ s2);

    if (!len.chars) ret.bits = MINIMUM(len.bits, ret.bits);

    return ret;
}

#define cb_prefix_count cb_prefix_count_pointer

#endif /* _CB_KEY_PTR_H */
