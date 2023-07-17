/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "string.h"

#ifdef __i386__
ATTRIBUTE((fastcall))
#endif
PIKE_HOT_ATTRIBUTE
PMOD_EXPORT UINT64 low_hashmem_siphash24( const void *s, size_t len, size_t nbytes,
                                                  UINT64 key );

#if PIKE_BYTEORDER == 1234
ATTRIBUTE((unused))
static inline UINT64 low_hashmem_siphash24_uint32( const unsigned INT32 *in, size_t len,
                                                           UINT64 key ) {
    return low_hashmem_siphash24(in, len*4, len*4, key);
}

ATTRIBUTE((unused))
static inline UINT64 low_hashmem_siphash24_uint16( const unsigned INT16 *in, size_t len,
                                                           UINT64 key ) {
    return low_hashmem_siphash24(in, len*2, len*2, key);
}

#elif PIKE_BYTEORDER == 4321
ATTRIBUTE((unused))
#ifdef __i386__
ATTRIBUTE((fastcall))
#endif
PIKE_HOT_ATTRIBUTE
PMOD_EXPORT UINT64 low_hashmem_siphash24_uint32( const unsigned INT32 *in, size_t len,
                                                         UINT64 key );

#ifdef __i386__
ATTRIBUTE((fastcall))
#endif
PIKE_HOT_ATTRIBUTE
PMOD_EXPORT UINT64 low_hashmem_siphash24_uint16( const unsigned INT16 *in, size_t len,
                                                         UINT64 key );
#else
#error siphash24 only supports big-endian and little-endian
#endif

ATTRIBUTE((unused))
static inline UINT64 pike_string_siphash24(const struct pike_string *s, UINT64 key) {
  if (s->size_shift == eightbit) {
    return low_hashmem_siphash24(STR0(s), s->len, s->len, key);
  } else if (s->size_shift == sixteenbit) {
    return low_hashmem_siphash24_uint16(STR1(s), s->len, key);
  } else {
    return low_hashmem_siphash24_uint32((unsigned INT32*)STR2(s), s->len, key);
  }
}
