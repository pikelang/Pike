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
  unsigned int e;
  struct mapping *m = allocate_mapping(10);
  m->refs++;
  push_text("_static_modules");
  push_mapping(m);
  f_add_constant(2);

  for(e=0;e<NELEM(module_list);e++)
  {
    struct program *p;
    struct pike_string *s;
    start_new_program();
    module_list[e].init();
    p=end_program();

    push_text(module_list[e].name); 
    push_program(p);
    mapping_insert(m, sp-2, sp-1);
    pop_n_elems(2);
  }
}

void exit_modules(void)
{
  int e;
  for(e=NELEM(module_list)-1;e>=0;e--)
    module_list[e].exit();
}
