#define ba_error Pike_error
#include "global.h"
#include "pike_error.h"
#include "pike_memory.h"

#define EXPORT PMOD_EXPORT
#define ctz ctz32
#define BA_CMEMSET cmemset

#include "GJAlloc/block_allocator.c"
