/* rc4.c
 *
 */

#include "crypto_types.h"
#include <rc4.h>

#define SWAP(a,b) (a ^= b, b ^= a, a ^= b)

#if 0
void rc4_init(struct rc4_ctx *ctx)
{
  int i;
  for (i = 0; i < 256; i++)
    ctx->S[i] = i;
  ctx->i = ctx->j = 0;
}
#endif

void rc4_set_key(struct rc4_ctx *ctx, const unsigned INT8 *key, INT32 len)
{
  register unsigned i, j;
  INT32 k;

  /* Initialize context */
  for (i = 0; i < 256; i++)
    ctx->S[i] = i;

  /* Expand key */
  for (i = j = k = 0; i < 256; i++)
    {
      j += ctx->S[i] + key[k];
      SWAP(ctx->S[i], ctx->S[j]);
      k = (k+1) % len; /* Repeat key if needed */
    }
  ctx->j = j; ctx->i = 0;
}

void rc4_crypt(struct rc4_ctx *ctx, unsigned INT8 *dest, const unsigned INT8 *src, INT32 len)
{
  register unsigned INT8 i,j; /* Depends on the 8-bitness of these variables */

  i = ctx->i; j = ctx->j;
  while(len--)
    {
      i = (i + 1);
      j = (j + ctx->S[i]);
      SWAP(ctx->S[i], ctx->S[j]);
      *dest++ = *src++ ^ ctx->S[ (ctx->S[i] + ctx->S[j]) ];
    }
  ctx->i = i; ctx->j = j;
}
