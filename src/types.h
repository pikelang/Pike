/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef TYPES_H
#define TYPES_H

#include "machine.h"
/* we here define a few types with more defined values */

#ifdef HAVE_LIMITS_H
#include <limits.h>
#undef HAVE_LIMITS_H
#endif

#define INT64 long long

#if SIZEOF_SHORT >= 4
#define INT32 short
#else
#if SIZEOF_INT >= 4
#define INT32 int
#else
#define INT32 long
#endif
#endif

#define INT16 short
#define INT8 char

#define SIZE_T unsigned INT32

#define TYPE_T unsigned INT8
#define TYPE_FIELD unsigned INT16

#define FLOAT_TYPE float


#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#undef HAVE_SYS_TYPES_H
#endif

#endif
