/* $Id: operator.c,v 1.4 1997/03/25 06:13:48 mirar Exp $ */
#include "global.h"

#include <math.h>
#include <ctype.h>

#include "stralloc.h"
#include "global.h"
#include "types.h"
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
#define THIS ((struct image *)(fp->current_storage))
#define THISOBJ (fp->current_object)

#define absdiff(a,b) ((a)<(b)?((b)-(a)):((a)-(b)))
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)<(b)?(b):(a))

#define STANDARD_OPERATOR_HEADER(what) \
   struct object *o;			   			\
   struct image *img,*oper;		   			\
   rgb_group *s1,*s2,*d,rgb;		   			\
   INT32 i;				   			\
					   			\
   if (!THIS->img) error("no image\n");	   			\
   		   			   			\
   if (args && sp[-args].type==T_ARRAY				\
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

void image_operator_plus(INT32 args)
{
STANDARD_OPERATOR_HEADER("'+")
   while (i--)
   {
      d->r=min(s1->r+s2->r,255);
      d->g=min(s1->g+s2->g,255);
      d->b=min(s1->b+s2->b,255);
      s1++; s2++; d++; 
   }
   else
   while (i--)
   {
      d->r=min(s1->r+rgb.r,255);
      d->g=min(s1->g+rgb.g,255);
      d->b=min(s1->b+rgb.b,255);
      s1++; d++;
   }
   THREADS_DISALLOW();
}

void image_operator_multiply(INT32 args)
{
   double q=1/255.0;
STANDARD_OPERATOR_HEADER("'+")
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
   else
   while (i--)
   {
      d->r=max(s1->r,rgb.r);
      d->g=max(s1->g,rgb.g);
      d->b=max(s1->b,rgb.b);
      s1++; s2++; d++; 
   }
   THREADS_DISALLOW();
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
   else
   while (i--)
   {
      d->r=min(s1->r,rgb.r);
      d->g=min(s1->g,rgb.g);
      d->b=min(s1->b,rgb.b);
      s1++; d++; 
   }
   THREADS_DISALLOW();
}


