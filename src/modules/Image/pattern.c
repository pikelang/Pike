/* $Id: pattern.c,v 1.4 1997/05/19 22:50:26 hubbe Exp $ */

#include "global.h"

#include <math.h>
#include <ctype.h>

#include "stralloc.h"
#include "global.h"
#include "pike_macros.h"
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

/**************** noise ************************/

#define NOISE_PTS 512
#define NOISE_PX 173
#define NOISE_PY 263
#define NOISE_PZ 337
#define NOISE_PHI 0.6180339
static unsigned short noise_p1[NOISE_PTS],noise_p2[NOISE_PTS];

#define COLORRANGE_LEVELS 1024

#define FRAC(X) ((X)-floor(X))

static INLINE double noise(double Vx,double Vy,unsigned short *noise_p)
{
   int Ax[3],Ay[3];
   int n,i,j;
   double Sx[3],Sy[3];
   double sum,dsum,f,fx,fy;

   fx=floor(Vx);
   fy=floor(Vy);
   
   for (n=0; n<3; n++) 
   {
      Ax[n]=(int)floor(NOISE_PX*FRAC( (fx+n)*NOISE_PHI ));
      Ay[n]=(int)floor(NOISE_PY*FRAC( (fy+n)*NOISE_PHI ));
   }
   
   f=FRAC(Vx);
   Sx[0]=0.5-f+0.5*f*f;
   Sx[1]=0.5+f-f*f;
   Sx[2]=0.5*f*f;

   f=FRAC(Vy);
   Sy[0]=0.5-f+0.5*f*f;
   Sy[1]=0.5+f-f*f;
   Sy[2]=0.5*f*f;

   sum=0;
   for (i=0; i<3; i++)
   {
      for (j=0,dsum=0; j<3; j++)
	 dsum+=Sy[j]*noise_p[ (Ax[i]+Ay[j]) & (NOISE_PTS-1) ];
      sum+=Sx[i]*dsum;
   }
   return sum;
}

static INLINE double turbulence(double x,double y,int octaves)
{
   double t=0;
   double mul=1;
   while (octaves-->0)
   {
      t+=noise(x*mul,y*mul,noise_p1)*mul;
      mul*=0.5;
   }
   return t;
}

static void init_colorrange(rgb_group *cr,struct svalue *s,char *where)
{
   float *v,*vp;
   int i,n,k;
   struct svalue s2,s3;
   rgbd_group lrgb,*rgbp,*rgb;
   float fr,fg,fb,q;
   int b;

   if (s->type!=T_ARRAY)
      error("Illegal colorrange to %s\n",where);
   else if (s->u.array->size<2)
      error("Colorrange array too small (meaningless) (to %s)\n",where);

   s2.type=T_INT;
   s3.type=T_INT; /* don't free these */

   vp=v=(void*)xalloc(sizeof(float)*(s->u.array->size/2+1));
   rgbp=rgb=(void*)xalloc(sizeof(rgbd_group)*(s->u.array->size/2+1));

   for (i=0; i<s->u.array->size-1; i+=2)
   {
      array_index(&s2,s->u.array,i);
      if (s2.type==T_INT) *vp=s2.u.integer;
      else if (s2.type==T_FLOAT) *vp=s2.u.float_number;
      else *vp=0;
      if (*vp>1) *vp=1;
      else if (*vp<0) *vp=0;
      vp++;

      array_index(&s2,s->u.array,i+1);
      
      if (s2.type==T_INT)
	 rgbp->r=rgbp->g=rgbp->b=testrange( s2.u.integer );
      else if ( s2.type==T_ARRAY
		&& s2.u.array->size>=3 )
      {
	 array_index(&s3,s2.u.array,0);
	 if (s3.type==T_INT) rgbp->r=testrange( s3.u.integer );
	 else rgbp->r=0;
	 array_index(&s3,s2.u.array,1);
	 if (s3.type==T_INT) rgbp->g=testrange( s3.u.integer );
	 else rgbp->g=0;
	 array_index(&s3,s2.u.array,2);
	 if (s3.type==T_INT) rgbp->b=testrange( s3.u.integer );
	 else rgbp->b=0;
      }
      else
	 rgbp->r=rgbp->g=rgbp->b=0;
      rgbp++;
   }
   *vp=v[0]+1+1.0/(COLORRANGE_LEVELS-1);
   lrgb=*rgbp=rgb[0]; /* back to original color */

   for (k=1,i=v[0]*(COLORRANGE_LEVELS-1); k<=s->u.array->size/2; k++)
   {
      n=v[k]*(COLORRANGE_LEVELS-1);

      if (n>i)
      {
	 q=1/((float)(n-i));
   
	 fr=(rgb[k].r-lrgb.r)*q;
	 fg=(rgb[k].g-lrgb.g)*q;
	 fb=(rgb[k].b-lrgb.b)*q;

	 for (b=0;i<n;i++,b++)
	 {
	    cr[i&(COLORRANGE_LEVELS-1)].r=(unsigned char)(lrgb.r+fr*b);
	    cr[i&(COLORRANGE_LEVELS-1)].g=(unsigned char)(lrgb.g+fg*b);
	    cr[i&(COLORRANGE_LEVELS-1)].b=(unsigned char)(lrgb.b+fb*b);
	 }
      }
      lrgb=rgb[k];
   }

   free_svalue(&s3);
   free_svalue(&s2);
   free(v);
   free(rgb);
}

#define GET_FLOAT_ARG(sp,args,n,def,where) \
   ( (args>n) \
      ? ( (sp[n-args].type==T_INT) ? (double)(sp[n-args].u.integer) \
	  : ( (sp[n-args].type==T_FLOAT) ? sp[n-args].u.float_number \
	      : ( error("illegal argument(s) to "where"\n"), 0.0 ) ) ) \
      : def )
#define GET_INT_ARG(sp,args,n,def,where) \
   ( (args>n) \
      ? ( (sp[n-args].type==T_INT) ? sp[n-args].u.integer \
	  : ( (sp[n-args].type==T_FLOAT) ? (int)(sp[n-args].u.float_number) \
	      : ( error("illegal argument(s) to "where"\n"), 0.0 ) ) ) \
      : def )

void image_noise(INT32 args)
{
/* parametrar: 	array(float|int|array(int)) colorrange,
                float scale=0.1,
		float xdiff=0,
		float ydiff=0,
   		float cscale=1
*/
   int x,y;
   rgb_group cr[COLORRANGE_LEVELS];
   double scale,xdiff,ydiff,cscale,xp,yp;
   rgb_group *d;
   struct object *o;
   struct image *img;

   if (!THIS->img) error("no image\n");

   if (args<1) error("too few arguments to image->noise()\n");

   scale=GET_FLOAT_ARG(sp,args,1,0.1,"image->noise");
   xdiff=GET_FLOAT_ARG(sp,args,2,0,"image->noise");
   ydiff=GET_FLOAT_ARG(sp,args,3,0,"image->noise");
   cscale=GET_FLOAT_ARG(sp,args,4,1,"image->noise");

   init_colorrange(cr,sp-args,"image->noise()");

   o=clone_object(image_program,0);
   img=(struct image*)o->storage;
   *img=*THIS;
   if (!(img->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize+1)))
   {
      free_object(o);
      error("Out of memory\n");
   }

   cscale=(32768*cscale)/COLORRANGE_LEVELS;

   d=img->img;
   for (y=THIS->ysize,xp=xdiff; y--; xp+=1.0)
      for (x=THIS->xsize,yp=ydiff; x--; yp+=1.0)
      {
	 *(d++)=
	    cr[(int)((noise((double)x*scale,(double)y*scale,noise_p1)
		      +noise((double)(x*0.5+y*0.8660254037844386)*scale,
			     (double)(-y*0.5+x*0.8660254037844386)*scale,
			     noise_p2))
		     *cscale)&(COLORRANGE_LEVELS-1)];
      }

   pop_n_elems(args);
   push_object(o);
}

void image_turbulence(INT32 args)
{
/* parametrar: 	array(float|int|array(int)) colorrange,
		int octaves=3,
                float scale=0.1,
		float xdiff=0,
		float ydiff=0,
   		float cscale=0.001
*/
   int x,y,octaves;
   rgb_group cr[COLORRANGE_LEVELS];
   double scale,xdiff,ydiff,cscale,xp,yp;
   rgb_group *d;
   struct object *o;
   struct image *img;

   if (!THIS->img) error("no image\n");

   if (args<1) error("too few arguments to image->turbulence()\n");

   octaves=GET_INT_ARG(sp,args,1,3,"image->turbulence");
   scale=GET_FLOAT_ARG(sp,args,2,0.1,"image->turbulence");
   xdiff=GET_FLOAT_ARG(sp,args,3,0,"image->turbulence");
   ydiff=GET_FLOAT_ARG(sp,args,4,0,"image->turbulence");
   cscale=GET_FLOAT_ARG(sp,args,5,0.001,"image->turbulence");

   init_colorrange(cr,sp-args,"image->turbulence()");

   o=clone_object(image_program,0);
   img=(struct image*)o->storage;
   *img=*THIS;
   if (!(img->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize+1)))
   {
      free_object(o);
      error("Out of memory\n");
   }

   cscale=(32768*cscale)/COLORRANGE_LEVELS;

   d=img->img;
   for (y=THIS->ysize,xp=xdiff; y--; xp+=1.0)
      for (x=THIS->xsize,yp=ydiff; x--; yp+=1.0)
      {
#if 0
	 if (y==0 && x<10)
	 {
	    fprintf(stderr,"%g*%g=%d => %d\n",
		    turbulence(xp*scale,yp*scale,octaves),
		    cscale,
		    (INT32)(turbulence(xp*scale,yp*scale,octaves)*cscale),
		    (INT32)(turbulence(xp*scale,yp*scale,octaves)*cscale)&(COLORRANGE_LEVELS)-1 );
	 }
#endif
	 *(d++)=
	    cr[(INT32)(turbulence(xp*scale,yp*scale,octaves)*cscale)
	       & (COLORRANGE_LEVELS-1)];
      }

   pop_n_elems(args);
   push_object(o);
}


void image_noise_init(void)
{
   int n;
   for (n=0; n<NOISE_PTS; n++)
   {
      noise_p1[n]=(unsigned short)(rand()&32767);
      noise_p2[n]=(unsigned short)(rand()&32767);
   }
}
