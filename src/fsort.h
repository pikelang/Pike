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
