#include "global.h"
#include "pike_error.h"
#include "pike_memory.h"
#define ba_error Pike_error
#define EXPORT PMOD_EXPORT
#ifdef PIKE_DEBUG
#undef BA_DEBUG
#define BA_DEBUG
#endif
#define round_up32_ ba_round_up32_
#define round_up32 ba_round_up32
#include "gjalloc/gjalloc.h"
#undef round_up32_
#undef round_up32
#undef EXPORT
#undef ba_error
#undef INC
