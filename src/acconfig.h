/*
 * $Id: acconfig.h,v 1.39 1999/03/08 05:34:34 hubbe Exp $
 */
#ifndef MACHINE_H
#define MACHINE_H

/* We must define this *always* */
#ifndef POSIX_SOURCE
#define POSIX_SOURCE
#endif

/* Where's the master.pike file installed? */
#define DEFAULT_MASTER "@prefix@/lib/pike/master.pike"

/* Define this if you want run time self tests */
#undef PIKE_DEBUG

/* Define this if you are going to use a memory access checker (like Purify) */
#undef __CHECKER__

/* Define this if you want malloc debugging */
#undef DEBUG_MALLOC

/* Define this to enable the internal Pike security system */
#undef PIKE_SECURITY

/* The following four are used by smartlink */
/* Define this if your ld sets the run path with -rpath */
#undef USE_RPATH

/* Define this if your ld sets the run path with -R */
#undef USE_R

/* Define this if your ld uses -rpath, but your cc wants -Wl,-rpath, */
#undef USE_Wl

/* Define this if your ld doesn't have an option to set the run path */
#undef USE_LD_LIBRARY_PATH

@TOP@

/* Enable profiling */
#undef PROFILING

/* Define for solaris */
#undef SOLARIS

/* Define if the closedir function returns void instead of int.  */
#undef VOID_CLOSEDIR

/* Define to 'int' if <sys/time.h> doesn't */
#undef time_t

/* Define to 'int' if <signal.h> doesn't */
#undef sig_atomic_t

/* Define as the return type of signal handlers (int or void).  */
#undef RETSIGTYPE

/* define this if igonoring SIGFPE helps with core dumps */
#undef IGNORE_SIGFPE

/* If using the C implementation of alloca, define if you know the
 * direction of stack growth for your system; otherwise it will be
 * automatically deduced at run-time.
 *	STACK_DIRECTION > 0 => grows toward higher addresses
 *	STACK_DIRECTION < 0 => grows toward lower addresses
 *	STACK_DIRECTION = 0 => direction of growth unknown
 *
 * Also used by Pike's runtime C-stack checker.
 */
#undef STACK_DIRECTION

/* If so, is it restricted to user and system time? */
#undef GETRUSAGE_RESTRICTED

/* Solaris has rusage as an ioctl on procfs */
#undef GETRUSAGE_THROUGH_PROCFS

/* Define if you have fork */
#undef HAVE_FORK

/* Define if you have isspace */
#undef HAVE_ISSPACE

/* Define if you have fpsetmask */
#undef HAVE_FPSETMASK

/* Define if you have crypt.  */
#undef HAVE_CRYPT

/* Define if you have ualarm. */
#undef HAVE_UALARM

/* Define if your ualarm takes two args.. */
#undef UALARM_TAKES_TWO_ARGS

/* Define if gettimeofday takes to arguments */
#undef GETTIMEOFDAY_TAKES_TWO_ARGS

/* Define if you have a working, 8-bit-clean memcmp */
#undef HAVE_MEMCMP

/* Define if you have gethostname */
#undef HAVE_GETHOSTNAME

/* Define if you have memmove.  */
#ifndef __CHECKER__
#undef HAVE_MEMMOVE
#endif

/* Define if you have memmem.  */
#undef HAVE_MEMMEM

/* Define this if you have dlopen */
#undef HAVE_DLOPEN

/* Define if you have ldexp.  */
#undef HAVE_LDEXP

/* Define if you have frexp.  */
#undef HAVE_FREXP

/* Define if your signals are one-shot */
#undef SIGNAL_ONESHOT

/* You have gcc stype function attributes? */
#undef HAVE_FUNCTION_ATTRIBUTES

/* Do your compiler grock 'volatile' */
#define VOLATILE volatile

/* How to extract a char and an unsigned char from a char * */
#undef EXTRACT_CHAR_BY_CAST
#undef EXTRACT_UCHAR_BY_CAST

/* Do you have IEEE floats and/or doubles (either big or little endian) ? */
#undef FLOAT_IS_IEEE_BIG
#undef FLOAT_IS_IEEE_LITTLE
#undef DOUBLE_IS_IEEE_BIG
#undef DOUBLE_IS_IEEE_LITTLE

/* Define this if strtol exists, and doesn't cut at 0x7fffffff */
#undef HAVE_WORKING_STRTOL

/* The rest of this file is just to eliminate warnings */

/* define if declaration of strchr is missing */
#undef STRCHR_DECL_MISSING

/* define if declaration of malloc is missing */
#undef MALLOC_DECL_MISSING

/* define if declaration of getpeername is missing */
#undef GETPEERNAME_DECL_MISSING

/* define if declaration of gethostname is missing */
#undef GETHOSTNAME_DECL_MISSING

/* define if declaration of popen is missing */
#undef POPEN_DECL_MISSING

/* define if declaration of getenv is missing */
#undef GETENV_DECL_MISSING

/* Define if we can declare 'extern char **environ' */
#undef DECLARE_ENVIRON

/* What byteorder does your machie use most machines use 4321, PC use 1234 */
#define PIKE_BYTEORDER 0

/* Number of possible filedesriptors */
#define MAX_OPEN_FILEDESCRIPTORS 1024

/* define this if #include <time.h> provides an external int timezone */
#undef HAVE_EXTERNAL_TIMEZONE

/* define this if your struct tm has a tm_gmtoff */
#undef STRUCT_TM_HAS_GMTOFF

/* Define if you have struct timeval */
#undef HAVE_STRUCT_TIMEVAL

/* Define this to the max value of an unsigned short unles <limits.h> does.. */
#undef USHRT_MAX

/* Define these if you are going to use threads */
#undef _REENTRANT
#undef _THREAD_SAFE

/* Define this if you want the UNIX taste of threads */
#undef _UNIX_THREADS

/* Define this if you want the POSIX taste of threads */
#undef _MIT_POSIX_THREADS

/* Define this if you want the SGI sproc taste of threads */
#undef _SGI_SPROC_THREADS
#undef _SGI_MP_SOURCE

/* Define this if you have Windows NT threads */
#undef NT_THREADS

/* Define this if your pthreads have pthread_condattr_default */
#undef HAVE_PTHREAD_CONDATTR_DEFAULT

/* Define this if you need to use &pthread_condattr_default in cond_init() */
#undef HAVE_PTHREAD_CONDATTR_DEFAULT_AIX

/* Define if you have the pthread_attr_setstacksize function.  */
#undef HAVE_PTHREAD_ATTR_SETSTACKSIZE

/* Define if you have the pthread_cond_init function.  */
#undef HAVE_PTHREAD_COND_INIT

/* Define if you have the pthread_yield function.  */
#undef HAVE_PTHREAD_YIELD

/* Define if your OS has the union wait. */
#undef HAVE_UNION_WAIT

/* Define if you have isgraph */
#undef HAVE_ISGRAPH

/* Define if your cpp supports the ANSI concatenation operator ## */
#undef HAVE_ANSI_CONCAT

/* Define if you don't have F_SETFD, or it doesn't work */
#undef HAVE_BROKEN_F_SETFD

/* Define if your cpp supports K&R-style concatenation */
#undef HAVE_KR_CONCAT

/* Use poll() instead of select() ? */
#undef HAVE_AND_USE_POLL

/* This works on Solaris or any UNIX where
 * waitpid can report ECHILD when running more than one at once
 * (or any UNIX where waitpid actually works)
 */
#undef USE_WAIT_THREAD

/* This works on Linux or any UNIX where
 * waitpid works or where threads and signals bugs in
 * less annoying ways than Solaris.
 */
#undef USE_SIGCHILD

/* Enable code to handle Out-Of-Band data */
#undef WITH_OOB

/* Enable individual tracing of threads */
#undef TRACE_THREADS

/* Enable tracing of the compiler */
#undef YYDEBUG

/* Define if your compiler has a symbol __func__ */
#undef HAVE_WORKING___FUNC__

/* Define if your compiler has a symbol __FUNCTION__ */
#undef HAVE_WORKING___FUNCTION__

/* The last argument to accept() is an ACCEPT_SIZE_T * */
#define ACCEPT_SIZE_T	int

@BOTTOM@

/* NT stuff */
#undef HAVE_GETSYSTEMTIMEASFILETIME
#undef HAVE_LOADLIBRARY
#undef HAVE_FREELIBRARY
#undef HAVE_GETPROCADDRESS
#undef DL_EXPORT

/* How to set a socket non-blocking */
#undef USE_IOCTL_FIONBIO
#undef USE_IOCTLSOCKET_FIONBIO
#undef USE_FCNTL_O_NDELAY
#undef USE_FCNTL_FNDELAY
#undef USE_FCNTL_O_NONBLOCK

/* We want to use errno later */
#ifdef _SGI_SPROC_THREADS
/* Magic define of _SGI_MP_SOURCE above might redefine errno below */
#include <errno.h>
#if defined(HAVE_OSERROR) && !defined(errno)
#define errno (oserror())
#endif /* HAVE_OSERROR && !errno */
#endif /* _SGI_SPROC_THREADS */

#ifdef HAVE_FUNCTION_ATTRIBUTES
#define ATTRIBUTE(X) __attribute__ (X)
#else
#define ATTRIBUTE(X)
#endif

#ifndef HAVE_WORKING___FUNC__
#ifdef HAVE_WORKING___FUNCTION__
#define __func__	__FUNCTION__
#else /* !HAVE_WORKING___FUNCTION__ */
#define __func__	"unknown"
#endif /* HAVE_WORKING___FUNCTION__ */
#endif /* !HAVE_WORKING___FUNC__ */

/* NOTE:
 *    PIKE_CONCAT doesn't get defined if there isn't any way to
 *    concatenate symbols
 */
#ifdef HAVE_ANSI_CONCAT
#define PIKE_CONCAT(X,Y)	X##Y
#define PIKE_CONCAT3(X,Y,Z)	X##Y##Z
#define PIKE_CONCAT4(X,Y,Z,Q)	X##Y##Z##Q
#else
#ifdef HAVE_KR_CONCAT
#define PIKE_CONCAT(X,Y)	X/**/Y
#define PIKE_CONCAT3(X,Y,Z)	X/**/Y/**/Z
#define PIKE_CONCAT4(X,Y,Z,Q)	X/**/Y/**/Z/**/Q
#endif /* HAVE_KR_CONCAT */
#endif /* HAVE_ANSI_CONCAT */

#endif /* MACHINE_H */
