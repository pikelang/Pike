/* $Id: blit.c,v 1.8 1997/04/18 06:47:18 mirar Exp $ */
#include "global.h"

/*
**! module Image
**! class image
*/

#include <math.h>
#include <ctype.h>

#include "stralloc.h"
#include "global.h"
#include "types.h"
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "array.h"
#include "error.h"
#include "threads.h"

#include "image.h"

extern struct program *image_program;
#define THIS ((struct image *)(fp->current_storage))
#define THISOBJ (fp->current_object)

#define absdiff(a,b) ((a)<(b)?((b)-(a)):((a)-(b)))
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)<(b)?(b):(a))

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

/***************** internals ***********************************/

#define testrange(x) max(min((x),255),0)

#define apply_alpha(x,y,alpha) \
   ((unsigned char)((y*(255L-(alpha))+x*(alpha))/255L))

#define set_rgb_group_alpha(dest,src,alpha) \
   (((dest).r=apply_alpha((dest).r,(src).r,alpha)), \
    ((dest).g=apply_alpha((dest).g,(src).g,alpha)), \
    ((dest).b=apply_alpha((dest).b,(src).b,alpha)))

#define pixel(_img,x,y) ((_img)->img[(x)+(y)*(_img)->xsize])

#define setpixel(x,y) \
   (THIS->alpha? \
    set_rgb_group_alpha(THIS->img[(x)+(y)*THIS->xsize],THIS->rgb,THIS->alpha): \
    ((pixel(THIS,x,y)=THIS->rgb),0))

#define setpixel_test(x,y) \
   (((x)<0||(y)<0||(x)>=THIS->xsize||(y)>=THIS->ysize)? \
    0:(setpixel(x,y),0))

static INLINE void getrgb(struct image *img,
			  INT32 args_start,INT32 args,char *name)
{
   INT32 i;
   if (args-args_start<3) return;
   for (i=0; i<3; i++)
      if (sp[-args+i+args_start].type!=T_INT)
         error("Illegal r,g,b argument to %s\n",name);
   img->rgb.r=(unsigned char)sp[-args+args_start].u.integer;
   img->rgb.g=(unsigned char)sp[1-args+args_start].u.integer;
   img->rgb.b=(unsigned char)sp[2-args+args_start].u.integer;
   if (args-args_start>=4)
      if (sp[3-args+args_start].type!=T_INT)
         error("Illegal alpha argument to %s\n",name);
      else
         img->alpha=sp[3-args+args_start].u.integer;
   else
      img->alpha=0;
}

/*** end internals ***/


void img_clear(rgb_group *dest,rgb_group rgb,INT32 size)
{
   THREADS_ALLOW();
   while (size--) *(dest++)=rgb;
   THREADS_DISALLOW();
}

void img_box_nocheck(INT32 x1,INT32 y1,INT32 x2,INT32 y2)
{
   INT32 x,mod;
   rgb_group *foo,*end,rgb;
   struct image *this;

   this=THIS;
   
   THREADS_ALLOW();
   mod=this->xsize-(x2-x1)-1;
   foo=this->img+x1+y1*this->xsize;
   end=this->img+x1+y2*this->xsize;
   rgb=this->rgb;


   if (!this->alpha)
      for (; foo<=end; foo+=mod) for (x=x1; x<=x2; x++) *(foo++)=rgb;
   else
      for (; foo<=end; foo+=mod) for (x=x1; x<=x2; x++,foo++) 
	 set_rgb_group_alpha(*foo,rgb,this->alpha);
   THREADS_DISALLOW();
}


void img_blit(rgb_group *dest,rgb_group *src,INT32 width,
	      INT32 lines,INT32 moddest,INT32 modsrc)
{
CHRONO("image_blit begin");

   THREADS_ALLOW();
   while (lines--)
   {
      MEMCPY(dest,src,sizeof(rgb_group)*width);
      dest+=moddest;
      src+=modsrc;
   }
   THREADS_DISALLOW();
CHRONO("image_blit end");

}

void img_crop(struct image *dest,
	      struct image *img,
	      INT32 x1,INT32 y1,
	      INT32 x2,INT32 y2)
{
   rgb_group *new;
   INT32 xp,yp,xs,ys;

   if (dest->img) { free(dest->img); dest->img=NULL; }

   if (x1>x2) x1^=x2,x2^=x1,x1^=x2;
   if (y1>y2) y1^=y2,y2^=y1,y1^=y2;

   if (x1==0 && y1==0 &&
       img->xsize-1==x2 && img->ysize-1==y2)
   {
      *dest=*img;
      new=malloc( (x2-x1+1)*(y2-y1+1)*sizeof(rgb_group) + 1);
      if (!new) 
	error("Out of memory.\n");
      THREADS_ALLOW();
      MEMCPY(new,img->img,(x2-x1+1)*(y2-y1+1)*sizeof(rgb_group));
      THREADS_DISALLOW();
      dest->img=new;
      return;
   }

   new=malloc( (x2-x1+1)*(y2-y1+1)*sizeof(rgb_group) +1);
   if (!new)
     error("Out of memory.\n");

   img_clear(new,THIS->rgb,(x2-x1+1)*(y2-y1+1));

   dest->xsize=x2-x1+1;
   dest->ysize=y2-y1+1;

   xp=max(0,-x1);
   yp=max(0,-y1);
   xs=max(0,x1);
   ys=max(0,y1);

   if (x1<0) x1=0; else if (x1>=img->xsize) x1=img->xsize-1;
   if (y1<0) y1=0; else if (y1>=img->ysize) y1=img->ysize-1;
   if (x2<0) x2=0; else if (x2>=img->xsize) x2=img->xsize-1;
   if (y2<0) y2=0; else if (y2>=img->ysize) y2=img->ysize-1;

   img_blit(new+xp+yp*dest->xsize,
	    img->img+xs+(img->xsize)*ys,
	    x2-x1+1,
	    y2-y1+1,
	    dest->xsize,
	    img->xsize);

   dest->img=new;
}

void img_clone(struct image *newimg,struct image *img)
{
   if (newimg->img) free(newimg->img);
   newimg->img=malloc(sizeof(rgb_group)*img->xsize*img->ysize +1);
   if (!newimg->img) error("Out of memory!\n");
   THREADS_ALLOW();
   MEMCPY(newimg->img,img->img,sizeof(rgb_group)*img->xsize*img->ysize);
   THREADS_DISALLOW();
   newimg->xsize=img->xsize;
   newimg->ysize=img->ysize;
   newimg->rgb=img->rgb;
}

/*
**! method object paste(object image)
**! method object paste(object image,int x,int y)
**!    Pastes a given image over the current image.
**!
**! returns the object called
**!
**! arg object image
**!	image to paste
**! arg int x
**! arg int y
**!	where to paste the image; default is 0,0
**!
**! see also: paste_mask, paste_alpha, paste_alpha_color
*/

void image_paste(INT32 args)
{
   struct image *img;
   INT32 x1,y1,x2,y2,blitwidth,blitheight;

   if (args<1
       || sp[-args].type!=T_OBJECT
       || !sp[-args].u.object
       || sp[-args].u.object->prog!=image_program)
      error("illegal argument 1 to image->paste()\n");
   if (!THIS->img) return;

   img=(struct image*)sp[-args].u.object->storage;
   if (!img) return;

   if (args>=3)
   {
      if (sp[1-args].type!=T_INT
	  || sp[2-args].type!=T_INT)
         error("illegal arguments to image->paste()\n");
      x1=sp[1-args].u.integer;
      y1=sp[2-args].u.integer;
   }
   else x1=y1=0;

   x2=x1+img->xsize-1;
   y2=y1+img->ysize-1;

   blitwidth=min(x2,THIS->xsize-1)-max(x1,0)+1;
   blitheight=min(y2,THIS->ysize-1)-max(y1,0)+1;

   img_blit(THIS->img+max(0,x1)+(THIS->xsize)*max(0,y1),
	    img->img+max(0,-x1)+(x2-x1+1)*max(0,-y1),
	    blitwidth,
	    blitheight,
	    THIS->xsize,
	    img->xsize);

   pop_n_elems(args);
   THISOBJ->refs++;
   push_object(THISOBJ);
}

/*
**! method object paste_alpha(object image,int alpha)
**! method object paste_alpha(object image,int alpha,int x,int y)
**!    	Pastes a given image over the current image, with
**!    	the specified alpha channel value.
**!	
**!    	An alpha channel value of 0 leaves nothing of the original 
**!     image in the paste area, 255 is meaningless and makes the
**!	given image invisible.
**!
**! returns the object called
**!
**! arg object image
**!	image to paste
**! arg int alpha
**!	alpha channel value
**! arg int x
**! arg int y
**!	where to paste the image; default is 0,0
**!
**! see also: paste_mask, paste, paste_alpha_color
*/

void image_paste_alpha(INT32 args)
{
   struct image *img;
   INT32 x1,y1,x,y;

   if (args<2
       || sp[-args].type!=T_OBJECT
       || !sp[-args].u.object
       || sp[-args].u.object->prog!=image_program
       || sp[1-args].type!=T_INT)
      error("illegal arguments to image->paste_alpha()\n");
   if (!THIS->img) return;

   img=(struct image*)sp[-args].u.object->storage;
   if (!img) return;
   THIS->alpha=(unsigned char)(sp[1-args].u.integer);
   
   if (args>=4)
   {
      if (sp[2-args].type!=T_INT
	  || sp[3-args].type!=T_INT)
         error("illegal arguments to image->paste_alpha()\n");
      x1=sp[2-args].u.integer;
      y1=sp[3-args].u.integer;
   }
   else x1=y1=0;

/* tråda här nåndag */

   for (x=0; x<img->xsize; x++)
      for (y=0; y<img->ysize; y++)
      {
	 THIS->rgb=pixel(img,x,y);
	 setpixel_test(x1+x,y1+y);
      }

   pop_n_elems(args);
   THISOBJ->refs++;
   push_object(THISOBJ);
}

/*
**! method object paste_mask(object image,object mask)
**! method object paste_mask(object image,object mask,int x,int y)
**!    Pastes a given image over the current image,
**!    using the given mask as opaque channel.  
**!    
**!    A pixel value of 255 makes the result become a pixel
**!    from the given image, 0 doesn't change anything.
**!
**!    The masks red, green and blue values are used separately.
**!
**! returns the object called
**!
**! arg object image
**!	image to paste
**! arg object mask
**!	mask image
**! arg int x
**! arg int y
**!	where to paste the image; default is 0,0
**!
**! see also: paste, paste_alpha, paste_alpha_color
*/

void image_paste_mask(INT32 args)
{
   struct image *img,*mask;
   INT32 x1,y1,x,y,x2,y2,smod,dmod,mmod;
   rgb_group *s,*d,*m;
   float q;

CHRONO("image_paste_mask init");

   if (args<2)
      error("illegal number of arguments to image->paste_mask()\n");
   if (sp[-args].type!=T_OBJECT
       || !sp[-args].u.object
       || sp[-args].u.object->prog!=image_program)
      error("illegal argument 1 to image->paste_mask()\n");
   if (sp[1-args].type!=T_OBJECT
       || !sp[1-args].u.object
       || sp[1-args].u.object->prog!=image_program)
      error("illegal argument 2 to image->paste_mask()\n");
   if (!THIS->img) return;

   img=(struct image*)sp[-args].u.object->storage;
   mask=(struct image*)sp[1-args].u.object->storage;
   if ((!img)||(!img->img)) error("argument 1 has no image\n");
   if ((!mask)||(!mask->img)) error("argument 2 (alpha) has no image\n");
   
   if (args>=4)
   {
      if (sp[2-args].type!=T_INT
	  || sp[3-args].type!=T_INT)
         error("illegal coordinate arguments to image->paste_mask()\n");
      x1=sp[2-args].u.integer;
      y1=sp[3-args].u.integer;
   }
   else x1=y1=0;

   x2=min(THIS->xsize-x1,min(img->xsize,mask->xsize));
   y2=min(THIS->ysize-y1,min(img->ysize,mask->ysize));

CHRONO("image_paste_mask begin");

   s=img->img+max(0,-x1)+max(0,-y1)*img->xsize;
   m=mask->img+max(0,-x1)+max(0,-y1)*mask->xsize;
   d=THIS->img+max(0,-x1)+x1+(y1+max(0,-y1))*THIS->xsize;
   x=max(0,-x1);
   smod=img->xsize-(x2-x);
   mmod=mask->xsize-(x2-x);
   dmod=THIS->xsize-(x2-x);

   q=1.0/255;

   THREADS_ALLOW();
   for (y=max(0,-y1); y<y2; y++)
   {
      for (x=max(0,-x1); x<x2; x++)
      {
	 if (m->r==255) d->r=s->r;
	 else if (m->r==0) {}
	 else d->r=(unsigned char)(((d->r*(255-m->r))+(s->r*m->r))*q);
	 if (m->g==255) d->g=s->g;
	 else if (m->g==0) {}
	 else d->g=(unsigned char)(((d->g*(255-m->g))+(s->g*m->g))*q);
	 if (m->b==255) d->b=s->b;
	 else if (m->b==0) {}
	 else d->b=(unsigned char)(((d->b*(255-m->b))+(s->b*m->b))*q);
	 s++; m++; d++;
      }
      s+=smod; m+=mmod; d+=dmod;
   }
   THREADS_DISALLOW();
CHRONO("image_paste_mask end");

   pop_n_elems(args);
   THISOBJ->refs++;
   push_object(THISOBJ);
}

/*
**! method object paste_alpha_color(object mask)
**! method object paste_alpha_color(object mask,int x,int y)
**! method object paste_alpha_color(object mask,int r,int g,int b)
**! method object paste_alpha_color(object mask,int r,int g,int b,int x,int y)
**!    Pastes a given color over the current image,
**!    using the given mask as opaque channel.  
**!    
**!    A pixel value of 255 makes the result become the color given,
**!    0 doesn't change anything.
**!    
**!    The masks red, green and blue values are used separately.
**!    If no color are given, the current is used.
**!
**! returns the object called
**!
**! arg object mask
**!     mask image
**! arg int r
**! arg int g
**! arg int b
**!     what color to paint with; default is current
**! arg int x
**! arg int y
**!     where to paste the image; default is 0,0
**!
**! see also: paste_mask, paste_alpha, paste_alpha_color
*/

void image_paste_alpha_color(INT32 args)
{
   struct image *img,*mask;
   INT32 x1,y1,x,y,x2,y2;
   rgb_group rgb,*d,*m;
   INT32 mmod,dmod;
   float q;

   if (args!=1 && args!=4 && args!=6 && args!=3)
      error("illegal number of arguments to image->paste_alpha_color()\n");
   if (sp[-args].type!=T_OBJECT
       || !sp[-args].u.object
       || sp[-args].u.object->prog!=image_program)
      error("illegal argument 1 to image->paste_alpha_color()\n");
   if (!THIS->img) return;

   if (args==6 || args==4) /* colors at arg 2..4 */
      getrgb(THIS,1,args,"image->paste_alpha_color()\n");
   if (args==3) /* coords at 2..3 */
   {
      if (sp[1-args].type!=T_INT
	  || sp[2-args].type!=T_INT)
         error("illegal coordinate arguments to image->paste_alpha_color()\n");
      x1=sp[1-args].u.integer;
      y1=sp[2-args].u.integer;
   }
   else if (args==6) /* at 5..6 */
   {
      if (sp[4-args].type!=T_INT
	  || sp[5-args].type!=T_INT)
         error("illegal coordinate arguments to image->paste_alpha_color()\n");
      x1=sp[4-args].u.integer;
      y1=sp[5-args].u.integer;
   }
   else x1=y1=0;

   mask=(struct image*)sp[-args].u.object->storage;
   if (!mask||!mask->img) error("argument 2 (alpha) has no image\n");
   
   x2=min(THIS->xsize-x1,mask->xsize);
   y2=min(THIS->ysize-y1,mask->ysize);

CHRONO("image_paste_alpha_color begin");

   m=mask->img+max(0,-x1)+max(0,-y1)*mask->xsize;
   d=THIS->img+max(0,-x1)+x1+(y1+max(0,-y1))*THIS->xsize;
   x=max(0,-x1);
   mmod=mask->xsize-(x2-x);
   dmod=THIS->xsize-(x2-x);

   q=1.0/255;

   rgb=THIS->rgb;

   THREADS_ALLOW();
   for (y=max(0,-y1); y<y2; y++)
   {
      for (x=max(0,-x1); x<x2; x++)
      {
	 if (m->r==255) d->r=rgb.r;
	 else if (m->r==0) ;
	 else d->r=(unsigned char)(((d->r*(255-m->r))+(rgb.r*m->r))*q);
	 if (m->g==255) d->g=rgb.g;
	 else if (m->g==0) ;
	 else d->g=(unsigned char)(((d->g*(255-m->g))+(rgb.g*m->g))*q);
	 if (m->b==255) d->b=rgb.b;
	 else if (m->b==0) ;
	 else d->b=(unsigned char)(((d->b*(255-m->b))+(rgb.b*m->b))*q);
	 m++; d++;
      }
      m+=mmod; d+=dmod;
   }
   THREADS_DISALLOW();
CHRONO("image_paste_alpha_color end");

   pop_n_elems(args);
   THISOBJ->refs++;
   push_object(THISOBJ);
}

void img_box(INT32 x1,INT32 y1,INT32 x2,INT32 y2)
{   
   if (x1>x2) x1^=x2,x2^=x1,x1^=x2;
   if (y1>y2) y1^=y2,y2^=y1,y1^=y2;
   if (x2<0||y2<0||x1>=THIS->xsize||y1>=THIS->ysize) return;
   img_box_nocheck(max(x1,0),max(y1,0),min(x2,THIS->xsize-1),min(y2,THIS->ysize-1));
}


/*
**! method object add_layers(array(int|object)) layer0,...)
**! method object add_layers(int x1,int y1,int x2,int y2,array(int|object)) layer0,...)
**!	Using the called object as base, adds layers using masks,
**!	opaque channel values and special methods.
**!
**!	The destination image can also be cropped, thus
**!	speeding up the process.
**!
**!	Each array in the layers array is one of:
**!	<pre>
**!	({object image,object|int mask})
**!	({object image,object|int mask,int opaque_value})
**!	({object image,object|int mask,int opaque_value,int method})
**!	</pre>
**!	Given 0 as mask means the image is totally opaque.
**!
**!	Default opaque value is 255, only using the mask.
**!
**!	Methods for now are:
**!	<pre>
**!	0  no operation (just paste with mask, default)
**!	1  maximum  (`|)
**!	2  minimum  (`&)
**!	3  multiply (`*)
**!	4  add      (`+)
**!	5  diff     (`-)
**!	</pre>
**!	The layer image and the current source are calculated
**!	through the given method and then pasted using the mask
**!	and the opaque channel value. 
**!
**!	All given images must be the same size.
**!
**! returns a new image object
**!
**! arg array(int|object) layer0
**!	image to paste
**! arg int x1
**! arg int y1
**! arg int x2
**! arg int y2
**!	rectangle for cropping
**!
**! see also: paste_mask, paste_alpha, paste_alpha_color, `|, `&, `*, `+, `-
*/

void image_add_layers(INT32 args)
/*

[x1,y1,x2,y2],
({object image,object mask|0,[int alpha_value,[int method]]}),
({object image,object mask|0,[int alpha_value,[int method]]}),
...
:object

 */
{
   struct img_layer
   {
      rgb_group *s;
      rgb_group *m;
      int opaque;
      enum layer_method method;
   } *layer;
   float q2=1/(255.0*255.0);
   float q=1/(255.0);
   rgb_group *s;
   rgb_group *d;
   struct object *o;

   int allopaque255=1,allmask=1,allnop=1;

   int i,j,l,mod,layers;
   int x1,y1,x2,y2;
   
   if (!THIS->img) error("No image.");
   s=THIS->img;

   if (args>=4
       && sp[-args].type==T_INT
       && sp[1-args].type==T_INT
       && sp[2-args].type==T_INT
       && sp[3-args].type==T_INT)
   {
      x1=sp[-args].u.integer;
      y1=sp[1-args].u.integer;
      x2=sp[2-args].u.integer;
      y2=sp[3-args].u.integer;

      if (x2<x1) x2^=x1,x1^=x2,x2^=x1;
      if (y2<y1) y2^=y1,y1^=y2,y2^=y1;

      if (x2>THIS->xsize-1 ||
          y2>THIS->ysize-1 ||
	  x1<0 || y1<0)
	 error("Illegal coordinates to image->add_layers()\n");
      layers=args-4;
   }
   else
   {
      x1=0;
      y1=0;
      x2=THIS->xsize-1;
      y2=THIS->ysize-1;
      layers=args;
   }

   layer=(struct img_layer*)xalloc(sizeof(struct img_layer)*layers);

   for (j=0,i=layers; i>0; i--,j++)
   {
      struct array *a;
      struct image *img;
      if (sp[-i].type!=T_ARRAY)
      {
	 free(layer);
	 error("Illegal argument(s) to image->add_layers()\n");
      }
      a=sp[-i].u.array;
      if (a->size<2)
      {
	 free(layer);
	 error("Illegal size of array argument to image->add_layers()\n");
      }

      if (a->item[0].type!=T_OBJECT 
	  || !a->item[0].u.object
	  || a->item[0].u.object->prog!=image_program)
      {
	 free(layer);
	 error("Illegal array contents, layer %d (argument %d) (wrong or no image) to image->add_layers()\n",layers-i,args-i+1);
      }
      img=(struct image*)a->item[0].u.object->storage;
      if (!img->img 
	  || img->xsize != THIS->xsize 
	  || img->ysize != THIS->ysize)
      {
	 free(layer);
	 error("Illegal array contents, layer %d (argument %d) (no image or wrong image size) to image->add_layers()\n",layers-i,args-i+1);
      }
      layer[j].s=img->img;

      if (a->item[1].type==T_INT
	  && a->item[1].u.integer==0)
      {
	 layer[j].m=NULL;
      }
      else
      {
	 if (a->item[1].type!=T_OBJECT 
	     || !a->item[1].u.object
	     || a->item[1].u.object->prog!=image_program)
	 {
	    free(layer);
	    error("Illegal array contents, layer %d (argument %d) (wrong or no image for mask) to image->add_layers()\n",layers-i,args-i+1);
	 }
	 img=(struct image*)a->item[1].u.object->storage;
	 if (!img->img 
	     || img->xsize != THIS->xsize 
	     || img->ysize != THIS->ysize)
	 {
	    free(layer);
	    error("Illegal array contents, layer %d (argument %d) (no mask or wrong mask size) to image->add_layers()\n",layers-i,args-i+1);
	 }
	 layer[j].m=img->img;
      }

      if (a->size>=3)
      {
	 if (a->item[2].type!=T_INT)
	 {
	    free(layer);
	    error("Illegal array contents, layer %d (argument %d) (illegal opaque) to image->add_layers()\n",layers-i,args-i+1);
	 }
	 layer[j].opaque=a->item[2].u.integer;
      }
      else
	 layer[j].opaque=255;

      if (a->size>=4)
      {
	 if (a->item[3].type!=T_INT)
	 {
	    free(layer);
	    error("Illegal array contents, layer %d (argument %d) (illegal method) to image->add_layers()\n",layers-i,args-i+1);
	 }
	 layer[j].method=a->item[3].u.integer;
      }
      else
	 layer[j].method=LAYER_NOP;
   }

   push_int(1+x2-x1);
   push_int(1+y2-y1);
   o=clone_object(image_program,2);
   d=((struct image*)(o->storage))->img;


   mod=THIS->xsize-(x2-x1+1);
   s+=x1+THIS->xsize*y1;
   for (i=0; i<layers; i++)
   {
      layer[i].s+=x1+THIS->xsize*y1;
      if (layer[i].m) layer[i].m+=x1+THIS->xsize*y1;
      else allmask=0;
      if (layer[i].opaque!=255) allopaque255=0;
      if (layer[i].method!=LAYER_NOP) allnop=0;
   }

   if (allmask && allopaque255 && allnop)
   {
#define ALL_IS_NOP
#define ALL_HAVE_MASK
#define ALL_HAVE_OPAQUE
#include "blit_layer_include.h"
#undef ALL_IS_NOP
#undef ALL_HAVE_MASK
#undef ALL_HAVE_OPAQUE
   }
   else if (allopaque255 && allnop)
   {
#define ALL_IS_NOP
#define ALL_HAVE_OPAQUE
#include "blit_layer_include.h"
#undef ALL_IS_NOP
#undef ALL_HAVE_OPAQUE
   }
   else if (allmask && allnop)
   {
#define ALL_HAVE_MASK
#define ALL_HAVE_OPAQUE
#include "blit_layer_include.h"
#undef ALL_HAVE_MASK
#undef ALL_HAVE_OPAQUE
   }
   else if (allnop)
   {
#define ALL_IS_NOP
#include "blit_layer_include.h"
#undef ALL_IS_NOP
   }
   else if (allmask && allopaque255)
   {
#define ALL_HAVE_MASK
#define ALL_HAVE_OPAQUE
#include "blit_layer_include.h"
#undef ALL_HAVE_MASK
#undef ALL_HAVE_OPAQUE
   }
   else if (allopaque255)
   {
#define ALL_HAVE_OPAQUE
#include "blit_layer_include.h"
#undef ALL_HAVE_OPAQUE
   }
   else if (allmask)
   {
#define ALL_HAVE_MASK
#include "blit_layer_include.h"
#undef ALL_HAVE_MASK
   }
   else
   {
#include "blit_layer_include.h"
   }


   pop_n_elems(args);

   free(layer);

   push_object(o);
}
