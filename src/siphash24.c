/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

/*
 * The following is the reference implementation of siphash as of
 * november 2013. All tests have been removed from the file and the
 * function signatures have been changed to match the one used by pike.
 * It also uses the pike int types instead of stdint.h.
 *
 * The 16bit and 32bit variants have been added as architecture invariant
 * versions for wide strings.
 */

/*
   SipHash reference C implementation

   Written in 2012 by
   Jean-Philippe Aumasson <jeanphilippe.aumasson@gmail.com>
   Daniel J. Bernstein <djb@cr.yp.to>

   To the extent possible under law, the author(s) have dedicated all copyright
   and related and neighboring rights to this software to the public domain
   worldwide. This software is distributed without any warranty.

   You should have received a copy of the CC0 Public Domain Dedication along with
   this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/

#include "bitvector.h"
#include "pike_macros.h"
#include "stralloc.h"
#include "siphash24.h"

#define ROTL(x,b) (UINT64)( ((x) << (b)) | ( (x) >> (64 - (b))) )

#define U8TO64_LE(p)    get_unaligned_le64(p)
#define U16TO64_LE(p)   ((UINT64)p[0] | ((UINT64)p[1] << 16) |\
                         ((UINT64)p[2] << 32) | ((UINT64)p[3] << 48))
#define U32TO64_LE(p)   ((UINT64)p[0] | ((UINT64)p[1] << 32))

#define SIPROUND                                        \
  do {                                                  \
    v0 += v1; v1=ROTL(v1,13); v1 ^= v0; v0=ROTL(v0,32); \
    v2 += v3; v3=ROTL(v3,16); v3 ^= v2;                 \
    v0 += v3; v3=ROTL(v3,21); v3 ^= v0;                 \
    v2 += v1; v1=ROTL(v1,17); v1 ^= v2; v2=ROTL(v2,32); \
  } while(0)

/* SipHash-2-4 */
#ifdef __i386__
ATTRIBUTE((fastcall))
#endif
PIKE_HOT_ATTRIBUTE
PMOD_EXPORT UINT64 low_hashmem_siphash24( const void *s, size_t len, size_t nbytes,
					  UINT64 key )
{
  const unsigned char * in = (const unsigned char*)s;
  unsigned long long inlen = MINIMUM(len, nbytes);

  /* "somepseudorandomlygeneratedbytes" */
  UINT64 v0 = 0x736f6d6570736575ULL;
  UINT64 v1 = 0x646f72616e646f6dULL;
  UINT64 v2 = 0x6c7967656e657261ULL;
  UINT64 v3 = 0x7465646279746573ULL;
  UINT64 b;
  UINT64 k0 = key;
  UINT64 k1 = key;
  UINT64 m;
  const unsigned char *end = in + inlen - ( inlen % sizeof( UINT64 ) );
  const int left = inlen & 7;
  b = ( ( UINT64 )inlen ) << 56;
  v3 ^= k1;
  v2 ^= k0;
  v1 ^= k1;
  v0 ^= k0;

  for ( ; in != end; in += 8 )
  {
    m = U8TO64_LE( in );
    v3 ^= m;
    SIPROUND;
    SIPROUND;
    v0 ^= m;
  }

  switch( left )
  {
  case 7: b |= ( ( UINT64 )in[ 6] )  << 48; /* FALLTHRU */

  case 6: b |= ( ( UINT64 )in[ 5] )  << 40; /* FALLTHRU */

  case 5: b |= ( ( UINT64 )in[ 4] )  << 32; /* FALLTHRU */

  case 4: b |= ( ( UINT64 )in[ 3] )  << 24; /* FALLTHRU */

  case 3: b |= ( ( UINT64 )in[ 2] )  << 16; /* FALLTHRU */

  case 2: b |= ( ( UINT64 )in[ 1] )  <<  8; /* FALLTHRU */

  case 1: b |= ( ( UINT64 )in[ 0] ); break;

  case 0: break;
  }

  v3 ^= b;
  SIPROUND;
  SIPROUND;
  v0 ^= b;
  v2 ^= 0xff;
  SIPROUND;
  SIPROUND;
  SIPROUND;
  SIPROUND;
  b = v0 ^ v1 ^ v2  ^ v3;
  return b;
}

/*
 * These 16 and 32 bit versions are only necessary on big endian machines,
 * on little endian machines we can simply call the standard version with adjusted length.
 * 
 */

#if PIKE_BYTEORDER == 4321
#ifdef __i386__
ATTRIBUTE((fastcall))
#endif
PIKE_HOT_ATTRIBUTE
PMOD_EXPORT UINT64 low_hashmem_siphash24_uint16( const unsigned INT16 *in, size_t len, UINT64 key )
{
  /* "somepseudorandomlygeneratedbytes" */
  UINT64 v0 = 0x736f6d6570736575ULL;
  UINT64 v1 = 0x646f72616e646f6dULL;
  UINT64 v2 = 0x6c7967656e657261ULL;
  UINT64 v3 = 0x7465646279746573ULL;
  UINT64 b;
  UINT64 k0 = key;
  UINT64 k1 = key;
  UINT64 m;
  const unsigned int mod = (sizeof( UINT64 )/sizeof(unsigned INT16));
  const int left = len % mod;
  const unsigned INT16 *end = in + len - left;
  b = ( ( UINT64 )len ) << 57;
  v3 ^= k1;
  v2 ^= k0;
  v1 ^= k1;
  v0 ^= k0;

  for ( ; in != end; in += mod )
  {
    m = U16TO64_LE( in );
    v3 ^= m;
    SIPROUND;
    SIPROUND;
    v0 ^= m;
  }

  switch( left )
  {
  case 3: b |= ( ( UINT64 )in[ 2] )  << 32; /* FALLTHRU */

  case 2: b |= ( ( UINT64 )in[ 1] )  << 16; /* FALLTHRU */

  case 1: b |= ( ( UINT64 )in[ 0] ); break;

  case 0: break;
  }

  v3 ^= b;
  SIPROUND;
  SIPROUND;
  v0 ^= b;
  v2 ^= 0xff;
  SIPROUND;
  SIPROUND;
  SIPROUND;
  SIPROUND;
  b = v0 ^ v1 ^ v2  ^ v3;
  return b;
}

ATTRIBUTE((unused))
#ifdef __i386__
ATTRIBUTE((fastcall))
#endif
PIKE_HOT_ATTRIBUTE
PMOD_EXPORT UINT64 low_hashmem_siphash24_uint32( const unsigned INT32 *in, size_t len, UINT64 key )
{
  /* "somepseudorandomlygeneratedbytes" */
  UINT64 v0 = 0x736f6d6570736575ULL;
  UINT64 v1 = 0x646f72616e646f6dULL;
  UINT64 v2 = 0x6c7967656e657261ULL;
  UINT64 v3 = 0x7465646279746573ULL;
  UINT64 b;
  UINT64 k0 = key;
  UINT64 k1 = key;
  UINT64 m;
  const unsigned int mod = (sizeof( UINT64 )/sizeof(unsigned INT32));
  const int left = len % mod;
  const unsigned INT32 *end = in + len - left;
  b = ( ( UINT64 )len ) << 58;
  v3 ^= k1;
  v2 ^= k0;
  v1 ^= k1;
  v0 ^= k0;

  for ( ; in != end; in += mod )
  {
    m = U32TO64_LE( in );
    v3 ^= m;
    SIPROUND;
    SIPROUND;
    v0 ^= m;
  }

  if (left) {
    b |= ( ( UINT64 )in[ 0] );
  }

  v3 ^= b;
  SIPROUND;
  SIPROUND;
  v0 ^= b;
  v2 ^= 0xff;
  SIPROUND;
  SIPROUND;
  SIPROUND;
  SIPROUND;
  b = v0 ^ v1 ^ v2  ^ v3;
  return b;
}
#endif

size_t hashmem_siphash24( const void *s, size_t len )
{
  return low_hashmem_siphash24( s,len,len,0 );
}
