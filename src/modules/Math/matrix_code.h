/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
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
 *   PNAME:     The class name on Pike level
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

static void Xmatrix(init_)(struct object *UNUSED(o))
{
   THIS->xsize=THIS->ysize=0;
   THIS->m=NULL;
}

static void Xmatrix(exit_)(struct object *UNUSED(o))
{
   if (THIS->m) free(THIS->m);
}


static void matrixX(_create)(INT32 args)
{
   int ys=0,xs=0;
   int i=0,j=0;
   FTYPE *m=NULL;

   if (!args)
      SIMPLE_WRONG_NUM_ARGS_ERROR(PNAME,1);

   if (THIS->m)
     Pike_error("create called twice.\n");

   if (TYPEOF(Pike_sp[-args]) == T_ARRAY)
   {
      ys=THIS->ysize=Pike_sp[-args].u.array->size;

      if (ys<1 || TYPEOF(Pike_sp[-args].u.array->item[0]) != T_ARRAY)
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
	 if (TYPEOF(Pike_sp[-args].u.array->item[i]) != T_ARRAY)
	    SIMPLE_ARG_TYPE_ERROR(PNAME,1,"array(array)");
	 if (i==0)
	 {
	    xs=Pike_sp[-args].u.array->item[i].u.array->size;
	    THIS->m=m=calloc(xs*ys, sizeof(FTYPE));
	    if (!m)
	       SIMPLE_OUT_OF_MEMORY_ERROR(PNAME,
					  sizeof(FTYPE)*xs*ys);
	 }
	 else
	    if (xs!=Pike_sp[-args].u.array->item[i].u.array->size)
	       SIMPLE_ARG_TYPE_ERROR(PNAME,1,
                                     "array of equal sized arrays");

	 a = Pike_sp[-args].u.array->item[i].u.array;

	 for (j=0; j<xs; j++)
	    switch (TYPEOF(a->item[j]))
	    {
	       case T_INT:
		  *(m++) = (FTYPE)a->item[j].u.integer;
		  break;
	       case T_FLOAT:
		  *(m++) = (FTYPE)a->item[j].u.float_number;
		  break;
	      case T_OBJECT:
		{
		  INT64 x;
		  if (is_bignum_object_in_svalue(&a->item[j])) {
		    /* Use push_svalue() so that we support subtypes... */
		    push_svalue(a->item+j);
		    o_cast_to_int();
		    if (TYPEOF(Pike_sp[-1]) == T_INT) {
		      *(m++) = (FTYPE)Pike_sp[-1].u.integer;
		      pop_stack();
		      break;
		    } else if (is_bignum_object_in_svalue(&Pike_sp[-1]) &&
			       int64_from_bignum(&x, Pike_sp[-1].u.object)) {
		      *(m++) = (FTYPE)x;
		      pop_stack();
		      break;
		    }
		    pop_stack();
		  } else if (int64_from_bignum(&x, a->item[j].u.object)) {
		    *(m++) = (FTYPE)x;
		    break;
		  }
		}
		/* FALLTHRU */
	      default:
		SIMPLE_ARG_TYPE_ERROR(PNAME,1,
                                      "array(array(int|float))");
	    }
      }
      THIS->xsize=xs;
   }
   else if (TYPEOF(Pike_sp[-args]) == T_INT)
   {
      FTYPE z = (FTYPE)0.0;

      if (args<2)
	 SIMPLE_WRONG_NUM_ARGS_ERROR(PNAME,2);
      if (TYPEOF(Pike_sp[1-args]) != T_INT)
	 SIMPLE_ARG_TYPE_ERROR(PNAME,2,"int");

      if ((THIS->xsize=xs=Pike_sp[-args].u.integer)<=0)
	 SIMPLE_ARG_TYPE_ERROR(PNAME,1,"int(1..)");
      if ((THIS->ysize=ys=Pike_sp[1-args].u.integer)<=0)
	 SIMPLE_ARG_TYPE_ERROR(PNAME,2,"int(1..)");

      THIS->m=m=xcalloc(sizeof(FTYPE),xs*ys);

      if (args>2) {
	 if (TYPEOF(Pike_sp[2-args]) == T_INT)
	    z=(FTYPE)Pike_sp[2-args].u.integer;
	 else if (TYPEOF(Pike_sp[2-args]) == T_FLOAT)
	    z=(FTYPE)Pike_sp[2-args].u.float_number;
	 else if (TYPEOF(Pike_sp[2-args]) == T_STRING)
	 {
	    if (Pike_sp[2-args].u.string==s__clr)
	    {
	       /* internal call: don't care */
	       goto done_made;
	    }
	    else if (Pike_sp[2-args].u.string==s_identity)
	    {
	       pop_n_elems(args-2); /* same as nothing */
	       args=2;
	    }
	    else
	       SIMPLE_ARG_TYPE_ERROR(PNAME,3,
                                     "valid matrix mode");
	    /* insert other base matrices here */
	 }
	 else
	    SIMPLE_ARG_TYPE_ERROR(PNAME,3,"int|float|string");
      }

      xs*=ys;
      while (xs--) *(m++)=z;

      if (args==2) /* fill base */
      {
	 xs=THIS->xsize;
	 for (i=0; i<xs && i<ys; i++)
            THIS->m[i*(xs+1)] = (FTYPE)1.0;
      }

done_made:
      ;
   }
   else if (TYPEOF(Pike_sp[-args]) == T_STRING)
   {
      char *dummy;
      INT_TYPE side,n;

      if (Pike_sp[-args].u.string==s_identity)
      {
         get_all_args(NULL,args,"%c%i",&dummy,&side);

	 THIS->xsize=THIS->ysize=side;
	 THIS->m=m=calloc(side*side, sizeof(FTYPE));
	 if (!m) SIMPLE_OUT_OF_MEMORY_ERROR(PNAME,sizeof(FTYPE)*side*side);

  	 n=side*side;
         while (n--) *(m++) = (FTYPE)0.0;
	 n=side*side;
	 for (i=0; i<n; i+=side+1)
            THIS->m[i] = (FTYPE)1.0;
      }
      else if (Pike_sp[-args].u.string==s_rotate)
      {
	 FLOAT_TYPE r;
	 FLOAT_TYPE x,y,z;
	 double c,s;
	 struct matrixX(_storage) *mx=NULL;

	 /* "rotate",size,degrees,x,y,z */

	 if (args>3 && TYPEOF(Pike_sp[3-args]) == T_OBJECT &&
	     ((mx=get_storage(Pike_sp[-1].u.object,XmatrixY(math_,_program)))))
	 {
	    if (mx->xsize*mx->ysize!=3)
	       SIMPLE_ARG_TYPE_ERROR(PNAME,4,"Matrix of size 1x3 or 3x1");

	    x = mx->m[0];
	    y = mx->m[1];
	    z = mx->m[2];

            get_all_args(NULL,args,"%c%i%F",&dummy,&side,&r);
	 }
	 else
            get_all_args(NULL,args,"%c%i%F%F%F%F",
			 &dummy,&side,&r,&x,&y,&z);

	 if (side<2)
	    SIMPLE_ARG_TYPE_ERROR(PNAME,2,"int(2..)");

	 THIS->xsize=THIS->ysize=side;
	 THIS->m=m=calloc(side*side, sizeof(FTYPE));
	 if (!m) SIMPLE_OUT_OF_MEMORY_ERROR(PNAME,sizeof(FTYPE)*side*side);

	 n=side*side;
         while (n--) *(m++) = (FTYPE)0.0;
	 for (i=3; i<side; i++)
            THIS->m[i*(side) + i] = (FTYPE)1.0;
	 c = cos(r);
	 s = sin(r);

         THIS->m[0+0*side] = (FTYPE)(x*x*(1-c)+c);
         THIS->m[1+0*side] = (FTYPE)(x*y*(1-c)-z*s);
         THIS->m[0+1*side] = (FTYPE)(y*x*(1-c)+z*s);
         THIS->m[1+1*side] = (FTYPE)(y*y*(1-c)+c);
	 if (side>2)
	 {
            THIS->m[2+0*side] = (FTYPE)(x*z*(1-c)+y*s);
            THIS->m[2+1*side] = (FTYPE)(y*z*(1-c)-x*s);
            THIS->m[0+2*side] = (FTYPE)(z*x*(1-c)-y*s);
            THIS->m[1+2*side] = (FTYPE)(z*y*(1-c)+x*s);
            THIS->m[2+2*side] = (FTYPE)(z*z*(1-c)+c);
	 }
      }
      else
	 SIMPLE_ARG_TYPE_ERROR(PNAME,1,
                               "valid matrix mode (identity or rotate)");
   }
   else
      SIMPLE_ARG_TYPE_ERROR(PNAME,1,"array|int");
   pop_n_elems(args);
}


void matrixX(_cast)(INT32 args)
{
   if (!THIS->m)
   {
      pop_n_elems(args);
      push_int(0);
   }

   if( !args || TYPEOF(Pike_sp[-1]) != T_STRING )
     SIMPLE_ARG_TYPE_ERROR("cast",1,"string");

   if( Pike_sp[-1].u.string != literal_array_string )
   {
     pop_n_elems(args);
     push_undefined();
     return;
   }

   {
     int i,j;
     int xs=THIS->xsize,ys=THIS->ysize;
     FTYPE *m=THIS->m;
     check_stack((long)(xs+ys));
     pop_n_elems(args);
     for (i=0; i<ys; i++)
     {
       for (j=0; j<xs; j++)
         PUSH_ELEM(*(m++));
       f_aggregate(xs);
     }
     f_aggregate(ys);
   }
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
      check_stack((long)(xs*ys));
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

   if (!THIS->m) {
       Pike_error("Some weirdo is calling sprintf with no ->m!\n");
   }

   get_all_args(NULL,args,"%i",&x);

   switch (x)
   {
      case 'O':
	 if (THIS->ysize>80 || THIS->xsize>80 ||
	     THIS->xsize*THIS->ysize > 500)
	 {
	    sprintf(buf,"Math." PNAME "( %d x %d elements )",
		    THIS->xsize,THIS->ysize);
	    push_text(buf);
	    stack_pop_n_elems_keep_top(args);
	    return;
	 }

	 push_static_text("Math." PNAME "( ({ ({ ");
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
	       push_static_text("}),\n                ({ ");
	    n++;
	 }
	 push_static_text("}) }) )");
	 f_add(n);
	 stack_pop_n_elems_keep_top(args);
	 return;
   }

   pop_n_elems(args);
   push_int(0);
}

/* --- helpers ---------------------------------------------------- */

static inline struct matrixX(_storage)
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
      math_error("norm",args,0,
		 "Cannot compute norm of non 1xn or nx1 matrices.\n");

   z=0.0;
   s=THIS->m;
   while (n--)
      z+=*s**s,s++;

   push_float((FLOAT_TYPE)sqrt(z));
}

static void matrixX(_norm2)(INT32 args)
{
  double z;
  FTYPE *s;
  int n=THIS->xsize*THIS->ysize;

  pop_n_elems(args);

  if (!(THIS->xsize==1 || THIS->ysize==1))
      math_error("norm2",args,0,
		 "Cannot compute norm of non 1xn or nx1 matrices.\n");

   z=0.0;
   s=THIS->m;
   while (n--)
      z+=*s**s,s++;

   push_float((FLOAT_TYPE)z);
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
        (FLOAT_TYPE)(1.0/Pike_sp[-1].u.float_number);
      matrixX(_mult)(1);
   }
}


static void matrixX(_add)(INT32 args)
{
   struct matrixX(_storage) *mx=NULL;
   struct matrixX(_storage) *dmx;
   int n,i;
   FTYPE *s1,*s2,*d;

   if (args<1)
      SIMPLE_WRONG_NUM_ARGS_ERROR("`+",1);

   if (args>1) /* one add per argument */
   {
      ref_push_object(THISOBJ);
      for (i=0; i<args; i++)
      {
	 push_svalue(Pike_sp-args+i-1);
	 f_add(2);
      }
      stack_pop_n_elems_keep_top(args);
      return;
   }

   if (TYPEOF(Pike_sp[-1]) != T_OBJECT ||
       !((mx=get_storage(Pike_sp[-1].u.object,XmatrixY(math_,_program)))))
      SIMPLE_ARG_TYPE_ERROR("`+",1,"object(Math.Matrix)");

   if (mx->xsize != THIS->xsize || mx->ysize != THIS->ysize)
      math_error("`+",args,0,
		 "Cannot add matrices of different size.\n");

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
   int n,i;
   FTYPE *s1,*s2=NULL,*d;

   if (args)
   {
      if (args>1) /* one subtract per argument */
      {
	 ref_push_object(THISOBJ);
	 for (i=0; i<args; i++)
	 {
	    push_svalue(Pike_sp-args+i-1);
	    f_minus(2);
	 }
	 stack_pop_n_elems_keep_top(args);
	 return;
      }

      if (TYPEOF(Pike_sp[-1]) != T_OBJECT ||
	  !((mx=get_storage(Pike_sp[-1].u.object,XmatrixY(math_,_program)))))
	 SIMPLE_ARG_TYPE_ERROR("`-",1,"object(Math.Matrix)");

      if (mx->xsize != THIS->xsize ||
	  mx->ysize != THIS->ysize)
         math_error("`-",args,0,
		    "Cannot add matrices of different size.\n");

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
   if (!n) math_error("max", args, 0,
		      "Cannot do max() from a zero-sized matrix.\n");
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
   if (!n) math_error("min", args, 0,
		      "Cannot do min() from a zero-sized matrix.\n");
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
      SIMPLE_WRONG_NUM_ARGS_ERROR("`*",1);

   if (args>1) /* one multiply per argument */
   {
      ref_push_object(THISOBJ);
      for (i=0; i<args; i++)
      {
	 push_svalue(Pike_sp-args+i-1);
	 f_multiply(2);
      }
      stack_pop_n_elems_keep_top(args);
      return;
   }

   if (TYPEOF(Pike_sp[-1]) == T_INT)
   {
      z=(FTYPE)Pike_sp[-1].u.integer;
      goto scalar_mult;
   }
   else if (TYPEOF(Pike_sp[-1]) == T_FLOAT)
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

   if (TYPEOF(Pike_sp[-1]) != T_OBJECT ||
       !((mx=get_storage(Pike_sp[-1].u.object,XmatrixY(math_,_program)))))
      SIMPLE_ARG_TYPE_ERROR("`*",1,"object(Math.Matrix)");

   if (mx->xsize != THIS->ysize)
      math_error("`*",args,0,
		 "Incompatible matrices.\n");

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

   if (args!=1)
      SIMPLE_WRONG_NUM_ARGS_ERROR("cross",1);

   if (TYPEOF(Pike_sp[-1]) != T_OBJECT ||
       !((mx=get_storage(Pike_sp[-1].u.object,XmatrixY(math_,_program)))))
      SIMPLE_ARG_TYPE_ERROR("cross",1,"object(Math.Matrix)");

   if (mx->xsize*mx->ysize != 3 ||
       THIS->ysize*THIS->xsize != 3)
      math_error("cross",args,0,
		 "Matrices must both be of size 1x3 or 3x1.\n");

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

#ifdef HAS_MPI
extern PMOD_EXPORT struct object *mpi_clone_sentinel(MPI_Datatype, unsigned int, unsigned int,
						     unsigned int, void *, struct object *);

static void matrixX(get_sentinel)(INT32 args)
{
    struct object *sentinel = mpi_clone_sentinel(MATRIX_MPI_TYPE,
						 THIS->xsize * THIS->ysize,
						 (unsigned int)sizeof(FTYPE),
						 MATRIX_MPI_SHIFT,
						 THIS->m,
						 Pike_fp->current_object);

    pop_n_elems(args);
    push_object(sentinel);
}

static struct op_info matrixX(_obj_create)(void *sinfo)
{
    struct object *o = low_clone(XmatrixY(math_,_program));
    struct op_info info = {o, (void**)&((struct matrixX(_storage)*)get_storage(o, XmatrixY(math_,_program)))->m};

    ((struct matrixX(_storage)*)get_storage(o, XmatrixY(math_,_program)))->xsize = ((struct size_info*)sinfo)->x;
    ((struct matrixX(_storage)*)get_storage(o, XmatrixY(math_,_program)))->ysize = ((struct size_info*)sinfo)->y;

    return info;
}

/* ufun, commute, x, y */
void matrixX(_op_create)(INT32 args)
{
    struct size_info *sinfo = ALLOC_STRUCT(size_info);
    struct object *o;

    if (args < 4) {
	SIMPLE_TOO_FEW_ARGS_ERROR("Op_create_", 4);
	/* TODO: typecheck */
    }

    sinfo->x = Pike_sp[-args+2].u.integer;
    sinfo->y = Pike_sp[-args+3].u.integer;

    fprintf(stderr, "> (%d)prelen: %d * %d %d\n", args, (int)sizeof(FTYPE), sinfo->x, sinfo->y);
    o = mpi_ex_op(matrixX(_obj_create), Pike_sp-args, Pike_sp[-args+1].u.integer, sinfo, MATRIX_MPI_SHIFT, sizeof(FTYPE)*sinfo->x*sinfo->y);

    pop_n_elems(args);
    push_object(o);
}

#endif

static void matrixX(_dot)(INT32 args)
{
  struct matrixX(_storage) *mx=NULL;
  int num,i;
  FTYPE res;
  FTYPE *a,*b;

  if (args!=1)
     SIMPLE_WRONG_NUM_ARGS_ERROR("dot_product",1);

  if (TYPEOF(Pike_sp[-1]) != T_OBJECT ||
      !((mx=get_storage(Pike_sp[-1].u.object,XmatrixY(math_,_program)))))
    SIMPLE_ARG_TYPE_ERROR("dot_product",1,"object(Math.Matrix)");

  if(!(mx->xsize==THIS->xsize &&
       mx->ysize==THIS->ysize &&
       (mx->xsize==1 || mx->ysize==1)))
    math_error("dot_product",args,0,
	       "Matrices must be the same sizes, and one-dimensional.\n");

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
      SIMPLE_WRONG_NUM_ARGS_ERROR("convolve",1);

   if (TYPEOF(Pike_sp[-args]) != T_OBJECT ||
       !((bmx=get_storage(Pike_sp[-args].u.object,XmatrixY(math_,_program)))))
      SIMPLE_ARG_TYPE_ERROR("convolve",1,"object(Math.Matrix)");

   if (bmx->xsize==0 || bmx->ysize==0 ||
       THIS->xsize==0 || THIS->ysize==0)
      math_error("convolve",args,0,
		 "Source or argument matrix too small (zero size).\n");

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


static void matrixX(_xsize)(INT32 args)
{
  pop_n_elems(args);
  push_int( THIS->xsize );
}

static void matrixX(_ysize)(INT32 args)
{
  pop_n_elems(args);
  push_int( THIS->ysize );
}


/* ---------------------------------------------------------------- */

void Xmatrix(init_math_)(void)
{
#define MKSTR(X) make_shared_binary_string(X,CONSTANT_STRLEN(X))
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
		      tFunc(tInt1Plus tInt1Plus tOr4(tInt,tFloat,tString,tVoid), tVoid)), ID_PROTECTED);

   ADD_FUNCTION("cast",matrixX(_cast),
		tFunc(tStr, tArr(tArr(tFloat))), ID_PROTECTED);
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

   ADD_FUNCTION("dot_product",matrixX(_dot), tFunc(tObj, tObj), 0);

   ADD_FUNCTION("convolve",matrixX(_convolve), tFunc(tObj, tObj), 0);

   ADD_FUNCTION("cross",matrixX(_cross), tFunc(tObj, tObj), 0);

   ADD_FUNCTION("xsize", matrixX(_xsize), tFunc(tNone, tInt), 0);
   ADD_FUNCTION("ysize", matrixX(_ysize), tFunc(tNone, tInt), 0);
#ifdef HAS_MPI
   ADD_FUNCTION("get_sentinel", matrixX(get_sentinel), tFunc(tNone, tObj), 0);
#endif

   Pike_compiler->new_program->flags |=
     PROGRAM_CONSTANT |
     PROGRAM_NO_EXPLICIT_DESTRUCT ;
}

void Xmatrix(exit_math_)(void)
{
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
