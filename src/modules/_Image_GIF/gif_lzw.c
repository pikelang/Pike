/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/*
**! module Image
*/

#include "global.h"
#include "config.h"

#ifdef WITH_GIF

#include "../Image/image_machine.h"
#include "gif_lzw.h"


#define DEFAULT_OUTBYTES 16384
#define STDLZWCODES 8192

static INLINE void lzw_output(struct gif_lzw *lzw,lzwcode_t codeno)
{
   if (lzw->outpos+4>=lzw->outlen)
   {
      unsigned char *new;
      new=realloc(lzw->out,lzw->outlen*=2);
      if (!new) { lzw->outpos=0; lzw->broken=1; return; }
      lzw->out=new;
   }

   if (!lzw->reversebits)
   {
      int bits,bitp;
      unsigned char c;

      bitp=lzw->outbit;
      c=(unsigned char)lzw->lastout;
      bits=lzw->codebits;
      if (bits>12) bits=12;

      while (bits)
      {
	 c|=(codeno<<bitp);
	 if (bits+bitp>=8)
	 {
	    bits-=8-bitp;
	    codeno>>=8-bitp;
	    bitp=0;
	    lzw->out[lzw->outpos++]=c;
	    c=0;
	 }
	 else
	 {
	    lzw->outbit=bitp+bits;
	    lzw->lastout=c;
	    return;
	 }
      }
      lzw->lastout=0;
      lzw->outbit=0;
   }
   else
   {
#ifdef GIF_DEBUG
      fprintf(stderr,"codeno=%x lastout=%x outbit=%d codebits=%d\n",
	      codeno,lzw->lastout,lzw->outbit,lzw->codebits);
#endif
      lzw->lastout=(lzw->lastout<<lzw->codebits)|codeno;
      lzw->outbit+=lzw->codebits;
#ifdef GIF_DEBUG
      fprintf(stderr,"-> codeno=%x lastout=%x outbit=%d codebits=%d\n",
	      codeno,lzw->lastout,lzw->outbit,lzw->codebits);
#endif
      while (lzw->outbit>8)
      {
	 lzw->out[lzw->outpos++] =
	    (unsigned char)(lzw->lastout>>(lzw->outbit-8));
	 lzw->outbit-=8;
#ifdef GIF_DEBUG
      fprintf(stderr,"(shiftout) codeno=%x lastout=%x outbit=%d codebits=%d\n",
	      codeno,lzw->lastout,lzw->outbit,lzw->codebits);
#endif
      }
   }
}

static INLINE void lzw_add(struct gif_lzw *lzw,int c)
{
   lzwcode_t lno,lno2;
   struct gif_lzwc *l;

   if (lzw->current==LZWCNULL) /* no current, load */
   {
      lzw->current=c;
#ifdef GIF_LZW_LZ
      lzw->skipone=0;
#endif
      return;
   }

#ifdef GIF_LZW_LZ
   if (!lzw->skipone)
   {
#endif
     /* check if we have this sequence */
     lno=lzw->code[lzw->current].firstchild;
     while (lno!=LZWCNULL)
     {
       if (lzw->code[lno].c==c && lno!=lzw->codes-1 )
       {
	 lzw->current=lno;
	 return;
       }
       lno=lzw->code[lno].next;
     }
#ifdef GIF_LZW_LZ
   }
#endif

   if (lzw->codes==4096)  /* needs more than 12 bits */
   {
      int i;

      lzw_output(lzw,lzw->current);

      for (i=0; i<(1L<<lzw->bits); i++)
	 lzw->code[i].firstchild=LZWCNULL;
      lzw->codes=(1L<<lzw->bits)+2;
      
      /* output clearcode, 1000... (bits) */
      lzw_output(lzw, (lzwcode_t)(1L<<lzw->bits));

      lzw->codebits=lzw->bits+1;
      lzw->current=c;
#ifdef GIF_LZW_LZ
      lzw->skipone=0;
#endif
      return;
   }

   /* output current code no, make new & reset */

   lzw_output(lzw,lzw->current);

   lno=lzw->code[lzw->current].firstchild;
   lno2 = DO_NOT_WARN((lzwcode_t)lzw->codes);
   l=lzw->code+lno2;
   l->next=lno;
   l->firstchild=LZWCNULL;
   l->c=c;
   lzw->code[lzw->current].firstchild=lno2;

   lzw->codes++;
   if (lzw->codes+lzw->earlychange>(unsigned long)(1L<<lzw->codebits)) 
      lzw->codebits++;

   lzw->current=c;
#ifdef GIF_LZW_LZ
   lzw->skipone=!lzw->skipone;
#endif
}

void image_gif_lzw_init(struct gif_lzw *lzw,int bits)
{
   unsigned long i;

   lzw->broken=0;

   lzw->codes=(1L<<bits)+2;
   lzw->bits=bits;
   lzw->codebits=bits+1;
   lzw->code=(struct gif_lzwc*) malloc(sizeof(struct gif_lzwc)*4096);

   if (!lzw->code) { lzw->broken=1; return; }

   for (i=0; i<lzw->codes; i++)
   {
      lzw->code[i].c=(unsigned char)i;
      lzw->code[i].firstchild=LZWCNULL;
      lzw->code[i].next=LZWCNULL;
   }
   lzw->out=malloc(DEFAULT_OUTBYTES);
   if (!lzw->out) { lzw->broken=1; return; }
   lzw->outlen=DEFAULT_OUTBYTES;
   lzw->outpos=0;
   lzw->current=LZWCNULL;
   lzw->outbit=0;
   lzw->lastout=0;
   lzw->earlychange=0;
   lzw->reversebits=0;
   lzw_output(lzw, (lzwcode_t)(1L<<bits));
}

void image_gif_lzw_finish(struct gif_lzw *lzw)
{
   if (lzw->current!=LZWCNULL)
      lzw_output(lzw,lzw->current);
   lzw_output( lzw, (lzwcode_t)(1L<<lzw->bits)+1 ); /* GIF end code */
   if (lzw->outbit)
   {
      if (lzw->reversebits)
	 lzw->out[lzw->outpos++] = DO_NOT_WARN((lzwcode_t)(lzw->lastout<<
							   (8-lzw->outbit)));
      else
	 lzw->out[lzw->outpos++] = DO_NOT_WARN((lzwcode_t)lzw->lastout);
   }
}

void image_gif_lzw_free(struct gif_lzw *lzw)
{
   if (lzw->out) free(lzw->out);
   if (lzw->code) free(lzw->code);
}

void image_gif_lzw_add(struct gif_lzw *lzw, unsigned char *data, size_t len)
{
   while (len--) lzw_add(lzw,*(data++));
}

#endif /* WITH_GIF */
