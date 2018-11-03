/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

/*
 * Sybase driver for the Pike programming language.
 *
 * By Francesco Chemolli <kinkie@roxen.com> 10/12/1999
 *
 */

#include "sybase_config.h"

#ifndef __PIKE_SYBASE_SYBASE_H
#define __PIKE_SYBASE_SYBASE_H
#ifdef HAVE_SYBASE

#include "threads.h"

#ifdef HAVE_SYBASEOPENCLIENT_SYBASEOPENCLIENT_H
#include <SybaseOpenClient/SybaseOpenClient.h>
#elif defined(HAVE_CTPUBLIC_H)
#include <ctpublic.h>
#endif /* HAVE_SYBASEOPENCLIENT_H || HAVE_CTPUBLIC_H */

#define SYBASE_DRIVER_VERSION "9"


#endif /* HAVE_SYBASE */
#endif /* __PIKE_SYBASE_SYBASE_H */
