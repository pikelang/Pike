#ifndef TESTING
#  include "global.h"
#  include "interpret.h"
#  include "constants.h"
#  include "error.h"
#  include "module.h"
#  include "stralloc.h"
#  include "pike_macros.h"

RCSID("$Id: dynamic_load.c,v 1.32 1999/01/31 23:50:15 per Exp $");

#endif /* !TESTING */

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif /* HAVE_ERRNO_H */
#ifdef HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#if !defined(HAVE_DLOPEN)

#if defined(HAVE_DLD_LINK) && defined(HAVE_DLD_GET_FUNC)
#define USE_DLD
#define HAVE_SOME_DLOPEN
#define EMULATE_DLOPEN
#else
#if defined(HAVE_SHL_LOAD) && defined(HAVE_DL_H)
#define USE_HPUX_DL
#define HAVE_SOME_DLOPEN
#define EMULATE_DLOPEN
#else
#if defined(HAVE_LOADLIBRARY) && defined(HAVE_FREELIBRARY) && \
    defined(HAVE_GETPROCADDRESS) && defined(HAVE_WINBASE_H)
#define USE_LOADLIBRARY
#define HAVE_SOME_DLOPEN
#define EMULATE_DLOPEN
#endif
#endif
#endif
#else
#define HAVE_SOME_DLOPEN
#endif


#ifdef HAVE_SOME_DLOPEN

typedef void (*modfun)(void);

#ifdef USE_LOADLIBRARY
#include <windows.h>

static TCHAR *convert_string(char *str, int len)
{
  int e;
  TCHAR *ret=(TCHAR *)xalloc((len+1) * sizeof(TCHAR));
  for(e=0;e<len;e++) ret[e]=EXTRACT_UCHAR(str+e);
  ret[e]=0;
  return ret;
}

static void *dlopen(char *foo, int how)
{
  TCHAR *tmp;
  HINSTANCE ret;
  tmp=convert_string(foo, strlen(foo));
  ret=LoadLibrary(tmp);
  free((char *)tmp);
  return (void *)ret;
}

static char * dlerror(void)
{
  static char buffer[200];
  sprintf(buffer,"LoadLibrary failed with error: %d",GetLastError());
  return buffer;
}

static void *dlsym(void *module, char * function)
{
  return (void *)GetProcAddress((HMODULE)module,
				function);
}

static void dlclose(void *module)
{
  FreeLibrary((HMODULE)module);
}

#define dlinit()

#endif /* USE_LOADLIBRARY */


#ifdef USE_DLD
#include <dld.h>
static void *dlopen(char *module_name, int how)
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

#endif /* USE_DLD */


#ifdef USE_HPUX_DL

#include <dl.h>

#if defined(BIND_VERBOSE)
#define RTLD_NOW	BIND_IMMEDIATE | BIND_VERBOSE
#else
#define RTLD_NOW	BIND_IMMEDIATE
#endif /* BIND_VERBOSE */

extern int errno;

static void *dlopen(char *libname, int how)
{
  shl_t lib;

  lib = shl_load(libname, how, 0L);

  return (void *)lib;
}

static char *dlerror(void)
{
#ifdef HAVE_STRERROR
  return strerror(errno);
#else
  return ""; /* I hope it's better than null..*/
#endif
}

static void *dlsym(void *module, char *function)
{
  void *func;
  int result;
  shl_t mod = (shl_t)module;

  result = shl_findsym(&mod, function, TYPE_UNDEFINED, &func);
  if (result == -1)
    return NULL;
  return func;
}

static void dlclose(void *module)
{
  shl_unload((shl_t)module);
}

#define dlinit()

#endif /* USE_HPUX_DL */


#ifndef EMULATE_DLOPEN

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#define dlinit()
#endif  /* !EMULATE_DLOPEN */


#endif /* HAVE_SOME_DLOPEN */

#ifndef RTLD_NOW
#define RTLD_NOW 0
#endif

#ifndef RTLD_LAZY
#define RTLD_LAZY 0
#endif

#ifndef RTLD_GLOBAL
#define RTLD_GLOBAL 0 
#endif

#ifndef TESTING

#if defined(HAVE_DLOPEN) || defined(USE_DLD) || defined(USE_HPUX_DL)

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

  module=dlopen(module_name, 
                (d_flag?RTLD_NOW|RTLD_GLOBAL:RTLD_LAZY|RTLD_GLOBAL));

  if(!module)
  {
    const char *err = dlerror();
    if(!err) err = "Unknown reason";
    if (sp[-args].u.string->len < 1024) {
      error("load_module(\"%s\") failed: %s\n",
	    sp[-args].u.string->str, err);
    } else {
      error("load_module() failed: %s\n", err);
    }
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

    if (strlen(module_name) < 1024) {
      error("Failed to initialize dynamic module \"%s\".\n", module_name);
    } else {
      error("Failed to initialize dynamic module.\n");
    }
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

#endif /* HAVE_DLOPEN || USE_DLD || USE_HPUX_DL */


void init_dynamic_load(void)
{
#if defined(HAVE_DLOPEN) || defined(USE_DLD) || defined(USE_HPUX_DL)
  dlinit();

  add_efun("load_module",f_load_module,"function(string:program)",OPT_EXTERNAL_DEPEND);
#endif
}

void exit_dynamic_load(void)
{
#if defined(HAVE_DLOPEN) || defined(USE_DLD) || defined(USE_HPUX_DL)
  while(dynamic_module_list)
  {
    struct module_list *tmp=dynamic_module_list;
    dynamic_module_list=tmp->next;
    (*tmp->exit)();
#ifndef DEBUG_MALLOC
    dlclose(tmp->module);
#endif
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
