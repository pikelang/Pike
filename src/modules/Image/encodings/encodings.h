/*
 * $Id: encodings.h,v 1.3 2002/07/13 23:46:16 nilsson Exp $
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
