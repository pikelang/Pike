/*\
||| This file is part of Pike. For copyright information see COPYRIGHT.
||| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
||| for more information.
||| $Id: math_module.h,v 1.8 2002/10/08 20:22:33 nilsson Exp $
\*/
extern void init_math_matrix(void);
extern void init_math_fmatrix(void);
#ifdef INT64
extern void init_math_lmatrix(void);
#endif /* INT64 */
extern void init_math_imatrix(void);
extern void init_math_smatrix(void);
extern void exit_math_matrix(void);
extern void exit_math_fmatrix(void);
#ifdef INT64
extern void exit_math_lmatrix(void);
#endif /* INT64 */
extern void exit_math_imatrix(void);
extern void exit_math_smatrix(void);
extern void init_math_transforms(void);
extern void exit_math_transforms(void);
