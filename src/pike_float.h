/* Misc stuff for dealing with floats.
 *
 * $Id: pike_float.h,v 1.1 2003/11/07 17:41:53 mast Exp $
 */

#ifndef PIKE_FLOAT_TYPES_H
#define PIKE_FLOAT_TYPES_H

#include "global.h"

#ifdef HAVE_FLOATINGPOINT_H
#include <floatingpoint.h>
#endif
#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif
#ifdef HAVE_FLOAT_H
#include <float.h>
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
static inline int pike_isnan(double x)
{
  return ((x == 0.0) == (x < 0.0)) &&
    ((x == 0.0) == (x > 0.0));
}
#define PIKE_ISNAN(X)	pike_isnan(X)
#endif /* HAVE__ISNAN */
#endif /* HAVE_ISNAN */

#endif	/* !PIKE_FLOAT_TYPES_H */
