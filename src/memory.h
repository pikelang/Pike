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
