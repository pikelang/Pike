/* $Id: math_matrix.c,v 1.11 1999/10/29 19:16:04 mirar Exp $ */

#include "global.h"
#include "config.h"

#include <math.h>

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
#include "module_support.h"

#include "math_module.h"

/*
**! module Math
**! class Matrix
**!
**!	This class hold Matrix capabilites,
**!	and support a range of matrix operations.
**!	
**!	It can only - for speed and simplicity - 
**!	hold floating point numbers and are only in 2 dimensions.
*/

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
static struct pike_string *s_rotate;

/* ---------------------------------------------------------------- */

static void matrix_mult(INT32 args);

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

/*
**! method void create(array(array(int|float)))
**! method void create(array(int|float))
**! method void create(int n,int m)
**! method void create(int n,int m,string type)
**! method void create(int n,int m,float|int init)
**! method void create("identity",int size)
**! method void create("rotate",int size,float rads,Matrix axis)
**! method void create("rotate",int size,float rads,float x,float y,float z)
**!
**!	This method initializes the matrix.
**!	It is illegal to create and hold an empty matrix.
**!	
**!	The normal operation is to create the matrix object
**!	with a double array, like
**!	<tt>Math.Matrix( ({({1,2}),({3,4})}) )</tt>.
**!	
**!	Another use is to create a special type of matrix,
**!	and this is told as third argument.
**!
**!	Currently there are only the "identity" type, 
**!	which gives a matrix of zeroes except in the diagonal,
**!	where the number one (1.0) is. This is the default,
**!	too.
**!
**!	The third use is to give all indices in the 
**!	matrix the same value, for instance zero or 42.
**!
**!	The forth use is some special matrixes. First the
**!	square identity matrix again, then the rotation
**!	matrix. 
*/

static void matrix_create(INT32 args)
{
   int ys=0,xs=0;
   int i=0,j=0;
   FTYPE *m=NULL;

   if (!args)
      SIMPLE_TOO_FEW_ARGS_ERROR("matrix",1);

   if (THIS->m)
      bad_arg_error("Matrix", sp-args, args, 1, "", sp-args,\
		    "Has already been called.\n");
   
   if (sp[-args].type==T_ARRAY)
   {
      ys=THIS->ysize=sp[-args].u.array->size;

      if (ys<1 || sp[-args].u.array->item[0].type!=T_ARRAY)
      {
	 push_svalue(sp-args);
	 f_aggregate(THIS->ysize=ys=1); 
	 free_svalue(sp-args-1);
	 sp[-args-1]=sp[-1];
	 sp--;
      }

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
	       SIMPLE_BAD_ARG_ERROR("matrix",3,
				    "valid matrix mode (identity)");
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
      ;
   }
   else if (sp[-args].type==T_STRING)
   {
      char *dummy;
      INT_TYPE side,n;

      if (sp[-args].u.string==s_identity)
      {
	 get_all_args("matrix",args,"%s%i",&dummy,&side);

	 THIS->xsize=THIS->ysize=side;
	 THIS->m=m=malloc(sizeof(FTYPE)*xs*ys);
	 if (!m) SIMPLE_OUT_OF_MEMORY_ERROR("matrix",sizeof(FTYPE)*side*side);

	 n=side*side;
	 while (n--) *(m++)=0.0;
	 for (n=0; i<side; i++)
	    THIS->m[i*(side+1)]=1.0;
      }
      else if (sp[-args].u.string==s_rotate)
      {
	 float r;
	 float x,y,z;
	 float c,s;
	 struct matrix_storage *mx=NULL;

	 /* "rotate",size,degrees,x,y,z */

	 if (args>3 && sp[3-args].type==T_OBJECT &&
	     ((mx=(struct matrix_storage*)
	       get_storage(sp[-1].u.object,math_matrix_program))))
	 {
	    if (mx->xsize*mx->ysize!=3)
	       SIMPLE_BAD_ARG_ERROR("matrix",4,"Matrix of size 1x3 or 3x1");
	    
	    x=mx->m[0];
	    y=mx->m[1];
	    z=mx->m[2];

	    get_all_args("matrix",args,"%s%i%F",&dummy,&side,&r);
	 }
	 else
	    get_all_args("matrix",args,"%s%i%F%F%F%F",
			 &dummy,&side,&r,&x,&y,&z);
	 
	 if (side<2)
	    SIMPLE_BAD_ARG_ERROR("matrix",2,"int(2..)");

	 THIS->xsize=THIS->ysize=side;
	 THIS->m=m=malloc(sizeof(FTYPE)*side*side);
	 if (!m) SIMPLE_OUT_OF_MEMORY_ERROR("matrix",sizeof(FTYPE)*side*side);

	 n=side*side;
	 while (n--) *(m++)=0.0;
	 for (n=3; i<side; i++)
	    THIS->m[i*(side+1)]=1.0;
	 c=cos(r);
	 s=sin(r);

	 THIS->m[0+0*side]=x*x*(1-c)+c;
	 THIS->m[1+0*side]=x*y*(1-c)-z*s;
	 THIS->m[0+1*side]=y*x*(1-c)+z*s;
	 THIS->m[1+1*side]=y*y*(1-c)+c;
	 if (side>2)
	 {
	    THIS->m[2+0*side]=x*z*(1-c)+y*s;
	    THIS->m[2+1*side]=y*z*(1-c)-x*s;
	    THIS->m[0+2*side]=z*x*(1-c)-y*s;
	    THIS->m[1+2*side]=z*y*(1-c)+x*s;
	    THIS->m[2+2*side]=z*z*(1-c)+c;
	 }
      }
      else
	 SIMPLE_BAD_ARG_ERROR("matrix",1,
			      "valid matrix mode (identity or rotate)");
   }
   else
      SIMPLE_BAD_ARG_ERROR("matrix",1,"array|int");

   pop_n_elems(args);
   push_int(0);
}

/* ---------------------------------------------------------------- */

/*
**! method array(array(float)) cast(string to_what)
**! method array(array(float)) cast(string to_what)
**! 	This is to be able to get the matrix values.
**!	<tt>(array)</tt> gives back a double array of floats.
**!	<tt>m->vect()</tt> gives back an array of floats.
*/

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

void matrix_vect(INT32 args)
{
   pop_n_elems(args);

   if (!THIS->m)
   {
      pop_n_elems(args);
      f_aggregate(0);
      return;
   }
   else
   {
      int i,j;
      int xs=THIS->xsize,ys=THIS->ysize;
      FTYPE *m=THIS->m;
      check_stack(xs+ys);
      pop_n_elems(args);
      for (i=0; i<ys; i++)
	 for (j=0; j<xs; j++)
	    push_float((FLOAT_TYPE)*(m++));
      f_aggregate(ys*xs);
      return;
   }
}

/*
**! method array(array(float)) cast(string to_what)
**! 	This is to be able to get the matrix values.
**!	This gives back a double array of floats.
*/

void matrix__sprintf(INT32 args)
{
   FTYPE *m=THIS->m;
   int x,y,n=0;
   char buf[80]; /* no %6.6g is bigger */

   get_all_args("_sprintf",args,"%i",&x);

   switch (x)
   {
      case 'O':
	 push_constant_text("Math.Matrix( ");
	 push_constant_text("({ ({ ");
	 n=2;
	 for (y=0; y<THIS->ysize; y++)
	 {
	    for (x=0; x<THIS->xsize; x++)
	    {
	       sprintf(buf,"%6.4g%s",*(m++),
		       (x<THIS->xsize-1)?", ":"");
	       push_text(buf); n++;
	    }
	    if (y<THIS->ysize-1)
	       push_constant_text("})\n                ({ "); 
	    n++;
	 }
	 push_constant_text("}) }) )"); 
	 f_add(n);
	 stack_pop_n_elems_keep_top(args);
	 return;
   }

   pop_n_elems(args);
   push_int(0);
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

/*
**! method object transpose()
**! 	Transpose of the matrix as a new object.
*/

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

/*
**! method float norm()
**! method float norm2()
**! method object normv()
**! 	Norm of the matrix, and the square of the norm
**!	of the matrix. (The later method is because you
**!	may skip a square root sometimes.)
**!
**!	This equals |A| or sqrt( A<sub>0</sub><sup>2</sup> +
**!	A<sub>1</sub><sup>2</sup> + ... + A<sub>n</sub><sup>2</sup> ).
**!
**!	It is only usable with 1xn or nx1 matrices.
**!
**!	m->normv() is equal to m*(1.0/m->norm()),
**!	with the exception that the zero vector will still be
**!	the zero vector (no error).
*/

static void matrix_norm(INT32 args)
{
   struct matrix_storage *mx;
   FTYPE z,*s;
   int n=THIS->xsize*THIS->ysize;

   pop_n_elems(args);

   if (!(THIS->xsize==1 || THIS->ysize==1))
      math_error("Matrix->norm",sp-args,args,0,
		 "Cannot compute norm of non 1xn or nx1 matrices");
   
   z=0.0;
   s=THIS->m;
   while (n--)
      z+=*s**s,s++;

   push_float(sqrt(z));
}

static void matrix_norm2(INT32 args)
{
   struct matrix_storage *mx;
   FTYPE z,*s;
   int n=THIS->xsize*THIS->ysize;

   pop_n_elems(args);

   if (!(THIS->xsize==1 || THIS->ysize==1))
      math_error("Matrix->norm",sp-args,args,0,
		 "Cannot compute norm of non 1xn or nx1 matrices");
   
   z=0.0;
   s=THIS->m;
   while (n--)
      z+=*s**s,s++;

   push_float(z);
}

static void matrix_normv(INT32 args)
{
   pop_n_elems(args);
   matrix_norm(0);
   if (sp[-1].u.float_number==0.0 || sp[-1].u.float_number==-0.0)
   {
      pop_stack();
      ref_push_object(THISOBJ);
   }
   else
   {
      sp[-1].u.float_number=1.0/sp[-1].u.float_number;
      matrix_mult(1);
   }
}

/*
**! method object `+(object with)
**! method object ``+(object with)
**! method object add(object with)
**! 	Add this matrix to another matrix. A new matrix is returned.
**!	The matrices must have the same size.
**!
**! method object `-()
**! method object `-(object with)
**! method object ``-(object with)
**! method object sub(object with)
**!	Subtracts this matrix from another. A new matrix is returned.
**!	-<i>m</i> is equal to -1*<i>m</i>.
*/

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

/*
**! method object `*(object with)
**! method object ``*(object with)
**! method object mult(object with)
**!	Matrix multiplication.
*/

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

/*
**! method object `×(object with)
**! method object ``×(object with)
**! method object cross(object with)
**!	Matrix cross-multiplication.
*/

static void matrix_cross(INT32 args)
{
   struct matrix_storage *mx=NULL;
   struct matrix_storage *dmx;
   int n,i,j,k,m,p;
   FTYPE *a,*b,*d,*st;
   FTYPE z;

   if (args<1)
      SIMPLE_TOO_FEW_ARGS_ERROR("matrix->`×",1);

   pop_n_elems(args-1); /* shouldn't be needed */

   if (sp[-1].type!=T_OBJECT ||
       !((mx=(struct matrix_storage*)
	  get_storage(sp[-1].u.object,math_matrix_program))))
      SIMPLE_BAD_ARG_ERROR("matrix->`×",1,"object(Math.Matrix)");

   if (mx->xsize*mx->ysize != 3 ||
       THIS->ysize*THIS->xsize != 3)
      math_error("Matrix->`×",sp-args,args,0,
		 "Matrices must both be of size 1x3 or 3x1");

   dmx=_push_new_matrix(THIS->xsize,THIS->ysize);
   a=THIS->m;
   b=mx->m;
   d=dmx->m;
   
   d[0]=a[1]*b[2] - a[2]*b[1];
   d[1]=a[2]*b[0] - a[0]*b[2];
   d[2]=a[0]*b[1] - a[1]*b[0];

   stack_swap();
   pop_stack();
}


/* ---------------------------------------------------------------- */

void init_math_matrix()
{
#define MKSTR(X) make_shared_binary_string(X,CONSTANT_STRLEN(X))
   dmalloc_accept_leak( s_array=MKSTR("array") );
   dmalloc_accept_leak( s_rotate=MKSTR("rotate") );
   dmalloc_accept_leak( s__clr=MKSTR("clr") );
   dmalloc_accept_leak( s_identity=MKSTR("identity") );

   ADD_STORAGE(struct matrix_storage);
   
   set_init_callback(init_matrix);
   set_exit_callback(exit_matrix);

   add_function("create",matrix_create,
		"function(array(array(int|float)):object)|"
		"function(array(int|float):object)|"
		"function(string,mixed...:object)|"
		"function(int(1..),int(1..),int|float|string|void:object)",
		0);
   
   add_function("cast",matrix_cast,
		"function(string:array(array(float)))",0);
   add_function("vect",matrix_vect,
		"function(:array(float))",0);
   add_function("_sprintf",matrix__sprintf,
		"function(int,mapping:string)",0);

   add_function("transpose",matrix_transpose,
		"function(:object)",0);
   add_function("t",matrix_transpose,
		"function(:object)",0);

   add_function("norm",matrix_norm,
		"function(:float)",0);
   add_function("norm2",matrix_norm2,
		"function(:float)",0);
   add_function("normv",matrix_normv,
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

   add_function("cross",matrix_cross,
		"function(object:object)",0);
   add_function("`×",matrix_cross,
		"function(object:object)",0);
   add_function("``×",matrix_cross,
		"function(object:object)",0);
}
