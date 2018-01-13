/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef MODULE_H
#define MODULE_H

#include "global.h"

#if defined(DYNAMIC_MODULE) && defined(__NT__)
#define PIKE_MODULE_INIT __declspec(dllexport) void pike_module_init(void)
#define PIKE_MODULE_EXIT __declspec(dllexport) void pike_module_exit(void)
#else
#define PIKE_MODULE_INIT PMOD_EXPORT void pike_module_init(void)
#define PIKE_MODULE_EXIT PMOD_EXPORT void pike_module_exit(void)
#endif /* DYNAMIC_MODULE && __NT__ */

PIKE_MODULE_INIT;
PIKE_MODULE_EXIT;

#endif
