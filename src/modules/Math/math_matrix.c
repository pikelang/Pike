/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: math_matrix.c,v 1.38 2004/03/10 16:32:26 nilsson Exp $
*/

#include "global.h"
#include "config.h"

#include <math.h>

#include "pike_error.h"
#include "interpret.h"
#include "svalue.h"
#include "operators.h"
#include "builtin_functions.h"
#include "module_support.h"

#include "math_module.h"

#include "bignum.h"

/*! @module Math
 */

/*! @class Matrix
 *! Matrix representation with double precision floating point values.
 */

static struct pike_string *s_array;
static struct pike_string *s__clr;
static struct pike_string *s_identity;
static struct pike_string *s_rotate;

extern struct program *math_matrix_program;
extern struct program *math_smatrix_program;
extern struct program *math_imatrix_program;
extern struct program *math_fmatrix_program;
#ifdef INT64
extern struct program *math_lmatrix_program;
#endif /* INT64 */

#define FTYPE double
#define PTYPE "float"
#define matrixX(X) PIKE_CONCAT(matrix,X)
#define Xmatrix(X) PIKE_CONCAT(X,matrix)
#define XmatrixY(X,Y) PIKE_CONCAT3(X,matrix,Y)
#define PUSH_ELEM( X )  push_float( (FLOAT_TYPE)(X) )
#include <matrix_code.h>
#undef PUSH_ELEM
#undef matrixX
#undef Xmatrix
#undef XmatrixY
#undef FTYPE
#undef PTYPE

#define FTYPE int
#define PTYPE "int"
#define matrixX(X) PIKE_CONCAT(imatrix,X)
#define Xmatrix(X) PIKE_CONCAT(X,imatrix)
#define XmatrixY(X,Y) PIKE_CONCAT3(X,imatrix,Y)
#define PUSH_ELEM( X )  push_int( (INT_TYPE)(X) )
#include <matrix_code.h>
#undef PUSH_ELEM
#undef Xmatrix
#undef matrixX
#undef XmatrixY
#undef FTYPE
#undef PTYPE

#ifdef INT64
#define FTYPE INT64
#define PTYPE "int"
#define matrixX(X) PIKE_CONCAT(lmatrix,X)
#define Xmatrix(X) PIKE_CONCAT(X,lmatrix)
#define XmatrixY(X,Y) PIKE_CONCAT3(X,lmatrix,Y)
#define PUSH_ELEM( X )  push_int64( (INT64)(X) )
#include <matrix_code.h>
#undef PUSH_ELEM
#undef Xmatrix
#undef matrixX
#undef XmatrixY
#undef FTYPE
#undef PTYPE
#endif /* INT64 */

#define FTYPE float
#define PTYPE "float"
#define matrixX(X) PIKE_CONCAT(fmatrix,X)
#define Xmatrix(X) PIKE_CONCAT(X,fmatrix)
#define XmatrixY(X,Y) PIKE_CONCAT3(X,fmatrix,Y)
#define PUSH_ELEM( X )  push_float( (FLOAT_TYPE)(X) )
#include <matrix_code.h>
#undef PUSH_ELEM
#undef Xmatrix
#undef matrixX
#undef XmatrixY
#undef FTYPE
#undef PTYPE

#define FTYPE short
#define PTYPE "int"
#define matrixX(X) PIKE_CONCAT(smatrix,X)
#define Xmatrix(X) PIKE_CONCAT(X,smatrix)
#define XmatrixY(X,Y) PIKE_CONCAT3(X,smatrix,Y)
#define PUSH_ELEM( X )  push_int( (INT_TYPE)(X) )
#include <matrix_code.h>
#undef PUSH_ELEM
#undef Xmatrix
#undef matrixX
#undef XmatrixY
#undef FTYPE
#undef PTYPE

/*! @decl void create(array(array(int|float)) matrix_2d)
 *! @decl void create(array(int|float) matrix_1d)
 *!  Initializes the matrix as a 1D or 2D matrix, e.g.
 *!  @expr{Math.Matrix( ({({1,2}),({3,4})}) )@}.
 */

/*! @decl void create(int n,int m)
 *! @decl void create(int n,int m,string type)
 *! @decl void create(int n,int m,float|int init)
 *! Initializes the matrix as to be a @[n]*@[m] matrix with @[init] in
 *! every value. If no third argument is given, or the third argument
 *! is @expr{"identity"@}, the matrix will be initialized with all
 *! zeroes except for the diagonal which will be @expr{1@}.
 */

/*! @decl void create(string type,int size)
 *! When @[type] is @expr{"identity"@} the matrix is initializes as a
 *! square identity matrix.
 */

/*! @decl void create(string type,int size,float rads,Matrix axis)
 *! @decl void create(string type,int size,float rads,float x,float y,float z)
 *!
 *! When @[type] is @expr{"rotate"@} the matrix is initialized as a
 *! rotation matrix.
 */

/* ---------------------------------------------------------------- */

/*! @decl array(array) cast(string to_what)
 *! @decl array(array) cast(string to_what)
 *!
 *! It is possible to cast the matrix to an array and get back a
 *! double array of floats with the matrix values.
 *! @seealso
 *!   @[vect]
 */

/*! @decl array vect()
 *! Return all the elements of the matrix as an array of numbers
 */

/*! @decl Matrix transpose()
 *! Returns the transpose of the matrix as a new object.
 */

/*! @decl float norm()
 *! @decl float norm2()
 *! @decl Matrix normv()
 *! 	Norm of the matrix, and the square of the norm
 *!	of the matrix. (The later method is because you
 *!	may skip a square root sometimes.)
 *!
 *!	This equals |A| or sqrt( A@sub{0@}@sup{2@} +
 *!	A@sub{1@}@sup{2@} + ... + A@sub{n@}@sup{2@} ).
 *!
 *!	It is only usable with 1xn or nx1 matrices.
 *!
 *!	@expr{m->normv()@} is equal to @expr{m*(1.0/m->norm())@},
 *!	with the exception that the zero vector will still be
 *!	the zero vector (no error).
 */

/*! @decl Matrix `+(object with)
 *! @decl Matrix ``+(object with)
 *! @decl Matrix add(object with)
 *! 	Add this matrix to another matrix. A new matrix is returned.
 *!	The matrices must have the same size.
 *!
 *! @decl Matrix `-()
 *! @decl Matrix `-(object with)
 *! @decl Matrix ``-(object with)
 *! @decl Matrix sub(object with)
 *!	Subtracts this matrix from another. A new matrix is returned.
 *!	@expr{-m@} is equal to @expr{-1*m@}.
 */

/*! @decl Matrix sum()
 *!	Produces the sum of all the elements in the matrix.
 */

/*! @decl Matrix max()
 *! @decl Matrix min()
 *! Produces the maximum or minimum value of all the elements in the
 *! matrix.
 */

/*! @decl Matrix `*(object with)
 *! @decl Matrix ``*(object with)
 *! @decl Matrix mult(object with)
 *!	Matrix multiplication.
 */

/*! @decl Matrix cross(object with)
 *!	Matrix cross-multiplication.
 */


/*! @decl float dot_product(object with)
 *!	Matrix dot product.
 */

/*! @decl Matrix convolve(object with)
 *!	Convolve called matrix with the argument.
 */

/*! @endclass
 */

/*! @class FMatrix
 *! @inherit Matrix
 *! Matrix representation with single precision floating point values.
 *! @endclass
 */

/*! @class LMatrix
 *! @inherit Matrix
 *! Matrix representation with 64 bit integer values.
 *! @endclass
 */

/*! @class IMatrix
 *! @inherit Matrix
 *! Matrix representation with 32 bit integer values.
 *! @endclass
 */

/*! @class SMatrix
 *! @inherit Matrix
 *! Matrix representation with 16 bit integer values.
 *! @endclass
 */

/*! @endmodule
 */
