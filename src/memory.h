/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef MEMORY_H
#define MEMORY_H

#include "types.h"

/* Prototypes begin here */
char *xalloc(SIZE_T size);
void reorder(char *memory,INT32 nitems,INT32 size,INT32 *order);
unsigned INT32 hashmem(const unsigned char *a,INT32 len,INT32 mlen);
unsigned INT32 hashstr(const unsigned char *str,INT32 maxn);
/* Prototypes end here */

#endif
