/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#include "global.h"
#include "svalue.h"
#include "interpret.h"
#include "stralloc.h"
#include "version.h"

RCSID("$Id$");

/*! @decl string version()
 *!
 *! Report the version of Pike.
 *!
 *! @seealso
 *!   @[__VERSION__], @[__MINOR__], @[__BUILD__],
 *!   @[__REAL_VERSION__], @[__REAL_MINOR__], @[__REAL_BUILD__],
 */
void f_version(INT32 args)
{
  char buffer[128];
  sprintf(buffer,"Pike v%d.%d release %d",
	  PIKE_MAJOR_VERSION,
	  PIKE_MINOR_VERSION,
	  PIKE_BUILD_VERSION);
  pop_n_elems(args);
  push_text(buffer);
}
