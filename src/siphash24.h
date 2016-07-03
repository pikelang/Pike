#include "global.h"
#include "string.h"

#ifdef __i386__
ATTRIBUTE((fastcall))
#endif
ATTRIBUTE((hot))
PMOD_EXPORT unsigned INT64 low_hashmem_siphash24( const void *s, size_t len, size_t nbytes,
                                                  unsigned INT64 key );

#if PIKE_BYTEORDER == 1234
ATTRIBUTE((unused))
static inline unsigned INT64 low_hashmem_siphash24_uint32( const unsigned INT32 *in, size_t len,
                                                           unsigned INT64 key ) {
    return low_hashmem_siphash24(in, len*4, len*4, key);
}

ATTRIBUTE((unused))
static inline unsigned INT64 low_hashmem_siphash24_uint16( const unsigned INT16 *in, size_t len,
                                                           unsigned INT64 key ) {
    return low_hashmem_siphash24(in, len*2, len*2, key);
}

#else
ATTRIBUTE((unused))
#ifdef __i386__
ATTRIBUTE((fastcall))
#endif
ATTRIBUTE((hot))
PMOD_EXPORT unsigned INT64 low_hashmem_siphash24_uint32( const unsigned INT32 *in, size_t len,
                                                         unsigned INT64 key );

#ifdef __i386__
ATTRIBUTE((fastcall))
#endif
ATTRIBUTE((hot))
PMOD_EXPORT unsigned INT64 low_hashmem_siphash24_uint16( const unsigned INT16 *in, size_t len,
                                                         unsigned INT64 key );
#endif

ATTRIBUTE((unused))
static inline unsigned INT64 pike_string_siphash24(const struct pike_string *s, unsigned INT64 key) {
  if (s->size_shift == eightbit) {
    return low_hashmem_siphash24(STR0(s), s->len, s->len, key);
  } else if (s->size_shift == sixteenbit) {
    return low_hashmem_siphash24_uint16(STR1(s), s->len, key);
  } else {
    return low_hashmem_siphash24_uint32((unsigned INT32*)STR2(s), s->len, key);
  }
}
