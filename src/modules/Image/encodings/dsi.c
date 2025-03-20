/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

/*! @module Image
 *!
 *! @module DSI
 *!
 *! Decode-only support for the Dream SNES image file format.
 *!
 *! This is a little-endian 16 bitplane image format that starts with
 *! two 32-bit integers, width and height, followed by w*h*2 bytes of
 *! image data.
 *!
 *! Each pixel is r5g6b5, a special case is the color r=31,g=0,b=31
 *! (full red, full blue, no green), which is transparent
 *! (chroma-keying)
 */
/* Dream SNES Image file */

#include "global.h"
#include "image_machine.h"

#include "object.h"
#include "interpret.h"
#include "svalue.h"
#include "mapping.h"
#include "pike_error.h"
#include "builtin_functions.h"
#include "operators.h"
#include "bignum.h"

#include "image.h"
#include "colortable.h"


#define sp Pike_sp

extern struct program *image_program;

/*! @decl mapping(string:Image.Image) _decode(string data)
 *!  Decode the DSI image.
 *!
 *! This function will treat pixels with full red, full blue, no green
 *! as transparent.
 */
static void f__decode( INT32 args )
{
  int xs, ys, x, y;
  unsigned char *data, *dp;
  size_t len;
  struct object *i, *a;
  struct image *ip, *ap;
  rgb_group black = {0,0,0};
  if( !args || TYPEOF(sp[-args]) != T_STRING )
    Pike_error("Illegal argument 1 to Image.DSI._decode\n");
  data = (unsigned char *)sp[-args].u.string->str;
  len = (size_t)sp[-args].u.string->len;

  if( len < 10 ) Pike_error("Data too short\n");

  xs = data[0] | (data[1]<<8) | (data[2]<<16) | (data[3]<<24);
  ys = data[4] | (data[5]<<8) | (data[6]<<16) | (data[7]<<24);

  if( (xs * ys * 2) != (ptrdiff_t)(len-8) ||
      INT32_MUL_OVERFLOW(xs, ys) || ((xs * ys) & -0x8000000) )
    Pike_error("Not a DSI %d * %d + 8 != %ld\n",
          xs, ys, (long)len);

  push_int( xs );
  push_int( ys );
  push_int( 255 );
  push_int( 255 );
  push_int( 255 );
  a = clone_object( image_program, 5 );
  push_int( xs );
  push_int( ys );
  i = clone_object( image_program, 2 );
  ip = (struct image *)i->storage;
  ap = (struct image *)a->storage;

  dp = data+8;
  for( y = 0; y<ys; y++ )
    for( x = 0; x<xs; x++,dp+=2 )
    {
      unsigned short px = dp[0] | (dp[1]<<8);
      int r, g, b;
      rgb_group p;
      if( px == ((31<<11) | 31) )
        ap->img[ x + y*xs ] = black;
      else
      {
        r = ((px>>11) & 31);
        g = ((px>>5) & 63);
        b = ((px) & 31);
        p.r = (r * 0x21)>>2;	/* (r<<3) | (r>>2) */
        p.g = (g * 0x41)>>4;	/* (g<<2) | (g>>4) */
        p.b = (b * 0x21)>>2;	/* (b<<3) | (b>>2) */
        ip->img[ x + y*xs ] = p;
      }
    }

  push_static_text( "image" );
  push_object( i );
  push_static_text( "alpha" );
  push_object( a );
  f_aggregate_mapping( 4 );
}

/*! @decl Image.Image decode(string data)
 *!  Decode the DSI image, without alpha decoding.
 */
static void f_decode( INT32 args )
{
  f__decode( args );
  push_static_text( "image" );
  f_index( 2 );
}

/*! @decl string(8bit) encode(Image.Image img)
 *!
 *!  Encode the image to DSI format.
 *!
 *! @note
 *!  Pixels of color full red, no green, full blue (ie full magenta)
 *!  will be regarded as transparent by @[decode()]. Pixels that
 *!  would have mapped to that color (@expr{0xf81f@}) by the colorspace
 *!  reduction will instead be mapped to full red, min green, full blue
 *!  (@expr{0xf83f@}).
 */
static void f_encode(INT32 args)
{
  struct image *img = NULL;
  size_t x, y;
  rgb_group *s;
  struct pike_string *res = NULL;
  p_wchar0 *pos;

  if (!args) {
     SIMPLE_WRONG_NUM_ARGS_ERROR("encode", 1);
  }

  if (TYPEOF(Pike_sp[-args]) != T_OBJECT ||
      !(img = get_storage(Pike_sp[-args].u.object, image_program))) {
    SIMPLE_ARG_TYPE_ERROR("encode", 1, "Image.Image");
  }

  if (!img->img) {
    PIKE_ERROR("encode", "No image.\n", sp, args);
  }

  if ((img->xsize & ~0x7fffffff) || (img->ysize & ~0x7fffffff) ||
      INT32_MUL_OVERFLOW(img->xsize, img->ysize)) {
    PIKE_ERROR("encode", "Invalid dimensions.\n", sp, args);
  }

  res = begin_shared_string((img->xsize * img->ysize * 2) + 8);
  pos = STR0(res);

  x = (size_t)img->xsize;
  *pos++ = x & 0xff;
  *pos++ = (x>>8) & 0xff;
  *pos++ = (x>>16) & 0xff;
  *pos++ = (x>>24) & 0xff;

  y = (size_t)img->ysize;
  *pos++ = y & 0xff;
  *pos++ = (y>>8) & 0xff;
  *pos++ = (y>>16) & 0xff;
  *pos++ = (y>>24) & 0xff;

  s = img->img;
  for (y = (size_t)img->ysize; y--;) {
    for (x = (size_t)img->xsize; x--;) {
      unsigned char r = s->r>>3;
      unsigned char g = s->g>>2;
      unsigned char b = s->b>>3;
      unsigned int c = (r << 11) | (g << 5) | b;
      if ((c == 0xf81f) && ((s->r != 0xff) || s->g || (s->b != 0xff))) {
        /* Pixel got transparent due to color space reduction. */
        c = 0xf83f;
      }
      *pos++ = c & 0xff;
      *pos++ = (c>>8) & 0xff;
      s++;
    }
  }
  push_string(end_shared_string(res));
}

void init_image_dsi(void)
{
  ADD_FUNCTION("_decode", f__decode, tFunc(tStr,tMapping), 0);
  ADD_FUNCTION("decode", f_decode, tFunc(tStr,tObj), 0);
  ADD_FUNCTION("encode", f_encode, tFunc(tObj,tStr), 0);
}


void exit_image_dsi(void)
{
}
/*! @endmodule
 *!
 *! @endmodule
 */
