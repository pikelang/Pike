/*
 * $Id: image_jpeg.c,v 1.28 2000/03/09 14:05:03 jhs Exp $
 */

#include "global.h"

#include "config.h"

#if !defined(HAVE_LIBJPEG)
#undef HAVE_JPEGLIB_H
#endif

#ifdef HAVE_JPEGLIB_H

#define FILE void
#define size_t unsigned int
/* NOTE: INT32 and INT16 are redefined by <jmorecfg.h>. */
#if 0
#ifdef INT16
#undef INT16
#endif /* INT16 */
#ifdef INT32
#undef INT32
#endif
#endif /* 0 */

#define XMD_H /* Avoid INT16 / INT32 being redefined */

#include <jpeglib.h>
#undef size_t
#undef FILE
#undef _SIZE_T_DEFINED
#undef _FILE_DEFINED

#endif /* HAVE_JPEGLIB_H */

#ifdef HAVE_STDLIB_H
#undef HAVE_STDLIB_H
#endif
RCSID("$Id: image_jpeg.c,v 1.28 2000/03/09 14:05:03 jhs Exp $");

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
#include "threads.h"
#include "builtin_functions.h"
#include "module_support.h"

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
static struct pike_string *param_quant_tables;

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

static int parameter_int(struct svalue *map,struct pike_string *what,INT32 *p)
{
   struct svalue *v;
   v=low_mapping_string_lookup(map->u.mapping,what);

   if (!v || v->type!=T_INT) return 0;

   *p=v->u.integer;
   return 1;
}

static int store_int_in_table(struct array *a,
			      int len,
			      unsigned int *d)
{
   int i;
   int z=0;
   for (i=0; i<a->size && len; i++)
      if (a->item[i].type==T_ARRAY)
      {
	 int n;
	 n=store_int_in_table(a->item[i].u.array,len,d);
	 d+=n;
	 len-=n;
	 z+=n;
      }
      else if (a->item[i].type==T_INT)
      {
	 *(d++)=(unsigned int)(a->item[i].u.integer);
	 len--;
	 z++;
      }
   return z;
}

static int parameter_qt(struct svalue *map,struct pike_string *what,
			j_compress_ptr cinfo)
{
   struct svalue *v;
   unsigned int table[DCTSIZE2];
   struct mapping *m;
   INT32 e;
   struct keypair *k;

   v=low_mapping_string_lookup(map->u.mapping,what);

   if (!v) return 0;
   else if (v->type!=T_MAPPING) 
      error("Image.JPEG.encode: illegal value of option quant_table; expected mapping\n");

   m=v->u.mapping;
   MAPPING_LOOP(m)
      {
	 int z;
	 if (k->ind.type!=T_INT || k->val.type!=T_ARRAY)
	    error("Image.JPEG.encode: illegal value of option quant_table; expected mapping(int:array)\n");

	 if (k->ind.u.integer<0 || k->ind.u.integer>=NUM_QUANT_TBLS)
	    error("Image.JPEG.encode: illegal value of option quant_table; expected mapping(int(0..%d):array)\n",NUM_QUANT_TBLS-1);

	 if ((z=store_int_in_table(k->val.u.array,DCTSIZE2,table))!=
	     DCTSIZE2)
	    error("Image.JPEG.encode: illegal value of option quant_table;"
		  " quant_table %d array is of illegal size (%d), "
		  "expected %d integers\n",
		  k->ind.u.integer,z,DCTSIZE2);

	 jpeg_add_quant_table(cinfo,k->ind.u.integer,table,100,0);
      }

   return 1;
}

static int parameter_qt_d(struct svalue *map,struct pike_string *what,
			  struct jpeg_decompress_struct *cinfo)
{
   struct svalue *v;
   unsigned int table[DCTSIZE2];
   struct mapping *m;
   INT32 e;
   struct keypair *k;

   v=low_mapping_string_lookup(map->u.mapping,what);

   if (!v) return 0;
   else if (v->type!=T_MAPPING) 
      error("Image.JPEG.encode: illegal value of option quant_table; expected mapping\n");

   m=v->u.mapping;
   MAPPING_LOOP(m)
      {
	 int z;
	 if (k->ind.type!=T_INT || k->val.type!=T_ARRAY)
	    error("Image.JPEG.encode: illegal value of option quant_table; expected mapping(int:array)\n");

	 if (k->ind.u.integer<0 || k->ind.u.integer>=NUM_QUANT_TBLS)
	    error("Image.JPEG.encode: illegal value of option quant_table; expected mapping(int(0..%d):array)\n",NUM_QUANT_TBLS-1);

	 if ((z=store_int_in_table(k->val.u.array,DCTSIZE2,table))!=
	     DCTSIZE2)
	    error("Image.JPEG.encode: illegal value of option quant_table;"
		  " quant_table %d array is of illegal size (%d), "
		  "expected %d integers\n",
		  k->ind.u.integer,z,DCTSIZE2);

	 /* jpeg_add_quant_table(cinfo,k->ind.u.integer,table,100,0); */

	 do /* ripped from jpeg_add_quant_table */
	 {
	    JQUANT_TBL ** qtblptr = & cinfo->quant_tbl_ptrs[k->ind.u.integer];
	    int i;
	    long temp;

	    if (*qtblptr == NULL)
	       *qtblptr = jpeg_alloc_quant_table((j_common_ptr) cinfo);

	    for (i = 0; i < DCTSIZE2; i++) {
	       temp = table[i];
	       /* limit the values to the valid range */
	       if (temp <= 0L) temp = 1L;
	       if (temp > 32767L) temp = 32767L; 
	       (*qtblptr)->quantval[i] = (UINT16) temp;
	    }

	    (*qtblptr)->sent_table = FALSE;
	 }
	 while (0);
      }

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
**!	    "density_unit":int
**!	    "x_density":int
**!	    "y_density":int
**!		density of image; unit is 1:dpi 2:dpcm 0:no units
**!
**!	wizard options:
**!	    "baseline":0|1
**!		Force baseline output. Useful for quality&lt;25.
**!		Default is on for quality&lt;25.
**!	    "quant_tables":mapping(int,array(array(int)))
**!		Tune quantisation tables manually.
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
   cinfo.in_color_space=JCS_RGB; /* JCS_GRAYSCALE */

   jpeg_set_defaults(&cinfo);

   cinfo.optimize_coding=(img->xsize*img->ysize)<50000;

   /* check configuration */

   if (args>1)
   {
      INT32 p,q=95;

      p=-1;
      if (parameter_int(sp+1-args,param_quality,&q)) 
	 if (q<25) p=1; else p=0;
      if (parameter_int(sp+1-args,param_baseline,&p) || p!=-1)
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

      parameter_qt(sp+1-args,param_quant_tables,&cinfo);
   }

   jpeg_start_compress(&cinfo, TRUE);
   
   y=img->ysize;
   s=img->img;

   THREADS_ALLOW();
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
   THREADS_DISALLOW();

   free(tmp);

   jpeg_finish_compress(&cinfo);

   pop_n_elems(args);
   push_string(my_result_and_clean(&cinfo));

   jpeg_destroy_compress(&cinfo);
}

/*
**! method object decode(string data)
**! method object decode(string data, mapping options)
**! method mapping _decode(string data)
**! method mapping _decode(string data, mapping options)
**! method mapping decode_header(string data)
**! 	Decodes a JPEG image. The simple <ref>decode</ref> function
**!	simply gives the image object, the other functions gives
**!	a mapping of information (see below)
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
**!	<ref>_decode</ref> and <ref>decode_header</ref> gives
**!	a mapping as result, with this content:
**!
**!	<pre>
**!	    "xsize":int
**!	    "ysize":int
**!		size of image
**!	    "xdpi":float
**!	    "ydpi":float
**!		image dpi, if known
**!	    "type":"image/jpeg"
**!		file type information as MIME type
**!
**!	JPEG specific:
**!	    "num_compontents":int
**!		number of channels in JPEG image
**!	    "color_space":"GRAYSCALE"|"RGB"|"YUV"|"CMYK"|"YCCK"|"UNKNOWN"
**!		color space of JPEG image
**!	    "density_unit":int
**!	    "x_density":int
**!	    "y_density":int
**!		density of image; unit is 1:dpi 2:dpcm 0:no units
**!	    "adobe_marker":0|1
**!		if the file has an adobe marker
**!	</pre>
**!
**! note
**!	Please read some about JPEG files. 
*/

enum { IMG_DECODE_MUCH,IMG_DECODE_IMAGE,IMG_DECODE_HEADER };

static void img_jpeg_decode(INT32 args,int mode)
{
   struct jpeg_error_mgr errmgr;
   struct my_source_mgr srcmgr;
   struct jpeg_decompress_struct cinfo;

   struct object *o=NULL;
   struct image *img=NULL;

   unsigned char *tmp,*s;
   INT32 y;
   rgb_group *d;
   JSAMPROW row_pointer[8];

   int n=0;

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

   /* we can only handle RGB or GRAYSCALE */

   if (cinfo.jpeg_color_space==JCS_GRAYSCALE)
      cinfo.out_color_space=JCS_GRAYSCALE;
   else
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

      parameter_qt_d(sp+1-args,param_quant_tables,&cinfo);
   }

   if (mode!=IMG_DECODE_IMAGE)
   {
      /* standard header info */

      push_text("type"); n++;
      push_text("image/jpeg");

      push_text("xsize"); n++;
      push_int(cinfo.image_width);

      push_text("ysize"); n++;
      push_int(cinfo.image_height);

      push_text("xdpi"); n++;
      push_text("ydpi"); n++;
      switch (cinfo.density_unit)
      {
	 default:
	    pop_n_elems(2); n-=2;
	    break;
	 case 1:
	    push_float( cinfo.X_density );
	    stack_swap();
	    push_float( cinfo.Y_density );
	    break;
	 case 2:
	    push_float( cinfo.X_density/2.54 );
	    stack_swap();
	    push_float( cinfo.Y_density/2.54 );
	    break;
      }

      /* JPEG special header */

      push_text("num_components"); n++;
      push_int(cinfo.num_components);

      push_text("color_space"); n++;
      switch (cinfo.jpeg_color_space)
      {
	 case JCS_UNKNOWN:	push_text("UNKNOWN"); break;
	 case JCS_GRAYSCALE:	push_text("GRAYSCALE"); break;
	 case JCS_RGB:		push_text("RGB"); break;
	 case JCS_YCbCr:	push_text("YUV"); break;
	 case JCS_CMYK:		push_text("CMYK"); break;
	 case JCS_YCCK:		push_text("YCCK"); break;
	 default:		push_text("?"); break;
      }

      push_text("density_unit"); n++;
      push_int(cinfo.density_unit);

      push_text("x_density"); n++;
      push_int(cinfo.X_density);

      push_text("y_density"); n++;
      push_int(cinfo.Y_density);

      push_text("adobe_marker"); n++;
      push_int(cinfo.saw_Adobe_marker);
   }

   if (mode!=IMG_DECODE_HEADER)
   {
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

      THREADS_ALLOW();
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
	 if (cinfo.out_color_space==JCS_RGB)
	    while (m--)
	       d->r=*(s++),
		  d->g=*(s++),
		  d->b=*(s++),d++;
	 else
	    while (m--)
	       d->r=d->g=d->b=*(s++),d++;
	 y-=n;
      }
      THREADS_DISALLOW();

      free(tmp);

      if (mode!=IMG_DECODE_IMAGE)
      {
	 int i,m,j;
	 push_text("quant_tables");
	 for (i=m=0; i<NUM_QUANT_TBLS; i++)
	 {
	    if (cinfo.quant_tbl_ptrs[i])
	    {
	       push_int(i);
	       for (j=0; j<DCTSIZE2; j++)
	       {
		  push_int(cinfo.quant_tbl_ptrs[i]->quantval[j]);
		  if (!((j+1)%DCTSIZE))
		     f_aggregate(DCTSIZE);
	       }
	       f_aggregate(DCTSIZE);
	       m++;
	    }
	 }
	 f_aggregate_mapping(m*2);
	 n++;
      }

      jpeg_finish_decompress(&cinfo);
      jpeg_destroy_decompress(&cinfo);
   }

   if (mode==IMG_DECODE_IMAGE)
   {
      pop_n_elems(args);
      push_object(o);
      return;
   }
   else if (mode==IMG_DECODE_MUCH)
   {
      push_text("image");
      push_object(o);
      n++;
   }

   f_aggregate_mapping(n*2);
   while (args--)
   {
      stack_swap();
      pop_stack();
   }
}

void image_jpeg_decode(INT32 args)
{
   img_jpeg_decode(args,IMG_DECODE_IMAGE);
}

void image_jpeg_decode_header(INT32 args)
{
   img_jpeg_decode(args,IMG_DECODE_HEADER);
}

void image_jpeg__decode(INT32 args)
{
   img_jpeg_decode(args,IMG_DECODE_MUCH);
}

void image_jpeg_quant_tables(INT32 args)
{
   struct jpeg_error_mgr errmgr;
   struct my_destination_mgr destmgr;
   struct jpeg_compress_struct cinfo;
   int i,m,j;

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

   cinfo.image_width=17;
   cinfo.image_height=17;
   cinfo.input_components=3;     
   cinfo.in_color_space=JCS_RGB; 

   if (args)
   {
      int q;
      get_all_args("Image.JPEG.quant_tables",args,"%i",&q);
      jpeg_set_quality(&cinfo,q,0);
   }

   for (i=m=0; i<NUM_QUANT_TBLS; i++)
   {
      if (cinfo.quant_tbl_ptrs[i])
      {
	 push_int(i);
	 for (j=0; j<DCTSIZE2; j++)
	 {
	    push_int(cinfo.quant_tbl_ptrs[i]->quantval[j]);
	    if (!((j+1)%DCTSIZE))
	       f_aggregate(DCTSIZE);
	 }
	 f_aggregate(DCTSIZE);
	 m++;
      }
   }
   f_aggregate_mapping(m*2);

   jpeg_destroy_compress(&cinfo);
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
   free_string(param_quant_tables);
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
#define tOptions tMap(tStr,tOr(tInt,tMap(tInt,tArr(tOr(tInt,tArr(tInt))))))
      ADD_FUNCTION("decode",image_jpeg_decode,tFunc(tStr tOr(tVoid,tOptions),tObj),0);
      ADD_FUNCTION("_decode",image_jpeg__decode,tFunc(tStr tOr(tVoid,tOptions),tObj),0);
      ADD_FUNCTION("decode_header",image_jpeg_decode_header,tFunc(tStr tOr(tVoid,tOptions),tObj),0);
      ADD_FUNCTION("encode",image_jpeg_encode,tFunc(tObj tOr(tVoid,tOptions),tStr),0);
   }

   add_integer_constant("IFAST", JDCT_IFAST, 0);
   add_integer_constant("FLOAT", JDCT_FLOAT, 0);
   add_integer_constant("DEFAULT", JDCT_DEFAULT, 0);
   add_integer_constant("ISLOW", JDCT_ISLOW, 0);
   add_integer_constant("FASTEST", JDCT_FASTEST, 0);

   ADD_FUNCTION("quant_tables",image_jpeg_quant_tables,
		tFunc(tOr(tVoid,tInt),tMap(tInt,tArr(tArr(tInt)))),0);

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
   param_quant_tables=make_shared_string("quant_tables");
   param_block_smoothing=make_shared_string("block_smoothing");
}
