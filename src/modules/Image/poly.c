/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/*
**! module Image
**! class Poly
**!
*/

/*
  
I have a 2d-space with a lots of lines,
where a line is a connection between two vertices (coordinates).

What is a good data structure and algorithm to see if any line crosses
another?

 */

#include "global.h"

RCSID("$Id$");

#include "image_machine.h"

#include <math.h>

#include "stralloc.h"
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "array.h"
#include "mapping.h"
#include "threads.h"
#include "builtin_functions.h"
#include "dmalloc.h"
#include "operators.h"
#include "module_support.h"
#include "opcodes.h"
#include "pike_error.h"

#include "image.h"
#include "colortable.h"
#include "initstuff.h"


#define sp Pike_sp

#ifdef PFLOAT
#undef PFLOAT
#endif
#define PFLOAT double

#ifdef THIS
#undef THIS
#endif
#define THIS ((struct poly *)(Pike_fp->current_storage))
#define THISOBJ (Pike_fp->current_object)

struct poly
{
   PFLOAT xmin,ymin,xmax,ymax;

   struct vertex
   {
      PFLOAT x,y;
      struct line *firstdown;
      struct line *firstup;
   } *vertex;
   int nvertex;
   int nallocatedvertex;

   struct line
   {
      struct vertex *up;
      struct vertex *down;
      struct line *nextup;
      struct line *nextdown;
      PFLOAT dx,dy,dxdy,dydx;
   } *line;
   int nline;
   int nallocatedline;
};

#define INITNOLINE 8

struct pike_string *str_array;

/*****************************************************************/

static void init_poly(struct object *dummy)
{
   THIS->xmin=THIS->ymin=0;
   THIS->xmax=THIS->ymax=-1;

   THIS->vertex=NULL;
   THIS->nvertex=THIS->nallocatedvertex=0;

   THIS->line=NULL;
   THIS->nline=THIS->nallocatedline=0;
}

static void exit_poly(struct object *dummy)
{
   if (THIS->vertex) free(THIS->vertex);
   if (THIS->line) free(THIS->line);
}

/*****************************************************************/
/* operations on vertices */

static struct line *line_new(struct poly *this)
{
   if (!this->nallocatedline)
   {
      this->line=(struct line*)xalloc(sizeof(struct line)*INITNOLINE);
      this->nline=0;
      this->nallocatedline=INITNOLINE;
   }
   else if (this->nallocatedline==this->nline)
   {
      int i;
      struct line *l,*lo;
      
      l=(struct line*)
	 realloc(lo=this->line,sizeof(struct line)*this->nallocatedline*2);
      if (!l) 
	 resource_error("Poly",0,0,"memory",
			this->nallocatedline*2*sizeof(struct line), 
			"Out of memory.\n");

      /* reorder */
      for (i=0; i<this->nline; i++)
      {
	 if (lo[i].nextdown) lo[i].nextdown=(lo[i].nextdown-lo)+l;
	 if (lo[i].nextup) lo[i].nextup=(lo[i].nextup-lo)+l;
      }
      for (i=0; i<this->nvertex; i++)
      {
	 if (this->vertex[i].firstdown) 
	    this->vertex[i].firstdown=(this->vertex[i].firstdown-lo)+l;
	 if (this->vertex[i].firstup) 
	    this->vertex[i].firstup=(this->vertex[i].firstup-lo)+l;
      }

      this->line=l;
      this->nallocatedline*=2;
   }

   return this->line+(this->nline++);
}

static struct line* vertices_join(struct poly *this,
				  struct vertex *from,struct vertex *to)
{
   struct line *l;
   if (from->x==to->x && from->y==to->y) return NULL; /* filter noop here */

   fprintf(stderr,"line from %g,%g - %g,%g\n",from->x,from->y,to->x,to->y);

   l=line_new(this);

   l->dx=to->x-from->x;
   l->dy=to->y-from->y;

   if (l->dy<0 || (l->dy==0.0 && l->dx<0)) /* down-up */
   {
      l->up=to;
      l->down=from;
      l->dy=-l->dy; /* always positive */
      l->dx=-l->dx; /* reverse sign */
   }
   else
   {
      l->up=from;
      l->down=to;
   }

   if (l->dx) l->dydx=l->dy/l->dx; else l->dydx=0.0;
   if (l->dy) l->dxdy=l->dx/l->dy; else l->dxdy=0.0;

   l->nextdown=l->up->firstdown; l->up->firstdown=l;
   l->nextup=l->down->firstup; l->down->firstup=l;

   return l;
}

static void vertices_renumber(struct vertex *v,int n)
{
   while (n--)
   {
      struct line *l;
      l=v->firstdown;
      while (l) { l->up=v; l=l->nextdown; }
      l=v->firstup;
      while (l) { l->down=v; l=l->nextup; }
      v++;
   }
}

void vertices_dump(struct poly *this,char *desc)
{
   int i;
   struct line *l;

   fprintf(stderr,"vertices %s\n",desc);

   for (i=0; i<this->nvertex; i++)
   {
      fprintf(stderr," %d:%g,%g",i,this->vertex[i].x,this->vertex[i].y);
      if ((l=this->vertex[i].firstdown))
      {
	 fprintf(stderr,", down");
	 while (l)
	 {
	    fprintf(stderr," %ld:%g,%g",
		    DO_NOT_WARN((long)(l->down - this->vertex)),
		    l->down->x, l->down->y);
	    if (l->up!=this->vertex+i)
	       fprintf(stderr,"(wrong up: %ld)",
		       DO_NOT_WARN((long)(l->up - this->vertex + i)));
	    l=l->nextdown;
	 }
      }
      if ((l=this->vertex[i].firstup))
      {
	 fprintf(stderr,", up");
	 while (l)
	 {
	    fprintf(stderr, " %ld:%g,%g",
		    DO_NOT_WARN((long)(l->up - this->vertex)),
		    l->up->x, l->up->y);
	    if (l->down!=this->vertex+i)
	       fprintf(stderr,"(wrong down: %ld)",
		       DO_NOT_WARN((long)(l->down - this->vertex + i)));
	    l=l->nextup;
	 }
      }
      fprintf(stderr,"\n");
   }
}

#define LINE_MAX_X(L) ( ((L)->dxdy < 0) ? (L)->up->x : (L)->down->x )
#define LINE_MIN_X(L) ( ((L)->dxdy < 0) ? (L)->down->x : (L)->up->x )

static INLINE int lines_crossing(struct line *l1,struct line *l2,
				 PFLOAT *x,PFLOAT *y)
{
   PFLOAT m;
   /* if we have two to select from, select the downmost */

   /* we now they are in the same y-section */

   /* are they in the same x-section? */
   if ( LINE_MAX_X(l1) < LINE_MIN_X(l2) ) return 0; /* no */

   fprintf(stderr,"checking for crossing %g,%g-%g,%g x %g,%g-%g,%g\n",
	   l1->up->x,l1->up->y,l1->down->x,l1->down->y,
	   l2->up->x,l2->up->y,l2->down->x,l2->down->y);
   
   if (l1->up->y==l1->down->y) /* l1 is horisontal */
   {
      fprintf(stderr,"l1 is horisontal\n");
      if (l2->down->y==l1->down->y &&
	  l2->down!=l1->down && l2->down!=l1->up)
	 { *x=l2->down->x; *y=l2->down->y; return 1; }
      if (l2->up->y==l1->down->y &&
	  l2->up!=l1->down && l2->up!=l1->up)
	 { *x=l2->up->x; *y=l2->up->y; return 1; }
      return 0; /* same line or just endpoints */
   }
   if (l2->up->y==l2->down->y) /* l2 is horisontal */
   {
      fprintf(stderr,"l2 is horisontal\n");
      if (l1->down->y==l2->down->y &&
	  l1->down!=l2->down && l1->down!=l2->up)
	 { *x=l1->down->x; *y=l1->down->y; return 1; }
      if (l1->up->y==l2->down->y &&
	  l1->up!=l2->down && l1->up!=l2->up)
	 { *x=l1->up->x; *y=l1->up->y; return 1; }
      return 0; /* same line or just endpoints */
   }
   if (l1->dxdy==l2->dxdy) /* parallell */
   {
      if (l2->dxdy!=0.0) /* vertical */
      {
	 fprintf(stderr,"parallell dxdy=%g,%g\n",l1->dxdy,l2->dxdy);

	 if ((l1->up->x-l2->up->x)/(l2->dxdy)!=
	     (l1->up->y-l2->up->y)/(l2->dydx))
	    return 0; /* but not in the same vector */
      }
      else
	 fprintf(stderr,"parallell vertical\n");

      if (l2->down->y<l1->down->y && l2->down->y>l1->up->y)
	 { *x=l2->down->x; *y=l2->down->y; return 1; }
      if (l2->up->y<l1->down->y && l2->up->y>l1->up->y)
	 { *x=l2->up->x; *y=l2->up->y; return 1; }
      return 0; /* same line or just endpoints */
   }

   /* now we know they can cross */

   m=(l1->up->x-l2->up->x+l1->dxdy*(l2->up->y-l1->up->y))/
      (l2->dxdy-l1->dxdy);
   *x=l2->up->x+l2->dxdy*m;
   *y=l2->up->y+m;
   fprintf(stderr," crossing ... m=%g at %g,%g\n",m,*x,*y);
   
   if (y[0]>l1->down->y || y[0]>l2->down->y ||
       y[0]<l1->up->y || y[0]>l2->down->y)
      return 0; /* outside one of the lines */
   if (y[0]==l1->down->y &&
       (y[0]==l2->up->y || y[0]==l2->down->y)) 
      return 0; /* endpoints */
   if (y[0]==l1->up->y &&
       (y[0]==l2->up->y || y[0]==l2->down->y)) 
      return 0; /* endpoints */

   return 1; /* cross */

/*

float m(float ax,float ay,float a2x,float a2y,
        float bx,float by,float b2x,float b2y)
{
   float Ax=(a2x-ax)/(a2y-ay);
   float Bx=(b2x-bx)/(b2y-by);

   werror("Ax=%g Bx=%g ",Ax,Bx);

   float m=(ax-bx+Ax*(by-ay))/(Bx-Ax);
   werror("m=%g (%g,%g)\n",m,bx+Bx*m,by+m);
}

m( 0.0,0.0, 1.0,1.0,  1.0,0.0,0.0,1.0 );
m( 0.0,0.0,10.0,1.0, 10.0,0.0,0.0,1.0 );
m( 0.0,0.0,10.0,1.0,  4.5,0.0,5.5,1.0 );
m( 0.0,0.0, 1.0,1.0,  1.0,0.5,1.0,1.0 );


 */
}

#define VERTEX_UNLINK_LINE(FROMV,L,DIR) 				\
        do { struct line **_l0;						\
	     _l0=&((FROMV)->first##DIR);				\
	     while (*_l0!=(L)) _l0=&(_l0[0]->next##DIR);		\
	     _l0[0]=(L)->next##DIR; } while(0)

#define VERTEX_ENLINK_LINE(DESTV,L,VDIR,LDIR) 				\
	do { (L)->next##VDIR=(DESTV)->first##VDIR;			\
	     (DESTV)->first##VDIR=(L); 					\
	     (L)->LDIR=(DESTV); } while (0)

#define VERTEX_RELINK_LINE(DESTV,SRCV,L,VDIR,LDIR) 			\
	do { VERTEX_UNLINK_LINE(SRCV,L,VDIR); 				\
             VERTEX_ENLINK_LINE(DESTV,L,VDIR,LDIR); } while (0)

#define VERTEX_MOVE_UP_LINE(DESTV,SRCV,L) \
	VERTEX_RELINK_LINE(DESTV,SRCV,L,up,down)
#define VERTEX_MOVE_DOWN_LINE(DESTV,SRCV,L) \
	VERTEX_RELINK_LINE(DESTV,SRCV,L,down,up)

static struct vertex *vertex_find_or_insert(struct poly *p,
					    PFLOAT x,PFLOAT y)
{
   int a=0,b=p->nvertex-1,j;
   while (a<b)
   {
      j=(a+b)/2;
      if (y<p->vertex[j].y) b=j-1;
      else if (y>p->vertex[j].y) a=j+1;
      else if (y==p->vertex[j].y) {
	 if (x<p->vertex[j].x) b=j-1;
	 else if (x>p->vertex[j].x) a=j+1;
	 else
	    return p->vertex+j;
      }
   }
   
   if (p->nallocatedvertex==p->nvertex)
   {
      struct vertex *v;
      v=(struct vertex*)
	 realloc(p->vertex,sizeof(struct vertex)*(p->nvertex+8));
      if (!v)
	 resource_error("Poly",0,0,"memory",
			(p->nvertex+8)*sizeof(struct vertex), 
			"Out of memory.\n");
      p->vertex=v;
      MEMMOVE(p->vertex+a+1,p->vertex+a,(p->nvertex-a)*sizeof(struct vertex));
      vertices_renumber(p->vertex+a+1,p->nvertex-a);
      vertices_renumber(p->vertex,a);
      p->nvertex++;
      p->nallocatedvertex+=8;
   }
   else
   {
      MEMMOVE(p->vertex+a+1,p->vertex+a,(p->nvertex-a)*sizeof(struct vertex));
      vertices_renumber(p->vertex+a+1,p->nvertex-a);
      p->nvertex++;
   }
   p->vertex[a].firstdown=   
      p->vertex[a].firstup=NULL;
   p->vertex[a].x=x;
   p->vertex[a].y=y;

   return p->vertex+a;
}

static void mend_crossed_lines(struct poly *p)
{
   struct line **active,*l;
   int nactive;
   int from,i,j=0;
   double f;
   int new;
   PFLOAT x,y;

   if (!p->nline) return;

   active=(struct line**)xalloc(p->nline*sizeof(struct line*));
   nactive=0;
   
   for (from=0; from<p->nvertex;)
   {
      /* add any new lines */

      f=p->vertex[from].y;
      new=nactive;
      for (i=from; p->vertex[i].y==f; i++)
      {
	 l=p->vertex[i].firstdown;
	 while (l)
	 {
	    active[nactive++]=l;
	    l=l->nextdown;
	 }
      }

      /* try all active; new..nactive-1 */

      fprintf(stderr,"check for cross new=%d nactive=%d...\n",new,nactive);

      for (;new<nactive;new++)
	 for (i=0; i<new; i++)
	    if (lines_crossing(active[i],active[new],&x,&y))
	    {
	       struct vertex *v,*v1,*v2;

	       fprintf(stderr,"cross: %g,%g\n",x,y);

	       /* make new vertex v */
	       v=vertex_find_or_insert(p,x,y);

	       if (v-p->vertex<from) 
		  Pike_error("internal error: unexpected v-p->vertex<from\n");

	       v1=active[i]->down;
	       v2=active[new]->down;

	       /* move old lines so they point down to v */
	       if (v!=v1) VERTEX_MOVE_UP_LINE(v,v1,active[i]);
	       if (v!=v2) VERTEX_MOVE_UP_LINE(v,v2,active[new]);

	       /* make new lines from v to v1, v2 */
	       if (y==f) 
		  for (j=new; j<nactive; j++) /* calc pos for new lines */
		     if (x<=active[j]->up->x) break;

	       if ( (l=vertices_join(p,v,v1)) && y==f )
	       {
		  /* insert l in active */
		  if (j!=nactive)
		     MEMMOVE(active+j+1,active+j,
			     (nactive-j)*sizeof(struct line*));
		  active[j++]=l;
		  nactive++;			
	       }
	       if ( (l=vertices_join(p,v,v2)) && y==f )
	       {
		  /* insert l in active */
		  if (j!=nactive)
		     MEMMOVE(active+j+1,active+j,
			     (nactive-j)*sizeof(struct line*));
		  active[j++]=l;
		  nactive++;			
	       }

	       vertices_dump(p,"after mend");
	    }
	    else
	       fprintf(stderr,"no cross i=%d new=%d\n",i,new);

      fprintf(stderr,"remove...\n");

      /* remove stuff we don't need anymore */

      for (i=from; p->vertex[i].y==f; i++)
      {
	 l=p->vertex[i].firstup;
	 while (l)
	 {
	    int a=0,b=nactive-1;

	    for (;;)
	    {
	       j=(a+b)/2;

	       if (active[j]==l) break;
	       if (active[j]->up->y>l->up->y ||
		   (active[j]->up->y==l->up->y &&
		    active[j]->up->x>l->up->x)) 
		  b=j-1;
	       else if (active[j]->up==l->up)
	       {
		  /* search linear on this vertex' active lines */
		  for (a=j-1; 
		       j>=0 && active[j]!=l && active[j]->up==l->up; 
		       j--);
		  if (active[a]==l) break;
		  for (j=a+1; 
		       j<nactive && active[j]!=l && active[j]->up==l->up; 
		       j++);
		  break;
	       }
	       else
		  a=j+1;
	    }

	    if (j+1<nactive)
	       MEMMOVE(active+j,active+j+1,
		       sizeof(struct line*)*(nactive-j-1));

	    nactive--;

	    l=l->nextup;
	 }
      }
      from=i;
   }
}

/*****************************************************************/

#define CMP(A,B) (((A)->y<(B)->y)?-1:((A)->y>(B)->y)?1: \
		  ((A)->x<(B)->x)?-1:((A)->x>(B)->x)?1:0)
#define TYPE struct vertex
#define ID image_sort_vertex
#include "../../fsort_template.h"
#undef CMP
#undef TYPE
#undef ID

static void image_poly_xor(INT32 args);

static void image_poly_create(INT32 args)
{
   int n=0,i,j=0;
   struct vertex *v;
   struct array *a;

   if (THIS->nvertex || THIS->nline) 
      Pike_error("Poly: create called on initialised object\n");

   /* this is to get the correct error message */
   for (i=0; i<args; i++)
      if (sp[i-args].type!=T_ARRAY) 
	 SIMPLE_BAD_ARG_ERROR("Poly",i+1,"array");

   if (args>1) /* make two poly objects and xor' them */
   {
      /* shrink and build args>1 to one poly */
      push_object(clone_object(image_poly_program,args-1));
      stack_swap();
      /* really create this object */
      image_poly_create(1); 
      /* only the above created poly (args>1) left on stack, xor with this  */
      image_poly_xor(1); 
      pop_stack(); 
      return;
   }

   if (!args) return; /* empty */

   a=sp[-1].u.array;
   v=THIS->vertex=(struct vertex*)xalloc(sizeof(struct vertex)*a->size/2);
   THIS->nvertex=0;

   /* insert all vertices */

   for (n=i=0; i<a->size-1; i+=2,n++)
   {
      if (a->item[i].type==T_INT)
	 v[n].x=(PFLOAT)a->item[i].u.integer;
      else if (a->item[i].type==T_FLOAT)
	 v[n].x=(PFLOAT)a->item[i].u.float_number;
      else
	 v[n].x=0.0;

      if (a->item[i+1].type==T_INT)
	 v[n].y=(PFLOAT)a->item[i+1].u.integer;
      else if (a->item[i+1].type==T_FLOAT)
	 v[n].y=(PFLOAT)a->item[i+1].u.float_number;
      else
	 v[n].y=0.0;

      /* clear other stuff */
      v[n].firstdown=v[n].firstup=NULL;

      if (n!=0) 
	 vertices_join(THIS,v+n-1,v+n);

      THIS->nvertex=n+1; /* needed for the join above, next loop */
   }
   if (n!=0) vertices_join(THIS,v+n-1,v);

   /* uniq them */

   vertices_dump(THIS,"before uniq"); 
   
   if (n)
   {
      struct line *l;

      /* sort */
      image_sort_vertex(v,v+n-1);

      /* join same vertices */
      for (i=1,j=0; i<n; i++)
	 if (v[i].x==v[j].x && v[i].y==v[j].y)
	 {
	    /* append to end of list */
	    if ( (l=v[j].firstdown) )
	       while (l->nextdown) l=l->nextdown;
	    l->nextdown=v[i].firstdown;

	    if ( (l=v[j].firstup) )
	       while (l->nextup) l=l->nextup;
	    l->nextup=v[i].firstup;
	 }
	 else if (++j!=i) /* optimize noop */
	    v[j]=v[i];

      vertices_renumber(v,j+1);
   }

   THIS->nvertex=j+1;
   THIS->nallocatedvertex=n;

   vertices_dump(THIS,"after uniq"); 

   /* check if we have crossing lines */

   mend_crossed_lines(THIS);

   vertices_dump(THIS,"after crossing"); 
}

/*****************************************************************/

static void image_poly_xor(INT32 args)
{
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*****************************************************************/

static void image_poly_cast(INT32 args)
{
   if (!args) 
      SIMPLE_TOO_FEW_ARGS_ERROR("Poly.cast",1);
   
   if (sp[-args].type==T_STRING)
      if (sp[-args].u.string==str_array)
      {
	 int j;
	 int n=0,ni=0,na=0;
	 unsigned char *mark;

	 pop_n_elems(args);

	 mark=(unsigned char*)xalloc(THIS->nline);
	 MEMSET(mark,0,THIS->nline);

	 vertices_dump(THIS,"try to cast"); 

	 for (j=0; j<THIS->nline; j++)
	    if (!mark[j])
	    {
	       struct line *l=THIS->line+j,*ln;
	       struct vertex *v=l->up;
	       int down; 
	       down=1;
	       for (;;)
	       {
		  push_float(DO_NOT_WARN((FLOAT_TYPE)(v->x)));
		  push_float(DO_NOT_WARN((FLOAT_TYPE)(v->y)));
		  n++;
		  ni++;
		  mark[l-THIS->line]=1;

#if 1
		  {
		     struct line *lk;

		     fprintf(stderr," %ld %ld:%g,%g - ",
			     DO_NOT_WARN((long)(l - THIS->line)),
			     DO_NOT_WARN((long)(v - THIS->vertex)),
			     v->x,v->y);
#endif

		     v=down?l->down:l->up;

#if 1
		     fprintf(stderr,"%ld:%g,%g: ",
			     DO_NOT_WARN((long)(v - THIS->vertex)),
			     v->x,v->y);

		     if ((lk=v->firstdown))
		     {
			fprintf(stderr,", down");
			while (lk)
			{
			   fprintf(stderr," %ld[%c]:%g,%g",
				   DO_NOT_WARN((long)(lk - THIS->line)),
				   mark[lk-THIS->line]?'x':' ',
				   lk->down->x,lk->down->y);
			   lk=lk->nextdown;
			}
		     }
		     if ((lk=v->firstup))
		     {
			fprintf(stderr,", up");
			while (lk)
			{
			   fprintf(stderr," %ld[%c]:%g,%g",
				   DO_NOT_WARN((long)(lk - THIS->line)),
				   mark[lk-THIS->line]?'x':' ',
				   lk->up->x,l->up->y);
			   lk=lk->nextup;
			}
		     }
		     fprintf(stderr,"\n");
		  }
#endif

		  ln=v->firstdown;
		  while (ln)
		     if (!mark[ln-THIS->line]) break;
		     else ln=ln->nextdown;
		  if (!ln) 
		  {
		     down=0;
		     ln=v->firstup;
		     while (ln)
			if (!mark[ln-THIS->line]) break;
			else ln=ln->nextup;
		     if (!ln)
		     {
			f_aggregate(n*2);
			n=0;
			na++;
			break; /* end of loop */
		     }
		  }
		  else
		     down=1;
		  l=ln;
	       }
	    }

	 free(mark);

	 if (ni!=THIS->nline) 
	    Pike_error("Poly: internal error; ni!=nline\n");

	 f_aggregate(na);

	 return;
      }
   SIMPLE_BAD_ARG_ERROR("Poly.cast",1,"string");
}

/*****************************************************************/

void init_image_poly(void)
{
   str_array=make_shared_string("array");

   ADD_STORAGE(struct poly);
   set_init_callback(init_poly);
   set_exit_callback(exit_poly);

   /* setup */
   ADD_FUNCTION("create",image_poly_create,tFuncV(tNone,tArray,tVoid),0);

   /* query */
   ADD_FUNCTION("cast",image_poly_cast,tFunc(tStr,tArray),0);
}

void exit_image_poly(void)
{
   free_string(str_array);
}
