/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: xbm.c,v 1.25 2005/01/23 13:30:05 nilsson Exp $
*/

#define NO_PIKE_SHORTHAND

#include "global.h"
#include "image_machine.h"

#include "interpret.h"
#include "svalue.h"
#include "object.h"
#include "program.h"
#include "pike_error.h"
#include "mapping.h"
#include "stralloc.h"
#include "operators.h"
#include "threads.h"
#include "module_support.h"
#include "builtin_functions.h"

#include "image.h"
#include "colortable.h"



extern struct program *image_colortable_program;
extern struct program *image_program;

/*
**! module Image
**! submodule XBM
**!
*/


#define SWAP_S(x) ((unsigned short)((((x)&0x00ff)<<8) | ((x)&0xff00)>>8))
#define SWAP_L(x) (SWAP_S((x)>>16)|SWAP_S((x)&0xffff)<<16)

struct buffer
{
  size_t len;
  char *str;
};

int buf_search( struct buffer *b, unsigned char match )
{
  unsigned int off = 0;
  if(b->len <= 1)
    return 0;
  while(off < b->len)
  {
    if(b->str[off]  == match)
      break;
    off++;
  }
  off++;
  if(off >= b->len)
    return 0;
  b->str += off;
  b->len -= off;
  return 1;
}

static int buf_getc( struct buffer *fp )
{
  if(fp->len >= 1)
  {
    fp->len--;
    return (int)*((unsigned char *)fp->str++);
  }
  return '0';
}

static int hextoint( int what )
{
  if(what >= '0' && what <= '9')
    return what-'0';
  if(what >= 'a' && what <= 'f')
    return 10+what-'a';
  if(what >= 'A' && what <= 'F')
    return 10+what-'A';
  return 0;
}

static struct object *load_xbm( struct pike_string *data )
{
  int width, height;
  int x, y;
  struct buffer buff;
  struct buffer *b = &buff;
  rgb_group *dest;
  struct object *io;

  buff.str = data->str;
  buff.len = data->len;

  if(!buf_search( b, '#' ) || !buf_search( b, ' ' ) || !buf_search( b, ' ' ))
    Pike_error("This is not a XBM image!\n");
  width = atoi(b->str);
  if(width <= 0)
    Pike_error("This is not a XBM image!\n");
  if(!buf_search( b, '#' ) || !buf_search( b, ' ' ) || !buf_search( b, ' ' ))
    Pike_error("This is not a XBM image!\n");
  height = atoi(b->str);
  if(height <= 0)
    Pike_error("This is not a XBM image!\n");
  
  if(!buf_search( b, '{' ))
    Pike_error("This is not a XBM image!\n");


  push_int( width );
  push_int( height );
  io = clone_object( image_program, 2 );
  dest = ((struct image *)get_storage(io, image_program))->img;
  /* .. the code below asumes black if the read fails.. */
  for(y=0; y<height; y++)
  {
    int next_byte, cnt;
    for(x=0; x<width;)
    {
      if(buf_search( b, 'x' ))
      {
        next_byte = (hextoint(buf_getc( b ))*0x10) | hextoint(buf_getc( b ));
        for(cnt=0; cnt<8&&x<width; cnt++,x++)
        {
          if((next_byte&(1<<(x%8))))
            dest->r = dest->g = dest->b = 255;
          dest++;
        }
      }
    }
  }
  return io;
}

static struct pike_string *save_xbm( struct image *i, struct pike_string *name )
{
  dynamic_buffer buf;
  char size[32];
  int x, y, first=-1;

#define ccat( X )   low_my_binary_strcat( X, (sizeof(X)-sizeof("")), &buf );

#define cname()  do{                                          \
      if(name)                                                \
        low_my_binary_strcat( name->str, name->len, &buf );   \
      else                                                    \
        ccat( "image" );                                      \
   } while(0)                                                 \



#define OUTPUT_BYTE(X) do{                                              \
      if(!++first)                                                      \
        sprintf( size, " 0x%02x", (X) );                                \
      else                                                              \
        sprintf( size, ",%s0x%02x", (first%12?" ":"\n "), (X) );        \
      (X)=0;                                                            \
      low_my_binary_strcat( size, strlen(size), &buf );                 \
  } while(0)


  initialize_buf(&buf);
  ccat( "#define ");  cname();  ccat( "_width " );
  sprintf( size, "%"PRINTPIKEINT"d\n", i->xsize );
  low_my_binary_strcat( size, strlen(size), &buf );

  ccat( "#define ");  cname();  ccat( "_height " );
  sprintf( size, "%"PRINTPIKEINT"d\n", i->ysize );
  low_my_binary_strcat( size, strlen(size), &buf );

  ccat( "static char " );  cname();  ccat( "_bits[] = {\n" );
  
  for(y=0; y<i->ysize; y++)
  {
    rgb_group *p = i->img+y*i->xsize;
    int next_byte = 0;
    for(x=0; x<i->xsize; x++)
    {
      if((p->r || p->g || p->b ))
        next_byte |= (1<<(x%8));
      if((x % 8) == 7)
        OUTPUT_BYTE( next_byte );
      p++;
    }
    if(i->xsize%8)
      OUTPUT_BYTE( next_byte );
  }
  ccat( "};\n" );
  return low_free_buf(&buf);
}



/*
**! method object decode(string data)
**! 	Decodes a XBM image. 
**!
**! note
**!	Throws upon error in data.
*/
static void image_xbm_decode( INT32 args )
{
  struct pike_string *data;
  struct object *o;
  get_all_args( "Image.XBM.decode", args, "%S", &data );
  o = load_xbm( data );
  pop_n_elems(args);
  push_object( o );
}


/*
**! method object _decode(string data)
**! method object _decode(string data, mapping options)
**! 	Decodes a XBM image to a mapping.
**!
**! 	<pre>
**!         Supported options:
**!        ([
**!           "fg":({fgcolor}),    // Foreground color. Default black
**!           "bg":({bgcolor}),    // Background color. Default white
**!           "invert":1,          // Invert the mask
**!         ])
**!	</pre>
**!
**! note
**!	Throws upon error in data.
*/


static struct pike_string *param_fg;
static struct pike_string *param_bg;
static struct pike_string *param_invert;
static void image_xbm__decode( INT32 args )
{
  struct array *fg = NULL;
  struct array *bg = NULL;
  int invert=0, ele;
  struct pike_string *data;
  struct object *i=NULL, *a;
  get_all_args( "Image.XBM.decode", args, "%S", &data );


  if (args>1)
  {
    if (Pike_sp[1-args].type!=PIKE_T_MAPPING)
      Pike_error("Image.XBM._decode: illegal argument 2\n");
      
    push_svalue(Pike_sp+1-args);
    ref_push_string(param_fg); 
    f_index(2);
    if(!UNSAFE_IS_ZERO(Pike_sp-1))
    {
      if(Pike_sp[-1].type != PIKE_T_ARRAY || Pike_sp[-1].u.array->size != 3)
        Pike_error("Wrong type for foreground. Should be array(int(0..255))"
              " with 3 elements\n");
      for(ele=0; ele<3; ele++)
        if(Pike_sp[-1].u.array->item[ele].type != PIKE_T_INT
           ||Pike_sp[-1].u.array->item[ele].u.integer < 0
           ||Pike_sp[-1].u.array->item[ele].u.integer > 255)
          Pike_error("Wrong type for foreground. Should be array(int(0..255))"
                " with 3 elements\n");
      fg = Pike_sp[-1].u.array;
    }
    Pike_sp--;

    push_svalue(Pike_sp+1-args);
    ref_push_string(param_bg);
    f_index(2);
    if(!UNSAFE_IS_ZERO(Pike_sp-1))
    {
      if(Pike_sp[-1].type != PIKE_T_ARRAY || Pike_sp[-1].u.array->size != 3)
        Pike_error("Wrong type for background. Should be array(int(0..255))"
              " with 3 elements\n");
      for(ele=0; ele<3; ele++)
        if(Pike_sp[-1].u.array->item[ele].type != PIKE_T_INT
           ||Pike_sp[-1].u.array->item[ele].u.integer < 0
           ||Pike_sp[-1].u.array->item[ele].u.integer > 255)
          Pike_error("Wrong type for background. Should be array(int(0..255))"
                " with 3 elements\n");
      bg = Pike_sp[-1].u.array;
    }
    Pike_sp--;
    
    push_svalue(Pike_sp+1-args);
    ref_push_string(param_invert);
    f_index(2);
    invert = !UNSAFE_IS_ZERO(Pike_sp-1);
    Pike_sp--;
  }

  a = load_xbm( data );

  if(!fg)
  {
    if(invert)
    {
      apply(a, "invert", 0);
      i = (struct object *)debug_malloc_pass(Pike_sp[-1].u.object);
      Pike_sp--;
    }
    else
    {
      i = a;
      add_ref(a);
    }
  } else {
    if(!bg)
    {
      push_int(255);
      push_int(255);
      push_int(255);
      f_aggregate(3);
      bg = (struct array *)debug_malloc_pass(Pike_sp[-1].u.array);
      Pike_sp--;
    }
    if(invert)
    {
      struct array *tmp = fg;
      fg = bg;
      bg = fg;
    }
    apply(a, "xsize", 0);
    apply(a, "ysize", 0);
    push_int( bg->item[0].u.integer );
    push_int( bg->item[1].u.integer );
    push_int( bg->item[2].u.integer );
    i = clone_object( image_program, 5 );
    ref_push_object( i );
    push_int( fg->item[0].u.integer );
    push_int( fg->item[1].u.integer );
    push_int( fg->item[2].u.integer );

    apply( i, "paste_alpha_color", 4 );
  }
  
  pop_n_elems(args);
  push_constant_text( "alpha" );
  push_object( a );
    push_constant_text( "image" );
  if(i)
    push_object( i );
  else
    push_int( 0 );
  f_aggregate_mapping(4);
}




/*
**! method string encode(object image)
**! method string encode(object image, mapping options)
**! 	Encodes a XBM image. 
**!
**!     The <tt>options</tt> argument may be a mapping
**!	containing zero or more encoding options.
**!
**!	<pre>
**!	normal options:
**!	    "name":"xbm_image_name"
**!		The name of the XBM. Defaults to 'image'
**!	</pre>
*/

static struct pike_string *param_name;
void image_xbm_encode( INT32 args )
{
  struct image *img = NULL;
  struct pike_string *name = NULL, *buf;
  if (!args)
    Pike_error("Image.XBM.encode: too few arguments\n");
   
  if (Pike_sp[-args].type!=PIKE_T_OBJECT ||
      !(img=(struct image*)
        get_storage(Pike_sp[-args].u.object,image_program)))
    Pike_error("Image.XBM.encode: illegal argument 1\n");
   
  if (!img->img)
    Pike_error("Image.XBM.encode: no image\n");

  if (args>1)
  {
    if (Pike_sp[1-args].type!=PIKE_T_MAPPING)
      Pike_error("Image.XBM.encode: illegal argument 2\n");
      
    push_svalue(Pike_sp+1-args);
    ref_push_string(param_name); 
    f_index(2);
    if(Pike_sp[-1].type == PIKE_T_STRING)
    {
      if(Pike_sp[-1].u.string->size_shift)
        Pike_error("The name of the image must be a normal non-wide string (sorry, not my fault)\n");
      name = Pike_sp[-1].u.string;
    }
    pop_stack();
  }

  buf = save_xbm( img, name );
  pop_n_elems(args);
  push_string( buf );
}


static struct program *image_encoding_xbm_program=NULL;
void init_image_xbm(void)
{
  ADD_FUNCTION( "_decode", image_xbm__decode,
		tFunc(tStr tOr(tVoid,tMapping),tMap(tStr,tObj)), 0);
  ADD_FUNCTION( "decode", image_xbm_decode, tFunc(tStr,tObj), 0);
  ADD_FUNCTION( "encode", image_xbm_encode,
		tFunc(tObj tOr(tVoid,tMapping),tStr), 0);
  param_name=make_shared_string("name");
  param_fg=make_shared_string("fg");
  param_bg=make_shared_string("bg");
  param_invert=make_shared_string("invert");
}

void exit_image_xbm(void)
{
   free_string(param_name);
   free_string(param_fg);
   free_string(param_bg);
   free_string(param_invert);
}
