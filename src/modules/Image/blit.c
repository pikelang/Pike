/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: blit.c,v 1.56 2004/03/05 23:04:02 nilsson Exp $
*/

#include "global.h"

/*
**! module Image
**! class Image
*/

#include <math.h>
#include <ctype.h>

#include "global.h"
#include "pike_macros.h"
#include "interpret.h"
#include "svalue.h"
#include "pike_error.h"
#include "threads.h"

#include "image.h"
#include "image_machine.h"

#define sp Pike_sp

extern struct program *image_program;
#ifdef THIS
#undef THIS /* Needed for NT */
#endif
#define THIS ((struct image *)(Pike_fp->current_storage))
#define THISOBJ (Pike_fp->current_object)

#define absdiff(a,b) ((a)<(b)?((b)-(a)):((a)-(b)))

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

#define testrange(x) MAXIMUM(MINIMUM((x),255),0)

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

static INLINE int getrgb(struct image *img,
			 INT32 args_start,INT32 args,INT32 max,char *name)
{
   INT32 i;
   if (args-args_start<1) return 0;

   if (image_color_svalue(sp-args+args_start,&(img->rgb)))
      return 1;

   if (max<3 || args-args_start<3) return 0;

   for (i=0; i<3; i++)
      if (sp[-args+i+args_start].type!=T_INT)
         Pike_error("Illegal r,g,b argument to %s\n",name);
   img->rgb.r=(unsigned char)sp[-args+args_start].u.integer;
   img->rgb.g=(unsigned char)sp[1-args+args_start].u.integer;
   img->rgb.b=(unsigned char)sp[2-args+args_start].u.integer;

   if (max > 3 && args-args_start>=4) {
      if (sp[3-args+args_start].type!=T_INT) {
         Pike_error("Illegal alpha argument to %s\n",name);
      }
      img->alpha=sp[3-args+args_start].u.integer;
      return 4;
   }
   img->alpha=0;
   return 3;
}


/*** end internals ***/


void img_clear(rgb_group *dest, rgb_group rgb, ptrdiff_t size)
{
  if(!size) return;
  THREADS_ALLOW();
  if( ( rgb.r == rgb.g && rgb.r == rgb.b ) )
    MEMSET(dest, rgb.r, size*sizeof(rgb_group) );
  else if(size)
  {
    int increment = 1;
    rgb_group *from = dest;
    *(dest++)=rgb;
    size -= 1;
    while (size>increment) 
    {
      MEMCPY(dest,from,increment*sizeof(rgb_group));
      size-=increment,dest+=increment;
      if (increment<1024) increment *= 2;
    }
    if(size>0) MEMCPY(dest,from,size*sizeof(rgb_group));
  }
  THREADS_DISALLOW();
}

void img_box_nocheck(INT32 x1,INT32 y1,INT32 x2,INT32 y2)
{
   INT32 x,mod;
   rgb_group *foo,*end,rgb;
   struct image *this;

   this=THIS;
   rgb=this->rgb;
   mod=this->xsize-(x2-x1)-1;
   foo=this->img+x1+y1*this->xsize;
   end=this->img+x2+y2*this->xsize+1;

   if(!this->alpha)
   {
     if(!mod)
       img_clear(foo,rgb,end-foo);
     else {
       THREADS_ALLOW();
       do {
	 int length = x2-x1+1, xs=this->xsize, y=y2-y1+1;
	 rgb_group *from = foo;
	 if(!length)
	   break;	/* Break to the while(0). */
	 for(x=0; x<length; x++)  *(foo+x) = rgb;
	 while(--y)  MEMCPY((foo+=xs), from, length*sizeof(rgb_group)); 
       } while(0);
       THREADS_DISALLOW();
     }
   } 
   else 
   {
     THREADS_ALLOW();
     do {
       for (; foo<=end; foo+=mod) for (x=x1; x<=x2; x++,foo++) 
	 set_rgb_group_alpha(*foo,rgb,this->alpha);
     } while(0);
     THREADS_DISALLOW();
   }
}


void img_blit(rgb_group *dest,rgb_group *src,INT32 width,
	      INT32 lines,INT32 moddest,INT32 modsrc)
{
  if(width <= 0 || lines <= 0)
    return;
CHRONO("image_blit begin");

   THREADS_ALLOW();
   if(!moddest && !modsrc)
     MEMCPY(dest,src,sizeof(rgb_group)*width*lines);
   else
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
   INT32 xp,yp,xs,ys,tmp;

   if (dest->img) { free(dest->img); dest->img=NULL; }

   if (x1>x2) tmp=x1, x1=x2, x2=tmp;
   if (y1>y2) tmp=y1, y1=y2, y2=tmp;

   if (x1==0 && y1==0 &&
       img->xsize-1==x2 && img->ysize-1==y2)
   {
      *dest=*img;
      new=malloc( (x2-x1+1)*(y2-y1+1)*sizeof(rgb_group) + 1);
      if (!new) 
	resource_error(NULL,0,0,"memory",0,"Out of memory.\n");
      THREADS_ALLOW();
      MEMCPY(new,img->img,(x2-x1+1)*(y2-y1+1)*sizeof(rgb_group));
      THREADS_DISALLOW();
      dest->img=new;
      return;
   }

   new=malloc( (x2-x1+1)*(y2-y1+1)*sizeof(rgb_group) +1);
   if (!new)
     resource_error(NULL,0,0,"memory",0,"Out of memory.\n");

   img_clear(new,THIS->rgb,(x2-x1+1)*(y2-y1+1));

   dest->xsize=x2-x1+1;
   dest->ysize=y2-y1+1;

   xp=MAXIMUM(0,-x1);
   yp=MAXIMUM(0,-y1);
   xs=MAXIMUM(0,x1);
   ys=MAXIMUM(0,y1);

   if( ! (( x2 < 0) || (y2 < 0) || (x1>=img->xsize) || (y1>=img->ysize))) {
     
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
     
   }     
   dest->img=new;
}

void img_clone(struct image *newimg,struct image *img)
{
   if (newimg->img) free(newimg->img);
   newimg->img=malloc(sizeof(rgb_group)*img->xsize*img->ysize +1);
   if (!newimg->img) resource_error(NULL,0,0,"memory",0,"Out of memory.\n");
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
**!	image to paste (may be empty, needs to be an image object)
**! arg int x
**! arg int y
**!	where to paste the image; default is 0,0
**!
**! see also: paste_mask, paste_alpha, paste_alpha_color
*/

void image_paste(INT32 args)
{
   struct image *img=NULL;
   INT32 x1,y1,x2,y2,blitwidth,blitheight;

   if (args<1
       || sp[-args].type!=T_OBJECT
       || !(img=(struct image*)get_storage(sp[-args].u.object,image_program)))
      bad_arg_error("image->paste",sp-args,args,1,"",sp+1-1-args,
		"Bad argument 1 to image->paste()\n");
   if (!THIS->img) return;

   if (!img->img) return;

   if (args>1)
   {
      if (args<3 
	  || sp[1-args].type!=T_INT
	  || sp[2-args].type!=T_INT)
         bad_arg_error("image->paste",sp-args,args,0,"",sp-args,
		"Bad arguments to image->paste()\n");
      x1=sp[1-args].u.integer;
      y1=sp[2-args].u.integer;
   }
   else x1=y1=0;

   if(x1 >= THIS->xsize || y1 >= THIS->ysize) /* Per */
   {
     pop_n_elems(args);
     ref_push_object(THISOBJ);
     return;
   }   
   x2=x1+img->xsize-1;
   y2=y1+img->ysize-1;

   if(x2 < 0 || y2 < 0) /* Per */
   {
     pop_n_elems(args);
     ref_push_object(THISOBJ);
     return;
   }   
   blitwidth=MINIMUM(x2,THIS->xsize-1)-MAXIMUM(x1,0)+1;
   blitheight=MINIMUM(y2,THIS->ysize-1)-MAXIMUM(y1,0)+1;
   
   img_blit(THIS->img+MAXIMUM(0,x1)+(THIS->xsize)*MAXIMUM(0,y1),
	    img->img+MAXIMUM(0,-x1)+(x2-x1+1)*MAXIMUM(0,-y1),
	    blitwidth,
	    blitheight,
	    THIS->xsize,
	    img->xsize);

   pop_n_elems(args);
   ref_push_object(THISOBJ);
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
   struct image *img = NULL;
   INT32 x1,y1;

   if (args<2
       || sp[-args].type!=T_OBJECT
       || !sp[-args].u.object
       || !(img=(struct image*)get_storage(sp[-args].u.object,image_program))
       || sp[1-args].type!=T_INT)
      bad_arg_error("image->paste_alpha",sp-args,args,0,"",sp-args,
		"Bad arguments to image->paste_alpha()\n");
   if (!THIS->img) return;
   if (!img->img) return;
   THIS->alpha=(unsigned char)(sp[1-args].u.integer);
   
   if (args>=4)
   {
      if (sp[2-args].type!=T_INT
	  || sp[3-args].type!=T_INT)
         bad_arg_error("image->paste_alpha",sp-args,args,0,"",sp-args,
		"Bad arguments to image->paste_alpha()\n");
      x1=sp[2-args].u.integer;
      y1=sp[3-args].u.integer;
   }
   else x1=y1=0;

   if(x1 >= THIS->xsize || y1 >= THIS->ysize) /* Per */
   {
     pop_n_elems(args);
     ref_push_object(THISOBJ);
     return;
   }   

/* tråda här nåndag.. Ok /Per */

   {
     rgb_group *source = img->img;
     struct image *this = THIS;
     int xs = this->xsize, ix, mx=img->xsize, my=img->ysize, x;
     int ys = this->ysize, iy, y;

     THREADS_ALLOW();
     for (iy=0; iy<my; iy++)
       for (ix=0; ix<mx; ix++)
       {
	 x = ix + x1; y = iy + y1;
	 if(x>=0 && y>=0 && x<xs && y<ys) {
	   if(this->alpha)
	     set_rgb_group_alpha(this->img[x+y*xs],*(source),this->alpha);
	   else
	     this->img[x+y*xs]=*(source);
	 }
	 source++;
       }
     THREADS_DISALLOW();
   }
   pop_n_elems(args);
   ref_push_object(THISOBJ);
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
   struct image *img = NULL, *mask = NULL;
   INT32 x1,y1,x,y,x2,y2,smod,dmod,mmod;
   rgb_group *s,*d,*m;
   double q;

CHRONO("image_paste_mask init");

   if (args<2)
      Pike_error("illegal number of arguments to image->paste_mask()\n");
   if (sp[-args].type!=T_OBJECT
       || !(img=(struct image*)get_storage(sp[-args].u.object,image_program)))
      bad_arg_error("image->paste_mask",sp-args,args,1,"",sp+1-1-args,
		"Bad argument 1 to image->paste_mask()\n");
   if (sp[1-args].type!=T_OBJECT
       || !(mask=(struct image*)get_storage(sp[1-args].u.object,image_program)))
      bad_arg_error("image->paste_mask",sp-args,args,2,"",sp+2-1-args,
		"Bad argument 2 to image->paste_mask()\n");
   if (!THIS->img) return;

   if (!mask->img) return;
   if (!img->img) return;
   
   if (args>=4)
   {
      if (sp[2-args].type!=T_INT
	  || sp[3-args].type!=T_INT)
         Pike_error("illegal coordinate arguments to image->paste_mask()\n");
      x1=sp[2-args].u.integer;
      y1=sp[3-args].u.integer;
   }
   else x1=y1=0;

   x2=MINIMUM(THIS->xsize-x1,MINIMUM(img->xsize,mask->xsize));
   y2=MINIMUM(THIS->ysize-y1,MINIMUM(img->ysize,mask->ysize));

CHRONO("image_paste_mask begin");

   s=img->img+MAXIMUM(0,-x1)+MAXIMUM(0,-y1)*img->xsize;
   m=mask->img+MAXIMUM(0,-x1)+MAXIMUM(0,-y1)*mask->xsize;
   d=THIS->img+MAXIMUM(0,-x1)+x1+(y1+MAXIMUM(0,-y1))*THIS->xsize;
   x=MAXIMUM(0,-x1);
   smod=img->xsize-(x2-x);
   mmod=mask->xsize-(x2-x);
   dmod=THIS->xsize-(x2-x);

   q=1.0/255;

   THREADS_ALLOW();
   for (y=MAXIMUM(0,-y1); y<y2; y++)
   {
      for (x=MAXIMUM(0,-x1); x<x2; x++)
      {
	 if (m->r==255) d->r=s->r;
	 else if (m->r==0) {}
	 else d->r = DOUBLE_TO_COLORTYPE(((d->r*(255-m->r))+(s->r*m->r))*q);
	 if (m->g==255) d->g=s->g;
	 else if (m->g==0) {}
	 else d->g = DOUBLE_TO_COLORTYPE(((d->g*(255-m->g))+(s->g*m->g))*q);
	 if (m->b==255) d->b=s->b;
	 else if (m->b==0) {}
	 else d->b = DOUBLE_TO_COLORTYPE(((d->b*(255-m->b))+(s->b*m->b))*q);
	 s++; m++; d++;
      }
      s+=smod; m+=mmod; d+=dmod;
   }
   THREADS_DISALLOW();
CHRONO("image_paste_mask end");

   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*
**! method object paste_alpha_color(object mask)
**! method object paste_alpha_color(object mask,int x,int y)
**! method object paste_alpha_color(object mask,int r,int g,int b)
**! method object paste_alpha_color(object mask,int r,int g,int b,int x,int y)
**! method object paste_alpha_color(object mask,Color color)
**! method object paste_alpha_color(object mask,Color color,int x,int y)
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
   struct image *mask = NULL;
   INT32 x1,y1,x,y,x2,y2;
   rgb_group rgb,*d,*m;
   INT32 mmod,dmod;
   double q;
   int arg=1;

   if (args<1)
      SIMPLE_TOO_FEW_ARGS_ERROR("image->paste_alpha_color",1);
   if (sp[-args].type!=T_OBJECT
       || !sp[-args].u.object
       || !(mask=(struct image*)get_storage(sp[-args].u.object,image_program)))
      bad_arg_error("image->paste_alpha_color",sp-args,args,1,"",sp+1-1-args,
		"Bad argument 1 to image->paste_alpha_color()\n");
   if (!THIS->img) return;
   if (!mask->img) return;

   if (args==6 || args==4 || args==2 || args==3) /* color at arg 2.. */
      arg=1+getrgb(THIS,1,args,3,"image->paste_alpha_color()\n");
   if (args>arg+1) 
   {
      if (sp[arg-args].type!=T_INT
	  || sp[1+arg-args].type!=T_INT)
         Pike_error("illegal coordinate arguments to image->paste_alpha_color()\n");
      x1=sp[arg-args].u.integer;
      y1=sp[1+arg-args].u.integer;
   }
   else x1=y1=0;
   
   x2=MINIMUM(THIS->xsize-x1,mask->xsize);
   y2=MINIMUM(THIS->ysize-y1,mask->ysize);

CHRONO("image_paste_alpha_color begin");

   m=mask->img+MAXIMUM(0,-x1)+MAXIMUM(0,-y1)*mask->xsize;
   d=THIS->img+MAXIMUM(0,-x1)+x1+(y1+MAXIMUM(0,-y1))*THIS->xsize;
   x=MAXIMUM(0,-x1);
   mmod=mask->xsize-(x2-x);
   dmod=THIS->xsize-(x2-x);

   q=1.0/255;

   rgb=THIS->rgb;

   THREADS_ALLOW();
   for (y=MAXIMUM(0,-y1); y<y2; y++)
   {
      for (x=MAXIMUM(0,-x1); x<x2; x++)
      {
	 if (m->r==255) d->r=rgb.r;
	 else if (m->r==0) ;
	 else d->r = DOUBLE_TO_COLORTYPE(((d->r*(255-m->r))+(rgb.r*m->r))*q);
	 if (m->g==255) d->g=rgb.g;
	 else if (m->g==0) ;
	 else d->g = DOUBLE_TO_COLORTYPE(((d->g*(255-m->g))+(rgb.g*m->g))*q);
	 if (m->b==255) d->b=rgb.b;
	 else if (m->b==0) ;
	 else d->b = DOUBLE_TO_COLORTYPE(((d->b*(255-m->b))+(rgb.b*m->b))*q);
	 m++; d++;
      }
      m+=mmod; d+=dmod;
   }
   THREADS_DISALLOW();
CHRONO("image_paste_alpha_color end");

   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

void img_box(INT32 x1,INT32 y1,INT32 x2,INT32 y2)
{   
   if (x1>x2) x1^=x2,x2^=x1,x1^=x2;
   if (y1>y2) y1^=y2,y2^=y1,y1^=y2;
   if (x2 >= THIS->xsize) x2 = THIS->xsize-1;
   if (y2 >= THIS->ysize) y2 = THIS->ysize-1;
   if (x2<0||y2<0||x1>=THIS->xsize||y1>=THIS->ysize) return;
   if (x1<0) x1 = 0;
   if (y1<0) y1 = 0;
   img_box_nocheck(MAXIMUM(x1,0),MAXIMUM(y1,0),MINIMUM(x2,THIS->xsize-1),MINIMUM(y2,THIS->ysize-1));
}
