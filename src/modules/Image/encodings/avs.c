#include "global.h"
#include "config.h"
#include <math.h>
#include <ctype.h>
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#include "stralloc.h"
RCSID("$Id: avs.c,v 1.3 1999/05/06 23:44:23 neotron Exp $");
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "threads.h"
#include "array.h"
#include "error.h"


#include "image.h"
#include "builtin_functions.h"
#include "module_support.h"

extern struct program *image_program;

/*
**! module Image
**! submodule AVS
**!
*/

/*
**! method object decode(string data)
**! method mapping _decode(string data)
**! method string encode(object image)
**!
**! Handle encoding and decoding of AVS-X images.
**! AVS is rather trivial, and not really useful, but:
**!
**! An AVS file is a raw (uncompressed) 24 bit image file with alpha.
**! The alpha channel is 8 bit, and there is no separate alpha for r,
**! g and b.
*/

struct program *image_encoding_avs_program;

void image_avs_f__decode(INT32 args)
{
  extern void f_aggregate_mapping(INT32 args);
  struct object *io, *ao;
  struct pike_string *s;
  unsigned int c;
  unsigned int w, h;
  unsigned char *q;
  get_all_args( "decode", args, "%S", &s);
  
  q = s->str;
  w = q[0]<<24 | q[1]<<16 | q[2]<<8 | q[3];
  h = q[4]<<24 | q[5]<<16 | q[6]<<8 | q[7];

  if( w <= 0 || h <= 0)
    error("This is not an AVS file (w=%d; h=%d)\n", w, h);

  if((unsigned)w*h*4+8 > (unsigned)s->len)
    error("This is not an AVS file (w=%d; h=%d; s=%d)\n",w,h,s->len);

  push_int( w );
  push_int( h );
  io = clone_object( image_program, 2);
  push_int( w );
  push_int( h );
  ao = clone_object( image_program, 2);

  for(c=0; c<w*h; c++)
  {
    rgb_group pix, apix;
    apix.r = apix.g = apix.b = q[c*4+8];
    pix.r = q[c*4+9];
    pix.g = q[c*4+10];
    pix.b = q[c*4+11];
    ((struct image *)io->storage)->img[c] = pix;
    ((struct image *)ao->storage)->img[c] = apix;
  }
  pop_n_elems(args);
  push_constant_text("image");
  push_object( io );
  push_constant_text("alpha");
  push_object( ao );
  f_aggregate_mapping( 4 );
}

void image_avs_f_decode(INT32 args)
{
  extern void f_index(INT32 args);
  image_avs_f__decode(args);
  push_constant_text("image");
  f_index(2);
}

void image_avs_f_encode(INT32 args )
{
  struct object *io, *ao=NULL;
  struct image *i, *a=NULL;
  rgb_group *is,*as=NULL;
  struct pike_string *s;
  int x,y;
  unsigned int *q;
  rgb_group apix = {255, 255, 255};
  get_all_args( "encode", args, "%o", &io);
  
  if(!(i = (struct image *)get_storage( io, image_program)))
    error("Wrong argument 1 to Image.AVS.encode\n");
  
  s = begin_shared_string( i->xsize*i->ysize*4+8 );
  MEMSET(s->str, 0, s->len );

  q = (int *)s->str;
  *(q++) = htonl( i->xsize );
  *(q++) = htonl( i->ysize );

  is = i->img;
  if(a) as = a->img;

  for(y=0; y<i->ysize; y++)
    for(x=0; x<i->xsize; x++)
    {
      register int rv = 0;
      rgb_group pix = *(is++);
      if(as) apix = *(as++);
      rv = ((apix.g<<24)|(pix.r<<16)|(pix.g<<8)|pix.b);
      *(q++) = htonl(rv);
    }
  pop_n_elems(args);
  push_string( end_shared_string(s) );
}

void init_image_avs()
{
  start_new_program();
  add_function( "decode",  image_avs_f_decode,  "function(string:object)", 0);
  add_function( "_decode", image_avs_f__decode, "function(string:mapping)", 0);
  add_function( "encode",  image_avs_f_encode,  "function(object:string)", 0);
  image_encoding_avs_program=end_program();
  push_object(clone_object(image_encoding_avs_program,0));
  {
    struct pike_string *s=make_shared_string("AVS");
    add_constant(s,sp-1,0);
    free_string(s);
  }
  pop_stack();
}

void exit_image_avs()
{
  if(image_encoding_avs_program)
  {
    free_program(image_encoding_avs_program);
    image_encoding_avs_program=NULL;
  }
}
