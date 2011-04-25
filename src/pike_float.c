/* Misc stuff for dealing with floats.
 *
 * $Id$
 */

#include "pike_float.h"
#include <stdio.h>
#include <string.h>

PMOD_EXPORT void format_pike_float (char *buf, FLOAT_TYPE f)
/* Writes a formatted float to the given buffer, which must be at
 * least MAX_FLOAT_SPRINTF_LEN chars long.
 *
 * For normal numbers, the format follows the standard programming
 * language format, i.e. [-]ddd[.ddd][e±ddd] in base 10, where either
 * a decimal point or an exponent always is present. The integer part
 * starts with a zero only if it is zero.
 *
 * The result can also be one of "nan", "inf" or "-inf" for those
 * special numbers. */
{
  /* There's no good way to do this:
   *
   * o  Using sprintf makes us susceptible to locale differences
   *    (affects the decimal separator, but nothing else).
   *
   * o  Temporarily changing the locale is not thread safe.
   *
   * o  Rolling an own formatter is not sufficiently trivial,
   *    considering how complicated the one in glibc is. Even if the
   *    code is ripped from somewhere, the risk of subtle formatting
   *    bugs is prohibitive.
   *
   * The conclusion is to use sprintf anyway and replace the decimal
   * separator afterwards.
   */

  char *p;

  if (PIKE_ISNAN (f)) {
    strcpy (buf, "nan");
    return;
  }

  if (PIKE_ISINF (f)) {
    if (f > 0.0)
      strcpy (buf, "inf");
    else
      strcpy (buf, "-inf");
    return;
  }

  sprintf (buf, "%.*"PRINTPIKEFLOAT"g", PIKEFLOAT_DIG, f);

  for (p = buf; *p; p++)
    if (*p != '-' && (*p < '0' || *p > '9')) break;

  if (*p == '.' || *p == 'e')
    /* Got either a standard decimal point or an exponent. We're done. */
    return;

  if (!*p) {
    /* The number has neither decimal point nor exponent. Add ".0". */
    strcpy (p, ".0");
    return;
  }

  /* Got a strange decimal separator. Replace it. */
  *p++ = '.';

  if ((*p >= '0' && *p <= '9') || !*p)
    return;

  /* Ok, the locale decimal separator is more than one char. This
   * might seem far out, but there is U+066B ARABIC DECIMAL SEPARATOR,
   * so we might be looking at a utf-8 sequence. */
  {
    char *s;
    for (s = p + 1; *s; s++)
      if (*s >= '0' && *s <= '9') break;
    memmove (p, s, strlen (s) + 1);
  }
}
