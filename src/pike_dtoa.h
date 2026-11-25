/*
 * Parse and format IEEE doubles.
 *
 * See dtoa.c for details.
 */

#ifndef PIKE_DTOA_H
#define PIKE_DTOA_H

double pike_strtod(const char *s, char **end);
void pike_freedtoa(char *s);
char *pike_dtoa_r(double dd, int mode, int ndigits, int *decpt,
                  int *sign, char **rve, char *buf, size_t blen);
char *pike_dtoa(double dd, int mode, int ndigits, int *decpt,
                int *sign, char **rve);

#endif
