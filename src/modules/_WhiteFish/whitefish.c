#include "global.h"
#include "stralloc.h"
#include "global.h"
RCSID("$Id: whitefish.c,v 1.2 2001/05/22 08:17:20 per Exp $");
#include "pike_macros.h"
#include "interpret.h"
#include "program.h"
#include "program_id.h"
#include "object.h"
#include "operators.h"

#include "config.h"

#include "whitefish.h"
#include "resultset.h"

/* must be included last */
#include "module_magic.h"


void pike_module_init(void)
{
  init_resultset_program();
}

void pike_module_exit(void)
{
  exit_resultset_program();
}
