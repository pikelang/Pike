/*
 * $Id: arcfour.h,v 1.4 2000/03/28 12:21:24 grubba Exp $
 */

#ifndef ARCFOUR_H_INCLUDED
#define ARCFOUR_H_INCLUDED

#include "crypto_types.h"

struct arcfour_ctx {
  unsigned INT8 S[256];
  unsigned INT8 i, j;
};

#if 0
void arcfour_init(struct arcfour_ctx *ctx);
#endif

void arcfour_set_key(struct arcfour_ctx *ctx, const unsigned INT8 *key, INT32 len);
void arcfour_crypt(struct arcfour_ctx *ctx, unsigned INT8 *dest, const unsigned INT8 *src, INT32 len);

#endif /* ARCFOUR_H_INCLUDED */
