#include "global.h"
#include "svalue.h"
#include "interpret.h"
#include "stralloc.h"

void f_version(INT32 args)
{
  pop_n_elems(args);
  push_text("Pike v0.4pl4");
}
