/*
 * $Id: top.c,v 1.4 1999/07/18 15:09:04 marcus Exp $
 *
 */

#include <GL/gl.h>
#include <GL/glx.h>

#include "global.h"

#include "config.h"

RCSID("$Id: top.c,v 1.4 1999/07/18 15:09:04 marcus Exp $");
#include "stralloc.h"
#include "pike_macros.h"
#include "object.h"
#include "program.h"
#include "interpret.h"
#include "builtin_functions.h"
#include "error.h"


void pike_module_init( void )
{
  extern void add_auto_funcs(void);

  add_auto_funcs();
}


void pike_module_exit( void )
{
}

