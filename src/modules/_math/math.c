/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
#include <math.h>
#include "interpret.h"
#include "constants.h"
#include "svalue.h"
#include "error.h"
#include "module_support.h"
#include "operators.h"

#ifdef PC
#undef PC
#endif /* PC */

#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif

#ifdef HAVE_FLOATINGPOINT_H
#include <floatingpoint.h>
#endif

RCSID("$Id: math.c,v 1.17 1999/03/31 13:12:37 mirar fake $");

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795080
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

void f_sin(INT32 args)
{
  if(args<1) error("Too few arguments to sin()\n");
  if(sp[-args].type!=T_FLOAT) error("Bad argument 1 to sin()\n");
  sp[-args].u.float_number=sin(sp[-args].u.float_number);
}

void f_asin(INT32 args)
{
  if(args<1) error("Too few arguments to asin()\n");
  if(sp[-args].type!=T_FLOAT) error("Bad argument 1 to asin()\n");
  sp[-args].u.float_number=asin(sp[-args].u.float_number);
}

void f_cos(INT32 args)
{
  if(args<1) error("Too few arguments to cos()\n");
  if(sp[-args].type!=T_FLOAT) error("Bad argument 1 to cos()\n");
  sp[-args].u.float_number=cos(sp[-args].u.float_number);
}

void f_acos(INT32 args)
{
  if(args<1) error("Too few arguments to acos()\n");
  if(sp[-args].type!=T_FLOAT) error("Bad argument 1 to acos()\n");
  sp[-args].u.float_number=acos(sp[-args].u.float_number);
}

void f_tan(INT32 args)
{
  float f;
  if(args<1) error("Too few arguments to tan()\n");
  if(sp[-args].type!=T_FLOAT) error("Bad argument 1 to tan()\n");

  f=(sp[-args].u.float_number-M_PI/2) / M_PI;
  if(f==floor(f+0.5))
  {
    error("Impossible tangent.\n");
    return;
  }
  sp[-args].u.float_number=tan(sp[-args].u.float_number);
}

void f_atan(INT32 args)
{
  if(args<1) error("Too few arguments to atan()\n");
  if(sp[-args].type!=T_FLOAT) error("Bad argument 1 to atan()\n");
  sp[-args].u.float_number=atan(sp[-args].u.float_number);
}

void f_atan2(INT32 args)
{
  if(args<2) error("Too few arguments to atan2()\n");
  if(sp[-args].type!=T_FLOAT) error("Bad argument 1 to atan2()\n");
  if(sp[-args+1].type!=T_FLOAT) error("Bad argument 2 to atan2()\n");
  sp[-args].u.float_number=
    atan2(sp[-args].u.float_number,sp[-args+1].u.float_number);
  pop_stack();
}

void f_sqrt(INT32 args)
{
  if(args<1) error("Too few arguments to sqrt()\n");

  if(sp[-args].type==T_INT)
  {
    unsigned INT32 n, b, s, y=0;
    unsigned INT16 x=0;
    
    n=sp[-args].u.integer;
    for(b=1<<(sizeof(INT32)*8-2); b; b>>=2)
    {
      x<<=1; s=b+y; y>>=1;
      if(n>=s)
      {
	x|=1; y|=b; n-=s;
      }
    }
    sp[-args].u.integer=x;
  }
  else if(sp[-args].type==T_FLOAT)
  {
    if (sp[-args].u.float_number< 0.0)
    {
      error("math: sqrt(x) with (x < 0.0)\n");
      return;
    }
    sp[-args].u.float_number=sqrt(sp[-args].u.float_number);
  }else{
    error("Bad argument 1 to sqrt().\n");
  }
}

void f_log(INT32 args)
{
  if(args<1) error("Too few arguments to log()\n");
  if(sp[-args].type!=T_FLOAT) error("Bad argument 1 to log()\n");
  if(sp[-args].u.float_number <=0.0)
    error("Log on number less or equal to zero.\n");

  sp[-args].u.float_number=log(sp[-args].u.float_number);
}

void f_exp(INT32 args)
{
  if(args<1) error("Too few arguments to exp()\n");
  if(sp[-args].type!=T_FLOAT) error("Bad argument 1 to exp()\n");
  sp[-args].u.float_number=exp(sp[-args].u.float_number);
}

void f_pow(INT32 args)
{
  float tmp;
  if(args<2) error("Too few arguments to pow()\n");
  if(sp[-args].type!=T_FLOAT) error("Bad argument 1 to pow()\n");
  if(sp[1-args].type!=T_FLOAT) error("Bad argument 2 to pow()\n");
  tmp = pow(sp[-args].u.float_number, sp[1-args].u.float_number);
  sp[-args].u.float_number = tmp;
  pop_n_elems(args-1);
}

void f_floor(INT32 args)
{
  if(args<1) error("Too few arguments to floor()\n");
  if(sp[-args].type!=T_FLOAT) error("Bad argument 1 to floor()\n");
  sp[-args].u.float_number=floor(sp[-args].u.float_number);
}

void f_ceil(INT32 args)
{
  if(args<1) error("Too few arguments to ceil()\n");
  if(sp[-args].type!=T_FLOAT) error("Bad argument 1 to ceil()\n");
  sp[-args].u.float_number=ceil(sp[-args].u.float_number);
}

void f_min(INT32 args)
{
  INT32 i;
  if(!args) error("Too few arguments to min()\n");
  for(i=args-2;i>=0;i--)
    if(is_gt(sp-args+i,sp-args+1+i))
      assign_svalue(sp-args+i,sp-args+1+i);
  pop_n_elems(args-1);
}

void f_max(INT32 args)
{
  INT32 i;
  if(!args) error("Too few arguments to max()\n");
  for(i=args-2;i>=0;i--)
    if(is_lt(sp-args+i,sp-args+1+i))
      assign_svalue(sp-args+i,sp-args+1+i);
  pop_n_elems(args-1);
}

void f_abs(INT32 args)
{
  struct svalue zero;
  zero.type=T_INT;
  zero.u.integer=0;

  check_all_args("abs",args,BIT_INT|BIT_FLOAT|BIT_OBJECT,0);
  pop_n_elems(args-1);
  if(is_lt(sp-1,&zero)) o_negate();
}

void f_sgn(INT32 args)
{
  struct svalue zero;
  zero.type=T_INT;
  zero.u.integer=0;

  check_all_args("sgn",args,BIT_MIXED,BIT_VOID|BIT_MIXED,0);
  if(args<2) push_int(0);

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

void pike_module_init(void)
{
#ifdef HAVE_FPSETMASK
  fpsetmask(0);
#endif
  
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
  
/* function(float:float)|function(int:int) */
  ADD_EFUN("sqrt",f_sqrt,tOr(tFunc(tFlt,tFlt),tFunc(tInt,tInt)),0);
  
/* function(float:float) */
  ADD_EFUN("log",f_log,tFunc(tFlt,tFlt),0);
  
/* function(float:float) */
  ADD_EFUN("exp",f_exp,tFunc(tFlt,tFlt),0);
  
/* function(float,float:float) */
  ADD_EFUN("pow",f_pow,tFunc(tFlt tFlt,tFlt),0);
  
/* function(float:float) */
  ADD_EFUN("floor",f_floor,tFunc(tFlt,tFlt),0);
  
/* function(float:float) */
  ADD_EFUN("ceil",f_ceil,tFunc(tFlt,tFlt),0);

#define CMP_TYPE \
  "!function(!object...:mixed)&function(mixed...:mixed)|" \
  "function(int...:int)|" \
  "function(float...:float)|" \
  "function(string...:string)|"  \
  "!function(int...:mixed)&!function(float...:mixed)&function(int|float...:int|float)"
    
  add_efun("max",f_max,CMP_TYPE,0);
  add_efun("min",f_min,CMP_TYPE,0);
  
/* function(float|int|object:float|int|object) */
  ADD_EFUN("abs",f_abs,tFunc(tOr3(tFlt,tInt,tObj),tOr3(tFlt,tInt,tObj)),0);
  
/* function(mixed,mixed|void:int) */
  ADD_EFUN("sgn",f_sgn,tFunc(tMix tOr(tMix,tVoid),tInt),0);
}

void pike_module_exit(void) {}
