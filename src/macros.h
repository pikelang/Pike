/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef MACROS_H
#define MACROS_H

#include <sys/param.h>
#include "memory.h"

#define BASEOF(ptr, str_type, field)  \
((struct str_type *)((char*)ptr - (char*)& (((struct str_type *)0)->field)))

#define NELEM(a) (sizeof (a) / sizeof ((a)[0]))
#define ALLOC_STRUCT(X) ( (struct X *)xalloc(sizeof(struct X)) )

#define MINIMUM(X,Y) ((X)<(Y)?(X):(Y))
#define MAXIMUM(X,Y) ((X)>(Y)?(X):(Y))

#define isidchar(X) (isalnum(X) || (X)=='_')

#define ALIGN_BOUND sizeof(char *)
#define DO_ALIGN(X,Y) (((long)(X)+(Y-1)) & -(Y))
#define MY_ALIGN(X) DO_ALIGN(X,ALIGN_BOUND)

#endif
