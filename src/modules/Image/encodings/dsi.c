/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: dsi.c,v 1.10 2005/01/23 13:30:04 nilsson Exp $
*/

/* Dream SNES Image file */

#include "global.h"
#include "image_machine.h"

#include "object.h"
#include "interpret.h"
#include "object.h"
#include "svalue.h"
#include "interpret.h"
#include "svalue.h"
#include "mapping.h"
#include "pike_error.h"
#include "builtin_functions.h"
#include "operators.h"

#include "image.h"
#include "colortable.h"


#define sp Pike_sp

extern struct program *image_program;

void f__decode( INT32 args )
{
  int xs, ys, x, y;
  unsigned char *data, *dp;
  size_t len;
  struct object *i, *a;
  struct image *ip, *ap;
  rgb_group black = {0,0,0};
  if( sp[-args].type != T_STRING )
    Pike_error("Illegal argument 1 to Image.DSI._decode\n");
  data = (unsigned char *)sp[-args].u.string->str;
  len = (size_t)sp[-args].u.string->len;

  if( len < 10 ) Pike_error("Data too short\n");

  xs = data[0] | (data[1]<<8) | (data[2]<<16) | (data[3]<<24);
  ys = data[4] | (data[5]<<8) | (data[6]<<16) | (data[7]<<24);

  if( (xs * ys * 2) != (ptrdiff_t)(len-8) )
    Pike_error("Not a DSI %d * %d + 8 != %ld\n",
	  xs, ys,
	  DO_NOT_WARN((long)len));

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
        p.r = (r*255)/31;
        p.g = (g*255)/63;
        p.b = (b*255)/31;
        ip->img[ x + y*xs ] = p;
      }
    }

  push_constant_text( "image" );
  push_object( i );
  push_constant_text( "alpha" );
  push_object( a );
  f_aggregate_mapping( 4 );
}

void f_decode( INT32 args )
{
  f__decode( args );
  push_constant_text( "image" );
  f_index( 2 );
}

void init_image_dsi()
{
  ADD_FUNCTION("_decode", f__decode, tFunc(tStr,tMapping), 0);
  ADD_FUNCTION("decode", f_decode, tFunc(tStr,tObj), 0);
}


void exit_image_dsi()
{
}
