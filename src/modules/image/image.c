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

#define sq(x) ((x)*(x))

#define CIRCLE_STEPS 128
static INT32 circle_sin_table[CIRCLE_STEPS];
#define circle_sin(x) circle_sin_table[((x)+CIRCLE_STEPS)%CIRCLE_STEPS]
#define circle_cos(x) circle_sin((x)-CIRCLE_STEPS/4)

#define circle_sin_mul(x,y) ((circle_sin(x)*(y))/4096)
#define circle_cos_mul(x,y) ((circle_cos(x)*(y))/4096)

/***************** init & exit *********************************/

static void init_image_struct(struct object *o)
{
  THIS->img=NULL;
  THIS->rgb.r=0;
  THIS->rgb.g=0;
  THIS->rgb.b=0;
}

static void exit_image_struct(struct object *obj)
{
  if (THIS->img) { free(THIS->img); THIS->img=NULL; }
}

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

static INLINE void getrgbl(rgbl_group *rgb,INT32 args_start,INT32 args,char *name)
{
   INT32 i;
   if (args-args_start<3) return;
   for (i=0; i<3; i++)
      if (sp[-args+i+args_start].type!=T_INT)
         error("Illegal r,g,b argument to %s\n",name);
   rgb->r=sp[-args+args_start].u.integer;
   rgb->g=sp[1-args+args_start].u.integer;
   rgb->b=sp[2-args+args_start].u.integer;
}

static void img_clear(rgb_group *dest,rgb_group rgb,INT32 size)
{
   while (size--) *(dest++)=rgb;
}

static INLINE void img_box_nocheck(INT32 x1,INT32 y1,INT32 x2,INT32 y2)
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

static INLINE void img_blit(rgb_group *dest,rgb_group *src,INT32 width,
			    INT32 lines,INT32 moddest,INT32 modsrc)
{
   while (lines--)
   {
      MEMCPY(dest,src,sizeof(rgb_group)*width);
      dest+=moddest;
      src+=modsrc;
   }
}

static void img_crop(struct image *dest,
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

static INLINE void img_clone(struct image *newimg,struct image *img)
{
   if (newimg->img) free(newimg->img);
   newimg->img=malloc(sizeof(rgb_group)*img->xsize*img->ysize +1);
   if (!newimg->img) error("Out of memory!\n");
   MEMCPY(newimg->img,img->img,sizeof(rgb_group)*img->xsize*img->ysize);
   newimg->xsize=img->xsize;
   newimg->ysize=img->ysize;
   newimg->rgb=img->rgb;
}

static INLINE void img_line(INT32 x1,INT32 y1,INT32 x2,INT32 y2)
{   
   INT32 pixelstep,pos;
   if (x1==x2) 
   {
      if (y1>y2) y1^=y2,y2^=y1,y1^=y2;
      if (x1<0||x1>=THIS->xsize||
	  y2<0||y2>=THIS->ysize) return;
      if (y1<0) y1=0;
      if (y2>=THIS->ysize) y2=THIS->ysize-1;
      for (;y1<=y2;y1++) setpixel_test(x1,y1);
      return;
   }
   else if (y1==y2)
   {
      if (x1>x2) x1^=x2,x2^=x1,x1^=x2;
      if (y1<0||y1>=THIS->ysize||
	  x2<0||x1>=THIS->xsize) return;
      if (x1<0) x1=0; 
      if (x2>=THIS->xsize) x2=THIS->xsize-1;
      for (;x1<=x2;x1++) setpixel_test(x1,y1);
      return;
   }
   else if (abs(x2-x1)<abs(y2-y1)) /* mostly vertical line */
   {
      if (y1>y2) y1^=y2,y2^=y1,y1^=y2,
                 x1^=x2,x2^=x1,x1^=x2;
      pixelstep=((x2-x1)*1024)/(y2-y1);
      pos=x1*1024;
      for (;y1<=y2;y1++)
      {
         setpixel_test((pos+512)/1024,y1);
	 pos+=pixelstep;
      }
   }
   else /* mostly horisontal line */
   {
      if (x1>x2) y1^=y2,y2^=y1,y1^=y2,
                 x1^=x2,x2^=x1,x1^=x2;
      pixelstep=((y2-y1)*1024)/(x2-x1);
      pos=y1*1024;
      for (;x1<=x2;x1++)
      {
         setpixel_test(x1,(pos+512)/1024);
	 pos+=pixelstep;
      }
   }
}


static INLINE void img_box(INT32 x1,INT32 y1,INT32 x2,INT32 y2)
{   
   if (x1>x2) x1^=x2,x2^=x1,x1^=x2;
   if (y1>y2) y1^=y2,y2^=y1,y1^=y2;
   if (x2<0||y2<0||x1>=THIS->xsize||y1>=THIS->ysize) return;
   img_box_nocheck(max(x1,0),max(y1,0),min(x2,THIS->xsize-1),min(y2,THIS->ysize-1));
}

#define decimals(x) ((x)-(int)(x))
#define testrange(x) max(min((x),255),0)
#define _scale_add_rgb(dest,src,factor) \
   ((dest).r=testrange((dest).r+(int)((src).r*(factor)+0.5)), \
    (dest).g=testrange((dest).g+(int)((src).g*(factor)+0.5)), \
    (dest).b=testrange((dest).b+(int)((src).b*(factor)+0.5))) 
#define scale_add_pixel(dest,dx,dy,dxw,src,sx,sy,sxw,factor) \
   _scale_add_rgb(dest[(dx)+(dy)*(dxw)],src[(sx)+(sy)*(sxw)],factor)

static INLINE void scale_add_line(rgb_group *new,INT32 yn,INT32 newx,
				  rgb_group *img,INT32 y,INT32 xsize,
				  double py,double dx)
{
   INT32 x,xd;
   double xn;
   for (x=0,xn=0; x<THIS->xsize; x++,xn+=dx)
   {
      if ((INT32)xn<(INT32)(xn+dx))
      {
         scale_add_pixel(new,(INT32)xn,yn,newx,img,x,y,xsize,py*(1.0-decimals(xn)));
	 if ((xd=(INT32)(xn+dx)-(INT32)(xn))>1) 
            while (--xd)
               scale_add_pixel(new,(INT32)xn+xd,yn,newx,img,x,y,xsize,py);
	 scale_add_pixel(new,(INT32)(xn+dx),yn,newx,img,x,y,xsize,py*decimals(xn+dx));
      }
      else
         scale_add_pixel(new,(int)xn,yn,newx,img,x,y,xsize,py*dx);
   }
}

static void img_scale(struct image *dest,
		      struct image *source,
		      INT32 newx,INT32 newy)
{
   rgb_group *new;
   INT32 y,yd;
   double yn,dx,dy;

   if (dest->img) { free(dest->img); dest->img=NULL; }

   if (!THIS->img || newx<=0 || newy<=0) return; /* no way */
   new=malloc(newx*newy*sizeof(rgb_group) +1);
   if (!new) error("Out of memory!\n");

   MEMSET(new,0,newx*newy*sizeof(rgb_group));
   
   dx=((double)newx-0.000001)/source->xsize; 
   dy=((double)newy-0.000001)/source->ysize; 

   for (y=0,yn=0; y<source->ysize; y++,yn+=dy)
   {
      if ((INT32)yn<(INT32)(yn+dy))
      {
	 scale_add_line(new,(INT32)(yn),newx,source->img,y,source->xsize,
			(1.0-decimals(yn)),dx);
	 if ((yd=(INT32)(yn+dy)-(INT32)(yn))>1) 
            while (--yd)
   	       scale_add_line(new,(INT32)yn+yd,newx,source->img,y,source->xsize,
			      1.0,dx);
	 scale_add_line(new,(INT32)(yn+dy),newx,source->img,y,source->xsize,
			(decimals(yn+dy)),dx);
      }
      else
	 scale_add_line(new,(INT32)yn,newx,source->img,y,source->xsize,
			dy,dx);
   }

   dest->img=new;
   dest->xsize=newx;
   dest->ysize=newy;
}

/* Special, faster, case for scale=1/2 */
static void img_scale2(struct image *dest, struct image *source)
{
   rgb_group *new;
   INT32 x, y, newx, newy;
   newx = source->xsize >> 1;
   newy = source->ysize >> 1;
   
   if (dest->img) { free(dest->img); dest->img=NULL; }
   if (!THIS->img || newx<=0 || newy<=0) return; /* no way */
   new=malloc(newx*newy*sizeof(rgb_group) +1);
   if (!new) error("Out of memory\n");
   MEMSET(new,0,newx*newy*sizeof(rgb_group));

   dest->img=new;
   dest->xsize=newx;
   dest->ysize=newy;
   for (y = 0; y < newy; y++)
     for (x = 0; x < newx; x++) {
       pixel(dest,x,y).r = (COLOURTYPE)
	                    (((INT32) pixel(source,2*x+0,2*y+0).r+
			      (INT32) pixel(source,2*x+1,2*y+0).r+
			      (INT32) pixel(source,2*x+0,2*y+1).r+
			      (INT32) pixel(source,2*x+1,2*y+1).r) >> 2);
       pixel(dest,x,y).g = (COLOURTYPE)
	                    (((INT32) pixel(source,2*x+0,2*y+0).g+
			      (INT32) pixel(source,2*x+1,2*y+0).g+
			      (INT32) pixel(source,2*x+0,2*y+1).g+
			      (INT32) pixel(source,2*x+1,2*y+1).g) >> 2);
       pixel(dest,x,y).b = (COLOURTYPE)
	                    (((INT32) pixel(source,2*x+0,2*y+0).b+
			      (INT32) pixel(source,2*x+1,2*y+0).b+
			      (INT32) pixel(source,2*x+0,2*y+1).b+
			      (INT32) pixel(source,2*x+1,2*y+1).b) >> 2);
     }
}

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

static INLINE rgb_group _pixel_apply_matrix(struct image *img,
					    int x,int y,
					    int width,int height,
					    rgbl_group *matrix,
					    rgb_group default_rgb,
					    INT32 div)
{
   rgb_group res;
   int i,j,bx,by,xp,yp;
   int sumr,sumg,sumb,r,g,b;

   sumr=sumg=sumb=0;
   r=g=b=0;

   bx=width/2;
   by=height/2;
   
   for (xp=x-bx,i=0; i<width; i++,xp++)
      for (yp=y-by,j=0; j<height; j++,yp++)
	 if (xp>=0 && xp<img->xsize && yp>=0 && yp<img->ysize)
	 {
	    r+=matrix[i+j*width].r*img->img[xp+yp*img->xsize].r;
	    g+=matrix[i+j*width].g*img->img[xp+yp*img->xsize].g;
	    b+=matrix[i+j*width].b*img->img[xp+yp*img->xsize].b;
#ifdef MATRIX_DEBUG
	    fprintf(stderr,"%d,%d %d,%d->%d,%d,%d\n",
		    i,j,xp,yp,
		    img->img[x+i+(y+j)*img->xsize].r,
		    img->img[x+i+(y+j)*img->xsize].g,
		    img->img[x+i+(y+j)*img->xsize].b);
#endif
	    sumr+=matrix[i+j*width].r;
	    sumg+=matrix[i+j*width].g;
	    sumb+=matrix[i+j*width].b;
	 }
   if (sumr) res.r=testrange(default_rgb.r+r/(sumr*div)); 
   else res.r=testrange(r/div+default_rgb.r);
   if (sumg) res.g=testrange(default_rgb.g+g/(sumg*div)); 
   else res.g=testrange(g/div+default_rgb.g);
   if (sumb) res.b=testrange(default_rgb.g+b/(sumb*div)); 
   else res.b=testrange(b/div+default_rgb.b);
#ifdef MATRIX_DEBUG
   fprintf(stderr,"->%d,%d,%d\n",res.r,res.g,res.b);
#endif
   return res;
}

static void img_apply_matrix(struct image *dest,
		      struct image *img,
		      int width,int height,
		      rgbl_group *matrix,
		      rgb_group default_rgb,
		      INT32 div)
{
   rgb_group *d;
   int i,j,x,y,bx,by,ex,ey,xp,yp;
   int sumr,sumg,sumb;

   sumr=sumg=sumb=0;
   for (i=0; i<width; i++)
      for (j=0; j<height; j++)
      {
	 sumr+=matrix[i+j*width].r;
	 sumg+=matrix[i+j*width].g;
	 sumb+=matrix[i+j*width].b;
      }

   if (!sumr) sumr=1; sumr*=div;
   if (!sumg) sumg=1; sumg*=div;
   if (!sumb) sumb=1; sumb*=div;

   bx=width/2;
   by=height/2;
   ex=width-bx;
   ey=height-by;
   
   d=malloc(sizeof(rgb_group)*img->xsize*img->ysize +1);

   if(!d) error("Out of memory.\n");
   
   for (x=bx; x<img->xsize-ex; x++)
   {
      for (y=by; y<img->ysize-ey; y++)
      {
	 long r=0,g=0,b=0;
	 for (xp=x-bx,i=0; i<width; i++,xp++)
	    for (yp=y-by,j=0; j<height; j++,yp++)
	    {
	       r+=matrix[i+j*width].r*img->img[xp+yp*img->xsize].r;
	       g+=matrix[i+j*width].g*img->img[xp+yp*img->xsize].g;
	       b+=matrix[i+j*width].b*img->img[xp+yp*img->xsize].b;
#ifdef MATRIX_DEBUG
	       fprintf(stderr,"%d,%d ->%d,%d,%d\n",
		       i,j,
		       img->img[x+i+(y+j)*img->xsize].r,
		       img->img[x+i+(y+j)*img->xsize].g,
		       img->img[x+i+(y+j)*img->xsize].b);
#endif
	    }
#ifdef MATRIX_DEBUG
	 fprintf(stderr,"->%d,%d,%d\n",r/sumr,g/sumg,b/sumb);
#endif
	 d[x+y*img->xsize].r=testrange(default_rgb.r+r/sumr);
	 d[x+y*img->xsize].g=testrange(default_rgb.g+g/sumg);
	 d[x+y*img->xsize].b=testrange(default_rgb.b+b/sumb);
      }
   }


   for (y=0; y<img->ysize; y++)
   {
      for (x=0; x<bx; x++)
	 d[x+y*img->xsize]=_pixel_apply_matrix(img,x,y,width,height,
					       matrix,default_rgb,div);
      for (x=img->xsize-ex; x<img->xsize; x++)
	 d[x+y*img->xsize]=_pixel_apply_matrix(img,x,y,width,height,
					       matrix,default_rgb,div);
   }

   for (x=0; x<img->xsize; x++)
   {
      for (y=0; y<by; y++)
	 d[x+y*img->xsize]=_pixel_apply_matrix(img,x,y,width,height,
					       matrix,default_rgb,div);
      for (y=img->ysize-ey; y<img->ysize; y++)
	 d[x+y*img->xsize]=_pixel_apply_matrix(img,x,y,width,height,
					       matrix,default_rgb,div);
   }

   if (dest->img) free(dest->img);
   *dest=*img;
   dest->img=d;
}

/***************** methods *************************************/

void image_create(INT32 args)
{
   if (args<2) return;
   if (sp[-args].type!=T_INT||
       sp[1-args].type!=T_INT)
      error("Illegal arguments to image->clone()\n");

   getrgb(THIS,2,args,"image->create()"); 

   if (THIS->img) free(THIS->img);
	
   THIS->xsize=sp[-args].u.integer;
   THIS->ysize=sp[1-args].u.integer;
   if (THIS->xsize<0) THIS->xsize=0;
   if (THIS->ysize<0) THIS->ysize=0;

   THIS->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize +1);
   if (!THIS->img)
     error("out of memory\n");


   img_clear(THIS->img,THIS->rgb,THIS->xsize*THIS->ysize);
   pop_n_elems(args);
}

void image_clone(INT32 args)
{
   struct object *o;
   struct image *img;

   if (args)
      if (args<2||
	  sp[-args].type!=T_INT||
	  sp[1-args].type!=T_INT)
	 error("Illegal arguments to image->clone()\n");

   o=clone(image_program,0);
   img=(struct image*)(o->storage);
   *img=*THIS;

   if (args)
   {
      if(sp[-args].u.integer < 0 ||
	 sp[1-args].u.integer < 0)
	 error("Illegal size to image->clone()\n");
      img->xsize=sp[-args].u.integer;
      img->ysize=sp[1-args].u.integer;
   }

   getrgb(img,2,args,"image->clone()"); 

   if (img->xsize<0) img->xsize=1;
   if (img->ysize<0) img->ysize=1;

   img->img=malloc(sizeof(rgb_group)*img->xsize*img->ysize +1);
   if (THIS->img)
   {
      if (!img->img)
      {
	 free_object(o);
	 error("out of memory\n");
      }
      if (img->xsize==THIS->xsize 
	  && img->ysize==THIS->ysize)
	 MEMCPY(img->img,THIS->img,sizeof(rgb_group)*img->xsize*img->ysize);
      else
	 img_crop(img,THIS,
		  0,0,img->xsize-1,img->ysize-1);
	 
   }
   else
      img_clear(img->img,img->rgb,img->xsize*img->ysize);

   pop_n_elems(args);
   push_object(o);
}

void image_clear(INT32 args)
{
   struct object *o;
   struct image *img;

   o=clone(image_program,0);
   img=(struct image*)(o->storage);
   *img=*THIS;

   getrgb(img,0,args,"image->clear()"); 

   img->img=malloc(sizeof(rgb_group)*img->xsize*img->ysize +1);
   if (!img->img)
   {
     free_object(o);
     error("out of memory\n");
   }

   img_clear(img->img,img->rgb,img->xsize*img->ysize);

   pop_n_elems(args);
   push_object(o);
}

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

void image_togif(INT32 args)
{
   char buf[80];
   struct pike_string *a,*b;
   rgb_group *transparent=NULL;
   struct colortable *ct;

   if (args>=3)
   {
      getrgb(THIS,0,args,"image->togif() (transparency)");
      transparent=&(THIS->rgb);
   }

   pop_n_elems(args);
   if (!THIS->img) { error("no image\n");  return; }
   ct=colortable_quant(THIS,256);
   push_string( image_encode_gif( THIS,ct, transparent, 0) );
   colortable_free(ct);
}

void image_togif_fs(INT32 args)
{
   char buf[80];
   struct pike_string *a,*b;
   rgb_group *transparent=NULL;
   struct colortable *ct;

   if (args>=3)
   {
      getrgb(THIS,0,args,"image->togif_fs() (transparency)");
      transparent=&(THIS->rgb);
   }

   pop_n_elems(args);
   if (!THIS->img) { error("no image\n");  return; }
   ct=colortable_quant(THIS,256);
   push_string( image_encode_gif( THIS,ct, transparent, 1) );
   colortable_free(ct);
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

void image_copy(INT32 args)
{
   struct object *o;
   struct image *img;

   if (!args)
   {
      o=clone(image_program,0);
      if (THIS->img) img_clone((struct image*)o->storage,THIS);
      pop_n_elems(args);
      push_object(o);
      return;
   }
   if (args<4||
       sp[-args].type!=T_INT||
       sp[1-args].type!=T_INT||
       sp[2-args].type!=T_INT||
       sp[3-args].type!=T_INT)
      error("illegal arguments to image->copy()\n");

   if (!THIS->img) error("no image\n");

   getrgb(THIS,2,args,"image->crop()"); 

   o=clone(image_program,0);
   img=(struct image*)(o->storage);

   img_crop(img,THIS,
	    sp[-args].u.integer,sp[1-args].u.integer,
	    sp[2-args].u.integer,sp[3-args].u.integer);

   pop_n_elems(args);
   push_object(o);
}

static INLINE int try_autocrop_vertical(INT32 x,INT32 y,INT32 y2,
					INT32 rgb_set,rgb_group *rgb)
{
   if (!rgb_set) *rgb=pixel(THIS,x,y);
   for (;y<=y2; y++)
      if (pixel(THIS,x,y).r!=rgb->r ||
	  pixel(THIS,x,y).g!=rgb->g ||
	  pixel(THIS,x,y).b!=rgb->b) return 0;
   return 1;
}

static INLINE int try_autocrop_horisontal(INT32 y,INT32 x,INT32 x2,
					  INT32 rgb_set,rgb_group *rgb)
{
   if (!rgb_set) *rgb=pixel(THIS,x,y);
   for (;x<=x2; x++)
      if (pixel(THIS,x,y).r!=rgb->r ||
	  pixel(THIS,x,y).g!=rgb->g ||
	  pixel(THIS,x,y).b!=rgb->b) return 0;
   return 1;
}

void image_autocrop(INT32 args)
{
   INT32 border=0,x1,y1,x2,y2;
   rgb_group rgb;
   int rgb_set=0,done;
   struct object *o;
   struct image *img;
   int left=1,right=1,top=1,bottom=1;

   if (args)
      if (sp[-args].type!=T_INT)
         error("Illegal argument to image->autocrop()\n");
      else
         border=sp[-args].u.integer; 

   if (args>=5)
   {
      left=!(sp[1-args].type==T_INT && sp[1-args].u.integer==0);
      right=!(sp[2-args].type==T_INT && sp[2-args].u.integer==0);
      top=!(sp[3-args].type==T_INT && sp[3-args].u.integer==0);
      bottom=!(sp[4-args].type==T_INT && sp[4-args].u.integer==0);
      getrgb(THIS,5,args,"image->autocrop()"); 
   }
   else getrgb(THIS,1,args,"image->autocrop()"); 

   if (!THIS->img)
   {
      error("no image\n");
      return;
   }

   x1=y1=0;
   x2=THIS->xsize-1;
   y2=THIS->ysize-1;

   while (x2>x1 && y2>y1)
   {
      done=0;
      if (left &&
	  try_autocrop_vertical(x1,y1,y2,rgb_set,&rgb)) x1++,done=rgb_set=1;
      if (right &&
	  x2>x1 && 
	  try_autocrop_vertical(x2,y1,y2,rgb_set,&rgb)) x2--,done=rgb_set=1;
      if (top &&
	  try_autocrop_horisontal(y1,x1,x2,rgb_set,&rgb)) y1++,done=rgb_set=1;
      if (bottom &&
	  y2>y1 && 
	  try_autocrop_horisontal(y2,x1,x2,rgb_set,&rgb)) y2--,done=rgb_set=1;
      if (!done) break;
   }

   o=clone(image_program,0);
   img=(struct image*)(o->storage);

   img_crop(img,THIS,x1-border,y1-border,x2+border,y2+border);

   pop_n_elems(args);
   push_object(o);
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

void image_setcolor(INT32 args)
{
   if (args<3)
      error("illegal arguments to image->setcolor()\n");
   getrgb(THIS,0,args,"image->setcolor()");
   pop_n_elems(args);
   THISOBJ->refs++;
   push_object(THISOBJ);
}

void image_setpixel(INT32 args)
{
   INT32 x,y;
   if (args<2||
       sp[-args].type!=T_INT||
       sp[1-args].type!=T_INT)
      error("Illegal arguments to image->setpixel()\n");
   getrgb(THIS,2,args,"image->setpixel()");   
   if (!THIS->img) return;
   x=sp[-args].u.integer;
   y=sp[1-args].u.integer;
   if (!THIS->img) return;
   setpixel_test(x,y);
   pop_n_elems(args);
   THISOBJ->refs++;
   push_object(THISOBJ);
}

void image_line(INT32 args)
{
   if (args<4||
       sp[-args].type!=T_INT||
       sp[1-args].type!=T_INT||
       sp[2-args].type!=T_INT||
       sp[3-args].type!=T_INT)
      error("Illegal arguments to image->line()\n");
   getrgb(THIS,4,args,"image->line()");
   if (!THIS->img) return;

   img_line(sp[-args].u.integer,
	    sp[1-args].u.integer,
	    sp[2-args].u.integer,
	    sp[3-args].u.integer);
   pop_n_elems(args);
   THISOBJ->refs++;
   push_object(THISOBJ);
}

void image_box(INT32 args)
{
   if (args<4||
       sp[-args].type!=T_INT||
       sp[1-args].type!=T_INT||
       sp[2-args].type!=T_INT||
       sp[3-args].type!=T_INT)
      error("Illegal arguments to image->box()\n");
   getrgb(THIS,4,args,"image->box()");
   if (!THIS->img) return;

   img_box(sp[-args].u.integer,
	   sp[1-args].u.integer,
	   sp[2-args].u.integer,
	   sp[3-args].u.integer);
   pop_n_elems(args);
   THISOBJ->refs++;
   push_object(THISOBJ);
}

void image_circle(INT32 args)
{
   INT32 x,y,rx,ry;
   INT32 i;

   if (args<4||
       sp[-args].type!=T_INT||
       sp[1-args].type!=T_INT||
       sp[2-args].type!=T_INT||
       sp[3-args].type!=T_INT)
      error("illegal arguments to image->circle()\n");
   getrgb(THIS,4,args,"image->circle()");
   if (!THIS->img) return;

   x=sp[-args].u.integer;
   y=sp[1-args].u.integer;
   rx=sp[2-args].u.integer;
   ry=sp[3-args].u.integer;
   
   for (i=0; i<CIRCLE_STEPS; i++)
      img_line(x+circle_sin_mul(i,rx),
	       y+circle_cos_mul(i,ry),
	       x+circle_sin_mul(i+1,rx),
	       y+circle_cos_mul(i+1,ry));
   THISOBJ->refs++;
   push_object(THISOBJ);
}

void image_scale(INT32 args)
{
   float factor;
   struct object *o;
   struct image *newimg;
   
   o=clone(image_program,0);
   newimg=(struct image*)(o->storage);

   if (args==1 && sp[-args].type==T_FLOAT) {
     if (sp[-args].u.float_number == 0.5)
       img_scale2(newimg,THIS);
     else
       img_scale(newimg,THIS,
		 (INT32)(THIS->xsize*sp[-args].u.float_number),
		 (INT32)(THIS->ysize*sp[-args].u.float_number));
   }
   else if (args>=2 &&
	    sp[-args].type==T_INT && sp[-args].u.integer==0 &&
	    sp[1-args].type==T_INT)
   {
      factor=((float)sp[1-args].u.integer)/THIS->ysize;
      img_scale(newimg,THIS,
		(INT32)(THIS->xsize*factor),
		sp[1-args].u.integer);
   }
   else if (args>=2 &&
	    sp[1-args].type==T_INT && sp[1-args].u.integer==0 &&
	    sp[-args].type==T_INT)
   {
      factor=((float)sp[-args].u.integer)/THIS->xsize;
      img_scale(newimg,THIS,
		sp[-args].u.integer,
		(INT32)(THIS->ysize*factor));
   }
   else if (args>=2 &&
	    sp[-args].type==T_FLOAT &&
	    sp[1-args].type==T_FLOAT)
      img_scale(newimg,THIS,
		(INT32)(THIS->xsize*sp[-args].u.float_number),
		(INT32)(THIS->ysize*sp[1-args].u.float_number));
   else if (args>=2 &&
	    sp[-args].type==T_INT &&
	    sp[1-args].type==T_INT)
      img_scale(newimg,THIS,
		sp[-args].u.integer,
		sp[1-args].u.integer);
   else
   {
      free_object(o);
      error("illegal arguments to image->scale()\n");
   }
   pop_n_elems(args);
   push_object(o);
}

static INLINE void get_rgba_group_from_array_index(rgba_group *rgba,struct array *v,INT32 index)
{
   struct svalue s,s2;
   array_index_no_free(&s,v,index);
   if (s.type!=T_ARRAY||
       s.u.array->size<3) 
      rgba->r=rgba->b=rgba->g=rgba->alpha=0; 
   else
   {
      array_index_no_free(&s2,s.u.array,0);
      if (s2.type!=T_INT) rgba->r=0; else rgba->r=s2.u.integer;
      array_index(&s2,s.u.array,1);
      if (s2.type!=T_INT) rgba->g=0; else rgba->g=s2.u.integer;
      array_index(&s2,s.u.array,2);
      if (s2.type!=T_INT) rgba->b=0; else rgba->b=s2.u.integer;
      if (s.u.array->size>=4)
      {
	 array_index(&s2,s.u.array,3);
	 if (s2.type!=T_INT) rgba->alpha=0; else rgba->alpha=s2.u.integer;
      }
      else rgba->alpha=0;
      free_svalue(&s2);
   }
   free_svalue(&s); 
   return; 
}

static INLINE void
   add_to_rgba_sum_with_factor(rgba_group *sum,
			       rgba_group rgba,
			       float factor)
{
   sum->r=testrange(sum->r+(INT32)(rgba.r*factor+0.5));
   sum->g=testrange(sum->g+(INT32)(rgba.g*factor+0.5));
   sum->b=testrange(sum->b+(INT32)(rgba.b*factor+0.5));
   sum->alpha=testrange(sum->alpha+(INT32)(rgba.alpha*factor+0.5));
}

void image_tuned_box(INT32 args)
{
   INT32 x1,y1,x2,y2,xw,yw,x,y;
   rgba_group topleft,topright,bottomleft,bottomright,sum,sumzero={0,0,0,0};
   rgb_group *img;
   INT32 mod;

   if (args<5||
       sp[-args].type!=T_INT||
       sp[1-args].type!=T_INT||
       sp[2-args].type!=T_INT||
       sp[3-args].type!=T_INT||
       sp[4-args].type!=T_ARRAY||
       sp[4-args].u.array->size<4)
      error("Illegal number of arguments to image->tuned_box()\n");

   if (!THIS->img)
      error("no image\n");

   x1=sp[-args].u.integer;
   y1=sp[1-args].u.integer;
   x2=sp[2-args].u.integer;
   y2=sp[3-args].u.integer;

   get_rgba_group_from_array_index(&topleft,sp[4-args].u.array,0);
   get_rgba_group_from_array_index(&topright,sp[4-args].u.array,1);
   get_rgba_group_from_array_index(&bottomleft,sp[4-args].u.array,2);
   get_rgba_group_from_array_index(&bottomright,sp[4-args].u.array,3);

   if (x1>x2) x1^=x2,x2^=x1,x1^=x2,
              sum=topleft,topleft=topright,topright=sum,
              sum=bottomleft,bottomleft=bottomright,bottomright=sum;
   if (y1>y2) y1^=y2,y2^=y1,y1^=y2,
              sum=topleft,topleft=bottomleft,bottomleft=sum,
              sum=topright,topright=bottomright,bottomright=sum;
   if (x2<0||y2<0||x1>=THIS->xsize||y1>=THIS->ysize) return;

   xw=x2-x1;
   yw=y2-y1;
   for (x=max(0,-x1); x<=xw && x+x1<THIS->xsize; x++)
   {
#define tune_factor(a,aw) (1.0-((float)(a)/(aw)))
      INT32 ymax;
      float tfx1=tune_factor(x,xw);
      float tfx2=tune_factor(xw-x,xw);

      ymax=min(yw,THIS->ysize-y1);
      img=THIS->img+x+x1+THIS->xsize*max(0,y1);
      for (y=max(0,-y1); y<ymax; y++)
      {
	 float tfy;
	 sum=sumzero;

	 add_to_rgba_sum_with_factor(&sum,topleft,(tfy=tune_factor(y,yw))*tfx1);
	 add_to_rgba_sum_with_factor(&sum,topright,tfy*tfx2);
	 add_to_rgba_sum_with_factor(&sum,bottomleft,(tfy=tune_factor(yw-y,yw))*tfx1);
	 add_to_rgba_sum_with_factor(&sum,bottomright,tfy*tfx2);

	 set_rgb_group_alpha(*img, sum,sum.alpha);
	 img+=THIS->xsize;
      }
   }

   pop_n_elems(args);
   THISOBJ->refs++;
   push_object(THISOBJ);
}

void image_xsize(INT32 args)
{
   pop_n_elems(args);
   if (THIS->img) push_int(THIS->xsize); else push_int(0);
}

void image_ysize(INT32 args)
{
   pop_n_elems(args);
   if (THIS->img) push_int(THIS->ysize); else push_int(0);
}

void image_gray(INT32 args)
{
   INT32 x,y,div;
   rgbl_group rgb;
   rgb_group *d,*s;
   struct object *o;
   struct image *img;

   if (args<3)
   {
      rgb.r=87;
      rgb.g=127;
      rgb.b=41;
   }
   else
      getrgbl(&rgb,0,args,"image->gray()");
   div=rgb.r+rgb.g+rgb.b;

   o=clone(image_program,0);
   img=(struct image*)o->storage;
   *img=*THIS;
   if (!(img->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize+1)))
   {
      free_object(o);
      error("Out of memory\n");
   }

   d=img->img;
   s=THIS->img;
   for (x=0; x<THIS->xsize; x++)
      for (y=0; y<THIS->ysize; y++)
      {
	 d->r=d->g=d->b=
	    testrange( ((((long)s->r)*rgb.r+
			 ((long)s->g)*rgb.g+
			 ((long)s->b)*rgb.b)/div) );
	 d++;
	 s++;
      }
   pop_n_elems(args);
   push_object(o);
}

void image_color(INT32 args)
{
   INT32 x,y;
   rgbl_group rgb;
   rgb_group *s,*d;
   struct object *o;
   struct image *img;

   if (!THIS->img) error("no image\n");
   if (args<3)
   {
      rgb.r=255;
      rgb.g=255;
      rgb.b=255;
   }
   else
      getrgbl(&rgb,0,args,"image->color()");

   o=clone(image_program,0);
   img=(struct image*)o->storage;
   *img=*THIS;
   if (!(img->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize+1)))
   {
      free_object(o);
      error("Out of memory\n");
   }

   d=img->img;
   s=THIS->img;
   for (x=0; x<THIS->xsize; x++)
      for (y=0; y<THIS->ysize; y++)
      {
	 d->r=testrange( (((long)rgb.r*s->r)/255) );
	 d->g=testrange( (((long)rgb.g*s->g)/255) );
	 d->b=testrange( (((long)rgb.b*s->b)/255) );
	 d++;
	 s++;
      }

   pop_n_elems(args);
   push_object(o);
}

void image_invert(INT32 args)
{
   INT32 x,y;
   rgb_group *s,*d;
   struct object *o;
   struct image *img;

   if (!THIS->img) error("no image\n");

   o=clone(image_program,0);
   img=(struct image*)o->storage;
   *img=*THIS;
   if (!(img->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize+1)))
   {
      free_object(o);
      error("Out of memory\n");
   }

   d=img->img;
   s=THIS->img;

   for (x=0; x<THIS->xsize; x++)
      for (y=0; y<THIS->ysize; y++)
      {
	 d->r=testrange( 255-s->r );
	 d->g=testrange( 255-s->g );
	 d->b=testrange( 255-s->b );
	 d++;
	 s++;
      }

   pop_n_elems(args);
   push_object(o);
}

void image_threshold(INT32 args)
{
   INT32 x,y;
   rgb_group *s,*d;
   struct object *o;
   struct image *img;

   if (!THIS->img) error("no image\n");

   getrgb(THIS,0,args,"image->threshold()");

   o=clone(image_program,0);
   img=(struct image*)o->storage;
   *img=*THIS;
   if (!(img->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize+1)))
   {
      free_object(o);
      error("Out of memory\n");
   }

   d=img->img;
   s=THIS->img;

   for (x=0; x<THIS->xsize; x++)
      for (y=0; y<THIS->ysize; y++)
      {
	 if (s->r>=THIS->rgb.r &&
	     s->g>=THIS->rgb.g &&
	     s->b>=THIS->rgb.b)
   	    d->r=d->g=d->b=255;
	 else
            d->r=d->g=d->b=0;

	 d++;
	 s++;
      }

   pop_n_elems(args);
   push_object(o);
}

void image_apply_matrix(INT32 args)
{
   int width,height,i,j;
   rgbl_group *matrix;
   rgb_group default_rgb;
   struct object *o;
   INT32 div;

   if (args<1 ||
       sp[-args].type!=T_ARRAY)
      error("Illegal arguments to image->apply_matrix()\n");

   if (args>3)
      if (sp[1-args].type!=T_INT ||
	  sp[2-args].type!=T_INT ||
	  sp[3-args].type!=T_INT)
	 error("Illegal argument(s) (2,3,4) to image->apply_matrix()\n");
      else
      {
	 default_rgb.r=sp[1-args].u.integer;
	 default_rgb.g=sp[1-args].u.integer;
	 default_rgb.b=sp[1-args].u.integer;
      }
   else
   {
      default_rgb.r=0;
      default_rgb.g=0;
      default_rgb.b=0;
   }
   if (args>4 
       && sp[4-args].type==T_INT)
   {
      div=sp[4-args].u.integer;
      if (!div) div=1;
   }
   else
      div=1;
   
   height=sp[-args].u.array->size;
   width=-1;
   for (i=0; i<height; i++)
   {
      struct svalue s;
      array_index_no_free(&s,sp[-args].u.array,i);
      if (s.type!=T_ARRAY) 
	 error("Illegal contents of (root) array (image->apply_matrix)\n");
      if (width==-1)
	 width=s.u.array->size;
      else
	 if (width!=s.u.array->size)
	    error("Arrays has different size (image->apply_matrix)\n");
      free_svalue(&s);
   }
   if (width==-1) width=0;

   matrix=malloc(sizeof(rgbl_group)*width*height+1);
   if (!matrix) error("Out of memory");
   
   for (i=0; i<height; i++)
   {
      struct svalue s,s2;
      array_index_no_free(&s,sp[-args].u.array,i);
      for (j=0; j<width; j++)
      {
	 array_index_no_free(&s2,s.u.array,j);
	 if (s2.type==T_ARRAY && s2.u.array->size == 3)
	 {
	    struct svalue s3;
	    array_index_no_free(&s3,s2.u.array,0);
	    if (s3.type==T_INT) matrix[j+i*width].r=s3.u.integer; 
	    else matrix[j+i*width].r=0;
	    free_svalue(&s3);
	    array_index_no_free(&s3,s2.u.array,1);
	    if (s3.type==T_INT) matrix[j+i*width].g=s3.u.integer;
	    else matrix[j+i*width].g=0;
	    free_svalue(&s3);
	    array_index_no_free(&s3,s2.u.array,2);
	    if (s3.type==T_INT) matrix[j+i*width].b=s3.u.integer; 
	    else matrix[j+i*width].b=0;
	    free_svalue(&s3);
	 }
	 else if (s2.type==T_INT)
	    matrix[j+i*width].r=matrix[j+i*width].g=
	       matrix[j+i*width].b=s2.u.integer;
	 else
	    matrix[j+i*width].r=matrix[j+i*width].g=
	       matrix[j+i*width].b=0;
	 free_svalue(&s2);
      }
      free_svalue(&s2);
   }

   o=clone(image_program,0);

   if (THIS->img)
      img_apply_matrix((struct image*)o->storage,THIS,
		       width,height,matrix,default_rgb,div);

   free(matrix);

   pop_n_elems(args);
   push_object(o);
}

void image_modify_by_intensity(INT32 args)
{
   INT32 x,y,i;
   rgbl_group rgb;
   rgb_group *list;
   rgb_group *s,*d;
   struct object *o;
   struct image *img;
   long div;

   if (!THIS->img) error("no image\n");
   if (args<5) 
      error("too few arguments to image->modify_by_intensity()\n");
   
   getrgbl(&rgb,0,args,"image->modify_by_intensity()");
   div=rgb.r+rgb.g+rgb.b;
   if (!div) div=1;

   s=malloc(sizeof(rgb_group)*(args-3)+1);
   if (!s) error("Out of memory\n");

   for (x=0; x<args-3; x++)
   {
      if (sp[3-args+x].type==T_INT)
	 s[x].r=s[x].g=s[x].b=testrange( sp[3-args+x].u.integer );
      else if (sp[3-args+x].type==T_ARRAY &&
	       sp[3-args+x].u.array->size >= 3)
      {
	 struct svalue sv;
	 array_index_no_free(&sv,sp[3-args+x].u.array,0);
	 if (sv.type==T_INT) s[x].r=testrange( sv.u.integer );
	 else s[x].r=0;
	 array_index(&sv,sp[3-args+x].u.array,1);
	 if (sv.type==T_INT) s[x].g=testrange( sv.u.integer );
	 else s[x].g=0;
	 array_index(&sv,sp[3-args+x].u.array,2);
	 if (sv.type==T_INT) s[x].b=testrange( sv.u.integer );
	 else s[x].b=0;
	 free_svalue(&sv);
      }
      else s[x].r=s[x].g=s[x].b=0;
   }

   list=malloc(sizeof(rgb_group)*256+1);
   if (!list) 
   {
      free(s);
      error("out of memory\n");
   }
   for (x=0; x<args-4; x++)
   {
      INT32 p1,p2,r;
      p1=(255L*x)/(args-4);
      p2=(255L*(x+1))/(args-4);
      r=p2-p1;
      for (y=0; y<r; y++)
      {
	 list[y+p1].r=(((long)s[x].r)*(r-y)+((long)s[x+1].r)*(y))/r;
	 list[y+p1].g=(((long)s[x].g)*(r-y)+((long)s[x+1].g)*(y))/r;
	 list[y+p1].b=(((long)s[x].b)*(r-y)+((long)s[x+1].b)*(y))/r;
      }
   }
   list[255]=s[x];
   free(s);

   o=clone(image_program,0);
   img=(struct image*)o->storage;
   *img=*THIS;
   if (!(img->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize+1)))
   {
      free_object(o);
      error("Out of memory\n");
   }

   d=img->img;
   s=THIS->img;
   for (x=0; x<THIS->xsize; x++)
      for (y=0; y<THIS->ysize; y++)
      {
	 i= testrange( ((((long)s->r)*rgb.r+
			 ((long)s->g)*rgb.g+
			 ((long)s->b)*rgb.b)/div) );
	 *d=list[i];
	 d++;
	 s++;
      }

   free(list);

   pop_n_elems(args);
   push_object(o);
}

static void image_quant(INT32 args)
{
   struct colortable *ct;
   long i;
   rgb_group *rgb;
   int colors;

   if (!THIS->img) error("no image\n");
   if (args>=1)
      if (sp[-args].type==T_INT) 
	 colors=sp[-args].u.integer;
      else
	 error("Illegal argument to image->quant()\n");
   else
      colors=256;
      
   pop_n_elems(args);

   ct=colortable_quant(THIS,colors);

   i=THIS->xsize*THIS->ysize;
   rgb=THIS->img;
   while (i--)
   {
      *rgb=ct->clut[colortable_rgb(ct,*rgb)];
      rgb++;
   }

   colortable_free(ct);

   pop_n_elems(args);
   push_int(0);
}

static void image_ccw(INT32 args)
{
   INT32 i,j;
   rgb_group *src,*dest;
   struct object *o;
   struct image *img;

   pop_n_elems(args);

   if (!THIS->img) error("no image\n");

   o=clone(image_program,0);
   img=(struct image*)o->storage;
   *img=*THIS;
   if (!(img->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize+1)))
   {
      free_object(o);
      error("Out of memory\n");
   }
   img->xsize=THIS->ysize;
   img->ysize=THIS->xsize;
   i=THIS->xsize;
   src=THIS->img+THIS->xsize-1;
   dest=img->img;
   while (i--)
   {
      j=THIS->ysize;
      while (j--) *(dest++)=*(src),src+=THIS->xsize;
      src--;
      src-=THIS->xsize*THIS->ysize;
   }

   push_object(o);
}

static void image_cw(INT32 args)
{
   INT32 i,j;
   rgb_group *src,*dest;
   struct object *o;
   struct image *img;

   pop_n_elems(args);

   if (!THIS->img) error("no image\n");

   o=clone(image_program,0);
   img=(struct image*)o->storage;
   *img=*THIS;
   if (!(img->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize+1)))
   {
      free_object(o);
      error("Out of memory\n");
   }
   img->xsize=THIS->ysize;
   img->ysize=THIS->xsize;
   i=THIS->xsize;
   src=THIS->img+THIS->xsize-1;
   dest=img->img+THIS->xsize*THIS->ysize;
   while (i--)
   {
      j=THIS->ysize;
      while (j--) *(--dest)=*(src),src+=THIS->xsize;
      src--;
      src-=THIS->xsize*THIS->ysize;
   }

   push_object(o);
}

static void image_mirrorx(INT32 args)
{
   rgb_group *src,*dest;
   struct object *o;
   struct image *img;
   INT32 i,j;

   pop_n_elems(args);

   if (!THIS->img) error("no image\n");

   o=clone(image_program,0);
   img=(struct image*)o->storage;
   *img=*THIS;
   if (!(img->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize+1)))
   {
      free_object(o);
      error("Out of memory\n");
   }
   
   i=THIS->ysize;
   src=THIS->img+THIS->xsize-1;
   dest=img->img;
   while (i--)
   {
      j=THIS->xsize;
      while (j--) *(dest++)=*(src--);
      src+=THIS->xsize*2;
   }

   push_object(o);
}

static void image_mirrory(INT32 args)
{
   rgb_group *src,*dest;
   struct object *o;
   struct image *img;
   INT32 i,j;

   pop_n_elems(args);

   if (!THIS->img) error("no image\n");

   o=clone(image_program,0);
   img=(struct image*)o->storage;
   *img=*THIS;
   if (!(img->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize+1)))
   {
      free_object(o);
      error("Out of memory\n");
   }
   
   i=THIS->ysize;
   src=THIS->img+THIS->xsize*(THIS->ysize-1);
   dest=img->img;
   while (i--)
   {
      j=THIS->xsize;
      while (j--) *(dest++)=*(src++);
      src-=THIS->xsize*2;
   }

   push_object(o);
 
}

/***************** global init etc *****************************/

#define RGB_TYPE "int|void,int|void,int|void,int|void"

void init_font_programs(void);
void exit_font(void);

void init_image_programs()
{
   int i;

   start_new_program();
   add_storage(sizeof(struct image));

   add_function("create",image_create,
		"function(int,int,"RGB_TYPE":void)",0);
   add_function("clone",image_clone,
		"function(int,int,"RGB_TYPE":object)",0);
   add_function("new",image_clone, /* alias */
		"function(int,int,"RGB_TYPE":object)",0);
   add_function("clear",image_clear,
		"function("RGB_TYPE":object)",0);
   add_function("toppm",image_toppm,
		"function(:string)",0);
   add_function("frompnm",image_frompnm,
		"function(string:object|string)",0);
   add_function("fromppm",image_frompnm,
		"function(string:object|string)",0);
   add_function("togif",image_togif,
		"function(:string)",0);
   add_function("togif_fs",image_togif_fs,
		"function(:string)",0);
   add_function("copy",image_copy,
		"function(void|int,void|int,void|int,void|int,"RGB_TYPE":object)",0);
   add_function("autocrop",image_autocrop,
		"function(:object)",0);
   add_function("scale",image_scale,
		"function(int|float,int|float|void:object)",0);

   add_function("paste",image_paste,
		"function(object,int|void,int|void:object)",0);
   add_function("paste_alpha",image_paste_alpha,
		"function(object,int,int|void,int|void:object)",0);
   add_function("paste_mask",image_paste_mask,
		"function(object,object,int|void,int|void:object)",0);
   add_function("paste_alpha_color",image_paste_alpha_color,
		"function(object,void|int,void|int,void|int,int|void,int|void:object)",0);

   add_function("setcolor",image_setcolor,
		"function(int,int,int:object)",0);
   add_function("setpixel",image_setpixel,
		"function(int,int,"RGB_TYPE":object)",0);
   add_function("line",image_line,
		"function(int,int,int,int,"RGB_TYPE":object)",0);
   add_function("circle",image_circle,
		"function(int,int,int,int,"RGB_TYPE":object)",0);
   add_function("box",image_box,
		"function(int,int,int,int,"RGB_TYPE":object)",0);
   add_function("tuned_box",image_tuned_box,
		"function(int,int,int,int,array:object)",0);

   add_function("gray",image_gray,
		"function("RGB_TYPE":object)",0);
   add_function("color",image_color,
		"function("RGB_TYPE":object)",0);
   add_function("invert",image_invert,
		"function("RGB_TYPE":object)",0);
   add_function("threshold",image_threshold,
		"function("RGB_TYPE":object)",0);

   add_function("apply_matrix",image_apply_matrix,
                "function(array(array(int|array(int))):object)",0);
   add_function("modify_by_intensity",image_modify_by_intensity,
                "function(int,int,int,int,int:object)",0);

   add_function("rotate_ccw",image_ccw,
		"function(:object)",0);
   add_function("rotate_cw",image_cw,
		"function(:object)",0);
   add_function("mirrorx",image_mirrorx,
		"function(:object)",0);
   add_function("mirrory",image_mirrory,
		"function(:object)",0);

   add_function("xsize",image_xsize,
		"function(:int)",0);
   add_function("ysize",image_ysize,
		"function(:int)",0);

   add_function("quant",image_quant,
                "function(:object)",0);
		
   set_init_callback(init_image_struct);
   set_exit_callback(exit_image_struct);
  
   image_program=end_c_program("/precompiled/image");
   
   image_program->refs++;
  
   for (i=0; i<CIRCLE_STEPS; i++) 
      circle_sin_table[i]=(INT32)4096*sin(((double)i)*2.0*3.141592653589793/(double)CIRCLE_STEPS);

   init_font_programs();
}

void init_image_efuns(void) {}

void exit_image(void) 
{
  if(image_program)
  {
    free_program(image_program);
    image_program=0;
  }
  exit_font();
}


