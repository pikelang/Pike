/*

togif 

Pontus Hagland, law@infovav.se

*/

#include <stdlib.h>

#include "stralloc.h"
#include "global.h"
#include "types.h"
#include "dynamic_buffer.h"

#include "image.h"
#include "lzw.h"

#define INITIAL_BUF_LEN 8192


#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)<(b)?(b):(a))
#define testrange(x) max(min((x),255),0)

static void buf_word( unsigned short w, dynamic_buffer *buf )
{
   low_my_putchar( w&0xff, buf );
   low_my_putchar( (w>>8)&0xff, buf );
}

#define FS_SCALE 1024
#define WEIGHT_NEXT(X) (((X)*7)/16)
#define WEIGHT_DOWNNEXT(X) (((X)*0)/20)
#define WEIGHT_DOWN(X) (((X)*5)/16)
#define WEIGHT_DOWNBACK(X) (((X)*0)/20)

static int floyd_steinberg_add(rgbl_group *errl,
			       rgbl_group *errlfwd,
			       rgbl_group *errlback,
			       rgbl_group *err,
			       rgb_group rgb,
			       struct colortable *ct)
{
   rgb_group rgb2,rgb3;
   rgbl_group cerr;
   int c;
   rgb2.r=testrange((long)rgb.r+err->r/FS_SCALE);
   rgb2.g=testrange((long)rgb.g+err->g/FS_SCALE);
   rgb2.b=testrange((long)rgb.b+err->b/FS_SCALE);
#ifdef FS_DEBUG
   fprintf(stderr,"%g,%g,%g+%g,%g,%g=%g,%g,%g ",
	   1.0*rgb.r, 1.0*rgb.g, 1.0*rgb.b, 
	   err->r*1.0/FS_SCALE, err->g*1.0/FS_SCALE, err->b*1.0/FS_SCALE,
	   rgb2.r*1.0, rgb2.g*1.0, rgb2.b*1.0);
#endif
   c=colortable_rgb(ct,rgb2);
   rgb3=ct->clut[c];
   cerr.r=(long)rgb.r*FS_SCALE-(long)rgb3.r*FS_SCALE+err->r;
   cerr.g=(long)rgb.g*FS_SCALE-(long)rgb3.g*FS_SCALE+err->g;
   cerr.b=(long)rgb.b*FS_SCALE-(long)rgb3.b*FS_SCALE+err->b;

#ifdef FS_DEBUG
   fprintf(stderr,"got %g,%g,%g err %g,%g,%g ",
	   1.0*rgb3.r, 
	   1.0*rgb3.g, 
	   1.0*rgb3.b, 
	   1.0*cerr.r,
	   1.0*cerr.g,
	   1.0*cerr.b);
#endif

   errl->r+=WEIGHT_DOWN(cerr.r);
   errl->g+=WEIGHT_DOWN(cerr.g);
   errl->b+=WEIGHT_DOWN(cerr.b);
   if (errlback)
   {
      errlback->r+=WEIGHT_DOWNBACK(cerr.r);
      errlback->g+=WEIGHT_DOWNBACK(cerr.g);
      errlback->b+=WEIGHT_DOWNBACK(cerr.b);
#ifdef FS_DEBUG
      fprintf(stderr,"errlback=>%g ",errlback->g*1.0/FS_SCALE);
#endif
   }
   if (errlfwd)
   {
      err->r=WEIGHT_NEXT(cerr.r);
      err->g=WEIGHT_NEXT(cerr.g);
      err->b=WEIGHT_NEXT(cerr.b);
      err->r+=errlfwd->r;
      err->g+=errlfwd->g;
      err->b+=errlfwd->b;
      errlfwd->r=WEIGHT_DOWNNEXT(cerr.r);
      errlfwd->g=WEIGHT_DOWNNEXT(cerr.g);
      errlfwd->b=WEIGHT_DOWNNEXT(cerr.b);
#ifdef FS_DEBUG
      fprintf(stderr,"errlfwd=>%g ",errlfwd->g*1.0/FS_SCALE);
#endif
   }
#ifdef FS_DEBUG
   fprintf(stderr,"errl=>%g ",errl->g*1.0/FS_SCALE);
   fprintf(stderr,"err=>%g\n",err->g*1.0/FS_SCALE);
#endif
   return c;
}

static void floyd_steinberg(rgb_group *rgb,int xsize,
			    rgbl_group *errl,
			    int way,int *res,
			    struct colortable *ct)
{
   rgbl_group err;
   int x;

   if (way)
   {
      err.r=errl[xsize-1].r;
      err.g=errl[xsize-1].g;
      err.b=errl[xsize-1].b;
      for (x=xsize-1; x>=0; x--)
	 res[x]=floyd_steinberg_add(errl+x,
				    (x==0)?NULL:errl+x-1,
				    (x==xsize-1)?NULL:errl+x+1,
				    &err,rgb[x],ct);
   }
   else
   {
      err.r=errl->r;
      err.g=errl->g;
      err.b=errl->b;
      for (x=0; x<xsize; x++)
	 res[x]=floyd_steinberg_add(errl+x,
				    (x==xsize-1)?NULL:errl+x+1,
				    (x==0)?NULL:errl+x-1,
				    &err,rgb[x],ct);
   }
}
		     

struct pike_string *
   image_encode_gif(struct image *img,struct colortable *ct,
		    rgb_group *transparent,int fs)
{
   dynamic_buffer buf;
   long i;
   rgb_group *rgb;
   struct lzw lzw;

   buf.s.str=NULL;
   low_init_buf(&buf);

   low_my_binary_strcat(transparent?"GIF89a":"GIF87a",6,&buf);
   buf_word((unsigned short)img->xsize,&buf);
   buf_word((unsigned short)img->ysize,&buf);
   low_my_putchar( (char)(0xe0|7), &buf);
   /* 7 is bpp - 1   e is global colormap + resolution (??!) */

   low_my_putchar( 0, &buf ); /* background color */
   low_my_putchar( 0, &buf ); /* just zero */

   for (i=0; i<256; i++)
   {
      low_my_putchar(ct->clut[i].r,&buf);
      low_my_putchar(ct->clut[i].g,&buf);
      low_my_putchar(ct->clut[i].b,&buf);
   }

   if (transparent)
   {
      low_my_putchar( '!', &buf );  /* extras */
      low_my_putchar( 0xf9, &buf ); /* transparency */
      low_my_putchar( 4, &buf );
      low_my_putchar( 1, &buf );
      low_my_putchar( 0, &buf );
      low_my_putchar( 0, &buf );
      low_my_putchar( colortable_rgb(ct,*transparent), &buf );
      low_my_putchar( 0, &buf );
   }


   low_my_putchar( ',', &buf ); /* image separator */

   buf_word(0,&buf); /* leftofs */
   buf_word(0,&buf); /* topofs */
   buf_word(img->xsize,&buf); /* width */
   buf_word(img->ysize,&buf); /* height */

   low_my_putchar(0x00, &buf); 
      /* not interlaced (interlaced == 0x40) */
      /* no local colormap ( == 0x80) */

   low_my_putchar( 8, &buf ); /* bits per pixel , or min 2 */
   
   i=img->xsize*img->ysize;
   rgb=img->img;

   lzw_init(&lzw,8);
   if (!fs)
      while (i--) lzw_add(&lzw,colortable_rgb(ct,*(rgb++)));
   else
   {
      rgbl_group err,*errb;
      rgb_group corgb;
      int w,*cres,j;
      errb=(rgbl_group*)xalloc(sizeof(rgbl_group)*img->xsize);
      cres=(int*)xalloc(sizeof(int)*img->xsize);
      err.r=err.g=err.b=0;
      for (i=0; i<img->xsize; i++)
	errb[i].r=(rand()%(FS_SCALE*2+1))-FS_SCALE,
	errb[i].g=(rand()%(FS_SCALE*2+1))-FS_SCALE,
	errb[i].b=(rand()%(FS_SCALE*2+1))-FS_SCALE;

      w=0;
      i=img->ysize;
      while (i--)
      {
	 floyd_steinberg(rgb,img->xsize,errb,w=!w,cres,ct);
	 for (j=0; j<img->xsize; j++)
	    lzw_add(&lzw,cres[j]);
	 rgb+=img->xsize;
      }

      free(errb);
      free(cres);
   }

   lzw_write_last(&lzw);

   for (i=0; i<(int)lzw.outpos; i+=254)
   {
      int wr;
      if (i+254>(int)lzw.outpos) wr=lzw.outpos-i;
      else wr=254;
      low_my_putchar( (unsigned char)wr, &buf ); /* bytes in chunk */
      low_my_binary_strcat( (char *) lzw.out+i, wr, &buf );
   }
   low_my_putchar( 0, &buf ); /* terminate stream */

   lzw_quit(&lzw);

   low_my_putchar( ';', &buf ); /* end gif file */
   
   return low_free_buf(&buf);
}

