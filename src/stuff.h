/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: stuff.h,v 1.17 2004/05/19 00:13:13 nilsson Exp $
*/

#ifndef STUFF_H
#define STUFF_H

#include "global.h"

/* Prototypes begin here */
PMOD_EXPORT int my_log2(size_t x) ATTRIBUTE((const));
PMOD_EXPORT int count_bits(unsigned INT32 x) ATTRIBUTE((const));
PMOD_EXPORT int is_more_than_one_bit(unsigned INT32 x) ATTRIBUTE((const));
PMOD_EXPORT double my_strtod(char *nptr, char **endptr);
PMOD_EXPORT unsigned INT32 my_sqrt(unsigned INT32 n) ATTRIBUTE((const));
unsigned long find_good_hash_size(unsigned long x) ATTRIBUTE((const));
/* Prototypes end here */

PMOD_EXPORT extern INT32 hashprimes[32];

#endif
