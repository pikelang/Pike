/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#ifndef MODULE_MAGIC_H
#define MODULE_MAGIC_H

#include "module.h"

/* NB: module_magic will be deprecated in the next release.
   Please use the PIKE_MODULE_INIT/PIKE_MODULE_EXIT macros
   directly in new code. */

#ifdef DYNAMIC_MODULE
#define pike_module_init(X) mYDummyFunctioN1(void); PIKE_MODULE_INIT
#define pike_module_exit(X) mYDummyFunctioN2(void); PIKE_MODULE_EXIT
#endif /* DYNAMIC_MODULE */

#endif /* MODULE_MAGIC_H */
