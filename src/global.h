/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: global.h,v 1.80 2005/03/18 21:38:18 grubba Exp $
*/

#ifndef GLOBAL_H
#define GLOBAL_H

#if defined(__WINNT__) && !defined(__NT__)
#define __NT__
#endif

#ifndef _LARGEFILE_SOURCE
#  define _FILE_OFFSET_BITS 64
#  define _LARGEFILE_SOURCE
#  define _LARGEFILE64_SOURCE 1
#endif

/* HPUX needs these too... */
#ifndef __STDC_EXT__
#  define __STDC_EXT__
#endif /* !__STDC_EXT__ */
#ifndef _PROTOTYPES
#  define _PROTOTYPES
#endif /* !_PROTOTYPES */

/*
 * Some structure forward declarations are needed.
 */

/* This is needed for linux */
#ifdef MALLOC_REPLACED
#define NO_FIX_MALLOC
#endif

#ifndef STRUCT_PROGRAM_DECLARED
#define STRUCT_PROGRAM_DECLARED
struct program;
#endif

struct function;
#ifndef STRUCT_SVALUE_DECLARED
#define STRUCT_SVALUE_DECLARED
struct svalue;
#endif
struct sockaddr;
struct object;
struct array;
struct svalue;

#ifndef STRUCT_TIMEVAL_DECLARED
#define STRUCT_TIMEVAL_DECLARED
struct timeval;
#endif

#include "machine.h"

/*
 * Max number of local variables in a function.
 * Currently there is no support for more than 256
 */
#define MAX_LOCAL	256

/*
 * define NO_GC to get rid of garbage collection
 */
#ifndef NO_GC
#define GC2
#endif

#if defined(i386)
#ifndef HANDLES_UNALIGNED_MEMORY_ACCESS
#define HANDLES_UNALIGNED_MEMORY_ACCESS
#endif
#endif

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

#ifdef __NT__
/* We are running NT */
#define FD_SETSIZE MAX_OPEN_FILEDESCRIPTORS
#endif

#include <stdio.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#undef HAVE_STDLIB_H
#endif

#ifdef HAVE_STDDEF_H
#include <stddef.h>
#undef HAVE_STDDEF_H
#endif

#ifdef HAVE_MALLOC_H
#if !defined(__FreeBSD__)
/* FreeBSD has <malloc.h>, but it just contains a warning... */
#include <malloc.h>
#endif /* !__FreeBSD__ */
#undef HAVE_MALLOC_H
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#undef HAVE_UNISTD_H
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#undef HAVE_STRING_H
#endif

#ifdef HAVE_LIMITS_H
#include <limits.h>
#undef HAVE_LIMITS_H
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#undef HAVE_SYS_TYPES_H
#endif

#ifdef HAVE_MEMORY_H
#include <memory.h>
#undef HAVE_MEMORY_H
#endif

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#undef HAVE_WINDOWS_H
#endif


/* we here define a few types with more defined values */

#if SIZEOF_LONG >= 8
#define INT64 long
#else
#if SIZEOF___INT64 - 0 >= 8
#define INT64 __int64
#else
#if SIZEOF_LONG_LONG - 0 >= 8
#define INT64 long long
#endif
#endif
#endif

#if SIZEOF_SHORT >= 4
#define INT32 short
#else
#if SIZEOF_INT >= 4
#define INT32 int
#else
#define INT32 long
#endif
#endif

#define MAX_INT32 2147483647
#define MIN_INT32 (-2147483647-1)

#if defined (WITH_SHORT_INT)

#define MAX_INT_TYPE	SHRT_MAX
#define MIN_INT_TYPE	SHRT_MIN
#define PRINTPIKEINT	"h"

#elif defined (WITH_INT_INT)

#define MAX_INT_TYPE	INT_MAX
#define MIN_INT_TYPE	INT_MIN
#define PRINTPIKEINT	""

#elif defined (WITH_LONG_INT)

#define MAX_INT_TYPE	LONG_MAX
#define MIN_INT_TYPE	LONG_MIN
#define PRINTPIKEINT	"l"

#elif defined (WITH_LONG_LONG_INT)

#ifdef LLONG_MAX
#define MAX_INT_TYPE	LLONG_MAX
#define MIN_INT_TYPE	LLONG_MIN
#else
#define MAX_INT_TYPE	LONG_LONG_MAX
#define MIN_INT_TYPE	LONG_LONG_MIN
#endif
#define PRINTPIKEINT	"ll"

#endif

#if SIZEOF_INT_TYPE - 0 == 0
#error Unsupported type chosen for native pike integers.
#endif

#if defined (WITH_LONG_DOUBLE_PRECISION_SVALUE)
#define PRINTPIKEFLOAT	"L"
#elif defined (WITH_DOUBLE_PRECISION_SVALUE)
#define PRINTPIKEFLOAT	""
#else
#define PRINTPIKEFLOAT	""
#endif

#if SIZEOF_FLOAT_TYPE - 0 == 0
#error Unsupported type chosen for pike floats.
#endif

#define INT16 short
#define INT8 char

#ifdef INT64
#define LONGEST INT64
#else
#define LONGEST INT32
#endif

#define SIZE_T unsigned INT32

#define TYPE_T unsigned INT8
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
#elif defined(B4_T)
struct b8_t_s { B4_T x,y; };
#define B8_T struct b8_t_s
#endif

#if defined(B8_T)
struct b16_t_s { B8_T x,y; };
#define B16_T struct b16_t_s
#endif

typedef unsigned char p_wchar0;
typedef unsigned INT16 p_wchar1;
typedef unsigned INT32 p_wchar2;

typedef struct p_wchar_p
{
  p_wchar0 *ptr;
  int shift;
} PCHARP;

#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)
#define RCSID(X) \
 static char *rcsid __attribute__ ((unused)) =X
#elif __GNUC__ == 2
#define RCSID(X) \
 static char *rcsid = X; \
 static void *use_rcsid=(&use_rcsid, (void *)&rcsid)
#else
#define RCSID(X) \
 static char *rcsid = X
#endif

#ifdef PIKE_DEBUG
#define DO_IF_DEBUG(X) X
#else
#define DO_IF_DEBUG(X)
#define NDEBUG
#endif

#ifdef INTERNAL_PROFILING
#define DO_IF_INTERNAL_PROFILING(X) X
#else
#define DO_IF_INTERNAL_PROFILING(X)
#endif

#if defined(__GNUC__) && !defined(PIKE_DEBUG) && !defined(lint)
#define INLINE inline
#else
#define INLINE
#endif

/* PMOD_EXPORT exports a function / variable vfsh. 
 * Putting PMOD_PROTO in front of a prototype does nothing.
 */
#ifndef PMOD_EXPORT
#if defined(__NT__) && !defined(DYNAMIC_MODULE)
/* #define PMOD_EXPORT __declspec(dllexport) */
#define PMOD_EXPORT
#else
#define PMOD_EXPORT
#endif
#endif


/* PMOD_PROTO is essentially the same as PMOD_EXPORT, but
 * it exports the identifier even if it only a prototype.
 */
#ifndef PMOD_PROTO
#define PMOD_PROTO
#endif

#if defined(PURIFY) || defined(__CHECKER__) ||				\
  defined(DEBUG_MALLOC) || defined(USE_VALGRIND)
#define DO_PIKE_CLEANUP
#endif

#ifdef PIKE_SECURITY
#define DO_IF_SECURITY(X) X
#else
#define DO_IF_SECURITY(X)
#endif

/* Used by the AutoBuild system to mark known warnings. */
#define DO_NOT_WARN(X)	(X)

/* Some functions/macros used to avoid loss of precision warnings. */
#ifdef __ECL
static inline long PTRDIFF_T_TO_LONG(ptrdiff_t x)
{
  return DO_NOT_WARN((long)x);
}
#else /* !__ECL */
#define PTRDIFF_T_TO_LONG(x)       ((long)(x))
#endif /* __ECL */

#include "port.h"
#include "dmalloc.h"
#include "pike_cpulib.h"

#ifdef BUFSIZ
#define PROT_STDIO(x) PROT(x)
#else
#define PROT_STDIO(x) ()
#endif

#ifdef __STDC__
#define PROT(x) x
#else
#define PROT(x) ()
#endif

#ifdef MALLOC_DECL_MISSING
char *malloc PROT((int));
char *realloc PROT((char *,int));
void free PROT((char *));
char *calloc PROT((int,int));
#endif

#ifdef GETPEERNAME_DECL_MISSING
int getpeername PROT((int, struct sockaddr *, int *));
#endif

#ifdef GETHOSTNAME_DECL_MISSING
void gethostname PROT((char *,int));
#endif

#ifdef POPEN_DECL_MISSING
FILE *popen PROT((char *,char *));
#endif

#ifdef GETENV_DECL_MISSING
char *getenv PROT((char *));
#endif

#ifdef USE_CRYPT_C
char *crypt(char *, char *);
#endif /* USE_CRYPT_C */

/* If this define is present, error() has been renamed to Pike_error() and
 * error.h has been renamed to pike_error.h
 * Expect to see other similar defines in the future. -Hubbe
 */
#define Pike_error_present

/* Compatibility... */
#define USE_PIKE_TYPE	2

#ifdef PIKE_RUN_UNLOCKED
#define DO_IF_RUN_UNLOCKED(X) X
#else
#define DO_IF_RUN_UNLOCKED(X)
#endif

/* Used in more than one place, better put it here */

#if defined(PROFILING) && defined(HAVE_GETHRTIME)
#define DO_IF_PROFILING(X) X
#else
#define DO_IF_PROFILING(X)
#endif



#endif
