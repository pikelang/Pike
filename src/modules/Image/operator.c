/* $Id: operator.c,v 1.17 1998/04/18 17:16:23 mirar Exp $ */

/*
**! module Image
**! note
**!	$Id: operator.c,v 1.17 1998/04/18 17:16:23 mirar Exp $
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
#include "builtin_functions.h"

#include "image.h"

extern struct program *image_program;
#ifdef THIS
#undef THIS
#endif
#define THIS ((struct image *)(fp->current_storage))
#define THISOBJ (fp->current_object)

#define absdiff(a,b) ((a)<(b)?((b)-(a)):((a)-(b)))

#define testrange(x) ((COLORTYPE)MAXIMUM(MINIMUM(((int)x),255),0))

#define STANDARD_OPERATOR_HEADER(what) \
   struct object *o;			   			\
   struct image *img,*oper;		   			\
   rgb_group *s1,*s2,*d;		   			\
   rgbl_group rgb;                                              \
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
   if (args && sp[-args].type==T_FLOAT)				\
   {								\
      rgb.r=(long)(255*sp[-args].u.float_number);		\
      rgb.g=(long)(255*sp[-args].u.float_number);		\
      rgb.b=(long)(255*sp[-args].u.float_number);		\
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
   else if (args && sp[-args].type==T_ARRAY                     \
       && sp[-args].u.array->size>=3                            \
       && sp[-args].u.array->item[0].type==T_FLOAT              \
       && sp[-args].u.array->item[1].type==T_FLOAT              \
       && sp[-args].u.array->item[2].type==T_FLOAT)             \
   {                                                            \
      rgb.r=(long)(sp[-args].u.array->item[0].u.float_number*255); \
      rgb.g=(long)(sp[-args].u.array->item[1].u.float_number*255); \
      rgb.b=(long)(sp[-args].u.array->item[2].u.float_number*255); \
      oper=NULL;                                                \
   }                                                            \
   else                                                         \
   {                                                            \
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
   s1=THIS->img;		   		   		\
   if (oper) s2=oper->img; else s2=NULL;   		   	\
   d=img->img;		   		   			\
		   		   		   		\
   i=img->xsize*img->ysize;			   		\
   THREADS_ALLOW();                                             \
   if (s2)


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
**! see also: `+, `|, `&, `*, add_layers, min, max, `==
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
      d->r=MAXIMUM(absdiff(s1->r,rgb.r),255);
      d->g=MAXIMUM(absdiff(s1->g,rgb.g),255);
      d->b=MAXIMUM(absdiff(s1->b,rgb.b),255);
      s1++; d++;
   }
   THREADS_DISALLOW();
   pop_n_elems(args);		   		   		
   push_object(o);		   		   		
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
   pop_n_elems(args);		   		   		
   push_object(o);		   		   		
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
      d->r=testrange(floor(s1->r*q*rgb.r+0.5));
      d->g=testrange(floor(s1->g*q*rgb.g+0.5));
      d->b=testrange(floor(s1->b*q*rgb.b+0.5));
      s1++; d++; 
   }
   THREADS_DISALLOW();
   pop_n_elems(args);		   		   		
   push_object(o);		   		   		
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
   pop_n_elems(args);		   		   		
   push_object(o);		   		   		
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
   pop_n_elems(args);		   		   		
   push_object(o);		   		   		
}


/*
**! method int `==(object operand)
**! method int `==(array(int) color)
**! method int `==(int value)
**! method int `&lt;(object operand)
**! method int `&lt;(array(int) color)
**! method int `&lt;(int value)
**! method int `>(object operand)
**! method int `>(array(int) color)
**! method int `>(int value)
**!	Compares an image with another image or a color.
**!
**!	Comparision is strict and on pixel-by-pixel basis. 
**!	(Means if not all pixel r,g,b values are 
**!	correct compared with the corresponding
**!	pixel values, 0 is returned.)
**!
**! returns true (1) or false (0).
**!
**! arg object operand
**!	the other image to compare with;
**!	the images must have the same size.
**! arg array(int) color
**!	an array in format ({r,g,b}), this is equal
**!	to using an uniform-colored image.
**! arg int value
**!	equal to ({value,value,value}).
**!
**! see also: `-, `+, `|, `*, `&
**!	
**! note:
**!	`&lt; or `> on empty ("no image") image objects or images
**!	with different size will result in an error. 
**!	`== is always true on two empty image objects and
**!	always false if one and only one of the image objects
**!	is empty or the images differs in size.
**!
**!	a>=b and a&lt;=b between objects is equal to !(a&lt;b) and !(a>b),
**!	which may not be what you want (since both &lt; and > can return
**!	false, comparing the same images).
*/

void image_operator_equal(INT32 args)
{
   struct image *oper;
   rgb_group *s1,*s2,rgb;
   INT32 i;
   int res=1;

   if (args && sp[-args].type==T_INT)
   {
      rgb.r=sp[-args].u.integer;
      rgb.g=sp[-args].u.integer;
      rgb.b=sp[-args].u.integer;
      oper=NULL;
      if (!THIS->img)
      {
	 pop_n_elems(args);
	 push_int(1); /* no image has all colors */
	 return;
      }
   }
   else if (args && sp[-args].type==T_ARRAY
       && sp[-args].u.array->size>=3
       && sp[-args].u.array->item[0].type==T_INT
       && sp[-args].u.array->item[1].type==T_INT
       && sp[-args].u.array->item[2].type==T_INT)
   {
      rgb.r=sp[-args].u.array->item[0].u.integer;
      rgb.g=sp[-args].u.array->item[1].u.integer;
      rgb.b=sp[-args].u.array->item[2].u.integer;
      oper=NULL;
      if (!THIS->img)
      {
	 pop_n_elems(args);
	 push_int(1); /* no image has all colors */
	 return;
      }
   }
   else
   {
      if (args<1 || sp[-args].type!=T_OBJECT
       || !sp[-args].u.object
       || !(oper=(struct image*)get_storage(sp[-args].u.object,image_program)))
	 error("`==: illegal argument 2\n");

      if (!oper->img || !THIS->img)
      {
	 pop_n_elems(args);
	 push_int(oper->img == THIS->img); /* 1 if NULL == NULL */
	 return;
      }
      if (oper->xsize!=THIS->xsize
          || oper->ysize!=THIS->ysize)
      {
	 pop_n_elems(args);
	 push_int(0);
	 return;
      }
   }

   s1=THIS->img;
   if (oper) s2=oper->img; else s2=NULL;

   if (s1==s2)
   {
      pop_n_elems(args);
fprintf(stderr,"ok 1\n");
      push_int(1);
      return; /* same image is equal */
   }

   i=THIS->xsize*THIS->ysize;
   THREADS_ALLOW();
   if (s2)
      while (i--)
      {
	 if (s1->r!=s2->r || s1->g!=s2->g || s1->b!=s2->b) { res=0; break; }
	 s1++; s2++;
      }
   else
      while (i--)
      {
	 if (s1->r!=rgb.r || s1->g!=rgb.g || s1->b!=rgb.b) { res=0; break; }
	 s1++;
      }
   THREADS_DISALLOW();
   pop_n_elems(args);
   push_int(res);
}

void image_operator_lesser(INT32 args)
{
   struct image *oper;
   rgb_group *s1,*s2,rgb;
   INT32 i;
   int res=1;

   if (!THIS->img)
      error("image->`<: operator 1 has no image\n");

   if (args && sp[-args].type==T_INT)
   {
      rgb.r=sp[-args].u.integer;
      rgb.g=sp[-args].u.integer;
      rgb.b=sp[-args].u.integer;
      oper=NULL;
   }
   else if (args && sp[-args].type==T_ARRAY
       && sp[-args].u.array->size>=3
       && sp[-args].u.array->item[0].type==T_INT
       && sp[-args].u.array->item[1].type==T_INT
       && sp[-args].u.array->item[2].type==T_INT)
   {
      rgb.r=sp[-args].u.array->item[0].u.integer;
      rgb.g=sp[-args].u.array->item[1].u.integer;
      rgb.b=sp[-args].u.array->item[2].u.integer;
      oper=NULL;
   }
   else
   {
      if (args<1 || sp[-args].type!=T_OBJECT
       || !sp[-args].u.object
       || !(oper=(struct image*)get_storage(sp[-args].u.object,image_program)))
	 error("`==: illegal argument 2\n");

      if (!oper->img)
	 error("image->`<: operator 2 has no image\n");
      if (oper->xsize!=THIS->xsize
          || oper->ysize!=THIS->ysize)
	 error("image->`<: operators differ in size\n");
   }

   s1=THIS->img;
   if (oper) s2=oper->img; else s2=NULL;

   if (s1==s2)
   {
      pop_n_elems(args);
      push_int(0);
      return; /* same image is not less */
   }

   i=THIS->xsize*THIS->ysize;
   THREADS_ALLOW();
   if (s2)
      while (i--)
      {
	 if (s1->r>=s1->r || s1->g>=s1->g || s1->b>=s1->b) { res=0; break; }
	 s1++; s2++;
      }
   else
      while (i--)
      {
	 if (s1->r>=rgb.r || s1->g>=rgb.g || s1->b>=rgb.b) { res=0; break; }
	 s1++;
      }
   THREADS_DISALLOW();
   pop_n_elems(args);
   push_int(res);
}


void image_operator_greater(INT32 args)
{
   struct image *oper;
   rgb_group *s1,*s2,rgb;
   INT32 i;
   int res=1;

   if (!THIS->img)
      error("image->`>: operator 1 has no image\n");

   if (args && sp[-args].type==T_INT)
   {
      rgb.r=sp[-args].u.integer;
      rgb.g=sp[-args].u.integer;
      rgb.b=sp[-args].u.integer;
      oper=NULL;
   }
   else if (args && sp[-args].type==T_ARRAY
       && sp[-args].u.array->size>=3
       && sp[-args].u.array->item[0].type==T_INT
       && sp[-args].u.array->item[1].type==T_INT
       && sp[-args].u.array->item[2].type==T_INT)
   {
      rgb.r=sp[-args].u.array->item[0].u.integer;
      rgb.g=sp[-args].u.array->item[1].u.integer;
      rgb.b=sp[-args].u.array->item[2].u.integer;
      oper=NULL;
   }
   else
   {
      if (args<1 || sp[-args].type!=T_OBJECT
       || !sp[-args].u.object
       || !(oper=(struct image*)get_storage(sp[-args].u.object,image_program)))
	 error("`==: illegal argument 2\n");

      if (!oper->img)
	 error("image->`>: operator 2 has no image\n");
      if (oper->xsize!=THIS->xsize
          || oper->ysize!=THIS->ysize)
	 error("image->`>: operators differ in size\n");
   }

   s1=THIS->img;
   if (oper) s2=oper->img; else s2=NULL;

   if (s1==s2)
   {
      pop_n_elems(args);
      push_int(0);
      return; /* same image is not less */
   }

   i=THIS->xsize*THIS->ysize;
   THREADS_ALLOW();
   if (s2)
      while (i--)
      {
	 if (s1->r<=s1->r || s1->g<=s1->g || s1->b<=s1->b) { res=0; break; }
	 s1++; s2++;
      }
   else
      while (i--)
      {
	 if (s1->r<=rgb.r || s1->g<=rgb.g || s1->b<=rgb.b) { res=0; break; }
	 s1++;
      }
   THREADS_DISALLOW();
   pop_n_elems(args);
   push_int(res);
}


/*
**! method array(float) average()
**! method array(int) min()
**! method array(int) max()
**! method array(int) sum()
**! method array(float) sumf()
**!     Gives back the average, minimum, maximum color value, 
**!	and the sum of all pixel's color value.
**!
**! note:
**!	sum() values can wrap! Most systems only have 31 bits
**!	available for positive integers. (Meaning, be careful 
**!	with images that have more than 8425104 pixels.)
**!
**!	average() and sumf() may also wrap, but on a line basis.
**!	(Meaning, be careful with images that are wider 
**!      than 8425104 pixels.) These functions may have a precision
**!	problem instead, during to limits in the 'double' C type and/or
**!	'float' Pike type.
*/

void image_average(INT32 args)
{
   unsigned long x,y,xz;
   struct { double r,g,b; } sumy={0.0,0.0,0.0};
   rgb_group *s=THIS->img;

   pop_n_elems(args);

   if (!THIS->img)
      error("Image.image->average(): no image\n");
   if (!THIS->xsize || !THIS->ysize)
      error("Image.image->average(): no pixels in image (division by zero)\n");

   y=THIS->ysize;
   xz=THIS->xsize;
   THREADS_ALLOW();
   while (y--)
   {
      rgbl_group sumx={0,0,0};
      x=xz;
      while (x--)
      {
	 sumx.r+=s->r;
	 sumx.g+=s->g;
	 sumx.b+=s->b;
	 s++;
      }
      sumy.r+=((float)sumx.r)/(float)xz;
      sumy.g+=((float)sumx.g)/(float)xz;
      sumy.b+=((float)sumx.b)/(float)xz;
   }
   THREADS_DISALLOW();

   push_float(sumy.r/(float)THIS->ysize);
   push_float(sumy.g/(float)THIS->ysize);
   push_float(sumy.b/(float)THIS->ysize);

   f_aggregate(3);
}


void image_sumf(INT32 args)
{
   unsigned long x,y,xz;
   struct { double r,g,b; } sumy={0.0,0.0,0.0};
   rgb_group *s=THIS->img;

   pop_n_elems(args);

   if (!THIS->img)
      error("Image.image->sumf(): no image\n");

   y=THIS->ysize;
   xz=THIS->xsize;
   THREADS_ALLOW();
   while (y--)
   {
      rgbl_group sumx={0,0,0};
      x=xz;
      while (x--)
      {
	 sumx.r+=s->r;
	 sumx.g+=s->g;
	 sumx.b+=s->b;
	 s++;
      }
      sumy.r+=(float)sumx.r;
      sumy.g+=(float)sumx.g;
      sumy.b+=(float)sumx.b;
   }
   THREADS_DISALLOW();

   push_float(sumy.r);
   push_float(sumy.g);
   push_float(sumy.b);

   f_aggregate(3);
}

void image_sum(INT32 args)
{
   unsigned long n;
   rgb_group *s=THIS->img;
   rgbl_group sum={0,0,0};

   pop_n_elems(args);

   if (!THIS->img)
      error("Image.image->sum(): no image\n");

   n=THIS->ysize*THIS->xsize;
   THREADS_ALLOW();
   while (n--)
   {
      sum.r+=s->r;
      sum.g+=s->g;
      sum.b+=s->b;
      s++;
   }
   THREADS_DISALLOW();

   push_int(sum.r);
   push_int(sum.g);
   push_int(sum.b);

   f_aggregate(3);
}

void image_min(INT32 args)
{
   unsigned long n;
   rgb_group *s=THIS->img;
   rgb_group x={255,255,255};

   pop_n_elems(args);

   if (!THIS->img)
      error("Image.image->min(): no image\n");

   n=THIS->ysize*THIS->xsize;
   THREADS_ALLOW();
   while (n--)
   {
      if (x.r>s->r) x.r=s->r;
      if (x.g>s->g) x.g=s->g;
      if (x.b>s->b) x.b=s->b;
      s++;
   }
   THREADS_DISALLOW();

   push_int(x.r);
   push_int(x.g);
   push_int(x.b);

   f_aggregate(3);
}

void image_max(INT32 args)
{
   unsigned long n;
   rgb_group *s=THIS->img;
   rgb_group x={0,0,0};

   pop_n_elems(args);

   if (!THIS->img)
      error("Image.image->max(): no image\n");

   n=THIS->ysize*THIS->xsize;
   THREADS_ALLOW();
   while (n--)
   {
      if (x.r<s->r) x.r=s->r;
      if (x.g<s->g) x.g=s->g;
      if (x.b<s->b) x.b=s->b;
      s++;
   }
   THREADS_DISALLOW();

   push_int(x.r);
   push_int(x.g);
   push_int(x.b);

   f_aggregate(3);
}


/*
**! method array(int) find_min()
**! method array(int) find_max()
**! method array(int) find_min(int r,int g,int b)
**! method array(int) find_max(int r,int g,int b)
**!     Gives back the position of the minimum or maximum
**!	pixel value, weighted to grey.
**!
**! arg int r
**! arg int g
**! arg int b
**!     weight of color, default is r=87,g=127,b=41, same
**!	as the <ref>grey</ref>() method.
*/

static INLINE void getrgbl(rgbl_group *rgb,INT32 args_start,INT32 args,char *name)
{
   INT32 i;
   if (args-args_start<3) return;
   for (i=0; i<3; i++)
      if (sp[-args+i+args_start].type!=T_INT)
         error("Illegal r,g,b argument to %s\n",name);
   rgb->r=sp[-args+args_start].u.integer;
   rgb->g=sp[1-args+args_start].u.integer;
   rgb->b=sp[2-args+args_start].u.integer;
}

void image_find_min(INT32 args)
{
   unsigned long x,y,xz,xp=0,yp=0,yz;
   rgb_group *s=THIS->img;
   rgbl_group rgb;
   double div,min;

   if (args<3)
   {
      rgb.r=87;
      rgb.g=127;
      rgb.b=41;
   }
   else
      getrgbl(&rgb,0,args,"Image.image->find_min()");
   if (rgb.r||rgb.g||rgb.b)
      div=1.0/(rgb.r+rgb.g+rgb.b);
   else
      div=1.0;
   
   pop_n_elems(args);

   if (!THIS->img)
      error("Image.image->find_min(): no image\n");
   if (!THIS->xsize || !THIS->ysize)
      error("Image.image->find_min(): no pixels in image (none to find)\n");

   yz=THIS->ysize;
   xz=THIS->xsize;
   min=(rgb.r+rgb.g+rgb.b)*256.0;
   THREADS_ALLOW();
   for (y=0; y<yz; y++)
   {
      for (x=0; x<xz; x++)
      {
	 double val=(s->r*rgb.r+s->g*rgb.g+s->b*rgb.b)*div;
	 if (val<min) xp=x,yp=y,min=val;
	 s++;
      }
   }
   THREADS_DISALLOW();

   push_int(xp);
   push_int(yp);

   f_aggregate(2);
}

void image_find_max(INT32 args)
{
   unsigned long x,y,xz,xp=0,yp=0,yz;
   rgb_group *s=THIS->img;
   rgbl_group rgb;
   double div,max;

   if (args<3)
   {
      rgb.r=87;
      rgb.g=127;
      rgb.b=41;
   }
   else
      getrgbl(&rgb,0,args,"Image.image->find_max()");
   if (rgb.r||rgb.g||rgb.b)
      div=1.0/(rgb.r+rgb.g+rgb.b);
   else
      div=1.0;
   
   pop_n_elems(args);

   if (!THIS->img)
      error("Image.image->find_max(): no image\n");
   if (!THIS->xsize || !THIS->ysize)
      error("Image.image->find_max(): no pixels in image (none to find)\n");

   yz=THIS->ysize;
   xz=THIS->xsize;
   max=0.0;
   THREADS_ALLOW();
   for (y=0; y<yz; y++)
   {
      for (x=0; x<xz; x++)
      {
	 double val=(s->r*rgb.r+s->g*rgb.g+s->b*rgb.b)*div;
	 if (val>max) xp=x,yp=y,max=val;
	 s++;
      }
   }
   THREADS_DISALLOW();

   push_int(xp);
   push_int(yp);

   f_aggregate(2);
}

