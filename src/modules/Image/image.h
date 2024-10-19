/* -*- mode: C; c-basic-offset: 3; -*-
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifdef PIKE_IMAGE_IMAGE_H
#error image.h included twice
#endif

#define PIKE_IMAGE_IMAGE_H

/* Various X86 dialects. */
#define IMAGE_MMX   1
#define IMAGE_SSE   2
#define IMAGE_3DNOW 4
#define IMAGE_EMMX  8
extern int image_cpuid;

#define COLORTYPE unsigned char
#define COLORMAX 255
#define COLORLMAX 0x7fffffff
#define COLORLBITS 31

#define COLORL_TO_COLOR(X) ((COLORTYPE)((X)>>23))
#define COLOR_TO_COLORL(X) ((((INT32)(X))*0x0808080)+((X)>>1))
#define COLOR_TO_FLOAT(X) (((float)(X))/(float)COLORMAX)
#define COLORL_TO_FLOAT(X) ((float)((((float)(X))/(float)(COLORLMAX>>8))/256.0))

#define CHECK_INIT() if(!THIS->img) \
    Pike_error("Image object not initialized.\n");

#define RGB_TO_RGBL(RGBL,RGB) (((RGBL).r=COLOR_TO_COLORL((RGB).r)),((RGBL).g=COLOR_TO_COLORL((RGB).g)),((RGBL).b=COLOR_TO_COLORL((RGB).b)))
#define RGBL_TO_RGB(RGB,RGBL) (((RGB).r=COLORL_TO_COLOR((RGBL).r)),((RGB).g=COLORL_TO_COLOR((RGBL).g)),((RGB).b=COLORL_TO_COLOR((RGBL).b)))

#define testrange(x) _testrange((int)x)

static inline int _testrange(int x)
{
  if(x>255) return 255;
  if(x<0) return 0;
  return x;
}

static inline int absdiff(int a, int b)
{
  return a<b?(b-a):(a-b);
}

#define pixel(_img,x,y) ((_img)->img[((int)(x))+((int)(y))*(int)(_img)->xsize])

#define apply_alpha(x,y,alpha) \
   ((unsigned char)((y*(255L-(alpha))+x*(alpha))/255L))

#define set_rgb_group_alpha(dest,src,alpha) \
   ((dest).r=apply_alpha((dest).r,(src).r,alpha), \
    (dest).g=apply_alpha((dest).g,(src).g,alpha), \
    (dest).b=apply_alpha((dest).b,(src).b,alpha))

static inline INT32 PIKE_UNUSED_ATTRIBUTE FLOAT_TO_COLORL(double X)
{
  /* stupid floats */
  /* The problem here is that the range of X is 0.0 - 1.0 inclusive,
   * while it would be simpler if the max value wasn't reachable.
   *
   * A straight multiplication with COLORLMAX would map too few
   * colors to COLORLMAX. We thus multiply with the size of the
   * COLORL range (0 - COLORLMAX, ie (COLORLMAX + 1)) and check
   * for the special case of X == 1.0.
   */
  unsigned INT32 tmp = (unsigned INT32)(X * ((double)COLORLMAX + 1.0));
  if (UNLIKELY(tmp > (unsigned INT32)COLORLMAX)) return COLORLMAX;
  return (INT32)tmp;
}

#ifdef USE_VALGRIND
/* Workaround for valgrind false alarms: gcc (4.2.3) can generate code
 * that reads a full native integer from memory when we retrieve an
 * rgb_group. This makes valgrind complain if it's done past the end
 * of an alloc'ed block, so pad some extra. */
#define RGB_VEC_PAD (SIZEOF_CHAR_P - sizeof (rgb_group))
#else
#define RGB_VEC_PAD 1
#endif

#define FS_SCALE 1024

typedef struct
{
   COLORTYPE r,g,b;
/*   COLORTYPE __padding_dont_use__;*/
} rgb_group;

typedef struct
{
   COLORTYPE r,g,b,alpha;
} rgba_group;

typedef struct
{
   INT32 r,g,b;
} rgbl_group;

typedef struct
{
   float r,g,b;
} rgbd_group; /* use float, it gets so big otherwise... */

typedef struct
{
   float r,g,b,alpha;
} rgbda_group; /* use float, it gets so big otherwise... */

struct image
{
   rgb_group *img;
   INT_TYPE xsize,ysize;
   rgb_group rgb;
   unsigned char alpha;
};

struct color_struct
{
   rgb_group rgb;
   rgbl_group rgbl;
   struct pike_string *name;
};

#define tColor tOr3(tLArr(tInt3,tInt8bit),tString,tObj)
#define tLayerMap tMap(tString,tOr4(tString,tColor,tFloat,tInt))

/* blit.c */

void img_clear(rgb_group *dest, rgb_group rgb, ptrdiff_t size);
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

/* x.c */

void image_x_encode_pseudocolor(INT32 args);

/* pattern.c */

void image_noise(INT32 args);
void image_turbulence(INT32 args);
void image_random(INT32 args);
void image_randomgrey(INT32 args);
void image_noise_init(void);

/* dct.c */

void image_dct(INT32 args);

/* operator.c */

void image_operator_minus(INT32 args);
void image_operator_plus(INT32 args);
void image_operator_multiply(INT32 args);
void image_operator_divide(INT32 args);
void image_operator_rest(INT32 args);
void image_operator_maximum(INT32 args);
void image_operator_minimum(INT32 args);

void image_operator_equal(INT32 args);
void image_operator_lesser(INT32 args);
void image_operator_greater(INT32 args);

void image_cast(INT32 args);

void image_min(INT32 args);
void image_max(INT32 args);
void image_sum(INT32 args);
void image_sumf(INT32 args);
void image_average(INT32 args);

void image_find_max(INT32 args);
void image_find_min(INT32 args);

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

void image_polyfill(INT32 args);

/* orient.c */

void image_orient(INT32 args);
void image_orient4(INT32 args);

/* color.c */

int image_color_svalue(struct svalue *v,rgb_group *rgb_dest);
int image_color_arg(int arg,rgb_group *rgb_dest);
void _image_make_rgb_color(INT32 r,INT32 g,INT32 b);

/* search.c */

void image_match_phase(INT32 args);
void image_match_norm(INT32 args);
void image_match_norm_corr(INT32 args);
void image_match(INT32 args);

void image_phaseh(INT32 args);
void image_phasev(INT32 args);
void image_phasehv(INT32 args);
void image_phasevh(INT32 args);
void image_apply_max(INT32 args);

void image_make_ascii(INT32 args);

/* image.c */

void img_find_autocrop(struct image *this,
		       INT32 *px1,INT32 *py1,INT32 *px2,INT32 *py2,
		       int border,
		       int left,int right,
		       int top,int bottom,
		       int rgb_set,
		       rgb_group rgb);

/* layers.c */

void image_lay(INT32 args);
