#include "global.h"

#include <unistd.h>
#include <math.h>

#include "config.h"

#include "stralloc.h"
#include "types.h"
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "array.h"
#include "threads.h"
#include "builtin_functions.h"

#include "image.h"

#define THIS ((struct image *)(fp->current_storage))
#define THISOBJ (fp->current_object)

#undef POLYDEBUG

/*
**! module Image
**! class image
*/

/*
**! method object polygone(array(int|float) ... curve)
**! 	fills an area with the current color
**!
**! returns the current object
**! arg array(int|float) curve
**! 	curve(s), <tt>({x1,y1,x2,y2,...,xn,yn})</tt>,
**! 	automatically closed.
**!
**!	If any given curve is inside another, it
**!	will make a hole.
**!
**! NOTE
**!	This function is new (april-97) and rather untested.
**! see also: box, setcolor
*/

struct vertex_list
{
   struct vertex *above,*below;
   float dx,dy;
   struct vertex_list *next;
   float xmin,xmax,yxmin,yxmax; /* temporary storage */
};

struct vertex
{
   float x,y;
   struct vertex *next;    /* total list, sorted downwards */
   struct vertex_list *below,*above; /* childs */
   int done;
};

#define VY(V,X) ((V)->above->y+(V)->dy*((X)-(V)->above->x))

struct vertex *vertex_new(float x,float y,struct vertex **top)
{
   struct vertex *c;
   while (*top && (*top)->y<y) top=&((*top)->next);
   
   if (*top && 
       (*top)->x==x && (*top)->y==y) return *top; /* found one */

   c=malloc(sizeof(struct vertex));
   c->x=x;
   c->y=y;
   c->next=*top;
   c->above=c->below=NULL;
   c->done=0;
   *top=c;

   return c;
}

static void vertex_connect(struct vertex *above,
			   struct vertex *below)
{
   struct vertex_list *c,*d;

   if (below==above) return;

   c=malloc(sizeof(struct vertex_list));
   c->above=above; c->below=below;
   c->next=above->below;
   if (below->y!=above->y)
      c->dx=(below->x-above->x)/(below->y-above->y);
   else
      c->dx=1e10;
   if (below->x!=above->x)
      c->dy=(below->y-above->y)/(below->x-above->x);
   else
      c->dy=1e10;
   above->below=c;

   d=malloc(sizeof(struct vertex_list));
   d->above=above; d->below=below;
   d->next=below->above;
   d->dx=c->dx;
   d->dy=c->dy;
   below->above=d;
}

static INLINE float vertex_xmax(struct vertex_list *v,float yp,float *ydest)
{
   if (v->dx>0.0)
      if (v->below->y>yp+1.0)
	 return v->above->x+v->dx*((*ydest=(yp+1.0))-v->above->y);
      else
	 return (*ydest=v->below->y),v->below->x;
   else if (v->dx<0.0)
      if (v->above->y<yp)
	 return v->above->x+v->dx*((*ydest=yp)-v->above->y);
      else
	 return (*ydest=v->above->y),v->above->x;
   else if (v->below->y>yp+1.0) 
      return (*ydest=yp+1.0),v->above->x;
   else
      return (*ydest=v->below->y),v->below->x;
}

static INLINE float vertex_xmin(struct vertex_list *v,float yp,float *ydest)
{
   if (v->dx<0.0)
      if (v->below->y>yp+1.0)
	 return v->above->x+v->dx*((*ydest=(yp+1.0))-v->above->y);
      else
	 return (*ydest=v->below->y),v->below->x;
   else if (v->dx>0.0)
      if (v->above->y<yp)
	 return v->above->x+v->dx*((*ydest=yp)-v->above->y);
      else
	 return (*ydest=v->above->y),v->above->x;
   else if (v->above->y<yp) 
      return (*ydest=yp),v->above->x;
   else
      return (*ydest=v->above->y),v->above->x;
}

static void add_vertices(struct vertex_list **first,
			 struct vertex_list *what,
			 float yp)
{
   struct vertex_list **ins,*c;

   while (what)
   {
      float xi,yi;
      float xw = what->above->x;
      float yw = what->above->y;

      ins=first;

#ifdef POLYDEBUG
   fprintf(stderr,"insert %g,%g - %g,%g  %g  xw==%g\n",
	  what->above->x,what->above->y,
	  what->below->x,what->below->y,what->dx,xw);
#endif


      while (*ins)
      {
	 if ((*ins)->dy==0 || what->dy==0) xi=xw;
	 else xi = (*ins)->above->x + (*ins)->dx*(yw-(*ins)->above->y);
	 yi=yw;

	 if (xw<xi && what->dx<0) break;
	 if (xw>xi && what->dx>0) break;

	 if (xw==xi && 
/*	     ((*ins)->below->x>xw || (*ins)->above->x>xw) &&*/
	     (*ins)->dx>what->dx) break;

#ifdef POLYDEBUG
	 fprintf(stderr," after %g,%g - %g,%g  %g  i=%g,%g w=%g,%g\n",
		(*ins)->above->x,(*ins)->above->y,
		(*ins)->below->x,(*ins)->below->y,(*ins)->dx,
		xi,yi,xw,yw);

#endif

	 ins=&((*ins)->next);
      }

#ifdef POLYDEBUG
      if (*ins)
	 fprintf(stderr," before %g,%g - %g,%g  %g  i=%g,%g w=%g,%g\n",
		(*ins)->above->x,(*ins)->above->y,
		(*ins)->below->x,(*ins)->below->y,(*ins)->dy,
		xi,yi,xw,yw);
#endif


      c=malloc(sizeof(struct vertex_list));
      *c=*what;
      c->next=*ins;
      *ins=c;

      what=what->next;
   }
}

static void sub_vertices(struct vertex_list **first,
			 struct vertex *below,
			 float yp)
{
   struct vertex_list **ins,*c;

   ins=first;

   while (*ins)
      if ((*ins)->below==below) 
      {
	 c=*ins;
	 *ins=(*ins)->next;
	 free(c);
      }
      else ins=&((*ins)->next);
}

static void polygone_row_fill(float *buf,
			      float xmin,float xmax)
{
   int i;
   int max;
   if (floor(xmin)==floor(xmax))
      buf[(int)floor(xmin)]+=xmax-xmin;
   else
   {
      buf[(int)floor(xmin)]+=1-(xmin-floor(xmin));
      max=floor(xmax);
      for (i=(int)floor(xmin)+1; i<max; i++) buf[i]=1.0;
      buf[(int)floor(xmax)]+=xmax-floor(xmax);
   }
}

static int polygone_row_vertices(float *buf,
				 struct vertex_list *v1,
				 struct vertex_list *v2,
				 float xmin,
				 float xmax,
				 float yp,
				 int fill)
{
   struct vertex_list *v;
   int xofill=1;
   float x;

#ifdef POLYDEBUG
   int i;
   fprintf(stderr,"aa %g..%g fill %d\n",xmin,xmax,fill);
#endif

   fill=fill?-1:1;
   
   v=v1;
   while (v)
   {
      if (v->xmin==xmin && v->yxmin==yp)
      {
	 fill=-fill;
#ifdef POLYDEBUG
	 fprintf(stderr,"aa toggle fill: %d\n",fill);
#endif
      }
      v=v->next;
   }

   if (fill<0) polygone_row_fill(buf,xmin,xmax);

   v=v1;
   while (v!=v2)
   {
#ifdef POLYDEBUG
      fprintf(stderr,"aa << %g,%g-%g,%g %g,%g-%g,%g fill=%d xofill=%d\n",
       v->above->x,v->above->y,
       v->below->x,v->below->y,
       v->xmin,v->yxmin,
       v->xmax,v->yxmax,
	     fill,xofill);
#endif

      if (xmin!=xmax && v->xmin<xmax && v->xmax>xmin) 
      {
#define CALC_AREA(FILL,X,Y1,Y2,YP) \
	    ((FILL)*(X)*( 1-0.5*((Y1)+(Y2))+(YP) ))
/*	    (fprintf(stderr,"area: %d*%g*%g y1=%g y2=%g yp=%g == %g\n", FILL,X,( 1-0.5*(Y1+Y2)+YP ),Y1,Y2,YP,((FILL)*(X)*( 1-0.5*((Y1)+(Y2))+(YP) ))), \*/

			 
	 if (floor(xmin)==floor(xmax))
	    buf[(int)floor(xmin)]+=
	       CALC_AREA(fill*xofill,(xmax-xmin), VY(v,xmin),VY(v,xmax),yp);
	 else
	 {
	    buf[(int)floor(xmin)]+=
	       CALC_AREA(xofill*fill, (1+floor(xmin)-xmin),
			 VY(v,xmin),VY(v,1+floor(xmin)), yp);

	    for (x=1+floor(xmin); x<floor(xmax); x+=1.0)
	       buf[(int)x]+=
		  CALC_AREA(fill*xofill,1.0, VY(v,x),VY(v,x+1.0),yp);
	    
	    buf[(int)floor(xmax)]+=
	       CALC_AREA(fill*xofill, xmax-floor(xmax), 
			 VY(v,xmax),VY(v,floor(xmax)),yp);
	 }
#ifdef POLYDEBUG
	 fprintf(stderr,"aa ");
	 for (i=0; i<10; i++) fprintf(stderr,"%5.3f,",buf[i]);
	 fprintf(stderr,"\n");
#endif
      }

      if (v->xmin<=xmin && v->xmax>=xmax) 
	 xofill=-xofill;

      v=v->next;
   }
#ifdef POLYDEBUG
   if (fill<0) fprintf(stderr,"aa fill is on\n");
#endif
   return fill<0;
}

static void polygone_row(struct image *img,
			 float *buf,
			 struct vertex_list *vertices,
			 float yp,
			 int *pixmin,int *pixmax)
{
   struct vertex_list *v,*v1;
   float xmax,xmin,nxmax;
   int fill=0,i;
   int ixmin,ixmax;

   xmin=1e10;
   xmax=-1e10;

   v=vertices;
   while (v)
   {
      v->xmin=vertex_xmin(v,yp,&v->yxmin);
      v->xmax=vertex_xmax(v,yp,&v->yxmax);
      if (v->xmin<xmin) xmin=v->xmin;
      if (v->xmax>xmax) xmax=v->xmax;
      v=v->next;
   }

   *pixmin=ixmin=floor(xmin);
   *pixmax=ixmax=ceil(xmax);

   if (ixmin<0)
   {
      *pixmin=ixmin=xmin=0;
      v=vertices;
      while (v)
      {
	 if (v->xmin<0 && v->yxmin==yp) fill=!fill;
	 if (v->xmax<0 && v->yxmax==yp) fill=!fill;
	 v=v->next;
      }
   }
   else if (ixmin>=img->xsize) { *pixmax=ixmin; return; } 
   if (ixmax>=img->xsize) *pixmax=ixmax=img->xsize;
   else if (ixmax<0) { *pixmax=ixmin; return; } 

   for (i=ixmin; i<ixmax; i++) buf[i]=0.0;

   v=vertices;
   nxmax=xmax=v->xmax;
   v1=v;
   
   v1=v=vertices;

#ifdef POLYDEBUG
   while (v)
      fprintf(stderr," >> %g,%g-%g,%g  %g,%g-%g,%g\n",
	     v->above->x,v->above->y,
	     v->below->x,v->below->y,
	     v->xmin,v->yxmin,
	     v->xmax,v->yxmax),v=v->next;

   v=vertices;
#endif

   for (;;)
   {
#ifdef POLYDEBUG
      fprintf(stderr,">>>again...xmin=%g xmax=%g nxmax=%g\n",
		xmin,xmax,nxmax);
#endif

      /* skip uninteresting, ends < v->xmin */
      while (v1 && v1->xmax<xmin) v1=v1->next;
      if (!v1) break;
	 
      /* get next start or end */

      v=v1;
      nxmax=0;
      xmax=1e10;
      /* known: v->xmin>=xmin */
      while (v)
      {
#ifdef POLYDEBUG
	 fprintf(stderr," ++ %g,%g-%g,%g  %g,%g-%g,%g  xmin=%g xmax=%g nxmax=%g -> ",
	     v->above->x,v->above->y,
	     v->below->x,v->below->y,
	     v->xmin,v->yxmin,
	     v->xmax,v->yxmax,xmin,xmax,nxmax);
#endif
	 if (v->xmin>xmin && v->xmin<xmax) xmax=v->xmin;
	 if (v->xmax>xmin && v->xmax<xmax) xmax=v->xmax;
	 if (v->xmax>nxmax) nxmax=v->xmax;

#ifdef POLYDEBUG
	 fprintf(stderr,"xmin=%g xmax=%g nxmax=%g\n",
		xmin,xmax,nxmax);
#endif

	 v=v->next;
      }
      
      if (xmax>nxmax) xmax=nxmax;
      if (xmin>xmax) xmax=xmin;
      if (xmax>ixmax) xmax=ixmax;
      
      if (xmin==nxmax) break;

      fill=polygone_row_vertices(buf,v1,v,xmin,xmax,yp,fill);
      
      if ((xmin=xmax)>=ixmax) break;

      while (v1 && v1->xmax<xmin) v1=v1->next;
      if (!v1) break;
      v=vertices;
      while (v) 
      {
	 if (v->xmax==xmax && v->yxmax==yp) fill=!fill;
	 v=v->next;
      }
      
      if (v && xmax==nxmax && v->xmin>nxmax) /* jump */
      {
	 xmin=v->xmin;
	 if (nxmax>=ixmax) break;
	 if (fill) 
	    if (v->xmin>ixmax)
	       polygone_row_fill(buf,nxmax,ixmax);
	    else
	       polygone_row_fill(buf,nxmax,v->xmin);
      }
   }
}

static void polygone_some(struct image *img,
			  struct vertex *top)
{
   struct vertex *tn;
   struct vertex_list *vertices,*v,*vn,*v1;
   struct vertex *next,*nextb;
   float yp;
   int i;


   rgb_group *dest=img->img,rgb=img->rgb;
   float *buf=alloca(sizeof(float)*img->xsize*2);

   if (!buf) error("out of stack, typ\n");
   for (i=0; i<img->xsize; i++) buf[i]=0.0;

   nextb=next=top;

   yp=floor(top->y);
   vertices=NULL;

   if (yp>0) dest+=((int)yp)*img->xsize;

   while (next)
   {
#ifdef POLYDEBUG
fprintf(stderr,"\n\nrow y=%g..%g\n",yp,yp+1);
#endif

/* add new vertices if any */

      while (next && next->y<=yp+1.0)
      {
	 add_vertices(&vertices,next->below,yp);
	 next=next->next;
      }

/* POLYDEBUG */

#ifdef POLYDEBUG
      fprintf(stderr,"vertices:\n");
      v1=vertices;
      while (v1)
      {
	 fprintf(stderr," (%g,%g)-(%g,%g) at ",
		v1->above->x,v1->above->y,
		v1->below->x,v1->below->y);
	 if (v1->above->y>yp)
	    fprintf(stderr,"(%g,%g), ",v1->above->x,v1->above->y);
	 else
	    fprintf(stderr,"(%g,%g), ",v1->above->x+v1->dx*(yp-v1->above->y),yp);

	 if (v1->below->y<yp+1.0)
	    fprintf(stderr,"(%g,%g)\n",v1->below->x,v1->below->y);
	 else
	    fprintf(stderr,"(%g,%g)\n",v1->above->x+v1->dx*(1+yp-v1->above->y),
		   (yp+1.0));
	 
	 v1=v1->next;
      }
#endif

/* find out what to antialias, and what to fill, and if to resort stuff */

      if (yp>=0 && yp<img->ysize)
      {
	 int xmin,xmax;

	 polygone_row(img, buf, vertices,yp, &xmin,&xmax);

#if POLYDEBUG
	 fprintf(stderr,"yp=%g dest=&%lx (&%lx..&%lx) xmin=%d xmax=%d;   ",
		 yp,(int)dest,(int)img->img,
		 (int)img->img+img->xsize*img->ysize,
		 xmin,xmax);
	 for (i=xmin; i<xmax; i++) fprintf(stderr,"%5.3f,",buf[i]);
	 fprintf(stderr,"\n");
#endif

	 for (;xmin<xmax;xmin++)
	    dest[xmin].r=(unsigned char)(dest[xmin].r*(1.0-buf[xmin])+rgb.r*buf[xmin]),
	    dest[xmin].g=(unsigned char)(dest[xmin].g*(1.0-buf[xmin])+rgb.g*buf[xmin]),
	    dest[xmin].b=(unsigned char)(dest[xmin].b*(1.0-buf[xmin])+rgb.b*buf[xmin]);
	 dest+=img->xsize;
      }

/* remove done vertices if any */

      yp+=1.0;

      while (nextb && nextb->y<=yp)
      {
	 sub_vertices(&vertices,nextb,yp);
	 nextb=nextb->next;
      }
   }
}

static INLINE void polygone_free(struct vertex *top)
{
   struct vertex_list *v,*vn;
   struct vertex *tn;

   while (top)
   {
      v=top->above;
      while (v) { vn=v->next; free(v); v=vn; }
      v=top->below;
      while (v) { vn=v->next; free(v); v=vn; }
      tn=top->next;
      free(top);
      top=tn;
   }
}

static INLINE struct vertex *polygone_begin()
{
   return NULL;
}

static INLINE struct vertex *polygone_add(struct vertex *top,
					  struct array *a,
					  int arg,
					  char* what)
{
   struct vertex *first,*last,*cur;
   int n;

   for (n=0; n<a->size; n++)
      if (a->item[n].type!=T_FLOAT &&
	  a->item[n].type!=T_INT)
      {
	 polygone_free(top);
	 error("Illegal argument %d to %s, array index %d is not int nor float\n",arg,what,n);
	 return NULL;
      }

   if (a->size<6) return; /* no polygon with less then tree corners */

#define POINT(A,N) (((A)->item[N].type==T_FLOAT)?((A)->item[N].u.float_number):((float)((A)->item[N].u.integer)))

   last=first=vertex_new(POINT(a,0),POINT(a,1),&top);

   for (n=2; n+1<a->size; n+=2)
   {
      cur=vertex_new(POINT(a,n),POINT(a,n+1),&top);
      if (cur->y<last->y)
	 vertex_connect(cur,last);
      else if (cur->y>last->y)
	 vertex_connect(last,cur);
      else if (cur->x<last->x)
	 vertex_connect(cur,last);
      else
	 vertex_connect(last,cur);

      last=cur;
   }

   if (cur->y<first->y)
      vertex_connect(cur,first);
   else if (cur->y>first->y)
      vertex_connect(first,cur);
   else if (cur->x<first->x)
      vertex_connect(cur,first);
   else
      vertex_connect(first,cur);

   return top;
}

void image_polygone(INT32 args)
{
   struct vertex *v;
   int n;

   if (!THIS->img)
      error("No image when calling image::polygone()\n");

   v=polygone_begin();

   n=args;
   while (args)
   {
      if (sp[-1].type!=T_ARRAY)
      {
	 polygone_free(v);
	 error("Illegal argument %d to image::polygone(), expected array\n",n);
      }
      v=polygone_add(v,sp[-1].u.array,n,"image::polygone()");
      n++;
      args--;
      pop_stack();
   }

   if (!v) return; /* no vertices */

   polygone_some(THIS,v);
   
   polygone_free(v);
   
   THISOBJ->refs++;
   push_object(THISOBJ);
}
