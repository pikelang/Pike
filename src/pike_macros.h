/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef MACROS_H
#define MACROS_H

#include <sys/param.h>
#include "pike_memory.h"

#define BASEOF(ptr, str_type, field)  \
((struct str_type *)((char*)ptr - (char*)& (((struct str_type *)0)->field)))

#define NELEM(a) (sizeof (a) / sizeof ((a)[0]))
#define ALLOC_STRUCT(X) ( (struct X *)xalloc(sizeof(struct X)) )

#define MINIMUM(X,Y) ((X)<(Y)?(X):(Y))
#define MAXIMUM(X,Y) ((X)>(Y)?(X):(Y))


#define is8bitalnum(X)	("0000000000000000000000000000100000000000000000001111111111000000011111111111111111111111111000010111111111111111111111111110000000000000000000000000000000000000101111010110001011110110011011100000000001111111111111101111111111111111111111111111111011111111"[((unsigned)(X))&0xff] == '1')
  
#define isidchar(X) is8bitalnum(X)

#define ALIGN_BOUND sizeof(char *)
#define DO_ALIGN(X,Y) (((long)(X)+(Y-1)) & -(Y))
#define MY_ALIGN(X) DO_ALIGN(X,ALIGN_BOUND)

#endif
