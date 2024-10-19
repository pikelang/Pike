/* -*- mode: C; c-basic-offset: 3; -*-
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
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
#include "pike_types.h"

#include "image.h"

#include "encodings.h"


/*! @module Image
 */

/*! @module ANY
 *!
 *!  These method calls other decoding methods
 *!  and has some heuristics for what type of image
 *!  this is.
 *!
 */

#define sp Pike_sp
#define CHAR2(a,b) ((((unsigned char)(a))<<8)|((unsigned char)(b)))

/* PNG module uses "type" for something else than what we want to use
   it for.  Rename "type" to "_type", and insert our own "type"...  */

static void fix_png_mapping(void)
{
  struct svalue *s;
  if(TYPEOF(sp[-1]) != T_MAPPING) return;
  if((s = low_mapping_string_lookup(sp[-1].u.mapping, literal_type_string))) {
    /* s is a pointer into the mapping data, so we have to push it onto
     * the stack, as mapping_insert might rehash, which would lead to a
     * use after free.
     */
    push_static_text("_type");
    push_svalue(s);
    mapping_insert(sp[-3].u.mapping, &sp[-2], &sp[-1]);
    pop_n_elems(2);
  }
  ref_push_string(literal_type_string);
  push_static_text("image/png");
  mapping_insert(sp[-3].u.mapping, &sp[-2], &sp[-1]);
  pop_n_elems(2);
}


static void call_resolved( const char *function )
{
  push_text(function);
  APPLY_MASTER("resolv_or_error",1);
  stack_swap();
  f_call_function(2);
}

/*! @decl mapping _decode(string data)
 *! @decl object decode(string data)
 *! @decl object decode_alpha(string data)
 *!
 *! Tries heuristics to find the correct method
 *! of decoding the data, then calls that method.
 *!
 *! The result of _decode() is a mapping that contains
 *!
 *! @mapping
 *!   @member string "type"
 *!     File type information as MIME type (ie "image/jpeg" or similar)
 *!   @member Image.Image "image"
 *!     the image object
 *!   @member int alpha
 *!     the alpha channel or 0 if N/A
 *! @endmapping
 *!
 *! @note
 *!	Throws upon failure.
 */
void image_any__decode(INT32 args)
{
   if (args!=1 || TYPEOF(sp[-args]) != T_STRING)
      Pike_error("Image.ANY.decode: illegal arguments\n");

   if (sp[-args].u.string->len<4)
      Pike_error("Image.ANY.decode: too short string\n");

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
	 push_static_text("image/x-pnm");
	 goto simple_image;

      case CHAR2(255,216):
	 /* JFIF */
         call_resolved("Image.JPEG._decode");
	 return;

      case CHAR2('g','i'):
	 /* XCF */
         call_resolved("Image.XCF._decode");
	 return;

      case CHAR2(137,'P'):
	 /* PNG */
	 call_resolved("Image.PNG._decode");
	 fix_png_mapping();
	 return;

      case CHAR2('G','I'):
	 /* GIF */
	 call_resolved("Image.GIF.decode_map");
	 return;

      case CHAR2('8','B'):
	/* Photoshop (8BPS) */
	call_resolved("Image.PSD._decode");
	return;

      case CHAR2('F','O'):
	 /* ILBM (probably) */
	 img_ilbm_decode(1);
	 push_static_text("image/x-ilbm");
	 goto simple_image;

      case CHAR2('I','I'):	/* Little endian. */
      case CHAR2('M','M'):	/* Big endian. */
	/* TIFF */
        call_resolved("Image.TIFF._decode");
	return;

       case CHAR2('R','I'):
        /* RIFF, used for WebP */
        call_resolved("Image.WebP._decode");
        return;

      case CHAR2('B','M'):
	 /* BMP */
	 img_bmp__decode(1);
	 return;

      case CHAR2(0x59,0xa6):
	 /* RAS */
	 img_ras_decode(1);
	 push_static_text("image/x-sun-raster");
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

      case CHAR2(0xc5, 0xd0):
	/* FIXME: DOS EPS Binary File Header. */
	goto unknown_format;
	break;

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
	  push_static_text("image/x-pcx");
	  goto simple_image;
	}
unknown_format:
	 Pike_error("Unknown image format.\n");
   }

simple_image:
   /* on stack: object image,string type */
   f_aggregate(2);
   push_static_text("image");
   ref_push_string(literal_type_string);
   f_aggregate(2);
   stack_swap();
   f_mkmapping(2);
   return;
}


/*! @decl mapping decode_header(string data)
 *!
 *! Tries heuristics to find the correct method
 *! of decoding the header, then calls that method.
 *!
 *! The resulting mapping depends on wich decode_header method that
 *! is executed, but these keys will probably exist
 *!
 *! @mapping
 *!   @member int "xsize"
 *!   @member int "ysize"
 *!       Size of image
 *!   @member string "type"
 *!     File type information as MIME type.
 *!   @member string "color_space"
 *!     Color space of image.
 *! @endmapping
 *!
 *! @note
 *!	Throws upon failure.
 */
void image_any_decode_header(INT32 args)
{
   if (args!=1 || TYPEOF(sp[-args]) != T_STRING)
      Pike_error("Image.ANY.decode_header: illegal arguments\n");

   if (sp[-args].u.string->len<4)
      Pike_error("Image.ANY.decode_header: too short string\n");

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
        call_resolved("Image.JPEG.decode_header");
        return;

      case CHAR2(137,'P'):
	 /* PNG */
         call_resolved("Image.PNG.decode_header");
	 fix_png_mapping();
	 return;

      case CHAR2('g','i'):
	 /* XCF */
         call_resolved("Image.XCF._decode");
	 return;

      case CHAR2('G','I'):
	 /* GIF */
         call_resolved("Image.GIF.decode_map");
	 return;

      case CHAR2('F','O'):
	 Pike_error("Image.ANY.decode: decoding of ILBM header unimplemented\n");

      case CHAR2('I','I'):	/* Little endian. */
      case CHAR2('M','M'):	/* Big endian. */
	/* TIFF */
        call_resolved("Image.TIFF.decode_header");
	return;

       case CHAR2('R','I'):
           /* RIFF, used for WebP */
        call_resolved("Image.WebP._decode");
        return;

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

      case CHAR2(0xc5, 0xd0):
      case CHAR2('%','!'):
	/* PS */
        call_resolved("Image.PS.decode_header");
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
  char *resolve = NULL;

  if (args!=1 || TYPEOF(sp[-args]) != T_STRING)
      Pike_error("Image.ANY.decode: illegal arguments\n");

   if (sp[-args].u.string->len<4)
      Pike_error("Image.ANY.decode: too short string\n");

   /* Heuristics for code with image only decoder, for increased
      efficiency and safety. */
   switch (CHAR2(sp[-args].u.string->str[0],sp[-args].u.string->str[1]))
   {
   case CHAR2(255,216):
     /* JFIF */
     resolve = "Image.JPEG.decode";
     break;

   case CHAR2('g','i'):
     /* XCF */
     resolve = "Image.XCF.decode";
     break;

   case CHAR2('R','I'):
     /* RIFF */
     resolve = "Image.WebP.decode";
     break;

   case CHAR2('G','I'):
     /* GIF */
     resolve = "Image.GIF.decode";
     break;

   case CHAR2(137,'P'):
     /* PNG */
     resolve = "Image.PNG.decode";
     break;

   case CHAR2('8','B'):
     /* Photoshop (8BPS) */
     resolve = "Image.PSD.decode";
     break;

   case CHAR2('I','I'):	/* Little endian. */
   case CHAR2('M','M'):	/* Big endian. */
     /* TIFF */
     resolve = "Image.TIFF.decode";
     break;
   }

   if(resolve)
   {
     call_resolved( resolve );
     return;
   }

   image_any__decode(args);
   push_static_text("image");
   f_index(2);
}

void image_any_decode_alpha(INT32 args)
{
   image_any__decode(args);
   push_static_text("alpha");
   f_index(2);
}

/*! @endmodule
 */

/*! @endmodule
 */

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
