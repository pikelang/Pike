/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: matrix.c,v 1.49 2004/05/19 00:10:11 nilsson Exp $
*/

/*
**! module Image
**! class Image
*/

#include "global.h"

#include <math.h>
#include <ctype.h>

#include "pike_macros.h"
#include "object.h"
#include "interpret.h"
#include "svalue.h"
#include "threads.h"
#include "pike_error.h"

#include "image.h"


#define sp Pike_sp

extern struct program *image_program;
#ifdef THIS
#undef THIS /* Needed for NT */
#endif

#define THIS ((struct image *)(Pike_fp->current_storage))
#define THISOBJ (Pike_fp->current_object)

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

#define apply_alpha(x,y,alpha) \
   ((unsigned char)((y*(255L-(alpha))+x*(alpha))/255L))

#define set_rgb_group_alpha(dest,src,alpha) \
   ((dest).r=apply_alpha((dest).r,(src).r,alpha), \
    (dest).g=apply_alpha((dest).g,(src).g,alpha), \
    (dest).b=apply_alpha((dest).b,(src).b,alpha))

#define pixel(_img,x,y) ((_img)->img[(x)+(y)*(_img)->xsize])

#define setpixel(x,y) \
   (THIS->alpha? \
    set_rgb_group_alpha(THIS->img[(x)+(y)*THIS->xsize],THIS->rgb,THIS->alpha): \
    ((pixel(THIS,x,y)=THIS->rgb),0))

#define setpixel_test(x,y) \
   (((x)<0||(y)<0||(x)>=THIS->xsize||(y)>=THIS->ysize)? \
    0:(setpixel(x,y),0))

static INLINE int getrgb(struct image *img,
			 INT32 args_start,INT32 args,char *name)
{
   INT32 i;
   if (args-args_start<1) return 0;

   if (image_color_svalue(sp-args+args_start,&(img->rgb)))
      return 1;

   if (args-args_start<3) return 0;

   for (i=0; i<3; i++)
      if (sp[-args+i+args_start].type!=T_INT)
         Pike_error("Illegal r,g,b argument to %s\n",name);
   img->rgb.r=(unsigned char)sp[-args+args_start].u.integer;
   img->rgb.g=(unsigned char)sp[1-args+args_start].u.integer;
   img->rgb.b=(unsigned char)sp[2-args+args_start].u.integer;

   if (args-args_start>=4) {
      if (sp[3-args+args_start].type!=T_INT) {
         Pike_error("Illegal alpha argument to %s\n",name);
      }

      img->alpha=sp[3-args+args_start].u.integer;
      return 4;
   }

   img->alpha=0;
   return 3;
}


static INLINE int getrgbl(rgbl_group *rgb,INT32 args_start,INT32 args,char *name)
{
   INT32 i;
   if (args-args_start<3) return 0;
   for (i=0; i<3; i++)
      if (sp[-args+i+args_start].type!=T_INT)
         Pike_error("Illegal r,g,b argument to %s\n",name);
   rgb->r=sp[-args+args_start].u.integer;
   rgb->g=sp[1-args+args_start].u.integer;
   rgb->b=sp[2-args+args_start].u.integer;
   return 1;
}

/** end internals **/


#define decimals(x) ((x)-DOUBLE_TO_INT(x))
#define testrange(x) MAXIMUM(MINIMUM((x),255),0)
#define _scale_add_rgb(dest,src,factor) \
   ((dest)->r += (float)((src)->r*(factor)), \
    (dest)->g += (float)((src)->g*(factor)), \
    (dest)->b += (float)((src)->b*(factor)))
#define scale_add_pixel(dest,dx,src,sx,factor) \
   _scale_add_rgb(dest,src,factor)

static INLINE void scale_add_line(double py,double dx,
				  rgbd_group *new,INT32 yn,INT32 newx,
				  rgb_group *img,INT32 y,INT32 xsize)
{
   INT32 x,xd;
   double xn,xndxd;
   new=new+yn*newx;
   img=img+y*xsize;
   for (x=0,xn=0; x<xsize; img++,x++,xn+=dx)
   {
      if (DOUBLE_TO_INT(xn) < DOUBLE_TO_INT(xn+dx))
      {
	 xndxd=py*(1.0-decimals(xn));
	 if (xndxd)
	    scale_add_pixel(new,(INT32)xn,img,x,xndxd);
	 if (dx>=1.0 && (xd = DOUBLE_TO_INT(xn+dx) - DOUBLE_TO_INT(xn))>1)
            while (--xd)
	    {
	       new++;
               scale_add_pixel(new,(INT32)(xn+xd),img,x,py);
	    }
	 xndxd=py*decimals(xn+dx);
	 new++;
	 if (xndxd)
	    scale_add_pixel(new,(INT32)(xn+dx),img,x,xndxd);
      }
      else
         scale_add_pixel(new,(int)xn,img,x,py*dx);
   }
}

void img_scale(struct image *dest,
	       struct image *source,
	       INT32 newx,INT32 newy)
{
   rgbd_group *new,*s;
   rgb_group *d;
   INT32 y,yd;
   double yn,dx,dy;

CHRONO("scale begin");

   if (dest->img) { free(dest->img); dest->img=NULL; }

   if (!THIS->img) return; /* no way */
   if (newx<1) newx=1;
   if (newy<1) newy=1;

   new=xalloc(newx*newy*sizeof(rgbd_group) +1);

   THREADS_ALLOW();

   for (y=0; y<newx*newy; y++)
      new[y].r=new[y].g=new[y].b=0.0;

   dx=((double)newx-0.000001)/source->xsize;
   dy=((double)newy-0.000001)/source->ysize;

   for (y=0,yn=0; y<source->ysize; y++,yn+=dy)
   {
      if (DOUBLE_TO_INT(yn)<DOUBLE_TO_INT(yn+dy))
      {
	 if (1.0-decimals(yn))
	    scale_add_line((1.0-decimals(yn)),dx,
			   new, DOUBLE_TO_INT(yn), newx,
			   source->img, y, source->xsize);
	 if ((yd = DOUBLE_TO_INT(yn+dy) - DOUBLE_TO_INT(yn))>1)
            while (--yd)
   	       scale_add_line(1.0, dx, new, DOUBLE_TO_INT(yn+yd), newx,
			      source->img, y, source->xsize);
	 if (decimals(yn+dy))
	    scale_add_line((decimals(yn+dy)), dx, new, DOUBLE_TO_INT(yn+dy),
			   newx, source->img, y, source->xsize);
      }
      else
	 scale_add_line(dy, dx, new, DOUBLE_TO_INT(yn), newx,
			source->img, y, source->xsize);
   }

   dest->img=d=malloc(newx*newy*sizeof(rgb_group) +1);
   if (d) 
   {

CHRONO("transfer begin");

     s=new;
     y=newx*newy;
     while (y--) 
     {
       d->r = MINIMUM(DOUBLE_TO_INT(s->r+0.5),255);
       d->g = MINIMUM(DOUBLE_TO_INT(s->g+0.5),255);
       d->b = MINIMUM(DOUBLE_TO_INT(s->b+0.5),255);
       d++; s++;
     }

     dest->xsize=newx;
     dest->ysize=newy;
   }
   free(new);

CHRONO("scale end");

   THREADS_DISALLOW();
   if (!d)
     resource_error(NULL,0,0,"memory",0,"Out of memory.\n");
}

/* Special, faster, case for scale=1/2 */
void img_scale2(struct image *dest, struct image *source)
{
   rgb_group *new;
   INT32 x, y, newx, newy;
   newx = (source->xsize+1) >> 1;
   newy = (source->ysize+1) >> 1;

   if (dest->img) { free(dest->img); dest->img=NULL; }
   if (!THIS->img || newx<0 || newy<0) return; /* no way */

   if (!newx) newx = 1;
   if (!newy) newy = 1;

   new=xalloc(newx*newy*sizeof(rgb_group) +1);

   THREADS_ALLOW();
   MEMSET(new,0,newx*newy*sizeof(rgb_group));

   dest->img=new;
   dest->xsize=newx;
   dest->ysize=newy;

   /* Adjust for edge. */
   newx -= source->xsize & 1;
   newy -= source->ysize & 1;

   /* The base case. */
   for (y = 0; y < newy; y++)
      for (x = 0; x < newx; x++)
      {
	 pixel(dest,x,y).r = (COLORTYPE)
	    (((INT32) pixel(source,2*x+0,2*y+0).r+
	      (INT32) pixel(source,2*x+1,2*y+0).r+
	      (INT32) pixel(source,2*x+0,2*y+1).r+
	      (INT32) pixel(source,2*x+1,2*y+1).r) >> 2);
	 pixel(dest,x,y).g = (COLORTYPE)
	    (((INT32) pixel(source,2*x+0,2*y+0).g+
	      (INT32) pixel(source,2*x+1,2*y+0).g+
	      (INT32) pixel(source,2*x+0,2*y+1).g+
	      (INT32) pixel(source,2*x+1,2*y+1).g) >> 2);
	 pixel(dest,x,y).b = (COLORTYPE)
	    (((INT32) pixel(source,2*x+0,2*y+0).b+
	      (INT32) pixel(source,2*x+1,2*y+0).b+
	      (INT32) pixel(source,2*x+0,2*y+1).b+
	      (INT32) pixel(source,2*x+1,2*y+1).b) >> 2);
      }
   /* X edge. */
   if (source->xsize & 1) {
     for (y = 0; y < newy; y++) {
       pixel(dest,newx,y).r = (COLORTYPE)
	 (((INT32) pixel(source,2*newx,2*y+0).r+
	   (INT32) pixel(source,2*newx,2*y+1).r) >> 1);
       pixel(dest,newx,y).g = (COLORTYPE)
	 (((INT32) pixel(source,2*newx,2*y+0).g+
	   (INT32) pixel(source,2*newx,2*y+1).g) >> 1);
       pixel(dest,newx,y).g = (COLORTYPE)
	 (((INT32) pixel(source,2*newx,2*y+0).b+
	   (INT32) pixel(source,2*newx,2*y+1).b) >> 1);
     }
   }
   /* Y edge. */
   if (source->ysize & 1) {
     for (x = 0; x < newx; x++) {
       pixel(dest,x,newy).r = (COLORTYPE)
	 (((INT32) pixel(source,2*x+0,2*newy).r+
	   (INT32) pixel(source,2*x+1,2*newy).r) >> 1);
       pixel(dest,x,newy).g = (COLORTYPE)
	 (((INT32) pixel(source,2*x+0,2*newy).g+
	   (INT32) pixel(source,2*x+1,2*newy).g) >> 1);
       pixel(dest,x,newy).b = (COLORTYPE)
	 (((INT32) pixel(source,2*x+0,2*newy).b+
	   (INT32) pixel(source,2*x+1,2*newy).b) >> 1);
     }
   }
   /* Last corner. */
   if (source->xsize & source->ysize & 1) {
     pixel(dest, newx, newy) = pixel(source, source->xsize-1, source->ysize-1);
   }
   THREADS_DISALLOW();
}


/*
**! method object scale(float factor)
**! method object scale(0.5)
**! method object scale(float xfactor,float yfactor)
**!	scales the image with a factor,
**!	0.5 is an optimized case.
**! returns the new image object
**! arg float factor
**!	factor to use for both x and y
**! arg float xfactor
**! arg float yfactor
**!	separate factors for x and y
**!
**! method object scale(int newxsize,int newysize)
**! method object scale(0,int newysize)
**! method object scale(int newxsize,0)
**!	scales the image to a specified new size,
**!	if one of newxsize or newysize is 0,
**!	the image aspect ratio is preserved.
**! returns the new image object
**! arg int newxsize
**! arg int newysize
**!	new image size in pixels
**!
**! note
**!     resulting image will be 1x1 pixels, at least
*/

void image_scale(INT32 args)
{
   float factor;
   struct object *o;
   struct image *newimg;
   extern void image_bitscale( INT32 args );

   o=clone_object(image_program,0);
   newimg=(struct image*)(o->storage);

   if (args==1 && sp[-args].type==T_INT)
   {
      free_object(o);
      image_bitscale( args );
      return;
   }
   else if (args==1 && sp[-args].type==T_FLOAT) 
   {
      if (sp[-args].u.float_number == 0.5)
	 img_scale2(newimg,THIS);
      else
      {
         if( floor( sp[-args].u.float_number ) == sp[-args].u.float_number)
         {
	    free_object(o);
            image_bitscale( args );
            return;
         }
	 img_scale(newimg, THIS,
		   DOUBLE_TO_INT(THIS->xsize*sp[-args].u.float_number),
		   DOUBLE_TO_INT(THIS->ysize*sp[-args].u.float_number));
      }
   }
   else if (args>=2 &&
	    sp[-args].type==T_INT && sp[-args].u.integer==0 &&
	    sp[1-args].type==T_INT)
   {
      factor=((float)sp[1-args].u.integer)/THIS->ysize;
      img_scale(newimg,THIS,
		(INT32)(THIS->xsize*factor),
		sp[1-args].u.integer);
   }
   else if (args>=2 &&
	    sp[1-args].type==T_INT && sp[1-args].u.integer==0 &&
	    sp[-args].type==T_INT)
   {
      factor=((float)sp[-args].u.integer)/THIS->xsize;
      img_scale(newimg,THIS,
		sp[-args].u.integer,
		(INT32)(THIS->ysize*factor));
   }
   else if (args>=2 &&
	    sp[-args].type==T_FLOAT &&
	    sp[1-args].type==T_FLOAT)
      img_scale(newimg, THIS,
		DOUBLE_TO_INT(THIS->xsize*sp[-args].u.float_number),
		DOUBLE_TO_INT(THIS->ysize*sp[1-args].u.float_number));
   else if (args>=2 &&
	    sp[-args].type==T_INT &&
	    sp[1-args].type==T_INT)
      img_scale(newimg,THIS,
		sp[-args].u.integer,
		sp[1-args].u.integer);
   else
   {
      free_object(o);
      bad_arg_error("image->scale",sp-args,args,0,"",sp-args,
		"Bad arguments to image->scale()\n");
   }
   pop_n_elems(args);
   push_object(o);
}

/*
**! method object rotate_ccw()
**!	rotates an image counter-clockwise, 90 degrees.
**!
**!	<table><tr valign=center>
**!	<td><illustration> return lena(); </illustration></td>
**!	<td><illustration> return lena()->rotate_ccw(); </illustration></td>
**!	</tr><tr valign=center>
**!	<td>original</td>
**!	<td>->rotate_ccw();</td>
**!	</tr></table>
**!
**! returns the new image object
**!
**!
*/

void image_ccw(INT32 args)
{
   INT32 i,j,xs,ys;
   rgb_group *src,*dest;
   struct object *o;
   struct image *img;

   pop_n_elems(args);

   if (!THIS->img)
     Pike_error("Called Image.Image object is not initialized\n");

   o=clone_object(image_program,0);
   img=(struct image*)o->storage;
   *img=*THIS;
   if (!(img->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize+1)))
   {
      free_object(o);
      SIMPLE_OUT_OF_MEMORY_ERROR("ccw",
				 sizeof(rgb_group)*THIS->xsize*THIS->ysize+1);
   }
   img->xsize=THIS->ysize;
   img->ysize=THIS->xsize;
   i=xs=THIS->xsize;
   ys=THIS->ysize;
   src=THIS->img+THIS->xsize-1;
   dest=img->img;

   THREADS_ALLOW();
   while (i--)
   {
      j=ys;
      while (j--) *(dest++)=*(src),src+=xs;
      src--;
      src-=xs*ys;
   }
   THREADS_DISALLOW();

   push_object(o);
}

static void img_cw(struct image *is,struct image *id)
{
   INT32 i,j;
   rgb_group *src,*dest;

   if (id->img) free(id->img);
   *id=*is;
   if (!(id->img=malloc(sizeof(rgb_group)*is->xsize*is->ysize+1)))
      resource_error(NULL,0,0,"memory",0,"Out of memory.\n");

   id->xsize=is->ysize;
   id->ysize=is->xsize;
   i=is->xsize;
   src=is->img+is->xsize-1;
   dest=id->img;
   THREADS_ALLOW();
   while (i--)
   {
      j=is->ysize;
      while (j--) *(dest++)=*(src),src+=is->xsize;
      src--;
      src-=is->xsize*is->ysize;
   }
   THREADS_DISALLOW();
}

void img_ccw(struct image *is,struct image *id)
{
   INT32 i,j;
   rgb_group *src,*dest;

   if (id->img) free(id->img);
   *id=*is;
   if (!(id->img=malloc(sizeof(rgb_group)*is->xsize*is->ysize+1)))
      resource_error(NULL,0,0,"memory",0,"Out of memory.\n");

   id->xsize=is->ysize;
   id->ysize=is->xsize;
   i=is->xsize;
   src=is->img+is->xsize-1;
   dest=id->img+is->xsize*is->ysize;
   THREADS_ALLOW();
   while (i--)
   {
      j=is->ysize;
      while (j--) *(--dest)=*(src),src+=is->xsize;
      src--;
      src-=is->xsize*is->ysize;
   }
   THREADS_DISALLOW();
}

/*
**! method object rotate_cw()
**!	rotates an image clockwise, 90 degrees.
**!
**!	<table><tr valign=center>
**!	<td><illustration> return lena(); </illustration></td>
**!	<td><illustration> return lena()->rotate_cw(); </illustration></td>
**!	</tr><tr valign=center>
**!	<td>original</td>
**!	<td>->rotate_cw();</td>
**!	</tr></table>
**! returns the new image object
**!
**!
*/

void image_cw(INT32 args)
{
   INT32 i,j,xs,ys;
   rgb_group *src,*dest;
   struct object *o;
   struct image *img;

   pop_n_elems(args);

   if (!THIS->img)
     Pike_error("Called Image.Image object is not initialized\n");

   o=clone_object(image_program,0);
   img=(struct image*)o->storage;
   *img=*THIS;
   if (!(img->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize+1)))
   {
      free_object(o);
      SIMPLE_OUT_OF_MEMORY_ERROR("cw",
				 sizeof(rgb_group)*THIS->xsize*THIS->ysize+1);
   }
   ys=img->xsize=THIS->ysize;
   i=xs=img->ysize=THIS->xsize;

   src=THIS->img+THIS->xsize-1;
   dest=img->img+THIS->xsize*THIS->ysize;
   THREADS_ALLOW();
   while (i--)
   {
      j=ys;
      while (j--) *(--dest)=*(src),src+=xs;
      src--;
      src-=xs*ys;
   }
   THREADS_DISALLOW();

   push_object(o);
}

/*
**! method object mirrorx()
**!	mirrors an image:
**!	<table><tr valign=center>
**!	<td><illustration> return lena(); </illustration></td>
**!	<td><illustration> return lena()->mirrorx(); </illustration></td>
**!	</tr><tr valign=center>
**!	<td>original</td>
**!	<td>->mirrorx();</td>
**!	</tr></table>
**! returns the new image object
**!
**!
*/

void image_mirrorx(INT32 args)
{
   rgb_group *src,*dest;
   struct object *o;
   struct image *img;
   INT32 i,j,xs;

   pop_n_elems(args);

   if (!THIS->img)
     Pike_error("Called Image.Image object is not initialized\n");

   o=clone_object(image_program,0);
   img=(struct image*)o->storage;
   *img=*THIS;
   if (!(img->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize+1)))
   {
      free_object(o);
      SIMPLE_OUT_OF_MEMORY_ERROR("mirrorx",
				 sizeof(rgb_group)*THIS->xsize*THIS->ysize+1);
   }

   i=THIS->ysize;
   src=THIS->img+THIS->xsize-1;
   dest=img->img;
   xs=THIS->xsize;
   THREADS_ALLOW();
   while (i--)
   {
      j=xs;
      while (j--) *(dest++)=*(src--);
      src+=xs*2;
   }
   THREADS_DISALLOW();

   push_object(o);
}

/*
**! method object mirrory()
**!	mirrors an image:
**!	<table><tr valign=center>
**!	<td><illustration> return lena(); </illustration></td>
**!	<td><illustration> return lena()->mirrory(); </illustration></td>
**!	</tr><tr valign=center>
**!	<td>original</td>
**!	<td>->mirrory();</td>
**!	</tr></table>
**!
*/

void image_mirrory(INT32 args)
{
   rgb_group *src,*dest;
   struct object *o;
   struct image *img;
   INT32 i,j,xs;

   pop_n_elems(args);

   if (!THIS->img)
     Pike_error("Called Image.Image object is not initialized\n");

   o=clone_object(image_program,0);
   img=(struct image*)o->storage;
   *img=*THIS;
   if (!(img->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize+1)))
   {
      free_object(o);
      SIMPLE_OUT_OF_MEMORY_ERROR("mirrory",
				 sizeof(rgb_group)*THIS->xsize*THIS->ysize+1);
   }

   i=THIS->ysize;
   src=THIS->img+THIS->xsize*(THIS->ysize-1);
   dest=img->img;
   xs=THIS->xsize;
   THREADS_ALLOW();
   while (i--)
   {
      j=xs;
      while (j--) *(dest++)=*(src++);
      src-=xs*2;
   }
   THREADS_DISALLOW();

   push_object(o);

}


#define ROUND(X) (DOUBLE_TO_COLORTYPE((X)+0.5))

static void img_skewx(struct image *src,
		      struct image *dest,
		      double diff,
		      int xpn) /* expand pixel for use with alpha instead */
{
   double x0,xmod,xm,x0f;
   INT32 y,len,x0i;
   rgb_group *s,*d;
   rgb_group rgb;

   if (dest->img) free(dest->img);
   if (diff<0)
      dest->xsize = DOUBLE_TO_INT(ceil(-diff)) + src->xsize, x0=-diff;
   else
      dest->xsize = DOUBLE_TO_INT(ceil(diff)) + src->xsize, x0=0;
   dest->ysize=src->ysize;
   len=src->xsize;

   if (!src->xsize) dest->xsize=0;
   d=dest->img=malloc(sizeof(rgb_group)*dest->xsize*dest->ysize+1);
   if (!d) return;
   s=src->img;

   if (!src->xsize || !src->ysize) {
     return;
   }

   THREADS_ALLOW();
   xmod=diff/src->ysize;
   rgb=dest->rgb;

   CHRONO("skewx begin\n");

   y=src->ysize;
   while (y--)
   {
      int j;

      if (xpn) rgb=*s;
      for (j = x0i = DOUBLE_TO_INT((x0f = floor(x0))); j--;) *(d++)=rgb;
      if (!(xm=(x0-x0f)))
      {
	 for (j=len; j--;) *(d++)=*(s++);
	 j = dest->xsize - x0i - len;
      }
      else
      {
	 double xn = 1.0 - xm;
	 if (xpn)
	    *d=*s;
	 else
	    d->r=ROUND(rgb.r*xm+s->r*xn),
	    d->g=ROUND(rgb.g*xm+s->g*xn),
	    d->b=ROUND(rgb.b*xm+s->b*xn);
	 d++;
	 for (j=len-1; j--;)
	 {
	    d->r=ROUND(s->r*xm+s[1].r*xn),
	    d->g=ROUND(s->g*xm+s[1].g*xn),
	    d->b=ROUND(s->b*xm+s[1].b*xn);
	    d++;
	    s++;
	 }
	 if (xpn)
	    *d=*s;
	 else
	    d->r=ROUND(rgb.r*xn+s->r*xm),
	    d->g=ROUND(rgb.g*xn+s->g*xm),
	    d->b=ROUND(rgb.b*xn+s->b*xm);
	 d++;
	 s++;
	 debug_malloc_touch(dest->img);
	 j = dest->xsize - x0i - len - 1;
      }
      if (xpn) rgb=s[-1];
      if(j>0)
	while (j--) *(d++)=rgb;
      else
	d += j;
      debug_malloc_touch(dest->img);
      x0+=xmod;
   }
   THREADS_DISALLOW();
   debug_malloc_touch(dest->img);

   CHRONO("skewx end\n");
}

static void img_skewy(struct image *src,
		      struct image *dest,
		      double diff,
		      int xpn) /* expand pixel for use with alpha instead */
{
   double y0,ymod,ym,y0f;
   INT32 x,len,xsz,y0i;
   rgb_group *s,*d;
   rgb_group rgb;

   if (dest->img) free(dest->img);
   if (diff<0)
      dest->ysize = DOUBLE_TO_INT(ceil(-diff)) + src->ysize, y0 =- diff;
   else
      dest->ysize = DOUBLE_TO_INT(ceil(diff)) + src->ysize, y0 = 0;
   xsz=dest->xsize=src->xsize;
   len=src->ysize;

   if (!src->ysize) dest->ysize=0;
   d=dest->img=malloc(sizeof(rgb_group)*dest->ysize*dest->xsize+1);
   if (!d) return;
   s=src->img;

   THREADS_ALLOW();
   ymod=diff/src->xsize;
   rgb=dest->rgb;

   if (!src->xsize || !src->ysize) {
     return;
   }

CHRONO("skewy begin\n");

   x=src->xsize;
   while (x--)
   {
      int j;

      if (xpn) rgb=*s;
      for (j = y0i = DOUBLE_TO_INT((y0f = floor(y0))); j--;) *d=rgb,d+=xsz;
      if (!(ym=(y0-y0f)))
      {
	 for (j=len; j--;) *d=*s,d+=xsz,s+=xsz;
	 j = dest->ysize - y0i - len;
      }
      else
      {
	 double yn = 1.0 - ym;
	 if (xpn)
	    *d=*s;
	 else
	    d->r=ROUND(rgb.r*ym+s->r*yn),
	    d->g=ROUND(rgb.g*ym+s->g*yn),
	    d->b=ROUND(rgb.b*ym+s->b*yn);
	 d+=xsz;
	 for (j=len-1; j--;)
	 {
	    d->r=ROUND(s->r*ym+s[xsz].r*yn),
	    d->g=ROUND(s->g*ym+s[xsz].g*yn),
	    d->b=ROUND(s->b*ym+s[xsz].b*yn);
	    d+=xsz;
	    s+=xsz;
	 }
	 if (xpn)
	    *d=*s;
	 else
	    d->r=ROUND(rgb.r*yn+s->r*ym),
	    d->g=ROUND(rgb.g*yn+s->g*ym),
	    d->b=ROUND(rgb.b*yn+s->b*ym);
	 d+=xsz;
	 s+=xsz;
	 j = dest->ysize - y0i - len - 1;
      }
      if (xpn) rgb=s[-xsz];
      if(j>0)
	while (j--) *d=rgb,d+=xsz;
      else
	d += j;
      s-=len*xsz-1;
      d-=dest->ysize*xsz-1;
      y0+=ymod;
   }
   THREADS_DISALLOW();

CHRONO("skewy end\n");

}

/*
**! method object skewx(int x)
**! method object skewx(int yfactor)
**! method object skewx(int x,int r,int g,int b)
**! method object skewx(int yfactor,int r,int g,int b)
**! method object skewx_expand(int x)
**! method object skewx_expand(int yfactor)
**! method object skewx_expand(int x,int r,int g,int b)
**! method object skewx_expand(int yfactor,int r,int g,int b)
**!	Skews an image an amount of pixels or a factor;
**!	a skew-x is a transformation:
**!
**!	<table><tr valign=center>
**!	<td><illustration> return lena(); </illustration></td>
**!	<td><illustration> return lena()->skewx(15,255,0,0); </illustration></td>
**!	<td><illustration> return lena()->skewx_expand(15); </illustration></td>
**!	</tr><tr valign=center>
**!	<td>original</td>
**!	<td>->skewx(15,255,0,0);</td>
**!	<td>->skewx_expand(15);</td>
**!	</tr></table>
**!
**! returns the new image object
**! arg int x
**!    the number of pixels
**!	The "expand" variant of functions stretches the 
**!	image border pixels rather then filling with 
**!	the given or current color.
**! arg float yfactor
**!    best described as: x=yfactor*this->ysize()
**! arg int r
**! arg int g
**! arg int b
**!    color to fill with; default is current
*/

void image_skewx(INT32 args)
{
   double diff=0;
   struct object *o;

   if (args<1)
      SIMPLE_TOO_FEW_ARGS_ERROR("image->skewx",1);
   else if (sp[-args].type==T_FLOAT)
      diff = THIS->ysize*sp[-args].u.float_number;
   else if (sp[-args].type==T_INT)
      diff = (double)sp[-args].u.integer;
   else
      bad_arg_error("image->skewx",sp-args,args,0,"",sp-args,
		"Bad arguments to image->skewx()\n");

   if (!THIS->img) Pike_error("Called Image.Image object is not initialized\n");;

   o=clone_object(image_program,0);

   if (!getrgb((struct image*)(o->storage),1,args,"image->skewx()"))
      ((struct image*)(o->storage))->rgb=THIS->rgb;

   img_skewx(THIS,(struct image*)(o->storage),diff,0);

   pop_n_elems(args);
   push_object(o);
}

/*
**! method object skewy(int y)
**! method object skewy(int xfactor)
**! method object skewy(int y,int r,int g,int b)
**! method object skewy(int xfactor,int r,int g,int b)
**! method object skewy_expand(int y)
**! method object skewy_expand(int xfactor)
**! method object skewy_expand(int y,int r,int g,int b)
**! method object skewy_expand(int xfactor,int r,int g,int b)
**!	Skews an image an amount of pixels or a factor;
**!	a skew-y is a transformation:
**!
**!	<table><tr valign=center>
**!	<td><illustration> return lena(); </illustration></td>
**!	<td><illustration> return lena()->skewy(15,255,0,0); </illustration></td>
**!	<td><illustration> return lena()->skewy_expand(15); </illustration></td>
**!	</tr><tr valign=center>
**!	<td>original</td>
**!	<td>->skewy(15,255,0,0);</td>
**!	<td>->skewy_expand(15);</td>
**!	</tr></table>
**!
**!	The "expand" variant of functions stretches the 
**!	image border pixels rather then filling with 
**!	the given or current color.
**! returns the new image object
**! arg int y
**!    the number of pixels
**! arg float xfactor
**!    best described as: t=xfactor*this->xsize()
**! arg int r
**! arg int g
**! arg int b
**!    color to fill with; default is current
*/

void image_skewy(INT32 args)
{
   double diff=0;
   struct object *o;

   if (args<1)
      SIMPLE_TOO_FEW_ARGS_ERROR("image->skewy",1);
   else if (sp[-args].type==T_FLOAT)
      diff = THIS->xsize*sp[-args].u.float_number;
   else if (sp[-args].type==T_INT)
      diff = (double)sp[-args].u.integer;
   else
      bad_arg_error("image->skewx",sp-args,args,0,"",sp-args,
		"Bad arguments to image->skewx()\n");

   if (!THIS->img) Pike_error("Called Image.Image object is not initialized\n");;

   o=clone_object(image_program,0);

   if (!getrgb((struct image*)(o->storage),1,args,"image->skewy()"))
      ((struct image*)(o->storage))->rgb=THIS->rgb;

   img_skewy(THIS,(struct image*)(o->storage),diff,0);

   pop_n_elems(args);
   push_object(o);
}

void image_skewx_expand(INT32 args)
{
   double diff=0;
   struct object *o;

   if (args<1)
      SIMPLE_TOO_FEW_ARGS_ERROR("image->skewx",1);
   else if (sp[-args].type==T_FLOAT)
      diff = THIS->ysize*sp[-args].u.float_number;
   else if (sp[-args].type==T_INT)
      diff = (double)sp[-args].u.integer;
   else
      bad_arg_error("image->skewx",sp-args,args,0,"",sp-args,
		"Bad arguments to image->skewx()\n");

   if (!THIS->img) Pike_error("Called Image.Image object is not initialized\n");;

   o=clone_object(image_program,0);

   if (!getrgb((struct image*)(o->storage),1,args,"image->skewx()"))
      ((struct image*)(o->storage))->rgb=THIS->rgb;

   img_skewx(THIS,(struct image*)(o->storage),diff,1);

   pop_n_elems(args);
   push_object(o);
}

void image_skewy_expand(INT32 args)
{
   double diff=0;
   struct object *o;

   if (args<1)
      SIMPLE_TOO_FEW_ARGS_ERROR("image->skewy",1);
   else if (sp[-args].type==T_FLOAT)
      diff = THIS->xsize*sp[-args].u.float_number;
   else if (sp[-args].type==T_INT)
      diff = (double)sp[-args].u.integer;
   else
      bad_arg_error("image->skewx",sp-args,args,0,"",sp-args,
		"Bad arguments to image->skewx()\n");

   if (!THIS->img) Pike_error("Called Image.Image object is not initialized\n");;

   o=clone_object(image_program,0);

   if (!getrgb((struct image*)(o->storage),1,args,"image->skewy()"))
      ((struct image*)(o->storage))->rgb=THIS->rgb;

   img_skewy(THIS,(struct image*)(o->storage),diff,1);

   pop_n_elems(args);
   push_object(o);
}



void img_rotate(INT32 args,int xpn)
{
   double angle=0;
   struct object *o;
   struct image *dest,d0,dest2;

   if (args<1)
      SIMPLE_TOO_FEW_ARGS_ERROR("image->rotate",1);
   else if (sp[-args].type==T_FLOAT)
      angle = sp[-args].u.float_number;
   else if (sp[-args].type==T_INT)
      angle = (double)sp[-args].u.integer;
   else
      bad_arg_error("image->rotate",sp-args,args,0,"",sp-args,
		"Bad arguments to image->rotate()\n");

   if (!THIS->img) 
      Pike_error("Called Image.Image object is not initialized\n");;

   dest2.img=d0.img=NULL;

   if (angle<-135) angle-=360*DOUBLE_TO_INT((angle-225)/360);
   else if (angle>225) angle-=360*DOUBLE_TO_INT((angle+135)/360);
   if (angle<-45)
   {
      img_ccw(THIS,&dest2);
      angle+=90;
   }
   else if (angle>135)
   {
      img_ccw(THIS,&d0);
      img_ccw(&d0,&dest2);
      angle-=180;
   }
   else if (angle>45)
   {
      img_cw(THIS,&dest2);
      angle-=90;
   }
   else dest2=*THIS;

   angle=(angle/180.0)*3.141592653589793;

   o=clone_object(image_program,0);

   dest=(struct image*)(o->storage);
   if (!getrgb(dest,1,args,"image->rotate()"))
      (dest)->rgb=THIS->rgb;
   d0.rgb=dest2.rgb=dest->rgb;

   img_skewy(&dest2,dest,-tan(angle/2)*dest2.xsize,xpn);
   img_skewx(dest,&d0,sin(angle)*dest->ysize,xpn);
   img_skewy(&d0,dest,-tan(angle/2)*d0.xsize,xpn);

   if (dest2.img!=THIS->img) free(dest2.img);
   free(d0.img);

   pop_n_elems(args);
   push_object(o);
}

/*
**! method object rotate(int|float angle)
**! method object rotate(int|float angle,int r,int g,int b)
**! method object rotate_expand(int|float angle)
**! method object rotate_expand(int|float angle,int r,int g,int b)
**!	Rotates an image a certain amount of degrees (360° is 
**!	a complete rotation) counter-clockwise:
**!
**!	<table><tr valign=center>
**!	<td><illustration> return lena(); </illustration></td>
**!	<td><illustration> return lena()->rotate(15,255,0,0); </illustration></td>
**!	<td><illustration> return lena()->rotate_expand(15); </illustration></td>
**!	</tr><tr valign=center>
**!	<td>original</td>
**!	<td>->rotate(15,255,0,0);</td>
**!	<td>->rotate_expand(15);</td>
**!	</tr></table>
**!
**!	The "expand" variant of functions stretches the 
**!	image border pixels rather then filling with 
**!	the given or current color.
**!
**!	This rotate uses the <ref>skewx</ref>() and <ref>skewy</ref>() functions.
**! returns the new image object
**! arg int|float angle
**!    the number of degrees to rotate
**! arg int r
**! arg int g
**! arg int b
**!    color to fill with; default is current
*/

void image_rotate(INT32 args)
{
   img_rotate(args,0);
}

void image_rotate_expand(INT32 args)
{
   img_rotate(args,1);
}

void img_translate(INT32 args,int expand)
{
   double xt = 0.0, yt = 0.0;
   int y,x;
   struct object *o;
   struct image *img;
   rgb_group *s,*d;

   if (args<2)
     Pike_error("illegal number of arguments to image->translate()\n");

   if (sp[-args].type==T_FLOAT) xt=sp[-args].u.float_number;
   else if (sp[-args].type==T_INT) xt=sp[-args].u.integer;
   else bad_arg_error("image->translate",sp-args,args,1,"",sp+1-1-args,
		"Bad argument 1 to image->translate()\n");

   if (sp[1-args].type==T_FLOAT) yt=sp[1-args].u.float_number;
   else if (sp[1-args].type==T_INT) yt=sp[1-args].u.integer;
   else bad_arg_error("image->translate",sp-args,args,2,"",sp+2-1-args,
		"Bad argument 2 to image->translate()\n");

   getrgb(THIS,2,args,"image->translate()\n");

   xt-=floor(xt);
   yt-=floor(yt);

   o=clone_object(image_program,0);
   img=(struct image*)o->storage;

   img->xsize=THIS->xsize+(xt!=0);
   img->ysize=THIS->ysize+(xt!=0);

   if (!(img->img=malloc(sizeof(rgb_group)*img->xsize*img->ysize+1)))
   {
      free_object(o);
      SIMPLE_OUT_OF_MEMORY_ERROR("translate",
				 sizeof(rgb_group)*img->xsize*img->ysize+1);
   }

   if (!xt)
   {
      MEMCPY(img->img,THIS->img,sizeof(rgb_group)*THIS->xsize*THIS->ysize);
   }
   else
   {
      double xn = 1.0 - xt;

      d=img->img;
      s=THIS->img;

      for (y=0; y<img->ysize; y++)
      {
	 x=THIS->xsize-1;
	 if (!expand)
	    d->r=ROUND(THIS->rgb.r*xt+s->r*xn),
	    d->g=ROUND(THIS->rgb.g*xt+s->g*xn),
	    d->b=ROUND(THIS->rgb.b*xt+s->b*xn);
	 else
	    d->r=s->r, d->g=s->g, d->b=s->b;
	 d++; s++;
	 while (x--)
	 {
	    d->r=ROUND(s->r*xn+s[1].r*xt),
	    d->g=ROUND(s->g*xn+s[1].g*xt),
	    d->b=ROUND(s->b*xn+s[1].b*xt);
	    d++; s++;
	 }
	 if (!expand)
	    d->r=ROUND(s->r*xn+THIS->rgb.r*xt),
	    d->g=ROUND(s->g*xn+THIS->rgb.g*xt),
	    d->b=ROUND(s->b*xn+THIS->rgb.b*xt);
	 else
	    d->r=s->r, d->g=s->g, d->b=s->b;
	 d++;
      }
   }

   if (yt)
   {
      double yn = 1.0 - yt;
      int xsz=img->xsize;

      d=s=img->img;

      for (x=0; x<img->xsize; x++)
      {
	 y=THIS->ysize-1;
	 if (!expand)
	    d->r=ROUND(THIS->rgb.r*yt+s->r*yn),
	    d->g=ROUND(THIS->rgb.g*yt+s->g*yn),
	    d->b=ROUND(THIS->rgb.b*yt+s->b*yn);
	 else
	    d->r=s->r, d->g=s->g, d->b=s->b;
	 d+=xsz; s+=xsz;
	 while (y--)
	 {
	    d->r=ROUND(s->r*yn+s[xsz].r*yt),
	    d->g=ROUND(s->g*yn+s[xsz].g*yt),
	    d->b=ROUND(s->b*yn+s[xsz].b*yt);
	    d+=xsz; s+=xsz;
	 }
	 if (!expand)
	    d->r=ROUND(s->r*yn+THIS->rgb.r*yt),
	    d->g=ROUND(s->g*yn+THIS->rgb.g*yt),
	    d->b=ROUND(s->b*yn+THIS->rgb.b*yt);
	 else
	    d->r=s->r, d->g=s->g, d->b=s->b;
	 d-=xsz*(img->ysize-1)-1;
	 s-=xsz*THIS->ysize-1;
      }
   }

   pop_n_elems(args);
   push_object(o);
}


void image_translate_expand(INT32 args)
{
   img_translate(args,1);
}

void image_translate(INT32 args)
{
   img_translate(args,0);
}
