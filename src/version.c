/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

#include "global.h"
#include "svalue.h"
#include "interpret.h"
#include "stralloc.h"

RCSID("$Id: version.c,v 1.34 1998/03/28 14:58:06 grubba Exp $");

void f_version(INT32 args)
{
  pop_n_elems(args);
  push_text("Pike v0.6 release 16");
}
