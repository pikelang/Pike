#include <unistd.h>
#include <stdio.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "types.h"
#include "image.h"
#include "error.h"
#include "global.h"


/**********************************************************************/

/*#define QUANT_DEBUG*/

typedef struct
{
  rgb_group rgb;
  int count;
} rgb_entry;

typedef struct
{
  int len, entries;
  rgb_entry tbl[1];
} rgb_hashtbl;

typedef struct colortable coltab;

static int hash(rgb_entry *entry, int len)
{
   return abs((23*entry->rgb.r + 21*entry->rgb.g + 17*entry->rgb.b)  % len);
}

static int same_col(rgb_entry *entry1, rgb_entry *entry2)
{
   return ((entry1->rgb.r == entry2->rgb.r) &&
	   (entry1->rgb.g == entry2->rgb.g) &&
	   (entry1->rgb.b == entry2->rgb.b));
}


static void hash_enter(rgb_hashtbl *tbl, rgb_entry *entry)
{
   int i = hash(entry, tbl->len);
   int inc = 1;

#if 0
/*    fprintf(stderr,"%d.", entry->count); */
/*  fprintf(stderr,"hash_enter: %3d.%3d.%3d count = %d, hash = %d ",
	  entry->rgb.r, entry->rgb.b, entry->rgb.b, entry->count, i);*/
#endif

   while (tbl->tbl[i].count &&
	  (entry->count || !same_col(entry, &tbl->tbl[i])))
   {
#ifdef QUANT_DEBUG
   fprintf(stderr,".");
#endif
      i = (i + inc++) % tbl->len;
   }

   if (tbl->tbl[i].count == 0)
   {
#if 0
      fprintf(stderr,"new");
#endif
      tbl->tbl[i] = *entry;
      if (tbl->tbl[i].count == 0)
	 tbl->tbl[i].count = 1;
      tbl->entries++;
   } else
      tbl->tbl[i].count++;

#if 0
   fprintf(stderr," (%d,%d)\n", tbl->tbl[i].count, tbl->entries);
#endif
}

static rgb_hashtbl *img_rehash(rgb_hashtbl *old, int newsize)
{
  int i;
  rgb_hashtbl *new = malloc(sizeof(rgb_hashtbl) + 
			    sizeof(rgb_entry) * ( newsize - 1 ) );
  MEMSET(new->tbl, 0, sizeof(rgb_entry) * newsize );
  
  if (!new) error("Out of memory\n");
#ifdef QUANT_DEBUG
  fprintf(stderr,"img_rehash: old size = %d, new size = %d\n",
	  (old ? old->len : 0), newsize);
#endif
  
  new->len = newsize;
  new->entries = 0;
  if (old)
  {
     for (i=0;i<old->len;i++)
     {
	if (old->tbl[i].count > 0)
	   hash_enter(new,&old->tbl[i]);
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
   return len/2;
}

static rgb_group get_tbl_point(rgb_entry *tbl,int len)
{
   return tbl[len/2].rgb;
}

static void sort_tbl(rgb_hashtbl *ht, int start, int len,
		     int level, int idx, coltab *ct,
		     rgb_group lower,rgb_group upper)
{
   rgb_entry *tbl = ht->tbl;


#ifdef QUANT_DEBUG

   int x,y;

   fprintf(stderr,"%*ssort_tbl: level %d  start = %d  len=%d, idx = %d",
	   level, "",
	   level, start, len, idx);
   fprintf(stderr,"%*s\n%d,%d,%d-%d,%d,%d ",level,"",
	   lower.r,lower.g,lower.b,upper.r,upper.g,upper.b);

   fprintf(stderr,"[%d,%d,%d] - [%d,%d,%d]\n",
	      QUANT_MAP_THIS(lower.r), 
	      QUANT_MAP_THIS(lower.g), 
	      QUANT_MAP_THIS(lower.b), 
	      QUANT_MAP_THIS(upper.r), 
	      QUANT_MAP_THIS(upper.g), 
	      QUANT_MAP_THIS(upper.b));

#endif
   
   switch (level%3)
   {
      case 0: qsort(tbl+start, len, sizeof(rgb_entry),
		    (int (*)(const void *, const void *))cmp_red); break;
      case 1: qsort(tbl+start, len, sizeof(rgb_entry),
		    (int (*)(const void *, const void *))cmp_green); break;
      case 2: qsort(tbl+start, len, sizeof(rgb_entry),
		    (int (*)(const void *, const void *))cmp_blue); break;
   }

   if (level < 8)
   {
      int pos;
      rgb_group less,more,rgb;
#ifdef QUANT_DEBUG
      fprintf(stderr, "\n");
#endif		
      pos=get_tbl_median(tbl+start,len);
      rgb=tbl[start+pos].rgb;

      less=upper;
      more=lower;

      switch (level%3)
      {
	 case 0: more.r=rgb.r+1; less.r=rgb.r;break;
	 case 1: more.g=rgb.g+1; less.g=rgb.g;break;
	 case 2: more.b=rgb.b+1; less.b=rgb.b;break;
      }
      
      sort_tbl(ht,start,pos,
	       level+1,idx,ct,
	       lower,less);

      sort_tbl(ht,start+pos,len-pos,
	       level+1,idx+(128>>level),ct,
	       more,upper);
   }
   else
   {
      int r,g,b;
      ct->clut[idx]=get_tbl_point(tbl+start,len);
#ifdef QUANT_DEBUG
      fprintf(stderr,"\n [%d,%d,%d] - [%d,%d,%d] %d\n",
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
      for (r = QUANT_MAP_THIS(lower.r); 
	   r <= QUANT_MAP_THIS(upper.r); r++)
	 for (g = QUANT_MAP_THIS(lower.g); 
	      g <= QUANT_MAP_THIS(upper.g); g++)
	    for (b = QUANT_MAP_THIS(lower.b); 
		 b <= QUANT_MAP_THIS(upper.b); b++)
	    {
#ifdef QUANT_DEBUG
	       fprintf(stderr,"[%d,%d,%d] = %d (%d,%d,%d)\n",r,g,b,idx,ct->clut[idx].r,ct->clut[idx].g,ct->clut[idx].b);
#endif
	       if (ct->map[r][g][b].used)
	       {
		  struct map_entry *me;
		  me=malloc(sizeof(struct map_entry));
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


struct colortable *colortable_quant(struct image *img)
{
   rgb_hashtbl *tbl;
   INT32 i,j;
   INT32 sz = img->xsize * img->ysize;
   rgb_entry entry;
   coltab *ct;
   rgb_group black,white;

#ifdef QUANT_DEBUG
   fprintf(stderr,"img_quant called\n");
#endif

   ct = malloc(sizeof(coltab));

   MEMSET(ct,0,sizeof(coltab));

   if (!ct) error("Out of memory.\n");

   tbl = img_rehash(NULL, 8192 /*(img->xsize*img->ysize) / 6*/);
   for (i=0;i<sz;i++)
   {
      entry.rgb = img->img[i];
      entry.count = 0;
      hash_enter(tbl, &entry);
      if (tbl->entries > tbl->len * 3 / 4)
	 tbl = img_rehash(tbl, tbl->len * 2);
   }

   /* Compact the hash table */

#ifdef QUANT_DEBUG
   fprintf(stderr,"Compacting\n");
#endif
   i = tbl->len - 1;
   j = 0;
   while (i > tbl->entries)
   {
      while ((i >= tbl->entries) && tbl->tbl[i].count == 0) i--;
      while ((j <  tbl->entries) && tbl->tbl[j].count != 0) j++;
      if (j<i)
      {
	 tbl->tbl[j] = tbl->tbl[i];
	 tbl->tbl[i].count = 0;
      }
   }
   tbl->len = tbl->entries;

   white.r=white.g=white.b=255;
   black.r=black.g=black.b=0;

   sort_tbl(tbl, 0, tbl->len, 0, 0, ct, black, white);

#ifdef QUANT_DEBUG
   fprintf(stderr,"img_quant done, %d colors found\n", tbl->entries);
#endif

   free(tbl);
   return ct;
}

#define sq(x) ((x)*(x))
#define DISTANCE(A,B) \
   (sq((A).r-(B).r)+sq((A).g-(B).g)+sq((A).b-(B).b))

int colortable_rgb(struct colortable *ct,rgb_group rgb)
{
   struct map_entry *me;
   int mindistance,best;
   me=&(ct->map[QUANT_MAP_THIS(rgb.r)]
	       [QUANT_MAP_THIS(rgb.g)]
               [QUANT_MAP_THIS(rgb.b)]);
#ifdef QUANT_DEBUG
   fprintf(stderr,"%d,%d,%d -> %d %d %d: ",rgb.r,rgb.g,rgb.b,
   QUANT_MAP_THIS(rgb.r),
   QUANT_MAP_THIS(rgb.g),
   QUANT_MAP_THIS(rgb.b));
   fprintf(stderr,"%lx %d,%d,%d %d ",me,ct->clut[me->cl].r,ct->clut[me->cl].g,ct->clut[me->cl].b,me->cl);
   if (!me->used) { fprintf(stderr,"unused "); }
#endif
   if (!me->next) return me->cl; 
   mindistance=DISTANCE(rgb,ct->clut[me->cl]);
   best=me->cl;
   while ( (me=me->next) && mindistance)
   {
      int d;
#ifdef QUANT_DEBUG
fprintf(stderr,"%lx %d,%d,%d ",me,ct->clut[me->cl].r,ct->clut[me->cl].g,ct->clut[me->cl].b);
#endif
      d=DISTANCE(rgb,ct->clut[me->cl]);
      if (d<mindistance)
      {
	 mindistance=DISTANCE(rgb,ct->clut[me->cl]);
	 best=me->cl;
      }
   }
#ifdef QUANT_DEBUG
fprintf(stderr,"%lx\n",me);
#endif
   return best;
}

void colortable_free(struct colortable *ct)
{
   int r,g,b;
   for (r=0; r<QUANT_MAP_REAL; r++)
      for (g=0; g<QUANT_MAP_REAL; g++)
	 for (b=0; b<QUANT_MAP_REAL; b++)
	 {
	    struct map_entry *me;
	    if ( (me=ct->map[r][g][b].next) )
	    {
	       ct->map[r][g][b].next=me->next;
	       free(me);
	    }
	 }
   free(ct);
}
