/*
 * $Id: acconfig.h,v 1.4 2002/10/07 10:40:54 kiwi Exp $
 */

#ifndef GMP_MACHINE_H
#define GMP_MACHINE_H

@TOP@

/* Define if your <jerror.h> defines JERR_BAD_CROP_SPEC */
#undef HAVE_JERR_BAD_CROP_SPEC

@BOTTOM@

/* Define this if you have -ljpeg */
#undef HAVE_LIBJPEG

/* Define this if you have height_in_blocks member in struct 
 * jpeg_component_info */
#undef HAVE_JPEG_HEIGHT_IN_BLOCKS

#endif
