/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: neo.c,v 1.4 2003/10/24 23:33:08 nilsson Exp $
*/

#include "global.h"
#include "image_machine.h"

#include "stralloc.h"
RCSID("$Id: neo.c,v 1.4 2003/10/24 23:33:08 nilsson Exp $");
#include "atari.h"

/* MUST BE INCLUDED LAST */
/* #include "module_magic.h" */

extern struct program *image_program;

/*! @module Image
 */

/*! @module NEO
 */

/*! @decl mapping _decode(string data)
 *! @returns
 *!   @mapping
 *!     @member Image.Image "image"
 *!       The decoded bitmap
 *!     @member string "filename"
 *!       The filename stored into the file.
 *!     @member int(0..15) "right_limit"
 *!     @member int(0..15) "left_limit"
 *!       The palette color range to be color cycled.
 *!     @member int(0..255) "speed"
 *!       The animation speed, expressed as the number of VBLs
 *!       per animation frame.
 *!     @member string "direction"
 *!       Color cycling direction. Can be either @expr{"left"@}
 *!       or @expr{"right"@}.
 *!   @endmapping
 */
void image_neo_f__decode(INT32 args)
{
  unsigned int res, size = 0;
  struct atari_palette *pal=0;
  struct object *img;

  struct pike_string *s, *fn;
  const unsigned char *q;

  get_all_args( "decode", args, "%S", &s );
  if(s->len!=32128)
    Pike_error("This is not a NEO file (wrong file size).\n");

  q = (unsigned char *)s->str;
  res = q[3];

  if(q[2]!=0 || (res!=0 && res!=1 && res!=2))
    Pike_error("This is not a NEO file (invalid resolution).\n");

  /* Checks done... */
  stack_pop_n_elems_keep_top(args);

  if(res==0)
    pal = decode_atari_palette(q+4, 16);
  else if(res==1)
    pal = decode_atari_palette(q+4, 4);

  /* FIXME: Push palette */

  img = decode_atari_screendump(q+128, res, pal);
  if(pal)
  {
    free(pal->colors);
    free(pal);
  }

  push_constant_text("image");
  push_object(img);
  size += 2;

  fn = make_shared_binary_string(q+36, 12);

  push_constant_text("filename");
  push_string(fn);
  size += 2;

  if(q[48]&128) {
    push_constant_text("right_limit");
    push_int( q[49]&0xf );
    push_constant_text("left_limit");
    push_int( (q[49]&0xf0)>>4 );
    push_constant_text("speed");
    push_int( q[51] );
    push_constant_text("direction");
    if( q[50]&128 )
      push_constant_text("right");
    else
      push_constant_text("left");
    size += 8;
  }

  f_aggregate_mapping(size);
}

/*! @decl Image.Image decode(string data)
 *! Decodes the image @[data] into an @[Image.Image] object.
 */
void image_neo_f_decode(INT32 args)
{
  image_neo_f__decode(args);
  push_constant_text("image");
  f_index(2);
}

/*! @endmodule
 */

/*! @endmodule
 */

void init_image_neo()
{
  add_function( "decode",  image_neo_f_decode,  "function(string:object)",  0);
  add_function( "_decode", image_neo_f__decode, "function(string:mapping)", 0);
}

void exit_image_neo()
{
}
