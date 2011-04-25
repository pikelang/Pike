/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#include "global.h"
#include "pike_error.h"

#include "image.h"

struct atari_palette
{
  unsigned int size;
  rgb_group *colors;
};

void rotate_atari_palette(struct atari_palette* pal, unsigned int left,
			  unsigned int right);
struct atari_palette* decode_atari_palette(unsigned char *pal,
					  unsigned int size);
struct object* decode_atari_screendump(unsigned char *q,
				      unsigned int resolution,
				      struct atari_palette *pal);
