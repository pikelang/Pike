#include "global.h"
RCSID("$Id: image_png.c,v 1.1 1998/02/26 21:38:01 mirar Exp $");

#include "config.h"

#if !defined(HAVE_LIBZ) && !defined(HAVE_LIBGZ)
#undef HAVE_ZLIB_H
#endif

#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "threads.h"
#include "array.h"
#include "mapping.h"
#include "error.h"
#include "stralloc.h"
#include "dynamic_buffer.h"

#ifdef HAVE_ZLIB

#include <zlib.h>
#include "../Image/image.h"

static struct program *image_program=NULL;
static struct program *colortable_program=NULL;

#endif /* HAVE_ZLIB */

static struct pike_string *param_blu;
static struct pike_string *param_blaa;

#ifdef HAVE_JPEGLIB_H

/*
**! module Image
**! submodule PNG
**!
**! note
**!	This module uses <tt>zlib</tt>.
*/

static void push_png_chunk(unsigned char *type,    /* 4 bytes */
			   struct pike_string *data)
{
   /* 
    *  0: 4 bytes of length of data block (=n)
    *  4: 4 bytes of chunk type
    *  8: n bytes of data
    *  8+n: 4 bytes of CRC
    */
   
   push_nbo_32bit(data->len);
   push_string(make_binary_shared_string(4,type));
   push_string(data);
   push_nbo_32bit(crc32(crc32(0,NULL,0),data->str,data->len));
   f_add(4);
}

/*
**! method string _chunk(string type,string data)
**! 	Encodes a PNG chunk.
**!
**! note
**!	Please read about the PNG file format.
*/

static void image_png_chunk(INT32 args)
{
   struct pike_string *a,*b;

   if (args!=2 ||
       sp[-args].type!=T_STRING ||
       sp[1-args].type!=T_STRING)
      error("Image.PNG.chunk: Illegal argument(s)\n");
   
   a=sp[-args].u.string;
   if (a->len!=4)
      error("Image.PNG.chunk: Type string not 4 characters\n");
   b=sp[1-args].u.string;
   pop_n_elems(args-2);
   sp-=2;
   push_png_chunk(a->str,b);
   free_string(a);
   free_string(b);
}


/*
**! method string encode(object image)
**! method string encode(object image, mapping options)
**! 	Encodes a PNG image. 
**!
**!     The <tt>options</tt> argument may be a mapping
**!	containing zero or more encoding options:
**!
**!	<pre>
**!	normal options:
**!	    "quality":0..100
**!		Set quality of result. Default is 75.
**!	    "optimize":0|1
**!		Optimize Huffman table. Default is on (1) for
**!		images smaller than 50kpixels.
**!	    "progressive":0|1
**!		Make a progressive JPEG. Default is off.
**!
**!	advanced options:
**!	    "smooth":1..100
**!		Smooth input. Value is strength.
**!	    "method":JPEG.IFAST|JPEG.ISLOW|JPEG.FLOAT|JPEG.DEFAULT|JPEG.FASTEST
**!		DCT method to use.
**!		DEFAULT and FASTEST is from the jpeg library,
**!		probably ISLOW and IFAST respective.
**!
**!	wizard options:
**!	    "baseline":0|1
**!		Force baseline output. Useful for quality&lt;20.
**!	</pre>
**!
**! note
**!	Please read some about JPEG files. A quality 
**!	setting of 100 does not mean the result is 
**!	lossless.
*/

static void image_jpeg_encode(INT32 args)
{
}

/*
**! method object decode(string data)
**! method object decode(string data, mapping options)
**! 	Decodes a PNG image. 
**!
**!     The <tt>options</tt> argument may be a mapping
**!	containing zero or more encoding options:
**!
**!	<pre>
**!	advanced options:
**!	    "block_smoothing":0|1
**!		Do interblock smoothing. Default is on (1).
**!	    "fancy_upsampling":0|1
**!		Do fancy upsampling of chroma components. 
**!		Default is on (1).
**!	    "method":JPEG.IFAST|JPEG.ISLOW|JPEG.FLOAT|JPEG.DEFAULT|JPEG.FASTEST
**!		DCT method to use.
**!		DEFAULT and FASTEST is from the jpeg library,
**!		probably ISLOW and IFAST respective.
**!
**!	wizard options:
**!	    "scale_num":1..
**!	    "scale_denom":1..
**!	        Rescale the image when read from JPEG data.
**!		My (Mirar) version (6a) of jpeglib can only handle
**!		1/1, 1/2, 1/4 and 1/8. 
**!
**!	</pre>
**!
**! note
**!	Please read some about JPEG files. 
*/

static void image_jpeg_decode(INT32 args)
{
}

#endif /* HAVE_JPEGLIB_H */

/*** module init & exit & stuff *****************************************/

void f_index(INT32 args);

void pike_module_exit(void)
{
}

void pike_module_init(void)
{
#ifdef HAVE_JPEGLIB_H
   push_string(make_shared_string("Image"));
   push_int(0);
   SAFE_APPLY_MASTER("resolv",2);
   if (sp[-1].type==T_OBJECT) 
   {
      push_string(make_shared_string("image"));
      f_index(2);
      image_program=program_from_svalue(sp-1);
   }
   pop_n_elems(1);

   push_string(make_shared_string("Image"));
   push_int(0);
   SAFE_APPLY_MASTER("resolv",2);
   if (sp[-1].type==T_OBJECT) 
   {
      push_string(make_shared_string("colortable"));
      f_index(2);
      image_program=program_from_svalue(sp-1);
   }
   pop_n_elems(1);

   if (image_program && image_colortable)
   {
      add_function("decode",image_png__chunk,
		   "function(string,string:string)",
		   OPT_TRY_OPTIMIZE);
      add_function("decode",image_png_decode,
		   "function(string,void|mapping(string:int):object)",0);
      add_function("encode",image_png_encode,
		   "function(object,void|mapping(string:int):string)",
		   OPT_TRY_OPTIMIZE);
   }

#endif /* HAVE_JPEGLIB_H */

}
