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

static void lzw_output(struct lzw *lzw,lzwcode_t codeno);

void lzw_init(struct lzw *lzw,int bits)
{
   int i;
#ifdef GIF_LZW
   lzw->codes=(1L<<bits)+2;
#else
   lzw->codes=(1L<<bits);
#endif
   lzw->bits=bits;
   lzw->codebits=bits+1;
   lzw->code=(struct lzwc*) malloc(sizeof(struct lzwc)*lzw->codes);

   for (i=0; i<lzw->codes; i++)
   {
      lzw->code[i].c=(unsigned char)i;
      lzw->code[i].firstchild=NULL;
      lzw->code[i].next=NULL;
      lzw->code[i].no=i;
   }
   lzw->out=malloc(DEFAULT_OUTBYTES);
   lzw->outlen=DEFAULT_OUTBYTES;
   lzw->outpos=0;
   lzw->current=NULL;
   lzw->outbit=0;
   lzw->lastout=0;
#ifdef GIF_LZW
   lzw_output(lzw,1L<<bits);
#endif
}

static void lzw_free_lzwc_tree(struct lzwc *lzwc)
{
   struct lzwc *l;
   while (lzwc)
   {
      l=lzwc->next;
      if (lzwc->firstchild) lzw_free_lzwc_tree(lzwc->firstchild);
      free(lzwc);
      lzwc=l;
   }
}

void lzw_quit(struct lzw *lzw)
{
   int i;

   for (i=0; i<(1L<<lzw->bits); i++)
   {
      lzw_free_lzwc_tree(lzw->code[i].firstchild);
      lzw->code[i].firstchild=NULL;
   }
   free(lzw->code);
   free(lzw->out);
}

static void lzw_output(struct lzw *lzw,lzwcode_t codeno)
{
   int bits,bitp;
   unsigned char c;

/*
   fprintf(stderr,"%03x bits=%d codes %d %c\n",codeno,lzw->codebits,lzw->codes+1,
	   (codeno==lzw->codes+1) ? '=' : ' ');
	   */

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
      lzw_output(lzw,lzw->current->no);
#ifdef GIF_LZW
   lzw_output( lzw, (1L<<lzw->bits)+1 ); /* GIF end code */
#endif
   if (lzw->outbit)
      lzw->out[lzw->outpos++]=lzw->lastout;
}

void lzw_add(struct lzw *lzw,int c)
{
   struct lzwc *l;

   if (!lzw->current) /* no current, load */
   {
      lzw->current=lzw->code+c;
      return;
   }

   l=lzw->current->firstchild; /* check if we have this sequence */
   while (l)
   {
      if (l->c==c && l->no!=lzw->codes )
      {
	 lzw->current=l;
	 return;
      }
      l=l->next;
   }


#ifdef GIF_LZW
   if (lzw->codes==4096)  /* needs more than 12 bits */
   {
      int i;

      lzw_output(lzw,lzw->current->no);

      for (i=0; i<(1L<<lzw->bits); i++)
      {
	 lzw_free_lzwc_tree(lzw->code[i].firstchild);
	 lzw->code[i].firstchild=NULL;
      }
      lzw->codes=(1L<<lzw->bits)+2;
      
      /* output clearcode, 1000... (bits) */
      lzw_output(lzw,1L<<lzw->bits);

      lzw->codebits=lzw->bits+1;
      lzw->current=lzw->code+c;
      return;
   }
#endif

   /* output current code no, make new & reset */

   lzw_output(lzw,lzw->current->no);

   l=lzw->current->firstchild;

   lzw->current->firstchild=malloc(sizeof(struct lzwc));
   lzw->current->firstchild->next=l;
   l=lzw->current->firstchild;

   l->firstchild=NULL;
   l->no=lzw->codes;
   l->c=c;

   lzw->codes++;
   if (lzw->codes>(1L<<lzw->codebits)) lzw->codebits++;

   lzw->current=lzw->code+c;
}

