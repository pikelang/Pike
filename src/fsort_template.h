/*
 * $Id: fsort_template.h,v 1.4 1998/04/27 22:33:17 hubbe Exp $
 */

#ifndef SWAP
#define UNDEF_SWAP
#define SWAP(X,Y) { tmp=*(X); *(X)=*(Y); *(Y)=tmp; }
#endif

#ifndef STEP
#define UNDEF_STEP
#define STEP(X,Y) (&((X)[(Y)]))
#endif

#define INC(X) X=STEP(X,1)
#define DEC(X) X=STEP(X,-1)
#define SIZE ((long)(char *)STEP((TYPE *)0,1))

static void ID(register TYPE *bas,
	       register TYPE *last
#ifdef EXTRA_ARGS
	       EXTRA_ARGS
#else
#define UNDEF_XARGS
#define XARGS
#endif
  )
{
  register TYPE *a,*b, tmp;
  if(bas >= last) return;
  a = STEP(bas,1);
  if(a == last)
  {
    if( CMP(bas,last) > 0) SWAP(bas,last); 
  }else{
    b=STEP(bas,((((char *)last)-((char *)bas))/SIZE)>>1);
    SWAP(bas,b);
    b=last;
    while(a < b)
    {
#if 1
      while(a<b)
      {
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

	if(a<b)
	{
	  SWAP(a,b);
	  INC(a);
	  if(b > STEP(a,1)) DEC(b);
	}
      }
#else
      while(a<=b && CMP(a,bas) < 0) INC(a);
      while(a< b && CMP(bas,b) < 0) DEC(b);
      if(a<b)
      {
        SWAP(a,b);
	INC(a);
	if(b > STEP(a,1)) DEC(b);
      }
#endif
    }
    DEC(a);
    SWAP(a,bas);
    DEC(a);
    ID(bas,a XARGS);
    ID(b,last XARGS);
  }
}

#undef INC
#undef DEC

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
