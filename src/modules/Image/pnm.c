/* $Id: pnm.c,v 1.6 1997/10/27 22:41:27 mirar Exp $ */

/*
**! module Image
**! note
**!	$Id: pnm.c,v 1.6 1997/10/27 22:41:27 mirar Exp $
**! class image
*/

#include "global.h"

#include <math.h>
#include <ctype.h>

#include "stralloc.h"
#include "global.h"
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "array.h"
#include "error.h"

#include "image.h"

#define THIS ((struct image *)(fp->current_storage))
#define THISOBJ (fp->current_object)
#define pixel(_img,x,y) ((_img)->img[(x)+(y)*(_img)->xsize])


static INLINE unsigned char getnext(struct pike_string *s,INT32 *pos)
{
   if (*pos>=s->len) return 0;
   if (s->str[(*pos)]=='#')
      for (;*pos<s->len && ISSPACE(s->str[*pos]);(*pos)++);
   return s->str[(*pos)++];
}

static INLINE void skip_to_eol(struct pike_string *s,INT32 *pos)
{
   for (;*pos<s->len && s->str[*pos]!=10;(*pos)++);
}

static INLINE unsigned char getnext_skip_comment(struct pike_string *s,INT32 *pos)
{
   unsigned char c;
   while ((c=getnext(s,pos))=='#')
      skip_to_eol(s,pos);
   return c;
}

static INLINE void skipwhite(struct pike_string *s,INT32 *pos)
{
   while (*pos<s->len && 
	  ( ISSPACE(s->str[*pos]) ||
	    s->str[*pos]=='#'))
      getnext_skip_comment(s,pos);
}

static INLINE INT32 getnextnum(struct pike_string *s,INT32 *pos)
{
   INT32 i;
   skipwhite(s,pos);
   i=0;
   while (*pos<s->len &&
	  s->str[*pos]>='0' && s->str[*pos]<='9')
   {
      i=(i*10)+s->str[*pos]-'0';
      getnext(s,pos);
   }
   return i;
}

static char* img_frompnm(struct pike_string *s)
{
   struct image new;
   INT32 type,c=0,maxval=255;
   INT32 pos=0,x,y,i;

   skipwhite(s,&pos);
   if (getnext(s,&pos)!='P') return "not pnm"; /* not pnm */
   type=getnext(s,&pos);
   if (type<'1'||type>'6') return "unknown type"; /* unknown type */
   new.xsize=getnextnum(s,&pos);
   new.ysize=getnextnum(s,&pos);
   if (new.xsize<=0||new.ysize<=0) return "illegal size"; /* illegal size */
   if (type=='3'||type=='2'||type=='6'||type=='5')
      maxval=getnextnum(s,&pos);
   new.img=malloc(new.xsize*new.ysize*sizeof(rgb_group)+1);
   if (!new.img) error("Out of memory.\n");

   if (type=='1'||type=='2'||type=='3')
   {
     skipwhite(s,&pos);
   }
   else
   {
     skip_to_eol(s,&pos);
     pos++;
   }
   for (y=0; y<new.ysize; y++)
   {
      for (i=0,x=0; x<new.xsize; x++)
      {
         switch (type)
	 {
	    case '1':
	       c=getnextnum(s,&pos);
               pixel(&new,x,y).r=pixel(&new,x,y).g=pixel(&new,x,y).b=
	          (unsigned char)~(c*255);
	       break;
	    case '2':
	       c=getnextnum(s,&pos);
               pixel(&new,x,y).r=pixel(&new,x,y).g=pixel(&new,x,y).b=
	          (unsigned char)((c*255L)/maxval);
	       break;
	    case '3':
	       pixel(&new,x,y).r=(unsigned char)((getnextnum(s,&pos)*255L)/maxval);
	       pixel(&new,x,y).g=(unsigned char)((getnextnum(s,&pos)*255L)/maxval);
	       pixel(&new,x,y).b=(unsigned char)((getnextnum(s,&pos)*255L)/maxval);
	       break;
	    case '4':
	       if (!i) c=getnext(s,&pos),i=8;
               pixel(&new,x,y).r=pixel(&new,x,y).g=pixel(&new,x,y).b=
	          (unsigned char)~(((c>>7)&1)*255);
	       c<<=1;
	       i--;
	       break;
	    case '5':
	       c=getnext(s,&pos);
               pixel(&new,x,y).r=pixel(&new,x,y).g=pixel(&new,x,y).b=
	          (unsigned char)((c*255L)/maxval);
	       break;
	    case '6':
	       pixel(&new,x,y).r=(unsigned char)((getnext(s,&pos)*255L)/maxval);
	       pixel(&new,x,y).g=(unsigned char)((getnext(s,&pos)*255L)/maxval);
	       pixel(&new,x,y).b=(unsigned char)((getnext(s,&pos)*255L)/maxval);
	       break;
	 }
      }
   }
   if (THIS->img) free(THIS->img);
   THIS->xsize=new.xsize;
   THIS->ysize=new.ysize;
   THIS->img=new.img;
   return NULL;
}

/*
**! method string toppm()
**!	Returns PPM (P6, binary pixmap) data from the
**!     current image object.
**! returns PPM data
**! see also: frompnm, fromgif
**!
**! method object|string frompnm(string pnm)
**! method object|string fromppm(string pnm)
**!	Reads a PNM (PBM, PGM or PPM in ascii or binary)
**!	to the called image object.
**!
**!	The method accepts P1 through P6 type of PNM data.
**!
**!	"fromppm" is an alias for "frompnm".
**! returns the called object or a hint of what wronged.
**! arg string pnm
**!	pnm data, as a string
**! see also: toppm, fromgif
*/

void image_toppm(INT32 args)
{
   char buf[80];
   struct pike_string *a,*b;
   
   pop_n_elems(args);
   if (!THIS->img) { error("no image\n");  return; }
   sprintf(buf,"P6\n%d %d\n255\n",THIS->xsize,THIS->ysize);
   a=make_shared_string(buf);
   b=make_shared_binary_string((char*)THIS->img,
			       THIS->xsize*THIS->ysize*3);
   push_string(add_shared_strings(a,b));
   free_string(a);
   free_string(b);
}

void image_frompnm(INT32 args)
{
   char *s;
   if (args<1||
       sp[-args].type!=T_STRING)
      error("Illegal argument to image->frompnm()\n");
   s=img_frompnm(sp[-args].u.string);
   pop_n_elems(args);
   if (!s) { push_object(THISOBJ); THISOBJ->refs++; }
   else push_string(make_shared_string(s));
}

