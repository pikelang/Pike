/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: atari.h,v 1.1 2003/09/14 18:50:13 sigge Exp $
*/

#include "global.h"

#include "stralloc.h"

#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "array.h"
#include "mapping.h"
#include "pike_error.h"
#include "threads.h"
#include "builtin_functions.h"
#include "module_support.h"
#include "operators.h"
#include "program.h"

#include "image.h"
#include "colortable.h"

#include "encodings.h"


struct atari_palette
{
  unsigned int size;
  rgb_group *colors;
};

struct atari_palette* decode_atari_palette(unsigned char *pal,
					  unsigned int size);
struct object* decode_atari_screendump(unsigned char *q,
				      unsigned int resolution,
				      struct atari_palette *pal);
