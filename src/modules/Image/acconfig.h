/*\
||| This file is part of Pike. For copyright information see COPYRIGHT.
||| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
||| for more information.
||| $Id: acconfig.h,v 1.6 2002/10/08 20:22:30 nilsson Exp $
\*/

#ifndef IMAGE_MACHINE_H
#define IMAGE_MACHINE_H

/* nasm exists and can be used to make .o-files */
#undef ASSEMBLY_OK

/* Define if you have the m library (-lm).  */
#undef HAVE_LIBM

@TOP@
@BOTTOM@

#endif
