#include "global.h"
#include "image_machine.h"
#include <math.h>
#include <ctype.h>

#include "stralloc.h"
RCSID("$Id: tim.c,v 1.1 2000/03/21 06:13:23 peter Exp $");
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "threads.h"
#include "array.h"
#include "mapping.h"
#include "error.h"
#include "operators.h"
#include "stralloc.h"
#include "builtin_functions.h"
#include "module_support.h"


#include "image.h"

extern struct program *image_program;

/*
**! module Image
**! submodule TIM
**!
**! 	Handle decoding of TIM images.
**! 
**! 	TIM is the framebuffer format of the PSX game system.
**! 	It is a simple, uncompressed, truecolor or CLUT format. 
*/

//FIXME: Add to ANY.
//FIXME: header_only doesn't get the width and height.

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
			    INT32 stride, unsigned int h, unsigned int w)
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
  }
}

static void pvr_decode_alpha_rect(INT32 attr, unsigned char *src,
				  rgb_group *dst, INT32 stride,
				  unsigned int h, unsigned int w)
{
  INT32 cnt = h * w;
  switch(attr&7) {
   case MODE_DC15:
     while(cnt--) {
       if(src[1]&0x80)
	 dst->b = dst->g = dst->r = ~0;
       else
	 dst->b = dst->g = dst->r = 0;
       src+=2;
       dst++;
     }
     break;
  }
}

void img_tim_decode(INT32 args, int header_only)
{
   struct pike_string *str;
   unsigned char *s;
   int n = 0;
   INT32 len, attr;
   unsigned int h, w, x;

   get_all_args("Image.TIM._decode", args, "%S", &str);
   s=(unsigned char *)str->str;
   len=str->len;
   pop_n_elems(args-1);

   if(len < 12 || (s[0] != 0x10 || s[2] != 0 || s[3] != 0))
     error("not a TIM texture\n");
   else if(s[2] != 0)
     error("unknown version of TIM texture\n");     

   push_text("type");
   push_text("image/x-tim");
   n++;

   attr = s[4]|(s[5]<<8)|(s[6]<<16)|(s[7]<<24);

   push_text("attr");
   push_int(attr);
   n++;

   if(!header_only) {
     int hasalpha=0, bpp=0;
     struct object *o;
     struct image *img;
     INT32 clut=0;

     if(attr&FLAG_CLUT)
       error("TIM with CLUT not supported\n");

     /* note: The size part of the CLUT is still present, so it will
      * take one word. */

     switch(attr&7) {
      case MODE_DC15:
	//dx and dy word ignored
	w = s[16]|(s[17]<<8);
	h = s[18]|(s[19]<<8);
	bpp = 2;
	hasalpha = 1;
	break;
      case MODE_DC24:
	error("24bit TIMs not supported\n");
      case MODE_CLUT4:
      case MODE_CLUT8:
   	error("palette TIMs not supported\n");
      case MODE_MIXED:
   	error("mixed TIMs not supported\n");
      default:
	error("unknown TIM format\n");
     }

     s += 20;
     len -= 20;

     push_text("xsize");
     push_int(w);
     n++;   
     push_text("ysize");
     push_int(h);
     n++;   
     printf("w: %d, h: %d\n", w, h);

     if(len < (INT32)(bpp*(h*w)))
       error("short pixel data\n");

     push_text("image");
     push_int(w);
     push_int(h);
     o=clone_object(image_program,2);
     img=(struct image*)get_storage(o,image_program);
     push_object(o);
     n++;
     
     tim_decode_rect(attr, s, img->img, 0, h, w);

     if(hasalpha) {
       push_text("alpha");
       push_int(w);
       push_int(h);
       o=clone_object(image_program,2);
       img=(struct image*)get_storage(o,image_program);
       push_object(o);
       n++;
       
       pvr_decode_alpha_rect(attr, s, img->img, 0, h, w);
     }
   }

   f_aggregate_mapping(2*n);

   stack_swap();
   pop_stack();
}

static void image_tim_f_decode(INT32 args)
{
   img_tim_decode(args,0);
   push_string(make_shared_string("image"));
   f_index(2);
}

static void image_tim_f_decode_alpha(INT32 args)
{
   img_tim_decode(args,0);
   push_string(make_shared_string("alpha"));
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
