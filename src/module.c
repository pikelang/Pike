#include "module.h"
#include "macros.h"
#include "error.h"

#include "modlist.h"

struct module *current_module=module_list;

void init_modules_efuns()
{
  unsigned int e;
  for(e=0;e<NELEM(module_list);e++)
  {
    current_module=module_list+e;
    module_list[e].init_efuns();
  }
  current_module=module_list;
}

void init_modules_programs()
{
  unsigned int e;
  for(e=0;e<NELEM(module_list);e++)
  {
    current_module=module_list+e;
    module_list[e].init_programs();
  }
  current_module=module_list;
}

void exit_modules()
{
  int e;
  for(e=NELEM(module_list)-1;e>=0;e--) module_list[e].exit();
}
