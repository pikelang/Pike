/*
**! module Image
**! note
**!	$Id: gif_lzw.h,v 1.5 1998/04/29 01:27:23 mirar Exp $
*/

typedef unsigned short lzwcode_t; /* no more than 12 bits used */
#define LZWCNULL ((lzwcode_t)(~0))

struct lzwc
{
   unsigned short prev;
   unsigned short len;
   unsigned short c;
} *c;

struct gif_lzw
{
   int broken; /* lzw failed, out of memory */

   unsigned char *out;
   unsigned long outlen,lastout;

   int earlychange;
   int reversebits;
   
   unsigned long codes;
   unsigned long bits; /* initial encoding bits */
   unsigned long codebits; /* current encoding bits */
   unsigned long outpos,outbit;
   struct gif_lzwc 
   {
      unsigned char c;
      lzwcode_t firstchild;
      lzwcode_t next;
   } *code;
   lzwcode_t current,firstfree;
   unsigned long alloced;
};

/* returns number of strings written on stack */

void image_gif_lzw_init(struct gif_lzw *lzw,int bits);
void image_gif_lzw_add(struct gif_lzw *lzw,unsigned char *data,int len);
void image_gif_lzw_finish(struct gif_lzw *lzw);
void image_gif_lzw_free(struct gif_lzw *lzw);
