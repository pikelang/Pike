/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: avs.c,v 1.17 2004/03/06 00:06:59 nilsson Exp $
*/

#include "global.h"
#include "image_machine.h"
#include <math.h>
#include <ctype.h>

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include "stralloc.h"
RCSID("$Id: avs.c,v 1.17 2004/03/06 00:06:59 nilsson Exp $");
#include "object.h"
#include "interpret.h"
#include "pike_error.h"
#include "mapping.h"
#include "operators.h"

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

void image_avs_f__decode(INT32 args)
{
  struct object *io, *ao;
  struct pike_string *s;
  unsigned int c;
  unsigned int w, h;
  unsigned char *q;
  get_all_args( "decode", args, "%S", &s);
  
  q = (unsigned char *)s->str;
  w = q[0]<<24 | q[1]<<16 | q[2]<<8 | q[3];
  h = q[4]<<24 | q[5]<<16 | q[6]<<8 | q[7];

  if( w <= 0 || h <= 0)
    Pike_error("This is not an AVS file (w=%d; h=%d)\n", w, h);

  if((size_t)w*h*4+8 > (size_t)s->len)
    Pike_error("This is not an AVS file (w=%d; h=%d; s=%ld)\n",
	  w, h,
	  DO_NOT_WARN((long)s->len));

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
    Pike_error("Wrong argument 1 to Image.AVS.encode\n");
  
  s = begin_shared_string( i->xsize*i->ysize*4+8 );
  MEMSET(s->str, 0, s->len );

  q = (unsigned int *)s->str;
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
  add_function( "decode",  image_avs_f_decode,  "function(string:object)", 0);
  add_function( "_decode", image_avs_f__decode, "function(string:mapping)", 0);
  add_function( "encode",  image_avs_f_encode,  "function(object:string)", 0);
}

void exit_image_avs()
{
}
