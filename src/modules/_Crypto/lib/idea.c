/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/* The basic IDEA transformation
 *
 * This implementation is taken from pgp, see note below.
 *
 * Only primitive operations are done here, chaining modes etc
 * are implemented in a higher level program.
 *
 **********************************************************************
 *
 *    idea.c - C source code for IDEA block cipher.
 *      IDEA (International Data Encryption Algorithm), formerly known as 
 *      IPES (Improved Proposed Encryption Standard).
 *      Algorithm developed by Xuejia Lai and James L. Massey, of ETH Zurich.
 *      This implementation modified and derived from original C code 
 *      developed by Xuejia Lai.  
 *      Zero-based indexing added, names changed from IPES to IDEA.
 *      CFB functions added.  Random number routines added.
 *
 *      Extensively optimized and restructured by Colin Plumb.
 *
 ***********************************************************************
 *
 * Some changes including endianness cleanup done by Niels Möller.
 *
 */

#include "crypto_types.h"
#include <idea.h>

#include <string.h>

/*-------------------------------------------------------------*/

#define low16(x)  ((x) & 0xffff)

/*
 *	Multiplication, modulo (2**16)+1
 * Note that this code is structured on the assumption that
 * untaken branches are cheaper than taken branches, and the
 * compiler doesn't schedule branches.
 */
#ifdef SMALL_CACHE
static unsigned INT16
mul(unsigned INT16 a, unsigned INT16 b)
{
  register unsigned INT32 p;

  p = (unsigned INT32)a * b;
  if (p)
    {
      b = low16(p);
      a = p>>16;
      return (b - a) + (b < a);
    }
  else if (a)
    {
      return 1-b;
    }
  else
    {
      return 1-a;
    }
} /* mul */
#endif /* SMALL_CACHE */

/*
 * Compute the multiplicative inverse of x, modulo 65537, using Euclid's
 * algorithm. It is unrolled twice to avoid swapping the registers each
 * iteration, and some subtracts of t have been changed to adds.
 */
static unsigned INT16
inv(unsigned INT16 x)     
{
  unsigned INT16 t0, t1;
  unsigned INT16 q, y;

  if (x <= 1)
    return x;	/* 0 and 1 are self-inverse */
  t1 = 0x10001L / x;	/* Since x >= 2, this fits into 16 bits */
  y = 0x10001L % x;
  if (y == 1)
    return low16(1-t1);
  t0 = 1;
  do
    {
      q = x / y;
      x = x % y;
      t0 += q * t1;
      if (x == 1)
	return t0;
      q = y / x;
      y = y % x;
      t1 += q * t0;
    }
  while (y != 1);
  return low16(1-t1);
} /* inv */

/*
 * Expand a 128-bit user key to a working encryption key ctx
 */
void
idea_expand(unsigned INT16 *ctx,
	    const unsigned INT8 *userkey)
{
  int i,j;
  
  for (j=0; j<8; j++) {
    ctx[j] = (userkey[0]<<8) + userkey[1];
    userkey += 2;
  }
  for (i=0; j < IDEA_KEYLEN; j++) {
    i++;
    ctx[i+7] = ctx[i & 7] << 9 | ctx[(i+1) & 7] >> 7;
    ctx += i & 8;
    i &= 7;
  }
} /* idea_expand */

/*
 * Compute IDEA decryption key DK from an expanded IDEA encryption key EK
 * Note that the input and output may be the same.  Thus, the key is
 * inverted into an internal buffer, and then copied to the output.
 */
void
idea_invert(unsigned INT16 *d,
	    const unsigned INT16 *e)
{
  int i;
  unsigned INT16 t1, t2, t3;
  unsigned INT16 temp[IDEA_KEYLEN];
  unsigned INT16 *p = temp + IDEA_KEYLEN;

  t1 = inv(*e++);
  t2 = -*e++;
  t3 = -*e++;
  *--p = inv(*e++);
  *--p = t3;
  *--p = t2;
  *--p = t1;

  for (i = 0; i < IDEA_ROUNDS-1; i++) {
    t1 = *e++;
    *--p = *e++;
    *--p = t1;

    t1 = inv(*e++);
    t2 = -*e++;
    t3 = -*e++;
    *--p = inv(*e++);
    *--p = t2;
    *--p = t3;
    *--p = t1;
  }
  t1 = *e++;
  *--p = *e++;
  *--p = t1;

  t1 = inv(*e++);
  t2 = -*e++;
  t3 = -*e++;
  *--p = inv(*e++);
  *--p = t3;
  *--p = t2;
  *--p = t1;
  /* Copy and destroy temp copy */
  memcpy(d, temp, sizeof(temp));
  memset(temp, 0, sizeof(temp));
} /* idea_invert */

/*
 * MUL(x,y) computes x = x*y, modulo 0x10001.  Requires two temps, 
 * t16 and t32.  x is modified, and must me a side-effect-free lvalue.
 * y may be anything, but unlike x, must be strictly 16 bits even if
 * low16() is #defined.
 * All of these are equivalent - see which is faster on your machine
 */
#ifdef SMALL_CACHE
#define MUL(x,y) (x = mul(low16(x),y))
#else /* !SMALL_CACHE */
#ifdef AVOID_JUMPS
#define MUL(x,y) (x = low16(x-1), t16 = low16((y)-1), \
		t32 = (unsigned INT32)x*t16 + x + t16 + 1, x = low16(t32), \
		t16 = t32>>16, x = (x-t16) + (x<t16) )
#else /* !AVOID_JUMPS (default) */
#define MUL(x,y) \
	((t16 = (y)) ? \
		(x=low16(x)) ? \
			t32 = (unsigned INT32)x*t16, \
			x = low16(t32), \
			t16 = t32>>16, \
			x = (x-t16)+(x<t16) \
		: \
			(x = 1-t16) \
	: \
		(x = 1-x))
#endif
#endif

/* Endian independent conversions */
#define char2word(dest, p) \
     do { \
	    (dest) = *(p)++ << 8; (dest) |= *(p)++; \
	} while(0)
     
#define word2char(src, p) \
     do { \
	    *(p)++ = (src) >> 8; *(p)++ = (src) & 0xff; \
	} while(0)
     
/*	IDEA encryption/decryption algorithm */
/* Note that in and out can be the same buffer */
void
idea_crypt(const unsigned INT16 *key,
	   unsigned INT8 *dest,
	   const unsigned INT8 *src)
{
  register unsigned INT16 x1, x2, x3, x4, s2, s3;
  
  /* Setup */
    
  char2word(x1, src); char2word(x2, src);
  char2word(x3, src); char2word(x4, src);
  
  /* Encrypt */
  {
#ifndef SMALL_CACHE
    register unsigned INT16 t16;	/* Temporaries needed by MUL macro */
    register unsigned INT32 t32;
#endif
    int r = IDEA_ROUNDS;
    do
      {
	MUL(x1,*key++);
	x2 += *key++;
	x3 += *key++;
	MUL(x4, *key++);

	s3 = x3;
	x3 ^= x1;
	MUL(x3, *key++);
	s2 = x2;
	x2 ^= x4;
	x2 += x3;
	MUL(x2, *key++);
	x3 += x2;

	x1 ^= x2;  x4 ^= x3;

	x2 ^= s3;  x3 ^= s2;
      }
    while (--r);
    MUL(x1, *key++);
    x3 += *key++;
    x2 += *key++;
    MUL(x4, *key);
  }
  word2char(x1, dest); word2char(x3, dest);
  word2char(x2, dest); word2char(x4, dest);
} /* idea_crypt */

/*-------------------------------------------------------------*/
