#include "global.h"

#include <math.h>
#include <ctype.h>

#include "stralloc.h"
#include "global.h"
#include "types.h"
#include "macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "array.h"
#include "error.h"

#include "image.h"

struct program *image_program;
#define THIS ((struct image *)(fp->current_storage))
#define THISOBJ (fp->current_object)

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)<(b)?(b):(a))

#undef MATRIX_CHRONO
#ifdef MATRIX_CHRONO
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
   if (args-args_start<3) return 0;
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
   return 1;
}

static INLINE int getrgbl(rgbl_group *rgb,INT32 args_start,INT32 args,char *name)
{
   INT32 i;
   if (args-args_start<3) return 0;
   for (i=0; i<3; i++)
      if (sp[-args+i+args_start].type!=T_INT)
         error("Illegal r,g,b argument to %s\n",name);
   rgb->r=sp[-args+args_start].u.integer;
   rgb->g=sp[1-args+args_start].u.integer;
   rgb->b=sp[2-args+args_start].u.integer;
   return 1;
}

/** end internals **/


#define decimals(x) ((x)-(int)(x))
#define testrange(x) max(min((x),255),0)
#define _scale_add_rgb(dest,src,factor) \
   ((dest).r=testrange((dest).r+(int)((src).r*(factor)+0.5)), \
    (dest).g=testrange((dest).g+(int)((src).g*(factor)+0.5)), \
    (dest).b=testrange((dest).b+(int)((src).b*(factor)+0.5))) 
#define scale_add_pixel(dest,dx,dy,dxw,src,sx,sy,sxw,factor) \
   _scale_add_rgb(dest[(dx)+(dy)*(dxw)],src[(sx)+(sy)*(sxw)],factor)

static INLINE void scale_add_line(rgb_group *new,INT32 yn,INT32 newx,
				  rgb_group *img,INT32 y,INT32 xsize,
				  double py,double dx)
{
   INT32 x,xd;
   double xn;
   for (x=0,xn=0; x<THIS->xsize; x++,xn+=dx)
   {
      if ((INT32)xn<(INT32)(xn+dx))
      {
         scale_add_pixel(new,(INT32)xn,yn,newx,img,x,y,xsize,py*(1.0-decimals(xn)));
	 if ((xd=(INT32)(xn+dx)-(INT32)(xn))>1) 
            while (--xd)
               scale_add_pixel(new,(INT32)xn+xd,yn,newx,img,x,y,xsize,py);
	 scale_add_pixel(new,(INT32)(xn+dx),yn,newx,img,x,y,xsize,py*decimals(xn+dx));
      }
      else
         scale_add_pixel(new,(int)xn,yn,newx,img,x,y,xsize,py*dx);
   }
}

void img_scale(struct image *dest,
	       struct image *source,
	       INT32 newx,INT32 newy)
{
   rgb_group *new;
   INT32 y,yd;
   double yn,dx,dy;

   if (dest->img) { free(dest->img); dest->img=NULL; }

   if (!THIS->img || newx<=0 || newy<=0) return; /* no way */
   new=malloc(newx*newy*sizeof(rgb_group) +1);
   if (!new) error("Out of memory!\n");

   MEMSET(new,0,newx*newy*sizeof(rgb_group));
   
   dx=((double)newx-0.000001)/source->xsize; 
   dy=((double)newy-0.000001)/source->ysize; 

   for (y=0,yn=0; y<source->ysize; y++,yn+=dy)
   {
      if ((INT32)yn<(INT32)(yn+dy))
      {
	 scale_add_line(new,(INT32)(yn),newx,source->img,y,source->xsize,
			(1.0-decimals(yn)),dx);
	 if ((yd=(INT32)(yn+dy)-(INT32)(yn))>1) 
            while (--yd)
   	       scale_add_line(new,(INT32)yn+yd,newx,source->img,y,source->xsize,
			      1.0,dx);
	 scale_add_line(new,(INT32)(yn+dy),newx,source->img,y,source->xsize,
			(decimals(yn+dy)),dx);
      }
      else
	 scale_add_line(new,(INT32)yn,newx,source->img,y,source->xsize,
			dy,dx);
   }

   dest->img=new;
   dest->xsize=newx;
   dest->ysize=newy;
}

/* Special, faster, case for scale=1/2 */
void img_scale2(struct image *dest, struct image *source)
{
   rgb_group *new;
   INT32 x, y, newx, newy;
   newx = source->xsize >> 1;
   newy = source->ysize >> 1;
   
   if (dest->img) { free(dest->img); dest->img=NULL; }
   if (!THIS->img || newx<=0 || newy<=0) return; /* no way */
   new=malloc(newx*newy*sizeof(rgb_group) +1);
   if (!new) error("Out of memory\n");
   MEMSET(new,0,newx*newy*sizeof(rgb_group));

   dest->img=new;
   dest->xsize=newx;
   dest->ysize=newy;
   for (y = 0; y < newy; y++)
     for (x = 0; x < newx; x++) {
       pixel(dest,x,y).r = (COLOURTYPE)
	                    (((INT32) pixel(source,2*x+0,2*y+0).r+
			      (INT32) pixel(source,2*x+1,2*y+0).r+
			      (INT32) pixel(source,2*x+0,2*y+1).r+
			      (INT32) pixel(source,2*x+1,2*y+1).r) >> 2);
       pixel(dest,x,y).g = (COLOURTYPE)
	                    (((INT32) pixel(source,2*x+0,2*y+0).g+
			      (INT32) pixel(source,2*x+1,2*y+0).g+
			      (INT32) pixel(source,2*x+0,2*y+1).g+
			      (INT32) pixel(source,2*x+1,2*y+1).g) >> 2);
       pixel(dest,x,y).b = (COLOURTYPE)
	                    (((INT32) pixel(source,2*x+0,2*y+0).b+
			      (INT32) pixel(source,2*x+1,2*y+0).b+
			      (INT32) pixel(source,2*x+0,2*y+1).b+
			      (INT32) pixel(source,2*x+1,2*y+1).b) >> 2);
     }
}



void image_scale(INT32 args)
{
   float factor;
   struct object *o;
   struct image *newimg;
   
   o=clone(image_program,0);
   newimg=(struct image*)(o->storage);

   if (args==1 && sp[-args].type==T_FLOAT) {
     if (sp[-args].u.float_number == 0.5)
       img_scale2(newimg,THIS);
     else
       img_scale(newimg,THIS,
		 (INT32)(THIS->xsize*sp[-args].u.float_number),
		 (INT32)(THIS->ysize*sp[-args].u.float_number));
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
      img_scale(newimg,THIS,
		(INT32)(THIS->xsize*sp[-args].u.float_number),
		(INT32)(THIS->ysize*sp[1-args].u.float_number));
   else if (args>=2 &&
	    sp[-args].type==T_INT &&
	    sp[1-args].type==T_INT)
      img_scale(newimg,THIS,
		sp[-args].u.integer,
		sp[1-args].u.integer);
   else
   {
      free_object(o);
      error("illegal arguments to image->scale()\n");
   }
   pop_n_elems(args);
   push_object(o);
}

void image_ccw(INT32 args)
{
   INT32 i,j;
   rgb_group *src,*dest;
   struct object *o;
   struct image *img;

   pop_n_elems(args);

   if (!THIS->img) error("no image\n");

   o=clone(image_program,0);
   img=(struct image*)o->storage;
   *img=*THIS;
   if (!(img->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize+1)))
   {
      free_object(o);
      error("Out of memory\n");
   }
   img->xsize=THIS->ysize;
   img->ysize=THIS->xsize;
   i=THIS->xsize;
   src=THIS->img+THIS->xsize-1;
   dest=img->img;
   while (i--)
   {
      j=THIS->ysize;
      while (j--) *(dest++)=*(src),src+=THIS->xsize;
      src--;
      src-=THIS->xsize*THIS->ysize;
   }

   push_object(o);
}

static void img_cw(struct image *is,struct image *id)
{
   INT32 i,j;
   rgb_group *src,*dest;

   if (id->img) free(id->img);
   *id=*is;
   if (!(id->img=malloc(sizeof(rgb_group)*is->xsize*is->ysize+1)))
      error("Out of memory\n");

   id->xsize=is->ysize;
   id->ysize=is->xsize;
   i=is->xsize;
   src=is->img+is->xsize-1;
   dest=id->img;
   while (i--)
   {
      j=is->ysize;
      while (j--) *(dest++)=*(src),src+=is->xsize;
      src--;
      src-=is->xsize*is->ysize;
   }
}

void img_ccw(struct image *is,struct image *id)
{
   INT32 i,j;
   rgb_group *src,*dest;

   if (id->img) free(id->img);
   *id=*is;
   if (!(id->img=malloc(sizeof(rgb_group)*is->xsize*is->ysize+1)))
      error("Out of memory\n");

   id->xsize=is->ysize;
   id->ysize=is->xsize;
   i=is->xsize;
   src=is->img+is->xsize-1;
   dest=id->img+is->xsize*is->ysize;
   while (i--)
   {
      j=is->ysize;
      while (j--) *(--dest)=*(src),src+=is->xsize;
      src--;
      src-=is->xsize*is->ysize;
   }
}

void image_cw(INT32 args)
{
   INT32 i,j;
   rgb_group *src,*dest;
   struct object *o;
   struct image *img;

   pop_n_elems(args);

   if (!THIS->img) error("no image\n");

   o=clone(image_program,0);
   img=(struct image*)o->storage;
   *img=*THIS;
   if (!(img->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize+1)))
   {
      free_object(o);
      error("Out of memory\n");
   }
   img->xsize=THIS->ysize;
   img->ysize=THIS->xsize;
   i=THIS->xsize;
   src=THIS->img+THIS->xsize-1;
   dest=img->img+THIS->xsize*THIS->ysize;
   while (i--)
   {
      j=THIS->ysize;
      while (j--) *(--dest)=*(src),src+=THIS->xsize;
      src--;
      src-=THIS->xsize*THIS->ysize;
   }

   push_object(o);
}

void image_mirrorx(INT32 args)
{
   rgb_group *src,*dest;
   struct object *o;
   struct image *img;
   INT32 i,j;

   pop_n_elems(args);

   if (!THIS->img) error("no image\n");

   o=clone(image_program,0);
   img=(struct image*)o->storage;
   *img=*THIS;
   if (!(img->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize+1)))
   {
      free_object(o);
      error("Out of memory\n");
   }
   
   i=THIS->ysize;
   src=THIS->img+THIS->xsize-1;
   dest=img->img;
   while (i--)
   {
      j=THIS->xsize;
      while (j--) *(dest++)=*(src--);
      src+=THIS->xsize*2;
   }

   push_object(o);
}

void image_mirrory(INT32 args)
{
   rgb_group *src,*dest;
   struct object *o;
   struct image *img;
   INT32 i,j;

   pop_n_elems(args);

   if (!THIS->img) error("no image\n");

   o=clone(image_program,0);
   img=(struct image*)o->storage;
   *img=*THIS;
   if (!(img->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize+1)))
   {
      free_object(o);
      error("Out of memory\n");
   }
   
   i=THIS->ysize;
   src=THIS->img+THIS->xsize*(THIS->ysize-1);
   dest=img->img;
   while (i--)
   {
      j=THIS->xsize;
      while (j--) *(dest++)=*(src++);
      src-=THIS->xsize*2;
   }

   push_object(o);
 
}


#define ROUND(X) ((unsigned char)((X)+0.5))

static void img_skewx(struct image *src,
		      struct image *dest,
		      float diff,
		      int xpn) /* expand pixel for use with alpha instead */
{
   double x0,xmod,xm;
   INT32 y,x,len;
   rgb_group *s,*d;
   rgb_group rgb;

   if (dest->img) free(dest->img);
   if (diff<0) 
      dest->xsize=ceil(-diff)+src->xsize,x0=-diff;
   else 
      dest->xsize=ceil(diff)+src->xsize,x0=0;
   dest->ysize=src->ysize;
   len=src->xsize;

   d=dest->img=malloc(sizeof(rgb_group)*dest->xsize*dest->ysize);
   if (!d) return;
   s=src->img;

   xmod=diff/src->ysize;
   rgb=dest->rgb;

   CHRONO("skewx begin\n");

   y=src->ysize;
   while (y--)
   {
      int j;
      if (xpn) rgb=*s;
      for (j=x0; j--;) *(d++)=rgb;
      if (!(xm=(x0-floor(x0))))
      {
	 for (j=len; j--;) *(d++)=*(s++);
	 j=dest->xsize-x0-len;
      }
      else
      {
	 float xn=1-xm;
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
	 j=dest->xsize-x0-len;
      }
      if (xpn) rgb=s[-1];
      while (j--) *(d++)=rgb;
      x0+=xmod;
   }

   CHRONO("skewx end\n");
}

static void img_skewy(struct image *src,
		      struct image *dest,
		      float diff,
		      int xpn) /* expand pixel for use with alpha instead */
{
   double y0,ymod,ym;
   INT32 y,x,len,xsz;
   rgb_group *s,*d;
   rgb_group rgb;

   if (dest->img) free(dest->img);
   if (diff<0) 
      dest->ysize=ceil(-diff)+src->ysize,y0=-diff;
   else 
      dest->ysize=ceil(diff)+src->ysize,y0=0;
   xsz=dest->xsize=src->xsize;
   len=src->ysize;

   d=dest->img=malloc(sizeof(rgb_group)*dest->ysize*dest->xsize);
   if (!d) return;
   s=src->img;
   
   ymod=diff/src->xsize;
   rgb=dest->rgb;

CHRONO("skewy begin\n");

   x=src->xsize;
   while (x--)
   {
      int j;
      if (xpn) rgb=*s;
      for (j=y0; j--;) *d=rgb,d+=xsz;
      if (!(ym=(y0-floor(y0))))
      {
	 for (j=len; j--;) *d=*s,d+=xsz,s+=xsz;
	 j=dest->ysize-y0-len;
      }
      else
      {
	 float yn=1-ym;
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
	 j=dest->ysize-y0-len;
      }
      if (xpn) rgb=s[-xsz];
      while (j--) *d=rgb,d+=xsz;
      s-=len*xsz-1;
      d-=dest->ysize*xsz-1;
      y0+=ymod;
   }

CHRONO("skewy end\n");

}

void image_skewx(INT32 args)
{
   float diff;
   struct object *o;

   if (args<1)
      error("too few arguments to image->skewx()\n");
   else if (sp[-args].type==T_FLOAT)
      diff=THIS->ysize*sp[-args].u.float_number;
   else if (sp[-args].type==T_INT)
      diff=sp[-args].u.integer;
   else
      error("illegal argument to image->skewx()\n");

   if (!THIS->img) error("no image\n");

   o=clone(image_program,0);

   if (!getrgb((struct image*)(o->storage),1,args,"image->skewx()"))
      ((struct image*)(o->storage))->rgb=THIS->rgb;

   img_skewx(THIS,(struct image*)(o->storage),diff,0);

   pop_n_elems(args);
   push_object(o);
}

void image_skewy(INT32 args)
{
   float diff;
   struct object *o;

   if (args<1)
      error("too few arguments to image->skewy()\n");
   else if (sp[-args].type==T_FLOAT)
      diff=THIS->xsize*sp[-args].u.float_number;
   else if (sp[-args].type==T_INT)
      diff=sp[-args].u.integer;
   else
      error("illegal argument to image->skewx()\n");

   if (!THIS->img) error("no image\n");

   o=clone(image_program,0);

   if (!getrgb((struct image*)(o->storage),1,args,"image->skewy()"))
      ((struct image*)(o->storage))->rgb=THIS->rgb;

   img_skewy(THIS,(struct image*)(o->storage),diff,0);

   pop_n_elems(args);
   push_object(o);
}

void image_skewx_expand(INT32 args)
{
   float diff;
   struct object *o;

   if (args<1)
      error("too few arguments to image->skewx()\n");
   else if (sp[-args].type==T_FLOAT)
      diff=THIS->ysize*sp[-args].u.float_number;
   else if (sp[-args].type==T_INT)
      diff=sp[-args].u.integer;
   else
      error("illegal argument to image->skewx()\n");

   if (!THIS->img) error("no image\n");

   o=clone(image_program,0);

   if (!getrgb((struct image*)(o->storage),1,args,"image->skewx()"))
      ((struct image*)(o->storage))->rgb=THIS->rgb;

   img_skewx(THIS,(struct image*)(o->storage),diff,1);

   pop_n_elems(args);
   push_object(o);
}

void image_skewy_expand(INT32 args)
{
   float diff;
   struct object *o;

   if (args<1)
      error("too few arguments to image->skewy()\n");
   else if (sp[-args].type==T_FLOAT)
      diff=THIS->xsize*sp[-args].u.float_number;
   else if (sp[-args].type==T_INT)
      diff=sp[-args].u.integer;
   else
      error("illegal argument to image->skewx()\n");

   if (!THIS->img) error("no image\n");

   o=clone(image_program,0);

   if (!getrgb((struct image*)(o->storage),1,args,"image->skewy()"))
      ((struct image*)(o->storage))->rgb=THIS->rgb;

   img_skewy(THIS,(struct image*)(o->storage),diff,1);

   pop_n_elems(args);
   push_object(o);
}



void img_rotate(INT32 args,int xpn)
{
   float angle;
   struct object *o;
   struct image *dest,d0,dest2;

   if (args<1)
      error("too few arguments to image->rotate()\n");
   else if (sp[-args].type==T_FLOAT)
      angle=sp[-args].u.float_number;
   else if (sp[-args].type==T_INT)
      angle=sp[-args].u.integer;
   else
      error("illegal argument to image->rotate()\n");

   if (!THIS->img) error("no image\n");

   dest2.img=d0.img=NULL;

   if (angle<-135) angle-=360*(int)((angle-225)/360);
   else if (angle>225) angle-=360*(int)((angle+135)/360);
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

   o=clone(image_program,0);

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

void image_rotate(INT32 args)
{
   img_rotate(args,0);
}

void image_rotate_expand(INT32 args)
{
   img_rotate(args,1);
}

