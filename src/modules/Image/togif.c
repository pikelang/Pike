/*

togif 

Pontus Hagland, law@infovav.se

$Id: togif.c,v 1.14 1997/05/29 17:03:27 mirar Exp $ 

*/

/*
**! module Image
**! class image
*/

#include "global.h"

#include <math.h>
#include <ctype.h>

#include "stralloc.h"
#include "global.h"
#include "threads.h"
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "array.h"
#include "error.h"
#include "dynamic_buffer.h"
#include "operators.h"

#include "image.h"
#include "lzw.h"

#define INITIAL_BUF_LEN 8192

#define THIS ((struct image *)(fp->current_storage))
#define THISOBJ (fp->current_object)

#if 0
#include <sys/resource.h>
#define CHRONO(X) chrono(X)

static void chrono(char *x)
{
   struct rusage r;
   static struct rusage rold;
   getrusage(RUSAGE_SELF,&r);
   fprintf(stderr,"%s: %ld.%06ld - %ld.%06ld\n",x,
	   r.ru_utime.tv_sec,r.ru_utime.tv_usec,

	   ((r.ru_utime.tv_usec-rold.ru_utime.tv_usec<0)?-1:0)
	   +r.ru_utime.tv_sec-rold.ru_utime.tv_sec,
           ((r.ru_utime.tv_usec-rold.ru_utime.tv_usec<0)?1000000:0)
           + r.ru_utime.tv_usec-rold.ru_utime.tv_usec
	   );

   rold=r;
}
#else
#define CHRONO(X)
#endif


#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)<(b)?(b):(a))
#define testrange(x) max(min((x),255),0)

static void buf_word( unsigned short w, dynamic_buffer *buf )
{
   low_my_putchar( w&0xff, buf );
   low_my_putchar( (w>>8)&0xff, buf );
}

#define WEIGHT_NEXT(X)     (((X)*8)/20)
#define WEIGHT_DOWNNEXT(X) (((X)*3)/20)
#define WEIGHT_DOWN(X)     (((X)*3)/20)
#define WEIGHT_DOWNBACK(X) (((X)*0)/20)

static int floyd_steinberg_add(rgbl_group *errl,
			       rgbl_group *errlfwd,
			       rgbl_group *errlback,
			       rgbl_group *err,
			       rgb_group rgb,
			       struct colortable *ct,
			       int closest)
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
   if (closest)
      c=colortable_rgb_nearest(ct,rgb2);
   else
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

void image_floyd_steinberg(rgb_group *rgb,int xsize,
			   rgbl_group *errl,
			   int way,int *res,
			   struct colortable *ct,
			   int closest)
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
				    &err,rgb[x],ct,closest);
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
				    &err,rgb[x],ct,closest);
   }
}
		     
#define STD_ARENA_SIZE 16384

int image_decode_gif(struct image *dest,struct image *dest_alpha,
		     unsigned char *src,unsigned long len)
{
   unsigned char *arena,*tmpstore,*q;
   rgb_group *global_palette,*palette;
   rgb_group *rgb;
   int bpp;
   unsigned long i,j;
   INT32 mod;
   INT32 leftofs,topofs,width,height;
   int interlaced,transparent;
   unsigned long arenalen,pos;

   if (src[0]!='G'
       ||src[1]!='I'
       ||src[2]!='F'
       ||len<12) 
      return 0; /* not a gif, you fool */

   dest->xsize=src[6]+(src[7]<<8);
   dest->ysize=src[8]+(src[9]<<8);

   if (! (arena=malloc(arenalen=STD_ARENA_SIZE)) ) return 0;

   dest->img=malloc(dest->xsize*dest->ysize*sizeof(rgb_group));
   if (!dest->img) { free(arena); return 0; }
      /* out of memory (probably illegal size) */

   bpp=(src[10]&7)+1;

   THREADS_ALLOW();
   if (src[10]&128)
   {
      global_palette=(rgb_group*)(src+13);
      src+=3*(1<<bpp);
      len-=3*(1<<bpp);
      rgb=dest->img;
      i=dest->xsize*dest->ysize;
/*
      while (i--)
	 *rgb=global_palette[src[11]]; * paint with background color */

   }
   else 
      global_palette=NULL;
   MEMSET(dest->img,0,sizeof(rgb_group)*dest->xsize*dest->ysize);
   src+=13; len-=13;
   
   do
   {
      switch (*src)
      {
	 case '!': /* function block */
	    if (len<7) break; /* no len left */
	    if (src[1]==0xf9) 
	       transparent=src[6];
	    len-=src[3]+1;
	    src+=src[3]+1;
	    continue;
 	 case ',': /* image block(s) */
	    if (len<10) len-=10; /* no len left */
	    leftofs=src[1]+(src[2]<<8);
	    topofs=src[3]+(src[4]<<8);
	    width=src[5]+(src[6]<<8);
	    height=src[7]+(src[8]<<8);
	    interlaced=src[9]&64;
	    
	    if (src[9]&128)
	    {
	       palette=(rgb_group*)(src+10);
	       src+=((src[9]&7)+1)*3;
	    }
	    else
	       palette=global_palette;

	    src+=11;
	    len-=11;
	    pos=0;
	    if (len<3) break; /* no len left */
	    bpp=src[-1];

	    while (len>1)
	    {
	       if (!(i=*src)) break; /* length of block */
	       if (pos+i>arenalen)
	       {
		  arena=realloc(arena,arenalen*=2);
		  if (!arena) return 1;
	       }
	       MEMCPY(arena+pos,src+1,min(i,len-1));
	       pos+=min(i,len-1);
	       len-=i+1;
	       src+=i+1;
	    }

	    if (leftofs+width<=dest->xsize && topofs+height<=dest->ysize)
	    {
	       tmpstore=malloc(width*height);
	       if (!tmpstore) break;
	       i=lzw_unpack(tmpstore,width*height,arena,pos,bpp);
	       if (i!=(unsigned long)(width*height))
		  MEMSET(tmpstore+i,0,width*height-i);
	       rgb=dest->img+leftofs+topofs*dest->ysize;
	       mod=width-dest->xsize;
	       j=height;
	       q=tmpstore;
	       if (palette)
		  while (j--)
		  {
		     i=width;
		     while (i--) *(rgb++)=palette[*(q++)];
		     rgb+=mod;
		  }
	       free(tmpstore);
	    }
	    
	    continue;
	 case ';':
	    
	    break; /* file done */
	 default:
	    break; /* unknown, file is ok? */
      }
   }
   while (0);
   THREADS_DISALLOW();

   if (arena) free(arena);
   return 1; /* ok */
}



/*
**! method object fromgif(string gif)
**!	Reads GIF data to the called image object.
**!
**!	GIF animation delay or loops are ignored,
**!	and the resulting image is the written result.
**! returns the called object
**! arg string pnm
**!	pnm data, as a string
**! see also: togif, frompnm
**! bugs
**!	yes, it does -- it may even do segment overrides...
*/

void image_fromgif(INT32 args)
{
   if (sp[-args].type!=T_STRING)
      error("Illegal argument to image->fromgif()\n");

   if (THIS->img) free(THIS->img);
   THIS->img=NULL;

   image_decode_gif(THIS,NULL,
		    (unsigned char*)sp[-args].u.string->str,
		    sp[-args].u.string->len);

   pop_n_elems(args);
   THISOBJ->refs++;
   push_object(THISOBJ);
}

static INLINE void getrgb(struct image *img,
			  INT32 args_start,INT32 args,char *name)
{
   INT32 i;
   if (args-args_start<3) return;
   for (i=0; i<3; i++)
      if (sp[-args+i+args_start].type!=T_INT)
         error("Illegal r,g,b argument to %s\n",name);
   img->rgb.r=(unsigned char)sp[-args+args_start].u.integer;
   img->rgb.g=(unsigned char)sp[1-args+args_start].u.integer;
   img->rgb.b=(unsigned char)sp[2-args+args_start].u.integer;
   if (args-args_start>=4)
      if (sp[3-args+args_start].type!=T_INT)
         error("Illegal alpha argument to %s\n",name);
      else
         img->alpha=sp[3-args+args_start].u.integer;
   else
      img->alpha=0;
}


/*
**! method string gif_begin()
**! method string gif_begin(int num_colors)
**! method string gif_begin(array(array(int)) colors)
**!	Makes GIF header. With no argument, there is no
**!	global colortable (palette).
**! returns the GIF data
**!
**! arg int num_colors
**!	number of colors to quantize to (default is 256) 
**! array array(array(int)) colors
**!	colors to map to, format is ({({r,g,b}),({r,g,b}),...}).
**! see also: gif_add, gif_end, togif, gif_netscape_loop
**!
**!
**! method string gif_end()
**!	Ends GIF data.
**! returns the GIF data.
**! see also: gif_begin
*/

void image_gif_begin(INT32 args)
{
   dynamic_buffer buf;
   long i;
   int colors,bpp;
   struct colortable *ct=NULL;

   if (args)
      if (sp[-args].type==T_INT && sp[-args].u.integer!=0)
	 ct=colortable_quant(THIS,max(256,min(2,sp[-args].u.integer)));
      else if (sp[-args].type==T_ARRAY)
	 ct=colortable_from_array(sp[-args].u.array,"image->gif_begin()\n");

   pop_n_elems(args);

   buf.s.str=NULL;
   initialize_buf(&buf);

   if (ct)
   {
      colors=4; bpp=2;
      while (colors<ct->numcol) { colors<<=1; bpp++; }
   }
   else 
   {
      colors=256; bpp=8;
   }

   low_my_binary_strcat("GIF89a",6,&buf);
   buf_word((unsigned short)THIS->xsize,&buf);
   buf_word((unsigned short)THIS->ysize,&buf);
   low_my_putchar( (char)((0x80*!!ct) | 0x70 | (bpp-1) ), &buf);
   /* | global colormap | 3 bits color res | sort | 3 bits bpp */
   /* color res is'nt cared of */

   low_my_putchar( 0, &buf ); /* background color */
   low_my_putchar( 0, &buf ); /* just zero */

   if (!!ct)
   {
      for (i=0; i<ct->numcol; i++)
      {
	 low_my_putchar(ct->clut[i].r,&buf);
	 low_my_putchar(ct->clut[i].g,&buf);
	 low_my_putchar(ct->clut[i].b,&buf);
      }

      for (; i<colors; i++)
      {
	 low_my_putchar(0,&buf);
	 low_my_putchar(0,&buf);
	 low_my_putchar(0,&buf);
      }
   }
   push_string(low_free_buf(&buf));
}

void image_gif_end(INT32 args)
{
   pop_n_elems(args);
   push_string(make_shared_binary_string(";",1));
}

/*
**! method string gif_netscape_loop()
**! method string gif_netscape_loop(int loops)
**!	
**! returns a gif chunk that defines that the GIF animation should loop
**!
**! arg int loops
**!	number of loops, default is 65535.
**! see also: gif_add, gif_begin, gif_end
*/

void image_gif_netscape_loop(INT32 args)
{
   unsigned short loops=0;
   char buf[30];
   if (args)
      if (sp[-args].type!=T_INT) 
	 error("Illegal argument to image->gif_netscape_loop()\n");
      else
	 loops=sp[-args].u.integer;
   else
      loops=65535;
   pop_n_elems(args);

   sprintf(buf,"%c%c%cNETSCAPE2.0%c%c%c%c%c",
	   33,255,11,3,1,loops&255,(loops>>8)&255,0);

   push_string(make_shared_binary_string(buf,19));
}

/*
**! method string gif_transparency(int color)
**!	
**! returns a gif chunk that transparent a color in the next image chunk
**!
**! arg int color
**!	index of color in the palette 
**! note
**!	Yes - i know this function is too hard to use. :/
**!	The palette _is_ unknown mostly...
**! see also: gif_add, gif_begin, gif_end
*/

void image_gif_transparency(INT32 args)
{
   unsigned short i=0;
   char buf[30];
   if (args)
      if (sp[-args].type!=T_INT) 
	 error("Illegal argument to image->gif_transparency()\n");
      else
	 i=sp[-args].u.integer;
   else
      error("Too few arguments to image->gif_transparency()\n");
   pop_n_elems(args);

   sprintf(buf,"%c%c%c%c%c%c%c%c",33,0xf9,4,1,0,0,i,0);

   push_string(make_shared_binary_string(buf,8));
}

static struct colortable *img_gif_add(INT32 args,int fs,int lm)
{
   INT32 x=0,y=0,i;
   struct lzw lzw;
   rgb_group *rgb;
   struct colortable *ct=NULL;
   dynamic_buffer buf;
   int colors,bpp;
   int closest=0;

CHRONO("gif add init");

   buf.s.str=NULL;
   initialize_buf(&buf);

   if (args==0) x=y=0;
   else if (args<2
	    || sp[-args].type!=T_INT
	    || sp[1-args].type!=T_INT)
      error("Illegal argument(s) to image->gif_add()\n");
   else 
   {
      x=sp[-args].u.integer;
      y=sp[1-args].u.integer;
   }


   if (args>2 && sp[2-args].type==T_ARRAY)
   {
      ct=colortable_from_array(sp[2-args].u.array,"image->gif_add()\n");
      closest=1;
   }
   else if (args>3 && sp[2-args].type==T_INT)
      ct=colortable_quant(THIS,max(256,min(2,sp[2-args].u.integer)));

   if (args>2+!!ct)
   {
      unsigned short delay=0;
      if (sp[2+!!ct-args].type==T_INT) 
	 delay=sp[2+!!ct-args].u.integer;
      else if (sp[2+!!ct-args].type==T_FLOAT) 
	 delay=(unsigned short)(sp[2+!!ct-args].u.float_number*100);
      else 
	 error("Illegal argument %d to image->gif_add()\n",3+!!ct);

      low_my_putchar( '!', &buf ); /* extension block */
      low_my_putchar( 0xf9, &buf ); /* graphics control */
      low_my_putchar( 4, &buf ); /* block size */
      low_my_putchar( 0, &buf ); /* disposal, transparency, blabla */
      buf_word( delay, &buf ); /* delay in centiseconds */
      low_my_putchar( 0, &buf ); /* (transparency index) */
      low_my_putchar( 0, &buf ); /* terminate block */
   }

   if(!ct)
     ct=colortable_quant(THIS,256);

   colors=4; bpp=2;
   while (colors<ct->numcol) { colors<<=1; bpp++; }


   low_my_putchar( ',', &buf ); /* image separator */

   buf_word(x,&buf); /* leftofs */
   buf_word(y,&buf); /* topofs */
   buf_word(THIS->xsize,&buf); /* width */
   buf_word(THIS->ysize,&buf); /* height */

   low_my_putchar((0x80*lm)|(bpp-1), &buf); 
      /* not interlaced (interlaced == 0x40) */
      /* local colormap ( == 0x80) */
      /* 8 bpp in map ( == 0x07)   */

   if (lm)
   {
      for (i=0; i<ct->numcol; i++)
      {
	 low_my_putchar(ct->clut[i].r,&buf);
	 low_my_putchar(ct->clut[i].g,&buf);
	 low_my_putchar(ct->clut[i].b,&buf);
      }
      for (; i<colors; i++)
      {
	 low_my_putchar(0,&buf);
	 low_my_putchar(0,&buf);
	 low_my_putchar(0,&buf);
      }
   }

   low_my_putchar( bpp, &buf ); 
   
   i=THIS->xsize*THIS->ysize;
   rgb=THIS->img;

CHRONO("begin pack");

   THREADS_ALLOW();
   lzw_init(&lzw,bpp);
   if (!fs)
      while (i--) lzw_add(&lzw,colortable_rgb(ct,*(rgb++)));
   else
   {
      rgbl_group *errb;
      rgb_group corgb;
      int w,*cres,j;
      errb=(rgbl_group*)xalloc(sizeof(rgbl_group)*THIS->xsize);
      cres=(int*)xalloc(sizeof(int)*THIS->xsize);
      for (i=0; i<THIS->xsize; i++)
	errb[i].r=(rand()%(FS_SCALE*2+1))-FS_SCALE,
	errb[i].g=(rand()%(FS_SCALE*2+1))-FS_SCALE,
	errb[i].b=(rand()%(FS_SCALE*2+1))-FS_SCALE;

      w=0;
      i=THIS->ysize;
      while (i--)
      {
	 image_floyd_steinberg(rgb,THIS->xsize,errb,w=!w,cres,ct,closest);
	 for (j=0; j<THIS->xsize; j++)
	    lzw_add(&lzw,cres[j]);
	 rgb+=THIS->xsize;
      }

      free(errb);
      free(cres);
   }

   lzw_write_last(&lzw);

CHRONO("end pack");

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

   THREADS_DISALLOW();

CHRONO("done");

   pop_n_elems(args);
   push_string(low_free_buf(&buf));

   return ct;
}

/*
**! method string gif_add()
**! method string gif_add(int x,int y)
**! method string gif_add(int x,int y,int delay_cs)
**! method string gif_add(int x,int y,float delay_s)
**! method string gif_add(int x,int y,int num_colors,int delay_cs)
**! method string gif_add(int x,int y,int num_colors,float delay_s)
**! method string gif_add(int x,int y,array(array(int)) colors,int delay_cs)
**! method string gif_add(int x,int y,array(array(int)) colors,float delay_s)
**! method string gif_add_fs()
**! method string gif_add_fs(int x,int y)
**! method string gif_add_fs(int x,int y,int delay_cs)
**! method string gif_add_fs(int x,int y,float delay_s)
**! method string gif_add_fs(int x,int y,int num_colors,int delay_cs)
**! method string gif_add_fs(int x,int y,int num_colors,float delay_s)
**! method string gif_add_fs(int x,int y,array(array(int)) colors,int delay_cs)
**! method string gif_add_fs(int x,int y,array(array(int)) colors,float delay_s)
**! method string gif_add_nomap()
**! method string gif_add_nomap(int x,int y)
**! method string gif_add_nomap(int x,int y,int delay_cs)
**! method string gif_add_nomap(int x,int y,float delay_s)
**! method string gif_add_nomap(int x,int y,int num_colors,int delay_cs)
**! method string gif_add_nomap(int x,int y,int num_colors,float delay_s)
**! method string gif_add_nomap(int x,int y,array(array(int)) colors,int delay_cs)
**! method string gif_add_nomap(int x,int y,array(array(int)) colors,float delay_s)
**! method string gif_add_fs_nomap()
**! method string gif_add_fs_nomap(int x,int y)
**! method string gif_add_fs_nomap(int x,int y,int delay_cs)
**! method string gif_add_fs_nomap(int x,int y,float delay_s)
**! method string gif_add_fs_nomap(int x,int y,int num_colors,int delay_cs)
**! method string gif_add_fs_nomap(int x,int y,int num_colors,float delay_s)
**! method string gif_add_fs_nomap(int x,int y,array(array(int)) colors,int delay_cs)
**! method string gif_add_fs_nomap(int x,int y,array(array(int)) colors,float delay_s)
**!	Makes a GIF (sub)image data chunk, to be placed 
**!	at the given position. 
**!
**!     The "fs" versions uses floyd-steinberg dithering, and the "nomap"
**!     versions have no local colormap.
**!
**!	Example: 
**!	<pre>
**!	object img1 = Image(200,200); 
**!	object img2 = Image(200,200); 
**!     // load img1 and img2 with stuff
**!     write(img1->gif_begin()+
**!	      img1->gif_netscape_loop()+
**!	      img1->gif_add(0,0,100)+
**!	      img2->gif_add(0,0,100)+
**!	      img1->gif_end());
**!	// voila, a gif animation...
**!     </pre>
**!
**! note
**!	I (Mirar) recommend reading about the GIF file format before 
**!	experementing with these. 
**! returns the GIF data chunk as a string
**!
**! arg int x
**! arg int y
**!	the location of this subimage
**! arg int delay_cs
**!     frame delay in centiseconds 
**! arg float delay_s
**!     frame delay in seconds
**! arg int num_colors
**!	number of colors to quantize to (default is 256) 
**! arg array array(array(int)) colors
**!	colors to map to, format is ({({r,g,b}),({r,g,b}),...}).
**! see also: gif_add, gif_end, gif_netscape_loop, togif */

void image_gif_add(INT32 args)
{
   colortable_free(img_gif_add(args,0,1));
}

void image_gif_add_fs(INT32 args)
{
   colortable_free(img_gif_add(args,1,1));
}

void image_gif_add_nomap(INT32 args)
{
   colortable_free(img_gif_add(args,0,0));
}

void image_gif_add_fs_nomap(INT32 args)
{
   colortable_free(img_gif_add(args,1,0));
}

/*
**! method string togif()
**! method string togif(int num_colors)
**! method string togif(array(array(int)) colors)
**! method string togif(int trans_r,int trans_g,int trans_b)
**! method string togif(int num_colors,int trans_r,int trans_g,int trans_b)
**! method string togif(array(array(int)) colors,int trans_r,int trans_g,int trans_b)
**! method string togif_fs()
**! method string togif_fs(int num_colors)
**! method string togif_fs(array(array(int)) colors)
**! method string togif_fs(int trans_r,int trans_g,int trans_b)
**! method string togif_fs(int num_colors,int trans_r,int trans_g,int trans_b)
**! method string togif_fs(array(array(int)) colors,int trans_r,int trans_g,int trans_b)
**!	Makes GIF data. The togif_fs variant uses floyd-steinberg 
**!	dithereing.
**! returns the GIF data
**!
**! arg int num_colors
**!	number of colors to quantize to (default is 256) 
**! array array(array(int)) colors
**!	colors to map to (default is to quantize to 256), format is ({({r,g,b}),({r,g,b}),...}).
**! arg int trans_r
**! arg int trans_g
**! arg int trans_b
**!	one color, that is to be transparent.
**! see also: togif_begin, togif_add, togif_end, toppm, fromgif
*/


static void img_encode_gif(rgb_group *transparent,int fs,INT32 args)
{
  struct colortable *ct;
  struct svalue sv;

  /* on stack is now:
     - eventual colortable instruction (number or array) */

  /* (swap in) arguments to gif_add, x and y position */
  push_int(0); if (args) { sv=sp[-1]; sp[-1]=sp[-2]; sp[-2]=sv; }
  push_int(0); if (args) { sv=sp[-1]; sp[-1]=sp[-2]; sp[-2]=sv; }

  ct=img_gif_add(args+2,fs,1);
  image_gif_begin(0);

  /* on stack is now:
     - gif image chunk with local palette 
     - gif beginning */

  /* swap them... */
  sv=sp[-1]; sp[-1]=sp[-2]; sp[-2]=sv;

  if (transparent)
  {
     push_int(colortable_rgb(ct,*transparent));
     image_gif_transparency(1);
     sv=sp[-1]; sp[-1]=sp[-2]; sp[-2]=sv;
  }
  colortable_free(ct);
  image_gif_end(0);

  /* on stack is now: 
     - gif beginning
     - eventual transparency chunk
     - image with local palette
     - gif end chunk */
  
  f_add(3+!!transparent);
/*  f_aggregate(4);*/
}

void image_togif(INT32 args)
{
   rgb_group *transparent=NULL;

   if (args>=3)
   {
      getrgb(THIS,args>3,args,"image->togif() (transparency)");
      transparent=&(THIS->rgb);
   }
   if (args==3) pop_n_elems(3);
   else if (args) pop_n_elems(args-1);

   if (!THIS->img) { error("no image\n");  return; }

   img_encode_gif(transparent, 0, args&&args!=3);
}


void image_togif_fs(INT32 args)
{
   rgb_group *transparent=NULL;

   if (args>=3)
   {
      getrgb(THIS,args>3,args,"image->togif() (transparency)");
      transparent=&(THIS->rgb);
   }
   if (args==3) pop_n_elems(3);
   else if (args) pop_n_elems(args-1);

   if (!THIS->img) { error("no image\n");  return; }

   img_encode_gif(transparent, 1, args&&args!=3);
}
