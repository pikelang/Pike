/* $Id: gif.c,v 1.1 1997/10/27 02:50:54 mirar Exp $ */

/*
**! module Image
**! note
**!	$Id: gif.c,v 1.1 1997/10/27 02:50:54 mirar Exp $
**!
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
RCSID("$Id: gif.c,v 1.1 1997/10/27 02:50:54 mirar Exp $");
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "threads.h"
#include "array.h"
#include "error.h"


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

string header_block(object img,object colortable,int noglobalpalette);
string header_block(object img,int numcol,int noglobalpalette);
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
**! method string _gce_block(int transparency,int transparency_index,int delay,int user_input,int disposal);
**!
**!     This function gives back a Graphic Control Extension block,
**!	normally placed before an render block.
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
**!	transparency.
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
**! see also:
**!	encode, _encode, header_block, end_block
**! 
**! note
**!	This is in the advanced sector of the GIF support;
**!	please read some about how GIFs are packed.
**!
**!	The user_input and disposal method are unsupported
**!	in most decoders.
*/

int image_gif_add_line(struct neo_colortable *nct,
		       struct gif_lzw *lzw,
		       rgb_group *s,
		       rgb_group *m,
		       int len,
		       int alphaidx)
{
   unsigned char *buf=alloca(len);
   int n;
   unsigned char *bd;
   
   image_colortable_get_index_line(nct,s,buf,len);

   if (m)
   {
      n=len;
      bd=buf;
      while (n--)
      {
	 if (!(m->r||m->g||m->b)) *bd=alphaidx;
	 m++;
	 bd++;
      }
   }

   return image_gif_lzw_add(lzw,buf,len);
}

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

   unsigned char *indexbuf;
   unsigned char buf[20];

   int y,xs,ys;
   rgb_group *img_s,*alpha_s=NULL;
   struct gif_lzw lzw;

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
      localpalette=sp[3-args].u.integer;
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
	 alphaidx=numcolors;
	 alphaentry=1;
	 if (numcolors>255) 
	    error("Image.GIF.render_block(): too many colors in colortable (255 is max, need one for transparency)\n");
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

/*** write image rendering header */

   sprintf(buf,"%c%c%c%c%c%c%c%c%c%c",
	   0x2c, /* render block initiator */
	   xpos&255, (xpos>>8)&255, /* left position */
	   ypos&255, (ypos>>8)&255, /* top position */
	   img->xsize&255, (img->xsize>>8)&255, /* width */
	   img->ysize&255, (img->ysize>>8)&255, /* height */
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
      ps=begin_shared_string((1<<bpp)*3);
      image_colortable_write_rgb(nct,ps->str);
      MEMSET(ps->str+numcolors*3,0,((1<<bpp)-numcolors)*3);
      if (alphaentry) 
      {
	 ps->str[3*alphaentry+0]=alphacolor.r;
	 ps->str[3*alphaentry+1]=alphacolor.g;
	 ps->str[3*alphaentry+2]=alphacolor.b;
      }
      push_string(end_shared_string(ps));
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
   
   numstrings+=image_gif_lzw_init(&lzw,bpp<2?2:bpp);

   xs=img->xsize;
   ys=img->ysize;
   img_s=img->img;
   if (alpha) alpha_s=alpha->img;

   if (interlace)
   {
   /* interlace: 
      row 0, 8, 16, ...
      row 4, 12, 20, ...
      row 2, 6, 10, ...
      row 1, 3, 5, 7, ... */
      
      for (y=0; y<ys; y+=8)
	 numstrings+=image_gif_add_line(nct,&lzw,img_s+xs*y,
					alpha?alpha_s+xs*y:NULL,xs,alphaidx);
      for (y=4; y<ys; y+=8)
	 numstrings+=image_gif_add_line(nct,&lzw,img_s+xs*y,
					alpha?alpha_s+xs*y:NULL,xs,alphaidx);
      for (y=2; y<ys; y+=4)
	 numstrings+=image_gif_add_line(nct,&lzw,img_s+xs*y,
					alpha?alpha_s+xs*y:NULL,xs,alphaidx);
      for (y=1; y<ys; y+=2)
	 numstrings+=image_gif_add_line(nct,&lzw,img_s+xs*y,
					alpha?alpha_s+xs*y:NULL,xs,alphaidx);
   }
   else
   {
      y=ys;
      if (!alpha) alpha_s=NULL;
      while (y--)
      {
	 numstrings+=image_gif_add_line(nct,&lzw,img_s,alpha_s,xs,alphaidx);
	 if (alpha) alpha_s+=xs;
	 img_s+=xs;
      }
   }

   numstrings+=image_gif_lzw_finish(&lzw);

/*** done */

   f_add(numstrings);

   (ps=sp[-1].u.string)->refs++;

   pop_n_elems(args+1);
   push_string(ps);
}

struct program *image_gif_module_program=NULL;

void init_image_gif()
{
   start_new_program();
   
   add_function("render_block",image_gif_render_block,
		"function(object,object,void|int,void|int,void|int,void|object,void|int,void|int,void|int,void|int,void|int,void|int,void|int:string)"
		"|function(object,object,void|int,void|int,void|int,void|int,void|int,void|int,void|int,void|int:string)",0);
   add_function("_gce_block",image_gif__gce_block,
		"function(int,int,int,int,int:string)",0);

   image_gif_module_program=end_program();
   push_object(clone_object(image_gif_module_program,0));
   add_constant(make_shared_string("GIF"),sp-1,0);
   pop_stack();
}

void exit_image_gif()
{
  if(image_gif_module_program)
  {
    free_program(image_gif_module_program);
    image_gif_module_program=0;
  }
}
