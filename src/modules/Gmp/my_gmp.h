/* $Id: my_gmp.h,v 1.4 1999/12/02 01:06:21 hubbe Exp $
 *
 * These functions or something similar will hopefully be included
 * with Gmp-2.1 .
 */

#ifndef MY_GMP_H_INCLUDED
#define MY_GMP_H_INCLUDED

#ifndef __MPN
#define __MPN(x) PIKE_CONCAT(__mpn_,x)
#endif

#undef _PROTO
#define _PROTO(x) x

#ifdef USE_GMP2
#include <gmp2/gmp.h>
#else /* !USE_GMP2 */
#include <gmp.h>
#endif /* USE_GMP2 */


unsigned long mpz_small_factor(mpz_t n, int limit);

void mpz_next_prime(mpz_t p, mpz_t n, int count, int prime_limit);

#endif /* MY_GMP_H_INCLUDED */
