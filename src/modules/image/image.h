/* $Id: image.h,v 1.16 1996/12/05 23:50:52 law Exp $ */

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
   signed long r,g,b;
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

struct colortable
{
   int numcol;
   struct rgb_cache
   {
      rgb_group index;
      int value;
   } cache[QUANT_SELECT_CACHE];
   unsigned long *rgb_node;
/*   bit
     31..30          29..22   21..0
     0=color         split    value
     1=split red     on	this  
     2=split green   =x       <=x     >x
     3=split blue             value   value+1
     It will fail for more than 2097152 colors. Sorry... *grin*
 */
   rgb_group clut[1];
};  /* rgb_node follows, 2*numcol */


/* colortable declarations - from quant */

struct colortable *colortable_quant(struct image *img,int numcol);
int colortable_rgb(struct colortable *ct,rgb_group rgb);
int colortable_rgb_nearest(struct colortable *ct,rgb_group rgb);
void colortable_free(struct colortable *ct);
struct colortable *colortable_from_array(struct array *arr,char *from);

/* encoding of a gif - from togif */

struct pike_string *
   image_encode_gif(struct image *img,struct colortable *ct,
		    rgb_group *transparent,
		    int floyd_steinberg);
void image_floyd_steinberg(rgb_group *rgb,int xsize,
			   rgbl_group *errl,
			   int way,int *res,
			   struct colortable *ct);

int image_decode_gif(struct image *dest,struct image *dest_alpha,
		     unsigned char *src,unsigned long len);

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
