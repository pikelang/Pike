/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
RCSID("$Id: main.c,v 1.27 1997/11/08 01:34:40 hubbe Exp $");
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
#include "mapping.h"

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


void main(int argc, char **argv, char **env)
{
  JMP_BUF back;
  int e, num;
  char *p;
  struct array *a;

  ARGV=argv;

  /* Close extra fds (if any) */
  for (e=3; e < MAX_OPEN_FILEDESCRIPTORS; e++) {
    do {
      num = close(e);
    } while ((num < 0) && (errno == EINTR));
  }

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
	  if(p[1]>='0' && p[1]<='9')
	    d_flag+=STRTOL(p+1,&p,10);
	  else
	    d_flag++,p++;
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
  init_lex();
  init_types();

  init_modules();
  master();
  call_callback(& post_master_callbacks, 0);
  free_callback(& post_master_callbacks);
  
  a=allocate_array_no_init(argc,0);
  for(num=0;num<argc;num++)
  {
    ITEM(a)[num].u.string=make_shared_string(argv[num]);
    ITEM(a)[num].type=T_STRING;
  }
  push_array(a);

  for(num=0;env[num];num++);
  a=allocate_array_no_init(num,0);
  for(num=0;env[num];num++)
  {
    ITEM(a)[num].u.string=make_shared_string(env[num]);
    ITEM(a)[num].type=T_STRING;
  }
  push_array(a);

  if(SETJMP(back))
  {
    ONERROR tmp;
    SET_ONERROR(tmp,exit_on_error,"Error in handle_error in master object!");
    assign_svalue_no_free(sp, & throw_value);
    sp++;
    APPLY_MASTER("handle_error", 1);
    pop_stack();
    UNSET_ONERROR(tmp);
    exit(10);
  }

  apply(master(),"_main",2);
  pop_stack();

  UNSETJMP(back);

  backend();

  push_int(0);
  f_exit(1);
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

  th_cleanup();
  exit_dynamic_load();
  exit_signals();
  exit_lex();
  cleanup_interpret();
  cleanup_added_efuns();
  cleanup_pike_types();
  cleanup_program();

  do_gc();

  cleanup_callbacks();
  zap_all_arrays();
  zap_all_mappings();

  cleanup_shared_string_table();
}

