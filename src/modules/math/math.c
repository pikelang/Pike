/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include <math.h>
#include "interpret.h"
#include "constants.h"
#include "svalue.h"

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795080
#endif

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
  if(args<2) error("Too few arguments to pow()\n");
  if(sp[-args].type!=T_FLOAT) error("Bad argument 1 to pow()\n");
  if(sp[1-args].type!=T_FLOAT) error("Bad argument 2 to pow()\n");
  sp[-args].u.float_number=pow(sp[-args].u.float_number,
			       sp[1-args].u.float_number);
  sp--;
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


void pike_module_init()
{
  add_efun("sin",f_sin,"function(float:float)",0);
  add_efun("asin",f_asin,"function(float:float)",0);
  add_efun("cos",f_cos,"function(float:float)",0);
  add_efun("acos",f_acos,"function(float:float)",0);
  add_efun("tan",f_tan,"function(float:float)",0);
  add_efun("atan",f_atan,"function(float:float)",0);
  add_efun("sqrt",f_sqrt,"function(float:float)|function(int:int)",0);
  add_efun("log",f_log,"function(float:float)",0);
  add_efun("exp",f_exp,"function(float:float)",0);
  add_efun("pow",f_pow,"function(float,float:float)",0);
  add_efun("floor",f_floor,"function(float:float)",0);
  add_efun("ceil",f_ceil,"function(float:float)",0);
}

void pike_module_exit() {}
