/* $Id: dct.c,v 1.2 1997/03/10 21:33:36 grubba Exp $ */

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

extern struct program *image_program;
#define THIS ((struct image *)(fp->current_storage))
#define THISOBJ (fp->current_object)

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)<(b)?(b):(a))
#define testrange(x) max(min((x),255),0)

static const double c0=0.70710678118654752440;
static const double pi=3.14159265358979323846;

void image_dct(INT32 args)
{
   rgbd_group *area,*val;
   struct object *o;
   struct image *img;
   INT32 x,y,u,v;
   double xsz2,ysz2,enh,xp,yp,dx,dy;
   double *costbl;
   rgb_group *pix;
   
   if (!THIS->img) error("no image\n");

   fprintf(stderr,"%lu bytes, %lu bytes\n",
	   (unsigned long)(sizeof(rgbd_group)*THIS->xsize*THIS->ysize),
	   (unsigned long)(sizeof(rgb_group)*THIS->xsize*THIS->ysize+1));
    
   if (!(area=malloc(sizeof(rgbd_group)*THIS->xsize*THIS->ysize+1)))
      error("Out of memory\n");

   if (!(costbl=malloc(sizeof(double)*THIS->xsize+1)))
   {
      free(area);
      error("Out of memory\n");
   }

   o=clone(image_program,0);
   img=(struct image*)(o->storage);
   *img=*THIS;
   
   if (args>=2 
       && sp[-args].type==T_INT 
       && sp[1-args].type==T_INT)
   {
      img->xsize=max(1,sp[-args].u.integer);
      img->ysize=max(1,sp[1-args].u.integer);
   }
   else error("Illegal arguments to image->dct()\n");

   if (!(img->img=malloc(sizeof(rgb_group)*img->xsize*img->ysize+1)))
   {
      free(area);
      free(costbl);
      free_object(o);
      error("Out of memory\n");
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
	       sum.r+=pix->r*z;
	       sum.g+=pix->g*z;
	       sum.b+=pix->b*z;
	       pix++;
	    }
	 }
	 sum.r*=d;
	 sum.g*=d;
	 sum.b*=d;
	 area[u+v*THIS->xsize]=sum;
      }
      fprintf(stderr,"."); fflush(stderr);
   }
   fprintf(stderr,"\n");

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
	       sum.r+=val->r*z;
	       sum.g+=val->g*z;
	       sum.b+=val->b*z;
	       val++;
	    }
	 }
	 sum.r*=enh;
	 sum.g*=enh;
	 sum.b*=enh;
	 pix->r=testrange(((int)(sum.r+0.5)));
	 pix->g=testrange(((int)(sum.g+0.5)));
	 pix->b=testrange(((int)(sum.b+0.5)));
	 pix++;
      }
      fprintf(stderr,"."); fflush(stderr);
   }

   free(area);
   free(costbl);

   pop_n_elems(args);
   push_object(o);
}
