#include "global.h"
RCSID("$Id: psd.c,v 1.12 1999/07/16 19:22:42 per Exp $");

#include "image_machine.h"

#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "module_support.h"
#include "interpret.h"
#include "object.h"
#include "svalue.h"
#include "threads.h"
#include "array.h"
#include "interpret.h"
#include "svalue.h"
#include "mapping.h"
#include "error.h"
#include "stralloc.h"
#include "builtin_functions.h"
#include "operators.h"
#include "dynamic_buffer.h"
#include "pike_macros.h"

#include "image.h"
#include "colortable.h"

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

extern struct program *image_colortable_program;
extern struct program *image_program;

/*
**! module Image
**! submodule PSD
**!
*/

#define STRING(X) static struct pike_string *s_##X;
#include "psd_constant_strings.h"
#undef STRING


struct buffer
{
  unsigned int len;
  unsigned char *str;
};

static unsigned int read_uint( struct buffer *from )
{
  unsigned int res;
  if(from->len < 4)
    error("Not enough space for 4 bytes (uint32)\n");
  res = from->str[0]<<24|from->str[1]<<16|from->str[2]<<8|from->str[3];
  from->str += 4;
  from->len -= 4;
  return res;
}

static int read_int( struct buffer *from )
{
  return (int)read_uint( from );
}

static unsigned short read_ushort( struct buffer *from )
{
  unsigned short res;
  if(from->len < 2)
    error("Not enough space for 2 bytes (uint16)\n");
  res = from->str[0]<<8 | from->str[1];
  from->str += 2;
  from->len -= 2;
  return res;
}

static int read_short( struct buffer *from )
{
  return (short)read_ushort( from );
}

static unsigned char read_uchar( struct buffer *from )
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

static int read_char( struct buffer *from )
{
  return (char)read_uchar( from );
}


static char *read_data( struct buffer * from, unsigned int len )
{
  char *res;
  if( from->len < len )
    error("Not enough space for %u bytes\n", len);
  res = from->str;
  from->str += len;
  from->len -= len;
  return res;
}

static struct buffer read_string( struct buffer *data )
{
  struct buffer res;
  res.len = read_int( data );
  res.str = read_data( data, res.len );
  if(res.len > 0) res.len--; /* len includes ending \0 */
  if(!res.str)
    error("String read failed\n");
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
  unsigned int top;
  unsigned int left;
  unsigned int right;
  unsigned int bottom;

  unsigned int mask_top;
  unsigned int mask_left;
  unsigned int mask_right;
  unsigned int mask_bottom;
  unsigned int mask_default_color;
  unsigned int mask_flags;

  unsigned int opacity;
  unsigned int num_channels;
  unsigned int clipping;
  unsigned int flags;
  int compression;
  struct channel_info channel_info[24];
  struct buffer mode;
  struct buffer extra_data;
};

static void decode_layers_and_masks( struct psd_image *dst, 
                                     struct buffer *src )
{
  short count, first_alpha_is_magic;
  unsigned int normal;
  unsigned int *q;
  struct layer *layer = NULL;
  int i;
  int exp_offset;
  if(!src->len) 
    return;

  exp_offset = src->len-read_int( src ); /* size of layer region */
  count = read_short( src );
  
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
    layer->top = read_int( src );
    layer->left = read_int( src );
    layer->bottom = read_int( src );
    layer->right = read_int( src );
    layer->num_channels = read_short( src );
    for(cnt=0; cnt<layer->num_channels; cnt++)
    {
      layer->channel_info[cnt].id = read_ushort(src);
      layer->channel_info[cnt].data.len = read_uint(src);
    }
    read_uint( src ); /* '8BIM' */
    layer->mode.len = 4;
    layer->mode.str = read_data( src, 4 );
    layer->opacity = read_uchar( src );
    layer->clipping = read_uchar( src );
    layer->flags = read_uchar( src );
    read_uchar( src );
    layer->extra_data = read_string( src );
    if(layer->extra_data.len)
    {
      struct buffer tmp = layer->extra_data;
      struct buffer tmp2;
      tmp2 = read_string( &tmp );
      if( tmp2.len )
      {
        layer->mask_top    = read_int( &tmp2 );
        layer->mask_left   = read_int( &tmp2 );
        layer->mask_bottom = read_int( &tmp2 );
        layer->mask_right  = read_int( &tmp2 );
        layer->mask_default_color = read_uchar( &tmp2 );
        layer->mask_flags = read_uchar( &tmp2 );
      }
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
        read_data(src,layer->channel_info[i].data.len);
    }
    layer = layer->prev;
  }
}


static struct buffer
packbitsdecode(struct buffer src, 
               struct buffer dst, 
               int nbytes)
{
  int n, b;

  while( nbytes--  )
  {
    n = read_uchar( &src );
    if(n >= 128)
      n -= 256;
    if (n > 0)
    {
      ++n;
      while(n--)
      {
        if(dst.len)
        {
          *(dst.str++) = read_uchar( &src );
          dst.len--;
        } else
          return src;
      }
    } else if( n == -128 ){
      /* noop */
    } else {
      unsigned char val;
      n = -n+1;
      val = read_uchar( &src );
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
    fprintf(stderr, "%d bytes left to write! (should be 0)\n", dst.len);
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
    error("internal argument error");


  if(args == 5)
  {
    nelems *= sp[-args+3].u.integer;
    compression = sp[-args+4].u.integer;
    b.str = src->str;
    b.len = src->len;
    pop_n_elems(4);
  } else if(args == 3) {
    if( src->str[0] )
      error("Impossible compression (%d)!\n", (src->str[0]<<8|src->str[1]) );
    compression = src->str[1];
    b.str = src->str+2;
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
     d.str = dest->str; d.len = width*nelems;
/*      while(nelems--) */
     /*ob =*/ 
     packbitsdecode( ob, d, width*nelems );
     push_string( end_shared_string( dest ) );
     break;
   case 0:
     push_string( make_shared_binary_string(b.str,b.len));
     break;
   default:
     error("Impossible compression (%d)!\n", src->str[1]);
  }
  stack_swap();
  pop_stack();
  return;
}

static void f_decode_image_channel( INT32 args )
{
  INT32 w, h, d;
  int y;
  struct pike_string *s;
  struct object *io;
  unsigned char *source;
  rgb_group *dst;
  get_all_args( "_decode_image_channel",args, "%d%d%S", &w,&h,&s);

  ref_push_string( s );
  push_int( h );
  push_int( w );
  f_decode_packbits_encoded( 3 );
  s = sp[-1].u.string;
  stack_swap();
  pop_stack();
  if(s->len < w*h)
    error("Not enough data in string for this channel\n");
  source = s->str;
  push_int( w ); push_int( h );
  io = clone_object( image_program, 2 );
  dst = ((struct image *)get_storage(io,image_program))->img;
  for(y=0; y<w*h; y++)
    dst->r = dst->g = (dst++)->b = *(source++);
  pop_n_elems(args);
  push_object( io );
}



static void f_decode_image_data( INT32 args )
{
  INT32 w, h, c, d, m;
  int y;
  struct pike_string *s, *ct;
  struct object *io;
  unsigned char *source, *source2, *source3, *source4;
  rgb_group *dst;
  get_all_args( "_decode_image_data",args, "%d%d%d%d%d%S%S", 
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
    error("Not enough data in string for this channel\n");
  source = s->str;
  source2 = s->str+w*h;
  source3 = s->str+w*h*2;
  source4 = s->str+w*h*3;
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
         dst->r = dst->g = (dst++)->b = *(source++);
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
  i.num_channels = read_ushort( b );
  i.rows = read_uint( b );
  i.columns = read_uint( b ); 
  i.depth = read_ushort( b );
  i.mode = read_ushort( b );
  i.color_data = read_string( b );
  i.resource_data = read_string( b );
  i.layer_data = read_string( b );
  i.compression = read_short( b );
  i.image_data = *b;
  decode_layers_and_masks( &i, &i.layer_data );
  UNSET_ONERROR( err );
  return i;
}


void push_buffer( struct buffer *b )
{
  push_string( make_shared_binary_string( b->str, b->len ) );
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
  ref_push_string( s_opacity );       push_int( l->opacity );
  ref_push_string( s_clipping );      push_int( l->clipping );
  ref_push_string( s_flags );         push_int( l->flags );
  ref_push_string( s_compression );   push_int( l->compression );
  ref_push_string( s_mode );          push_buffer( &l->mode );
  ref_push_string( s_extra_data );    push_buffer( &l->extra_data );
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
  f_aggregate_mapping( sp-osp);
}


void push_psd_image( struct psd_image *i )
{
  struct svalue *osp = sp, *tsp;
  struct layer *l;
  ref_push_string( s_channels ); push_int( i->num_channels );
  ref_push_string( s_height ); push_int( i->rows );
  ref_push_string( s_width );  push_int( i->columns );
  ref_push_string( s_compression ); push_int( i->compression );
  ref_push_string( s_depth ); push_int( i->compression );
  ref_push_string( s_mode ); push_int( i->mode );
  ref_push_string( s_color_data ); push_buffer( &i->color_data );
  ref_push_string( s_resource_data ); push_buffer( &i->resource_data );
  ref_push_string( s_image_data ); push_buffer( &i->image_data );
  ref_push_string( s_layers );
  l = i->first_layer;
  tsp = sp;
  while( l )
  {
    push_layer( l );
    l = l->next;
  }
  f_aggregate( sp-tsp );
  f_aggregate_mapping( sp - osp );
}

static void image_f_psd___decode( INT32 args )
{
  struct pike_string *s;
  struct buffer b;
  get_all_args( "Image.PSD.___decode", args, "%S", &s );
  if(args > 1)
    pop_n_elems( args-1 );
  if(s->str[0] != '8' || s->str[1] != 'B'  
     || s->str[2] != 'P'  || s->str[3] != 'S' )
    error("This is not a Photoshop PSD file (invalid signature)\n");
  if(s->str[4] || s->str[5] != 1)
    error("This is not a Photoshop PSD file (invalid version)\n");
    
  b.len = s->len-12;
  b.str = s->str+12;
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
    error("Invalid colormap resource\n");
  if(!(i = (struct image *)get_storage( io, image_program )))
    error("Invalid image object\n");
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
  add_function( "___decode", image_f_psd___decode, 
                "function(string:mapping)", 0);
  add_function( "___decode_image_channel", f_decode_image_channel, 
                "mixed", 0);
  add_function( "___decode_image_data", f_decode_image_data, 
                "mixed", 0);
  add_function( "__apply_cmap", f_apply_cmap, "mixed", 0);

  add_integer_constant("Bitmap" , Bitmap, 0 );
  add_integer_constant("Greyscale" , Greyscale, 0 );
  add_integer_constant("Indexed" , Indexed, 0 );
  add_integer_constant("RGB" , RGB, 0 );
  add_integer_constant("CMYK" , CMYK, 0 );
  add_integer_constant("Multichannel" , Multichannel, 0 );
  add_integer_constant("Duotone" , Duotone, 0 );
  add_integer_constant("Lab" , Lab, 0 );



  add_integer_constant("LAYER_FLAG_VISIBLE", 0x01, 0 );
  add_integer_constant("LAYER_FLAG_OBSOLETE", 0x02, 0 );
  add_integer_constant("LAYER_FLAG_BIT4", 0x04, 0 );
  add_integer_constant("LAYER_FLAG_NOPIX", 0x08, 0 );

#define STRING(X) s_##X = make_shared_binary_string(#X,sizeof( #X )-sizeof(""));
#include "psd_constant_strings.h"
#undef STRING
}


void exit_image_psd()
{
#define STRING(X) free_string(s_##X)
#include "psd_constant_strings.h"
#undef STRING
}

