#include "stralloc.h"
#include "global.h"
#include "types.h"
#include "dynamic_buffer.h"

#include "image.h"
#include "lzw.h"

#define INITIAL_BUF_LEN 8192

/** compress algorithm *************/


void buf_word( unsigned short w, dynamic_buffer *buf )
{
   low_my_putchar( w&0xff, buf );
   low_my_putchar( (w>>8)&0xff, buf );
}

struct pike_string *
   image_encode_gif(struct image *img,struct colortable *ct,
		    rgb_group *transparent)
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
   while (i--) lzw_add(&lzw,colortable_rgb(ct,*(rgb++)));

   lzw_write_last(&lzw);

   for (i=0; i<(int)lzw.outpos; i+=254)
   {
      int wr;
      if (i+254>(int)lzw.outpos) wr=lzw.outpos-i;
      else wr=254;
      low_my_putchar( (unsigned char)wr, &buf ); /* bytes in chunk */
      low_my_binary_strcat( lzw.out+i, wr, &buf );
   }
   low_my_putchar( 0, &buf ); /* terminate stream */

   lzw_quit(&lzw);

   low_my_putchar( ';', &buf ); /* end gif file */
   
   return low_free_buf(&buf);
}

