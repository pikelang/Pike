/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "stuff.h"
#include "bitvector.h"
#include "pike_cpulib.h"
#include "pike_memory.h"

/* Used by isidchar in pike_macros.h. */
PMOD_EXPORT const char Pike_isidchar_vector[] =
  "0000000000000000"
  "0000000000000000"
  "0000000000000000"
  "1111111111000000"
  "0111111111111111"
  "1111111111100001"
  "0111111111111111"
  "1111111111100000"
  "0000000000000000"
  "0000000000000000"
  "0011110101100010"
  "1011011001101110"
  "1111111111111111"
  "1111111011111111"
  "1111111111111111"
  "1111111011111111";

const unsigned char hexdecode[256] =
{
  16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
  16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,

  /* '0' - '9' */
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,

  16,16,16,16,16,16,16,

  /* 'A' - 'F' */
  10,  11,  12,  13,  14,  15,

  16,16,16,16,16,16,16,16,16,16,16,16,16,
  16,16,16,16,16,16,16,16,16,16,16,16,16,

  /* 'a' - 'f' */
  10,  11,  12,  13,  14,  15,

  16,16,16,16,16,16,16,16,16,
  16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
  16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
  16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
  16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
  16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
  16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
  16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
  16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
  16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
};


/* Same thing as (int)floor(log((double)x) / log(2.0)), except a bit
   quicker. Number of log2 per second:

   log(x)/log(2.0)             50,000,000
   log2(x)                     75,000,000
   Table lookup             3,000,000,000
   Intrinsic       30,000,000,000,000,000
*/

PMOD_EXPORT int my_log2(UINT64 x)
{
    if( x == 0 ) return 0;
    if(x & ~((UINT64)0xffffffffUL)) {
      return 32 + log2_u32((unsigned INT32)(x>>32));
    }
    return log2_u32((unsigned INT32)x);
}


PMOD_EXPORT double my_strtod(char *nptr, char **endptr)
{
  double tmp=strtod(nptr,endptr);
  if(*endptr>nptr)
  {
    if(endptr[0][-1]=='.')
      endptr[0]--;
  }
  return tmp;
}
