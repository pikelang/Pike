#ifndef CB_KEY_PIKEINT_H
#define CB_KEY_PIKEINT_H
#include "bitvector.h"
#include "pike_int_types.h"
#include <gmp.h>
#include "bignum.h"

typedef struct object * CB_NAME(string);
typedef mp_limb_t CB_NAME(char);

#ifdef cb_string
# undef cb_string
#endif
#define cb_string CB_NAME(string)

#ifdef cb_char
# undef cb_char
#endif
#define cb_char CB_NAME(char)

static CB_INLINE unsigned INT32 gclz(mp_limb_t a) {
    if (sizeof(mp_limb_t) == 8) {
	return clz64((unsigned INT64)a);
    } else {
	return clz32((unsigned INT32)a);
    }
}

#define O2G(o) ((MP_INT*)(o->storage))
#define K2G(k) ((MP_INT*)((k).str->storage))

#define CB_ADD_KEY_REF(x)	do { if ((x).str) add_ref((x).str); } while(0)
#define CB_FREE_KEY(x)		do { if ((x).str) { free_object((x).str); (x).str = NULL; } } while(0)
#define CB_SET_KEY(node, x)				\
	do { CB_ADD_KEY_REF(x);				\
	    CB_FREE_KEY((node)->key); (node)->key = (x); } while(0)

#define CB_GC_CHECK_KEY(key)	gc_check((key).str)
#define CB_GC_RECURSE_KEY(key)	gc_recurse_object((key).str)

// int mpz_cmp (mpz_t op1, mpz_t op2)
#define CB_KEY_EQ(k1, k2)	( (k1).str == (k2).str || !mpz_cmp(K2G(k1), K2G(k2)) )
#define CB_KEY_LT(k1, k2)	( (k1).str != (k2).str && mpz_cmp(K2G(k1), K2G(k2)) < 0 )

static CB_INLINE mp_limb_t CB_GET_CHAR(cb_string s, ptrdiff_t n) {
    MP_INT * i = O2G(s);

    n += abs(i->_mp_size);
    if (n > 0) {
	//fprintf(stderr, ">> %lld %lld\n", n, i->_mp_d[abs(n)]);
	return i->_mp_d[abs(i->_mp_size)-n];
    } else {
	//fprintf(stderr, "»» %lld %lld\n", n, i->_mp_d[abs(n)]);
	return 0;
    }
}

#define CB_WIDTH(s)	(sizeof(cb_char)*8)
#define CB_LENGTH(str)	0
#define CB_SIZE(key)	(-abs(K2G(key)->_mp_size))
#define CB_GET_BIT(str, size)			\
	(BITN(cb_char, CB_GET_CHAR((str), (size).chars), (size).bits))
#define CB_COUNT_PREFIX(s1, s2, n)		\
	(gclz(CB_GET_CHAR((s1), (n)) ^ CB_GET_CHAR((s2), (n))))
#define CB_FIRST_CHAR(key)	(-abs(K2G(key)->_mp_size))

static inline cb_size cb_prefix_count_fallback(const cb_string s1,
					       const cb_string s2,
					       const cb_size len,
					       cb_size start);

static inline cb_size cb_prefix_count_bignum(const cb_string s1,
					     const cb_string s2,
					     const cb_size len,
					     cb_size start) {

    if (start.chars == 0 && start.bits == 0) {
	start.chars = -MAXIMUM(abs(O2G(s1)->_mp_size), abs(O2G(s2)->_mp_size));
    }

    return cb_prefix_count_fallback(s1, s2, len, start);
}

#define cb_prefix_count	cb_prefix_count_bignum

#endif
