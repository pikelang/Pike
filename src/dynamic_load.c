#ifndef TESTING
#  include "global.h"
#  include "interpret.h"
#  include "constants.h"
#  include "error.h"
#  include "module.h"
#  include "stralloc.h"
#  include "pike_macros.h"
#endif

#if !defined(HAVE_DLOPEN) && defined(HAVE_DLD_LINK) && defined(HAVE_DLD_GET_FUNC)
#define USE_DLD
#endif

#if defined(HAVE_DLOPEN) || defined(USE_DLD)

typedef void (*modfun)(void);

#ifdef USE_DLD
#include <dld.h>
static void *dlopen(char *foo, int how)
{
  dld_create_reference("pike_module_init");
  if(dld_link(module_name))
  {
    return (void *)strdup(module_name);
  }else{
    return 0;
  }
}

static char *dlerror(void)
{
  return dld_strerror(dld_errno);
}

static void *dlsym(void *module, char *function)
{
  return dld_get_func(function);
}

static void *dlclose(void *module)
{
  if(!module) return;
  dld_unlink_by_file((char *)module);
  free(module);
}

static void dlinit(void)
{
  extern char ** ARGV;
  if(dld_init(dld_find_executable(ARGV[0])))
  {
    fprintf(stderr,"Failed to init dld\n");
    exit(1);
  }
}


#else
#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#define dlinit()
#endif

#ifndef RTLD_NOW
#define RTLD_NOW 0
#endif


#endif /* HAVE_DLOPEN || USE_DLD */

#ifndef TESTING

#if defined(HAVE_DLOPEN) || defined(USE_DLD)

struct module_list
{
  struct module_list * next;
  void *module;
  modfun init, exit;
};

struct module_list *dynamic_module_list = 0;

void f_load_module(INT32 args)
{
  void *module;
  modfun init, exit;
  struct module_list *new_module;
  const char *module_name;

  if(sp[-args].type != T_STRING)
    error("Bad argument 1 to load_module()\n");

  module_name = sp[-args].u.string->str;

  module=dlopen(module_name, RTLD_NOW);
  if(!module)
  {
    const char *err = dlerror();
    if(!err) err = "Unknown reason";
    error("load_module(\"%s\") failed: %s\n",
	  sp[-args].u.string->str, err);
  }

  init=(modfun)dlsym(module, "pike_module_init");
  if (!init) {
    init=(modfun)dlsym(module, "_pike_module_init");
  }
  exit=(modfun)dlsym(module, "pike_module_exit");
  if (!exit) {
    exit=(modfun)dlsym(module, "_pike_module_exit");
  }

  if(!init || !exit)
  {
    dlclose(module);
    
    error("Failed to initialize module \"%s\".\n", module_name);
  }

  new_module=ALLOC_STRUCT(module_list);
  new_module->next=dynamic_module_list;
  dynamic_module_list=new_module;
  new_module->module=module;
  new_module->init=init;
  new_module->exit=exit;

  pop_n_elems(args);
  start_new_program();
  (*(modfun)init)();

  push_program(end_program());
}


#endif /* HAVE_DLOPEN || USE_DLD */

void init_dynamic_load(void)
{
#if defined(HAVE_DLOPEN) || defined(USE_DLD)
  dlinit();

  add_efun("load_module",f_load_module,"function(string:program)",OPT_EXTERNAL_DEPEND);
#endif
}

void exit_dynamic_load(void)
{
#if defined(HAVE_DLOPEN) || defined(USE_DLD)
  while(dynamic_module_list)
  {
    struct module_list *tmp=dynamic_module_list;
    dynamic_module_list=tmp->next;
    (*tmp->exit)();
    dlclose(tmp->module);
    free((char *)tmp);
  }
#endif
}


#else /* TESTING */
#include <stdio.h>

int main()
{
  void *module,*fun;
  dlinit();
  module=dlopen("./myconftest.so",RTLD_NOW);
  if(!module)
  {
    fprintf(stderr,"Failed to link myconftest.so: %s\n",dlerror());
    exit(1);
  }
  fun=dlsym(module,"testfunc");
  if(!fun) fun=dlsym(module,"_testfunc");
  if(!fun)
  {
    fprintf(stderr,"Failed to find function testfunc: %s\n",dlerror());
    exit(1);
  }
  fprintf(stderr,"Calling testfunc\n");
  ((void (*)(void))fun)();
  fprintf(stderr,"testfunc returned!\n");
  exit(1);
}
#endif
