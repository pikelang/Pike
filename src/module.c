/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
#include "module.h"
#include "pike_macros.h"
#include "error.h"
#include "builtin_functions.h"
#include "main.h"
#include "svalue.h"
#include "interpret.h"
#include "stralloc.h"
#include "object.h"
#include "mapping.h"

#include "modules/modlist_headers.h"

RCSID("$Id: module.c,v 1.10 1999/02/10 21:46:44 hubbe Exp $");

typedef void (*modfun)(void);

struct static_module
{
  char *name;
  modfun init;
  modfun exit;
};

static struct static_module module_list[] = {
  { "Builtin", low_init_main, low_exit_main }
#include "modules/modlist.h"
  ,{ "Builtin2", init_main, exit_main }
};

void init_modules(void)
{
  struct program *p;
  unsigned int e;

  start_new_program();

  for(e=0;e<NELEM(module_list);e++)
  {
    start_new_program();
    module_list[e].init();
    debug_end_class(module_list[e].name,strlen(module_list[e].name),0);
  }
  push_text("_static_modules");
  push_object(low_clone(p=end_program()));
  f_add_constant(2);
  free_program(p);
}

void exit_modules(void)
{
  int e;
  for(e=NELEM(module_list)-1;e>=0;e--)
    module_list[e].exit();
}
