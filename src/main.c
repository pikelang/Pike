/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
RCSID("$Id: main.c,v 1.41 1998/02/11 00:05:01 hubbe Exp $");
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
#include "error.h"
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

#include <errno.h>

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#include "time_stuff.h"

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif


char *master_file;
char **ARGV;

int debug_options=0;
int d_flag=0;
int c_flag=0;
int t_flag=0;
int a_flag=0;
int l_flag=0;
int p_flag=0;

static struct callback_list post_master_callbacks;

struct callback *add_post_master_callback(callback_func call,
					  void *arg,
					  callback_func free_func)
{
  return add_to_callback(&post_master_callbacks, call, arg, free_func);
}


static struct callback_list exit_callbacks;

struct callback *add_exit_callback(callback_func call,
				   void *arg,
				   callback_func free_func)
{
  return add_to_callback(&exit_callbacks, call, arg, free_func);
}

int dbm_main(int argc, char **argv)
{
  JMP_BUF back;
  int e, num, do_backend;
  char *p;
  struct array *a;
#ifdef DECLARE_ENVIRON
  extern char **environ;
#endif

  ARGV=argv;

  fd_init();

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
  master_file = getenv("PIKE_MASTER");
#endif
#if __NT__
  if(!master_file)
  {
    char buffer[4096];
    DWORD len=sizeof(buffer)-1,type=REG_SZ;
    
    if(RegQueryValueEx(HKEY_CURRENT_USER,
		       "SOFTWARE\\Idonex\\Pike\\0.6\\PIKE_MASTER",
		       0,
		       &type,
		       buffer,
		       &len)==ERROR_SUCCESS ||
       RegQueryValueEx(HKEY_LOCAL_MACHINE,
		       "SOFTWARE\\Idonex\\Pike\\0.6\\PIKE_MASTER",
		       0,
		       &type,
		       buffer,
		       &len)==ERROR_SUCCESS)
    {
      master_file=strdup(buffer);
    }
  }
#endif

  if(!master_file) master_file = DEFAULT_MASTER;

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
	  }
	  stack_size=STRTOL(p+1,&p,0);
	  p+=strlen(p);

	  if(stack_size < 256)
	  {
	    fprintf(stderr,"Stack size must at least be 256.\n");
	    exit(1);
	  }
	  break;

	case 'd':
	more_d_flags:
	  switch(p[1])
	  {
	    case '0': case '1': case '2': case '3': case '4':
	    case '5': case '6': case '7': case '8': case '9':
	      d_flag+=STRTOL(p+1,&p,10);
	      break;

	    case 's':
	      debug_options|=DEBUG_SIGNALS;
	      p++;
	      if(p[1]) goto more_d_flags;
	      p++;
	      break;

	    default:
	      d_flag++,p++;
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
	  break;

	case 'p':
	  if(p[1]>='0' && p[1]<='9')
	    p_flag+=STRTOL(p+1,&p,10);
	  else
	    p_flag++,p++;
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

#if !defined(RLIMIT_NOFILE) && defined(RLIMIT_OFILE)
#define RLIMIT_NOFILE RLIMIT_OFILE
#endif

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

  GETTIMEOFDAY(&current_time);

  init_shared_string_table();
  init_interpreter();
  init_cpp();
  init_lex();
  init_types();

  init_modules();
  master();
  call_callback(& post_master_callbacks, 0);
  free_callback(& post_master_callbacks);
  
  if(SETJMP(back))
  {
    if(throw_severity == THROW_EXIT)
    {
      num=throw_value.u.integer;
    }else{
      ONERROR tmp;
      SET_ONERROR(tmp,exit_on_error,"Error in handle_error in master object!");
      push_svalue(& throw_value);
      APPLY_MASTER("handle_error", 1);
      pop_stack();
      UNSET_ONERROR(tmp);
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

  do_exit(num);
  return num; /* avoid warning */
}

#undef ATTRIBUTE
#define ATTRIBUTE(X)

void do_exit(int num) ATTRIBUTE((noreturn))
{
  call_callback(&exit_callbacks, (void *)0);
  free_callback(&exit_callbacks);

  exit_modules();

#ifdef DEBUG_MALLOC
  {
    extern cleanup_memhdrs(void);
    cleanup_memhdrs();
  }
#endif

  exit(num);
}


void low_init_main(void)
{
  th_init();
  init_builtin_efuns();
  init_signals();
  init_dynamic_load();
}

void exit_main(void)
{
  cleanup_objects();
}

void init_main(void)
{
}

void low_exit_main(void)
{
  void cleanup_added_efuns(void);
  void cleanup_pike_types(void);
  void cleanup_program(void);
  void cleanup_compiler(void);

  th_cleanup();
  exit_dynamic_load();
  exit_signals();
  exit_lex();
  exit_cpp();
  cleanup_interpret();
  cleanup_added_efuns();
  cleanup_pike_types();
  cleanup_program();
  cleanup_compiler();

  do_gc();
  free_svalue(& throw_value);
  throw_value.type=T_INT;

  cleanup_callbacks();
#if defined(DEBUG) && defined(DEBUG_MALLOC)
  if(verbose_debug_exit)
  {
    INT32 num,size,recount=0;
    fprintf(stderr,"Exited normally, counting bytes.\n");

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
      {
	describe_something(p, T_PROGRAM);
      }
    }


    count_memory_in_objects(&num, &size);
    if(num)
      fprintf(stderr,"Objects left: %d (%d bytes)\n",num,size);

    cleanup_shared_string_table();
  }
#else

  zap_all_arrays();
  zap_all_mappings();

  cleanup_shared_string_table();
#endif
}

