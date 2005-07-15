/* Misc stuff for dealing with floats.
 *
 * $Id: pike_float.h,v 1.8 2005/07/15 18:16:52 grubba Exp $
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

#endif	/* !PIKE_FLOAT_H */
