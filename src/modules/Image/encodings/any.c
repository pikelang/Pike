/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: any.c,v 1.31 2005/01/23 13:30:04 nilsson Exp $
*/

/*
**! module Image
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

#include "operators.h"
#include "builtin_functions.h"
#include "interpret.h"
#include "svalue.h"
#include "mapping.h"
#include "pike_error.h"

#include "image.h"

#include "encodings.h"


#define sp Pike_sp

/* PNG module uses "type" for something else than what we want to use
   it for.  Rename "type" to "_type", and insert our own "type"...  */

static void fix_png_mapping(void)
{
  struct svalue *s;
  if(sp[-1].type != T_MAPPING) return;
  if((s = simple_mapping_string_lookup(sp[-1].u.mapping, "type"))) {
    push_text("_type");
    mapping_insert(sp[-2].u.mapping, &sp[-1], s);
    pop_stack();
  }
  push_text("type");
  push_text("image/png");
  mapping_insert(sp[-3].u.mapping, &sp[-2], &sp[-1]);
  pop_n_elems(2);
}


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
      Pike_error("Image.ANY.decode: illegal arguments\n");
   
   if (sp[-args].u.string->len<4)
      Pike_error("Image.ANY.decode: too short string\n");

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
	 push_text("Image.JPEG._decode");
	 SAFE_APPLY_MASTER("resolv_or_error",1);
	 stack_swap();
	 f_call_function(2);
	 return;

      case CHAR2('g','i'):
	 /* XCF */
	 push_text("Image.XCF._decode");
	 SAFE_APPLY_MASTER("resolv_or_error",1);
	 stack_swap();
	 f_call_function(2);
	 return;

      case CHAR2(137,'P'):
	 /* PNG */
	 push_text("Image.PNG._decode");
	 SAFE_APPLY_MASTER("resolv_or_error",1);
	 stack_swap();
	 f_call_function(2);
	 fix_png_mapping();
	 return;

      case CHAR2('G','I'):
	 /* GIF */
	 push_text("Image.GIF.decode_map");
	 SAFE_APPLY_MASTER("resolv_or_error", 1);
	 stack_swap();
	 f_call_function(2);
	 return;

      case CHAR2('8','B'):
	/* Photoshop (8BPS) */
	push_text("Image.PSD._decode");
	SAFE_APPLY_MASTER("resolv_or_error",1);
	stack_swap();
	f_call_function(2);
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

      case CHAR2('P','V'):
      case CHAR2('G','B'):
	 /* PVR */
	 image_pvr_f__decode(1);
	 return;

      case CHAR2(0x10,0):
         /* TIM */
	 image_tim_f__decode(1);
	 return;

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
	if( sp[-args].u.string->str[0] == 10 ) {
	  /* PCX */
	  image_pcx_decode(1);
	  push_text("image/x-pcx");
	  goto simple_image;
	}
unknown_format:
	 Pike_error("Unknown image format.\n");	 
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
      Pike_error("Image.ANY.decode_header: illegal arguments\n");
   
   if (sp[-args].u.string->len<4)
      Pike_error("Image.ANY.decode_header: too short string\n");

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
	 Pike_error("Image.ANY.decode: decoding of PNM header unimplemented\n");

      case CHAR2(255,216):
	 /* JFIF */
	 push_text("Image.JPEG.decode_header");
	 SAFE_APPLY_MASTER("resolv_or_error",1);
	 stack_swap();
	 f_call_function(2);
	 return;

      case CHAR2(137,'P'):
	 /* PNG */
	 push_text("Image.PNG.decode_header");
	 SAFE_APPLY_MASTER("resolv_or_error",1);
	 stack_swap();
	 f_call_function(2);
	 fix_png_mapping();
	 return;

      case CHAR2('g','i'):
	 /* XCF */
	 push_text("Image.XCF._decode");
	 SAFE_APPLY_MASTER("resolv_or_error",1);
	 stack_swap();
	 f_call_function(2);
	 return;

      case CHAR2('G','I'):
	 /* GIF */
	 push_text("Image.GIF.decode_map");
	 SAFE_APPLY_MASTER("resolv_or_error", 1);
	 stack_swap();
	 f_call_function(2);
	 return;

      case CHAR2('F','O'):
	 Pike_error("Image.ANY.decode: decoding of ILBM header unimplemented\n");

      case CHAR2('B','M'):
	 /* BMP */
	 img_bmp_decode_header(1);
	 return;

      case CHAR2(0x59,0xa6):
	 /* RAS */
	 Pike_error("Image.ANY.decode: decoding of RAS header unimplemented\n");

      case CHAR2('P','V'):
      case CHAR2('G','B'):
	 /* PVR */
	 image_pvr_f_decode_header(1);
	 return;

      case CHAR2(0x10,0):
	 /* TIM */
	 image_tim_f_decode_header(1);
	 return;

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
	 Pike_error("Unknown image format.\n");	 
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
  ADD_FUNCTION("_decode",image_any__decode,tFunc(tStr,tMapping), 0);
  ADD_FUNCTION("decode_header",image_any_decode_header,tFunc(tStr,tMapping),0);

  ADD_FUNCTION("decode",image_any_decode,tFunc(tStr,tObj),0);
  ADD_FUNCTION("decode_alpha",image_any_decode_alpha,tFunc(tStr,tObj),0);
}

void exit_image_any(void)
{
}
