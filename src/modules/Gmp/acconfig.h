/* -*- c -*-
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: acconfig.h,v 1.1 2003/01/09 15:55:08 marcus Exp $
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

/* define this if INT_TYPE is bigger then signed long int */
#undef BIG_PIKE_INT

/* Define if your cpp supports the ANSI concatenation operator ## */
#undef HAVE_ANSI_CONCAT

/* Define if your cpp supports K&R-style concatenation */
#undef HAVE_KR_CONCAT

#endif
