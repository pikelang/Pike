#include "global.h"
#include "macros.h"
#include <ctype.h>
#include <math.h>
#include <sys/types.h>
#include <errno.h>
#include <float.h>
#include <string.h>

#ifdef sun
time_t time PROT((time_t *));
#endif

/*
 * This file defines things that may have to be changem when porting
 * LPmud to new environments. Hopefully, there are #ifdef's that will take
 * care of everything.
 */

static unsigned long RandSeed1 = 0x5c2582a4;
static unsigned long RandSeed2 = 0x64dff8ca;

unsigned long my_rand(void)
{
  RandSeed1 = ((RandSeed1 * 13 + 1) ^ (RandSeed1 >> 9)) + RandSeed2;
  RandSeed2 = (RandSeed2 * RandSeed1 + 13) ^ (RandSeed2 >> 13);
  return RandSeed1;
}

void my_srand(int seed)
{
  RandSeed1 = (seed - 1) ^ 0xA5B96384;
  RandSeed2 = (seed + 1) ^ 0x56F04021;
  my_rand();
  my_rand();
  my_rand();
}

long get_current_time(void) { return (long)time(0L); }



#ifndef HAVE_STRTOL
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
    while (isspace(c))
      c = *++str;
    switch (c) {
    case '-':
      neg++;
    case '+':			/* fall-through */
      c = *++str;
    }
  }
  if (base == 0)
    if (c != '0')
      base = 10;
    else if (str[1] == 'x' || str[1] == 'X')
      base = 16;
    else
      base = 8;
  /*
   * for any base > 10, the digits incrementally following
   *	9 are assumed to be "abc...z" or "ABC...Z"
   */
  if (!isalnum(c) || (xx = DIGIT(c)) >= base)
    return (0);			/* no number formed */
  if (base == 16 && c == '0' && isxdigit(str[2]) &&
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

#ifndef HAVE_MEMSET
char *MEMSET(char *s,int c,int n)
{
  char *t;
  for(t=s;n;n--) *(s++)=c;
  return s;
}
#endif

#if !defined(HAVE_MEMCPY) && !defined(HAVE_BCOPY)
char *MEMCPY(char *b,const char *a,int s)
{
  char *t;
  for(t=b;s;s--) *(t++)=*(a++);
  return b;
}
#endif

#ifndef HAVE_MEMMOVE
char *MEMMOVE(char *b,const char *a,int s)
{
  char *t;
  if(a>b) for(t=b;s;s--) *(t++)=*(a++);
  if(a<b) for(t=b+s,a+=s;s;s--) *(--t)=*(--a);
  return b;
}
#endif


#ifndef HAVE_MEMCMP
int MEMCMP(const char *b,const char *a,int s)
{
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
  e++;
  while(--e >= 0) if(*(p++)==c) return p-1;
  return (char *)0;
}
#endif

#ifndef HAVE_MEMMEM

/*
 * a quick memmem
 * Written by Fredrik Hubinette (hubbe@lysator.liu.se)
 */

#define LINKS 1024

typedef struct link2_s
{
  struct link2_s *next;
  INT32 key, offset;
} link2;

char *MEMMEM(char *needle, SIZE_T needlelen, char *haystack, SIZE_T haystacklen)
{
  if(needlelen > haystacklen) return 0;

  switch(needlelen)
  {
  case 0: return haystack;
  case 1: return MEMCHR(haystack,needle[0],haystacklen);

  default:
  {
    if(haystacklen > needlelen + 64)
    {
      static link2 links[LINKS], *set[LINKS];

      INT32 tmp, h;
      unsigned INT32 hsize, e, max;
      char *q, *end;
      link2 *ptr;

      hsize=52+(haystacklen >> 7)  - (needlelen >> 8);
      max  =13+(haystacklen >> 4)  - (needlelen >> 5);

      if(hsize > NELEM(set))
      {
	hsize=NELEM(set);
      }else{
	for(e=8;e<hsize;e+=e);
	hsize=e;
      }
    
      for(e=0;e<hsize;e++) set[e]=0;
      hsize--;

      if(max > needlelen) max=needlelen;
      max=(max-sizeof(INT32)+1) & -sizeof(INT32);
      if(max > LINKS) max=LINKS;

      ptr=&links[0];

      q=needle;

#if BYTEORDER == 4321
      for(tmp=e=0;e<sizeof(INT32)-1;e++)
      {
	tmp<<=8;
	tmp|=*(q++);
      }
#endif

      for(e=0;e<max;e++)
      {
#if BYTEORDER == 4321
	tmp<<=8;
	tmp|=*(q++);
#else
	tmp=EXTRACT_INT((unsigned char *)q);
	q++;
#endif
	h=tmp;
	h+=h>>7;
	h+=h>>17;
	h&=hsize;

	ptr->offset=e;
	ptr->key=tmp;
	ptr->next=set[h];
	set[h]=ptr;
	ptr++;
      }

      end=haystack+haystacklen+1;
    
      q=haystack+max-sizeof(INT32);
      q=(char *)( ((INT32)q) & -sizeof(INT32));
      for(;q<end-sizeof(INT32)+1;q+=max)
      {
	h=tmp=*(INT32 *)q;

	h+=h>>7;
	h+=h>>17;
	h&=hsize;
      
	for(ptr=set[h];ptr;ptr=ptr->next)
	{
	  char *where;

	  if(ptr->key != tmp) continue;

	  where=q-ptr->offset;
	  if(where<haystack) continue;
	  if(where+needlelen>end) return 0;

	  if(!MEMCMP(where,needle,needlelen))
	    return where;
	}
      }
      return 0;
    }
  }

  case 2:
  case 3:
  case 4:
  case 5:
  case 6: 
  {
    char *end,c;
  
    end=haystack+haystacklen-needlelen+1;
    c=needle[0];
    needle++;
    needlelen--;
    while((haystack=MEMCHR(haystack,c,end-haystack)))
      if(!MEMCMP(++haystack,needle,needlelen))
	return haystack-1;

    return 0;
  }
    
  }
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
    for(p1=s1,p2=s2;*p1==*p2;p1++,p2++) if(!*p2) return s1;
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
  while (isspace(*s)) ++s;

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

#if defined(DEBUG) && !defined(HANDLES_UNALIGNED_MEMORY_ACCESS)
unsigned INT16 EXTRACT_UWORD(unsigned char *p)
{
  return (EXTRACT_UCHAR(p)<<8) + EXTRACT_UCHAR(p+1);
}

INT16 EXTRACT_WORD(unsigned char *p)
{
  return (EXTRACT_CHAR(p)<<8) | EXTRACT_UCHAR(p+1);
}

INT32 EXTRACT_INT(unsigned char *p)
{
  return (EXTRACT_WORD(p)<<16) | EXTRACT_UWORD(p+2);
}
#endif
