#include "global.h"

#if !defined(HAVE_DLSYM) || !defined(HAVE_DLOPEN)
#undef HAVE_DLOPEN
#endif

#ifdef HAVE_DLOPEN
#include "interpret.h"
#include "constants.h"
#include "error.h"
#include "module.h"
#include "stralloc.h"
#include "macros.h"

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif

struct module_list
{
  struct module_list * next;
  void *module;
  struct module mod;
};

struct module_list *dynamic_module_list = 0;

void f_load_module(INT32 args)
{
  void *module;
  struct module_list *new_module;

  if(sp[-args].type != T_STRING)
    error("Bad argument 1 to load_module()\n");
  module=dlopen(sp[-args].u.string->str, RTLD_NOW);
  pop_stack();

  if(module)
  {
    struct module *tmp;
    void *init, *init2, *exit;
    init=dlsym(module, "init_module_efuns");
    init2=dlsym(module, "init_module_programs");
    exit=dlsym(module, "exit_module");

    if(!init || !init2 || !exit)
    {
      char *foo,  buf1[1024], buf2[1024];
      foo=STRRCHR(sp[-args].u.string->str,'/');
      if(!foo) foo=sp[-args].u.string->str;
      foo++;
      if(strlen(foo) < 1000)
      {
	strcpy(buf1, foo);
	foo=buf1;
      
	while((*foo <= 'a' && *foo >= 'z' ) || (*foo <= 'A' && *foo >= 'Z' ))
	  foo++;

	*foo=0;
      
	strcpy(buf2,"init_");
	strcat(buf2,buf1);
	strcat(buf2,"_efuns");
	init=dlsym(module, buf2);

	strcpy(buf2,"init_");
	strcat(buf2,buf1);
	strcat(buf2,"_programs");
	init2=dlsym(module, buf2);

	strcpy(buf2,"exit_");
	strcat(buf2,buf1);
	exit=dlsym(module, buf2);
      }
    }

    if(!init || !init2 || !exit)
      error("Failed to initialize module.\n");

    new_module=ALLOC_STRUCT(module_list);
    new_module->next=dynamic_module_list;
    dynamic_module_list=new_module;
    new_module->module=module;
    new_module->mod.init_efuns=init;
    new_module->mod.init_programs=init2;
    new_module->mod.exit=exit;
    new_module->mod.refs=0;

    tmp=current_module;
    current_module=& new_module->mod;

    (*(fun)init)();
    (*(fun)init2)();

    current_module=tmp;

    push_int(1);
  } else {
    push_int(0);
  }
}


#endif

void init_dynamic_load()
{
#ifdef HAVE_DLOPEN
  add_efun("load_module",f_load_module,"function(string:int)",OPT_SIDE_EFFECT);
#endif
}

void exit_dynamic_load()
{
#ifdef HAVE_DLOPEN
  while(dynamic_module_list)
  {
    struct module_list *tmp=dynamic_module_list;
    dynamic_module_list=tmp->next;
    (*tmp->mod.exit)();
    dlclose(tmp->module);
    free((char *)tmp);
  }
#endif
}


