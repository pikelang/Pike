#include "global.h"
#include "image_machine.h"
#include <math.h>
#include <ctype.h>

#include "stralloc.h"
RCSID("$Id: pvr.c,v 1.1 2000/02/24 01:11:23 marcus Exp $");
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "threads.h"
#include "array.h"
#include "error.h"
#include "operators.h"
#include "builtin_functions.h"
#include "module_support.h"


#include "image.h"

extern struct program *image_program;

/*
**! module Image
**! submodule PVR
**!
**! 	Handle encoding and decoding of PVR images.
**! 
**! 	PVR is the texture format of the NEC PowerVR system
**! 	It is a rather simple, uncompressed, truecolor
**!     format. 
*/

/*
**! method object decode(string data)
**! method object decode_alpha(string data)
**! method mapping decode_header(string data)
**! method mapping _decode(string data)
**!	decodes a PVR image
**!
**!	The <ref>decode_header</ref> and <ref>_decode</ref>
**!	has these elements:
**!
**!	<pre>
**!        "image":object            - image object    \- not decode_header
**!	   "alpha":object            - decoded alpha   /
**!	   
**!	   "type":"image/x-pvr"      - image type
**!	   "xsize":int               - horisontal size in pixels
**!	   "ysize":int               - vertical size in pixels
**!	   "attr":int		     - texture attributes
**!	   "global_index":int	     - global index (if present)
**!	</pre>
**!	  
**!
**! method string encode(object image)
**! method string encode(object image,mapping options)
**!	encode a PVR image
**!
**!	options is a mapping with optional values:
**!	<pre>
**!	   "alpha":object            - alpha channel
**!	</pre>
*/

#define MODE_ARGB1555 0x00
#define MODE_RGB565   0x01
#define MODE_ARGB4444 0x02
#define MODE_YUV422   0x03
#define MODE_BUMPMAP  0x04
#define MODE_RGB555   0x05
#define MODE_ARGB8888 0x06

#define MODE_TWIDDLE            0x0100
#define MODE_TWIDDLE_MIPMAP     0x0200
#define MODE_COMPRESSED         0x0300
#define MODE_COMPRESSED_MIPMAP  0x0400
#define MODE_CLUT4              0x0500
#define MODE_CLUT4_MIPMAP       0x0600
#define MODE_CLUT8              0x0700
#define MODE_CLUT8_MIPMAP       0x0800
#define MODE_RECTANGLE          0x0900
#define MODE_STRIDE             0x0b00
#define MODE_TWIDDLED_RECTANGLE 0x0d00

static INT32 twiddletab[1024];
static int twiddleinited=0;

static void init_twiddletab()
{
  int x;
  for(x=0; x<1024; x++)
    twiddletab[x] = (x&1)|((x&2)<<1)|((x&4)<<2)|((x&8)<<3)|((x&16)<<4)|
      ((x&32)<<5)|((x&64)<<6)|((x&128)<<7)|((x&256)<<8)|((x&512)<<9);
  twiddleinited=1;
}

void image_pvr_f_encode(INT32 args)
{
   error("not implemented\n");
}

void img_pvr_decode(INT32 args,int header_only)
{
   struct pike_string *str;
   unsigned char *s;
   int n = 0;
   INT32 len, attr;
   unsigned int h, w, x, y;

   get_all_args("Image.PVR._decode", args, "%S", &str);
   s=(unsigned char *)str->str;
   len=str->len;
   pop_n_elems(args-1);

   if(len >= 12 && !strncmp(s, "GBIX", 4) &&
      s[4]==4 && s[5]==0 && s[6]==0 && s[7]==0) {
     push_text("global_index");
     push_int(s[8]|(s[9]<<8)|(s[10]<<16)|(s[11]<<24));
     n++;
     len -= 12;
     s += 12;
   }

   if(len < 16 || strncmp(s, "PVRT", 4))
     error("not a PVR texture\n");
   else {
     INT32 l = s[4]|(s[5]<<8)|(s[6]<<16)|(s[7]<<24);
     if(l+8>len)
       error("file is truncated\n");
     else if(l<8)
       error("invalid PVRT chunk length\n");
     len = l+8;
   }

   push_text("type");
   push_text("image/x-pvr");
   n++;

   attr = s[8]|(s[9]<<8)|(s[10]<<16)|(s[11]<<24);
   w = s[12]|(s[13]<<8);
   h = s[14]|(s[15]<<8);

   s += 16;
   len -= 16;

   push_text("attr");
   push_int(attr);
   n++;
   push_text("xsize");
   push_int(w);
   n++;   
   push_text("ysize");
   push_int(h);
   n++;   

   if(!header_only) {
     int twiddle=0, hasalpha=0, bpp=0;
     struct object *o;
     struct image *img;
     unsigned char *src = s;
     rgb_group *dst;
     INT32 cnt;

     switch(attr&0xff00) {
      case MODE_TWIDDLE:
      case MODE_TWIDDLE_MIPMAP:
	twiddle = 1;
	if(w != h || w<8 || w>1024 || (w&(w-1)))
	  error("invalid size for twiddle texture\n");
      case MODE_RECTANGLE:
      case MODE_STRIDE:
	break;
      case MODE_TWIDDLED_RECTANGLE:
	error("twiddled rectangle not supported\n");
      case MODE_COMPRESSED:
      case MODE_COMPRESSED_MIPMAP:
	error("compressed PVRs not supported\n");
      case MODE_CLUT4:
      case MODE_CLUT4_MIPMAP:
      case MODE_CLUT8:
      case MODE_CLUT8_MIPMAP:
	error("palette PVRs not supported\n");
      default:
	error("unknown PVR format\n");
     }

     switch(attr&0xff) {
      case MODE_ARGB1555:
      case MODE_ARGB4444:
	hasalpha=1;
      case MODE_RGB565:
      case MODE_RGB555:
	bpp=2; break;
      case MODE_YUV422:
	error("YUV mode not supported\n");
      case MODE_ARGB8888:
	error("ARGB8888 mode not supported\n");
      case MODE_BUMPMAP:
	error("bumpmap mode not supported\n");
      default:
	error("unknown PVR color mode\n");
     }

     if(len < bpp*(cnt = h*w))
       error("short PVRT chunk\n");

     push_text("image");
     push_int(w);
     push_int(h);
     o=clone_object(image_program,2);
     img=(struct image*)get_storage(o,image_program);
     dst=img->img;
     push_object(o);
     n++;

     if(twiddle && !twiddleinited)
       init_twiddletab();
     
     if(twiddle)
       switch(attr&0xff) {
	case MODE_ARGB1555:
	case MODE_RGB555:
	  for(y=0; y<h; y++)
	    for(x=0; x<w; x++) {
	      unsigned int p;
	      src = s+(((twiddletab[x]<<1)|twiddletab[y])<<1);
	      p = src[0]|(src[1]<<8);
	      dst->r = ((p&0x7c00)>>7)|((p&0x7000)>>12);
	      dst->g = ((p&0x03e0)>>2)|((p&0x0380)>>7);
	      dst->b = ((p&0x001f)<<3)|((p&0x001c)>>2);
	      src+=2;
	      dst++;
	    }
	  break;
	case MODE_RGB565:
	  for(y=0; y<h; y++)
	    for(x=0; x<w; x++) {
	      unsigned int p;
	      src = s+(((twiddletab[x]<<1)|twiddletab[y])<<1);
	      p = src[0]|(src[1]<<8);
	      dst->r = ((p&0xf800)>>8)|((p&0xe000)>>13);
	      dst->g = ((p&0x07e0)>>3)|((p&0x0600)>>9);
	      dst->b = ((p&0x001f)<<3)|((p&0x001c)>>2);
	      src+=2;
	      dst++;
	    }
	  break;
	case MODE_ARGB4444:
	  for(y=0; y<h; y++)
	    for(x=0; x<w; x++) {
	      unsigned int p;
	      src = s+(((twiddletab[x]<<1)|twiddletab[y])<<1);
	      p = src[0]|(src[1]<<8);
	      dst->r = ((p&0x0f00)>>4)|((p&0x0f00)>>8);
	      dst->g = (p&0x00f0)|((p&0x00f0)>>4);
	      dst->b = ((p&0x000f)<<4)|(p&0x000f);
	      src+=2;
	      dst++;
	    }
	  break;
       }
     else
       switch(attr&0xff) {
	case MODE_ARGB1555:
	case MODE_RGB555:
	  while(cnt--) {
	    unsigned int p = src[0]|(src[1]<<8);
	    dst->r = ((p&0x7c00)>>7)|((p&0x7000)>>12);
	    dst->g = ((p&0x03e0)>>2)|((p&0x0380)>>7);
	    dst->b = ((p&0x001f)<<3)|((p&0x001c)>>2);
	    src+=2;
	    dst++;
	  }
	  break;
	case MODE_RGB565:
	  while(cnt--) {
	    unsigned int p = src[0]|(src[1]<<8);
	    dst->r = ((p&0xf800)>>8)|((p&0xe000)>>13);
	    dst->g = ((p&0x07e0)>>3)|((p&0x0600)>>9);
	    dst->b = ((p&0x001f)<<3)|((p&0x001c)>>2);
	    src+=2;
	    dst++;
	  }
	  break;
	case MODE_ARGB4444:
	  while(cnt--) {
	    unsigned int p = src[0]|(src[1]<<8);
	    dst->r = ((p&0x0f00)>>4)|((p&0x0f00)>>8);
	    dst->g = (p&0x00f0)|((p&0x00f0)>>4);
	    dst->b = ((p&0x000f)<<4)|(p&0x000f);
	    src+=2;
	    dst++;
	  }
	  break;
       }
     
   }

   f_aggregate_mapping(2*n);

#ifdef PVR_DEBUG
   print_svalue(stderr, sp-1);
#endif

   stack_swap();
   pop_stack();
}

static void image_pvr_f_decode(INT32 args)
{
   img_pvr_decode(args,0);
   push_string(make_shared_string("image"));
   f_index(2);
}

static void image_pvr_f_decode_alpha(INT32 args)
{
   img_pvr_decode(args,0);
   push_string(make_shared_string("alpha"));
   f_index(2);
}

void image_pvr_f_decode_header(INT32 args)
{
   img_pvr_decode(args,1);
}

void image_pvr_f__decode(INT32 args)
{
   img_pvr_decode(args,0);
}

void init_image_pvr()
{
  ADD_FUNCTION( "decode",  image_pvr_f_decode,  tFunc(tStr,tObj), 0);
  ADD_FUNCTION( "decode_alpha",  image_pvr_f_decode_alpha,  tFunc(tStr,tObj), 0);
  ADD_FUNCTION( "_decode", image_pvr_f__decode, tFunc(tStr,tMapping), 0);
  ADD_FUNCTION( "encode",  image_pvr_f_encode,  tFunc(tStr,tObj), 0);
  ADD_FUNCTION( "decode_header", image_pvr_f_decode_header, tFunc(tStr,tMapping), 0);
}

void exit_image_pvr()
{
}
