#ifndef MODULE_MAGIC_H
#define MODULE_MAGIC_H

#ifdef DYNAMIC_MODULE

#ifdef __NT__

/* UGLY - hubbe */
#undef HIDE_GLOBAL_VARIABLES
#undef REVEAL_GLOBAL_VARIABLES

#define HIDE_GLOBAL_VARIABLES()
#define REVEAL_GLOBAL_VARIABLES()


#define pike_module_init mYDummyFunctioN1(void); __declspec(dllexport) void **PikeSymbol; __declspec(dllexport) void pike_module_init
#define pike_module_exit mYDummyFunctioN2(void); __declspec(dllexport) void pike_module_exit

#undef PMOD_EXPORT
#define PMOD_EXPORT __declspec(dllexport)

#include "import_functions.h"

#endif /* __NT__ */
#endif /* DYNAMIC_MODULE */

#ifndef PMOD_EXPORT
#define PMOD_EXPORT
#endif

#endif /* MODULE_MAGIC_H */


