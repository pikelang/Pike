/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: next_prime.c,v 1.13 2004/10/07 22:49:56 nilsson Exp $
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

#if 0
/* Raw millerrabin test. Works only for odd integers n >= 5 */
static int
millerrabin1(mpz_srcptr n, int count)
{
  mpz_t m;
  unsigned long b;
  mpz_t n1;
  mpz_t a;
  mpz_t z;
  unsigned long j;
  int result;

  /* Preparations */
  mpz_init(m);
  mpz_init(n1);
  mpz_init(a);
  mpz_init(z);

  mpz_sub_ui(n1, n, 1);
  b = mpz_scan1(n1, 0);
  mpz_fdiv_q_2exp(m, n1, b);

  result = 0;
  while(count-- > 0)
    {
      do
	{
	  mpz_random(a, mpz_size(n1));
	  mpz_fdiv_r(a, a, n1);
	}
      while (mpz_cmp_ui(a, 1) <= 0);

      result = 0;
      
      /* z = a^{m*2^j} */
      j = 0;
      mpz_powm(z, a, m, n);
      
      while (1)
	{
	  if (mpz_cmp_ui(z, 1) == 0)
	    {
	      result = (j == 0);
	      break;
	    }
	  if (mpz_cmp(z, n1) == 0)
	    {
	      result = 1;
	      break;
	    }
	  j++;
	  if (j == b)
	    break;
	  mpz_powm_ui(z, z, 2, n);
	}
      if (!result)    /* Not prime */
	break;
    }
  mpz_clear(m);
  mpz_clear(n1);
  mpz_clear(a);
  mpz_clear(z);
  return result;
}
#endif

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

#if 0
int
mpz_millerrabin(mpz_srcptr n, int count, int prime_limit)
{
  if (mpz_cmp_ui(n, 5) < 0)
    /* Tiny numbers */
    return ( (mpz_cmp_ui(n, 2) == 0)
	     || (mpz_cmp_ui(n, 3) == 0));
  /* Filter out even numbers */
  if (mpz_fdiv_ui(n, 2) == 0)
    return 0;
  /* Try Miller-Rabin */
  return millerrabin1(n, count);
}
#endif

void
mpz_next_prime(mpz_t p, mpz_t n, int count, int prime_limit)
{
  mpz_t tmp;
  unsigned long *moduli = 0;
  unsigned long difference;
  int i;
  int composite;
  
  /* First handle tiny numbers */
  if (mpz_cmp_ui(n, 2) <= 0)
    {
      mpz_set_ui(p, 2);
      return;
    }
  mpz_set(p, n);
  mpz_setbit(p, 0);

  if (mpz_cmp_ui(n, 8) < 0)
    return;

  mpz_init(tmp);

  if (prime_limit > (NUMBER_OF_PRIMES -1))
    prime_limit = NUMBER_OF_PRIMES - 1;
  if (prime_limit && (mpz_cmp_ui(p, primes[prime_limit]) <= 0) )
    /* Don't use table for small numbers */
    prime_limit = 0;
  if (prime_limit)
    {
      /* Compute residues modulo small odd primes */
      moduli = (unsigned long*) alloca(prime_limit * sizeof(*moduli));
      for (i = 0; i < prime_limit; i++)
	moduli[i] = mpz_fdiv_ui(p, primes[i + 1]);
    }
 for (difference = 0; ; difference += 2)
    {
      if (difference >= ULONG_MAX - 10)
	{ /* Should not happen, at least not very often... */
	  mpz_add_ui(p, p, difference);
	  difference = 0;
	}
      composite = 0;

      /* First check residues */
      if (prime_limit)
	for (i = 0; i < prime_limit; i++)
	  {
	    if (moduli[i] == 0)
	      composite = 1;
	    moduli[i] = (moduli[i] + 2) % primes[i + 1];
	  }
      if (composite)
	continue;
      
      mpz_add_ui(p, p, difference);
      difference = 0;

      /* Fermat test, with respect to 2 */
      mpz_set_ui(tmp, 2);
      mpz_powm(tmp, tmp, p, p);
      if (mpz_cmp_ui(tmp, 2) != 0)
	continue;

      /* Miller-Rabin test */
      if (mpz_probab_prime_p(p, count))
	break;
    }
  mpz_clear(tmp);
}

#endif
