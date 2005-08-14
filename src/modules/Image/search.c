/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: search.c,v 1.30 2005/08/14 02:26:40 nilsson Exp $
*/

/*
**! module Image
**! class Image
*/
#include "global.h"

#include <math.h>
#include <ctype.h>

#include "stralloc.h"
#include "global.h"
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

#define testrange(x) MAXIMUM(MINIMUM((x),255),0)

static const double c0=0.70710678118654752440;
static const double my_PI=3.14159265358979323846;

#if 0
#include <sys/resource.h>
#define CHRONO(X) chrono(X);

static void chrono(char *x)
{
   struct rusage r;
   static struct rusage rold;
   getrusage(RUSAGE_SELF,&r);
   fprintf(stderr,"%s: %ld.%06ld - %ld.%06ld\n",x,
	   (long)r.ru_utime.tv_sec,(long)r.ru_utime.tv_usec,
	   
	   (long)(((r.ru_utime.tv_usec-rold.ru_utime.tv_usec<0)?-1:0)
		  +r.ru_utime.tv_sec-rold.ru_utime.tv_sec),
	   (long)(((r.ru_utime.tv_usec-rold.ru_utime.tv_usec<0)?1000000:0)
		  + r.ru_utime.tv_usec-rold.ru_utime.tv_usec)
      );

   rold=r;
}
#else
#define CHRONO(X)
#endif

/*
**! method object phaseh()
**! method object phasev()
**! method object phasevh()
**! method object phasehv()
**!	Draws images describing the phase
**!     of the current image. phaseh gives the
**!     horizontal phase and phasev the vertical
**!     phase.
**!
**!	<tt>phaseh</tt> gives an image
**!	where
**!     <pre>
**!            max  falling   min  rising
**!     value=  0     64      128   192 
**!     </pre>
**!   
**!     0 is set if there is no way to determine
**!     if it is rising or falling. This is done
**!     for the every red, green and blue part of
**!     the image.
**! 
**!     Phase images can be used to create ugly
**!     effects or to find meta-information
**!     in the orginal image.
**!
**!	<table border=0>
**!	<tr>
**!	<td><illustration> return lena(); </illustration></td>
**!	<td><illustration> return lena()->phaseh(); </illustration></td>
**!	<td><illustration> return lena()->phasev(); </illustration></td>
**!	<td><illustration> return lena()->phasevh(); </illustration></td>
**!	<td><illustration> return lena()->phasehv(); </illustration></td>
**!	</tr>
**!	<tr>
**!	<td>original </td>
**!	<td>phaseh() </td>
**!	<td>phasev() </td>
**!	<td>phasevh()</td>
**!	<td>phasehv()</td>
**!	</tr>
**!	</table>
**! 
**! returns the new image object
**  see also: match_phase
**! bugs
**!	0 should not be set as explained above.
**! note
**!	<b>experimental status</b>; may not be exact the same
**!	output in later versions
*/
static INLINE int sq(int a) { return a*a; }
static INLINE int my_abs(int a) { return (a<0)?-a:a; }

/* phase-image creating functions */

#define NEIG 1
#define IMAGE_PHASE image_phaseh
#include "phase.h"
#undef NEIG
#undef IMAGE_PHASE

#define NEIG xz
#define IMAGE_PHASE image_phasev
#include "phase.h"
#undef NEIG
#undef IMAGE_PHASE

#define NEIG xz+1
#define IMAGE_PHASE image_phasehv
#include "phase.h"
#undef NEIG
#undef IMAGE_PHASE

#define NEIG xz-1
#define IMAGE_PHASE image_phasevh
#include "phase.h"
#undef NEIG
#undef IMAGE_PHASE

/* the match-functions */

/*
**! method object match(int|float scale, object needle)
**! method object match(int|float scale, object needle, object haystack_cert, object needle_cert)
**! method object match(int|float scale, object needle, object haystack_avoid, int foo)
**! method object match(int|float scale, object needle, object haystack_cert, object needle_cert, object haystack_avoid, int foo)
**!
**!     This method creates an image that describes the
**!     match in every pixel in the image and the 
**!     needle-Image.
**!
**!     <pre>
**!        new pixel value = 
**!          sum( my_abs(needle_pixel-haystack_pixel))
**!     </pre>
**!
**!     The new image only have the red rgb-part set.
**!
**!
**! arg int|float scale
**!	Every pixel is divided with this value.
**!     Note that a proper value here depends on
**!     the size of the neadle.
**!
**! arg object needle
**!	The image to use for the matching. 
**!
**! arg object haystack_cert
**!	This image should be the same size as
**!     the image itselves. A non-white-part of the
**!     haystack_cert-image modifies the output
**!     by lowering it. 
**!
**! arg object needle_cert
**!	The same, but for the needle-image.
**!
**! arg int foo
**! arg object haystack_avoid
**!	This image should be the same size as
**!     the image itselves. If foo is less than the red
**!     value in haystack_avoid the corresponding
**!     matching-calculating is not calculated. The avoided parts
**!     are drawn in the color 0,100,0.
**!
**! returns the new image object
**! see also: phasev,phaseh
**! note
**!	<b>experimental status</b>; may not be exact the same
**!	output in later versions
*/


#define SCALE_MODIFY(x) (x)
#define NAME "match"
#define INAME image_match
#define PIXEL_VALUE_DISTANCE(CO) \
       (my_abs(haystacki[j].CO-needlei[ny*nxs+nx].CO))
#include "match.h"

#define NAME "match_phase"
#define INAME image_match_phase
#define PIXEL_VALUE_DISTANCE(CO)  \
       ((h=haystacki[j].CO),(n=needlei[ny*nxs+nx].CO), \
        ((h>n)? MINIMUM((h-n),(255-h+n)) : MINIMUM((n-h),(255-n+h))))
#include "match.h"

#define NAME "match_norm"
#define INAME image_match_norm
#define NEEDLEAVRCODE 
#define PIXEL_VALUE_DISTANCE(CO)  \
 (my_abs(( haystacki[j].CO-tempavr )-( needlei[ny*nxs+nx].CO-needle_average)))
#include "match.h"

#define NAME "match_norm_corr"
#define INAME image_match_norm_corr
#undef SCALE_MODIFY 
#define SCALE_MODIFY(x) (1.0/MAXIMUM(1.0,x))
#define PIXEL_VALUE_DISTANCE(CO)  \
       (((haystacki[j].CO-tempavr)/2+128) \
        * ( (needlei[ny*nxs+nx].CO-needle_average)/2+128 ))
#include "match.h"
#undef NEEDLEAVRCODE
#undef SUMCHECK


/*
**! method string make_ascii(object orient1,object orient2,object orient3,object orient4,int|void tlevel,int|void xsize,int|void ysize)
**!
**!     This method creates a string that looks like 
**!     the image. Example:
**!     <pre>
**!        //Stina is an image with a cat.
**!        array(object) Stina4=Stina->orient4();
**!        Stina4[1]*=215;
**!        Stina4[3]*=215;
**!        string foo=Stina->make_ascii(@Stina4,40,4,8);
**!     </pre>
**!
**! returns some nice acsii-art.
**! see also: orient, orient4
**! note
**!	<b>experimental status</b>; may not be exact the same
**!	output in later versions
*/


void image_make_ascii(INT32 args)
{
  struct image *img[4],*this;
  INT32 xchar_size=0; 
  INT32 ychar_size=0;
  INT32 tlevel=0;
  int i, x, y,xy=0,y2=0, xmax=0,ymax=0,max;
  struct pike_string *s;

  if (!THIS->img)
    Pike_error("Called Image.Image object is not initialized\n");

  this=THIS;

  if (args<4)
    SIMPLE_TOO_FEW_ARGS_ERROR("Image.Image->make_ascii", 4);

  for(i=0; i<4; i++) {
    if(sp[i-args].type!=T_OBJECT)
      SIMPLE_BAD_ARG_ERROR("Image.Image->make_ascii", i+1, "Image.Image");
    /* FIXME: Check that object type is Image.Image */
    /* FIXME: Check that image sizes are identical */
    img[i]=(struct image*)sp[i-args].u.object->storage;
  }

  if (args>=4)
    tlevel=sp[4-args].u.integer;
  if (args>=5)
    xchar_size=sp[5-args].u.integer;
  if (args>=6)
    ychar_size=sp[6-args].u.integer;

  if (!tlevel) tlevel=40;
  if (!xchar_size) xchar_size=5;
  if (!ychar_size) ychar_size=8;

  tlevel=tlevel*xchar_size*ychar_size;
  xmax=((img[0]->xsize-1)/xchar_size+2);
  ymax=((img[0]->ysize-1)/ychar_size+1);
  max=xmax*ymax;
  s=begin_shared_string(max);
  
  THREADS_ALLOW();  

  /*fix /n at each row*/
  for(i=xmax-1; i<max; i+=xmax)
    s->str[i]='\n';
  
  for(x=0; x<xmax-1; x++)
    {
      for(y=0; y<ymax-1; y++)
	{	
	  int dir0,dir1,dir2,dir3;
	  int xstop=0,ystop=0;
	  char t=' ';
	  dir0=0;
	  dir1=0;
	  dir2=0;
	  dir3=0;
	  
	  ystop=y*ychar_size+ychar_size;
	  for(y2=y*ychar_size; y2<(ystop); y2++)
	    {
	      xy=y2*img[0]->xsize+x*xchar_size;
	      xstop=xy+xchar_size;
	      for(; xy<xstop; xy++)
		{
		  dir0+=img[0]->img[xy].r;
		  dir1+=img[1]->img[xy].r;
		  dir2+=img[2]->img[xy].r;
		  dir3+=img[3]->img[xy].r;
		}
	    }
	  
	  /*set a part of the string*/
	  if ((dir0<=tlevel)&&
	      (dir1<=tlevel)&&
	      (dir2<=tlevel)&&
	      (dir3<=tlevel))
	    {
	      t=' ';
	    }
	  else if ((dir0>tlevel)&&
		   (dir1>tlevel)&&
		   (dir2>tlevel)&&
		   (dir3>tlevel))
	    {
	      t='*';
	    }
	  else if ((dir0>=dir1)&&(dir0>=dir2)&&
		   (dir0>=dir3))
	    {
	      if ((dir2>=tlevel)&&
		  (dir2>dir1)&&(dir2>dir3))
		t='+';
	      else
		t='|';
	    }
	  else if ((dir1>=dir2)&&(dir1>=dir3))
	    {
	      if ((dir3>=tlevel)&&
		  (dir3>dir0)&&(dir3>dir2))
		t='X';
	      else
		t='/';
	    }
	  else if (dir2>=dir3)
	    {
	      if ((dir0>=tlevel)&&
		  (dir0>dir1)&&(dir0>dir3))
		t='+';
	      else
		t='-';
	    }
	  else
	    {
	      if ((dir1>=tlevel)&&
		  (dir1>dir0)&&(dir1>dir2))
		t='X';
	      else
		t='\\';
	    }
	      
	  s->str[y*xmax+x]=t;
	  
	}
    }
  
  /*fix end of rows*/
  
  /*fix middle*/
  
  
  /*fixa last row*/
  
  /*fix last position*/ 
  /*
**!     <pre>      |      /    -    \
**!          hue=  0     64   128  192  (=red in an hsv image)
**!	</pre>
  */
  
  THREADS_DISALLOW();

  pop_n_elems(args);
  push_string(end_shared_string(s));
  return;
}
/* End make_ascii */



/*
**! method object apply_max(array(array(int|array(int))) matrix)
**! method object apply_max(array(array(int|array(int))) matrix,int r,int g,int b)
**! method object apply_max(array(array(int|array(int))) matrix,int r,int g,int b,int|float div)
**!     This is the same as apply_matrix, but it uses the maximum
**!     instead.
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
**! note
**!	<b>experimental status</b>; may not be exact the same
**!	output in later versions
*/



static INLINE rgb_group _pixel_apply_max(struct image *img,
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
	    r = DOUBLE_TO_INT(MAXIMUM(r, matrix[i+j*width].r *
				      img->img[xp+yp*img->xsize].r));
	    g = DOUBLE_TO_INT(MAXIMUM(g, matrix[i+j*width].g *
				      img->img[xp+yp*img->xsize].g));
	    b = DOUBLE_TO_INT(MAXIMUM(b, matrix[i+j*width].b *
				      img->img[xp+yp*img->xsize].b));
#ifdef MATRIX_DEBUG
	    fprintf(stderr,"%d,%d %d,%d->%d,%d,%d\n",
		    i,j,xp,yp,
		    img->img[x+i+(y+j)*img->xsize].r,
		    img->img[x+i+(y+j)*img->xsize].g,
		    img->img[x+i+(y+j)*img->xsize].b);
#endif
	    sumr = DOUBLE_TO_INT(MAXIMUM(sumr, matrix[i+j*width].r));
	    sumg = DOUBLE_TO_INT(MAXIMUM(sumg, matrix[i+j*width].g));
	    sumb = DOUBLE_TO_INT(MAXIMUM(sumb, matrix[i+j*width].b));
	 }
   if (sumr)
     res.r = DOUBLE_TO_COLORTYPE(testrange(default_rgb.r + r/(sumr * div)));
   else
     res.r = DOUBLE_TO_COLORTYPE(testrange(r * qdiv + default_rgb.r));
   if (sumg)
     res.g = DOUBLE_TO_COLORTYPE(testrange(default_rgb.g + g/(sumg * div)));
   else
     res.g = DOUBLE_TO_COLORTYPE(testrange(g * qdiv + default_rgb.g));
   if (sumb)
     res.b = DOUBLE_TO_COLORTYPE(testrange(default_rgb.g + b/(sumb * div)));
   else
     res.b = DOUBLE_TO_COLORTYPE(testrange(b * qdiv + default_rgb.b));
#ifdef MATRIX_DEBUG
   fprintf(stderr,"->%d,%d,%d\n",res.r,res.g,res.b);
#endif
   return res;
   REVEAL_GLOBAL_VARIABLES();
}


void img_apply_max(struct image *dest,
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

   d=xalloc(sizeof(rgb_group)*img->xsize*img->ysize);

THREADS_ALLOW();

   widthheight=width*height;
   sumr=sumg=sumb=0;
   for (i=0; i<widthheight;)
     {
       sumr=MAXIMUM(sumr, matrix[i].r);
       sumg=MAXIMUM(sumg, matrix[i].g);
       sumb=MAXIMUM(sumb, matrix[i++].b);
     }

   if (!sumr) sumr=1; sumr*=div; qr=1.0/sumr;
   if (!sumg) sumg=1; sumg*=div; qg=1.0/sumg;
   if (!sumb) sumb=1; sumb*=div; qb=1.0/sumb;

   bx=width/2;
   by=height/2;
   ex=width-bx;
   ey=height-by;
   
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
	       r=MAXIMUM(r, ip->r*mp->r);
 	       g=MAXIMUM(g, ip->g*mp->g);
 	       b=MAXIMUM(b, ip->b*mp->b);
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
	 r = default_rgb.r+DOUBLE_TO_INT(r*qr+0.5);
	 dp->r = DOUBLE_TO_COLORTYPE(testrange(r));
	 g = default_rgb.g+DOUBLE_TO_INT(g*qg+0.5);
	 dp->g = DOUBLE_TO_COLORTYPE(testrange(g));
	 b = default_rgb.b+DOUBLE_TO_INT(b*qb+0.5);
	 dp->b = DOUBLE_TO_COLORTYPE(testrange(b));
	 dp++;
      }
   }


   for (y=0; y<img->ysize; y++)
   {
      for (x=0; x<bx; x++)
	 d[x+y*img->xsize]=_pixel_apply_max(img,x,y,width,height,
					       matrix,default_rgb,div);
      for (x=img->xsize-ex; x<img->xsize; x++)
	 d[x+y*img->xsize]=_pixel_apply_max(img,x,y,width,height,
					       matrix,default_rgb,div);
   }

   for (x=0; x<img->xsize; x++)
   {
      for (y=0; y<by; y++)
	 d[x+y*img->xsize]=_pixel_apply_max(img,x,y,width,height,
					       matrix,default_rgb,div);
      for (y=img->ysize-ey; y<img->ysize; y++)
	 d[x+y*img->xsize]=_pixel_apply_max(img,x,y,width,height,
					       matrix,default_rgb,div);
   }


   if (dest->img) free(dest->img);
   *dest=*img;
   dest->img=d;

THREADS_DISALLOW();
}



void image_apply_max(INT32 args)
{
   int width,height,i,j;
   rgbd_group *matrix;
   rgb_group default_rgb;
   struct object *o;
   double div;

   if (args<1 ||
       sp[-args].type!=T_ARRAY)
      bad_arg_error("Image",sp-args,args,0,"",sp-args,
		"Bad arguments to Image()\n");

   if (args>3) 
      if (sp[1-args].type!=T_INT ||
	  sp[2-args].type!=T_INT ||
	  sp[3-args].type!=T_INT)
	 Pike_error("Illegal argument(s) (2,3,4) to Image.Image->apply_max()\n");
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
	 Pike_error("Illegal contents of (root) array (Image.Image->apply_max)\n");
      if (width==-1)
	 width=s.u.array->size;
      else
	 if (width!=s.u.array->size)
	    Pike_error("Arrays has different size (Image.Image->apply_max)\n");
   }
   if (width==-1) width=0;

   matrix=xalloc(sizeof(rgbd_group)*width*height);

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
	       matrix[j+i*width].b=0;
      }
   }

   o=clone_object(image_program,0);

   if (THIS->img)
      img_apply_max((struct image*)o->storage,THIS,
		       width,height,matrix,default_rgb,div);

   free(matrix);

   pop_n_elems(args);
   push_object(o);
}
