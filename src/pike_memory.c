/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
#include "pike_memory.h"
#include "error.h"
#include "pike_macros.h"


/* strdup() is used by several modules, so let's provide it */
#ifndef HAVE_STRDUP
char *strdup(const char *str)
{
  char *res = NULL;
  if (str) {
    int len = strlen(str)+1;

    res = xalloc(len);
    if (res) {
      MEMCPY(res, str, len);
    }
  }
  return(res);
}
#endif /* !HAVE_STRDUP */

void swap(char *a, char *b, INT32 size)
{
  int tmp;
  char tmpbuf[1024];
  while(size)
  {
    tmp=MINIMUM((long)sizeof(tmpbuf), size);
    MEMCPY(tmpbuf,a,tmp);
    MEMCPY(a,b,tmp);
    MEMCPY(b,tmpbuf,tmp);
    size-=tmp;
    a+=tmp;
    b+=tmp;
  }
}

void reverse(char *memory, INT32 nitems, INT32 size)
{

#define DOSIZE(X,Y)						\
 case X:							\
 {								\
  Y tmp;							\
  Y *start=(Y *) memory;					\
  Y *end=start+nitems-1;					\
  while(start<end){tmp=*start;*(start++)=*end;*(end--)=tmp;}	\
  break;							\
 }

#ifdef HANDLES_UNALIGNED_MEMORY_ACCESS
  switch(size)
#else
  switch( (((unsigned long)memory) % size) ? 0 : size)
#endif
  {
    DOSIZE(1,B1_T)
#ifdef B2_T
    DOSIZE(2,B2_T)
#endif
#ifdef B4_T
    DOSIZE(4,B4_T)
#endif
#ifdef B16_T
    DOSIZE(8,B8_T)
#endif
#ifdef B16_T
    DOSIZE(16,B8_T)
#endif
  default:
  {
    char *start = (char *) memory;
    char *end=start+(nitems-1)*size;
    while(start<end)
    {
      swap(start,end,size);
      start+=size;
      end-=size;
    }
  }
  }
}

/*
 * This function may NOT change 'order'
 * This function is hopefully fast enough...
 */
void reorder(char *memory, INT32 nitems, INT32 size,INT32 *order)
{
  INT32 e;
  char *tmp;
  if(nitems<2) return;


  tmp=xalloc(size * nitems);

#undef DOSIZE
#define DOSIZE(X,Y)				\
 case X:					\
 {						\
  Y *from=(Y *) memory;				\
  Y *to=(Y *) tmp;				\
  for(e=0;e<nitems;e++) to[e]=from[order[e]];	\
  break;					\
 }
  

#ifdef HANDLES_UNALIGNED_MEMORY_ACCESS
  switch(size)
#else
  switch( (((unsigned long)memory) % size) ? 0 : size )
#endif
 {
   DOSIZE(1,B1_T)
#ifdef B2_T
     DOSIZE(2,B2_T)
#endif
#ifdef B4_T
     DOSIZE(4,B4_T)
#endif
#ifdef B8_T
     DOSIZE(8,B8_T)
#endif
#ifdef B16_T
    DOSIZE(16,B16_T)
#endif

  default:
    for(e=0;e<nitems;e++) MEMCPY(tmp+e*size, memory+order[e]*size, size);
  }

  MEMCPY(memory, tmp, size * nitems);
  free(tmp);
}

unsigned INT32 hashmem(const unsigned char *a,INT32 len,INT32 mlen)
{
  unsigned INT32 ret;

  ret=9248339*len;
  if(len<mlen)
    mlen=len;
  else
  {
    switch(len-mlen)
    {
      default: ret^=(ret<<6) + a[len-7];
      case 7:
      case 6: ret^=(ret<<7) + a[len-5];
      case 5:
      case 4: ret^=(ret<<4) + a[len-4];
      case 3: ret^=(ret<<3) + a[len-3];
      case 2: ret^=(ret<<3) + a[len-2];
      case 1: ret^=(ret<<3) + a[len-1];
    }
  }
  switch(mlen&7)
  {
    case 7: ret^=*(a++);
    case 6: ret^=(ret<<4)+*(a++);
    case 5: ret^=(ret<<7)+*(a++);
    case 4: ret^=(ret<<6)+*(a++);
    case 3: ret^=(ret<<3)+*(a++);
    case 2: ret^=(ret<<7)+*(a++);
    case 1: ret^=(ret<<5)+*(a++);
  }

#ifdef HANDLES_UNALIGNED_MEMORY_ACCESS
  {
    unsigned INT32 *b;
    b=(unsigned INT32 *)a;

    for(mlen>>=3;--mlen>=0;)
    {
      ret^=(ret<<7)+*(b++);
      ret^=(ret>>6)+*(b++);
    }
  }
#else
  for(mlen>>=3;--mlen>=0;)
  {
    register unsigned int t1,t2;
    t1= *(a++);
    t2= *(a++);
    t1=(t1<<5) + *(a++);
    t2=(t2<<4) + *(a++);
    t1=(t1<<7) + *(a++);
    t2=(t2<<5) + *(a++);
    t1=(t1<<3) + *(a++);
    t2=(t2<<4) + *(a++);
    ret^=(ret<<7) + (ret>>6) + t1 + (t2<<6);
  }
#endif

  return ret;
}

unsigned INT32 hashstr(const unsigned char *str,INT32 maxn)
{
  unsigned INT32 ret,c;
  
  ret=str++[0];
  for(; maxn>=0; maxn--)
  {
    c=str++[0];
    if(!c) break;
    ret ^= ( ret << 4 ) + c ;
    ret &= 0x7fffffff;
  }

  return ret;
}


/*
 * a quick memory search function.
 * Written by Fredrik Hubinette (hubbe@lysator.liu.se)
 */
void init_memsearch(struct mem_searcher *s,
		    char *needle,
		    SIZE_T needlelen,
		    SIZE_T max_haystacklen)
{
  s->needle=needle;
  s->needlelen=needlelen;

  switch(needlelen)
  {
  case 0: s->method=no_search; break;
  case 1: s->method=use_memchr; break;
  case 2:
  case 3:
  case 4:
  case 5:
  case 6: s->method=memchr_and_memcmp; break;
  default:
    if(max_haystacklen <= needlelen + 64)
    {
      s->method=memchr_and_memcmp;
    }else{
      INT32 tmp, h;
      unsigned INT32 hsize, e, max;
      unsigned char *q;
      struct link *ptr;

      hsize=52+(max_haystacklen >> 7)  - (needlelen >> 8);
      max  =13+(max_haystacklen >> 4)  - (needlelen >> 5);

      if(hsize > NELEM(s->set))
      {
	hsize=NELEM(s->set);
      }else{
	for(e=8;e<hsize;e+=e);
	hsize=e;
      }
    
      for(e=0;e<hsize;e++) s->set[e]=0;
      hsize--;

      if(max > needlelen) max=needlelen;
      max=(max-sizeof(INT32)+1) & -sizeof(INT32);
      if(max > MEMSEARCH_LINKS) max=MEMSEARCH_LINKS;

      ptr=& s->links[0];

      q=(unsigned char *)needle;

#if BYTEORDER == 4321
      for(tmp=e=0;e<sizeof(INT32)-1;e++)
      {
	tmp<<=8;
	tmp|=*(q++);
      }
#endif

      for(e=0;e<max;e++)
      {
#if BYTEORDER == 4321
	tmp<<=8;
	tmp|=*(q++);
#else
	tmp=EXTRACT_INT(q);
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
      }
      s->hsize=hsize;
      s->max=max;
      s->method=hubbe_search;
    }
  }
}
		    

char *memory_search(struct mem_searcher *s,
		    char *haystack,
		    SIZE_T haystacklen)
{
  if(s->needlelen > haystacklen) return 0;

  switch(s->method)
  {
  case no_search:
    return haystack;

  case use_memchr:
    return MEMCHR(haystack,s->needle[0],haystacklen);

  case memchr_and_memcmp:
    {
      char *end,c,*needle;
      SIZE_T needlelen;
      
      needle=s->needle;
      needlelen=s->needlelen;
      
      end=haystack + haystacklen - needlelen+1;
      c=needle[0];
      needle++;
      needlelen--;
      while((haystack=MEMCHR(haystack,c,end-haystack)))
	if(!MEMCMP(++haystack,needle,needlelen))
	  return haystack-1;

      return 0;
    }

  case hubbe_search:
    {
      INT32 tmp, h;
      char *q, *end;
      register struct link *ptr;
      
      end=haystack+haystacklen;
      q=haystack + s->max - sizeof(INT32);
      q=(char *)( ((long)q) & -sizeof(INT32));
      for(;q<=end-sizeof(INT32);q+=s->max)
      {
	h=tmp=*(INT32 *)q;
	
	h+=h>>7;
	h+=h>>17;
	h&=s->hsize;
	
	for(ptr=s->set[h];ptr;ptr=ptr->next)
	{
	  char *where;
	  
	  if(ptr->key != tmp) continue;
	  
	  where=q-ptr->offset;
	  if(where<haystack) continue;
	  if(where+s->needlelen>end) return 0;
	  
	  if(!MEMCMP(where,s->needle,s->needlelen))
	    return where;
	}
      }
    }
  }
  return 0;
}

char *my_memmem(char *needle,
		SIZE_T needlelen,
		char *haystack,
		SIZE_T haystacklen)
{
  struct mem_searcher tmp;
  init_memsearch(&tmp, needle, needlelen, haystacklen);
  return memory_search(&tmp, haystack, haystacklen);
}

void memfill(char *to,
	     INT32 tolen,
	     char *from,
	     INT32 fromlen,
	     INT32 offset)
{
  if(fromlen==1)
  {
    MEMSET(to, *from, tolen);
  }
  else if(tolen>0)
  {
    INT32 tmp=MINIMUM(tolen, fromlen - offset);
    MEMCPY(to, from + offset, tmp);
    to+=tmp;
    tolen-=tmp;

    if(tolen > 0)
    {
      tmp=MINIMUM(tolen, fromlen);
      MEMCPY(to, from, tmp);
      from=to;
      to+=tmp;
      tolen-=tmp;
      
      while(tolen>0)
      {
	tmp=MINIMUM(tolen, fromlen);
	MEMCPY(to, from, MINIMUM(tolen, fromlen));
	fromlen+=tmp;
	tolen-=tmp;
	to+=tmp;
      }
    }
  }
}

char *debug_xalloc(long size)
{
  char *ret;
  if(!size) return 0;

  ret=(char *)malloc(size);
  if(ret) return ret;

  error("Out of memory.\n");
  return 0;
}

#ifdef DEBUG_MALLOC

#include "threads.h"

#ifdef _REENTRANT
static MUTEX_T debug_malloc_mutex;
#endif


#undef malloc
#undef free
#undef realloc
#undef calloc
#undef strdup
#undef main

static void add_location(struct memhdr *mh, int locnum);
static struct memhdr *find_memhdr(void *p);

#include "block_alloc.h"

int verbose_debug_malloc = 0;
int verbose_debug_exit = 1;

#define HSIZE 1109891
#define LHSIZE 1109891
#define FLSIZE 8803

struct fileloc
{
  struct fileloc *next;
  const char *file;
  int line;
  int number;
};

BLOCK_ALLOC(fileloc, 4090)

struct memloc
{
  struct memloc *next;
  struct memhdr *mh;
  int locnum;
  int times;
};

BLOCK_ALLOC(memloc, 16382)

struct memhdr
{
  struct memhdr *next;
  size_t size;
  void *data;
  struct memloc *locations;
};

static struct fileloc *flhash[FLSIZE];
static struct memloc *mlhash[LHSIZE];
static struct memhdr *hash[HSIZE];

static struct memhdr no_leak_memlocs;
static int file_location_number=0;

static int location_number(const char *file, int line)
{
  struct fileloc *f,**prev;
  unsigned long h=(long)file;
  h*=4711;
  h+=line;
  h%=FLSIZE;
  for(prev=flhash+h;(f=*prev);prev=&f->next)
  {
    if(f->line == line && f->file == file)
    {
      *prev=f->next;
      f->next=flhash[h];
      flhash[h]=f;
      return f->number;
    }
  }

  f=alloc_fileloc();
  f->line=line;
  f->file=file;
  f->number=++file_location_number;
  f->next=flhash[h];
  flhash[h]=f;
  return f->number;
}

static struct fileloc *find_file_location(int locnum)
{
  int e;
  struct fileloc *r;
  for(e=0;e<FLSIZE;e++)
    for(r=flhash[e];r;r=r->next)
      if(r->number == locnum)
	return r;
  fprintf(stderr,"Internal error in DEBUG_MALLOC, failed to find location.\n");
  exit(127);
}

void low_add_marks_to_memhdr(struct memhdr *to,
			     struct memhdr *from)
{
  struct memloc *l;
  if(!from) return;
  for(l=from->locations;l;l=l->next)
    add_location(to, l->locnum);
}

void add_marks_to_memhdr(struct memhdr *to, void *ptr)
{
  low_add_marks_to_memhdr(to,find_memhdr(ptr));
}

static inline unsigned long lhash(struct memhdr *m, int locnum)
{
  unsigned long l;
  l=(long)m;
  l*=53;
  l+=locnum;
  l%=LHSIZE;
  return l;
}

#undef INIT_BLOCK
#undef EXIT_BLOCK

#define INIT_BLOCK(X) X->locations=0
#define EXIT_BLOCK(X) do {				\
  struct memloc *ml;					\
  while((ml=X->locations))				\
  {							\
    unsigned long l=lhash(X,ml->locnum);		\
    if(mlhash[l]==ml) mlhash[l]=0;			\
							\
    X->locations=ml->next;				\
    free_memloc(ml);					\
  }							\
}while(0)

BLOCK_ALLOC(memhdr,16382)

#undef INIT_BLOCK
#undef EXIT_BLOCK

#define INIT_BLOCK(X)
#define EXIT_BLOCK(X)


static struct memhdr *find_memhdr(void *p)
{
  struct memhdr *mh,**prev;
  unsigned long h=(long)p;
  h%=HSIZE;
  for(prev=hash+h; (mh=*prev); prev=&mh->next)
  {
    if(mh->data==p)
    {
      *prev=mh->next;
      mh->next=hash[h];
      hash[h]=mh;
      return mh;
    }
  }
  return NULL;
}


static int find_location(struct memhdr *mh, int locnum)
{
  struct memloc *ml;
  unsigned long l=lhash(mh,locnum);

  if(mlhash[l] &&
     mlhash[l]->mh==mh &&
     mlhash[l]->locnum==locnum)
    return 1;

  for(ml=mh->locations;ml;ml=ml->next)
  {
    if(ml->locnum==locnum)
    {
      mlhash[l]=ml;
      return 1;
    }
  }
  return 0;
}

static void add_location(struct memhdr *mh, int locnum)
{
  struct memloc *ml;
  unsigned long l;

#if ( DEBUG_MALLOC - 0 ) < 2
  if(find_location(&no_leak_memlocs, locnum)) return;
#endif

  l=lhash(mh,locnum);

  if(mlhash[l] && mlhash[l]->mh==mh && mlhash[l]->locnum==locnum)
  {
    mlhash[l]->times++;
    return;
  }

  for(ml=mh->locations;ml;ml=ml->next)
    if(ml->locnum==locnum)
      break;

  if(!ml)
  {
    ml=alloc_memloc();
    ml->locnum=locnum;
    ml->next=mh->locations;
    ml->mh=mh;
    mh->locations=ml;
  }
  ml->times++;
  mlhash[l]=ml;
}

static void make_memhdr(void *p, int s, int locnum)
{
  struct memhdr *mh=alloc_memhdr();
  struct memloc *ml=alloc_memloc();
  unsigned long l=lhash(mh,locnum);
  unsigned long h=(long)p;
  h%=HSIZE;

  mh->next=hash[h];
  mh->data=p;
  mh->size=s;
  mh->locations=ml;
  ml->locnum=locnum;
  ml->next=0;
  ml->times=1;
  hash[h]=mh;
  mlhash[l]=ml;
}

static int remove_memhdr(void *p)
{
  struct memhdr **prev,*mh;
  unsigned long h=(long)p;
  h%=HSIZE;
  for(prev=hash+h;(mh=*prev);prev=&(mh->next))
  {
    if(mh->data==p)
    {
      *prev=mh->next;
      low_add_marks_to_memhdr(&no_leak_memlocs, mh);
      free_memhdr(mh);
      
      return 1;
    }
  }
  return 0;
}

void *debug_malloc(size_t s, const char *fn, int line)
{
  void *m;

  mt_lock(&debug_malloc_mutex);

  m=malloc(s);
  if(m)
    make_memhdr(m, s, location_number(fn,line));

  if(verbose_debug_malloc)
    fprintf(stderr, "malloc(%d) => %p  (%s:%d)\n", s, m, fn, line);

  mt_unlock(&debug_malloc_mutex);
  return m;
}


void *debug_calloc(size_t a, size_t b, const char *fn, int line)
{
  void *m;
  int locnum;
  mt_lock(&debug_malloc_mutex);
  m=calloc(a, b);

  if(m) make_memhdr(m, a*b, location_number(fn,line));

  if(verbose_debug_malloc)
    fprintf(stderr, "calloc(%d,%d) => %p  (%s:%d)\n", a, b, m, fn, line);

  mt_unlock(&debug_malloc_mutex);
  return m;
}

void *debug_realloc(void *p, size_t s, const char *fn, int line)
{
  void *m;
  mt_lock(&debug_malloc_mutex);
  m=realloc(p, s);
  if(m) {
    if(p) remove_memhdr(p);
    make_memhdr(m, s, location_number(fn,line));
  }
  if(verbose_debug_malloc)
    fprintf(stderr, "realloc(%p,%d) => %p  (%s:%d)\n", p, s, m, fn,line);
  mt_unlock(&debug_malloc_mutex);
  return m;
}

void debug_free(void *p, const char *fn, int line)
{
  mt_lock(&debug_malloc_mutex);
  remove_memhdr(p);
  free(p);
  if(verbose_debug_malloc)
    fprintf(stderr, "free(%p) (%s:%d)\n", p, fn,line);
  mt_unlock(&debug_malloc_mutex);
}

char *debug_strdup(const char *s, const char *fn, int line)
{
  char *m;
  mt_lock(&debug_malloc_mutex);
  m=strdup(s);

  if(m) make_memhdr(m, strlen(s)+1, location_number(fn,line));

  if(verbose_debug_malloc)
    fprintf(stderr, "strdup(\"%s\") => %p  (%s:%d)\n", s, m, fn, line);
  mt_unlock(&debug_malloc_mutex);
  return m;
}


void dump_memhdr_locations(struct memhdr *from,
			   struct memhdr *notfrom)
{
  struct memloc *l;
  for(l=from->locations;l;l=l->next)
  {
    struct fileloc *f;
    if(notfrom && find_location(notfrom, l->locnum))
      continue;

    f=find_file_location(l->locnum);
    fprintf(stderr," *** %s:%d (%d times)\n",f->file,f->line,l->times);
  }
}

void cleanup_memhdrs(void)
{
  unsigned long h;
  mt_lock(&debug_malloc_mutex);
  if(verbose_debug_exit)
  {
    int first=1;
    for(h=0;h<HSIZE;h++)
    {
      struct memhdr *m;
      for(m=hash[h];m;m=m->next)
      {
	struct memloc *l;
	if(first)
	{
	  fprintf(stderr,"\n");
	  first=0;
	}

	
	fprintf(stderr, "LEAK: (%p) %d bytes\n",m->data, m->size);
	for(l=m->locations;l;l=l->next)
	{
	  struct fileloc *f=find_file_location(l->locnum);
	  fprintf(stderr,"  *** %s:%d (%d times) %s\n",
		  f->file,
		  f->line,
		  l->times,
		  find_location(&no_leak_memlocs, l->locnum) ? "" : " *");
	}
      }
    }
  }
  mt_unlock(&debug_malloc_mutex);
  mt_destroy(&debug_malloc_mutex);
}

int main(int argc, char *argv[])
{
  extern int dbm_main(int, char **);
  mt_init(&debug_malloc_mutex);
  return dbm_main(argc, argv);
}

void * debug_malloc_update_location(void *p,const char *fn, int line)
{
  if(p)
  {
    struct memhdr *mh;
    if((mh=find_memhdr(p)))
      add_location(mh, location_number(fn,line));
  }
  return p;
}


#endif
