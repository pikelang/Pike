/* $Id: bmp.c,v 1.10 1999/05/02 20:15:37 mirar Exp $ */

/*
**! module Image
**! note
**!	$Id: bmp.c,v 1.10 1999/05/02 20:15:37 mirar Exp $
**! submodule BMP
**!
**!	This submodule keeps the BMP (Windows Bitmap)
**!	encode/decode capabilities of the <ref>Image</ref> module.
**!
**!	BMP is common in the Windows environment.
**!
**!	Simple encoding:<br>
**!	<ref>encode</ref>
**!
**! see also: Image, Image.Image, Image.Colortable
*/
#include "global.h"

#include <math.h>
#include <ctype.h>

#include "stralloc.h"
RCSID("$Id: bmp.c,v 1.10 1999/05/02 20:15:37 mirar Exp $");
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


#include "image.h"
#include "colortable.h"

#include "builtin_functions.h"

extern void f_add(INT32 args);

extern struct program *image_colortable_program;
extern struct program *image_program;

static INLINE void push_ubo_32bit(unsigned long x)
{
   char buf[4];

   buf[0]=(char)(x);
   buf[1]=(char)(x>>8);
   buf[2]=(char)(x>>16);
   buf[3]=(char)(x>>24);
   push_string(make_shared_binary_string(buf,4));
}

static INLINE void push_ubo_16bit(unsigned long x)
{
   char buf[2];
   buf[0]=(char)(x);
   buf[1]=(char)(x>>8);
   push_string(make_shared_binary_string(buf,2));
}

static INLINE unsigned long int_from_32bit(unsigned char *data)
{
   return (data[0])|(data[1]<<8)|(data[2]<<16)|(data[3]<<24);
}

#define int_from_16bit(X) _int_from_16bit((unsigned char*)(X))
static INLINE unsigned long _int_from_16bit(unsigned char *data)
{
   return (data[0])|(data[1]<<8);

}

/*
**! method string encode(object image)
**! method string encode(object image,object colortable)
**  method string encode(object image,mapping options)
**!	Make a BMP. It default to a 24 bpp BMP file,
**!	but if a colortable is given, it will be 8bpp 
**!	with a palette entry.
**!
** 	<tt>option</tt> is a mapping that may contain:
** 	<pre>
** 	"colortable": Image.Colortable   - palette
** 	"bpp":        1|4|8|24           - force this many bits per pixel
** 	"windows":    0|1                - windows mode (default is 1)
** 	"rle":        0|1                - run-length encode (default is 0)
** 	</pre>
**!
**! arg object image
**!	Source image.
**! arg object colortable
**!	Colortable object, shortcut for "colortable" in options.
**!
**! see also: decode
**!
**! returns the encoded image as a string
**!
**! bugs
**!	Doesn't support all BMP modes. At all.
*/

void img_bmp_encode(INT32 args)
{
   struct object *o=NULL,*oc=NULL;
   struct image *img=NULL;
   struct neo_colortable *nct=NULL;
   int n=0,bpp=0;
   int size,offs;
   struct pike_string *ps; 

   if (!args)
      SIMPLE_TOO_FEW_ARGS_ERROR("Image.BMP.encode",1);

   if (sp[-args].type!=T_OBJECT ||
       !(img=(struct image*)get_storage(o=sp[-args].u.object,image_program)))
      SIMPLE_BAD_ARG_ERROR("Image.BMP.encode",1,"image object");

   if (args==1)
      nct=NULL,oc=NULL;
   else
      if (sp[-args].type!=T_OBJECT ||
	  !(nct=(struct neo_colortable*)
	    get_storage(oc=sp[1-args].u.object,image_colortable_program)))
	 SIMPLE_BAD_ARG_ERROR("Image.BMP.encode",2,"colortable object");

   if (!nct) 
      bpp=24;
   else if (image_colortable_size(nct)<=256)
      bpp=8; /* only supports this for now */
   else
      SIMPLE_BAD_ARG_ERROR("Image.BMP.encode",2,
			   "colortable object with max 256 colors");

   if (oc) oc->refs++;
   o->refs++;
   pop_n_elems(args);

   apply(o,"mirrory",0);
   free_object(o);
   if (sp[-1].type!=T_OBJECT ||
       !(img=(struct image*)get_storage(o=sp[-1].u.object,image_program)))
      error("Image.BMP.encode: wierd result from ->mirrory()\n");
   if (nct) push_object(oc);

   /* bitmapinfo */
   push_ubo_32bit(40); /* size of info structure */
   push_ubo_32bit(img->xsize); /* width */
   push_ubo_32bit(img->ysize); /* height */
   push_ubo_16bit(1);  /* "number of planes for the target device" */
   push_ubo_16bit(bpp); /* bits per pixel (see above) */
   push_ubo_32bit(0); /* compression: none */
   push_ubo_32bit(0); /* size of image (0 is valid if no compression) */
   push_ubo_32bit(0); /* horisontal resolution in pixels/meter */
   push_ubo_32bit(0); /* vertical resolution in pixels/meter */

   if (nct) /* size of colortable */
      push_ubo_32bit(image_colortable_size(nct));
   else
      push_ubo_32bit(0);
   
   push_ubo_32bit(0); /* number of important colors or zero */
   n=11;

   /* palette */

   if (nct)
   {
      ps=begin_shared_string((1<<bpp)*4);
      MEMSET(ps->str,0,(1<<bpp)*4);
      image_colortable_write_bgrz(nct,(unsigned char *)ps->str);
      push_string(end_shared_string(ps));
      n++;
   }

   f_add(n);
   n=1;
   /* the image */
   offs=sp[-1].u.string->len;

   if (nct)
   {
      /* 8bpp image */
      ps=begin_shared_string(img->xsize*img->ysize);
      image_colortable_index_8bit_image(nct,img->img,(unsigned char *)ps->str,
					img->xsize*img->ysize,img->xsize);
      push_string(ps=end_shared_string(ps));
      n++;
   }
   else
   {
      unsigned char *c;
      int m=img->xsize*img->ysize;
      rgb_group *s=img->img;
      c=(unsigned char*)((ps=begin_shared_string(m*3))->str);
      while (m--)
      {
	 *(c++)=s->b;
	 *(c++)=s->g;
	 *(c++)=s->r;
	 s++;
      }
      push_string(end_shared_string(ps));
      n++;
   }

   /* on stack:      source (nct) info*n image */
   
   /* done */
   f_add(n);

   /* on stack:      source (nct) info+image */

   /* "bitmapfileheader" */
   /* this is last, since we need to know the size of the file here */
   
   size=sp[-1].u.string->len;

   push_text("BM");         /* "BM" */
   push_ubo_32bit(size+14); /* "size of file" */
   push_ubo_16bit(0);       /* reserved */
   push_ubo_16bit(0);       /* reserved */
   push_ubo_32bit(offs+14);    /* offset to bitmap data */

   f_add(5);
   
   /* on stack:      source (nct) info+image header */

   stack_swap();  /* source (nct) header info+image */
   f_add(2);      /* source (nct) header+info+image */

   if (nct)
   {
      stack_swap();
      pop_stack();
   }
   stack_swap();
   pop_stack();  /* get rid of colortable & source objects */
}

/*
**! method object decode(string data)
**! method mapping _decode(string data)
**! method mapping decode_header(string data)
**! method object decode(string data,mapping options)
**! method mapping _decode(string data,mapping options)
**! method mapping decode_header(string data,mapping options)
**!	Decode a BMP. 
**!
**!	<ref>decode</ref> gives an image object,
**!	<ref>_decode</ref> gives a mapping in the format
**!	<pre>
**!		"type":"image/bmp",
**!		"image":image object,
**!		"colortable":colortable object (if applicable)
**!
**!		"xsize":int,
**!		"ysize":int,
**!		"compression":int,
**!		"bpp":int,
**!		"windows":int,
**!	</pre>
**!
**! see also: encode
**!
**! returns the encoded image as a string
**!
**! bugs
**!	Doesn't support all BMP modes. At all.
*/

static int parameter_int(struct svalue *map,struct pike_string *what,INT32 *p)
{
   struct svalue *v;
   v=low_mapping_string_lookup(map->u.mapping,what);

   if (!v || v->type!=T_INT) return 0;

   *p=v->u.integer;
   return 1;
}

void i_img_bmp__decode(INT32 args,int header_only)
{
   p_wchar0 *s,*os;
   int len,olen;
   int xsize=0,ysize=0,bpp=0,comp=0;
   struct image *img=NULL;
   struct neo_colortable *nct=NULL;
   struct object *o;
   rgb_group *d;
   int n=0,j,i,y,skip;
   int windows=0;
   int quality=50; /* for JPEG decoding */

   if (args<1)
      SIMPLE_TOO_FEW_ARGS_ERROR("Image.BMP.decode",1);

   if (sp[-args].type!=T_STRING)
      SIMPLE_BAD_ARG_ERROR("Image.BMP.decode",1,"string");
   if (sp[-args].u.string->size_shift)
      SIMPLE_BAD_ARG_ERROR("Image.BMP.decode",1,"8 bit string");

   if (args>1)
      if (sp[1-args].type!=T_MAPPING)
	 SIMPLE_BAD_ARG_ERROR("Image.BMP.decode",2,"mapping");
      else
      {
	 struct pike_string *qs;
	 MAKE_CONSTANT_SHARED_STRING(qs,"quality");
	 parameter_int(sp+1-args,qs,&quality);
      }

   os=s=(p_wchar0*)sp[-args].u.string->str;
   len=sp[-args].u.string->len;
   olen=len;

   pop_n_elems(args-1);
   
   if (len<20)
      error("Image.BMP.decode: not a BMP (file to short)\n");

   if (s[0]!='B' || s[1]!='M')
      error("Image.BMP.decode: not a BMP (illegal header)\n");

   /* ignore stupid info like file size */

   switch (int_from_32bit(s+14))
   {
      case 40: /* windows mode */
      case 68: /* fuji jpeg mode */

	 if (len<54)
	    error("Image.BMP.decode: unexpected EOF in header (at byte %d)\n",
		  len);

	 push_text("xsize");
	 push_int(xsize=int_from_32bit(s+14+4*1));
	 n++;

	 push_text("ysize");
	 push_int(ysize=int_from_32bit(s+14+4*2));
	 n++;

	 push_text("target_planes");
	 push_int(int_from_16bit(s+14+4*3));
	 n++;

	 push_text("bpp");
	 push_int(bpp=int_from_16bit(s+14+4*3+2));
	 n++;

	 push_text("compression");
	 push_int(comp=int_from_32bit(s+14+4*4));
	 n++;

	 push_text("xres");
	 push_int(int_from_32bit(s+14+4*5));
	 n++;

	 push_text("yres");
	 push_int(int_from_32bit(s+14+4*6));
	 n++;
	 
	 windows=1;

	 switch (int_from_32bit(s+14))
	 {
	    case 40:
	       push_text("windows");
	       push_int(1);
	       break;
	    case 68:
	       push_text("fuji");
	       push_int(1);
	       break;
	 }

	 n++;

	 s+=54;     /* fuji mode doesn't care */
	 len-=54;

	 break;

      case 12: /* dos (?) mode */

	 if (len<54)
	    error("Image.BMP.decode: unexpected EOF in header (at byte %d)\n",
		  len);

	 push_text("xsize");
	 push_int(xsize=int_from_16bit(s+14+4));
	 n++;

	 push_text("ysize");
	 push_int(ysize=int_from_16bit(s+14+6));
	 n++;

	 push_text("target_planes");
	 push_int(int_from_16bit(s+14+8));
	 n++;

	 push_text("bpp");
	 push_int(bpp=int_from_16bit(s+14+10));
	 n++;

	 push_text("compression");
	 push_int(comp=0);
	 n++;

	 s+=26;
	 len-=26;

	 break;

      default:
	 error("Image.BMP.decode: not a known BMP type "
	       "(illegal info size %ld, expected 68, 40 or 12)\n",
	       int_from_32bit(s+14));
   }

   if (header_only)
   {
      f_aggregate_mapping(n*2);
      stack_swap();
      pop_stack();
      return;
   }

   if (comp==1195724874) /* "JPEG" */
   {
      int n=int_from_32bit(os+10);

      push_text("image");

      if (olen-n<0)
	 error("Image.BMP.decode: unexpected EOF in JFIF data\n");

      push_text("Image");
      push_int(0);
      SAFE_APPLY_MASTER("resolv",2);
      push_text("JPEG");
      f_index(2);
      push_text("decode");
      f_index(2);

      push_string(make_shared_binary_string(os+n,olen-n));

      push_text("quant_tables");

      push_text("Image");
      push_int(0);
      SAFE_APPLY_MASTER("resolv",2);
      push_text("JPEG");
      f_index(2);
      push_text("quant_tables");
      f_index(2);
      push_int(quality);
      f_call_function(2);

      f_aggregate_mapping(2);

      f_call_function(3);

      n++;

      goto final;
   }
   
   if (bpp!=24) /* get palette */
   {
      push_text("colortable");

      if (windows)
      {
	 if ((4<<bpp)>len)
	    error("Image.BMP.decode: unexpected EOF in palette\n");

	 push_string(make_shared_binary_string(s,(4<<bpp)));
	 push_int(2);
	 push_object(o=clone_object(image_colortable_program,2));
	 nct=(struct neo_colortable*)get_storage(o,image_colortable_program);

	 s+=(4<<bpp);
	 len-=(4<<bpp);
      }
      else
      {
	 if ((3<<bpp)>len)
	    error("Image.BMP.decode: unexpected EOF in palette\n");

	 push_string(make_shared_binary_string(s,(3<<bpp)));
	 push_int(1);
	 push_object(o=clone_object(image_colortable_program,2));
	 nct=(struct neo_colortable*)get_storage(o,image_colortable_program);

	 s+=(3<<bpp);
	 len-=(3<<bpp);
      }
   }

   push_text("image");

   push_int(xsize);
   push_int(ysize);
   push_object(o=clone_object(image_program,2));
   img=(struct image*)get_storage(o,image_program);
   n++;

   if (int_from_32bit(os+10)) 
   {
      s=os+int_from_32bit(os+10);
      len=olen-int_from_32bit(os+10);
   }

   if (len>0) switch (bpp)
   {
      case 24:
	 if (comp)
	    error("Image.BMP.decode: can't handle compressed 24bpp BMP\n");

	 skip=(4-((img->xsize*3)&3))&3;

	 j=(len)/3;
	 y=img->ysize;
	 while (j && y--)
	 {
	    d=img->img+img->xsize*y;
	    i=img->xsize; if (i>j) i=j; 
	    j-=i;
	    while (i--)
	    {
	       d->b=*(s++);
	       d->g=*(s++);
	       d->r=*(s++);
	       d++;
	    }
	    if (j>=skip) { j-=skip; s+=skip; }
	 }
	 break;
      case 8:
	 if (comp != 0 && comp != 1 )
	    error("Image.BMP.decode: can't handle compression %d for 8bpp BMP\n",comp);

	 if (comp==1)
	 {
	    int i;
	    rgb_group *maxd=img->img+img->xsize*img->ysize;


	    /* 00 00        -	end of line
	       00 01        -	end of data
	       00 02 dx dy  -	move cursor
	       00  x (x bytes) 00   -  x bytes of data
	        x  a        -   x bytes of rle data (a a ...)
	    */

	    y=img->ysize-1;
	    d=img->img+img->xsize*y;
	    
	    while (j--)
	    {
	       if (!j--) break;
	       switch (*s)
	       {
		  case 0:
		     switch (s[1])
		     {
			case 0:
			   if (y!=0) y--;
			   d=img->img+img->xsize*y;
			   break;
			case 1:
			   goto done_rle8;
			case 2:
			   error("Image.BMP.decode: advanced RLE "
				 "decompression (cursor movement) "
				 "is unimplemented (please send this "
				 "image to mirar@idonex.se)\n");
			default:
			   error("Image.BMP.decode: advanced RLE "
				 "decompression (non-rle data) "
				 "is unimplemented (please send this "
				 "image to mirar@idonex.se)\n");
		     }
		     break;
		  default:
		     for (i=0; i<s[0] && d<maxd; i++)
			if (s[1]>nct->u.flat.numentries) 
			   d++;
			else 
			   *(d++)=nct->u.flat.entries[s[1]].color;
		     break;
	       }
	       s+=2;
	    }
   done_rle8:
	 }
	 else
	 {
	    skip=(4-(img->xsize&3))&3;
	    j=len;
	    y=img->ysize;
	    while (j && y--)
	    {
	       d=img->img+img->xsize*y;
	       i=img->xsize; if (i>j) i=j; 
	       j-=i;
	       while (i--)
		  *(d++)=nct->u.flat.entries[*(s++)].color;
	       if (j>=skip) { j-=skip; s+=skip; }
	    }
	 }
	 break;
      case 4:
	 if (comp != 0 && comp != 2 )
	    error("Image.BMP.decode: can't handle compression %d for 4bpp BMP\n",comp);

	 if (comp == 2) /* RLE4 */
	 {
	    int i;
	    rgb_group *maxd=img->img+img->xsize*img->ysize;
	    y=img->ysize-1;
	    d=img->img+img->xsize*y;

	    /* 00 00        -	end of line
	       00 01        -	end of data
	       00 02 dx dy  -	move cursor
	       00  x ((x+1)/2 bytes) 00   -  x nibbles of data
	        x ab        -   x nibbles of rle data (a b a b a ...)
	    */

	    j=len;
	    
	    while (j--)
	    {
	       if (!j--) break;
	       switch (*s)
	       {
		  case 0:
		     switch (s[1])
		     {
			case 0:
#if RLE_DEBUG
			   fprintf(stderr,"end of line  %5d %02x %02x\n",
				   j,s[0],s[1]);
#endif
			   if (y>0) y--;
			   d=img->img+img->xsize*y;
			   
			   break;
			case 1:
#if RLE_DEBUG
			   fprintf(stderr,"end of data  %5d %02x %02x\n",
				   j,s[0],s[1]);
#endif
			   goto done_rle4;
			case 2:
			   error("Image.BMP.decode: advanced RLE "
				 "decompression (cursor movement) "
				 "is unimplemented (please send this "
				 "image to mirar@idonex.se)\n");
#if RLE_DEBUG
			   if (j<2) break;
			   fprintf(stderr,"cursor       "
				   "%5d %02x %02x %02x %02x\n",
				   j,s[0],s[1],s[2],s[3]);
			   j-=2; s+=2;
#endif
			   break;
			default:
			   error("Image.BMP.decode: advanced RLE "
				 "decompression (non-rle data) "
				 "is unimplemented (please send this "
				 "image to mirar@idonex.se)\n");
#if RLE_DEBUG
			   fprintf(stderr,"data         %5d %02x %02x [ ",
				   j,s[0],s[1]);
			   fflush(stderr);
			   
			   for (i=0; i<(s[1]+1)/2; i++)
			      fprintf(stderr,"%02x ",s[2+i]);
			   fprintf(stderr,"] %02x\n",s[2+i]);

			   j-=2; s+=2;
			   j-=i; s+=i;
#endif 

			   break;
		     }
		     break;
		  default:
#if RLE_DEBUG		     
		     fprintf(stderr,"rle data     %02x %02x\n",s[0],s[1]);		     
#endif
		     for (i=0; i<s[0] && d<maxd; i++)
		     {
			int c;
			if (i&1)
			   c=s[1]&15;
			else
			   c=(s[1]>>4)&15;

			if (c>nct->u.flat.numentries) 
			   d++;
			else 
			   *(d++)=nct->u.flat.entries[c].color;
		     }
		     break;
	       }
	       s+=2;
	    }
   done_rle4:
	 }
	 else
	 {
	    skip=(4-(((img->xsize+1)/2)&3))&3;
	    j=len;
	    y=img->ysize;
	    while (j && y--)
	    {
	       d=img->img+img->xsize*y;
	       i=img->xsize;
	       while (i && j--)
	       {
		  if (i) *(d++)=nct->u.flat.entries[(s[0]>>4)&15].color,i--;
		  if (i) *(d++)=nct->u.flat.entries[s[0]&15].color,i--;
		  s++;
	       }
	       if (j>=skip) { j-=skip; s+=skip; }
	    }
	 }
	 break;
      case 1:
	 if (comp)
	    error("Image.BMP.decode: can't handle compressed 1bpp BMP\n");

	 skip=(4-(((img->xsize+7)/8)&3))&3;
	 j=len;
	 y=img->ysize;
	 while (j && y--)
	 {
	    d=img->img+img->xsize*y;
	    i=img->xsize;
	    while (i && j--)
	    {
	       if (i) *(d++)=nct->u.flat.entries[!!(s[0]&128)].color,i--;
	       if (i) *(d++)=nct->u.flat.entries[!!(s[0]&64)].color,i--;
	       if (i) *(d++)=nct->u.flat.entries[!!(s[0]&32)].color,i--;
	       if (i) *(d++)=nct->u.flat.entries[!!(s[0]&16)].color,i--;
	       if (i) *(d++)=nct->u.flat.entries[!!(s[0]&8)].color,i--;
	       if (i) *(d++)=nct->u.flat.entries[!!(s[0]&4)].color,i--;
	       if (i) *(d++)=nct->u.flat.entries[!!(s[0]&2)].color,i--;
	       if (i) *(d++)=nct->u.flat.entries[s[0]&1].color,i--;
	       s++;
	    }
	    if (j>=skip) { j-=skip; s+=skip; }
	 }
	 break;
      default:
	 error("Image.BMP.decode: unknown bpp: %d\n",bpp);
   }

final:

   f_aggregate_mapping(n*2);
   stack_swap();
   pop_stack();
}


void img_bmp__decode(INT32 args)
{
   i_img_bmp__decode(args,0);
}

void img_bmp_decode_header(INT32 args)
{
   i_img_bmp__decode(args,1);
}

void img_bmp_decode(INT32 args)
{
   img_bmp__decode(args);
   push_text("image");
   f_index(2);
}

struct program *image_bmp_module_program=NULL;

void init_image_bmp(void)
{
   start_new_program();
   
   add_function("encode",img_bmp_encode,
		"function(object,void|object:string)",0);
   add_function("_decode",img_bmp__decode,
		"function(string,void|mapping:mapping)",0);
   add_function("decode",img_bmp_decode,
		"function(string,void|mapping:object)",0);
   add_function("decode_header",img_bmp_decode_header,
		"function(string,void|mapping:mapping)",0);

   image_bmp_module_program=end_program();
   push_object(clone_object(image_bmp_module_program,0));
   {
     struct pike_string *s=make_shared_string("BMP");
     add_constant(s,sp-1,0);
     free_string(s);
   }
   pop_stack();
}

void exit_image_bmp(void)
{
  if(image_bmp_module_program)
  {
    free_program(image_bmp_module_program);
    image_bmp_module_program=0;
  }
}
