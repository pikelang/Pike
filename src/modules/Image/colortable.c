/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: colortable.c,v 1.121 2004/09/19 00:19:01 nilsson Exp $
*/

#include "global.h"

/*
**! module Image
**! class Colortable
**!
**!	This object keeps colortable information,
**!	mostly for image re-coloring (quantization).
**!
**!	The object has color reduction, quantisation,
**!	mapping and dithering capabilities.
**!
**! see also: Image, Image.Image, Image.Font, Image.GIF
*/

/* #define COLORTABLE_DEBUG */
/* #define COLORTABLE_REDUCE_DEBUG */
/* #define CUBICLE_DEBUG */

RCSID("$Id: colortable.c,v 1.121 2004/09/19 00:19:01 nilsson Exp $");

#include <math.h> /* fabs() */

#include "image_machine.h"

#include "stralloc.h"
#include "pike_macros.h"
#include "object.h"
#include "interpret.h"
#include "svalue.h"
#include "mapping.h"
#include "builtin_functions.h"
#include "pike_error.h"
#include "module_support.h"
#include "operators.h"
#include "dmalloc.h"
#include "bignum.h"

#include "image.h"
#include "colortable.h"
#include "initstuff.h"


#define sp Pike_sp

#define WEIGHT_NEEDED (nct_weight_t)(0x10000000)
#define WEIGHT_REMOVE (nct_weight_t)(0x10000001)

#define COLORLOOKUPCACHEHASHR 7
#define COLORLOOKUPCACHEHASHG 17
#define COLORLOOKUPCACHEHASHB 1
#define COLORLOOKUPCACHEHASHVALUE(r,g,b) \
               (((COLORLOOKUPCACHEHASHR*(int)(r))+ \
		 (COLORLOOKUPCACHEHASHG*(int)(g))+ \
		 (COLORLOOKUPCACHEHASHB*(int)(b)))% \
		COLORLOOKUPCACHEHASHSIZE) 

#define SPACEFACTOR_R 3
#define SPACEFACTOR_G 4
#define SPACEFACTOR_B 1

#define CUBICLE_DEFAULT_R 10
#define CUBICLE_DEFAULT_G 10
#define CUBICLE_DEFAULT_B 10
#define CUBICLE_DEFAULT_ACCUR 4

#define RIGID_DEFAULT_R 16
#define RIGID_DEFAULT_G 16
#define RIGID_DEFAULT_B 16

#define SQ(x) ((x)*(x))
static INLINE ptrdiff_t sq(ptrdiff_t x) { return x*x; }
static INLINE int sq_int(int x) { return x*x; }

#ifdef THIS
#undef THIS /* Needed for NT */
#endif
#define THIS ((struct neo_colortable *)(Pike_fp->current_storage))
#define THISOBJ (Pike_fp->current_object)

static struct pike_string *s_array;
static struct pike_string *s_string;
static struct pike_string *s_mapping;

/***************** init & exit *********************************/


static void colortable_free_lookup_stuff(struct neo_colortable *nct)
{
   switch (nct->lookup_mode)
   {
      case NCT_CUBICLES: 
         if (nct->lu.cubicles.cubicles)
	 {
	    int i=nct->lu.cubicles.r*nct->lu.cubicles.g*nct->lu.cubicles.b;
	    while (i--) 
	       if (nct->lu.cubicles.cubicles[i].index)
		  free(nct->lu.cubicles.cubicles[i].index);
	    free(nct->lu.cubicles.cubicles);
	 }
	 nct->lu.cubicles.cubicles=NULL;
	 break;
      case NCT_RIGID:
	 if (nct->lu.rigid.index)
	    free(nct->lu.rigid.index);
	 nct->lu.rigid.index=NULL;
	 break;
      case NCT_FULL:
         break;
   }
}

static void colortable_free_dither_union(struct neo_colortable *nct)
{
  switch (nct->dither_type)
  {
    case NCTD_ORDERED:
      free(nct->du.ordered.rdiff);
      free(nct->du.ordered.gdiff);
      free(nct->du.ordered.bdiff);
      break;
    default:
      ;
  }
}

static void free_colortable_struct(struct neo_colortable *nct)
{
   struct nct_scale *s;
   colortable_free_lookup_stuff(nct);
   switch (nct->type)
   {
      case NCT_NONE:
         break; /* done */
      case NCT_FLAT:
         free(nct->u.flat.entries);
	 nct->type=NCT_NONE;
	 break; /* done */
      case NCT_CUBE:
         while ((s=nct->u.cube.firstscale))
	 {
	    nct->u.cube.firstscale=s->next;
	    free(s);
	 }
	 nct->type=NCT_NONE;
         break; /* done */
   }

   colortable_free_dither_union(nct);
}

static void colortable_init_stuff(struct neo_colortable *nct)
{
   int i;
   nct->type=NCT_NONE;
   nct->lookup_mode=NCT_CUBICLES;
   nct->lu.cubicles.cubicles=NULL;

   nct->spacefactor.r=SPACEFACTOR_R;
   nct->spacefactor.g=SPACEFACTOR_G;
   nct->spacefactor.b=SPACEFACTOR_B;
   
   nct->lu.cubicles.r=CUBICLE_DEFAULT_R;
   nct->lu.cubicles.g=CUBICLE_DEFAULT_G;
   nct->lu.cubicles.b=CUBICLE_DEFAULT_B;
   nct->lu.cubicles.accur=CUBICLE_DEFAULT_ACCUR;

   for (i=0; i<COLORLOOKUPCACHEHASHSIZE; i++)
      nct->lookupcachehash[i].index=-1;

   nct->dither_type=NCTD_NONE;
}

static void init_colortable_struct(struct object *obj)
{
   colortable_init_stuff(THIS);
}

static void exit_colortable_struct(struct object *obj)
{
   free_colortable_struct(THIS);
}

/***************** internal stuff ******************************/

#if 0
#include <sys/resource.h>
#define CHRONO(X) chrono(X);

static void chrono(char *x)
{
  static long long first;
  static long long last;
  long long cur;
  if(!first)
    last = first = gethrtime();
  cur=gethrtime();
  fprintf(stderr, "CHRONO %-20s ... %4.4f (%4.4f)\n",x,(cur-last)/1000000000.0,(cur-first)/1000000000.0);
  last=cur;
}
#else
#define CHRONO(X)
#endif

static void _img_copy_colortable(struct neo_colortable *dest,
				 struct neo_colortable *src)
{
   struct nct_scale *s,*d,**np;
   int i;

   /* copy/clear lookup info */

   for (i=0; i<COLORLOOKUPCACHEHASHSIZE; i++)
      dest->lookupcachehash[i].index=-1;

   dest->lookup_mode=src->lookup_mode;
   switch (dest->lookup_mode)
   {
      case NCT_CUBICLES: dest->lu.cubicles.cubicles=NULL; break;
      case NCT_RIGID:    dest->lu.rigid.index=NULL; break;
      case NCT_FULL:     break;
   }
   
   /* copy dither info */
   dest->dither_type=src->dither_type;
   dest->du=src->du; 

   switch (src->type)
   {
      case NCT_NONE:
         dest->type=NCT_NONE;
         return; /* done */
      case NCT_FLAT:
         dest->type=NCT_NONE; /* don't free anything if xalloc throws */
         dest->u.flat.entries=(struct nct_flat_entry*)
	    xalloc(src->u.flat.numentries*sizeof(struct nct_flat_entry));
	 memcpy(dest->u.flat.entries,src->u.flat.entries,
		src->u.flat.numentries*sizeof(struct nct_flat_entry));
	 dest->u.flat.numentries=src->u.flat.numentries;
	 dest->type=NCT_FLAT;
	 return; /* done */
      case NCT_CUBE:
         *dest=*src;
	 dest->u.cube.firstscale=NULL;
	 np=&(dest->u.cube.firstscale);
	 s=src->u.cube.firstscale;
	 while (s)
	 {
	    d=(struct nct_scale*)
	       xalloc(sizeof(struct nct_scale)+s->steps*sizeof(int));
	    memcpy(d,s,sizeof(struct nct_scale)+s->steps*sizeof(int));
	    d->next=NULL;
	    *np=d;
	    np=&(d->next); /* don't change order */
	    s=s->next;
	 }
         return; /* done */
   }
}


#ifdef COLORTABLE_DEBUG
static void stderr_print_entries(struct nct_flat_entry *src,int n)
{
   int i;

   for (i=0; i<n; i++)
    if (src[i].weight==WEIGHT_NEEDED)
       fprintf(stderr,"#%02x%02x%02x  nd %3d ",src[i].color.r,src[i].color.g,src[i].color.b,src[i].no);
    else 
       fprintf(stderr,"#%02x%02x%02x %3d %3d ",src[i].color.r,src[i].color.g,src[i].color.b,src[i].weight,src[i].no);
   fprintf(stderr,"\n");
}
#endif

/*** reduction ***/

#define DIFF_R_MULT 3
#define DIFF_G_MULT 4
#define DIFF_B_MULT 3
#define DIFF_GREY_MULT 4 /* should be bigger or equal or max of above */

#define POSITION_DONT (-1.0)

enum nct_reduce_method { NCT_REDUCE_MEAN, NCT_REDUCE_WEIGHT };

static ptrdiff_t reduce_recurse(struct nct_flat_entry *src,
				struct nct_flat_entry *dest,
				ptrdiff_t src_size,
				ptrdiff_t target_size,
				int level,
				rgbl_group sf,
				rgbd_group position,rgbd_group space,
				enum nct_reduce_method type)
{
   ptrdiff_t n, i, m;
   rgbl_group sum={0,0,0},diff={0,0,0};
   rgbl_group min={256,256,256},max={0,0,0};
   nct_weight_t mmul,tot=0;
   INT32 g, gdiff=0;
   ptrdiff_t left, right;
   enum { SORT_R,SORT_G,SORT_B,SORT_GREY } st;
   rgbd_group newpos1,newpos2;

#ifdef COLORTABLE_REDUCE_DEBUG
   fprintf(stderr, "COLORTABLE%*s reduce_recurse %lx,%lx, %ld,%ld\n",
	   level, "", (unsigned long)src, (unsigned long)dest,
	   (long)src_size, (long)target_size);
#if 0
   stderr_print_entries(src,src_size);
#endif
#endif

   if (!src_size) return 0; 
      /* can't go figure some good colors just like that... */
   
   if (src_size==1)
   {
      *dest=*src;
      dest->no=-1;
      return 1;
   }

   if (target_size<=1)
   {
      for (i=n=0; i<src_size; i++)
	 if (src[i].weight==WEIGHT_NEEDED)
	 {
	    *dest=src[i];
	    dest++;
	    n++;
	 }

      if (n>=target_size) return n;

      switch (type)
      {
	 case NCT_REDUCE_MEAN:
	    /* get the mean color */

 	    if (src_size > 10240) {
	       mmul = 10240;
	    } else {
	       mmul = DO_NOT_WARN((nct_weight_t)src_size);
	    }
	    
	    for (i=0; i<src_size; i++)
	    {
	       nct_weight_t mul = src[i].weight;
	       
	       sum.r += src[i].color.r*mul;
	       sum.g += src[i].color.g*mul;
	       sum.b += src[i].color.b*mul;
	       tot += mul;
	    }
	    
	    dest->color.r = sum.r/tot;
	    dest->color.g = sum.g/tot;
	    dest->color.b = sum.b/tot;
	    dest->weight = tot;
	    dest->no = -1;

#ifdef COLORTABLE_REDUCE_DEBUG
	    fprintf(stderr, "COLORTABLE%*s sum=%d,%d,%d tot=%ld\n",
		    level, "", sum.r, sum.g, sum.b, (unsigned long)tot);
	    fprintf(stderr, "COLORTABLE%*s dest=%d,%d,%d weight=%d no=%d\n",
		    level, "", dest->color.r, dest->color.g, dest->color.b,
		    dest->weight, dest->no);
#endif
	    break;
         case NCT_REDUCE_WEIGHT:
	    /* find min and max */
	    for (i=0; i<src_size; i++)
	    {
	       if (min.r>(INT32)src[i].color.r) min.r=src[i].color.r;
	       if (min.g>(INT32)src[i].color.g) min.g=src[i].color.g;
	       if (min.b>(INT32)src[i].color.b) min.b=src[i].color.b;
	       if (max.r<(INT32)src[i].color.r) max.r=src[i].color.r;
	       if (max.g<(INT32)src[i].color.g) max.g=src[i].color.g;
	       if (max.b<(INT32)src[i].color.b) max.b=src[i].color.b;
	       tot+=src[i].weight;
	    }
    
	    dest->color.r =
	      DO_NOT_WARN((COLORTYPE)(max.r*position.r+min.r*(1-position.r)));
	    dest->color.g =
	      DO_NOT_WARN((COLORTYPE)(max.g*position.g+min.g*(1-position.g)));
	    dest->color.b =
	      DO_NOT_WARN((COLORTYPE)(max.b*position.b+min.b*(1-position.b)));
	    dest->weight=tot;
	    dest->no=-1;
#ifdef COLORTABLE_REDUCE_DEBUG
	    fprintf(stderr, "COLORTABLE%*s min=%d,%d,%d max=%d,%d,%d position=%g,%g,%g\n",
		    level, "", min.r, min.g, min.b, max.r, max.g, max.b,
		    position.r, position.g, position.b);
	    fprintf(stderr, "COLORTABLE%*s dest=%d,%d,%d weight=%d no=%d\n",
		    level, "", dest->color.r, dest->color.g, dest->color.b,
		    dest->weight, dest->no);
#endif
	    break;
      }
      return 1;
   }

   /* figure out what direction to split */
   
   tot=0;
   for (i=0; i<src_size; i++)
      if (src[i].weight!=WEIGHT_NEEDED) tot+=src[i].weight; 
   mmul=tot*10;

   if (!tot) /* all needed */
   {
      *dest=*src;
      for (i=n=1; i<src_size; i++)
	 if (src[i].color.r!=dest[n-1].color.r ||
	     src[i].color.g!=dest[n-1].color.g ||
	     src[i].color.b!=dest[n-1].color.b)
	    dest[n++]=src[i];
      return n;
   }

   tot=0;
   for (i=0; i<src_size; i++)
   {
      nct_weight_t mul;
      if ((mul=src[i].weight)==WEIGHT_NEEDED)
	 mul=mmul;
      sum.r += src[i].color.r*mul;
      sum.g += src[i].color.g*mul;
      sum.b += src[i].color.b*mul;
      tot += mul;
   }

#ifdef COLORTABLE_REDUCE_DEBUG
   fprintf(stderr, "COLORTABLE%*s sum=%d,%d,%d\n",
	   level, "", sum.r, sum.g, sum.b);
#endif

   g = (sum.r*sf.r+sum.g*sf.g+sum.b*sf.b)/tot;
   sum.r = sum.r/tot;
   sum.g = sum.g/tot;
   sum.b = sum.b/tot;

#ifdef COLORTABLE_REDUCE_DEBUG
   fprintf(stderr, "COLORTABLE%*s mean=%d,%d,%d,%ld tot=%ld\n",
	   level, "", sum.r, sum.g, sum.b, g, tot);
#endif

   for (i=0; i<src_size; i++)
   {
      nct_weight_t mul;
      if ((mul=src[i].weight)==WEIGHT_NEEDED)
	 mul=mmul;
      diff.r += (sq(src[i].color.r-(INT32)sum.r)/8)*mul;
      diff.g += (sq(src[i].color.g-(INT32)sum.g)/8)*mul;
      diff.b += (sq(src[i].color.b-(INT32)sum.b)/8)*mul;
      gdiff  += (sq(src[i].color.r*sf.r+src[i].color.g*sf.g+
		    src[i].color.b*sf.b-g)/8)*mul;
      tot+=mul;
   }

#ifdef COLORTABLE_REDUCE_DEBUG
   fprintf(stderr, "COLORTABLE%*s pure diff=%d,%d,%d,%d sort=?\n",
	   level, "", diff.r, diff.g, diff.b, gdiff);
#endif

   diff.r*=DIFF_R_MULT;
   diff.g*=DIFF_G_MULT;
   diff.b*=DIFF_B_MULT;
   gdiff = (gdiff*DIFF_GREY_MULT)/(sq(sf.r+sf.g+sf.b));

   if (diff.r > diff.g) 
      if (diff.r > diff.b) 
	 if (diff.r > gdiff) st=SORT_R; else st=SORT_GREY;
      else 
	 if (diff.b > gdiff) st=SORT_B; else st=SORT_GREY;
   else 
      if (diff.g > diff.b) 
	 if (diff.g > gdiff) st=SORT_G; else st=SORT_GREY;
      else 
	 if (diff.b > gdiff) st=SORT_B; else st=SORT_GREY;
#ifdef COLORTABLE_REDUCE_DEBUG
   fprintf(stderr, "COLORTABLE%*s diff=%d,%d,%d,%d sort=%d\n",
	   level, "", diff.r, diff.g, diff.b, gdiff, st);
#endif

   /* half-sort */

   left=0;
   right=src_size-1;

#define HALFSORT(C) \
      while (left<right) \
      { \
         if (src[left].color.C > sum.C) { \
	    struct nct_flat_entry tmp = src[left]; \
            src[left] = src[right]; \
            src[right--] = tmp; \
         } \
         else left++; \
      } \
      space.C/=2.0; \
      newpos1=newpos2=position; \
      newpos1.C-=space.C; \
      newpos2.C+=space.C; 

   switch (st)
   {
      case SORT_R: HALFSORT(r); break;
      case SORT_G: HALFSORT(g); break;
      case SORT_B: HALFSORT(b); break;
      case SORT_GREY:
         while (left<right) 
	 { 
	    if ((src[left].color.r*sf.r + src[left].color.g*sf.g +
		 src[left].color.b*sf.b) > g) {
	       struct nct_flat_entry tmp = src[left];
	       src[left] = src[right];
	       src[right--] = tmp;
	    }
	    else left++; 
	 } 
	 space.r/=2.0;
	 space.g/=2.0;
	 space.b/=2.0;
	 newpos1.r=position.r-space.r;
	 newpos1.g=position.g-space.g;
	 newpos1.b=position.b-space.b;
	 newpos2.r=position.r+space.r;
	 newpos2.g=position.g+space.g;
	 newpos2.b=position.b+space.b;
         break;
   }
   

#ifdef COLORTABLE_REDUCE_DEBUG
   fprintf(stderr, "COLORTABLE%*s left=%d right=%d\n", level, "", left, right);
#endif

   if (left==0) left++;
   while (left &&
	  src[left].color.r==src[left-1].color.r &&
	  src[left].color.g==src[left-1].color.g &&
	  src[left].color.b==src[left-1].color.b) 
      left--;
   
   if (right==src_size-1 && !left) 
   {
#ifdef COLORTABLE_REDUCE_DEBUG
      fprintf(stderr,"got all in one color: %02x%02x%02x %02x%02x%02x\n",
	      src[left].color.r,
	      src[left].color.g,
	      src[left].color.b,
	      src[right].color.r,
	      src[right].color.g,
	      src[right].color.b);
#endif

      *dest=*src;
      while (dest->weight!=WEIGHT_NEEDED && left<=right) 
      {
	 if (src[left].weight==WEIGHT_NEEDED) dest->weight=WEIGHT_NEEDED;
	 else dest->weight+=src[left].weight;
	 left++;
      }
      return 1;
   }

   i=target_size/2;
   if (src_size-left<target_size-i) i+=(target_size-i)-(src_size-left);
   else if (i>left) i=left;

#ifdef COLORTABLE_REDUCE_DEBUG
   fprintf(stderr, "COLORTABLE%*s try i=%d/%d - %d+%d=%d from %d+%d=%d\n",
	   level+1, "", i, target_size, i, target_size-i, target_size, left,
	   src_size-left, src_size);
#endif

   n=reduce_recurse(src,dest,left,i,level+2,sf,newpos1,space,type); 
   m=reduce_recurse(src+left,dest+n,src_size-left,
		    target_size-n,level+2,sf,newpos2,space,type);

#ifdef COLORTABLE_REDUCE_DEBUG
   fprintf(stderr, "COLORTABLE%*s ->%d+%d=%d (tried for %d+%d=%d)\n",
	   level, "", n, m, n+m, i, target_size-i, target_size);
#endif

   if (m>target_size-n && n<=i) /* right is too big, try again */
   {
      ptrdiff_t oldn=n;

      i=target_size-m;
      if (src_size-left<target_size-i) i+=(target_size-i)-(src_size-left);
      else if (i>left) i=left;

#ifdef COLORTABLE_REDUCE_DEBUG
      fprintf(stderr, "COLORTABLE%*s try i=%d/%d - %d+%d=%d from %d+%d=%d\n",
	      level+1, "", i, target_size, i, target_size-i, target_size,
	      left, src_size-left, src_size);
#endif

      n=reduce_recurse(src,dest,left,i,level+2,sf,newpos1,space,type);
      if (n!=oldn) {
	 if (n<oldn) /* i certainly hope so */
	    MEMMOVE(dest+n,dest+oldn,sizeof(struct nct_flat_entry)*m);
	 else /* huh? */
	    /* this is the only case we don't have them already */
	    m=reduce_recurse(src+left,dest+n,src_size-left,
			     target_size-n,level+2,sf,newpos2,space,type);
      }
#ifdef COLORTABLE_REDUCE_DEBUG
      fprintf(stderr, "COLORTABLE%*s ->%d+%d=%d (retried for %d+%d=%d)\n",
	      level, "", n, m, n+m, i, target_size-i, target_size);
#endif
   }

   return n+m;
}

static struct nct_flat _img_reduce_number_of_colors(struct nct_flat flat,
						    unsigned long maxcols,
						    rgbl_group sf)
{
   ptrdiff_t i,j;
   struct nct_flat_entry *newe;
   rgbd_group pos={0.5,0.5,0.5},space={0.5,0.5,0.5};

   if ((size_t)flat.numentries <= (size_t)maxcols) return flat;

   newe=malloc(sizeof(struct nct_flat_entry)*flat.numentries);
   if (!newe) { return flat; }

   i = reduce_recurse(flat.entries,newe, flat.numentries, maxcols, 0, sf,
		      pos, space, NCT_REDUCE_WEIGHT);

   free(flat.entries);

   flat.entries=realloc(newe,i*sizeof(struct nct_flat_entry));
   flat.numentries=i;
   if (!flat.entries) {
     free(newe);
     resource_error(NULL, 0, 0, "memory", 0, "Out of memory.\n");
   }

   for (j=0; j<i; j++)
      flat.entries[j].no=j;
   
   return flat;
}

/*** scanning ***/

#define DEFAULT_COLOR_HASH_SIZE 8192
#define MAX_COLOR_HASH_SIZE 32768

struct color_hash_entry
{
  rgb_group color;
  unsigned long pixels;
  ptrdiff_t no;
};

static INLINE struct color_hash_entry *insert_in_hash(rgb_group rgb,
						struct color_hash_entry *hash,
						size_t hashsize)
{
   size_t j, k;
   j=(rgb.r*127+rgb.b*997+rgb.g*2111)%hashsize;
   k=100;
   if (j+100>=hashsize)
      while (--k && hash[j].pixels && 
	     (hash[j].color.r!=rgb.r || 
	      hash[j].color.g!=rgb.g || 
	      hash[j].color.b!=rgb.b))
	 j=(j+1)%hashsize;
   else
      while (--k && hash[j].pixels && 
	     (hash[j].color.r!=rgb.r || 
	      hash[j].color.g!=rgb.g || 
	      hash[j].color.b!=rgb.b))
	 j++;
   if (!k) return NULL; /* no space */
   hash[j].pixels++;
   hash[j].color=rgb;
   return hash+j;
}

static INLINE struct color_hash_entry *insert_in_hash_nd(rgb_group rgb,
						struct color_hash_entry *hash,
						size_t hashsize)
{
   size_t j, k;
   j=(rgb.r*127+rgb.b*997+rgb.g*2111)%hashsize;
   k=100;
   if (j+100>=hashsize)
      while (--k && hash[j].pixels && 
	     (hash[j].color.r!=rgb.r || 
	      hash[j].color.g!=rgb.g || 
	      hash[j].color.b!=rgb.b))
	 j=(j+1)%hashsize;
   else
      while (--k && hash[j].pixels && 
	     (hash[j].color.r!=rgb.r || 
	      hash[j].color.g!=rgb.g || 
	      hash[j].color.b!=rgb.b))
	 j++;
   if (!k) return NULL; /* no space */
   hash[j].color=rgb;
   return hash+j;
}

static INLINE struct color_hash_entry *insert_in_hash_mask(rgb_group rgb,
    struct color_hash_entry *hash,unsigned long hashsize,rgb_group mask)
{
   unsigned long j,k;
   rgb.r&=mask.r;
   rgb.g&=mask.g;
   rgb.b&=mask.b;
   j=(rgb.r*127+rgb.b*997+rgb.g*2111)%hashsize;
   k=100;
   if (j+100>=hashsize)
      while (--k && hash[j].pixels && 
	     (hash[j].color.r!=rgb.r || 
	      hash[j].color.g!=rgb.g || 
	      hash[j].color.b!=rgb.b))
	 j=(j+1)%hashsize;
   else
      while (--k && hash[j].pixels && 
	     (hash[j].color.r!=rgb.r || 
	      hash[j].color.g!=rgb.g || 
	      hash[j].color.b!=rgb.b))
	 j++;
   if (!k) return NULL; /* no space */
   hash[j].pixels++;
   hash[j].color=rgb;
   return hash+j;
}

static INLINE rgb_group get_mask_of_level(int level)
{
   static const unsigned char strip_r[24]=
   { 0xff, 0xfe, 0xfe, 0xfe, 0xfc, 0xfc, 0xfc, 0xf8, 0xf8, 0xf8, 0xf0,
     0xf0, 0xf0, 0xe0, 0xe0, 0xe0, 0xc0, 0xc0, 0xc0, 0x80, 0x80, 0x80 };
   static const unsigned char strip_g[24]=
   { 0xff, 0xff, 0xfe, 0xfe, 0xfe, 0xfc, 0xfc, 0xfc, 0xf8, 0xf8, 0xf8, 
     0xf0, 0xf0, 0xf0, 0xe0, 0xe0, 0xe0, 0xc0, 0xc0, 0xc0, 0x80, 0x80, 0x80 };
   static const unsigned char strip_b[24]=
   { 0xfe, 0xfe, 0xfe, 0xfc, 0xfc, 0xfc, 0xf8, 0xf8, 0xf8, 0xf0, 0xf0, 
     0xf0, 0xe0, 0xe0, 0xe0, 0xc0, 0xc0, 0xc0, 0x80, 0x80, 0x80 };

   rgb_group res;
   res.r=strip_r[level];
   res.g=strip_g[level];
   res.b=strip_b[level];

   return res;
}

static struct nct_flat _img_get_flat_from_image(struct image *img,
						unsigned long maxcols)
{
   struct color_hash_entry *hash;
   unsigned long hashsize=DEFAULT_COLOR_HASH_SIZE;
   unsigned long i,j,k;
   rgb_group *s;
   struct nct_flat flat;

   hash=(struct color_hash_entry*)
      xalloc(sizeof(struct color_hash_entry)*hashsize);
   i=hashsize;
   while (i--) hash[i].pixels=0;
   
   i=img->xsize*img->ysize;
   s=img->img;

   while (i)
   {
      if (!insert_in_hash(*s,hash,hashsize))
      {
	 struct color_hash_entry *oldhash=hash,*mark;
	 j=hashsize;

rerun_rehash:

	 hashsize*=2;
	 if (hashsize>MAX_COLOR_HASH_SIZE)
	 {
	    hashsize/=2;
	    break; /* try again, with mask */
	 }
#ifdef COLORTABLE_DEBUG
   fprintf(stderr,"COLORTABLE %ld pixels left; hashsize=%ld\n",i,hashsize);
#endif

	 hash=malloc(sizeof(struct color_hash_entry)*hashsize);
	 if (!hash)
	 {
	    free(oldhash);
	    resource_error(NULL,0,0,"memory",0,"Out of memory.\n");
	 }
	 k=hashsize;
	 while (k--) hash[k].pixels=0;

	 while (j--) 
	    if (oldhash[j].pixels)
	    {
	       mark=insert_in_hash(oldhash[j].color,hash,hashsize);
	       if (!mark) goto rerun_rehash;
	       mark->pixels=oldhash[j].pixels;
	    }
	 
	 free(oldhash);
	 continue;
      }

      i--;
      s++;
   }

   if (i) /* restart, but with mask */
   {
      int mask_level=0;
      rgb_group rgb_mask;
      struct color_hash_entry *oldhash,*mark;

rerun_mask:

      rgb_mask=get_mask_of_level(mask_level);
      mask_level++;
#ifdef COLORTABLE_DEBUG
      fprintf(stderr,"COLORTABLE mask level=%d mask=%02x%02x%02x\n",
	      mask_level,rgb_mask.r,rgb_mask.g,rgb_mask.b);
#endif
      
      oldhash=hash;
      j=hashsize;
      
      hash=malloc(sizeof(struct color_hash_entry)*hashsize);
      if (!hash)
      {
	 free(oldhash);
	 resource_error(NULL,0,0,"memory",0,"Out of memory.\n");
      }
      
      k=hashsize;
      while (k--) hash[k].pixels=0;

      while (j--) 
	 if (oldhash[j].pixels)
	 {
	    mark=insert_in_hash_mask(oldhash[j].color,hash,hashsize,rgb_mask);
	    if (!mark) goto rerun_mask; /* increase mask level inst of hash */
	    mark->pixels+=oldhash[j].pixels-1;
	 }

      free(oldhash);
      
      while (i) 
      {
	 if (!insert_in_hash_mask(*s,hash,hashsize,rgb_mask))
	    goto rerun_mask; /* increase mask */

	 i--;
	 s++;
      }
   }

   /* convert to flat */

   i=hashsize;
   j=0;
   while (i--)
      if (hash[i].pixels) j++;
   /* j is now the number of colors */

#ifdef COLORTABLE_DEBUG
   fprintf(stderr,"COLORTABLE %ld colors found in image; hashsize=%ld\n",j,hashsize);
#endif

   flat.numentries=j;
   flat.entries=malloc(sizeof(struct nct_flat_entry)*j);
   if (!flat.entries)
   {
      free(hash);
      resource_error(NULL,0,0,"memory",0,"Out of memory.\n");
   }
   j=0;
   i=hashsize;
   while (i--)
      if (hash[i].pixels)
      {
	 flat.entries[j].color=hash[i].color;
	 flat.entries[j].no=j;
	 flat.entries[j].weight=hash[i].pixels;
	 j++;
      }

   if (((int)j)!=flat.numentries) abort();
   
   free(hash);

   return flat;
}

static struct nct_flat _img_get_flat_from_array(struct array *arr)
{
   struct svalue s,s2;
   struct nct_flat flat;
   int i,n=0;

   flat.numentries=arr->size;
   flat.entries=(struct nct_flat_entry*)
      xalloc(flat.numentries*sizeof(struct nct_flat_entry));

   s2.type=s.type=T_INT;
   for (i=0; i<arr->size; i++)
   {
      if (arr->item[i].type==T_INT && !arr->item[i].u.integer)
	 continue;

      if (!image_color_svalue(arr->item+i,
			      &(flat.entries[n].color)))
	 bad_arg_error("Colortable", 
		       0,0, 1, "array of colors or 0", 0,
		       "Colortable(): bad element %d of colorlist\n",i);

#if DEBUG
      fprintf(stderr,"got %d: %02x%02x%02x\n",n,
	      flat.entries[n].color.r,
	      flat.entries[n].color.g,
	      flat.entries[n].color.b);
#endif

      flat.entries[n].weight=1;
      flat.entries[n].no=i;
      n++;
   }
   flat.numentries=n;

   return flat;
}

static struct nct_flat _img_get_flat_from_string(struct pike_string *str)
{
   struct nct_flat flat;
   int i;

   flat.numentries = str->len/3;
   if (flat.numentries<1) 
      Pike_error("Can't make a colortable with less then one (1) color.\n");

   flat.entries=(struct nct_flat_entry*)
      xalloc(flat.numentries*sizeof(struct nct_flat_entry));

   for (i=0; i<flat.numentries; i++)
   {
      flat.entries[i].color.r=str->str[i*3];
      flat.entries[i].color.g=str->str[i*3+1];
      flat.entries[i].color.b=str->str[i*3+2];
      flat.entries[i].weight=1;
      flat.entries[i].no=i;
   }

   return flat;
}

static struct nct_flat _img_get_flat_from_bgr_string(struct pike_string *str)
{
   struct nct_flat flat;
   int i;

   flat.numentries=str->len/3;
   if (flat.numentries<1) 
      Pike_error("Can't make a colortable with less then one (1) color.\n");

   flat.entries=(struct nct_flat_entry*)
      xalloc(flat.numentries*sizeof(struct nct_flat_entry));

   for (i=0; i<flat.numentries; i++)
   {
      flat.entries[i].color.r=str->str[i*3+2];
      flat.entries[i].color.g=str->str[i*3+1];
      flat.entries[i].color.b=str->str[i*3];
      flat.entries[i].weight=1;
      flat.entries[i].no=i;
   }

   return flat;
}

static struct nct_flat _img_get_flat_from_bgrz_string(struct pike_string *str)
{
   struct nct_flat flat;
   int i;

   flat.numentries=str->len/4;
   if (flat.numentries<1) 
      Pike_error("Can't make a colortable with less then one (1) color.\n");

   flat.entries=(struct nct_flat_entry*)
      xalloc(flat.numentries*sizeof(struct nct_flat_entry));

   for (i=0; i<flat.numentries; i++)
   {
      flat.entries[i].color.r=str->str[i*4+2];
      flat.entries[i].color.g=str->str[i*4+1];
      flat.entries[i].color.b=str->str[i*4];
      flat.entries[i].weight=1;
      flat.entries[i].no=i;
   }

   return flat;
}

static INLINE void _find_cube_dist(struct nct_cube cube,rgb_group rgb,
				   int *dist,int *no,
				   rgbl_group sf)
{
   int mindist;
   struct nct_scale *s;
   int nc;

   *no=-1;

   if (cube.r&&cube.g&&cube.b)
   {
      mindist =
	sf.r*sq_int((((rgb.r*cube.r+cube.r/2)>>8)*255)/(cube.r-1)-rgb.r)+
	sf.g*sq_int((((rgb.g*cube.g+cube.g/2)>>8)*255)/(cube.g-1)-rgb.g)+
	sf.b*sq_int((((rgb.b*cube.b+cube.b/2)>>8)*255)/(cube.b-1)-rgb.b);

      *no=((INT32)(rgb.r*cube.r+cube.r/2)>>8)+
	  ((INT32)(rgb.g*cube.g+cube.g/2)>>8)*cube.r+
	  ((INT32)(rgb.b*cube.b+cube.b/2)>>8)*cube.r*cube.g;

      if (mindist<cube.disttrig)
      {
	 *dist = mindist;
	 return;
      }
   }
   else
      mindist=10000000;

   s=cube.firstscale;

   nc=cube.r*cube.g*cube.b;

   while (s)
   {
      rgbl_group b;
      int n;

      b.r=((INT32)rgb.r)-s->low.r;
      b.g=((INT32)rgb.g)-s->low.g;
      b.b=((INT32)rgb.b)-s->low.b;

      n = DOUBLE_TO_INT((s->steps*(b.r*s->vector.r+
				   b.g*s->vector.g+
				   b.b*s->vector.b))*s->invsqvector);
      
      if (n<0) n=0; else if (n>=s->steps) n=s->steps-1;

      if (s->no[n]>=nc) 
      {
	 int steps=s->steps;
	 int ldist =
	   sf.r*sq_int(rgb.r-((s->high.r*n+s->low.r*(steps-n-1))/(steps-1)))+
	   sf.g*sq_int(rgb.g-((s->high.g*n+s->low.g*(steps-n-1))/(steps-1)))+
	   sf.b*sq_int(rgb.b-((s->high.b*n+s->low.b*(steps-n-1))/(steps-1)));

	 if (ldist<mindist)
	 {
	    *no=s->no[n];
	    mindist = ldist;
	 }
      }

      nc+=s->realsteps;
      s=s->next;
   }
  
   *dist = mindist;
}

static struct nct_cube _img_get_cube_from_args(INT32 args)
{
   struct nct_cube cube;
   struct nct_scale *s,**np;
   int no,i;
   int osteps,o2steps;
   rgbl_group sf;

   INT32 ap;

   if (args<3 ||
       sp[-args].type!=T_INT ||
       sp[1-args].type!=T_INT ||
       sp[2-args].type!=T_INT)
      Pike_error("Image.Colortable->create (get cube from args): Illegal argument(s) 1, 2 or 3\n");

   cube.r=sp[-args].u.integer;
   cube.g=sp[1-args].u.integer;
   cube.b=sp[2-args].u.integer;
   if (cube.r<2) cube.r=0;
   if (cube.g<2) cube.g=0;
   if (cube.b<2) cube.b=0;

   sf=THIS->spacefactor;

   if (cube.r && cube.b && cube.g)
      cube.disttrig=(sf.r*SQ(255/cube.r)+
		     sf.g*SQ(255/cube.g)+
		     sf.b*SQ(255/cube.b))/4;
  			 /* 4 is for /2², closest dot, always */
   else
      cube.disttrig=100000000;
      
   no=cube.r*cube.g*cube.b;

   ap=3;

   cube.firstscale=NULL;
   np=&(cube.firstscale);

   while (args>=ap+3)
   {
      rgb_group low,high;
      int steps;
      int isteps;
      int mdist;
      int c;

      if (!image_color_arg(-args,&low))
	 SIMPLE_BAD_ARG_ERROR("colortable",1,"color");
      if (!image_color_arg(1-args,&high))
	 SIMPLE_BAD_ARG_ERROR("colortable",2,"color");
      if (sp[2+ap-args].type!=T_INT)
	 Pike_error("illegal argument(s) %d, %d or %d\n",ap,ap+1,ap+2);

      steps=isteps=sp[2+ap-args].u.integer;
      ap+=3;

      if (steps<2) continue; /* no idea */

      c=0;
      o2steps=osteps=-1;
      for (;;)
      {
	 mdist=((INT32)(SQ((INT32)low.r-high.r)+
			SQ((INT32)low.g-high.g)+
			SQ((INT32)low.b-high.b))
		/SQ(steps)) / ( 4 );
  			 /* 4 is for /2², closest dot, always */
			 /* 6 makes a suitable constant */
	 if (mdist>cube.disttrig) mdist=cube.disttrig;

	 c=0;
	 for (i=0; i<steps; i++)
	 {
	    rgb_group rgb;
	    int dist,dummyno;

	    rgb.r=(unsigned char)((INT32)(high.r*i+low.r*(steps-i-1))/(steps-1));
	    rgb.g=(unsigned char)((INT32)(high.g*i+low.g*(steps-i-1))/(steps-1));
	    rgb.b=(unsigned char)((INT32)(high.b*i+low.b*(steps-i-1))/(steps-1));

	    _find_cube_dist(cube,rgb,&dist,&dummyno,sf);

	    if (dist>mdist) c++; /* important! same test as below */
	 }

	 if (c>=isteps) break;
	 if (c==o2steps) break; /* sanity check */
	 o2steps=osteps;
	 osteps=c;
	 steps++; /* room for more */
      }

      if (isteps<c) { steps--; c=osteps; }
      isteps=c;

      cube.disttrig=mdist;

      s=malloc(sizeof(struct nct_scale)+sizeof(int)*steps);
      if (!s) continue;
      s->low=low;
      s->high=high;
      s->vector.r=high.r-(INT32)low.r;
      s->vector.g=high.g-(INT32)low.g;
      s->vector.b=high.b-(INT32)low.b;
      s->invsqvector=1.0/(SQ(s->vector.r)+SQ(s->vector.g)+SQ(s->vector.b));
      s->steps=steps;
      s->realsteps=isteps;
      s->mqsteps=1.0/(steps-1);

#ifdef COLORTABLE_DEBUG
      fprintf(stderr,"COLORTABLE %d steps, %d live, trig=%d\n",s->steps,s->realsteps,cube.disttrig);
#endif

      for (i=0; i<steps; i++)
      {
	 rgb_group rgb;
	 int dist,dummyno;

	 rgb.r=(unsigned char)((int)(high.r*i+low.r*(steps-i))/(steps-1));
	 rgb.g=(unsigned char)((int)(high.g*i+low.g*(steps-i))/(steps-1));
	 rgb.b=(unsigned char)((int)(high.b*i+low.b*(steps-i))/(steps-1));

	 _find_cube_dist(cube,rgb,&dist,&dummyno,sf);

#ifdef COLORTABLE_DEBUG
	 fprintf(stderr,"COLORTABLE step %d: %02x%02x%02x s_no=%d dist=%d\n",i,rgb.r,rgb.g,rgb.b,dummyno,dist);
#endif

	 if (dist>cube.disttrig)  /* important! same test as above */
	    s->no[i]=no++;
	 else 
	    s->no[i]=dummyno;
      }
      
      *np=s;
      s->next=NULL;
      np=&(s->next);
   }
   cube.numentries=no;

   return cube;
}

static struct nct_flat _img_nct_cube_to_flat(struct nct_cube cube)
{
   struct nct_flat flat;
   int no;
   int r,g,b;
   struct nct_scale *s;

   flat.numentries=cube.numentries;
   flat.entries=(struct nct_flat_entry*)
      xalloc(sizeof(struct nct_flat_entry)*flat.numentries);

   no=0;

   if (cube.b && cube.g && cube.b)
      for (b=0; b<cube.b; b++)
	 for (g=0; g<cube.g; g++)
	    for (r=0; r<cube.r; r++)
	    {
	       flat.entries[no].color.r=(unsigned char)((0xff*r)/(cube.r-1));
	       flat.entries[no].color.g=(unsigned char)((0xff*g)/(cube.g-1));
	       flat.entries[no].color.b=(unsigned char)((0xff*b)/(cube.b-1));
	       flat.entries[no].no=no;
	       flat.entries[no].weight=cube.weight;
	       no++;
	    }

   s=cube.firstscale;
   while (s)
   {
      int i;
      for (i=0; i<s->steps; i++)
	 if (s->steps && s->no[i]>=no)
	 {
	    flat.entries[no].color.r=
	       (unsigned char)((int)(s->high.r*i+s->low.r*(s->steps-i-1))/(s->steps-1));
	    flat.entries[no].color.g=
	       (unsigned char)((int)(s->high.g*i+s->low.g*(s->steps-i-1))/(s->steps-1));
	    flat.entries[no].color.b=
	       (unsigned char)((int)(s->high.b*i+s->low.b*(s->steps-i-1))/(s->steps-1));
	    flat.entries[no].no=no;
	    flat.entries[no].weight=cube.weight;
	    no++;
	 }
      s=s->next;
   }

   if (no!=cube.numentries) abort(); /* simple sanity check */

   return flat;
} 

static void _img_add_colortable(struct neo_colortable *rdest,
				struct neo_colortable *src)
{
   struct neo_colortable tmp1,tmp2;
   struct color_hash_entry *hash,*mark;
   size_t i,j,hashsize,k;
   struct nct_flat_entry *en;
   struct nct_flat flat;
   struct neo_colortable *dest=rdest;
   ptrdiff_t no;

   colortable_init_stuff(&tmp1);
   colortable_init_stuff(&tmp2);

   if (dest->type==NCT_NONE)
   {
      _img_copy_colortable(dest,src);
      return;
   }

   for (i=0; i<COLORLOOKUPCACHEHASHSIZE; i++)
      dest->lookupcachehash[i].index=-1;

   if (src->type==NCT_CUBE)
   {
      tmp1.u.flat=_img_nct_cube_to_flat(src->u.cube);
      tmp1.type=NCT_FLAT;
      src=&tmp1;
   }
   if (dest->type==NCT_CUBE)
   {
      tmp2.u.flat=_img_nct_cube_to_flat(dest->u.cube);
      tmp2.type=NCT_FLAT;
      dest=&tmp2;
   }

   hashsize=(dest->u.flat.numentries+src->u.flat.numentries)*2;
   hash=(struct color_hash_entry*)
      xalloc(sizeof(struct color_hash_entry)*hashsize);

   i=hashsize;
   while (i--) hash[i].pixels=0;

#if 0
   fprintf(stderr,"add\n");
   stderr_print_entries(src->u.flat.entries,src->u.flat.numentries);
   fprintf(stderr,"to\n");
   stderr_print_entries(dest->u.flat.entries,dest->u.flat.numentries);
#endif

   i=dest->u.flat.numentries;
   en=dest->u.flat.entries;

   while (i)
   {
      if (!(mark=insert_in_hash_nd(en->color,hash,hashsize)))
      {
	 struct color_hash_entry *oldhash=hash;
	 j=hashsize;

rerun_rehash_add_1:

	 hashsize*=2;

	 hash=malloc(sizeof(struct color_hash_entry)*hashsize);
	 if (!hash)
	 {
	    free(oldhash);
	    free_colortable_struct(&tmp1);
	    free_colortable_struct(&tmp2);
	    resource_error(NULL,0,0,"memory",0,"Out of memory.\n");
	 }
	 k=hashsize;
	 while (k--) hash[k].pixels=0;

	 while (j--) 
	    if (oldhash[j].pixels)
	    {
	       mark=insert_in_hash_nd(oldhash[j].color,hash,hashsize);
	       if (!mark) goto rerun_rehash_add_1;
	       mark->no=oldhash[i].no;
	       mark->pixels=oldhash[i].pixels;
	    }
	 
	 free(oldhash);
	 continue;
      }

      mark->no=en->no;
      mark->pixels += DO_NOT_WARN((unsigned long)en->weight);

      i--;
      en++;
   }

   i=src->u.flat.numentries;
   en=src->u.flat.entries;
   no=dest->u.flat.numentries;

   while (i)
   {
      if (!(mark=insert_in_hash_nd(en->color,hash,hashsize)))
      {
	 struct color_hash_entry *oldhash=hash;
	 j=hashsize;

rerun_rehash_add_2:

	 hashsize*=2;

	 hash=malloc(sizeof(struct color_hash_entry)*hashsize);
	 if (!hash)
	 {
	    free(oldhash);
	    free_colortable_struct(&tmp1);
	    free_colortable_struct(&tmp2);
	    resource_error(NULL,0,0,"memory",0,"Out of memory.\n");
	 }
	 i=hashsize;
	 while (i--) hash[i].pixels=0;

	 while (j--) 
	    if (oldhash[j].pixels)
	    {
	       mark=insert_in_hash_nd(oldhash[j].color,hash,hashsize);
	       if (!mark) goto rerun_rehash_add_2;
	       if (mark->pixels!=1)
		  mark->no=oldhash[i].no;
	       mark->pixels=oldhash[i].pixels;
	    }
	 
	 free(oldhash);
	 continue;
      }

      if (mark->pixels==WEIGHT_NEEDED || en->weight==WEIGHT_NEEDED) 
	 mark->pixels=WEIGHT_NEEDED;
      else
      {
	 if (!mark->pixels) mark->no=no++;
	 mark->pixels += DO_NOT_WARN((unsigned long)en->weight);
      }

      i--;
      en++;
   }

   /* convert to flat */

   i=hashsize;
   j=0;
   while (i--)
      if (hash[i].pixels) j++;
   /* j is now the number of colors */
   flat.numentries=j;
   flat.entries=malloc(sizeof(struct nct_flat_entry)*j);
   if (!flat.entries)
   {
      free(hash);
      resource_error(NULL,0,0,"memory",0,"Out of memory.\n");
   }
   j=0;
   i=hashsize;
   while (i--)
      if (hash[i].pixels)
      {
	 flat.entries[j].color=hash[i].color;
	 flat.entries[j].no=j;
	 flat.entries[j].weight=hash[i].pixels;
	 j++;
      }
   free(hash);

   free_colortable_struct(&tmp1);
   free_colortable_struct(&tmp2);

   free_colortable_struct(rdest);

   rdest->type=NCT_FLAT; 
   rdest->u.flat=flat;
}

static void _img_sub_colortable(struct neo_colortable *rdest,
				struct neo_colortable *src)
{
   struct neo_colortable tmp1,tmp2;
   struct color_hash_entry *hash,*mark;
   size_t i,j,hashsize,k;
   struct nct_flat_entry *en;
   struct nct_flat flat;
   struct neo_colortable *dest=rdest;
   ptrdiff_t no;

   colortable_init_stuff(&tmp1);
   colortable_init_stuff(&tmp2);

   if (dest->type==NCT_NONE)
   {
      _img_copy_colortable(dest,src);
      return;
   }

   for (i=0; i<COLORLOOKUPCACHEHASHSIZE; i++)
      dest->lookupcachehash[i].index=-1;

   if (src->type==NCT_CUBE)
   {
      tmp1.u.flat=_img_nct_cube_to_flat(src->u.cube);
      tmp1.type=NCT_FLAT;
      src=&tmp1;
   }
   if (dest->type==NCT_CUBE)
   {
      tmp2.u.flat=_img_nct_cube_to_flat(dest->u.cube);
      tmp2.type=NCT_FLAT;
      dest=&tmp2;
   }

   hashsize=(dest->u.flat.numentries+src->u.flat.numentries)*2;
   hash=(struct color_hash_entry*)
      xalloc(sizeof(struct color_hash_entry)*hashsize);

   i=hashsize;
   while (i--) hash[i].pixels=0;

#if 0
   fprintf(stderr,"add\n");
   stderr_print_entries(src->u.flat.entries,src->u.flat.numentries);
   fprintf(stderr,"to\n");
   stderr_print_entries(dest->u.flat.entries,dest->u.flat.numentries);
#endif

   i=dest->u.flat.numentries;
   en=dest->u.flat.entries;

   while (i)
   {
      if (!(mark=insert_in_hash_nd(en->color,hash,hashsize)))
      {
	 struct color_hash_entry *oldhash=hash;
	 j=hashsize;

rerun_rehash_add_1:

	 hashsize*=2;

	 hash=malloc(sizeof(struct color_hash_entry)*hashsize);
	 if (!hash)
	 {
	    free(oldhash);
	    free_colortable_struct(&tmp1);
	    free_colortable_struct(&tmp2);
	    resource_error(NULL,0,0,"memory",0,"Out of memory.\n");
	 }
	 k=hashsize;
	 while (k--) hash[k].pixels=0;

	 while (j--) 
	    if (oldhash[j].pixels)
	    {
	       mark=insert_in_hash_nd(oldhash[j].color,hash,hashsize);
	       if (!mark) goto rerun_rehash_add_1;
	       mark->no=oldhash[i].no;
	       mark->pixels=oldhash[i].pixels;
	    }
	 
	 free(oldhash);
	 continue;
      }

      mark->no=en->no;
      mark->pixels += DO_NOT_WARN((unsigned long)en->weight);

      i--;
      en++;
   }

   i=src->u.flat.numentries;
   en=src->u.flat.entries;
   no=dest->u.flat.numentries;

   while (i)
   {
      if (!(mark=insert_in_hash_nd(en->color,hash,hashsize)))
      {
	 struct color_hash_entry *oldhash=hash;
	 j=hashsize;

rerun_rehash_add_2:

	 hashsize*=2;

	 hash=malloc(sizeof(struct color_hash_entry)*hashsize);
	 if (!hash)
	 {
	    free(oldhash);
	    free_colortable_struct(&tmp1);
	    free_colortable_struct(&tmp2);
	    resource_error(NULL,0,0,"memory",0,"Out of memory.\n");
	 }
	 i=hashsize;
	 while (i--) hash[i].pixels=0;

	 while (j--) 
	    if (oldhash[j].pixels)
	    {
	       mark=insert_in_hash_nd(oldhash[j].color,hash,hashsize);
	       if (!mark) goto rerun_rehash_add_2;
	       if (mark->pixels!=1)
		  mark->no=oldhash[i].no;
	       mark->pixels=oldhash[i].pixels;
	    }
	 
	 free(oldhash);
	 continue;
      }

      mark->pixels=WEIGHT_REMOVE;

      i--;
      en++;
   }

   /* convert to flat */

   i=hashsize;
   j=0;
   while (i--)
      if (hash[i].pixels && hash[i].pixels!=WEIGHT_REMOVE) j++;
   /* j is now the number of colors */
   flat.numentries=j;
   flat.entries=malloc(sizeof(struct nct_flat_entry)*j);
   if (!flat.entries)
   {
      free(hash);
      resource_error(NULL,0,0,"memory",0,"Out of memory.\n");
   }
   j=0;
   i=hashsize;
   while (i--)
      if (hash[i].pixels && hash[i].pixels!=WEIGHT_REMOVE)
      {
	 flat.entries[j].color=hash[i].color;
	 flat.entries[j].no=j;
	 flat.entries[j].weight=hash[i].pixels;
	 j++;
      }
   free(hash);

   free_colortable_struct(&tmp1);
   free_colortable_struct(&tmp2);

   free_colortable_struct(rdest);

   rdest->type=NCT_FLAT; 
   rdest->u.flat=flat;
}

/**** dither ****/


static rgbl_group dither_floyd_steinberg_encode(struct nct_dither *dith,
					       int rowpos,
					       rgb_group s)
{
   rgbl_group rgb;
   int i;
   rgbd_group *err=dith->u.floyd_steinberg.errors+rowpos;
   if (err->r>255) err->r=255; else if (err->r<-255) err->r=-255;
   if (err->g>255) err->g=255; else if (err->g<-255) err->g=-255;
   if (err->b>255) err->b=255; else if (err->b<-255) err->b=-255;
   i = DOUBLE_TO_INT((int)s.r-err->r+0.5); 
   rgb.r=i<0?0:(i>255?255:i);
   i = DOUBLE_TO_INT((int)s.g-err->g+0.5); 
   rgb.g=i<0?0:(i>255?255:i);
   i = DOUBLE_TO_INT((int)s.b-err->b+0.5); 
   rgb.b=i<0?0:(i>255?255:i);
   return rgb;
}

static void dither_floyd_steinberg_got(struct nct_dither *dith,
				       int rowpos,
				       rgb_group s,
				       rgb_group d)
{
   int cd = dith->u.floyd_steinberg.currentdir;

   rgbd_group *ner=dith->u.floyd_steinberg.nexterrors;
   rgbd_group *er=dith->u.floyd_steinberg.errors;
   rgbd_group err;

   err.r = DO_NOT_WARN((float)((DOUBLE_TO_INT(d.r)-DOUBLE_TO_INT(s.r)) +
			       er[rowpos].r+0.5));
   err.g = DO_NOT_WARN((float)((DOUBLE_TO_INT(d.g)-DOUBLE_TO_INT(s.g)) +
			       er[rowpos].g+0.5));
   err.b = DO_NOT_WARN((float)((DOUBLE_TO_INT(d.b)-DOUBLE_TO_INT(s.b)) +
			       er[rowpos].b+0.5));
 
   ner[rowpos].r+=err.r*dith->u.floyd_steinberg.down;
   ner[rowpos].g+=err.g*dith->u.floyd_steinberg.down;
   ner[rowpos].b+=err.b*dith->u.floyd_steinberg.down;

   if (rowpos+cd>=0 && rowpos+cd<dith->rowlen)
   {
      ner[rowpos+cd].r+=err.r*dith->u.floyd_steinberg.downforward;
      ner[rowpos+cd].g+=err.g*dith->u.floyd_steinberg.downforward;
      ner[rowpos+cd].b+=err.b*dith->u.floyd_steinberg.downforward;
      er[rowpos+cd].r+=err.r*dith->u.floyd_steinberg.forward;
      er[rowpos+cd].g+=err.g*dith->u.floyd_steinberg.forward;
      er[rowpos+cd].b+=err.b*dith->u.floyd_steinberg.forward;
   }
   if (rowpos-cd>=0 && rowpos-cd<dith->rowlen)
   {
      ner[rowpos-cd].r+=err.r*dith->u.floyd_steinberg.downback;
      ner[rowpos-cd].g+=err.g*dith->u.floyd_steinberg.downback;
      ner[rowpos-cd].b+=err.b*dith->u.floyd_steinberg.downback;
   }
}

static void dither_floyd_steinberg_newline(struct nct_dither *dith,
					   int *rowpos,
					   rgb_group **s,
					   rgb_group **drgb,
					   unsigned char **d8bit,
					   unsigned short **d16bit,
					   unsigned INT32 **d32bit,
					   int *cd)
{
   rgbd_group *er;
   int i;
   er=dith->u.floyd_steinberg.errors;
   dith->u.floyd_steinberg.errors=dith->u.floyd_steinberg.nexterrors;
   dith->u.floyd_steinberg.nexterrors=er;
   for (i=0; i<dith->rowlen; i++) er[i].r=er[i].g=er[i].b=0.0;

#if 0
   fprintf(stderr,"line r: ");
   er=dith->u.floyd_steinberg.errors;
   for (i=0; i<dith->rowlen; i++)
      fprintf(stderr,"%.2f,",er[i].r);
   fprintf(stderr,"\n");
#endif

   if (dith->u.floyd_steinberg.dir==0)
   {
      dith->u.floyd_steinberg.currentdir=*cd=-*cd;
      switch (*cd)
      {
	 case -1: /* switched from 1 to -1, jump rowlen-1 */
	    (*s)+=dith->rowlen-1;
	    if (drgb) (*drgb)+=dith->rowlen-1;
	    if (d8bit) (*d8bit)+=dith->rowlen-1;
	    if (d16bit) (*d16bit)+=dith->rowlen-1;
	    if (d32bit) (*d32bit)+=dith->rowlen-1;
	    (*rowpos)=dith->rowlen-1;
	    break;
	 case 1: /* switched from 1 to -1, jump rowlen+1 */
	    (*s)+=dith->rowlen+1;
	    if (drgb) (*drgb)+=dith->rowlen+1;
	    if (d8bit) (*d8bit)+=dith->rowlen+1;
	    if (d16bit) (*d16bit)+=dith->rowlen+1;
	    if (d32bit) (*d32bit)+=dith->rowlen+1;
	    (*rowpos)=0;
	    break;
      }
   }
   else if (*cd==-1)
   {
      (*s)+=dith->rowlen*2;
      if (drgb) (*drgb)+=dith->rowlen*2;
      if (d8bit) (*d8bit)+=dith->rowlen*2;
      if (d16bit) (*d16bit)+=dith->rowlen*2;
      if (d32bit) (*d32bit)+=dith->rowlen*2;
      (*rowpos)=dith->rowlen-1;
   }
   else
   {
      (*rowpos)=0;
   }
}

static void dither_floyd_steinberg_firstline(struct nct_dither *dith,
					     int *rowpos,
					     rgb_group **s,
					     rgb_group **drgb,
					     unsigned char **d8bit,
					     unsigned short **d16bit,
					     unsigned INT32 **d32bit,
					     int *cd)
{
   rgbd_group *er;
   int i;

   er=dith->u.floyd_steinberg.errors;
   for (i=0; i<dith->rowlen; i++)
   {
      er[i].r = DO_NOT_WARN((float)((my_rand()&65535)*(1.0/65536)-0.49999));
      er[i].g = DO_NOT_WARN((float)((my_rand()&65535)*(1.0/65536)-0.49999));
      er[i].b = DO_NOT_WARN((float)((my_rand()&65535)*(1.0/65536)-0.49999));
   }

   er=dith->u.floyd_steinberg.nexterrors;
   for (i=0; i<dith->rowlen; i++) er[i].r=er[i].g=er[i].b=0.0;

   if (dith->u.floyd_steinberg.dir>=0)
   {
      dith->u.floyd_steinberg.currentdir=(*cd)=1;
      *rowpos=0;
   }
   else
   {
      dith->u.floyd_steinberg.currentdir=(*cd)=-1;
      (*rowpos)=dith->rowlen-1;
      (*s)+=dith->rowlen-1;
      if (drgb) (*drgb)+=dith->rowlen-1;
      if (d8bit) (*d8bit)+=dith->rowlen-1;
      if (d16bit) (*d16bit)+=dith->rowlen-1;
      if (d32bit) (*d32bit)+=dith->rowlen-1;
   }
}

static rgbl_group dither_randomcube_encode(struct nct_dither *dith,
					   int rowpos,
					   rgb_group s)
{
   rgbl_group rgb;
   int i;
   i=(int)(s.r-(my_rand()%(dith->u.randomcube.r*2-1))+dith->u.randomcube.r+1); 
   rgb.r=i<0?0:(i>255?255:i); 			       
   i=(int)(s.g-(my_rand()%(dith->u.randomcube.g*2-1))+dith->u.randomcube.g+1); 
   rgb.g=i<0?0:(i>255?255:i); 			       
   i=(int)(s.b-(my_rand()%(dith->u.randomcube.b*2-1))+dith->u.randomcube.b+1); 
   rgb.b=i<0?0:(i>255?255:i);
   return rgb;
}

static rgbl_group dither_randomgrey_encode(struct nct_dither *dith,
					   int rowpos,
					   rgb_group s)
{
   rgbl_group rgb;
   int i;
   int err = -(int)((my_rand()%(dith->u.randomcube.r*2-1))+
		    dith->u.randomcube.r+1);
   i=(int)(s.r+err); 
   rgb.r=i<0?0:(i>255?255:i); 			       
   i=(int)(s.g+err); 
   rgb.g=i<0?0:(i>255?255:i); 			       
   i=(int)(s.b+err); 
   rgb.b=i<0?0:(i>255?255:i);
   return rgb;
}

static void dither_ordered_newline(struct nct_dither *dith,
				   int *rowpos,
				   rgb_group **s,
				   rgb_group **drgb,
				   unsigned char **d8bit,
				   unsigned short **d16bit,
				   unsigned INT32 **d32bit,
				   int *cd)
{
   dith->u.ordered.row++;
}

static rgbl_group dither_ordered_encode(struct nct_dither *dith,
					int rowpos,
					rgb_group s)
{
   rgbl_group rgb;
   int i;
   int xs=dith->u.ordered.xs;
   int ys=dith->u.ordered.ys;

   i=(int)(s.r+dith->u.ordered.rdiff
	          [((rowpos+dith->u.ordered.rx)%xs)+
		   ((dith->u.ordered.row+dith->u.ordered.ry)%ys)*xs]); 
   rgb.r=i<0?0:(i>255?255:i); 			       
   i=(int)(s.g+dith->u.ordered.gdiff
	          [((rowpos+dith->u.ordered.gx)%xs)+
		   ((dith->u.ordered.row+dith->u.ordered.gy)%ys)*xs]); 
   rgb.g=i<0?0:(i>255?255:i); 			       
   i=(int)(s.b+dith->u.ordered.bdiff
	          [((rowpos+dith->u.ordered.bx)%xs)+
		   ((dith->u.ordered.row+dith->u.ordered.by)%ys)*xs]); 
   rgb.b=i<0?0:(i>255?255:i);
   return rgb;
}

static rgbl_group dither_ordered_encode_same(struct nct_dither *dith,
                                             int rowpos,
                                             rgb_group s)
{
   rgbl_group rgb;
   int i;

   i=(int)(dith->u.ordered.rdiff
           [((rowpos+dith->u.ordered.rx)&dith->u.ordered.xa)+
           ((dith->u.ordered.row+dith->u.ordered.ry)&dith->u.ordered.ya)*dith->u.ordered.xs]); 
   if (i<0)
   {
     rgb.r=s.r+i; rgb.r=rgb.r<0?0:rgb.r;
     rgb.g=s.g+i; rgb.g=rgb.g<0?0:rgb.g;
     rgb.b=s.b+i; rgb.b=rgb.b<0?0:rgb.b;
   }
   else
   {
     rgb.r=s.r+i; rgb.r=rgb.r>255?255:rgb.r;
     rgb.g=s.g+i; rgb.g=rgb.g>255?255:rgb.g;
     rgb.b=s.b+i; rgb.b=rgb.b>255?255:rgb.b;
   }
   return rgb;
}


int image_colortable_initiate_dither(struct neo_colortable *nct,
				     struct nct_dither *dith,
				     int rowlen)
{
   dith->rowlen=rowlen;

   dith->encode=NULL;
   dith->got=NULL;
   dith->newline=NULL;
   dith->firstline=NULL;

   switch (dith->type=nct->dither_type)
   {
      case NCTD_NONE:
#ifdef COLORTABLE_DEBUG
	 fprintf(stderr, "COLORTABLE image_colortable_initiate_dither: "
		 "NONE\n");
#endif /* COLORTABLE_DEBUG */
	 return 1;

      case NCTD_FLOYD_STEINBERG:
#ifdef COLORTABLE_DEBUG
	 fprintf(stderr, "COLORTABLE image_colortable_initiate_dither: "
		 "FLOYD_STEINBERG\n");
#endif /* COLORTABLE_DEBUG */
	 dith->u.floyd_steinberg.errors=malloc(rowlen*sizeof(rgbd_group));
	 if (!dith->u.floyd_steinberg.errors) return 0;
	 dith->u.floyd_steinberg.nexterrors=malloc(rowlen*sizeof(rgbd_group));
	 if (!dith->u.floyd_steinberg.nexterrors)
	    { free(dith->u.floyd_steinberg.errors);  return 0; }

	 dith->encode=dither_floyd_steinberg_encode;
	 dith->got=dither_floyd_steinberg_got;
	 dith->newline=dither_floyd_steinberg_newline;
	 dith->firstline=dither_floyd_steinberg_firstline;

	 dith->u.floyd_steinberg.forward=nct->du.floyd_steinberg.forward;
	 dith->u.floyd_steinberg.downforward=nct->du.floyd_steinberg.downforward;
	 dith->u.floyd_steinberg.downback=nct->du.floyd_steinberg.downback;
	 dith->u.floyd_steinberg.down=nct->du.floyd_steinberg.down;
	 dith->u.floyd_steinberg.currentdir=
	    dith->u.floyd_steinberg.dir=
	       nct->du.floyd_steinberg.dir;
	 return 1;

      case NCTD_RANDOMCUBE:
#ifdef COLORTABLE_DEBUG
	 fprintf(stderr, "COLORTABLE image_colortable_initiate_dither: "
		 "RANDOMCUBE\n");
#endif /* COLORTABLE_DEBUG */
	 dith->u.randomcube=THIS->du.randomcube;

	 dith->encode=dither_randomcube_encode;

	 return 1;

      case NCTD_RANDOMGREY:
#ifdef COLORTABLE_DEBUG
	 fprintf(stderr, "COLORTABLE image_colortable_initiate_dither: "
		 "RANDOMGREY\n");
#endif /* COLORTABLE_DEBUG */
	 dith->u.randomcube=THIS->du.randomcube;

	 dith->encode=dither_randomgrey_encode;

	 return 1;

      case NCTD_ORDERED:
#ifdef COLORTABLE_DEBUG
	 fprintf(stderr, "COLORTABLE image_colortable_initiate_dither: "
		 "ORDERED\n");
#endif /* COLORTABLE_DEBUG */
	 /* copy it all */
	 dith->u.ordered=nct->du.ordered;

	 /* make space and copy diff matrices */
	 dith->u.ordered.rdiff=malloc(sizeof(int)*nct->du.ordered.xs*
				      nct->du.ordered.ys);
	 dith->u.ordered.gdiff=malloc(sizeof(int)*nct->du.ordered.xs*
				      nct->du.ordered.ys);
	 dith->u.ordered.bdiff=malloc(sizeof(int)*nct->du.ordered.xs*
				      nct->du.ordered.ys);
	 if (!dith->u.ordered.rdiff||
	     !dith->u.ordered.gdiff||
	     !dith->u.ordered.bdiff)
	 {
	    if (dith->u.ordered.rdiff) free(dith->u.ordered.rdiff);
	    if (dith->u.ordered.gdiff) free(dith->u.ordered.gdiff);
	    if (dith->u.ordered.bdiff) free(dith->u.ordered.bdiff);
	    return 0;
	 }
	 MEMCPY(dith->u.ordered.rdiff,nct->du.ordered.rdiff,
		sizeof(int)*nct->du.ordered.xs*nct->du.ordered.ys);
	 MEMCPY(dith->u.ordered.gdiff,nct->du.ordered.gdiff,
		sizeof(int)*nct->du.ordered.xs*nct->du.ordered.ys);
	 MEMCPY(dith->u.ordered.bdiff,nct->du.ordered.bdiff,
		sizeof(int)*nct->du.ordered.xs*nct->du.ordered.ys);

	 dith->u.ordered.row=0;

         if (nct->du.ordered.same)
         {
           dith->encode=dither_ordered_encode_same;
           dith->u.ordered.xa=dith->u.ordered.xs-1;
           dith->u.ordered.ya=dith->u.ordered.ys-1;
         }
         else
           dith->encode=dither_ordered_encode;
	 dith->newline=dither_ordered_newline;

	 return 1;
   }
   Pike_error("Illegal dither method\n");
   return 0; /* uh */
}

void image_colortable_free_dither(struct nct_dither *dith)
{
   switch (dith->type)
   {
      case NCTD_NONE:
	 break;
      case NCTD_FLOYD_STEINBERG:
	 free(dith->u.floyd_steinberg.errors);
	 free(dith->u.floyd_steinberg.nexterrors);
	 break;
      case NCTD_RANDOMCUBE:
      case NCTD_RANDOMGREY:
	 break;
      case NCTD_ORDERED:
         free(dith->u.ordered.rdiff);
         free(dith->u.ordered.gdiff);
         free(dith->u.ordered.bdiff);
	 break;
   }
}

/***************** global methods ******************************/

/*
**! method void create()
**! method void create(array(array(int)) colors)
**! method void create(object(Image.Image) image,int number)
**! method void create(object(Image.Image) image,int number,array(array(int)) needed)
**! method void create(int r,int g,int b)
**! method void create(int r,int g,int b, array(int) from1,array(int) to1,int steps1, ..., array(int) fromn,array(int) ton,int stepsn)
**! method object add(array(array(int)) colors)
**! method object add(object(Image.Image) image,int number)
**! method object add(object(Image.Image) image,int number,array(array(int)) needed)
**! method object add(int r,int g,int b)
**! method object add(int r,int g,int b, array(int) from1,array(int) to1,int steps1, ..., array(int) fromn,array(int) ton,int stepsn)
**
**  developers note:
**  method object add|create(string "rgbrgbrgb...")
**!
**!	<ref>create</ref> initiates a colortable object. 
**!	Default is that no colors are in the colortable. 
**!
**!	<ref>add</ref> takes the same argument(s) as
**!	<ref>create</ref>, thus adding colors to the colortable.
**!
**!	The colortable is mostly a list of colors,
**!	or more advanced, colors and weight.
**!
**!	The colortable could also be a colorcube, with or 
**!	without additional scales. A colorcube is the by-far 
**!	fastest way to find colors. 
**!
**!	Example:
**!	<pre>
**!	ct=colortable(my_image,256); // the best 256 colors
**!	ct=colortable(my_image,256,({0,0,0})); // black and the best other 255
**!
**!	ct=colortable(({({0,0,0}),({255,255,255})})); // black and white
**!
**!	ct=colortable(6,7,6); // a colortable of 252 colors
**!	ct=colortable(7,7,5, ({0,0,0}),({255,255,255}),11); 
**!		// a colorcube of 245 colors, and a greyscale of the rest -> 256
**!	</pre>
**!	
**! arg array(array(int)) colors
**!	list of colors 
**! arg object(Image.Image) image
**!	source image 
**!
**!	note: you may not get all colors from image,
**!	max hash size is (probably, set by a <tt>#define</tt>) 
**!	32768 entries, giving
**!	maybe half that number of colors as maximum.
**! arg int number
**!	number of colors to get from the image
**!	
**!	0 (zero) gives all colors in the image.
**!
**!	Default value is 256.
**! arg array(array(int)) needed
**!	needed colors (to optimize selection of others to these given)
**!	
**!	this will add to the total number of colors (see argument 'number')
**! arg int r
**! arg int g
**! arg int b
**!	size of sides in the colorcube, must (of course) be equal
**!	or larger than 2 - if smaller, the cube is ignored (no colors).
**!	This could be used to have only scales (like a greyscale)
**!	in the output.
**! arg array(int) fromi
**! arg array(int) toi
**! arg int stepi
**!	This is to add the possibility of adding a scale
**!	of colors to the colorcube; for instance a grayscale
**!	using the arguments <tt>({0,0,0}),({255,255,255}),17</tt>,
**!	adding a scale from black to white in 17 or more steps.
**!
**!	Colors already in the cube is used again to add the number
**!	of steps, if possible. 
**!
**!	The total number of colors in the table is therefore
**!	<tt>r*b*g+step1+...+stepn</tt>.
*/

static void image_colortable_add(INT32 args)
{
   if (!args) 
   {
      pop_n_elems(args);
      ref_push_object(THISOBJ);
      return;
   }
   
   if (THIS->type!=NCT_NONE)
   {
      struct object *o;

      if (sp[-args].type==T_OBJECT)
      {
	 struct neo_colortable *ct2;
	 ct2=(struct neo_colortable*)
	    get_storage(sp[-args].u.object,image_colortable_program);
	 if (ct2)
	 {
#ifdef COLORTABLE_DEBUG
	    fprintf(stderr,"COLORTABLE added %lx and %lx to %lx (args=%d)\n",
		           THIS,ct2,THIS,args);
#endif
	    _img_add_colortable(THIS,ct2);
	    pop_n_elems(args);
	    ref_push_object(THISOBJ);
	    return;
	 }
      }

#ifdef COLORTABLE_DEBUG
      fprintf(stderr,
	      "COLORTABLE %lx isn't empty; create new and add (args=%d)\n",
	      THIS,args);
#endif
      o=clone_object(image_colortable_program,args);
      push_object(o);
      image_colortable_add(1);

#ifdef COLORTABLE_DEBUG
      fprintf(stderr,
	      "COLORTABLE done (%lx isn't empty...)\n",
	      THIS,args);
#endif
      return;
   }

   /* initiate */
#ifdef COLORTABLE_DEBUG
   fprintf(stderr,"COLORTABLE %lx created with %d args, sp-args=%lx\n",THIS,args,sp-args);
#endif

   if (sp[-args].type==T_OBJECT)
   {
      struct neo_colortable *ct2;
      struct image *img;

      if ((ct2=(struct neo_colortable*)
	   get_storage(sp[-args].u.object,image_colortable_program)))
      {
	 /* just copy that colortable */
	 _img_copy_colortable(THIS,ct2);
      }
      else if ((img=(struct image*)
		get_storage(sp[-args].u.object,image_program)))
      {
	 /* get colors from image */
	 if (args>=2) 
	    if (sp[1-args].type==T_INT)
	    {
	       int numcolors=sp[1-args].u.integer;
	       if (args>2)
	       {
		  struct object *o;
		  struct neo_colortable *nct;

		  o=clone_object(image_colortable_program,args-2);

		  nct=(struct neo_colortable*)
		     get_storage(o,image_colortable_program);

		  if (!nct) abort();

		  if (nct->type==NCT_CUBE)
		     nct->u.cube.weight=WEIGHT_NEEDED;
		  else if (nct->type==NCT_FLAT)
		  {
		     size_t i;
		     i = nct->u.flat.numentries;
		     while (i--)
			nct->u.flat.entries[i].weight=WEIGHT_NEEDED;
		  }

		  numcolors-=image_colortable_size(nct);
		  if (numcolors<0) numcolors=1;

		  THIS->u.flat=_img_get_flat_from_image(img,2500+numcolors);
		  THIS->type=NCT_FLAT;

		  push_object(o);
		  image_colortable_add(1);
		  pop_stack();
		  /* we will keep flat... */
		  args=2;

		  THIS->u.flat=
		     _img_reduce_number_of_colors(THIS->u.flat,
						  numcolors,
						  THIS->spacefactor);
	       }
	       else
	       {
		  THIS->u.flat=_img_get_flat_from_image(img,numcolors);
		  THIS->type=NCT_FLAT;
		  THIS->u.flat=
		     _img_reduce_number_of_colors(THIS->u.flat,
						  numcolors,
						  THIS->spacefactor);
	       }
	    }
	    else 
	       bad_arg_error("Image",sp-args,args,2,"",sp+2-1-args,
		"Bad argument 2 to Image()\n");
	 else
	 {
	    THIS->u.flat=_img_get_flat_from_image(img,256); 
	    if (THIS->u.flat.numentries>256)
	       THIS->u.flat=_img_reduce_number_of_colors(THIS->u.flat,256,
							 THIS->spacefactor);
	    THIS->type=NCT_FLAT;
	 }
      }
      else bad_arg_error("Image",sp-args,args,1,"",sp+1-1-args,
		"Bad argument 1 to Image()\n");
   }
   else if (sp[-args].type==T_ARRAY)
   {
      THIS->u.flat=_img_get_flat_from_array(sp[-args].u.array);
      THIS->type=NCT_FLAT;
   }
   else if (sp[-args].type==T_STRING)
   {
      if (args>1)
      {
	 if (sp[1-args].type!=T_INT)
	    SIMPLE_BAD_ARG_ERROR("Image.Colortable",2,"int");
	 switch (sp[1-args].u.integer)
	 {
	    case 0: /* rgb */
	       THIS->u.flat=_img_get_flat_from_string(sp[-args].u.string); 
	       break;
	    case 1: /* bgr */
	       THIS->u.flat=_img_get_flat_from_bgr_string(sp[-args].u.string); 
	       break;
	    case 2: /* bgrz */
	       THIS->u.flat=
		  _img_get_flat_from_bgrz_string(sp[-args].u.string); 
	       break;
	    default:
	       SIMPLE_BAD_ARG_ERROR("Image.Colortable",2,"int(0..2)");
	 }
      }
      else
	 THIS->u.flat=_img_get_flat_from_string(sp[-args].u.string);
      THIS->type=NCT_FLAT;
   }
   else if (sp[-args].type==T_INT)
   {
      THIS->u.cube=_img_get_cube_from_args(args);
      THIS->type=NCT_CUBE;
   }
   else bad_arg_error("Image",sp-args,args,0,"",sp-args,
		"Bad arguments to Image()\n");
   pop_n_elems(args);
   ref_push_object(THISOBJ);

#ifdef COLORTABLE_DEBUG
   fprintf(stderr,"COLORTABLE done (%lx created, %d args was left, sp-1=%lx)\n",THIS,args,sp-1);
#endif

}

void image_colortable_create(INT32 args)
{
   if (args)  /* optimize */
      image_colortable_add(args);
   else
      push_undefined();
}

/*
**! method object reduce(int colors)
**! method object reduce_fs(int colors)
**!	reduces the number of colors
**!
**!	All needed (see <ref>create</ref>) colors are kept.
**!
**!	<ref>reduce_fs</ref> creates and keeps 
**!	the outmost corners of the color space, to 
**!	improve floyd-steinberg dithering result.
**!	(It doesn't work very well, though.)
**!
**! returns the new <ref>Colortable</ref> object
**!
**! arg int colors
**!	target number of colors
**!
**! note
**!	this algorithm assumes all colors are different to 
**!     begin with (!)
**!	
**!	<ref>reduce_fs</ref> keeps the "corners" as
**!	"needed colors".
**!
**! see also: corners
**/

void image_colortable_reduce(INT32 args)
{
   struct object *o;
   struct neo_colortable *nct;
   int numcolors=0;

   if (args) 
     if (sp[-args].type!=T_INT) 
	SIMPLE_BAD_ARG_ERROR("Image.Colortable->reduce",1,"int");
     else
	 numcolors=sp[-args].u.integer;
   else
      numcolors=1293791; /* a lot */
   
   o=clone_object_from_object(THISOBJ,0);
   nct=(struct neo_colortable*)get_storage(o,image_colortable_program);
   
   switch (nct->type = THIS->type)
   {
      case NCT_NONE: pop_n_elems(args); push_object(o); return;
      case NCT_CUBE:
         nct->type=NCT_FLAT;
         nct->u.flat=_img_nct_cube_to_flat(THIS->u.cube);
	 break;
      case NCT_FLAT:
         _img_copy_colortable(nct,THIS);
	 break;
   }

   if (sp[-args].u.integer<1) sp[-args].u.integer=1;

   nct->u.flat=_img_reduce_number_of_colors(nct->u.flat,numcolors,
					    nct->spacefactor);

   pop_n_elems(args);
   push_object(o);
}

void image_colortable_corners(INT32 args);

void image_colortable_reduce_fs(INT32 args)
{
   int numcolors = 1293791;	/* a lot */
   int i;
   struct object *o;
   struct neo_colortable *nct;

   if (args) {
     if (sp[-args].type!=T_INT) 
	SIMPLE_BAD_ARG_ERROR("Image.Colortable->reduce",1,"int");
     else
	numcolors=sp[-args].u.integer;
   }

   if (numcolors<2)
      SIMPLE_BAD_ARG_ERROR("Image.Colortable->reduce",1,"int(2..)");

   pop_n_elems(args);
   image_colortable_corners(0);

   if (numcolors<8)
   {
      push_int(0);
      push_int(1);
      f_index(3);
   }
   push_object(o=clone_object(image_colortable_program,1));
   nct=(struct neo_colortable*)get_storage(o,image_colortable_program);
   
   for (i=0; i<nct->u.flat.numentries; i++)
      nct->u.flat.entries[i].weight=WEIGHT_NEEDED;

   image_colortable_add(1);
   pop_stack();
   push_int(numcolors);
   image_colortable_reduce(1);
}


/*
**! method object `+(object with, mixed ... more)
**!	sums colortables
**! returns the resulting new <ref>Colortable</ref> object
**!
**! arg object(<ref>Colortable</ref>) with
**!	<ref>Colortable</ref> object with colors to add
**/

void image_colortable_operator_plus(INT32 args)
{
   struct object *o,*tmpo=NULL;
   struct neo_colortable *dest,*src=NULL;

   int i;

   ref_push_object(THISOBJ);
   o=clone_object_from_object(THISOBJ,1);
   dest=(struct neo_colortable*)get_storage(o,image_colortable_program);

   for (i=0; i<args; i++)
   {
      if (sp[i-args].type==T_OBJECT &&
	  (src=(struct neo_colortable*)
	   get_storage(sp[i-args].u.object,image_colortable_program)))
      {
	 tmpo=NULL;
      }
      else if (sp[i-args].type==T_ARRAY ||
	       sp[i-args].type==T_OBJECT)
      {
	 struct svalue *sv=sp+i-args;
	 push_svalue(sv);
	 tmpo=clone_object(image_colortable_program,1);
	 src=(struct neo_colortable*)
	   get_storage(tmpo,image_colortable_program);
	 if (!src) abort();
      }
      else {
	bad_arg_error("Image-colortable->`+",sp-args,args,0,"",sp-args,
		"Bad arguments to Image-colortable->`+()\n");
	/* Not reached, but keeps the compiler happy. */
	src = NULL;
      }
    
      _img_add_colortable(dest,src);
      
      if (tmpo) free_object(tmpo);
   }
   pop_n_elems(args);
   push_object(o);
}

/*
**! method object `-(object with, mixed ... more)
**!	subtracts colortables
**! returns the resulting new <ref>Colortable</ref> object
**!
**! arg object(<ref>Colortable</ref>) with
**!	<ref>Colortable</ref> object with colors to subtract
**/

void image_colortable_operator_minus(INT32 args)
{
   struct object *o;
   struct neo_colortable *dest,*src=NULL;

   int i;

   ref_push_object(THISOBJ);
   o=clone_object_from_object(THISOBJ,1);
   dest=(struct neo_colortable*)get_storage(o,image_colortable_program);

   for (i=0; i<args; i++)
      if (sp[i-args].type==T_OBJECT)
      {
	 src=(struct neo_colortable*)
	    get_storage(sp[i-args].u.object,image_colortable_program);
	 if (!src) 
	 { 
	    free_object(o); 
	    bad_arg_error("Image",sp-args,args,i+2,"",sp+i+2-1-args,
		"Bad argument %d to Image()\n",i+2); 
	 }
	 _img_sub_colortable(dest,src);
      }
      else 
      { 
	 free_object(o); 
	 bad_arg_error("Image",sp-args,args,i+2,"",sp+i+2-1-args,
		"Bad argument %d to Image()\n",i+2); 
      }
   pop_n_elems(args);
   push_object(o);
}

void image_colortable_cast_to_array(struct neo_colortable *nct)
{
   struct nct_flat flat;
   int i;
   int n=0;
   
   if (nct->type==NCT_NONE)
   {
      f_aggregate(0);
      return;
   }

   if (nct->type==NCT_CUBE)
      flat=_img_nct_cube_to_flat(nct->u.cube);
   else
      flat=nct->u.flat;

   /* sort in number order? */

   for (i=0; i<flat.numentries; i++)
      if (flat.entries[i].no!=-1)
      {
	 _image_make_rgb_color(flat.entries[i].color.r,
			       flat.entries[i].color.g,
			       flat.entries[i].color.b);
	 n++;
      }
   f_aggregate(n);

   if (nct->type==NCT_CUBE)
      free(flat.entries);
}

void image_colortable_cast_to_mapping(struct neo_colortable *nct)
{
   struct nct_flat flat;
   int i,n=0;
   
   if (nct->type==NCT_NONE)
   {
      f_aggregate(0);
      return;
   }

   if (nct->type==NCT_CUBE)
      flat=_img_nct_cube_to_flat(nct->u.cube);
   else
      flat=nct->u.flat;

   for (i=0; i<flat.numentries; i++)
      if (flat.entries[i].no!=-1)
      {
	 push_int64(flat.entries[i].no);
	 _image_make_rgb_color(flat.entries[i].color.r,
			       flat.entries[i].color.g,
			       flat.entries[i].color.b);
	 n++;
      }

   f_aggregate_mapping(n*2);

   if (nct->type==NCT_CUBE)
      free(flat.entries);
}

ptrdiff_t image_colortable_size(struct neo_colortable *nct)
{
   if (nct->type==NCT_FLAT)
      return nct->u.flat.numentries;
   else if (nct->type==NCT_CUBE)
      return nct->u.cube.numentries;
   else return 0;
}

void image_colortable__sizeof(INT32 args)
{
   pop_n_elems(args);
   push_int64(image_colortable_size(THIS));
}

void image_colortable_write_rgb(struct neo_colortable *nct,
				unsigned char *dest)
{
   struct nct_flat flat;
   int i;
   
   if (nct->type==NCT_NONE)
      return;

   if (nct->type==NCT_CUBE)
      flat=_img_nct_cube_to_flat(nct->u.cube);
   else
      flat=nct->u.flat;

   /* sort in number order? */

   for (i=0; i<flat.numentries; i++)
   {
      *(dest++)=flat.entries[i].color.r;
      *(dest++)=flat.entries[i].color.g;
      *(dest++)=flat.entries[i].color.b;
   }

   if (nct->type==NCT_CUBE)
      free(flat.entries);
}

void image_colortable_write_rgbz(struct neo_colortable *nct,
				 unsigned char *dest)
{
   struct nct_flat flat;
   int i;
   
   if (nct->type==NCT_NONE)
      return;

   if (nct->type==NCT_CUBE)
      flat=_img_nct_cube_to_flat(nct->u.cube);
   else
      flat=nct->u.flat;

   /* sort in number order? */

   for (i=0; i<flat.numentries; i++)
   {
      *(dest++)=flat.entries[i].color.r;
      *(dest++)=flat.entries[i].color.g;
      *(dest++)=flat.entries[i].color.b;
      *(dest++)=0;
   }

   if (nct->type==NCT_CUBE)
      free(flat.entries);
}

void image_colortable_write_bgrz(struct neo_colortable *nct,
				 unsigned char *dest)
{
   struct nct_flat flat;
   int i;
   
   if (nct->type==NCT_NONE)
      return;

   if (nct->type==NCT_CUBE)
      flat=_img_nct_cube_to_flat(nct->u.cube);
   else
      flat=nct->u.flat;

   /* sort in number order? */

   for (i=0; i<flat.numentries; i++)
   {
      *(dest++)=flat.entries[i].color.b;
      *(dest++)=flat.entries[i].color.g;
      *(dest++)=flat.entries[i].color.r;
      *(dest++)=0;
   }

   if (nct->type==NCT_CUBE)
      free(flat.entries);
}

void image_colortable_cast_to_string(struct neo_colortable *nct)
{
   struct pike_string *str;
   str=begin_shared_string(image_colortable_size(nct)*3);
   image_colortable_write_rgb(nct,(unsigned char *)str->str);
   push_string(end_shared_string(str));
}

/*
**! method object cast(string to)
**!	cast the colortable to an array or mapping,
**!	the array consists of <ref>Image.Color</ref> objects
**!	and are not in index order. The mapping consists of
**!	index:<ref>Image.Color</ref> pairs, where index is 
**!	the index (int) of that color.
**!
**!	example: <tt>(mapping)Image.Colortable(img)</tt>
**!
**! arg string to
**!	must be "string", "array" or "mapping".
**/

void image_colortable_cast(INT32 args)
{
   if (!args)
      SIMPLE_TOO_FEW_ARGS_ERROR("Image.Colortable->cast",1);
   if (sp[-args].type==T_STRING||sp[-args].u.string->size_shift)
   {
      if (strncmp(sp[-args].u.string->str,"array",5)==0)
      {
	 pop_n_elems(args);
	 image_colortable_cast_to_array(THIS);
	 return;
      }
      if (strncmp(sp[-args].u.string->str,"string",6)==0)
      {
	 pop_n_elems(args);
	 image_colortable_cast_to_string(THIS);
	 return;
      }
      if (strncmp(sp[-args].u.string->str,"mapping",7)==0)
      {
	 pop_n_elems(args);
	 image_colortable_cast_to_mapping(THIS);
	 return;
      }
   }
   SIMPLE_BAD_ARG_ERROR("Image.Colortable->cast",1,
			"string(\"mapping\"|\"array\"|\"string\")");
}

/*
**! method object full()
**!	Set the colortable to use full scan to lookup the closest color.
**!
**!	example: <tt>colors=Image.Colortable(img)->full();</tt>
**!
**!     algorithm time: O[n*m], where n is numbers of colors 
**!	and m is number of pixels
**!
**! returns the object being called
**!
**! see also: cubicles, map
**! note
**!     Not applicable to colorcube types of colortable.
**/

void image_colortable_full(INT32 args)
{
   if (THIS->lookup_mode!=NCT_FULL) 
   {
      colortable_free_lookup_stuff(THIS);
      THIS->lookup_mode=NCT_FULL;
   }
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*
**! method object rigid()
**! method object rigid(int r,int g,int b)
**!	Set the colortable to use the "rigid" method of
**!     looking up colors. 
**!	
**!	This is a very simple way of finding the correct
**!     color. The algorithm initializes a cube with
**!	<tt>r</tt> x <tt>g</tt> x <tt>b</tt> colors,
**!	where every color is chosen by closest match to
**!	the corresponding coordinate.
**!	
**!	This format is not recommended for exact match,
**!	but may be usable when it comes to quickly
**!	view pictures on-screen.
**!
**!	It has a high init-cost and low use-cost.
**!	The structure is initiated at first usage.
**!
**! returns the object being called
**!
**! see also: cubicles, map, full
**! note
**!     Not applicable to colorcube types of colortable.
**/

void image_colortable_rigid(INT32 args)
{
   INT_TYPE r,g,b;

   if (args)
   {
      get_all_args("Image.Colortable->rigid()",args,"%i%i%i",&r,&g,&b);
   }
   else
   {
      r=RIGID_DEFAULT_R;
      g=RIGID_DEFAULT_G;
      b=RIGID_DEFAULT_B;
   }

   if (!(THIS->lookup_mode==NCT_RIGID &&
	 THIS->lu.rigid.r == r &&
	 THIS->lu.rigid.g == g &&
	 THIS->lu.rigid.b == b))
   {
      colortable_free_lookup_stuff(THIS);
      THIS->lookup_mode=NCT_RIGID;

      if (r<1) SIMPLE_BAD_ARG_ERROR("Image.Colortable->rigid",1,"integer >0");
      if (g<1) SIMPLE_BAD_ARG_ERROR("Image.Colortable->rigid",2,"integer >0");
      if (b<1) SIMPLE_BAD_ARG_ERROR("Image.Colortable->rigid",3,"integer >0");

      THIS->lu.rigid.r=r;
      THIS->lu.rigid.g=g;
      THIS->lu.rigid.b=b;
      THIS->lu.rigid.index=NULL;
   }
      
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*
**! method object cubicles()
**! method object cubicles(int r,int g,int b)
**! method object cubicles(int r,int g,int b,int accuracy)
**!	Set the colortable to use the cubicles algorithm to lookup
**!     the closest color. This is a mostly very fast and very
**!     accurate way to find the correct color, and the default
**!     algorithm.
**!
**!
**!	The colorspace is divided in small cubes, each cube
**!	containing the colors in that cube. Each cube then gets
**!	a list of the colors in the cube, and the closest from
**!	the corners and midpoints between corners.  
**!
**!	When a color is needed, the algorithm first finds the
**!	correct cube and then compares with all the colors in
**!	the list for that cube.
**!
**!	example: <tt>colors=Image.Colortable(img)->cubicles();</tt>
**!
**!     algorithm time: between O[m] and O[m * n], 
**!	where n is numbers of colors and m is number of pixels
**!
**!	The arguments can be heavy trimmed for the usage
**!	of your colortable; a large number (10×10×10 or bigger)
**!	of cubicles is recommended when you use the colortable
**!	repeatedly, since the calculation takes much
**!	more time than usage.
**!
**!	recommended values:
**!
**!     <pre>	
**!	image size  setup
**!	100×100	    cubicles(4,5,4) (default)
**!	1000×1000   cubicles(12,12,12) (factor 2 faster than default)
**!     </pre>	
**!
**!	In some cases, the <ref>full</ref> method is faster.
**!
**!	<table><tr valign=center>
**!	<td><illustration> return lena(); </illustration></td>
**!	<td><illustration> object c=Image.Colortable(lena(),16); return c*lena(); </illustration></td>
**!	<td><illustration> object c=Image.Colortable(lena(),16)->cubicles(4,5,4,200); return c*lena(); </illustration></td>
**!	</tr><tr valign=center>
**!	<td>original</td>
**!	<td>default cubicles,<br>16 colors</td>
**!	<td>accuracy=200</td>
**!	</tr></table>
**!
**! returns the object being called
**!
**! arg int r
**! arg int g
**! arg int b
**!     Size, ie how much the colorspace is divided.
**!     Note that the size of each cubicle is at least about 8b,
**!     and that it takes time to calculate them. The number of
**!     cubicles are <tt>r*g*b</tt>, and default is 4,5,4,
**!     ie 80 cubicles. This works good for 200±100 colors.
**!
**! arg int accuracy
**!	Accuracy when checking sides of cubicles.
**!	Default is 16. A value of 1 gives complete accuracy,
**!	ie cubicle() method gives exactly the same result
**!	as full(), but takes (in worst case) 16× the time
**!	to calculate.
**!
**! note
**!     this method doesn't figure out the cubicles, this is 
**!     done on the first use of the colortable.
**!
**!     Not applicable to colorcube types of colortable.
**/

void image_colortable_cubicles(INT32 args)
{
   colortable_free_lookup_stuff(THIS);

   if (args) 
      if (args>=3 && 
	  sp[-args].type==T_INT &&
	  sp[2-args].type==T_INT &&
	  sp[1-args].type==T_INT)
      {
	 THIS->lu.cubicles.r=MAXIMUM(sp[-args].u.integer,1);
	 THIS->lu.cubicles.g=MAXIMUM(sp[1-args].u.integer,1);
	 THIS->lu.cubicles.b=MAXIMUM(sp[2-args].u.integer,1);
	 if (args>=4 &&
	     sp[3-args].type==T_INT)
	    THIS->lu.cubicles.accur=MAXIMUM(sp[3-args].u.integer,1);
	 else
	    THIS->lu.cubicles.accur=CUBICLE_DEFAULT_ACCUR;
      }
      else
	 bad_arg_error("colortable->cubicles",sp-args,args,0,"",sp-args,
		"Bad arguments to colortable->cubicles()\n");
   else
   {
      THIS->lu.cubicles.r=CUBICLE_DEFAULT_R;
      THIS->lu.cubicles.g=CUBICLE_DEFAULT_G;
      THIS->lu.cubicles.b=CUBICLE_DEFAULT_B;
      THIS->lu.cubicles.accur=CUBICLE_DEFAULT_ACCUR;
   }

   THIS->lookup_mode=NCT_CUBICLES;

   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

static  int _cub_find_2cub_add(int *i,int *p,
			       int *p2,int n2,
			       struct nct_flat_entry *fe,
			       rgbl_group sf,
			       int r,int g,int b)
{
   int mindist=256*256*100; /* max dist is 256²*3 */
   int c=0;
   int *p1=p;
   int n=*i;
   int k=1;

   while (n--)
   {
      int dist=sf.r*SQ(fe[*p1].color.r-r)+
	       sf.g*SQ(fe[*p1].color.g-g)+
	       sf.b*SQ(fe[*p1].color.b-b);
      
      if (dist<mindist)
      {
	 c=*p1;
	 mindist=dist;
	 if (!dist) break;
      }

      p1++;
   }
   if (mindist) while (n2--)
   {
      int dist=sf.r*SQ(fe[*p2].color.r-r)+
	       sf.g*SQ(fe[*p2].color.g-g)+
	       sf.b*SQ(fe[*p2].color.b-b);
      
      if (dist<mindist)
      {
	 c=*p2;
	 k=0;
	 if (!dist) break;
	 mindist=dist;
      }

      p2++;
   }

   if (!k)
   {
      n=*i;
      while (n--)
	 if (*p==c) return c; else p++;

      *p=c;
      (*i)++;
   }

   return c;
}

static void _cub_add_cs_2cub_recur(int *i,int *p,
				   int *p2,int n2,
				   struct nct_flat_entry *fe,
				   int rp,int gp,int bp,
				   int rd1,int gd1,int bd1,
				   int rd2,int gd2,int bd2,
				   INT32 *a, INT32 *b,
				   INT32 *c, INT32 *d,
				   rgbl_group sf,
				   int accur)
{

/* a-h-b -> 2
 | |   |
 v e f g
 1 |   |
   c-j-d */

   INT32 e=-1,f=-1,g=-1,h=-1,j=-1;
   int rm1,gm1,bm1;
   int rm2,gm2,bm2;

   if (*a==-1) *a=_cub_find_2cub_add(i,p,p2,n2,fe,sf, rp,gp,bp); /* 0,0 */   
   if (*b==-1) *b=_cub_find_2cub_add(i,p,p2,n2,fe,sf,
                		     rp+rd2,gp+gd2,bp+bd2); /* 0,1 */
   if (*c==-1) *c=_cub_find_2cub_add(i,p,p2,n2,fe,sf,
                		     rp+rd1,gp+gd1,bp+bd1); /* 1,0 */
   if (*d==-1) *d=_cub_find_2cub_add(i,p,p2,n2,fe,sf,
				   rp+rd2+rd1,gp+gd2+gd1,bp+bd2+bd1); /* 1,1 */

   if (rd1+gd1+bd1<=accur && rd2+gd2+bd2<=accur) return;

   if (*a==*b) h=*a;
   if (*c==*d) j=*c;

   if (h!=-1 && h==j) return; /* all done */

   if (*a==*c) e=*a;
   if (*b==*d) g=*b;
   if (*a==*d) f=*a;
   if (*b==*c) f=*b;

   rm1=rd1-(rd1>>1); rd1>>=1;
   gm1=gd1-(gd1>>1); gd1>>=1;
   bm1=bd1-(bd1>>1); bd1>>=1;
   rm2=rd2-(rd2>>1); rd2>>=1;
   gm2=gd2-(gd2>>1); gd2>>=1;
   bm2=bd2-(bd2>>1); bd2>>=1;

   _cub_add_cs_2cub_recur(i,p,p2,n2,fe, rp,gp,bp, rd1,gd1,bd1, rd2,gd2,bd2, a,&h,&e,&f,sf,accur);
   _cub_add_cs_2cub_recur(i,p,p2,n2,fe, rp+rd2,gp+gd2,bp+bd2, rd2?rm1:rd1,gd2?gm1:gd1,bd2?bm1:bd1, rd2?rm2:rd2,gd2?gm2:gd2,bd2?bm2:bd2, &h,b,&f,&g,sf,accur);
   _cub_add_cs_2cub_recur(i,p,p2,n2,fe, rp+rd1,gp+gd1,bp+bd1, rd1?rm1:rd1,gd1?gm1:gd1,bd1?bm1:bd1, rd1?rm2:rd2,gd1?gm2:gd2,bd1?bm2:bd2, &e,&f,c,&j,sf,accur);
   _cub_add_cs_2cub_recur(i,p,p2,n2,fe, rp+rd2+rd1,gp+gd2+gd1,bp+bd2+bd1, rm1,gm1,bm1, rm2,gm2,bm2, &f,&g,&j,d,sf,accur);
}

static INLINE ptrdiff_t _cub_find_full_add(int **pp, int *i, int *p,
					   ptrdiff_t n,
					   struct nct_flat_entry *fe,
					   int r,int g,int b,
					   rgbl_group sf)
{
   int mindist=256*256*100; /* max dist is 256²*3 */
   int c = 0;

   while (n--)
      if (fe->no==-1) fe++;
      else
      {
	 int dist=
	    sf.r*SQ(fe->color.r-r)+
	    sf.g*SQ(fe->color.g-g)+
	    sf.b*SQ(fe->color.b-b);
      
	 if (dist<mindist)
	 {
	    c=fe->no;
	    if (!dist) break;
	    mindist=dist;
	 }
	 
	 fe++;
      }

   n=*i;
   while (n--)
      if (*p==c) return c; else p++;

   *p = c;
   (*i)++;
   (*pp)++;

   return c;
}

static void _cub_add_cs_full_recur(int **pp,int *i,int *p,
				   ptrdiff_t n, struct nct_flat_entry *fe,
				   int rp,int gp,int bp,
				   int rd1,int gd1,int bd1,
				   int rd2,int gd2,int bd2,
				   INT32 *a, INT32 *b,
				   INT32 *c, INT32 *d,
				   rgbl_group sf,
				   int accur)
{

/*
   a-h-b -> 2
 | |   |
 v e f g
 1 |   |
   c-i-d
 */

   INT32 e,f,g,h,j;
   int rm1,gm1,bm1;
   int rm2,gm2,bm2;

#ifdef CUBICLE_DEBUG
   fprintf(stderr," _cub_add_cs_full_recur #%02x%02x%02x, %d,%d,%d, %d,%d,%d %d,%d,%d,%d->",
	   /* rlvl,"", */
	   rp,gp,bp,rd1,gd1,bd1,rd2,gd2,bd2,*a,*b,*c,*d);
#endif

   if (*a==-1) *a=_cub_find_full_add(pp,i,p,n,fe,rp,gp,bp,sf); /* 0,0 */   
   if (*b==-1) *b=_cub_find_full_add(pp,i,p,n,fe,rp+rd2,gp+gd2,bp+bd2,sf); /* 0,1 */
   if (*c==-1) *c=_cub_find_full_add(pp,i,p,n,fe,rp+rd1,gp+gd1,bp+bd1,sf); /* 1,0 */
   if (*d==-1) *d=_cub_find_full_add(pp,i,p,n,fe,rp+rd2+rd1, gp+gd2+gd1,bp+bd2+bd1,sf); /* 1,1 */

#ifdef CUBICLE_DEBUG
   fprintf(stderr,"%d,%d,%d,%d\n",*a,*b,*c,*d);
#endif

   if (rd1+gd1+bd1<=accur && rd2+gd2+bd2<=accur) return;

   if (*a==*b) h=*a; else h=-1;
   if (*c==*d) j=*c; else j=-1;

   if (h!=-1 && h==j) return; /* all done */

   if (*a==*c) e=*a; else e=-1;
   if (*b==*d) g=*b; else g=-1;
   if (*a==*d) f=*a;
   else if (*b==*c) f=*b;
   else f=-1;

   rm1=rd1>>1; rd1-=rm1;
   gm1=gd1>>1; gd1-=gm1;
   bm1=bd1>>1; bd1-=bm1;
   rm2=rd2>>1; rd2-=rm2;
   gm2=gd2>>1; gd2-=gm2;
   bm2=bd2>>1; bd2-=bm2;
   
   _cub_add_cs_full_recur(pp,i,p,n,fe, rp,gp,bp, rd1,gd1,bd1, rd2,gd2,bd2, a,&h,&e,&f,sf,accur);
   _cub_add_cs_full_recur(pp,i,p,n,fe, rp+rd2,gp+gd2,bp+bd2, rd2?rm1:rd1,gd2?gm1:gd1,bd2?bm1:bd1, rd2?rm2:rd2,gd2?gm2:gd2,bd2?bm2:bd2, &h,b,&f,&g,sf,accur);
   _cub_add_cs_full_recur(pp,i,p,n,fe, rp+rd1,gp+gd1,bp+bd1, rd1?rm1:rd1,gd1?gm1:gd1,bd1?bm1:bd1, rd1?rm2:rd2,gd1?gm2:gd2,bd1?bm2:bd2, &e,&f,c,&j,sf,accur);
   _cub_add_cs_full_recur(pp,i,p,n,fe, rp+rd2+rd1,gp+gd2+gd1,bp+bd2+bd1, rm1,gm1,bm1, rm2,gm2,bm2, &f,&g,&j,d,sf,accur);
}

static INLINE void _cub_add_cs(struct neo_colortable *nct,
			       struct nctlu_cubicle *cub,
			       int **pp,int *i,int *p,
			       int ri,int gi,int bi,
			       int red,int green,int blue,
			       int rp,int gp,int bp,
			       int rd1,int gd1,int bd1,
			       int rd2,int gd2,int bd2)
{
   INT32 a=-1,b=-1,c=-1,d=-1;
#ifdef CUBICLE_DEBUG
   fprintf(stderr,
	   " _cub_add_cs %d,%d,%d %d,%d,%d, %d,%d,%d, %d,%d,%d, %d,%d,%d\n",
	   ri,gi,bi,red,green,blue,rp,gp,bp,rd1,gd1,bd1,rd2,gd2,bd2);
#endif

   if (ri<0||gi<0||bi<0||ri>=red||gi>=green||bi>=blue) 
      return; /* no, colorspace ends here */

#ifdef CUBICLE_DEBUG
   if (nct->lu.cubicles.cubicles[ri+gi*red+bi*red*green].index) 
      /* use the fact that the cube besides is known */
      _cub_add_cs_2cub_recur(i,p,
			     nct->lu.cubicles.cubicles[ri+gi*red+bi*red*green].index,
			     nct->lu.cubicles.cubicles[ri+gi*red+bi*red*green].n,
			     nct->u.flat.entries,
			     rp,gp,bp, rd1,gd1,bd1, rd2,gd2,bd2,
			     &a,&b,&c,&d,
			     nct->spacefactor,
			     nct->lu.cubicles.accur);
   else
#endif
      _cub_add_cs_full_recur(pp,i,p,
			     nct->u.flat.numentries,
			     nct->u.flat.entries,
			     rp,gp,bp, rd1,gd1,bd1, rd2,gd2,bd2,
			     &a,&b,&c,&d,
			     nct->spacefactor,
			     nct->lu.cubicles.accur);
}

static INLINE void _build_cubicle(struct neo_colortable *nct,
				  int r,int g,int b,
				  int red,int green,int blue,
				  struct nctlu_cubicle *cub)
{
   int rmin,rmax;
   int gmin,gmax;
   int bmin,bmax;

   struct nct_flat_entry *fe=nct->u.flat.entries;
   INT32 n = nct->u.flat.numentries;

   int i=0;
   int *p=xalloc(n*sizeof(struct nctlu_cubicle));
   int *pp; /* write */

   rmin=(r*256)/red;   rmax=((r+1)*256)/red-1;
   gmin=(g*256)/green; gmax=((g+1)*256)/green-1;
   bmin=(b*256)/blue;  bmax=((b+1)*256)/blue-1;

#ifdef CUBICLE_DEBUG
   fprintf(stderr,"build cubicle %d,%d,%d #%02x%02x%02x-#%02x%02x%02x...",
	   r,g,b,rmin,gmin,bmin,rmax-1,gmax-1,bmax-1);
#endif

   pp=p;

   while (n--)
      if (fe->no==-1) fe++;
      else 
      {
	 if ((int)fe->color.r>=rmin && (int)fe->color.r<=rmax &&
	     (int)fe->color.g>=gmin && (int)fe->color.g<=gmax &&
	     (int)fe->color.b>=bmin && (int)fe->color.b<=bmax)
	 {
	    *pp = DO_NOT_WARN((int)fe->no);
	    pp++; i++;
	 }
	 
	 fe++;
      }

   /* add closest to sides */
   _cub_add_cs(nct,cub,&pp,&i,p,r-1,g,b,red,green,blue,rmin,gmin,bmin,0,gmax-gmin,0,0,0,bmax-bmin);
   _cub_add_cs(nct,cub,&pp,&i,p,r,g-1,b,red,green,blue,rmin,gmin,bmin,rmax-rmin,0,0,0,0,bmax-bmin);
   _cub_add_cs(nct,cub,&pp,&i,p,r,g,b-1,red,green,blue,rmin,gmin,bmin,rmax-rmin,0,0,0,gmax-gmin,0);
   _cub_add_cs(nct,cub,&pp,&i,p,r+1,g,b,red,green,blue,rmax,gmin,bmin,0,gmax-gmin,0,0,0,bmax-bmin);
   _cub_add_cs(nct,cub,&pp,&i,p,r,g+1,b,red,green,blue,rmin,gmax,bmin,rmax-rmin,0,0,0,0,bmax-bmin);
   _cub_add_cs(nct,cub,&pp,&i,p,r,g,b+1,red,green,blue,rmin,gmin,bmax,rmax-rmin,0,0,0,gmax-gmin,0);

#ifdef CUBICLE_DEBUG
   fprintf(stderr," size=%d\n",i);

   do
   {
      int j;
      for (j=0; j<i; j++)
	 fprintf(stderr,"#%02x%02x%02x,",
		 nct->u.flat.entries[p[j]].color.r,
		 nct->u.flat.entries[p[j]].color.g,
		 nct->u.flat.entries[p[j]].color.b);
      fprintf(stderr,"\n");
   } 
   while (0);
#endif

   cub->n=i;
   cub->index=realloc(p,i*sizeof(struct nctlu_cubicle));

   if (!cub->index) 
      cub->index=p; /* out of memory, or weird */
}

void build_rigid(struct neo_colortable *nct)
{
   int *dist,*ddist;
   int *index,*dindex;
   int r=nct->lu.rigid.r;
   int g=nct->lu.rigid.g;
   int b=nct->lu.rigid.b;
   int i,ri,gi,bi;
   int rc,gc,bc;
   int di,hdi,hhdi;

   if (nct->lu.rigid.index) Pike_fatal("rigid is initialized twice.\n");

   index=malloc(sizeof(int)*r*g*b);
   dist=malloc(sizeof(int)*r*g*b);

   if (!index||!dist)
   {
      if (index) free(index);
      if (dist) free(dist);
      resource_error(NULL,0,0,"memory",r*g*b*sizeof(int),"Out of memory.\n");
   }

   for (i=0; i<nct->u.flat.numentries; i++)
   {
      rc=nct->u.flat.entries[i].color.r;
      gc=nct->u.flat.entries[i].color.g;
      bc=nct->u.flat.entries[i].color.b;

      ddist=dist;
      dindex=index;
      for (bi=0; bi<b; bi++)
      {
	 hhdi=(bc-bi*COLORMAX/b)*(bc-bi*COLORMAX/b);
	 for (gi=0; gi<g; gi++)
	 {
	    hdi=hhdi+(gc-gi*COLORMAX/g)*(gc-gi*COLORMAX/g);
	    if (i==0)
	       for (ri=0; ri<r; ri++)
	       {
		  *(ddist++)=di=hdi+(rc-ri*COLORMAX/r)*(rc-ri*COLORMAX/r);
		  *(dindex++)=0;
	       }
	    else
	       for (ri=0; ri<r; ri++)
	       {
		  di=hdi+(rc-ri*COLORMAX/r)*(rc-ri*COLORMAX/r);
		  if (*ddist>di) 
		  {
		     *(ddist++)=di;
		     *(dindex++)=i;
		  }
		  else
		  {
		     dindex++;
		     ddist++;
		  }
	       }
	 }
      }
   }

   nct->lu.rigid.index=index;
   free(dist);
}

/*
**! method object map(object image)
**! method object `*(object image)
**! method object ``*(object image)
**! method object map(string data,int xsize,int ysize)
**! method object `*(string data,int xsize,int ysize)
**! method object ``*(string data,int xsize,int ysize)
**!	Map colors in an image object to the colors in 
**!     the colortable, and creates a new image with the
**!     closest colors. 
**!
**!	<table><tr valign=center>
**!	<td></td>
**!	<td><illustration> object c=Image.Colortable(lena(),2); return c*lena(); </illustration></td>
**!	<td><illustration> object c=Image.Colortable(lena(),4); return c*lena(); </illustration></td>
**!	<td><illustration> object c=Image.Colortable(lena(),8); return c*lena(); </illustration></td>
**!	<td><illustration> object c=Image.Colortable(lena(),16); return c*lena(); </illustration></td>
**!	<td><illustration> object c=Image.Colortable(lena(),32); return c*lena(); </illustration></td>
**!	<td>no dither</td>
**!	</tr><tr valign=center>
**!	<td></td>
**!	<td><illustration> object c=Image.Colortable(lena(),2)->floyd_steinberg(); return c*lena(); </illustration></td>
**!	<td><illustration> object c=Image.Colortable(lena(),4)->floyd_steinberg(); return c*lena(); </illustration></td>
**!	<td><illustration> object c=Image.Colortable(lena(),8)->floyd_steinberg(); return c*lena(); </illustration></td>
**!	<td><illustration> object c=Image.Colortable(lena(),16)->floyd_steinberg(); return c*lena(); </illustration></td>
**!	<td><illustration> object c=Image.Colortable(lena(),32)->floyd_steinberg(); return c*lena(); </illustration></td>
**!	<td><ref>floyd_steinberg</ref> dither</td>
**!	</tr><tr valign=center>
**!	<td></td>
**!	<td><illustration> object c=Image.Colortable(lena(),2)->ordered(60,60,60); return c*lena(); </illustration></td>
**!	<td><illustration> object c=Image.Colortable(lena(),4)->ordered(45,45,45); return c*lena(); </illustration></td>
**!	<td><illustration> object c=Image.Colortable(lena(),8)->ordered(40,40,40); return c*lena(); </illustration></td>
**!	<td><illustration> object c=Image.Colortable(lena(),16)->ordered(40,40,40); return c*lena(); </illustration></td>
**!	<td><illustration> object c=Image.Colortable(lena(),32)->ordered(15,15,15); return c*lena(); </illustration></td>
**!	<td><ref>ordered</ref> dither</td>
**!	</tr><tr valign=center>
**!	<td><illustration> return lena(); </illustration></td>
**!	<td><illustration> object c=Image.Colortable(lena(),2)->randomcube(60,60,60); return c*lena(); </illustration></td>
**!	<td><illustration> object c=Image.Colortable(lena(),4)->randomcube(45,45,45); return c*lena(); </illustration></td>
**!	<td><illustration> object c=Image.Colortable(lena(),8)->randomcube(40,40,40); return c*lena(); </illustration></td>
**!	<td><illustration> object c=Image.Colortable(lena(),16)->randomcube(40,40,40); return c*lena(); </illustration></td>
**!	<td><illustration> object c=Image.Colortable(lena(),32)->randomcube(15,15,15); return c*lena(); </illustration></td>
**!	<td><ref>randomcube</ref> dither</td>
**!	</tr><tr valign=center>
**!	<td>original</td>
**!	<td>2</td>
**!	<td>4</td>
**!	<td>8</td>
**!	<td>16</td>
**!	<td>32 colors</td>
**!	</tr></table>
**!
**! returns a new image object
**!
**! note
**!     Flat (not cube) colortable and not '<ref>full</ref>' method: 
**!     this method does figure out the data needed for
**!	the lookup method, which may take time the first
**!	use of the colortable - the second use is quicker.
**!
**! see also: cubicles, full
**/

/* Some functions to avoid warnings about losss of precision. */
#ifdef __ECL
static inline unsigned char TO_UCHAR(ptrdiff_t val)
{
  return DO_NOT_WARN((unsigned char)val);
}
static inline unsigned short TO_USHORT(ptrdiff_t val)
{
  return DO_NOT_WARN((unsigned short)val);
}
static inline unsigned INT32 TO_UINT32(ptrdiff_t val)
{
  return DO_NOT_WARN((unsigned INT32)val);
}
#else /* !__ECL */
#define TO_UCHAR(x)	((unsigned char)x)
#define TO_USHORT(x)	((unsigned short)x)
#define TO_UINT32(x)	((unsigned INT32)x)
#endif /* __ECL */

/* begin instantiating from colortable_lookup.h */
/* instantiate map functions */

#define NCTLU_DESTINATION rgb_group
#define NCTLU_CACHE_HIT_WRITE *d=lc->dest;
#define NCTLU_DITHER_GOT *d
#define NCTLU_FLAT_CUBICLES_NAME _img_nct_map_to_flat_cubicles
#define NCTLU_FLAT_FULL_NAME _img_nct_map_to_flat_full
#define NCTLU_CUBE_NAME _img_nct_map_to_cube
#define NCTLU_FLAT_RIGID_NAME _img_nct_map_to_flat_rigid
#define NCTLU_LINE_ARGS (dith,&rowpos,&s,&d,NULL,NULL,NULL,&cd)
#define NCTLU_RIGID_WRITE (d[0]=feprim[i].color)
#define NCTLU_DITHER_RIGID_GOT (*d)
#define NCTLU_SELECT_FUNCTION image_colortable_map_function
#define NCTLU_EXECUTE_FUNCTION image_colortable_map_image

#define NCTLU_CUBE_FAST_WRITE(SRC) \
            d->r=((int)(((int)((SRC)->r*red+hred)>>8)*redf)); \
	    d->g=((int)(((int)((SRC)->g*green+hgreen)>>8)*greenf)); \
	    d->b=((int)(((int)((SRC)->b*blue+hblue)>>8)*bluef));

#define NCTLU_CUBE_FAST_WRITE_DITHER_GOT(SRC) \
            dither_got(dith,rowpos,*s,*d)

#include "colortable_lookup.h"

#undef NCTLU_DESTINATION
#undef NCTLU_CACHE_HIT_WRITE
#undef NCTLU_DITHER_GOT
#undef NCTLU_FLAT_CUBICLES_NAME
#undef NCTLU_FLAT_FULL_NAME 
#undef NCTLU_CUBE_NAME 
#undef NCTLU_LINE_ARGS 
#undef NCTLU_CUBE_FAST_WRITE
#undef NCTLU_CUBE_FAST_WRITE_DITHER_GOT
#undef NCTLU_RIGID_WRITE
#undef NCTLU_FLAT_RIGID_NAME
#undef NCTLU_DITHER_RIGID_GOT
#undef NCTLU_SELECT_FUNCTION
#undef NCTLU_EXECUTE_FUNCTION

/* instantiate 8bit functions */

#define NCTLU_DESTINATION unsigned char
#define NCTLU_CACHE_HIT_WRITE *d=((unsigned char)(lc->index))
#define NCTLU_DITHER_GOT lc->dest
#define NCTLU_FLAT_CUBICLES_NAME _img_nct_index_8bit_flat_cubicles
#define NCTLU_FLAT_FULL_NAME _img_nct_index_8bit_flat_full
#define NCTLU_CUBE_NAME _img_nct_index_8bit_cube
#define NCTLU_FLAT_RIGID_NAME _img_nct_index_8bit_flat_rigid
#define NCTLU_LINE_ARGS (dith,&rowpos,&s,NULL,&d,NULL,NULL,&cd)
#define NCTLU_RIGID_WRITE (d[0] = TO_UCHAR(feprim[i].no))
#define NCTLU_DITHER_RIGID_GOT (feprim[i].color)
#define NCTLU_SELECT_FUNCTION image_colortable_index_8bit_function
#define NCTLU_EXECUTE_FUNCTION image_colortable_index_8bit_image

#define NCTLU_CUBE_FAST_WRITE(SRC) \
   *d=(unsigned char) \
      ((int)((SRC)->r*red+hred)>>8)+ \
      (((int)((SRC)->g*green+hgreen)>>8)+ \
       ((int)((SRC)->b*blue+hblue)>>8)*green)*red;

#define NCTLU_CUBE_FAST_WRITE_DITHER_GOT(SRC) \
   do \
   { \
      rgb_group tmp; \
      tmp.r=((int)((((SRC)->r*red+hred)>>8)*redf)); \
      tmp.g=((int)((((SRC)->g*green+hgreen)>>8)*greenf)); \
      tmp.b=((int)((((SRC)->b*blue+hblue)>>8)*bluef)); \
      dither_got(dith,rowpos,*s,tmp); \
   } while (0)

#include "colortable_lookup.h"

#undef NCTLU_DESTINATION
#undef NCTLU_CACHE_HIT_WRITE
#undef NCTLU_DITHER_GOT
#undef NCTLU_FLAT_CUBICLES_NAME
#undef NCTLU_FLAT_FULL_NAME 
#undef NCTLU_CUBE_NAME 
#undef NCTLU_LINE_ARGS 
#undef NCTLU_CUBE_FAST_WRITE
#undef NCTLU_CUBE_FAST_WRITE_DITHER_GOT
#undef NCTLU_RIGID_WRITE
#undef NCTLU_FLAT_RIGID_NAME
#undef NCTLU_DITHER_RIGID_GOT
#undef NCTLU_SELECT_FUNCTION
#undef NCTLU_EXECUTE_FUNCTION

/* instantiate 16bit functions */

#define NCTLU_DESTINATION unsigned short
#define NCTLU_CACHE_HIT_WRITE *d=((unsigned short)(lc->index))
#define NCTLU_DITHER_GOT lc->dest
#define NCTLU_FLAT_CUBICLES_NAME _img_nct_index_16bit_flat_cubicles
#define NCTLU_FLAT_FULL_NAME _img_nct_index_16bit_flat_full
#define NCTLU_CUBE_NAME _img_nct_index_16bit_cube
#define NCTLU_FLAT_RIGID_NAME _img_nct_index_16bit_flat_rigid
#define NCTLU_LINE_ARGS (dith,&rowpos,&s,NULL,NULL,&d,NULL,&cd)
#define NCTLU_RIGID_WRITE (d[0] = TO_USHORT(feprim[i].no))
#define NCTLU_DITHER_RIGID_GOT (feprim[i].color)
#define NCTLU_SELECT_FUNCTION image_colortable_index_16bit_function
#define NCTLU_EXECUTE_FUNCTION image_colortable_index_16bit_image

#define NCTLU_CUBE_FAST_WRITE(SRC) \
   *d=(unsigned short) \
      ((int)((SRC)->r*red+hred)>>8)+ \
      (((int)((SRC)->g*green+hgreen)>>8)+ \
       ((int)((SRC)->b*blue+hblue)>>8)*green)*red;

#define NCTLU_CUBE_FAST_WRITE_DITHER_GOT(SRC) \
   do \
   { \
      rgb_group tmp; \
      tmp.r=((int)((((SRC)->r*red+hred)>>8)*redf)); \
      tmp.g=((int)((((SRC)->g*green+hgreen)>>8)*greenf)); \
      tmp.b=((int)((((SRC)->b*blue+hblue)>>8)*bluef)); \
      dither_got(dith,rowpos,*s,tmp); \
   } while (0)

#include "colortable_lookup.h"

#undef NCTLU_DESTINATION
#undef NCTLU_CACHE_HIT_WRITE
#undef NCTLU_DITHER_GOT
#undef NCTLU_FLAT_CUBICLES_NAME
#undef NCTLU_FLAT_FULL_NAME 
#undef NCTLU_CUBE_NAME 
#undef NCTLU_LINE_ARGS 
#undef NCTLU_CUBE_FAST_WRITE
#undef NCTLU_CUBE_FAST_WRITE_DITHER_GOT
#undef NCTLU_RIGID_WRITE
#undef NCTLU_FLAT_RIGID_NAME
#undef NCTLU_DITHER_RIGID_GOT
#undef NCTLU_SELECT_FUNCTION
#undef NCTLU_EXECUTE_FUNCTION

/* instantiate 32bit functions */

#define NCTLU_DESTINATION unsigned INT32
#define NCTLU_CACHE_HIT_WRITE *d=((unsigned INT32)(lc->index))
#define NCTLU_DITHER_GOT lc->dest
#define NCTLU_FLAT_CUBICLES_NAME _img_nct_index_32bit_flat_cubicles
#define NCTLU_FLAT_FULL_NAME _img_nct_index_32bit_flat_full
#define NCTLU_CUBE_NAME _img_nct_index_32bit_cube
#define NCTLU_FLAT_RIGID_NAME _img_nct_index_32bit_flat_rigid
#define NCTLU_LINE_ARGS (dith,&rowpos,&s,NULL,NULL,NULL,&d,&cd)
#define NCTLU_RIGID_WRITE (d[0] = TO_UINT32(feprim[i].no))
#define NCTLU_DITHER_RIGID_GOT (feprim[i].color)
#define NCTLU_SELECT_FUNCTION image_colortable_index_32bit_function
#define NCTLU_EXECUTE_FUNCTION image_colortable_index_32bit_image

#define NCTLU_CUBE_FAST_WRITE(SRC) \
   *d=(unsigned INT32) \
      ((int)((SRC)->r*red+hred)>>8)+ \
      (((int)((SRC)->g*green+hgreen)>>8)+ \
       ((int)((SRC)->b*blue+hblue)>>8)*green)*red;

#define NCTLU_CUBE_FAST_WRITE_DITHER_GOT(SRC) \
   do \
   { \
      rgb_group tmp; \
      tmp.r=((int)((((SRC)->r*red+hred)>>8)*redf)); \
      tmp.g=((int)((((SRC)->g*green+hgreen)>>8)*greenf)); \
      tmp.b=((int)((((SRC)->b*blue+hblue)>>8)*bluef)); \
      dither_got(dith,rowpos,*s,tmp); \
   } while (0)

#include "colortable_lookup.h"

#undef NCTLU_DESTINATION
#undef NCTLU_CACHE_HIT_WRITE
#undef NCTLU_DITHER_GOT
#undef NCTLU_FLAT_CUBICLES_NAME
#undef NCTLU_FLAT_FULL_NAME 
#undef NCTLU_CUBE_NAME 
#undef NCTLU_LINE_ARGS 
#undef NCTLU_CUBE_FAST_WRITE
#undef NCTLU_CUBE_FAST_WRITE_DITHER_GOT
#undef NCTLU_RIGID_WRITE
#undef NCTLU_FLAT_RIGID_NAME
#undef NCTLU_DITHER_RIGID_GOT
#undef NCTLU_SELECT_FUNCTION
#undef NCTLU_EXECUTE_FUNCTION

/* done instantiating from colortable_lookup.h */

void image_colortable_map(INT32 args)
{
   struct image *src=NULL;
   struct image *dest;
   struct object *o;

   if (args<1)
      SIMPLE_TOO_FEW_ARGS_ERROR("colortable->map",1);

   if (sp[-args].type==T_STRING)
   {
      struct object *o;
      struct pike_string *ps=sp[-args].u.string;
      struct image *img;
      ptrdiff_t n;
      struct neo_colortable *nct=THIS;
      rgb_group *d;

      if (args!=3) 
	 Pike_error("illegal number of arguments to colortable->map()\n");
      o=clone_object(image_program,2);
      img=(struct image*)get_storage(o,image_program);
      d=img->img;
      
      n=img->xsize*img->ysize;
      if (n>ps->len) n=ps->len;

      switch (ps->size_shift)
      {
	 case 0:
	 {
	    p_wchar0 *s=(p_wchar0*)ps->str;
	    
	    while (n--)
	    {
	       if (*s<nct->u.flat.numentries)
		  *(d++)=nct->u.flat.entries[*s].color;
	       else 
		  d++; /* it's black already, and this is illegal */
	       s++;
	    }
 	    break;
	 }
	 case 1:
	 {
	    p_wchar1 *s=(p_wchar1*)ps->str;
	    
	    while (n--)
	    {
	       if (*s < nct->u.flat.numentries)
		  *(d++)=nct->u.flat.entries[*s].color;
	       else 
		  d++; /* it's black already, and this is illegal */
	       s++;
	    }
 	    break;
	 }
	 case 2:
	 {
	    p_wchar2 *s = (p_wchar2*)ps->str;
	    
	    while (n--)
	    {
	       if ((size_t)*s < (size_t)nct->u.flat.numentries)
		  *(d++)=nct->u.flat.entries[*s].color;
	       else 
		  d++; /* it's black already, and this is illegal */
	       s++;
	    }
 	    break;
	 }
      }
      
      pop_stack(); /* pops the given string */
      push_object(o); /* pushes the image object */

      return;
   }

   if (sp[-args].type!=T_OBJECT ||
       ! (src=(struct image*)get_storage(sp[-args].u.object,image_program)))
      bad_arg_error("colortable->map",sp-args,args,1,"",sp+1-1-args,
		"Bad argument 1 to colortable->map()\n");

   if (!src->img) 
      Pike_error("Called Image.Image object is not initialized\n");;

   o=clone_object(image_program,0);
   dest=(struct image*)(o->storage);
   *dest=*src;

   dest->img=malloc(sizeof(rgb_group)*src->xsize*src->ysize +1);
   if (!dest->img)
   {
      free_object(o);
      resource_error(NULL,0,0,"memory",0,"Out of memory.\n");
   }

   if (!image_colortable_map_image(THIS,src->img,dest->img,
				   src->xsize*src->ysize,src->xsize))
   {
      free_object(o);
      Pike_error("colortable->map(): called colortable is not initiated\n");
   }

   pop_n_elems(args);
   push_object(o);
}

void image_colortable_index_32bit(INT32 args)
{
   struct image *src=NULL;
   struct pike_string *ps;

   if (args<1)
      SIMPLE_TOO_FEW_ARGS_ERROR("Colortable.index",1);
   if (sp[-args].type!=T_OBJECT ||
       ! (src=(struct image*)get_storage(sp[-args].u.object,image_program)))
      SIMPLE_BAD_ARG_ERROR("Colortable.index",1,"image object");

   if (!src->img) 
      SIMPLE_BAD_ARG_ERROR("Colortable.index",1,"non-empty image object");

   if (sizeof(unsigned INT32)!=4)
      Pike_fatal("INT32 isn't 32 bits (sizeof is %ld)\n",
	    (long)TO_UINT32(sizeof(unsigned INT32)));

   ps=begin_wide_shared_string(src->xsize*src->ysize,2);

   if (!image_colortable_index_32bit_image(THIS,src->img,
					   (unsigned INT32 *)ps->str,
					   src->xsize*src->ysize,src->xsize))
   {
      free_string(end_shared_string(ps));
      SIMPLE_BAD_ARG_ERROR("Colortable.index",1,"non-empty image object");
      return;
   }

   pop_n_elems(args);

   push_string(end_shared_string(ps));
}

/*
**! method object spacefactors(int r,int g,int b)
**!	Colortable tuning option, this sets the color space
**!	distance factors. This is used when comparing distances
**!	in the colorspace and comparing grey levels.
**!
**!	Default factors are 3, 4 and 1; blue is much 
**!	darker than green. Compare with <ref>Image.Image->grey</ref>().
**!
**! returns the object being called
**!
**! note
**!	This has no sanity check. Some functions may bug
**!	if the factors are to high - color reduction functions 
**!	sums grey levels in the image, this could exceed maxint
**!	in the case of high factors. Negative values may 
**!	also cause strange effects. *grin*
**/

void image_colortable_spacefactors(INT32 args)
{
   if (args<3)
      SIMPLE_TOO_FEW_ARGS_ERROR("colortable->spacefactors",1);

   if (sp[0-args].type!=T_INT ||
       sp[1-args].type!=T_INT ||
       sp[2-args].type!=T_INT)
      bad_arg_error("colortable->spacefactors",sp-args,args,0,"",sp-args,
		"Bad arguments to colortable->spacefactors()\n");

   THIS->spacefactor.r=sp[0-args].u.integer;
   THIS->spacefactor.g=sp[1-args].u.integer;
   THIS->spacefactor.b=sp[2-args].u.integer;

   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*
**! method object floyd_steinberg()
**! method object floyd_steinberg(int bidir,int|float forward,int|float downforward,int|float down,int|float downback,int|float factor)
**!	Set dithering method to floyd_steinberg.
**!	
**!	The arguments to this method is for fine-tuning of the 
**!	algorithm (for computer graphics wizards). 
**!
**!	<table><tr valign=center>
**!	<td><illustration> return lena(); </illustration></td>
**!	<td><illustration> object c=Image.Colortable(4,4,4)->floyd_steinberg(); return c*lena(); </illustration></td>
**!	<td><illustration> object c=Image.Colortable(lena(),16)->floyd_steinberg(); return c*lena(); </illustration></td>
**!	</tr><tr valign=center>
**!	<td>original</td>
**!	<td>floyd_steinberg to a 4×4×4 colorcube</td>
**!	<td>floyd_steinberg to 16 chosen colors</td>
**!	</tr></table>
**!
**! arg int bidir
**!	Set algorithm direction of forward.
**!	-1 is backward, 1 is forward, 0 for toggle of direction
**!	each line (default).
**! arg int|float forward
**! arg int|float downforward
**! arg int|float down
**! arg int|float downback
**!	Set error correction directions. Default is 
**!	forward=7, downforward=1, down=5, downback=3.
**! arg int|float factor
**!     Error keeping factor. 
**!	Error will increase if more than 1.0 and decrease if less than 1.0.
**!	A value of 0.0 will cancel any dither effects.
**!     Default is 0.95.
**! returns the object being called
**/

void image_colortable_floyd_steinberg(INT32 args)
{
   double forward=7.0,downforward=1.0,down=5.0,downback=3.0,sum;
   double factor=0.95;
   THIS->dither_type=NCTD_NONE;

   if (args>=1) 
      if (sp[-args].type!=T_INT) 
	 bad_arg_error("colortable->spacefactors",sp-args,args,0,"",sp-args,
		"Bad arguments to colortable->spacefactors()\n");
      else 
	 THIS->du.floyd_steinberg.dir=sp[-args].u.integer;
   else 
      THIS->du.floyd_steinberg.dir=0;
   if (args>=6) {
      if (sp[5-args].type==T_FLOAT)
	 factor = sp[5-args].u.float_number;
      else if (sp[5-args].type==T_INT)
	 factor = (double)sp[5-args].u.integer;
      else
	 bad_arg_error("colortable->spacefactors",sp-args,args,0,"",sp-args,
		"Bad arguments to colortable->spacefactors()\n");
   }
   if (args>=5)
   {
      if (sp[1-args].type==T_FLOAT)
	 forward = sp[1-args].u.float_number;
      else if (sp[1-args].type==T_INT)
	 forward = (double)sp[1-args].u.integer;
      else
	 bad_arg_error("colortable->spacefactors",sp-args,args,0,"",sp-args,
		"Bad arguments to colortable->spacefactors()\n");
      if (sp[2-args].type==T_FLOAT)
	 downforward = sp[2-args].u.float_number;
      else if (sp[2-args].type==T_INT)
	 downforward = (double)sp[2-args].u.integer;
      else
	 bad_arg_error("colortable->spacefactors",sp-args,args,0,"",sp-args,
		"Bad arguments to colortable->spacefactors()\n");
      if (sp[3-args].type==T_FLOAT)
	 down = sp[3-args].u.float_number;
      else if (sp[3-args].type==T_INT)
	 down = (double)sp[3-args].u.integer;
      else
	 bad_arg_error("colortable->spacefactors",sp-args,args,0,"",sp-args,
		"Bad arguments to colortable->spacefactors()\n");
      if (sp[4-args].type==T_FLOAT)
	 downback = sp[4-args].u.float_number;
      else if (sp[4-args].type==T_INT)
	 downback = (double)sp[4-args].u.integer;
      else
	 bad_arg_error("colortable->spacefactors",sp-args,args,0,"",sp-args,
		"Bad arguments to colortable->spacefactors()\n");
   }

   sum=forward+downforward+down+downback;
   if (fabs(sum)<1e-10) sum=1.0;
   sum/=factor;

   THIS->du.floyd_steinberg.forward = DO_NOT_WARN((float)(forward/sum));
   THIS->du.floyd_steinberg.downforward = DO_NOT_WARN((float)(downforward/sum));
   THIS->du.floyd_steinberg.down = DO_NOT_WARN((float)(down/sum));
   THIS->du.floyd_steinberg.downback = DO_NOT_WARN((float)(downback/sum));

   THIS->dither_type = NCTD_FLOYD_STEINBERG;

   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/* called by GIF encoder */
void image_colortable_internal_floyd_steinberg(struct neo_colortable *nct)
{
   nct->du.floyd_steinberg.forward = DO_NOT_WARN((float)(0.95*(7.0/16)));
   nct->du.floyd_steinberg.downforward = DO_NOT_WARN((float)(0.95*(1.0/16)));
   nct->du.floyd_steinberg.down = DO_NOT_WARN((float)(0.95*(5.0/16)));
   nct->du.floyd_steinberg.downback = DO_NOT_WARN((float)(0.95*(3.0/16)));

   nct->dither_type=NCTD_FLOYD_STEINBERG;
}

/*
**! method object nodither()
**!	Set no dithering (default).
**!
**! returns the object being called
**/

void image_colortable_nodither(INT32 args)
{
   THIS->dither_type=NCTD_NONE;
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*
**! method object randomcube()
**! method object randomcube(int r,int g,int b)
**! method object randomgrey()
**! method object randomgrey(int err)
**!	Set random cube dithering.
**!	Color choosen is the closest one to color in picture
**!	plus (flat) random error; <tt>color±random(error)</tt>.
**!
**!	The randomgrey method uses the same random error on red, green
**!	and blue and the randomcube method has three random errors.
**!
**!	<table><tr valign=center>
**!	<td><illustration> return lena(); </illustration></td>
**!	<td><illustration> object c=Image.Colortable(4,4,4)->randomcube(); return c*lena(); </illustration></td>
**!	<td><illustration> object c=Image.Colortable(4,4,4)->randomgrey(); return c*lena(); </illustration></td>
**!	</tr><tr valign=top>
**!	<td>original</td>
**!	<td colspan=2>mapped to <br><tt>Image.Colortable(4,4,4)-></tt></td>
**!	</tr><tr valign=top>
**!	<td></td>
**!	<td>randomcube()</td>
**!	<td>randomgrey()</td>
**!	</tr><tr valign=top>
**!	<td><illustration> 
**!		object i=Image.Image(lena()->xsize(),lena()->ysize());
**!		i->tuned_box(0,0,i->xsize()-1,i->ysize()-1,
**!		     ({({0,0,0}),({0,0,0}),({255,255,255}),({255,255,255})})); 
**!		return i; 
**!	</illustration></td>
**!	<td><illustration> 
**!		object i=Image.Image(lena()->xsize(),lena()->ysize());
**!		i->tuned_box(0,0,i->xsize()-1,i->ysize()-1,
**!		     ({({0,0,0}),({0,0,0}),({255,255,255}),({255,255,255})})); 
**!		object c=Image.Colortable(4,4,4)->randomcube();
**!		return c*i; 
**!	</illustration></td>
**!	<td><illustration> 
**!		object i=Image.Image(lena()->xsize(),lena()->ysize());
**!		i->tuned_box(0,0,i->xsize()-1,i->ysize()-1,
**!		     ({({0,0,0}),({0,0,0}),({255,255,255}),({255,255,255})})); 
**!		object c=Image.Colortable(4,4,4)->randomgrey();
**!		return c*i; 
**!	</illustration></td>
**!	</tr></table>
**!
**! arg int r
**! arg int g
**! arg int b
**! arg int err
**!	The maximum error. Default is 32, or colorcube step.
**!
**! returns the object being called
**!
**! see also: ordered, nodither, floyd_steinberg, create
**!
**! note
**!	<ref>randomgrey</ref> method needs colorcube size to be the same on
**!	red, green and blue sides to work properly. It uses the
**!	red colorcube value as default.
**/

void image_colortable_randomcube(INT32 args)
{
   THIS->dither_type=NCTD_NONE;

   if (args>=3) 
      if (sp[-args].type!=T_INT||
	  sp[1-args].type!=T_INT||
	  sp[2-args].type!=T_INT)
	 bad_arg_error("Image.Colortable->randomcube",sp-args,args,0,"",sp-args,
		"Bad arguments to Image.Colortable->randomcube()\n");
      else
      {
	 THIS->du.randomcube.r=sp[-args].u.integer;
	 THIS->du.randomcube.g=sp[1-args].u.integer;
	 THIS->du.randomcube.b=sp[2-args].u.integer;
      }
   else if (THIS->type==NCT_CUBE && THIS->u.cube.r && 
	    THIS->u.cube.g && THIS->u.cube.b)
   {
      THIS->du.randomcube.r=256/THIS->u.cube.r;
      THIS->du.randomcube.g=256/THIS->u.cube.g;
      THIS->du.randomcube.b=256/THIS->u.cube.b;
   }
   else
   {
      THIS->du.randomcube.r=32;
      THIS->du.randomcube.g=32;
      THIS->du.randomcube.b=32;
   }

   THIS->dither_type=NCTD_RANDOMCUBE;

   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

void image_colortable_randomgrey(INT32 args)
{
   THIS->dither_type=NCTD_NONE;

   if (args) 
      if (sp[-args].type!=T_INT)
	 bad_arg_error("Image.Colortable->randomgrey",sp-args,args,0,"",sp-args,
		"Bad arguments to Image.Colortable->randomgrey()\n");
      else
	 THIS->du.randomcube.r=sp[-args].u.integer;
   else if (THIS->type==NCT_CUBE && THIS->u.cube.r)
      THIS->du.randomcube.r=256/THIS->u.cube.r;
   else
      THIS->du.randomcube.r=32;

   THIS->dither_type=NCTD_RANDOMGREY;

   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

static int* ordered_calculate_errors(int dxs,int dys)
{
   int *src,*dest;
   int sxs,sys;

   static const int errors2x1[2]={0,1};
   static const int errors2x2[4]={0,2,3,1};
   static const int errors3x1[3]={1,0,2};
   static const int errors3x2[6]={4,0,2,1,5,3};
   static const int errors3x3[9]={6,8,4,1,0,3,5,2,7};

   const int *errs;
   int szx,szy,sz,*d,*s;
   int xf,yf;
   int x,y;
   
   src=malloc(sizeof(int)*dxs*dys);
   dest=malloc(sizeof(int)*dxs*dys);

   if (!src||!dest) 
   {
      if (src) free(src);
      if (dest) free(dest);
      return NULL;
   }

   *src=0;
   sxs=sys=1;
   MEMSET(src,0,sizeof(int)*dxs*dys);
   MEMSET(dest,0,sizeof(int)*dxs*dys);

   for (;;)
   {
      if (dxs==sxs) xf=1;
      else if (((dxs/sxs)%2)==0) xf=2;
      else if (((dxs/sxs)%3)==0) xf=3;
      else break;
      if (dys==sys) yf=1;
      else if (((dys/sys)%2)==0) yf=2;
      else if (((dys/sys)%3)==0) yf=3;
      else break;

      if (xf==1 && yf==1) break;

      szx=xf; szy=yf;

      switch (xf*yf)
      {
	 case 2: errs=errors2x1; break;
	 case 3: errs=errors3x1; break;
	 case 4: errs=errors2x2; break;
	 case 6: errs=errors3x2; break;
	 case 9: errs=errors3x3; break;
	 default:
	   Pike_fatal("impossible case in colortable ordered dither generator.\n");
	   return NULL; /* uh<tm> (not in {x|x={1,2,3}*{1,2,3}}) */
      }
      
      sz=sxs*sys;
      d=dest;
      s=src;

      for (y=0; y<sys; y++)
      {
	 const int *errq=errs;
	 for (yf=0; yf<szy; yf++)
	 {
	    int *sd=s;
	    for (x=0; x<sxs; x++)
	    {
	       const int *errp=errq;
	       for (xf=0; xf<szx; xf++)
		  *(d++)=*sd+sz**(errp++);
	       sd++;
	    }
	    errq+=szx;
	 }
	 s+=sxs;
      }

      sxs*=szx;
      sys*=szy;

      /* ok, rotate... */
      {
	int *tmp=src;
	src=dest;
	dest=tmp;
      }
   }

#if 0
   s=src;
   for (y=0; y<dys; y++)
   {
      printf("(");
      for (x=0; x<dxs; x++)
         printf("%4d",*(s++));
      printf(" )\n");
   }
#endif
   free(dest);

   return src;
}

static int *ordered_make_diff(int *errors,int sz,int err)
{
   /* ok, i want errors between -err and +err from 0 and sz-1 */

   int *dest;
   int *d;
   int n=sz;
   double q;

   d=dest=(int*)malloc(sizeof(int)*sz);
   if (!d) return d;

   if (sz!=1) q = DO_NOT_WARN(1.0/(sz-1)); else q=1.0;

   while (n--)
      *(d++) = DOUBLE_TO_INT((*(errors++)*q-0.5)*2*err);
   
   return dest;
}

/*
**! method object ordered()
**! method object ordered(int r,int g,int b)
**! method object ordered(int r,int g,int b,int xsize,int ysize)
**! method object ordered(int r,int g,int b,int xsize,int ysize,int x,int y)
**! method object ordered(int r,int g,int b,int xsize,int ysize,int rx,int ry,int gx,int gy,int bx,int by)
**!	Set ordered dithering, which gives a position-dependent error added
**!	to the pixel values. 
**!
**!	<table><tr valign=center>
**!	<td><illustration> return lena(); </illustration></td>
**!	<td><illustration> object c=Image.Colortable(6,6,6)->ordered(42,42,42,2,2); return c*lena(); </illustration></td>
**!	<td><illustration> object c=Image.Colortable(6,6,6)->ordered(); return c*lena(); </illustration></td>
**!	<td><illustration> object c=Image.Colortable(6,6,6)->ordered(42,42,42,8,8,0,0,0,1,1,0); return c*lena(); </illustration></td>
**!	</tr><tr valign=top>
**!	<td>original</td>
**!	<td colspan=2>mapped to <br><tt>Image.Colortable(6,6,6)-></tt></td>
**!	</tr><tr valign=top>
**!	<td></td>
**!	<td><tt>ordered<br> (42,42,42,2,2)</tt></td>
**!	<td><tt>ordered()</tt></td>
**!	<td><tt>ordered<br> (42,42,42, 8,8,<br> 0,0, 0,1, 1,0)</tt></td>
**!	</tr><tr valign=top>
**!	<td><illustration> 
**!		object i=Image.Image(lena()->xsize(),lena()->ysize());
**!		i->tuned_box(0,0,i->xsize()-1,i->ysize()-1,
**!		     ({({0,0,0}),({0,0,0}),({255,255,255}),({255,255,255})})); 
**!		return i; 
**!	</illustration></td>
**!	<td><illustration> 
**!		object i=Image.Image(lena()->xsize(),lena()->ysize());
**!		i->tuned_box(0,0,i->xsize()-1,i->ysize()-1,
**!		     ({({0,0,0}),({0,0,0}),({255,255,255}),({255,255,255})})); 
**!		object c=Image.Colortable(6,6,6)->ordered(42,42,42,2,2);
**!		return c*i; 
**!	</illustration></td>
**!	<td><illustration> 
**!		object i=Image.Image(lena()->xsize(),lena()->ysize());
**!		i->tuned_box(0,0,i->xsize()-1,i->ysize()-1,
**!		     ({({0,0,0}),({0,0,0}),({255,255,255}),({255,255,255})})); 
**!		object c=Image.Colortable(6,6,6)->ordered();
**!		return c*i; 
**!	</illustration></td>
**!	<td><illustration> 
**!		object i=Image.Image(lena()->xsize(),lena()->ysize());
**!		i->tuned_box(0,0,i->xsize()-1,i->ysize()-1,
**!		     ({({0,0,0}),({0,0,0}),({255,255,255}),({255,255,255})})); 
**!		object c=Image.Colortable(6,6,6)->ordered(42,42,42,8,8,0,0,0,1,1,0);
**!		return c*i; 
**!	</illustration></td>
**!	</tr></table>
**!
**! arg int r
**! arg int g
**! arg int b
**!	The maximum error. Default is 32, or colorcube steps (256/size).
**!
**! arg int xsize
**! arg int ysize
**!	Size of error matrix. Default is 8×8.
**!	Only values which factors to multiples of 2 and 3 are
**!	possible to choose (2,3,4,6,8,12,...).
**!
**! arg int x
**! arg int y
**! arg int rx
**! arg int ry
**! arg int gx
**! arg int gy
**! arg int bx
**! arg int by
**!	Offset for the error matrix. <tt>x</tt> and <tt>y</tt> is for
**!	both red, green and blue values, the other is individual.
**!
**! returns the object being called
**!
**! see also: randomcube, nodither, floyd_steinberg, create
**/

void image_colortable_ordered(INT32 args)
{
   int *errors;
   int r,g,b;
   int xsize,ysize;

   colortable_free_dither_union(THIS);
   THIS->dither_type=NCTD_NONE;

   if (args>=3) 
      if (sp[-args].type!=T_INT||
	  sp[1-args].type!=T_INT||
	  sp[2-args].type!=T_INT) 
      {
	 bad_arg_error("Image.Colortable->ordered",sp-args,args,0,"",sp-args,
		"Bad arguments to Image.Colortable->ordered()\n");
	 /* Not reached, but keep the compiler happy */
	 r = 0;
	 g = 0;
	 b = 0;
      } 
      else 
      {
	 r=sp[-args].u.integer;
	 g=sp[1-args].u.integer;
	 b=sp[2-args].u.integer;
      }
   else if (THIS->type==NCT_CUBE && THIS->u.cube.r && 
	      THIS->u.cube.g && THIS->u.cube.b)
   {
      r=256/THIS->u.cube.r;
      g=256/THIS->u.cube.g;
      b=256/THIS->u.cube.b;
   }
   else
   {
      r=32;
      g=32;
      b=32;
   }

   xsize=ysize=8;

   THIS->du.ordered.rx=
   THIS->du.ordered.ry=
   THIS->du.ordered.gx=
   THIS->du.ordered.gy=
   THIS->du.ordered.bx=
   THIS->du.ordered.by=0;

   if (args>=5)
   {
      if (sp[3-args].type!=T_INT||
	  sp[4-args].type!=T_INT)
	 bad_arg_error("Image.Colortable->ordered",sp-args,args,0,"",sp-args,
		"Bad arguments to Image.Colortable->ordered()\n");
      else
      {
	 xsize=MAXIMUM(sp[3-args].u.integer,1);
	 ysize=MAXIMUM(sp[4-args].u.integer,1);
      }
   }

   if (args>=11)
   {
      if (sp[5-args].type!=T_INT||
	  sp[6-args].type!=T_INT||
	  sp[7-args].type!=T_INT||
	  sp[8-args].type!=T_INT||
          sp[9-args].type!=T_INT||
	  sp[10-args].type!=T_INT)
	 bad_arg_error("Image.Colortable->ordered",sp-args,args,0,"",sp-args,
		"Bad arguments to Image.Colortable->ordered()\n");
      else
      {
	 THIS->du.ordered.rx=sp[5-args].u.integer;
	 THIS->du.ordered.ry=sp[6-args].u.integer;
	 THIS->du.ordered.gx=sp[7-args].u.integer;
	 THIS->du.ordered.gy=sp[8-args].u.integer;
	 THIS->du.ordered.bx=sp[9-args].u.integer;
	 THIS->du.ordered.by=sp[10-args].u.integer;
      }
   }
   else if (args>=7)
   {
      if (sp[5-args].type!=T_INT||
	  sp[6-args].type!=T_INT)
	 bad_arg_error("Image.Colortable->ordered",sp-args,args,0,"",sp-args,
		"Bad arguments to Image.Colortable->ordered()\n");
      else
      {
	 THIS->du.ordered.rx=
	 THIS->du.ordered.gx=
	 THIS->du.ordered.bx=sp[5-args].u.integer;
	 THIS->du.ordered.ry=
	 THIS->du.ordered.gy=
	 THIS->du.ordered.by=sp[6-args].u.integer;
      }
   }

   errors=ordered_calculate_errors(xsize,ysize);
   if (!errors) 
   {
      resource_error(NULL,0,0,"memory",0,"Out of memory.\n");
      return;
   }

   THIS->du.ordered.rdiff=ordered_make_diff(errors,xsize*ysize,r);
   THIS->du.ordered.gdiff=ordered_make_diff(errors,xsize*ysize,g);
   THIS->du.ordered.bdiff=ordered_make_diff(errors,xsize*ysize,b);

   if (r==g && g==b && 
       THIS->du.ordered.rx==THIS->du.ordered.gx && THIS->du.ordered.gx==THIS->du.ordered.bx) 
   {
     THIS->du.ordered.same=1;
   }
   else
     THIS->du.ordered.same=0;

   free(errors);

   if (!THIS->du.ordered.rdiff||
       !THIS->du.ordered.gdiff||
       !THIS->du.ordered.bdiff)
   {
      if (THIS->du.ordered.rdiff) free(THIS->du.ordered.rdiff);
      if (THIS->du.ordered.gdiff) free(THIS->du.ordered.gdiff);
      if (THIS->du.ordered.bdiff) free(THIS->du.ordered.bdiff);
      resource_error(NULL,0,0,"memory",0,"Out of memory.\n");
      return;
   }

   THIS->du.ordered.xs=xsize;
   THIS->du.ordered.ys=ysize;

   THIS->dither_type=NCTD_ORDERED;

   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*
**! method object image()
**!	cast the colortable to an image object
**!
**!	each pixel in the image object is an entry in the colortable
**!
**! returns the resulting image object
**/

void image_colortable_image(INT32 args)
{
   struct object *o;
   struct image *img;
   struct nct_flat flat;
   int i;
   rgb_group *dest;

   pop_n_elems(args);
   push_int64(image_colortable_size(THIS));
   push_int(1);
   o=clone_object(image_program,2);
   push_object(o);

   if (THIS->type==NCT_NONE)
      return;

   img=(struct image*)get_storage(o,image_program);
   dest=img->img;
   
   if (THIS->type==NCT_CUBE)
      flat=_img_nct_cube_to_flat(THIS->u.cube);
   else
      flat=THIS->u.flat;

   /* sort in number order? */

   for (i=0; i<flat.numentries; i++)
   {
      dest->r=flat.entries[i].color.r;
      dest->g=flat.entries[i].color.g;
      dest->g=flat.entries[i].color.b;
      dest++;
   }

   if (THIS->type==NCT_CUBE)
      free(flat.entries);
}

/*
**! method array(object) corners()
**!	Gives the eight corners in rgb colorspace as an array.
**!	The "black" and "white" corners are the first two.
**/

void image_colortable_corners(INT32 args)
{
   struct nct_flat flat;
   int i;
   rgb_group min={COLORMAX,COLORMAX,COLORMAX};
   rgb_group max={0,0,0};

   pop_n_elems(args);
   
   if (THIS->type==NCT_NONE)
   {
      f_aggregate(0);
      return;
   }

   if (THIS->type==NCT_CUBE)
      flat=_img_nct_cube_to_flat(THIS->u.cube);
   else
      flat=THIS->u.flat;

   /* sort in number order? */

   for (i=0; i<flat.numentries; i++)
      if (flat.entries[i].no!=-1)
      {
	 rgb_group rgb=flat.entries[i].color;
	 if (rgb.r<min.r) min.r=rgb.r;
	 if (rgb.g<min.g) min.g=rgb.g;
	 if (rgb.b<min.b) min.b=rgb.b;
	 if (rgb.r>max.r) max.r=rgb.r;
	 if (rgb.g>max.g) max.g=rgb.g;
	 if (rgb.b>max.b) max.b=rgb.b;
      }

   _image_make_rgb_color(min.r,min.g,min.b);
   _image_make_rgb_color(max.r,max.g,max.b);

   _image_make_rgb_color(max.r,min.g,min.b);
   _image_make_rgb_color(min.r,max.g,min.b);
   _image_make_rgb_color(max.r,max.g,min.b);
   _image_make_rgb_color(min.r,min.g,max.b);
   _image_make_rgb_color(max.r,min.g,max.b);
   _image_make_rgb_color(min.r,max.g,max.b);

   f_aggregate(8);

   if (THIS->type==NCT_CUBE)
      free(flat.entries);
}

static void image_colortable__sprintf( INT32 args )
{
  int x;
  if (args != 2 )
    SIMPLE_TOO_FEW_ARGS_ERROR("_sprintf",2);
  if (sp[-args].type!=T_INT)
    SIMPLE_BAD_ARG_ERROR("_sprintf",0,"integer");
  if (sp[1-args].type!=T_MAPPING)
    SIMPLE_BAD_ARG_ERROR("_sprintf",1,"mapping");

  x = sp[-2].u.integer;

  pop_n_elems( 2 );
  switch( x )
  {
   case 't':
     push_constant_text("Image.Colortable");
     return;
   case 'O':
     push_constant_text( "Image.Colortable( %d, m=%s, d=%s )" );
     push_int64( image_colortable_size( THIS ) );
     switch( THIS->type )
     {
      case NCT_NONE: push_constant_text( "none" );   break;
      case NCT_FLAT: push_constant_text( "flat" );   break;
      case NCT_CUBE: push_constant_text( "cube" );   break;
     }
     switch( THIS->dither_type )
     {
      case NCTD_NONE: push_constant_text( "none" );   break;
      case NCTD_FLOYD_STEINBERG:push_constant_text( "floyd-steinberg" );break;
      case NCTD_RANDOMCUBE: push_constant_text( "randomcube" );   break;
      case NCTD_RANDOMGREY: push_constant_text( "randomgrey" );   break;
      case NCTD_ORDERED: push_constant_text( "ordered" );   break;
     }
     f_sprintf( 4 );
     return;
   default:
     push_int(0);
     return;
  }
}

/***************** global init etc *****************************/

void init_image_colortable(void)
{
   s_array=make_shared_string("array");
   s_string=make_shared_string("string");
   s_mapping=make_shared_string("mapping");

   ADD_STORAGE(struct neo_colortable);

   set_init_callback(init_colortable_struct);
   set_exit_callback(exit_colortable_struct);

   /* function(void:void)|"
		"function(array(array(int)|string|object):void)|"
		"function(object,void|int,mixed ...:void)|"
		"function(int,int,int,void|int ...:void) */
   ADD_FUNCTION("create",image_colortable_create,tOr4(tFunc(tVoid,tVoid),tFunc(tOr(tArr(tColor),tStr),tVoid),tFuncV(tObj tOr(tVoid,tInt),tMix,tVoid),tFuncV(tInt tInt tInt,tOr(tVoid,tInt),tVoid)),0);

   ADD_FUNCTION("_sprintf", image_colortable__sprintf, 
                tFunc(tInt tMapping, tString ), 0 );
   /* function(void:void)|"
		"function(array(array(int)|string|object):void)|"
		"function(object,void|int,mixed ...:void)|"
		"function(int,int,int,void|int ...:void) */
   ADD_FUNCTION("add",image_colortable_create,tOr4(tFunc(tVoid,tVoid),tFunc(tArr(tColor),tVoid),tFuncV(tObj tOr(tVoid,tInt),tMix,tVoid),tFuncV(tInt tInt tInt,tOr(tVoid,tInt),tVoid)),0);

   /* function(int:object) */
   ADD_FUNCTION("reduce",image_colortable_reduce,tFunc(tInt,tObj),0);
   ADD_FUNCTION("reduce_fs",image_colortable_reduce_fs,tFunc(tInt,tObj),0);

   /* operators */
   ADD_FUNCTION("`+",image_colortable_operator_plus,tFunc(tObj,tObj),0);
   ADD_FUNCTION("``+",image_colortable_operator_plus,tFunc(tObj,tObj),0);

   /* cast to array */
   ADD_FUNCTION("cast",image_colortable_cast,tFunc(tStr,tArray),0);

   /* info */
   ADD_FUNCTION("_sizeof",image_colortable__sizeof,tFunc(tNone,tInt),0);

   /* lookup modes */
   ADD_FUNCTION("cubicles",image_colortable_cubicles,tOr(tFunc(tNone,tObj),tFunc(tInt tInt tInt tOr(tVoid,tInt),tObj)),0);
   ADD_FUNCTION("rigid",image_colortable_rigid,tOr(tFunc(tNone,tObj),tFunc(tInt tInt tInt,tObj)),0);
   ADD_FUNCTION("full",image_colortable_full,tFunc(tNone,tObj),0);

   /* map image */
   /* function(object:object)|function(string,int,int) */
#define map_func_type tOr(tFunc(tObj,tObj),tFunc(tString tInt tInt,tObj))
   ADD_FUNCTION("map",image_colortable_map,map_func_type,0);
   ADD_FUNCTION("`*",image_colortable_map,map_func_type,0);
   ADD_FUNCTION("``*",image_colortable_map,map_func_type,0);

   ADD_FUNCTION("index",image_colortable_index_32bit,tFunc(tObj,tStr),0);

   /* dither */
   /* function(:object) */
   ADD_FUNCTION("nodither",image_colortable_nodither,tFunc(tNone,tObj),0);
   /* function(void|int:object)"
      "|function(int,int|float,int|float,int|float,int|float:object) */
   ADD_FUNCTION("floyd_steinberg",image_colortable_floyd_steinberg,tOr(tFunc(tOr(tVoid,tInt),tObj),tFunc(tInt tOr(tInt,tFlt) tOr(tInt,tFlt) tOr(tInt,tFlt) tOr(tInt,tFlt),tObj)),0);
   /* function(:object)|function(int,int,int:object) */
   ADD_FUNCTION("randomcube",image_colortable_randomcube,tOr(tFunc(tNone,tObj),tFunc(tInt tInt tInt,tObj)),0);
   /* function(:object)|function(int:object) */
   ADD_FUNCTION("randomgrey",image_colortable_randomgrey,tOr(tFunc(tNone,tObj),tFunc(tInt,tObj)),0);
   /* function(:object)"
      "|function(int,int,int:object) */
   ADD_FUNCTION("ordered",image_colortable_ordered,
		tOr5(tFunc(tNone,tObj),
		     tFunc(tInt tInt tInt,tObj),
		     tFunc(tInt tInt tInt tInt tInt,tObj),
		     tFunc(tInt tInt tInt tInt tInt tInt tInt,tObj),
		     tFunc(tInt tInt tInt tInt tInt tInt tInt tInt tInt tInt tInt,tObj)),0);

   /* function(:object) */
   ADD_FUNCTION("image",image_colortable_image,tFunc(tNone,tObj),0);

   /* tuning image */
   /* function(int,int,int:object) */
   ADD_FUNCTION("spacefactors",image_colortable_spacefactors,tFunc(tInt tInt tInt,tObj),0);

   ADD_FUNCTION("corners",image_colortable_corners,tFunc(tNone,tArray),0);

}

void exit_image_colortable(void) 
{
   free_string(s_array);
   free_string(s_mapping);
   free_string(s_string);
}
