/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
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

RCSID("$Id: signal_handler.c,v 1.82 1998/07/19 22:52:17 grubba Exp $");

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

/* Added so we are able to patch older versions of Pike. */
#ifndef add_ref
#define add_ref(X)	((X)->refs++)
#endif /* add_ref */

extern int fd_from_object(struct object *o);

static struct svalue signal_callbacks[MAX_SIGNALS];

static unsigned char sigbuf[SIGNAL_BUFFER];
static int firstsig, lastsig;
static struct callback *signal_evaluator_callback =0;

#ifndef __NT__
struct wait_data {
  pid_t pid;
  WAITSTATUSTYPE status;
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

#ifdef __NT__

#ifndef SIGKILL
#define SIGKILL 9
#endif

#endif /* __NT__ */

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

#ifndef __NT__
  if(signum==SIGCHLD)
  {
    pid_t pid;
    WAITSTATUSTYPE status;

  try_reap_again:
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
      if(tmp2 == WAIT_BUFFER) tmp2=0;
      if(tmp2 != lastwait)
      {
	wait_buf[tmp2].pid=pid;
	wait_buf[tmp2].status=status;
	firstwait=tmp2;
	goto try_reap_again;
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
			 WAITSTATUSTYPE status)
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
	  if(WIFEXITED(status)) {
	    p->result = WEXITSTATUS(status);
	  } else {
	    p->result=-1;
	  }
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
  {
    int err=0;
    while(THIS->state == PROCESS_RUNNING)
    {
      int pid;
      WAITSTATUSTYPE status;
      pid=THIS->pid;

      if(err)
	error("Pike lost track of a child, pid=%d, errno=%d.\n",pid,err);

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

struct perishables
{
  char **env;
  char **argv;
  struct pike_string *nice_s;
  struct pike_string *cwd_s;
  struct pike_string *stdin_s;
  struct pike_string *stdout_s;
  struct pike_string *stderr_s;
  struct pike_string *keep_signals_s;
  int disabled;
#ifdef HAVE_SETGROUPS
  gid_t *wanted_gids;
  struct array *wanted_gids_array;
  int num_wanted_gids;
#endif
};

static void free_perishables(struct perishables *storage)
{
  if (storage->disabled)
    exit_threads_disable(NULL);

  if(storage->env) free((char *)storage->env);

  if(storage->argv) free((char *)storage->argv);
  if(storage->nice_s) free_string(storage->nice_s);
  if(storage->cwd_s) free_string(storage->cwd_s);
  if(storage->stdin_s) free_string(storage->stdin_s);
  if(storage->stdout_s) free_string(storage->stdout_s);
  if(storage->stderr_s) free_string(storage->stderr_s);
  if(storage->keep_signals_s) free_string(storage->keep_signals_s);

#ifdef HAVE_SETGROUPS
  if(storage->wanted_gids) free((char *)storage->wanted_gids);

  if(storage->wanted_gids_array) free_array(storage->wanted_gids_array);
  
#endif
}

#endif


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
 *
 * only on UNIX:
 *
 *   uid		int|string
 *   gid		int|string
 *   nice		int
 *   noinitgroups	int
 *   setgroups		array(int|string)
 *   keep_signals	int
 *
 * FIXME:
 *   Support for setresgid().
 */
void f_create_process(INT32 args)
{
  struct array *cmd=0;
  struct mapping *optional=0;
  struct svalue *tmp;
  int e;

  if(!args) return;	/* FIXME: Why? */

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
	  dir=convert_string(tmp->u.string->str, tmp->u.string->len);

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
    
    if(dir) free((char *)dir);
    if(command_line) free((char *)command_line);
    if(filename) free((char *)filename);
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

    storage.env=0;
    storage.argv=0;
    storage.disabled=0;
    MAKE_CONSTANT_SHARED_STRING(storage.nice_s, "nice");
    MAKE_CONSTANT_SHARED_STRING(storage.cwd_s, "cwd");
    MAKE_CONSTANT_SHARED_STRING(storage.stdin_s, "stdin");
    MAKE_CONSTANT_SHARED_STRING(storage.stdout_s, "stdout");
    MAKE_CONSTANT_SHARED_STRING(storage.stderr_s, "stderr");
    MAKE_CONSTANT_SHARED_STRING(storage.keep_signals_s, "keep_signals");

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
#ifdef PROC_DEBUG
    fprintf(stderr, "%s:%d: wanted_gid=%d\n", __FILE__, __LINE__, wanted_gid);
#endif /* PROC_DEBUG */

    SET_ONERROR(err, free_perishables, &storage);

    if(optional)
    {
      if((tmp = simple_mapping_string_lookup(optional, "gid")))
      {
	switch(tmp->type)
	{
	  case T_INT:
	    wanted_gid=tmp->u.integer;
#ifdef PROC_DEBUG
    fprintf(stderr, "%s:%d: wanted_gid=%d\n", __FILE__, __LINE__, wanted_gid);
#endif /* PROC_DEBUG */
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
#ifdef PROC_DEBUG
    fprintf(stderr, "%s:%d: wanted_gid=%d\n", __FILE__, __LINE__, wanted_gid);
#endif /* PROC_DEBUG */
	    pop_stack();
	    gid_request=1;
	  }
	  break;
#endif
	  
	  default:
	    error("Invalid argument for gid.\n");
	}
      }
      
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
#ifdef PROC_DEBUG
    fprintf(stderr, "%s:%d: wanted_gid=%d\n", __FILE__, __LINE__, wanted_gid);
#endif /* PROC_DEBUG */
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
#ifdef PROC_DEBUG
    fprintf(stderr, "%s:%d: wanted_gid=%d\n", __FILE__, __LINE__, wanted_gid);
#endif /* PROC_DEBUG */
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
#ifdef PROC_DEBUG
	fprintf(stderr, "Use setgroups\n");
#endif /* PROC_DEBUG */
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
#ifdef PROC_DEBUG
      fprintf(stderr, "Creating wanted_gids_array\n");
#endif /* PROC_DEBUG */
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
#ifdef PROC_DEBUG
    fprintf(stderr, "Creating wanted_gids (size=%d)\n", storage.wanted_gids_array->size);
#endif /* PROC_DEBUG */
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
#ifdef PROC_DEBUG
	fprintf(stderr, "GROUP #%d is %ld\n", e, storage.wanted_gids[e]);
#endif /* PROC_DEBUG */
      }
      storage.num_wanted_gids=storage.wanted_gids_array->size;
      free_array(storage.wanted_gids_array);
      storage.wanted_gids_array=0;
      do_initgroups=0;
    }
#endif /* HAVE_SETGROUPS */
    
    storage.argv=(char **)xalloc((1+cmd->size) * sizeof(char *));

#if 1
    init_threads_disable(NULL);
    storage.disabled = 1;
#endif

#if defined(HAVE_FORK1) && defined(_REENTRANT)
    pid=fork1();
#else
    pid=fork();
#endif

    UNSET_ONERROR(err);

    if(pid==-1) {
      free_perishables(&storage);

      error("Failed to start process.\n"
	    "errno:%d\n", errno);
    } else if(pid) {
      free_perishables(&storage);

      pop_n_elems(sp - stack_save);

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
      push_int(0);
    }else{
      ONERROR oe;

#ifdef DECLARE_ENVIRON
      extern char **environ;
#endif
      extern void my_set_close_on_exec(int,int);
      extern void do_set_close_on_exec(void);

      SET_ONERROR(oe, exit_on_error, "Error in create_process() child.");

#ifdef _REENTRANT
      /* forked copy. there is now only one thread running, this one. */
      num_threads=1;
#endif
      call_callback(&fork_child_callback, 0);

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

      if (!keep_signals) {
	/* Restore the signals to the defaults. */
      }

      if(optional)
      {
	int toclose[3];
	int fd;

	if((tmp=low_mapping_string_lookup(optional, storage.cwd_s)))
	  if(tmp->type == T_STRING)
	    if(chdir(tmp->u.string->str))
	      exit(69);

#ifdef HAVE_NICE
	if ((tmp=low_mapping_string_lookup(optional, storage.nice_s))) {
	  if (tmp->type == T_INT) {
	    nice(tmp->u.integer);
	  }
	}
#endif /* HAVE_NICE */

	for(fd=0;fd<3;fd++)
	{
	  struct pike_string *fdname;
	  toclose[fd]=-1;
	  switch(fd)
	  {
	    case 0: fdname=storage.stdin_s; break;
	    case 1: fdname=storage.stdout_s; break;
	    default: fdname=storage.stderr_s; break;
	  }
	  
	  if((tmp=low_mapping_string_lookup(optional, fdname)))
	  {
	    if(tmp->type == T_OBJECT)
	    {
	      INT32 f=fd_from_object(tmp->u.object);
	      if(f != -1 && fd!=f)
	      {
		if(dup2(toclose[fd]=f, fd) < 0)
		{
		  exit(67);
		}
	      }
	    }
	  }
	}

	for(fd=0;fd<3;fd++)
	{
	  int f2;
	  if(toclose[fd]<3) continue;

	  for(f2=0;f2<fd;f2++)
	    if(toclose[fd]==toclose[f2])
	      break;

	  if(f2 == fd)
	  {
	    close(toclose[fd]);
	  }
	}

	/* Left to do: cleanup? */
      }

#ifdef HAVE_SETGID
#ifdef HAVE_GETGID
      if(wanted_gid != getgid())
#endif
      {
#ifdef PROC_DEBUG
    fprintf(stderr, "%s:%d: wanted_gid=%d\n", __FILE__, __LINE__, wanted_gid);
#endif /* PROC_DEBUG */
	if(setgid(wanted_gid))
#ifdef _HPUX_SOURCE
          /* Kluge for HP-(S)UX */
	  if(wanted_gid > 60000 && setgid(-2) && setgid(65534) && setgid(60001))
#endif
	    exit(77);
      }
#endif

#ifdef HAVE_SETGROUPS
      if(storage.wanted_gids)
      {
#ifdef PROC_DEBUG
    fprintf(stderr, "Calling setgroups()\n");
#endif /* PROC_DEBUG */
	if(setgroups(storage.num_wanted_gids, storage.wanted_gids))
	{
	  exit(77);
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
	  if(!pw) exit(77);
	  initgroupgid=pw->pw_gid;
/*	  printf("uid=%d euid=%d initgroups(%s,%d)\n",getuid(),geteuid(),pw->pw_name, initgroupgid); */
#ifdef PROC_DEBUG
    fprintf(stderr, "Calling initgroups()\n");
#endif /* PROC_DEBUG */
	  if(initgroups(pw->pw_name, initgroupgid))
#ifdef _HPUX_SOURCE
	    /* Kluge for HP-(S)UX */
	    if(initgroupgid>60000 &&
	       initgroups(wanted_uid,-2) &&
	       initgroups(wanted_uid,65534) &&
	       initgroups(wanted_uid,60001))
#endif /* _HPUX_SOURCE */
	    {
#ifdef HAVE_SETGROUPS
	      gid_t x[]={ 65534 };
	      if(setgroups(0,x))
#endif /* SETGROUPS */
		exit(77);
	    }
	}
#endif /* INITGROUPS */
/*	printf("uid=%d gid=%d euid=%d egid=%d setuid(%d)\n",getuid(),getgid(),geteuid(),getegid(),wanted_uid); */
	if(setuid(wanted_uid))
	{
	  perror("setuid");
	  exit(77);
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

#ifdef _REENTRANT
  if(num_threads >1)
    error("You cannot use fork in a multithreaded application.\n");
#endif

  THREADS_ALLOW_UID();
#if defined(HAVE_FORK1) && defined(_REENTRANT)
  pid=fork1();
#else
  pid=fork();
#endif
  THREADS_DISALLOW_UID();

  if(pid==-1) {
    error("Fork failed\n"
	  "errno: %d\n", errno);
  }

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

#ifdef DEBUG
static RETSIGTYPE fatal_signal(int signum)
{
  my_signal(signum,SIG_DFL);
  fatal("Fatal signal (%s) recived.\n",signame(signum));
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

#ifdef DEBUG
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


