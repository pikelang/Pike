/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: stuff.h,v 1.11 2000/12/16 05:45:45 marcus Exp $
 */
#ifndef STUFF_H
#define STUFF_H

#include "global.h"

/* Prototypes begin here */
PMOD_EXPORT int my_log2(size_t x);
PMOD_EXPORT int count_bits(unsigned INT32 x);
PMOD_EXPORT int is_more_than_one_bit(unsigned INT32 x);
PMOD_EXPORT double my_strtod(char *nptr, char **endptr);
PMOD_EXPORT unsigned INT32 my_sqrt(unsigned INT32 n);
/* Prototypes end here */

PMOD_EXPORT extern INT32 hashprimes[32];

#endif
