#include "global.h"
#include "config.h"

#include "pike_macros.h"
#include "../../error.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "threads.h"
#include "array.h"
#include "operators.h"
#include "builtin_functions.h"
#include "mapping.h"

/* ---------------------------------------------------------------- */

extern struct program *math_matrix_program;

#define FTYPE double

struct matrix_storage
{
   int xsize,ysize;
   FTYPE *m;
};

#ifdef THIS
#undef THIS /* Needed for NT */
#endif

#define THIS ((struct matrix_storage*)(fp->current_storage))
#define THISOBJ (fp->current_object)

static struct pike_string *s_array;
static struct pike_string *s__clr;
static struct pike_string *s_identity;

/* ---------------------------------------------------------------- */

static void init_matrix(struct object *o)
{
   THIS->xsize=THIS->ysize=0;
   THIS->m=NULL;
}

static void exit_matrix(struct object *o)
{
   if (THIS->m) free(THIS->m);
}

/* ---------------------------------------------------------------- */

static void matrix_create(INT32 args)
{
   int ys=0,xs=0;
   int i,j;
   FTYPE *m=NULL;

   if (!args)
      SIMPLE_TOO_FEW_ARGS_ERROR("matrix",1);

   if (THIS->m)
      bad_arg_error("Matrix", sp-args, args, 1, "", sp-args,\
		    "Has already been called.\n");
   
   if (sp[-args].type==T_ARRAY)
   {
      ys=THIS->ysize=sp[-args].u.array->size;

      if (ys<1)
	 SIMPLE_BAD_ARG_ERROR("matrix",1,"non-empty array");
      for (i=0; i<ys; i++)
      {
	 if (sp[-args].u.array->item[i].type!=T_ARRAY)
	    SIMPLE_BAD_ARG_ERROR("matrix",1,"array(array)");
	 if (i==0) 
	 {
	    xs=sp[-args].u.array->item[i].u.array->size;
	    THIS->m=m=malloc(sizeof(FTYPE)*xs*ys);
	    if (!m)
	       SIMPLE_OUT_OF_MEMORY_ERROR("matrix",
					  sizeof(FTYPE)*xs*ys);
	 }
	 else
	    if (xs!=sp[-args].u.array->item[i].u.array->size)
	       SIMPLE_BAD_ARG_ERROR("matrix",1,
				    "array of equal sized arrays");
	 for (j=0; j<xs; j++)
	    switch (sp[-args].u.array->item[i].u.array->item[j].type)
	    {
	       case T_INT:
		  *(m++)=(FTYPE)
		     sp[-args].u.array->item[i].u.array->item[j].u.integer;
		  break;
	       case T_FLOAT:
		  *(m++)=(FTYPE)
		     sp[-args].u.array->item[i].u.array->item[j].u.float_number;
		  break;
	       default:
	       SIMPLE_BAD_ARG_ERROR("matrix",1,
				    "array(array(int|float))");
	    }
      }
      THIS->xsize=xs;
   }
   else if (sp[-args].type==T_INT)
   {
      FTYPE z=0.0;

      if (args<2)
	 SIMPLE_TOO_FEW_ARGS_ERROR("matrix",2);
      if (sp[1-args].type!=T_INT)
	 SIMPLE_BAD_ARG_ERROR("matrix",2,"int");

      if ((THIS->xsize=xs=sp[-args].u.integer)<=0)
	 SIMPLE_BAD_ARG_ERROR("matrix",1,"int > 0");
      if ((THIS->ysize=ys=sp[1-args].u.integer)<=0)
	 SIMPLE_BAD_ARG_ERROR("matrix",2,"int > 0");

      THIS->m=m=malloc(sizeof(FTYPE)*xs*ys);
      if (!m)
	 SIMPLE_OUT_OF_MEMORY_ERROR("matrix",
				    sizeof(FTYPE)*xs*ys);
      
      if (args>2)
	 if (sp[2-args].type==T_INT)
	    z=(FTYPE)sp[2-args].u.integer;
	 else if (sp[2-args].type==T_FLOAT)
	    z=(FTYPE)sp[2-args].u.float_number;
	 else if (sp[2-args].type==T_STRING)
	 {
	    if (sp[2-args].u.string==s__clr)
	    {
	       /* internal call: don't care */
	       MEMSET(m,0,xs*ys*sizeof(FTYPE));
	       goto done_made;
	    }
	    else if (sp[2-args].u.string==s_identity)
	    {
	       pop_n_elems(args-2); /* same as nothing */
	       args=2;
	    }
	    else
	       SIMPLE_BAD_ARG_ERROR("matrix",3,"\"identity\"|int|float");
	    /* insert other base matrices here */
	 }
	 else
	    SIMPLE_BAD_ARG_ERROR("matrix",3,"int|float|string");
      
      xs*=ys;
      while (xs--) *(m++)=z;

      if (args==2) /* fill base */
      {
	 xs=THIS->xsize;
	 for (i=0; i<xs && i<ys; i++)
	    THIS->m[i*(xs+1)]=1.0;
      }

done_made:
   }
   else
      SIMPLE_BAD_ARG_ERROR("matrix",1,"array|int");

   pop_n_elems(args);
   push_int(0);
}

/* ---------------------------------------------------------------- */

void matrix_cast(INT32 args)
{
   if (!THIS->m)
   {
      pop_n_elems(args);
      push_int(0);
   }

   if (args)
      if (sp[-1].type==T_STRING)
	 if (sp[-1].u.string==s_array)
	 {
	    int i,j;
	    int xs=THIS->xsize,ys=THIS->ysize;
	    FTYPE *m=THIS->m;
	    check_stack(xs+ys);
	    pop_n_elems(args);
	    for (i=0; i<ys; i++)
	    {
	       for (j=0; j<xs; j++)
		  push_float((FLOAT_TYPE)*(m++));
	       f_aggregate(xs);
	    }
	    f_aggregate(ys);
	    return;
	 }

   SIMPLE_BAD_ARG_ERROR("matrix->cast",1,"string");
}

/* --- helpers ---------------------------------------------------- */

static INLINE struct matrix_storage * _push_new_matrix(int xsize,int ysize)
{
   push_int(xsize);
   push_int(ysize);
   ref_push_string(s__clr);
   push_object(clone_object(math_matrix_program,3));
   return (struct matrix_storage*)
      get_storage(sp[-1].u.object,math_matrix_program);
}

/* --- real math stuff --------------------------------------------- */

static void matrix_transpose(INT32 args)
{
   struct matrix_storage *mx;
   int x,y,xs,ys;
   FTYPE *s,*d;

   pop_n_elems(args);
   mx=_push_new_matrix(THIS->ysize,THIS->xsize);

   ys=THIS->ysize;
   xs=THIS->xsize;
   s=THIS->m;
   d=mx->m;

   y=xs;
   while (y--)
   {
      x=ys;
      while (x--)
	 *(d++)=*s,s+=xs;
      s-=xs*ys-1;
   }
}

static void matrix_add(INT32 args)
{
   struct matrix_storage *mx=NULL;
   struct matrix_storage *dmx;
   int n;
   FTYPE *s1,*s2,*d;

   if (args<1)
      SIMPLE_TOO_FEW_ARGS_ERROR("matrix->`+",1);

   if (sp[-1].type!=T_OBJECT ||
       !((mx=(struct matrix_storage*)
	  get_storage(sp[-1].u.object,math_matrix_program))))
      SIMPLE_BAD_ARG_ERROR("matrix->`+",1,"object(Math.Matrix)");

   if (mx->xsize != THIS->xsize ||
       mx->ysize != THIS->ysize)
      math_error("Matrix->`+",sp-args,args,0,
		 "Can't add matrices of different size");

   pop_n_elems(args-1); /* shouldn't be needed */
   
   dmx=_push_new_matrix(mx->xsize,mx->ysize);

   s1=THIS->m;
   s2=mx->m;
   d=dmx->m;
   n=mx->xsize*mx->ysize;
   while (n--)
      *(d++)=*(s1++)+*(s2++);

   stack_swap();
   pop_stack();
}

static void matrix_sub(INT32 args)
{
   struct matrix_storage *mx=NULL;
   struct matrix_storage *dmx;
   int n;
   FTYPE *s1,*s2=NULL,*d;

   if (args) 
   {
      if (sp[-1].type!=T_OBJECT ||
	  !((mx=(struct matrix_storage*)
	     get_storage(sp[-1].u.object,math_matrix_program))))
	 SIMPLE_BAD_ARG_ERROR("matrix->`-",1,"object(Math.Matrix)");

      if (mx->xsize != THIS->xsize ||
	  mx->ysize != THIS->ysize)
	 math_error("Matrix->`-",sp-args,args,0,
		    "Can't add matrices of different size");

      pop_n_elems(args-1); /* shouldn't be needed */

      s2=mx->m;
   }
   
   dmx=_push_new_matrix(THIS->xsize,THIS->ysize);

   s1=THIS->m;
   d=dmx->m;
   n=THIS->xsize*THIS->ysize;

   if (s2)
   {
      while (n--)
	 *(d++)=*(s1++)-*(s2++);
      stack_swap();
      pop_stack();
   }
   else
      while (n--)
	 *(d++)=-*(s1++);
}

static void matrix_mult(INT32 args)
{
   struct matrix_storage *mx=NULL;
   struct matrix_storage *dmx;
   int n,i,j,k,m,p;
   FTYPE *s1,*s2,*d,*st;
   FTYPE z;

   if (args<1)
      SIMPLE_TOO_FEW_ARGS_ERROR("matrix->`*",1);

   pop_n_elems(args-1); /* shouldn't be needed */

   if (sp[-1].type==T_INT)
   {
      z=(FTYPE)sp[-1].u.integer;
      goto scalar_mult;
   }
   else if (sp[-1].type==T_FLOAT)
   {
      z=(FTYPE)sp[-1].u.float_number;
scalar_mult:

      dmx=_push_new_matrix(THIS->xsize,THIS->ysize);
      
      s1=THIS->m;
      d=dmx->m;
      n=THIS->xsize*THIS->ysize;
      while (n--)
	 *(d++)=*(s1++)*z;

      stack_swap();
      pop_stack();
      return;
   }
	 
   if (sp[-1].type!=T_OBJECT ||
       !((mx=(struct matrix_storage*)
	  get_storage(sp[-1].u.object,math_matrix_program))))
      SIMPLE_BAD_ARG_ERROR("matrix->`*",1,"object(Math.Matrix)");

   if (mx->xsize != THIS->ysize)
      math_error("Matrix->`*",sp-args,args,0,
		 "Incompatible matrices");

   m=THIS->xsize;
   n=THIS->ysize; /* == mx->xsize */
   p=mx->ysize;

   dmx=_push_new_matrix(m,p);

   s1=THIS->m;
   s2=mx->m;
   d=dmx->m;
   for (k=0; k<p; k++)
      for (i=0; i<m; i++)
      {
	 z=0.0;
	 st=s2+k*n;
	 for (j=0; j<n; j++)
	    z+=s1[i+j*m]**(st++);
	 *(d++)=z;
      }

   stack_swap();
   pop_stack();
}

/* ---------------------------------------------------------------- */

void init_math_matrix()
{
   MAKE_CONSTANT_SHARED_STRING(s_array,"array");
   MAKE_CONSTANT_SHARED_STRING(s__clr,"_clr");
   MAKE_CONSTANT_SHARED_STRING(s_identity,"identity");

   ADD_STORAGE(struct matrix_storage);
   
   set_init_callback(init_matrix);
   set_exit_callback(exit_matrix);

   add_function("create",matrix_create,
		"function(array(array(int|float)):object)|"
		"function(int(1..),int(1..),int|float|string|void:object)",
		0);
   
   add_function("cast",matrix_cast,
		"function(string:array(array(float)))",0);


   add_function("transpose",matrix_transpose,
		"function(:object)",0);

   add_function("add",matrix_add,
		"function(object:object)",0);
   add_function("`+",matrix_add,
		"function(object:object)",0);
   add_function("`-",matrix_sub,
		"function(object:object)",0);

   add_function("mult",matrix_mult,
		"function(object|float|int:object)",0);
   add_function("`*",matrix_mult,
		"function(object|float|int:object)",0);
   add_function("``*",matrix_mult,
		"function(object|float|int:object)",0);
   add_function("`·",matrix_mult,
		"function(object|float|int:object)",0);
   add_function("``·",matrix_mult,
		"function(object|float|int:object)",0);
}
