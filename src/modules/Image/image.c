/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: image.c,v 1.215 2004/05/19 00:10:10 nilsson Exp $
*/

/*
**! module Image
**! class Image
**!
**!	The main object of the <ref>Image</ref> module, this object
**!	is used as drawing area, mask or result of operations.
**!
**!	basic: <br>
**!	<ref>clear</ref>,
**!	<ref>clone</ref>,
**!	<ref>create</ref>, 
**!	<ref>xsize</ref>,
**!	<ref>ysize</ref>
**!
**!	plain drawing: <br>
**!	<ref>box</ref>,
**!	<ref>circle</ref>,
**!	<ref>getpixel</ref>, 
**!	<ref>line</ref>,
**!	<ref>setcolor</ref>,
**!	<ref>setpixel</ref>, 
**!	<ref>threshold</ref>,
**!	<ref>polyfill</ref>
**!
**!	operators: <br>
**!	<ref>`&</ref>,
**!	<ref>`*</ref>,
**!	<ref>`+</ref>,
**!	<ref>`-</ref>,
**!	<ref>`==</ref>,
**!	<ref>`></ref>,
**!	<ref>`&lt;</ref>,
**!	<ref>`|</ref>
**!
**!	pasting images: <br>
**!	<ref>paste</ref>,
**!	<ref>paste_alpha</ref>,
**!	<ref>paste_alpha_color</ref>,
**!	<ref>paste_mask</ref>
**!
**!	getting subimages, scaling, rotating: <br>
**!	<ref>autocrop</ref>, 
**!	<ref>clone</ref>,
**!	<ref>copy</ref>, 
**!	<ref>dct</ref>,
**!	<ref>mirrorx</ref>, 
**!	<ref>rotate</ref>,
**!	<ref>rotate_ccw</ref>,
**!	<ref>rotate_cw</ref>,
**!	<ref>rotate_expand</ref>, 
**!	<ref>scale</ref>, 
**!	<ref>skewx</ref>,
**!	<ref>skewx_expand</ref>,
**!	<ref>skewy</ref>,
**!	<ref>skewy_expand</ref>
**!
**!	calculation by pixels: <br>
**!	<ref>apply_matrix</ref>, 
**!	<ref>change_color</ref>,
**!	<ref>color</ref>,
**!	<ref>distancesq</ref>, 
**!	<ref>grey</ref>,
**!	<ref>invert</ref>, 
**!	<ref>modify_by_intensity</ref>,
**!	<ref>outline</ref>
**!	<ref>select_from</ref>, 
**!	<ref>rgb_to_hsv</ref>,
**!	<ref>hsv_to_rgb</ref>,<br>
**!
**!	<ref>average</ref>,
**!	<ref>max</ref>,
**!	<ref>min</ref>,
**!	<ref>sum</ref>,
**!	<ref>sumf</ref>,
**!	<ref>find_min</ref>,
**!	<ref>find_max</ref>
**!
**!	special pattern drawing:<br>
**!	<ref>noise</ref>,
**!	<ref>turbulence</ref>,
**!	<ref>test</ref>,
**!	<ref>tuned_box</ref>,
**!	<ref>gradients</ref>,
**!	<ref>random</ref>
**!
**! see also: Image, Image.Font, Image.Colortable, Image.X
*/

#include "global.h"
#include "image_machine.h"

#include <math.h>
#include <ctype.h>

#include "stralloc.h"
#include "global.h"
RCSID("$Id: image.c,v 1.215 2004/05/19 00:10:10 nilsson Exp $");
#include "pike_macros.h"
#include "object.h"
#include "interpret.h"
#include "svalue.h"
#include "threads.h"
#include "array.h"
#include "pike_error.h"
#include "module_support.h"


#include "image.h"
#include "colortable.h"
#include "builtin_functions.h"

#ifdef ASSEMBLY_OK
#include "assembly.h"
#endif


#define sp Pike_sp

extern struct program *image_program;
extern struct program *image_colortable_program;

#ifdef THIS
#undef THIS /* Needed for NT */
#endif

#define THIS ((struct image *)(Pike_fp->current_storage))
#define THISOBJ (Pike_fp->current_object)

#define testrange(x) ((COLORTYPE)MAXIMUM(MINIMUM(DOUBLE_TO_INT(x),255),0))

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

   if (max > 3 && args-args_start>=4) 
      if (sp[3-args+args_start].type!=T_INT)
      {
         Pike_error("Illegal alpha argument to %s\n",name);
	 return 0; /* avoid stupid warning */
      }
      else
      {
         img->alpha=sp[3-args+args_start].u.integer;
	 return 4;
      }
   else
   {
      img->alpha=0;
      return 3;
   }
}

static INLINE void getrgbl(rgbl_group *rgb,INT32 args_start,INT32 args,char *name)
{
   INT32 i;
   if (args-args_start<3) return;
   for (i=0; i<3; i++)
      if (sp[-args+i+args_start].type!=T_INT)
         Pike_error("Illegal r,g,b argument to %s\n",name);
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
   double qdiv=1.0/div;

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
	    r += DO_NOT_WARN((int)(matrix[i+j*width].r*img->img[xp+yp*img->xsize].r));
	    g += DO_NOT_WARN((int)(matrix[i+j*width].g*img->img[xp+yp*img->xsize].g));
	    b += DO_NOT_WARN((int)(matrix[i+j*width].b*img->img[xp+yp*img->xsize].b));
#ifdef MATRIX_DEBUG
	    fprintf(stderr,"%d,%d %d,%d->%d,%d,%d\n",
		    i,j,xp,yp,
		    img->img[x+i+(y+j)*img->xsize].r,
		    img->img[x+i+(y+j)*img->xsize].g,
		    img->img[x+i+(y+j)*img->xsize].b);
#endif
	    sumr += DO_NOT_WARN((int)matrix[i+j*width].r);
	    sumg += DO_NOT_WARN((int)matrix[i+j*width].g);
	    sumb += DO_NOT_WARN((int)matrix[i+j*width].b);
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


static void img_apply_matrix(struct image *dest,
			     struct image *img,
			     int width,int height,
			     rgbd_group *matrix,
			     double div,
			     rgb_group default_rgb)
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
   
THREADS_DISALLOW();

   d=xalloc(sizeof(rgb_group)*img->xsize*img->ysize +1);

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
	 r=default_rgb.r+DOUBLE_TO_INT(r*qr+0.5); dp->r=testrange(r);
	 g=default_rgb.g+DOUBLE_TO_INT(g*qg+0.5); dp->g=testrange(g);
	 b=default_rgb.b+DOUBLE_TO_INT(b*qb+0.5); dp->b=testrange(b);
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
**! method void create(int xsize,int ysize,Color color)
**! method void create(int xsize,int ysize,int r,int g,int b)
**! method void create(int xsize,int ysize,int r,int g,int b,int alpha)
**
**! method void create(int xsize,int ysize,string method,method ...)
**! 	Initializes a new image object.
**!
**!	<table><tr valign=center>
**!	<td><illustration> return lena()->clear(0,0,0); </illustration></td>
**!	<td><illustration> return lena()->clear(255,128,0); </illustration></td>
**!	</tr><tr>
**!	<td>Image.Image<wbr>(XSIZE,YSIZE)</td>
**!	<td>Image.Image<wbr>(XSIZE,YSIZE,255,128,0)</td>
**!	</tr></table>
**!
**!	The image can also be calculated from some special methods,
**!	for convinience:
**!
**!	<pre>
**!	channel modes; followed by a number of 1-char-per-pixel strings 
**!	or image objects (where red channel will be used), 
**!	or an integer value:
**!	  "grey" : make a grey image (needs 1 source: grey)
**!	  "rgb"  : make an rgb image (needs 3 sources: red, green and blue)
**!	  "cmyk" : make a rgb image from cmyk (cyan, magenta, yellow, black)
**!
**!	generate modes; all extra arguments is given to the
**!	generation function. These has the same name as the method:
**!	  "<ref>test</ref>," 
**!	  "<ref>gradients</ref>"
**!	  "<ref>noise</ref>"
**!	  "<ref>turbulence</ref>"
**!	  "<ref>random</ref>"
**!	  "<ref>randomgrey</ref>"
**!	specials cases:
**!	  "<ref>tuned_box</ref>" (coordinates is automatic)
**!	</pre>
**!
**! arg int xsize
**! arg int ysize
**! 	size of (new) image in pixels
**! arg Color color
**! arg int r
**! arg int g
**! arg int b
**!     background color (will also be current color),
**!     default color is black
**! arg int alpha
**! 	default alpha channel value
**! see also: copy, clone, Image.Image
**! bugs
**!	SIGSEGVS can be caused if the size is too big, due
**!	to unchecked overflow - 
**!	(xsize*ysize)&MAXINT is small enough to allocate.
*/

int image_too_big(INT_TYPE xsize,INT_TYPE ysize)
{
   register INT_TYPE a,b,c,d;

   if (xsize<0 || ysize<0) return 1;

   if (xsize<0x20000000) xsize*=sizeof(rgb_group);
   else if (ysize<0x20000000) ysize*=sizeof(rgb_group);
   else return 1;

   a=(xsize>>16);
   b=xsize&0xffff;
   c=(ysize>>16);
   d=ysize&0xffff;

   /* check for overflow */
   if ((a&&c) || ((b*d>>16)&0xffff) + (a*d) + (b*c) > 0x7fff) return 1;

   return 0;
}

void img_read_get_channel(int arg,char *name,INT32 args,
			  int *m,unsigned char **s,COLORTYPE *c)
{
   struct image *img;
   if (arg>args)
      SIMPLE_TOO_FEW_ARGS_ERROR("create_method",1+arg);
   switch (sp[arg-args-1].type)
   {
      case T_INT:
	 *c=(COLORTYPE)sp[arg-args-1].u.integer;
	 *s=c;
	 *m=0;
	 break;
      case T_STRING:
	 if (sp[arg-args-1].u.string->size_shift)
	    Pike_error("create_method: argument %d (%s channel): "
		  "wide strings are not supported (yet)\n",arg+1,name);
	 if (sp[arg-args-1].u.string->len!=THIS->xsize*THIS->ysize)
	    Pike_error("create_method: argument %d (%s channel): "
		  "string is %ld characters, expected %ld\n",
		  arg+1, name,
		  DO_NOT_WARN((long)sp[arg-args-1].u.string->len),
		  DO_NOT_WARN((long)(THIS->xsize*THIS->ysize)));
	 *s=(unsigned char *)sp[arg-args-1].u.string->str;
	 *m=1;
	 break;
      case T_OBJECT:
	 img=(struct image*)get_storage(sp[arg-args-1].u.object,image_program);
	 if (!img) 
	    Pike_error("create_method: argument %d (%s channel): "
		  "not an image object\n",arg+1,name);
	 if (!img->img) 
	    Pike_error("create_method: argument %d (%s channel): "
		  "uninitialized image object\n",arg+1,name);
	 if (img->xsize!=THIS->xsize || img->ysize!=THIS->ysize) 
	    Pike_error("create_method: argument %d (%s channel): "
		  "size is wrong, %"PRINTPIKEINT"dx%"PRINTPIKEINT"d;"
		       " expected %"PRINTPIKEINT"dx%"PRINTPIKEINT"d\n",
		  arg+1,name,img->xsize,img->ysize,
		  THIS->xsize,THIS->ysize);
	 *s=(COLORTYPE*)img->img;
	 *m=sizeof(rgb_group);
	 break;
      default:
	 Pike_error("create_method: argument %d (%s channel): "
	       "illegal type\n",arg+1,name);
   }
}

void img_read_grey(INT32 args)
{
   int m1;
   COLORTYPE c1;
   unsigned char *s1;
   int n=THIS->xsize*THIS->ysize;
   rgb_group *d;
   img_read_get_channel(1,"grey",args,&m1,&s1,&c1);
   d=THIS->img=(rgb_group*)xalloc(sizeof(rgb_group)*n);
   switch (m1)
   {
      case 0: MEMSET(d,c1,n*sizeof(rgb_group)); break;
      case 1: while (n--) { d->r=d->g=d->b=*(s1++); d++; } break;
      default: while (n--) { d->r=d->g=d->b=*s1; s1+=m1; d++; }
   }
}

void img_read_rgb(INT32 args)
{
   int m1,m2,m3;
   unsigned char *s1,*s2,*s3;
   int n=THIS->xsize*THIS->ysize;
   rgb_group *d,rgb;
   img_read_get_channel(1,"red",args,&m1,&s1,&(rgb.r));
   img_read_get_channel(2,"green",args,&m2,&s2,&(rgb.g));
   img_read_get_channel(3,"blue",args,&m3,&s3,&(rgb.b));
   d=THIS->img=(rgb_group*)xalloc(sizeof(rgb_group)*n);

   switch (m1|(m2<<4)|(m3<<4))
   {
      case 0: /* all constant */
	 while (n--) *(d++)=rgb;
	 break;
      case 0x111: /* all is one-byte */
	 while (n--)
	 {
	    d->r=*(s1++);
	    d->g=*(s2++);
	    d->b=*(s3++);
	    d++;
	 }
	 break;
      case 0x333: /* all is rgb-source */
	 while (n--)
	 {
	    d->r=*s1;
	    d->g=*s2;
	    d->b=*s3;
	    s1+=3;
	    s2+=3;
	    s3+=3;
	    d++;
	 }
	 break;
      default:
	 while (n--)
	 {
	    d->r=*s1;
	    d->g=*s2;
	    d->b=*s3;
	    s1+=m1;
	    s2+=m2;
	    s3+=m3;
	    d++;
	 }
	 break;
   }
}

void img_read_cmyk(INT32 args)
{
   int m1,m2,m3,m4;
   unsigned char *s1,*s2,*s3,*s4;
   int n=THIS->xsize*THIS->ysize;
   rgb_group *d,rgb;
   COLORTYPE k;
   img_read_get_channel(1,"cyan",args,&m1,&s1,&(rgb.r));
   img_read_get_channel(2,"magenta",args,&m2,&s2,&(rgb.g));
   img_read_get_channel(3,"yellow",args,&m3,&s3,&(rgb.b));
   img_read_get_channel(4,"black",args,&m4,&s4,&k);
   d=THIS->img=(rgb_group*)xalloc(sizeof(rgb_group)*n);

   while (n--)
   {
      d->r=COLORMAX-*s1-*s4;
      d->g=COLORMAX-*s2-*s4;
      d->b=COLORMAX-*s3-*s4;
      s1+=m1;
      s2+=m2;
      s3+=m3;
      s4+=m4;
      d++;
   }
}

void img_read_cmy(INT32 args)
{
   int m1,m2,m3;
   unsigned char *s1,*s2,*s3;
   int n=THIS->xsize*THIS->ysize;
   rgb_group *d,rgb;
   img_read_get_channel(1,"cyan",args,&m1,&s1,&(rgb.r));
   img_read_get_channel(2,"magenta",args,&m2,&s2,&(rgb.g));
   img_read_get_channel(3,"yellow",args,&m3,&s3,&(rgb.b));
   d=THIS->img=(rgb_group*)xalloc(sizeof(rgb_group)*n);

   while (n--)
   {
      d->r=COLORMAX-*s1;
      d->g=COLORMAX-*s2;
      d->b=COLORMAX-*s3;
      s1+=m1;
      s2+=m2;
      s3+=m3;
      d++;
   }
}

static void image_gradients(INT32 args);
static void image_tuned_box(INT32 args);
static void image_test(INT32 args);

static struct pike_string *s_grey,*s_rgb,*s_cmyk,*s_cmy;
static struct pike_string *s_test,*s_gradients,*s_noise,*s_turbulence,
  *s_random,*s_randomgrey,*s_tuned_box;

void image_create_method(INT32 args)
{
   struct image *img;

   if (!args)
      SIMPLE_TOO_FEW_ARGS_ERROR("create_method",1);

   if (sp[-args].type!=T_STRING)
      SIMPLE_BAD_ARG_ERROR("create_method",1,"string");

   MAKE_CONST_STRING(s_grey,"grey");
   MAKE_CONST_STRING(s_rgb,"rgb");
   MAKE_CONST_STRING(s_cmyk,"cmyk");
   MAKE_CONST_STRING(s_cmy,"cmy");
   MAKE_CONST_STRING(s_test,"test");
   MAKE_CONST_STRING(s_gradients,"gradients");
   MAKE_CONST_STRING(s_noise,"noise");
   MAKE_CONST_STRING(s_turbulence,"turbulence");
   MAKE_CONST_STRING(s_random,"random");
   MAKE_CONST_STRING(s_randomgrey,"randomgrey");
   MAKE_CONST_STRING(s_tuned_box,"tuned_box");

   if (THIS->xsize<=0 || THIS->ysize<=0)
      Pike_error("create_method: image size is too small\n");

   if (sp[-args].u.string==s_grey)
   {
      img_read_grey(args-1);
      pop_n_elems(2);
      ref_push_object(THISOBJ);
      return;
   }
   if (sp[-args].u.string==s_rgb)
   {
      img_read_rgb(args-1);
      pop_n_elems(2);
      ref_push_object(THISOBJ);
      return;
   }
   if (sp[-args].u.string==s_cmyk)
   {
      img_read_cmyk(args-1);
      pop_n_elems(2);
      ref_push_object(THISOBJ);
      return;
   }
   if (sp[-args].u.string==s_cmy)
   {
      img_read_cmyk(args-1);
      pop_n_elems(2);
      ref_push_object(THISOBJ);
      return;
   }

   if (sp[-args].u.string==s_test)
      image_test(args-1);
   else if (sp[-args].u.string==s_gradients) {
     if(args<2) {
       push_int(THIS->xsize/2);
       push_int(0);
       push_int(0); push_int(0); push_int(0);
       f_aggregate(5);
       push_int(THIS->xsize/2);
       push_int(THIS->ysize);
       push_int(255); push_int(255); push_int(255);
       f_aggregate(5);
       args+=2;
     }
     image_gradients(args-1);
   }
   else if (sp[-args].u.string==s_noise)
      image_noise(args-1);
   else if (sp[-args].u.string==s_turbulence)
      image_turbulence(args-1);
   else if (sp[-args].u.string==s_random)
      image_random(args-1);
   else if (sp[-args].u.string==s_randomgrey)
      image_randomgrey(args-1);
   else if (sp[-args].u.string==s_tuned_box)
   {
      if (args<2) push_int(0);

      THIS->img=(rgb_group*)
	 xalloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize);

      if (args>2) pop_n_elems(args-2);
      push_int(0); stack_swap();
      push_int(0); stack_swap();
      push_int(THIS->xsize-1); stack_swap();
      push_int(THIS->ysize-1); stack_swap();
      image_tuned_box(5);
      return;
   }
   else 
      Pike_error("create_method: unknown method\n");

   /* on stack: "string" image */
   /* want: put that image in this, crap that image */
   img=(struct image*)get_storage(sp[-1].u.object,image_program);
   THIS->img=img->img;
   img->img=NULL;
   pop_n_elems(2);
   ref_push_object(THISOBJ);
}

void image_create(INT32 args)
{
   if (args<2) return;
   if (sp[-args].type!=T_INT||
       sp[1-args].type!=T_INT)
      bad_arg_error("Image.Image->create",sp-args,args,0,"",sp-args,
		"Bad arguments to Image.Image->create()\n");

   if (THIS->img) { free(THIS->img); THIS->img=NULL; }
	
   THIS->xsize=sp[-args].u.integer;
   THIS->ysize=sp[1-args].u.integer;
   if (THIS->xsize<0) THIS->xsize=0;
   if (THIS->ysize<0) THIS->ysize=0;

   if (image_too_big(THIS->xsize,THIS->ysize)) 
      Pike_error("Image.Image->create(): image too large (>2Gpixels)\n");

   if (args>2 && sp[2-args].type==T_STRING &&
       !image_color_svalue(sp+2-args,&(THIS->rgb))) 
      /* don't try method "lightblue", etc */
   {
      image_create_method(args-2);
      pop_n_elems(3);
      return ;
   }
   else
      getrgb(THIS,2,args,args,"Image.Image->create()"); 

   THIS->img=xalloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize +1);

   img_clear(THIS->img,THIS->rgb,THIS->xsize*THIS->ysize);
   pop_n_elems(args);
}

/*
**! method object clone()
**! method object clone(int xsize,int ysize)
**! method object clone(int xsize,int ysize,int r,int g,int b)
**! method object clone(int xsize,int ysize,int r,int g,int b,int alpha)
**! 	Copies to or initialize a new image object.
**!
**!	<table><tr valign=center>
**!	<td><illustration> return lena(); </illustration></td>
**!	<td><illustration> return lena()->clone(); </illustration></td>
**!	<td><illustration> return lena()->clone(50,50); </illustration></td>
**!	</tr><tr>
**!	<td>original</td>
**!	<td>clone</td>
**!	<td>clone(50,50)</td>
**!	</tr></table>
**!	
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

static void my_free_object(struct object *o)
{
  free_object(o);
}

void image_clone(INT32 args)
{
   struct object *o;
   struct image *img;
   ONERROR err;

   if (args)
      if (args<2||
	  sp[-args].type!=T_INT||
	  sp[1-args].type!=T_INT)
	 bad_arg_error("Image",sp-args,args,0,"",sp-args,
		"Bad arguments to Image()\n");

   o=clone_object(image_program,0);
   SET_ONERROR(err, my_free_object, o);
   img=(struct image*)(o->storage);
   *img=*THIS;

   if (args)
   {
      if(sp[-args].u.integer < 0 ||
	 sp[1-args].u.integer < 0)
	 Pike_error("Illegal size to Image.Image->clone()\n");
      img->xsize=sp[-args].u.integer;
      img->ysize=sp[1-args].u.integer;
   }

   getrgb(img,2,args,args,"Image.Image->clone()"); 

   if (img->xsize<0) img->xsize=1;
   if (img->ysize<0) img->ysize=1;

   img->img=xalloc(sizeof(rgb_group)*img->xsize*img->ysize +1);
   if (THIS->img)
   {
      if (img->xsize==THIS->xsize
	  && img->ysize==THIS->ysize)
	 MEMCPY(img->img,THIS->img,sizeof(rgb_group)*img->xsize*img->ysize);
      else
	 img_crop(img,THIS,
		  0,0,img->xsize-1,img->ysize-1);
	 
   }
   else
      img_clear(img->img,img->rgb,img->xsize*img->ysize);

   UNSET_ONERROR(err);
   pop_n_elems(args);
   push_object(o);
}


/*
**! method void clear()
**! method void clear(int r,int g,int b)
**! method void clear(int r,int g,int b,int alpha)
**! 	gives a new, cleared image with the same size of drawing area
**!
**!	<table><tr valign=center>
**!	<td><illustration> return lena(); </illustration></td>
**!	<td><illustration> return lena()->clear(0,128,255); </illustration></td>
**!	</tr><tr>
**!	<td>original</td>
**!	<td>->clear<wbr>(0,128,255)</td>
**!	</tr></table>
**!
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

   getrgb(img,0,args,args,"Image.Image->clear()"); 

   img->img=malloc(sizeof(rgb_group)*img->xsize*img->ysize +1);
   if (!img->img)
   {
     free_object(o);
     SIMPLE_OUT_OF_MEMORY_ERROR("clear",0);
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
**!	<table><tr valign=center>
**!	<td><illustration> return lena(); </illustration></td>
**!	<td><illustration> return lena()->copy(5,5,lena()->xsize()-6,lena()->ysize()-6); </illustration></td>
**!	<td><illustration> return lena()->copy(-5,-5,lena()->xsize()+4,lena()->ysize()+4,10,75,10); </illustration></td>
**!	</tr><tr>
**!	<td>original</td>
**!	<td>->copy<wbr>(5,5,<wbr>XSIZE-6,YSIZE-6)</td>
**!	<td>->copy<wbr>(-5,-5,<wbr>XSIZE+4,YSIZE+4,<wbr>10,75,10)</td>
**!	</tr></table>
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
      bad_arg_error("Image",sp-args,args,0,"",sp-args,
		"Bad arguments to Image()\n");

   if (!THIS->img) Pike_error("Called Image.Image object is not initialized\n");;

   getrgb(THIS,4,args,args,"Image.Image->copy()"); 

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
**! method object change_color(int fromr,int fromg,int fromb,int tor,int tog,int tob)
**! 	Changes one color (exact match) to another.
**!	If non-exact-match is preferred, check <ref>distancesq</ref>
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
   int arg;

   if (!THIS->img) Pike_error("Called Image.Image object is not initialized\n");;

   to=THIS->rgb;   
   if (!(arg=getrgb(THIS,0,args,3,"Image.Image->change_color()")))
      SIMPLE_TOO_FEW_ARGS_ERROR("Image",1);
   from=THIS->rgb;
   if (getrgb(THIS,arg,args,args,"Image.Image->change_color()"))
      to=THIS->rgb;
   
   o=clone_object(image_program,0);
   img=(struct image*)(o->storage);
   *img=*THIS;

   if (!(img->img=malloc(sizeof(rgb_group)*img->xsize*img->ysize +1)))
   {
      free_object(o);
      SIMPLE_OUT_OF_MEMORY_ERROR("change_color",0);
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

/*
**! method object autocrop()
**! method object autocrop(int border)
**! method object autocrop(int border,Color color)
**! method object autocrop(int border,int left,int right,int top,int bottom)
**! method object autocrop(int border,int left,int right,int top,int bottom,Color color)
**! method array(int) find_autocrop()
**! method array(int) find_autocrop(int border)
**! method array(int) find_autocrop(int border,int left,int right,int top,int bottom)
**! 	Removes "unneccesary" borders around the image, adds one of
**!	its own if wanted to, in selected directions.
**!
**!	"Unneccesary" is all pixels that are equal -- ie if all the same pixels
**!	to the left are the same color, that column of pixels are removed.
**!
**!	The find_autocrop() function simply returns x1,y1,x2,y2 for the
**!	kept area. (This can be used with <ref>copy</ref> later.)
**!
**! returns the new image object
**!
**! arg int border
**!     added border size in pixels
**! arg int left
**! arg int right
**! arg int top
**! arg int bottom
**!	which borders to scan and cut the image; 
**!	a typical example is removing the top and bottom unneccesary
**!	pixels:
**!	<pre>img=img->autocrop(0, 0,0,1,1);</pre>
**!
**! see also: copy
*/

static INLINE int try_autocrop_vertical(struct image *this,
					INT32 x,INT32 y,INT32 y2,
					INT32 rgb_set,rgb_group *rgb)
{
   if (!rgb_set) *rgb=pixel(THIS,x,y);
   for (;y<=y2; y++)
      if (pixel(this,x,y).r!=rgb->r ||
	  pixel(this,x,y).g!=rgb->g ||
	  pixel(this,x,y).b!=rgb->b) return 0;
   return 1;
}

static INLINE int try_autocrop_horisontal(struct image *this,
					  INT32 y,INT32 x,INT32 x2,
					  INT32 rgb_set,rgb_group *rgb)
{
   if (!rgb_set) *rgb=pixel(THIS,x,y);
   for (;x<=x2; x++)
      if (pixel(this,x,y).r!=rgb->r ||
	  pixel(this,x,y).g!=rgb->g ||
	  pixel(this,x,y).b!=rgb->b) return 0;
   return 1;
}

void img_find_autocrop(struct image *this,
		       INT32 *px1,INT32 *py1,INT32 *px2,INT32 *py2,
		       int border,
		       int left,int right,
		       int top,int bottom,
		       int rgb_set,
		       rgb_group rgb)
{
   int done;
   INT32 x1=0,y1=0,x2=this->xsize-1,y2=this->ysize-1;

   while (x2>x1 && y2>y1)
   {
      done=0;
      if (left &&
	  try_autocrop_vertical(this,x1,y1,y2,rgb_set,&rgb)) 
	 x1++,done=rgb_set=1;
      if (right &&
	  x2>x1 && 
	  try_autocrop_vertical(this,x2,y1,y2,rgb_set,&rgb)) 
	 x2--,done=rgb_set=1;
      if (top &&
	  try_autocrop_horisontal(this,y1,x1,x2,rgb_set,&rgb)) 
	 y1++,done=rgb_set=1;
      if (bottom &&
	  y2>y1 && 
	  try_autocrop_horisontal(this,y2,x1,x2,rgb_set,&rgb)) 
	 y2--,done=rgb_set=1;
      if (!done) break;
   }

   x2+=border;
   y2+=border;
   x1-=border;
   y1-=border;

   if (x2<x1||y2<y1) px1[0]=py1[0]=0,px2[0]=py2[0]=-1;
   else px1[0]=x1,py1[0]=y1,px2[0]=x2,py2[0]=y2;
}

static void image_find_autocrop(INT32 args)
{
   INT32 border=0,x1,y1,x2,y2;
   rgb_group rgb={0,0,0};
   int left=1,right=1,top=1,bottom=1;

   if (args) {
      if (sp[-args].type!=T_INT)
         bad_arg_error("find_autocrop",sp-args,args,0,"",sp-args,
		"Bad arguments to find_autocrop()\n");
      else
         border=sp[-args].u.integer; 
   }

   if (args>=5)
   {
      left=!(sp[1-args].type==T_INT && sp[1-args].u.integer==0);
      right=!(sp[2-args].type==T_INT && sp[2-args].u.integer==0);
      top=!(sp[3-args].type==T_INT && sp[3-args].u.integer==0);
      bottom=!(sp[4-args].type==T_INT && sp[4-args].u.integer==0);
   }

   if (!THIS->img)
      Pike_error("Called Image.Image object is not initialized\n");;

   img_find_autocrop(THIS,&x1,&y1,&x2,&y2,
		     border,left,right,top,bottom,0,rgb);


   pop_n_elems(args);
   push_int(x1);
   push_int(y1);
   push_int(x2);
   push_int(y2);
   f_aggregate(4);
}

void image_autocrop(INT32 args)
{
   INT32 border=0,x1,y1,x2,y2;
   int rgb_set=0;
   struct object *o;
   struct image *img;
   int left=1,right=1,top=1,bottom=1;

   if (args>=5)
      getrgb(THIS,5,args,args,"Image.Image->autocrop()"); 
   else 
      getrgb(THIS,1,args,args,"Image.Image->autocrop()"); 

   image_find_autocrop(args);
   args++;
   
   x1=sp[-1].u.array->item[0].u.integer;
   y1=sp[-1].u.array->item[1].u.integer;
   x2=sp[-1].u.array->item[2].u.integer;
   y2=sp[-1].u.array->item[3].u.integer;

   push_object(o=clone_object(image_program,0));
   img=(struct image*)(o->storage);

   if (x2==-1 && y2==-1 && x1==0 && y1==0) /* magic, equal image */
      img_crop(img,THIS,0,0,0,0);
   else
      img_crop(img,THIS,x1,y1,x2,y2);
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
      bad_arg_error("Image.Image->setcolor",sp-args,args,0,"",sp-args,
		"Bad arguments to Image.Image->setcolor()\n");
   getrgb(THIS,0,args,args,"Image.Image->setcolor()");
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*
**! method object setpixel(int x,int y)
**! method object setpixel(int x,int y,Image.Color c)
**! method object setpixel(int x,int y,int r,int g,int b)
**! method object setpixel(int x,int y,int r,int g,int b,int alpha)
**!    
**!	<table><tr valign=center>
**!	<td><illustration> return lena(); </illustration></td>
**!	<td><illustration> return lena()->copy()->setpixel(10,10,255,0,0); </illustration></td>
**!	</tr><tr>
**!	<td>original</td>
**!	<td>->setpixel<wbr>(10,10,<wbr>255,0,0)</td>
**!	</tr></table>
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
      bad_arg_error("setpixel",sp-args,args,0,"",sp-args,
		"Bad arguments to setpixel()\n");
   getrgb(THIS,2,args,args,"Image.Image->setpixel()");   
   if (!THIS->img) return;
   x=sp[-args].u.integer;
   y=sp[1-args].u.integer;
   if (!THIS->img) return;
   setpixel_test(x,y);
   pop_n_elems(args);
   ref_push_object(THISOBJ);
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
      bad_arg_error("Image",sp-args,args,0,"",sp-args,
		"Bad arguments to Image()\n");

   if (!THIS->img) Pike_error("Called Image.Image object is not initialized\n");;

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
**!	<table><tr valign=center>
**!	<td><illustration> return lena(); </illustration></td>
**!	<td><illustration> return lena()->copy()->line(50,10,10,50,255,0,0); </illustration></td>
**!	</tr><tr>
**!	<td>original</td>
**!	<td>->line<wbr>(50,10,<wbr>10,50,<wbr>255,0,0)</td>
**!	</tr></table>
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
      bad_arg_error("Image",sp-args,args,0,"",sp-args,
		"Bad arguments to Image()\n");
   getrgb(THIS,4,args,args,"Image.Image->line()");
   if (!THIS->img) return;

   img_line(sp[-args].u.integer,
	    sp[1-args].u.integer,
	    sp[2-args].u.integer,
	    sp[3-args].u.integer);
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*
**! method object box(int x1,int y1,int x2,int y2)
**! method object box(int x1,int y1,int x2,int y2,int r,int g,int b)
**! method object box(int x1,int y1,int x2,int y2,int r,int g,int b,int alpha)
**! 	Draws a filled rectangle on the image. 
**! 
**!	<table><tr valign=center>
**!	<td><illustration> return lena(); </illustration></td>
**!	<td><illustration> return lena()->copy()->box(40,10,10,80,0,255,0); </illustration></td>
**!	<td><illustration> return lena()->copy()->box(40,10,10,80,255,0,0,75); </illustration></td>
**!	</tr><tr>
**!	<td>original</td>
**!	<td>->box<wbr>(40,10,<wbr>10,80,<wbr>0,255,0)</td>
**!	<td>->box<wbr>(40,10,<wbr>10,80,<wbr>255,0,0,75)</td>
**!	</tr></table>
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
      bad_arg_error("Image",sp-args,args,0,"",sp-args,
		"Bad arguments to Image()\n");
   getrgb(THIS,4,args,args,"Image.Image->box()");
   if (!THIS->img) return;

   img_box(sp[-args].u.integer,
	   sp[1-args].u.integer,
	   sp[2-args].u.integer,
	   sp[3-args].u.integer);
   ref_push_object(THISOBJ);
   stack_pop_n_elems_keep_top(args);
}

/*
**! method object circle(int x,int y,int rx,int ry)
**! method object circle(int x,int y,int rx,int ry,int r,int g,int b)
**! method object circle(int x,int y,int rx,int ry,int r,int g,int b,int alpha)
**! 	Draws a circle on the image. The circle is <i>not</i> antialiased.
**! 
**!	<table><tr valign=center>
**!	<td><illustration> return lena(); </illustration></td>
**!	<td><illustration> return lena()->copy()->circle(50,50,30,50,255,255); </illustration></td>
**!	</tr><tr>
**!	<td>original</td>
**!	<td>->circle<wbr>(50,50,<wbr>30,50,<wbr>0,255,255)</td>
**!	</tr></table>
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
      bad_arg_error("Image",sp-args,args,0,"",sp-args,
		"Bad arguments to Image()\n");
   getrgb(THIS,4,args,args,"Image.Image->circle()");
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
   ref_push_object(THISOBJ);
}

static INLINE int image_color_svalue_rgba(struct svalue *s,
					   rgba_group *d)
{
   rgb_group rgb;
   if (s->type==T_ARRAY && s->u.array->size>=4)
   {
     struct array *a = s->u.array;
     if( (a->type_field!=BIT_INT) &&
	 (array_fix_type_field(a)!=BIT_INT) )
       return 0;

     d->r=(COLORTYPE)a->item[0].u.integer;
     d->g=(COLORTYPE)a->item[1].u.integer;
     d->b=(COLORTYPE)a->item[2].u.integer;
     d->alpha=(COLORTYPE)a->item[3].u.integer;
     return 1;
   }
   else if (image_color_svalue(s,&rgb))
   {
      d->r=rgb.r;
      d->g=rgb.g;
      d->b=rgb.b;
      d->alpha=0;
      return 1;
   }
   return 0; 
}

static INLINE void
   add_to_rgbda_sum_with_factor(rgbda_group *sum,
				rgba_group rgba,
				double factor)
{
  /* NOTE:
   *	This code MUST be MT-SAFE! (but also fast /per)
   */
/*   HIDE_GLOBAL_VARIABLES(); */
   sum->r = DO_NOT_WARN((float)(sum->r + rgba.r*factor));
   sum->g = DO_NOT_WARN((float)(sum->g + rgba.g*factor));
   sum->b = DO_NOT_WARN((float)(sum->b + rgba.b*factor));
   sum->alpha = DO_NOT_WARN((float)(sum->alpha + rgba.alpha*factor));
/*    REVEAL_GLOBAL_VARIABLES(); */
}

static INLINE void
   add_to_rgbd_sum_with_factor(rgbd_group *sum,
			       rgba_group rgba,
			       double factor)
{
  /* NOTE:
   *	This code MUST be MT-SAFE! (but also fast /per)
   */
/*   HIDE_GLOBAL_VARIABLES(); */
   sum->r = DO_NOT_WARN((float)(sum->r+rgba.r*factor));
   sum->g = DO_NOT_WARN((float)(sum->g+rgba.g*factor));
   sum->b = DO_NOT_WARN((float)(sum->b+rgba.b*factor));
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
**!	<td><illustration> return lena()->copy()->tuned_box(30,10,lena()->xsize()-20,lena()->ysize()-20,({({255,0,0}),({0,255,0}),({0,0,255}),({255,255,0})})); </illustration></td>
**!	<td><illustration> return lena()->copy()->tuned_box(0,0,lena()->xsize(),lena()->ysize(),({({255,0,0}),({0,255,0}),({0,0,255}),({255,255,0})})); </illustration></td>
**!	<td><illustration> return lena()->copy()->tuned_box(0,0,lena()->xsize(),lena()->ysize(),({({255,0,0,255}),({0,255,0,128}),({0,0,255,128}),({255,255,0})})); </illustration></td>
**!	</tr><tr valign=center>
**!	<td>original</td>
**!	<td>tuned box</td>
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
    (dest+x)->r = DO_NOT_WARN((COLORTYPE)((((long)left.r)*(length-x)+((long)right.r)*(x))/length));
    (dest+x)->g = DO_NOT_WARN((COLORTYPE)((((long)left.g)*(length-x)+((long)right.g)*(x))/length));
    (dest+x)->b = DO_NOT_WARN((COLORTYPE)((((long)left.b)*(length-x)+((long)right.b)*(x))/length));
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
      color.r = DO_NOT_WARN((COLORTYPE)((((long)left.r)*(height-y)+((long)right.r)*(y))/height));
      color.g = DO_NOT_WARN((COLORTYPE)((((long)left.g)*(height-y)+((long)right.g)*(y))/height));
      color.b = DO_NOT_WARN((COLORTYPE)((((long)left.b)*(height-y)+((long)right.b)*(y))/height));
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
      color.r = DO_NOT_WARN((COLORTYPE)((((long)left.r)*(height-y)+((long)right.r)*(y))/height));
      color.g = DO_NOT_WARN((COLORTYPE)((((long)left.g)*(height-y)+((long)right.g)*(y))/height));
      color.b = DO_NOT_WARN((COLORTYPE)((((long)left.b)*(height-y)+((long)right.b)*(y))/height));
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

static void image_tuned_box(INT32 args)
{
  INT32 x1,y1,x2,y2,xw,yw,x,y;
  rgba_group topleft,topright,bottomleft,bottomright,sum;
  rgb_group *img;
  INT32 ymax;
  struct image *this;
  double dxw, dyw;

  if (args<5||
      sp[-args].type!=T_INT||
      sp[1-args].type!=T_INT||
      sp[2-args].type!=T_INT||
      sp[3-args].type!=T_INT)
     SIMPLE_TOO_FEW_ARGS_ERROR("Image.Image->tuned_box",5);

  if (!THIS->img)
    Pike_error("Called Image.Image object is not initialized\n");;

  x1=sp[-args].u.integer;
  y1=sp[1-args].u.integer;
  x2=sp[2-args].u.integer;
  y2=sp[3-args].u.integer;

  if (sp[4-args].type==T_ARRAY)
  {
     if (args>5) pop_n_elems(args-5);
     args+=sp[-1].u.array->size-1;
     sp--;
     push_array_items(sp->u.array); /* frees */
  }

  if (args<8)
     SIMPLE_TOO_FEW_ARGS_ERROR("Image.Image->tuned_box",8);

  if (!image_color_svalue_rgba(sp-4,&topleft))
     SIMPLE_BAD_ARG_ERROR("Image.Image->tuned_box",5,"color");
  if (!image_color_svalue_rgba(sp-3,&topright))
     SIMPLE_BAD_ARG_ERROR("Image.Image->tuned_box",6,"color");
  if (!image_color_svalue_rgba(sp-2,&bottomleft))
     SIMPLE_BAD_ARG_ERROR("Image.Image->tuned_box",7,"color");
  if (!image_color_svalue_rgba(sp-1,&bottomright))
     SIMPLE_BAD_ARG_ERROR("Image.Image->tuned_box",8,"color");

  if (x1>x2) x1^=x2,x2^=x1,x1^=x2,
	       sum=topleft,topleft=topright,topright=sum,
	       sum=bottomleft,bottomleft=bottomright,bottomright=sum;
  if (y1>y2) y1^=y2,y2^=y1,y1^=y2,
	       sum=topleft,topleft=bottomleft,bottomleft=sum,
	       sum=topright,topright=bottomright,bottomright=sum;

  pop_n_elems(args);
  ref_push_object(THISOBJ);

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
      dxw = 1.0/(double)xw;
      dyw = 1.0/(double)yw;
      ymax=MINIMUM(yw,this->ysize-y1-1);
      for (x=MAXIMUM(0,-x1); x<=xw && x+x1<this->xsize; x++)
	{
#define tune_factor(a,aw) (1.0-((double)(a)*(aw)))
	  double tfx1 = tune_factor(x,dxw);
	  double tfx2 = tune_factor(xw-x,dxw);

	  img=this->img+x+x1+this->xsize*MAXIMUM(0,y1);
	  if (topleft.alpha||topright.alpha||bottomleft.alpha||bottomright.alpha)
	    for (y=MAXIMUM(0,-y1); y<=ymax; y++)
	      {
		double tfy;
		rgbda_group sum={0.0,0.0,0.0,0.0};
		rgbd_group rgb;

		add_to_rgbda_sum_with_factor(&sum, topleft,
					     (tfy=tune_factor(y,dyw))*tfx1);
		add_to_rgbda_sum_with_factor(&sum, topright, tfy*tfx2);
		add_to_rgbda_sum_with_factor(&sum, bottomleft,
					     (tfy=tune_factor(yw-y,dyw))*tfx1);
		add_to_rgbda_sum_with_factor(&sum, bottomright, tfy*tfx2);

		sum.alpha *= DO_NOT_WARN((float)(1.0/255.0));

		rgb.r = DO_NOT_WARN((float)(sum.r*(1.0-sum.alpha)+img->r*sum.alpha));
		rgb.g = DO_NOT_WARN((float)(sum.g*(1.0-sum.alpha)+img->g*sum.alpha));
		rgb.b = DO_NOT_WARN((float)(sum.b*(1.0-sum.alpha)+img->b*sum.alpha));

		img->r = testrange(rgb.r+0.5);
		img->g = testrange(rgb.g+0.5);
		img->b = testrange(rgb.b+0.5);

		img += this->xsize;
	      }
	  else
	    for (y=MAXIMUM(0,-y1); y<=ymax; y++)
	      {
		double tfy;
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
**!
**!	<table><tr valign=center>
**!	<td><illustration> return lena(); </illustration></td>
**!	<td><illustration> return lena()->gradients(
**!              ({random(lena()->xsize()),random(lena()->ysize()),255,0,0}),
**!              ({random(lena()->xsize()),random(lena()->ysize()),255,255,0})
**!              )</illustration></td>
**!	<td><illustration> return lena()->gradients(
**!              ({random(lena()->xsize()),random(lena()->ysize()),255,0,0}),
**!              ({random(lena()->xsize()),random(lena()->ysize()),255,255,0}),
**!              ({random(lena()->xsize()),random(lena()->ysize()),255,0,255}),
**!              ({random(lena()->xsize()),random(lena()->ysize()),0,0,0}),
**!              ({random(lena()->xsize()),random(lena()->ysize()),128,128,255}),
**!              ({random(lena()->xsize()),random(lena()->ysize()),0,128,255}),
**!              ({random(lena()->xsize()),random(lena()->ysize()),0,0,255}),
**!              ({random(lena()->xsize()),random(lena()->ysize()),255,128,0}),
**!              ({random(lena()->xsize()),random(lena()->ysize()),255,255,255}),
**!              ({random(lena()->xsize()),random(lena()->ysize()),0,255,0})
**!              )</illustration></td>
**!	<td><illustration> return lena()->gradients(
**!              ({(int)(0.2*lena()->xsize()),(int)(0.8*lena()->ysize()),255,0,0}),
**!              ({(int)(0.5*lena()->xsize()),(int)(-0.2*lena()->ysize()),16,16,64}),
**!              ({(int)(0.7*lena()->xsize()),(int)(0.6*lena()->ysize()),255,255,0}),
**!              4.0)</illustration></td>
**!	<td><illustration> return lena()->gradients(
**!              ({(int)(0.2*lena()->xsize()),(int)(0.8*lena()->ysize()),255,0,0}),
**!              ({(int)(0.5*lena()->xsize()),(int)(-0.2*lena()->ysize()),16,16,64}),
**!              ({(int)(0.7*lena()->xsize()),(int)(0.6*lena()->ysize()),255,255,0}),
**!              1.0)</illustration></td>
**!	<td><illustration> return lena()->gradients(
**!              ({(int)(0.2*lena()->xsize()),(int)(0.8*lena()->ysize()),255,0,0}),
**!              ({(int)(0.5*lena()->xsize()),(int)(-0.2*lena()->ysize()),16,16,64}),
**!              ({(int)(0.7*lena()->xsize()),(int)(0.6*lena()->ysize()),255,255,0}),
**!              0.25)</illustration></td>
**!	</tr><tr>
**!	<td>original</td>
**!	<td>2 color<br>gradient</td>
**!	<td>10 color<br>gradient</td>
**!	<td>3 colors,<br>grad=4.0</td>
**!	<td>3 colors,<br>grad=1.0</td>
**!	<td>3 colors,<br>grad=0.25</td>
**!	</tr></table>
**!
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
      struct array *a = NULL;
      if (sp[-1].type!=T_ARRAY ||
	  (a=sp[-1].u.array)->size!=5 ||
	  ( (a->type_field & ~BIT_INT) &&
	    (array_fix_type_field(a) & ~BIT_INT) ))
      {
	 while (first) { c=first; first=c->next; free(c); }
	 bad_arg_error("Image.Image->gradients",sp-args,args,0,"",sp-args,
		"Bad arguments to Image.Image->gradients()\n");
      }
      c=malloc(sizeof(struct gr_point));
      if (!c)
      {
	 while (first) { c=first; first=c->next; free(c); }
	 SIMPLE_OUT_OF_MEMORY_ERROR("gradients", sizeof(struct gr_point));
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
      SIMPLE_TOO_FEW_ARGS_ERROR("Image.Image->gradients",1);

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

	 if (grad==0.0)
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
	 else if (grad==2.0)
	    while (c)
	    {
	       c->xd++;
	       di=(c->xd*c->xd)+(c->yd*c->yd);
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
	       di=pow((c->xd*c->xd)+(c->yd*c->yd),0.5*grad);
	       if (!di) di=1e20; else di=1.0/di;
	       r+=c->r*di;
	       g+=c->g*di;
	       b+=c->b*di;
	       z+=di;

	       c=c->next;
	    }

	 z=1.0/z;

	 d->r=DOUBLE_TO_COLORTYPE(r*z);
	 d->g=DOUBLE_TO_COLORTYPE(g*z);
	 d->b=DOUBLE_TO_COLORTYPE(b*z);
	 d++;
      }
   }

   while (first) { c=first; first=c->next; free(c); }

   THREADS_DISALLOW();

   push_object(o);
}

/*
**! method object test()
**! method object test(int seed)
**!    	Generates a test image, currently random gradients.
**!
**!	<table><tr valign=center>
**!	<td><illustration> return lena(); </illustration></td>
**!	<td><illustration> return lena()->test()</illustration></td>
**!	<td><illustration> return lena()->test()</illustration></td>
**!	</tr><tr>
**!	<td>original</td>
**!	<td>->test()</td>
**!	<td>...and again</td>
**!	</tr></table>
**!
**! returns the new image
**! note
**!    May be subject to change or cease without prior warning.
**!
**! see also: gradients, tuned_box
*/

static void image_test(INT32 args)
{
   int i;

   if (args) f_random_seed(args);

   for (i=0; i<5; i++)
   {
      push_int(THIS->xsize); f_random(1); 
      push_int(THIS->ysize); f_random(1);
      push_int((i!=0)?255:0); f_random(1);
      push_int((i!=1)?255:0); if (i!=4) f_random(1);
      push_int((i!=2)?255:0); if (i!=3) f_random(1);
      f_aggregate(5);
   }
   push_float(2.0);
   image_gradients(6);
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
      getrgbl(&rgb,0,args,"Image.Image->grey()");
   div=rgb.r+rgb.g+rgb.b;

   o=clone_object(image_program,0);
   img=(struct image*)o->storage;
   *img=*THIS;
   if (!(img->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize+1)))
   {
      free_object(o);
      SIMPLE_OUT_OF_MEMORY_ERROR("grey",
				 sizeof(rgb_group)*THIS->xsize*THIS->ysize+1);
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

   if (!THIS->img) Pike_error("Called Image.Image object is not initialized\n");;
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
      getrgbl(&rgb,0,args,"Image.Image->color()");

   o=clone_object(image_program,0);
   img=(struct image*)o->storage;
   *img=*THIS;
   if (!(img->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize+1)))
   {
      free_object(o);
      SIMPLE_OUT_OF_MEMORY_ERROR("color",
				 sizeof(rgb_group)*THIS->xsize*THIS->ysize+1);
   }

   d=img->img;
   s=THIS->img;

   x=THIS->xsize*THIS->ysize;

   THREADS_ALLOW();
#if 0
#ifdef ASSEMBLY_OK
   if( (image_cpuid & IMAGE_MMX) && x>>2 )
   {
     image_mult_buffer_mmx_x86asm( d,s,x>>2, RGB2ASMCOL( rgb ) ); 
     s += x; x = x%4; s -= x;
   }
#endif
#endif
   while (x--)
   {
      d->r = DO_NOT_WARN((COLORTYPE)( (((long)rgb.r*s->r)/255) ));
      d->g = DO_NOT_WARN((COLORTYPE)( (((long)rgb.g*s->g)/255) ));
      d->b = DO_NOT_WARN((COLORTYPE)( (((long)rgb.b*s->b)/255) ));
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

   if (!THIS->img) Pike_error("Called Image.Image object is not initialized\n");;

   o=clone_object(image_program,0);
   img=(struct image*)o->storage;
   *img=*THIS;
   if (!(img->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize+1)))
   {
      free_object(o);
      SIMPLE_OUT_OF_MEMORY_ERROR("invert",
				 sizeof(rgb_group)*THIS->xsize*THIS->ysize+1);
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
**! method object threshold(int level)
**! method object threshold(int r,int g,int b)
**! method object threshold(Color color)
**! 	Makes a black-white image. 
**!
**! 	If any of red, green, blue parts of a pixel
**!    	is larger then the given value, the pixel will become
**!	white, else black.
**!
**!	This method works fine with the grey method.
**!
**!	If no arguments are given, it will paint all non-black
**!	pixels white. (Ie, default is 0,0,0.)
**!
**!	<table><tr valign=center>
**!	<td><illustration> return lena(); </illustration></td>
**!	<td><illustration> return lena()->threshold(100); </illustration></td>
**!	<td><illustration> return lena()->threshold(90,100,110); </illustration></td>
**!	</tr><tr valign=center>
**!	<td>original</td>
**!	<td>->threshold(100);</td>
**!	<td>->threshold(0,100,0);</td>
**!	</tr></table>
**!
**! returns the new image object
**!
**! see also: grey
**!
**! note
**!	The above statement "any ..." was changed from "all ..."
**!	in Pike 0.7 (9906). It also uses 0,0,0 as default input,
**!	instead of current color. This is more useful.
*/


void image_threshold(INT32 args)
{
   INT_TYPE x;
   rgb_group *s,*d,rgb;
   struct object *o;
   struct image *img;
   INT_TYPE level=-1;

   if (!THIS->img) Pike_error("Called Image.Image object is not initialized\n");;

   if (args==1)
      get_all_args("threshold",args,"%i",&level),level*=3;
   else if (!getrgb(THIS,0,args,args,"Image.Image->threshold()"))
      rgb.r=rgb.g=rgb.b=0;
   else
      rgb=THIS->rgb;

   o=clone_object(image_program,0);
   img=(struct image*)o->storage;
   *img=*THIS;
   if (!(img->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize+1)))
   {
      free_object(o);
      SIMPLE_OUT_OF_MEMORY_ERROR("threshold",
				 sizeof(rgb_group)*THIS->xsize*THIS->ysize+1);
   }

   d=img->img;
   s=THIS->img;

   x=THIS->xsize*THIS->ysize;
   THREADS_ALLOW();
   if (level==-1)
      while (x--)
      {
	 if (s->r>rgb.r ||
	     s->g>rgb.g ||
	     s->b>rgb.b)
	    d->r=d->g=d->b=255;
	 else
	    d->r=d->g=d->b=0;

	 d++;
	 s++;
      }
   else
      while (x--)
      {
	 if (s->r+s->g+s->b>level)
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
**!     return Image.image(67,67)->tuned_box(0,0, 67,67,
**!                      ({ ({ 255,255,128 }), ({ 0,255,128 }),
**!                         ({ 255,255,255 }), ({ 0,255,255 })}));
**!	</illustration></td>
**!	<td><illustration>
**!     return Image.image(67,67)->tuned_box(0,0, 67,67,
**!                      ({ ({ 255,255,128 }), ({ 0,255,128 }),
**!                         ({ 255,255,255 }), ({ 0,255,255 })}))
**!          ->hsv_to_rgb();
**!	</illustration></td>
**!	<td><illustration>
**!     return Image.image(67,67)->tuned_box(0,0, 67,67,
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
**!     object i = Image.Image(200,200);
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
   if (!THIS->img) Pike_error("Called Image.Image object is not initialized\n");;

   o=clone_object(image_program,0);
   img=(struct image*)o->storage;
   *img=*THIS;

   if (!(img->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize+1)))
   {
      free_object(o);
      SIMPLE_OUT_OF_MEMORY_ERROR("hsv_to_rgb",
				 sizeof(rgb_group)*THIS->xsize*THIS->ysize+1);
   }

   d=img->img;
   s=THIS->img;

   THREADS_ALLOW();
   i=img->xsize*img->ysize;
   while (i--)
   {
     double h,sat,v;
     double r,g,b;
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
	switch(DOUBLE_TO_INT(i))
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
#define FIX(X) ((X)<0.0?0:(X)>=1.0?255:DOUBLE_TO_INT((X)*255.0))
     d->r = FIX(r);
     d->g = FIX(g);
     d->b = FIX(b);
     s++; d++;
   }
exit_loop:
   ;	/* Needed to keep some compilers happy. */
   THREADS_DISALLOW();

   if (err) {
     Pike_error("%s\n", err);
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
   if (!THIS->img)
     Pike_error("Called Image.Image object is not initialized\n");

   o=clone_object(image_program,0);
   img=(struct image*)o->storage;
   *img=*THIS;

   if (!(img->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize+1)))
   {
      free_object(o);
      SIMPLE_OUT_OF_MEMORY_ERROR("rgb_to_hsv",
				 sizeof(rgb_group)*THIS->xsize*THIS->ysize+1);
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

      if(r==v)      h = DOUBLE_TO_INT(((g-b)/(double)delta)*(255.0/6.0));
      else if(g==v) h = DOUBLE_TO_INT((2.0+(b-r)/(double)delta)*(255.0/6.0));
      else h = DOUBLE_TO_INT((4.0+(r-g)/(double)delta)*(255.0/6.0));
      if(h<0) h+=255;

      /*     printf("hsv={ %d,%d,%d }\n", h, (int)((delta/(float)v)*255), v);*/
      d->r = DOUBLE_TO_INT(h);
      d->g = DOUBLE_TO_INT((delta/(double)v)*255.0);
      d->b = v;
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
**!	<td><illustration> return lena()->distancesq(0,255,255); </illustration></td>
**!	<td><illustration> return lena()->distancesq(255,0,255); </illustration></td>
**!	<td><illustration> return lena()->distancesq(255,255,0); </illustration></td>
**!	</tr><tr valign=center>
**!	<td>original</td>
**!	<td>distance² to cyan</td>
**!	<td>...to purple</td>
**!	<td>...to yellow</td>
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

   if (!THIS->img)
     Pike_error("Called Image.Image object is not initialized\n");

   getrgb(THIS,0,args,args,"Image.Image->distancesq()");

   o=clone_object(image_program,0);
   img=(struct image*)o->storage;
   *img=*THIS;
   if (!(img->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize+1)))
   {
      free_object(o);
      SIMPLE_OUT_OF_MEMORY_ERROR("distancesq",
				 sizeof(rgb_group)*THIS->xsize*THIS->ysize+1);
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
**!	<table><tr valign=center>
**!	<td><illustration> return lena()->select_from(35,35,16); </illustration></td>
**!	<td><illustration> return lena()->select_from(35,35,32); </illustration></td>
**!	<td><illustration> return lena()->select_from(35,35,64); </illustration></td>
**!	<td><illustration> return lena()->select_from(35,35,96); </illustration></td>
**!	<td><illustration> return lena()->select_from(35,35,128); </illustration></td>
**!	<td><illustration> return lena()->select_from(35,35,192); </illustration></td>
**!	<td><illustration> return lena()->select_from(35,35,256); </illustration></td>
**!	</tr><tr>
**!	<td>35, 35, 16</td>
**!	<td>35, 35, 32</td>
**!	<td>35, 35, 64</td>
**!	<td>35, 35, 96</td>
**!	<td>35, 35, 128</td>
**!	<td>35, 35, 192</td>
**!	<td>35, 35, 256</td>
**!	</tr></table>
**!
**!	<table><tr valign=center>
**!	<td><illustration> return lena(); </illustration></td>
**!	<td><illustration> return lena()*lena()->select_from(35,35,200); </illustration></td>
**!	</tr><tr>
**!	<td>original</td>
**!	<td>original * select_from(35,35,200)</td>
**!	</tr></table>
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

   if (!THIS->img)
     Pike_error("Called Image.Image object is not initialized\n");

   if (args<2 
       || sp[-args].type!=T_INT
       || sp[1-args].type!=T_INT)
      bad_arg_error("Image",sp-args,args,0,"",sp-args,
		"Bad arguments to Image()\n");

   if (args>=3) 
      if (sp[2-args].type!=T_INT)
	 bad_arg_error("Image",sp-args,args,3,"",sp+3-1-args,
		"Bad argument 3 (edge value) to Image()\n");
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
      SIMPLE_OUT_OF_MEMORY_ERROR("select_from",
				 sizeof(rgb_group)*THIS->xsize*THIS->ysize+1);
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
**! method object bitscale(float factor)
**! method object bitscale(float xfactor,float yfactor)
**!	scales the image with a factor, without smoothing.
**!     This routine is faster than scale, but gives less correct
**!     results
**!
**!	<table><tr valign=center>
**!	<td><illustration> return lena(); </illustration></td>
**!	<td><illustration> return lena()->bitscale(0.75); </illustration></td>
**!	<td><illustration> return lena()->scale(0.75); </illustration></td>
**!	</tr><tr>
**!	<td>original</td>
**!	<td>bitscale(0.75)</td>
**!	<td>scale(0.75)</td>
**!	</tr></table>
**!
**! returns the new image object
**! arg float factor
**!	factor to use for both x and y
**! arg float xfactor
**! arg float yfactor
**!	separate factors for x and y
**!
**! method object bitscale(int newxsize,int newysize)
**! method object bitscale(0,int newysize)
**! method object bitscale(int newxsize,0)
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
void image_bitscale( INT32 args )
{
  int newx=1, newy=1;
  int oldx, oldy;
  int x, y;
  struct object *ro;
  rgb_group *s, *d;
  oldx = THIS->xsize;
  oldy = THIS->ysize;

  if( args == 1 )
  {
    if( sp[-1].type == T_INT )
    {
      newx = oldx * sp[-1].u.integer;
      newy = oldy * sp[-1].u.integer;
    } else if( sp[-1].type == T_FLOAT ) {
      newx = DOUBLE_TO_INT(oldx * sp[-1].u.float_number);
      newy = DOUBLE_TO_INT(oldy * sp[-1].u.float_number);
    } else 
      Pike_error("The scale factor must be an integer less than 2^32, or a float\n");
  } else if( args == 2 ) {
    if( sp[-1].type != sp[-2].type )
      Pike_error("Wrong type of argument\n");
    if( sp[-2].type  == T_INT )
      (newx = sp[-2].u.integer),(newy = sp[-1].u.integer);
    else if( sp[-2].type  == T_FLOAT )
    {
      newx = DOUBLE_TO_INT(oldx*sp[-2].u.float_number);
      newy = DOUBLE_TO_INT(oldy*sp[-1].u.float_number);

    } else
      Pike_error( "Wrong type of arguments\n");
  }

  /* Not really nessesary if I use floats below.. */
  /* FIXME */
  if( newx > 65536 || newy > 65536 || oldx > 65536 || oldy > 65536)
    Pike_error("Image too big.\n");

  if( newx < 1 ) newx = 1;
  if( newy < 1 ) newy = 1;

  pop_n_elems( args );
  push_int( newx );
  push_int( newy );
  ro = clone_object( image_program, 2 );
  d=((struct image *)get_storage( ro, image_program))->img;
  
  for( y = 0; y<newy; y++ )
  {
    s = THIS->img + (y * oldy / newy) * THIS->xsize;
    for( x = 0; x<newx; x++ )
      *(d++) = *(s+x*oldx/newx);
  }
  push_object( ro );
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
   ONERROR o_err, matrix_err;

CHRONO("apply_matrix");

   if (args<1 ||
       sp[-args].type!=T_ARRAY)
      bad_arg_error("Image",sp-args,args,0,"",sp-args,
		"Bad arguments to Image()\n");

   if (args>3) 
      if (sp[1-args].type!=T_INT ||
	  sp[2-args].type!=T_INT ||
	  sp[3-args].type!=T_INT)
	 Pike_error("Illegal argument(s) (2,3,4) to Image.Image->apply_matrix()\n");
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
	 Pike_error("Illegal contents of (root) array (Image.Image->apply_matrix)\n");
      if (width==-1)
	 width=s.u.array->size;
      else
	 if (width!=s.u.array->size)
	    Pike_error("Arrays has different size (Image.Image->apply_matrix)\n");
   }
   if (width==-1) width=0;

   matrix=xalloc(sizeof(rgbd_group)*width*height+1);

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
	    if (s3.type==T_INT) matrix[j+i*width].r = (float)s3.u.integer; 
	    else matrix[j+i*width].r=0;

	    s3=s2.u.array->item[1];
	    if (s3.type==T_INT) matrix[j+i*width].g = (float)s3.u.integer;
	    else matrix[j+i*width].g=0;

	    s3=s2.u.array->item[2];
	    if (s3.type==T_INT) matrix[j+i*width].b = (float)s3.u.integer; 
	    else matrix[j+i*width].b=0;
	 }
	 else if (s2.type==T_INT)
	    matrix[j+i*width].r=matrix[j+i*width].g=
	       matrix[j+i*width].b = (float)s2.u.integer;
	 else
	    matrix[j+i*width].r=matrix[j+i*width].g=
	       matrix[j+i*width].b = 0.0;
      }
   }

   o=clone_object(image_program,0);

   SET_ONERROR(matrix_err, free, matrix);
   SET_ONERROR(o_err, my_free_object, o);

CHRONO("apply_matrix, begin");

   if (THIS->img)
      img_apply_matrix((struct image*)o->storage,THIS,
		       width,height,matrix,div,default_rgb);

CHRONO("apply_matrix, end");

   UNSET_ONERROR(o_err);
   UNSET_ONERROR(matrix_err);
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
**!	<table><tr valign=center>
**!	<td><illustration> return lena(); </illustration></td>
**!	<td><illustration> return lena()*lena()->threshold(100,100,100)</illustration></td>
**!	<td><illustration> return (lena()*lena()->threshold(100,100,100))->outline(255,0,0)</illustration></td>
**!	</tr><tr>
**!	<td>original</td>
**!	<td>masked<br>through<br>threshold</td>
**!	<td>...and<br>outlined<br>with red</td>
**!	</tr></table>
**! 
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
      Pike_error("Called Image.Image object is not initialized\n");;

   if (args && sp[-args].type==T_ARRAY)
   {
      int i,j;
      height=sp[-args].u.array->size;
      width=-1;
      for (i=0; i<height; i++)
      {
	 struct svalue s=sp[-args].u.array->item[i];
	 if (s.type!=T_ARRAY) 
	    Pike_error("Image.Image->outline: Illegal contents of (root) array\n");
	 if (width==-1)
	    width=s.u.array->size;
	 else
	    if (width!=s.u.array->size)
	       Pike_error("Image.Image->outline: Arrays has different size\n");
      }
      if (width==-1) width=0;

      matrix=xalloc(sizeof(int)*width*height+1);

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
   if (!tmp) {
     free_object(o);
     if (matrix!=defaultmatrix) free(matrix);
     SIMPLE_OUT_OF_MEMORY_ERROR("outline",
				(THIS->xsize+width)*(THIS->ysize+height));
   }
   MEMSET(tmp,0,(THIS->xsize+width)*(THIS->ysize+height));
 
   s=THIS->img;

   if (!mask)
   {
      if (args-ai==6)
      {
	 getrgbl(&bkgl,ai+3,args,"Image.Image->outline");
	 pop_n_elems(args-(ai+3));
	 args=ai+3;
      }
      else if (args-ai==7)
      {
	 getrgbl(&bkgl,ai+4,args,"Image.Image->outline");
	 pop_n_elems(args-(ai+4));
	 args=ai+4;
      }
      else
      {
	 bkgl.r=s->r;
	 bkgl.g=s->g;
	 bkgl.b=s->b;
      }
      getrgb(img,ai,args,args,"Image.Image->outline");
   }
   else
   {
      if (args-ai==4)
      {
	 getrgbl(&bkgl,ai,args,"Image.Image->outline_mask");
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
   free(tmp);

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
**! method object modify_by_intensity(int r,int g,int b,int|array(int) ... vn)
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

   if (!THIS->img)
     Pike_error("Called Image.Image object is not initialized\n");
   if (args<5) 
      SIMPLE_TOO_FEW_ARGS_ERROR("Image.Image->modify_by_intensity()",5);
   
   getrgbl(&rgb,0,args,"Image.Image->modify_by_intensity()");
   div=rgb.r+rgb.g+rgb.b;
   if (!div) div=1;

   s=xalloc(sizeof(rgb_group)*(args-3)+1);

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
      SIMPLE_OUT_OF_MEMORY_ERROR("modify_by_intensity",
				 sizeof(rgb_group)*256+1);
   }
   for (x=0; x<args-4; x++)
   {
      INT32 p1,p2,r;
      p1=(255L*x)/(args-4);
      p2=(255L*(x+1))/(args-4);
      r=p2-p1;
      for (y=0; y<r; y++)
      {
	 list[y+p1].r = DO_NOT_WARN((COLORTYPE)((((long)s[x].r)*(r-y)+((long)s[x+1].r)*(y))/r));
	 list[y+p1].g = DO_NOT_WARN((COLORTYPE)((((long)s[x].g)*(r-y)+((long)s[x+1].g)*(y))/r));
	 list[y+p1].b = DO_NOT_WARN((COLORTYPE)((((long)s[x].b)*(r-y)+((long)s[x+1].b)*(y))/r));
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
      free(list);
      SIMPLE_OUT_OF_MEMORY_ERROR("modify_by_intensity",
				 sizeof(rgb_group)*THIS->xsize*THIS->ysize+1);
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
**! method object apply_curve( array(int(0..255)) curve_r, array(int(0..255)) curve_g, array(int(0..255)) curve_b )
**! method object apply_curve( array(int(0..255)) curve )
**! method object apply_curve( string channel, array(int(0..255)) curve )
**!
**!  Apply a lookup-table on all pixels in an image.
**!  If only one curve is passed, use the same curve for red, green and blue.
**!  If 'channel' is specified, the curve is only applied to the
**!  specified channel.
**! 
**!  returns a new image object
**! 
**!  arg array(int(0..255)) curve_r
**!  arg array(int(0..255)) curve_g
**!  arg array(int(0..255)) curve_b
**!  arg array(int(0..255)) curve
**!      An array with 256 elements, each between 0 and 255.
**!      It is used as a look-up table, if the pixel value is 2 and
**!      curve[2] is 10, the new pixel value will be 10.
**!
**!  arg string channel
**!      one of "red", "green", "blue", "value", "saturation" and "hue".
**!
**!  see also: gamma, `*, modify_by_intensity
*/

static void image_apply_curve_3( unsigned char curve[3][256] )
{
  int i;
  struct object *o;
  rgb_group *s = THIS->img, *d;
  push_int( THIS->xsize );
  push_int( THIS->ysize );
  o = clone_object( image_program, 2 );
  d = ((struct image *)get_storage( o, image_program ))->img;
  i = THIS->xsize*THIS->ysize;
  
  THREADS_ALLOW();
  for( ; i>0; i-- )
  {
    d->r = curve[0][s->r];
    d->g = curve[1][s->g];
    (d++)->b = curve[2][(s++)->b];
  }
  THREADS_DISALLOW();

  push_object( o );
}

static void image_apply_curve_1( unsigned char curve[256] )
{
  int i;
  struct object *o;
  rgb_group *s = THIS->img, *d;
  push_int( THIS->xsize );
  push_int( THIS->ysize );
  o = clone_object( image_program, 2 );
  d = ((struct image *)get_storage( o, image_program ))->img;
  i = THIS->xsize*THIS->ysize;
  THREADS_ALLOW();
  for( ; i>0; i-- )
  {
    d->r = curve[s->r];
    d->g = curve[s->g];
    (d++)->b = curve[(s++)->b];
  }
  THREADS_DISALLOW();
  push_object( o );
}


static void image_apply_curve_2( struct object *o, 
                                 int channel, 
                                 unsigned char curve[256] )
{
  int i;
  rgb_group *d;
  d = ((struct image *)get_storage(o,image_program))->img;
  i = THIS->xsize*THIS->ysize;

  THREADS_ALLOW();
  switch( channel ) 
  {
   case 0: for( ; i>0; i--,d++ ) d->r = curve[d->r]; break;
   case 1: for( ; i>0; i--,d++ ) d->g = curve[d->g]; break;
   case 2: for( ; i>0; i--,d++ ) d->b = curve[d->b]; break;
  }
  THREADS_DISALLOW();

  push_object( o );
}


struct pike_string *s_red, *s_green, *s_blue;
struct pike_string *s_saturation, *s_value, *s_hue;

static void image_apply_curve( INT32 args )
{
  int i, j;
  switch( args )
  {
   case 3:
     {
       unsigned char curve[3][256];
       for( i = 0; i<3; i++ )
         if( sp[-args+i].type != T_ARRAY ||
             sp[-args+i].u.array->size != 256 )
           bad_arg_error("Image.Image->apply_curve", 
                         sp-args, args, 0, "", sp-args,
                         "Bad arguments to apply_curve()\n");
         else
           for( j = 0; j<256; j++ )
             if( sp[-args+i].u.array->item[j].type == T_INT )
               curve[i][j]=MINIMUM(sp[-args+i].u.array->item[j].u.integer,255);
       pop_n_elems( args );
       image_apply_curve_3( curve );
       return;
     }
   case 2:
     {
       unsigned char curve[256];
       int chan = 0, co = 0;
       struct object *o;

       if( sp[-args].type != T_STRING )
	 SIMPLE_BAD_ARG_ERROR("Image.Image->apply_curve()", 1,
			      "string");
       if( sp[-args+1].type != T_ARRAY ||
           sp[-args+1].u.array->size != 256 )
	 SIMPLE_BAD_ARG_ERROR("Image.Image->apply_curve()", 2,
			      "256 element array");

       for( j = 0; j<256; j++ )
	 if( sp[-args+1].u.array->item[j].type == T_INT )
	   curve[j] = MINIMUM(sp[-args+1].u.array->item[j].u.integer,255);

       MAKE_CONST_STRING(s_red,"red");
       MAKE_CONST_STRING(s_green,"green");
       MAKE_CONST_STRING(s_blue,"blue");
       MAKE_CONST_STRING(s_saturation,"saturation");
       MAKE_CONST_STRING(s_value,"value");
       MAKE_CONST_STRING(s_hue,"hue");

       if( sp[-args].u.string == s_red )
       {
         co = 1;
         chan = 0;
       }
       else if( sp[-args].u.string == s_green )
       {
         co = 1;
         chan = 1;
       }
       else if( sp[-args].u.string == s_blue )
       {
         co = 1;
         chan = 2;
       }
       else if( sp[-args].u.string == s_hue )
       {
         chan = 0;
         co = 0;
       }
       else if( sp[-args].u.string == s_saturation )
       {
         chan = 1;
         co = 0;
       }
       else if( sp[-args].u.string == s_value )
       {
         chan = 2;
         co = 0;
       }
       else
	 Pike_error("Unknown channel in argument 1.\n");

       if( co )
       { 
         push_int( THIS->xsize );
         push_int( THIS->ysize );
         o = clone_object( image_program, 2 );
         MEMCPY( ((struct image *)get_storage(o,image_program))->img, 
                 THIS->img, 
                 THIS->xsize*THIS->ysize*sizeof(rgb_group) );
       }
       else
       {
         image_rgb_to_hsv( 0 );
         o = sp[-1].u.object;
         sp--;
       }
       image_apply_curve_2( o, chan, curve );
       if( !co )
       {
         apply( sp[-1].u.object, "hsv_to_rgb", 0 );
         stack_swap();
         pop_stack();
       }
       return;
     }
   case 1:
     {
       unsigned char curve[256];
       if( sp[-args].type != T_ARRAY ||
           sp[-args].u.array->size != 256 )
         bad_arg_error("Image.Image->apply_curve", 
                       sp-args, args, 0, "", sp-args,
                       "Bad arguments to apply_curve()\n" );
       else
         for( j = 0; j<256; j++ )
           if( sp[-args].u.array->item[j].type == T_INT )
             curve[j] = MINIMUM(sp[-args].u.array->item[j].u.integer,255);
       pop_n_elems( args );
       image_apply_curve_1( curve );
       return;
     }
  }
}


/*
**! method object gamma(float g)
**! method object gamma(float gred, float ggreen, float gblue)
**!     Calculate pixels in image by gamma curve.
**!
**!	Intensity of new pixels are calculated by:<br>
**!     <i>i</i>' = <i>i</i>^<i>g</i>
**!
**!	For example, you are viewing your image on a screen
**!	with gamma 2.2. To correct your image to the correct
**!	gamma value, do something like:
**!
**!	<tt>my_display_image(my_image()->gamma(1/2.2);</tt>
**!
**! returns a new image object
**!
**! arg int g
**! arg int gred
**! arg int ggreen
**! arg int gblue
**!	gamma value
**!
**! see also: grey, `*, color
*/

static void img_make_gammatable(COLORTYPE *d,double gamma)
{
   static COLORTYPE last_gammatable[256];
   static double last_gamma;
   static int had_gamma=0;

   if (had_gamma && last_gamma==gamma)
      MEMCPY(d,last_gammatable,sizeof(COLORTYPE)*256);
   else
   {
      int i;
      COLORTYPE *dd=d;
      double q=1/255.0;
      for (i=0; i<256; i++)
      {
	 double d=pow(i*q,gamma)*255;
	 *(dd++)=testrange(d);
      }
      MEMCPY(last_gammatable,d,sizeof(COLORTYPE)*256);
      last_gamma=gamma;
      had_gamma=1;
   }
}

void image_gamma(INT32 args)
{
   INT32 x;
   rgb_group *s,*d;
   struct object *o;
   struct image *img;
   COLORTYPE _newg[256],_newb[256],*newg,*newb;
   double gammar=0.0, gammab=0.0, gammag=0.0;
   COLORTYPE newr[256];

   if (!THIS->img)
     Pike_error("Called Image.Image object is not initialized\n");
   if (args==1) {
      if (sp[-args].type==T_INT) 
	 gammar=gammab=gammag=(double)sp[-args].u.integer;
      else if (sp[-args].type==T_FLOAT) 
	 gammar=gammab=gammag=sp[-args].u.float_number;
      else
	SIMPLE_BAD_ARG_ERROR("Image.Image->gamma",1,"int|float");
   }
   else if (args==3)
   {
      if (sp[-args].type==T_INT) gammar=(double)sp[-args].u.integer;
      else if (sp[-args].type==T_FLOAT) gammar=sp[-args].u.float_number;
      else SIMPLE_BAD_ARG_ERROR("Image.Image->gamma",1,"int|float");

      if (sp[1-args].type==T_INT) gammag=(double)sp[1-args].u.integer;
      else if (sp[1-args].type==T_FLOAT) gammag=sp[1-args].u.float_number;
      else SIMPLE_BAD_ARG_ERROR("Image.Image->gamma",2,"int|float");

      if (sp[2-args].type==T_INT) gammab=(double)sp[2-args].u.integer;
      else if (sp[2-args].type==T_FLOAT) gammab=sp[2-args].u.float_number;
      else SIMPLE_BAD_ARG_ERROR("Image.Image->gamma",3,"int|float");
   }
   else
      Pike_error("Image.Image->gamma(): illegal number of arguments\n");

   if (gammar==gammab && gammab==gammag)
   {
      if (gammar==1.0)  /* just copy */
      {
	 pop_n_elems(args);
	 image_clone(0);
	 return;
      }
      img_make_gammatable(newb=newg=newr,gammar);
   }
   else
   {
      img_make_gammatable(newr,gammar);
      img_make_gammatable(newg=_newg,gammag);
      img_make_gammatable(newb=_newb,gammab);
   }
   
   o=clone_object(image_program,0);
   img=(struct image*)o->storage;
   *img=*THIS;
   if (!(img->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize+1)))
   {
      free_object(o);
      SIMPLE_OUT_OF_MEMORY_ERROR("gamma",
				 sizeof(rgb_group)*THIS->xsize*THIS->ysize+1);
   }

   d=img->img;
   s=THIS->img;

   x=THIS->xsize*THIS->ysize;
   THREADS_ALLOW();
   while (x--)
   {
      d->r=newr[s->r];
      d->g=newg[s->g];
      d->b=newb[s->b];
      d++;
      s++;
   }
   THREADS_DISALLOW();

   pop_n_elems(args);
   push_object(o);
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
   int n,b;
   ptrdiff_t l;
   rgb_group *d;
   char *s;

   if (args<1
       || sp[-args].type!=T_STRING)
      bad_arg_error("Image",sp-args,args,0,"",sp-args,
		"Bad arguments to Image()\n");
   
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
   
   ref_push_object(THISOBJ);
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
   int n,b;
   ptrdiff_t l;
   rgb_group *d;
   char *s;

   if (args<1
       || sp[-args].type!=T_STRING)
      bad_arg_error("Image",sp-args,args,0,"",sp-args,
		"Bad arguments to Image()\n");
   
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
   
   ref_push_object(THISOBJ);
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
      int q=(s->r&1)+(s->g&1)+(s->b&1); /* vote */
      if (b==0) { b=128; d++; }
      *d|=(q>1)*b; b>>=1;
      s++;
   }

   pop_n_elems(args);
   push_string(end_shared_string(ps));
}

/*
**! method string cast(string type)
**!	Cast the image to another datatype. Currently supported
**!	are string ("rgbrgbrgb...") and array (double array 
**!	of <ref>Image.Color</ref> objects).
**! see also: Image.Color,Image.X
*/

void image_cast(INT32 args)
{
   if (!args)
      SIMPLE_TOO_FEW_ARGS_ERROR("Image.Image->cast",1);
   if (sp[-args].type==T_STRING||sp[-args].u.string->size_shift)
   {
     if (!THIS->img)
       Pike_error("Called Image.Image object is not initialized\n");

      if (strncmp(sp[-args].u.string->str,"array",5)==0)
      {
	 int i,j;
	 rgb_group *s=THIS->img;

	 pop_n_elems(args);

	 for (i=0; i<THIS->ysize; i++)
	 {
	    for (j=0; j<THIS->xsize; j++)
	    {
	       _image_make_rgb_color(s->r,s->g,s->b);
	       s++;
	    }
	    f_aggregate(THIS->xsize);
	 }
	 f_aggregate(THIS->ysize);

	 return;
      }
      if (strncmp(sp[-args].u.string->str,"string",6)==0)
      {
	 pop_n_elems(args);
	 push_string(make_shared_binary_string((char *)THIS->img,
					       THIS->xsize*THIS->ysize
					       *sizeof(rgb_group)));
	 return;
      }
      
   }
   SIMPLE_BAD_ARG_ERROR("Image.Image->cast",1,"string(\"array\"|\"string\")");
}

static void image__sprintf( INT32 args )
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
     push_constant_text("Image.Image");
     return;
   case 'O':
     push_constant_text( "Image.Image( %d x %d /* %.1fKb */)" );
     push_int( THIS->xsize );
     push_int( THIS->ysize );
     push_float( DO_NOT_WARN((FLOAT_TYPE)((THIS->xsize * THIS->ysize) /
					  1024.0 * 3.0)) ); 
     f_sprintf( 4 );
     return;
   default:
     push_int(0);
     return;
  }
}

/*
**! method object grey_blur(int no_pass)
**!	Works like blur, but only operates on the r color channel.
**!     A faster alternative to blur for grey scale images. @[no_pass]
**!     is the number of times the blur matrix is applied.
**!
**!	<table><tr valign=center>
**!	<td><illustration> return lena(); </illustration></td>
**!	<td><illustration> return lena()->grey_blur(1); </illustration></td>
**!	<td><illustration> return lena()->grey_blur(5); </illustration></td>
**!	</tr><tr>
**!	<td>original</td>
**!	<td>grey_blur(1)</td>
**!	<td>grey_blur(5)</td>
**!	</tr></table>
**!
**! see also: blur
*/

static void image_grey_blur( INT32 args )
{
  /* Basically a exactly like blur, but only uses the r color channel. */
  INT_TYPE t;
  int x, y, cnt;
  int xe = THIS->xsize;
  int ye = THIS->ysize;
  rgb_group *rgb = THIS->img;
  if( args != 1 )
    SIMPLE_TOO_FEW_ARGS_ERROR("grey_blur",1);

  if( !rgb )
    Pike_error("This object is not initialized\n");
    
  if (sp[-args].type!=T_INT)  SIMPLE_BAD_ARG_ERROR("grey_blur",0,"integer");

  t = sp[-args].u.integer;  /* times */

  for( cnt=0; cnt<t; cnt++ )
  {
    rgb_group *ro1=NULL, *ro2=NULL, *ro3=rgb;
    for( y=0; y<ye; y++ )
    {
      ro1 = ro2;
      ro2 = ro3;
      ro3 = ( y < ye-1 ) ? rgb+xe*(y+1) : 0;

      for( x=0; x<xe; x++ )
      {
	int tmp = 0;
	int n = 0;
	if( ro1 )
	{
	  if( x > 1 )    {n++;tmp += ro1[x-1].r;};
	  tmp += ro1[x].r; n++;
	  if( x < xe-1 ) {n++;tmp += ro1[x+1].r;};
	}
	if( x > 1 )    {n++;tmp += ro2[x-1].r;};
	tmp += ro2[x].r; n++;
	if( x < xe-1 ) {n++;tmp += ro2[x+1].r;};
	if( ro3 )
	{
	  if( x > 1 )    {n++;tmp += ro3[x-1].r;};
	  tmp += ro3[x].r; n++;
	  if( x < xe-1 ) {n++;tmp += ro3[x+1].r;};
	}
	ro2[x].r = ro2[x].g = ro2[x].b = tmp/n;
      }
    }
  }
  pop_n_elems( args );
  ref_push_object( THISOBJ );
}

/*
**! method string blur(int no_pass)
**!	A special case of apply_matrix that creates a blur effect.
**!     About four times faster than the generic apply_matrix.
**!     @[no_pass] is the number of times the blur matrix is applied.
**!
**!	<table><tr valign=center>
**!	<td><illustration> return lena(); </illustration></td>
**!	<td><illustration> return lena()->blur(1); </illustration></td>
**!	<td><illustration> return lena()->blur(5); </illustration></td>
**!	</tr><tr>
**!	<td>original</td>
**!	<td>blur(1)</td>
**!	<td>blur(5)</td>
**!	</tr></table>
**!
**! see also: apply_matrix, grey_blur
*/

static void image_blur( INT32 args )
/* about four times faster than the generic apply matrix for this
 * special case.  */
{
  INT_TYPE t;
  int x, y, cnt;
  int xe = THIS->xsize;
  int ye = THIS->ysize;
  rgb_group *rgb = THIS->img;
  if( args != 1 )
    SIMPLE_TOO_FEW_ARGS_ERROR("blur",1);

  if( !rgb )
    Pike_error("This object is not initialized\n");
    
  if (sp[-args].type!=T_INT)  SIMPLE_BAD_ARG_ERROR("blur",0,"integer");

  t = sp[-args].u.integer;  /* times */

  for( cnt=0; cnt<t; cnt++ )
  {
    rgb_group *ro1=NULL, *ro2=NULL, *ro3=rgb;
    for( y=0; y<ye; y++ )
    {
      ro1 = ro2;
      ro2 = ro3;
      ro3 = ( y < ye-1 ) ? rgb+xe*(y+1) : 0;

      for( x=0; x<xe; x++ )
      {
	int tmpr=0, tmpg=0, tmpb=0;
	int n=0;
	if( ro1 )
	{
	  if( x > 1 )    {
	    n++;
	    tmpr += ro1[x-1].r;
	    tmpg += ro1[x-1].g;
	    tmpb += ro1[x-1].b;
	  };
	  n++;
	  tmpr += ro1[x].r;
	  tmpg += ro1[x].g;
	  tmpb += ro1[x].b;
	  if( x < xe-1 )
	  {
	    n++;
	    tmpr += ro1[x+1].r;
	    tmpg += ro1[x+1].g;
	    tmpb += ro1[x+1].b;
	  };
	}
	if( x > 1 )
	{
	  n++;
	  tmpr += ro2[x-1].r;
	  tmpg += ro2[x-1].g;
	  tmpb += ro2[x-1].b;
	}
	n++;
	tmpr += ro2[x].r;
	tmpg += ro2[x].g;
	tmpb += ro2[x].b;

	if( x < xe-1 )
	{
	  n++;
	  tmpr += ro2[x+1].r;
	  tmpg += ro2[x+1].g;
	  tmpb += ro2[x+1].b;
	}
	if( ro3 )
	{
	  if( x > 1 )
	  {
	    n++;
	    tmpr += ro3[x-1].r;
	    tmpg += ro3[x-1].g;
	    tmpb += ro3[x-1].b;
	  }
	  n++;
	  tmpr += ro3[x].r;
	  tmpg += ro3[x].g;
	  tmpb += ro3[x].b;
	  if( x < xe-1 )
	  {
	    n++;
	    tmpr += ro3[x+1].r;
	    tmpg += ro3[x+1].g;
	    tmpb += ro3[x+1].b;
	  }
	}
	ro2[x].r = tmpr/n;
	ro2[x].g = tmpg/n;
	ro2[x].b = tmpb/n;
      }
    }
  }
  pop_n_elems( args );
  ref_push_object( THISOBJ );
}



void image_tobitmap(INT32 args)
{
   int xs;
   int i,j,left,bit,dbits;
   struct pike_string *res;
   unsigned char *d;
   rgb_group *s;

   pop_n_elems(args);
   if (!THIS->img)
     Pike_error("Called Image.Image object is not initialized\n");

   xs=(THIS->xsize+7)>>3;

   res=begin_shared_string(xs*THIS->ysize);
   d=(unsigned char *)res->str;

   s=THIS->img;

   j=THIS->ysize;
   while (j--)
   {
      i=THIS->xsize;
      while (i)
      {
	 left=8;
	 bit=1;
	 dbits=0;
	 while (left-- && i)
	 {
	    if (s->r||s->g||s->b) dbits|=bit;
	    bit<<=1;
	    s++;
	    i--;
	 }
	 *(d++)=(unsigned char)dbits;
      }
   }

   push_string(end_shared_string(res));
}

/***************** global init etc *****************************/

#define tRGB tOr3(tColor,tVoid,tInt) tOr(tInt,tVoid) tOr(tInt,tVoid)

void init_image_image(void)
{
   int i;
   for (i=0; i<CIRCLE_STEPS; i++) 
      circle_sin_table[i]=DOUBLE_TO_INT(4096*sin(((double)i)*2.0*
						 3.141592653589793/
						 (double)CIRCLE_STEPS));

   ADD_STORAGE(struct image);
   
   ADD_FUNCTION("_sprintf", image__sprintf, tFunc(tInt , tString), 0 );

   ADD_FUNCTION("create",image_create,
		tOr(tFunc(tOr(tInt,tVoid) tOr(tInt,tVoid) tRGB,tVoid),
		    tFuncV(tInt tInt tString,tMixed,tVoid)),0);
   ADD_FUNCTION("clone",image_clone,
		tOr3(tFunc(tInt tInt tInt tInt tRGB,tObj),
                     tFunc(tRGB,tObj),
                     tFunc(tNone,tObj)),0);
   ADD_FUNCTION("new",image_clone, /* alias */
		tFunc(tOr(tInt,tVoid) tOr(tInt,tVoid) tRGB,tObj),0);
   ADD_FUNCTION("clear",image_clear,
		tFunc(tRGB,tObj),0);

   ADD_FUNCTION("cast",image_cast,
		tFunc(tStr,tStr),0);
   ADD_FUNCTION("tobitmap",image_tobitmap,tFunc(tNone,tStr),0);


   ADD_FUNCTION("copy",image_copy,
		tFunc(tOr(tVoid,tInt) tOr(tVoid,tInt) tOr(tVoid,tInt) 
		      tOr(tVoid,tInt) tRGB,tObj),0);
   ADD_FUNCTION("autocrop",image_autocrop,
		tFuncV(tNone,tOr(tVoid,tInt),tObj),0);
   ADD_FUNCTION("find_autocrop",image_find_autocrop,
		tFuncV(tNone,tOr(tVoid,tInt),tArr(tInt)),0);
   ADD_FUNCTION("scale",image_scale,
		tFunc(tOr(tInt,tFlt) tOr3(tInt,tFlt,tVoid),tObj),0);
   ADD_FUNCTION("bitscale",image_bitscale,
		tFunc(tOr(tInt,tFlt) tOr3(tInt,tFlt,tVoid),tObj),0);
   ADD_FUNCTION("translate",image_translate,
		tFunc(tOr(tInt,tFlt) tOr(tInt,tFlt),tObj),0);
   ADD_FUNCTION("translate_expand",image_translate_expand,
		tFunc(tOr(tInt,tFlt) tOr(tInt,tFlt),tObj),0);

   ADD_FUNCTION("paste",image_paste,
		tFunc(tObj tOr(tInt,tVoid) tOr(tInt,tVoid),tObj),0);
   ADD_FUNCTION("paste_alpha",image_paste_alpha,
		tFunc(tObj tInt tOr(tInt,tVoid) tOr(tInt,tVoid),tObj),0);
   ADD_FUNCTION("paste_mask",image_paste_mask,
		tFunc(tObj tObj tOr(tInt,tVoid) tOr(tInt,tVoid),tObj),0);
   ADD_FUNCTION("paste_alpha_color",image_paste_alpha_color,
		tOr6(tFunc(tObj tInt tInt,tObj),
		     tFunc(tObj tInt tInt tInt,tObj),
		     tFunc(tObj tInt tInt tInt tInt tInt,tObj),
		     tFunc(tObj tColor tInt tInt,tObj),
		     tFunc(tObj tColor,tObj),
		     tFunc(tObj,tObj)),0);

   ADD_FUNCTION("setcolor",image_setcolor,
		tFunc(tInt tInt tInt tOr(tInt,tVoid),tObj),0);
   ADD_FUNCTION("setpixel",image_setpixel,
		tFunc(tInt tInt tRGB,tObj),0);
   ADD_FUNCTION("getpixel",image_getpixel,
		tFunc(tInt tInt,tArr(tInt)),0);
   ADD_FUNCTION("line",image_line,
		tFunc(tInt tInt tInt tInt tRGB,tObj),0);
   ADD_FUNCTION("circle",image_circle,
		tFunc(tInt tInt tInt tInt tRGB,tObj),0);
   ADD_FUNCTION("box",image_box,
		tFunc(tInt tInt tInt tInt tRGB,tObj),0);
   ADD_FUNCTION("tuned_box",image_tuned_box,
		tFunc(tInt tInt tInt tInt tArray,tObj),0);
   ADD_FUNCTION("gradients",image_gradients,
		tFuncV(tNone,tOr(tArr(tInt),tFlt),tObj),0);
   ADD_FUNCTION("polygone",image_polyfill,
		tFuncV(tNone,tArr(tOr(tFlt,tInt)),tObj),0);
   ADD_FUNCTION("polyfill",image_polyfill,
		tFuncV(tNone,tArr(tOr(tFlt,tInt)),tObj),0);

   ADD_FUNCTION("gray",image_grey,
		tFunc(tRGB,tObj),0);
   ADD_FUNCTION("grey",image_grey,
		tFunc(tRGB,tObj),0);
   ADD_FUNCTION("color",image_color,
		tFunc(tRGB,tObj),0);
   ADD_FUNCTION("change_color",image_change_color,
		tOr(tFunc(tInt tInt tInt tRGB,tObj),
		    tFunc(tColor tRGB,tObj)),0);
   ADD_FUNCTION("invert",image_invert,
		tFunc(tRGB,tObj),0);
   ADD_FUNCTION("threshold",image_threshold,
		tFunc(tOr(tInt,tRGB),tObj),0);
   ADD_FUNCTION("distancesq",image_distancesq,
		tFunc(tRGB,tObj),0);

   ADD_FUNCTION("rgb_to_hsv",image_rgb_to_hsv,
		tFunc(tVoid,tObj),0);
   ADD_FUNCTION("hsv_to_rgb",image_hsv_to_rgb,
		tFunc(tVoid,tObj),0);

   ADD_FUNCTION("select_from",image_select_from,
		tFunc(tInt tInt tOr(tInt, tVoid),tObj),0);

   ADD_FUNCTION("apply_matrix",image_apply_matrix,
		tFuncV(tArr(tArr(tOr(tInt,tArr(tInt)))),tOr(tVoid,tInt),tObj),0);

   ADD_FUNCTION("grey_blur",image_grey_blur,tFunc(tInt,tObj),0);
   ADD_FUNCTION("blur",image_blur,tFunc(tInt,tObj),0);
   
   ADD_FUNCTION("outline",image_outline,
		tOr5(tFunc(tOr(tVoid,tArr(tArr(tInt))),tObj),
		     tFunc(tArr(tArr(tInt)) tInt tInt tInt tOr(tVoid,tInt),tObj),
		     tFunc(tArr(tArr(tInt)) tInt tInt tInt tInt tInt tInt tOr(tVoid,tInt),tObj),
		     tFunc(tInt tInt tInt tOr(tVoid,tInt),tObj),
		     tFunc(tInt tInt tInt tInt tInt tInt tOr(tVoid,tInt),tObj)),0);
   ADD_FUNCTION("outline_mask",image_outline_mask,
		tOr(tFunc(tOr(tVoid,tArr(tArr(tInt))),tObj),
		    tFunc(tArr(tArr(tInt)) tInt tInt tInt,tObj)),0);
   ADD_FUNCTION("modify_by_intensity",image_modify_by_intensity,
		tFuncV(tInt tInt tInt tOr(tInt,tColor) tOr(tInt,tColor),
		       tOr(tInt,tColor),tObj),0);
   ADD_FUNCTION("gamma",image_gamma,
		tOr(tFunc(tOr(tFlt,tInt),tObj),
		    tFunc(tOr(tFlt,tInt) tOr(tFlt,tInt) tOr(tFlt,tInt),tObj)),0);

   ADD_FUNCTION("apply_curve",image_apply_curve,
                tOr3( tFunc(tArr(tInt) tArr(tInt) tArr(tInt),tObj),
                      tFunc(tArr(tInt),tObj),
                      tFunc(tString tArr(tInt),tObj) ), 0);

   ADD_FUNCTION("rotate_ccw",image_ccw,
		tFunc(tNone,tObj),0);
   ADD_FUNCTION("rotate_cw",image_cw,
		tFunc(tNone,tObj),0);
   ADD_FUNCTION("mirrorx",image_mirrorx,
		tFunc(tNone,tObj),0);
   ADD_FUNCTION("mirrory",image_mirrory,
		tFunc(tNone,tObj),0);
   ADD_FUNCTION("skewx",image_skewx,
		tFunc(tOr(tInt,tFlt) tRGB,tObj),0);
   ADD_FUNCTION("skewy",image_skewy,
		tFunc(tOr(tInt,tFlt) tRGB,tObj),0);
   ADD_FUNCTION("skewx_expand",image_skewx_expand,
		tFunc(tOr(tInt,tFlt) tRGB,tObj),0);
   ADD_FUNCTION("skewy_expand",image_skewy_expand,
		tFunc(tOr(tInt,tFlt) tRGB,tObj),0);

   ADD_FUNCTION("rotate",image_rotate,
		tFunc(tOr(tInt,tFlt) tRGB,tObj),0);
   ADD_FUNCTION("rotate_expand",image_rotate_expand,
		tFunc(tOr(tInt,tFlt) tRGB,tObj),0);

   ADD_FUNCTION("xsize",image_xsize,
		tFunc(tNone,tInt),0);
   ADD_FUNCTION("ysize",image_ysize,
		tFunc(tNone,tInt),0);

   ADD_FUNCTION("noise",image_noise,
		tFunc(tArr(tOr3(tInt,tFloat,tColor))
		      tOr(tFlt,tVoid) tOr(tFlt,tVoid) 
		      tOr(tFlt,tVoid) tOr(tFlt,tVoid),tObj),0);
   ADD_FUNCTION("turbulence",image_turbulence,
		tFunc(tArr(tOr3(tInt,tFloat,tColor))
		      tOr(tInt,tVoid) tOr(tFlt,tVoid) 
		      tOr(tFlt,tVoid) tOr(tFlt,tVoid) tOr(tFlt,tVoid),tObj),0);
   ADD_FUNCTION("random",image_random,
		tFunc(tOr(tVoid,tInt),tObj),0);
   ADD_FUNCTION("randomgrey",image_randomgrey,
		tFunc(tOr(tVoid,tInt),tObj),0);

   ADD_FUNCTION("dct",image_dct,
		tFunc(tNone,tObj),0);

   ADD_FUNCTION("`-",image_operator_minus,
		tFunc(tOr3(tObj,tArr(tInt),tInt),tObj),0);
   ADD_FUNCTION("`+",image_operator_plus,
		tFunc(tOr3(tObj,tArr(tInt),tInt),tObj),0);
   ADD_FUNCTION("`*",image_operator_multiply,
		tFunc(tOr3(tObj,tArr(tInt),tInt),tObj),0);
   ADD_FUNCTION("`/",image_operator_divide,
		tFunc(tOr3(tObj,tArr(tInt),tInt),tObj),0);
   ADD_FUNCTION("`%",image_operator_rest,
		tFunc(tOr3(tObj,tArr(tInt),tInt),tObj),0);
   ADD_FUNCTION("`&",image_operator_minimum,
		tFunc(tOr3(tObj,tArr(tInt),tInt),tObj),0);
   ADD_FUNCTION("`|",image_operator_maximum,
		tFunc(tOr3(tObj,tArr(tInt),tInt),tObj),0);

   ADD_FUNCTION("`==",image_operator_equal,
		tFunc(tOr3(tObj,tArr(tInt),tInt),tInt),0);
   ADD_FUNCTION("`<",image_operator_lesser,
		tFunc(tOr3(tObj,tArr(tInt),tInt),tInt),0);
   ADD_FUNCTION("`>",image_operator_greater,
		tFunc(tOr3(tObj,tArr(tInt),tInt),tInt),0);

   ADD_FUNCTION("min",image_min,
		tFunc(tNone,tArr(tInt)),0);
   ADD_FUNCTION("max",image_max,
		tFunc(tNone,tArr(tInt)),0);
   ADD_FUNCTION("sum",image_sum,
		tFunc(tNone,tArr(tInt)),0);
   ADD_FUNCTION("sumf",image_sumf,
		tFunc(tNone,tArr(tInt)),0);
   ADD_FUNCTION("average",image_average,
		tFunc(tNone,tArr(tInt)),0);
  
   ADD_FUNCTION("find_min",image_find_min,
		tOr(tFunc(tNone,tArr(tInt)),
		    tFunc(tInt tInt tInt,tArr(tInt))),0);
   ADD_FUNCTION("find_max",image_find_max,
		tOr(tFunc(tNone,tArr(tInt)),
		    tFunc(tInt tInt tInt,tArr(tInt))),0);
		
   ADD_FUNCTION("read_lsb_rgb",image_read_lsb_rgb,
		tFunc(tNone,tString),0);
   ADD_FUNCTION("write_lsb_rgb",image_write_lsb_rgb,
		tFunc(tString,tObj),0);
   ADD_FUNCTION("read_lsb_grey",image_read_lsb_grey,
		tFunc(tNone,tString),0);
   ADD_FUNCTION("write_lsb_grey",image_write_lsb_grey,
		tFunc(tString,tObj),0);

   ADD_FUNCTION("orient4",image_orient4,
		tFunc(tNone,tArr(tObj)),0);
   ADD_FUNCTION("orient",image_orient,
		tFunc(tNone,tObj),0);
  
   ADD_FUNCTION("phaseh",image_phaseh,
		tFunc(tNone,tObj),0);
   ADD_FUNCTION("phasev",image_phasev,
		tFunc(tNone,tObj),0);
   ADD_FUNCTION("phasehv",image_phasehv,
		tFunc(tNone,tObj),0);
   ADD_FUNCTION("phasevh",image_phasevh,
		tFunc(tNone,tObj),0);

   ADD_FUNCTION("match_phase",image_match_phase,
		tOr4(tFunc(tOr(tInt,tFloat) tObj,tObj),
		     tFunc(tOr(tInt,tFloat) tObj tObj tObj,tObj),
		     tFunc(tOr(tInt,tFloat) tObj tObj tInt,tObj),
		     tFunc(tOr(tInt,tFloat) tObj tObj tObj tObj tInt,tObj)),0);
   ADD_FUNCTION("match_norm",image_match_norm,
		tOr4(tFunc(tOr(tInt,tFloat) tObj,tObj),
		     tFunc(tOr(tInt,tFloat) tObj tObj tObj,tObj),
		     tFunc(tOr(tInt,tFloat) tObj tObj tInt,tObj),
		     tFunc(tOr(tInt,tFloat) tObj tObj tObj tObj tInt,tObj)),0);
   ADD_FUNCTION("match_norm_corr",image_match_norm_corr,
		tOr4(tFunc(tOr(tInt,tFloat) tObj,tObj),
		     tFunc(tOr(tInt,tFloat) tObj tObj tObj,tObj),
		     tFunc(tOr(tInt,tFloat) tObj tObj tInt,tObj),
		     tFunc(tOr(tInt,tFloat) tObj tObj tObj tObj tInt,tObj)),0);
   ADD_FUNCTION("match",image_match,
		tOr4(tFunc(tOr(tInt,tFloat) tObj,tObj),
		     tFunc(tOr(tInt,tFloat) tObj tObj tObj,tObj),
		     tFunc(tOr(tInt,tFloat) tObj tObj tInt,tObj),
		     tFunc(tOr(tInt,tFloat) tObj tObj tObj tObj tInt,tObj)),0);
  
   ADD_FUNCTION("apply_max",image_apply_max,
		tFuncV(tArr(tArr(tOr(tInt,tArr(tInt)))),
		       tOr(tVoid,tInt),tObj),0);
   ADD_FUNCTION("make_ascii",image_make_ascii,
		tFunc(tObj tObj tObj tObj tOr(tInt,tVoid),tStr),0);
  
   ADD_FUNCTION("test",image_test,
		tFunc(tOr(tVoid,tInt),tObj),0);

   set_init_callback(init_image_struct);
   set_exit_callback(exit_image_struct);


#ifndef FAKE_DYNAMIC_LOAD
   /* Added by per: Export all functions needed by _Image_GIF */
   PIKE_MODULE_EXPORT(Image, image_lay );
   PIKE_MODULE_EXPORT(Image, image_colortable_write_rgb );
   PIKE_MODULE_EXPORT(Image, image_colortable_size );
   PIKE_MODULE_EXPORT(Image, image_colortable_index_8bit_image );
   PIKE_MODULE_EXPORT(Image, image_colortable_internal_floyd_steinberg );
#endif


   s_red=0;
   s_green=0;
   s_blue=0;
   s_value=0;
   s_saturation=0;
   s_hue=0;

   s_grey=0;
   s_rgb=0;
   s_cmyk=0;
   s_cmy=0;
   s_test=0;
   s_gradients=0;
   s_noise=0;
   s_turbulence=0;
   s_random=0;
   s_randomgrey=0;
   s_tuned_box=0;
}

void exit_image_image(void) 
{
}
