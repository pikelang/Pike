/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "stuff.h"

/* Not all of these are primes, but they should be adequate */
INT32 hashprimes[32] =
{
  31,        /* ~ 2^0  = 1 */
  31,        /* ~ 2^1  = 2 */
  31,        /* ~ 2^2  = 4 */
  31,        /* ~ 2^3  = 8 */
  31,        /* ~ 2^4  = 16 */
  31,        /* ~ 2^5  = 32 */
  61,        /* ~ 2^6  = 64 */
  127,       /* ~ 2^7  = 128 */
  251,       /* ~ 2^8  = 256 */
  541,       /* ~ 2^9  = 512 */
  1151,      /* ~ 2^10 = 1024 */
  2111,      /* ~ 2^11 = 2048 */
  4327,      /* ~ 2^12 = 4096 */
  8803,      /* ~ 2^13 = 8192 */
  17903,     /* ~ 2^14 = 16384 */
  32321,     /* ~ 2^15 = 32768 */
  65599,     /* ~ 2^16 = 65536 */
  133153,    /* ~ 2^17 = 131072 */
  270001,    /* ~ 2^18 = 264144 */
  547453,    /* ~ 2^19 = 524288 */
  1109891,   /* ~ 2^20 = 1048576 */
  2000143,   /* ~ 2^21 = 2097152 */
  4561877,   /* ~ 2^22 = 4194304 */
  9248339,   /* ~ 2^23 = 8388608 */
  16777215,  /* ~ 2^24 = 16777216 */
  33554431,  /* ~ 2^25 = 33554432 */
  67108863,  /* ~ 2^26 = 67108864 */
  134217727, /* ~ 2^27 = 134217728 */
  268435455, /* ~ 2^28 = 268435456 */
  536870911, /* ~ 2^29 = 536870912 */
  1073741823,/* ~ 2^30 = 1073741824 */
  2147483647,/* ~ 2^31 = 2147483648 */
};

/* same thing as (int)floor(log((double)x) / log(2.0)) */
/* Except a bit quicker :) (hopefully) */

int my_log2(unsigned INT32 x)
{
  static char bit[256] =
  {
    -1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 
     4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 
     5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 
     5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 
     6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 
     6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 
     6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 
     6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 
     7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
     7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
     7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
     7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
     7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
     7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
     7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
     7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
  };
  register unsigned int tmp;
  if((tmp=(x>>16)))
  {
    if((x=(tmp>>8))) return bit[x]+24;
    return bit[tmp]+16;
  }
  if((tmp=(x>>8))) return bit[tmp]+8;
  return bit[x];
}


/* Return the number of bits in a 32-bit integer */
int count_bits(unsigned INT32 x)
{
#define B(X) X+0,X+1,X+1,X+2,\
             X+1,X+2,X+2,X+3,\
             X+1,X+2,X+2,X+3,\
             X+2,X+3,X+3,X+4
  static char bits[256] =
  {
    B(0), B(1), B(1), B(2),
    B(1), B(2), B(2), B(3),
    B(1), B(2), B(2), B(3),
    B(2), B(3), B(3), B(4)
  };

  return (bits[x & 255] +
	  bits[(x>>8) & 255] +
	  bits[(x>>16) & 255] +
	  bits[(x>>24) & 255]);
}

/* Return true for integers with more than one bit set */
int is_more_than_one_bit(unsigned INT32 x)
{
  return ((x & 0xaaaaaaaaUL) && (x & 0x55555555UL)) ||
         ((x & 0xccccccccUL) && (x & 0x33333333UL)) ||
         ((x & 0xf0f0f0f0UL) && (x & 0x0f0f0f0fUL)) ||
         ((x & 0xff00ff00UL) && (x & 0x00ff00ffUL)) ||
         ((x & 0xff00ff00UL) && (x & 0x00ff00ffUL)) ||
         ((x & 0xffff0000UL) && (x & 0x0000ffffUL));
}

double my_strtod(char *nptr, char **endptr)
{
  double tmp=STRTOD(nptr,endptr);
  if(*endptr>nptr)
  {
    if(endptr[0][-1]=='.')
      endptr[0]--;
  }
  return tmp;
}
