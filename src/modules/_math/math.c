/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "config.h"
#include "interpret.h"
#include "constants.h"
#include "svalue.h"
#include "pike_error.h"
#include "module_support.h"
#include "operators.h"
#include "bignum.h"
#include "pike_types.h"
#include "pike_float.h"

#define sp Pike_sp
#define TRIM_STACK(X) if(args>(X)) pop_n_elems(args-(X));
#define ARG_CHECK(X) if(args!=1) SIMPLE_WRONG_NUM_ARGS_ERROR(X, 1); \
  if(TYPEOF(sp[-args]) == T_INT) SET_SVAL(sp[-1],T_FLOAT,0,float_number,(FLOAT_TYPE)(sp[-1].u.integer)); \
  else if(TYPEOF(sp[-args]) != T_FLOAT) SIMPLE_ARG_TYPE_ERROR(X, 1, "float");

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795080
#endif

#if defined (WITH_LONG_DOUBLE_PRECISION_SVALUE)
#define FL(FN) PIKE_CONCAT(FN,l)
#elif defined (WITH_DOUBLE_PRECISION_SVALUE) || !defined(HAVE_COSF)
#define FL(FN) FN
#else
#define FL(FN) PIKE_CONCAT(FN,f)
#endif

#ifndef HAVE_ASINH
/* Visual Studio versions prior to 2013 did not implement
 * the arcus hyperbolic functions.
 */
FLOAT_TYPE FL(asinh)(FLOAT_TYPE x)
{
  /* Cf https://en.wikipedia.org/wiki/Inverse_hyperbolic_functions */
  FLOAT_TYPE x2 = x*x;
  if (x2 < 1.0) {
    /* Use expansion series when x*x is less than 1.0 in order
     * to avoid issues with running out of mantissa in the
     * argument to sqrt().
     */
    FLOAT_TYPE res = 0.0;
    FLOAT_TYPE delta = x;
    int i = 1;

    while (delta) {
      res += delta;

      delta *= -i*i * x2;
      delta /= (i + 1) * (i + 2);
      i += 2;
    }
    return res;
  } else if (x < 0.0) {
    return -FL(log)(FL(sqrt)(x2 + 1.0) - x);
  }
  return FL(log)(FL(sqrt)(x2 + 1.0) + x);
}
FLOAT_TYPE FL(acosh)(FLOAT_TYPE x)
{
  return FL(log)(FL(sqrt)(x*x - 1.0) + x);
}
FLOAT_TYPE FL(atanh)(FLOAT_TYPE x)
{
  return 0.5 * (FL(log)(1.0 + x) - FL(log)(1.0 - x));
}
#endif

/*! @decl float sin(int|float f)
 *!
 *! Returns the sine value for @[f].
 *! @[f] should be specified in radians.
 *!
 *! @seealso
 *!   @[asin()], @[cos()], @[tan()]
 */
void f_sin(INT32 args)
{
  ARG_CHECK("sin");
  sp[-1].u.float_number = FL(sin)(sp[-1].u.float_number);
}

/*! @decl float asin(int|float f)
 *!
 *! Return the arcus sine value for @[f].
 *! The result will be in radians.
 *!
 *! @seealso
 *!   @[sin()], @[acos()]
 */
void f_asin(INT32 args)
{
  ARG_CHECK("asin");
  if ((sp[-1].u.float_number >= -1.0) &&
      (sp[-1].u.float_number <= 1.0)) {
    sp[-1].u.float_number = FL(asin)(sp[-1].u.float_number);
  } else {
    sp[-1].u.float_number = (FLOAT_TYPE) MAKE_NAN();
  }
}

/*! @decl float cos(int|float f)
 *!
 *! Return the cosine value for @[f].
 *! @[f] should be specified in radians.
 *!
 *! @seealso
 *!   @[acos()], @[sin()], @[tan()]
 */
void f_cos(INT32 args)
{
  ARG_CHECK("cos");
  sp[-1].u.float_number = FL(cos)(sp[-1].u.float_number);
}

/*! @decl float acos(int|float f)
 *!
 *! Return the arcus cosine value for @[f].
 *! The result will be in radians.
 *!
 *! @seealso
 *!   @[cos()], @[asin()]
 */
void f_acos(INT32 args)
{
  ARG_CHECK("acos");
  if ((sp[-1].u.float_number >= -1.0) &&
      (sp[-1].u.float_number <= 1.0)) {
    sp[-1].u.float_number = FL(acos)(sp[-1].u.float_number);
  } else {
    sp[-1].u.float_number = (FLOAT_TYPE) MAKE_NAN();
  }
}

/*! @decl float tan(int|float f)
 *!
 *! Returns the tangent value for @[f].
 *! @[f] should be specified in radians.
 *!
 *! @seealso
 *!   @[atan()], @[sin()], @[cos()]
 */
void f_tan(INT32 args)
{
  FLOAT_ARG_TYPE f;
  ARG_CHECK("tan");

  f = (sp[-1].u.float_number-M_PI/2) / M_PI;
  if(f==floor(f+0.5))
  {
    Pike_error("Impossible tangent.\n");
    return;
  }
  sp[-1].u.float_number = FL(tan)(sp[-1].u.float_number);
}

/*! @decl float atan(int|float f)
 *!
 *! Returns the arcus tangent value for @[f].
 *! The result will be in radians.
 *!
 *! @seealso
 *!   @[tan()], @[asin()], @[acos()], @[atan2()]
 */
void f_atan(INT32 args)
{
  ARG_CHECK("atan");
  sp[-1].u.float_number = FL(atan)(sp[-1].u.float_number);
}

/*! @decl float atan2(float f1, float f2)
 *!
 *! Returns the arcus tangent value for @[f1]/@[f2], and uses
 *! the signs of @[f1] and @[f2] to determine the quadrant.
 *! The result will be in radians.
 *!
 *! @seealso
 *!   @[tan()], @[asin()], @[acos()], @[atan()]
 */
void f_atan2(INT32 args)
{
  if(args!=2)
    SIMPLE_WRONG_NUM_ARGS_ERROR("atan2", 2);

  if(TYPEOF(sp[-2]) != T_FLOAT)
    SIMPLE_ARG_TYPE_ERROR("atan2", 1, "float");
  if(TYPEOF(sp[-1]) != T_FLOAT)
    SIMPLE_ARG_TYPE_ERROR("atan2", 2, "float");
  sp[-2].u.float_number= FL(atan2)(sp[-2].u.float_number,sp[-1].u.float_number);
  pop_stack();
}

/*! @decl float sinh(int|float f)
 *!
 *! Returns the hyperbolic sine value for @[f].
 *!
 *! @seealso
 *!   @[asinh()], @[cosh()], @[tanh()]
 */
void f_sinh(INT32 args)
{
  ARG_CHECK("sinh");
  sp[-1].u.float_number = FL(sinh)(sp[-1].u.float_number);
}

/*! @decl float asinh(int|float f)
 *!
 *! Return the hyperbolic arcus sine value for @[f].
 *!
 *! @seealso
 *!   @[sinh()], @[acosh()]
 */
void f_asinh(INT32 args)
{
  ARG_CHECK("asinh");
  sp[-1].u.float_number = FL(asinh)(sp[-1].u.float_number);
}

/*! @decl float cosh(int|float f)
 *!
 *! Return the hyperbolic cosine value for @[f].
 *!
 *! @seealso
 *!   @[acosh()], @[sinh()], @[tanh()]
 */
void f_cosh(INT32 args)
{
  ARG_CHECK("cosh");
  sp[-1].u.float_number = FL(cosh)(sp[-1].u.float_number);
}

/*! @decl float acosh(int|float f)
 *!
 *! Return the hyperbolic arcus cosine value for @[f].
 *!
 *! @seealso
 *!   @[cosh()], @[asinh()]
 */
void f_acosh(INT32 args)
{
  ARG_CHECK("acosh");
  sp[-1].u.float_number = FL(acosh)(sp[-1].u.float_number);
}

/*! @decl float tanh(int|float f)
 *!
 *! Returns the hyperbolic tangent value for @[f].
 *!
 *! @seealso
 *!   @[atanh()], @[sinh()], @[cosh()]
 */
void f_tanh(INT32 args)
{
  ARG_CHECK("tanh");
  sp[-1].u.float_number = FL(tanh)(sp[-1].u.float_number);
}

/*! @decl float atanh(int|float f)
 *!
 *! Returns the hyperbolic arcus tangent value for @[f].
 *!
 *! @seealso
 *!   @[tanh()], @[asinh()], @[acosh()]
 */
void f_atanh(INT32 args)
{
  ARG_CHECK("atanh");
  sp[-1].u.float_number = FL(atanh)(sp[-1].u.float_number);
}

/*! @decl float sqrt(float f)
 *! @decl int(0..) sqrt(int(0..) i)
 *! @decl mixed sqrt(object o)
 *!
 *! Returns the square root of @[f], or in the integer case, the square root
 *! truncated to the closest lower integer. If the argument is an object,
 *! the lfun _sqrt in the object will be called.
 *!
 *! @seealso
 *!   @[pow()], @[log()], @[exp()], @[floor()], @[lfun::_sqrt]
 */

/*! @decl mixed lfun::_sqrt()
 *!   Called by sqrt when the square root of an object is requested.
 *! @note
 *!   _sqrt is not a real lfun, so it must not be defined as static.
 *! @seealso
 *!   @[predef::sqrt()]
 */
void f_sqrt(INT32 args)
{
  if(args!=1)
    SIMPLE_WRONG_NUM_ARGS_ERROR("sqrt", 1);

  if(TYPEOF(sp[-1]) == T_INT)
  {
    unsigned INT_TYPE n, b, s, y=0;
    unsigned INT_TYPE x=0;

    /* FIXME: Note: Regards i as an unsigned value. */

    if(sp[-1].u.integer<0) Pike_error("math: sqrt(x) with (x < 0)\n");
    n=sp[-1].u.integer;

    for(b=(INT_TYPE) 1 << (sizeof(INT_TYPE)*8-2); b; b>>=2)
    {
      x<<=1; s=b+y; y>>=1;
      if(n>=s)
      {
	x|=1; y|=b; n-=s;
      }
    }
    sp[-1].u.integer=x;
  }
  else if(TYPEOF(sp[-1]) == T_FLOAT)
  {
    if (sp[-1].u.float_number< 0.0)
    {
      Pike_error("math: sqrt(x) with (x < 0.0)\n");
      return;
    }
    sp[-1].u.float_number = FL(sqrt)(sp[-1].u.float_number);
  }
  else if(TYPEOF(sp[-1]) == T_OBJECT)
  {
      int i = FIND_LFUN(sp[-1].u.object->prog,LFUN__SQRT);
      if( i<0 )
        Pike_error("Object has no _sqrt method.\n");
      apply_low(sp[-1].u.object,i,0);
      stack_swap();
      pop_stack();
  }
  else
  {
    SIMPLE_ARG_TYPE_ERROR("sqrt", 1, "int|float|object");
  }
}

/*! @decl float log(int|float f)
 *!
 *! Return the natural logarithm of @[f].
 *! @expr{exp( log(x) ) == x@} for x > 0.
 *!
 *! @seealso
 *!   @[pow()], @[exp()]
 */
void f_log(INT32 args)
{
  ARG_CHECK("log");
  if(sp[-1].u.float_number <=0.0)
    Pike_error("Log on number less or equal to zero.\n");

  sp[-1].u.float_number = FL(log)(sp[-1].u.float_number);
}

/*! @decl float exp(float|int f)
 *!
 *! Return the natural exponential of @[f].
 *! @expr{log( exp( x ) ) == x@} as long as exp(x) doesn't overflow an int.
 *!
 *! @seealso
 *!   @[pow()], @[log()]
 */
void f_exp(INT32 args)
{
  ARG_CHECK("exp");
  SET_SVAL(sp[-1], T_FLOAT, 0, float_number, FL(exp)(sp[-1].u.float_number));
}

/*! @decl int|float pow(float|int n, float|int x)
 *! @decl mixed pow(object n, float|int|object x)
 *!
 *! Return @[n] raised to the power of @[x]. If both @[n]
 *! and @[x] are integers the result will be an integer.
 *! If @[n] is an object its pow method will be called with
 *! @[x] as argument.
 *!
 *! @seealso
 *!   @[exp()], @[log()]
 */

/*! @decl float floor(int|float f)
 *!
 *! Return the closest integer value less or equal to @[f].
 *!
 *! @note
 *!   @[floor()] does @b{not@} return an @expr{int@}, merely an integer value
 *!   stored in a @expr{float@}.
 *!
 *! @seealso
 *!   @[ceil()], @[round()]
 */
void f_floor(INT32 args)
{
  ARG_CHECK("floor");
  sp[-1].u.float_number = FL(floor)(sp[-1].u.float_number);
}

/*! @decl float ceil(int|float f)
 *!
 *! Return the closest integer value greater or equal to @[f].
 *!
 *! @note
 *!   @[ceil()] does @b{not@} return an @expr{int@}, merely an integer value
 *!   stored in a @expr{float@}.
 *!
 *! @seealso
 *!   @[floor()], @[round()]
 */
void f_ceil(INT32 args)
{
  ARG_CHECK("ceil");
  sp[-1].u.float_number = FL(ceil)(sp[-1].u.float_number);
}

/*! @decl float round(int|float f)
 *!
 *! Return the closest integer value to @[f].
 *!
 *! @note
 *!   @[round()] does @b{not@} return an @expr{int@}, merely an integer value
 *!   stored in a @expr{float@}.
 *!
 *! @seealso
 *!   @[floor()], @[ceil()]
 */
void f_round(INT32 args)
{
  ARG_CHECK("round");
  sp[-1].u.float_number = FL(rint)(sp[-1].u.float_number);
}


/*! @decl int|float|object limit(int|float|object minval, @
 *!                              int|float|object x, @
 *!                              int|float|object maxval)
 *!
 *! Limits the value @[x] so that it's between @[minval] and @[maxval].
 *! If @[x] is an object, it must implement the @[lfun::`<] method.
 *!
 *! @seealso
 *!   @[max()] and @[min()]
 */
void f_limit(INT32 args)
{
  INT32 minpos = 0;

  if(args != 3)
  {
      Pike_error("limit needs 3 arguments\n");
  }

  /* -3  -2  -1 */
  /*  a < X < b */
  if( is_lt( Pike_sp-3,  Pike_sp-2 ) )      /* a < X */
  {
      if( is_lt( Pike_sp-2,  Pike_sp-1 ) )      /* X < b */
      {
	  /* return X (-2) */
	  pop_stack();
	  stack_pop_keep_top();
      }
      else
      {
         /* X > b, return b (-1) */
	  stack_unlink( 2 );
      }
  }
  else
      pop_n_elems(2); /* a > X, return a (-3)*/
}

/*! @decl int|float|object min(int|float|object, int|float|object ... args)
 *! @decl string min(string, string ... args)
 *! @decl int(0..0) min()
 *!
 *! Returns the smallest value among @[args]. Compared objects
 *! must implement the @[lfun::`<] method.
 *!
 *! @seealso
 *!   @[max()] and @[limit()]
 */
void f_min(INT32 args)
{
  INT32 i;
  INT32 minpos = 0;

  if(!args) {
    pop_n_elems(args);
    push_int(0);
    return;
  }

  for (i=args-1; i>0; i--) {
    if (is_gt(sp+minpos-args, sp+i-args)) {
      minpos = i;
    }
  }
  if (minpos) {
    assign_svalue(sp-args, sp+minpos-args);
  }
  pop_n_elems(args-1);
}

/*! @decl int|float|object max(int|float|object, int|float|object ... args)
 *! @decl string max(string, string ... args)
 *! @decl int(0..0) max()
 *!
 *! Returns the largest value among @[args]. Compared objects
 *! must implement the @[lfun::`<] method.
 *!
 *! @seealso
 *!   @[min()] and @[limit()]
 */
void f_max(INT32 args)
{
  INT32 i;
  INT32 maxpos = 0;

  if(!args) {
    pop_n_elems(args);
    push_int(0);
    return;
  }

  for (i=args-1; i>0; i--) {
    if (is_lt(sp+maxpos-args, sp+i-args)) {
      maxpos = i;
    }
  }
  if (maxpos) {
    assign_svalue(sp-args, sp+maxpos-args);
  }
  pop_n_elems(args-1);
}

/*! @decl float abs(float f)
 *! @decl int abs(int f)
 *! @decl object abs(object f)
 *!
 *! Return the absolute value for @[f]. If @[f] is
 *! an object it must implement @[lfun::`<] and
 *! unary @[lfun::`-].
 */
void f_abs(INT32 args)
{
  struct svalue zero;
  SET_SVAL(zero, T_INT, NUMBER_NUMBER, integer, 0);

  check_all_args(NULL,args,BIT_INT|BIT_FLOAT|BIT_OBJECT,0);
  pop_n_elems(args-1);
  if(is_lt(sp-1,&zero)) o_negate();
}

static node *optimize_abs(node *n)
{
  struct pike_type *t = n->type;
  if (t->type == T_INT) {
    INT32 min = CAR_TO_INT(t);
    INT32 max = CDR_TO_INT(t);
    INT32 res_max;
    INT32 res_min;
    if (min > 0) {
      /* Both above zero. */
      res_max = max;
      res_min = min;
    } else if (max < 0) {
      /* Both below zero. */
      res_max = (min == MIN_INT32? MAX_INT32 : -min);
      res_min = (max == MIN_INT32? MAX_INT32 : -max);
    } else if (min < -max) {
      /* Zero in interval and more below zero. */
      res_max = (min == MIN_INT32? MAX_INT32 : -min);
      res_min = 0;
    } else {
      /* Zero in interval and more above zero. */
      res_max = max;
      res_min = 0;
    }
    type_stack_mark();
    push_int_type(res_min, res_max);
    n->type = pop_unfinished_type();
    free_type(t);
  }
  return NULL;
}

/*! @decl int sgn(mixed value)
 *! @decl int sgn(mixed value, mixed zero)
 *!
 *! Check the sign of a value.
 *!
 *! @returns
 *!   Returns @expr{-1@} if @[value] is less than @[zero],
 *!   @expr{1@} if @[value] is greater than @[zero] and @expr{0@}
 *!   (zero) otherwise.
 *!
 *! @seealso
 *!   @[abs()]
 */
void f_sgn(INT32 args)
{
  TRIM_STACK(2);
  check_all_args(NULL,args,BIT_MIXED,BIT_VOID|BIT_MIXED,0);
  if(args<2)
    push_int(0);

  if(is_lt(sp-2,sp-1))
  {
    pop_n_elems(2);
    push_int(-1);
  }
  else if(is_gt(sp-2,sp-1))
  {
    pop_n_elems(2);
    push_int(1);
  }
  else
  {
    pop_n_elems(2);
    push_int(0);
  }
}

#define tNUM tOr(tInt,tFlt)

PIKE_MODULE_INIT
{
#ifdef HAVE_FPSETMASK
#ifdef HAVE_WORKING_FPSETMASK
  fpsetmask(0);
#endif
#endif
#ifdef HAVE_FPSETROUND
#ifndef HAVE_FP_RZ
#define FP_RZ 0
#endif /* !HAVE_FP_RZ */
  fpsetround(FP_RZ);	/* Round to zero (truncate) */
#endif /* HAVE_FPSETROUND */
#ifdef HAVE_FPSETFASTMODE
  fpsetfastmode(1);
#endif /* HAVE_FPSETFASTMODE */

  /* function(int|float:float) */
  ADD_EFUN("sin",f_sin,tFunc(tNUM,tFlt),0);

  /* function(int|float:float) */
  ADD_EFUN("asin",f_asin,tFunc(tNUM,tFlt),0);

  /* function(int|float:float) */
  ADD_EFUN("cos",f_cos,tFunc(tNUM,tFlt),0);

  /* function(int|float:float) */
  ADD_EFUN("acos",f_acos,tFunc(tNUM,tFlt),0);

  /* function(int|float:float) */
  ADD_EFUN("tan",f_tan,tFunc(tNUM,tFlt),0);

  /* function(int|float:float) */
  ADD_EFUN("atan",f_atan,tFunc(tNUM,tFlt),0);

  /* function(int|float,float:float) */
  ADD_EFUN("atan2",f_atan2,tFunc(tFlt tFlt,tFlt),0);

  ADD_EFUN("sinh",f_sinh,tFunc(tNUM,tFlt),0);
  ADD_EFUN("asinh",f_asinh,tFunc(tNUM,tFlt),0);
  ADD_EFUN("cosh",f_cosh,tFunc(tNUM,tFlt),0);
  ADD_EFUN("acosh",f_acosh,tFunc(tNUM,tFlt),0);
  ADD_EFUN("tanh",f_tanh,tFunc(tNUM,tFlt),0);
  ADD_EFUN("atanh",f_atanh,tFunc(tNUM,tFlt),0);

  /* function(float:float)|function(int:int) */
  ADD_EFUN("sqrt",f_sqrt,tOr3(tFunc(tFlt,tFlt),
			      tFunc(tSetvar(0, tIntPos),
				    tRangeInt(tZero, tVar(0))),
			      tFunc(tObj,tMix)),0);

  /* function(int|float:float) */
  ADD_EFUN("log",f_log,tFunc(tNUM,tFlt),0);

  /* function(int|float:float) */
  ADD_EFUN("exp",f_exp,tFunc(tNUM,tFlt),0);

  /* function(float,float:float) */
  ADD_EFUN("pow",f_exponent,
	   tOr5(tFunc(tFlt tFlt,tFlt),
		tFunc(tInt tFlt,tFlt),
		tFunc(tFlt tInt,tFlt),
		tFunc(tInt tInt,tInt),
		tFunc(tObj tOr3(tInt,tObj,tFlt),tOr3(tObj,tInt,tFlt))),0);

  /* function(int|float:float) */
  ADD_EFUN("floor",f_floor,tFunc(tNUM,tFlt),0);

  /* function(int|float:float) */
  ADD_EFUN("ceil",f_ceil,tFunc(tNUM,tFlt),0);

  /* function(int|float:float) */
  ADD_EFUN("round",f_round,tFunc(tNUM,tFlt),0);

#define MINMAX_TYPE(OP)							\
  tOr(tFunc(tNone, tInt0),						\
      tTransitive(tFunc(tSetvar(0, tOr4(tInt, tFloat, tString, tObj)),	\
			tVar(0)),					\
		  tOr4(tFunc(tSetvar(0, tInt) tSetvar(1, tInt),		\
			     OP(tVar(0), tVar(1))),			\
		       tFunc(tSetvar(0, tOr(tFloat, tObj))		\
			     tSetvar(1, tOr3(tInt, tFloat, tObj)),	\
			     tOr(tVar(0), tVar(1))),			\
		       tFunc(tSetvar(0, tInt)				\
			     tSetvar(1, tOr(tFloat, tObj)),		\
			     tOr(tVar(0), tVar(1))),			\
		       tFunc(tSetvar(0, tString) tSetvar(1, tString),	\
			     tOr(tVar(0), tVar(1))))))

  ADD_EFUN("max", f_max, MINMAX_TYPE(tMaxInt), 0);
  ADD_EFUN("min", f_min, MINMAX_TYPE(tMinInt), 0);

  ADD_EFUN("limit",f_limit,
	   tOr4(tFunc(tSetvar(0, tInt) tSetvar(1, tInt) tSetvar(2, tInt),
		      tRangeInt(tMinInt(tMaxInt(tVar(0), tVar(1)), tVar(2)),
				tMaxInt(tVar(0), tMinInt(tVar(1), tVar(2))))),
		tFunc(tSetvar(0, tOr(tFlt, tObj))
		      tSetvar(1, tOr3(tFlt, tInt, tObj))
		      tSetvar(2, tOr3(tFlt, tInt, tObj)),
		      tOr3(tVar(0),tVar(1), tVar(2))),
		tFunc(tSetvar(0, tOr3(tFlt, tInt, tObj))
		      tSetvar(1, tOr(tFlt, tObj))
		      tSetvar(2, tOr3(tFlt, tInt, tObj)),
		      tOr3(tVar(0),tVar(1), tVar(2))),
		tFunc(tSetvar(0, tOr3(tFlt, tInt, tObj))
		      tSetvar(1, tOr3(tFlt, tInt, tObj))
		      tSetvar(2, tOr(tFlt, tObj)),
		      tOr3(tVar(0), tVar(1), tVar(2)))), 0);

  /* function(float|int|object:float|int|object) */
  ADD_EFUN2("abs", f_abs,
	    tOr(tFunc(tSetvar(0, tInt),
		      tMaxInt(tVar(0), tNegateInt(tVar(0)))),
		tFunc(tSetvar(0, tOr(tFlt, tObj)), tVar(0))),
	    OPT_TRY_OPTIMIZE, optimize_abs, 0);

  /* function(mixed,mixed|void:int) */
  ADD_EFUN("sgn", f_sgn,
	   tOr6(tFunc(tInt1Plus, tInt1),
		tFunc(tZero, tZero),
		tFunc(tIntMinus, tInt_1),
		tFunc(tSetvar(0, tInt) tSetvar(1, tInt),
		      tAnd(tRangeInt(tInt_1,
				     tMaxInt(tSubInt(tVar(0), tVar(1)), tInt_1)),
			   tRangeInt(tMinInt(tSubInt(tVar(0), tVar(1)), tInt1),
				     tInt1))),
		tFunc(tOr(tFloat, tObj), tInt_11),
		tFunc(tNot(tInt) tMix, tInt_11)), 0);
}

PIKE_MODULE_EXIT {}
