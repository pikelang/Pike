/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: encode_truecolor.c,v 1.1 2005/07/28 15:19:36 nilsson Exp $
*/

static unsigned char swap_bits[256] = 
{ 0,128,64,192,32,160,96,224,16,144,80,208,48,176,112,
  240,8,136,72,200,40,168,104,232,24,152,88,216,56,184,
  120,248,4,132,68,196,36,164,100,228,20,148,84,212,52,
  180,116,244,12,140,76,204,44,172,108,236,28,156,92,220,
  60,188,124,252,2,130,66,194,34,162,98,226,18,146,82,
  210,50,178,114,242,10,138,74,202,42,170,106,234,26,154,
  90,218,58,186,122,250,6,134,70,198,38,166,102,230,22,
  150,86,214,54,182,118,246,14,142,78,206,46,174,110,238,
  30,158,94,222,62,190,126,254,1,129,65,193,33,161,97,
  225,17,145,81,209,49,177,113,241,9,137,73,201,41,169,
  105,233,25,153,89,217,57,185,121,249,5,133,69,197,37,
  165,101,229,21,149,85,213,53,181,117,245,13,141,77,205,
  45,173,109,237,29,157,93,221,61,189,125,253,3,131,67,
  195,35,163,99,227,19,147,83,211,51,179,115,243,11,139,
  75,203,43,171,107,235,27,155,91,219,59,187,123,251,7,
  135,71,199,39,167,103,231,23,151,87,215,55,183,119,247,
  15,143,79,207,47,175,111,239,31,159,95,223,63,191,127,255};


static void encode_truecolor_24_rgb_swapped( rgb_group *s,
                                             unsigned char *d,
                                             int q, int w )
{
  while(q--)
  {
    *(d++) = s->b;
    *(d++) = s->g;
    *(d++) = (s++)->r;
  }
}

static void encode_truecolor_24_rgb( rgb_group *s,
                                     unsigned char *d,
                                     int q, int w )
{
  MEMCPY(d,(unsigned char *)s,q);
}

static void encode_truecolor_24_rgb_al32_swapped( rgb_group *s,
                                                  unsigned char *d,
                                                  int q, int w )
{
  int l;
  while(q)
  {
    l=w;
    while(l--)
    {
      *(d++) = s->b;
      *(d++) = s->g;
      *(d++) = (s++)->r;
      q--;
    }
    d += (4-((w*3)&3))&3;
  }
}

static void encode_truecolor_24_rgb_al32( rgb_group *s,
                                          unsigned char *d,
                                          int q, int w )
{
  int l;
  while(q)
  {
    for(l=0; l<(q/w)/3; l++)
    {
      MEMCPY(d,(unsigned char *)s,w*3);
      d += (w*3+3)&~3;
    }
  }
}


#define encode_truecolor_24_bgr encode_truecolor_24_rgb_swapped
#define encode_truecolor_24_bgr_swapped encode_truecolor_24_rgb
#define encode_truecolor_24_bgr_al32 encode_truecolor_24_rgb_al32_swapped
#define encode_truecolor_24_bgr_al32_swapped encode_truecolor_24_rgb_al32

static void encode_truecolor_32_argb_swapped( rgb_group *s,
                                              unsigned char *d,
                                              int q,int w )
{
  int *dp = (int *)d;
#if PIKE_BYTEORDER==1234
  while(q--){ *(dp++) = (s->r<<16) | (s->g<<8) | (s)->b;s++;}
#else
  while(q--){ *(dp++) = ((s->r) | (s->g<<8) | (s)->b<<16)<<8;s++;}
#endif
}

static void encode_truecolor_32_argb( rgb_group *s,
                                      unsigned char *d,
                                      int q,int w )
{
  int *dp = (int *)d;
#if PIKE_BYTEORDER==4321
  while(q--) {*(dp++) = (s->r<<16) | (s->g<<8) | (s)->b;s++;}
#else
  while(q--) {*(dp++) = ((s->r) | (s->g<<8) | (s)->b<<16)<<8;s++;}
#endif
}

static void encode_truecolor_32_abgr( rgb_group *s,
                                      unsigned char *d,
                                      int q,int w )
{
  while(q--)
  {
    *(d++) = 0;
    *(d++) = s->b;
    *(d++) = s->g;
    *(d++) = (s++)->r;
  }
}


static void encode_truecolor_32_abgr_swapped( rgb_group *s,
                                              unsigned char *d,
                                              int q,int w )
{
  while(q--)
  {
    *(d++) = s->r;
    *(d++) = s->g;
    *(d++) = (s++)->b;
    *(d++) = 0;
  }
}

#define ENCODE_8(X,Y,Z)                                                              \
static void encode_truecolor_##X##_##Y( rgb_group *s, unsigned char *d, int q,int w )           \
{                                                                       \
  while(q--)                                                            \
    {*(d++)=(Z);s++;}                                                     \
}                                                                       \
                                                                        \
static void encode_truecolor_##X##_##Y##_swapped( rgb_group *s, unsigned char *d, int q ,int w) \
{                                                                       \
  while(q--)                                                        \
    {*(d++)=(Z);s++;}                                                     \
}


#if PIKE_BYTEORDER == 1234
#define ENCODE_16(X,Y,Z)                                                              \
static void encode_truecolor_##X##_##Y( rgb_group *s, unsigned char *d, int q,int w )           \
{                                                                       \
  unsigned short *dp = (unsigned short *)d;                             \
  while(q--)                                                            \
    {*(dp++) = (Z);s++;}                                                 \
}                                                                       \
                                                                        \
static void encode_truecolor_##X##_##Y##_swapped( rgb_group *s, unsigned char *d, int q ,int w) \
{                                                                       \
  while(q--)                                                            \
  {                                                                     \
    unsigned short v = (Z);                                             \
    *(d++)=v&255;                                                       \
    *(d++)=v>>8;s++;                                                    \
  }                                                                     \
}
#else
#define ENCODE_16(X,Y,Z)                                                              \
static void encode_truecolor_##X##_##Y( rgb_group *s, unsigned char *d, int q,int w )           \
{                                                                       \
  while(q--)                                                            \
  {                                                                     \
    unsigned short v = (Z);                                             \
    *(d++)=v>>8;                                                        \
    *(d++)=v&255;s++;                                                   \
  }                                                                     \
}                                                                       \
                                                                        \
static void encode_truecolor_##X##_##Y##_swapped( rgb_group *s, unsigned char *d, int q ,int w) \
{                                                                       \
  unsigned short *dp = (unsigned short *)d;                             \
  while(q--)                                                            \
    {*(dp++) = (Z);s++;}                                                      \
}
#endif

ENCODE_16(565,rgb,(((s->r>>3)<<6) | (s->g>>2))<<5 | (s->b>>3));
ENCODE_16(565,bgr,(((s->b>>3)<<6) | (s->g>>2))<<5 | (s->r>>3));
ENCODE_16(555,rgb,(((s->r>>3)<<5) | (s->g>>3))<<5 | (s->b>>3));
ENCODE_16(555,bgr,(((s->b>>3)<<5) | (s->g>>3))<<5 | (s->r>>3));

ENCODE_8(232,rgb,(((s->r>>6)<<3) | (s->g>>5))<<2 | (s->b>>6));
ENCODE_8(332,rgb,(((s->r>>5)<<3) | (s->g>>5))<<2 | (s->b>>6));
ENCODE_8(242,rgb,(((s->r>>5)<<4) | (s->g>>4))<<2 | (s->b>>6));
ENCODE_8(233,rgb,(((s->r>>5)<<3) | (s->g>>4))<<3 | (s->b>>5));
ENCODE_8(232,bgr,(((s->b>>6)<<3) | (s->g>>5))<<2 | (s->r>>6));
ENCODE_8(332,bgr,(((s->b>>5)<<3) | (s->g>>5))<<2 | (s->r>>6));
ENCODE_8(242,bgr,(((s->b>>5)<<4) | (s->g>>4))<<2 | (s->r>>6));
ENCODE_8(233,bgr,(((s->b>>5)<<3) | (s->g>>4))<<3 | (s->r>>5));


static void encode_truecolor_generic(int rbits, int rshift, int gbits, 
                                     int gshift, int bbits, int bshift, 
                                     int bpp, int alignbits,int swap_bytes, 
                                     struct image *img, unsigned char *dest,
                                     int len)
{
   unsigned char *d = dest;
   int x,y;
   rgb_group *s;
   s=img->img;

   if(!alignbits) alignbits=1;

#define OPTIMIZE(rb,rs,gb,gs,bb,bs,al,fun) do { \
      if(rbits==rb && rshift==rs && gbits==gb && gshift==gs && \
    	 bbits==bb && bshift==bs && alignbits==al) { \
    	DEBUG_PF(("Optimized case %s%s\n", #fun, swap_bytes?"_swapped":"")); \
    	if (swap_bytes) \
    	  fun##_swapped(s, (void *)dest, img->xsize*img->ysize, img->xsize); \
    	else \
    	  fun(s,(void *)dest,img->xsize*img->ysize,img->xsize); \
        return; \
      } \
    } while(0)

   switch(bpp)
   {
    case 32: /* 4 alternatives */
      OPTIMIZE(8,16,8,8,8,0,1,encode_truecolor_32_argb);
      OPTIMIZE(8,0,8,8,8,16,1,encode_truecolor_32_abgr);
      break;
    case 24: /* 8 alternatives */
      OPTIMIZE(8,16,8,8,8,0,32,encode_truecolor_24_rgb_al32);
      OPTIMIZE(8,0,8,8,8,16,32,encode_truecolor_24_bgr_al32);
      OPTIMIZE(8,16,8,8,8,0,1,encode_truecolor_24_rgb);
      OPTIMIZE(8,0,8,8,8,16,1,encode_truecolor_24_bgr);
      break;
    case 16: /* 8 alternatives */
      OPTIMIZE(5,11,6,5,5,0,1,encode_truecolor_565_rgb);
      OPTIMIZE(5,10,5,5,5,0,1,encode_truecolor_555_rgb);
      OPTIMIZE(5,0,6,5,5,11,1,encode_truecolor_565_bgr);
      OPTIMIZE(5,0,5,5,5,10,1,encode_truecolor_555_bgr);
      break;
    case 8: /* 16 alternatives */
      OPTIMIZE(2,5,3,2,2,0,1,encode_truecolor_232_rgb);
      OPTIMIZE(3,5,3,2,2,0,1,encode_truecolor_332_rgb);
      OPTIMIZE(2,6,4,2,2,0,1,encode_truecolor_242_rgb);
      OPTIMIZE(2,6,3,3,3,0,1,encode_truecolor_233_rgb);
      OPTIMIZE(2,0,3,2,2,5,1,encode_truecolor_232_bgr);
      OPTIMIZE(3,0,3,2,2,5,1,encode_truecolor_332_bgr);
      OPTIMIZE(2,0,4,2,2,6,1,encode_truecolor_242_bgr);
      OPTIMIZE(2,0,3,3,3,6,1,encode_truecolor_233_bgr);
      break;
   }
#undef OPTIMIZE

   DEBUG_PF(("Generic encode_truecolor for r%dxg%dxb%d (%d bpp, shift=r%d,g%d,b%d)\n",
             rbits,gbits,bbits,bpp,rshift,gshift,bshift));
   {
     int rfmask,gfmask,bfmask;
     int rfshift,gfshift,bfshift,rzshift,gzshift,bzshift;
     int bpshift,blinemod,bit;

     *d=0;

     rfshift=rshift-(sizeof(COLORTYPE)*8-rbits);
     gfshift=gshift-(sizeof(COLORTYPE)*8-gbits);
     bfshift=bshift-(sizeof(COLORTYPE)*8-bbits);
     if (rfshift<0) rzshift=-rfshift,rfshift=0; else rzshift=0;
     if (gfshift<0) gzshift=-gfshift,gfshift=0; else gzshift=0;
     if (bfshift<0) bzshift=-bfshift,bfshift=0; else bzshift=0;

     rfmask=(((1<<rbits)-1)<<(sizeof(COLORTYPE)*8-rbits));
     gfmask=(((1<<gbits)-1)<<(sizeof(COLORTYPE)*8-gbits));
     bfmask=(((1<<bbits)-1)<<(sizeof(COLORTYPE)*8-bbits));
     bpshift=sizeof(unsigned long)*8-bpp;
     blinemod=alignbits-(((img->xsize*bpp+7)&~7)%alignbits);
     if(blinemod==alignbits)
       blinemod=0;
     else
       blinemod>>=3;

     bit=0;
     y = img->ysize;
     while (y--)
     {
       int bp;
       x=img->xsize;
       while (x--)
       {
         register unsigned long b =
           ((((s->r&rfmask)>>rzshift)<<rfshift)|
            (((s->g&gfmask)>>gzshift)<<gfshift)|
            (((s->b&bfmask)>>bzshift)<<bfshift))<<bpshift;
         bp = bpp;
         while (bp>8-bit)
         {
	   if(bit)
	     *d|=(unsigned char)(b>>(24+bit));
	   else
	     *d=(unsigned char)(b>>24);
           b<<=8-bit;
           bp-=8-bit;
           bit=0;
	   d++;
         }
	 if(bit)
	   *d|=b>>(24+bit); 
	 else
	   *d=b>>24;
         bit+=bp;
         if (bit==8) ++d,bit=0;
         s++;
       }
       if(!y)
	 break;
       if(bit) ++d,bit=0;
       bp=blinemod;
       while (bp--) *(++d)=0;
     }
   }

   if (swap_bytes)
   {
     d=dest;
     x=len;

     switch (bpp)
     {
      case 32:
        while (x>=4)
        {
          d[0]^=d[3],d[3]^=d[0],d[0]^=d[3];
          d[1]^=d[2],d[2]^=d[1],d[1]^=d[2];
          d+=4;
          x-=4;
        }
        break;
      case 24:
        while (x>=3)
        {
          d[0]^=d[2],d[2]^=d[0],d[0]^=d[2];
          d+=3;
          x-=3;
        }
        break;
      case 16:
        while (x>=2)
        {
          d[0]^=d[1],d[1]^=d[0],d[0]^=d[1];
          d+=2;
          x-=2;
        }
        break;
      case 1: 
        while (x--) {*(d)=swap_bits[*d]; d++; }
        break;
     }
   }
}

static INLINE void examine_mask(unsigned int x, int *bits,int *shift)
{
   *bits=0; 
   *shift=0; 
   if (!x) return;
   while (!(x&1)) x>>=1,(*shift)++;
   while (x&1) x>>=1,(*bits)++;
}

void pgtk_encode_grey(struct image *i, unsigned char *dest, int bpp, int bpl )
{
  int x, y;
  rgb_group *s = i->img;
  switch(bpp)
  {
   case 1:
     for(y=0; y<i->ysize; y++)
     {
       unsigned char *d = dest+y*bpl;
       for(x=0; x<i->xsize; x++,s++)
         *d = (s->r + (s->g<<1) + s->b)>>2;
     }
     return;
   case 2:
     for(y=0; y<i->ysize; y++)
     {
       unsigned short *d = (unsigned short *)(dest+y*bpl);
       for(x=0; x<i->xsize; x++,s++)
         *d = (s->r + (s->g<<1) + s->b)<<6;
     }
     return;
   default:
     Pike_error("This greyscale is to wide for me!\n");
  }
}

void pgtk_encode_truecolor_masks(struct image *i,
                                 int bitspp,
                                 int pad,
                                 int byteorder,
                                 unsigned int red_mask,
                                 unsigned int green_mask,
                                 unsigned int blue_mask,
                                 unsigned char *buffer, 
                                 int debug_len)
{
   int rbits,rshift,gbits,gshift,bbits,bshift;

  examine_mask(red_mask,&rbits,&rshift);
  examine_mask(green_mask,&gbits,&gshift);
  examine_mask(blue_mask,&bbits,&bshift);
  encode_truecolor_generic( rbits, rshift, gbits, gshift, bbits, bshift,
                            bitspp, pad, byteorder, i, buffer, debug_len );

}
