/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
#include "fdlib.h"
#include "svalue.h"
#include "interpret.h"
#include "stralloc.h"
#include "constants.h"
#include "pike_macros.h"
#include "backend.h"
#include "error.h"
#include "callback.h"
#include "mapping.h"
#include "threads.h"
#include "signal_handler.h"
#include "module_support.h"
#include <signal.h>

#ifdef HAVE_WINBASE_H
#include <winbase.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef NSIG
#define MAX_SIGNALS NSIG
#else
#define MAX_SIGNALS 256
#endif

#define SIGNAL_BUFFER 16384
#define WAIT_BUFFER 4096

static struct svalue signal_callbacks[MAX_SIGNALS];

static unsigned char sigbuf[SIGNAL_BUFFER];
static int firstsig, lastsig;
static struct callback *signal_evaluator_callback =0;

#ifndef __NT__
struct wait_data {
  pid_t pid;
  int status;
};

static struct wait_data wait_buf[WAIT_BUFFER];
static int firstwait=0, lastwait=0;

#endif /* ! NT */

struct sigdesc
{
  int signum;
  char *signame;
};


/*
 * All known signals
 */

static struct sigdesc signal_desc []={
#ifdef SIGHUP
  { SIGHUP, "SIGHUP" },
#endif
#ifdef SIGINT
  { SIGINT, "SIGINT" },
#endif
#ifdef SIGQUIT
  { SIGQUIT, "SIGQUIT" },
#endif
#ifdef SIGILL
  { SIGILL, "SIGILL" },
#endif
#ifdef SIGTRAP
  { SIGTRAP, "SIGTRAP" },
#endif
#ifdef SIGABRT
  { SIGABRT, "SIGABRT" },
#endif
#ifdef SIGIOT
  { SIGIOT, "SIGIOT" },
#endif
#ifdef SIGBUS
  { SIGBUS, "SIGBUS" },
#endif
#ifdef SIGFPE
  { SIGFPE, "SIGFPE" },
#endif
#ifdef SIGKILL
  { SIGKILL, "SIGKILL" },
#endif

#if !defined(__linux__) || !defined(_REENTRANT)
#ifdef SIGUSR1
  { SIGUSR1, "SIGUSR1" },
#endif
#ifdef SIGUSR2
  { SIGUSR2, "SIGUSR2" },
#endif
#endif

#ifdef SIGSEGV
  { SIGSEGV, "SIGSEGV" },
#endif
#ifdef SIGPIPE
  { SIGPIPE, "SIGPIPE" },
#endif
#ifdef SIGALRM
  { SIGALRM, "SIGALRM" },
#endif
#ifdef SIGTERM
  { SIGTERM, "SIGTERM" },
#endif
#ifdef SIGSTKFLT
  { SIGSTKFLT, "SIGSTKFLT" },
#endif
#ifdef SIGCHLD
  { SIGCHLD, "SIGCHLD" },
#endif
#ifdef SIGCLD
  { SIGCLD, "SIGCLD" },
#endif
#ifdef SIGCONT
  { SIGCONT, "SIGCONT" },
#endif
#ifdef SIGSTOP
  { SIGSTOP, "SIGSTOP" },
#endif
#ifdef SIGTSTP
  { SIGTSTP, "SIGTSTP" },
#endif
#ifdef SIGTSTP
  { SIGTSTP, "SIGTSTP" },
#endif
#ifdef SIGTTIN
  { SIGTTIN, "SIGTTIN" },
#endif
#ifdef SIGTTIO
  { SIGTTIO, "SIGTTIO" },
#endif
#ifdef SIGURG
  { SIGURG, "SIGURG" },
#endif
#ifdef SIGXCPU
  { SIGXCPU, "SIGXCPU" },
#endif
#ifdef SIGXFSZ
  { SIGXFSZ, "SIGXFSZ" },
#endif
#ifdef SIGVTALRM
  { SIGVTALRM, "SIGVTALRM" },
#endif
#ifdef SIGPROF
  { SIGPROF, "SIGPROF" },
#endif
#ifdef SIGWINCH
  { SIGWINCH, "SIGWINCH" },
#endif
#ifdef SIGIO
  { SIGIO, "SIGIO" },
#endif
#ifdef SIGPOLL
  { SIGPOLL, "SIGPOLL" },
#endif
#ifdef SIGLOST
  { SIGLOST, "SIGLOST" },
#endif
#ifdef SIGPWR
  { SIGPWR, "SIGPWR" },
#endif
#ifdef SIGUNUSED
  { SIGUNUSED, "SIGUNUSED" },
#endif
  { -1, "END" } /* Notused */
};

typedef RETSIGTYPE (*sigfunctype) (int);

static void my_signal(int sig, sigfunctype fun)
{
#ifdef HAVE_SIGACTION
  {
    struct sigaction action;
    action.sa_handler=fun;
    sigfillset(&action.sa_mask);
    action.sa_flags=0;
#ifdef SA_INTERRUPT
    if(fun != SIG_IGN)
      action.sa_flags=SA_INTERRUPT;
#endif
    sigaction(sig,&action,0);
  }
#else
#ifdef HAVE_SIGVEC
  {
    struct sigvec action;
    MEMSET((char *)&action, 0, sizeof(action));
    action.sv_handler= fun;
    action.sv_mask=-1;
#ifdef SA_INTERRUPT
    if(fun != SIG_IGN)
      action.sv_flags=SV_INTERRUPT;
#endif
    sigvec(sig,&action,0);
  }
#else
  signal(sig, fun);
#endif
#endif
}

static RETSIGTYPE receive_signal(int signum)
{
  int tmp;

  tmp=firstsig+1;
  if(tmp == SIGNAL_BUFFER) tmp=0;
  if(tmp != lastsig)
  {
    sigbuf[tmp]=signum;
    firstsig=tmp;
  }

#ifndef __NT__
  if(signum==SIGCHLD)
  {
    pid_t pid;
    int status;
    /* We carefully reap what we saw */
#ifdef HAVE_WAITPID
    pid=waitpid(-1,& status,WNOHANG);
#else
#ifdef HAVE_WAIT3
    pid=wait3(&status,WNOHANG,0);
#else
#ifdef HAVE_WAIT4
    pid=wait4(-1,&status,WNOHANG,0);
#else
    pid=-1;
#endif
#endif
#endif
    fprintf(stderr,"pid %d died with code %d\n",pid,status);

    if(pid>0)
    {
      int tmp2=firstwait+1;
      if(tmp2 == WAIT_BUFFER) tmp=0;
      if(tmp2 != lastwait)
      {
	wait_buf[tmp2].pid=pid;
	wait_buf[tmp2].status=status;
	firstwait=tmp2;
      }
    }
  }
#endif

  wake_up_backend();

#ifndef SIGNAL_ONESHOT
  my_signal(signum, receive_signal);
#endif
}

#define PROCESS_UNKNOWN -1
#define PROCESS_RUNNING 0
#define PROCESS_EXITED 2

#undef THIS
#define THIS ((struct pid_status *)fp->current_storage)

struct pid_status
{
  int pid;
#ifdef __NT__
  HANDLE handle;
#else
  int state;
  int result;
#endif
};

#ifndef __NT__
static struct mapping *pid_mapping=0;
#endif
static struct program *pid_status_program=0;


static void init_pid_status(struct object *o)
{
  THIS->pid=-1;
#ifdef __NT__
  THIS->handle=INVALID_HANDLE_VALUE;
#else
  THIS->state=PROCESS_UNKNOWN;
  THIS->result=-1;
#endif
}

static void exit_pid_status(struct object *o)
{
#ifndef __NT__
  if(pid_mapping)
  {
    struct svalue key;
    key.type=T_INT;
    key.u.integer=THIS->pid;
    map_delete(pid_mapping, &key);
  }
#else
  CloseHandle(THIS->handle);
#endif
}

#ifndef __NT__
static void report_child(int pid,
			 int status)
{
  fprintf(stderr,"pid %d exited with status %d\n",pid,status);

  if(pid_mapping)
  {
    struct svalue *s, key;
    key.type=T_INT;
    key.u.integer=pid;
    if((s=low_mapping_lookup(pid_mapping, &key)))
    {
      if(s->type == T_OBJECT)
      {
	struct pid_status *p;
	if((p=(struct pid_status *)get_storage(s->u.object,
					       pid_status_program)))
	{
	  p->state = PROCESS_EXITED;
	  p->result = WEXITSTATUS(status);
	}
      }
      map_delete(pid_mapping, &key);
    }
  }
}
#endif

static void f_pid_status_wait(INT32 args)
{
  pop_n_elems(args);
  if(THIS->pid == -1)
    error("This process object has no process associated with it.\n");
#ifdef __NT__
  {
    int err=0;
    DWORD xcode;
    HANDLE h=THIS->handle;
    THREADS_ALLOW();
    while(1)
    {
      if(GetExitCodeProcess(h, &xcode))
      {
	if(xcode == STILL_ACTIVE)
	{
	  WaitForSingleObject(h, INFINITE);
	}else{
	  break;
	}
      }else{
	err=1;
	break;
      }
    }
    THREADS_DISALLOW();
    if(err)
      error("Failed to get status of process.\n");
    push_int(xcode);
  }
#else

#if 1
  while(THIS->state == PROCESS_RUNNING)
  {
    int pid, status;
    pid=THIS->pid;
    THREADS_ALLOW();
#ifdef HAVE_WAITPID
    pid=waitpid(pid,& status,0);
#else
#ifdef HAVE_WAIT4
    pid=wait4(pid,&status,0,0);
#else
    pid=-1;
#endif
#endif
    THREADS_DISALLOW();
    if(pid >= -1)
      report_child(pid, status);
    check_signals(0,0,0);
  }
#else
  init_signal_wait();
  while(THIS->state == PROCESS_RUNNING)
  {
    wait_for_signal();
    check_threads_etc();
  }
  exit_signal_wait();
#endif
  push_int(THIS->result);
#endif /* __NT__ */
}

static void f_pid_status_status(INT32 args)
{
  pop_n_elems(args);
#ifdef __NT__
  {
    DWORD x;
    if(GetExitCodeProcess(THIS->handle, &x))
    {
      push_int( x == STILL_ACTIVE ? PROCESS_RUNNING : PROCESS_EXITED);
    }else{
      push_int(PROCESS_UNKNOWN);
    }
  }
#else
  push_int(THIS->state);
#endif
}

static void f_pid_status_pid(INT32 args)
{
  pop_n_elems(args);
  push_int(THIS->pid);
}

#ifdef __NT__
static TCHAR *convert_string(char *str, int len)
{
  int e;
  TCHAR *ret=(TCHAR *)xalloc((len+1) * sizeof(TCHAR));
  for(e=0;e<len;e++) ret[e]=EXTRACT_UCHAR(str+e);
  ret[e]=0;
  return ret;
}

static HANDLE get_inheritable_handle(struct mapping *optional, char *name)
{
  HANDLE ret=INVALID_HANDLE_VALUE;
  struct svalue *tmp;
  if((tmp=simple_mapping_string_lookup(optional, name)))
  {
    if(tmp->type == T_OBJECT)
    {
      apply(tmp->u.object,"query_fd",0);
      if(sp[-1].type == T_INT)
      {
	if(!DuplicateHandle(GetCurrentProcess(),
			    (HANDLE)da_handle[sp[-1].u.integer],
			    GetCurrentProcess(),
			    &ret,
			    NULL,
			    1,
			    DUPLICATE_SAME_ACCESS))
	  /* This could cause handle-leaks */
	  error("Failed to duplicate handle %d.\n",GetLastError());
      }
      pop_stack();
    }
  }
  return ret;
}
#endif

/*
 * create_process(string *arguments, mapping optional_data);
 */
void f_create_process(INT32 args)
{
  struct array *cmd=0;
  struct mapping *optional=0;
  struct svalue *tmp;
  int e;

  if(!args) return;

  check_all_args("create_process",args, BIT_ARRAY, BIT_MAPPING | BIT_VOID, 0);

  switch(args)
  {
    default:
      optional=sp[1-args].u.mapping;
      mapping_fix_type_field(optional);

      if(m_ind_types(optional) & ~BIT_STRING)
	error("Bad index type in argument 2 to Process->create()\n");

    case 1: cmd=sp[-args].u.array;
      if(cmd->size < 1)
	error("Too few elements in argument array.\n");
      
      for(e=0;e<cmd->size;e++)
	if(ITEM(cmd)[e].type!=T_STRING)
	  error("Argument is not a string.\n");

      array_fix_type_field(cmd);

      if(cmd->type_field & ~BIT_STRING)
	error("Bad argument 1 to Process().\n");
  }


#ifdef __NT__
  {
    HANDLE t1=INVALID_HANDLE_VALUE;
    HANDLE t2=INVALID_HANDLE_VALUE;
    HANDLE t3=INVALID_HANDLE_VALUE;
    STARTUPINFO info;
    PROCESS_INFORMATION proc;
    int ret;
    TCHAR *filename=NULL, *command_line=NULL, *dir=NULL;
    struct pike_string *p1,*p2;


    /* FIX QUOTING LATER */
    p1=make_shared_string(" ");
    p2=implode(cmd, p1);
    command_line=convert_string(p2->str, p2->len);
    free_string(p1);
    free_string(p2);
    
    GetStartupInfo(&info);

    info.dwFlags|=STARTF_USESTDHANDLES;
    info.hStdInput=GetStdHandle(STD_INPUT_HANDLE);
    info.hStdOutput=GetStdHandle(STD_OUTPUT_HANDLE);
    info.hStdError=GetStdHandle(STD_ERROR_HANDLE);

    if(optional)
    {
      if(tmp=simple_mapping_string_lookup(optional, "cwd"))
	if(tmp->type == T_STRING)
	  dir=convert_string(tmp->u.string->str, tmp->u.string->len);

      t1=get_inheritable_handle(optional, "stdin");
      if(t1!=INVALID_HANDLE_VALUE) info.hStdInput=t1;

      t2=get_inheritable_handle(optional, "stdout");
      if(t2!=INVALID_HANDLE_VALUE) info.hStdOutput=t2;

      t3=get_inheritable_handle(optional, "stderr");
      if(t3!=INVALID_HANDLE_VALUE) info.hStdError=t3;

      /* FIX: env, cleanup */
    }

    THREADS_ALLOW();
    ret=CreateProcess(filename,
		      command_line,
		      NULL,  /* process security attribute */
		      NULL,  /* thread security attribute */
		      1,     /* inherithandles */
		      0,     /* create flags */
		      NULL,  /* environment */
		      dir,   /* current dir */
		      &info,
		      &proc);
    THREADS_DISALLOW();
    
    if(dir) free((char *)dir);
    if(command_line) free((char *)command_line);
    if(filename) free((char *)filename);

#if 1
    if(t1!=INVALID_HANDLE_VALUE) CloseHandle(t1);
    if(t2!=INVALID_HANDLE_VALUE) CloseHandle(t2);
    if(t3!=INVALID_HANDLE_VALUE) CloseHandle(t3);
#endif
    
    if(ret)
    {
      CloseHandle(proc.hThread);
      THIS->handle=proc.hProcess;
      THIS->pid=proc.dwProcessId;
    }else{
      error("Failed to start process (%d).\n",GetLastError());
    }
  }
#else /* __NT__ */
  {
    pid_t pid;
#if defined(HAVE_FORK1) && defined(_REENTRANT)
    pid=fork1();
#else
    pid=fork();
#endif
    if(pid==-1) error("Failed to start process.\n");
    if(pid)
    {
      if(!signal_evaluator_callback)
      {
	signal_evaluator_callback=add_to_callback(&evaluator_callbacks,
						  check_signals,
						  0,0);
      }
      THIS->pid=pid;
      THIS->state=PROCESS_RUNNING;
      ref_push_object(fp->current_object);
      push_int(pid);
      mapping_insert(pid_mapping,sp-1, sp-2);
      pop_n_elems(2);
    }else{
      char **argv;
#ifdef DECLARE_ENVIRON
      extern char **environ;
#endif
      extern void my_set_close_on_exec(int,int);
      extern void do_set_close_on_exec(void);

#ifdef _REENTRANT
      /* forked copy. there is now only one thread running, this one. */
      num_threads=1;
#endif
      call_callback(&fork_child_callback, 0);
      
      
      argv=(char **)xalloc((1+cmd->size) * sizeof(char *));
      
      for(e=0;e<cmd->size;e++)
	argv[e]=ITEM(cmd)[e].u.string->str;
      
      argv[e]=0;
      
      if(optional)
      {
	int toclose[3];
	int fd;

	if((tmp=simple_mapping_string_lookup(optional, "cwd")))
	  if(tmp->type == T_STRING)
	    if(!chdir(tmp->u.string->str))
	      exit(69);

	for(fd=0;fd<3;fd++)
	{
	  char *fdname;
	  switch(fd)
	  {
	    case 0: fdname="stdin"; break;
	    case 1: fdname="stdout"; break;
	    default: fdname="stderr"; break;
	  }
	  
	  if((tmp=simple_mapping_string_lookup(optional, fdname)))
	  {
	    if(tmp->type == T_OBJECT)
	    {
	      apply(tmp->u.object,"query_fd",0);
	      if(sp[-1].type == T_INT)
		dup2(toclose[fd]=sp[-1].u.integer, fd);
	    }
	  }
	}

	for(fd=0;fd<3;fd++)
	{
	  int f2;
	  if(toclose[fd]<0) continue;

	  for(f2=0;f2<fd;f2++)
	    if(toclose[fd]==toclose[f2])
	      break;

	  if(f2 == fd)
	    close(toclose[fd]);
	}
	
	/* Left to do: env, cleanup */
      }
      
      
      my_set_close_on_exec(0,0);
      my_set_close_on_exec(1,0);
      my_set_close_on_exec(2,0);
      do_set_close_on_exec();
      
      execvp(argv[0],argv);
      exit(69);
    }
  }
#endif /* __NT__ */
  pop_n_elems(args);
  push_int(0);
}

#ifdef HAVE_FORK
void f_fork(INT32 args)
{
  struct object *o;
  pid_t pid;
  pop_n_elems(args);
#if defined(HAVE_FORK1) && defined(_REENTRANT)
  pid=fork1();
#else
  pid=fork();
#endif
  if(pid==-1) error("Fork failed\n");

  if(pid)
  {
    struct pid_status *p;
    if(!signal_evaluator_callback)
    {
      signal_evaluator_callback=add_to_callback(&evaluator_callbacks,
						check_signals,
						0,0);
    }
    o=clone_object(pid_status_program,0);
    p=(struct pid_status *)get_storage(o,pid_status_program);
    p->pid=pid;
    p->state=PROCESS_RUNNING;
    push_object(o);
    push_int(pid);
    mapping_insert(pid_mapping,sp-1, sp-2);
    pop_stack();
  }else{
#ifdef _REENTRANT
    /* forked copy. there is now only one thread running, this one. */
    num_threads=1;
#endif
    call_callback(&fork_child_callback, 0);
    push_int(0);
  }
}
#endif /* HAVE_FORK */


#ifdef HAVE_KILL
static void f_kill(INT32 args)
{
  if(args < 2)
    error("Too few arguments to kill().\n");

  switch(sp[-args].type)
  {
  case T_INT:
    break;

  case T_OBJECT:
  {
    INT32 pid;
    struct pid_status *p;
    if((p=(struct pid_status *)get_storage(sp[-args].u.object,
					  pid_status_program)))
    {
      pid=p->pid;
      free_svalue(sp-args);
      sp[-args].type=T_INT;
      sp[-args].subtype=NUMBER_NUMBER;
      sp[-args].u.integer=pid;
      break;
    }
  }
  default:
    error("Bad argument 1 to kill().\n");
  }
    
  if(sp[1-args].type != T_INT)
    error("Bad argument 1 to kill().\n");

  sp[-args].u.integer=!kill(sp[-args].u.integer,
			    sp[1-args].u.integer);
  check_signals(0,0,0);
  pop_n_elems(args-1);
}

#endif


static int signalling=0;

static void unset_signalling(void *notused) { signalling=0; }

void check_signals(struct callback *foo, void *bar, void *gazonk)
{
  ONERROR ebuf;
#ifdef DEBUG
  extern int d_flag;
  if(d_flag>5) do_debug();
#endif

  if(firstsig != lastsig && !signalling)
  {
    int tmp=firstsig;
    signalling=1;

    SET_ONERROR(ebuf,unset_signalling,0);

    while(lastsig != tmp)
    {
      if(++lastsig == SIGNAL_BUFFER) lastsig=0;

#ifdef SIGCHLD
      if(sigbuf[lastsig]==SIGCHLD)
      {
	int tmp2 = firstwait;
	while(lastwait != tmp2)
	{
	  if(++lastwait == WAIT_BUFFER) lastwait=0;
	  report_child(wait_buf[lastwait].pid,
		       wait_buf[lastwait].status);
	}
      }
#endif

      push_int(sigbuf[lastsig]);
      apply_svalue(signal_callbacks + sigbuf[lastsig], 1);
      pop_stack();
    }

    UNSET_ONERROR(ebuf);

    signalling=0;
  }
}

/* Get the name of a signal given the number */
static char *signame(int sig)
{
  int e;
  for(e=0;e<(int)NELEM(signal_desc)-1;e++)
  {
    if(sig==signal_desc[e].signum)
      return signal_desc[e].signame;
  }
  return 0;
}

/* Get the signal's number given the name */
static int signum(char *name)
{
  int e;
  for(e=0;e<(int)NELEM(signal_desc)-1;e++)
  {
    if(!STRCASECMP(name,signal_desc[e].signame) ||
       !STRCASECMP(name,signal_desc[e].signame+3) )
      return signal_desc[e].signum;
  }
  return -1;
}

static void f_signal(int args)
{
  int signum;
  sigfunctype func;

  check_signals(0,0,0);

  if(args < 1)
    error("Too few arguments to signal()\n");

  if(sp[-args].type != T_INT)
  {
    error("Bad argument 1 to signal()\n");
  }

  signum=sp[-args].u.integer;
  if(signum <0 ||
     signum >=MAX_SIGNALS
#if defined(__linux__) && defined(_REENTRANT)
     || signum == SIGUSR1
     || signum == SIGUSR2
#endif
    )
  {
    error("Signal out of range.\n");
  }

  if(!signal_evaluator_callback)
  {
    signal_evaluator_callback=add_to_callback(&evaluator_callbacks,
					      check_signals,
					      0,0);
  }

  if(args == 1)
  {
    push_int(0);
    args++;

    switch(signum)
    {
#ifdef SIGCHLD
    case SIGCHLD:
      func=receive_signal;
      break;
#endif

#ifdef SIGPIPE
    case SIGPIPE:
      func=(sigfunctype) SIG_IGN;
      break;
#endif

    default:
      func=(sigfunctype) SIG_DFL;
    }
  } else {
    if(IS_ZERO(sp+1-args))
    {
      func=(sigfunctype) SIG_IGN;
    }else{
      func=receive_signal;
    }
  }
  assign_svalue(signal_callbacks + signum, sp+1-args);
  my_signal(signum, func);
  pop_n_elems(args);
}

static void f_signum(int args)
{
  int i;
  if(args < 1)
    error("Too few arguments to signum()\n");

  if(sp[-args].type != T_STRING)
    error("Bad argument 1 to signum()\n");

  i=signum(sp[-args].u.string->str);
  pop_n_elems(args);
  push_int(i);
}

static void f_signame(int args)
{
  char *n;
  if(args < 1)
    error("Too few arguments to signame()\n");

  if(sp[-args].type != T_INT)
    error("Bad argument 1 to signame()\n");

  n=signame(sp[-args].u.integer);
  pop_n_elems(args);
  if(n)
    push_string(make_shared_string(n));
  else
    push_int(0);
}


static void f_getpid(INT32 args)
{
  pop_n_elems(args);
  push_int(getpid());
}

#ifdef HAVE_ALARM
static void f_alarm(INT32 args)
{
  long seconds;

  if(args < 1)
    error("Too few arguments to signame()\n");

  if(sp[-args].type != T_INT)
    error("Bad argument 1 to signame()\n");

  seconds=sp[-args].u.integer;

  pop_n_elems(args);
  push_int(alarm(seconds));
}
#endif

#ifdef HAVE_UALARM
static void f_ualarm(INT32 args)
{
  long seconds;

  if(args < 1)
    error("Too few arguments to ualarm()\n");

  if(sp[-args].type != T_INT)
    error("Bad argument 1 to ualarm()\n");

  seconds=sp[-args].u.integer;

  pop_n_elems(args);
#ifdef UALARM_TAKES_TWO_ARGS
  push_int(ualarm(seconds,0));
#else
  push_int(ualarm(seconds));
#endif
}
#endif

void init_signals(void)
{
  int e;

#ifdef SIGCHLD
  my_signal(SIGCHLD, receive_signal);
#endif

#ifdef SIGPIPE
  my_signal(SIGPIPE, SIG_IGN);
#endif

  for(e=0;e<MAX_SIGNALS;e++)
    signal_callbacks[e].type=T_INT;

  firstsig=lastsig=0;

#ifndef __NT__
  pid_mapping=allocate_mapping(2);
#endif

  start_new_program();
  add_storage(sizeof(struct pid_status));
  set_init_callback(init_pid_status);
  set_exit_callback(exit_pid_status);
  add_function("wait",f_pid_status_wait,"function(:int)",0);
  add_function("status",f_pid_status_status,"function(:int)",0);
  add_function("pid",f_pid_status_pid,"function(:int)",0);
  add_function("create",f_create_process,"function(array(string),void|mapping(string:mixed):object)",0);
  pid_status_program=end_program();
  add_program_constant("create_process",pid_status_program,0);

  add_efun("signal",f_signal,"function(int,mixed|void:void)",OPT_SIDE_EFFECT);
#ifdef HAVE_KILL
  add_efun("kill",f_kill,"function(int|object,int:int)",OPT_SIDE_EFFECT);
#endif
#ifdef HAVE_FORK
  add_efun("fork",f_fork,"function(void:object)",OPT_SIDE_EFFECT);
#endif

  add_efun("signame",f_signame,"function(int:string)",0);
  add_efun("signum",f_signum,"function(string:int)",0);
  add_efun("getpid",f_getpid,"function(:int)",0);
#ifdef HAVE_ALARM
  add_efun("alarm",f_alarm,"function(int:int)",OPT_SIDE_EFFECT);
#endif
#ifdef HAVE_UALARM
  add_efun("ualarm",f_ualarm,"function(int:int)",OPT_SIDE_EFFECT);
#endif
}

void exit_signals(void)
{
  int e;
#ifndef __NT__
  if(pid_mapping)
  {
    free_mapping(pid_mapping);
    pid_mapping=0;
  }
#endif
  if(pid_status_program)
  {
    free_program(pid_status_program);
    pid_status_program=0;
  }


  for(e=0;e<MAX_SIGNALS;e++)
  {
    free_svalue(signal_callbacks+e);
    signal_callbacks[e].type=T_INT;
  }
}


