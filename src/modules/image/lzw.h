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
      lzwcode_t no;
      unsigned char c;
      struct lzwc *firstchild;
      struct lzwc *next;
   } *code,*current;
};

void lzw_add(struct lzw *lzw,int c);
void lzw_quit(struct lzw *lzw);
void lzw_init(struct lzw *lzw,int bits);


