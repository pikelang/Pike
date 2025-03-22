/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
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
#include "pike_types.h"
#include "bignum.h"


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

/* See also "The TIM file format explained":
 *   https://www.psxdev.net/forum/viewtopic.php?t=109
 * and "Parsing PSX TIM Images with Typescript":
 *   https://jackrobinson.co.nz/blog/tim-image-parsing/
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
			    unsigned char *clut, int clutlength,
                            unsigned int h, unsigned int w)
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
   case MODE_DC24:
     {
       INT32 remaining = 0;
       /* Convert width to scanline bytes including pad. */
       w = w*3 + (w & 1);
       while(cnt--) {
         if (remaining < 3) {
           /* Skip the end of scanline pad byte. */
           src += remaining;
           remaining = w;
         }
         dst->r = *src++;
         dst->g = *src++;
         dst->b = *src++;
         dst++;
         remaining -= 3;
       }
     }
     break;
   case MODE_CLUT4:
     cnt = cnt/2;
     while(cnt--) {
       int i, cluti = (src[0]&0xf)*2;

       if (clutlength <= cluti + 1)
         Pike_error("Malformed TIM image.\n");

       for(i=0; i<2; i++)
       {
         unsigned int p = clut[cluti]|(clut[cluti+1]<<8);

         dst->b = ((p&0x7c00)>>7)|((p&0x7000)>>12);
         dst->g = ((p&0x03e0)>>2)|((p&0x0380)>>7);
         dst->r = ((p&0x001f)<<3)|((p&0x001c)>>2);
         dst++;
         cluti = (src[0]>>4)*2;

         if (clutlength <= cluti + 1)
           Pike_error("Malformed TIM image.\n");
       }
       src++;
     }
     break;
   case MODE_CLUT8:
     while(cnt--) {
       int cluti = (src[0])*2;
       unsigned int p;

       if (clutlength <= cluti + 1)
         Pike_error("Malformed TIM image.\n");

       p = clut[cluti]|(clut[cluti+1]<<8);

       dst->b = ((p&0x7c00)>>7)|((p&0x7000)>>12);
       dst->g = ((p&0x03e0)>>2)|((p&0x0380)>>7);
       dst->r = ((p&0x001f)<<3)|((p&0x001c)>>2);
       dst++;
       src++;
     }
     break;
  }
}

/* NB: a is the high byte of the 16bit color below. */
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

  get_all_args(NULL, args, "%n", &str);
  clut=s=(unsigned char *)str->str;
  clut+=20;
  len = str->len;
  pop_n_elems(args-1);

  if(len < 12 || (s[0] != 0x10 || s[2] != 0 || s[3] != 0))
    Pike_error("not a TIM texture\n");
  else if(s[1] != 0)
    Pike_error("unknown version of TIM texture\n");

  s += 4; len -= 4;

  ref_push_string(literal_type_string);
  push_static_text("image/x-tim");
  n++;

  attr = s[0]|(s[1]<<8)|(s[2]<<16)|(s[3]<<24);
  if(attr&0xfffffff0)
    Pike_error("unknown flags in TIM texture\n");

  s += 4; len -= 4;

  push_static_text("attr");
  push_int(attr);
  n++;

  if(attr&FLAG_CLUT) {
    bsize = s[0]|(s[1]<<8)|(s[2]<<16)|(s[3]<<24);
    if (bsize > len || bsize < 0)
      Pike_error("Malformed TIM.\n");
#ifdef TIM_DEBUG
    fprintf(stderr, "bsize: %d\n", bsize);
#endif
    s += bsize; len -= bsize;
  }

  /* FIXME: Unknown what this comes from */
  s += 4; len -= 4;

  if (len < 8)
    Pike_error("Malformed TIM.\n");

  switch(attr&7) {
   case MODE_DC15:
#ifdef TIM_DEBUG
     fprintf(stderr, "15bit\n");
     fprintf(stderr, "dx: %d, dy: %d\n", s[0]|(s[1]<<8), s[2]|(s[3]<<8));
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
     fprintf(stderr, "24bit\n");
#endif
     s += 4; len -= 4;
     w = s[0]|(s[1]<<8);
     h = s[2]|(s[3]<<8);
     s += 4; len -= 4;
     bitpp = 24;
     hasalpha = 0;
     break;
   case MODE_CLUT4:
     /* dx and dy word ignored */
#ifdef TIM_DEBUG
     fprintf(stderr, "CLUT4\n");
     fprintf(stderr, "dx: %d, dy: %d\n", s[0]|(s[1]<<8), s[2]|(s[3]<<8));
#endif
     if (!(attr&FLAG_CLUT))
       Pike_error("Malformed TIM image (CLUT mode but no CLUT bit)\n");

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
     fprintf(stderr, "CLUT8\n");
     fprintf(stderr, "dx: %d, dy: %d\n", s[0]|(s[1]<<8), s[2]|(s[3]<<8));
#endif
     if (!(attr&FLAG_CLUT))
       Pike_error("Malformed TIM image (CLUT mode but no CLUT bit)\n");
     s += 4; len -= 4;
     w = (s[0]|(s[1]<<8))*2;
     h = s[2]|(s[3]<<8);
     s += 4; len -= 4;
     bitpp = 8;
     hasalpha = 1;
     break;
   case MODE_MIXED:
#ifdef TIM_DEBUG
     fprintf(stderr, "Mixed\n");
#endif
     Pike_error("mixed TIMs not supported\n");
   default:
     Pike_error("unknown TIM format\n");
  }

  push_static_text("xsize");
  push_int(w);
  n++;
  push_static_text("ysize");
  push_int(h);
  n++;

#ifdef TIM_DEBUG
  fprintf(stderr, "w: %d, h: %d\n", w, h);
#endif

  if(!header_only) {
    struct object *o;
    struct image *img;
    INT32 bytes_needed = 0;

    if ((INT32)h < 0 || (INT32)w < 0
        || DO_INT32_MUL_OVERFLOW(h, w, &bytes_needed)
        || DO_INT32_MUL_OVERFLOW(bytes_needed, bitpp, &bytes_needed))
    {
      Pike_error("TIM Image too large.\n");
    }

    bytes_needed >>= 3;	/* Adjust unit from bits to bytes. */

    if(len < bytes_needed)
      Pike_error("short pixel data (%ld < %d)\n", (long)len, bytes_needed);

    push_static_text("image");
    push_int(w);
    push_int(h);
    o=clone_object(image_program,2);
    img=get_storage(o,image_program);
    push_object(o);
    n++;

    tim_decode_rect(attr, s, img->img, clut, bsize, h, w);

    if(hasalpha) {
      push_static_text("alpha");
      push_int(w);
      push_int(h);
      o=clone_object(image_program,2);
      img=get_storage(o,image_program);
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

static void image_tim_f_encode(INT32 args)
{
  struct image *img = NULL;
  size_t x, y;
  rgb_group *s;
  struct pike_string *res = NULL;
  p_wchar0 *pos;
  int encoding = MODE_DC15;
  size_t image_size = 0;

  if (!args) {
     SIMPLE_WRONG_NUM_ARGS_ERROR("encode", 1);
  }

  if (TYPEOF(Pike_sp[-args]) != PIKE_T_OBJECT ||
      !(img = get_storage(Pike_sp[-args].u.object, image_program))) {
    SIMPLE_ARG_TYPE_ERROR("encode", 1, "Image.Image");
  }

  if (!img->img) {
    PIKE_ERROR("encode", "No image.\n", Pike_sp, args);
  }

  if ((img->xsize & ~0xffff) || (img->ysize & ~0xffff)) {
    PIKE_ERROR("encode", "Invalid dimensions.\n", Pike_sp, args);
  }

  if (args>1) {
    if ((TYPEOF(Pike_sp[1-args]) != PIKE_T_INT) ||
        ((Pike_sp[1-args].u.integer != MODE_DC15) &&
         (Pike_sp[1-args].u.integer != MODE_DC24))) {
      SIMPLE_ARG_TYPE_ERROR("encode", 2, "int(2..3)");
    }
    encoding = Pike_sp[1-args].u.integer;
  }

  if (encoding == MODE_DC24) {
    /* 12 bytes of header.
     * 3 bytes per pixel
     * 1 byte padding per line if the width is odd.
     */
    image_size = (img->xsize * 3 + (img->xsize & 1)) * img->ysize + 12;
  } else {
    /* MODE_DC15
     *
     * 12 bytes of header.
     * 2 bytes per pixel
     */
    image_size = img->xsize * 2 * img->ysize + 12;
  }

  res = begin_shared_string(image_size + 16);
  pos = STR0(res);

  *pos++ = 0x10;
  *pos++ = 0x0;
  *pos++ = 0x0;
  *pos++ = 0x0;
  *pos++ = encoding;
  *pos++ = 0x0;
  *pos++ = 0x0;
  *pos++ = 0x0;

  /* No CLUT for DC15 and DC24. */

  /* Number of bytes of image data including header (32bit). */
  *pos++ = image_size & 0xff;
  *pos++ = (image_size>>8) & 0xff;
  *pos++ = (image_size>>16) & 0xff;
  *pos++ = (image_size>>24) & 0xff;

  /* Texture position x, y (16 + 16 bit). */
  *pos++ = 0x00;
  *pos++ = 0x00;
  *pos++ = 0x00;
  *pos++ = 0x00;

  /* Texture width (16bit). */
  x = (size_t)img->xsize;
  *pos++ = x & 0xff;
  *pos++ = (x>>8) & 0xff;

  /* Texture height (16bit). */
  y = (size_t)img->ysize;
  *pos++ = y & 0xff;
  *pos++ = (y>>8) & 0xff;

  s = img->img;
  if (encoding == MODE_DC24) {
    /* Direct color 24bit. */
    for (y = (size_t)img->ysize; y--;) {
      for (x = (size_t)img->xsize; x--;) {
        *pos++ = s->r;
        *pos++ = s->g;
        *pos++ = s->b;
        s++;
      }
      if (img->xsize & 1) {
        *pos++ = 0x00;	/* Pad to 16bit. */
      }
    }
  } else {
    /* Direct color 15+1bit. */
    for (y = (size_t)img->ysize; y--;) {
      for (x = (size_t)img->xsize; x--;) {
        unsigned char r = s->r>>3;
        unsigned char g = s->g>>3;
        unsigned char b = s->b>>3;
        unsigned int c = r | (g << 5) | (b << 10);
        if (!c) {
          /* Transparent -- Set STP to get black. */
          c |= 0x8000;
        }
        *pos++ = c & 0xff;
        *pos++ = (c>>8) & 0xff;
        s++;
      }
    }
  }

  push_string(end_shared_string(res));
}


void init_image_tim(void)
{
  ADD_FUNCTION( "decode",  image_tim_f_decode,  tFunc(tStr,tObj), 0);
  ADD_FUNCTION( "decode_alpha",  image_tim_f_decode_alpha,  tFunc(tStr,tObj), 0);
  ADD_FUNCTION( "_decode", image_tim_f__decode, tFunc(tStr,tMapping), 0);
  ADD_FUNCTION( "decode_header", image_tim_f_decode_header, tFunc(tStr,tMapping), 0);

  ADD_FUNCTION( "encode",  image_tim_f_encode,
                tFunc(tObj tOr3(tInt2, tInt3, tVoid), tStr), 0);
}

void exit_image_tim(void)
{
}
