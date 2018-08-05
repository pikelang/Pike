/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
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
#include "pike_error.h"
#include "module_support.h"

#include "image.h"


#define sp Pike_sp

extern struct program *image_program;
#ifdef THIS
#undef THIS /* Needed for NT */
#endif
#define THIS ((struct image *)(Pike_fp->current_storage))

static const double c0=0.70710678118654752440;
static const double pi=3.14159265358979323846;

/*
**! method object dct(int newx,int newy)
**!	Scales the image to a new size.
**!
**!	Method for scaling is rather complex;
**!	the image is transformed via a cosine transform,
**!	and then resampled back.
**!
**!	This gives a quality-conserving upscale,
**!	but the algorithm used is n*n+n*m, where n
**!	and m is pixels in the original and new image.
**!
**!	Recommended wrapping algorithm is to scale
**!	overlapping parts of the image-to-be-scaled.
**!
**!	This functionality is actually added as an
**!	true experiment, but works...
**!
**! note
**!	Do NOT use this function if you don't know what
**!     you're dealing with! Read some signal theory first...
**!
**!	It doesn't use any fct (compare: fft) algorithms.
**! returns the new image object
**! arg int newx
**! arg int newy
**!	new image size in pixels
**!
*/

void image_dct(INT32 args)
{
   rgbd_group *area,*val;
   struct object *o;
   struct image *img;
   INT32 x,y,u,v;
   double xsz2,ysz2,enh,xp,yp,dx,dy;
   double *costbl;
   rgb_group *pix;

   CHECK_INIT();

   get_all_args(NULL, args, "%d%d", &x, &y);
   x = MAXIMUM(1, x);
   y = MAXIMUM(1, y);

#ifdef DCT_DEBUG
   fprintf(stderr,"%lu bytes, %lu bytes\n",
           (unsigned long)(sizeof(rgbd_group)*THIS->xsize*THIS->ysize),
           (unsigned long)(sizeof(rgb_group)*THIS->xsize*THIS->ysize+1));
#endif

   area=xalloc(sizeof(rgbd_group)*THIS->xsize*THIS->ysize+1);

   if (!(costbl=malloc(sizeof(double)*THIS->xsize+1)))
   {
      free(area);
      out_of_memory_error(NULL, -1, 0);
   }

   o=clone_object(image_program,0);
   img=(struct image*)(o->storage);
   *img=*THIS;
   img->xsize = x;
   img->ysize = y;

   if (!(img->img=malloc(sizeof(rgb_group)*
                         img->xsize*img->ysize+RGB_VEC_PAD)))
   {
      free(area);
      free(costbl);
      free_object(o);
      out_of_memory_error(NULL, -1, 0);
   }

   xsz2=THIS->xsize*2.0;
   ysz2=THIS->ysize*2.0;

   enh=(8.0/THIS->xsize)*(8.0/THIS->ysize);

   for (u=0; u<THIS->xsize; u++)
   {
      double d,z0;
      rgbd_group sum;

      for (v=0; v<THIS->ysize; v++)
      {
	 d=(u?1:c0)*(v?1:c0)/4.0;
	 sum.r=sum.g=sum.b=0;
	 pix=THIS->img;

	 for (x=0; x<THIS->xsize; x++)
	    costbl[x]=cos( (2*x+1)*u*pi/xsz2 );

	 for (y=0; y<THIS->ysize; y++)
	 {
	    z0=cos( (2*y+1)*v*pi/ysz2 );
	    for (x=0; x<THIS->xsize; x++)
	    {
	       double z;
	       z =  costbl[x] * z0;
	       sum.r += (float)(pix->r*z);
	       sum.g += (float)(pix->g*z);
	       sum.b += (float)(pix->b*z);
	       pix++;
	    }
	 }
	 sum.r *= (float)d;
	 sum.g *= (float)d;
	 sum.b *= (float)d;
	 area[u+v*THIS->xsize]=sum;
      }
#ifdef DCT_DEBUG
      fprintf(stderr,"."); fflush(stderr);
#endif
   }
#ifdef DCT_DEBUG
   fprintf(stderr,"\n");
#endif

   dx=((double)(THIS->xsize-1))/(img->xsize);
   dy=((double)(THIS->ysize-1))/(img->ysize);

   pix=img->img;
   for (y=0,yp=0; y<img->ysize; y++,yp+=dy)
   {
      double z0;
      rgbd_group sum;

      for (x=0,xp=0; x<img->xsize; x++,xp+=dx)
      {
	 sum.r=sum.g=sum.b=0;
	 val=area;

	 for (u=0; u<THIS->xsize; u++)
	    costbl[u]=cos( (2*xp+1)*u*pi/xsz2 );

	 for (v=0; v<THIS->ysize; v++)
	 {
	    z0=cos( (2*yp+1)*v*pi/ysz2 )*(v?1:c0)/4.0;
	    for (u=0; u<THIS->xsize; u++)
	    {
	       double z;
	       z = (u?1:c0) * costbl[u] * z0;
               sum.r += (float)(val->r*z);
               sum.g += (float)(val->g*z);
               sum.b += (float)(val->b*z);
	       val++;
	    }
	 }
	 sum.r *= (float)enh;
	 sum.g *= (float)enh;
	 sum.b *= (float)enh;
         pix->r=testrange((int)(sum.r+0.5));
         pix->g=testrange((int)(sum.g+0.5));
         pix->b=testrange((int)(sum.b+0.5));
	 pix++;
      }
#ifdef DCT_DEBUG
      fprintf(stderr,"."); fflush(stderr);
#endif
   }

   free(area);
   free(costbl);

   pop_n_elems(args);
   push_object(o);
}
