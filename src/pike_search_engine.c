/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: pike_search_engine.c,v 1.12 2004/09/18 20:19:41 per Exp $
*/

/*
 * Written by Fredrik Hubinette (hubbe@lysator.liu.se)
 */

#define HSHIFT 0
#include "pike_search_engine2.c"
#undef HSHIFT

#define HSHIFT 1
#include "pike_search_engine2.c"
#undef HSHIFT

#define HSHIFT 2
#include "pike_search_engine2.c"
#undef HSHIFT



#define INTERCASE(NAME,X) \
 case X: return MKPCHARP(PxC3(NAME,NSHIFT,X)(s,(PxC(p_wchar,X) *)haystack.ptr,haystacklen),X)


#define INTERMEDIATE(NAME)			\
PCHARP PxC3(NAME,NSHIFT,N)(void *s,	\
		   PCHARP haystack,		\
		   ptrdiff_t haystacklen)		\
{						\
  switch(haystack.shift)			\
  {						\
    INTERCASE(NAME,0);				\
    INTERCASE(NAME,1);				\
    INTERCASE(NAME,2);				\
  }                                             \
  Pike_fatal("Illegal shift\n");                     \
  return haystack;	/* NOT_REACHED */	\
}						\
						\
static const struct SearchMojtVtable PxC3(NAME,NSHIFT,_vtable) = {	\
  (SearchMojtFunc0)PxC3(NAME,NSHIFT,0),		\
  (SearchMojtFunc1)PxC3(NAME,NSHIFT,1),			\
  (SearchMojtFunc2)PxC3(NAME,NSHIFT,2),			\
  (SearchMojtFuncN)PxC3(NAME,NSHIFT,N),			\
  PxC2(NAME,_free),			\
};


INTERMEDIATE(memchr_search)
INTERMEDIATE(memchr_memcmp2)
INTERMEDIATE(memchr_memcmp3)
INTERMEDIATE(memchr_memcmp4)
INTERMEDIATE(memchr_memcmp5)
INTERMEDIATE(memchr_memcmp6)
INTERMEDIATE(boyer_moore_hubbe)
INTERMEDIATE(hubbe_search)


/* */
int NameN(init_hubbe_search)(struct hubbe_searcher *s,
			     NCHAR *needle,
			     ptrdiff_t needlelen,
			     ptrdiff_t max_haystacklen)
{
  INT32 tmp, h;
  ptrdiff_t hsize, e, max;
  NCHAR *q;
  struct hubbe_search_link *ptr;
  INT32 linklen[NELEM(s->set)];
  INT32 maxlinklength;

  s->needle=needle;
  s->needlelen=needlelen;

#ifdef PIKE_DEBUG
  if(needlelen < 7)
    Pike_fatal("hubbe search does not work with search strings shorter than 7 characters!\n");
#endif
  
#ifdef TUNAFISH
  hsize=52+(max_haystacklen >> 7)  - (needlelen >> 8);
  max  =13+(max_haystacklen >> 4)  - (needlelen >> 5);
  
  if(hsize > (ptrdiff_t) NELEM(s->set))
  {
    hsize=NELEM(s->set);
  }else{
    for(e=8;e<hsize;e+=e);
    hsize=e;
  }
#else
  hsize=NELEM(s->set);
  max=needlelen;
#endif


  for(e=0;e<hsize;e++)
  {
    s->set[e]=0;
    linklen[e]=0;
  }
  hsize--;
  
  if(max > (ptrdiff_t)needlelen) max=needlelen;
  max=(max-sizeof(INT32)+1) & -sizeof(INT32);
  if(max > MEMSEARCH_LINKS) max=MEMSEARCH_LINKS;
  
  /* This assumes 512 buckets - Hubbe */
  maxlinklength = my_sqrt(DO_NOT_WARN((unsigned int)max/2))+1;
  
  ptr=& s->links[0];
  
  q=(NCHAR *)needle;
  
#if PIKE_BYTEORDER == 4321 && NSHIFT == 0
  for(tmp = e = 0; e < (ptrdiff_t)sizeof(INT32)-1; e++)
  {
    tmp<<=8;
    tmp|=*(q++);
  }
#endif
	
  for(e=0;e<max;e++)
  {
#if PIKE_BYTEORDER == 4321  && NSHIFT == 0
    tmp<<=8;
    tmp|=*(q++);
#else
    /* FIXME tmp=EXTRACT_INT(q); */
    tmp=NameN(GET_4_UNALIGNED_CHARS)(q);
    q++;
#endif
    h=tmp;
    h+=h>>7;
    h+=h>>17;
    h&=hsize;
    
    ptr->offset=e;
    ptr->key=tmp;
    ptr->next=s->set[h];
    s->set[h]=ptr;
    ptr++;
    linklen[h]++;
    
    if(linklen[h] > maxlinklength)
    {
      return 0;
    }
    
  }
  s->hsize=hsize;
  s->max=max;

  return 1;
}
			

void NameN(init_boyer_moore_hubbe)(struct boyer_moore_hubbe_searcher *s,
				   NCHAR *needle,
				   ptrdiff_t needlelen,
				   ptrdiff_t max_haystacklen)
{
  ptrdiff_t e;

  s->needle=needle;
  s->needlelen=needlelen;

#ifdef PIKE_DEBUG
  if(needlelen < 2)
    Pike_fatal("boyer-moore-hubbe search does not work with single-character search strings!\n");
#endif
  
#ifdef TUNAFISH
  s->plen = 8 + ((max_haystacklen-needlelen) >> 5);
  if(s->plen>needlelen) s->plen=needlelen;
  if(s->plen>BMLEN) s->plen=BMLEN;
#else
  s->plen=BMLEN;
  if(s->plen>needlelen) s->plen=needlelen;
#endif
  
  for(e=0;e<(ptrdiff_t)NELEM(s->d1);e++) s->d1[e]=s->plen;
  
  for(e=0;e<s->plen;e++)
  {
    s->d1[ PxC3(BMHASH,NSHIFT,0) (needle[e]) ]=s->plen-e-1;
    s->d2[e]=s->plen*2-e-1;
  }
  
  for(e=0;e<s->plen;e++)
  {
    ptrdiff_t d;
    for(d=0;d<=e && needle[s->plen-1-d]==needle[e-d];d++);
    
    if(d>e)
    {
      while(s->plen-1-d>=0)
      {
	s->d2[s->plen-1-d]=s->plen-e+d-1;
	d++;
      }
    }else{
      s->d2[s->plen-1-d]=s->plen-e+d-1;
    }
  }

}


void NameN(init_memsearch)(
  struct pike_mem_searcher *s,
  NCHAR *needle,
  ptrdiff_t needlelen,
  ptrdiff_t max_haystacklen)
{
  switch(needlelen)
  {
    case 0:
      s->mojt.vtab=&nil_search_vtable;
      return;

    case 1:
      s->mojt.data=(void *)(ptrdiff_t)(needle[0]);
      s->mojt.vtab=& PxC3(memchr_search,NSHIFT,_vtable);
      return;

#define MMCASE(X)							\
    case X:								\
      s->mojt.data=(void *) needle;				\
      s->mojt.vtab=& PxC4(memchr_memcmp,X,NSHIFT,_vtable);	\
      return

      MMCASE(2);
      MMCASE(3);
      MMCASE(4);
      MMCASE(5);
      MMCASE(6);

#undef MMCASE

    case 7: case 8: case 9:
    case 10: case 11: case 12: case 13: case 14:
    case 15: case 16: case 17: case 18: case 19:
    case 20: case 21: case 22: case 23: case 24:
    case 25: case 26: case 27: case 28: case 29:
    case 30: case 31: case 32: case 33: case 34:
      break;

  default:
    if(max_haystacklen > needlelen + 64)
    {
      if(NameN(init_hubbe_search)(& s->data.hubbe,
				  needle,needlelen,
				  max_haystacklen))
      {
	s->mojt.vtab=& PxC3(hubbe_search,NSHIFT,_vtable);
	s->mojt.data=(void *)& s->data.hubbe;
	return;
      }
    }
  }

  NameN(init_boyer_moore_hubbe)(& s->data.bm,
				needle,needlelen,
				max_haystacklen);
  s->mojt.vtab=& PxC3(boyer_moore_hubbe,NSHIFT,_vtable);
  s->mojt.data=(void *)& s->data.bm;
}


SearchMojt NameN(compile_memsearcher)(NCHAR *needle,
				      ptrdiff_t len,
				      int max_haystacklen,
				      struct pike_string *hashkey)
{
  /* FIXME:
   * if NSHIFT > 0 but all characters in needle can fit in
   * a narrower string, it might pay off to reduce the width
   * of the string.
   */
  if(len<7)
  {
    struct pike_mem_searcher tmp;
    NameN(init_memsearch)(&tmp,
			  needle,len,
			  max_haystacklen);
    return tmp.mojt;
  }else{
    struct svalue *sval,stmp;
    struct pike_mem_searcher *s;
    struct object *o;
    
    if(!hashkey)
      hashkey=NameN(make_shared_binary_string)(needle,len);
    else
      add_ref(hashkey);

    if((sval=low_mapping_string_lookup(memsearch_cache,hashkey)))
    {
      if(sval->type == T_OBJECT)
      {
	o=sval->u.object;
	if(o->prog == pike_search_program)
	{
	  s=OB2MSEARCH(sval->u.object);
	  add_ref(sval->u.object);
	  free_string(hashkey);
	  return s->mojt;
	}
      }
    }

    o=low_clone(pike_search_program);
    s=OB2MSEARCH(o);
    s->data.hubbe.o=o;
    s->s=hashkey;

    /* We use 0x7fffffff for max_haystacklen because we do
     * not know how many times this search struct will be
     * reused.
     */
    NameN(init_memsearch)(s,
			  needle,len,
			  0x7fffffff);
    stmp.type = T_OBJECT;
    stmp.subtype = 0;
    stmp.u.object = o;

    mapping_string_insert(memsearch_cache, hashkey, &stmp);

    return s->mojt;
  }
}
