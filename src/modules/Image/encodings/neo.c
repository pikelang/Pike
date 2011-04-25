/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#include "global.h"
#include "program.h"
#include "module_support.h"
#include "interpret.h"
#include "builtin_functions.h"
#include "mapping.h"
#include "operators.h"

#include "image_machine.h"
#include "atari.h"


extern struct program *image_program;

/*! @module Image
 */

/*! @module NEO
 *! Decodes images from the Atari image editor Neochrome.
 */

/*! @decl mapping _decode(string data)
 *! Low level decoding of the NEO file contents in @[data].
 *! @returns
 *!   @mapping
 *!     @member Image.Image "image"
 *!       The decoded bitmap
 *!     @member array(Image.Image) "images"
 *!       Coler cycled images.
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
 *!     @member array(array(int(0..255))) "palette"
 *!       The palette to be used for color cycling.
 *!   @endmapping
 */
void image_neo_f__decode(INT32 args)
{
  unsigned int i, res, size = 0;
  struct atari_palette *pal=0;
  struct object *img;

  struct pike_string *s, *fn;
  unsigned char *q;

  get_all_args( "decode", args, "%S", &s );
  if(s->len!=32128)
    Pike_error("This is not a NEO file (wrong file size).\n");

  q = (unsigned char *)s->str;
  res = q[3];

  if(q[2]!=0 || (res!=0 && res!=1 && res!=2))
    Pike_error("This is not a NEO file (invalid resolution).\n");

  /* Checks done... */
  add_ref(s);
  pop_n_elems(args);

  if(res==0)
    pal = decode_atari_palette(q+4, 16);
  else if(res==1)
    pal = decode_atari_palette(q+4, 4);

  push_constant_text("palette");
  for( i=0; i<pal->size; i++ ) {
    push_int(pal->colors[i].r);
    push_int(pal->colors[i].g);
    push_int(pal->colors[i].b);
    f_aggregate(3);
  }
  f_aggregate(pal->size);
  size += 2;

  img = decode_atari_screendump(q+128, res, pal);

  push_constant_text("image");
  push_object(img);
  size += 2;

  if(q[48]&128) {
    int rl, ll, i;
    rl = q[49]&0xf;
    ll = (q[49]&0xf0)>>4;

    push_constant_text("right_limit");
    push_int( rl );
    push_constant_text("left_limit");
    push_int( ll );
    push_constant_text("speed");
    push_int( q[51] );
    push_constant_text("direction");
    if( q[50]&128 )
      push_constant_text("right");
    else
      push_constant_text("left");

    push_constant_text("images");
    for(i=0; i<rl-ll+1; i++) {
      if( q[50]&128 )
	rotate_atari_palette(pal, ll, rl);
      else
	rotate_atari_palette(pal, rl, ll);
      img = decode_atari_screendump(q+128, res, pal);
      push_object(img);
    }
    f_aggregate(rl-ll+1);

    size += 10;
  }

  if(pal)
  {
    free(pal->colors);
    free(pal);
  }

  fn = make_shared_binary_string((const char *)q+36, 12);

  push_constant_text("filename");
  push_string(fn);
  size += 2;

  free_string(s);
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
  ADD_FUNCTION("decode",  image_neo_f_decode,  tFunc(tStr,tObj),  0);
  ADD_FUNCTION("_decode", image_neo_f__decode,
	       tFunc(tStr,tMap(tStr,tMix)), 0);
}

void exit_image_neo()
{
}
