/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: fsort.c,v 1.23 2004/09/18 20:50:50 nilsson Exp $
*/

/* fsort- a smarter quicksort /Hubbe */
/* Optimized for minimum amount of compares */

#include "global.h"
#include "pike_error.h"
#include "fsort.h"
#include "main.h"
#include "pike_macros.h"

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

#define EXTRA_ARGS , fsortfun cmpfun, char *tmp_area, long size
#define XARGS , cmpfun, tmp_area, size

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
    {
      /* NOTE: We need to put the alloca'd value in a variable,
       *       otherwise cc/HPUX will generate broken code.
       *       Hmm, that didn't work, but reordering the arguments,
       *       putting size last seems to have fixed the problem...
       *       /grubba hunting compiler bugs 2002-09-03
       */
      char *buf = alloca(elmSize);

      fsort_n((char *)base,((char *)base) + elmSize * (elms - 1),
	      cmpfunc, buf, elmSize);
    }
  }

}
