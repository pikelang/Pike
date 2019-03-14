/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

/*
 * Prime number test using trial division for small primes and then
 * Miller-Rabin, as suggested in Schneier's Applied Cryptography.
 *
 * These functions or something similar will hopefully be included
 * with Gmp-2.1 .
 */

#include "global.h"
#include "gmp_machine.h"

#if defined(HAVE_GMP2_GMP_H) && defined(HAVE_LIBGMP2)
#define USE_GMP2
#else /* !HAVE_GMP2_GMP_H || !HAVE_LIBGMP2 */
#if defined(HAVE_GMP_H) && defined(HAVE_LIBGMP)
#define USE_GMP
#endif /* HAVE_GMP_H && HAVE_LIBGMP */
#endif /* HAVE_GMP2_GMP_H && HAVE_LIBGMP2 */

#if defined(USE_GMP) || defined(USE_GMP2)

#include <limits.h>
#include "my_gmp.h"

/* Define NUMBER_OF_PRIMES and primes[] */
#include "prime_table.out"

#define SQR(x) ((x)*(x))

/* Returns a small factor of n, or 0 if none is found.*/
unsigned long
mpz_small_factor(mpz_t n, int limit)
{
  int i;
  unsigned long stop;
  
  if (limit > NUMBER_OF_PRIMES)
    limit = NUMBER_OF_PRIMES;

  stop = mpz_get_ui(n);
  if (mpz_cmp_ui(n, stop) != 0)
    stop = ULONG_MAX;

  for (i = 0;
       (i < limit)
	 && (SQR(primes[i]) <= stop); /* I think it's not worth the
				       * effort to get rid of this
				       * extra multiplication. */
       i++)
    if (mpz_fdiv_ui(n, primes[i]) == 0)
      return primes[i];
  return 0;
}

#endif
