/*
 * $Id $
 *
 *  md4.c :  Implementation of the MD4 hash function
 *
 * Adapted from md5.c by Marcus Comstedt
 */

#include "crypto_types.h"
#include "md4.h"

void md4_copy(struct md4_ctx *dest, struct md4_ctx *src)
{
  int i;
  dest->count_l=src->count_l;
  dest->count_h=src->count_h;
  for(i=0; i<MD4_DIGESTLEN; i++)
    dest->digest[i]=src->digest[i];
  for(i=0; i < src->index; i++)
    dest->block[i] = src->block[i];
  dest->index = src->index;
}

void md4_init(struct md4_ctx *ctx)
{
  ctx->digest[0] = 0x67452301;
  ctx->digest[1] = 0xefcdab89;
  ctx->digest[2] = 0x98badcfe;
  ctx->digest[3] = 0x10325476;
  
  ctx->count_l = ctx->count_h = 0;
  ctx->index = 0;
}

/* MD4 functions */
#define F(x, y, z) (((y) & (x)) | ((z) & ~(x)))
#define G(x, y, z) (((y) & (x)) | ((z) & (x)) | ((y) & (z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))

#define ROUND(f, w, x, y, z, data, s) \
( w += f(x, y, z) + data,  w = w<<s | w>>(32-s) )
  
/* Perform the MD4 transformation on one full block of 16 32-bit words. */
   
static void md4_transform(struct md4_ctx *ctx, unsigned INT32 *data)
{
  unsigned INT32 a, b, c, d;
  a = ctx->digest[0];
  b = ctx->digest[1];
  c = ctx->digest[2];
  d = ctx->digest[3];

  ROUND(F, a, b, c, d, data[ 0], 3);
  ROUND(F, d, a, b, c, data[ 1], 7);
  ROUND(F, c, d, a, b, data[ 2], 11);
  ROUND(F, b, c, d, a, data[ 3], 19);
  ROUND(F, a, b, c, d, data[ 4], 3);
  ROUND(F, d, a, b, c, data[ 5], 7);
  ROUND(F, c, d, a, b, data[ 6], 11);
  ROUND(F, b, c, d, a, data[ 7], 19);
  ROUND(F, a, b, c, d, data[ 8], 3);
  ROUND(F, d, a, b, c, data[ 9], 7);
  ROUND(F, c, d, a, b, data[10], 11);
  ROUND(F, b, c, d, a, data[11], 19);
  ROUND(F, a, b, c, d, data[12], 3);
  ROUND(F, d, a, b, c, data[13], 7);
  ROUND(F, c, d, a, b, data[14], 11);
  ROUND(F, b, c, d, a, data[15], 19);

  ROUND(G, a, b, c, d, data[ 0] + 0x5a827999, 3);
  ROUND(G, d, a, b, c, data[ 4] + 0x5a827999, 5);
  ROUND(G, c, d, a, b, data[ 8] + 0x5a827999, 9);
  ROUND(G, b, c, d, a, data[12] + 0x5a827999, 13);
  ROUND(G, a, b, c, d, data[ 1] + 0x5a827999, 3);
  ROUND(G, d, a, b, c, data[ 5] + 0x5a827999, 5);
  ROUND(G, c, d, a, b, data[ 9] + 0x5a827999, 9);
  ROUND(G, b, c, d, a, data[13] + 0x5a827999, 13);
  ROUND(G, a, b, c, d, data[ 2] + 0x5a827999, 3);
  ROUND(G, d, a, b, c, data[ 6] + 0x5a827999, 5);
  ROUND(G, c, d, a, b, data[10] + 0x5a827999, 9);
  ROUND(G, b, c, d, a, data[14] + 0x5a827999, 13);
  ROUND(G, a, b, c, d, data[ 3] + 0x5a827999, 3);
  ROUND(G, d, a, b, c, data[ 7] + 0x5a827999, 5);
  ROUND(G, c, d, a, b, data[11] + 0x5a827999, 9);
  ROUND(G, b, c, d, a, data[15] + 0x5a827999, 13);

  ROUND(H, a, b, c, d, data[ 0] + 0x6ed9eba1, 3);
  ROUND(H, d, a, b, c, data[ 8] + 0x6ed9eba1, 9);
  ROUND(H, c, d, a, b, data[ 4] + 0x6ed9eba1, 11);
  ROUND(H, b, c, d, a, data[12] + 0x6ed9eba1, 15);
  ROUND(H, a, b, c, d, data[ 2] + 0x6ed9eba1, 3);
  ROUND(H, d, a, b, c, data[10] + 0x6ed9eba1, 9);
  ROUND(H, c, d, a, b, data[ 6] + 0x6ed9eba1, 11);
  ROUND(H, b, c, d, a, data[14] + 0x6ed9eba1, 15);
  ROUND(H, a, b, c, d, data[ 1] + 0x6ed9eba1, 3);
  ROUND(H, d, a, b, c, data[ 9] + 0x6ed9eba1, 9);
  ROUND(H, c, d, a, b, data[ 5] + 0x6ed9eba1, 11);
  ROUND(H, b, c, d, a, data[13] + 0x6ed9eba1, 15);
  ROUND(H, a, b, c, d, data[ 3] + 0x6ed9eba1, 3);
  ROUND(H, d, a, b, c, data[11] + 0x6ed9eba1, 9);
  ROUND(H, c, d, a, b, data[ 7] + 0x6ed9eba1, 11);
  ROUND(H, b, c, d, a, data[15] + 0x6ed9eba1, 15);

  ctx->digest[0] += a;
  ctx->digest[1] += b;
  ctx->digest[2] += c;
  ctx->digest[3] += d;
}

#ifndef EXTRACT_UCHAR
#define EXTRACT_UCHAR(p)  (*(unsigned char *)(p))
#endif

/* Note that MD4 uses little endian byteorder */
#define STRING2INT(s) ((((((EXTRACT_UCHAR(s+3) << 8)    \
			   | EXTRACT_UCHAR(s+2)) << 8)  \
			 | EXTRACT_UCHAR(s+1)) << 8)    \
		       | EXTRACT_UCHAR(s))
  
static void md4_block(struct md4_ctx *ctx, unsigned INT8 *block)
{
  unsigned INT32 data[MD4_DATALEN];
  int i;
  
  /* Update block count */
  if (!++ctx->count_l)
    ++ctx->count_h;

  /* Endian independent conversion */
  for (i = 0; i<16; i++, block += 4)
    data[i] = STRING2INT(block);

  md4_transform(ctx, data);
}

void md4_update(struct md4_ctx *ctx,
		      unsigned INT8 *buffer,
		      unsigned INT32 len)
{
  if (ctx->index)
  { /* Try to fill partial block */
    unsigned left = MD4_DATASIZE - ctx->index;
    if (len < left)
    {
      memcpy(ctx->block + ctx->index, buffer, len);
      ctx->index += len;
      return; /* Finished */
    }
    else
    {
      memcpy(ctx->block + ctx->index, buffer, left);
      md4_block(ctx, ctx->block);
      buffer += left;
      len -= left;
    }
  }
  while (len >= MD4_DATASIZE)
  {
    md4_block(ctx, buffer);
    buffer += MD4_DATASIZE;
    len -= MD4_DATASIZE;
  }
  if ((ctx->index = len))     /* This assignment is intended */
    /* Buffer leftovers */
    memcpy(ctx->block, buffer, len);
}

/* Final wrapup - pad to MD4_DATASIZE-byte boundary with the bit pattern
   1 0* (64-bit count of bits processed, LSB-first) */

void md4_final(struct md4_ctx *ctx)
{
  unsigned INT32 data[MD4_DATALEN];
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
  
  if (words > (MD4_DATALEN-2))
    { /* No room for length in this block. Process it and
       * pad with another one */
      for (i = words ; i < MD4_DATALEN; i++)
	data[i] = 0;
      md4_transform(ctx, data);
      for (i = 0; i < (MD4_DATALEN-2); i++)
	data[i] = 0;
    }
  else
    for (i = words ; i < MD4_DATALEN - 2; i++)
      data[i] = 0;
  /* Theres 512 = 2^9 bits in one block 
   * Little-endian order => Least significant word first */
  data[MD4_DATALEN-1] = (ctx->count_h << 9) | (ctx->count_l >> 23);
  data[MD4_DATALEN-2] = (ctx->count_l << 9) | (ctx->index << 3);
  md4_transform(ctx, data);
}

void md4_digest(struct md4_ctx *ctx, INT8 *s)
{
  int i;

  /* Little endian order */
  for (i = 0; i < MD4_DIGESTLEN; i++)
    {
      *s++ = 0xff &  ctx->digest[i];
      *s++ = 0xff & (ctx->digest[i] >> 8);
      *s++ = 0xff & (ctx->digest[i] >> 16);
      *s++ =         ctx->digest[i] >> 24;
    }
}
