/* $Id: pnm.c,v 1.1 1997/11/02 03:42:38 mirar Exp $ */

/*
**! module Image
**! note
**!	$Id: pnm.c,v 1.1 1997/11/02 03:42:38 mirar Exp $
**! submodule PNM
**!
**!	This submodule keep the PNM encode/decode capabilities
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
**! see also: Image, Image.image, Image.GIF
*/

#include <math.h>
#include <ctype.h>

#include "stralloc.h"
#include "global.h"
RCSID("$Id: pnm.c,v 1.1 1997/11/02 03:42:38 mirar Exp $");
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "threads.h"
#include "array.h"
#include "error.h"


#include "image.h"
#include "builtin_functions.h"

extern struct program *image_colortable_program;
extern struct program *image_program;


/* internal read foo */

static INLINE unsigned char getnext(struct pike_string *s,INT32 *pos)
{
   if (*pos>=s->len) return 0;
   if (s->str[(*pos)]=='#')
      for (;*pos<s->len && ISSPACE(s->str[*pos]);(*pos)++);
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
	  ( ISSPACE(s->str[*pos]) ||
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
   INT32 pos=0,x,y,i,n;
   struct object *o;
   struct image *new;
   rgb_group *d;

   struct pike_string *s;

   if (args<1 ||
       sp[-args].type!=T_STRING)
      error("Image.PNM.decode(): Illegal arguments\n");

   s=sp[-args].u.string;

   skipwhite(s,&pos);
   if (getnext(s,&pos)!='P') 
      error("Image.PNM.decode(): given string is not a pnm file\n"); 
   type=getnext(s,&pos);
   if (type<'1'||type>'6')
      error("Image.PNM.decode(): given pnm data has illegal or unknown type\n"); 
   x=getnextnum(s,&pos);
   y=getnextnum(s,&pos);
   if (x<=0||y<=0) 
      error("Image.PNM.decode(): given pnm data has illegal size\n"); 
   if (type=='3'||type=='2'||type=='6'||type=='5')
      maxval=getnextnum(s,&pos);

   push_int(x);
   push_int(y);

   o=clone_object(image_program,2);
   new=(struct image*)get_storage(o,image_program);
   if (!new) 
      error("Image.PNM.decode(): cloned image object isn't an image (internal)\n");

   if (type=='1'||type=='2'||type=='3')
   {
     skipwhite(s,&pos);
   }
   else
   {
     skip_to_eol(s,&pos);
     pos++;
   }
   d=new->img;
   n=x*y;
   i=0;

   while (n--)
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
	    break;
	 case '5':
	    c=getnext(s,&pos);
	    d->r=d->g=d->b=
	       (unsigned char)((c*255L)/maxval);
	    break;
	 case '6':
	    d->r=(unsigned char)((getnext(s,&pos)*255L)/maxval);
	    d->g=(unsigned char)((getnext(s,&pos)*255L)/maxval);
	    d->b=(unsigned char)((getnext(s,&pos)*255L)/maxval);
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
**  method string encode_ascii(object image)
**  method string encode_P1(object image)
**  method string encode_P2(object image)
**  method string encode_P3(object image)
**  method string encode_P4(object image)
**  method string encode_P5(object image)
**! method string encode_P6(object image)
**  method string encode_P7(object image)
**!	Make a complete PNM file from an image.
**!
**!	<ref>encode_binary</ref>() and <ref>encode_ascii</ref>()
**!	uses the most optimized encoding for this image (bitmap, grey
**!	or truecolor) - P4, P5 or P6 respective P1, P2 or P3.
**!
**!	<ref>encode</ref>() maps to <ref>encode_binary</ref>().
**! 	
**! see also: decode
**!
**! returns the encoded image as a string
**!
**! bugs
**!	Currently only supports type P5 (binary grey) and
**!	P6 (binary truecolor). 
*/

void img_pnm_encode_P6(INT32 args)
{
   char buf[80];
   struct pike_string *a,*b;
   struct image *img;

   if (args<1 ||
       sp[-args].type!=T_OBJECT ||
       !(img=(struct image*)get_storage(sp[-args].u.object,image_program)))
      error("Image.PNM.encode_P6(): Illegal arguments\n");
   if (!img->img)
      error("Image.PNM.encode_P6(): Given image is empty\n");

   sprintf(buf,"P6\n%d %d\n255\n",img->xsize,img->ysize);
   a=make_shared_string(buf);
   if (sizeof(rgb_group)==3)
      b=make_shared_binary_string((char*)img->img,
				  img->xsize*img->ysize*3);
   else
   {
      char *c;
      int n=img->xsize*img->ysize;
      rgb_group *s=img->img;
      b=begin_shared_string(n*3);
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


void img_pnm_encode_binary(INT32 args)
{
   img_pnm_encode_P6(args);
}


struct program *image_pnm_module_program=NULL;

void init_image_pnm(void)
{
   start_new_program();
   
   add_function("encode",img_pnm_encode_binary,
		"function(object:string)",0);
   add_function("encode_binary",img_pnm_encode_binary,
		"function(object:string)",0);
   add_function("encode_P6",img_pnm_encode_P6,
		"function(object:string)",0);

   add_function("decode",img_pnm_decode,
		"function(string:object)",0);

   image_pnm_module_program=end_program();
   push_object(clone_object(image_pnm_module_program,0));
   add_constant(make_shared_string("PNM"),sp-1,0);
   pop_stack();
}

void exit_image_pnm(void)
{
  if(image_pnm_module_program)
  {
    free_program(image_pnm_module_program);
    image_pnm_module_program=0;
  }
}
