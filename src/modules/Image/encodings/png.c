/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: png.c,v 1.66 2004/09/18 23:00:25 nilsson Exp $
*/

#include "global.h"
RCSID("$Id: png.c,v 1.66 2004/09/18 23:00:25 nilsson Exp $");

#include "image_machine.h"

#include "object.h"
#include "interpret.h"
#include "svalue.h"
#include "threads.h"
#include "mapping.h"
#include "pike_error.h"
#include "stralloc.h"
#include "builtin_functions.h"
#include "operators.h"

#include "image.h"
#include "colortable.h"


#define sp Pike_sp

extern struct program *image_colortable_program;
extern struct program *image_program;

static struct program *gz_inflate=NULL;
static struct program *gz_deflate=NULL;
static struct svalue gz_crc32;

static struct pike_string *param_palette;
static struct pike_string *param_spalette;
static struct pike_string *param_image;
static struct pike_string *param_alpha;
static struct pike_string *param_type;
static struct pike_string *param_bpp;
static struct pike_string *param_background;

/*! @module Image
 */

/*! @module PNG
 *!   Support for encoding and decoding the Portable Network Graphics
 *!   format, PNG.
 *!
 *! @note
 *!   This module uses zlib.
 */

static INLINE void push_nbo_32bit(size_t x)
{
   char buf[4];
   buf[0] = DO_NOT_WARN((char)(x>>24));
   buf[1] = DO_NOT_WARN((char)(x>>16));
   buf[2] = DO_NOT_WARN((char)(x>>8));
   buf[3] = DO_NOT_WARN((char)(x));
   push_string(make_shared_binary_string(buf,4));
}

static INLINE unsigned long int_from_32bit(unsigned char *data)
{
   return (data[0]<<24)|(data[1]<<16)|(data[2]<<8)|(data[3]);
}

#define int_from_16bit(X) _int_from_16bit((unsigned char*)(X))
static INLINE unsigned long _int_from_16bit(unsigned char *data)
{
   return (data[0]<<8)|(data[1]);
}

static INLINE COLORTYPE _png_c16(unsigned long z,int bpp)
{
   switch (bpp)
   {
      case 16: return DO_NOT_WARN((COLORTYPE)(z>>8));
      case 4:  return DO_NOT_WARN((COLORTYPE)(z*17));
      case 2:  return DO_NOT_WARN((COLORTYPE)(z*0x55));
      case 1:  return DO_NOT_WARN((COLORTYPE)(z*255));
      default: return DO_NOT_WARN((COLORTYPE)z);
   }
}

static INLINE INT32 call_gz_crc32(INT32 args)
{
   INT32 z;
   apply_svalue(&gz_crc32,args);
   if (sp[-1].type!=T_INT)
      PIKE_ERROR("Image.PNG", "Internal error (not integer from Gz.crc32).\n",
		 sp, args);
   z=sp[-1].u.integer;
   pop_stack();
   return z;
}

static INLINE void add_crc_string(void)
{
   push_svalue(sp-1);
   push_nbo_32bit(call_gz_crc32(1));
}

static INLINE INT32 my_crc32(INT32 init,unsigned char *data,INT32 len)
{
   push_string(make_shared_binary_string((char*)data,len));
   push_int(init);
   return call_gz_crc32(2);
}

static void push_png_chunk(char *type,    /* 4 bytes */
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
   add_crc_string();
   f_add(3);
}

static void png_decompress(int style)
{
   struct object *o;

   if (style)
      Pike_error("Internal error: illegal decompression style %d\n",style);
   
   o=clone_object(gz_inflate,0);
   apply(o,"inflate",1);
   free_object(o);
}

static void png_compress(int style)
{
   struct object *o;

   if (style)
      Pike_error("Internal error: illegal decompression style %d\n",style);
   
   push_int(8);
   o=clone_object(gz_deflate,1);
   apply(o,"deflate",1);
   free_object(o);
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
       sp[-args].type!=T_STRING ||
       sp[1-args].type!=T_STRING)
      Pike_error("Image.PNG.chunk: Illegal argument(s)\n");
   
   a=sp[-args].u.string;
   if (a->len!=4)
      Pike_error("Image.PNG.chunk: Type string not 4 characters\n");
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
   int n=0;
   ONERROR uwp;

   if (args<1)
     SIMPLE_TOO_FEW_ARGS_ERROR("Image.PNG.__decode", 1);
   if (sp[-args].type!=T_STRING)
     SIMPLE_BAD_ARG_ERROR("Image.PNG.__decode", 1, "string");

   if (args>1 &&
       (sp[1-args].type!=T_INT ||
	sp[1-args].u.integer!=0))
      nocrc=1;
   
   add_ref(str=sp[-args].u.string);
   data=(unsigned char*)str->str;
   len=str->len;
   SET_ONERROR(uwp,do_free_string,str);

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
      UNSET_ONERROR(uwp);
      free_string(str);
      push_int(0);
      return;
   }

   len-=8; data+=8;

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
	 n++;
	 break;
      }
      push_string(make_shared_binary_string((char*)data,x));
      if (!nocrc && x+4<=len)
	 push_int( my_crc32(my_crc32(0,NULL,0),data-4,x+4) ==
		   (INT32)int_from_32bit(data+x) );
      else
	 push_int(0);
      if (x+4>len) break;
      f_aggregate(3);
      n++;
      len-=x+4;
      data+=x+4;
   }

   UNSET_ONERROR(uwp);
   free_string(str);
   f_aggregate(n);
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
	    Pike_error("Unsupported subfilter %d (filter %d)\n", s[-1],type);
      }
   }
}

static int _png_write_rgb(rgb_group *w1,
			  rgb_group *wa1,
			  int type,int bpp,
			  unsigned char *s,
			  size_t len,
			  unsigned long width,
			  size_t n,
			  struct neo_colortable *ct,
			  struct pike_string *trns)
{
   /* returns 1 if alpha channel, 0 if not */
   /* w1, wa1 will be freed upon error */

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
	       while (len--)
	       {
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
	       if(len>(n/4)) len=n/4;
	       while (len--)
	       {
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
	    default:
	       free(wa1); free(w1);
	       Pike_error("Image.PNG._decode: Unsupported color type/bit depth %d (grey)/%d bit.\n",
		     type,bpp);
	 }
	 if (trns && trns->len==2)
	 {
	    rgb_group tr;
	    tr.r=tr.g=tr.b=_png_c16(int_from_16bit(trns->str),bpp);
	    n=n0;
	    while (n--) *(da1++)=tr;
	    return 1; /* alpha channel */
	 }
	 return 0; /* no alpha channel */

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
	    default:
	       free(wa1); free(w1);
	       Pike_error("Image.PNG._decode: Unsupported color type/bit depth %d (rgb)/%d bit.\n",
		     type,bpp);
	 }
	 if (trns && trns->len==6)
	 {
	    rgb_group tr;
	    tr.r=_png_c16(int_from_16bit(trns->str),bpp);
	    tr.g=_png_c16(int_from_16bit(trns->str+2),bpp);
	    tr.b=_png_c16(int_from_16bit(trns->str+4),bpp);

	    n=n0;
	    while (n--) *(da1++)=tr;
	    return 1; /* alpha channel */
	 }
	 return 0; /* no alpha channel */

      case 3: /* 1,2,4,8 bit palette index. Alpha might be in palette */
	 if (!ct)
	 {
	    free(w1);
	    free(wa1);
	    Pike_error("Image.PNG.decode: No palette (PLTE entry), but color type (3) needs one\n");
	 }
	 if (ct->type!=NCT_FLAT)
	 {
	    free(w1);
	    free(wa1);
	    Pike_error("Image.PNG.decode: Internal error (created palette isn't flat)\n");
	 }
	 mz=ct->u.flat.numentries;
	 if (mz==0)
	 {
	    free(w1);
	    free(wa1);
	    Pike_error("Image.PNG.decode: palette is zero entries long; need at least one color.\n");
	 }

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
		  while (n)
		  {
		     int i;
		     for (i=8; i;)
		     {
			i--;
			if (x) 
			{
			   int m=((*s)>>i)&1;
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
		     if (n<8) break;
		     n-=8;
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
	       
	    default:
	       free(w1); free(wa1);
	       Pike_error("Image.PNG._decode: Unsupported color type/bit depth %d (palette)/%d bit.\n",
		     type,bpp);
	 }
	 return !!trns; /* alpha channel if trns chunk */

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
	    default:
	       free(wa1); free(w1);
	       Pike_error("Image.PNG._decode: Unsupported color type/bit depth %d (grey+a)/%d bit.\n",
		     type,bpp);
	 }
	 return 1; /* alpha channel */

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
	    default:
	       free(wa1); free(w1);
	       Pike_error("Image.PNG._decode: Unsupported color type/bit depth %d(rgba)/%d bit.\n",
		     type,bpp);
	 }
	 return 1; /* alpha channel */
      default:
	 free(wa1); free(w1);
	 Pike_error("Image.PNG._decode: Unknown color type %d (bit depth %d).\n",
	       type,bpp);
   }
   Pike_error("Image.PNG._decode: illegal state\n");
   return 0; /* stupid */
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

static void img_png_decode(INT32 args,int header_only)
{
   struct array *a;
   struct mapping *m;
   struct neo_colortable *ct=NULL;
   rgb_group *d1,*da1,*w1,*wa1,*t1,*ta1=NULL;
   struct pike_string *fs,*trns=NULL;
   unsigned char *s,*s0;
   struct image *img;
   ONERROR err, a_err, t_err;

   int n=0,i,x=-1,y;
   struct ihdr
   {
      INT32 width,height;
      int bpp;  /* bit depth, 1, 2, 4, 8 or 16  */
      int type; /* 0, 2,3,4 or 6 */
      int compression; /* 0 */
      int filter; 
      int interlace;
   } ihdr={-1,-1,-1,0,-1,-1,-1};

   if (args<1)
     SIMPLE_TOO_FEW_ARGS_ERROR("Image.PNG._decode", 1);

   m=allocate_mapping(10);
   push_mapping(m);

   if (args>=2)
   {
      if (sp[1-args-1].type!=T_MAPPING)
	SIMPLE_BAD_ARG_ERROR("Image.PNG._decode", 2, "mapping");

      push_svalue(sp+1-args-1);
      ref_push_string(param_palette);
      f_index(2);
      switch (sp[-1].type)
      {
	 case T_OBJECT:
	    push_text("cast");
	    if (sp[-1].type==T_INT)
	       Pike_error("Image.PNG._decode: illegal value of option \"palette\"\n");
	    f_index(2);
	    push_text("array");
	    f_call_function(2);
	 case T_ARRAY:
	 case T_STRING:
	    push_object(clone_object(image_colortable_program,1));

	    ct=(struct neo_colortable*)get_storage(sp[-1].u.object,
						   image_colortable_program);
	    if (!ct)
	       Pike_error("Image.PNG._decode: internal error: cloned colortable isn't colortable\n");
	    ref_push_string(param_palette);
	    mapping_insert(m,sp-1,sp-2);
	    pop_n_elems(2);
	    break;
	 case T_INT:
	    pop_n_elems(1);
	    break;
	 default:
	    Pike_error("Image.PNG._decode: illegal value of option \"palette\"\n");
      }
   }

   sp--;
   pop_n_elems(args-1);
   push_mapping(m);
   stack_swap();

   if (sp[-1].type==T_STRING)
   {
      push_int(1); /* no care crc */
      image_png___decode(2);
      if (sp[-1].type!=T_ARRAY)
	 Pike_error("Image.PNG._decode: Not PNG data\n");
   }
   else if (sp[-1].type!=T_ARRAY)
     SIMPLE_BAD_ARG_ERROR("Image.PNG._decode", 1, "string");

   a=sp[-1].u.array;

   for (i=0; i<a->size; i++)
   {
      struct array *b = NULL;
      unsigned char *data;
      size_t len;

      if (a->item[i].type!=T_ARRAY ||
	  (b=a->item[i].u.array)->size!=3 ||
	  b->item[0].type!=T_STRING ||
	  b->item[1].type!=T_STRING ||
	  b->item[0].u.string->len!=4)
	 Pike_error("Image.PNG._decode: Illegal stuff in array index %d\n",i);

      data = (unsigned char *)b->item[1].u.string->str;
      len = (size_t)b->item[1].u.string->len;

      if (!i &&
	  int_from_32bit((unsigned char*)b->item[0].u.string->str)
	  != 0x49484452 )
	 Pike_error("Imge.PNG.decode: first chunk isn't IHDR\n");

      switch (int_from_32bit((unsigned char*)b->item[0].u.string->str))
      {
	 /* ------ major chunks ------------ */
         case 0x49484452: /* IHDR */
	    /* header info */
	    if (b->item[1].u.string->len!=13)
	       Pike_error("Image.PNG._decode: illegal header (IHDR chunk)\n");

	    ihdr.width=int_from_32bit(data+0);
	    ihdr.height=int_from_32bit(data+4);
	    ihdr.bpp=data[8];
	    ihdr.type=data[9];
	    ihdr.compression=data[10];
	    ihdr.filter=data[11];
	    ihdr.interlace=data[12];
	    break;

         case 0x504c5445: /* PLTE */
	    /* palette info, 3×n bytes */

	    if (ct) break; /* we have a palette already */

	    ref_push_string(b->item[1].u.string);
	    push_object(clone_object(image_colortable_program,1));

	    if (ihdr.type==3)
	    {
	       ct=(struct neo_colortable*)
		  get_storage(sp[-1].u.object,image_colortable_program);
	       ref_push_string(param_palette);
	       mapping_insert(m,sp-1,sp-2);
	    }
	    else
	    {
	       ref_push_string(param_spalette);
	       mapping_insert(m,sp-1,sp-2);
	    }
	    pop_n_elems(2);
	    break;

         case 0x49444154: /* IDAT */
	    /* compressed image data. push, n++ */
	    if (header_only) break;

	    if (ihdr.compression!=0)
	       Pike_error("Image.PNG._decode: unknown compression (%d)\n",
		     ihdr.compression);

	    ref_push_string(b->item[1].u.string);
	    n++;
	    if (n>32) { f_add(n); n=1; }
	    break;

         case 0x49454e44: /* IEND */
	    /* end of file */
	    break;

	 /* ------ minor chunks ------------ */

         case 0x6348524d: /* cHRM */
	    break;

         case 0x67414d41: /* gAMA */
	    break;

         case 0x73524742: /* sRGB */
	   break;

         case 0x69434350: /* iCCP */
	   break;

         case 0x73424954: /* sBIT */
	    break;

         case 0x624b4744: /* bKGD */
	    switch (ihdr.type)
	    {
	       case 0:
	       case 4:
		  if (b->item[1].u.string->len==2)
		  {
		     int z=_png_c16(b->item[1].u.string->str[0],ihdr.bpp);
		     push_int(z);
		     push_int(z);
		     push_int(z);
		  }
		  else
		     continue;
		  break;
	       case 3:
		  if (b->item[1].u.string->len!=1 ||
		      !ct || ct->type!=NCT_FLAT ||
		      b->item[1].u.string->str[0] >=
		      ct->u.flat.numentries)
		     continue;
		  else
		  {
		     push_int(ct->u.flat.entries[(int)b->item[1].u.string->str[0]].color.r);
		     push_int(ct->u.flat.entries[(int)b->item[1].u.string->str[0]].color.g);
		     push_int(ct->u.flat.entries[(int)b->item[1].u.string->str[0]].color.b);
		  }
                  break;
	       default:
		  if (b->item[1].u.string->len==6)
		  {
		     int j;
		     for (j=0; j<3; j++)
		     {
			int z=_png_c16(b->item[1].u.string->str[j*2],ihdr.bpp);
			push_int(z);
		     }
		  } else
                    continue;
		  break;
	    }
	    f_aggregate(3);
	    ref_push_string(param_background);
	    mapping_insert(m,sp-1,sp-2);
	    pop_n_elems(2);
	    break;

         case 0x68495354: /* hIST */
	    break;

         case 0x74524e53: /* tRNS */
	    push_string(trns=b->item[1].u.string);
	    push_int(-2);
	    mapping_insert(m,sp-1,sp-2);
	    sp-=2; /* we have no own ref to trns */
	    break;

         case 0x70485973: /* pHYs */
	    break;

         case 0x74494d45: /* tIME */
	    break;

         case 0x7a455874: /* tEXt */
	    break;

         case 0x7a545874: /* zTXt */
	    break;

         case 0x69545874: /* iTXt */
	   break;

         /* Extensions */
         case 0x6f464673: /* oFFs */
	   break;

         case 0x7043414c: /* pCAL */
	   break;

         case 0x7343414c: /* sCAL */
	   break;

         case 0x67494667: /* gIFg */
	   break;

         case 0x67494678: /* gIFg */
	   break;

         case 0x66524163: /* fRAc */
	   break;
      }
   }

   if (header_only)  goto header_stuff;

   /* on stack: mapping   n×string */

   /* IDAT stuff on stack, now */
   if (!n) 
      push_text("");
   else
      f_add(n);

   if (ihdr.type==-1)
   {
      Pike_error("Image.PNG._decode: missing header (IHDR chunk)\n");
   }
   if (ihdr.type==3 && !ct)
   {
      Pike_error("Image.PNG._decode: missing palette (PLTE chunk)\n");
   }

   if (ihdr.compression==0)
   {
      png_decompress(ihdr.compression);
      if (sp[-1].type!=T_STRING)
	 Pike_error("Image.PNG._decode: got weird stuff from decompression\n");
   }
   else
      Pike_error("Image.PNG._decode: illegal compression type 0x%02x\n",
	    ihdr.compression);

   fs=sp[-1].u.string;
   push_int(-1);
   mapping_insert(m,sp-1,sp-2);

   pop_n_elems(2);

   s=(unsigned char*)fs->str;
   if(sizeof(rgb_group)*(double)ihdr.width*(double)ihdr.height>(double)INT_MAX)
       Pike_error("Image.PNG._decode: Too large image "
		  "(total size exceeds %d bytes (%.0f))\n", INT_MAX, 
		 sizeof(rgb_group)*(double)ihdr.width*(double)ihdr.height);
   w1=d1=xalloc(sizeof(rgb_group)*ihdr.width*ihdr.height);
   SET_ONERROR(err, free_and_clear, &d1);
   wa1=da1=xalloc(sizeof(rgb_group)*ihdr.width*ihdr.height);
   SET_ONERROR(a_err, free_and_clear, &da1);

   /* --- interlace decoding --- */

   switch (ihdr.interlace)
   {
      case 0: /* none */
	 fs=_png_unfilter((unsigned char*)fs->str,fs->len,
			  ihdr.width,ihdr.height,
			  ihdr.filter,ihdr.type,ihdr.bpp,
			  NULL);
	 push_string(fs);
	 if (!_png_write_rgb(w1,wa1,
			     ihdr.type,ihdr.bpp,(unsigned char*)fs->str,
			     fs->len,
			     ihdr.width,
			     ihdr.width*ihdr.height,
			     ct,trns))
	 {
	    free(wa1);
	    wa1=NULL;
	 }
	 pop_stack();
	 break;

      case 1: /* adam7 */

	 /* need arena */
	 t1=xalloc(sizeof(rgb_group)*ihdr.width*ihdr.height);
	 SET_ONERROR(t_err, free_and_clear, &t1);
	 ta1=xalloc(sizeof(rgb_group)*ihdr.width*ihdr.height);
	 UNSET_ONERROR(t_err);

	 /* loop over adam7 interlace's 
	    and write them to the arena */

	 s0=(unsigned char*)fs->str;
	 for (i=0; i<7; i++)
	 {
	    struct pike_string *ds;
	    unsigned int x0 = adam7[i].x0;
	    unsigned int xd = adam7[i].xd;
	    unsigned int y0 = adam7[i].y0;
	    unsigned int yd = adam7[i].yd;
	    unsigned int iwidth = (ihdr.width+xd-1-x0)/xd;
	    unsigned int iheight = (ihdr.height+yd-1-y0)/yd;

	    if(!iwidth || !iheight) continue;

	    ds=_png_unfilter(s0,fs->len-(s0-(unsigned char*)fs->str),
			     iwidth, iheight,
			     ihdr.filter,ihdr.type,ihdr.bpp,
			     &s0);

	    push_string(ds);
	    if (!_png_write_rgb(w1,wa1,ihdr.type,ihdr.bpp,
				(unsigned char*)ds->str,ds->len,
				iwidth,
				iwidth*iheight,
				ct,trns))
	    {
	       if (wa1) free(wa1);
	       wa1=NULL;
	    }
	    d1=w1;
	    for (y=y0; y<ihdr.height; y+=yd)
	       for (x=x0; x<ihdr.width; x+=xd)
		  t1[x+y*ihdr.width]=*(d1++);

	    if (wa1)
	    {
	       da1=wa1;
	       for (y=y0; y<ihdr.height; y+=yd)
		  for (x=x0; x<ihdr.width; x+=xd)
		     ta1[x+y*ihdr.width]=*(da1++);
	    }

	    pop_stack();
	 }

	 free(w1);
	 w1=t1;
	 if (wa1) { free(wa1); wa1=ta1; }

	 break;
      default:
	 Pike_error("Image.PNG._decode: Unknown interlace type\n");
   }

   UNSET_ONERROR(a_err);
   UNSET_ONERROR(err);
   
   
   /* --- done, store in mapping --- */

   ref_push_string(param_image);
   push_object(clone_object(image_program,0));
   img=(struct image*)get_storage(sp[-1].u.object,image_program);
   if (img->img) free(img->img); /* protect from memleak */
   img->xsize=ihdr.width;
   img->ysize=ihdr.height;
   img->img=w1;
   mapping_insert(m,sp-2,sp-1);
   pop_n_elems(2);
   
   if (wa1)
   {
      ref_push_string(param_alpha);
      push_object(clone_object(image_program,0));
      img=(struct image*)get_storage(sp[-1].u.object,image_program);
      if (img->img) free(img->img); /* protect from memleak */
      img->xsize=ihdr.width;
      img->ysize=ihdr.height;
      img->img=wa1;
      mapping_insert(m,sp-2,sp-1);
      pop_n_elems(2);
   }

header_stuff:

   ref_push_string(param_type);
   push_int(ihdr.type);
   mapping_insert(m,sp-2,sp-1);
   pop_n_elems(2);
   ref_push_string(param_bpp);
   push_int(ihdr.bpp);
   mapping_insert(m,sp-2,sp-1);
   pop_n_elems(2);

   push_constant_text("xsize");
   push_int(ihdr.width);
   mapping_insert(m,sp-2,sp-1);
   pop_n_elems(2);
   push_constant_text("ysize");
   push_int(ihdr.height);
   mapping_insert(m,sp-2,sp-1);
   pop_n_elems(2);

   push_int(-1);
   map_delete(m,sp-1);
   pop_stack();
   push_int(-2);
   map_delete(m,sp-1);
   pop_stack();

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
   char buf[20];
   
   if (!args)
     SIMPLE_TOO_FEW_ARGS_ERROR("Image.PNG.encode", 1);

   if (sp[-args].type!=T_OBJECT ||
       !(img=(struct image*)
	 get_storage(sp[-args].u.object,image_program)))
     SIMPLE_BAD_ARG_ERROR("Image.PNG.encode", 1, "Image.Image");

   if (!img->img)
      Pike_error("Image.PNG.encode: no image\n");

   if (args>1)
   {
      if (sp[1-args].type!=T_MAPPING)
	SIMPLE_BAD_ARG_ERROR("Image.PNG.encode", 2, "mapping");

      push_svalue(sp+1-args);
      ref_push_string(param_alpha);
      f_index(2);
      if ( !UNSAFE_IS_ZERO( sp - 1 ) )
	 if (sp[-1].type!=T_OBJECT ||
	     !(alpha=(struct image*)
	       get_storage(sp[-1].u.object,image_program)))
	    PIKE_ERROR("Image.PNG.encode",
		       "Option (arg 2) \"alpha\" has illegal type.\n",
		       sp, args);
      pop_stack();

      if (alpha &&
	  (alpha->xsize!=img->xsize ||
	   alpha->ysize!=img->ysize))
	 PIKE_ERROR("Image.PNG.encode",
		    "Option (arg 2) \"alpha\"; images differ in size.\n",
		    sp, args);
      if (alpha && !alpha->img)
	PIKE_ERROR("Image.PNG.encode", "Option (arg 2) \"alpha\"; no image\n",
		   sp, args);

      push_svalue(sp+1-args);
      ref_push_string(param_palette); 
      f_index(2);
      if (!(sp[-1].type==T_INT 
	    && sp[-1].subtype==NUMBER_UNDEFINED))
	 if (sp[-1].type!=T_OBJECT ||
	     !(ct=(struct neo_colortable*)
	       get_storage(sp[-1].u.object,image_colortable_program)))
	   PIKE_ERROR("Image.PNG.encode",
		      "Option (arg 2) \"palette\" has illegal type.\n",
		      sp, args);
      pop_stack();
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
	 PIKE_ERROR("Image.PNG.encode", "Palette size to large; "
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
	   0 /* compression */,
	   0 /* filter */,
	   0 /* interlace */);
   push_string(make_shared_binary_string(buf,13));

   push_png_chunk("IHDR",NULL);
   n++;
	      
   if (ct)
   {
      struct pike_string *ps;
      ps=begin_shared_string(3<<bpp);
      MEMSET(ps->str,0,3<<bpp);
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
	 PIKE_ERROR("Image.PNG.encode",
		    "Colortable and alpha channel not supported "
		    "at the same time.\n", sp, args);
      }
      else
      {
	 unsigned char *tmp=xalloc(img->xsize*img->ysize),*ts;

	 image_colortable_index_8bit_image(ct,img->img,tmp,
					   img->xsize*img->ysize,img->xsize);
	 ts=tmp;
	 while (y--)
	 {
	    unsigned char *d;
	    struct pike_string *ps;
	    ps=begin_shared_string((img->xsize*bpp+7)/8+1);
	    d=(unsigned char*)ps->str;
	    x=img->xsize;
	    *(d++)=0; /* filter */
	    if (bpp==8)
	    {
	       MEMCPY(d,ts,img->xsize);
	       ts+=img->xsize;
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
	    }
	    push_string(end_shared_string(ps));
	 }
	 free(tmp);
      }
   }
   else
      while (y--)
      {
	 struct pike_string *ps;
	 unsigned char *d;
	 ps=begin_shared_string(img->xsize*(3+!!alpha)+1);
	 d=(unsigned char*)ps->str;
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
	 push_string(end_shared_string(ps));
      }
   f_add(img->ysize);
   png_compress(0);
   push_png_chunk("IDAT",NULL);
   n++;

   push_text("");
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
   img_png_decode(args,0);
}

/*! @decl mapping decode_header(string data)
 *!   Decodes only the PNG headers.
 *! @seealso
 *!   @[_decode]
 */

static void image_png_decode_header(INT32 args)
{
   img_png_decode(args,1);
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
     SIMPLE_TOO_FEW_ARGS_ERROR("Image.PNG.decode", 1);
   
   image_png__decode(args);
   push_constant_text("image");
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
     SIMPLE_TOO_FEW_ARGS_ERROR("Image.PNG.decode_alpha", 1);

   image_png__decode(args);
   assign_svalue_no_free(&s,sp-1);
   push_constant_text("alpha");
   f_index(2);

   if (sp[-1].type==T_INT)
   {
      push_svalue(&s);
      push_constant_text("xsize");
      f_index(2);
      push_svalue(&s);
      push_constant_text("ysize");
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
   free_string(param_type);
   free_svalue(&gz_crc32);

   if(gz_inflate)
     free_program(gz_inflate);

   if(gz_deflate)
     free_program(gz_deflate);
}

void init_image_png(void)
{
   push_text("Gz");
   SAFE_APPLY_MASTER("resolv",1);
   if (sp[-1].type==T_OBJECT) 
   {
     stack_dup();
     push_text("inflate");
     f_index(2);
     gz_inflate=program_from_svalue(sp-1);
     if(gz_inflate) 
       add_ref(gz_inflate);
     pop_stack();

     stack_dup();
     push_text("deflate");
     f_index(2);
     gz_deflate=program_from_svalue(sp-1);
     if(gz_deflate) 
       add_ref(gz_deflate);
     pop_stack();

     stack_dup();
     push_text("crc32");
     f_index(2);
     gz_crc32=sp[-1];
     sp--;
   }else{
     gz_crc32.type=T_INT;
   }
   pop_stack();

   if (gz_deflate && gz_inflate && gz_crc32.type!=T_INT)
   {
      add_function("_chunk",image_png__chunk,
		   "function(string,string:string)",
		   OPT_TRY_OPTIMIZE);
      add_function("__decode",image_png___decode,
		   "function(string:array)",
		   OPT_TRY_OPTIMIZE);

      add_function("decode_header",image_png_decode_header,
		   "function(string:mapping)",
		   OPT_TRY_OPTIMIZE);

      if (gz_deflate)
      {
	 add_function("_decode",image_png__decode,
		      "function(array|string,void|mapping(string:mixed):mapping)",0);
	 add_function("decode",image_png_decode,
		      "function(string,void|mapping(string:mixed):object)",0);
	 add_function("decode_alpha",image_png_decode_alpha,
		      "function(string,void|mapping(string:mixed):object)",0);
      }
      add_function("encode",image_png_encode,
		   "function(object,void|mapping(string:mixed):string)",
		   OPT_TRY_OPTIMIZE);
   }

   param_palette=make_shared_string("palette");
   param_spalette=make_shared_string("spalette");
   param_image=make_shared_string("image");
   param_alpha=make_shared_string("alpha");
   param_bpp=make_shared_string("bpp");
   param_type=make_shared_string("type");
   param_background=make_shared_string("background");
}
