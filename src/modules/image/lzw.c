/* $Id: lzw.c,v 1.7 1996/11/14 12:35:00 law Exp $ */

/*

lzw, used by togif

Pontus Hagland, law@infovav.se

this file can be used generally for lzw algoritms,
the existanse of #define GIF_LZW is for that purpose. :-)
/Pontus

*/

#include "global.h"

#include "lzw.h"

#define DEFAULT_OUTBYTES 16384
#ifndef GIF_LZW
#define STDLZWCODES 8192
#endif

static void lzw_output(struct lzw *lzw,lzwcode_t codeno);

void lzw_init(struct lzw *lzw,int bits)
{
   unsigned long i;
#ifdef GIF_LZW
   lzw->codes=(1L<<bits)+2;
#else
   lzw->codes=(1L<<bits);
#endif
   lzw->bits=bits;
   lzw->codebits=bits+1;
   lzw->code=(struct lzwc*) malloc(sizeof(struct lzwc)*
#ifdef GIF_LZW
				   4096
#else
                                   (lzw->alloced=STDLZWCODES)
#endif
				   );

   for (i=0; i<lzw->codes; i++)
   {
      lzw->code[i].c=(unsigned char)i;
      lzw->code[i].firstchild=LZWCNULL;
      lzw->code[i].next=LZWCNULL;
   }
   lzw->out=malloc(DEFAULT_OUTBYTES);
   lzw->outlen=DEFAULT_OUTBYTES;
   lzw->outpos=0;
   lzw->current=LZWCNULL;
   lzw->outbit=0;
   lzw->lastout=0;
#ifdef GIF_LZW
   lzw_output(lzw,1L<<bits);
#endif
}

void lzw_quit(struct lzw *lzw)
{
   int i;

   free(lzw->code);
   free(lzw->out);
}

#if 0
static void lzw_recurse_find_code(struct lzw *lzw,lzwcode_t codeno)
{
   lzwcode_t i;
   if (codeno<(1L<<lzw->bits))
      fprintf(stderr,"%c",codeno);
   else if (codeno==(1L<<lzw->bits))
      fprintf(stderr,"(clear)");
   else if (codeno==(1L<<lzw->bits)+1)
      fprintf(stderr,"(end)");
   else for (;;)
   {
      /* direct child? */
      for (i=0; i<lzw->codes; i++)
	 if (lzw->code[i].firstchild==codeno)
	 {
	    lzw_recurse_find_code(lzw,i);
	    fprintf(stderr,"%c",lzw->code[codeno].c);
	    return;
	 }
      for (i=0; i<lzw->codes; i++)
	 if (lzw->code[i].next==codeno) codeno=i;
   }
}
#endif

static INLINE void lzw_output(struct lzw *lzw,lzwcode_t codeno)
{
   int bits,bitp;
   unsigned char c;

/*
   fprintf(stderr,"%03x bits=%d codes %d %c\n",
           codeno,lzw->codebits,lzw->codes+1,
	   (codeno==lzw->codes+1) ? '=' : ' ');
	   */
#if 0
   fprintf(stderr,"\nwrote %4d<'",codeno);
   lzw_recurse_find_code(lzw,codeno);
   fprintf(stderr,"' ");
#endif

   if (lzw->outpos+4>=lzw->outlen)
      lzw->out=realloc(lzw->out,lzw->outlen*=2);

   bitp=lzw->outbit;
   c=lzw->lastout;
   bits=lzw->codebits;
#ifdef GIF_LZW
   if (bits>12) bits=12;
#endif

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

void lzw_write_last(struct lzw *lzw)
{
   if (lzw->current)
      lzw_output(lzw,lzw->current);
#ifdef GIF_LZW
   lzw_output( lzw, (1L<<lzw->bits)+1 ); /* GIF end code */
#endif
   if (lzw->outbit)
      lzw->out[lzw->outpos++]=lzw->lastout;
}

void lzw_add(struct lzw *lzw,int c)
{
   lzwcode_t lno,lno2;
   struct lzwc *l;

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


#ifdef GIF_LZW
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
#else
   if (lzw->codes==MAXLZWCODES)
      realloc lzwc->code... move lzw->current
#endif

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

#ifdef GIF_LZW
unsigned long lzw_unpack(unsigned char *dest,unsigned long destlen,
			 unsigned char *src,unsigned long srclen,
			 int bits)
{
   struct lzwuc
   {
      unsigned char c;
      lzwcode_t parent;
      lzwcode_t len; /* no string is longer then that ... */
   } *code;

   static unsigned short mask[16]={0,1,3,7,017,037,077,0177,0377,0777,01777,
				   03777,07777,01777,03777,07777};

   unsigned long wrote=0;
   int i,cbits,cbit,clear=(1<<bits),end=(1<<bits)+1;
   lzwcode_t current,last,used;
   unsigned long store;
   unsigned char *srcend=src+srclen,*destend=dest+destlen;

   code=malloc(sizeof(struct lzwuc)*4096);
   if (!code) return 0;

   for (i=0; i<(1<<bits); i++)
   {
      code[i].c=i;
      code[i].parent=LZWCNULL;
      code[i].len=1;
   }

   cbit=0;
   cbits=bits+1;
   store=0;
   last=LZWCNULL;
   used=end+1;

   while (src!=srcend)
   {
      if (cbit>cbits)
      {
	 current=store>>(32-cbits);
	 cbit-=cbits;
      }
      else 
      {
	 while (cbits-cbit>=8)
	 {
	    store=(store>>8)|((*(src++))<<24);
	    cbit+=8;
	    if (src==srcend) return wrote;
	 }
	 store=(store>>8)|((*(src++))<<24);
	 cbit+=8;
	 current=(store>>(32-(cbit)))&mask[cbits];
	 cbit-=cbits;
      }
      
      if (current==clear) /* clear tree */
      {
	 for (i=0; i<end-2; i++)
	    code[i].parent=LZWCNULL;
	 last=LZWCNULL;
	 used=end+1;
	 if (cbits!=bits+1)
	 {
	    cbits=bits+1;
	    cbit++;
	 }
      }
      else if (current==end) /* end of data */
	 break;
      else
      {
	 lzwcode_t n;
	 unsigned char *dest2;

	 if (current>=used /* wrong code, cancel */
	    || code[current].len+dest>destend) /* no space, cancel */
	    break; 
	 
	 dest+=code[current].len;
	 wrote+=code[current].len;

	 dest2=dest;
	 n=current;
	 *--dest2=code[n].c;
	 while (n>end)
	 {
	    n=code[n].parent;
	    *--dest2=code[n].c;
	 }


/*
	 *dest=0;
	 fprintf(stderr,"read %4d>'",current);
	 for (i=0; i<code[current].len; i++)
	    fprintf(stderr,"%c",(dest-code[current].len)[i]);
	 fprintf(stderr,"' == ");
	 for (i=0; i<code[current].len; i++)
	    fprintf(stderr,"%02x",(dest-code[current].len)[i]);
	 fprintf(stderr,"\n");
*/

	 if (last!=LZWCNULL)
	 {
	    code[used].c=code[n].c;
	    code[used].parent=last;
	    code[used].len=code[last].len+1;
	    used++;
	    if (used>=(1<<cbits)) cbits++;
	 }
	 
	 last=current;
      }
   }
   return wrote;
}
#endif

