/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
#include "types.h"
#include "backend.h"
#include "module.h"
#include "object.h"
#include "lex.h"
#include "lpc_types.h"
#include "builtin_efuns.h"
#include "array.h"
#include "stralloc.h"
#include "interpret.h"
#include "error.h"
#include "macros.h"
#include "callback.h"
#include "lpc_signal.h"

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  if HAVE_TIME_H
#   include <time.h>
#  endif
# endif
#endif

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

char *master_file;

int d_flag=0;
int c_flag=0;
int t_flag=0;
int a_flag=0;
int l_flag=0;

static struct callback_list *post_master_callbacks =0;

struct callback_list *add_post_master_callback(struct array *a)
{
  return add_to_callback_list(&post_master_callbacks, a);
}


void main(int argc, char **argv, char **env)
{
  JMP_BUF back;
  int e, num;
  char *p;
  struct array *a;

#ifdef HAVE_SETLOCALE
  setlocale(LC_ALL, "");
#endif  
  init_backend();
  master_file = 0;
#ifdef HAVE_GETENV
  master_file = getenv("LPC_MASTER");
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

	case 'l':
	  if(p[1]>='0' && p[1]<='9')
	    l_flag+=STRTOL(p+1,&p,10);
	  else
	    l_flag++,p++;
	  break;

	default:
	  fprintf(stderr,"Uknown flag '%c'\n",*p);
	  exit(1);
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

  current_time = get_current_time();

  init_modules_efuns();
  master();
  call_and_free_callback_list(& post_master_callbacks);
  init_modules_programs();

  a=allocate_array_no_init(argc-e,0);
  for(num=0;e<argc;e++)
  {
    ITEM(a)[num].u.string=make_shared_string(argv[e]);
    ITEM(a)[num].type=T_STRING;
    num++;
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
    automatic_fatal="Error in handle_error, previous error (in _main): ";
    assign_svalue_no_free(sp++, & throw_value);
    APPLY_MASTER("handle_error", 1);
    pop_stack();
    exit(10);
  }

  apply(master(),"_main",2);
  pop_stack();

  UNSETJMP(back);

  backend();

  push_int(0);
  f_exit(1);
}


void init_main_efuns()
{
  init_lex();
  init_types();
  init_builtin_efuns();
  init_signals();
}

void init_main_programs()
{
}


void exit_main()
{
  void cleanup_added_efuns();
  void free_all_call_outs();
  void cleanup_lpc_types();
  void cleanup_program();

  automatic_fatal="uLPC is exiting: ";
  exit_signals();
  exit_lex();
  cleanup_objects();
  cleanup_interpret();
  cleanup_added_efuns();
  free_all_call_outs();
  cleanup_lpc_types();
  cleanup_shared_string_table();
  cleanup_program();
}

