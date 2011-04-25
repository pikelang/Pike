/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#ifndef MODULE_MAGIC_H
#define MODULE_MAGIC_H

#include "module.h"

/* NB: use of module_magic is deprecated.
   Please use the PIKE_MODULE_INIT/PIKE_MODULE_EXIT macros
   directly in new code. */


#ifndef ALLOW_DEPRECATED_MODULE_MAGIC

/* To get rid of the following message, define ALLOW_DEPRECATED_MODULE_MAGIC.
   This option is a transitional measure and may be removed at some
   future time.  Please update your code to use PIKE_MODULE_INIT/
   PIKE_MODULE_EXIT instead. */

#error module_magic.h is deprecated.  Please read the header file for details.

#endif /* ALLOW_DEPRECATED_MODULE_MAGIC */


#ifdef DYNAMIC_MODULE
#define pike_module_init(X) mYDummyFunctioN1(void); PIKE_MODULE_INIT
#define pike_module_exit(X) mYDummyFunctioN2(void); PIKE_MODULE_EXIT
#endif /* DYNAMIC_MODULE */

#endif /* MODULE_MAGIC_H */
