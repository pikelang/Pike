/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

#include "global.h"
#include "svalue.h"
#include "interpret.h"
#include "stralloc.h"

void f_version(INT32 args)
{
  pop_n_elems(args);
  push_text("Pike v0.5 release 63");
}
