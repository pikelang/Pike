/* $Id: image.c,v 1.95 1998/04/03 00:18:29 mirar Exp $ */

/*
**! module Image
**! note
**!	$Id: image.c,v 1.95 1998/04/03 00:18:29 mirar Exp $
**! class image
**!
**!	The main object of the <ref>Image</ref> module, this object
**!	is used as drawing area, mask or result of operations.
**!
**!	init: <ref>clear</ref>,
**!	<ref>clone</ref>,
**!	<ref>create</ref>, 
**!	<ref>xsize</ref>,
**!	<ref>ysize</ref>
**!
**!	plain drawing: <ref>box</ref>,
**!	<ref>circle</ref>,
**!	<ref>getpixel</ref>, 
**!	<ref>line</ref>,
**!	<ref>setcolor</ref>,
**!	<ref>setpixel</ref>, 
**!	<ref>threshold</ref>,
**!	<ref>tuned_box</ref>,
**!	<ref>polyfill</ref>
**!
**!	operators: <ref>`&</ref>,
**!	<ref>`*</ref>,
**!	<ref>`+</ref>,
**!	<ref>`-</ref>,
**!	<ref>`|</ref>
**!
**!	pasting images, layers: <ref>add_layers</ref>, 
**!	<ref>paste</ref>,
**!	<ref>paste_alpha</ref>,
**!	<ref>paste_alpha_color</ref>,
**!	<ref>paste_mask</ref>
**!
**!	getting subimages, scaling, rotating: <ref>autocrop</ref>, 
**!	<ref>clone</ref>,
**!	<ref>copy</ref>, 
**!	<ref>dct</ref>,
**!	<ref>mirrorx</ref>, 
**!	<ref>rotate</ref>,
**!	<ref>rotate_expand</ref>, 
**!	<ref>rotate_ccw</ref>,
**!	<ref>rotate_cw</ref>,
**!	<ref>scale</ref>, 
**!	<ref>skewx</ref>,
**!	<ref>skewx_expand</ref>,
**!	<ref>skewy</ref>,
**!	<ref>skewy_expand</ref>
**!
**!	calculation by pixels: <ref>apply_matrix</ref>, 
**!	<ref>change_color</ref>,
**!	<ref>color</ref>,
**!	<ref>distancesq</ref>, 
**!	<ref>grey</ref>,
**!	<ref>invert</ref>, 
**!	<ref>modify_by_intensity</ref>,
**!	<ref>select_from</ref>, 
**!	<ref>rgb_to_hsv</ref>,
**!	<ref>hsv_to_rgb</ref>,
**!	<ref>Image.colortable</ref>
**!
**!	converting to other datatypes: 
**!	<ref>Image.GIF</ref>,
**!	<ref>Image.PNM</ref>
**!
**!	special pattern drawing:
**!	<ref>noise</ref>,
**!	<ref>turbulence</ref>
**!
**! see also: Image, Image.font
*/

#include "global.h"

#include <math.h>
#include <ctype.h>

#include "stralloc.h"
#include "global.h"
RCSID("$Id: image.c,v 1.95 1998/04/03 00:18:29 mirar Exp $");
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "threads.h"
#include "array.h"
#include "error.h"
#include "operators.h"


#include "image.h"
#include "colortable.h"
#include "builtin_functions.h"

struct program *image_program;
extern struct program *image_colortable_program;

#ifdef THIS
#undef THIS /* Needed for NT */
#endif

#define THIS ((struct image *)(fp->current_storage))
#define THISOBJ (fp->current_object)

#define testrange(x) MAXIMUM(MINIMUM(((int)x),255),0)

#define sq(x) ((x)*(x))

#define CIRCLE_STEPS 128
static INT32 circle_sin_table[CIRCLE_STEPS];
#define circle_sin(x) circle_sin_table[((x)+CIRCLE_STEPS)%CIRCLE_STEPS]
#define circle_cos(x) circle_sin((x)-CIRCLE_STEPS/4)

#define circle_sin_mul(x,y) ((circle_sin(x)*(y))/4096)
#define circle_cos_mul(x,y) ((circle_cos(x)*(y))/4096)



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


/***************** init & exit *********************************/

static void init_image_struct(struct object *obj)
{
  THIS->img=NULL;
  THIS->rgb.r=0;
  THIS->rgb.g=0;
  THIS->rgb.b=0;
  THIS->xsize=THIS->ysize=0;
  THIS->alpha=0;
/*  fprintf(stderr,"init %lx (%d)\n",obj,++obj_counter);*/
}

static void exit_image_struct(struct object *obj)
{
  if (THIS->img) { free(THIS->img); THIS->img=NULL; }
/*
  fprintf(stderr,"exit %lx (%d) %dx%d=%.1fKb\n",obj,--obj_counter,
	  THIS->xsize,THIS->ysize,
	  (THIS->xsize*THIS->ysize*sizeof(rgb_group)+sizeof(struct image))/1024.0);
	  */
}

/***************** internals ***********************************/

#define apply_alpha(x,y,alpha) \
   ((unsigned char)((y*(255L-(alpha))+x*(alpha))/255L))

#define set_rgb_group_alpha(dest,src,alpha) \
   ((dest).r=apply_alpha((dest).r,(src).r,alpha), \
    (dest).g=apply_alpha((dest).g,(src).g,alpha), \
    (dest).b=apply_alpha((dest).b,(src).b,alpha))

#define pixel(_img,x,y) ((_img)->img[((int)(x))+((int)(y))*(int)(_img)->xsize])

#define setpixel(x,y) \
   (THIS->alpha? \
    set_rgb_group_alpha(THIS->img[(x)+(y)*THIS->xsize],THIS->rgb,THIS->alpha): \
    ((pixel(THIS,x,y)=THIS->rgb),0))

#define color_equal(A,B) (((A).r == (B).r) && ((A).g == (B).g) && ((A).b == (B).b))

#define setpixel_test(x,y) \
   ((((int)x)<0||((int)y)<0||((int)x)>=(int)THIS->xsize||((int)y)>=(int)THIS->ysize)? \
    0:(setpixel((int)x,(int)y),0))

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
      pixelstep=(int)((x2-x1)*1024)/(int)(y2-y1);
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

  /* NOTE:
   *	This code MUST be MT-SAFE!
   */
  HIDE_GLOBAL_VARIABLES();

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
   REVEAL_GLOBAL_VARIABLES();
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
   int i,x,y,bx,by,ex,ey,yp;
   int widthheight;
   double sumr,sumg,sumb;
   double qr,qg,qb;
   register double r=0,g=0,b=0;

THREADS_ALLOW();

   widthheight=width*height;
   sumr=sumg=sumb=0;
   for (i=0; i<widthheight;)
     {
       sumr+=matrix[i].r;
       sumg+=matrix[i].g;
       sumb+=matrix[i++].b;
     }

   if (!sumr) sumr=1; sumr*=div; qr=1.0/sumr;
   if (!sumg) sumg=1; sumg*=div; qg=1.0/sumg;
   if (!sumb) sumb=1; sumb*=div; qb=1.0/sumb;

   bx=width/2;
   by=height/2;
   ex=width-bx;
   ey=height-by;
   
   d=malloc(sizeof(rgb_group)*img->xsize*img->ysize +1);
THREADS_DISALLOW();

   if(!d) error("Out of memory.\n");
   
THREADS_ALLOW();
CHRONO("apply_matrix, one");

   for (y=by; y<img->ysize-ey; y++)
   {
      dp=d+y*img->xsize+bx;
      for (x=bx; x<img->xsize-ex; x++)
      {
	 r=g=b=0;
	 mp=matrix;
	 ip=img->img+(x-bx)+(y-by)*img->xsize;
	 /* for (yp=y-by,j=0; j<height; j++,yp++) */
#ifdef MATRIX_DEBUG
j=-1;
#endif
	 for (yp=y-by; yp<height+y-by; yp++)
	 {
#ifdef MATRIX_DEBUG
j++;
#endif
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
	    ip+=img->xsize-width;
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

/*
**! method void create()
**! method void create(int xsize,int ysize)
**! method void create(int xsize,int ysize,int r,int g,int b)
**! method void create(int xsize,int ysize,int r,int g,int b,int alpha)
**! 	Initializes a new image object.
**! arg int xsize
**! arg int ysize
**! 	size of (new) image in pixels
**! arg int r
**! arg int g
**! arg int b
**!     background color (will also be current color),
**!     default color is black
**! arg int alpha
**! 	default alpha channel value
**! see also: copy, clone, Image.image
**! bugs
**!	SIGSEGVS can be caused if the size is too big, due
**!	to unchecked overflow - 
**!	(xsize*ysize)&MAXINT is small enough to allocate.
*/

void image_create(INT32 args)
{
   if (args<2) return;
   if (sp[-args].type!=T_INT||
       sp[1-args].type!=T_INT)
      error("Illegal arguments to Image.image->create()\n");

   getrgb(THIS,2,args,"Image.image->create()"); 

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

/*
**! method object clone()
**! method object clone(int xsize,int ysize)
**! method object clone(int xsize,int ysize,int r,int g,int b)
**! method object clone(int xsize,int ysize,int r,int g,int b,int alpha)
**! 	Copies to or initialize a new image object.
**! returns the new object
**! arg int xsize
**! arg int ysize
**! 	size of (new) image in pixels, called image
**!     is cropped to that size
**! arg int r
**! arg int g
**! arg int b
**!     current color of the new image, 
**!     default is black. 
**!     Will also be the background color if the cloned image
**!     is empty (no drawing area made).
**! arg int alpha
**! 	new default alpha channel value
**! see also: copy, create
*/

void image_clone(INT32 args)
{
   struct object *o;
   struct image *img;

   if (args)
      if (args<2||
	  sp[-args].type!=T_INT||
	  sp[1-args].type!=T_INT)
	 error("Illegal arguments to Image.image->clone()\n");

   o=clone_object(image_program,0);
   img=(struct image*)(o->storage);
   *img=*THIS;

   if (args)
   {
      if(sp[-args].u.integer < 0 ||
	 sp[1-args].u.integer < 0)
	 error("Illegal size to Image.image->clone()\n");
      img->xsize=sp[-args].u.integer;
      img->ysize=sp[1-args].u.integer;
   }

   getrgb(img,2,args,"Image.image->clone()"); 

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


/*
**! method void clear()
**! method void clear(int r,int g,int b)
**! method void clear(int r,int g,int b,int alpha)
**! 	gives a new, cleared image with the same size of drawing area
**! arg int r
**! arg int g
**! arg int b
**!     color of the new image
**! arg int alpha
**! 	new default alpha channel value
**! see also: copy, clone
*/

void image_clear(INT32 args)
{
   struct object *o;
   struct image *img;

   o=clone_object(image_program,0);
   img=(struct image*)(o->storage);
   *img=*THIS;

   getrgb(img,0,args,"Image.image->clear()"); 

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

/*
**! method object copy()
**! method object copy(int x1,int y1,int x2,int y2)
**! method object copy(int x1,int y1,int x2,int y2,int r,int g,int b)
**! method object copy(int x1,int y1,int x2,int y2,int r,int g,int b,int alpha)
**! 	Copies this part of the image. The requested area can
**!	be smaller, giving a cropped image, or bigger - 
**!	the new area will be filled with the given or current color.
**!
**! returns a new image object
**!
**! note
**! 	<ref>clone</ref>(void) and <ref>copy</ref>(void) does the same 
**!	operation
**!
**! arg int x1
**! arg int y1
**! arg int x2
**! arg int y2
**!	The requested new area. Default is the old image size.
**! arg int r
**! arg int g
**! arg int b
**!     color of the new image
**! arg int alpha
**! 	new default alpha channel value
**! see also: clone, autocrop
*/

void image_copy(INT32 args)
{
   struct object *o;
   struct image *img;

   if (!args)
   {
      o=clone_object(image_program,0);
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
      error("illegal arguments to Image.image->copy()\n");

   if (!THIS->img) error("no image\n");

   getrgb(THIS,4,args,"Image.image->copy()"); 

   o=clone_object(image_program,0);
   img=(struct image*)(o->storage);

   img_crop(img,THIS,
	    sp[-args].u.integer,sp[1-args].u.integer,
	    sp[2-args].u.integer,sp[3-args].u.integer);

   pop_n_elems(args);
   push_object(o);
}

/*
**! method object change_color(int tor,int tog,int tob)
**! method object change_color(int fromr,int fromg,int fromb, int tor,int tog,int tob)
**! 	Changes one color (exakt match) to another.
**!	If non-exakt-match is preferred, check <ref>distancesq</ref>
**!	and <ref>paste_alpha_color</ref>.
**! returns a new (the destination) image object
**!
**! arg int tor
**! arg int tog
**! arg int tob
**!     destination color and next current color
**! arg int fromr
**! arg int fromg
**! arg int fromb
**!     source color, default is current color
*/

static void image_change_color(INT32 args)

{
   /* ->change_color([int from-r,g,b,] int to-r,g,b); */
   rgb_group from,to,*s,*d;
   INT32 left;
   struct object *o;
   struct image *img;

   if (!THIS->img) error("no image\n");
   if (args<3) error("too few arguments to Image.image->change_color()\n");

   if (args<6)
   {
      to=THIS->rgb;   
      getrgb(THIS,0,args,"Image.image->change_color()");
      from=THIS->rgb;
   }
   else
   {
      getrgb(THIS,0,args,"Image.image->change_color()");
      from=THIS->rgb;
      getrgb(THIS,3,args,"Image.image->change_color()");
      to=THIS->rgb;
   }
   
   o=clone_object(image_program,0);
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
      if (color_equal(*s,from))
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

/*
**! method object autocrop()
**! method object autocrop(int border)
**! method object autocrop(int border,int r,int g,int b)
**! method object autocrop(int border,int left,int right,int top,int bottom)
**! method object autocrop(int border,int left,int right,int top,int bottom,int r,int g,int b)
**! 	Removes "unneccesary" borders around the image, adds one of
**!	its own if wanted to, in selected directions.
**!
**!	"Unneccesary" is all pixels that are equal -- ie if all the same pixels
**!	to the left are the same color, that column of pixels are removed.
**!
**! returns the new image object
**!
**! arg int border
**!     added border size in pixels
**! arg int r
**! arg int g
**! arg int b
**!     color of the new border
**! arg int left
**! arg int right
**! arg int top
**! arg int bottom
**!	which borders to scan and cut the image; 
**!	a typical example is removing the top and bottom unneccesary
**!	pixels:
**!	<pre>img=img->autocrop(0, 0,0,1,1);</pre>
**! see also: copy
*/


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
         error("Illegal argument to Image.image->autocrop()\n");
      else
         border=sp[-args].u.integer; 

   if (args>=5)
   {
      left=!(sp[1-args].type==T_INT && sp[1-args].u.integer==0);
      right=!(sp[2-args].type==T_INT && sp[2-args].u.integer==0);
      top=!(sp[3-args].type==T_INT && sp[3-args].u.integer==0);
      bottom=!(sp[4-args].type==T_INT && sp[4-args].u.integer==0);
      getrgb(THIS,5,args,"Image.image->autocrop()"); 
   }
   else getrgb(THIS,1,args,"Image.image->autocrop()"); 

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

   o=clone_object(image_program,0);
   img=(struct image*)(o->storage);

   img_crop(img,THIS,x1-border,y1-border,x2+border,y2+border);

   pop_n_elems(args);
   push_object(o);
}


/*
**! method object setcolor(int r,int g,int b)
**! method object setcolor(int r,int g,int b,int alpha)
**!    set the current color
**!
**! returns the object called
**!
**! arg int r
**! arg int g
**! arg int b
**!     new color
**! arg int alpha
**!     new alpha value
*/
void image_setcolor(INT32 args)
{
   if (args<3)
      error("illegal arguments to Image.image->setcolor()\n");
   getrgb(THIS,0,args,"Image.image->setcolor()");
   pop_n_elems(args);
   THISOBJ->refs++;
   push_object(THISOBJ);
}

/*
**! method object setpixel(int x,int y)
**! method object setpixel(int x,int y,int r,int g,int b)
**! method object setpixel(int x,int y,int r,int g,int b,int alpha)
**!    
**! returns the object called
**!
**! arg int x
**! arg int y
**!	position of the pixel
**! arg int r
**! arg int g
**! arg int b
**!     color
**! arg int alpha
**!     alpha value
*/
void image_setpixel(INT32 args)
{
   INT32 x,y;
   if (args<2||
       sp[-args].type!=T_INT||
       sp[1-args].type!=T_INT)
      error("Illegal arguments to Image.image->setpixel()\n");
   getrgb(THIS,2,args,"Image.image->setpixel()");   
   if (!THIS->img) return;
   x=sp[-args].u.integer;
   y=sp[1-args].u.integer;
   if (!THIS->img) return;
   setpixel_test(x,y);
   pop_n_elems(args);
   THISOBJ->refs++;
   push_object(THISOBJ);
}

/*
**! method array(int) getpixel(int x,int y)
**!    
**! returns color of the requested pixel -- ({int red,int green,int blue})
**!
**! arg int x
**! arg int y
**!	position of the pixel
*/
void image_getpixel(INT32 args)
{
   INT32 x,y;
   rgb_group rgb;

   if (args<2||
       sp[-args].type!=T_INT||
       sp[1-args].type!=T_INT)
      error("Illegal arguments to Image.image->getpixel()\n");

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

/*
**! method object line(int x1,int y1,int x2,int y2)
**! method object line(int x1,int y1,int x2,int y2,int r,int g,int b)
**! method object line(int x1,int y1,int x2,int y2,int r,int g,int b,int alpha)
**! 	Draws a line on the image. The line is <i>not</i> antialiased.
**! 
**! returns the object called
**!
**! arg int x1
**! arg int y1
**! arg int x2
**! arg int y2
**!	line endpoints
**! arg int r
**! arg int g
**! arg int b
**!     color
**! arg int alpha
**!     alpha value
*/
void image_line(INT32 args)
{
   if (args<4||
       sp[-args].type!=T_INT||
       sp[1-args].type!=T_INT||
       sp[2-args].type!=T_INT||
       sp[3-args].type!=T_INT)
      error("Illegal arguments to Image.image->line()\n");
   getrgb(THIS,4,args,"Image.image->line()");
   if (!THIS->img) return;

   img_line(sp[-args].u.integer,
	    sp[1-args].u.integer,
	    sp[2-args].u.integer,
	    sp[3-args].u.integer);
   pop_n_elems(args);
   THISOBJ->refs++;
   push_object(THISOBJ);
}

/*
**! method object box(int x1,int y1,int x2,int y2)
**! method object box(int x1,int y1,int x2,int y2,int r,int g,int b)
**! method object box(int x1,int y1,int x2,int y2,int r,int g,int b,int alpha)
**! 	Draws a filled rectangle on the image. 
**! 
**! returns the object called
**!
**! arg int x1
**! arg int y1
**! arg int x2
**! arg int y2
**!	box corners
**! arg int r
**! arg int g
**! arg int b
**!     color of the box
**! arg int alpha
**!     alpha value 
*/
void image_box(INT32 args)
{
   if (args<4||
       sp[-args].type!=T_INT||
       sp[1-args].type!=T_INT||
       sp[2-args].type!=T_INT||
       sp[3-args].type!=T_INT)
      error("Illegal arguments to Image.image->box()\n");
   getrgb(THIS,4,args,"Image.image->box()");
   if (!THIS->img) return;

   img_box(sp[-args].u.integer,
	   sp[1-args].u.integer,
	   sp[2-args].u.integer,
	   sp[3-args].u.integer);
   pop_n_elems(args);
   THISOBJ->refs++;
   push_object(THISOBJ);
}

/*
**! method object circle(int x,int y,int rx,int ry)
**! method object circle(int x,int y,int rx,int ry,int r,int g,int b)
**! method object circle(int x,int y,int rx,int ry,int r,int g,int b,int alpha)
**! 	Draws a line on the image. The line is <i>not</i> antialiased.
**! 
**! returns the object called
**!
**! arg int x
**! arg int y
**!     circle center
**! arg int rx
**! arg int ry
**!	circle radius in pixels
**! arg int r
**! arg int g
**! arg int b
**!     color
**! arg int alpha
**!     alpha value
*/
void image_circle(INT32 args)
{
   INT32 x,y,rx,ry;
   INT32 i;

   if (args<4||
       sp[-args].type!=T_INT||
       sp[1-args].type!=T_INT||
       sp[2-args].type!=T_INT||
       sp[3-args].type!=T_INT)
      error("illegal arguments to Image.image->circle()\n");
   getrgb(THIS,4,args,"Image.image->circle()");
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
   
   pop_n_elems(args);
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
   add_to_rgbda_sum_with_factor(rgbda_group *sum,
				rgba_group rgba,
				float factor)
{
  /* NOTE:
   *	This code MUST be MT-SAFE! (but also fast /per)
   */
/*   HIDE_GLOBAL_VARIABLES(); */
   sum->r=sum->r+rgba.r*factor;
   sum->g=sum->g+rgba.g*factor;
   sum->b=sum->b+rgba.b*factor;
   sum->alpha=sum->alpha+rgba.alpha*factor;
/*    REVEAL_GLOBAL_VARIABLES(); */
}

static INLINE void
   add_to_rgbd_sum_with_factor(rgbd_group *sum,
			       rgba_group rgba,
			       float factor)
{
  /* NOTE:
   *	This code MUST be MT-SAFE! (but also fast /per)
   */
/*   HIDE_GLOBAL_VARIABLES(); */
   sum->r=sum->r+rgba.r*factor;
   sum->g=sum->g+rgba.g*factor;
   sum->b=sum->b+rgba.b*factor;
/*    REVEAL_GLOBAL_VARIABLES(); */
}

/*
**! method object tuned_box(int x1,int y1,int x2,int y2,array(array(int)) corner_color)
**! 	Draws a filled rectangle with colors (and alpha values) tuned
**!	between the corners.
**!
**!	Tuning function is (1.0-x/xw)*(1.0-y/yw) where x and y is
**!	the distance to the corner and xw and yw are the sides of the
**!	rectangle.
**!
**!	<table><tr valign=center>
**!	<td><illustration> return lena(); </illustration></td>
**!	<td><illustration> return lena()->copy()->tuned_box(0,0,lena()->xsize(),lena()->ysize(),({({255,0,0}),({0,255,0}),({0,0,255}),({255,255,0})})); </illustration></td>
**!	<td><illustration> return lena()->copy()->tuned_box(0,0,lena()->xsize(),lena()->ysize(),({({255,0,0,255}),({0,255,0,128}),({0,0,255,128}),({255,255,0})})); </illustration></td>
**!	</tr><tr valign=center>
**!	<td>original</td>
**!	<td>solid tuning<br>(blue,red,green,yellow)</td>
**!	<td>tuning transparency<br>(as left + 255,128,128,0)</td>
**!	</tr></table>
**!
**! returns the object called
**!
**! arg int x1
**! arg int y1
**! arg int x2
**! arg int y2
**!	rectangle corners
**! arg array(array(int)) corner_color
**!     colors of the corners:
**!	<pre>
**!	({x1y1,x2y1,x1y2,x2y2})
**!	</pre>
**!	each of these is an array of integeres:
**!	<pre>
**!	({r,g,b}) or ({r,g,b,alpha})
**!	</pre>
**!	Default alpha channel value is 0 (opaque).
*/
INLINE static void 
image_tuned_box_leftright(const rgba_group left, const rgba_group right, 
			  rgb_group *dest, 
			  const int length, const int maxlength,  
			  const int xsize,  const int height)
{
  int x, y=height;
  rgb_group *from = dest;
  if(!xsize || !height) return;
  for(x=0; x<maxlength; x++)
  {
    (dest+x)->r = (((long)left.r)*(length-x)+((long)right.r)*(x))/length;
    (dest+x)->g = (((long)left.g)*(length-x)+((long)right.g)*(x))/length;
    (dest+x)->b = (((long)left.b)*(length-x)+((long)right.b)*(x))/length;
  }
  while(--y)  MEMCPY((dest+=xsize), from, maxlength*sizeof(rgb_group)); 
}



INLINE static void 
image_tuned_box_topbottom(const rgba_group left, const rgba_group right,
			  rgb_group *dest, 
			  const int length, const int xsize, 
			  const int height,
			  const int maxheight)
{
  int x,y;
  rgb_group color, *from, old;
  if(!xsize || !maxheight) return;
  if(length > 128)
  {
    for(y=0; y<maxheight; y++)
    {
      color.r = (((long)left.r)*(height-y)+((long)right.r)*(y))/height;
      color.g = (((long)left.g)*(height-y)+((long)right.g)*(y))/height;
      color.b = (((long)left.b)*(height-y)+((long)right.b)*(y))/height;
      if(y && color_equal(old, color))
      {
	MEMCPY(dest,dest-xsize,length*sizeof(rgb_group));
	dest+=xsize;
      } else {
	from = dest;
	for(x=0; x<64; x++) *(dest++) = color;
	for(;x<length-64;x+=64,dest+=64) 
	  MEMCPY(dest, from, 64*sizeof(rgb_group));
	for(;x<length; x++) *(dest++) = color;
	dest += xsize-length;
	old = color;
      }
    }
  } else {
    for(y=0; y<maxheight; y++)
    {
      color.r = (((long)left.r)*(height-y)+((long)right.r)*(y))/height;
      color.g = (((long)left.g)*(height-y)+((long)right.g)*(y))/height;
      color.b = (((long)left.b)*(height-y)+((long)right.b)*(y))/height;
      if(y && color_equal(old, color))
      {
	MEMCPY(dest,dest-xsize,length*sizeof(rgb_group));
	dest+=xsize;
      } else {
	for(x=0; x<length; x++) *(dest++) = color;
	dest += xsize-length;
	old = color;
      }
    }
  }
}

void image_tuned_box(INT32 args)
{
  INT32 x1,y1,x2,y2,xw,yw,x,y;
  rgba_group topleft,topright,bottomleft,bottomright,sum;
  rgb_group *img;
  INT32 ymax;
  struct image *this;
  float dxw, dyw;

  if (args<5||
      sp[-args].type!=T_INT||
      sp[1-args].type!=T_INT||
      sp[2-args].type!=T_INT||
      sp[3-args].type!=T_INT||
      sp[4-args].type!=T_ARRAY||
      sp[4-args].u.array->size<4)
    error("Illegal number of arguments to Image.image->tuned_box()\n");

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

  pop_n_elems(args);
  THISOBJ->refs++;
  push_object(THISOBJ);

  if (x2<0||y2<0||x1>=THIS->xsize||y1>=THIS->ysize) return;
  xw=x2-x1;
  yw=y2-y1;
  if(xw == 0 || yw == 0) return;
  this=THIS;
  THREADS_ALLOW();

  if (! (topleft.alpha||topright.alpha||bottomleft.alpha||bottomright.alpha))
    {
      if(color_equal(topleft,bottomleft) && 
	 color_equal(topright, bottomright))
	{
	  image_tuned_box_leftright(topleft, bottomright, 
				    this->img+x1+this->xsize*y1, 
				    xw+1, MINIMUM(xw+1, this->xsize-x1),  
				    this->xsize, MINIMUM(yw+1, this->ysize-y1));

	} 
      else if(color_equal(topleft,topright) && color_equal(bottomleft,bottomright))
	{
	  image_tuned_box_topbottom(topleft, bottomleft, 
				    this->img+x1+this->xsize*y1, 
				    MINIMUM(xw+1, this->xsize-x1), this->xsize, 
				    yw+1, MINIMUM(yw+1, this->ysize-y1));
	} else 
	  goto ugly;
    } else {  
    ugly:
      dxw = 1.0/(float)xw;
      dyw = 1.0/(float)yw;
      ymax=MINIMUM(yw,this->ysize-y1);
      for (x=MAXIMUM(0,-x1); x<=xw && x+x1<this->xsize; x++)
	{
#define tune_factor(a,aw) (1.0-((float)(a)*(aw)))
	  float tfx1=tune_factor(x,dxw);
	  float tfx2=tune_factor(xw-x,dxw);

	  img=this->img+x+x1+this->xsize*MAXIMUM(0,y1);
	  if (topleft.alpha||topright.alpha||bottomleft.alpha||bottomright.alpha)
	    for (y=MAXIMUM(0,-y1); y<ymax; y++)
	      {
		float tfy;
		rgbda_group sum={0.0,0.0,0.0,0.0};
		rgbd_group rgb;

		add_to_rgbda_sum_with_factor(&sum,topleft,(tfy=tune_factor(y,dyw))*tfx1);
		add_to_rgbda_sum_with_factor(&sum,topright,tfy*tfx2);
		add_to_rgbda_sum_with_factor(&sum,bottomleft,(tfy=tune_factor(yw-y,dyw))*tfx1);
		add_to_rgbda_sum_with_factor(&sum,bottomright,tfy*tfx2);

		sum.alpha*=(1.0/255.0);

		rgb.r=sum.r*sum.alpha+img->r*(1.0-sum.alpha);
		rgb.g=sum.g*sum.alpha+img->g*(1.0-sum.alpha);
		rgb.b=sum.b*sum.alpha+img->b*(1.0-sum.alpha);

		img->r=testrange(rgb.r+0.5);
		img->g=testrange(rgb.g+0.5);
		img->b=testrange(rgb.b+0.5);

		img+=this->xsize;
	      }
	  else
	    for (y=MAXIMUM(0,-y1); y<ymax; y++)
	      {
		float tfy;
		rgbd_group sum={0,0,0};

		add_to_rgbd_sum_with_factor(&sum,topleft,(tfy=tune_factor(y,dyw))*tfx1);
		add_to_rgbd_sum_with_factor(&sum,topright,tfy*tfx2);
		add_to_rgbd_sum_with_factor(&sum,bottomleft,(tfy=tune_factor(yw-y,dyw))*tfx1);
		add_to_rgbd_sum_with_factor(&sum,bottomright,tfy*tfx2);

		img->r=testrange(sum.r+0.5);
		img->g=testrange(sum.g+0.5);
		img->b=testrange(sum.b+0.5);
		img+=this->xsize;
	      }
	 
	}
    }
  THREADS_DISALLOW();
}

/*
**! method int gradients(array(int) point, ...)
**! method int gradients(array(int) point, ..., float grad)
**! returns the new image
*/

static void image_gradients(INT32 args)
{
   struct gr_point
   {
      INT32 x,y,yd,xd;
      double r,g,b;
      struct gr_point *next;
   } *first=NULL,*c;
   INT32 n;
   INT32 x,y,xz;
   struct object *o;
   struct image *img;
   rgb_group *d;
   double grad=0.0;

   push_int(THIS->xsize);
   push_int(THIS->ysize);
   o=clone_object(image_program,2);
   img=(struct image*)get_storage(o,image_program);
   d=img->img;

   if (args && sp[-1].type==T_FLOAT)
   {
      args--;
      grad=sp[-1].u.float_number;
      pop_n_elems(1);
   }

   n=args;

   while (args--)
   {
      struct array *a;
      if (sp[-1].type!=T_ARRAY ||
	  (a=sp[-1].u.array)->size!=5 ||
	  a->item[0].type!=T_INT ||
	  a->item[1].type!=T_INT ||
	  a->item[2].type!=T_INT ||
	  a->item[3].type!=T_INT ||
	  a->item[4].type!=T_INT)
      {
	 while (first) { c=first; first=c->next; free(c); }
	 error("Image.image->gradients: Illegal argument %d\n",n);
      }
      c=malloc(sizeof(struct gr_point));
      if (!c)
      {
	 while (first) { c=first; first=c->next; free(c); }
	 error("Image.image->gradients: out of memory\n");
      }
      c->next=first;
      c->x=a->item[0].u.integer;
      c->y=a->item[1].u.integer;
      c->r=(double)a->item[2].u.integer;
      c->g=(double)a->item[3].u.integer;
      c->b=(double)a->item[4].u.integer;
      first=c;
      n--;
      pop_n_elems(1);
   }

   if (!first) 
      error("Image.image->gradients: need at least one argument\n");

   THREADS_ALLOW();

   xz=img->xsize;
   for (y=0; y<img->ysize; y++)
   {
      c=first;
      while (c)
      {
	 c->yd=y-c->y;
	 c->xd=-1-c->x;
	 c=c->next;
      }
      for (x=0; x<xz; x++)
      {
	 double r,g,b;
	 double z,di;

	 r=g=b=z=0.0;

	 c=first;

	 if (grad!=0.0)
	    while (c)
	    {
	       c->xd++;
	       di=pow((c->xd*c->xd)+(c->yd*c->yd),0.5*grad);
	       if (!di) di=1e20; else di=1.0/di;
	       r+=c->r*di;
	       g+=c->g*di;
	       b+=c->b*di;
	       z+=di;

	       c=c->next;
	    }
	 else
	    while (c)
	    {
	       c->xd++;
	       di=pow((c->xd*c->xd)+(c->yd*c->yd),0.5);
	       if (!di) di=1e20; else di=1.0/di;
	       r+=c->r*di;
	       g+=c->g*di;
	       b+=c->b*di;
	       z+=di;

	       c=c->next;
	    }

	 z=1.0/z;

	 d->r=(COLORTYPE)(r*z);
	 d->g=(COLORTYPE)(g*z);
	 d->b=(COLORTYPE)(b*z);
	 d++;
      }
   }

   while (first) { c=first; first=c->next; free(c); }

   THREADS_DISALLOW();

   push_object(o);
}

/*
**! method int xsize()
**! returns the width of the image
*/

void image_xsize(INT32 args)
{
   pop_n_elems(args);
   if (THIS->img) push_int(THIS->xsize); else push_int(0);
}

/*
**! method int ysize()
**! returns the height of the image
*/

void image_ysize(INT32 args)
{
   pop_n_elems(args);
   if (THIS->img) push_int(THIS->ysize); else push_int(0);
}

/*
**! method object grey()
**! method object grey(int r,int g,int b)
**!    Makes a grey-scale image (with weighted values).
**!
**! returns the new image object
**!
**!	<table><tr valign=center>
**!	<td><illustration> return lena(); </illustration></td>
**!	<td><illustration> return lena()->grey(); </illustration></td>
**!	<td><illustration> return lena()->grey(0,0,255); </illustration></td>
**!	</tr><tr valign=center>
**!	<td>original</td>
**!	<td>->grey();</td>
**!	<td>->grey(0,0,255);</td>
**!	</tr></table>
**!
**! arg int r
**! arg int g
**! arg int b
**!     weight of color, default is r=87,g=127,b=41,
**!	which should be pretty accurate of what the eyes see...
**!
**! see also: color, `*, modify_by_intensity
*/

void image_grey(INT32 args)
{
   INT32 x,div;
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
      getrgbl(&rgb,0,args,"Image.image->grey()");
   div=rgb.r+rgb.g+rgb.b;

   o=clone_object(image_program,0);
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

/*
**! method object color()
**! method object color(int value)
**! method object color(int r,int g,int b)
**!    Colorize an image. 
**!
**!    The red, green and blue values of the pixels are multiplied
**!    with the given value(s). This works best on a grey image...
**!
**!    The result is divided by 255, giving correct pixel values.
**!
**!    If no arguments are given, the current color is used as factors.
**!
**!	<table><tr valign=center>
**!	<td><illustration> return lena(); </illustration></td>
**!	<td><illustration> return lena()->color(128,128,255); </illustration></td>
**!	</tr><tr valign=center>
**!	<td>original</td>
**!	<td>->color(128,128,255);</td>
**!	</tr></table>
**!
**! returns the new image object
**!
**! arg int r
**! arg int g
**! arg int b
**!	red, green, blue factors
**! arg int value
**!	factor
**!
**! see also: grey, `*, modify_by_intensity
*/

void image_color(INT32 args)
{
   INT32 x;
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
      getrgbl(&rgb,0,args,"Image.image->color()");

   o=clone_object(image_program,0);
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
      d->r=( (((long)rgb.r*s->r)/255) );
      d->g=( (((long)rgb.g*s->g)/255) );
      d->b=( (((long)rgb.b*s->b)/255) );
      d++;
      s++;
   }
   THREADS_DISALLOW();

   pop_n_elems(args);
   push_object(o);
}

/*
**! method object invert()
**!    Invert an image. Each pixel value gets to be 255-x, where x 
**!    is the old value.
**!
**!	<table><tr valign=center>
**!	<td><illustration> return lena(); </illustration></td>
**!	<td><illustration> return lena()->invert(); </illustration></td>
**!	<td><illustration> return lena()->rgb_to_hsv()->invert()->hsv_to_rgb(); </illustration></td>
**!	</tr><tr valign=center>
**!	<td>original</td>
**!	<td>->invert();</td>
**!	<td>->rgb_to_hsv()->invert()->hsv_to_rgb();</td>
**!	</tr></table>
**!
**! returns the new image object
*/

void image_invert(INT32 args)
{
   INT32 x;
   rgb_group *s,*d;
   struct object *o;
   struct image *img;

   if (!THIS->img) error("no image\n");

   o=clone_object(image_program,0);
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
      d->r=( 255-s->r );
      d->g=( 255-s->g );
      d->b=( 255-s->b );
      d++;
      s++;
   }
   THREADS_DISALLOW();

   pop_n_elems(args);
   push_object(o);
}

/*
**! method object threshold()
**! method object threshold(int r,int g,int b)
**! 	Makes a black-white image. 
**!
**! 	If all red, green, blue parts of a pixel
**!    	is larger or equal then the given value, the pixel will become
**!	white, else black.
**!
**!	This method works fine with the grey method.
**!
**!	If no arguments are given, the current color is used 
**!	for threshold values.
**!
**!	<table><tr valign=center>
**!	<td><illustration> return lena(); </illustration></td>
**!	<td><illustration> return lena()->threshold(90,100,110); </illustration></td>
**!	</tr><tr valign=center>
**!	<td>original</td>
**!	<td>->threshold(90,100,110);</td>
**!	</tr></table>
**!
**! returns the new image object
**!
**! arg int r
**! arg int g
**! arg int b
**! 	red, green, blue threshold values
**!
**! see also: grey
*/


void image_threshold(INT32 args)
{
   INT32 x;
   rgb_group *s,*d,rgb;
   struct object *o;
   struct image *img;

   if (!THIS->img) error("no image\n");

   getrgb(THIS,0,args,"Image.image->threshold()");

   o=clone_object(image_program,0);
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


/*
**! method object rgb_to_hsv()
**! method object hsv_to_rgb()
**!    Converts RGB data to HSV data, or the other way around.
**!    When converting to HSV, the resulting data is stored like this:
**!     pixel.r = h; pixel.g = s; pixel.b = v;
**!
**!    When converting to RGB, the input data is asumed to be placed in
**!    the pixels as above.
**!
**!	<table><tr valign=center>
**!	<td><illustration> return lena(); </illustration></td>
**!	<td><illustration> return lena()->hsv_to_rgb(); </illustration></td>
**!	<td><illustration> return lena()->rgb_to_hsv(); </illustration></td>
**!	</tr><tr valign=center>
**!	<td>original</td>
**!	<td>->hsv_to_rgb();</td>
**!	<td>->rgb_to_hsv();</td>
**!	</tr><tr valign=center>
**!	<td><illustration>
**!     return image(67,67)->tuned_box(0,0, 67,67,
**!                      ({ ({ 255,255,128 }), ({ 0,255,128 }),
**!                         ({ 255,255,255 }), ({ 0,255,255 })}));
**!	</illustration></td>
**!	<td><illustration>
**!     return image(67,67)->tuned_box(0,0, 67,67,
**!                      ({ ({ 255,255,128 }), ({ 0,255,128 }),
**!                         ({ 255,255,255 }), ({ 0,255,255 })}))
**!          ->hsv_to_rgb();
**!	</illustration></td>
**!	<td><illustration>
**!     return image(67,67)->tuned_box(0,0, 67,67,
**!                      ({ ({ 255,255,128 }), ({ 0,255,128 }),
**!                         ({ 255,255,255 }), ({ 0,255,255 })}))
**!          ->rgb_to_hsv();
**!	</illustration></td>
**!	</tr><tr valign=center>
**!	<td>tuned box (below)</td>
**!	<td>the rainbow (below)</td>
**!	<td>same, but rgb_to_hsv()</td>
**!	</tr></table>
**!
**!
**!    HSV to RGB calculation:
**!    <pre>
**!    in = input pixel
**!    out = destination pixel
**!    h=-pos*c_angle*3.1415/(float)NUM_SQUARES;
**!    out.r=(in.b+in.g*cos(in.r));
**!    out.g=(in.b+in.g*cos(in.r + pi*2/3));
**!    out.b=(in.b+in.g*cos(in.r + pi*4/3));
**!    </pre>
**!
**!    RGB to HSV calculation: Hmm.
**!    <pre>
**!    </pre>
**!
**!     Example: Nice rainbow.
**!     <pre>
**!     object i = Image.image(200,200);
**!     i = i->tuned_box(0,0, 200,200,
**!                      ({ ({ 255,255,128 }), ({ 0,255,128 }),
**!                         ({ 255,255,255 }), ({ 0,255,255 })}))
**!          ->hsv_to_rgb();
**!	</pre>
**! returns the new image object
*/

void image_hsv_to_rgb(INT32 args)
{
   INT32 i;
   rgb_group *s,*d;
   struct object *o;
   struct image *img;
   char *err = NULL;
   if (!THIS->img) error("no image\n");

   o=clone_object(image_program,0);
   img=(struct image*)o->storage;
   *img=*THIS;

   if (!(img->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize+1)))
   {
      free_object(o);
      error("Out of memory\n");
   }

   d=img->img;
   s=THIS->img;

   THREADS_ALLOW();
   i=img->xsize*img->ysize;
   while (i--)
   {
     double h,sat,v;
     float r,g,b;
     h = (s->r/255.0)*(360.0/60.0);
     sat = s->g/255.0;
     v = s->b/255.0;
     
     if(sat==0.0)
     {
       r = g = b = v;
     } else {
#define i floor(h)
#define f (h-i)
#define p (v * (1 - sat))
#define q (v * (1 - (sat * f)))
#define t (v * (1 - (sat * (1 -f))))
	switch((int)i)
	{
	   case 6: /* 360 degrees. Same as 0.. */
	   case 0: r = v; g = t; b = p;	 break;
	   case 1: r = q; g = v; b = p;	 break;
	   case 2: r = p; g = v; b = t;	 break;
	   case 3: r = p; g = q; b = v;	 break;
	   case 4: r = t; g = p; b = v;	 break;
	   case 5: r = v; g = p; b = q;	 break;
	   default:
	      err = "Nope. Not possible";
	      goto exit_loop;
	}
     }
#undef i
#undef f
#undef p
#undef q
#undef t
#define FIX(X) ((X)<0.0?0:(X)>=1.0?255:(int)((X)*255.0))
     d->r = FIX(r);
     d->g = FIX(g);
     d->b = FIX(b);
     s++; d++;
   }
exit_loop:
   ;	/* Needed to keep some compilers happy. */
   THREADS_DISALLOW();

   if (err) {
     error(err);
   }

   pop_n_elems(args);
   push_object(o);
}

#ifndef MAX3
#define MAX3(X,Y,Z) MAXIMUM(MAXIMUM(X,Y),Z)
#endif

#ifndef MIN3
#define MIN3(X,Y,Z) MINIMUM(MINIMUM(X,Y),Z)
#endif

void image_rgb_to_hsv(INT32 args)
{
   INT32 i;
   rgb_group *s,*d;
   struct object *o;
   struct image *img;
   if (!THIS->img) error("no image\n");

   o=clone_object(image_program,0);
   img=(struct image*)o->storage;
   *img=*THIS;

   if (!(img->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize+1)))
   {
      free_object(o);
      error("Out of memory\n");
   }

   d=img->img;
   s=THIS->img;

   THREADS_ALLOW();
   i=img->xsize*img->ysize;
   while (i--)
   {
      register int r,g,b;
      register int v, delta;
      register int h;

      r = s->r; g = s->g; b = s->b;
      v = MAX3(r,g,b);
      delta = v - MIN3(r,g,b);

      if(r==v)      h = (int)(((g-b)/(float)delta)*(255.0/6.0));
      else if(g==v) h = (int)((2.0+(b-r)/(float)delta)*(255.0/6.0));
      else h = (int)((4.0+(r-g)/(float)delta)*(255.0/6.0));
      if(h<0) h+=255;

      /*     printf("hsv={ %d,%d,%d }\n", h, (int)((delta/(float)v)*255), v);*/
      d->r = (int)h;
      d->g=(int)((delta/(float)v)*255.0);
      d->b=v;
      s++; d++;
   }
   THREADS_DISALLOW();

   pop_n_elems(args);
   push_object(o);
}



/*
**! method object distancesq()
**! method object distancesq(int r,int g,int b)
**!    Makes an grey-scale image, for alpha-channel use.
**!    
**!    The given value (or current color) are used for coordinates
**!    in the color cube. Each resulting pixel is the 
**!    distance from this point to the source pixel color,
**!    in the color cube, squared, rightshifted 8 steps:
**!
**!    <pre>
**!    p = pixel color
**!    o = given color
**!    d = destination pixel
**!    d.red=d.blue=d.green=
**!	   ((o.red-p.red)²+(o.green-p.green)²+(o.blue-p.blue)²)>>8
**!    </pre>
**!
**!	<table><tr valign=center>
**!	<td><illustration> return lena(); </illustration></td>
**!	<td><illustration> return lena()->distancesq(255,0,128); </illustration></td>
**!	</tr><tr valign=center>
**!	<td>original</td>
**!	<td>->distancesq(255,0,128);</td>
**!	</tr></table>
**!
**!
**! returns the new image object
**!
**! arg int r
**! arg int g
**! arg int b
**!	red, green, blue coordinates
**!
**! see also: select_from
*/

void image_distancesq(INT32 args)
{
   INT32 i;
   rgb_group *s,*d,rgb;
   struct object *o;
   struct image *img;

   if (!THIS->img) error("no image\n");

   getrgb(THIS,0,args,"Image.image->distancesq()");

   o=clone_object(image_program,0);
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
     int dist;
#define DISTANCE(A,B) \
     (sq((long)(A).r-(B).r)+sq((long)(A).g-(B).g)+sq((long)(A).b-(B).b))
      dist = DISTANCE(*s,rgb)>>8;
      d->r=d->g=d->b=testrange(dist);
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
      ((_dest).r=(_dest).g=(_dest).b=(MAXIMUM(1,255-(_value>>8))))
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
/*	 x++; hokuspokus /mirar */
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

/*
**! method object select_from(int x,int y)
**! method object select_from(int x,int y,int edge_value)
**!    Makes an grey-scale image, for alpha-channel use.
**!    
**!    This is very close to a floodfill.
**!    
**!    The image is scanned from the given pixel,
**!    filled with 255 if the color is the same,
**!    or 255 minus distance in the colorcube, squared, rightshifted
**!    8 steps (see <ref>distancesq</ref>).
**!
**!    When the edge distance is reached, the scan is stopped.
**!    Default edge value is 30.
**!    This value is squared and compared with the square of the 
**!    distance above.
**!
**! returns the new image object
**!
**! arg int x
**! arg int y
**!    originating pixel in the image
**!
**! see also: distancesq
*/

void image_select_from(INT32 args)
{
   struct object *o;
   struct image *img;
   INT32 low_limit=0;

   if (!THIS->img) error("no image\n");

   if (args<2 
       || sp[-args].type!=T_INT
       || sp[1-args].type!=T_INT)
      error("Illegal argument(s) to Image.image->select_from()\n");

   if (args>=3) 
      if (sp[2-args].type!=T_INT)
	 error("Illegal argument 3 (edge value) to Image.image->select_from()\n");
      else
	 low_limit=MAXIMUM(0,sp[2-args].u.integer);
   else
      low_limit=30;
   low_limit=low_limit*low_limit;

   o=clone_object(image_program,0);
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

      MARK_DISTANCE(pixel(img,sp[-args].u.integer,sp[1-args].u.integer),0);
   }

   pop_n_elems(args);
   push_object(o);
}


/*
**! method object apply_matrix(array(array(int|array(int))) matrix)
**! method object apply_matrix(array(array(int|array(int))) matrix,int r,int g,int b)
**! method object apply_matrix(array(array(int|array(int))) matrix,int r,int g,int b,int|float div)
**!     Applies a pixel-transform matrix, or filter, to the image.
**!    
**!	<pre>
**!                            2   2
**!	pixel(x,y)= base+ k ( sum sum pixel(x+k-1,y+l-1)*matrix(k,l) ) 
**!                           k=0 l=0 
**!     </pre>
**!	
**!	1/k is sum of matrix, or sum of matrix multiplied with div.
**!	base is given by r,g,b and is normally black.
**!
**!	<table><tr><td rowspan=2>
**!     blur (ie a 2d gauss function):
**!	<pre>
**!	({({1,2,1}),
**!	  ({2,5,2}),
**!	  ({1,2,1})})
**!	</pre>
**!	</td><td>
**!	<illustration>
**!	return lena()->apply_matrix(
**!	({({1,2,1}),
**!	  ({2,5,2}),
**!	  ({1,2,1})})
**!	);
**!	</illustration>
**!	</td><td>
**!	<illustration>
**!	return lena();
**!	</illustration>
**!	</td></tr>
**!	<tr><td></td><td>original</td></tr>
**!	
**!	<tr><td>
**!     sharpen (k>8, preferably 12 or 16):
**!	<pre>
**!	({({-1,-1,-1}),
**!	  ({-1, k,-1}),
**!	  ({-1,-1,-1})})
**!	</pre>
**!	</td><td>
**!	<illustration>
**!	return lena()->apply_matrix(
**!	({({-1,-1,-1}),
**!	  ({-1,14,-1}),
**!	  ({-1,-1,-1})})
**!	);
**!	</illustration>
**!	</td></tr>
**!
**!	<tr><td>
**!     edge detect:
**!	<pre>
**!	({({1, 1,1}),
**!	  ({1,-8,1}),
**!	  ({1, 1,1})})
**!	</pre>
**!	</td><td>
**!	<illustration>
**!	return lena()->apply_matrix(
**!	({({1, 1,1}),
**!	  ({1,-8,1}),
**!	  ({1, 1,1})})
**!	);
**!	</illustration>
**!	</td></tr>
**!
**!	<tr><td>
**!     horisontal edge detect (get the idea):
**!	<pre>
**!	({({0, 0,0}),
**!	  ({1,-2,1}),
**!	  ({0, 0,0})})
**!	</pre>
**!	</td><td>
**!	<illustration>
**!	return lena()->apply_matrix(
**!	({({0, 0,0}),
**!	  ({1,-2,1}),
**!	  ({0, 0,0})})
**!	);
**!	</illustration>
**!	</td></tr>
**!
**!	<tr><td rowspan=2>
**!     emboss (might prefer to begin with a <ref>grey</ref> image):
**!	<pre>
**!	({({2, 1, 0}),
**!	  ({1, 0,-1}),
**!	  ({0,-1,-2})}), 128,128,128, 3
**!	</pre>
**!	</td><td>
**!	<illustration>
**!	return lena()->apply_matrix(
**!	({({2, 1, 0}),
**!	  ({1, 0,-1}),
**!	  ({0,-1,-2})}), 128,128,128, 3
**!	);
**!	</illustration>
**!	</td><td>
**!	<illustration>
**!	return lena()->grey()->apply_matrix(
**!	({({2, 1, 0}),
**!	  ({1, 0,-1}),
**!	  ({0,-1,-2})}), 128,128,128, 3
**!	);
**!	</illustration>
**!	</td></tr>
**!	<tr><td></td><td>greyed</td></tr></table>
**!
**!	This function is not very fast.
**!
**! returns the new image object
**!
**! arg array(array(int|array(int)))
**!     the matrix; innermost is a value or an array with red, green, blue
**!     values for red, green, blue separation.
**! arg int r
**! arg int g
**! arg int b
**!	base level of result, default is zero
**! arg int|float div
**!	division factor, default is 1.0.
*/


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
      error("Illegal arguments to Image.image->apply_matrix()\n");

   if (args>3) 
      if (sp[1-args].type!=T_INT ||
	  sp[2-args].type!=T_INT ||
	  sp[3-args].type!=T_INT)
	 error("Illegal argument(s) (2,3,4) to Image.image->apply_matrix()\n");
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
      struct svalue s=sp[-args].u.array->item[i];
      if (s.type!=T_ARRAY) 
	 error("Illegal contents of (root) array (Image.image->apply_matrix)\n");
      if (width==-1)
	 width=s.u.array->size;
      else
	 if (width!=s.u.array->size)
	    error("Arrays has different size (Image.image->apply_matrix)\n");
   }
   if (width==-1) width=0;

   matrix=malloc(sizeof(rgbd_group)*width*height+1);
   if (!matrix) error("Out of memory");
   
   for (i=0; i<height; i++)
   {
      struct svalue s=sp[-args].u.array->item[i];
      for (j=0; j<width; j++)
      {
	 struct svalue s2=s.u.array->item[j];
	 if (s2.type==T_ARRAY && s2.u.array->size == 3)
	 {
	    struct svalue s3;
	    s3=s2.u.array->item[0];
	    if (s3.type==T_INT) matrix[j+i*width].r=s3.u.integer; 
	    else matrix[j+i*width].r=0;

	    s3=s2.u.array->item[1];
	    if (s3.type==T_INT) matrix[j+i*width].g=s3.u.integer;
	    else matrix[j+i*width].g=0;

	    s3=s2.u.array->item[2];
	    if (s3.type==T_INT) matrix[j+i*width].b=s3.u.integer; 
	    else matrix[j+i*width].b=0;
	 }
	 else if (s2.type==T_INT)
	    matrix[j+i*width].r=matrix[j+i*width].g=
	       matrix[j+i*width].b=s2.u.integer;
	 else
	    matrix[j+i*width].r=matrix[j+i*width].g=
	       matrix[j+i*width].b=0;
      }
   }

   o=clone_object(image_program,0);

CHRONO("apply_matrix, begin");

   if (THIS->img)
      img_apply_matrix((struct image*)o->storage,THIS,
		       width,height,matrix,default_rgb,div);

CHRONO("apply_matrix, end");

   free(matrix);

   pop_n_elems(args);
   push_object(o);
}

/*
**! method object outline()
**! method object outline(int olr,int olg,int olb)
**! method object outline(int olr,int olg,int olb,int bkgr,int bkgg,int bkgb)
**! method object outline(array(array(int)) mask)
**! method object outline(array(array(int)) mask,int olr,int olg,int olb)
**! method object outline(array(array(int)) mask,int olr,int olg,int olb,int bkgr,int bkgg,int bkgb)
**! method object outline_mask()
**! method object outline_mask(int bkgr,int bkgg,int bkgb)
**! method object outline_mask(array(array(int)) mask)
**! method object outline_mask(array(array(int)) mask,int bkgr,int bkgg,int bkgb)
**!     Makes an outline of this image, ie paints with the
**!	given color around the non-background pixels.
**!
**!	Default is to paint above, below, to the left and the right of 
**!	these pixels.
**!
**!	You can also run your own outline mask.
**!
**!	The outline_mask function gives the calculated outline as an
**!	alpha channel image of white and black instead.
**!    
**! returns the new image object
**!
**! arg array(array(int)) mask
**!     mask matrix. Default is <tt>({({0,1,0}),({1,1,1}),({0,1,0})})</tt>.
**! arg int olr
**! arg int olg
**! arg int olb
**!	outline color. Default is current.
**! arg int bkgr
**! arg int bkgg
**! arg int bkgb
**!	background color (what color to outline to);
**!	default is color of pixel 0,0.
**! arg int|float div
**!	division factor, default is 1.0.
**!
**! note
**!	no antialias!
*/

static void _image_outline(INT32 args,int mask)
{
   static unsigned char defaultmatrix[9]={0,1,0,1,1,1,0,1,0};
   unsigned char *matrix=defaultmatrix;
   int height=3;
   int width=3;
   unsigned char *tmp,*d;
   INT32 ai=0;
   rgb_group *s,*di;
   int x,y,xz;
   rgbl_group bkgl={0,0,0};
   struct object *o;
   struct image *img;

   if (!THIS->img || !THIS->xsize || !THIS->ysize)
      error("Image.image->outline: no image\n");

   if (args && sp[-args].type==T_ARRAY)
   {
      int i,j;
      height=sp[-args].u.array->size;
      width=-1;
      for (i=0; i<height; i++)
      {
	 struct svalue s=sp[-args].u.array->item[i];
	 if (s.type!=T_ARRAY) 
	    error("Image.image->outline: Illegal contents of (root) array\n");
	 if (width==-1)
	    width=s.u.array->size;
	 else
	    if (width!=s.u.array->size)
	       error("Image.image->outline: Arrays has different size\n");
      }
      if (width==-1) width=0;

      matrix=malloc(sizeof(int)*width*height+1);
      if (!matrix) error("Out of memory");
   
      for (i=0; i<height; i++)
      {
	 struct svalue s=sp[-args].u.array->item[i];
	 for (j=0; j<width; j++)
	 {
	    struct svalue s2=s.u.array->item[j];
	    if (s2.type==T_INT)
	       matrix[j+i*width]=(unsigned char)s2.u.integer;
	    else
	       matrix[j+i*width]=1;
	 }
      }
      ai=1;
   }

   push_int(THIS->xsize);
   push_int(THIS->ysize);
   o=clone_object(image_program,2);
   img=(struct image*)(o->storage);
   img->rgb=THIS->rgb;

   tmp=malloc((THIS->xsize+width)*(THIS->ysize+height));
   if (!tmp) { free_object(o); error("out of memory\n"); }
   MEMSET(tmp,0,(THIS->xsize+width)*(THIS->ysize+height));
 
   s=THIS->img;

   if (!mask)
   {
      if (args-ai==6)
      {
	 getrgbl(&bkgl,ai+3,args,"Image.image->outline");
	 pop_n_elems(args-(ai+3));
	 args=ai+3;
      }
      else if (args-ai==7)
      {
	 getrgbl(&bkgl,ai+4,args,"Image.image->outline");
	 pop_n_elems(args-(ai+4));
	 args=ai+4;
      }
      else
      {
	 bkgl.r=s->r;
	 bkgl.g=s->g;
	 bkgl.b=s->b;
      }
      getrgb(img,ai,args,"Image.image->outline");
   }
   else
   {
      if (args-ai==4)
      {
	 getrgbl(&bkgl,ai,args,"Image.image->outline_mask");
	 pop_n_elems(args-(ai+3));
	 args=ai+3;
      }
      else
      {
	 bkgl.r=s->r;
	 bkgl.g=s->g;
	 bkgl.b=s->b;
      }
   }

   xz=img->xsize;
   d=tmp+width/2+(height/2)*(width+xz);
   y=img->ysize;
   while (y--)
   {
      x=xz;
      while (x--)
      {
	 if (s->r!=bkgl.r || s->g!=bkgl.g || s->b!=bkgl.b)
	 {
	    unsigned char *d2=d-width/2-(height/2)*(width+xz);
	    int y2,x2;
	    unsigned char *s2=matrix;
	    y2=height;
	    while (y2--)
	    {
	       x2=width;
	       while (x2--) *(d2++)|=*(s2++);
	       d2+=xz;
	    }
	 }
	 s++;
	 d++;
      }
      d+=width;
   }

   di=img->img;
   d=tmp+width/2+(height/2)*(width+xz);
   s=THIS->img;
   y=img->ysize;
   while (y--)
   {
      x=xz;
      if (mask)
	 while (x--)
	 {
	    static rgb_group white={255,255,255};
	    static rgb_group black={0,0,0};
	    if (*d && s->r==bkgl.r && s->g==bkgl.g && s->b==bkgl.b)
	       *di=white;
	    else
	       *di=black;
	    s++;
	    d++;
	    di++;
	 }
      else
	 while (x--)
	 {
	    if (*d && s->r==bkgl.r && s->g==bkgl.g && s->b==bkgl.b)
	       *di=img->rgb;
	    else
	       *di=*s;
	    s++;
	    d++;
	    di++;
	 }
      d+=width;
   }

   if (matrix!=defaultmatrix) free(matrix);

   pop_n_elems(args);
   push_object(o);
}

static void image_outline(INT32 args)
{
   _image_outline(args,0);
}

static void image_outline_mask(INT32 args)
{
   _image_outline(args,1);
}

/*
**! method object modify_by_intensity(int r,int g,int b,int|array(int) v1,...,int|array(int) vn)
**!    Recolor an image from intensity values.
**!
**!    For each color an intensity is calculated, from r, g and b factors
**!    (see <ref>grey</ref>), this gives a value between 0 and max.
**!
**!    The color is then calculated from the values given, v1 representing
**!    the intensity value of 0, vn representing max, and colors between
**!    representing intensity values between, linear.
**!
**!	<table><tr valign=center>
**!	<td><illustration> return lena(); </illustration></td>
**!	<td><illustration> return lena()->grey()->modify_by_intensity(1,0,0,0,({255,0,0}),({0,255,0})); </illustration></td>
**!	</tr><tr valign=center>
**!	<td>original</td>
**!	<td>->grey()->modify_by_intensity(1,0,0, 0,({255,0,0}),({0,255,0}));</td>
**!	</tr></table>
**!
**! returns the new image object
**!
**! arg int r
**! arg int g
**! arg int b
**!	red, green, blue intensity factors
**! arg int|array(int) v1
**! arg int|array(int) vn
**!	destination color
**!
**! see also: grey, `*, color
*/


void image_modify_by_intensity(INT32 args)
{
   INT32 x,y;
   rgbl_group rgb;
   rgb_group *list;
   rgb_group *s,*d;
   struct object *o;
   struct image *img;
   long div;

   if (!THIS->img) error("no image\n");
   if (args<5) 
      error("too few arguments to Image.image->modify_by_intensity()\n");
   
   getrgbl(&rgb,0,args,"Image.image->modify_by_intensity()");
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

   o=clone_object(image_program,0);
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
      int q = ((((int)s->r)*rgb.r+((int)s->g)*rgb.g+((int)s->b)*rgb.b)/div);
      *(d++)=list[testrange( q )];
      s++;
   }
   THREADS_DISALLOW();

   free(list);

   pop_n_elems(args);
   push_object(o);
}


/*
**! method object map_closest(array(array(int)) colors)
**! method object map_fast(array(array(int)) colors)
**! method object map_fs(array(array(int)) colors)
**! method array select_colors(int num)
**!	Compatibility functions. Do not use!
**!
**!	Replacement examples:
**!
**!	Old code:
**!	<pre>img=map_fs(img->select_colors(200));</pre>
**!	New code:
**!	<pre>img=Image.colortable(img,200)->floyd_steinberg()->map(img);</pre>
**!
**!	Old code:
**!	<pre>img=map_closest(img->select_colors(17)+({({255,255,255}),({0,0,0})}));</pre>
**!	New code:
**!	<pre>img=Image.colortable(img,19,({({255,255,255}),({0,0,0})}))->map(img);</pre>
*/

void _image_map_compat(INT32 args,int fs) /* compat function */
{
  struct neo_colortable *nct;
  struct object *o,*co;
  struct image *this = THIS;
  rgb_group *d;

  co=clone_object(image_colortable_program,args);
  nct=(struct neo_colortable*)get_storage(co,image_colortable_program);

  if (fs) image_colortable_internal_floyd_steinberg(
	   (struct neo_colortable *)get_storage(co,image_colortable_program));

  push_int(this->xsize);
  push_int(this->ysize);
  o=clone_object(image_program,2);
      
  d=((struct image*)(o->storage))->img;

  THREADS_ALLOW();

  image_colortable_map_image(nct,this->img,d,
			     this->xsize*this->ysize,this->xsize);

  THREADS_DISALLOW();

  free_object(co);
  push_object(o);
}

void image_map_compat(INT32 args) { _image_map_compat(args,0); }
void image_map_fscompat(INT32 args) { _image_map_compat(args,1); }

void image_select_colors(INT32 args)
{
   int colors;
   struct object *o;

   if (args<1
      || sp[-args].type!=T_INT)
      error("Illegal argument to Image.image->select_colors()\n");

   colors=sp[-args].u.integer;
   pop_n_elems(args);

   push_object(THISOBJ); THISOBJ->refs++;
   push_int(colors);

   o=clone_object(image_colortable_program,2);
   image_colortable_cast_to_array((struct neo_colortable*)
				  get_storage(o,image_colortable_program));
   free_object(o);
}

/*
**! method object write_lsb_rgb(string what)
**! method object write_lsb_grey(string what)
**! method string read_lsb_rgb()
**! method string read_lsb_grey()
**!    	These functions read/write in the least significant bit
**!     of the image pixel values. The _rgb() functions
**!	read/write on each of the red, green and blue values,
**!	and the grey keeps the same lsb on all three.
**!
**!	The string is nullpadded or cut to fit.
**!
**! returns the current object or the read string
**!
**! arg string what
**!	the hidden message
*/

void image_write_lsb_rgb(INT32 args)
{
   int n,l,b;
   rgb_group *d;
   char *s;

   if (args<1
       || sp[-args].type!=T_STRING)
      error("Illegal argument to Image.image->write_lowbit()\n");
   
   s=sp[-args].u.string->str;
   l=sp[-args].u.string->len;

   n=THIS->xsize * THIS->ysize;
   d=THIS->img;

   b=128;

   if (d)
   while (n--)
   {
      if (b==0) { b=128; l--; s++; }
      if (l>0) d->r=(d->r&254)|(((*s)&b)?1:0); else d->r&=254; b>>=1;
      if (b==0) { b=128; l--; s++; }
      if (l>0) d->g=(d->r&254)|(((*s)&b)?1:0); else d->g&=254; b>>=1;
      if (b==0) { b=128; l--; s++; }
      if (l>0) d->b=(d->r&254)|(((*s)&b)?1:0); else d->b&=254; b>>=1;
      d++;
   }

   pop_n_elems(args);
   
   THISOBJ->refs++;
   push_object(THISOBJ);
}

void image_read_lsb_rgb(INT32 args)
{
   int n,b;
   rgb_group *s;
   char *d;
   struct pike_string *ps;

   ps=begin_shared_string((THIS->xsize*THIS->ysize*sizeof(rgb_group)+7)>>3);

   d=ps->str;

   n=THIS->xsize * THIS->ysize;
   s=THIS->img;

   b=128;

   MEMSET(d,0,(THIS->xsize*THIS->ysize*sizeof(rgb_group)+7)>>3);

   if (s)
   while (n--)
   {
      if (b==0) { b=128; d++; }
      *d|=(s->r&1)*b; b>>=1;
      if (b==0) { b=128; d++; }
      *d|=(s->g&1)*b; b>>=1;
      if (b==0) { b=128; d++; }
      *d|=(s->b&1)*b; b>>=1;
      s++;
   }

   pop_n_elems(args);
   push_string(end_shared_string(ps));
}

void image_write_lsb_grey(INT32 args)
{
   int n,l,b;
   rgb_group *d;
   char *s;

   if (args<1
       || sp[-args].type!=T_STRING)
      error("Illegal argument to Image.image->write_lowbit()\n");
   
   s=sp[-args].u.string->str;
   l=sp[-args].u.string->len;

   n=THIS->xsize * THIS->ysize;
   d=THIS->img;

   b=128;

   if (d)
   while (n--)
   {
      if (b==0) { b=128; l--; s++; }
      if (l>0) 
      {
	 d->r=(d->r&254)|(((*s)&b)?1:0); 
	 d->g=(d->g&254)|(((*s)&b)?1:0); 
	 d->b=(d->b&254)|(((*s)&b)?1:0); 
      }
      else 
      {
	 d->r&=254;
	 d->g&=254;
	 d->b&=254;
      }
      b>>=1;
      d++;
   }

   pop_n_elems(args);
   
   THISOBJ->refs++;
   push_object(THISOBJ);
}

void image_read_lsb_grey(INT32 args)
{
   int n,b;
   rgb_group *s;
   char *d;
   struct pike_string *ps;

   ps=begin_shared_string((THIS->xsize*THIS->ysize+7)>>3);

   d=ps->str;

   n=THIS->xsize * THIS->ysize;
   s=THIS->img;

   b=128;

   MEMSET(d,0,(THIS->xsize*THIS->ysize+7)>>3);

   if (s)
   while (n--)
   {
      if (b==0) { b=128; d++; }
      *d|=(s->r&1)*b; b>>=1;
      s++;
   }

   pop_n_elems(args);
   push_string(end_shared_string(ps));
}


/***************** global init etc *****************************/

#define RGB_TYPE "int|void,int|void,int|void,int|void"

extern void init_font_programs(void);
extern void exit_font(void);
extern void init_colortable_programs(void);
extern void exit_colortable(void);

/* encoders */

extern void init_image_gif(void);
extern void exit_image_gif(void);
extern void init_image_pnm(void);
extern void exit_image_pnm(void);
extern void init_image_xwd(void);
extern void exit_image_xwd(void);
extern void init_image_x(void);
extern void exit_image_x(void);

/* dynamic encoders (dependent on other modules, loaded dynamically) */

extern struct object* init_image_png(void);
extern void exit_image_png(void);

static struct pike_string 
   *magic_JPEG, 
   *magic_XFace, 
   *magic_PNG;

static struct object *png_object=NULL;

static void image_index_magic(INT32 args)
{
   struct svalue tmp;
   if (args!=1) 
      error("Image.`[]: Too few or too many arguments\n");
   if (sp[-1].type!=T_STRING)
      error("Image.`[]: Illegal type of argument\n");
   if (sp[-1].u.string==magic_JPEG)
   {
      pop_stack();
      push_string(make_shared_string("_Image_JPEG"));
      push_int(0);
      SAFE_APPLY_MASTER("resolv",2);
      return;
   }
   else if (sp[-1].u.string==magic_XFace)
   {
      pop_stack();
      push_string(make_shared_string("_Image_XFace"));
      push_int(0);
      SAFE_APPLY_MASTER("resolv",2);
      return;
   }
   else if (sp[-1].u.string==magic_PNG)
   {
      pop_stack();
      if (!png_object)
	 png_object=init_image_png();
      png_object->refs++;
      push_object(png_object);
      return;
   }
   push_object(THISOBJ); THISOBJ->refs++;
   tmp=sp[-1], sp[-1]=sp[-2], sp[-2]=tmp;
   f_arrow(2);
}

void pike_module_init(void)
{
   int i;

   magic_JPEG=make_shared_string("JPEG");
   magic_PNG=make_shared_string("PNG");
   magic_XFace=make_shared_string("XFace");

   image_noise_init();

   start_new_program();
   add_storage(sizeof(struct image));

   add_function("create",image_create,
		"function(int|void,int|void,"RGB_TYPE":void)",0);
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
   add_function("togif",image_togif,
		"function(:string)",0);
   add_function("togif_fs",image_togif_fs,
		"function(:string)",0);
   add_function("gif_begin",image_gif_begin,
		"function(int:string)",0);
   add_function("gif_add",image_gif_add,
		"function(int|void,int|void,int|float:string)"
		"|function(int|void,int|void,array(array(int)),int|float:string)",0);
   add_function("gif_add_fs",image_gif_add_fs,
		"function(int|void,int|void,int|float:string)"
		"|function(int|void,int|void,array(array(int)),int|float:string)",0);
   add_function("gif_add_nomap",image_gif_add_nomap,
		"function(int|void,int|void,int|float:string)"
		"|function(int|void,int|void,array(array(int)),int|float:string)",0);
   add_function("gif_add_fs_nomap",image_gif_add_fs_nomap,
		"function(int|void,int|void,int|float:string)"
		"|function(int|void,int|void,array(array(int)),int|float:string)",0);
   add_function("gif_end",image_gif_end,
		"function(:string)",0);
   add_function("gif_netscape_loop",image_gif_netscape_loop,
		"function(:string)",0);

   add_function("cast",image_cast, "function(string:string)",0);
   add_function("to8bit",image_to8bit,
		"function(array(array(int)):string)",0);
   add_function("to8bit_closest",image_to8bit,
		"function(array(array(int)):string)",0);
   add_function("to8bit_fs",image_to8bit,
		"function(:string)",0);
   add_function("torgb",image_torgb,
		"function(:string)",0);
   add_function("tozbgr",image_tozbgr,
		"function(array(array(int)):string)",0);
   add_function("to8bit_rgbcube",image_to8bit_rgbcube,
		"function(int,int,int,void|string:string)",0);
   add_function("tobitmap",image_tobitmap,
		"function(:string)",0);
   add_function("to8bit_rgbcube_rdither",image_to8bit_rgbcube_rdither,
		"function(int,int,int,void|string:string)",0);


   add_function("copy",image_copy,
		"function(void|int,void|int,void|int,void|int,"RGB_TYPE":object)",0);
   add_function("autocrop",image_autocrop,
		"function(void|int ...:object)",0);
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

   add_function("add_layers",image_add_layers,
		"function(int|array|void ...:object)",0);

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
   add_function("gradients",image_gradients,
		"function(array(int)|float ...:object)",0);
   add_function("polygone",image_polyfill,
		"function(array(float|int) ...:object)",0);
   add_function("polyfill",image_polyfill,
		"function(array(float|int) ...:object)",0);

   add_function("gray",image_grey,
		"function("RGB_TYPE":object)",0);
   add_function("grey",image_grey,
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

   add_function("rgb_to_hsv",image_rgb_to_hsv, "function(void:object)",0);
   add_function("hsv_to_rgb",image_hsv_to_rgb,"function(void:object)",0);

   add_function("select_from",image_select_from,
		"function(int,int:object)",0);

   add_function("apply_matrix",image_apply_matrix,
                "function(array(array(int|array(int))), void|int ...:object)",0);
   add_function("outline",image_outline,
                "function(void|array(array(int)):object)"
                "|function(array(array(int)),int,int,int,void|int:object)"
                "|function(array(array(int)),int,int,int,int,int,int,void|int:object)"
                "|function(int,int,int,void|int:object)"
                "|function(int,int,int,int,int,int,void|int:object)",0);
   add_function("outline_mask",image_outline_mask,
                "function(void|array(array(int)):object)"
                "|function(array(array(int)),int,int,int:object)",0);
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

   add_function("map_closest",image_map_compat,
                "function(array(array(int)):object)",0);
   add_function("map_fast",image_map_compat,
                "function(array(array(int)):object)",0);
   add_function("map_fs",image_map_fscompat,
                "function(array(array(int)):object)",0);
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
		
   add_function("read_lsb_rgb",image_read_lsb_rgb,
		"function(:object)",0);
   add_function("write_lsb_rgb",image_write_lsb_rgb,
		"function(:object)",0);
   add_function("read_lsb_grey",image_read_lsb_rgb,
		"function(:object)",0);
   add_function("write_lsb_grey",image_write_lsb_rgb,
		"function(:object)",0);

   add_function("orient4",image_orient4,
                "function(:array(object))",0);
   add_function("orient",image_orient,
                "function(:object)",0);


   set_init_callback(init_image_struct);
   set_exit_callback(exit_image_struct);
  
   image_program=end_program();
   add_program_constant("image",image_program, 0);
  
   for (i=0; i<CIRCLE_STEPS; i++) 
      circle_sin_table[i]=(INT32)4096*sin(((double)i)*2.0*3.141592653589793/(double)CIRCLE_STEPS);

   init_font_programs();
   init_colortable_programs();

   add_function("`[]",image_index_magic,
		"function(string:object)",0);

   init_image_gif();
   init_image_pnm();
   init_image_xwd();
   init_image_x();
}

void pike_module_exit(void) 
{
  if(image_program)
  {
    free_program(image_program);
    image_program=0;
  }
  exit_font();
  exit_colortable();

  exit_image_gif();
  exit_image_pnm();
  exit_image_xwd();
  if (png_object) 
  {
     free_object(png_object);
     png_object=NULL;
     exit_image_png();
  }
  exit_image_x();

  free_string(magic_PNG);
  free_string(magic_JPEG);
  free_string(magic_XFace);
}


