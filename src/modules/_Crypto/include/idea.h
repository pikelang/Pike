/* idea.h */

#ifndef IDEA_H_INCLUDED
#define IDEA_H_INCLUDED

#define IDEA_KEYSIZE 16
#define IDEA_BLOCKSIZE 8

#define IDEA_ROUNDS 8
#define IDEA_KEYLEN (6*IDEA_ROUNDS+4)

void idea_expand(unsigned INT16 *ctx,
		 const unsigned INT8 *key);

void idea_invert(unsigned INT16 *d,
		  const unsigned INT16 *e);

void idea_crypt(unsigned INT8 *dest,
		const unsigned INT16 *key,
		const unsigned INT8 *src);

#endif /* IDEA_H_INCLUDED */
