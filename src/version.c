/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

#include "global.h"
#include "svalue.h"
#include "interpret.h"
#include "stralloc.h"
#include "version.h"

RCSID("$Id: version.c,v 1.132 1999/03/26 19:29:54 hubbe Exp $");

#define STR(X) #X
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
