/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
#include "svalue.h"
#include "interpret.h"
#include "stralloc.h"
#include "constants.h"
#include "pike_macros.h"
#include "backend.h"
#include "error.h"
#include "callback.h"
#include "mapping.h"
#include "object.h"
#include "threads.h"
#include <signal.h>
#include <sys/wait.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef NSIG
#define MAX_SIGNALS NSIG
#else
#define MAX_SIGNALS 256
#endif

#define SIGNAL_BUFFER 16384
#define WAIT_BUFFER 4096

struct wait_data {
  pid_t pid;
  int status;
};

static struct svalue signal_callbacks[MAX_SIGNALS];

static unsigned char sigbuf[SIGNAL_BUFFER];
static int firstsig=0, lastsig=0;

static struct wait_data wait_buf[WAIT_BUFFER];
static int firstwait=0, lastwait=0;

static struct callback *signal_evaluator_callback =0;
static int sigchild_arrived=0;


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
#ifdef SIGUSR1
  { SIGUSR1, "SIGUSR1" },
#endif
#ifdef SIGSEGV
  { SIGSEGV, "SIGSEGV" },
#endif
#ifdef SIGUSR2
  { SIGUSR2, "SIGUSR2" },
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

  wake_up_backend();

#ifndef SIGNAL_ONESHOT
  my_signal(signum, receive_signal);
#endif
}

#define PROCESS_UNKNOWN 0
#define PROCESS_RUNNING 1
#define PROCESS_EXITED 2

#define THIS ((struct pid_status *)fp->current_storage)

struct mapping *pid_mapping=0;;
struct program *pid_status_program=0;

struct pid_status
{
  int pid;
  int state;
  int result;
};

static void init_pid_status(struct object *o)
{
  THIS->pid=-1;
  THIS->state=PROCESS_UNKNOWN;
  THIS->result=-1;
}

static void exit_pid_status(struct object *o)
{
  if(pid_mapping)
  {
    struct svalue key;
    key.type=T_INT;
    key.u.integer=THIS->pid;
    map_delete(pid_mapping, &key);
  }
}

static void report_child(int pid,
			 int status)
{
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

static int signalling=0;

static void unset_signalling(void *notused) { signalling=0; }

void check_signals(struct callback *foo, void *bar, void *gazonk)
{
  ONERROR ebuf;
#ifdef DEBUG
  extern int d_flag;
  if(d_flag>5) do_debug(0);
#endif

  if(firstsig != lastsig && !signalling)
  {
    int tmp=firstsig;
    signalling=1;

    SET_ONERROR(ebuf,unset_signalling,0);

    while(lastsig != tmp)
    {
      if(++lastsig == SIGNAL_BUFFER) lastsig=0;

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

      if(signal_callbacks[sigbuf[lastsig]].type != T_INT)
      {
	push_int(sigbuf[lastsig]);
	apply_svalue(signal_callbacks + sigbuf[lastsig], 1);
	pop_stack();
      }
    }

    UNSET_ONERROR(ebuf);

    signalling=0;
  }
}

static void f_pid_status_wait(INT32 args)
{
  pop_n_elems(args);
#if 1
  while(THIS->state == PROCESS_RUNNING)
  {
    int pid, status;
    pid=THIS->pid;
    THREADS_ALLOW();
#ifdef HAVE_WAITPID
    pid=waitpid(pid,& status,WNOHANG);
#else
#ifdef HAVE_WAIT4
    pid=wait4(pid,&status,WNOHANG,0);
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
}

static void f_pid_status_status(INT32 args)
{
  pop_n_elems(args);
  push_int(THIS->state);
}

static void f_pid_status_pid(INT32 args)
{
  pop_n_elems(args);
  push_int(THIS->pid);
}

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
    push_int(0);
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
  if(signum <0 || signum >=MAX_SIGNALS)
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
    case SIGCHLD:
      func=receive_signal;
      break;

    case SIGPIPE:
      func=(sigfunctype) SIG_IGN;
      break;

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

static void f_kill(INT32 args)
{
  pid_t pid;
  if(args < 2)
    error("Too few arguments to kill().\n");
  switch(sp[-args].type)
  {
  case T_INT:
    pid=sp[-args].u.integer;
    break;

  case T_OBJECT:
  {
    struct pid_status *p;
    if((p=(struct pid_status *)get_storage(sp[-args].u.object,
					  pid_status_program)))
    {
      pid=p->pid;
      break;
    }
  }
  default:
    error("Bad argument 1 to kill().\n");
  }
    
  if(sp[-args].type != T_INT)
  if(sp[1-args].type != T_INT)
    error("Bad argument 1 to kill().\n");

  sp[-args].u.integer=!kill(sp[-args].u.integer,sp[1-args].u.integer);
  pop_n_elems(args-1);
}

static void f_getpid(INT32 args)
{
  pop_n_elems(args);
  push_int(getpid());
}

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

void init_signals()
{
  int e;

  my_signal(SIGCHLD, receive_signal);
  my_signal(SIGPIPE, SIG_IGN);

  for(e=0;e<MAX_SIGNALS;e++)
    signal_callbacks[e].type=T_INT;

  firstsig=lastsig=0;

  pid_mapping=allocate_mapping(2);
  start_new_program();
  add_storage(sizeof(struct pid_status));
  set_init_callback(init_pid_status);
  set_init_callback(exit_pid_status);
  add_function("wait",f_pid_status_wait,"function(:int)",0);
  add_function("status",f_pid_status_status,"function(:int)",0);
  add_function("pid",f_pid_status_pid,"function(:int)",0);
  pid_status_program=end_program();

  add_efun("signal",f_signal,"function(int,mixed|void:void)",OPT_SIDE_EFFECT);
  add_efun("kill",f_kill,"function(int|object,int:int)",OPT_SIDE_EFFECT);
  add_efun("signame",f_signame,"function(int:string)",0);
  add_efun("signum",f_signum,"function(string:int)",0);
  add_efun("getpid",f_getpid,"function(:int)",0);
  add_efun("alarm",f_alarm,"function(int:int)",OPT_SIDE_EFFECT);
  add_efun("fork",f_fork,"function(:object)",OPT_SIDE_EFFECT);
#ifdef HAVE_UALARM
  add_efun("ualarm",f_ualarm,"function(int:int)",OPT_SIDE_EFFECT);
#endif
}

void exit_signals()
{
  int e;
  if(pid_mapping)
  {
    free_mapping(pid_mapping);
    pid_mapping=0;
  }

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


