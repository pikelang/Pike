/*
**! module Image
**! note
**!	$Id: poly.c,v 1.1 1999/07/16 11:43:50 mirar Exp $
**! class Poly
**!
*/

#include "global.h"

RCSID("$Id: poly.c,v 1.1 1999/07/16 11:43:50 mirar Exp $");

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
};

/*****************************************************************/

static void init_poly(struct object *dummy)
{
   THIS->xmin=THIS->ymin=0;
   THIS->xmax=THIS->ymax=-1;

   THIS->vertex=NULL;
   THIS->nvertex=THIS->nallocatedvertex=0;
}

static void exit_poly(struct object *dummy)
{
}

/*****************************************************************/

#define CMP(A,B) (((A)->y<(B)->y)?-1:((A)->y<(B)->y)?1: \
	          ((A)->x<(B)->x)?-1:((A)->x<(B)->x)?1:0)
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
   }
   
   if (n)
   {
      image_sort_vertex(v,v+n-1);

      for (i=0; i<n; i++)
	 fprintf(stderr,"%g,%g\n",THIS->vertex[i].x,THIS->vertex[i].y);

      for (i=j=1; i<n; i++)
	 if (v[i].x!=v[i-1].x || v[i].y!=v[i-1].y)
	    v[j++]=v[i];

      fprintf(stderr,"---\n");
   }

   THIS->nvertex=j;
   THIS->nallocatedvertex=n;

   for (i=0; i<THIS->nvertex; i++)
      fprintf(stderr,"%g,%g\n",THIS->vertex[i].x,THIS->vertex[i].y);
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
   error("unimplemented");
}

/*****************************************************************/

void init_image_poly(void)
{
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
}

