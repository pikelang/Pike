/*
 * $Id: image_jpeg.c,v 1.15 1998/04/05 21:14:09 mirar Exp $
 */

#include "config.h"

#if !defined(HAVE_LIBJPEG)
#undef HAVE_JPEGLIB_H
#endif

#ifdef HAVE_JPEGLIB_H

#define FILE void
#define size_t unsigned int
#include <jpeglib.h>
#undef size_t
#undef FILE

#endif /* HAVE_JPEGLIB_H */

#ifdef HAVE_STDLIB_H
#undef HAVE_STDLIB_H
#endif
#include "global.h"
RCSID("$Id: image_jpeg.c,v 1.15 1998/04/05 21:14:09 mirar Exp $");

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

#ifdef HAVE_JPEGLIB_H

#include "../Image/image.h"

#ifdef DYNAMIC_MODULE
static struct program *image_program=NULL;
#else
extern struct program *image_program; 
/* Image module is probably linked static too. */
#endif

#endif /* HAVE_JPEGLIB_H */

static struct pike_string *param_baseline;
static struct pike_string *param_quality;
static struct pike_string *param_optimize;
static struct pike_string *param_smoothing;
static struct pike_string *param_x_density;
static struct pike_string *param_y_density;
static struct pike_string *param_density;
static struct pike_string *param_density_unit;
static struct pike_string *param_method;
static struct pike_string *param_progressive;
static struct pike_string *param_block_smoothing;
static struct pike_string *param_scale_denom;
static struct pike_string *param_scale_num;
static struct pike_string *param_fancy_upsampling;

#ifdef HAVE_JPEGLIB_H

/*
**! module Image
**! submodule JPEG
**!
**! note
**!	This module uses <tt>libjpeg</tt>, a software from
**!	Independent JPEG Group.
*/

static void my_output_message(struct jpeg_common_struct *cinfo)
{
   /* no message */
   /* (this should not be called) */
}

static void my_error_exit(struct jpeg_common_struct *cinfo)
{
   char buffer[JMSG_LENGTH_MAX];
   (*cinfo->err->format_message) (cinfo, buffer);

   jpeg_destroy(cinfo);
   error("Image.JPEG: fatal error in libjpeg; %s\n",buffer);
}

static void my_emit_message(struct jpeg_common_struct *cinfo,int msg_level)
{
   /* no trace */
}

struct my_destination_mgr
{
   struct jpeg_destination_mgr pub;

   char *buf;
   size_t len;
};

#define DEFAULT_BUF_SIZE 8192
#define BUF_INCREMENT 8192

static void my_init_destination(struct jpeg_compress_struct *cinfo)
{
   struct my_destination_mgr *dm=(struct my_destination_mgr *)cinfo->dest;

   dm->buf=malloc(DEFAULT_BUF_SIZE);
   if (dm->buf==0) dm->len=0; else dm->len=DEFAULT_BUF_SIZE;

   dm->pub.free_in_buffer=DEFAULT_BUF_SIZE;
   dm->pub.next_output_byte=(JOCTET*)dm->buf;
}

static boolean my_empty_output_buffer(struct jpeg_compress_struct *cinfo)
{
   struct my_destination_mgr *dm=(struct my_destination_mgr *)cinfo->dest;
   int pos;
   char *new;

   pos=dm->len; /* foo! dm->len-dm->pub.free_in_buffer; */
   new=(char*)realloc(dm->buf,dm->len+BUF_INCREMENT);
   if (!new) return FALSE;

   dm->buf=new;
   dm->len+=BUF_INCREMENT;
   dm->pub.free_in_buffer=dm->len-pos;
   dm->pub.next_output_byte=(JOCTET*)new+pos;

   return TRUE;
}

static void my_term_destination(struct jpeg_compress_struct *cinfo)
{
   /* don't do anything */
}

struct pike_string* my_result_and_clean(struct jpeg_compress_struct *cinfo)
{
   struct my_destination_mgr *dm=(struct my_destination_mgr *)cinfo->dest;

   if (dm->buf)
   {
      struct pike_string *ps;
      ps=make_shared_binary_string(dm->buf,
				   (char*)dm->pub.next_output_byte-(char*)dm->buf);
      
      free(dm->buf);
      dm->buf=NULL;
      return ps;
   }
   return make_shared_string("");
}

int parameter_int(struct svalue *map,struct pike_string *what,INT32 *p)
{
   struct svalue *v;
   v=low_mapping_string_lookup(map->u.mapping,what);

   if (!v || v->type!=T_INT) return 0;

   *p=v->u.integer;
   return 1;
}



struct my_source_mgr
{
   struct jpeg_source_mgr pub;
   struct pike_string *str;
};

static void my_init_source(struct jpeg_decompress_struct *cinfo)
{
  struct my_source_mgr *sm=(struct my_source_mgr *)cinfo->src;
  
  sm->pub.next_input_byte=(JOCTET*)sm->str->str;
  sm->pub.bytes_in_buffer=sm->str->len;
}

static boolean my_fill_input_buffer(struct jpeg_decompress_struct *cinfo)
{
   static unsigned char my_eoi[2]={0xff,JPEG_EOI};

   struct my_source_mgr *sm=(struct my_source_mgr *)cinfo->src;

   sm->pub.next_input_byte=my_eoi;
   sm->pub.bytes_in_buffer=2;

   return TRUE; /* hope this works */
}

static void my_skip_input_data(struct jpeg_decompress_struct *cinfo,
			       long num_bytes)
{
   struct my_source_mgr *sm=(struct my_source_mgr *)cinfo->src;
 
   if (((unsigned long)num_bytes)>sm->pub.bytes_in_buffer)
      num_bytes=sm->pub.bytes_in_buffer;

   sm->pub.next_input_byte += (size_t) num_bytes;
   sm->pub.bytes_in_buffer -= (size_t) num_bytes;
}

static void my_term_source(struct jpeg_decompress_struct *cinfo)
{
   /* nop */
}

/*
**! method string encode(object image)
**! method string encode(object image, mapping options)
**! 	Encodes a JPEG image. 
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

static void image_jpeg_encode(INT32 args)
{
   struct jpeg_error_mgr errmgr;
   struct my_destination_mgr destmgr;
   struct jpeg_compress_struct cinfo;

   struct image *img;

   unsigned char *tmp;
   INT32 y;
   rgb_group *s;
   JSAMPROW row_pointer[8];

   if (args<1 
       || sp[-args].type!=T_OBJECT
       || !(img=(struct image*)
	    get_storage(sp[-args].u.object,image_program))
       || (args>1 && sp[1-args].type!=T_MAPPING))
      error("Image.JPEG.encode: Illegal arguments\n");


   if (!img->img)
      error("Image.JPEG.encode: Given image is empty.\n");

   tmp=malloc(img->xsize*3*8);
   if (!tmp) 
      error("Image.JPEG.encode: out of memory\n");

   /* init jpeg library objects */

   jpeg_std_error(&errmgr);

   errmgr.error_exit=my_error_exit;
   errmgr.emit_message=my_emit_message;
   errmgr.output_message=my_output_message;

   destmgr.pub.init_destination=my_init_destination;
   destmgr.pub.empty_output_buffer=my_empty_output_buffer;
   destmgr.pub.term_destination=my_term_destination;

   cinfo.err=&errmgr;

   jpeg_create_compress(&cinfo);

   cinfo.dest=(struct jpeg_destination_mgr*)&destmgr;

   cinfo.image_width=img->xsize;
   cinfo.image_height=img->ysize;
   cinfo.input_components=3;     /* 1 */
   cinfo.in_color_space=JCS_RGB; /* JCS_GRAYSVALE */

   jpeg_set_defaults(&cinfo);

   cinfo.optimize_coding=(img->xsize*img->ysize)<50000;

   /* check configuration */

   if (args>1)
   {
      INT32 p,q;

      p=0;
      q=75;
      if (parameter_int(sp+1-args,param_baseline,&p)
	  || parameter_int(sp+1-args,param_quality,&q))
      {
	 if (q<0) q=0; else if (q>100) q=100;
	 jpeg_set_quality(&cinfo,q,!!p);
      }

      if (parameter_int(sp+1-args,param_optimize,&p))
      {
	 cinfo.optimize_coding=!!p;
      }

      if (parameter_int(sp+1-args,param_smoothing,&p))
      {
	 if (p<1) p=1; else if (p>100) p=100;
	 cinfo.smoothing_factor=p;
      }

      
      if (parameter_int(sp+1-args,param_x_density,&p) &&
	  parameter_int(sp+1-args,param_y_density,&q))
      {
	 cinfo.X_density=p;
	 cinfo.Y_density=q;
	 cinfo.density_unit=1;
      }

      if (parameter_int(sp+1-args,param_density,&p))
      {
	 cinfo.X_density=p;
	 cinfo.Y_density=p;
	 cinfo.density_unit=1;
      }

      if (parameter_int(sp+1-args,param_density_unit,&p))
	 cinfo.density_unit=p;

      if (parameter_int(sp+1-args,param_method,&p)
	  && (p==JDCT_IFAST ||
	      p==JDCT_FLOAT ||
	      p==JDCT_DEFAULT ||
	      p==JDCT_ISLOW ||
	      p==JDCT_FASTEST))
	 cinfo.dct_method=p;
      
      if (parameter_int(sp+1-args,param_progressive,&p))
	 jpeg_simple_progression(&cinfo);
   }

   jpeg_start_compress(&cinfo, TRUE);
   
   y=img->ysize;
   s=img->img;

   while (y)
   {
      int n,i,y2=y;
      if (y2>8) y2=8;
      n=img->xsize*y2; 
      i=0;
      while (n--)
	 tmp[i++]=s->r, tmp[i++]=s->g, tmp[i++]=s->b, s++;

      row_pointer[0]=tmp;
      row_pointer[1]=tmp+img->xsize*3;
      row_pointer[2]=tmp+img->xsize*3*2;
      row_pointer[3]=tmp+img->xsize*3*3;
      row_pointer[4]=tmp+img->xsize*3*4;
      row_pointer[5]=tmp+img->xsize*3*5;
      row_pointer[6]=tmp+img->xsize*3*6;
      row_pointer[7]=tmp+img->xsize*3*7;
      jpeg_write_scanlines(&cinfo, row_pointer, y2);
      
      y-=y2;
   }

   free(tmp);

   jpeg_finish_compress(&cinfo);

   pop_n_elems(args);
   push_string(my_result_and_clean(&cinfo));

   jpeg_destroy_compress(&cinfo);
}

/*
**! method object decode(string data)
**! method object decode(string data, mapping options)
**! 	Decodes a JPEG image. 
**!
**!     The <tt>options</tt> argument may be a mapping
**!	containing zero or more encoding options:
**!
**!	<pre>
**!	advanced options:
**!	    "block_smoothing":0|1
**!		Do interblock smoothing. Default is on (1).
**!	    "fancy_upsampling":0|1
**!		Do fancy upsampling of chroma components. 
**!		Default is on (1).
**!	    "method":JPEG.IFAST|JPEG.ISLOW|JPEG.FLOAT|JPEG.DEFAULT|JPEG.FASTEST
**!		DCT method to use.
**!		DEFAULT and FASTEST is from the jpeg library,
**!		probably ISLOW and IFAST respective.
**!
**!	wizard options:
**!	    "scale_num":1..
**!	    "scale_denom":1..
**!	        Rescale the image when read from JPEG data.
**!		My (Mirar) version (6a) of jpeglib can only handle
**!		1/1, 1/2, 1/4 and 1/8. 
**!
**!	</pre>
**!
**! note
**!	Please read some about JPEG files. 
*/

static void image_jpeg_decode(INT32 args)
{
   struct jpeg_error_mgr errmgr;
   struct my_source_mgr srcmgr;
   struct jpeg_decompress_struct cinfo;

   struct object *o;
   struct image *img;

   unsigned char *tmp,*s;
   INT32 y;
   rgb_group *d;
   JSAMPROW row_pointer[8];

   if (args<1 
       || sp[-args].type!=T_STRING
       || (args>1 && sp[1-args].type!=T_MAPPING))
      error("Image.JPEG.decode: Illegal arguments\n");

   /* init jpeg library objects */

   jpeg_std_error(&errmgr);

   errmgr.error_exit=my_error_exit;
   errmgr.emit_message=my_emit_message;
   errmgr.output_message=my_output_message;

   srcmgr.pub.init_source=my_init_source;
   srcmgr.pub.fill_input_buffer=my_fill_input_buffer;
   srcmgr.pub.skip_input_data=my_skip_input_data;
   srcmgr.pub.resync_to_restart=jpeg_resync_to_restart;
   srcmgr.pub.term_source=my_term_source;
   srcmgr.str=sp[-args].u.string;

   cinfo.err=&errmgr;

   jpeg_create_decompress(&cinfo);

   cinfo.src=(struct jpeg_source_mgr*)&srcmgr;

   jpeg_read_header(&cinfo,TRUE);

   /* we can only handle RGB */

   cinfo.out_color_space=JCS_RGB;

   /* check configuration */

   if (args>1)
   {
      INT32 p,q;

      p=0;
      q=75;
      if (parameter_int(sp+1-args,param_method,&p)
	  && (p==JDCT_IFAST ||
	      p==JDCT_FLOAT ||
	      p==JDCT_DEFAULT ||
	      p==JDCT_ISLOW ||
	      p==JDCT_FASTEST))
	 cinfo.dct_method=p;

      if (parameter_int(sp+1-args,param_fancy_upsampling,&p))
	 cinfo.do_fancy_upsampling=!!p;
      
      if (parameter_int(sp+1-args,param_block_smoothing,&p))
	 cinfo.do_block_smoothing=!!p;
      
      if (parameter_int(sp+1-args,param_scale_denom,&p)
	 &&parameter_int(sp+1-args,param_scale_num,&q))
	 cinfo.scale_num=q,
	 cinfo.scale_denom=p;
   }

   jpeg_start_decompress(&cinfo);

   o=clone_object(image_program,0);
   img=(struct image*)get_storage(o,image_program);
   if (!img) error("image no image? foo?\n"); /* should never happen */
   img->img=malloc(sizeof(rgb_group)*
		   cinfo.output_width*cinfo.output_height);
   if (!img->img)
   {
      jpeg_destroy((struct jpeg_common_struct*)&cinfo);
      free_object(o);
      error("Image.JPEG.decode: out of memory\n");
   }
   img->xsize=cinfo.output_width;
   img->ysize=cinfo.output_height;

   tmp=malloc(8*cinfo.output_width*cinfo.output_components);
   if (!tmp)
   {
      jpeg_destroy((struct jpeg_common_struct*)&cinfo);
      free_object(o);
      error("Image.JPEG.decode: out of memory\n");
   }
   
   y=img->ysize;
   d=img->img;

   while (y)
   {
      int n,m;

      if (y<8) n=y; else n=8;

      row_pointer[0]=tmp;
      row_pointer[1]=tmp+img->xsize*3;
      row_pointer[2]=tmp+img->xsize*3*2;
      row_pointer[3]=tmp+img->xsize*3*3;
      row_pointer[4]=tmp+img->xsize*3*4;
      row_pointer[5]=tmp+img->xsize*3*5;
      row_pointer[6]=tmp+img->xsize*3*6;
      row_pointer[7]=tmp+img->xsize*3*7;

      n=jpeg_read_scanlines(&cinfo, row_pointer, n);
      /* read 8 rows */

      s=tmp;
      m=img->xsize*n;
      while (m--)
	 d->r=*(s++),
	 d->g=*(s++),
	 d->b=*(s++),d++;

      y-=n;
   }

   free(tmp);

   jpeg_finish_decompress(&cinfo);

   pop_n_elems(args);
   push_object(o);

   jpeg_destroy_decompress(&cinfo);
}

#endif /* HAVE_JPEGLIB_H */

/*** module init & exit & stuff *****************************************/

void f_index(INT32 args);


void pike_module_exit(void)
{
   free_string(param_baseline);
   free_string(param_quality);
   free_string(param_optimize);
   free_string(param_smoothing);
   free_string(param_x_density);
   free_string(param_y_density);
   free_string(param_density);
   free_string(param_density_unit);
   free_string(param_method);
   free_string(param_progressive);
   free_string(param_fancy_upsampling);
   free_string(param_block_smoothing);
   free_string(param_scale_denom);
   free_string(param_scale_num);
}

void pike_module_init(void)
{
#ifdef HAVE_JPEGLIB_H
#ifdef DYNAMIC_MODULE
   push_string(make_shared_string("Image"));
   push_int(0);
   SAFE_APPLY_MASTER("resolv",2);
   if (sp[-1].type==T_OBJECT) 
   {
      push_string(make_shared_string("image"));
      f_index(2);
      image_program=program_from_svalue(sp-1);
   }
   pop_n_elems(1);
#endif /* DYNAMIC_MODULE */

   if (image_program)
   {
      add_function("decode",image_jpeg_decode,
		   "function(string,void|mapping(string:int):object)",0);
      add_function("encode",image_jpeg_encode,
		   "function(object,void|mapping(string:int):string)",0);

      push_int(JDCT_IFAST);
      add_constant(make_shared_string("IFAST"),sp-1,0);
      pop_stack();
      push_int(JDCT_FLOAT);
      add_constant(make_shared_string("FLOAT"),sp-1,0);
      pop_stack();
      push_int(JDCT_DEFAULT);
      add_constant(make_shared_string("DEFAULT"),sp-1,0);
      pop_stack();
      push_int(JDCT_ISLOW);
      add_constant(make_shared_string("ISLOW"),sp-1,0);
      pop_stack();
      push_int(JDCT_FASTEST);
      add_constant(make_shared_string("FASTEST"),sp-1,0);
      pop_stack();
   }

#endif /* HAVE_JPEGLIB_H */

   param_baseline=make_shared_string("baseline");
   param_quality=make_shared_string("quality");
   param_optimize=make_shared_string("optimize");
   param_smoothing=make_shared_string("smoothing");
   param_x_density=make_shared_string("x_density");
   param_y_density=make_shared_string("y_density");
   param_density=make_shared_string("density");
   param_density_unit=make_shared_string("density_unit");
   param_method=make_shared_string("method");
   param_progressive=make_shared_string("progressive");
   param_scale_denom=make_shared_string("scale_denom");
   param_scale_num=make_shared_string("scale_num");
   param_fancy_upsampling=make_shared_string("fancy_upsampling");
   param_block_smoothing=make_shared_string("block_smoothing");
}
