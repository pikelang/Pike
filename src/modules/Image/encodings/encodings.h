/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#ifndef IMAGE_ENCODINGS_ENCODINGS_H
#define IMAGE_ENCODINGS_ENCODINGS_H

void image_pcx_decode(INT32 args);
void image_pvr_f__decode(INT32 args);
void image_pvr_f_decode_header(INT32 args);
void image_tim_f__decode(INT32 args);
void image_tim_f_decode_header(INT32 args);
void image_xwd__decode(INT32 args);
void image_xwd_decode_header(INT32 args);
void img_bmp__decode(INT32 args);
void img_bmp_decode_header(INT32 args);
void img_ilbm_decode(INT32 args);
void img_pnm_decode(INT32 args);
void img_ras_decode(INT32 args);

#endif /* !IMAGE_ENCODINGS_ENCODINGS_H */
