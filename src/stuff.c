/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "stuff.h"
#include "bitvector.h"
#include "pike_cpulib.h"

/* Used by is8bitalnum in pike_macros.h. */
PMOD_EXPORT const char Pike_is8bitalnum_vector[] =
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
  "1011110101100010"
  "1011011001101110"
  "1111111111111111"
  "1111111011111111"
  "1111111111111111"
  "1111111011111111";

/* Not all of these are primes, but they should be adequate */
const INT32 hashprimes[32] =
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

/* Same thing as (int)floor(log((double)x) / log(2.0)), except a bit
   quicker. Number of log2 per second:

   log(x)/log(2.0)             50,000,000
   log2(x)                     75,000,000
   Table lookup             3,000,000,000
   Intrinsic       30,000,000,000,000,000
*/

PMOD_EXPORT int my_log2(size_t x)
{
    if( x == 0 ) return 0;
    return log2_u32(x);
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

/*
 * This rounds an integer up to the next power of two. For x a power
 * of two, this will just return the same again.
 */
unsigned INT32 find_next_power(unsigned INT32 x)
{
    if( x == 0 ) return 1;
    return 1<<(my_log2(x-1)+1);
}

#if HAS___BUILTIN_IA32_RDRAND32_STEP
static int use_rdrnd;
#define bit_RDRND_2 (1<<30)
#endif

/* Bob Jenkins small noncryptographic PRNG */

typedef unsigned long int  u4;
static u4 rnd_a;
static u4 rnd_b;
static u4 rnd_c;
static u4 rnd_d;

#define rot(x,k) (((x)<<(k))|((x)>>(32-(k))))
static u4 ranval(void) {
  u4 e = rnd_a - rot(rnd_b, 27);
  rnd_a = rnd_b ^ rot(rnd_c, 17);
  rnd_b = rnd_c + rnd_d;
  rnd_c = rnd_d + e;
  rnd_d = e + rnd_a;
  return rnd_d;
}

static void raninit( u4 seed ) {
  u4 i;
  rnd_a = 0xf1ea5eed, rnd_b = rnd_c = rnd_d = seed;
  for (i=0; i<20; ++i) {
    (void)ranval();
  }
}

PMOD_EXPORT void my_srand(INT32 seed)
{
#if HAS___BUILTIN_IA32_RDRAND32_STEP
  unsigned int ignore, cpuid_ecx;
  if( !use_rdrnd )
  {
    INT32 cpuid[4];
    x86_get_cpuid (1, cpuid);
    if( cpuid[3] & bit_RDRND_2 )
      use_rdrnd = 1;
  }
  /* We still do the initialization here, since rdrnd might stop
     working if the hardware random unit in the CPU fails (according
     to intel documentation).


     This is likely to be rather rare. But the cost is not exactly
     high.

     Source:

     http://software.intel.com/en-us/articles/intel-digital-random-number-generator-drng-software-implementation-guide
  */
#endif

  raninit(seed);
}

static unsigned INT32 low_rand(void)
{
#if HAS___BUILTIN_IA32_RDRAND32_STEP
  if( use_rdrnd )
  {
    unsigned int rnd;
    unsigned int cnt = 0;
    do{
      if( __builtin_ia32_rdrand32_step( &rnd ) )
        return rnd;
    } while(cnt++ < 100);

    /* hardware random unit most likely not healthy.
       Switch to software random. */
    use_rdrnd = 0;
  }
#endif
  return ranval();
}

static unsigned INT64 low_rand64(void)
{
#if HAS___BUILTIN_IA32_RDRAND32_STEP
  if( use_rdrnd )
  {
    unsigned long long rnd;
    unsigned int cnt = 0;
    do{
      /* We blindly trust that _builtin_ia32_rdrand64_step exists when
         the 32 bit version does. */
      if( __builtin_ia32_rdrand64_step( &rnd ) )
        return rnd;
    } while(cnt++ < 100);

    /* hardware random unit most likely not healthy.
       Switch to software random. */
    use_rdrnd = 0;
  }
#endif
  return ((INT64)ranval()<<32) | ranval();
}

PMOD_EXPORT unsigned INT32 my_rand(unsigned INT32 limit)
{
  if(limit==0xffffffff)
    return low_rand();
  return low_rand()%limit;
}

PMOD_EXPORT unsigned INT64 my_rand64(unsigned INT64 limit)
{
  if(limit==0xffffffffffffffff)
    return low_rand64();
  return low_rand64()%limit;
}
