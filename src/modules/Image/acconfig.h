/*
 * $Id: acconfig.h,v 1.4 2000/05/29 09:58:24 per Exp $
 */

#ifndef IMAGE_MACHINE_H
#define IMAGE_MACHINE_H

/* define if you want lzw code to generate only rle packing */
#undef GIF_LZW_RLE

/* nasm exists and can be used to make .o-files */
#undef ASSEMBLY_OK

/* Define if you have the m library (-lm).  */
#undef HAVE_LIBM

@TOP@
@BOTTOM@

#endif
