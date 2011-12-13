#ifndef _WIDESTRING_H
#define _WIDESTRING_H
#include <stdint.h>
#include "bitvector.h"
#include "stralloc.h"

typedef struct pike_string * CB_NAME(string);
typedef p_wchar2 CB_NAME(char);

#include "svalue_value.h"
#include "tree_low.h"

#define CB_DO_PRINTING

/* the following is used by tree_low */
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
static inline uint32_t CB_COUNT_PREFIX(const cb_string s1, const cb_string s2, const size_t n) {
    cb_char c;
    c = CB_GET_CHAR(s1, n);
    c ^= CB_GET_CHAR(s2, n);

    return clz32(c);
}
#if 0
#define CB_COUNT_PREFIX(s1, s2, n)			\
	(clz32(CB_GET_CHAR((s1), (n)) ^ CB_GET_CHAR((s2), (n))))
#endif
#endif /* CB_SOURCE */

/* this is only used in pike code! */
#define CB_PRINT_CHAR(buf, str, n)			\
    do { string_builder_sprintf((buf), "%c", CB_GET_CHAR(str, n)); } while(0)

#define CB_PRINT_KEY(buf, key)				\
	do { string_builder_shared_strcat((buf), (key).str); } while(0)
#define CB_LOW_ASSIGN_SVALUE_KEY(key, s)	do {	\
    struct pike_string * _key = (key).str;		\
    struct svalue * _svalue = (s);			\
    add_ref(_key);					\
    _svalue->subtype = 0;				\
    _svalue->u.string = _key;				\
    _svalue->type = PIKE_T_STRING;			\
} while(0)
#define CB_PUSH_KEY(key)	ref_push_string((key).str)
#define CB_PUSH_STRING(str)	ref_push_string(str)
#define CB_STRING_FROM_SVALUE(v)	((v)->u.string)
#define CB_LOW_KEY_FROM_SVALUE(v)	CB_KEY_FROM_STRING(CB_STRING_FROM_SVALUE(v))

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
    size_t k = j/sizeof(uint ##n ##_t);				\
    for (;k < len.chars/sizeof(uint ##n ##_t);k++) {		\
	uint ##n ##_t x = ((uint ##n ##_t *)p1)[k] 		\
			^ ((uint ##n ##_t *)p2)[k];	    	\
	if (x) {						\
	    uint32_t a;						\
	    a = clz ##n(hton ##n(x));				\
	    start.chars = k*sizeof(uint ##n ##_t) + a/8;	\
	    start.bits = 24 + a % 8;				\
	    return start;					\
	}							\
    }								\
    j = k*sizeof(uint ##n ##_t);				\
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
	uint32_t a = clz8(((p_wchar0*)p1)[j]^((p_wchar0*)p2)[j]);
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
		    r.bits/(uint64_t)zero);
	    fprintf(stderr, "%p %p %p", zero, n1, n2);
	    n1 = n2 = (void*)(zero = (uint64_t)chrptr ^ (uint64_t)n1
			      ^ (uint64_t)n2);
	    fprintf(stderr, "%p %p %p %p %p %p", zero, n1, n2, rp, (void*)rp,
		    tp, (void*)tp);
	}
#endif


    return cb_prefix_count_fallback(s1, s2, len, start);
}

#define	cb_prefix_count	cb_prefix_count_widestring

#undef CB_WIDTH
#define CB_WIDTH(s)	(32)

#endif
