/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
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
#define STEP(X,Y) ((X)+(Y))

#define ID fsort_1
typedef char b1;
#define TYPE b1
#include "fsort_template.h"
#undef ID
#undef TYPE

#define ID fsort_2
typedef struct a2 { char b[2]; } b2;
#define TYPE b2
#include "fsort_template.h"
#undef ID
#undef TYPE

#define ID fsort_4
typedef struct a4 { char b[4]; } b4;
#define TYPE b4
#include "fsort_template.h"
#undef ID
#undef TYPE


#define ID fsort_8
typedef struct a8 { char b[8]; } b8;
#define TYPE b8
#include "fsort_template.h"
#undef ID
#undef TYPE

#define ID fsort_16
typedef struct a16 { char b[16]; } b16;
#define TYPE b16
#include "fsort_template.h"
#undef ID
#undef TYPE

#undef SWAP
#undef STEP


#define SWAP(X,Y) do { \
    MEMCPY(tmp_area,X,size); \
    MEMCPY(X,Y,size); \
    MEMCPY(Y,tmp_area,size); \
 } while(0)

#define STEP(X,Y) ((X)+(Y)*size)
#define TYPE char
#define ID fsort_n
#include "fsort_template.h"
#undef ID
#undef TYPE
#undef SWAP
#undef STEP

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
  case  1:  fsort_1(( b1 *)base,(elms-1)+( b1 *)base); break;
  case  2:  fsort_2(( b2 *)base,(elms-1)+( b2 *)base); break;
  case  4:  fsort_4(( b4 *)base,(elms-1)+( b4 *)base); break;
  case  8:  fsort_8(( b8 *)base,(elms-1)+( b8 *)base); break;
  case 16: fsort_16((b16 *)base,(elms-1)+(b16 *)base); break;
  default:
    tmp_area=(char *)alloca(elmSize);
    fsort_n((char *)base,((char *)base) + size * (elms - 1));
  }

}


