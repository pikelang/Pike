/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: matrix_code.h,v 1.11 2005/01/20 10:52:18 nilsson Exp $
*/

/*
 * template for Math.*Matrix
 *
 * available macros:
 *
 *   matrixX(X)          generate suitable identifiers
 *   Xmatrix(X)          with the name of the current program
 *   XmatrixY(X,Y)       in them.
 *
 *   FTYPE:     The type of the element
 *   PUSH_ELEM: Push a element on the stack
 *   PTYPE:     the pike type
 */


struct matrixX(_storage)
{
   int xsize,ysize;
   FTYPE *m;
};

#ifdef THIS
#undef THIS /* Needed for NT */
#endif

#define THIS ((struct matrixX(_storage)*)(Pike_fp->current_storage))
#define THISOBJ (Pike_fp->current_object)


/* ---------------------------------------------------------------- */

static void matrixX(_mult)( INT32 args );

static void Xmatrix(init_)(struct object *o)
{
   THIS->xsize=THIS->ysize=0;
   THIS->m=NULL;
}

static void Xmatrix(exit_)(struct object *o)
{
   if (THIS->m) free(THIS->m);
}


static void matrixX(_create)(INT32 args)
{
   int ys=0,xs=0;
   int i=0,j=0;
   FTYPE *m=NULL;

   if (!args)
      SIMPLE_TOO_FEW_ARGS_ERROR("Matrix",1);

   if (THIS->m)
      bad_arg_error("Matrix", Pike_sp-args, args, 1, "", Pike_sp-args,
		    "Has already been called.\n");
   
   if (Pike_sp[-args].type==T_ARRAY)
   {
      ys=THIS->ysize=Pike_sp[-args].u.array->size;

      if (ys<1 || Pike_sp[-args].u.array->item[0].type!=T_ARRAY)
      {
	 push_svalue(Pike_sp-args);
	 f_aggregate(THIS->ysize=ys=1); 
	 free_svalue(Pike_sp-args-1);
	 Pike_sp[-args-1]=Pike_sp[-1];
	 Pike_sp--;
      }

      for (i=0; i<ys; i++)
      {
	struct array *a;
	 if (Pike_sp[-args].u.array->item[i].type!=T_ARRAY)
	    SIMPLE_BAD_ARG_ERROR("matrix",1,"array(array)");
	 if (i==0) 
	 {
	    xs=Pike_sp[-args].u.array->item[i].u.array->size;
	    THIS->m=m=malloc(sizeof(FTYPE)*xs*ys);
	    if (!m)
	       SIMPLE_OUT_OF_MEMORY_ERROR("matrix",
					  sizeof(FTYPE)*xs*ys);
	 }
	 else
	    if (xs!=Pike_sp[-args].u.array->item[i].u.array->size)
	       SIMPLE_BAD_ARG_ERROR("matrix",1,
				    "array of equal sized arrays");

	 a = Pike_sp[-args].u.array->item[i].u.array;
	 
	 for (j=0; j<xs; j++)
	    switch (a->item[j].type)
	    {
	       case T_INT:
		  *(m++)=(FTYPE)
		     a->item[j].u.integer;
		  break;
	       case T_FLOAT:
		  *(m++)=(FTYPE)
  	            a->item[j].u.float_number;
		  break;
	      case T_OBJECT:
		{
		  INT64 x;
		  if( int64_from_bignum( &x, a->item[j].u.object ) )
		  {
		    *(m++)=(FTYPE)x;
		    break;
		  }
		}
	      default:
		SIMPLE_BAD_ARG_ERROR("matrix",1,
				     "array(array(int|float))");
	    }
      }
      THIS->xsize=xs;
   }
   else if (Pike_sp[-args].type==T_INT)
   {
      FTYPE z = DO_NOT_WARN((FTYPE)0.0);

      if (args<2)
	 SIMPLE_TOO_FEW_ARGS_ERROR("matrix",2);
      if (Pike_sp[1-args].type!=T_INT)
	 SIMPLE_BAD_ARG_ERROR("matrix",2,"int");

      if ((THIS->xsize=xs=Pike_sp[-args].u.integer)<=0)
	 SIMPLE_BAD_ARG_ERROR("matrix",1,"int > 0");
      if ((THIS->ysize=ys=Pike_sp[1-args].u.integer)<=0)
	 SIMPLE_BAD_ARG_ERROR("matrix",2,"int > 0");

      THIS->m=m=malloc(sizeof(FTYPE)*xs*ys);
      if (!m)
	 SIMPLE_OUT_OF_MEMORY_ERROR("matrix",
				    sizeof(FTYPE)*xs*ys);
      
      if (args>2) {
	 if (Pike_sp[2-args].type==T_INT)
	    z=(FTYPE)Pike_sp[2-args].u.integer;
	 else if (Pike_sp[2-args].type==T_FLOAT)
	    z=(FTYPE)Pike_sp[2-args].u.float_number;
	 else if (Pike_sp[2-args].type==T_STRING)
	 {
	    if (Pike_sp[2-args].u.string==s__clr)
	    {
	       /* internal call: don't care */
	       MEMSET(m,0,xs*ys*sizeof(FTYPE));
	       goto done_made;
	    }
	    else if (Pike_sp[2-args].u.string==s_identity)
	    {
	       pop_n_elems(args-2); /* same as nothing */
	       args=2;
	    }
	    else
	       SIMPLE_BAD_ARG_ERROR("Matrix",3,
				    "valid matrix mode (identity)");
	    /* insert other base matrices here */
	 }
	 else
	    SIMPLE_BAD_ARG_ERROR("Matrix",3,"int|float|string");
      }
      
      xs*=ys;
      while (xs--) *(m++)=z;

      if (args==2) /* fill base */
      {
	 xs=THIS->xsize;
	 for (i=0; i<xs && i<ys; i++)
	    THIS->m[i*(xs+1)] = DO_NOT_WARN((FTYPE)1.0);
      }

done_made:
      ;
   }
   else if (Pike_sp[-args].type==T_STRING)
   {
      char *dummy;
      INT_TYPE side,n;

      if (Pike_sp[-args].u.string==s_identity)
      {
	 get_all_args("Matrix",args,"%s%i",&dummy,&side);

	 THIS->xsize=THIS->ysize=side;
	 THIS->m=m=malloc(sizeof(FTYPE)*side*side);
	 if (!m) SIMPLE_OUT_OF_MEMORY_ERROR("Matrix",sizeof(FTYPE)*side*side);

  	 n=side*side;
	 while (n--) *(m++) = DO_NOT_WARN((FTYPE)0.0); 
	 n=side*side;
	 for (i=0; i<n; i+=side+1)
	    THIS->m[i] = DO_NOT_WARN((FTYPE)1.0);
      }
      else if (Pike_sp[-args].u.string==s_rotate)
      {
	 float r;
	 float x,y,z;
	 double c,s;
	 struct matrixX(_storage) *mx=NULL;

	 /* "rotate",size,degrees,x,y,z */

	 if (args>3 && Pike_sp[3-args].type==T_OBJECT &&
	     ((mx=(struct matrixX(_storage)*)
	       get_storage(Pike_sp[-1].u.object,XmatrixY(math_,_program)))))
	 {
	    if (mx->xsize*mx->ysize!=3)
	       SIMPLE_BAD_ARG_ERROR("Matrix",4,"Matrix of size 1x3 or 3x1");
	    
	    x = mx->m[0];
	    y = mx->m[1];
	    z = mx->m[2];

	    get_all_args("Matrix",args,"%s%i%F",&dummy,&side,&r);
	 }
	 else
	    get_all_args("Matrix",args,"%s%i%F%F%F%F",
			 &dummy,&side,&r,&x,&y,&z);
	 
	 if (side<2)
	    SIMPLE_BAD_ARG_ERROR("Matrix",2,"int(2..)");

	 THIS->xsize=THIS->ysize=side;
	 THIS->m=m=malloc(sizeof(FTYPE)*side*side);
	 if (!m) SIMPLE_OUT_OF_MEMORY_ERROR("Matrix",sizeof(FTYPE)*side*side);

	 n=side*side;
	 while (n--) *(m++) = DO_NOT_WARN((FTYPE)0.0);
	 for (i=3; i<side; i++)
	    THIS->m[i*(side) + i] = DO_NOT_WARN((FTYPE)1.0);
	 c = cos(r);
	 s = sin(r);

	 THIS->m[0+0*side] = DO_NOT_WARN((FTYPE)(x*x*(1-c)+c));
	 THIS->m[1+0*side] = DO_NOT_WARN((FTYPE)(x*y*(1-c)-z*s));
	 THIS->m[0+1*side] = DO_NOT_WARN((FTYPE)(y*x*(1-c)+z*s));
	 THIS->m[1+1*side] = DO_NOT_WARN((FTYPE)(y*y*(1-c)+c));
	 if (side>2)
	 {
	    THIS->m[2+0*side] = DO_NOT_WARN((FTYPE)(x*z*(1-c)+y*s));
	    THIS->m[2+1*side] = DO_NOT_WARN((FTYPE)(y*z*(1-c)-x*s));
	    THIS->m[0+2*side] = DO_NOT_WARN((FTYPE)(z*x*(1-c)-y*s));
	    THIS->m[1+2*side] = DO_NOT_WARN((FTYPE)(z*y*(1-c)+x*s));
	    THIS->m[2+2*side] = DO_NOT_WARN((FTYPE)(z*z*(1-c)+c));
	 }
      }
      else
	 SIMPLE_BAD_ARG_ERROR("Matrix",1,
			      "valid matrix mode (identity or rotate)");
   }
   else
      SIMPLE_BAD_ARG_ERROR("Matrix",1,"array|int");

   pop_n_elems(args);
   push_int(0);
}


void matrixX(_cast)(INT32 args)
{
   if (!THIS->m)
   {
      pop_n_elems(args);
      push_int(0);
   }

   if (args)
      if (Pike_sp[-1].type==T_STRING)
	 if (Pike_sp[-1].u.string==s_array)
	 {
	    int i,j;
	    int xs=THIS->xsize,ys=THIS->ysize;
	    FTYPE *m=THIS->m;
	    check_stack(DO_NOT_WARN((long)(xs+ys)));
	    pop_n_elems(args);
	    for (i=0; i<ys; i++)
	    {
	      for (j=0; j<xs; j++)
		PUSH_ELEM(*(m++));
	      f_aggregate(xs);
	    }
	    f_aggregate(ys);
	    return;
	 }

   SIMPLE_BAD_ARG_ERROR("matrix->cast",1,"string");
}


void matrixX(_vect)(INT32 args)
{
   pop_n_elems(args);

   if (!THIS->m)
   {
      f_aggregate(0);
      return;
   }
   else
   {
      int i;
      int xs=THIS->xsize,ys=THIS->ysize;
      FTYPE *m=THIS->m;
      check_stack(DO_NOT_WARN((long)(xs*ys)));
      for (i=0; i<xs * ys; i++)
	PUSH_ELEM(*(m++));
      f_aggregate(ys*xs);
      return;
   }
}


void matrixX(__sprintf)(INT32 args)
{
   FTYPE *m=THIS->m;
   INT_TYPE x,y,n=0;
   char buf[80]; /* no %6.6g is bigger */

   get_all_args("_sprintf",args,"%i",&x);

   switch (x)
   {
      case 'O':
	 if (THIS->ysize>80 || THIS->xsize>80 ||
	     THIS->xsize*THIS->ysize > 500)
	 {
	    sprintf(buf,"Math.Matrix( %d x %d elements )",
		    THIS->xsize,THIS->ysize);
	    push_text(buf);
	    stack_pop_n_elems_keep_top(args);
	    return;
	 }

	 push_constant_text("Math.Matrix( ({ ({ ");
	 n=1;
	 for (y=0; y<THIS->ysize; y++)
	 {
	    for (x=0; x<THIS->xsize; x++)
	    {
	       sprintf(buf,"%6.4g%s",(double)(*((m++))),
		       (x<THIS->xsize-1)?", ":"");
	       push_text(buf); n++;
	    }
	    if (y<THIS->ysize-1)
	       push_constant_text("}),\n                ({ "); 
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

static INLINE struct matrixX(_storage)
      *matrixX(_push_new_)(int xsize,int ysize)
{
   push_int(xsize);
   push_int(ysize);
   ref_push_string(s__clr);
   push_object(clone_object(XmatrixY(math_,_program),3));
   return (struct matrixX(_storage)*)Pike_sp[-1].u.object->storage;
}


/* --- real math stuff --------------------------------------------- */


static void matrixX(_transpose)(INT32 args)
{
   struct matrixX(_storage) *mx;
   int x,y,xs,ys;
   FTYPE *s,*d;

   pop_n_elems(args);
   mx=matrixX(_push_new_)(THIS->ysize,THIS->xsize);

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


static void matrixX(_norm)(INT32 args)
{
  double z;
  FTYPE *s;
  int n=THIS->xsize*THIS->ysize;

   pop_n_elems(args);

   if (!(THIS->xsize==1 || THIS->ysize==1))
      math_error("Matrix->norm",Pike_sp-args,args,0,
		 "Cannot compute norm of non 1xn or nx1 matrices");
   
   z=0.0;
   s=THIS->m;
   while (n--)
      z+=*s**s,s++;

   push_float(DO_NOT_WARN((FLOAT_TYPE)sqrt(z)));
}

static void matrixX(_norm2)(INT32 args)
{
  double z;
  FTYPE *s;
  int n=THIS->xsize*THIS->ysize;

  pop_n_elems(args);

  if (!(THIS->xsize==1 || THIS->ysize==1))
      math_error("Matrix->norm",Pike_sp-args,args,0,
		 "Cannot compute norm of non 1xn or nx1 matrices");
   
   z=0.0;
   s=THIS->m;
   while (n--)
      z+=*s**s,s++;

   push_float(DO_NOT_WARN((FLOAT_TYPE)z));
}

static void matrixX(_normv)(INT32 args)
{
   pop_n_elems(args);
   matrix_norm(0);
   if (Pike_sp[-1].u.float_number==0.0 || Pike_sp[-1].u.float_number==-0.0)
   {
      pop_stack();
      ref_push_object(THISOBJ);
   }
   else
   {
      Pike_sp[-1].u.float_number =
	DO_NOT_WARN((FLOAT_TYPE)(1.0/Pike_sp[-1].u.float_number));
      matrixX(_mult)(1);
   }
}


static void matrixX(_add)(INT32 args)
{
   struct matrixX(_storage) *mx=NULL;
   struct matrixX(_storage) *dmx;
   int n;
   FTYPE *s1,*s2,*d;

   if (args<1)
      SIMPLE_TOO_FEW_ARGS_ERROR("matrix->`+",1);

   if (Pike_sp[-1].type!=T_OBJECT ||
       !((mx=(struct matrixX(_storage)*)
	  get_storage(Pike_sp[-1].u.object,XmatrixY(math_,_program)))))
      SIMPLE_BAD_ARG_ERROR("matrix->`+",1,"object(Math.Matrix)");

   if (mx->xsize != THIS->xsize || mx->ysize != THIS->ysize)
      math_error("Matrix->`+",Pike_sp-args,args,0,
		 "Can't add matrices of different size");

   pop_n_elems(args-1); /* shouldn't be needed */
   
   dmx=matrixX(_push_new_)(mx->xsize,mx->ysize);

   s1=THIS->m;
   s2=mx->m;
   d=dmx->m;
   n=mx->xsize*mx->ysize;
   while (n--)
      *(d++)=*(s1++)+*(s2++);

   stack_swap();
   pop_stack();
}


static void matrixX(_sub)(INT32 args)
{
   struct matrixX(_storage) *mx=NULL;
   struct matrixX(_storage) *dmx;
   int n;
   FTYPE *s1,*s2=NULL,*d;

   if (args) 
   {
      if (Pike_sp[-1].type!=T_OBJECT ||
	  !((mx=(struct matrixX(_storage)*)
	     get_storage(Pike_sp[-1].u.object,XmatrixY(math_,_program)))))
	 SIMPLE_BAD_ARG_ERROR("matrix->`-",1,"object(Math.Matrix)");

      if (mx->xsize != THIS->xsize ||
	  mx->ysize != THIS->ysize)
	 math_error("Matrix->`-",Pike_sp-args,args,0,
		    "Can't add matrices of different size");

      pop_n_elems(args-1); /* shouldn't be needed */

      s2=mx->m;
   }
   
   dmx=matrixX(_push_new_)(THIS->xsize,THIS->ysize);

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



static void matrixX(_sum)(INT32 args)
{
   FTYPE sum=(FTYPE)0;
   FTYPE *s;
   int n;

   pop_n_elems(args);

   n=THIS->xsize*THIS->ysize;
   s=THIS->m;
   while (n--)
      sum+=*(s++);
   
   PUSH_ELEM(sum);
}

static void matrixX(_max)(INT32 args)
{
  FTYPE max;
  int n;
  FTYPE *s;

   pop_n_elems(args);

   n=THIS->xsize*THIS->ysize;
   s=THIS->m;
   if (!n) math_error("Matrix->max", Pike_sp-args, args, 0,
		      "Cannot do max() from a zero-sized matrix");
   max=*(s++);
   while (--n) { if (*s>max) max=*s; s++; }
   
   PUSH_ELEM(max);
}


static void matrixX(_min)(INT32 args)
{
   FTYPE min;
   int n;
   FTYPE *s;

   pop_n_elems(args);

   n=THIS->xsize*THIS->ysize;
   s=THIS->m;
   if (!n) math_error("Matrix->min", Pike_sp-args, args, 0,
		      "Cannot do min() from a zero-sized matrix");
   min=*(s++);
   while (--n) { if (*s<min) min=*s; s++; }
   
   PUSH_ELEM(min);
}



static void matrixX(_mult)(INT32 args)
{
   struct matrixX(_storage) *mx=NULL;
   struct matrixX(_storage) *dmx;
   int n,i,j,k,m,p;
   FTYPE *s1,*s2,*d,*st;
   FTYPE z;

   if (args<1)
      SIMPLE_TOO_FEW_ARGS_ERROR("matrix->`*",1);

   pop_n_elems(args-1); /* shouldn't be needed */

   if (Pike_sp[-1].type==T_INT)
   {
      z=(FTYPE)Pike_sp[-1].u.integer;
      goto scalar_mult;
   }
   else if (Pike_sp[-1].type==T_FLOAT)
   {
      z=(FTYPE)Pike_sp[-1].u.float_number;
scalar_mult:

      dmx=matrixX(_push_new_)(THIS->xsize,THIS->ysize);
      
      s1=THIS->m;
      d=dmx->m;
      n=THIS->xsize*THIS->ysize;
      while (n--)
	 *(d++)=*(s1++)*z;

      stack_swap();
      pop_stack();
      return;
   }
	 
   if (Pike_sp[-1].type!=T_OBJECT ||
       !((mx=(struct matrixX(_storage)*)
	  get_storage(Pike_sp[-1].u.object,XmatrixY(math_,_program)))))
      SIMPLE_BAD_ARG_ERROR("matrix->`*",1,"object(Math.Matrix)");

   if (mx->xsize != THIS->ysize)
      math_error("Matrix->`*",Pike_sp-args,args,0,
		 "Incompatible matrices");

   m=THIS->xsize;
   n=THIS->ysize; /* == mx->xsize */
   p=mx->ysize;

   dmx=matrixX(_push_new_)(p,m);

   s1=mx->m;
   s2=THIS->m;
   d=dmx->m;
   for (k=0; k<p; k++)
      for (i=0; i<m; i++)
      {
	 z=(FTYPE)0;
	 st=s2+k*n;
	 for (j=i; j<i+m*n; j+=m)
	    z += s1[ j ] * *(st++);
	 *(d++)=z;
      }

   stack_swap();
   pop_stack();
}


static void matrixX(_cross)(INT32 args)
{
   struct matrixX(_storage) *mx=NULL;
   struct matrixX(_storage) *dmx;
   FTYPE *a,*b,*d;

   if (args<1)
      SIMPLE_TOO_FEW_ARGS_ERROR("matrix->`×",1);

   pop_n_elems(args-1); /* shouldn't be needed */

   if (Pike_sp[-1].type!=T_OBJECT ||
       !((mx=(struct matrixX(_storage)*)
	  get_storage(Pike_sp[-1].u.object,XmatrixY(math_,_program)))))
      SIMPLE_BAD_ARG_ERROR("matrix->`×",1,"object(Math.Matrix)");

   if (mx->xsize*mx->ysize != 3 ||
       THIS->ysize*THIS->xsize != 3)
      math_error("Matrix->`×",Pike_sp-args,args,0,
		 "Matrices must both be of size 1x3 or 3x1");

   dmx=matrixX(_push_new_)(THIS->xsize,THIS->ysize);
   a=THIS->m;
   b=mx->m;
   d=dmx->m;
   
   d[0]=a[1]*b[2] - a[2]*b[1];
   d[1]=a[2]*b[0] - a[0]*b[2];
   d[2]=a[0]*b[1] - a[1]*b[0];

   stack_swap();
   pop_stack();
}

static void matrixX(_dot)(INT32 args)
{
  struct matrixX(_storage) *mx=NULL;
  int num,i;
  FTYPE res;
  FTYPE *a,*b;
  
  if (args<1)
     SIMPLE_TOO_FEW_ARGS_ERROR("matrix->`·",1);
  
  pop_n_elems(args-1); 
  
  if (Pike_sp[-1].type!=T_OBJECT ||
      !((mx=(struct matrixX(_storage)*)
	 get_storage(Pike_sp[-1].u.object,XmatrixY(math_,_program)))))
    SIMPLE_BAD_ARG_ERROR("matrix->`·",1,"object(Math.Matrix)");
  
  if(!(mx->xsize==THIS->xsize &&
       mx->ysize==THIS->ysize &&
       (mx->xsize==1 || mx->ysize==1)))
    math_error("Matrix->`·",Pike_sp-args,args,0,
	       "Matrices must be the same sizes, and one-dimensional\n");
  
  res=(FTYPE)0;
  num=THIS->xsize+THIS->ysize;
  a=THIS->m;
  b=mx->m;
  
  for(i=0;i<num;i++)
    res+=a[i]*b[i];
  
  PUSH_ELEM(res);
  
  stack_swap();
  pop_stack();
}

static void matrixX(_convolve)(INT32 args)
{
   struct matrixX(_storage) *mx=NULL;
   struct matrixX(_storage) *dmx,*amx,*bmx;
   INT32 ax,ay;
   INT32 axb,ayb;
   INT32 dxz,dyz,axz,ayz,bxz,byz;
   FTYPE *bs,*as,*d;

   if (args<1)
      SIMPLE_TOO_FEW_ARGS_ERROR("matrix->`*",1);

   if (Pike_sp[-args].type!=T_OBJECT ||
       !((bmx=(struct matrixX(_storage)*)
	  get_storage(Pike_sp[-args].u.object,XmatrixY(math_,_program)))))
      SIMPLE_BAD_ARG_ERROR("matrix->something",1,"object(Math.Matrix)");

   if (bmx->xsize==0 || bmx->ysize==0 ||
       THIS->xsize==0 || THIS->ysize==0)
      math_error("matrix->something",Pike_sp-args,args,0,
		 "source or argument matrix too small (zero size)");

   bxz=bmx->xsize;
   byz=bmx->ysize;

   amx=THIS; /* matrix a */
   axz=amx->xsize;
   ayz=amx->ysize;

   dxz=axz+bxz-1;
   dyz=ayz+byz-1;

   dmx=matrixX(_push_new_)(dxz,dyz);
   d=dmx->m; /* destination pointer */

/* matrix a source pointer: forwards */
   as=amx->m-axz*(byz-1)-(bxz-1);
/* matrix b source pointer: backwards */
   bs=bmx->m+bxz*byz-1; 

/*

   bbb         d.....
   bb#aaa   -> ......  ax=-bxz+1
     aaaa      ......  ay=-byz+1

    bbb        :d....  
    b##aa   -> ......  ax=-axz+1 +1
     aaaa      ......  

     bbb       :d....  
     ###a   -> ......  ax=0
     aaaa      ......  

      bbb      :::d..
     a###  ->  ......  ax=axz-bxz
     aaaa      ......  
...
        bbb    :::::d  ax=axz-1
     aaa#bb -> ......
     aaaa      ......
...
     aaaa      ::::::
     aaa#bb -> ::::::  ax=axz-1
        bbb    :::::d  ay=ayz-1

*/

#define DO_SOME_CONVOLVING(CHECK_X,CHECK_Y)				\
	 do								\
	 {								\
	    FTYPE *a=as;						\
	    FTYPE *b=bs;						\
	    FTYPE sum = (FTYPE)0.0;					\
	    INT32 yn=byz;						\
	    INT32 y=ay;							\
            INT32 x=0;							\
									\
	    while (yn--)						\
	    {								\
	       if (!(CHECK_Y && (y<0 || y>=ayz)))			\
	       {							\
		  INT32 xn=bxz;						\
		  if (CHECK_X) x=ax;					\
									\
		  while (xn--)						\
		  {							\
		     if (!(CHECK_X && (x<0 || x>=axz)))			\
		     {							\
/*  			fprintf(stderr, */				\
/*  				"a=%d,%d[%d]:%g b=[%d]:%g a*b=%g\n", */	\
/*  				x,y,(a-amx->m),*a, */			\
/*  				(b-bmx->m),*b, */			\
/*  				*a**b); */				\
			sum+=*a**b;					\
		     }							\
/*  		     else */						\
/*  			fprintf(stderr, */				\
/*  				"a=%d,%d[%d]:outside b=[%d]:%g\n", */	\
/*  				x,y,(a-amx->m), */			\
/*  				(b-bmx->m),*b); */			\
		     b--;						\
		     a++;						\
		     if (CHECK_X) x++;					\
		  }							\
		  a+=axz-bxz; /* skip to next line */			\
	       }							\
	       else							\
	       {							\
/*  		  fprintf(stderr,"skip y=%d\n",y); */			\
		  a+=axz;						\
		  b-=bxz;						\
	       }							\
	       if (CHECK_Y) y++;					\
	    }								\
/*  	    fprintf(stderr,"=== a=%d,%d:%g\n", */			\
/*  		    ax,ay,sum); */					\
									\
	    *(d++)=sum;							\
            as++;							\
	 }								\
	 while(0)

   ayb=ayz-byz+1; /* 0,0-axb,ayb         */
   axb=axz-bxz+1; /* doesn't need checks */

   for (ay=-byz+1; ay<0; ay++) 
   {
      for (ax=-bxz+1; ax<0; ax++)
	 DO_SOME_CONVOLVING(1,1);
      for (; ax<axb; ax++)
	 DO_SOME_CONVOLVING(0,1);
      for (; ax<axz; ax++)
	 DO_SOME_CONVOLVING(1,1);
      as-=bxz-1;
   }

   for (; ay<ayb; ay++) 
   {
      for (ax=-bxz+1; ax<0; ax++)
	 DO_SOME_CONVOLVING(1,0);
      for (; ax<axb; ax++)
	 DO_SOME_CONVOLVING(0,0);
      for (; ax<axz; ax++)
	 DO_SOME_CONVOLVING(1,0);
      as-=bxz-1;
   }

   for (; ay<ayz; ay++) 
   {
      for (ax=-bxz+1; ax<0; ax++)
	 DO_SOME_CONVOLVING(1,1);
      for (; ax<axb; ax++)
	 DO_SOME_CONVOLVING(0,1);
      for (; ax<axz; ax++)
	 DO_SOME_CONVOLVING(1,1);
      as-=bxz-1;
   }

   stack_pop_n_elems_keep_top(args);
}



/* ---------------------------------------------------------------- */

void Xmatrix(init_math_)(void)
{
#define MKSTR(X) make_shared_binary_string(X,CONSTANT_STRLEN(X))
  if( !s_array )
    s_array=MKSTR("array");
  if( !s_rotate )
    s_rotate=MKSTR("rotate");
  if( !s__clr )
    s__clr=MKSTR("clr");
  if( !s_identity )
    s_identity=MKSTR("identity");

   ADD_STORAGE(struct matrixX(_storage));
   
   set_init_callback(Xmatrix(init_));
   set_exit_callback(Xmatrix(exit_));

   ADD_FUNCTION("create",matrixX(_create),
		tOr4( tFunc(tArr(tArr(tOr(tInt,tFloat))), tVoid),
		      tFunc(tArr(tOr(tInt,tFloat)), tVoid),
		      tFuncV(tStr, tMix, tVoid),
		      tFunc(tInt1Plus tInt1Plus tOr4(tInt,tFloat,tString,tVoid), tVoid)), 0);

   ADD_FUNCTION("cast",matrixX(_cast),
		tFunc(tStr, tArr(tArr(tFloat))), 0);
   ADD_FUNCTION("vect",matrixX(_vect), tFunc(tNone,tArr(PTYPE)), 0);
   ADD_FUNCTION("_sprintf",matrixX(__sprintf), tFunc(tInt tMapping, tStr), 0);

   ADD_FUNCTION("transpose",matrixX(_transpose), tFunc(tNone, tObj), 0);
   ADD_FUNCTION("t",matrixX(_transpose), tFunc(tNone, tObj), 0);

   ADD_FUNCTION("norm",matrixX(_norm), tFunc(tNone, tFloat), 0);
   ADD_FUNCTION("norm2",matrixX(_norm2), tFunc(tNone, tFloat), 0);
   ADD_FUNCTION("normv",matrixX(_normv), tFunc(tNone, tObj), 0);

   ADD_FUNCTION("sum",matrixX(_sum), tFunc(tNone,PTYPE), 0);
   ADD_FUNCTION("max",matrixX(_max), tFunc(tNone,PTYPE), 0);
   ADD_FUNCTION("min",matrixX(_min), tFunc(tNone,PTYPE), 0);

   ADD_FUNCTION("add",matrixX(_add), tFunc(tObj, tObj), 0);
   ADD_FUNCTION("`+",matrixX(_add), tFunc(tObj, tObj), 0);
   ADD_FUNCTION("sub",matrixX(_sub), tFunc(tObj, tObj), 0);
   ADD_FUNCTION("`-",matrixX(_sub), tFunc(tObj, tObj), 0);

   ADD_FUNCTION("mult",matrixX(_mult),
		tFunc(tOr3(tObj,tFloat,tInt), tObj), 0);
   ADD_FUNCTION("`*",matrixX(_mult),
		tFunc(tOr3(tObj,tFloat,tInt), tObj), 0);
   ADD_FUNCTION("``*",matrixX(_mult),
		tFunc(tOr3(tObj,tFloat,tInt), tObj), 0);

   ADD_FUNCTION("`·",matrixX(_dot),
		tFunc(tOr3(tObj,tFloat,tInt), tObj), 0);
   ADD_FUNCTION("``·",matrixX(_dot),
		tFunc(tOr3(tObj,tFloat,tInt), tObj), 0);

   ADD_FUNCTION("dot_product",matrixX(_dot), tFunc(tObj, tObj), 0);

   ADD_FUNCTION("convolve",matrixX(_convolve), tFunc(tObj, tObj), 0);
   
   ADD_FUNCTION("cross",matrixX(_cross), tFunc(tObj, tObj), 0);
   ADD_FUNCTION("`×",matrixX(_cross), tFunc(tObj, tObj), 0);
   ADD_FUNCTION("``×",matrixX(_cross), tFunc(tObj, tObj), 0);

   Pike_compiler->new_program->flags |= 
     PROGRAM_CONSTANT |
     PROGRAM_NO_EXPLICIT_DESTRUCT ;
}

void Xmatrix(exit_math_)(void)
{
  if (s_array) {
    free_string(s_array);
    s_array = NULL;
  }
  if (s_rotate) {
    free_string(s_rotate);
    s_rotate = NULL;
  }
  if (s__clr) {
    free_string(s__clr);
    s__clr = NULL;
  }
  if (s_identity) {
    free_string(s_identity);
    s_identity = NULL;
  }
}
