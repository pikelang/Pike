/*
 * Format IEEE-doubles.
 *
 * See dtoa.c for details.
 */

#include "global.h"

#ifndef DEBUG
/* Conflicts with redefinition in dtoa.c. */
#undef assert
#endif

#include "pike_dtoa.h"

/*
 * Set up macros used by dtoa.c.
 */

#if PIKE_BYTEORDER == 4321
#define IEEE_MC68k
#else
#define IEEE_8087
#endif

#if SIZEOF_INT == 4
#define Long int
#elif SIZEOF_LONG == 4
#define Long long
#endif

#if !SIZEOF_LONG_LONG
#define NO_LONG_LONG
#endif

#ifndef HAVE_FLOAT_H
#define Bad_float_h
#endif

/* Enable pike changes (mostly signed/unsigned fixes). */
#define PIKE_CHANGE(X)	X

/* Rename non-static symbols to avoid potential clashes with other code. */
#ifndef SET_INEXACT
#define dtoa_divmax pike_dtoa_divmax
#endif
#ifdef DIGLIM_DEBUG
#define strtod_diglim pike_strtod_diglim
#endif
#define rnd_prod pike_rnd_prod
#define rnd_quot pike_rnd_quot
#define strtod pike_strtod
#define dtoa pike_dtoa
#define set_max_dtoa_threads pike_set_max_dtoa_threads
#define gethex pike_gethex
#define freedtoa pike_freedtoa
#define dtoa_r pike_dtoa_r

/* netlib/fp
 *
 * https://netlib.sandia.gov/fp/
 *
 * Floating point formatting by David Gay.
 */
#include "dtoa.c"
