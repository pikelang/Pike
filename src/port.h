/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef PORT_H
#define PORT_H

#include "global.h"
#include "pike_memory.h"

#ifdef __MINGW32__
/******************************************************/
/* First we must ensure that all defines are in mingw */
/******************************************************/
#ifndef PROCESSOR_PPC_601
#define PROCESSOR_PPC_601       601
#endif

#ifndef PROCESSOR_PPC_604
#define PROCESSOR_PPC_603       603
#endif

#ifndef PROCESSOR_PPC_604
#define PROCESSOR_PPC_604       604
#endif

#ifndef PROCESSOR_PPC_620
#define PROCESSOR_PPC_620       620
#endif

#ifndef PROCESSOR_OPTIL
#define PROCESSOR_OPTIL         0x494f  /* MSIL */
#endif

#ifndef PROCESSOR_ARCHITECTURE_MSIL
#define PROCESSOR_ARCHITECTURE_MSIL 8
#endif

#ifndef PROCESSOR_ARCHITECTURE_AMD64
#define PROCESSOR_ARCHITECTURE_AMD64            9
#endif

#ifndef PROCESSOR_ARCHITECTURE_IA32_ON_WIN64
#define PROCESSOR_ARCHITECTURE_IA32_ON_WIN64    10
#endif

#ifndef PROCESSOR_HITACHI_SH3
#define PROCESSOR_HITACHI_SH3   10003   /* Windows CE */
#endif

#ifndef PROCESSOR_HITACHI_SH3E
#define PROCESSOR_HITACHI_SH3E  10004   /* Windows CE */
#endif

#ifndef PROCESSOR_HITACHI_SH4
#define PROCESSOR_HITACHI_SH4   10005   /* Windows CE */
#endif

#ifndef PROCESSOR_SHx_SH3
#define PROCESSOR_SHx_SH3       103     /* Windows CE */
#endif

#ifndef PROCESSOR_SHx_SH4
#define PROCESSOR_SHx_SH4       104     /* Windows CE */
#endif

#ifndef PROCESSOR_STRONGARM
#define PROCESSOR_STRONGARM     2577    /* Windows CE - 0xA11 */
#endif

#ifndef PROCESSOR_ARM720
#define PROCESSOR_ARM720        1824    /* Windows CE - 0x720 */
#endif

#ifndef PROCESSOR_ARM820
#define PROCESSOR_ARM820        2080    /* Windows CE - 0x820 */
#endif

#ifndef PROCESSOR_ARM920
#define PROCESSOR_ARM920        2336    /* Windows CE - 0x920 */
#endif

#ifndef PROCESSOR_ARM_7TDMI
#define PROCESSOR_ARM_7TDMI     70001   /* Windows CE */
#endif

#ifndef LOGON32_LOGON_NETWORK
#define LOGON32_LOGON_NETWORK 3
#endif

/* FP_CLASS compleation */

/* Now for some functions */
#define Emulate_GetLongPathName GetLongPathNameA

#endif /* __MINGW32__ */

#ifndef HAVE_GETTIMEOFDAY
PMOD_EXPORT void GETTIMEOFDAY(struct timeval *t);
#else
#  ifdef GETTIMEOFDAY_TAKES_TWO_ARGS
#    define GETTIMEOFDAY(X) gettimeofday((X),NULL)
#  else
#    define GETTIMEOFDAY gettimeofday
#  endif
#endif

#ifdef HAVE__SNPRINTF
/* In WIN32 snprintf is known as _snprintf... */
#define snprintf _snprintf
#endif

#ifndef HAVE_RINT
#define rintf(X) floorf ((X) + 0.5)
#define rint(X) floor( (X) + 0.5 )
#define rintl(X) floorl ((X) + 0.5)
#endif

#ifndef HAVE_STRCASECMP
PMOD_EXPORT int STRCASECMP(const char *a,const char *b);
#else
#  define STRCASECMP(A,B) strcasecmp(A,B)
#endif

#define SNPRINTF snprintf

#define HAVE_STRCHR 1
#define STRCHR strchr
#ifdef STRCHR_DECL_MISSING
char *strchr(const char *s,int c);
#endif

#ifndef HAVE_STRDUP
#undef strdup
#ifdef HAVE__STRDUP
#define strdup(X) _strdup(X)
#endif
#endif

#ifdef EXTRACT_UCHAR_BY_CAST
#  define EXTRACT_UCHAR(p) (*(const unsigned char *)(p))
#else
#  define EXTRACT_UCHAR(p) (0xff & (int)*(p))
#endif

#ifdef EXTRACT_CHAR_BY_CAST
#  define EXTRACT_CHAR(p) (*(const signed char *)(p))
#else
static inline int EXTRACT_CHAR(const char *p) { return *p > 0x7f ? *p - 0x100 : *p; }
#endif

/* Implementation in pike_memory.h */
#define EXTRACT_UWORD(p) (get_unaligned16(p))
#define EXTRACT_WORD(p) ((INT16)get_unaligned16(p))
#define EXTRACT_INT(p) ((INT32)get_unaligned32(p))

PMOD_EXPORT void sysleep(double left);

PMOD_EXPORT /*@null@*/ void *pike_realloc(void *ptr, size_t sz);

#ifdef OWN_GETHRTIME
void own_gethrtime_init(void);
void own_gethrtime_update(struct timeval *ptr);
long long gethrtime(void);

#define hrtime_t long long
#endif

#ifdef __MINGW32__
#ifndef HAVE__DOSMAPERR
void _dosmaperr(int x);
#endif
#endif

#ifdef __clang__
#define PIKE_CLANG_FEATURE(x)   __has_feature(x)
#define PIKE_CLANG_BUILTIN(x)   __has_builtin(x)
#else
#define PIKE_CLANG_FEATURE(x)	(0)
#define PIKE_CLANG_BUILTIN(x)   (0)
#endif

#ifdef USE_CRYPT_C
char *crypt(const char *, const char *);
#endif /* USE_CRYPT_C */

#endif
