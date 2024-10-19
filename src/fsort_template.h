/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "pike_cpulib.h"

#ifdef SORT_BY_INDEX
/* Sort by index. */

#ifndef SWAP
#error SWAP required when SORT_BY_INDEX is defined.
#endif

#define PTYPE ptrdiff_t

#else  /* !SORT_BY_INDEX */
/* Sort by pointer. */

#ifndef SWAP
#define UNDEF_SWAP
#define SWAP(X,Y) { TYPE tmp=*(X); *(X)=*(Y); *(Y)=tmp; }
#endif

#define PTYPE TYPE *

#endif	/* !SORT_BY_INDEX */

#ifndef STEP
#define UNDEF_STEP
#define STEP(X,Y) ((X) + (Y))
#endif

#ifndef DIFF
#define DIFF(last, bas) ((last) - (bas))
#define UNDEF_DIFF
#endif

#define INC(X) X=STEP(X,1)
#define DEC(X) X=STEP(X,-1)

#define PARENT(X) (((X)-1)>>1)
#define CHILD1(X) (((X)<<1)+1)

#define MKNAME(X) MKNAME2(ID,X)
#define MKNAME2(X,Y) PIKE_CONCAT(X,Y)

static void MKNAME(_do_sort)(PTYPE bas,
                             PTYPE last,
                             int max_recursion
#ifdef EXTRA_ARGS
                             EXTRA_ARGS
#else
#define UNDEF_XARGS
#define XARGS
#endif
  )
{
  PTYPE a;
  PTYPE b;
#ifdef EXTRA_LOCALS
  EXTRA_LOCALS
#endif

  while(bas < last)
  {
    a = STEP(bas,1);
    if(a == last)
    {
      if( CMP(bas,last) > 0) SWAP(bas,last);
      return;
    }else{
      if(--max_recursion <= 0)
      {
	ptrdiff_t howmany,x,y,z;
	howmany=DIFF(last, bas)+1;
	if(howmany<2) return;

	for(x=PARENT(howmany-1);x>=0;x--)
	{
	  y=x;
	  while(1)
	  {
	    z=CHILD1(y);
	    if(z>=howmany) break;
	    if(z+1 < howmany && CMP(STEP(bas,z),STEP(bas,z+1))<=0) z++;
	    if(CMP( STEP(bas, y), STEP(bas, z)) >0) break;
	    SWAP( STEP(bas, y), STEP(bas, z));
	    y=z;
	  }
	}

	for(x=howmany-1;x;x--)
	{
	  SWAP( STEP(bas,x), bas);
	  y=0;
	  while(1)
	  {
	    z=CHILD1(y);
	    if(z>=x) break;
	    if(z+1 < x && CMP(STEP(bas,z),STEP(bas,z+1))<=0) z++;
	    if(CMP( STEP(bas, y), STEP(bas, z)) >0) break;
	    SWAP( STEP(bas, y), STEP(bas, z));
	    y=z;
	  }
	}

#ifdef PIKE_DEBUG
	if(d_flag>1)
	  for(x=howmany-1;x;x--)
	    if( CMP( STEP(bas,x-1), STEP(bas,x)  ) > 0)
	      Pike_fatal("Sorting failed!\n");
#endif

	return;
      }

      b=STEP(bas, DIFF(last, bas) >> 1);
      SWAP(bas,b);
      b=last;
      while(a < b)
      {
#if 1
	while(1)
	{
	  if(a<=b && CMP(a,bas) <= 0)
	    INC(a);
	  else
	  {
	    while(a< b && CMP(bas,b) <= 0) DEC(b);
	    break;
	  }
	  if(a< b && CMP(bas,b) <= 0)
	    DEC(b);
	  else
	  {
	    while(a<=b && CMP(a,bas) <= 0) INC(a);
	    break;
	  }
	}
#else
	while(a<=b && CMP(a,bas) < 0) INC(a);
	while(a< b && CMP(bas,b) < 0) DEC(b);
#endif
	if(a<b)
	{
	  SWAP(a,b);
	  INC(a);
	  if(b > STEP(a,1)) DEC(b);
	}
      }

      DEC(a);
      if( a != bas ) {
	SWAP(a,bas);
	DEC(a);

	if(  (char *)a - (char *)bas < (char *)last - (char *)b )
        {
	  MKNAME(_do_sort)(bas,a,max_recursion XARGS);
	  bas=b;
	} else {
	  MKNAME(_do_sort)(b,last,max_recursion XARGS);
	  last=a;
	}
      }
      else
      {
	DEC(a);
	if( STEP(0,-1) < (char *)last - (char *)b )
	{
	  MKNAME(_do_sort)(bas,a,max_recursion XARGS);
	  bas=b;
	} else {
	  MKNAME(_do_sort)(b,last,max_recursion XARGS);
	  last=a;
	}
      }
    }
  }
}

void ID(PTYPE bas,
	PTYPE last
#ifdef EXTRA_ARGS
	EXTRA_ARGS
#endif
  )
{
  MKNAME(_do_sort)(bas,last, my_log2( last-bas ) * 2 XARGS);;
}


#undef PTYPE
#undef INC
#undef DEC
#undef PARENT
#undef CHILD1

#ifdef UNDEF_XARGS
#undef XARGS
#undef UNDEF_XARGS
#endif

#ifdef UNDEF_SWAP
#undef SWAP
#undef UNDEF_SWAP
#endif

#ifdef UNDEF_STEP
#undef STEP
#undef UNDEF_STEP
#endif

#ifdef UNDEF_DIFF
#undef DIFF
#undef UNDEF_DIFF
#endif
