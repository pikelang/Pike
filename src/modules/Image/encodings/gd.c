#include "global.h"
#include "image_machine.h"
#include <math.h>
#include <ctype.h>

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "stralloc.h"
RCSID("$Id: gd.c,v 1.3 1999/05/30 20:12:08 mirar Exp $");
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "threads.h"
#include "array.h"
#include "error.h"
#include "operators.h"
#include "builtin_functions.h"
#include "module_support.h"


#include "image.h"

extern struct program *image_program;

/*
**! module Image
**! submodule GD
**!
**! 	Handle encoding and decoding of GD images.
**! 
**! 	GD is the internal format of libgd by Thomas Boutell,
**!     <a href=http://www.boutell.com/gd/><tt>http://www.boutell.com/gd/</tt></a>
**! 	It is a rather simple, uncompressed, palette
**!     format. 
*/

/*
**! method object decode(string data)
**! method object decode_alpha(string data)
**! method mapping decode_header(string data)
**! method mapping _decode(string data)
**!	decodes a GD image
**!
**!	The <ref>decode_header</ref> and <ref>_decode</ref>
**!	has these elements:
**!
**!	<pre>
**!        "image":object            - image object    \
**!	   "alpha":object            - decoded alpha   |- not decode_header
**!	   "colortable":object       - decoded palette /
**!	   
**!	   "type":"image/x-gd"       - image type
**!	   "xsize":int               - horisontal size in pixels
**!	   "ysize":int               - vertical size in pixels
**!	   "alpha_index":int         - index to transparancy in palette 
**!                                    -1 means no transparency
**!	   "colors":int              - numbers of colors
**!	</pre>
**!	  
**!
**! method string encode(object image)
**! method string encode(object image,mapping options)
**!	encode a GD image
**!
**!	options is a mapping with optional values:
**!	<pre>
**!	   "colortable":object       - palette to use (max 256 colors)
**!	   "alpha":object            - alpha channel (truncated to 1 bit)
**!	   "alpha_index":int         - index to transparancy in palette 
**!	</pre>
*/

void image_gd_f_encode(INT32 args)
{
   error("not implemented\n");
}

void img_gd_decode(INT32 args,int header_only)
{
   error("not implemented\n");
}

static void image_gd_f_decode(INT32 args)
{
   img_gd_decode(args,0);
   push_string(make_shared_string("image"));
   f_index(2);
}

static void image_gd_f_decode_alpha(INT32 args)
{
   img_gd_decode(args,0);
   push_string(make_shared_string("alpha"));
   f_index(2);
}

static void image_gd_f_decode_header(INT32 args)
{
   img_gd_decode(args,1);
}

static void image_gd_f__decode(INT32 args)
{
   img_gd_decode(args,0);
}

void init_image_gd()
{
  ADD_FUNCTION( "decode",  image_gd_f_decode,  tFunc(tStr,tObj), 0);
  ADD_FUNCTION( "decode_alpha",  image_gd_f_decode_alpha,  tFunc(tStr,tObj), 0);
  ADD_FUNCTION( "_decode", image_gd_f__decode, tFunc(tStr,tMapping), 0);
  ADD_FUNCTION( "encode",  image_gd_f_encode,  tFunc(tStr,tObj), 0);
  ADD_FUNCTION( "decode_header", image_gd_f_decode_header, tFunc(tStr,tMapping), 0);
}

void exit_image_gd()
{
}
