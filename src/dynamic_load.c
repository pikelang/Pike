#include "global.h"

#if !defined(HAVE_DLSYM) || !defined(HAVE_DLOPEN)
#undef HAVE_DLOPEN
#endif

#if !defined(HAVE_DLOPEN) && defined(HAVE_DLD_LINK) && defined(HAVE_DLD_GET_FUNC)
#define USE_DLD
#endif

#if defined(HAVE_DLOPEN) || defined(USE_DLD)
#include "interpret.h"
#include "constants.h"
#include "error.h"
#include "module.h"
#include "stralloc.h"
#include "macros.h"

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#ifdef HAVE_DLD_H
#include <dld.h>
#endif

struct module_list
{
  struct module_list * next;
#ifdef HAVE_DLOPEN
  void *module;
#elif defined(USE_DLD)
  char *module_path;
#endif
  struct module mod;
};

struct module_list *dynamic_module_list = 0;

void f_load_module(INT32 args)
{
#ifdef HAVE_DLOPEN
  void *module;
#endif
  struct module_list *new_module;
  const char *module_name;
  int res;

  if(sp[-args].type != T_STRING)
    error("Bad argument 1 to load_module()\n");

  module_name = sp[-args].u.string->str;

#ifdef HAVE_DLOPEN

#ifndef RTLD_NOW
#define RTLD_NOW 0
#endif
  module=dlopen(module_name, RTLD_NOW);

  if(module)
#endif /* HAVE_DLOPEN */
  {
    struct module *tmp;
    fun init, init2, exit;

#ifdef HAVE_DLOPEN
    init=(fun)dlsym(module, "init_module_efuns");
    init2=(fun)dlsym(module, "init_module_programs");
    exit=(fun)dlsym(module, "exit_module");

    if(!init || !init2 || !exit)
#endif /* HAVE_DLOPEN */
    {
      char *foo,  buf1[1024], buf2[1024];
      foo=STRRCHR(sp[-args].u.string->str,'/');
      if(foo)
	foo++;
      else
	foo=sp[-args].u.string->str;
      if(strlen(foo) < 1000)
      {
	strcpy(buf1, foo);
	foo=buf1;

	/* Strip extension, if any */
	foo = STRCHR(foo, '.');
	if (foo)
	  *foo=0;
      
	strcpy(buf2,"init_");
	strcat(buf2,buf1);
	strcat(buf2,"_efuns");
#ifdef HAVE_DLOPEN
	init=(fun)dlsym(module, buf2);
#elif defined(USE_DLD)
	dld_create_reference(buf2);
	if (dld_link(module_name)) {
	  error("load_module(\"%s\") failed: %s\n",
		module_name, dld_strerror(dld_errno));
	}
	init = (fun)dld_get_func(buf2);
#endif /* USE_DLD */

	strcpy(buf2,"init_");
	strcat(buf2,buf1);
	strcat(buf2,"_programs");
#ifdef HAVE_DLOPEN
	init2=(fun)dlsym(module, buf2);
#elif defined(USE_DLD)
	init2=(fun)dld_get_func(buf2);
#endif

	strcpy(buf2,"exit_");
	strcat(buf2,buf1);
#ifdef HAVE_DLOPEN
	exit=(fun)dlsym(module, buf2);
#elif defined(USE_DLD)
	exit=(fun)dld_get_func(buf2);
#endif
      }
    }

    if(!init || !init2 || !exit)
      error("Failed to initialize module.\n");

    new_module=ALLOC_STRUCT(module_list);
    new_module->next=dynamic_module_list;
    dynamic_module_list=new_module;
#ifdef HAVE_DLOPEN
    new_module->module=module;
#elif defined(USE_DLD)
    new_module->module_path = 0;
    new_module->module_path = xalloc(strlen(module_name) + 1);
    strcpy(new_module->module_path,module_name);
#endif
    new_module->mod.init_efuns=init;
    new_module->mod.init_programs=init2;
    new_module->mod.exit=exit;
    new_module->mod.refs=0;

    tmp=current_module;
    current_module = & new_module->mod;

    (*(fun)init)();
    (*(fun)init2)();

    current_module=tmp;

    res = 1;
  }
#ifdef HAVE_DLOPEN
  else {
    error("load_module(\"%s\") failed: %s\n",
	  sp[-args].u.string->str, dlerror());
  }
#endif
  pop_n_elems(args);
  push_int(res);
}


#endif /* HAVE_DLOPEN || USE_DLD */

void init_dynamic_load()
{
#ifdef USE_DLD
  if (dld_init(dld_find_executable("pike"))) /* should be argv[0] */
    return;
#endif

#if defined(HAVE_DLOPEN) || defined(USE_DLD)
  add_efun("load_module",f_load_module,"function(string:int)",OPT_SIDE_EFFECT);
#endif
}

void exit_dynamic_load()
{
#if defined(HAVE_DLOPEN) || defined(USE_DLD)
  while(dynamic_module_list)
  {
    struct module_list *tmp=dynamic_module_list;
    dynamic_module_list=tmp->next;
    (*tmp->mod.exit)();
#ifdef HAVE_DLOPEN
    dlclose(tmp->module);
#elif defined(USE_DLD)
    if (tmp->module_path) {
      dld_unlink_by_file(tmp->module_path, 1);
      free(tmp->module_path);
    }
#endif
    free((char *)tmp);
  }
#endif
}


