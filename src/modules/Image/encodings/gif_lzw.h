/*
**! module Image
**! note
**!	$Id: gif_lzw.h,v 1.2 1997/10/27 22:41:37 mirar Exp $
*/

struct gif_lzw
{
};

/* returns number of strings written on stack */

int image_gif_lzw_init(struct gif_lzw *lzw,int bits);
int image_gif_lzw_add(struct gif_lzw *lzw,unsigned char *data,int len);
int image_gif_lzw_finish(struct gif_lzw *lzw);
