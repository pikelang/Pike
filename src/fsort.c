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
#define STEP(X,Y) (&((X)[(Y)]))

#define ID fsort_1
#define TYPE B1_T
#include "fsort_template.h"
#undef ID
#undef TYPE

#ifdef B2_T
#define ID fsort_2
#define TYPE B2_T
#include "fsort_template.h"
#undef ID
#undef TYPE
#endif

#ifdef B4_T
#define ID fsort_4
#define TYPE B4_T
#include "fsort_template.h"
#undef ID
#undef TYPE
#endif


#ifdef B8_T
#define ID fsort_8
#define TYPE B8_T
#include "fsort_template.h"
#undef ID
#undef TYPE
#endif


#ifdef B16_T
#define ID fsort_16
#define TYPE B16_T
#include "fsort_template.h"
#undef ID
#undef TYPE
#endif

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
  case  1:  fsort_1(( B1_T *)base,(elms-1)+( B1_T *)base); break;
#ifdef B2_T
  case  2:  fsort_2(( B2_T *)base,(elms-1)+( B2_T *)base); break;
#endif
#ifdef B4_T
  case  4:  fsort_4(( B4_T *)base,(elms-1)+( B4_T *)base); break;
#endif
#ifdef B8_T
  case  8:  fsort_8(( B8_T *)base,(elms-1)+( B8_T *)base); break;
#endif
#ifdef B16_T
  case 16: fsort_16((B16_T *)base,(elms-1)+(B16_T *)base); break;
#endif
  default:
    tmp_area=(char *)alloca(elmSize);
    fsort_n((char *)base,((char *)base) + size * (elms - 1));
  }

}


