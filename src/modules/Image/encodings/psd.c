/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: psd.c,v 1.45 2005/12/12 20:26:20 nilsson Exp $
*/

#include "global.h"
#include "image_machine.h"

#include "pike_macros.h"
#include "object.h"
#include "module_support.h"
#include "interpret.h"
#include "object.h"
#include "svalue.h"
#include "threads.h"
#include "interpret.h"
#include "svalue.h"
#include "mapping.h"
#include "pike_error.h"
#include "stralloc.h"
#include "builtin_functions.h"

#include "image.h"
#include "colortable.h"
#include "bignum.h"

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif


#define sp Pike_sp

extern struct program *image_colortable_program;
extern struct program *image_program;

#define STRING(X) static struct pike_string *PIKE_CONCAT(s_, X)
#include "psd_constant_strings.h"
#undef STRING


struct buffer
{
  size_t len;
  unsigned char *str;
};

static unsigned int psd_read_uint( struct buffer *from )
{
  unsigned int res;
  if(from->len < 4)
    Pike_error("Not enough space for 4 bytes (uint32)\n");
  res = from->str[0]<<24|from->str[1]<<16|from->str[2]<<8|from->str[3];
  from->str += 4;
  from->len -= 4;
  return res;
}

static int psd_read_int( struct buffer *from )
{
  return (int)psd_read_uint( from );
}

static unsigned short psd_read_ushort( struct buffer *from )
{
  unsigned short res;
  if(from->len < 2)
    Pike_error("Not enough space for 2 bytes (uint16)\n");
  res = from->str[0]<<8 | from->str[1];
  from->str += 2;
  from->len -= 2;
  return res;
}

static int psd_read_short( struct buffer *from )
{
  return (short)psd_read_ushort( from );
}

static unsigned char psd_read_uchar( struct buffer *from )
{
  unsigned char res = 0;
  if(from->len)
  {
    res = from->str[0];
    from->str++;
    from->len--;
  }
  return res;
}

static int psd_read_char( struct buffer *from )
{
  return (char)psd_read_uchar( from );
}


static char *psd_read_data( struct buffer * from, size_t len )
{
  char *res;
  if( from->len < len )
    Pike_error("Not enough space for %lu bytes\n",
	  DO_NOT_WARN((unsigned long)len));
  res = (char *)from->str;
  from->str += len;
  from->len -= len;
  return res;
}

static struct buffer psd_read_string( struct buffer *data )
{
  struct buffer res;
  res.len = psd_read_int( data );
  res.str = (unsigned char *)psd_read_data( data, res.len );
  if(res.len > 0) res.len--; /* len includes ending \0 */
  if(!res.str)
    Pike_error("String read failed\n");
  return res;
}

static struct buffer psd_read_pstring( struct buffer *data )
{
  struct buffer res;
  res.len = psd_read_uchar( data );
  res.str = (unsigned char *)psd_read_data( data, res.len );
  if(!res.str)
    Pike_error("PString read failed\n");
  return res;
}

enum image_mode
{
  Bitmap = 0,
  Greyscale = 1,
  Indexed = 2,
  RGB = 3,
  CMYK = 4,
  Multichannel = 7,
  Duotone = 8,
  Lab = 9,
};


struct psd_image
{
  unsigned short num_channels;
  unsigned int rows;
  unsigned int columns;
  unsigned int compression;
  unsigned short depth;
  enum image_mode mode;
  struct buffer color_data;
  struct buffer resource_data;
  struct buffer layer_data;
  struct buffer image_data;


  struct layer *first_layer;
};

struct channel_info
{
  short id;
  struct buffer data;
};

struct layer
{
  struct layer *next;
  struct layer *prev;
  int top;
  int left;
  int right;
  int bottom;

  int mask_top;
  int mask_left;
  int mask_right;
  int mask_bottom;
  unsigned int mask_default_color;
  unsigned int mask_flags;

  unsigned int opacity;
  unsigned int num_channels;
  unsigned int clipping;
  unsigned int flags;
  struct channel_info channel_info[24];
  struct buffer mode;
  struct buffer extra_data;
  struct buffer name;
};

static void decode_layers_and_masks( struct psd_image *dst, 
                                     struct buffer *src )
{
  short count, first_alpha_is_magic;
  struct layer *layer = NULL;
  ptrdiff_t exp_offset;
  if(!src->len) 
    return;

  exp_offset = src->len-psd_read_int( src ); /* size of layer region */
  count = psd_read_short( src );
  
  if( count < 0 )
  {
    count = -count;
    first_alpha_is_magic = 1;
  } else if(count == 0)
    return;
  while( count-- )
  {
    unsigned int cnt;
    struct layer *l= 
    layer = (struct layer *)xalloc( sizeof( struct layer ));
    MEMSET(layer, 0, sizeof(struct layer));
    layer->next = dst->first_layer;
    if(dst->first_layer) dst->first_layer->prev = layer;
    dst->first_layer = layer;
    layer->top = psd_read_int( src );
    layer->left = psd_read_int( src );
    layer->bottom = psd_read_int( src );
    layer->right = psd_read_int( src );
    layer->num_channels = psd_read_short( src );
    for(cnt=0; cnt<layer->num_channels; cnt++)
    {
      layer->channel_info[cnt].id = psd_read_ushort(src);
      layer->channel_info[cnt].data.len = psd_read_uint(src);
    }
    psd_read_uint( src ); /* '8BIM' */
    layer->mode.len = 4;
    layer->mode.str = (unsigned char *)psd_read_data( src, 4 );
    layer->opacity = psd_read_uchar( src );
    layer->clipping = psd_read_uchar( src );
    layer->flags = psd_read_uchar( src );
    psd_read_uchar( src ); /* filler byte */
    layer->extra_data = psd_read_string( src );
    layer->extra_data.len++;
    if(layer->extra_data.len)
    {
      struct buffer tmp = layer->extra_data;
      struct buffer tmp2;
      tmp2 = psd_read_string( &tmp );
      if( tmp2.len )
      {
        layer->mask_top    = psd_read_int( &tmp2 );
        layer->mask_left   = psd_read_int( &tmp2 );
        layer->mask_bottom = psd_read_int( &tmp2 );
        layer->mask_right  = psd_read_int( &tmp2 );
        layer->mask_default_color = psd_read_uchar( &tmp2 );
        layer->mask_flags = psd_read_uchar( &tmp2 );
      }
      tmp2 = psd_read_string( &tmp );
      if( tmp2.len )
      {
	/* Layer blending ranges data */
      }
      layer->name = psd_read_pstring( &tmp );
    }
  }
  while(layer->next)
    layer = layer->next;
  /* Now process the layer channel data.. */
  while(layer)
  {
    unsigned int i;
    for(i=0; i<layer->num_channels; i++)
    {
      layer->channel_info[i].data.str=
        (unsigned char *)psd_read_data(src,layer->channel_info[i].data.len);
    }
    layer = layer->prev;
  }
}


static struct buffer
packbitsdecode(struct buffer src, 
               struct buffer dst, 
               int nbytes)
{
  int n;

  while( nbytes--  )
  {
    n = psd_read_uchar( &src );
    if(n >= 128)
      n -= 256;
    if (n > 0)
    {
      ++n;
      while(n--)
      {
        if(dst.len)
        {
          *(dst.str++) = psd_read_uchar( &src );
          dst.len--;
        } else
          return src;
      }
    } else if( n == -128 ){
      /* noop */
    } else {
      unsigned char val;
      n = -n+1;
      val = psd_read_uchar( &src );
      while(n--)
      {
        if(dst.len)
        {
          *(dst.str++) = val;
          dst.len--;
        } else
          return src;
      }
    }
  }
  if(dst.len)
    fprintf(stderr, "%ld bytes left to write! (should be 0)\n",
	    DO_NOT_WARN((long)dst.len));
  return src;
}

static void f_decode_packbits_encoded(INT32 args)
{
  struct pike_string *src = sp[-args].u.string;
  int nelems = sp[-args+1].u.integer;
  int width = sp[-args+2].u.integer;
  struct pike_string *dest;
  int compression = 0;
  struct buffer b, ob, d;
  if(sp[-args].type != T_STRING)
    Pike_error("Internal argument error.\n");

#ifdef HIDE_WARNINGS
  b.str = NULL;
  b.len = 0;
#endif


  if(args == 5)
  {
    nelems *= sp[-args+3].u.integer;
    compression = sp[-args+4].u.integer;
    b.str = (unsigned char *)src->str;
    b.len = src->len;
    pop_n_elems(4);
  } else if(args == 3) {
    if( src->str[0] )
      Pike_error("Impossible compression (%d)!\n", (src->str[0]<<8|src->str[1]) );
    compression = src->str[1];
    b.str = (unsigned char *)src->str+2;
    b.len = src->len-2;
    pop_n_elems(2);
  }

  ob = b;
  ob.str += nelems*2;
  ob.len -= nelems*2;

  switch(compression)
  {
   case 1:
     dest = begin_shared_string( width * nelems );
     d.str = (unsigned char *)dest->str; d.len = width*nelems;
/*      while(nelems--) */
     /*ob =*/ 
     packbitsdecode( ob, d, width*nelems );
     push_string( end_shared_string( dest ) );
     break;
   case 0:
     push_string( make_shared_binary_string((char *)b.str,b.len));
     break;
   default:
     Pike_error("Impossible compression (%d)!\n", src->str[1]);
  }
  stack_swap();
  pop_stack();
  return;
}

static void f_decode_image_channel( INT32 args )
{
  INT_TYPE w, h;
  int y;
  struct pike_string *s;
  struct object *io;
  unsigned char *source;
  rgb_group *dst;
  get_all_args( "_decode_image_channel",args, "%i%i%S", &w,&h,&s);

  ref_push_string( s );
  push_int( h );
  push_int( w );
  f_decode_packbits_encoded( 3 );
  s = sp[-1].u.string;
  stack_swap();
  pop_stack();
  if(s->len < w*h)
    Pike_error("Not enough data in string for this channel\n");
  source = (unsigned char *)s->str;
  push_int( w ); push_int( h );
  io = clone_object( image_program, 2 );
  dst = ((struct image *)get_storage(io,image_program))->img;
  for(y=0; y<w*h; y++)
  {
    dst->r = dst->g = dst->b = *(source++);
    dst++;
  }
  pop_n_elems(args);
  push_object( io );
}



static void f_decode_image_data( INT32 args )
{
  INT_TYPE w, h, c, d, m;
  int y;
  struct pike_string *s, *ct;
  struct object *io;
  unsigned char *source, *source2, *source3, *source4;
  rgb_group *dst;
  get_all_args( "_decode_image_data",args, "%i%i%i%i%i%S%S", 
                &w,&h,&d,&m,&c,&s,&ct);

  if(!ct->len) ct = NULL;

  ref_push_string( s );
  push_int( h );
  push_int( w );
  push_int( d );
  push_int( c );
  f_decode_packbits_encoded( 5 );
  s = sp[-1].u.string;
  stack_swap();
  pop_stack();
  if(s->len < w*h*d)
    Pike_error("Not enough data in string for this channel\n");
  source = (unsigned char *)s->str;
  source2 = source+w*h;
  source3 = source+w*h*2;
  source4 = source+w*h*3;
  push_int( w ); push_int( h );
  io = clone_object( image_program, 2 );
  dst = ((struct image *)get_storage(io,image_program))->img;
  for(y=0; y<w*h; y++)
  {
    switch( d )
    {
     case 4:
       /* cmyk.. */
       dst->r = MAXIMUM(255-(*(source++) + *source4),  0);
       dst->g = MAXIMUM(255-(*(source2++) + *source4), 0);
       dst->b = MAXIMUM(255-(*(source3++) + *source4), 0);
       dst++; source4++;
       break;
     case 3:
       if( m != CMYK )
       {
         dst->r = *(source++);
         dst->g = *(source2++);
         (dst++)->b = *(source3++);
       } else {
         dst->r = 255-*(source++);
         dst->g = 255-*(source2++);
         dst->b = 255-*(source3++);
       }
       break;
     case 2:
     case 1:
       if(ct)
       {
         dst->r = ct->str[*source];
         dst->g = ct->str[*source+256];
         dst->b = ct->str[*source+256*2];
         *source++;
         *dst++;
       }
       else
       {
         dst->r = dst->g = dst->b = *(source++);
	 dst++;
       }
       break;
    }
  }
  pop_n_elems(args);
  push_object( io );
}




static void free_image( struct psd_image *i )
{
  while(i->first_layer)
  {
    struct layer *t = i->first_layer;
    i->first_layer = t->next;
    free(t);
  }
}

static struct psd_image low_psd_decode( struct buffer *b )
{
  int *s = (int *)b->str;
  struct psd_image i;
  ONERROR err;
  MEMSET(&i, 0, sizeof(i));
  SET_ONERROR( err, free_image, &i );
  i.num_channels = psd_read_ushort( b );
  i.rows = psd_read_uint( b );
  i.columns = psd_read_uint( b ); 
  i.depth = psd_read_ushort( b );
  i.mode = psd_read_ushort( b );
  i.color_data = psd_read_string( b );
  i.resource_data = psd_read_string( b ); i.resource_data.len++;
  i.layer_data = psd_read_string( b );    /*i.layer_data.len++;*/
  i.compression = psd_read_short( b );
  i.image_data = *b;
  decode_layers_and_masks( &i, &i.layer_data );
  UNSET_ONERROR( err );
  return i;
}


void push_buffer( struct buffer *b )
{
  push_string( make_shared_binary_string( (char *)b->str, b->len ) );
}

void push_layer( struct layer  *l)
{
  unsigned int i;
  struct svalue *osp = sp;
  ref_push_string( s_top );           push_int( l->top );
  ref_push_string( s_left );          push_int( l->left );
  ref_push_string( s_right );         push_int( l->right );
  ref_push_string( s_bottom );        push_int( l->bottom );
  ref_push_string( s_mask_top );      push_int( l->mask_top );
  ref_push_string( s_mask_left );     push_int( l->mask_left );
  ref_push_string( s_mask_right );    push_int( l->mask_right );
  ref_push_string( s_mask_bottom );   push_int( l->mask_bottom );
  ref_push_string( s_mask_flags );    push_int( l->mask_flags );
  ref_push_string( s_mask_default_color ); push_int( l->mask_default_color );
  ref_push_string( s_opacity );       push_int( l->opacity );
  ref_push_string( s_clipping );      push_int( l->clipping );
  ref_push_string( s_flags );         push_int( l->flags );
  ref_push_string( s_mode );          push_buffer( &l->mode );
  ref_push_string( s_extra_data );    push_buffer( &l->extra_data );
  ref_push_string( s_name );          push_buffer( &l->name );
  ref_push_string( s_channels );
  for( i = 0; i<l->num_channels; i++ )
  {
    ref_push_string( s_id );
    push_int( l->channel_info[i].id );
    ref_push_string( s_data );
    push_buffer( &l->channel_info[i].data );
    f_aggregate_mapping( 4 );
  }
  f_aggregate( l->num_channels );
  f_aggregate_mapping(DO_NOT_WARN((INT32)(sp - osp)));
}


static void decode_resources( struct buffer *b );
void push_psd_image( struct psd_image *i )
{
  struct svalue *osp = sp, *tsp;
  struct layer *l;
  ref_push_string( s_channels ); push_int( i->num_channels );
  ref_push_string( s_height ); push_int( i->rows );
  ref_push_string( s_width );  push_int( i->columns );
  ref_push_string( s_compression ); push_int( i->compression );
  ref_push_string( s_depth ); push_int( i->depth );
  ref_push_string( s_mode ); push_int( i->mode );
  ref_push_string( s_color_data ); push_buffer( &i->color_data );
  ref_push_string( s_resources ); decode_resources( &i->resource_data );
  ref_push_string( s_image_data ); push_buffer( &i->image_data );
  ref_push_string( s_layers );
  l = i->first_layer;
  tsp = sp;
  while( l )
  {
    push_layer( l );
    l = l->next;
  }
  f_aggregate(DO_NOT_WARN((INT32)(sp - tsp)));
  f_aggregate_mapping(DO_NOT_WARN((INT32)(sp - osp)));
}

static void decode_resources( struct buffer *b )
{
  struct svalue *osp = Pike_sp;

  while( b->len > 11 )
  {
    char  *signature = psd_read_data( b, 4 );

    int id;
    struct buffer data;
    struct buffer name;

    /* Non-photoshop software may use another identifier and
       still be compatible with the Adobe PSD file format specification.
       FIXME? */
    if( signature[0] != '8' ||  signature[1] != 'B' ||
        signature[2] != 'I' ||  signature[3] != 'M' )
      break;

    id = psd_read_short( b );
    name = psd_read_pstring( b );
    if( !(name.len & 1) ) psd_read_uchar( b );
    data = psd_read_string( b );
    data.len++;

    if( data.len & 1 ) psd_read_uchar( b );

    switch( id )
    {
      case 0x03f0: /* caption */
	{
	  struct buffer b = psd_read_pstring( &data );
	  push_constant_text( "caption" );
	  push_buffer( &b );
	}
	break;

      case 0x0400: /* layer state info */
	push_constant_text( "active_layer" );
	push_int( psd_read_short( &data ) );
	break;
      case 0x0408: /* guides */
	push_constant_text( "guides" );
	{
	  int i,num_guides;
	  short magic1, magic2, magic3, magic4, magic5, magic6; /*from gimp.*/
	  magic1 = psd_read_short( &data ); magic2 = psd_read_short( &data );
	  magic3 = psd_read_short( &data ); magic4 = psd_read_short( &data );
	  magic5 = psd_read_short( &data ); magic6 = psd_read_short( &data );
	  num_guides = psd_read_int( &data );

	  if( data.len != (unsigned)(num_guides * 5) )
	  {
	    f_aggregate( 0 );
	    break;
	  }
	  for (i=0; i<num_guides; i++)
	  {
	    ptrdiff_t p = psd_read_int( &data );
	    int h = psd_read_uchar( &data );
	    if( h )
	      p = (ptrdiff_t)((((double)p) * (magic4>>8)) / ((double)(magic4&255)));
	    else
	      p = (ptrdiff_t)((((double)p) * (magic6>>8)) / ((double)(magic6&255)));
	    push_constant_text( "pos" );      push_int64( p );
	    push_constant_text( "vertical" ); push_int( !h );
	    f_aggregate_mapping( 4 );
	  }
	  f_aggregate( num_guides );
	}
	break;
      case 0x040b: /* URL */
	{
	  struct buffer b = psd_read_pstring( &data );
	  push_constant_text( "url" );
	  push_buffer( &b );
	}
	break;
      case 0x03ed: /* res. info. */
	push_constant_text( "resinfo" );

	push_constant_text( "hres" );       push_int(psd_read_int( &data ) );
	push_constant_text( "hres_unit" );  push_int(psd_read_short( &data ) );
	push_constant_text( "width_unit" ); push_int(psd_read_short( &data ) );

	push_constant_text( "vres" );       push_int(psd_read_int( &data ) );
	push_constant_text( "vres_unit" );  push_int(psd_read_short( &data ) );
	push_constant_text( "height_unit" );push_int(psd_read_short( &data ) );

	f_aggregate_mapping( 12 );
	break;

      case 0x03fc: /* Declared completely obsolete by Adobe */
      case 0x03ff:
      case 0x0403:
	break;

      case 0x03e8: /* Photoshop 2.0. Obsolete! */
	/* 5*int16  channels, rows, columns, depth, mode */

      case 0x03e9: /* Machintosh print manager print info record. */

      case 0x03eb: /* Photoshop 2.0. Obsolete! Indexed color table */

      case 0x03ee:
        /* Names of alpha channels as a series of pascal strings */

      case 0x03ef: /* DisplayInfo structure */

      case 0x03f1: /* Border information */
	/* Contains a fixed-number for the border width and an int16 for the
	   border units (1=inches, 2=cm, 3=points, 4=picas, 5=columns) */

      case 0x03f2: /* Background color. */

      case 0x03f3: /* Print flags */
	/* A series of one byte boolean values for
	   labels, crop marks, color bars, registration marks, negative, flip
	   interpolate and caption */

      case 0x03f4: /* Grayscale and multichannel halftoning information. */
      case 0x03f5: /* Color halftoning information. */
      case 0x03f6: /* Duotone halftoning information. */
      case 0x03f7: /* Grayscale and multichannel transfer functions. */
      case 0x03f8: /* Color transfer functions. */
      case 0x03f9: /* Duotone transfer functions. */
      case 0x03fa: /* Duotone image information. */
      case 0x03fb: /* Two bytes for the effective black and white values
		      for the dot range. */
      case 0x03fd: /* EPS options */
      case 0x03fe: /* Quick Mask information. 2 bytes containing Quick Mask
		      channel ID. One byte boolean indicating whether the
		      mask was initially empty. */
      case 0x0401: /* Working path */
      case 0x0402: /* Layers group information. 2 bytes per layer containing a
		      group ID for the dragging groups. */
      case 0x0404: /* IPTC-NAA record */
      case 0x0405: /* Image mode for raw format files. */
      case 0x0406: /* JPEG quality */
      case 0x040A: /* Copyright flag */
      case 0x0bb7: /* Name of clipping path */
      case 0x2710: /* Print flags information .
		      2 bytes version (=1), 1 byte center crop marks,
		      1 zeroed byte, 4 bytes bleed width value,
		      2 bytes bleed width scale. */
      default:
	if ((id >= 0x07d0) && (id <= 0x0bb6)) {
	  /* Path information */
	}
	push_int( id );
	push_buffer( &data );
	break;
    }
  }
  f_aggregate_mapping( DO_NOT_WARN((int)(sp-osp)) );
}

static void image_f_psd___decode( INT32 args )
{
  struct pike_string *s;
  struct buffer b;
  get_all_args( "Image.PSD.___decode", args, "%S", &s );
  if(args > 1)
    pop_n_elems( args-1 );
  if(s->len < 26+4+4+4 ) /* header+color mode+image res+layers */
    Pike_error("This is not a Photoshop PSD file (too short)\n");
  if(s->str[0] != '8' || s->str[1] != 'B'  
     || s->str[2] != 'P'  || s->str[3] != 'S' )
    Pike_error("This is not a Photoshop PSD file (invalid signature)\n");
  if(s->str[4] || s->str[5] != 1)
    Pike_error("This is not a Photoshop PSD file (invalid version)\n");
    
  b.len = s->len-12;
  b.str = (unsigned char *)s->str+12;
  {
    ONERROR onerr;
    struct psd_image i = low_psd_decode( &b );
    SET_ONERROR( onerr, free_image, &i );
    push_psd_image( &i );
    UNSET_ONERROR( onerr );
    free_image( &i );
    stack_swap();
    pop_stack();
  }
}

static void f_apply_cmap( INT32 args )
{
  struct object *io;
  struct image *i;
  rgb_group *d;
  struct pike_string *cmap;
  int n;
  get_all_args( "apply_cmap", args, "%o%S", &io, &cmap );
  if(cmap->len < 256*3)
    Pike_error("Invalid colormap resource\n");
  if(!(i = (struct image *)get_storage( io, image_program )))
    Pike_error("Invalid image object\n");
  n = i->xsize * i->ysize;
  d = i->img;
  THREADS_ALLOW();
  while(n--)
  {
    int i = d->g;
    d->r = cmap->str[i];
    d->g = cmap->str[i+256];
    d->b = cmap->str[i+512];
  }
  THREADS_DISALLOW();
  pop_n_elems(args);
  push_int(0);
}

static struct program *image_encoding_psd_program=NULL;
void init_image_psd()
{
  ADD_FUNCTION( "___decode", image_f_psd___decode, tFunc(tStr,tMapping), 0);
  ADD_FUNCTION( "___decode_image_channel", f_decode_image_channel,tFunction,0);
  ADD_FUNCTION( "___decode_image_data", f_decode_image_data,tFunction,0);
  ADD_FUNCTION( "__apply_cmap", f_apply_cmap, tFunc(tObj tStr,tVoid), 0);

  add_integer_constant("Bitmap" , Bitmap, 0 );
  add_integer_constant("Greyscale" , Greyscale, 0 );
  add_integer_constant("Indexed" , Indexed, 0 );
  add_integer_constant("RGB" , RGB, 0 );
  add_integer_constant("CMYK" , CMYK, 0 );
  add_integer_constant("Multichannel" , Multichannel, 0 );
  add_integer_constant("Duotone" , Duotone, 0 );
  add_integer_constant("Lab" , Lab, 0 );

  add_integer_constant("LAYER_FLAG_PRESERVE_TRANSPARENCY", 0x01, 0 );
  add_integer_constant("LAYER_FLAG_INVISIBLE", 0x02, 0 );
  add_integer_constant("LAYER_FLAG_BIT4", 0x04, 0 );
  add_integer_constant("LAYER_FLAG_NOPIX", 0x08, 0 );

#define STRING(X) PIKE_CONCAT(s_, X) = \
  make_shared_binary_string(#X,sizeof( #X )-sizeof(""))
#include "psd_constant_strings.h"
#undef STRING
}


void exit_image_psd()
{
#define STRING(X) free_string(PIKE_CONCAT(s_, X))
#include "psd_constant_strings.h"
#undef STRING
}
