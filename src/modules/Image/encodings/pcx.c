#include "global.h"
RCSID("$Id: pcx.c,v 1.2 1999/04/09 04:50:43 hubbe Exp $");

#include "config.h"

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
#include "dynamic_buffer.h"

#include "image.h"
#include "colortable.h"

extern struct program *image_colortable_program;
extern struct program *image_program;

/*
**! module Image
**! submodule PCX
**!
*/

struct buffer
{
  unsigned int len;
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
  char *db;
  if(b->len < len)
    return 0;
  db = b->str;
  b->str+=len;
  b->len-=len;
  return db;
}

unsigned char get_char( struct buffer *b )
{
  if(b->len > 1)
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
      MEMCPY( dest, source, nelems );
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
  unsigned char *line = xalloc( hdr->bytesperline*hdr->planes );
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
    get_rle_decoded_from_data( line, b,hdr->bytesperline*hdr->planes, hdr, &state );
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
  unsigned char *line = xalloc( hdr->bytesperline*hdr->planes );
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
  unsigned char *line = xalloc( hdr->bytesperline*hdr->planes );
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
  unsigned char *line = xalloc( hdr->bytesperline*hdr->planes );
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
  b.str = data->str;
  b.len = data->len;

  if(b.len < sizeof(struct pcx_header))
    error("There is not enough data available for this to be a PCX image\n");
  pcx_header = *((struct pcx_header *)get_chunk(&b,sizeof(struct pcx_header)));
#if BYTEORDER == 1234
  SWAP_S(pcx_header.x1);
  SWAP_S(pcx_header.x2);
  SWAP_S(pcx_header.y1);
  SWAP_S(pcx_header.y2);
  SWAP_S(pcx_header.bytesperline);
  SWAP_S(pcx_header.color);
#endif

  if(pcx_header.manufacturer != 10)
    error("This is not a known type of PCX\n");

  push_int( pcx_header.x2 - pcx_header.x1 + 1 );
  push_int( pcx_header.y2 - pcx_header.y1 + 1 );

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
        error("Unsupported number of planes for %d bpp image: %d\n",
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
        error("Unsupported number of planes for %d bpp image: %d\n",
              pcx_header.bpp, pcx_header.planes);
     }
   default:
     error("Unsupported bits per plane: %d\n", pcx_header.bpp);
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
**! method object _decode(string data)
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



static struct pike_string *param_raw;

/* TODO: */
/*
** method string encode(object image)
** method string encode(object image, mapping options)
** 	Encodes a Targa image. 
**
**     The <tt>options</tt> argument may be a mapping
**	containing zero or more encoding options:
**
**	<pre>
**	normal options:
**	    "raw":1
**		Do not RLE encode the image
**	</pre>
*/

static struct program *image_encoding_pcx_program=NULL;
void init_image_pcx( )
{
  start_new_program();
  add_function( "_decode", image_pcx__decode, 
                "function(string:mapping(string:object))", 0);
  add_function( "decode", image_pcx_decode, 
                "function(string:object)", 0);
/*   add_function( "encode", image_pcx_encode,  */
/*                 "function(object,mapping|void:string)", 0); */
  image_encoding_pcx_program=end_program();

  push_object(clone_object(image_encoding_pcx_program,0));
  {
    struct pike_string *s=make_shared_string("PCX");
    add_constant(s,sp-1,0);
    free_string(s);
  }
  param_raw=make_shared_string("raw");
}

void exit_image_pcx(void)
{
  if(image_encoding_pcx_program)
  {
    free_program(image_encoding_pcx_program);
    image_encoding_pcx_program=0;
    free_string(param_raw);
  }
}
