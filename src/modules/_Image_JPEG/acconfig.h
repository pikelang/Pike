/*
 * $Id: acconfig.h,v 1.5 2002/10/08 17:11:15 norrby Exp $
 */

#ifndef GMP_MACHINE_H
#define GMP_MACHINE_H

@TOP@

/* Define if your <jerror.h> defines JERR_BAD_CROP_SPEC */
#undef HAVE_JERR_BAD_CROP_SPEC

@BOTTOM@

/* Define this if you have -ljpeg */
#undef HAVE_LIBJPEG

/* Define this if you don't have image transformation capabilities in jpeglib*/
#undef TRANSFORMS_NOT_SUPPORTED

#endif
