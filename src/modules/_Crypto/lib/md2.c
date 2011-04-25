/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/*
 *  md2.c : MD2 hash algorithm.
 *
 * Part of the Python Cryptography Toolkit, version 1.0.1
 *
 * Further hacked and adapted to pike by Andreas Sigfridsson
 */

#include "md2.h"

void md2_copy(struct md2_ctx *dest, struct md2_ctx *src)
{
  dest->count=src->count;  
  memcpy(dest->buf, src->buf, src->count); /* dest->count ? */
  memcpy(dest->X, src->X, 48);
  memcpy(dest->C, src->C, 16);
}

void md2_init(struct md2_ctx *ctx)
{
  memset(ctx->X, 0, 48);
  memset(ctx->C, 0, 16);
  ctx->count=0;
}

static unsigned INT8 S[256] = {
  41, 46, 67, 201, 162, 216, 124, 1, 61, 54, 84, 161, 236, 240, 6,
  19, 98, 167, 5, 243, 192, 199, 115, 140, 152, 147, 43, 217, 188,
  76, 130, 202, 30, 155, 87, 60, 253, 212, 224, 22, 103, 66, 111, 24,
  138, 23, 229, 18, 190, 78, 196, 214, 218, 158, 222, 73, 160, 251,
  245, 142, 187, 47, 238, 122, 169, 104, 121, 145, 21, 178, 7, 63,
  148, 194, 16, 137, 11, 34, 95, 33, 128, 127, 93, 154, 90, 144, 50,
  39, 53, 62, 204, 231, 191, 247, 151, 3, 255, 25, 48, 179, 72, 165,
  181, 209, 215, 94, 146, 42, 172, 86, 170, 198, 79, 184, 56, 210,
  150, 164, 125, 182, 118, 252, 107, 226, 156, 116, 4, 241, 69, 157,
  112, 89, 100, 113, 135, 32, 134, 91, 207, 101, 230, 45, 168, 2, 27,
  96, 37, 173, 174, 176, 185, 246, 28, 70, 97, 105, 52, 64, 126, 15,
  85, 71, 163, 35, 221, 81, 175, 58, 195, 92, 249, 206, 186, 197,
  234, 38, 44, 83, 13, 110, 133, 40, 132, 9, 211, 223, 205, 244, 65,
  129, 77, 82, 106, 220, 55, 200, 108, 193, 171, 250, 36, 225, 123,
  8, 12, 189, 177, 74, 120, 136, 149, 139, 227, 99, 232, 109, 233,
  203, 213, 254, 59, 0, 29, 57, 242, 239, 183, 14, 102, 88, 208, 228,
  166, 119, 114, 248, 235, 117, 75, 10, 49, 68, 80, 180, 143, 237,
  31, 26, 219, 153, 141, 51, 159, 17, 131, 20
};

void md2_update(struct md2_ctx *ctx,
		unsigned INT8 *buffer,
		unsigned INT32 len)
{
  unsigned INT32 L;
  while (len) 
  {
    L = (((unsigned INT32)16) < (len + ctx->count)) ?
      ((unsigned INT32)(16-ctx->count)) : len;
    memcpy(ctx->buf+ctx->count, buffer, L);
    ctx->count+=L;
    buffer+=L;
    len-=L;
    if (ctx->count==16) 
    {
      unsigned INT8 t;
      int i,j;

      ctx->count=0;
      memcpy(ctx->X+16, ctx->buf, 16);
      t=ctx->C[15];
      for(i=0; i<16; i++)
      {
	ctx->X[32+i]=ctx->X[16+i]^ctx->X[i];
	t=ctx->C[i]^=S[ctx->buf[i]^t];
      }

      t=0;
      for(i=0; i<18; i++)
      {
	for(j=0; j<48; j++)
	  t=ctx->X[j]^=S[t];
	t=(t+i) & 0xFF;
      }
    }
  }
}

void md2_final(struct md2_ctx *ctx)
{
  unsigned INT32 L;
  unsigned INT8 buf[16];
  L=16-ctx->count;
  memset(buf, (unsigned INT8)L, L);
  md2_update(ctx, buf, L);
}

void md2_digest(struct md2_ctx *ctx, INT8 *s)
{
  unsigned INT8 padding[16];
  unsigned INT32 padlen;
  struct md2_ctx temp;
  unsigned int i;

  md2_copy(&temp, ctx);
  padlen = 16-ctx->count;
  for(i=0; i<padlen; i++)
    padding[i] = padlen;
  md2_update(&temp, padding, padlen);
  md2_update(&temp, temp.C, 16);
  memcpy(s, temp.X, 16);
}
