/* $Id: any.c,v 1.13 1999/10/21 22:39:01 marcus Exp $ */

/*
**! module Image
**! note
**!	$Id: any.c,v 1.13 1999/10/21 22:39:01 marcus Exp $
**! submodule ANY
**!
**!	This method calls the other decoding methods
**!	and has some heuristics for what type of image
**!	this is.
**!
**!	Methods:
**!	<ref>decode</ref>, <ref>decode_alpha</ref>,
**!	<ref>_decode</ref>
**!
**! see also: Image
**!
*/
#include "global.h"

#include <math.h>
#include <ctype.h>

#include "stralloc.h"
RCSID("$Id: any.c,v 1.13 1999/10/21 22:39:01 marcus Exp $");
#include "pike_macros.h"
#include "operators.h"
#include "builtin_functions.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "threads.h"
#include "array.h"
#include "error.h"
#include "threads.h"

#include "image.h"

void image_gif__decode(INT32 args);
void image_pnm_decode(INT32 args);
void image_xwd__decode(INT32 args);
void image_ilbm_decode(INT32 args);
void image_ras_decode(INT32 args);

/*
**! method mapping _decode(string data)
**! method object decode(string data)
**! method object decode_alpha(string data)
**!	Tries heuristics to find the correct method 
**!	of decoding the data, then calls that method.
**!
**! 	The result of _decode() is a mapping that contains
**!	<pre>
**!		"type":image data type (ie, "image/jpeg" or similar)
**!		"image":the image object,
**!		"alpha":the alpha channel or 0 if N/A
**!	</pre>
**!
**! note
**!	Throws upon failure.
*/

void image_any__decode(INT32 args)
{
   if (args!=1 || sp[-args].type!=T_STRING)
      error("Image.ANY.decode: illegal arguments\n");
   
   if (sp[-args].u.string->len<4)
      error("Image.ANY.decode: too short string\n");

#define CHAR2(a,b) ((((unsigned char)(a))<<8)|((unsigned char)(b)))
   /* ok, try the heuristics */
   switch (CHAR2(sp[-args].u.string->str[0],sp[-args].u.string->str[1]))
   {
      case CHAR2('P','1'):
      case CHAR2('P','2'):
      case CHAR2('P','3'):
      case CHAR2('P','4'):
      case CHAR2('P','5'):
      case CHAR2('P','6'):
      case CHAR2('P','7'):
	 /* ok, a PNM */
	 img_pnm_decode(1);
	 push_text("image/x-pnm");
	 goto simple_image;

      case CHAR2(255,216):
	 /* JFIF */
	 push_text("Image");
	 push_int(0);
	 SAFE_APPLY_MASTER("resolv",2);
	 push_text("JPEG");
	 f_index(2);
	 push_text("_decode");
	 f_index(2);
	 stack_swap();
	 f_call_function(2);
	 return;

      case CHAR2('g','i'):
	 /* XCF */
	 push_text("Image");
	 push_int(0);
	 SAFE_APPLY_MASTER("resolv",2);
	 push_text("XCF");
	 f_index(2);
	 push_text("_decode");
	 f_index(2);
	 stack_swap();
	 f_call_function(2);
	 return;

      case CHAR2(137,'P'):
	 /* PNG */
	 push_text("Image");
	 push_int(0);
	 SAFE_APPLY_MASTER("resolv",2);
	 push_text("PNG");
	 f_index(2);
	 push_text("_decode");
	 f_index(2);
	 stack_swap();
	 f_call_function(2);
	 return;

      case CHAR2('G','I'):
	 /* GIF */
	 image_gif_decode_map(1);
	 return;

      case CHAR2('F','O'):
	 /* ILBM (probably) */
	 img_ilbm_decode(1);
	 push_text("image/x-ilbm");
	 goto simple_image;

      case CHAR2('B','M'):
	 /* BMP */
	 img_bmp__decode(1);
	 return;

      case CHAR2(0x59,0xa6):
	 /* RAS */
	 img_ras_decode(1);
	 push_text("image/x-sun-raster");
	 goto simple_image;

      case CHAR2(0,0):
	 switch (CHAR2(sp[-args].u.string->str[2],sp[-args].u.string->str[3]))
	 {
	    case CHAR2(0,'k'):
	       /* XWD */
	       image_xwd__decode(1);
	       return; /* done */
	 }
	 
	 goto unknown_format;

      default:
unknown_format:
	 error("Unknown image format.\n");	 
   }

simple_image:
   /* on stack: object image,string type */
   f_aggregate(2);
   push_text("image");
   push_text("type");
   f_aggregate(2);
   stack_swap();
   f_mkmapping(2);
   return;
}

void image_any_decode_header(INT32 args)
{
   if (args!=1 || sp[-args].type!=T_STRING)
      error("Image.ANY.decode_header: illegal arguments\n");
   
   if (sp[-args].u.string->len<4)
      error("Image.ANY.decode_header: too short string\n");

#define CHAR2(a,b) ((((unsigned char)(a))<<8)|((unsigned char)(b)))
   /* ok, try the heuristics */
   switch (CHAR2(sp[-args].u.string->str[0],sp[-args].u.string->str[1]))
   {
      case CHAR2('P','1'):
      case CHAR2('P','2'):
      case CHAR2('P','3'):
      case CHAR2('P','4'):
      case CHAR2('P','5'):
      case CHAR2('P','6'):
      case CHAR2('P','7'):
	 error("Image.ANY.decode: decoding of PNM header unimplemented\n");

      case CHAR2(255,216):
	 /* JFIF */
	 push_text("Image");
	 push_int(0);
	 SAFE_APPLY_MASTER("resolv",2);
	 push_text("JPEG");
	 f_index(2);
	 push_text("decode_header");
	 f_index(2);
	 stack_swap();
	 f_call_function(2);
	 return;

      case CHAR2(137,'P'):
	 /* PNG */
	 push_text("Image");
	 push_int(0);
	 SAFE_APPLY_MASTER("resolv",2);
	 push_text("PNG");
	 f_index(2);
	 push_text("decode_header");
	 f_index(2);
	 stack_swap();
	 f_call_function(2);
	 return;

      case CHAR2('g','i'):
	 /* XCF */
	 push_text("Image");
	 push_int(0);
	 SAFE_APPLY_MASTER("resolv",2);
	 push_text("XCF");
	 f_index(2);
	 push_text("_decode"); /* just try it ... */
	 f_index(2);
	 stack_swap();
	 f_call_function(2);
	 return;

      case CHAR2('G','I'):
	 /* GIF */
	 image_gif_decode_map(1);
	 return;

      case CHAR2('F','O'):
	 error("Image.ANY.decode: decoding of ILBM header unimplemented\n");

      case CHAR2('B','M'):
	 /* BMP */
	 img_bmp_decode_header(1);
	 return;

      case CHAR2(0x59,0xa6):
	 /* RAS */
	 error("Image.ANY.decode: decoding of RAS header unimplemented\n");

      case CHAR2(0,0):
	 switch (CHAR2(sp[-args].u.string->str[2],sp[-args].u.string->str[3]))
	 {
	    case CHAR2(0,'k'):
	       /* XWD */
	       image_xwd_decode_header(1);
	       return; /* done */
	 }
	 
	 goto unknown_format;

      default:
unknown_format:
	 error("Unknown image format.\n");	 
   }
}

void image_any_decode(INT32 args)
{
   image_any__decode(args);
   push_text("image");
   f_index(2);
}

void image_any_decode_alpha(INT32 args)
{
   image_any__decode(args);
   push_text("alpha");
   f_index(2);
}


/** module *******************************************/

void init_image_any(void)
{
   add_function("_decode",image_any__decode,
		"function(string:mapping)",0);
   add_function("decode_header",image_any_decode_header,
		"function(string:mapping)",0);

   add_function("decode",image_any_decode,
		"function(string:object)",0);
   add_function("decode_alpha",image_any_decode_alpha,
		"function(string:object)",0);
}

void exit_image_any(void)
{
}
