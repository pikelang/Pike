/* -*- mode: C; c-basic-offset: 3; -*-
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "image_machine.h"

#include "object.h"
#include "interpret.h"
#include "svalue.h"
#include "threads.h"
#include "array.h"
#include "mapping.h"
#include "pike_error.h"
#include "stralloc.h"
#include "builtin_functions.h"
#include "operators.h"
#include "module_support.h"
#include "pike_types.h"

#include "image.h"
#include "colortable.h"

#include "bignum.h"
#include "bitvector.h"

#if defined(__MINGW32__) || defined(__amigaos__)
/* encodings.a will never contain a crc32 symbol. */
#define DYNAMIC_MODULE
#endif

#define sp Pike_sp

extern struct program *image_colortable_program;
extern struct program *image_program;

#ifdef DYNAMIC_MODULE
typedef unsigned INT32 (_crc32)(unsigned INT32, unsigned char*,
				unsigned INT32);
typedef void (_pack)(struct pike_string*, struct byte_buffer*, int, int, int);
typedef void (_unpack)(struct pike_string*, struct byte_buffer*, int);
static _crc32 *crc32;
static _pack *zlibmod_pack;
static _unpack *zlibmod_unpack;
#else
extern unsigned INT32 crc32(unsigned INT32, unsigned char*, unsigned INT32);
extern void zlibmod_pack(struct pike_string*, struct byte_buffer*, int, int, int);
extern void zlibmod_unpack(struct pike_string*, struct byte_buffer*, int);
#endif

static struct pike_string *param_palette;
static struct pike_string *param_spalette;
static struct pike_string *param_image;
static struct pike_string *param_alpha;
static struct pike_string *param_bpp;
static struct pike_string *param_background;
static struct pike_string *param_zlevel;
static struct pike_string *param_zstrategy;

/*! @module Image
 */

/*! @module PNG
 *!   Support for encoding and decoding the Portable Network Graphics
 *!   format, PNG.
 *!
 *! @note
 *!   This module uses zlib.
 */

static inline void push_nbo_32bit(size_t x)
{
   char buf[4];
   buf[0] = (char)(x>>24);
   buf[1] = (char)(x>>16);
   buf[2] = (char)(x>>8);
   buf[3] = (char)(x);
   push_string(make_shared_binary_string(buf,4));
}

static inline unsigned long int_from_32bit(const unsigned char *data)
{
   /* NB: Avoid sign-extension in implicit cast from int to unsigned long. */
   return (((unsigned long)data[0])<<24)|(data[1]<<16)|(data[2]<<8)|(data[3]);
}

#define int_from_16bit(X) _int_from_16bit((unsigned char*)(X))
static inline unsigned long _int_from_16bit(const unsigned char *data)
{
   return (data[0]<<8)|(data[1]);
}

static inline COLORTYPE _png_c16(unsigned long z,int bpp)
{
   switch (bpp)
   {
      case 16: return (COLORTYPE)(z>>8);
      case 4:  return (COLORTYPE)(z*17);
      case 2:  return (COLORTYPE)(z*0x55);
      case 1:  return (COLORTYPE)(z*255);
      default: return (COLORTYPE)z;
   }
}

static void push_png_chunk(const char *type,    /* 4 bytes */
			   struct pike_string *data) /* (freed) or on stack */
{
   /*
    *  0: 4 bytes of length of data block (=n)
    *  4: 4 bytes of chunk type
    *  8: n bytes of data
    *  8+n: 4 bytes of CRC
    */

   if (!data) { data=sp[-1].u.string; sp--; }

   push_nbo_32bit(data->len);
   push_string(make_shared_binary_string(type,4));
   push_string(data);
   f_add(2);
   push_nbo_32bit(crc32(0,(unsigned char*)sp[-1].u.string->str,
			(unsigned INT32)(sp[-1].u.string->len)));
   f_add(3);
}

static void png_decompress(int style)
{
  struct byte_buffer buf;
  ONERROR err;

  if (style!=0)
    Pike_error("Internal error: Illegal decompression style %d.\n",style);

  buffer_init(&buf);
  SET_ONERROR(err, buffer_free, &buf);
  zlibmod_unpack(Pike_sp[-1].u.string, &buf, 0);
  UNSET_ONERROR(err);

  pop_stack();
  push_string(buffer_finish_pike_string(&buf));
}

static void png_compress(int style, int zlevel, int zstrategy)
{
  struct byte_buffer buf;
  ONERROR err;

  if (style)
    Pike_error("Internal error: Illegal decompression style %d.\n",style);

  buffer_init(&buf);
  SET_ONERROR(err, buffer_free, &buf);
  zlibmod_pack(Pike_sp[-1].u.string, &buf, zlevel, zstrategy, 15);
  UNSET_ONERROR(err);

  pop_stack();
  push_string(buffer_finish_pike_string(&buf));
}

/*! @decl string _chunk(string type, string data)
 *! 	Encodes a PNG chunk.
 *!
 *! @note
 *!	Please read about the PNG file format.
 */

static void image_png__chunk(INT32 args)
{
   struct pike_string *a,*b;

   if (args!=2 ||
       TYPEOF(sp[-args]) != T_STRING ||
       TYPEOF(sp[1-args]) != T_STRING)
      PIKE_ERROR("Image.PNG._chunk", "Illegal argument(s).\n", sp, args);

   a=sp[-args].u.string;
   if (a->len!=4)
      PIKE_ERROR("Image.PNG._chunk", "Type string not 4 characters.\n",
		 sp ,args);
   b=sp[1-args].u.string;
   pop_n_elems(args-2);
   sp-=2;
   push_png_chunk(a->str,b);
   free_string(a);
}


/*! @decl array __decode(string data)
 *! @decl array __decode(string data, int dontcheckcrc)
 *! 	Splits a PNG file into chunks.
 *!
 *! @returns
 *!   Result is an array of arrays, or 0 if data isn't a PNG file.
 *!   Each element in the array is constructed as follows.
 *!   @array
 *!     @elem string 0
 *!       The type of the chunk, e.g. "IHDR" or "IDAT".
 *!     @elem string 1
 *!       The actual chunk data.
 *!     @elem int(0..1) 2
 *!       Set to 1 if the checksum is ok and @[dontcheckcrc]
 *!       isn't set.
 *!   @endarray
 *!
 *! @note
 *!   Please read about the PNG file format.
 *!   Support for decoding cHRM, gAMA, sBIT, hIST, pHYs, tIME,
 *!   tEXt and zTXt chunks are missing.
 */

static void image_png___decode(INT32 args)
{
   int nocrc=0;
   unsigned char *data;
   size_t len;
   struct pike_string *str;
   ONERROR uwp;

   if (args<1)
     SIMPLE_WRONG_NUM_ARGS_ERROR("__decode", 1);
   if (TYPEOF(sp[-args]) != T_STRING)
     SIMPLE_ARG_TYPE_ERROR("__decode", 1, "string");

   if (args>1 &&
       (TYPEOF(sp[1-args]) != T_INT ||
	sp[1-args].u.integer!=0))
      nocrc=1;

   add_ref(str=sp[-args].u.string);
   data=(unsigned char*)str->str;
   len=str->len;

   pop_n_elems(args);

   if (len<8 ||
       data[0]!=137 ||
       data[1]!='P' ||
       data[2]!='N' ||
       data[3]!='G' ||
       data[4]!=13 ||
       data[5]!=10 ||
       data[6]!=26 ||
       data[7]!=10)
   {
      free_string(str);
      push_int(0);
      return;
   }

   SET_ONERROR(uwp,do_free_string,str);
   len-=8; data+=8;

   check_stack(20);
   BEGIN_AGGREGATE_ARRAY(10);

   while (len>8)
   {
      unsigned long x;
      x=int_from_32bit(data);
      push_string(make_shared_binary_string((char*)data+4,4));
      len-=8;
      data+=8;

      if (x>len)
      {
	push_string(make_shared_binary_string((char*)data,len));
	push_int(0);
	f_aggregate(3);
	DO_AGGREGATE_ARRAY(20);
        break;
      }

      push_string(make_shared_binary_string((char*)data,x));
      if (!nocrc && x+4<=len)
	push_int( crc32(crc32(0,NULL,0),data-4,x+4) ==
                  int_from_32bit(data+x) );
      else
	push_int(0);
      f_aggregate(3);
      DO_AGGREGATE_ARRAY(20);

      if (x+4>len) break;

      if( int_from_32bit((unsigned char *)data) == 0x49454e44 ) /* IEND */
        break;

      len-=x+4;
      data+=x+4;
   }
   CALL_AND_UNSET_ONERROR(uwp);

   END_AGGREGATE_ARRAY;
}

/*! @decl mapping _decode(string|array data)
 *! @decl mapping _decode(string|array data, mapping options)
 *!   Decode a PNG image file.
 *!
 *! @param options
 *!    @mapping
 *!      @member string|array|Image.Colortable "colortable"
 *!        A replacement color table to be used instead of the one
 *!        in the PNG file, if any.
 *!    @endmapping
 *!
 *! @returns
 *!   @mapping
 *!     @member Image.Image "image"
 *!       The decoded image.
 *!     @member int "bpp"
 *!       Number of bitplanes in the image. One of 1, 2, 4, 8 and 16.
 *!     @member int "type"
 *!       Image color type. Bit values are:
 *!       @int
 *!         @value 1
 *!           Palette used.
 *!         @value 2
 *!           Color used.
 *!         @value 4
 *!           Alpha channel used.
 *!       @endint
 *!       Valid values are 0, 2, 3, 4 and 6.
 *!     @member int "xsize"
 *!     @member int "ysize"
 *!       Image dimensions.
 *!     @member array(int) "background"
 *!       The background color, if any. An array of size three with
 *!       the RGB values.
 *!     @member Image.Image "alpha"
 *!       The alpha channel, if any.
 *!   @endmapping
 *!
 *! @throws
 *!   Throws an error if the image data is erroneous.
 *!
 *! @note
 *!	Please read about the PNG file format.
 *!	This function ignores any checksum errors in the file.
 *!	A PNG of higher color resolution than the Image module
 *!	supports (8 bit) will lose that information in the conversion.
 */

/*
 *     basic options:
 *
 *	    "alpha": object alpha,            - alpha channel
 *
 *	    "palette": object colortable,     - image palette
 *                                             (if non-truecolor)
 *
 *     advanced options:
 *
 *	    "background": array(int) color,   - suggested background color
 *	    "background_index": int index,    - what index in colortable
 *
 *	    "chroma": ({ float white_point_x,
 *	                 float white_point_y,
 *			 float red_x,
 *			 float red_y,         - CIE x,y chromaticities
 *			 float green_x,
 *			 float green_y,
 *			 float blue_x,
 *			 float blue_y })
 *
 *	    "gamma":  float gamma,            - gamma
 *
 *	    "spalette": object colortable,    - suggested palette,
 *                                             for truecolor images
 *	    "histogram": array(int) hist,     - histogram for the image,
 *	                                        corresponds to palette index
 *
 *	    "physical": ({ int unit,          - physical pixel dimension
 *	                   int x,y })           unit 0 means pixels/meter
 *
 *	    "sbit": array(int) sbits          - significant bits
 *
 *	    "text": array(array(string)) text - text information,
 *                 ({ ({ keyword, data }), ... })
 *
 *                 Standard keywords:
 *
 *                 Title          Short (one line) title or caption for image
 *                 Author         Name of image's creator
 *                 Description    Description of image (possibly long)
 *                 Copyright      Copyright notice
 *                 Creation Time  Time of original image creation
 *                 Software       Software used to create the image
 *                 Disclaimer     Legal disclaimer
 *                 Warning        Warning of nature of content
 *                 Source         Device used to create the image
 *                 Comment        Miscellaneous comment
 *
 *	    "time": ({ int year, month, day,  - time of last modification
 *	               hour, minute, second })
 *
 *      wizard options:
 *	    "compression": int method         - compression method (0)
 */

static struct pike_string *_png_unfilter(unsigned char *data,
					 size_t len,
					 int xsize,int ysize,
					 int filter,int type,
					 int bpp,
					 unsigned char **pos)
{
   struct pike_string *ps;
   unsigned char *d;
   unsigned char *s;
   int x;
   int sbb;

   if(filter!=0)
     Pike_error("Unknown filter type %d.\n", filter);

   switch (type) /* Each pixel is ... */
   {
      case 2: x=3; break; /* an R,G,B triple */
      case 4: x=2; break; /* a grayscale sample, followed by an alpha sample */
      case 6: x=4; break; /* an R,G,B triple, followed by an alpha sample */
      default: x=1;       /* a palette index / a grayscale sample */
   }

   bpp*=x; /* multipy for units/pixel (rgba=4, grey=1, etc) */
   xsize=(xsize*bpp+7)>>3; /* xsize in bytes */

   ps=begin_shared_string(len-((len+xsize)/(xsize+1)));
   d=(unsigned char*)ps->str;
   s=data;

   sbb=(bpp+7)>>3; /* rounding up */

   for (;;)
   {
      if (!len || !ysize--)
      {
	 if (pos) *pos=s;
         if (d - STR0(ps) < ps->len) {
            /* Should we throw an error here, instead?
             * This case means that there is extra
             * IDAT data, when we have already processed all
             * scanlines specified in the IHDR
             *  /arne
             */
            memset(d, 0, ps->len - (d - STR0(ps)));
         }
	 return end_shared_string(ps);
      }
      x=xsize;

#if 0
      fprintf(stderr,
	      "%05d 0x%08lx %02x %02x %02x > %02x < %02x %02x %02x "
	      "len=%ld xsize=%d sbb=%d next=0x%08lx d=0x%lx nd=0x%lx\n",
	      ysize+1,(unsigned long)s,
	      s[-3],s[-2],s[-1],s[0],s[1],s[2],s[3],
	      len,xsize,sbb,
	      (unsigned long)s+xsize+1,
	      (unsigned long)d,
	      (unsigned long)d+xsize);
#endif

      switch (*(s++))
      {
	 case 0: /* no filter */
	    while (x-- && --len)
	       *(d++)=*(s++);
	    if (len) len--;
	    break;
	 case 1: /* sub left */
	    while (x-- && --len)
	       if (x+sbb<xsize)
		  *d=s[0]+d[-sbb],d++,s++;
	       else
		  *(d++)=*(s++);
	    if (len) len--;
	    break;
	 case 2: /* sub up */
	    if (d - (unsigned char*)ps->str >= xsize)
	       while (x-- && --len)
		  *d=s[0]+d[-xsize],d++,s++;
	    else
	       while (x-- && --len)
		  *(d++)=*(s++);
	    if (len) len--;
	    break;
	 case 3: /* average */
	    while (x-- && --len)
	    {
	       int a,b;

	       if (x+sbb<xsize) a=d[-sbb]; else a=0;
	       if (d - (unsigned char*)ps->str >= xsize) b=d[-xsize]; else b=0;
	       a=(a+b)>>1;

	       *d=*s+a;

	       d++;
	       s++;
	    }
	    if (len) len--;

	    break;
	 case 4: /* paeth */
	    while (x-- && --len)
	    {
	       int a,b,c,p,pa,pb,pc;

	       if (x+sbb<xsize)
		  {
		     a=d[-sbb];
		     if (d - (unsigned char*)ps->str >= xsize)
		     {
			b=d[-xsize];
			c=d[-xsize-sbb];
		     }
		     else b=c=0;

		     p=a+b-c;
		     pa=abs(p-a);
		     pb=abs(p-b);
		     pc=abs(p-c);
		     if (pa<=pb && pa<=pc) p=a;
		     else if (pb<=pc) p=b;
		     else p=c;

		     *d=(unsigned char)(p+*s);
		  }
	       else if (d - (unsigned char*)ps->str >= xsize)
		  *d=(unsigned char)(d[-xsize]+*s); /* de facto */
	       else *d=*s;

	       d++;
	       s++;
	    }
	    if (len) len--;

	    break;
	 default:
            free_string(ps);
	    Pike_error("Unsupported subfilter %d (filter %d)\n", s[-1],type);
      }
   }
}

static void _png_write_rgb(rgb_group *w1,
                           rgb_group *wa1,
                           int type,int bpp,
                           const unsigned char *s,
                           size_t len,
                           unsigned long width,
                           size_t n,
                           const struct neo_colortable *ct,
                           const struct pike_string *trns)
{
   static const rgb_group white={255,255,255};
   static const rgb_group grey4[4]={{0,0,0},{85,85,85},
			      {170,170,170},{255,255,255}};
   static const rgb_group black={0,0,0};

   rgb_group *d1=w1;
   rgb_group *da1=wa1;

   size_t n0=n;

   unsigned long x;
   ptrdiff_t mz;

   /* write stuff to d1 */

   switch (type)
   {
      case 0: /* 1,2,4,8 or 16 bit greyscale */
	 switch (bpp)
	 {
	    case 1:
	       x=width;
	       while (len-- && n >= (x > 8 ? 8 : x))
	       {
                  n -= x > 8 ? 8 : x;
		  if (x) x--,*(d1++)=((*s)&128)?white:black;
		  if (x) x--,*(d1++)=((*s)&64)?white:black;
		  if (x) x--,*(d1++)=((*s)&32)?white:black;
		  if (x) x--,*(d1++)=((*s)&16)?white:black;
		  if (x) x--,*(d1++)=((*s)&8)?white:black;
		  if (x) x--,*(d1++)=((*s)&4)?white:black;
		  if (x) x--,*(d1++)=((*s)&2)?white:black;
		  if (x) x--,*(d1++)=((*s)&1)?white:black;
		  if (!x) x=width;
		  s++;
	       }
	       break;
	    case 2:
	       x=width;
	       while (len-- && n >= (x > 4 ? 4 : x))
	       {
                  n -= x > 4 ? 4 : x;
		  if (x) x--,*(d1++)=grey4[((*s)>>6)&3];
		  if (x) x--,*(d1++)=grey4[((*s)>>4)&3];
		  if (x) x--,*(d1++)=grey4[((*s)>>2)&3];
		  if (x) x--,*(d1++)=grey4[(*s)&3];
		  if (!x) x=width;
		  s++;
	       }
	       break;
	    case 4:
	       if (n>len*2) n=len*2;
	       x=width;
	       while (n)
	       {
		  int q;
		  if (x)
		  {
		     x--,q=(((*s)>>4)&15)|((*s)&240);
		     d1->r=d1->g=d1->b=q; d1++;
		  }
		  if (x)
		  {
		     x--,q=((*s)&15)|((*s)<<4);
		     d1->r=d1->g=d1->b=q; d1++;
		  }
		  if (n<2) break;
		  n-=2;
		  s++;
		  if (!x) x=width;
	       }
	       break;
	    case 8:
	       if (n>len) n=len;
	       while (n)
	       {
		  d1->r=d1->g=d1->b=*(s++); d1++;
		  n--;
	       }
	       break;
	    case 16:
	       if (n>len/2) n=len/2;
	       while (n)
	       {
		  d1->r=d1->g=d1->b=*(s++); d1++; s++;
		  n--;
	       }
	       break;
	 }
	 if (trns && trns->len==2)
	 {
	    rgb_group tr;
	    tr.r=tr.g=tr.b=_png_c16(int_from_16bit(trns->str),bpp);
	    n=n0;
	    d1=w1;
	    while (n--) *(da1++)=(tr.r==(d1++)->r)?black:white;
	 }
	 return;

      case 2: /* 8 or 16 bit r,g,b */
	 switch (bpp)
	 {
	    case 8:
	       if (n>len/3) n=len/3;
	       while (n)
	       {
		  d1->r=*(s++);
		  d1->g=*(s++);
		  d1->b=*(s++);
		  d1++;
		  n--;
	       }
	       break;
	    case 16:
	       if (n>len/6) n=len/6;
	       while (n)
	       {
		  d1->r=*s;
		  s += 2;
		  d1->g=*s;
		  s += 2;
		  d1->b=*s;
		  s += 2;
		  d1++;
		  n--;
	       }
	       break;
	 }
	 if (trns && trns->len==6)
	 {
	    rgb_group tr;
	    tr.r=_png_c16(int_from_16bit(trns->str),bpp);
	    tr.g=_png_c16(int_from_16bit(trns->str+2),bpp);
	    tr.b=_png_c16(int_from_16bit(trns->str+4),bpp);

	    n=n0;
	    d1=w1;
	    while (n--) *(da1++)=(tr.r==d1->r && tr.g==d1->g && tr.b==d1->b)?black:white,d1++;
	 }
	 return;

      case 3: /* 1,2,4,8 bit palette index. Alpha might be in palette */
	 mz=ct->u.flat.numentries;

#define CUTPLTE(X,Z) (((X)>=(Z))?0:(X))
	 switch (bpp)
	 {
	    case 1:
	       if (n>len*8) n=len*8;
	       x=width;
	       if (!trns)
		  while (n)
		  {
		    if (n&&x) n--,x--,*(d1++)=ct->u.flat.entries[CUTPLTE(((*s)>>7)&1,mz)].color;
                    if (n&&x) n--,x--,*(d1++)=ct->u.flat.entries[CUTPLTE(((*s)>>6)&1,mz)].color;
                    if (n&&x) n--,x--,*(d1++)=ct->u.flat.entries[CUTPLTE(((*s)>>5)&1,mz)].color;
                    if (n&&x) n--,x--,*(d1++)=ct->u.flat.entries[CUTPLTE(((*s)>>4)&1,mz)].color;
                    if (n&&x) n--,x--,*(d1++)=ct->u.flat.entries[CUTPLTE(((*s)>>3)&1,mz)].color;
                    if (n&&x) n--,x--,*(d1++)=ct->u.flat.entries[CUTPLTE(((*s)>>2)&1,mz)].color;
                    if (n&&x) n--,x--,*(d1++)=ct->u.flat.entries[CUTPLTE(((*s)>>1)&1,mz)].color;
                    if (n&&x) n--,x--,*(d1++)=ct->u.flat.entries[CUTPLTE((*s)&1,mz)].color;
                    s++;
                    if (!x) x=width;
		  }
	       else
		  while (n>0)
		  {
		     int i;
		     for (i=8; i; i--)
		     {
                       int m=((*s)>>(i-1))&1;
                       if(!x) break;
                       x--;
                       *(d1++)=ct->u.flat.entries[CUTPLTE(m,mz)].color;
                       if (m>=trns->len)
                         *(da1++)=white;
                       else
                       {
                         da1->r=trns->str[m];
                         da1->g=trns->str[m];
                         da1->b=trns->str[m];
                         da1++;
                       }
		     }
		     s++;
		     if (n<8 && n<width) break;
		     n -= (8-i);
		     if (!x) x=width;
		  }
	       break;
	    case 2:
	       if (n>len*4) n=len*4;
	       x=width;
	       if (!trns)
		  while (n)
		  {
		     if (n&&x) x--,n--,*(d1++)=ct->u.flat.entries[CUTPLTE(((*s)>>6)&3,mz)].color;
		     if (n&&x) x--,n--,*(d1++)=ct->u.flat.entries[CUTPLTE(((*s)>>4)&3,mz)].color;
		     if (n&&x) x--,n--,*(d1++)=ct->u.flat.entries[CUTPLTE(((*s)>>2)&3,mz)].color;
		     if (n&&x) x--,n--,*(d1++)=ct->u.flat.entries[CUTPLTE((*s)&3,mz)].color;
		     s++;
		     if (!x) x=width;
		  }
	       else
		  while (n)
		  {
		     int i;
		     for (i=8; i;)
		     {
			i-=2;
			if (x)
			{
			   int m=((*s)>>i)&3;
			   x--;
			   *(d1++)=ct->u.flat.entries[CUTPLTE(m,mz)].color;
			   if (m>=trns->len)
			      *(da1++)=white;
			   else
			   {
			      da1->r=trns->str[m];
			      da1->g=trns->str[m];
			      da1->b=trns->str[m];
			      da1++;
			   }
			}
		     }
		     s++;
		     if (n<4) break;
		     n-=4;
		     if (!x) x=width;
		  }
	       break;
	    case 4:
	       if (n>len*2) n=len*2;
	       x=width;
	       if (!trns)
		  while (n)
		  {
		     if (n&&x) x--,n--,*(d1++)=ct->u.flat.entries[CUTPLTE(((*s)>>4)&15,mz)].color;
		     if (n&&x) x--,n--,*(d1++)=ct->u.flat.entries[CUTPLTE((*s)&15,mz)].color;
		     s++;
		     if (!x) x=width;
		  }
	       else
		  while (n)
		  {
		     int i;
		     for (i=8; i>=4;)
		     {
			i-=4;
			if (x)
			{
			   int m=((*s)>>i)&15;
			   x--;
			   *(d1++)=ct->u.flat.entries[CUTPLTE(m,mz)].color;
			   if (m>=trns->len)
			      *(da1++)=white;
			   else
			   {
			      da1->r=trns->str[m];
			      da1->g=trns->str[m];
			      da1->b=trns->str[m];
			      da1++;
			   }
			}
		     }
		     s++;
		     if (n<2) break;
		     n-=2;
		     if (!x) x=width;
		  }
	       break;
	    case 8:
	       if (n>len) n=len;

	       if (!trns)
		  while (n)
		  {
		     *(d1++)=ct->u.flat.entries[CUTPLTE(*s,mz)].color;

		     s++;
		     n--;
		  }
	       else
		  while (n)
		  {
		     int m=CUTPLTE(*s,mz);
		     *(d1++)=ct->u.flat.entries[m].color;
		     if (m>=trns->len)
			*(da1++)=white;
		     else
		     {
			da1->r=trns->str[m];
			da1->g=trns->str[m];
			da1->b=trns->str[m];
			da1++;
		     }

		     s++;
		     n--;
		  }
	       break;
	 }
	 return;

      case 4: /* 8 or 16 bit grey,a */
	 switch (bpp)
	 {
	    case 8:
	       if (n>len/2) n=len/2;
	       while (n)
	       {
		  d1->r=d1->g=d1->b=*(s++);
		  da1->r=da1->g=da1->b=*(s++);
		  d1++;
		  da1++;
		  n--;
	       }
	       break;
	    case 16:
	       if (n>len/4) n=len/4;
	       while (n)
	       {
		  d1->r=d1->g=d1->b=*s;
		  s += 2;
		  d1++;
		  da1->r=da1->g=da1->b=*s;
		  s += 2;
		  da1++;
		  n--;
	       }
	       break;
	 }
	 return;

      case 6: /* 8 or 16 bit r,g,b,a */
	 switch (bpp)
	 {
	    case 8:
	       if (n>len/4) n=len/4;
	       while (n)
	       {
		  d1->r=*(s++);
		  d1->g=*(s++);
		  d1->b=*(s++);
		  da1->r=da1->g=da1->b=*(s++);
		  d1++;
		  da1++;
		  n--;
	       }
	       break;
	    case 16:
	       if (n>len/8) n=len/8;
	       while (n)
	       {
		  d1->r=*s;
		  s += 2;
		  d1->g=*s;
		  s += 2;
		  d1->b=*s;
		  s += 2;
		  d1++;
		  da1->r=da1->g=da1->b=*s;
		  s += 2;
		  da1++;
		  n--;
	       }
	       break;
	 }
	 return;
   }
}

struct png_interlace
{
   int y0,yd,x0,xd;
};

static const struct png_interlace adam7[8]=
{ {0,8,0,8},
  {0,8,4,8},
  {4,8,0,4},
  {0,4,2,4},
  {2,4,0,2},
  {0,2,1,2},
  {1,2,0,1} };

static void free_and_clear(void **mem)
{
  if(*mem) {
    free(*mem);
    *mem=0;
  }
}

struct IHDR
{
  unsigned INT32 width,height;
  int bpp;  /* bit depth, 1, 2, 4, 8 or 16  */
  int type; /* 0, 2,3,4 or 6 */
  int compression; /* 0 */
  int filter;
  int interlace;
};

/* IN: IDAT string on stack */
/* OUT: Image object with image (and possibly alpha, depending on
   return value) */
static int _png_decode_idat(struct IHDR *ihdr, struct neo_colortable *ct,
                            struct pike_string *trns)
{
  struct pike_string *fs;
  struct image *img;
  rgb_group *w1,*wa1=NULL;
  unsigned char *s0;
  unsigned int i,x,y,got_alpha=0;
  ONERROR err, a_err;

  switch(ihdr->type)
  {
  case 0: /* 1,2,4,8 or 16 bit greyscale */
    switch(ihdr->bpp)
    {
    case 1:
    case 2:
    case 4:
    case 8:
    case 16:
      break;
    default:
      Pike_error("Image.PNG._decode: Unsupported color type/bit depth %d (grey)/%d bit.\n", 0, ihdr->bpp);
    }
    if( trns && trns->len==2 )
      got_alpha=1;
    break;

  case 2: /* 8 or 16 bit r,g,b */
    switch(ihdr->bpp)
    {
    case 8:
    case 16:
      break;
    default:
      Pike_error("Image.PNG._decode: Unsupported color type/bit depth %d (rgb)/%d bit.\n", 1, ihdr->bpp);
    }
    if( trns && trns->len==6)
      got_alpha=1;
    break;

  case 3: /* 1,2,4,8 bit palette index */
    if (!ct)
      Pike_error("Image.PNG.decode: No palette, but color type 3 needs one.\n");
    if (ct->type!=NCT_FLAT)
      Pike_error("Image.PNG.decode: Internal error (created palette isn't flat).\n");
    if (ct->u.flat.numentries==0)
      Pike_error("Image.PNG.decode: Palette is zero entries long;"
                 " need at least one color.\n");
    switch(ihdr->bpp)
    {
    case 1:
    case 2:
    case 4:
    case 8:
      break;
    default:
      Pike_error("Image.PNG._decode: Unsupported color type/bit depth %d (palette)/%d bit.\n", 3, ihdr->bpp);
    }
    got_alpha = !!trns;
    break;

  case 4: /* 8 or 16 bit grey */
    switch(ihdr->bpp)
    {
    case 8:
    case 16:
      break;
    default:
      Pike_error("Image.PNG._decode: Unsupported color type/bit depth %d (palette)/%d bit.\n", 4, ihdr->bpp);
    }
    got_alpha=1;
    break;

  case 6: /* 8 ot 16 bit rgba */
    switch(ihdr->bpp)
    {
    case 8:
    case 16:
      break;
    default:
      Pike_error("Image.PNG._decode: Unsupported color type/bit depth %d (palette)/%d bit.\n", 4, ihdr->bpp);
    }
    got_alpha=1;
    break;

  default:
    Pike_error("Image.PNG._decode: Unsupported color type/bit depth %d (palette)/%d bit.\n", ihdr->type, ihdr->bpp);
  }

  png_decompress(ihdr->compression);
  if( TYPEOF(sp[-1]) != T_STRING )
    Pike_error("Got illegal data from decompression.\n");

  if (INT32_MUL_OVERFLOW(ihdr->width, ihdr->height) ||
      ihdr->width*ihdr->height > (0x7fffffff - RGB_VEC_PAD)/sizeof(rgb_group)) {
    /* Overflow in size calculation below. */
    Pike_error("Image too large (%d * %d)\n",
	       ihdr->width, ihdr->height);
  }

  if( got_alpha )
    wa1=xalloc(sizeof(rgb_group)*ihdr->width*ihdr->height + RGB_VEC_PAD);
  SET_ONERROR(a_err, free_and_clear, &wa1);
  w1=xalloc(sizeof(rgb_group)*ihdr->width*ihdr->height + RGB_VEC_PAD);
  SET_ONERROR(err, free_and_clear, &w1);

  fs = sp[-1].u.string;

  /* --- interlace decoding --- */

  switch (ihdr->interlace)
  {
  case 0: /* none */
    fs=_png_unfilter((unsigned char*)fs->str,fs->len,
                     ihdr->width,ihdr->height,
                     ihdr->filter,ihdr->type,ihdr->bpp,
                     NULL);

    push_string(fs);

    _png_write_rgb(w1,wa1,
                   ihdr->type,ihdr->bpp,
                   (unsigned char*)fs->str,fs->len,
                   ihdr->width,
                   ihdr->width*ihdr->height,
                   ct,trns);
    pop_stack();
    break;

  case 1: /* adam7 */
    {
      rgb_group *t1, *ta1 = NULL;
      ONERROR t_err, ta_err;

      /* need arena */
      t1=xalloc(sizeof(rgb_group)*ihdr->width*ihdr->height + RGB_VEC_PAD);
      SET_ONERROR(t_err, free_and_clear, &t1);
      if( got_alpha )
        ta1=xalloc(sizeof(rgb_group)*ihdr->width*ihdr->height + RGB_VEC_PAD);
      SET_ONERROR(ta_err, free_and_clear, &ta1);

      /* loop over adam7 interlace's
	 and write them to the arena */

      s0=(unsigned char*)fs->str;
      for (i=0; i<7; i++)
      {
	struct pike_string *ds;
	rgb_group *d1, *da1 = NULL;
	unsigned int x0 = adam7[i].x0;
	unsigned int xd = adam7[i].xd;
	unsigned int y0 = adam7[i].y0;
	unsigned int yd = adam7[i].yd;
	unsigned int iwidth = (ihdr->width+xd-1-x0)/xd;
	unsigned int iheight = (ihdr->height+yd-1-y0)/yd;

	if(!iwidth || !iheight) continue;

	ds=_png_unfilter(s0,fs->len-(s0-(unsigned char*)fs->str),
			 iwidth, iheight,
			 ihdr->filter,ihdr->type,ihdr->bpp,
			 &s0);

        push_string(ds);

	_png_write_rgb(w1,wa1,ihdr->type,ihdr->bpp,
                       (unsigned char*)ds->str,ds->len,
                       iwidth,
                       iwidth*iheight,
                       ct,trns);

        if( got_alpha )
	{
	  da1 = wa1;
	  for (y=y0; y<ihdr->height; y+=yd)
	    for (x=x0; x<ihdr->width; x+=xd)
	      ta1[x+y*ihdr->width]=*(da1++);
	}
	d1=w1;
	for (y=y0; y<ihdr->height; y+=yd)
	  for (x=x0; x<ihdr->width; x+=xd)
	    t1[x+y*ihdr->width]=*(d1++);

        pop_stack();
      }

      UNSET_ONERROR(ta_err);
      UNSET_ONERROR(t_err);

      if (got_alpha) {
        free(wa1);
	wa1 = ta1;
      }

      free(w1);
      w1 = t1;
    }
    break;
  default:
    Pike_error("Unknown interlace type %d.\n", ihdr->interlace);
  }

  /* Image data now in w1, alpha in wa1 */
  pop_stack();

  /* Create image object and leave it on the stack */
  push_object(clone_object(image_program,0));
  img=get_storage(sp[-1].u.object,image_program);
  if (img->img) free(img->img); /* protect from memleak */
  img->xsize=ihdr->width;
  img->ysize=ihdr->height;
  img->img=w1;

  UNSET_ONERROR(err);

  /* Create alpha object and leave it on the stack */
  if( wa1 )
  {
    push_object(clone_object(image_program,0));
    img=get_storage(sp[-1].u.object,image_program);
    if (img->img) free(img->img); /* protect from memleak */
    img->xsize=ihdr->width;
    img->ysize=ihdr->height;
    img->img=wa1;
    UNSET_ONERROR(a_err);
    return 1;
  }
  UNSET_ONERROR(a_err);

  return 0;
}

static void png_free_string(struct pike_string *str)
{
  if(str) free_string(str);
}

#define MODE_ALL          0
#define MODE_HEADER_ONLY  1
#define MODE_IMAGE_ONLY   2
static void img_png_decode(INT32 args, int mode)
{
   struct array *a;
   struct mapping *m;
   struct neo_colortable *ct=NULL;
   struct pike_string *trns=NULL;

   int n=0, i;
   struct IHDR ihdr={-1,-1,-1,-1,-1,-1,-1};
   ONERROR err;

   if (args<1)
     SIMPLE_WRONG_NUM_ARGS_ERROR("_decode", 1);

   m=allocate_mapping(10);
   push_mapping(m);

   if (args>=2)
   {
      if (TYPEOF(sp[1-args-1]) != T_MAPPING)
	SIMPLE_ARG_TYPE_ERROR("_decode", 2, "mapping");

      push_svalue(sp+1-args-1);
      ref_push_string(param_palette);
      f_index(2);
      switch (TYPEOF(sp[-1]))
      {
	 case T_ARRAY:
	 case T_STRING:
	    push_object(clone_object(image_colortable_program,1));

            /* Fallthrough */
	 case T_OBJECT:
	    ct=get_storage(sp[-1].u.object,
                           image_colortable_program);
	    if (!ct)
	       PIKE_ERROR("_decode",
			  "Object isn't colortable.\n", sp, args);
	    mapping_string_insert(m, param_palette, sp-1);
	    pop_stack();
	    break;
	 case T_INT:
	    pop_stack();
	    break;
	 default:
	    PIKE_ERROR("_decode",
		       "Illegal value of option \"palette\".\n",
		       sp, args);
      }
   }

   sp--;
   pop_n_elems(args-1);
   push_mapping(m);
   stack_swap();

   if (TYPEOF(sp[-1]) == T_STRING)
   {
      push_int(1); /* no care crc */
      image_png___decode(2);
      if (TYPEOF(sp[-1]) != T_ARRAY)
	 PIKE_ERROR("_decode", "Not PNG data.\n", sp ,args);
   }
   else if (TYPEOF(sp[-1]) != T_ARRAY)
     SIMPLE_ARG_TYPE_ERROR("_decode", 1, "string");

   a=sp[-1].u.array;

   for (i=0; i<a->size; i++)
   {
      struct array *b = NULL;
      unsigned char *data;
      size_t len;

#ifdef PIKE_DEBUG
      if (TYPEOF(a->item[i]) != T_ARRAY ||
	  (b=a->item[i].u.array)->size!=3 ||
	  TYPEOF(b->item[0]) != T_STRING ||
	  TYPEOF(b->item[1]) != T_STRING ||
	  b->item[0].u.string->len!=4)
	 Pike_error("Image.PNG._decode: Illegal stuff in array index %d\n",i);
#else
      b = a->item[i].u.array;
#endif

      data = (unsigned char *)b->item[1].u.string->str;
      len = (size_t)b->item[1].u.string->len;

      if (!i &&
	  int_from_32bit((unsigned char*)b->item[0].u.string->str)
	  != 0x49484452 )
	 PIKE_ERROR("_decode", "First chunk isn't IHDR.\n",
		    sp, args);

      switch (int_from_32bit((unsigned char*)b->item[0].u.string->str))
      {
	 /* ------ major chunks ------------ */
         case 0x49484452: { /* IHDR */
            size_t bytes;
	    /* header info */
	    if (len!=13)
	       PIKE_ERROR("_decode",
			  "Illegal header (IHDR chunk).\n", sp, args);

	    ihdr.width=int_from_32bit(data+0);
	    ihdr.height=int_from_32bit(data+4);

            if (!ihdr.width || !ihdr.height)
              Pike_error("Image.PNG._decode: Invalid dimensions in IHDR chunk.\n");

            if (DO_SIZE_T_MUL_OVERFLOW(ihdr.width, ihdr.height, &bytes) ||
                DO_SIZE_T_MUL_OVERFLOW(bytes, sizeof(rgb_group), &bytes) ||
                bytes > INT_MAX) {
              Pike_error("Image.PNG._decode: Too large image "
                         "(total size exceeds %d bytes)\n", INT_MAX);
            }

	    ihdr.bpp=data[8];
	    ihdr.type=data[9];
	    ihdr.compression=data[10];
	    ihdr.filter=data[11];
	    ihdr.interlace=data[12];
	    break;
         }
         case 0x504c5445: /* PLTE */
	    /* palette info, 3�n bytes */

	    if (ct) break; /* we have a palette already */

	    ref_push_string(b->item[1].u.string);
	    push_object(clone_object(image_colortable_program,1));

	    if (ihdr.type==3)
	    {
	       ct=get_storage(sp[-1].u.object,image_colortable_program);
	       mapping_string_insert(m, param_palette, sp-1);
	    }
	    else
	    {
	       mapping_string_insert(m, param_spalette, sp-1);
	    }
	    pop_stack();
	    break;

         case 0x49444154: /* IDAT */
	    /* compressed image data. push, n++ */
	    if ( mode == MODE_HEADER_ONLY ) break;

	    ref_push_string(b->item[1].u.string);
	    n++;
	    if (n>32) { f_add(n); n=1; }
	    break;

         case 0x49454e44: /* IEND */
	    /* end of file */
	    break;

        /* ------ minor chunks ------------ */

          case 0x6348524d: /* cHRM */
	  {
	    int i;
            if(mode==MODE_IMAGE_ONLY) break;
	    if(len!=32) break;
	    for(i=0; i<32; i+=4)
	      push_float((float)int_from_32bit(data+i)/100000.0);
	    f_aggregate(8);
	    push_static_text("chroma");
	    mapping_insert(m,sp-1,sp-2);
	    pop_n_elems(2);
	  }
	  break;

          case 0x73424954: /* sBIT */
	  {
	    size_t i;
            if(mode==MODE_IMAGE_ONLY) break;
            /* sBIT chunks are not longer than 4 bytes */
            if (len > 4) break;
	    for(i=0; i<len; i++)
	      push_int(data[i]);
	    f_aggregate(len);
	    push_static_text("sbit");
	    mapping_insert(m,sp-1,sp-2);
	    pop_n_elems(2);
	  }
	  break;

          case 0x67414d41: /* gAMA */
            if(mode==MODE_IMAGE_ONLY) break;
	    if(len!=4) break;
	    push_static_text("gamma");
	    push_float((float)int_from_32bit(data)/100000.0);
	    mapping_insert(m,sp-2,sp-1);
	    pop_n_elems(2);
	    break;

          case 0x70485973: { /* pHYs */
            int tmp1, tmp2;
            if(mode==MODE_IMAGE_ONLY) break;
	    if(len!=9) break;
            tmp1 = get_unaligned_be32(data);
            tmp2 = get_unaligned_be32(data+4);
            /* the image dimensions are valid in the range
             * 0 .. MAX_INT32
             */
            if (data[8] != 0 && data[8] != 1) break;
            if (tmp1 < 0 || tmp2 < 0) break;
	    push_int(data[8]);
	    push_int(tmp1);
	    push_int(tmp2);
	    f_aggregate(3);
	    push_static_text("physical");
	    mapping_insert(m,sp-1,sp-2);
	    pop_n_elems(2);
	    break;
          }
          case 0x6f464673: { /* oFFs */
            int tmp1, tmp2;
            if(mode==MODE_IMAGE_ONLY) break;
	    if(len!=9) break;
            tmp1 = get_unaligned_be32(data);
            tmp2 = get_unaligned_be32(data+4);
            /* the oFFs image offsets are valid in the range
             * of -MAX_INT32 .. MAX_INT32 */
            if (data[8] != 0 && data[8] != 1) break;
            if (tmp1 == MIN_INT32 || tmp2 == MIN_INT32) break;
	    push_int(data[8]);
	    push_int(tmp1);
	    push_int(tmp2);
	    f_aggregate(3);
	    push_static_text("offset");
	    mapping_insert(m,sp-1,sp-2);
	    pop_n_elems(2);
	    break;
          }
          case 0x74494d45: /* tIME */
            if(mode==MODE_IMAGE_ONLY) break;
	    if(len!=7) break;
	    push_int(int_from_16bit(data));
	    push_int(data[2]);
	    push_int(data[3]);
	    push_int(data[4]);
	    push_int(data[5]);
	    push_int(data[6]);
	    f_aggregate(6);
	    push_static_text("time");
	    mapping_insert(m,sp-1,sp-2);
	    pop_n_elems(2);
	    break;

         case 0x624b4744: /* bKGD */
            if(mode==MODE_IMAGE_ONLY) break;
	    switch (ihdr.type)
	    {
	       case 0:
	       case 4:
		  if (len==2)
		  {
		     int z=_png_c16(data[0],ihdr.bpp);
		     push_int(z);
		     push_int(z);
		     push_int(z);
		  }
		  else
		     continue;
		  break;
	       case 3:
		  if (len!=1 ||
		      !ct || ct->type!=NCT_FLAT ||
		      data[0] >= ct->u.flat.numentries)
		     continue;
		  else
		  {
		     push_int(ct->u.flat.entries[data[0]].color.r);
		     push_int(ct->u.flat.entries[data[0]].color.g);
		     push_int(ct->u.flat.entries[data[0]].color.b);
		  }
                  break;
	       default:
		  if (len==6)
		  {
		     int j;
		     for (j=0; j<3; j++)
		     {
			int z=_png_c16(data[j*2],ihdr.bpp);
			push_int(z);
		     }
		  } else
                    continue;
		  break;
	    }
	    f_aggregate(3);
	    mapping_string_insert(m, param_background, sp-1);
	    pop_stack();
	    break;

         case 0x74524e53: /* tRNS */
            if (trns) break;
	    trns=b->item[1].u.string;
            add_ref(trns);
            SET_ONERROR(err, png_free_string, trns);
	    break;

         default:
            if(mode==MODE_IMAGE_ONLY) break;

            /* Private chunks from Adobe Fireworks: prVW, mkBF, mkBS,
               mkBT, mkTS */

	    ref_push_string(b->item[1].u.string);
	    ref_push_string(b->item[0].u.string);
            /* do not replace existing entries */
	    low_mapping_insert(m,sp-1,sp-2,0);
	    pop_n_elems(2);
      }
   }

   /* on stack: mapping   n�string */

   if ( mode != MODE_HEADER_ONLY )
   {
     if (ihdr.type==-1)
       PIKE_ERROR("Image.PNG._decode", "Missing header (IHDR chunk).\n",
                  sp, args);

     if (ihdr.type==3 && !ct)
       PIKE_ERROR("Image.PNG._decode", "Missing palette (PLTE chunk).\n",
                  sp, args);

     /* Join IDAT blocks */
     if (!n)
       push_empty_string();
     else
       f_add(n);

     if (_png_decode_idat(&ihdr, ct, trns)==1)
     {
       mapping_string_insert(m, param_alpha, sp-1);
       pop_stack();
     }
     mapping_string_insert(m, param_image, sp-1);
     pop_stack();

   }

   if (trns) {
     UNSET_ONERROR(err);
     png_free_string (trns);
   }

   if ( mode != MODE_IMAGE_ONLY )
   {
     push_int(ihdr.type);
     mapping_string_insert(m, literal_type_string, sp-1);
     pop_stack();

     push_int(ihdr.bpp);
     mapping_string_insert(m, param_bpp, sp-1);
     pop_stack();

     push_static_text("xsize");
     push_int(ihdr.width);
     mapping_insert(m,sp-2,sp-1);
     pop_n_elems(2);

     push_static_text("ysize");
     push_int(ihdr.height);
     mapping_insert(m,sp-2,sp-1);
     pop_n_elems(2);
   }

   pop_stack(); /* remove 'a' from stack */
}


/*! @decl string encode(Image.Image image)
 *! @decl string encode(Image.Image image, mapping options)
 *! 	Encodes a PNG image.
 *!
 *! @param options
 *!   @mapping
 *!     @member Image.Image "alpha"
 *!       Use this image as alpha channel (Note: PNG alpha
 *!       channel is grey. The values are calculated by (r+2g+b)/4.)
 *!     @member Image.Colortable "palette"
 *!       Use this as palette for pseudocolor encoding
 *!       (Note: encoding with alpha channel and pseudocolor
 *!       at the same time are not supported)
 *!     @member int(0..9) "zlevel"
 *!       The level of z-compression to be applied. Default is 9.
 *!     @member int "zstrategy"
 *!       The type of LZ77 strategy to be used. Possible values are
 *!       @[Gz.DEFAULT_STRATEGY], @[Gz.FILTERED], @[Gz.HUFFMAN_ONLY],
 *!       @[Gz.RLE], @[Gz.FIXED]. Default is @[Gz.DEFAULT_STRATEGY].
 *!   @endmapping
 *!
 *! @seealso
 *!   @[__decode]
 *!
 *! @note
 *!	Please read some about PNG files.
 */

static void image_png_encode(INT32 args)
{
   struct image *img = NULL, *alpha = NULL;
   rgb_group *s,*sa=NULL;
   struct neo_colortable *ct=NULL;

   int n=0,y,x,bpp;
   int zlevel=9;
   int zstrategy=0;
   char buf[20];

   if (!args)
     SIMPLE_WRONG_NUM_ARGS_ERROR("encode", 1);

   if (TYPEOF(sp[-args]) != T_OBJECT ||
       !(img=get_storage(sp[-args].u.object,image_program)))
     SIMPLE_ARG_TYPE_ERROR("encode", 1, "Image.Image");

   if (!img->img)
      PIKE_ERROR("encode", "No image.\n", sp, args);

   if ((args>1) && !IS_UNDEFINED(sp + 1-args))
   {
     struct svalue *s;

     if (TYPEOF(sp[1-args]) != T_MAPPING)
	SIMPLE_ARG_TYPE_ERROR("encode", 2, "mapping");

      /* Attribute alpha */
      s = low_mapping_string_lookup(sp[1-args].u.mapping, param_alpha);

      if( s )
      {
        if( TYPEOF(*s) == T_OBJECT &&
            (alpha=get_storage(s->u.object,image_program)) )
        {
          if (alpha->xsize!=img->xsize ||
              alpha->ysize!=img->ysize)
            PIKE_ERROR("encode",
                       "Option (arg 2) \"alpha\"; images differ in size.\n",
                       sp, args);

          if (!alpha->img)
            PIKE_ERROR("encode",
                       "Option (arg 2) \"alpha\"; no image\n",
                       sp, args);
        }
        else if( !(TYPEOF(*s) == T_INT && s->u.integer==0) )
          PIKE_ERROR("encode",
                     "Option (arg 2) \"alpha\" has illegal type.\n",
                     sp, args);
      }

      /* Attribute palette */
      s = low_mapping_string_lookup(sp[1-args].u.mapping, param_palette);

      if (s && !(TYPEOF(*s) == T_INT && s->u.integer==0))
	 if (TYPEOF(*s) != T_OBJECT ||
	     !(ct=get_storage(s->u.object,image_colortable_program)))
	   PIKE_ERROR("encode",
		      "Option (arg 2) \"palette\" has illegal type.\n",
		      sp, args);

      /* Attribute zlevel */
      s = low_mapping_string_lookup(sp[1-args].u.mapping, param_zlevel);

      if ( s )
      {
        if( TYPEOF(*s) != T_INT )
          PIKE_ERROR("encode",
                     "Option (arg 2) \"zlevel\" has illegal value.\n",
                     sp, args);
        else
          zlevel = s->u.integer;
      }

      /* Attribute zstrategy */
      s = low_mapping_string_lookup(sp[1-args].u.mapping, param_zstrategy);
      if( s )
      {
        if ( TYPEOF(*s) != T_INT )
          PIKE_ERROR("encode",
                     "Option (arg 2) \"zstrategy\" has illegal value.\n",
                     sp, args);
        else
          zstrategy = s->u.integer;
      }
   }

   sprintf(buf,"%c%c%c%c%c%c%c%c",
	   137,'P','N','G',13,10,26,10);
   push_string(make_shared_binary_string(buf,8));
   n++;

   if (ct)
   {
      ptrdiff_t sz;
      sz = image_colortable_size(ct);
      if (sz>256)
	 PIKE_ERROR("encode", "Palette size to large; "
		    "PNG doesn't support bigger palettes then 256 colors.\n",
		    sp, args);
      if (sz>16) bpp=8;
      else if (sz>4) bpp=4;
      else if (sz>2) bpp=2;
      else bpp=1;
   }
   else
      bpp=8;

   sprintf(buf,"%c%c%c%c%c%c%c%c%c%c%c%c%c",
	   (char)((img->xsize>>24)&255),(char)((img->xsize>>16)&255),
	   (char)((img->xsize>>8)&255), (char)((img->xsize)&255),
	   (char)((img->ysize>>24)&255),(char)((img->ysize>>16)&255),
	   (char)((img->ysize>>8)&255), (char)((img->ysize)&255),
	   bpp /* bpp */,
	   ct?3:(alpha?6:2) /* type (P/(RGBA/RGB)) */,
	   0 /* compression, 0=deflate */,
	   0 /* filter, 0=per line filter */,
	   0 /* interlace */);
   push_string(make_shared_binary_string(buf,13));

   push_png_chunk("IHDR",NULL);
   n++;

   if (ct)
   {
      struct pike_string *ps;
      ps=begin_shared_string(3<<bpp);
      memset(ps->str,0,3<<bpp);
      image_colortable_write_rgb(ct,(unsigned char*)ps->str);
      push_string(end_shared_string(ps));
      push_png_chunk("PLTE",NULL);
      n++;
   }

   y=img->ysize;
   s=img->img;
   if (alpha) sa=alpha->img;
   if (ct) {
      if (alpha) {
	 PIKE_ERROR("encode",
		    "Colortable and alpha channel not supported "
		    "at the same time.\n", sp, args);
      }
      else
      {
         unsigned char *d;
         struct pike_string *ps;
	 unsigned char *tmp, *ts;

         x = img->xsize;
	 tmp=xalloc(x*y + RGB_VEC_PAD);

	 image_colortable_index_8bit_image(ct,s,tmp,x*y,x);
         ps=begin_shared_string( y * ((x*bpp+7)/8+1) );
         d=(unsigned char*)ps->str;
	 ts=tmp;

	 while (y--)
	 {
	    x=img->xsize;
	    *(d++)=0; /* filter */
	    if (bpp==8)
	    {
	       memcpy(d,ts,x);
	       ts += x;
               d += x;
	    }
	    else
	    {
	       int bit=8-bpp;
	       *d=0;
	       while (x--)
	       {
		  *d|=(*ts)<<bit;
		  if (!bit) { bit=8-bpp; *++d=0; }
		  else bit-=bpp;
		  ts++;
	       }
	       if (bit != (8 - bpp)) {
		 /* The last byte wasn't filled fully,
		  * so we need to pad with a few bits.
		  */
		 d++;
	       }
	    }
	 }

#ifdef PIKE_DEBUG
	 if (d != (unsigned char *)(ps->str + ps->len)) {
	   Pike_fatal("PNG data doesn't align properly "
		      "%"PRINTPIKEINT"d x %"PRINTPIKEINT"d (%d bpp) len: %ld, got: %ld.\n",
		      img->xsize, img->ysize, bpp,
		      (long)ps->len, (long)(d - (unsigned char *)ps->str));
	 }
#endif

         push_string(end_shared_string(ps));
	 free(tmp);
      }
   }
   else {
     struct pike_string *ps;
     unsigned char *d;
     ps=begin_shared_string(y*(img->xsize*(3+!!alpha)+1));
     d=(unsigned char*)ps->str;
      while (y--)
      {
	 x=img->xsize;
	 *(d++)=0; /* filter */
	 if (alpha)
	    while (x--)
	    {
	       *(d++)=s->r;
	       *(d++)=s->g;
	       *(d++)=s->b;
	       *(d++)=(sa->r+sa->g*2+sa->b)>>2;
	       s++;
	       sa++;
	    }
	 else
	    while (x--)
	    {
	       *(d++)=s->r;
	       *(d++)=s->g;
	       *(d++)=s->b;
	       s++;
	    }
      }
      push_string(end_shared_string(ps));
   }

   png_compress(0, zlevel, zstrategy);
   push_png_chunk("IDAT",NULL);
   n++;

   push_empty_string();
   push_png_chunk("IEND",NULL);
   n++;

   f_add(n);
   sp--;
   pop_n_elems(args);
   *sp=sp[args];
   sp++;
}

static void image_png__decode(INT32 args)
{
   img_png_decode(args, MODE_ALL);
}

/*! @decl mapping decode_header(string data)
 *!   Decodes only the PNG headers.
 *! @seealso
 *!   @[_decode]
 */

static void image_png_decode_header(INT32 args)
{
   img_png_decode(args, MODE_HEADER_ONLY);
}

/*! @decl Image.Image decode(string data)
 *! @decl Image.Image decode(string data, mapping(string:mixed) options)
 *!   Decodes a PNG image. The @[options] mapping is the
 *!   as for @[_decode].
 *!
 *! @throws
 *!   Throws upon error in data.
 */

static void image_png_decode(INT32 args)
{
   if (!args)
     SIMPLE_WRONG_NUM_ARGS_ERROR("decode", 1);

   img_png_decode(args, MODE_IMAGE_ONLY);
   push_static_text("image");
   f_index(2);
}

/*! @decl Image.Image decode_alpha(string data, @
 *!     void|mapping(string:mixed) options)
 *!   Decodes the alpha channel in a PNG file. The
 *!   @[options] mapping is the same as for @[_decode].
 *!
 *! @throws
 *!   Throws upon error in data.
 */

static void image_png_decode_alpha(INT32 args)
{
   struct svalue s;
   if (!args)
     SIMPLE_WRONG_NUM_ARGS_ERROR("decode_alpha", 1);

   image_png__decode(args);
   assign_svalue_no_free(&s,sp-1);
   push_static_text("alpha");
   f_index(2);

   if (TYPEOF(sp[-1]) == T_INT)
   {
      push_svalue(&s);
      push_static_text("xsize");
      f_index(2);
      push_svalue(&s);
      push_static_text("ysize");
      f_index(2);
      push_int(255);
      push_int(255);
      push_int(255);
      push_object(clone_object(image_program,5));
   }
   free_svalue(&s);
}

/*! @endmodule
 */

/*! @endmodule
 */

/*** module init & exit & stuff *****************************************/

void exit_image_png(void)
{
   free_string(param_palette);
   free_string(param_spalette);
   free_string(param_image);
   free_string(param_alpha);
   free_string(param_bpp);
   free_string(param_background);
   free_string(param_zlevel);
   free_string(param_zstrategy);
}

void init_image_png(void)
{
  int gz = 0;
#ifdef DYNAMIC_MODULE
   crc32 = PIKE_MODULE_IMPORT(_Gz, crc32);
   zlibmod_pack = PIKE_MODULE_IMPORT(_Gz, zlibmod_pack);
   zlibmod_unpack = PIKE_MODULE_IMPORT(_Gz, zlibmod_unpack);
   if(crc32 && zlibmod_pack && zlibmod_unpack)
     gz = 1;
#else
   push_static_text("Gz.inflate");
   SAFE_APPLY_MASTER("resolv",1);
   if( TYPEOF(Pike_sp[-1]) == T_PROGRAM )
     gz = 1;
   pop_stack();
#endif

   if (gz)
   {
     ADD_FUNCTION2("_chunk",image_png__chunk,tFunc(tStr tStr,tStr),0,
		  OPT_TRY_OPTIMIZE);
     ADD_FUNCTION2("__decode",image_png___decode,tFunc(tStr,tArray),0,
		   OPT_TRY_OPTIMIZE);

     ADD_FUNCTION2("decode_header",image_png_decode_header,
		   tFunc(tStr,tMapping),0,OPT_TRY_OPTIMIZE);

     ADD_FUNCTION("_decode",image_png__decode,
		  tFunc(tOr(tArray,tStr) tOr(tVoid,tMap(tStr,tMix)),
			tMapping),0);

     ADD_FUNCTION("decode",image_png_decode,
		  tFunc(tStr tOr(tVoid,tMap(tStr,tMix)), tObj),0);
     ADD_FUNCTION("decode_alpha",image_png_decode_alpha,
		  tFunc(tStr tOr(tVoid,tMap(tStr,tMix)), tObj),0);

     ADD_FUNCTION2("encode",image_png_encode,
		   tFunc(tObj tOr(tVoid,tMap(tStr,tMix)),tStr),0,
		   OPT_TRY_OPTIMIZE);
   }

   param_palette=make_shared_string("palette");
   param_spalette=make_shared_string("spalette");
   param_image=make_shared_string("image");
   param_alpha=make_shared_string("alpha");
   param_bpp=make_shared_string("bpp");
   param_background=make_shared_string("background");
   param_zlevel=make_shared_string("zlevel");
   param_zstrategy=make_shared_string("zstrategy");
}
