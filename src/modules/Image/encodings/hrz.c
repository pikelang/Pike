/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: hrz.c,v 1.11 2004/10/07 22:49:57 nilsson Exp $
*/

#include "global.h"

#include <math.h>
#include <ctype.h>

#include "stralloc.h"
#include "object.h"
#include "mapping.h"
#include "interpret.h"
#include "pike_error.h"
#include "builtin_functions.h"

#include "image.h"
#include "module_support.h"


extern struct program *image_program;

/*
**! module Image
**! submodule HRZ
**!
*/

/*
**! method object decode(string data)
**! method mapping _decode(string data)
**! method string encode(object image)
**!
**! Handle encoding and decoding of HRZ images.
**! HRZ is rather trivial, and not really useful, but:
**!
**! The HRZ file is always 256x240 with RGB values from 0 to 63.
**! No compression, no header, just the raw RGB data.
**! HRZ is (was?) used for amatuer radio slow-scan TV.
*/

void image_hrz_f_decode(INT32 args)
{
  struct object *io;
  struct pike_string *s;
  int c;
  get_all_args( "decode", args, "%S", &s);
  
  if(s->len != 256*240*3) Pike_error("This is not a HRZ file\n");

  push_int( 256 );
  push_int( 240 );
  io = clone_object( image_program, 2);

  for(c=0; c<256*240; c++)
  {
    rgb_group pix;
    pix.r = s->str[c*3]<<2 | s->str[c*3]>>4;
    pix.g = s->str[c*3+1]<<2 | s->str[c*3+1]>>4;
    pix.b = s->str[c*3+2]<<2 | s->str[c*3+2]>>4;
    ((struct image *)io->storage)->img[c] = pix;
  }
  pop_n_elems(args);
  push_object( io );
}

void image_hrz_f__decode(INT32 args)
{
  image_hrz_f_decode(args);
  push_constant_text("image");
  stack_swap();
  f_aggregate_mapping(2);
}

void image_hrz_f_encode(INT32 args )
{
  struct object *io;
  struct image *i;
  struct pike_string *s;
  int x,y;
  get_all_args( "encode", args, "%o", &io);
  
  if(!(i = (struct image *)get_storage( io, image_program)))
    Pike_error("Wrong argument 1 to Image.HRZ.encode\n");
  
  s = begin_shared_string( 256*240*3 );
  
  MEMSET(s->str, 0, s->len );
  for(y=0; y<240; y++)
    if(y < i->ysize)
      for(x=0; x<256; x++)
        if(x < i->xsize)
        {
          int in = (x + y*256)*3;
          rgb_group pix = i->img[y*i->xsize+x];
          s->str[in+0] = pix.r >> 2;
          s->str[in+1] = pix.g >> 2;
          s->str[in+2] = pix.b >> 2;
        }
  pop_n_elems(args);
  push_string( end_shared_string(s) );
}

void init_image_hrz()
{
  add_function( "decode", image_hrz_f_decode, "function(string:object)", 0);
  add_function( "_decode", image_hrz_f__decode, "function(string:mapping)", 0);
  add_function( "encode", image_hrz_f_encode, "function(object:string)", 0);
}

void exit_image_hrz()
{
}
