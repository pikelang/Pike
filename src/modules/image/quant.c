/*

quant, used by image when making gif's (mainly)

Pontus Hagland, law@infovav.se
David Kågedal, kg@infovav.se

*/

#include <unistd.h>
#include <stdio.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "types.h"
#include "error.h"
#include "global.h"
#include "array.h"

#include "image.h"

/*
#define QUANT_DEBUG
#define QUANT_DEBUG_DEEP
#define QUANT_CHRONO
*/

/**********************************************************************/

#ifdef QUANT_CHRONO
#include <sys/resource.h>
#define CHRONO(X) chrono(X)

void chrono(char *x)
{
   struct rusage r;
   getrusage(RUSAGE_SELF,&r);
   fprintf(stderr,"%s: %ld.%06ld %ld.%06ld %ld.%06ld\n",x,
	   r.ru_utime.tv_sec,r.ru_utime.tv_usec,
	   r.ru_stime.tv_sec,r.ru_stime.tv_usec,
	   r.ru_stime.tv_sec+r.ru_utime.tv_sec,
	   r.ru_stime.tv_usec+r.ru_utime.tv_usec);
}
#else
#define CHRONO(X)
#endif

/**********************************************************************/

typedef struct
{
  rgb_group rgb;
  unsigned long count;
  unsigned long next;
} rgb_entry;

typedef struct
{
  unsigned long len;
  rgb_entry tbl[1];
} rgb_hashtbl;

typedef struct colortable coltab;

#define hash(rgb,l) (((rgb).r*127+(rgb).g*997+(rgb).b*2111)&(l-1))
#define same_col(rgb1,rgb2) \
     (((rgb1).r==(rgb2).r) && ((rgb1).g==(rgb2).g) && ((rgb1).b==(rgb2).b))


static INLINE int hash_enter(rgb_entry *rgbe,rgb_entry *end,
			      int len,rgb_group rgb)
{
   register rgb_entry *re;

   re=rgbe+hash(rgb,len);
/*   fprintf(stderr,"%d\n",hash(rgb,len));*/

   while (re->count && !same_col(re->rgb,rgb))
      if (++re==end) re=rgbe;

   if (!re->count)  /* allocate it */
   {
      re->rgb=rgb; 
      re->count=1; 
      return 1; 
   }

   re->count++;
   return 0;
}

static rgb_hashtbl *img_rehash(rgb_hashtbl *old, unsigned long newsize)
{
   unsigned long i;
   rgb_hashtbl *new = malloc(sizeof(rgb_hashtbl) + 
			     sizeof(rgb_entry) * newsize  );
   MEMSET(new->tbl, 0, sizeof(rgb_entry) * newsize );
  
   if (!new) error("Out of memory\n");
#ifdef QUANT_DEBUG
   fprintf(stderr,"img_rehash: old size = %lu, new size = %lu\n",
	   (old ? old->len : 0), newsize);
#endif
  
   new->len = newsize;
   if (old)
   {
      for (i=0;i<old->len;i++)
	 if (old->tbl[i].count)
	 {
	    register rgb_entry *re;

	    re=new->tbl+hash(old->tbl[i].rgb,newsize);

	    while (re->count)
	       if (++re==new->tbl+newsize) re=new->tbl;

	    *re=old->tbl[i];
	 }
      free(old);
   }
   return new;
}

static int cmp_red(rgb_entry *e1, rgb_entry *e2)
{
   return e1->rgb.r - e2->rgb.r;
}

static int cmp_green(rgb_entry *e1, rgb_entry *e2)
{
   return e1->rgb.g - e2->rgb.g;
}

static int cmp_blue(rgb_entry *e1, rgb_entry *e2)
{
   return e1->rgb.b - e2->rgb.b;
}

static int get_tbl_median(rgb_entry *tbl,int len)
{
   int a=0,x=0;
   rgb_entry *m,*n;

   len--;
   m=tbl; n=tbl+len-1;
   while (m<n)
   {
#if 0
      fprintf(stderr,"pos %3d,%3d sum %3d low=%3d,%3d,%3dc%3d high=%3d,%3d,%3dc%3d\n",
	      x,len,a,
	      tbl[x].rgb.r,tbl[x].rgb.g,tbl[x].rgb.b,tbl[x].count, 
	      tbl[len].rgb.r,tbl[len].rgb.g,tbl[len].rgb.b,tbl[len].count); 
#endif
      if (a>0) a-=m++->count; 
      else a+=n--->count;
   }
   return m-tbl;
}

static rgb_group get_tbl_point(rgb_entry *tbl,int len)
{
   unsigned long r=0,g=0,b=0,n=0;
   int x;
   rgb_group rgb;
   for (x=0; x<len; x++)
   {
      r+=((unsigned long)tbl[x].rgb.r)*tbl[x].count,
      g+=((unsigned long)tbl[x].rgb.g)*tbl[x].count,
      b+=((unsigned long)tbl[x].rgb.b)*tbl[x].count;
      n+=tbl[x].count;
#ifdef QUANT_DEBUG
   fprintf(stderr,"(%d,%d,%d)*%lu; ",
	   tbl[x].rgb.r,
	   tbl[x].rgb.g,
	   tbl[x].rgb.b,
	   tbl[x].count);
#endif
   }
   rgb.r=(unsigned char)(r/n);
   rgb.g=(unsigned char)(g/n);
   rgb.b=(unsigned char)(b/n);
#ifdef QUANT_DEBUG
   fprintf(stderr,"-> (%lu,%lu,%lu)/%lu = %d,%d,%d\n",
	   r,g,b,n,
	   rgb.r,rgb.g,rgb.b);
#endif
   return rgb;
}

#define MAKE_BINSORT(C,NAME) \
   void NAME(rgb_entry *e, unsigned long len) \
   { \
      unsigned long pos[256];\
      unsigned long i,j,k;\
   \
      rgb_entry *e2;\
      e2=(rgb_entry*)xalloc(sizeof(rgb_entry)*len);\
   \
      for (i=0; i<256; i++) pos[i]=~0;\
      \
      for (i=0; i<len; i++)\
      {\
         e[i].next=pos[j=e[i].rgb.C];\
         pos[j]=i;\
      }\
   \
      for (i=j=0; i<256; i++)\
      {\
         k=pos[i];\
         while (k!=(unsigned long)~0)\
         {\
   	 e2[j++]=e[k];\
   	 k=e[k].next;\
         }\
      }\
      MEMCPY(e,e2,len*sizeof(rgb_entry));\
      free(e2);\
   }

MAKE_BINSORT(r,binsort_red)
MAKE_BINSORT(g,binsort_green)
MAKE_BINSORT(b,binsort_blue)


static void sort_tbl(rgb_hashtbl *ht, 
		     unsigned long start, unsigned long len,
		     int level, unsigned long idx,
		     unsigned long gap, int ldir,
		     coltab *ct, rgb_group lower,rgb_group upper)
{
   rgb_entry *tbl = ht->tbl;

   int (*sortfun)(const void *, const void *);
   unsigned long x,y;
   int dir=0;

#ifdef QUANT_DEBUG

   fprintf(stderr,"%*ssort_tbl: level %d  start = %lu "
	          " len=%lu, idx = %lu, gap = %lu",
	   level, "",
	   level, start, len, idx, gap);
   fprintf(stderr,"\n%*s%d,%d,%d-%d,%d,%d ",level,"",
	   lower.r,lower.g,lower.b,upper.r,upper.g,upper.b);

   fprintf(stderr,"[%d,%d,%d] - [%d,%d,%d]\n",
	      QUANT_MAP_THIS(lower.r), 
	      QUANT_MAP_THIS(lower.g), 
	      QUANT_MAP_THIS(lower.b), 
	      QUANT_MAP_THIS(upper.r), 
	      QUANT_MAP_THIS(upper.g), 
	      QUANT_MAP_THIS(upper.b));

#endif

   if (len>1)
   {
   /* check which direction has the most span */
   /* we make it easy for us, only three directions: r,g,b */

      rgb_group min,max;

      max.r=min.r=tbl[start].rgb.r;
      max.g=min.g=tbl[start].rgb.g;
      max.b=min.b=tbl[start].rgb.b;

      for (x=1; x<=len; x++)
      {
	 if (min.r>tbl[x].rgb.r) min.r=tbl[x].rgb.r;
	 if (min.g>tbl[x].rgb.g) min.g=tbl[x].rgb.g;
	 if (min.b>tbl[x].rgb.b) min.b=tbl[x].rgb.b;
	 if (max.r<tbl[x].rgb.r) max.r=tbl[x].rgb.r;
	 if (max.g<tbl[x].rgb.g) max.g=tbl[x].rgb.g;
	 if (max.b<tbl[x].rgb.b) max.b=tbl[x].rgb.b;
      }

      /* do this weighted, red=2 green=3 blue=1 */
      if ((max.r-min.r)*5>(max.g-min.g)*8) /* r>g */
	 if ((max.r-min.r)*5>(max.b-min.b)) /* r>g, r>b */
	    dir=0;
         else /* r>g, b>r */
	    dir=2;
      else
	 if ((max.g-min.g)*8>(max.b-min.b)) /* g>r, g>b */
	    dir=1;
         else /* g>r, b>g */
	    dir=2;
      
#ifdef QUANT_DEBUG
      fprintf(stderr," dir=%d ",dir);
#endif
      if (dir!=ldir)
	 switch (dir)
	 {
	    case 0: binsort_red(tbl+start,len); break;
	    case 1: binsort_green(tbl+start,len); break;
	    case 2: binsort_blue(tbl+start,len); break;
	 }

#ifdef QUANT_DEBUG
      fprintf(stderr,"low: %d,%d,%d high: %d,%d,%d\n",
	      tbl[start].rgb.r,
	      tbl[start].rgb.g,
	      tbl[start].rgb.b,
	      tbl[start+len-1].rgb.r,
	      tbl[start+len-1].rgb.g,
	      tbl[start+len-1].rgb.b);
#endif

   }

   if (len>1 && gap>1)
   {
      unsigned long pos,g1,g2;
      rgb_group less,more,rgb;

      pos=get_tbl_median(tbl+start,len);
      rgb=tbl[start+pos].rgb;

#ifdef QUANT_DEBUG
      fprintf(stderr, " pos=%lu len=%lu\n",pos,len);
#endif		

      less=upper;
      more=lower;

      switch (dir)
      {
	 case 0: more.r=rgb.r+1; less.r=rgb.r;break;
	 case 1: more.g=rgb.g+1; less.g=rgb.g;break;
	 case 2: more.b=rgb.b+1; less.b=rgb.b;break;
      }

      g1=gap>>1;
      if (pos+1<g1) g1=pos+1,g2=gap-g1;
      else if (len-pos-1<gap-g1) g2=(len-pos)+1,g1=gap-g2;
      else g2=gap-g1;

      sort_tbl(ht,start,pos+1,
	       level+1,idx,g1,dir,
	       ct,lower,less);

      if (gap>1)
	 sort_tbl(ht,start+pos+1,len-pos-1,
		  level+1,idx+g1,g2,dir,
		  ct,more,upper);
   }
   else 
   {
      int r,g,b;
      if (len>1)
	 ct->clut[idx]=get_tbl_point(tbl+start,len);
      else
	 ct->clut[idx]=tbl[start].rgb;

#ifdef QUANT_DEBUG
      fprintf(stderr,"\n [%d,%d,%d] - [%d,%d,%d] %u\n",
	      lower.r, lower.g, lower.b, upper.r, upper.g, upper.b,
	      (upper.r-lower.r+1)*(upper.g-lower.g+1)*(upper.b-lower.b+1));
      fprintf(stderr,"[%d,%d,%d] - [%d,%d,%d]\n",
	      QUANT_MAP_THIS(lower.r), 
	      QUANT_MAP_THIS(lower.g), 
	      QUANT_MAP_THIS(lower.b), 
	      QUANT_MAP_THIS(upper.r), 
	      QUANT_MAP_THIS(upper.g), 
	      QUANT_MAP_THIS(upper.b));
#endif      

#ifdef QUANT_DEBUG
#ifndef QUANT_DEBUG_DEEP
      fprintf(stderr,"[%d,%d,%d]-[%d,%d,%d] = %lu (%d,%d,%d)\n",
   QUANT_MAP_THIS(lower.r), QUANT_MAP_THIS(lower.g), QUANT_MAP_THIS(lower.b),
   QUANT_MAP_THIS(upper.r), QUANT_MAP_THIS(upper.g), QUANT_MAP_THIS(upper.b),
	      idx, ct->clut[idx].r,ct->clut[idx].g,ct->clut[idx].b);
#endif
#endif

      for(r = QUANT_MAP_THIS(lower.r); r <= QUANT_MAP_THIS(upper.r); r++)
	 for(g = QUANT_MAP_THIS(lower.g); g <= QUANT_MAP_THIS(upper.g); g++)
	    for(b = QUANT_MAP_THIS(lower.b); b <= QUANT_MAP_THIS(upper.b); b++)
	    {
#ifdef QUANT_DEBUG_DEEP
	       fprintf(stderr,"%*s[%d,%d,%d] = %lu (%d,%d,%d)\n",level,"",
		       r,g,b,idx,
		       ct->clut[idx].r,ct->clut[idx].g,ct->clut[idx].b);
#endif
	       if (ct->map[r][g][b].used)
	       {
		  struct map_entry *me;
		  me=(struct map_entry *)xalloc(sizeof(struct map_entry));
		  me->used=1;
		  me->cl=idx;
		  me->next=ct->map[r][g][b].next;
		  ct->map[r][g][b].next=me;
	       }
	       else
	       {
		  ct->map[r][g][b].used=1;
		  ct->map[r][g][b].next=NULL;
		  ct->map[r][g][b].cl=idx;
	       }
	    }
   }
}


struct colortable *colortable_quant(struct image *img,int numcol)
{
  rgb_hashtbl *tbl;
  INT32 i,j;
  INT32 sz = img->xsize * img->ysize;
  rgb_entry entry;
  coltab *ct;
  rgb_group black,white;
  rgb_group *p;
  INT32 entries=0;

#ifdef QUANT_DEBUG
  fprintf(stderr,"img_quant called\n");
#endif
  CHRONO("quant");

  if (numcol<2) numcol=2;
  if (numcol>MAX_NUMCOL) numcol=MAX_NUMCOL; 

  ct = malloc(sizeof(struct colortable)+sizeof(rgb_group)*numcol);
  if (!ct) error("Out of memory.\n");
  MEMSET(ct,0,sizeof(struct colortable)+sizeof(rgb_group)*numcol);
  ct->numcol=numcol;

#ifdef QUANT_DEBUG
  fprintf(stderr,"Moving colors into hashtable\n");
#endif
  CHRONO("hash init");

  tbl = img_rehash(NULL, 8192);

  if (1)
  {
     p=img->img;
     i=sz;
     do
     {
	register rgb_entry *rgbe,*end;
	int len,trig;

#ifdef QUANT_DEBUG
	fprintf(stderr,"hash: %d pixels left...\n",i);
#endif
	CHRONO("hash...");
	
	len=tbl->len;
	trig=(len*2)/10; /* 20% full => rehash bigger */
	end=(rgbe=tbl->tbl)+tbl->len;
     
	while (i--)
	   if ( (entries+=hash_enter(rgbe,end,len,*(p++))) > trig )
	   {
#ifdef QUANT_DEBUG
	      fprintf(stderr,"rehash: 20%% = %d / %d...\n",entries,len);
#endif
	      CHRONO("rehash...");
	      tbl=img_rehash(tbl,len<<2); /* multiple size by 4 */
	      break;
	   }
     }
     while (i>=0);
  }
     
  /* Compact the hash table */
  CHRONO("compact");

#ifdef QUANT_DEBUG
  fprintf(stderr,"Compacting\n");
#endif
  i = tbl->len - 1;
  j = 0;
  while (i > entries)
  {
    while ((i >= entries) && tbl->tbl[i].count == 0) i--;
    while ((j <  entries) && tbl->tbl[j].count != 0) j++;
    if (j<i)
    {
      tbl->tbl[j] = tbl->tbl[i];
      tbl->tbl[i].count = 0;
    }
  }

  white.r=white.g=white.b=255;
  black.r=black.g=black.b=0;

#ifdef QUANT_DEBUG
  fprintf(stderr,"%d colors found, sorting and quantizing...\n",j);
#endif

  CHRONO("sort");
  if (j<numcol) ct->numcol=numcol=j;
  sort_tbl(tbl, 0, j, 0, 0, numcol, -1, ct, black, white);

#ifdef QUANT_DEBUG
  fprintf(stderr,"img_quant done, %d colors selected\n", numcol);
#endif
  CHRONO("done");

  free(tbl);
  CHRONO("really done");
  return ct;
}


struct colortable *colortable_from_array(struct array *arr,char *from)
{
  rgb_hashtbl *tbl;
  INT32 i,j;
  coltab *ct;
  rgb_group black,white;
  rgb_group *p;
  INT32 entries=0;
  struct svalue s,s2;

#ifdef QUANT_DEBUG
  fprintf(stderr,"ctfa called\n");
#endif
  CHRONO("ctfa");

  white.r=white.g=white.b=255;
  black.r=black.g=black.b=0;

  tbl=img_rehash(NULL,arr->size);
  
  s2.type=s.type=T_INT;
  for (i=0; i<arr->size; i++)
  {
     array_index(&s,arr,i);
     if (s.type!=T_ARRAY || s.u.array->size<3)
     {
	free(tbl);
	error("Illegal type in colorlist, element %d, %s\n",i,from);
     }
     array_index(&s2,s.u.array,0);
     if (s2.type!=T_INT) tbl->tbl[i].rgb.r=0; else tbl->tbl[i].rgb.r=s2.u.integer;
     array_index(&s2,s.u.array,1);
     if (s2.type!=T_INT) tbl->tbl[i].rgb.g=0; else tbl->tbl[i].rgb.g=s2.u.integer;
     array_index(&s2,s.u.array,2);
     if (s2.type!=T_INT) tbl->tbl[i].rgb.b=0; else tbl->tbl[i].rgb.b=s2.u.integer;
  }
  free_svalue(&s);
  free_svalue(&s2);

  ct = malloc(sizeof(struct colortable)+sizeof(rgb_group)*arr->size);
  if (!ct) { free(tbl); error("Out of memory.\n"); }
  MEMSET(ct,0,sizeof(struct colortable)+sizeof(rgb_group)*arr->size);
  ct->numcol=arr->size;

  CHRONO("sort");
  sort_tbl(tbl, 0, arr->size, 0, 0, arr->size, -1, ct, black, white);

#ifdef QUANT_DEBUG
  fprintf(stderr,"img_quant done, %d colors selected\n", arr->size);
#endif
  CHRONO("done");

  free(tbl);

  for (i=0; i<QUANT_SELECT_CACHE; i++)
     ct->cache[i].index=white;
  j=colortable_rgb(ct,black); /* ^^ dont find it in the cache... */
  for (i=0; i<QUANT_SELECT_CACHE; i++)
     ct->cache[i].index=black,
     ct->cache[i].value=j;

  CHRONO("really done");
  return ct;
}


#define sq(x) ((x)*(x))
#define DISTANCE(A,B) \
   (sq((A).r-(B).r)+sq((A).g-(B).g)+sq((A).b-(B).b))

int colortable_rgb(struct colortable *ct,rgb_group rgb)
{
   struct map_entry *me,**eme,**beme,*feme;
   int mindistance,best=0,i;

   if (ct->cache->index.r==rgb.r &&
       ct->cache->index.g==rgb.g &&
       ct->cache->index.b==rgb.b) 
      return ct->cache->value;

   feme=me=&(ct->map[QUANT_MAP_THIS(rgb.r)]
	            [QUANT_MAP_THIS(rgb.g)]
 	            [QUANT_MAP_THIS(rgb.b)]);

#ifdef QUANT_DEBUG_RGB
   fprintf(stderr,"%d,%d,%d -> %lu %lu %lu: ",rgb.r,rgb.g,rgb.b,
   QUANT_MAP_THIS(rgb.r),
   QUANT_MAP_THIS(rgb.g),
   QUANT_MAP_THIS(rgb.b));
   fprintf(stderr,"%lx %d,%d,%d %lu ",me,ct->clut[me->cl].r,ct->clut[me->cl].g,ct->clut[me->cl].b,me->cl);
   if (!me->used) { fprintf(stderr,"unused "); }
#endif
   if (!me->next) 
   {
#ifdef QUANT_DEBUG_RGB
      fprintf(stderr," -> %lu: %d,%d,%d\n",best,
	      ct->clut[best].r,ct->clut[best].g,ct->clut[best].b);
#endif
      return me->cl; 
   }
   for (i=1; i<QUANT_SELECT_CACHE; i++)
      if (ct->cache[i].index.r==rgb.r &&
	  ct->cache[i].index.g==rgb.g &&
	  ct->cache[i].index.b==rgb.b) 
      {
	 best=ct->cache[i].value;

	 MEMMOVE(ct->cache+1,ct->cache,
		 i*sizeof(struct rgb_cache));
	 ct->cache[0].index=rgb;
	 ct->cache[0].value=best;

#ifdef QUANT_DEBUG_RGB
fprintf(stderr,"cache: %lu: %d,%d,%d\n",best,ct->clut[best].r,ct->clut[best].g,ct->clut[best].b);
#endif
	 return best;
      }

   mindistance=DISTANCE(rgb,ct->clut[me->cl]);
   best=me->cl;
   me=me->next; eme=&(me->next); beme=NULL;
   while ( me && mindistance )
   {
      int d;
#ifdef QUANT_DEBUG_RGB
fprintf(stderr,"%lx %d,%d,%d ",me,ct->clut[me->cl].r,ct->clut[me->cl].g,ct->clut[me->cl].b);
#endif
      d=DISTANCE(rgb,ct->clut[me->cl]);
      if (d<mindistance)
      {
	 mindistance=DISTANCE(rgb,ct->clut[me->cl]);
	 best=me->cl;
	 beme=eme;
      }
      eme=&(me->next);
      me=me->next;
   }
   if (!mindistance && beme) /* exact match, place first */
   {
      struct map_entry e;
      e=*feme;
      me=*beme;
      *feme=*me;
      *beme=me->next;
      *me=e;
      feme->next=me;
   }
   else /* place in cache */
   {
      MEMMOVE(ct->cache+1,ct->cache,
	      (QUANT_SELECT_CACHE-1)*sizeof(struct rgb_cache));
      ct->cache[0].index=rgb;
      ct->cache[0].value=best;
   }
#ifdef QUANT_DEBUG_RGB
fprintf(stderr,"%lx ",me);
fprintf(stderr," -> %lu: %d,%d,%d\n",best,
	ct->clut[best].r,ct->clut[best].g,ct->clut[best].b);
#endif
   return best;
}

void colortable_free(struct colortable *ct)
{
   int r,g,b;
   for (r=0; r<QUANT_MAP_REAL; r++)
   {
      for (g=0; g<QUANT_MAP_REAL; g++)
      {
	 for (b=0; b<QUANT_MAP_REAL; b++)
	 {
	    struct map_entry *me;
	    while (me=ct->map[r][g][b].next)
	    {
	       ct->map[r][g][b].next=me->next;
	       free((char *)me);
	    }
	 }
      }
   }
   free((char *)ct);
}
