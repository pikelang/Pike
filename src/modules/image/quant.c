/* $Id: quant.c,v 1.23 1997/01/14 16:20:18 law Exp $ */

/*

quant, used by image when making gif's (mainly)

Pontus Hagland, law@infovav.se
David Kågedal, kg@infovav.se

*/

#include <unistd.h>
#include <stdio.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#include "threads.h"
#endif

#include "types.h"
#include "error.h"
#include "global.h"
#include "array.h"

#include "image.h"

/*
#define QUANT_DEBUG_GAP
#define QUANT_DEBUG_POINT
#define QUANT_DEBUG
#define QUANT_DEBUG_RGB
*/

#define QUANT_MAXIMUM_NUMBER_OF_COLORS 65535

/**********************************************************************/

#if 0
#include <sys/resource.h>
#define CHRONO(X) chrono(X)

static void chrono(char *x)
{
   struct rusage r;
   static struct rusage rold;
   getrusage(RUSAGE_SELF,&r);
   fprintf(stderr,"%s: %ld.%06ld - %ld.%06ld\n",x,
	   r.ru_utime.tv_sec,r.ru_utime.tv_usec,

	   ((r.ru_utime.tv_usec-rold.ru_utime.tv_usec<0)?-1:0)
	   +r.ru_utime.tv_sec-rold.ru_utime.tv_sec,
           ((r.ru_utime.tv_usec-rold.ru_utime.tv_usec<0)?1000000:0)
           + r.ru_utime.tv_usec-rold.ru_utime.tv_usec
	   );

   rold=r;
}
#else
#define CHRONO(X)
#endif

/**********************************************************************/

#define sq(x) ((x)*(x))
#define DISTANCE(A,B) \
   (sq(((int)(A).r)-((int)(B).r)) \
    +sq(((int)(A).g)-((int)(B).g)) \
    +sq(((int)(A).b)-((int)(B).b)))


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

static INLINE int hash_enter_strip(rgb_entry *rgbe,rgb_entry *end,
				   int len,rgb_group rgb,int strip)
{
   register rgb_entry *re;

   unsigned char strip_r[24]=
{ 0xff, 0xfe, 0xfe, 0xfe, 0xfc, 0xfc, 0xfc, 0xf8, 0xf8, 0xf8, 
  0xf0, 0xf0, 0xf0, 0xe0, 0xe0, 0xe0, 0xc0, 0xc0, 0xc0, 0x80, 0x80, 0x80 };
   unsigned char strip_g[24]=
{ 0xff, 0xff, 0xfe, 0xfe, 0xfe, 0xfc, 0xfc, 0xfc, 0xf8, 0xf8, 0xf8, 
  0xf0, 0xf0, 0xf0, 0xe0, 0xe0, 0xe0, 0xc0, 0xc0, 0xc0, 0x80, 0x80, 0x80 };
   unsigned char strip_b[24]=
{ 0xfe, 0xfe, 0xfe, 0xfc, 0xfc, 0xfc, 0xf8, 0xf8, 0xf8, 
  0xf0, 0xf0, 0xf0, 0xe0, 0xe0, 0xe0, 0xc0, 0xc0, 0xc0, 0x80, 0x80, 0x80 };

   rgb.r&=strip_r[strip];
   rgb.g&=strip_g[strip];
   rgb.b&=strip_b[strip];

   re=rgbe+hash(rgb,len);

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
#ifdef QUANT_DEBUG_POINT
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
#ifdef QUANT_DEBUG_POINT
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
		     struct colortable *ct, rgb_group lower,rgb_group upper,
		     unsigned long **rn_next,
		     unsigned long *rgb_node)
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

#endif

   if (len>1)
   {
   /* check which direction has the most span */
   /* we make it easy for us, only three directions: r,g,b */


#define PRIO_RED   2
#define PRIO_GREEN 4
#define PRIO_BLUE  1

#if 0
      rgb_group min,max;

      max.r=min.r=tbl[start].rgb.r;
      max.g=min.g=tbl[start].rgb.g;
      max.b=min.b=tbl[start].rgb.b;

      for (x=1; x<len; x++)
      {
	 if (min.r>tbl[start+x].rgb.r) min.r=tbl[start+x].rgb.r;
	 if (min.g>tbl[start+x].rgb.g) min.g=tbl[start+x].rgb.g;
	 if (min.b>tbl[start+x].rgb.b) min.b=tbl[start+x].rgb.b;
	 if (max.r<tbl[start+x].rgb.r) max.r=tbl[start+x].rgb.r;
	 if (max.g<tbl[start+x].rgb.g) max.g=tbl[start+x].rgb.g;
	 if (max.b<tbl[start+x].rgb.b) max.b=tbl[start+x].rgb.b;
      }
#ifdef QUANT_DEBUG
fprintf(stderr,"space: %d,%d,%d-%d,%d,%d  ",
	min.r,min.g,min.b,
	max.r,max.g,max.b);
#endif

      /* do this weighted, red=2 green=3 blue=1 */
      if ((max.r-min.r)*PRIO_RED>(max.g-min.g)*PRIO_GREEN) /* r>g */
	 if ((max.r-min.r)*PRIO_RED>(max.b-min.b)*PRIO_BLUE) /* r>g, r>b */
	    dir=0;
         else /* r>g, b>r */
	    dir=2;
      else
	 if ((max.g-min.g)*PRIO_GREEN>(max.b-min.b)*PRIO_BLUE) /* g>r, g>b */
	    dir=1;
         else /* g>r, b>g */
	    dir=2;
#endif

      rgbl_group sum={0,0,0};
      rgb_group mid;

      for (x=0; x<len; x++)
      {
	 sum.r+=tbl[start+x].rgb.r;
	 sum.g+=tbl[start+x].rgb.g;
	 sum.b+=tbl[start+x].rgb.b;
      }
      mid.r=(unsigned char)(sum.r/len);
      mid.g=(unsigned char)(sum.g/len);
      mid.b=(unsigned char)(sum.b/len);

      sum.r=sum.g=sum.b=0;
      for (x=0; x<len; x++)
      {
	 sum.r+=sq(tbl[start+x].rgb.r-mid.r);
	 sum.g+=sq(tbl[start+x].rgb.g-mid.g);
	 sum.b+=sq(tbl[start+x].rgb.b-mid.b);
      }

      if (sum.r*PRIO_RED>sum.g*PRIO_GREEN) /* r>g */
	 if (sum.r*PRIO_RED>sum.b*PRIO_BLUE) /* r>g, r>b */
	    dir=0;
         else /* r>g, b>r */
	    dir=2;
      else
	 if (sum.g*PRIO_GREEN>sum.b*PRIO_BLUE) /* g>r, g>b */
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

   if (len>1 && gap>1) /* recurse */
   {
      unsigned long pos,g1,g2;
      rgb_group less,more,rgb;
      unsigned char split_on=0;
      int i;

      pos=get_tbl_median(tbl+start,len);

      rgb=tbl[start+pos].rgb;

      less=upper;
      more=lower;

      switch (dir)
      {
	 case 0: more.r=rgb.r+1; split_on=less.r=rgb.r;
                 while (pos<len-1 && tbl[start].rgb.r==tbl[start+pos].rgb.r) pos++;
                 while (pos>0 && tbl[start+len-1].rgb.r==tbl[start+pos].rgb.r) pos--;
                 break;
	 case 1: more.g=rgb.g+1; split_on=less.g=rgb.g;
                 while (pos<len-1 && tbl[start].rgb.g==tbl[start+pos].rgb.g) pos++;
                 while (pos>0 && tbl[start+len-1].rgb.g==tbl[start+pos].rgb.g) pos--;
                 break;
	 case 2: more.b=rgb.b+1; split_on=less.b=rgb.b;
                 while (pos<len-1 && tbl[start].rgb.b==tbl[start+pos].rgb.b) pos++;
                 while (pos>0 && tbl[start+len-1].rgb.b==tbl[start+pos].rgb.b) pos--;
                 break;
      }

#ifdef QUANT_DEBUG
      fprintf(stderr, " pos=%lu len=%lu\n",pos,len);
#endif

#ifdef QUANT_DEBUG_GAP

      fprintf(stderr,"\n left: ");
      for (i=0; i<=pos; i++) 
	 fprintf(stderr,"%d,%d,%d; ",
		 tbl[start+i].rgb.r,tbl[start+i].rgb.g,tbl[start+i].rgb.b);
      fprintf(stderr,"\n right: ");
      for (; i<len; i++) 
	 fprintf(stderr,"%d,%d,%d; ",
		 tbl[start+i].rgb.r,tbl[start+i].rgb.g,tbl[start+i].rgb.b);
      fprintf(stderr,"\n");

#endif		
      g1=gap>>1; 

#ifdef QUANT_DEBUG_GAP
      fprintf(stderr,"gap: %d / %d pos+1=%d len-pos-1=%d gap-g1=%d\n",
	      g1,gap-g1,pos+1,len-pos-1,gap-g1);
#endif

      if (pos+1<g1) g1=pos+1,g2=gap-g1;
      else if (len-pos-1<gap-g1) g2=(len-pos-1),g1=gap-g2;
      else g2=gap-g1;

#ifdef QUANT_DEBUG
      fprintf(stderr,"gap: %d / %d  ",g1,g2);
#endif

      if (gap>1)
      {
	 /* split tree */

	 *rgb_node=
	    ( (*rn_next)-ct->rgb_node )
	    | ( ((unsigned long)split_on) << 22 )
	    | ( (dir+1)<<30 ) & 0xffffffff;
	 rgb_node=*rn_next;
	 (*rn_next)+=2;

	 sort_tbl(ht,start,pos+1,
		  level+1,idx,g1,dir,
		  ct,lower,less,rn_next,rgb_node++);
	 
	 sort_tbl(ht,start+pos+1,len-pos-1,
		  level+1,idx+g1,g2,dir,
		  ct,more,upper,rn_next,rgb_node);
      }
      else
      {
	 /* this shouldn't occur? /law */
	 abort();
	 sort_tbl(ht,start,pos+1,
		  level+1,idx,g1,dir,
		  ct,lower,less,rn_next,rgb_node);
      }
      return;
   }
   else 
   {
      int r,g,b;
      if (len>1)
	 ct->clut[idx]=get_tbl_point(tbl+start,len);
      else
	 ct->clut[idx]=tbl[start].rgb;

#ifdef QUANT_DEBUG
      fprintf(stderr,"-> end node [%d,%d,%d] - [%d,%d,%d] => %d,%d,%d\n",
	      lower.r, lower.g, lower.b, upper.r, upper.g, upper.b,
	      ct->clut[idx].r,ct->clut[idx].g,ct->clut[idx].b);
#endif      
      
      /* write end node */
      *rgb_node=idx; /* done */
   }
}

static struct colortable *colortable_allocate(int numcol)
{
   struct colortable *ct;
   ct = malloc(sizeof(struct colortable)+
	       sizeof(rgb_group)*numcol);
   if (!ct) error("Out of memory.\n");
   MEMSET(ct,0,sizeof(struct colortable)+
	       sizeof(rgb_group)*numcol);
   ct->numcol=numcol;
   ct->rgb_node=malloc(sizeof(unsigned long)*numcol*4);
   MEMSET(ct->rgb_node,0,
	  sizeof(unsigned long)*numcol*4);
   return ct;
}

struct colortable *colortable_quant(struct image *img,int numcol)
{
   rgb_hashtbl *tbl;
   INT32 i,j;
   INT32 sz = img->xsize * img->ysize;
   rgb_entry entry;
   struct colortable *ct=NULL;
   rgb_group black,white;
   rgb_group *p;
   INT32 entries=0;
   unsigned long *next_free_rgb_node;

   THREADS_ALLOW();
#ifdef QUANT_DEBUG
   fprintf(stderr,"img_quant called\n");
#endif
   CHRONO("quant");

   if (numcol<2) numcol=2;

   if (numcol>MAX_NUMCOL) numcol=MAX_NUMCOL; 

   ct = colortable_allocate(numcol);

#ifdef QUANT_DEBUG
   fprintf(stderr,"Moving colors into hashtable\n");
#endif

   for (;;)
   {
      int strip=0;
rerun:
      entries=0;


      CHRONO("hash init");
      
      tbl = img_rehash(NULL, 8192);
      
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

	 if (!strip)
	 {
	    while (i--)
	       if ( (entries+=hash_enter(rgbe,end,len,*(p++))) > trig )
	       {
#ifdef QUANT_DEBUG
		  fprintf(stderr,"rehash: 20%% = %d / %d...\n",entries,len);
#endif
		  CHRONO("rehash...");
		  if ((len<<2) > QUANT_MAXIMUM_NUMBER_OF_COLORS)
		  {
		     strip++;
		     fprintf(stderr,"strip: %d\n",strip);
		     free(tbl);
		     goto rerun;
		  }
		  tbl=img_rehash(tbl,len<<2); /* multiple size by 4 */
		  break;
	       }
	 }
	 else
	    while (i--)
	       if ( (entries+=hash_enter_strip(rgbe,end,len,*(p++),strip)) > trig )
	       {
#ifdef QUANT_DEBUG
		  fprintf(stderr,"rehash: 20%% = %d / %d...\n",entries,len);
#endif
		  CHRONO("rehash...");
		  if ((len<<2) > QUANT_MAXIMUM_NUMBER_OF_COLORS)
		  {
		     strip++;
		     fprintf(stderr,"strip: %d\n",strip);
		     free(tbl);
		     goto rerun;
		  }
		  tbl=img_rehash(tbl,len<<2); /* multiple size by 4 */
		  break;
	       }
      }
      while (i>=0);
      break;
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

   next_free_rgb_node=ct->rgb_node+1;
   sort_tbl(tbl, 0, j, 0, 0, numcol, -1, ct, black, white,
	    &next_free_rgb_node,ct->rgb_node);

#ifdef QUANT_DEBUG
   fprintf(stderr,"img_quant done, %d colors selected\n", numcol);
#endif
   CHRONO("done");

   free(tbl);
   CHRONO("really done");
   THREADS_DISALLOW();
   return ct;
}


struct colortable *colortable_from_array(struct array *arr,char *from)
{
   rgb_hashtbl *tbl;
   INT32 i,j;
   struct colortable *ct=NULL;
   rgb_group black,white;
   rgb_group *p;
   INT32 entries=0;
   struct svalue s,s2;
   unsigned long *next_free_rgb_node;

   THREADS_ALLOW();
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
      tbl->tbl[i].count=1;
   }
   free_svalue(&s);
   free_svalue(&s2);

   ct = colortable_allocate(arr->size);

   CHRONO("sort");
   next_free_rgb_node=ct->rgb_node+1;
   sort_tbl(tbl, 0, arr->size, 0, 0, arr->size, -1, ct, black, white,
	    &next_free_rgb_node,ct->rgb_node);

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
   THREADS_DISALLOW();
   return ct;
}


int colortable_rgb(struct colortable *ct,rgb_group rgb)
{
   int i,best;

   if (ct->cache->index.r==rgb.r &&
       ct->cache->index.g==rgb.g &&
       ct->cache->index.b==rgb.b) 
      return ct->cache->value;

#ifdef QUANT_DEBUG_RGB
fprintf(stderr,"rgb: %d,%d,%d\n",rgb.r,rgb.g,rgb.b);
#endif

#if QUANT_SELECT_CACHE>1
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
#endif

   /* find node */

#if 1

   do 
   {
      rgb_group min={0,0,0},max={255,255,255};
      unsigned long *rn;
      unsigned char split;

      rn=ct->rgb_node;

      for (;;)
      {
#ifdef QUANT_DEBUG_RGB
      fprintf(stderr,"-> %d: %c%d %d\n",
	      rn-ct->rgb_node,
	      (((*rn)>>30)==0)?'c':
	      (((*rn)>>30)==1)?'r':
	      (((*rn)>>30)==2)?'g':'b',
	      ((*rn)>>22) & 255,
	      ((*rn)&4194303));
#endif

	 switch ((*rn)>>30)
	 {
	    case 0: /* end node */ break;
	    case 1: /* red */
	       split=(unsigned char)( ((*rn)>>22) & 255 );
	       rn=ct->rgb_node+((*rn)&4194303);
	       if (rgb.r<=split) max.r=split;
	       else rn++,min.r=split+1;
	       continue;
	    case 2: /* green */
	       split=(unsigned char)( ((*rn)>>22) & 255 );
	       rn=ct->rgb_node+((*rn)&4194303);
	       if (rgb.g<=split) max.g=split;
	       else rn++,min.g=split+1;
	       continue;
	    case 3: /* blue */
	       split=(unsigned char)( ((*rn)>>22) & 255 );
	       rn=ct->rgb_node+((*rn)&4194303);
	       if (rgb.b<=split) max.b=split;
	       else rn++,min.b=split+1;
	       continue;
	 }
	 break;
      }
      best=*rn;
   } 
   while (0);

#endif

   /* place in cache */
#if QUANT_SELECT_CACHE>1
   MEMMOVE(ct->cache+1,ct->cache,
	   (QUANT_SELECT_CACHE-1)*sizeof(struct rgb_cache));
#endif
   ct->cache[0].index=rgb;
   ct->cache[0].value=best;

#ifdef QUANT_DEBUG_RGB
fprintf(stderr," -> %lu: %d,%d,%d\n",best,
	ct->clut[best].r,ct->clut[best].g,ct->clut[best].b);
#endif
   return best;
}

int colortable_rgb_nearest(struct colortable *ct,rgb_group rgb)
{
   int i,best=0,di,di2;
   rgb_group *prgb;

   if (ct->cache->index.r==rgb.r &&
       ct->cache->index.g==rgb.g &&
       ct->cache->index.b==rgb.b) 
      return ct->cache->value;

#ifdef QUANT_DEBUG_RGB
fprintf(stderr,"rgb: %d,%d,%d\n",rgb.r,rgb.g,rgb.b);
#endif

#if QUANT_SELECT_CACHE>1
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
#endif

   /* find node */

   di=1000000L;
   for (i=0; i<ct->numcol; i++)
   {
      prgb=ct->clut+i;
      if ((di2=DISTANCE(*prgb,rgb))<di) 
      { 
	 best=i; 
	 di=di2;
      }
   }

   /* place in cache */
#if QUANT_SELECT_CACHE>1
   MEMMOVE(ct->cache+1,ct->cache,
	   (QUANT_SELECT_CACHE-1)*sizeof(struct rgb_cache));
#endif
   ct->cache[0].index=rgb;
   ct->cache[0].value=best;

#ifdef QUANT_DEBUG_RGB
fprintf(stderr," -> %lu: %d,%d,%d\n",best,
	ct->clut[best].r,ct->clut[best].g,ct->clut[best].b);
#endif
   return best;
}

void colortable_free(struct colortable *ct)
{
   int r,g,b;
   free((char *)ct->rgb_node);
   free((char *)ct);
}
