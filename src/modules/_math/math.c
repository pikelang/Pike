/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: math.c,v 1.76 2004/03/19 15:47:06 grubba Exp $
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

#include <math.h>

#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif

#ifdef HAVE_FP_CLASS_H
#include <fp_class.h>
#endif

#ifdef HAVE_FLOATINGPOINT_H
#include <floatingpoint.h>
#endif


#define sp Pike_sp
#define TRIM_STACK(X) if(args>(X)) pop_n_elems(args-(X));
#define ARG_CHECK(X) if(args<1) SIMPLE_TOO_FEW_ARGS_ERROR(X, 1); \
  if(sp[-args].type!=T_FLOAT) SIMPLE_BAD_ARG_ERROR(X, 1, "float"); \
  TRIM_STACK(1)

RCSID("$Id: math.c,v 1.76 2004/03/19 15:47:06 grubba Exp $");

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795080
#endif

#if defined (WITH_LONG_DOUBLE_PRECISION_SVALUE)

/* Assume that if these two l-suffixed functions exist, they all do. */
# if defined (HAVE_EXPL) && defined (HAVE_FLOORL)
#  define FL1(FN, ARG1) PIKE_CONCAT(FN,l) (ARG1)
#  define FL2(FN, ARG1, ARG2) PIKE_CONCAT(FN,l) (ARG1, ARG2)
# else
/* Will lose precision. :\ */
#  define FL1(FN, ARG1) FN (DO_NOT_WARN ((double) (ARG1)))
#  define FL2(FN, ARG1, ARG2) FN (DO_NOT_WARN ((double) (ARG1)),	\
				  DO_NOT_WARN ((double) (ARG2)))
# endif

#elif defined (WITH_DOUBLE_PRECISION_SVALUE)

# define FL1(FN, ARG1) FN (ARG1)
# define FL2(FN, ARG1, ARG2) FN (ARG1, ARG2)

#else

/* Assume that if these two f-suffixed functions exist, they all do. */
# if defined (HAVE_EXPF) && defined (HAVE_FLOORF)
#  define FL1(FN, ARG1) PIKE_CONCAT(FN,f) (ARG1)
#  define FL2(FN, ARG1, ARG2) PIKE_CONCAT(FN,f) (ARG1, ARG2)
# else
#  define FL1(FN, ARG1) DO_NOT_WARN ((FLOAT_TYPE) FN (ARG1))
#  define FL2(FN, ARG1, ARG2) DO_NOT_WARN ((FLOAT_TYPE) FN ((ARG1), (ARG2)))
# endif

# define FA1(FN, ARG1) FN (ARG1)
# define FA2(FN, ARG1, ARG2) FN (ARG1, ARG2)

#endif

/* FA1 and FA2 are used for FLOAT_ARG_TYPE. */
#ifndef FA1
# define FA1(FN, ARG1) FL1 (FN, (ARG1))
# define FA2(FN, ARG1, ARG2) FL2 (FN, (ARG1), (ARG2))
#endif

#ifndef NO_MATHERR
#ifdef HAVE_STRUCT_EXCEPTION

int matherr(struct exception *exc)
{
#ifdef HUGE_VAL
  if (exc) {
    switch(exc->type) {
    case OVERFLOW:
      exc->retval = HUGE_VAL;
      return 1;	/* No error */
    case UNDERFLOW:
      exc->retval = 0.0;
      return 1; /* No error */
#ifdef TLOSS
    case TLOSS:
      return 1; /* No error */
#endif /* TLOSS */
#ifdef PLOSS
    case PLOSS:
      return 1; /* No error */
#endif /* PLOSS */
    default:
      return 0; /* Error */
    }
  }
#endif /* HUGE_VAL */
  return 1;	/* No error */
}

#endif /* HAVE_STRUCT_EXCEPTION */
#endif /* !NO_MATHERR */

/*! @decl float sin(float f)
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
  sp[-1].u.float_number = FL1(sin,sp[-1].u.float_number);
}

/*! @decl float asin(float f)
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
  sp[-1].u.float_number = FL1(asin,sp[-1].u.float_number);
}

/*! @decl float cos(float f)
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
  sp[-1].u.float_number = FL1(cos,sp[-1].u.float_number);
}

/*! @decl float acos(float f)
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
  sp[-1].u.float_number = FL1(acos,sp[-1].u.float_number);
}

/*! @decl float tan(float f)
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
  sp[-1].u.float_number = FL1(tan,sp[-1].u.float_number);
}

/*! @decl float atan(float f)
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
  sp[-1].u.float_number = FL1(atan,sp[-1].u.float_number);
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
  if(args<2)
    SIMPLE_TOO_FEW_ARGS_ERROR("atan2", 1);
  TRIM_STACK(2);
  if(sp[-2].type!=T_FLOAT)
    SIMPLE_BAD_ARG_ERROR("atan2", 1, "float");
  if(sp[-1].type!=T_FLOAT)
    SIMPLE_BAD_ARG_ERROR("atan2", 2, "float");
  sp[-2].u.float_number= FL2(atan2,sp[-2].u.float_number,sp[-1].u.float_number);
  pop_stack();
}

/* insert configure test if necessary */
#if 1 
/*! @decl float sinh(float f)
 *!
 *! Returns the hyperbolic sine value for @[f].
 *!
 *! @seealso
 *!   @[asinh()], @[cosh()], @[tanh()]
 */
void f_sinh(INT32 args)
{
  FLOAT_ARG_TYPE x;
  ARG_CHECK("sinh");
  x=sp[-1].u.float_number;

  sp[-1].u.float_number =
    DO_NOT_WARN ((FLOAT_TYPE) (0.5*(FA1(exp,x)-FA1(exp,-x))));
}

/*! @decl float asinh(float f)
 *!
 *! Return the hyperbolic arcus sine value for @[f].
 *!
 *! @seealso
 *!   @[sinh()], @[acosh()]
 */
void f_asinh(INT32 args)
{
  FLOAT_ARG_TYPE x;
  ARG_CHECK("asinh");
  x=sp[-1].u.float_number;

  sp[-1].u.float_number =
    DO_NOT_WARN ((FLOAT_TYPE) (FA1(log,x+FA1(sqrt,1+x*x))));
}

/*! @decl float cosh(float f)
 *!
 *! Return the hyperbolic cosine value for @[f].
 *!
 *! @seealso
 *!   @[acosh()], @[sinh()], @[tanh()]
 */
void f_cosh(INT32 args)
{
  FLOAT_ARG_TYPE x;
  ARG_CHECK("cosh");
  x=sp[-1].u.float_number;

  sp[-1].u.float_number =
    DO_NOT_WARN ((FLOAT_TYPE) (0.5*(FA1(exp,x)+FA1(exp,-x))));
}

/*! @decl float acosh(float f)
 *!
 *! Return the hyperbolic arcus cosine value for @[f].
 *!
 *! @seealso
 *!   @[cosh()], @[asinh()]
 */
void f_acosh(INT32 args)
{
  FLOAT_ARG_TYPE x;
  ARG_CHECK("acosh");
  x=sp[-1].u.float_number;

  sp[-1].u.float_number =
    DO_NOT_WARN ((FLOAT_TYPE) (2*FA1(log,FA1(sqrt,0.5*(x+1))+FA1(sqrt,0.5*(x-1)))));
}

/*! @decl float tanh(float f)
 *!
 *! Returns the hyperbolic tangent value for @[f].
 *!
 *! @seealso
 *!   @[atanh()], @[sinh()], @[cosh()]
 */
void f_tanh(INT32 args)
{
  FLOAT_ARG_TYPE x;
  ARG_CHECK("tanh");
  x=sp[-1].u.float_number;

  sp[-1].u.float_number =
    DO_NOT_WARN ((FLOAT_TYPE) ((FA1(exp,x)-FA1(exp,-x))/(FA1(exp,x)+FA1(exp,-x))));
}

/*! @decl float atanh(float f)
 *!
 *! Returns the hyperbolic arcus tangent value for @[f].
 *!
 *! @seealso
 *!   @[tanh()], @[asinh()], @[acosh()]
 */ 
void f_atanh(INT32 args)
{
  FLOAT_ARG_TYPE x;
  ARG_CHECK("atanh");
  x=sp[-1].u.float_number;

  sp[-1].u.float_number =
    DO_NOT_WARN ((FLOAT_TYPE) (0.5*(FA1(log,1+x)-FA1(log,1-x))));
}

#endif

/*! @decl float sqrt(float f)
 *! @decl int sqrt(int i)
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
  if(args<1)
    SIMPLE_TOO_FEW_ARGS_ERROR("sqrt", 1);
  TRIM_STACK(1);

  if(sp[-1].type==T_INT)
  {
    /* Note: This algorithm is also implemented in src/stuff.c */
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
  else if(sp[-1].type==T_FLOAT)
  {
    if (sp[-1].u.float_number< 0.0)
    {
      Pike_error("math: sqrt(x) with (x < 0.0)\n");
      return;
    }
    sp[-1].u.float_number = FL1(sqrt,sp[-1].u.float_number);
  }
  else if(sp[-1].type == T_OBJECT)
  {
    stack_dup();
    push_constant_text("_sqrt");
    o_index();
    if(IS_UNDEFINED(&sp[-1]))
      Pike_error("Object to to sqrt() does not have _sqrt.\n");
    pop_stack(); /* Maybe we can use this result instead of throwing it? */
    apply(sp[-1].u.object, "_sqrt", 0);
    stack_swap();
    pop_stack();
  }
  else
  {
    SIMPLE_BAD_ARG_ERROR("sqrt", 1, "int|float|object");
  }
}

/*! @decl float log(float f)
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

  sp[-1].u.float_number = FL1(log,sp[-1].u.float_number);
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
  FLOAT_TYPE f;
  get_all_args("exp",args,"%F",&f);
  TRIM_STACK(1);
  sp[-1].type = T_FLOAT;
  sp[-1].u.float_number = FL1(exp,f);
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
void f_pow(INT32 args)
{
  if(args < 2)
    SIMPLE_TOO_FEW_ARGS_ERROR("pow", 2);
  TRIM_STACK(2);

  switch(Pike_sp[-2].type * 16 + Pike_sp[-1].type)
  {
    case T_INT * 17:
    case T_INT * 16 + T_OBJECT:
#ifndef AUTO_BIGNUM
      ref_push_type_value(float_type_string);
      stack_swap();
      f_cast();
      stack_swap();

      ref_push_type_value(float_type_string);
      stack_swap();
      f_cast();

      stack_swap();
      f_pow(2);

      o_cast_to_int();
      return;
#endif
    case T_OBJECT * 17:
    case T_OBJECT * 16 + T_INT:
    case T_OBJECT * 16 + T_FLOAT:
      stack_swap();
      push_constant_text("pow");
      f_index(2);
      stack_swap();
      f_call_function(2);
      return;

    case T_FLOAT * 16 + T_INT:
    case T_INT * 16 + T_FLOAT:
    case T_FLOAT * 17:
    {
      FLOAT_TYPE x,y;
      get_all_args("pow",2,"%F%F",&x,&y);
      pop_stack();
      sp[-1].type = T_FLOAT;
      sp[-1].u.float_number = FL2(pow,x,y);
      return;
    }

  default:
    Pike_error("Invalid argument types to pow.\n");
  }
}

/*! @decl float floor(float f)
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
  sp[-1].u.float_number = FL1(floor,sp[-1].u.float_number);
}

/*! @decl float ceil(float f)
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
  sp[-1].u.float_number = FL1(ceil,sp[-1].u.float_number);
}

/*! @decl float round(float f)
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
  sp[-1].u.float_number = FL1(RINT,sp[-1].u.float_number);
}

/*! @decl int|float|object min(int|float|object ... args)
 *! @decl string min(string ... args)
 *! @decl int(0..0) min()
 *!
 *! Returns the smallest value among @[args]. Compared objects
 *! must implement the @[lfun::`<] method.
 *!
 *! @seealso
 *!   @[max()]
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

/*! @decl int|float|object max(int|float|object ... args)
 *! @decl string max(string ... args)
 *! @decl int(0..0) max()
 *!
 *! Returns the largest value among @[args]. Compared objects
 *! must implement the @[lfun::`<] method.
 *!
 *! @seealso
 *!   @[min()]
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
  zero.type=T_INT;
  zero.u.integer=0;

  check_all_args("abs",args,BIT_INT|BIT_FLOAT|BIT_OBJECT,0);
  pop_n_elems(args-1);
  if(is_lt(sp-1,&zero)) o_negate();
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
  check_all_args("sgn",args,BIT_MIXED,BIT_VOID|BIT_MIXED,0);
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
  
/* function(float:float) */
  ADD_EFUN("sin",f_sin,tFunc(tFlt,tFlt),0);
  
/* function(float:float) */
  ADD_EFUN("asin",f_asin,tFunc(tFlt,tFlt),0);
  
/* function(float:float) */
  ADD_EFUN("cos",f_cos,tFunc(tFlt,tFlt),0);
  
/* function(float:float) */
  ADD_EFUN("acos",f_acos,tFunc(tFlt,tFlt),0);
  
/* function(float:float) */
  ADD_EFUN("tan",f_tan,tFunc(tFlt,tFlt),0);
  
/* function(float:float) */
  ADD_EFUN("atan",f_atan,tFunc(tFlt,tFlt),0);
  
/* function(float,float:float) */
  ADD_EFUN("atan2",f_atan2,tFunc(tFlt tFlt,tFlt),0);
 
  ADD_EFUN("sinh",f_sinh,tFunc(tFlt,tFlt),0);
  ADD_EFUN("asinh",f_asinh,tFunc(tFlt,tFlt),0);
  ADD_EFUN("cosh",f_cosh,tFunc(tFlt,tFlt),0);
  ADD_EFUN("acosh",f_acosh,tFunc(tFlt,tFlt),0);
  ADD_EFUN("tanh",f_tanh,tFunc(tFlt,tFlt),0);
  ADD_EFUN("atanh",f_atanh,tFunc(tFlt,tFlt),0);
 
/* function(float:float)|function(int:int) */
  ADD_EFUN("sqrt",f_sqrt,tOr3(tFunc(tFlt,tFlt),
			      tFunc(tInt,tInt),
			      tFunc(tObj,tMix)),0);

/* function(float:float) */
  ADD_EFUN("log",f_log,tFunc(tFlt,tFlt),0);
  
/* function(float:float) */
  ADD_EFUN("exp",f_exp,tFunc(tNUM,tFlt),0);
  
/* function(float,float:float) */
  ADD_EFUN("pow",f_pow,
	   tOr5(tFunc(tFlt tFlt,tFlt),
		tFunc(tInt tFlt,tFlt),
		tFunc(tFlt tInt,tFlt),
		tFunc(tInt tInt,tInt),
		tFunc(tObj tOr3(tInt,tObj,tFlt),tMix)),0);
  
/* function(float:float) */
  ADD_EFUN("floor",f_floor,tFunc(tFlt,tFlt),0);
  
/* function(float:float) */
  ADD_EFUN("ceil",f_ceil,tFunc(tFlt,tFlt),0);

/* function(float:float) */
  ADD_EFUN("round",f_round,tFunc(tFlt,tFlt),0);

#define CMP_TYPE \
  tOr4(tFuncV(tNone,tString,tString), \
       tFunc(tVoid,tInt0), \
       tFuncV(tNone,tSetvar(0,tOr(tInt,tFloat)),tVar(0)), \
       tIfnot(tFuncV(tNone,tString,tMix), \
              tIfnot(tFuncV(tNone,tOr(tInt,tFloat),tMix), \
	             tIfnot(tFuncV(tNone,tNot(tOr(tObj,tMix)),tMix), \
		            tFuncV(tNone,tMix,tMix)))))
    /*
     * "!function(!object...:mixed)&function(mixed...:mixed)|" \
     * "function(int|zero...:int)|" \
     * "function(float...:float)|" \
     * "function(string...:string)|"  \
     * "function(:int(0..0)|" \
     * "!function(int...:mixed)&!function(float...:mixed)&function(int|float...:int|float)"
     */

  ADD_EFUN("max",f_max,CMP_TYPE,0);
  ADD_EFUN("min",f_min,CMP_TYPE,0);
  
/* function(float|int|object:float|int|object) */
  ADD_EFUN("abs",f_abs,tFunc(tOr3(tFlt,tInt,tObj),tOr3(tFlt,tInt,tObj)),0);
  
/* function(mixed,mixed|void:int) */
  ADD_EFUN("sgn",f_sgn,tFunc(tMix tOr(tMix,tVoid),tInt),0);
}

PIKE_MODULE_EXIT {}
