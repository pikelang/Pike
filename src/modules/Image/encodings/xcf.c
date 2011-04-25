/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#include "global.h"
RCSID("$Id$");

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
#include "pike_error.h"
#include "stralloc.h"
#include "builtin_functions.h"
#include "operators.h"
#include "dynamic_buffer.h"
#include "signal_handler.h"
#include "bignum.h"

#include "image.h"
#include "colortable.h"


#define sp Pike_sp
#define fp Pike_fp

extern struct program *image_colortable_program;
extern struct program *image_program;

/*
**! module Image
**! submodule XCF
**!
*/

#define TILE_WIDTH 64
#define TILE_HEIGHT 64

#define STRING(X) static struct pike_string *s_##X
#include "xcf_constant_strings.h"
#undef STRING


struct buffer
{
  struct pike_string *s;
  ptrdiff_t base_offset;
  ptrdiff_t base_len;
  size_t len;
  unsigned char *str;
};

struct substring
{
  struct pike_string *s;
  ptrdiff_t offset;
  ptrdiff_t len;
};

static struct program *substring_program;
#define SS(X) ((struct substring*)X->storage)

static void f_substring_cast( INT32 args )
{
  /* FIXME: assumes string */
  struct substring *s = SS(fp->current_object);
  pop_n_elems( args );
  push_string( make_shared_binary_string( (((char *)s->s->str)+s->offset),
                                          s->len ) );
}

static void f_substring_index( INT32 args )
{
  ptrdiff_t i = sp[-1].u.integer;
  struct substring *s = SS(fp->current_object);
  pop_n_elems( args );

  if( i < 0 ) i = s->len + i;
  if( i >= s->len ) {
    Pike_error("Index out of bounds, %d > %ld\n", i,
	  DO_NOT_WARN((long)s->len-1) );
  }
  push_int( ((unsigned char *)s->s->str)[s->offset+i] );
}

static void f_substring__sprintf( INT32 args )
{
  int x;
  struct substring *s = SS(fp->current_object);

  if (args != 2 )
    SIMPLE_TOO_FEW_ARGS_ERROR("_sprintf",2);
  if (sp[-2].type!=T_INT)
    SIMPLE_BAD_ARG_ERROR("_sprintf",0,"integer");
  if (sp[-1].type!=T_MAPPING)
    SIMPLE_BAD_ARG_ERROR("_sprintf",1,"mapping");
  x = sp[-2].u.integer;
  pop_n_elems( args );

  switch( x )
  {
   case 't':
     push_constant_text("SubString");
     return;
   case 'O':
     push_constant_text("SubString( %O /* [+%d .. %d] */ )" );
     push_text("string"); f_substring_cast( 1 );

     push_int64( s->len );
     push_int64( s->offset );
     f_sprintf( 4 );
     return;
   default: 
     push_int(0);
     return;
  }
}

static void f_substring_get_int( INT32 args )
{
  struct substring *s = SS(fp->current_object);
  int res;
  unsigned char *p;
  int x = sp[-1].u.integer;
  if( x > s->len>>2 )
    Pike_error("Index %d out of range", x );

  p = ((unsigned char *)s->s->str) + s->offset + x*4;
  res = (p[0]<<24) | (p[1]<<16) | (p[2]<<8) | p[3];
  push_int( res );
}


static void f_substring_get_uint( INT32 args )
{
  struct substring *s = SS(fp->current_object);
  unsigned int res;
  unsigned char *p;
  int x = sp[-1].u.integer;
  if( x > s->len>>2 )
    Pike_error("Index %d out of range", x );

  p = ((unsigned char *)s->s->str) + s->offset + x*4;
  res = (p[0]<<24) | (p[1]<<16) | (p[2]<<8) | p[3];
  push_int64( res );
}

static void f_substring_get_ushort( INT32 args )
{
  struct substring *s = SS(fp->current_object);
  unsigned short res;
  unsigned char *p;
  int x = sp[-1].u.integer;
  if( x > s->len>>1 )
    Pike_error("Index %d out of range", x );

  p = ((unsigned char *)s->s->str) + s->offset + x*2;
  res = (p[2]<<8) | p[3];
  push_int( res );
}

static void f_substring_get_short( INT32 args )
{
  struct substring *s = SS(fp->current_object);
  short res;
  unsigned char *p;
  int x = sp[-1].u.integer;
  if( x > s->len>>1 )
    Pike_error("Index %d out of range", x );

  p = ((unsigned char *)s->s->str) + s->offset + x*2;
  res = (p[2]<<8) | p[3];
  push_int( res );
}

static void push_substring( struct pike_string *s, 
                            ptrdiff_t offset,
                            ptrdiff_t len )
{
  struct object *o = clone_object( substring_program, 0 );
  struct substring *ss = SS(o);
  ss->s = s;
  s->refs++;
  ss->offset = offset;
  ss->len = len;
  push_object( o );
}

static void free_substring(struct object *o)
{
  if( SS(fp->current_object)->s )
  {
    free_string( SS(fp->current_object)->s );
    SS(fp->current_object)->s = 0;
  }
}


typedef enum
{
  PROP_END = 0,
  PROP_COLORMAP = 1,
  PROP_ACTIVE_LAYER = 2,
  PROP_ACTIVE_CHANNEL = 3,
  PROP_SELECTION = 4,
  PROP_FLOATING_SELECTION = 5,
  PROP_OPACITY = 6,
  PROP_MODE = 7,
  PROP_VISIBLE = 8,
  PROP_LINKED = 9,
  PROP_PRESERVE_TRANSPARENCY = 10,
  PROP_APPLY_MASK = 11,
  PROP_EDIT_MASK = 12,
  PROP_SHOW_MASK = 13,
  PROP_SHOW_MASKED = 14,
  PROP_OFFSETS = 15,
  PROP_COLOR = 16,
  PROP_COMPRESSION = 17,
  PROP_GUIDES = 18,
  PROP_RESOLUTION = 19,
  PROP_TATTOO = 20,
  PROP_PARASITES = 21,
  PROP_UNIT = 22,
  PROP_PATHS = 23,
  PROP_USER_UNIT = 24
} property_type;


typedef enum
{
  RGB = 0,
  GRAY = 1,
  INDEXED = 2,
} image_type;

typedef enum
{
  Red,
  Green,
  Blue,
  Gray,
  Indexed,
  Auxillary
} channel_type;

typedef enum
{
  RGB_GIMAGE,
  RGBA_GIMAGE,
  GRAY_GIMAGE,
  GRAYA_GIMAGE,
  INDEXED_GIMAGE,
  INDEXEDA_GIMAGE
} layer_type;


typedef enum {  /*< chop=_MODE >*/
  NORMAL_MODE,
  DISSOLVE_MODE,
  BEHIND_MODE,
  MULTIPLY_MODE,      /*< nick=MULTIPLY/BURN >*/
  SCREEN_MODE,
  OVERLAY_MODE,
  DIFFERENCE_MODE,
  ADDITION_MODE,
  SUBTRACT_MODE,
  DARKEN_ONLY_MODE,   /*< nick=DARKEN-ONLY >*/
  LIGHTEN_ONLY_MODE,  /*< nick=LIGHTEN-ONLY >*/
  HUE_MODE,
  SATURATION_MODE,
  COLOR_MODE,
  VALUE_MODE,
  DIVIDE_MODE,        /*< nick=DIVIDE/DODGE >*/
  ERASE_MODE,
  REPLACE_MODE,
} LayerModeEffects;


typedef enum
{
  COMPRESS_NONE = 0,
  COMPRESS_RLE = 1,
  COMPRESS_ZLIB = 2,
  COMPRESS_FRACTAL = 3,
} compression_type;

struct property
{
  int type;
  struct buffer data;
  struct property *next;
};

static unsigned int read_uint( struct buffer *from )
{
  unsigned int res;
  if(from->len < 4)
    Pike_error("Not enough space for 4 bytes (uint32)\n");
  res = (from->str[0]<<24)|(from->str[1]<<16)|(from->str[2]<<8)|from->str[3];
  from->str += 4;
  from->len -= 4;
  return res;
}

static int xcf_read_int( struct buffer *from )
{
  return (int)read_uint( from );
}

static char *read_data( struct buffer *from, size_t len )
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

static struct buffer read_string( struct buffer *data )
{
  struct buffer res = *data;
  res.len = xcf_read_int( data );
  res.base_offset = (data->base_offset+(data->base_len-data->len));
  res.str = (unsigned char *)read_data( data, res.len );
  if(res.len > 0)  res.len--;  /* len includes ending \0 */
  res.base_len = res.len;
  if(!res.str)
    Pike_error("String read failed\n");
  return res;
}

static struct property read_property( struct buffer * data )
{
  struct property res;
  res.type = read_uint( data );
  if(res.type == PROP_COLORMAP)
  {
    unsigned int foo;
    read_uint(data); /* bogus 'len'... */
    foo = read_uint( data );
    res.data.len = foo*3;
    res.data.base_offset = data->base_offset+(data->base_len-data->len);
    res.data.base_len = res.data.len;
    res.data.str = (unsigned char *)read_data( data,foo*3 );
    res.data.s   = data->s;
  } else {
    res.data.len = read_uint( data );
    res.data.base_offset = data->base_offset+(data->base_len-data->len);
    res.data.base_len = res.data.len;
    res.data.str = (unsigned char *)read_data( data,res.data.len );
    res.data.s   = data->s;
  }
  res.next = NULL;
  return res;
}


struct tile
{
  struct tile *next;
  struct buffer data;
};

struct level
{
  unsigned int width;
  unsigned int height;

  struct tile *first_tile;
};

struct hierarchy
{
  unsigned int width;
  unsigned int height;
  unsigned int bpp;
  struct level level;
};


struct channel
{
  struct channel *next;
  unsigned int width;
  unsigned int height;
  struct buffer name;
  struct hierarchy image_data;
  struct property *first_property;
};

struct layer_mask
{
  unsigned int width;
  unsigned int height;
  struct buffer name;
  struct hierarchy image_data;
  struct property *first_property;
};

struct layer
{
  struct layer *next;
  unsigned int width;
  unsigned int height;
  int type;
  struct buffer name;
  struct hierarchy image_data;
  struct property *first_property;
  struct layer_mask *mask;
};

struct gimp_image
{
  unsigned int width;
  unsigned int height;
  int type;
  struct property *first_property;
  struct layer *first_layer;      /* OBS: In reverse order! */
  struct channel *first_channel;      /* OBS: In reverse order! */
};



static void free_level( struct level *l )
{
  struct tile *t;
  while( (t = l->first_tile) )
  {
    l->first_tile = t->next;
    free(t);
  }
}

static void free_layer_mask( struct layer_mask *l )
{
  struct property *p;

  while( (p = l->first_property) )
  {
    l->first_property = p->next;
    free(p);
  }
  free_level( &l->image_data.level );
}


static void free_layer( struct layer *l )
{
  struct property *p;

  while( (p = l->first_property) )
  {
    l->first_property = p->next;
    free(p);
  }
  if(l->mask)
  {
    free_layer_mask( l->mask );
    free( l->mask );
  }
  free_level( &l->image_data.level );
}

static void free_channel( struct channel *l )
{
  struct property *p;

  while( (p = l->first_property) )
  {
    l->first_property = p->next;
    free(p);
  }
  free_level( &l->image_data.level );
}


static void free_image( struct gimp_image *i )
{
  struct property *p;
  struct layer *l;
  struct channel *c;
  while( (p = i->first_property) )
  {
    i->first_property = p->next;
    free(p);
  }
  while( (l = i->first_layer) )
  {
    i->first_layer = l->next;
    free_layer( l );
    free( l );
  }
  while( (c = i->first_channel) )
  {
    i->first_channel = c->next;
    free_channel(c);
    free(c);
  }
}

static struct level read_level( struct buffer *buff,
                                struct buffer *initial )
{
  struct level res;
  ONERROR err;
  int offset;
  struct tile *last_tile = NULL;
/*   int all_tiles_eq = 0; */
  MEMSET(&res, 0, sizeof(res));
  res.width = read_uint( buff );
  res.height = read_uint( buff );

  SET_ONERROR( err, free_level, &res );
  offset = read_uint( buff );
  while(offset)
  {
    struct buffer ob = *initial;
    int offset2 = read_uint( buff );
    struct tile *tile = (struct tile *)xalloc(sizeof(struct tile));
    read_data( &ob, offset );
    if(last_tile)
      last_tile->next = tile;
    last_tile = tile;
    if(!res.first_tile)
      res.first_tile = tile;
    tile->data = ob;
    tile->next = NULL;
    offset = offset2;
  }
  UNSET_ONERROR( err );
  return res;
}

static struct hierarchy read_hierarchy( struct buffer *buff,
                                        struct buffer *initial )
{
  struct hierarchy res;
  unsigned int offset;
  struct buffer ob;

  MEMSET(&res, 0, sizeof(res));
  res.width = read_uint( buff );
  res.height = read_uint( buff );
  res.bpp = read_uint( buff );
  offset = read_uint( buff );
  ob = *initial;
  read_data( &ob, offset );
  res.level = read_level( &ob, initial );
  return res;
}

static struct layer_mask read_layer_mask( struct buffer *buff,
                                          struct buffer *initial )
{
  struct layer_mask res;
  ONERROR err;
  int offset;
  struct property tmp;

  MEMSET(&res, 0, sizeof(res));
  res.width = read_uint( buff );
  res.height = read_uint( buff );
  res.name = read_string( buff );

  SET_ONERROR( err, free_layer_mask, &res );
  do
  {
    tmp = read_property( buff );
    if(tmp.type)
    {
      struct property *s = (struct property *)xalloc(sizeof(struct property));
      *s = tmp;
      s->next = res.first_property;
      res.first_property = s;
    }
  } while(tmp.type);
  offset = read_uint( buff );

  if(offset)
  {
    struct buffer ob = *initial;
    read_data( &ob, offset );
    res.image_data = read_hierarchy( &ob, initial );
  }
  UNSET_ONERROR( err );
  return res;
}

static struct channel read_channel( struct buffer *buff,
                                    struct buffer *initial )
{
  struct channel res;
  ONERROR err;
  int offset;
  struct property tmp;

  MEMSET(&res, 0, sizeof(res));
  res.width = read_uint( buff );
  res.height = read_uint( buff );
  res.name = read_string( buff );

  SET_ONERROR( err, free_channel, &res );
  do
  {
    tmp = read_property( buff );
    if(tmp.type)
    {
      struct property *s =(struct property *)xalloc( sizeof(struct property ));
      *s = tmp;
      s->next = res.first_property;
      res.first_property = s;
    }
  }  while( tmp.type );
  offset = read_uint( buff );

  if(offset)
  {
    struct buffer ob = *initial;
    read_data( &ob, offset );
    res.image_data = read_hierarchy( &ob, initial );
  }
  UNSET_ONERROR( err );
  return res;
}

static struct layer read_layer( struct buffer *buff, struct buffer *initial )
{
  struct layer res;
  struct property tmp;
  int lm_offset;
  int h_offset;
  ONERROR err;

  MEMSET(&res, 0, sizeof(res));
  SET_ONERROR( err, free_layer, &res );
  res.width = read_uint( buff );
  res.height = read_uint( buff );
  res.type = xcf_read_int( buff );
  res.name = read_string( buff );


  do
  {
    tmp = read_property( buff );
    if(tmp.type)
    {
      struct property *s=(struct property *)xalloc( sizeof(struct property ));
      *s = tmp;
      s->next = res.first_property;
      res.first_property = s;
    }
  } while( tmp.type );

  h_offset = xcf_read_int( buff );
  lm_offset = xcf_read_int( buff );

  if(lm_offset)
  {
    struct buffer loffset = *initial;
    struct layer_mask *m=(struct layer_mask *)xalloc(sizeof(struct layer_mask));
    res.mask = m;
    read_data( &loffset, lm_offset );
    MEMSET(m, 0, sizeof( struct layer_mask ));
    *m = read_layer_mask( &loffset, initial );
  }

  if(h_offset)
  {
    struct buffer loffset = *initial;
    read_data( &loffset, h_offset );
    res.image_data = read_hierarchy( &loffset, initial );
  }

  UNSET_ONERROR( err );
  return res;
}


static struct gimp_image read_image( struct buffer * data )
{
  struct gimp_image res;
  struct property tmp;
  struct buffer initial;
  unsigned int offset;
  ONERROR err;

  MEMSET(&res, 0, sizeof(res));
  initial = *data;
  if(data->len < 34)
    Pike_error("This is not an xcf file (to little data)\n");
  if(!(data->str[0] == 'g' &&
       data->str[1] == 'i' &&
       data->str[2] == 'm' &&
       data->str[3] == 'p' &&
       data->str[4] == ' '))
  {
    if(strlen((char *)data->str) == 13)
      Pike_error("This is not an xcf file (%s)\n", data->str);
    else
      Pike_error("This is not an xcf file\n");
  }
  data->str+=14; data->len-=14;

  res.width = read_uint( data );
  res.height = read_uint( data );
  res.type = xcf_read_int( data );

  SET_ONERROR( err, free_image, &res );

  do
  {
    tmp = read_property( data );
    if(tmp.type)
    {
      struct property *s= (struct property *)xalloc( sizeof(struct property ));
      *s = tmp;
      s->next = res.first_property;
      res.first_property = s;
    }
  } while( tmp.type );

  while( (offset = read_uint( data )) )
  {
    struct buffer layer_data = initial;
    struct layer tmp;
    read_data( &layer_data, offset );
    tmp = read_layer( &layer_data, &initial );
    if( tmp.width && tmp.height )
    {
      struct layer *s = (struct layer *)xalloc( sizeof(struct layer));
      *s = tmp;
      s->next = res.first_layer;
      res.first_layer = s;
    }
  }

  while( (offset = read_uint( data )) )
  {
    struct buffer channel_data = initial;
    struct channel tmp;
    read_data( &channel_data, offset );
    tmp = read_channel( &channel_data, &initial );
    if( tmp.width && tmp.height )
    {
      struct channel *s = (struct channel *)xalloc( sizeof(struct channel));
      *s = tmp;
      s->next = res.first_channel;
      res.first_channel = s;
    }
  }
  UNSET_ONERROR( err );
  return res;
}

static void push_buffer( struct buffer *b )
{
  push_substring( b->s, b->base_offset+(b->base_len-b->len), b->len );
/* push_string( make_shared_binary_string( (char *)b->str, b->len ) );*/
}

static void push_properties( struct property *p )
{
  struct svalue *osp = sp;
  while(p)
  {
    ref_push_string( s_type );    push_int( p->type );
    ref_push_string( s_data );    push_buffer( &p->data );
    f_aggregate_mapping( 4 );
    p = p->next;
  }
  f_aggregate(DO_NOT_WARN((INT32)(sp - osp)));
}

static void push_tile( struct tile *t )
{
  push_buffer( &t->data );
}

static void push_hierarchy( struct hierarchy * h )
{
  struct tile *t = h->level.first_tile;
  struct svalue *osp = sp, *tsp;
  if(h->level.width != h->width ||
     h->level.height != h->height)
    Pike_error("Illegal hierarchy level sizes!\n");

  ref_push_string( s_width );  push_int( h->width );
  ref_push_string( s_height ); push_int( h->height );
  ref_push_string( s_bpp ); push_int( h->bpp );

  ref_push_string( s_tiles );
  tsp = sp;
  while(t)
  {
    push_tile( t );
    t=t->next;
  }
  f_aggregate(DO_NOT_WARN((INT32)(sp - tsp)));
  f_aggregate_mapping(DO_NOT_WARN((INT32)(sp - osp)));
}

static void push_layer_mask(struct layer_mask *i)
{
  struct svalue *osp = sp;
  ref_push_string( s_width );  push_int( i->width );
  ref_push_string( s_height ); push_int( i->height );
  ref_push_string( s_properties );
  push_properties( i->first_property );
  ref_push_string( s_name );  push_buffer( &i->name );
  ref_push_string( s_image_data );
  push_hierarchy( &i->image_data );
  f_aggregate_mapping(DO_NOT_WARN((INT32)(sp-osp)));
}

static void push_channel(struct channel *i)
{
  struct svalue *osp = sp;
  ref_push_string( s_width );  push_int( i->width );
  ref_push_string( s_height ); push_int( i->height );
  ref_push_string( s_properties );
  push_properties( i->first_property );
  ref_push_string( s_name );  push_buffer( &i->name );
  ref_push_string( s_image_data );
  push_hierarchy( &i->image_data );
  f_aggregate_mapping(DO_NOT_WARN((INT32)(sp-osp)));
}

static void push_layer(struct layer *i)
{
  struct svalue *osp = sp;
  ref_push_string( s_width );  push_int( i->width );
  ref_push_string( s_height ); push_int( i->height );
  ref_push_string( s_type );   push_int( i->type );
  ref_push_string( s_properties );
  push_properties( i->first_property );
  ref_push_string( s_name );  push_buffer( &i->name );
  ref_push_string( s_image_data );
  push_hierarchy( &i->image_data );
  if( i->mask )
  {
    ref_push_string( s_mask );
    push_layer_mask( i->mask );
  }
  f_aggregate_mapping(DO_NOT_WARN((INT32)(sp - osp)));
}


static void push_image( struct gimp_image *i )
{
  struct layer *l;
  struct channel *c;
  int nitems = 0;
  struct svalue *osp = sp;
  ref_push_string( s_width );  push_int( i->width );
  ref_push_string( s_height ); push_int( i->height );
  ref_push_string( s_type );   push_int( i->type );
  ref_push_string( s_properties ); push_properties( i->first_property );

  ref_push_string( s_layers );
  l = i->first_layer;
  while(l)
  {
    nitems++;
    push_layer( l );
    l = l->next;
  }
  f_aggregate( nitems );

  ref_push_string( s_channels );
  nitems=0;
  c = i->first_channel;
  while(c)
  {
    nitems++;
    push_channel( c );
    c = c->next;
  }
  f_aggregate( nitems );
  f_aggregate_mapping(DO_NOT_WARN((INT32)(sp-osp)));
}


static void image_xcf____decode( INT32 args )
{
  struct pike_string *s;
  struct buffer b;
  struct gimp_image res;
  ONERROR err;
  get_all_args( "___decode", args, "%S", &s );
  if(args > 1)
    Pike_error("Too many arguments to Image.XCF.___decode()\n");

  b.s = s;
  b.base_offset = 0;
  b.base_len = s->len;
  b.len = s->len;
  b.str = (unsigned char *)s->str;

  res = read_image( &b );
  SET_ONERROR( err, free_image, &res );
  push_image( &res );
  UNSET_ONERROR( err );
  free_image( &res );
  stack_swap();
  pop_stack();
}


static unsigned char read_char( struct buffer *from )
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

/*
**! method array(object) decode_layers( string data )
**! method array(object) decode_layers( string data, mapping options )
**!     Decodes a XCF image to an array of Image.Layer objects
**!
**!     The layer object have the following extra variables (to be queried
**!     using get_misc_value):
**!
**!      image_xres, image_yres, image_colormap, image_guides, image_parasites,
**!      name, parasites, visible, active
**!
**!	Takes the same argument mapping as <ref>_decode</ref>,
**!	note especially "draw_all_layers":1.
*/

/*
**! method object decode(string data)
**! 	Decodes a XCF image to a single image object.
**!
**! note
**!	Throws upon error in data, you will loose quite a lot of
**!     information by doing this. See Image.XCF._decode and Image.XCF.__decode
*/


/*
**! method mapping _decode(string|object data,mapping|void options)
**! 	Decodes a XCF image to a mapping, with at least an
**!   'image' and possibly an 'alpha' object. Data is either a XCF image, or
**!    a XCF.GimpImage object structure (as received from __decode)
**!
**!   <pre> Supported options
**!    ([
**!       "background":({r,g,b})||Image.Color object
**!       "draw_all_layers":1,
**!            Draw invisible layers as well
**!
**!       "draw_guides":1,
**!            Draw the guides
**!
**!       "draw_selection":1,
**!             Mark the selection using an overlay
**!
**!       "ignore_unknown_layer_modes":1
**!             Do not asume 'Normal' for unknown layer modes.
**!
**!       "mark_layers":1,
**!            Draw an outline around all (drawn) layers
**!
**!       "mark_layer_names":Image.Font object,
**!            Write the name of all layers using the font object,
**!
**!       "mark_active_layer":1,
**!            Draw an outline around the active layer
**!     ])</pre>
**!
**! note
**!	Throws upon error in data. For more information, see Image.XCF.__decode
*/

/*
**! method object __decode(string|mapping data, mapping|void options)
**! 	Decodes a XCF image to a Image.XCF.GimpImage object.
**!
**!     <pre>Returned structure reference
**!
**!    !class GimpImage
**!    {
**!      int width;
**!      int height;
**!      int compression;
**!      int type;
**!      int tattoo_state;
**!      float xres = 72.0;
**!      float yres = 72.0;
**!      int res_unit;
**!      Image.Colortable colormap;
**!      Image.Colortable meta_colormap;
**!      array(Layer) layers = ({});
**!      array(Channel) channels = ({});
**!      array(Guide) guides = ({});
**!      array(Parasite) parasites = ({});
**!      array(Path) paths = ({});
**!
**!      Layer active_layer;
**!      Channel active_channel;
**!      Channel selection;
**!    }
**!
**!    !class Layer
**!    {
**!      string name;
**!      int opacity;
**!      int type;
**!      int mode;
**!      int tattoo;
**!      mapping flags = ([]);
**!      int width, height;
**!      int xoffset, yoffset;
**!      array (Parasite) parasites;
**!      LayerMask mask;
**!      Hierarchy image;
**!    }
**!
**!    !class Channel
**!    {
**!      string name;
**!      int width;
**!      int height;
**!      int opacity;
**!      int r, g, b;
**!      int tattoo;
**!      Hierarchy image_data;
**!      object parent;
**!      mapping flags = ([]);
**!      array (Parasite) parasites;
**!    }
**!
**!    !class Hierarchy
**!    {
**!      Image.Image img;
**!      Image.Image alpha;
**!      int width;
**!      int height;
**!      int bpp;
**!    }
**!
**!    !class Parasite
**!    {
**!      string name;
**!      int flags;
**!      string data;
**!    }
**!
**!
**!    !class Guide
**!    {
**!      int pos;
**!      int vertical;
**!    }
**!
**!    !class Path
**!    {
**!      string name;
**!      int ptype;
**!      int tattoo;
**!      int closed;
**!      int state;
**!      int locked;
**!      array (PathPoint) points = ({});
**!    }
**!
**!    !class PathPoint
**!    {
**!      int type;
**!      float x;
**!      float y;
**!    }
**!
**! </pre>
*/

/*
**! method object ___decode(string|mapping data)
**! 	Decodes a XCF image to a mapping.
**!
**!     <pre>Structure reference
**!
**!    ([
**!      "width":int,
**!      "height":int,
**!      "type":int,
**!      "properties":({
**!        ([
**!          "type":int,
**!          "data":string,
**!        ]),
**!        ...
**!      }),
**!      "layers":({
**!        ([
**!          "name":string,
**!          "width":int,
**!          "height":int,
**!          "type":type,
**!          "properties":({
**!            ([
**!              "type":int,
**!              "data":string,
**!            ]),
**!            ...
**!          }),
**!          "mask":0 || ([
**!            "name":string,
**!            "width":int,
**!            "height":int,
**!            "properties":({
**!              ([
**!                "type":int,
**!                "data":string,
**!              ]),
**!              ...
**!            }),
**!            "image_data":([
**!              "bpp":int,
**!              "width":int,
**!              "height":int,
**!              "tiles":({
**!                string,
**!                ...
**!              }),
**!            ]),
**!          ]),
**!          "image_data":([
**!            "bpp":int,
**!            "width":int,
**!            "height":int,
**!            "tiles":({
**!              string,
**!              ...
**!            }),
**!          ]),
**!        ]),
**!        ...
**!      }),
**!      "channels":({
**!        "name":string,
**!        "width":int,
**!        "height":int,
**!        "properties":({
**!          ([
**!            "type":int,
**!            "data":string,
**!          ]),
**!          ...
**!        }),
**!        "image_data":([
**!          "bpp":int,
**!          "width":int,
**!          "height":int,
**!          "tiles":({
**!            string,
**!            ...
**!          }),
**!        ]),
**!      }),
**!    ])
**!</pre>
*/

void image_xcf_f__decode_tiles( INT32 args )
{
  struct object *io,*ao, *cmapo;
  struct array *tiles;
  struct image *i=NULL, *a=NULL;
  struct neo_colortable *cmap = NULL;
  int rxs, rys;
  rgb_group *colortable=NULL;
  rgb_group pix = {0,0,0};
  rgb_group apix= {255,255,255}; /* avoid may use uninitialized warnings */

  INT_TYPE rle, bpp, span, shrink;
  unsigned int l, x=0, y=0, cx, cy;
  get_all_args( "_decode_tiles", args, "%o%O%a%i%i%O%d%d%d",
                &io, &ao, &tiles, &rle, &bpp, &cmapo, &shrink, &rxs, &rys);


  if( !(i = (struct image *)get_storage( io, image_program )))
    Pike_error("Wrong type object argument 1 (image)\n");

  if(ao && !(a = (struct image *)get_storage( ao, image_program )))
    Pike_error("Wrong type object argument 2 (image)\n");

  if( cmapo &&
      !(cmap=(struct neo_colortable *)get_storage(cmapo,
                                                  image_colortable_program)))
    Pike_error("Wrong type object argument 4 (colortable)\n");

  for(l=0; l<(unsigned int)tiles->size; l++)
    if(tiles->item[l].type != T_OBJECT)
      Pike_error("Wrong type array argument 3 (tiles)\n");

  if(a && ((i->xsize != a->xsize) || (i->ysize != a->ysize)))
    Pike_error("Image and alpha objects are not identically sized.\n");

  if(cmap)
  {
    colortable = malloc(sizeof(rgb_group)*image_colortable_size( cmap ));
    image_colortable_write_rgb( cmap, (unsigned char *)colortable );
  }

  x=y=0;

  THREADS_ALLOW();
  for(l=0; l<(unsigned)tiles->size; l++)
  {
    struct object *to = tiles->item[l].u.object;
    struct substring *tile_ss = SS(to);
    struct buffer tile;
    char *df = 0;
    unsigned int eheight, ewidth;
    unsigned char *s;

    if(!tile_ss)
      continue;

    tile.str = (unsigned char *)(tile_ss->s->str + tile_ss->offset);
    tile.len = tile_ss->len;

    ewidth = MINIMUM(TILE_WIDTH, (rxs-x));
    eheight = MINIMUM(TILE_HEIGHT, (rys-y));

    if(rle)
    {
      struct buffer s = tile, od, d;
      int i;
      od.len = eheight*ewidth*bpp;  /* Max and only size, really */
      df = (char *)(od.str = (unsigned char *)xalloc( eheight*ewidth*bpp+1 ));
      d = od;

      for(i=0; i<bpp; i++)
      {
        int nelems = ewidth*eheight;
        int length;
        while(nelems)
        {
          unsigned char val = read_char( &s );
          if(!s.len)
          {
            break; /* Hm. This is actually rather fatal */
          }
          length = val;
          if( length >= 128 )
          {
            length = 255-(length-1);
            if (length == 128)
              length = (read_char( &s )<<8) + read_char( &s );
            nelems -= length;
            while(length--)
            {
              if(d.len < 1)
                break;
              d.str[0] = read_char( &s );
              d.str++;
              d.len--;
            }
          } else {
            length++;
            if(length == 128)
              length = (read_char( &s )<<8) + read_char( &s );
            nelems -= length;
            val = read_char( &s );
            while(length--)
            {
              if(d.len < 1)
                break;
              d.str[0] = val;
              d.str++;
              d.len--;
            }
          }
        }
      }
      tile = od;
    }

    if( (size_t)(tile.len) < (size_t)(eheight * ewidth * bpp ))
    {
      if( df ) free( df ); df=0;
      continue;
    }
    s = (unsigned char *)tile.str;


#define LOOP_INIT() {\
    int ix, iy=y/shrink;                                        \
    for(cy=0; cy<eheight; cy+=shrink,iy=((cy+y)/shrink))        \
    {                                                           \
      rgb_group *ip, *ap = NULL;                                \
      int ind=cy*ewidth;                                        \
      int ds = 0;                                               \
      if(iy >= i->ysize)  continue;                             \
      ix = x/shrink;                                            \
      ip = i->img + (i->xsize*iy);                              \
      if( a ) ap = a->img + (i->xsize*iy);                      \
      for(cx=0; cx<ewidth; cx+=shrink,ind+=shrink,ix++)         \
      {                                                         \
        if(ix >= i->xsize)  continue

#define LOOP_EXIT()                                             \
        ip[ix] = pix;                                           \
      }                                                         \
    }}

    if(rle)
      span = ewidth*eheight;
    else
      span = 1;

    switch(bpp)
    {
     case 1: /* indexed or grey */
       if(colortable)
       {
         LOOP_INIT();
         pix = colortable[s[ind]];
         LOOP_EXIT();
       } 
       else
       {
         LOOP_INIT();
         pix.r = pix.g = pix.b = s[ind];
         LOOP_EXIT();
       }
       break;
     case 2: /* indexed or grey with alpha */
       if(colortable)
       {
         LOOP_INIT();
         pix = colortable[ s[ ind ] ];
	 if( a )
	 {
	   apix.r = apix.g = apix.b = s[ind+span];
	   ap[ix] = apix;
	 }
         LOOP_EXIT();
       }
       else
       {
         LOOP_INIT();
         pix.r  =  pix.g =  pix.b = s[ind];
	 if( a )
	 {
	   apix.r = apix.g = apix.b = s[ind+span];
	   ap[ix] = apix;
	 }
         LOOP_EXIT();
       }
       break;
     case 3: /* rgb */
       LOOP_INIT();
       pix.r = s[ind];
       pix.g = s[ind+span];
       pix.b = s[ind+span*2];
       LOOP_EXIT();
       break;
     case 4: /* rgba */
       LOOP_INIT();
       pix.r = s[ind];
       pix.g = s[ind+span];
       pix.b = s[ind+span*2];
       if(a)
       {
	 apix.r = apix.b = apix.g = s[ind+span*3];
	 ap[ix] = apix;
       }
       LOOP_EXIT();
       break;
    }

    if( df )
    { 
      free(df); 
      df=0; 
    }
    x += TILE_WIDTH;

    if( (int)x >= (int)rxs )
    {
      x = 0;
      y += TILE_HEIGHT;
    }
  }
  THREADS_DISALLOW();
  if(colortable) 
    free( colortable );

  pop_n_elems(args);
  push_int(0);
}


static struct program *image_encoding_xcf_program=NULL;
void init_image_xcf()
{
  add_function( "___decode", image_xcf____decode,
                "function(string:mapping)", 0);

  add_function( "_decode_tiles", image_xcf_f__decode_tiles, "mixed", 0);

  add_integer_constant( "PROP_END", PROP_END,0 );
  add_integer_constant( "PROP_COLORMAP", PROP_COLORMAP, 0 );
  add_integer_constant( "PROP_ACTIVE_LAYER", PROP_ACTIVE_LAYER, 0 );
  add_integer_constant( "PROP_ACTIVE_CHANNEL", PROP_ACTIVE_CHANNEL, 0 );
  add_integer_constant( "PROP_SELECTION", PROP_SELECTION, 0 );
  add_integer_constant( "PROP_FLOATING_SELECTION", PROP_FLOATING_SELECTION, 0 );
  add_integer_constant( "PROP_OPACITY", PROP_OPACITY, 0 );
  add_integer_constant( "PROP_MODE", PROP_MODE, 0 );
  add_integer_constant( "PROP_VISIBLE", PROP_VISIBLE, 0 );
  add_integer_constant( "PROP_LINKED", PROP_LINKED, 0 );
  add_integer_constant( "PROP_PRESERVE_TRANSPARENCY",
                        PROP_PRESERVE_TRANSPARENCY, 0);
  add_integer_constant( "PROP_APPLY_MASK", PROP_APPLY_MASK, 0 );
  add_integer_constant( "PROP_EDIT_MASK", PROP_EDIT_MASK, 0 );
  add_integer_constant( "PROP_SHOW_MASK", PROP_SHOW_MASK, 0 );
  add_integer_constant( "PROP_SHOW_MASKED", PROP_SHOW_MASKED, 0 );
  add_integer_constant( "PROP_OFFSETS", PROP_OFFSETS, 0 );
  add_integer_constant( "PROP_COLOR", PROP_COLOR, 0 );
  add_integer_constant( "PROP_COMPRESSION", PROP_COMPRESSION, 0 );
  add_integer_constant( "PROP_GUIDES", PROP_GUIDES, 0 );
  add_integer_constant( "PROP_RESOLUTION", PROP_RESOLUTION, 0 );
  add_integer_constant( "PROP_TATTOO", PROP_TATTOO, 0 );
  add_integer_constant( "PROP_PARASITES", PROP_PARASITES, 0 );
  add_integer_constant( "PROP_UNIT", PROP_UNIT, 0 );
  add_integer_constant( "PROP_PATHS", PROP_PATHS, 0 );
  add_integer_constant( "PROP_USER_UNIT", PROP_USER_UNIT, 0 );

  add_integer_constant( "RGB", RGB, 0 );
  add_integer_constant( "GRAY", GRAY, 0 );
  add_integer_constant( "INDEXED", INDEXED, 0 );

  add_integer_constant( "COMPRESS_NONE", COMPRESS_NONE, 0 );
  add_integer_constant( "COMPRESS_RLE", COMPRESS_RLE, 0 );
  add_integer_constant( "COMPRESS_ZLIB", COMPRESS_ZLIB, 0 );
  add_integer_constant( "COMPRESS_FRACTAL", COMPRESS_FRACTAL, 0 );


  add_integer_constant("NORMAL_MODE", NORMAL_MODE, 0);
  add_integer_constant("DISSOLVE_MODE", DISSOLVE_MODE, 0);
  add_integer_constant("BEHIND_MODE",BEHIND_MODE, 0);
  add_integer_constant("MULTIPLY_MODE",MULTIPLY_MODE, 0);
  add_integer_constant("SCREEN_MODE",SCREEN_MODE, 0);
  add_integer_constant("OVERLAY_MODE",OVERLAY_MODE, 0);
  add_integer_constant("DIFFERENCE_MODE",DIFFERENCE_MODE, 0);
  add_integer_constant("ADDITION_MODE",ADDITION_MODE, 0);
  add_integer_constant("SUBTRACT_MODE",SUBTRACT_MODE, 0);
  add_integer_constant("DARKEN_ONLY_MODE",DARKEN_ONLY_MODE, 0);
  add_integer_constant("LIGHTEN_ONLY_MODE",LIGHTEN_ONLY_MODE, 0);
  add_integer_constant("HUE_MODE",HUE_MODE, 0);
  add_integer_constant("SATURATION_MODE",SATURATION_MODE, 0);
  add_integer_constant("COLOR_MODE",COLOR_MODE, 0);
  add_integer_constant("VALUE_MODE",VALUE_MODE, 0);
  add_integer_constant("DIVIDE_MODE",DIVIDE_MODE, 0);
  add_integer_constant("ERASE_MODE",ERASE_MODE, 0);
  add_integer_constant("REPLACE_MODE",REPLACE_MODE, 0);


  add_integer_constant( "Red", Red, 0 );
  add_integer_constant( "Green", Green, 0 );
  add_integer_constant( "Blue", Blue, 0 );
  add_integer_constant( "Gray", Gray, 0 );
  add_integer_constant( "Indexed", Indexed, 0 );
  add_integer_constant( "Auxillary", Auxillary, 0 );

  add_integer_constant( "RGB_GIMAGE", RGB_GIMAGE, 0 );
  add_integer_constant( "RGBA_GIMAGE", RGBA_GIMAGE, 0 );
  add_integer_constant( "GRAY_GIMAGE", GRAY_GIMAGE, 0 );
  add_integer_constant( "GRAYA_GIMAGE", GRAYA_GIMAGE, 0 );
  add_integer_constant( "INDEXED_GIMAGE", INDEXED_GIMAGE, 0 );
  add_integer_constant( "INDEXEDA_GIMAGE", INDEXEDA_GIMAGE, 0 );

#define STRING(X) s_##X = make_shared_binary_string(#X,sizeof( #X )-sizeof(""))
#include "xcf_constant_strings.h"
#undef STRING

  start_new_program();
  ADD_STORAGE( struct substring );
  add_function("cast", f_substring_cast, "function(string:mixed)",0);
  add_function("`[]", f_substring_index, "function(int:int)",0);
  add_function("get_short", f_substring_get_short, "function(int:int)", 0 );
  add_function("get_ushort", f_substring_get_ushort, "function(int:int)", 0 );
  add_function("get_int", f_substring_get_int, "function(int:int)", 0 );
  add_function("get_uint", f_substring_get_uint, "function(int:int)", 0 );
  add_function("_sprintf",f_substring__sprintf,
               "function(int,mapping:mixed)", 0);
/*   set_init_callback(init_substring); */
  set_exit_callback(free_substring);
  substring_program = end_program();  
}


void exit_image_xcf()
{
#define STRING(X) free_string(s_##X)
#include "xcf_constant_strings.h"
#undef STRING
  free_program( substring_program );
}
