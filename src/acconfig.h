/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: acconfig.h,v 1.137 2004/03/10 17:10:22 grubba Exp $
*/

#ifndef MACHINE_H
#define MACHINE_H

/* We must define this *always* */
#ifndef POSIX_SOURCE
#define POSIX_SOURCE
#endif

/* Get more declarations in GNU libc. */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

/* Get more declarations from AIX libc. */
#ifndef _ALL_SOURCE
#define _ALL_SOURCE
#endif

/* Where's the master.pike file installed? */
#define DEFAULT_MASTER "@prefix@/lib/pike/master.pike"

/* Define this if you want run time self tests */
#undef PIKE_DEBUG

/* Define this if you want pike to interact with valgrind. */
#undef USE_VALGRIND

/* Define this if you are going to use a memory access checker (like Purify) */
#undef __CHECKER__

/* Define this if you want malloc debugging */
#undef DEBUG_MALLOC

/* Define this if you want checkpoints */
#undef DMALLOC_TRACE

/* With this, dmalloc will trace malloc(3) calls */
#undef ENCAPSULATE_MALLOC

/* With this, dmalloc will report leaks made by malloc(3) calls */
#undef REPORT_ENCAPSULATED_MALLOC

/* Define this to enable the internal Pike security system */
#undef PIKE_SECURITY

/* Define this to enable the internal bignum conversion */
#undef AUTO_BIGNUM

/* Define this to enable experimental code for multicpu machines */
#undef PIKE_RUN_UNLOCKED

/* Define this if you want to enable the shared nodes mode of the optimizer. */
#undef SHARED_NODES

/* Define this to use the new keypair loop. */
#undef PIKE_MAPPING_KEYPAIR_LOOP

/* Define this to use the new multiset implementation. */
#undef PIKE_NEW_MULTISETS

/* Define this to get portable dumped bytecode. */
#undef PIKE_PORTABLE_BYTECODE

/* Enable profiling */
#undef PROFILING

/* Enable internal profiling */
#undef INTERNAL_PROFILING

/* The following USE_* are used by smartlink */
/* Define this if your ld sets the run path with -rpath */
#undef USE_RPATH

/* Define this if your ld sets the run path with -R */
#undef USE_R

/* Define this if your ld sets the run path with -YP, */
#undef USE_YP_

/* Define this if your ld sets the run path with +b */
#undef USE_PLUS_b

/* Define this if your ld uses -rpath, but your cc wants -Wl,-rpath, */
#undef USE_Wl

/* Define this if your ld uses -R, but your cc wants -Wl,-R */
#undef USE_Wl_R

/* Define this if your ld uses -rpath, but your cc -Qoption,ld,-rpath (icc) */
#undef USE_Qoption

/* Define this if your ld uses -YP, , but your cc wants -Xlinker -YP, */
#undef USE_XLINKER_YP_

/* Define this if your ld doesn't have an option to set the run path */
#undef USE_LD_LIBRARY_PATH

/* Define if your tcc supports #pragma TenDRA longlong type allow. */
#undef HAVE_PRAGMA_TENDRA_LONGLONG

/* Define if your tcc supports #pragma TenDRA set longlong type : long long. */
#undef HAVE_PRAGMA_TENDRA_SET_LONGLONG_TYPE

/* The worlds most stringent C compiler? */
#ifdef __TenDRA__
/* We want to be able to use 64bit arithmetic */
#ifdef HAVE_PRAGMA_TENDRA_LONGLONG
#pragma TenDRA longlong type allow
#endif /* HAVE_PRAGMA_TENDRA_LONGLONG */
#ifdef HAVE_PRAGMA_TENDRA_SET_LONGLONG_TYPE
#pragma TenDRA set longlong type : long long
#endif /* HAVE_PRAGMA_TENDRA_SET_LONGLONG_TYPE */

#ifdef _NO_LONGLONG
#undef _NO_LONGLONG
#endif /* _NO_LONGLONG */
#endif /* __TenDRA__ */

@TOP@

/* Define this if your compiler attempts to use _chkstk, but libc contains
 * __chkstk. */
#undef HAVE_BROKEN_CHKSTK

/* Define for solaris */
#undef SOLARIS

/* Define if the closedir function returns void instead of int.  */
#undef VOID_CLOSEDIR

/* Define to 'int' if <sys/time.h> doesn't */
#undef time_t

/* Define to 'short' if <sys/types.h> doesn't */
#undef pri_t

/* Define to 'int' if <sys/types.h> doesn't */
#undef uid_t

/* Define to 'int' if <sys/types.h> doesn't */
#undef gid_t

/* Define to 'int' if <sys/types.h> doesn't */
#undef pid_t

/* Define to 'unsigned long' if <sys/types.h> or <stddef.h> doesn't */
#undef size_t

/* Define to 'long' if <sys/types.h> of <stddef.h> doesn't */
#undef ptrdiff_t

/* Define to 'long' if <sys/types.h> doesn't */
#undef off_t

/* Define to 'int' if <signal.h> doesn't */
#undef sig_atomic_t

/* Define as the return type of signal handlers (int or void).  */
#undef RETSIGTYPE

/* define this if igonoring SIGFPE helps with core dumps */
#undef IGNORE_SIGFPE

/* define if you want to use double precision floats instead of single */
#undef WITH_DOUBLE_PRECISION_SVALUE

/* define if you want to use long double precision floats */
#undef WITH_LONG_DOUBLE_PRECISION_SVALUE

/* define to the type of pike floats */
#undef FLOAT_TYPE

/* define to the size of pike floats */
#undef SIZEOF_FLOAT_TYPE

/* force this type upon ints */
#undef WITH_LONG_INT
#undef WITH_LONG_LONG_INT
#undef WITH_INT_INT

/* define to the type of pike primitive ints */
#undef INT_TYPE

/* define to the size of pike primitive ints */
#undef SIZEOF_INT_TYPE

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

/* Define this to the number of KB in the initial stack,
 * currently this is 1 Mb on FreeBSD, 2Mb on Linux and
 * unlimited (undefined) everywhere else
 */
#undef Pike_INITIAL_STACK_SIZE

/* If so, is it restricted to user and system time? */
#undef GETRUSAGE_RESTRICTED

/* Solaris has rusage as an ioctl on procfs */
#undef GETRUSAGE_THROUGH_PROCFS

/* So has True64, but no useful information in prstatus_t */
#undef GETRUSAGE_THROUGH_PROCFS_PRS

/* Define if you have infnan */
#undef HAVE_INFNAN

/* Define if you have _isnan */
#undef HAVE__ISNAN

/* Define if you have fork */
#undef HAVE_FORK

/* Define if you have isspace */
#undef HAVE_ISSPACE

/* Define if you have fpsetmask */
#undef HAVE_FPSETMASK

/* Define if you have fpsetround */
#undef HAVE_FPSETROUND

/* Define if you have isless */
#undef HAVE_ISLESS

/* Define if you have isunordered */
#undef HAVE_ISUNORDERED

/* Define if you have crypt.  */
#undef HAVE_CRYPT

/* Define if you have ualarm. */
#undef HAVE_UALARM

/* Define if your ualarm takes two args. */
#undef UALARM_TAKES_TWO_ARGS

/* Define if your ptrace takes four args. */
#undef PTRACE_TAKES_FOUR_ARGS

/* Define if argument 3 to ptrace is a pointer type. */
#undef PTRACE_ADDR_TYPE_IS_POINTER

/* Define if gettimeofday takes to arguments */
#undef GETTIMEOFDAY_TAKES_TWO_ARGS

/* Define if realloc(NULL, SZ) works. */
#undef HAVE_WORKING_REALLOC_NULL

/* Define if gethrvtime works (i.e. even without ptime). */
#undef HAVE_WORKING_GETHRVTIME

/* Define if you have gethrtime */
#undef HAVE_GETHRTIME

/* Can we make our own gethrtime? */
#undef OWN_GETHRTIME

/* ... by using the RDTSC instruction? */
#undef OWN_GETHRTIME_RDTSC

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

/* Define if you have memset.  */
#undef HAVE_MEMSET

/* Define if you have memcpy.  */
#undef HAVE_MEMCPY

/* Define if you have strcoll */
#undef HAVE_STRCOLL

/* Define this if you have dlopen */
#undef HAVE_DLOPEN

/* Define if you have ldexp.  */
#undef HAVE_LDEXP

/* Define if you have rint.  */
#undef HAVE_RINT

/* Define if you have frexp.  */
#undef HAVE_FREXP

/* Define if your signals are one-shot */
#undef SIGNAL_ONESHOT

/* Define if you have gcc-style computed goto, and want to use them. */
#undef HAVE_COMPUTED_GOTO

/* Define this to use machine code */
#undef PIKE_USE_MACHINE_CODE

/* Define this to one of the available bytecode methods. */
#undef PIKE_BYTECODE_METHOD

/* You have gcc-type function attributes? */
#undef HAVE_FUNCTION_ATTRIBUTES

/* You have cl-type __declspec? */
#undef HAVE_DECLSPEC

/* Do your compiler grock 'volatile' */
#define VOLATILE volatile

/* Define this if your compiler doesn't allow cast of void * to function pointer */
#undef NO_CAST_TO_FUN

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

/* define if you are using crypt.c. */
#undef USE_CRYPT_C

/* Define if we can declare 'extern char **environ' */
#undef DECLARE_ENVIRON

/* The byteorder your machine use, most use 4321, PC use 1234 */
#define PIKE_BYTEORDER 0

/* What alignment do pointers need */
#define PIKE_POINTER_ALIGNMENT 4

/* Assembler prefix for general purpose registers */
#undef PIKE_CPU_REG_PREFIX

/* Number of possible filedesriptors */
#define MAX_OPEN_FILEDESCRIPTORS 1024

/* define this if #include <time.h> provides an external int timezone */
#undef HAVE_EXTERNAL_TIMEZONE

/* define this if your struct tm has a tm_gmtoff */
#undef STRUCT_TM_HAS_GMTOFF

/* define this if your struct tm has a __tm_gmtoff */
#undef STRUCT_TM_HAS___TM_GMTOFF

/* Define if you have struct timeval */
#undef HAVE_STRUCT_TIMEVAL

/* Define if you have struct sockaddr_in6 */
#undef HAVE_STRUCT_SOCKADDR_IN6

/* Define this to the max value of an unsigned short unles <limits.h> does.. */
#undef USHRT_MAX

/* Define these if you are going to use threads */
#undef PIKE_THREADS
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

/* Define this if your THREAD_T type is a pointer type. */
#undef PIKE_THREAD_T_IS_POINTER

/* Define to the flag to get an error checking mutex, if supported. */
#undef PIKE_MUTEX_ERRORCHECK

/* Define to the flag to get a recursive mutex, if supported. */
#undef PIKE_MUTEX_RECURSIVE

/* Define this if your pthreads have pthread_condattr_default */
#undef HAVE_PTHREAD_CONDATTR_DEFAULT

/* Define this if you need to use &pthread_condattr_default in cond_init() */
#undef HAVE_PTHREAD_CONDATTR_DEFAULT_AIX

/* Define if you have the pthread_attr_setstacksize function.  */
#undef HAVE_PTHREAD_ATTR_SETSTACKSIZE

/* Define if you have the pthread_atfork function.  */
#undef HAVE_PTHREAD_ATFORK

/* Define if you have the pthread_cond_init function.  */
#undef HAVE_PTHREAD_COND_INIT

/* Define if you have the pthread_yield function.  */
#undef HAVE_PTHREAD_YIELD

/* Define if you have the pthread_yield_np function.  */
#undef HAVE_PTHREAD_YIELD_NP

/* Hack for stupid glibc linuxthreads */
#undef HAVE_PTHREAD_INITIAL_THREAD_BOS

/* Define if your OS has the union wait. */
#undef HAVE_UNION_WAIT

/* Define if you have isgraph */
#undef HAVE_ISGRAPH

/* Define if your cpp supports the ANSI concatenation operator ## */
#undef HAVE_ANSI_CONCAT

/* Define if you don't have F_SETFD, or it doesn't work */
#undef HAVE_BROKEN_F_SETFD

/* Define if your thread implementation doesn't propagate euid & egid. */
#undef HAVE_BROKEN_LINUX_THREAD_EUID

/* Define if your cpp supports K&R-style concatenation */
#undef HAVE_KR_CONCAT

/* Use poll() instead of select() ? */
#undef HAVE_AND_USE_POLL

/* Enable use of /dev/epoll on Linux. */
#undef WITH_EPOLL

/* Define to the poll device (eg "/dev/poll") */
#undef PIKE_POLL_DEVICE

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

/* Enable tracing of the compiler */
#undef YYDEBUG

/* Define if your compiler has a symbol __func__ */
#undef HAVE_WORKING___FUNC__

/* Define if your compiler has a symbol __FUNCTION__ */
#undef HAVE_WORKING___FUNCTION__

/* The last argument to accept() is an ACCEPT_SIZE_T * */
#define ACCEPT_SIZE_T	int

/* Can we compile in MMX support? */
#undef TRY_USE_MMX

/* Define if you have the <sys/resource.h> header file.  */
#undef HAVE_SYS_RESOURCE_H

/* set this to the modifier type string to print size_t, like "" or "l" */
#undef PRINTSIZET

/* set this to the modifier type string to print ptrdiff_t, like "" or "l" */
#undef PRINTPTRDIFFT

/* set this to the modifier type string to print INT64 if that type exists */
#undef PRINTINT64

/* Define if the compiler understand union initializations. */
#undef HAVE_UNION_INIT

/* Define when binary --disable-binary is used. */
#undef DISABLE_BINARY

/* Define to the size of the overhead for a malloc'ed block. (Slightly
 * too much is better than slightly too little.) */
#undef PIKE_MALLOC_OVERHEAD

/* Define to the page size (handled efficiently by malloc). */
#undef PIKE_MALLOC_PAGE_SIZE

/* PIKE_YES if the number reported by get_cpu_time (rusage.c) is
 * thread local, PIKE_NO if it isn't, PIKE_UNKNOWN if it couldn't be
 * established. */
#undef CPU_TIME_IS_THREAD_LOCAL

@BOTTOM@

/* Define to the size of the c-stack for new threads */
#undef PIKE_THREAD_C_STACK_SIZE

/* NT stuff */
#undef HAVE_GETSYSTEMTIMEASFILETIME
#undef HAVE_LOADLIBRARY
#undef HAVE_FREELIBRARY
#undef HAVE_GETPROCADDRESS
#undef DL_EXPORT
#undef USE_MY_WIN32_DLOPEN

/* CygWin kludge. */
#if defined(HAVE_UNISTD_H) && defined(HAVE_WINDOWS_H)
#undef HAVE_WINDOWS_H
#undef HAVE_WINBASE_H
#undef HAVE_WINSOCK_H
#undef HAVE_WINSOCK2_H
#undef HAVE_FD_FLOCK
#endif /* HAVE_SYS_UNISTD_H && HAVE_WINDOWS_H */

/* How to set a socket non-blocking */
#undef USE_IOCTL_FIONBIO
#undef USE_IOCTLSOCKET_FIONBIO
#undef USE_FCNTL_O_NDELAY
#undef USE_FCNTL_FNDELAY
#undef USE_FCNTL_O_NONBLOCK

/* How well is OOB TCP working?
 * -1 = unknown
 *  0 = doesn't seem to be working at all
 *  1 = very limited functionality
 *  2 = should be working as long as you are cautious
 *  3 = works excellently
 */
#define PIKE_OOB_WORKS -1

/* We want to use errno later */
#ifdef _SGI_SPROC_THREADS
/* Magic define of _SGI_MP_SOURCE above might redefine errno below */
#include <errno.h>
#if defined(HAVE_OSERROR) && !defined(errno)
#define errno (oserror())
#endif /* HAVE_OSERROR && !errno */
#endif /* _SGI_SPROC_THREADS */

/* This macro is only provided for compatibility with
 * Windows PreRelease. Use ALIGNOF() instead!
 * (Needed for va_arg().)
 */
#ifndef __alignof
#define __alignof(X) ((size_t)&(((struct { char ignored_ ; X fooo_; } *)0)->fooo_))
#endif /* __alignof */

#ifdef HAVE_FUNCTION_ATTRIBUTES
#define ATTRIBUTE(X) __attribute__ (X)
#else
#define ATTRIBUTE(X)
#endif

#ifdef HAVE_DECLSPEC
#define DECLSPEC(X) __declspec(X)
#else /* !HAVE_DECLSPEC */
#define DECLSPEC(X)
#endif /* HAVE_DECLSPEC */

#ifndef HAVE_WORKING___FUNC__
#ifdef HAVE_WORKING___FUNCTION__
#define __func__	__FUNCTION__
#else /* !HAVE_WORKING___FUNCTION__ */
#define __func__	"unknown"
#endif /* HAVE_WORKING___FUNCTION__ */
#endif /* !HAVE_WORKING___FUNC__ */

#ifndef HAVE_WORKING_REALLOC_NULL
#define realloc(PTR, SZ)	pike_realloc(PTR,SZ)
#endif

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

#define TOSTR(X)	#X
#define DEFINETOSTR(X)	TOSTR(X)

/* Some identifiers used as flags in the defines above. */
#define PIKE_YES	1
#define PIKE_NO		2
#define PIKE_UNKNOWN	3

#endif /* MACHINE_H */
