/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: port.h,v 1.20 1998/11/22 11:03:13 hubbe Exp $
 */
#ifndef PORT_H
#define PORT_H

#include "global.h"

#ifndef STRUCT_TIMEVAL_DECLARED
#define STRUCT_TIMEVAL_DECLARED
struct timeval;
#endif
#ifdef HAVE_ISSPACE
#define ISSPACE(X) isspace(X)
#else
#define ISSPACE(X) ("0012345678SSSSS456789012345678901S3456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789000000"[(X)+1] == 'S')
#endif

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

#if defined(HAVE_STRTOL) && defined(HAVE_WORKING_STRTOL)
#  define STRTOL strtol
#else
long STRTOL(char *str,char **ptr,int base);
#endif

#ifndef HAVE_STRTOD
double STRTOD(char * nptr, char **endptr);
#else
#  define STRTOD strtod
#endif

#ifndef HAVE_STRCSPN
int STRCSPN(const char *s,const char * set);
#else
#  define STRCSPN strcspn
#endif

#ifndef HAVE_STRCASECMP
int STRCASECMP(const char *a,const char *b);
#else
#  define STRCASECMP strcasecmp
#endif

#ifndef HAVE_MEMSET
char *MEMSET (char *s,int c,int n);
#else
#  define MEMSET memset
#endif

#ifndef HAVE_MEMCPY
#  ifdef HAVE_BCOPY
#    define MEMCPY(X,Y,Z) bcopy(Y,X,Z)
#    define __builtin_memcpy(X,Y,Z) bcopy(Y,X,Z)
#  else
void MEMCPY(void *b,const void *a,int s);
#    define __builtin_memcpy MEMCPY
#  endif
#else
#  define MEMCPY(X,Y,Z) memcpy((char*)(X),(char*)(Y),(Z))
#endif

#ifndef HAVE_MEMMOVE
void MEMMOVE(void *b,const void *a,int s);
#else
#  define MEMMOVE memmove
#endif

#ifndef HAVE_MEMCMP
int MEMCMP(const void *b,const void *a,int s);
#else
#  define MEMCMP(X,Y,Z) memcmp((char*)(X),(char*)(Y),(Z))
#endif

#ifndef HAVE_MEMCHR
char *MEMCHR(char *p,char c,int e);
#else
#  define MEMCHR(X,Y,Z) ((char *)memchr(X,Y,Z))
#endif

#ifndef HAVE_STRCHR
#  ifdef HAVE_INDEX
#    define STRCHR(X,Y) ((char *)index(X,Y))
#  else
char *STRCHR(char *s,char c);
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
char *STRRCHR(char *s,int c);
#  endif
#else
#  define STRRCHR strrchr
#endif

#ifndef HAVE_STRSTR
char *STRSTR(char *s1,const char *s2);
#else
#  define STRSTR strstr
#endif

#ifndef HAVE_STRTOK
char *STRTOK(char *s1,char *s2);
#else
#  define STRTOK strtok
#endif

#if !defined(HAVE_VFPRINTF) || !defined(HAVE_VSPRINTF)
#  include <stdarg.h>
#endif

#ifndef HAVE_VFPRINTF
int VFPRINTF(FILE *f,char *s,va_list args);
#else
#  define VFPRINTF vfprintf
#endif

#ifndef HAVE_VSPRINTF
int VSPRINTF(char *buf,char *fmt,va_list args);
#else
#  define VSPRINTF vsprintf
#endif


#ifdef EXTRACT_UCHAR_BY_CAST
#  define EXTRACT_UCHAR(p) (*(unsigned char *)(p))
#else
#  define EXTRACT_UCHAR(p) (0xff & (int)*(p))
#endif

#ifdef EXTRACT_CHAR_BY_CAST
#  define EXTRACT_CHAR(p) (*(signed char *)(p))
#else
static INLINE int EXTRACT_CHAR(char *p) { return *p > 0x7f ? *p - 0x100 : *p; }
#endif

#ifdef HANDLES_UNALIGNED_MEMORY_ACCESS
#  define EXTRACT_UWORD(p) (*(unsigned INT16 *)(p))
#  define EXTRACT_WORD(p) (*(INT16 *)(p))
#  define EXTRACT_INT(p) (*(INT32 *)(p))
#else
#ifdef PIKE_DEBUG
unsigned INT16 EXTRACT_UWORD_(unsigned char *p);
INT16 EXTRACT_WORD_(unsigned char *p);
INT32 EXTRACT_INT_(unsigned char *p);
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

unsigned long my_rand(void);
void my_srand(long seed);

#endif
