/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: port.c,v 1.81 2005/01/02 00:36:41 nilsson Exp $
*/

/*
 * This file defines things that may ought to already exist on the
 * target system, but are missing in some ports. There are #ifdef's
 * that will hopefully take care of everything.
 */

#define PORT_DO_WARN

#include "global.h"
#include "time_stuff.h"
#include "pike_error.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef __MINGW32__
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#endif

#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <float.h>
#include <string.h>

#ifndef HAVE_ISSPACE
PMOD_EXPORT const char Pike_isspace_vector[] =
  "0012345678SSSSS456789012345678901"
   "S3456789012345678901234567890123"
   "45678901234567890123456789012345"
   "67890123456789012345678901234567"
   "89012345678901234567890123456789"
   "01234567890123456789012345678901"
   "23456789012345678901234567890123"
   "45678901234567890123456789000000";
#endif

#ifdef sun
time_t time PROT((time_t *));
#endif

#if (SIZEOF_LONG == 4) && defined(_LP64)
/* Kludge for gcc and the system header files not using the same model... */
#undef LONG_MIN
#undef LONG_MAX
#undef ULONG_MAX
#define LONG_MIN	INT_MIN
#define LONG_MAX	INT_MAX
#define ULONG_MAX	UINT_MAX
#endif

#ifndef HAVE_GETTIMEOFDAY

#ifdef HAVE_GETSYSTEMTIMEASFILETIME
#include <winbase.h>

void GETTIMEOFDAY(struct timeval *t)
{
  union {
    unsigned __int64 ft_scalar;
    FILETIME ft_struct;
  } ft;

  GetSystemTimeAsFileTime(&ft.ft_struct);

  ft.ft_scalar /= 10;
#ifndef __GNUC__
  ft.ft_scalar -= 11644473600000000i64;
  t->tv_sec = (long)(ft.ft_scalar / 1000000i64);
  t->tv_usec = (long)(ft.ft_scalar % 1000000i64);
#else
  ft.ft_scalar -= 11644473600000000;
  t->tv_sec = (long)(ft.ft_scalar / 1000000);
  t->tv_usec = (long)(ft.ft_scalar % 1000000);
#endif
}

#else
void GETTIMEOFDAY(struct timeval *t)
{
  t->tv_sec=(long)time(0);
  t->tv_usec=0;
}
#endif
#endif

#ifndef HAVE_TIME
time_t TIME(time_t *t)
{
  struct timeval tv;
  GETTIMEOFDAY(&tv);
  if(t) *t=tv.tv_sec;
  return tv.tv_sec;
}
#endif

static unsigned INT32 RandSeed1 = 0x5c2582a4;
static unsigned INT32 RandSeed2 = 0x64dff8ca;

static unsigned INT32 slow_rand(void)
{
  RandSeed1 = ((RandSeed1 * 13 + 1) ^ (RandSeed1 >> 9)) + RandSeed2;
  RandSeed2 = (RandSeed2 * RandSeed1 + 13) ^ (RandSeed2 >> 13);
  return RandSeed1;
}

static void slow_srand(INT32 seed)
{
  RandSeed1 = (seed - 1) ^ 0xA5B96384UL;
  RandSeed2 = (seed + 1) ^ 0x56F04021UL;
}

#define RNDBUF 250
#define RNDSTEP 7
#define RNDJUMP 103

static unsigned INT32 rndbuf[ RNDBUF ];
static int rnd_index;

PMOD_EXPORT void my_srand(INT32 seed)
{
  int e;
  unsigned INT32 mask;

  slow_srand(seed);
  
  rnd_index = 0;
  for (e=0;e < RNDBUF; e++) rndbuf[e]=slow_rand();

  mask = (unsigned INT32) -1;

  for (e=0;e< (int)sizeof(INT32)*8 ;e++)
  {
    int d = RNDSTEP * e + 3;
    rndbuf[d % RNDBUF] &= mask;
    mask>>=1;
    rndbuf[d % RNDBUF] |= (mask+1);
  }
}

PMOD_EXPORT unsigned INT32 my_rand(void)
{
  if( ++rnd_index == RNDBUF) rnd_index=0;
  return rndbuf[rnd_index] += rndbuf[rnd_index+RNDJUMP-(rnd_index<RNDBUF-RNDJUMP?0:RNDBUF)];
}

#ifndef CONFIGURE_TEST

PMOD_EXPORT /*@null@*/ void *pike_realloc(void *ptr, size_t sz)
{
  if (!ptr) return malloc(sz);
#ifndef HAVE_WORKING_REALLOC_NULL
#undef realloc
#endif	/* !HAVE_WORKING_REALLOC_NULL */
  return realloc(ptr, sz);
#ifndef HAVE_WORKING_REALLOC_NULL
#define realloc(PTR, SZ)	pike_realloc((PTR),(SZ))
#endif	/* !HAVE_WORKING_REALLOC_NULL */
}

#endif	/* !CONFIGURE_TEST */

#define DIGIT(x)	(isdigit(x) ? (x) - '0' : \
			islower(x) ? (x) + 10 - 'a' : (x) + 10 - 'A')
#define MBASE	('z' - 'a' + 1 + 10)

long STRTOL(const char *str,char **ptr,int base)
{
  /* Note: Code duplication in STRTOL_PCHARP and pcharp_to_svalue_inumber. */

  unsigned long val, mul_limit;
  int c;
  int xx, neg = 0, add_limit, overflow = 0;

  if (ptr != (char **)NULL)
    *ptr = (char *)str;		/* in case no number is formed */
  if (base < 0 || base > MBASE)
    return (0);			/* base is invalid -- should be a fatal error */
  if (!isalnum(c = (int)*str)) {
    while (ISSPACE(c))
      c = (int)*++str;
    switch (c) {
    case '-':
      neg++;
      /*@fallthrough@*/
    case '+':
      c = (int)*++str;
    }
  }

  if (base == 0) {
    if (c != '0')
      base = 10;
    else if (str[1] == 'x' || str[1] == 'X')
      base = 16;
    else
      base = 8;
  }

  /*
   * for any base > 10, the digits incrementally following
   *	9 are assumed to be "abc...z" or "ABC...Z"
   */
  if (!isalnum(c) || (xx = DIGIT(c)) >= base)
    return (0);			/* no number formed */
  if (base == 16 && c == '0' && isxdigit(((const unsigned char *)str)[2]) &&
      (str[1] == 'x' || str[1] == 'X'))
    c = *(str += 2);		/* skip over leading "0x" or "0X" */

  mul_limit = LONG_MAX / base;
  add_limit = (int) (LONG_MAX % base);
  
  if (neg) {
    if (++add_limit == base) {
      mul_limit++;
      add_limit = 0;
    }
  }

  for (val = (unsigned long)DIGIT(c);
       isalnum(c = *++str) && (xx = DIGIT(c)) < base; ) {
    if (val > mul_limit || (val == mul_limit && xx > add_limit))
      overflow = 1;
    else
      val = base * val + xx;
  }

  if (ptr != (char **)NULL)
    *ptr = (char *)str;
  if (overflow) {
    errno = ERANGE;
    return neg ? LONG_MIN : LONG_MAX;
  }
  else {
    if (neg)
      return (long)(~val + 1);
    else
      return (long) val;
  }
}

#ifndef HAVE_STRCASECMP
PMOD_EXPORT int STRCASECMP(const char *a,const char *b)
{
  int ac, bc;

  while(1)
  {
    ac=*(a++);
    bc=*(b++);

    if(ac && isupper(ac)) ac=tolower(ac);
    if(bc && isupper(bc)) bc=tolower(bc);
    if(ac - bc) return ac-bc;
    if(!ac) return 0;
  }
}
#endif

#ifndef HAVE_STRNLEN
size_t STRNLEN(const char *s, size_t maxlen)
{
  char *tmp=MEMCHR(s,0,maxlen);
  if(tmp) return tmp-s;
  return maxlen;
}
#endif

#ifndef HAVE_STRNCMP
int STRNCMP(const char *a, const char *b, size_t maxlen)
{
  size_t alen=STRNLEN(a,maxlen);
  size_t blen=STRNLEN(b,maxlen);
  int ret=MEMCMP(a,b, alen < blen ? alen : blen);
  if(ret) return ret;
  return DO_NOT_WARN((int)(alen - blen));
}
#endif

#ifndef HAVE_MEMSET
void *MEMSET(void *s,int c,size_t n)
{
  char *t;
  for(t=s;n;n--) *(t++)=c;
  return s;
}
#endif

#if (0 && defined(TRY_USE_MMX)) || !defined(HAVE_MEMCPY) && !defined(HAVE_BCOPY)
#ifdef TRY_USE_MMX
#ifdef HAVE_MMX_H
#include <mmx.h>
#else
#include <asm/mmx.h>
#endif
#endif
PMOD_EXPORT void MEMCPY(void *bb,const void *aa,size_t s)
{
  if(!s) return;
#ifdef TRY_USE_MMX
  {
    extern int try_use_mmx;
    if( (s>64) && !(((int)bb)&7) && !(((int)aa)&7) && try_use_mmx )
    {
      unsigned char *source=(char *)aa;
      unsigned char *dest=(char *)bb;

/*       fprintf(stderr, "mmx memcpy[%d]\n",s); */
      while( s > 64 )
      {
        movq_m2r(*source, mm0);      source += 8;
        movq_m2r(*source, mm1);      source += 8;
        movq_m2r(*source, mm2);      source += 8;
        movq_m2r(*source, mm3);      source += 8;
        movq_m2r(*source, mm4);      source += 8;
        movq_m2r(*source, mm5);      source += 8;
        movq_m2r(*source, mm6);      source += 8;
        movq_m2r(*source, mm7);      source += 8;
        movq_r2m(mm0,*dest);         dest += 8;
        movq_r2m(mm1,*dest);         dest += 8;
        movq_r2m(mm2,*dest);         dest += 8;
        movq_r2m(mm3,*dest);         dest += 8;
        movq_r2m(mm4,*dest);         dest += 8;
        movq_r2m(mm5,*dest);         dest += 8;
        movq_r2m(mm6,*dest);         dest += 8;
        movq_r2m(mm7,*dest);         dest += 8;
        s -= 64;
      }
      if( s > 31 )
      {
        movq_m2r(*source, mm0);      source += 8;
        movq_m2r(*source, mm1);      source += 8;
        movq_m2r(*source, mm2);      source += 8;
        movq_m2r(*source, mm3);      source += 8;
        movq_r2m(mm0,*dest);         dest += 8;
        movq_r2m(mm1,*dest);         dest += 8;
        movq_r2m(mm2,*dest);         dest += 8;
        movq_r2m(mm3,*dest);         dest += 8;
        s -= 32;
      }
      if( s > 15 )
      {
        movq_m2r(*source, mm0);      source += 8;
        movq_m2r(*source, mm1);      source += 8;
        movq_r2m(mm0,*dest);         dest += 8;
        movq_r2m(mm1,*dest);         dest += 8;
        s -= 16;
      }
      if( s > 7 )
      {
        movq_m2r(*source, mm0);      source += 8;
        movq_r2m(mm0,*dest);         dest += 8;
        s -= 8;
      }
      emms();
      while( s )
      {
        *(dest++) = *(source++);
        s-=1;
      }
    }
    else 
    {
#endif
#ifdef HAVE_MEMCPY
      /*     fprintf(stderr, "plain ol' memcpy\n"); */
      memcpy( bb, aa, s );
#else
      {
	char *b=(char *)bb;
	char *a=(char *)aa;
	for(;s;s--) *(b++)=*(a++);
      }
#endif
#ifdef TRY_USE_MMX
    }
  }
#endif
}
#endif

#ifndef HAVE_MEMMOVE
PMOD_EXPORT void MEMMOVE(void *b,const void *aa,size_t s)
{
  char *t=(char *)b;
  char *a=(char *)aa;
  if(a>t)
    for(;s;s--) *(t++)=*(a++);
  else
    if(a<t)
      for(t+=s,a+=s;s;s--) *(--t)=*(--a);
}
#endif


#ifndef HAVE_MEMCMP
PMOD_EXPORT int MEMCMP(const void *bb,const void *aa,size_t s)
{
  unsigned char *a=(unsigned char *)aa;
  unsigned char *b=(unsigned char *)bb;
  for(;s;s--,b++,a++)
  {
    if(*b!=*a)
    {
      if(*b<*a) return -1;
      return 1;
    }
  }
  return 0;
}
#endif

#ifndef HAVE_MEMCHR
PMOD_EXPORT void *MEMCHR(const void *p,char c,size_t e)
{
  const char *t = p;
  while(e--) if(*(t++)==c) return t-1;
  return (char *)NULL;
}
#endif


#if !defined(HAVE_INDEX) && !defined(HAVE_STRCHR)
PMOD_EXPORT char *STRCHR(char *s,int c)
{
  for(;*s;s++) if(*s==c) return s;
  return NULL;
}
#endif

#if !defined(HAVE_RINDEX) && !defined(HAVE_STRRCHR)
PMOD_EXPORT char *STRRCHR(char *s,int c)
{
  char *p;
  for(p=NULL;*s;s++) if(*s==c) p=s;
  return p;
}
#endif

#ifndef HAVE_STRSTR
PMOD_EXPORT char *STRSTR(char *s1,const char *s2)
{
  for(;*s1;s1++)
  {
    const char *p1,*p2;
    for(p1=s1,p2=s2;*p2;p1++,p2++) if(*p2!=*p1) break;
    if(!*p2) return s1;
  }
  return NULL;
}
#endif

#ifndef HAVE_STRTOK
static char *temporary_for_strtok;
PMOD_EXPORT char *STRTOK(char *s1,char *s2)
{
  if(s1!=NULL) temporary_for_strtok=s1;
  for(s1=temporary_for_strtok;*s1;s1++)
  {
    char *p1,*p2;
    for(p1=s1,p2=s2;*p1==*p2;p1++,p2++)
    {
      if(!*(p2+1))
      {
        char *retval;
        *s1=0;
        retval=temporary_for_strtok;
        temporary_for_strtok=p1+1;
        return(retval);
      }
    }
  }
  return NULL;
}
#endif

/* Convert NPTR to a double.  If ENDPTR is not NULL, a pointer to the
   character after the last one used in the number is put in *ENDPTR.  */
PMOD_EXPORT double STRTOD(const char * nptr, char **endptr)
{
  /* Note: Code duplication in STRTOD_PCHARP. */

  register const unsigned char *s;
  short int sign;

  /* The number so far.  */
  double num;

  int got_dot;      /* Found a decimal point.  */
  int got_digit;    /* Seen any digits.  */

  /* The exponent of the number.  */
  long int exponent;

  if (nptr == NULL)
  {
    errno = EINVAL;
    goto noconv;
  }

  s = (const unsigned char *)nptr;

  /* Eat whitespace.  */
  while (ISSPACE(*s)) ++s;

  /* Get the sign.  */
  sign = *s == '-' ? -1 : 1;
  if (*s == '-' || *s == '+')
    ++s;

  num = 0.0;
  got_dot = 0;
  got_digit = 0;
  exponent = 0;
  for (;; ++s)
  {
    if (isdigit(*s))
    {
      got_digit = 1;

      /* Make sure that multiplication by 10 will not overflow.  */
      if (num > DBL_MAX * 0.1)
	/* The value of the digit doesn't matter, since we have already
	   gotten as many digits as can be represented in a `double'.
	   This doesn't necessarily mean the result will overflow.
	   The exponent may reduce it to within range.
	   
	   We just need to record that there was another
	   digit so that we can multiply by 10 later.  */
	++exponent;
      else
	num = (num * 10.0) + (*s - '0');

      /* Keep track of the number of digits after the decimal point.
	 If we just divided by 10 here, we would lose precision.  */
      if (got_dot)
	--exponent;
    }
    else if (!got_dot && (*s == '.'))
      /* Record that we have found the decimal point.  */
      got_dot = 1;
    else
      /* Any other character terminates the number.  */
      break;
  }

  if (!got_digit)
    goto noconv;

  if (tolower(*s) == 'e')
    {
      /* Get the exponent specified after the `e' or `E'.  */
      int save = errno;
      char *end;
      long int exp;

      errno = 0;
      ++s;
      exp = STRTOL((const char *)s, &end, 10);
      if (errno == ERANGE)
      {
	/* The exponent overflowed a `long int'.  It is probably a safe
	   assumption that an exponent that cannot be represented by
	   a `long int' exceeds the limits of a `double'.  */
	/* NOTE: Don't trust the value returned from STRTOL.
	 * We need to find the sign of the exponent by hand.
	 */
	while(ISSPACE(*s)) {
	  s++;
	}
	if (endptr != NULL)
	  *endptr = end;
	if (*s == '-')
	  goto underflow;
	else
	  goto overflow;
      }
      else if (end == (char *)s)
	/* There was no exponent.  Reset END to point to
	   the 'e' or 'E', so *ENDPTR will be set there.  */
	end = (char *) s - 1;
      errno = save;
      s = (unsigned char *)end;
      exponent += exp;
    }

  if (endptr != NULL)
    *endptr = (char *) s;

  if (num == 0.0)
    return 0.0 * sign;

  /* Multiply NUM by 10 to the EXPONENT power,
     checking for overflow and underflow.  */

  if (exponent < 0)
  {
    if (num < DBL_MIN * pow(10.0, (double) -exponent))
      goto underflow;

    num /= pow(10.0, (double) -exponent);
  }
  else if (exponent > 0)
  {
    if (num > DBL_MAX * pow(10.0, (double) -exponent))
      goto overflow;
    num *= pow(10.0, (double) exponent);
  }

  return num * sign;

 overflow:
  /* Return an overflow error.  */
  errno = ERANGE;
  return HUGE_VAL * sign;

 underflow:
  /* Return an underflow error.  */
#if 0
  if (endptr != NULL)
    *endptr = (char *) nptr;
#endif
  errno = ERANGE;
  return 0.0;
  
 noconv:
  /* There was no number.  */
  if (endptr != NULL)
    *endptr = (char *) nptr;
  return 0.0;
}

#ifndef HAVE_VSPRINTF
PMOD_EXPORT int VSPRINTF(char *buf,const char *fmt,va_list args)
{
  char *b=buf;
  char *s;

  int tmpA;
  char fmt2[120];
  char *fmt2p;

  fmt2[0]='%';
  for(;(s=STRCHR(fmt,'%'));fmt=s)
  {
    MEMCPY(buf,fmt,s-fmt);
    buf+=s-fmt;
    fmt=s;
    fmt2p=fmt2+1;
    s++;
  unknown_character:
    switch((*(fmt2p++)=*(s++)))
    {
    default:
      goto unknown_character;

    case '*':
      fmt2p--;
      sprintf(fmt2p,"%d",va_arg(args,int));
      fmt2p+=strlen(fmt2p);
      goto unknown_character;

    case 0:
      Pike_fatal("Error in vsprintf format.\n");
      return 0;

    case '%':
      *(buf++)='%';
      break;

    case 'p':
    case 's':
      *fmt2p=0;
      sprintf(buf,fmt2,va_arg(args,char *));
      buf+=strlen(buf);
      break;

    case 'd':
    case 'c':
    case 'x':
    case 'X':
      *fmt2p=0;
      sprintf(buf,fmt2,va_arg(args,int));
      buf+=strlen(buf);
      break;

    case 'f':
    case 'e':
    case 'E':
    case 'g':
      *fmt2p=0;
      sprintf(buf,fmt2,va_arg(args,double));
      buf+=strlen(buf);
      break;
    }
  }
  tmpA=strlen(fmt);
  MEMCPY(buf,fmt,tmpA);
  buf+=tmpA;
  *buf=0;
  return buf-b;
}
#endif

#ifndef HAVE_VSNPRINTF
/* Warning: It's possible to trick this with something like
 * snprintf("...%c...", 0). */
PMOD_EXPORT int VSNPRINTF(char *buf, size_t size, const char *fmt, va_list args)
{
  int res;
  if (!size) {
    buf = alloca(size=1000);
  }
  buf[size - 1] = 0;
  res = VSPRINTF (buf, fmt, args);
  if (buf[size - 1]) Pike_fatal ("Buffer overflow in VSPRINTF.\n");
  return res;
}
#endif

#ifndef HAVE_SNPRINTF
/* Warning: It's possible to trick this with something like
 * snprintf("...%c...", 0). */
PMOD_EXPORT int SNPRINTF(char *buf, size_t size, const char *fmt, ...)
{
  int res;
  va_list args;
  va_start (args, fmt);
  res = VSNPRINTF (buf, size, fmt, args);
  va_end (args);
  return res;
}
#endif

#ifndef HAVE_VFPRINTF
PMOD_EXPORT int VFPRINTF(FILE *f,const char *s,va_list args)
{
  char buffer[10000];
  VSNPRINTF(buffer,sizeof(buffer),s,args);
  return fwrite(buffer,i,1,f);
}
#endif

#if defined(PIKE_DEBUG) && !defined(HANDLES_UNALIGNED_MEMORY_ACCESS)

PMOD_EXPORT unsigned INT16 EXTRACT_UWORD_(unsigned char *p)
{
  unsigned INT16 a;
  MEMCPY((char *)&a,p,sizeof(a));
  return a;
}

PMOD_EXPORT INT16 EXTRACT_WORD_(unsigned char *p)
{
  INT16 a;
  MEMCPY((char *)&a,p,sizeof(a));
  return a;
}

PMOD_EXPORT INT32 EXTRACT_INT_(unsigned char *p)
{
  INT32 a;
  MEMCPY((char *)&a,p,sizeof(a));
  return a;
}
#endif

#ifdef HAVE_BROKEN_CHKSTK
/* Intels ecl compiler adds calls to _stkchk() in the prologue,
 * MicroSoft seems to have renamed it to __stkchk() in later
 * versions of their SDK. This kludge attempts to workaround
 * that compiler bug.
 *	/grubba 2001-02-17.
 */
extern size_t __chkstk();
size_t _chkstk() { return __chkstk(); }
#endif /* HAVE_BROKEN_CHKSTK */

#ifdef OWN_GETHRTIME

#ifdef OWN_GETHRTIME_RDTSC

/* Estimated errors in max standard deviation in seconds between the
 * realtime and rtsc clocks. */

#define GETTIMEOFDAY_ERR 1e-3
/* Conservative value for gettimeofday resolution: It's taken from a
 * Linux with SMP kernel where it's reliable only down to msec. */

#define CPUINFO_ERR 1e-4
/* Only count on four accurate digits in the MHz figure in
 * /proc/cpuinfo. */

#define SATISFACTORY_ERR 1e-6
/* This is a compromise between final accurancy and the risk that the
 * system clock changes during calibration so that the zero time gets
 * wrong. (It's still better than the typical gethrtime accurancy on
 * Solaris.) */

#define COUNT_THRESHOLD 10
/* Number of measurements before we can trust the variance
 * calculation. */

static long long hrtime_rtsc_zero;
static long long hrtime_rtsc_last;
#ifdef PIKE_DEBUG
static long long hrtime_max = 0;
#endif
static struct timeval hrtime_timeval_zero;
static long long hrtime_rtsc_base, hrtime_nsec_base = 0;
static long double hrtime_conv=0.0;
static double hrtime_conv_var = GETTIMEOFDAY_ERR * GETTIMEOFDAY_ERR;
static int hrtime_is_calibrated = 0;

/*  #define RTSC_DEBUG 1 */
/*  #define RTSC_DEBUG_MORE 1 */

#define RTSC(l,h)							\
   __asm__ __volatile__ (  "rdtsc"					\
			   :"=a" (l),					\
			   "=d" (h))

static INLINE long long rtsc()
{
   long long now;
   unsigned long nl,nh;
   RTSC(nl,nh);
   return (((long long)nh)<<32)|nl;
}

void own_gethrtime_init()
{
   int fd;

   GETTIMEOFDAY(&hrtime_timeval_zero);
   hrtime_rtsc_zero=rtsc();
   hrtime_rtsc_last = hrtime_rtsc_base = hrtime_rtsc_zero;
#ifdef RTSC_DEBUG
   fprintf(stderr,"init: %lld\n",hrtime_rtsc_zero);
#endif   

   do {
     fd = fd_open("/proc/cpuinfo", fd_RDONLY, 0);
   } while (fd < 0 && errno == EINTR);
   if (fd >= 0) {
     char buf[10240];
     ptrdiff_t len;

     set_nonblocking(fd, 1);	/* Paranoia. */
     do {
       len = fd_read(fd, buf, sizeof(buf) - 1);
     } while (len < 0 && errno == EINTR);
     fd_close(fd);

     if (len > 0) {
       char *p;
       buf[len] = 0;
       p = STRSTR(buf, "\ncpu MHz");
       if (p) {
	 p += sizeof("\ncpu MHz");
	 p += STRCSPN(p, "0123456789\n");
	 if (*p != '\n') {
	   long long hz = 0;

	   for (; *p >= '0' && *p <= '9'; p++)
	     hz = hz * 10 + (*p - '0') * 1000000;
	   if (*p == '.') {
	     int mult = 100000;
	     for (p++; mult && *p >= '0' && *p <= '9'; p++, mult /= 10)
	       hz += (*p - '0') * mult;

	     hrtime_conv = 1e9 / (long double) hz;
	     hrtime_conv_var = CPUINFO_ERR * CPUINFO_ERR;
#ifdef RTSC_DEBUG
	     fprintf(stderr, "cpu Mhz read from /proc/cpuinfo: %lld.%lld, "
		     "conv=%#.10Lg, est err=%#.3g\n",
		     hz / 1000000, hz % 1000000, hrtime_conv, sqrt(hrtime_conv_var));
#endif

	     if (hrtime_conv_var < GETTIMEOFDAY_ERR * GETTIMEOFDAY_ERR) {
#ifdef RTSC_DEBUG
	       fprintf(stderr, "using rtsc from the start\n");
#endif
	       hrtime_is_calibrated = 1;
	     }
	   }
	 }
       }
     }
   }
}

void own_gethrtime_update(struct timeval *ptr)
{
   long long td,t;
   long long now;
   long double conv;
   double var;
   static long long td_last = 0;
   static long long w_sum = 0.0, w_c_sum = 0.0;
   static double w_c_c_sum = 0.0;
   static int count = -COUNT_THRESHOLD;

   GETTIMEOFDAY(ptr);
   now=rtsc();

#ifdef RTSC_DEBUG
   fprintf(stderr,"update: rtsc now: %lld\n",now);
#endif

   if (hrtime_is_calibrated == 2) return; /* Calibration done. */

   td=((long long)ptr->tv_sec-hrtime_timeval_zero.tv_sec)*1000000000+
      ((long long)ptr->tv_usec-hrtime_timeval_zero.tv_usec)*1000;

   if (!hrtime_is_calibrated) {
     /* We're in a hurry to make the rtsc kick in - require only
      * hundreds of seconds between measurements. */
     if (td - td_last < 10000000) return;
     /* Drop the count threshold if a second has gone by. That ought
      * to be long enough to get a reasonable reading. */
     if (td - td_last > 1000000000) count = 0;
   }
   else
     /* Doing fine calibration - require an order of whole seconds
      * between measurements. */
     if (now - hrtime_rtsc_last < 1000000000) return;

   t=now-hrtime_rtsc_zero;
   if (t <= 0 || td <= 0)
     /* Ouch, someone must have backed the clock(s). This situation
      * could be handled better. */
     return;
   td_last = td;
   hrtime_rtsc_last=now;
   conv=((long double)td)/t;

/* fixme: add time deviation detection;
   when time is adjusted, this calculation isn't linear anymore,
   so it might be best to reset it */

   /* Calculate the variance. Each conversion factor is weighted by
    * the time it was measured over, since later factors can be
    * considered more exact due to the elapsed time. */
   w_sum += t;
   w_c_sum += td;		/* = t * conv */
   w_c_c_sum += td * conv;	/* = t * conv * conv */

   if (++count >= 0) {
     double var = (w_c_c_sum - (double) w_c_sum * w_c_sum / w_sum) / w_sum;

     if (var > 0.0 && var < hrtime_conv_var) {
       hrtime_nsec_base +=
	 (long long) ( (long double)(now-hrtime_rtsc_base) * hrtime_conv );
       hrtime_rtsc_base = now;
       hrtime_conv_var = var;
       hrtime_conv = (long double) w_c_sum / w_sum;

#ifdef RTSC_DEBUG
       fprintf(stderr, "[%d, %#5.4g] "
	       "last=%#.10Lg, conv=%#.10Lg +/- %#.3g (%#.10Lg MHz)\n",
	       count, td / 1e9,
	       conv, hrtime_conv, sqrt(hrtime_conv_var), 1000.0/hrtime_conv);
#endif

       if (!hrtime_is_calibrated) {
	 if (hrtime_conv_var < GETTIMEOFDAY_ERR * GETTIMEOFDAY_ERR) {
#ifdef RTSC_DEBUG
	   fprintf(stderr, "switching to rtsc after %g sec\n", td / 1e9);
#endif
	   hrtime_is_calibrated = 1;
	   /* Reinitialize the statistics since it's hard to weight
	    * away the initial burst of noisy measurements we might
	    * have done to make rtsc good enough to use. */
	   count = -COUNT_THRESHOLD;
	   w_c_sum = w_sum = 0;
	   w_c_c_sum = 0.0;
	   /* The variance might be too low if we dropped the count
	    * threshold above. */
	   hrtime_conv_var = GETTIMEOFDAY_ERR * GETTIMEOFDAY_ERR;
	 }
       }
       else
	 if (hrtime_conv_var < SATISFACTORY_ERR * SATISFACTORY_ERR) {
#ifdef RTSC_DEBUG
	   fprintf(stderr, "rtsc calibration finished, took %g sec\n", td / 1e9);
#endif
	   hrtime_is_calibrated = 2;
	 }
     }
     else {
#ifdef RTSC_DEBUG
       fprintf(stderr, "[%d, %#5.4g] last=%#.10Lg +/- %#.3g, conv=%#.10Lg\n",
	       count, td / 1e9, conv, var > 0.0 ? sqrt(var) : -1.0, hrtime_conv);
#endif
     }
   }
   else {
#ifdef RTSC_DEBUG
     fprintf(stderr, "[%d, %#5.4g] last=%#.10Lg\n", count, td / 1e9, conv);
#endif
   }
}

long long gethrtime()
{
   long long now;
   struct timeval tv;

   if (!hrtime_is_calibrated) {
     own_gethrtime_update(&tv);
     if (hrtime_is_calibrated) goto use_rtsc;
     hrtime_rtsc_base=rtsc();
     now=hrtime_nsec_base =
       ((long long)tv.tv_sec-hrtime_timeval_zero.tv_sec)*1000000000+
       ((long long)tv.tv_usec-hrtime_timeval_zero.tv_usec)*1000;
   }
   else {
     now=rtsc();
     if ((hrtime_is_calibrated == 1) &
	 (now - hrtime_rtsc_last > 1000000000)) { /* Some seconds between updates. */
       own_gethrtime_update(&tv);
     use_rtsc:
       now=rtsc();
     }
#ifdef RTSC_DEBUG_MORE
     fprintf(stderr,"rtime: rtsc now: %lld\n",now);
#endif
     now = (long long) ( (long double)(now-hrtime_rtsc_base) * hrtime_conv ) +
       hrtime_nsec_base;
   }

#ifdef RTSC_DEBUG_MORE
   fprintf(stderr,"(%lld)\n",now);
#endif

#ifdef PIKE_DEBUG
   if (now<hrtime_max)
     Pike_fatal("Time calculation went backwards with %lld.\n", hrtime_max - now);
   hrtime_max = now;
#endif
   return now;
}

#endif	/* OWN_GETHRTIME_RDTSC */

#endif	/* OWN_GETHRTIME */

#ifndef HAVE_LDEXP
double LDEXP(double x, int exp)
{
  return x * pow(2.0,(double)exp);
}
#endif

#ifndef HAVE_FREXP
double FREXP(double x, int *exp)
{
  double ret;
  *exp = DO_NOT_WARN((int)ceil(log(fabs(x))/log(2.0)));
  ret = (x*pow(2.0,(double)-*exp));
  return ret;
}
#endif

#ifdef __MINGW32__
static unsigned int __loctotime_t_init = 0;
static FILETIME base_ft;
static time_t base_utc;

time_t __loctotime_t(WORD wYear, WORD wMonth, 
		     WORD wDay,WORD wHour, 
		     WORD wMinute, WORD wSecond, 
		     int dst) {
  time_t ret;
  FILETIME ft;
  SYSTEMTIME st;

  if (!__loctotime_t_init) {
    st.wYear = 1970;
    st.wMonth = 1;
    st.wDay = 1;
    st.wHour = 0;
    st.wMinute = 0;
    st.wSecond = 0;
    st.wMilliseconds = 0;
    
    SystemTimeToFileTime (&st, &base_ft);
    base_utc = (time_t) base_ft.dwHighDateTime
      * 4294967296 + base_ft.dwLowDateTime;
    
    __loctotime_t_init = 1;
  }
  st.wYear = wYear;
  st.wMonth = wMonth;
  st.wDay = wDay;
  st.wHour = wHour;
  st.wMinute = wMinute;
  st.wSecond = wSecond;
  SystemTimeToFileTime(&st, &ft);

  if (CompareFileTime(&ft, &base_ft) < 0)
    return 0;
  
  ret = (time_t) ft.dwHighDateTime * 4294967296 + ft.dwLowDateTime;
  ret -= base_utc;
  return (time_t) (ret / 10000000);
}

#define ATTR_READONLY  0x01
#define ATTR_DIRECTORY 0x10
#define ISSLASH(a)  ((a) == '\\' || (a) == '/')

unsigned short __dtoxmode(int attr, const char *name) {
  unsigned short unix_mode;
  unsigned dos_mode;
  const char *p;
  dos_mode = attr & 0xff;
  if ((p=name)[1]==(':'))
    p+=2;

  unix_mode = (unsigned short) (((ISSLASH(*p) && !p[1]) ||
				 (dos_mode & ATTR_DIRECTORY) ||
				 !*p)
    ? _S_IFDIR|_S_IEXEC:_S_IFREG);
  
  unix_mode |= (dos_mode & ATTR_READONLY)?_S_IREAD : (_S_IREAD|_S_IWRITE);

  if (p=strchr(name, '.')) {
    if ( !strcmp(p, ".exe") ||
	 !strcmp(p, ".cmd") ||
	 !strcmp(p, ".bat") ||
	 !strcmp(p, ".com"))
      unix_mode |= _S_IEXEC;
  }

  unix_mode |= (unix_mode & 0700) >> 3;
  unix_mode |= (unix_mode & 0700) >> 6;
  return unix_mode;
}

void _dosmaperr(int x) {
}
// #define _dosmaperr(X)

#endif
