// Compatibility namespace
// $Id: __default.pmod,v 1.15 2005/01/08 19:57:16 grubba Exp $

#pike 7.5

//! Pike 7.4 compatibility.
//!
//! The symbols in this namespace will appear in
//! programs that use @tt{#pike 7.4@} or lower.

//! @decl inherit 7.6::

//!   Return resource usage. An error is thrown if it isn't supported
//!   or if the system fails to return any information.
//!
//! @returns
//!   Returns an array of ints describing how much resources the interpreter
//!   process has used so far. This array will have at least 29 elements, of
//!   which those values not available on this system will be zero.
//!
//!   The elements are as follows:
//!   @array
//!     @elem int user_time
//!       Time in milliseconds spent in user code.
//!     @elem int system_time
//!       Time in milliseconds spent in system calls.
//!     @elem int maxrss
//!       Maximum used resident size in kilobytes.
//!     @elem int ixrss
//!       Quote from GNU libc: An integral value expressed in
//!       kilobytes times ticks of execution, which indicates the
//!       amount of memory used by text that was shared with other
//!       processes.
//!     @elem int idrss
//!       Quote from GNU libc: An integral value expressed the same
//!       way, which is the amount of unshared memory used for data.
//!     @elem int isrss
//!       Quote from GNU libc: An integral value expressed the same
//!       way, which is the amount of unshared memory used for stack
//!       space.
//!     @elem int minor_page_faults
//!       Minor page faults, i.e. TLB misses which required no disk I/O.
//!     @elem int major_page_faults
//!       Major page faults, i.e. paging with disk I/O required.
//!     @elem int swaps
//!       Number of times the process has been swapped out entirely.
//!     @elem int block_input_op
//!       Number of block input operations.
//!     @elem int block_output_op
//!       Number of block output operations.
//!     @elem int messages_sent
//!       Number of IPC messsages sent.
//!     @elem int messages_received
//!       Number of IPC messsages received.
//!     @elem int signals_received
//!       Number of signals received.
//!     @elem int voluntary_context_switches
//!       Number of voluntary context switches (usually to wait for
//!       some service).
//!     @elem int involuntary_context_switches
//!       Number of preemptions, i.e. context switches due to expired
//!       time slices, or when processes with higher priority were
//!       scheduled.
//!     @elem int sysc
//!       Number of system calls.
//!     @elem int ioch
//!       Number of characters read and written.
//!     @elem int rtime
//!       Elapsed real time (ms).
//!     @elem int ttime
//!       Elapsed system trap (system call) time (ms).
//!     @elem int tftime
//!       Text page fault sleep time (ms).
//!     @elem int dftime
//!       Data page fault sleep time (ms).
//!     @elem int kftime
//!       Kernel page fault sleep time (ms).
//!     @elem int ltime
//!       User lock wait sleep time (ms).
//!     @elem int slptime
//!       Other sleep time (ms).
//!     @elem int wtime
//!       Wait CPU (latency) time (ms).
//!     @elem int stoptime
//!       Time spent in stopped (suspended) state.
//!     @elem int brksize
//!       Heap size.
//!     @elem int stksize
//!       Stack size.
//!   @endarray
//!
//!   The values will not be further explained here; read your system manual
//!   for more information.
//!
//! @note
//!   All values may not be present on all systems.
//!
//! @deprecated System.getrusage
//!
//! @seealso
//!   @[time()], @[System.getrusage()]
array(int) rusage() {
  mapping(string:int) m=System.getrusage();
  return ({ m->utime, m->stime, m->maxrss, m->ixrss, m->idrss,
	    m->isrss, m->minflt, m->majflt, m->nswap, m->inblock,
	    m->oublock, m->msgsnd, m->msgrcv, m->nsignals,
	    m->nvcsw, m->nivcsw, m->sysc, m->ioch, m->rtime,
	    m->ttime, m->tftime, m->dftime, m->kftime, m->ltime,
	    m->slptime, m->wtime, m->stoptime, m->brksize,
	    m->stksize });
}

mapping(string:mixed) all_constants()
{
  mapping(string:mixed) ret = predef::all_constants()+([]);
  ret->rusage = rusage;
  ret->hash = hash_7_4;
#if constant(__builtin.security)
  ret->call_with_creds = __builtin.security.call_with_creds;
  ret->get_current_creds = __builtin.security.get_current_creds;
  ret->get_object_creds = __builtin.security.get_object_creds;
#endif
#if constant(Pipe._pipe_debug)
  ret->_pipe_debug = Pipe._pipe_debug;
#endif /* constant(Pipe._pipe_debug) */
#if constant(System.getpwent)
  ret->getpwent = System.getpwent;
#endif
#if constant(System.endpwent)
  ret->endpwent = System.endpwent;
#endif
#if constant(System.setpwent)
  ret->setpwent = System.setpwent;
#endif
#if constant(System.getgrent)
  ret->getgrent = System.getgrent;
#endif
#if constant(System.endgrent)
  ret->endgrent = System.endgrent;
#endif
#if constant(System.getgrent)
  ret->setgrent = System.getgrent;
#endif
#if constant(__builtin.security)
  ret->call_with_creds = Pike.Security.call_with_creds;
  ret->get_current_creds = Pike.Security.get_current_creds;
  ret->get_object_creds = Pike.Security.get_object_creds;
#endif
#ifdef __NT__
  ret->explode_path=lambda(string x) { return replace(x,"\\","/")/"/"; };
#else
  ret->explode_path=lambda(string x) { return x/"/"; };
#endif
  return ret;
}
