
#define MAX_NUMCOL 32768
#define QUANT_MAP_BITS 4
#define QUANT_MAP_SKIP_BITS (8-(QUANT_MAP_BITS))
#define QUANT_MAP_THIS(X) ((X)>>QUANT_MAP_SKIP_BITS)
#define QUANT_MAP_REAL (1L<<QUANT_MAP_BITS)

#define COLOURTYPE unsigned char

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
   rgb_group clut[1];
};


/* colortable declarations - from quant */

struct colortable *colortable_quant(struct image *img,int numcol);
int colortable_rgb(struct colortable *ct,rgb_group rgb);
void colortable_free(struct colortable *ct);

/* encoding of a gif - from togif */

struct pike_string *
   image_encode_gif(struct image *img,struct colortable *ct,
		    rgb_group *transparent);

