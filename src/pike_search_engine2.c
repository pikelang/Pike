/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/*
 * NCHAR = Needle character
 * HCHAR = Haystack character
 */

static INLINE HCHAR *NameNH(MEMCHR)(HCHAR *p, NCHAR c, ptrdiff_t e)
{
#if NSHIFT > HSHIFT
  if(c > (1<<(8*HSHIFT))) return 0;
#endif
  return NameH(MEMCHR)(p,c,e);
}


static INLINE int NameNH(MEMCMP)(NCHAR *a, HCHAR *b, ptrdiff_t e)
{
#if NSHIFT == HSHIFT
  return MEMCMP(a, b, sizeof(HCHAR)*e);
#else
  for(;e;e--,b++,a++)
  {
    if(*b!=*a)
    {
      if(*b<*a) return -1;
      if(*b>*a) return 1;
    }
  }
  return 0;
  
#endif
}

INLINE HCHAR *NameNH(memchr_search)(void *data,
				    HCHAR *haystack,
				    ptrdiff_t haystacklen)
{
  return NameNH(MEMCHR)(haystack, DO_NOT_WARN((NCHAR)(ptrdiff_t)data), haystacklen);
}


INLINE HCHAR *NameNH(memchr_memcmp)(NCHAR *needle,
				    ptrdiff_t needlelen,
				    HCHAR *haystack,
				    ptrdiff_t haystacklen)
{
  NCHAR c;
  HCHAR *end;

  if(needlelen > haystacklen) return 0;

  end=haystack + haystacklen - needlelen+1;
  c=needle[0];
  needle++;
  needlelen--;
  while((haystack=NameNH(MEMCHR)(haystack,c,end-haystack)))
    if(!NameNH(MEMCMP)(needle,++haystack,needlelen))
      return haystack-1;

  return 0;
}


#define make_memchr_memcmpX(X)					\
HCHAR *PxC4(memchr_memcmp,X,NSHIFT,HSHIFT)(void *n,	       \
					   HCHAR *haystack,	\
					   ptrdiff_t haystacklen)	\
{								\
  return NameNH(memchr_memcmp)((NCHAR *)n,			\
			       X,				\
			       haystack,			\
			       haystacklen);			\
}

make_memchr_memcmpX(2)
make_memchr_memcmpX(3)
make_memchr_memcmpX(4)
make_memchr_memcmpX(5)
make_memchr_memcmpX(6)

#undef make_memchr_memcmpX

HCHAR *NameNH(boyer_moore_hubbe)(struct boyer_moore_hubbe_searcher *s,
				 HCHAR *haystack,
				 ptrdiff_t haystacklen)
{
  ptrdiff_t i=s->plen-1;
  ptrdiff_t hlen=haystacklen;
  if(NEEDLELEN > s->plen)
    hlen -= NEEDLELEN-s->plen;
  
 restart:
  while(i<hlen)
  {
    ptrdiff_t k,j;
    
    if((k=s->d1[ NameNH(BMHASH)( haystack[i] ) ]))
      i+=k;
    else
    {
#if NSHIFT == 0
      j=s->plen-1;
      
#ifdef PIKE_DEBUG
      if(NEEDLE[j] != haystack[i])
	Pike_fatal("T2BM failed!\n");
#endif

#else
      i++;
      j=s->plen;
#endif
      
      while(NEEDLE[--j] == haystack[--i])
      {
	if(!j)
	{
	  if(NEEDLELEN > s->plen)
	  {
	    if(!NameNH(MEMCMP)(NEEDLE+s->plen,
			       haystack+i+s->plen, NEEDLELEN-s->plen))
	    {
	      return haystack+i;
	    }else{
	      /* this can be optimized... */
	      i+=s->plen;
	      goto restart;
	    }
	  }else{
	    return haystack+i;
	  }
	}
      }
      
      i+=
	(s->d1[ NameNH(BMHASH)(haystack[i]) ] >= s->d2[j]) ?
	(s->d1[ NameNH(BMHASH)(haystack[i]) ]):
	(s->d2[j]);
    }
  }
  return 0;
}


HCHAR *NameNH(hubbe_search)(struct hubbe_searcher *s,
			    HCHAR *haystack,
			    ptrdiff_t haystacklen)
{
  INT32 tmp, h;
  HCHAR *q, *end;
  register struct hubbe_search_link *ptr;
  
  end=haystack+haystacklen;
  q=haystack + s->max - 4;

  NameH(HUBBE_ALIGN)(q);

  for(;q<=end-sizeof(INT32);q+=s->max)
  {
    h=tmp=NameH(GET_4_ALIGNED_CHARS)(q);
    
    h+=h>>7;
    h+=h>>17;
    h&=s->hsize;
    
    for(ptr=s->set[h];ptr;ptr=ptr->next)
    {
      HCHAR *where;
      
      if(ptr->key != tmp) continue;
      
      where=q-ptr->offset;

      /* FIXME: This if statement is not required
       * HUBBE_ALIGN is a noop
       */
      if(where<haystack) continue;

      if(where+NEEDLELEN>end) return 0;
      
      if(!NameNH(MEMCMP)(NEEDLE,where,NEEDLELEN))
	return where;
    }
  }
  return 0;
}
