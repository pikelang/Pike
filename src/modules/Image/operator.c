/* $Id: operator.c,v 1.11 1998/01/13 22:59:23 hubbe Exp $ */

/*
**! module Image
**! note
**!	$Id: operator.c,v 1.11 1998/01/13 22:59:23 hubbe Exp $
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
#include "threads.h"

#include "image.h"

extern struct program *image_program;
#ifdef THIS
#undef THIS
#endif
#define THIS ((struct image *)(fp->current_storage))
#define THISOBJ (fp->current_object)

#define absdiff(a,b) ((a)<(b)?((b)-(a)):((a)-(b)))

#define STANDARD_OPERATOR_HEADER(what) \
   struct object *o;			   			\
   struct image *img,*oper;		   			\
   rgb_group *s1,*s2,*d,rgb;		   			\
   INT32 i;				   			\
					   			\
   if (!THIS->img) error("no image\n");	   			\
   		   			   			\
   if (args && sp[-args].type==T_INT)				\
   {								\
      rgb.r=sp[-args].u.integer;				\
      rgb.g=sp[-args].u.integer;				\
      rgb.b=sp[-args].u.integer;				\
      oper=NULL;						\
   }								\
   else if (args && sp[-args].type==T_ARRAY			\
       && sp[-args].u.array->size>=3				\
       && sp[-args].u.array->item[0].type==T_INT		\
       && sp[-args].u.array->item[1].type==T_INT		\
       && sp[-args].u.array->item[2].type==T_INT)		\
   {								\
      rgb.r=sp[-args].u.array->item[0].u.integer;		\
      rgb.g=sp[-args].u.array->item[1].u.integer;		\
      rgb.b=sp[-args].u.array->item[2].u.integer;		\
      oper=NULL;						\
   }								\
   else								\
   {								\
      if (args<1 || sp[-args].type!=T_OBJECT			\
       || !sp[-args].u.object					\
       || sp[-args].u.object->prog!=image_program)		\
      error("illegal arguments to image->"what"()\n");		\
   		   		   				\
      oper=(struct image*)sp[-args].u.object->storage;		\
      if (!oper->img) error("no image (operand)\n");		\
      if (oper->xsize!=THIS->xsize 		   		\
          || oper->ysize!=THIS->ysize) 		   		\
         error("operands differ in size (image->"what")");	\
   }  	   		   		   		        \
		   		   		   		\
   push_int(THIS->xsize);		   			\
   push_int(THIS->ysize);		   			\
   o=clone_object(image_program,2);				\
   img=(struct image*)o->storage;		   		\
   if (!img->img) { free_object(o); error("out of memory\n"); }	\
		   		   		   		\
   pop_n_elems(args);		   		   		\
   push_object(o);		   		   		\
   		   		   		   		\
   s1=THIS->img;		   		   		\
   if (oper) s2=oper->img; else s2=NULL;   		   	\
   d=img->img;		   		   			\
		   		   		   		\
   i=img->xsize*img->ysize;			   		\
   THREADS_ALLOW();                                             \
   if (oper)


/*
**! method object `-(object operand)
**! method object `-(array(int) color)
**! method object `-(int value)
**!	makes a new image out of the difference
**! returns the new image object
**!
**! arg object operand
**!	the other image to compare with;
**!	the images must have the same size.
**! arg array(int) color
**!	an array in format ({r,g,b}), this is equal
**!	to using an uniform-colored image.
**! arg int value
**!	equal to ({value,value,value}).
**! see also: `+, `|, `&, `*, add_layers
*/

void image_operator_minus(INT32 args)
{
STANDARD_OPERATOR_HEADER("`-")
   while (i--)
   {
      d->r=absdiff(s1->r,s2->r);
      d->g=absdiff(s1->g,s2->g);
      d->b=absdiff(s1->b,s2->b);
      s1++; s2++; d++;
   }
   else
   while (i--)
   {
      d->r=absdiff(s1->r,rgb.r);
      d->g=absdiff(s1->g,rgb.g);
      d->b=absdiff(s1->b,rgb.b);
      s1++; d++;
   }
   THREADS_DISALLOW();
}

/*
**! method object `+(object operand)
**! method object `+(array(int) color)
**! method object `+(int value)
**!	adds two images; values are truncated at 255.
**! returns the new image object
**!
**! arg object operand
**!	the image which to add.
**! arg array(int) color
**!	an array in format ({r,g,b}), this is equal
**!	to using an uniform-colored image.
**! arg int value
**!	equal to ({value,value,value}).
**! see also: `-, `|, `&, `*, add_layers
*/

void image_operator_plus(INT32 args)
{
STANDARD_OPERATOR_HEADER("`+")
   while (i--)
   {
      d->r=MINIMUM(s1->r+s2->r,255);
      d->g=MINIMUM(s1->g+s2->g,255);
      d->b=MINIMUM(s1->b+s2->b,255);
      s1++; s2++; d++; 
   }
   else
   while (i--)
   {
      d->r=MINIMUM(s1->r+rgb.r,255);
      d->g=MINIMUM(s1->g+rgb.g,255);
      d->b=MINIMUM(s1->b+rgb.b,255);
      s1++; d++;
   }
   THREADS_DISALLOW();
}

/*
**! method object `*(object operand)
**! method object `*(array(int) color)
**! method object `*(int value)
**!	Multiplies pixel values and creates a new image.
**! returns the new image object
**!
**!	This can be useful to lower the values of an image,
**!	making it greyer, for instance:
**!
**!	<pre>image=image*128+64;</pre>
**!
**! arg object operand
**!	the other image to multiply with;
**!	the images must have the same size.
**! arg array(int) color
**!	an array in format ({r,g,b}), this is equal
**!	to using an uniform-colored image.
**! arg int value
**!	equal to ({value,value,value}).
**!
**! see also: `-, `+, `|, `&, add_layers
*/

void image_operator_multiply(INT32 args)
{
   double q=1/255.0;
STANDARD_OPERATOR_HEADER("`*")
   while (i--)
   {
      d->r=floor(s1->r*q*s2->r+0.5);
      d->g=floor(s1->g*q*s2->g+0.5);
      d->b=floor(s1->b*q*s2->b+0.5);
      s1++; s2++; d++; 
   }
   else
   while (i--)
   {
      d->r=floor(s1->r*q*rgb.r+0.5);
      d->g=floor(s1->g*q*rgb.g+0.5);
      d->b=floor(s1->b*q*rgb.b+0.5);
      s1++; d++; 
   }
   THREADS_DISALLOW();
}

/*
**! method object `|(object operand)
**! method object `|(array(int) color)
**! method object `|(int value)
**!	makes a new image out of the maximum pixels values
**!	
**! returns the new image object
**!
**! arg object operand
**!	the other image to compare with;
**!	the images must have the same size.
**! arg array(int) color
**!	an array in format ({r,g,b}), this is equal
**!	to using an uniform-colored image.
**! arg int value
**!	equal to ({value,value,value}).
**! see also: `-, `+, `&, `*, add_layers
*/

void image_operator_maximum(INT32 args)
{
STANDARD_OPERATOR_HEADER("`| 'maximum'")
   while (i--)
   {
      d->r=MAXIMUM(s1->r,s2->r);
      d->g=MAXIMUM(s1->g,s2->g);
      d->b=MAXIMUM(s1->b,s2->b);
      s1++; s2++; d++; 
   }
   else
   while (i--)
   {
      d->r=MAXIMUM(s1->r,rgb.r);
      d->g=MAXIMUM(s1->g,rgb.g);
      d->b=MAXIMUM(s1->b,rgb.b);
      s1++; s2++; d++; 
   }
   THREADS_DISALLOW();
}

/*
**! method object `&(object operand)
**! method object `&(array(int) color)
**! method object `&(int value)
**!	makes a new image out of the minimum pixels values
**!	
**! returns the new image object
**!
**! arg object operand
**!	the other image to compare with;
**!	the images must have the same size.
**! arg array(int) color
**!	an array in format ({r,g,b}), this is equal
**!	to using an uniform-colored image.
**! arg int value
**!	equal to ({value,value,value}).
**! see also: `-, `+, `|, `*, add_layers
*/

void image_operator_minimum(INT32 args)
{
STANDARD_OPERATOR_HEADER("`& 'minimum'")
   while (i--)
   {
      d->r=MINIMUM(s1->r,s2->r);
      d->g=MINIMUM(s1->g,s2->g);
      d->b=MINIMUM(s1->b,s2->b);
      s1++; s2++; d++; 
   }
   else
   while (i--)
   {
      d->r=MINIMUM(s1->r,rgb.r);
      d->g=MINIMUM(s1->g,rgb.g);
      d->b=MINIMUM(s1->b,rgb.b);
      s1++; d++; 
   }
   THREADS_DISALLOW();
}


