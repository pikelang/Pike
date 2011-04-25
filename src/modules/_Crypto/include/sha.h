/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#include "crypto_types.h"

/* The SHA block size and message digest sizes, in bytes */

#define SHA_DATASIZE    64
#define SHA_DATALEN     16
#define SHA_DIGESTSIZE  20
#define SHA_DIGESTLEN    5
/* The structure for storing SHA info */

struct sha_ctx {
  unsigned INT32 digest[SHA_DIGESTLEN];  /* Message digest */
  unsigned INT32 count_l, count_h;       /* 64-bit block count */
  unsigned INT8 block[SHA_DATASIZE];     /* SHA data buffer */
  int index;                             /* index into buffer */
};

void sha_init(struct sha_ctx *ctx);
void sha_update(struct sha_ctx *ctx, unsigned INT8 *buffer, unsigned INT32 len);
void sha_final(struct sha_ctx *ctx);
void sha_digest(struct sha_ctx *ctx, INT8 *s);
void sha_copy(struct sha_ctx *dest, struct sha_ctx *src);
