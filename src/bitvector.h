#ifndef BITVECTOR_H
#define BITVECTOR_H

#include "machine.h"
#define MOD_PRIME

#include "pike_int_types.h"

#if defined(HAS__BITSCANFORWARD) || defined(HAS__BITSCANFORWARD64) || defined(HAS__BITSCANREVERSE) || defined(HAS__BITSCANREVERSE64)
# include <intrin.h>
#endif

#if defined(HAS__BYTESWAP_ULONG) || defined(HAS__BYTESWAP_UINT64)
#include <stdlib.h>
#endif

#define MASK(type, bits)	(~((~((type)0)) >> (bits)))
#define BITMASK(type, n)	((type)1 << (type)(sizeof(type)*8 - 1 - (n)))
#define BITN(type, p, n)	(!!((p) & BITMASK(type, n)))

#define TBITMASK(type, n)	((type)1 << (type)((n)))
#define TBITN(type, p, n)	(!!((p) & TBITMASK(type, n)))

#if 1
# define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n
static const char logTable[256] = {
    0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
    LT(5), LT(6), LT(6), LT(7), LT(7), LT(7), LT(7),
    LT(8), LT(8), LT(8), LT(8), LT(8), LT(8), LT(8), LT(8)
};
# undef LT
#endif

static const unsigned char LMultiplyDeBruijnBitPosition32[32] =
{
    31, 22, 30, 21, 18, 10, 29,  2,
    20, 17, 15, 13,  9,  6, 28,  1,
    23, 19, 11,  3, 16, 14,  7, 24,
    12,  4,  8, 25,  5, 26, 27,  0,
};
/* maps 2^n - 1 -> n
 */
static inline unsigned INT32 leading_bit_pos32(const unsigned INT32 v) {
    return LMultiplyDeBruijnBitPosition32[(((unsigned INT32)((v) * 0x07C4ACDDU)) >> 27)];
}
static inline unsigned INT32 trailing_bit_pos32(const unsigned INT32 v) {
    return 31 - leading_bit_pos32(v);
}
#if SIZEOF_LONG == 8 || SIZEOF_LONG_LONG == 8

static const unsigned char TMultiplyDeBruijnBitPosition64[64] = {
     0, 63,  5, 62,  4, 16, 10, 61,
     3, 24, 15, 36,  9, 30, 21, 60,
     2, 12, 26, 23, 14, 45, 35, 43,
     8, 33, 29, 52, 20, 49, 41, 59,
     1,  6, 17, 11, 25, 37, 31, 22,
    13, 27, 46, 44, 34, 53, 50, 42,
     7, 18, 38, 32, 28, 47, 54, 51,
    19, 39, 48, 55, 40, 56, 57, 58
};
static inline unsigned INT64 trailing_bit_pos64(const unsigned INT64 v) {
    return TMultiplyDeBruijnBitPosition64[(((unsigned INT64)((v) * 0x07EDD5E59A4E28C2ULL)) >> 58)];
}

static inline unsigned INT64 leading_bit_pos64(const unsigned INT64 v) {
    return 63 - trailing_bit_pos64(v);
}
#endif

static inline unsigned INT32 round_up32_(unsigned INT32 v) {
    v |= v >> 1; /* first round down to one less than a power of 2 */
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    return v;
}

static inline unsigned INT32 round_up32(const unsigned INT32 v) {
    return round_up32_(v)+1;
}

static inline unsigned INT32 round_down32(const unsigned INT32 v) {
    return round_up32(v-1);
}

static inline unsigned INT64 round_up64_(unsigned INT64 v) {
    v |= v >> 1; /* first round down to one less than a power of 2 */
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v |= v >> 32;
    return v;
}

static inline unsigned INT64 round_up64(const unsigned INT64 v) {
    return round_up64_(v) + 1;
}

static inline unsigned INT64 round_down64(const unsigned INT64 v) {
    return round_up64(v-1);
}

#ifdef HAS___BUILTIN_CLZ
# define clz32(i) ((unsigned INT32)(!(i) ? 32 : __builtin_clz((unsigned INT32)i)))
#elif defined(HAS__BIT_SCAN_REVERSE)
# define clz32(i) ((unsigned INT32)(!(i) ? 32 : _bit_scan_reverse((int)i)))
#elif defined(HAS___CNTLZ4)
# define clz32(i) ((unsigned INT32)__cntlz4(i))
#elif defined(HAS__BITSCANREVERSE)
static inline unsigned INT32 clz32(const unsigned INT32 i) {
    static unsigned long index;
    if (_BitScanReverse(&index, (unsigned long)i))
	return (unsigned INT32)index;
    return 32;
}
#else
static inline unsigned INT32 clz32(const unsigned INT32 y) {
#ifdef MOD_PRIME
    register unsigned INT32 t;

    if ((t = y >> 24)) {
	return 8 - logTable[t];
    } else if ((t = y >> 16)) {
	return 16 - logTable[t];
    } else if ((t = y >> 8)) {
	return 24 - logTable[t];
    } else {
	return 32 - logTable[y];
    }
#else
    return y ? leading_bit_pos32(round_up32_(y)) : 32;
#endif
}
#endif

#define clz16(i) ((unsigned INT32)(!(i) ? 16 : clz32((unsigned INT32)i)-16))
#define clz8(i) ((char)(!(i) ? 8 : clz32((unsigned INT32)i)-24))

#ifdef HAS___BUILTIN_CTZ
# define ctz32(i) ((unsigned INT32)(!(i) ? 32 : __builtin_ctz((unsigned INT32)i)))
#elif defined(HAS__BIT_SCAN_FORWARD)
# define ctz32(i) ((unsigned INT32)(!(i) ? 32 : _bit_scan_forward((int)i)))
#elif defined(HAS___CNTTZ4)
# define ctz32(i) ((unsigned INT32)__cnttz4(i))
#elif defined(HAS__BITSCANFORWARD)
static inline unsigned INT32 ctz32(const unsigned INT32 i) {
    static unsigned long index;
    if (_BitScanForward(&index, (unsigned long)i))
	return (unsigned INT32)index;
    return 32;
}
#else
static inline unsigned INT32 ctz32(const unsigned INT32 i) {
#ifdef MOD_PRIME
    /* this table maps (2^n % 37) to n */
    static const char ctz32Table[37] = {
	32, 0, 1, 26, 2, 23, 27, -1, 3, 16, 24, 30, 28, 11, -1, 13, 4, 7, 17,
	-1, 25, 22, 31, 15, 29, 10, 12, 6, -1, 21, 14, 9, 5, 20, 8, 19, 18
    };
    return (unsigned INT32)ctz32Table[((unsigned INT32)((int)i & -(int)i))%37];
#else
    return i ? trailing_bit_pos32((unsigned INT32)((int)i & -(int)i)) : 32;
#endif
}
#endif

#define ctz16(i) ((unsigned INT32)(!(i) ? 16 : ctz32((unsigned INT32)i)-16))
#define ctz8(i) ((char)(!(i) ? 8 : ctz32((unsigned INT32)i)-24))

#if SIZEOF_LONG == 8 || SIZEOF_LONG_LONG == 8

# if SIZEOF_LONG == 8 && defined(HAS___BUILTIN_CLZL)
#  define clz64(x)	((unsigned INT32)(!(x) ? 64\
			       : __builtin_clzl((unsigned long)(x))))
# elif SIZEOF_LONG_LONG == 8 && defined(HAS___BUILTIN_CLZLL)
#  define clz64(x)	((unsigned INT32)(!(x) ? 64\
			       : __builtin_clzll((unsigned long long)(x))))
# elif defined(HAS___CNTLZ8)
#  define clz64(x)	((unsigned INT32)__cntlz8(x))
# elif defined(HAS__BITSCANREVERSE64)
static inline unsigned INT32 clz64(const unsigned INT64 i) {
    static unsigned long index;
    if (_BitScanReverse64(&index, (__int64)i))
	return (unsigned INT32)index;
    return 64;
}
# else
static inline unsigned INT32 clz64(const unsigned INT64 y) {
#ifdef MOD_PRIME
    register unsigned INT64 t;

    if ((t = y >> 56)) {
        return 8 - logTable[t];
    } else if ((t = y >> 48)) {
        return 16 - logTable[t];
    } else if ((t = y >> 40)) {
        return 24 - logTable[t];
    } else if ((t = y >> 32)) {
        return 32 - logTable[t];
    } else if ((t = y >> 24)) {
        return 40 - logTable[t];
    } else if ((t = y >> 16)) {
        return 48 - logTable[t];
    } else if ((t = y >> 8)) {
        return 56 - logTable[t];
    } else {
        return 64 - logTable[y];
    }
#else
    /* This code comes from http://graphics.stanford.edu/~seander/bithacks.html */
    return y ? leading_bit_pos64(round_up64(y)) : 64;
# endif
}
# endif

# if SIZEOF_LONG == 8 && defined(HAS___BUILTIN_CTZL)
#  define ctz64(x)	((unsigned INT32)(!(x) ? 64\
			       : __builtin_ctzl((unsigned long)(x))))
/* #  warning __builtin_ctzl used */
# elif SIZEOF_LONG_LONG == 8 && defined(HAS___BUILTIN_CTZLL)
#  define ctz64(x)	((unsigned INT32)(!(x) ? 64\
			       : __builtin_ctzll((unsigned long long)(x))))
/* #  warning __builtin_ctzll used */
# elif defined(HAS___CNTTZ8)
#  define ctz64(x)	((unsigned INT32)__cnttz8(x))
/* #  warning __cnttz8 used */
# elif defined(HAS__BITSCANFORWARD64)
static inline unsigned INT32 ctz64(const unsigned INT64 i) {
    static unsigned long index;
    if (_BitScanForward64(&index, (__int64)i))
	return (unsigned INT32)index;
    return 64;
}
# else
static inline unsigned INT32 ctz64(const unsigned INT64 i) {
#ifdef MOD_PRIME
    /* this table maps (2^n % 67) to n */
    static const char ctz64Table[67] = {
	64, 0, 1, 39, 2, 15, 40, 23, 3, 12, 16, 59, 41, 19, 24, 54, 4, 63, 13,
	10, 17, 62, 60, 28, 42, 30, 20, 51, 25, 44, 55, 47, 5, 32, 63, 38, 14,
	22, 11, 58, 18, 53, 63, 9, 61, 27, 29, 50, 43, 46, 31, 37, 21, 57, 52,
	8, 26, 49, 45, 36, 56, 7, 48, 35, 6, 34, 33
    };
    return (unsigned INT32)ctz64Table[((unsigned INT64)((INT64)i & -(INT64)i))%67];
#else
    return i ? trailing_bit_pos64((unsigned INT64)((INT64)i & -(INT64)i)) : 64;
#endif
}
# endif

#endif /* SIZEOF_LONG == 8 || SIZEOF_LONG_LONG == 8 */

#define hton8(x)	(x)

#if !(defined(PIKE_BYTEORDER))
# error "Byte order could not be decided."
#else
# if PIKE_BYTEORDER == 1234
/* # warning "little endian" */
#  define LBITMASK(type, n)	((type)1 << (type)(n))
#  define LBITN(type, p, n)	(!!((p) & LBITMASK(type, n)))
#  ifdef HAS___BUILTIN_BSWAP32
#   define hton32(x)	__builtin_bswap32(x)
#  elif defined(HAS__BSWAP)
#   define hton32(x)	_bswap(x)
#  elif defined(HAS__BYTESWAP_ULONG)
#   define hton32(x)	_byteswap_ulong((unsigned long)x)
#  else
#   define hton32(x)	((((x) & 0xff000000) >> 24) | (((x) & 0x000000ff) << 24) \
			 | (((x) & 0x00ff0000) >> 8) | (((x) & 0x0000ff00) << 8))
#  endif
#  ifdef HAS___BUILTIN_BSWAP64
#   define hton64(x)	__builtin_bswap64(x)
#  elif defined(HAS__BSWAP64)
#   define hton64(x)	_bswap64(x)
#  elif defined(HAS__BYTESWAP_UINT64)
#   define hton64(x)	_byteswap_uint64((unsigned __int64)x)
#  else
#   define hton64(x)	((INT64)hton32((int)((x) >> 32))\
	      | (((INT64)hton32((int)((x) & 0x00000000ffffffff))) << 32))
#  endif
# else /* PIKE_BYTEORDER = 1234 */
/* # warning "big endian" */
#  define hton64(x)	(x)
#  define hton32(x)	(x)
#  define LBITMASK(type, n)	((type)1 << (type)((sizeof(type)	\
				    - ((type)(n)<<8) - 1)*8 + (type)(n)&7))
#  define LBITN(type, p, n)	(!!((p) & LBITMASK(type, n)))
# endif /* PIKE_BYTEORDER == 1234 */
#endif

#endif /* BITVECTOR_H */
