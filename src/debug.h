/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef DEBUG_H
#define DEBUG_H

#include "types.h"

/* Prototypes begin here */
struct marker;
struct marker_chunk;
INT32 checked(void *a,INT32 delta);
void init_checked();
void exit_checked();
/* Prototypes end here */

#endif
