/* rc4.h */

#ifndef RC4_H_INCLUDED
#define RC4_H_INCLUDED

#include "crypto_types.h"

struct rc4_ctx {
  unsigned INT8 S[256];
  unsigned INT8 i, j;
};

#if 0
void rc4_init(struct rc4_ctx *ctx);
#endif

void rc4_set_key(struct rc4_ctx *ctx, const unsigned INT8 *key, INT32 len);
void rc4_crypt(struct rc4_ctx *ctx, unsigned INT8 *dest, const unsigned INT8 *src, INT32 len);

#endif /* RC4_H_INCLUDED */
