/* Misc stuff for dealing with floats.
 *
 * $Id: pike_float.h,v 1.9 2010/05/27 23:17:09 mast Exp $
 */

#ifndef PIKE_FLOAT_H
#define PIKE_FLOAT_H

#include "global.h"

#ifdef HAVE_FLOATINGPOINT_H
#include <floatingpoint.h>
#endif
#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif
#ifdef HAVE_FP_CLASS_H
#include <fp_class.h>
#endif

/* isnan()...
 */
#ifdef HAVE_ISNAN
#if defined(HAVE__ISNAN) && defined(__NT__)
/* On NT only _isnan() has a prototype.
 * isnan() is the standardized name, so use that
 * on all other platforms.
 */
#define PIKE_ISNAN(X)	_isnan(X)
#else /* !(HAVE__ISNAN && __NT__) */
#define PIKE_ISNAN(X)	isnan(X)
#endif /* HAVE__ISNAN && __NT__ */
#else /* !HAVE_ISNAN */
#ifdef HAVE__ISNAN
#define PIKE_ISNAN(X)	_isnan(X)
#else /* !HAVE__ISNAN */
/* Fallback function */
static INLINE int pike_isnan(double x)
{
  return ((x == 0.0) == (x < 0.0)) &&
    ((x == 0.0) == (x > 0.0));
}
#define PIKE_ISNAN(X)	pike_isnan(X)
#endif /* HAVE__ISNAN */
#endif /* HAVE_ISNAN */

/* isinf()...
 */
#ifdef HAVE_ISINF
#define PIKE_ISINF(X)	isinf(X)
#else /* HAVE_ISINF */
#define PIKE_ISINF(X)	((X) && ((X)+(X) == (X)))
#endif /* HAVE_ISINF */

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
