/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#ifndef MODULE_H
#define MODULE_H

#include "global.h"

#if defined(DYNAMIC_MODULE) && defined(__NT__)
#define PIKE_MODULE_INIT __declspec(dllexport) void pike_module_init(void)
#define PIKE_MODULE_EXIT __declspec(dllexport) void pike_module_exit(void)
#else
#define PIKE_MODULE_INIT void pike_module_init(void)
#define PIKE_MODULE_EXIT void pike_module_exit(void)
#endif /* DYNAMIC_MODULE && __NT__ */

/* Prototypes begin here */
struct static_module;
void init_modules(void);
void exit_modules(void);
PIKE_MODULE_INIT;
PIKE_MODULE_EXIT;
/* Prototypes end here */

#endif
