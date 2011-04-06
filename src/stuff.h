/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#ifndef STUFF_H
#define STUFF_H

#include "global.h"
#include "pike_macros.h"

/* Prototypes begin here */
PMOD_EXPORT int my_log2(size_t x) ATTRIBUTE((const));
PMOD_EXPORT int count_bits(unsigned INT32 x) ATTRIBUTE((const));
PMOD_EXPORT int is_more_than_one_bit(unsigned INT32 x) ATTRIBUTE((const));
PMOD_EXPORT double my_strtod(char *nptr, char **endptr);
PMOD_EXPORT unsigned INT32 my_sqrt(unsigned INT32 n) ATTRIBUTE((const));
unsigned long find_good_hash_size(unsigned long x) ATTRIBUTE((const));
unsigned INT32 find_next_power(unsigned INT32 x) ATTRIBUTE((const));
/* Prototypes end here */

PMOD_EXPORT extern const INT32 hashprimes[32];

/*
 * This simple mixing function comes from the java HashMap implementation.
 * It makes sure that the resulting hash value can be used for hash tables
 * with power of 2 size and simple masking instead of modulo prime.
 */
static INLINE unsigned INT32 my_mix(unsigned INT32 x) {
  x ^= (x >> 20) ^ (x >> 12);
  return x ^ (x >> 7) ^ (x >> 4);
}

#define hash_int32(x)	my_mix((unsigned INT32)(x))
#define hash_int64(x)	my_mix((unsigned INT32)(x) ^ (unsigned INT32)((x) << 32))

#if SIZEOF_INT_TYPE == 8
# define hash_int(x)	hash_int64(x)
#else
# define hash_int(x)	hash_int32(x)
#endif

static INLINE unsigned INT32 hash_pointer(void * ptr) {
#if SIZEOF_CHAR_P == 8
    return hash_int64(PTR_TO_INT(ptr));
#else
    return hash_int32(PTR_TO_INT(ptr));
#endif
}

#endif
