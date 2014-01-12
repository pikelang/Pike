#ifndef CB_KEY_PIKESTRING_H
#define CB_KEY_PIKESTRING_H
#include "bitvector.h"
#include "pike_int_types.h"

typedef struct pike_string * CB_NAME(string);
typedef p_wchar2 CB_NAME(char);

#ifdef cb_string
# undef cb_string
#endif
#define cb_string CB_NAME(string)

#ifdef cb_char
# undef cb_char
#endif
#define cb_char CB_NAME(char)

#define hton8(x) (x)
#define get_unaligned_be8(x) ((unsigned INT8 *)x)[0]

#ifdef CB_SOURCE
#define CB_ADD_KEY_REF(x)	do { if ((x).str) add_ref((x).str); } while(0)
#define CB_FREE_KEY(x)		do { if ((x).str) free_string((x).str); } while(0)
#define CB_SET_KEY(node, x)				\
	do { CB_ADD_KEY_REF(x);				\
	    CB_FREE_KEY((node)->key); (node)->key = (x); } while(0)

#define CB_KEY_EQ(k1, k2)	(CB_S_EQ((k1).len, (k2).len) && (k1).str == (k2).str)
#define CB_KEY_LT(k1, k2)				\
	(CB_LT((k1).len, (k2).len) && my_quick_strcmp((k1).str, (k2).str) < 0)

#define CB_GET_CHAR(s, n)	((cb_char)INDEX_CHARP((s)->str, (n), (s)->size_shift))
#define CB_WIDTH(s)	(1 << (3 + (s)->size_shift)) /* width in bits */
#define CB_LENGTH(str)	((str)->len)
#define CB_SIZE(key)	((key).len)
#define CB_GET_BIT(str, pos)				\
	(BITN(cb_char, CB_GET_CHAR((str), (pos).chars), (pos).bits))
static inline unsigned INT32 CB_COUNT_PREFIX(const cb_string s1, const cb_string s2, const size_t n) {
    cb_char c;
    c = CB_GET_CHAR(s1, n);
    c ^= CB_GET_CHAR(s2, n);

    return clz32(c);
}


static inline cb_size cb_prefix_count_fallback(const cb_string,
					    const cb_string,
					    const cb_size,
					    cb_size);

static inline cb_size cb_prefix_count_wide0(const cb_string s1,
					    const cb_string s2,
					    const cb_size len,
					    cb_size start) {
    size_t j = start.chars;
    const unsigned char * p1, * p2;

    p1 = STR0(s1);
    p2 = STR0(s2);

#if 0
    asm volatile(
    "pushf\t\n"
    "orl $(1<<18), (%esp)\t\n"
    "popf\t\n"
  );
#endif

#define PREFIX(n) do {						\
    size_t k = j/sizeof(unsigned INT ##n );				\
    for (;k < len.chars/sizeof(unsigned INT ##n );k++) {		\
        unsigned INT ##n x = get_unaligned_be ##n (p1 + k * sizeof(unsigned INT ##n))\
                           ^ get_unaligned_be ##n (p2 + k * sizeof(unsigned INT ##n));\
	if (x) {						\
	    unsigned INT32 a;						\
	    a = clz ##n(x);				\
	    start.chars = k*sizeof(unsigned INT ##n ) + a/8;	\
	    start.bits = 24 + a % 8;				\
	    return start;					\
	}							\
    }								\
    j = k*sizeof(unsigned INT ##n);				\
    } while(0)
#ifdef lzc64
   PREFIX(64);
#endif
   PREFIX(32);
    PREFIX(8);

#if 0
    asm volatile(
    "pushf\t\n"
    "andl $(~(1<<18)), (%esp)\t\n"
    "popf\t\n"
  );
#endif

    start.chars = j;

    if (len.bits) {
	unsigned INT32 a = clz8(((p_wchar0*)p1)[j]^((p_wchar0*)p2)[j]);
	start.bits = MINIMUM(len.bits, 24+a);
	return start;
    }

    start.bits = 0;
    return start;
}

static inline cb_size cb_prefix_count_widestring(const cb_string s1,
						 const cb_string s2,
						 const cb_size len,
						 const cb_size start) {
#if 0
    size_t i;
    cb_size t, r;
    unsigned char* chrptr = s1->str;;
    static volatile cb_string n1, n2;
    static volatile void* zero = NULL;
    static volatile cb_size *rp, *tp;

    n1 = s1;
    n2 = s2;

#endif
    if (CB_WIDTH(s1) == 8 && 8 == CB_WIDTH(s2))
	return cb_prefix_count_wide0(s1, s2, len, start);
#if 0
	if (!CB_S_EQ(r, t)) {
	    rp = &r;
	    tp = &t;
	    fprintf(stderr, "%p(%p) and %p(%p) have prefix %lu,%lu. should "
		    + "have %lu,%lu (len: %lu, %lu  start: %lu, %lu)\n",
		    s1->str, s1, s2->str, s2, t.chars, t.bits, r.chars,
		    r.bits, len.chars, len.bits, start.chars, start.bits);
	    fprintf(stderr, "here you die: %p, %lu\n", chrptr,
		    r.bits/(unsigned INT64)zero);
	    fprintf(stderr, "%p %p %p", zero, n1, n2);
	    n1 = n2 = (void*)(zero = (unsigned INT64)chrptr ^ (unsigned INT64)n1
			      ^ (unsigned INT64)n2);
	    fprintf(stderr, "%p %p %p %p %p %p", zero, n1, n2, rp, (void*)rp,
		    tp, (void*)tp);
	}
#endif


    return cb_prefix_count_fallback(s1, s2, len, start);
}

#define	cb_prefix_count	cb_prefix_count_widestring

#undef CB_WIDTH
#define CB_WIDTH(s)	(32)

#endif /* CB_SOURCE */

#endif
