/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: version.c,v 1.143 2005/05/27 18:34:22 mast Exp $
*/

#include "global.h"
#include "svalue.h"
#include "interpret.h"
#include "stralloc.h"
#include "version.h"

/*! @decl string version()
 *!
 *! Report the version of Pike. Does the same as
 *! @code
 *! sprintf("Pike v%d.%d release %d", __REAL_VERSION__,
 *!         __REAL_MINOR__, __REAL_BUILD__);
 *! @endcode
 *!
 *! @seealso
 *!   @[__VERSION__], @[__MINOR__], @[__BUILD__],
 *!   @[__REAL_VERSION__], @[__REAL_MINOR__], @[__REAL_BUILD__],
 */
void f_version(INT32 args)
{
  pop_n_elems(args);
  push_constant_text ("Pike v"
		      DEFINETOSTR (PIKE_MAJOR_VERSION)
		      "."
		      DEFINETOSTR (PIKE_MINOR_VERSION)
		      " release "
		      DEFINETOSTR (PIKE_BUILD_VERSION));
}

void push_compact_version()
{
  push_constant_string_code (str, {
      str = begin_wide_shared_string (3, 2);
      STR2(str)[0] = PIKE_MAJOR_VERSION;
      STR2(str)[1] = PIKE_MINOR_VERSION;
      STR2(str)[2] = PIKE_BUILD_VERSION;
      str = end_shared_string (str);
    });
}
