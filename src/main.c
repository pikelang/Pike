/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: main.c,v 1.222 2005/01/01 17:35:54 grubba Exp $
*/

#include "global.h"
#include "fdlib.h"
#include "backend.h"
#include "module.h"
#include "object.h"
#include "lex.h"
#include "pike_types.h"
#include "builtin_functions.h"
#include "array.h"
#include "stralloc.h"
#include "interpret.h"
#include "pike_error.h"
#include "pike_macros.h"
#include "callback.h"
#include "signal_handler.h"
#include "threads.h"
#include "dynamic_load.h"
#include "gc.h"
#include "multiset.h"
#include "mapping.h"
#include "cpp.h"
#include "main.h"
#include "operators.h"
#include "rbtree.h"
#include "pike_security.h"
#include "constants.h"
#include "version.h"
#include "program.h"
#include "pike_rusage.h"
#include "module_support.h"
#include "opcodes.h"

#ifdef AUTO_BIGNUM
#include "bignum.h"
#endif

#include "pike_embed.h"

#if defined(__linux__) && defined(HAVE_DLOPEN) && defined(HAVE_DLFCN_H)
#include <dlfcn.h>
#endif

#include "las.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#include "time_stuff.h"

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

/* Define this to trace the execution of main(). */
/* #define TRACE_MAIN */

#ifdef TRACE_MAIN
#define TRACE(X)	fprintf X
#else /* !TRACE_MAIN */
#define TRACE(X)
#endif /* TRACE_MAIN */

/*
 * Code searching for master & libpike.
 */

#define MASTER_COOKIE1 "(#*&)@(*&$"
#define MASTER_COOKIE2 "Master Cookie:"

#define MASTER_COOKIE MASTER_COOKIE1 MASTER_COOKIE2

#ifndef MAXPATHLEN
#define MAXPATHLEN 32768
#endif

static char master_location[MAXPATHLEN * 2] = MASTER_COOKIE;

static void set_master(const char *file)
{
  if (strlen(file) >= MAXPATHLEN*2 - CONSTANT_STRLEN(MASTER_COOKIE)) {
    fprintf(stderr, "Too long path to master: \"%s\" (limit:%d)\n",
	    file, MAXPATHLEN*2 - CONSTANT_STRLEN(MASTER_COOKIE));
    exit(1);
  }
  strcpy(master_location + CONSTANT_STRLEN(MASTER_COOKIE), file);
}

#ifdef __NT__
static void get_master_key(HKEY cat)
{
  HKEY k;
  char buffer[4096];
  DWORD len=sizeof(buffer)-1,type=REG_SZ;

  if(RegOpenKeyEx(cat,
		  (LPCTSTR)("SOFTWARE\\Pike\\"
			    DEFINETOSTR(PIKE_MAJOR_VERSION)
			    "."
			    DEFINETOSTR(PIKE_MINOR_VERSION)
			    "."
			    DEFINETOSTR(PIKE_BUILD_VERSION)),
		  0,KEY_READ,&k)==ERROR_SUCCESS)
  {
    if(RegQueryValueEx(k,
		       "PIKE_MASTER",
		       0,
		       &type,
		       buffer,
		       &len)==ERROR_SUCCESS)
    {
      /* FIXME: Look at len? UNICODE? */
      set_master(buffer);
    }
    RegCloseKey(k);
  }
}
#endif /* __NT__ */

static void set_default_master(void)
{
#ifdef HAVE_GETENV
  if(!master_location[CONSTANT_STRLEN(MASTER_COOKIE)] &&
     getenv("PIKE_MASTER")) {
    set_master(getenv("PIKE_MASTER"));
  }
#endif

#ifdef __NT__
  if(!master_location[CONSTANT_STRLEN(MASTER_COOKIE)])
    get_master_key(HKEY_CURRENT_USER);
  if(!master_location[CONSTANT_STRLEN(MASTER_COOKIE)])
    get_master_key(HKEY_LOCAL_MACHINE);
#endif

  if(!master_location[CONSTANT_STRLEN(MASTER_COOKIE)])
  {
    sprintf(master_location + CONSTANT_STRLEN(MASTER_COOKIE),
	    DEFAULT_MASTER,
	    PIKE_MAJOR_VERSION,
	    PIKE_MINOR_VERSION,
	    PIKE_BUILD_VERSION);
  }

  TRACE((stderr, "Default master at \"%s\"...\n",
	 master_location + CONSTANT_STRLEN(MASTER_COOKIE)));
}

#ifdef LIBPIKE
static char libpike_file[MAXPATHLEN * 2];
static void *libpike;

static void (*init_pike)(const char **argv, const char *file);
static void (*init_pike_runtime)(void (*exit_cb)(int));
#endif /* LIBPIKE */

static void find_lib_dir(int argc, char **argv)
{
  int e;
  char *p;
  char *dir;

  TRACE((stderr, "find_lib_dir...\n"));
  
  set_default_master();

  for(e=1; e<argc; e++)
  {
    TRACE((stderr, "Parse argument %d:\"%s\"...\n", e, argv[e]));
  
    if(argv[e][0] != '-') break;

    switch(argv[e][1])
    {
    default:
      break;
	  
    case 'm':
      if(argv[e][2])
      {
	set_master(argv[e]+2);
      }else{
	e++;
	if(e >= argc)
	{
	  fprintf(stderr,"Missing argument to -m\n");
	  exit(1);
	}
	set_master(argv[e]);
      }
      break;

    case 's':
      if((!argv[e][2]) ||
	 ((argv[e][2] == 's') && !argv[e][3])) {
	e++;
      }
      break;

    case 'q':
      if(!argv[e][2]) e++;
      break;
    }
  }
#ifdef LIBPIKE
  memcpy(libpike_file, master_location + CONSTANT_STRLEN(MASTER_COOKIE),
	 sizeof(master_location) - CONSTANT_STRLEN(MASTER_COOKIE));
  for (p = dir = libpike_file; *p; p++) {
    if ((*p == '/')
#ifdef __NT__
	|| (*p == '\\')
#endif /* __NT__ */
	)
      dir = p+1;
  }
  if ((p + CONSTANT_STRLEN("pike.so")) >= libpike_file + 2*MAXPATHLEN) {
    /* Not likely to happen as long as MASTER_COOKIE is longer than "pike.so".
     */
    fprintf(stderr, "Too long path to pike.so.\n");
    exit(1);
  }
  /* Don't forget the NUL! */
  memcpy(p, "pike.so", CONSTANT_STRLEN("pike.so") + 1);
#endif /* LIBPIKE */
}

int main(int argc, char **argv)
{
  JMP_BUF back;
  int e, num;
  char *p;
  struct array *a;

  TRACE((stderr, "Init master...\n"));
  
  find_lib_dir(argc, argv);

#ifdef LIBPIKE
  if (!dlinit()) {
    fprintf(stderr, "dlinit failed.\n");
    exit(1);
  }

  if (!(libpike = dlopen(libpike_name, RTLD_NOW))) {
    const char *err = dlerror();
    if (!err) err = "Unknown reason.";
    fprintf(stderr, "Failed to open %s: %s\n", libpike_name, err);
    exit(1);
  }

#define LOOKUP(symbol) do {						\
    if (!(symbol = CAST_TO_FUN(dlsym(libpike, TOSTR(symbol)))) &&	\
	!(symbol = CAST_TO_FUN(dlsym(libpike, "_" TOSTR(symbol))))) {	\
      fprintf(stderr, "Missing symbol in %s: " TOSTR(symbol) "\n",	\
	      libpike_name);						\
      dlclose(libpike);							\
      exit(1);								\
    }									\
  } while(0)

  LOOKUP(init_pike);
  LOOKUP(init_pike_runtime);
  LOOKUP(add_predefine);
  
#endif /* LIBPIKE */

  TRACE((stderr, "init_pike()\n"));

  init_pike(argv, master_location + CONSTANT_STRLEN(MASTER_COOKIE));

  for(e=1; e<argc; e++)
  {
    TRACE((stderr, "Parse argument %d:\"%s\"...\n", e, argv[e]));
  
    if(argv[e][0]=='-')
    {
      for(p=argv[e]+1; *p;)
      {
	switch(*p)
	{
	case 'D':
	  add_predefine(p+1);
	  p+=strlen(p);
	  break;
	  
	case 'm':
	  if(p[1])
	  {
	    p+=strlen(p);
	  }else{
	    e++;
	    if(e >= argc)
	    {
	      fprintf(stderr,"Missing argument to -m\n");
	      exit(1);
	    }
	    p+=strlen(p);
	  }
	  break;

	case 's':
	  if(!p[1])
	  {
	    e++;
	    if(e >= argc)
	    {
	      fprintf(stderr,"Missing argument to -s\n");
	      exit(1);
	    }
	    p=argv[e];
	  }else{
	    p++;
	    if(*p=='s')
	    {
	      if(!p[1])
	      {
		e++;
		if(e >= argc)
		{
		  fprintf(stderr,"Missing argument to -ss\n");
		  exit(1);
		}
		p=argv[e];
	      }else{
		p++;
	      }
#ifdef _REENTRANT
	      thread_stack_size=STRTOL(p,&p,0);
#endif
	      p+=strlen(p);
	      break;
	    }
	  }
	  Pike_stack_size=STRTOL(p,&p,0);
	  p+=strlen(p);

	  if(Pike_stack_size < 256)
	  {
	    fprintf(stderr,"Stack size must at least be 256.\n");
	    exit(1);
	  }
	  break;

	case 'q':
	  if(!p[1])
	  {
	    e++;
	    if(e >= argc)
	    {
	      fprintf(stderr,"Missing argument to -q\n");
	      exit(1);
	    }
	    p=argv[e];
	  }else{
	    p++;
	  }
	  set_pike_evaluator_limit(STRTOL(p, &p, 0));
	  p+=strlen(p);
	  break;

	case 'd':
	more_d_flags:
	  switch(p[1])
	  {
	    case '0': case '1': case '2': case '3': case '4':
	    case '5': case '6': case '7': case '8': case '9':
	      d_flag+=STRTOL(p+1,&p,10);
	      break;

	    case 'c':
	      p++;
#if defined(YYDEBUG) || defined(PIKE_DEBUG)
	      yydebug++;
#endif /* YYDEBUG || PIKE_DEBUG */
	      break;

	    case 's':
	      set_pike_debug_options(DEBUG_SIGNALS, DEBUG_SIGNALS);
	      p++;
	      goto more_d_flags;

	    case 't':
	      set_pike_debug_options(NO_TAILRECURSION, NO_TAILRECURSION);
	      p++;
	      goto more_d_flags;

	    case 'g':
	      set_pike_debug_options(GC_RESET_DMALLOC, GC_RESET_DMALLOC);
	      p++;
	      goto more_d_flags;

	    case 'p':
	      set_pike_debug_options(NO_PEEP_OPTIMIZING, NO_PEEP_OPTIMIZING);
	      p++;
	      goto more_d_flags;

	    case 'T':
	      set_pike_debug_options(ERRORCHECK_MUTEXES, ERRORCHECK_MUTEXES);
	      p++;
	      goto more_d_flags;

	    default:
	      d_flag += (p[0] == 'd');
	      p++;
	  }
	  break;

	case 'r':
	more_r_flags:
	  switch(p[1]) 
          {
	  case 't':
	    set_pike_runtime_options(RUNTIME_CHECK_TYPES, RUNTIME_CHECK_TYPES);
	    p++;
	    goto more_r_flags;

	  case 'T':
	    set_pike_runtime_options(RUNTIME_STRICT_TYPES,
				     RUNTIME_STRICT_TYPES);
	    p++;
	    goto more_r_flags;

         default:
            p++;
	    break;
	  }
	  break;

	case 'a':
	  if(p[1]>='0' && p[1]<='9')
	    a_flag+=STRTOL(p+1,&p,10);
	  else
	    a_flag++,p++;
	  break;

	case 't':
	  more_t_flags:
	  switch (p[1]) {
	    case '0': case '1': case '2': case '3': case '4':
	    case '5': case '6': case '7': case '8': case '9':
	      Pike_interpreter.trace_level+=STRTOL(p+1,&p,10);
	      break;

	    case 'g':
	      gc_trace++;
	      p++;
	      goto more_t_flags;

	    default:
	      if (p[0] == 't')
		Pike_interpreter.trace_level++;
	      p++;
	  }
	  default_t_flag = Pike_interpreter.trace_level;
	  break;

	case 'p':
	  if(p[1]=='s')
	  {
	    pike_enable_stack_profiling();
	    p+=strlen(p);
	  }else{
	    if(p[1]>='0' && p[1]<='9')
	      p_flag+=STRTOL(p+1,&p,10);
	    else
	      p_flag++,p++;
	  }
	  break;

	case 'l':
	  if(p[1]>='0' && p[1]<='9')
	    l_flag+=STRTOL(p+1,&p,10);
	  else
	    l_flag++,p++;
	  break;

	default:
	  p+=strlen(p);
	}
      }
    }else{
      break;
    }
  }

#ifndef PIKE_MUTEX_ERRORCHECK
  if (set_pike_debug_options(0,0) & ERRORCHECK_MUTEXES)
    fputs ("Warning: -dT (error checking mutexes) not supported on this system.\n",
	   stderr);
#endif
  if (d_flag)
    set_pike_debug_options(ERRORCHECK_MUTEXES, ERRORCHECK_MUTEXES);

  init_pike_runtime(exit);

  /* NOTE: Reuse master_location here to avoid duplicates of
   *       the MASTER_COOKIE string in the binary.
   */
  add_pike_string_constant("__master_cookie",
			   master_location, CONSTANT_STRLEN(MASTER_COOKIE));

  if(SETJMP(back))
  {
    if(throw_severity == THROW_EXIT)
    {
      num=throw_value.u.integer;
    }else{
      if (throw_value.type == T_OBJECT &&
	  throw_value.u.object->prog == master_load_error_program &&
	  !get_master()) {
	/* Report this specific error in a nice way. Since there's no
	 * master it'd be reported with a raw error dump otherwise. */
	struct generic_error_struct *err;

	dynamic_buffer buf;
	dynbuf_string s;
	struct svalue t;

	*(Pike_sp++) = throw_value;
	dmalloc_touch_svalue(Pike_sp-1);
	throw_value.type=T_INT;
	err = (struct generic_error_struct *)
	  get_storage (Pike_sp[-1].u.object, generic_error_program);

	t.type = PIKE_T_STRING;
	t.u.string = err->error_message;

	init_buf(&buf);
	describe_svalue(&t,0,0);
	s=complex_free_buf(&buf);

	fputs(s.str, stderr);
	free(s.str);
      }
      else
	call_handle_error();
      num=10;
    }
  }else{
    struct object *m;

    back.severity=THROW_EXIT;

    if ((m = load_pike_master())) {
      TRACE((stderr, "Call master->_main()...\n"));

      pike_push_argv(argc, argv);

      pike_push_env();
  
      apply(m, "_main", 2);
      pop_stack();
      num=0;
    } else {
      num = -1;
    }
  }
  UNSETJMP(back);

  TRACE((stderr, "Exit %d...\n", num));

  pike_do_exit(num);
  return num; /* avoid warning */
}
