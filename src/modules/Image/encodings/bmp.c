/* $Id: bmp.c,v 1.1 1999/03/22 14:35:25 mirar Exp $ */

/*
**! module Image
**! note
**!	$Id: bmp.c,v 1.1 1999/03/22 14:35:25 mirar Exp $
**! submodule BMP
**!
**!	This submodule keeps the BMP (Windows Bitmap)
**!	encode/decode capabilities of the <ref>Image</ref> module.
**!
**!	BMP is common in the Windows environment.
**!
**!	Simple encoding:<br>
**!	<ref>encode</ref>
**!
**!	Advanced encoding:<br>
**!	<ref>encode_P1</ref>, <br>
**!	<ref>encode_P2</ref>, <br>
**!	<ref>encode_P3</ref>, <br>
**!	<ref>encode_P4</ref>, <br>
**!	<ref>encode_P5</ref>, <br>
**!	<ref>encode_P6</ref>
**!
**! see also: Image, Image.image, Image.colortable
*/
#include "global.h"

#include <math.h>
#include <ctype.h>

#include "stralloc.h"
RCSID("$Id: bmp.c,v 1.1 1999/03/22 14:35:25 mirar Exp $");
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "threads.h"
#include "array.h"
#include "error.h"


#include "image.h"
#include "colortable.h"

#include "builtin_functions.h"

extern void f_add(INT32 args);

extern struct program *image_colortable_program;
extern struct program *image_program;

static INLINE void push_ubo_32bit(unsigned long x)
{
   char buf[4];

   buf[0]=(char)(x);
   buf[1]=(char)(x>>8);
   buf[2]=(char)(x>>16);
   buf[3]=(char)(x>>24);
   push_string(make_shared_binary_string(buf,4));
}

static INLINE void push_ubo_16bit(unsigned long x)
{
   char buf[2];
   buf[0]=(char)(x);
   buf[1]=(char)(x>>8);
   push_string(make_shared_binary_string(buf,2));
}

static INLINE unsigned long int_from_32bit(unsigned char *data)
{
   return (data[0])|(data[1]<<8)|(data[2]<<16)|(data[3]<<24);
}

#define int_from_16bit(X) _int_from_16bit((unsigned char*)(X))
static INLINE unsigned long _int_from_16bit(unsigned char *data)
{
   return (data[0])|(data[1]<<8);

}

/*
**! method string encode(object image)
**! method string encode(object image,object colortable)
**!	Make a BMP. It default to a 24 bpp BMP file,
**!	but if a colortable is given, it will be 8bpp 
**!	with a palette entry.
**!
**! arg object image
**!	Source image.
**! arg object colortable
**!	Colortable object. 
**!
**  see also: decode
**!
**! returns the encoded image as a string
**!
**! bugs
**!	Doesn't support all BMP modes. At all.
*/

void img_bmp_encode(INT32 args)
{
   struct object *o,*oc;
   struct image *img;
   struct neo_colortable *nct;
   int n=0,bpp;
   int size,offs;
   struct pike_string *ps; 

   if (!args)
      error("Image.BMP.encode: Illegal number of arguments\n");

   if (sp[-args].type!=T_OBJECT ||
       !(img=(struct image*)get_storage(o=sp[-args].u.object,image_program)))
      error("Image.BMP.encode: Illegal argument 1, expected image object\n");

   if (args==1)
      nct=NULL,oc=NULL;
   else
      if (sp[-args].type!=T_OBJECT ||
	  !(nct=(struct neo_colortable*)
	    get_storage(oc=sp[1-args].u.object,image_colortable_program)))
	 error("Image.BMP.encode: Illegal argument 2, "
	       "expected colortable object\n");

   if (!nct) 
      bpp=24;
   else if (image_colortable_size(nct)<=256)
      bpp=8; /* only supports this for now */
   else
      error("Image.BMP.encode: Illegal argument 2, "
	    "can only handle less or equal then 256 colors\n");

   if (oc) oc->refs++;
   o->refs++;
   pop_n_elems(args);

   apply(o,"mirrory",0);
   free_object(o);
   if (sp[-1].type!=T_OBJECT ||
       !(img=(struct image*)get_storage(o=sp[-1].u.object,image_program)))
      error("Image.BMP.encode: wierd result from ->mirrory()\n");
   if (nct) push_object(oc);

   /* bitmapinfo */
   push_ubo_32bit(40); /* size of info structure */
   push_ubo_32bit(img->xsize); /* width */
   push_ubo_32bit(img->ysize); /* height */
   push_ubo_16bit(1);  /* "number of planes for the target device" */
   push_ubo_16bit(bpp); /* bits per pixel (see above) */
   push_ubo_32bit(0); /* compression: none */
   push_ubo_32bit(0); /* size of image (0 is valid if no compression) */
   push_ubo_32bit(0); /* horisontal resolution in pixels/meter */
   push_ubo_32bit(0); /* vertical resolution in pixels/meter */

   if (nct) /* size of colortable */
      push_ubo_32bit(image_colortable_size(nct));
   else
      push_ubo_32bit(0);
   
   push_ubo_32bit(0); /* number of important colors or zero */
   n=11;

   /* palette */

   if (nct)
   {
      ps=begin_shared_string((1<<bpp)*4);
      MEMSET(ps->str,0,(1<<bpp)*4);
      image_colortable_write_bgra(nct,(unsigned char *)ps->str);
      push_string(end_shared_string(ps));
      n++;
   }

   f_add(n);
   n=1;
   /* the image */
   offs=sp[-1].u.string->len;

   if (nct)
   {
      ps=begin_shared_string(img->xsize*img->ysize);
      image_colortable_index_8bit_image(nct,img->img,(unsigned char *)ps->str,
					img->xsize*img->ysize,img->xsize);
      push_string(ps=end_shared_string(ps));
      n++;
   }
   else
   {
      if (sizeof(rgb_group)==3)
	 push_string(make_shared_binary_string((char*)img->img,
					       img->xsize*img->ysize*3));
      else
      {
	 unsigned char *c;
	 int n=img->xsize*img->ysize;
	 rgb_group *s=img->img;
	 c=(unsigned char*)((ps=begin_shared_string(n*3))->str);
	 while (n--)
	 {
	    *(c++)=s->r;
	    *(c++)=s->g;
	    *(c++)=s->b;
	    s++;
	 }
	 push_string(end_shared_string(ps));
      }
      n++;
   }
   
   /* done */
   f_add(n);

   /* "bitmapfileheader" */
   /* this is last, since we need to know the size of the file here */
   
   size=sp[-1].u.string->len;

   push_text("BM");         /* "BM" */
   push_ubo_32bit(size+14); /* "size of file" */
   push_ubo_16bit(0);       /* reserved */
   push_ubo_16bit(0);       /* reserved */
   push_ubo_32bit(offs+14);    /* offset to bitmap data */

   f_add(5);
   stack_swap();
   f_add(2);

   if (nct)
   {
      stack_swap();
      pop_stack();
   }
   stack_swap();
   pop_stack();  /* get rid of colortable & source objects */
}

struct program *image_bmp_module_program=NULL;

void init_image_bmp(void)
{
   start_new_program();
   
   add_function("encode",img_bmp_encode,
		"function(object,void|object:string)",0);

   image_bmp_module_program=end_program();
   push_object(clone_object(image_bmp_module_program,0));
   {
     struct pike_string *s=make_shared_string("BMP");
     add_constant(s,sp-1,0);
     free_string(s);
   }
   pop_stack();
}

void exit_image_bmp(void)
{
  if(image_bmp_module_program)
  {
    free_program(image_bmp_module_program);
    image_bmp_module_program=0;
  }
}
