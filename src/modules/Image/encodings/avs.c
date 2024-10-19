/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
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
#include "object.h"
#include "interpret.h"
#include "pike_error.h"
#include "mapping.h"
#include "operators.h"
#include "bignum.h"

#include "image.h"
#include "builtin_functions.h"
#include "module_support.h"


extern struct program *image_program;

/*
**! module Image
**! submodule AVS
**!
**! An AVS file is a raw (uncompressed) 24 bit image file with alpha.
**!
**! The file starts with width and height as 32-bit ingegers, followed
**! by 4 x width x height bytes of image data. The format is big
**! endian.
*/

/*
**! method object decode(string data)
**! method mapping _decode(string data)
**! method string encode(object image, object|void alpha)
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
  unsigned int len;
  int w, h;
  unsigned char *q;
  rgb_group *img_i, *img_a;

  get_all_args( NULL, args, "%n", &s);

  if (s->len < 12)	/* Width + height + one pixel */
    Pike_error("This is not an AVS file.\n");

  q = (unsigned char *)s->str;
  w = q[0]<<24 | q[1]<<16 | q[2]<<8 | q[3];
  h = q[4]<<24 | q[5]<<16 | q[6]<<8 | q[7];

  /* Check for under- and overflow. */
  if ((w <= 0) || (h <= 0) || INT32_MUL_OVERFLOW(w, h) ||
      INT32_MUL_OVERFLOW(w*h, 4) || ((w * h) & -0x40000000))
    Pike_error("This is not an AVS file (w=%d; h=%d)\n", w, h);

  if((size_t)w*h*4 != (size_t)(s->len-8))
    Pike_error("This is not an AVS file (w=%d; h=%d; s=%ld)\n",
          w, h, (long)s->len);

  push_int( w );
  push_int( h );
  io = clone_object( image_program, 2);
  push_int( w );
  push_int( h );
  ao = clone_object( image_program, 2);

  img_i = ((struct image *)io->storage)->img;
  img_a = ((struct image *)ao->storage)->img;

  len = (s->len - 8) / 4;

  for(c=0; c< len; c++)
  {
    rgb_group pix, apix;
    apix.r = apix.g = apix.b = q[c*4+8];
    pix.r = q[c*4+9];
    pix.g = q[c*4+10];
    pix.b = q[c*4+11];
    img_i[c] = pix;
    img_a[c] = apix;
  }
  pop_n_elems(args);
  push_static_text("image");
  push_object( io );
  push_static_text("alpha");
  push_object( ao );
  f_aggregate_mapping( 4 );
}

void image_avs_f_decode(INT32 args)
{
  image_avs_f__decode(args);
  push_static_text("image");
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
  get_all_args( NULL, args, "%o.%o", &io, &ao);

  if(!(i = get_storage( io, image_program)))
    Pike_error("Wrong argument 1 to Image.AVS.encode\n");

  if(ao) {
    if (!(a = get_storage( ao, image_program)))
      Pike_error("Wrong argument 2 to Image.AVS.encode\n");
    if ((a->xsize != i->xsize) || (a->ysize != i->ysize))
      Pike_error("Bad size for alpha channel to Image.AVS.encode.\n");
  }

  s = begin_shared_string( i->xsize*i->ysize*4+8 );
  memset(s->str, 0, s->len );

  q = (unsigned int *)s->str;
  *(q++) = htonl( i->xsize );
  *(q++) = htonl( i->ysize );

  is = i->img;
  if(a) as = a->img;

  for(y=0; y<i->ysize; y++)
    for(x=0; x<i->xsize; x++)
    {
      unsigned int rv = 0;
      rgb_group pix = *(is++);
      if(as) apix = *(as++);
      rv = ((apix.g<<24)|(pix.r<<16)|(pix.g<<8)|pix.b);
      *(q++) = htonl(rv);
    }
  pop_n_elems(args);
  push_string( end_shared_string(s) );
}

void init_image_avs(void)
{
  ADD_FUNCTION( "decode",  image_avs_f_decode,  tFunc(tStr,tObj), 0);
  ADD_FUNCTION( "_decode", image_avs_f__decode, tFunc(tStr,tMapping), 0);
  ADD_FUNCTION( "encode",  image_avs_f_encode,
		tFunc(tObj tOr(tObj,tVoid),tStr), 0);
}

void exit_image_avs(void)
{
}
