#ifndef PORT_H
#define PORT_H

#include "types.h"

#ifndef HAVE_STRTOL
long STRTOL(char *str,char **ptr,int base);
#else
#  define STRTOL strtol
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
char *MEMCPY(char *b,const char *a,int s);
#    define __builtin_memcpy MEMCPY
#  endif
#else
#  define MEMCPY memcpy
#endif

#ifndef HAVE_MEMMOVE
char *MEMMOVE(char *b,const char *a,int s);
#else
#  define MEMCPY memmove
#endif

#ifndef HAVE_MEMCMP
int MEMCMP(const char *b,const char *a,int s);
#else
#  define MEMCMP memcmp
#endif

#ifndef HAVE_MEMCHR
char *MEMCHR(char *p,char c,int e);
#else
#  define MEMCHR(X,Y,Z) ((char *)memchr(X,Y,Z))
#endif

#ifndef HAVE_MEMMEM
char *MEMMEM(char *needle,
	     SIZE_T needlelen,
	     char *haystack,
	     SIZE_T haystacklen);
#else
#  define MEMMEM memmem
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
static inline int EXTRACT_UCHAR(p) { return *p < 0 ? *p + 0x100 : *p; }
#endif

#ifdef EXTRACT_CHAR_BY_CAST
#  define EXTRACT_CHAR(p) (*(signed char *)(p))
#else
static inline int EXTRACT_CHAR(p) { return *p > 0x7f ? *p - 0x100 : *p; }
#endif

#ifdef HANDLES_UNALIGNED_MEMORY_ACCESS
#  define EXTRACT_UWORD(p) (*(unsigned INT16 *)(p))
#  define EXTRACT_WORD(p) (*(INT16 *)(p))
#  define EXTRACT_INT(p) (*(INT32 *)(p))
#else
#ifdef DEBUG
unsigned INT16 EXTRACT_UWORD(unsigned char *p);
INT16 EXTRACT_WORD(unsigned char *p);
INT32 EXTRACT_INT(unsigned char *p);
#else
static INLINE unsigned INT16 EXTRACT_UWORD(unsigned char *p)
{
  return (EXTRACT_UCHAR(p)<<8) + EXTRACT_UCHAR(p+1);
}

static INLINE INT16 EXTRACT_WORD(unsigned char *p)
{
  return (EXTRACT_CHAR(p)<<8) | EXTRACT_UCHAR(p+1);
}

static INLINE INT32 EXTRACT_INT(unsigned char *p)
{
  return (EXTRACT_WORD(p)<<16) | EXTRACT_UWORD(p+2);
}
#endif
#endif

long get_current_time(void);
unsigned long my_rand(void);
void my_srand(int seed);

#endif
