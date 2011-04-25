/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/*
 *  md5.c :  Implementation of the MD5 hash function
 *
 * Part of the Python Cryptography Toolkit, version 1.0.1
 * Colin Plumb's original code modified by A.M. Kuchling
 *
 * Further hacked and adapted to pike by Niels Möller
 */

#include "crypto_types.h"
#include "md5.h"

void md5_copy(struct md5_ctx *dest, struct md5_ctx *src)
{
  int i;
  dest->count_l=src->count_l;
  dest->count_h=src->count_h;
  for(i=0; i<MD5_DIGESTLEN; i++)
    dest->digest[i]=src->digest[i];
  for(i=0; i < src->index; i++)
    dest->block[i] = src->block[i];
  dest->index = src->index;
}

void md5_init(struct md5_ctx *ctx)
{
  ctx->digest[0] = 0x67452301;
  ctx->digest[1] = 0xefcdab89;
  ctx->digest[2] = 0x98badcfe;
  ctx->digest[3] = 0x10325476;
  
  ctx->count_l = ctx->count_h = 0;
  ctx->index = 0;
}

/* MD5 functions */
#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

#define ROUND(f, w, x, y, z, data, s) \
( w += f(x, y, z) + data,  w = w<<s | w>>(32-s),  w += x )
  
/* Perform the MD5 transformation on one full block of 16 32-bit words. */
   
static void md5_transform(struct md5_ctx *ctx, unsigned INT32 *data)
{
  unsigned INT32 a, b, c, d;
  a = ctx->digest[0];
  b = ctx->digest[1];
  c = ctx->digest[2];
  d = ctx->digest[3];

  ROUND(F1, a, b, c, d, data[ 0] + 0xd76aa478, 7);
  ROUND(F1, d, a, b, c, data[ 1] + 0xe8c7b756, 12);
  ROUND(F1, c, d, a, b, data[ 2] + 0x242070db, 17);
  ROUND(F1, b, c, d, a, data[ 3] + 0xc1bdceee, 22);
  ROUND(F1, a, b, c, d, data[ 4] + 0xf57c0faf, 7);
  ROUND(F1, d, a, b, c, data[ 5] + 0x4787c62a, 12);
  ROUND(F1, c, d, a, b, data[ 6] + 0xa8304613, 17);
  ROUND(F1, b, c, d, a, data[ 7] + 0xfd469501, 22);
  ROUND(F1, a, b, c, d, data[ 8] + 0x698098d8, 7);
  ROUND(F1, d, a, b, c, data[ 9] + 0x8b44f7af, 12);
  ROUND(F1, c, d, a, b, data[10] + 0xffff5bb1, 17);
  ROUND(F1, b, c, d, a, data[11] + 0x895cd7be, 22);
  ROUND(F1, a, b, c, d, data[12] + 0x6b901122, 7);
  ROUND(F1, d, a, b, c, data[13] + 0xfd987193, 12);
  ROUND(F1, c, d, a, b, data[14] + 0xa679438e, 17);
  ROUND(F1, b, c, d, a, data[15] + 0x49b40821, 22);

  ROUND(F2, a, b, c, d, data[ 1] + 0xf61e2562, 5);
  ROUND(F2, d, a, b, c, data[ 6] + 0xc040b340, 9);
  ROUND(F2, c, d, a, b, data[11] + 0x265e5a51, 14);
  ROUND(F2, b, c, d, a, data[ 0] + 0xe9b6c7aa, 20);
  ROUND(F2, a, b, c, d, data[ 5] + 0xd62f105d, 5);
  ROUND(F2, d, a, b, c, data[10] + 0x02441453, 9);
  ROUND(F2, c, d, a, b, data[15] + 0xd8a1e681, 14);
  ROUND(F2, b, c, d, a, data[ 4] + 0xe7d3fbc8, 20);
  ROUND(F2, a, b, c, d, data[ 9] + 0x21e1cde6, 5);
  ROUND(F2, d, a, b, c, data[14] + 0xc33707d6, 9);
  ROUND(F2, c, d, a, b, data[ 3] + 0xf4d50d87, 14);
  ROUND(F2, b, c, d, a, data[ 8] + 0x455a14ed, 20);
  ROUND(F2, a, b, c, d, data[13] + 0xa9e3e905, 5);
  ROUND(F2, d, a, b, c, data[ 2] + 0xfcefa3f8, 9);
  ROUND(F2, c, d, a, b, data[ 7] + 0x676f02d9, 14);
  ROUND(F2, b, c, d, a, data[12] + 0x8d2a4c8a, 20);

  ROUND(F3, a, b, c, d, data[ 5] + 0xfffa3942, 4);
  ROUND(F3, d, a, b, c, data[ 8] + 0x8771f681, 11);
  ROUND(F3, c, d, a, b, data[11] + 0x6d9d6122, 16);
  ROUND(F3, b, c, d, a, data[14] + 0xfde5380c, 23);
  ROUND(F3, a, b, c, d, data[ 1] + 0xa4beea44, 4);
  ROUND(F3, d, a, b, c, data[ 4] + 0x4bdecfa9, 11);
  ROUND(F3, c, d, a, b, data[ 7] + 0xf6bb4b60, 16);
  ROUND(F3, b, c, d, a, data[10] + 0xbebfbc70, 23);
  ROUND(F3, a, b, c, d, data[13] + 0x289b7ec6, 4);
  ROUND(F3, d, a, b, c, data[ 0] + 0xeaa127fa, 11);
  ROUND(F3, c, d, a, b, data[ 3] + 0xd4ef3085, 16);
  ROUND(F3, b, c, d, a, data[ 6] + 0x04881d05, 23);
  ROUND(F3, a, b, c, d, data[ 9] + 0xd9d4d039, 4);
  ROUND(F3, d, a, b, c, data[12] + 0xe6db99e5, 11);
  ROUND(F3, c, d, a, b, data[15] + 0x1fa27cf8, 16);
  ROUND(F3, b, c, d, a, data[ 2] + 0xc4ac5665, 23);

  ROUND(F4, a, b, c, d, data[ 0] + 0xf4292244, 6);
  ROUND(F4, d, a, b, c, data[ 7] + 0x432aff97, 10);
  ROUND(F4, c, d, a, b, data[14] + 0xab9423a7, 15);
  ROUND(F4, b, c, d, a, data[ 5] + 0xfc93a039, 21);
  ROUND(F4, a, b, c, d, data[12] + 0x655b59c3, 6);
  ROUND(F4, d, a, b, c, data[ 3] + 0x8f0ccc92, 10);
  ROUND(F4, c, d, a, b, data[10] + 0xffeff47d, 15);
  ROUND(F4, b, c, d, a, data[ 1] + 0x85845dd1, 21);
  ROUND(F4, a, b, c, d, data[ 8] + 0x6fa87e4f, 6);
  ROUND(F4, d, a, b, c, data[15] + 0xfe2ce6e0, 10);
  ROUND(F4, c, d, a, b, data[ 6] + 0xa3014314, 15);
  ROUND(F4, b, c, d, a, data[13] + 0x4e0811a1, 21);
  ROUND(F4, a, b, c, d, data[ 4] + 0xf7537e82, 6);
  ROUND(F4, d, a, b, c, data[11] + 0xbd3af235, 10);
  ROUND(F4, c, d, a, b, data[ 2] + 0x2ad7d2bb, 15);
  ROUND(F4, b, c, d, a, data[ 9] + 0xeb86d391, 21);

  ctx->digest[0] += a;
  ctx->digest[1] += b;
  ctx->digest[2] += c;
  ctx->digest[3] += d;
}

#ifndef EXTRACT_UCHAR
#define EXTRACT_UCHAR(p)  (*(unsigned char *)(p))
#endif

/* Note that MD5 uses little endian byteorder */
#define STRING2INT(s) ((((((EXTRACT_UCHAR(s+3) << 8)    \
			   | EXTRACT_UCHAR(s+2)) << 8)  \
			 | EXTRACT_UCHAR(s+1)) << 8)    \
		       | EXTRACT_UCHAR(s))
  
static void md5_block(struct md5_ctx *ctx, unsigned INT8 *block)
{
  unsigned INT32 data[MD5_DATALEN];
  int i;
  
  /* Update block count */
  if (!++ctx->count_l)
    ++ctx->count_h;

  /* Endian independent conversion */
  for (i = 0; i<16; i++, block += 4)
    data[i] = STRING2INT(block);

  md5_transform(ctx, data);
}

void md5_update(struct md5_ctx *ctx,
		      unsigned INT8 *buffer,
		      unsigned INT32 len)
{
  if (ctx->index)
  { /* Try to fill partial block */
    unsigned left = MD5_DATASIZE - ctx->index;
    if (len < left)
    {
      memcpy(ctx->block + ctx->index, buffer, len);
      ctx->index += len;
      return; /* Finished */
    }
    else
    {
      memcpy(ctx->block + ctx->index, buffer, left);
      md5_block(ctx, ctx->block);
      buffer += left;
      len -= left;
    }
  }
  while (len >= MD5_DATASIZE)
  {
    md5_block(ctx, buffer);
    buffer += MD5_DATASIZE;
    len -= MD5_DATASIZE;
  }
  if ((ctx->index = len))     /* This assignment is intended */
    /* Buffer leftovers */
    memcpy(ctx->block, buffer, len);
}

/* Final wrapup - pad to MD5_DATASIZE-byte boundary with the bit pattern
   1 0* (64-bit count of bits processed, LSB-first) */

void md5_final(struct md5_ctx *ctx)
{
  unsigned INT32 data[MD5_DATALEN];
  int i;
  int words;
  
  i = ctx->index;
  /* Set the first char of padding to 0x80.  This is safe since there is
     always at least one byte free */
  ctx->block[i++] = 0x80;

  /* Fill rest of word */
  for( ; i & 3; i++)
    ctx->block[i] = 0;

  /* i is now a multiple of the word size 4 */
  words = i >> 2;
  for (i = 0; i < words; i++)
    data[i] = STRING2INT(ctx->block + 4*i);
  
  if (words > (MD5_DATALEN-2))
    { /* No room for length in this block. Process it and
       * pad with another one */
      for (i = words ; i < MD5_DATALEN; i++)
	data[i] = 0;
      md5_transform(ctx, data);
      for (i = 0; i < (MD5_DATALEN-2); i++)
	data[i] = 0;
    }
  else
    for (i = words ; i < MD5_DATALEN - 2; i++)
      data[i] = 0;
  /* Theres 512 = 2^9 bits in one block 
   * Little-endian order => Least significant word first */
  data[MD5_DATALEN-1] = (ctx->count_h << 9) | (ctx->count_l >> 23);
  data[MD5_DATALEN-2] = (ctx->count_l << 9) | (ctx->index << 3);
  md5_transform(ctx, data);
}

void md5_digest(struct md5_ctx *ctx, INT8 *s)
{
  int i;

  /* Little endian order */
  for (i = 0; i < MD5_DIGESTLEN; i++)
    {
      *s++ = 0xff &  ctx->digest[i];
      *s++ = 0xff & (ctx->digest[i] >> 8);
      *s++ = 0xff & (ctx->digest[i] >> 16);
      *s++ =         ctx->digest[i] >> 24;
    }
}
