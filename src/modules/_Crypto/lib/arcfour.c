/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/* arcfour.c
 *
 */

#include "crypto_types.h"
#include <arcfour.h>

#ifdef RCSID
RCSID("$Id$");
#endif

#define SWAP(a,b) do { int _t = a; a = b; b = _t; } while(0)

void arcfour_set_key(struct arcfour_ctx *ctx, const unsigned INT8 *key, INT32 len)
{
  register unsigned INT8 j; /* Depends on the eight-bitness of these variables. */
  unsigned i;
  INT32 k;

  /* Initialize context */
  i = 0;
  do ctx->S[i] = i; while (++i < 256);

  /* Expand key */
  i = j = k = 0;
  do {
    j += ctx->S[i] + key[k];
    SWAP(ctx->S[i], ctx->S[j]);
    k = (k+1) % len; /* Repeat key if needed */
  } while(++i < 256);
  
  ctx->i = ctx->j = 0;
}

void arcfour_crypt(struct arcfour_ctx *ctx, unsigned INT8 *dest, const unsigned INT8 *src, INT32 len)
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
