/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#ifndef PORT_H
#define PORT_H

#include "global.h"
#include <math.h>

#ifndef STRUCT_TIMEVAL_DECLARED
#define STRUCT_TIMEVAL_DECLARED
struct timeval;
#endif

#ifdef HAVE_ISSPACE
#define ISSPACE(X) isspace(X)
#else
#define ISSPACE(X) ("0012345678SSSSS456789012345678901" \
                     "S3456789012345678901234567890123" \
                     "45678901234567890123456789012345" \
                     "67890123456789012345678901234567" \
                     "89012345678901234567890123456789" \
                     "01234567890123456789012345678901" \
                     "23456789012345678901234567890123" \
                     "45678901234567890123456789000000"[(X)+1] == 'S')
#endif

/* Warning, these run 'C' more than once */
#define WIDE_ISSPACE(C)	(((C) < 256)?ISSPACE(C):0)
#define WIDE_ISIDCHAR(C) (((C) < 256)?isidchar(C):1)
#define WIDE_ISALNUM(C)	(((C) < 256)?isalnum(C):0)
#define WIDE_ISDIGIT(C)	(((C) < 256)?isdigit(C):0)
#define WIDE_ISLOWER(C)	(((C) < 256)?islower(C):0)


#ifndef HAVE_GETTIMEOFDAY
void GETTIMEOFDAY(struct timeval *t);
#else
#  ifdef GETTIMEOFDAY_TAKES_TWO_ARGS
#    define GETTIMEOFDAY(X) gettimeofday((X),(void *)0)
#  else
#    define GETTIMEOFDAY gettimeofday
#  endif
#endif

#ifndef HAVE_TIME
time_t TIME(time_t *);
#else
#  define TIME time
#endif

#ifndef HAVE_RINT
#define RINT(X) floor( (X) + 0.5 )
#else
#define RINT rint
#endif

long STRTOL(const char *str,char **ptr,int base);
PMOD_EXPORT double STRTOD(const char * nptr, char **endptr);

#ifndef HAVE_STRCSPN
int STRCSPN(const char *s,const char * set);
#else
#  define STRCSPN strcspn
#endif

#ifndef HAVE_STRCASECMP
PMOD_EXPORT int STRCASECMP(const char *a,const char *b);
#else
#  define STRCASECMP strcasecmp
#endif

#ifndef HAVE_STRNLEN
PMOD_EXPORT size_t STRNLEN(const char *a,size_t len);
#else
#  define STRNLEN strnlen
#endif

#ifndef HAVE_STRNCMP
PMOD_EXPORT int STRNCMP(const char *a, const char *b, size_t len);
#else
#  define STRNCMP strncmp
#endif

#ifndef HAVE_MEMSET
void *MEMSET (void *s,int c,size_t n);
#else
#  define MEMSET memset
#endif

#if 0 && defined(TRY_USE_MMX)
PMOD_EXPORT void MEMCPY(void *b,const void *a,size_t s);
# define __builtin_memcpy MEMCPY
#else
# ifndef HAVE_MEMCPY
#  ifdef HAVE_BCOPY
#    define MEMCPY(X,Y,Z) bcopy(Y,X,Z)
#    define __builtin_memcpy(X,Y,Z) bcopy(Y,X,Z)
#  else
void MEMCPY(void *b,const void *a,size_t s);
#    define __builtin_memcpy MEMCPY
#  endif
# else
#  define MEMCPY(X,Y,Z) memcpy((char*)(X),(char*)(Y),(Z))
# endif
#endif

#ifndef HAVE_MEMMOVE
PMOD_EXPORT void MEMMOVE(void *b,const void *a,size_t s);
#else
#  define MEMMOVE memmove
#endif

#ifndef HAVE_MEMCMP
PMOD_EXPORT int MEMCMP(const void *b,const void *a,size_t s);
#else
#  define MEMCMP(X,Y,Z) memcmp((char*)(X),(char*)(Y),(Z))
#endif

#ifndef HAVE_MEMCHR
PMOD_EXPORT void *MEMCHR(void *p,char c,size_t e);
#else
#  define MEMCHR(X,Y,Z) ((void *)memchr(X,Y,Z))
#endif

#ifndef HAVE_STRCHR
#  ifdef HAVE_INDEX
#    define STRCHR(X,Y) ((char *)index(X,Y))
#  else
PMOD_EXPORT char *STRCHR(char *s,char c);
#  endif
#else
#  define STRCHR strchr
#  ifdef STRCHR_DECL_MISSING
char *STRCHR(char *s,char c);
#  endif
#endif

#ifndef HAVE_STRRCHR
#  ifdef HAVE_RINDEX
#    define STRRCHR(X,Y) ((char *)rindex(X,Y))
#  else
PMOD_EXPORT char *STRRCHR(char *s,int c);
#  endif
#else
#  define STRRCHR strrchr
#endif

#ifndef HAVE_STRSTR
PMOD_EXPORT char *STRSTR(char *s1,const char *s2);
#else
#  define STRSTR strstr
#endif

#ifndef HAVE_STRTOK
PMOD_EXPORT char *STRTOK(char *s1,char *s2);
#else
#  define STRTOK strtok
#endif

#if !defined(HAVE_VFPRINTF) || !defined(HAVE_VSPRINTF)
#  include <stdarg.h>
#endif

#ifndef HAVE_VFPRINTF
PMOD_EXPORT int VFPRINTF(FILE *f,char *s,va_list args);
#else
#  define VFPRINTF vfprintf
#endif

#ifndef HAVE_VSPRINTF
PMOD_EXPORT int VSPRINTF(char *buf,char *fmt,va_list args);
#else
#  define VSPRINTF vsprintf
#endif


#ifdef EXTRACT_UCHAR_BY_CAST
#  define EXTRACT_UCHAR(p) (*(const unsigned char *)(p))
#else
#  define EXTRACT_UCHAR(p) (0xff & (int)*(p))
#endif

#ifdef EXTRACT_CHAR_BY_CAST
#  define EXTRACT_CHAR(p) (*(const signed char *)(p))
#else
static INLINE int EXTRACT_CHAR(const char *p) { return *p > 0x7f ? *p - 0x100 : *p; }
#endif

#ifdef HANDLES_UNALIGNED_MEMORY_ACCESS
#  define EXTRACT_UWORD(p) (*(unsigned INT16 *)(p))
#  define EXTRACT_WORD(p) (*(INT16 *)(p))
#  define EXTRACT_INT(p) (*(INT32 *)(p))
#else
#ifdef PIKE_DEBUG
PMOD_EXPORT unsigned INT16 EXTRACT_UWORD_(unsigned char *p);
PMOD_EXPORT INT16 EXTRACT_WORD_(unsigned char *p);
PMOD_EXPORT INT32 EXTRACT_INT_(unsigned char *p);
#else
static INLINE unsigned INT16 EXTRACT_UWORD_(unsigned char *p)
{
  unsigned INT16 a;
  MEMCPY((char *)&a,p,sizeof(a));
  return a;
}

static INLINE INT16 EXTRACT_WORD_(unsigned char *p)
{
  INT16 a;
  MEMCPY((char *)&a,p,sizeof(a));
  return a;
}

static INLINE INT32 EXTRACT_INT_(unsigned char *p)
{
  INT32 a;
  MEMCPY((char *)&a,p,sizeof(a));
  return a;
}
#endif

#define EXTRACT_UWORD(p) EXTRACT_UWORD_((unsigned char *)(p))
#define EXTRACT_WORD(p) EXTRACT_WORD_((unsigned char *)(p))
#define EXTRACT_INT(p) EXTRACT_INT_((unsigned char *)(p))

#endif

PMOD_EXPORT unsigned INT32 my_rand(void);
PMOD_EXPORT void my_srand(INT32 seed);

#ifdef OWN_GETHRTIME
void own_gethrtime_init(void);
void own_gethrtime_update(struct timeval *ptr);
long long gethrtime(void);

#define hrtime_t long long
#endif

#ifdef DOUBLE_IS_IEEE_BIG
#define DECLARE_INF static struct { unsigned char c[8]; double d[1]; } \
	inf_ = { { 0x7f, 0xf0, 0, 0, 0, 0, 0, 0 }, { 0.0 } };
#define DECLARE_NAN static struct { unsigned char c[8]; double d[1]; } \
	nan_ = { { 0x7f, 0xf8, 0, 0, 0, 0, 0, 0 }, { 0.0 } };
#define MAKE_INF(s) ((s)*inf_.d[-1])
#define MAKE_NAN() (nan_.d[-1])
#else
#ifdef DOUBLE_IS_IEEE_LITTLE
#define DECLARE_INF static struct { unsigned char c[8]; double d[1]; } \
	inf_ = { { 0, 0, 0, 0, 0, 0, 0xf0, 0x7f }, { 0.0 } };
#define DECLARE_NAN static struct { unsigned char c[8]; double d[1]; } \
	nan_ = { { 0, 0, 0, 0, 0, 0, 0xf8, 0x7f }, { 0.0 } };
#define MAKE_INF(s) ((s)*inf_.d[-1])
#define MAKE_NAN() (nan_.d[-1])
#else
#ifdef FLOAT_IS_IEEE_BIG
#define DECLARE_INF static struct { unsigned char c[4]; float f[1]; } \
	inf_ = { { 0x7f, 0x80, 0, 0 }, { 0.0 } };
#define DECLARE_NAN static struct { unsigned char c[4]; float f[1]; } \
	nan_ = { { 0x7f, 0xc0, 0, 0 }, { 0.0 } };
#define MAKE_INF(s) ((s)*inf_.f[-1])
#define MAKE_NAN() (nan_.f[-1])
#else
#ifdef FLOAT_IS_IEEE_LITTLE
#define DECLARE_INF static struct { unsigned char c[4]; float f[1]; } \
	inf_ = { { 0, 0, 0x80, 0x7f }, { 0.0 } };
#define DECLARE_NAN static struct { unsigned char c[4]; float f[1]; } \
	nan_ = { { 0, 0, 0xc0, 0x7f }, { 0.0 } };
#define MAKE_INF(s) ((s)*inf_.f[-1])
#define MAKE_NAN() (nan_.f[-1])
#else

#define DECLARE_INF
#define DECLARE_NAN

#ifdef HAVE_INFNAN
#define MAKE_INF(s) (infnan((s)*ERANGE))
#else
#ifdef HUGE_VAL
#define MAKE_INF(s) ((s)*HUGE_VAL)
#else
#ifdef PORT_DO_WARN
/* Only warn when compiling port.c; might get here when using
 * --disable-binary. */
#warning Don´t know how to create Inf on the system!
#endif
#define MAKE_INF(s) ((s)*LDEXP(1.0, 1024))
#endif /* HUGE_VAL */
#endif /* HAVE_INFNAN */

#ifdef HAVE_INFNAN
#define MAKE_NAN() (infnan(EDOM))
#else
#ifdef HAVE_NAN
/* C99 provides a portable way of generating NaN */
#define MAKE_NAN() (nan(""))
#else
#ifdef NAN
#define MAKE_NAN() (NAN)
#else
#ifdef PORT_DO_WARN
#warning Don´t know how to create NaN on this system!
#endif
#define MAKE_NAN() (0.0)
#endif /* NAN */
#endif /* HAVE_NAN */
#endif /* HAVE_INFNAN */

#endif /* FLOAT_IS_IEEE_LITTLE */
#endif /* FLOAT_IS_IEEE_BIG */
#endif /* DOUBLE_IS_IEEE_LITTLE */
#endif /* DOUBLE_IS_IEEE_BIG */

#ifdef HAVE_FREXP
#define FREXP frexp
#else
double FREXP(double x, int *exp);
#endif

#if HAVE_LDEXP
#define LDEXP ldexp
#else
double LDEXP(double x, int exp);
#endif

#endif
