/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: fsort.h,v 1.2 1998/03/28 15:16:30 grubba Exp $
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
