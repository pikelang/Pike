/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef STUFF_H
#define STUFF_H

#include "global.h"

/* Prototypes begin here */
int my_log2(unsigned INT32 x);
int count_bits(unsigned INT32 x);
int is_more_than_one_bit(unsigned INT32 x);
/* Prototypes end here */

extern INT32 hashprimes[32];

#endif
