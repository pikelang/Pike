/*\
||| This file is part of Pike. For copyright information see COPYRIGHT.
||| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
||| for more information.
\*/
/* fsort- a smarter quicksort /Hubbe */
/* Optimized for minimum amount of compares */

#include "global.h"
#include "pike_error.h"
#include "fsort.h"
#include "main.h"

RCSID("$Id: fsort.c,v 1.17 2002/05/31 22:41:24 nilsson Exp $");

#define CMP(X,Y) ( (*cmpfun)((void *)(X),(void *)(Y)) )
#define EXTRA_ARGS ,fsortfun cmpfun
#define XARGS ,cmpfun

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


#undef EXTRA_ARGS
#undef XARGS

#define EXTRA_ARGS ,fsortfun cmpfun,long size, char *tmp_area
#define XARGS ,cmpfun,size,tmp_area

#define SWAP(X,Y) do { \
    MEMCPY(tmp_area,X,size); \
    MEMCPY(X,Y,size); \
    MEMCPY(Y,tmp_area,size); \
 } while(0)

#define STEP(X,Y) ((X)+(Y)*size)
#define TYPE char
#define ID fsort_n
#define TMP_AREA
#include "fsort_template.h"
#undef ID
#undef TYPE
#undef EXTRA_ARGS
#undef XARGS

void fsort(void *base,
	   long elms,
	   long elmSize,
	   fsortfun cmpfunc)
{

  if(elms<=0) return;

#ifdef HANDLES_UNALIGNED_MEMORY_ACCESS
  switch(elmSize)
#else
  switch( (((size_t)base) % elmSize) ? 0 : elmSize )
#endif
  {
  case  1:  fsort_1(( B1_T *)base,(elms-1)+( B1_T *)base, cmpfunc); break;
#ifdef B2_T
  case  2:  fsort_2(( B2_T *)base,(elms-1)+( B2_T *)base, cmpfunc); break;
#endif
#ifdef B4_T
  case  4:  fsort_4(( B4_T *)base,(elms-1)+( B4_T *)base, cmpfunc); break;
#endif
#ifdef B8_T
  case  8:  fsort_8(( B8_T *)base,(elms-1)+( B8_T *)base, cmpfunc); break;
#endif
#ifdef B16_T
  case 16: fsort_16((B16_T *)base,(elms-1)+(B16_T *)base, cmpfunc); break;
#endif
  default:
    fsort_n((char *)base,((char *)base) + elmSize * (elms - 1), cmpfunc, elmSize, (char *)alloca(elmSize));
  }

}


