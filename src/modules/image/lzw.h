#define GIF_LZW

#ifdef GIF_LZW
typedef unsigned short lzwcode_t; /* no more than 12 bits used */
#else
typedef unsigned long lzwcode_t;
#endif

struct lzw
{
   unsigned long codes;
   unsigned long bits; /* initial encoding bits */
   unsigned long codebits; /* current encoding bits */
   unsigned long outlen,outpos,outbit;
   unsigned char *out,lastout;
   struct lzwc 
   {
      unsigned char c;
      lzwcode_t firstchild;
      lzwcode_t next;
   } *code;
   lzwcode_t current,firstfree;
#ifndef GIF_LZW
   unsigned long alloced;
#endif
};

#define LZWCNULL ((lzwcode_t)(~0))

void lzw_add(struct lzw *lzw,int c);
void lzw_quit(struct lzw *lzw);
void lzw_init(struct lzw *lzw,int bits);
unsigned long lzw_unpack(unsigned char *dest,unsigned long destlen,
			 unsigned char *src,unsigned long srclen,
			 int bits);



