/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
/* fsort- a smarter quicksort /Hubbe */
/* Optimized for minimum amount of compares */

#include "global.h"
#include "fsort.h"

static fsortfun cmpfun;
static long size;
static char *tmp_area;

#define SWAP(X,Y) { tmp=*(X); *(X)=*(Y); *(Y)=tmp; }

#define FSORT(ID,TYPE) \
static void ID(register TYPE *bas,register TYPE *last) \
{ \
  register TYPE *a,*b, tmp; \
  if(bas >= last) return; \
  a = bas + 1; \
  if(a == last) \
  { \
    if( (*cmpfun)((void *)bas,(void *)last) > 0) SWAP(bas,last);  \
  }else{ \
    b=bas+((last-bas)>>1); \
    SWAP(bas,b); \
    b=last; \
    while(a < b) \
    { \
      while(a<=b && (*cmpfun)((void *)a,(void *)bas) < 0) a++; \
      while(a< b && (*cmpfun)((void *)bas,(void *)b) < 0) b--; \
      if(a<b) \
      { \
        SWAP(a,b); \
        a++; \
        if(b > a+1) b--; \
      } \
    } \
    a--; \
    SWAP(a,bas); \
    a--; \
    ID(bas,a); \
    ID(b,last); \
  } \
}

#define FSWAP(X,Y) \
 { \
    MEMCPY(tmp_area,X,size); \
    MEMCPY(X,Y,size); \
    MEMCPY(Y,tmp_area,size); \
 }

static void fsort_n(register char *bas,register char *last)
{
  register char *a,*b;
  register int dum;

  if(bas>=last) return;
  a=bas+size;
  if(a==last)
  {
    if((*cmpfun)(bas,last) > 0) FSWAP(bas,last);
  }else{
    dum=(last-bas)>>1;
    dum-=dum % size;
    FSWAP(bas,bas+dum);
    b=last;
    while(a<b)
    {
      while(a<=b && (*cmpfun)(a,bas) < 0) a+=size;
      while(a< b && (*cmpfun)(bas,b) < 0) b-=size;
      if(a<b)
      {
	FSWAP(a,b);
	a+=size;
	if(b-a>size) b-=size;
      }
    }
    a-=size;
    FSWAP(bas,a);
    a-=size;
    fsort_n(bas,a);
    fsort_n(b,last);
  }
}

typedef struct a { char b[1]; } onebyte;
typedef struct b { char b[2]; } twobytes;
typedef struct c { char b[4]; } fourbytes;
typedef struct d { char b[8]; } eightbytes;

FSORT(fsort_1, onebyte)
FSORT(fsort_2, twobytes)
FSORT(fsort_4, fourbytes)
FSORT(fsort_8, eightbytes)

void fsort(void *base,
	   long elms,
	   long elmSize,
	   fsortfun cmpfunc)
{

  if(elms<=0) return;
  cmpfun=cmpfunc;
  size=elmSize;

  switch(elmSize)
  {
  case 1: fsort_1((onebyte *)base,(elms-1)+(onebyte *)base); break;
  case 2: fsort_2((twobytes *)base,(elms-1)+(twobytes *)base); break;
  case 4: fsort_4((fourbytes *)base,(elms-1)+(fourbytes *)base); break;
  case 8: fsort_8((eightbytes *)base,(elms-1)+(eightbytes *)base); break;
  default:
    tmp_area=(char *)alloca(elmSize);
    fsort_n((char *)base,((char *)base) + size * (elms - 1));
  }

}


