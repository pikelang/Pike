/* rc4.c
 *
 */

#include "crypto_types.h"
#include <rc4.h>

#ifdef RCSID
RCSID("$Id: rc4.c,v 1.5 1997/03/15 04:52:44 nisse Exp $");
#endif

#define SWAP(a,b) do { int _t = a; a = b; b = _t; } while(0)

void rc4_set_key(struct rc4_ctx *ctx, const unsigned INT8 *key, INT32 len)
{
  register unsigned INT8 i, j; /* Depends on the eight-bitness of these variables. */
  INT32 k;

  /* Initialize context */
  i = 0;
  do ctx->S[i] = i; while (++i);

  /* Expand key */
  i = j = k = 0;
  do {
    j += ctx->S[i] + key[k];
    SWAP(ctx->S[i], ctx->S[j]);
    k = (k+1) % len; /* Repeat key if needed */
  } while(++i);
  
  ctx->i = ctx->j = 0;
}

void rc4_crypt(struct rc4_ctx *ctx, unsigned INT8 *dest, const unsigned INT8 *src, INT32 len)
{
  register unsigned INT8 i, j; /* Depends on the eight-bitness of these variables */

  i = ctx->i; j = ctx->j;
  while(len--)
    {
      i++;
      j += ctx->S[i];
      SWAP(ctx->S[i], ctx->S[j]);
      *dest++ = *src++ ^ ctx->S[ (ctx->S[i] + ctx->S[j]) & 0xff ];
    }
  ctx->i = i; ctx->j = j;
}
