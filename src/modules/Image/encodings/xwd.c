/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: xwd.c,v 1.27 2004/10/07 22:49:57 nilsson Exp $
*/

/*
**! module Image
**! submodule XWD
**!
**!	This submodule keeps the XWD (X Windows Dump) 
**!     decode capabilities of the <ref>Image</ref> module.
**!
**!	XWD is the output format for the xwd program.
**!
**!	Simple decoding:<br>
**!	<ref>decode</ref>
**!
**!	Advanced decoding:<br>
**!	<ref>_decode</ref>
**!
**! see also: Image, Image.Image, Image.PNM, Image.X
*/
#include "global.h"

#include <math.h>
#include <ctype.h>

#include "stralloc.h"
#include "object.h"
#include "interpret.h"
#include "svalue.h"
#include "pike_error.h"
#include "mapping.h"

#include "image.h"
#include "builtin_functions.h"
#include "operators.h"
#include "encodings.h"


#define sp Pike_sp

extern struct program *image_colortable_program;
extern struct program *image_program;

void image_x_decode_truecolor_masks(INT32 args);
void image_x_decode_pseudocolor(INT32 args);

/*
**! method mapping _decode(string data)
**! method mapping decode_header(string data)
**!	Decodes XWD data and returns the result.
**!
**!	Supported XWD visual classes and encoding formats are
**!		TrueColor / ZPixmap
**!		DirectColor / ZPixmap
**!		PseudoColor / ZPixmap
**!
**!	If someone sends me files of other formats, these formats
**!	may be implemented. <tt>:)</tt> /<tt>mirar+pike@mirar.org</tt>
**!
**! see also: decode
**!
**! note
**!	This function may throw errors upon illegal or unknown XWD data.
**!
**! returns the decoded image as an image object
*/

static INLINE unsigned long int_from_32bit(unsigned char *data)
{
   return (data[0]<<24)|(data[1]<<16)|(data[2]<<8)|(data[3]);
}

static INLINE unsigned long int_from_16bit(unsigned char *data)
{
   return (data[0]<<8)|(data[1]);
}

#define CARD32n(S,N) int_from_32bit((unsigned char*)(S)->str+(N)*4)

void img_xwd__decode(INT32 args,int header_only,int skipcmap)
{
   struct object *co=NULL;

   struct pike_string *s;

   struct 
   {
      unsigned long header_size;         

      unsigned long file_version;     /* = XWD_FILE_VERSION above */
      unsigned long pixmap_format;    /* ZPixmap or XYPixmap */
      unsigned long pixmap_depth;     /* Pixmap depth */
      unsigned long pixmap_width;     /* Pixmap width */
      unsigned long pixmap_height;    /* Pixmap height */
      unsigned long xoffset;          /* Bitmap x offset, normally 0 */
      unsigned long byte_order;       /* of image data: MSBFirst, LSBFirst */
				   
      unsigned long bitmap_unit;      /* scanline padding for bitmaps */
				   
      unsigned long bitmap_bit_order; /* bitmaps only: MSBFirst, LSBFirst */
				   
      unsigned long bitmap_pad;       /* scanline padding for pixmaps */
				   
      unsigned long bits_per_pixel;   /* Bits per pixel */

      /* bytes_per_line is pixmap_width padded to bitmap_unit (bitmaps)
       * or bitmap_pad (pixmaps).  It is the delta (in bytes) to get
       * to the same x position on an adjacent row. */
      unsigned long bytes_per_line;
      unsigned long visual_class;     /* Class of colormap */
      unsigned long red_mask;         /* Z red mask */
      unsigned long green_mask;       /* Z green mask */
      unsigned long blue_mask;        /* Z blue mask */
      unsigned long bits_per_rgb;     /* Log2 of distinct color values */
      unsigned long colormap_entries; /* Number of entries in colmap unused?*/
      unsigned long ncolors;          /* Number of XWDColor structures */
      unsigned long window_width;     /* Window width */
      unsigned long window_height;    /* Window height */
      unsigned long window_x;         /* Window upper left X coordinate */
      unsigned long window_y;         /* Window upper left Y coordinate */
      unsigned long window_bdrwidth;  /* Window border width */
   } header;

   int n=0;

   ONERROR uwp;

   if (args<1 ||
       sp[-args].type!=T_STRING)
      Pike_error("Image.XWD._decode(): Illegal arguments\n");
   
   s=sp[-args].u.string;

   /* header_size = SIZEOF(XWDheader) + length of null-terminated
    * window name. */

   if (s->len<4) 
      Pike_error("Image.XWD._decode: header to small\n");
   header.header_size=CARD32n(s,0);

   if ((size_t)s->len < header.header_size || s->len<100)
      Pike_error("Image.XWD._decode: header to small\n");

   header.file_version=CARD32n(s,1);     

   if (header.file_version!=7)
      Pike_error("Image.XWD._decode: don't understand any other file format then 7\n");

   add_ref(s);
   pop_n_elems(args);
   SET_ONERROR(uwp,do_free_string,s);

   header.pixmap_format=CARD32n(s,2);
   header.pixmap_depth=CARD32n(s,3);     
   header.pixmap_width=CARD32n(s,4);     
   header.pixmap_height=CARD32n(s,5);    
   header.xoffset=CARD32n(s,6);          
   header.byte_order=CARD32n(s,7);       
   header.bitmap_unit=CARD32n(s,8);      
   header.bitmap_bit_order=CARD32n(s,9); 
   header.bitmap_pad=CARD32n(s,10);       
   header.bits_per_pixel=CARD32n(s,11);   
   header.bytes_per_line=CARD32n(s,12);   
   header.visual_class=CARD32n(s,13);     
   header.red_mask=CARD32n(s,14);         
   header.green_mask=CARD32n(s,15);       
   header.blue_mask=CARD32n(s,16);        
   header.bits_per_rgb=CARD32n(s,17);     
   header.colormap_entries=CARD32n(s,18); 
   header.ncolors=CARD32n(s,19);          
   header.window_width=CARD32n(s,20);     
   header.window_height=CARD32n(s,21);    
   header.window_x=CARD32n(s,22);         
   header.window_y=CARD32n(s,23);         
   header.window_bdrwidth=CARD32n(s,24);  

   push_text("header_size"); 
   push_int(header.header_size);
   push_text("file_version");
   push_int(header.file_version);
   push_text("pixmap_format");
   push_int(header.pixmap_format);
   push_text("pixmap_depth");
   push_int(header.pixmap_depth);
   push_text("pixmap_width");
   push_int(header.pixmap_width);

   push_text("pixmap_height");
   push_int(header.pixmap_height);
   push_text("xoffset");
   push_int(header.xoffset);
   push_text("byte_order");
   push_int(header.byte_order);
   push_text("bitmap_unit");
   push_int(header.bitmap_unit);
   push_text("bitmap_bit_order");
   push_int(header.bitmap_bit_order);

   push_text("bitmap_pad");
   push_int(header.bitmap_pad);
   push_text("bits_per_pixel");
   push_int(header.bits_per_pixel);
   push_text("bytes_per_line");
   push_int(header.bytes_per_line);
   push_text("visual_class");
   push_int(header.visual_class);
   push_text("red_mask");
   push_int(header.red_mask);

   push_text("green_mask");
   push_int(header.green_mask);
   push_text("blue_mask");
   push_int(header.blue_mask);
   push_text("bits_per_rgb");
   push_int(header.bits_per_rgb);
   push_text("colormap_entries");
   push_int(header.colormap_entries);
   push_text("ncolors");
   push_int(header.ncolors);

   push_text("window_width");
   push_int(header.window_width);
   push_text("window_height");
   push_int(header.window_height);
   push_text("window_x");
   push_int(header.window_x);
   push_text("window_y");
   push_int(header.window_y);
   push_text("window_bdrwidth");
   push_int(header.window_bdrwidth);

   n+=25;

   push_text("type");
   push_text("image/x-xwd");
   n++;

   /* the size of the header is 100 bytes, name is null-terminated */
   push_text("windowname");
   if (header.header_size>100)
      push_string(make_shared_binary_string(s->str+100,header.header_size-100-1));
   else 
      push_int(0);
   n++;

   /* header.ncolors XWDColor structs */

   if (!skipcmap || 
       header.visual_class==3 || 
       header.visual_class==5)
   {
      push_text("colors");
      if (s->len-header.header_size>=12*header.ncolors)
      {
	 unsigned long i;

	 if (skipcmap)
	    push_int(0);
	 else
	 {
	    for (i=0; i<header.ncolors; i++)
	    {
	       push_int(int_from_32bit((unsigned char*)s->str+header.header_size+i*12 +0));
	       push_int(int_from_16bit((unsigned char*)s->str+header.header_size+i*12 +4));
	       push_int(int_from_16bit((unsigned char*)s->str+header.header_size+i*12 +6));
	       push_int(int_from_16bit((unsigned char*)s->str+header.header_size+i*12 +8));
	       push_int(*(unsigned char*)(s->str+header.header_size+i*12 +10));
	       push_int(*(unsigned char*)(s->str+header.header_size+i*12 +11));
	       f_aggregate(6);
	    }
	    f_aggregate(header.ncolors);
	 }

	 push_text("colortable");

	 for (i=0; i<header.ncolors; i++)
	 {
	    push_int(int_from_16bit((unsigned char*)s->str+header.header_size+i*12 +4)>>8);
	    push_int(int_from_16bit((unsigned char*)s->str+header.header_size+i*12 +6)>>8);
	    push_int(int_from_16bit((unsigned char*)s->str+header.header_size+i*12 +8)>>8);
	    f_aggregate(3);
	 }
      
	 f_aggregate(header.ncolors);
	 push_object(co=clone_object(image_colortable_program,1));
      }
      else
      {
	 f_aggregate(0); /* no room for colors */
	 push_text("colortable");
	 push_int(0);
      }
      n+=2;
      skipcmap=0;
   }

   if (!header_only)
   {
      n++;
      push_text("image");

      if (s->len-(int)(header.header_size+header.ncolors*12)<0)
	 push_text("");
      else
	 push_string(make_shared_binary_string(
	    s->str+(header.header_size+header.ncolors*12),
	    s->len-(header.header_size+header.ncolors*12)));

      switch (header.visual_class*100+header.pixmap_format)
      {
	 /*
	   #define StaticGray              0
	   #define GrayScale               1
	   #define StaticColor             2
	   #define PseudoColor             3
	   #define TrueColor               4
	   #define DirectColor             5
	 */
	 case 402: /* TrueColor / ZPixmap */
	    push_int(header.pixmap_width);
	    push_int(header.pixmap_height);
	    push_int(header.bits_per_pixel);
	    push_int(header.bitmap_pad);
	    push_int(header.byte_order==1);
	    push_int(header.red_mask);
	    push_int(header.green_mask);
	    push_int(header.blue_mask);
	    image_x_decode_truecolor_masks(9);
	    break;
	 case 502: /* DirectColor / ZPixmap */
	    push_int(header.pixmap_width);
	    push_int(header.pixmap_height);
	    push_int(header.bits_per_pixel);
	    push_int(header.bitmap_pad);
	    push_int(header.byte_order==1);
	    push_int(header.red_mask);
	    push_int(header.green_mask);
	    push_int(header.blue_mask);
	    if (!co)
	       image_x_decode_truecolor_masks(9);
	    else
	    {
	       ref_push_object(co);
	       image_x_decode_truecolor_masks(10);
	    }
	    break;
	 case 302: /* PseudoColor / ZPixmap */
	    push_int(header.pixmap_width);
	    push_int(header.pixmap_height);
	    push_int(header.bits_per_pixel);
	    push_int(header.bitmap_pad);
	    push_int(header.byte_order==1);
	    ref_push_object(co);
	    image_x_decode_pseudocolor(7);
	    break;
	 default:
	    pop_stack();
	    push_int(0);
      }
   }

   free_string(s);

   f_aggregate_mapping(DO_NOT_WARN(n*2));

   UNSET_ONERROR(uwp);
}

void image_xwd__decode(INT32 args)
{
   img_xwd__decode(args,0,0);
}

void image_xwd_decode_header(INT32 args)
{
   img_xwd__decode(args,1,0);
}

/*
**! method object decode(string data)
**!	Simple decodes a XWD image file.
*/

static void image_xwd_decode(INT32 args)
{
   if (!args)
      Pike_error("Image.XWD.decode: missing argument\n");

   pop_n_elems(args-1);
   push_int(1);
   img_xwd__decode(2,0,1);
   
   push_text("image");
   f_index(2);
}

struct program *image_xwd_module_program=NULL;

void init_image_xwd(void)
{
   add_function("_decode",image_xwd__decode,
		"function(string,void|int:mapping(string:int|array|object))",0);
   add_function("decode",image_xwd_decode,
		"function(string:object)",0);

   add_function("decode_header",image_xwd_decode_header,
		"function(string:object)",0);

}

void exit_image_xwd(void)
{
}
