/*
**! module Image
**! note
**!	$Id: image.h,v 1.13 1997/11/11 22:17:50 mirar Exp $
*/

#ifdef PIKE_IMAGE_IMAGE_H
#error IMAGE.h included twice
#endif

#define PIKE_IMAGE_IMAGE_H


#define MAX_NUMCOL 32768

#define QUANT_SELECT_CACHE 6

#define COLOURTYPE unsigned char

#define FS_SCALE 1024

typedef struct 
{
   COLOURTYPE r,g,b;
} rgb_group;

typedef struct 
{
   unsigned char r,g,b,alpha;
} rgba_group;

typedef struct
{
   INT32 r,g,b;
} rgbl_group;

typedef struct
{
   float r,g,b;
} rgbd_group; /* use float, it gets so big otherwise... */

struct image
{
   rgb_group *img;
   INT32 xsize,ysize;
   rgb_group rgb;
   unsigned char alpha;
};

/* COMPAT: encoding of a gif - from togif */

void image_togif(INT32 args);
void image_togif_fs(INT32 args);
void image_fromgif(INT32 args);
void image_gif_begin(INT32 args);
void image_gif_add(INT32 args);
void image_gif_add_fs(INT32 args);
void image_gif_add_nomap(INT32 args);
void image_gif_add_fs_nomap(INT32 args);
void image_gif_end(INT32 args);
void image_gif_netscape_loop(INT32 args);

/* blit.c */

void img_clear(rgb_group *dest,rgb_group rgb,INT32 size);
void img_box_nocheck(INT32 x1,INT32 y1,INT32 x2,INT32 y2);
void img_box(INT32 x1,INT32 y1,INT32 x2,INT32 y2);
void img_blit(rgb_group *dest,rgb_group *src,INT32 width,
	      INT32 lines,INT32 moddest,INT32 modsrc);
void img_crop(struct image *dest,
	      struct image *img,
	      INT32 x1,INT32 y1,
	      INT32 x2,INT32 y2);
void img_clone(struct image *newimg,struct image *img);
void image_paste(INT32 args);
void image_paste_alpha(INT32 args);
void image_paste_mask(INT32 args);
void image_paste_alpha_color(INT32 args);

void image_add_layers(INT32 args);
enum layer_method
   { LAYER_NOP=0, LAYER_MAX=1, LAYER_MIN=2, 
     LAYER_MULT=3, LAYER_ADD=4, LAYER_DIFF=5 };
	 

/* matrix.c */

void image_scale(INT32 args);
void image_translate(INT32 args);
void image_translate_expand(INT32 args);
void image_skewx(INT32 args);
void image_skewy(INT32 args);
void image_skewx_expand(INT32 args);
void image_skewy_expand(INT32 args);
void image_rotate(INT32 args);
void image_rotate_expand(INT32 args);
void image_cw(INT32 args);
void image_ccw(INT32 args);
void image_ccw(INT32 args);
void image_mirrorx(INT32 args);
void image_mirrory(INT32 args);

/* pnm.c */

void image_toppm(INT32 args);
void image_frompnm(INT32 args);
 
/* pattern.c */

void image_noise(INT32 args);
void image_turbulence(INT32 args);
void image_noise_init(void);

/* dct.c */

void image_dct(INT32 args);

/* operator.c */

void image_operator_minus(INT32 args);
void image_operator_plus(INT32 args);
void image_operator_multiply(INT32 args);
void image_operator_maximum(INT32 args);
void image_operator_minimum(INT32 args);
void image_cast(INT32 args);

/* x.c */

void image_to8bit_closest(INT32 args);
void image_to8bit(INT32 args);
void image_to8bit_fs(INT32 args);
void image_tozbgr(INT32 args);
void image_torgb(INT32 args);
void image_to8bit_rgbcube(INT32 args);
void image_to8bit_rgbcube_rdither(INT32 args);
void image_tobitmap(INT32 args);

/* polyfill.c */

void image_polygone(INT32 args);


