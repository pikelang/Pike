#ifdef __SSE4_2__
# include <smmintrin.h>
#endif
#include "cyclic.h"
#include "global.h"
#include "interpret.h"
#include "mapping.h"
#include "module.h"
#include "array.h"
#include "pike_error.h"
#include "pike_float.h"
#include "pike_types.h"
#include "stralloc.h"
#include "svalue.h"
#include "array.h"
#include "operators.h"
#include "builtin_functions.h"

#ifdef PIKE_NEW_BLOCK_ALLOC
/*
 * we have to rename these functions, they
 * are already defined in bitvector.h
 */
# include "block_allocator.h"
#else
/* we use this as a CVAR, and since we dont have
 * PIKE_NEW_BLOCK_ALLOC as a cmod_define, we need
 * an empty definition here
 */
struct ba_local {};
#endif

#include "dmalloc.h"

#define CB_STATIC
#define CB_INLINE
#define CB_SOURCE
#define CB_NAMESPACE
