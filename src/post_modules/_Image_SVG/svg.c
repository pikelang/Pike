/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: svg.c,v 1.10 2004/10/07 22:19:12 nilsson Exp $
*/

#include "config.h"
#include "module.h"

#ifdef HAVE_SVG
#include "global.h"

#include "stralloc.h"
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "array.h"
#include "mapping.h"
#include "pike_error.h"
#include "threads.h"
#include "builtin_functions.h"
#include "module_support.h"
#include "operators.h"
#include "program.h"

#include "modules/Image/image.h"

#include <librsvg/rsvg.h>


/*! @module Image
 */

/*! @module SVG
 *! This is a glue against the librsvg-2.0 library,
 *! providing Scalable Vector Graphics (SVG) for Pike.
 */


static void _do_free_mapping( void *opts )
{
  if( !opts )
    return;
  free_mapping( (struct mapping *)opts );
}


static void do_resize( gint *width, gint *height, gpointer user_data )
{
  struct mapping *opts = (struct mapping *)user_data;
  struct svalue *opt;
  int xsize=0;
  
  if( !opts )
    return;

  /* Exact size specified */
  
  if( (opt = simple_mapping_string_lookup( opts, "xsize" ))
      && opt->type == PIKE_T_INT)
  {
    xsize = opt->u.integer;
    *height = (*height * xsize) / *width;
    *width = xsize;
  }

  if( (opt = simple_mapping_string_lookup( opts, "ysize" ))
      && opt->type == PIKE_T_INT)
  {
    if( xsize )
      *height = opt->u.integer;
    else
    {
      xsize = opt->u.integer;
      *width = (*width * xsize) / *height;
      *height = xsize;
    }
    return;
  }
  else if( xsize )
    return;


  /* Scale specified */
  if( (opt = simple_mapping_string_lookup( opts, "scale" ))
      && opt->type == PIKE_T_FLOAT )
  {
    *width *= opt->u.float_number;
    *height *= opt->u.float_number;
    return;
  }
  /* FIXME: Max size specified */
}

static void low__decode( INT32 args, int header_only )
{
  RsvgHandle *handle;
  GError *err = NULL;
  GdkPixbuf *res;
  struct pike_string *data;
  struct mapping *opts = 0;
  struct svalue *debugsp = Pike_sp;
  if( args > 2 )
    Pike_error("Too many arguments\n");
  
  if( args == 2 )
  {
    if( Pike_sp[-1].type != PIKE_T_MAPPING )
      Pike_error("Illegal argument 2, expected mapping\n");
    opts = Pike_sp[-1].u.mapping;
    Pike_sp--;
    args--;
  }

  if( Pike_sp[-1].type != PIKE_T_STRING )
    Pike_error("Illegal argument 1, expected string\n");

  f_string_to_utf8( 1 );
  data = Pike_sp[-1].u.string;
  handle = rsvg_handle_new( );

  if( !handle )
    Pike_error("rsvg_handle_new() failed\n");

  rsvg_handle_set_size_callback( handle, do_resize,
				 (void *)opts, _do_free_mapping );

  rsvg_handle_write( handle, (void *)data->str, data->len, &err );

  if( err )
  {
    rsvg_handle_free( handle );
    Pike_error("Failed to decode SVG\n");
  }
  rsvg_handle_close( handle, &err );

  if( err )
  {
    rsvg_handle_free( handle );
    Pike_error("Failed to decode SVG\n");
  }

  res = rsvg_handle_get_pixbuf( handle );
  pop_n_elems( args );

  if( !res )
  {
    rsvg_handle_free( handle );
    Pike_error("Failed to decode SVG\n");
  }
  
  {
    struct svalue *osp = Pike_sp;
    struct object *ao=0, *io=0;
    struct image *i=0, *a=0;
    int alpha = gdk_pixbuf_get_has_alpha( res ), x, y;
    int xs = gdk_pixbuf_get_width( res );
    int ys = gdk_pixbuf_get_height( res );
    int span = gdk_pixbuf_get_rowstride( res );
    guchar *data = gdk_pixbuf_get_pixels( res );
    
    push_text( "xsize" ); push_int( xs );
    push_text( "ysize" ); push_int( ys );
    push_text( "type" );  push_text( "image/svg");
    if( !header_only )
    {
      push_text( "Image.Image" );
      APPLY_MASTER( "resolv", 1 );
      push_int( xs );
      push_int( ys );
      apply_svalue( Pike_sp-3, 2 );
      io = Pike_sp[-1].u.object;
      i = (struct image*)io->storage;
      Pike_sp--;
      if( alpha )
      {
	push_int( xs );
	push_int( ys );
	apply_svalue( Pike_sp-3, 2 );
	ao = Pike_sp[-1].u.object;
	a = (struct image*)ao->storage;
	Pike_sp--;
      }
      pop_stack();


      for( y = 0; y<ys; y++ )
      {
	unsigned char *row = data + (y*span);
	rgb_group *ipix = (i->img + (y*xs));
	rgb_group *apix = a?(a->img + (y*xs)):0;
	for( x = 0; x<xs; x++ )
	{
	  ipix->r = *(row++);
	  ipix->g = *(row++);
	  ipix->b = *(row++);
	  ipix++;
	  if( apix )
	  {
	    apix->r = apix->g = apix->b = *(row++);
	    apix++;
	  }
	}
      }
      push_text( "image" ); push_object( io );
      if( ao )
      {
	push_text( "alpha" ); push_object( ao );
      }
    }
    f_aggregate_mapping( Pike_sp - osp );
  }
  rsvg_handle_free( handle );
}

static void f_decode_header( INT32 args )
/*! @decl mapping decode_header( string data, void|mapping options )
 *!  Data is the SVG data, the charset must be unicode.
 *!
 *!  If @[options] is specified, it contains one or more of the
 *!  following options
 *!
 *!  @mapping
 *!  @member int xsize
 *!   If specified, gives the exact width in pixels the image will
 *!   have. If only one of xsize or ysize is specified, the other
 *!   is calculated.
 *!  @member int ysize
 *!   If specified, gives the exact height in pixels the image will
 *!   have. If only one of xsize or ysize is specified, the other
 *!   is calculated.
 *!  @member float scale
 *!   If specified, gives the scale the image will be drawn with.
 *!   A scale of 2.0 will give an image that is twice as large.
 *!  @endmapping
 *!
 *!  The result is a mapping with the following members:
 *!  @mapping
 *!  @member string type
 *!    Always image/svg
 *!  @member int xsize
 *!    The width of the image, in pixels
 *!  @member int ysize
 *!    The height of the image, in pixels
 *!  @endmapping
 */

{
  low__decode( args, 1 );
}

static void f__decode( INT32 args )
/*! @decl mapping _decode( string data, void|mapping options )
 *!  Data is the SVG data, the charset must be unicode.
 *!
 *!  If @[options] is specified, it contains one or more of the
 *!  following options
 *!
 *!  @mapping
 *!  @member int xsize
 *!   If specified, gives the exact width in pixels the image will
 *!   have. If only one of xsize or ysize is specified, the other
 *!   is calculated.
 *!  @member int ysize
 *!   If specified, gives the exact height in pixels the image will
 *!   have. If only one of xsize or ysize is specified, the other
 *!   is calculated.
 *!  @member float scale
 *!   If specified, gives the scale the image will be drawn with.
 *!   A scale of 2.0 will give an image that is twice as large.
 *!  @endmapping
 *!
 *!  The result is a mapping with the following members:
 *!  @mapping
 *!  @member string type
 *!    Always image/svg
 *!  @member int xsize
 *!    The width of the image, in pixels
 *!  @member int ysize
 *!    The height of the image, in pixels
 *!  @member Image.Image image
 *!    The image data
 *!  @member Image.Image alpha
 *!    The alpha channel data
 *!  @endmapping
 */
{
  low__decode( args, 0 );
}

static void f_decode_layers( INT32 args )
/*! @decl array(Image.Layer) decode_layers( string data, void|mapping options )
 *!  Data is the SVG data, the charset must be unicode.
 *!
 *!  If @[options] is specified, it contains one or more of the
 *!  following options
 *!
 *!  @mapping
 *!  @member int xsize
 *!   If specified, gives the exact width in pixels the image will
 *!   have. If only one of xsize or ysize is specified, the other
 *!   is calculated.
 *!  @member int ysize
 *!   If specified, gives the exact height in pixels the image will
 *!   have. If only one of xsize or ysize is specified, the other
 *!   is calculated.
 *!  @member float scale
 *!   If specified, gives the scale the image will be drawn with.
 *!   A scale of 2.0 will give an image that is twice as large.
 *!  @endmapping
 *!
 *!  The result is an array of Image.Layer objects.
 *!  For now there is always at most one member in the array.
 */
{
  low__decode( args, 0 );
  /* stack: mapping */
  push_text( "Image.Layer" );
  APPLY_MASTER( "resolv", 1 );
  /* stack: mapping Image.Layer */
  stack_swap();
  apply_svalue( Pike_sp-2, 1 );
  f_aggregate( 1 );
}

static void f_decode( INT32 args )
/*! @decl Image.Image decode( string data, void|mapping options )
 *!  Data is the SVG data, the charset must be unicode.
 *!
 *!  If @[options] is specified, it contains one or more of the
 *!  following options
 *!
 *!  @mapping
 *!  @member int xsize
 *!   If specified, gives the exact width in pixels the image will
 *!   have. If only one of xsize or ysize is specified, the other
 *!   is calculated.
 *!  @member int ysize
 *!   If specified, gives the exact height in pixels the image will
 *!   have. If only one of xsize or ysize is specified, the other
 *!   is calculated.
 *!  @member float scale
 *!   If specified, gives the scale the image will be drawn with.
 *!   A scale of 2.0 will give an image that is twice as large.
 *!  @endmapping
 *!
 *!  Returns the image member of the mapping returned by _decode
 */
{
  low__decode( args, 0 );
  push_text( "image" );
  f_index( 2 );
}
#endif
  
PIKE_MODULE_INIT
{
#ifdef HAVE_SVG
  g_type_init(); /* Initialize the glib type system. */

  add_function( "decode", f_decode,
		"function(string,mapping|void:object)", 0 );
  add_function( "_decode", f__decode,
		"function(string,mapping|void:"
		"      mapping(string:int|string|object))", 0 );
  add_function( "decode_layers", f_decode_layers,
		"function(string,mapping|void:array(object))", 0 );
  add_function( "decode_header", f_decode_header,
		"function(string,mapping|void:mapping(string:string|int))",
		0 );
#endif
}

PIKE_MODULE_EXIT
{
}

/*! @endmodule
 */

/*! @endmodule
 */
