/*
**! module Image
**! note
**!	$Id: gif_lzw.h,v 1.3 1997/11/01 14:30:20 noring Exp $
*/

struct gif_lzw
{
  int very_dummy;
};

/* returns number of strings written on stack */

int image_gif_lzw_init(struct gif_lzw *lzw,int bits);
int image_gif_lzw_add(struct gif_lzw *lzw,unsigned char *data,int len);
int image_gif_lzw_finish(struct gif_lzw *lzw);
