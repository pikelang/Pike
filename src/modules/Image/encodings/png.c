#include "global.h"
RCSID("$Id: png.c,v 1.7 1998/04/05 21:10:43 mirar Exp $");

#include "config.h"

#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "threads.h"
#include "array.h"
#include "mapping.h"
#include "error.h"
#include "stralloc.h"
#include "dynamic_buffer.h"

#include "image.h"
#include "colortable.h"

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


void f_add(INT32 args);
void f_aggregate(INT32 args);

/*
**! module Image
**! submodule PNG
**!
**! note
**!	This module uses <tt>zlib</tt>.
*/

static INLINE void push_nbo_32bit(unsigned long x)
{
   char buf[4];
   buf[0]=(char)(x>>24);
   buf[1]=(char)(x>>16);
   buf[2]=(char)(x>>8);
   buf[3]=(char)(x);
   push_string(make_shared_binary_string(buf,4));
}

static INLINE unsigned long int_from_32bit(unsigned char *data)
{
   return (data[0]<<24)|(data[1]<<16)|(data[2]<<8)|(data[3]);
}

static INLINE INT32 call_gz_crc32(INT32 args)
{
   INT32 z;
   apply_svalue(&gz_crc32,args);
   if (sp[-1].type!=T_INT)
      error("Image.PNG: internal error (not integer from Gz.crc32)\n");
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
      error("internal error: illegal decompression style %d\n",style);
   
   o=clone_object(gz_inflate,0);
   apply(o,"inflate",1);
   free_object(o);
}

static void png_compress(int style)
{
   struct object *o;

   if (style)
      error("internal error: illegal decompression style %d\n",style);
   
   push_int(8);
   o=clone_object(gz_deflate,1);
   apply(o,"deflate",1);
   free_object(o);
}

/*
49 48 44 52
00 00 01 e0  00 00 01 68  08 06 00 00  00 8f 37 28  20
-> 00 00 00  04
*/
/*
**! method string _chunk(string type,string data)
**! 	Encodes a PNG chunk.
**!
**! note
**!	Please read about the PNG file format.
*/

static void image_png__chunk(INT32 args)
{
   struct pike_string *a,*b;

   if (args!=2 ||
       sp[-args].type!=T_STRING ||
       sp[1-args].type!=T_STRING)
      error("Image.PNG.chunk: Illegal argument(s)\n");
   
   a=sp[-args].u.string;
   if (a->len!=4)
      error("Image.PNG.chunk: Type string not 4 characters\n");
   b=sp[1-args].u.string;
   pop_n_elems(args-2);
   sp-=2;
   push_png_chunk(a->str,b);
   free_string(a);
}


/*
**! method array __decode(string data)
**! method array __decode(string data, int dontcheckcrc)
**! 	Splits a PNG file into chunks.
**!
**!     Result is an array of arrays,
**!	<tt>({ ({ string chunk_type, string data, int crc_ok }), 
**!            ({ string chunk_type, string data, int crc_ok }) ... })</tt>
**!
**!	<tt>chunk_type</tt> is the type of the chunk, like
**!	<tt>"IHDR"</tt> or <tt>"IDAT"</tt>.
**!
**!	<tt>data</tt> is the actual chunk data.
**!	
**!	<tt>crcok</tt> is set to 1 if the checksum is ok and
**!	<tt>dontcheckcrc</tt> parameter isn't set.
**!
**!	Returns 0 if it isn't a PNG file.
**!
**! note
**!	Please read about the PNG file format.
*/

static void image_png___decode(INT32 args)
{
   int nocrc=0;
   unsigned char *data;
   unsigned long len;
   struct pike_string *str;
   int n=0;

   if (args<1) 
      error("Image.PNG.__decode: too few arguments\n");
   if (sp[-args].type!=T_STRING)
      error("Image.PNG.__decode: illegal argument 1\n");
   
   if (args==2 &&
       (sp[1-args].type!=T_INT ||
	sp[1-args].u.integer!=0))
      nocrc=1;
   
   (str=sp[-args].u.string)->refs++;
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

   free_string(str);
   f_aggregate(n);
}

/*
**! method array _decode(string|array data)
**! 	Decode a PNG image file.
**!
**!     Result is a mapping,
**!	<pre>
**!	([
**!	   "image": object image,
**!
**!        ... options ...
**!     ])
**!	</pre>
**!
**!	<tt>image</tt> is the stored image.
**!
**!	Valid entries in <tt>options</tt> is the same
**!	as given to <ref>encode</ref>:
**!
**!	<pre>
**!     basic options:
**!
**!	    "alpha": object alpha,            - alpha channel
**!
**!	    "palette": object colortable,     - image palette
**!                                             (if non-truecolor)
**!         
**!     advanced options:
**! 
**!	    "background": array(int) color,   - suggested background color
**!	    "background_index": int index,    - what index in colortable
**!
**!	    "chroma": ({ float white_point_x,
**!	                 float white_point_y,
**!			 float red_x,
**!			 float red_y,         - CIE x,y chromaticities
**!			 float green_x,         
**!			 float green_y,
**!			 float blue_x,
**!			 float blue_y })  
**!
**!	    "gamma":  float gamma,            - gamma
**!
**!	    "spalette": object colortable,    - suggested palette, 
**!                                             for truecolor images
**!	    "histogram": array(int) hist,     - histogram for the image,
**!	                                        corresponds to palette index
**!	
**!	    "physical": ({ int unit,          - physical pixel dimension
**!	                   int x,y })           unit 0 means pixels/meter
**!
**!	    "sbit": array(int) sbits          - significant bits
**!
**!	    "text": array(array(string)) text - text information, 
**!                 ({ ({ keyword, data }), ... })
**!
**!                 Standard keywords:
**!
**!                 Title          Short (one line) title or caption for image
**!                 Author         Name of image's creator
**!                 Description    Description of image (possibly long)
**!                 Copyright      Copyright notice
**!                 Creation Time  Time of original image creation
**!                 Software       Software used to create the image
**!                 Disclaimer     Legal disclaimer
**!                 Warning        Warning of nature of content
**!                 Source         Device used to create the image
**!                 Comment        Miscellaneous comment
**!
**!	    "time": ({ int year, month, day,  - time of last modification
**!	               hour, minute, second })  
**!
**!      wizard options:
**!	    "compression": int method         - compression method (0)
**!
**!      </pre>
**!
**! note
**!	Please read about the PNG file format.
**!	This function ignores any checksum errors in the file.
**!	A PNG of higher color resolution then the Image module
**!	supports (8 bit) will have a lose that information in 
**!     the conversion.
**!	It throws if the image data is erranous.
*/

static struct pike_string *_png_unfilter(unsigned char *data,
					 INT32 len,
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

   switch (type)
   {
      case 2: x=3; break;
      case 4: x=2; break;
      case 6: x=4; break;
      default: x=1; 
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

      fprintf(stderr,
	      "%05d 0x%08lx %02x %02x %02x > %02x < %02x %02x %02x "
	      "len=%d xsize=%d sbb=%d next=0x%08lx d=0x%lx nd=0x%lx\n",
	      ysize+1,(unsigned long)s,
	      s[-3],s[-2],s[-1],s[0],s[1],s[2],s[3],
	      len,xsize,sbb,
	      (unsigned long)s+xsize+1,
	      (unsigned long)d,
	      (unsigned long)d+xsize);

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
	    error("Image.PNG._decode: unsupported filter %d\n",s[-1]);
      }
   }
}

static int _png_write_rgb(rgb_group *w1,
			  rgb_group *wa1,
			  int type,int bpp,
			  unsigned char *s,
			  unsigned long len,
			  unsigned long width,
			  int n,
			  struct neo_colortable *ct)
{
   /* returns 1 if interlace, 0 if not */
   /* w1, wa1 will be freed upon error */

   static rgb_group white={255,255,255};
   static rgb_group grey4[4]={{0,0,0},{85,85,85},{170,170,170},{255,255,255}};
   static rgb_group black={0,0,0};

   rgb_group *d1=w1;
   rgb_group *da1=wa1;

   unsigned long x,mz;

   /* write stuff to d1 */

   switch (type)
   {
      case 0: /* 1,2,4,8 or 16 bit greyscale */
	 switch (bpp)
	 {
	    case 1:
	       if (n>len*8) n=len*8;
	       x=width;
	       while (n)
	       {
		  if (x) x--,*(d1++)=((*s)&128)?white:black;
		  if (x) x--,*(d1++)=((*s)&64)?white:black;
		  if (x) x--,*(d1++)=((*s)&32)?white:black;
		  if (x) x--,*(d1++)=((*s)&16)?white:black;
		  if (x) x--,*(d1++)=((*s)&8)?white:black;
		  if (x) x--,*(d1++)=((*s)&4)?white:black;
		  if (x) x--,*(d1++)=((*s)&2)?white:black;
		  if (x) x--,*(d1++)=((*s)&1)?white:black;
		  if (n<8) break;
		  n-=8;
		  s++;
		  if (!x) x=width;
	       }
	       break;
	    case 2:
	       if (n>len*4) n=len*4;
	       x=width;
	       while (n)
	       {
		  if (x) x--,*(d1++)=grey4[((*s)>>6)&3];
		  if (x) x--,*(d1++)=grey4[((*s)>>4)&3];
		  if (x) x--,*(d1++)=grey4[((*s)>>2)&3];
		  if (x) x--,*(d1++)=grey4[(*s)&3];
		  if (n<4) break;
		  n-=4;
		  s++;
		  if (!x) x=width;
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
	       error("Image.PNG->_decode: Unsupported color type/bit depth %d (grey)/%d bit.\n",
		     type,bpp);
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
		  d1->r=*(s++);
		  d1->g=*(s++);
		  d1->b=*(s++);
		  d1++;
		  s++;
		  n--;
	       }
	       break;
	    default:
	       free(wa1); free(w1);
	       error("Image.PNG->_decode: Unsupported color type/bit depth %d (rgb)/%d bit.\n",
		     type,bpp);
	 }
	 return 0; /* no alpha channel */

      case 3: /* 1,2,4,8 bit palette index */
	 if (!ct)
	 {
	    free(w1);
	    free(wa1);
	    error("Image.PNG->decode: No palette (PLTE entry), but color type (3) needs one\n");
	 }
	 if (ct->type!=NCT_FLAT)
	 {
	    free(w1);
	    free(wa1);
	    error("Image.PNG->decode: Internal error (created palette isn't flat)\n");
	 }
	 mz=ct->u.flat.numentries;

#define CUTPLTE(X,Z) (((X)>=(Z))?0:(X))
	 switch (bpp)
	 {
	    case 1:
	       if (n>len*8) n=len*8;
	       x=width;
	       while (n)
	       {
		  if (x) x--,*(d1++)=ct->u.flat.entries[CUTPLTE(((*s)>>7)&1,mz)].color;
		  if (x) x--,*(d1++)=ct->u.flat.entries[CUTPLTE(((*s)>>6)&1,mz)].color;
		  if (x) x--,*(d1++)=ct->u.flat.entries[CUTPLTE(((*s)>>5)&1,mz)].color;
		  if (x) x--,*(d1++)=ct->u.flat.entries[CUTPLTE(((*s)>>4)&1,mz)].color;
		  if (x) x--,*(d1++)=ct->u.flat.entries[CUTPLTE(((*s)>>3)&1,mz)].color;
		  if (x) x--,*(d1++)=ct->u.flat.entries[CUTPLTE(((*s)>>2)&1,mz)].color;
		  if (x) x--,*(d1++)=ct->u.flat.entries[CUTPLTE(((*s)>>1)&1,mz)].color;
		  if (x) x--,*(d1++)=ct->u.flat.entries[CUTPLTE((*s)&1,mz)].color;
		  s++;
		  if (n<8) break;
		  n-=8;
		  if (!x) x=width;
	       }
	       break;
	    case 2:
	       if (n>len*4) n=len*4;
	       x=width;
	       while (n)
	       {
		  if (x) x--,*(d1++)=ct->u.flat.entries[CUTPLTE(((*s)>>6)&3,mz)].color;
		  if (x) x--,*(d1++)=ct->u.flat.entries[CUTPLTE(((*s)>>4)&3,mz)].color;
		  if (x) x--,*(d1++)=ct->u.flat.entries[CUTPLTE(((*s)>>2)&3,mz)].color;
		  if (x) x--,*(d1++)=ct->u.flat.entries[CUTPLTE((*s)&3,mz)].color;
		  s++;
		  if (n<4) break;
		  n-=4;
		  if (!x) x=width;
	       }
	       break;
	    case 4:
	       if (n>len*2) n=len*2;
	       x=width;
	       while (n)
	       {
		  if (x) x--,*(d1++)=ct->u.flat.entries[CUTPLTE(((*s)>>4)&15,mz)].color;
		  if (x) x--,*(d1++)=ct->u.flat.entries[CUTPLTE((*s)&15,mz)].color;
		  s++;
		  if (n<2) break;
		  n--;
		  if (!x) x=width;
	       }
	       break;
	    case 8:
	       if (n>len) n=len;
	       while (n)
	       {
		  *(d1++)=ct->u.flat.entries[CUTPLTE(*s,mz)].color;
		  s++;
		  n--;
	       }
	       break;
	       
	    default:
	       error("Image.PNG->_decode: Unsupported color type/bit depth %d (palette)/%d bit.\n",
		     type,bpp);
	 }
	 return 0; /* no alpha channel */

      case 4: /* 8 or 16 bit grey,a */
	 switch (bpp)
	 {
	    case 8:
	       if (n>len/3) n=len/3;
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
	       if (n>len/6) n=len/6;
	       while (n)
	       {
		  d1->r=d1->g=d1->b=*(s++);
		  d1++;
		  s++;
		  da1->r=da1->g=da1->b=*(s++);
		  s++;
		  da1++;
		  n--;
	       }
	       break;
	    default:
	       free(wa1); free(w1);
	       error("Image.PNG->_decode: Unsupported color type/bit depth %d (grey+a)/%d bit.\n",
		     type,bpp);
	 }
	 return 1; /* alpha channel */

      case 6: /* 8 or 16 bit r,g,b,a */
	 switch (bpp)
	 {
	    case 8:
	       if (n>len/3) n=len/3;
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
	       if (n>len/6) n=len/6;
	       while (n)
	       {
		  d1->r=*(s++);
		  d1->g=*(s++);
		  d1->b=*(s++);
		  d1++;
		  s++;
		  da1->r=da1->g=da1->b=*(s++);
		  s++;
		  da1++;
		  n--;
	       }
	       break;
	    default:
	       free(wa1); free(w1);
	       error("Image.PNG->_decode: Unsupported color type/bit depth %d(rgba)/%d bit.\n",
		     type,bpp);
	 }
	 return 1; /* alpha channel */
      default:
	 free(wa1); free(w1);
	 error("Image.PNG->_decode: Unknown color type %d (bit depth %d).\n",
	       type,bpp);
   }
}

struct png_interlace
{
   int y0,yd,x0,xd;
};

static struct png_interlace adam7[8]=
{ {0,8,0,8},
  {0,8,4,8},
  {4,8,0,4},
  {0,4,2,4},
  {2,4,0,2},
  {0,2,1,2},
  {1,2,0,1} };

static void image_png__decode(INT32 args)
{
   struct array *a;
   struct mapping *m;
   struct neo_colortable *ct=NULL;
   rgb_group *d1,*da1,*w1,*wa1,*t1;
   struct pike_string *fs;
   unsigned char *s,*s0;
   struct image *img;

   int n=0,i,x,y;
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
      error("Image.PNG.__decode: too few arguments\n");

   pop_n_elems(args-1);

   if (sp[-1].type==T_STRING)
   {
      push_int(1); /* no care crc */
      image_png___decode(2);
      if (sp[-1].type!=T_ARRAY)
	 error("Image.PNG._decode: Not PNG data\n");
   }
   else if (sp[-1].type!=T_ARRAY)
      error("Image.PNG._decode: Illegal argument\n");

   (a=sp[-1].u.array)->refs++;

   pop_n_elems(1);

   m=allocate_mapping(10);
   push_mapping(m);

   for (i=0; i<a->size; i++)
   {
      struct array *b;
      unsigned char *data;
      unsigned long len;

      if (a->item[i].type!=T_ARRAY ||
	  (b=a->item[i].u.array)->size!=3 ||
	  b->item[0].type!=T_STRING ||
	  b->item[1].type!=T_STRING ||
	  b->item[0].u.string->len!=4)
	 error("Image.PNG._decode: Illegal stuff in array index %d\n",i);

      data=(unsigned char*)b->item[1].u.string->str;
      len=(unsigned long)b->item[1].u.string->len;

      switch (int_from_32bit((unsigned char*)b->item[0].u.string->str))
      {
	 /* ------ major chunks ------------ */
         case 0x49484452: /* IHDR */
	    /* header info */
	    if (b->item[1].u.string->len!=13)
	       error("Image.PNG._decode: illegal header (IHDR chunk)\n");

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

	    push_string(b->item[1].u.string);
	    b->item[1].u.string->refs++;
	    push_object(clone_object(image_colortable_program,1));

	    if (ihdr.type==3)
	    {
	       ct=(struct neo_colortable*)
		  get_storage(sp[-1].u.object,image_colortable_program);
	       push_string(param_palette);
	       param_palette->refs++;
	       mapping_insert(m,sp-1,sp-2);
	    }
	    else
	    {
	       push_string(param_spalette);
	       param_spalette->refs++;
	       mapping_insert(m,sp-1,sp-2);
	    }
	    pop_n_elems(2);
	    break;

         case 0x49444154: /* IDAT */
	    /* compressed image data. push, n++ */
	    if (ihdr.compression!=0)
	       free_mapping(m);

	    push_string(b->item[1].u.string);
	    b->item[1].u.string->refs++;
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

         case 0x73424954: /* sBIT */
	    break;

         case 0x624b4744: /* bKGD */
	    break;

         case 0x68495354: /* hIST */
	    break;

         case 0x74524e53: /* tRNS */
	    break;

         case 0x70485973: /* pHYs */
	    break;

         case 0x74494d45: /* tIME */
	    break;

         case 0x7a455874: /* tEXt */
	    break;

         case 0x7a545874: /* zTXt */
	    break;
      }
   }

   /* on stack: mapping   n×string */

   /* IDAT stuff on stack, now */
   f_add(n);

   if (ihdr.type==-1)
   {
      error("Image.PNG._decode: missing header (IHDR chunk)\n");
   }
   if (ihdr.type==3 && !ct)
   {
      error("Image.PNG._decode: missing palette (PLTE chunk)\n");
   }
   
   if (ihdr.compression==0)
   {
      png_decompress(ihdr.compression);
      if (sp[-1].type!=T_STRING)
	 error("Image.PNG._decode: got wierd stuff from decompression\n");
   }
   else
      error("Image.PNG._decode: illegal compression type 0x%02x\n",
	    ihdr.compression);

   fs=sp[-1].u.string; 
   push_int(-1);
   mapping_insert(m,sp-2,sp-1);

   pop_n_elems(2);

   s=(unsigned char*)fs->str;

   w1=d1=malloc(sizeof(rgb_group)*ihdr.width*ihdr.height);
   if (!d1)
      error("Image.PNG._decode: Out of memory\n");

   wa1=da1=malloc(sizeof(rgb_group)*ihdr.width*ihdr.height);
   if (!da1)
   {
      free(d1);
      error("Image.PNG._decode: Out of memory\n");
   }

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
			     ihdr.type,ihdr.bpp,fs->str,
			     fs->len,
			     ihdr.width,
			     ihdr.width*ihdr.height,
			     ct))
	 {
	    free(wa1);
	    wa1=NULL;
	 }
	 pop_stack();
	 break;

      case 1: /* adam7 */

	 /* need arena */
	 t1=malloc(sizeof(rgb_group)*ihdr.width*ihdr.height);
	 if (!t1)
	 {
	    if (wa1) free(wa1); free(w1); 
	    error("Image.PNG->_decode: out of memory (close one)\n");
	 }

	 /* loop over adam7 interlace's 
	    and write them to the arena */

	 s0=(unsigned char*)fs->str;
	 for (i=0; i<7; i++)
	 {
	    struct pike_string *ds;

	    ds=_png_unfilter(s0,fs->len-(s0-(unsigned char*)fs->str),
			     (ihdr.width+adam7[i].xd-1-adam7[i].x0)/
			     adam7[i].xd,			     
			     (ihdr.height+adam7[i].yd-1-adam7[i].y0)/
			     adam7[i].yd,
			     ihdr.filter,ihdr.type,ihdr.bpp,
			     &s0);

	    push_string(ds);
	    if (!_png_write_rgb(w1,wa1,ihdr.type,ihdr.bpp,ds->str,ds->len,
				(ihdr.width+adam7[i].xd-1-adam7[i].x0)/
				adam7[i].xd,
				(ihdr.width+adam7[i].xd-1-adam7[i].x0)/
				adam7[i].xd*
				(ihdr.height+adam7[i].yd-1-adam7[i].y0)/
				adam7[i].yd,
				ct))
	    {
	       if (wa1) free(wa1);
	       wa1=NULL;
	    }
	    d1=w1;
	    for (y=adam7[i].y0;y<ihdr.height;y+=adam7[i].yd)
	       for (x=adam7[i].x0;x<ihdr.width;x+=adam7[i].xd)
		  t1[x+y*ihdr.width]=*(d1++);

	    pop_stack();
	 }
	 
	 free(w1);
	 if (wa1) free(wa1);
	 w1=t1;

	 break;
      default:
	 free(w1); if (wa1) free(wa1);
	 error("Image.PNG._decode: Unknown interlace type\n");
   }


   
   
   /* --- done, store in mapping --- */

   push_string(param_image);
   param_image->refs++;
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
      push_string(param_alpha);
      param_image->refs++;
      push_object(clone_object(image_program,0));
      img=(struct image*)get_storage(sp[-1].u.object,image_program);
      if (img->img) free(img->img); /* protect from memleak */
      img->xsize=ihdr.width;
      img->ysize=ihdr.height;
      img->img=wa1;
      mapping_insert(m,sp-2,sp-1);
      pop_n_elems(2);
   }

   push_string(param_type); param_type->refs++;
   push_int(ihdr.type);
   mapping_insert(m,sp-2,sp-1);
   pop_n_elems(2);
   push_string(param_bpp); param_bpp->refs++;
   push_int(ihdr.bpp);
   mapping_insert(m,sp-2,sp-1);
   pop_n_elems(2);

   push_int(-1);
   map_delete(m,sp-1);
   pop_stack();
}


/*
**! method string encode(object image)
**! method string encode(object image, mapping options)
**! 	Encodes a PNG image. 
**!
**!     The <tt>options</tt> argument may be a mapping
**!	containing zero or more encoding options:
**!
**!	<pre>
**!	normal options:
**!	    "quality":0..100
**!		Set quality of result. Default is 75.
**!	    "optimize":0|1
**!		Optimize Huffman table. Default is on (1) for
**!		images smaller than 50kpixels.
**!	    "progressive":0|1
**!		Make a progressive JPEG. Default is off.
**!
**!	advanced options:
**!	    "smooth":1..100
**!		Smooth input. Value is strength.
**!	    "method":JPEG.IFAST|JPEG.ISLOW|JPEG.FLOAT|JPEG.DEFAULT|JPEG.FASTEST
**!		DCT method to use.
**!		DEFAULT and FASTEST is from the jpeg library,
**!		probably ISLOW and IFAST respective.
**!
**!	wizard options:
**!	    "baseline":0|1
**!		Force baseline output. Useful for quality&lt;20.
**!	</pre>
**!
**! note
**!	Please read some about JPEG files. A quality 
**!	setting of 100 does not mean the result is 
**!	lossless.
*/

static void image_png_encode(INT32 args)
{
}

/*
**! method object decode(string data)
**! method object decode(string data, mapping options)
**! 	Decodes a PNG image. 
**!
**!     The <tt>options</tt> argument may be a mapping
**!	containing zero or more encoding options:
**!
**!	<pre>
**!	</pre>
**!
**! note
**!	Throws upon error in data.
*/

void f_index(INT32 args);

static void image_png_decode(INT32 args)
{
   if (!args)
      error("Image.PNG.decode: missing argument(s)\n");
   
   image_png__decode(args);
   push_string(make_shared_string("image"));
   f_index(2);
}

/*** module init & exit & stuff *****************************************/

void exit_image_png(void)
{
   free_string(param_palette);
   free_string(param_spalette);
   free_string(param_image);
   free_string(param_alpha);
   free_string(param_bpp);
   free_string(param_type);
}

struct object *init_image_png(void)
{
   start_new_program();

   push_string(make_shared_string("Gz"));
   push_int(0);
   SAFE_APPLY_MASTER("resolv",2);
   if (sp[-1].type==T_OBJECT) 
   {
      push_string(make_shared_string("deflate"));
      f_index(2);
      gz_deflate=program_from_svalue(sp-1);
   }
   pop_n_elems(1);

   push_string(make_shared_string("Gz"));
   push_int(0);
   SAFE_APPLY_MASTER("resolv",2);
   if (sp[-1].type==T_OBJECT) 
   {
      push_string(make_shared_string("inflate"));
      f_index(2);
      gz_inflate=program_from_svalue(sp-1);
   }
   pop_n_elems(1);

   push_string(make_shared_string("Gz"));
   push_int(0);
   SAFE_APPLY_MASTER("resolv",2);
   if (sp[-1].type==T_OBJECT) 
   {
      push_string(make_shared_string("crc32"));
      f_index(2);
      gz_crc32=sp[-1];
      sp--;
   }
   else gz_crc32.type=T_INT;
   pop_n_elems(1);

   if (gz_deflate && gz_inflate &&
       gz_crc32.type!=T_INT)
   {
      add_function("_chunk",image_png__chunk,
		   "function(string,string:string)",
		   OPT_TRY_OPTIMIZE);
      add_function("__decode",image_png___decode,
		   "function(string:array)",
		   OPT_TRY_OPTIMIZE);

      if (gz_deflate)
      {
	 add_function("_decode",image_png__decode,
		      "function(array|string,void|mapping(string:int):object)",0);
	 add_function("decode",image_png_decode,
		      "function(string,void|mapping(string:int):object)",0);
      }
      add_function("encode",image_png_encode,
		   "function(object,void|mapping(string:int):string)",
		   OPT_TRY_OPTIMIZE);
   }

   param_palette=make_shared_string("palette");
   param_spalette=make_shared_string("spalette");
   param_image=make_shared_string("image");
   param_alpha=make_shared_string("alpha");
   param_bpp=make_shared_string("bpp");
   param_type=make_shared_string("type");
   
   return clone_object(end_program(),0);
}
