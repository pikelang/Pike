/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#define IDEA_KEY_SIZE 16
#define IDEA_BLOCK_SIZE 8

#define IDEA_ROUNDS 8
#define IDEA_KEYLEN (6*IDEA_ROUNDS+4)

struct idea_ctx
{
  unsigned INT16 *ctx[IDEA_KEYLEN];
};

void idea_crypt_blocks(struct idea_ctx *ctx, int len,
		       unsigned char *to, unsigned char *from);

void idea_expand(unsigned INT16 *ctx,
		 const unsigned INT8 *key);

void idea_invert(unsigned INT16 *d,
		 const unsigned INT16 *e);
