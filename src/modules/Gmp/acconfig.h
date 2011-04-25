/* -*- c -*-
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#ifndef GMP_MACHINE_H
#define GMP_MACHINE_H

@TOP@
@BOTTOM@

/* Define this to the prefix used by __MPN() (usually __mpn_ or __gmpn_). */
#undef PIKE_MPN_PREFIX

/* Define this if you have <gmp2/gmp.h> */
#undef HAVE_GMP2_GMP_H

/* Define this if you have -lgmp2 */
#undef HAVE_LIBGMP2

/* Define this if you have <gmp.h> */
#undef HAVE_GMP_H

/* Define this if you have -lgmp */
#undef HAVE_LIBGMP

/* Define this if you have mpz_popcount */
#undef HAVE_MPZ_POPCOUNT

/* Define this if you have mpz_xor */
#undef HAVE_MPZ_XOR

/* Define this if you have mpz_getlimbn */
#undef HAVE_MPZ_GETLIMBN

/* Define this if you have mpz_import */
#undef HAVE_MPZ_IMPORT

/* Define this if you have mpz_fits_ulong_p */
#undef HAVE_MPZ_FITS_ULONG_P

/* Define if your cpp supports the ANSI concatenation operator ## */
#undef HAVE_ANSI_CONCAT

/* Define if your cpp supports K&R-style concatenation */
#undef HAVE_KR_CONCAT

/* Define to the size of mp_limb_t */
#undef SIZEOF_MP_LIMB_T

/* Define if mpz_getlimbn works on negative numbers. */
#undef MPZ_GETLIMBN_WORKS

/* Define if mpz_set_si works for LONG_MIN. */
#undef MPZ_SET_SI_WORKS

#endif
