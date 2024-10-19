/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef GLOBAL_H
#define GLOBAL_H

/* Mingw32 workarounds */
#if (defined(__WINNT__) || defined(__WIN32__)) && !defined(__NT__)
#define __NT__
#endif

#ifndef _LARGEFILE_SOURCE
#  define _FILE_OFFSET_BITS 64
#  define _TIME_BITS 64
#  define _LARGEFILE_SOURCE
/* #  define _LARGEFILE64_SOURCE 1 */	/* This one is for explicit 64bit. */
#endif

/* HPUX needs these too... */
#ifndef __STDC_EXT__
#  define __STDC_EXT__
#endif /* !__STDC_EXT__ */
#ifndef _PROTOTYPES
#  define _PROTOTYPES
#endif /* !_PROTOTYPES */

/* And Linux wants this one... */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif /* !_GNU_SOURCE */

/* This is needed for SysV stuff. */
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#ifdef __NT__
/* To get <windows.h> to stop including the entire OS,
 * we need to define this one.
 */
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

/* We also need to ensure that we get the WIN32 APIs. */
#ifndef WIN32
#define WIN32	100	/* WinNT 1.0 */
#endif

/* We want WinNT 5.0+ API's if available.
 *
 * We avoid the WinNT 6.0+ API's for now.
 */
#if !defined(_WIN32_WINDOWS) || (_WIN32_WINDOWS < 0x5ff)
#undef _WIN32_WINDOWS
#define _WIN32_WINDOWS 0x05ff
#endif

#if !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x5ff)
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x05ff
#endif

/* In later versions of the WIN32 SDKs, we also need to define this one. */
#if !defined(NTDDI_VERSION) || (NTDDI_VERSION < 0x05ffffff)
#undef NTDDI_VERSION
#define NTDDI_VERSION 0x05ffffff
#endif

#ifdef _MSC_VER
/* Microsoft C.
 *
 * Version table from
 * http://stackoverflow.com/questions/70013/how-to-detect-if-im-compiling-code-with-visual-studio-2008
 * MSVC++ 14.0 _MSC_VER == 1900 (Visual Studio 2015)
 * MSVC++ 12.0 _MSC_VER == 1800 (Visual Studio 2013)
 * MSVC++ 11.0 _MSC_VER == 1700 (Visual Studio 2012)
 * MSVC++ 10.0 _MSC_VER == 1600 (Visual Studio 2010)
 * MSVC++ 9.0  _MSC_VER == 1500 (Visual Studio 2008)
 * MSVC++ 8.0  _MSC_VER == 1400 (Visual Studio 2005)
 * MSVC++ 7.1  _MSC_VER == 1310 (Visual Studio 2003)
 * MSVC++ 7.0  _MSC_VER == 1300
 * MSVC++ 6.0  _MSC_VER == 1200
 * MSVC++ 5.0  _MSC_VER == 1100
 */
#if _MSC_VER <= 1900
/* VS 2015 or earlier do not have all C99 keywords...
 * cf https://msdn.microsoft.com/en-us/library/bw1hbe6y.aspx
 */
#define inline __inline
#if _MSC_VER <= 1800
/* The isnan() macro was added in VS 2015.
 */
#define isnan(X)	_isnan(X)
#endif /* _MSC_VER <= 1800 */
#endif /* _MSC_VER <= 1900 */
#endif /* _MSC_VER */

#endif /* __NT__ */

#if defined(__NT__) || defined(__HAIKU__)
/* NB: Defaults to 64. */
#ifndef FD_SETSIZE
/*
 * In reality: almost unlimited actually.
 */
#define FD_SETSIZE 65536
#endif /* FD_SETSIZE */

#endif /* __NT__ */

/*
 * Some structure forward declarations are needed.
 */

/* This is needed for linux */
#ifdef MALLOC_REPLACED
#define NO_FIX_MALLOC
#endif

struct array;
struct function;
struct mapping;
struct multiset;
struct object;
struct pike_string;
struct program;
struct sockaddr;
struct svalue;
struct timeval;


#ifndef CONFIGURE_TEST
/* machine.h doesn't exist if we're included from a configure test
 * program. In that case these defines will already be included. */

/* Newer autoconf adds the PACKAGE_* defines for us, regardless
 * whether we want them or not. If we're being included from a module
 * they will clash, and so we need to ensure the one for the module
 * survives, either they are defined already or get defined later.
 * Tedious work.. */
#ifndef PIKE_CORE
#ifdef PACKAGE_NAME
#define ORIG_PACKAGE_NAME PACKAGE_NAME
#undef PACKAGE_NAME
#endif
#ifdef PACKAGE_TARNAME
#define ORIG_PACKAGE_TARNAME PACKAGE_TARNAME
#undef PACKAGE_TARNAME
#endif
#ifdef PACKAGE_VERSION
#define ORIG_PACKAGE_VERSION PACKAGE_VERSION
#undef PACKAGE_VERSION
#endif
#ifdef PACKAGE_STRING
#define ORIG_PACKAGE_STRING PACKAGE_STRING
#undef PACKAGE_STRING
#endif
#ifdef PACKAGE_BUGREPORT
#define ORIG_PACKAGE_BUGREPORT PACKAGE_BUGREPORT
#undef PACKAGE_BUGREPORT
#endif
#ifdef PACKAGE_URL
#define ORIG_PACKAGE_URL PACKAGE_URL
#undef PACKAGE_URL
#endif
#endif /* PIKE_CORE */

#include "machine.h"

#ifndef PIKE_CORE
#undef PACKAGE_NAME
#ifdef ORIG_PACKAGE_NAME
#define PACKAGE_NAME ORIG_PACKAGE_NAME
#undef ORIG_PACKAGE_NAME
#endif
#undef PACKAGE_TARNAME
#ifdef ORIG_PACKAGE_TARNAME
#define PACKAGE_TARNAME ORIG_PACKAGE_TARNAME
#undef ORIG_PACKAGE_TARNAME
#endif
#undef PACKAGE_VERSION
#ifdef ORIG_PACKAGE_VERSION
#define PACKAGE_VERSION ORIG_PACKAGE_VERSION
#undef ORIG_PACKAGE_VERSION
#endif
#undef PACKAGE_STRING
#ifdef ORIG_PACKAGE_STRING
#define PACKAGE_STRING ORIG_PACKAGE_STRING
#undef ORIG_PACKAGE_STRING
#endif
#undef PACKAGE_BUGREPORT
#ifdef ORIG_PACKAGE_BUGREPORT
#define PACKAGE_BUGREPORT ORIG_PACKAGE_BUGREPORT
#undef ORIG_PACKAGE_BUGREPORT
#endif
#undef PACKAGE_URL
#ifdef ORIG_PACKAGE_URL
#define PACKAGE_URL ORIG_PACKAGE_URL
#undef ORIG_PACKAGE_URL
#endif
#endif /* PIKE_CORE */

#endif /* CONFIGURE_TEST */

/* Some identifiers used as flags in the machine.h defines. */
#define PIKE_YES	1
#define PIKE_NO		2
#define PIKE_UNKNOWN	3

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

#ifdef HAVE_DECLSPEC
#define DECLSPEC(X) __declspec(X)
#else /* !HAVE_DECLSPEC */
#define DECLSPEC(X)
#endif /* HAVE_DECLSPEC */

#ifdef HAS___BUILTIN_EXPECT
# define UNLIKELY(X) __builtin_expect( (long)(X), 0 )
# define LIKELY(X) __builtin_expect( (long)(X), 1 )
#else
# define UNLIKELY(X) X
# define LIKELY(X) X
#endif

#ifdef HAS___BUILTIN_UNREACHABLE
# define UNREACHABLE()	__builtin_unreachable()
#elif defined(HAS___ASSUME)
# define UNREACHABLE()	__assume(0)
#else
# include <setjmp.h>
# define UNREACHABLE()	longjmp(NULL, 0)
#endif

#ifdef HAS___BUILTIN_ASSUME
# define STATIC_ASSUME(X) __builtin_assume(X)
#elif defined(HAS___ASSUME)
# define STATIC_ASSUME(X) __assume(X)
#else
# define STATIC_ASSUME(X) do { if (!(X)) UNREACHABLE(); } while(0)
#endif

#ifdef HAS___BUILTIN_CONSTANT_P
# define STATIC_IS_CONSTANT(X)	__builtin_constant_p(X)
#else
# define STATIC_IS_CONSTANT(X)	0
#endif

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

/*
 * Max number of local variables in a function.
 * Currently there is no support for more than 256
 */
#define MAX_LOCAL	256

#if defined(i386) || defined(__powerpc__) || defined(__x86_64__) || (defined(__aarch64__) && defined(__ARM_FEATURE_UNALIGNED))
#ifndef HANDLES_UNALIGNED_MEMORY_ACCESS
#define HANDLES_UNALIGNED_MEMORY_ACCESS
#endif
#endif /* i386 */

/* AIX requires this to be the first thing in the file.  */
#if HAVE_ALLOCA_H
# include <alloca.h>
# ifdef __GNUC__
#  ifdef alloca
#   undef alloca
#  endif
#  define alloca __builtin_alloca
# endif
#else
# ifdef __GNUC__
#  ifdef alloca
#   undef alloca
#  endif
#  define alloca __builtin_alloca
# else
#  ifdef _AIX
#pragma alloca
#  else
#   ifndef alloca /* predefined by HP cc +Olibcalls */
void *alloca();
#   endif
#  endif
# endif
#endif

#ifdef HAVE_DEVICES_TIMER_H
/* On AmigaOS, struct timeval is defined in a variety of places
   and a variety of ways.  Making sure <devices/timer.h> is included
   first brings some amount of order to the chaos. */
#include <devices/timer.h>
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include <float.h>

#ifdef HAVE_MALLOC_H
#if !defined(__FreeBSD__) && !defined(__OpenBSD__)
/* FreeBSD and OpenBSD has <malloc.h>, but it just contains a warning... */
#include <malloc.h>
#endif /* !__FreeBSD__ && !__OpenBSD */
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/types.h>

#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

/* Get INT64, INT32, INT16, INT8, et al. */
#include "pike_int_types.h"

#define SIZE_T unsigned INT32

#define TYPE_T unsigned int
#define TYPE_FIELD unsigned INT16

#define B1_T char

#if SIZEOF_SHORT == 2
#define B2_T short
#elif SIZEOF_INT == 2
#define B2_T int
#endif

#if SIZEOF_SHORT == 4
#define B4_T short
#elif SIZEOF_INT == 4
#define B4_T int
#elif SIZEOF_LONG == 4
#define B4_T long
#endif

#if SIZEOF_INT == 8
#define B8_T int
#elif SIZEOF_LONG == 8
#define B8_T long
#elif (SIZEOF_LONG_LONG - 0) == 8
#define B8_T long long
#elif (SIZEOF___INT64 - 0) == 8
#define B8_T __int64
#elif SIZEOF_CHAR_P == 8
#define B8_T char *
#endif

#if (SIZEOF___INT128 - 0) == 16
#define B16_T __int128
#endif

/* INT_TYPE stuff */
#ifndef MAX_INT_TYPE
# ifdef WITH_SHORT_INT

#  define MAX_INT_TYPE	SHRT_MAX
#  define MIN_INT_TYPE	SHRT_MIN
#  define PRINTPIKEINT	"h"
#  define INT_ARG_TYPE	int

# elif defined(WITH_INT_INT)

#  define MAX_INT_TYPE	INT_MAX
#  define MIN_INT_TYPE	INT_MIN
#  define PRINTPIKEINT	""

# elif defined(WITH_LONG_INT)

#  define MAX_INT_TYPE	LONG_MAX
#  define MIN_INT_TYPE	LONG_MIN
#  define PRINTPIKEINT	"l"

# elif defined(WITH_LONG_LONG_INT)

#  ifdef LLONG_MAX
#   define MAX_INT_TYPE	LLONG_MAX
#   define MIN_INT_TYPE	LLONG_MIN
#  else
#   define MAX_INT_TYPE	LONG_LONG_MAX
#   define MIN_INT_TYPE	LONG_LONG_MIN
#  endif
#  define PRINTPIKEINT	"ll"

# endif
#endif

/* INT_ARG_TYPE is a type suitable for argument passing that at least
 * can hold an INT_TYPE value. */
#ifndef INT_ARG_TYPE
#define INT_ARG_TYPE INT_TYPE
#endif

#if SIZEOF_INT_TYPE - 0 == 0
# error Unsupported type chosen for native pike integers.
#endif

#if SIZEOF_INT_TYPE != 4
# define INT_TYPE_INT32_CONVERSION
#endif

/* FLOAT_TYPE stuff */
#ifdef WITH_LONG_DOUBLE_PRECISION_SVALUE

#  define PIKEFLOAT_MANT_DIG	LDBL_MANT_DIG
#  define PIKEFLOAT_DIG		LDBL_DIG
#  define PIKEFLOAT_MIN_EXP	LDBL_MIN_EXP
#  define PIKEFLOAT_MAX_EXP	LDBL_MAX_EXP
#  define PIKEFLOAT_MIN_10_EXP	LDBL_MIN_10_EXP
#  define PIKEFLOAT_MAX_10_EXP	LDBL_MAX_10_EXP
#  define PIKEFLOAT_MAX		LDBL_MAX
#  define PIKEFLOAT_MIN		LDBL_MIN
#  define PIKEFLOAT_EPSILON	LDBL_EPSILON
#  define PRINTPIKEFLOAT	"L"
#  define PIKEFLOAT_C(c)        c ## L

#elif defined(WITH_DOUBLE_PRECISION_SVALUE)

#  define PIKEFLOAT_MANT_DIG	DBL_MANT_DIG
#  define PIKEFLOAT_DIG		DBL_DIG
#  define PIKEFLOAT_MIN_EXP	DBL_MIN_EXP
#  define PIKEFLOAT_MAX_EXP	DBL_MAX_EXP
#  define PIKEFLOAT_MIN_10_EXP	DBL_MIN_10_EXP
#  define PIKEFLOAT_MAX_10_EXP	DBL_MAX_10_EXP
#  define PIKEFLOAT_MAX		DBL_MAX
#  define PIKEFLOAT_MIN		DBL_MIN
#  define PIKEFLOAT_EPSILON	DBL_EPSILON
#  define PRINTPIKEFLOAT	""
#  define PIKEFLOAT_C(c)        c

#else

#  define PIKEFLOAT_MANT_DIG	FLT_MANT_DIG
#  define PIKEFLOAT_DIG		FLT_DIG
#  define PIKEFLOAT_MIN_EXP	FLT_MIN_EXP
#  define PIKEFLOAT_MAX_EXP	FLT_MAX_EXP
#  define PIKEFLOAT_MIN_10_EXP	FLT_MIN_10_EXP
#  define PIKEFLOAT_MAX_10_EXP	FLT_MAX_10_EXP
#  define PIKEFLOAT_MAX		FLT_MAX
#  define PIKEFLOAT_MIN		FLT_MIN
#  define PIKEFLOAT_EPSILON	FLT_EPSILON
#  define PRINTPIKEFLOAT	""
#  define FLOAT_ARG_TYPE	double
#  define PIKEFLOAT_C(c)        c

#endif

/* FLOAT_ARG_TYPE is a type suitable for argument passing that at
 * least can hold a FLOAT_TYPE value. */
#ifndef FLOAT_ARG_TYPE
#define FLOAT_ARG_TYPE FLOAT_TYPE
#endif

#if SIZEOF_FLOAT_TYPE - 0 == 0
#error Unsupported type chosen for pike floats.
#endif

/* Conceptually a char is a 32 bit signed value. Implementationwise
 * that means that the shorter ones don't have space for the sign bit. */
typedef unsigned char p_wchar0;
typedef unsigned INT16 p_wchar1;
typedef signed INT32 p_wchar2;

enum size_shift {
    eightbit=0,
    sixteenbit=1,
    thirtytwobit=2,
};

typedef struct p_wchar_p
{
  void *ptr;
  enum size_shift shift;
} PCHARP;

#define WERR(...) fprintf(stderr,__VA_ARGS__)

#ifdef PIKE_DEBUG

#define DO_IF_DEBUG(X) X
#define DO_IF_DEBUG_ELSE(DEBUG, NO_DEBUG) DEBUG
#define DWERR(...) WERR(__VA_ARGS__)

/* Control assert() definition in <assert.h> */
#undef NDEBUG

/* Set of macros to simplify passing __FILE__ and __LINE__ to
 * functions only in debug mode. */
#define DLOC			__FILE__, __LINE__
#define COMMA_DLOC		, __FILE__, __LINE__
#define DLOC_DECL		const char *dloc_file, int dloc_line
#define COMMA_DLOC_DECL		, const char *dloc_file, int dloc_line
#define DLOC_ARGS		dloc_file, dloc_line
#define DLOC_ARGS_OPT		dloc_file, dloc_line
#define COMMA_DLOC_ARGS_OPT	, dloc_file, dloc_line
#define USE_DLOC_ARGS()		((void)(DLOC_ARGS_OPT))
#define DLOC_ENABLED

#else  /* !PIKE_DEBUG */

#define DO_IF_DEBUG(X)
#define DO_IF_DEBUG_ELSE(DEBUG, NO_DEBUG) NO_DEBUG
#define DWERR(...)
#define NDEBUG

#define DLOC
#define COMMA_DLOC
#define DLOC_DECL
#define COMMA_DLOC_DECL
#define DLOC_ARGS		__FILE__, __LINE__
#define DLOC_ARGS_OPT
#define COMMA_DLOC_ARGS_OPT
#define USE_DLOC_ARGS()

#endif	/* !PIKE_DEBUG */

#include <assert.h>

#if defined (PIKE_DEBUG) || defined (DO_PIKE_CLEANUP)
#define DO_IF_DEBUG_OR_CLEANUP(X) X
#else
#define DO_IF_DEBUG_OR_CLEANUP(X)
#endif

#ifdef INTERNAL_PROFILING
#define DO_IF_INTERNAL_PROFILING(X) X
#else
#define DO_IF_INTERNAL_PROFILING(X)
#endif

/* Suppress compiler warnings for unused parameters if possible. The mangling of
   argument name is required to catch when an unused argument later is used without
   removing the annotation. */
#ifndef PIKE_UNUSED_ATTRIBUTE
# ifdef __GNUC__
#  define PIKE_UNUSED_ATTRIBUTE  __attribute__((unused))
#  if (__GNUC__ > 3) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
#   define PIKE_WARN_UNUSED_RESULT_ATTRIBUTE  __attribute__((warn_unused_result))
#  else /* GCC < 3.4 */
#   define PIKE_WARN_UNUSED_RESULT_ATTRIBUTE
#  endif
# else
#  define PIKE_UNUSED_ATTRIBUTE
#  define PIKE_WARN_UNUSED_RESULT_ATTRIBUTE
# endif
#endif
#ifndef PIKE_UNUSED
# define PIKE_UNUSED(x)  PIKE_CONCAT(x,_UNUSED) PIKE_UNUSED_ATTRIBUTE
#endif
#ifndef UNUSED
# define UNUSED(x)  PIKE_UNUSED(x)
#endif
#ifdef PIKE_DEBUG
# define DEBUGUSED(x) x
#else
# define DEBUGUSED(x) PIKE_UNUSED(x)
#endif
#ifdef DEBUG_MALLOC
# define DMALLOCUSED(x) x
#else
# define DMALLOCUSED(x) PIKE_UNUSED(x)
#endif

/* Add some recognition macros for availability of #pragmas. */
#ifdef __GNUC__
# if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 4)
   /* Several #pragmas were added in GCC 4.4. */
#  define HAVE_PRAGMA_GCC_OPTIMIZE
#  define HAVE_PRAGMA_GCC_PUSH_POP_OPTIONS
#  define HAVE_PRAGMA_GCC_RESET_OPTIONS
# endif
# if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
   /* #pragma GCC diagnostic was added in GCC 4.6. */
#  define HAVE_PRAGMA_GCC_DIAGNOSTIC
# endif
# if (__GNUC__ >= 8)
#  define HAVE_PRAGMA_GCC_UNROLL
# endif
#endif

#ifdef __GNUC__
# if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
/* The hot and cold attributes are not implemented in GCC
 * versions prior to 4.3.
 */
#  define PIKE_HOT_ATTRIBUTE	ATTRIBUTE((hot))
#  define PIKE_COLD_ATTRIBUTE	ATTRIBUTE((cold))
# elif defined(__clang__)
#  define PIKE_HOT_ATTRIBUTE	ATTRIBUTE((hot))
#  define PIKE_COLD_ATTRIBUTE	ATTRIBUTE((cold))
# endif
# if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5)
/* The noclone attribute was added in GCC 4.5. */
#  define ATTRIBUTE_NOCLONE	ATTRIBUTE((noinline,noclone))
# elif defined(__clang__)
#  define ATTRIBUTE_NOCLONE	ATTRIBUTE((noinline,noclone))
# endif
#endif
#ifndef PIKE_HOT_ATTRIBUTE
# define PIKE_HOT_ATTRIBUTE
# define PIKE_COLD_ATTRIBUTE
#endif
#ifndef ATTRIBUTE_NOCLONE
/* In prior versions of GCC noinline implied noclone. */
# define ATTRIBUTE_NOCLONE	ATTRIBUTE((noinline))
#endif

/* PMOD_EXPORT exports a function / variable vfsh. */
#ifndef PMOD_EXPORT
# if defined (__NT__) && defined (USE_DLL)
#  ifdef DYNAMIC_MODULE
#   define PMOD_EXPORT __declspec(dllimport)
#  else
/* A pmod export becomes an import in the dynamic module. This means
 * that modules can't use PMOD_EXPORT for identifiers they export
 * themselves, unless they are compiled statically. */
#   define PMOD_EXPORT __declspec(dllexport)
#  endif
# elif defined(__clang__) && (defined(MAC_OS_X_VERSION_MIN_REQUIRED) || defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__))
/* According to Clang source the protected behavior is ELF-specific and not
   applicable to OS X. */
#  define PMOD_EXPORT    __attribute__ ((visibility("default")))
# elif __GNUC__ >= 4 && !defined(DISABLE_ATTRIBUTE_VISIBILITY)
#  if defined(DYNAMIC_MODULE) || defined(__HAIKU__)
#    define PMOD_EXPORT  __attribute__ ((visibility("default")))
#  else
#    define PMOD_EXPORT  __attribute__ ((visibility("protected")))
#  endif
# else
#  define PMOD_EXPORT
# endif
#endif

#ifndef PMOD_PROTO
#define PMOD_PROTO
#endif

#ifndef DO_PIKE_CLEANUP
#if defined(PURIFY) || defined(__CHECKER__) || defined(DEBUG_MALLOC)
#define DO_PIKE_CLEANUP
#endif
#endif

#ifndef HAVE_STRUCT_IOVEC
#define HAVE_STRUCT_IOVEC
struct iovec {
  void *iov_base;
  size_t iov_len;
};
#endif /* !HAVE_STRUCT_IOVEC */

#ifdef HAVE_NON_SCALAR_OFF64_T
/* Old Solaris uses unions instead of long long for 64bit values when __STDC__.
 *
 * Add some conversion functions for convenience.
 *
 * The types longlong_t and u_longlong_t are both from <sys/types.h>.
 *
 * Common types that are compatible with longlong_t:
 *   off64_t, blckcnt64_t, offset_t, diskaddr_t
 *
 * Common types that are compatible with u_longlong_t:
 *   ino64_t, fsblkcnt64_t, fsfilcnt64_t, u_offset_t, len_t
 */
static inline INT64 PIKE_UNUSED_ATTRIBUTE pike_longlong_to_int64(longlong_t val)
{
  union {
    INT64	scalar;
    longlong_t	longlong;
  } tmp;
  tmp.longlong = val;
  return tmp.scalar;
}
static inline unsigned INT64 PIKE_UNUSED_ATTRIBUTE pike_ulonglong_to_uint64(u_longlong_t val)
{
  union {
    unsigned INT64	uscalar;
    u_longlong_t	ulonglong;
  } tmp;
  tmp.ulonglong = val;
  return tmp.uscalar;
}
static inline longlong_t PIKE_UNUSED_ATTRIBUTE pike_int64_to_longlong(INT64 val)
{
  union {
    INT64	scalar;
    longlong_t	longlong;
  } tmp;
  tmp.scalar = val;
  return tmp.longlong;
}
static inline u_longlong_t PIKE_UNUSED_ATTRIBUTE pike_uint64_to_ulonglong(unsigned INT64 val)
{
  union {
    unsigned INT64	uscalar;
    u_longlong_t	ulonglong;
  } tmp;
  tmp.uscalar = val;
  return tmp.ulonglong;
}
#else /* !HAVE_NON_SCALAR_OFF64_T */
#define pike_longlong_to_int64(VAL)	((INT64)(VAL))
#define pike_ulonglong_to_uint64(VAL)	((unsigned INT64)(VAL))
#define pike_int64_to_longlong(VAL)	((INT64)(VAL))
#define pike_uint64_to_ulonglong(VAL)	((unsigned INT64)(VAL))
#endif /* HAVE_NON_SCALAR_OFF64_T */

#include "port.h"
#include "dmalloc.h"

/* Either this include must go or the include of threads.h in
 * pike_cpulib.h. Otherwise we get pesky include loops. */
/* #include "pike_cpulib.h" */

#ifdef MALLOC_DECL_MISSING
void *malloc (int);
void *realloc (void *,int);
void free (void *);
void *calloc (int,int);
#endif

#ifdef GETPEERNAME_DECL_MISSING
int getpeername (int, struct sockaddr *, int *);
#endif

#ifdef GETHOSTNAME_DECL_MISSING
void gethostname (char *,int);
#endif

#ifdef POPEN_DECL_MISSING
FILE *popen (char *,char *);
#endif

#ifdef GETENV_DECL_MISSING
char *getenv (char *);
#endif

#ifdef HAVE_SYNC_INSTRUCTION_MEMORY
/* Solaris libc has the function, but no prototype.
 *
 * NB: <asm/sunddi.h> has an inline function that shadows
 *     it when _BOOT is defined.
 *
 * There is also a corresponding kernel-api function
 * kobj_sync_instruction_memory() declared in <sys/kobj_impl.h>.
 */
void sync_instruction_memory(caddr_t v, size_t len);
#endif

/* If this define is present, error() has been renamed to Pike_error() and
 * error.h has been renamed to pike_error.h
 * Expect to see other similar defines in the future. -Hubbe
 */
#define Pike_error_present

/* Compatibility... */
#define USE_PIKE_TYPE	2

/* Used in more than one place, better put it here */
#ifdef PROFILING
#define DO_IF_PROFILING(X) X
#else
#define DO_IF_PROFILING(X)
#endif

/* #define PROFILING_DEBUG */

#ifdef PROFILING_DEBUG
#define W_PROFILING_DEBUG(...) WERR(__VA_ARGS__)
#else /* !PROFILING_DEBUG */
#define W_PROFILING_DEBUG(...)
#endif /* PROFILING_DEBUG */

#ifdef HAVE_C99_STRUCT_LITERAL_EXPR
/* This macro is used for eg type-safe struct initializers. */
#define CAST_STRUCT_LITERAL(TYPE)	(TYPE)
#else
/* Prior to C99 the literal was a special form only valid in
 * initializers (ie not in general expressions).
 */
#define CAST_STRUCT_LITERAL(TYPE)
#endif

#endif
