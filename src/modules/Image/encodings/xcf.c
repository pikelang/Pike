#include "global.h"
RCSID("$Id: xcf.c,v 1.11 1999/08/16 18:10:23 grubba Exp $");

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

#include "image.h"
#include "colortable.h"

extern struct program *image_colortable_program;
extern struct program *image_program;

/*
**! module Image
**! submodule XCF
**!
*/

#define TILE_WIDTH 64
#define TILE_HEIGHT 64

#define STRING(X) static struct pike_string *s_##X;
#include "xcf_constant_strings.h"
#undef STRING


struct buffer
{
  unsigned int len;
  unsigned char *str;
};


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

static char *read_data( struct buffer * from, unsigned int len )
{
  char *res;
  if( from->len < len )
    error("Not enough space for %ud bytes\n", len);
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

static struct property read_property( struct buffer * data )
{
  int i;
  struct property res;
  res.type = read_uint( data );
  if(res.type == PROP_COLORMAP)
  {
    unsigned int foo;
    read_uint(data); /* bogus 'len'... */
    foo = read_uint( data );
    res.data.len = foo*3;
    res.data.str = read_data( data,foo*3 );
  } else {
    res.data.len = read_uint( data );
    res.data.str = read_data( data,res.data.len );
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
    struct tile *n;
    n = t->next;
    free(t);
    l->first_tile = n;
  }
}

static void free_layer_mask( struct layer_mask *l )
{
  struct property *p;

  while( (p = l->first_property) )
  {
    struct property *n;
    n = p->next;
    free(p);
    l->first_property = n;
  }
  free_level( &l->image_data.level );
}


static void free_layer( struct layer *l )
{
  struct property *p;

  while( (p = l->first_property) )
  {
    struct property *n;
    n = p->next;
    free(p);
    l->first_property = n;
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
    struct property *n;
    n = p->next;
    free(p);
    l->first_property = n;
  }
  free_level( &l->image_data.level );
}


static void free_image( struct gimp_image *i )
{
  struct property *p;
  struct layer *l;
  while( (p = i->first_property) )
  {
    struct property *n;
    n = p->next;
    free(p);
    i->first_property = n;
  }
  while( (l = i->first_layer) )
  {
    struct layer *n;
    n = l->next;
    free_layer( l );
    free( l );
    i->first_layer = n;
  }
}

static struct level read_level( struct buffer *buff,
                                struct buffer *initial )
{
  struct level res;
  ONERROR err;
  int offset;
  struct tile *last_tile = NULL;
  int all_tiles_eq = 0;
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
    {
      res.first_tile = tile;
      if(offset2)
        all_tiles_eq = offset2-offset;
    }
    if(all_tiles_eq && offset2 && offset2-offset != all_tiles_eq)
      all_tiles_eq = 0;
    if(all_tiles_eq)
      ob.len = all_tiles_eq;
    else if(offset2)
      ob.len = offset2-offset;
    else
      ob.len = MINIMUM((TILE_WIDTH*TILE_HEIGHT*5),ob.len);
    tile->data = ob;
    tile->next = NULL;
/* fprintf(stderr, "tile, o=%d; o2=%d; l=%d\n", offset, offset2,ob.len); */
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
  struct buffer ob;

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
  struct buffer ob;

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
  res.type = read_int( buff );
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

  h_offset = read_int( buff );
  lm_offset = read_int( buff );

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
    error("This is not an xcf file (to little data)\n");
  if(!(data->str[0] == 'g' &&
       data->str[1] == 'i' &&
       data->str[2] == 'm' &&
       data->str[3] == 'p' &&
       data->str[4] == ' '))
  {
    if(strlen(data->str) == 13)
      error("This is not an xcf file (%s)\n", data->str);
    else
      error("This is not an xcf file\n");
  }
  data->str+=14; data->len-=14;

  res.width = read_uint( data );
  res.height = read_uint( data );
  res.type = read_int( data );

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
  push_string( make_shared_binary_string( b->str, b->len ) );
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
  f_aggregate( sp-osp );
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
    error("Illegal hierarchy level sizes!\n");

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
  f_aggregate( sp-tsp );
  f_aggregate_mapping( sp-osp );
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
  f_aggregate_mapping( sp-osp );
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
  f_aggregate_mapping( sp-osp );
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
  f_aggregate_mapping( sp-osp );
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
  f_aggregate_mapping( sp-osp );
}


static void image_xcf____decode( INT32 args )
{
  struct pike_string *s;
  struct buffer b;
  struct gimp_image res;
  ONERROR err;
  get_all_args( "___decode", args, "%S", &s );
  if(args > 1)
    error("Too many arguments to Image.XCF.___decode()\n");

  b.str = s->str; b.len = s->len;
  res = read_image( &b );
  SET_ONERROR( err, free_image, &res );
  push_image( &res );
  UNSET_ONERROR( err );
  free_image( &res );
  stack_swap();
  pop_stack();
}


unsigned char read_char( struct buffer *from )
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


void image_xcf_f__rle_decode( INT32 args )
{
  struct pike_string *t;
  struct buffer s;
  struct buffer od, d;
  int bpp, xsize, ysize, i;
  get_all_args( "_rle_decode", args, "%S%d%d%d", &t, &bpp, &xsize, &ysize);

  s.len = t->len;
  s.str = t->str;
  od.len = xsize*ysize*bpp;
  od.str = xalloc( xsize*ysize*bpp ); /* Max and only size, really */
  d = od;
  for(i=0; i<bpp; i++)
  {
    int nelems = xsize*ysize;
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

  pop_n_elems(args);
    /* fprintf(stderr, "%d bytes of source data used out of %d bytes\n" */
    /*         "%d bytes decoded data generated\n",  */
    /*         t->len-s.len,t->len,od.len); */
  push_string(make_shared_binary_string(od.str,od.len));
  free(od.str);
}


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
  extern void check_signals();
  struct object *io,*ao, *cmapo;
  struct array *tiles;
  struct image *i, *a=NULL;
  struct neo_colortable *cmap = NULL;
  rgb_group *colortable=NULL;
  int rle, bpp, span;
  unsigned int l, x=0, y=0, cx, cy;
  get_all_args( "_decode_tiles", args, "%o%O%a%i%i%O",
                &io, &ao, &tiles, &rle, &bpp, &cmapo);
  if( !(i = (struct image *)get_storage( io, image_program )))
    error("Wrong type object argument 1 (image)\n");
  if(ao && !(a = (struct image *)get_storage( ao, image_program )))
    error("Wrong type object argument 2 (image)\n");
  if( cmapo && 
      !(cmap=(struct neo_colortable *)get_storage(cmapo,
                                                  image_colortable_program)))
    error("Wrong type object argument 4 (colortable)\n");
  for(l=0; l<(unsigned int)tiles->size; l++)
    if(tiles->item[l].type != T_STRING)
      error("Wrong type array argument 3 (tiles)\n");
  if(a && (i->xsize != a->xsize ||i->ysize != a->ysize))
    error("Image and alpha objects are not identically sized.\n");

  if(cmap)
  {
    colortable = malloc(sizeof(rgb_group)*image_colortable_size( cmap ));
    image_colortable_write_rgb( cmap, (unsigned char *)colortable );
  }


/*   switch(bpp) */
/*   { */
/*    case 1: */
/*    case 3: */
/*      if(ao) */
/*      { */
/*        destruct( ao ); */
/*        a=0; */
/*        ao=0; */
/*      } */
/*      break; */
/*   } */

  x=y=0;
  for(l=0; l<(unsigned)tiles->size; l++)
  {
    struct pike_string *tile = tiles->item[l].u.string;
    unsigned int eheight, ewidth;
    unsigned char *s;
    ewidth = MINIMUM(TILE_WIDTH, i->xsize-x);
    eheight = MINIMUM(TILE_HEIGHT, i->ysize-y);
    add_ref(tile);

/*     fprintf(stderr, "       tile %d/%d [%dx%d]  %dbpp      \n", */
/*             l+1, tiles->size, ewidth, eheight,bpp); */

    if(rle)
    {
      push_string( tile );
      push_int( bpp );
      push_int( ewidth );
      push_int( eheight );
      image_xcf_f__rle_decode( 4 );
      tile = (struct pike_string *)debug_malloc_pass(sp[-1].u.string);
      if(sp[-1].type != T_STRING)
        fatal("Internal disaster in XCF module\n");
      sp--;
    }

    if( (unsigned)(tile->len) < (unsigned)(eheight * ewidth * bpp ))
      error("Too small tile, was %d bytes, I really need %d\n",
            tile->len, eheight*ewidth * bpp);
    
    s = tile->str;
 
    check_signals(); /* Allow ^C */

    for(cy=0; cy<eheight; cy++)
    {
      for(cx=0; cx<ewidth; cx++)
      {
        rgb_group pix;
        rgb_group apix;
        int ind = (cx+cy*ewidth);

        if(rle)
          span = ewidth*eheight;
        else
          span = 1;
        if(cx+x > (unsigned)i->xsize)  continue;
        if(cy+y > (unsigned)i->ysize)  continue;

        switch( bpp )
        {
         case 1: /* indexed or grey */
           if(colortable) 
             pix = colortable[s[ind]];
           else 
             pix.r = pix.g = pix.b = s[ind];
           break;
         case 2: /* indexed or grey with alpha */
           if(colortable) 
             pix = colortable[s[ind]];
           else 
             pix.r = pix.g = pix.b = s[ind];
           apix.r = apix.g = apix.b = s[ind+span];
           break;
         case 3: /* rgb */
           pix.r = s[ind];
           pix.g = s[ind+span];
           pix.b = s[ind+span*2];
           break;
         case 4: /* rgb with alpha */
           pix.r = s[ind];
           pix.g = s[ind+span*1];
           pix.b = s[ind+span*2];
           apix.r = apix.g = apix.b = s[ind+span*3];
           break;
        }
        ind = i->xsize*(cy+y)+(cx+x);
        i->img[ind] = pix;
        if(a) a->img[ind] = apix;
      }
    }
    x += TILE_WIDTH;
    if( x >= (unsigned)i->xsize )
    {
      x = 0;
      y += TILE_HEIGHT;
    }
    if(y>=(unsigned)i->ysize)
    {
      free_string(tile);
      if(colortable) free( colortable );
      return;
    }
    free_string(tile);
  }
  if(colortable) free( colortable );
  
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

#define STRING(X) s_##X = make_shared_binary_string(#X,sizeof( #X )-sizeof(""));
#include "xcf_constant_strings.h"
#undef STRING
}


void exit_image_xcf()
{
#define STRING(X) free_string(s_##X)
#include "xcf_constant_strings.h"
#undef STRING
}

