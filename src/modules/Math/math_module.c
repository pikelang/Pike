/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "module.h"
#include "config.h"
#include "pike_float.h"

#include "math_module.h"
#include "transforms.h"


/*** module init & exit & stuff *****************************************/

/* add other parsers here */

struct program *math_matrix_program;
struct program *math_imatrix_program;
struct program *math_fmatrix_program;
struct program *math_smatrix_program;
struct program *math_lmatrix_program;
struct program *math_transforms_program;

static const struct math_class
{
   char *name;
   void (*func)(void);
   struct program **pd;
} sub[] = {
   {"Matrix",init_math_matrix,&math_matrix_program},
   {"IMatrix",init_math_imatrix,&math_imatrix_program},
   {"LMatrix",init_math_lmatrix,&math_lmatrix_program},
   {"FMatrix",init_math_fmatrix,&math_fmatrix_program},
   {"SMatrix",init_math_smatrix,&math_smatrix_program},
   {"Transforms",init_math_transforms,&math_transforms_program},
};

/*! @module Math */

/*! @decl constant pi
 *! The constant pi (3.14159265358979323846).
 */

/*! @decl constant e
 *! The constant e (2.7182818284590452354).
 */

/*! @decl constant inf
 *! Floating point infinity.
 */

/*! @decl constant nan
 *! Floating point not-a-number (e.g. inf/inf).
 *! @seealso
 *!   @[Float.isnan()]
 */

/*! @endmodule */

PIKE_MODULE_EXIT
{
   int i;
   for (i=0; i<(int)(sizeof(sub)/sizeof(sub[0])); i++)
      if (sub[i].pd && sub[i].pd[0])
	 free_program(sub[i].pd[0]);

   exit_math_matrix();
   exit_math_imatrix();
   exit_math_fmatrix();
   exit_math_smatrix();
   exit_math_transforms();
}


#ifdef HAS_MPI
void matrix_op_create(INT32 args);
void imatrix_op_create(INT32 args);
void lmatrix_op_create(INT32 args);
void fmatrix_op_create(INT32 args);
void smatrix_op_create(INT32 args);
#endif

PIKE_MODULE_INIT
{
   int i;
   for (i=0; i<(int)(sizeof(sub)/sizeof(sub[0])); i++)
   {
      struct program *p;

      start_new_program();
      sub[i].func();
      p=end_program();
      add_program_constant(sub[i].name,p,0);
      if (sub[i].pd) sub[i].pd[0]=p;
      else free_program(p);
   }

   add_float_constant("pi",3.14159265358979323846  ,ID_LOCAL);
   add_float_constant("e", 2.7182818284590452354   ,ID_LOCAL);
   add_float_constant("inf", MAKE_INF(), ID_LOCAL);
   add_float_constant("nan", MAKE_NAN(), ID_LOCAL);
#ifdef HAS_MPI
   add_function_constant("Matrix_op_create", matrix_op_create, "function(function,int,int,int:object)", 0);
   add_function_constant("IMatrix_op_create", imatrix_op_create, "function(function,int,int,int:object)", 0);
   add_function_constant("LMatrix_op_create", lmatrix_op_create, "function(function,int,int,int:object)", 0);
   add_function_constant("FMatrix_op_create", fmatrix_op_create, "function(function,int,int,int:object)", 0);
   add_function_constant("SMatrix_op_create", smatrix_op_create, "function(function,int,int,int:object)", 0);
#endif
}
