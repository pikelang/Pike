/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: atari.h,v 1.4 2004/05/19 00:06:06 nilsson Exp $
*/

#include "global.h"
#include "pike_error.h"
#include "object.h"

#include "image.h"

struct atari_palette
{
  unsigned int size;
  rgb_group *colors;
};

void rotate_atari_palette(struct atari_palette* pal, unsigned int left,
			  unsigned int right);
void free_atari_palette(struct atari_palette* pal);
struct atari_palette* decode_atari_palette(unsigned char *pal,
					  unsigned int size);
struct object* decode_atari_screendump(unsigned char *q,
				      unsigned int resolution,
				      struct atari_palette *pal);
