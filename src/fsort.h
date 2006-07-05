/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: fsort.h,v 1.7 2006/07/05 19:28:09 mast Exp $
*/

#ifndef FSORT_H
#define FSORT_H

typedef int (*fsortfun)(const void *,const void *);

/* Prototypes begin here */
PMOD_EXPORT void fsort(void *base,
		       long elms,
		       long elmSize,
		       fsortfun cmpfunc);
/* Prototypes end here */


#endif
