/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#include "global.h"
#include "config.h"

#include <math.h>

#include "pike_macros.h"
#include "pike_error.h"
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

#include "pike_macros.h"
#include "math_module.h"

#include "bignum.h"

/*
**! module Math
**! class Matrix
**! class FMatrix
**! class LMatrix
**! class IMatrix
**! class SMatrix
**!
**!	These classes hold Matrix capabilites,
**!	and support a range of matrix operations.
**!
**!	Matrix only handles double precision floating point values
**!	FMatrix only handles single precision floating point values
**!	LMatrix only handles integers (64 bit)
**!	IMatrix only handles integers (32 bit)
**!	SMatrix only handles integers (16 bit)
**!	
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

/*
**! method void create(array(array(int|float)) 2d_matrix)
**! method void create(array(int|float) 1d_matrix)
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

/* ---------------------------------------------------------------- */

/*
**! method array(array) cast(string to_what)
**! method array(array) cast(string to_what)
**! 	This is to be able to get the matrix values.
**!	<tt>(array)</tt> gives back a double array of floats.
**!	<tt>m->vect()</tt> gives back an array of floats.
*/

/*
**! method array vect()
**!     Return all the elements of the matrix as an array of numbers
*/



/*
**! method Matrix transpose()
**! 	Transpose of the matrix as a new object.
*/


/*
**! method float norm()
**! method float norm2()
**! method Matrix normv()
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


/*
**! method Matrix `+(object with)
**! method Matrix ``+(object with)
**! method Matrix add(object with)
**! 	Add this matrix to another matrix. A new matrix is returned.
**!	The matrices must have the same size.
**!
**! method Matrix `-()
**! method Matrix `-(object with)
**! method Matrix ``-(object with)
**! method Matrix sub(object with)
**!	Subtracts this matrix from another. A new matrix is returned.
**!	-<i>m</i> is equal to -1*<i>m</i>.
*/


/* 
**! method Matrix sum()
**!	Produces the sum of all the elements in the matrix.
*/

/* 
**! method Matrix max()
**! method Matrix min()
**!	Produces the maximum or minimum value 
**!	of all the elements in the matrix.
*/

/*
**! method Matrix `*(object with)
**! method Matrix ``*(object with)
**! method Matrix mult(object with)
**!	Matrix multiplication.
*/
/*
**! method Matrix cross(object with)
**! method Matrix `×(object with)
**! method Matrix ``×(object with)
**!	Matrix cross-multiplication.
*/


/*
**! method float dot_product(object with)
**! method float `·(object with)
**! method float ``·(object with)
**!	Matrix dot product.
*/

/*
**! method Matrix convolve(object with)
**!	Convolve called matrix with the argument.
*/
