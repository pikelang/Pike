/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "program.h"
#include "module_support.h"
#include "interpret.h"
#include "builtin_functions.h"
#include "mapping.h"
#include "operators.h"

#include "image_machine.h"
#include "atari.h"
#include "colortable.h"


extern struct program *image_program;
extern struct program *image_colortable_program;

/* Some format references:
 *
 *   http://oreilly.com/www/centers/gff/formats/atari/index.htm
 *   http://wiki.multimedia.cx/index.php?title=Neochrome
 *   http://www.mediatel.lu/workshop/graphic/2D_fileformat/h_atari.html
 */

/*! @module Image
 */

/*! @module NEO
 *! Decodes images from the Atari image editor Neochrome.
 *!
 *! These are images in a few fixed resolutions:
 *! @dl
 *!   @item Low resolution
 *!     16 color images with the fixed size of @expr{320 * 200@}.
 *!   @item Medium resolution
 *!     4 color images with the fixed size of @expr{640 * 200@}.
 *!   @item High resolution
 *!     2 color images with the fixed size of @expr{640 * 400@}.
 *! @enddl
 *!
 *! @note
 *!   There is apparently a second NEOchrome format that allows
 *!   for images that are twice the size in both dimensions.
 *!   That format is currently NOT supported.
 */

#define NEOMAGIC	0
#define NEOSIZE		32128
#define NEOWIDTH	320
#define NEOHEIGHT	200

/* Cf https://www.atari-wiki.com/index.php?title=NEOchrome_file_format */
#define NEOVCMAGIC	0xbabe
#define NEOVCSIZE	128128
#define NEOVCWIDTH	640
#define NEOVCHEIGHT	400

/*! @decl constant SIZE = 32128
 *!
 *! Size of Neochrome files.
 */

/*! @decl constant HEIGHT = 200
 *!
 *! Height of Neochrome images.
 */

/*! @decl constant WIDTH = 320
 *!
 *! Width of Neochrome images.
 */

/*! @decl mapping _decode(string data)
 *! Low level decoding of the NEO file contents in @[data].
 *! @returns
 *!   @mapping
 *!     @member Image.Image "image"
 *!       The decoded bitmap
 *!     @member array(Image.Image) "images"
 *!       Color cycled images.
 *!     @member string "filename"
 *!       The filename stored into the file.
 *!     @member int(0..15) "right_limit"
 *!     @member int(0..15) "left_limit"
 *!       The palette color range to be color cycled.
 *!     @member int(0..255) "speed"
 *!       The animation speed, expressed as the number of VBLs
 *!       per animation frame.
 *!     @member string "direction"
 *!       Color cycling direction. Can be either @expr{"left"@}
 *!       or @expr{"right"@}.
 *!     @member array(array(int(0..255))) "palette"
 *!       The palette to be used for color cycling.
 *!   @endmapping
 */
void image_neo_f__decode(INT32 args)
{
  unsigned int i, res, size = 0;
  struct atari_palette *pal=0;
  struct object *img;

  struct pike_string *s, *fn;
  unsigned char *q;
  ONERROR err;

  get_all_args( NULL, args, "%n", &s );
  if(s->len!=32128)
    Pike_error("This is not a NEO file (wrong file size).\n");

  q = (unsigned char *)s->str;
  res = q[3];

  if(q[2]!=0 || (res!=0 && res!=1 && res!=2))
    Pike_error("This is not a NEO file (invalid resolution).\n");

  /* Checks done... */
  add_ref(s);
  pop_n_elems(args);

  if(res==0)
    pal = decode_atari_palette(q+4, 16);
  else if(res==1)
    pal = decode_atari_palette(q+4, 4);
  else if(res==2)
    pal = decode_atari_palette(q+4, 2);
  SET_ONERROR(err, free_atari_palette, pal);

  push_static_text("palette");
  for( i=0; i<pal->size; i++ ) {
    push_int(pal->colors[i].r);
    push_int(pal->colors[i].g);
    push_int(pal->colors[i].b);
    f_aggregate(3);
  }
  f_aggregate(pal->size);
  size += 2;

  img = decode_atari_screendump(q+128, res, pal);

  push_static_text("image");
  push_object(img);
  size += 2;

  if(q[48]&128) {
    int rl, ll, i;
    rl = q[49]&0xf;
    ll = (q[49]&0xf0)>>4;

    push_static_text("right_limit");
    push_int( rl );
    push_static_text("left_limit");
    push_int( ll );
    push_static_text("speed");
    push_int( q[51] );
    push_static_text("direction");
    if( q[50]&128 )
      push_static_text("right");
    else
      push_static_text("left");

    push_static_text("images");
    for(i=0; i<rl-ll+1; i++) {
      if( q[50]&128 )
	rotate_atari_palette(pal, ll, rl);
      else
	rotate_atari_palette(pal, rl, ll);
      img = decode_atari_screendump(q+128, res, pal);
      push_object(img);
    }
    f_aggregate(rl-ll+1);

    size += 10;
  }

  UNSET_ONERROR(err);
  free_atari_palette(pal);

  fn = make_shared_binary_string((const char *)q+36, 12);

  push_static_text("filename");
  push_string(fn);
  size += 2;

  free_string(s);
  f_aggregate_mapping(size);
}

/*! @decl Image.Image decode(string data)
 *! Decodes the image @[data] into an @[Image.Image] object.
 */
void image_neo_f_decode(INT32 args)
{
  image_neo_f__decode(args);
  push_static_text("image");
  f_index(2);
}

#define UINT64_CONSTANT(HI, LOW)	((((unsigned INT64)(HI))<<32)|(LOW))

/*! @decl string(8bit) encode(Image.Image img, @
 *!              Image.Colortable|int(0..15)|void colortable)
 */
void image_neo_f_encode(INT32 args)
{
  struct image *img=NULL;
  struct object *imgobj=NULL;

  struct object *nctobj=NULL;
  struct neo_colortable *nct=NULL;

  int maxcolors = 16;
  int numcolors = 0;
  int resolution = 0;

  struct pike_string *res = NULL;
  unsigned char *chunky = NULL;
  unsigned char *src, *end;

  if (args<1)
    Pike_error("Image.NEO.encode(): Too few arguments\n");

  if (TYPEOF(Pike_sp[-args]) != T_OBJECT ||
      !(img=get_storage(imgobj = Pike_sp[-args].u.object,
                        image_program)))
    Pike_error("Image.NEO.encode(): Illegal argument 1 (expected image object)\n");

  if ((img->xsize == 320) && (img->ysize == 200)) {
  } else if ((img->xsize == 640) && (img->ysize == 200)) {
    maxcolors = 4;
    resolution = 1;
  } else if ((img->xsize == 640) && (img->ysize == 400)) {
    maxcolors = 2;
    resolution = 2;
  } else {
    Pike_error("Image.NEO.encode(): Invalid image size (must be one of 320x200, 640x200 or 640x400).\n");
  }

  if (args>=2) {
    if (TYPEOF(Pike_sp[1-args]) == T_INT)
    {
      if (args!=4)
      {
        numcolors = Pike_sp[1-args].u.integer;
        if (numcolors < 2) numcolors = 2;
      }
    }
    else if (TYPEOF(Pike_sp[1-args]) != T_OBJECT)
      Pike_error("Image.NEO.encode(): Illegal argument 2 (expected image or colortable object or int)\n");
    else {
      nctobj = Pike_sp[1-args].u.object,image_colortable_program;
      add_ref(nctobj);
    }
  }

  if (!nctobj) {
    ref_push_object(imgobj);

    if (!numcolors || (numcolors > 16)) numcolors = maxcolors;

    push_int(numcolors);
    nctobj = clone_object(image_colortable_program, 2);
  }

  push_object(nctobj);	/* Let the stack hold its reference. */

  nct = get_storage(nctobj, image_colortable_program);
  if (!nct) {
    Pike_error("Image.NEO.encode(): Internal error; colortable isn't colortable\n");
  }

  numcolors = image_colortable_size(nct);
  if (!numcolors) {
    Pike_error("Image.NEO.encode(): no colors in colortable\n");
  } else if (numcolors > maxcolors) {
    Pike_error("Image.NEO.encode(): too many colors in given colortable: "
               "%ld (%d is max)\n",
               (long)numcolors, maxcolors);
  }

  res = begin_shared_string(NEOSIZE);

  chunky = malloc(NEOWIDTH * NEOHEIGHT * 4);
  if (!chunky) {
    free_string(res);
    Pike_error("Image.NEO.encode(): Out of memory.\n");
  }

  memset(STR0(res), 0, NEOSIZE);

  STR0(res)[0] = NEOMAGIC & 0xff;
  STR0(res)[1] = (NEOMAGIC >> 8) & 0xff;
  STR0(res)[2] = resolution;
  STR0(res)[3] = 0;

  /* Palette */
  /* NB: We have quite a bit of space in the image buffer,
   *     so use it to contain the one byte/channel representation.
   */
  image_colortable_write_rgb(nct, STR0(res) + 128);
  src = STR0(res) + 128;
  end = STR0(res) + 4;
  while (numcolors--) {
    /* NB: We encode 5 bits/channel as per the Spectrum 512 enhanced
     *     palette format. Note that the fifth bit is ignored by
     *     decode().
     *
     * Cf http://justsolve.archiveteam.org/wiki/Atari_ST_color_palette
     */
    end[0] =
      ((((src[0] & 0xf0) * 0x11) >> 5) & 0x0f) |
        ((src[0] & 0x08)<<4) | ((src[1] & 0x08)<<3) | ((src[2] & 0x08)<<2);
    end[1] =
      ((((src[1] & 0xf0) * 0x11) >> 1) & 0xf0) |
      ((((src[2] & 0xf0) * 0x11) >> 5) & 0x0f);
    src += 3;
    end += 2;
  }

  memcpy(res->str + 36, "        .   ", 12);	/* Filename */

  STR0(res)[58] = img->xsize & 0xff;		/* Image width */
  STR0(res)[59] = (img->xsize >> 8) & 0xff;
  STR0(res)[60] = img->ysize & 0xff;		/* Image height */
  STR0(res)[61] = (img->ysize >> 8) & 0xff;

  image_colortable_index_8bit_image(nct, img->img, chunky,
                                    NEOWIDTH * NEOHEIGHT, NEOWIDTH);

  /* Convert chunky to planar. */
  src = chunky;
  end = (chunky + NEOWIDTH * NEOHEIGHT);
  switch(resolution) {
  case 0:	/* Low-res, 4bits/pixel */
    {
      /* We convert the image in blocks of 4 * int16
       * (ie 4 bitplanes in parallel.
       *
       * The multiplication in the first subloop corresponds to the operation
       * (c << 52)|(c << 37)|(c << 22)|(c << 7) for little-endian, and
       * (c << 63)|(c << 46)|(c << 29)|(c << 12) for big-endian.
       * After the masking this means that it has set the most
       * significant bit in the first byte of each of the int16s
       * according to the bits of the color nybble. The bits are are then
       * shifted to their desired positions in the byte and accumulated in a.
       * The second subloop handles the second byte of each of the int16s
       * analogously.
       */
      unsigned INT64 *dst = (unsigned INT64 *)(STR0(res) + 128);
      while (src < end) {
        int i;
        unsigned INT64 a = 0;
        if ((unsigned char *)dst >= (STR0(res) + NEOSIZE)) {
          Pike_fatal("Pixel %ld destination out of buffer: %ld\n",
                     (long)(src - chunky),
                     (long)(((unsigned char *)dst) - STR0(res)));
        }
        for (i = 0; i < 8; i++) {
#if PIKE_BYTEORDER == 1234
          /* Little-endian */
          unsigned INT64 v =
            (((unsigned INT64)(*(src++) & 0x0f)) *
             UINT64_CONSTANT(0x00100020, 0x00400080)) &
            UINT64_CONSTANT(0x00800080, 0x00800080);
#else
          /* Big-endian */
          unsigned INT64 v =
            (((unsigned INT64)(*(src++) & 0x0f)) *
             UINT64_CONSTANT(0x80004000UL, 0x20001000)) &
            UINT64_CONSTANT(0x80008000UL, 0x80008000UL);
#endif
          a |= v>>i;
        }
        for (i = 0; i < 8; i++) {
#if PIKE_BYTEORDER == 1234
          /* Little-endian */
          unsigned INT64 v =
            (((unsigned INT64)(*(src++) & 0x0f)) *
             UINT64_CONSTANT(0x10002000, 0x40008000)) &
            UINT64_CONSTANT(0x80008000UL, 0x80008000UL);
#else
          /* Big-endian */
          unsigned INT64 v =
            (((unsigned INT64)(*(src++) & 0x0f)) *
             UINT64_CONSTANT(0x00800040, 0x00200010)) &
            UINT64_CONSTANT(0x00800080, 0x00800080);
#endif
          a |= v>>i;
        }
        *(dst++) = a;
      }
    }
    break;
  case 1:	/* Med-res, 2bits/pixel */
    {
      unsigned INT32 *dst = (unsigned INT32 *)(STR0(res) + 128);
      while (src < end) {
        int i;
        unsigned INT32 a = 0;
        for (i = 0; i < 8; i++) {
#if PIKE_BYTEORDER == 1234
          /* Little-endian */
          unsigned INT32 v =
            (((unsigned INT32)(*(src++) & 0x03)) *
             0x00400080) & 0x00800080;
#else
          /* Big-endian */
          unsigned INT32 v =
            (((unsigned INT32)(*(src++) & 0x03)) *
             0x40008000) & 0x80008000UL;
#endif
          a |= v>>i;
        }
        for (i = 0; i < 8; i++) {
#if PIKE_BYTEORDER == 1234
          /* Little-endian */
          unsigned INT32 v =
            (((unsigned INT32)(*(src++) & 0x03)) *
             0x40008000) & 0x80008000UL;
#else
          /* Big-endian */
          unsigned INT32 v =
            (((unsigned INT32)(*(src++) & 0x03)) *
             0x00400080) & 0x00800080;
#endif
          a |= v>>i;
        }
        *(dst++) = a;
      }
    }
    break;
  case 2:	/* Hi-res, 1bit/pixel */
    {
      unsigned char *dst = STR0(res) + 128;
      while (src < end) {
        int i;
        unsigned char a = 0;
        for (i = 0; i < 8; i++) {
          unsigned char v = (*(src++) * 0x80) & 1;
          a |= v>>i;
        }
        *(dst++) = a;
      }
    }
    break;
  }

  free(chunky);

  push_string(end_shared_string(res));
}

/*! @endmodule
 */

/*! @endmodule
 */

void init_image_neo(void)
{
  ADD_FUNCTION("decode",  image_neo_f_decode,  tFunc(tStr,tObj),  0);
  ADD_FUNCTION("_decode", image_neo_f__decode,
	       tFunc(tStr,tMap(tStr,tMix)), 0);

  ADD_FUNCTION("encode",  image_neo_f_encode,
               tFunc(tObj tOr3(tObj, tInt8bit, tVoid), tStr8), 0);

  ADD_INT_CONSTANT("SIZE", NEOSIZE, 0);
  ADD_INT_CONSTANT("WIDTH", NEOWIDTH, 0);
  ADD_INT_CONSTANT("HEIGHT", NEOHEIGHT, 0);
}

void exit_image_neo(void)
{
}
