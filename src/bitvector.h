#ifndef BITVECTOR_H
#define BITVECTOR_H

#include "machine.h"
#include "pike_int_types.h"

#if defined(HAS__BITSCANFORWARD) || defined(HAS__BITSCANFORWARD64) || defined(HAS__BITSCANREVERSE) || defined(HAS__BITSCANREVERSE64)
# include <intrin.h>
#endif

#if defined(HAS__BYTESWAP_ULONG) || defined(HAS__BYTESWAP_UINT64)
#include <stdlib.h>
#endif

#define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n
static const char logTable[256] = {
    0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
    LT(5), LT(6), LT(6), LT(7), LT(7), LT(7), LT(7),
    LT(8), LT(8), LT(8), LT(8), LT(8), LT(8), LT(8), LT(8)
};
#undef LT

static INLINE unsigned INT32 clz32(unsigned INT32 i) {
#ifdef HAS___BUILTIN_CLZ
    return i ? __builtin_clz(i) : 32;
#elif defined(HAS__BIT_SCAN_REVERSE)
    return i ? _bit_scan_reverse(i) : 32;
#elif defined(HAS___CNTLZ4)
    return i ? __cntlz4(i) : 32;
#elif defined(HAS__BITSCANREVERSE)
    unsigned long index;
    if (_BitScanReverse(&index, (unsigned long)i))
	return (unsigned INT32)index;
    return 32;
}
#else
    unsigned INT32 t;

    if ((t = i >> 24)) {
	return 8 - logTable[t];
    } else if ((t = i >> 16)) {
	return 16 - logTable[t];
    } else if ((t = i >> 8)) {
	return 24 - logTable[t];
    } else {
	return 32 - logTable[i];
    }
#endif
}

#define clz16(i) (clz32(i) - 16)
#define clz8(i) (clz32(i) - 24)

static INLINE unsigned INT32 ctz32(unsigned INT32 i) {
#ifdef HAS___BUILTIN_CTZ
    return i ? __builtin_ctz(i) : 32;
#elif defined(HAS__BIT_SCAN_FORWARD)
    return i ? _bit_scan_forward(i) : 32;
#elif defined(HAS___CNTTZ4)
    return i ? __cnttz4(i) : 32;
#elif defined(HAS__BITSCANFORWARD)
    unsigned long index;
    if (_BitScanForward(&index, (unsigned long)i))
	return (unsigned INT32)index;
    return 32;
#else
    /* this table maps (2^n % 37) to n */
    const char ctz32Table[37] = {
	32, 0, 1, 26, 2, 23, 27, -1, 3, 16, 24, 30, 28, 11, -1, 13, 4, 7, 17,
	-1, 25, 22, 31, 15, 29, 10, 12, 6, -1, 21, 14, 9, 5, 20, 8, 19, 18
    };
    return (unsigned INT32)ctz32Table[((unsigned INT32)((int)i & -(int)i))%37];
#endif
}

#define ctz16(i) (i ? ctz32(i) : 16)
#define ctz8(i) (i ? ctz32(i) : 8)


static INLINE unsigned INT32 bswap32(unsigned INT32 x) {
#ifdef HAS___BUILTIN_BSWAP32
    return __builtin_bswap32(x);
#elif defined(HAS__BSWAP)
    return _bswap(x);
#elif defined(HAS__BYTESWAP_ULONG)
    return _byteswap_ulong((unsigned long)x);
#else
    return (((x & 0xff000000) >> 24) | ((x & 0x000000ff) << 24)
            | ((x & 0x00ff0000) >> 8) | ((x & 0x0000ff00) << 8));
#endif
}

#ifdef INT64

static INLINE unsigned INT32 clz64(unsigned INT64 i) {
# if SIZEOF_LONG == 8 && defined(HAS___BUILTIN_CLZL)
    return i ? __builtin_clzl(i) : 64;
# elif SIZEOF_LONG_LONG == 8 && defined(HAS___BUILTIN_CLZLL)
    return i ? __builtin_clzll(i) : 64;
# elif defined(HAS___CNTLZ8)
    return i ? __cntlz8(i) : 64;
# elif defined(HAS__BITSCANREVERSE64)
    unsigned long index;
    if (_BitScanReverse64(&index, i))
	return index;
    return 64;
# else
    unsigned INT64 t;

    if ((t = i >> 56)) {
        return 8 - logTable[t];
    } else if ((t = i >> 48)) {
        return 16 - logTable[t];
    } else if ((t = i >> 40)) {
        return 24 - logTable[t];
    } else if ((t = i >> 32)) {
        return 32 - logTable[t];
    } else if ((t = i >> 24)) {
        return 40 - logTable[t];
    } else if ((t = i >> 16)) {
        return 48 - logTable[t];
    } else if ((t = i >> 8)) {
        return 56 - logTable[t];
    } else {
        return 64 - logTable[i];
    }
# endif
}

static INLINE unsigned INT32 ctz64(unsigned INT64 i) {
# if SIZEOF_LONG == 8 && defined(HAS___BUILTIN_CTZL)
    return i ? __builtin_ctzl(i) : 64;
# elif SIZEOF_LONG_LONG == 8 && defined(HAS___BUILTIN_CTZLL)
    return i ? __builtin_ctzll(i) : 64;
# elif defined(HAS___CNTTZ8)
    return i ? __cnttz8(i) : 64;
# elif defined(HAS__BITSCANFORWARD64)
    unsigned long index;
    if (_BitScanForward64(&index, i))
	return index;
    return 64;
# else
    /* this table maps (2^n % 67) to n */
    const char ctz64Table[67] = {
	64, 0, 1, 39, 2, 15, 40, 23, 3, 12, 16, 59, 41, 19, 24, 54, 4, 63, 13,
	10, 17, 62, 60, 28, 42, 30, 20, 51, 25, 44, 55, 47, 5, 32, 63, 38, 14,
	22, 11, 58, 18, 53, 63, 9, 61, 27, 29, 50, 43, 46, 31, 37, 21, 57, 52,
	8, 26, 49, 45, 36, 56, 7, 48, 35, 6, 34, 33
    };
    return (unsigned INT32)ctz64Table[((unsigned INT64)((INT64)i & -(INT64)i))%67];
# endif
}

static INLINE unsigned INT64 bswap64(unsigned INT64 x) {
#ifdef HAS___BUILTIN_BSWAP64
    return __builtin_bswap64(x);
#elif defined(HAS__BSWAP64)
    return _bswap64(x);
#elif defined(HAS__BYTESWAP_UINT64)
    return _byteswap_uint64((unsigned INT64)x);
#else
    return bswap32(x >> 32) | (unsigned INT64)bswap32(x & 0xffffffff) << 32;
#endif
}

static INLINE unsigned INT64 round_up64(unsigned INT64 v) {
    unsigned INT64 i;

    if (!v) return 0;

    return (unsigned INT64)1 << (64 - clz64(v));
}

static INLINE unsigned INT32 ffs64(unsigned INT64 v) {
    return ctz64(v) + 1;
}

static INLINE unsigned INT32 fls64(unsigned INT64 v) {
    return 64 - clz64(v);
}

/* returns -1 for 0 */
static INLINE unsigned INT32 log2_u64(unsigned INT64 v) {
    return fls64(v) - 1;
}
#endif /* INT64 */

static INLINE unsigned INT32 round_up32(unsigned INT32 v) {
    unsigned INT32 i;

    if (!v) return 0;

    return 1U << (32 - clz32(v));
}

static INLINE unsigned INT32 ffs32(unsigned INT32 v) {
    return ctz32(v) + 1;
}

static INLINE unsigned INT32 fls32(unsigned INT32 v) {
    return 32 - clz32(v);
}

/* returns -1 for 0 */
static INLINE unsigned INT32 log2_u32(unsigned INT32 v) {
    return fls32(v) - 1;
}

#define bswap16(x)     ((unsigned INT16)bswap32((unsigned INT32)x << 16))

#if PIKE_BYTEORDER == 1234
#define get_unaligned_le16 get_unaligned16
#define get_unaligned_le32 get_unaligned32
#define get_unaligned_le64 get_unaligned64
#define set_unaligned_le16 set_unaligned16
#define set_unaligned_le32 set_unaligned32
#define set_unaligned_le64 set_unaligned64
#define get_unaligned_be16(x) bswap16(get_unaligned16(x))
#define get_unaligned_be32(x) bswap32(get_unaligned32(x))
#define get_unaligned_be64(x) bswap64(get_unaligned64(x))
#define set_unaligned_be16(ptr, x) set_unaligned16(ptr, bswap16(x))
#define set_unaligned_be32(ptr, x) set_unaligned32(ptr, bswap32(x))
#define set_unaligned_be64(ptr, x) set_unaligned64(ptr, bswap64(x))
#define hton32(x) bswap32(x)
#define hton64(x) bswap64(x)
#else
#define get_unaligned_be16 get_unaligned16
#define get_unaligned_be32 get_unaligned32
#define get_unaligned_be64 get_unaligned64
#define set_unaligned_be16 set_unaligned16
#define set_unaligned_be32 set_unaligned32
#define set_unaligned_be64 set_unaligned64
#define get_unaligned_le16(x) bswap16(get_unaligned16(x))
#define get_unaligned_le32(x) bswap32(get_unaligned32(x))
#define get_unaligned_le64(x) bswap64(get_unaligned64(x))
#define set_unaligned_le16(ptr, x) set_unaligned16(ptr, bswap16(x))
#define set_unaligned_le32(ptr, x) set_unaligned32(ptr, bswap32(x))
#define set_unaligned_le64(ptr, x) set_unaligned64(ptr, bswap64(x))
#define hton32(x) (x)
#define hton64(x) (x)
#endif

#endif /* BITVECTOR_H */
