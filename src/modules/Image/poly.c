/*
**! module Image
**! note
**!	$Id: poly.c,v 1.2 1999/07/22 18:33:46 mirar Exp $
**! class Poly
**!
*/

#include "global.h"

RCSID("$Id: poly.c,v 1.2 1999/07/22 18:33:46 mirar Exp $");

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
#include "error.h"

#include "image.h"
#include "colortable.h"
#include "initstuff.h"

#ifdef PFLOAT
#undef PFLOAT
#endif
#define PFLOAT double

#ifdef THIS
#undef THIS
#endif
#define THIS ((struct poly *)(fp->current_storage))
#define THISOBJ (fp->current_object)

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
      PFLOAT dx,dy;
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

static void vertices_join(struct poly *this,
			  struct vertex *from,struct vertex *to)
{
   struct line *l;
   if (from->x==to->x && from->y==to->y) return; /* filter noop here */

   fprintf(stderr,"line from %g,%g - %g,%g\n",from->x,from->y,to->x,to->y);

   l=line_new(this);

   l->dx=to->x-from->x;
   l->dy=to->y-from->y;
   
   if (l->dy<0 || (l->dy==0.0 && l->dx<0)) /* down-up */
   {
      l->up=to;
      l->down=from;
   }
   else
   {
      l->up=from;
      l->down=to;
   }

   l->nextdown=l->up->firstdown; l->up->firstdown=l;
   l->nextup=l->down->firstup; l->down->firstup=l;
}

void vertices_combine(struct poly *this,
		      struct vertex *dest,struct vertex *source)
{
   struct line *l,*nl;

   fprintf(stderr,"combine %d->%d\n",source-this->vertex,dest-this->vertex);

   nl=source->firstdown;
   while ((l=nl))
   {
      l->up=dest;
      nl=l->nextdown;
      l->nextdown=dest->firstdown; 
      dest->firstdown=l;
   }

   nl=source->firstup;
   while ((l=nl))
   {
      l->down=dest;
      nl=l->nextup;
      l->nextup=dest->firstup; 
      dest->firstup=l;
   }

   source->firstdown=source->firstup=NULL;
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
	    fprintf(stderr," %d:%g,%g",
		    l->down-this->vertex,l->down->x,l->down->y);
	    l=l->nextdown;
	 }
      }
      if ((l=this->vertex[i].firstup))
      {
	 fprintf(stderr,", up");
	 while (l)
	 {
	    fprintf(stderr," %d:%g,%g",l->up-this->vertex,l->up->x,l->up->y);
	    l=l->nextup;
	 }
      }
      fprintf(stderr,"\n");
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

/*    vertices_dump(THIS,"before uniq"); */
   
   if (n)
   {
      image_sort_vertex(v,v+n-1);
      for (i=1,j=0; i<n; i++)
	 if (v[i].x==v[j].x && v[i].y==v[j].y)
	    vertices_combine(THIS,v+j,v+i);
	 else if (++j!=i) /* optimize noop */
	 {
	    v[j]=v[i];
	    v[j].firstdown=v[j].firstup=NULL;
	    vertices_combine(THIS,v+j,v+i); /* move */
	 }
   }

   THIS->nvertex=j+1;
   THIS->nallocatedvertex=n;

/*    vertices_dump(THIS,"after uniq"); */

   /* check if we have crossing lines */

   
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
	 pop_n_elems(args);
	 
	 /* fixme */

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

