/* -*- mode: C; c-basic-offset: 3; -*-
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
#include "threads.h"
#include "pike_error.h"

#include "image.h"

#include <builtin_functions.h>


#define sp Pike_sp

extern struct program *image_program;
#ifdef THIS
#undef THIS /* Needed for NT */
#endif
#define THIS ((struct image *)(Pike_fp->current_storage))
#define THISOBJ (Pike_fp->current_object)

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
**! method object orient(void|array(object) something)
**! method array(object) orient4()
**!	Draws images describing the orientation
**!     of the current image.
**!
**!	<tt>orient</tt> gives an HSV image
**!	(run a <ref>hsv_to_rgb</ref> pass on it
**!	to get a viewable image).
**!     corresponding to the angle of the
**!	orientation:
**!     <pre>      |      /    -    \
**!          hue=  0     64   128  192  (=red in an hsv image)
**!              purple cyan green red
**!     </pre>
**!	Red, green and blue channels are added
**!	and not compared separately.
**!
**!     If you first use orient4 you can give its
**!     output as input to this function.
**!
**!     The <tt>orient4</tt> function gives back
**!	4 image objects, corresponding to the
**!	amount of different directions, see above.
**!
**! returns an image or an array of the four new image objects
**!
**! note
**!	experimental status; may not be exact the same
**!	output in later versions
*/
static inline int my_abs(int a) { return (a<0)?-a:a; }

static void _image_orient(struct image *source,
			  struct object *o[5],
			  struct image *img[5])
{
   int i;
   struct { int x,y; } or[4]={ {1,0}, {1,1}, {0,1}, {-1,1} };
   int x,y;

   for (i=0; i<5; i++)
   {
      push_int(source->xsize);
      push_int(source->ysize);
      o[i]=clone_object(image_program,2);
      img[i]=get_storage(o[i],image_program);
      push_object(o[i]);
   }

THREADS_ALLOW();
CHRONO("start");
   for (i=0; i<4; i++) /* four directions */
   {
      rgb_group *d=img[i]->img;
      rgb_group *s=source->img;
      int xz=source->xsize;
      int yz=source->ysize;
      int xd=or[i].x;
      int yd=or[i].y;

      for(x=1; x<xz-1; x++)
	 for(y=1; y<yz-1; y++)
	 {
#define FOOBAR(CO) \
  d[x+y*xz].CO \
     = \
  (COLORTYPE) \
     my_abs( s[(x+xd)+(y+yd)*xz].CO - s[(x-xd)+(y-yd)*xz].CO )

	    FOOBAR(r);
	    FOOBAR(g);
	    FOOBAR(b);

#undef FOOBAR
	 }
   }
CHRONO("end");
THREADS_DISALLOW();
}

void image_orient(INT32 args)
{
  struct object *o[5];
  struct image *img[5],*this,*img1;
  int n;
  rgb_group *d,*s1,*s2,*s3,*s0;
  double mag;
  int i, w, h;

  CHECK_INIT();

  this=THIS;

  if (args)
  {
    if (TYPEOF(sp[-args]) == T_INT)
      mag=sp[-args].u.integer;
    else if (TYPEOF(sp[-args]) == T_FLOAT)
      mag=sp[-args].u.float_number;
    else {
      SIMPLE_ARG_TYPE_ERROR("orient",1,"int|float");
      UNREACHABLE();
    }
  }
  else mag=1.0;

  if (args==1)
    pop_n_elems(args);

  if (args>1)
  {
    if (TYPEOF(sp[1-args]) != T_ARRAY)
      SIMPLE_ARG_TYPE_ERROR("orient",2,"array");
    if (sp[1-args].u.array->size!=4)
      Pike_error("The array given as argument 2 to orient do not have size 4\n");
    for(i=0; i<4; i++)
      if ((TYPEOF(sp[1-args].u.array->item[i]) != T_OBJECT) ||
	  (!(sp[1-args].u.array->item[i].u.object)) ||
	  (sp[1-args].u.array->item[i].u.object->prog!=image_program))
	Pike_error("The array given as argument 2 to orient do not contain images\n");
    img1=(struct image*)sp[1-args].u.array->item[0].u.object->storage;

    w=this->xsize;
    h=this->ysize;

    for(i=0; i<4; i++)
    {
      img1=(struct image*)sp[1-args].u.array->item[i].u.object->storage;
      if ((img1->xsize!=w)||
	  (img1->ysize!=h))
	Pike_error("The images in the array given as argument 2 to orient have different sizes\n");
    }
    for(i=0; i<4; i++)
        img[i]=get_storage(sp[1-args].u.array->item[i].u.object,image_program);
    pop_n_elems(args);
    push_int(this->xsize);
    push_int(this->ysize);
    o[4]=clone_object(image_program,2);
    img[4]=get_storage(o[4],image_program);
    push_object(o[4]);
    w=1;
  }
  else
  {
    _image_orient(this,o,img);
    w=0;
  }
  s0=img[0]->img;
  s1=img[1]->img;
  s2=img[2]->img;
  s3=img[3]->img;
  d=img[4]->img;

THREADS_ALLOW();
CHRONO("begin hsv...");
  n=this->xsize*this->ysize;
  while (n--)
  {
     /* F�rsta f�rg, sista m�rkhet */
     double j=(s0->r+s0->g+s0->b-s2->r-s2->g-s2->b)/3.0;
                /* riktning - - riktning | */

     double h=(s1->r+s1->g+s1->b-s3->r-s3->g-s3->b)/3.0;
                /* riktning \ - riktning / */

     int z,w;

     if (my_abs((int)h) > my_abs((int)j))
	if (h) {
          z = -(int)(32*(j/h)+(h>0)*128+64);
          w = my_abs((int)h);
	}
	else z=0,w=0;
     else {
	if (j) {
          z = -(int)(-32*(h/j)+(j>0)*128+128);
          w = my_abs((int)j);
	}
	else z=0,w=0;
     }

     d->r=(COLORTYPE)z;
     d->g=255;
     d->b = MINIMUM((COLORTYPE)(w*mag), 255);

     d++;
     s0++;
     s1++;
     s2++;
     s3++;
  }
CHRONO("end hsv...");
THREADS_DISALLOW();

  if (!w)
  {
    add_ref(o[4]);
    pop_n_elems(5);
    push_object(o[4]);
  }
}


void image_orient4(INT32 args)
{
  struct object *o[5];
  struct image *img[5];

  CHECK_INIT();

  pop_n_elems(args);
  _image_orient(THIS,o,img);

  pop_n_elems(1);
  f_aggregate(4);
}
