/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: image_jpeg.c,v 1.70 2005/04/08 17:19:51 grubba Exp $
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

/* FAR is defined by windef.h and jmorecfg.h */
#ifdef FAR
#undef FAR
#endif

#ifdef HAVE_JCONFIG_H_HAVE_BOOLEAN
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#ifdef HAVE_WTYPES_H
/* jconfig.h has probably been compiled without WIN32_LEAN_AND_MEAN...
 * So we need this one to get the boolean typedef.
 */
#include <wtypes.h>
#endif
#endif
#endif

#include <jpeglib.h>
#include "transupp.h" /* Support routines for jpeg transformations */

#undef size_t
#undef FILE
#undef _SIZE_T_DEFINED
#undef _FILE_DEFINED

#endif /* HAVE_JPEGLIB_H */

#ifdef HAVE_STDLIB_H
#undef HAVE_STDLIB_H
#endif

/* jpeglib defines EXTERN for some reason.
 * This is not good, since it confuses compilation.h.
 */
#ifdef EXTERN
#undef EXTERN
#endif

#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "threads.h"
#include "array.h"
#include "mapping.h"
#include "pike_error.h"
#include "stralloc.h"
#include "threads.h"
#include "builtin_functions.h"
#include "module_support.h"
#include "operators.h"

#define sp Pike_sp

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
static struct pike_string *param_grayscale;
static struct pike_string *param_marker;
static struct pike_string *param_comment;
static struct pike_string *param_transform;

static const int reverse_quality[101]=
{
   3400,3400,1700,1133,850,680,566,486,425,377,340,309,283,261,243,
   226,212,200,188,179,170,162,154,148,141,136,131,126,121,117,113,109,
   106,103,100,97,94,92,89,87,85,82,81,79,77,75,73,72,71,69,68,67,65,64,
   63,61,60,58,57,56,54,53,52,50,49,48,46,45
};

#ifdef HAVE_JPEGLIB_H

/*! @module Image
 */

/*! @module JPEG
 *!
 *! @note
 *!	This module uses @tt{libjpeg@}, a software from
 *!	Independent JPEG Group.
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
   Pike_error("Image.JPEG: fatal error in libjpeg; %s\n",buffer);
}

static void my_emit_message(struct jpeg_common_struct *cinfo,int msg_level)
{
   /* no trace */
}

struct my_decompress_struct
{
   struct jpeg_decompress_struct cinfo;
   struct my_marker
   {
      struct my_marker *next;
      INT32 id;
      INT32 len;
      unsigned char data[1];
   } *first_marker;
};

/* jpeg_getc is from jpeg-6b/djpeg.c */
static unsigned int jpeg_getc (j_decompress_ptr cinfo)
{
  struct jpeg_source_mgr * datasrc = cinfo->src;

  if (datasrc->bytes_in_buffer == 0)
    if (! (*datasrc->fill_input_buffer) (cinfo))
       return 0; /* ignore the problem */

  datasrc->bytes_in_buffer--;
  return GETJOCTET(*datasrc->next_input_byte++);
}

/* examine_app14 from jpeg6b/jdmarker.c */
static void examine_app14 (j_decompress_ptr cinfo, JOCTET FAR * data,
			  unsigned int datalen)
{
  unsigned int version, flags0, flags1, transform;

  if (datalen >= 12 &&
      GETJOCTET(data[0]) == 0x41 &&
      GETJOCTET(data[1]) == 0x64 &&
      GETJOCTET(data[2]) == 0x6F &&
      GETJOCTET(data[3]) == 0x62 &&
      GETJOCTET(data[4]) == 0x65) {
    /* Found Adobe APP14 marker */
    version = (GETJOCTET(data[5]) << 8) + GETJOCTET(data[6]);
    flags0 = (GETJOCTET(data[7]) << 8) + GETJOCTET(data[8]);
    flags1 = (GETJOCTET(data[9]) << 8) + GETJOCTET(data[10]);
    transform = GETJOCTET(data[11]);
    cinfo->saw_Adobe_marker = TRUE;
    cinfo->Adobe_transform = (UINT8) transform;
  }
}

static boolean my_jpeg_marker_parser(j_decompress_ptr cinfo)
{
   INT32 length;
   struct my_decompress_struct *mds=
      (struct my_decompress_struct*)cinfo;
   struct my_marker *mm;
   char *d;

   length=jpeg_getc(cinfo)<<8;
   length|=jpeg_getc(cinfo);
   length-=2;

   mm=(struct my_marker*)xalloc(sizeof(struct my_marker)+length);
   mm->id=cinfo->unread_marker;
   mm->len=length;
   mm->next=mds->first_marker;
   mds->first_marker=mm;

   d = (char *)mm->data;
   while (length--) *(d++)=(char)jpeg_getc(cinfo);

   if (mm->id==JPEG_APP0+14)
      examine_app14(cinfo,mm->data,mm->len);

   return 1;
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
   size_t pos;
   char *new;

   pos=dm->len; /* foo! dm->len-dm->pub.free_in_buffer; */
   new=(char*)realloc(dm->buf,dm->len+BUF_INCREMENT);
   if (!new) return FALSE;

   dm->buf = new;
   dm->len += BUF_INCREMENT;
   dm->pub.free_in_buffer = DO_NOT_WARN(dm->len - pos);
   dm->pub.next_output_byte = (JOCTET*)new + pos;

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
   struct mapping_data *md;
   INT32 e;
   struct keypair *k;

   v=low_mapping_string_lookup(map->u.mapping,what);

   if (!v) return 0;
   else if (v->type!=T_MAPPING) 
      Pike_error("Image.JPEG.encode: illegal value of option quant_table; expected mapping\n");

   md=v->u.mapping->data;
   NEW_MAPPING_LOOP(md)
      {
	 int z;
	 if (k->ind.type!=T_INT || k->val.type!=T_ARRAY)
	    Pike_error("Image.JPEG.encode: illegal value of option quant_table; expected mapping(int:array)\n");

	 if (k->ind.u.integer<0 || k->ind.u.integer>=NUM_QUANT_TBLS)
	    Pike_error("Image.JPEG.encode: illegal value of option quant_table; expected mapping(int(0..%d):array)\n",NUM_QUANT_TBLS-1);

	 if ((z=store_int_in_table(k->val.u.array,DCTSIZE2,table))!=
	     DCTSIZE2)
	    Pike_error("Image.JPEG.encode: illegal value of option quant_table;"
		       " quant_table %"PRINTPIKEINT"d array is of illegal size (%d), "
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
   struct mapping_data *md;
   INT32 e;
   struct keypair *k;

   v=low_mapping_string_lookup(map->u.mapping,what);

   if (!v) return 0;
   else if (v->type!=T_MAPPING) 
      Pike_error("Image.JPEG.encode: illegal value of option quant_table; expected mapping\n");

   md=v->u.mapping->data;
   NEW_MAPPING_LOOP(md)
      {
	 int z;
	 if (k->ind.type!=T_INT || k->val.type!=T_ARRAY)
	    Pike_error("Image.JPEG.encode: illegal value of option quant_table; expected mapping(int:array)\n");

	 if (k->ind.u.integer<0 || k->ind.u.integer>=NUM_QUANT_TBLS)
	    Pike_error("Image.JPEG.encode: illegal value of option quant_table; expected mapping(int(0..%d):array)\n",NUM_QUANT_TBLS-1);

	 if ((z=store_int_in_table(k->val.u.array,DCTSIZE2,table))!=
	     DCTSIZE2)
	    Pike_error("Image.JPEG.encode: illegal value of option quant_table;"
		       " quant_table %"PRINTPIKEINT"d array is of illegal size (%d), "
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

static int parameter_marker(struct svalue *map,struct pike_string *what,
			    struct jpeg_compress_struct *cinfo)
{
   struct svalue *v;
   struct mapping_data *md;
   INT32 e;
   struct keypair *k;

   v=low_mapping_string_lookup(map->u.mapping,what);

   if (!v) return 0;
   else if (v->type!=T_MAPPING) 
      Pike_error("Image.JPEG.encode: illegal value of option marker;"
		 " expected mapping\n");
   md=v->u.mapping->data;
   NEW_MAPPING_LOOP(md)
      {
	 if (k->ind.type!=T_INT || k->val.type!=T_STRING ||
	     k->val.u.string->size_shift)
	    Pike_error("Image.JPEG.encode: illegal value of option "
		       "marker; expected mapping(int:8 bit string)\n");
	 jpeg_write_marker(cinfo, k->ind.u.integer, 
			   (const unsigned char *)k->val.u.string->str,
			   k->val.u.string->len); 
      }

   return 1;
}

static int parameter_comment(struct svalue *map,struct pike_string *what,
			     struct jpeg_compress_struct *cinfo)
{
   struct svalue *v;

   v=low_mapping_string_lookup(map->u.mapping,what);

   if (!v) return 0;
   else if (v->type!=T_STRING || v->u.string->size_shift) 
      Pike_error("Image.JPEG.encode: illegal value of option comment;"
		 " expected 8 bit string\n");

   jpeg_write_marker(cinfo, JPEG_COM, 
		     (const unsigned char *)v->u.string->str,
		     v->u.string->len); 

   return 1;
}

struct my_source_mgr
{
   struct jpeg_source_mgr pub;
   struct pike_string *str;
};

static void my_init_source(struct jpeg_decompress_struct *cinfo)
{
  struct my_source_mgr *sm = (struct my_source_mgr *)cinfo->src;
  
  sm->pub.next_input_byte = (JOCTET*)sm->str->str;
  sm->pub.bytes_in_buffer = DO_NOT_WARN(sm->str->len);
}

static boolean my_fill_input_buffer(struct jpeg_decompress_struct *cinfo)
{
   static const unsigned char my_eoi[2]={0xff,JPEG_EOI};

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

static int marker_exists_in_args(INT32 args, int which)
{
   struct svalue *map=sp+1-args;
   struct svalue *v=NULL;
   struct mapping_data *md;
   INT32 e;
   struct keypair *k;
   v=low_mapping_string_lookup(map->u.mapping, param_comment);
   if (which==JPEG_COM && v) {
     return (v->type==T_STRING && !v->u.string->size_shift);
   } else {
     v=low_mapping_string_lookup(map->u.mapping, param_marker);
     if (v && v->type==T_MAPPING) {
       md=v->u.mapping->data;
       NEW_MAPPING_LOOP(md)
       {
	 if (k->ind.type==T_INT && k->val.type==T_STRING
	     && k->ind.u.integer==which && !k->val.u.string->size_shift) {
	   return 1;
	 }
       }
     }
   }
   return 0;
}

static void my_copy_jpeg_markers(INT32 args, 
				 struct my_decompress_struct *mds,
				 j_compress_ptr cinfo)
{
    while (mds->first_marker) {
	struct my_marker *mm=mds->first_marker;
	if (args < 2 || !marker_exists_in_args(args, mm->id)) {
	  jpeg_write_marker(cinfo, mm->id,
			    mm->data,
			    mm->len); 
	}
	mds->first_marker=mm->next;
	free(mm);
    }
}

static void init_src(struct pike_string *raw_img,
		     struct jpeg_error_mgr *errmgr,
		     struct my_source_mgr *srcmgr,
                     struct my_decompress_struct *mds)
{
   mds->first_marker=NULL;

   jpeg_std_error(errmgr);

   errmgr->error_exit=my_error_exit;
   errmgr->emit_message=my_emit_message;
   errmgr->output_message=my_output_message;

   srcmgr->pub.init_source=my_init_source;
   srcmgr->pub.fill_input_buffer=my_fill_input_buffer;
   srcmgr->pub.skip_input_data=my_skip_input_data;
   srcmgr->pub.resync_to_restart=jpeg_resync_to_restart;
   srcmgr->pub.term_source=my_term_source;
   srcmgr->str=raw_img;

   mds->cinfo.err=errmgr;

   jpeg_create_decompress(&mds->cinfo);

   jpeg_set_marker_processor(&mds->cinfo, JPEG_COM, my_jpeg_marker_parser);
   jpeg_set_marker_processor(&mds->cinfo, JPEG_APP0+1, my_jpeg_marker_parser);
   jpeg_set_marker_processor(&mds->cinfo, JPEG_APP0+2, my_jpeg_marker_parser);
   jpeg_set_marker_processor(&mds->cinfo, JPEG_APP0+3, my_jpeg_marker_parser);
   jpeg_set_marker_processor(&mds->cinfo, JPEG_APP0+4, my_jpeg_marker_parser);
   jpeg_set_marker_processor(&mds->cinfo, JPEG_APP0+5, my_jpeg_marker_parser);
   jpeg_set_marker_processor(&mds->cinfo, JPEG_APP0+6, my_jpeg_marker_parser);
   jpeg_set_marker_processor(&mds->cinfo, JPEG_APP0+7, my_jpeg_marker_parser);
   jpeg_set_marker_processor(&mds->cinfo, JPEG_APP0+8, my_jpeg_marker_parser);
   jpeg_set_marker_processor(&mds->cinfo, JPEG_APP0+9, my_jpeg_marker_parser);
   jpeg_set_marker_processor(&mds->cinfo, JPEG_APP0+10, my_jpeg_marker_parser);
   jpeg_set_marker_processor(&mds->cinfo, JPEG_APP0+11, my_jpeg_marker_parser);
   jpeg_set_marker_processor(&mds->cinfo, JPEG_APP0+12, my_jpeg_marker_parser);
   jpeg_set_marker_processor(&mds->cinfo, JPEG_APP0+13, my_jpeg_marker_parser);
   jpeg_set_marker_processor(&mds->cinfo, JPEG_APP0+14, my_jpeg_marker_parser);
   jpeg_set_marker_processor(&mds->cinfo, JPEG_APP0+15, my_jpeg_marker_parser);

   mds->cinfo.src=(struct jpeg_source_mgr*)srcmgr;
   /*jcopy_markers_setup(&mds->cinfo, JCOPYOPT_ALL);*/
   jpeg_read_header(&mds->cinfo,TRUE);
}

#ifdef TRANSFORMS_SUPPORTED
void set_jpeg_transform_options(INT32 args, jpeg_transform_info *options)
{
    int transform = 0;
    if (args > 1 && parameter_int(sp+1-args,param_transform,&transform) &&
	((transform == JXFORM_FLIP_H) || 
	(transform == JXFORM_FLIP_V) || 
	(transform == JXFORM_NONE) || 
	(transform == JXFORM_ROT_90) || 
	(transform == JXFORM_ROT_180) || 
	(transform == JXFORM_ROT_270) || 
	(transform == JXFORM_TRANSPOSE) || 
	(transform == JXFORM_TRANSVERSE))) {
	options->transform = transform;
    } else {
	options->transform = JXFORM_NONE;
    }
    options->trim = FALSE;
    options->force_grayscale = FALSE;
    options->crop = FALSE;
}
#endif /*TRANSFORMS_SUPPORTED*/

/*! @decl string encode(object image)
 *! @decl string encode(string|object image, mapping options)
 *! Encodes an @[image] object with JPEG compression. The image
 *! may also be a string containing a raw JPEG image. In the
 *! The @[options] argument may be a mapping containing zero or more
 *! encoding options:
 *!
 *!
 *!
 *! @mapping
 *!   @member int(0..100) "quality"
 *!     Set quality of result. Default is 75.
 *!   @member int(0..1) "optimize"
 *!     Optimize Huffman table. Default is on (1) for
 *!     images smaller than 50kpixels.
 *!   @member int(0..1) "progressive"
 *!     Make a progressive JPEG. Default is off (0).
 *!   @member int(0..1) "grayscale"
 *!     Make a grayscale JPEG instead of color (YCbCr).
 *!   @member int(1..100) "smooth"
 *!     Smooth input. Value is strength.
 *!   @member int "method"
 *!     DCT method to use. Any of
 *!     @[IFAST], @[ISLOW], @[FLOAT], @[DEFAULT] or @[FASTEST].
 *!     @[DEFAULT] and @[FASTEST] is from the jpeg library,
 *!     probably @[ISLOW] and @[IFAST] respectively.
 *!   @member int(0..2) "density_unit"
 *!     The unit used for x_density and y_density.
 *!     @int
 *!       @value 0
 *!         No unit
 *!       @value 1
 *!         dpi
 *!       @value 2
 *!         dpcm
 *!     @endint
 *!   @member int "x_density"
 *!   @member int "y_density"
 *!     Density of image.
 *!   @member string "comment"
 *!     Comment to be written in the JPEG file. Must not be a wide string.
 *!   @member int(0..1) "baseline"
 *!     Force baseline output. Useful for quality<25.
 *!     Default is off for quality<25.
 *!   @member mapping(int:array(array(int))) "quant_tables"
 *!     Tune quantisation tables manually.
 *!   @member mapping(int:string) "marker"
 *!     Application and comment markers;
 *!     the integer should be one of @[Marker.COM], @[Marker.APP0],
 *!     @[Marker.APP1], ..., @[Marker.APP15]. The string is up to the application;
 *!     most notable are Adobe and Photoshop markers.
 *!   @member int "transform"
 *!     Lossless image transformation. Has only effect when supplying a
 *!     JPEG file as indata.
 *!     @int
 *!       @value FLIP_H
 *!         Flip image horizontally
 *!       @value FLIP_V
 *!         Flip image vertically
 *!       @value ROT_90
 *!         Rotate image 90 degrees clockwise
 *!       @value ROT_180
 *!         Rotate image 180 degrees clockwise
 *!       @value ROT_270
 *!         Rotate image 270 degrees clockwise
 *!       @value TRANSPOSE
 *!         Transpose image
 *!       @value TRANSVERSE
 *!         Transverse image
 *!       @endint
 *! @endmapping
 *!
 *! @note
 *!   Please read some about JPEG files. A quality
 *!   setting of 100 does not mean the result is lossless.
 */

static void image_jpeg_encode(INT32 args)
{
   struct jpeg_error_mgr errmgr;
   struct my_destination_mgr destmgr;
   struct jpeg_compress_struct cinfo;
   struct image *img = NULL;

   struct my_source_mgr srcmgr;
   struct my_decompress_struct mds;

   unsigned char *tmp = NULL;
   INT32 y;
   rgb_group *s;
   JSAMPROW row_pointer[8];

   if (args<1 
       || (sp[-args].type!=T_OBJECT && sp[-args].type!=T_STRING)
       || (sp[-args].type==T_OBJECT &&
	   !(img=(struct image*) get_storage(sp[-args].u.object,image_program)))
       || (args>1 && sp[1-args].type!=T_MAPPING))
      Pike_error("Image.JPEG.encode: Illegal arguments\n");

   if (img) {
       /* Compression from Image.Image object */
       if (!img->img)
	   Pike_error("Image.JPEG.encode: Given image is empty.\n");
       
       tmp=malloc(img->xsize*3*8);
       if (!tmp) 
	   Pike_error("Image.JPEG.encode: out of memory\n");
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
   } else {
       /* "Compression" from JPEG block */
#ifdef TRANSFORMS_SUPPORTED
       jpeg_transform_info transformoption;
       jvirt_barray_ptr *src_coef_arrays, *dst_coef_arrays;
#else
       Pike_error("Image.JPEG.encode: Raw JPEG data transformation not allowed"
                  " when Pike is compiled with this version of libjpeg.\n"); 
#endif /*TRANSFORMS_SUPPORTED*/

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

       init_src(sp[-args].u.string, &errmgr, &srcmgr, &mds);

#ifdef TRANSFORMS_SUPPORTED
       set_jpeg_transform_options(args, &transformoption);
       jtransform_request_workspace(&mds.cinfo, &transformoption);
       src_coef_arrays = jpeg_read_coefficients(&mds.cinfo);

       jpeg_copy_critical_parameters(&mds.cinfo, &cinfo);

       dst_coef_arrays = jtransform_adjust_parameters(&mds.cinfo, &cinfo,
						      src_coef_arrays,
						      &transformoption);
       jpeg_write_coefficients(&cinfo, dst_coef_arrays);
#endif /*TRANSFORMS_SUPPORTED*/

       my_copy_jpeg_markers(args, &mds, &cinfo);
       /*jcopy_markers_execute(&mds.cinfo, &cinfo, JCOPYOPT_ALL);*/
#ifdef TRANSFORMS_SUPPORTED
       jtransform_execute_transformation(&mds.cinfo, &cinfo,
					 src_coef_arrays,
					 &transformoption);
#endif /*TRANSFORMS_SUPPORTED*/
   }

   /* check configuration */
   if (args>1)
   {
      INT32 p,q=95;

      p=-1;
      if (parameter_int(sp+1-args,param_quality,&q)) {
	 if (q<25) p=0; else p=1;
      }
      if (parameter_int(sp+1-args,param_baseline,&p) || p!=-1)
      {
	 if (q<0) q=0; else if (q>100) q=100;
	 jpeg_set_quality(&cinfo, q, (boolean)(!!p));
      }

      if (parameter_int(sp+1-args,param_grayscale,&p) && p)
      {
	 jpeg_set_colorspace(&cinfo,JCS_GRAYSCALE);
	 cinfo.input_components=3;     /* 1 */
	 cinfo.in_color_space=JCS_RGB; /* JCS_GRAYSCALE */
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
      
      if (parameter_int(sp+1-args,param_progressive,&p) && p)
	 jpeg_simple_progression(&cinfo);

      parameter_qt(sp+1-args,param_quant_tables,&cinfo);
   }

   if (img) {
       jpeg_start_compress(&cinfo, TRUE);
   }

   if (args>1)
   {
      parameter_comment(sp+1-args,param_comment,&cinfo);
      parameter_marker(sp+1-args,param_marker,&cinfo);
   }

   if (img) {
       /* Compression from Image.Image object */
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
   } else {
       /* "Compression" from JPEG block */

   }
   jpeg_finish_compress(&cinfo);
   
   pop_n_elems(args);
   push_string(my_result_and_clean(&cinfo));

   if (!img) {
     /* remove extra workspace if data was read from JPEG string */
     jpeg_finish_decompress(&mds.cinfo);
     jpeg_destroy_decompress(&mds.cinfo);
   }
   jpeg_destroy_compress(&cinfo);
}

/*! @decl object decode(string data)
 *! @decl object decode(string data, mapping options)
 *! @decl mapping _decode(string data)
 *! @decl mapping _decode(string data, mapping options)
 *! @decl mapping decode_header(string data)
 *! Decodes a JPEG image. The simple @[decode] function
 *! simply gives the image object, the other functions gives
 *! a mapping of information (see below).
 *!
 *! The @[options] argument may be a mapping
 *! containing zero or more decoding options:
 *!
 *! @mapping
 *!   @member int(0..1) "block_smoothing"
 *!     Do interblock smoothing. Default is on (1).
 *!   @member int(0..1) "fancy_upsampling"
 *!     Do fancy upsampling of chroma components.
 *!     Default is on (1).
 *!   @member int "method"
 *!     DCT method to use. Any of
 *!     @[IFAST], @[ISLOW], @[FLOAT], @[DEFAULT] or @[FASTEST].
 *!     @[DEFAULT] and @[FASTEST] is from the jpeg library,
 *!     probably @[ISLOW] and @[IFAST] respective.
 *!   @member int(1..) "scale_num"
 *!   @member int(1..) "scale_denom"
 *!     Rescale the image when read from JPEG data.
 *!     My (Mirar) version (6a) of jpeglib can only handle
 *!     1/1, 1/2, 1/4 and 1/8.
 *! @endmapping
 *!
 *! @[_decode] and @[decode_header] gives
 *! a mapping as result, with this content:
 *!
 *! @mapping
 *!   @member string "comment"
 *!     Comment marker of JPEG file, if present.
 *!   @member int "xsize"
 *!   @member int "ysize"
 *!     Size of image
 *!   @member float "xdpi"
 *!   @member float "ydpi"
 *!     Image dpi, if known.
 *!   @member string "type"
 *!     File type information as MIME type. Always "image/jpeg".
 *!   @member int "num_compontents"
 *!     Number of channels in JPEG image.
 *!   @member string "color_space"
 *!     Color space of JPEG image. Any of "GRAYSCALE", "RGB", "YUV",
 *!     "CMYK", "YCCK" or "UNKNOWN".
 *!   @member int(0..2) "density_unit"
 *!     The unit used for x_density and y_density.
 *!     @int
 *!       @value 0
 *!         No unit
 *!       @value 1
 *!         dpi
 *!       @value 2
 *!         dpcm
 *!     @endint
 *!   @member int "x_density"
 *!   @member int "y_density"
 *!     Density of image.
 *!   @member int(0..1) "adobe_marker"
 *!     If the file has an Adobe marker.
 *!   @member mapping(int:array(array(int))) "quant_tables"
 *!     JPEG quant tables.
 *!   @member int(0..100) "quality"
 *!     JPEG quality guess.
 *! @endmapping
 */

enum { IMG_DECODE_MUCH,IMG_DECODE_IMAGE,IMG_DECODE_HEADER };

static void img_jpeg_decode(INT32 args,int mode)
{
   struct jpeg_error_mgr errmgr;
   struct my_source_mgr srcmgr;
   struct my_decompress_struct mds;

   struct object *o=NULL;
   struct image *img=NULL;

   unsigned char *tmp,*s;
   INT32 y;
   rgb_group *d;
   JSAMPROW row_pointer[8];

   int n=0,m;

   if (args<1 
       || sp[-args].type!=T_STRING
       || (args>1 && sp[1-args].type!=T_MAPPING))
      Pike_error("Image.JPEG.decode: Illegal arguments\n");

   /* init jpeg library objects */
   
   init_src(sp[-args].u.string, &errmgr, &srcmgr, &mds);

   /* we can only handle RGB or GRAYSCALE */

/* don't know about this code; the jpeg library handles 
   RGB destination for GRAYSCALE / Mirar 2001-05-19 */
/*     if (mds.cinfo.jpeg_color_space==JCS_GRAYSCALE) */
/*        mds.cinfo.out_color_space=JCS_GRAYSCALE; */
/*     else */

   mds.cinfo.out_color_space=JCS_RGB; 

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
	 mds.cinfo.dct_method=p;

      if (parameter_int(sp+1-args,param_fancy_upsampling,&p))
	 mds.cinfo.do_fancy_upsampling=!!p;
      
      if (parameter_int(sp+1-args,param_block_smoothing,&p))
	 mds.cinfo.do_block_smoothing=!!p;
      
      if (parameter_int(sp+1-args,param_scale_denom,&p)
	 &&parameter_int(sp+1-args,param_scale_num,&q))
	 mds.cinfo.scale_num=q,
	 mds.cinfo.scale_denom=p;

      parameter_qt_d(sp+1-args,param_quant_tables,&mds.cinfo);
   }

   if (mode!=IMG_DECODE_IMAGE)
   {
      /* standard header info */

      push_text("type"); n++;
      push_text("image/jpeg");

      push_text("xsize"); n++;
      push_int(mds.cinfo.image_width);

      push_text("ysize"); n++;
      push_int(mds.cinfo.image_height);

      push_text("xdpi"); n++;
      push_text("ydpi"); n++;
      switch (mds.cinfo.density_unit)
      {
	 default:
	    pop_n_elems(2); n-=2;
	    break;
	 case 1:
	    push_float( mds.cinfo.X_density );
	    stack_swap();
	    push_float( mds.cinfo.Y_density );
	    break;
	 case 2:
	    push_float( DO_NOT_WARN((FLOAT_TYPE)(mds.cinfo.X_density/2.54)) );
	    stack_swap();
	    push_float( DO_NOT_WARN((FLOAT_TYPE)(mds.cinfo.Y_density/2.54)) );
	    break;
      }

      /* JPEG special header */

      push_text("num_components"); n++;
      push_int(mds.cinfo.num_components);

      push_text("color_space"); n++;
      switch (mds.cinfo.jpeg_color_space)
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
      push_int(mds.cinfo.density_unit);

      push_text("x_density"); n++;
      push_int(mds.cinfo.X_density);

      push_text("y_density"); n++;
      push_int(mds.cinfo.Y_density);

      push_text("adobe_marker"); n++;
      push_int(mds.cinfo.saw_Adobe_marker);

      ref_push_string(param_marker); n++;
      m=0;
      while (mds.first_marker)
      {
	 struct my_marker *mm=mds.first_marker;
	 push_int(mm->id);
	 push_string(make_shared_binary_string((char *)mm->data, mm->len));
	 m++;
	 mds.first_marker=mm->next;
	 free(mm);
      }
      f_aggregate_mapping(m*2);

      if (m)
      {
	 stack_dup();
	 push_int(JPEG_COM);
	 f_index(2);
	 if (sp[-1].type==T_STRING) 
	 {
	    ref_push_string(param_comment); n++;
	    stack_swap();
	 }
	 else
	    pop_stack();
      }
   }

   while (mds.first_marker)
   {
     struct my_marker *mm=mds.first_marker;
     mds.first_marker=mm->next;
     free(mm);
   }

   if (mode!=IMG_DECODE_HEADER)
   {
      jpeg_start_decompress(&mds.cinfo);

      o=clone_object(image_program,0);
      img=(struct image*)get_storage(o,image_program);
      if (!img) Pike_error("image no image? foo?\n"); /* should never happen */
      img->img=malloc(sizeof(rgb_group)*
		      mds.cinfo.output_width*mds.cinfo.output_height);
      if (!img->img)
      {
	 jpeg_destroy((struct jpeg_common_struct*)&mds.cinfo);
	 free_object(o);
	 Pike_error("Image.JPEG.decode: out of memory\n");
      }
      img->xsize=mds.cinfo.output_width;
      img->ysize=mds.cinfo.output_height;

      tmp=malloc(8*mds.cinfo.output_width*mds.cinfo.output_components);
      if (!tmp)
      {
	 jpeg_destroy((struct jpeg_common_struct*)&mds.cinfo);
	 free_object(o);
	 Pike_error("Image.JPEG.decode: out of memory\n");
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

	 n=jpeg_read_scanlines(&mds.cinfo, row_pointer, n);
	 /* read 8 rows */

	 s=tmp;
	 m=img->xsize*n;
	 if (mds.cinfo.out_color_space==JCS_RGB)
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
	    if (mds.cinfo.quant_tbl_ptrs[i])
	    {
	       push_int(i);
	       for (j=0; j<DCTSIZE2; j++)
	       {
		  push_int(mds.cinfo.quant_tbl_ptrs[i]->quantval[j]);
		  if (!((j+1)%DCTSIZE))
		     f_aggregate(DCTSIZE);
	       }
	       f_aggregate(DCTSIZE);
	       m++;
	    }
	 }
	 f_aggregate_mapping(m*2);
	 n++;

	 if (mds.cinfo.quant_tbl_ptrs[0])
	 {
	    int q=mds.cinfo.quant_tbl_ptrs[0]->quantval[4*DCTSIZE+4];
	    int a=0,b=100,c,w;
	    while (b>a)
	    {
	       c=(b+a)/2;
	       w=reverse_quality[c];
	       if (w==q) break;
	       else if (w>q) a=++c;
	       else b=c;
	    }
	    push_text("quality");
	    push_int(c);
	    n++;
	 }
      }

      jpeg_finish_decompress(&mds.cinfo);
   }
   jpeg_destroy_decompress(&mds.cinfo);

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

/*! @decl mapping(int:array(array(int))) quant_tables(int|void a)
 *! @fixme
 *!   Document this function
 */
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
      INT_TYPE q;
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

/*! @decl constant IFAST
 */

/*! @decl constant FLOAT
 */

/*! @decl constant DEFAULT
 */

/*! @decl constant ISLOW
 */

/*! @decl constant FASTEST
 */

/*! @decl constant FLIP_H
 */

/*! @decl constant FLIP_V
 */

/*! @decl constant ROT_90
 */

/*! @decl constant ROT_180
 */

/*! @decl constant ROT_270
 */

/*! @decl constant TRANSPOSE
 */

/*! @decl constant TRANSVERSE
 */

/*! @class Marker
 */

/*! @decl constant EOI
 */

/*! @decl constant RST0
 */

/*! @decl constant COM
 */

/*! @decl constant APP0
 */

/*! @decl constant APP1
 */

/*! @decl constant APP2
 */

/*! @decl constant APP3
 */

/*! @decl constant APP4
 */

/*! @decl constant APP5
 */

/*! @decl constant APP6
 */

/*! @decl constant APP7
 */

/*! @decl constant APP8
 */

/*! @decl constant APP9
 */

/*! @decl constant APP10
 */

/*! @decl constant APP11
 */

/*! @decl constant APP12
 */

/*! @decl constant APP13
 */

/*! @decl constant APP14
 */

/*! @decl constant APP15
 */

/*! @endclass
 */

/*! @endmodule
 */

/*! @endmodule
 */

/*** module init & exit & stuff *****************************************/


PIKE_MODULE_EXIT
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
   free_string(param_grayscale);
   free_string(param_marker);
   free_string(param_comment);
   free_string(param_transform);
}

PIKE_MODULE_INIT
{
#ifdef HAVE_JPEGLIB_H
#ifdef DYNAMIC_MODULE
   push_text("Image.Image");
   SAFE_APPLY_MASTER("resolv",1);
   if (sp[-1].type==T_PROGRAM)
      image_program=program_from_svalue(sp-1);
   pop_n_elems(1);
#endif /* DYNAMIC_MODULE */

   if (image_program)
   {
#define tOptions tMap(tStr,tOr3(					\
    tInt,								\
    tStr,								\
    tMap(tInt,tOr(tStr,tArr(tOr(tInt,tArr(tInt)))))))

      ADD_FUNCTION("decode",image_jpeg_decode,tFunc(tStr tOr(tVoid,tOptions),tObj),0);
      ADD_FUNCTION("_decode",image_jpeg__decode,tFunc(tStr tOr(tVoid,tOptions),tMap(tStr,tMixed)),0);
      ADD_FUNCTION("decode_header",image_jpeg_decode_header,
		   tFunc(tStr tOr(tVoid,tOptions),tMap(tStr,tOr4(tStr,tInt,tFlt,tMap(tInt,tStr)))),0);
      ADD_FUNCTION("encode",image_jpeg_encode,tFunc(tOr(tObj,tStr) tOr(tVoid,tOptions),tStr),0);
   }

   add_integer_constant("IFAST", JDCT_IFAST, 0);
   add_integer_constant("FLOAT", JDCT_FLOAT, 0);
   add_integer_constant("DEFAULT", JDCT_DEFAULT, 0);
   add_integer_constant("ISLOW", JDCT_ISLOW, 0);
   add_integer_constant("FASTEST", JDCT_FASTEST, 0);
#ifdef TRANSFORMS_SUPPORTED
   add_integer_constant("FLIP_H", JXFORM_FLIP_H, 0);
   add_integer_constant("FLIP_V", JXFORM_FLIP_V, 0);
   add_integer_constant("NONE", JXFORM_NONE, 0);
   add_integer_constant("ROT_90", JXFORM_ROT_90, 0);
   add_integer_constant("ROT_180", JXFORM_ROT_180, 0);
   add_integer_constant("ROT_270", JXFORM_ROT_270, 0);
   add_integer_constant("TRANSPOSE", JXFORM_TRANSPOSE, 0);
   add_integer_constant("TRANSVERSE", JXFORM_TRANSVERSE, 0);
#endif /*TRANSFORMS_SUPPORTED*/
   ADD_FUNCTION("quant_tables",image_jpeg_quant_tables,
		tFunc(tOr(tVoid,tInt),tMap(tInt,tArr(tArr(tInt)))),0);


   start_new_program();

   add_integer_constant("EOI",JPEG_EOI,0);
   add_integer_constant("RST0",JPEG_RST0,0);
   add_integer_constant("COM",JPEG_COM,0);
   add_integer_constant("APP0",JPEG_APP0,0);
   add_integer_constant("APP1",JPEG_APP0+1,0);
   add_integer_constant("APP2",JPEG_APP0+2,0);
   add_integer_constant("APP3",JPEG_APP0+3,0);
   add_integer_constant("APP4",JPEG_APP0+4,0);
   add_integer_constant("APP5",JPEG_APP0+5,0);
   add_integer_constant("APP6",JPEG_APP0+6,0);
   add_integer_constant("APP7",JPEG_APP0+7,0);
   add_integer_constant("APP8",JPEG_APP0+8,0);
   add_integer_constant("APP9",JPEG_APP0+9,0);
   add_integer_constant("APP10",JPEG_APP0+10,0);
   add_integer_constant("APP11",JPEG_APP0+11,0);
   add_integer_constant("APP12",JPEG_APP0+12,0);
   add_integer_constant("APP13",JPEG_APP0+13,0);
   add_integer_constant("APP14",JPEG_APP0+14,0);
   add_integer_constant("APP15",JPEG_APP0+15,0);

   push_program(end_program());
   f_call_function(1);
   simple_add_constant("Marker",sp-1,0);
   pop_stack();

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
   param_grayscale=make_shared_string("grayscale");
   param_marker=make_shared_string("marker");
   param_comment=make_shared_string("comment");
   param_transform=make_shared_string("transform");
}
