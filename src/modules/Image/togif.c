/*

togif 

$Id: togif.c,v 1.29 1998/01/13 22:59:25 hubbe Exp $ 

old GIF API compat stuff

*/

/*
**! module Image
**! note
**!	$Id: togif.c,v 1.29 1998/01/13 22:59:25 hubbe Exp $
**! class image
*/

#include "global.h"

#include <math.h>
#include <ctype.h>

#include "stralloc.h"
#include "global.h"
#include "threads.h"
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "array.h"
#include "error.h"
#include "dynamic_buffer.h"
#include "operators.h"

#include "image.h"
#include "colortable.h"

#ifdef THIS
#undef THIS /* Needed for NT */
#endif

#define THIS ((struct image *)(fp->current_storage))
#define THISOBJ (fp->current_object)

extern struct program *image_colortable_program;

/*
**! method string gif_begin()
**! method string gif_begin(int num_colors)
**! method string gif_begin(array(array(int)) colors)
**! method string gif_end()
**! method string gif_netscape_loop(int loops)
**! method string togif()
**! method string togif(int trans_r,int trans_g,int trans_b)
**! method string togif(int num_colors,int trans_r,int trans_g,int trans_b)
**! method string togif(array(array(int)) colors,int trans_r,int trans_g,int trans_b)
**! method string togif_fs()
**! method string togif_fs(int trans_r,int trans_g,int trans_b)
**! method string togif_fs(int num_colors,int trans_r,int trans_g,int trans_b)
**! method string togif_fs(array(array(int)) colors,int trans_r,int trans_g,int trans_b)
**! method string gif_add()
**! method string gif_add_fs()
**! method string gif_add_nomap()
**! method string gif_add_fs_nomap()
**! method string gif_add*(int x,int y)
**! method string gif_add*(int x,int y,int delay_cs)
**! method string gif_add*(int x,int y,int num_colors,int delay_cs)
**! method string gif_add*(int x,int y,array(array(int)) colors,int delay_cs)
**!	old GIF API compatibility function. Don't use 
**!	in any new code. 
**!
**!	<table>
**!	<tr><td></td><td>is replaced by</td></tr>
**!	<tr><td>gif_begin</td><td><ref>Image.GIF.header_block</ref></td></tr>
**!	<tr><td>gif_end</td><td><ref>Image.GIF.end_block</ref></td></tr>
**!	<tr><td>gif_netscape_loop</td><td><ref>Image.GIF.netscape_loop_block</ref></td></tr>
**!	<tr><td>togif</td><td><ref>Image.GIF.encode</ref></td></tr>
**!	<tr><td>togif_fs</td><td><ref>Image.GIF.encode</ref>¹</td></tr>
**!	<tr><td>gif_add</td><td><ref>Image.GIF.render_block</ref>¹²</td></tr>
**!	<tr><td>gif_add_fs</td><td><ref>Image.GIF.render_block</ref>¹</td></tr>
**!	<tr><td>gif_add_nomap</td><td><ref>Image.GIF.render_block</ref>²</td></tr>
**!	<tr><td>gif_add_fs_nomap</td><td><ref>Image.GIF.render_block</ref>¹²</td></tr>
**!	</table>
**!
**!	¹ Use <ref>Image.colortable</ref> to get whatever dithering
**!	you want.
**!
**!	² local map toggle is sent as an argument
**!
**! returns GIF data.
*/

extern void image_gif_header_block(INT32 args);
extern void image_gif_end_block(INT32 args);
extern void image_gif_netscape_loop_block(INT32 args);
extern void image_gif_render_block(INT32 args);

void image_gif_begin(INT32 args)
{
   if (args)
   {
      struct object *o;
      if (sp[-args].type==T_INT)
      {
	 int i=sp[-args].u.integer;
	 pop_n_elems(args);
	 push_int(THIS->xsize);
	 push_int(THIS->ysize);
	 THISOBJ->refs++;
	 push_object(THISOBJ);
	 push_int(i);
	 o=clone_object(image_colortable_program,2);
      }
      else 
      {
	 o=clone_object(image_colortable_program,args);
      }
      push_int(THIS->xsize);
      push_int(THIS->ysize);
      push_object(o);
      image_gif_header_block(3);
   }
   else
   {
      push_int(THIS->xsize);
      push_int(THIS->ysize);
      push_int(256);
      image_gif_header_block(3);
   }
}

void image_gif_end(INT32 args)
{
   image_gif_end_block(args);
}

void image_gif_netscape_loop(INT32 args)
{
   image_gif_netscape_loop_block(args);
}

static void img_gif_add(INT32 args,int fs,int lm,
			rgb_group *transparent)
{
   INT32 x=0,y=0;
   int delay=0;
   struct object *ncto=NULL;

   struct svalue *msp=sp;

   if (args==0) x=y=0;
   else if (args<2
            || sp[-args].type!=T_INT
            || sp[1-args].type!=T_INT)
      error("Illegal argument(s) to image->gif_add()\n");
   else 
   {
      x=sp[-args].u.integer;
      y=sp[1-args].u.integer;
   }

   if (args>2 && sp[2-args].type==T_ARRAY)
   {
      struct svalue *sv=sp+2-args;
      push_svalue(sv);
      ncto=clone_object(image_colortable_program,1);
   }
   else if (args>3 && sp[2-args].type==T_INT)
   {
      INT32 i=sp[2-args].u.integer;
      THISOBJ->refs++;
      push_object(THISOBJ);
      push_int(i);
      ncto=clone_object(image_colortable_program,2);
   }

   if (args>2+!!ncto)
   {
      if (sp[2+!!ncto-args].type==T_INT) 
         delay=sp[2+!!ncto-args].u.integer;
      else if (sp[2+!!ncto-args].type==T_FLOAT) 
         delay=(unsigned short)(sp[2+!!ncto-args].u.float_number*100);
      else 
         error("Illegal argument %d to image->gif_add()\n",3+!!ncto);
   }
   
   if (!ncto)
   {
      THISOBJ->refs++;
      push_object(THISOBJ);
      push_int(255);
      ncto=clone_object(image_colortable_program,2);
   }

   if (fs) image_colortable_internal_floyd_steinberg(
         (struct neo_colortable *)get_storage(ncto,image_colortable_program));

   pop_n_elems(args);

   THISOBJ->refs++;
   push_object(THISOBJ);
   push_object(ncto);
   push_int(x);
   push_int(y);
   push_int(lm);
   push_int(delay);
   
   if (transparent) 
   {
      unsigned char trd;
      
      image_colortable_index_8bit_image((struct neo_colortable *)
					get_storage(ncto,
						    image_colortable_program),
					transparent,&trd,1,1);
      
      push_int(trd);

      image_gif_render_block(7);
   }
   else
      image_gif_render_block(6);
}


void image_gif_add(INT32 args)
{
   img_gif_add(args,0,1,NULL);
}

void image_gif_add_fs(INT32 args)
{
   img_gif_add(args,1,1,NULL);
}

void image_gif_add_nomap(INT32 args)
{
   img_gif_add(args,0,0,NULL);
}

void image_gif_add_fs_nomap(INT32 args)
{
   img_gif_add(args,1,0,NULL);
}

/*
**! method string togif()
**! method string togif(int trans_r,int trans_g,int trans_b)
**! method string togif(int num_colors,int trans_r,int trans_g,int trans_b)
**! method string togif(array(array(int)) colors,int trans_r,int trans_g,int trans_b)
**! method string togif_fs()
**! method string togif_fs(int trans_r,int trans_g,int trans_b)
**! method string togif_fs(int num_colors,int trans_r,int trans_g,int trans_b)
**! method string togif_fs(array(array(int)) colors,int trans_r,int trans_g,int trans_b)
**!	Makes GIF data. The togif_fs variant uses floyd-steinberg 
**!	dithereing.
**! returns the GIF data
**!
**! arg int num_colors
**!	number of colors to quantize to (default is 256) 
**! array array(array(int)) colors
**!	colors to map to (default is to quantize to 256), format is ({({r,g,b}),({r,g,b}),...}).
**! arg int trans_r
**! arg int trans_g
**! arg int trans_b
**!	one color, that is to be transparent.
**! see also: gif_begin, gif_add, gif_end, toppm, fromgif
*/

extern void _image_gif_encode(INT32 args,int fs);

static void img_encode_gif(rgb_group *transparent,int fs,INT32 args)
{
   struct object *co=NULL;
   if (args)
   {
      if (sp[-args].type==T_OBJECT)
      {
	 (co=sp[-args].u.object)->refs++;
	 pop_n_elems(args);
      }
      else if (sp[-args].type==T_ARRAY)
	 co=clone_object(image_colortable_program,args);
      else if (sp[-args].type==T_INT)
      {
	 unsigned long numcolors=sp[-args].u.integer;
	 pop_n_elems(args);
	 push_object(THISOBJ); THISOBJ->refs++;
	 push_int(numcolors);
	 co=clone_object(image_colortable_program,2);
      }
      else
	 error("Illegal argument to img->togif()\n");
   }
   else
   {
      push_object(THISOBJ); THISOBJ->refs++;
      push_int(256);
      co=clone_object(image_colortable_program,2);
   }
   push_object(THISOBJ); THISOBJ->refs++;
   push_object(co);
   if (transparent)
   {
      push_int(transparent->r);
      push_int(transparent->g);
      push_int(transparent->b);
      _image_gif_encode(5,fs);
   }
   else _image_gif_encode(2,fs);
}

static INLINE void getrgb(struct image *img,
                          INT32 args_start,INT32 args,char *name)
{
   INT32 i;
   if (args-args_start<3) return;
   for (i=0; i<3; i++)
      if (sp[-args+i+args_start].type!=T_INT)
         error("Illegal r,g,b argument to %s\n",name);
   img->rgb.r=(unsigned char)sp[-args+args_start].u.integer;
   img->rgb.g=(unsigned char)sp[1-args+args_start].u.integer;
   img->rgb.b=(unsigned char)sp[2-args+args_start].u.integer;
   if (args-args_start>=4)
      if (sp[3-args+args_start].type!=T_INT)
         error("Illegal alpha argument to %s\n",name);
      else
         img->alpha=sp[3-args+args_start].u.integer;
   else
      img->alpha=0;
}

void image_togif(INT32 args)
{
   rgb_group *transparent=NULL;

   if (args>=3)
   {
      getrgb(THIS,args>3,args,"image->togif() (transparency)");
      transparent=&(THIS->rgb);
   }
   if (args==3) pop_n_elems(3);
   else if (args) pop_n_elems(args-1);

   if (!THIS->img) { error("no image\n");  return; }

   img_encode_gif(transparent, 0, args&&args!=3);
}


void image_togif_fs(INT32 args)
{
   rgb_group *transparent=NULL;

   if (args>=3)
   {
      getrgb(THIS,args>3,args,"image->togif() (transparency)");
      transparent=&(THIS->rgb);
   }
   if (args==3) pop_n_elems(3);
   else if (args) pop_n_elems(args-1);

   if (!THIS->img) { error("no image\n");  return; }

   img_encode_gif(transparent, 1, args&&args!=3);
}
