/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: main.c,v 1.195 2004/03/16 18:33:45 mast Exp $
*/

#include "global.h"
RCSID("$Id: main.c,v 1.195 2004/03/16 18:33:45 mast Exp $");
#include "fdlib.h"
#include "backend.h"
#include "module.h"
#include "object.h"
#include "language.h"
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
#include "security.h"
#include "constants.h"
#include "version.h"
#include "program.h"
#include "pike_rusage.h"
#include "module_support.h"
#include "opcodes.h"

#ifdef AUTO_BIGNUM
#include "bignum.h"
#endif

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

#ifdef TRY_USE_MMX
#ifdef HAVE_MMX_H
#include <mmx.h>
#else
#include <asm/mmx.h>
#endif
int try_use_mmx;
#endif

/* Define this to trace the execution of main(). */
/* #define TRACE_MAIN */

#ifdef TRACE_MAIN
#define TRACE(X)	fprintf X
#else /* !TRACE_MAIN */
#define TRACE(X)
#endif /* TRACE_MAIN */

char *master_file;
char **ARGV;

PMOD_EXPORT int debug_options=0;
PMOD_EXPORT int runtime_options=0;
PMOD_EXPORT int d_flag=0;
PMOD_EXPORT int c_flag=0;
PMOD_EXPORT int default_t_flag=0;
PMOD_EXPORT int a_flag=0;
PMOD_EXPORT int l_flag=0;
PMOD_EXPORT int p_flag=0;
#if defined(YYDEBUG) || defined(PIKE_DEBUG)
extern int yydebug;
#endif /* YYDEBUG || PIKE_DEBUG */
static long instructions_left;

#define MASTER_COOKIE "(#*&)@(*&$Master Cookie:"

#ifndef MAXPATHLEN
#define MAXPATHLEN 32768
#endif

char master_location[MAXPATHLEN * 2] = MASTER_COOKIE;

static void time_to_exit(struct callback *cb,void *tmp,void *ignored)
{
  if(instructions_left-- < 0)
  {
    push_int(0);
    f_exit(1);
  }
}

#ifdef PROFILING
static unsigned int samples[8200];
long record;

static void sample_stack(struct callback *cb,void *tmp,void *ignored)
{
  long stack_size=( ((char *)&cb) - Pike_interpreter.stack_bottom) * STACK_DIRECTION;
  stack_size>>=10;
  stack_size++;
  if(stack_size<0) stack_size=0;
  if(stack_size >= (long)NELEM(samples)) stack_size=NELEM(samples)-1;
  samples[stack_size]++;
#ifdef PIKE_DEBUG
  if(stack_size > record)
  {
    extern void gdb_break_on_stack_record(long);
    gdb_break_on_stack_record(stack_size);
    record=stack_size;
  }
#endif
}

#ifdef PIKE_DEBUG
void gdb_break_on_stack_record(long stack_size)
{
  ;
}
#endif
#endif

static struct callback_list post_master_callbacks;

PMOD_EXPORT struct callback *add_post_master_callback(callback_func call,
					  void *arg,
					  callback_func free_func)
{
  return add_to_callback(&post_master_callbacks, call, arg, free_func);
}


static struct callback_list exit_callbacks;

PMOD_EXPORT struct callback *add_exit_callback(callback_func call,
				   void *arg,
				   callback_func free_func)
{
  return add_to_callback(&exit_callbacks, call, arg, free_func);
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
			    DEFINETOSTR(PIKE_MINOR_VERSION)),
		  0,KEY_READ,&k)==ERROR_SUCCESS)
  {
    if(RegQueryValueEx(k,
		       "PIKE_MASTER",
		       0,
		       &type,
		       buffer,
		       &len)==ERROR_SUCCESS)
    {
      dmalloc_accept_leak( master_file=strdup(buffer) );
    }
    RegCloseKey(k);
  }
}
#endif /* __NT__ */

#ifdef __amigaos4__
#define timeval timeval_amigaos
#include <exec/types.h>
#include <utility/hooks.h>
#include <dos/dosextens.h>
#include <proto/dos.h>

static SAVEDS LONG scan_amigaos_environment_func(struct Hook *REG(a0,hook),
						 APTR REG(a2,userdata),
						 struct ScanVarsMsg *REG(a1,msg))
{
  if(msg->sv_GDir[0] == '\0' ||
     !strcmp(msg->sv_GDir, "ENV:")) {
    push_text(msg->sv_Name);
    push_constant_text("=");
    push_string(make_shared_binary_string(msg->sv_Var, msg->sv_VarLen));
    f_add(3);
  }

  return 0;
}

static struct Hook scan_amigaos_environment_hook = {
  { NULL, NULL },
  (ULONG (*)())scan_amigaos_environment_func,
  NULL, NULL
};
#endif /* __amigsos4__ */

int dbm_main(int argc, char **argv)
{
  JMP_BUF back;
  int e, num;
  char *p;
  struct array *a;
#ifdef DECLARE_ENVIRON
  extern char **environ;
#endif

  TRACE((stderr, "dbm_main()\n"));

  init_rusage();

  /* Attempt to make sure stderr is unbuffered. */
#ifdef HAVE_SETVBUF
  setvbuf(stderr, NULL, _IONBF, 0);
#else /* !HAVE_SETVBUF */
#ifdef HAVE_SETBUF
  setbuf(stderr, NULL);
#endif /* HAVE_SETBUF */
#endif /* HAVE_SETVBUF */

  TRACE((stderr, "Init CPU lib...\n"));
  
  init_pike_cpulib();

#ifdef TRY_USE_MMX
  TRACE((stderr, "Init MMX...\n"));
  
  try_use_mmx=mmx_ok();
#endif
#ifdef OWN_GETHRTIME
/* initialize our own gethrtime conversion /Mirar */
  TRACE((stderr, "Init gethrtime...\n"));
  
  own_gethrtime_init();
#endif

  ARGV=argv;

  TRACE((stderr, "Main init...\n"));
  
  fd_init();
  {
    extern void init_mapping_blocks(void);
    extern void init_callable_blocks(void);
    extern void init_gc_frame_blocks(void);
    extern void init_pike_frame_blocks(void);
    extern void init_node_s_blocks(void);
    extern void init_object_blocks(void);
    extern void init_callback_blocks(void);

    init_mapping_blocks();
    init_callable_blocks();
    init_gc_frame_blocks();
    init_pike_frame_blocks();
    init_node_s_blocks();
    init_object_blocks();
#ifndef DEBUG_MALLOC
    /* This has already been done by initialize_dmalloc(). */
    init_callback_blocks();
#endif /* !DEBUG_MALLOC */
#ifdef PIKE_NEW_MULTISETS
    init_multiset();
#endif
    init_builtin_constants();
  }

#ifdef SHARED_NODES
  TRACE((stderr, "Init shared nodes...\n"));
  
  node_hash.table = malloc(sizeof(node *)*32831);
  if (!node_hash.table) {
    Pike_fatal("Out of memory!\n");
  }
  MEMSET(node_hash.table, 0, sizeof(node *)*32831);
  node_hash.size = 32831;
#endif /* SHARED_NODES */

#ifdef HAVE_TZSET
  tzset();
#endif /* HAVE_TZSET */

#ifdef HAVE_SETLOCALE
#ifdef LC_NUMERIC
  setlocale(LC_NUMERIC, "C");
#endif
#ifdef LC_CTYPE
  setlocale(LC_CTYPE, "");
#endif
#ifdef LC_TIME
  setlocale(LC_TIME, "C");
#endif
#ifdef LC_COLLATE
  setlocale(LC_COLLATE, "");
#endif
#ifdef LC_MESSAGES
  setlocale(LC_MESSAGES, "");
#endif
#endif  

  TRACE((stderr, "Init master...\n"));
  
  master_file = 0;

#ifdef HAVE_GETENV
  if(getenv("PIKE_MASTER"))
    master_file = getenv("PIKE_MASTER");
#endif

  if(master_location[CONSTANT_STRLEN(MASTER_COOKIE)])
    master_file=master_location + CONSTANT_STRLEN(MASTER_COOKIE);

#if __NT__
  if(!master_file) get_master_key(HKEY_CURRENT_USER);
  if(!master_file) get_master_key(HKEY_LOCAL_MACHINE);
#endif

  if(!master_file)
  {
    sprintf(master_location,DEFAULT_MASTER,
	    PIKE_MAJOR_VERSION,
	    PIKE_MINOR_VERSION,
	    PIKE_BUILD_VERSION);
    master_file=master_location;
  }

  TRACE((stderr, "Default master at \"%s\"...\n", master_file));

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
	    master_file=p+1;
	    p+=strlen(p);
	  }else{
	    e++;
	    if(e >= argc)
	    {
	      fprintf(stderr,"Missing argument to -m\n");
	      exit(1);
	    }
	    master_file=argv[e];
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
	  instructions_left=STRTOL(p,&p,0);
	  p+=strlen(p);
	  add_to_callback(&evaluator_callbacks,
			  time_to_exit,
			  0,0);
	  
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
	      debug_options|=DEBUG_SIGNALS;
	      p++;
	      goto more_d_flags;

	    case 't':
	      debug_options|=NO_TAILRECURSION;
	      p++;
	      goto more_d_flags;

#ifdef DEBUG_MALLOC
	    case 'g':
	      debug_options|=GC_RESET_DMALLOC;
	      p++;
	      goto more_d_flags;
#endif

	    case 'p':
	      debug_options|=NO_PEEP_OPTIMIZING;
	      p++;
	      goto more_d_flags;

	    case 'T':
	      debug_options |= ERRORCHECK_MUTEXES;
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
	    runtime_options |= RUNTIME_CHECK_TYPES;
	    p++;
	    goto more_r_flags;

	  case 'T':
	    runtime_options |= RUNTIME_STRICT_TYPES;
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
	  if(p[1]>='0' && p[1]<='9')
	    Pike_interpreter.trace_level+=STRTOL(p+1,&p,10);
	  else
	    Pike_interpreter.trace_level++,p++;
	  default_t_flag = Pike_interpreter.trace_level;
	  break;

	case 'p':
	  if(p[1]=='s')
	  {
#ifdef PROFILING
	    add_to_callback(&evaluator_callbacks,
			    sample_stack,
			    0,0);
	    p+=strlen(p);
#endif	    
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
  if (debug_options & ERRORCHECK_MUTEXES)
    fputs ("Warning: -dT (error checking mutexes) not supported on this system.\n",
	   stderr);
#endif
  if (d_flag) debug_options |= ERRORCHECK_MUTEXES;

#if !defined(RLIMIT_NOFILE) && defined(RLIMIT_OFILE)
#define RLIMIT_NOFILE RLIMIT_OFILE
#endif

  TRACE((stderr, "Init C stack...\n"));
  
  Pike_interpreter.stack_top = (char *)&argv;

  /* Adjust for anything already pushed on the stack.
   * We align on a 64 KB boundary.
   * Thus we at worst, lose 64 KB stack.
   *
   * We have to do it this way since some compilers don't like
   * & and | on pointers, and casting to an integer type is
   * too unsafe (consider 64-bit systems).
   */
#if STACK_DIRECTION < 0
  /* Equvivalent with |= 0xffff */
  Pike_interpreter.stack_top += ~(PTR_TO_INT(Pike_interpreter.stack_top)) & 0xffff;
#else /* STACK_DIRECTION >= 0 */
  /* Equvivalent with &= ~0xffff */
  Pike_interpreter.stack_top -= PTR_TO_INT(Pike_interpreter.stack_top) & 0xffff;
#endif /* STACK_DIRECTION < 0 */

#ifdef PROFILING
  Pike_interpreter.stack_bottom=Pike_interpreter.stack_top;
#endif

#if defined(HAVE_GETRLIMIT) && defined(RLIMIT_STACK)
  {
    struct rlimit lim;
    if(!getrlimit(RLIMIT_STACK, &lim))
    {
#ifdef RLIM_INFINITY
      if(lim.rlim_cur == RLIM_INFINITY)
	lim.rlim_cur=1024*1024*32;
#endif

#ifdef Pike_INITIAL_STACK_SIZE
      if(lim.rlim_cur > Pike_INITIAL_STACK_SIZE)
	lim.rlim_cur=Pike_INITIAL_STACK_SIZE;
#endif

#if defined(__linux__) && defined(PIKE_THREADS)
      /* This is a really really *stupid* limit in glibc 2.x
       * which is not detectable since __pthread_initial_thread_bos
       * went static. On a stupidity-scale from 1-10, this rates a
       * solid 11. - Hubbe
       */
      if(lim.rlim_cur > 2*1024*1024) lim.rlim_cur=2*1024*1024;
#endif

#if defined(_AIX) && defined(__ia64)
      /* getrlimit() on AIX 5L/IA64 Beta 3 reports 32MB by default,
       * even though the stack is just 8MB.
       */
      if (lim.rlim_cur > 8*1024*1024) {
        lim.rlim_cur = 8*1024*1024;
      }
#endif /* _AIX && __ia64 */

#if STACK_DIRECTION < 0
      Pike_interpreter.stack_top -= lim.rlim_cur;
#else /* STACK_DIRECTION >= 0 */
      Pike_interpreter.stack_top += lim.rlim_cur;
#endif /* STACK_DIRECTION < 0 */

#if defined(__linux__) && defined(HAVE_DLOPEN) && defined(HAVE_DLFCN_H) && !defined(PPC)
      {
	char ** bos_location;
	void *handle;
	/* damn this is ugly -Hubbe */
	if((handle=dlopen(0, RTLD_LAZY)))
	{
	  bos_location=dlsym(handle,"__pthread_initial_thread_bos");

	  if(bos_location && *bos_location &&
	     (*bos_location - Pike_interpreter.stack_top) *STACK_DIRECTION < 0)
	  {
	    Pike_interpreter.stack_top=*bos_location;
	  }

	  dlclose(handle);
	}
      }
#else
#ifdef HAVE_PTHREAD_INITIAL_THREAD_BOS
      {
	extern char * __pthread_initial_thread_bos;
	/* Linux glibc threads are limited to a 4 Mb stack
	 * __pthread_initial_thread_bos is the actual limit
	 */
	
	if(__pthread_initial_thread_bos && 
	   (__pthread_initial_thread_bos - Pike_interpreter.stack_top) *STACK_DIRECTION < 0)
	{
	  Pike_interpreter.stack_top=__pthread_initial_thread_bos;
	}
      }
#endif /* HAVE_PTHREAD_INITIAL_THREAD_BOS */
#endif /* __linux__ && HAVE_DLOPEN && HAVE_DLFCN_H && !PPC*/

#if STACK_DIRECTION < 0
      Pike_interpreter.stack_top += 8192 * sizeof(char *);
#else /* STACK_DIRECTION >= 0 */
      Pike_interpreter.stack_top -= 8192 * sizeof(char *);
#endif /* STACK_DIRECTION < 0 */


#ifdef STACK_DEBUG
      fprintf(stderr, "1: C-stack: 0x%08p - 0x%08p, direction:%d\n",
	      &argv, Pike_interpreter.stack_top, STACK_DIRECTION);
#endif /* STACK_DEBUG */
    }
  }
#else /* !HAVE_GETRLIMIT || !RLIMIT_STACK */
  /* 128 MB seems a bit extreme, most OS's seem to have their limit at ~8MB */
  Pike_interpreter.stack_top += STACK_DIRECTION * (1024*1024 * 8 - 8192 * sizeof(char *));
#ifdef STACK_DEBUG
  fprintf(stderr, "2: C-stack: 0x%08p - 0x%08p, direction:%d\n",
	  &argv, Pike_interpreter.stack_top, STACK_DIRECTION);
#endif /* STACK_DEBUG */
#endif /* HAVE_GETRLIMIT && RLIMIT_STACK */

#if 0
#if defined(HAVE_SETRLIMIT) && defined(RLIMIT_NOFILE)
  {
    struct rlimit lim;
    long tmp;
    if(!getrlimit(RLIMIT_NOFILE, &lim))
    {
#ifdef RLIM_INFINITY
      if(lim.rlim_max == RLIM_INFINITY)
	lim.rlim_max=MAX_OPEN_FILEDESCRIPTORS;
#endif
      tmp=MINIMUM(lim.rlim_max, MAX_OPEN_FILEDESCRIPTORS);
      lim.rlim_cur=tmp;
      setrlimit(RLIMIT_NOFILE, &lim);
    }
  }
#endif
#endif
  
  TRACE((stderr, "Init time...\n"));
  
  GETTIMEOFDAY(&current_time);

  TRACE((stderr, "Init threads...\n"));

  low_th_init();

  TRACE((stderr, "Init strings...\n"));
  
  init_shared_string_table();

  TRACE((stderr, "Init interpreter...\n"));

  init_interpreter();

  TRACE((stderr, "Init types...\n"));

  init_types();

  TRACE((stderr, "Init opcodes...\n"));

  init_opcodes();

  TRACE((stderr, "Init programs...\n"));

  init_program();

  TRACE((stderr, "Init objects...\n"));

  init_object();

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
	*(Pike_sp++) = throw_value;
	dmalloc_touch_svalue(Pike_sp-1);
	throw_value.type=T_INT;
	err = (struct generic_error_struct *)
	  get_storage (Pike_sp[-1].u.object, generic_error_program);
	ref_push_string (err->error_message);
	f_werror (1);
	pop_stack();
      }
      else
	call_handle_error();
      num=10;
    }
  }else{
    back.severity=THROW_EXIT;

    TRACE((stderr, "Init modules...\n"));

    init_modules();

#ifdef TEST_MULTISET
    /* A C-level testsuite for the low level stuff in multisets. */
    test_multiset();
#endif

    TRACE((stderr, "Init master...\n"));

    master();
    call_callback(& post_master_callbacks, 0);
    free_callback_list(& post_master_callbacks);

    TRACE((stderr, "Call master->_main()...\n"));

    a=allocate_array_no_init(argc,0);
    for(num=0;num<argc;num++)
    {
      ITEM(a)[num].u.string=make_shared_string(argv[num]);
      ITEM(a)[num].type=T_STRING;
    }
    a->type_field = BIT_STRING;
    push_array(a);
    
#ifdef __amigaos__
#ifdef __amigaos4__
    if(DOSBase->dl_lib.lib_Version >= 50) {
      struct svalue *mark = Pike_sp;
      IDOS->ScanVars(&scan_amigaos_environment_hook,
		     GVF_BINARY_VAR|GVF_DONT_NULL_TERM,
		     NULL);
      f_aggregate(Pike_sp-mark);
    } else
#endif
      push_array(allocate_array_no_init(0,0));
#else
    for(num=0;environ[num];num++);
    a=allocate_array_no_init(num,0);
    for(num=0;environ[num];num++)
    {
      ITEM(a)[num].u.string=make_shared_string(environ[num]);
      ITEM(a)[num].type=T_STRING;
    }
    a->type_field = BIT_STRING;
    push_array(a);
#endif
  
    apply(master(),"_main",2);
    pop_stack();
    num=0;
  }
  UNSETJMP(back);

  TRACE((stderr, "Exit %d...\n", num));
  
  pike_do_exit(num);
  return num; /* avoid warning */
}

#undef ATTRIBUTE
#define ATTRIBUTE(X)

DECLSPEC(noreturn) void pike_do_exit(int num) ATTRIBUTE((noreturn))
{
  call_callback(&exit_callbacks, NULL);
  free_callback_list(&exit_callbacks);

  exit_modules();

#ifdef DEBUG_MALLOC
  cleanup_memhdrs();
  cleanup_debug_malloc();
#endif


#ifdef PROFILING
  {
    int q;
    for(q=0;q<(long)NELEM(samples);q++)
      if(samples[q])
	fprintf(stderr,"STACK WAS %4d Kb %12u times\n",q-1,samples[q]);
  }
#endif

#ifdef PIKE_DEBUG
  /* For profiling */
  exit_opcodes();
#endif

#ifdef INTERNAL_PROFILING
  fprintf (stderr, "Evaluator callback calls: %lu\n", evaluator_callback_calls);
#ifdef PIKE_THREADS
  fprintf (stderr, "Thread yields: %lu\n", thread_yields);
#endif
  fprintf (stderr, "Main thread summary:\n");
  debug_print_rusage (stderr);
#endif

  exit(num);
}


void low_init_main(void)
{
  void init_iterators(void);

  init_cpp();
  init_backend();
  init_iterators();
  init_pike_searching();
  init_error();
  init_pike_security();
  th_init();
  init_operators();

  init_builtin();

  init_builtin_efuns();
  init_signals();
  init_dynamic_load();
}

void exit_main(void)
{
#ifdef DO_PIKE_CLEANUP
  size_t count;

#ifdef PIKE_DEBUG
  if (verbose_debug_exit)
    fprintf(stderr,"Exited normally, counting bytes.\n");
#endif

  /* Destruct all remaining objects while we have a proper execution
   * environment. The downside is that the leak report below will
   * always report destructed objects. We use the gc in a special mode
   * for this to get a reasonably sane destruct order. */
  gc_destruct_everything = 1;
  count = do_gc (NULL, 1);
  while (count) {
    size_t new_count = do_gc (NULL, 1);
    if (new_count >= count) {
      fprintf (stderr, "Some destroy function is creating new objects "
	       "during final cleanup - can't exit cleanly.\n");
      break;
    }
    count = new_count;
  }
  gc_destruct_everything = 0;

  /* Unload dynamic modules before static ones. */
  exit_dynamic_load();
#endif
}

void init_main(void)
{
}

void low_exit_main(void)
{
#ifdef DO_PIKE_CLEANUP
  void exit_iterators(void);

  /* Clear various global references. */

#ifdef AUTO_BIGNUM
  exit_auto_bignum();
#endif
  exit_pike_searching();
  exit_object();
  exit_signals();
  exit_builtin();
  exit_cpp();
  cleanup_interpret();
  exit_builtin_constants();
  cleanup_module_support();
  exit_operators();
  exit_iterators();
  cleanup_program();
  cleanup_compiler();
  cleanup_error();
  exit_backend();
  cleanup_gc();
  cleanup_pike_types();

  /* This zaps Pike_interpreter.thread_state among other things, so
   * THREADS_ALLOW/DISALLOW don't work beyond this point. */
  th_cleanup();

#ifdef SHARED_NODES
  free(node_hash.table);
#endif /* SHARED_NODES */

  exit_pike_security();
  free_svalue(& throw_value);
  throw_value.type=T_INT;

  do_gc(NULL, 1);

#ifdef PIKE_DEBUG
  if(verbose_debug_exit)
  {
#ifdef _REENTRANT
    if(count_pike_threads()>1)
    {
      fprintf(stderr,"Byte counting aborted, because all threads have not exited properly.\n");
      verbose_debug_exit=0;
      return;
    }
#endif

#ifdef DEBUG_MALLOC
    search_all_memheaders_for_references();
#endif

    /* The use of markers below only works after a gc run where it
     * hasn't freed anything. Since we've destructed all objects in
     * exit_main, nothing should be left after the run above, so only
     * one more run is necessary. */
    gc_keep_markers = 1;
    do_gc (NULL, 1);

#define STATIC_ARRAYS {&empty_array, &weak_empty_array, &weak_shrink_empty_array}

#define REPORT_LINKED_LIST_LEAKS(TYPE, START, STATICS, T_TYPE, NAME) do { \
      size_t num = 0;							\
      struct TYPE *x;							\
      for (x = START; x; x = x->next) {					\
	struct marker *m = find_marker (x);				\
	if (!m) {							\
	  DO_IF_DEBUG (							\
	    fprintf (stderr, "Didn't find gc marker as expected for:\n"); \
	    describe_something (x, T_TYPE, 2, 2, 0, NULL);		\
	  );								\
	}								\
	else {								\
	  int is_static = 0;						\
	  static const struct TYPE *statics[] = STATICS;		\
	  ptrdiff_t i; /* Use signed type to avoid warnings from gcc. */ \
	  for (i = 0; i < (ptrdiff_t) NELEM (statics); i++)		\
	    if (x == statics[i])					\
	      is_static = 1;						\
	  if (x->refs != m->refs + is_static) {				\
	    num++;							\
	    fprintf (stderr, NAME " got %d unaccounted references:\n",	\
		     x->refs - (m->refs + is_static));			\
	    describe_something (x, T_TYPE, 2, 2, 0, NULL);		\
	  }								\
	}								\
      }									\
      if (num)								\
	fprintf (stderr, NAME "s left: %"PRINTSIZET"d\n", num);		\
    } while (0)

    REPORT_LINKED_LIST_LEAKS (array, first_array, STATIC_ARRAYS, T_ARRAY, "Array");
    REPORT_LINKED_LIST_LEAKS (multiset, first_multiset, {}, T_MULTISET, "Multiset");
    REPORT_LINKED_LIST_LEAKS (mapping, first_mapping, {}, T_MAPPING, "Mapping");
    REPORT_LINKED_LIST_LEAKS (program, first_program, {}, T_PROGRAM, "Program");
    REPORT_LINKED_LIST_LEAKS (object, first_object, {}, T_OBJECT, "Object");

#undef REPORT_LINKED_LIST_LEAKS

    /* Just remove the extra external refs reported above and do
     * another gc so that we don't report the blocks again in the low
     * level dmalloc reports. */

#define ZAP_LINKED_LIST_LEAKS(TYPE, START, STATICS) do {		\
      struct TYPE *x;							\
      for (x = START; x; x = x->next) {					\
	struct marker *m = find_marker (x);				\
	if (m) {							\
	  int is_static = 0;						\
	  static const struct TYPE *statics[] = STATICS;		\
	  ptrdiff_t i; /* Use signed type to avoid warnings from gcc. */ \
	  for (i = 0; i < (ptrdiff_t) NELEM (statics); i++)		\
	    if (x == statics[i])					\
	      is_static = 1;						\
	  while (x->refs > m->refs + is_static)				\
	    PIKE_CONCAT(free_, TYPE) (x);				\
	}								\
      }									\
    } while (0)

    ZAP_LINKED_LIST_LEAKS (array, first_array, STATIC_ARRAYS);
    ZAP_LINKED_LIST_LEAKS (multiset, first_multiset, {});
    ZAP_LINKED_LIST_LEAKS (mapping, first_mapping, {});
    ZAP_LINKED_LIST_LEAKS (program, first_program, {});
    ZAP_LINKED_LIST_LEAKS (object, first_object, {});

#undef ZAP_LINKED_LIST_LEAKS

    /* If we stumble on the real refs whose refcounts we've zapped
     * above we should try to handle it gracefully. */
    gc_external_refs_zapped = 1;

    gc_keep_markers = 0;
    do_gc (NULL, 1);

#ifdef DEBUG_MALLOC
    {
      INT32 num, size;
      count_memory_in_pike_types(&num, &size);
      if (num)
	fprintf(stderr, "Types left: %d (%d bytes)\n", num, size);
      describe_all_types();
    }
#endif
  }
#endif

  destruct_objects_to_destruct_cb();

  /* Now there are no arrays/objects/programs/anything left. */

  really_clean_up_interpret();

  cleanup_callbacks();
  free_all_callable_blocks();
  exit_destroy_called_mark_hash();

  cleanup_pike_type_table();
  cleanup_shared_string_table();

  free_dynamic_load();
  first_mapping=0;
  free_all_mapping_blocks();
  first_object=0;
  free_all_object_blocks();
  first_program=0;
  free_all_program_blocks();
#ifdef PIKE_NEW_MULTISETS
  exit_multiset();
#endif
#endif
}
