/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "fdlib.h"
#include "fd_control.h"
#include "svalue.h"
#include "interpret.h"
#include "stralloc.h"
#include "constants.h"
#include "pike_macros.h"
#include "backend.h"
#include "pike_error.h"
#include "callback.h"
#include "mapping.h"
#include "threads.h"
#include "signal_handler.h"
#include "module_support.h"
#include "operators.h"
#include "builtin_functions.h"
#include "main.h"
#include "time_stuff.h"

#include <signal.h>
#include <errno.h>

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

#ifdef HAVE_SYS_ID_H
# include <sys/id.h>
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

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_POLL
#ifdef HAVE_POLL_H
#include <poll.h>
#elif defined(HAVE_SYS_POLL_H)
#include <sys/poll.h>
#endif /* HAVE_POLL_H || HAVE_SYS_POLL_H */
#endif /* HAVE_POLL */

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#ifdef HAVE_SYS_TERMIO_H
# include <sys/termio.h>
#endif

#ifdef HAVE_TERMIOS_H
# include <termios.h>
#elif defined(HAVE_SYS_TERMIOS_H)
/* NB: Deprecated by <termios.h> above. */
# include <sys/termios.h>
#endif

#ifdef HAVE_SYS_PTRACE_H
#include <sys/ptrace.h>
#endif

#ifdef HAVE_SYS_PARAM_H
/* Needed for <sys/user.h> on OpenBSD. */
#include <sys/param.h>
#endif

#ifdef HAVE_SYS_USER_H
/* NOTE: On AIX 4.x with COFF32 format, this file
 *       includes <syms.h>, which has incompatible
 *       definitions of T_INT and T_FLOAT.
 *	 Use PIKE_T_INT and PIKE_T_FLOAT in this file!
 *	/grubba 2003-04-14.
 */
#include <sys/user.h>
#endif

#ifdef __amigaos__
#define timeval amigaos_timeval
#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/exall.h>
#ifdef __amigaos4__
#include <interfaces/dos.h>
#include <inline4/dos.h>
#else
#include <clib/dos_protos.h>
#include <inline/dos.h>
#endif
#undef timeval
#endif

#define fp Pike_fp


#ifdef NSIG
#define MAX_SIGNALS NSIG
#else
#define MAX_SIGNALS 256
#define NSIG 256
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

#ifndef WUNTRACED
#define WUNTRACED	0
#endif /* !WUNTRACED */

#ifdef HAVE_PTRACE
/* BSDs have different names for these constants...
 *
 * ... and so does HPUX...
 *
 * And Solaris 2.9 and later don't name them at all...
 *
 * Requests 0 - 9 are standardized by AT&T/SVID as follows:
 *
 * PTRACE_TRACEME	0	Enter trace mode. Stop at exec() and signals.
 * PTRACE_PEEKTEXT	1	Read a word from the text area.
 * PTRACE_PEEKDATA	2	Read a word from the data area.
 * PTRACE_PEEKUSER	3	Read a word from the user area.
 * PTRACE_POKETEXT	4	Set a word in the text area.
 * PTRACE_POKEDATA	5	Set a word in the data area.
 * PTRACE_POKEUSER	6	Set a word in the user area.
 * PTRACE_CONT		7	Continue process with specified signal.
 * PTRACE_KILL		8	Exit process.
 * PTRACE_SINGLESTEP	9	Execute a single instruction.
 *
 * NB: In Solaris 2.x ptrace() is simulated by libc.
 */
#ifndef PTRACE_TRACEME
#ifdef PT_TRACE_ME
/* BSD */
#define PTRACE_TRACEME	PT_TRACE_ME
#elif defined(PT_SETTRC)
/* HPUX */
#define PTRACE_TRACEME	PT_SETTRC
#else
#define PTRACE_TRACEME	0
#endif
#endif

#ifndef PTRACE_PEEKUSER
#ifdef PT_READ_U
/* BSD */
#define PTRACE_PEEKUSER	PT_READ_U
#elif defined(PT_RUAREA)
/* HPUX */
#define PTRACE_PEEKUSER	PT_RUAREA
#else
#define PTRACE_PEEKUSER	3
#endif
#endif

#ifndef PTRACE_CONT
#ifdef PT_CONTINUE
/* BSD */
#define PTRACE_CONT	PT_CONTINUE
#elif defined(PT_CONTIN)
/* HPUX */
#define PTRACE_CONT	PT_CONTIN
#else
#define PTRACE_CONT	7
#endif
#endif

#ifndef PTRACE_KILL
#ifdef PT_KILL
/* BSD */
#define PTRACE_KILL	PT_KILL
#elif defined(PT_EXIT)
#define PTRACE_KILL	PT_EXIT
#else
#define PTRACE_KILL	8
#endif
#endif

#ifndef PTRACE_TAKES_FOUR_ARGS
/* HPUX and SunOS 4 have a fifth argument "addr2". */
#define ptrace(R,P,A,D)	ptrace(R,P,A,D,NULL)
#endif

#ifdef PTRACE_ADDR_TYPE_IS_POINTER
#define CAST_TO_PTRACE_ADDR(X)	((void *)(ptrdiff_t)(X))
#else /* !PTRACE_ADDR_TYPE_IS_POINTER */
#define CAST_TO_PTRACE_ADDR(X)	((ptrdiff_t)(X))
#endif /* PTRACE_ADDR_TYPE_IS_POINTER */

#endif /* HAVE_PTRACE */

/* Number of EBADF's before the set_close_on_exec() loop terminates. */
#ifndef PIKE_BADF_LIMIT
#define PIKE_BADF_LIMIT	1024
#endif /* !PIKE_BADF_LIMIT */

/* #define PROC_DEBUG */

#ifdef PROC_DEBUG
#define PROC_FPRINTF(...)	fprintf(stderr, __VA_ARGS__)
#else
#define PROC_FPRINTF(...)
#endif /* PROC_DEBUG */

#if !defined(__NT__) && !defined(__amigaos__)
#define USE_PID_MAPPING
#else
#undef USE_WAIT_THREAD
#undef USE_SIGCHILD
#endif

#if defined(USE_SIGCHILD) && defined(__linux__) && defined(_REENTRANT)
#define NEED_SIGNAL_SAFE_FIFO
#endif


#ifndef NEED_SIGNAL_SAFE_FIFO

#ifdef DEBUG
#define SAFE_FIFO_DEBUG_BEGIN() do {\
  static volatile int inside=0;	      \
  if(inside) \
    Pike_fatal("You need to define NEED_SIGNAL_SAFE_FIFO in signal_handler.c!\n"); \
  inside=1;

#define SAFE_FIFO_DEBUG_END() inside=0; }while(0)
#endif /* DEBUG */

#define DECLARE_FIFO(pre,TYPE)						  \
  static volatile TYPE PIKE_CONCAT(pre,buf) [SIGNAL_BUFFER];		  \
  static volatile int PIKE_CONCAT(pre,_first)=0,PIKE_CONCAT(pre,_last)=0; \
  static inline int PIKE_CONCAT(pre,_pop)(TYPE *val) {			  \
    int tmp2 = PIKE_CONCAT(pre,_first);					  \
    if(PIKE_CONCAT(pre,_last) != tmp2) {				  \
      int tmp;								  \
      if( ++ PIKE_CONCAT(pre,_last) == SIGNAL_BUFFER)			  \
	PIKE_CONCAT(pre,_last)=0;					  \
      tmp = PIKE_CONCAT(pre,_last);					  \
      *val = PIKE_CONCAT(pre,buf)[tmp];					  \
      return 1;								  \
    }									  \
    return 0;								  \
  }									  \
									  \
  static inline void PIKE_CONCAT(pre,_push)(TYPE val) {			  \
    int tmp = PIKE_CONCAT(pre, _first) + 1;				  \
    if (tmp >= SIGNAL_BUFFER) tmp = 0;					  \
    PIKE_CONCAT(pre, buf)[tmp] = val;					  \
    PIKE_CONCAT(pre, _first) = tmp;					  \
  }


#define QUICK_CHECK_FIFO(pre,TYPE) ( PIKE_CONCAT(pre,_first) != PIKE_CONCAT(pre,_last) )

#define INIT_FIFO(pre,TYPE)
#define REINIT_FIFO(pre,TYPE)

#else /* NEED_SIGNAL_SAFE_FIFO */

#define DECLARE_FIFO(pre,TYPE)					 \
  static int PIKE_CONCAT(pre,_fd)[2];				 \
  static volatile sig_atomic_t PIKE_CONCAT(pre,_data_available); \
  static inline int PIKE_CONCAT(pre, _pop)(TYPE *val) {		 \
    PIKE_CONCAT(pre,_data_available) = 0;			 \
    if (read(PIKE_CONCAT(pre,_fd)[0], val, sizeof(*val))	 \
	== sizeof(*val)) {					 \
      /* NB: We must reset and set data_available here		 \
       *     to avoid races. */					 \
      PIKE_CONCAT(pre,_data_available) = 1;			 \
      return 1;							 \
    }								 \
    return 0;							 \
  }								 \
								 \
  static inline void PIKE_CONCAT(pre, _push)(TYPE val) {	 \
    int sz;							 \
    int errno_save = errno;					 \
    while( (sz = write(PIKE_CONCAT(pre,_fd)[1], (char *)&val,	 \
			 sizeof(val))) < 0 &&			 \
	   errno==EINTR)					 \
      ;								 \
    DO_IF_DEBUG(if (sz != sizeof(val))				 \
		  Pike_fatal("Atomic pipe write failed!!\n"); )	 \
    errno = errno_save;					 	 \
    PIKE_CONCAT(pre,_data_available)=1;				 \
  }


#define QUICK_CHECK_FIFO(pre,TYPE) PIKE_CONCAT(pre,_data_available)

#define INIT_FIFO(pre,TYPE) do {			\
  if(pike_make_pipe(PIKE_CONCAT(pre,_fd)) <0)		\
    Pike_fatal("Couldn't create buffer " #pre ".\n");	\
							\
  set_nonblocking(PIKE_CONCAT(pre,_fd)[0],1);		\
  set_nonblocking(PIKE_CONCAT(pre,_fd)[1],1);		\
  set_close_on_exec(PIKE_CONCAT(pre,_fd)[0], 1);	\
  set_close_on_exec(PIKE_CONCAT(pre,_fd)[1], 1);	\
}while(0)

#define REINIT_FIFO(pre,TYPE) do {			\
  close(PIKE_CONCAT(pre,_fd)[0]);			\
  close(PIKE_CONCAT(pre,_fd)[1]);			\
  INIT_FIFO(pre,TYPE);					\
} while(0)

#endif /* else NEED_SIGNAL_SAFE_FIFO */

#ifndef SAFE_FIFO_DEBUG_END
#define SAFE_FIFO_DEBUG_BEGIN() do {
#define SAFE_FIFO_DEBUG_END()  }while(0)
#endif


/* There is really no need at all to optimize this for speed.

   The issue is that the loops checking EINTR etc are unrolled 32
   times or so, which is very very pointless and makes the code size
   explode for no good reason.

   An alternative solution would be helper functions for all
   systemcalls that can be interrupted (in reality, usually none when
   using modern OS:es and setting the signal flags correctly, but...)
*/

#ifdef HAVE_PRAGMA_GCC_OPTIMIZE
#pragma GCC optimize "-Os"
#endif

extern int fd_from_object(struct object *o);
static int set_priority( int pid, char *to );


static struct svalue signal_callbacks[MAX_SIGNALS];
static sig_atomic_t signal_masks[MAX_SIGNALS];
static void (*default_signals[MAX_SIGNALS])(INT32);
static struct callback *signal_evaluator_callback =0;

DECLARE_FIFO(sig, unsigned char);

#ifdef USE_PID_MAPPING
static void report_child(int pid,
			 WAITSTATUSTYPE status,
			 const char *called_from);
#endif


#ifdef USE_SIGCHILD
static RETSIGTYPE receive_sigchild(int signum);

typedef struct wait_data_s {
  pid_t pid;
  WAITSTATUSTYPE status;
} wait_data;

DECLARE_FIFO(wait, wait_data);

#endif /* USE_SIGCHILD */


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

static const struct sigdesc signal_desc []={
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
#ifdef SIGUSR1
  { SIGUSR1, "SIGUSR1" },
#endif
#ifdef SIGUSR2
  { SIGUSR2, "SIGUSR2" },
#endif
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
#ifdef SIGBREAK
  { SIGBREAK, "SIGBREAK" },
#endif

  { -1, "END" } /* Notused */
};


/* Process stuff */

#ifdef PIKE_DEBUG

#define MY_MAX_PID 65536

char process_info[MY_MAX_PID];
int last_pid_p;
int last_pids[4096];

#define P_NOT_STARTED 0
#define P_RUNNING 1
#define P_DONE 2
#define P_RUNNING_AGAIN 3
#define P_DONE_AGAIN 4

void dump_process_history(pid_t pid)
{
  int e;
  if(pid < 1)
    Pike_fatal("Pid out of range: %ld\n",(long)pid);

  fprintf(stderr,"Process history:");
  for(e=MAXIMUM(-4095,-last_pid_p);e<0;e++)
  {
    fprintf(stderr," %d",last_pids[ (last_pid_p + e) & 4095]);
  }

  if(pid<MY_MAX_PID)
    fprintf(stderr,"\nProblem pid = %d, status = %d\n",
	    (int)pid, process_info[pid]);
}


void process_started(pid_t pid)
{
  if(pid < 1)
    Pike_fatal("Pid out of range: %ld\n",(long)pid);

  last_pids[last_pid_p++ & 4095]=pid;

  if(pid>=MY_MAX_PID)
    return;

  switch(process_info[pid])
  {
    case P_NOT_STARTED:
    case P_DONE:
      process_info[pid]++;
      break;

    case P_DONE_AGAIN:
      process_info[pid]=P_RUNNING_AGAIN;
      break;

    default:
      dump_process_history(pid);
      Pike_fatal("Process debug: Pid %ld started without stopping! (status=%d)\n",(long)pid,process_info[pid]);
  }
}

void process_done(pid_t pid, const char *from)
{
  if(pid < 1)
    Pike_fatal("Pid out of range in %s: %ld\n",from,(long)pid);

  if(pid>=MY_MAX_PID)
    return;

  switch(process_info[pid])
  {
    case P_RUNNING:
    case P_RUNNING_AGAIN:
      process_info[pid]++;
      break;

    default:
#ifdef PROC_DEBUG
      dump_process_history(pid);
      PROC_FPRINTF("[%d] Process debug: Unknown child %ld in %s! (status=%d)\n",
                   getpid(), (long)pid,from,process_info[pid]);
#endif
      break;
  }
}


#else

void process_started(pid_t UNUSED(pid)) { }
void process_done(pid_t UNUSED(pid), const char *UNUSED(from)) { }

#endif /* PIKE_DEBUG */


static void register_signal(int signum)
{
  sig_push(signum);
  wake_up_backend();
}

static RETSIGTYPE receive_signal(int signum)
{
  SAFE_FIFO_DEBUG_BEGIN();
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

  register_signal(signum);
  SAFE_FIFO_DEBUG_END();
#ifdef SIGNAL_ONESHOT
  my_signal(signum, receive_signal);
#endif
}

/* This function is intended to work like signal(), but better :) */
void my_signal(int sig, sigfunctype fun)
{
  PROC_FPRINTF("[%d] my_signal(%d, 0x%p)\n", getpid(), sig, (void *)fun);
#ifdef HAVE_SIGACTION
  {
    struct sigaction action;
    /* NOTE:
     *   sa_handler is really _funcptr._handler, and
     *   sa_sigaction is really _funcptr._sigaction,
     *   where _funcptr is a union. ie sa_handler and
     *   sa_sigaction overlap.
     */
    memset(&action, 0, sizeof(action));
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
    memset(&action, 0, sizeof(action));
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



PMOD_EXPORT void check_signals(struct callback *UNUSED(foo), void *UNUSED(bar), void *UNUSED(gazonk))
{
#ifdef PIKE_DEBUG
  if(d_flag>5) do_debug();
#endif

  if (QUICK_CHECK_FIFO(sig, unsigned char))
  {
    unsigned char sig = ~0;

    while (sig_pop(&sig)) {

#ifdef USE_SIGCHILD
      if(sig == SIGCHLD)
      {
	wait_data wd;

	while (wait_pop(&wd)) {

	  if(!wd.pid)
	    Pike_fatal("wd.pid=0 NEED_SIGNAL_SAFE_FIFO is "
#ifndef NEED_SIGNAL_SAFE_FIFO
		       "not "
#endif
		       "defined.\n");

	  report_child(wd.pid, wd.status, "check_signals");
	}
      }
#endif

#if MAX_SIGNALS < 256
      if (sig >= MAX_SIGNALS) continue;
#endif

      if (!signal_masks[sig]) {
	signal_masks[sig] = 1;

	do {
	  /* NB: We check this every loop in case the
	   *     Pike-level signal handler has been
	   *     disabled during the call. */
	  if(SAFE_IS_ZERO(signal_callbacks + sig))
	  {
	    if(default_signals[sig])
	      default_signals[sig](sig);
	  } else {
	    JMP_BUF recovery;
	    free_svalue(&throw_value);
	    mark_free_svalue(&throw_value);
	    if (SETJMP_SP(recovery, 0)) {
	      call_handle_error();
	    } else {
	      push_svalue(signal_callbacks + sig);
	      push_int(sig);
	      f_call_function(2);
	      pop_stack();
	    }
	    UNSETJMP(recovery);
	  }
	} while (--signal_masks[sig]);
      } else {
	/* Signal handler is being called somewhere else.
	 * Make it repeat when finished.
	 */
	signal_masks[sig] = 2;
      }
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
    if(!STRCASECMP(name,signal_desc[e].signame) ||
       !STRCASECMP(name,signal_desc[e].signame+3) )
      return signal_desc[e].signum;
  }
  return -1;
}

/*! @decl function(int|void:void) signal(int sig, function(int|void:void) callback)
 *! @decl function(int|void:void) signal(int sig)
 *!
 *! Trap signals.
 *!
 *! This function allows you to trap a signal and have a function called
 *! when the process receives a signal. Although it IS possible to trap
 *! SIGBUS, SIGSEGV etc, I advise you not to; Pike should not receive any
 *! such signals, and if it does, it is because of bugs in the Pike
 *! interpreter. And all bugs should be reported, no matter how trifle.
 *!
 *! The callback will receive the signal number as its only argument.
 *!
 *! See the documentation for @[kill()] for a list of signals.
 *!
 *! If no second argument is given, the signal handler for that signal
 *! is restored to the default handler.
 *!
 *! If the second argument is zero, the signal will be completely ignored.
 *!
 *! @returns
 *! Returns the previous signal function, or 0 if none had been registered.
 *!
 *! @seealso
 *!   @[kill()], @[signame()], @[signum()]
 */
static void f_signal(int args)
{
  int signum;
  sigfunctype func;

  PROC_FPRINTF("[%d] f_signal(%d)\n", getpid(), args);

  check_signals(0,0,0);

  if(args < 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR("signal", 1);

  if(TYPEOF(Pike_sp[-args]) != PIKE_T_INT)
    SIMPLE_ARG_TYPE_ERROR("signal", 1, "int");

  signum=Pike_sp[-args].u.integer;
  if(signum <0 ||
     signum >=MAX_SIGNALS
    )
  {
    Pike_error("Signal %d out of range.\n", signum);
  }

  if(!signal_evaluator_callback)
  {
    signal_evaluator_callback=add_to_callback(&evaluator_callbacks,
					      check_signals,
					      0,0);
    dmalloc_accept_leak(signal_evaluator_callback);
  }

  if(args == 1)
  {
    push_int(0);
    args++;

    switch(signum)
    {
#ifdef USE_SIGCHILD
      case SIGCHLD:
	func=receive_sigchild;
	break;
#endif

#ifdef SIGPIPE
      case SIGPIPE:
	func=(sigfunctype) SIG_IGN;
	break;
#endif

      default:
	if(default_signals[signum])
	  func=receive_signal;
	else
	  func=(sigfunctype) SIG_DFL;
	break;
    }
  } else {
    if(SAFE_IS_ZERO(Pike_sp+1-args))
    {
      /* Fixme: this can disrupt sigchild and other important signal handling
       */
      func=(sigfunctype) SIG_IGN;
    }else{
      func=receive_signal;
#ifdef USE_SIGCHILD
      if(signum == SIGCHLD)
	func=receive_sigchild;
#endif
    }
  }
  assign_svalue(Pike_sp-args,signal_callbacks+signum);
  assign_svalue(signal_callbacks + signum, Pike_sp+1-args);
  my_signal(signum, func);
  pop_n_elems(args-1);
}

void set_default_signal_handler(int signum, void (*func)(INT32))
{
  int is_on=!!SAFE_IS_ZERO(signal_callbacks+signum);
  int want_on=!!func;
  default_signals[signum]=func;
  if(is_on!=want_on)
    my_signal(signum, want_on ? receive_signal : (sigfunctype) SIG_DFL);
}

/*! @decl int signum(string sig)
 *!
 *! Get a signal number given a descriptive string.
 *!
 *! This function is the inverse of @[signame()].
 *!
 *! @seealso
 *!   @[signame()], @[kill()], @[signal()]
 */
static void f_signum(int args)
{
  int i;
  if(args < 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR("signum", 1);

  if(TYPEOF(Pike_sp[-args]) != T_STRING)
    SIMPLE_ARG_TYPE_ERROR("signum", 1, "string");

  i=signum(Pike_sp[-args].u.string->str);
  pop_n_elems(args);
  push_int(i);
}

/*! @decl string signame(int sig)
 *!
 *! Returns a string describing the signal @[sig].
 *!
 *! @seealso
 *!   @[kill()], @[signum()], @[signal()]
 */
static void f_signame(int args)
{
  char *n;
  if(args < 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR("signame", 1);

  if(TYPEOF(Pike_sp[-args]) != PIKE_T_INT)
    SIMPLE_ARG_TYPE_ERROR("signame", 1, "int");

  n=signame(Pike_sp[-args].u.integer);
  pop_n_elems(args);
  if(n)
    push_text(n);
  else
    push_int(0);
}

/* #define PIKE_USE_FORKD */

#ifdef PIKE_USE_FORKD
/* Forkd communication fd. */
static int forkd_fd = -1;

void forkd(int fd)
{
  struct msghdr msg;
  struct iovec iov;
  char cmsgbuf[CMSG_LEN(sizeof(int))];
  char buf;
  int i;
  int num_fail = 0;
  msg.msg_name = NULL;
  msg.msg_namelen = 0;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_flags = 0;

  /* Clean up the fds so we don't need to do it for the children. */
  if (fd != 3) {
    do {
      i = dup2(fd, 3);
    } while (UNLIKELY(i < 0) && UNLIKELY((errno == EINTR) || (errno == EBUSY)));
    if (i < 0) {
      write(2, "FORKD: Failed to dup fd!\n", 25);
      _exit(0);
    }
    fd = 3;
  }
  for (i = 4; num_fail < PIKE_BADF_LIMIT; i++) {
    int j;
    do {
      j = close(i);
    } while (UNLIKELY(j < 0) && UNLIKELY(errno == EINTR));
    if ((j < 0) && (errno == EBADF)) num_fail++;
  }

  while (1) {
    int i;
    struct cmsghdr *cmsg;
    msg.msg_control = cmsgbuf;
    msg.msg_controllen = sizeof(cmsgbuf);
    iov.iov_base = &buf;
    iov.iov_len = 1;
    do {
      i = recvmsg(fd, &msg, 0);
    } while (UNLIKELY(i < 0) && UNLIKELY(errno == EINTR));
    if (!i) _exit(0);	/* Connection closed, shutdown forkd. */
    for (cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
      int ctrl_fd = NULL;
      if ((cmsg->cmsg_level != SOL_SOCKET) ||
	  (cmsg->cmsg_type != SCM_RIGHTS) ||
	  (cmsg->cmsg_len != CMSG_LEN(sizeof(int)))) {
	continue;
      }
      ctrl_fd = ((int *)CMSG_DATA(cmsg))[0];
      num_fds = (cmsg->cmsg_len - CMSG_LEN(0)) / sizeof(int);
      do {
	i = fork();
      } while (UNLIKELY(i < 0) && UNLIKELY(errno == EINTR));
      if (i < 0) {
	/* Fork failure. */
      } else if (i) {
	/* Fork success. */
      } else {
	/* Child. */
	forkd_child(ctrl_fd);
      }
      do {
	i = close(ctrl_fd);
      } while (UNLIKELY(i < 0) && UNLIKELY(errno == EINTR));
    }
  }
}

#endif


/* Define two macros:
 *
 * WAITPID(pid_t PID, WAITSTATUSTYPE *STATUS, int OPT)
 *	Wait for pid PID.
 *
 * MY_WAIT_ANY(WAITSTATUSTYPE *STATUS, int OPT)
 *	Wait for any child process.
 */
#if defined(HAVE_WAIT4) && defined(HAVE_WAITPID) && defined(__FreeBSD__)
/* waitpid(2) is more broken than wait4(2) on FreeBSD.
 * wait4(2) is more broken than waitpid(2) on AIX 5L 5.1.0.0/ia64.
 */
#undef HAVE_WAITPID
#endif

#ifdef HAVE_WAITPID
#define WAITPID(PID,STATUS,OPT) waitpid((PID), (STATUS), (OPT))
#define MY_WAIT_ANY(STATUS,OPT) waitpid(-1, (STATUS), (OPT))
#else
#ifdef HAVE_WAIT4
#define WAITPID(PID,STATUS,OPT) wait4((PID), (STATUS), (OPT), 0)
#define MY_WAIT_ANY(STATUS,OPT) wait4(0, (STATUS), (OPT), 0)
#else
#define WAITPID(PID,STATUS,OPT) -1
#ifdef HAVE_WAIT3
#define MY_WAIT_ANY(STATUS,OPT) wait3((STATUS), (OPT), 0)
#else
#define MY_WAIT_ANY(STATUS,OPT) ((errno=ENOTSUP), -1)
#endif
#endif
#endif


#ifdef USE_SIGCHILD

#ifdef SIGNAL_ONESHOT
static RETSIGTYPE receive_sigchild(int signum)
#else
static RETSIGTYPE receive_sigchild(int UNUSED(signum))
#endif
{
  pid_t pid;
  WAITSTATUSTYPE status;
  int masked_errno = errno;

  PROC_FPRINTF("[%d] receive_sigchild\n", getpid());

#ifdef SIGNAL_ONESHOT
  /* Reregister the signal early, so that we don't
   * miss any children.
   */
  my_signal(signum, receive_sigchild);
#endif

  SAFE_FIFO_DEBUG_BEGIN();

 try_reap_again:
  /* We carefully reap what we saw */
  pid=MY_WAIT_ANY(&status, WNOHANG|WUNTRACED);

  if(pid>0)
  {
    wait_data wd;

#ifdef __CHECKER__
    /* Clear potential padding. */
    memset(&wd, 0, sizeof(wd));
#endif

    PROC_FPRINTF("[%d] receive_sigchild got pid %d\n",
                 getpid(), pid);

    wd.pid=pid;
    wd.status=status;
    wait_push(wd);
    goto try_reap_again;
  }
  PROC_FPRINTF("[%d] receive_sigchild: No more dead children.\n",
               getpid());
  register_signal(SIGCHLD);

  SAFE_FIFO_DEBUG_END();

  /* The wait and possibly the signal stuff can obfuscate errno here,
   * while outside of the signal handler we might be in, say, the backend,
   * and cause a bit of trouble there. Let's leave errno as we found it. */
  errno = masked_errno;
}
#endif


#define PROCESS_UNKNOWN		-1
#define PROCESS_RUNNING		0
#define PROCESS_STOPPED		1
#define PROCESS_EXITED		2


#define PROCESS_FLAG_TRACED	0x01

#undef THIS
#define THIS ((struct pid_status *)CURRENT_STORAGE)

#ifdef USE_PID_MAPPING
static struct mapping *pid_mapping=0;
#endif

struct pid_status
{
  INT_TYPE pid;
#ifdef __NT__
  HANDLE handle;
  struct pid_status *next_pty_client;	/* PTY client chain. */
  struct my_pty *pty;			/* PTY master, refcounted. */
#else
  INT_TYPE sig;
  INT_TYPE flags;
  INT_TYPE state;
  INT_TYPE result;
  struct svalue callback;
#endif
};

static struct program *pid_status_program=0;

static void init_pid_status(struct object *UNUSED(o))
{
  THIS->pid=-1;
#ifdef __NT__
  THIS->handle = INVALID_HANDLE_VALUE;
#else
  THIS->sig=0;
  THIS->flags=0;
  THIS->state=PROCESS_UNKNOWN;
  THIS->result=-1;
  SET_SVAL(THIS->callback, T_INT, NUMBER_NUMBER, integer, 0);
#endif
}

#ifdef __NT__
struct pid_status *pid_status_unlink_pty(struct pid_status *pid)
{
  struct pid_status *ret;
  if (!pid) return NULL;
  ret = pid->next_pty_client;
  pid->next_pty_client = NULL;
  if (pid->pty) {
    free_pty(pid->pty);
    pid->pty = NULL;
  }
  return ret;
}

static void pid_status_link_pty(struct pid_status *pid, struct my_pty *pty)
{
  add_ref(pty);
  pid->pty = pty;
  pid->next_pty_client = pty->clients;
  pty->clients = pid;
}

int check_pty_clients(struct my_pty *pty)
{
  struct pid_status *pid;
  while ((pid = pty->clients)) {
    DWORD status;
    /* FIXME: RACE: pid->handle my be INVALID_HANDLE_VALUE if
     *              the process is about to be started.
     */
    if ((pid->pid == -1) && (pid->handle == INVALID_HANDLE_VALUE)) return 1;
    if (GetExitCodeProcess(pid->handle, &status) &&
	(status == STILL_ACTIVE)) {
      return 1;
    }
    /* Process has terminated. Unlink and check the next. */
    pty->clients = pid_status_unlink_pty(pid);
  }
  return 0;
}
#endif /* __NT__ */

static void exit_pid_status(struct object *UNUSED(o))
{
#ifdef USE_PID_MAPPING
  if(pid_mapping)
  {
    struct svalue key;
    SET_SVAL(key, PIKE_T_INT, NUMBER_NUMBER, integer, THIS->pid);
    map_delete(pid_mapping, &key);
  }
#endif

#ifdef __NT__
  if (THIS->pty) {
    struct my_pty *pty = THIS->pty;
    struct pid_status **pidptr = &pty->clients;
    while (*pidptr && (*pidptr != THIS)) {
      pidptr = &(*pidptr)->next_pty_client;
    }
    if (*pidptr) {
      *pidptr = pid_status_unlink_pty(THIS);
    }
    if (!pty->clients && !pty->other && pty->conpty) {
      /* Last client for this ConPTY has terminated,
       * and our local copy of the slave has been closed.
       *
       * Close the ConPTY.
       */
      Pike_NT_ClosePseudoConsole(pty->conpty);
      pty->conpty = 0;
    }
  }
  CloseHandle(THIS->handle);
#endif
}

#ifdef USE_PID_MAPPING
static void call_pid_status_callback(struct callback *cb, void *pid, void *UNUSED(arg))
{
  struct object *o = pid;
  struct pid_status *p;
  remove_callback(cb);
  if (!o) return;
  if(!(p=get_storage(o, pid_status_program))) {
    free_object(o);
    return;
  }
  /* Steal the reference from o. */
  push_object(o);
  safe_apply_svalue(&p->callback, 1, 1);
  pop_stack();
}
static void report_child(int pid,
			 WAITSTATUSTYPE status,
			 const char *from)
{
  /* FIXME: This won't work if WAITSTATUSTYPE == union wait. */
  PROC_FPRINTF("[%d] report_child(%d, %d, \"%s\")\n",
               getpid(), (int)pid, *(int *) &status, from);

  if (!WIFSTOPPED(status)) {
    process_done(pid, from);
  }
  if(pid_mapping)
  {
    struct svalue *s, key;
    SET_SVAL(key, PIKE_T_INT, NUMBER_NUMBER, integer, pid);
    if((s=low_mapping_lookup(pid_mapping, &key)))
    {
      struct pid_status *p = NULL;
      int call_callback = 0;
      if(TYPEOF(*s) == T_OBJECT)
      {
	struct object *o;
        PROC_FPRINTF("[%d] Found pid object for pid %d: %p\n",
                     getpid(), (int)pid, s->u.object);
	if((p=get_storage((o = s->u.object),
                          pid_status_program)))
	{
	  if (!SAFE_IS_ZERO(&p->callback)) {
	    /* Call the callback from a proper pike thread... */
	    add_ref(o);
	    add_to_callback(&evaluator_callbacks, call_pid_status_callback,
			    o, NULL);
            wake_up_backend();
	  }
	  if(WIFSTOPPED(status)) {
	    p->sig = WSTOPSIG(status);
	    p->state = PROCESS_STOPPED;

	    /* Make sure we don't remove the entry from the mapping... */
	    return;
	  } else {
	    if(WIFEXITED(status)) {
	      p->result = WEXITSTATUS(status);
	    } else {
	      if (WIFSIGNALED(status)) {
#if 0
		if (WTERMSIG(status) != 9) {
		  fprintf(stderr, "Process %d died of signal %d.\n",
			  pid, WTERMSIG(status));
		}
#endif
		p->sig = WTERMSIG(status);
	      } else {
                DWERR("Process %d died of strange cause 0x%08lx.\n",
                      pid, (unsigned long)status);
              }
	      p->result=-1;
	    }
	    p->state = PROCESS_EXITED;
            PROC_FPRINTF("[%d] Pid %d has exited\n",
                         getpid(), (int)pid);
	  }
	}
      }
      /* FIXME: Is this a good idea?
       * Shouldn't this only be done if p->state is PROCESS_EXITED?
       *	/grubba 1999-04-23
       *
       * But that will only happen if there isn't a proper pid status
       * object in the value, i.e. either a destructed object or
       * garbage that shouldn't be in the mapping to begin with. /mast
       *
       * Now when we wait(2) with WUNTRACED, there are cases
       * when we want to keep the entry. This is currently
       * achieved with the return above.
       *	/grubba 2003-02-28
       *
       * Note that this also invalidates s (and p unless we have added
       * extra references to the object).
       *	/grubba 2006-12-27
       */
      map_delete(pid_mapping, &key);
    }else{
      PROC_FPRINTF("[%d] report_child on unknown child: %d,%d\n",
                   getpid(),pid,status);
    }
  }
}
#endif

#ifdef USE_WAIT_THREAD
static COND_T process_status_change;
static COND_T start_wait_thread;
static PIKE_MUTEX_T wait_thread_mutex;
static int wait_thread_running = 0;

static void do_da_lock(void)
{
  PROC_FPRINTF("[%d] fork: getting the lock.\n", getpid());

  mt_lock(&wait_thread_mutex);

  PROC_FPRINTF("[%d] fork: got the lock.\n", getpid());
}

static void do_bi_do_da_lock(void)
{
  PROC_FPRINTF("[%d] wait thread: This is your wakeup call!\n",
               getpid());

  co_signal(&start_wait_thread);

  PROC_FPRINTF("[%d] fork: releasing the lock.\n", getpid());

  mt_unlock(&wait_thread_mutex);
}

static TH_RETURN_TYPE wait_thread(void *UNUSED(data))
{
  if(th_atfork(do_da_lock,do_bi_do_da_lock,0))
  {
    perror("pthread atfork");
    exit(1);
  }

  while(1)
  {
    WAITSTATUSTYPE status;
    int pid;
    int err;

    PROC_FPRINTF("[%d] wait_thread: getting the lock.\n", getpid());

    mt_lock(&wait_thread_mutex);
    pid=MY_WAIT_ANY(&status, WNOHANG|WUNTRACED);

    err = errno;

    if(pid < 0 && err == ECHILD)
    {
      PROC_FPRINTF("[%d] wait_thread: sleeping\n", getpid());

      co_wait(&start_wait_thread, &wait_thread_mutex);

      PROC_FPRINTF("[%d] wait_thread: waking up\n", getpid());
    }

    PROC_FPRINTF("[%d] wait_thread: releasing the lock.\n", getpid());

    mt_unlock(&wait_thread_mutex);

#ifdef ENODEV
    do {
#endif
      errno = 0;
      if(pid <= 0) pid=MY_WAIT_ANY(&status, 0|WUNTRACED);

      PROC_FPRINTF("[%d] wait thread: pid=%d status=%d errno=%d\n",
                   getpid(), pid, status, errno);

#ifdef ENODEV
      /* FreeBSD threads are broken, and sometimes
       * signals status 0, errno ENODEV on living processes.
       */
    } while (UNLIKELY(errno == ENODEV));
#endif

    if(pid>0)
    {
#if defined(HAVE_PTRACE) && \
    (defined(SIGPROF) || \
     defined(_W_SLWTED) || defined(_W_SEWTED) || defined(_W_SFWTED))
      if (WIFSTOPPED(status) &&
#if !defined(_W_SLWTED) && !defined(_W_SEWTED) && !defined(_W_SFWTED)
	  /* FreeBSD sends spurious SIGPROF signals to the child process
	   * which interferes with the process trace startup code.
	   */
	  ((WSTOPSIG(status) == SIGPROF) ||
	  /* FreeBSD is further broken in that it catches SIGKILL, and
	   * it even does it twice, so that two PTRACE_CONT(9) are
	   * required before the traced process dies. This makes it a
	   * bit hard to kill traced processes...
	   */
	   (WSTOPSIG(status) == SIGKILL))
#else
	  /* AIX has these...
	   *   _W_SLWTED	Stopped after Load Wait TracED.
	   *   _W_SEWTED	Stopped after Exec Wait TracED.
	   *   _W_SFWTED	Stopped after Fork Wait TracED.
	   *
	   * Ignore them for now.
	   */
	  ((status & 0xff) != 0x7f)
#endif
	  ) {
#if !defined(_W_SLWTED) && !defined(_W_SEWTED) && !defined(_W_SFWTED)

        PROC_FPRINTF("[%d] wait thread: Got signal %d from pid %d\n",
                     getpid(), WSTOPSIG(status), pid);

	ptrace(PTRACE_CONT, pid, CAST_TO_PTRACE_ADDR(1), WSTOPSIG(status));
#else /* defined(_W_SLWTED) || defined(_W_SEWTED) || defined(_W_SFWTED) */

        PROC_FPRINTF("[%d] wait thread: Got L/E/F status (0x%08x) from pid %d\n",
                     getpid(), status, pid);

	ptrace(PTRACE_CONT, pid, CAST_TO_PTRACE_ADDR(1), 0);
#endif /* !defined(_W_SLWTED) && !defined(_W_SEWTED) && !defined(_W_SFWTED) */
	continue;
      }
#endif /* HAVE_PTRACE && (SIGPROF || _W_SLWTED || _W_SEWTED || _W_SFWTED) */

      PROC_FPRINTF("[%d] wait thread: locking interpreter, pid=%d\n",
                   getpid(), pid);

      low_mt_lock_interpreter(); /* Can run even if threads_disabled. */

      PROC_FPRINTF("[%d] wait thread: reporting the event!\n", getpid());

      report_child(pid, status, "wait_thread");
      co_broadcast(& process_status_change);

      mt_unlock_interpreter();
      continue;
    }

    if(pid == -1)
    {
      switch(err)
      {
#if 0
        case 0:
	  /* Some versions of FreeBSD have a waitpid that fails with
	   * errno 0. See [bug 3917].
	   */
#endif /* 0 */
	case EINTR:
	case ECHILD:
	  break;

	default:
	  fprintf(stderr,"Wait thread: waitpid returned error: %d\n",err);
      }
    }
  }
}
#endif

/*! @module Process
 */

/*! @class create_process
 *! This is the recommended and most portable way to start processes
 *! in Pike. The process object is a pike abstraction of the running
 *! system process, with methods for various process housekeeping.
 *!
 *! @seealso
 *!   @[Process]
 */

/*! @decl int wait()
 *!   Waits for the process to end.
 *!
 *! @returns
 *!   @int
 *!     @value 0..
 *!       The exit code of the process.
 *!     @value -1
 *!       The process was killed by a signal.
 *!     @value -2
 *!       The process is stopped.
 *!   @endint
 *!
 *! @seealso
 *!   @[TraceProcess()->wait()]
 */
static void f_pid_status_wait(INT32 args)
{
  if(THIS->pid == -1)
    Pike_error("This process object has no process associated with it.\n");

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
      Pike_error("Failed to get status of process.\n");

    pop_n_elems(args);
    push_int(xcode);
  }
#else
  {
    int wait_for_stopped, alreadydied = 0;
  if (THIS->pid == getpid())
    Pike_error("Waiting for self.\n");

  /* NB: This function also implements TraceProcess()->wait(). */
  wait_for_stopped = THIS->flags & PROCESS_FLAG_TRACED;

  while((THIS->state == PROCESS_RUNNING) ||
	(!wait_for_stopped && (THIS->state == PROCESS_STOPPED)))
  {
#ifdef USE_WAIT_THREAD
    SWAP_OUT_CURRENT_THREAD();
    co_wait_interpreter( & process_status_change);
    SWAP_IN_CURRENT_THREAD();
#else
    int err;
    int pid = THIS->pid;
    WAITSTATUSTYPE status;

    PROC_FPRINTF("[%d] wait(%d): Waiting for child...\n", getpid(), pid);

    THREADS_ALLOW();
    pid = WAITPID(pid, &status, 0|WUNTRACED);
    err = errno;
    THREADS_DISALLOW();

    if(pid > 0)
    {
      report_child(pid, status, "->wait");
    }
    else if(pid<0)
    {
      switch(err)
      {
      case EINTR:
	break;

#if defined(_REENTRANT) || defined(USE_SIGCHILD)
      case ECHILD:
	/* If there is a SIGCHILD handler, it might have reaped
	 * the child before the call to WAITPID() above.
	 */
	/* Linux stupidity...
	 * child might be forked by another thread (aka process).
	 *
	 * SIGCHILD will be sent to all threads on Linux.
	 * The sleep in the loop below will thus be awakened by EINTR.
	 *
	 * But not if there's a race or if Linux has gotten more
	 * POSIX compliant (>= 2.4), so don't sleep too long
	 * unless we can poll the wait data pipe. /mast
	 */
	pid = THIS->pid;
	if ((THIS->state == PROCESS_RUNNING) ||
	    (!wait_for_stopped && (THIS->state == PROCESS_STOPPED))) {
	  struct pid_status *this = THIS;
	  int killret, killerr;
	  THREADS_ALLOW();
	  while ((!(killret = kill(pid, 0), killerr = errno, killret)) &&
		 (!wait_for_stopped || (this->state != PROCESS_STOPPED))) {
            PROC_FPRINTF("[%d] wait(%d): Sleeping...\n", getpid(), pid);
#ifdef HAVE_POLL
	    {
	      struct pollfd pfd[1];
#ifdef NEED_SIGNAL_SAFE_FIFO
	      pfd[0].fd = wait_fd[0];
	      pfd[0].events = POLLIN;
	      poll (pfd, 1, 10000);
#else
	      poll(pfd, 0, 100);
#endif
	    }
#else /* !HAVE_POLL */
	    /* FIXME: If this actually gets used then we really
	     * ought to do better here. :P */
	    sleep(1);
#endif /* HAVE_POLL */
	  }
	  THREADS_DISALLOW();
	  if (killret && killerr == ESRCH) {
	    if (alreadydied)
	      goto lostchild;	/* But if we already looped, punt */
	    alreadydied = 1;	/* We try looping once, to cover for races */
	  }
	}
	/* The process has died. */
        PROC_FPRINTF("[%d] wait(%d): Process dead.\n", getpid(), pid);

	if ((THIS->state == PROCESS_RUNNING) ||
	    (!wait_for_stopped && THIS->state == PROCESS_STOPPED)) {
	  /* The child hasn't been reaped yet.
	   * Try waiting some more, and if that
	   * doesn't work, let the main loop complain.
	   */
          PROC_FPRINTF("[%d] wait(%d): ... but not officially, yet.\n"
                       "[%d] wait(%d): Sleeping some more...\n",
                       getpid(), pid, getpid(), pid);
	  THREADS_ALLOW();
#ifdef HAVE_POLL
	  poll(NULL, 0, 100);
#else /* !HAVE_POLL */
	  sleep(1);
#endif /* HAVE_POLL */
	  THREADS_DISALLOW();

	  /* We can get here if several threads are waiting on the
	   * same process, or if the second sleep above wasn't enough
	   * for receive_sigchild to put the entry into the wait_data
	   * fifo. In either case we just loop and try again. */
          PROC_FPRINTF("[%d] wait(%d): Child isn't reaped yet, looping.\n",
                       getpid(), pid);
	}
	break;
#endif /* _REENTRANT || USE_SIGCHILD */

      default:
lostchild:
        Pike_error("Lost track of a child (pid %ld, errno from wait %d).\n",
                   (long)THIS->pid, err);
	break;
      }
    } else {
      /* This should not happen! */
      Pike_fatal("Pid = 0 in waitpid(%d)\n",pid);
    }
#endif
    check_threads_etc();
  }

  pop_n_elems(args);

  if (THIS->state == PROCESS_STOPPED) {
    push_int(-2);
  } else {
    push_int(THIS->result);
  }
  }
#endif /* __NT__ */

  INVALIDATE_CURRENT_TIME();
}

/*! @decl int(-1..2) status()
 *!   Returns the status of the process:
 *! @int
 *!   @value -1
 *!     Unknown
 *!   @value 0
 *!     Running
 *!   @value 1
 *!     Stopped
 *!   @value 2
 *!     Exited
 *! @endint
 */
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

/*! @decl int(0..) last_signal()
 *!   Returns the last signal that was sent to the process.
 */
static void f_pid_status_last_signal(INT32 args)
{
  pop_n_elems(args);
#ifdef __NT__
  push_int(0);
#else
  push_int(THIS->sig);
#endif
}

/*! @decl int pid()
 *! Returns the process identifier of the process.
 */
static void f_pid_status_pid(INT32 args)
{
  pop_n_elems(args);
  push_int(THIS->pid);
}

/*! @decl int set_priority(string priority)
 *! Sets the priority of the process. @[priority] is one of the strings
 *! @dl
 *!   @item "realtime"
 *!   @item "highest"
 *!   @item "higher"
 *!   @item "high"
 *!   @item "normal"
 *!   @item "low"
 *!   @item "lowest"
 *! @enddl
 */
static void f_pid_status_set_priority(INT32 args)
{
  char *to;
  int r;

  get_all_args(NULL, args, "%c", &to);
  r = set_priority( THIS->pid, to );
  pop_n_elems(args);
  push_int(r);
}


/*! @endclass
 */

#ifdef HAVE_PTRACE

/*! @class TraceProcess
 *!
 *!   Class that enables tracing of processes.
 *!
 *!   The new process will be started in stopped state.
 *!   Use @[cont()] to let it start executing.
 *!
 *! @note
 *!   This class currently only exists on systems that
 *!   implement @tt{ptrace()@}.
 */

/*! @decl inherit create_process
 */

/* NB: The code below can use THIS only because
 *     pid_status_program is inherited first.
 */

static void init_trace_process(struct object *UNUSED(o))
{
  THIS->flags |= PROCESS_FLAG_TRACED;
}

static void exit_trace_process(struct object *UNUSED(o))
{
  /* FIXME: Detach the process? */
}

/*! @decl int wait()
 *!   Waits for the process to stop.
 *!
 *! @returns
 *!   @int
 *!     @value 0..
 *!       The exit code of the process.
 *!     @value -1
 *!       The process was killed by a signal.
 *!     @value -2
 *!       The process has stopped.
 *!   @endint
 *!
 *! @seealso
 *!   @[create_process::wait()]
 */

/*! @decl void cont(int|void signal)
 *!   Allow a traced process to continue.
 *!
 *! @param signal
 *!   Deliver this signal to the process.
 *!
 *! @note
 *!   This function may only be called for stopped processes.
 *!
 *! @seealso
 *!   @[wait()]
 */
static void f_trace_process_cont(INT32 args)
{
  int cont_signal = 0;

  if(THIS->pid == -1)
    Pike_error("This process object has no process associated with it.\n");

  if (THIS->state != PROCESS_STOPPED) {
    if (THIS->state == PROCESS_RUNNING) {
      Pike_error("Process already running.\n");
    }
    Pike_error("Process not stopped\n");
  }

  if (args && TYPEOF(Pike_sp[-args]) == PIKE_T_INT) {
    cont_signal = Pike_sp[-args].u.integer;
  }

  THIS->state = PROCESS_RUNNING;

  /* The addr argument must be 1 for this request. */
  if (ptrace(PTRACE_CONT, THIS->pid,
	     CAST_TO_PTRACE_ADDR(1), cont_signal) == -1) {
    int err = errno;
    THIS->state = PROCESS_STOPPED;
    /* FIXME: Better diagnostics. */
    Pike_error("Failed to release process: %s (errno %d)\n", strerror (err), err);
  }

#ifdef __FreeBSD__
  /* Traced processes on FreeBSD have a tendency not to die
   * of cont(SIGKILL) unless they get some help...
   */
  if (cont_signal == SIGKILL) {
    PROC_FPRINTF("[%d] cont(): Continue with SIGKILL for pid %d on FreeBSD\n",
                 getpid(), THIS->pid);
    if (kill(THIS->pid, SIGKILL) == -1) {
      int err = errno;
      if (err != ESRCH) {
	Pike_error("Failed to kill process. errno:%d\n", err);
      }
    }
  }
#endif /* __FreeBSD__ */
}

/*! @decl void exit()
 *!   Cause the traced process to exit.
 *!
 *! @note
 *!   This function may only be called for stopped processes.
 *!
 *! @seealso
 *!   @[cont()], @[wait()]
 */
static void f_trace_process_exit(INT32 args)
{
  if(THIS->pid == -1)
    Pike_error("This process object has no process associated with it.\n");

  if (THIS->state != PROCESS_STOPPED) {
    if (THIS->state == PROCESS_EXITED) {
      Pike_error("Process already exited.\n");
    }
    Pike_error("Process not stopped\n");
  }

  if (ptrace(PTRACE_KILL, THIS->pid, NULL, 0) == -1) {
    int err = errno;
    /* FIXME: Better diagnostics. */
    Pike_error("Failed to exit process. errno:%d\n", err);
  }
  pop_n_elems(args);
}

#if 0	/* Disabled for now. */

/*! @class Registers
 *!   Interface to the current register contents of
 *!   a stopped process.
 *!
 *! @seealso
 *!   @[TraceProcess]
 */

/* NB: No storage needed, all state is in the parent object. */

#define THIS_PROC_REG_PROC_ID	((struct pid_status *)parent_storage(1, pid_status_program))

/*! @decl int `[](int regno)
 *!   Get the contents of register @[regno].
 */
static void f_proc_reg_index(INT32 args)
{
  struct pid_status *proc = THIS_PROC_REG_PROC_ID;
  INT_TYPE regno = 0;
  long val;

  if(proc->pid == -1)
    Pike_error("This process object has no process associated with it.\n");

  if (proc->state != PROCESS_STOPPED) {
    if (proc->state == PROCESS_EXITED) {
      Pike_error("Process has exited.\n");
    }
    Pike_error("Process not stopped.\n");
  }

  get_all_args(NULL, args, "%+", &regno);

  if (regno * sizeof(long) > sizeof(((struct user *)NULL)->regs))
    SIMPLE_ARG_TYPE_ERROR("`[]", 1, "register number");

  if ((val = ptrace(PTRACE_PEEKUSER, proc->pid,
		    ((long *)(((struct user *)NULL)->regs)) + regno, 0)) == -1) {
    int err = errno;
    /* FIXME: Better diagnostics. */
    if (errno) {
      Pike_error("Failed to exit process. errno:%d\n", err);
    }
  }
  pop_n_elems(args);
  push_int64(val);
}

/*! @endclass
 */

#endif /* 0 */

/*! @endclass
 */

#endif /* HAVE_PTRACE */

/*! @endmodule
 */

#ifdef __amigaos__

extern struct DosLibrary *DOSBase;
#ifdef __amigaos4__
extern struct DOSIFace *IDOS;
#endif

static BPTR get_amigados_handle(struct mapping *optional, char *name, int fd)
{
  char buf[32];
  long ext;
  struct svalue *tmp;
  BPTR b;

  if(optional && (tmp=simple_mapping_string_lookup(optional, name)))
  {
    if(TYPEOF(*tmp) == T_OBJECT)
    {
      fd = fd_from_object(tmp->u.object);

      if(fd == -1)
	Pike_error("File for %s is not open.\n",name);
    }
  }

#ifdef __ixemul__
  if((ext = fcntl(fd, F_EXTERNALIZE, 0)) < 0)
    Pike_error("File for %s can not be externalized.\n", name);

  sprintf(buf, "IXPIPE:%lx", ext);

  /* It's a kind of magic... */
  if((b = Open(buf, 0x4242)) == 0)
    Pike_error("File for %s can not be internalized.\n", name);

  return b;
#else
  /* FIXME! */
  return MKBADDR(NULL);
#endif
}

struct perishables
{
  BPTR stdin_b;
  BPTR stdout_b;
  BPTR stderr_b;
  BPTR cwd_lock;
  struct byte_buffer cmd_buf;
};

static void free_perishables(struct perishables *storage)
{
  if(storage->stdin_b!=0) Close(storage->stdin_b);
  if(storage->stdout_b!=0) Close(storage->stdout_b);
  if(storage->stderr_b!=0) Close(storage->stderr_b);
  if(storage->cwd_lock!=0)
    UnLock(storage->cwd_lock);
  buffer_free(&storage->cmd_buf);
}

#else /* !__amigaos__ */

#ifdef __NT__

/* NB: Handles returned by this function are only safe as
 *     long as the interpreter lock isn't released.
 */
static HANDLE get_inheritable_handle(struct mapping *optional,
				     char *name,
				     int for_reading,
				     HPCON *conpty)
{
  HANDLE ret = INVALID_HANDLE_VALUE;
  struct svalue *save_stack=Pike_sp;
  struct svalue *tmp;
  if((tmp=simple_mapping_string_lookup(optional, name)))
  {
    if(TYPEOF(*tmp) == T_OBJECT)
    {
      INT32 fd=fd_from_object(tmp->u.object);
      HANDLE h;
      int type;

      if(fd == -1)
	Pike_error("File for %s is not open.\n",name);

      if(!(fd_query_properties(fd, 0) & fd_INTERPROCESSABLE))
      {
	void create_proxy_pipe(struct object *o, int for_reading);

	create_proxy_pipe(tmp->u.object, for_reading);
	fd=fd_from_object(Pike_sp[-1].u.object);
      }

      if(fd_to_handle(fd, &type, &h, 0) < 0)
	Pike_error("File for %s is not open.\n",name);

      if (type == FD_PTY) {
	struct my_pty *pty = (struct my_pty *)h;
	if (!conpty || pty->conpty || !pty->other || !pty->other->conpty ||
	    (*conpty && (*conpty != pty->other->conpty))) {
	  /* Master side, or detached slave,
	   * or we have already selected a different conpty.
	   */
	  if (for_reading) {
	    h = pty->read_pipe;
	  } else {
	    h = pty->write_pipe;
	  }
	} else {
	  /* Slave side. Get the conpty from the master side.
	   *
	   * NB: Stdin, stdout and stderr are handled automatically
	   *     by setting the conpty. The conpty has duplicated
	   *     handles of the original pipes, and in turn provides
	   *     actual CONSOLE handles to the subprocess. We thus
	   *     do NOT return the base pipe handle.
	   *
	   * BUGS: It is not possible to have multiple conpty
	   *       objects for the same process, so the first
	   *       pty for stdin, stdout and stderr wins
	   *       (see above).
	   *
	   * BUGS: As the conpty is a separate process that may survive
	   *       our subprocess terminating, we need to keep track
	   *       of which master pty the process was bound to so
	   *       that we can close the corresponding conpty
	   *       when no one else will use it.
	   *
	   * BUGS: The above behavior is apparently a bug according
	   *       to the ConPTY developers. Fixing the bug however
	   *       leads to a race condition with respect to short-
	   *       lived processes causing the ConPTY to terminate
	   *       before we have closed out slave connection.
	   */
	  pid_status_link_pty(THIS, pty->other);
	  *conpty = pty->other->conpty;
	  release_fd(fd);

	  /* From the documentation for GetStdHandle():
	   *
	   *   If the existing value of the standard handle is NULL, or
	   *   the existing value of the standard handle looks like a
	   *   console pseudohandle, the handle is replaced with a
	   *   console handle.
	   *
	   * This is not documented in conjunction with CreateProcess() or
	   * PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE, but it seems to work.
	   */
	  return 0;
	}
      }

      if(!DuplicateHandle(GetCurrentProcess(),	/* Source process */
			  h,
			  GetCurrentProcess(),	/* Target process */
			  &ret,
			  0,			/* Access */
			  1,
			  DUPLICATE_SAME_ACCESS)) {
	release_fd(fd);
	/* This could cause handle-leaks */
	Pike_error("Failed to duplicate handle %d.\n", GetLastError());
      }
      release_fd(fd);
    }
  }
  pop_n_elems(Pike_sp-save_stack);
  return ret;
}
#endif

#ifndef __NT__

#ifdef HAVE_SETRLIMIT
#include <sys/time.h>
struct pike_limit
{
  int resource;
  struct rlimit rlp;
  struct pike_limit *next;
};
#endif

struct perishables
{
  char **env;
  char **argv;

  int *fds;

  int disabled;
#ifdef HAVE_SETRLIMIT
  struct pike_limit *limits;
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

  if (storage->fds) free(storage->fds);

  if(storage->env) free( storage->env );
  if(storage->argv) free( storage->argv );
#ifdef HAVE_SETRLIMIT
  while(storage->limits)
  {
    struct pike_limit *n = storage->limits->next;
    free( storage->limits );
    storage->limits = n;
  }
#endif
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
 *
 * NOTE: 0 is reserved.
 */

#define PROCE_CHDIR		1
#define PROCE_DUP2		2
#define PROCE_SETGID		3
#define PROCE_SETGROUPS		4
#define PROCE_GETPWUID		5
#define PROCE_INITGROUPS	6
#define PROCE_SETUID		7
#define PROCE_EXEC		8
#define PROCE_CLOEXEC		9
#define PROCE_DUP		10
#define PROCE_SETSID		11
#define PROCE_SETCTTY		12
#define PROCE_CHROOT		13
#define PROCE_CLRCLOEXEC	14

#ifdef B_POSIX_ERROR_BASE
#define ERRNO_OFFSET	B_POSIX_ERROR_BASE
#else
#define ERRNO_OFFSET	0
#endif

#define PROCERROR(err, id)	do { int _l, _i; \
    buf[0] = err; buf[1] = errno - ERRNO_OFFSET; buf[2] = id; \
    for(_i = 0; _i < 3; _i += _l) { \
      while (((_l = write(control_pipe[1], buf + _i, 3 - _i)) < 0) && \
             (errno == EINTR)) \
        ; \
      if (_l < 0) exit (99 - (errno - ERRNO_OFFSET)); \
    } \
    while((_l = close(control_pipe[1])) < 0 && errno==EINTR); \
    if (_l < 0) exit (99 + (errno - ERRNO_OFFSET)); \
    exit(99); \
  } while(0)

#endif /* !__NT__ && !__amigaos__ */

#ifdef HAVE___PRIOCNTL
# include <sys/priocntl.h>
# include <sys/rtpriocntl.h>
# include <sys/tspriocntl.h>
#else
# ifdef HAVE_SCHED_SETSCHEDULER
#  ifdef HAVE_SCHED_H
#    include <sched.h>
#  else
#   ifdef HAVE_SYS_SCHED_H
#    include <sys/sched.h>
#   endif
#  endif
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
  else if(!strcmp( to, "higher" ))
    prilevel = 2;
  else if(!strcmp( to, "high" ))
    prilevel = 1;
  else if(!strcmp( to, "normal" ))
    prilevel = 0;
  else if(!strcmp( to, "low" ))
    prilevel = -1;
  else if(!strcmp( to, "lowest" ))
    prilevel = -2;
  else
      Pike_error("bad priority %s", to);
#ifdef __NT__
  {
    HANDLE process;
    DWORD how = NORMAL_PRIORITY_CLASS;
    if( pid == getpid() || !pid ) /* pid == 0 == this process. */
      process = GetCurrentProcess();
    else
      process = OpenProcess( PROCESS_SET_INFORMATION, FALSE, pid );

    if( !process )
    {
      /* Permission denied, or no such process. */
      return 0;
    }

    switch( prilevel )
    {
    case -1:
# ifdef BELOW_NORMAL_PRIORITY_CLASS
      how = BELOW_NORMAL_PRIORITY_CLASS;
      break;
# endif
      /* Fallthrough */
    case -2:           how = IDLE_PRIORITY_CLASS;     break;
    case 0:            how = NORMAL_PRIORITY_CLASS;   break;
    case 1:
# ifdef ABOVE_NORMAL_PRIORITY_CLASS
      how = ABOVE_NORMAL_PRIORITY_CLASS;
      break;
# endif
      /* Fallthrough */
    case 2:            how = HIGH_PRIORITY_CLASS;     break;
    case 3:            how = REALTIME_PRIORITY_CLASS; break;
    }

    if( SetPriorityClass( process, how ) )
      how = 1;
    else
      how = 0;
    CloseHandle( process );
    return how;
  }
#else
# ifdef HAVE___PRIOCNTL
  if(!pid) pid = getpid();
  if( prilevel > 1 )
  {
    /* Time to get tricky :-) */
    struct {
      id_t pc_cid;
      short rt_pri;
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

    memset(&params, 0, sizeof(params));
    memset(&foo, 0, sizeof(foo));

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

    memset(&params, 0, sizeof(params));
    memset(&foo, 0, sizeof(foo));
    strcpy(foo.pc_clname, "TS");
    if( priocntl((idtype_t)0, (id_t)0, PC_GETCID, (void *)(&foo)) == -1)
      return 0;
    params.pc_cid = foo.pc_cid;
    params.ts_upri = TS_NOCHANGE;
    params.ts_uprilim = prilevel*foo.ts_maxupri/2;
    return priocntl(P_PID, (id_t)pid, PC_SETPARMS, (void *)(&params)) != -1;
  }
# else
#  ifdef HAVE_SCHED_SETSCHEDULER
  {
    int class;
    int maxprio = 2;
    int minprio = -2;
    struct sched_param param;
    memset(&param, 0, sizeof(param));

    if( prilevel == 3 )
    {
      class = SCHED_FIFO;
      param.sched_priority = sched_get_priority_max( class );
      return !sched_setscheduler( pid, class, &param );
    }
#   ifdef SCHED_RR
    if(prilevel == 2)
    {
      class = SCHED_RR;
      param.sched_priority = sched_get_priority_min( class );
      return !sched_setscheduler( pid, class, &param );
    }
    maxprio = 1;
#   endif
#   ifdef SCHED_IDLE
    if(prilevel == -2)
    {
      class = SCHED_IDLE;
      param.sched_priority = sched_get_priority_min( class );
      return !sched_setscheduler( pid, class, &param );
    }
    minprio = -1;
#   endif

    class = SCHED_OTHER;
    param.sched_priority = sched_get_priority_min( class )+
      (sched_get_priority_max( class )-
       sched_get_priority_min( class ))/(maxprio-minprio) * (prilevel-minprio);
    if (sched_setscheduler( pid, class, &param ) < 0)
      return 0;

    /* When using SCHED_OTHER, we also want to use setpriority, so we
       don't return in here in that case.  Instead, we fall through to
       the HAVE_SETPRIORITY (or even HAVE_NICE) code blocks, if
       available. */
  }

#  endif
#  ifdef HAVE_SETPRIORITY
  {
    if(prilevel == 3)
      prilevel = 2;
    errno = 0;
    return setpriority( PRIO_PROCESS, pid, -prilevel*10 )!=-1 || errno==0;
  }
#  else
#   ifdef HAVE_NICE
  if(!pid || pid == getpid())
  {
    errno=0;
    return !(nice( -prilevel*10 - nice(0) ) != -1) || errno!=EPERM;
  }
#   endif
#  endif
# endif
#endif
  return 0;
}

/*! @decl int set_priority(string level, int(0..)|void pid)
 */
void f_set_priority( INT32 args )
{
  INT_TYPE pid = 0;
  char *plevel;

  get_all_args("set_priority", args, "%c.%+", &plevel, &pid);
  pid = set_priority( pid, plevel );
  pop_n_elems(args);
  push_int( pid );
}


#ifndef __amigaos__
#ifdef HAVE_SETRLIMIT
static void internal_add_limit( struct perishables *storage,
                                int limit_resource,
                                struct svalue *limit_value )
{
  struct rlimit ol;
  struct pike_limit *l = NULL;

  if( limit_resource < 0  )
    return; /* Compat: Ignore unknown limits. */

#ifndef RLIM_SAVED_MAX
  getrlimit( limit_resource, &ol );
#else
  ol.rlim_max = RLIM_SAVED_MAX;
  ol.rlim_cur = RLIM_SAVED_CUR;
#endif

  if(TYPEOF(*limit_value) == PIKE_T_INT)
  {
    l = malloc(sizeof( struct pike_limit ));
    l->rlp.rlim_max = ol.rlim_max;
    l->rlp.rlim_cur = limit_value->u.integer;
  } else if(TYPEOF(*limit_value) == T_MAPPING) {
    struct svalue *tmp3;
    l = malloc(sizeof( struct pike_limit ));
    if((tmp3=simple_mapping_string_lookup(limit_value->u.mapping, "soft"))) {
      if(TYPEOF(*tmp3) == PIKE_T_INT)
        l->rlp.rlim_cur = (tmp3->u.integer >= 0) ?
	  (unsigned INT32)tmp3->u.integer:(unsigned INT32)ol.rlim_cur;
      else
        l->rlp.rlim_cur = RLIM_INFINITY;
    } else
      l->rlp.rlim_cur = ol.rlim_cur;
    if((tmp3=simple_mapping_string_lookup(limit_value->u.mapping, "hard"))) {
      if(TYPEOF(*tmp3) == PIKE_T_INT)
        l->rlp.rlim_max = (tmp3->u.integer >= 0) ?
	  (unsigned INT32)tmp3->u.integer:(unsigned INT32)ol.rlim_max;
      else
        l->rlp.rlim_max = RLIM_INFINITY;
    } else
      l->rlp.rlim_max = ol.rlim_max;
  } else if(TYPEOF(*limit_value) == T_ARRAY && limit_value->u.array->size == 2) {
    l = malloc(sizeof( struct pike_limit ));
    if(TYPEOF(limit_value->u.array->item[0]) == PIKE_T_INT)
      l->rlp.rlim_max = limit_value->u.array->item[0].u.integer;
    else
      l->rlp.rlim_max = ol.rlim_max;
    if(TYPEOF(limit_value->u.array->item[1]) == PIKE_T_INT)
      l->rlp.rlim_cur = limit_value->u.array->item[1].u.integer;
    else
      l->rlp.rlim_cur = ol.rlim_cur;
  } else if(TYPEOF(*limit_value) == T_STRING) {
    l = malloc(sizeof(struct pike_limit));
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
#endif /* __amigaos__ */


/*! @module Process */

/*! @class create_process
 */

#ifdef HAVE_FDWALK
struct fd_cleanup_info
{
  int num_fds;
  int control_pipe;
};
int fd_cleanup_cb(void *data, int fd)
{
  struct fd_cleanup_info *info = data;
  if ((fd < 2) || (fd < info->num_fds)) return 0;
#ifdef HAVE_BROKEN_F_SETFD
  if (fd == info->control_pipe) return 0;
  while ((close(fd) == -1) && (errno == EINTR))
    ;
#else
  set_close_on_exec(fd, 1);
#endif
  return 0;
}
#endif /* HAVE_FDWALK */

/*! @decl constant limit_value = int|array(int|string)|mapping(string:int|string)|string
 *!  Each @i{limit_value@} may be either of:
 *!
 *!  @dl
 *!  @item integer
 *!   sets current limit, max is left as it is.
 *!  @item mapping
 *!   ([ "hard":int, "soft":int ]) Both values are optional,
 *!   hard <= soft.
 *!  @item array
 *!   ({ hard, soft }), both can be set to the string
 *!   "unlimited". A value of -1 means 'keep the old value'.
 *!  @item string
 *!   The string "unlimited" sets both the hard and soft limit to unlimited
 *!  @enddl
 */

/*! @decl void create(array(string) command_args, void|mapping modifiers);
 *!
 *! @param command_args
 *! The command name and its command-line arguments. You do not have
 *! to worry about quoting them; pike does this for you.
 *!
 *! @param modifiers
 *! This optional mapping can can contain zero or more of the
 *! following parameters:
 *!
 *! @mapping
 *!
 *! @member function(Process.Process:void) "callback"
 *!  Function called when the created process changes state.
 *!
 *!  Note that this function is called in a signal handler context,
 *!  which means that it may be called by any thread at any time after
 *!  the child process has changed state, and is thus not only called
 *!  by the main thread when the main backend is idle. Indeed, you can
 *!  specify a callback even if your program does not use a backend.
 *!
 *! @member string|object(Stdio.Fd) "cwd"
 *!  Execute the command in another directory than the current
 *!  directory of this process. Please note that if the command is
 *!  given is a relative path, it will be relative to this directory
 *!  rather than the current directory of this process.
 *!
 *!  Note also that the path is relative to the @expr{"chroot"@} if used.
 *!
 *!  Note that support for the @[Stdio.Fd] variant is not present prior
 *!  to Pike 9.0, and is only present on some OSes.
 *!
 *! @member string|object(Stdio.Fd) "chroot"
 *!   Chroot to this directory before executing the command.
 *!
 *!   Note that the current directory will be changed to @expr{"/"@} in
 *!   the chroot environment, unless @expr{"cwd"@} above has been set.
 *!
 *!   Note that support for the @[Stdio.Fd] variant is not present prior
 *!   to Pike 9.0, and is only present on some OSes.
 *!
 *! @member Stdio.File "stdin"
 *! @member Stdio.File "stdout"
 *! @member Stdio.File "stderr"
 *!  These parameters allows you to change the standard input, output
 *!  and error streams of the newly created process. This is
 *!  particularly useful in combination with @[Stdio.File.pipe].
 *!
 *! @member mapping(string:string) "env"
 *!  This mapping will become the environment variables for the
 *!  created process. Normally you will want to only add or change
 *!  variables which can be achived by getting the environment mapping
 *!  for this process with @[getenv]. See the examples section.
 *!
 *! @member int|string "uid"
 *!  This parameter changes which user the new process will execute
 *!  as. Note that the current process must be running as UID 0 to use
 *!  this option. The uid can be given either as an integer as a
 *!  string containing the login name of that user.
 *!
 *!  The "gid" and "groups" for the new process will be set to the
 *!  right values for that user unless overriden by options below.
 *!
 *!  (See @[setuid] and @[getpwuid] for more info.)
 *!
 *! @member int|string "gid"
 *!  This parameter changes the primary group for the new process.
 *!  When the new process creates files, they will will be created
 *!  with this group. The group can either be given as an int or a
 *!  string containing the name of the group. (See @[setuid]
 *!  and @[getgrgid] for more info.)
 *!
 *! @member int(0..1)|object(Stdio.File) "setsid"
 *!  Set this to @expr{1@} to create a new session group.
 *!  It is also possible to set it to a File object, in which
 *!  case a new session group will be created with this file
 *!  as the controlling terminal.
 *!
 *! @member array(int|string) "setgroups"
 *!  This parameter allows you to the set the list of groups that the
 *!  new process belongs to. It is recommended that if you use this
 *!  parameter you supply at least one and no more than 16 groups.
 *!  (Some system only support up to 8...) The groups can be given as
 *!  gids or as strings with the group names.
 *!
 *! @member int(0..1) "noinitgroups"
 *!  This parameter overrides a behaviour of the "uid" parameter. If
 *!  this parameter is used, the gid and groups of the new process
 *!  will be inherited from the current process rather than changed to
 *!  the approperiate values for that uid.
 *!
 *! @member string "priority"
 *!  This sets the priority of the new process, see
 *!  @[set_priority] for more info.
 *!
 *! @member int "nice"
 *!  This sets the nice level of the new process; the lower the
 *!  number, the higher the priority of the process. Note that only
 *!  UID 0 may use negative numbers.
 *!
 *! @member int(0..1) "keep_signals"
 *!  This prevents Pike from restoring all signal handlers to their
 *!  default values for the new process. Useful to ignore certain
 *!  signals in the new process.
 *!
 *! @member array(Stdio.File|int(0..0)) "fds"
 *!  This parameter allows you to map files to filedescriptors 3 and
 *!  up. The file @expr{fds[0]@} will be remapped to fd 3 in the new
 *!  process, etc.
 *!
 *! @member mapping(string:limit_value) "rlimit"
 *!  There are two values for each limit, the soft limit and the hard
 *!  limit. Processes that do not have UID 0 may not raise the hard
 *!  limit, and the soft limit may never be increased over the hard
 *!  limit. The indices of the mapping indicate what limit to impose,
 *!  and the values dictate what the limit should be. (See also
 *!  @[System.setrlimit])
 *!
 *! @mapping
 *! @member limit_value "core"
 *!  maximum core file size in bytes
 *!
 *! @member limit_value "cpu"
 *!  maximum amount of cpu time used by the process in seconds
 *!
 *! @member limit_value "data"
 *!  maximum heap (brk, malloc) size in bytes
 *!
 *! @member limit_value "fsize"
 *!  maximum size of files created by the process in bytes
 *!
 *! @member limit_value "map_mem"
 *!  maximum size of the process's mapped address space (mmap() and
 *!  heap size) in bytes
 *!
 *! @member limit_value "mem"
 *!  maximum size of the process's total amount of available memory
 *!  (mmap, heap and stack size) in bytes
 *!
 *! @member limit_value "nofile"
 *!  maximum number of file descriptors the process may create
 *!
 *! @member limit_value "stack"
 *!  maximum stack size in bytes
 *! @endmapping
 *!
 *! @member Stdio.File "conpty"
 *!  Bind the process to the console associated with this
 *!  pty slave. NT only.
 *!
 *! @endmapping
 *!
 *! @example
 *! Process.create_process(({ "/usr/bin/env" }),
 *!                        (["env" : getenv() + (["TERM":"vt100"]) ]));
 *!
 *! @example
 *! //! Spawn a new process with the args @@[args] and optionally a
 *! //! standard input if you provide such a @@[Stdio.File] object.
 *! //! @@returns
 *! //!   Returns the new process and a pipe from which you can read
 *! //!   its output.
 *! array(Process.Process|Stdio.File) spawn(Stdio.File|void stdin, string ... args)
 *! {
 *!   Stdio.File stdout = Stdio.File();
 *!   mapping opts = ([ "stdout" : stdout->pipe() ]);
 *!   if( stdin )
 *!    opts->stdin = stdin;
 *!   return ({ Process.create_process( args, opts ), stdout });
 *! }
 *!
 *! @note
 *!   All parameters that accept both string or int input can be
 *!   noticeably slower using a string instead of an integer; if maximum
 *!   performance is an issue, please use integers.
 *!
 *! @note
 *!   On NT the only supported modifiers are: @expr{"cwd"@},
 *!   @expr{"conpty"@}, @expr{"stdin"@}, @expr{"stdout"@}, @expr{"stderr"@}
 *!   and @expr{"env"@}. All other modifiers are silently ignored.
 *!
 *! @note
 *!   Support for @expr{"callback"@} was added in Pike 7.7.
 *!
 *! @note
 *!   Chroot changing directory to @expr{"/"@} was added in Pike 7.9.
 */

/*! @endclass */

/*! @endmodule */

#ifdef __NT__
DEFINE_IMUTEX(handle_protection_mutex);
#endif /* __NT__ */

#ifdef HAVE_SETRLIMIT
extern int get_limit_id( const char *limit );
#endif

void f_create_process(INT32 args)
{
  struct array *cmd=0;
  struct mapping *optional=0;
  struct svalue *tmp;
  int e;

  check_all_args(NULL, args, BIT_ARRAY, BIT_MAPPING | BIT_VOID, 0);

  switch(args)
  {
    default:
      optional=Pike_sp[1-args].u.mapping;
      mapping_fix_type_field(optional);

      if(m_ind_types(optional) & ~BIT_STRING)
	Pike_error("Bad index type in argument 2 to Process->create()\n");

      /* FALLTHRU */

    case 1: cmd=Pike_sp[-args].u.array;
      if(cmd->size < 1)
	Pike_error("Too few elements in argument array.\n");

      if( (cmd->type_field & ~BIT_STRING) &&
	  (array_fix_type_field(cmd) & ~BIT_STRING) )
	Pike_error("Bad argument 1 to Process().\n");

      for(e=0;e<cmd->size;e++)
	if(ITEM(cmd)[e].u.string->size_shift)
	  Pike_error("Argument is not an 8-bit string.\n");
  }


#ifdef __NT__
  {
    HANDLE t1 = INVALID_HANDLE_VALUE;
    HANDLE t2 = INVALID_HANDLE_VALUE;
    HANDLE t3 = INVALID_HANDLE_VALUE;
    STARTUPINFOEXW info;
    PROCESS_INFORMATION proc;
    int ret = 0, err;
    WCHAR *filename=NULL, *command_line=NULL, *dir=NULL;
    WCHAR *env=NULL;

    /* Quote command to allow all characters (if possible) */
    /* Damn! NT doesn't have quoting! The below code attempts to
     * fake it
     */
    /* Note: On NT the following characters are illegal in filenames:
     *	\ / : * ? " < > |
     */
    {
      int e,d;
      struct byte_buffer buf;

      buffer_init(&buf);
      for(e=0;e<cmd->size;e++)
      {
	int quote=0;
	if(e)
	{
	  buffer_add_char( &buf, ' ');
	}
        /* If the argument begins AND ends with double quote assume
         * it is already correctly quoted
         */
        if (ITEM(cmd)[e].u.string->len <= 1 ||
            ITEM(cmd)[e].u.string->str[0] != '"' ||
            ITEM(cmd)[e].u.string->str[ITEM(cmd)[e].u.string->len-1] != '"')
          {
            quote=strchr(ITEM(cmd)[e].u.string->str,'"') ||
              strchr(ITEM(cmd)[e].u.string->str,' ');
          }

	if(quote)
	{
          int numslash;
	  buffer_add_char( &buf, '"');

          /* Quoting rules used by Microsoft VC startup code:
           * literal double quote must be preceeded by
           * a backslash, ONLY backslashes BEFORE double quote must be
           * escaped by doubling the backslashes
           */
	  for(d=0;d<ITEM(cmd)[e].u.string->len;d++)
	  {
	    switch(ITEM(cmd)[e].u.string->str[d])
	    {
	      case '\\':
                numslash = 1;
                /* count number of backslashes, used below */
                while(++d<ITEM(cmd)[e].u.string->len &&
                      ITEM(cmd)[e].u.string->str[d] == '\\')
                  {
                    numslash++;
                  }
                if (d >= ITEM(cmd)[e].u.string->len)
                  numslash *= 2; /* argument ends with backslashes, need to
                                    double because we add a doubleqoute below */
                else if (ITEM(cmd)[e].u.string->str[d] == '"')
                  numslash = 2*numslash + 1; /* escape backslashes and the
                                                the doublequote */

                /* insert the correct number of backslashes */
                for (;numslash > 0; numslash--)
                  buffer_add_char( &buf, '\\');

                /* add the character following backslash, if any */
                if (d<ITEM(cmd)[e].u.string->len)
                  buffer_add_char( &buf, ITEM(cmd)[e].u.string->str[d]);

		break;

	      case '"':
		buffer_add_char( &buf, '\\');
                /* fall through */
	      default:
		buffer_add_char( &buf, ITEM(cmd)[e].u.string->str[d]);
	    }
	  }
	  buffer_add_char( &buf, '"');
	}else{
	  buffer_memcpy(&buf, ITEM(cmd)[e].u.string->str, ITEM(cmd)[e].u.string->len);
	}
      }
      buffer_add_char( &buf, 0);

/*      fprintf(stderr,"COM: %s\n",buf.s.str); */

      /* NOTE: buf isn't finalized, since CreateProcess performs destructive
       *       operations on it.
       */

      command_line = pike_dwim_utf8_to_utf16(buffer_get_string(&buf));
      buffer_free(&buf);
    }

    /* FIXME: Ought to set filename properly.
     */

    memset(&info,0,sizeof(info));
    info.StartupInfo.cb = sizeof(info);

#if 0
    /* CAUTION!!!!
     *
     * This function fills in several reserved fields in the
     * StartupInfo, which in turn cause CreateProcessW() below
     * to fail with error ERROR_INVALID_PARAMETER (87) when
     * EXTENDED_STARTUPINFO_PRESENT is set.
     */
    GetStartupInfoW(&info.StartupInfo);
#endif

    /* Protect inherit status for handles */
    LOCK_IMUTEX(&handle_protection_mutex);

    info.StartupInfo.dwFlags |= STARTF_USESTDHANDLES;
    info.StartupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    info.StartupInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    info.StartupInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    SetHandleInformation(info.StartupInfo.hStdInput, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(info.StartupInfo.hStdOutput, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(info.StartupInfo.hStdError, HANDLE_FLAG_INHERIT, 0);

    info.lpAttributeList = NULL;

    if(optional)
    {
      HPCON conpty = (HPCON)0;

      if( (tmp=simple_mapping_string_lookup(optional, "cwd")) )
      {
	if(TYPEOF(*tmp) == T_STRING)
	{
	  dir = pike_dwim_utf8_to_utf16(STR0(tmp->u.string));
	  /* fprintf(stderr,"DIR: %s\n",STR0(tmp->u.string)); */
	}
      }

      CloseHandle(get_inheritable_handle(optional, "conpty", 1, &conpty));

      t1 = get_inheritable_handle(optional, "stdin", 1, &conpty);
      if(t1 != INVALID_HANDLE_VALUE) {
	info.StartupInfo.hStdInput = t1;
      }

      t2 = get_inheritable_handle(optional, "stdout", 0, &conpty);
      if(t2 != INVALID_HANDLE_VALUE) {
	info.StartupInfo.hStdOutput = t2;
      }

      t3 = get_inheritable_handle(optional, "stderr", 0, &conpty);
      if(t3 != INVALID_HANDLE_VALUE) {
	info.StartupInfo.hStdError = t3;
      }

      if (conpty) {
	LPPROC_THREAD_ATTRIBUTE_LIST attrlist = NULL;
	SIZE_T attrlist_sz = 0;
	/* Hook in the pty controller. */

	/* Get the required size for a single attribute. */
	Pike_NT_InitializeProcThreadAttributeList(attrlist, 1, 0, &attrlist_sz);

	if (!(attrlist = malloc(attrlist_sz)) ||
	    !(Pike_NT_InitializeProcThreadAttributeList(attrlist, 1, 0,
							&attrlist_sz) &&
	      (info.lpAttributeList = attrlist)) ||
	    !Pike_NT_UpdateProcThreadAttribute(attrlist, 0,
					       PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
					       conpty, sizeof(conpty),
					       NULL, NULL)) {
	  /* Out of memory or similar. */
	  if (attrlist) {
	    err = GetLastError();
	    if (!info.lpAttributeList) {
	      /* InitializeProcThreadAttributeList() failed. */
	      free(attrlist);
	    }
	  } else {
	    /* malloc() failed. */
	    err = ERROR_NOT_ENOUGH_MEMORY;
	  }

	  goto fail;
	}
      }

	if((tmp=simple_mapping_string_lookup(optional, "env")))
	{
	  if(TYPEOF(*tmp) == T_MAPPING)
	  {
	    struct mapping *m=tmp->u.mapping;
	    struct array *i,*v;
	    int ptr=0;
	    i=mapping_indices(m);
	    v=mapping_values(m);

            /* make sure the environment block is sorted */
            ref_push_array(i);
            ref_push_array(v);
            f_sort(2);
            pop_stack();

	    for(e=0;e<i->size;e++)
	    {
	      if(TYPEOF(ITEM(i)[e]) == T_STRING &&
		 TYPEOF(ITEM(v)[e]) == T_STRING)
	      {
		check_stack(3);
		ref_push_string(ITEM(i)[e].u.string);
		push_static_text("=");
		ref_push_string(ITEM(v)[e].u.string);
		f_add(3);
		ptr++;
	      }
	    }
	    free_array(i);
	    free_array(v);
	    push_string(make_shared_binary_string("\0",1));
	    f_aggregate(ptr+1);
	    push_string(make_shared_binary_string("\0",1));
	    o_multiply();

	    /* NB: The environment string contains lots of NUL characters,
	     *     so we must use the low-level variant here.
	     */
	    env = low_dwim_utf8_to_utf16(Pike_sp[-1].u.string->str,
					 Pike_sp[-1].u.string->len);
	    pop_stack();
	  }
	}

      /* FIX: env, cleanup */
    }

    THREADS_ALLOW_UID();


    SetHandleInformation(info.StartupInfo.hStdInput, HANDLE_FLAG_INHERIT,
			 HANDLE_FLAG_INHERIT);
    SetHandleInformation(info.StartupInfo.hStdOutput, HANDLE_FLAG_INHERIT,
			 HANDLE_FLAG_INHERIT);
    SetHandleInformation(info.StartupInfo.hStdError, HANDLE_FLAG_INHERIT,
			 HANDLE_FLAG_INHERIT);
    ret = CreateProcessW(filename,
			 command_line,
			 NULL,  /* process security attribute */
			 NULL,  /* thread security attribute */
			 1,     /* inherithandles */
			 CREATE_UNICODE_ENVIRONMENT |
			 EXTENDED_STARTUPINFO_PRESENT,     /* create flags */
			 env,  /* environment */
			 dir,   /* current dir */
			 &info.StartupInfo,
			 &proc);
    err=GetLastError();

    THREADS_DISALLOW_UID();

  fail:

    UNLOCK_IMUTEX(&handle_protection_mutex);

    if (info.lpAttributeList) {
      Pike_NT_DeleteProcThreadAttributeList(info.lpAttributeList);
      free(info.lpAttributeList);
    }
    if(env) free(env);
    if(dir) free(dir);
    if(filename) free(filename);
    if(command_line) free(command_line);
#if 1
    if(t1 != INVALID_HANDLE_VALUE) CloseHandle(t1);
    if(t2 != INVALID_HANDLE_VALUE) CloseHandle(t2);
    if(t3 != INVALID_HANDLE_VALUE) CloseHandle(t3);
#endif

    if(ret)
    {
      CloseHandle(proc.hThread);
      THIS->handle=proc.hProcess;

      THIS->pid=proc.dwProcessId;
    }else{
      Pike_error("Failed to start process (%d).\n",err);
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
    buffer_init(&storage.cmd_buf);

    SET_ONERROR(err, free_perishables, &storage);

    for(e=0;e<cmd->size;e++)
    {
      if(e)
        buffer_add_char( &storage.cmd_buf, ' ');
      if(strchr(STR0(ITEM(cmd)[e].u.string),'"') || strchr(STR0(ITEM(cmd)[e].u.string),' ')) {
        buffer_add_char( &storage.cmd_buf, '"');
	for(d=0;d<ITEM(cmd)[e].u.string->len;d++)
	{
	  switch(STR0(ITEM(cmd)[e].u.string)[d])
	  {
	    case '*':
	    case '"':
	      buffer_add_char( &storage.cmd_buf, '*');
            default:
	      buffer_add_char( &storage.cmd_buf, STR0(ITEM(cmd)[e].u.string)[d]);
	  }
	}
        buffer_add_char( &storage.cmd_buf, '"');
      } else
        buffer_memcpy(&storage.cmd_buf, STR0(ITEM(cmd)[e].u.string), ITEM(cmd)[e].u.string->len);
    }
    buffer_add_char( &storage.cmd_buf, '\0');

    if(optional && (tmp=simple_mapping_string_lookup(optional, "cwd")))
      if(TYPEOF(*tmp) == T_STRING)
        if((storage.cwd_lock=Lock((char *)STR0(tmp->u.string), ACCESS_READ))==0)
	  Pike_error("Failed to lock cwd \"%S\".\n", tmp->u.string);

    storage.stdin_b = get_amigados_handle(optional, "stdin", 0);
    storage.stdout_b = get_amigados_handle(optional, "stdout", 1);
    storage.stderr_b = get_amigados_handle(optional, "stderr", 2);

    PROC_FPRINTF("[%d] SystemTags(\"%s\", SYS_Asynch, TRUE, NP_Input, %p, "
                 "NP_Output, %p, NP_Error, %p, %s, %p, TAG_END);\n",
                 getpid(),
                 buffer_get_string(&storage.cmd_buf), storage.stdin_b,
                 storage.stdout_b, storage.stderr_b,
                 (storage.cwd_lock!=0? "NP_CurrentDir":"TAG_IGNORE"),
                 storage.cwd_lock);

    if(SystemTags(buffer_get_string(&storage.cmd_buf), SYS_Asynch, TRUE,
		  NP_Input, storage.stdin_b, NP_Output, storage.stdout_b,
		  NP_Error, storage.stderr_b,
	          (storage.cwd_lock!=0? NP_CurrentDir:TAG_IGNORE),
		  storage.cwd_lock, TAG_END))
      Pike_error("Failed to start process (%ld).\n", IoErr());

    UNSET_ONERROR(err);

    /*

     * Ideally, these resources should be freed here.
     * But that would cause dos.library to go nutzoid, so
     * we better not...

      if(storage.cwd_lock!=0)
        UnLock(storage.cwd_lock);
      buffer_free(&storage.cmd_buf);

    */
  }
#else /* !__amigaos__ */
  {
    struct svalue *stack_save=Pike_sp;
    ONERROR err;
    struct passwd *pw=0;
    struct perishables storage;
    int do_initgroups=1;
    int do_trace=0;
    uid_t wanted_uid;
    gid_t wanted_gid;
    int gid_request=0;
    int setsid_request=0;
    int keep_signals = 0;
    pid_t pid=-2;
    int control_pipe[2];	/* Used for communication with the child. */
    char buf[4];

    int nice_val;
    int stds[3]; /* stdin, out and err */
    int cterm; /* controlling terminal */
    char *tmp_cwd; /* to CD to */
    int tmp_cwd_fd = -1;
    char *mchroot;
    int mchroot_fd = -1;
    char *priority = NULL;
    int *fds;
    int num_fds = 3;
    int errnum = 0;

    stds[0] = stds[1] = stds[2] = -1;
    cterm = -1;
    fds = stds;
    nice_val = 0;
    tmp_cwd = NULL;
    mchroot = NULL;

    storage.env=0;
    storage.argv=0;
    storage.disabled=0;

    storage.fds = NULL;

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
      if ((tmp = simple_mapping_string_lookup(optional, "callback"))) {
	assign_svalue(&(THIS->callback), tmp);
      }
      if((tmp = simple_mapping_string_lookup(optional, "priority")) &&
         TYPEOF(*tmp) == T_STRING)
        priority = tmp->u.string->str;

      if((tmp = simple_mapping_string_lookup(optional, "gid")))
      {
	switch(TYPEOF(*tmp))
	{
	  case PIKE_T_INT:
	    wanted_gid=tmp->u.integer;
	    gid_request=1;
	    break;
#if defined(HAVE_GETGRNAM) || defined(HAVE_GETGRENT)
	  case T_STRING:
	  {
	    extern void f_getgrnam(INT32);
	    push_svalue(tmp);
	    f_getgrnam(1);
	    if(TYPEOF(Pike_sp[-1]) != T_ARRAY)
	      Pike_error("No such group.\n");
	    if(TYPEOF(Pike_sp[-1].u.array->item[2]) != PIKE_T_INT)
	      Pike_error("Getgrnam failed!\n");
	    wanted_gid = Pike_sp[-1].u.array->item[2].u.integer;
	    pop_stack();
	    gid_request=1;
	  }
	  break;
#endif
	  default:
	    Pike_error("Invalid argument for gid.\n");
	}
      }

      if ((tmp = simple_mapping_string_lookup( optional, "chroot" ))) {
        if (TYPEOF(*tmp) == T_STRING && !tmp->u.string->size_shift) {
          mchroot = tmp->u.string->str;
        } else if (TYPEOF(*tmp) == T_OBJECT) {
          mchroot_fd = fd_from_object(tmp->u.object);
        }
      }

      if ((tmp = simple_mapping_string_lookup( optional, "cwd" ))) {
        if (TYPEOF(*tmp) == T_STRING && !tmp->u.string->size_shift) {
          tmp_cwd = tmp->u.string->str;
        } else if (TYPEOF(*tmp) == T_OBJECT) {
          tmp_cwd_fd = fd_from_object(tmp->u.object);
        }
      }

      if((tmp = simple_mapping_string_lookup( optional, "setsid" )) &&
	 ((TYPEOF(*tmp) == PIKE_T_INT && tmp->u.integer) ||
	  (TYPEOF(*tmp) == T_OBJECT &&
	   (cterm = fd_from_object(tmp->u.object)) >= 0)))
        setsid_request=1;

      if ((tmp = simple_mapping_string_lookup( optional, "fds" )) &&
	  TYPEOF(*tmp) == T_ARRAY) {
	struct array *a = tmp->u.array;
	int i = a->size;
	if (i) {
	  /* Don't forget stdin, stdout, and stderr */
	  num_fds = i+3;
	  storage.fds = fds = xalloc(sizeof(int)*(num_fds));
	  fds[0] = fds[1] = fds[2] = -1;
	  while (i--) {
	    if (TYPEOF(a->item[i]) == T_OBJECT) {
	      fds[i+3] = fd_from_object(a->item[i].u.object);
	      /* FIXME: Error if -1? */
	    } else {
	      fds[i+3] = -1;
	    }
	  }
	}
      }

      if((tmp = simple_mapping_string_lookup( optional, "stdin" )) &&
         TYPEOF(*tmp) == T_OBJECT)
      {
        fds[0] = fd_from_object( tmp->u.object );
        if(fds[0] == -1)
          Pike_error("Invalid stdin file\n");
      }

      if((tmp = simple_mapping_string_lookup( optional, "stdout" )) &&
	 TYPEOF(*tmp) == T_OBJECT)
      {
        fds[1] = fd_from_object( tmp->u.object );
        if(fds[1] == -1)
          Pike_error("Invalid stdout file\n");
      }

      if((tmp = simple_mapping_string_lookup( optional, "stderr" )) &&
	 TYPEOF(*tmp) == T_OBJECT)
      {
        fds[2] = fd_from_object( tmp->u.object );
        if(fds[2] == -1)
          Pike_error("Invalid stderr file\n");
      }

      if((tmp = simple_mapping_string_lookup( optional, "nice"))
         && TYPEOF(*tmp) == PIKE_T_INT )
        nice_val = tmp->u.integer;


#ifdef HAVE_SETRLIMIT
      if((tmp=simple_mapping_string_lookup(optional, "rlimit")))
      {
        struct svalue *tmp2;
        unsigned int i;
        struct mapping *m = tmp->u.mapping;
        struct keypair *k;
        if(TYPEOF(*tmp) != T_MAPPING)
          Pike_error("Wrong type of argument for the 'rusage' option. "
                     "Should be mapping.\n");
        NEW_MAPPING_LOOP( m->data )
          if( TYPEOF(k->ind) == PIKE_T_STRING )
            internal_add_limit( &storage, get_limit_id(k->ind.u.string->str), &k->val );
      }
#endif

      if((tmp=simple_mapping_string_lookup(optional, "uid")))
      {
	switch(TYPEOF(*tmp))
	{
	  case PIKE_T_INT:
	    wanted_uid=tmp->u.integer;
#if defined(HAVE_GETPWUID) || defined(HAVE_GETPWENT)
	    if(!gid_request)
	    {
	      extern void f_getpwuid(INT32);
	      push_int(wanted_uid);
	      f_getpwuid(1);

	      if(TYPEOF(Pike_sp[-1]) == T_ARRAY)
	      {
		if(TYPEOF(Pike_sp[-1].u.array->item[3]) != PIKE_T_INT)
		  Pike_error("Getpwuid failed!\n");
		wanted_gid = Pike_sp[-1].u.array->item[3].u.integer;
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
	    if(TYPEOF(Pike_sp[-1]) != T_ARRAY)
	      Pike_error("No such user.\n");
	    if(TYPEOF(Pike_sp[-1].u.array->item[2]) != PIKE_T_INT ||
	       TYPEOF(Pike_sp[-1].u.array->item[3]) != PIKE_T_INT)
	      Pike_error("Getpwnam failed!\n");
	    wanted_uid=Pike_sp[-1].u.array->item[2].u.integer;
	    if(!gid_request)
	      wanted_gid=Pike_sp[-1].u.array->item[3].u.integer;
	    pop_stack();
	    break;
	  }
#endif
	  default:
	    Pike_error("Invalid argument for uid.\n");
	}
      }

      if((tmp=simple_mapping_string_lookup(optional, "setgroups")))
      {
#ifdef HAVE_SETGROUPS
	if(TYPEOF(*tmp) == T_ARRAY)
	{
	  storage.wanted_gids_array=tmp->u.array;
	  add_ref(storage.wanted_gids_array);
	  for(e=0;e<storage.wanted_gids_array->size;e++)
	    if(TYPEOF(storage.wanted_gids_array->item[e]) != PIKE_T_INT)
	      Pike_error("Invalid type for setgroups.\n");
	  do_initgroups=0;
	}else{
	  Pike_error("Invalid type for setgroups.\n");
	}
#else
	Pike_error("Setgroups is not available.\n");
#endif
      }


      if((tmp=simple_mapping_string_lookup(optional, "env")))
      {
	if(TYPEOF(*tmp) == T_MAPPING)
	{
	  struct mapping *m=tmp->u.mapping;
	  struct array *i,*v;
	  int ptr=0;
	  i=mapping_indices(m);
	  v=mapping_values(m);

	  storage.env=xalloc((1+m_sizeof(m)) * sizeof(char *));
	  for(e=0;e<i->size;e++)
	  {
	    if(TYPEOF(ITEM(i)[e]) == T_STRING &&
	       TYPEOF(ITEM(v)[e]) == T_STRING)
	    {
	      check_stack(3);
	      ref_push_string(ITEM(i)[e].u.string);
	      push_static_text("=");
	      ref_push_string(ITEM(v)[e].u.string);
	      f_add(3);
	      storage.env[ptr++]=Pike_sp[-1].u.string->str;
	    }
	  }
	  storage.env[ptr++]=0;
	  free_array(i);
	  free_array(v);
	  }
	}

      if((tmp=simple_mapping_string_lookup(optional, "noinitgroups")))
	if(!SAFE_IS_ZERO(tmp))
	  do_initgroups=0;

      if((tmp=simple_mapping_string_lookup(optional, "keep_signals")))
	keep_signals = !SAFE_IS_ZERO(tmp);
    }

    if (THIS->flags & PROCESS_FLAG_TRACED) {
      /* We're inherited from TraceProcess.
       * Start the process in trace mode.
       */
      do_trace = 1;
    }

#ifdef HAVE_SETGROUPS

#ifdef HAVE_GETGRENT
    if(wanted_uid != getuid() && do_initgroups)
    {
      extern void f_get_groups_for_user(INT32);
      push_int(wanted_uid);
      f_get_groups_for_user(1);
      if(TYPEOF(Pike_sp[-1]) == T_ARRAY)
      {
	storage.wanted_gids_array=Pike_sp[-1].u.array;
	Pike_sp--;
	dmalloc_touch_svalue(Pike_sp);
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
	switch(TYPEOF(storage.wanted_gids_array->item[e]))
	{
	  case PIKE_T_INT:
	    storage.wanted_gids[e]=storage.wanted_gids_array->item[e].u.integer;
	    break;

#if defined(HAVE_GETGRENT) || defined(HAVE_GETGRNAM)
	  case T_STRING:
	  {
	    extern void f_getgrnam(INT32);
	    ref_push_string(storage.wanted_gids_array->item[e].u.string);
	    f_getgrnam(2);
	    if(TYPEOF(Pike_sp[-1]) != T_ARRAY)
	      Pike_error("No such group.\n");

	    storage.wanted_gids[e]=Pike_sp[-1].u.array->item[2].u.integer;
	    pop_stack();
	    break;
	  }
#endif

	  default:
	    Pike_error("Gids must be integers or strings only.\n");
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
      Pike_error("Failed to create child communication pipe.\n");
    }

    PROC_FPRINTF("[%d] control_pipe: %d, %d\n",
                 getpid(), control_pipe[0], control_pipe[1]);
#ifdef USE_WAIT_THREAD
    if (!wait_thread_running) {
      THREAD_T foo;
      if (th_create_small(&foo, wait_thread, 0)) {
	Pike_error("Failed to create wait thread!\n"
		   "errno: %d\n", errno);
      }
      wait_thread_running = 1;
    }
    num_threads++;    /* We use the interpreter lock */
#endif

    th_atfork_prepare();
    {
      int loop_cnt = 0;
      sigset_t new_sig, old_sig;
      sigfillset(&new_sig);
      while(sigprocmask(SIG_BLOCK, &new_sig, &old_sig))
	;

      do {
        PROC_FPRINTF("[%d] Forking... (pid=%d errno=%d)\n",
                     getpid(), pid, errno);
#ifdef HAVE_VFORK
	pid=vfork();
#else /* !HAVE_VFORK */
#if defined(HAVE_FORK1) && defined(_REENTRANT)
	pid=fork1();
#else
	pid=fork();
#endif
#endif /* HAVE_VFORK */
	if (pid == -1) {
	  errnum = errno;
	  if (UNLIKELY(errnum == EAGAIN)) {
            PROC_FPRINTF("[%d] Fork failed with EAGAIN\n", getpid());
	    /* Process table full or similar.
	     * Try sleeping for a bit.
	     *
	     * FIXME: Ought to release the interpreter lock.
	     */
	    if (loop_cnt++ < 60) {
	      /* Don't sleep for too long... */
#ifdef HAVE_POLL
	      poll(NULL, 0, 100);
#else /* !HAVE_POLL */
	      sleep(1);
#endif /* HAVE_POLL */

	      /* Try again */
	      continue;
	    }
	  } else if (UNLIKELY(errnum == EINTR)) {
            PROC_FPRINTF("[%d] Fork failed with EINTR\n", getpid());
	    /* Try again */
	    continue;
	  } else {
            PROC_FPRINTF("[%d] Fork failed with errno:%d\n",
                         getpid(), errnum);
	  }
	}
#ifdef PROC_DEBUG
	else if (pid) {
          PROC_FPRINTF("[%d] Fork ok pid:%d\n", getpid(), pid);
	} else {
	  write(2, "Fork ok pid:0\n", 14);
	}
#endif /* PROC_DEBUG */
	break;
      } while(1);

      while(UNLIKELY(sigprocmask(SIG_SETMASK, &old_sig, 0))) {
#ifdef PROC_DEBUG
	char errcode[3] = {
	  '0'+((errno/100)%10),
	  '0'+((errno/10)%10),
	  '0'+(errno%10),
	};
	write(2, "sigprocmask() failed with errno:", 32);
	write(2, errcode, 3);
	write(2, "\n", 1);
#endif /* PROC_DEBUG */
      }
    }

    if(pid) {
      PROC_FPRINTF("[%d] th_at_fork_parent()...\n", getpid());
      th_atfork_parent();
      PROC_FPRINTF("[%d] th_at_fork_parent() done\n", getpid());
    } else {
#ifdef PROC_DEBUG
      write(2, "th_at_fork_child()...\n", 22);
#endif /* PROC_DEBUG */
      th_atfork_child();
#ifdef PROC_DEBUG
      write(2, "th_at_fork_child() done\n", 24);
#endif /* PROC_DEBUG */
    }

    if (pid) {
      /* It's a bad idea to do this in the forked process... */
      UNSET_ONERROR(err);
    }

    if(pid == -1) {
      /*
       * fork() failed
       */

        while(UNLIKELY(close(control_pipe[0]) < 0) && UNLIKELY(errno==EINTR));
        while(UNLIKELY(close(control_pipe[1]) < 0) && UNLIKELY(errno==EINTR));

      free_perishables(&storage);

      if (UNLIKELY(errnum == EAGAIN)) {
	Pike_error("fork() failed with EAGAIN. Process table full?\n");
      }
#ifdef ENOMEM
      if (UNLIKELY(errnum == ENOMEM)) {
	Pike_error("fork() failed with ENOMEM. Out of memory?\n");
      }
#endif /* ENOMEM */
      Pike_error("fork() failed. errno:%d\n", errnum);
    } else if(pid) {
      int olderrno;
      int cnt = 0;

      PROC_FPRINTF("[%d] Parent\n", getpid());

      process_started(pid);  /* Debug */
      /*
       * The parent process
       */

      /* Close our child's end of the pipe. */
      while(UNLIKELY(close(control_pipe[1]) < 0) && UNLIKELY(errno==EINTR));

      free_perishables(&storage);

      pop_n_elems(Pike_sp - stack_save);


#ifdef USE_SIGCHILD
      /* FIXME: Can anything of the following throw an error? */
      if(!signal_evaluator_callback)
      {
	signal_evaluator_callback=add_to_callback(&evaluator_callbacks,
						  check_signals,
						  0,0);
	dmalloc_accept_leak(signal_evaluator_callback);
      }
#endif

      THIS->pid = pid;
      THIS->state = PROCESS_RUNNING;
      ref_push_object(Pike_fp->current_object);
      push_int(pid);
      mapping_insert(pid_mapping, Pike_sp-1, Pike_sp-2);
      pop_n_elems(2);

      /* Wake up the child. */
      buf[0] = 0;


      THREADS_ALLOW();

#ifndef HAVE_VFORK
      /* The following code won't work with a real vfork(). */
      PROC_FPRINTF("[%d] Parent: Wake up child.\n", getpid());
      while (UNLIKELY(((e = write(control_pipe[0], buf, 1)) < 0)) && UNLIKELY(errno == EINTR))
	;
      if(e!=1) {
	/* Paranoia in case close() sets errno. */
	olderrno = errno;
	while(UNLIKELY(close(control_pipe[0]) < 0) && UNLIKELY(errno==EINTR))
	  ;
	Pike_error("Child process died prematurely. (e=%d errno=%d)\n",
		   e ,olderrno);
      }
#endif /* !HAVE_VFORK */

      PROC_FPRINTF("[%d] Parent: Wait for child...\n", getpid());

      buf[0] = buf[1] = buf[2] = 0;
      /* Wait for exec or error */
#if defined(EBADF) && defined(EPIPE)
      /* Attempt to workaround spurious errors from read(2) on FreeBSD. */
      while (UNLIKELY((e = read(control_pipe[0], buf, 3)) < 0) &&
             (errno == EINTR ||
              (LIKELY(errno != EBADF) && (errno != EPIPE) && (cnt++ < 16))))
	;
#else /* !EBADF || !EPIPE */
      /* This code *should* work... */
      while (UNLIKELY((e = read(control_pipe[0], buf, 3)) < 0) && (errno == EINTR))
	;
#endif /* EBADF && EPIPE */
      /* Paranoia in case close() sets errno. */
      olderrno = errno;

      while(UNLIKELY(close(control_pipe[0]) < 0) && UNLIKELY(errno==EINTR));

      THREADS_DISALLOW();

      PROC_FPRINTF("[%d] Parent: Child init done.\n", getpid());

      if (!e) {
	/* OK! */
	pop_n_elems(args);
        return;
      } else if (e < 0) {
	/* Something went wrong with our read(2). */
#ifdef ENODEV
	if (olderrno == ENODEV) {
	  /* This occurrs sometimes on FreeBSD... */
	  Pike_error("read(2) failed with ENODEV. Probable operating system bug.\n");
	}
#endif /* ENODEV */
	Pike_error("read(2) failed with errno %d.\n", olderrno);
      } else {
	/* Something went wrong in the child. */
        int child_errno = buf[1] + ERRNO_OFFSET;
	switch(buf[0]) {
	case PROCE_CHDIR:
	  if (buf[2]) {
            Pike_error("chdir(\"/\") in chroot failed. errno:%d.\n",
                       child_errno);
	  }
          Pike_error("chdir() failed. errno:%d.\n", child_errno);
	  break;
	case PROCE_DUP:
          Pike_error("dup() failed. errno:%d.\n", child_errno);
	  break;
	case PROCE_DUP2:
          if (child_errno == EINVAL) {
	    Pike_error("dup2(x, %d) failed with EINVAL.\n", buf[2]);
	  }
          Pike_error("dup2(x, %d) failed. errno:%d.\n", buf[2], child_errno);
	  break;
	case PROCE_SETGID:
          Pike_error("setgid(%d) failed. errno:%d\n", buf[2], child_errno);
	  break;
#ifdef HAVE_SETGROUPS
	case PROCE_SETGROUPS:
          if (child_errno == EINVAL) {
	    Pike_error("setgroups() failed with EINVAL.\n"
		       "Too many supplementary groups (%d)?\n",
		       storage.num_wanted_gids);
	  }
          Pike_error("setgroups() failed. errno:%d.\n", child_errno);
	  break;
#endif
	case PROCE_GETPWUID:
          Pike_error("getpwuid(%d) failed. errno:%d.\n", buf[2], child_errno);
	  break;
	case PROCE_INITGROUPS:
          Pike_error("initgroups() failed. errno:%d.\n", child_errno);
	  break;
	case PROCE_SETUID:
          if (child_errno == EINVAL) {
	    Pike_error("Invalid uid: %d.\n", (int)wanted_uid);
	  }
          Pike_error("setuid(%d) failed. errno:%d.\n", buf[2], child_errno);
	  break;
	case PROCE_SETSID:
          Pike_error("setsid() failed.\n");
	  break;
	case PROCE_SETCTTY:
          Pike_error("Failed to set controlling TTY. errno:%d.\n", child_errno);
	  break;
	case PROCE_EXEC:
          switch(child_errno) {
	  case ENOENT:
	    Pike_error("Executable file not found.\n");
	    break;
	  case EACCES:
	    Pike_error("Access denied.\n");
	    break;
#ifdef E2BIG
	  case E2BIG:
	    Pike_error("Arglist too long.\n");
	    break;
#endif /* E2BIG */
	  }

          Pike_error("exec() failed. errno:%d. File not found?\n", child_errno);
	  break;
	case PROCE_CLOEXEC:
	  Pike_error("set_close_on_exec(%d, 1) failed. errno:%d.\n",
                     buf[2], child_errno);
	  break;
	case PROCE_CLRCLOEXEC:
	  Pike_error("set_close_on_exec(%d, 0) failed. errno:%d.\n",
                     buf[2], child_errno);
	  break;
	case PROCE_CHROOT:
          Pike_error("chroot() failed. errno:%d.\n", child_errno);
	  break;
	case 0:
	  /* read() probably failed. */
	default:
          Pike_error("Child failed: %d, %d (%d), %d, %d, %d!\n",
                     buf[0], child_errno, buf[1], buf[2], e, olderrno);
	  break;
	}
      }
    }else{
      /*
       * The child process
       *
       * NB: Avoid calling any functions in libc here, since
       *     internal mutexes may be held by other nonforked
       *     threads.
       */
#ifdef DECLARE_ENVIRON
      extern char **environ;
#endif
      extern void my_set_close_on_exec(int,int);
      extern void do_set_close_on_exec(void);

#ifdef PROC_DEBUG
      char debug_prefix[] = {
	'[',
	'0' + (getpid()/100000)%10,
	'0' + (getpid()/10000)%10,
	'0' + (getpid()/1000)%10,
	'0' + (getpid()/100)%10,
	'0' + (getpid()/10)%10,
	'0' + getpid()%10,
	']',
	' ',
      };
      write(2, debug_prefix, sizeof(debug_prefix));
      write(2, "Child\n", 6);
#endif /* PROC_DEBUG */

      pid = getpid();

      /* Close our parent's end of the pipe. */
      while(UNLIKELY(close(control_pipe[0]) < 0) && UNLIKELY(errno==EINTR));

#ifndef HAVE_VFORK
      /* Wait for parent to get ready... */
      while (UNLIKELY((( e = read(control_pipe[1], buf, 1)) < 0)) && UNLIKELY((errno == EINTR)))
	;

#ifdef PROC_DEBUG
      if (e < 0) {
        int ee = errno - ERRNO_OFFSET;
	char buf[5] = {
          '0' + (ee/1000)%10,
          '0' + (ee/100)%10,
          '0' + (ee/10)%10,
          '0' + ee%10,
          '\n'
	};

	write(2, debug_prefix, sizeof(debug_prefix));
	write(2, "Child: Control pipe read failed with errno: ", 44);
	write(2, buf, 5);
      } else if (!e) {
	write(2, debug_prefix, sizeof(debug_prefix));
	write(2, "Child: No data from control pipe.\n", 34);
      }
      write(2, debug_prefix, sizeof(debug_prefix));
      write(2, "Child: Woken up.\n", 17);
#endif /* PROC_DEBUG */

#endif /* HAVE_VFORK */
      for(e=0;e<cmd->size;e++) storage.argv[e]=cmd->item[e].u.string->str;
      storage.argv[e]=0;

      if(storage.env) environ=storage.env;

#ifdef HAVE_SETEUID
      if( seteuid(0) == -1 ) {
          /* all is ok, just silencing gcc-warning */
      }
#else /* !HAVE_SETEUID */
#ifdef HAVE_SETRESUID
      setresuid(0,0,-1);
#endif /* HAVE_SETRESUID */
#endif /* HAVE_SETEUID */

      if (!keep_signals)
      {
	int i;
	/* Restore the signals to the defaults. */
#ifdef _sys_nsig
        for(i=0; i<_sys_nsig; i++)
          signal(i, SIG_DFL);
#else /* !_sys_nsig */
        for(i=0; i<NSIG; i++)
          signal(i, SIG_DFL);
#endif /* _sys_nsig */
      }

#ifdef HAVE_CHROOT
      if(mchroot)
      {
	if( chroot( mchroot ) )
        {
	  /* fprintf is not safe if we use vfork. */
          PROC_FPRINTF("[%d] child: chroot(\"%s\") failed, errno=%d\n",
                       getpid(), mchroot, errno);
          PROCERROR(PROCE_CHROOT, 0);
        }
	if (chdir("/"))
	{
          PROC_FPRINTF("[%d] child: chdir(\"/\") failed, errno=%d\n",
                       getpid(), errno);
	  PROCERROR(PROCE_CHDIR, 1);
	}
      }

#ifdef HAVE_FCHDIR
      if(mchroot_fd != -1)
      {
        if (fchdir(mchroot_fd)) {
          /* fprintf is not safe if we use vfork. */
          PROC_FPRINTF("[%d] child: fchdir(%d) failed, errno=%d\n",
                       getpid(), mchroot_fd, errno);
          PROCERROR(PROCE_CHDIR, 0);
        }
        if (chroot("."))
        {
          /* fprintf is not safe if we use vfork. */
          PROC_FPRINTF("[%d] child: chroot(\".\") failed, errno=%d\n",
                       getpid(), errno);
          PROCERROR(PROCE_CHROOT, 1);
        }
      }
#endif
#endif

      if(tmp_cwd)
      {
        if( chdir( tmp_cwd ) )
        {
	  /* fprintf is not safe if we use vfork. */
          PROC_FPRINTF("[%d] child: chdir(\"%s\") failed, errno=%d\n",
                       getpid(), tmp_cwd, errno);
          PROCERROR(PROCE_CHDIR, 0);
        }
      }

#ifdef HAVE_FCHDIR
      if(tmp_cwd_fd != -1)
      {
        if( fchdir( tmp_cwd_fd ) )
        {
          /* fprintf is not safe if we use vfork. */
          PROC_FPRINTF("[%d] child: fchdir(%d) failed, errno=%d\n",
                       getpid(), tmp_cwd_fd, errno);
          PROCERROR(PROCE_CHDIR, 0);
        }
      }
#endif

#ifdef HAVE_NICE
      if(nice_val)
          if( nice(nice_val) == -1 )
          {
              /* nice failed. Should we do something? */
          }
#endif

#ifdef HAVE_SETRLIMIT
      if(storage.limits)
      {
        struct pike_limit *l = storage.limits;
        while(l)
        {
          setrlimit( l->resource, &l->rlp );
          l = l->next;
        }
      }
#endif

#ifdef HAVE_SETSID
      if (setsid_request) {
	int fd;
#ifdef TIOCNOTTY
	fd = open("/dev/tty", O_RDWR | O_NOCTTY);
	if (fd >= 0) {
	  (void) ioctl(fd, TIOCNOTTY, NULL);
	  close(fd);
	}
#endif
	if (setsid()==-1)
	  PROCERROR(PROCE_SETSID,0);

#ifdef TCSETCTTY
	 if (cterm >= 0)
	   if (ioctl(cterm, TCSETCTTY, NULL)<0)
	     PROCERROR(PROCE_SETCTTY,0);
#else
	 if (cterm >= 0) {
	   char *ttn;
#ifdef TIOCSCTTY
	   if (ioctl(cterm, TIOCSCTTY, NULL)<0)
	     PROCERROR(PROCE_SETCTTY,0);
#endif
	   ttn = ttyname(cterm);
	   if (ttn == NULL)
	     PROCERROR(PROCE_SETCTTY,0);
	   fd = open(ttn, O_RDWR);
	   if (fd < 0)
	     PROCERROR(PROCE_SETCTTY,0);
	   close(fd);
	 }
/* FIXME: else... what? error or ignore? */
#endif
      }

/* FIXME: else... what? error or ignore? */
#endif

      /* Perform fd remapping */
      {
        int fd;
	/* Note: This is O(n�), but that ought to be ok. */
	for (fd=0; fd<num_fds; fd++) {
	  /*fprintf(stderr, "Remapping fd %d to %d\n", fds[fd], fd);*/
	  if (fds[fd] == -1) continue;
	  if (fds[fd] == fd) {
	    /* Clear close on exec. */
	    int code = set_close_on_exec(fd, 0);
	    if (code < 0)
	      PROCERROR(PROCE_CLRCLOEXEC, fd);
	    continue;
	  }
	  if (fd == control_pipe[1]) {
	    /* Our control pipe is in the way.
	     * Move it.
	     */
	    int remapped;
	    if ((remapped = dup(fd)) < 0)
	      PROCERROR(PROCE_DUP, fd);
	    /*fprintf(stderr, "Moved control pipe to fd %d\n", remapped);*/
	    control_pipe[1] = remapped;
	  } else {
	    /* Is there any other fd in the way?
	     */
	    int fd2;
	    int remapped = -1;
	    for (fd2 = fd+1; fd2 < num_fds; fd2++) {
	      if (fds[fd2] == fd) {
		/* We need to temporarily remap this fd,
		 * since it's in the way
		 */
		if (remapped == -1) {
		  if ((remapped = dup(fd)) < 0)
		    PROCERROR(PROCE_DUP, fd);
		}
		/*fprintf(stderr, "Moved fd %d to be mapped to fd %d to fd %d\n",
		  fds[fd2], fd2, remapped);*/
		fds[fd2] = remapped;
	      }
	    }
	  }
	  if (dup2(fds[fd], fd) < 0)
	    PROCERROR(PROCE_DUP2, fd);
	  /*fprintf(stderr, "fd %d successfully remapped to fd %d\n",
	    fds[fd], fd);*/
	}
	/* Close the source fds. */
	for (fd=0; fd<num_fds; fd++) {
	  /* FIXME: Should it be a comparison against -1 instead? */
	  /* Always keep stdin, stdout & stderr. */
	  if (fds[fd] > 2) {
	    if ((fds[fd] >= num_fds) ||
		(fds[fds[fd]] == -1)) {
	      /*fprintf(stderr, "Closing fd %d\n", fds[fd]);*/
	      close(fds[fd]);
	    }
	  }
	}

	/* Close unknown fds which have been created elsewhere (e.g. in
	   the Java environment) */
#ifdef HAVE_FDWALK
	{
	  struct fd_cleanup_info info = {
	    num_fds, control_pipe[1],
	  };
	  fdwalk(fd_cleanup_cb, &info);
	}
#else /* !HAVE_FDWALK */
	{
	  int num_fail = 0;
#ifdef _SC_OPEN_MAX
	  int max_fds = sysconf(_SC_OPEN_MAX);
#endif
	  for (fd = num_fds;
#ifdef _SC_OPEN_MAX
	       fd < max_fds;
#else
	       1;
#endif
	       fd++) {
#ifdef HAVE_CLOSEFROM
	    if (fd > control_pipe[1]) {
	      /* NOTE: We need to loop on all errno's since we don't know
	       *       if any of them failed with EINTR and got overwritten.
	       * NOTE: EBADF shouldn't occur as an overwritten errno, since
	       *       if would mean that close(2) was called for an invalid
	       *       fd, which closefrom shouldn't do.
	       */
	      do {
		errno = 0;
		closefrom(fd);
		/* NOTE: OpenBSD sets errno to EBADF if fd is > than
		 *       any open fd.
		 */
	      } while (UNLIKELY(errno && (errno != EBADF && errno != ENOSYS)));
	      if (errno != ENOSYS)
		break;
	    }
#endif
#ifdef HAVE_BROKEN_F_SETFD
	    int code = 0;
	    if (fd != control_pipe[1]) {
	      /* In this case set_close_on_exec is not fork1(2) safe. */
	      code = close(fd);
	    }
#else /* !HAVE_BROKEN_F_SETFD */
	    /* Delay close to actual exec */
	    int code = set_close_on_exec(fd, 1);
#endif /* HAVE_BROKEN_F_SETFD */
	    if (code == -1) {
	      if (++num_fail >= PIKE_BADF_LIMIT) {
		break;
	      }
	    } else {
	      /*fprintf(stderr, "Set close on exec on fd %d\n", fd);*/
	      num_fail = 0;
	    }
	  }
	}
#endif /* HAVE_FDWALK */
	/* FIXME: Map the fds as not close on exec? */
      }

      /* Ensure that the pipe will be closed when the child starts. */
      if(set_close_on_exec(control_pipe[1], 1) < 0)
	PROCERROR(PROCE_CLOEXEC, control_pipe[1]);

      /*fprintf(stderr, "Set close on exec on fd %d (control_pipe)\n",
	control_pipe[1]);*/

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
      if( seteuid(wanted_uid) == -1 )
      {
          perror("seteuid");
          PROCERROR(PROCE_SETUID, (int)wanted_uid);
      }
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

#ifdef HAVE_PTRACE
      if (do_trace) {
#ifdef PROC_DEBUG
	write(2, debug_prefix, sizeof(debug_prefix));
	write(2, "Child: Calling ptrace()...\n", 27);
#endif /* PROC_DEBUG */

	/* NB: A return value is not defined for this ptrace request! */
	ptrace(PTRACE_TRACEME, 0, NULL, 0);
      }
#endif /* HAVE_PTRACE */

#ifdef PROC_DEBUG
      write(2, debug_prefix, sizeof(debug_prefix));
      write(2, "Child: Calling exec()...\n", 25);
#endif /* PROC_DEBUG */

      execvp(storage.argv[0],storage.argv);

      /* fprintf is not safe if we use vfork. */
      PROC_FPRINTF("[%d] Child: execvp(\"%s\", ...) failed\n"
                   "errno = %d\n",
                   getpid(), storage.argv[0], errno);
#ifndef HAVE_BROKEN_F_SETFD
      /* No way to tell about this on broken OS's :-( */
      PROCERROR(PROCE_EXEC, 0);
#endif /* HAVE_BROKEN_F_SETFD */
      exit(99);
    }
  }
#endif /* __amigaos__ */
#endif /* __NT__ */
  pop_n_elems(args);
}

#ifdef HAVE_FORK
/*! @decl object fork()
 *!
 *! Fork the process in two.
 *!
 *! Fork splits the process in two, and for the parent it returns a
 *! pid object for the child. Refer to your Unix manual for further
 *! details.
 *!
 *! @note
 *!   This function can cause endless bugs if used without proper care.
 *!
 *!   This function is disabled when using threads.
 *!
 *!   This function is not available on all platforms.
 *!
 *!   The most common use for fork is to start sub programs, which is
 *!   better done with @[Process.create_process()].
 *!
 *! @seealso
 *!   @[Process.create_process()]
 */
void Pike_f_fork(INT32 args)
{
  struct object *o;
  pid_t pid;
  pop_n_elems(args);

#ifdef _REENTRANT
  if(num_threads >1)
    Pike_error("You cannot use fork in a multithreaded application.\n");
#endif

  th_atfork_prepare();
/*   THREADS_ALLOW_UID(); */
#if defined(HAVE_FORK1) && defined(_REENTRANT)
  /* This section is disabled, since fork1() isn't usefull if
   * you aren't about do an exec().
   * In addition any helper threads (like the wait thread) would
   * disappear, so the child whould be crippled.
   *	/grubba 1999-03-07
   *
   * Reenabled since fork() usually performs a fork1() anyway.
   *	/grubba 2004-04-02
   */
  pid=fork1();
#else
  pid=fork();
#endif
/*  THREADS_DISALLOW_UID(); */

  if(pid) {
#ifdef USE_WAIT_THREAD
    if (!wait_thread_running) {
      /* NOTE: This code is delayed to after the fork so as to allow for
       *       detaching.
       */
      THREAD_T foo;
      if (th_create_small(&foo, wait_thread, 0)) {
	Pike_error("Failed to create wait thread. errno: %d.\n", errno);
      }
      wait_thread_running = 1;
    }
    num_threads++;    /* We use the interpreter lock */
#endif
    th_atfork_parent();
  } else {
    th_atfork_child();
    DO_IF_PROFILING({
	/* Reset profiling information. */
	struct pike_frame *frame = Pike_fp;
	cpu_time_t now = get_cpu_time();
	Pike_interpreter.unlocked_time = 0;
	Pike_interpreter.accounted_time = 0;
	while(frame) {
	  frame->start_time = now;
	  frame->children_base = 0;
	  frame = frame->next;
	}
      });
    in_forked_child = 1;
  }

  if(pid==-1) {
    Pike_error("Fork failed. errno:%d.\n", errno);
  }

  if(pid)
  {
    struct pid_status *p;

    process_started(pid);

#ifdef USE_SIGCHILD
    if(!signal_evaluator_callback)
    {
      signal_evaluator_callback=add_to_callback(&evaluator_callbacks,
						check_signals,
						0,0);
      dmalloc_accept_leak(signal_evaluator_callback);
    }
#endif

    o=fast_clone_object(pid_status_program);
    p=get_storage(o,pid_status_program);
    p->pid=pid;
    p->state=PROCESS_RUNNING;
    push_object(o);
    push_int(pid);
    mapping_insert(pid_mapping,Pike_sp-1, Pike_sp-2);
    pop_stack();
  }else{
#ifdef _REENTRANT
    /* forked copy. there is now only one thread running, this one. */
    num_threads=1;
#endif

#if defined(PIKE_USE_MACHINE_CODE) && defined(HAVE_PTHREAD_JIT_WRITE_PROTECT_NP)
    /* Workaround for spurious crashes when returning to JITed code on macOS */
    pthread_jit_write_protect_np(1);
#endif

    REINIT_FIFO(sig, unsigned char);
    REINIT_FIFO(wait,wait_data);

    /* FIXME: Ought to clear pid_mapping here. */
    call_callback(&fork_child_callback, 0);
    push_int(0);
  }
}

#endif /* HAVE_FORK */


#ifdef HAVE_KILL
/*! @decl int(0..1) kill(int pid, int signal)
 *!
 *! Send a signal to another process.
 *!
 *! Some signals and their supposed purpose:
 *! @int
 *!   @value SIGHUP
 *!     Hang-up, sent to process when user logs out.
 *!   @value SIGINT
 *!     Interrupt, normally sent by ctrl-c.
 *!   @value SIGQUIT
 *!     Quit, sent by ctrl-\.
 *!   @value SIGILL
 *!     Illegal instruction.
 *!   @value SIGTRAP
 *!     Trap, mostly used by debuggers.
 *!   @value SIGABRT
 *!     Aborts process, can be caught, used by Pike whenever something
 *!     goes seriously wrong.
 *!   @value SIGEMT
 *!     Emulation trap.
 *!   @value SIGFPE
 *!     Floating point error (such as division by zero).
 *!   @value SIGKILL
 *!     Really kill a process, cannot be caught.
 *!   @value SIGBUS
 *!     Bus error.
 *!   @value SIGSEGV
 *!     Segmentation fault, caused by accessing memory where you
 *!     shouldn't. Should never happen to Pike.
 *!   @value SIGSYS
 *!     Bad system call. Should never happen to Pike.
 *!   @value SIGPIPE
 *!     Broken pipe.
 *!   @value SIGALRM
 *!     Signal used for timer interrupts.
 *!   @value SIGTERM
 *!     Termination signal.
 *!   @value SIGUSR1
 *!     Signal reserved for whatever you want to use it for.
 *!     Note that some OSs reserve this signal for the thread library.
 *!   @value SIGUSR2
 *!     Signal reserved for whatever you want to use it for.
 *!     Note that some OSs reserve this signal for the thread library.
 *!   @value SIGCHLD
 *!     Child process died. This signal is reserved for internal use
 *!     by the Pike run-time.
 *!   @value SIGPWR
 *!     Power failure or restart.
 *!   @value SIGWINCH
 *!     Window change signal.
 *!   @value SIGURG
 *!     Urgent socket data.
 *!   @value SIGIO
 *!     Pollable event.
 *!   @value SIGSTOP
 *!     Stop (suspend) process.
 *!   @value SIGTSTP
 *!     Stop (suspend) process. Sent by ctrl-z.
 *!   @value SIGCONT
 *!     Continue suspended.
 *!   @value SIGTTIN
 *!     TTY input for background process.
 *!   @value SIGTTOU
 *!     TTY output for background process.
 *!   @value SIGVTALRM
 *!     Virtual timer expired.
 *!   @value SIGPROF
 *!     Profiling trap.
 *!   @value SIGXCPU
 *!     Out of CPU.
 *!   @value SIGXFSZ
 *!     File size limit exceeded.
 *!   @value SIGSTKFLT
 *!     Stack fault
 *! @endint
 *!
 *! @returns
 *!   @int
 *!     @value 1
 *!       Success.
 *!     @value 0
 *!       Failure. @[errno()] is set to EINVAL, EPERM or ESRCH.
 *!   @endint
 *!
 *! @note
 *!   Note that you have to use signame to translate the name of a signal
 *!   to its number.
 *!
 *!   Note that the kill function is not available on platforms that do not
 *!   support signals. Some platforms may also have signals not listed here.
 *!
 *! @seealso
 *!   @[signal()], @[signum()], @[signame()], @[fork()]
 */
static void f_kill(INT32 args)
{
  int signum;
  int pid = 0;
  int res, save_errno;

  if(args < 2)
    SIMPLE_WRONG_NUM_ARGS_ERROR("kill", 2);

  switch(TYPEOF(Pike_sp[-args]))
  {
  case PIKE_T_INT:
    pid = Pike_sp[-args].u.integer;
    break;

    /* FIXME: What about if it's an object? */

  default:
    SIMPLE_ARG_TYPE_ERROR("kill", 1, "int");
  }

  if(TYPEOF(Pike_sp[1-args]) != PIKE_T_INT)
    SIMPLE_ARG_TYPE_ERROR("kill", 2, "int");

  signum = Pike_sp[1-args].u.integer;

  PROC_FPRINTF("[%d] kill: pid=%d, signum=%d\n", getpid(), pid, signum);

  THREADS_ALLOW_UID();
  res = !kill(pid, signum);
  save_errno = errno;
  THREADS_DISALLOW_UID();

  check_signals(0,0,0);
  pop_n_elems(args);
  push_int(res);
  errno = save_errno;
}

/*! @module Process
 */

/*! @class create_process
 */

/*! @decl int(0..1) kill(int signal)
 *!
 *! Send a signal to the process.
 *!
 *! @returns
 *!   @int
 *!     @value 1
 *!       Success.
 *!     @value 0
 *!       Failure. @[errno()] is set to EINVAL, EPERM or ESRCH.
 *!   @endint
 *!
 *! @note
 *!   This function is only available on platforms that
 *!   support signals.
 *!
 *! @seealso
 *!   @[predef::kill()]
 */
static void f_pid_status_kill(INT32 args)
{
  int pid = THIS->pid;
  INT_TYPE signum;
  int res, save_errno;

  get_all_args(NULL, args, "%+", &signum);

  PROC_FPRINTF("[%d] kill: pid=%d, signum=%"PRINTPIKEINT"d\n", getpid(), pid, signum);

  THREADS_ALLOW_UID();
  res = !kill(pid, signum);
  save_errno = errno;
  THREADS_DISALLOW_UID();

  check_signals(0,0,0);
  pop_n_elems(args);
  push_int(res);
  errno = save_errno;
}

/*! @endclass
 */

/*! @endmodule
 */
#else

#ifdef __NT__
#define HAVE_KILL
static void f_kill(INT32 args)
{
  HANDLE proc = INVALID_HANDLE_VALUE;
  HANDLE tofree = INVALID_HANDLE_VALUE;

  if(args < 2)
    SIMPLE_WRONG_NUM_ARGS_ERROR("kill", 2);

  switch(TYPEOF(Pike_sp[-args]))
  {
  case PIKE_T_INT:
    tofree=proc=OpenProcess(PROCESS_TERMINATE,
			    0,
			    Pike_sp[-args].u.integer);
/*    fprintf(stderr,"PROC: %ld %ld\n",(long)proc,INVALID_HANDLE_VALUE); */
    if(!proc || proc == INVALID_HANDLE_VALUE)
    {
      errno=EPERM;
      pop_n_elems(args);
      push_int(0);
      return;
    }
    break;

  case T_OBJECT:
  {
    struct pid_status *p;
    if((p=get_storage(Pike_sp[-args].u.object,
                      pid_status_program)))
    {
      proc=p->handle;
      break;
    }
  }

  default:
    SIMPLE_ARG_TYPE_ERROR("kill", 1, "int|object");
  }

  if(TYPEOF(Pike_sp[1-args]) != PIKE_T_INT)
    SIMPLE_ARG_TYPE_ERROR("kill", 2, "int");

  switch(Pike_sp[1-args].u.integer)
  {
    case 0:
      pop_n_elems(args);
      push_int(1);
      break;

    case SIGKILL:
    {
      int i=TerminateProcess(proc,0xff);
      pop_n_elems(args);
      push_int(i);
      check_signals(0,0,0);
      break;
    }

    default:
      errno=EINVAL;
      pop_n_elems(args);
      push_int(0);
      break;
  }
  if(tofree != INVALID_HANDLE_VALUE)
    CloseHandle(tofree);
}

static void f_pid_status_kill(INT32 args)
{
  INT_TYPE signum;

  get_all_args(NULL, args, "%i", &signum);

  pop_n_elems(args);

  push_int(THIS->pid);
  push_int(signum);
  f_kill(2);
}

#endif

#endif


/*! @decl int getpid()
 *!
 *! Returns the process ID of this process.
 *!
 *! @seealso
 *!    @[System.getppid()], @[System.getpgrp()]
 */
static void f_getpid(INT32 args)
{
  pop_n_elems(args);
  push_int(getpid());
}

#ifdef HAVE_ALARM
/*! @decl int alarm(int seconds)
 *!
 *! Set an alarm clock for delivery of a signal.
 *!
 *! @[alarm()] arranges for a SIGALRM signal to be delivered to the
 *! process in @[seconds] seconds.
 *!
 *! If @[seconds] is @expr{0@} (zero), no new alarm will be scheduled.
 *!
 *! Any previous alarms will in any case be canceled.
 *!
 *! @returns
 *!   Returns the number of seconds remaining until any previously
 *!   scheduled alarm was due to be delivered, or zero if there was
 *!   no previously scheduled alarm.
 *!
 *! @note
 *!   This function is only available on platforms that support
 *!   signals.
 *!
 *! @seealso
 *!   @[ualarm()], @[signal()], @[call_out()]
 */
static void f_alarm(INT32 args)
{
  long seconds;

  if(args < 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR("alarm", 1);

  if(TYPEOF(Pike_sp[-args]) != PIKE_T_INT)
    SIMPLE_ARG_TYPE_ERROR("alarm", 1, "int");

  seconds=Pike_sp[-args].u.integer;

  pop_n_elems(args);
  push_int(alarm(seconds));
}
#endif

#if defined(HAVE_UALARM) || defined(HAVE_SETITIMER)
/*! @decl int ualarm(int useconds)
 *!
 *! Set an alarm clock for delivery of a signal.
 *!
 *! @[alarm()] arranges for a SIGALRM signal to be delivered to the
 *! process in @[useconds] microseconds.
 *!
 *! If @[useconds] is @expr{0@} (zero), no new alarm will be scheduled.
 *!
 *! Any previous alarms will in any case be canceled.
 *!
 *! @returns
 *!   Returns the number of microseconds remaining until any previously
 *!   scheduled alarm was due to be delivered, or zero if there was
 *!   no previously scheduled alarm.
 *!
 *! @note
 *!   This function is only available on platforms that support
 *!   signals.
 *!
 *! @seealso
 *!   @[alarm()], @[signal()], @[call_out()]
 */
static void f_ualarm(INT32 args)
{
#ifndef HAVE_UALARM
  struct itimerval new, old;
#endif /* !HAVE_UALARM */
  long useconds;

  if(args < 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR("ualarm", 1);

  if(TYPEOF(Pike_sp[-args]) != PIKE_T_INT)
    SIMPLE_ARG_TYPE_ERROR("ualarm", 1, "int");

  useconds=Pike_sp[-args].u.integer;

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

static struct array *atexit_functions;

static void run_atexit_functions(struct callback *UNUSED(cb),
				 void *UNUSED(arg),
				 void *UNUSED(arg2))
{
  if(atexit_functions)
  {
    int i;
    for (i = atexit_functions->size; i--;) {
      struct svalue *s = ITEM (atexit_functions) + i;
      if (callablep(s)) {
	safe_apply_svalue (s, 0, 1);
	pop_stack();
      }
    }
    free_array (atexit_functions);
    atexit_functions = 0;
  }
}

static void do_signal_exit(INT32 sig)
{
  push_int(sig);
  f_exit(1);
}

/*! @decl void atexit(function callback)
 *!
 *! This function puts the @[callback] in a queue of callbacks to
 *! call when pike exits. The call order is reversed, i.e. callbacks
 *! that have been added earlier are called after @[callback].
 *!
 *! @note
 *!   Please note that @[atexit] callbacks are not called if Pike
 *!   exits abnormally.
 *!
 *! @seealso
 *!   @[exit()], @[_exit()]
 */
void f_atexit(INT32 args)
{
  if(args < 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR("atexit", 1);

  if(!atexit_functions)
  {
#ifdef SIGHUP
    set_default_signal_handler(SIGHUP, do_signal_exit);
#endif
#ifdef SIGINT
    set_default_signal_handler(SIGINT, do_signal_exit);
#endif
#ifdef SIGBREAK
    set_default_signal_handler(SIGBREAK, do_signal_exit);
#endif
#ifdef SIGQUIT
    set_default_signal_handler(SIGQUIT, do_signal_exit);
#endif
    add_exit_callback(run_atexit_functions,0,0);
    atexit_functions=low_allocate_array(0,1);
  }

  atexit_functions=append_array(atexit_functions,Pike_sp-args);
  atexit_functions->flags |= ARRAY_WEAK_FLAG;
  pop_n_elems(args);
}

/* This function may be called from modules that may have thrashed
 * the signal handler for a specific signal. eg MariaDB and SIGPIPE.
 */
PMOD_EXPORT void restore_signal_handler(int sig)
{
  if ((TYPEOF(signal_callbacks[sig]) != PIKE_T_INT) || default_signals[sig])
  {
    sigfunctype func = receive_signal;
#ifdef USE_SIGCHILD
    if (sig == SIGCHLD) {
      func = receive_sigchild;
    }
#endif
    my_signal(sig, func);
  } else {
    switch(sig) {
      /* SIGCHLD */
#ifdef SIGCHLD
    case SIGCHLD:
#ifdef USE_SIGCHILD
      my_signal(sig, receive_sigchild);
#else
      my_signal(sig, SIG_DFL);
#endif
      break;
#endif

      /* SIGFPE */
#ifdef SIGFPE
    case SIGFPE:
#ifdef IGNORE_SIGFPE
      my_signal(sig, SIG_IGN);
#else
      my_signal(sig, SIG_DFL);
#endif
      break;
#endif

      /* SIGPIPE */
#ifdef SIGPIPE
    case SIGPIPE:
      my_signal(sig, SIG_IGN);
      break;
#endif

  /* The Java JVM has a tendency to mess with the following... */
#ifdef SIGSEGV
    case SIGSEGV:
      my_signal(sig, SIG_DFL);
      break;
#endif
#ifdef SIGBUS
    case SIGBUS:
      my_signal(sig, SIG_DFL);
      break;
#endif
#ifdef SIGXFSZ
    case SIGXFSZ:
      my_signal(sig, SIG_DFL);
      break;
#endif
#ifdef SIGILL
    case SIGILL:
      my_signal(SIGILL, SIG_DFL);
      break;
#endif

    default:
      break;
    }
  }
}

/* This fuction may be called from modules that may have thrashed
 * the signal handler state on load. eg the Java module.
 */
PMOD_EXPORT void low_init_signals(void)
{
  int e;
  /* Restore any custom signals if needed. */
  for(e=0;e<MAX_SIGNALS;e++) {
    restore_signal_handler(e);
  }
}

void init_signals(void)
{
  int e;

  INIT_FIFO(sig, unsigned char);
  INIT_FIFO(wait,wait_data);

#ifdef __NT__
  init_interleave_mutex(&handle_protection_mutex);
#endif /* __NT__ */

  for(e=0;e<MAX_SIGNALS;e++) {
    SET_SVAL_TYPE(signal_callbacks[e], PIKE_T_INT);
    SET_SVAL_SUBTYPE(signal_callbacks[e], NUMBER_NUMBER);
  }

  low_init_signals();

#ifdef USE_PID_MAPPING
  pid_mapping=allocate_mapping(2);
#endif

#ifdef USE_WAIT_THREAD
  co_init(& process_status_change);
  co_init(& start_wait_thread);
  mt_init(& wait_thread_mutex);
#endif

#if 0
  if(!signal_evaluator_callback)
  {
    signal_evaluator_callback=add_to_callback(&evaluator_callbacks,
					      check_signals,
					      0,0);
    dmalloc_accept_leak(signal_evaluator_callback);
  }
#endif

  START_NEW_PROGRAM_ID(PROCESS);
  ADD_STORAGE(struct pid_status);
  PIKE_MAP_VARIABLE("__pid", OFFSETOF(pid_status, pid),
		    tInt, T_INT, ID_PROTECTED);
#ifndef __NT__
  /* Note that several of the fields are renamed to
   * match closer with the corresponding function.
   */
  PIKE_MAP_VARIABLE("__last_signal", OFFSETOF(pid_status, sig),
		    tInt, T_INT, ID_PROTECTED);
  PIKE_MAP_VARIABLE("__flags", OFFSETOF(pid_status, flags),
		    tInt, T_INT, ID_PROTECTED|ID_PRIVATE); /* Don't touch! */
  PIKE_MAP_VARIABLE("__status", OFFSETOF(pid_status, state),
		    tInt, T_INT, ID_PROTECTED);
  PIKE_MAP_VARIABLE("__result", OFFSETOF(pid_status, result),
		    tInt, T_INT, ID_PROTECTED);
  PIKE_MAP_VARIABLE("__callback", OFFSETOF(pid_status, callback),
		    tFunc(tObjIs_PROCESS,tVoid), T_MIXED, ID_PROTECTED);
#endif /* !__NT__ */
  set_init_callback(init_pid_status);
  set_exit_callback(exit_pid_status);
  /* function(string:int) */
  ADD_FUNCTION("set_priority",f_pid_status_set_priority,tFunc(tStr,tIntPos),0);
  /* function(int(0..1)|void:int) */
  ADD_FUNCTION("wait",f_pid_status_wait,tFunc(tNone,tInt),0);
  /* function(:int) */
  ADD_FUNCTION("status",f_pid_status_status,tFunc(tNone,tInt),0);
  /* function(:int) */
  ADD_FUNCTION("last_signal", f_pid_status_last_signal,tFunc(tNone,tInt),0);
  /* function(:int) */
  ADD_FUNCTION("pid",f_pid_status_pid,tFunc(tNone,tInt),0);
#ifdef HAVE_KILL
  /* function(int:int) */
  ADD_FUNCTION("kill", f_pid_status_kill, tFunc(tInt,tInt01), 0);
#endif /* HAVE_KILL */
  /* function(array(string),void|mapping(string:mixed):object) */
  ADD_FUNCTION("create",f_create_process,tFunc(tArr(tStr) tOr(tVoid,tMap(tStr,tMix)),tObj),0);

  pid_status_program=end_program();
  add_program_constant("create_process",pid_status_program,0);

#ifdef HAVE_PTRACE
  start_new_program();
  /* NOTE: This inherit MUST be first! */
  low_inherit(pid_status_program, NULL, -1, 0, 0, NULL, NULL);
  set_init_callback(init_trace_process);
  set_exit_callback(exit_trace_process);

  /* NOTE: Several of the functions inherited from pid_status_program
   *       change behaviour if PROCESS_FLAG_TRACED is set.
   */

  /* function(int|void:void) */
  ADD_FUNCTION("cont",f_trace_process_cont,
	       tFunc(tOr(tVoid,tInt),tVoid), 0);
  /* function(:void) */
  ADD_FUNCTION("exit", f_trace_process_exit,
	       tFunc(tNone, tVoid), 0);

#if 0	/* Disabled for now. */

  start_new_program();
  Pike_compiler->new_program->flags |= PROGRAM_USES_PARENT;
  ADD_FUNCTION("`[]", f_proc_reg_index, tFunc(tMix, tInt), ID_PROTECTED);
  end_class("Registers", 0);

#endif /* 0 */

  end_class("TraceProcess", 0);
#endif /* HAVE_PTRACE */

/* function(string,int|void:int) */
  ADD_EFUN("set_priority",f_set_priority,tFunc(tStr tOr(tInt,tVoid),tInt),
           OPT_SIDE_EFFECT);

  ADD_EFUN("signal",f_signal,
	   tFunc(tInt tOr(tVoid,tFunc(tOr(tVoid,tInt),tVoid)),tMix),
	   OPT_SIDE_EFFECT);

#ifdef HAVE_KILL
/* function(int|object,int:int) */
  ADD_EFUN("kill", f_kill, tFunc(tOr(tInt,tObj) tInt,tInt01), OPT_SIDE_EFFECT);
#endif

#ifdef HAVE_FORK
/* function(void:object) */
  ADD_EFUN("fork",Pike_f_fork,tFunc(tVoid,tObj),OPT_SIDE_EFFECT);
#endif

/* function(int:string) */
  ADD_EFUN("signame",f_signame,tFunc(tInt,tStr),0);

/* function(string:int) */
  ADD_EFUN("signum",f_signum,tFunc(tStr,tInt),0);

/* function(:int) */
  ADD_EFUN("getpid",f_getpid,tFunc(tNone,tInt),OPT_EXTERNAL_DEPEND);

#ifdef HAVE_ALARM
/* function(int:int) */
  ADD_EFUN("alarm",f_alarm,tFunc(tInt,tInt),OPT_SIDE_EFFECT);
#endif

#if defined(HAVE_UALARM) || defined(HAVE_SETITIMER)
/* function(int:int) */
  ADD_EFUN("ualarm",f_ualarm,tFunc(tInt,tInt),OPT_SIDE_EFFECT);
#endif

  ADD_EFUN("atexit",f_atexit,tFunc(tMix,tVoid),OPT_SIDE_EFFECT);
}

void exit_signals(void)
{
  int e;
#ifdef USE_PID_MAPPING
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
    SET_SVAL_TYPE(signal_callbacks[e], PIKE_T_INT);
    SET_SVAL_SUBTYPE(signal_callbacks[e], NUMBER_NUMBER);
  }
}
