/* $Id: bmp.c,v 1.3 1999/04/12 11:41:13 mirar Exp $ */

/*
**! module Image
**! note
**!	$Id: bmp.c,v 1.3 1999/04/12 11:41:13 mirar Exp $
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
**! see also: Image, Image.image, Image.colortable
*/
#include "global.h"

#include <math.h>
#include <ctype.h>

#include "stralloc.h"
RCSID("$Id: bmp.c,v 1.3 1999/04/12 11:41:13 mirar Exp $");
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "threads.h"
#include "array.h"
#include "mapping.h"
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
**! see also: decode
**!
**! returns the encoded image as a string
**!
**! bugs
**!	Doesn't support all BMP modes. At all.
*/

void img_bmp_encode(INT32 args)
{
   struct object *o=NULL,*oc=NULL;
   struct image *img=NULL;
   struct neo_colortable *nct=NULL;
   int n=0,bpp=0;
   int size,offs;
   struct pike_string *ps; 

   if (!args)
      SIMPLE_TOO_FEW_ARGS_ERROR("Image.BMP.encode",1);

   if (sp[-args].type!=T_OBJECT ||
       !(img=(struct image*)get_storage(o=sp[-args].u.object,image_program)))
      SIMPLE_BAD_ARG_ERROR("Image.BMP.encode",1,"image object");

   if (args==1)
      nct=NULL,oc=NULL;
   else
      if (sp[-args].type!=T_OBJECT ||
	  !(nct=(struct neo_colortable*)
	    get_storage(oc=sp[1-args].u.object,image_colortable_program)))
	 SIMPLE_BAD_ARG_ERROR("Image.BMP.encode",2,"colortable object");

   if (!nct) 
      bpp=24;
   else if (image_colortable_size(nct)<=256)
      bpp=8; /* only supports this for now */
   else
      SIMPLE_BAD_ARG_ERROR("Image.BMP.encode",2,"colortable object with max 256 colors");

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
      image_colortable_write_bgrz(nct,(unsigned char *)ps->str);
      push_string(end_shared_string(ps));
      n++;
   }

   f_add(n);
   n=1;
   /* the image */
   offs=sp[-1].u.string->len;

   if (nct)
   {
      /* 8bpp image */
      ps=begin_shared_string(img->xsize*img->ysize);
      image_colortable_index_8bit_image(nct,img->img,(unsigned char *)ps->str,
					img->xsize*img->ysize,img->xsize);
      push_string(ps=end_shared_string(ps));
      n++;
   }
   else
   {
      unsigned char *c;
      int n=img->xsize*img->ysize;
      rgb_group *s=img->img;
      c=(unsigned char*)((ps=begin_shared_string(n*3))->str);
      while (n--)
      {
	 *(c++)=s->b;
	 *(c++)=s->g;
	 *(c++)=s->r;
	 s++;
      }
      push_string(end_shared_string(ps));
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

/*
**! method mapping _decode(string data)
**! method mapping decode_header(string data)
**! method object decode(string data)
**!	Decode a BMP. Not all modes are supported.
**!
**!	<ref>decode</ref> gives an image object,
**!	<ref>_decode</ref> gives a mapping in the format
**!	<pre>
**!		"type":"image/bmp",
**!		"image":image object,
**!		"colortable":colortable object (if applicable)
**!
**!		"xsize":int,
**!		"ysize":int,
**!		"compression":int,
**!		"bpp":int,
**!		"windows":int,
**!	</pre>
**!
**! see also: encode
**!
**! returns the encoded image as a string
**!
**! bugs
**!	Doesn't support all BMP modes. At all.
*/

void i_img_bmp__decode(INT32 args,int header_only)
{
   p_wchar0 *s;
   int len;
   int xsize=0,ysize=0,bpp=0,comp=0;
   struct image *img=NULL;
   struct neo_colortable *nct=NULL;
   struct object *o;
   rgb_group *d;
   int n=0,j,i,y,skip;
   int windows=0;

   if (args<1)
      SIMPLE_TOO_FEW_ARGS_ERROR("Image.BMP.decode",1);

   if (sp[-args].type!=T_STRING)
      SIMPLE_BAD_ARG_ERROR("Image.BMP.decode",1,"string");
   if (sp[-args].u.string->size_shift)
      SIMPLE_BAD_ARG_ERROR("Image.BMP.decode",1,"8 bit string");

   s=(p_wchar0*)sp[-args].u.string->str;
   len=sp[-args].u.string->len;

   pop_n_elems(args-1);
   
   if (len<20)
      error("Image.BMP.decode: not a BMP (file to short)\n");

   if (s[0]!='B' || s[1]!='M')
      error("Image.BMP.decode: not a BMP (illegal header)\n");

   /* ignore stupid info like file size */

   switch (int_from_32bit(s+14))
   {
      case 40: /* windows mode */

	 if (len<54)
	    error("Image.BMP.decode: unexpected EOF in header\n");

	 push_text("xsize");
	 push_int(xsize=int_from_32bit(s+14+4*1));
	 n++;

	 push_text("ysize");
	 push_int(ysize=int_from_32bit(s+14+4*2));
	 n++;

	 push_text("target_planes");
	 push_int(int_from_16bit(s+14+4*3));
	 n++;

	 push_text("bpp");
	 push_int(bpp=int_from_16bit(s+14+4*3+2));
	 n++;

	 push_text("compression");
	 push_int(comp=int_from_32bit(s+14+4*4));
	 n++;

	 push_text("xres");
	 push_int(int_from_32bit(s+14+4*5));
	 n++;

	 push_text("yres");
	 push_int(int_from_32bit(s+14+4*6));
	 n++;
	 
	 push_text("windows");
	 push_int(windows=1);
	 n++;

	 s+=54;
	 len-=54;

	 break;

      case 12: /* dos (?) mode */

	 if (len<26)
	    error("Image.BMP.decode: unexpected EOF in header\n");

	 push_text("xsize");
	 push_int(xsize=int_from_16bit(s+14+4));
	 n++;

	 push_text("ysize");
	 push_int(ysize=int_from_16bit(s+14+6));
	 n++;

	 push_text("target_planes");
	 push_int(int_from_16bit(s+14+8));
	 n++;

	 push_text("bpp");
	 push_int(bpp=int_from_16bit(s+14+10));
	 n++;

	 push_text("compression");
	 push_int(comp=0);
	 n++;

	 s+=26;
	 len-=26;

	 break;

      default:
	 error("Image.BMP.decode: not a BMP (illegal info)\n");
   }

   if (header_only)
   {
      f_aggregate_mapping(n*2);
      stack_swap();
      pop_stack();
      return;
   }

   if (bpp!=24) /* get palette */
   {
      push_text("colortable");

      if (windows)
      {
	 if ((4<<bpp)>len)
	    error("Image.BMP.decode: unexpected EOF in palette\n");

	 push_string(make_shared_binary_string(s,(4<<bpp)));
	 push_int(2);
	 push_object(o=clone_object(image_colortable_program,2));
	 nct=(struct neo_colortable*)get_storage(o,image_colortable_program);

	 s+=(4<<bpp);
	 len-=(4<<bpp);
      }
      else
      {
	 if ((3<<bpp)>len)
	    error("Image.BMP.decode: unexpected EOF in palette\n");

	 push_string(make_shared_binary_string(s,(3<<bpp)));
	 push_int(1);
	 push_object(o=clone_object(image_colortable_program,2));
	 nct=(struct neo_colortable*)get_storage(o,image_colortable_program);

	 s+=(3<<bpp);
	 len-=(3<<bpp);
      }
   }

   push_text("image");

   push_int(xsize);
   push_int(ysize);
   push_object(o=clone_object(image_program,2));
   img=(struct image*)get_storage(o,image_program);
   n++;

   if (comp > 2)
      error("Image.BMP.decode: illegal compression: %d\n",comp);

   switch (bpp)
   {
      case 24:
	 if (comp)
	    error("Image.BMP.decode: can't handle compressed 24bpp BMP\n");

	 j=(len)/3;
	 y=img->ysize;
	 while (j && y--)
	 {
	    d=img->img+img->xsize*y;
	    i=img->xsize; if (i>j) i=j; 
	    j-=i;
	    while (i--)
	    {
	       d->b=*(s++);
	       d->g=*(s++);
	       d->r=*(s++);
	       d++;
	    }
	 }
	 break;
      case 8:
	 if (comp)
	    error("Image.BMP.decode: can't handle compressed 8bpp BMP\n");

	 skip=4-(img->xsize&3);
	 j=len;
	 y=img->ysize;
	 while (j && y--)
	 {
	    d=img->img+img->xsize*y;
	    i=img->xsize; if (i>j) i=j; 
	    j-=i;
	    while (i--)
	       *(d++)=nct->u.flat.entries[*(s++)].color;
	    if (j>=skip) { j-=skip; s+=skip; }
	 }
	 break;
      case 4:
	 if (comp == 1)
	    error("Image.BMP.decode: can't handle RLE4 compressed 4bpp BMP\n");

	 if (comp == 2) /* RLE4 */
	 {
	    
	 }
	 else
	 {
	    skip=3-((img->xsize/2)&3);
	    j=len;
	    y=img->ysize;
	    while (j && y--)
	    {
	       d=img->img+img->xsize*y;
	       i=img->xsize;
	       while (i && j--)
	       {
		  if (i) *(d++)=nct->u.flat.entries[(s[0]>>4)&15].color,i--;
		  if (i) *(d++)=nct->u.flat.entries[s[0]&15].color,i--;
		  s++;
	       }
	       if (j>=skip) { j-=skip; s+=skip; }
	    }
	 }
	 break;
      case 1:
	 if (comp)
	    error("Image.BMP.decode: can't handle compressed 1bpp BMP\n");

	 skip=3-((img->xsize/8)&3);
	 j=len;
	 y=img->ysize;
	 while (j && y--)
	 {
	    d=img->img+img->xsize*y;
	    i=img->xsize;
	    while (i && j--)
	    {
	       if (i) *(d++)=nct->u.flat.entries[!!(s[0]&128)].color,i--;
	       if (i) *(d++)=nct->u.flat.entries[!!(s[0]&64)].color,i--;
	       if (i) *(d++)=nct->u.flat.entries[!!(s[0]&32)].color,i--;
	       if (i) *(d++)=nct->u.flat.entries[!!(s[0]&16)].color,i--;
	       if (i) *(d++)=nct->u.flat.entries[!!(s[0]&8)].color,i--;
	       if (i) *(d++)=nct->u.flat.entries[!!(s[0]&4)].color,i--;
	       if (i) *(d++)=nct->u.flat.entries[!!(s[0]&2)].color,i--;
	       if (i) *(d++)=nct->u.flat.entries[s[0]&1].color,i--;
	       s++;
	    }
	    if (j>=skip) { j-=skip; s+=skip; }
	 }
	 break;
      default:
	 error("Image.BMP.decode: unknown bpp: %d\n",bpp);
   }

   f_aggregate_mapping(n*2);
   stack_swap();
   pop_stack();
}


void img_bmp__decode(INT32 args)
{
   return i_img_bmp__decode(args,0);
}

void img_bmp_decode_header(INT32 args)
{
   return i_img_bmp__decode(args,1);
}

void f_index(INT32);

void img_bmp_decode(INT32 args)
{
   img_bmp__decode(args);
   push_text("image");
   f_index(2);
}

struct program *image_bmp_module_program=NULL;

void init_image_bmp(void)
{
   start_new_program();
   
   add_function("encode",img_bmp_encode,
		"function(object,void|object:string)",0);
   add_function("_decode",img_bmp__decode,
		"function(string:mapping)",0);
   add_function("decode",img_bmp_decode,
		"function(string:object)",0);
   add_function("decode_header",img_bmp_decode_header,
		"function(string:mapping)",0);

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
