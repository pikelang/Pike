/*
 * The following is the reference implementation if siphash as of
 * november 2013. All tests have been removed from the file and the
 * signature has been changed to match the one used by pike. It also
 * uses the pike int types instead of stdint.h.
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

#define ROTL(x,b) (unsigned INT64)( ((x) << (b)) | ( (x) >> (64 - (b))) )

#define U32TO8_LE(p, v)         \
    (p)[0] = (unsigned char)((v)      ); (p)[1] = (unsigned char)((v) >>  8); \
    (p)[2] = (unsigned char)((v) >> 16); (p)[3] = (unsigned char)((v) >> 24);

#define U64TO8_LE(p, v)         \
  U32TO8_LE((p),     (unsigned INT32)((v)      ));   \
  U32TO8_LE((p) + 4, (unsigned INT32)((v) >> 32));

#define U8TO64_LE(p) \
  (((unsigned INT64)((p)[0])      ) | \
   ((unsigned INT64)((p)[1]) <<  8) | \
   ((unsigned INT64)((p)[2]) << 16) | \
   ((unsigned INT64)((p)[3]) << 24) | \
   ((unsigned INT64)((p)[4]) << 32) | \
   ((unsigned INT64)((p)[5]) << 40) | \
   ((unsigned INT64)((p)[6]) << 48) | \
   ((unsigned INT64)((p)[7]) << 56))

#define SIPROUND            \
  do {              \
    v0 += v1; v1=ROTL(v1,13); v1 ^= v0; v0=ROTL(v0,32); \
    v2 += v3; v3=ROTL(v3,16); v3 ^= v2;     \
    v0 += v3; v3=ROTL(v3,21); v3 ^= v0;     \
    v2 += v1; v1=ROTL(v1,17); v1 ^= v2; v2=ROTL(v2,32); \
  } while(0)

/* SipHash-2-4 */
#ifdef __i386__
__attribute__((fastcall))
#endif
__attribute__((hot))
static size_t low_hashmem_siphash24( const void *s, size_t len, size_t nbytes, size_t key )
{
  const unsigned char * in = (const unsigned char*)s;
  unsigned long long inlen = MINIMUM(len, nbytes);
  
  /* "somepseudorandomlygeneratedbytes" */
  unsigned INT64 v0 = 0x736f6d6570736575ULL;
  unsigned INT64 v1 = 0x646f72616e646f6dULL;
  unsigned INT64 v2 = 0x6c7967656e657261ULL;
  unsigned INT64 v3 = 0x7465646279746573ULL;
  unsigned INT64 b;
  unsigned INT64 k0 = (unsigned INT64)key;
  unsigned INT64 k1 = (unsigned INT64)key;
  unsigned INT64 m;
  const unsigned char *end = in + inlen - ( inlen % sizeof( unsigned INT64 ) );
  const int left = inlen & 7;
  b = ( ( unsigned INT64 )inlen ) << 56;
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
  case 7: b |= ( ( unsigned INT64 )in[ 6] )  << 48;

  case 6: b |= ( ( unsigned INT64 )in[ 5] )  << 40;

  case 5: b |= ( ( unsigned INT64 )in[ 4] )  << 32;

  case 4: b |= ( ( unsigned INT64 )in[ 3] )  << 24;

  case 3: b |= ( ( unsigned INT64 )in[ 2] )  << 16;

  case 2: b |= ( ( unsigned INT64 )in[ 1] )  <<  8;

  case 1: b |= ( ( unsigned INT64 )in[ 0] ); break;

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
  return (size_t)b;
}
