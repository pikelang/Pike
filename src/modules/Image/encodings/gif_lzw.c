/*
**! module Image
**! note
**!	$Id: gif_lzw.c,v 1.3 1997/11/02 03:44:50 mirar Exp $
*/

#include "global.h"
#include "gif_lzw.h"

#define DEFAULT_OUTBYTES 16384
#define STDLZWCODES 8192

static INLINE void lzw_output(struct gif_lzw *lzw,lzwcode_t codeno)
{
   int bits,bitp;
   unsigned char c;

   if (lzw->outpos+4>=lzw->outlen)
   {
      unsigned char *new;
      new=realloc(lzw->out,lzw->outlen*=2);
      if (!new) { lzw->outpos=0; lzw->broken=1; return; }
      lzw->out=new;
   }

   bitp=lzw->outbit;
   c=lzw->lastout;
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

static INLINE void lzw_add(struct gif_lzw *lzw,int c)
{
   lzwcode_t lno,lno2;
   struct gif_lzwc *l;

   if (lzw->current==LZWCNULL) /* no current, load */
   {
      lzw->current=c;
      return;
   }

   lno=lzw->code[lzw->current].firstchild; /* check if we have this sequence */
   while (lno!=LZWCNULL)
   {
      if (lzw->code[lno].c==c && lno!=lzw->codes-1 )
      {
	 lzw->current=lno;
	 return;
      }
      lno=lzw->code[lno].next;
   }

   if (lzw->codes==4096)  /* needs more than 12 bits */
   {
      int i;

      lzw_output(lzw,lzw->current);

      for (i=0; i<(1L<<lzw->bits); i++)
	 lzw->code[i].firstchild=LZWCNULL;
      lzw->codes=(1L<<lzw->bits)+2;
      
      /* output clearcode, 1000... (bits) */
      lzw_output(lzw,1L<<lzw->bits);

      lzw->codebits=lzw->bits+1;
      lzw->current=c;
      return;
   }

   /* output current code no, make new & reset */

   lzw_output(lzw,lzw->current);

   lno=lzw->code[lzw->current].firstchild;
   lno2=lzw->codes;
   l=lzw->code+lno2;
   l->next=lno;
   l->firstchild=LZWCNULL;
   l->c=c;
   lzw->code[lzw->current].firstchild=lno2;

   lzw->codes++;
   if (lzw->codes>(unsigned long)(1L<<lzw->codebits)) lzw->codebits++;

   lzw->current=c;
}

void image_gif_lzw_init(struct gif_lzw *lzw,int bits)
{
   int i;

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
   lzw_output(lzw,1L<<bits);
}

void image_gif_lzw_finish(struct gif_lzw *lzw)
{
   if (lzw->current!=LZWCNULL)
      lzw_output(lzw,lzw->current);
   lzw_output( lzw, (1L<<lzw->bits)+1 ); /* GIF end code */
   if (lzw->outbit)
      lzw->out[lzw->outpos++]=lzw->lastout;
}

void image_gif_lzw_free(struct gif_lzw *lzw)
{
   if (lzw->out) free(lzw->out);
   if (lzw->code) free(lzw->code);
}

void image_gif_lzw_add(struct gif_lzw *lzw,unsigned char *data,int len)
{
   while (len--) lzw_add(lzw,*(data++));
}
