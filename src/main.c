/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
/**/
#include "global.h"
RCSID("$Id: main.c,v 1.116 2003/05/07 21:01:03 mast Exp $");
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
#include "security.h"
#include "constants.h"
#include "version.h"

#if defined(__linux__) && defined(HAVE_DLOPEN) && defined(HAVE_DLFCN_H)
#include <dlfcn.h>
#endif

#include "las.h"

#include <errno.h>

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#include "time_stuff.h"

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#ifdef TRY_USE_MMX
#include <mmx.h>
int try_use_mmx;
#endif


char *master_file;
char **ARGV;

PMOD_EXPORT int debug_options=0;
PMOD_EXPORT int runtime_options=0;
PMOD_EXPORT int d_flag=0;
PMOD_EXPORT int c_flag=0;
PMOD_EXPORT int t_flag=0;
PMOD_EXPORT int default_t_flag=0;
PMOD_EXPORT int a_flag=0;
PMOD_EXPORT int l_flag=0;
PMOD_EXPORT int p_flag=0;
#ifdef YYDEBUG
extern int yydebug;
#endif /* YYDEBUG */
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
  long ret;
  if(RegOpenKeyEx(cat,
		  (LPCTSTR)"SOFTWARE\\Idonex\\Pike\\7.2",
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

int dbm_main(int argc, char **argv)
{
  JMP_BUF back;
  int e, num, do_backend;
  char *p;
  struct array *a;
#ifdef DECLARE_ENVIRON
  extern char **environ;
#endif

#ifdef TRY_USE_MMX
  try_use_mmx=mmx_ok();
#endif
#ifdef OWN_GETHRTIME
/* initialize our own gethrtime conversion /Mirar */
  own_gethrtime_init();
#endif

  ARGV=argv;

  fd_init();

#ifdef SHARED_NODES
  node_hash.table = malloc(sizeof(node *)*32831);
  if (!node_hash.table) {
    fatal("Out of memory!\n");
  }
  MEMSET(node_hash.table, 0, sizeof(node *)*32831);
  node_hash.size = 32831;
#endif /* SHARED_NODES */

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

  init_backend();
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

  for(e=1; e<argc; e++)
  {
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
#ifdef YYDEBUG
	      yydebug++;
#endif /* YYDEBUG */
	      break;

	    case 's':
	      debug_options|=DEBUG_SIGNALS;
	      p++;
	      goto more_d_flags;

	    case 't':
	      debug_options|=NO_TAILRECURSION;
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
	    t_flag+=STRTOL(p+1,&p,10);
	  else
	    t_flag++,p++;
	  default_t_flag = t_flag;
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

#if !defined(HAVE_PTHREAD_MUTEX_ERRORCHECK) && !defined(HAVE_PTHREAD_MUTEX_ERRORCHECK_NP)
  if (debug_options & ERRORCHECK_MUTEXES)
    fputs ("Warning: -dT (error checking mutexes) not supported on this system.\n",
	   stderr);
#endif
  if (d_flag) debug_options |= ERRORCHECK_MUTEXES;

#if !defined(RLIMIT_NOFILE) && defined(RLIMIT_OFILE)
#define RLIMIT_NOFILE RLIMIT_OFILE
#endif

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
  Pike_interpreter.stack_top += (~((size_t)Pike_interpreter.stack_top)) & 0xffff;
#else /* STACK_DIRECTION >= 0 */
  /* Equvivalent with &= ~0xffff */
  Pike_interpreter.stack_top -= ( ((size_t)Pike_interpreter.stack_top)) & 0xffff;
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

#if defined(__linux__) && defined(HAVE_DLOPEN) && defined(HAVE_DLFCN_H)
      {
	char ** bos_location;
	void *handle;
	/* damn this is ugly -Hubbe */
	if((handle=dlopen(0, RTLD_LAZY)))
	{
	  bos_location=dlsym(handle,"__pthread_initial_thread_bos");
	  dlclose(handle);

	  if(bos_location && *bos_location &&
	     (*bos_location - Pike_interpreter.stack_top) *STACK_DIRECTION < 0)
	  {
	    Pike_interpreter.stack_top=*bos_location;
	  }
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
#endif /* __linux__ && HAVE_DLOPEN && HAVE_DLFCN_H */

      Pike_interpreter.stack_top -= STACK_DIRECTION * 8192 * sizeof(char *);

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
  
  GETTIMEOFDAY(&current_time);

  low_th_init();
  
  init_shared_string_table();
  init_interpreter();
  init_types();
  init_cpp();
  init_lex();
  init_program();
  init_object();

  init_modules();
  master();
  call_callback(& post_master_callbacks, 0);
  free_callback_list(& post_master_callbacks);
  
  if(SETJMP(back))
  {
    if(throw_severity == THROW_EXIT)
    {
      num=throw_value.u.integer;
    }else{
      call_handle_error();
      num=10;
    }
  }else{
    back.severity=THROW_EXIT;

    a=allocate_array_no_init(argc,0);
    for(num=0;num<argc;num++)
    {
      ITEM(a)[num].u.string=make_shared_string(argv[num]);
      ITEM(a)[num].type=T_STRING;
    }
    push_array(a);
    
    for(num=0;environ[num];num++);
    a=allocate_array_no_init(num,0);
    for(num=0;environ[num];num++)
    {
      ITEM(a)[num].u.string=make_shared_string(environ[num]);
      ITEM(a)[num].type=T_STRING;
    }
    push_array(a);
  
    apply(master(),"_main",2);
    pop_stack();
    
    backend();
    num=0;
  }
  UNSETJMP(back);

  pike_do_exit(num);
  return num; /* avoid warning */
}

#undef ATTRIBUTE
#define ATTRIBUTE(X)

DECLSPEC(noreturn) void pike_do_exit(int num) ATTRIBUTE((noreturn))
{
  call_callback(&exit_callbacks, (void *)0);
  free_callback_list(&exit_callbacks);

  exit_modules();

#ifdef DEBUG_MALLOC
  {
    extern void cleanup_memhdrs(void);
    cleanup_memhdrs();
  }
#endif

#ifdef PROFILING
  {
    int q;
    for(q=0;q<(long)NELEM(samples);q++)
      if(samples[q])
	fprintf(stderr,"STACK WAS %4d Kb %12u times\n",q-1,samples[q]);
  }
#endif

  exit(num);
}


void low_init_main(void)
{
  init_pike_searching();
  init_error();
  init_pike_security();
  th_init();
  init_operators();
  init_builtin_efuns();
  init_signals();
  init_dynamic_load();
}

void exit_main(void)
{
#ifdef DO_PIKE_CLEANUP
  cleanup_objects();
#endif
}

void init_main(void)
{
}

void low_exit_main(void)
{
#ifdef DO_PIKE_CLEANUP
  void cleanup_added_efuns(void);
  void cleanup_pike_types(void);
  void cleanup_program(void);
  void cleanup_compiler(void);
  void cleanup_backend(void);
  void free_all_mapping_blocks(void);
  void free_all_object_blocks(void);
  void exit_builtin(void);

#ifdef AUTO_BIGNUM
  void exit_auto_bignum(void);
  exit_auto_bignum();
#endif
  exit_pike_searching();
  th_cleanup();
  exit_object();
  exit_dynamic_load();
  exit_signals();
  exit_builtin();
  exit_lex();
  exit_cpp();
  cleanup_interpret();
  cleanup_added_efuns();
  exit_operators();
  cleanup_pike_types();
  cleanup_program();
  cleanup_compiler();
  cleanup_error();
  cleanup_backend();

#ifdef SHARED_NODES
  free(node_hash.table);
#endif /* SHARED_NODES */

  exit_pike_security();
  free_svalue(& throw_value);
  throw_value.type=T_INT;
  do_gc();

  cleanup_gc();

#if defined(PIKE_DEBUG) && defined(DEBUG_MALLOC)
  if(verbose_debug_exit)
  {
    INT32 num,size,recount=0;
    fprintf(stderr,"Exited normally, counting bytes.\n");

#ifdef _REENTRANT
    if(count_pike_threads()>1)
    {
      fprintf(stderr,"Byte counting aborted, because all threads have not exited properly.\n");
      verbose_debug_exit=0;
      return;
    }
#endif


    search_all_memheaders_for_references();

    count_memory_in_arrays(&num, &size);
    if(num)
    {
      recount++;
      fprintf(stderr,"Arrays left: %d (%d bytes) (zapped)\n",num,size);
    }

    zap_all_arrays();

    count_memory_in_mappings(&num, &size);
    if(num)
    {
      recount++;
      fprintf(stderr,"Mappings left: %d (%d bytes) (zapped)\n",num,size);
    }

    zap_all_mappings();

    count_memory_in_multisets(&num, &size);
    if(num)
      fprintf(stderr,"Multisets left: %d (%d bytes)\n",num,size);


    if(recount)
    {
      fprintf(stderr,"Garbage collecting..\n");
      do_gc();
      
      count_memory_in_arrays(&num, &size);
      fprintf(stderr,"Arrays left: %d (%d bytes)\n",num,size);
      count_memory_in_mappings(&num, &size);
      fprintf(stderr,"Mappings left: %d (%d bytes)\n",num,size);
      count_memory_in_multisets(&num, &size);
      fprintf(stderr,"Multisets left: %d (%d bytes)\n",num,size);
    }
    

    count_memory_in_programs(&num, &size);
    if(num)
      fprintf(stderr,"Programs left: %d (%d bytes)\n",num,size);

    {
      struct program *p;
      for(p=first_program;p;p=p->next)
	describe_something(p, T_PROGRAM, 0,2,0);
    }


    count_memory_in_objects(&num, &size);
    if(num)
      fprintf(stderr,"Objects left: %d (%d bytes)\n",num,size);

    {
      struct object *o;
      for(o=first_object;o;o=o->next)
	describe_something(o, T_OBJECT, 0,2,0);
    }

    cleanup_shared_string_table();
  }
#else

  zap_all_arrays();
  zap_all_mappings();

  cleanup_shared_string_table();
#endif

  really_clean_up_interpret();

  cleanup_callbacks();
  free_all_callable_blocks();
  exit_destroy_called_mark_hash();

  free_dynamic_load();
  first_mapping=0;
  free_all_mapping_blocks();
  first_object=0;
  free_all_object_blocks();
#endif
}


