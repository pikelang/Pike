/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
/**/
#include "global.h"
#include "fdlib.h"
#include "fd_control.h"
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
#include "operators.h"
#include "builtin_functions.h"
#include <signal.h>

RCSID("$Id: signal_handler.c,v 1.120 1999/04/02 20:52:48 hubbe Exp $");

#ifdef HAVE_PASSWD_H
# include <passwd.h>
#endif

#ifdef HAVE_GROUP_H
# include <group.h>
#endif

#ifdef HAVE_PWD_H
# include <pwd.h>
#endif

#ifdef HAVE_GRP_H
# include <grp.h>
#endif

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

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef __amigaos__
#define timeval amigaos_timeval
#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/exall.h>
#include <clib/dos_protos.h>
#include <inline/dos.h>
#undef timeval
#endif


#ifdef NSIG
#define MAX_SIGNALS NSIG
#else
#define MAX_SIGNALS 256
#endif

#define SIGNAL_BUFFER 16384
#define WAIT_BUFFER 4096

#ifdef HAVE_UNION_WAIT
#define WAITSTATUSTYPE union wait
#else
#define WAITSTATUSTYPE int
#endif

#ifndef WEXITSTATUS
#ifdef HAVE_UNION_WAIT
#define WEXITSTATUS(x) ((x).w_retcode)
#else /* !HAVE_UNION_WAIT */
#define WEXITSTATUS(x)	(((x)>>8)&0xff)
#endif /* HAVE_UNION_WAIT */
#endif /* !WEXITSTATUS */

/* #define PROC_DEBUG */

#ifndef __NT__
#define USE_PID_MAPPING
#else
#undef USE_WAIT_THREAD
#undef USE_SIGCHILD
#endif


/* Added so we are able to patch older versions of Pike. */
#ifndef add_ref
#define add_ref(X)	((X)->refs++)
#endif /* add_ref */

extern int fd_from_object(struct object *o);
static int set_priority( int pid, char *to );



static struct svalue signal_callbacks[MAX_SIGNALS];
static unsigned char sigbuf[SIGNAL_BUFFER];
static int firstsig, lastsig;
static struct callback *signal_evaluator_callback =0;



/*
 * All known signals
 */

#ifdef __NT__

#ifndef SIGKILL
#define SIGKILL 9
#endif

#endif /* __NT__ */

struct sigdesc
{
  int signum;
  char *signame;
};

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
#ifdef SIGEMT
  { SIGEMT, "SIGEMT" },
#endif
#ifdef SIGFPE
  { SIGFPE, "SIGFPE" },
#endif
#ifdef SIGKILL
  { SIGKILL, "SIGKILL" },
#endif
#ifdef SIGBUS
  { SIGBUS, "SIGBUS" },
#endif
#ifdef SIGSEGV
  { SIGSEGV, "SIGSEGV" },
#endif
#ifdef SIGSYS
  { SIGSYS, "SIGSYS" },
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

#if !defined(__linux__) || !defined(_REENTRANT)
#ifdef SIGUSR1
  { SIGUSR1, "SIGUSR1" },
#endif
#ifdef SIGUSR2
  { SIGUSR2, "SIGUSR2" },
#endif
#endif /* !defined(__linux__) || !defined(_REENTRANT) */

#ifdef SIGCHLD
  { SIGCHLD, "SIGCHLD" },
#endif
#ifdef SIGCLD
  { SIGCLD, "SIGCLD" },
#endif
#ifdef SIGPWR
  { SIGPWR, "SIGPWR" },
#endif
#ifdef SIGWINCH
  { SIGWINCH, "SIGWINCH" },
#endif
#ifdef SIGURG
  { SIGURG, "SIGURG" },
#endif
#ifdef SIGIO
  { SIGIO, "SIGIO" },
#endif
#ifdef SIGSTOP
  { SIGSTOP, "SIGSTOP" },
#endif
#ifdef SIGTSTP
  { SIGTSTP, "SIGTSTP" },
#endif
#ifdef SIGCONT
  { SIGCONT, "SIGCONT" },
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
#ifdef SIGVTALRM
  { SIGVTALRM, "SIGVTALRM" },
#endif
#ifdef SIGPROF
  { SIGPROF, "SIGPROF" },
#endif
#ifdef SIGXCPU
  { SIGXCPU, "SIGXCPU" },
#endif
#ifdef SIGXFSZ
  { SIGXFSZ, "SIGXFSZ" },
#endif

#ifdef SIGSTKFLT
  { SIGSTKFLT, "SIGSTKFLT" },
#endif
#ifdef SIGPOLL
  { SIGPOLL, "SIGPOLL" },
#endif
#ifdef SIGLOST
  { SIGLOST, "SIGLOST" },
#endif
#ifdef SIGUNUSED
  { SIGUNUSED, "SIGUNUSED" },
#endif
#ifdef SIGINFO
  { SIGINFO, "SIGINFO" },
#endif
#ifdef SIGMSG
  { SIGMSG, "SIGMSG" },
#endif
#ifdef SIGDANGER
  { SIGDANGER, "SIGDANGER" },
#endif
#ifdef SIGMIGRATE
  { SIGMIGRATE, "SIGMIGRATE" },
#endif
#ifdef SIGPRE
  { SIGPRE, "SIGPRE" },
#endif
#ifdef SIGVIRT
  { SIGVIRT, "SIGVIRT" },
#endif
#ifdef SIGALRM1
  { SIGALRM1, "SIGALRM1" },
#endif
#ifdef SIGWAITING
  { SIGWAITING, "SIGWAITING" },
#endif
#ifdef SIGKAP
  { SIGKAP, "SIGKAP" },
#endif
#ifdef SIGGRANT
  { SIGGRANT, "SIGGRANT" },
#endif
#ifdef SIGRETRACT
  { SIGRETRACT, "SIGRETRACT" },
#endif
#ifdef SIGSOUND
  { SIGSOUND, "SIGSOUND" },
#endif
#ifdef SIGSAK
  { SIGSAK, "SIGSAK" },
#endif
#ifdef SIGDIL
  { SIGDIL, "SIGDIL" },
#endif
#ifdef SIG32
  { SIG32, "SIG32" },
#endif
#ifdef SIGCKPT
  { SIGCKPT, "SIGCKPT" },
#endif

#ifdef SIGPTRESCHED
  { SIGPTRESCHED, "SIGPTRESCHED" },
#endif

#ifndef _REENTRANT
#ifdef SIGWAITING
  { SIGWAITING, "SIGWAITING" },
#endif
#ifdef SIGLWP
  { SIGLWP, "SIGLWP" },
#endif
#ifdef SIGCANCEL
  { SIGCANCEL, "SIGCANCEL" },
#endif
#endif /* !_REENTRANT */

#ifdef SIGFREEZE
  { SIGFREEZE, "SIGFREEZE" },
#endif
#ifdef SIGTHAW
  { SIGTHAW, "SIGTHAW" },
#endif

  { -1, "END" } /* Notused */
};


static RETSIGTYPE receive_signal(int signum)
{
  int tmp;

  if ((signum < 0) || (signum >= MAX_SIGNALS)) {
    /* Some OSs (Solaris 2.6) send a bad signum sometimes.
     * SIGCHLD is the safest signal to substitute.
     *	/grubba 1998-05-19
     */
#ifdef SIGCHLD
    signum = SIGCHLD;
#else
    signum = 0;
#endif
  }

  tmp=firstsig+1;
  if(tmp == SIGNAL_BUFFER) tmp=0;
  if(tmp != lastsig)
  {
    sigbuf[tmp]=signum;
    firstsig=tmp;
  }
  wake_up_backend();

#ifndef SIGNAL_ONESHOT
  my_signal(signum, receive_signal);
#endif
}

/* This function is intended to work like signal(), but better :) */
void my_signal(int sig, sigfunctype fun)
{
#ifdef HAVE_SIGACTION
  {
    struct sigaction action;
    /* NOTE:
     *   sa_handler is really _funcptr._handler, and
     *   sa_sigaction is really _funcptr._sigaction,
     *   where _funcptr is a union. ie sa_handler and
     *   sa_sigaction overlap.
     */
    MEMSET((char *)&action, 0, sizeof(action));
    action.sa_handler = fun;
    sigfillset(&action.sa_mask);
    action.sa_flags = 0;
#ifdef SA_INTERRUPT
    if(fun != SIG_IGN)
      action.sa_flags |= SA_INTERRUPT;
#endif
    sigaction(sig, &action, NULL);
  }
#else
#ifdef HAVE_SIGVEC
  {
    struct sigvec action;
    MEMSET((char *)&action, 0, sizeof(action));
    action.sv_handler= fun;
    action.sv_mask=-1;
#ifdef SV_INTERRUPT
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



static int signalling=0;

static void unset_signalling(void *notused) { signalling=0; }

void check_signals(struct callback *foo, void *bar, void *gazonk)
{
  ONERROR ebuf;
#ifdef PIKE_DEBUG
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

#ifdef USE_SIGCHLD
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

      if(IS_ZERO(signal_callbacks + sigbuf[lastsig]))
      {
	switch(sigbuf[lastsig])
	{
#ifdef SIGINT
	  case SIGINT:
#endif
#ifdef SIGHUP
	  case SIGHUP:
#endif
#ifdef SIGQUIT
	  case SIGQUIT:
#endif
	    push_int(1);
	    f_exit(1);
	}
      }else{
	push_int(sigbuf[lastsig]);
	apply_svalue(signal_callbacks + sigbuf[lastsig], 1);
	pop_stack();
      }
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
#ifdef USE_SIGCHLD
    case SIGCHLD:
      func=receive_sigchild;
      break;
#endif

#ifdef SIGPIPE
    case SIGPIPE:
      func=(sigfunctype) SIG_IGN;
      break;
#endif

#ifdef SIGHUP
      case SIGHUP:
#endif
#ifdef SIGINT
      case SIGINT:
#endif
#ifdef SIGQUIT
      case SIGQUIT:
#endif
	func=receive_signal;
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
#ifdef USE_SIGCHLD
      if(signum == SIGCHLD)
	func=receive_sigchild;
#endif
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





/* Process stuff */

#if PIKE_DEBUG

char process_info[65536];

#define P_NOT_STARTED 0
#define P_RUNNING 1
#define P_DONE 2
#define P_RUNNING_AGAIN 3
#define P_DONE_AGAIN 4

void process_started(pid_t pid)
{
  if(pid < 0 || pid > 65536)
    fatal("Pid out of range: %ld\n",(long)pid);

  switch(process_info[pid])
  {
    case P_NOT_STARTED:
    case P_DONE:
      process_info[pid]++;
      break;

    case P_DONE_AGAIN:
      process_info[pid]=P_RUNNING_AGAIN;

    default:
      fatal("Process debug: Pid %ld started without stopping! (status=%d)\n",(long)pid,process_info[pid]);
  }
}

void process_done(pid_t pid)
{
  if(pid < 0 || pid > 65536)
    fatal("Pid out of range: %ld\n",(long)pid);
  switch(process_info[pid])
  {
    case P_RUNNING:
    case P_RUNNING_AGAIN:
      process_info[pid]++;
      break;

    default:
      fatal("Process debug: Unknown child %ld! (status=%d)\n",(long)pid,process_info[pid]);
  }
}

#else

#define process_started(PID)
#define process_done(PID)

#endif


#ifdef HAVE_WAITPID
#define WAITPID(PID,STATUS,OPT) waitpid(PID,STATUS,OPT)
#else
#ifdef HAVE_WAIT4
#define WAITPID(PID,STATUS,OPT) wait4( (PID),(STATUS),(OPT),0 )
#else
#define WAITPID(PID,STATUS,OPT) -1
#endif
#endif

#ifdef HAVE_WAITPID
#define MY_WAIT_ANY(STATUS,OPT) waitpid(-1,STATUS,OPT)
#else
#ifdef HAVE_WAIT3
#define MY_WAIT_ANY(STATUS,OPT) wait3((STATUS),(OPT),0 )
#else
#ifdef HAVE_WAIT4
#define MY_WAIT_ANY(STATUS,OPT) wait4(-1,(STATUS),(OPT),0 )
#else
#define MY_WAIT_ANY(STATUS,OPT) ((errno=ENOTSUP),-1)
#endif
#endif
#endif


#ifdef USE_SIGCHILD

struct wait_data {
  pid_t pid;
  WAITSTATUSTYPE status;
};

static volatile struct wait_data wait_buf[WAIT_BUFFER];
static volatile int firstwait=0;
static volatile int lastwait=0;

static RETSIGTYPE receive_sigchild(int signum)
{
  pid_t pid;
  WAITSTATUSTYPE status;
  
 try_reap_again:
  /* We carefully reap what we saw */
  pid=MY_WAIT_ANY(&status, WNOHANG);
  
  if(pid>0)
  {
    int tmp2=firstwait+1;
    if(tmp2 == WAIT_BUFFER) tmp2=0;
    if(tmp2 != lastwait)
    {
      wait_buf[tmp2].pid=pid;
      wait_buf[tmp2].status=status;
      firstwait=tmp2;
      goto try_reap_again;
    }
  }
  receive_signal(SIGCHLD);
}
#endif


#define PROCESS_UNKNOWN -1
#define PROCESS_RUNNING 0
#define PROCESS_EXITED 2

#undef THIS
#define THIS ((struct pid_status *)fp->current_storage)

#ifdef USE_PID_MAPPING
static struct mapping *pid_mapping=0;
#endif

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
#ifdef USE_PID_MAPPING
  if(pid_mapping)
  {
    struct svalue key;
    key.type=T_INT;
    key.u.integer=THIS->pid;
    map_delete(pid_mapping, &key);
  }
#endif

#ifdef __NT__
  CloseHandle(THIS->handle);
#endif
}

#ifdef USE_PID_MAPPING
static void report_child(int pid,
			 WAITSTATUSTYPE status)
{
  process_done(pid);
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
	  if(WIFEXITED(status)) {
	    p->result = WEXITSTATUS(status);
	  } else {
	    p->result=-1;
	  }
	}
      }
      map_delete(pid_mapping, &key);
    }else{
      fprintf(stderr,"report_child on unknown child: %d,%d\n",pid,status);
    }
  }
}
#endif

#ifdef USE_WAIT_THREAD
static COND_T process_status_change;
static COND_T start_wait_thread;
static MUTEX_T wait_thread_mutex;

static void do_da_lock(void)
{
  mt_lock(&wait_thread_mutex);
}

static void do_bi_do_da_lock(void)
{
#ifdef PROC_DEBUG
  fprintf(stderr, "wait thread: This is your wakeup call!\n");
#endif
  co_signal(&start_wait_thread);
  mt_unlock(&wait_thread_mutex);
}

static void *wait_thread(void *data)
{
  if(pthread_atfork(do_da_lock,do_bi_do_da_lock,0))
  {
    perror("pthread atfork");
    exit(1);
  }
  
  while(1)
  {
    WAITSTATUSTYPE status;
    int pid;

    mt_lock(&wait_thread_mutex);
    pid=MY_WAIT_ANY(&status, WNOHANG);
    
    if(pid < 0 && errno==ECHILD)
    {
#ifdef PROC_DEBUG
      fprintf(stderr, "wait_thread: sleeping\n");
#endif
      co_wait(&start_wait_thread, &wait_thread_mutex);

#ifdef PROC_DEBUG
      fprintf(stderr, "wait_thread: waking up\n");
#endif
    }

    mt_unlock(&wait_thread_mutex);

    if(pid <= 0) pid=MY_WAIT_ANY(&status, 0);

#ifdef PROC_DEBUG
    fprintf(stderr, "wait thread: pid=%d errno=%d\n",pid,errno);
#endif
    
    if(pid>0)
    {
      mt_lock(&interpreter_lock);
      report_child(pid, status);
      co_broadcast(& process_status_change);

      mt_unlock(&interpreter_lock);
      continue;
    }

    if(pid == -1)
    {
      switch(errno)
      {
	case EINTR:
	case ECHILD:
	  break;

	default:
	  fprintf(stderr,"Wait thread: waitpid returned error: %d\n",errno);
      }
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

#ifdef USE_WAIT_THREAD

  while(THIS->state == PROCESS_RUNNING)
  {
    SWAP_OUT_CURRENT_THREAD();
    co_wait( & process_status_change, &interpreter_lock);
    SWAP_IN_CURRENT_THREAD();
  }

#else

  {
    int err=0;
    while(THIS->state == PROCESS_RUNNING)
    {
      int pid;
      WAITSTATUSTYPE status;
      pid=THIS->pid;

      if(err)
      {
	struct svalue key,*s;
	key.type=T_INT;
	key.u.integer=pid;
	s=low_mapping_lookup(pid_mapping, &key);
	if(s && s->type == T_OBJECT && s->u.object == fp->current_object)
	{
	  error("Operating system failiuer: Pike lost track of a child, pid=%d, errno=%d.\n",pid,err);

	}
	else
	  error("Pike lost track of a child, pid=%d, errno=%d.\n",pid,err);
      }

      THREADS_ALLOW();
      pid=WAITPID(pid,& status,0);
      THREADS_DISALLOW();
      if(pid > -1)
      {
	report_child(pid, status);
      }else{
	switch(errno)
	{
	  case EINTR: break;

	  default:
	    err=errno;
	}
      }
	
      
      check_signals(0,0,0);
    }
  }
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

static void f_pid_status_set_priority(INT32 args)
{
  char *to;
  int r;
  get_all_args("set_priority", args, "%s", &to);
  r = set_priority( THIS->pid, to );
  pop_n_elems(args);
  push_int(r);
}
#ifdef __amigaos__

extern struct DosLibrary *DOSBase;

static BPTR get_amigados_handle(struct mapping *optional, char *name, int fd)
{
  char buf[32];
  long ext;
  struct svalue *tmp;
  BPTR b;

  if((tmp=simple_mapping_string_lookup(optional, name)))
  {
    if(tmp->type == T_OBJECT)
    {
      fd = fd_from_object(tmp->u.object);

      if(fd == -1)
	error("File for %s is not open.\n",name);
    }
  }

  if((ext = fcntl(fd, F_EXTERNALIZE, 0)) < 0)
    error("File for %s can not be externalized.\n", name);
  
  sprintf(buf, "IXPIPE:%lx", ext);

  /* It's a kind of magic... */
  if((b = Open(buf, 0x4242)) == 0)
    error("File for %s can not be internalized.\n", name);

  return b;
}

struct perishables
{
  BPTR stdin_b;
  BPTR stdout_b;
  BPTR stderr_b;
  BPTR cwd_lock;
  dynamic_buffer cmd_buf;
};

static void free_perishables(struct perishables *storage)
{
  if(storage->stdin_b!=0) Close(storage->stdin_b);
  if(storage->stdout_b!=0) Close(storage->stdout_b);
  if(storage->stderr_b!=0) Close(storage->stderr_b);
  if(storage->cwd_lock!=0)
    UnLock(storage->cwd_lock);
  low_free_buf(&storage->cmd_buf);
}

#else /* !__amigaos__ */

#ifdef __NT__

static HANDLE get_inheritable_handle(struct mapping *optional,
				     char *name,
				     int for_reading)
{
  HANDLE ret=INVALID_HANDLE_VALUE;
  struct svalue *save_stack=sp;
  struct svalue *tmp;
  if((tmp=simple_mapping_string_lookup(optional, name)))
  {
    if(tmp->type == T_OBJECT)
    {
      INT32 fd=fd_from_object(tmp->u.object);

      if(fd == -1)
	error("File for %s is not open.\n",name);

#if 0
      {
	int q;
	for(q=0;q<MAX_OPEN_FILEDESCRIPTORS;q++)
	{
	  if(fd_type[q]<-1)
	  {
	    DWORD flags;
	    fprintf(stderr,"%3d: %d %08x",q,fd_type[q],da_handle[q],flags);
	    GetHandleInformation((HANDLE)da_handle[q],&flags);
	    if(flags & HANDLE_FLAG_INHERIT)
	      fprintf(stderr," inheritable");
	    if(flags & HANDLE_FLAG_PROTECT_FROM_CLOSE)
	      fprintf(stderr," non-closable");
	    fprintf(stderr,"\n");
	  }
	}
      }
#endif


      if(!(fd_query_properties(fd, 0) & fd_INTERPROCESSABLE))
      {
	void create_proxy_pipe(struct object *o, int for_reading);
	
	create_proxy_pipe(tmp->u.object, for_reading);
	fd=fd_from_object(sp[-1].u.object);
	
	if(fd == -1)
	  error("Proxy thread creation failed for %s.\n",name);
      }
      
      
      if(!DuplicateHandle(GetCurrentProcess(),
			    (HANDLE)da_handle[fd],
			    GetCurrentProcess(),
			    &ret,
			    NULL,
			    1,
			    DUPLICATE_SAME_ACCESS))
	  /* This could cause handle-leaks */
	  error("Failed to duplicate handle %d.\n",GetLastError());
    }
  }
  pop_n_elems(sp-save_stack);
  return ret;
}
#endif

#ifndef __NT__

#ifdef HAVE_SETRLIMIT
#include <sys/time.h>
#include <sys/resource.h>
struct plimit
{
  int resource;
  struct rlimit rlp;
  struct plimit *next;
};
#endif

struct perishables
{
  char **env;
  char **argv;

  int disabled;
#ifdef HAVE_SETRLIMIT
  struct plimit *limits;
#endif

#ifdef HAVE_SETGROUPS
  gid_t *wanted_gids;
  struct array *wanted_gids_array;
  int num_wanted_gids;
#endif
};

static void free_perishables(struct perishables *storage)
{
  if (storage->disabled) {
    exit_threads_disable(NULL);
  }

  if(storage->env) free( storage->env );
  if(storage->argv) free( storage->argv );
  while(storage->limits) 
  {
    struct plimit *n = storage->limits->next;
    free( storage->limits );
    storage->limits = n;
  }

#ifdef HAVE_SETGROUPS
  if(storage->wanted_gids) free(storage->wanted_gids);
  if(storage->wanted_gids_array) free_array(storage->wanted_gids_array);
#endif
}

#endif

#endif

#if !defined(__NT__) && !defined(__amigaos__)

extern int pike_make_pipe(int *);

/*
 * Errors that can be generated by the child process
 */

#define PROCE_CHDIR		1
#define PROCE_DUP2		2
#define PROCE_SETGID		3
#define PROCE_SETGROUPS		4
#define PROCE_GETPWUID		5
#define PROCE_INITGROUPS	6
#define PROCE_SETUID		7
#define PROCE_EXEC		8

#define PROCERROR(err, id)	do { int _l, _i; \
    buf[0] = err; buf[1] = errno; buf[2] = id; \
    for(_i = 0; _i < 3; _i += _l) { \
      while (((_l = write(control_pipe[1], buf + _i, 3 - _i)) < 0) && \
             (errno == EINTR)) \
        ; \
      if (_l < 0) break; \
    } \
    close(control_pipe[1]); \
    exit(99); \
  } while(0)

#endif /* !__NT__ && !__amigaos__ */

#ifdef HAVE___PRIOCNTL
#undef PC
# include <sys/priocntl.h>
# include <sys/rtpriocntl.h>
# include <sys/tspriocntl.h>
#else
# ifdef HAVE_SCHED_SETSCHEDULER
#  include <sched.h>
# endif
#endif

static int set_priority( int pid, char *to )
{
  int prilevel = 0;

  /* Yes, a char*. We call this function from create_process and other similar
     places, so a struct pike_string * is not really all that good an idea here
  */
  if(!strcmp( to, "realtime" ) ||
     !strcmp( to, "highest" ))
    prilevel = 3;
  if(!strcmp( to, "higher" ))
    prilevel = 2;
  else if(!strcmp( to, "high" ))
    prilevel = 1;
  else if(!strcmp( to, "low" ))
    prilevel = -1;
  else if(!strcmp( to, "lowest" ))
    prilevel = -2;
#ifdef HAVE___PRIOCNTL
  if(!pid) pid = getpid();
  if( prilevel > 1 )
  {
    /* Time to get tricky :-) */
    struct {
      id_t pc_cid;
      pri_t rt_pri;
      ulong rt_tqsecs;
      long rt_tqnsecs;
      long padding[10];
    }  params;

    struct {
      id_t pc_cid;
      char pc_clname[PC_CLNMSZ];
      short rt_maxpri;
      short _pad;
      long pad2[10];
    } foo;

    MEMSET(&params, sizeof(params), 0);
    MEMSET(&foo, sizeof(foo), 0);

    strcpy(foo.pc_clname, "RT");
    if( priocntl((idtype_t)0, (id_t)0, PC_GETCID, (void *)(&foo)) == -1)
      return 0;
    params.pc_cid = foo.pc_cid;
    params.rt_pri = prilevel == 3 ? foo.rt_maxpri : 0;
    params.rt_tqsecs = 1;
    params.rt_tqnsecs = 0;
    return priocntl(P_PID, (id_t)pid, PC_SETPARMS, (void *)(&params)) != -1;
  } else {
    struct {
      id_t pc_cid;
      pri_t ts_uprilim;
      pri_t ts_upri;
      long padding[12];
    }  params;
    struct {
      id_t pc_cid;
      char pc_clname[PC_CLNMSZ];
      short ts_maxupri;
      short _pad;
      long pad2[10];
    } foo;

    MEMSET(&params, sizeof(params), 0);
    MEMSET(&foo, sizeof(foo), 0);
    strcpy(foo.pc_clname, "TS");
    if( priocntl((idtype_t)0, (id_t)0, PC_GETCID, (void *)(&foo)) == -1)
      return 0;
    params.pc_cid = foo.pc_cid;
    params.ts_upri = TS_NOCHANGE;
    params.ts_uprilim = prilevel*foo.ts_maxupri/2;
    return priocntl(P_PID, (id_t)pid, PC_SETPARMS, (void *)(&params)) != -1;
  }
#else
#ifdef HAVE_SCHED_SETSCHEDULER
  if( prilevel == 3 )
  {
    struct sched_param param;
    MEMSET(&param, 0, sizeof(param));
    param.sched_priority = sched_get_priority_max( SCHED_FIFO );
    return !sched_setscheduler( pid, SCHED_FIFO, &param );
  } else {
#ifdef SCHED_RR
    struct sched_param param;
    int class = SCHED_OTHER;
    MEMSET(&param, 0, sizeof(param));
    if(prilevel == 2)
    {
      class = SCHED_RR;
      prilevel = -2; // lowest RR priority...
      param.sched_priority = sched_get_priority_min( class )+
        (sched_get_priority_max( class )-
         sched_get_priority_min( class ))/3 * (prilevel+2);
      return !sched_setscheduler( pid, class, &param );
    } 
#endif
#ifdef HAVE_SETPRIORITY
    errno = 0;
    return setpriority( PRIO_PROCESS, pid, -prilevel*10 )!=-1 || errno==0;
#else
    param.sched_priority = sched_get_priority_min( class )+
      (sched_get_priority_max( class )-
       sched_get_priority_min( class ))/3 * (prilevel+2);
    return !sched_setscheduler( pid, class, &param );
#endif
  }
#else
#ifdef HAVE_SETPRIORITY
  {
    if(prilevel == 3) 
      prilevel = 2;
    errno = 0;
    return setpriority( PRIO_PROCESS, pid, -prilevel*10 )!=-1 || errno==0;
  }
#else
#ifdef HAVE_NICE
  if(!pid || pid == getpid())
  {
    errno=0;
    return !(nice( -prilevel*10 - nice(0) ) != -1) || errno!=EPERM;
  }
#endif
#endif
#endif
#endif
  return 0;
}

void f_set_priority( INT32 args )
{
  int pid;
  char *plevel;
  if(args == 1)
  {
    pid = 0;
    get_all_args( "set_priority", args, "%s", &plevel );
  } else if(args == 2) {
    get_all_args( "set_priority", args, "%s%d", &plevel, &pid );
  }
  pid = set_priority( pid, plevel );
  pop_n_elems(args);
  push_int( pid );
}


/*
 * create_process(string *arguments, mapping optional_data);
 *
 * optional_data:
 *
 *   stdin	object(files.file)
 *   stdout	object(files.file)
 *   stderr	object(files.file)
 *   cwd	string
 *   env	mapping(string:string)
 *   priority   string
 *               "highest"                      realtime, or similar
 *               "high"
 *               "normal"                       the normal execution level
 *               "low"
 *               "lowest*                       the lowest possible priority
 *
 *
 * only on UNIX:
 *
 *   uid		int|string
 *   gid		int|string
 *   nice		int
 *   noinitgroups	int
 *   setgroups		array(int|string)
 *   keep_signals	int
 *   rlimit             mapping(string:int|array(int|string)|mapping(string:int|string)|string)
 *                         There are two values for each limit, the
 *                         soft limit and the hard limit. Processes
 *                         that does not have UID 0 may not raise the
 *                         hard limit, and the soft limit may never be
 *                         increased over the hard limit.
 *                         value is:
 *                            integer     -> sets cur, max left as it is.
 *                            mapping     -> ([ "hard":int, "soft":int ])
 *                                           Both values are optional.
 *                            array       -> ({ hard, soft }), 
 *                                            both can be set to the string
 *                                            "unlimited". 
 *                                            a value of -1 means 
 *                                            'keep the old value'
 *                            "unlimited" ->  set both to unlimited
 * 
 *                         the limits are
 *                            core          core file size
 *                            cpu           cpu time, in seconds
 *                            fsize         file size, in bytes
 *                            nofiles       the number of file descriptors
 * 
 *                            stack         stack size in bytes
 *                            data          heap (brk, malloc) size
 *                            map_mem       mmap() and heap size
 *                            mem           mmap, heap and stack size
 *
 *
 * FIXME:
 *   Support for setresgid().
 */
#ifdef HAVE_SETRLIMIT
static void internal_add_limit( struct perishables *storage, 
                                char *limit_name,
                                int limit_resource,
                                struct svalue *limit_value )
{
  struct rlimit ol;
  struct plimit *l = NULL;
#ifndef RLIM_SAVED_MAX
  getrlimit( limit_resource, &ol );
#else
  ol.rlim_max = RLIM_SAVED_MAX;
  ol.rlim_cur = RLIM_SAVED_CUR;
#endif

  if(limit_value->type == T_INT)
  {
    l = malloc(sizeof( struct plimit ));
    l->rlp.rlim_max = ol.rlim_max;
    l->rlp.rlim_cur = limit_value->u.integer;
  } else if(limit_value->type == T_MAPPING) {
    struct svalue *tmp3;
    l = malloc(sizeof( struct plimit ));
    if((tmp3=simple_mapping_string_lookup(limit_value->u.mapping, "soft"))) {
      if(tmp3->type == T_INT)
        l->rlp.rlim_cur=
          tmp3->u.integer>=0?tmp3->u.integer:ol.rlim_cur;
      else
        l->rlp.rlim_cur = RLIM_INFINITY;
    } else
      l->rlp.rlim_cur = ol.rlim_cur;
    if((tmp3=simple_mapping_string_lookup(limit_value->u.mapping, "hard"))) {
      if(tmp3->type == T_INT)
        l->rlp.rlim_max =
          tmp3->u.integer>=0?tmp3->u.integer:ol.rlim_max;
      else
        l->rlp.rlim_max = RLIM_INFINITY;
    } else
      l->rlp.rlim_max = ol.rlim_max;
  } else if(limit_value->type == T_ARRAY && limit_value->u.array->size == 2) {
    l = malloc(sizeof( struct plimit ));
    if(limit_value->u.array->item[0].type == T_INT)
      l->rlp.rlim_max = limit_value->u.array->item[0].u.integer;
    else
      l->rlp.rlim_max = ol.rlim_max;
    if(limit_value->u.array->item[1].type == T_INT)
      l->rlp.rlim_cur = limit_value->u.array->item[1].u.integer;
    else
      l->rlp.rlim_max = ol.rlim_cur;
  } else if(limit_value->type == T_STRING) {
    l = malloc(sizeof(struct plimit));
    l->rlp.rlim_max = RLIM_INFINITY;
    l->rlp.rlim_cur = RLIM_INFINITY;
  }
  if(l)
  {
    l->resource = limit_resource;
    l->next = storage->limits;
    storage->limits = l;
  }
}
#endif

void f_create_process(INT32 args)
{
  struct array *cmd=0;
  struct mapping *optional=0;
  struct svalue *tmp;
  int e;

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
    void *env=NULL;

    /* Quote command to allow all characters (if possible) */
    /* Damn! NT doesn't have quoting! The below code attempts to
     * fake it
     */
    {
      int e,d;
      dynamic_buffer buf;
      initialize_buf(&buf);
      for(e=0;e<cmd->size;e++)
      {
	int quote;
	if(e)
	{
	  low_my_putchar(' ', &buf);
	  quote=STRCHR(ITEM(cmd)[e].u.string->str,'"') ||
	    STRCHR(ITEM(cmd)[e].u.string->str,' ');
	}else{
	  quote=0;
	}

	if(quote)
	{
	  low_my_putchar('"', &buf);

	  for(d=0;d<ITEM(cmd)[e].u.string->len;d++)
	  {
	    switch(ITEM(cmd)[e].u.string->str[d])
	    {
	      case '"':
	      case '\\':
		low_my_putchar('\\', &buf);
	      default:
		low_my_putchar(ITEM(cmd)[e].u.string->str[d], &buf);
	    }
	  }
	  low_my_putchar('"', &buf);
	}else{
	  low_my_binary_strcat(ITEM(cmd)[e].u.string->str,
			       ITEM(cmd)[e].u.string->len,
			       &buf);
	}
      }
      low_my_putchar(0, &buf);
      command_line=(TCHAR *)buf.s.str;
    }


    
    GetStartupInfo(&info);

    info.dwFlags|=STARTF_USESTDHANDLES;
    info.hStdInput=GetStdHandle(STD_INPUT_HANDLE);
    info.hStdOutput=GetStdHandle(STD_OUTPUT_HANDLE);
    info.hStdError=GetStdHandle(STD_ERROR_HANDLE);

    if(optional)
    {
      if(tmp=simple_mapping_string_lookup(optional, "cwd"))
	if(tmp->type == T_STRING)
	  dir=(TCHAR *)STR0(tmp->u.string);

      t1=get_inheritable_handle(optional, "stdin",1);
      if(t1!=INVALID_HANDLE_VALUE) info.hStdInput=t1;

      t2=get_inheritable_handle(optional, "stdout",0);
      if(t2!=INVALID_HANDLE_VALUE) info.hStdOutput=t2;

      t3=get_inheritable_handle(optional, "stderr",0);
      if(t3!=INVALID_HANDLE_VALUE) info.hStdError=t3;

	if((tmp=simple_mapping_string_lookup(optional, "env")))
	{
	  if(tmp->type == T_MAPPING)
	  {
	    struct mapping *m=tmp->u.mapping;
	    struct array *i,*v;
	    int ptr=0;
	    i=mapping_indices(m);
	    v=mapping_indices(m);

	    for(e=0;e<i->size;e++)
	    {
	      if(ITEM(i)[e].type == T_STRING && ITEM(v)[e].type == T_STRING)
	      {
		check_stack(3);
		ref_push_string(ITEM(i)[e].u.string);
		push_string(make_shared_string("="));
		ref_push_string(ITEM(v)[e].u.string);
		f_add(3);
		ptr++;
	      }
	    }
	    free_array(i);
	    free_array(v);
	    push_string(make_shared_binary_string("\0\0",1));
	    f_aggregate(ptr+1);
	    push_string(make_shared_binary_string("\0\0",1));
	    o_multiply();
	    env=(void *)sp[-1].u.string->str;
	  }
	}

      /* FIX: env, cleanup */
    }

    THREADS_ALLOW_UID();
    ret=CreateProcess(filename,
		      command_line,
		      NULL,  /* process security attribute */
		      NULL,  /* thread security attribute */
		      1,     /* inherithandles */
		      0,     /* create flags */
		      env,  /* environment */
		      dir,   /* current dir */
		      &info,
		      &proc);
    THREADS_DISALLOW_UID();
    
    if(env) pop_stack();

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
#else /* !__NT__ */
#ifdef __amigaos__
  {
    ONERROR err;
    struct perishables storage;
    int d, e;

    storage.stdin_b = storage.stdout_b = storage.stderr_b = 0;
    storage.cwd_lock = 0;
    initialize_buf(&storage.cmd_buf);

    SET_ONERROR(err, free_perishables, &storage);

    for(e=0;e<cmd->size;e++)
    {
      if(e)
        low_my_putchar(' ', &storage.cmd_buf);
      if(STRCHR(STR0(ITEM(cmd)[e].u.string),'"') || STRCHR(STR0(ITEM(cmd)[e].u.string),' ')) {
        low_my_putchar('"', &storage.cmd_buf);
	for(d=0;d<ITEM(cmd)[e].u.string->len;d++)
	{
	  switch(STR0(ITEM(cmd)[e].u.string)[d])
	  {
	    case '*':
	    case '"':
	      low_my_putchar('*', &storage.cmd_buf);
            default:
	      low_my_putchar(STR0(ITEM(cmd)[e].u.string)[d], &storage.cmd_buf);
	  }
	}
        low_my_putchar('"', &storage.cmd_buf);	
      } else
        low_my_binary_strcat(STR0(ITEM(cmd)[e].u.string),
			     ITEM(cmd)[e].u.string->len,
			     &storage.cmd_buf);
    }
    low_my_putchar('\0', &storage.cmd_buf);

    if((tmp=simple_mapping_string_lookup(optional, "cwd")))
      if(tmp->type == T_STRING)
        if((storage.cwd_lock=Lock((char *)STR0(tmp->u.string), ACCESS_READ))==0)
	  error("Failed to lock cwd \"%s\".\n", STR0(tmp->u.string));

    storage.stdin_b = get_amigados_handle(optional, "stdin", 0);
    storage.stdout_b = get_amigados_handle(optional, "stdout", 1);
    storage.stderr_b = get_amigados_handle(optional, "stderr", 2);

#ifdef PROC_DEBUG
    fprintf(stderr, "SystemTags(\"%s\", SYS_Asynch, TRUE, NP_Input, %p, NP_Output, %p, NP_Error, %p, %s, %p, TAG_END);\n",
	storage.cmd_buf.s.str, storage.stdin_b, storage.stdout_b, storage.stderr_b,
	(storage.cwd_lock!=0? "NP_CurrentDir":"TAG_IGNORE"),
	storage.cwd_lock);
#endif /* PROC_DEBUG */

    if(SystemTags(storage.cmd_buf.s.str, SYS_Asynch, TRUE,
		  NP_Input, storage.stdin_b, NP_Output, storage.stdout_b,
		  NP_Error, storage.stderr_b,
	          (storage.cwd_lock!=0? NP_CurrentDir:TAG_IGNORE),
		  storage.cwd_lock, TAG_END))
      error("Failed to start process (%ld).\n", IoErr());

    UNSET_ONERROR(err);

    /*

     * Ideally, these resources should be freed here.
     * But that would cause dos.library to go nutzoid, so
     * we better not...

      if(storage.cwd_lock!=0)
        UnLock(storage.cwd_lock);
      low_free_buf(&storage.cmd_buf);

    */
  }
#else /* !__amigaos__ */
  {
    struct svalue *stack_save=sp;
    ONERROR err;
    struct passwd *pw=0;
    struct perishables storage;
    int do_initgroups=1;
    uid_t wanted_uid;
    gid_t wanted_gid;
    int gid_request=0;
    int keep_signals = 0;
    pid_t pid;
    int control_pipe[2];	/* Used for communication with the child. */
    char buf[4];

    int nice_val;
    int stds[3]; /* stdin, out and err */
    char *tmp_cwd; /* to CD to */
    char *priority = NULL;

    stds[0] = stds[1] = stds[2] = 0;
    nice_val = 0;
    tmp_cwd = NULL;

    storage.env=0;
    storage.argv=0;
    storage.disabled=0;

#ifdef HAVE_SETRLIMIT
    storage.limits = 0;
#endif

#ifdef HAVE_SETGROUPS
    storage.wanted_gids=0;
    storage.wanted_gids_array=0;
#endif

#ifdef HAVE_GETEUID
    wanted_uid=geteuid();
#else
    wanted_uid=getuid();
#endif

#ifdef HAVE_GETEGID
    wanted_gid=getegid();
#else
    wanted_gid=getgid();
#endif

    SET_ONERROR(err, free_perishables, &storage);

    if(optional)
    {
      if((tmp = simple_mapping_string_lookup(optional, "priority")) &&
         tmp->type == T_STRING)
        priority = tmp->u.string->str;

      if((tmp = simple_mapping_string_lookup(optional, "gid")))
      {
	switch(tmp->type)
	{
	  case T_INT:
	    wanted_gid=tmp->u.integer;
	    gid_request=1;
	    break;
	    
#if defined(HAVE_GETGRNAM) || defined(HAVE_GETGRENT)
	  case T_STRING:
	  {
	    extern void f_getgrnam(INT32);
	    push_svalue(tmp);
	    f_getgrnam(1);
	    if(!sp[-1].type != T_ARRAY)
	      error("No such group.\n");
	    if(sp[-1].u.array->item[2].type != T_INT)
	      error("Getgrnam failed!\n");
	    wanted_gid = sp[-1].u.array->item[2].u.integer;
	    pop_stack();
	    gid_request=1;
	  }
	  break;
#endif
	  
	  default:
	    error("Invalid argument for gid.\n");
	}
      }
      
      if((tmp = simple_mapping_string_lookup( optional, "cwd" )) &&
         tmp->type == T_STRING && !tmp->u.string->size_shift)
        tmp_cwd = tmp->u.string->str;

      if((tmp = simple_mapping_string_lookup( optional, "stdin" )) &&
         tmp->type == T_OBJECT)
      {
        stds[0] = fd_from_object( tmp->u.object );
        if(stds[0] == -1)
          error("Invalid stdin file\n");
      }

      if((tmp = simple_mapping_string_lookup( optional, "stdout" )) &&
	 tmp->type == T_OBJECT)
      {
        stds[1] = fd_from_object( tmp->u.object );
        if(stds[1] == -1)
          error("Invalid stdout file\n");
      }

      if((tmp = simple_mapping_string_lookup( optional, "stderr" )) &&
	 tmp->type == T_OBJECT)
      {
        stds[2] = fd_from_object( tmp->u.object );
        if(stds[2] == -1)
          error("Invalid stderr file\n");
      }

      if((tmp = simple_mapping_string_lookup( optional, "nice"))
         && tmp->type == T_INT )
        nice_val = tmp->u.integer;


#ifdef HAVE_SETRLIMIT
      if((tmp=simple_mapping_string_lookup(optional, "rlimit")))
      {
        struct svalue *tmp2;
        if(tmp->type != T_MAPPING)
          error("Wrong type of argument for the 'rusage' option. "
                "Should be mapping.\n");

#define ADD_LIMIT(X,Y,Z) internal_add_limit(&storage,X,Y,Z);
          
#ifdef RLIMIT_CORE
      if((tmp2=simple_mapping_string_lookup(tmp->u.mapping, "core")))
        ADD_LIMIT( "core", RLIMIT_CORE, tmp2 );
#endif        
#ifdef RLIMIT_CPU
      if((tmp2=simple_mapping_string_lookup(tmp->u.mapping, "cpu")))
        ADD_LIMIT( "cpu", RLIMIT_CPU, tmp2 );
#endif        
#ifdef RLIMIT_DATA
      if((tmp2=simple_mapping_string_lookup(tmp->u.mapping, "data")))
        ADD_LIMIT( "data", RLIMIT_DATA, tmp2 );
#endif        
#ifdef RLIMIT_FSIZE
      if((tmp2=simple_mapping_string_lookup(tmp->u.mapping, "fsize")))
        ADD_LIMIT( "fsize", RLIMIT_FSIZE, tmp2 );
#endif        
#ifdef RLIMIT_NOFILE
      if((tmp2=simple_mapping_string_lookup(tmp->u.mapping, "nofile")))
        ADD_LIMIT( "nofile", RLIMIT_NOFILE, tmp2 );
#endif        
#ifdef RLIMIT_STACK
      if((tmp2=simple_mapping_string_lookup(tmp->u.mapping, "stack")))
        ADD_LIMIT( "stack", RLIMIT_STACK, tmp2 );
#endif        
#ifdef RLIMIT_VMEM
      if((tmp2=simple_mapping_string_lookup(tmp->u.mapping, "map_mem"))
         ||(tmp2=simple_mapping_string_lookup(tmp->u.mapping, "vmem")))
        ADD_LIMIT( "map_mem", RLIMIT_VMEM, tmp2 );
#endif        
#ifdef RLIMIT_AS
      if((tmp2=simple_mapping_string_lookup(tmp->u.mapping, "as"))
         ||(tmp2=simple_mapping_string_lookup(tmp->u.mapping, "mem")))
        ADD_LIMIT( "mem", RLIMIT_AS, tmp2 );
#endif        
      }
#undef ADD_LIMIT
#endif        

      
      if((tmp=simple_mapping_string_lookup(optional, "uid")))
      {
	switch(tmp->type)
	{
	  case T_INT:
	    wanted_uid=tmp->u.integer;
#if defined(HAVE_GETPWUID) || defined(HAVE_GETPWENT)
	    if(!gid_request)
	    {
	      extern void f_getpwuid(INT32);
	      push_int(wanted_uid);
	      f_getpwuid(1);

	      if(sp[-1].type==T_ARRAY)
	      {
		if(sp[-1].u.array->item[3].type!=T_INT)
		  error("Getpwuid failed!\n");
		wanted_gid = sp[-1].u.array->item[3].u.integer;
	      }
	      pop_stack();
	    }
#endif
	    break;
	    
#if defined(HAVE_GETPWNAM) || defined(HAVE_GETPWENT)
	  case T_STRING:
	  {
	    extern void f_getpwnam(INT32);
	    push_svalue(tmp);
	    f_getpwnam(1);
	    if(sp[-1].type != T_ARRAY)
	      error("No such user.\n");
	    if(sp[-1].u.array->item[2].type!=T_INT ||
	       sp[-1].u.array->item[3].type!=T_INT)
	      error("Getpwnam failed!\n");
	    wanted_uid=sp[-1].u.array->item[2].u.integer;
	    if(!gid_request)
	      wanted_gid=sp[-1].u.array->item[3].u.integer;
	    pop_stack();
	    break;
	  }
#endif
	    
	  default:
	    error("Invalid argument for uid.\n");
	}
      }

      if((tmp=simple_mapping_string_lookup(optional, "setgroups")))
      {
#ifdef HAVE_SETGROUPS
	if(tmp->type == T_ARRAY)
	{
	  storage.wanted_gids_array=tmp->u.array;
	  add_ref(storage.wanted_gids_array);
	  for(e=0;e<storage.wanted_gids_array->size;e++)
	    if(storage.wanted_gids_array->item[e].type != T_INT)
	      error("Invalid type for setgroups.\n");
	  do_initgroups=0;
	}else{
	  error("Invalid type for setgroups.\n");
	}
#else
	error("Setgroups is not available.\n");
#endif
      }


      if((tmp=simple_mapping_string_lookup(optional, "env")))
      {
	if(tmp->type == T_MAPPING)
	{
	  struct mapping *m=tmp->u.mapping;
	  struct array *i,*v;
	  int ptr=0;
	  i=mapping_indices(m);
	  v=mapping_values(m);
	  
	  storage.env=(char **)xalloc((1+m_sizeof(m)) * sizeof(char *));
	  for(e=0;e<i->size;e++)
	  {
	    if(ITEM(i)[e].type == T_STRING &&
	       ITEM(v)[e].type == T_STRING)
	    {
	      check_stack(3);
	      ref_push_string(ITEM(i)[e].u.string);
	      push_string(make_shared_string("="));
	      ref_push_string(ITEM(v)[e].u.string);
	      f_add(3);
	      storage.env[ptr++]=sp[-1].u.string->str;
	    }
	  }
	  storage.env[ptr++]=0;
	  free_array(i);
	  free_array(v);
	  }
	}

      if((tmp=simple_mapping_string_lookup(optional, "noinitgroups")))
	if(!IS_ZERO(tmp))
	  do_initgroups=0;

      if((tmp=simple_mapping_string_lookup(optional, "keep_signals")))
	keep_signals = !IS_ZERO(tmp);
    }

#ifdef HAVE_SETGROUPS

#ifdef HAVE_GETGRENT
    if(wanted_uid != getuid() && do_initgroups)
    {
      extern void f_get_groups_for_user(INT32);
      push_int(wanted_uid);
      f_get_groups_for_user(1);
      if(sp[-1].type == T_ARRAY)
      {
	storage.wanted_gids_array=sp[-1].u.array;
	sp--;
      } else {
	pop_stack();
      }
    }
#endif

    if(storage.wanted_gids_array)
    {
      int e;
      storage.wanted_gids=(gid_t *)xalloc(sizeof(gid_t) * (storage.wanted_gids_array->size + 1));
      storage.wanted_gids[0]=65534; /* Paranoia */
      for(e=0;e<storage.wanted_gids_array->size;e++)
      {
	switch(storage.wanted_gids_array->item[e].type)
	{
	  case T_INT:
	    storage.wanted_gids[e]=storage.wanted_gids_array->item[e].u.integer;
	    break;

#if defined(HAVE_GETGRENT) || defined(HAVE_GETGRNAM)
	  case T_STRING:
	  {
	    extern void f_getgrnam(INT32);
	    ref_push_string(storage.wanted_gids_array->item[e].u.string);
	    f_getgrnam(2);
	    if(sp[-1].type != T_ARRAY)
	      error("No such group.\n");

	    storage.wanted_gids[e]=sp[-1].u.array->item[2].u.integer;
	    pop_stack();
	    break;
	  }
#endif

	  default:
	    error("Gids must be integers or strings only.\n");
	}
      }
      storage.num_wanted_gids=storage.wanted_gids_array->size;
      free_array(storage.wanted_gids_array);
      storage.wanted_gids_array=0;
      do_initgroups=0;
    }
#endif /* HAVE_SETGROUPS */
    
    storage.argv=(char **)xalloc((1+cmd->size) * sizeof(char *));

    if (pike_make_pipe(control_pipe) < 0) {
      error("Failed to create child communication pipe.\n");
    }

#if 0 /* Changed to 0 by hubbe 990306 - why do we need it? */
    init_threads_disable(NULL);
    storage.disabled = 1;
#endif

#if defined(HAVE_FORK1) && defined(_REENTRANT)
    pid=fork1();
#else
    pid=fork();
#endif

    UNSET_ONERROR(err);

    if(pid == -1) {
      /*
       * fork() failed
       */

      close(control_pipe[0]);
      close(control_pipe[1]);

      free_perishables(&storage);

      error("Failed to start process.\n" "errno:%d\n", errno);
    } else if(pid) {
      process_started(pid);  /* Debug */
      /*
       * The parent process
       */

      /* Close our child's end of the pipe. */
      close(control_pipe[1]);

      free_perishables(&storage);

      pop_n_elems(sp - stack_save);


#ifdef USE_SIGCHILD
      /* FIXME: Can anything of the following throw an error? */
      if(!signal_evaluator_callback)
      {
	signal_evaluator_callback=add_to_callback(&evaluator_callbacks,
						  check_signals,
						  0,0);
      }
#endif

      THIS->pid = pid;
      THIS->state = PROCESS_RUNNING;
      ref_push_object(fp->current_object);
      push_int(pid);
      mapping_insert(pid_mapping, sp-1, sp-2);
      pop_n_elems(2);

      /* Wake up the child. */
      buf[0] = 0;
      while (((e = write(control_pipe[0], buf, 1)) < 0) && (errno == EINTR))
	;
      if(e!=1) {
	/* Paranoia in case close() sets errno. */
	int olderrno = errno;
	close(control_pipe[0]);
	error("Child process died prematurely. (e=%d errno=%d)\n",
	      e ,olderrno);
      }

      /* Wait for exec or error */
      while (((e = read(control_pipe[0], buf, 3)) < 0) && (errno == EINTR))
	;
      close(control_pipe[0]);
      if (!e) {
	/* OK! */
	pop_n_elems(args);
	push_int(0);
	return;
      } else {
	/* Something went wrong. */
	switch(buf[0]) {
	case PROCE_CHDIR:
	  error("Process.create_process(): chdir() failed. errno:%d\n",
		buf[1]);
	  break;
	case PROCE_DUP2:
	  error("Process.create_process(): dup2() failed. errno:%d\n",
		buf[1]);
	  break;
	case PROCE_SETGID:
	  error("Process.create_process(): setgid(%d) failed. errno:%d\n",
		buf[2], buf[1]);
	  break;
	case PROCE_SETGROUPS:
	  error("Process.create_process(): setgroups() failed. errno:%d\n",
		buf[1]);
	  break;
	case PROCE_GETPWUID:
	  error("Process.create_process(): getpwuid(%d) failed. errno:%d\n",
		buf[2], buf[1]);
	  break;
	case PROCE_INITGROUPS:
	  error("Process.create_process(): initgroups() failed. errno:%d\n",
		buf[1]);
	  break;
	case PROCE_SETUID:
	  error("Process.create_process(): setuid(%d) failed. errno:%d\n",
		buf[2], buf[1]);
	  break;
	case PROCE_EXEC:
	  error("Process.create_process(): exec() failed. errno:%d\n"
		"File not found?\n", buf[1]);
	  break;
	case 0:
	  /* read() probably failed. */
	default:
	  error("Process.create_process(): Child failed: %d, %d, %d!\n",
		buf[0], buf[1], buf[2]);
	  break;
	}
      }
    }else{
      /*
       * The child process
       */
      ONERROR oe;

#ifdef DECLARE_ENVIRON
      extern char **environ;
#endif
      extern void my_set_close_on_exec(int,int);
      extern void do_set_close_on_exec(void);

      /* Close our parent's end of the pipe. */
      close(control_pipe[0]);
      /* Ensure that the pipe will be closed when the child starts. */
      set_close_on_exec(control_pipe[1], 1);

      SET_ONERROR(oe, exit_on_error, "Error in create_process() child.");

      /* Wait for parent to get ready... */
      while ((( e = read(control_pipe[1], buf, 1)) < 0) && (errno == EINTR))
	;

      /* FIXME: What to do if e < 0 ? */

/* We don't call _any_ pike functions at all after this point, so
 * there is no need at all to call this callback, really.
 */

/* 
#ifdef _REENTRANT
      num_threads=1;
#endif
      call_callback(&fork_child_callback, 0); 
*/

      for(e=0;e<cmd->size;e++) storage.argv[e]=ITEM(cmd)[e].u.string->str;
      storage.argv[e]=0;

      if(storage.env) environ=storage.env;

#ifdef HAVE_SETEUID
      seteuid(0);
#else /* !HAVE_SETEUID */
#ifdef HAVE_SETRESUID
      setresuid(0,0,-1);
#endif /* HAVE_SETRESUID */
#endif /* HAVE_SETEUID */

      if (!keep_signals) 
      {
	/* Restore the signals to the defaults. */
#ifdef HAVE_SIGNAL
        for(i=0; i<60; i++)
          signal(i, SIG_DFL);
#else
#endif
      }

      if(tmp_cwd)
      {
        if( chdir( tmp_cwd ) )
        {
#ifdef PROC_DEBUG
          fprintf(stderr, "%s:%d: child: chdir(\"%s\") failed, errno=%d\n",
                  __FILE__, __LINE__, tmp_cwd, errno);
#endif /* PROC_DEBUG */
          PROCERROR(PROCE_CHDIR, 0);
        }
      }

#ifdef HAVE_NICE
      if(nice_val)
        nice(nice_val);
#endif

#ifdef HAVE_SETRLIMIT
      if(storage.limits)
      {
        struct plimit *l = storage.limits;
        while(l)
        {
          setrlimit( l->resource, &l->rlp );
          l = l->next;
        }
      }
#endif

      {
        int fd;
        for(fd=0; fd<3; fd++)
        {
          if(stds[fd])
            if(dup2(stds[fd], fd) < 0)
              PROCERROR(PROCE_DUP2, fd);
        }
/* Why? (Per) Because people might want to be able to close them! /Hubbe */
	for(fd=0; fd<3; fd++)
	  if(stds[fd] && stds[fd]>2)
	    close( stds[fd] );
      }

      if(priority)
        set_priority( 0, priority );

#ifdef HAVE_SETGID
#ifdef HAVE_GETGID
      if(wanted_gid != getgid())
#endif
      {
	if(setgid(wanted_gid))
#ifdef _HPUX_SOURCE
          /* Kluge for HP-(S)UX */
	  if(wanted_gid > 60000 && setgid(-2) && setgid(65534) && setgid(60001))
#endif
	  {
	    PROCERROR(PROCE_SETGID, (int)wanted_gid);
	  }
      }
#endif

#ifdef HAVE_SETGROUPS
      if(storage.wanted_gids)
      {
	if(setgroups(storage.num_wanted_gids, storage.wanted_gids))
	{
	  PROCERROR(PROCE_SETGROUPS,0);
	}
      }
#endif


#ifdef HAVE_SETUID
#ifdef HAVE_GETUID
      if(wanted_uid != getuid())
#endif
      {
#ifdef HAVE_INITGROUPS
	if(do_initgroups)
	{
	  int initgroupgid;
	  if(!pw) pw=getpwuid(wanted_uid);
	  if(!pw) {
	    PROCERROR(PROCE_GETPWUID, wanted_uid);
	  }
	  initgroupgid=pw->pw_gid;
	  if(initgroups(pw->pw_name, initgroupgid))
#ifdef _HPUX_SOURCE
	    /* Kluge for HP-(S)UX */
	    if((initgroupgid > 60000) &&
	       initgroups(pw->pw_name, -2) &&
	       initgroups(pw->pw_name, 65534) &&
	       initgroups(pw->pw_name, 60001))
#endif /* _HPUX_SOURCE */
	    {
#ifdef HAVE_SETGROUPS
	      gid_t x[]={ 65534 };
	      if(setgroups(0, x))
#endif /* SETGROUPS */
	      {

		PROCERROR(PROCE_INITGROUPS, 0);
	      }
	    }
	}
#endif /* INITGROUPS */
	if(setuid(wanted_uid))
	{
	  perror("setuid");

	  PROCERROR(PROCE_SETUID, (int)wanted_uid);
	}
      }
#endif /* SETUID */

#ifdef HAVE_SETEUID
      seteuid(wanted_uid);
#else /* !HAVE_SETEUID */
#ifdef HAVE_SETRESUID
      setresuid(wanted_uid, wanted_uid, -1);
#endif /* HAVE_SETRESUID */
#endif /* HAVE_SETEUID */
	
      do_set_close_on_exec();
      set_close_on_exec(0,0);
      set_close_on_exec(1,0);
      set_close_on_exec(2,0);
      
#ifdef HAVE_BROKEN_F_SETFD
      do_close_on_exec();
#endif /* HAVE_BROKEN_F_SETFD */

      execvp(storage.argv[0],storage.argv);
#ifndef HAVE_BROKEN_F_SETFD
#ifdef PROC_DEBUG
      fprintf(stderr,
	      "execvp(\"%s\", ...) failed\n"
	      "errno = %d\n",
	      storage.argv[0], errno);
#endif /* PROC_DEBUG */
      /* No way to tell about this on broken OS's :-( */
      PROCERROR(PROCE_EXEC, 0);
#endif /* HAVE_BROKEN_F_SETFD */
      exit(99);
    }
  }
#endif /* __amigaos__ */
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

#ifdef _REENTRANT
  if(num_threads >1)
    error("You cannot use fork in a multithreaded application.\n");
#endif

/*   THREADS_ALLOW_UID(); */
#if 0 && defined(HAVE_FORK1) && defined(_REENTRANT)
  /* This section is disabled, since fork1() isn't usefull if
   * you aren't about do an exec().
   * In addition any helper threads (like the wait thread) would
   * disappear, so the child whould be crippled.
   *	/grubba 1999-03-07
   */
  pid=fork1();
#else
  pid=fork();
#endif
/*  THREADS_DISALLOW_UID(); */

  if(pid==-1) {
    error("Fork failed\n"
	  "errno: %d\n", errno);
  }

  if(pid)
  {
    struct pid_status *p;

#ifdef USE_SIGCHILD
    if(!signal_evaluator_callback)
    {
      signal_evaluator_callback=add_to_callback(&evaluator_callbacks,
						check_signals,
						0,0);
    }
#endif

    o=low_clone(pid_status_program);
    call_c_initializers(o);
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
  int signum;
  int pid;
  int res;

  if(args < 2)
    error("Too few arguments to kill().\n");

  switch(sp[-args].type)
  {
  case T_INT:
    pid = sp[-args].u.integer;
    break;

  default:
    error("Bad argument 1 to kill().\n");
  }
    
  if(sp[1-args].type != T_INT)
    error("Bad argument 1 to kill().\n");

  signum = sp[1-args].u.integer;

  THREADS_ALLOW_UID();
  res = !kill(pid, signum);
  THREADS_DISALLOW_UID();

  check_signals(0,0,0);
  pop_n_elems(args);
  push_int(res);
}

#else

#ifdef __NT__
#define HAVE_KILL
void f_kill(INT32 args)
{
  HANDLE proc=INVALID_HANDLE_VALUE;

  if(args < 2)
    error("Too few arguments to kill().\n");

  switch(sp[-args].type)
  {
  case T_INT:
    proc=OpenProcess(PROCESS_TERMINATE,
		     0,
		     sp[-args].u.integer);
    if(proc==INVALID_HANDLE_VALUE)
    {
      errno=EPERM;
      pop_n_elems(args);
      push_int(-1);
      return;
    }
    break;

  case T_OBJECT:
  {
    INT32 pid;
    struct pid_status *p;
    if((p=(struct pid_status *)get_storage(sp[-args].u.object,
					  pid_status_program)))
    {
      proc=p->handle;
      break;
    }
  }

  default:
    error("Bad argument 1 to kill().\n");
  }
    
  if(sp[1-args].type != T_INT)
    error("Bad argument 1 to kill().\n");

  switch(sp[1-args].u.integer)
  {
    case SIGKILL:
    {
      int i=TerminateProcess(proc,0xff)?0:-1;
      pop_n_elems(args);
      push_int(i);
      check_signals(0,0,0);
      break;
    }
      
    default:
      errno=EINVAL;
      pop_n_elems(args);
      push_int(-1);
      break;
  }
}
#endif

#endif



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

#if defined(HAVE_UALARM) || defined(HAVE_SETITIMER)
static void f_ualarm(INT32 args)
{
#ifndef HAVE_UALARM
  struct itimerval new, old;
#endif /* !HAVE_UALARM */
  long useconds;

  if(args < 1)
    error("Too few arguments to ualarm()\n");

  if(sp[-args].type != T_INT)
    error("Bad argument 1 to ualarm()\n");

  useconds=sp[-args].u.integer;

  pop_n_elems(args);

#ifdef HAVE_UALARM
#ifdef UALARM_TAKES_TWO_ARGS
  push_int(ualarm(useconds,0));
#else
  push_int(ualarm(useconds));
#endif
#else /* !HAVE_UALARM */
  /*
   * Use setitimer instead.
   */
  new.it_value.tv_sec = useconds / 1000000;
  new.it_value.tv_usec = useconds % 1000000;
  new.it_interval.tv_sec = 0;
  new.it_interval.tv_usec = 0;

  if (!setitimer(ITIMER_REAL, &new, &old)) {
    push_int(old.it_value.tv_usec + old.it_value.tv_sec * 1000000);
  } else {
    push_int(-1);
  }
#endif /* HAVE_UALARM */
}
#endif /* HAVE_UALARM || HAVE_SETITIMER */

#ifdef PIKE_DEBUG
static RETSIGTYPE fatal_signal(int signum)
{
  my_signal(signum,SIG_DFL);
  fatal("Fatal signal (%s) recived.\n",signame(signum));
}
#endif

void init_signals(void)
{
  int e;
#ifdef USE_SIGCHLD
  my_signal(SIGCHLD, receive_sigchild);
#endif

#ifdef USE_WAIT_THREAD
  {
    THREAD_T foo;
    co_init(& process_status_change);
    co_init(& start_wait_thread);
    mt_init(& wait_thread_mutex);
    th_create_small(&foo,wait_thread,0);
    my_signal(SIGCHLD, SIG_DFL);
  }
#endif

#ifdef SIGPIPE
  my_signal(SIGPIPE, SIG_IGN);
#endif

#ifdef PIKE_DEBUG
#ifdef SIGSEGV
/*  my_signal(SIGSEGV, fatal_signal); */
#endif
#ifdef SIGBUS
/*  my_signal(SIGBUS, fatal_signal); */
#endif
#endif

#ifdef IGNORE_SIGFPE
  my_signal(SIGFPE, SIG_IGN);
#endif

#ifdef SIGINT
  my_signal(SIGINT, receive_signal);
#endif

#ifdef SIGHUP
  my_signal(SIGHUP, receive_signal);
#endif

#ifdef SIGQUIT
  my_signal(SIGQUIT, receive_signal);
#endif

  for(e=0;e<MAX_SIGNALS;e++)
    signal_callbacks[e].type=T_INT;

  firstsig=lastsig=0;

  if(!signal_evaluator_callback)
  {
    signal_evaluator_callback=add_to_callback(&evaluator_callbacks,
					      check_signals,
					      0,0);
  }

#ifdef USE_PID_MAPPING
  pid_mapping=allocate_mapping(2);

#ifndef USE_WAIT_THREAD
  pid_mapping->flags|=MAPPING_FLAG_WEAK;
#endif
#endif

  start_new_program();
  ADD_STORAGE(struct pid_status);
  set_init_callback(init_pid_status);
  set_exit_callback(exit_pid_status);
  /* function(string:int) */
  ADD_FUNCTION("set_priority",f_pid_status_set_priority,tFunc(tStr,tInt),0);
  /* function(:int) */
  ADD_FUNCTION("wait",f_pid_status_wait,tFunc(,tInt),0);
  /* function(:int) */
  ADD_FUNCTION("status",f_pid_status_status,tFunc(,tInt),0);
  /* function(:int) */
  ADD_FUNCTION("pid",f_pid_status_pid,tFunc(,tInt),0);
  /* function(array(string),void|mapping(string:mixed):object) */
  ADD_FUNCTION("create",f_create_process,tFunc(tArr(tStr) tOr(tVoid,tMap(tStr,tMix)),tObj),0);
  pid_status_program=end_program();
  add_program_constant("create_process",pid_status_program,0);

  
/* function(string,int|void:int) */
  ADD_EFUN("set_priority",f_set_priority,tFunc(tStr tOr(tInt,tVoid),tInt),
           OPT_SIDE_EFFECT);
  
/* function(int,mixed|void:void) */
  ADD_EFUN("signal",f_signal,tFunc(tInt tOr(tMix,tVoid),tVoid),OPT_SIDE_EFFECT);
#ifdef HAVE_KILL
  
/* function(int|object,int:int) */
  ADD_EFUN("kill",f_kill,tFunc(tOr(tInt,tObj) tInt,tInt),OPT_SIDE_EFFECT);
#endif
#ifdef HAVE_FORK
  
/* function(void:object) */
  ADD_EFUN("fork",f_fork,tFunc(tVoid,tObj),OPT_SIDE_EFFECT);
#endif

  
/* function(int:string) */
  ADD_EFUN("signame",f_signame,tFunc(tInt,tStr),0);
  
/* function(string:int) */
  ADD_EFUN("signum",f_signum,tFunc(tStr,tInt),0);
  
/* function(:int) */
  ADD_EFUN("getpid",f_getpid,tFunc(,tInt),0);
#ifdef HAVE_ALARM
  
/* function(int:int) */
  ADD_EFUN("alarm",f_alarm,tFunc(tInt,tInt),OPT_SIDE_EFFECT);
#endif
#if defined(HAVE_UALARM) || defined(HAVE_SETITIMER)
  
/* function(int:int) */
  ADD_EFUN("ualarm",f_ualarm,tFunc(tInt,tInt),OPT_SIDE_EFFECT);
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


