/* $Id: x.c,v 1.19 1998/04/24 13:50:18 mirar Exp $ */

/*
**! module Image
**! class image
*/

#include "global.h"

#include <math.h>
#include <ctype.h>

#include "stralloc.h"
#include "global.h"
RCSID("$Id: x.c,v 1.19 1998/04/24 13:50:18 mirar Exp $");
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "threads.h"
#include "array.h"
#include "error.h"
#include "port.h"

#include "image.h"
#include "colortable.h"
#include "builtin_functions.h"

extern struct program *image_colortable_program;

#ifdef THIS
#undef THIS
#endif
#define THIS ((struct image *)(fp->current_storage))
#define THISOBJ (fp->current_object)

/*
**! method string cast(string type)
**! returns the image data as a string ("rgbrgbrgb...")
**! see also: Image.colortable
**! bugs
**!	always casts to string...
*/

void image_cast(INT32 args)
{
  /* CHECK TYPE TO CAST TO HERE! FIXME FIXME FIXME! */
  pop_n_elems(args);
  push_string(make_shared_binary_string((char *)THIS->img,
					THIS->xsize*THIS->ysize
					*sizeof(rgb_group)));
}

void image_to8bit(INT32 args) /* compat function */
{
  struct neo_colortable *nct;
  struct object *o;
  struct image *this = THIS;
  struct pike_string *res = begin_shared_string((this->xsize*this->ysize));
  unsigned long i;
  rgb_group *s;
  unsigned char *d;

  if(!res) error("Out of memory\n");

  o = clone_object(image_colortable_program, args);
  nct=(struct neo_colortable *)get_storage(o, image_colortable_program);

  i=this->xsize*this->ysize;
  s=this->img;
  d=(unsigned char *)res->str;

  THREADS_ALLOW();

  image_colortable_index_8bit_image(nct,this->img,d,
				    this->xsize*this->ysize,this->xsize);

  THREADS_DISALLOW();

  free_object(o);

  pop_n_elems(args);
  push_string(end_shared_string(res));
}

void image_tozbgr(INT32 args)
{
   unsigned char *d;
   rgb_group *s;
   unsigned long i;
   struct pike_string *sres = begin_shared_string(THIS->xsize*THIS->ysize*4);
   
   if (!THIS->img) error("no image\n");
   pop_n_elems(args);

   i=THIS->ysize*THIS->xsize;
   s=THIS->img;
   d=(unsigned char *)sres->str;

   THREADS_ALLOW();
   while (i--)
   {
      *(d++)=0;
      *(d++)=s->b;
      *(d++)=s->g;
      *(d++)=s->r;
      s++;
   }
   THREADS_DISALLOW();

   push_string(end_shared_string(sres));
}

void image_torgb(INT32 args)
{
   if (!THIS->img) error("no image\n");
   pop_n_elems(args);

   push_string(make_shared_binary_string((char*)THIS->img,
        THIS->xsize*THIS->ysize*sizeof(rgb_group)));
}

void image_to8bit_rgbcube(INT32 args)
/*

  ->to8bit_rgbcube(int red,int green,int blue,
                [string map])
  
  gives r+red*g+red*green*b       
 */
{
  struct pike_string *res = begin_shared_string((THIS->xsize*THIS->ysize));
  unsigned long i;
  rgb_group *s;
  unsigned char *d;
  unsigned char *map=NULL;

  int red,green,blue,redgreen,redgreenblue,hred,hgreen,hblue;
  
  if(!res) error("Out of memory\n");

  if (!THIS->img) 
     error("No image\n");

  if (args<3) 
     error("Too few arguments to image->to8bit_rgbcube()\n");
  
  if (sp[-args].type!=T_INT
      || sp[1-args].type!=T_INT
      || sp[2-args].type!=T_INT) 
     error("Illegal argument(s) to image->to8bit_rgbcube()\n");

  red=sp[-args].u.integer; 	hred=red/2;
  green=sp[1-args].u.integer;	hgreen=green/2;
  blue=sp[2-args].u.integer; 	hblue=blue/2;
  redgreen=red*green;
  redgreenblue=red*green*blue;

  if (args>3) 
     if (sp[3-args].type!=T_STRING)
	error("Illegal argument 4 to image->to8bit_rgbcube()"
	      " (expected string or no argument)\n");
     else if (sp[3-args].u.string->len<red*green*blue)
	error("map string is not long enough to image->to8bit_rgbcube()\n");
     else
	map=(unsigned char *)sp[3-args].u.string->str;

  i=THIS->xsize*THIS->ysize;
  s=THIS->img;
  d=(unsigned char *)res->str;

  THREADS_ALLOW();
  if (!map)
     while (i--)
     {
	*(d++)=
	   (unsigned char)( ((s->r*red+hred)>>8)+
			    ((s->g*green+hgreen)>>8)*red+
			    ((s->b*blue+hblue)>>8)*redgreen );
	s++;
     }
  else
     while (i--)
     {
	*(d++)=
	   map[ ((s->r*red+hred)>>8)+
	        ((s->g*green+hgreen)>>8)*red+
	        ((s->b*blue+hblue)>>8)*redgreen ];
	s++;
     }
  THREADS_DISALLOW();

  pop_n_elems(args);
  push_string(end_shared_string(res));
}

void image_to8bit_rgbcube_rdither(INT32 args)
/*
  ->to8bit_rgbcube_rdither(int red,int green,int blue,
                [string map])
  
  gives r+red*g+red*green*b       
 */
{
  struct pike_string *res = begin_shared_string((THIS->xsize*THIS->ysize));
  unsigned long i;
  rgb_group *s;
  unsigned char *d;
  unsigned char *map=NULL;

  int red,green,blue,redgreen,redgreenblue,rmax,gmax,bmax;
  
  if(!res) error("Out of memory\n");

  if (!THIS->img) 
     error("No image\n");

  if (args<3) 
     error("Too few arguments to image->to8bit_rgbcube()\n");
  
  if (sp[-args].type!=T_INT
      || sp[1-args].type!=T_INT
      || sp[2-args].type!=T_INT) 
     error("Illegal argument(s) to image->to8bit_rgbcube()\n");

  red=sp[-args].u.integer; 	rmax=red*255;
  green=sp[1-args].u.integer;	gmax=green*255;
  blue=sp[2-args].u.integer; 	bmax=blue*255;
  redgreen=red*green;
  redgreenblue=red*green*blue;

  if (args>3) {
     if (sp[3-args].type!=T_STRING)
	error("Illegal argument 4 to image->to8bit_rgbcube()"
	      " (expected string or no argument)\n");
     else if (sp[3-args].u.string->len<red*green*blue)
	error("map string is not long enough to image->to8bit_rgbcube()\n");
     else
	map=(unsigned char *)sp[3-args].u.string->str;
  }
  i=THIS->xsize*THIS->ysize;
  s=THIS->img;
  d=(unsigned char *)res->str;

  THREADS_ALLOW();
  if (!map)
     while (i--)
     {
       unsigned int tal = my_rand();

       int r=(s->r*red)+(tal&255);
       int g=(s->g*green)+((tal>>8)&255);
       int b=(s->b*blue+((tal>>16)&255));

       if(r>rmax) r=rmax; if(g>gmax) g=gmax; if(b>bmax) b=bmax;
       *(d++)= (unsigned char)((r>>8)+(g>>8)*red+(b>>8)*redgreen);
       s++;
     }
  else
     while (i--)
     {
       unsigned int tal = my_rand();

       int r=(s->r*red)+(tal&255);
       int g=(s->g*green)+((tal>>8)&255);
       int b=(s->b*blue+((tal>>16)&255));

       if(r>rmax) r=rmax; if(g>gmax) g=gmax; if(b>bmax) b=bmax;
       *(d++)= map[ (r>>8)+(g>>8)*red+(b>>8)*redgreen ];
       s++;
     }
  THREADS_DISALLOW();

  pop_n_elems(args);
  push_string(end_shared_string(res));
}

void image_tobitmap(INT32 args)
{
   int xs;
   int i,j,left,bit,dbits;
   struct pike_string *res;
   unsigned char *d;
   rgb_group *s;

   pop_n_elems(args);
   if (!THIS->img) error("No image.\n");

   xs=(THIS->xsize+7)>>3;

   res=begin_shared_string(xs*THIS->ysize);
   d=(unsigned char *)res->str;

   s=THIS->img;

   j=THIS->ysize;
   while (j--)
   {
      i=THIS->xsize;
      while (i)
      {
	 left=8;
	 bit=1;
	 dbits=0;
	 while (left-- && i)
	 {
	    if (s->r||s->g||s->b) dbits|=bit;
	    bit<<=1;
	    s++;
	    i--;
	 }
	 *(d++)=(unsigned char)dbits;
      }
   }

   push_string(end_shared_string(res));
}

