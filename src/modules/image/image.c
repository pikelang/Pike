/* $Id: image.c,v 1.36 1997/01/07 00:41:41 law Exp $ */

#include "global.h"

#include <math.h>
#include <ctype.h>

#include "stralloc.h"
#include "global.h"
RCSID("$Id: image.c,v 1.36 1997/01/07 00:41:41 law Exp $");
#include "types.h"
#include "macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "threads.h"
#include "array.h"
#include "error.h"

#include "image.h"
#include "builtin_functions.h"

struct program *image_program;
#define THIS ((struct image *)(fp->current_storage))
#define THISOBJ (fp->current_object)

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)<(b)?(b):(a))
#define testrange(x) max(min((x),255),0)

#define sq(x) ((x)*(x))

#define CIRCLE_STEPS 128
static INT32 circle_sin_table[CIRCLE_STEPS];
#define circle_sin(x) circle_sin_table[((x)+CIRCLE_STEPS)%CIRCLE_STEPS]
#define circle_cos(x) circle_sin((x)-CIRCLE_STEPS/4)

#define circle_sin_mul(x,y) ((circle_sin(x)*(y))/4096)
#define circle_cos_mul(x,y) ((circle_cos(x)*(y))/4096)



#if 1
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


/***************** init & exit *********************************/

static int obj_counter=0;

static void init_image_struct(struct object *obj)
{
  THIS->img=NULL;
  THIS->rgb.r=0;
  THIS->rgb.g=0;
  THIS->rgb.b=0;
/*  fprintf(stderr,"init %lx (%d)\n",obj,++obj_counter);*/
}

static void exit_image_struct(struct object *obj)
{
  if (THIS->img) { free(THIS->img); THIS->img=NULL; }
/*
  fprintf(stderr,"exit %lx (%d) %dx%d=%.1fKb\n",obj,--obj_counter,
	  THIS->xsize,THIS->ysize,
	  (THIS->xsize*THIS->ysize*3+sizeof(struct image))/1024.0);
	  */
}

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

static INLINE void getrgbl(rgbl_group *rgb,INT32 args_start,INT32 args,char *name)
{
   INT32 i;
   if (args-args_start<3) return;
   for (i=0; i<3; i++)
      if (sp[-args+i+args_start].type!=T_INT)
         error("Illegal r,g,b argument to %s\n",name);
   rgb->r=sp[-args+args_start].u.integer;
   rgb->g=sp[1-args+args_start].u.integer;
   rgb->b=sp[2-args+args_start].u.integer;
}

static INLINE void img_line(INT32 x1,INT32 y1,INT32 x2,INT32 y2)
{   
   INT32 pixelstep,pos;
   if (x1==x2) 
   {
      if (y1>y2) y1^=y2,y2^=y1,y1^=y2;
      if (x1<0||x1>=THIS->xsize||
	  y2<0||y1>=THIS->ysize) return;
      if (y1<0) y1=0;
      if (y2>=THIS->ysize) y2=THIS->ysize-1;
      for (;y1<=y2;y1++) setpixel_test(x1,y1);
      return;
   }
   else if (y1==y2)
   {
      if (x1>x2) x1^=x2,x2^=x1,x1^=x2;
      if (y1<0||y1>=THIS->ysize||
	  x2<0||x1>=THIS->xsize) return;
      if (x1<0) x1=0; 
      if (x2>=THIS->xsize) x2=THIS->xsize-1;
      for (;x1<=x2;x1++) setpixel_test(x1,y1);
      return;
   }
   else if (abs(x2-x1)<abs(y2-y1)) /* mostly vertical line */
   {
      if (y1>y2) y1^=y2,y2^=y1,y1^=y2,
                 x1^=x2,x2^=x1,x1^=x2;
      pixelstep=((x2-x1)*1024)/(y2-y1);
      pos=x1*1024;
      for (;y1<=y2;y1++)
      {
         setpixel_test((pos+512)/1024,y1);
	 pos+=pixelstep;
      }
   }
   else /* mostly horisontal line */
   {
      if (x1>x2) y1^=y2,y2^=y1,y1^=y2,
                 x1^=x2,x2^=x1,x1^=x2;
      pixelstep=((y2-y1)*1024)/(x2-x1);
      pos=y1*1024;
      for (;x1<=x2;x1++)
      {
         setpixel_test(x1,(pos+512)/1024);
	 pos+=pixelstep;
      }
   }
}

static INLINE rgb_group _pixel_apply_matrix(struct image *img,
					    int x,int y,
					    int width,int height,
					    rgbd_group *matrix,
					    rgb_group default_rgb,
					    double div)
{
   rgb_group res;
   int i,j,bx,by,xp,yp;
   int sumr,sumg,sumb,r,g,b;
   float qdiv=1.0/div;

   sumr=sumg=sumb=0;
   r=g=b=0;

   bx=width/2;
   by=height/2;
   
   for (xp=x-bx,i=0; i<width; i++,xp++)
      for (yp=y-by,j=0; j<height; j++,yp++)
	 if (xp>=0 && xp<img->xsize && yp>=0 && yp<img->ysize)
	 {
	    r+=matrix[i+j*width].r*img->img[xp+yp*img->xsize].r;
	    g+=matrix[i+j*width].g*img->img[xp+yp*img->xsize].g;
	    b+=matrix[i+j*width].b*img->img[xp+yp*img->xsize].b;
#ifdef MATRIX_DEBUG
	    fprintf(stderr,"%d,%d %d,%d->%d,%d,%d\n",
		    i,j,xp,yp,
		    img->img[x+i+(y+j)*img->xsize].r,
		    img->img[x+i+(y+j)*img->xsize].g,
		    img->img[x+i+(y+j)*img->xsize].b);
#endif
	    sumr+=matrix[i+j*width].r;
	    sumg+=matrix[i+j*width].g;
	    sumb+=matrix[i+j*width].b;
	 }
   if (sumr) res.r=testrange(default_rgb.r+r/(sumr*div)); 
   else res.r=testrange(r*qdiv+default_rgb.r);
   if (sumg) res.g=testrange(default_rgb.g+g/(sumg*div)); 
   else res.g=testrange(g*qdiv+default_rgb.g);
   if (sumb) res.b=testrange(default_rgb.g+b/(sumb*div)); 
   else res.b=testrange(b*qdiv+default_rgb.b);
#ifdef MATRIX_DEBUG
   fprintf(stderr,"->%d,%d,%d\n",res.r,res.g,res.b);
#endif
   return res;
}


void img_apply_matrix(struct image *dest,
		      struct image *img,
		      int width,int height,
		      rgbd_group *matrix,
		      rgb_group default_rgb,
		      double div)
{
   rgb_group *d,*ip,*dp;
   rgbd_group *mp;
   int i,j,x,y,bx,by,ex,ey,xp,yp;
   int sumr,sumg,sumb;
   double qr,qg,qb;
   register double r=0,g=0,b=0;

THREADS_ALLOW();

   sumr=sumg=sumb=0;
   for (i=0; i<width; i++)
      for (j=0; j<height; j++)
      {
	 sumr+=matrix[i+j*width].r;
	 sumg+=matrix[i+j*width].g;
	 sumb+=matrix[i+j*width].b;
      }

   if (!sumr) sumr=1; sumr*=div; qr=1.0/sumr;
   if (!sumg) sumg=1; sumg*=div; qg=1.0/sumg;
   if (!sumb) sumb=1; sumb*=div; qb=1.0/sumb;

   bx=width/2;
   by=height/2;
   ex=width-bx;
   ey=height-by;
   
   d=malloc(sizeof(rgb_group)*img->xsize*img->ysize +1);

   if(!d) error("Out of memory.\n");
   
CHRONO("apply_matrix, one");

   for (y=by; y<img->ysize-ey; y++)
   {
      dp=d+y*img->xsize+by;
      for (x=bx; x<img->xsize-ex; x++)
      {
	 r=g=b=0;
	 mp=matrix;
	 for (yp=y-by,j=0; j<height; j++,yp++)
	 {
	    ip=img->img+(x-bx)+yp*img->xsize;
	    for (i=0; i<width; i++)
	    {
	       r+=ip->r*mp->r;
 	       g+=ip->g*mp->g;
 	       b+=ip->b*mp->b;
#ifdef MATRIX_DEBUG
	       fprintf(stderr,"%d,%d ->%d,%d,%d\n",
		       i,j,
		       img->img[x+i+(y+j)*img->xsize].r,
		       img->img[x+i+(y+j)*img->xsize].g,
		       img->img[x+i+(y+j)*img->xsize].b);
#endif
	       mp++;
	       ip++;
	    }
	 }
#ifdef MATRIX_DEBUG
	 fprintf(stderr,"->%d,%d,%d\n",r/sumr,g/sumg,b/sumb);
#endif
	 r=default_rgb.r+(int)(r*qr+0.5); dp->r=testrange(r);
	 g=default_rgb.g+(int)(g*qg+0.5); dp->g=testrange(g);
	 b=default_rgb.b+(int)(b*qb+0.5); dp->b=testrange(b);
	 dp++;
      }
   }

CHRONO("apply_matrix, two");

   for (y=0; y<img->ysize; y++)
   {
      for (x=0; x<bx; x++)
	 d[x+y*img->xsize]=_pixel_apply_matrix(img,x,y,width,height,
					       matrix,default_rgb,div);
      for (x=img->xsize-ex; x<img->xsize; x++)
	 d[x+y*img->xsize]=_pixel_apply_matrix(img,x,y,width,height,
					       matrix,default_rgb,div);
   }

   for (x=0; x<img->xsize; x++)
   {
      for (y=0; y<by; y++)
	 d[x+y*img->xsize]=_pixel_apply_matrix(img,x,y,width,height,
					       matrix,default_rgb,div);
      for (y=img->ysize-ey; y<img->ysize; y++)
	 d[x+y*img->xsize]=_pixel_apply_matrix(img,x,y,width,height,
					       matrix,default_rgb,div);
   }

CHRONO("apply_matrix, three");

   if (dest->img) free(dest->img);
   *dest=*img;
   dest->img=d;

THREADS_DISALLOW();
}

/***************** methods *************************************/

void image_create(INT32 args)
{
   if (args<2) return;
   if (sp[-args].type!=T_INT||
       sp[1-args].type!=T_INT)
      error("Illegal arguments to image->clone()\n");

   getrgb(THIS,2,args,"image->create()"); 

   if (THIS->img) free(THIS->img);
	
   THIS->xsize=sp[-args].u.integer;
   THIS->ysize=sp[1-args].u.integer;
   if (THIS->xsize<0) THIS->xsize=0;
   if (THIS->ysize<0) THIS->ysize=0;

   THIS->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize +1);
   if (!THIS->img)
     error("out of memory\n");


   img_clear(THIS->img,THIS->rgb,THIS->xsize*THIS->ysize);
   pop_n_elems(args);
}

void image_clone(INT32 args)
{
   struct object *o;
   struct image *img;

   if (args)
      if (args<2||
	  sp[-args].type!=T_INT||
	  sp[1-args].type!=T_INT)
	 error("Illegal arguments to image->clone()\n");

   o=clone(image_program,0);
   img=(struct image*)(o->storage);
   *img=*THIS;

   if (args)
   {
      if(sp[-args].u.integer < 0 ||
	 sp[1-args].u.integer < 0)
	 error("Illegal size to image->clone()\n");
      img->xsize=sp[-args].u.integer;
      img->ysize=sp[1-args].u.integer;
   }

   getrgb(img,2,args,"image->clone()"); 

   if (img->xsize<0) img->xsize=1;
   if (img->ysize<0) img->ysize=1;

   img->img=malloc(sizeof(rgb_group)*img->xsize*img->ysize +1);
   if (THIS->img)
   {
      if (!img->img)
      {
	 free_object(o);
	 error("out of memory\n");
      }
      if (img->xsize==THIS->xsize 
	  && img->ysize==THIS->ysize)
	 MEMCPY(img->img,THIS->img,sizeof(rgb_group)*img->xsize*img->ysize);
      else
	 img_crop(img,THIS,
		  0,0,img->xsize-1,img->ysize-1);
	 
   }
   else
      img_clear(img->img,img->rgb,img->xsize*img->ysize);

   pop_n_elems(args);
   push_object(o);
}

void image_clear(INT32 args)
{
   struct object *o;
   struct image *img;

   o=clone(image_program,0);
   img=(struct image*)(o->storage);
   *img=*THIS;

   getrgb(img,0,args,"image->clear()"); 

   img->img=malloc(sizeof(rgb_group)*img->xsize*img->ysize +1);
   if (!img->img)
   {
     free_object(o);
     error("out of memory\n");
   }

   img_clear(img->img,img->rgb,img->xsize*img->ysize);

   pop_n_elems(args);
   push_object(o);
}

void image_copy(INT32 args)
{
   struct object *o;
   struct image *img;

   if (!args)
   {
      o=clone(image_program,0);
      if (THIS->img) img_clone((struct image*)o->storage,THIS);
      pop_n_elems(args);
      push_object(o);
      return;
   }
   if (args<4||
       sp[-args].type!=T_INT||
       sp[1-args].type!=T_INT||
       sp[2-args].type!=T_INT||
       sp[3-args].type!=T_INT)
      error("illegal arguments to image->copy()\n");

   if (!THIS->img) error("no image\n");

   getrgb(THIS,2,args,"image->crop()"); 

   o=clone(image_program,0);
   img=(struct image*)(o->storage);

   img_crop(img,THIS,
	    sp[-args].u.integer,sp[1-args].u.integer,
	    sp[2-args].u.integer,sp[3-args].u.integer);

   pop_n_elems(args);
   push_object(o);
}

static void image_change_color(INT32 args)

{
   /* ->change_color([int from-r,g,b,] int to-r,g,b); */
   rgb_group from,to,*s,*d;
   INT32 left;
   struct object *o;
   struct image *img;

   if (!THIS->img) error("no image\n");
   if (args<3) error("too few arguments to image->change_color()\n");

   if (args<6)
   {
      to=THIS->rgb;   
      getrgb(THIS,0,args,"image->change_color()");
      from=THIS->rgb;
   }
   else
   {
      getrgb(THIS,0,args,"image->change_color()");
      from=THIS->rgb;
      getrgb(THIS,3,args,"image->change_color()");
      to=THIS->rgb;
   }
   
   o=clone(image_program,0);
   img=(struct image*)(o->storage);
   *img=*THIS;

   if (!(img->img=malloc(sizeof(rgb_group)*img->xsize*img->ysize +1)))
   {
      free_object(o);
      error("out of memory\n");
   }

   left=THIS->xsize*THIS->ysize;
   s=THIS->img;
   d=img->img;
   while (left--)
   {
      if (s->r==from.r && s->g==from.g && s->b==from.b)
         *d=to;
      else
         *d=*s;
      d++; s++;
   }

   pop_n_elems(args);
   push_object(o);
}

static INLINE int try_autocrop_vertical(INT32 x,INT32 y,INT32 y2,
					INT32 rgb_set,rgb_group *rgb)
{
   if (!rgb_set) *rgb=pixel(THIS,x,y);
   for (;y<=y2; y++)
      if (pixel(THIS,x,y).r!=rgb->r ||
	  pixel(THIS,x,y).g!=rgb->g ||
	  pixel(THIS,x,y).b!=rgb->b) return 0;
   return 1;
}

static INLINE int try_autocrop_horisontal(INT32 y,INT32 x,INT32 x2,
					  INT32 rgb_set,rgb_group *rgb)
{
   if (!rgb_set) *rgb=pixel(THIS,x,y);
   for (;x<=x2; x++)
      if (pixel(THIS,x,y).r!=rgb->r ||
	  pixel(THIS,x,y).g!=rgb->g ||
	  pixel(THIS,x,y).b!=rgb->b) return 0;
   return 1;
}

void image_autocrop(INT32 args)
{
   INT32 border=0,x1,y1,x2,y2;
   rgb_group rgb;
   int rgb_set=0,done;
   struct object *o;
   struct image *img;
   int left=1,right=1,top=1,bottom=1;

   if (args)
      if (sp[-args].type!=T_INT)
         error("Illegal argument to image->autocrop()\n");
      else
         border=sp[-args].u.integer; 

   if (args>=5)
   {
      left=!(sp[1-args].type==T_INT && sp[1-args].u.integer==0);
      right=!(sp[2-args].type==T_INT && sp[2-args].u.integer==0);
      top=!(sp[3-args].type==T_INT && sp[3-args].u.integer==0);
      bottom=!(sp[4-args].type==T_INT && sp[4-args].u.integer==0);
      getrgb(THIS,5,args,"image->autocrop()"); 
   }
   else getrgb(THIS,1,args,"image->autocrop()"); 

   if (!THIS->img)
   {
      error("no image\n");
      return;
   }

   x1=y1=0;
   x2=THIS->xsize-1;
   y2=THIS->ysize-1;

   while (x2>x1 && y2>y1)
   {
      done=0;
      if (left &&
	  try_autocrop_vertical(x1,y1,y2,rgb_set,&rgb)) x1++,done=rgb_set=1;
      if (right &&
	  x2>x1 && 
	  try_autocrop_vertical(x2,y1,y2,rgb_set,&rgb)) x2--,done=rgb_set=1;
      if (top &&
	  try_autocrop_horisontal(y1,x1,x2,rgb_set,&rgb)) y1++,done=rgb_set=1;
      if (bottom &&
	  y2>y1 && 
	  try_autocrop_horisontal(y2,x1,x2,rgb_set,&rgb)) y2--,done=rgb_set=1;
      if (!done) break;
   }

   o=clone(image_program,0);
   img=(struct image*)(o->storage);

   img_crop(img,THIS,x1-border,y1-border,x2+border,y2+border);

   pop_n_elems(args);
   push_object(o);
}


void image_setcolor(INT32 args)
{
   if (args<3)
      error("illegal arguments to image->setcolor()\n");
   getrgb(THIS,0,args,"image->setcolor()");
   pop_n_elems(args);
   THISOBJ->refs++;
   push_object(THISOBJ);
}

void image_setpixel(INT32 args)
{
   INT32 x,y;
   if (args<2||
       sp[-args].type!=T_INT||
       sp[1-args].type!=T_INT)
      error("Illegal arguments to image->setpixel()\n");
   getrgb(THIS,2,args,"image->setpixel()");   
   if (!THIS->img) return;
   x=sp[-args].u.integer;
   y=sp[1-args].u.integer;
   if (!THIS->img) return;
   setpixel_test(x,y);
   pop_n_elems(args);
   THISOBJ->refs++;
   push_object(THISOBJ);
}

void image_getpixel(INT32 args)
{
   INT32 x,y;
   rgb_group rgb;

   if (args<2||
       sp[-args].type!=T_INT||
       sp[1-args].type!=T_INT)
      error("Illegal arguments to image->getpixel()\n");

   if (!THIS->img) error("No image.\n");

   x=sp[-args].u.integer;
   y=sp[1-args].u.integer;
   if (!THIS->img) return;
   if(x<0||y<0||x>=THIS->xsize||y>=THIS->ysize)
      rgb=THIS->rgb;
   else
      rgb=pixel(THIS,x,y);

   pop_n_elems(args);
   push_int(rgb.r);
   push_int(rgb.g);
   push_int(rgb.b);
   f_aggregate(3);
}

void image_line(INT32 args)
{
   if (args<4||
       sp[-args].type!=T_INT||
       sp[1-args].type!=T_INT||
       sp[2-args].type!=T_INT||
       sp[3-args].type!=T_INT)
      error("Illegal arguments to image->line()\n");
   getrgb(THIS,4,args,"image->line()");
   if (!THIS->img) return;

   img_line(sp[-args].u.integer,
	    sp[1-args].u.integer,
	    sp[2-args].u.integer,
	    sp[3-args].u.integer);
   pop_n_elems(args);
   THISOBJ->refs++;
   push_object(THISOBJ);
}

void image_box(INT32 args)
{
   if (args<4||
       sp[-args].type!=T_INT||
       sp[1-args].type!=T_INT||
       sp[2-args].type!=T_INT||
       sp[3-args].type!=T_INT)
      error("Illegal arguments to image->box()\n");
   getrgb(THIS,4,args,"image->box()");
   if (!THIS->img) return;

   img_box(sp[-args].u.integer,
	   sp[1-args].u.integer,
	   sp[2-args].u.integer,
	   sp[3-args].u.integer);
   pop_n_elems(args);
   THISOBJ->refs++;
   push_object(THISOBJ);
}

void image_circle(INT32 args)
{
   INT32 x,y,rx,ry;
   INT32 i;

   if (args<4||
       sp[-args].type!=T_INT||
       sp[1-args].type!=T_INT||
       sp[2-args].type!=T_INT||
       sp[3-args].type!=T_INT)
      error("illegal arguments to image->circle()\n");
   getrgb(THIS,4,args,"image->circle()");
   if (!THIS->img) return;

   x=sp[-args].u.integer;
   y=sp[1-args].u.integer;
   rx=sp[2-args].u.integer;
   ry=sp[3-args].u.integer;
   
   for (i=0; i<CIRCLE_STEPS; i++)
      img_line(x+circle_sin_mul(i,rx),
	       y+circle_cos_mul(i,ry),
	       x+circle_sin_mul(i+1,rx),
	       y+circle_cos_mul(i+1,ry));
   THISOBJ->refs++;
   push_object(THISOBJ);
}

static INLINE void get_rgba_group_from_array_index(rgba_group *rgba,struct array *v,INT32 index)
{
   struct svalue s,s2;
   array_index_no_free(&s,v,index);
   if (s.type!=T_ARRAY||
       s.u.array->size<3) 
      rgba->r=rgba->b=rgba->g=rgba->alpha=0; 
   else
   {
      array_index_no_free(&s2,s.u.array,0);
      if (s2.type!=T_INT) rgba->r=0; else rgba->r=s2.u.integer;
      array_index(&s2,s.u.array,1);
      if (s2.type!=T_INT) rgba->g=0; else rgba->g=s2.u.integer;
      array_index(&s2,s.u.array,2);
      if (s2.type!=T_INT) rgba->b=0; else rgba->b=s2.u.integer;
      if (s.u.array->size>=4)
      {
	 array_index(&s2,s.u.array,3);
	 if (s2.type!=T_INT) rgba->alpha=0; else rgba->alpha=s2.u.integer;
      }
      else rgba->alpha=0;
      free_svalue(&s2);
   }
   free_svalue(&s); 
   return; 
}

static INLINE void
   add_to_rgba_sum_with_factor(rgba_group *sum,
			       rgba_group rgba,
			       float factor)
{
   sum->r=testrange(sum->r+(INT32)(rgba.r*factor+0.5));
   sum->g=testrange(sum->g+(INT32)(rgba.g*factor+0.5));
   sum->b=testrange(sum->b+(INT32)(rgba.b*factor+0.5));
   sum->alpha=testrange(sum->alpha+(INT32)(rgba.alpha*factor+0.5));
}

static INLINE void
   add_to_rgb_sum_with_factor(rgb_group *sum,
			      rgba_group rgba,
			      float factor)
{
   sum->r=testrange(sum->r+(INT32)(rgba.r*factor+0.5));
   sum->g=testrange(sum->g+(INT32)(rgba.g*factor+0.5));
   sum->b=testrange(sum->b+(INT32)(rgba.b*factor+0.5));
}

void image_tuned_box(INT32 args)
{
   INT32 x1,y1,x2,y2,xw,yw,x,y;
   rgba_group topleft,topright,bottomleft,bottomright,sum,sumzero={0,0,0,0};
   rgb_group *img;
   struct image *this;

   if (args<5||
       sp[-args].type!=T_INT||
       sp[1-args].type!=T_INT||
       sp[2-args].type!=T_INT||
       sp[3-args].type!=T_INT||
       sp[4-args].type!=T_ARRAY||
       sp[4-args].u.array->size<4)
      error("Illegal number of arguments to image->tuned_box()\n");

   if (!THIS->img)
      error("no image\n");

   x1=sp[-args].u.integer;
   y1=sp[1-args].u.integer;
   x2=sp[2-args].u.integer;
   y2=sp[3-args].u.integer;

   get_rgba_group_from_array_index(&topleft,sp[4-args].u.array,0);
   get_rgba_group_from_array_index(&topright,sp[4-args].u.array,1);
   get_rgba_group_from_array_index(&bottomleft,sp[4-args].u.array,2);
   get_rgba_group_from_array_index(&bottomright,sp[4-args].u.array,3);

   if (x1>x2) x1^=x2,x2^=x1,x1^=x2,
              sum=topleft,topleft=topright,topright=sum,
              sum=bottomleft,bottomleft=bottomright,bottomright=sum;
   if (y1>y2) y1^=y2,y2^=y1,y1^=y2,
              sum=topleft,topleft=bottomleft,bottomleft=sum,
              sum=topright,topright=bottomright,bottomright=sum;
   if (x2<0||y2<0||x1>=THIS->xsize||y1>=THIS->ysize) return;

   xw=x2-x1;
   yw=y2-y1;

   this=THIS;
   THREADS_ALLOW();

   for (x=max(0,-x1); x<=xw && x+x1<THIS->xsize; x++)
   {
#define tune_factor(a,aw) (1.0-((float)(a)/(aw)))
      INT32 ymax;
      float tfx1=tune_factor(x,xw);
      float tfx2=tune_factor(xw-x,xw);

      ymax=min(yw,this->ysize-y1);
      img=this->img+x+x1+this->xsize*max(0,y1);
      if (topleft.alpha||topright.alpha||bottomleft.alpha||bottomright.alpha)
	 for (y=max(0,-y1); y<ymax; y++)
	 {
	    float tfy;
	    sum=sumzero;

	    add_to_rgba_sum_with_factor(&sum,topleft,(tfy=tune_factor(y,yw))*tfx1);
	    add_to_rgba_sum_with_factor(&sum,topright,tfy*tfx2);
	    add_to_rgba_sum_with_factor(&sum,bottomleft,(tfy=tune_factor(yw-y,yw))*tfx1);
	    add_to_rgba_sum_with_factor(&sum,bottomright,tfy*tfx2);

	    set_rgb_group_alpha(*img, sum,sum.alpha);
	    img+=this->xsize;
	 }
      else
	 for (y=max(0,-y1); y<ymax; y++)
	 {
	    float tfy;
	    rgb_group sum={0,0,0};

	    add_to_rgb_sum_with_factor(&sum,topleft,(tfy=tune_factor(y,yw))*tfx1);
	    add_to_rgb_sum_with_factor(&sum,topright,tfy*tfx2);
	    add_to_rgb_sum_with_factor(&sum,bottomleft,(tfy=tune_factor(yw-y,yw))*tfx1);
	    add_to_rgb_sum_with_factor(&sum,bottomright,tfy*tfx2);

	    *img=sum;
	    img+=this->xsize;
	 }
	 
   }
   THREADS_DISALLOW();

   pop_n_elems(args);
   THISOBJ->refs++;
   push_object(THISOBJ);
}

void image_xsize(INT32 args)
{
   pop_n_elems(args);
   if (THIS->img) push_int(THIS->xsize); else push_int(0);
}

void image_ysize(INT32 args)
{
   pop_n_elems(args);
   if (THIS->img) push_int(THIS->ysize); else push_int(0);
}

void image_gray(INT32 args)
{
   INT32 x,y,div;
   rgbl_group rgb;
   rgb_group *d,*s;
   struct object *o;
   struct image *img;

   if (args<3)
   {
      rgb.r=87;
      rgb.g=127;
      rgb.b=41;
   }
   else
      getrgbl(&rgb,0,args,"image->gray()");
   div=rgb.r+rgb.g+rgb.b;

   o=clone(image_program,0);
   img=(struct image*)o->storage;
   *img=*THIS;
   if (!(img->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize+1)))
   {
      free_object(o);
      error("Out of memory\n");
   }

   d=img->img;
   s=THIS->img;
   x=THIS->xsize*THIS->ysize;
   THREADS_ALLOW();
   while (x--)
   {
      d->r=d->g=d->b=
	 testrange( ((((long)s->r)*rgb.r+
		      ((long)s->g)*rgb.g+
		      ((long)s->b)*rgb.b)/div) );
      d++;
      s++;
   }
   THREADS_DISALLOW();
   pop_n_elems(args);
   push_object(o);
}

void image_color(INT32 args)
{
   INT32 x,y;
   rgbl_group rgb;
   rgb_group *s,*d;
   struct object *o;
   struct image *img;

   if (!THIS->img) error("no image\n");
   if (args<3)
   {
      if (args>0 && sp[-args].type==T_INT)
	 rgb.r=rgb.b=rgb.g=sp[-args].u.integer;
      else 
	 rgb.r=THIS->rgb.r,
	 rgb.g=THIS->rgb.g,
	 rgb.b=THIS->rgb.b;
   }
   else
      getrgbl(&rgb,0,args,"image->color()");

   o=clone(image_program,0);
   img=(struct image*)o->storage;
   *img=*THIS;
   if (!(img->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize+1)))
   {
      free_object(o);
      error("Out of memory\n");
   }

   d=img->img;
   s=THIS->img;

   x=THIS->xsize*THIS->ysize;

   THREADS_ALLOW();
   while (x--)
   {
      d->r=testrange( (((long)rgb.r*s->r)/255) );
      d->g=testrange( (((long)rgb.g*s->g)/255) );
      d->b=testrange( (((long)rgb.b*s->b)/255) );
      d++;
      s++;
   }
   THREADS_DISALLOW();

   pop_n_elems(args);
   push_object(o);
}

void image_invert(INT32 args)
{
   INT32 x,y;
   rgb_group *s,*d;
   struct object *o;
   struct image *img;

   if (!THIS->img) error("no image\n");

   o=clone(image_program,0);
   img=(struct image*)o->storage;
   *img=*THIS;
   if (!(img->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize+1)))
   {
      free_object(o);
      error("Out of memory\n");
   }

   d=img->img;
   s=THIS->img;

   x=THIS->xsize*THIS->ysize;
   THREADS_ALLOW();
   while (x--)
   {
      d->r=testrange( 255-s->r );
      d->g=testrange( 255-s->g );
      d->b=testrange( 255-s->b );
      d++;
      s++;
   }
   THREADS_DISALLOW();

   pop_n_elems(args);
   push_object(o);
}

void image_threshold(INT32 args)
{
   INT32 x,y;
   rgb_group *s,*d,rgb;
   struct object *o;
   struct image *img;

   if (!THIS->img) error("no image\n");

   getrgb(THIS,0,args,"image->threshold()");

   o=clone(image_program,0);
   img=(struct image*)o->storage;
   *img=*THIS;
   if (!(img->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize+1)))
   {
      free_object(o);
      error("Out of memory\n");
   }

   d=img->img;
   s=THIS->img;
   rgb=THIS->rgb;

   x=THIS->xsize*THIS->ysize;
   THREADS_ALLOW();
   while (x--)
   {
      if (s->r>=rgb.r &&
	  s->g>=rgb.g &&
	  s->b>=rgb.b)
	 d->r=d->g=d->b=255;
      else
	 d->r=d->g=d->b=0;

      d++;
      s++;
   }
   THREADS_DISALLOW();

   pop_n_elems(args);
   push_object(o);
}

void image_distancesq(INT32 args)
{
   INT32 i;
   rgb_group *s,*d,rgb;
   struct object *o;
   struct image *img;

   if (!THIS->img) error("no image\n");

   getrgb(THIS,0,args,"image->threshold()");

   o=clone(image_program,0);
   img=(struct image*)o->storage;
   *img=*THIS;
   if (!(img->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize+1)))
   {
      free_object(o);
      error("Out of memory\n");
   }

   d=img->img;
   s=THIS->img;
   rgb=THIS->rgb;

   THREADS_ALLOW();
   i=img->xsize*img->ysize;
   while (i--)
   {
#define DISTANCE(A,B) \
   (sq((long)(A).r-(B).r)+sq((long)(A).g-(B).g)+sq((long)(A).b-(B).b))
      d->r=d->g=d->b=testrange(DISTANCE(*s,rgb)>>8);
      d++; s++;
   }
   THREADS_DISALLOW();

   pop_n_elems(args);
   push_object(o);
}

/*#define DEBUG_ISF*/
#define ISF_LEFT 4
#define ISF_RIGHT 8

void isf_seek(int mode,int ydir,INT32 low_limit,INT32 x1,INT32 x2,INT32 y,
	      rgb_group *src,rgb_group *dest,INT32 xsize,INT32 ysize,
	      rgb_group rgb,int reclvl)
{
   INT32 x,xr;
   INT32 j;

#ifdef DEBUG_ISF
   fprintf(stderr,"isf_seek reclvl=%d mode=%d, ydir=%d, low_limit=%d, x1=%d, x2=%d, y=%d, src=%lx, dest=%lx, xsize=%d, ysize=%d, rgb=%d,%d,%d\n",reclvl,
	   mode,ydir,low_limit,x1,x2,y,src,dest,xsize,ysize,rgb.r,rgb.g,rgb.b);
#endif

#define MARK_DISTANCE(_dest,_value) \
      ((_dest).r=(_dest).g=(_dest).b=(max(1,255-(_value>>8))))
   if ( mode&ISF_LEFT ) /* scan left from x1 */
   {
      x=x1;
      while (x>0)
      {
	 x--;
#ifdef DEBUG_ISF
fprintf(stderr,"<-- %d (",DISTANCE(rgb,src[x+y*xsize]));
fprintf(stderr," %d,%d,%d)\n",src[x+y*xsize].r,src[x+y*xsize].g,src[x+y*xsize].b);
#endif
	 if ( (j=DISTANCE(rgb,src[x+y*xsize])) >low_limit)
	 {
	    x++;
	    break;
	 }
	 else
	 {
	    if (dest[x+y*xsize].r) { x++; break; } /* been there */
	    MARK_DISTANCE(dest[x+y*xsize],j);
	 }
      }
      if (x1>x)
      {
	 isf_seek(ISF_LEFT,-ydir,low_limit,
		  x,x1-1,y,src,dest,xsize,ysize,rgb,reclvl+1);
      }
      x1=x;
   }
   if ( mode&ISF_RIGHT ) /* scan right from x2 */
   {
      x=x2;
      while (x<xsize-1)
      {
	 x++;
#ifdef DEBUG_ISF
fprintf(stderr,"--> %d (",DISTANCE(rgb,src[x+y*xsize]));
fprintf(stderr," %d,%d,%d)\n",src[x+y*xsize].r,src[x+y*xsize].g,src[x+y*xsize].b);
#endif
	 if ( (j=DISTANCE(rgb,src[x+y*xsize])) >low_limit)
	 {
	    x--;
	    break;
	 }
	 else
	 {
	    if (dest[x+y*xsize].r) { x--; break; } /* done that */
	    MARK_DISTANCE(dest[x+y*xsize],j);
	 }
      }
      if (x2<x)
      {
	 isf_seek(ISF_RIGHT,-ydir,low_limit,
		  x2+1,x,y,src,dest,xsize,ysize,rgb,reclvl+1);
      }
      x2=x;
   }
   xr=x=x1;
   y+=ydir;
   if (y<0 || y>=ysize) return;
   while (x<=x2)
   {
#ifdef DEBUG_ISF
fprintf(stderr,"==> %d (",DISTANCE(rgb,src[x+y*xsize]));
fprintf(stderr," %d,%d,%d)\n",src[x+y*xsize].r,src[x+y*xsize].g,src[x+y*xsize].b);
#endif
      if ( dest[x+y*xsize].r || /* seen that */
	   (j=DISTANCE(rgb,src[x+y*xsize])) >low_limit) 
      {
	 if (xr<x)
	    isf_seek(ISF_LEFT*(xr==x1),ydir,low_limit,
		     xr,x-1,y,src,dest,xsize,ysize,rgb,reclvl+1);
	 while (++x<=x2)
	    if ( (j=DISTANCE(rgb,src[x+y*xsize])) <=low_limit) break;
	 xr=x;
/*	 x++; hokuspokus /law */
/*       nån dag ska jag försöka begripa varför... */
	 if (x>x2) return;
	 continue;
      }
      MARK_DISTANCE(dest[x+y*xsize],j);
      x++;
   }
   if (x>xr)
      isf_seek((ISF_LEFT*(xr==x1))|ISF_RIGHT,ydir,low_limit,
	       xr,x-1,y,src,dest,xsize,ysize,rgb,reclvl+1);
}

void image_select_from(INT32 args)
{
   struct object *o;
   struct image *img;
   INT32 low_limit=0;

   if (!THIS->img) error("no image\n");

   if (args<2 
       || sp[-args].type!=T_INT
       || sp[1-args].type!=T_INT)
      error("Illegal argument(s) to image->select_from()\n");

   if (args>=3)
      if (sp[2-args].type!=T_INT)
	 error("Illegal argument 3 (edge type) to image->select_from()\n");
      else
	 low_limit=max(0,sp[2-args].u.integer);
   else
      low_limit=30;
   low_limit=low_limit*low_limit;

   o=clone(image_program,0);
   img=(struct image*)o->storage;
   *img=*THIS;
   if (!(img->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize+1)))
   {
      free_object(o);
      error("Out of memory\n");
   }
   MEMSET(img->img,0,sizeof(rgb_group)*img->xsize*img->ysize);

   if (sp[-args].u.integer>=0 && sp[-args].u.integer<img->xsize 
       && sp[1-args].u.integer>=0 && sp[1-args].u.integer<img->ysize)
   {
      isf_seek(ISF_LEFT|ISF_RIGHT,1,low_limit,
	       sp[-args].u.integer,sp[-args].u.integer,
	       sp[1-args].u.integer,
	       THIS->img,img->img,img->xsize,img->ysize,
	       pixel(THIS,sp[-args].u.integer,sp[1-args].u.integer),0);
      isf_seek(ISF_LEFT|ISF_RIGHT,-1,low_limit,
	       sp[-args].u.integer,sp[-args].u.integer,
	       sp[1-args].u.integer,
	       THIS->img,img->img,img->xsize,img->ysize,
	       pixel(THIS,sp[-args].u.integer,sp[1-args].u.integer),0);
   }

   pop_n_elems(args);
   push_object(o);
}

void image_apply_matrix(INT32 args)
{
   int width,height,i,j;
   rgbd_group *matrix;
   rgb_group default_rgb;
   struct object *o;
   double div;

CHRONO("apply_matrix");

   if (args<1 ||
       sp[-args].type!=T_ARRAY)
      error("Illegal arguments to image->apply_matrix()\n");

   if (args>3)
      if (sp[1-args].type!=T_INT ||
	  sp[2-args].type!=T_INT ||
	  sp[3-args].type!=T_INT)
	 error("Illegal argument(s) (2,3,4) to image->apply_matrix()\n");
      else
      {
	 default_rgb.r=sp[1-args].u.integer;
	 default_rgb.g=sp[1-args].u.integer;
	 default_rgb.b=sp[1-args].u.integer;
      }
   else
   {
      default_rgb.r=0;
      default_rgb.g=0;
      default_rgb.b=0;
   }

   if (args>4 
       && sp[4-args].type==T_INT)
   {
      div=sp[4-args].u.integer;
      if (!div) div=1;
   }
   else if (args>4 
	    && sp[4-args].type==T_FLOAT)
   {
      div=sp[4-args].u.float_number;
      if (!div) div=1;
   }
   else div=1;
   
   height=sp[-args].u.array->size;
   width=-1;
   for (i=0; i<height; i++)
   {
      struct svalue s;
      array_index_no_free(&s,sp[-args].u.array,i);
      if (s.type!=T_ARRAY) 
	 error("Illegal contents of (root) array (image->apply_matrix)\n");
      if (width==-1)
	 width=s.u.array->size;
      else
	 if (width!=s.u.array->size)
	    error("Arrays has different size (image->apply_matrix)\n");
      free_svalue(&s);
   }
   if (width==-1) width=0;

   matrix=malloc(sizeof(rgbd_group)*width*height+1);
   if (!matrix) error("Out of memory");
   
   for (i=0; i<height; i++)
   {
      struct svalue s,s2;
      array_index_no_free(&s,sp[-args].u.array,i);
      for (j=0; j<width; j++)
      {
	 array_index_no_free(&s2,s.u.array,j);
	 if (s2.type==T_ARRAY && s2.u.array->size == 3)
	 {
	    struct svalue s3;
	    array_index_no_free(&s3,s2.u.array,0);
	    if (s3.type==T_INT) matrix[j+i*width].r=s3.u.integer; 
	    else matrix[j+i*width].r=0;
	    free_svalue(&s3);
	    array_index_no_free(&s3,s2.u.array,1);
	    if (s3.type==T_INT) matrix[j+i*width].g=s3.u.integer;
	    else matrix[j+i*width].g=0;
	    free_svalue(&s3);
	    array_index_no_free(&s3,s2.u.array,2);
	    if (s3.type==T_INT) matrix[j+i*width].b=s3.u.integer; 
	    else matrix[j+i*width].b=0;
	    free_svalue(&s3);
	 }
	 else if (s2.type==T_INT)
	    matrix[j+i*width].r=matrix[j+i*width].g=
	       matrix[j+i*width].b=s2.u.integer;
	 else
	    matrix[j+i*width].r=matrix[j+i*width].g=
	       matrix[j+i*width].b=0;
	 free_svalue(&s2);
      }
      free_svalue(&s2);
   }

   o=clone(image_program,0);

CHRONO("apply_matrix, begin");

   if (THIS->img)
      img_apply_matrix((struct image*)o->storage,THIS,
		       width,height,matrix,default_rgb,div);

CHRONO("apply_matrix, end");

   free(matrix);

   pop_n_elems(args);
   push_object(o);
}

void image_modify_by_intensity(INT32 args)
{
   INT32 x,y,i;
   rgbl_group rgb;
   rgb_group *list;
   rgb_group *s,*d;
   struct object *o;
   struct image *img;
   long div;

   if (!THIS->img) error("no image\n");
   if (args<5) 
      error("too few arguments to image->modify_by_intensity()\n");
   
   getrgbl(&rgb,0,args,"image->modify_by_intensity()");
   div=rgb.r+rgb.g+rgb.b;
   if (!div) div=1;

   s=malloc(sizeof(rgb_group)*(args-3)+1);
   if (!s) error("Out of memory\n");

   for (x=0; x<args-3; x++)
   {
      if (sp[3-args+x].type==T_INT)
	 s[x].r=s[x].g=s[x].b=testrange( sp[3-args+x].u.integer );
      else if (sp[3-args+x].type==T_ARRAY &&
	       sp[3-args+x].u.array->size >= 3)
      {
	 struct svalue sv;
	 array_index_no_free(&sv,sp[3-args+x].u.array,0);
	 if (sv.type==T_INT) s[x].r=testrange( sv.u.integer );
	 else s[x].r=0;
	 array_index(&sv,sp[3-args+x].u.array,1);
	 if (sv.type==T_INT) s[x].g=testrange( sv.u.integer );
	 else s[x].g=0;
	 array_index(&sv,sp[3-args+x].u.array,2);
	 if (sv.type==T_INT) s[x].b=testrange( sv.u.integer );
	 else s[x].b=0;
	 free_svalue(&sv);
      }
      else s[x].r=s[x].g=s[x].b=0;
   }

   list=malloc(sizeof(rgb_group)*256+1);
   if (!list) 
   {
      free(s);
      error("out of memory\n");
   }
   for (x=0; x<args-4; x++)
   {
      INT32 p1,p2,r;
      p1=(255L*x)/(args-4);
      p2=(255L*(x+1))/(args-4);
      r=p2-p1;
      for (y=0; y<r; y++)
      {
	 list[y+p1].r=(((long)s[x].r)*(r-y)+((long)s[x+1].r)*(y))/r;
	 list[y+p1].g=(((long)s[x].g)*(r-y)+((long)s[x+1].g)*(y))/r;
	 list[y+p1].b=(((long)s[x].b)*(r-y)+((long)s[x+1].b)*(y))/r;
      }
   }
   list[255]=s[x];
   free(s);

   o=clone(image_program,0);
   img=(struct image*)o->storage;
   *img=*THIS;
   if (!(img->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize+1)))
   {
      free_object(o);
      error("Out of memory\n");
   }

   d=img->img;
   s=THIS->img;


   x=THIS->xsize*THIS->ysize;
   THREADS_ALLOW();
   while (x--)
   {
      i= testrange( ((((long)s->r)*rgb.r+
		      ((long)s->g)*rgb.g+
		      ((long)s->b)*rgb.b)/div) );
      *d=list[i];
      d++;
      s++;
   }
   THREADS_DISALLOW();

   free(list);

   pop_n_elems(args);
   push_object(o);
}

static void image_map_closest(INT32 args)
{
   struct colortable *ct;
   long i;
   rgb_group *d,*s;
   struct object *o;

   if (!THIS->img) error("no image\n");
   if (args<1
       || sp[-args].type!=T_ARRAY)
      error("illegal argument to image->map_closest()\n");

   push_int(THIS->xsize);
   push_int(THIS->ysize);
   o=clone(image_program,2);
      
   ct=colortable_from_array(sp[-args].u.array,"image->map_closest()\n");
   pop_n_elems(args);

   i=THIS->xsize*THIS->ysize;
   s=THIS->img;
   d=((struct image*)(o->storage))->img;
   THREADS_ALLOW();
   while (i--)
   {
      *d=ct->clut[colortable_rgb_nearest(ct,*s)];
      d++; *s++;
   }
   THREADS_DISALLOW();

   colortable_free(ct);
   push_object(o);
}

static void image_map_fast(INT32 args)
{
   struct colortable *ct;
   long i;
   rgb_group *d,*s;
   struct object *o;

   if (!THIS->img) error("no image\n");
   if (args<1
       || sp[-args].type!=T_ARRAY)
      error("illegal argument to image->map_closest()\n");

   push_int(THIS->xsize);
   push_int(THIS->ysize);
   o=clone(image_program,2);
      
   ct=colortable_from_array(sp[-args].u.array,"image->map_closest()\n");
   pop_n_elems(args);

   i=THIS->xsize*THIS->ysize;
   s=THIS->img;
   d=((struct image*)(o->storage))->img;
   THREADS_ALLOW();
   while (i--)
   {
      *d=ct->clut[colortable_rgb(ct,*s)];
      d++; *s++;
   }
   THREADS_DISALLOW();

   colortable_free(ct);
   push_object(o);
}

static void image_map_fs(INT32 args)
{
   struct colortable *ct;
   INT32 i,j,xs;
   rgb_group *d,*s;
   struct object *o;
   int *res,w;
   rgbl_group *errb;
   
   if (!THIS->img) error("no image\n");
   if (args<1
       || sp[-args].type!=T_ARRAY)
      error("illegal argument to image->map_fs()\n");

   push_int(THIS->xsize);
   push_int(THIS->ysize);
   o=clone(image_program,2);
      
   res=(int*)xalloc(sizeof(int)*THIS->xsize);
   errb=(rgbl_group*)xalloc(sizeof(rgbl_group)*THIS->xsize);
      
   ct=colortable_from_array(sp[-args].u.array,"image->map_closest()\n");
   pop_n_elems(args);

   for (i=0; i<THIS->xsize; i++)
      errb[i].r=(rand()%(FS_SCALE*2+1))-FS_SCALE,
      errb[i].g=(rand()%(FS_SCALE*2+1))-FS_SCALE,
      errb[i].b=(rand()%(FS_SCALE*2+1))-FS_SCALE;

   i=THIS->ysize;
   s=THIS->img;
   d=((struct image*)(o->storage))->img;
   w=0;
   xs=THIS->xsize;
   THREADS_ALLOW();
   while (i--)
   {
      image_floyd_steinberg(s,xs,errb,w=!w,res,ct);
      for (j=0; j<THIS->xsize; j++)
	 *(d++)=ct->clut[res[j]];
      s+=xs;
   }
   THREADS_DISALLOW();

   free(errb);
   free(res);
   colortable_free(ct);
   push_object(o);
}

void image_select_colors(INT32 args)
{
   struct colortable *ct;
   int colors,i;

   if (args<1
      || sp[-args].type!=T_INT)
      error("Illegal argument to image->select_colors()\n");

   colors=sp[-args].u.integer;
   pop_n_elems(args);
   if (!THIS->img) { error("no image\n");  return; }

   ct=colortable_quant(THIS,colors);
   for (i=0; i<colors; i++)
   {
      push_int(ct->clut[i].r);
      push_int(ct->clut[i].g);
      push_int(ct->clut[i].b);
      f_aggregate(3);
   }
   f_aggregate(colors);
   colortable_free(ct);
}

/***************** global init etc *****************************/

#define RGB_TYPE "int|void,int|void,int|void,int|void"

void init_font_programs(void);
void exit_font(void);

void init_image_programs()
{
   int i;

   image_noise_init();

   start_new_program();
   add_storage(sizeof(struct image));

   add_function("create",image_create,
		"function(int,int,"RGB_TYPE":void)",0);
   add_function("clone",image_clone,
		"function(int,int,"RGB_TYPE":object)",0);
   add_function("new",image_clone, /* alias */
		"function(int,int,"RGB_TYPE":object)",0);
   add_function("clear",image_clear,
		"function("RGB_TYPE":object)",0);
   add_function("toppm",image_toppm,
		"function(:string)",0);
   add_function("frompnm",image_frompnm,
		"function(string:object|string)",0);
   add_function("fromppm",image_frompnm,
		"function(string:object|string)",0);
   add_function("fromgif",image_fromgif,
		"function(string:object)",0);
   add_function("togif",image_togif,
		"function(:string)",0);
   add_function("togif_fs",image_togif_fs,
		"function(:string)",0);
   add_function("gif_begin",image_gif_begin,
		"function(int:string)",0);
   add_function("gif_add",image_gif_add,
		"function(int|void,int|void:string)",0);
   add_function("gif_add_fs",image_gif_add_fs,
		"function(int|void,int|void:string)",0);
   add_function("gif_add_nomap",image_gif_add_nomap,
		"function(int|void,int|void:string)",0);
   add_function("gif_add_fs_nomap",image_gif_add_fs_nomap,
		"function(int|void,int|void:string)",0);
   add_function("gif_end",image_gif_end,
		"function(:string)",0);
   add_function("gif_netscape_loop",image_gif_netscape_loop,
		"function(:string)",0);

   add_function("copy",image_copy,
		"function(void|int,void|int,void|int,void|int,"RGB_TYPE":object)",0);
   add_function("autocrop",image_autocrop,
		"function(:object)",0);
   add_function("scale",image_scale,
		"function(int|float,int|float|void:object)",0);
   add_function("translate",image_translate,
		"function(int|float,int|float:object)",0);
   add_function("translate_expand",image_translate_expand,
		"function(int|float,int|float:object)",0);

   add_function("paste",image_paste,
		"function(object,int|void,int|void:object)",0);
   add_function("paste_alpha",image_paste_alpha,
		"function(object,int,int|void,int|void:object)",0);
   add_function("paste_mask",image_paste_mask,
		"function(object,object,int|void,int|void:object)",0);
   add_function("paste_alpha_color",image_paste_alpha_color,
		"function(object,void|int,void|int,void|int,int|void,int|void:object)",0);

   add_function("setcolor",image_setcolor,
		"function(int,int,int:object)",0);
   add_function("setpixel",image_setpixel,
		"function(int,int,"RGB_TYPE":object)",0);
   add_function("getpixel",image_getpixel,
		"function(int,int:array(int))",0);
   add_function("line",image_line,
		"function(int,int,int,int,"RGB_TYPE":object)",0);
   add_function("circle",image_circle,
		"function(int,int,int,int,"RGB_TYPE":object)",0);
   add_function("box",image_box,
		"function(int,int,int,int,"RGB_TYPE":object)",0);
   add_function("tuned_box",image_tuned_box,
		"function(int,int,int,int,array:object)",0);

   add_function("gray",image_gray,
		"function("RGB_TYPE":object)",0);
   add_function("color",image_color,
		"function("RGB_TYPE":object)",0);
   add_function("change_color",image_change_color,
		"function(int,int,int,"RGB_TYPE":object)",0);
   add_function("invert",image_invert,
		"function("RGB_TYPE":object)",0);
   add_function("threshold",image_threshold,
		"function("RGB_TYPE":object)",0);
   add_function("distancesq",image_distancesq,
		"function("RGB_TYPE":object)",0);
   add_function("select_from",image_select_from,
		"function(int,int:object)",0);

   add_function("apply_matrix",image_apply_matrix,
                "function(array(array(int|array(int))):object)",0);
   add_function("modify_by_intensity",image_modify_by_intensity,
                "function(int,int,int,int,int:object)",0);

   add_function("rotate_ccw",image_ccw,
		"function(:object)",0);
   add_function("rotate_cw",image_cw,
		"function(:object)",0);
   add_function("mirrorx",image_mirrorx,
		"function(:object)",0);
   add_function("mirrory",image_mirrory,
		"function(:object)",0);
   add_function("skewx",image_skewx,
		"function(int|float,"RGB_TYPE":object)",0);
   add_function("skewy",image_skewy,
		"function(int|float,"RGB_TYPE":object)",0);
   add_function("skewx_expand",image_skewx_expand,
		"function(int|float,"RGB_TYPE":object)",0);
   add_function("skewy_expand",image_skewy_expand,
		"function(int|float,"RGB_TYPE":object)",0);

   add_function("rotate",image_rotate,
		"function(int|float,"RGB_TYPE":object)",0);
   add_function("rotate_expand",image_rotate_expand,
		"function(int|float,"RGB_TYPE":object)",0);

   add_function("xsize",image_xsize,
		"function(:int)",0);
   add_function("ysize",image_ysize,
		"function(:int)",0);

   add_function("map_closest",image_map_closest,
                "function(:object)",0);
   add_function("map_fast",image_map_fast,
                "function(:object)",0);
   add_function("map_fs",image_map_fs,
                "function(:object)",0);
   add_function("select_colors",image_select_colors,
                "function(int:array(array(int)))",0);

   add_function("noise",image_noise,
                "function(array(float|int|array(int)),float|void,float|void,float|void,float|void:object)",0);
   add_function("turbulence",image_turbulence,
                "function(array(float|int|array(int)),int|void,float|void,float|void,float|void,float|void:object)",0);

   add_function("dct",image_dct,
		"function(:object)",0);

   add_function("`-",image_operator_minus,
		"function(object|array(int):object)",0);
   add_function("`+",image_operator_plus,
		"function(object|array(int):object)",0);
   add_function("`*",image_operator_multiply,
		"function(object|array(int):object)",0);
   add_function("`&",image_operator_minimum,
		"function(object|array(int):object)",0);
   add_function("`|",image_operator_maximum,
		"function(object|array(int):object)",0);
		
   set_init_callback(init_image_struct);
   set_exit_callback(exit_image_struct);
  
   image_program=end_c_program("/precompiled/image");
   
   image_program->refs++;
  
   for (i=0; i<CIRCLE_STEPS; i++) 
      circle_sin_table[i]=(INT32)4096*sin(((double)i)*2.0*3.141592653589793/(double)CIRCLE_STEPS);

   init_font_programs();
}

void init_image_efuns(void) {}

void exit_image(void) 
{
  if(image_program)
  {
    free_program(image_program);
    image_program=0;
  }
  exit_font();
}


