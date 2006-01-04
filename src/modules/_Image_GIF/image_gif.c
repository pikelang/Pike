/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: image_gif.c,v 1.26 2006/01/04 20:09:37 grendel Exp $
*/

/*
**! module Image
**! submodule GIF
**!
**!	This submodule keep the GIF encode/decode capabilities
**!	of the <ref>Image</ref> module.
**!
**!	GIF is a common image storage format,
**!	usable for a limited color palette - a GIF image can 
**!	only contain as most 256 colors - and animations.
**!
**!	Simple encoding:
**!	<ref>encode</ref>, <ref>encode_trans</ref>
**!
**!	Advanced stuff:
**!	<ref>render_block</ref>, <ref>header_block</ref>,
**!	<ref>end_block</ref>, <ref>netscape_loop_block</ref>
**!
**!	Very advanced stuff:
**!	<ref>_render_block</ref>, <ref>_gce_block</ref>
**!
**! see also: Image, Image.Image, Image.Colortable
*/
#include "global.h"
#include "module.h"

#include "config.h"

#ifdef WITH_GIF

#include <math.h>
#include <ctype.h>

#include "stralloc.h"
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "threads.h"
#include "array.h"
#include "pike_error.h"
#include "threads.h"

#include "../Image/image.h"
#include "../Image/colortable.h"

#include "builtin_functions.h"
#include "operators.h"
#include "mapping.h"
#include "bignum.h"
#include "module_support.h"

#include "gif_lzw.h"

#endif /* WITH_GIF */

#ifdef WITH_GIF

#define sp Pike_sp

#ifdef DYNAMIC_MODULE
static struct program *image_program = NULL;
static struct program *image_colortable_program = NULL;
static struct program *image_layer_program = NULL;

/*  #ifdef FAKE_DYNAMIC_LOAD */

/* These should really be cached in local, static variables */
#define image_lay ((void(*)(INT32))PIKE_MODULE_IMPORT(Image,image_lay))

#define image_colortable_write_rgb \
 ((void(*)(struct neo_colortable *,unsigned char *))PIKE_MODULE_IMPORT(Image,image_colortable_write_rgb))

#define image_colortable_size \
  ((ptrdiff_t(*)(struct neo_colortable *))PIKE_MODULE_IMPORT(Image,image_colortable_size))

#define image_colortable_index_8bit_image \
  ((int(*)(struct neo_colortable *,rgb_group *,unsigned char *,int,int))PIKE_MODULE_IMPORT(Image,image_colortable_index_8bit_image))

#define image_colortable_internal_floyd_steinberg \
  ((void(*)(struct neo_colortable *))PIKE_MODULE_IMPORT(Image,image_colortable_internal_floyd_steinberg))

/*  #endif */

#else
extern struct program *image_program;
extern struct program *image_colortable_program;
extern struct program *image_layer_program;

#endif /* DYNAMIC_MODULE */

enum 
{
   GIF_ILLEGAL,

   GIF_RENDER,
   GIF_EXTENSION,

   GIF_LOOSE_GCE,
   GIF_NETSCAPE_LOOP,

   GIF_ERROR_PREMATURE_EOD,
   GIF_ERROR_UNKNOWN_DATA,
   GIF_ERROR_TOO_MUCH_DATA
};

#if 0
#include <sys/resource.h>
#define CHRONO(X) chrono(X);

static void chrono(char *x)
{
   struct rusage r;
   static struct rusage rold;
   getrusage(RUSAGE_SELF,&r);
   fprintf(stderr,"%s: %ld.%06ld - %ld.%06ld\n",x,
	   (long)r.ru_utime.tv_sec,(long)r.ru_utime.tv_usec,
	   
	   (long)(((r.ru_utime.tv_usec-rold.ru_utime.tv_usec<0)?-1:0)
		  +r.ru_utime.tv_sec-rold.ru_utime.tv_sec),
	   (long)(((r.ru_utime.tv_usec-rold.ru_utime.tv_usec<0)?1000000:0)
		  + r.ru_utime.tv_usec-rold.ru_utime.tv_usec)
      );

   rold=r;
}
#else
#define CHRONO(X)
#endif

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
**!
**! see also: header_block, end_block
*/

void image_gif_header_block(INT32 args)
{
   int xs,ys,bkgi=0,aspect=0,gif87a=0;
   struct neo_colortable *nct=NULL;
   int globalpalette=0;
   ptrdiff_t numcolors=0;
   int bpp=1;
   char buf[20];
   struct pike_string *ps;
   rgb_group alphacolor={0,0,0};
   int alphaentry=0;

   if (args<3)
      Pike_error("Image.GIF.header_block(): too few arguments\n");
   if (sp[-args].type!=T_INT ||
       sp[1-args].type!=T_INT)
      Pike_error("Image.GIF.header_block(): illegal argument(s) 1..2 (expected int)\n");

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
      globalpalette=1;
   }
   else
      Pike_error("Image.GIF.header_block(): illegal argument 3 (expected int or colortable object)\n");

   if (args>=4) {
      if (sp[3-args].type!=T_INT)
	 Pike_error("Image.GIF.header_block(): illegal argument 4 (expected int)\n");
      else
	 bkgi=sp[3-args].u.integer;
   }
   if (args>=5) {
      if (sp[4-args].type!=T_INT)
	 Pike_error("Image.GIF.header_block(): illegal argument 4 (expected int)\n");
      else
	 gif87a=sp[4-args].u.integer;
   }
   if (args>=7) {
      if (sp[5-args].type!=T_INT ||
	  sp[6-args].type!=T_INT)
	 Pike_error("Image.GIF.header_block(): illegal argument(s) 5..6 (expected int)\n");
      else
	 if (sp[5-args].u.integer &&
	     sp[6-args].u.integer)
	 {
	    aspect=(64*sp[5-args].u.integer)/sp[6-args].u.integer-15;
	    if (aspect>241) aspect=241; else if (aspect<1) aspect=1;
	 }
   }
   if (args>=10)
   {
      if (sp[7-args].type!=T_INT ||
	  sp[8-args].type!=T_INT ||
	  sp[9-args].type!=T_INT)
	 Pike_error("Image.GIF.header_block(): illegal argument 8..10 (expected int)\n");
      alphacolor.r=(unsigned char)(sp[7-args].u.integer);
      alphacolor.g=(unsigned char)(sp[8-args].u.integer);
      alphacolor.b=(unsigned char)(sp[9-args].u.integer);
      alphaentry=1;
   }

   if (numcolors+alphaentry>256)
      Pike_error("Image.GIF.header_block(): too many colors (%ld%s)\n",
	    DO_NOT_WARN((long)(numcolors + alphaentry)),
	    alphaentry?" including alpha channel color":"");

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
      image_colortable_write_rgb(nct,(unsigned char *)ps->str);
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

   add_ref(ps=sp[-1].u.string);

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
**!
**! see also: header_block, end_block
*/

void image_gif_end_block(INT32 args)
{
   pop_n_elems(args);
   push_constant_text("\x3b");
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
**!	<dt>0</dt><dd>No disposal specified. The decoder is
**!	         not required to take any action.</dd>
**!	<dt>1</dt><dd>Do not dispose. The graphic is to be left
**!	         in place.</dd>
**!     <dt>2</dt><dd>Restore to background color. The area used by the
**!              graphic must be restored to the background color.</dd>
**!     <dt>3</dt><dd>Restore to previous. The decoder is required to
**!              restore the area overwritten by the graphic with
**!              what was there prior to rendering the graphic.</dd>
**!     <dt compact>4-7</dt><dd>To be defined.</dd>
**!     </dl>
**!
**! see also: _render_block, render_block
**!
**! note
**!	This is in the very advanced sector of the GIF support;
**!	please read about how GIF files works.
**!
**!	Most decoders just ignore some or all of these parameters.
*/

static void image_gif__gce_block(INT32 args)
{
   char buf[20];
   if (args<5) 
      Pike_error("Image.GIF._gce_block(): too few arguments\n");
   if (sp[-args].type!=T_INT ||
       sp[1-args].type!=T_INT ||
       sp[2-args].type!=T_INT ||
       sp[3-args].type!=T_INT ||
       sp[4-args].type!=T_INT)
      Pike_error("Image.GIF._gce_block(): Illegal argument(s)\n");
   sprintf(buf,"%c%c%c%c%c%c%c%c",
	   0x21, /* extension intruder */
	   0xf9, /* gce extension */
	   4, /* block size */
	   ((((int) sp[4-args].u.integer & 7)<<2) /* disposal */
	    | ((!!(int) sp[3-args].u.integer)<<1) /* user input */
	    | (!!(int) sp[-args].u.integer)), /* transparency */
	   (int) sp[2-args].u.integer & 255, /* delay, ls8 */
	   ((int) sp[2-args].u.integer>>8) & 255, /* delay, ms8 */
	   (int) sp[1-args].u.integer & 255, /* transparency index */
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
**! arg int bpp
**!	Bits per pixels in the indices. Valid range 1..8.
**! arg string indices
**!	The image indices as an 8bit indices.
**! arg string colortable
**!	Colortable with colors to write as palette.
**!	If this argument is zero, no local colortable is written.
**!	Colortable string len must be 1&lt;&lt;bpp.
**! arg int interlace
**!     Interlace index data and set interlace bit. The given string
**!	should _not_ be pre-interlaced.
**!
**! see also: encode, _encode, header_block, end_block
**! 
**! note
**!	This is in the very advanced sector of the GIF support;
**!	please read about how GIF files works.
*/

static void image_gif__render_block(INT32 args)
{
   int xpos,ypos,xs,ys,bpp,interlace;
   int localpalette=0;
   struct pike_string *ips,*cps=NULL,*ps;
   char buf[20];
   struct gif_lzw lzw;
   int i;
   int numstrings=0;
   
CHRONO("gif _render_block begun");

   if (args<8)
      Pike_error("Image.GIF._render_block(): Too few arguments\n");

   if (sp[-args].type!=T_INT ||
       sp[1-args].type!=T_INT ||
       sp[2-args].type!=T_INT ||
       sp[3-args].type!=T_INT ||
       sp[4-args].type!=T_INT ||
       sp[5-args].type!=T_STRING ||
       sp[7-args].type!=T_INT)
      Pike_error("Image.GIF._render_block(): Illegal argument(s)\n");

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
	 Pike_error("Image.GIF._render_block(): colortable string has wrong length\n");
   }
   else
      Pike_error("Image.GIF._render_block(): Illegal argument(s)\n");

   if (xs*ys!=ips->len)
      Pike_error("Image.GIF._render_block(): indices string has wrong length\n");

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
     ref_push_string(cps);
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
   
   image_gif_lzw_init(&lzw,(bpp<2)?2:bpp);
   if (lzw.broken) Pike_error("out of memory\n");

   THREADS_ALLOW();

   if (!interlace)
      image_gif_lzw_add(&lzw,(unsigned char *)ips->str,ips->len);
   else
   {
      int y;
      for (y=0; y<ys; y+=8)
         image_gif_lzw_add(&lzw,((unsigned char *)ips->str)+y*xs,xs); 
      for (y=4; y<ys; y+=8)
         image_gif_lzw_add(&lzw,((unsigned char *)ips->str)+y*xs,xs);
      for (y=2; y<ys; y+=4)
         image_gif_lzw_add(&lzw,((unsigned char *)ips->str)+y*xs,xs);
      for (y=1; y<ys; y+=2)
         image_gif_lzw_add(&lzw,((unsigned char *)ips->str)+y*xs,xs);
   }

   image_gif_lzw_finish(&lzw);
   
   THREADS_DISALLOW();

   if (lzw.broken) Pike_error("out of memory\n");

CHRONO("gif _render_block push of packed data begin");

   for (i=0;;)
      if (lzw.outpos-i==0)
      {
	 push_string(make_shared_binary_string("\0",1));
	 numstrings++;
         break;
      }
      else if (lzw.outpos-i>=255)
      {
	 ps=begin_shared_string(256);
	 *((unsigned char*)(ps->str))=255;
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
	 ps->str[0] = DO_NOT_WARN((char)(lzw.outpos-i));
	 MEMCPY(ps->str+1,lzw.out+i,lzw.outpos-i);
	 ps->str[lzw.outpos-i+1]=0;
	 push_string(end_shared_string(ps));
	 numstrings++;
	 break;
      }

CHRONO("gif _render_block push of packed data end");

   image_gif_lzw_free(&lzw);

/*** done */

   f_add(numstrings);

   add_ref(ps=sp[-1].u.string);

   pop_n_elems(args+1);
   push_string(ps);

CHRONO("gif _render_block end");
}

/*
**! method string render_block(object img,object colortable,int x,int y,int localpalette);
**! method string render_block(object img,object colortable,int x,int y,int localpalette,object alpha);
**! method string render_block(object img,object colortable,int x,int y,int localpalette,object alpha,int r,int g,int b);
**! method string render_block(object img,object colortable,int x,int y,int localpalette,int delay,int transp_index,int interlace,int user_input,int disposal);
**! method string render_block(object img,object colortable,int x,int y,int localpalette,object alpha,int r,int g,int b,int delay,int interlace,int user_input,int disposal);
**!
**!     This function gives a image block for placement in a GIF file,
**!	with or without transparency.
**!	The some options actually gives two blocks, 
**!	the first with graphic control extensions for such things
**!	as delay or transparency.
**!
**!	Example:
**!	<pre>
**!	img1=<ref>Image.Image</ref>([...]);
**!	img2=<ref>Image.Image</ref>([...]);
**!	[...] // make your very-nice images
**!	nct=<ref>Image.Colortable</ref>([...]); // make a nice colortable
**!	write(<ref>Image.GIF.header_block</ref>(xsize,ysize,nct)); // write a GIF header
**!	write(<ref>Image.GIF.render_block</ref>(img1,nct,0,0,0,10)); // write a render block
**!	write(<ref>Image.GIF.render_block</ref>(img2,nct,0,0,0,10)); // write a render block
**!	[...]
**!	write(<ref>Image.GIF.end_block</ref>()); // write end block
**!	// voila! A GIF animation on stdout.
**!	</pre>
**!
**!	<illustration type=image/gif>
**!	object nct=Image.Colortable(lena(),32,({({0,0,0})}));
**!	string s=Image.GIF.header_block(lena()->xsize(),lena()->ysize(),nct);
**!	foreach ( ({lena()->xsize(),
**!		    (int)(lena()->xsize()*0.75),
**!		    (int)(lena()->xsize()*0.5),
**!		    (int)(lena()->xsize()*0.25),
**!		    (int)(1),
**!		    (int)(lena()->xsize()*0.25),
**!		    (int)(lena()->xsize()*0.5),
**!		    (int)(lena()->xsize()*0.75)}),int xsize)
**!	{
**!	   object o=lena()->scale(xsize,lena()->ysize());
**!	   object p=lena()->clear(0,0,0);
**!	   p->paste(o,(lena()->xsize()-o->xsize())/2,0);
**!	   s+=Image.GIF.render_block(p,nct,0,0,0,25);
**!	}
**!	s+=Image.GIF.netscape_loop_block(200);
**!	s+=Image.GIF.end_block();
**!	return s;
**!	</illustration>The above animation is thus created:
**!	<pre>
**!	object nct=Image.Colortable(lena,32,({({0,0,0})}));
**!	string s=GIF.header_block(lena->xsize(),lena->ysize(),nct);
**!	foreach ( ({lena->xsize(),
**!		    (int)(lena->xsize()*0.75),
**!		    (int)(lena->xsize()*0.5),
**!		    (int)(lena->xsize()*0.25),
**!		    (int)(1),
**!		    (int)(lena->xsize()*0.25),
**!		    (int)(lena->xsize()*0.5),
**!		    (int)(lena->xsize()*0.75)}),int xsize)
**!	{
**!	   object o=lena->scale(xsize,lena->ysize());
**!	   object p=lena->clear(0,0,0);
**!	   p->paste(o,(lena->xsize()-o->xsize())/2,0);
**!	   s+=Image.GIF.render_block(p,nct,0,0,0,25);
**!	}
**!	s+=Image.GIF.netscape_loop_block(200);
**!	s+=Image.GIF.end_block();
**!	write(s);
**!	</pre>
**!
**! arg object img
**!	The image.
**! arg object colortable
**!	Colortable with colors to use and to write as palette.
**! arg int x
**! arg int y
**!	Position of this image.
**! arg int localpalette
**!	If set, writes a local palette. 
**! arg object alpha
**!	Alpha channel image; black is transparent.
**! arg int r
**! arg int g
**! arg int b
**!	Color of transparent pixels. Not all decoders understands
**!	transparency. This is ignored if localpalette isn't set.
**! arg int delay
**!	View this image for this many centiseconds. Default is zero.
**! arg int transp_index
**!	Index of the transparent color in the colortable.
**!	<tt>-1</tt> indicates no transparency.
**! arg int user_input
**!	If set: wait the delay or until user input. If delay is zero,
**!	wait indefinitely for user input. May sound the bell
**!	upon decoding. Default is non-set.
**! arg int disposal
**!	Disposal method number;
**!	<dl compact>
**!	<dt>0</dt><dd>No disposal specified. The decoder is
**!	         not required to take any action. (default)</dd>
**!	<dt>1</dt><dd>Do not dispose. The graphic is to be left
**!	         in place.</dd>
**!     <dt>2</dt><dd>Restore to background color. The area used by the
**!              graphic must be restored to the background color.</dd>
**!     <dt>3</dt><dd>Restore to previous. The decoder is required to
**!              restore the area overwritten by the graphic with
**!              what was there prior to rendering the graphic.</dd>
**!     <dt compact>4-7</dt><dd>To be defined.</dd>
**!     </dl>
**!
**! see also: encode, header_block, end_block
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
   struct image *img=NULL,*alpha=NULL;
   struct neo_colortable *nct=NULL;
   ptrdiff_t numcolors;
   int localpalette,xpos,ypos;
   ptrdiff_t alphaidx=-1;
   rgb_group alphacolor;
   int alphaentry=0;
   int transparency;
   int n=0;
   int delay=0;
   int user_input=0;
   int disposal=0;
   int numstrings=0;
   int interlace=0;
   int bpp;
   struct pike_string *ps;

CHRONO("gif render_block begin");

   if (args<2) 
      Pike_error("Image.GIF.render_block(): Too few arguments\n");
   if (sp[-args].type!=T_OBJECT ||
       !(img=(struct image*)get_storage(sp[-args].u.object,image_program)))
      Pike_error("Image.GIF.render_block(): Illegal argument 1 (expected image object)\n");
   else if (!img->img)
      Pike_error("Image.GIF.render_block(): given image has no image\n");
   if (sp[1-args].type!=T_OBJECT ||
       !(nct=(struct neo_colortable*)
  	     get_storage(sp[1-args].u.object,image_colortable_program)))
      Pike_error("Image.GIF.render_block(): Illegal argument 2 (expected colortable object)\n");

   if (args>=4)
   {
      if (sp[2-args].type!=T_INT)
	 Pike_error("Image:GIF.render_block(): Illegal argument 3 (expected int)\n");
      if (sp[3-args].type!=T_INT)
	 Pike_error("Image:GIF.render_block(): Illegal argument 4 (expected int)\n");
      xpos=sp[2-args].u.integer;
      ypos=sp[3-args].u.integer;
   }
   else xpos=ypos=0;

   numcolors=image_colortable_size(nct);
   if (numcolors==0)
      Pike_error("Image.GIF.render_block(): no colors in colortable\n");
   else if (numcolors>256)
      Pike_error("Image.GIF.render_block(): too many colors in given colortable: "
	    "%ld (256 is max)\n",
	    DO_NOT_WARN((long)numcolors));

   if (args>=5)
   {
      if (sp[4-args].type!=T_INT)
	 Pike_error("Image:GIF.render_block(): Illegal argument 5 (expected int)\n");
      localpalette=sp[4-args].u.integer;
   }
   else localpalette=0;
   if (args>=6)
   {
      if (sp[5-args].type==T_OBJECT &&
	  (alpha=(struct image*)get_storage(sp[5-args].u.object,image_program)))
      {
	 if (!alpha->img)
	    Pike_error("Image.GIF.render_block(): given alpha channel has no image\n");
	 else if (alpha->xsize != img->xsize ||
		  alpha->ysize != img->ysize)
	    Pike_error("Image.GIF.render_block(): given alpha channel differ in size from given image\n");
	 alphaidx=numcolors;
	 n=9;

	 alphacolor.r=alphacolor.g=alphacolor.b=0;
	 if (args>=9)
	 {
	    if (sp[6-args].type!=T_INT ||
		sp[7-args].type!=T_INT ||
		sp[8-args].type!=T_INT)
	       Pike_error("Image.GIF.render_block(): illegal argument 7..9 (expected int)\n");
	    alphacolor.r=(unsigned char)(sp[6-args].u.integer);
	    alphacolor.g=(unsigned char)(sp[7-args].u.integer);
	    alphacolor.b=(unsigned char)(sp[8-args].u.integer);

	    /* note: same as transparent color index 
	             in image_gif_header_block */
	    alphaentry=1;
	    if (numcolors>255) 
	       Pike_error("Image.GIF.render_block(): too many colors in colortable (255 is max, need one for transparency)\n");
	 }
      }
      else if (sp[5-args].type==T_INT)
      {
	 n=5;
      }
      else
	 Pike_error("Image:GIF.render_block(): Illegal argument 6 (expected int or image object)\n");
      if (alphaidx!=-1) transparency=1; else transparency=0;

      if (args>n) /* interlace and gce arguments */
      {
	 if (args>n) {
	    if (sp[n-args].type!=T_INT)
	       Pike_error("Image:GIF.render_block(): Illegal argument %d (expected int)\n",n+1);
	    else
	       delay=sp[n-args].u.integer;
	 }
	 n++;
	 if (!alpha)
	 {
	    if (args>n) {
	       if (sp[n-args].type!=T_INT)
	          Pike_error("Image:GIF.render_block(): Illegal argument %d (expected int)\n",n+1);
	       else
	       {
	          alphaidx=sp[n-args].u.integer;
	          alpha=0;
	          alphaentry=0;
	          if (alphaidx!=-1)
		  {
		     transparency=1;
		     if (numcolors<=alphaidx || alphaidx<0)
			Pike_error("Image.GIF.render_block(): illegal index to transparent color\n");
		  }
	          n=6;
	       }
	    }
	    n++;
	 }
	 if (args>n) {
	    if (sp[n-args].type!=T_INT)
	       Pike_error("Image:GIF.render_block(): Illegal argument %d (expected int)\n",n+1);
	    else
	       interlace=!!sp[n-args].u.integer;
	 }
	 n++;
	 if (args>n) {
	    if (sp[n-args].type!=T_INT)
	       Pike_error("Image:GIF.render_block(): Illegal argument %d (expected int)\n",n+1);
	    else
	       user_input=!!sp[n-args].u.integer;
	 }
	 n++;
	 if (args>n) {
	    if (sp[n-args].type!=T_INT)
	       Pike_error("Image:GIF.render_block(): Illegal argument %d (expected int)\n",n+1);
	    else
	       disposal=sp[n-args].u.integer&7;
	 }
      }
   }
   else 
      transparency=0;

   bpp=1;
   while ((1<<bpp)<numcolors+alphaentry) bpp++;
	 
/*** write GCE if needed */

   if (transparency || delay || user_input || disposal)
      /* write gce control block */
   {
      push_int(transparency);
      if (alphaidx!=-1)
	push_int64(alphaidx);
      else
	push_int(0);
      push_int(delay);
      push_int(user_input);
      push_int(disposal);
      image_gif__gce_block(5);
      numstrings++;
   }

   ps=begin_shared_string(img->xsize*img->ysize);
CHRONO("render_block index begin");
   image_colortable_index_8bit_image(nct,img->img,(unsigned char *)ps->str,
				     img->xsize*img->ysize,img->xsize);
CHRONO("render_block index end");

   if (alpha)
   {
      rgb_group *a=alpha->img;
      int n2=img->xsize*img->ysize;
      unsigned char *d=(unsigned char *)ps->str;
      while (n2--)
      {
	 if (!(a->r||a->g||a->b))
	    *d = DO_NOT_WARN((unsigned char)alphaidx);
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
CHRONO("gif render_block write local colortable begin");
      ps=begin_shared_string((1<<bpp)*3);
      image_colortable_write_rgb(nct,(unsigned char *)ps->str);
      MEMSET(ps->str+(numcolors+alphaentry)*3,0,((1<<bpp)-numcolors)*3);
      if (alphaentry) 
      {
	 ps->str[3*alphaidx+0]=alphacolor.r;
	 ps->str[3*alphaidx+1]=alphacolor.g;
	 ps->str[3*alphaidx+2]=alphacolor.b;
      }
      push_string(end_shared_string(ps));
CHRONO("gif render_block write local colortable end");
   }
   else
      push_int(0);
   push_int(interlace);

CHRONO("gif render_block _render_block begin");
   image_gif__render_block(8);
CHRONO("gif render_block _render_block end");

   numstrings++;

/*** done */

   f_add(numstrings);

   add_ref(ps=sp[-1].u.string);

   pop_n_elems(args+1);
   push_string(ps);

CHRONO("gif render_block end");
}

/*
**! method string encode(object img);
**! method string encode(object img,int colors);
**! method string encode(object img,object colortable);
**! method string encode_trans(object img,object alpha);
**! method string encode_trans(object img,int tr_r,int tr_g,int tr_b);
**! method string encode_trans(object img,int colors,object alpha);
**! method string encode_trans(object img,int colors,int tr_r,int tr_g,int tr_b);
**! method string encode_trans(object img,int colors,object alpha,int tr_r,int tr_g,int tr_b);
**! method string encode_trans(object img,object colortable,object alpha);
**! method string encode_trans(object img,object colortable,int tr_r,int tr_g,int tr_b);
**! method string encode_trans(object img,object colortable,object alpha,int a_r,int a_g,int a_b);
**! method string encode_trans(object img,object colortable,int transp_index);
**!     Create a complete GIF file.
**!
**!	The latter (<ref>encode_trans</ref>) functions 
**!	add transparency capabilities.
**!
**!	Example:
**!	<pre>
**!	img=<ref>Image.Image</ref>([...]);
**!	[...] // make your very-nice image
**!	write(<ref>Image.GIF.encode</ref>(img)); // write it as GIF on stdout 
**!	</pre>
**!
**! arg object img
**!	The image which to encode.
**! arg int colors
**! arg object colortable
**!	These arguments decides what colors the image should
**!	be encoded with. If a number is given, a colortable
**!	with be created with (at most) that amount of colors.
**!	Default is '256' (GIF maximum amount of colors).
**! arg object alpha
**!	Alpha channel image (defining what is transparent); black
**!	color indicates transparency. GIF has only transparent
**!	or nontransparent (no real alpha channel).
**!	You can always dither a transparency channel:
**!	<tt>Image.Colortable(my_alpha, ({({0,0,0}),({255,255,255})}))<wbr>
**!	    ->full()<wbr>->floyd_steinberg()<wbr>->map(my_alpha)</tt>
**! arg int tr_r
**! arg int tr_g
**! arg int tr_b
**!	Use this (or the color closest to this) color as transparent
**!	pixels.
**! arg int a_r
**! arg int a_g
**! arg int a_b
**!	Encode transparent pixels (given by alpha channel image) 
**!	to have this color. This option is for making GIFs for 
**!	the decoders that doesn't support transparency.
**! arg int transp_index
**!	Use this color no in the colortable as transparent color.
**!
**! note
**!	For advanced users:
**!	<pre>Image.GIF.encode_trans(img,colortable,alpha);</pre>
**!	is equivalent of using
**!	<pre>Image.GIF.header_block(img->xsize(),img->ysize(),colortable)+
**!	Image.GIF.render_block(img,colortable,0,0,0,alpha)+
**!	Image.GIF.end_block();</pre>
**!	and is actually implemented that way.
*/

void _image_gif_encode(INT32 args,int fs)
{
   struct image *img=NULL;
   struct object *imgobj=NULL;

   struct object *nctobj=NULL;
   struct neo_colortable *nct=NULL;

   struct object *alphaobj=NULL;
   struct image *alpha=NULL;

   rgbl_group ac={0,0,0};
   int alphaentry=0;
   int trans=0;
   ptrdiff_t tridx=0;

   int n=0;
   int arg=2;

   if (args<1)
      Pike_error("Image.GIF.encode(): Too few arguments\n");

   if (sp[-args].type!=T_OBJECT ||
       !(img=(struct image*)get_storage(imgobj=sp[-args].u.object,
					image_program)))
      Pike_error("Image.GIF.encode(): Illegal argument 1 (expected image object)\n");
   add_ref(imgobj);

   
   if (args>=2) {
      if (sp[1-args].type==T_INT)
      {
	 if (args!=4)
	 {
	    n=sp[1-args].u.integer;
	    if (n>256) n=256; if (n<2) n=2;

	    ref_push_object(imgobj);
	    push_int(n);
	    nctobj=clone_object(image_colortable_program,2);
	    nct=(struct neo_colortable*)
	       get_storage(nctobj,image_colortable_program);
	    if (!nct)
	       Pike_error("Image.GIF.encode(): Internal error; colortable isn't colortable\n");
	    arg=2;
	 }
	 else arg=1;
      }
      else if (sp[1-args].type!=T_OBJECT)
	 Pike_error("Image.GIF.encode(): Illegal argument 2 (expected image or colortable object or int)\n");
      else if ((nct=(struct neo_colortable*)
		get_storage(nctobj=sp[1-args].u.object,image_colortable_program)))
      {
	add_ref(nctobj);
      }
      else
      {
	 nctobj=NULL;
	 arg=1;
      }
   }
   /* check transparency arguments */
   if (args-arg>0) {
      if (sp[arg-args].type==T_OBJECT &&
	  (alpha=(struct image*)
	   get_storage(alphaobj=sp[arg-args].u.object,image_program)))
      {
	add_ref(alphaobj);
	 if (args-arg>1) {
	    if (args-arg<4 ||
		sp[1+arg-args].type!=T_INT ||
		sp[2+arg-args].type!=T_INT ||
		sp[3+arg-args].type!=T_INT)
	       Pike_error("Image.GIF.encode: Illegal arguments %d..%d (expected int)\n",arg+2,arg+4);
	    else
	    {
	       ac.r=sp[1+arg-args].u.integer;
	       ac.g=sp[2+arg-args].u.integer;
	       ac.b=sp[3+arg-args].u.integer;
	       alphaentry=1;
	    }
	 }
	 trans=1;
	 if (!nct) tridx=255;
	 else tridx=image_colortable_size(nct);
      }
      else if (args-arg<3 ||
	       sp[arg-args].type!=T_INT ||
	       sp[1+arg-args].type!=T_INT ||
	       sp[2+arg-args].type!=T_INT)
      {
	 if (arg==2 &&
	     sp[arg-args].type==T_INT)
	    tridx=sp[arg-args].u.integer;
	 else
	    Pike_error("Image.GIF.encode(): Illegal argument %d or %d..%d\n",
		  arg+1,arg+1,arg+3);
	 trans=1;
      }
      else
      {
	 rgb_group tr;
	 unsigned char trd;

	 /* make a colortable if we don't have one */
	 if (!nct)
	 {
	   add_ref(imgobj);
	    push_object(imgobj);
	    push_int(256);
	    nctobj=clone_object(image_colortable_program,2);
	    nct=(struct neo_colortable*)
	       get_storage(nctobj,image_colortable_program);
	    if (!nct)
	       Pike_error("Image.GIF.encode(): Internal error; colortable isn't colortable\n");
	 }
   
	 tr.r=(unsigned char)sp[arg-args].u.integer;
	 tr.g=(unsigned char)sp[1+arg-args].u.integer;
	 tr.b=(unsigned char)sp[2+arg-args].u.integer;

	 image_colortable_index_8bit_image(nct,&tr,&trd,1,1);

	 tridx=trd;
	 trans=2;
      }
   }
   /* make a colortable if we don't have one */
   if (!nct)
   {
     ref_push_object(imgobj);
      if (alpha) push_int(255); else push_int(256);
      nctobj=clone_object(image_colortable_program,2);
      nct=(struct neo_colortable*)
	 get_storage(nctobj,image_colortable_program);
      if (!nct)
	 Pike_error("Image.GIF.encode(): Internal error; colortable isn't colortable\n");
   }

   if (fs) image_colortable_internal_floyd_steinberg(nct);

   pop_n_elems(args);

   /* build header */

   push_int(img->xsize);
   push_int(img->ysize);
   ref_push_object(nctobj);
   if (!trans)
      image_gif_header_block(3);
   else if (trans==1)
   {
      push_int64(tridx); 
      push_int(0);
      push_int(0);
      push_int(0);
      push_int(ac.r);
      push_int(ac.g);
      push_int(ac.b);
      image_gif_header_block(10);
   }
   else
   {
      push_int64(tridx); 
      image_gif_header_block(4);
   }

   /* build renderblock */

   push_object(imgobj);
   push_object(nctobj);
   push_int(0);
   push_int(0);
   push_int(0);
   if (!trans)
      image_gif_render_block(5);
   else
      if (alpha)
      {
	 push_object(alphaobj);
	 push_int(ac.r);
	 push_int(ac.g);
	 push_int(ac.b);
	 image_gif_render_block(9);
      }
      else
      {
	 push_int(0);
	 push_int64(tridx);
	 image_gif_render_block(7);
      }

   /* build trailer */

   image_gif_end_block(0);

   /* add them and return */

   f_add(3);
}

void image_gif_encode(INT32 args)
{
   _image_gif_encode(args,0);
}

void image_gif_encode_fs(INT32 args)
{
   _image_gif_encode(args,1);
}

/*
**! method string netscape_loop_block();
**! method string netscape_loop_block(int number_of_loops);
**!     Creates a application-specific extention block;
**!	this block makes netscape and compatible browsers
**!	loop the animation a certain amount of times.
**! arg int number_of_loops
**!	Number of loops. Max and default is 65535.
*/

void image_gif_netscape_loop_block(INT32 args)
{
   unsigned short loops=0;
   char buf[30];
   if (args) {
      if (sp[-args].type!=T_INT) 
	 Pike_error("Image.GIF.netscape_loop_block: illegal argument (exected int)\n");
      else
	 loops=sp[-args].u.integer;
   } else
      loops=65535;
   pop_n_elems(args);

   sprintf(buf,"%c%c%cNETSCAPE2.0%c%c%c%c%c",
	   33,255,11,3,1,(loops>>8)&255,loops&255,0);

   push_string(make_shared_binary_string(buf,19));
}

/** decoding *****************************************/

/*
**! method array __decode();
**!     Decodes a GIF image structure down to chunks and
**!     returns an array containing the GIF structure;
**!	
**!     <pre>
**!	({int xsize,int ysize,      // 0: size of image drawing area
**!	  int numcol,               // 2: suggested number of colors 
**!	  void|string colortable,   // 3: opt. global colortable 
**!	  ({ int aspx, int aspy,    // 4,0: aspect ratio or 0, 0 if not set
**!	     int background }),     //   1: index of background color
**!	</pre>
**!     followed by any number these blocks in any order:
**!	<pre>
**!	  ({ GIF.EXTENSION,         //   0: block identifier
**!	     int extension,         //   1: extension number
**!	     string data })         //   2: extension data
**!				    
**!	  ({ GIF.RENDER,            //   0: block identifier
**!	     int x, int y,          //   1: position of render
**!	     int xsize, int ysize,  //   3: size of render
**!	     int interlace,         //   5: interlace flag
**!	     void|string colortbl,  //   6: opt. local colortable 
**!	     int lzwsize,           //   7: lzw code size
**!	     string lzwdata })      //   8: packed lzw data
**!     </pre>
**!	and possibly ended with one of these:
**!     <pre>
**!	  ({ GIF.ERROR_PREMATURE_EOD })   // premature end-of-data
**!
**!	  ({ GIF.ERROR_TOO_MUCH_DATA,     // data following end marker
**!	     string data })               // (rest of file)
**!
**!       ({ GIF.ERROR_UNKNOWN_DATA,      // unknown data
**!          string data })               // (rest of file)
**!     </pre>
**!
**! returns the above array
**!
**! note
**!	May throw errors if the GIF header is incomplete or illegal.
**!
**!	This is in the very advanced sector of the GIF support;
**!	please read about how GIF files works.
*/

static void _decode_get_extension(unsigned char **s,
				  size_t *len)
{
   int ext;
   size_t n,sz;

   if (*len<3) { (*s)+=*len; (*len)=0; return; }
   n=0;
   
   ext=(*s)[1];
   
   (*len)-=2;
   (*s)+=2;

   push_int(GIF_EXTENSION);

   push_int(ext);

   while (*len && (sz=**s))
   {
      if ((*len)-1<sz) sz=(*len)-1;

      push_string(make_shared_binary_string((char *)(*s)+1,sz));
      n++;

      (*len)-=(sz+1);
      (*s)+=(sz+1);
   }
   if (*len) { (*len)-=1; (*s)+=1; }

   if (!n)
      push_empty_string();
   else
      f_add(DO_NOT_WARN(n));

   f_aggregate(3);
}

static void _decode_get_render(unsigned char **s,
			       size_t *len)
{
   int n=0,bpp;
   size_t sz;

/* byte ...
   0  0x2c (render block init)
   1  xpos, low 8 bits
   2  xpos, high 8 bits
   3  ypos, low 8 bits
   4  ypos, high 8 bits
   5  xsize, low 8 bits
   6  xsize, high 8 bits
   7  ysize, low 8 bits
   8  ysize, high 8 bits
   9  packed field
      7 (128)  local palette
      6 (64)   interlace
      5 (32)   sorted palette (ignored)
      4 (16)   unused
      3 (8)    unused
      2..0 (7) bits per pixel - 1 (ie, palette size)
   10+ ...local palette... 
   y-1  lzw minimum code
   y+0  size
   y+1+ size bytes of packed lzw codes 
   ..repeat from y+0
   z    0 (end)
*/

   if (*len<10) { *len=0; return; }

   push_int( GIF_RENDER );
   push_int( (*s)[1]+((*s)[2]<<8) );
   push_int( (*s)[3]+((*s)[4]<<8) );
   push_int( (*s)[5]+((*s)[6]<<8) );
   push_int( (*s)[7]+((*s)[8]<<8) );
   bpp=((*s)[9]&7)+1;
   push_int( !!((*s)[9]&64) );

   if ( ((*s)[9]&128) ) {
      if ((*len)>10+(size_t)(3<<bpp) )
      {
	 push_string(make_shared_binary_string((char *)(*s)+10,3<<bpp));
	 (*s)+=10+(3<<bpp);
	 (*len)-=10+(3<<bpp);
      }
      else
      {
	 push_int(0);
	 push_int(0);
	 push_int(0);
	 push_int(0);
	 *len=0;
	 f_aggregate(10);
	 return;
      }
   } else {
      push_int(0);
      (*s)+=10;
      (*len)-=10;
   }

   if (*len) { push_int(**s); (*s)++; (*len)--; } else push_int(0);

   while (*len && (sz=**s))
   {
      if ((*len)-1<sz) sz=(*len)-1;

      push_string(make_shared_binary_string((char *)(*s)+1,sz));
      n++;

      (*len)-=(sz+1);
      (*s)+=(sz+1);
   }
   if (*len) { (*len)-=1; (*s)+=1; }

   if (!n)
      push_empty_string();
   else
      f_add(n);

   f_aggregate(9);
}

static void image_gif___decode(INT32 args)
{
   int xsize,ysize,globalpalette,colorres,bpp,bkgi,aspect;
   unsigned char *s;
   size_t len;
   struct pike_string *str;
   int n;
   ONERROR uwp;

   if (args!=1 
       || sp[-args].type!=T_STRING) 
      Pike_error("Image.GIF.__decode: illegal or illegal number of arguments\n");

   add_ref(str=sp[-args].u.string);
   s=(unsigned char *)str->str;
   len = str->len;
   pop_n_elems(args);
   SET_ONERROR(uwp,do_free_string,str);

/* byte ... is 
   0  'G'
   1  'I'
   2  'F'
   3  '8' (ignored) 
   4  '9' / '7' (ignored)
   5  'a' (ignored)
   6  xsize, low 8 bits
   7  xsize, high 8 bits
   8  ysize, low 8 bits
   9  ysize, high 8 bits
   10 bitfield : 
      7 (128)    global palette flag
      6..4 (112) color resolution (= 2<<x)
      3 (8)      palette is sorted (ignored)
      2..0 (7)   palette size (= 2<<x)   
   11 background color index
   12 aspect     (64*aspx/aspy-15)
   +numcolors*3 bytes of palette

   blocks
 */

   if (len<13 ||
       s[0]!='G' ||
       s[1]!='I' ||
       s[2]!='F')
      Pike_error("Image.GIF.__decode: not a GIF (no GIF header found)\n");
   
   xsize=s[6]+(s[7]<<8);
   ysize=s[8]+(s[9]<<8);
   
   globalpalette=s[10]&128;
   colorres=((s[10]>>4)&7)+1;
   bpp=(s[10]&7)+1;
   bkgi=s[11];
   aspect=s[12];

   s+=13; len-=13;
   if (globalpalette && len<(unsigned long)(3<<bpp))
      Pike_error("Image.GIF.__decode: premature EOD (in global palette)\n");
   
   push_int(xsize);
   push_int(ysize);
   push_int(1<<colorres);

   if (globalpalette)
   {
      push_string(make_shared_binary_string((char *)s,3<<bpp));
      s+=3<<bpp;
      len-=3<<bpp;
   }
   else
      push_int(0);

   if (aspect)
   {
      int aspx=aspect+15;
      int aspy=64;
      int prim[]={2,3,5,7};
      int i;
      for (i=0; i<4; i++)
	 while (!(aspx%prim[i]) && !(aspy%prim[i])) 
	    aspx/=prim[i],aspy/=prim[i];
      push_int(aspx); /* aspectx */
      push_int(aspy); /* aspecty */
   }
   else
   {
      push_int(0); /* aspectx */
      push_int(0); /* aspecty */
   }
   push_int(bkgi); /* background */
   f_aggregate(3);

   n=5; /* end aggregate size */

   /* blocks */

   for (;;)
   {
      if (!len)
      {
	 push_int(GIF_ERROR_PREMATURE_EOD);
	 f_aggregate(1);
	 s+=len;
	 len=0;
	 n++;
	 break;
      }
      if (*s==0x3b && len==1) break;
      switch (*s)
      {
	 case 0x21: _decode_get_extension(&s, &len); n++; break;
	 case 0x2c: _decode_get_render(&s, &len); n++; break;
	 case 0x3b: 
	    push_int(GIF_ERROR_TOO_MUCH_DATA);
	    push_string(make_shared_binary_string((char *)s+1,len-1));
	    f_aggregate(2);
	    s+=len;
	    len=0;
	    n++;
	    break;
	 default:
	    push_int(GIF_ERROR_UNKNOWN_DATA);
	    push_string(make_shared_binary_string((char *)s,len));
	    f_aggregate(2);
	    s+=len;
	    len=0;
	    n++;
	    break;
      }
      if (!len) break;
   }

   /* all done */

   f_aggregate(n);
   UNSET_ONERROR(uwp);
   free_string(str);
}

/*
**! method array _decode(string gifdata);
**! method array _decode(array __decoded);
**!     Decodes a GIF image structure down to chunks, and
**!     also decode the images in the render chunks.
**!	
**!     <pre>
**!	({int xsize,int ysize,    // 0: size of image drawing area
**!	  void|object colortable, // 2: opt. global colortable 
**!	  ({ int aspx, int aspy,  // 3 0: aspect ratio or 0, 0 if not set
**!	     int background }),   //   2: index of background color
**!	</pre>
**!     followed by any number these blocks in any order (gce chunks 
**!	are decoded and incorporated in the render chunks):
**!	<pre>
**!	  ({ GIF.RENDER,          //   0: block identifier
**!	    int x, int y,         //   1: position of render
**!	    object image,         //   3: render image 
**!	    void|object alpha,    //   4: 0 or render alpha channel
**!	    object colortable,    //   5: colortable (may be same as global)
**!				       	   
**!	    int interlace,        //   6: interlace flag 
**!	    int trans_index,      //   7: 0 or transparent color index 
**!	    int delay,            //   8: 0 or delay in centiseconds
**!	    int user_input,       //   9: user input flag
**!	    int disposal})        //  10: disposal method number (0..7)
**!
**!	  ({ GIF.EXTENSION,       //   0: block identifier
**!	     int extension,       //   1: extension number
**!	     string data })       //   2: extension data
**!
**!     </pre>
**!	and possibly ended with one of these:
**!     <pre>
**!	  ({ GIF.ERROR_PREMATURE_EOD })   // premature end-of-data
**!
**!	  ({ GIF.ERROR_TOO_MUCH_DATA,     // data following end marker
**!	     string data })               // (rest of file)
**!
**!       ({ GIF.ERROR_UNKNOWN_DATA,      // unknown data
**!          string data })               // (rest of file)
**!     </pre>
**!
**!	The <ref>decode</ref> method uses this data in a way similar
**!	to this program:
**!   
**!	<pre> 
**!	import Image;
**!	
**!	object my_decode_gif(string data)
**!	{
**!	   array a=GIF._decode(data);
**!	   object img=image(a[0],a[1]);
**!	   foreach (a[4..],array b)
**!	      if (b[0]==GIF.RENDER)
**!		 if (b[4]) img->paste_alpha(b[3],b[4],b[1],b[2]);
**!		 else img->paste(b[3],b[1],b[2]);
**!	   return img;
**!	}
**!	</pre>
**!	
**!
**! arg string gifdata
**!	GIF data (with header and all)
**! arg array __decoded
**!	GIF data as from <ref>__decode</ref>
**!
**! returns the above array
**!
**! note
**!	May throw errors if the GIF header is incomplete or illegal.
**!
**!	This is in the very advanced sector of the GIF support;
**!	please read about how GIF files works.
*/

static void _gif_decode_lzw(unsigned char *s,
			    size_t len,
			    int obits,
			    struct object *ncto,
			    rgb_group *dest,
			    rgb_group *alpha,
			    size_t dlen,
			    int tidx)
{
   struct neo_colortable *nct;

   rgb_group white={255,255,255},black={0,0,0};

   int bit=0,bits=obits+1;
   unsigned short n,last,maxcode=(1<<bits);
   unsigned short clearcode=(1<<(bits-1));
   unsigned short endcode=clearcode+1;
   int m=endcode;
   unsigned int q;
   unsigned int mask=(unsigned short)((1<<bits)-1);
   struct lzwc *c;

#ifdef GIF_DEBUG
   int debug=0;

fprintf(stderr,"_gif_decode_lzw(%lx,%lu,%d,%lx,%lx,%lx,%lu,%d)\n",
	s,len,obits,ncto,dest,alpha,dlen,tidx);
#endif

   nct=(struct neo_colortable*)get_storage(ncto,image_colortable_program);
   if (!nct || nct->type!=NCT_FLAT) return; /* uh? */

   if (len<2) return;
   q=s[0]|(s[1]<<8);
   bit=16;
   s+=2; len-=2;

   last=clearcode;

#define MAX_GIF_CODE 4096

   c=(struct lzwc*)xalloc(sizeof(struct lzwc)*MAX_GIF_CODE);

   for (n=0; n<clearcode; n++)
      c[n].prev=0xffff,c[n].len=1,c[n].c=n;
   c[clearcode].len=0; 
   c[endcode].len=0;   

   while (bit>0)
   {
      /* get next code */

      n=q&mask; 
      q>>=bits;
      bit-=bits; 

#ifdef GIF_DEBUG
      if (debug) fprintf(stderr,"code=%d 0x%02x bits=%d\n",n,n,bits);
#endif

      if (n==m) 
      {
	 c[n].prev=last;
	 c[n].c=c[last].c;
	 c[n].len=c[last].len+1;
      }
      else if (n>=m) 
      {
#ifdef GIF_DEBUG
	 fprintf(stderr,"cancel; illegal code, %d>=%d at %lx\n",n,m,s);
#endif
	 break; /* illegal code */
      }
      if (!c[n].len) {
	 if (n==clearcode) 
	 {
	    bits=obits+1;
	    mask=(1<<bits)-1;
	    m=endcode;
	    last=clearcode;
	    maxcode=1<<bits;
	 }
	 else 
	 {
	    /* endcode */
#ifdef GIF_DEBUG
	    fprintf(stderr,"endcode at %lx\n",s);
#endif
	    break; 
	 }
      } else {
	 struct lzwc *myc;
	 rgb_group *d,*da=NULL;
	 unsigned short lc;
	 myc=c+n;
	 
	 if (myc->len>dlen) 
	 {
#ifdef GIF_DEBUG
	    fprintf(stderr,"cancel at dlen left=%lu\n",dlen);
#endif
	    break;
	 }

	 d=(dest+=myc->len);
	 if (alpha) da=(alpha+=myc->len);
	 dlen-=myc->len;
	 
	 for (;;)
	 {
	    lc=myc->c;
	    if (lc<nct->u.flat.numentries)
	       *(--d)=nct->u.flat.entries[lc].color;
	    if (alpha) {
	       if (tidx==lc)
		  *(--da)=black;
	       else
		  *(--da)=white;
	    }
	    if (myc->prev==0xffff) break;
	    myc=c+myc->prev;
	 }

	 if (last!=clearcode)
	 {
	    c[m].prev=last;
	    c[m].len=c[last].len+1;
	    c[m].c=lc;
	 }
	 last=n;

	 m++;
	 if (m>=maxcode) {
	    if (m==MAX_GIF_CODE)
	    {
#ifdef GIF_DEBUG
	       fprintf(stderr,"too many codes at %lx\n",s);
#endif
	       m--;
	       bits=12;
	    }
	    else
	    { 
	       bits++; 
	       mask=(1<<bits)-1;
	       maxcode<<=1; 
	       if (maxcode>MAX_GIF_CODE) 
	       {
#ifdef GIF_DEBUG
		  fprintf(stderr,"cancel; gif codes=%ld m=%ld\n",maxcode,m);
#endif
		  break; /* error! too much codes */
	       }
	    }
	 }
      }


      while (bit<bits && len) 
	 q|=((*s)<<bit),bit+=8,s++,len--;
   }
#ifdef GIF_DEBUG
   fprintf(stderr,"end. bit=%d\n",bit);
#endif

   free(c);
}

static void gif_deinterlace(rgb_group *s,
			    unsigned long xsize,
			    unsigned long ysize)
{
   rgb_group *tmp;
   unsigned long y,n;

   tmp=malloc(xsize*ysize*sizeof(rgb_group));
   if (!tmp) return;

   MEMCPY(tmp,s,xsize*ysize*sizeof(rgb_group));

   n=0;
   for (y=0; y<ysize; y+=8)
      MEMCPY(s+y*xsize,tmp+n++*xsize,xsize*sizeof(rgb_group));
   for (y=4; y<ysize; y+=8)		  
      MEMCPY(s+y*xsize,tmp+n++*xsize,xsize*sizeof(rgb_group));
   for (y=2; y<ysize; y+=4)		  
      MEMCPY(s+y*xsize,tmp+n++*xsize,xsize*sizeof(rgb_group));
   for (y=1; y<ysize; y+=2)		  
      MEMCPY(s+y*xsize,tmp+n++*xsize,xsize*sizeof(rgb_group));
   
   free(tmp);
}
	
void image_gif__decode(INT32 args)
{
   struct array *a,*b=NULL;
   int n,i;
   struct object *o,*o2,*cto,*lcto;
   int transparency_index=0,transparency=0,
      disposal=0,user_input=0,delay=0,interlace;
   unsigned char *s=NULL;
   struct image *img,*aimg=NULL;

   if (!args)
      Pike_error("Image.GIF._decode: too few argument\n");

   if (sp[-args].type==T_ARRAY)
      pop_n_elems(args-1);
   else
      image_gif___decode(args);

   if (sp[-1].type!=T_ARRAY)
      Pike_error("Image.GIF._decode: internal error: "
	    "illegal result from __decode\n");
   
   a=sp[-1].u.array;
   if (a->size<5)
      Pike_error("Image.GIF._decode: given (__decode'd) array "
	    "is too small\n");

   push_svalue(a->item+0); /* xsize */
   push_svalue(a->item+1); /* ysize */

   if (a->item[3].type==T_STRING)
   {
      push_svalue(a->item+3);
      push_object(cto=clone_object(image_colortable_program,1));
   }
   else
   {
      push_int(0);
      cto=0;
   }

   push_svalue(a->item+4); /* misc array */

   n=4;

   i=5;
   for (; i<a->size; i++)
      if (a->item[i].type!=T_ARRAY ||
	  (b=a->item[i].u.array)->size<1 ||
	  b->item[0].type!=T_INT)
	 Pike_error("Image.GIF._decode: given (__decode'd) "
	       "array has illegal contents (position %d)\n",i);
      else
	 switch (b->item[0].u.integer)
	 {
	    case GIF_RENDER:
	       if (b->size!=9)
		  Pike_error("Image.GIF._decode: given (__decode'd) "
			"array has illegal contents "
			"(illegal size of block array in position %d)\n",i);
	       if (b->item[0].type!=T_INT ||
		   b->item[1].type!=T_INT ||
		   b->item[2].type!=T_INT ||
		   b->item[3].type!=T_INT ||
		   b->item[4].type!=T_INT ||
		   b->item[5].type!=T_INT ||
		   b->item[7].type!=T_INT ||
		   b->item[8].type!=T_STRING)
		  Pike_error("Image.GIF._decode: given (__decode'd) "
			"array has illegal contents "
			"(illegal type(s) in block array in position %d)\n",i);

	       push_int(GIF_RENDER);

	       push_svalue(b->item+1);
	       push_svalue(b->item+2);

	       interlace=b->item[5].u.integer;

	       if (b->item[6].type==T_STRING)
	       {
		  push_svalue(b->item+6);
		  lcto=clone_object(image_colortable_program,1);
	       }
	       else
	       {
		  lcto=cto;
		  if (lcto) add_ref(lcto);
	       }

	       push_int(b->item[3].u.integer);
	       push_int(b->item[4].u.integer);
	       o=clone_object(image_program,2);
	       img=(struct image*)get_storage(o,image_program);
	       push_object(o);
	       if (transparency)
	       {
		  push_int(b->item[3].u.integer);
		  push_int(b->item[4].u.integer);
		  o2=clone_object(image_program,2);
		  aimg=(struct image*)get_storage(o2,image_program);
		  push_object(o2);
		  if (lcto)
		     _gif_decode_lzw((unsigned char *)
				     b->item[8].u.string->str, /* lzw string */
				     b->item[8].u.string->len, /* lzw len */
				     b->item[7].u.integer,     /* lzw bits */
				     lcto, /* colortable */
				     img->img,
				     aimg->img,
				     img->xsize*img->ysize,
				     transparency_index);
	       }
	       else
	       {
		  push_int(0);
		  if (lcto)
		     _gif_decode_lzw((unsigned char *)
				     b->item[8].u.string->str, /* lzw string */
				     b->item[8].u.string->len, /* lzw len */
				     b->item[7].u.integer,     /* lzw bits */
				     lcto, /* colortable */
				     img->img,
				     NULL,
				     img->xsize*img->ysize,
				     0);
	       }

	       if (interlace)
	       {
		  gif_deinterlace(img->img,img->xsize,img->ysize);
		  if (aimg)
		     gif_deinterlace(aimg->img,aimg->xsize,aimg->ysize);
	       }

	       if (lcto) push_object(lcto); 
	       else push_int(0);
	       
	       push_int(interlace);
	       push_int(transparency_index);
	       push_int(delay);
	       push_int(user_input);
	       push_int(disposal);

	       f_aggregate(11);
	       n++;

	       transparency=disposal=user_input=delay=0;
	       break;
	    case GIF_EXTENSION:
	       if (b->size!=3)
		  Pike_error("Image.GIF._decode: given (__decode'd) "
			"array has illegal contents "
			"(illegal size of block array in position %d)\n",i);
	       if (b->item[2].type!=T_STRING)
		  Pike_error("Image.GIF._decode: given (__decode'd) "
			"array has illegal contents "
			"(no data string of block array in position %d)\n",i);
	       switch (b->item[1].u.integer)
	       {
		  case 0xf9: /* gce */
		     if (b->item[2].u.string->len>=4)
		     s=(unsigned char *)b->item[2].u.string->str;
		     transparency=s[0]&1;
		     user_input=!!(s[0]&2);
		     disposal=(s[0]>>2)&7;
		     delay=s[1]+(s[2]<<8);
		     transparency_index=s[3];
		     break;
		  default: /* unknown */
		     push_svalue(a->item+i);
		     n++;
		     break;
	       }
	       break;
	    case GIF_ERROR_PREMATURE_EOD:
	    case GIF_ERROR_UNKNOWN_DATA:
	    case GIF_ERROR_TOO_MUCH_DATA:
	       push_svalue(a->item+i);
	       i=a->size;
	       n++;
	       break;
	    default:
	       Pike_error("Image.GIF._decode: given (__decode'd) "
		     "array has illegal contents (illegal type of "
		     "block in position %d)\n",i);
	 }

   f_aggregate(n);
   stack_swap();
   pop_stack();
}

/*
**! method object decode(string data)
**! method object decode(array _decoded)
**! method object decode(array __decoded)
**!	Decodes GIF data and creates an image object.
**! 	
**! see also: encode
**!
**! note
**!	This function may throw errors upon illegal GIF data.
**!	This function uses <ref>__decode</ref>, <ref>_decode</ref>,
**!	<ref>Image.Image->paste</ref> and 
**!	<ref>Image.Image->paste_alpha</ref> internally.
**!
**! returns the decoded image as an image object
*/

void image_gif_decode(INT32 args)
{
   struct array *a,*b;
   struct image *img,*src,*alpha;
   struct object *o;
   int n;

   if (!args)
      Pike_error("Image.GIF._decode: too few argument\n");

   if (sp[-args].type==T_ARRAY)
   {
      pop_n_elems(args-1);
      if (sp[-args].u.array->size<4)
	 Pike_error("Image.GIF.decode: illegal argument\n");
      if (sp[-args].u.array->item[3].type!=T_ARRAY)
	 image_gif__decode(1);
   }
   else
      image_gif__decode(args);

   if (sp[-1].type!=T_ARRAY)
      Pike_error("Image.GIF.decode: internal error: "
	    "illegal result from _decode\n");

   a=sp[-1].u.array;
   if (a->size<4)
      Pike_error("Image.GIF.decode: given (_decode'd) array "
	    "is too small\n");

   push_svalue(a->item+0);
   push_svalue(a->item+1);
   o=clone_object(image_program,2);
   img=(struct image*)get_storage(o,image_program);
   
   for (n=4; n<a->size; n++)
      if (a->item[n].type==T_ARRAY
	  && (b=a->item[n].u.array)->size==11
	  && b->item[0].type==T_INT 
	  && b->item[0].u.integer==GIF_RENDER
	  && b->item[3].type==T_OBJECT
	  && (src=(struct image*)get_storage(b->item[3].u.object,
					     image_program)) )
      {
	 if (b->item[4].type==T_OBJECT)
	    alpha=(struct image*)get_storage(b->item[4].u.object,
					     image_program);
	 else
	    alpha=NULL;
	     
	 if (alpha) 
	 {
	    push_svalue(b->item+3);
	    push_svalue(b->item+4);
	    push_svalue(b->item+1);
	    push_svalue(b->item+2);
	    apply(o,"paste_mask",4);
	    pop_stack();
	 }
	 else
	 {
	    push_svalue(b->item+3);
	    push_svalue(b->item+1);
	    push_svalue(b->item+2);
	    apply(o,"paste",3);
	    pop_stack();
	 }
      }

   push_object(o);
   stack_swap();
   pop_stack();
}

/*
**! method object decode_layers(string data)
**! method object decode_layers(array _decoded)
**! method object decode_layer(string data)
**! method object decode_layer(array _decoded)
**!	Decodes GIF data and creates an array of layers
**!	or the resulting layer.
**! 	
**! see also: encode, decode_map
**!
**! note
**!	The resulting layer may not have the same size
**!	as the gif image, but the resulting bounding box
**!	of all render chunks in the gif file.
**!	The offset should be correct, though.
**!
*/

void image_gif_decode_layers(INT32 args)
{
   struct array *a,*b;
   struct image *src,*alpha;
   int n;
   int numlayers=0;

   if (!args)
      Pike_error("Image.GIF.decode_layers: too few argument\n");

   if (sp[-args].type==T_ARRAY)
   {
      pop_n_elems(args-1);
      if (sp[-args].u.array->size<4)
	 Pike_error("Image.GIF.decode: illegal argument\n");
      if (sp[-args].u.array->item[3].type!=T_ARRAY)
	 image_gif__decode(1);
   }
   else
      image_gif__decode(args);

   if (sp[-1].type!=T_ARRAY)
      Pike_error("Image.GIF.decode: internal error: "
	    "illegal result from _decode\n");

   a=sp[-1].u.array;
   if (a->size<4)
      Pike_error("Image.GIF.decode: given (_decode'd) array "
	    "is too small\n");

   for (n=4; n<a->size; n++)
      if (a->item[n].type==T_ARRAY
	  && (b=a->item[n].u.array)->size==11
	  && b->item[0].type==T_INT 
	  && b->item[0].u.integer==GIF_RENDER
	  && b->item[3].type==T_OBJECT
	  && (src=(struct image*)get_storage(b->item[3].u.object,
					     image_program)) )
      {
	 if (b->item[4].type==T_OBJECT)
	    alpha=(struct image*)get_storage(b->item[4].u.object,
					     image_program);
	 else
	    alpha=NULL;
	     
	 if (alpha) 
	 {
	    push_constant_text("image");
	    push_svalue(b->item+3);
	    push_constant_text("alpha");
	    push_svalue(b->item+4);
	    push_constant_text("xoffset");
	    push_svalue(b->item+1);
	    push_constant_text("yoffset");
	    push_svalue(b->item+2);
	    f_aggregate_mapping(8);
	    push_object(clone_object(image_layer_program,1));
	    numlayers++;
	 }
	 else
	 {
	    push_constant_text("image");
	    push_svalue(b->item+3);
	    push_constant_text("xoffset");
	    push_svalue(b->item+1);
	    push_constant_text("yoffset");
	    push_svalue(b->item+2);
	    f_aggregate_mapping(6);
	    push_object(clone_object(image_layer_program,1));
	    numlayers++;
	 }
      }

   f_aggregate(numlayers);
   stack_swap();
   pop_stack();
}

void image_gif_decode_layer(INT32 args)
{
   image_gif_decode_layers(args);
   image_lay(1);
}

/*
**! method mapping decode_map(string|array layers)
**!	Returns a mapping similar to other decoders
**!	<tt>_decode</tt> function.
**!
**!	<pre>
**!	    "image":the image
**!	    "alpha":the alpha channel
**!
**!	    "xsize":int
**!	    "ysize":int
**!		size of image
**!	    "type":"image/gif"
**!		file type information as MIME type
**!     </pre>
**!
**! note:
**!	The weird name of this function (not <tt>_decode</tt>
**!	as the other decoders) is because gif was the first
**!	decoder and was written before the API was finally
**!	defined. Sorry about that. /Mirar
*/

void image_gif_decode_map(INT32 args)
{
   image_gif_decode_layer(args);

   push_constant_text("image");
   push_constant_text("alpha");
   push_constant_text("xsize");
   push_constant_text("ysize");
   f_aggregate(4);
#define stack_swap_behind() do { struct svalue _=sp[-2]; sp[-2]=sp[-3]; sp[-3]=_; } while(0)
   stack_dup();
   stack_swap_behind();
   f_rows(2);
   f_call_function(1);
   f_mkmapping(2);
   push_constant_text("type");
   push_constant_text("image/gif");
   f_aggregate_mapping(2);
   f_add(2);
}

/*
**! method string _encode(array data)
**!	Encodes GIF data; reverses _decode.
**!
**! arg array data
**!	data as returned from _encode
**!
**! note
**!	Some given values in the array are ignored.
**!	This function does not give the _exact_ data back!
*/

void image_gif__encode_render(INT32 args)
{
   struct array *a;
   int localp;

   if (args<2 ||
       sp[-args].type!=T_ARRAY ||
       sp[1-args].type!=T_INT)
      Pike_error("Image.GIF._encode_render: Illegal argument(s) (expected array, int)\n");

   localp=sp[1-args].u.integer;
   add_ref(a=sp[-args].u.array);

   if (a->size<11)
      Pike_error("Image.GIF._encode_render: Illegal size of array\n");

   pop_n_elems(args);

   push_svalue(a->item+3); /* img */
   push_svalue(a->item+5); /* colortable */
   push_svalue(a->item+1); /* x */
   push_svalue(a->item+2); /* y */
   
   push_int(localp);

   if (a->item[4].type==T_OBJECT)
   {
      struct neo_colortable *nct;

      nct=(struct neo_colortable*)
	 get_storage(a->item[4].u.object,image_colortable_program);
      if (!nct)
      {
	 free_array(a);
	 Pike_error("Image.GIF._encode_render: Passed object is not colortable\n");
      }
      
      if (nct->type!=NCT_FLAT)
      {
	 free_array(a);
	 Pike_error("Image.GIF._encode_render: Passed colortable is not flat (sorry9\n");
      }
      push_svalue(a->item+4);
      if (a->item[7].type==T_INT 
	  && a->item[7].u.integer>=0 
	  && a->item[7].u.integer<nct->u.flat.numentries)
      {
	 push_int(nct->u.flat.entries[a->item[7].u.integer].color.r);
	 push_int(nct->u.flat.entries[a->item[7].u.integer].color.g);
	 push_int(nct->u.flat.entries[a->item[7].u.integer].color.b);
      }
      else
      {
	 push_int(0);
	 push_int(0);
	 push_int(0);
      }
      
   }

   push_svalue(a->item+8); /* delay */

   if (a->item[4].type!=T_OBJECT)
      push_int(-1);

   push_svalue(a->item+6); /* interlace */
   push_svalue(a->item+9); /* user_input */
   push_svalue(a->item+10); /* disposal */

   image_gif_render_block( (a->item[4].type==T_OBJECT) ? 13 : 10 );

   free_array(a);
}

void image_gif__encode_extension(INT32 args)
{
   struct array *a;
   char buf[2];
   int n,i;
   struct pike_string *s,*d;

   if (args<1 ||
       sp[-args].type!=T_ARRAY)
      Pike_error("Image.GIF._encode_extension: Illegal argument(s) (expected array)\n");

   add_ref(a=sp[-args].u.array);
   pop_n_elems(args);

   if (a->size<3)
      Pike_error("Image.GIF._encode_extension: Illegal size of array\n");
   if (a->item[1].type!=T_INT ||
       a->item[2].type!=T_STRING)
      Pike_error("Image.GIF._encode_extension: Illegal type in indices 1 or 2\n");

   sprintf(buf,"%c%c",0x21,(int) a->item[1].u.integer);
   push_string(make_shared_binary_string(buf,2));

   n=1;
   s=a->item[2].u.string;
   for (i=0;;)
      if (s->len-i==0)
      {
	 push_string(make_shared_binary_string("\0",1));
	 n++;
      }
      else if (s->len-i>=255)
      {
   	 d=begin_shared_string(256);
	 *((unsigned char*)(d->str))=255;
	 MEMCPY(d->str+1,s->str+i,255);
	 push_string(end_shared_string(d));
	 n++;
	 if (n>32) /* shrink stack */
	 {
	    f_add(n);
	    n=1;
	 }
	 i+=255;
      }
      else
      {
	 d=begin_shared_string(s->len-i+2);
	 d->str[0] = DO_NOT_WARN(s->len - i);
	 MEMCPY(d->str+1, s->str+i, d->len-i);
	 d->str[d->len-i+1]=0;
	 push_string(end_shared_string(d));
	 n++;
	 break;
      }

   f_add(n);

   free_array(a);
}

void image_gif__encode(INT32 args)
{
   struct array *a,*b;
   INT32 pos;
   int n;

   if (args<1 ||
       sp[-args].type!=T_ARRAY)
      Pike_error("Image.GIF._encode: Illegal argument (expected array)\n");

   add_ref(a=sp[-args].u.array);
   pos=0;
   n=0;
   pop_n_elems(args);

   if (a->size<4) 
      Pike_error("Image.GIF._encode: Given array too small\n");
   
   push_svalue(a->item+0); /* xsize */
   push_svalue(a->item+1); /* ysize */
   push_svalue(a->item+2); /* colortable or void */

   if (a->item[3].type!=T_ARRAY 
      || a->item[3].u.array->size<3)
   {
      free_array(a);
      Pike_error("Image.GIF._encode: Illegal type on array index 3 (expected array)\n");
   }

   push_svalue(a->item[3].u.array->item+2); /* bkgi */
   push_int(0); /* GIF87a-flag */
   push_svalue(a->item[3].u.array->item+0); /* aspectx */
   push_svalue(a->item[3].u.array->item+1); /* aspecty */

   image_gif_header_block(7); n++;

   pos=4;

   while (pos<a->size)
   {
      if (a->item[pos].type!=T_ARRAY)
      {
	 free_array(a);
	 Pike_error("Image.GIF._encode: Illegal type on array index %d (expected array)\n",pos);
      }
      b=a->item[pos].u.array;

      if (b->size<1
	  || b->item[0].type!=T_INT)
      {
	 free_array(a);
	 Pike_error("Image.GIF._encode: Illegal array on array index %d\n",pos);
      }
      
      if (b->item[0].u.integer==GIF_RENDER)
      {
	 push_svalue(a->item+pos);
	 push_int(is_equal(b->item+6,a->item+2));
	 image_gif__encode_render(2);
	 n++;
      }
      else if (b->item[0].u.integer==GIF_EXTENSION)
      {
	 push_svalue(a->item+pos);
	 image_gif__encode_extension(1);
	 n++;
      }
      else break; /* unknown, bail! */
      pos++;
   }

   image_gif_end_block(0); n++;

   free_array(a);

   f_add(n); /* add the strings */
}

/** lzw stuff ***************************************************/

static void image_gif_lzw_encode(INT32 args)
{
   struct gif_lzw lzw;

   if (!args || sp[-args].type!=T_STRING)
      Pike_error("Image.GIF.lzw_encode(): illegal argument\n");

   image_gif_lzw_init(&lzw,8);
   if (lzw.broken) Pike_error("out of memory\n");

   if (args>=2 && !UNSAFE_IS_ZERO(sp+1-args))
      lzw.earlychange=1;

   if (args>=3 && !UNSAFE_IS_ZERO(sp+2-args))
      lzw.reversebits=1;
   
   image_gif_lzw_add(&lzw,
		     (unsigned char *)sp[-args].u.string->str,
		     sp[-args].u.string->len);

   image_gif_lzw_finish(&lzw);
   
   if (lzw.broken) Pike_error("out of memory\n");

   pop_n_elems(args);
   push_string(make_shared_binary_string((char*)lzw.out,lzw.outpos));
}

static void image_gif_lzw_decode(INT32 args)
{
   unsigned char *s,*dest0,*dest;
   int earlychange=0;
   ptrdiff_t len;
   signed long n;
   signed long clearcode,endcode,last,q,bit,m;
   ptrdiff_t dlen,dlen0;
   unsigned int mask;
   struct lzwc *c;
   signed long bits,obits=8;
   signed long maxcode;
   int reversebits=0;

   if (!args || sp[-args].type!=T_STRING)
      Pike_error("Image.GIF.lzw_encode(): illegal argument\n");

   s=(unsigned char*)sp[-args].u.string->str;
   len = (ptrdiff_t)sp[-args].u.string->len;

   if (args>=2 && !UNSAFE_IS_ZERO(sp+1-args))
      earlychange=1;
   if (args>=3 && !UNSAFE_IS_ZERO(sp+2-args))
      reversebits=1;

   if (len<1)
   {
      pop_n_elems(args);
      push_empty_string();
      return;
   }

   clearcode=(1<<obits);
   endcode=clearcode+1;
   bits=obits+1;
   mask=(unsigned short)((1<<bits)-1);
   m=endcode;
   maxcode=(1<<bits);

   last=clearcode;

   c=(struct lzwc*)xalloc(sizeof(struct lzwc)*MAX_GIF_CODE);

   dest0=(unsigned char*)malloc(dlen0=len*4);
   if (!dest0)
   {
      free(c);
      Pike_error("Image.GIF.lzw_decode: out of memory\n");
   }
   dest=dest0; dlen=dlen0;

   for (n=0; n<clearcode; n++) {
      c[n].prev=0xffff;
      c[n].len=1;
      c[n].c = DO_NOT_WARN((unsigned short)n);
   }
   c[clearcode].len=0; 
   c[endcode].len=0;   

   if (len>1)
   {
      if (reversebits) q=s[1]|(s[0]<<8);
      else q=s[0]|(s[1]<<8);
      bit=16;
      s+=2; len-=2;
   }
   else
   {
      q=s[0];
      bit=8;
      s+=1; len-=1;
   }

   while (bit>0)
   {
      /* get next code */

#ifdef GIF_DEBUG
      fprintf(stderr,"q=0x%04x bit=%2d bits=%2d ... ",q,bit,bits);
#endif

      if (reversebits)
	 n=(q>>(bit-bits))&mask; 
      else
      {
	 n=q&mask; 
	 q>>=bits;
      }
      bit-=bits; 

#ifdef GIF_DEBUG
      fprintf(stderr,"code=%3d 0x%02x bits=%d bit=%2d *s=0x%02x len=%d\n",n,n,bits,bit,*s,len);
#endif

      if (n==m) 
      {
	 c[n].prev = DO_NOT_WARN((unsigned short)last);
	 c[n].c=c[last].c;
	 c[n].len=c[last].len+1;
      }
      else if (n>=m) 
      {
#ifdef GIF_DEBUG
	 fprintf(stderr,"cancel; illegal code, %d>=%d at %lx\n",n,m,s);
#endif
	 break; /* illegal code */
      }
      if (!c[n].len) 
	 if (n==clearcode) 
	 {
	    bits=obits+1;
	    mask=(1<<bits)-1;
	    m=endcode;
	    last=clearcode;
	    maxcode=1<<bits;
	 }
	 else 
	 {
	    /* endcode */
#ifdef GIF_DEBUG
	    fprintf(stderr,"endcode at %lx\n",s);
#endif
	    break; 
	 }
      else 
      {
	 struct lzwc *myc;
	 unsigned char *d;
	 unsigned short lc;
	 myc=c+n;
	 
	 if (myc->len>dlen) 
	 {
	    ptrdiff_t p;
	    p = (dest - dest0);

#ifdef GIF_DEBUG
	    fprintf(stderr,"increase at dlen left=%lu p=%ld dlen0=%d\n",dlen,p,dlen0);
#endif
	    dest=realloc(dest0,dlen0*2);
	    if (!dest)
	    {
	       dest=dest0+p;
	       break; /* out of memory */
	    }
	    dest0=dest;
	    dest=dest0+p;
	    dlen+=dlen0;
	    dlen0+=dlen0;

#ifdef GIF_DEBUG
	    fprintf(stderr,"increase at dlen left=%lu p=%ld dlen0=%d\n",dlen,dest-dest0,dlen0);
#endif
	 }

	 d=(dest+=myc->len);
	 dlen-=myc->len;
	 
	 for (;;)
	 {
	    lc=myc->c;
	    *(--d)=(unsigned char)lc;
	    if (myc->prev==0xffff) break;
	    myc=c+myc->prev;
	 }

	 if (last!=clearcode)
	 {
	    c[m].prev = DO_NOT_WARN((unsigned short)last);
	    c[m].len=c[last].len+1;
	    c[m].c=lc;
	 }
	 last=n;

	 m++;
	 if (m>=maxcode - earlychange) {
	    if (m==MAX_GIF_CODE - earlychange)
	    {
#ifdef GIF_DEBUG
	       fprintf(stderr,"too many codes at %lx\n",s);
#endif
	       m--;
	       bits=12;
	    }
	    else 
	    { 
	       bits++; 
	       mask=(1<<bits)-1;
	       maxcode<<=1; 
	       if (maxcode>MAX_GIF_CODE) 
	       {
#ifdef GIF_DEBUG
		  fprintf(stderr,"cancel; gif codes=%ld m=%ld\n",maxcode,m);
#endif
		  break; /* error! too much codes */
	       }
	    }
	 }
      }

      if (reversebits)
	 while (bit<bits && len) 
	    q=(q<<8)|(*s),bit+=8,s++,len--;
      else
	 while (bit<bits && len) 
	    q|=((*s)<<bit),bit+=8,s++,len--;
   }
   free(c);

   pop_n_elems(args);
   push_string(make_shared_binary_string((char*)dest0,dest-dest0));
   free(dest0);
}

/** module *******************************************/

struct program *image_encoding_gif_program=NULL;

PIKE_MODULE_INIT
{
#ifdef DYNAMIC_MODULE
  image_program = PIKE_MODULE_IMPORT(Image, image_program);
  image_colortable_program=PIKE_MODULE_IMPORT(Image, image_colortable_program);
  image_layer_program = PIKE_MODULE_IMPORT(Image, image_layer_program);
#endif /* DYNAMIC_MODULE */

  if (!image_program || !image_colortable_program || !image_layer_program) {
    yyerror("Could not load Image module.");
    return;
  }

  ADD_FUNCTION("render_block", image_gif_render_block,
	       tFunc(tObj tObj
		     tOr(tInt,tVoid) tOr(tInt,tVoid) tOr(tInt,tVoid)
		     tOr3(tInt,tObj,tVoid)
		     tOr(tInt,tVoid) tOr(tInt,tVoid) tOr(tInt,tVoid)
		     tOr(tInt,tVoid) tOr(tInt,tVoid)
		     tOr(tInt,tVoid) tOr(tInt,tVoid), tStr), 0);
  ADD_FUNCTION("_gce_block", image_gif__gce_block,
	       tFunc(tInt tInt tInt tInt tInt, tStr), 0);
  ADD_FUNCTION("_render_block", image_gif__render_block,
	       tFunc(tInt tInt tInt tInt tInt tStr tStr tInt, tStr), 0);

  ADD_FUNCTION("header_block", image_gif_header_block,
	       tFunc(tInt tInt tOr(tInt,tObj)
		     tOr(tInt,tVoid) tOr(tInt,tVoid) tOr(tInt,tVoid)
		     tOr(tInt,tVoid) tOr(tInt,tVoid) tOr(tInt,tVoid)
		     tOr(tInt,tVoid), tStr), 0);
  ADD_FUNCTION("end_block", image_gif_end_block,
	       tFunc(tNone,tStr), 0);
  ADD_FUNCTION("encode", image_gif_encode,
	       tFunc(tObj tOr3(tInt,tObj,tVoid) tOr3(tInt,tObj,tVoid)
		     tOr(tInt,tVoid) tOr(tInt,tVoid) tOr(tInt,tVoid),
		     tStr), 0);
  ADD_FUNCTION("encode_trans",image_gif_encode,
	       tFunc(tObj tOr3(tInt,tObj,tVoid) tOr3(tInt,tObj,tVoid)
		     tOr(tInt,tVoid) tOr(tInt,tVoid) tOr(tInt,tVoid),
		     tStr), 0);
  ADD_FUNCTION("encode_fs", image_gif_encode_fs,
	       tFunc(tObj tOr3(tInt,tObj,tVoid) tOr3(tInt,tObj,tVoid)
		     tOr(tInt,tVoid) tOr(tInt,tVoid) tOr(tInt,tVoid),
		     tStr), 0);

  ADD_FUNCTION("netscape_loop_block", image_gif_netscape_loop_block,
	       tFunc(tOr(tInt,tVoid),tStr), 0);

  ADD_FUNCTION("__decode", image_gif___decode,
	       tFunc(tStr,tArray), 0);
  ADD_FUNCTION("_decode", image_gif__decode,
	       tFunc(tOr(tStr,tArray),tArray), 0);
  ADD_FUNCTION("decode", image_gif_decode,
	       tFunc(tOr(tStr,tArray),tObj), 0);
  ADD_FUNCTION("decode_layers", image_gif_decode_layers,
	       tFunc(tOr(tStr,tArray),tArr(tObj)), 0);
  ADD_FUNCTION("decode_layer", image_gif_decode_layer,
	       tFunc(tOr(tStr,tArray),tObj), 0);
  ADD_FUNCTION("decode_map", image_gif_decode_map,
	       tFunc(tOr(tStr,tArray),tMapping), 0);

  ADD_FUNCTION("_encode", image_gif__encode,
	       tFunc(tArray,tStr), 0);
  ADD_FUNCTION("_encode_render", image_gif__encode_render,
	       tFunc(tArray,tStr), 0);
  ADD_FUNCTION("_encode_extension", image_gif__encode_extension,
	       tFunc(tArray,tStr), 0);

  ADD_FUNCTION("lzw_encode", image_gif_lzw_encode,
	       tFunc(tStr tOr(tInt,tVoid) tOr(tInt,tVoid), tStr), 0);
  ADD_FUNCTION("lzw_decode", image_gif_lzw_decode,
	       tFunc(tStr tOr(tInt,tVoid) tOr(tInt,tVoid), tStr), 0);

  /** constants **/
   
  add_integer_constant("RENDER",GIF_RENDER,0);
  add_integer_constant("EXTENSION",GIF_EXTENSION,0);

  add_integer_constant("LOOSE_GCE",GIF_LOOSE_GCE,0);
  add_integer_constant("NETSCAPE_LOOP",GIF_NETSCAPE_LOOP,0);

  add_integer_constant("ERROR_PREMATURE_EOD",GIF_ERROR_PREMATURE_EOD,0);
  add_integer_constant("ERROR_UNKNOWN_DATA",GIF_ERROR_UNKNOWN_DATA,0);
  add_integer_constant("ERROR_TOO_MUCH_DATA",GIF_ERROR_TOO_MUCH_DATA,0);

  /** done **/
}

#else /* !WITH_GIF */
PIKE_MODULE_INIT
{
}
#endif /* WITH_GIF */

PIKE_MODULE_EXIT
{
}
