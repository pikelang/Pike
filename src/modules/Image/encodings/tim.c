/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: tim.c,v 1.18 2004/10/07 22:49:57 nilsson Exp $
*/

#include "global.h"
#include "image_machine.h"
#include <math.h>
#include <ctype.h>

#include "object.h"
#include "interpret.h"
#include "svalue.h"
#include "mapping.h"
#include "pike_error.h"
#include "operators.h"
#include "builtin_functions.h"
#include "module_support.h"


#include "image.h"

#include "encodings.h"



extern struct program *image_program;

/*
**! module Image
**! submodule TIM
**!
**! 	Handle decoding of TIM images.
**! 
**! 	TIM is the framebuffer format of the PSX game system.
**! 	It is a simple, uncompressed, truecolor or CLUT format
**!     with a one bit alpha channel.
*/

/*
**! method object decode(string data)
**! method object decode_alpha(string data)
**! method mapping decode_header(string data)
**! method mapping _decode(string data)
**!	decodes a TIM image
**!
**!	The <ref>decode_header</ref> and <ref>_decode</ref>
**!	has these elements:
**!
**!	<pre>
**!        "image":object            - image object    \- not decode_header
**!	   "alpha":object            - decoded alpha   /
**!	   
**!	   "type":"image/x-tim"      - image type
**!	   "xsize":int               - horisontal size in pixels
**!	   "ysize":int               - vertical size in pixels
**!	   "attr":int		     - texture attributes
**!	</pre>
*/

#define MODE_CLUT4              0
#define MODE_CLUT8              1
#define MODE_DC15               2
#define MODE_DC24               3
#define MODE_MIXED              4
#define FLAG_CLUT               8


static void tim_decode_rect(INT32 attr, unsigned char *src, rgb_group *dst,
			    unsigned char *clut, unsigned int h,
			    unsigned int w)
{
  INT32 cnt = h * w;
  switch(attr&7) {
   case MODE_DC15:
     while(cnt--) {
       unsigned int p = src[0]|(src[1]<<8);
       dst->b = ((p&0x7c00)>>7)|((p&0x7000)>>12);
       dst->g = ((p&0x03e0)>>2)|((p&0x0380)>>7);
       dst->r = ((p&0x001f)<<3)|((p&0x001c)>>2);
       src+=2;
       dst++;
     }
     break;
   case MODE_CLUT4:
     cnt = cnt/2;
     while(cnt--) {
       int i, cluti = (src[0]&0xf)*2;
       unsigned int p;

       for(i=0; i<2; i++)
       {
	 p = clut[cluti]|(clut[cluti+1]<<8);
         dst->b = ((p&0x7c00)>>7)|((p&0x7000)>>12);
         dst->g = ((p&0x03e0)>>2)|((p&0x0380)>>7);
         dst->r = ((p&0x001f)<<3)|((p&0x001c)>>2);
         dst++;
         cluti = (src[0]>>4)*2;
       }
       src++;
     }
     break;
   case MODE_CLUT8:
     while(cnt--) {
       int cluti = (src[0])*2;
       unsigned int p = clut[cluti]|(clut[cluti+1]<<8);

       dst->b = ((p&0x7c00)>>7)|((p&0x7000)>>12);
       dst->g = ((p&0x03e0)>>2)|((p&0x0380)>>7);
       dst->r = ((p&0x001f)<<3)|((p&0x001c)>>2);
       dst++;
       src++;
     }
     break;
  }
}

#define ALPHA(a) if(!a)               /* Transparent */           \
	           dst->b = dst->g = dst->r = 0;                  \
                 else if(!(a&0x80))  /* Not transparent */        \
	           dst->b = dst->g = dst->r = 0xff;               \
                 else if(!(a&0x7f))  /* Non-transparent black */  \
	           dst->b = dst->g = dst->r = 0xff;               \
                 else                /* Semi transparent */       \
	           dst->b = dst->g = dst->r = 0x7f


static void tim_decode_alpha_rect(INT32 attr, unsigned char *src,
				  rgb_group *dst, unsigned char *clut, 
				  unsigned int h, unsigned int w)
{
  /* Pixels rendereding on the PSX is made in one of two modes. One of
     them has semi transparency, but what mode to use is not indicated
     in the TIM as far as I know. Let's render everything in semi
     transparent mode with .5 from source and .5 from destination. */

  INT32 cnt = h * w;
  switch(attr&7) {
   case MODE_DC15:
     while(cnt--) {
       ALPHA(src[1]);
       src+=2;
       dst++;
     }
     break;
   case MODE_CLUT4:
     cnt = cnt/2;
     while(cnt--) {
       ALPHA( clut[(src[0]&0xf)*2] );
       dst++;
       ALPHA( clut[(src[0]>>4)*2] );
       src++;
       dst++;
     }
     break;
   case MODE_CLUT8:
     while(cnt--) {
       ALPHA( clut[(src[0])*2] );
       src++;
       dst++;
     }
     break;
  }
}

void img_tim_decode(INT32 args, int header_only)
{
  struct pike_string *str;
  unsigned char *s, *clut;
  int n=0, hasalpha=0, bitpp=0, bsize=0;
  ptrdiff_t len;
  INT32 attr;
  unsigned int h=0, w=0;
  
  get_all_args("Image.TIM._decode", args, "%S", &str);
  clut=s=(unsigned char *)str->str;
  clut+=20;
  len = str->len;
  pop_n_elems(args-1);
  
  if(len < 12 || (s[0] != 0x10 || s[2] != 0 || s[3] != 0))
    Pike_error("not a TIM texture\n");
  else if(s[2] != 0)
    Pike_error("unknown version of TIM texture\n");     

  s += 4; len -= 4;
  
  push_text("type");
  push_text("image/x-tim");
  n++;
  
  attr = s[0]|(s[1]<<8)|(s[2]<<16)|(s[3]<<24);
  if(attr&0xfffffff0)
    Pike_error("unknown flags in TIM texture\n");
  
  s += 4; len -= 4;

  push_text("attr");
  push_int(attr);
  n++;
  
  if(attr&FLAG_CLUT) {
    bsize = s[0]|(s[1]<<8)|(s[2]<<16)|(s[3]<<24);   
#ifdef TIM_DEBUG
    printf("bsize: %d\n", bsize);
#endif
    s += bsize; len -= bsize;
  }

  /* FIXME: Unknown what this comes from */
  s += 4; len -= 4;

  switch(attr&7) {
   case MODE_DC15:
#ifdef TIM_DEBUG
     printf("15bit\n");
     printf("dx: %d, dy: %d\n", s[0]|(s[1]<<8), s[2]|(s[3]<<8));
#endif
     s += 4; len -= 4;
     w = s[0]|(s[1]<<8);
     h = s[2]|(s[3]<<8);
     s += 4; len -= 4;
     bitpp = 16;
     hasalpha = 1;
     break;
   case MODE_DC24:
#ifdef TIM_DEBUG
     printf("24bit\n");
#endif
     Pike_error("24bit TIMs not supported. Please send an example to peter@roxen.com\n");
   case MODE_CLUT4:
     /* dx and dy word ignored */
#ifdef TIM_DEBUG
     printf("CLUT4\n");
     printf("dx: %d, dy: %d\n", s[0]|(s[1]<<8), s[2]|(s[3]<<8));
#endif
     s += 4; len -= 4;
     w = (s[0]|(s[1]<<8))*4;
     h = s[2]|(s[3]<<8);
     s += 4; len -= 4;
     bitpp = 4;
     hasalpha = 1;
     break;    
   case MODE_CLUT8:
     /* dx and dy word ignored */
#ifdef TIM_DEBUG
     printf("CLUT8\n");
     printf("dx: %d, dy: %d\n", s[0]|(s[1]<<8), s[2]|(s[3]<<8));
#endif
     s += 4; len -= 4;
     w = (s[0]|(s[1]<<8))*2;
     h = s[2]|(s[3]<<8);
     s += 4; len -= 4;
     bitpp = 8;
     hasalpha = 1;
     break;
   case MODE_MIXED:
#ifdef TIM_DEBUG
     printf("Mixed\n");
#endif
     Pike_error("mixed TIMs not supported\n");
   default:
     Pike_error("unknown TIM format\n");
  }
  
  push_text("xsize");
  push_int(w);
  n++;   
  push_text("ysize");
  push_int(h);
  n++;   

#ifdef TIM_DEBUG
  printf("w: %d, h: %d\n", w, h);
#endif  

  if(!header_only) {
    struct object *o;
    struct image *img;
    
    if(len < (INT32)(bitpp*(h*w)/8))
      Pike_error("short pixel data\n");
    
    push_text("image");
    push_int(w);
    push_int(h);
    o=clone_object(image_program,2);
    img=(struct image*)get_storage(o,image_program);
    push_object(o);
    n++;
    
    tim_decode_rect(attr, s, img->img, clut, h, w);
    
    if(hasalpha) {
      push_text("alpha");
      push_int(w);
      push_int(h);
      o=clone_object(image_program,2);
      img=(struct image*)get_storage(o,image_program);
      push_object(o);
      n++;
      
      tim_decode_alpha_rect(attr, s, img->img, clut, h, w);
    }
  }
  
  f_aggregate_mapping(2*n);
  
  stack_swap();
  pop_stack();
}

static void image_tim_f_decode(INT32 args)
{
   img_tim_decode(args,0);
   push_constant_text("image");
   f_index(2);
}

static void image_tim_f_decode_alpha(INT32 args)
{
   img_tim_decode(args,0);
   push_constant_text("alpha");
   f_index(2);
}

void image_tim_f_decode_header(INT32 args)
{
   img_tim_decode(args,1);
}

void image_tim_f__decode(INT32 args)
{
   img_tim_decode(args,0);
}

void init_image_tim()
{
  ADD_FUNCTION( "decode",  image_tim_f_decode,  tFunc(tStr,tObj), 0);
  ADD_FUNCTION( "decode_alpha",  image_tim_f_decode_alpha,  tFunc(tStr,tObj), 0);
  ADD_FUNCTION( "_decode", image_tim_f__decode, tFunc(tStr,tMapping), 0);
  ADD_FUNCTION( "decode_header", image_tim_f_decode_header, tFunc(tStr,tMapping), 0);
}

void exit_image_tim()
{
}
