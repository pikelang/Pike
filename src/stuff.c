/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "stuff.h"

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
