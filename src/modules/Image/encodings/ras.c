/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: ras.c,v 1.24 2005/01/23 13:30:04 nilsson Exp $
*/

/*
**! module Image
**! submodule RAS
**!
**!	This submodule keep the RAS encode/decode capabilities
**!	of the <ref>Image</ref> module.
**!
**! see also: Image, Image.Image, Image.Colortable
*/
#include "global.h"

#include "stralloc.h"
#include "object.h"
#include "interpret.h"
#include "svalue.h"
#include "mapping.h"
#include "pike_error.h"
#include "builtin_functions.h"
#include "module_support.h"
#include "operators.h"
#include "program.h"

#include "image.h"
#include "colortable.h"

#include "encodings.h"


#define sp Pike_sp

extern struct program *image_colortable_program;
extern struct program *image_program;


struct rasterfile {
  INT32 ras_magic;              /* magic number */
  INT32 ras_width;              /* width (pixels) of image */
  INT32 ras_height;             /* height (pixels) of image */
  INT32 ras_depth;              /* depth (1, 8, or 24 bits) of pixel */
  INT32 ras_length;             /* length (bytes) of image */
  INT32 ras_type;               /* type of file; see RT_* below */
  INT32 ras_maptype;            /* type of colormap; see RMT_* below */
  INT32 ras_maplength;          /* length (bytes) of following map */
  /* color map follows for ras_maplength bytes, followed by image */
};

#define RT_OLD          0       /* Raw pixrect image in 68000 byte order */
#define RT_STANDARD     1       /* Raw pixrect image in 68000 byte order */
#define RT_BYTE_ENCODED 2       /* Run-length compression of bytes */
#define RT_FORMAT_RGB   3       /* XRGB or RGB instead of XBGR or BGR */
#define RT_FORMAT_TIFF  4       /* tiff <-> standard rasterfile */
#define RT_FORMAT_IFF   5       /* iff (TAAC format) <-> standard rasterfile */
#define RT_EXPERIMENTAL 0xffff  /* Reserved for testing */

#define RMT_NONE        0       /* ras_maplength is expected to be 0 */
#define RMT_EQUAL_RGB   1       /* red[ras_maplength/3],green[],blue[] */
#define RMT_RAW         2


/** decoding *****************************************/

/*
**! method object decode(string data)
**!	Decodes RAS data and creates an image object.
**! 	
**! see also: encode
**!
**! note
**!	This function may throw errors upon illegal RAS data.
**!
**! returns the decoded image as an image object
*/

static void decode_ras_header(struct rasterfile *rs, unsigned char *p)
{
  INT32 *rp = (INT32*)rs;
  int i;
  for(i=0; i<8; i++) {
    *rp++ = (((p[0]&0x80)? p[0]-0x100:p[0])<<24)|(p[1]<<16)|(p[2]<<8)|p[3];
    p += 4;
  }
}

static ptrdiff_t unpack_rle(unsigned char *src, ptrdiff_t srclen,
			    unsigned char *dst, ptrdiff_t dstlen)
{
  unsigned char *dst0 = dst;
  while(srclen>0 && dstlen>0)
    if((*dst++ = *src++) == 0x80 &&
       --srclen && *src++!=0 && --srclen) {
      int n = src[-1];
      int c = *src++;
      --dst;
      while(n-->=0 && dstlen>0) {
	*dst++ = c;
	--dstlen;
      }
      --srclen;
    } else {
      --srclen;
      --dstlen;
    }
  return dst-dst0;
}

void img_ras__decode(INT32 args)
{
  /* Double check args to give the correct function name in the error
     messages. */
  if(args<1)
    SIMPLE_TOO_FEW_ARGS_ERROR("Image.RAS._decode", 1);
  if(Pike_sp[-1].type!=T_STRING)
    SIMPLE_BAD_ARG_ERROR("Image.RAS._decode", 1, "string");
  img_ras_decode(args);
  push_constant_text("image");
  stack_swap();
  push_constant_text("format");
  push_constant_text("image/x-sun-raster");
  f_aggregate_mapping(4);
}

void img_ras_decode(INT32 args)
{
   struct pike_string *str;
   struct rasterfile rs;
   struct object *o, *ctab=NULL;
   struct image *img;
   rgb_group *rgb;
   unsigned char *src, *tmpdata=NULL;
   ptrdiff_t len;
   INT32 x, y;
   size_t numcolors = 0;
   struct nct_flat_entry *entries = NULL;

   get_all_args("Image.RAS.decode", args, "%S", &str);

   if(str->len < 32)
     Pike_error("Image.RAS.decode: header too small\n");

   decode_ras_header(&rs, STR0(str));

   if(rs.ras_magic != 0x59a66a95)
     Pike_error("Image.RAS.decode: bad magic\n");

   if(rs.ras_type < 0 || rs.ras_type > RT_BYTE_ENCODED)
     Pike_error("Image.RAS.decode: unsupported ras_type %d\n", rs.ras_type);

   if(rs.ras_maptype < 0 || rs.ras_maptype > RMT_EQUAL_RGB)
     Pike_error("Image.RAS.decode: unsupported ras_maptype %d\n", rs.ras_maptype);

   if(rs.ras_depth != 1 && rs.ras_depth != 8 && rs.ras_depth != 24)
     Pike_error("Image.RAS.decode: unsupported ras_depth %d\n", rs.ras_depth);

   if(rs.ras_width < 0)
     Pike_error("Image.RAS.decode: negative ras_width\n");

   if(rs.ras_height < 0)
     Pike_error("Image.RAS.decode: negative ras_height\n");

   if(rs.ras_length < 0)
     Pike_error("Image.RAS.decode: negative ras_length\n");

   if(rs.ras_maplength < 0)
     Pike_error("Image.RAS.decode: negative ras_maplength\n");

   src = (unsigned char *)(STR0(str)+32);
   len = str->len - 32;

   if(rs.ras_maplength != 0) {

     unsigned char *map = src;

     if(len < rs.ras_maplength)
       Pike_error("Image.RAS.decode: colormap truncated\n");
     
     src += rs.ras_maplength;
     len -= rs.ras_maplength;
     if(len && (rs.ras_maplength&1)) {
       src++;
       --len;
     }

     switch(rs.ras_maptype) {
      case RMT_NONE:
	Pike_error("Image.RAS.decode: RMT_NONE colormap has length != 0 ( == %d )\n", rs.ras_maplength);
	break;
      case RMT_EQUAL_RGB:
	{
	  INT32 col, ncol = rs.ras_maplength / 3;
	  unsigned char *red = map;
	  unsigned char *green = red + ncol;
	  unsigned char *blue = green + ncol;
	  for(col=0; col<ncol; col++) {
	    push_int(*red++);
	    push_int(*green++);
	    push_int(*blue++);
	    f_aggregate(3);
	  }
	  f_aggregate(ncol);
	  ctab = clone_object(image_colortable_program,1);
	}
	break;
     }

   }

   if(rs.ras_length) {
     if(rs.ras_length > len) {
       /* Better to proceed and make a partly black image? */
       if(ctab != NULL)
	 free_object(ctab);
       Pike_error("Image.RAS.decode: image data truncated\n");
     } else
       len = rs.ras_length;
   }

   if( (double)((rs.ras_width+1)&~1)*3*(double)rs.ras_height > (double)INT_MAX )
       Pike_error("Too large RAS image (overflow imminent)");

   if(rs.ras_type == RT_BYTE_ENCODED) {
     INT32 img_sz = 0;
     switch(rs.ras_depth) {
      case 1:
	img_sz = ((rs.ras_width+15)>>4)*2*rs.ras_height;
	break;
      case 8:
	img_sz = ((rs.ras_width+1)&~1)*rs.ras_height;
	break;
      case 24:
	img_sz = ((rs.ras_width+1)&~1)*3*rs.ras_height;
	break;
     }
     tmpdata = (unsigned char *)xalloc(img_sz);
     len = unpack_rle(src, len, tmpdata, img_sz);
     src = tmpdata;
   }

   push_int(rs.ras_width);
   push_int(rs.ras_height);
   o=clone_object(image_program,2);
   img=(struct image*)get_storage(o,image_program);
   rgb=img->img;
   if(ctab != NULL) {
     struct neo_colortable *ctable =
       (struct neo_colortable*)get_storage(ctab, image_colortable_program);
     if(ctable!=NULL && ctable->type==NCT_FLAT) {
       numcolors = ctable->u.flat.numentries;
       entries = ctable->u.flat.entries;
     }
   }

   for(y=0; y<rs.ras_height; y++)
     switch(rs.ras_depth) {
      case 24:
	for(x=0; x<rs.ras_width; x++) {
	  if(len<3) {
	    /* Better to proceed and make a partly black image? */
	    if(tmpdata != NULL)
	      free((char *)tmpdata);
	    if(ctab != NULL)
	      free_object(ctab);
	    free_object(o);
	    Pike_error("Image.RAS.decode: image data too short\n");
	  }
	  rgb->b = *src++;
	  rgb->g = *src++;
	  rgb->r = *src++;
	  rgb++;
	  len -= 3;
	}
	if(rs.ras_width & 1) {
	  src++;
	  --len;
	}
	break;
      case 8:
	for(x=0; x<rs.ras_width; x++) {
	  if(len<1) {
	    /* Better to proceed and make a partly black image? */
	    if(tmpdata != NULL)
	      free((char *)tmpdata);
	    if(ctab != NULL)
	      free_object(ctab);
	    free_object(o);
	    Pike_error("Image.RAS.decode: image data too short\n");
	  }
	  if(*src<numcolors)
	    *rgb++ = entries[*src++].color;
	  else {
	    rgb++;
	    src++;
	  }
	  --len;
	}
	if(rs.ras_width & 1) {
	  src++;
	  --len;
	}
	break;
      case 1:
	{
	  int bits = 0, data = 0;
	  for(x=0; x<rs.ras_width; x++) {
	    if(!bits) {
	      if(len<2) {
		/* Better to proceed and make a partly black image? */
		if(tmpdata != NULL)
		  free((char *)tmpdata);
		if(ctab != NULL)
		  free_object(ctab);
		free_object(o);
		Pike_error("Image.RAS.decode: image data too short\n");
	      }
	      data = (src[0]<<8)|src[1];
	      src += 2;
	      len -= 2;
	      bits = 16;
	    }
	    if(data&0x8000)
	      if(numcolors>1)
		*rgb = entries[1].color;
	      else
		rgb->r = rgb->g = rgb->b = 0;
	    else
	      if(numcolors>0)
		*rgb = entries[0].color;
	      else
		rgb->r = rgb->g = rgb->b = 0xff;
	    rgb++;
	    data<<=1;
	    --bits;
	  }
	  if(rs.ras_width & 1) {
	    src++;
	    --len;
	  }
	}
	break;
     }

   if(tmpdata != NULL)
     free((char *)tmpdata);
   if(ctab != NULL)
     free_object(ctab);
   pop_n_elems(args);
   push_object(o);
}


/** encoding *****************************************/

/*
**! method string encode(object image)
**! method string encode(object image, mapping options)
**! 	Encodes a RAS image. 
**!
**!     The <tt>options</tt> argument may be a mapping
**!	containing zero or more encoding options:
**!
**!	<pre>
**!	normal options:
**!
**!	    "palette":colortable object
**!		Use this as palette for pseudocolor encoding
**!
**!	</pre>
*/

static void encode_ras_header(struct rasterfile *rs, unsigned char *p)
{
  INT32 *rp = (INT32*)rs;
  int i;
  for(i=0; i<8; i++) {
    *p++ = (*rp>>24)&0xff;
    *p++ = (*rp>>16)&0xff;
    *p++ = (*rp>>8)&0xff;
    *p++ = (*rp)&0xff;
    rp ++;
  }
}

static ptrdiff_t pack_rle(unsigned char *src, ptrdiff_t srclen,
			  unsigned char *dst, ptrdiff_t dstlen)
{
  unsigned char *dst0 = dst;
  while(srclen>0 && dstlen>0) {
    int run;
    for(run=1; run<srclen && src[run]==*src && run<256; run++);
    if(run>3 || *src==0x80)
      if(run==1 && *src==0x80) {
	if(dstlen<2) break;
	*dst++ = 0x80;
	*dst++ = 0x00;
	dstlen -= 2;
	src++;
	--srclen;
      } else {
	if(dstlen<3) break;
	*dst++ = 0x80;
	*dst++ = run-1;
	*dst++ = *src;
	dstlen -= 3;
	src += run;
	srclen -= run;
      }
    else {
	*dst++ = *src++;
	--srclen;
	--dstlen;
      }
  }
  return dst-dst0;
}

static void img_ras_encode(INT32 args)
{
  struct object *imgo;
  struct mapping *optm = NULL;
  struct image *alpha = NULL, *img;
  struct neo_colortable *ct = NULL;
  struct pike_string *res, *res2;
  struct rasterfile rs;
  struct nct_dither dith;
  rgb_group *rgb;
  INT32 x, y, llen = 0;
  unsigned char *dst;
  void (*ctfunc)(rgb_group *, unsigned char *, int,
		 struct neo_colortable *, struct nct_dither *, int) = NULL;

  get_all_args("Image.RAS.decode", args,
	       (args>1 && !UNSAFE_IS_ZERO(&sp[1-args])? "%o%m":"%o"),
	       &imgo, &optm);

  if((img=(struct image*)get_storage(imgo, image_program))==NULL)
     Pike_error("Image.RAS.encode: illegal argument 1\n");

  if(optm != NULL) {
    struct svalue *s;

    if((s=simple_mapping_string_lookup(optm, "palette"))!=NULL && !UNSAFE_IS_ZERO(s))
      if(s->type != T_OBJECT ||
	 (ct=(struct neo_colortable*)
	  get_storage(s->u.object, image_colortable_program))==NULL)
	Pike_error("Image.RAS.encode: option (arg 2) \"palette\" has illegal type\n");
  }

  if (!img->img)
    Pike_error("Image.RAS.encode: no image\n");

  rgb = img->img;

  if(ct && ct->type == NCT_NONE)
    ct = NULL;

  rs.ras_magic = 0x59a66a95;
  rs.ras_width = img->xsize;
  rs.ras_height = img->ysize;
  rs.ras_depth = 0;
  rs.ras_length = 0;
  rs.ras_type = RT_STANDARD;
  rs.ras_maptype = RMT_NONE;
  rs.ras_maplength = 0;

  if(ct) {
    struct pike_string *cts;
    ptrdiff_t i, n = image_colortable_size(ct);
    unsigned char *tmp;
    rs.ras_depth = 8;
    rs.ras_maptype = RMT_EQUAL_RGB;
    rs.ras_maplength = DO_NOT_WARN((INT32)n*3);
    cts = begin_shared_string(rs.ras_maplength+(rs.ras_maplength&1));
    if(rs.ras_maplength & 1) {
      STR0(cts)[rs.ras_maplength] = '\0';
      rs.ras_maplength++;
    }
    tmp = (unsigned char *)xalloc(rs.ras_maplength);
    image_colortable_write_rgb(ct, tmp);
    for(i=0; i<n; i++) {
      STR0(cts)[i] = tmp[i*3];
      STR0(cts)[i+n] = tmp[i*3+1];
      STR0(cts)[i+2*n] = tmp[i*3+2];
    }
    free((char *)tmp);
    push_string(end_shared_string(cts));
    image_colortable_initiate_dither(ct, &dith, img->xsize);
    ctfunc = image_colortable_index_8bit_function(ct);
  } else
    push_text("");

  if(!rs.ras_depth) {
    INT32 px = img->xsize * img->ysize;
    while(px--)
      if((rgb[px].r != 0 || rgb[px].g != 0 || rgb[px].b != 0) &&
	 (rgb[px].r != 255 || rgb[px].g != 255 || rgb[px].b != 255)) {
	rs.ras_depth = 24;
	break;
      }
    if(!rs.ras_depth)
      rs.ras_depth = 1;
  }

  switch(rs.ras_depth) {
   case 1:
     llen = ((img->xsize+15)>>4)*2;
     break;
   case 8:
     llen = ((img->xsize+1)&~1);
     break;
   case 24:
     llen = ((img->xsize+1)&~1)*3;
     break;
  }
  
  rs.ras_length = llen*img->ysize;

  res2 = begin_shared_string(rs.ras_length);
  dst = (unsigned char *)STR0(res2);
  for(y=0; y<img->ysize; y++)
    switch(rs.ras_depth) {
     case 1:
       for(x=0; x<img->xsize; x+=16) {
	 int bit, data = 0;
	 INT32 xx=x;
	 for(bit=0x8000; bit!=0 && xx<img->xsize; xx++, (bit>>=1))
	   if((rgb++)->r == 0)
	     data |= bit;
	 *dst++ = (data>>8)&0xff;
	 *dst++ = data&0xff;
       }
       break;
     case 8:
       ctfunc(rgb, dst, img->xsize, ct, &dith, img->xsize);
       rgb += img->xsize;
       dst += img->xsize;
       if(img->xsize&1)
	 *dst++ = 0;
       break;
     case 24:
       for(x=0; x<img->xsize; x++) {
	 *dst++ = rgb->b;
	 *dst++ = rgb->g;
	 *dst++ = rgb->r;
	 rgb++;
       }
       if(img->xsize&1)
	 *dst++ = 0;
       break;
    }
  if(ct != NULL)
    image_colortable_free_dither(&dith);

  {
    unsigned char *pkdata = (unsigned char *)xalloc(rs.ras_length+16);
    unsigned char *pk = pkdata, *src = STR0(res2);
    ptrdiff_t pklen = 0, pkleft = rs.ras_length+16;
    for(y=0; y<img->ysize; y++) {
      ptrdiff_t n = pack_rle(src, llen, pk, pkleft);
      src += llen;
      pk += n;
      pkleft -= n;
      if((pklen += n)>rs.ras_length)
	break;
    }
      
    if(pklen<rs.ras_length) {
      free((char *)res2);
      res2 = make_shared_binary_string((char *)pkdata, pklen);
      rs.ras_length = DO_NOT_WARN((INT32)pklen);
      rs.ras_type = RT_BYTE_ENCODED;
    } else
      res2 = end_shared_string(res2);

    free(pkdata);
  }

  res = begin_shared_string(32);
  encode_ras_header(&rs, STR0(res));
  push_string(end_shared_string(res));
  stack_swap();
  push_string(res2);
  f_add(3);
  res = (--sp)->u.string;
  pop_n_elems(args);
  push_string(res);
}


/** module *******************************************/

void init_image_ras(void)
{
  ADD_FUNCTION("decode",img_ras_decode, tFunc(tStr,tObj), 0);
  ADD_FUNCTION("_decode",img_ras__decode, tFunc(tStr,tMapping), 0);
  ADD_FUNCTION("encode",img_ras_encode,
	       tFunc(tObj tOr(tVoid,tMap(tStr,tMix)),tStr), 0);
}

void exit_image_ras(void)
{
}
