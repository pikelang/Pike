/* $Id: gif.c,v 1.6 1997/11/02 03:44:49 mirar Exp $ */

/*
**! module Image
**! note
**!	$Id: gif.c,v 1.6 1997/11/02 03:44:49 mirar Exp $
**! submodule GIF
**!
**!	This submodule keep the GIF encode/decode capabilities
**!	of the <ref>Image</ref> module.
**!
**!	GIF is a common image storage format,
**!	usable for a limited color palette - a GIF image can 
**!	only contain as most 256 colors - and animations.
**!
**! see also: Image, Image.image, Image.colortable
*/

#include <math.h>
#include <ctype.h>

#include "stralloc.h"
#include "global.h"
RCSID("$Id: gif.c,v 1.6 1997/11/02 03:44:49 mirar Exp $");
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "threads.h"
#include "array.h"
#include "error.h"
#include "threads.h"

#include "image.h"
#include "colortable.h"
#include "builtin_functions.h"

#include "gif_lzw.h"

extern struct program *image_colortable_program;
extern struct program *image_program;

extern void f_add(INT32 args);

/*

goal:

string encode(object img);
string encode(object img,int colors);
string encode(object img,object colortable);
   Simply make a complete GIF file.

string encode_transparent(object img,object alpha);
string encode_transparent(object img,object alpha,int r,int g,int b);
string encode_transparent(object img,int colors,int r,int g,int b);
string encode_transparent(object img,int colors,object alpha,int r,int g,int b);
string encode_transparent(object img,object colortable,object alpha);
string encode_transparent(object img,object colortable,object alpha,int r,int g,int b);
string encode_transparent(object img,object colortable,int transp_index);
   Simply make a complete GIF file with transparency;
   Transparency is taken from the given alpha mask 
   (black=transparent), the color closest(!) to the given, or
   by the given index in the colortable. The following r,g,b
   values gives the possibility of setting the transparent color
   (some decoders ignore the transparency).

object decode(string data);
   Decode GIF data to one (!) image object.

object decode_alpha(string data);
   Decode GIF alpha channel to an image object.
   black marks transparent, white marks full opaque.

advanced:

string header_block(int xsize,int ysize,int numcolors);
string header_block(int xsize,int ysize,object colortable);
string header_block(int xsize,int ysize,object colortable,int background_color_index);
string end_block();
string transparency_block(int no);
string delay_block(int centiseconds);
string netscape_loop_block(void|int number_of_loops);

string image_block(int x,int y,object img,object colortable,int nolocalpalette,int transp_index);
string image_block(int x,int y,object img,object colortable,object|int alpha,int nolocalpalette);
string image_block(int x,int y,object img,object colortable,object|int alpha,int nolocalpalette,int r,int g,int b);

string _function_block(int function,string data);

array _decode(string data);
:  int xsize, int ysize, int numcol, 0|object colortable, 
:  (not in absolut order:)
:  ({ int x, int y, object image, 0|object alpha, object colortable, 
:     int transparency, int transparency_index,
:     int user_input, int disposal, int delay, 
:     string data}),
:  ({ int major, int minor, string data }),

string _encode(array data);

 */

/*
**! method string header_block(int xsize,int ysize,int numcolors);
**! method string header_block(int xsize,int ysize,object colortable);
**! method string header_block(int xsize,int ysize,object colortable,int background_color_index,int gif87a,int aspectx,int aspecty);
**! method string header_block(int xsize,int ysize,object colortable,int background_color_index,int gif87a,int aspectx,int aspecty,int r,int g,int b);
**!     This function gives back a GIF header block.
**!
**! 	Giving a colortable to this function includes a 
**!	global palette in the header block.
**!
**! returns the created header block as a string
**!
**! arg int xsize
**! arg int ysize
**! 	Size of drawing area. Usually same size as in
**!	the first (or only) render block(s).
**! arg int background_color_index
**!	This color in the palette is the background color.
**!	Background is visible if the following render block(s)
**!	doesn't fill the drawing area or are transparent.
**!	Most decoders doesn't use this value, though.
**! arg int gif87a
**!	If set, write 'GIF87a' instead of 'GIF89a' (default 0 == 89a).
**! arg int aspectx
**! arg int aspecty
**!	Aspect ratio of pixels,
**!	ranging from 4:1 to 1:4 in increments
**!	of 1/16th. Ignored by most decoders. 
**!	If any of <tt>aspectx</tt> or <tt>aspecty</tt> is zero,
**!	aspectratio information is skipped.
**! arg int r
**! arg int g
**! arg int b
**!	Add this color as the transparent color.
**!	This is the color used as transparency color in
**!	case of alpha-channel given as image object. 
**!	This increases (!) the number of colors by one.
**!
**! note
**!	This is in the advanced sector of the GIF support;
**!	please read some about how GIFs are packed.
**!
**!	This GIF encoder doesn't support different size
**!	of colors in global palette and color resolution.
*/

void image_gif_header_block(INT32 args)
{
   int xs,ys,bkgi=0,aspect=0,gif87a=0;
   struct neo_colortable *nct=NULL;
   int globalpalette;
   int numcolors;
   int bpp=0;
   char buf[20];
   struct pike_string *ps;
   rgb_group alphacolor={0,0,0};
   int alphaentry=0;

   if (args<3)
      error("Image.GIF.header_block(): too few arguments\n");
   if (sp[-args].type!=T_INT ||
       sp[1-args].type!=T_INT)
      error("Image.GIF.header_block(): illegal argument(s) 1..2 (expected int)\n");

   xs=sp[-args].u.integer;
   ys=sp[1-args].u.integer;

   if (sp[2-args].type==T_INT)
   {
      numcolors=sp[2-args].u.integer;
      if (numcolors<2) numcolors=2;
      globalpalette=0;
   } 
   else if (sp[2-args].type==T_OBJECT &&
	    (nct=(struct neo_colortable*)
	        get_storage(sp[2-args].u.object,image_colortable_program)))
   {
      numcolors=image_colortable_size(nct);
      if (numcolors<2) numcolors=2;
      globalpalette=1;
   }
   else
      error("Image.GIF.header_block(): illegal argument 3 (expected int or colortable object)\n");

   if (args>=4)
      if (sp[3-args].type!=T_INT)
	 error("Image.GIF.header_block(): illegal argument 4 (expected int)\n");
      else
	 bkgi=sp[3-args].u.integer;

   if (args>=5)
      if (sp[4-args].type!=T_INT)
	 error("Image.GIF.header_block(): illegal argument 4 (expected int)\n");
      else
	 gif87a=sp[4-args].u.integer;

   if (args>=7)
      if (sp[5-args].type!=T_INT ||
	  sp[6-args].type!=T_INT)
	 error("Image.GIF.header_block(): illegal argument(s) 5..6 (expected int)\n");
      else
	 if (sp[5-args].u.integer &&
	     sp[6-args].u.integer)
	    aspect=(64*sp[5-args].u.integer)/sp[6-args].u.integer-15;

   if (args>=10)
   {
      if (sp[7-args].type!=T_INT ||
	  sp[8-args].type!=T_INT ||
	  sp[9-args].type!=T_INT)
	 error("Image.GIF.render_block(): illegal argument 8..10 (expected int)\n");
      alphacolor.r=(unsigned char)(sp[7-args].u.integer);
      alphacolor.g=(unsigned char)(sp[8-args].u.integer);
      alphacolor.b=(unsigned char)(sp[9-args].u.integer);
      alphaentry=1;
   }

   if (numcolors+alphaentry>256)
      error("Image.GIF.header_block(): too many colors\n");

   while ((1<<bpp)<numcolors+alphaentry) bpp++;

   sprintf(buf,"GIF8%ca%c%c%c%c%c%c%c",
	   gif87a?'7':'9',
	   xs&255, (xs>>8)&255, /* width */
	   ys&255, (ys>>8)&255, /* height */
	   ((globalpalette<<7) 
	    | ((bpp-1)<<4) /* color resolution = 2^bpp */
	    | (0 <<3) /* palette is sorted, most used first */
	    | ((bpp)-1)), /* palette size = 2^bpp */
	   bkgi,
	   aspect);
   
   push_string(make_shared_binary_string(buf,13));

   if (globalpalette)
   {
      ps=begin_shared_string((1<<bpp)*3);
      image_colortable_write_rgb(nct,ps->str);
      MEMSET(ps->str+(numcolors+alphaentry)*3,0,((1<<bpp)-numcolors)*3);

      if (alphaentry) 
      {
	 ps->str[3*numcolors+0]=alphacolor.r;
	 ps->str[3*numcolors+1]=alphacolor.g;
	 ps->str[3*numcolors+2]=alphacolor.b;
      }
      /* note: same as _calculated_ 'alphaidx' 
	       in image_gif_render_block */

      push_string(end_shared_string(ps));
      f_add(2);
   }

   (ps=sp[-1].u.string)->refs++;

   pop_n_elems(args+1);
   push_string(ps);
}

/*
**! method string end_block();
**!	This function gives back a GIF end (trailer) block.
**!
**! returns the end block as a string.
**!
**! note
**!	This is in the advanced sector of the GIF support;
**!	please read some about how GIFs are packed.
**!
**!	The result of this function is always ";" or "\x3b",
**!	but I recommend using this function anyway for code clearity.
*/

void image_gif_end_block(INT32 args)
{
   pop_n_elems(args);
   push_string(make_shared_string("\x3b"));
}

/*
**! method string _gce_block(int transparency,int transparency_index,int delay,int user_input,int disposal);
**!
**!     This function gives back a Graphic Control Extension block.
**!	A GCE block has the scope of the following render block.
**!	
**! arg int transparency
**! arg int transparency_index
**!	The following image has transparency, marked with this index.
**! arg int delay
**!	View the following rendering for this many centiseconds (0..65535).
**! arg int user_input
**!	Wait the delay or until user input. If delay is zero,
**!	wait indefinitely for user input. May sound the bell
**!	upon decoding.
**! arg int disposal
**!	Disposal method number;
**!	<dl compact>
**!	<dt>0<dd>No disposal specified. The decoder is
**!	         not required to take any action.
**!	<dt>1<dd>Do not dispose. The graphic is to be left
**!	         in place.
**!     <dt>2<dd>Restore to background color. The area used by the
**!              graphic must be restored to the background color.
**!     <dt>3<dd>Restore to previous. The decoder is required to
**!              restore the area overwritten by the graphic with
**!              what was there prior to rendering the graphic.
**!     <dt compact>4-7<dd>To be defined.
**!     </dl>
**!
**! see also: _render_block, render_block
**!
**! note
**!	This is in the very advanced sector of the GIF support;
**!	please read about how GIFs file works.
**!
**!	Most decoders just ignore some or all of these parameters.
*/

void image_gif__gce_block(INT32 args)
{
   char buf[20];
   if (args<5) 
      error("Image.GIF._gce_block(): too few arguments\n");
   if (sp[-args].type!=T_INT ||
       sp[1-args].type!=T_INT ||
       sp[2-args].type!=T_INT ||
       sp[3-args].type!=T_INT ||
       sp[4-args].type!=T_INT)
      error("Image.GIF._gce_block(): Illegal argument(s)\n");
   sprintf(buf,"%c%c%c%c%c%c%c%c",
	   0x21, /* extension intruder */
	   0xf9, /* gce extension */
	   4, /* block size */
	   (((sp[4-args].u.integer & 7)<<2) /* disposal */
	    | ((!!sp[3-args].u.integer)<<1) /* user input */
	    | (!!sp[-args].u.integer)), /* transparency */
	   sp[2-args].u.integer & 255, /* delay, ls8 */
	   (sp[2-args].u.integer>>8) & 255, /* delay, ms8 */
	   sp[1-args].u.integer & 255, /* transparency index */
	   0 /* end block */
	   );
	   
   pop_n_elems(args);
   push_string(make_shared_binary_string(buf,8));
}


/*
**! method string _render_block(int x,int y,int xsize,int ysize,int bpp,string indices,0|string colortable,int interlace);
**!	Advanced (!) method for writing renderblocks for placement
**!	in a GIF file. This method only applies LZW encoding on the
**!	indices and makes the correct headers. 
**!
**! arg int x
**! arg int y
**!	Position of this image.
**! arg int xsize
**! arg int ysize
**!	Size of the image. Length if the <tt>indices</tt> string
**!	must be xsize*ysize.
**! int bpp
**!	Bits per pixels in the indices. Valid range 1..8.
**! string indices
**!	The image indices as an 8bit indices.
**! string colortable
**!	Colortable with colors to write as palette.
**!	If this argument is zero, no local colortable is written.
**!	Colortable string len must be 1<<bpp.
**! arg int interlace
**!     Interlace index data and set interlace bit. The given string
**!	should _not_ be pre-interlaced.
**!
**! see also: encode, _encode, header_block, end_block
**! 
**! note
**!	This is in the very advanced sector of the GIF support;
**!	please read about how GIFs file works.
*/

void image_gif__render_block(INT32 args)
{
   int xpos,ypos,xs,ys,bpp,interlace;
   int localpalette=0;
   struct pike_string *ips,*cps,*ps;
   char buf[20];
   struct gif_lzw lzw;
   int i;
   int numstrings=0;
   
   if (args<8)
      error("Image.GIF._render_block(): Too few arguments\n");

   if (sp[-args].type!=T_INT ||
       sp[1-args].type!=T_INT ||
       sp[2-args].type!=T_INT ||
       sp[3-args].type!=T_INT ||
       sp[4-args].type!=T_INT ||
       sp[5-args].type!=T_STRING ||
       sp[7-args].type!=T_INT)
      error("Image.GIF._render_block(): Illegal argument(s)\n");

   xpos=sp[-args].u.integer;
   ypos=sp[1-args].u.integer;
   xs=sp[2-args].u.integer;
   ys=sp[3-args].u.integer;
   bpp=sp[4-args].u.integer;
   ips=sp[5-args].u.string;
   interlace=sp[7-args].u.integer;

   if (bpp<1) bpp=1;
   else if (bpp>8) bpp=8;

   if (sp[6-args].type==T_INT)
   {
      localpalette=0;
   }
   else if (sp[6-args].type==T_STRING)
   {
      cps=sp[6-args].u.string;
      localpalette=1;
      if (cps->len!=3*(1<<bpp))
	 error("Image.GIF._render_block(): colortable string has wrong length\n");
   }
   else
      error("Image.GIF._render_block(): Illegal argument(s)\n");

   if (xs*ys!=ips->len)
      error("Image.GIF._render_block(): indices string has wrong length\n");

/*** write image rendering header */

   sprintf(buf,"%c%c%c%c%c%c%c%c%c%c",
	   0x2c, /* render block initiator */
	   xpos&255, (xpos>>8)&255, /* left position */
	   ypos&255, (ypos>>8)&255, /* top position */
	   xs&255, (xs>>8)&255, /* width */
	   ys&255, (ys>>8)&255, /* height */
	   /* packed field */
	   ((localpalette<<7) 
	    | (interlace<<6)
	    | (0 <<5) /* palette is sorted, most used first */
	    | ((bpp)-1)) /* palette size = 2^bpp */
           );
   push_string(make_shared_binary_string(buf,10)); 
   numstrings++;

/*** write local palette if needed */
   
   if (localpalette)
   {
      cps->refs++;
      push_string(cps);
      numstrings++;
   }
/*** write the image */

   /* write lzw minimum code size */
   if (bpp<2)
      sprintf(buf,"%c",2);
   else
      sprintf(buf,"%c",bpp);

   push_string(make_shared_binary_string(buf,1));
   numstrings++;
   
   image_gif_lzw_init(&lzw,bpp);
   if (lzw.broken) error("out of memory\n");

   THREADS_ALLOW();

   if (!interlace)
      image_gif_lzw_add(&lzw,ips->str,ips->len);
   else
   {
      int y;
      for (y=0; y<ys; y+=8)
         image_gif_lzw_add(&lzw,ips->str+y*xs,xs); 
      for (y=4; y<ys; y+=8)
         image_gif_lzw_add(&lzw,ips->str+y*xs,xs);
      for (y=2; y<ys; y+=4)
         image_gif_lzw_add(&lzw,ips->str+y*xs,xs);
      for (y=1; y<ys; y+=2)
         image_gif_lzw_add(&lzw,ips->str+y*xs,xs);
   }

   image_gif_lzw_finish(&lzw);
   
   THREADS_DISALLOW();

   if (lzw.broken) error("out of memory\n");

   for (i=0;;)
      if (lzw.outpos-i>=255)
      {
	 ps=begin_shared_string(256);
	 ps->str[0]=255;
	 MEMCPY(ps->str+1,lzw.out+i,255);
	 push_string(end_shared_string(ps));
	 numstrings++;
	 if (numstrings>32) /* shrink stack */
	 {
	    f_add(numstrings);
	    numstrings=1;
	 }
	 i+=255;
      }
      else
      {
	 ps=begin_shared_string(lzw.outpos-i+2);
	 ps->str[0]=lzw.outpos-i;
	 MEMCPY(ps->str+1,lzw.out+i,lzw.outpos-i);
	 ps->str[lzw.outpos-i+1]=0;
	 push_string(end_shared_string(ps));
	 numstrings++;
	 break;
      }

   image_gif_lzw_free(&lzw);

/*** done */

   f_add(numstrings);

   (ps=sp[-1].u.string)->refs++;

   pop_n_elems(args+1);
   push_string(ps);
}

/*
**! method string render_block(object img,object colortable,int x,int y,int localpalette);
**! method string render_block(object img,object colortable,int x,int y,int localpalette,int transp_index);
**! method string render_block(object img,object colortable,int x,int y,int localpalette,object alpha);
**! method string render_block(object img,object colortable,int x,int y,int localpalette,object alpha,int r,int g,int b);
**! method string render_block(object img,object colortable,int x,int y,int localpalette,int transp_index,int interlace,int delay,int user_input,int disposal);
**! method string render_block(object img,object colortable,int x,int y,int localpalette,object alpha,int r,int g,int b,int interlace,int delay,int user_input,int disposal);
**!
**!     This function gives a image block for placement in a GIF file,
**!	with or without transparency.
**!	The some options actually gives two blocks, 
**!	the first with graphic control extensions for such things
**!	as delay or transparency.
**!
**! object img
**!	The image.
**! object colortable
**!	Colortable with colors to use and to write as palette.
**! arg int x
**! arg int y
**!	Position of this image.
**! int localpalette
**!	If set, writes a local palette. 
**! int transp_index
**!	Index of the transparent color in the colortable.
**!	<tt>-1</tt> indicates no transparency.
**! object alpha
**!	Alpha channel image; black is transparent.
**! int r
**! int g
**! int b
**!	Color of transparent pixels. Not all decoders understands
**!	transparency. This is ignored if localpalette isn't set.
**! arg int delay
**!	View this image for this many centiseconds. Default is zero.
**! arg int user_input
**!	If set: wait the delay or until user input. If delay is zero,
**!	wait indefinitely for user input. May sound the bell
**!	upon decoding. Default is non-set.
**! arg int disposal
**!	Disposal method number;
**!	<dl compact>
**!	<dt>0<dd>No disposal specified. The decoder is
**!	         not required to take any action. (default)
**!	<dt>1<dd>Do not dispose. The graphic is to be left
**!	         in place.
**!     <dt>2<dd>Restore to background color. The area used by the
**!              graphic must be restored to the background color.
**!     <dt>3<dd>Restore to previous. The decoder is required to
**!              restore the area overwritten by the graphic with
**!              what was there prior to rendering the graphic.
**!     <dt compact>4-7<dd>To be defined.
**!     </dl>
**!
**! see also: encode, _encode, header_block, end_block
**! 
**! note
**!	This is in the advanced sector of the GIF support;
**!	please read some about how GIFs are packed.
**!
**!	The user_input and disposal method are unsupported
**!	in most decoders.
*/

void image_gif_render_block(INT32 args)
{
   struct image *img,*alpha=NULL;
   struct neo_colortable *nct;
   int numcolors;
   int localpalette,xpos,ypos;
   int alphaidx=-1;
   rgb_group alphacolor;
   int alphaentry=0;
   int transparency;
   int n;
   int delay=0;
   int user_input=0;
   int disposal=0;
   int numstrings=0;
   int interlace=0;
   int bpp;
   struct pike_string *ps;

   if (args<2) 
      error("Image.GIF.render_block(): Too few arguments\n");
   if (sp[-args].type!=T_OBJECT ||
       !(img=(struct image*)get_storage(sp[-args].u.object,image_program)))
      error("Image.GIF.render_block(): Illegal argument 1 (expected image object)\n");
   else if (!img->img)
      error("Image.GIF.render_block(): given image has no image\n");
   if (sp[1-args].type!=T_OBJECT ||
       !(nct=(struct neo_colortable*)
  	     get_storage(sp[1-args].u.object,image_colortable_program)))
      error("Image.GIF.render_block(): Illegal argument 2 (expected colortable object)\n");

   if (args>=4)
   {
      if (sp[2-args].type!=T_INT)
	 error("Image:GIF.render_block(): Illegal argument 3 (expected int)\n");
      if (sp[3-args].type!=T_INT)
	 error("Image:GIF.render_block(): Illegal argument 4 (expected int)\n");
      xpos=sp[2-args].u.integer;
      ypos=sp[3-args].u.integer;
   }
   else xpos=ypos=0;

   numcolors=image_colortable_size(nct);
   if (numcolors==0)
      error("Image.GIF.render_block(): no colors in colortable\n");
   else if (numcolors>256)
      error("Image.GIF.render_block(): too many colors in colortable (256 is max)\n");

   if (args>=5)
   {
      if (sp[4-args].type!=T_INT)
	 error("Image:GIF.render_block(): Illegal argument 5 (expected int)\n");
      localpalette=sp[4-args].u.integer;
   }
   else localpalette=0;
   if (args>=6)
   {
      if (sp[5-args].type==T_INT)
      {
	 alphaidx=sp[5-args].u.integer;
	 alpha=0;
	 alphaentry=0;
	 if (alphaidx!=-1 && numcolors<=alphaidx)
	    error("Image.GIF.render_block(): illegal index to transparent color\n");
	 n=6;
      }
      else if (sp[5-args].type==T_OBJECT &&
	       (alpha=(struct image*)get_storage(sp[-args].u.object,image_program)))
      {
	 if (!alpha->img)
	    error("Image.GIF.render_block(): given alpha channel has no image\n");
	 else if (alpha->xsize != img->xsize ||
		  alpha->ysize != img->ysize)
	    error("Image.GIF.render_block(): given alpha channel differ in size from given image\n");
	 alphaidx=0;
	 n=9;

	 alphacolor.r=alphacolor.g=alphacolor.b=0;
	 if (args>=9)
	 {
	    if (sp[6-args].type!=T_INT ||
		sp[7-args].type!=T_INT ||
		sp[8-args].type!=T_INT)
	       error("Image.GIF.render_block(): illegal argument 7..9 (expected int)\n");
	    alphacolor.r=(unsigned char)(sp[6-args].u.integer);
	    alphacolor.g=(unsigned char)(sp[7-args].u.integer);
	    alphacolor.b=(unsigned char)(sp[8-args].u.integer);

	    alphaidx=numcolors; 
	    /* note: same as transparent color index 
	             in image_gif_header_block */
	    alphaentry=1;
	    if (numcolors>255) 
	       error("Image.GIF.render_block(): too many colors in colortable (255 is max, need one for transparency)\n");
	 }
      }
      else
	 error("Image:GIF.render_block(): Illegal argument 6 (expected int or image object)\n");
      if (alphaidx!=-1) transparency=1; else transparency=0;

      if (args>n) /* interlace and gce arguments */
      {
	 if (args>n)
	    if (sp[n-args].type!=T_INT)
	       error("Image:GIF.render_block(): Illegal argument %d (expected int)\n",n+1);
	    else
	       interlace=!!sp[n-args].u.integer;
	 n++;
	 if (args>n)
	    if (sp[n-args].type!=T_INT)
	       error("Image:GIF.render_block(): Illegal argument %d (expected int)\n",n+1);
	    else
	       delay=sp[n-args].u.integer;
	 n++;
	 if (args>n)
	    if (sp[n-args].type!=T_INT)
	       error("Image:GIF.render_block(): Illegal argument %d (expected int)\n",n+1);
	    else
	       user_input=!!sp[n-args].u.integer;
	 n++;
	 if (args>n)
	    if (sp[n-args].type!=T_INT)
	       error("Image:GIF.render_block(): Illegal argument %d (expected int)\n",n+1);
	    else
	       disposal=sp[n-args].u.integer&7;
      }
   }
   else 
      transparency=0;

   bpp=1;
   while ((1<<bpp)<numcolors+alphaentry) bpp++;
   if (alphaentry) alphaidx=(1<<bpp)-1;
	 
/*** write GCE if needed */

   if (transparency || delay || user_input || disposal)
      /* write gce control block */
   {
      push_int(transparency);
      push_int(alphaidx);
      push_int(delay);
      push_int(user_input);
      push_int(disposal);
      image_gif__gce_block(5);
      numstrings++;
   }

   ps=begin_shared_string(img->xsize*img->ysize);
   image_colortable_index_8bit_image(nct,img->img,ps->str,
				     img->xsize*img->ysize,img->xsize);

   if (alpha)
   {
      rgb_group *a=alpha->img;
      int n=img->xsize*img->ysize;
      unsigned char *d=ps->str;
      while (n--)
      {
	 if (a->r||a->g||a->b) 
	    *d=alphaidx;
	 d++;
	 a++;
      }
   }

   ps=end_shared_string(ps);

   push_int(xpos);
   push_int(ypos);
   push_int(img->xsize);
   push_int(img->ysize);
   push_int(bpp);
   push_string(ps);
   if (localpalette)
   {
      int numcolors=image_colortable_size(nct);

      ps=begin_shared_string((1<<bpp)*3);
      image_colortable_write_rgb(nct,ps->str);
      MEMSET(ps->str+(numcolors+alphaentry)*3,0,((1<<bpp)-numcolors)*3);
      if (alphaentry) 
      {
	 ps->str[3*alphaidx+0]=alphacolor.r;
	 ps->str[3*alphaidx+1]=alphacolor.g;
	 ps->str[3*alphaidx+2]=alphacolor.b;
      }
      push_string(end_shared_string(ps));
   }
   else
      push_int(0);
   push_int(interlace);

   image_gif__render_block(8);

   numstrings++;

/*** done */

   f_add(numstrings);

   (ps=sp[-1].u.string)->refs++;

   pop_n_elems(args+1);
   push_string(ps);
}

struct program *image_gif_module_program=NULL;

void init_image_gif(void)
{
   start_new_program();
   
   add_function("render_block",image_gif_render_block,
		"function(object,object,void|int,void|int,void|int,void|object,void|int,void|int,void|int,void|int,void|int,void|int,void|int:string)"
		"|function(object,object,void|int,void|int,void|int,void|int,void|int,void|int,void|int,void|int:string)",0);
   add_function("_gce_block",image_gif__gce_block,
		"function(int,int,int,int,int:string)",0);
   add_function("_render_block",image_gif__render_block,
		"function(int,int,int,int,string,void|string,int:string)",0);
   add_function("header_block",image_gif_header_block,
		"function(int,int,int|object,void|int,void|int,void|int,void|int,void|int,void|int,void|int:string)",0);
   add_function("end_block",image_gif_end_block,
		"function(:string)",0);

   image_gif_module_program=end_program();
   push_object(clone_object(image_gif_module_program,0));
   add_constant(make_shared_string("GIF"),sp-1,0);
   pop_stack();
}

void exit_image_gif(void)
{
  if(image_gif_module_program)
  {
    free_program(image_gif_module_program);
    image_gif_module_program=0;
  }
}
