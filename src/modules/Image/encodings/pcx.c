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
#include "bignum.h"

#include "image.h"
#include "colortable.h"


#define sp Pike_sp

extern struct program *image_colortable_program;
extern struct program *image_program;

/*
**! module Image
**! submodule PCX
**!
*/

struct buffer
{
  size_t len;
  char *str;
};

struct pcx_header /* All fields are in ~NBO */
{
  unsigned char manufacturer;
  unsigned char version;
  unsigned char rle_encoded;
  unsigned char bpp;
  unsigned short x1, y1;
  unsigned short x2, y2;
  unsigned short hdpi;
  unsigned short vdpi;
  unsigned char palette[48];
  unsigned char reserved;
  unsigned char planes;
  unsigned short bytesperline;
  unsigned short color;
  unsigned char filler[58];
};

unsigned char *get_chunk( struct buffer *b, unsigned int len )
{
  unsigned char *db;
  if(b->len < len)
    return 0;
  db = (unsigned char *)b->str;
  b->str+=len;
  b->len-=len;
  return db;
}

unsigned char get_char( struct buffer *b )
{
  if(b->len)
  {
    b->str++;
    b->len--;
    return b->str[-1];
  }
  return 0; /* This is acceptable for this application... */
}

#define SWAP_S(x) ((unsigned short)((((x)&0x00ff)<<8) | ((x)&0xff00)>>8))
#define SWAP_L(x) (SWAP_S((x)>>16)|SWAP_S((x)&0xffff)<<16)

struct rle_state
{
  unsigned int nitems;
  unsigned char value;
};

void get_rle_decoded_from_data( unsigned char *dest, struct buffer * source, 
                                int nelems,
                                struct pcx_header *hdr, 
                                struct rle_state *state )
{
  if(!hdr->rle_encoded)
  {
    unsigned char *c = get_chunk( source, nelems );
    if(c)
      MEMCPY( dest, c, nelems );
    else
      MEMSET( dest, 0, nelems );
    return;
  }

  while (nelems--) 
  {
    if(state->nitems == 0) 
    {
      unsigned char nb;
      nb = get_char( source );
      if (nb < 0xc0) {
        state->nitems = 1;
        state->value = nb;
      } else {
        state->nitems = nb - 0xc0;
        state->value = get_char( source );
      }
    }
    state->nitems--;
    *(dest++)= state->value;
  }
}

static void load_rgb_pcx( struct pcx_header *hdr, struct buffer *b, 
                          rgb_group *dest )
{
  unsigned char *line = (unsigned char *)xalloc(hdr->bytesperline*hdr->planes);
  struct rle_state state;
  int width, height;
  int x, y;
  THREADS_ALLOW();
  state.nitems = 0;
  state.value = 0;
  width = hdr->x2 - hdr->x1 + 1;
  height = hdr->y2 - hdr->y1 + 1;

  for(y=0; y<height; y++)
  {
    get_rle_decoded_from_data(line, b,hdr->bytesperline*hdr->planes, hdr, &state);
    /* rrrr... ggg... bbb.. */
    for(x=0; x<width; x++)
    {
      dest->r = line[x];
      dest->g = line[x+hdr->bytesperline];
      (dest++)->b = line[x+hdr->bytesperline+hdr->bytesperline];
    }
  }
  free(line);
  THREADS_DISALLOW();
}

static void load_mono_pcx( struct pcx_header *hdr, struct buffer *b,
                           rgb_group *dest)
{
  unsigned char *line = (unsigned char *)xalloc(hdr->bytesperline*hdr->planes);
  struct rle_state state;
  int width, height;
  int x, y;
  THREADS_ALLOW();
  state.nitems = 0;
  state.value = 0;
  width = hdr->x2 - hdr->x1 + 1;
  height = hdr->y2 - hdr->y1 + 1;

  for(y=0; y<height; y++)
  {
    get_rle_decoded_from_data(line,b,hdr->bytesperline*hdr->planes,hdr,&state );
    for(x=0; x<width; x++)
    {
      if(line[x>>3]&(128>>(x%8)))
        dest->r = dest->g = dest->b = 255;
      dest++;
    }
  }
  free(line);
  THREADS_DISALLOW();
}

static void load_planar_palette_pcx( struct pcx_header *hdr, struct buffer *b,
                                     rgb_group *dest)
{
  unsigned char *line = (unsigned char *)xalloc(hdr->bytesperline*hdr->planes);
  struct rle_state state;
  rgb_group * palette = (rgb_group *)hdr->palette;
  int width, height;
  int x, y;
  THREADS_ALLOW();
  state.nitems = 0;
  state.value = 0;
  width = hdr->x2 - hdr->x1 + 1;
  height = hdr->y2 - hdr->y1 + 1;

  for(y=0; y<height; y++)
  {
    get_rle_decoded_from_data(line,b,hdr->bytesperline*hdr->planes,hdr,&state );
    for(x=0; x<width; x++)
    {
      unsigned char pixel = ((line[x>>3]&(128>>(x%8)) ? 1 : 0) +
                             (line[(x>>3)+hdr->bytesperline]&(128>>(x%8)) ? 2 : 0) +
                             (line[(x>>3)+hdr->bytesperline*2]&(128>>(x%8))?4 : 0) +
                             (line[(x>>3)+hdr->bytesperline*3]&(128>>(x%8))?8 : 0));
      *(dest++) = palette[pixel];
    }
  }
  free(line);
  THREADS_DISALLOW();
}


static void load_palette_pcx( struct pcx_header *hdr, struct buffer *b,
                             rgb_group *dest)
{
  unsigned char *line = (unsigned char *)xalloc(hdr->bytesperline*hdr->planes);
  struct rle_state state;
  /* It's at the end of the 'file' */
  rgb_group * palette = (rgb_group *)(b->str+b->len-(256*3)); 
  int width, height;
  int x, y;
  THREADS_ALLOW();
  state.nitems = 0;
  state.value = 0;
  width = hdr->x2 - hdr->x1 + 1;
  height = hdr->y2 - hdr->y1 + 1;

  for(y=0; y<height; y++)
  {
    get_rle_decoded_from_data(line,b,hdr->bytesperline*hdr->planes,hdr,&state );
    for(x=0; x<width; x++)
      *(dest++) = palette[line[x]];
  }
  free(line);
  THREADS_DISALLOW();
}

static struct object *low_pcx_decode( struct pike_string *data )
{
  struct buffer b;
  rgb_group *dest;
  struct pcx_header pcx_header;
  struct ONERROR onerr;
  struct object *io;
  ptrdiff_t width, height;
  b.str = data->str;
  b.len = data->len;

  if(b.len < sizeof(struct pcx_header))
    Pike_error("There is not enough data available for this to be a PCX image\n");
  pcx_header = *((struct pcx_header *)get_chunk(&b,sizeof(struct pcx_header)));
#if BYTEORDER == 1234
  SWAP_S(pcx_header.x1);
  SWAP_S(pcx_header.x2);
  SWAP_S(pcx_header.y1);
  SWAP_S(pcx_header.y2);
  SWAP_S(pcx_header.bytesperline);
  SWAP_S(pcx_header.color);
#endif

  if((pcx_header.manufacturer != 10) || (pcx_header.reserved) ||
     (pcx_header.rle_encoded & ~1))
    Pike_error("This is not a known type of PCX\n");

  if ((pcx_header.bpp != 8) &&
      (pcx_header.bpp != 1)) {
    Pike_error("Unsupported bits per plane: %d\n", pcx_header.bpp);
  }

  if ((pcx_header.planes < 1) || (pcx_header.planes > 4)) {
    Pike_error("Unsupported number of planes: %d\n", pcx_header.planes);
  }

  width = pcx_header.x2 - pcx_header.x1 + 1;
  height = pcx_header.y2 - pcx_header.y1 + 1;
  if ((width <= 0) || (height <= 0)) {
    Pike_error("Unsupported PCX image.\n");
  }

  push_int64(width);
  push_int64(height);

  io = clone_object( image_program, 2 );
  dest = ((struct image *)get_storage( io, image_program ))->img;
  SET_ONERROR(onerr, do_free_object, io );
  
  switch(pcx_header.bpp)
  {
   case 8:
     switch(pcx_header.planes)
     {
      case 1:
        load_palette_pcx( &pcx_header, &b, dest );
        break;
      case 3:
        load_rgb_pcx( &pcx_header, &b, dest );
        break;
      default:
        Pike_error("Unsupported number of planes for %d bpp image: %d\n",
              pcx_header.bpp, pcx_header.planes);
     }
     break;
   case 1:
     switch(pcx_header.planes)
     {
      case 1:
        load_mono_pcx( &pcx_header, &b, dest );
        break;
      case 4: /* palette 16 bpl planar image!? */
        load_planar_palette_pcx( &pcx_header, &b, dest );
        break;
      default:
        Pike_error("Unsupported number of planes for %d bpp image: %d\n",
              pcx_header.bpp, pcx_header.planes);
     }
   default:
     Pike_error("Unsupported bits per plane: %d\n", pcx_header.bpp);
  }
  UNSET_ONERROR(onerr);
  return io;
}


/*
**! method object decode(string data)
**! 	Decodes a PCX image. 
**!
**! note
**!	Throws upon error in data.
*/
void image_pcx_decode( INT32 args )
{
  struct pike_string *data;
  struct object *o;
  get_all_args( "Image.PCX.decode", args, "%S", &data );
  o = low_pcx_decode( data );
  pop_n_elems(args);
  push_object( o );
}


/*
**! method mapping _decode(string data)
**! 	Decodes a PCX image to a mapping. 
**!
**! note
**!	Throws upon error in data.
*/
void image_pcx__decode( INT32 args )
{
  image_pcx_decode( args );
  push_constant_text( "image" );
  stack_swap();
  f_aggregate_mapping(2);
}



static struct pike_string *opt_raw,  *opt_dpy,  *opt_xdpy,  *opt_colortable,
                          *opt_ydpy,  *opt_xoffset, *opt_yoffset;
/*
**! method string encode(object image)
**! method string encode(object image, mapping options)
**! method string _encode(object image)
**! method string _encode(object image, mapping options)
**! 	Encodes a PCX image.  The _encode and the encode functions are identical
**!
**!     The <tt>options</tt> argument may be a mapping
**!	containing zero or more encoding options:
**!
**!	<pre>
**!	normal options:
**!	    "raw":1
**!		Do not RLE encode the image
**!	    "dpy":int
**!	    "xdpy":int
**!	    "ydpy":int
**!		Image resolution (in pixels/inch, integer numbers)
**!	    "xoffset":int
**!	    "yoffset":int
**!		Image offset (not used by most programs, but gimp uses it)
**!	</pre>
*/

struct options
{
  int raw;
  int offset_x, offset_y;
  int hdpi, vdpi;
  struct neo_colortable *colortable;
};


static void f_rle_encode( INT32 args )
{
  struct pike_string *data;
  struct string_builder result;
  unsigned char value, *source;
  unsigned char nelems;
  int i;
  get_all_args( "rle_encode", args, "%S", &data );
  init_string_builder( &result, 0 );

  source = (unsigned char *)data->str;
  for(i=0; i<data->len;)
  {
    value = *(source++);
    nelems = 1;
    i++;
    while( i<data->len && nelems<63 && *source == value)
    {
      nelems++;
      source++;
      i++;
    }
    if(nelems == 1 && value < 0xC0 )
      string_builder_putchar( &result, value );
    else
    {
      string_builder_putchar( &result, 0xC0 + nelems );
      string_builder_putchar( &result, value );
    }
  }
#if 0
  fprintf(stderr, "read: %d\n", source-(unsigned char*)data->str);
  fprintf(stderr, "RLE encode source len = %d;  dest len = %d\n",
          sp[-1].u.string->len, result.s->len );
#endif
  pop_n_elems( args );
  push_string( finish_string_builder( &result ));
}

static struct pike_string *encode_pcx_24( struct pcx_header *pcx_header,
                                          struct image *data,
                                          struct options *opts )
{
  struct pike_string *b;
  int x, y;
  char *buffer;
  rgb_group *s;
  
  pcx_header->planes = 3;

  push_string( make_shared_binary_string( (char *)pcx_header, 
                                          sizeof(struct pcx_header) ) );
  
  buffer = malloc(data->xsize*data->ysize*3);
  s = data->img;
  for(y=0; y<data->ysize; y++)
  {
    unsigned char *line = (unsigned char *)buffer+y*data->xsize*3;
    for(x=0; x<data->xsize; x++)
    {
      line[x] = s->r;
      line[x+data->xsize] = s->g;
      line[x+data->xsize*2] = s->b;
      s++;
    }
  }
  push_string(make_shared_binary_string(buffer,data->xsize*data->ysize*3));
  free(buffer);

  if(pcx_header->rle_encoded)
    f_rle_encode( 1 );

  f_add( 2 );
  b = sp[-1].u.string;
  sp--;
  return b;
}


static struct pike_string *encode_pcx_8( struct pcx_header *pcx_header,
                                         struct image *data,
                                         struct options *opts )
{
  struct pike_string *b;
  char *buffer;

  pcx_header->planes = 1;
  push_string( make_shared_binary_string( (char *)pcx_header, 
                                          sizeof(struct pcx_header) ) );


  buffer = malloc(data->xsize*data->ysize);
  image_colortable_index_8bit_image( opts->colortable, data->img,
				     (unsigned char *)buffer,
				     data->xsize*data->ysize, data->xsize );
  push_string(make_shared_binary_string(buffer,data->xsize*data->ysize));
  free(buffer);

  if(pcx_header->rle_encoded)
    f_rle_encode( 1 );

  {
    char data[256*3+1];
    MEMSET(data, 0x0c, 256*3+1);
    image_colortable_write_rgb(opts->colortable, (unsigned char *)data+1);
    push_string(make_shared_binary_string(data,256*3+1));
  }

  f_add( 3 );
  b = sp[-1].u.string;
  sp--;
  return b;
}

static struct pike_string *low_pcx_encode(struct image *data,struct options *opts)
{
  struct pcx_header pcx_header;
  pcx_header.x1 = opts->offset_x;
  pcx_header.x2 = opts->offset_x+data->xsize-1;
  pcx_header.y1 = opts->offset_y;
  pcx_header.y2 = opts->offset_y+data->ysize-1;
  pcx_header.hdpi = opts->hdpi;
  pcx_header.vdpi = opts->vdpi;
  pcx_header.bytesperline = data->xsize;
  pcx_header.rle_encoded = opts->raw?0:1;
  pcx_header.manufacturer = 10;
  pcx_header.version = 5;
  pcx_header.reserved = 0;
  pcx_header.bpp = 8;
  MEMSET(pcx_header.palette, 0, 48);
  MEMSET(pcx_header.filler, 0, 58);
  pcx_header.color = 1;
#if BYTEORDER == 1234
  SWAP_S(pcx_header.hdpi);
  SWAP_S(pcx_header.vdpi);
  SWAP_S(pcx_header.x1);
  SWAP_S(pcx_header.y1);
  SWAP_S(pcx_header.x2);
  SWAP_S(pcx_header.y2);
  SWAP_S(pcx_header.bytesperline);
  SWAP_S(pcx_header.color);
#endif
  if(!opts->colortable)
    return encode_pcx_24( &pcx_header, data, opts );
  return encode_pcx_8( &pcx_header, data, opts );
}

static int parameter_int(struct svalue *map,struct pike_string *what,
                         INT32 *p)
{
   struct svalue *v;
   v=low_mapping_string_lookup(map->u.mapping,what);
   if (!v || v->type!=T_INT) 
     return 0;
   *p=v->u.integer;
   return 1;
}

static int parameter_colortable(struct svalue *map,struct pike_string *what,
                                struct neo_colortable **p)
{
   struct svalue *v;
   v=low_mapping_string_lookup(map->u.mapping,what);
   if (!v || v->type!=T_OBJECT) return 0;
   if( !(*p = (struct neo_colortable *)get_storage( v->u.object, image_colortable_program )))
     return 0;
   return 1;
}

void image_pcx_encode( INT32 args )
{
  struct options c;
  struct pike_string *res;
  struct object *i;
  struct image *img;

  get_all_args( "Image.PCX.encode", args, "%o", &i );

  if(!get_storage( i, image_program ))
    Pike_error("Invalid object argument to Image.PCX.encode\n");

  img = ((struct image *)get_storage( i, image_program ));
  
  MEMSET(&c, sizeof(c), 0);
  c.hdpi = 150;
  c.vdpi = 150;
  c.raw = 0;
  c.offset_x = c.offset_y = 0;
  c.colortable = 0;
  if(args > 1)
  {
    int dpy;
    if(sp[-args+1].type != T_MAPPING)
      Pike_error("Invalid argument 2 to Image.PCX.encode. Expected mapping.\n");
    parameter_int( sp-args+1, opt_raw, &c.raw );
    if(parameter_int( sp-args+1, opt_dpy, &dpy ))
      c.hdpi = c.vdpi = dpy;
    parameter_int( sp-args+1, opt_xdpy, &c.hdpi );
    parameter_int( sp-args+1, opt_ydpy, &c.vdpi );
    parameter_int( sp-args+1, opt_xoffset, &c.offset_x );
    parameter_int( sp-args+1, opt_yoffset, &c.offset_y );
    parameter_colortable( sp-args+1, opt_colortable, &c.colortable );
  }
  res = low_pcx_encode( img, &c );
  pop_n_elems( args );
  push_string( res );
}

static struct program *image_encoding_pcx_program=NULL;
void init_image_pcx( )
{
  add_function( "_decode", image_pcx__decode, 
                "function(string:mapping(string:object))", 0);
  add_function( "decode", image_pcx_decode, 
                "function(string:object)", 0);
  add_function( "encode", image_pcx_encode, 
                "function(object,mapping|void:string)", 0);
  add_function( "_encode", image_pcx_encode, 
                "function(object,mapping|void:string)", 0);

  opt_raw=make_shared_string("raw");
  opt_dpy=make_shared_string("dpy");
  opt_xdpy=make_shared_string("xdpy");
  opt_ydpy=make_shared_string("ydpy");
  opt_xoffset=make_shared_string("xoffset");
  opt_colortable=make_shared_string("colortable");
  opt_yoffset=make_shared_string("yoffset");
}

void exit_image_pcx(void)
{
   free_string(opt_raw);
   free_string(opt_dpy);
   free_string(opt_xdpy);
   free_string(opt_ydpy);
   free_string(opt_xoffset);
   free_string(opt_colortable);
   free_string(opt_yoffset);
}
