/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/*
**! module Image
**! submodule PNM
**!
**!	This submodule keeps the PNM encode/decode capabilities
**!	of the <ref>Image</ref> module.
**!
**!	PNM is a common image storage format on unix systems,
**!	and is a very simple format.
**!
**!	This format doesn't use any color palette.
**!
**!	The format is divided into seven subformats;
**!
**!	<pre>
**!	P1(PBM) - ascii bitmap (only two colors)
**!	P2(PGM) - ascii greymap (only grey levels)
**!	P3(PPM) - ascii truecolor
**!	P4(PBM) - binary bitmap 
**!	P5(PGM) - binary greymap 
**!	P6(PPM) - binary truecolor
** 	P7 - binary truecolor (used by xv for thumbnails)
**!	</pre>
**!
**!	Simple encoding:<br>
**!	<ref>encode</ref>,<br> <ref>encode_binary</ref>,<br>
**!	<ref>encode_ascii</ref>
**!
**!	Simple decoding:<br>
**!	<ref>decode</ref>
**!
**!	Advanced encoding:<br>
**!	<ref>encode_P1</ref>, <br>
**!	<ref>encode_P2</ref>, <br>
**!	<ref>encode_P3</ref>, <br>
**!	<ref>encode_P4</ref>, <br>
**!	<ref>encode_P5</ref>, <br>
**!	<ref>encode_P6</ref>
**!
**! see also: Image, Image.Image, Image.GIF
*/
#include "global.h"

#include <math.h>
#include <ctype.h>

#include "stralloc.h"
RCSID("$Id$");
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "threads.h"
#include "array.h"
#include "pike_error.h"
#include "operators.h"

#include "image.h"
#include "builtin_functions.h"

#include "encodings.h"


#define sp Pike_sp

extern struct program *image_colortable_program;
extern struct program *image_program;


/* internal read foo */

static INLINE unsigned char getnext(struct pike_string *s,INT32 *pos)
{
   if (*pos>=s->len) return 0;
   if (s->str[(*pos)]=='#')
      for (;*pos<s->len && ISSPACE(((unsigned char *)(s->str))[*pos]);(*pos)++);
   return s->str[(*pos)++];
}

static INLINE void skip_to_eol(struct pike_string *s,INT32 *pos)
{
   for (;*pos<s->len && s->str[*pos]!=10;(*pos)++);
}

static INLINE unsigned char getnext_skip_comment(struct pike_string *s,INT32 *pos)
{
   unsigned char c;
   while ((c=getnext(s,pos))=='#')
      skip_to_eol(s,pos);
   return c;
}

static INLINE void skipwhite(struct pike_string *s,INT32 *pos)
{
   while (*pos<s->len && 
	  ( ISSPACE(((unsigned char *)(s->str))[*pos]) ||
	    s->str[*pos]=='#'))
      getnext_skip_comment(s,pos);
}

static INLINE INT32 getnextnum(struct pike_string *s,INT32 *pos)
{
   INT32 i;
   skipwhite(s,pos);
   i=0;
   while (*pos<s->len &&
	  s->str[*pos]>='0' && s->str[*pos]<='9')
   {
      i=(i*10)+s->str[*pos]-'0';
      getnext(s,pos);
   }
   return i;
}

/*
**! method object decode(string data)
**!	Decodes PNM (PBM/PGM/PPM) data and creates an image object.
**! 	
**! see also: encode
**!
**! note
**!	This function may throw errors upon illegal PNM data.
**!
**! returns the decoded image as an image object
*/

void img_pnm_decode(INT32 args)
{
   INT32 type,c=0,maxval=255;
   INT32 pos=0,x,y,i,n,nx;
   struct object *o;
   struct image *new;
   rgb_group *d;

   struct pike_string *s;

   if (args<1 ||
       sp[-args].type!=T_STRING)
      Pike_error("Image.PNM.decode(): Illegal arguments\n");

   s=sp[-args].u.string;

   skipwhite(s,&pos);
   if (getnext(s,&pos)!='P') 
      Pike_error("Image.PNM.decode(): given string is not a pnm file\n"); 
   type=getnext(s,&pos);
   if (type<'1'||type>'6')
      Pike_error("Image.PNM.decode(): given pnm data has illegal or unknown type\n"); 
   x=getnextnum(s,&pos);
   y=getnextnum(s,&pos);
   if (x<=0||y<=0) 
      Pike_error("Image.PNM.decode(): given pnm data has illegal size\n"); 
   if (type=='3'||type=='2'||type=='6'||type=='5')
      maxval=getnextnum(s,&pos);

   push_int(x);
   push_int(y);

   o=clone_object(image_program,2);
   new=(struct image*)get_storage(o,image_program);
   if (!new) 
      Pike_error("Image.PNM.decode(): cloned image object isn't an image (internal)\n");

   if (type=='1'||type=='2'||type=='3')
   {
     skipwhite(s,&pos);
   }
   else
   {
/*     skip_to_eol(s,&pos); */ 
/* just skip one char:

      - Whitespace is not allowed in the pixels area, and only a
         single  character of whitespace (typically a newline) is
         allowed after the maxval.

   [man ppm]
*/
     pos++;
   }
   d=new->img;
   n=x*y;
   i=0;

   nx=x;

   if (type=='6' && maxval==255 && sizeof(rgb_group)==3)  /* optimize */
   {
      if (pos<s->len)
	 MEMCPY(d,s->str+pos,MINIMUM(n*3,s->len-pos));
   }
   else while (n--)
   {
      switch (type)
      {
	 case '1':
	    c=getnextnum(s,&pos);
	    d->r=d->g=d->b=
	       (unsigned char)~(c*255);
	    break;
	 case '2':
	    c=getnextnum(s,&pos);
	    d->r=d->g=d->b=
	       (unsigned char)((c*255L)/maxval);
	    break;
	 case '3':
	    d->r=(unsigned char)((getnextnum(s,&pos)*255L)/maxval);
	    d->g=(unsigned char)((getnextnum(s,&pos)*255L)/maxval);
	    d->b=(unsigned char)((getnextnum(s,&pos)*255L)/maxval);
	    break;
	 case '4':
	    if (!i) c=getnext(s,&pos),i=8;
	    d->r=d->g=d->b=
	       (unsigned char)~(((c>>7)&1)*255);
	    c<<=1;
	    i--;
	    if (!--nx) { i=0; nx=x; } /* pbm; last byte on row is padded */
	    break;
	 case '5':
	    c=getnext(s,&pos);
	    d->r=d->g=d->b=
	       (unsigned char)((c*255L)/maxval);
	    break;
	 case '6':
	    d->r=(unsigned char)((((INT32)getnext(s,&pos))*255L)/maxval);
	    d->g=(unsigned char)((((INT32)getnext(s,&pos))*255L)/maxval);
	    d->b=(unsigned char)((((INT32)getnext(s,&pos))*255L)/maxval);
	    break;
      }
      d++;
   }
   pop_n_elems(args);
   push_object(o);
}

/*
**! method string encode(object image)
**! method string encode_binary(object image)
**! method string encode_ascii(object image)
**! method string encode_P1(object image)
**! method string encode_P2(object image)
**! method string encode_P3(object image)
**! method string encode_P4(object image)
**! method string encode_P5(object image)
**! method string encode_P6(object image)
**  method string encode_P7(object image)
**!	Make a complete PNM file from an image.
**!
**!	<ref>encode_binary</ref>() and <ref>encode_ascii</ref>()
**!	uses the most optimized encoding for this image (bitmap, grey
**!	or truecolor) - P4, P5 or P6 respective P1, P2 or P3.
**!
**!	<ref>encode_P1</ref>/<ref>encode_P4</ref> 
**!	assumes the image is black and white. Use 
**!	<ref>Image.Image->threshold</ref>() or something like
**!	<tt><ref>Image.Colortable</ref>( ({({0,0,0}),({255,255,255})}) )<wbr>->floyd_steinberg()<wbr>->map(my_image)</tt> 
**!	to get a black and white image.
**!
**!	<ref>encode_P2</ref>/<ref>encode_P5</ref> assumes the image is greyscale. Use
**!	<ref>Image.Image->grey</ref>() to get a greyscale image.
**!
**! see also: decode
**!
**! returns the encoded image as a string
**!
**! note
**!	<ref>encode</ref>() is equal to <ref>encode_binary</ref>(),
**!	but may change in a future release.
*/

void img_pnm_encode_P1(INT32 args) /* ascii PBM */
{
   char buf[80];
   struct pike_string *a,*b;
   struct image *img = NULL;
   unsigned char *c;
   int x,y;
   rgb_group *s;

   if (args<1 ||
       sp[-args].type!=T_OBJECT ||
       !(img=(struct image*)get_storage(sp[-args].u.object,image_program)))
      Pike_error("Image.PNM.encode_P1(): Illegal arguments\n");
   if (!img->img)
      Pike_error("Image.PNM.encode_P1(): Given image is empty\n");

   sprintf(buf,"P1\n%d %d\n",img->xsize,img->ysize);
   a=make_shared_string(buf);

   y=img->ysize;
   s=img->img;
   c=(unsigned char*)((b=begin_shared_string((img->xsize*2)*
					     img->ysize))->str);
   if (img->xsize)
   while (y--)
   {
      x=img->xsize;
      while (x--)
      {
	 *(c++)=48+!(s->r|s->g|s->b);
	 *(c++)=' ';
	 s++;
      }
      *(c-1)='\n';
   }
   b=end_shared_string(b);

   pop_n_elems(args);
   push_string(add_shared_strings(a,b));
   free_string(a);
   free_string(b);
}

void img_pnm_encode_P2(INT32 args) /* ascii PGM */
{
   char buf[80];
   struct image *img = NULL;
   int y,x;
   rgb_group *s;
   int n;
   struct object *o = NULL;

   if (args<1 ||
       sp[-args].type!=T_OBJECT ||
       !(img=(struct image*)get_storage((o=sp[-args].u.object),image_program)))
      Pike_error("Image.PNM.encode_P2(): Illegal arguments\n");
   if (!img->img)
      Pike_error("Image.PNM.encode_P2(): Given image is empty\n");

   add_ref(o);
   pop_n_elems(args);

   sprintf(buf,"P2\n%d %d\n255\n",img->xsize,img->ysize);
   push_string(make_shared_string(buf));
   n=1;

   y=img->ysize;
   s=img->img;
   while (y--)
   {
      x=img->xsize;
      while (x--)
      {
	 sprintf(buf,"%d%c",(s->r+s->g*2+s->b)/4,x?' ':'\n');
	 push_string(make_shared_string(buf));
	 n++;
	 if (n>32) { f_add(n); n=1; }
	 s++;
      }
   }
   f_add(n);
   free_object(o);
}

void img_pnm_encode_P3(INT32 args) /* ascii PPM */
{
   char buf[80];
   struct image *img = NULL;
   int y,x;
   rgb_group *s;
   int n;
   struct object *o = NULL;

   if (args<1 ||
       sp[-args].type!=T_OBJECT ||
       !(img=(struct image*)get_storage((o=sp[-args].u.object),image_program)))
      Pike_error("Image.PNM.encode_P3(): Illegal arguments\n");
   if (!img->img)
      Pike_error("Image.PNM.encode_P3(): Given image is empty\n");

   add_ref(o);
   pop_n_elems(args);

   sprintf(buf,"P3\n%d %d\n255\n",img->xsize,img->ysize);
   push_string(make_shared_string(buf));
   n=1;

   y=img->ysize;
   s=img->img;
   while (y--)
   {
      x=img->xsize;
      while (x--)
      {
	 sprintf(buf,"%d %d %d%c",s->r,s->g,s->b,x?' ':'\n');
	 push_string(make_shared_string(buf));
	 n++;
	 if (n>32) { f_add(n); n=1; }
	 s++;
      }
   }
   f_add(n);
   free_object(o);
}

void img_pnm_encode_P4(INT32 args) /* binary PBM */
{
   char buf[80];
   struct pike_string *a,*b;
   struct image *img=NULL;
   unsigned char *c;
   int y,x,bit;
   rgb_group *s;

   if (args<1 ||
       sp[-args].type!=T_OBJECT ||
       !(img=(struct image*)get_storage(sp[-args].u.object,image_program)))
      Pike_error("Image.PNM.encode_P4(): Illegal arguments\n");
   if (!img->img)
      Pike_error("Image.PNM.encode_P4(): Given image is empty\n");

   sprintf(buf,"P4\n%d %d\n",img->xsize,img->ysize);
   a=make_shared_string(buf);

   y=img->ysize;
   s=img->img;

   c=(unsigned char*)((b=begin_shared_string(((img->xsize+7)>>3)*
					     img->ysize))->str);
   if (img->xsize)
   while (y--)
   {
      x=img->xsize;
      bit=128;
      *c=0;
      while (x--)
      {
	 *c|=bit*!(s->r|s->g|s->b);
	 if (!(bit>>=1)) { *(++c)=0; bit=128; }
	 s++;
      }
      if (bit!=128) ++c;
   }
   b=end_shared_string(b);

   pop_n_elems(args);
   push_string(add_shared_strings(a,b));
   free_string(a);
   free_string(b);
}

void img_pnm_encode_P5(INT32 args) /* binary PGM */
{
   char buf[80];
   struct pike_string *a,*b;
   struct image *img=NULL;
   unsigned char *c;
   int n;
   rgb_group *s;

   if (args<1 ||
       sp[-args].type!=T_OBJECT ||
       !(img=(struct image*)get_storage(sp[-args].u.object,image_program)))
      Pike_error("Image.PNM.encode_P5(): Illegal arguments\n");
   if (!img->img)
      Pike_error("Image.PNM.encode_P5(): Given image is empty\n");

   sprintf(buf,"P5\n%d %d\n255\n",img->xsize,img->ysize);
   a=make_shared_string(buf);

   n=img->xsize*img->ysize;
   s=img->img;
   c=(unsigned char*)((b=begin_shared_string(n))->str);
   while (n--)
   {
      *(c++)=(s->r+s->g*2+s->b)/4;
      s++;
   }
   b=end_shared_string(b);

   pop_n_elems(args);
   push_string(add_shared_strings(a,b));
   free_string(a);
   free_string(b);
}

void img_pnm_encode_P6(INT32 args)
{
   char buf[80];
   struct pike_string *a,*b;
   struct image *img=NULL;

   if (args<1 ||
       sp[-args].type!=T_OBJECT ||
       !(img=(struct image*)get_storage(sp[-args].u.object,image_program)))
      Pike_error("Image.PNM.encode_P6(): Illegal arguments\n");
   if (!img->img)
      Pike_error("Image.PNM.encode_P6(): Given image is empty\n");

   sprintf(buf,"P6\n%d %d\n255\n",img->xsize,img->ysize);
   a=make_shared_string(buf);
   if (sizeof(rgb_group)==3)
      b=make_shared_binary_string((char*)img->img,
				  img->xsize*img->ysize*3);
   else
   {
      unsigned char *c;
      int n=img->xsize*img->ysize;
      rgb_group *s=img->img;
      c=(unsigned char*)((b=begin_shared_string(n*3))->str);
      while (n--)
      {
	 *(c++)=s->r;
	 *(c++)=s->g;
	 *(c++)=s->b;
	 s++;
      }
      b=end_shared_string(b);
   }
   pop_n_elems(args);
   push_string(add_shared_strings(a,b));
   free_string(a);
   free_string(b);
}


void img_pnm_encode_ascii(INT32 args)
{
   struct image *img=NULL;
   rgb_group *s;
   int n;
   void (*func)(INT32);

   if (args<1 ||
       sp[-args].type!=T_OBJECT ||
       !(img=(struct image*)get_storage(sp[-args].u.object,image_program)))
      Pike_error("Image.PNM.encode_ascii(): Illegal arguments\n");
   if (!img->img)
      Pike_error("Image.PNM.encode_ascii(): Given image is empty\n");

   func=img_pnm_encode_P1; /* PBM */
   n=img->xsize*img->ysize;
   s=img->img;
   while (n--)
   {
      if (s->r!=s->g || s->g!=s->b)
      {
	 func=img_pnm_encode_P3; 
	 break;
      }
      else if ((s->r!=0 && s->r!=255) ||
	       (s->g!=0 && s->g!=255) ||
	       (s->b!=0 && s->b!=255))
	 func=img_pnm_encode_P2; 
      s++;
   }

   (*func)(args);
}

void img_pnm_encode_binary(INT32 args)
{
   struct image *img=NULL;
   rgb_group *s;
   int n;
   void (*func)(INT32);

   if (args<1 ||
       sp[-args].type!=T_OBJECT ||
       !(img=(struct image*)get_storage(sp[-args].u.object,image_program)))
      Pike_error("Image.PNM.encode_binary(): Illegal arguments\n");
   if (!img->img)
      Pike_error("Image.PNM.encode_binary(): Given image is empty\n");

   func=img_pnm_encode_P4; /* PBM */
   n=img->xsize*img->ysize;
   s=img->img;
   while (n--)
   {
      if (s->r!=s->g || s->g!=s->b)
      {
	 func=img_pnm_encode_P6; 
	 break;
      }
      else if ((s->r!=0 && s->r!=255) ||
	       (s->g!=0 && s->g!=255) ||
	       (s->b!=0 && s->b!=255))
	 func=img_pnm_encode_P5; 
      s++;
   }

   (*func)(args);
}


struct program *image_pnm_module_program=NULL;

void init_image_pnm(void)
{
   add_function("encode",img_pnm_encode_binary,
		"function(object:string)",0);
   add_function("encode_binary",img_pnm_encode_binary,
		"function(object:string)",0);
   add_function("encode_ascii",img_pnm_encode_ascii,
		"function(object:string)",0);

   add_function("encode_P1",img_pnm_encode_P1,
		"function(object:string)",0);
   add_function("encode_P2",img_pnm_encode_P2,
		"function(object:string)",0);
   add_function("encode_P3",img_pnm_encode_P3,
		"function(object:string)",0);

   add_function("encode_P4",img_pnm_encode_P4,
		"function(object:string)",0);
   add_function("encode_P5",img_pnm_encode_P5,
		"function(object:string)",0);
   add_function("encode_P6",img_pnm_encode_P6,
		"function(object:string)",0);

   add_function("decode",img_pnm_decode,
		"function(string:object)",0);
}

void exit_image_pnm(void)
{
}
