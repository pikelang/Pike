
#define MAX_NUMCOL 32768
#define QUANT_MAP_BITS 4
#define QUANT_MAP_SKIP_BITS (8-(QUANT_MAP_BITS))
#define QUANT_MAP_THIS(X) ((X)>>QUANT_MAP_SKIP_BITS)
#define QUANT_MAP_REAL (1L<<QUANT_MAP_BITS)
#define QUANT_SELECT_CACHE 5

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
   struct map_entry
   {
      unsigned char cl;
      unsigned char used; 
      struct map_entry *next;
   } map[QUANT_MAP_REAL][QUANT_MAP_REAL][QUANT_MAP_REAL];
   struct rgb_cache
   {
      rgb_group index;
      int value;
   } cache[QUANT_SELECT_CACHE];
   rgb_group clut[1];
};


/* colortable declarations - from quant */

struct colortable *colortable_quant(struct image *img,int numcol);
int colortable_rgb(struct colortable *ct,rgb_group rgb);
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
