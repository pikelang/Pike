/* $Id: orient.c,v 1.2 1998/02/15 15:50:10 hedda Exp $ */

/*
**! module Image
**! note
**!	$Id: orient.c,v 1.2 1998/02/15 15:50:10 hedda Exp $
**! class image
*/

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

#include <builtin_functions.h>

extern struct program *image_program;
#ifdef THIS
#undef THIS /* Needed for NT */
#endif
#define THIS ((struct image *)(fp->current_storage))
#define THISOBJ (fp->current_object)

#define testrange(x) MAXIMUM(MINIMUM((x),255),0)

static const double c0=0.70710678118654752440;
static const double PI=3.14159265358979323846;

/*
**! method object orient( foo bar args?)
**!	Draws four images describing the orientation.
**!	
**!	
**!	
**!
**!	
**!	
**!	
**!
**!	
**!	
**!
**!	
**!	
**!
**!
**!
**!
**! returns an array of four new image objects
**! arg int newx
**! arg int newy
**!	new image size in pixels
**!
*/
static INLINE int sq(int a) { return a*a; }

void image_orient(INT32 args)
{
  int x,y;
  int h,j;
  struct object *o1;
  struct object *o2;
  struct object *o3;
  struct object *o4;
  struct object *o5;
  struct image *o1img;
  struct image *o2img;
  struct image *o3img;
  struct image *o4img;
  struct image *o5img;
  
  if (!THIS->img) { error("no image\n");  return; }
  
  push_int(THIS->xsize);
  push_int(THIS->ysize);
  o1=clone_object(image_program,2);
  o1img=(struct image*)get_storage(o1,image_program);

  push_int(THIS->xsize);
  push_int(THIS->ysize);
  o2=clone_object(image_program,2);
  o2img=(struct image*)get_storage(o2,image_program);

  push_int(THIS->xsize);
  push_int(THIS->ysize);
  o3=clone_object(image_program,2);
  o3img=(struct image*)get_storage(o3,image_program);

  push_int(THIS->xsize);
  push_int(THIS->ysize);
  o4=clone_object(image_program,2);
  o4img=(struct image*)get_storage(o4,image_program);

  push_int(THIS->xsize);
  push_int(THIS->ysize);
  o5=clone_object(image_program,2);
  o5img=(struct image*)get_storage(o5,image_program);

#define A o1img
#define B THIS



#define FOOBAR(CO,xd,yd) \
	A->img[x+y*B->xsize].CO= \
	   sqrt((sq((B->img[(x-xd)+(y-yd)*B->xsize].CO-B-> \
                img[x+y*B->xsize].CO ))+ \
		sq((B->img[x+y*B->xsize].CO- \
		B->img[(x+xd)+(y+yd)*B->xsize].CO)) \
		     )/2.0); 

  //Create image 1
  for(x=1; x<A->xsize-1; x++)
    for(y=1; y<A->ysize-1; y++)
      {
	FOOBAR(r,1,0);
	FOOBAR(g,1,0);
	FOOBAR(b,1,0);
      }

#undef A
#define A o2img

  //Create image 2
  for(x=1; x<A->xsize-1; x++)
    for(y=1; y<A->ysize-1; y++)
      {
	FOOBAR(r,1,1);
	FOOBAR(g,1,1);
	FOOBAR(b,1,1);
      }


#undef A
#define A o3img

  //Create image 3
  for(x=1; x<A->xsize-1; x++)
    for(y=1; y<A->ysize-1; y++)
      {
	FOOBAR(r,0,1);
	FOOBAR(g,0,1);
	FOOBAR(b,0,1);
      }

#undef A
#define A o4img

  //Create image 4
  for(x=1; x<A->xsize-1; x++)
    for(y=1; y<A->ysize-1; y++)
      {
	FOOBAR(r,-1,1);
	FOOBAR(g,-1,1);
	FOOBAR(b,-1,1);
      }

#undef A
#define A o5img

  //Create image 5, the hsv-thing...
  for(x=1; x<A->xsize-1; x++)
    for(y=1; y<A->ysize-1; y++)
      {
	//Första färg, sista mörkhet
	j=o1img->img[x+y*B->xsize].r+
	  o1img->img[x+y*B->xsize].g+
	  o1img->img[x+y*B->xsize].b-
	  o3img->img[x+y*B->xsize].r-
	  o3img->img[x+y*B->xsize].g-
	  o3img->img[x+y*B->xsize].b;
	
	h=o2img->img[x+y*B->xsize].r+
	  o2img->img[x+y*B->xsize].g+
	  o2img->img[x+y*B->xsize].b-
	  o4img->img[x+y*B->xsize].r-
	  o4img->img[x+y*B->xsize].g-
	  o4img->img[x+y*B->xsize].b;
	
	if (h>0)
	  if (j>0)
	    A->img[x+y*B->xsize].r=(int)(0.5+(255/(2*PI))*
					 atan((float)h/(float)j));
	  else
	    if (j<0)
	      A->img[x+y*B->xsize].r=(int)(0.5+(255/(2*PI))*
					   (PI+atan((float)h/(float)j)));
	    else
	      A->img[x+y*B->xsize].r=255/4;
	else
	  if (h<0)
	    if (j>0)
	      A->img[x+y*B->xsize].r=(int)(0.5+(255/(2*PI))*
					   (2*PI+atan((float)h/(float)j)));
	    else
	      if (j<0)
		A->img[x+y*B->xsize].r=(int)(0.5+(255/(2*PI))*
					     (PI+atan((float)h/(float)j)));
	      else
		A->img[x+y*B->xsize].r=(3*255)/4;
	  else
	    if (j<0)
	      A->img[x+y*B->xsize].r=255/2;
	    else
	      A->img[x+y*B->xsize].r=0;
	
	
	A->img[x+y*B->xsize].g=255;

	A->img[x+y*B->xsize].b=MINIMUM(255, sqrt((sq(j)+sq(h))));

      }




  //Och så fylla ut de andra bilderna med lite junk.

  pop_n_elems(args);
  push_object(o1);
  push_object(o2);
  push_object(o3);
  push_object(o4);
  push_object(o5);
  
  f_aggregate(5);
}
