/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: fsort.h,v 1.6 2002/10/11 01:39:31 nilsson Exp $
*/

#ifndef FSORT_H
#define FSORT_H

typedef int (*fsortfun)(const void *,const void *);

/* Prototypes begin here */
void fsort(void *base,
	   long elms,
	   long elmSize,
	   fsortfun cmpfunc);
/* Prototypes end here */


#endif
