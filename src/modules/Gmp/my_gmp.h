/* $Id: my_gmp.h,v 1.8 2000/09/05 13:03:03 grubba Exp $
 *
 * These functions or something similar will hopefully be included
 * with Gmp-2.1 .
 */

#ifndef MY_GMP_H_INCLUDED
#define MY_GMP_H_INCLUDED


/* Kludge for some compilers only defining __STDC__ in strict mode,
 * which leads to <gmp.h> using the wrong token concat method.
 */
#if !defined(__STDC__) && defined(HAVE_ANSI_CONCAT) && defined(PIKE_MPN_PREFIX)
#define PIKE_LOW_MPN_CONCAT(x,y)	x##y
#define PIKE_MPN_CONCAT(x,y)	PIKE_LOW_MPN_CONCAT(x,y)
#define __MPN(x)	PIKE_MPN_CONCAT(PIKE_MPN_PREFIX,x)
#endif /* !__STDC__ && HAVE_ANSI_CONCAT && PIKE_MPN_PREFIX */

#undef _PROTO
#define _PROTO(x) x

#ifdef USE_GMP2
#include <gmp2/gmp.h>
#else /* !USE_GMP2 */
#include <gmp.h>
#endif /* USE_GMP2 */

unsigned long mpz_small_factor(mpz_t n, int limit);

void mpz_next_prime(mpz_t p, mpz_t n, int count, int prime_limit);
void my_mpz_xor _PROTO ((mpz_ptr, mpz_srcptr, mpz_srcptr));

#endif /* MY_GMP_H_INCLUDED */
