/* $Id: ras.c,v 1.1 1999/10/21 22:39:02 marcus Exp $ */

/*
**! module Image
**! note
**!	$Id: ras.c,v 1.1 1999/10/21 22:39:02 marcus Exp $
**! submodule RAS
**!
**!	This submodule keep the RAS encode/decode capabilities
**!	of the <ref>Image</ref> module.
**!
**! see also: Image, Image.Image, Image.Colortable
*/
#include "global.h"

#include "stralloc.h"
RCSID("$Id: ras.c,v 1.1 1999/10/21 22:39:02 marcus Exp $");
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "array.h"
#include "mapping.h"
#include "error.h"
#include "threads.h"
#include "builtin_functions.h"
#include "module_support.h"

#include "image.h"
#include "colortable.h"

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

void img_ras_decode(INT32 args)
{
   struct pike_string *str;
   struct rasterfile rs;
   struct object *o, *ctab=NULL;
   struct image *img;
   rgb_group *rgb;
   unsigned char *src;
   INT32 len, x, y;
   unsigned int numcolors = 0;
   struct nct_flat_entry *entries;

   get_all_args("Image.RAS.decode", args, "%S", &str);

   if(str->len < 32)
     error("Image.RAS.decode: header too small\n");

   decode_ras_header(&rs, str->str);

   if(rs.ras_magic != 0x59a66a95)
     error("Image.RAS.decode: bad magic\n");

   if(rs.ras_type < 0 || rs.ras_type > RT_BYTE_ENCODED)
     error("Image.RAS.decode: unsupported ras_type %d\n", rs.ras_type);

   if(rs.ras_maptype < 0 || rs.ras_maptype > RMT_EQUAL_RGB)
     error("Image.RAS.decode: unsupported ras_maptype %d\n", rs.ras_maptype);

   if(rs.ras_depth != 1 && rs.ras_depth != 8 && rs.ras_depth != 24)
     error("Image.RAS.decode: unsupported ras_depth %d\n", rs.ras_depth);

   if(rs.ras_width < 0)
     error("Image.RAS.decode: negative ras_width\n");

   if(rs.ras_height < 0)
     error("Image.RAS.decode: negative ras_height\n");

   if(rs.ras_length < 0)
     error("Image.RAS.decode: negative ras_length\n");

   if(rs.ras_maplength < 0)
     error("Image.RAS.decode: negative ras_maplength\n");

   src = (unsigned char *)(str->str+32);
   len = str->len - 32;

   if(rs.ras_maplength != 0) {

     unsigned char *map = src;

     if(len < rs.ras_maplength)
       error("Image.RAS.decode: colormap truncated\n");
     
     src += rs.ras_maplength;
     len -= rs.ras_maplength;
     if(len && (rs.ras_maplength&1)) {
       src++;
       --len;
     }

     switch(rs.ras_maptype) {
      case RMT_NONE:
	error("Image.RAS.decode: RMT_NONE colormap has length != 0 ( == %d )\n", rs.ras_maplength);
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

   if(rs.ras_type == RT_BYTE_ENCODED) {
     if(ctab != NULL)
       free_object(ctab);
     error("Image.RAS.decode: RT_BYTE_ENCODED unimplemented\n");
   }

   if(rs.ras_length)
     if(rs.ras_length > len) {
       if(ctab != NULL)
	 free_object(ctab);
       error("Image.RAS.decode: image data truncated\n");
     } else
       len = rs.ras_length;

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
	    if(ctab != NULL)
	      free_object(ctab);
	    free_object(o);
	    error("Image.RAS.decode: image data too short\n");
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
	    if(ctab != NULL)
	      free_object(ctab);
	    free_object(o);
	    error("Image.RAS.decode: image data too short\n");
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
	  int bits = 0, data;
	  for(x=0; x<rs.ras_width; x++) {
	    if(!bits) {
	      if(len<2) {
		if(ctab != NULL)
		  free_object(ctab);
		free_object(o);
		error("Image.RAS.decode: image data too short\n");
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
		rgb->r = rgb->g = rgb->b = ~0;
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
      default:
	if(ctab != NULL)
	  free_object(ctab);
	free_object(o);
	error("Not implemented depth %d\n", rs.ras_depth);
     }

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

static void image_ras_encode(INT32 args)
{
  pop_n_elems(args);
  push_text("");
}


/** module *******************************************/

void init_image_ras(void)
{
   add_function("decode",img_ras_decode,
		"function(string:object)",0);
   add_function("encode",image_ras_encode,
		"function(object,void|mapping(string:mixed):string)",0);
}

void exit_image_ras(void)
{
}
