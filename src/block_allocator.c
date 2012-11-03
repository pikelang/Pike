#define ba_error Pike_error
#include "global.h"
#include "pike_error.h"
#include "pike_memory.h"

#define EXPORT PMOD_EXPORT
#define ctz ctz32
#define BA_CMEMSET cmemset
#define BA_XALLOC xalloc

#ifdef PIKE_DEBUG
# undef BA_DEBUG
# define BA_DEBUG
#endif

#ifdef USE_VALGRIND
# define BA_USE_VALGRIND
#endif

#include "gjalloc/gjalloc.c"
