#ifndef TYPES_H
#define TYPES_H
#include "machine.h"

/* we here define a few types with more defined values */

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

#include "memory.h"
#endif
