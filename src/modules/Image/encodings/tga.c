/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: tga.c,v 1.38 2004/10/07 22:49:57 nilsson Exp $
*/

/*
 *  Targa codec for pike. Based on the tga plugin for gimp.
 *
 *  The information below is from the original TGA module.
 *
 *
 * Id: tga.c,v 1.6 1999/01/15 17:34:47 unammx Exp
 * TrueVision Targa loading and saving file filter for the Gimp.
 * Targa code Copyright (C) 1997 Raphael FRANCOIS and Gordon Matzigkeit
 *
 * The Targa reading and writing code was written from scratch by
 * Raphael FRANCOIS <fraph@ibm.net> and Gordon Matzigkeit
 * <gord@gnu.ai.mit.edu> based on the TrueVision TGA File Format
 * Specification, Version 2.0:
 *
 *   <URL:ftp://ftp.truevision.com/pub/TGA.File.Format.Spec/>
 *
 * It does not contain any code written for other TGA file loaders.
 * Not even the RLE handling. ;)
 *
 */


/*
**! module Image
**! submodule TGA
**!
*/
#define NO_PIKE_SHORTHAND

#include "global.h"

#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "interpret.h"
#include "svalue.h"
#include "pike_macros.h"
#include "object.h"
#include "program.h"
#include "pike_error.h"
#include "mapping.h"
#include "stralloc.h"
#include "operators.h"
#include "pike_memory.h"
#include "threads.h"
#include "module_support.h"
#include "builtin_functions.h"


#include "image.h"
#include "colortable.h"

#define ROUNDUP_DIVIDE(n,d) (((n) + (d - 1)) / (d))

extern struct program *image_colortable_program;
extern struct program *image_program;

typedef unsigned char guint8;
typedef unsigned char gchar;
typedef unsigned char guchar;
typedef char gint8;
typedef unsigned INT32 guint32;
typedef INT32 gint32;

enum {
  RGB,
  GRAY,
  INDEXED,
};

struct tga_header
{
  guint8 idLength;
  guint8 colorMapType;

  /* The image type. */
#define TGA_TYPE_MAPPED 1
#define TGA_TYPE_COLOR 2
#define TGA_TYPE_GRAY 3
#define TGA_TYPE_MAPPED_RLE 9
#define TGA_TYPE_COLOR_RLE 10
#define TGA_TYPE_GRAY_RLE 11
  guint8 imageType;

  /* Color Map Specification. */
  /* We need to separately specify high and low bytes to avoid endianness
     and alignment problems. */
  guint8 colorMapIndexLo, colorMapIndexHi;
  guint8 colorMapLengthLo, colorMapLengthHi;
  guint8 colorMapSize;

  /* Image Specification. */
  guint8 xOriginLo, xOriginHi;
  guint8 yOriginLo, yOriginHi;

  guint8 widthLo, widthHi;
  guint8 heightLo, heightHi;

  guint8 bpp;

  /* Image descriptor.
     3-0: alpha bpp
     4:   left-to-right ordering
     5:   top-to-bottom ordering
     7-6: zero
     */
#define TGA_DESC_ABITS 0x0f
#define TGA_DESC_HORIZONTAL 0x10
#define TGA_DESC_VERTICAL 0x20
  guint8 descriptor;
};

struct tga_footer
{
  guint32 extensionAreaOffset;
  guint32 developerDirectoryOffset;
#define TGA_SIGNATURE "TRUEVISION-XFILE"
  gchar signature[16];
  gchar dot;
  gchar null;
};

struct buffer
{
  size_t len;
  char *str;
};

struct image_alpha
{
  struct image *img;
  struct object *io;
  struct image *alpha;
  struct object *ao;
};

static struct image_alpha ReadImage (struct buffer *, struct tga_header *);

static struct image_alpha load_image(struct pike_string *str)
{
  struct tga_header hdr;
  /* struct tga_footer footer; */
  struct buffer buffer;
  INT32 image_ID = -1;

  buffer.str = str->str;
  buffer.len = str->len;

  if(buffer.len < ((sizeof(struct tga_footer)+sizeof(struct tga_header))))
    Pike_error("Data (%ld bytes) is too short\n",
	  DO_NOT_WARN((long)buffer.len));


/*   MEMCPY(&footer, (buffer.str+(buffer.len-sizeof(struct tga_footer))), */
/*          sizeof( struct tga_footer) ); */

  hdr = *((struct tga_header *)buffer.str);
  buffer.len -= sizeof(struct tga_header);
  buffer.str += sizeof(struct tga_header);
  buffer.str += hdr.idLength;
  buffer.len -= hdr.idLength;

  if( (hdr.bpp != 8) && (hdr.bpp != 16) && (hdr.bpp != 24) && (hdr.bpp != 32) )
    Pike_error("Unsupported TGA file (bpp==%d)\n", hdr.bpp);

  if( hdr.imageType > 11 )
    Pike_error("Unsupported TGA image type\n");

  if(buffer.len < 3)
    Pike_error("Not enough data in buffer to decode a TGA image\n");

  return ReadImage (&buffer, &hdr);
}

static ptrdiff_t std_fread (unsigned char *buf,
			    size_t datasize, size_t nelems, struct buffer *fp)
{
  size_t amnt = MINIMUM((nelems*datasize), fp->len);
  MEMCPY(buf, fp->str, amnt);
  fp->len -= amnt;
  fp->str += amnt;
  return amnt / datasize;
}

static ptrdiff_t std_fwrite (unsigned char *buf,
			     size_t datasize, size_t nelems, struct buffer *fp)
{
  size_t amnt = MINIMUM((nelems*datasize), fp->len);
  MEMCPY(fp->str, buf, amnt);
  fp->len -= amnt;
  fp->str += amnt;
  return amnt / datasize;
}

static int std_fgetc( struct buffer *fp )
{
  if(fp->len >= 1)
  {
    fp->len--;
    return (int)*((unsigned char *)fp->str++);
  }
  return EOF;
}

static int std_fputc( int c, struct buffer *fp )
{
  if(fp->len >= 1)
  {
    fp->len--;
    *fp->str++=c;
    return 1;
  }
  return EOF;
}

#define RLE_PACKETSIZE 0x80

/* Decode a bufferful of file. */

static ptrdiff_t rle_fread (guchar *buf, size_t datasize, size_t nelems,
			    struct buffer *fp)
{
  /* If we want to call this function more than once per image, change the
     variables below to be static..  */
  guchar *statebuf = 0;
  ptrdiff_t statelen = 0;
  ptrdiff_t laststate = 0;

  /* end static variables.. */
  ptrdiff_t j, k;
  ptrdiff_t buflen, count, bytes;
  guchar *p;

  /* Scale the buffer length. */
  buflen = nelems * datasize;

  j = 0;
  while (j < buflen)
  {
    if (laststate < statelen)
    {
      /* Copy bytes from our previously decoded buffer. */
      bytes = MINIMUM (buflen - j, statelen - laststate);
      MEMCPY (buf + j, statebuf + laststate, bytes);
      j += bytes;
      laststate += bytes;

      /* If we used up all of our state bytes, then reset them. */
      if (laststate >= statelen)
      {
        laststate = 0;
        statelen = 0;
      }

      /* If we filled the buffer, then exit the loop. */
      if (j >= buflen)
        break;
    }

    /* Decode the next packet. */
    count = std_fgetc (fp);
    if (count == EOF)
    {
      return j / datasize;
    }

    /* Scale the byte length to the size of the data. */
    bytes = ((count & ~RLE_PACKETSIZE) + 1) * datasize;

    if (j + bytes <= buflen)
    {
      /* We can copy directly into the image buffer. */
      p = buf + j;
    }
    else
    {
      /* Allocate the state buffer if we haven't already. */
      if (!statebuf)
        statebuf = (unsigned char *) malloc (RLE_PACKETSIZE * datasize);
      p = statebuf;
    }

    if (count & RLE_PACKETSIZE)
    {
      /* Fill the buffer with the next value. */
      if (std_fread (p, datasize, 1, fp) != 1)
      {
        return j / datasize;
      }

      /* Optimized case for single-byte encoded data. */
      if (datasize == 1)
        MEMSET (p + 1, *p, bytes - 1);
      else
        for (k = datasize; k < bytes; k += datasize)
          MEMCPY (p + k, p, datasize);
    }
    else
    {
      /* Read in the buffer. */
      if (std_fread (p, bytes, 1, fp) != 1)
        return j / datasize;
    }

    /* We may need to copy bytes from the state buffer. */
    if (p == statebuf)
      statelen = bytes;
    else
      j += bytes;
  }
  return nelems;
}


/* This function is stateless, which means that we always finish packets
   on buffer boundaries.  As a beneficial side-effect, rle_fread
   never has to allocate a state buffer when it loads our files, provided
   it is called using the same buffer lengths!

   So, we get better compression than line-by-line encoders, and better
   loading performance than whole-stream images.
*/

/* RunLength Encode a bufferful of file. */
static ptrdiff_t rle_fwrite (guchar *buf, size_t datasize, size_t nelems,
			     struct buffer *fp)
{
  /* Now runlength-encode the whole buffer. */
  ptrdiff_t count, j, buflen;
  guchar *begin;

  /* Scale the buffer length. */
  buflen = datasize * nelems;

  begin = buf;
  j = datasize;
  while (j < buflen)
  {
    /* BUF[J] is our lookahead element, BEGIN is the beginning of this
       run, and COUNT is the number of elements in this run. */
    if (memcmp (buf + j, begin, datasize) == 0)
    {
      /* We have a run of identical characters. */
      count = 1;
      do
      {
        j += datasize;
        count ++;
      }
      while (j < buflen && count < RLE_PACKETSIZE &&
             memcmp (buf + j, begin, datasize) == 0);

      /* J now either points to the beginning of the next run,
         or close to the end of the buffer. */

      /* Write out the run. */
      if (std_fputc(DO_NOT_WARN((int)(count - 1))|RLE_PACKETSIZE, fp) == EOF ||
	  std_fwrite(begin, datasize, 1, fp) != 1)
        return 0;

    }
    else
    {
      /* We have a run of raw characters. */
      count = 0;
      do
      {
        j += datasize;
        count ++;
      }
      while (j < buflen && count < RLE_PACKETSIZE &&
             memcmp (buf + j - datasize, buf + j, datasize) != 0);

      /* Back up to the previous character. */
      j -= datasize;

      /* J now either points to the beginning of the next run,
         or at the end of the buffer. */

      /* Write out the raw packet. */
      if (std_fputc(DO_NOT_WARN((int)(count - 1)), fp) == EOF ||
          std_fwrite(begin, datasize, count, fp) != count)
        return 0;
    }

    /* Set the beginning of the next run and the next lookahead. */
    begin = buf + j;
    j += datasize;
  }

  /* If we didn't encode all the elements, write one last packet. */
  if (begin < buf + buflen)
  {
    if (std_fputc (0, fp) == EOF ||
        std_fwrite (begin, datasize, 1, fp) != 1)
      return 0;
  }

  return nelems;
}

static int getbit( unsigned char **p, int *o )
{
  int bit = (((*p)[0]) & (1<<*o));
  (*o)++;
  if( *o == 8 )
  {
    (*o) = 0;
    (*p)++;
  }
  return bit ? 1 : 0;
}

static int getbits( unsigned char **pointer, int numbits, int *bittoffset,
                    int wantedbits )
{
  int result = 0;
  int scale = (1<<wantedbits)-1;
  int scale2 = (1<<numbits)-1;
  if(!numbits) return 0;
  while( numbits-- ) result = (result << 1) | getbit( pointer,bittoffset );
  return (result * scale) / scale2;
}

static void swap_every_other_byte( unsigned char *p, size_t nelems )
{
  size_t i;
  for( i = 0; i<nelems; i+=2 )
  {
    unsigned char tmp = p[i];
    p[i] = p[i+1];
    p[i+1] = tmp;
  }
}

static unsigned short extract_le_short( unsigned char **data )
{
  (*data)+=2;
  return (*data)[-2] | ((*data)[-1]<<8);
}

static unsigned char c5to8bit( unsigned char v )
{
  return (v << 3) + (v >> 2);
}

static struct image_alpha ReadImage(struct buffer *fp, struct tga_header *hdr)
{
  int width, height, bpp, abpp, pbpp, bypp;
  int pelbytes=0, rle=0;
  unsigned char *cmap=NULL, *data;
  int itype=0;
  int really_no_alpha = 0;
  ptrdiff_t pels, npels, read_so_far = 0;
  ptrdiff_t (*myfread)(unsigned char *, size_t, size_t, struct buffer *);

  /* Find out whether the image is horizontally or vertically reversed.
     The GIMP likes things left-to-right, top-to-bottom. */
  int horzrev = hdr->descriptor & TGA_DESC_HORIZONTAL;
  int vertrev = !(hdr->descriptor & TGA_DESC_VERTICAL);

  /* Reassemble the multi-byte values correctly, regardless of
     host endianness. */
  width = (hdr->widthHi << 8) | hdr->widthLo;
  height = (hdr->heightHi << 8) | hdr->heightLo;

  bpp = hdr->bpp;
  abpp = hdr->descriptor & TGA_DESC_ABITS;
  if (hdr->imageType == TGA_TYPE_COLOR ||
      hdr->imageType == TGA_TYPE_COLOR_RLE)
    pbpp = MINIMUM (bpp / 3, 8) * 3;
  else if (abpp < bpp)
    pbpp = bpp - abpp;
  else
    pbpp = bpp;

  if (abpp + pbpp > bpp)
  {
    /* Assume that alpha bits were set incorrectly. */
    abpp = bpp - pbpp;
  }
  else if (abpp + pbpp < bpp)
  {
    /* Again, assume that alpha bits were set incorrectly. */
    abpp = bpp - pbpp;
    really_no_alpha = 1;
  }


  if((double)width * (double)height * bpp  > (double)INT_MAX )
    Pike_error("Too large image (width * height * bpp overflows)\n");


  switch (hdr->imageType)
  {
   case TGA_TYPE_MAPPED_RLE:
     rle = 1;
   case TGA_TYPE_MAPPED:
     itype = INDEXED;

     /* Find the size of palette elements. */
     pbpp = MINIMUM (hdr->colorMapSize / 3, 8) * 3;
     if (pbpp < hdr->colorMapSize)
       abpp = hdr->colorMapSize - pbpp;
     else
       abpp = 0;

     if (bpp != 8)
       /* We can only cope with 8-bit indices. */
       Pike_error ("TGA: index sizes other than 8 bits are unimplemented\n");
     break;

   case TGA_TYPE_GRAY_RLE:
     rle = 1;
   case TGA_TYPE_GRAY:
     itype = GRAY;
     break;

   case TGA_TYPE_COLOR_RLE:
     rle = 1;
   case TGA_TYPE_COLOR:
     itype = RGB;
     break;

   default:
     Pike_error ("TGA: unrecognized image type %d\n", hdr->imageType);
  }
  /* Check that we have a color map only when we need it. */
  if (itype == INDEXED)
  {
    if (hdr->colorMapType != 1)
      Pike_error ("TGA: indexed image has invalid color map type %d\n",
              hdr->colorMapType);
  }
  else if (hdr->colorMapType != 0)
  {
    Pike_error ("TGA: non-indexed image has invalid color map type %d\n",
            hdr->colorMapType);
  }

  if (hdr->colorMapType == 1)
  {
    /* We need to read in the colormap. */
    int index, length, colors;

    index = (hdr->colorMapIndexHi << 8) | hdr->colorMapIndexLo;
    length = (hdr->colorMapLengthHi << 8) | hdr->colorMapLengthLo;

    if (length == 0)
      Pike_error ("TGA: invalid color map length %d\n", length);

    pelbytes = ROUNDUP_DIVIDE (hdr->colorMapSize, 8);
    colors = length + index;
    cmap = malloc (colors * pelbytes);

    /* Zero the entries up to the beginning of the map. */
    MEMSET (cmap, 0, index * pelbytes);

    /* Read in the rest of the colormap. */
    if (std_fread (cmap + (index * pelbytes), pelbytes, length, fp) != length)
    {
      free(cmap);
      Pike_error ("TGA: error reading colormap\n");
    }

    /* Now pretend as if we only have 8 bpp. */
    abpp = 0;
    pbpp = 8;
  }


  /* Allocate the data. */
  data = (guchar *) malloc (ROUNDUP_DIVIDE((width * height * bpp), 8));
  if(!data)
    Pike_error("TGA: malloc failed\n");

  if (rle)
    myfread = rle_fread;
  else
    myfread = std_fread;

  npels = width*height;
  bypp = ROUNDUP_DIVIDE(bpp,8);
 /* Suck in the data. */
  pels = myfread(data+read_so_far,bypp,npels,fp);
  read_so_far += pels;
  npels -= pels;
  if(npels)
    MEMSET( data+(read_so_far*bypp), 0, npels*bypp );

  /* Now convert the data to two image objects.  */
  {
    int x, y;
    struct image_alpha i;
    unsigned char *sd = data;
    rgb_group *id;
    rgb_group *ad;
    push_int( width );
    push_int( height );
    i.io = clone_object( image_program, 2 );
    i.img = (struct image*)get_storage(i.io,image_program);
    push_int( width );
    push_int( height );
    push_int( 255 );
    push_int( 255 );
    push_int( 255 );
    i.ao = clone_object( image_program, 5 );
    i.alpha = (struct image*)get_storage(i.ao,image_program);

    id = i.img->img;
    ad = i.alpha->img;
    if( bpp == 16 && pbpp == 15 && itype == RGB )
    {
      for( y = 0; y<height; y++ )
        for(x = 0; x<width; x++)
        {
          unsigned short pixel = extract_le_short(&sd);
          id->b = c5to8bit((unsigned char)(pixel & 31));
          id->g = c5to8bit((unsigned char)((pixel & 922)>>5));
          id->r = c5to8bit((unsigned char)((pixel & 31744)>>10));
          id++;
        }
    }
    else /* 8 bits / channel. Simple and fast implementation.  */
    {
      switch( itype )
      {
       case INDEXED:
         for(y = 0; y<height; y++)
           for(x = 0; x<width; x++)
           {
             int cmapind = (*sd)*pelbytes;
             id->b = cmap[cmapind++];
             id->g = cmap[cmapind++];
             (id++)->r = cmap[cmapind++];
             if(pelbytes>3)
             {
               ad->r = ad->g = ad->b = cmap[cmapind];
	       ad++;
             }
             sd++;
           }
         break;
       case GRAY:
         for(y = 0; y<height; y++)
           for(x = 0; x<width; x++)
           {
             id->r =  id->g = id->b = *(sd++);
	     id++;
             if(abpp)
             {
               ad->r = ad->g = ad->b = *(sd++);
	       ad++;
             }
           }
         break;
       case RGB:
         for(y = 0; y<height; y++)
           for(x = 0; x<width; x++)
           {
             id->b = *(sd++);
             id->g = *(sd++);
             (id++)->r = *(sd++);
             if(abpp)
             {
               if( !really_no_alpha )
	       {
                 ad->r = ad->g = ad->b = *sd;
		 ad++;
	       }
               sd++;
             }
           }
      }
    }
    free (data);
    if(cmap) free (cmap);
    if(horzrev)
    {
      apply( i.io, "mirrorx", 0 );
      free_object(i.io);
      i.io = Pike_sp[-1].u.object;
      Pike_sp--;
      apply( i.ao, "mirrorx", 0 );
      free_object(i.ao);
      i.ao = Pike_sp[-1].u.object;
      Pike_sp--;
    }
    if(vertrev)
    {
      apply( i.io, "mirrory", 0 );
      free_object(i.io);
      i.io = Pike_sp[-1].u.object;
      Pike_sp--;
      apply( i.ao, "mirrory", 0 );
      free_object(i.ao);
      i.ao = Pike_sp[-1].u.object;
      Pike_sp--;
    }
    return i;
  }
}


#define SAVE_ID_STRING "Pike image library TGA"

static struct buffer save_tga(struct image *img, struct image *alpha,
                              int rle_encode)
{
  int width, height;
  struct buffer buf;
  struct buffer obuf;
  struct buffer *fp = &buf;
  struct tga_header hdr;
  ONERROR err;
  ptrdiff_t (*myfwrite)(unsigned char *, size_t, size_t,
			struct buffer *);


  if(alpha &&
     (alpha->xsize != img->xsize ||
      alpha->ysize != img->ysize ))
    Pike_error("Alpha and image objects are not equally sized.\n");

  width = img->xsize;
  height = img->ysize;

  memset (&hdr, 0, sizeof (hdr));

  /* We like our images top-to-bottom, thank you! */
  hdr.descriptor |= TGA_DESC_VERTICAL;

  /* Choose the imageType based on alpha precense and compression options. */

  hdr.bpp = 24;
  hdr.imageType = TGA_TYPE_COLOR;

  if(alpha)
  {
    hdr.bpp += 8;
    hdr.descriptor |= 8;
  }

  if (rle_encode)
  {
    /* Here we take advantage of the fact that the RLE image type codes
       are exactly 8 greater than the non-RLE. */
    hdr.imageType += 8;
    myfwrite = rle_fwrite;
  }
  else
    myfwrite = std_fwrite;

  hdr.widthLo = (width & 0xff);
  hdr.widthHi = (width >> 8);

  hdr.heightLo = (height & 0xff);
  hdr.heightHi = (height >> 8);

  /* Mark our save ID. */
  hdr.idLength = DO_NOT_WARN((unsigned INT32)strlen(SAVE_ID_STRING));

  buf.len = width*height*(alpha?4:3)+strlen(SAVE_ID_STRING)+sizeof(hdr)+65535;
  buf.str = xalloc(buf.len);
  obuf.len = buf.len;
  obuf.str = buf.str;
  SET_ONERROR(err, free, obuf.str);

  /* Just write the header. */
  if (std_fwrite((void *)&hdr, sizeof (hdr), 1, fp) != 1)
    Pike_error(msg_out_of_mem);

  if (std_fwrite ((void *)SAVE_ID_STRING, hdr.idLength, 1, fp) != 1)
    Pike_error(msg_out_of_mem);

  /* Allocate a new set of pixels. */

  /* Write out the pixel data. */
  {
    char *data, *p;
    int datalen;
    int pixsize=3;
    int x, y;
    rgb_group *is = img->img;
    if(alpha)
    {
      rgb_group *as = alpha->img;
      pixsize++;
      p = data = xalloc( width*height*4 );
      datalen = width*height*4;
      for(y=0; y<height; y++)
        for(x=0; x<width; x++)
        {
          *(p++) = is->b;
          *(p++) = is->g;
          *(p++) = (is++)->r;
          *(p++) = ((int)as->r+(int)as->g*2+as->b)/4;
	  as++;
        }
    } else {
      p = data = xalloc( width*height*3 );
      datalen = width*height*3;
      for(y=0; y<height; y++)
        for(x=0; x<width; x++)
        {
          *(p++) = is->b;
          *(p++) = is->g;
          *(p++) = (is++)->r;
        }
    }
    if (myfwrite((void *)data, pixsize,datalen/pixsize, fp) !=
	datalen/pixsize)
    {
      free(data);
      Pike_error(msg_out_of_mem);
    }
    free(data);
  }
  UNSET_ONERROR(err);
  obuf.len -= buf.len;
  return obuf;
}






/* Pike functions. */
/*
**! method object _decode(string data)
**! 	Decodes a Targa image to a mapping.
**!       The mapping follows this format:
**!           ([ "image":img_object, "alpha":alpha_channel ])
**!
**! note
**!	Throws upon error in data.
*/
void image_tga__decode( INT32 args )
{
  struct pike_string *data;
  struct image_alpha i;
  get_all_args( "Image.TGA._decode", args, "%S", &data );
  i = load_image( data );

  pop_n_elems(args);

  push_constant_text( "alpha" );
  push_object( i.ao );
  push_constant_text( "image" );
  push_object( i.io );

  push_constant_text( "type" );
  push_constant_text( "image/x-targa" );

  push_constant_text( "xsize" );
  push_int( i.img->xsize );
  push_constant_text( "ysize" );
  push_int( i.img->ysize );

  f_aggregate_mapping( 10 );
}

/*
**! method object decode(string data)
**! 	Decodes a Targa image.
**!
**! note
**!	Throws upon error in data.
*/
void image_tga_decode( INT32 args )
{
  struct pike_string *data;
  struct image_alpha i;
  get_all_args( "Image.TGA.decode", args, "%S", &data );
  i = load_image(data);
  pop_n_elems(args);
  free_object( i.ao );
  push_object( i.io );
}


/*
**! method string encode(object image)
**! method string encode(object image, mapping options)
**! 	Encodes a Targa image.
**!
**!     The <tt>options</tt> argument may be a mapping
**!	containing zero or more encoding options:
**!
**!	<pre>
**!	normal options:
**!	    "alpha":image object
**!		Use this image as alpha channel
**!		(Note: Targa alpha channel is grey.
**!		 The values are calculated by (r+2g+b)/4.)
**!
**!	    "raw":1
**!		Do not RLE encode the image
**!
**!	</pre>
**!
*/

static struct pike_string *param_raw;
static struct pike_string *param_alpha;
void image_tga_encode( INT32 args )
{
  struct image *img = NULL;
  struct image *alpha = NULL;
  struct buffer buf;
  int rle = 1;
  if (!args)
    Pike_error("Image.TGA.encode: too few arguments\n");

  if (Pike_sp[-args].type!=PIKE_T_OBJECT ||
      !(img=(struct image*)
        get_storage(Pike_sp[-args].u.object,image_program)))
    Pike_error("Image.TGA.encode: illegal argument 1\n");

  if (!img->img)
    Pike_error("Image.TGA.encode: no image\n");

  if (args>1)
  {
    if (Pike_sp[1-args].type!=PIKE_T_MAPPING)
      Pike_error("Image.TGA.encode: illegal argument 2\n");

    push_svalue(Pike_sp+1-args);
    ref_push_string(param_alpha);
    f_index(2);
    if (!(Pike_sp[-1].type==PIKE_T_INT
          && Pike_sp[-1].subtype==NUMBER_UNDEFINED))
      if (Pike_sp[-1].type!=PIKE_T_OBJECT ||
          !(alpha=(struct image*)
            get_storage(Pike_sp[-1].u.object,image_program)))
        Pike_error("Image.TGA.encode: option (arg 2) \"alpha\" has illegal type\n");
    pop_stack();

    if (alpha &&
        (alpha->xsize!=img->xsize ||
         alpha->ysize!=img->ysize))
      Pike_error("Image.TGA.encode option (arg 2) \"alpha\"; images differ in size\n");
    if (alpha && !alpha->img)
      Pike_error("Image.TGA.encode option (arg 2) \"alpha\"; no image\n");

    push_svalue(Pike_sp+1-args);
    ref_push_string(param_raw);
    f_index(2);
    rle = !Pike_sp[-1].u.integer;
    pop_stack();
  }

  buf = save_tga( img, alpha, rle );
  pop_n_elems(args);
  push_string( make_shared_binary_string( buf.str, buf.len ));
  free(buf.str);
}


static struct program *image_encoding_tga_program=NULL;
void init_image_tga( )
{
   pike_add_function( "_decode", image_tga__decode,
                 "function(string:mapping(string:object))", 0);
   pike_add_function( "decode", image_tga_decode,
                 "function(string:object)", 0);
   pike_add_function( "encode", image_tga_encode,
                 "function(object,mapping|void:string)", 0);

   param_alpha=make_shared_string("alpha");
   param_raw=make_shared_string("raw");
}

void exit_image_tga(void)
{
   free_string(param_alpha);
   free_string(param_raw);
}
