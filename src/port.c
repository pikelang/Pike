/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
#include "pike_macros.h"
#include "time_stuff.h"
#include "error.h"

#include <ctype.h>
#include <math.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <errno.h>
#include <float.h>
#include <string.h>

RCSID("$Id: port.c,v 1.21 1999/08/31 23:31:13 hubbe Exp $");

#ifdef sun
time_t time PROT((time_t *));
#endif

#ifndef HAVE_GETTIMEOFDAY

#ifdef HAVE_GETSYSTEMTIMEASFILETIME
#include <winbase.h>

void GETTIMEOFDAY(struct timeval *t)
{
  double t2;
  FILETIME tmp;
  GetSystemTimeAsFileTime(&tmp);
  t2=tmp.dwHighDateTime * pow(2.0,32.0) + (double)tmp.dwLowDateTime;
  t2/=10000000.0;
  t2-=11644473600.0;
  t->tv_sec=floor(t2);
  t->tv_usec=(t2 - t->tv_sec)*1000000.0;
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

/*
 * This file defines things that may have to be changem when porting
 * LPmud to new environments. Hopefully, there are #ifdef's that will take
 * care of everything.
 */

static unsigned long RandSeed1 = 0x5c2582a4;
static unsigned long RandSeed2 = 0x64dff8ca;

static unsigned long slow_rand(void)
{
  RandSeed1 = ((RandSeed1 * 13 + 1) ^ (RandSeed1 >> 9)) + RandSeed2;
  RandSeed2 = (RandSeed2 * RandSeed1 + 13) ^ (RandSeed2 >> 13);
  return RandSeed1;
}

static void slow_srand(long seed)
{
  RandSeed1 = (seed - 1) ^ 0xA5B96384UL;
  RandSeed2 = (seed + 1) ^ 0x56F04021UL;
}

#define RNDBUF 250
#define RNDSTEP 7
#define RNDJUMP 103

static unsigned long rndbuf[ RNDBUF ];
static int rnd_index;

void my_srand(long seed)
{
  int e;
  unsigned long mask;

  slow_srand(seed);
  
  rnd_index = 0;
  for (e=0;e < RNDBUF; e++) rndbuf[e]=slow_rand();

  mask = (unsigned long) -1;

  for (e=0;e< (int)sizeof(long)*8 ;e++)
  {
    int d = RNDSTEP * e + 3;
    rndbuf[d % RNDBUF] &= mask;
    mask>>=1;
    rndbuf[d % RNDBUF] |= (mask+1);
  }
}

unsigned long my_rand(void)
{
  if( ++rnd_index == RNDBUF) rnd_index=0;
  return rndbuf[rnd_index] += rndbuf[rnd_index+RNDJUMP-(rnd_index<RNDBUF-RNDJUMP?0:RNDBUF)];
}

#if !defined(HAVE_STRTOL) || !defined(HAVE_WORKING_STRTOL)
#define DIGIT(x)	(isdigit(x) ? (x) - '0' : \
			islower(x) ? (x) + 10 - 'a' : (x) + 10 - 'A')
#define MBASE	('z' - 'a' + 1 + 10)

long STRTOL(char *str,char **ptr,int base)
{
  register long val;
  register int c;
  int xx, neg = 0;

  if (ptr != (char **)0)
    *ptr = str;			/* in case no number is formed */
  if (base < 0 || base > MBASE)
    return (0);			/* base is invalid -- should be a fatal error */
  if (!isalnum(c = *str)) {
    while (ISSPACE(c))
      c = *++str;
    switch (c) {
    case '-':
      neg++;
    case '+':			/* fall-through */
      c = *++str;
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
  if (base == 16 && c == '0' && isxdigit(((unsigned char *)str)[2]) &&
      (str[1] == 'x' || str[1] == 'X'))
    c = *(str += 2);		/* skip over leading "0x" or "0X" */
  for (val = -DIGIT(c); isalnum(c = *++str) && (xx = DIGIT(c)) < base; )
    /* accumulate neg avoids surprises near MAXLONG */
    val = base * val - xx;
  if (ptr != (char **)0)
    *ptr = str;
  return (neg ? val : -val);
}
#endif

#ifndef HAVE_STRCASECMP
int STRCASECMP(const char *a,const char *b)
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

#ifndef HAVE_MEMSET
char *MEMSET(char *s,int c,int n)
{
  char *t;
  for(t=s;n;n--) *(s++)=c;
  return s;
}
#endif

#if defined(TRY_USE_MMX) || !defined(HAVE_MEMCPY) && !defined(HAVE_BCOPY)
#ifdef TRY_USE_MMX
#include <mmx.h>
#endif
void MEMCPY(void *bb,const void *aa,int s)
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
      char *b=(char *)bb;
      char *a=(char *)aa;
      for(;s;s--) *(b++)=*(a++);
#endif
#ifdef TRY_USE_MMX
    }
  }
#endif
}
#endif

#ifndef HAVE_MEMMOVE
void MEMMOVE(void *b,const void *aa,int s)
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
int MEMCMP(const void *bb,const void *aa,int s)
{
  char *a=(char *)aa;
  char *b=(char *)bb;
  for(;s;s--,b++,a++)
  {
    if(*b!=*a)
    {
      if(*b<*a) return -1;
      if(*b>*a) return 1;
    }
  }
  return 0;
}

#endif

#ifndef HAVE_MEMCHR
char *MEMCHR(char *p,char c,int e)
{
  while(--e >= 0) if(*(p++)==c) return p-1;
  return (char *)0;
}
#endif


#if !defined(HAVE_INDEX) && !defined(HAVE_STRCHR)
char *STRCHR(char *s,char c)
{
  for(;*s;s++) if(*s==c) return s;
  return NULL;
}
#endif

#if !defined(HAVE_RINDEX) && !defined(HAVE_STRRCHR)
char *STRRCHR(char *s,int c)
{
  char *p;
  for(p=NULL;*s;s++) if(*s==c) p=s;
  return p;
}
#endif

#ifndef HAVE_STRSTR
char *STRSTR(char *s1,const char *s2)
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
char *STRTOK(char *s1,char *s2)
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

#ifndef HAVE_STRTOD
/* Convert NPTR to a double.  If ENDPTR is not NULL, a pointer to the
   character after the last one used in the number is put in *ENDPTR.  */
double STRTOD(char * nptr, char **endptr)
{
  register char *s;
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

  s = nptr;

  /* Eat whitespace.  */
  while (ISSPACE(*s)) ++s;

  /* Get the sign.  */
  sign = *s == '-' ? -1 : 1;
  if (*s == '-' || *s == '+')
    ++s;

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
    if (isdigit (*s))
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
    else if (!got_dot && (char) *s == '.')
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
      exp = STRTOL(s, &end, 10);
      if (errno == ERANGE)
      {
	/* The exponent overflowed a `long int'.  It is probably a safe
	   assumption that an exponent that cannot be represented by
	   a `long int' exceeds the limits of a `double'.  */
	if (endptr != NULL)
	  *endptr = end;
	if (exp < 0)
	  goto underflow;
	else
	  goto overflow;
      }
      else if (end == s)
	/* There was no exponent.  Reset END to point to
	   the 'e' or 'E', so *ENDPTR will be set there.  */
	end = (char *) s - 1;
      errno = save;
      s = end;
      exponent += exp;
    }

  if (endptr != NULL)
    *endptr = (char *) s;

  if (num == 0.0)
    return 0.0;

  /* Multiply NUM by 10 to the EXPONENT power,
     checking for overflow and underflow.  */

  if (exponent < 0)
  {
    if (num < DBL_MIN * pow(10.0, (double) -exponent))
      goto underflow;
  }
  else if (exponent > 0)
  {
    if (num > DBL_MAX * pow(10.0, (double) -exponent))
      goto overflow;
  }

  num *= pow(10.0, (double) exponent);

  return num * sign;

 overflow:
  /* Return an overflow error.  */
  errno = ERANGE;
  return HUGE * sign;

 underflow:
  /* Return an underflow error.  */
  if (endptr != NULL)
    *endptr = (char *) nptr;
  errno = ERANGE;
  return 0.0;
  
 noconv:
  /* There was no number.  */
  if (endptr != NULL)
    *endptr = (char *) nptr;
  return 0.0;
}
#endif

#ifndef HAVE_VSPRINTF
int VSPRINTF(char *buf,char *fmt,va_list args)
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
      fatal("Error in vsprintf format.\n");
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
      sprintf(buf,fmt2,va_arg(args,FLOAT_TYPE));
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

#ifndef HAVE_VFPRINTF
int VFPRINTF(FILE *f,char *s,va_list args)
{
  int i;
  char buffer[10000];
  VSPRINTF(buffer,s,args);
  i=strlen(buffer);
  if(i+1>sizeof(buffer))
    fatal("Buffer overflow.\n");
  return fwrite(buffer,i,1,f);
}
#endif

#if defined(PIKE_DEBUG) && !defined(HANDLES_UNALIGNED_MEMORY_ACCESS)

unsigned INT16 EXTRACT_UWORD_(unsigned char *p)
{
  unsigned INT16 a;
  MEMCPY((char *)&a,p,sizeof(a));
  return a;
}

INT16 EXTRACT_WORD_(unsigned char *p)
{
  INT16 a;
  MEMCPY((char *)&a,p,sizeof(a));
  return a;
}

INT32 EXTRACT_INT_(unsigned char *p)
{
  INT32 a;
  MEMCPY((char *)&a,p,sizeof(a));
  return a;
}
#endif
