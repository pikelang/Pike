/* $Id: operator.c,v 1.2 1996/12/10 00:40:07 law Exp $ */
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

struct program *image_program;
#define THIS ((struct image *)(fp->current_storage))
#define THISOBJ (fp->current_object)

#define absdiff(a,b) ((a)<(b)?((b)-(a)):((a)-(b)))
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)<(b)?(b):(a))

#define STANDARD_OPERATOR_HEADER(what) \
   struct object *o;			   			\
   struct image *img,*oper;		   			\
   rgb_group *s1,*s2,*d;		   			\
   INT32 i;				   			\
					   			\
   if (!THIS->img) error("no image\n");	   			\
   		   			   			\
   if (args<1 || sp[-args].type!=T_OBJECT			\
       || !sp[-args].u.object					\
       || sp[-args].u.object->prog!=image_program)		\
      error("illegal arguments to image->"what"()\n");		\
   		   		   				\
   oper=(struct image*)sp[-args].u.object->storage;		\
   if (!oper->img) error("no image (operand)\n");		\
   if (oper->xsize!=THIS->xsize 		   		\
       || oper->ysize!=THIS->ysize) 		   		\
      error("operands differ in size (image->"what")");		\
		   		   		   		\
   push_int(THIS->xsize);		   			\
   push_int(THIS->ysize);		   			\
   o=clone(image_program,2);		   			\
   img=(struct image*)o->storage;		   		\
   if (!img->img) { free_object(o); error("out of memory\n"); }	\
		   		   		   		\
   pop_n_elems(args);		   		   		\
   push_object(o);		   		   		\
   		   		   		   		\
   s1=THIS->img;		   		   		\
   s2=oper->img;		   		   		\
   d=img->img;		   		   			\
		   		   		   		\
   i=img->xsize*img->ysize;		


void image_operator_minus(INT32 args)
{
STANDARD_OPERATOR_HEADER("'-")
   while (i--)
   {
      d->r=absdiff(s1->r,s2->r);
      d->g=absdiff(s1->g,s2->g);
      d->b=absdiff(s1->b,s2->b);
      s1++; s2++; d++;
   }
}

void image_operator_plus(INT32 args)
{
STANDARD_OPERATOR_HEADER("'+")
   while (i--)
   {
      d->r=max(s1->r+s2->r,255);
      d->g=max(s1->g+s2->g,255);
      d->b=max(s1->b+s2->b,255);
      s1++; s2++; d++; 
   }
}

void image_operator_multiply(INT32 args)
{
   double q=1/255.0;
STANDARD_OPERATOR_HEADER("'+")
   while (i--)
   {
      d->r=floor(s1->r*s2->r*q+0.5);
      d->g=floor(s1->g*s2->g*q+0.5);
      d->b=floor(s1->b*s2->b*q+0.5);
      s1++; s2++; d++; 
   }
}

void image_operator_maximum(INT32 args)
{
STANDARD_OPERATOR_HEADER("'| 'maximum'")
   while (i--)
   {
      d->r=max(s1->r,s2->r);
      d->g=max(s1->g,s2->g);
      d->b=max(s1->b,s2->b);
      s1++; s2++; d++; 
   }
}

void image_operator_minimum(INT32 args)
{
STANDARD_OPERATOR_HEADER("'& 'minimum'")
   while (i--)
   {
      d->r=min(s1->r,s2->r);
      d->g=min(s1->g,s2->g);
      d->b=min(s1->b,s2->b);
      s1++; s2++; d++; 
   }
}


