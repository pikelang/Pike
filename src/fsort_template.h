/*
 * $Id: fsort_template.h,v 1.5 1999/04/30 07:25:42 hubbe Exp $
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

#define PARENT(X) (((X)-1)>>1)
#define CHILD1(X) (((X)<<1)+1)

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
  register TYPE tmp;
  long howmany,x,y,z;
#ifdef PIKE_DEBUG
  extern int d_flag;
#endif

  howmany=((((char *)last)-((char *)bas))/SIZE)+1;
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
	fatal("Sorting failed!\n");
#endif
}

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
