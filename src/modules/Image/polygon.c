#include "global.h"
#include <config.h>

/* $Id: polygon.c,v 1.1.2.1 1998/04/29 22:10:19 mirar Exp $ */

/*
**! module Image
**! note
**!	$Id: polygon.c,v 1.1.2.1 1998/04/29 22:10:19 mirar Exp $
**! class polygon
**!
**!	This object keep polygon information,
**!	for quick polygon operations.
**!
**! see also: Image, Image.image, Image.image->polyfill
*/

#undef COLORTABLE_DEBUG
#undef COLORTABLE_REDUCE_DEBUG

RCSID("$Id: polygon.c,v 1.1.2.1 1998/04/29 22:10:19 mirar Exp $");

#include <math.h> 

#include "config.h"

#include "stralloc.h"
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "array.h"
#include "threads.h"
#include "builtin_functions.h"

#include "image.h"
#include "polygon.h"
#include "dmalloc.h"

struct program *image_polygon_program;
extern struct program *image_program;


#ifdef THIS
#undef THIS /* Needed for NT */
#endif
#define THIS ((struct polygon *)(fp->current_storage))
#define THISOBJ (fp->current_object)

/***************** init & exit *********************************/

static void free_polygon_struct(struct polygon *nct)
{
   struct vertex *v=THIS->top,*w;
   
   while (v)
   {
      struct line_list *b,*c;
      v=(w=v)->next;
      b=w->below;
      while (b) { b=(c=b)->next; free(c); }
      free(w);
   }
   THIS->top=NULL;
}

static void polygon_init_stuff(struct polygon *nct)
{
   THIS->max.x=THIS->max.y=0.0;
   THIS->min=THIS->max;

   THIS->top=NULL;
}

static void init_polygon_struct(struct object *obj)
{
   polygon_init_stuff(THIS);
}

static void exit_polygon_struct(struct object *obj)
{
   free_polygon_struct(THIS);
}

/***************** internal stuff ******************************/

static void polydebug()
{
   struct vertex *v=THIS->top;
   
   while (v)
   {
      struct line_list *b;
      fprintf(stderr," %g,%g\n",v->c.x,v->c.y);
      b=v->above;
      while (b) 
      {  
	 fprintf(stderr,"   <- %g,%g\n",b->above->c.x,b->above->c.y);
	 b=b->next; 
      }
      b=v->below;
      while (b) 
      {  
	 fprintf(stderr,"   -> %g,%g\n",b->below->c.x,b->below->c.y);
	 b=b->next; 
      }
      v=v->next;
   }
}

static INLINE struct vertex *vertex_new(PFLOAT x,PFLOAT y,struct vertex **top)
{
   struct vertex *c;

   /* fix a better algoritm some day */
   while (*top && 
	  ( (*top)->c.y<y ||
	    ((*top)->c.y==y && (*top)->c.x<=x) ) )
      if ((*top)->c.x==x && (*top)->c.y==y) return *top; /* found one */
      else top=&((*top)->next);
   
   c=malloc(sizeof(struct vertex));
   if (!c) return NULL;
   c->c.x=x;
   c->c.y=y;
   c->next=*top;
   c->above=c->below=NULL;
   c->done=0;
   *top=c;

   return c;
}

static INLINE void vertex_connect(struct vertex *above,
				  struct vertex *below)
{
   struct line_list *c,*d;
   PFLOAT diff;

   if (below==above) return;

   c = malloc(sizeof(struct line_list));
   if (!c) return;
   c->above = above; c->below = below;
   c->next = above->below;
   if (((diff = (below->c.y - above->c.y)) < 1.0e-10) &&
       (diff > -1.0e-10))
     c->dx = 1.0e10;
   else
     c->dx = (below->c.x - above->c.x)/diff;
   if (((diff = (below->c.x - above->c.x)) < 1.0e-10) &&
       (diff > -1.0e-10))
     c->dy = 1.0e10;
   else
     c->dy = (below->c.y - above->c.y)/diff;
   above->below = c;

   d = malloc(sizeof(struct line_list));
   d->above = above; d->below = below;
   d->next = below->above;
   d->dx = c->dx;
   d->dy = c->dy;
   below->above = d;
}

static INLINE void polygon_add(struct array *a,
			       int arg,
			       char* what)
{
   struct vertex *first,*last,*cur = NULL;
   int n;
   struct vertex **top;

   if (a->size&1)
   {
      free_polygon_struct(THIS);
      error("%s: Illegal argument %d, array has illegal number of elements (should be even)\n",what,arg);
      return;
   }

   for (n=0; n<a->size; n++)
      if (a->item[n].type!=T_FLOAT &&
	  a->item[n].type!=T_INT)
      {
	 free_polygon_struct(THIS);
	 error("%s: Illegal argument %d, array index %d is not int nor PFLOAT\n",what,arg,n);
	 return;
      }

   if (a->size<6) 
      return; /* no area in this polygon */

#define POINT(A,N) (((A)->item[N].type==T_FLOAT)?((A)->item[N].u.float_number):((PFLOAT)((A)->item[N].u.integer)))

   top=&(THIS->top);
   
   THREADS_ALLOW();

   last=first=vertex_new(POINT(a,0),POINT(a,1),top);

   if (!last) return;

   for (n=2; n+1<a->size; n+=2)
   {
      cur=vertex_new(POINT(a,n),POINT(a,n+1),top);
      if (!cur) break;
      if (cur->c.y<last->c.y)
	 vertex_connect(cur,last);
      else if (cur->c.y>last->c.y)
	 vertex_connect(last,cur);
      else if (cur->c.x<last->c.x)
	 vertex_connect(cur,last);
      else
	 vertex_connect(last,cur);

      last=cur;
   }

   THREADS_DISALLOW();

   if (n+1<a->size)
   {
      free_polygon_struct(THIS);
      error("%s: out of memory\n",what);
      return;
   }

#undef POINT

   if (cur->c.y<first->c.y)
      vertex_connect(cur,first);
   else if (cur->c.y>first->c.y)
      vertex_connect(first,cur);
   else if (cur->c.x<first->c.x)
      vertex_connect(cur,first);
   else
      vertex_connect(first,cur);

   return;
}

/***************** called stuff ********************************/

static void image_polygon_create(INT32 args)
{
   while (args)
   {
      if (sp[-1].type!=T_ARRAY)
      {
	 free_polygon_struct(THIS);
	 error("Image.polygon(): Illegal argument %d, expected array\n",
	       args);
      }
      polygon_add(sp[-1].u.array, args, "Image.polygon()");
      args--;
      pop_stack();
   }

   polydebug();
}

/***************** global init etc *****************************/

void init_polygon_programs(void)
{
   start_new_program();
   add_storage(sizeof(struct polygon));

   set_init_callback(init_polygon_struct);
   set_exit_callback(exit_polygon_struct);

   add_function("create",image_polygon_create,
		"function(object|array(int|float) ...:void)",0);

   image_polygon_program=end_program();
   add_program_constant("polygon",image_polygon_program, 0);
}

void exit_polygon(void) 
{
  if(image_polygon_program)
  {
    free_program(image_polygon_program);
    image_polygon_program=0;
  }
}

