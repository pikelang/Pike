/* $Id: ilbm.c,v 1.5 1999/04/07 17:47:12 marcus Exp $ */

/*
**! module Image
**! note
**!	$Id: ilbm.c,v 1.5 1999/04/07 17:47:12 marcus Exp $
**! submodule ILBM
**!
**!	This submodule keep the ILBM encode/decode capabilities
**!	of the <ref>Image</ref> module.
**!
**! see also: Image, Image.image, Image.colortable
*/
#include "global.h"

#include "stralloc.h"
RCSID("$Id: ilbm.c,v 1.5 1999/04/07 17:47:12 marcus Exp $");
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
   INT32 len;
   struct pike_string *str;
   struct mapping *m;
   int n;
   extern void parse_iff(char *, unsigned char *, INT32,
			 struct mapping *, char *);

   get_all_args("__decode", args, "%S", &str);

   s=(unsigned char *)str->str;
   len=str->len;
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
     error("Missing BMHD chunk\n");
   if(sp[-2].type != T_STRING)
     error("Missing BODY chunk\n");

   /* Extract image size from BMHD */
   s=(unsigned char *)STR0(sp[-5].u.string);
   len=sp[-5].u.string->len;

   if(len<20)
     error("Short BMHD chunk\n");

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

static void parse_bmhd(struct BMHD *bmhd, unsigned char *s, INT32 len)
{
  if(len<20)
    error("Short BMHD chunk\n");

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

static INT32 unpackByteRun1(unsigned char *src, INT32 srclen,
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

static void parse_body(struct BMHD *bmhd, unsigned char *body, INT32 blen,
		       struct image *img, struct image *alpha,
		       struct neo_colortable *ctable, int ham)
{
  unsigned int x, y;
  int rbyt = ((bmhd->w+15)&~15)>>3;
  int eplanes = (bmhd->masking == mskHasMask? bmhd->nPlanes+1:bmhd->nPlanes);
  unsigned char *line;
  INT32 *cptr, *cline = alloca((rbyt<<3)*sizeof(INT32));
  INT32 suse;
  rgb_group *dest = img->img;

  if(ctable != NULL && ctable->type != NCT_FLAT)
    ctable = NULL;

  switch(bmhd->compression) {
  case cmpNone:
    break;
  case cmpByteRun1:
    line = alloca(rbyt*eplanes);
    break;
  default:
    error("Unsupported ILBM compression %d\n", bmhd->compression);
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
    planar2chunky(line, rbyt, bmhd->nPlanes, bmhd->w, cptr=cline);
    if(ctable != NULL)
      if(ham)
	if(bmhd->nPlanes>6) {
	  /* HAM7/HAM8 */
	  rgb_group hold;
	  int clr, numcolors = ctable->u.flat.numentries;
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
	  int clr, numcolors = ctable->u.flat.numentries;
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
	int numcolors = ctable->u.flat.numentries;
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
    error("truncated or corrupt BODY chunk\n");
}

static void image_ilbm__decode(INT32 args)
{
  struct array *arr;
  struct object *o;
  struct image *img;
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
    error("Image.ILBM._decode: illegal argument\n");

  parse_bmhd(&bmhd, STR0(ITEM(arr)[2].u.string), ITEM(arr)[2].u.string->len);

  push_text("image");
  push_int(bmhd.w);
  push_int(bmhd.h);
  o=clone_object(image_program,2);
  img=(struct image*)get_storage(o,image_program);
  push_object(o);
  n++;

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
    INT32 col, ncol = ITEM(arr)[3].u.string->len/3, mcol = 1<<bmhd.nPlanes;
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
    f_aggregate(ncol);
    push_object(clone_object(image_colortable_program,1));
    ctable=(struct neo_colortable*)get_storage(sp[-1].u.object,
					       image_colortable_program);
    n++;
  }

  parse_body(&bmhd, STR0(ITEM(arr)[5].u.string), ITEM(arr)[5].u.string->len,
	     img, NULL, ctable, !!(camg & CAMG_HAM));

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
      error("Image.ILBM.decode: too few argument\n");

   if (sp[-args].type != T_MAPPING) {
     image_ilbm__decode(args);
     args = 1;
   }
     
   if (sp[-args].type != T_MAPPING)
     error("Image.ILBM.decode: illegal argument\n");

   if(args>1)
     pop_n_elems(args-1);

   sv = simple_mapping_string_lookup(sp[-args].u.mapping, "image");

   if(sv == NULL || sv->type != T_OBJECT)
     error("Image.ILBM.decode: illegal argument\n");

   ref_push_object(sv->u.object);
   stack_swap();
   pop_stack();
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

   start_new_program();
   
   add_function("__decode",image_ilbm___decode,
		"function(string:array)",0);
   add_function("_decode",image_ilbm__decode,
		"function(string|array:mapping)",0);
   add_function("decode",img_ilbm_decode,
		"function(string|array:object)",0);

   image_encoding_ilbm_program=end_program();
   push_object(clone_object(image_encoding_ilbm_program,0));
   {
     struct pike_string *s=make_shared_string("ILBM");
     add_constant(s,sp-1,0);
     free_string(s);
   }
   pop_stack();
}

void exit_image_ilbm(void)
{
  int n;
  if(image_encoding_ilbm_program)
  {
    free_program(image_encoding_ilbm_program);
    image_encoding_ilbm_program=NULL;
  }
  for(n=0; n<4; n++)
    free_svalue(&string_[n]);
}
