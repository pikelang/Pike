/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: ilbm.c,v 1.28 2004/03/06 00:06:59 nilsson Exp $
*/

/*
**! module Image
**! submodule ILBM
**!
**!	This submodule keep the ILBM encode/decode capabilities
**!	of the <ref>Image</ref> module.
**!
**! see also: Image, Image.Image, Image.Colortable
*/
#include "global.h"

#include "stralloc.h"
RCSID("$Id: ilbm.c,v 1.28 2004/03/06 00:06:59 nilsson Exp $");
#include "object.h"
#include "interpret.h"
#include "svalue.h"
#include "array.h"
#include "mapping.h"
#include "pike_error.h"
#include "threads.h"
#include "builtin_functions.h"
#include "module_support.h"

#include "image.h"
#include "colortable.h"

#include "encodings.h"


#define sp Pike_sp

extern struct program *image_colortable_program;
extern struct program *image_program;

#define string_BMHD 0
#define string_CMAP 1
#define string_CAMG 2
#define string_BODY 3

static struct svalue string_[4];

struct BMHD {
    unsigned int w, h;
    int x, y;
    unsigned char nPlanes, masking, compression, pad1;
    unsigned int transparentColor;
    unsigned char xAspect, yAspect;
    int pageWidth, pageHeight;
};

#define mskNone                 0
#define mskHasMask              1
#define mskHasTransparentColor  2
#define mskLasso                3

#define cmpNone                 0
#define cmpByteRun1             1

#define CAMG_HAM 0x800
#define CAMG_EHB 0x080


/** decoding *****************************************/

/*
**! method array __decode();
**!     Decodes an ILBM image structure down to chunks and
**!     returns an array containing the ILBM structure;
**!	
**!     <pre>
**!	({int xsize,int ysize,      // 0: size of image drawing area
**!	  string bitmapheader,      // 2: BMHD chunk
**!	  void|string colortable,   // 3: opt. colortable chunk (CMAP)
**!	  void|string colortable,   // 4: opt. colormode chunk (CAMG)
**!	  string body,		    // 5: BODY chunk
**!	  mapping more_chunks})	    // 6: mapping with other chunks
**!	</pre>
**!
**! returns the above array
**!
**! note
**!	May throw errors if the ILBM header is incomplete or illegal.
*/

static void image_ilbm___decode(INT32 args)
{
   unsigned char *s;
   ptrdiff_t len;
   struct pike_string *str;
   struct mapping *m;
   int n;
   extern void parse_iff(char *, unsigned char *, ptrdiff_t,
			 struct mapping *, char *);

   get_all_args("__decode", args, "%S", &str);

   s = (unsigned char *)str->str;
   len = str->len;
   pop_n_elems(args-1);

   for(n=0; n<5; n++)
     push_int(0);
   push_mapping(m = allocate_mapping(4));

   parse_iff("ILBM", s, len, m, "BODY");

   mapping_index_no_free(sp-5, m, &string_[string_BMHD]);
   mapping_index_no_free(sp-4, m, &string_[string_CMAP]);
   mapping_index_no_free(sp-3, m, &string_[string_CAMG]);
   mapping_index_no_free(sp-2, m, &string_[string_BODY]);

   map_delete(m, &string_[string_BMHD]);
   map_delete(m, &string_[string_CMAP]);
   map_delete(m, &string_[string_CAMG]);
   map_delete(m, &string_[string_BODY]);

   if(sp[-5].type != T_STRING)
     Pike_error("Missing BMHD chunk\n");
   if(sp[-2].type != T_STRING)
     Pike_error("Missing BODY chunk\n");

   /* Extract image size from BMHD */
   s = (unsigned char *)STR0(sp[-5].u.string);
   len = sp[-5].u.string->len;

   if(len<20)
     Pike_error("Short BMHD chunk\n");

   free_svalue(sp-7);

   sp[-7].u.integer = (s[0]<<8)|s[1];
   sp[-7].type = T_INT;
   sp[-7].subtype = NUMBER_NUMBER;
   sp[-6].u.integer = (s[2]<<8)|s[3];
   sp[-6].type = T_INT;
   sp[-6].subtype = NUMBER_NUMBER;

   f_aggregate(7);
}


/*
**! method array _decode(string|array data)
**! 	Decode an ILBM image file.
**!
**!     Result is a mapping,
**!	<pre>
**!	([
**!	   "image": object image,
**!
**!        ... more ...
**!     ])
**!	</pre>
**!
**!	<tt>image</tt> is the stored image.
**!
*/

static void parse_bmhd(struct BMHD *bmhd, unsigned char *s, ptrdiff_t len)
{
  if(len<20)
    Pike_error("Short BMHD chunk\n");

  bmhd->w = (s[0]<<8)|s[1];
  bmhd->h = (s[2]<<8)|s[3];
  bmhd->x = (EXTRACT_CHAR(s+4)<<8)|s[5];
  bmhd->y = (EXTRACT_CHAR(s+6)<<8)|s[7];
  bmhd->nPlanes = s[8];
  bmhd->masking = s[9];
  bmhd->compression = s[10];
  bmhd->pad1 = s[11];
  bmhd->transparentColor = (s[12]<<8)|s[13];
  bmhd->xAspect = s[14];
  bmhd->yAspect = s[15];
  bmhd->pageWidth = (EXTRACT_CHAR(s+16)<<8)|s[17];
  bmhd->pageHeight = (EXTRACT_CHAR(s+18)<<8)|s[19];

#ifdef ILBM_DEBUG
  fprintf(stderr, "w = %d, h = %d, x = %d, y = %d, nPlanes = %d,\n"
	  "masking = %d, compression = %d, pad1 = %d, transparentColor = %d,\n"
	  "xAspect = %d, yAspect = %d, pageWidth = %d, pageHeight = %d\n",
	  bmhd->w, bmhd->h, bmhd->x, bmhd->y, bmhd->nPlanes, bmhd->masking,
	  bmhd->compression, bmhd->pad1, bmhd->transparentColor, bmhd->xAspect,
	  bmhd->yAspect, bmhd->pageWidth, bmhd->pageHeight);	  
#endif
}

static ptrdiff_t unpackByteRun1(unsigned char *src, ptrdiff_t srclen,
				unsigned char *dest, int destlen, int depth)
{
  unsigned char d, *src0 = src;

  while(depth>0) {
    int c, left = destlen;
    while(left>0) {
      if(srclen<=0)
	return src+1-src0;
      if((c=EXTRACT_CHAR(src++))>=0) {
	if(srclen<c+2)
	  return src+2+c-src0;
	srclen -= c+2;
	if(1+c>left) {
	  c = left-1;
	} else
	  left -= c+1;
	do { *dest++ = *src++; } while(c--);
      } else if(c!=-128) {
	if(srclen<2)
	  return src+2-src0;
	d = *src++;
	srclen -= 2;
	if(1-c>left) {
	  c = 1-left;
	  left = 0;
	} else
	  left -= 1-c;
	do { *dest++ = d; } while(c++);
      }
    }
    --depth;
  }
  return src-src0;
}

static void planar2chunky(unsigned char *src, int srcmod, int depth,
			  int w, INT32 *dest)
{
  int x, p, bit=0x80;
  /* Really slow... */
  for(x=0; x<w; x++) {
    INT32 pix = 0;
    for(p=0; p<depth; p++)
      if(src[p*srcmod]&bit)
	pix |= 1<<p;
    *dest++ = pix;
    if(!(bit>>=1)) {
      bit = 0x80;
      src++;
    }
  }
}

static void parse_body(struct BMHD *bmhd, unsigned char *body, ptrdiff_t blen,
		       struct image *img, struct image *alpha,
		       struct neo_colortable *ctable, int ham)
{
  unsigned int x, y;
  int rbyt = ((bmhd->w+15)&~15)>>3;
  int eplanes = (bmhd->masking == mskHasMask? bmhd->nPlanes+1:bmhd->nPlanes);
  unsigned char *line=0;
  unsigned INT32 *cptr, *cline = alloca((rbyt<<3)*sizeof(unsigned INT32));
  ptrdiff_t suse=0;
  rgb_group *dest = img->img;
  rgb_group *adest = (alpha==NULL? NULL : alpha->img);

  if(ctable != NULL && ctable->type != NCT_FLAT)
    ctable = NULL;

  switch(bmhd->compression) {
  case cmpNone:
    break;
  case cmpByteRun1:
    line = alloca(rbyt*eplanes);
    break;
  default:
    Pike_error("Unsupported ILBM compression %d\n", bmhd->compression);
  }

  switch(bmhd->masking) {
  case mskNone:
  case mskHasMask:
  case mskHasTransparentColor:
    break;
  case mskLasso:
    Pike_error("Lasso masking not supported\n");
  default:
    Pike_error("Unsupported ILBM masking %d\n", bmhd->masking);
  }

  THREADS_ALLOW();

  for(y=0; y<bmhd->h; y++) {
    switch(bmhd->compression) {
    case cmpNone:
      line = body;
      suse = rbyt*eplanes;
      break;
    case cmpByteRun1:
      suse = unpackByteRun1(body, blen, line, rbyt, eplanes);
      break;
    }
    body += suse;
    if((blen -= suse)<0)
      break;
    planar2chunky(line, rbyt, bmhd->nPlanes, bmhd->w, (INT32 *)(cptr=cline));
    if(alpha != NULL)
      switch(bmhd->masking) {
      case mskNone:
	memset((char *)adest, ~0, bmhd->w*sizeof(*adest));
	adest += bmhd->w;
	break;
      case mskHasMask:
	{
	  int bit=0x80;
	  unsigned char *src = line+bmhd->nPlanes*rbyt;
	  unsigned char ss = *src++;
	  for(x=0; x<bmhd->w; x++) {
	    if(ss&bit) {
	      adest->r = adest->g = adest->b = 0xff;
	    } else {
	      adest->r = adest->g = adest->b = 0;
	    }
	    if(!(bit>>=1)) {
	      bit = 0x80;
	      ss = *src++;
	    }
	  }
	}
	break;
      case mskHasTransparentColor:
	for(x=0; x<bmhd->w; x++) {
	  if(cline[x] == bmhd->transparentColor)
	    adest->r = adest->g = adest->b = 0;
	  else
	    adest->r = adest->g = adest->b = 0xff;
	  adest++;
	}
	break;
      }
    if(ctable != NULL)
      if(ham)
	if(bmhd->nPlanes>6) {
	  /* HAM7/HAM8 */
	  rgb_group hold;
	  int clr;
	  size_t numcolors = ctable->u.flat.numentries;
	  struct nct_flat_entry *entries = ctable->u.flat.entries;
	  hold.r = hold.g = hold.b = 0;
	  for(x=0; x<bmhd->w; x++)
	    switch((*cptr)&0xc0) {
	    case 0x00:
	      if(*cptr<numcolors)
		*dest++ = hold = entries[*cptr++].color;
	      else {
		*dest++ = hold;
		cptr++;
	      }
	      break;
	    case 0x80:
	      clr = (*cptr++)&0x3f;
	      hold.r = (clr<<2)|(clr>>4);
	      *dest++ = hold;
	      break;
	    case 0xc0:
	      clr = (*cptr++)&0x3f;
	      hold.g = (clr<<2)|(clr>>4);
	      *dest++ = hold;
	      break;
	    case 0x40:
	      clr = (*cptr++)&0x3f;
	      hold.b = (clr<<2)|(clr>>4);
	      *dest++ = hold;
	      break;
	    }
	} else {
	  /* HAM5/HAM6 */	  
	  rgb_group hold;
	  int clr;
	  size_t numcolors = ctable->u.flat.numentries;
	  struct nct_flat_entry *entries = ctable->u.flat.entries;
	  hold.r = hold.g = hold.b = 0;
	  for(x=0; x<bmhd->w; x++)
	    switch((*cptr)&0x30) {
	    case 0x00:
	      if(*cptr<numcolors)
		*dest++ = hold = entries[*cptr++].color;
	      else {
		*dest++ = hold;
		cptr++;
	      }
	      break;
	    case 0x20:
	      clr = (*cptr++)&0xf;
	      hold.r = (clr<<4)|clr;
	      *dest++ = hold;
	      break;
	    case 0x30:
	      clr = (*cptr++)&0xf;
	      hold.g = (clr<<4)|clr;
	      *dest++ = hold;
	      break;
	    case 0x10:
	      clr = (*cptr++)&0xf;
	      hold.b = (clr<<4)|clr;
	      *dest++ = hold;
	      break;
	    }
	}
      else {
	/* normal palette */
	size_t numcolors = ctable->u.flat.numentries;
	struct nct_flat_entry *entries = ctable->u.flat.entries;
	for(x=0; x<bmhd->w; x++)
	  if(*cptr<numcolors)
	    *dest++ = entries[*cptr++].color;
	  else {
	    dest++;
	    cptr++;
	  }
      }
    else
      for(x=0; x<bmhd->w; x++) {
	/* ILBM-24 */
	dest->b = ((*cptr)&0xff0000)>>16;
	dest->g = ((*cptr)&0x00ff00)>>8;
	dest->r = ((*cptr)&0x0000ff);
	cptr++;
	dest++;
      }
  }

  THREADS_DISALLOW();

  if(blen<0)
    Pike_error("truncated or corrupt BODY chunk\n");
}

static void image_ilbm__decode(INT32 args)
{
  struct array *arr;
  struct object *o;
  struct image *img, *alpha = NULL;
  struct BMHD bmhd;
  struct neo_colortable *ctable = NULL;
  int n = 0;
  INT32 camg = 0;

  if(args>0 && sp[-args].type == T_STRING) {
    image_ilbm___decode(args);
    args = 1;
  }
  
  get_all_args("_decode", args, "%a", &arr);

  if(args>1)
    pop_n_elems(args-1);

  if(arr->size < 6 ||
     ITEM(arr)[2].type != T_STRING || ITEM(arr)[2].u.string->size_shift != 0 ||
     ITEM(arr)[5].type != T_STRING || ITEM(arr)[5].u.string->size_shift != 0)
    Pike_error("Image.ILBM._decode: illegal argument\n");

  parse_bmhd(&bmhd, STR0(ITEM(arr)[2].u.string), ITEM(arr)[2].u.string->len);

  push_text("image");
  push_int(bmhd.w);
  push_int(bmhd.h);
  o=clone_object(image_program,2);
  img=(struct image*)get_storage(o,image_program);
  push_object(o);
  n++;

  if(bmhd.masking != mskNone) {
    push_text("alpha");
    push_int(bmhd.w);
    push_int(bmhd.h);
    o=clone_object(image_program,2);
    alpha=(struct image*)get_storage(o,image_program);
    push_object(o);
    n++;
  }

  if(ITEM(arr)[4].type == T_STRING && ITEM(arr)[4].u.string->size_shift == 0 &&
     ITEM(arr)[4].u.string->len>=4) {
    unsigned char *camgp = STR0(ITEM(arr)[4].u.string);
    camg = (camgp[0]<<24)|(camgp[1]<<16)|(camgp[2]<<8)|camgp[3];
  }

  if((camg & CAMG_HAM)) {
    push_text("ham");
    push_int(1);
    n++;
  }
  if((camg & CAMG_EHB)) {
    push_text("ehb");
    push_int(1);
    n++;
  }

  if(ITEM(arr)[3].type == T_STRING && ITEM(arr)[3].u.string->size_shift == 0) {
    unsigned char *pal = STR0(ITEM(arr)[3].u.string);
    INT32 col, mcol = 1<<bmhd.nPlanes;
    ptrdiff_t ncol = ITEM(arr)[3].u.string->len/3;
    if((camg & CAMG_HAM))
      mcol = (bmhd.nPlanes>6? 64:16);
    else if((camg & CAMG_EHB))
      mcol >>= 1;
    if(ncol>mcol)
      ncol = mcol;
    push_text("palette");
    for(col=0; col<ncol; col++) {
      push_int(*pal++);
      push_int(*pal++);
      push_int(*pal++);
      f_aggregate(3);
    }
    if((camg & CAMG_EHB) && !(camg & CAMG_HAM)) {
      for(col=0; col<ncol; col++) {
	struct svalue *old = ITEM(sp[-ncol].u.array);
	push_int(old[0].u.integer>>1);
	push_int(old[1].u.integer>>1);
	push_int(old[2].u.integer>>1);
	f_aggregate(3);
      }
      ncol <<= 1;
    }
    f_aggregate(DO_NOT_WARN((INT32)ncol));
    push_object(clone_object(image_colortable_program,1));
    ctable=(struct neo_colortable*)get_storage(sp[-1].u.object,
					       image_colortable_program);
    n++;
  }

  parse_body(&bmhd, STR0(ITEM(arr)[5].u.string), ITEM(arr)[5].u.string->len,
	     img, alpha, ctable, !!(camg & CAMG_HAM));

  f_aggregate_mapping(2*n);
  stack_swap();
  pop_stack();
}


/*
**! method object decode(string data)
**! method object decode(array _decoded)
**! method object decode(array __decoded)
**!	Decodes ILBM data and creates an image object.
**! 	
**! see also: encode
**!
**! note
**!	This function may throw errors upon illegal ILBM data.
**!	This function uses <ref>__decode</ref> and
**!	<ref>_decode</ref> internally.
**!
**! returns the decoded image as an image object
*/

void img_ilbm_decode(INT32 args)
{
   struct svalue *sv;

   if (!args)
      Pike_error("Image.ILBM.decode: too few argument\n");

   if (sp[-args].type != T_MAPPING) {
     image_ilbm__decode(args);
     args = 1;
   }
     
   if (sp[-args].type != T_MAPPING)
     Pike_error("Image.ILBM.decode: illegal argument\n");

   if(args>1)
     pop_n_elems(args-1);

   sv = simple_mapping_string_lookup(sp[-args].u.mapping, "image");

   if(sv == NULL || sv->type != T_OBJECT)
     Pike_error("Image.ILBM.decode: illegal argument\n");

   ref_push_object(sv->u.object);
   stack_swap();
   pop_stack();
}



/** encoding *****************************************/

/*
**! method string encode(object image)
**! method string encode(object image, mapping options)
**! 	Encodes an ILBM image. 
**!
**!     The <tt>options</tt> argument may be a mapping
**!	containing zero or more encoding options:
**!
**!	<pre>
**!	normal options:
**!	    "alpha":image object
**!		Use this image as mask
**!		(Note: ILBM mask is boolean.
**!		 The values are calculated by (r+2g+b)/4>=128.)
**!
**!	    "palette":colortable object
**!		Use this as palette for pseudocolor encoding
**!
**!	</pre>
*/

static struct pike_string *make_bmhd(struct BMHD *bmhd)
{
  unsigned char bdat[20];

  bdat[0] = (bmhd->w>>8); bdat[1] = bmhd->w&0xff;
  bdat[2] = (bmhd->h>>8); bdat[3] = bmhd->h&0xff;
  bdat[4] = (bmhd->x>>8); bdat[5] = bmhd->x&0xff;
  bdat[6] = (bmhd->y>>8); bdat[7] = bmhd->y&0xff;
  bdat[8] = bmhd->nPlanes;
  bdat[9] = bmhd->masking;
  bdat[10] = bmhd->compression;
  bdat[11] = bmhd->pad1;
  bdat[12] = (bmhd->transparentColor>>8); bdat[13]=bmhd->transparentColor&0xff;
  bdat[14] = bmhd->xAspect;
  bdat[15] = bmhd->yAspect;
  bdat[16] = (bmhd->pageWidth>>8); bdat[17] = bmhd->pageWidth&0xff;
  bdat[18] = (bmhd->pageHeight>>8); bdat[19] = bmhd->pageHeight&0xff;

  return make_shared_binary_string((char *)bdat, 20);
}

static void packByteRun1(unsigned char *src, int srclen, int depth,
			 struct string_builder *dest)
{
  while(depth>0) {
    int c, left = srclen;
    while(left>0) {
      if(left<2 || src[0] != src[1]) {
	for(c=1; c<128 && c<left; c++)
	  if(c+2<left && src[c] == src[c+1] && src[c] == src[c+2])
	    break;
	string_builder_putchar(dest, c-1);
	string_builder_binary_strcat(dest, (char *)src, c);
      } else {
	for(c=2; c<128 && c<left && src[c]==src[0]; c++);
	string_builder_putchar(dest, (1-c)&0xff);
	string_builder_putchar(dest, src[0]);
      }
      src += c;
      left -= c;
    }
    --depth;
  }
}

static void chunky2planar(INT32 *src, int w,
			  unsigned char *dest, int destmod, int depth)
{
  int x, p, bit=0x80;
  for(x=0; x<w; x+=8) {
    INT32 p0, p1, p2, p3, p4, p5, p6, p7;
    switch(w-x) {
    default:
      p0 = *src++; p1 = *src++; p2 = *src++; p3 = *src++;
      p4 = *src++; p5 = *src++; p6 = *src++; p7 = *src++; break;
    case 7:
      p0 = *src++; p1 = *src++; p2 = *src++; p3 = *src++;
      p4 = *src++; p5 = *src++; p6 = *src++; p7 = 0; break;
    case 6:
      p0 = *src++; p1 = *src++; p2 = *src++; p3 = *src++;
      p4 = *src++; p5 = *src++; p6 = p7 = 0; break;
    case 5:
      p0 = *src++; p1 = *src++; p2 = *src++; p3 = *src++;
      p4 = *src++; p5 = p6 = p7 = 0; break;
    case 4:
      p0 = *src++; p1 = *src++; p2 = *src++; p3 = *src++;
      p4 = p5 = p6 = p7 = 0; break;
    case 3:
      p0 = *src++; p1 = *src++; p2 = *src++;
      p3 = p4 = p5 = p6 = p7 = 0; break;
    case 2:
      p0 = *src++; p1 = *src++; p2 = p3 = p4 = p5 = p6 = p7 = 0; break;
    case 1:
      p0 = *src; p1 = p2 = p3 = p4 = p5 = p6 = p7 = 0; break;
    }
    for(p=0; p<depth; p++) {
      dest[p*destmod] =
	((p0&1)<<7)|((p1&1)<<6)|((p2&1)<<5)|((p3&1)<<4)|
	((p4&1)<<3)|((p5&1)<<2)|((p6&1)<<1)|(p7&1);
      p0>>=1; p1>>=1; p2>>=1; p3>>=1;
      p4>>=1; p5>>=1; p6>>=1; p7>>=1;
    }
    dest++;
  }
}

static struct pike_string *make_body(struct BMHD *bmhd,
				     struct image *img, struct image *alpha,
				     struct neo_colortable *ctable)
{
  unsigned int x, y;
  int rbyt = ((bmhd->w+15)&~15)>>3;
  int eplanes = (bmhd->masking == mskHasMask? bmhd->nPlanes+1:bmhd->nPlanes);
  unsigned char *line = alloca(rbyt*eplanes);
  INT32 *cptr, *cline = alloca((rbyt<<3)*sizeof(INT32));
  rgb_group *src = img->img;
  struct string_builder bldr;
  struct nct_dither dith;
  void (*ctfunc)(rgb_group *, unsigned INT32 *, int,
		 struct neo_colortable *, struct nct_dither *, int) = NULL;

  if(ctable != NULL) {
    image_colortable_initiate_dither(ctable, &dith, bmhd->w);
    ctfunc = image_colortable_index_32bit_function(ctable);
  }

  memset(line, 0, rbyt*eplanes);
  init_string_builder(&bldr, 0);
  for(y=0; y<bmhd->h; y++) {
    if(ctfunc != NULL) {
      ctfunc(src, (unsigned INT32 *)cline, bmhd->w, ctable, &dith, bmhd->w);
      src += bmhd->w;
    } else {
      cptr = cline;
      for(x=0; x<bmhd->w; x++) {
	/* ILBM-24 */
	*cptr++ = (src->b<<16)|(src->g<<8)|src->r;
	src++;
      }
    }
    chunky2planar(cline, bmhd->w, line, rbyt, bmhd->nPlanes);
    if(bmhd->compression == cmpByteRun1)
      packByteRun1(line, rbyt, eplanes, &bldr);
    else
      string_builder_binary_strcat(&bldr, (char *)line, rbyt*eplanes);
  }
  if(ctable != NULL)
    image_colortable_free_dither(&dith);
  return finish_string_builder(&bldr);
}

static void image_ilbm_encode(INT32 args)
{
  struct object *imgo;
  struct mapping *optm = NULL;
  struct image *alpha = NULL, *img;
  struct neo_colortable *ct = NULL;
  struct pike_string *res;
  struct BMHD bmhd;
  int bpp, n = 0;

  extern struct pike_string *make_iff(char *id, struct array *chunks);

  get_all_args("encode", args, (args>1 && !UNSAFE_IS_ZERO(&sp[1-args])? "%o%m":"%o"),
	       &imgo, &optm);

  if((img=(struct image*)get_storage(imgo, image_program))==NULL)
     Pike_error("Image.ILBM.encode: illegal argument 1\n");

  if(optm != NULL) {
    struct svalue *s;
    if((s = simple_mapping_string_lookup(optm, "alpha"))!=NULL && !UNSAFE_IS_ZERO(s))
      if(s->type != T_OBJECT ||
	 (alpha=(struct image*)get_storage(s->u.object, image_program))==NULL)
	Pike_error("Image.ILBM.encode: option (arg 2) \"alpha\" has illegal type\n");
    if((s=simple_mapping_string_lookup(optm, "palette"))!=NULL && !UNSAFE_IS_ZERO(s))
      if(s->type != T_OBJECT ||
	 (ct=(struct neo_colortable*)
	  get_storage(s->u.object, image_colortable_program))==NULL)
	Pike_error("Image.ILBM.encode: option (arg 2) \"palette\" has illegal type\n");
  }

  if (!img->img)
    Pike_error("Image.ILBM.encode: no image\n");
  if (alpha && !alpha->img)
    Pike_error("Image.ILBM.encode: no alpha image\n");

  if(ct && ct->type == NCT_NONE)
    ct = NULL;

  if(ct) {
    ptrdiff_t sz = image_colortable_size(ct);
    for(bpp=1; (1<<bpp)<sz; bpp++);
  } else
    bpp = 24;

  bmhd.w = img->xsize;
  bmhd.h = img->ysize;
  bmhd.x = bmhd.y = 0;
  bmhd.nPlanes = bpp;
  bmhd.masking = mskNone;
  bmhd.compression = (img->xsize>32? cmpByteRun1:cmpNone);
  bmhd.pad1 = 0;
  bmhd.transparentColor = 0;
  bmhd.xAspect = bmhd.yAspect = 1;
  bmhd.pageWidth = img->xsize;
  bmhd.pageHeight = img->ysize;

  push_svalue(&string_[string_BMHD]);
  push_string(make_bmhd(&bmhd));
  f_aggregate(2);
  n++;

  if(ct) {
    struct pike_string *str = begin_shared_string(image_colortable_size(ct)*3);
    push_svalue(&string_[string_CMAP]);
    image_colortable_write_rgb(ct, STR0(str));
    push_string(end_shared_string(str));
    f_aggregate(2);
    n++;
  }

  push_svalue(&string_[string_BODY]);
  push_string(make_body(&bmhd, img, alpha, ct));
  f_aggregate(2);
  n++;

  f_aggregate(n);
  res = make_iff("ILBM", sp[-1].u.array);
  pop_n_elems(args+1);
  push_string(res);
}


/** module *******************************************/

struct program *image_encoding_ilbm_program=NULL;

void init_image_ilbm(void)
{
   static char *str[] = { "BMHD", "CMAP", "CAMG", "BODY" };
   int n;

   for(n=0; n<4; n++) {
     push_string(make_shared_binary_string(str[n], 4));
     assign_svalue_no_free(&string_[n], sp-1);
     pop_stack();
   }

   
   add_function("__decode",image_ilbm___decode,
		"function(string:array)",0);
   add_function("_decode",image_ilbm__decode,
		"function(string|array:mapping)",0);
   add_function("decode",img_ilbm_decode,
		"function(string|array:object)",0);
   add_function("encode",image_ilbm_encode,
		"function(object,void|mapping(string:mixed):string)",0);
}

void exit_image_ilbm(void)
{
  int n;
  for(n=0; n<4; n++)
    free_svalue(&string_[n]);
}
