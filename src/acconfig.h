/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef MACHINE_H
#define MACHINE_H

/* Building as a library? */
#undef LIBPIKE

/* Where's the master.pike file installed? */
#define DEFAULT_MASTER "@prefix@/lib/pike/master.pike"

/* Define this if you want run time self tests */
#undef PIKE_DEBUG

/* Define this if you want some extra (possibly verbose) run time self tests */
#undef PIKE_EXTRA_DEBUG

/* Define this to enable some experimental code. */
#undef PIKE_EXPERIMENTAL

/* Define to make Pike do a full cleanup at exit to detect leaks. */
#undef DO_PIKE_CLEANUP

/* Define this if you want pike to interact with valgrind. */
#undef USE_VALGRIND

/* Define this to embed DTrace probes */
#undef USE_DTRACE

/* Define this if you are going to use a memory access checker (like Purify) */
#undef __CHECKER__

/* Defined if Doug Leas malloc implementation is used. */
#undef USE_DL_MALLOC

/* Define this if you want malloc debugging */
#undef DEBUG_MALLOC

/* Define this if you want checkpoints */
#undef DMALLOC_TRACE

/* Define this if you want backtraces for C code in your dmalloc output. */
#undef DMALLOC_C_STACK_TRACE

/* Define this if you want dmalloc to keep track of freed memory. */
#undef DMALLOC_TRACK_FREE

/* With this, dmalloc will trace malloc(3) calls */
#undef ENCAPSULATE_MALLOC

/* With this, dmalloc will report leaks made by malloc(3) calls */
#undef REPORT_ENCAPSULATED_MALLOC

/* Define this to simulate dynamic module loading with static modules. */
#undef USE_SEMIDYNAMIC_MODULES

/* Define this to use the new keypair loop. */
#undef PIKE_MAPPING_KEYPAIR_LOOP

/* Enable profiling */
#undef PROFILING

/* Enable internal profiling */
#undef INTERNAL_PROFILING

/* Enable machine code stack frames */
#undef MACHINE_CODE_STACK_FRAMES

/* If possible, the expansion for a "#define short" to avoid that bison
 * uses short everywhere internally. */
#undef BISON_SHORT_EXPANSION

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

/* Define this if your ld uses Darwin-style -rpath, but your cc wants -Wl,-rpath, */
#undef USE_Wl_rpath_darwin

/* Define this if your ld uses -R, but your cc wants -Wl,-R */
#undef USE_Wl_R

/* Define this if your ld uses -rpath, but your cc -Qoption,ld,-rpath (icc) */
#undef USE_Qoption

/* Define this if your ld uses -YP, , but your cc wants -Xlinker -YP, */
#undef USE_XLINKER_YP_

/* Define this if your ld doesn't have an option to set the run path */
#undef USE_LD_LIBRARY_PATH

/* Define this on OS X to get two-level namespace support in ld */
#undef USE_OSX_TWOLEVEL_NAMESPACE

@TOP@

/* Define this if your compiler attempts to use _chkstk, but libc contains
 * __chkstk. */
#undef HAVE_BROKEN_CHKSTK

/* Define if you have a working getcwd(3) (ie one that returns a malloc()ed
 * buffer if the first argument is NULL).
 *
 * Define to 1 if the second argument being 0 causes getcwd(3) to allocate
 * a buffer of suitable size (ie never fail with ERANGE).
 *
 * Define to 0 if the second argument MUST be > 0.
 */
#undef HAVE_WORKING_GETCWD

/* Define for solaris */
#undef SOLARIS

/* Define if the closedir function returns void instead of int.  */
#undef VOID_CLOSEDIR

/* Number of args to mkdir() */
#define MKDIR_ARGS 2

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

/* Define if you have _isnan */
#undef HAVE__ISNAN

/* Define if you have isfinite */
#undef HAVE_ISFINITE

/* Define if you have fork */
#undef HAVE_FORK

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

/* Define if it is possible to allocate PROT_EXEC memory with mmap */
#undef MEXEC_USES_MMAP

/* Define if you have gethostname */
#undef HAVE_GETHOSTNAME

/* Define this if you have dlopen */
#undef HAVE_DLOPEN

/* Define if you have rint.  */
#undef HAVE_RINT

/* Define if your signals are one-shot */
#undef SIGNAL_ONESHOT

/* Define this if eval_instruction gets large on your platform. */
#undef PIKE_SMALL_EVAL_INSTRUCTION

/* Define this to use machine code */
#undef PIKE_USE_MACHINE_CODE

/* Define this if NULL ptr is not exactly 0 on your plaform. */
#undef PIKE_NULL_IS_SPECIAL

/* Define if you have the RDTSC instruction */
#undef HAVE_RDTSC

/* Define this to one of the available bytecode methods. */
#undef PIKE_BYTECODE_METHOD

/* You have gcc-type function attributes? */
#undef HAVE_FUNCTION_ATTRIBUTES

/* You have cl-type __declspec? */
#undef HAVE_DECLSPEC

/* Your va_list is a state pointer? */
#undef VA_LIST_IS_STATE_PTR

/* Defined if va_copy exists in stdarg.h. */
#undef HAVE_VA_COPY

/* Does your compiler grock 'volatile' */
#define VOLATILE volatile

/* Define to empty if your compiler doesn't support C99's restrict keyword. */
#undef restrict

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

/* define this if #include <time.h> provides an external int timezone */
#undef HAVE_EXTERNAL_TIMEZONE

/* define this if #include <time.h> provides an external int altzone */
#undef HAVE_EXTERNAL_ALTZONE

/* define this if your struct tm has a tm_gmtoff */
#undef STRUCT_TM_HAS_GMTOFF

/* define this if your struct tm has a __tm_gmtoff */
#undef STRUCT_TM_HAS___TM_GMTOFF

/* Define if you have struct timeval */
#undef HAVE_STRUCT_TIMEVAL

/* Define if you have struct sockaddr_in6 */
#undef HAVE_STRUCT_SOCKADDR_IN6

/* Define this if you have a struct iovec */
#undef HAVE_STRUCT_IOVEC

/* Define this if you have a struct msghdr */
#undef HAVE_STRUCT_MSGHDR

/* Define this if you have a struct msghdr with 'msg_control' member */
#undef HAVE_STRUCT_MSGHDR_MSG_CONTROL

/* Define this if you have a struct msghdr with 'msg_accrights' member */
#undef HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS

/* Define this to the max value of an unsigned short unless <limits.h> does.. */
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

/* Use DDLs for dynamically linked modules on NT. */
#undef USE_DLL

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

/* Define this if your struct dvpoll has a dp_setp */
#undef STRUCT_DVPOLL_HAS_DP_SETP

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

/* Define if you have the <sys/resource.h> header file.  */
#undef HAVE_SYS_RESOURCE_H

/* set this to the modifier type string to print size_t, like "" or "l" */
#undef PRINTSIZET

/* set this to the modifier type string to print ptrdiff_t, like "" or "l" */
#undef PRINTPTRDIFFT

/* set this to the modifier type string to print off_t if that type exists */
#undef PRINTOFFT

/* set this to the modifier type string to print INT64 if that type exists */
#undef PRINTINT64

/* Define when binary --disable-binary is used. */
#undef DISABLE_BINARY

/* Define to the size of the overhead for a malloc'ed block. (Slightly
 * too much is better than slightly too little.) */
#undef PIKE_MALLOC_OVERHEAD

/* Define to the page size (handled efficiently by malloc). */
#undef PIKE_MALLOC_PAGE_SIZE

/* PIKE_YES if the number reported by fallback_get_cpu_time (rusage.c)
 * is thread local, PIKE_NO if it isn't, PIKE_UNKNOWN if it couldn't
 * be established. */
#undef FB_CPU_TIME_IS_THREAD_LOCAL

@BOTTOM@

/* Define this if your <sys/sendfile.h> is broken. */
#undef HAVE_BROKEN_SYS_SENDFILE_H

/* Define this if you have a FreeBSD-style (7 args) sendfile(). */
#undef HAVE_FREEBSD_SENDFILE

/* Define this if you have a HP/UX-style (6 args) sendfile()
 * with no struct sf_hdtr. */
#undef HAVE_HPUX_SENDFILE

/* Define this if you have a MacOS X-style (6 args) sendfile()
 * with struct sf_hdtr. */
#undef HAVE_MACOSX_SENDFILE

/* Define this if you have a sendfile(2) where the length of the headers
 * are counted towards the file length argument.
 * This is the case for MacOS X and FreeBSD before FreeBSD 5.0. */
#undef HAVE_SENDFILE_HEADER_LEN_PROBLEM

/* Define this if you want to disable the use of sendfile(2). */
#undef HAVE_BROKEN_SENDFILE

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

#ifdef USE_DL_MALLOC
/* Increase alignment to 128 bits on platforms with such datatypes. */
#if defined(SIZEOF_UNSIGNED___INT128) || defined(SIZEOF_UNSIGNED___INT128_T)
#define MALLOC_ALIGNMENT	(size_t)16
#elif defined(SIZEOF_LONG_DOUBLE) && (SIZEOF_LONG_DOUBLE > 8)
#define MALLOC_ALIGNMENT	(size_t)16
#endif
#endif

/* dlmalloc has mallinfo. */
#if defined(USE_DL_MALLOC) && !defined(HAVE_MALLINFO)
#define HAVE_MALLINFO

#if defined (HAVE_MALLOC_H) && defined (HAVE_STRUCT_MALLINFO)
#include <malloc.h>
#else /* HAVE_MALLOC_H && HAVE_STRUCT_MALLINFO */

#ifndef MALLINFO_FIELD_TYPE
#define MALLINFO_FIELD_TYPE size_t
#endif  /* MALLINFO_FIELD_TYPE */

/* Needed for size_t. */
#include <stddef.h>

/* dlmalloc definition of struct mallinfo. */
struct mallinfo {
  MALLINFO_FIELD_TYPE arena;    /* non-mmapped space allocated from system */
  MALLINFO_FIELD_TYPE ordblks;  /* number of free chunks */
  MALLINFO_FIELD_TYPE smblks;   /* always 0 */
  MALLINFO_FIELD_TYPE hblks;    /* always 0 */
  MALLINFO_FIELD_TYPE hblkhd;   /* space in mmapped regions */
  MALLINFO_FIELD_TYPE usmblks;  /* maximum total allocated space */
  MALLINFO_FIELD_TYPE fsmblks;  /* always 0 */
  MALLINFO_FIELD_TYPE uordblks; /* total allocated space */
  MALLINFO_FIELD_TYPE fordblks; /* total free space */
  MALLINFO_FIELD_TYPE keepcost; /* releasable (via malloc_trim) space */
};

#endif /* HAVE_USR_INCLUDE_MALLOC_H */

#endif

#ifdef PIKE_DEBUG
#ifndef YYDEBUG
/* May also be set above. */
#define YYDEBUG 1
#endif /* YYDEBUG */
#endif

#ifdef PIKE_EXPERIMENTAL
#define MACHINE_CODE_STACK_FRAMES
#define PIKE_AMD64_VALIDATE_RSP
#endif

#endif /* MACHINE_H */
