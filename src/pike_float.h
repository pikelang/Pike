/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

/* Misc stuff for dealing with floats.
 */

#ifndef PIKE_FLOAT_H
#define PIKE_FLOAT_H

#include "global.h"

#include <math.h>

#ifdef HAVE_FLOATINGPOINT_H
#include <floatingpoint.h>
#endif
#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif
#ifdef HAVE_FP_CLASS_H
#include <fp_class.h>
#endif

#if defined(HAVE__ISNAN) && !defined(HAVE_ISNAN) && !defined(isnan)
/* Only fall back to using _isnan() if isnan() does not exist. */
#define PIKE_ISNAN(X) _isnan(X)
#else
#define PIKE_ISNAN(X) isnan(X)
#endif

#ifdef HAVE_ISINF
#define PIKE_ISINF(X)	isinf(X)
#elif defined(HAVE_ISFINITE)
#define PIKE_ISINF(X)   (!isfinite(X))
#elif defined(HAVE_FINITE)
#define PIKE_ISINF(X)   (!finite(X))
#elif defined(HAVE__FINITE)
#define PIKE_ISINF(X)   (!_finite(X))
#else
#define PIKE_ISINF(X)	((X) && ((X)+(X) == (X)))
#endif /* HAVE_ISINF */

#ifdef INFINITY
#define MAKE_INF() INFINITY
#else
#define MAKE_INF() (DBL_MAX+DBL_MAX)
#endif

#ifdef HAVE_NAN
#define MAKE_NAN() (nan(""))
#else
#define MAKE_NAN() (MAKE_INF()-MAKE_INF())
#endif


#ifdef HAVE_ISUNORDERED
#define PIKE_ISUNORDERED(X,Y) isunordered(X,Y)
#else
#define PIKE_ISUNORDERED(X,Y) (PIKE_ISNAN(X)||PIKE_ISNAN(Y))
#endif /* HAVE_ISUNORDERED */

#ifndef FLOAT_IS_IEEE_BIG
#ifndef FLOAT_IS_IEEE_LITTLE
#define NEED_CUSTOM_IEEE
#endif
#endif
#ifndef NEED_CUSTOM_IEEE
#ifndef DOUBLE_IS_IEEE_BIG
#ifndef DOUBLE_IS_IEEE_LITTLE
#define NEED_CUSTOM_IEEE
#endif
#endif
#endif

/* This calculation should always give some margin based on the size. */
/* It utilizes that log10(256) ~= 2.4 < 5/2. */
/* One quarter of the float is the exponent. */
#define MAX_FLOAT_EXP_LEN ((SIZEOF_FLOAT_TYPE * 5 + 4) / 8)

/* Ten extra bytes: Mantissa sign, decimal point (up to four bytes),
 * zero before the decimal point, the 'e', the exponent sign, an extra
 * digit due to the mantissa/exponent split, and the \0 termination.
 *
 * Four bytes for the decimal point is built on the assumption that it
 * can be any unicode char, which in utf8 encoding can take up to four
 * bytes. See format_pike_float. */
#define MAX_FLOAT_SPRINTF_LEN (10 + PIKEFLOAT_DIG + MAX_FLOAT_EXP_LEN)

PMOD_EXPORT void format_pike_float (char *buf, FLOAT_TYPE f);

#endif	/* !PIKE_FLOAT_H */
