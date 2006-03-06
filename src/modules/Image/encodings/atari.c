/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: atari.c,v 1.4 2006/03/06 08:51:19 peter Exp $
*/

#include "global.h"
#include "interpret.h"
#include "object.h"

#include "atari.h"

extern struct program *image_program;

void rotate_atari_palette(struct atari_palette* pal, unsigned int left,
			  unsigned int right)
{
  rgb_group tmp;
  unsigned int i;

  if(right>left) {
    tmp = pal->colors[right];
    for(i=right; i>left; i--)
      pal->colors[i] = pal->colors[i-1];
    pal->colors[left] = tmp;
  }
  else {
    tmp = pal->colors[left];
    for(i=left; i<right; i++)
      pal->colors[i] = pal->colors[i+1];
    pal->colors[right] = tmp;
  }
}

/* pal is 2*size of palette data */
struct atari_palette* decode_atari_palette(unsigned char *pal,
					  unsigned int size)
{
  unsigned int i;
  struct atari_palette* ret_pal = xalloc(sizeof(struct atari_palette));

  ret_pal->size=size;
  ret_pal->colors=xalloc(size*sizeof(rgb_group));

  if(size==2)
  {
    rgb_group col;
    col.r = col.g = col.b = 0;
    ret_pal->colors[0] = col;
    col.r = col.g = col.b = 255;
    ret_pal->colors[1] = col;
    return ret_pal;
  }

  for(i=0; i<size; i++)
  {
    rgb_group col;
    /* The first part is the ST palette. The second part is
       the STE color bit */
    col.r = (pal[i*2]&7)*36 + ((pal[i*2]&8)?3:0);
    col.g = ((pal[i*2+1]>>4)&7)*36 + ((pal[i*2+1]&0x80)?3:0);
    col.b = (pal[i*2+1]&7)*36 + ((pal[i*2+1]&8)?3:0);
    ret_pal->colors[i] = col;
  }
  return ret_pal;
}

/* q is 32000 bytes of data.
   resolution is { 0=low, 1=medium, 2=high }
*/
struct object* decode_atari_screendump(unsigned char *q,
				      unsigned int resolution,
				      struct atari_palette *pal)
{
  struct object *img=0;
  unsigned int by, bi, c=0;
  rgb_group b,w;

  switch(resolution)
  {
  case 0:
    /* Low resolution (320x200x16) */
    if(pal->size < 16)
      Pike_error("Low res palette too small.\n");

    push_int( 320 );
    push_int( 200 );
    img = clone_object( image_program, 2 );
    for(by=0; by<32000; by+=8)
    {
      for(bi=128; bi!=0; bi=bi>>1)
	((struct image *)img->storage)->img[c++] =
	  pal->colors[ !!(q[by]&bi) + 2*!!(q[by+2]&bi) +
                      4*!!(q[by+4]&bi) + 8*!!(q[by+6]&bi) ];
      for(bi=128; bi!=0; bi=bi>>1)
	((struct image *)img->storage)->img[c++] =
	  pal->colors[ !!(q[by+1]&bi) + 2*!!(q[by+3]&bi) +
                      4*!!(q[by+5]&bi) + 8*!!(q[by+7]&bi) ];
    }
    break;

  case 1:
    /* Medium resolution (640x200x4) */
    if(pal->size < 4)
      Pike_error("Low res palette too small.\n");

    push_int( 640 );
    push_int( 200 );
    img = clone_object( image_program, 2 );
    for(by=0; by<32000; by+=4)
    {
      for(bi=128; bi!=0; bi=bi>>1)
	((struct image *)img->storage)->img[c++] =
	  pal->colors[ !!(q[by]&bi) + 2*!!(q[by+2]&bi) ];
      for(bi=128; bi!=0; bi=bi>>1)
	((struct image *)img->storage)->img[c++] =
	  pal->colors[ !!(q[by+1]&bi) + 2*!!(q[by+3]&bi) ];
    }
    break;

  case 2:
    /* High resolution (640x400x2) */
    push_int( 640 );
    push_int( 400 );
    img = clone_object( image_program, 2 );
    b.r = b.g = b.b = 0;
    w.r = w.g = w.b = 255;
    for(by=0; by<32000; by++)
    {
      for(bi=128; bi!=0; bi=bi>>1)
      {
	if( q[by]&bi )
	  ((struct image *)img->storage)->img[c++] = w;
	else
	  ((struct image *)img->storage)->img[c++] = b;
      }
    }
    break;
  }

  return img;
}
