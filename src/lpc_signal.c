/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
#include "svalue.h"
#include "interpret.h"
#include "stralloc.h"
#include "add_efun.h"
#include "macros.h"
#include "backend.h"
#include <signal.h>

#ifdef NSIG
#define MAX_SIGNALS NSIG
#else
#define MAX_SIGNALS 256
#endif

#define SIGNAL_BUFFER 16384

static struct svalue signal_callbacks[MAX_SIGNALS];

static unsigned char sigbuf[SIGNAL_BUFFER];
static int firstsig, lastsig;

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

static int my_signal(int sig, sigfunctype fun)
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
    action.sv_handler= fun;
    action.sv_mask=-1;
    action.sv_flags=0;
#ifdef SA_INTERRUPT
    if(fun != SIG_IGN)
      action.sv_flags=SV_INTERRUPT;
#endif
    sigvec(sig,&action,0);
  }
#else
  signal(sig, func);
#endif
#endif
}

static RETSIGTYPE sig_child(int arg)
{
#ifdef HAVE_WAITPID
  waitpid(-1,0,WNOHANG);
#else
#ifdef HAVE_WAIT3
  wait3(-1,0,WNOHANG);
#else

  /* Leave'em hanging */

#endif /* HAVE_WAIT3 */
#endif /* HAVE_WAITPID */

#ifdef SIGNAL_ONESHOT
  my_signal(SIGCHLD, sig_child);
#endif
}

static RETSIGTYPE receive_signal(int signum)
{
  int tmp;

  tmp=firstsig+1;
  if(tmp == SIGNAL_BUFFER) tmp=0;
  if(tmp != lastsig)
    sigbuf[firstsig=tmp]=signum;

  if(signum==SIGCHLD) sig_child(signum);

#ifndef SIGNAL_ONESHOT
  my_signal(signum, receive_signal);
#endif
}

void check_signals()
{
#ifdef DEBUG
  extern int d_flag;
  if(d_flag>5) do_debug(0);
#endif

  if(firstsig != lastsig)
  {
    int tmp=firstsig;

    while(lastsig != tmp)
    {
      if(++lastsig == SIGNAL_BUFFER) lastsig=0;

      push_int(sigbuf[lastsig]);
      apply_svalue(signal_callbacks + sigbuf[lastsig], 1);
      pop_stack();
    }
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
    if(!strcasecmp(name,signal_desc[e].signame) ||
       !strcasecmp(name,signal_desc[e].signame+3) )
      return signal_desc[e].signum;
  }
  return -1;
}

static void f_signal(int args)
{
  int signum;
  sigfunctype func;

  if(args < 1)
    error("Too few arguments to signal()\n");

  if(args == 1)
  {
    push_int(0);
    args++;
  }
  
  if(sp[-args].type != T_INT)
  {
    error("Bad argument 1 to signal()\n");
  }

  signum=sp[-args].u.integer;
  if(signum <0 || signum >=MAX_SIGNALS)
  {
    error("Signal out of range.\n");
  }

  assign_svalue(signal_callbacks + signum, sp+1-args);
  if(IS_ZERO(sp+1-args))
  {
    switch(signum)
    {
    case SIGCHLD:
      func=sig_child;
      break;

    case SIGPIPE:
      func=(sigfunctype) SIG_IGN;
      break;

    default:
      func=(sigfunctype) SIG_DFL;
    }
  }else{
    func=receive_signal;
  }
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
  if(args < 2)
    error("Too few arguments to kill().\n");
  if(sp[-args].type != T_INT)
    error("Bad argument 1 to kill().\n");
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

void init_signals()
{
  int e;

  my_signal(SIGCHLD, sig_child);
  my_signal(SIGPIPE, SIG_IGN);

  for(e=0;e<MAX_SIGNALS;e++)
    signal_callbacks[e].type=T_INT;

  firstsig=lastsig=sigbuf[0];

  add_efun("signal",f_signal,"function(int,mixed|void:void)",OPT_SIDE_EFFECT);
  add_efun("kill",f_kill,"function(int,int:int)",OPT_SIDE_EFFECT);
  add_efun("signame",f_signame,"function(int:string)",0);
  add_efun("signum",f_signum,"function(string:int)",0);
  add_efun("getpid",f_getpid,"function(:int)",0);
}

void exit_signals()
{
  int e;
  for(e=0;e<MAX_SIGNALS;e++)
  {
    free_svalue(signal_callbacks+e);
    signal_callbacks[e].type=T_INT;
  }
}


