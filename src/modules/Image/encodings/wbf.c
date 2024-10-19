/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"

#include <math.h>
#include <ctype.h>

#include "stralloc.h"
#include "object.h"
#include "mapping.h"
#include "interpret.h"
#include "operators.h"
#include "svalue.h"
#include "pike_error.h"
#include "builtin_functions.h"
#include "program.h"

#include "image.h"
#include "module_support.h"


#define sp Pike_sp

extern struct program *image_program;

/*! @module Image
 */

/*! @module WBMP
 *! Wireless Application Protocol Bitmap Format - WAP bitmap format - WBMP.
 *!
 *! Wireless Application Protocol Bitmap Format (shortened to Wireless
 *! Bitmap and with file extension .wbmp) is a monochrome graphics
 *! file format that was optimized for the extremely low-end mobile
 *! computing devices that were common in the early days of the mobile web.
 */

struct buffer
{
  size_t len;
  unsigned char *str;
};

struct ext_header
{
  struct ext_header *next;
  char name[8];
  char value[16];
  char name_len;
  char value_len;
};

struct wbf_header
{
  unsigned int width;
  unsigned int height;
  int type;
  int header;
  int fix_header_field;
  int ext_header_field;
  struct ext_header *first_ext_header;
};

static void read_string( struct buffer *from, unsigned int len, char *to )
{
  if( from->len < len )
    Pike_error("Invalid data format\n");
  memcpy( from->str, to, len );
  from->str += len;
  from->len -= len;
}

static unsigned char read_uchar( struct buffer *from )
{
  unsigned char res = 0;
  if(from->len)
  {
    res = from->str[0];
    from->str++;
    from->len--;
  } else
    Pike_error("Invalid data format\n");
  return res;
}

static int wbf_read_int( struct buffer *from )
{
  int res = 0;
  while( 1 )
  {
    int i = read_uchar( from );
    res <<= 7;
    res |= i&0x7f;
    if( !(i & 0x80 ) )
      break;
  }
  return res;
}

static void push_ext_header( struct ext_header *eh )
{
  push_static_text( "identifier" );
  push_string( make_shared_binary_string( eh->name, eh->name_len ) );
  push_static_text( "value" );
  push_string( make_shared_binary_string( eh->value, eh->value_len ) );
  f_aggregate_mapping( 4 );
}

static void free_wbf_header_contents( struct wbf_header *wh )
{
  while( wh->first_ext_header )
  {
    struct ext_header *eh = wh->first_ext_header;
    wh->first_ext_header = eh->next;
    free( eh );
  }
}

static struct wbf_header decode_header( struct buffer *data )
{
  struct wbf_header res;
  ONERROR err;
  memset( &res, 0, sizeof(res) );
  res.type = wbf_read_int( data );
  res.fix_header_field = read_uchar( data );
  SET_ONERROR(err, free_wbf_header_contents, &res);

  if( res.fix_header_field & 0x80 )
  {
    switch( (res.fix_header_field>>5) & 0x3 )
    {
     case 0: /* Single varint extra header */
       res.ext_header_field = wbf_read_int( data );
       break;
     case 1: /* reserved */
     case 2: /* reserved */
     case 3: /* Array of parameter/value */
       {
         int q = 0x80;

         while( q & 0x80 )
         {
           struct ext_header *eh;
           q = read_uchar( data );
           eh = xcalloc( 1, sizeof( struct ext_header ) );
           eh->next = res.first_ext_header;
           res.first_ext_header = eh;
           eh->name_len = ((q>>4) & 0x7) + 1;
           eh->value_len = (q & 0xf) + 1;
           read_string( data, eh->name_len, eh->name );
           read_string( data, eh->value_len, eh->value );
         }
       }
    }
  }
  res.width = wbf_read_int( data );
  res.height = wbf_read_int( data );
  UNSET_ONERROR(err);
  return res;
}

static void low_image_f_wbf_decode_type0( struct wbf_header *wh,
                                          struct buffer *buff )
{
  unsigned int x, y;
  struct image *i;
  struct object *io;
  unsigned int rowsize = (wh->width+7) / 8;
  rgb_group white;
  rgb_group *id;
  push_int( wh->width );
  push_int( wh->height );
  io = clone_object( image_program, 2 );
  i = get_storage(io,image_program);
  id = i->img;

  white.r = 255;
  white.g = 255;
  white.b = 255;

  for( y = 0; y<wh->height; y++ )
  {
    unsigned char q = 0; /* avoid warning */
    unsigned char *data = buff->str + y * rowsize;
    if( buff->len < rowsize * (y + 1) )
      break;
    for( x = 0; x<wh->width; x++ )
    {
      if( !(x % 8) )
        q = data[x/8];
      else
        q <<= 1;
      if( q & 128 )
        *id = white;
      id++;
    }
  }
  push_object( io );
}

static void low_image_f_wbf_decode( int args, int mode )
{
  struct pike_string *s;
  struct wbf_header wh;
  int map_num_elems = 0;
  struct buffer buff;

  get_all_args( NULL, args, "%n", &s );

  buff.len = s->len;
  buff.str = (unsigned char *)s->str;
  sp--; /* Evil me */

  wh = decode_header( &buff );

  switch( wh.type )
  {
   case 0:
     /*
        The only supported format. B/W uncompressed bitmap

         No compresson.
         Colour: 1 = white, 0 = black.
         Depth: 1 bit.
         high bit = rightmost.
         first byte is upper left corner of image.
         each row is padded to one byte
      */
     switch( mode )
     {
      case 2: /* Image only */
        low_image_f_wbf_decode_type0( &wh, &buff );
        return;

      case 1: /* Image and header */
        push_static_text( "image" );
        low_image_f_wbf_decode_type0( &wh, &buff );
        map_num_elems++;
	/* FALLTHRU */

      case 0: /* Header only */
        push_static_text( "format" );
        push_static_text( "image/x-wap.wbmp; type=0" );
        map_num_elems++;

        push_static_text( "xsize" );
        push_int( wh.width );
        map_num_elems++;

        push_static_text( "ysize" );
        push_int( wh.height );
        map_num_elems++;

        if( wh.fix_header_field )
        {
          push_static_text( "fix_header_field" );
          push_int( wh.fix_header_field );
          map_num_elems++;
        }

        if( wh.ext_header_field )
        {
          push_static_text( "ext_header_field" );
          push_int( wh.ext_header_field );
          map_num_elems++;
        }

        if( wh.first_ext_header )
        {
          int num_headers = 0;
          struct ext_header *eh = wh.first_ext_header;
          while( eh )
          {
            push_ext_header( eh );
            eh = eh->next;
            num_headers++;
          }
          f_aggregate( num_headers );
          f_reverse( 1 );
          map_num_elems++;
        }
        f_aggregate_mapping( map_num_elems * 2 );
     }
     free_string( s );
     free_wbf_header_contents( &wh );
     break;

   default:
     free_string( s );
     free_wbf_header_contents( &wh );

     Pike_error("Unsupported wbf image type.\n");
  }
}

/*! @decl Image.Image decode(string image)
 *!
 *! Decode the given image
 */

static void image_f_wbf_decode( int args )
{
  low_image_f_wbf_decode( args, 2 );
}

/*! @decl mapping _decode(string image)
 *!
 *! Decode the given image
 *! @returns
 *!   @mapping
 *!     @member Image.Image "img"
 *!     @member string "format"
 *!       The MIME type and encoding for the image, e.g. "image/x-wap.wbmp; type=0".
 *!     @member int "xsize"
 *!     @member int "ysize"
 *!     @member int "fix_header_field"
 *!     @member int "ext_header_field"
 *!  @endmapping
 */

static void image_f_wbf__decode( int args )
{
  low_image_f_wbf_decode( args, 1 );
}

/*! @decl mapping decode_header(string image)
 *!
 *! @returns
 *!   @mapping
 *!     @member string "format"
 *!       The MIME type and encoding for the image, e.g. "image/x-wap.wbmp; type=0".
 *!     @member int "xsize"
 *!     @member int "ysize"
 *!     @member int "fix_header_field"
 *!     @member int "ext_header_field"
 *!  @endmapping
 */

static void image_f_wbf_decode_header( int args )
{
  low_image_f_wbf_decode( args, 0 );
}

void push_wap_integer( unsigned int i )
{
  char data[10]; /* More than big enough... */
  int pos = 0;

  if( !i )
  {
    data[0] = 0;
    pos=1;
  }

  while( i )
  {
    data[pos] = (i&0x7f) | 0x80;
    i>>=7;
    pos++;
  }
  data[0] &= 0x7f;
  push_string( make_shared_binary_string( data, pos ) );
  f_reverse(1);
}

static void push_wap_type0_image_data( struct image *i )
{
  int x, y;
  unsigned char *data, *p;
  rgb_group *is;
  data = xcalloc( i->ysize, (i->xsize+7)/8 );
  is = i->img;
  for( y = 0; y<i->ysize; y++ )
  {
    p = data + (i->xsize+7)/8*y;
    for( x = 0; x<i->xsize; x++ )
    {
      if( is->r || is->g || is->b )
        p[x/8] |= 128 >> (x%8);
      is++;
    }
  }
  push_string( make_shared_binary_string( (char *)data,
					  i->ysize * (i->xsize+7)/8 ) );
}

/*! @decl string encode(object image, void|mapping args)
 *! @decl string _encode(object image, void|mapping args)
 *!
 *! Encode the given image as a WBMP image. All pixels that are not
 *! black will be encoded as white.
 */

static void image_f_wbf_encode( int args )
{
  struct object *o;
  struct image *i;
  struct mapping *options = NULL;
  int num_strings = 0;

  if( !args )
    Pike_error("No image given to encode.\n");
  if( args > 2 )
    Pike_error("Too many arguments to encode.\n");
  if( TYPEOF(sp[-args]) != T_OBJECT )
    Pike_error("No image given to encode.\n");

  o = sp[-args].u.object;
  i = get_storage(o,image_program);
  if(!i)
    Pike_error("Wrong type object argument\n");
  if( args == 2 )
  {
    if( TYPEOF(sp[-args+1]) != T_MAPPING )
      Pike_error("Wrong type for argument 2.\n");
    options = sp[-args+1].u.mapping;
  }
  sp-=args;

  num_strings=0;
  push_wap_integer( 0 ); num_strings++; /* type */
  push_wap_integer( 0 ); num_strings++; /* extra header */
  push_wap_integer( i->xsize ); num_strings++;
  push_wap_integer( i->ysize ); num_strings++;
  push_wap_type0_image_data( i ); num_strings++;
  f_add( num_strings );
  if( options ) free_mapping( options );
  free_object( o );
}

/*! @endmodule
 */

/*! @endmodule
 */

void init_image_wbf(void)
{
  ADD_FUNCTION( "encode", image_f_wbf_encode,
		tFunc(tObj tOr(tVoid,tMapping),tStr), 0);
  ADD_FUNCTION( "_encode", image_f_wbf_encode,
		tFunc(tObj tOr(tVoid,tMapping),tStr), 0);
  ADD_FUNCTION( "decode", image_f_wbf_decode, tFunc(tStr,tObj), 0);
  ADD_FUNCTION( "_decode", image_f_wbf__decode, tFunc(tStr,tMapping), 0);
  ADD_FUNCTION( "decode_header", image_f_wbf_decode_header,
		tFunc(tStr,tMapping), 0);
}

void exit_image_wbf(void)
{
}
