#include "global.h"

#include <math.h>
#include <ctype.h>

#include "stralloc.h"
#include "global.h"
#include "types.h"
#include "macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "array.h"
#include "error.h"

#include "image.h"

struct program *image_program;
#define THIS ((struct image *)(fp->current_storage))
#define THISOBJ (fp->current_object)

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)<(b)?(b):(a))

/***************** internals ***********************************/

#define apply_alpha(x,y,alpha) \
   ((unsigned char)((y*(255L-(alpha))+x*(alpha))/255L))

#define set_rgb_group_alpha(dest,src,alpha) \
   ((dest).r=apply_alpha((dest).r,(src).r,alpha), \
    (dest).g=apply_alpha((dest).g,(src).g,alpha), \
    (dest).b=apply_alpha((dest).b,(src).b,alpha))

#define pixel(_img,x,y) ((_img)->img[(x)+(y)*(_img)->xsize])

#define setpixel(x,y) \
   (THIS->alpha? \
    set_rgb_group_alpha(THIS->img[(x)+(y)*THIS->xsize],THIS->rgb,THIS->alpha): \
    ((pixel(THIS,x,y)=THIS->rgb),0))

#define setpixel_test(x,y) \
   (((x)<0||(y)<0||(x)>=THIS->xsize||(y)>=THIS->ysize)? \
    0:(setpixel(x,y),0))

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

/*** end internals ***/


void img_clear(rgb_group *dest,rgb_group rgb,INT32 size)
{
   while (size--) *(dest++)=rgb;
}

void img_box_nocheck(INT32 x1,INT32 y1,INT32 x2,INT32 y2)
{
   INT32 x,mod;
   rgb_group *foo,*end,rgb;

   mod=THIS->xsize-(x2-x1)-1;
   foo=THIS->img+x1+y1*THIS->xsize;
   end=THIS->img+x2+y2*THIS->xsize;
   rgb=THIS->rgb;

   if (!THIS->alpha)
      for (; foo<end; foo+=mod) for (x=x1; x<=x2; x++) *(foo++)=rgb;
   else
      for (; foo<end; foo+=mod) for (x=x1; x<=x2; x++,foo++) 
	 set_rgb_group_alpha(*foo,rgb,THIS->alpha);
}


void img_blit(rgb_group *dest,rgb_group *src,INT32 width,
	      INT32 lines,INT32 moddest,INT32 modsrc)
{
   while (lines--)
   {
      MEMCPY(dest,src,sizeof(rgb_group)*width);
      dest+=moddest;
      src+=modsrc;
   }
}

void img_crop(struct image *dest,
	      struct image *img,
	      INT32 x1,INT32 y1,
	      INT32 x2,INT32 y2)
{
   rgb_group *new;
   INT32 blitwidth;
   INT32 blitheight;

   if (dest->img) { free(dest->img); dest->img=NULL; }

   if (x1==0 && y1==0 &&
       img->xsize-1==x2 && img->ysize-1==y2)
   {
      *dest=*img;
      new=malloc( (x2-x1+1)*(y2-y1+1)*sizeof(rgb_group) + 1);
      if (!new) 
	error("Out of memory.\n");
      MEMCPY(new,img->img,(x2-x1+1)*(y2-y1+1)*sizeof(rgb_group));
      dest->img=new;
      return;
   }

   if (x1>x2) x1^=x2,x2^=x1,x1^=x2;
   if (y1>y2) y1^=y2,y2^=y1,y1^=y2;

   new=malloc( (x2-x1+1)*(y2-y1+1)*sizeof(rgb_group) +1);
   if (!new)
     error("Out of memory.\n");

   img_clear(new,THIS->rgb,(x2-x1+1)*(y2-y1+1));

   blitwidth=min(x2,img->xsize-1)-max(x1,0)+1;
   blitheight=min(y2,img->ysize-1)-max(y1,0)+1;

   img_blit(new+max(0,-x1)+(x2-x1+1)*max(0,-y1),
	    img->img+max(0,x1)+(img->xsize)*max(0,y1),
	    blitwidth,
	    blitheight,
	    (x2-x1+1),
	    img->xsize);

   dest->img=new;
   dest->xsize=x2-x1+1;
   dest->ysize=y2-y1+1;
}

void img_clone(struct image *newimg,struct image *img)
{
   if (newimg->img) free(newimg->img);
   newimg->img=malloc(sizeof(rgb_group)*img->xsize*img->ysize +1);
   if (!newimg->img) error("Out of memory!\n");
   MEMCPY(newimg->img,img->img,sizeof(rgb_group)*img->xsize*img->ysize);
   newimg->xsize=img->xsize;
   newimg->ysize=img->ysize;
   newimg->rgb=img->rgb;
}

void image_paste(INT32 args)
{
   struct image *img;
   INT32 x1,y1,x2,y2,blitwidth,blitheight;

   if (args<1
       || sp[-args].type!=T_OBJECT
       || !sp[-args].u.object
       || sp[-args].u.object->prog!=image_program)
      error("illegal argument 1 to image->paste()\n");
   if (!THIS->img) return;

   img=(struct image*)sp[-args].u.object->storage;
   if (!img) return;

   if (args>=3)
   {
      if (sp[1-args].type!=T_INT
	  || sp[2-args].type!=T_INT)
         error("illegal arguments to image->paste()\n");
      x1=sp[1-args].u.integer;
      y1=sp[2-args].u.integer;
   }
   else x1=y1=0;
   pop_n_elems(args-1);

   x2=x1+img->xsize-1;
   y2=y1+img->ysize-1;

   blitwidth=min(x2,THIS->xsize-1)-max(x1,0)+1;
   blitheight=min(y2,THIS->ysize-1)-max(y1,0)+1;

   img_blit(THIS->img+max(0,x1)+(THIS->xsize)*max(0,y1),
	    img->img+max(0,-x1)+(x2-x1+1)*max(0,-y1),
	    blitwidth,
	    blitheight,
	    THIS->xsize,
	    img->xsize);
}

void image_paste_alpha(INT32 args)
{
   struct image *img;
   INT32 x1,y1,x,y;

   if (args<2
       || sp[-args].type!=T_OBJECT
       || !sp[-args].u.object
       || sp[-args].u.object->prog!=image_program
       || sp[1-args].type!=T_INT)
      error("illegal arguments to image->paste_alpha()\n");
   if (!THIS->img) return;

   img=(struct image*)sp[-args].u.object->storage;
   if (!img) return;
   THIS->alpha=(unsigned char)(sp[1-args].u.integer);
   
   if (args>=4)
   {
      if (sp[2-args].type!=T_INT
	  || sp[3-args].type!=T_INT)
         error("illegal arguments to image->paste_alpha()\n");
      x1=sp[2-args].u.integer;
      y1=sp[3-args].u.integer;
   }
   else x1=y1=0;

   for (x=0; x<img->xsize; x++)
      for (y=0; y<img->ysize; y++)
      {
	 THIS->rgb=pixel(img,x,y);
	 setpixel_test(x1+x,y1+y);
      }

   pop_n_elems(args);
   THISOBJ->refs++;
   push_object(THISOBJ);
}

void image_paste_mask(INT32 args)
{
   struct image *img,*mask;
   INT32 x1,y1,x,y,x2,y2;

   if (args<2)
      error("illegal number of arguments to image->paste_mask()\n");
   if (sp[-args].type!=T_OBJECT
       || !sp[-args].u.object
       || sp[-args].u.object->prog!=image_program)
      error("illegal argument 1 to image->paste_mask()\n");
   if (sp[1-args].type!=T_OBJECT
       || !sp[1-args].u.object
       || sp[1-args].u.object->prog!=image_program)
      error("illegal argument 2 to image->paste_mask()\n");
   if (!THIS->img) return;

   img=(struct image*)sp[-args].u.object->storage;
   mask=(struct image*)sp[1-args].u.object->storage;
   if ((!img)||(!img->img)) error("argument 1 has no image\n");
   if ((!mask)||(!mask->img)) error("argument 2 (alpha) has no image\n");
   
   if (args>=4)
   {
      if (sp[2-args].type!=T_INT
	  || sp[3-args].type!=T_INT)
         error("illegal coordinate arguments to image->paste_mask()\n");
      x1=sp[2-args].u.integer;
      y1=sp[3-args].u.integer;
   }
   else x1=y1=0;

   x2=min(THIS->xsize-x1,min(img->xsize,mask->xsize));
   y2=min(THIS->ysize-y1,min(img->ysize,mask->ysize));

   for (x=max(0,-x1); x<x2; x++)
      for (y=max(0,-y1); y<y2; y++)
      {
	 pixel(THIS,x+x1,y+y1).r=
            (unsigned char)((pixel(THIS,x+x1,y+y1).r*(long)(255-pixel(mask,x,y).r)+
			     pixel(img,x,y).r*(long)pixel(mask,x,y).r)/255);
	 pixel(THIS,x+x1,y+y1).g=
            (unsigned char)((pixel(THIS,x+x1,y+y1).g*(long)(255-pixel(mask,x,y).g)+
			     pixel(img,x,y).g*(long)pixel(mask,x,y).g)/255);
	 pixel(THIS,x+x1,y+y1).b=
            (unsigned char)((pixel(THIS,x+x1,y+y1).b*(long)(255-pixel(mask,x,y).b)+
			     pixel(img,x,y).b*(long)pixel(mask,x,y).b)/255);
      }
   pop_n_elems(args);
   THISOBJ->refs++;
   push_object(THISOBJ);
}

void image_paste_alpha_color(INT32 args)
{
   struct image *img,*mask;
   INT32 x1,y1,x,y,x2,y2;

   if (args!=1 && args!=4 && args!=6 && args!=3)
      error("illegal number of arguments to image->paste_alpha_color()\n");
   if (sp[-args].type!=T_OBJECT
       || !sp[-args].u.object
       || sp[-args].u.object->prog!=image_program)
      error("illegal argument 1 to image->paste_alpha_color()\n");
   if (!THIS->img) return;

   if (args==6 || args==4) /* colors at arg 2..4 */
      getrgb(THIS,1,args,"image->paste_alpha_color()\n");
   if (args==3) /* coords at 2..3 */
   {
      if (sp[1-args].type!=T_INT
	  || sp[2-args].type!=T_INT)
         error("illegal coordinate arguments to image->paste_alpha_color()\n");
      x1=sp[1-args].u.integer;
      y1=sp[2-args].u.integer;
   }
   else if (args==6) /* at 5..6 */
   {
      if (sp[4-args].type!=T_INT
	  || sp[5-args].type!=T_INT)
         error("illegal coordinate arguments to image->paste_alpha_color()\n");
      x1=sp[4-args].u.integer;
      y1=sp[5-args].u.integer;
   }
   else x1=y1=0;

   mask=(struct image*)sp[-args].u.object->storage;
   if (!mask||!mask->img) error("argument 2 (alpha) has no image\n");
   
   x2=min(THIS->xsize-x1,mask->xsize);
   y2=min(THIS->ysize-y1,mask->ysize);

   for (x=max(0,-x1); x<x2; x++)
      for (y=max(0,-y1); y<y2; y++)
      {
	 pixel(THIS,x+x1,y+y1).r=
            (unsigned char)((pixel(THIS,x+x1,y+y1).r*(long)(255-pixel(mask,x,y).r)+
			     THIS->rgb.r*(long)pixel(mask,x,y).r)/255);
	 pixel(THIS,x+x1,y+y1).g=
            (unsigned char)((pixel(THIS,x+x1,y+y1).g*(long)(255-pixel(mask,x,y).g)+
			     THIS->rgb.g*(long)pixel(mask,x,y).g)/255);
	 pixel(THIS,x+x1,y+y1).b=
            (unsigned char)((pixel(THIS,x+x1,y+y1).b*(long)(255-pixel(mask,x,y).b)+
			     THIS->rgb.b*(long)pixel(mask,x,y).b)/255);
      }
   pop_n_elems(args);
   THISOBJ->refs++;
   push_object(THISOBJ);
}

void img_box(INT32 x1,INT32 y1,INT32 x2,INT32 y2)
{   
   if (x1>x2) x1^=x2,x2^=x1,x1^=x2;
   if (y1>y2) y1^=y2,y2^=y1,y1^=y2;
   if (x2<0||y2<0||x1>=THIS->xsize||y1>=THIS->ysize) return;
   img_box_nocheck(max(x1,0),max(y1,0),min(x2,THIS->xsize-1),min(y2,THIS->ysize-1));
}

