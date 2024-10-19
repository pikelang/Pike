/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifdef TESTING
#define NO_PIKE_INCLUDES
#define CREATE_MAIN
#define NO_PIKE_GUTS
#endif

#ifndef NO_PIKE_INCLUDES
#  include "global.h"
#  include "interpret.h"
#  include "constants.h"
#  include "pike_error.h"
#  include "pike_compiler.h"
#  include "stralloc.h"
#  include "pike_macros.h"
#  include "main.h"
#  include "constants.h"
#  include "lex.h"
#  include "object.h"
#  include "cyclic.h"
#  include "pike_modules.h"

#else /* TESTING */

#include <stdio.h>
#include <string.h>

#endif /* !TESTING */

#include <errno.h>

#if !defined(HAVE_DLOPEN) || defined(USE_SEMIDYNAMIC_MODULES)

#ifdef USE_SEMIDYNAMIC_MODULES
#undef HAVE_DLOPEN
#define USE_STATIC_MODULES
#define HAVE_SOME_DLOPEN
#define EMULATE_DLOPEN
#define USE_DYNAMIC_MODULES
#elif defined(HAVE_DLD_LINK) && defined(HAVE_DLD_GET_FUNC)
#define USE_DLD
#define HAVE_SOME_DLOPEN
#define EMULATE_DLOPEN
#elif defined(HAVE_SHL_LOAD) && defined(HAVE_DL_H)
#define USE_HPUX_DL
#define HAVE_SOME_DLOPEN
#define EMULATE_DLOPEN
#elif defined(USE_DLL) && \
      defined(HAVE_LOADLIBRARY) && defined(HAVE_FREELIBRARY) && \
      defined(HAVE_GETPROCADDRESS) && defined(HAVE_WINBASE_H)
#define USE_LOADLIBRARY
#define HAVE_SOME_DLOPEN
#define EMULATE_DLOPEN
#elif defined(HAVE_MACH_O_DYLD_H)
/* MacOS X... */
#define USE_DYLD
#define HAVE_SOME_DLOPEN
#define EMULATE_DLOPEN
#endif

#else
/* HAVE_DLOPEN */
#define HAVE_SOME_DLOPEN
#endif


#ifdef HAVE_SOME_DLOPEN

typedef void (*modfun)(void);

#ifdef USE_STATIC_MODULES

static void *dlopen(const char *foo, int UNUSED(how))
{
  struct pike_string *s = low_read_file(foo);
  char *name, *end;
  void *res;

  if (!s) return NULL;
  if (strncmp(s->str, "PMODULE=\"", 9)) {
    free_string(s);
    return NULL;
  }
  name = s->str + 9;
  if (!(end = strchr(name, '\"'))) {
    free_string(s);
    return NULL;
  }

  res = (void *)find_semidynamic_module(name, end - name);
  free_string(s);
  return res;
}

static char *dlerror(void)
{
  return "Invalid dynamic module.";
}

static void *dlsym(void *module, char *function)
{
  if (!strcmp(function, "pike_module_init"))
    return get_semidynamic_init_fun(module);
  if (!strcmp(function, "pike_module_exit"))
    return get_semidynamic_exit_fun(module);
  return NULL;
}

static int dlinit(void)
{
  return 1;
}

static void dlclose(void *UNUSED(module))
{
}

#elif defined(USE_LOADLIBRARY)
#include <windows.h>

static TCHAR *convert_string(const char *str, ptrdiff_t len)
{
  ptrdiff_t e;
  TCHAR *ret=xalloc((len+1) * sizeof(TCHAR));
  for(e=0;e<len;e++) ret[e]=EXTRACT_UCHAR(str+e);
  ret[e]=0;
  return ret;
}

static void *dlopen(const char *foo, int how)
{
  TCHAR *tmp;
  HINSTANCE ret;
  tmp=convert_string(foo, strlen(foo));
  ret=LoadLibrary(tmp);
  free(tmp);
  return (void *)ret;
}

static char * dlerror(void)
{
  static char buffer[200];
  int err = GetLastError();
  switch(err) {
  case ERROR_MOD_NOT_FOUND:
    return "The specified module could not be found.";
  default:
    sprintf(buffer,"LoadLibrary failed with error: %d",GetLastError());
  }
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

#define dlinit()	1

#endif /* USE_LOADLIBRARY */


#ifdef USE_DLD
#include <dld.h>
static void *dlopen(const char *module_name, int how)
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

static int dlinit(void)
{
  extern char ** ARGV;
  if(dld_init(dld_find_executable(ARGV[0])))
  {
    fprintf(stderr,"Failed to init dld\n");
    return 0;
  }
  /* OK */
  return 1;
}

#endif /* USE_DLD */


#ifdef USE_HPUX_DL

#include <dl.h>

#ifdef BIND_VERBOSE
#define RTLD_NOW	BIND_IMMEDIATE | BIND_VERBOSE
#else
#define RTLD_NOW	BIND_IMMEDIATE
#endif /* BIND_VERBOSE */

extern int errno;

static void *dlopen(const char *libname, int how)
{
  shl_t lib;

  lib = shl_load(libname, how, 0L);

  return (void *)lib;
}

static char *dlerror(void)
{
  return strerror(errno);
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

#define dlinit()	1

#endif /* USE_HPUX_DL */

#ifdef USE_DYLD

#include <mach-o/dyld.h>

#define RTLD_NOW	NSLINKMODULE_OPTION_BINDNOW

#define dlinit()	_dyld_present()

struct pike_dl_handle
{
  NSObjectFileImage image;
  NSModule module;
};

static void *dlclose(void *handle_)
{
  struct pike_dl_handle *handle = handle_;
  if (handle) {
    if (handle->module)
      NSUnLinkModule(handle->module, NSUNLINKMODULE_OPTION_NONE);
    handle->module = NULL;
    if (handle->image)
      NSDestroyObjectFileImage(handle->image);
    handle->image = NULL;
    free(handle);
  }
  return NULL;
}

static char *pike_dl_error = NULL;

static void *dlopen(const char *module_name, int how)
{
  struct pike_dl_handle *handle = malloc(sizeof(struct pike_dl_handle));
  NSObjectFileImageReturnCode code = 0;

  pike_dl_error = NULL;
  if (!handle) {
    pike_dl_error = "Out of memory.";
    return NULL;
  }

  handle->image = NULL;
  handle->module = NULL;

  /* FIXME: Should be fixed to detect if the module already is loaded. */
  if ((code = NSCreateObjectFileImageFromFile(module_name,
					      &handle->image)) !=
      NSObjectFileImageSuccess) {
    DWERR("NSCreateObjectFileImageFromFile(\"%s\") failed with %d\n",
          module_name, code);
    pike_dl_error = "NSCreateObjectFileImageFromFile() failed.";
    dlclose(handle);
    return NULL;
  }

  handle->module = NSLinkModule(handle->image, module_name,
				how | NSLINKMODULE_OPTION_RETURN_ON_ERROR |
				NSLINKMODULE_OPTION_PRIVATE);
  if (!handle->module) {
    dlclose(handle);
    return NULL;
  }
  return handle;
}

static void *dlsym(void *handle, char *function)
{
  NSSymbol symbol =
    NSLookupSymbolInModule(((struct pike_dl_handle *)handle)->module,
			   function);
  return symbol?NSAddressOfSymbol(symbol):NULL;
}

static const char *dlerror(void)
{
  NSLinkEditErrors class = 0;
  int error_number = 0;
  const char *file_name = NULL;
  const char *error_string = NULL;

  if (pike_dl_error) return pike_dl_error;

  NSLinkEditError(&class, &error_number, &file_name, &error_string);
  return error_string;
}

#endif /* USE_DYLD */


#ifndef EMULATE_DLOPEN

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#define dlinit()	1
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

#ifndef NO_PIKE_GUTS

#if defined(HAVE_DLOPEN) || defined(USE_DLD) || defined(USE_HPUX_DL) || \
    defined(USE_LOADLIBRARY) || defined(USE_DYLD)
#define USE_DYNAMIC_MODULES
#endif

#ifdef USE_DYNAMIC_MODULES

struct module_list
{
  struct module_list * next;
  void *module;
  struct pike_string *name;
  struct program *module_prog;
  modfun init, exit;
};

struct module_list *dynamic_module_list = 0;

#ifdef NO_CAST_TO_FUN
/* Function pointers can't be casted to scalar pointers according to
 * ISO-C (probably to support true Harward achitecture machines).
 */
static modfun CAST_TO_FUN(void *ptr)
{
  union {
    void *ptr;
    modfun fun;
  } u;
  u.ptr = ptr;
  return u.fun;
}
#else /* !NO_CAST_TO_FUN */
#define CAST_TO_FUN(X)	((modfun)X)
#endif /* NO_CAST_TO_FUN */

static void cleanup_compilation(void *UNUSED(ignored))
{
  struct program *p = end_program();
  if (p) {
    free_program(p);
  }
}

static DECLSPEC(noreturn)
  void module_load_error(const char *err,
                         struct pike_string *module_name,
                         INT32 args)
  ATTRIBUTE((noreturn));
static void module_load_error(const char *err, struct pike_string *module_name,
                              INT32 args)
{
  struct object *err_obj = fast_clone_object(module_load_error_program);
#define LOADERR_STRUCT(OBJ)                             \
  ((struct module_load_error_struct *)                  \
   (err_obj->storage + module_load_error_offset))

  struct pike_string *str;
  if (err) {
    if (err[strlen (err) - 1] == '\n')
      push_string (make_shared_binary_string (err, strlen (err) - 1));
    else
      push_text (err);
  }
  else
    push_static_text ("Unknown reason");

  add_ref (LOADERR_STRUCT (err_obj)->path = module_name);
  add_ref (LOADERR_STRUCT (err_obj)->reason = str = Pike_sp[-1].u.string);
  pop_stack();

  /* NB: str is still valid here as LOADER_STRUCT->reason holds a reference. */
  if (str->len < 1024) {
    throw_error_object (err_obj, "load_module", args,
                        "load_module(%pq) failed: %pS\n",
                        module_name, str);
  }

  throw_error_object (err_obj, "load_module", args,
                      "load_module(%pq) failed.\n",
                      module_name);
}

/*! @decl program load_module(string module_name)
 *!
 *! Load a binary module.
 *!
 *! This function loads a module written in C or some other language
 *! into Pike. The module is initialized and any programs or constants
 *! defined will immediately be available.
 *!
 *! When a module is loaded the C function @tt{pike_module_init()@} will
 *! be called to initialize it. When Pike exits @tt{pike_module_exit()@}
 *! will be called. These two functions @b{must@} be available in the module.
 *!
 *! @note
 *!   The current working directory is normally not searched for
 *!   dynamic modules. Please use @expr{"./name.so"@} instead of just
 *!   @expr{"name.so"@} to load modules from the current directory.
 */
void f_load_module(INT32 args)
{
  extern int global_callable_flags;

  void *module = NULL;
  modfun init, exit;
  struct module_list *new_module;
  struct pike_string *module_name;

  ONERROR err;
  DECLARE_CYCLIC();

  module_name = Pike_sp[-args].u.string;

  if((TYPEOF(Pike_sp[-args]) != T_STRING) ||
     (module_name->size_shift) ||
     string_has_null(module_name)) {
    Pike_error("Bad argument 1 to load_module()\n");
  }

  {
    struct module_list *mp;
    for (mp = dynamic_module_list; mp; mp = mp->next)
      if (mp->name == module_name && mp->module_prog) {
	pop_n_elems(args);
	ref_push_program(mp->module_prog);
	return;
      }
  }

  if (!BEGIN_CYCLIC(module_name, 0)) {
    SET_CYCLIC_RET(1);

    /* Removing RTLD_GLOBAL breaks some PiGTK themes - Hubbe */
    /* Using RTLD_LAZY is faster, but makes it impossible to
     * detect linking problems at runtime..
     */
    module=dlopen(module_name->str,
		  RTLD_NOW /*|RTLD_GLOBAL*/  );
  }

  if(!module) {
    module_load_error(dlerror(), module_name, args);
  }

#ifdef PIKE_DEBUG
  {
    struct module_list *mp;
    for (mp = dynamic_module_list; mp; mp = mp->next)
      if (mp->module == module && mp->module_prog) {
	fprintf(stderr, "load_module(): Module loaded twice:\n"
		"Old name: %s\n"
		"New name: %s\n",
		mp->name->str, module_name->str);
	pop_n_elems(args);
	ref_push_program(mp->module_prog);
	END_CYCLIC();
	return;
      }
  }
#endif /* PIKE_DEBUG */

  init = CAST_TO_FUN(dlsym(module, "pike_module_init"));
  if (!init) {
    init = CAST_TO_FUN(dlsym(module, "_pike_module_init"));
    if (!init) {
      dlclose(module);
      Pike_error("pike_module_init missing in dynamic module %pq.\n",
		 module_name);
    }
  }

  exit = CAST_TO_FUN(dlsym(module, "pike_module_exit"));
  if (!exit) {
    exit = CAST_TO_FUN(dlsym(module, "_pike_module_exit"));
    if (!exit) {
      dlclose(module);
      Pike_error("pike_module_exit missing in dynamic module %pq.\n",
		 module_name);
    }
  }

  new_module=ALLOC_STRUCT(module_list);
  new_module->next=dynamic_module_list;
  dynamic_module_list=new_module;
  new_module->module=module;
  copy_shared_string(new_module->name, Pike_sp[-args].u.string);
  new_module->module_prog = NULL;
  new_module->init=init;
  new_module->exit=exit;

  enter_compiler(new_module->name, 1);

  start_new_program();

  global_callable_flags|=CALLABLE_DYNAMIC;

#ifdef PIKE_DEBUG
  { struct svalue *save_sp=Pike_sp;
#endif
  SET_ONERROR(err, cleanup_compilation, NULL);
  (*(modfun)init)();
  UNSET_ONERROR(err);
#ifdef PIKE_DEBUG
  if(Pike_sp != save_sp)
    Pike_fatal("pike_module_init in %s left "
	       "%"PRINTPTRDIFFT"d droppings on stack.\n",
	       module_name->str, Pike_sp - save_sp);
  }
#endif

  pop_n_elems(args);
  {
    struct program *p = end_program();
    exit_compiler();
    if (p) {
      if (
#if 0
	  p->num_identifier_references
#else /* !0 */
	  1
#endif /* 0 */
	  ) {
	push_program(p);
	add_ref(new_module->module_prog = Pike_sp[-1].u.program);
      } else {
	/* No identifier references -- Disabled module. */
	free_program(p);
	push_undefined();
      }
    } else {
      /* Initialization failed. */
#ifdef PIKE_DEBUG
      struct svalue *save_sp=Pike_sp;
#endif
      new_module->exit();

      /* Clean up any objects that may have code residing in the module. */
      destruct_objects_to_destruct();

#ifdef PIKE_DEBUG
      if(Pike_sp != save_sp)
	Pike_fatal("pike_module_exit in %s left "
		   "%"PRINTPTRDIFFT"d droppings on stack.\n",
		   module_name->str, Pike_sp - save_sp);
#endif

      dlclose(module);
      dynamic_module_list = new_module->next;
      free_string(new_module->name);
      free(new_module);
      END_CYCLIC();

      module_load_error("Failed to initialize dynamic module.",
                        module_name, args);
    }
  }
  END_CYCLIC();
}

#endif /* USE_DYNAMIC_MODULES */


void init_dynamic_load(void)
{
#ifdef USE_DYNAMIC_MODULES
  if (dlinit()) {

    /* function(string:program) */

    ADD_EFUN("load_module", f_load_module,
	     tFunc(tStr,tPrg(tObj)), OPT_EXTERNAL_DEPEND);
  }
#endif
}

/* Call the pike_module_exit() callbacks for the dynamic modules. */
void exit_dynamic_load(void)
{
#ifdef USE_DYNAMIC_MODULES
  struct module_list * volatile tmp;
  JMP_BUF recovery;
  for (tmp = dynamic_module_list; tmp; tmp = tmp->next)
  {
    if(SETJMP(recovery))
      call_handle_error();
    else {
#ifdef PIKE_DEBUG
      struct svalue *save_sp=Pike_sp;
#endif
      tmp->exit();
#ifdef PIKE_DEBUG
      if(Pike_sp != save_sp)
	Pike_fatal("pike_module_exit in %s left "
		   "%"PRINTPTRDIFFT"d droppings on stack.\n",
		   tmp->name->str, Pike_sp - save_sp);
#endif
    }
    UNSETJMP(recovery);

    if (tmp->module_prog) {
      free_program(tmp->module_prog);
      tmp->module_prog = NULL;
    }
    free_string(tmp->name);
    tmp->name = NULL;
  }
#endif
}

/* Unload all the dynamically loaded modules. */
void free_dynamic_load(void)
{
#ifdef USE_DYNAMIC_MODULES
  while(dynamic_module_list)
  {
    struct module_list *tmp=dynamic_module_list;
    dynamic_module_list=tmp->next;
#ifndef DEBUG_MALLOC
    dlclose(tmp->module);
#endif
#ifdef PIKE_DEBUG
    if (tmp->module_prog)
      Pike_fatal ("There's still a program for a dynamic module.\n");
#endif
    free(tmp);
  }
#endif
}


#endif /* NO_PIKE_GUTS */

#ifdef CREATE_MAIN
#include <stdio.h>

int main()
{
  void *module,*fun;
  if (!dlinit()) {
    fprintf(stderr, "dlinit() failed.\n");
    exit(1);
  }
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
#endif /* CREATE_MAIN */
