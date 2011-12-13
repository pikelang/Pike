#ifndef BITVECTOR_H
#define BITVECTOR_H

#include "critbit_config.h"
#define MOD_PRIME

#include <stdint.h>

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

static const unsigned char LMultiplyDeBruijnBitPosition32[33] =
{
    32,
    31, 22, 30, 21, 18, 10, 29,  2,
    20, 17, 15, 13,  9,  6, 28,  1,
    23, 19, 11,  3, 16, 14,  7, 24,
    12,  4,  8, 25,  5, 26, 27,  0,
/*
     0,  1, 28,  2, 29, 14, 24,  3,
    30, 22, 20, 15, 25, 17,  4,  8,
    31, 27, 13, 23, 21, 19, 16,  7,
    26, 12, 18,  6, 11,  5, 10,  9
*/
};
static const unsigned char TMultiplyDeBruijnBitPosition32[33] =
{
    32,
    31, 30,  3, 29,  2, 17,  7, 28,
     1,  9, 11, 16,  6, 14, 27, 23,
     0,  4, 18,  8, 10, 12, 15, 24,
     5, 19, 13, 25, 20, 26, 21, 22
};
static const unsigned char LMultiplyDeBruijnBitPosition64[65] = {
    64,
};
static const unsigned char TMultiplyDeBruijnBitPosition64[65] = {
    64,
     0, 63,  5, 62,  4, 16, 10, 61,
     3, 24, 15, 36,  9, 30, 21, 60,
     2, 12, 26, 23, 14, 45, 35, 43,
     8, 33, 29, 52, 20, 49, 41, 59,
     1,  6, 17, 11, 25, 37, 31, 22,
    13, 27, 46, 44, 34, 53, 50, 42,
     7, 18, 38, 32, 28, 47, 54, 51,
    19, 39, 48, 55, 40, 56, 57, 58
};
/* maps 2^n - 1 -> n
 * returns 32 is none is set.
 */
static inline uint32_t leading_bit_pos32(uint32_t v) {
    return LMultiplyDeBruijnBitPosition32[!!v + (((uint32_t)((v) * 0x07C4ACDDU)) >> 27)];
}
static inline uint32_t trailing_bit_pos32(uint32_t v) {
    return TMultiplyDeBruijnBitPosition32[!!v + (((uint32_t)((v) * 0x077CB531U)) >> 27)];
}
static inline uint64_t leading_bit_pos64(uint64_t v) {
    /*
     * v * M == (2*v - 1) * N  + N = v * (2*N)
     * N = M/2;
     */
    const uint64_t N = 0x3F6EAF2CD271461UL;

    return LMultiplyDeBruijnBitPosition64[!!v + (((uint64_t)((v) * N + N)) >> 58)];
}
static inline uint64_t trailing_bit_pos64(uint64_t v) {
    return TMultiplyDeBruijnBitPosition64[!!v + (((uint64_t)((v) * 0x07EDD5E59A4E28C2UL)) >> 58)];
}

static inline uint32_t round_up32_(uint32_t v) {
    v |= v >> 1; /* first round down to one less than a power of 2 */
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    return v;
}

static inline uint32_t round_up32(uint32_t v) {
    v = round_up32_(v);
    v++;
    return v;
}

static inline uint32_t round_down32(uint32_t v) {
    v--;
    return round_up32(v);
}

static inline uint64_t round_up64_(uint64_t v) {
    v |= v >> 1; /* first round down to one less than a power of 2 */
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v |= v >> 32;
    return v;
}

static inline uint64_t round_up64(uint64_t v) {
    v = round_up64_(v);
    v++;
    return v;
}

static inline uint64_t round_down64(uint64_t v) {
    v--;
    return round_up64(v);
}

#ifdef HAS___BUILTIN_CLZ
# define clz32(i) ((uint32_t)(!(i) ? 32 : __builtin_clz((uint32_t)i)))
#elif defined(HAS__BIT_SCAN_REVERSE)
# define clz32(i) ((uint32_t)(!(i) ? 32 : _bit_scan_reverse((int)i)))
#elif defined(HAS___CNTLZ4)
# define clz32(i) ((uint32_t)__cntlz4(i))
#elif defined(HAS__BITSCANREVERSE)
static inline uint32_t clz32(const uint32_t i) {
    static unsigned long index;
    if (_BitScanReverse(&index, (unsigned long)i))
	return (uint32_t)index;
    return 32;
}
#else
static inline uint32_t clz32(const uint32_t y) {
#ifdef MOD_PRIME
    register uint32_t t;

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
    return leading_bit_pos32(round_up32_(y));
#endif
}
#endif

#define clz16(i) ((uint32_t)(!(i) ? 16 : clz32((uint32_t)i)-16))
#define clz8(i) ((char)(!(i) ? 8 : clz32((uint32_t)i)-24))

#ifdef HAS___BUILTIN_CTZ
# define ctz32(i) ((uint32_t)(!(i) ? 32 : __builtin_ctz((uint32_t)i)))
#elif defined(HAS__BIT_SCAN_FORWARD)
# define ctz32(i) ((uint32_t)(!(i) ? 32 : _bit_scan_forward((int)i)))
#elif defined(HAS___CNTTZ4)
# define ctz32(i) ((uint32_t)__cnttz4(i))
#elif defined(HAS__BITSCANFORWARD)
static inline uint32_t ctz32(const uint32_t i) {
    static unsigned long index;
    if (_BitScanForward(&index, (unsigned long)i))
	return (uint32_t)index;
    return 32;
}
#else
static inline uint32_t ctz32(const uint32_t i) {
#ifdef MOD_PRIME
    /* this table maps (2^n % 37) to n */
    static const char ctz32Table[37] = {
	32, 0, 1, 26, 2, 23, 27, -1, 3, 16, 24, 30, 28, 11, -1, 13, 4, 7, 17,
	-1, 25, 22, 31, 15, 29, 10, 12, 6, -1, 21, 14, 9, 5, 20, 8, 19, 18
    };
    return (uint32_t)ctz32Table[((uint32_t)((int)i & -(int)i))%37];
#else
    return trailing_bit_pos32((uint32_t)((int)i & -(int)i));
#endif
}
#endif

#define ctz16(i) ((uint32_t)(!(i) ? 16 : ctz32((uint32_t)i)-16))
#define ctz8(i) ((char)(!(i) ? 8 : ctz32((uint32_t)i)-24))

#if SIZEOF_LONG == 8 || SIZEOF_LONG_LONG == 8

# if SIZEOF_LONG == 8 && defined(HAS___BUILTIN_CLZL)
#  define clz64(x)	((uint32_t)(!(x) ? 64\
			       : __builtin_clzl((unsigned long)(x))))
# elif SIZEOF_LONG_LONG == 8 && defined(HAS___BUILTIN_CLZLL)
#  define clz64(x)	((uint32_t)(!(x) ? 64\
			       : __builtin_clzll((unsigned long long)(x))))
# elif defined(HAS___CNTLZ8)
#  define clz64(x)	((uint32_t)__cntlz8(x))
# elif defined(HAS__BITSCANREVERSE64)
static inline uint32_t clz64(const uint64_t i) {
    static unsigned long index;
    if (_BitScanReverse64(&index, (__int64)i))
	return (uint32_t)index;
    return 64;
}
# else
static inline uint32_t clz64(const uint64_t y) {
#ifdef MOD_PRIME
    register uint64_t t;

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
    return leading_bit_pos64(round_up64(y));
# endif
}
# endif

# if SIZEOF_LONG == 8 && defined(HAS___BUILTIN_CTZL)
#  define ctz64(x)	((uint32_t)(!(x) ? 64\
			       : __builtin_ctzl((unsigned long)(x))))
/* #  warning __builtin_ctzl used */
# elif SIZEOF_LONG_LONG == 8 && defined(HAS___BUILTIN_CTZLL)
#  define ctz64(x)	((uint32_t)(!(x) ? 64\
			       : __builtin_ctzll((unsigned long long)(x))))
/* #  warning __builtin_ctzll used */
# elif defined(HAS___CNTTZ8)
#  define ctz64(x)	((uint32_t)__cnttz8(x))
/* #  warning __cnttz8 used */
# elif defined(HAS__BITSCANFORWARD64)
static inline uint32_t ctz64(const uint64_t i) {
    static unsigned long index;
    if (_BitScanForward64(&index, (__int64)i))
	return (uint32_t)index;
    return 64;
}
# else
static inline uint32_t ctz64(const uint64_t i) {
#ifdef MOD_PRIME
    /* this table maps (2^n % 67) to n */
    static const char ctz64Table[67] = {
	64, 0, 1, 39, 2, 15, 40, 23, 3, 12, 16, 59, 41, 19, 24, 54, 4, 63, 13,
	10, 17, 62, 60, 28, 42, 30, 20, 51, 25, 44, 55, 47, 5, 32, 63, 38, 14,
	22, 11, 58, 18, 53, 63, 9, 61, 27, 29, 50, 43, 46, 31, 37, 21, 57, 52,
	8, 26, 49, 45, 36, 56, 7, 48, 35, 6, 34, 33
    };
    return (uint32_t)ctz64Table[((uint64_t)((int64_t)i & -(int64_t)i))%67];
#else
    register uint64_t v = (uint64_t)((int64_t)i & -(int64_t)i);
    return trailing_bit_pos64(v);
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
#   define hton64(x)	((int64_t)hton32((int)((x) >> 32))\
	      | (((int64_t)hton32((int)((x) & 0x00000000ffffffff))) << 32))
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
