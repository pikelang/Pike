#define INC(X) X=STEP(X,1)
#define DEC(X) X=STEP(X,-1)
#define SIZE ((long)STEP((TYPE *)0,1))

static void ID(register TYPE *bas, register TYPE *last)
{
  register TYPE *a,*b, tmp;
  if(bas >= last) return;
  a = STEP(bas,1);
  if(a == last)
  {
    if( (*cmpfun)((void *)bas,(void *)last) > 0) SWAP(bas,last); 
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
	  if(a<=b && (*cmpfun)((void *)a,(void *)bas) <= 0)
	    INC(a);
	  else
	  {
	    while(a< b && (*cmpfun)((void *)bas,(void *)b) <= 0) DEC(b);
	    break;
	  }
	  if(a< b && (*cmpfun)((void *)bas,(void *)b) <= 0)
	    DEC(b);
	  else
	  {
	    while(a<=b && (*cmpfun)((void *)a,(void *)bas) <= 0) INC(a);
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
      while(a<=b && (*cmpfun)((void *)a,(void *)bas) < 0) INC(a);
      while(a< b && (*cmpfun)((void *)bas,(void *)b) < 0) DEC(b);
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
    ID(bas,a);
    ID(b,last);
  }
}

#undef INC
#undef DEC
#undef SIZE
