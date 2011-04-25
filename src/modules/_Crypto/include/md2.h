/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#include "crypto_types.h"

struct md2_ctx {
  unsigned INT8 C[16], X[48];
  unsigned INT8 buf[16];
  INT32 count;
};

void md2_init(struct md2_ctx *ctx);
void md2_update(struct md2_ctx *ctx, unsigned INT8 *buffer, unsigned INT32 len);
void md2_final(struct md2_ctx *ctx);
void md2_digest(struct md2_ctx *ctx, INT8 *s);
void md2_copy(struct md2_ctx *dest, struct md2_ctx *src);
