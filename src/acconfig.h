#ifndef MACHINE_H
#define MACHINE_H

/* Where's the master.pike file installed? */
#define DEFAULT_MASTER "@prefix@/lib/pike/master.pike"

/* Define this if you want run time self tests */
#undef DEBUG

@TOP@

/* Define for solaris */
#undef SOLARIS

/* Define if the closedir function returns void instead of int.  */
#undef VOID_CLOSEDIR

/* Define to 'int' if <sys/time.h> doesn't */
#undef time_t

/* Define as the return type of signal handlers (int or void).  */
#undef RETSIGTYPE

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at run-time.
	STACK_DIRECTION > 0 => grows toward higher addresses
	STACK_DIRECTION < 0 => grows toward lower addresses
	STACK_DIRECTION = 0 => direction of growth unknown
 */
#undef STACK_DIRECTION

/* If so, is it restricted to user and system time? */
#undef GETRUSAGE_RESTRICTED

/* Solaris has rusage as an ioctl on procfs */
#undef GETRUSAGE_THROUGH_PROCFS

/* Define if you have isspace */
#undef HAVE_ISSPACE

/* Define if you have crypt.  */
#undef HAVE_CRYPT

/* Define if you have ualarm (else put ualarm.o in the makefile).  */
#undef HAVE_UALARM

/* Define if your ualarm takes two args.. */
#undef UALARM_TAKES_TWO_ARGS

/* Define if gettimeofday takes to arguments */
#undef GETTIMEOFDAY_TAKES_TWO_ARGS

/* Define if you have a working, 8-bit-clean memcmp */
#undef HAVE_MEMCMP

/* Define if you have memmove.  */
#ifndef __CHECKER__
#undef HAVE_MEMMOVE
#endif

/* Define if you have memmem.  */
#undef HAVE_MEMMEM

/* Define if your signals are one-shot */
#undef SIGNAL_ONESHOT

/* You have gcc stype function attributes? */
#undef HAVE_FUNCTION_ATTRIBUTES

/* Do your compiler grock 'volatile' */
#define VOLATILE volatile

/* How to extract a char and an unsigned char from a char * */
#undef EXTRACT_CHAR_BY_CAST
#undef EXTRACT_UCHAR_BY_CAST

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

/* What byteorder does your machie use most machines use 4321, PC use 1234 */
#define BYTEORDER 0

/* Number of possible filedesriptors */
#define MAX_OPEN_FILEDESCRIPTORS 1024

/* Value of first constant defined by byacc/bison/yacc or whatever you use. */
#define F_OFFSET 257

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

@BOTTOM@

/* How to set a socket non-blocking */
#undef USE_IOCTL_FIONBIO
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

#endif /* MACHINE_H */
